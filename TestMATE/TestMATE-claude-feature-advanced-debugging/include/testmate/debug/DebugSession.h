/**
 * @file DebugSession.h
 * @brief Debug session manager for interactive test execution control
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the CDebugSession class which manages a debugging session,
 * including execution control (pause, resume, step), variable inspection,
 * and synchronization between the test executor and debugger.
 */

#ifndef TESTMATE_DEBUG_DEBUG_SESSION_H
#define TESTMATE_DEBUG_DEBUG_SESSION_H

#include "testmate/debug/BreakpointManager.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <string>
#include <mutex>
#include <memory>
#include <variant>

namespace TestMATE {

// Type aliases
using TInt32 = int32_t;
using TInt64 = int64_t;
template<typename K, typename V>
using TMap = std::map<K, V>;

/**
 * @brief Debug actions that can be taken when execution is paused
 */
enum class EDebugAction {
    kNone,          ///< No action (waiting)
    kContinue,      ///< Continue execution until next breakpoint
    kStepOver,      ///< Execute current step and pause at next step
    kStepInto,      ///< Step into substep/function call
    kStepOut,       ///< Execute until current step exits
    kRunToCursor,   ///< Run until specific step ID
    kAbort          ///< Abort test execution
};

/**
 * @brief Debug session state
 */
enum class EDebugState {
    kIdle,          ///< No debugging session active
    kRunning,       ///< Test executing normally
    kPaused,        ///< Execution paused at breakpoint
    kStepping,      ///< Single-stepping through code
    kFinished,      ///< Test execution completed
    kAborted        ///< Test execution aborted
};

/**
 * @brief Call stack frame for debugging
 */
struct SCallFrame {
    TString stepId;                 ///< Step identifier
    TString stepName;               ///< Human-readable step name
    TUInt32 depth{0};              ///< Stack depth (0 = top)
    TMap<TString, TString> locals; ///< Local variables (name -> value as string)
};

/**
 * @brief Variable value (supports multiple types)
 */
using TVariableValue = std::variant<
    bool,
    TInt32,
    TUInt32,
    TInt64,
    TUInt64,
    TDouble,
    TString
>;

/**
 * @brief Debug session configuration
 */
struct SDebugSessionConfig {
    bool autoBreakOnException{true};    ///< Automatically break when exception occurs
    bool autoBreakOnError{true};        ///< Automatically break when step fails
    bool printStepsOnRun{false};        ///< Print step names during execution
    TUInt32 maxCallStackDepth{100};    ///< Maximum call stack depth to track
};

/**
 * @brief Debug session statistics
 */
struct SDebugSessionStats {
    TUInt32 breakpointsHit{0};         ///< Number of breakpoints hit
    TUInt32 stepsExecuted{0};          ///< Number of steps executed
    TUInt32 exceptionsOccurred{0};     ///< Number of exceptions caught
    TDouble executionTimeMs{0.0};      ///< Total execution time
};

/**
 * @brief Manages an interactive debugging session
 *
 * CDebugSession coordinates the debugging session between the test executor
 * and the debugger UI/CLI. It handles:
 * - Execution control (pause/resume/step)
 * - Call stack tracking
 * - Variable inspection
 * - Thread synchronization
 *
 * Thread Safety:
 * - Executor thread: Calls OnStepEnter(), OnStepExit(), WaitForDebugAction()
 * - Debugger thread: Calls Continue(), StepOver(), etc.
 * - All methods are thread-safe
 *
 * Example usage:
 * @code
 * // Executor thread:
 * CDebugSession session(breakpointManager);
 * session.Start();
 *
 * for (auto step : steps) {
 *     session.OnStepEnter(step->GetId(), step->GetName());
 *
 *     // Check if should break
 *     if (session.ShouldBreak()) {
 *         session.WaitForDebugAction(); // Blocks until user continues
 *     }
 *
 *     step->Execute();
 *     session.OnStepExit(step->GetId());
 * }
 *
 * session.Finish();
 *
 * // Debugger thread:
 * session.StepOver();  // User presses "step over" button
 * @endcode
 */
class CDebugSession {
public:
    /**
     * @brief Construct a new debug session
     * @param in_breakpointManager Shared breakpoint manager
     * @param in_config Session configuration
     */
    explicit CDebugSession(
        std::shared_ptr<CBreakpointManager> in_breakpointManager,
        const SDebugSessionConfig& in_config = {});

    /**
     * @brief Destructor
     */
    ~CDebugSession();

    // Prevent copying
    CDebugSession(const CDebugSession&) = delete;
    CDebugSession& operator=(const CDebugSession&) = delete;

    // ==================== Session Lifecycle ====================

    /**
     * @brief Start a new debugging session
     */
    void Start();

    /**
     * @brief Finish the debugging session normally
     */
    void Finish();

    /**
     * @brief Abort the debugging session (terminates execution)
     */
    void Abort();

    /**
     * @brief Check if session is active
     * @return true if session is active, false otherwise
     */
    [[nodiscard]] bool IsActive() const;

    /**
     * @brief Get current session state
     * @return Current state
     */
    [[nodiscard]] EDebugState GetState() const;

    // ==================== Execution Control (Called by Test Executor) ====================

    /**
     * @brief Called when entering a test step
     * @param in_stepId Step identifier
     * @param in_stepName Human-readable step name
     */
    void OnStepEnter(const TString& in_stepId, const TString& in_stepName);

    /**
     * @brief Called when exiting a test step
     * @param in_stepId Step identifier
     */
    void OnStepExit(const TString& in_stepId);

    /**
     * @brief Check if execution should break (pause) at current location
     * @return true if should break, false if should continue
     */
    [[nodiscard]] bool ShouldBreak();

    /**
     * @brief Wait for debug action from user (blocks until action received)
     * @return Action to take (Continue, StepOver, Abort, etc.)
     */
    [[nodiscard]] EDebugAction WaitForDebugAction();

    /**
     * @brief Notify that an exception occurred
     * @param in_exceptionMessage Exception message
     */
    void OnException(const TString& in_exceptionMessage);

    /**
     * @brief Notify that a step failed
     * @param in_stepId Step that failed
     * @param in_errorMessage Error message
     */
    void OnError(const TString& in_stepId, const TString& in_errorMessage);

    // ==================== Debug Actions (Called by Debugger/CLI) ====================

    /**
     * @brief Continue execution until next breakpoint
     */
    void Continue();

    /**
     * @brief Step over current step (execute and pause at next step)
     */
    void StepOver();

    /**
     * @brief Step into substep or function call
     */
    void StepInto();

    /**
     * @brief Step out of current step (run until step exits)
     */
    void StepOut();

    /**
     * @brief Run to specific cursor location
     * @param in_targetStepId Target step ID to run to
     */
    void RunToCursor(const TString& in_targetStepId);

    // ==================== Inspection (Called by Debugger/CLI) ====================

    /**
     * @brief Get current call stack
     * @return Vector of call frames (top of stack first)
     */
    [[nodiscard]] TVector<SCallFrame> GetCallStack() const;

    /**
     * @brief Get current step ID
     * @return Current step identifier (empty if not in a step)
     */
    [[nodiscard]] TString GetCurrentStepId() const;

    /**
     * @brief Get current step name
     * @return Current step name (empty if not in a step)
     */
    [[nodiscard]] TString GetCurrentStepName() const;

    /**
     * @brief Get variable value
     * @param in_variableName Variable name
     * @return Variable value (nullopt if not found)
     */
    [[nodiscard]] std::optional<TVariableValue> GetVariable(const TString& in_variableName) const;

    /**
     * @brief Get all variables at current location
     * @return Map of variable names to values
     */
    [[nodiscard]] TMap<TString, TVariableValue> GetAllVariables() const;

    /**
     * @brief Set variable value (for inspection/modification)
     * @param in_variableName Variable name
     * @param in_value New value
     * @return true if set successfully, false if variable not found
     */
    bool SetVariable(const TString& in_variableName, const TVariableValue& in_value);

    /**
     * @brief Register a variable accessor (callback to get variable value)
     * @param in_variableName Variable name
     * @param in_accessor Function that returns variable value
     */
    void RegisterVariableAccessor(const TString& in_variableName,
                                  std::function<TVariableValue()> in_accessor);

    // ==================== Statistics & Status ====================

    /**
     * @brief Get session statistics
     * @return Statistics structure
     */
    [[nodiscard]] SDebugSessionStats GetStats() const;

    /**
     * @brief Get last exception message
     * @return Exception message (empty if no exception)
     */
    [[nodiscard]] TString GetLastException() const;

    /**
     * @brief Get last error message
     * @return Error message (empty if no error)
     */
    [[nodiscard]] TString GetLastError() const;

    /**
     * @brief Check if execution is paused
     * @return true if paused, false otherwise
     */
    [[nodiscard]] bool IsPaused() const;

private:
    /**
     * @brief Pause execution and wait for user action
     */
    void PauseExecution();

    /**
     * @brief Resume execution with specified action
     * @param in_action Action to take
     */
    void ResumeExecution(EDebugAction in_action);

    /**
     * @brief Push a new call frame onto the stack
     * @param in_stepId Step ID
     * @param in_stepName Step name
     */
    void PushCallFrame(const TString& in_stepId, const TString& in_stepName);

    /**
     * @brief Pop the top call frame from the stack
     */
    void PopCallFrame();

    /**
     * @brief Update statistics
     */
    void UpdateStats();

    // Configuration
    SDebugSessionConfig m_config;

    // Breakpoint manager
    std::shared_ptr<CBreakpointManager> m_breakpointManager;

    // Session state
    EDebugState m_state{EDebugState::kIdle};
    EDebugAction m_pendingAction{EDebugAction::kNone};

    // Call stack
    TVector<SCallFrame> m_callStack;
    TString m_currentStepId;
    TString m_currentStepName;

    // Stepping state
    TString m_stepOutTargetStepId;     ///< Target step for step-out
    TString m_runToCursorTarget;       ///< Target step for run-to-cursor
    TUInt32 m_stepOverDepth{0};        ///< Depth for step-over

    // Variable tracking
    TMap<TString, TVariableValue> m_variables;
    TMap<TString, std::function<TVariableValue()>> m_variableAccessors;

    // Exception/error tracking
    TString m_lastException;
    TString m_lastError;

    // Statistics
    SDebugSessionStats m_stats;
    TTime m_sessionStartTime;

    // Thread synchronization
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_executorWaiting{false};
};

} // namespace TestMATE

#endif // TESTMATE_DEBUG_DEBUG_SESSION_H
