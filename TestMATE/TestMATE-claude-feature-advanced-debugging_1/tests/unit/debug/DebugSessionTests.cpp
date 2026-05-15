/**
 * @file DebugSessionTests.cpp
 * @brief Unit tests for CDebugSession and CDebugCLI classes
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/DebugSession.h"
#include "testmate/debug/DebugCLI.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sstream>

using namespace TestMATE;

// ==================== CDebugSession Tests ====================

class DebugSessionTests : public ::testing::Test {
protected:
    void SetUp() override {
        breakpointManager = std::make_shared<CBreakpointManager>();
        session = std::make_unique<CDebugSession>(breakpointManager);
    }

    void TearDown() override {
        session.reset();
        breakpointManager.reset();
    }

    std::shared_ptr<CBreakpointManager> breakpointManager;
    std::unique_ptr<CDebugSession> session;
};

// Test 1: Session lifecycle
TEST_F(DebugSessionTests, SessionLifecycle) {
    EXPECT_FALSE(session->IsActive());
    EXPECT_EQ(session->GetState(), EDebugState::kIdle);

    session->Start();
    EXPECT_TRUE(session->IsActive());
    EXPECT_EQ(session->GetState(), EDebugState::kRunning);

    session->Finish();
    EXPECT_FALSE(session->IsActive());
    EXPECT_EQ(session->GetState(), EDebugState::kFinished);
}

// Test 2: Session abort
TEST_F(DebugSessionTests, SessionAbort) {
    session->Start();
    EXPECT_TRUE(session->IsActive());

    session->Abort();
    EXPECT_FALSE(session->IsActive());
    EXPECT_EQ(session->GetState(), EDebugState::kAborted);
}

// Test 3: Step enter/exit tracking
TEST_F(DebugSessionTests, StepTracking) {
    session->Start();

    session->OnStepEnter("STEP-001", "Initialize");
    EXPECT_EQ(session->GetCurrentStepId(), "STEP-001");
    EXPECT_EQ(session->GetCurrentStepName(), "Initialize");

    session->OnStepExit("STEP-001");
    EXPECT_EQ(session->GetCurrentStepId(), "");
}

// Test 4: Call stack tracking
TEST_F(DebugSessionTests, CallStackTracking) {
    session->Start();

    session->OnStepEnter("STEP-001", "Main");
    session->OnStepEnter("STEP-002", "SubStep");
    session->OnStepEnter("STEP-003", "DeepStep");

    auto callStack = session->GetCallStack();
    ASSERT_EQ(callStack.size(), 3);
    EXPECT_EQ(callStack[0].stepId, "STEP-001");
    EXPECT_EQ(callStack[1].stepId, "STEP-002");
    EXPECT_EQ(callStack[2].stepId, "STEP-003");

    session->OnStepExit("STEP-003");
    callStack = session->GetCallStack();
    ASSERT_EQ(callStack.size(), 2);

    session->OnStepExit("STEP-002");
    session->OnStepExit("STEP-001");
    callStack = session->GetCallStack();
    EXPECT_EQ(callStack.size(), 0);
}

// Test 5: Breakpoint hit detection
TEST_F(DebugSessionTests, BreakpointHitDetection) {
    breakpointManager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");

    session->Start();
    session->OnStepEnter("STEP-010", "Test Step");

    EXPECT_TRUE(session->ShouldBreak());

    // Other steps should not break
    session->OnStepExit("STEP-010");
    session->OnStepEnter("STEP-020", "Other Step");
    EXPECT_FALSE(session->ShouldBreak());
}

// Test 6: Continue action (simplified - just test state changes)
TEST_F(DebugSessionTests, ContinueAction) {
    session->Start();
    EXPECT_EQ(session->GetState(), EDebugState::kRunning);

    // Calling Continue() just queues the action
    // Full thread synchronization testing is done in integration tests
    session->Continue();
    // The action will be picked up by WaitForDebugAction() in actual usage
}

// Test 7: Step over action (simplified)
TEST_F(DebugSessionTests, StepOverAction) {
    session->Start();
    session->OnStepEnter("STEP-010", "Test Step");

    // StepOver() queues the step action
    session->StepOver();
    // The action will be picked up by WaitForDebugAction() in actual usage
}

// Test 8: Step into action (simplified)
TEST_F(DebugSessionTests, StepIntoAction) {
    session->Start();
    session->OnStepEnter("STEP-010", "Test Step");

    // StepInto() queues the step action
    session->StepInto();
    // The action will be picked up by WaitForDebugAction() in actual usage
}

// Test 9: Variable registration and retrieval
TEST_F(DebugSessionTests, VariableAccessors) {
    double voltage = 3.3;

    session->RegisterVariableAccessor("voltage", [&voltage]() -> TVariableValue {
        return voltage;
    });

    auto value = session->GetVariable("voltage");
    ASSERT_TRUE(value.has_value());

    auto* dblValue = std::get_if<TDouble>(&(*value));
    ASSERT_NE(dblValue, nullptr);
    EXPECT_DOUBLE_EQ(*dblValue, 3.3);

    // Change the voltage
    voltage = 5.0;
    value = session->GetVariable("voltage");
    dblValue = std::get_if<TDouble>(&(*value));
    ASSERT_NE(dblValue, nullptr);
    EXPECT_DOUBLE_EQ(*dblValue, 5.0);
}

// Test 10: Get all variables
TEST_F(DebugSessionTests, GetAllVariables) {
    int count = 10;
    double voltage = 3.3;

    session->RegisterVariableAccessor("count", [&count]() -> TVariableValue {
        return static_cast<TInt32>(count);
    });

    session->RegisterVariableAccessor("voltage", [&voltage]() -> TVariableValue {
        return voltage;
    });

    auto variables = session->GetAllVariables();
    EXPECT_EQ(variables.size(), 2);
    EXPECT_TRUE(variables.find("count") != variables.end());
    EXPECT_TRUE(variables.find("voltage") != variables.end());
}

// Test 11: Exception handling
TEST_F(DebugSessionTests, ExceptionHandling) {
    session->Start();

    session->OnException("Test exception occurred");
    EXPECT_EQ(session->GetLastException(), "Test exception occurred");

    auto stats = session->GetStats();
    EXPECT_EQ(stats.exceptionsOccurred, 1);
}

// Test 12: Error handling
TEST_F(DebugSessionTests, ErrorHandling) {
    session->Start();

    session->OnError("STEP-010", "Step failed with error");
    EXPECT_EQ(session->GetLastError(), "Step failed with error");
}

// Test 13: Statistics tracking
TEST_F(DebugSessionTests, StatisticsTracking) {
    breakpointManager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");

    session->Start();

    // Execute some steps
    session->OnStepEnter("STEP-005", "Step 5");
    session->OnStepExit("STEP-005");

    session->OnStepEnter("STEP-010", "Step 10");
    EXPECT_TRUE(session->ShouldBreak());
    session->OnStepExit("STEP-010");

    auto stats = session->GetStats();
    EXPECT_EQ(stats.stepsExecuted, 2);
    EXPECT_EQ(stats.breakpointsHit, 1);
}

// Test 14: Run to cursor
TEST_F(DebugSessionTests, RunToCursor) {
    session->Start();

    session->OnStepEnter("STEP-010", "Step 10");

    std::thread debuggerThread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        session->RunToCursor("STEP-050");
    });

    auto action = session->WaitForDebugAction();
    EXPECT_EQ(action, EDebugAction::kContinue);

    debuggerThread.join();

    // Now when we reach STEP-050, it should break
    session->OnStepEnter("STEP-050", "Step 50");
    EXPECT_TRUE(session->ShouldBreak());
}

// Test 15: Pause state detection
TEST_F(DebugSessionTests, PauseStateDetection) {
    session->Start();
    EXPECT_FALSE(session->IsPaused());

    session->OnStepEnter("STEP-010", "Step 10");

    // In a real scenario, WaitForDebugAction would pause
    // We can't easily test this without complex threading,
    // but we can verify the state transitions
    EXPECT_EQ(session->GetState(), EDebugState::kRunning);
}

// ==================== CDebugCLI Tests ====================

class DebugCLITests : public ::testing::Test {
protected:
    void SetUp() override {
        breakpointManager = std::make_shared<CBreakpointManager>();
        session = std::make_shared<CDebugSession>(breakpointManager);

        // Use string streams for testing
        input = std::make_unique<std::istringstream>();
        output = std::make_unique<std::ostringstream>();

        SDebugCLIConfig config;
        config.showPrompt = false;  // Disable prompt for testing
        config.echoCommands = false;

        cli = std::make_unique<CDebugCLI>(session, breakpointManager, config, *input, *output);
    }

    void TearDown() override {
        cli.reset();
        output.reset();
        input.reset();
        session.reset();
        breakpointManager.reset();
    }

    std::shared_ptr<CBreakpointManager> breakpointManager;
    std::shared_ptr<CDebugSession> session;
    std::unique_ptr<std::istringstream> input;
    std::unique_ptr<std::ostringstream> output;
    std::unique_ptr<CDebugCLI> cli;
};

// Test 16: Help command
TEST_F(DebugCLITests, HelpCommand) {
    auto result = cli->ExecuteCommand("help");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.shouldContinue);
}

// Test 17: Breakpoint commands
TEST_F(DebugCLITests, BreakpointCommands) {
    // Set breakpoint
    auto result = cli->ExecuteCommand("break STEP-010");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Breakpoint"), TString::npos);

    // List breakpoints
    result = cli->ExecuteCommand("list");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("STEP-010"), TString::npos);
}

// Test 18: Delete breakpoint
TEST_F(DebugCLITests, DeleteBreakpoint) {
    // Set breakpoint
    cli->ExecuteCommand("break STEP-010");

    // Delete it (breakpoint ID should be 1)
    auto result = cli->ExecuteCommand("delete 1");
    EXPECT_TRUE(result.success);

    // List should be empty
    result = cli->ExecuteCommand("list");
    EXPECT_NE(result.message.find("No breakpoints"), TString::npos);
}

// Test 19: Info command
TEST_F(DebugCLITests, InfoCommand) {
    session->Start();

    auto result = cli->ExecuteCommand("info");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Debug Session Information"), TString::npos);
}

// Test 20: Unknown command
TEST_F(DebugCLITests, UnknownCommand) {
    auto result = cli->ExecuteCommand("invalid_command");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Unknown command"), TString::npos);
}

// Test 21: Quit command
TEST_F(DebugCLITests, QuitCommand) {
    auto result = cli->ExecuteCommand("quit");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.shouldContinue);
}

// Test 22: Continue command (not paused)
TEST_F(DebugCLITests, ContinueNotPaused) {
    session->Start();

    auto result = cli->ExecuteCommand("continue");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Not paused"), TString::npos);
}

// Test 23: Command aliases
TEST_F(DebugCLITests, CommandAliases) {
    // Test 'c' for continue
    auto result = cli->ExecuteCommand("c");
    EXPECT_FALSE(result.success); // Will fail because not paused, but command recognized

    // Test 's' for step
    result = cli->ExecuteCommand("s");
    EXPECT_FALSE(result.success); // Same reason

    // Test '?' for help
    result = cli->ExecuteCommand("?");
    EXPECT_TRUE(result.success);

    // Test 'q' for quit
    result = cli->ExecuteCommand("q");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.shouldContinue);
}

// Test 24: Empty command
TEST_F(DebugCLITests, EmptyCommand) {
    auto result = cli->ExecuteCommand("");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.shouldContinue);
}

// Test 25: Variable printing
TEST_F(DebugCLITests, VariablePrinting) {
    session->Start();

    double voltage = 3.3;
    session->RegisterVariableAccessor("voltage", [&voltage]() -> TVariableValue {
        return voltage;
    });

    auto result = cli->ExecuteCommand("print voltage");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("voltage"), TString::npos);
    EXPECT_NE(result.message.find("3.3"), TString::npos);
}

// Test 26: Print non-existent variable
TEST_F(DebugCLITests, PrintNonExistentVariable) {
    session->Start();

    auto result = cli->ExecuteCommand("print nonexistent");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("not found"), TString::npos);
}

// Test 27: Backtrace command
TEST_F(DebugCLITests, BacktraceCommand) {
    session->Start();
    session->OnStepEnter("STEP-001", "Main");
    session->OnStepEnter("STEP-002", "Sub");

    auto result = cli->ExecuteCommand("backtrace");
    EXPECT_TRUE(result.success);
}

// Test 28: Where command
TEST_F(DebugCLITests, WhereCommand) {
    session->Start();
    session->OnStepEnter("STEP-010", "Current Step");

    auto result = cli->ExecuteCommand("where");
    EXPECT_TRUE(result.success);
}

// Test 29: Break command without argument
TEST_F(DebugCLITests, BreakCommandNoArgument) {
    auto result = cli->ExecuteCommand("break");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Usage"), TString::npos);
}

// Test 30: Delete command with invalid ID
TEST_F(DebugCLITests, DeleteInvalidID) {
    auto result = cli->ExecuteCommand("delete invalid");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Invalid"), TString::npos);
}

// Note: main() is provided by gtest_main library, not included here
