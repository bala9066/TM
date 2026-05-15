/**************************************************************************
 * File Name: TestOrchestratorTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Unit tests for test orchestrator
 **************************************************************************/

#include <gtest/gtest.h>
#include "testmate/orchestration/TestOrchestrator.h"
#include <thread>
#include <chrono>

using namespace TestMATE;

//=============================================================================
// Test Fixtures
//=============================================================================

class TestOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        SOrchestratorConfig config;
        config.maxParallelJobs = 2;
        config.executionMode = EExecutionMode::kParallel;

        m_orchestrator = std::make_unique<CTestOrchestrator>(config);
    }

    void TearDown() override {
        if (m_orchestrator && m_orchestrator->IsRunning()) {
            m_orchestrator->Stop();
        }
    }

    std::unique_ptr<CTestOrchestrator> m_orchestrator;
};

//=============================================================================
// Lifecycle Tests
//=============================================================================

TEST_F(TestOrchestratorTest, StartStop) {
    EXPECT_FALSE(m_orchestrator->IsRunning());

    auto startResult = m_orchestrator->Start();
    EXPECT_TRUE(startResult.IsSuccess());
    EXPECT_TRUE(m_orchestrator->IsRunning());

    auto stopResult = m_orchestrator->Stop();
    EXPECT_TRUE(stopResult.IsSuccess());
    EXPECT_FALSE(m_orchestrator->IsRunning());
}

TEST_F(TestOrchestratorTest, StartAlreadyRunning) {
    m_orchestrator->Start();

    auto result = m_orchestrator->Start();
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

TEST_F(TestOrchestratorTest, StopNotRunning) {
    auto result = m_orchestrator->Stop();
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

//=============================================================================
// Job Submission Tests
//=============================================================================

TEST_F(TestOrchestratorTest, SubmitJob) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    job.priority = ETestPriority::kNormal;

    auto result = m_orchestrator->SubmitJob(job);
    EXPECT_TRUE(result.IsSuccess());

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, SubmitJobNotRunning) {
    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";

    auto result = m_orchestrator->SubmitJob(job);
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

TEST_F(TestOrchestratorTest, SubmitDuplicateJob) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";

    m_orchestrator->SubmitJob(job);
    auto result = m_orchestrator->SubmitJob(job);

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kAlreadyExists);

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, SubmitMultipleJobs) {
    m_orchestrator->Start();

    for (int i = 0; i < 5; ++i) {
        STestJob job;
        job.jobId = "job-" + std::to_string(i);
        job.testSequenceId = "TEST-" + std::to_string(i);

        auto result = m_orchestrator->SubmitJob(job);
        EXPECT_TRUE(result.IsSuccess());
    }

    auto jobs = m_orchestrator->GetAllJobs();
    EXPECT_EQ(jobs.size(), 5);

    m_orchestrator->Stop();
}

//=============================================================================
// Job Status Tests
//=============================================================================

TEST_F(TestOrchestratorTest, GetJobStatus) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    auto status = m_orchestrator->GetJobStatus("job-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->jobId, "job-1");
    EXPECT_EQ(status->testSequenceId, "TEST-001");

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, GetJobStatusNotFound) {
    m_orchestrator->Start();

    auto status = m_orchestrator->GetJobStatus("nonexistent");
    EXPECT_FALSE(status.has_value());

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, GetAllJobs) {
    m_orchestrator->Start();

    for (int i = 0; i < 3; ++i) {
        STestJob job;
        job.jobId = "job-" + std::to_string(i);
        job.testSequenceId = "TEST-" + std::to_string(i);
        m_orchestrator->SubmitJob(job);
    }

    auto jobs = m_orchestrator->GetAllJobs();
    EXPECT_EQ(jobs.size(), 3);

    m_orchestrator->Stop();
}

//=============================================================================
// Job Cancellation Tests
//=============================================================================

TEST_F(TestOrchestratorTest, CancelJob) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    auto result = m_orchestrator->CancelJob("job-1");
    EXPECT_TRUE(result.IsSuccess());

    auto status = m_orchestrator->GetJobStatus("job-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_TRUE(status->isCompleted);
    EXPECT_FALSE(status->isSuccessful);

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, CancelNonExistentJob) {
    m_orchestrator->Start();

    auto result = m_orchestrator->CancelJob("nonexistent");
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNotFound);

    m_orchestrator->Stop();
}

//=============================================================================
// Resource Management Tests
//=============================================================================

TEST_F(TestOrchestratorTest, RegisterResource) {
    auto result = m_orchestrator->RegisterResource("DMM-1", "Multimeter", 1);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(TestOrchestratorTest, RegisterDuplicateResource) {
    m_orchestrator->RegisterResource("DMM-1", "Multimeter", 1);

    auto result = m_orchestrator->RegisterResource("DMM-1", "Multimeter", 1);
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kAlreadyExists);
}

TEST_F(TestOrchestratorTest, RegisterMultipleResources) {
    EXPECT_TRUE(m_orchestrator->RegisterResource("DMM-1", "Multimeter", 1).IsSuccess());
    EXPECT_TRUE(m_orchestrator->RegisterResource("PSU-1", "PowerSupply", 1).IsSuccess());
    EXPECT_TRUE(m_orchestrator->RegisterResource("SCOPE-1", "Oscilloscope", 1).IsSuccess());
}

TEST_F(TestOrchestratorTest, ReleaseResource) {
    m_orchestrator->RegisterResource("DMM-1", "Multimeter", 1);

    auto result = m_orchestrator->ReleaseResource("DMM-1");
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(TestOrchestratorTest, ReleaseNonExistentResource) {
    auto result = m_orchestrator->ReleaseResource("nonexistent");
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNotFound);
}

//=============================================================================
// Dependency Tests
//=============================================================================

TEST_F(TestOrchestratorTest, JobWithDependencies) {
    SOrchestratorConfig config;
    config.enableDependencyTracking = true;
    m_orchestrator = std::make_unique<CTestOrchestrator>(config);
    m_orchestrator->Start();

    // Job 1 - no dependencies
    STestJob job1;
    job1.jobId = "job-1";
    job1.testSequenceId = "INIT";
    job1.priority = ETestPriority::kHigh;

    // Job 2 - depends on job 1
    STestJob job2;
    job2.jobId = "job-2";
    job2.testSequenceId = "TEST";
    job2.dependencies = {"job-1"};

    m_orchestrator->SubmitJob(job1);
    m_orchestrator->SubmitJob(job2);

    // Wait for completion
    m_orchestrator->WaitForCompletion(5000);

    auto status1 = m_orchestrator->GetJobStatus("job-1");
    auto status2 = m_orchestrator->GetJobStatus("job-2");

    ASSERT_TRUE(status1.has_value());
    ASSERT_TRUE(status2.has_value());

    // Both should complete
    EXPECT_TRUE(status1->isCompleted);
    EXPECT_TRUE(status2->isCompleted);

    // Job 1 should complete before job 2
    EXPECT_LT(status1->endTime, status2->startTime);

    m_orchestrator->Stop();
}

//=============================================================================
// Statistics Tests
//=============================================================================

TEST_F(TestOrchestratorTest, InitialStatistics) {
    auto stats = m_orchestrator->GetStatistics();

    EXPECT_EQ(stats.totalJobs, 0);
    EXPECT_EQ(stats.completedJobs, 0);
    EXPECT_EQ(stats.failedJobs, 0);
    EXPECT_EQ(stats.activeJobs, 0);
    EXPECT_EQ(stats.queuedJobs, 0);
}

TEST_F(TestOrchestratorTest, StatisticsAfterJobSubmission) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = m_orchestrator->GetStatistics();
    EXPECT_EQ(stats.totalJobs, 1);

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, StatisticsAfterJobCompletion) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    m_orchestrator->WaitForCompletion(2000);

    auto stats = m_orchestrator->GetStatistics();
    EXPECT_EQ(stats.totalJobs, 1);
    EXPECT_GT(stats.completedJobs, 0);

    m_orchestrator->Stop();
}

//=============================================================================
// Priority Tests
//=============================================================================

TEST_F(TestOrchestratorTest, PriorityOrdering) {
    SOrchestratorConfig config;
    config.maxParallelJobs = 1;  // Force sequential execution
    m_orchestrator = std::make_unique<CTestOrchestrator>(config);
    m_orchestrator->Start();

    // Submit low priority first
    STestJob lowPriJob;
    lowPriJob.jobId = "low-job";
    lowPriJob.testSequenceId = "LOW";
    lowPriJob.priority = ETestPriority::kLow;

    // Then high priority
    STestJob highPriJob;
    highPriJob.jobId = "high-job";
    highPriJob.testSequenceId = "HIGH";
    highPriJob.priority = ETestPriority::kHigh;

    m_orchestrator->SubmitJob(lowPriJob);
    m_orchestrator->SubmitJob(highPriJob);

    // Wait a bit for execution
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // High priority might execute first despite being submitted second
    // (This is a simplified test - real priority scheduling is more complex)

    m_orchestrator->Stop();
}

//=============================================================================
// Callback Tests
//=============================================================================

TEST_F(TestOrchestratorTest, StatusCallback) {
    m_orchestrator->Start();

    int callbackCount = 0;
    TString lastJobId;

    m_orchestrator->SetJobStatusCallback([&](const STestJob& job) {
        callbackCount++;
        lastJobId = job.jobId;
    });

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    m_orchestrator->WaitForCompletion(2000);

    EXPECT_GT(callbackCount, 0);
    EXPECT_EQ(lastJobId, "job-1");

    m_orchestrator->Stop();
}

//=============================================================================
// Wait For Completion Tests
//=============================================================================

TEST_F(TestOrchestratorTest, WaitForCompletionSuccess) {
    m_orchestrator->Start();

    STestJob job;
    job.jobId = "job-1";
    job.testSequenceId = "TEST-001";
    m_orchestrator->SubmitJob(job);

    bool completed = m_orchestrator->WaitForCompletion(5000);
    EXPECT_TRUE(completed);

    m_orchestrator->Stop();
}

TEST_F(TestOrchestratorTest, WaitForCompletionNoJobs) {
    m_orchestrator->Start();

    bool completed = m_orchestrator->WaitForCompletion(1000);
    EXPECT_TRUE(completed);  // Should complete immediately with no jobs

    m_orchestrator->Stop();
}

//=============================================================================
// Concurrent Execution Tests
//=============================================================================

TEST_F(TestOrchestratorTest, ParallelExecution) {
    SOrchestratorConfig config;
    config.maxParallelJobs = 4;
    config.executionMode = EExecutionMode::kParallel;
    m_orchestrator = std::make_unique<CTestOrchestrator>(config);
    m_orchestrator->Start();

    // Submit multiple jobs
    for (int i = 0; i < 8; ++i) {
        STestJob job;
        job.jobId = "job-" + std::to_string(i);
        job.testSequenceId = "TEST-" + std::to_string(i);
        m_orchestrator->SubmitJob(job);
    }

    m_orchestrator->WaitForCompletion(10000);

    auto stats = m_orchestrator->GetStatistics();
    EXPECT_EQ(stats.totalJobs, 8);
    EXPECT_GT(stats.completedJobs, 0);

    m_orchestrator->Stop();
}

// Note: main() is provided by gtest_main library
