/**************************************************************************
 * File Name: SequentialModel.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of sequential process model
 * Requirements: REQ-PM-010 to REQ-PM-018
 **************************************************************************/

#include "SequentialModel.h"
#include "utils/LogManager.h"
#include "utils/TimeUtils.h"

namespace TestMATE {

namespace {
    const char* kLogSource = "SequentialModel";
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CSequentialModel::CSequentialModel()
    : CProcessModelBase()
    , m_context(0, 0)  // Socket 0, Site 0 for sequential
{
}

CSequentialModel::~CSequentialModel() {
    // Ensure thread is stopped
    if (m_pExecutionThread && m_pExecutionThread->joinable()) {
        Abort();
        m_pExecutionThread->join();
    }
}

//=============================================================================
// IProcessModel Implementation
//=============================================================================

CResult CSequentialModel::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_eState != EProcessModelState::kIdle) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Cannot start execution from current state");
    }

    if (!m_pSequence) {
        return TESTMATE_FAILURE(EErrorCode::kNotInitialized,
                                "No test sequence loaded");
    }

    // Ensure previous thread is cleaned up
    if (m_pExecutionThread && m_pExecutionThread->joinable()) {
        m_pExecutionThread->join();
    }

    // Reset state
    m_uiCurrentStep = 0;
    m_context.ClearAllVariables();
    m_context.ClearAbortRequest();
    m_startTime = TimeUtils::Now();
    m_context.SetStartTime(m_startTime);

    // Start execution thread
    SetState(EProcessModelState::kRunning);
    m_pExecutionThread = std::make_unique<std::thread>(&CSequentialModel::ExecutionThreadFunc, this);

    LOG_INFO(kLogSource, "Sequential execution started");

    return TESTMATE_SUCCESS();
}

bool CSequentialModel::WaitForCompletion(TInt64 in_timeoutMs) {
    if (!m_pExecutionThread || !m_pExecutionThread->joinable()) {
        return true;
    }

    if (in_timeoutMs <= 0) {
        m_pExecutionThread->join();
        return true;
    }

    // Timed wait using polling
    auto startTime = TimeUtils::Now();
    while (IsRunning() || IsPaused()) {
        if (TimeUtils::ElapsedMs(startTime) >= in_timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (m_pExecutionThread->joinable()) {
        m_pExecutionThread->join();
    }

    return true;
}

//=============================================================================
// Private Methods
//=============================================================================

void CSequentialModel::ExecutionThreadFunc() {
    LOG_DEBUG(kLogSource, "Execution thread started");

    try {
        SetState(EProcessModelState::kInitializing);

        // Initialize resources (placeholder)
        // TODO: Implement resource initialization when resource system is ready

        SetState(EProcessModelState::kRunning);

        // Get total steps (placeholder - will use actual sequence)
        TUInt32 totalSteps = GetTotalSteps();
        if (totalSteps == 0) {
            // Simulate some steps for testing
            totalSteps = 10;
        }

        // Execute steps sequentially
        for (TUInt32 stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
            // Check for abort
            if (m_context.IsAbortRequested() ||
                m_eState == EProcessModelState::kAborted) {
                LOG_INFO(kLogSource, "Execution aborted at step {}", stepIndex);
                break;
            }

            m_uiCurrentStep = stepIndex;
            NotifyProgress(stepIndex, totalSteps);

            // Execute step (placeholder - will use actual step from sequence)
            ETestVerdict verdict = ExecuteStep(m_context, nullptr);

            if (verdict == ETestVerdict::kAborted) {
                break;
            }

            LOG_TRACE(kLogSource, "Step {} completed with verdict {}",
                      stepIndex, static_cast<int>(verdict));
        }

        // Final state
        if (m_eState == EProcessModelState::kAborted) {
            // Already in aborted state
        } else if (m_context.GetFailCount() > 0) {
            SetState(EProcessModelState::kCompleted);
        } else {
            SetState(EProcessModelState::kCompleted);
        }

        NotifyProgress(totalSteps, totalSteps);

        LOG_INFO(kLogSource, "Execution completed. Pass: {}, Fail: {}",
                 m_context.GetPassCount(), m_context.GetFailCount());

    } catch (const std::exception& ex) {
        LOG_ERROR(kLogSource, "Exception during execution: {}", ex.what());
        SetState(EProcessModelState::kError);
    } catch (...) {
        LOG_ERROR(kLogSource, "Unknown exception during execution");
        SetState(EProcessModelState::kError);
    }

    LOG_DEBUG(kLogSource, "Execution thread finished");
}

} // namespace TestMATE
