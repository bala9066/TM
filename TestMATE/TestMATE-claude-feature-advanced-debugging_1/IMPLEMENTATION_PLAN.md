# TestMATE Advanced Features Implementation Plan

**Branch:** `feature/advanced-debugging-and-enhancements`
**Target Release:** TestMATE v2.0
**Created:** 2025-11-23
**Status:** Planning Phase

---

## 📋 Table of Contents

1. [Executive Summary](#executive-summary)
2. [Phase 1: Interactive Debugging System](#phase-1-interactive-debugging-system)
3. [Phase 2: Test Retry & Recovery](#phase-2-test-retry--recovery)
4. [Phase 3: Performance Profiling](#phase-3-performance-profiling)
5. [Phase 4: REST API Server](#phase-4-rest-api-server)
6. [Phase 5: Instrument Resource Manager](#phase-5-instrument-resource-manager)
7. [Implementation Timeline](#implementation-timeline)
8. [Testing Strategy](#testing-strategy)
9. [Documentation Requirements](#documentation-requirements)

---

## Executive Summary

### Goals
- **Primary:** Implement interactive debugging with breakpoints for test development
- **Secondary:** Add production-critical features (retry, profiling, API, resource management)
- **Tertiary:** Establish foundation for advanced features (ML, cloud sync, multi-site)

### Success Criteria
- ✅ Debugging reduces test development time by 50%
- ✅ Retry system improves test reliability by 30%
- ✅ Performance profiler identifies bottlenecks in <5 seconds
- ✅ REST API enables remote monitoring with <100ms latency
- ✅ All features have >90% test coverage

### Development Approach
- **Incremental:** Each phase delivers standalone value
- **Test-Driven:** Write tests before implementation
- **Backward Compatible:** No breaking changes to existing tests
- **Well Documented:** API docs, examples, tutorials for each feature

---

# Phase 1: Interactive Debugging System

**Duration:** 4 weeks
**Priority:** CRITICAL
**Complexity:** High

## 1.1 Architecture Overview

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                     CDebugController                         │
│  - Manages debug sessions                                    │
│  - Handles breakpoints                                       │
│  - Controls execution flow                                   │
└──────────────┬──────────────────────────────────────────────┘
               │
               ├──────────────────────────────────────────────┐
               │                                              │
       ┌───────▼─────────┐                           ┌───────▼─────────┐
       │ CBreakpointMgr  │                           │ CDebugSession   │
       │ - Add/Remove    │                           │ - Pause/Resume  │
       │ - Conditions    │                           │ - Inspect Vars  │
       │ - Hit Count     │                           │ - Step Control  │
       └─────────────────┘                           └─────────────────┘
               │                                              │
               └──────────────┬───────────────────────────────┘
                              │
                      ┌───────▼──────────┐
                      │ CDebugInterface  │
                      │ - CLI Interface  │
                      │ - WebSocket API  │
                      │ - GUI Protocol   │
                      └──────────────────┘
```

## 1.2 Detailed Design

### 1.2.1 Breakpoint System

**File:** `include/testmate/debug/Breakpoint.h`

```cpp
/**************************************************************************
 * File Name: Breakpoint.h
 * Description: Breakpoint system for interactive debugging
 * Requirements: REQ-DEBUG-001 to REQ-DEBUG-020
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <optional>

namespace TestMATE {

/**************************************************************************
 * Enum: EBreakpointType
 * Description: Types of breakpoints
 **************************************************************************/
enum class EBreakpointType {
    kStepEntry,      ///< Break when step starts
    kStepExit,       ///< Break when step completes
    kConditional,    ///< Break when condition is true
    kDataAccess,     ///< Break when variable is read/written
    kException,      ///< Break when error occurs
    kHitCount        ///< Break after N hits
};

/**************************************************************************
 * Enum: EBreakpointState
 * Description: Breakpoint states
 **************************************************************************/
enum class EBreakpointState {
    kEnabled,
    kDisabled,
    kOneShot        ///< Auto-disable after first hit
};

/**************************************************************************
 * Struct: SBreakpointCondition
 * Description: Conditional breakpoint evaluation
 **************************************************************************/
struct SBreakpointCondition {
    TString expression;         ///< e.g., "voltage > 5.0"
    TFunction<bool()> evaluator; ///< Custom condition function

    SBreakpointCondition() = default;
    explicit SBreakpointCondition(const TString& expr)
        : expression(expr) {}
};

/**************************************************************************
 * Class: CBreakpoint
 * Description: Represents a breakpoint in test execution
 **************************************************************************/
class CBreakpoint {
public:
    /**
     * @brief Constructor
     * @param in_id Unique breakpoint ID
     * @param in_type Breakpoint type
     */
    CBreakpoint(TUInt32 in_id, EBreakpointType in_type);

    /**
     * @brief Set breakpoint location
     * @param in_stepId Test step ID
     */
    void SetLocation(const TString& in_stepId);

    /**
     * @brief Set conditional expression
     * @param in_condition Condition to evaluate
     */
    void SetCondition(const SBreakpointCondition& in_condition);

    /**
     * @brief Set hit count threshold
     * @param in_count Break after this many hits
     */
    void SetHitCount(TUInt32 in_count);

    /**
     * @brief Enable/disable breakpoint
     */
    void SetEnabled(bool in_enabled);

    /**
     * @brief Check if breakpoint should trigger
     * @return true if breakpoint should pause execution
     */
    [[nodiscard]] bool ShouldBreak() const;

    /**
     * @brief Record a hit
     */
    void RecordHit();

    /**
     * @brief Get breakpoint info
     */
    [[nodiscard]] TString GetInfo() const;

    // Getters
    [[nodiscard]] TUInt32 GetId() const { return m_id; }
    [[nodiscard]] EBreakpointType GetType() const { return m_type; }
    [[nodiscard]] const TString& GetLocation() const { return m_stepId; }
    [[nodiscard]] TUInt32 GetHitCount() const { return m_currentHits; }
    [[nodiscard]] bool IsEnabled() const { return m_state == EBreakpointState::kEnabled; }

private:
    TUInt32 m_id;
    EBreakpointType m_type;
    EBreakpointState m_state{EBreakpointState::kEnabled};
    TString m_stepId;
    std::optional<SBreakpointCondition> m_condition;
    TUInt32 m_hitCountThreshold{0};
    TUInt32 m_currentHits{0};
    TTime m_createdTime;
    TTime m_lastHitTime;
};

/**************************************************************************
 * Class: CBreakpointManager
 * Description: Manages all breakpoints in the system
 **************************************************************************/
class CBreakpointManager {
public:
    CBreakpointManager();
    ~CBreakpointManager();

    /**
     * @brief Add a new breakpoint
     * @param in_type Breakpoint type
     * @param in_location Step ID or location
     * @return Breakpoint ID
     */
    TUInt32 AddBreakpoint(EBreakpointType in_type, const TString& in_location);

    /**
     * @brief Remove a breakpoint
     * @param in_id Breakpoint ID
     */
    CResult RemoveBreakpoint(TUInt32 in_id);

    /**
     * @brief Enable/disable a breakpoint
     */
    CResult SetBreakpointEnabled(TUInt32 in_id, bool in_enabled);

    /**
     * @brief Set breakpoint condition
     */
    CResult SetBreakpointCondition(TUInt32 in_id, const SBreakpointCondition& in_condition);

    /**
     * @brief Check if execution should break at location
     * @param in_stepId Current step ID
     * @return Breakpoint ID if should break, 0 otherwise
     */
    [[nodiscard]] TUInt32 CheckBreakpoint(const TString& in_stepId);

    /**
     * @brief Get all breakpoints
     */
    [[nodiscard]] TVector<CBreakpoint*> GetBreakpoints() const;

    /**
     * @brief Clear all breakpoints
     */
    void ClearAll();

    /**
     * @brief Save breakpoints to file
     */
    CResult SaveToFile(const TString& in_filePath);

    /**
     * @brief Load breakpoints from file
     */
    CResult LoadFromFile(const TString& in_filePath);

private:
    std::unordered_map<TUInt32, std::unique_ptr<CBreakpoint>> m_breakpoints;
    TUInt32 m_nextId{1};
    mutable std::mutex m_mutex;
};

} // namespace TestMATE
```

### 1.2.2 Debug Session Controller

**File:** `include/testmate/debug/DebugSession.h`

```cpp
/**************************************************************************
 * File Name: DebugSession.h
 * Description: Interactive debug session control
 **************************************************************************/

#pragma once

#include "testmate/debug/Breakpoint.h"
#include "testmate/core/process_models/ExecutionContext.h"
#include "testmate/core/test_sequence/ITestStep.h"
#include <condition_variable>

namespace TestMATE {

/**************************************************************************
 * Enum: EDebugAction
 * Description: Actions available during debug pause
 **************************************************************************/
enum class EDebugAction {
    kContinue,      ///< Resume execution
    kStepOver,      ///< Execute current step, pause at next
    kStepInto,      ///< Enter sub-sequence if available
    kStepOut,       ///< Complete current sequence, pause at parent
    kRunToCursor,   ///< Run until specified step
    kAbort          ///< Stop execution completely
};

/**************************************************************************
 * Struct: SDebugState
 * Description: Current state of debug session
 **************************************************************************/
struct SDebugState {
    bool isPaused{false};
    TString currentStepId;
    TUInt32 currentStepIndex{0};
    TUInt32 totalSteps{0};
    TUInt32 breakpointId{0};  ///< ID of breakpoint that triggered pause
    TTime pauseTime;
    TVector<TString> callStack;
};

/**************************************************************************
 * Struct: SVariableInfo
 * Description: Information about a variable during debug
 **************************************************************************/
struct SVariableInfo {
    TString name;
    TString type;
    TString value;
    bool isModifiable{true};
};

/**************************************************************************
 * Class: CDebugSession
 * Description: Manages an interactive debug session
 **************************************************************************/
class CDebugSession {
public:
    explicit CDebugSession(CBreakpointManager* in_pBreakpointMgr);
    ~CDebugSession();

    /**
     * @brief Start debug session
     */
    CResult Start();

    /**
     * @brief Stop debug session
     */
    CResult Stop();

    /**
     * @brief Check if should pause at step
     * @param in_pStep Current step
     * @param in_context Execution context
     * @return true if paused
     */
    bool CheckPausePoint(ITestStep* in_pStep, CExecutionContext& in_context);

    /**
     * @brief Wait for user action (blocking)
     * @return Action to take
     */
    EDebugAction WaitForAction();

    /**
     * @brief Execute debug action (called from debug UI)
     */
    CResult ExecuteAction(EDebugAction in_action);

    /**
     * @brief Get current debug state
     */
    [[nodiscard]] SDebugState GetState() const;

    /**
     * @brief Inspect all variables in current context
     */
    [[nodiscard]] TVector<SVariableInfo> InspectVariables(CExecutionContext& in_context);

    /**
     * @brief Get specific variable value
     */
    [[nodiscard]] std::optional<TString> GetVariable(
        const TString& in_name,
        CExecutionContext& in_context);

    /**
     * @brief Modify variable value
     */
    CResult SetVariable(
        const TString& in_name,
        const TString& in_value,
        CExecutionContext& in_context);

    /**
     * @brief Evaluate expression
     * @param in_expression Expression to evaluate (e.g., "voltage * 2")
     */
    [[nodiscard]] std::optional<TDouble> EvaluateExpression(
        const TString& in_expression,
        CExecutionContext& in_context);

    /**
     * @brief Get call stack
     */
    [[nodiscard]] TVector<TString> GetCallStack() const;

    /**
     * @brief Set callback for state changes
     */
    void SetStateChangeCallback(TFunction<void(const SDebugState&)> in_callback);

private:
    void Pause(const TString& in_stepId, TUInt32 in_breakpointId);
    void Resume();

    CBreakpointManager* m_pBreakpointMgr;
    SDebugState m_state;
    EDebugAction m_pendingAction{EDebugAction::kContinue};
    TString m_runToCursorTarget;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    TFunction<void(const SDebugState&)> m_stateChangeCallback;
    TVector<TString> m_callStack;
    bool m_isActive{false};
};

} // namespace TestMATE
```

### 1.2.3 Debug Interface (CLI)

**File:** `include/testmate/debug/DebugCLI.h`

```cpp
/**************************************************************************
 * File Name: DebugCLI.h
 * Description: Command-line interface for debugging
 **************************************************************************/

#pragma once

#include "testmate/debug/DebugSession.h"
#include <iostream>
#include <thread>

namespace TestMATE {

/**************************************************************************
 * Class: CDebugCLI
 * Description: Command-line debug interface
 **************************************************************************/
class CDebugCLI {
public:
    explicit CDebugCLI(CDebugSession* in_pSession);
    ~CDebugCLI();

    /**
     * @brief Start CLI in separate thread
     */
    void Start();

    /**
     * @brief Stop CLI
     */
    void Stop();

    /**
     * @brief Print current state
     */
    void PrintState(const SDebugState& in_state);

    /**
     * @brief Available commands
     */
    void PrintHelp();

private:
    void RunCommandLoop();
    void ProcessCommand(const TString& in_command);

    // Command handlers
    void HandleContinue();
    void HandleStepOver();
    void HandleStepInto();
    void HandleStepOut();
    void HandlePrint(const TString& in_varName);
    void HandleSet(const TString& in_varName, const TString& in_value);
    void HandleBreakpoint(const TString& in_args);
    void HandleList();
    void HandleEvaluate(const TString& in_expression);
    void HandleCallStack();

    CDebugSession* m_pSession;
    std::unique_ptr<std::thread> m_commandThread;
    bool m_isRunning{false};
};

} // namespace TestMATE
```

### 1.2.4 Integration with Test Execution

**File:** `src/core/process_models/SequentialExecutor.cpp` (modifications)

```cpp
// Modified SequentialExecutor to support debugging

CResult CSequentialExecutor::Execute(
    CTestSequence* in_pSequence,
    CExecutionContext& in_context) {

    // Check if debug session is active
    if (m_pDebugSession && m_pDebugSession->IsActive()) {
        for (auto* pStep : in_pSequence->GetSteps()) {
            // Check for breakpoint before executing step
            if (m_pDebugSession->CheckPausePoint(pStep, in_context)) {
                // Wait for user action
                EDebugAction action = m_pDebugSession->WaitForAction();

                if (action == EDebugAction::kAbort) {
                    return TESTMATE_FAILURE(EErrorCode::kOperationCancelled,
                                          "Execution aborted by user");
                }

                // Handle other actions (step over, step into, etc.)
                HandleDebugAction(action, pStep, in_context);
            }

            // Execute step normally
            SStepResult stepResult;
            auto result = pStep->Execute(stepResult);

            // Store result
            in_context.SetStepResult(pStep->GetId(), stepResult);

            if (!result.IsSuccess()) {
                // Check for exception breakpoint
                if (m_pDebugSession && m_pDebugSession->HasExceptionBreakpoint()) {
                    m_pDebugSession->CheckPausePoint(pStep, in_context);
                }

                if (stepResult.severity == ESeverity::kCritical) {
                    return result;
                }
            }
        }
    } else {
        // Normal execution without debugging
        return ExecuteNormal(in_pSequence, in_context);
    }

    return TESTMATE_SUCCESS();
}
```

## 1.3 Usage Examples

### Example 1: Basic Breakpoint

```cpp
#include "testmate/debug/DebugSession.h"
#include "testmate/debug/DebugCLI.h"

// Setup
CBreakpointManager bpMgr;
CDebugSession session(&bpMgr);
CDebugCLI cli(&session);

// Add breakpoint at specific step
TUInt32 bpId = bpMgr.AddBreakpoint(
    EBreakpointType::kStepEntry,
    "STEP-005-VOLTAGE-MEASURE"
);

// Start debug session
session.Start();
cli.Start();

// Run test sequence (will pause at breakpoint)
auto result = executor.Execute(sequence, context);

// CLI will show:
// Breakpoint 1 hit at STEP-005-VOLTAGE-MEASURE
// (debug)
```

### Example 2: Conditional Breakpoint

```cpp
// Break when voltage exceeds threshold
SBreakpointCondition condition;
condition.expression = "voltage > 5.0";
condition.evaluator = [&context]() {
    auto voltage = context.GetVariable<TDouble>("voltage");
    return voltage.has_value() && *voltage > 5.0;
};

TUInt32 bpId = bpMgr.AddBreakpoint(
    EBreakpointType::kConditional,
    "STEP-010-LIMIT-CHECK"
);
bpMgr.SetBreakpointCondition(bpId, condition);
```

### Example 3: Interactive Debug Commands

```
Test paused at: STEP-005-VOLTAGE-MEASURE
(debug) print voltage
voltage = 4.85 (TDouble)

(debug) print all
voltage = 4.85 (TDouble)
current = 0.125 (TDouble)
resistance = 38.8 (TDouble)
test_status = "RUNNING" (TString)

(debug) set voltage 5.5
Variable 'voltage' set to 5.5

(debug) eval voltage * current
Result: 0.6875

(debug) step
Executing step: STEP-006-CURRENT-MEASURE

(debug) continue
Resuming execution...
```

### Example 4: Hit Count Breakpoint

```cpp
// Break after 10 iterations in a loop
TUInt32 bpId = bpMgr.AddBreakpoint(
    EBreakpointType::kHitCount,
    "STEP-LOOP-ITERATION"
);
bpMgr.SetHitCount(bpId, 10);
```

## 1.4 Implementation Tasks

### Week 1: Core Infrastructure
- [ ] Create `include/testmate/debug/` directory structure
- [ ] Implement `CBreakpoint` class
- [ ] Implement `CBreakpointManager` class
- [ ] Write unit tests for breakpoint system
- [ ] Document breakpoint API

### Week 2: Debug Session
- [ ] Implement `CDebugSession` class
- [ ] Add thread synchronization (mutex, condition variable)
- [ ] Implement pause/resume logic
- [ ] Implement variable inspection
- [ ] Write unit tests for debug session

### Week 3: CLI Interface & Integration
- [ ] Implement `CDebugCLI` class
- [ ] Add command parsing and handling
- [ ] Integrate with `CSequentialExecutor`
- [ ] Integrate with `CParallelExecutor`
- [ ] Integration tests

### Week 4: Advanced Features & Polish
- [ ] Implement expression evaluator
- [ ] Add call stack tracking
- [ ] Implement breakpoint save/load
- [ ] Performance optimization
- [ ] Documentation and examples
- [ ] User guide

## 1.5 Testing Strategy

### Unit Tests (80+ tests)

```cpp
// tests/unit/debug/BreakpointTests.cpp
TEST_F(BreakpointTests, CreateBasicBreakpoint) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.SetLocation("STEP-001");

    EXPECT_EQ(bp.GetId(), 1);
    EXPECT_EQ(bp.GetLocation(), "STEP-001");
    EXPECT_TRUE(bp.IsEnabled());
}

TEST_F(BreakpointTests, ConditionalBreakpointTriggersWhenTrue) {
    CBreakpoint bp(1, EBreakpointType::kConditional);

    SBreakpointCondition condition;
    condition.evaluator = []() { return true; };
    bp.SetCondition(condition);

    EXPECT_TRUE(bp.ShouldBreak());
}

TEST_F(BreakpointTests, HitCountBreakpointTriggersAfterNHits) {
    CBreakpoint bp(1, EBreakpointType::kHitCount);
    bp.SetHitCount(3);

    bp.RecordHit(); // Hit 1
    EXPECT_FALSE(bp.ShouldBreak());

    bp.RecordHit(); // Hit 2
    EXPECT_FALSE(bp.ShouldBreak());

    bp.RecordHit(); // Hit 3
    EXPECT_TRUE(bp.ShouldBreak());
}

TEST_F(BreakpointManagerTests, AddAndRemoveBreakpoints) {
    CBreakpointManager mgr;

    auto id1 = mgr.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");
    auto id2 = mgr.AddBreakpoint(EBreakpointType::kStepExit, "STEP-002");

    EXPECT_EQ(mgr.GetBreakpoints().size(), 2);

    mgr.RemoveBreakpoint(id1);
    EXPECT_EQ(mgr.GetBreakpoints().size(), 1);
}

TEST_F(DebugSessionTests, PauseAndResume) {
    CBreakpointManager bpMgr;
    CDebugSession session(&bpMgr);

    session.Start();

    // Simulate pause
    MockTestStep step("STEP-001");
    MockExecutionContext context;

    auto id = bpMgr.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");

    // This should pause
    bool paused = session.CheckPausePoint(&step, context);
    EXPECT_TRUE(paused);

    auto state = session.GetState();
    EXPECT_TRUE(state.isPaused);
    EXPECT_EQ(state.currentStepId, "STEP-001");
}
```

### Integration Tests

```cpp
// tests/integration/DebugIntegrationTests.cpp
TEST_F(DebugIntegrationTests, DebugSessionWithRealSequence) {
    // Load real test sequence
    auto sequence = LoadSequenceFromFile("test_sequences/voltage_test.xml");

    // Setup debug session
    CBreakpointManager bpMgr;
    CDebugSession session(&bpMgr);

    // Add breakpoint at step 3
    bpMgr.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-003");

    session.Start();

    // Execute in separate thread
    std::thread execThread([&]() {
        CSequentialExecutor executor;
        executor.SetDebugSession(&session);

        CExecutionContext context(1, 1);
        executor.Execute(sequence.get(), context);
    });

    // Wait for pause
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto state = session.GetState();
    EXPECT_TRUE(state.isPaused);
    EXPECT_EQ(state.currentStepId, "STEP-003");

    // Resume
    session.ExecuteAction(EDebugAction::kContinue);

    execThread.join();
}
```

## 1.6 Documentation

### 1.6.1 API Reference
- Doxygen comments for all public methods
- Usage examples for each class
- Code snippets for common scenarios

### 1.6.2 User Guide
**File:** `docs/user_guide/debugging.md`

```markdown
# Interactive Debugging Guide

## Introduction
TestMATE's interactive debugging system allows you to:
- Set breakpoints at any test step
- Pause execution and inspect variables
- Step through tests line-by-line
- Evaluate expressions on-the-fly
- Modify variables during execution

## Quick Start

### 1. Enable Debug Mode
```cpp
CBreakpointManager bpMgr;
CDebugSession session(&bpMgr);
CDebugCLI cli(&session);

session.Start();
cli.Start();
```

### 2. Set Breakpoints
```cpp
// Break at specific step
bpMgr.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-005");

// Break when condition is true
auto bpId = bpMgr.AddBreakpoint(EBreakpointType::kConditional, "STEP-010");
SBreakpointCondition condition;
condition.expression = "voltage > 5.0";
bpMgr.SetBreakpointCondition(bpId, condition);
```

### 3. Debug Commands
- `continue` or `c` - Resume execution
- `step` or `s` - Execute one step
- `print <var>` or `p <var>` - Print variable
- `set <var> <value>` - Modify variable
- `breakpoint <step>` or `b <step>` - Add breakpoint
- `list` or `l` - List all breakpoints
- `help` or `h` - Show help

[... more detailed guide ...]
```

---

# Phase 2: Test Retry & Recovery

**Duration:** 2 weeks
**Priority:** HIGH
**Complexity:** Medium

## 2.1 Architecture

```cpp
/**************************************************************************
 * File Name: RetryPolicy.h
 * Description: Automatic test retry with configurable policies
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"

namespace TestMATE {

enum class ERetryStrategy {
    kFixedDelay,        ///< Fixed wait between retries
    kExponentialBackoff, ///< Exponentially increasing delay
    kLinearBackoff,     ///< Linearly increasing delay
    kJittered           ///< Random jitter to avoid thundering herd
};

struct SRetryPolicy {
    TUInt32 maxRetries{3};
    TUInt32 initialDelayMs{100};
    TDouble backoffMultiplier{2.0};
    TUInt32 maxDelayMs{10000};
    ERetryStrategy strategy{ERetryStrategy::kExponentialBackoff};

    // Which errors should trigger retry
    TVector<EErrorCode> retriableErrors{
        EErrorCode::kTimeout,
        EErrorCode::kConnectionLost,
        EErrorCode::kReceiveFailed,
        EErrorCode::kSendFailed
    };

    // Custom retry decision function
    TFunction<bool(const CResult&)> shouldRetry;
};

class CRetryExecutor {
public:
    explicit CRetryExecutor(const SRetryPolicy& in_policy);

    /**
     * @brief Execute test step with retry
     */
    CResult ExecuteWithRetry(
        ITestStep* in_pStep,
        CExecutionContext& in_context);

    /**
     * @brief Get retry statistics
     */
    struct SRetryStats {
        TUInt32 totalAttempts{0};
        TUInt32 successfulRetries{0};
        TUInt32 failedRetries{0};
        TVector<TString> retriedSteps;
    };

    [[nodiscard]] SRetryStats GetStatistics() const;

private:
    TUInt32 CalculateDelay(TUInt32 in_attemptNumber) const;
    bool IsRetriable(const CResult& in_result) const;

    SRetryPolicy m_policy;
    SRetryStats m_stats;
};

} // namespace TestMATE
```

## 2.2 Implementation

```cpp
// src/core/retry/RetryExecutor.cpp

CResult CRetryExecutor::ExecuteWithRetry(
    ITestStep* in_pStep,
    CExecutionContext& in_context) {

    CResult lastResult;

    for (TUInt32 attempt = 0; attempt <= m_policy.maxRetries; ++attempt) {
        // Execute step
        SStepResult stepResult;
        lastResult = in_pStep->Execute(stepResult);

        if (lastResult.IsSuccess()) {
            if (attempt > 0) {
                LOG_INFO("RetryExecutor",
                         "Step {} succeeded after {} retries",
                         in_pStep->GetId(), attempt);
                m_stats.successfulRetries++;
            }
            return lastResult;
        }

        // Check if error is retriable
        if (!IsRetriable(lastResult)) {
            LOG_DEBUG("RetryExecutor",
                     "Error {} is not retriable, failing immediately",
                     GetErrorCodeName(lastResult.GetCode()));
            return lastResult;
        }

        // Don't sleep after last attempt
        if (attempt < m_policy.maxRetries) {
            TUInt32 delayMs = CalculateDelay(attempt);

            LOG_WARNING("RetryExecutor",
                       "Step {} failed (attempt {}/{}), retrying in {}ms: {}",
                       in_pStep->GetId(),
                       attempt + 1,
                       m_policy.maxRetries + 1,
                       delayMs,
                       lastResult.GetMessage());

            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        m_stats.totalAttempts++;
    }

    // All retries exhausted
    m_stats.failedRetries++;
    m_stats.retriedSteps.push_back(in_pStep->GetId());

    return TESTMATE_FAILURE(
        lastResult.GetCode(),
        std::format("Step failed after {} retries: {}",
                   m_policy.maxRetries,
                   lastResult.GetMessage())
    );
}

TUInt32 CRetryExecutor::CalculateDelay(TUInt32 in_attemptNumber) const {
    TUInt32 delay = m_policy.initialDelayMs;

    switch (m_policy.strategy) {
        case ERetryStrategy::kFixedDelay:
            // No change
            break;

        case ERetryStrategy::kExponentialBackoff:
            delay = static_cast<TUInt32>(
                m_policy.initialDelayMs *
                std::pow(m_policy.backoffMultiplier, in_attemptNumber)
            );
            break;

        case ERetryStrategy::kLinearBackoff:
            delay = m_policy.initialDelayMs * (in_attemptNumber + 1);
            break;

        case ERetryStrategy::kJittered:
            delay = m_policy.initialDelayMs * (in_attemptNumber + 1);
            // Add random jitter ±25%
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.75, 1.25);
            delay = static_cast<TUInt32>(delay * dis(gen));
            break;
    }

    // Cap at max delay
    return std::min(delay, m_policy.maxDelayMs);
}
```

## 2.3 Usage Example

```cpp
// Configure retry policy
SRetryPolicy policy;
policy.maxRetries = 5;
policy.initialDelayMs = 100;
policy.strategy = ERetryStrategy::kExponentialBackoff;
policy.retriableErrors = {
    EErrorCode::kTimeout,
    EErrorCode::kConnectionLost,
    EErrorCode::kReceiveFailed
};

// Create retry executor
CRetryExecutor retryExec(policy);

// Execute test sequence with retry
for (auto* step : sequence->GetSteps()) {
    auto result = retryExec.ExecuteWithRetry(step, context);

    if (!result.IsSuccess()) {
        // Handle permanent failure
        break;
    }
}

// Get retry statistics
auto stats = retryExec.GetStatistics();
std::cout << "Total retries: " << stats.totalAttempts << std::endl;
std::cout << "Successful retries: " << stats.successfulRetries << std::endl;
```

## 2.4 Implementation Tasks

### Week 1
- [ ] Implement `SRetryPolicy` struct
- [ ] Implement `CRetryExecutor` class
- [ ] Add exponential backoff algorithm
- [ ] Write unit tests
- [ ] Integration with test executors

### Week 2
- [ ] Add retry statistics tracking
- [ ] Implement custom retry decision functions
- [ ] Performance testing
- [ ] Documentation
- [ ] Examples

---

# Phase 3: Performance Profiling

**Duration:** 2 weeks
**Priority:** HIGH
**Complexity:** Medium

## 3.1 Architecture

```cpp
/**************************************************************************
 * File Name: TestProfiler.h
 * Description: Performance profiling for test sequences
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include <chrono>

namespace TestMATE {

struct SStepProfile {
    TString stepId;
    TString stepName;
    TUInt64 executionTimeUs{0};    ///< Microseconds
    TUInt64 minTimeUs{UINT64_MAX};
    TUInt64 maxTimeUs{0};
    TUInt64 totalTimeUs{0};
    TUInt32 executionCount{0};
    TDouble averageTimeMs{0.0};
    TDouble percentOfTotal{0.0};
};

struct SProfilingReport {
    TUInt64 totalExecutionTimeUs{0};
    TVector<SStepProfile> stepProfiles;
    TVector<TString> bottlenecks;      ///< Steps taking >10% of total time
    TDouble averageStepTimeMs{0.0};

    TString GenerateReport() const;     ///< Human-readable report
    TString GenerateCSV() const;        ///< CSV for Excel
    TString GenerateJSON() const;       ///< JSON for programmatic use
};

class CTestProfiler {
public:
    CTestProfiler();

    /**
     * @brief Start profiling session
     */
    void StartProfiling();

    /**
     * @brief Stop profiling session
     */
    void StopProfiling();

    /**
     * @brief Record step execution start
     */
    void BeginStep(const TString& in_stepId, const TString& in_stepName);

    /**
     * @brief Record step execution end
     */
    void EndStep(const TString& in_stepId);

    /**
     * @brief Get profiling report
     */
    [[nodiscard]] SProfilingReport GenerateReport() const;

    /**
     * @brief Clear profiling data
     */
    void Reset();

    /**
     * @brief Save report to file
     */
    CResult SaveReport(const TString& in_filePath, const TString& in_format = "txt");

private:
    struct SStepTiming {
        std::chrono::steady_clock::time_point startTime;
        bool isRunning{false};
    };

    std::unordered_map<TString, SStepProfile> m_profiles;
    std::unordered_map<TString, SStepTiming> m_currentTimings;
    std::chrono::steady_clock::time_point m_sessionStartTime;
    bool m_isProfiling{false};
    mutable std::mutex m_mutex;
};

} // namespace TestMATE
```

## 3.2 Usage Example

```cpp
CTestProfiler profiler;
profiler.StartProfiling();

// Instrument test execution
for (auto* step : sequence->GetSteps()) {
    profiler.BeginStep(step->GetId(), step->GetName());

    auto result = step->Execute(stepResult);

    profiler.EndStep(step->GetId());
}

profiler.StopProfiling();

// Generate report
auto report = profiler.GenerateReport();
std::cout << report.GenerateReport() << std::endl;

// Save to file
profiler.SaveReport("profile_results.txt");
profiler.SaveReport("profile_results.csv", "csv");
profiler.SaveReport("profile_results.json", "json");
```

**Sample Output:**
```
=== TestMATE Performance Profile Report ===

Total Execution Time: 5,234.56 ms
Average Step Time: 261.73 ms
Total Steps Executed: 20

Top 5 Slowest Steps:
1. STEP-008-RF-SWEEP         2,145.23 ms  (41.0%) ⚠️ BOTTLENECK
2. STEP-012-SCOPE-CAPTURE    1,234.56 ms  (23.6%) ⚠️ BOTTLENECK
3. STEP-005-CALIBRATION        876.54 ms  (16.7%) ⚠️ BOTTLENECK
4. STEP-015-DATA-ANALYSIS      345.67 ms  ( 6.6%)
5. STEP-003-POWER-ON           234.56 ms  ( 4.5%)

Recommendations:
- Consider parallelizing RF-SWEEP and SCOPE-CAPTURE
- Optimize CALIBRATION step (takes 16.7% of total time)
- 3 steps account for 81.3% of execution time
```

---

# Phase 4: REST API Server

**Duration:** 3 weeks
**Priority:** HIGH
**Complexity:** High

## 4.1 Architecture

Using **Crow** (lightweight C++ web framework) or **Pistache**

```cpp
/**************************************************************************
 * File Name: RESTServer.h
 * Description: REST API for remote test control and monitoring
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include <crow.h>

namespace TestMATE {

class CRESTServer {
public:
    explicit CRESTServer(TUInt16 in_port = 8080);
    ~CRESTServer();

    /**
     * @brief Start REST API server
     */
    CResult Start();

    /**
     * @brief Stop server
     */
    void Stop();

    /**
     * @brief Register test controller
     */
    void SetTestController(ITestController* in_pController);

private:
    void RegisterEndpoints();

    // API endpoints
    crow::response HandleGetStatus();
    crow::response HandleExecuteTest(const crow::request& req);
    crow::response HandleAbortTest();
    crow::response HandleGetResults(TUInt64 testId);
    crow::response HandleListSequences();
    crow::response HandleGetSequence(const TString& id);

    crow::SimpleApp m_app;
    ITestController* m_pController{nullptr};
    TUInt16 m_port;
    std::thread m_serverThread;
};

} // namespace TestMATE
```

## 4.2 API Endpoints

### GET /api/v1/status
Get system status

**Response:**
```json
{
    "status": "ready",
    "version": "2.0.0",
    "uptime_seconds": 3600,
    "active_tests": 0,
    "connected_instruments": 5
}
```

### POST /api/v1/tests/execute
Execute a test sequence

**Request:**
```json
{
    "sequence_id": "VOLTAGE-TEST-001",
    "device_id": "DUT-12345",
    "lot_id": "LOT-2025-001",
    "parameters": {
        "voltage_limit": 5.0,
        "current_limit": 1.5
    }
}
```

**Response:**
```json
{
    "test_id": 12345,
    "status": "running",
    "started_at": "2025-11-23T10:30:00Z"
}
```

### GET /api/v1/tests/{test_id}
Get test results

**Response:**
```json
{
    "test_id": 12345,
    "status": "completed",
    "result": "PASS",
    "started_at": "2025-11-23T10:30:00Z",
    "completed_at": "2025-11-23T10:35:23Z",
    "duration_ms": 323000,
    "steps": [
        {
            "step_id": "STEP-001",
            "name": "Voltage Measure",
            "result": "PASS",
            "measured_value": 4.85,
            "limits": {"min": 4.5, "max": 5.5}
        }
    ]
}
```

### WebSocket: /api/v1/tests/{test_id}/stream
Real-time test progress updates

```javascript
// JavaScript client example
const ws = new WebSocket('ws://localhost:8080/api/v1/tests/12345/stream');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Progress:', data.current_step, data.percent_complete);
};
```

## 4.3 Implementation Tasks

### Week 1
- [ ] Choose web framework (Crow recommended)
- [ ] Implement basic server startup/shutdown
- [ ] Implement GET /status endpoint
- [ ] Implement GET /sequences endpoint
- [ ] Unit tests for server

### Week 2
- [ ] Implement POST /tests/execute
- [ ] Implement GET /tests/{id}
- [ ] Implement WebSocket streaming
- [ ] Authentication/authorization
- [ ] Integration tests

### Week 3
- [ ] Add CORS support
- [ ] Rate limiting
- [ ] API documentation (OpenAPI/Swagger)
- [ ] Performance testing
- [ ] Security audit

---

# Phase 5: Instrument Resource Manager

**Duration:** 2 weeks
**Priority:** MEDIUM
**Complexity:** Medium

## 5.1 Architecture

```cpp
/**************************************************************************
 * File Name: InstrumentPool.h
 * Description: Shared instrument resource management
 **************************************************************************/

#pragma once

namespace TestMATE {

enum class EInstrumentState {
    kAvailable,
    kReserved,
    kInUse,
    kError,
    kCalibrationDue
};

struct SInstrumentInfo {
    TString instrumentId;
    TString type;           // "DMM", "Scope", "PowerSupply"
    TString model;
    TString serialNumber;
    EInstrumentState state;
    TString currentUser;    // Empty if available
    TTime reservedUntil;
};

class CInstrumentPool {
public:
    static CInstrumentPool& GetInstance();

    /**
     * @brief Register instrument in pool
     */
    CResult RegisterInstrument(IInstrument* in_pInstrument);

    /**
     * @brief Reserve instrument for exclusive use
     * @param in_type Instrument type
     * @param in_timeoutMs Timeout if not available
     * @return Instrument pointer or nullptr
     */
    IInstrument* ReserveInstrument(
        const TString& in_type,
        TUInt32 in_timeoutMs = 5000);

    /**
     * @brief Release instrument back to pool
     */
    CResult ReleaseInstrument(IInstrument* in_pInstrument);

    /**
     * @brief Get available instruments of type
     */
    [[nodiscard]] TVector<SInstrumentInfo> GetAvailableInstruments(
        const TString& in_type) const;

    /**
     * @brief Check if instrument is available
     */
    [[nodiscard]] bool IsAvailable(const TString& in_instrumentId) const;

private:
    CInstrumentPool() = default;

    std::unordered_map<TString, IInstrument*> m_instruments;
    std::unordered_map<TString, SInstrumentInfo> m_instrumentInfo;
    mutable std::mutex m_mutex;
};

} // namespace TestMATE
```

---

# Implementation Timeline

## Gantt Chart

```
Week  1  2  3  4  5  6  7  8  9 10 11 12 13
      |--|--|--|--|--|--|--|--|--|--|--|--|
Phase 1: Interactive Debugging
      [====================]
         Core | Session | CLI | Polish

Phase 2: Retry & Recovery
                  [========]
                   Impl|Test

Phase 3: Performance Profiling
                        [========]
                         Impl|Test

Phase 4: REST API Server
                              [============]
                               Setup|API|WS

Phase 5: Instrument Pool
                                    [========]
                                     Impl|Test
```

## Milestones

- **Week 4:** ✅ Debugging system functional
- **Week 6:** ✅ Retry system integrated
- **Week 8:** ✅ Profiler generating reports
- **Week 11:** ✅ REST API operational
- **Week 13:** ✅ Full integration complete

---

# Testing Strategy

## Test Coverage Goals

| Component | Unit Tests | Integration Tests | Target Coverage |
|-----------|-----------|-------------------|-----------------|
| Debugging | 50+ | 10+ | >90% |
| Retry | 30+ | 5+ | >85% |
| Profiler | 20+ | 5+ | >80% |
| REST API | 40+ | 15+ | >85% |
| Instrument Pool | 25+ | 8+ | >85% |

## Continuous Integration

```yaml
# .github/workflows/ci.yml
name: TestMATE CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake g++ libsqlite3-dev

    - name: Build
      run: |
        mkdir build && cd build
        cmake -DTESTMATE_BUILD_TESTS=ON ..
        make -j4

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure

    - name: Coverage report
      run: |
        cd build
        lcov --capture --directory . --output-file coverage.info
        genhtml coverage.info --output-directory coverage_html

    - name: Upload coverage
      uses: codecov/codecov-action@v3
```

---

# Documentation Requirements

## 1. API Documentation
- Doxygen for all public APIs
- Example code for each major feature
- Migration guide from v1.x

## 2. User Guides
- Getting Started with Debugging
- Performance Optimization Guide
- REST API Reference
- Instrument Management Best Practices

## 3. Developer Documentation
- Architecture overview
- Design decisions
- Contributing guidelines
- Code style guide

## 4. Release Notes
- Feature descriptions
- Breaking changes
- Migration path
- Known issues

---

# Success Metrics

## Development Metrics
- Code coverage: >85%
- All tests passing
- Zero critical bugs before release
- Documentation complete

## Performance Metrics
- Debug overhead: <5% when enabled
- REST API latency: <100ms
- Profiler overhead: <2%
- Memory usage: <500MB additional

## User Metrics
- Debugging reduces dev time by >50%
- Retry improves reliability by >30%
- API enables 100% remote operation
- Profiler identifies bottlenecks in <5s

---

# Risk Management

## Technical Risks

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Thread safety bugs | HIGH | MEDIUM | Extensive testing, code review |
| Performance degradation | HIGH | LOW | Profiling, benchmarks |
| Breaking changes | MEDIUM | MEDIUM | Backward compatibility layer |
| Security vulnerabilities | HIGH | LOW | Security audit, authentication |

## Schedule Risks

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Debugging takes longer | MEDIUM | MEDIUM | Reduce scope, phased release |
| REST API complexity | MEDIUM | LOW | Use proven framework (Crow) |
| Integration issues | HIGH | MEDIUM | Early integration, continuous testing |

---

# Appendix

## A. Dependencies

- **Crow:** REST API framework
- **JSON for Modern C++:** JSON parsing
- **spdlog:** Logging (already used)
- **Google Test:** Testing (already used)
- **Doxygen:** Documentation (already used)

## B. Code Style

All new code must follow TestMATE coding standards:
- C++20 features
- Hungarian notation for member variables
- Doxygen comments
- RAII for resource management
- No raw pointers in public APIs

## C. Review Process

1. Feature branch → PR
2. Code review by 2+ developers
3. All tests passing
4. Coverage maintained
5. Documentation updated
6. Merge to main

---

**END OF IMPLEMENTATION PLAN**
