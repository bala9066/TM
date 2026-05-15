/**************************************************************************
 * File Name: InstrumentMeasureStep.h
 * Description: Test step for taking measurements from instruments
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"
#include "core/instruments/IInstrument.h"
#include "core/instruments/InstrumentManager.h"
#include <memory>

namespace TestMATE {

/**************************************************************************
 * Class: CInstrumentMeasureStep
 * Description: Performs measurements using test instruments
 *
 * Parameters:
 *   - instrument_id: ID of instrument to use (required)
 *   - command: SCPI/instrument command to execute (required)
 *   - timeout_ms: Command timeout in milliseconds (default: 5000)
 *   - result_name: Name for measurement result (default: "measurement")
 *   - unit: Measurement unit (optional)
 *   - parse_numeric: Parse response as numeric value (default: true)
 *
 * Supports:
 *   - SCPI query commands (e.g., "MEAS:VOLT:DC?")
 *   - Custom instrument commands
 *   - Automatic numeric parsing
 *   - Result storage for limit checking
 **************************************************************************/
class CInstrumentMeasureStep : public CTestStepBase {
public:
    explicit CInstrumentMeasureStep(const TString& in_strId = "MEAS-001",
                                   const TString& in_strName = "Instrument Measurement");

    ~CInstrumentMeasureStep() override = default;

    /**************************************************************************
     * Function Name: Execute
     * Description: Performs the measurement
     **************************************************************************/
    CResult Execute(SStepResult& out_result) override;

    /**************************************************************************
     * Function Name: SetInstrument
     * Description: Sets the instrument to use
     **************************************************************************/
    void SetInstrument(const TString& in_strInstrumentId);

    /**************************************************************************
     * Function Name: SetCommand
     * Description: Sets the measurement command
     **************************************************************************/
    void SetCommand(const TString& in_strCommand);

    /**************************************************************************
     * Function Name: SetTimeout
     * Description: Sets command timeout
     **************************************************************************/
    void SetTimeout(TInt64 in_timeoutMs) { m_timeoutMs = in_timeoutMs; }

    /**************************************************************************
     * Function Name: GetMeasuredValue
     * Description: Gets the last measured value (if numeric)
     **************************************************************************/
    [[nodiscard]] TDouble GetMeasuredValue() const { return m_measuredValue; }

    /**************************************************************************
     * Function Name: GetRawResponse
     * Description: Gets the raw instrument response
     **************************************************************************/
    [[nodiscard]] TString GetRawResponse() const { return m_strRawResponse; }

private:
    TString m_strInstrumentId;
    TString m_strCommand;
    TInt64 m_timeoutMs{5000};
    TString m_strResultName{"measurement"};
    TString m_strUnit;
    bool m_bParseNumeric{true};

    // Results
    TDouble m_measuredValue{0.0};
    TString m_strRawResponse;

    IInstrument* GetInstrument();
    bool ParseNumericResponse(const TString& in_strResponse, TDouble& out_value);
};

} // namespace TestMATE
