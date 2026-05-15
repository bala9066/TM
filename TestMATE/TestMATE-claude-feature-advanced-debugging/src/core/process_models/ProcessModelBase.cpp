/**************************************************************************
 * File Name: ProcessModelBase.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of ProcessModelBase class
 * Requirements: REQ-PM-006 to REQ-PM-009
 **************************************************************************/

#include "ProcessModelBase.h"
#include "utils/LogManager.h"
#include "utils/TimeUtils.h"

namespace TestMATE {

namespace {
    const char* kLogSource = "ProcessModelBase";
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CProcessModelBase::CProcessModelBase()
    : m_pSequence(nullptr)
    , m_eState(EProcessModelState::kIdle)
    , m_uiCurrentStep(0)
{
}

CProcessModelBase::~CProcessModelBase() {
    Shutdown();
}

//=============================================================================
// IProcessModel Implementation
//=============================================================================

CResult CProcessModelBase::Initialize(TSharedPtr<CTestSequence> in_pSequence) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_eState != EProcessModelState::kIdle) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Cannot initialize while not in idle state");
    }

    if (!in_pSequence) {
        return TESTMATE_FAILURE(EErrorCode::kNullPointer,
                                "Test sequence cannot be null");
    }

    m_pSequence = in_pSequence;
    m_uiCurrentStep = 0;

    LOG_INFO(kLogSource, "Process model initialized with sequence");

    return TESTMATE_SUCCESS();
}

CResult CProcessModelBase::Pause() {
    if (m_eState != EProcessModelState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Can only pause from running state");
    }

    m_bPauseRequested = true;
    SetState(EProcessModelState::kPaused);

    LOG_INFO(kLogSource, "Execution paused");

    return TESTMATE_SUCCESS();
}

CResult CProcessModelBase::Resume() {
    if (m_eState != EProcessModelState::kPaused) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Can only resume from paused state");
    }

    m_bPauseRequested = false;
    SetState(EProcessModelState::kRunning);
    m_cvPause.notify_all();

    LOG_INFO(kLogSource, "Execution resumed");

    return TESTMATE_SUCCESS();
}

CResult CProcessModelBase::Abort() {
    EProcessModelState currentState = m_eState.load();

    if (currentState != EProcessModelState::kRunning &&
        currentState != EProcessModelState::kPaused) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Can only abort from running or paused state");
    }

    SetState(EProcessModelState::kAborted);

    // Wake up any paused thread
    m_bPauseRequested = false;
    m_cvPause.notify_all();

    LOG_WARNING(kLogSource, "Execution aborted");

    return TESTMATE_SUCCESS();
}

void CProcessModelBase::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // If running, abort first
    EProcessModelState currentState = m_eState.load();
    if (currentState == EProcessModelState::kRunning ||
        currentState == EProcessModelState::kPaused) {
        m_eState = EProcessModelState::kAborted;
        m_cvPause.notify_all();
    }

    m_pSequence.reset();
    m_eState = EProcessModelState::kIdle;
    m_uiCurrentStep = 0;

    LOG_DEBUG(kLogSource, "Process model shutdown complete");
}

EProcessModelState CProcessModelBase::GetState() const {
    return m_eState.load();
}

bool CProcessModelBase::IsRunning() const {
    return m_eState.load() == EProcessModelState::kRunning;
}

bool CProcessModelBase::IsPaused() const {
    return m_eState.load() == EProcessModelState::kPaused;
}

TUInt32 CProcessModelBase::GetCurrentStepIndex() const {
    return m_uiCurrentStep.load();
}

TUInt32 CProcessModelBase::GetTotalSteps() const {
    // Will be implemented when CTestSequence is available
    return 0;
}

TInt64 CProcessModelBase::GetElapsedTimeMs() const {
    if (m_eState == EProcessModelState::kIdle) {
        return 0;
    }
    return TimeUtils::ElapsedMs(m_startTime);
}

void CProcessModelBase::SetPreStepCallback(TPreStepCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fnPreStepCallback = std::move(in_callback);
}

void CProcessModelBase::SetPostStepCallback(TPostStepCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fnPostStepCallback = std::move(in_callback);
}

void CProcessModelBase::SetStateChangeCallback(TStateChangeCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fnStateChangeCallback = std::move(in_callback);
}

void CProcessModelBase::SetProgressCallback(TProgressCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fnProgressCallback = std::move(in_callback);
}

//=============================================================================
// Protected Methods
//=============================================================================

void CProcessModelBase::SetState(EProcessModelState in_eNewState) {
    EProcessModelState oldState = m_eState.exchange(in_eNewState);

    if (oldState != in_eNewState && m_fnStateChangeCallback) {
        m_fnStateChangeCallback(oldState, in_eNewState);
    }
}

bool CProcessModelBase::ValidateStateTransition(EProcessModelState in_eFrom,
                                                 EProcessModelState in_eTo) const {
    // Define valid transitions
    switch (in_eFrom) {
        case EProcessModelState::kIdle:
            return in_eTo == EProcessModelState::kInitializing;

        case EProcessModelState::kInitializing:
            return in_eTo == EProcessModelState::kRunning ||
                   in_eTo == EProcessModelState::kError;

        case EProcessModelState::kRunning:
            return in_eTo == EProcessModelState::kPaused ||
                   in_eTo == EProcessModelState::kCompleted ||
                   in_eTo == EProcessModelState::kAborted ||
                   in_eTo == EProcessModelState::kError;

        case EProcessModelState::kPaused:
            return in_eTo == EProcessModelState::kRunning ||
                   in_eTo == EProcessModelState::kAborted;

        case EProcessModelState::kCompleted:
        case EProcessModelState::kError:
        case EProcessModelState::kAborted:
            return in_eTo == EProcessModelState::kIdle;

        default:
            return false;
    }
}

ETestVerdict CProcessModelBase::ExecuteStep(CExecutionContext& in_out_context,
                                             CTestStep* in_pStep) {
    if (!in_pStep) {
        return ETestVerdict::kError;
    }

    // Check for pause
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_bPauseRequested && m_eState == EProcessModelState::kPaused) {
            m_cvPause.wait(lock);

            // Check if aborted while paused
            if (m_eState == EProcessModelState::kAborted) {
                return ETestVerdict::kAborted;
            }
        }
    }

    // Check for abort
    if (in_out_context.IsAbortRequested() ||
        m_eState == EProcessModelState::kAborted) {
        return ETestVerdict::kAborted;
    }

    // Pre-step callback
    CResult preResult = InvokePreStepCallback(in_out_context, *in_pStep);
    if (preResult.IsFailure()) {
        return ETestVerdict::kError;
    }

    // Execute step (placeholder - actual execution when CTestStep is implemented)
    ETestVerdict verdict = ETestVerdict::kPass;

    // Update context
    in_out_context.SetCurrentStepVerdict(verdict);
    if (verdict == ETestVerdict::kPass) {
        in_out_context.IncrementPassCount();
    } else if (verdict == ETestVerdict::kFail) {
        in_out_context.IncrementFailCount();
    }

    // Post-step callback
    InvokePostStepCallback(in_out_context, *in_pStep, verdict);

    return verdict;
}

CResult CProcessModelBase::InvokePreStepCallback(CExecutionContext& in_out_context,
                                                  CTestStep& in_out_step) {
    if (m_fnPreStepCallback) {
        return m_fnPreStepCallback(in_out_context, in_out_step);
    }
    return TESTMATE_SUCCESS();
}

void CProcessModelBase::InvokePostStepCallback(CExecutionContext& in_out_context,
                                                CTestStep& in_step,
                                                ETestVerdict in_eVerdict) {
    if (m_fnPostStepCallback) {
        m_fnPostStepCallback(in_out_context, in_step, in_eVerdict);
    }
}

void CProcessModelBase::NotifyProgress(TUInt32 in_uiCurrent, TUInt32 in_uiTotal) {
    if (m_fnProgressCallback) {
        m_fnProgressCallback(in_uiCurrent, in_uiTotal);
    }
}

} // namespace TestMATE
