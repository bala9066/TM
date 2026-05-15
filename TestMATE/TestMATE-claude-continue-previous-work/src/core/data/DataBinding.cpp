/**************************************************************************
 * File Name: DataBinding.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of data binding system
 * Requirements: REQ-DATA-001 to REQ-DATA-020
 **************************************************************************/

#include "DataBinding.h"
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace TestMATE {

//=============================================================================
// CDataContext Implementation
//=============================================================================

void CDataContext::SetValue(const TString& in_strKey, const TDataValue& in_value) {
    m_mapValues[in_strKey] = in_value;
}

void CDataContext::SetString(const TString& in_strKey, const TString& in_strValue) {
    m_mapValues[in_strKey] = in_strValue;
}

void CDataContext::SetInt(const TString& in_strKey, TInt64 in_value) {
    m_mapValues[in_strKey] = in_value;
}

void CDataContext::SetFloat(const TString& in_strKey, TDouble in_value) {
    m_mapValues[in_strKey] = in_value;
}

void CDataContext::SetBool(const TString& in_strKey, bool in_value) {
    m_mapValues[in_strKey] = in_value;
}

std::optional<TDataValue> CDataContext::GetValue(const TString& in_strKey) const {
    auto it = m_mapValues.find(in_strKey);
    if (it != m_mapValues.end()) {
        return it->second;
    }

    // Check parent context
    if (m_pParent) {
        return m_pParent->GetValue(in_strKey);
    }

    return std::nullopt;
}

std::optional<TString> CDataContext::GetString(const TString& in_strKey) const {
    auto value = GetValue(in_strKey);
    if (!value) {
        return std::nullopt;
    }

    return ValueToString(*value);
}

std::optional<TInt64> CDataContext::GetInt(const TString& in_strKey) const {
    auto value = GetValue(in_strKey);
    if (!value) {
        return std::nullopt;
    }

    if (auto* pInt = std::get_if<TInt64>(&*value)) {
        return *pInt;
    }
    if (auto* pFloat = std::get_if<TDouble>(&*value)) {
        return static_cast<TInt64>(*pFloat);
    }
    if (auto* pStr = std::get_if<TString>(&*value)) {
        try {
            return std::stoll(*pStr);
        } catch (...) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<TDouble> CDataContext::GetFloat(const TString& in_strKey) const {
    auto value = GetValue(in_strKey);
    if (!value) {
        return std::nullopt;
    }

    if (auto* pFloat = std::get_if<TDouble>(&*value)) {
        return *pFloat;
    }
    if (auto* pInt = std::get_if<TInt64>(&*value)) {
        return static_cast<TDouble>(*pInt);
    }
    if (auto* pStr = std::get_if<TString>(&*value)) {
        try {
            return std::stod(*pStr);
        } catch (...) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<bool> CDataContext::GetBool(const TString& in_strKey) const {
    auto value = GetValue(in_strKey);
    if (!value) {
        return std::nullopt;
    }

    if (auto* pBool = std::get_if<bool>(&*value)) {
        return *pBool;
    }
    if (auto* pStr = std::get_if<TString>(&*value)) {
        TString lower = *pStr;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "1" || lower == "yes";
    }
    if (auto* pInt = std::get_if<TInt64>(&*value)) {
        return *pInt != 0;
    }

    return std::nullopt;
}

bool CDataContext::HasKey(const TString& in_strKey) const {
    if (m_mapValues.count(in_strKey) > 0) {
        return true;
    }
    if (m_pParent) {
        return m_pParent->HasKey(in_strKey);
    }
    return false;
}

void CDataContext::Remove(const TString& in_strKey) {
    m_mapValues.erase(in_strKey);
}

void CDataContext::Clear() {
    m_mapValues.clear();
}

TVector<TString> CDataContext::GetKeys() const {
    TVector<TString> keys;
    for (const auto& [key, value] : m_mapValues) {
        keys.push_back(key);
    }
    return keys;
}

TString CDataContext::Resolve(const TString& in_strExpression) const {
    TString result = in_strExpression;

    // Match ${varname} pattern
    std::regex varPattern(R"(\$\{([^}]+)\})");
    std::smatch match;

    TString working = result;
    while (std::regex_search(working, match, varPattern)) {
        TString varName = match[1].str();
        TString replacement;

        auto value = GetValue(varName);
        if (value) {
            replacement = ValueToString(*value);
        } else {
            // Try global resolver
            replacement = CDataBinder::GetInstance().Resolve(varName);
        }

        // Replace in result
        size_t pos = result.find(match[0].str());
        if (pos != TString::npos) {
            result.replace(pos, match[0].length(), replacement);
        }

        working = match.suffix().str();
    }

    return result;
}

std::map<TString, TString> CDataContext::ResolveAll(
    const std::map<TString, TString>& in_mapParams) const {

    std::map<TString, TString> resolved;
    for (const auto& [key, value] : in_mapParams) {
        resolved[key] = Resolve(value);
    }
    return resolved;
}

TString CDataContext::ValueToString(const TDataValue& in_value) const {
    return std::visit([](auto&& arg) -> TString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, TString>) {
            return arg;
        } else if constexpr (std::is_same_v<T, TInt64>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, TDouble>) {
            std::ostringstream oss;
            oss << std::setprecision(10) << arg;
            return oss.str();
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        }
        return "";
    }, in_value);
}

//=============================================================================
// CDataBinder Implementation
//=============================================================================

CDataBinder& CDataBinder::GetInstance() {
    static CDataBinder instance;
    return instance;
}

CDataBinder::CDataBinder() {
    // Register built-in resolvers
    RegisterResolver("env:", [](const TString& name) {
        return ResolveEnvVar(name);
    });

    RegisterResolver("time:", [](const TString& format) {
        return ResolveTimestamp(format);
    });
}

void CDataBinder::RegisterResolver(const TString& in_strPrefix, FResolver in_resolver) {
    m_mapResolvers[in_strPrefix] = std::move(in_resolver);
}

TString CDataBinder::Resolve(const TString& in_strRef) const {
    // Check for prefixed resolvers
    for (const auto& [prefix, resolver] : m_mapResolvers) {
        if (in_strRef.find(prefix) == 0) {
            return resolver(in_strRef.substr(prefix.length()));
        }
    }

    // Check global context
    auto value = m_globalContext.GetString(in_strRef);
    if (value) {
        return *value;
    }

    // Return original if not resolved
    return "${" + in_strRef + "}";
}

TString CDataBinder::ResolveEnvVar(const TString& in_strName) {
    const char* value = std::getenv(in_strName.c_str());
    return value ? TString(value) : "";
}

TString CDataBinder::ResolveTimestamp(const TString& in_strFormat) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time),
                         in_strFormat.empty() ? "%Y-%m-%d %H:%M:%S" : in_strFormat.c_str());
    return oss.str();
}

} // namespace TestMATE
