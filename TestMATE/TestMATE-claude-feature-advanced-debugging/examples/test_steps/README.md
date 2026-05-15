# TestMATE Essential Test Steps

This directory contains production-ready test step implementations that can be used directly in your TestMATE test sequences.

## Overview

TestMATE provides 6 essential test steps covering all common testing operations:

| Step | Type | Purpose | Use Cases |
|------|------|---------|-----------|
| **WaitStep** | Delay | Time delays and synchronization | Power-up delays, settling time, polling intervals |
| **LimitCheckStep** | Validation | Validate measurements against limits | Pass/fail checks, range validation, tolerance checking |
| **CalculationStep** | Measurement | Mathematical calculations | Power = V × I, efficiency, derived parameters |
| **InstrumentMeasureStep** | Measurement | Instrument measurements | Voltage, current, resistance, temperature |
| **SerialCommandStep** | Action | Serial port communication | Device control, UART commands |
| **FileOperationStep** | Action | File I/O operations | Data logging, configuration files, reports |

---

## 1. WaitStep

**Purpose**: Introduce time delays with optional abort capability

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `duration_ms` | int64 | Yes | 1000 | Wait duration in milliseconds |
| `allow_abort` | bool | No | true | Allow step to be aborted during wait |

### Usage Example

```cpp
#include "test_steps/WaitStep.h"

// Create wait step
auto waitStep = std::make_unique<CWaitStep>(
    "WAIT-001",           // ID
    "Wait for Power-Up",  // Name
    500                   // 500ms duration
);

// Configure
waitStep->SetDuration(1000);          // Change to 1000ms
waitStep->SetAllowAbort(true);        // Allow user abort

// Execute
SStepResult result;
waitStep->Execute(result);

// Check result
if (result.verdict == ETestVerdict::kPass) {
    std::cout << "Wait completed: " << result.durationMs << "ms" << std::endl;
}
```

### JSON Sequence Example

```json
{
  "id": "WAIT-001",
  "name": "Wait for Stabilization",
  "type": "wait",
  "parameters": {
    "duration_ms": "2000",
    "allow_abort": "true"
  }
}
```

### Features

- ✅ Abortable waits (checks every 100ms)
- ✅ Precise timing with std::this_thread::sleep_for
- ✅ Actual duration reported in results
- ✅ Thread-safe

---

## 2. LimitCheckStep

**Purpose**: Validate measured values against specified limits

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `value` | double | Yes | - | Value to check |
| `limit_min` | double | No | -∞ | Minimum acceptable value |
| `limit_max` | double | No | +∞ | Maximum acceptable value |
| `tolerance` | double | No | 0.001 | Tolerance for equality checks |
| `unit` | string | No | "" | Measurement unit for display |

### Limit Types

- **Range Check**: `value` must be between `limit_min` and `limit_max`
- **Minimum Only**: `value` must be ≥ `limit_min`
- **Maximum Only**: `value` must be ≤ `limit_max`
- **Equality**: `value` must equal target ± `tolerance`
- **Not Equal**: `value` must differ from target by more than `tolerance`

### Usage Example

```cpp
#include "test_steps/LimitCheckStep.h"

// Create limit check
auto limitCheck = std::make_unique<CLimitCheckStep>(
    "LIMIT-001",
    "Validate Supply Voltage"
);

// Configure limits
limitCheck->SetLimits(4.75, 5.25);    // 5.0V ±5%
limitCheck->SetValue(5.02);            // Measured value
limitCheck->SetUnit("V");              // Volts

// Execute
SStepResult result;
limitCheck->Execute(result);

// Check result
if (result.verdict == ETestVerdict::kPass) {
    std::cout << result.message << std::endl;
    // Output: "PASS: 5.020000 V within range [4.750000 V, 5.250000 V]"
} else {
    std::cout << "FAIL: " << result.message << std::endl;
}
```

### JSON Sequence Example

```json
{
  "id": "LIMIT-001",
  "name": "Validate 3.3V Rail",
  "type": "validation",
  "parameters": {
    "value": "3.31",
    "limit_min": "3.135",
    "limit_max": "3.465",
    "unit": "V"
  }
}
```

### Features

- ✅ Multiple limit types (range, min-only, max-only, equals, not-equals)
- ✅ Automatic verdict generation (Pass/Fail)
- ✅ Detailed pass/fail messages
- ✅ Measurement values stored in results
- ✅ Unit support for clear reporting

---

## 3. CalculationStep

**Purpose**: Perform mathematical calculations on input values

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `operation` | string | Yes | "add" | Calculation type (see below) |
| `operand_a` | double | Yes | - | First operand |
| `operand_b` | double | No | - | Second operand (for binary operations) |
| `formula` | string | No | "" | Custom formula string |
| `result_name` | string | No | "result" | Name for result in output |
| `unit` | string | No | "" | Unit for result value |

### Supported Operations

| Operation | Formula | Description | Operands |
|-----------|---------|-------------|----------|
| `add` | a + b + c + ... | Sum of all operands | 1+ |
| `subtract` | a - b | Subtraction | 2 |
| `multiply` | a × b × c × ... | Product of all operands | 1+ |
| `divide` | a / b | Division | 2 |
| `power` | a^b | Exponentiation | 2 |
| `sqrt` | √a | Square root | 1 |
| `abs` | \|a\| | Absolute value | 1 |
| `average` | (a + b + ...) / n | Average/mean | 1+ |
| `min` | min(a, b, ...) | Minimum value | 1+ |
| `max` | max(a, b, ...) | Maximum value | 1+ |
| `formula` | Custom | Evaluate custom formula | Variable |

### Usage Example

```cpp
#include "test_steps/CalculationStep.h"

// Calculate power from voltage and current
auto calcPower = std::make_unique<CCalculationStep>(
    "CALC-001",
    "Calculate Power"
);

// Set operation and operands
calcPower->SetOperation(ECalculationType::kMultiply);
std::map<TString, TDouble> operands;
operands["a"] = 12.0;  // Voltage
operands["b"] = 2.5;   // Current
calcPower->SetOperands(operands);
calcPower->SetUnit("W");

// Execute
SStepResult result;
calcPower->Execute(result);

// Get result
TDouble power = calcPower->GetResult();  // 30.0
std::cout << "Power: " << result.measurements["result"] << std::endl;
// Output: "Power: 30.000000 W"
```

### Common Calculations

**Power (P = V × I)**:
```cpp
calcStep->SetOperation(ECalculationType::kMultiply);
operands["a"] = voltage;
operands["b"] = current;
calcStep->SetUnit("W");
```

**Resistance (R = V / I)**:
```cpp
calcStep->SetOperation(ECalculationType::kDivide);
operands["a"] = voltage;
operands["b"] = current;
calcStep->SetUnit("Ω");
```

**Efficiency (η = P_out / P_in × 100)**:
```cpp
calcStep->SetOperation(ECalculationType::kDivide);
operands["a"] = powerOut;
operands["b"] = powerIn;
// Then multiply by 100
calcStep->SetUnit("%");
```

### JSON Sequence Example

```json
{
  "id": "CALC-001",
  "name": "Calculate Resistance",
  "type": "measurement",
  "parameters": {
    "operation": "divide",
    "operand_a": "5.0",
    "operand_b": "0.050",
    "result_name": "resistance",
    "unit": "Ω"
  }
}
```

---

## 4. InstrumentMeasureStep

**Purpose**: Take measurements from test instruments

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `instrument_id` | string | Yes | - | ID of instrument to use |
| `command` | string | Yes | - | SCPI/instrument command |
| `timeout_ms` | int64 | No | 5000 | Command timeout (ms) |
| `result_name` | string | No | "measurement" | Name for result |
| `unit` | string | No | "" | Measurement unit |
| `parse_numeric` | bool | No | true | Parse response as number |

### Usage Example

```cpp
#include "test_steps/InstrumentMeasureStep.h"

// Measure DC voltage with multimeter
auto measStep = std::make_unique<CInstrumentMeasureStep>(
    "MEAS-001",
    "Measure Supply Voltage"
);

// Configure
measStep->SetInstrument("DMM-001");          // Multimeter ID
measStep->SetCommand("MEAS:VOLT:DC?");       // SCPI command
measStep->SetTimeout(3000);                  // 3 second timeout
measStep->SetUnit("V");

// Execute
SStepResult result;
measStep->Execute(result);

// Get measurement
TDouble voltage = measStep->GetMeasuredValue();
std::cout << "Voltage: " << voltage << " V" << std::endl;
```

### Common Instrument Commands

**Multimeter (SCPI)**:
```cpp
measStep->SetCommand("MEAS:VOLT:DC?");       // DC Voltage
measStep->SetCommand("MEAS:CURR:DC?");       // DC Current
measStep->SetCommand("MEAS:RES?");            // Resistance
measStep->SetCommand("MEAS:FREQ?");           // Frequency
```

**Power Supply (SCPI)**:
```cpp
measStep->SetCommand("MEAS:VOLT?");          // Measure output voltage
measStep->SetCommand("MEAS:CURR?");          // Measure output current
```

**Oscilloscope (SCPI)**:
```cpp
measStep->SetCommand("MEAS:VMAX? CHAN1");    // Peak voltage Ch1
measStep->SetCommand("MEAS:FREQ? CHAN1");    // Frequency Ch1
```

### JSON Sequence Example

```json
{
  "id": "MEAS-001",
  "name": "Measure 3.3V Rail",
  "type": "measurement",
  "parameters": {
    "instrument_id": "DMM-001",
    "command": "MEAS:VOLT:DC?",
    "timeout_ms": "5000",
    "result_name": "rail_3v3",
    "unit": "V",
    "parse_numeric": "true"
  }
}
```

### Features

- ✅ SCPI command support
- ✅ Automatic numeric parsing from response
- ✅ Configurable timeouts
- ✅ Raw response available (`GetRawResponse()`)
- ✅ Integration with InstrumentManager

---

## 5. SerialCommandStep

**Purpose**: Send commands via serial port

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `port` | string | Yes | - | Serial port name |
| `baud_rate` | uint32 | No | 9600 | Baud rate |
| `command` | string | Yes | - | Command to send |
| `expect_response` | bool | No | true | Wait for response |
| `timeout_ms` | int64 | No | 1000 | Response timeout (ms) |
| `terminator` | string | No | "\\r\\n" | Line terminator |

### Usage Example

```cpp
#include "test_steps/SerialCommandStep.h"

// Send command to device via serial
auto serialStep = std::make_unique<CSerialCommandStep>(
    "SERIAL-001",
    "Reset Device"
);

// Configure
serialStep->SetPort("/dev/ttyUSB0");  // Linux
// serialStep->SetPort("COM3");        // Windows
serialStep->SetBaudRate(115200);
serialStep->SetCommand("RESET");

// Execute
SStepResult result;
serialStep->Execute(result);

// Check response if available
if (result.measurements.count("response")) {
    std::cout << "Device response: " << result.measurements["response"] << std::endl;
}
```

### Common Use Cases

**Device Reset**:
```cpp
serialStep->SetCommand("*RST");
```

**Query Device ID**:
```cpp
serialStep->SetCommand("*IDN?");
```

**Configure Settings**:
```cpp
serialStep->SetCommand("CONFIG:MODE AUTO");
```

---

## 6. FileOperationStep

**Purpose**: Perform file system operations

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `operation` | string | Yes | - | File operation type |
| `file_path` | string | Yes | - | Path to file |
| `content` | string | No | "" | Content for write/append |
| `destination` | string | No | "" | Destination for copy/move |

### Supported Operations

| Operation | Description | Parameters Used |
|-----------|-------------|-----------------|
| `read` | Read file contents | `file_path` |
| `write` | Write to file (overwrite) | `file_path`, `content` |
| `append` | Append to file | `file_path`, `content` |
| `delete` | Delete file | `file_path` |
| `exists` | Check if file exists | `file_path` |

### Usage Example

```cpp
#include "test_steps/FileOperationStep.h"

// Write test data to file
auto fileStep = std::make_unique<CFileOperationStep>(
    "FILE-001",
    "Save Test Results"
);

// Configure
fileStep->SetOperation(EFileOperation::kWrite);
fileStep->SetFilePath("/tmp/test_results.txt");
fileStep->SetContent("Voltage: 5.02V\nCurrent: 2.50A\nPower: 12.55W\n");

// Execute
SStepResult result;
fileStep->Execute(result);

// Read back
fileStep->SetOperation(EFileOperation::kRead);
fileStep->Execute(result);
TString content = fileStep->GetReadContent();
```

### Common Use Cases

**Log Data**:
```cpp
fileStep->SetOperation(EFileOperation::kAppend);
fileStep->SetFilePath("test_log.csv");
fileStep->SetContent("2024-01-22,5.02,PASS\n");
```

**Save Configuration**:
```cpp
fileStep->SetOperation(EFileOperation::kWrite);
fileStep->SetFilePath("config.json");
fileStep->SetContent(jsonConfig);
```

**Check File Exists Before Reading**:
```cpp
fileStep->SetOperation(EFileOperation::kExists);
fileStep->Execute(result);
if (result.measurements["exists"] == "true") {
    fileStep->SetOperation(EFileOperation::kRead);
    fileStep->Execute(result);
}
```

---

## Complete Example: Power Supply Test

Here's a complete test sequence using all steps:

```cpp
#include "test_steps/WaitStep.h"
#include "test_steps/InstrumentMeasureStep.h"
#include "test_steps/CalculationStep.h"
#include "test_steps/LimitCheckStep.h"
#include "test_steps/FileOperationStep.h"

void RunPowerSupplyTest() {
    // Step 1: Wait for power-up
    auto wait1 = std::make_unique<CWaitStep>("WAIT-001", "Power-Up Delay", 500);
    SStepResult result;
    wait1->Execute(result);

    // Step 2: Measure voltage
    auto measV = std::make_unique<CInstrumentMeasureStep>("MEAS-001", "Measure Voltage");
    measV->SetInstrument("DMM-001");
    measV->SetCommand("MEAS:VOLT:DC?");
    measV->Execute(result);
    TDouble voltage = measV->GetMeasuredValue();

    // Step 3: Measure current
    auto measI = std::make_unique<CInstrumentMeasureStep>("MEAS-002", "Measure Current");
    measI->SetInstrument("DMM-001");
    measI->SetCommand("MEAS:CURR:DC?");
    measI->Execute(result);
    TDouble current = measI->GetMeasuredValue();

    // Step 4: Calculate power
    auto calcP = std::make_unique<CCalculationStep>("CALC-001", "Calculate Power");
    calcP->SetOperation(ECalculationType::kMultiply);
    std::map<TString, TDouble> ops;
    ops["a"] = voltage;
    ops["b"] = current;
    calcP->SetOperands(ops);
    calcP->Execute(result);
    TDouble power = calcP->GetResult();

    // Step 5: Validate voltage
    auto limitV = std::make_unique<CLimitCheckStep>("LIMIT-001", "Validate Voltage");
    limitV->SetValue(voltage);
    limitV->SetLimits(4.75, 5.25);  // 5V ±5%
    limitV->Execute(result);

    // Step 6: Log results
    auto logFile = std::make_unique<CFileOperationStep>("FILE-001", "Log Results");
    std::ostringstream logData;
    logData << "Voltage: " << voltage << " V\n";
    logData << "Current: " << current << " A\n";
    logData << "Power: " << power << " W\n";
    logData << "Verdict: " << (result.verdict == ETestVerdict::kPass ? "PASS" : "FAIL") << "\n";
    logFile->SetOperation(EFileOperation::kAppend);
    logFile->SetFilePath("power_test_log.txt");
    logFile->SetContent(logData.str());
    logFile->Execute(result);
}
```

---

## Building and Using

### Build Library

```bash
cd examples/test_steps
cmake -B build
cmake --build build
```

### Link in Your Project

```cmake
target_link_libraries(your_test_program
    PRIVATE
        testmate_test_steps
)
```

### Include in Code

```cpp
#include "test_steps/WaitStep.h"
#include "test_steps/LimitCheckStep.h"
#include "test_steps/CalculationStep.h"
#include "test_steps/InstrumentMeasureStep.h"
#include "test_steps/SerialCommandStep.h"
#include "test_steps/FileOperationStep.h"
```

---

## Best Practices

1. **Error Handling**: Always check `result.verdict` after Execute()
2. **Units**: Always specify units for measurements and limits
3. **Timeouts**: Set appropriate timeouts for instruments (consider settling time)
4. **Naming**: Use descriptive step IDs and names for debugging
5. **Logging**: Use FileOperationStep to append to log files for traceability
6. **Abort**: Enable abort for long waits to allow user interruption

---

## See Also

- [Example Test Sequences](../basic_test_plan/) - Complete sequences using these steps
- [Plugin Development](../custom_plugin/) - Creating custom test step plugins
- [Instrument Integration](../../docs/instruments/) - Setting up instruments
- [TestMATE Core Documentation](../../docs/) - Full API reference
