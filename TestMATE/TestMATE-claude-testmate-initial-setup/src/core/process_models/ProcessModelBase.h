/**************************************************************************
 * File Name: ProcessModelBase.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Abstract base class for process models providing common
 *              functionality for all execution models.
 * Requirements: REQ-PM-006 to REQ-PM-009
 **************************************************************************/

#pragma once

#include "IProcessModel.h"
#include "ExecutionContext.h"
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace TestMATE {

// Forward declaration
class CTestSequence;
class CTestStep;

/**************************************************************************
 * Class: CProcessModelBase
 * Description: Abstract base class implementing common process model
 *              functionality. Derived classes implement specific execution
 *              strategies (sequential, parallel, batch).
 * Requirements: REQ-PM-006 to REQ-PM-009
 **************************************************************************/
class CProcessModelBase : public IProcessModel {
public:
    CProcessModelBase();
    ~CProcessModelBase() override;

    // Non-copyable
    CProcessModelBase(const CProcessModelBase&) = delete;
    CProcessModelBase& operator=(const CProcessModelBase&) = delete;

    //=========================================================================
    // IProcessModel Implementation
    //=========================================================================

    CResult Initialize(TSharedPtr<CTestSequence> in_pSequence) override;
    CResult Pause() override;
    CResult Resume() override;
    CResult Abort() override;
    void Shutdown() override;

    [[nodiscard]] EProcessModelState GetState() const override;
    [[nodiscard]] bool IsRunning() const override;
    [[nodiscard]] bool IsPaused() const override;

    [[nodiscard]] TUInt32 GetCurrentStepIndex() const override;
    [[nodiscard]] TUInt32 GetTotalSteps() const override;
    [[nodiscard]] TInt64 GetElapsedTimeMs() const override;

    void SetPreStepCallback(TPreStepCallback in_callback) override;
    void SetPostStepCallback(TPostStepCallback in_callback) override;
    void SetStateChangeCallback(TStateChangeCallback in_callback) override;
    void SetProgressCallback(TProgressCallback in_callback) override;

protected:
    //=========================================================================
    // State Management
    //=========================================================================

    /**************************************************************************
     * Function Name: SetState
     * Description: Changes execution state and notifies listeners
     * Parameters:
     *   in_eNewState - New state to transition to
     **************************************************************************/
    void SetState(EProcessModelState in_eNewState);

    /**************************************************************************
     * Function Name: ValidateStateTransition
     * Description: Checks if state transition is valid
     * Parameters:
     *   in_eFrom - Current state
     *   in_eTo - Desired state
     * Returns: true if transition is valid
     **************************************************************************/
    [[nodiscard]] bool ValidateStateTransition(EProcessModelState in_eFrom,
                                                EProcessModelState in_eTo) const;

    //=========================================================================
    // Step Execution Helpers
    //=========================================================================

    /**************************************************************************
     * Function Name: ExecuteStep
     * Description: Executes a single test step with callbacks
     * Parameters:
     *   in_out_context - Execution context
     *   in_pStep - Step to execute
     * Returns: Test verdict from step execution
     * Requirements: REQ-PM-007
     **************************************************************************/
    ETestVerdict ExecuteStep(CExecutionContext& in_out_context, CTestStep* in_pStep);

    /**************************************************************************
     * Function Name: InvokePreStepCallback
     * Description: Invokes pre-step callback if registered
     * Parameters:
     *   in_out_context - Execution context
     *   in_out_step - Step about to execute
     * Returns: Result from callback
     **************************************************************************/
    CResult InvokePreStepCallback(CExecutionContext& in_out_context, CTestStep& in_out_step);

    /**************************************************************************
     * Function Name: InvokePostStepCallback
     * Description: Invokes post-step callback if registered
     * Parameters:
     *   in_out_context - Execution context
     *   in_step - Step that executed
     *   in_eVerdict - Step verdict
     **************************************************************************/
    void InvokePostStepCallback(CExecutionContext& in_out_context,
                                CTestStep& in_step,
                                ETestVerdict in_eVerdict);

    /**************************************************************************
     * Function Name: NotifyProgress
     * Description: Notifies progress callback
     * Parameters:
     *   in_uiCurrent - Current step index
     *   in_uiTotal - Total steps
     **************************************************************************/
    void NotifyProgress(TUInt32 in_uiCurrent, TUInt32 in_uiTotal);

    //=========================================================================
    // Protected Members
    //=========================================================================

    TSharedPtr<CTestSequence> m_pSequence;
    std::atomic<EProcessModelState> m_eState{EProcessModelState::kIdle};
    std::atomic<TUInt32> m_uiCurrentStep{0};
    TTimePoint m_startTime;

    // Callbacks
    TPreStepCallback m_fnPreStepCallback;
    TPostStepCallback m_fnPostStepCallback;
    TStateChangeCallback m_fnStateChangeCallback;
    TProgressCallback m_fnProgressCallback;

    mutable std::mutex m_mutex;
    std::condition_variable m_cvPause;
    std::atomic<bool> m_bPauseRequested{false};
};

} // namespace TestMATE
