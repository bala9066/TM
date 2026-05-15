/**
 * @file DebugSession.cpp
 * @brief Implementation of CDebugSession class
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/DebugSession.h"
#include <chrono>

namespace TestMATE {

// ==================== Constructor & Destructor ====================

CDebugSession::CDebugSession(
    std::shared_ptr<CBreakpointManager> in_breakpointManager,
    const SDebugSessionConfig& in_config)
    : m_config(in_config)
    , m_breakpointManager(std::move(in_breakpointManager))
    , m_sessionStartTime(std::chrono::system_clock::now()) {
}

CDebugSession::~CDebugSession() {
    // Ensure any waiting threads are released
    if (m_state == EDebugState::kPaused) {
        Abort();
    }
}

// ==================== Session Lifecycle ====================

void CDebugSession::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_state = EDebugState::kRunning;
    m_pendingAction = EDebugAction::kNone;
    m_callStack.clear();
    m_currentStepId.clear();
    m_currentStepName.clear();
    m_lastException.clear();
    m_lastError.clear();
    m_stats = SDebugSessionStats{};
    m_sessionStartTime = std::chrono::system_clock::now();
}

void CDebugSession::Finish() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_state = EDebugState::kFinished;

    // Calculate total execution time
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_sessionStartTime).count();
    m_stats.executionTimeMs = static_cast<TDouble>(duration);

    // Wake up any waiting executor thread
    m_cv.notify_all();
}

void CDebugSession::Abort() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_state = EDebugState::kAborted;
    m_pendingAction = EDebugAction::kAbort;

    // Wake up any waiting executor thread
    m_cv.notify_all();
}

bool CDebugSession::IsActive() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state != EDebugState::kIdle &&
           m_state != EDebugState::kFinished &&
           m_state != EDebugState::kAborted;
}

EDebugState CDebugSession::GetState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

// ==================== Execution Control (Called by Test Executor) ====================

void CDebugSession::OnStepEnter(const TString& in_stepId, const TString& in_stepName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_state != EDebugState::kRunning && m_state != EDebugState::kStepping) {
        return;
    }

    m_currentStepId = in_stepId;
    m_currentStepName = in_stepName;
    m_stats.stepsExecuted++;

    // Push call frame
    PushCallFrame(in_stepId, in_stepName);

    // Print step if configured
    if (m_config.printStepsOnRun) {
        // In a real implementation, this would call a logger
        // For now, we just track it
    }
}

void CDebugSession::OnStepExit(const TString& in_stepId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_state != EDebugState::kRunning && m_state != EDebugState::kStepping) {
        return;
    }

    // Pop call frame
    if (!m_callStack.empty() && m_callStack.back().stepId == in_stepId) {
        PopCallFrame();
    }

    // Update current step to parent (if any)
    if (!m_callStack.empty()) {
        m_currentStepId = m_callStack.back().stepId;
        m_currentStepName = m_callStack.back().stepName;
    } else {
        m_currentStepId.clear();
        m_currentStepName.clear();
    }
}

bool CDebugSession::ShouldBreak() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_state != EDebugState::kRunning && m_state != EDebugState::kStepping) {
        return false;
    }

    // Check if we're stepping
    if (m_state == EDebugState::kStepping) {
        // Step into: always break
        if (m_pendingAction == EDebugAction::kStepInto) {
            return true;
        }

        // Step over: break if at same or shallower depth
        if (m_pendingAction == EDebugAction::kStepOver) {
            if (m_callStack.size() <= m_stepOverDepth) {
                return true;
            }
        }

        // Step out: break if returned to parent
        if (m_pendingAction == EDebugAction::kStepOut) {
            if (!m_stepOutTargetStepId.empty() && m_currentStepId == m_stepOutTargetStepId) {
                return true;
            }
        }
    }

    // Check run-to-cursor
    if (!m_runToCursorTarget.empty() && m_currentStepId == m_runToCursorTarget) {
        m_runToCursorTarget.clear();
        return true;
    }

    // Check breakpoints
    if (m_breakpointManager) {
        auto bpId = m_breakpointManager->CheckBreakpoint(m_currentStepId);
        if (bpId != 0) {
            m_stats.breakpointsHit++;
            return true;
        }
    }

    return false;
}

EDebugAction CDebugSession::WaitForDebugAction() {
    std::unique_lock<std::mutex> lock(m_mutex);

    // Pause execution
    m_state = EDebugState::kPaused;
    m_pendingAction = EDebugAction::kNone;
    m_executorWaiting = true;

    // Wait for action from debugger
    m_cv.wait(lock, [this]() {
        return m_pendingAction != EDebugAction::kNone ||
               m_state == EDebugState::kAborted;
    });

    m_executorWaiting = false;

    // Get the action
    EDebugAction action = m_pendingAction;
    m_pendingAction = EDebugAction::kNone;

    // Update state based on action
    if (action == EDebugAction::kAbort) {
        m_state = EDebugState::kAborted;
    } else if (action == EDebugAction::kStepOver ||
               action == EDebugAction::kStepInto ||
               action == EDebugAction::kStepOut) {
        m_state = EDebugState::kStepping;
    } else {
        m_state = EDebugState::kRunning;
    }

    return action;
}

void CDebugSession::OnException(const TString& in_exceptionMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_lastException = in_exceptionMessage;
    m_stats.exceptionsOccurred++;

    // Auto-break if configured
    if (m_config.autoBreakOnException) {
        // Force a pause on next ShouldBreak() check
        // In real implementation, might set a flag here
    }
}

void CDebugSession::OnError(const TString& in_stepId, const TString& in_errorMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_lastError = in_errorMessage;

    // Auto-break if configured
    if (m_config.autoBreakOnError) {
        // Force a pause on next ShouldBreak() check
    }
}

// ==================== Debug Actions (Called by Debugger/CLI) ====================

void CDebugSession::Continue() {
    ResumeExecution(EDebugAction::kContinue);
}

void CDebugSession::StepOver() {
    // ResumeExecution will lock the mutex
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stepOverDepth = static_cast<TUInt32>(m_callStack.size());
    }
    ResumeExecution(EDebugAction::kStepOver);
}

void CDebugSession::StepInto() {
    ResumeExecution(EDebugAction::kStepInto);
}

void CDebugSession::StepOut() {
    // ResumeExecution will lock the mutex
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Set target to parent step (one level up)
        if (m_callStack.size() >= 2) {
            m_stepOutTargetStepId = m_callStack[m_callStack.size() - 2].stepId;
        } else {
            m_stepOutTargetStepId.clear();
        }
    }
    ResumeExecution(EDebugAction::kStepOut);
}

void CDebugSession::RunToCursor(const TString& in_targetStepId) {
    // ResumeExecution will lock the mutex
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_runToCursorTarget = in_targetStepId;
    }
    ResumeExecution(EDebugAction::kContinue);
}

// ==================== Inspection (Called by Debugger/CLI) ====================

TVector<SCallFrame> CDebugSession::GetCallStack() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_callStack;
}

TString CDebugSession::GetCurrentStepId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStepId;
}

TString CDebugSession::GetCurrentStepName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStepName;
}

std::optional<TVariableValue> CDebugSession::GetVariable(const TString& in_variableName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check registered accessors first
    auto accessorIt = m_variableAccessors.find(in_variableName);
    if (accessorIt != m_variableAccessors.end()) {
        try {
            return accessorIt->second();
        } catch (...) {
            return std::nullopt;
        }
    }

    // Check stored variables
    auto it = m_variables.find(in_variableName);
    if (it != m_variables.end()) {
        return it->second;
    }

    return std::nullopt;
}

TMap<TString, TVariableValue> CDebugSession::GetAllVariables() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TMap<TString, TVariableValue> result = m_variables;

    // Add accessor-based variables
    for (const auto& [name, accessor] : m_variableAccessors) {
        try {
            result[name] = accessor();
        } catch (...) {
            // Skip variables that throw
        }
    }

    return result;
}

bool CDebugSession::SetVariable(const TString& in_variableName, const TVariableValue& in_value) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Can only set stored variables, not accessor-based ones
    auto it = m_variables.find(in_variableName);
    if (it != m_variables.end()) {
        it->second = in_value;
        return true;
    }

    // Variable not found
    return false;
}

void CDebugSession::RegisterVariableAccessor(
    const TString& in_variableName,
    std::function<TVariableValue()> in_accessor) {

    std::lock_guard<std::mutex> lock(m_mutex);
    m_variableAccessors[in_variableName] = std::move(in_accessor);
}

// ==================== Statistics & Status ====================

SDebugSessionStats CDebugSession::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

TString CDebugSession::GetLastException() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastException;
}

TString CDebugSession::GetLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

bool CDebugSession::IsPaused() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state == EDebugState::kPaused;
}

// ==================== Private Methods ====================

void CDebugSession::PauseExecution() {
    // Assumes mutex is already locked
    m_state = EDebugState::kPaused;
}

void CDebugSession::ResumeExecution(EDebugAction in_action) {
    // Note: This overload is called when mutex is NOT locked
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pendingAction = in_action;
    m_cv.notify_all();
}

void CDebugSession::PushCallFrame(const TString& in_stepId, const TString& in_stepName) {
    // Assumes mutex is already locked

    // Check max depth
    if (m_callStack.size() >= m_config.maxCallStackDepth) {
        return; // Don't push if at max depth
    }

    SCallFrame frame;
    frame.stepId = in_stepId;
    frame.stepName = in_stepName;
    frame.depth = static_cast<TUInt32>(m_callStack.size());

    m_callStack.push_back(frame);
}

void CDebugSession::PopCallFrame() {
    // Assumes mutex is already locked

    if (!m_callStack.empty()) {
        m_callStack.pop_back();
    }
}

void CDebugSession::UpdateStats() {
    // Assumes mutex is already locked
    // Update any derived statistics here
}

} // namespace TestMATE
