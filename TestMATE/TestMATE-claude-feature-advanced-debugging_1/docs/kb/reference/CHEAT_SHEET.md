# TestMATE Quick Reference Cheat Sheet

**Fast reference for common TestMATE operations**

---

## Building TestMATE

```bash
# Standard build
mkdir build && cd build
cmake ..
cmake --build . -j4

# Run tests
./bin/testmate_unit_tests

# Build with Qt GUI
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build .
```

---

## Essential Test Steps

### WaitStep - Time Delay

```cpp
#include "examples/test_steps/WaitStep.h"

CWaitStep wait("WAIT-001", "Stabilization", 500);  // 500ms
SStepResult result;
wait.Execute(result);
```

### LimitCheckStep - Validation

```cpp
#include "examples/test_steps/LimitCheckStep.h"

CLimitCheckStep check("LIMIT-001", "Voltage Check");
check.SetValue(5.02);
check.SetLimits(4.75, 5.25);  // Min, Max
check.SetUnit("V");
check.Execute(result);

if (result.verdict == ETestVerdict::kPass) {
    // Passed
}
```

### CalculationStep - Math Operations

```cpp
#include "examples/test_steps/CalculationStep.h"

CCalculationStep calc("CALC-001", "Power");
calc.SetOperation(ECalculationType::kMultiply);
std::map<TString, TDouble> operands;
operands["a"] = voltage;
operands["b"] = current;
calc.SetOperands(operands);
calc.Execute(result);
TDouble power = calc.GetResult();
```

**Operations:** `kAdd`, `kSubtract`, `kMultiply`, `kDivide`, `kSquareRoot`, `kPower`, `kAbsolute`, `kMin`, `kMax`, `kAverage`

### InstrumentMeasureStep - SCPI Measurement

```cpp
#include "examples/test_steps/InstrumentMeasureStep.h"

CInstrumentMeasureStep meas("MEAS-001", "Voltage");
meas.SetInstrument("DMM-001");
meas.SetCommand("MEAS:VOLT:DC?");
meas.SetResultName("voltage");
meas.SetUnit("V");
meas.Execute(result);
```

### SerialCommandStep - Serial Communication

```cpp
#include "examples/test_steps/SerialCommandStep.h"

CSerialCommandStep serial("SERIAL-001", "Query ID");
serial.SetPort("/dev/ttyUSB0");
serial.SetBaudRate(9600);
serial.SetCommand("*IDN?");
serial.SetExpectedResponse("OK");
serial.Execute(result);
```

### FileOperationStep - File I/O

```cpp
#include "examples/test_steps/FileOperationStep.h"

// Write file
CFileOperationStep file("FILE-001", "Log Results");
file.SetOperation(EFileOperation::kWrite);
file.SetFilePath("results.txt");
file.SetContent("Test passed\n");
file.Execute(result);

// Read file
file.SetOperation(EFileOperation::kRead);
file.Execute(result);
TString content = file.GetContent();
```

**Operations:** `kRead`, `kWrite`, `kAppend`, `kDelete`, `kExists`

---

## Test Sequences

### Create Sequence (C++)

```cpp
#include "core/test_sequence/TestSequence.h"

CTestSequence sequence;
STestInfo info;
info.id = "TEST-001";
info.name = "My Test";
info.version = "1.0.0";
sequence.SetInfo(info);

// Add steps
sequence.AddStep(std::make_shared<CWaitStep>("WAIT-001", "Delay", 100));
sequence.AddStep(std::make_shared<CLimitCheckStep>("LIMIT-001", "Check"));

// Validate
auto result = sequence.Validate();
```

### Load Sequence (JSON)

```cpp
#include "core/test_sequence/SequenceFileIO.h"

auto& fileIO = CSequenceFileIO::GetInstance();
CTestSequence sequence;

auto result = fileIO.LoadSequence("test.json", sequence);
if (result.IsSuccess()) {
    std::cout << "Loaded: " << sequence.GetInfo().name << std::endl;
}
```

### Save Sequence (JSON)

```cpp
auto result = fileIO.SaveSequence("test.json", sequence);
```

---

## Process Models

### Sequential Execution

```cpp
#include "core/process_models/SequentialModel.h"

CSequentialModel model;
model.Initialize();
model.Execute(sequence);
```

### Parallel Execution (Multi-Socket)

```cpp
#include "core/process_models/ParallelModel.h"

CParallelModel model(4);  // 4 sockets
model.SetCallback(ECallbackType::kOnSocketComplete,
    [](TUInt32 socketId) {
        std::cout << "Socket " << socketId << " done\n";
    });
model.Initialize();
model.Execute(sequence);
```

### Batch Execution

```cpp
#include "core/process_models/BatchModel.h"

CBatchModel model(10);  // 10 devices
model.Initialize();
model.Execute(sequence);
```

---

## Database Operations

### Connect to Database

```cpp
#include "database/DataStoreFactory.h"

auto& factory = CDataStoreFactory::GetInstance();

// SQLite (default)
auto db = factory.CreateDataStore(EDatabaseType::kSqlite, "results.db");

// PostgreSQL
auto db = factory.CreateDataStore(
    EDatabaseType::kPostgreSql,
    "host=localhost dbname=testmate user=admin password=pass"
);

// MySQL
auto db = factory.CreateDataStore(
    EDatabaseType::kMySql,
    "host=localhost;database=testmate;user=admin;password=pass"
);
```

### Store Test Data

```cpp
STestDataRecord record;
record.testId = "PWR-001";
record.deviceId = "DUT-12345";
record.lotNumber = "LOT-789";
record.result = ETestVerdict::kPass;
record.startTime = "2025-11-23 10:00:00";
record.endTime = "2025-11-23 10:01:30";
record.testDuration = 90.0;

auto result = db->StoreTestData(record);
```

### Query Test Data

```cpp
// By lot number
auto results = db->GetTestDataByLot("LOT-789");

// By device ID
auto results = db->GetTestDataById("TEST-001", "DUT-12345");

// Custom query
TString sql = "SELECT * FROM test_data WHERE result = 'PASS' LIMIT 10";
auto queryResult = db->Query(sql);
```

---

## Configuration

### Get/Set Configuration

```cpp
#include "core/config/ConfigManager.h"

auto& config = CConfigManager::GetInstance();

// Set values
config.SetValue("system.timeout", "30");
config.SetValue("database.path", "/var/testmate/db");
config.SetValue("instruments.dmm.address", "GPIB::1");

// Get values
auto timeout = config.GetValue("system.timeout");  // Optional<TString>
if (timeout.has_value()) {
    int timeoutMs = std::stoi(*timeout);
}

// Get with default
TString dbPath = config.GetValueOr("database.path", "/tmp/test.db");
```

### Load/Save Configuration

```cpp
// Load from file
config.LoadFromFile("config.json");

// Save to file
config.SaveToFile("config.json");
```

---

## Error Handling

### CResult Pattern

```cpp
CResult<TDouble> MeasureVoltage() {
    if (!IsConnected()) {
        return CResult<TDouble>::Error(
            EErrorCode::kConnectionFailed,
            "DMM not connected"
        );
    }

    TDouble voltage = 5.0;
    return CResult<TDouble>::Success(voltage);
}

// Usage
auto result = MeasureVoltage();
if (result.IsSuccess()) {
    TDouble v = result.GetValue();
    std::cout << "Voltage: " << v << "V\n";
} else {
    std::cerr << "Error: " << result.GetMessage() << "\n";
}
```

### Error Codes

```cpp
enum class EErrorCode : TUInt32 {
    kSuccess = 0,
    kGenericError,
    kInvalidParameter,
    kNotFound,
    kAlreadyExists,
    kNotInitialized,
    kConnectionFailed,
    kTimeoutExpired,
    kResourceBusy,
    kInsufficientResources,
    kOperationFailed,
    kNotSupported,
    kDatabaseQueryFailed,
    kFileNotFound,
    kParseError,
    kValidationError
};
```

---

## Logging

```cpp
#include "utils/LogManager.h"

auto& log = CLogManager::GetInstance();

// Set log level
log.SetLogLevel(ELogLevel::kDebug);

// Log messages
log.LogDebug("Starting test sequence");
log.LogInfo("Test started: {}", testId);
log.LogWarning("Resource busy, retrying...");
log.LogError("Failed to connect to instrument");
log.LogFatal("Critical system error");

// Log to file
log.SetLogFile("testmate.log");
```

---

## Performance Profiling

```cpp
#include "utils/PerformanceProfiler.h"

auto& profiler = CPerformanceProfiler::GetInstance();

// Profile a scope
{
    PROFILE_SCOPE("MyOperation");
    // Code is automatically profiled
    DoExpensiveWork();
}

// Or manual profiling
profiler.StartMeasurement("CustomOp");
DoWork();
profiler.StopMeasurement("CustomOp");

// Generate report
profiler.GenerateHtmlReport("profile.html");
profiler.GenerateJsonReport("profile.json");

// Get statistics
auto stats = profiler.GetStatistics("MyOperation");
std::cout << "Avg: " << stats.averageMs << "ms\n";
std::cout << "Min: " << stats.minMs << "ms\n";
std::cout << "Max: " << stats.maxMs << "ms\n";
```

---

## Threading

### Thread Pool

```cpp
#include "core/threading/ThreadPool.h"

CThreadPool pool(4);  // 4 threads
pool.Start();

// Submit tasks
auto future = pool.Submit([]() {
    return DoWork();
});

// Wait for result
auto result = future.get();

pool.Stop();
```

### Synchronization Point

```cpp
#include "core/threading/SyncPoint.h"

CSyncPoint sync(4);  // 4 threads must sync

// In each thread:
DoIndependentWork();
sync.WaitFor(std::chrono::seconds(30));
DoSynchronizedWork();
```

---

## Common Patterns

### Measure and Validate

```cpp
// 1. Measure
TDouble voltage = 5.02;

// 2. Validate
CLimitCheckStep check("LIMIT", "Voltage");
check.SetValue(voltage);
check.SetLimits(4.75, 5.25);
check.Execute(result);

// 3. Check verdict
if (result.verdict == ETestVerdict::kPass) {
    std::cout << "PASS\n";
} else {
    std::cout << "FAIL: " << result.message << "\n";
}
```

### Calculate Derived Parameter

```cpp
// Inputs
TDouble voltage = 5.0;
TDouble current = 2.5;

// Calculate Power = V * I
CCalculationStep calc("CALC", "Power");
calc.SetOperation(ECalculationType::kMultiply);
std::map<TString, TDouble> ops;
ops["a"] = voltage;
ops["b"] = current;
calc.SetOperands(ops);
calc.Execute(result);

TDouble power = calc.GetResult();  // 12.5 W
```

### Instrument Workflow

```cpp
// 1. Wait for stabilization
CWaitStep wait("WAIT", "Stabilize", 500);
wait.Execute(result);

// 2. Measure
CInstrumentMeasureStep meas("MEAS", "Read Voltage");
meas.SetInstrument("DMM-001");
meas.SetCommand("MEAS:VOLT:DC?");
meas.Execute(result);

// 3. Validate
CLimitCheckStep check("LIMIT", "Check Voltage");
check.SetValue(result.measuredValue);
check.SetLimits(4.5, 5.5);
check.Execute(result);

// 4. Log result
CFileOperationStep log("LOG", "Save");
log.SetOperation(EFileOperation::kAppend);
log.SetFilePath("results.txt");
log.SetContent("Voltage: " + std::to_string(result.measuredValue) + "V\n");
log.Execute(result);
```

---

## JSON Sequence Format

```json
{
  "id": "TEST-001",
  "name": "Power Supply Test",
  "version": "1.0.0",
  "steps": [
    {
      "id": "WAIT-001",
      "name": "Stabilization",
      "type": "wait",
      "enabled": true,
      "parameters": {
        "duration_ms": "500"
      }
    },
    {
      "id": "MEAS-001",
      "name": "Measure Voltage",
      "type": "measurement",
      "enabled": true,
      "parameters": {
        "instrument_id": "DMM-001",
        "command": "MEAS:VOLT:DC?",
        "result_name": "voltage",
        "unit": "V"
      }
    },
    {
      "id": "LIMIT-001",
      "name": "Check Voltage",
      "type": "validation",
      "enabled": true,
      "parameters": {
        "value": "${MEAS-001.voltage}",
        "limit_min": "4.75",
        "limit_max": "5.25",
        "unit": "V"
      }
    }
  ]
}
```

---

## Type Aliases Quick Reference

```cpp
// Integer types
TInt32, TInt64, TUInt32, TUInt64

// Floating point
TFloat, TDouble

// String
TString, TStringView

// Time
TTimePoint, TDuration, TMicroseconds, TNanoseconds

// Containers
TVector<T>, TMap<K,V>, TOptional<T>

// Smart pointers
TUniquePtr<T>, TSharedPtr<T>, TWeakPtr<T>

// Callbacks
TCallback<Args...>, TVoidCallback, TBoolCallback
```

---

## Useful Enums

```cpp
// Verdicts
ETestVerdict::kPass
ETestVerdict::kFail
ETestVerdict::kError
ETestVerdict::kSkipped

// Process Model States
EProcessModelState::kIdle
EProcessModelState::kRunning
EProcessModelState::kPaused
EProcessModelState::kCompleted
EProcessModelState::kAborted
EProcessModelState::kError

// Calculation Operations
ECalculationType::kAdd, kSubtract, kMultiply, kDivide
ECalculationType::kSquareRoot, kPower, kAbsolute
ECalculationType::kMin, kMax, kAverage

// File Operations
EFileOperation::kRead, kWrite, kAppend, kDelete, kExists

// Limit Types
ELimitType::kBetween      // min <= value <= max
ELimitType::kGreaterThan  // value > min
ELimitType::kLessThan     // value < max
ELimitType::kEqual        // value == expected
ELimitType::kNotEqual     // value != expected
```

---

## Build Targets

```bash
# Libraries
testmate_utils       # Utilities
testmate_core        # Core engine
testmate_database    # Database layer
testmate_api         # API facade
testmate_ui          # UI adapter

# Applications
testmate_qt          # Qt GUI app (if Qt available)

# Tests
testmate_unit_tests         # Unit tests
testmate_integration_tests  # Integration tests
testmate_qt_tests          # Qt tests (if Qt available)

# Examples
testmate_test_steps        # Essential test steps library
simple_multimeter          # Example instrument plugin
custom_measurement_step    # Example step plugin
```

---

## File Locations

```
TestMATE/
├── include/            # Public headers
├── src/               # Implementation
│   ├── core/          # Core engine
│   ├── database/      # Database backends
│   ├── qt_app/        # Qt GUI
│   └── utils/         # Utilities
├── examples/          # Examples
│   ├── test_steps/    # Essential test steps
│   ├── custom_plugin/ # Plugin examples
│   └── database/      # Database tools
├── tests/             # Tests
├── docs/              # Documentation
└── build/             # Build output
    └── bin/           # Executables
```

---

## Quick Commands

```bash
# Build
cmake --build build -j4

# Run tests
build/bin/testmate_unit_tests

# Run specific test
build/bin/testmate_unit_tests --gtest_filter=ExecutionContextTest.*

# Clean build
rm -rf build && mkdir build && cd build && cmake .. && cmake --build .

# Install
sudo cmake --install build

# Generate documentation
cd docs && doxygen Doxyfile
```

---

**For more details, see:**
- [Quick Start Guide](../../QUICKSTART.md)
- [User Manual](../../USER_MANUAL.md)
- [API Reference](../api/CORE_API.md)
- [Examples](../../../examples/)

---

*Last updated: 2025-11-23 | TestMATE v2.0*
