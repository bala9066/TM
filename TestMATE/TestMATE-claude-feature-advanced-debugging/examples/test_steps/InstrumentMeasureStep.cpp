/**************************************************************************
 * File Name: InstrumentMeasureStep.cpp
 * Description: Implementation of InstrumentMeasureStep
 **************************************************************************/

#include "InstrumentMeasureStep.h"
#include <sstream>
#include <iomanip>
#include <regex>

namespace TestMATE {

CInstrumentMeasureStep::CInstrumentMeasureStep(const TString& in_strId,
                                               const TString& in_strName)
    : CTestStepBase(in_strId, in_strName, EStepType::kMeasurement)
{
    // Add parameter definitions
    SStepParameter instParam;
    instParam.name = "instrument_id";
    instParam.type = "string";
    instParam.required = true;
    instParam.description = "ID of instrument to use for measurement";
    AddParameter(instParam);

    SStepParameter cmdParam;
    cmdParam.name = "command";
    cmdParam.type = "string";
    cmdParam.required = true;
    cmdParam.description = "SCPI or instrument command to execute";
    AddParameter(cmdParam);

    SStepParameter timeoutParam;
    timeoutParam.name = "timeout_ms";
    timeoutParam.type = "int64";
    timeoutParam.required = false;
    timeoutParam.description = "Command timeout in milliseconds";
    timeoutParam.defaultValue = "5000";
    AddParameter(timeoutParam);

    SStepParameter resultParam;
    resultParam.name = "result_name";
    resultParam.type = "string";
    resultParam.required = false;
    resultParam.description = "Name for measurement result";
    resultParam.defaultValue = "measurement";
    AddParameter(resultParam);

    SStepParameter unitParam;
    unitParam.name = "unit";
    unitParam.type = "string";
    unitParam.required = false;
    unitParam.description = "Measurement unit (e.g., 'V', 'A', 'Ω')";
    AddParameter(unitParam);

    SStepParameter parseParam;
    parseParam.name = "parse_numeric";
    parseParam.type = "bool";
    parseParam.required = false;
    parseParam.description = "Parse response as numeric value";
    parseParam.defaultValue = "true";
    AddParameter(parseParam);

    SetDescription("Perform instrument measurement");
}

CResult CInstrumentMeasureStep::Execute(SStepResult& out_result) {
    out_result.startTime = std::chrono::steady_clock::now();

    // Get required parameters
    auto instParam = GetParameter("instrument_id");
    if (!instParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing required 'instrument_id' parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing instrument_id");
    }
    m_strInstrumentId = instParam.value();

    auto cmdParam = GetParameter("command");
    if (!cmdParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing required 'command' parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing command");
    }
    m_strCommand = cmdParam.value();

    // Get optional parameters
    auto timeoutParam = GetParameter("timeout_ms");
    if (timeoutParam.has_value()) {
        try {
            m_timeoutMs = std::stoll(timeoutParam.value());
        } catch (...) {
            m_timeoutMs = 5000;  // Use default
        }
    }

    auto resultNameParam = GetParameter("result_name");
    if (resultNameParam.has_value()) {
        m_strResultName = resultNameParam.value();
    }

    auto unitParam = GetParameter("unit");
    if (unitParam.has_value()) {
        m_strUnit = unitParam.value();
    }

    auto parseParam = GetParameter("parse_numeric");
    if (parseParam.has_value()) {
        m_bParseNumeric = (parseParam.value() == "true");
    }

    // Get instrument from manager
    auto pInstrument = GetInstrument();
    if (!pInstrument) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Instrument not found: " + m_strInstrumentId;
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Instrument not found");
    }

    // Check if instrument is connected
    if (!pInstrument->IsConnected()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Instrument not connected: " + m_strInstrumentId;
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }

    // Execute measurement command
    CResult queryResult = pInstrument->Query(m_strCommand, m_strRawResponse, m_timeoutMs);
    if (!queryResult.IsSuccess()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Instrument query failed: " + queryResult.GetMessage();
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return queryResult;
    }

    // Parse response
    if (m_bParseNumeric) {
        if (ParseNumericResponse(m_strRawResponse, m_measuredValue)) {
            // Format numeric result
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << m_measuredValue;
            if (!m_strUnit.empty()) {
                oss << " " << m_strUnit;
            }
            out_result.measurements[m_strResultName] = oss.str();
            out_result.measurements[m_strResultName + "_raw"] = m_strRawResponse;
        } else {
            // Couldn't parse as numeric, store as string
            out_result.measurements[m_strResultName] = m_strRawResponse;
        }
    } else {
        // Store raw response
        out_result.measurements[m_strResultName] = m_strRawResponse;
    }

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "Measurement completed successfully";
    out_result.endTime = std::chrono::steady_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return TESTMATE_SUCCESS();
}

void CInstrumentMeasureStep::SetInstrument(const TString& in_strInstrumentId) {
    m_strInstrumentId = in_strInstrumentId;
    SetParameter("instrument_id", in_strInstrumentId);
}

void CInstrumentMeasureStep::SetCommand(const TString& in_strCommand) {
    m_strCommand = in_strCommand;
    SetParameter("command", in_strCommand);
}

TSharedPtr<IInstrument> CInstrumentMeasureStep::GetInstrument() {
    // Get instrument manager instance
    auto& instMgr = CInstrumentManager::GetInstance();

    // Get instrument by ID
    return instMgr.GetInstrument(m_strInstrumentId);
}

bool CInstrumentMeasureStep::ParseNumericResponse(const TString& in_strResponse, TDouble& out_value) {
    // Try to extract numeric value from response
    // Handles formats like:
    //   "+1.234567E+00"
    //   "1.23456"
    //   "1.23e-6"
    //   "Value: 123.456 V" (extracts 123.456)

    std::regex numericRegex(R"(([+-]?\d+\.?\d*[eE]?[+-]?\d*))");
    std::smatch match;

    if (std::regex_search(in_strResponse, match, numericRegex)) {
        try {
            out_value = std::stod(match[1].str());
            return true;
        } catch (...) {
            return false;
        }
    }

    return false;
}

} // namespace TestMATE
