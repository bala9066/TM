/**************************************************************************
 * File Name: DataBinding.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Data binding system for dynamic value resolution.
 * Requirements: REQ-DATA-001 to REQ-DATA-020
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <map>
#include <regex>
#include <variant>

namespace TestMATE {

/**************************************************************************
 * Enum: EDataType
 * Description: Supported data types for binding
 **************************************************************************/
enum class EDataType {
    kString,
    kInt,
    kFloat,
    kBool,
    kArray,
    kObject
};

/**************************************************************************
 * Type: TDataValue
 * Description: Variant type for data values
 **************************************************************************/
using TDataValue = std::variant<TString, TInt64, TDouble, bool>;

/**************************************************************************
 * Class: CDataContext
 * Description: Context for variable resolution and data binding
 * Requirements: REQ-DATA-001 to REQ-DATA-010
 **************************************************************************/
class CDataContext {
public:
    CDataContext() = default;
    ~CDataContext() = default;

    //=========================================================================
    // Variable Management
    //=========================================================================

    void SetValue(const TString& in_strKey, const TDataValue& in_value);
    void SetString(const TString& in_strKey, const TString& in_strValue);
    void SetInt(const TString& in_strKey, TInt64 in_value);
    void SetFloat(const TString& in_strKey, TDouble in_value);
    void SetBool(const TString& in_strKey, bool in_value);

    [[nodiscard]] std::optional<TDataValue> GetValue(const TString& in_strKey) const;
    [[nodiscard]] std::optional<TString> GetString(const TString& in_strKey) const;
    [[nodiscard]] std::optional<TInt64> GetInt(const TString& in_strKey) const;
    [[nodiscard]] std::optional<TDouble> GetFloat(const TString& in_strKey) const;
    [[nodiscard]] std::optional<bool> GetBool(const TString& in_strKey) const;

    [[nodiscard]] bool HasKey(const TString& in_strKey) const;
    void Remove(const TString& in_strKey);
    void Clear();

    [[nodiscard]] TVector<TString> GetKeys() const;

    //=========================================================================
    // Expression Resolution
    //=========================================================================

    /**************************************************************************
     * Function Name: Resolve
     * Description: Resolves variable references in a string
     *              e.g., "${voltage}" -> "3.3"
     * Parameters:
     *   in_strExpression - String with variable references
     * Returns: Resolved string
     **************************************************************************/
    [[nodiscard]] TString Resolve(const TString& in_strExpression) const;

    /**************************************************************************
     * Function Name: ResolveAll
     * Description: Resolves all variables in a map of parameters
     **************************************************************************/
    [[nodiscard]] std::map<TString, TString> ResolveAll(
        const std::map<TString, TString>& in_mapParams) const;

    //=========================================================================
    // Parent Context
    //=========================================================================

    void SetParent(CDataContext* in_pParent) { m_pParent = in_pParent; }
    [[nodiscard]] CDataContext* GetParent() const { return m_pParent; }

private:
    [[nodiscard]] TString ValueToString(const TDataValue& in_value) const;

    std::map<TString, TDataValue> m_mapValues;
    CDataContext* m_pParent{nullptr};
};

/**************************************************************************
 * Class: CDataBinder
 * Description: Singleton for global data binding operations
 * Requirements: REQ-DATA-011 to REQ-DATA-020
 **************************************************************************/
class CDataBinder {
public:
    static CDataBinder& GetInstance();

    // Non-copyable
    CDataBinder(const CDataBinder&) = delete;
    CDataBinder& operator=(const CDataBinder&) = delete;

    //=========================================================================
    // Global Context
    //=========================================================================

    [[nodiscard]] CDataContext& GetGlobalContext() { return m_globalContext; }

    //=========================================================================
    // Custom Resolvers
    //=========================================================================

    using FResolver = std::function<TString(const TString&)>;

    /**************************************************************************
     * Function Name: RegisterResolver
     * Description: Registers a custom resolver for a prefix
     *              e.g., "env:" for environment variables
     **************************************************************************/
    void RegisterResolver(const TString& in_strPrefix, FResolver in_resolver);

    /**************************************************************************
     * Function Name: Resolve
     * Description: Resolves a reference using registered resolvers
     **************************************************************************/
    [[nodiscard]] TString Resolve(const TString& in_strRef) const;

    //=========================================================================
    // Built-in Resolvers
    //=========================================================================

    [[nodiscard]] static TString ResolveEnvVar(const TString& in_strName);
    [[nodiscard]] static TString ResolveTimestamp(const TString& in_strFormat);

private:
    CDataBinder();
    ~CDataBinder() = default;

    CDataContext m_globalContext;
    std::map<TString, FResolver> m_mapResolvers;
};

} // namespace TestMATE
