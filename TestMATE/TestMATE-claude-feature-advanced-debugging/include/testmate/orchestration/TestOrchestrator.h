/**
 * @file TestOrchestrator.h
 * @brief Advanced test orchestration with dependencies and parallel execution
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the test orchestration system that manages complex
 * test execution flows with dependencies, parallel execution, and resource
 * management.
 */

#ifndef TESTMATE_ORCHESTRATION_TEST_ORCHESTRATOR_H
#define TESTMATE_ORCHESTRATION_TEST_ORCHESTRATOR_H

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <set>

namespace TestMATE {

/**
 * @brief Test execution priority
 */
enum class ETestPriority {
    kLow = 0,
    kNormal = 1,
    kHigh = 2,
    kCritical = 3
};

/**
 * @brief Test execution mode
 */
enum class EExecutionMode {
    kSequential,        ///< Execute tests one at a time
    kParallel,         ///< Execute tests in parallel
    kDependencyBased   ///< Execute based on dependencies
};

/**
 * @brief Test job definition
 */
struct STestJob {
    TString jobId;
    TString testSequenceId;
    ETestPriority priority{ETestPriority::kNormal};
    TVector<TString> dependencies;     ///< Job IDs this job depends on
    TVector<TString> resources;        ///< Required resources
    TMap<TString, TString> parameters;
    TTimePoint submittedTime;
    TTimePoint startTime;
    TTimePoint endTime;
    bool isCompleted{false};
    bool isSuccessful{false};
    TString errorMessage;
};

/**
 * @brief Resource definition
 */
struct SResource {
    TString resourceId;
    TString resourceType;
    TUInt32 maxConcurrent{1};      ///< Max concurrent uses
    TUInt32 currentUsage{0};       ///< Current usage count
    bool isAvailable{true};
};

/**
 * @brief Orchestration statistics
 */
struct SOrchestrationStats {
    TUInt32 totalJobs{0};
    TUInt32 completedJobs{0};
    TUInt32 failedJobs{0};
    TUInt32 activeJobs{0};
    TUInt32 queuedJobs{0};
    TDouble averageExecutionTime{0.0};
    TDouble successRate{0.0};
};

/**
 * @brief Orchestrator configuration
 */
struct SOrchestratorConfig {
    EExecutionMode executionMode{EExecutionMode::kParallel};
    TUInt32 maxParallelJobs{4};
    TUInt32 maxQueueSize{100};
    bool enableDependencyTracking{true};
    bool enableResourceManagement{true};
    TUInt32 jobTimeoutSeconds{3600};  // 1 hour default
};

/**
 * @brief Job status callback
 */
using FJobStatusCallback = std::function<void(const STestJob&)>;

/**
 * @brief Test Orchestrator
 *
 * Manages complex test execution with:
 * - Dependency management
 * - Parallel execution
 * - Resource allocation
 * - Priority scheduling
 * - Queue management
 *
 * Example usage:
 * @code
 * SOrchestratorConfig config;
 * config.maxParallelJobs = 8;
 * config.executionMode = EExecutionMode::kDependencyBased;
 *
 * CTestOrchestrator orchestrator(config);
 * orchestrator.Start();
 *
 * // Define resources
 * orchestrator.RegisterResource("DMM-1", "Multimeter", 1);
 * orchestrator.RegisterResource("PSU-1", "PowerSupply", 1);
 *
 * // Submit jobs with dependencies
 * STestJob job1;
 * job1.jobId = "job-1";
 * job1.testSequenceId = "INIT-001";
 * job1.priority = ETestPriority::kHigh;
 *
 * STestJob job2;
 * job2.jobId = "job-2";
 * job2.testSequenceId = "TEST-001";
 * job2.dependencies = {"job-1"};  // Depends on job-1
 * job2.resources = {"DMM-1"};
 *
 * orchestrator.SubmitJob(job1);
 * orchestrator.SubmitJob(job2);
 *
 * // Monitor progress
 * auto stats = orchestrator.GetStatistics();
 *
 * orchestrator.Stop();
 * @endcode
 */
class CTestOrchestrator {
public:
    /**
     * @brief Construct orchestrator with configuration
     * @param in_config Orchestrator configuration
     */
    explicit CTestOrchestrator(const SOrchestratorConfig& in_config = {});

    /**
     * @brief Destructor
     */
    ~CTestOrchestrator();

    // Non-copyable, non-movable
    CTestOrchestrator(const CTestOrchestrator&) = delete;
    CTestOrchestrator& operator=(const CTestOrchestrator&) = delete;
    CTestOrchestrator(CTestOrchestrator&&) = delete;
    CTestOrchestrator& operator=(CTestOrchestrator&&) = delete;

    /**
     * @brief Start the orchestrator
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult Start();

    /**
     * @brief Stop the orchestrator
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult Stop();

    /**
     * @brief Check if orchestrator is running
     * @return true if running
     */
    [[nodiscard]] bool IsRunning() const { return m_isRunning; }

    /**
     * @brief Submit test job
     * @param in_job Test job to submit
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult SubmitJob(const STestJob& in_job);

    /**
     * @brief Cancel test job
     * @param in_jobId Job ID to cancel
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult CancelJob(const TString& in_jobId);

    /**
     * @brief Get job status
     * @param in_jobId Job ID
     * @return Job info or nullopt if not found
     */
    [[nodiscard]] std::optional<STestJob> GetJobStatus(const TString& in_jobId) const;

    /**
     * @brief Get all jobs
     * @return Vector of all jobs
     */
    [[nodiscard]] TVector<STestJob> GetAllJobs() const;

    /**
     * @brief Register resource
     * @param in_resourceId Resource identifier
     * @param in_resourceType Resource type
     * @param in_maxConcurrent Maximum concurrent uses
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult RegisterResource(
        const TString& in_resourceId,
        const TString& in_resourceType,
        TUInt32 in_maxConcurrent = 1);

    /**
     * @brief Release resource
     * @param in_resourceId Resource identifier
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult ReleaseResource(const TString& in_resourceId);

    /**
     * @brief Get orchestration statistics
     * @return Statistics
     */
    [[nodiscard]] SOrchestrationStats GetStatistics() const;

    /**
     * @brief Set job status callback
     * @param in_callback Callback function
     */
    void SetJobStatusCallback(FJobStatusCallback in_callback);

    /**
     * @brief Wait for all jobs to complete
     * @param in_timeoutMs Timeout in milliseconds (0 = infinite)
     * @return true if all jobs completed, false if timeout
     */
    [[nodiscard]] bool WaitForCompletion(TUInt32 in_timeoutMs = 0);

private:
    /**
     * @brief Worker thread function
     */
    void WorkerThread();

    /**
     * @brief Get next job to execute
     * @return Job or nullopt if none available
     */
    std::optional<STestJob> GetNextJob();

    /**
     * @brief Check if job dependencies are satisfied
     * @param in_job Job to check
     * @return true if dependencies satisfied
     */
    bool AreDependenciesSatisfied(const STestJob& in_job) const;

    /**
     * @brief Check (without acquiring) whether a job's resources are free
     * @param in_job Job to check
     * @return true if every required resource has spare capacity
     */
    bool AreResourcesAvailable(const STestJob& in_job) const;

    /**
     * @brief Check whether any queued job is currently runnable (not
     *        completed, not active, dependencies satisfied, resources free).
     *        Used as the worker condition-variable predicate so idle workers
     *        block instead of busy-spinning on a permanently-true predicate.
     * @return true if at least one job could be started now
     */
    bool HasRunnableJob() const;

    /**
     * @brief Acquire resources for job
     * @param in_job Job requiring resources
     * @return true if resources acquired
     */
    bool AcquireResources(const STestJob& in_job);

    /**
     * @brief Release resources for job
     * @param in_job Job releasing resources
     */
    void ReleaseJobResources(const STestJob& in_job);

    /**
     * @brief Execute job
     * @param in_job Job to execute
     */
    void ExecuteJob(STestJob& in_job);

    /**
     * @brief Notify job status change
     * @param in_job Job with status change
     */
    void NotifyJobStatus(const STestJob& in_job);

    SOrchestratorConfig m_config;
    std::atomic<bool> m_isRunning{false};

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    TMap<TString, STestJob> m_jobs;              ///< All jobs
    std::priority_queue<TString> m_jobQueue;     ///< Pending jobs (sorted by priority)
    TSet<TString> m_activeJobs;                  ///< Currently executing jobs
    TSet<TString> m_completedJobs;               ///< Completed job IDs
    TMap<TString, SResource> m_resources;        ///< Available resources

    TVector<std::thread> m_workerThreads;
    FJobStatusCallback m_statusCallback;

    // Statistics
    std::atomic<TUInt32> m_totalJobs{0};
    std::atomic<TUInt32> m_completedJobCount{0};
    std::atomic<TUInt32> m_failedJobs{0};
};

} // namespace TestMATE

#endif // TESTMATE_ORCHESTRATION_TEST_ORCHESTRATOR_H
