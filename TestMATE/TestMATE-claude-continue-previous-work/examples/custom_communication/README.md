# Custom Communication Protocol Examples

This directory contains comprehensive examples for implementing custom Ethernet and UART communication protocols for your Device Under Test (DUT). These examples are specifically useful for RF board testing, embedded device communication, and any scenario requiring custom binary protocols.

---

## 📚 Table of Contents

- [Overview](#overview)
- [When to Use These Examples](#when-to-use-these-examples)
- [What's Included](#whats-included)
- [Quick Start](#quick-start)
- [Ethernet Communication](#ethernet-communication)
- [UART Communication](#uart-communication)
- [Test Steps](#test-steps)
- [Example Test Sequence](#example-test-sequence)
- [Customization Guide](#customization-guide)
- [Building](#building)
- [API Reference](#api-reference)

---

## Overview

TestMATE provides a flexible communication framework, but doesn't include drivers for every possible custom protocol. These examples show you how to:

1. **Implement custom Ethernet protocols** (TCP/UDP) with packet framing
2. **Implement custom UART protocols** with CRC validation
3. **Create test steps** that use your custom communication
4. **Build complete test sequences** for RF boards and embedded devices

**Key Features:**
- ✅ Custom packet framing and validation
- ✅ CRC/checksum support
- ✅ Timeout handling
- ✅ Retry logic for robustness
- ✅ Register read/write abstraction
- ✅ Thread-safe execution context integration

---

## When to Use These Examples

### ✅ Use These Examples If:
- You have RF boards with Ethernet or UART interfaces
- Your DUT uses a custom binary protocol (not SCPI)
- You need register-level access to embedded devices
- You want packet-based communication with validation
- Your protocol requires custom framing/checksums

### ❌ Not Needed If:
- Your instruments use standard SCPI commands (use TestMATE's built-in SCPI support)
- You only need simple text-based communication
- Your DUT uses standard protocols (HTTP, Modbus, etc.)

---

## What's Included

### Core Communication Classes

#### 1. **EthernetConnection** (`EthernetConnection.h/cpp`)
Complete TCP/UDP Ethernet communication with custom packet framing.

**Features:**
- TCP streaming and UDP datagram support
- Custom packet header format
- XOR checksum validation
- Configurable timeouts
- Keep-alive support

**Packet Format:**
```
[StartByte][PacketType][Length MSB][Length LSB][Checksum][Payload...]
   0xAA        1 byte      1 byte      1 byte     1 byte    0-65535 bytes
```

#### 2. **CustomUARTProtocol** (`CustomUARTProtocol.h/cpp`)
UART protocol wrapper with command/response pattern.

**Features:**
- CRC-8 validation
- Retry logic (configurable)
- Register read/write abstraction
- Firmware version query
- Device status monitoring

**Packet Format:**
```
[Header][CommandID][Length][Payload...][CRC8]
  0x55     1 byte    1 byte   0-255 bytes  1 byte
```

### Test Step Examples

#### 3. **CDUTRegisterTestStep** (`DUTCommTestStep.h/cpp`)
Test step for register operations via UART.

**Operations:**
- Read: Read and validate register value
- Write: Write value to register
- Verify: Write then read back to confirm

#### 4. **CEthernetCommandTestStep** (`DUTCommTestStep.h/cpp`)
Test step for Ethernet packet commands.

**Features:**
- Send custom packets
- Receive and validate responses
- Store results in execution context

### Example Files

- **example_main.cpp** - Complete usage examples
- **rf_board_test.json** - Example test sequence for RF board
- **CMakeLists.txt** - Build configuration

---

## Quick Start

### 1. Review the Example Application

```bash
cd examples/custom_communication
cat example_main.cpp
```

This shows three complete examples:
1. Ethernet communication
2. UART communication
3. Test steps with custom protocols

### 2. Customize for Your DUT

**Modify packet formats:**
```cpp
// In EthernetConnection.h - customize this structure
struct SPacketHeader {
    TUInt8 startByte;    // Change to your start marker
    TUInt8 packetType;   // Your command IDs
    TUInt16 length;      // Payload size
    TUInt8 checksum;     // Your validation method
};
```

**Modify UART commands:**
```cpp
// In CustomUARTProtocol.h - add your DUT's commands
enum class ECommand : TUInt8 {
    kReadRegister    = 0x01,  // Your read command ID
    kWriteRegister   = 0x02,  // Your write command ID
    kYourCustomCmd   = 0x10,  // Add your commands here
    // ...
};
```

### 3. Build the Examples

```bash
cd build
cmake ..
make custom_communication_examples
make custom_comm_example_app
```

### 4. Run the Example Application

```bash
./bin/custom_comm_example_app
```

**Note:** Edit `example_main.cpp` to uncomment the examples and adjust IP addresses/ports for your setup.

---

## Ethernet Communication

### Basic Usage

```cpp
#include "EthernetConnection.h"

// Create connection
CEthernetConnection ethernet;

// Connect via TCP
auto result = ethernet.ConnectTCP("192.168.1.100", 5000);
if (!result.IsSuccess()) {
    // Handle error
}

// Send custom packet
TVector<TUInt8> payload = {0x01, 0x02, 0x03, 0x04};
TUInt8 packetType = 0x10;  // Your command type
result = ethernet.SendPacket(payload, packetType);

// Receive response
TVector<TUInt8> responsePayload;
TUInt8 responseType;
result = ethernet.ReceivePacket(responsePayload, responseType);

// Disconnect
ethernet.Disconnect();
```

### UDP Example

```cpp
// Connect via UDP
auto result = ethernet.ConnectUDP("192.168.1.100", 5000);

// Send/receive works the same as TCP
ethernet.SendPacket(payload, packetType);
ethernet.ReceivePacket(responsePayload, responseType);
```

### Custom Packet Format

Modify the `SPacketHeader` structure in `EthernetConnection.h` to match your DUT's protocol:

```cpp
struct SPacketHeader {
    TUInt8 startByte;     // Change: 0xAA -> your marker
    TUInt8 packetType;    // Your command IDs
    TUInt16 length;       // Keep or change to TUInt32 for larger packets
    TUInt8 checksum;      // Change to CRC16, CRC32, or your algorithm
};
```

Then update the serialization/parsing code in `EthernetConnection.cpp`:
- `SendPacket()` - Modify packet building
- `ReceivePacket()` - Modify packet parsing
- `CalculateChecksum()` - Implement your checksum/CRC

---

## UART Communication

### Basic Usage

```cpp
#include "core/communication/SerialConnection.h"
#include "CustomUARTProtocol.h"

// Create serial connection
CSerialConnection serial;
auto result = serial.Open("/dev/ttyUSB0", 115200);

// Create protocol wrapper
CCustomUARTProtocol protocol(&serial);

// Read register (16-bit address, 32-bit value)
TUInt16 address = 0x1000;
TUInt32 value;
result = protocol.ReadRegister(address, value);

// Write register
result = protocol.WriteRegister(0x1004, 0x12345678);

// Get firmware version
TString version;
result = protocol.GetFirmwareVersion(version);
// Returns: "1.2.3"

// Get device status
TUInt8 status;
result = protocol.GetDeviceStatus(status);

// Close connection
serial.Close();
```

### Custom Commands

Add your own commands to the protocol:

```cpp
// In CustomUARTProtocol.h - add command ID
enum class ECommand : TUInt8 {
    // ... existing commands
    kStartCalibration = 0x10,  // Your new command
    kGetTemperature   = 0x11,
};

// In CustomUARTProtocol.cpp - implement command
CResult CCustomUARTProtocol::StartCalibration() {
    TVector<TUInt8> params;  // No parameters
    TVector<TUInt8> response;

    return SendCommandWithRetry(ECommand::kStartCalibration, params, response);
}

CResult CCustomUARTProtocol::GetTemperature(TDouble& out_temperature) {
    TVector<TUInt8> params;
    TVector<TUInt8> response;

    auto result = SendCommandWithRetry(ECommand::kGetTemperature, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse temperature from response (example: 4 bytes as float)
    if (response.size() < 4) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidData, "Invalid response");
    }

    // Convert bytes to temperature value
    // ... your parsing logic

    return TESTMATE_SUCCESS();
}
```

### Packet Format Customization

Modify the packet structure in `CustomUARTProtocol.h`:

```cpp
struct SPacket {
    TUInt8 header;           // Change: 0x55 -> your header
    TUInt8 commandId;        // Keep or expand to TUInt16
    TUInt8 length;           // Keep or use TUInt16 for larger payloads
    TVector<TUInt8> payload;
    TUInt8 crc;              // Change to TUInt16 for CRC16
};
```

Then update:
- `CalculateCRC8()` - Implement your CRC algorithm
- `SerializePacket()` - Match your packet structure
- `ParsePacket()` - Parse your format

---

## Test Steps

### Register Test Step Example

```cpp
#include "DUTCommTestStep.h"

// Create UART protocol
CSerialConnection serial;
serial.Open("/dev/ttyUSB0", 115200);
CCustomUARTProtocol protocol(&serial);

// Create test step
CDUTRegisterTestStep testStep("REG-001", "Verify Device ID");
testStep.SetUARTProtocol(&protocol);
testStep.SetRegisterAddress(0x0000);
testStep.SetExpectedValue(0x12345678);
testStep.SetMask(0xFFFFFFFF);  // Check all bits
testStep.SetOperation(CDUTRegisterTestStep::EOperation::kRead);

// Execute test
CExecutionContext context(0, 0);
auto result = testStep.Execute(context);

// Check result
if (!result.IsSuccess()) {
    std::cerr << "Test failed: " << result.GetMessage() << std::endl;
}

// Retrieve stored values
auto regValue = context.GetVariable<TInt64>("register_value");
if (regValue) {
    std::cout << "Register value: 0x" << std::hex << *regValue << std::endl;
}
```

### Ethernet Command Test Step Example

```cpp
// Create Ethernet connection
CEthernetConnection ethernet;
ethernet.ConnectTCP("192.168.1.100", 5000);

// Create test step
CEthernetCommandTestStep testStep("ETH-001", "Get Status");
testStep.SetConnection(&ethernet);
testStep.SetPacketType(0x10);  // Your command type
testStep.SetPayload({0x01, 0x02});  // Command parameters
testStep.SetExpectedResponseType(0x90);  // Expected response
testStep.SetTimeout(5000);

// Execute test
CExecutionContext context(0, 0);
auto result = testStep.Execute(context);

// Check response
auto responseType = context.GetVariable<TInt64>("response_packet_type");
auto responseSize = context.GetVariable<TInt64>("response_payload_size");
```

---

## Example Test Sequence

See `rf_board_test.json` for a complete RF board test sequence:

**Test Flow:**
1. Connect via UART
2. Verify device ID and firmware version
3. Configure RF transmitter (frequency, power)
4. Enable TX and verify status
5. Switch to Ethernet for high-speed data transfer
6. Request test data via Ethernet
7. Disable TX
8. Disconnect

**To use this sequence:**
1. Customize register addresses for your DUT
2. Adjust IP address and UART port
3. Modify expected values
4. Add your RF measurement steps (spectrum analyzer, etc.)

---

## Customization Guide

### 1. Change Packet Start Markers

**Ethernet:**
```cpp
// In EthernetConnection.h
static constexpr TUInt8 PACKET_START_BYTE = 0xAA;  // Change this
```

**UART:**
```cpp
// In CustomUARTProtocol.h
static constexpr TUInt8 PACKET_HEADER = 0x55;  // Change this
```

### 2. Implement Different CRC/Checksum

**Example: CRC16-CCITT**
```cpp
TUInt16 CalculateCRC16(const TVector<TUInt8>& in_data) {
    TUInt16 crc = 0xFFFF;
    for (TUInt8 byte : in_data) {
        crc ^= (static_cast<TUInt16>(byte) << 8);
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;  // CCITT polynomial
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

### 3. Add Multi-Byte Register Support

```cpp
// Read 64-bit register
CResult ReadRegister64(TUInt16 in_address, TUInt64& out_value) {
    TVector<TUInt8> params = {
        static_cast<TUInt8>((in_address >> 8) & 0xFF),
        static_cast<TUInt8>(in_address & 0xFF)
    };

    TVector<TUInt8> response;
    auto result = SendCommandWithRetry(ECommand::kReadRegister, params, response);

    if (response.size() < 8) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidData, "Invalid response size");
    }

    out_value = (static_cast<TUInt64>(response[0]) << 56) |
                (static_cast<TUInt64>(response[1]) << 48) |
                // ... continue for all 8 bytes
                 static_cast<TUInt64>(response[7]);

    return TESTMATE_SUCCESS();
}
```

### 4. Add Burst Read/Write

```cpp
CResult ReadRegisterBurst(TUInt16 in_startAddr, TUInt16 in_count,
                          TVector<TUInt32>& out_values) {
    TVector<TUInt8> params = {
        static_cast<TUInt8>((in_startAddr >> 8) & 0xFF),
        static_cast<TUInt8>(in_startAddr & 0xFF),
        static_cast<TUInt8>((in_count >> 8) & 0xFF),
        static_cast<TUInt8>(in_count & 0xFF)
    };

    TVector<TUInt8> response;
    auto result = SendCommandWithRetry(ECommand::kReadBurst, params, response);

    // Parse response into vector of values
    out_values.clear();
    for (size_t i = 0; i + 3 < response.size(); i += 4) {
        TUInt32 value = (static_cast<TUInt32>(response[i]) << 24) |
                        (static_cast<TUInt32>(response[i+1]) << 16) |
                        (static_cast<TUInt32>(response[i+2]) << 8) |
                         static_cast<TUInt32>(response[i+3]);
        out_values.push_back(value);
    }

    return TESTMATE_SUCCESS();
}
```

---

## Building

### Add to Your Project

1. **Copy files to your project:**
```bash
cp -r examples/custom_communication /path/to/your/project/
```

2. **Add to CMakeLists.txt:**
```cmake
add_subdirectory(custom_communication)

target_link_libraries(your_executable
    PRIVATE
        custom_communication_examples
        testmate_core
        testmate_utils
)
```

3. **Include headers:**
```cpp
#include "custom_communication/EthernetConnection.h"
#include "custom_communication/CustomUARTProtocol.h"
#include "custom_communication/DUTCommTestStep.h"
```

### Build Examples Only

```bash
cd examples/custom_communication
mkdir build
cd build
cmake ..
make
```

This creates:
- `libcustom_communication_examples.a` - Static library
- `custom_comm_example_app` - Example application

---

## API Reference

### CEthernetConnection

| Method | Description |
|--------|-------------|
| `ConnectTCP(ip, port)` | Connect via TCP |
| `ConnectUDP(ip, port)` | Connect via UDP |
| `SendPacket(payload, type)` | Send custom packet |
| `ReceivePacket(payload, type)` | Receive packet |
| `SetTimeout(ms)` | Set receive timeout |
| `SetKeepAlive(enable)` | Enable TCP keep-alive |

### CCustomUARTProtocol

| Method | Description |
|--------|-------------|
| `ReadRegister(addr, value)` | Read 32-bit register |
| `WriteRegister(addr, value)` | Write 32-bit register |
| `GetDeviceStatus(status)` | Get status byte |
| `GetFirmwareVersion(version)` | Get version string |
| `ResetDevice()` | Reset DUT |
| `SetRetryCount(count)` | Set retry attempts |
| `SetDefaultTimeout(ms)` | Set response timeout |

### CDUTRegisterTestStep

| Method | Description |
|--------|-------------|
| `SetRegisterAddress(addr)` | Set target address |
| `SetExpectedValue(value)` | Set expected value |
| `SetWriteValue(value)` | Set write value |
| `SetMask(mask)` | Set comparison mask |
| `SetOperation(op)` | Read/Write/Verify |
| `SetUARTProtocol(protocol)` | Set protocol handler |

### CEthernetCommandTestStep

| Method | Description |
|--------|-------------|
| `SetConnection(conn)` | Set Ethernet connection |
| `SetPacketType(type)` | Set command type |
| `SetPayload(data)` | Set command data |
| `SetExpectedResponseType(type)` | Set expected response |
| `SetTimeout(ms)` | Set response timeout |

---

## Error Handling

All methods return `CResult` which can be checked:

```cpp
auto result = protocol.ReadRegister(0x1000, value);
if (!result.IsSuccess()) {
    // Get error code
    auto errorCode = result.GetErrorCode();

    // Get error message
    std::cerr << "Error: " << result.GetMessage() << std::endl;

    // Handle specific errors
    if (errorCode == EErrorCode::kTimeout) {
        // Retry or handle timeout
    }
}
```

Common error codes:
- `kConnectionFailed` - Cannot connect to DUT
- `kCommunicationFailed` - Send/receive error
- `kTimeout` - Operation timed out
- `kInvalidData` - Checksum/CRC mismatch
- `kTestFailed` - Register value mismatch

---

## Performance Tips

1. **Adjust timeouts** based on your DUT response time
2. **Use retry logic** for noisy environments
3. **Batch operations** when possible (burst read/write)
4. **Cache frequently accessed registers** in execution context
5. **Use appropriate baud rates** for UART (115200 is a good start)

---

## Common Issues

### "Failed to connect" errors
- Check IP address/port are correct
- Verify DUT is powered and network accessible
- Check firewall settings
- Try `ping` to verify network connectivity

### "CRC mismatch" errors
- Verify CRC algorithm matches your DUT
- Check byte order (big-endian vs little-endian)
- Ensure packet framing is correct

### "Timeout" errors
- Increase timeout value
- Check UART baud rate matches DUT
- Verify DUT is responding to commands
- Add logging to see what's being sent/received

---

## Next Steps

1. **Study the example code** in `example_main.cpp`
2. **Customize packet formats** for your DUT
3. **Add your DUT-specific commands** to the protocol classes
4. **Create test steps** for your specific tests
5. **Build test sequences** using JSON format
6. **Integrate with RF instrument drivers** (see `examples/custom_plugin/`)

---

## Support

For questions or issues:
1. Check the TestMATE main documentation
2. Review the example code in this directory
3. File an issue on the TestMATE GitHub repository

---

**Happy Testing! 🚀**
