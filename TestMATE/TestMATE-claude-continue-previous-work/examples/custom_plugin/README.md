# TestMATE Custom Plugin Examples

This directory contains example plugins demonstrating how to extend TestMATE with custom functionality.

## Example Plugins

### 1. SimpleMultimeter (Instrument Plugin)
**Type**: `IInstrumentPlugin`
**Location**: `SimpleMultimeter/`
**Purpose**: Demonstrates how to create a hardware instrument driver plugin

**Features**:
- DC voltage measurement
- DC current measurement
- Resistance measurement
- SCPI command interface
- Simulated mode for testing without hardware
- Thread-safe implementation

**Use Cases**:
- Creating drivers for custom test equipment
- Integrating third-party instruments
- Simulating instruments for test development

### 2. CustomMeasurementStep (Test Step Plugin)
**Type**: `ITestStepPlugin`
**Location**: `CustomMeasurementStep/`
**Purpose**: Demonstrates how to create custom test step logic

**Features**:
- Custom calculation formulas
- Parameter-based configuration
- Limit checking
- Pass/Fail verdict generation

**Use Cases**:
- Complex mathematical calculations (power, efficiency, etc.)
- Custom validation logic
- Derived parameter measurements

## Plugin Architecture

### Plugin Types

TestMATE supports several plugin types:

| Type | Interface | Purpose |
|------|-----------|---------|
| `kInstrument` | `IInstrumentPlugin` | Hardware instrument drivers |
| `kTestStep` | `ITestStepPlugin` | Custom test step logic |
| `kReporter` | `IReporterPlugin` | Custom report formats |
| `kDataSource` | N/A | Data providers |
| `kProtocol` | N/A | Communication protocols |
| `kExtension` | N/A | Generic extensions |

### Plugin Lifecycle

```
1. Load (Dynamic library loading)
   ↓
2. Create (CreatePlugin() factory function)
   ↓
3. Initialize (Plugin::Initialize())
   ↓
4. Active Use (Connect, Execute, etc.)
   ↓
5. Shutdown (Plugin::Shutdown())
   ↓
6. Destroy (DestroyPlugin())
   ↓
7. Unload (Library unloaded)
```

## Building Plugins

### Prerequisites

- CMake 3.15+
- C++20 compatible compiler
- TestMATE development headers
- TestMATE core libraries

### Build Instructions

#### Option 1: Build with TestMATE

Add to main CMakeLists.txt:
```cmake
# Build example plugins
add_subdirectory(examples/custom_plugin/SimpleMultimeter)
add_subdirectory(examples/custom_plugin/CustomMeasurementStep)
```

Then build normally:
```bash
cd /path/to/TestMATE
cmake -B build
cmake --build build
```

Plugins will be output to: `build/plugins/`

#### Option 2: Standalone Build

```bash
cd examples/custom_plugin/SimpleMultimeter
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/TestMATE/build
cmake --build build
```

### Plugin Output

**Linux**: `libSimpleMultimeter.so`, `libCustomMeasurementStep.so`
**macOS**: `libSimpleMultimeter.dylib`, `libCustomMeasurementStep.dylib`
**Windows**: `SimpleMultimeter.dll`, `CustomMeasurementStep.dll`

## Using Plugins

### Loading Plugins Programmatically

```cpp
#include "core/plugins/PluginManager.h"

// Get plugin manager instance
auto& pluginMgr = TestMATE::CPluginManager::GetInstance();

// Add plugin search path
pluginMgr.AddSearchPath("./plugins");

// Load specific plugin
auto result = pluginMgr.LoadPlugin("./plugins/libSimpleMultimeter.so");
if (result.IsSuccess()) {
    std::cout << "Plugin loaded successfully" << std::endl;
}

// Or scan directory for all plugins
TUInt32 count = pluginMgr.ScanDirectory("./plugins");
std::cout << "Found " << count << " plugins" << std::endl;
```

### Using Instrument Plugin

```cpp
// Get plugin instance
auto* pPlugin = pluginMgr.GetPlugin("simple-multimeter");
auto* pMultimeter = dynamic_cast<IInstrumentPlugin*>(pPlugin);

if (pMultimeter) {
    // Connect to instrument
    pMultimeter->Connect("SIMULATED");  // or "GPIB::5" for real hardware

    // Query voltage
    TString response;
    pMultimeter->Query("MEAS:VOLT:DC?", response, 5000);
    std::cout << "Voltage: " << response << " V" << std::endl;

    // Disconnect
    pMultimeter->Disconnect();
}
```

### Using Test Step Plugin

```cpp
// Get plugin instance
auto* pPlugin = pluginMgr.GetPlugin("custom-measurement-step");
auto* pTestStep = dynamic_cast<ITestStepPlugin*>(pPlugin);

if (pTestStep) {
    // Set up parameters
    std::map<TString, TString> params;
    params["formula"] = "voltage * current";
    params["voltage"] = "12.0";
    params["current"] = "2.5";
    params["limit_min"] = "25.0";
    params["limit_max"] = "35.0";

    // Execute test step
    std::map<TString, TString> results;
    ETestVerdict verdict = pTestStep->Execute(params, results);

    std::cout << "Measured Power: " << results["measured_value"] << " W" << std::endl;
    std::cout << "Verdict: " << results["verdict"] << std::endl;
}
```

## Creating Your Own Plugins

### Step 1: Choose Plugin Type

Decide which interface your plugin needs to implement:
- `IInstrumentPlugin` - For hardware drivers
- `ITestStepPlugin` - For custom test logic
- `IReporterPlugin` - For custom reports
- `IPlugin` - Base interface (for other types)

### Step 2: Implement Required Methods

All plugins must implement:
```cpp
class MyPlugin : public IXxxPlugin {
public:
    // IPlugin interface
    SPluginInfo GetInfo() const override;
    CResult Initialize() override;
    CResult Shutdown() override;
    EPluginState GetState() const override;
    TString GetLastError() const override;

    // Type-specific interface methods
    // ...
};
```

### Step 3: Export Plugin

Use the macro to export your plugin:
```cpp
TESTMATE_DECLARE_PLUGIN(MyNamespace::MyPlugin)
```

This creates the required factory functions:
- `CreatePlugin()` - Instantiate your plugin
- `DestroyPlugin()` - Clean up your plugin
- `GetPluginApiVersion()` - Return API version

### Step 4: Build as Shared Library

Create CMakeLists.txt:
```cmake
add_library(MyPlugin SHARED
    MyPlugin.cpp
    MyPlugin.h
)

target_link_libraries(MyPlugin
    PRIVATE testmate_core
)
```

## Plugin Best Practices

### 1. Thread Safety
- Use mutexes for shared state
- All plugins should be thread-safe
- Example pattern:
```cpp
CResult MyPlugin::SomeMethod() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Your code here
}
```

### 2. Error Handling
- Always return `CResult` for operations that can fail
- Store last error message for debugging
- Use appropriate error codes

### 3. Resource Management
- Clean up in `Shutdown()` method
- Use RAII for resource handling
- Check connection state before operations

### 4. Plugin Metadata
- Provide accurate version information
- Document dependencies
- Specify minimum host version compatibility

### 5. Testing
- Create unit tests for your plugin
- Test simulated mode thoroughly
- Verify error handling paths

## Advanced Topics

### Custom SCPI Commands

For instrument plugins, implement standard SCPI commands:

```cpp
CResult ProcessSCPICommand(const TString& cmd, TString& response) {
    if (cmd == "*IDN?") {
        response = "Manufacturer,Model,Serial,Version";
    } else if (cmd == "*RST") {
        // Reset instrument
    } else if (cmd.find("MEAS:") == 0) {
        // Handle measurement commands
    }
    // ...
}
```

### State Management

Track plugin state correctly:

```cpp
enum class EPluginState {
    kUnloaded,      // Before Initialize()
    kInitialized,   // After Initialize()
    kActive,        // During operation (e.g., connected)
    kError          // Error occurred
};
```

### Parameter Validation

For test step plugins, validate parameters:

```cpp
ETestVerdict Execute(const std::map<TString, TString>& params,
                    std::map<TString, TString>& results) {
    // Check required parameters
    if (params.find("voltage") == params.end()) {
        return ETestVerdict::kError;
    }

    // Validate parameter ranges
    double voltage = std::stod(params.at("voltage"));
    if (voltage < 0.0 || voltage > 1000.0) {
        return ETestVerdict::kError;
    }

    // Execute logic
    // ...
}
```

## Troubleshooting

### Plugin Won't Load

**Problem**: `LoadPlugin()` fails
**Solutions**:
1. Check that library file exists and has correct extension
2. Verify all TestMATE dependencies are available
3. Check for symbol conflicts with TESTMATE_DECLARE_PLUGIN macro
4. Review error logs for missing symbols

### Plugin Crashes

**Problem**: Segmentation fault or crash during use
**Solutions**:
1. Ensure thread safety (add mutexes)
2. Check for null pointers before dereferencing
3. Validate all parameters
4. Use smart pointers where possible

### Build Errors

**Problem**: Compilation or linking fails
**Solutions**:
1. Verify C++20 support in compiler
2. Check TestMATE headers are in include path
3. Link against required TestMATE libraries
4. Match plugin ABI with TestMATE build configuration

## See Also

- [Plugin Manager API Documentation](../../docs/plugins/plugin_manager.md)
- [Instrument Interface Reference](../../docs/instruments/instrument_interface.md)
- [Test Step Development Guide](../../docs/test_steps/custom_steps.md)
- [Example Test Sequences](../basic_test_plan/)

---

**Note**: These are example/template plugins. For production use, implement proper error handling, hardware communication, and comprehensive testing.
