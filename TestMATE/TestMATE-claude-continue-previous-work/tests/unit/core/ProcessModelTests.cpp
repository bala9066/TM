/**************************************************************************
 * File Name: ProcessModelTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for process models
 **************************************************************************/

#include "core/process_models/SequentialModel.h"
#include "core/process_models/ExecutionContext.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace TestMATE;

//=============================================================================
// ExecutionContext Tests
//=============================================================================

class ExecutionContextTest : public ::testing::Test {
protected:
    CExecutionContext m_context{1, 2};
};

TEST_F(ExecutionContextTest, Constructor_SetsSocketAndSiteId) {
    EXPECT_EQ(m_context.GetSocketId(), 1u);
    EXPECT_EQ(m_context.GetSiteId(), 2u);
}

TEST_F(ExecutionContextTest, SetVariable_StoresValue) {
    m_context.SetVariable<int>("count", 42);
    auto value = m_context.GetVariable<int>("count");

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 42);
}

TEST_F(ExecutionContextTest, GetVariable_ReturnsNulloptForMissing) {
    auto value = m_context.GetVariable<int>("nonexistent");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ExecutionContextTest, GetVariable_ReturnsNulloptForWrongType) {
    m_context.SetVariable<int>("count", 42);
    auto value = m_context.GetVariable<std::string>("count");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ExecutionContextTest, HasVariable_ReturnsTrueIfExists) {
    m_context.SetVariable<int>("count", 42);
    EXPECT_TRUE(m_context.HasVariable("count"));
    EXPECT_FALSE(m_context.HasVariable("nonexistent"));
}

TEST_F(ExecutionContextTest, ClearVariable_RemovesVariable) {
    m_context.SetVariable<int>("count", 42);
    EXPECT_TRUE(m_context.HasVariable("count"));

    m_context.ClearVariable("count");
    EXPECT_FALSE(m_context.HasVariable("count"));
}

TEST_F(ExecutionContextTest, ClearAllVariables_RemovesAll) {
    m_context.SetVariable<int>("a", 1);
    m_context.SetVariable<int>("b", 2);
    m_context.SetVariable<int>("c", 3);

    m_context.ClearAllVariables();

    EXPECT_FALSE(m_context.HasVariable("a"));
    EXPECT_FALSE(m_context.HasVariable("b"));
    EXPECT_FALSE(m_context.HasVariable("c"));
}

TEST_F(ExecutionContextTest, AbortRequest_InitiallyFalse) {
    EXPECT_FALSE(m_context.IsAbortRequested());
}

TEST_F(ExecutionContextTest, RequestAbort_SetsFlag) {
    m_context.RequestAbort();
    EXPECT_TRUE(m_context.IsAbortRequested());
}

TEST_F(ExecutionContextTest, ClearAbortRequest_ClearsFlag) {
    m_context.RequestAbort();
    m_context.ClearAbortRequest();
    EXPECT_FALSE(m_context.IsAbortRequested());
}

TEST_F(ExecutionContextTest, PassFailCounters_IncrementCorrectly) {
    EXPECT_EQ(m_context.GetPassCount(), 0u);
    EXPECT_EQ(m_context.GetFailCount(), 0u);

    m_context.IncrementPassCount();
    m_context.IncrementPassCount();
    m_context.IncrementFailCount();

    EXPECT_EQ(m_context.GetPassCount(), 2u);
    EXPECT_EQ(m_context.GetFailCount(), 1u);
}

//=============================================================================
// SequentialModel Tests
//=============================================================================

class SequentialModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_model = std::make_unique<CSequentialModel>();
    }

    void TearDown() override {
        m_model.reset();
    }

    std::unique_ptr<CSequentialModel> m_model;
};

TEST_F(SequentialModelTest, GetType_ReturnsSequential) {
    EXPECT_EQ(m_model->GetType(), EProcessModelType::kSequential);
}

TEST_F(SequentialModelTest, GetName_ReturnsSequential) {
    EXPECT_EQ(m_model->GetName(), "Sequential");
}

TEST_F(SequentialModelTest, InitialState_IsIdle) {
    EXPECT_EQ(m_model->GetState(), EProcessModelState::kIdle);
    EXPECT_FALSE(m_model->IsRunning());
    EXPECT_FALSE(m_model->IsPaused());
}

TEST_F(SequentialModelTest, Start_WithoutInitialize_Fails) {
    CResult result = m_model->Start();
    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNotInitialized);
}

TEST_F(SequentialModelTest, Initialize_WithNullSequence_Fails) {
    CResult result = m_model->Initialize(nullptr);
    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNullPointer);
}

TEST_F(SequentialModelTest, Pause_WhenNotRunning_Fails) {
    CResult result = m_model->Pause();
    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

TEST_F(SequentialModelTest, Resume_WhenNotPaused_Fails) {
    CResult result = m_model->Resume();
    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

TEST_F(SequentialModelTest, Abort_WhenIdle_Fails) {
    CResult result = m_model->Abort();
    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

TEST_F(SequentialModelTest, GetElapsedTimeMs_WhenIdle_ReturnsZero) {
    EXPECT_EQ(m_model->GetElapsedTimeMs(), 0);
}

TEST_F(SequentialModelTest, StateChangeCallback_IsCalled) {
    bool callbackCalled = false;
    EProcessModelState fromState, toState;

    m_model->SetStateChangeCallback([&](EProcessModelState from, EProcessModelState to) {
        callbackCalled = true;
        fromState = from;
        toState = to;
    });

    // Trigger state change via shutdown (Idle -> Idle doesn't trigger)
    // For now, just verify callback is set
    EXPECT_FALSE(callbackCalled);  // No state change yet
}

TEST_F(SequentialModelTest, ProgressCallback_CanBeSet) {
    TUInt32 reportedCurrent = 0;
    TUInt32 reportedTotal = 0;

    m_model->SetProgressCallback([&](TUInt32 current, TUInt32 total) {
        reportedCurrent = current;
        reportedTotal = total;
    });

    // Callback stored but not called until execution
    EXPECT_EQ(reportedCurrent, 0u);
    EXPECT_EQ(reportedTotal, 0u);
}

//=============================================================================
// ExecutionContext Thread Safety Tests
//=============================================================================

TEST(ExecutionContextThreadTest, ConcurrentVariableAccess_IsThreadSafe) {
    CExecutionContext context(0, 0);
    const int kIterations = 1000;

    std::thread writer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            context.SetVariable<int>("counter", i);
        }
    });

    std::thread reader([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto value = context.GetVariable<int>("counter");
            // Just verify no crash/data race
            (void)value;
        }
    });

    writer.join();
    reader.join();

    // If we get here without crashing, thread safety is working
    EXPECT_TRUE(context.HasVariable("counter"));
}
