/**************************************************************************
 * File Name: ThreadPool.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of thread pool
 * Requirements: REQ-THR-001 to REQ-THR-010
 **************************************************************************/

#include "ThreadPool.h"
#include "utils/LogManager.h"

namespace TestMATE {

namespace {
    const char* kLogSource = "ThreadPool";
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CThreadPool::CThreadPool(TUInt32 in_uiNumThreads)
    : m_uiThreadCount(in_uiNumThreads)
{
    if (m_uiThreadCount == 0) {
        m_uiThreadCount = std::thread::hardware_concurrency();
        if (m_uiThreadCount == 0) {
            m_uiThreadCount = kDefaultThreadPoolSize;
        }
    }

    // Clamp to valid range
    m_uiThreadCount = std::max(kMinThreadPoolSize,
                               std::min(m_uiThreadCount, kMaxThreadPoolSize));

    LOG_INFO(kLogSource, "Creating thread pool with {} workers", m_uiThreadCount);

    m_vecWorkers.reserve(m_uiThreadCount);
    for (TUInt32 i = 0; i < m_uiThreadCount; ++i) {
        m_vecWorkers.emplace_back(&CThreadPool::WorkerThread, this);
    }
}

CThreadPool::~CThreadPool() {
    Shutdown(false);
}

//=============================================================================
// Public Methods
//=============================================================================

TUInt32 CThreadPool::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<TUInt32>(m_queueTasks.size());
}

void CThreadPool::WaitForAll() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvComplete.wait(lock, [this]() {
        return m_queueTasks.empty() && m_uiActiveTasks == 0;
    });
}

void CThreadPool::Shutdown(bool in_bForce) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_bShutdown) {
            return;  // Already shutdown
        }

        m_bShutdown = true;

        if (in_bForce) {
            // Clear pending tasks
            while (!m_queueTasks.empty()) {
                m_queueTasks.pop();
            }
        }
    }

    // Wake all workers
    m_cvTask.notify_all();

    // Join all threads
    for (auto& worker : m_vecWorkers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    LOG_INFO(kLogSource, "Thread pool shutdown complete");
}

//=============================================================================
// Private Methods
//=============================================================================

void CThreadPool::WorkerThread() {
    LOG_DEBUG(kLogSource, "Worker thread started");

    while (true) {
        STask task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cvTask.wait(lock, [this]() {
                return m_bShutdown || !m_queueTasks.empty();
            });

            if (m_bShutdown && m_queueTasks.empty()) {
                break;
            }

            if (m_queueTasks.empty()) {
                continue;
            }

            task = std::move(const_cast<STask&>(m_queueTasks.top()));
            m_queueTasks.pop();
            ++m_uiActiveTasks;
        }

        // Execute task outside lock
        try {
            task.function();
        } catch (const std::exception& ex) {
            LOG_ERROR(kLogSource, "Task exception: {}", ex.what());
        } catch (...) {
            LOG_ERROR(kLogSource, "Task threw unknown exception");
        }

        // Decrement the active count under the mutex so WaitForAll cannot
        // evaluate its predicate between the decrement and the notify and
        // then sleep forever (lost wakeup).
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_uiActiveTasks;
        }
        m_cvComplete.notify_all();
    }

    LOG_DEBUG(kLogSource, "Worker thread exiting");
}

} // namespace TestMATE
