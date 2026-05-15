/**************************************************************************
 * File Name: TestOrchestrator.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Test orchestration system implementation
 **************************************************************************/

#include "testmate/orchestration/TestOrchestrator.h"
#include <algorithm>
#include <thread>

namespace TestMATE {

//=============================================================================
// CTestOrchestrator Implementation
//=============================================================================

CTestOrchestrator::CTestOrchestrator(const SOrchestratorConfig& in_config)
    : m_config(in_config)
{
}

CTestOrchestrator::~CTestOrchestrator() {
    if (m_isRunning) {
        [[maybe_unused]] auto result = Stop();
    }
}

CResult CTestOrchestrator::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Orchestrator already running");
    }

    m_isRunning = true;

    // Start worker threads
    for (TUInt32 i = 0; i < m_config.maxParallelJobs; ++i) {
        m_workerThreads.emplace_back(&CTestOrchestrator::WorkerThread, this);
    }

    return TESTMATE_SUCCESS();
}

CResult CTestOrchestrator::Stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isRunning) {
            return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Orchestrator not running");
        }

        m_isRunning = false;
    }

    // Notify all workers to wake up and exit
    m_cv.notify_all();

    // Wait for all workers to finish
    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    m_workerThreads.clear();

    return TESTMATE_SUCCESS();
}

CResult CTestOrchestrator::SubmitJob(const STestJob& in_job) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Orchestrator not running");
    }

    if (m_jobs.size() >= m_config.maxQueueSize) {
        return TESTMATE_FAILURE(EErrorCode::kResourceBusy, "Job queue is full");
    }

    if (m_jobs.count(in_job.jobId) > 0) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists, "Job ID already exists");
    }

    // Store job
    STestJob job = in_job;
    job.submittedTime = std::chrono::steady_clock::now();
    m_jobs[job.jobId] = job;

    m_totalJobs++;

    // Notify workers
    m_cv.notify_one();

    return TESTMATE_SUCCESS();
}

CResult CTestOrchestrator::CancelJob(const TString& in_jobId) {
    STestJob jobSnapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_jobs.find(in_jobId);
        if (it == m_jobs.end()) {
            return TESTMATE_FAILURE(EErrorCode::kNotFound, "Job not found");
        }

        if (it->second.isCompleted) {
            return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Job already completed");
        }

        // Mark as failed/cancelled
        it->second.isCompleted = true;
        it->second.isSuccessful = false;
        it->second.errorMessage = "Job cancelled by user";
        it->second.endTime = std::chrono::steady_clock::now();

        m_failedJobs++;
        m_completedJobs.insert(in_jobId);
        jobSnapshot = it->second;
    }

    // Notify outside the lock: the callback may re-enter the orchestrator.
    NotifyJobStatus(jobSnapshot);
    // Wake workers so dependents can re-evaluate.
    m_cv.notify_all();

    return TESTMATE_SUCCESS();
}

std::optional<STestJob> CTestOrchestrator::GetJobStatus(const TString& in_jobId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_jobs.find(in_jobId);
    if (it == m_jobs.end()) {
        return std::nullopt;
    }

    return it->second;
}

TVector<STestJob> CTestOrchestrator::GetAllJobs() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<STestJob> jobs;
    jobs.reserve(m_jobs.size());

    for (const auto& [id, job] : m_jobs) {
        jobs.push_back(job);
    }

    return jobs;
}

CResult CTestOrchestrator::RegisterResource(
    const TString& in_resourceId,
    const TString& in_resourceType,
    TUInt32 in_maxConcurrent) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_resources.count(in_resourceId) > 0) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists, "Resource already registered");
    }

    SResource resource;
    resource.resourceId = in_resourceId;
    resource.resourceType = in_resourceType;
    resource.maxConcurrent = in_maxConcurrent;
    resource.currentUsage = 0;
    resource.isAvailable = true;

    m_resources[in_resourceId] = resource;

    return TESTMATE_SUCCESS();
}

CResult CTestOrchestrator::ReleaseResource(const TString& in_resourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.find(in_resourceId);
    if (it == m_resources.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Resource not found");
    }

    if (it->second.currentUsage > 0) {
        it->second.currentUsage--;
    }

    return TESTMATE_SUCCESS();
}

SOrchestrationStats CTestOrchestrator::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    SOrchestrationStats stats;
    stats.totalJobs = m_totalJobs.load();
    stats.completedJobs = m_completedJobCount.load();
    stats.failedJobs = m_failedJobs.load();
    stats.activeJobs = static_cast<TUInt32>(m_activeJobs.size());
    stats.queuedJobs = static_cast<TUInt32>(m_jobs.size() - m_completedJobs.size() - m_activeJobs.size());

    if (stats.totalJobs > 0) {
        stats.successRate = static_cast<TDouble>(stats.completedJobs - stats.failedJobs) / stats.totalJobs;
    }

    // Calculate average execution time
    TDouble totalTime = 0.0;
    TUInt32 completedCount = 0;

    for (const auto& [id, job] : m_jobs) {
        if (job.isCompleted && job.isSuccessful) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                job.endTime - job.startTime);
            totalTime += duration.count();
            completedCount++;
        }
    }

    if (completedCount > 0) {
        stats.averageExecutionTime = totalTime / completedCount;
    }

    return stats;
}

void CTestOrchestrator::SetJobStatusCallback(FJobStatusCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusCallback = std::move(in_callback);
}

bool CTestOrchestrator::WaitForCompletion(TUInt32 in_timeoutMs) {
    auto deadline = in_timeoutMs > 0
        ? std::chrono::steady_clock::now() + std::chrono::milliseconds(in_timeoutMs)
        : std::chrono::time_point<std::chrono::steady_clock>::max();

    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            bool allComplete = true;
            for (const auto& [id, job] : m_jobs) {
                if (!job.isCompleted) {
                    allComplete = false;
                    break;
                }
            }

            if (allComplete && m_jobs.size() > 0) {
                return true;
            }

            if (m_jobs.empty()) {
                return true;  // No jobs to wait for
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;  // Timeout
}

void CTestOrchestrator::WorkerThread() {
    while (m_isRunning) {
        std::optional<STestJob> job;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Wait for runnable work or shutdown. The predicate must test
            // for an actually-runnable job: m_jobs is never emptied, so
            // "!m_jobs.empty()" would be permanently true and every worker
            // would busy-spin at 100% CPU once any job finished.
            m_cv.wait(lock, [this]() {
                return !m_isRunning || HasRunnableJob();
            });

            if (!m_isRunning) {
                break;
            }

            // Get next job
            job = GetNextJob();
        }

        if (job) {
            ExecuteJob(*job);
        }
    }
}

std::optional<STestJob> CTestOrchestrator::GetNextJob() {
    // Find highest priority job that:
    // 1. Is not completed
    // 2. Is not currently active
    // 3. Has dependencies satisfied
    // 4. Has resources available

    STestJob* bestJob = nullptr;
    ETestPriority bestPriority = ETestPriority::kLow;

    for (auto& [id, job] : m_jobs) {
        // Skip completed or active jobs
        if (job.isCompleted || m_activeJobs.count(id) > 0) {
            continue;
        }

        // Check dependencies
        if (m_config.enableDependencyTracking && !AreDependenciesSatisfied(job)) {
            continue;
        }

        // Check resources
        if (m_config.enableResourceManagement && !AcquireResources(job)) {
            continue;
        }

        // Check priority
        if (!bestJob || job.priority > bestPriority) {
            if (bestJob) {
                // Release resources from previous candidate
                ReleaseJobResources(*bestJob);
            }
            bestJob = &job;
            bestPriority = job.priority;
        } else {
            // Not selected, release resources
            ReleaseJobResources(job);
        }
    }

    if (bestJob) {
        m_activeJobs.insert(bestJob->jobId);
        return *bestJob;
    }

    return std::nullopt;
}

bool CTestOrchestrator::AreDependenciesSatisfied(const STestJob& in_job) const {
    for (const auto& depId : in_job.dependencies) {
        auto it = m_jobs.find(depId);
        if (it == m_jobs.end()) {
            return false;  // Dependency not found
        }

        if (!it->second.isCompleted || !it->second.isSuccessful) {
            return false;  // Dependency not completed successfully
        }
    }

    return true;
}

bool CTestOrchestrator::AreResourcesAvailable(const STestJob& in_job) const {
    for (const auto& resourceId : in_job.resources) {
        auto it = m_resources.find(resourceId);
        if (it == m_resources.end()) {
            return false;  // Resource not registered
        }
        if (it->second.currentUsage >= it->second.maxConcurrent) {
            return false;  // Resource at capacity
        }
    }
    return true;
}

bool CTestOrchestrator::HasRunnableJob() const {
    for (const auto& [id, job] : m_jobs) {
        if (job.isCompleted || m_activeJobs.count(id) > 0) {
            continue;
        }
        if (m_config.enableDependencyTracking && !AreDependenciesSatisfied(job)) {
            continue;
        }
        if (m_config.enableResourceManagement && !AreResourcesAvailable(job)) {
            continue;
        }
        return true;
    }
    return false;
}

bool CTestOrchestrator::AcquireResources(const STestJob& in_job) {
    // Check if all resources are available
    if (!AreResourcesAvailable(in_job)) {
        return false;
    }

    // Acquire all resources
    for (const auto& resourceId : in_job.resources) {
        m_resources[resourceId].currentUsage++;
    }

    return true;
}

void CTestOrchestrator::ReleaseJobResources(const STestJob& in_job) {
    for (const auto& resourceId : in_job.resources) {
        auto it = m_resources.find(resourceId);
        if (it != m_resources.end() && it->second.currentUsage > 0) {
            it->second.currentUsage--;
        }
    }
}

void CTestOrchestrator::ExecuteJob(STestJob& in_job) {
    // Update job status
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& job = m_jobs[in_job.jobId];
        job.startTime = std::chrono::steady_clock::now();
    }

    // Simulate job execution (in real implementation, this would execute the test)
    // For now, we'll just sleep briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Complete job
    STestJob jobSnapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& job = m_jobs[in_job.jobId];
        job.endTime = std::chrono::steady_clock::now();
        job.isCompleted = true;
        job.isSuccessful = true;  // Assume success for now

        m_completedJobs.insert(in_job.jobId);
        m_activeJobs.erase(in_job.jobId);
        m_completedJobCount++;

        // Release resources
        ReleaseJobResources(job);
        jobSnapshot = job;
    }

    // Notify outside the lock: the callback may re-enter the orchestrator,
    // which would self-deadlock on the non-recursive mutex.
    NotifyJobStatus(jobSnapshot);

    // Wake up other workers in case they were waiting for this job's completion
    m_cv.notify_all();
}

void CTestOrchestrator::NotifyJobStatus(const STestJob& in_job) {
    if (m_statusCallback) {
        // Call callback without holding lock
        m_statusCallback(in_job);
    }
}

} // namespace TestMATE
