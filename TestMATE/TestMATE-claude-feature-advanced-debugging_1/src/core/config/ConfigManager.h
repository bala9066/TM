/**************************************************************************
 * File Name: ConfigManager.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Configuration management for TestMATE.
 * Requirements: REQ-CFG-001 to REQ-CFG-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <map>
#include <mutex>

namespace TestMATE {

/**************************************************************************
 * Struct: SSystemConfig
 * Description: System-wide configuration settings
 **************************************************************************/
struct SSystemConfig {
    TString dataDirectory;
    TString pluginDirectory;
    TString reportDirectory;
    TString logDirectory;
    ELogLevel logLevel{ELogLevel::kInfo};
    TUInt32 maxLogFileSizeMB{10};
    TUInt32 maxLogFiles{5};
    bool autoSaveResults{true};
    TInt64 defaultTimeoutMs{30000};
};

/**************************************************************************
 * Struct: SLimitDefinition
 * Description: Test limit specification
 **************************************************************************/
struct SLimitDefinition {
    TString testName;
    TString unit;
    std::optional<TDouble> lowLimit;
    std::optional<TDouble> highLimit;
    std::optional<TDouble> nominal;
    TUInt32 failBin{0};
    TString description;
};

/**************************************************************************
 * Class: CConfigManager
 * Description: Singleton for configuration management
 **************************************************************************/
class CConfigManager {
public:
    static CConfigManager& GetInstance();

    CConfigManager(const CConfigManager&) = delete;
    CConfigManager& operator=(const CConfigManager&) = delete;

    //=========================================================================
    // System Configuration
    //=========================================================================

    CResult LoadSystemConfig(const TString& in_strPath);
    CResult SaveSystemConfig(const TString& in_strPath);
    [[nodiscard]] const SSystemConfig& GetSystemConfig() const { return m_systemConfig; }
    void SetSystemConfig(const SSystemConfig& in_config) { m_systemConfig = in_config; }

    //=========================================================================
    // Generic Key-Value Settings
    //=========================================================================

    void SetString(const TString& in_strKey, const TString& in_strValue);
    void SetInt(const TString& in_strKey, TInt64 in_value);
    void SetFloat(const TString& in_strKey, TDouble in_value);
    void SetBool(const TString& in_strKey, bool in_value);

    [[nodiscard]] std::optional<TString> GetString(const TString& in_strKey) const;
    [[nodiscard]] std::optional<TInt64> GetInt(const TString& in_strKey) const;
    [[nodiscard]] std::optional<TDouble> GetFloat(const TString& in_strKey) const;
    [[nodiscard]] std::optional<bool> GetBool(const TString& in_strKey) const;

    [[nodiscard]] TString GetStringOrDefault(const TString& in_strKey, const TString& in_strDefault) const;

    //=========================================================================
    // Test Limits
    //=========================================================================

    CResult LoadLimitsFile(const TString& in_strPath);
    CResult SaveLimitsFile(const TString& in_strPath);
    void AddLimit(const SLimitDefinition& in_limit);
    [[nodiscard]] std::optional<SLimitDefinition> GetLimit(const TString& in_strTestName) const;
    [[nodiscard]] TVector<SLimitDefinition> GetAllLimits() const;
    void ClearLimits();

    //=========================================================================
    // Environment
    //=========================================================================

    [[nodiscard]] TString GetEnvironmentVariable(const TString& in_strName) const;
    [[nodiscard]] TString ExpandPath(const TString& in_strPath) const;

private:
    CConfigManager();
    ~CConfigManager() = default;

    CResult ParseConfigFile(const TString& in_strContent);
    CResult ParseLimitsFile(const TString& in_strContent);

    mutable std::mutex m_mutex;
    SSystemConfig m_systemConfig;
    std::map<TString, TString> m_mapSettings;
    std::map<TString, SLimitDefinition> m_mapLimits;
};

} // namespace TestMATE
