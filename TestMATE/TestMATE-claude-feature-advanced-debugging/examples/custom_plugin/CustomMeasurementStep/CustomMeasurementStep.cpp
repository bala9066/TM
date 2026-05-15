/**************************************************************************
 * File Name: CustomMeasurementStep.cpp
 * Description: Implementation of CustomMeasurementStep plugin
 **************************************************************************/

#include "CustomMeasurementStep.h"
#include <sstream>
#include <cmath>

using namespace TestMATE;

namespace TestMATEPlugins {

CCustomMeasurementStep::CCustomMeasurementStep()
    : m_eState(EPluginState::kUnloaded)
{
    m_info.id = "custom-measurement-step";
    m_info.name = "Custom Measurement Step Plugin";
    m_info.version = "1.0.0";
    m_info.author = "TestMATE Example";
    m_info.description = "Example test step plugin for custom measurement calculations";
    m_info.type = EPluginType::kTestStep;
    m_info.minHostVersion = "1.0.0";
}

SPluginInfo CCustomMeasurementStep::GetInfo() const {
    return m_info;
}

CResult CCustomMeasurementStep::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_eState != EPluginState::kUnloaded) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Already initialized");
    }
    m_eState = EPluginState::kInitialized;
    return TESTMATE_SUCCESS();
}

CResult CCustomMeasurementStep::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eState = EPluginState::kUnloaded;
    return TESTMATE_SUCCESS();
}

EPluginState CCustomMeasurementStep::GetState() const {
    return m_eState;
}

TString CCustomMeasurementStep::GetLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_strLastError;
}

ETestVerdict CCustomMeasurementStep::Execute(
    const std::map<TString, TString>& in_mapParams,
    std::map<TString, TString>& out_mapResults)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Get parameters
    auto itFormula = in_mapParams.find("formula");
    auto itLimitMin = in_mapParams.find("limit_min");
    auto itLimitMax = in_mapParams.find("limit_max");

    if (itFormula == in_mapParams.end()) {
        SetError("Missing 'formula' parameter");
        return ETestVerdict::kError;
    }

    // Perform calculation
    TDouble result = PerformCalculation(itFormula->second, in_mapParams);

    // Store result
    std::ostringstream oss;
    oss << result;
    out_mapResults["measured_value"] = oss.str();

    // Check limits
    if (itLimitMin != in_mapParams.end() && itLimitMax != in_mapParams.end()) {
        TDouble limitMin = std::stod(itLimitMin->second);
        TDouble limitMax = std::stod(itLimitMax->second);

        if (result < limitMin || result > limitMax) {
            out_mapResults["verdict"] = "FAIL";
            out_mapResults["reason"] = "Out of limits";
            return ETestVerdict::kFail;
        }
    }

    out_mapResults["verdict"] = "PASS";
    return ETestVerdict::kPass;
}

TVector<TString> CCustomMeasurementStep::GetParameterDefinitions() const {
    return {
        "formula: Mathematical expression to calculate (e.g., 'voltage * current')",
        "voltage: Input voltage value",
        "current: Input current value",
        "limit_min: Minimum acceptable value (optional)",
        "limit_max: Maximum acceptable value (optional)"
    };
}

void CCustomMeasurementStep::SetError(const TString& in_strError) {
    m_strLastError = in_strError;
}

TDouble CCustomMeasurementStep::PerformCalculation(const TString& in_strFormula,
                                                  const std::map<TString, TString>& in_mapParams)
{
    // Simple example: parse "voltage * current" to calculate power
    if (in_strFormula.find("voltage * current") != TString::npos ||
        in_strFormula.find("V * I") != TString::npos) {

        auto itV = in_mapParams.find("voltage");
        auto itI = in_mapParams.find("current");

        if (itV != in_mapParams.end() && itI != in_mapParams.end()) {
            TDouble voltage = std::stod(itV->second);
            TDouble current = std::stod(itI->second);
            return voltage * current;  // Power = V * I
        }
    }

    // For demonstration, return 0.0 if formula not recognized
    return 0.0;
}

} // namespace TestMATEPlugins
