/**************************************************************************
 * File Name: example_main.cpp
 * Description: Example application demonstrating custom communication
 * Author: TestMATE Development Team
 *
 * This example shows how to:
 * 1. Connect to DUT via Ethernet or UART
 * 2. Send custom protocol commands
 * 3. Read/write registers
 * 4. Create test sequences using custom communication
 **************************************************************************/

#include "EthernetConnection.h"
#include "CustomUARTProtocol.h"
#include "DUTCommTestStep.h"
#include "core/communication/SerialConnection.h"
#include "utils/LogManager.h"
#include <iostream>

using namespace TestMATE;

/**
 * @brief Example 1: Ethernet communication with DUT
 */
void ExampleEthernetCommunication() {
    std::cout << "\n=== Example 1: Ethernet Communication ===\n" << std::endl;

    // Create Ethernet connection
    CEthernetConnection ethernet;

    // Connect to DUT (TCP)
    auto result = ethernet.ConnectTCP("192.168.1.100", 5000);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to connect: " << result.GetMessage() << std::endl;
        return;
    }

    std::cout << "Connected to DUT via Ethernet" << std::endl;

    // Send custom packet
    TVector<TUInt8> payload = {0x01, 0x02, 0x03, 0x04};  // Example payload
    TUInt8 packetType = 0x10;  // Example command type

    result = ethernet.SendPacket(payload, packetType);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to send packet: " << result.GetMessage() << std::endl;
        ethernet.Disconnect();
        return;
    }

    std::cout << "Sent packet type 0x" << std::hex << (int)packetType << std::dec << std::endl;

    // Receive response
    TVector<TUInt8> responsePayload;
    TUInt8 responseType;

    result = ethernet.ReceivePacket(responsePayload, responseType);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to receive response: " << result.GetMessage() << std::endl;
        ethernet.Disconnect();
        return;
    }

    std::cout << "Received response type 0x" << std::hex << (int)responseType << std::dec
              << " with " << responsePayload.size() << " bytes" << std::endl;

    // Disconnect
    ethernet.Disconnect();
    std::cout << "Disconnected from DUT\n" << std::endl;
}

/**
 * @brief Example 2: UART communication with DUT
 */
void ExampleUARTCommunication() {
    std::cout << "\n=== Example 2: UART Communication ===\n" << std::endl;

    // Configure serial connection
    SSerialConfig config;
    config.portName = "/dev/ttyUSB0";
    config.baudRate = 115200;
    config.timeoutMs = 1000;

    // Create serial connection with configuration
    CSerialConnection serial(config);

    // Open connection (adjust port and baud rate for your system)
    auto result = serial.Open();
    if (!result.IsSuccess()) {
        std::cerr << "Failed to open serial port: " << result.GetMessage() << std::endl;
        std::cerr << "Note: Adjust port name (/dev/ttyUSB0) for your system\n" << std::endl;
        return;
    }

    std::cout << "Opened UART connection at 115200 baud" << std::endl;

    // Create custom UART protocol wrapper
    CCustomUARTProtocol protocol(&serial);

    // Read register from DUT
    TUInt16 regAddress = 0x1000;  // Example register address
    TUInt32 regValue;

    result = protocol.ReadRegister(regAddress, regValue);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to read register: " << result.GetMessage() << std::endl;
        serial.Close();
        return;
    }

    std::cout << "Register 0x" << std::hex << regAddress << " = 0x" << regValue << std::dec << std::endl;

    // Write register to DUT
    TUInt32 newValue = 0x12345678;
    result = protocol.WriteRegister(regAddress, newValue);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to write register: " << result.GetMessage() << std::endl;
        serial.Close();
        return;
    }

    std::cout << "Wrote 0x" << std::hex << newValue << std::dec << " to register 0x"
              << std::hex << regAddress << std::dec << std::endl;

    // Get firmware version
    TString version;
    result = protocol.GetFirmwareVersion(version);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to get version: " << result.GetMessage() << std::endl;
    } else {
        std::cout << "Firmware version: " << version << std::endl;
    }

    // Get device status
    TUInt8 status;
    result = protocol.GetDeviceStatus(status);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to get status: " << result.GetMessage() << std::endl;
    } else {
        std::cout << "Device status: 0x" << std::hex << (int)status << std::dec << std::endl;
    }

    // Close connection
    serial.Close();
    std::cout << "Closed UART connection\n" << std::endl;
}

/**
 * @brief Example 3: Using test steps with custom communication
 */
void ExampleTestSteps() {
    std::cout << "\n=== Example 3: Test Steps with Custom Communication ===\n" << std::endl;

    // Configure serial connection
    SSerialConfig config;
    config.portName = "/dev/ttyUSB0";
    config.baudRate = 115200;
    config.timeoutMs = 1000;

    // Create serial connection and protocol
    CSerialConnection serial(config);
    auto result = serial.Open();
    if (!result.IsSuccess()) {
        std::cerr << "Failed to open serial port: " << result.GetMessage() << std::endl;
        std::cerr << "Note: This example requires a connected DUT\n" << std::endl;
        return;
    }

    CCustomUARTProtocol protocol(&serial);

    // Create test step
    CDUTRegisterTestStep testStep("REG-001", "Read Control Register");
    testStep.SetUARTProtocol(&protocol);
    testStep.SetRegisterAddress(0x1000);
    testStep.SetExpectedValue(0x00000001);  // Expect bit 0 to be set
    testStep.SetMask(0x00000001);           // Check only bit 0
    testStep.SetOperation(CDUTRegisterTestStep::EOperation::kRead);

    // Create simple context for storing results
    SSimpleContext context;

    // Execute test step
    result = testStep.Execute(context);
    if (!result.IsSuccess()) {
        std::cerr << "Test step failed: " << result.GetMessage() << std::endl;
    } else {
        std::cout << "Test step PASSED" << std::endl;

        // Retrieve stored values from context
        auto regAddr = context.GetVariable<TInt64>("register_address");
        auto regVal = context.GetVariable<TInt64>("register_value");

        if (regAddr && regVal) {
            std::cout << "Register 0x" << std::hex << *regAddr << " = 0x" << *regVal << std::dec << std::endl;
        }
    }

    serial.Close();
    std::cout << std::endl;
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "TestMATE Custom Communication Examples\n";
    std::cout << "======================================\n" << std::endl;

    // Set log level
    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kInfo);

    // Note: These examples require actual hardware connections
    std::cout << "NOTE: These examples require connected DUT hardware." << std::endl;
    std::cout << "Adjust IP addresses, ports, and serial port names for your setup.\n" << std::endl;

    // Run examples (comment out if hardware not available)
    // ExampleEthernetCommunication();
    // ExampleUARTCommunication();
    // ExampleTestSteps();

    std::cout << "Examples complete. Check the source code for implementation details." << std::endl;
    std::cout << "Edit example_main.cpp to customize for your DUT.\n" << std::endl;

    return 0;
}
