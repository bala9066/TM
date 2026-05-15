# TestMATE Quickstart Guide

Get started with TestMATE in 10 minutes! This guide will walk you through building TestMATE, creating your first test sequence, and running automated tests.

---

## 📋 Prerequisites

- **C++20 compiler**: GCC 10+, Clang 12+, or MSVC 2019+
- **CMake**: 3.15 or higher
- **SQLite3**: For database functionality
- **Qt5/Qt6** (optional): For GUI features

---

## 🔨 Step 1: Build TestMATE

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/sathishk35/TestMATE.git
cd TestMATE

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build (use -j4 for parallel build)
cmake --build . -j4
```

### Build Output

You should see:
```
[  3%] Built target testmate_utils
[ 55%] Built target testmate_core
[ 67%] Built target testmate_database
[ 70%] Built target testmate_api
[ 70%] Built target testmate_ui
[100%] Built target testmate_unit_tests
```

### Verify Installation

```bash
# Run unit tests
./bin/testmate_unit_tests

# You should see:
# [==========] 261 tests from 32 test suites
# [  PASSED  ] 261 tests
```

✅ **Success!** TestMATE core is built and all tests pass.

---

## 🎯 Step 2: Your First Test - Power Supply Validation

Let's create a simple test that measures voltage and validates it's within spec.

### Create Test Program

Create `my_first_test.cpp`:

```cpp
#include "examples/test_steps/WaitStep.h"
#include "examples/test_steps/InstrumentMeasureStep.h"
#include "examples/test_steps/LimitCheckStep.h"
#include "examples/test_steps/FileOperationStep.h"
#include <iostream>

using namespace TestMATE;

int main() {
    std::cout << "=== TestMATE Power Supply Validation ===" << std::endl;

    SStepResult result;

    // Step 1: Wait for power stabilization
    std::cout << "\n[1/4] Waiting for power stabilization..." << std::endl;
    CWaitStep waitStep("WAIT-001", "Power Stabilization", 500);
    waitStep.Execute(result);
    std::cout << "✓ " << result.message << std::endl;

    // Step 2: Simulate voltage measurement (in real use, connect to instrument)
    std::cout << "\n[2/4] Measuring voltage..." << std::endl;
    // For this demo, we'll simulate with a calculation
    TDouble measuredVoltage = 5.02;  // Simulated reading
    std::cout << "✓ Measured: " << measuredVoltage << " V" << std::endl;

    // Step 3: Validate voltage is within 5V ±5% (4.75V to 5.25V)
    std::cout << "\n[3/4] Validating voltage limits..." << std::endl;
    CLimitCheckStep limitCheck("LIMIT-001", "Validate 5V Rail");
    limitCheck.SetValue(measuredVoltage);
    limitCheck.SetLimits(4.75, 5.25);  // 5V ±5%
    limitCheck.SetUnit("V");
    limitCheck.Execute(result);

    if (result.verdict == ETestVerdict::kPass) {
        std::cout << "✓ PASS: " << result.message << std::endl;
    } else {
        std::cout << "✗ FAIL: " << result.message << std::endl;
    }

    // Step 4: Log results to file
    std::cout << "\n[4/4] Logging results..." << std::endl;
    CFileOperationStep logStep("FILE-001", "Log Results");
    logStep.SetOperation(EFileOperation::kWrite);
    logStep.SetFilePath("test_results.txt");

    std::ostringstream logContent;
    logContent << "Power Supply Test Results\n";
    logContent << "=========================\n";
    logContent << "Measured Voltage: " << measuredVoltage << " V\n";
    logContent << "Limit Min: 4.75 V\n";
    logContent << "Limit Max: 5.25 V\n";
    logContent << "Verdict: " << (result.verdict == ETestVerdict::kPass ? "PASS" : "FAIL") << "\n";

    logStep.SetContent(logContent.str());
    logStep.Execute(result);
    std::cout << "✓ Results saved to test_results.txt" << std::endl;

    std::cout << "\n=== Test Complete ===" << std::endl;
    return (result.verdict == ETestVerdict::kPass) ? 0 : 1;
}
```

### Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyFirstTest)

set(CMAKE_CXX_STANDARD 20)

# Add TestMATE directories
add_subdirectory(path/to/TestMATE ${CMAKE_BINARY_DIR}/testmate)

# Create executable
add_executable(my_first_test my_first_test.cpp)

# Link TestMATE libraries
target_link_libraries(my_first_test
    PRIVATE
        testmate_core
        testmate_utils
        testmate_test_steps  # Our essential test steps
)
```

### Build and Run

```bash
mkdir build && cd build
cmake ..
cmake --build .
./my_first_test
```

### Expected Output

```
=== TestMATE Power Supply Validation ===

[1/4] Waiting for power stabilization...
✓ Waited 500 ms

[2/4] Measuring voltage...
✓ Measured: 5.02 V

[3/4] Validating voltage limits...
✓ PASS: 5.020000 V within range [4.750000 V, 5.250000 V]

[4/4] Logging results...
✓ Results saved to test_results.txt

=== Test Complete ===
```

✅ **Congratulations!** You've created and run your first TestMATE test!

---

## 🔬 Step 3: Using Test Sequences (JSON)

Instead of hardcoding tests, use JSON sequences for flexibility.

### Create Test Sequence File

Create `power_test.json`:

```json
{
  "id": "PWR-TEST-001",
  "name": "Power Supply Validation Test",
  "version": "1.0.0",
  "steps": [
    {
      "id": "WAIT-001",
      "name": "Wait for Power Stabilization",
      "type": "wait",
      "enabled": true,
      "parameters": {
        "duration_ms": "500"
      }
    },
    {
      "id": "MEAS-001",
      "name": "Measure 5V Rail",
      "type": "measurement",
      "enabled": true,
      "parameters": {
        "instrument_id": "DMM-001",
        "command": "MEAS:VOLT:DC?",
        "result_name": "rail_5v",
        "unit": "V"
      }
    },
    {
      "id": "LIMIT-001",
      "name": "Validate 5V ±5%",
      "type": "validation",
      "enabled": true,
      "parameters": {
        "value": "${MEAS-001.rail_5v}",
        "limit_min": "4.75",
        "limit_max": "5.25",
        "unit": "V"
      }
    }
  ]
}
```

### Load and Execute Sequence

```cpp
#include "core/test_sequence/SequenceFileIO.h"
#include "core/test_sequence/TestSequence.h"

int main() {
    // Load sequence from JSON
    auto& fileIO = CSequenceFileIO::GetInstance();
    CTestSequence sequence;

    auto result = fileIO.LoadSequence("power_test.json", sequence);
    if (!result.IsSuccess()) {
        std::cerr << "Failed to load sequence: " << result.GetMessage() << std::endl;
        return 1;
    }

    std::cout << "Loaded: " << sequence.GetInfo().name << std::endl;
    std::cout << "Steps: " << sequence.GetStepCount() << std::endl;

    // Execute sequence
    // (Requires execution context setup - see full examples)

    return 0;
}
```

---

## 🧪 Step 4: Complete Example - Multimeter Test

Here's a complete example using all essential test steps:

```cpp
#include "examples/test_steps/WaitStep.h"
#include "examples/test_steps/InstrumentMeasureStep.h"
#include "examples/test_steps/CalculationStep.h"
#include "examples/test_steps/LimitCheckStep.h"
#include "examples/test_steps/FileOperationStep.h"
#include <iostream>
#include <iomanip>

using namespace TestMATE;

int main() {
    std::cout << "=== Complete Multimeter Test Example ===" << std::endl;
    SStepResult result;
    bool allPassed = true;

    // Test Configuration
    const TDouble TARGET_VOLTAGE = 12.0;
    const TDouble TARGET_CURRENT = 2.5;
    const TDouble VOLTAGE_TOLERANCE = 0.5;  // ±0.5V
    const TDouble CURRENT_TOLERANCE = 0.1;  // ±0.1A

    // Simulated measurements (replace with real instrument measurements)
    TDouble voltage = 12.05;
    TDouble current = 2.48;

    std::cout << "\n--- Test Execution ---" << std::endl;

    // 1. Wait for system stabilization
    std::cout << "\n[Step 1] Power-up delay..." << std::endl;
    CWaitStep wait("WAIT-001", "Stabilization Delay", 1000);
    wait.Execute(result);
    std::cout << "  " << result.message << " (" << result.durationMs << "ms)" << std::endl;

    // 2. Measure voltage
    std::cout << "\n[Step 2] Voltage measurement..." << std::endl;
    std::cout << "  Measured: " << std::fixed << std::setprecision(3)
              << voltage << " V" << std::endl;

    // 3. Validate voltage
    std::cout << "\n[Step 3] Voltage validation..." << std::endl;
    CLimitCheckStep voltageCheck("LIMIT-001", "Validate Voltage");
    voltageCheck.SetValue(voltage);
    voltageCheck.SetLimits(TARGET_VOLTAGE - VOLTAGE_TOLERANCE,
                           TARGET_VOLTAGE + VOLTAGE_TOLERANCE);
    voltageCheck.SetUnit("V");
    voltageCheck.Execute(result);
    std::cout << "  " << result.message << std::endl;
    if (result.verdict != ETestVerdict::kPass) allPassed = false;

    // 4. Measure current
    std::cout << "\n[Step 4] Current measurement..." << std::endl;
    std::cout << "  Measured: " << std::fixed << std::setprecision(3)
              << current << " A" << std::endl;

    // 5. Validate current
    std::cout << "\n[Step 5] Current validation..." << std::endl;
    CLimitCheckStep currentCheck("LIMIT-002", "Validate Current");
    currentCheck.SetValue(current);
    currentCheck.SetLimits(TARGET_CURRENT - CURRENT_TOLERANCE,
                          TARGET_CURRENT + CURRENT_TOLERANCE);
    currentCheck.SetUnit("A");
    currentCheck.Execute(result);
    std::cout << "  " << result.message << std::endl;
    if (result.verdict != ETestVerdict::kPass) allPassed = false;

    // 6. Calculate power
    std::cout << "\n[Step 6] Power calculation..." << std::endl;
    CCalculationStep powerCalc("CALC-001", "Calculate Power");
    powerCalc.SetOperation(ECalculationType::kMultiply);
    std::map<TString, TDouble> operands;
    operands["a"] = voltage;
    operands["b"] = current;
    powerCalc.SetOperands(operands);
    powerCalc.SetUnit("W");
    powerCalc.Execute(result);
    TDouble power = powerCalc.GetResult();
    std::cout << "  Power = " << std::fixed << std::setprecision(2)
              << power << " W" << std::endl;

    // 7. Generate test report
    std::cout << "\n[Step 7] Generating report..." << std::endl;
    CFileOperationStep report("FILE-001", "Generate Report");
    report.SetOperation(EFileOperation::kWrite);
    report.SetFilePath("multimeter_test_report.txt");

    std::ostringstream reportContent;
    reportContent << "MULTIMETER TEST REPORT\n";
    reportContent << "=====================\n\n";
    reportContent << "Test Configuration:\n";
    reportContent << "  Target Voltage: " << TARGET_VOLTAGE << " V ±"
                  << VOLTAGE_TOLERANCE << " V\n";
    reportContent << "  Target Current: " << TARGET_CURRENT << " A ±"
                  << CURRENT_TOLERANCE << " A\n\n";
    reportContent << "Measurements:\n";
    reportContent << "  Voltage: " << std::fixed << std::setprecision(3)
                  << voltage << " V\n";
    reportContent << "  Current: " << std::fixed << std::setprecision(3)
                  << current << " A\n";
    reportContent << "  Power:   " << std::fixed << std::setprecision(2)
                  << power << " W\n\n";
    reportContent << "Overall Result: " << (allPassed ? "PASS" : "FAIL") << "\n";

    report.SetContent(reportContent.str());
    report.Execute(result);
    std::cout << "  Report saved to multimeter_test_report.txt" << std::endl;

    // Final summary
    std::cout << "\n--- Test Summary ---" << std::endl;
    std::cout << "Voltage: " << (voltage >= (TARGET_VOLTAGE - VOLTAGE_TOLERANCE) &&
                                  voltage <= (TARGET_VOLTAGE + VOLTAGE_TOLERANCE) ? "PASS" : "FAIL")
              << std::endl;
    std::cout << "Current: " << (current >= (TARGET_CURRENT - CURRENT_TOLERANCE) &&
                                  current <= (TARGET_CURRENT + CURRENT_TOLERANCE) ? "PASS" : "FAIL")
              << std::endl;
    std::cout << "\nFINAL VERDICT: " << (allPassed ? "✓ PASS" : "✗ FAIL") << std::endl;

    return allPassed ? 0 : 1;
}
```

### Build and Run

```bash
g++ -std=c++20 complete_example.cpp \
    -I/path/to/TestMATE/src \
    -I/path/to/TestMATE/examples \
    -L/path/to/TestMATE/build/lib \
    -ltestmate_core -ltestmate_utils -ltestmate_test_steps \
    -o complete_example

./complete_example
```

### Expected Output

```
=== Complete Multimeter Test Example ===

--- Test Execution ---

[Step 1] Power-up delay...
  Waited 1000 ms (1002ms)

[Step 2] Voltage measurement...
  Measured: 12.050 V

[Step 3] Voltage validation...
  PASS: 12.050000 V within range [11.500000 V, 12.500000 V]

[Step 4] Current measurement...
  Measured: 2.480 A

[Step 5] Current validation...
  PASS: 2.480000 A within range [2.400000 A, 2.600000 A]

[Step 6] Power calculation...
  Power = 29.88 W

[Step 7] Generating report...
  Report saved to multimeter_test_report.txt

--- Test Summary ---
Voltage: PASS
Current: PASS

FINAL VERDICT: ✓ PASS
```

---

## 🎨 Step 5: Using the Qt GUI (Optional)

If you have Qt installed, you can use the graphical interface.

### Build with Qt

```bash
# Install Qt5 or Qt6
sudo apt-get install qt5-default  # Ubuntu/Debian
# or
brew install qt@6  # macOS

# Configure with Qt
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt

# Build
cmake --build . -j4

# Run Qt application
./bin/testmate_qt
```

### GUI Features

- **Sequence Editor**: Visual test sequence creation
- **Execution Monitor**: Real-time test progress
- **Report Viewer**: HTML test reports
- **Configuration**: System settings

---

## 📚 Next Steps

### Learn More

1. **Test Steps Guide**: See `examples/test_steps/README.md`
   - All 6 essential steps with examples
   - Common formulas and calculations
   - SCPI command references

2. **Plugin Development**: See `examples/custom_plugin/README.md`
   - Create custom instrument drivers
   - Implement custom test steps
   - 80+ page comprehensive guide

3. **Test Sequences**: See `examples/basic_test_plan/README.md`
   - JSON and XML format details
   - Example sequences (simple, advanced, semiconductor)
   - Naming conventions and best practices

### Try These Examples

1. **Modify the voltage limits** in the example above
2. **Add more measurements** (temperature, frequency, etc.)
3. **Create a test sequence** in JSON format
4. **Connect real instruments** using InstrumentMeasureStep
5. **Build a custom plugin** for your hardware

### Get Help

- **Documentation**: See `docs/` directory
- **Examples**: See `examples/` directory
- **Issues**: Report bugs on GitHub
- **Source Code**: Fully documented with comments

---

## 🔧 Troubleshooting

### Build Errors

**Error**: `fatal error: testmate/common/Types.h: No such file or directory`
- **Solution**: Add `-I/path/to/TestMATE/src` to include path

**Error**: `undefined reference to TestMATE::CWaitStep::Execute`
- **Solution**: Link testmate_test_steps library: `-ltestmate_test_steps`

### Runtime Errors

**Error**: `Instrument not found`
- **Solution**: Register instrument with InstrumentManager before use

**Error**: `File not found`
- **Solution**: Use absolute paths or check current working directory

### Common Issues

**Tests fail with timeout**
- Increase timeout values in step parameters
- Check instrument connections

**Incorrect measurements**
- Verify instrument SCPI commands
- Check parameter units (V vs mV, A vs mA)

---

## ✅ Quick Reference

### Essential Test Steps

```cpp
// Wait/Delay
CWaitStep wait("ID", "Name", milliseconds);

// Limit Check
CLimitCheckStep limit("ID", "Name");
limit.SetValue(value);
limit.SetLimits(min, max);

// Calculation
CCalculationStep calc("ID", "Name");
calc.SetOperation(ECalculationType::kMultiply);

// Instrument Measurement
CInstrumentMeasureStep meas("ID", "Name");
meas.SetInstrument("DMM-001");
meas.SetCommand("MEAS:VOLT:DC?");

// File I/O
CFileOperationStep file("ID", "Name");
file.SetOperation(EFileOperation::kWrite);
file.SetFilePath("output.txt");

// Serial Communication
CSerialCommandStep serial("ID", "Name");
serial.SetPort("/dev/ttyUSB0");
serial.SetCommand("*IDN?");
```

### Common Patterns

**Measure and Validate**:
```cpp
// 1. Measure
TDouble voltage = GetMeasurement();

// 2. Validate
CLimitCheckStep check("LIMIT", "Check Voltage");
check.SetValue(voltage);
check.SetLimits(4.75, 5.25);
check.Execute(result);

// 3. Check result
if (result.verdict == ETestVerdict::kPass) {
    // Success
}
```

**Calculate Derived Parameter**:
```cpp
CCalculationStep calc("CALC", "Power");
calc.SetOperation(ECalculationType::kMultiply);
std::map<TString, TDouble> ops;
ops["a"] = voltage;
ops["b"] = current;
calc.SetOperands(ops);
calc.Execute(result);
TDouble power = calc.GetResult();
```

---

## 🎯 You're Ready!

You now know how to:
- ✅ Build TestMATE
- ✅ Create test programs using essential steps
- ✅ Validate measurements against limits
- ✅ Calculate derived parameters
- ✅ Log test results
- ✅ Load test sequences from JSON
- ✅ Use the Qt GUI (if available)

**Start building your automated tests today!** 🚀

For more examples, see the `examples/` directory in the TestMATE repository.
