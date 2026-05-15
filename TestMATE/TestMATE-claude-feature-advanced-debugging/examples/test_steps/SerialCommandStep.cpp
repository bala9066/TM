/**************************************************************************
 * File Name: SerialCommandStep.cpp
 * Description: Implementation of SerialCommandStep
 **************************************************************************/

#include "SerialCommandStep.h"

namespace TestMATE {

CSerialCommandStep::CSerialCommandStep(const TString& in_strId,
                                       const TString& in_strName)
    : CTestStepBase(in_strId, in_strName, EStepType::kAction)
{
    SStepParameter portParam;
    portParam.name = "port";
    portParam.type = "string";
    portParam.required = true;
    portParam.description = "Serial port name (e.g., 'COM1', '/dev/ttyUSB0')";
    AddParameter(portParam);

    SStepParameter baudParam;
    baudParam.name = "baud_rate";
    baudParam.type = "uint32";
    baudParam.required = false;
    baudParam.description = "Baud rate";
    baudParam.defaultValue = "9600";
    AddParameter(baudParam);

    SStepParameter cmdParam;
    cmdParam.name = "command";
    cmdParam.type = "string";
    cmdParam.required = true;
    cmdParam.description = "Command string to send";
    AddParameter(cmdParam);

    SetDescription("Send command via serial port");
}

CResult CSerialCommandStep::Execute(SStepResult& out_result) {
    out_result.startTime = std::chrono::steady_clock::now();

    auto portParam = GetParameter("port");
    if (!portParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing port parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing port");
    }
    m_strPort = portParam.value();

    auto cmdParam = GetParameter("command");
    if (!cmdParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing command parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing command");
    }
    m_strCommand = cmdParam.value();

    // Create serial connection
    SSerialConfig serialConfig;
    serialConfig.portName = m_strPort;
    serialConfig.baudRate = m_baudRate;
    serialConfig.timeoutMs = m_timeoutMs;
    CSerialConnection serial(serialConfig);

    auto connectResult = serial.Open();
    if (!connectResult.IsSuccess()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Failed to connect to " + m_strPort;
        out_result.endTime = std::chrono::steady_clock::now();
        return connectResult;
    }

    // Send command
    auto sendResult = serial.Write(m_strCommand + m_strTerminator);
    if (!sendResult.IsSuccess()) {
        serial.Close();
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Failed to send command";
        out_result.endTime = std::chrono::steady_clock::now();
        return sendResult;
    }

    // Read response if expected
    if (m_bExpectResponse) {
        auto receiveResult = serial.Read(m_strResponse, m_timeoutMs);
        if (receiveResult.IsSuccess()) {
            out_result.measurements["response"] = m_strResponse;
        }
    }

    serial.Close();

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "Command sent successfully";
    out_result.endTime = std::chrono::steady_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return TESTMATE_SUCCESS();
}

void CSerialCommandStep::SetPort(const TString& in_strPort) {
    m_strPort = in_strPort;
    SetParameter("port", in_strPort);
}

void CSerialCommandStep::SetBaudRate(TUInt32 in_baudRate) {
    m_baudRate = in_baudRate;
}

void CSerialCommandStep::SetCommand(const TString& in_strCommand) {
    m_strCommand = in_strCommand;
    SetParameter("command", in_strCommand);
}

} // namespace TestMATE
