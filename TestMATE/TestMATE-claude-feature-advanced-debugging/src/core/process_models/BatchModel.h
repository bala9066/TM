/**************************************************************************
 * File Name: BatchModel.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Batch process model for running multiple test configurations.
 *              Executes batches of tests with configurable parallelism.
 * Requirements: REQ-PM-039 to REQ-PM-057
 **************************************************************************/

#pragma once

#include "ProcessModelBase.h"
#include "core/threading/ThreadPool.h"
#include <queue>
#include <set>

namespace TestMATE {

/**************************************************************************
 * Struct: SBatchJob
 * Description: Represents a single job in a batch
 **************************************************************************/
struct SBatchJob {
    TUInt64 jobId{0};
    TString name;
    TString testSequencePath;
    std::map<TString, TString> parameters;
    EExecutionState state{EExecutionState::kIdle};
    ETestVerdict verdict{ETestVerdict::kNone};
    TTimePoint startTime;
    TTimePoint endTime;
    TUInt32 passCount{0};
    TUInt32 failCount{0};
};

/**************************************************************************
 * Enum: EBatchStrategy
 * Description: How to distribute jobs across workers
 **************************************************************************/
enum class EBatchStrategy {
    kFIFO,           // First in, first out
    kPriority,       // Priority-based execution
    kRoundRobin,     // Distribute evenly across workers
    kLoadBalanced    // Assign to least loaded worker
};

/**************************************************************************
 * Class: CBatchModel
 * Description: Batch execution model. Runs multiple independent test
 *              configurations, optionally in parallel.
 * Requirements: REQ-PM-039 to REQ-PM-057
 **************************************************************************/
class CBatchModel : public CProcessModelBase {
public:
    /**************************************************************************
     * Function Name: CBatchModel
     * Description: Constructor
     * Parameters:
     *   in_uiConcurrency - Max concurrent jobs (0 = auto)
     **************************************************************************/
    explicit CBatchModel(TUInt32 in_uiConcurrency = 1);
    ~CBatchModel() override;

    //=========================================================================
    // IProcessModel Implementation
    //=========================================================================

    [[nodiscard]] EProcessModelType GetType() const override {
        return EProcessModelType::kBatch;
    }

    [[nodiscard]] TString GetName() const override {
        return "Batch";
    }

    [[nodiscard]] TString GetDescription() const override {
        return "Batch test execution model for multiple configurations";
    }

    CResult Start() override;
    CResult Stop();

    //=========================================================================
    // Job Management
    //=========================================================================

    /**************************************************************************
     * Function Name: AddJob
     * Description: Adds a job to the batch queue
     * Parameters:
     *   in_job - Job to add
     * Returns: Job ID assigned to this job
     **************************************************************************/
    TUInt64 AddJob(SBatchJob in_job);

    /**************************************************************************
     * Function Name: RemoveJob
     * Description: Removes a pending job from the queue
     * Parameters:
     *   in_jobId - ID of job to remove
     * Returns: Result indicating success or failure
     **************************************************************************/
    CResult RemoveJob(TUInt64 in_jobId);

    /**************************************************************************
     * Function Name: GetJobStatus
     * Description: Gets status of a specific job
     * Parameters:
     *   in_jobId - Job identifier
     * Returns: Job info or nullopt if not found
     **************************************************************************/
    [[nodiscard]] std::optional<SBatchJob> GetJobStatus(TUInt64 in_jobId) const;

    /**************************************************************************
     * Function Name: GetPendingJobCount
     * Description: Returns number of jobs waiting to execute
     **************************************************************************/
    [[nodiscard]] TUInt32 GetPendingJobCount() const;

    /**************************************************************************
     * Function Name: GetRunningJobCount
     * Description: Returns number of currently executing jobs
     **************************************************************************/
    [[nodiscard]] TUInt32 GetRunningJobCount() const;

    /**************************************************************************
     * Function Name: GetCompletedJobCount
     * Description: Returns number of completed jobs
     **************************************************************************/
    [[nodiscard]] TUInt32 GetCompletedJobCount() const;

    /**************************************************************************
     * Function Name: ClearCompletedJobs
     * Description: Removes completed jobs from history
     **************************************************************************/
    void ClearCompletedJobs();

    //=========================================================================
    // Configuration
    //=========================================================================

    /**************************************************************************
     * Function Name: SetConcurrency
     * Description: Sets maximum concurrent jobs
     * Parameters:
     *   in_uiConcurrency - Max concurrent jobs
     **************************************************************************/
    void SetConcurrency(TUInt32 in_uiConcurrency);

    [[nodiscard]] TUInt32 GetConcurrency() const { return m_uiConcurrency; }

    /**************************************************************************
     * Function Name: SetStrategy
     * Description: Sets job distribution strategy
     **************************************************************************/
    void SetStrategy(EBatchStrategy in_eStrategy) { m_eStrategy = in_eStrategy; }

    [[nodiscard]] EBatchStrategy GetStrategy() const { return m_eStrategy; }

    /**************************************************************************
     * Function Name: SetStopOnFirstFailure
     * Description: Configure whether to stop batch on first job failure
     **************************************************************************/
    void SetStopOnFirstFailure(bool in_bStop) { m_bStopOnFirstFailure = in_bStop; }

    [[nodiscard]] bool GetStopOnFirstFailure() const { return m_bStopOnFirstFailure; }

    /**************************************************************************
     * Function Name: WaitForCompletion
     * Description: Blocks until all jobs complete
     * Parameters:
     *   in_timeoutMs - Timeout in milliseconds (0 = infinite)
     * Returns: true if completed, false if timeout
     **************************************************************************/
    bool WaitForCompletion(TInt64 in_timeoutMs = 0);

    //=========================================================================
    // Statistics
    //=========================================================================

    [[nodiscard]] TUInt32 GetTotalPassCount() const;
    [[nodiscard]] TUInt32 GetTotalFailCount() const;
    [[nodiscard]] TUInt32 GetSuccessfulJobCount() const;
    [[nodiscard]] TUInt32 GetFailedJobCount() const;

private:
    void JobDispatcherFunc();
    void ExecuteJob(TUInt64 in_jobId);
    SBatchJob* GetNextJob();

    TUInt32 m_uiConcurrency;
    EBatchStrategy m_eStrategy{EBatchStrategy::kFIFO};
    bool m_bStopOnFirstFailure{false};

    mutable std::mutex m_jobMutex;
    std::condition_variable m_jobCV;

    std::atomic<TUInt64> m_uiNextJobId{1};
    std::map<TUInt64, SBatchJob> m_mapJobs;
    std::queue<TUInt64> m_queuePending;
    std::set<TUInt64> m_setRunning;
    std::set<TUInt64> m_setCompleted;

    TUniquePtr<CThreadPool> m_pThreadPool;
    std::atomic<bool> m_bShouldStop{false};
    std::atomic<bool> m_bDispatcherRunning{false};
    TUniquePtr<std::thread> m_pDispatcherThread;
};

} // namespace TestMATE
