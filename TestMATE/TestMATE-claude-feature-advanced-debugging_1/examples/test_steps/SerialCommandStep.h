/**************************************************************************
 * File Name: SerialCommandStep.h
 * Description: Test step for sending commands via serial port
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"
#include "core/communication/SerialConnection.h"
#include <memory>

namespace TestMATE {

/**************************************************************************
 * Class: CSerialCommandStep
 * Description: Sends commands and reads responses via serial port
 *
 * Parameters:
 *   - port: Serial port name (e.g., "COM1", "/dev/ttyUSB0")
 *   - baud_rate: Baud rate (default: 9600)
 *   - command: Command string to send
 *   - expect_response: Wait for response (default: true)
 *   - timeout_ms: Response timeout (default: 1000)
 *   - terminator: Line terminator (default: "\r\n")
 **************************************************************************/
class CSerialCommandStep : public CTestStepBase {
public:
    explicit CSerialCommandStep(const TString& in_strId = "SERIAL-001",
                               const TString& in_strName = "Serial Command");

    ~CSerialCommandStep() override = default;

    CResult Execute(SStepResult& out_result) override;

    void SetPort(const TString& in_strPort);
    void SetBaudRate(TUInt32 in_baudRate);
    void SetCommand(const TString& in_strCommand);

private:
    TString m_strPort;
    TUInt32 m_baudRate{9600};
    TString m_strCommand;
    bool m_bExpectResponse{true};
    TInt64 m_timeoutMs{1000};
    TString m_strTerminator{"\r\n"};
    TString m_strResponse;
};

} // namespace TestMATE
