/**************************************************************************
 * File Name: StringUtils.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: String utility functions for TestMATE application.
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace TestMATE {
namespace StringUtils {

/**************************************************************************
 * Function Name: Trim
 * Description: Removes leading and trailing whitespace from string
 * Parameters:
 *   in_str - String to trim
 * Returns: Trimmed string
 **************************************************************************/
[[nodiscard]] inline TString Trim(const TString& in_str) {
    auto start = std::find_if_not(in_str.begin(), in_str.end(),
                                   [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(in_str.rbegin(), in_str.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end) ? TString(start, end) : TString();
}

/**************************************************************************
 * Function Name: TrimLeft
 * Description: Removes leading whitespace from string
 * Parameters:
 *   in_str - String to trim
 * Returns: Left-trimmed string
 **************************************************************************/
[[nodiscard]] inline TString TrimLeft(const TString& in_str) {
    auto start = std::find_if_not(in_str.begin(), in_str.end(),
                                   [](unsigned char c) { return std::isspace(c); });
    return TString(start, in_str.end());
}

/**************************************************************************
 * Function Name: TrimRight
 * Description: Removes trailing whitespace from string
 * Parameters:
 *   in_str - String to trim
 * Returns: Right-trimmed string
 **************************************************************************/
[[nodiscard]] inline TString TrimRight(const TString& in_str) {
    auto end = std::find_if_not(in_str.rbegin(), in_str.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    return TString(in_str.begin(), end);
}

/**************************************************************************
 * Function Name: ToLower
 * Description: Converts string to lowercase
 * Parameters:
 *   in_str - String to convert
 * Returns: Lowercase string
 **************************************************************************/
[[nodiscard]] inline TString ToLower(const TString& in_str) {
    TString result = in_str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**************************************************************************
 * Function Name: ToUpper
 * Description: Converts string to uppercase
 * Parameters:
 *   in_str - String to convert
 * Returns: Uppercase string
 **************************************************************************/
[[nodiscard]] inline TString ToUpper(const TString& in_str) {
    TString result = in_str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

/**************************************************************************
 * Function Name: Split
 * Description: Splits string by delimiter
 * Parameters:
 *   in_str - String to split
 *   in_cDelimiter - Delimiter character
 * Returns: Vector of substrings
 **************************************************************************/
[[nodiscard]] inline TVector<TString> Split(const TString& in_str, char in_cDelimiter) {
    TVector<TString> vecTokens;
    std::stringstream ss(in_str);
    TString strToken;

    while (std::getline(ss, strToken, in_cDelimiter)) {
        vecTokens.push_back(strToken);
    }

    // Handle empty string case: should return vector with one empty element
    if (vecTokens.empty()) {
        vecTokens.push_back("");
    }

    return vecTokens;
}

/**************************************************************************
 * Function Name: Join
 * Description: Joins strings with delimiter
 * Parameters:
 *   in_vecStrings - Strings to join
 *   in_strDelimiter - Delimiter between strings
 * Returns: Joined string
 **************************************************************************/
[[nodiscard]] inline TString Join(const TVector<TString>& in_vecStrings,
                                   const TString& in_strDelimiter) {
    if (in_vecStrings.empty()) {
        return TString();
    }

    std::ostringstream oss;
    oss << in_vecStrings[0];

    for (size_t i = 1; i < in_vecStrings.size(); ++i) {
        oss << in_strDelimiter << in_vecStrings[i];
    }

    return oss.str();
}

/**************************************************************************
 * Function Name: StartsWith
 * Description: Checks if string starts with prefix
 * Parameters:
 *   in_str - String to check
 *   in_strPrefix - Prefix to look for
 * Returns: true if string starts with prefix
 **************************************************************************/
[[nodiscard]] inline bool StartsWith(const TString& in_str, const TString& in_strPrefix) {
    if (in_strPrefix.size() > in_str.size()) {
        return false;
    }
    return in_str.compare(0, in_strPrefix.size(), in_strPrefix) == 0;
}

/**************************************************************************
 * Function Name: EndsWith
 * Description: Checks if string ends with suffix
 * Parameters:
 *   in_str - String to check
 *   in_strSuffix - Suffix to look for
 * Returns: true if string ends with suffix
 **************************************************************************/
[[nodiscard]] inline bool EndsWith(const TString& in_str, const TString& in_strSuffix) {
    if (in_strSuffix.size() > in_str.size()) {
        return false;
    }
    return in_str.compare(in_str.size() - in_strSuffix.size(),
                          in_strSuffix.size(), in_strSuffix) == 0;
}

/**************************************************************************
 * Function Name: Contains
 * Description: Checks if string contains substring
 * Parameters:
 *   in_str - String to search in
 *   in_strSubstr - Substring to find
 * Returns: true if substring found
 **************************************************************************/
[[nodiscard]] inline bool Contains(const TString& in_str, const TString& in_strSubstr) {
    return in_str.find(in_strSubstr) != TString::npos;
}

/**************************************************************************
 * Function Name: Replace
 * Description: Replaces all occurrences of substring
 * Parameters:
 *   in_str - Source string
 *   in_strFrom - String to replace
 *   in_strTo - Replacement string
 * Returns: String with replacements
 **************************************************************************/
[[nodiscard]] inline TString Replace(const TString& in_str,
                                      const TString& in_strFrom,
                                      const TString& in_strTo) {
    TString result = in_str;
    size_t pos = 0;

    while ((pos = result.find(in_strFrom, pos)) != TString::npos) {
        result.replace(pos, in_strFrom.length(), in_strTo);
        pos += in_strTo.length();
    }

    return result;
}

/**************************************************************************
 * Function Name: IsEmpty
 * Description: Checks if string is empty or whitespace only
 * Parameters:
 *   in_str - String to check
 * Returns: true if empty or whitespace only
 **************************************************************************/
[[nodiscard]] inline bool IsEmpty(const TString& in_str) {
    return Trim(in_str).empty();
}

// Format implementation helpers - must be declared before Format()
inline void FormatImpl(std::ostringstream& out_oss, const TString& in_strFormat) {
    out_oss << in_strFormat;
}

template<typename T, typename... Rest>
void FormatImpl(std::ostringstream& out_oss, const TString& in_strFormat,
                T&& value, Rest&&... rest) {
    size_t pos = in_strFormat.find("{}");
    if (pos != TString::npos) {
        out_oss << in_strFormat.substr(0, pos) << std::forward<T>(value);
        FormatImpl(out_oss, in_strFormat.substr(pos + 2), std::forward<Rest>(rest)...);
    } else {
        out_oss << in_strFormat;
    }
}

/**************************************************************************
 * Function Name: Format
 * Description: Simple printf-style formatting
 * Parameters:
 *   in_strFormat - Format string with {} placeholders
 *   args - Arguments to substitute
 * Returns: Formatted string
 **************************************************************************/
template<typename... Args>
[[nodiscard]] TString Format(const TString& in_strFormat, Args&&... args) {
    std::ostringstream oss;
    FormatImpl(oss, in_strFormat, std::forward<Args>(args)...);
    return oss.str();
}

} // namespace StringUtils
} // namespace TestMATE
