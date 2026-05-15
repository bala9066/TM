/**************************************************************************
 * File Name: ParallelBatchTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for ParallelModel and BatchModel
 **************************************************************************/

#include "core/process_models/ParallelModel.h"
#include "core/process_models/BatchModel.h"
#include <gtest/gtest.h>

using namespace TestMATE;

//=============================================================================
// ParallelModel Tests
//=============================================================================

class ParallelModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_pModel = std::make_unique<CParallelModel>(4);
    }

    void TearDown() override {
        if (m_pModel && m_pModel->GetState() == EProcessModelState::kRunning) {
            m_pModel->Stop();
        }
        m_pModel.reset();
    }

    std::unique_ptr<CParallelModel> m_pModel;
};

TEST_F(ParallelModelTest, Constructor_SetsSocketCount) {
    EXPECT_EQ(m_pModel->GetSocketCount(), 4u);
    EXPECT_EQ(m_pModel->GetType(), EProcessModelType::kParallel);
}

TEST_F(ParallelModelTest, GetName_ReturnsParallel) {
    EXPECT_EQ(m_pModel->GetName(), "Parallel");
}

TEST_F(ParallelModelTest, SetSocketCount_UpdatesCount) {
    auto result = m_pModel->SetSocketCount(8);
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_pModel->GetSocketCount(), 8u);
}

TEST_F(ParallelModelTest, SetSocketCount_FailsWhileRunning) {
    m_pModel->Start();
    auto result = m_pModel->SetSocketCount(8);
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(ParallelModelTest, GetSocketInfo_ReturnsValidInfo) {
    auto info = m_pModel->GetSocketInfo(1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->socketId, 1u);
    EXPECT_TRUE(info->enabled);
}

TEST_F(ParallelModelTest, GetSocketInfo_InvalidSocket_ReturnsNullopt) {
    auto info = m_pModel->GetSocketInfo(999);
    EXPECT_FALSE(info.has_value());
}

TEST_F(ParallelModelTest, EnableSocket_DisablesSocket) {
    m_pModel->EnableSocket(1, false);
    auto info = m_pModel->GetSocketInfo(1);
    ASSERT_TRUE(info.has_value());
    EXPECT_FALSE(info->enabled);
}

TEST_F(ParallelModelTest, GetEnabledSocketCount_ReflectsState) {
    EXPECT_EQ(m_pModel->GetEnabledSocketCount(), 4u);

    m_pModel->EnableSocket(1, false);
    EXPECT_EQ(m_pModel->GetEnabledSocketCount(), 3u);
}

TEST_F(ParallelModelTest, Start_SetsRunningState) {
    auto result = m_pModel->Start();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_pModel->GetState(), EProcessModelState::kRunning);
}

TEST_F(ParallelModelTest, Start_FailsWithNoEnabledSockets) {
    for (TUInt32 i = 1; i <= 4; ++i) {
        m_pModel->EnableSocket(i, false);
    }

    auto result = m_pModel->Start();
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(ParallelModelTest, Stop_StopsExecution) {
    m_pModel->Start();
    auto result = m_pModel->Stop();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_pModel->GetState(), EProcessModelState::kIdle);
}

TEST_F(ParallelModelTest, SynchronizationMode_DefaultOff) {
    EXPECT_FALSE(m_pModel->GetSynchronizationMode());
}

TEST_F(ParallelModelTest, SetSynchronizationMode_Changes) {
    m_pModel->SetSynchronizationMode(true);
    EXPECT_TRUE(m_pModel->GetSynchronizationMode());
}

TEST_F(ParallelModelTest, WaitForCompletion_CompletesSuccessfully) {
    m_pModel->Start();
    bool completed = m_pModel->WaitForCompletion(5000);  // 5 second timeout
    EXPECT_TRUE(completed);
}

//=============================================================================
// BatchModel Tests
//=============================================================================

class BatchModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_pModel = std::make_unique<CBatchModel>(2);
    }

    void TearDown() override {
        if (m_pModel && m_pModel->GetState() == EProcessModelState::kRunning) {
            m_pModel->Stop();
        }
        m_pModel.reset();
    }

    std::unique_ptr<CBatchModel> m_pModel;
};

TEST_F(BatchModelTest, Constructor_SetsConcurrency) {
    EXPECT_EQ(m_pModel->GetConcurrency(), 2u);
    EXPECT_EQ(m_pModel->GetType(), EProcessModelType::kBatch);
}

TEST_F(BatchModelTest, GetName_ReturnsBatch) {
    EXPECT_EQ(m_pModel->GetName(), "Batch");
}

TEST_F(BatchModelTest, AddJob_ReturnsUniqueId) {
    SBatchJob job1;
    job1.name = "Job1";

    SBatchJob job2;
    job2.name = "Job2";

    TUInt64 id1 = m_pModel->AddJob(job1);
    TUInt64 id2 = m_pModel->AddJob(job2);

    EXPECT_NE(id1, id2);
    EXPECT_EQ(m_pModel->GetPendingJobCount(), 2u);
}

TEST_F(BatchModelTest, RemoveJob_RemovesPendingJob) {
    SBatchJob job;
    job.name = "Test";
    TUInt64 id = m_pModel->AddJob(job);

    auto result = m_pModel->RemoveJob(id);
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_pModel->GetPendingJobCount(), 0u);
}

TEST_F(BatchModelTest, RemoveJob_FailsForNonexistent) {
    auto result = m_pModel->RemoveJob(999);
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(BatchModelTest, GetJobStatus_ReturnsJobInfo) {
    SBatchJob job;
    job.name = "TestJob";
    TUInt64 id = m_pModel->AddJob(job);

    auto status = m_pModel->GetJobStatus(id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->name, "TestJob");
    EXPECT_EQ(status->state, EExecutionState::kIdle);
}

TEST_F(BatchModelTest, SetConcurrency_UpdatesValue) {
    m_pModel->SetConcurrency(4);
    EXPECT_EQ(m_pModel->GetConcurrency(), 4u);
}

TEST_F(BatchModelTest, SetStrategy_UpdatesValue) {
    m_pModel->SetStrategy(EBatchStrategy::kPriority);
    EXPECT_EQ(m_pModel->GetStrategy(), EBatchStrategy::kPriority);
}

TEST_F(BatchModelTest, SetStopOnFirstFailure_UpdatesValue) {
    m_pModel->SetStopOnFirstFailure(true);
    EXPECT_TRUE(m_pModel->GetStopOnFirstFailure());
}

TEST_F(BatchModelTest, Start_WithJobs_ExecutesThem) {
    SBatchJob job1;
    job1.name = "Job1";
    m_pModel->AddJob(job1);

    SBatchJob job2;
    job2.name = "Job2";
    m_pModel->AddJob(job2);

    auto result = m_pModel->Start();
    EXPECT_TRUE(result.IsSuccess());

    bool completed = m_pModel->WaitForCompletion(5000);
    EXPECT_TRUE(completed);
    EXPECT_EQ(m_pModel->GetCompletedJobCount(), 2u);
}

TEST_F(BatchModelTest, Stop_StopsExecution) {
    m_pModel->Start();
    auto result = m_pModel->Stop();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(BatchModelTest, ClearCompletedJobs_RemovesThem) {
    SBatchJob job;
    job.name = "Test";
    m_pModel->AddJob(job);

    m_pModel->Start();
    m_pModel->WaitForCompletion(5000);
    m_pModel->Stop();

    EXPECT_EQ(m_pModel->GetCompletedJobCount(), 1u);

    m_pModel->ClearCompletedJobs();
    EXPECT_EQ(m_pModel->GetCompletedJobCount(), 0u);
}

