/**************************************************************************
 * File Name: LimitCheckStep.cpp
 * Description: Implementation of LimitCheckStep
 **************************************************************************/

#include "LimitCheckStep.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace TestMATE {

CLimitCheckStep::CLimitCheckStep(const TString& in_strId,
                                 const TString& in_strName)
    : CTestStepBase(in_strId, in_strName, EStepType::kValidation)
{
    // Add parameter definitions
    SStepParameter valueParam;
    valueParam.name = "value";
    valueParam.type = "double";
    valueParam.required = true;
    valueParam.description = "Value to check against limits";
    AddParameter(valueParam);

    SStepParameter minParam;
    minParam.name = "limit_min";
    minParam.type = "double";
    minParam.required = false;
    minParam.description = "Minimum acceptable value";
    AddParameter(minParam);

    SStepParameter maxParam;
    maxParam.name = "limit_max";
    maxParam.type = "double";
    maxParam.required = false;
    maxParam.description = "Maximum acceptable value";
    AddParameter(maxParam);

    SStepParameter toleranceParam;
    toleranceParam.name = "tolerance";
    toleranceParam.type = "double";
    toleranceParam.required = false;
    toleranceParam.description = "Tolerance for equality checks";
    toleranceParam.defaultValue = "0.001";
    AddParameter(toleranceParam);

    SStepParameter unitParam;
    unitParam.name = "unit";
    unitParam.type = "string";
    unitParam.required = false;
    unitParam.description = "Measurement unit (e.g., 'V', 'A', 'Ω')";
    AddParameter(unitParam);

    SetDescription("Validate measurement against specified limits");
}

CResult CLimitCheckStep::Execute(SStepResult& out_result) {
    out_result.startTime = std::chrono::system_clock::now();

    // Get value parameter (required)
    auto valueParam = GetParameter("value");
    if (!valueParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing required 'value' parameter";
        out_result.endTime = std::chrono::system_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing value parameter");
    }

    try {
        m_value = std::stod(valueParam.value());
    } catch (...) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Invalid value parameter format";
        out_result.endTime = std::chrono::system_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid value");
    }

    // Get optional limit parameters
    auto minParam = GetParameter("limit_min");
    if (minParam.has_value()) {
        try {
            m_limitMin = std::stod(minParam.value());
            if (m_eLimitType == ELimitType::kRange || m_eLimitType == ELimitType::kMinOnly) {
                // Use range or min-only checking
            }
        } catch (...) {
            out_result.verdict = ETestVerdict::kError;
            out_result.message = "Invalid limit_min parameter";
            out_result.endTime = std::chrono::system_clock::now();
            out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                out_result.endTime - out_result.startTime).count();
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid limit_min");
        }
    }

    auto maxParam = GetParameter("limit_max");
    if (maxParam.has_value()) {
        try {
            m_limitMax = std::stod(maxParam.value());
        } catch (...) {
            out_result.verdict = ETestVerdict::kError;
            out_result.message = "Invalid limit_max parameter";
            out_result.endTime = std::chrono::system_clock::now();
            out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                out_result.endTime - out_result.startTime).count();
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid limit_max");
        }
    }

    auto toleranceParam = GetParameter("tolerance");
    if (toleranceParam.has_value()) {
        try {
            m_tolerance = std::stod(toleranceParam.value());
        } catch (...) {
            // Use default tolerance
        }
    }

    auto unitParam = GetParameter("unit");
    if (unitParam.has_value()) {
        m_strUnit = unitParam.value();
    }

    // Determine limit type if not explicitly set
    if (minParam.has_value() && maxParam.has_value()) {
        m_eLimitType = ELimitType::kRange;
    } else if (minParam.has_value()) {
        m_eLimitType = ELimitType::kMinOnly;
    } else if (maxParam.has_value()) {
        m_eLimitType = ELimitType::kMaxOnly;
    }

    // Perform limit check
    TString message;
    bool passed = CheckLimit(m_value, message);

    out_result.verdict = passed ? ETestVerdict::kPass : ETestVerdict::kFail;
    out_result.message = message;
    out_result.measurements["value"] = FormatValue(m_value);
    out_result.measurements["limit_min"] = FormatValue(m_limitMin);
    out_result.measurements["limit_max"] = FormatValue(m_limitMax);

    out_result.endTime = std::chrono::system_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return TESTMATE_SUCCESS();
}

void CLimitCheckStep::SetLimits(TDouble in_min, TDouble in_max) {
    m_limitMin = in_min;
    m_limitMax = in_max;
    m_eLimitType = ELimitType::kRange;

    std::ostringstream ossMin, ossMax;
    ossMin << in_min;
    ossMax << in_max;
    SetParameter("limit_min", ossMin.str());
    SetParameter("limit_max", ossMax.str());
}

void CLimitCheckStep::SetMinLimit(TDouble in_min) {
    m_limitMin = in_min;
    m_eLimitType = ELimitType::kMinOnly;

    std::ostringstream oss;
    oss << in_min;
    SetParameter("limit_min", oss.str());
}

void CLimitCheckStep::SetMaxLimit(TDouble in_max) {
    m_limitMax = in_max;
    m_eLimitType = ELimitType::kMaxOnly;

    std::ostringstream oss;
    oss << in_max;
    SetParameter("limit_max", oss.str());
}

void CLimitCheckStep::SetValue(TDouble in_value) {
    m_value = in_value;

    std::ostringstream oss;
    oss << in_value;
    SetParameter("value", oss.str());
}

bool CLimitCheckStep::CheckLimit(TDouble in_value, TString& out_strMessage) {
    std::ostringstream oss;

    switch (m_eLimitType) {
        case ELimitType::kRange:
            if (in_value >= m_limitMin && in_value <= m_limitMax) {
                oss << "PASS: " << FormatValue(in_value) << " within range ["
                    << FormatValue(m_limitMin) << ", " << FormatValue(m_limitMax) << "]";
                out_strMessage = oss.str();
                return true;
            } else {
                oss << "FAIL: " << FormatValue(in_value) << " outside range ["
                    << FormatValue(m_limitMin) << ", " << FormatValue(m_limitMax) << "]";
                out_strMessage = oss.str();
                return false;
            }

        case ELimitType::kMinOnly:
            if (in_value >= m_limitMin) {
                oss << "PASS: " << FormatValue(in_value) << " >= " << FormatValue(m_limitMin);
                out_strMessage = oss.str();
                return true;
            } else {
                oss << "FAIL: " << FormatValue(in_value) << " < " << FormatValue(m_limitMin);
                out_strMessage = oss.str();
                return false;
            }

        case ELimitType::kMaxOnly:
            if (in_value <= m_limitMax) {
                oss << "PASS: " << FormatValue(in_value) << " <= " << FormatValue(m_limitMax);
                out_strMessage = oss.str();
                return true;
            } else {
                oss << "FAIL: " << FormatValue(in_value) << " > " << FormatValue(m_limitMax);
                out_strMessage = oss.str();
                return false;
            }

        case ELimitType::kEquals:
            if (std::abs(in_value - m_limitMin) <= m_tolerance) {
                oss << "PASS: " << FormatValue(in_value) << " ≈ " << FormatValue(m_limitMin)
                    << " (tolerance: " << m_tolerance << ")";
                out_strMessage = oss.str();
                return true;
            } else {
                oss << "FAIL: " << FormatValue(in_value) << " ≠ " << FormatValue(m_limitMin)
                    << " (tolerance: " << m_tolerance << ")";
                out_strMessage = oss.str();
                return false;
            }

        case ELimitType::kNotEquals:
            if (std::abs(in_value - m_limitMin) > m_tolerance) {
                oss << "PASS: " << FormatValue(in_value) << " ≠ " << FormatValue(m_limitMin);
                out_strMessage = oss.str();
                return true;
            } else {
                oss << "FAIL: " << FormatValue(in_value) << " ≈ " << FormatValue(m_limitMin)
                    << " (should be different)";
                out_strMessage = oss.str();
                return false;
            }

        default:
            out_strMessage = "ERROR: Unknown limit type";
            return false;
    }
}

TString CLimitCheckStep::FormatValue(TDouble in_value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << in_value;
    if (!m_strUnit.empty()) {
        oss << " " << m_strUnit;
    }
    return oss.str();
}

} // namespace TestMATE
