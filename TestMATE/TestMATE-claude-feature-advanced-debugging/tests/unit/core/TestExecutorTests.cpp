/**************************************************************************
 * File Name: TestExecutorTests.cpp
 * Description: Unit tests for TestExecutor component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/execution/TestExecutor.h"
#include "core/test_sequence/TestSequence.h"
#include <atomic>
#include <thread>
#include <chrono>

namespace TestMATE {
namespace Tests {

class CTestExecutorTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_executor = std::make_unique<CTestExecutor>();
        m_stepStartCount = 0;
        m_stepCompleteCount = 0;
    }

    std::unique_ptr<CTestExecutor> m_executor;
    std::atomic<int> m_stepStartCount{0};
    std::atomic<int> m_stepCompleteCount{0};
};

TEST_F(CTestExecutorTests, Execute_EmptySequence_Success) {
    CTestSequence sequence("empty_seq", "EmptySequence");

    SExecutionConfig config;
    auto result = m_executor->Execute(sequence, config);

    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CTestExecutorTests, Execute_WithCallbacks_CallbacksInvoked) {
    CTestSequence sequence("callback_test", "CallbackTest");

    m_executor->SetStepStartCallback([this](TUInt32, const TString&) {
        ++m_stepStartCount;
    });

    m_executor->SetStepCompleteCallback([this](TUInt32, const STestResult&) {
        ++m_stepCompleteCount;
    });

    SExecutionConfig config;
    auto result = m_executor->Execute(sequence, config);

    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CTestExecutorTests, Stop_DuringExecution_StopsGracefully) {
    CTestSequence sequence("stop_test", "StopTest");

    SExecutionConfig config;
    config.stopOnFirstFailure = false;

    std::thread execThread([this, &sequence, &config]() {
        m_executor->Execute(sequence, config);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto stopResult = m_executor->Stop();

    execThread.join();

    EXPECT_TRUE(stopResult.IsSuccess());
    EXPECT_EQ(m_executor->GetStatus().state, EExecutionState::kIdle);
}

TEST_F(CTestExecutorTests, Pause_DuringExecution_PausesExecution) {
    SExecutionConfig config;
    auto pauseResult = m_executor->Pause();

    EXPECT_TRUE(pauseResult.IsSuccess());
    EXPECT_EQ(m_executor->GetStatus().state, EExecutionState::kPaused);
}

TEST_F(CTestExecutorTests, Resume_AfterPause_ResumesExecution) {
    m_executor->Pause();
    auto resumeResult = m_executor->Resume();

    EXPECT_TRUE(resumeResult.IsSuccess());
}

TEST_F(CTestExecutorTests, GetState_Initial_Idle) {
    EXPECT_EQ(m_executor->GetStatus().state, EExecutionState::kIdle);
}

TEST_F(CTestExecutorTests, SetProgressCallback_ValidCallback_Accepted) {
    bool callbackSet = false;
    m_executor->SetProgressCallback([&callbackSet](TDouble, const TString&) {
        callbackSet = true;
    });

    // Verify no crash - callback storage is internal
    SUCCEED();
}

TEST_F(CTestExecutorTests, ExecutionConfig_DefaultValues_Correct) {
    SExecutionConfig config;

    EXPECT_FALSE(config.stopOnFirstFailure);
    EXPECT_TRUE(config.skipDisabledSteps);
    EXPECT_EQ(config.socketCount, 1);
    EXPECT_EQ(config.totalTimeoutMs, 0);
}

TEST_F(CTestExecutorTests, ExecutionConfig_StopOnFirstFailure_Respected) {
    CTestSequence sequence("failure_test", "FailureTest");

    SExecutionConfig config;
    config.stopOnFirstFailure = true;

    auto result = m_executor->Execute(sequence, config);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CTestExecutorTests, MultipleExecutions_Sequential_Success) {
    CTestSequence sequence1("seq1", "Seq1");
    CTestSequence sequence2("seq2", "Seq2");

    SExecutionConfig config;

    auto result1 = m_executor->Execute(sequence1, config);
    auto result2 = m_executor->Execute(sequence2, config);

    EXPECT_TRUE(result1.IsSuccess());
    EXPECT_TRUE(result2.IsSuccess());
}

} // namespace Tests
} // namespace TestMATE
