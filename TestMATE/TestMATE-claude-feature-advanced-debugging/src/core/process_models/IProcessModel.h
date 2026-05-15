/**************************************************************************
 * File Name: IProcessModel.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Interface definition for test execution process models.
 *              Defines contract for Sequential, Parallel, and Batch models.
 * Requirements: REQ-PM-001 to REQ-PM-005
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>

namespace TestMATE {

// Forward declarations
class CExecutionContext;
class ITestStep;
class CTestSequence;

//=============================================================================
// Process Model State
//=============================================================================

/**************************************************************************
 * Enum: EProcessModelState
 * Description: Current state of process model execution
 * Requirements: REQ-PM-003
 **************************************************************************/
enum class EProcessModelState : TInt32 {
    kIdle = 0,          ///< Not running, ready to start
    kInitializing = 1,  ///< Setting up resources
    kRunning = 2,       ///< Actively executing tests
    kPaused = 3,        ///< Execution paused
    kStopping = 4,      ///< Gracefully stopping
    kCompleted = 5,     ///< Execution finished normally
    kError = 6,         ///< Error state
    kAborted = 7        ///< Execution aborted by user
};

/**************************************************************************
 * Enum: EProcessModelType
 * Description: Type identifier for process models
 * Requirements: REQ-PM-001
 **************************************************************************/
enum class EProcessModelType : TInt32 {
    kSequential = 0,    ///< Single-threaded sequential execution
    kParallel = 1,      ///< Multi-socket parallel execution
    kBatch = 2,         ///< Batch/lot-based execution
    kCustom = 99        ///< Custom user-defined model
};

//=============================================================================
// Callback Types
//=============================================================================

/// Callback invoked before each step execution
using TPreStepCallback = std::function<CResult(CExecutionContext&, ITestStep&)>;

/// Callback invoked after each step execution
using TPostStepCallback = std::function<void(CExecutionContext&, ITestStep&, ETestVerdict)>;

/// Callback invoked on execution state change
using TStateChangeCallback = std::function<void(EProcessModelState, EProcessModelState)>;

/// Callback for progress updates
using TProgressCallback = std::function<void(TUInt32 current, TUInt32 total)>;

//=============================================================================
// Process Model Interface
//=============================================================================

/**************************************************************************
 * Class: IProcessModel
 * Description: Abstract interface for test execution process models.
 *              All process models (Sequential, Parallel, Batch) must
 *              implement this interface.
 * Requirements: REQ-PM-001 to REQ-PM-005
 **************************************************************************/
class IProcessModel {
public:
    virtual ~IProcessModel() = default;

    //=========================================================================
    // Identification
    //=========================================================================

    /**************************************************************************
     * Function Name: GetType
     * Description: Returns the type of this process model
     * Returns: Process model type identifier
     * Requirements: REQ-PM-001
     **************************************************************************/
    [[nodiscard]] virtual EProcessModelType GetType() const = 0;

    /**************************************************************************
     * Function Name: GetName
     * Description: Returns human-readable name of this model
     * Returns: Model name string
     **************************************************************************/
    [[nodiscard]] virtual TString GetName() const = 0;

    /**************************************************************************
     * Function Name: GetDescription
     * Description: Returns description of this model's behavior
     * Returns: Description string
     **************************************************************************/
    [[nodiscard]] virtual TString GetDescription() const = 0;

    //=========================================================================
    // Lifecycle Management
    //=========================================================================

    /**************************************************************************
     * Function Name: Initialize
     * Description: Initializes the process model with configuration
     * Parameters:
     *   in_pSequence - Test sequence to execute
     * Returns: Result indicating success or failure
     * Requirements: REQ-PM-002
     **************************************************************************/
    virtual CResult Initialize(TSharedPtr<CTestSequence> in_pSequence) = 0;

    /**************************************************************************
     * Function Name: Start
     * Description: Starts test execution
     * Returns: Result indicating success or failure
     * Requirements: REQ-PM-003
     **************************************************************************/
    virtual CResult Start() = 0;

    /**************************************************************************
     * Function Name: Pause
     * Description: Pauses test execution (can be resumed)
     * Returns: Result indicating success or failure
     * Requirements: REQ-PM-004
     **************************************************************************/
    virtual CResult Pause() = 0;

    /**************************************************************************
     * Function Name: Resume
     * Description: Resumes paused execution
     * Returns: Result indicating success or failure
     * Requirements: REQ-PM-004
     **************************************************************************/
    virtual CResult Resume() = 0;

    /**************************************************************************
     * Function Name: Abort
     * Description: Aborts execution immediately
     * Returns: Result indicating success or failure
     * Requirements: REQ-PM-005
     **************************************************************************/
    virtual CResult Abort() = 0;

    /**************************************************************************
     * Function Name: Shutdown
     * Description: Cleans up resources and shuts down
     **************************************************************************/
    virtual void Shutdown() = 0;

    //=========================================================================
    // State Query
    //=========================================================================

    /**************************************************************************
     * Function Name: GetState
     * Description: Returns current execution state
     * Returns: Current state
     * Requirements: REQ-PM-003
     **************************************************************************/
    [[nodiscard]] virtual EProcessModelState GetState() const = 0;

    /**************************************************************************
     * Function Name: IsRunning
     * Description: Checks if model is currently executing
     * Returns: true if running
     **************************************************************************/
    [[nodiscard]] virtual bool IsRunning() const = 0;

    /**************************************************************************
     * Function Name: IsPaused
     * Description: Checks if execution is paused
     * Returns: true if paused
     **************************************************************************/
    [[nodiscard]] virtual bool IsPaused() const = 0;

    //=========================================================================
    // Progress Information
    //=========================================================================

    /**************************************************************************
     * Function Name: GetCurrentStepIndex
     * Description: Returns index of currently executing step
     * Returns: Current step index (0-based)
     **************************************************************************/
    [[nodiscard]] virtual TUInt32 GetCurrentStepIndex() const = 0;

    /**************************************************************************
     * Function Name: GetTotalSteps
     * Description: Returns total number of steps in sequence
     * Returns: Total step count
     **************************************************************************/
    [[nodiscard]] virtual TUInt32 GetTotalSteps() const = 0;

    /**************************************************************************
     * Function Name: GetElapsedTime
     * Description: Returns elapsed execution time
     * Returns: Elapsed time in milliseconds
     **************************************************************************/
    [[nodiscard]] virtual TInt64 GetElapsedTimeMs() const = 0;

    //=========================================================================
    // Callbacks
    //=========================================================================

    /**************************************************************************
     * Function Name: SetPreStepCallback
     * Description: Sets callback invoked before each step
     * Parameters:
     *   in_callback - Callback function
     **************************************************************************/
    virtual void SetPreStepCallback(TPreStepCallback in_callback) = 0;

    /**************************************************************************
     * Function Name: SetPostStepCallback
     * Description: Sets callback invoked after each step
     * Parameters:
     *   in_callback - Callback function
     **************************************************************************/
    virtual void SetPostStepCallback(TPostStepCallback in_callback) = 0;

    /**************************************************************************
     * Function Name: SetStateChangeCallback
     * Description: Sets callback for state changes
     * Parameters:
     *   in_callback - Callback function
     **************************************************************************/
    virtual void SetStateChangeCallback(TStateChangeCallback in_callback) = 0;

    /**************************************************************************
     * Function Name: SetProgressCallback
     * Description: Sets callback for progress updates
     * Parameters:
     *   in_callback - Callback function
     **************************************************************************/
    virtual void SetProgressCallback(TProgressCallback in_callback) = 0;
};

} // namespace TestMATE
