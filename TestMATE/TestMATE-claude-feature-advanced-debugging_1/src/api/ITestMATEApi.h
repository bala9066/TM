/**************************************************************************
 * File Name: ITestMATEApi.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Main API interface for TestMATE core functionality.
 *              Provides clean separation between core and UI layers.
 * Requirements: REQ-API-001 to REQ-API-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include "core/process_models/IProcessModel.h"
#include <functional>

namespace TestMATE {

// Forward declarations
class IPlugin;

/**************************************************************************
 * Enum: EApiEvent
 * Description: Events that can be subscribed to via the API
 **************************************************************************/
enum class EApiEvent {
    kTestStarted,
    kTestCompleted,
    kStepStarted,
    kStepCompleted,
    kStateChanged,
    kLogMessage,
    kError,
    kProgressUpdate,
    kResourceChanged
};

/**************************************************************************
 * Struct: STestConfiguration
 * Description: Configuration for test execution
 **************************************************************************/
struct STestConfiguration {
    TString sequencePath;
    EProcessModelType modelType{EProcessModelType::kSequential};
    TUInt32 socketCount{1};
    bool stopOnFirstFailure{false};
    TInt64 timeoutMs{0};
    std::map<TString, TString> variables;
};

/**************************************************************************
 * Struct: STestStatus
 * Description: Current test execution status
 **************************************************************************/
struct STestStatus {
    EExecutionState state{EExecutionState::kIdle};
    TUInt32 currentStep{0};
    TUInt32 totalSteps{0};
    TUInt32 passCount{0};
    TUInt32 failCount{0};
    TDouble progress{0.0};
    TString currentStepName;
    TInt64 elapsedMs{0};
};

/**************************************************************************
 * Callback types
 **************************************************************************/
using FApiEventCallback = std::function<void(EApiEvent, const TString&)>;
using FProgressCallback = std::function<void(TDouble, const TString&)>;
using FLogCallback = std::function<void(ELogLevel, const TString&, const TString&)>;

/**************************************************************************
 * Interface: ITestMATEApi
 * Description: Main API interface for TestMATE functionality
 * Requirements: REQ-API-001 to REQ-API-030
 **************************************************************************/
class ITestMATEApi {
public:
    virtual ~ITestMATEApi() = default;

    //=========================================================================
    // Initialization
    //=========================================================================

    /**************************************************************************
     * Function Name: Initialize
     * Description: Initializes the API and core systems
     * Returns: Result indicating success or failure
     **************************************************************************/
    virtual CResult Initialize() = 0;

    /**************************************************************************
     * Function Name: Shutdown
     * Description: Shuts down the API and releases resources
     **************************************************************************/
    virtual CResult Shutdown() = 0;

    /**************************************************************************
     * Function Name: IsInitialized
     * Description: Checks if API is initialized
     **************************************************************************/
    [[nodiscard]] virtual bool IsInitialized() const = 0;

    /**************************************************************************
     * Function Name: GetVersion
     * Description: Returns API version string
     **************************************************************************/
    [[nodiscard]] virtual TString GetVersion() const = 0;

    //=========================================================================
    // Test Execution
    //=========================================================================

    /**************************************************************************
     * Function Name: LoadSequence
     * Description: Loads a test sequence file
     * Parameters:
     *   in_strPath - Path to sequence file
     **************************************************************************/
    virtual CResult LoadSequence(const TString& in_strPath) = 0;

    /**************************************************************************
     * Function Name: StartTest
     * Description: Starts test execution
     * Parameters:
     *   in_config - Test configuration
     **************************************************************************/
    virtual CResult StartTest(const STestConfiguration& in_config) = 0;

    /**************************************************************************
     * Function Name: StopTest
     * Description: Stops current test execution
     **************************************************************************/
    virtual CResult StopTest() = 0;

    /**************************************************************************
     * Function Name: PauseTest
     * Description: Pauses current test execution
     **************************************************************************/
    virtual CResult PauseTest() = 0;

    /**************************************************************************
     * Function Name: ResumeTest
     * Description: Resumes paused test execution
     **************************************************************************/
    virtual CResult ResumeTest() = 0;

    /**************************************************************************
     * Function Name: GetTestStatus
     * Description: Gets current test execution status
     **************************************************************************/
    [[nodiscard]] virtual STestStatus GetTestStatus() const = 0;

    //=========================================================================
    // Event Subscription
    //=========================================================================

    /**************************************************************************
     * Function Name: Subscribe
     * Description: Subscribes to an API event
     * Parameters:
     *   in_eEvent - Event to subscribe to
     *   in_callback - Callback function
     * Returns: Subscription ID for unsubscribing
     **************************************************************************/
    virtual TUInt64 Subscribe(EApiEvent in_eEvent, FApiEventCallback in_callback) = 0;

    /**************************************************************************
     * Function Name: Unsubscribe
     * Description: Unsubscribes from an event
     * Parameters:
     *   in_subscriptionId - ID returned from Subscribe
     **************************************************************************/
    virtual void Unsubscribe(TUInt64 in_subscriptionId) = 0;

    /**************************************************************************
     * Function Name: SetProgressCallback
     * Description: Sets callback for progress updates
     **************************************************************************/
    virtual void SetProgressCallback(FProgressCallback in_callback) = 0;

    /**************************************************************************
     * Function Name: SetLogCallback
     * Description: Sets callback for log messages
     **************************************************************************/
    virtual void SetLogCallback(FLogCallback in_callback) = 0;

    //=========================================================================
    // Configuration
    //=========================================================================

    /**************************************************************************
     * Function Name: SetVariable
     * Description: Sets a test variable
     **************************************************************************/
    virtual void SetVariable(const TString& in_strName, const TString& in_strValue) = 0;

    /**************************************************************************
     * Function Name: GetVariable
     * Description: Gets a test variable value
     **************************************************************************/
    [[nodiscard]] virtual std::optional<TString> GetVariable(const TString& in_strName) const = 0;

    /**************************************************************************
     * Function Name: ClearVariables
     * Description: Clears all test variables
     **************************************************************************/
    virtual void ClearVariables() = 0;

    //=========================================================================
    // Plugin Access
    //=========================================================================

    /**************************************************************************
     * Function Name: LoadPlugin
     * Description: Loads a plugin
     **************************************************************************/
    virtual CResult LoadPlugin(const TString& in_strPath) = 0;

    /**************************************************************************
     * Function Name: GetLoadedPlugins
     * Description: Returns list of loaded plugin IDs
     **************************************************************************/
    [[nodiscard]] virtual TVector<TString> GetLoadedPlugins() const = 0;
};

} // namespace TestMATE
