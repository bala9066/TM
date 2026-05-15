/**************************************************************************
 * File Name: ConfigManager.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Configuration manager implementation
 **************************************************************************/

#include "ConfigManager.h"
#include "utils/LogManager.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cstdlib>

namespace TestMATE {

CConfigManager& CConfigManager::GetInstance() {
    static CConfigManager instance;
    return instance;
}

CConfigManager::CConfigManager() {
    // Set defaults
    m_systemConfig.dataDirectory = "./data";
    m_systemConfig.pluginDirectory = "./plugins";
    m_systemConfig.reportDirectory = "./reports";
    m_systemConfig.logDirectory = "./logs";
}

CResult CConfigManager::LoadSystemConfig(const TString& in_strPath) {
    std::ifstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "Config not found: " + in_strPath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseConfigFile(buffer.str());
}

CResult CConfigManager::SaveSystemConfig(const TString& in_strPath) {
    std::ofstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Cannot write: " + in_strPath);
    }

    file << "# TestMATE Configuration\n\n";
    file << "[directories]\n";
    file << "data=" << m_systemConfig.dataDirectory << "\n";
    file << "plugins=" << m_systemConfig.pluginDirectory << "\n";
    file << "reports=" << m_systemConfig.reportDirectory << "\n";
    file << "logs=" << m_systemConfig.logDirectory << "\n\n";

    file << "[logging]\n";
    file << "level=" << static_cast<int>(m_systemConfig.logLevel) << "\n";
    file << "max_size_mb=" << m_systemConfig.maxLogFileSizeMB << "\n";
    file << "max_files=" << m_systemConfig.maxLogFiles << "\n\n";

    file << "[execution]\n";
    file << "auto_save=" << (m_systemConfig.autoSaveResults ? "true" : "false") << "\n";
    file << "default_timeout_ms=" << m_systemConfig.defaultTimeoutMs << "\n";

    return TESTMATE_SUCCESS();
}

CResult CConfigManager::ParseConfigFile(const TString& in_strContent) {
    std::istringstream stream(in_strContent);
    TString line;
    TString currentSection;

    while (std::getline(stream, line)) {
        // Trim
        while (!line.empty() && std::isspace(line.front())) line.erase(0, 1);
        while (!line.empty() && std::isspace(line.back())) line.pop_back();

        if (line.empty() || line[0] == '#') continue;

        // Section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // Key=value
        size_t eqPos = line.find('=');
        if (eqPos != TString::npos) {
            TString key = line.substr(0, eqPos);
            TString value = line.substr(eqPos + 1);

            // Trim
            while (!key.empty() && std::isspace(key.back())) key.pop_back();
            while (!value.empty() && std::isspace(value.front())) value.erase(0, 1);

            TString fullKey = currentSection.empty() ? key : currentSection + "." + key;
            m_mapSettings[fullKey] = value;

            // Apply to system config
            if (currentSection == "directories") {
                if (key == "data") m_systemConfig.dataDirectory = value;
                else if (key == "plugins") m_systemConfig.pluginDirectory = value;
                else if (key == "reports") m_systemConfig.reportDirectory = value;
                else if (key == "logs") m_systemConfig.logDirectory = value;
            } else if (currentSection == "logging") {
                if (key == "level") m_systemConfig.logLevel = static_cast<ELogLevel>(std::stoi(value));
                else if (key == "max_size_mb") m_systemConfig.maxLogFileSizeMB = std::stoul(value);
                else if (key == "max_files") m_systemConfig.maxLogFiles = std::stoul(value);
            } else if (currentSection == "execution") {
                if (key == "auto_save") m_systemConfig.autoSaveResults = (value == "true");
                else if (key == "default_timeout_ms") m_systemConfig.defaultTimeoutMs = std::stoll(value);
            }
        }
    }

    return TESTMATE_SUCCESS();
}

void CConfigManager::SetString(const TString& in_strKey, const TString& in_strValue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapSettings[in_strKey] = in_strValue;
}

void CConfigManager::SetInt(const TString& in_strKey, TInt64 in_value) {
    SetString(in_strKey, std::to_string(in_value));
}

void CConfigManager::SetFloat(const TString& in_strKey, TDouble in_value) {
    SetString(in_strKey, std::to_string(in_value));
}

void CConfigManager::SetBool(const TString& in_strKey, bool in_value) {
    SetString(in_strKey, in_value ? "true" : "false");
}

std::optional<TString> CConfigManager::GetString(const TString& in_strKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_mapSettings.find(in_strKey);
    if (it != m_mapSettings.end()) return it->second;
    return std::nullopt;
}

std::optional<TInt64> CConfigManager::GetInt(const TString& in_strKey) const {
    auto str = GetString(in_strKey);
    if (str) {
        try {
            return std::stoll(*str);
        } catch (const std::exception& e) {
            CLogManager::GetInstance().LogWarning("ConfigManager",
                "Failed to convert config '{}' value '{}' to integer: {}",
                in_strKey, *str, e.what());
        } catch (...) {
            CLogManager::GetInstance().LogWarning("ConfigManager",
                "Unknown exception converting config '{}' value '{}' to integer",
                in_strKey, *str);
        }
    }
    return std::nullopt;
}

std::optional<TDouble> CConfigManager::GetFloat(const TString& in_strKey) const {
    auto str = GetString(in_strKey);
    if (str) {
        try {
            return std::stod(*str);
        } catch (const std::exception& e) {
            CLogManager::GetInstance().LogWarning("ConfigManager",
                "Failed to convert config '{}' value '{}' to double: {}",
                in_strKey, *str, e.what());
        } catch (...) {
            CLogManager::GetInstance().LogWarning("ConfigManager",
                "Unknown exception converting config '{}' value '{}' to double",
                in_strKey, *str);
        }
    }
    return std::nullopt;
}

std::optional<bool> CConfigManager::GetBool(const TString& in_strKey) const {
    auto str = GetString(in_strKey);
    if (str) return (*str == "true" || *str == "1");
    return std::nullopt;
}

TString CConfigManager::GetStringOrDefault(const TString& in_strKey, const TString& in_strDefault) const {
    auto val = GetString(in_strKey);
    return val ? *val : in_strDefault;
}

CResult CConfigManager::LoadLimitsFile(const TString& in_strPath) {
    std::ifstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "Limits file not found: " + in_strPath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseLimitsFile(buffer.str());
}

CResult CConfigManager::ParseLimitsFile(const TString& in_strContent) {
    std::istringstream stream(in_strContent);
    TString line;
    bool headerSkipped = false;

    while (std::getline(stream, line)) {
        while (!line.empty() && std::isspace(line.front())) line.erase(0, 1);
        if (line.empty() || line[0] == '#') continue;

        if (!headerSkipped) {
            headerSkipped = true;
            continue;  // Skip CSV header
        }

        // Parse CSV: TestName,Unit,LowLimit,HighLimit,Nominal,FailBin,Description
        std::vector<TString> fields;
        std::stringstream ss(line);
        TString field;
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        if (fields.size() >= 5) {
            SLimitDefinition limit;
            limit.testName = fields[0];
            limit.unit = fields[1];
            if (!fields[2].empty()) limit.lowLimit = std::stod(fields[2]);
            if (!fields[3].empty()) limit.highLimit = std::stod(fields[3]);
            if (!fields[4].empty()) limit.nominal = std::stod(fields[4]);
            if (fields.size() > 5 && !fields[5].empty()) limit.failBin = std::stoul(fields[5]);
            if (fields.size() > 6) limit.description = fields[6];

            AddLimit(limit);
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CConfigManager::SaveLimitsFile(const TString& in_strPath) {
    std::ofstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Cannot write: " + in_strPath);
    }

    file << "TestName,Unit,LowLimit,HighLimit,Nominal,FailBin,Description\n";

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [name, limit] : m_mapLimits) {
        file << limit.testName << ","
             << limit.unit << ","
             << (limit.lowLimit ? std::to_string(*limit.lowLimit) : "") << ","
             << (limit.highLimit ? std::to_string(*limit.highLimit) : "") << ","
             << (limit.nominal ? std::to_string(*limit.nominal) : "") << ","
             << limit.failBin << ","
             << limit.description << "\n";
    }

    return TESTMATE_SUCCESS();
}

void CConfigManager::AddLimit(const SLimitDefinition& in_limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapLimits[in_limit.testName] = in_limit;
}

std::optional<SLimitDefinition> CConfigManager::GetLimit(const TString& in_strTestName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_mapLimits.find(in_strTestName);
    if (it != m_mapLimits.end()) return it->second;
    return std::nullopt;
}

TVector<SLimitDefinition> CConfigManager::GetAllLimits() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    TVector<SLimitDefinition> limits;
    for (const auto& [name, limit] : m_mapLimits) {
        limits.push_back(limit);
    }
    return limits;
}

void CConfigManager::ClearLimits() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapLimits.clear();
}

TString CConfigManager::GetEnvironmentVariable(const TString& in_strName) const {
    const char* val = std::getenv(in_strName.c_str());
    return val ? TString(val) : "";
}

TString CConfigManager::ExpandPath(const TString& in_strPath) const {
    TString result = in_strPath;

    // Expand $VAR and ${VAR}
    std::regex envRegex(R"(\$\{?(\w+)\}?)");
    std::smatch match;

    while (std::regex_search(result, match, envRegex)) {
        TString varName = match[1].str();
        TString varValue = GetEnvironmentVariable(varName);
        result = result.replace(match.position(), match.length(), varValue);
    }

    return result;
}

} // namespace TestMATE
