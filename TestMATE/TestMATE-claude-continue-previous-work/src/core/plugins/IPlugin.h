/**************************************************************************
 * File Name: IPlugin.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Plugin interface definitions for TestMATE extensibility.
 * Requirements: REQ-PLG-001 to REQ-PLG-020
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: EPluginType
 * Description: Types of plugins supported by TestMATE
 **************************************************************************/
enum class EPluginType {
    kUnknown,
    kTestStep,          // Custom test step implementations
    kInstrument,        // Hardware instrument drivers
    kDataSource,        // Data providers (files, databases)
    kReporter,          // Report generators
    kProtocol,          // Communication protocols
    kAnalyzer,          // Data analyzers
    kValidator,         // Custom validators
    kExtension          // Generic extensions
};

/**************************************************************************
 * Enum: EPluginState
 * Description: Plugin lifecycle states
 **************************************************************************/
enum class EPluginState {
    kUnloaded,
    kLoaded,
    kInitialized,
    kActive,
    kError
};

/**************************************************************************
 * Struct: SPluginInfo
 * Description: Plugin metadata
 **************************************************************************/
struct SPluginInfo {
    TString id;
    TString name;
    TString version;
    TString author;
    TString description;
    EPluginType type{EPluginType::kUnknown};
    TVector<TString> dependencies;

    // Version compatibility
    TString minHostVersion;
    TString maxHostVersion;
};

/**************************************************************************
 * Interface: IPlugin
 * Description: Base interface that all plugins must implement
 * Requirements: REQ-PLG-001 to REQ-PLG-010
 **************************************************************************/
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**************************************************************************
     * Function Name: GetInfo
     * Description: Returns plugin metadata
     * Returns: Plugin information structure
     **************************************************************************/
    [[nodiscard]] virtual SPluginInfo GetInfo() const = 0;

    /**************************************************************************
     * Function Name: Initialize
     * Description: Called when plugin is first loaded
     * Returns: Result indicating success or failure
     **************************************************************************/
    virtual CResult Initialize() = 0;

    /**************************************************************************
     * Function Name: Shutdown
     * Description: Called before plugin is unloaded
     * Returns: Result indicating success or failure
     **************************************************************************/
    virtual CResult Shutdown() = 0;

    /**************************************************************************
     * Function Name: GetState
     * Description: Returns current plugin state
     **************************************************************************/
    [[nodiscard]] virtual EPluginState GetState() const = 0;

    /**************************************************************************
     * Function Name: GetLastError
     * Description: Returns last error that occurred
     **************************************************************************/
    [[nodiscard]] virtual TString GetLastError() const = 0;
};

/**************************************************************************
 * Interface: ITestStepPlugin
 * Description: Interface for custom test step plugins
 * Requirements: REQ-PLG-011 to REQ-PLG-015
 **************************************************************************/
class ITestStepPlugin : public IPlugin {
public:
    /**************************************************************************
     * Function Name: Execute
     * Description: Executes the test step
     * Parameters:
     *   in_mapParams - Input parameters
     *   out_mapResults - Output results
     * Returns: Test verdict
     **************************************************************************/
    virtual ETestVerdict Execute(
        const std::map<TString, TString>& in_mapParams,
        std::map<TString, TString>& out_mapResults) = 0;

    /**************************************************************************
     * Function Name: GetParameterDefinitions
     * Description: Returns expected parameter definitions
     **************************************************************************/
    [[nodiscard]] virtual TVector<TString> GetParameterDefinitions() const = 0;
};

/**************************************************************************
 * Interface: IInstrumentPlugin
 * Description: Interface for hardware instrument plugins
 * Requirements: REQ-PLG-016 to REQ-PLG-020
 **************************************************************************/
class IInstrumentPlugin : public IPlugin {
public:
    virtual CResult Connect(const TString& in_strAddress) = 0;
    virtual CResult Disconnect() = 0;
    [[nodiscard]] virtual bool IsConnected() const = 0;

    virtual CResult Write(const TString& in_strCommand) = 0;
    virtual CResult Read(TString& out_strResponse, TInt64 in_timeoutMs) = 0;
    virtual CResult Query(const TString& in_strCommand,
                          TString& out_strResponse,
                          TInt64 in_timeoutMs) = 0;
};

/**************************************************************************
 * Interface: IReporterPlugin
 * Description: Interface for report generation plugins
 **************************************************************************/
class IReporterPlugin : public IPlugin {
public:
    virtual CResult BeginReport(const TString& in_strPath) = 0;
    virtual CResult EndReport() = 0;
    virtual CResult AddSection(const TString& in_strTitle) = 0;
    virtual CResult AddData(const TString& in_strKey, const TString& in_strValue) = 0;
    virtual CResult AddTable(const TString& in_strTitle,
                             const TVector<TVector<TString>>& in_data) = 0;
};

/**************************************************************************
 * Plugin Factory Function Types
 **************************************************************************/
using FPluginCreateFunc = IPlugin* (*)();
using FPluginDestroyFunc = void (*)(IPlugin*);

/**************************************************************************
 * Macros for Plugin Export
 **************************************************************************/
#define TESTMATE_PLUGIN_EXPORT extern "C"

#define TESTMATE_DECLARE_PLUGIN(ClassName) \
    TESTMATE_PLUGIN_EXPORT IPlugin* CreatePlugin() { \
        return new ClassName(); \
    } \
    TESTMATE_PLUGIN_EXPORT void DestroyPlugin(IPlugin* plugin) { \
        delete plugin; \
    } \
    TESTMATE_PLUGIN_EXPORT const char* GetPluginApiVersion() { \
        return "1.0"; \
    }

} // namespace TestMATE
