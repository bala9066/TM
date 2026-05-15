/**************************************************************************
 * File Name: BatchModel.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of batch process model
 * Requirements: REQ-PM-039 to REQ-PM-057
 **************************************************************************/

#include "BatchModel.h"
#include "utils/LogManager.h"

namespace TestMATE {

CBatchModel::CBatchModel(TUInt32 in_uiConcurrency)
    : m_uiConcurrency(in_uiConcurrency)
{
    if (m_uiConcurrency == 0) {
        m_uiConcurrency = 1;
    }

    CLogManager::GetInstance().LogInfo("BatchModel",
        "Initialized with concurrency {}", m_uiConcurrency);
}

CBatchModel::~CBatchModel() {
    if (GetState() == EProcessModelState::kRunning) {
        Stop();
    }
}

TUInt64 CBatchModel::AddJob(SBatchJob in_job) {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    TUInt64 jobId = m_uiNextJobId++;
    in_job.jobId = jobId;
    in_job.state = EExecutionState::kIdle;

    m_mapJobs[jobId] = std::move(in_job);
    m_queuePending.push(jobId);

    CLogManager::GetInstance().LogDebug("BatchModel",
        "Added job {} to queue", jobId);

    m_jobCV.notify_one();

    return jobId;
}

CResult CBatchModel::RemoveJob(TUInt64 in_jobId) {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    auto it = m_mapJobs.find(in_jobId);
    if (it == m_mapJobs.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Job not found: " + std::to_string(in_jobId));
    }

    if (m_setRunning.count(in_jobId) > 0) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Cannot remove running job");
    }

    m_mapJobs.erase(it);
    m_setCompleted.erase(in_jobId);

    // Remove from pending queue (rebuild without this ID)
    std::queue<TUInt64> newQueue;
    while (!m_queuePending.empty()) {
        TUInt64 id = m_queuePending.front();
        m_queuePending.pop();
        if (id != in_jobId) {
            newQueue.push(id);
        }
    }
    m_queuePending = std::move(newQueue);

    return TESTMATE_SUCCESS();
}

std::optional<SBatchJob> CBatchModel::GetJobStatus(TUInt64 in_jobId) const {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    auto it = m_mapJobs.find(in_jobId);
    if (it != m_mapJobs.end()) {
        return it->second;
    }
    return std::nullopt;
}

TUInt32 CBatchModel::GetPendingJobCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);
    return static_cast<TUInt32>(m_queuePending.size());
}

TUInt32 CBatchModel::GetRunningJobCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);
    return static_cast<TUInt32>(m_setRunning.size());
}

TUInt32 CBatchModel::GetCompletedJobCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);
    return static_cast<TUInt32>(m_setCompleted.size());
}

void CBatchModel::ClearCompletedJobs() {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    for (TUInt64 id : m_setCompleted) {
        m_mapJobs.erase(id);
    }
    m_setCompleted.clear();
}

void CBatchModel::SetConcurrency(TUInt32 in_uiConcurrency) {
    if (in_uiConcurrency > 0) {
        m_uiConcurrency = in_uiConcurrency;
    }
}

CResult CBatchModel::Start() {
    if (GetState() == EProcessModelState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Batch already running");
    }

    m_bShouldStop = false;

    // Create thread pool
    m_pThreadPool = std::make_unique<CThreadPool>(m_uiConcurrency);

    SetState(EProcessModelState::kRunning);

    // Start dispatcher thread
    m_bDispatcherRunning = true;
    m_pDispatcherThread = std::make_unique<std::thread>([this]() {
        JobDispatcherFunc();
    });

    CLogManager::GetInstance().LogInfo("BatchModel",
        "Started with concurrency {}", m_uiConcurrency);

    return TESTMATE_SUCCESS();
}

CResult CBatchModel::Stop() {
    if (GetState() != EProcessModelState::kRunning &&
        GetState() != EProcessModelState::kPaused) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Batch not running");
    }

    m_bShouldStop = true;
    m_jobCV.notify_all();

    // Wait for dispatcher to finish
    if (m_pDispatcherThread && m_pDispatcherThread->joinable()) {
        m_pDispatcherThread->join();
    }
    m_pDispatcherThread.reset();

    // Shutdown thread pool
    if (m_pThreadPool) {
        m_pThreadPool->Shutdown(true);
        m_pThreadPool.reset();
    }

    SetState(EProcessModelState::kIdle);

    CLogManager::GetInstance().LogInfo("BatchModel", "Stopped");

    return TESTMATE_SUCCESS();
}

void CBatchModel::JobDispatcherFunc() {
    while (!m_bShouldStop) {
        SBatchJob* pJob = nullptr;

        {
            std::unique_lock<std::mutex> lock(m_jobMutex);

            // Wait for a job or stop signal
            m_jobCV.wait(lock, [this]() {
                return m_bShouldStop ||
                       (!m_queuePending.empty() &&
                        m_setRunning.size() < m_uiConcurrency);
            });

            if (m_bShouldStop) {
                break;
            }

            pJob = GetNextJob();
            if (pJob) {
                pJob->state = EExecutionState::kRunning;
                pJob->startTime = std::chrono::steady_clock::now();
                m_setRunning.insert(pJob->jobId);
            }
        }

        if (pJob) {
            TUInt64 jobId = pJob->jobId;

            // Submit job to thread pool. ExecuteJob takes the lock only for
            // bookkeeping — the job body itself runs unlocked.
            m_pThreadPool->Submit([this, jobId]() {
                ExecuteJob(jobId);
            });
        }
    }

    m_bDispatcherRunning = false;
}

SBatchJob* CBatchModel::GetNextJob() {
    // Must be called with m_jobMutex held

    if (m_queuePending.empty()) {
        return nullptr;
    }

    TUInt64 jobId = m_queuePending.front();
    m_queuePending.pop();

    auto it = m_mapJobs.find(jobId);
    if (it != m_mapJobs.end()) {
        return &it->second;
    }

    return nullptr;
}

void CBatchModel::ExecuteJob(TUInt64 in_jobId) {
    // Validate the job still exists.
    {
        std::lock_guard<std::mutex> lock(m_jobMutex);
        if (m_mapJobs.find(in_jobId) == m_mapJobs.end()) {
            return;
        }
    }

    CLogManager::GetInstance().LogInfo("BatchModel", "Executing job {}", in_jobId);

    // Placeholder for actual test execution. CRITICAL: this work runs
    // WITHOUT m_jobMutex held — otherwise every "parallel" job serializes
    // on the lock and effective concurrency collapses to one.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Take the lock only to record results and update tracking sets.
    {
        std::lock_guard<std::mutex> lock(m_jobMutex);

        auto it = m_mapJobs.find(in_jobId);
        if (it == m_mapJobs.end()) {
            return;
        }
        SBatchJob& job = it->second;

        job.passCount = 10;
        job.failCount = 0;
        job.verdict = ETestVerdict::kPass;
        job.state = EExecutionState::kCompleted;
        job.endTime = std::chrono::steady_clock::now();

        m_setRunning.erase(in_jobId);
        m_setCompleted.insert(in_jobId);

        if (m_bStopOnFirstFailure && job.verdict == ETestVerdict::kFail) {
            m_bShouldStop = true;
        }

        // Check if all jobs complete
        if (m_queuePending.empty() && m_setRunning.empty()) {
            SetState(EProcessModelState::kCompleted);
        }
    }

    // Notify dispatcher/waiters after releasing the lock.
    m_jobCV.notify_all();
}

bool CBatchModel::WaitForCompletion(TInt64 in_timeoutMs) {
    auto start = std::chrono::steady_clock::now();

    while (GetState() == EProcessModelState::kRunning) {
        if (in_timeoutMs > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();

            if (elapsed >= in_timeoutMs) {
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;
}

TUInt32 CBatchModel::GetTotalPassCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    TUInt32 total = 0;
    for (const auto& [id, job] : m_mapJobs) {
        total += job.passCount;
    }
    return total;
}

TUInt32 CBatchModel::GetTotalFailCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    TUInt32 total = 0;
    for (const auto& [id, job] : m_mapJobs) {
        total += job.failCount;
    }
    return total;
}

TUInt32 CBatchModel::GetSuccessfulJobCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    TUInt32 count = 0;
    for (TUInt64 id : m_setCompleted) {
        auto it = m_mapJobs.find(id);
        if (it != m_mapJobs.end() && it->second.verdict == ETestVerdict::kPass) {
            ++count;
        }
    }
    return count;
}

TUInt32 CBatchModel::GetFailedJobCount() const {
    std::lock_guard<std::mutex> lock(m_jobMutex);

    TUInt32 count = 0;
    for (TUInt64 id : m_setCompleted) {
        auto it = m_mapJobs.find(id);
        if (it != m_mapJobs.end() && it->second.verdict == ETestVerdict::kFail) {
            ++count;
        }
    }
    return count;
}

} // namespace TestMATE
