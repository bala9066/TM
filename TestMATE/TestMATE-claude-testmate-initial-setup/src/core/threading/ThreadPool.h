/**************************************************************************
 * File Name: ThreadPool.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Thread pool implementation for parallel task execution.
 * Requirements: REQ-THR-001 to REQ-THR-010
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

namespace TestMATE {

/**************************************************************************
 * Class: CThreadPool
 * Description: Fixed-size thread pool for executing tasks in parallel.
 *              Supports task submission with futures for result retrieval.
 * Requirements: REQ-THR-001 to REQ-THR-010
 **************************************************************************/
class CThreadPool {
public:
    /**************************************************************************
     * Function Name: CThreadPool
     * Description: Creates thread pool with specified number of workers
     * Parameters:
     *   in_uiNumThreads - Number of worker threads (default: hardware concurrency)
     * Requirements: REQ-THR-001
     **************************************************************************/
    explicit CThreadPool(TUInt32 in_uiNumThreads = 0);

    /**************************************************************************
     * Function Name: ~CThreadPool
     * Description: Destructor - waits for all tasks to complete
     **************************************************************************/
    ~CThreadPool();

    // Non-copyable, non-movable
    CThreadPool(const CThreadPool&) = delete;
    CThreadPool& operator=(const CThreadPool&) = delete;
    CThreadPool(CThreadPool&&) = delete;
    CThreadPool& operator=(CThreadPool&&) = delete;

    /**************************************************************************
     * Function Name: Submit
     * Description: Submits a task for execution
     * Parameters:
     *   in_fn - Function to execute
     *   args - Arguments to pass to function
     * Returns: Future for retrieving result
     * Requirements: REQ-THR-002
     **************************************************************************/
    template<typename F, typename... Args>
    auto Submit(F&& in_fn, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    /**************************************************************************
     * Function Name: SubmitWithPriority
     * Description: Submits a task with specified priority
     * Parameters:
     *   in_ePriority - Task priority
     *   in_fn - Function to execute
     *   args - Arguments to pass to function
     * Returns: Future for retrieving result
     * Requirements: REQ-THR-003
     **************************************************************************/
    template<typename F, typename... Args>
    auto SubmitWithPriority(ETaskPriority in_ePriority, F&& in_fn, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    /**************************************************************************
     * Function Name: GetThreadCount
     * Description: Returns number of worker threads
     * Returns: Worker count
     * Requirements: REQ-THR-004
     **************************************************************************/
    [[nodiscard]] TUInt32 GetThreadCount() const { return m_uiThreadCount; }

    /**************************************************************************
     * Function Name: GetPendingTaskCount
     * Description: Returns number of tasks waiting in queue
     * Returns: Pending task count
     * Requirements: REQ-THR-005
     **************************************************************************/
    [[nodiscard]] TUInt32 GetPendingTaskCount() const;

    /**************************************************************************
     * Function Name: GetActiveTaskCount
     * Description: Returns number of tasks currently executing
     * Returns: Active task count
     **************************************************************************/
    [[nodiscard]] TUInt32 GetActiveTaskCount() const { return m_uiActiveTasks.load(); }

    /**************************************************************************
     * Function Name: WaitForAll
     * Description: Blocks until all submitted tasks complete
     * Requirements: REQ-THR-006
     **************************************************************************/
    void WaitForAll();

    /**************************************************************************
     * Function Name: Shutdown
     * Description: Stops accepting new tasks and waits for completion
     * Parameters:
     *   in_bForce - If true, discard pending tasks
     * Requirements: REQ-THR-007
     **************************************************************************/
    void Shutdown(bool in_bForce = false);

    /**************************************************************************
     * Function Name: IsRunning
     * Description: Checks if pool is accepting tasks
     * Returns: true if running
     **************************************************************************/
    [[nodiscard]] bool IsRunning() const { return !m_bShutdown.load(); }

private:
    struct STask {
        std::function<void()> function;
        ETaskPriority priority;

        bool operator<(const STask& other) const {
            return static_cast<int>(priority) < static_cast<int>(other.priority);
        }
    };

    void WorkerThread();

    TVector<std::thread> m_vecWorkers;
    std::priority_queue<STask> m_queueTasks;

    mutable std::mutex m_mutex;
    std::condition_variable m_cvTask;
    std::condition_variable m_cvComplete;

    std::atomic<bool> m_bShutdown{false};
    std::atomic<TUInt32> m_uiActiveTasks{0};
    TUInt32 m_uiThreadCount;
};

//=============================================================================
// Template Implementation
//=============================================================================

template<typename F, typename... Args>
auto CThreadPool::Submit(F&& in_fn, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    return SubmitWithPriority(ETaskPriority::kNormal,
                              std::forward<F>(in_fn),
                              std::forward<Args>(args)...);
}

template<typename F, typename... Args>
auto CThreadPool::SubmitWithPriority(ETaskPriority in_ePriority, F&& in_fn, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using ReturnType = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(in_fn), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_bShutdown) {
            throw std::runtime_error("Cannot submit task to shutdown pool");
        }

        m_queueTasks.push(STask{
            [task]() { (*task)(); },
            in_ePriority
        });
    }

    m_cvTask.notify_one();
    return result;
}

} // namespace TestMATE
