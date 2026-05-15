# TestMATE Example Test Sequences

This directory contains example test sequences demonstrating the TestMATE sequence file formats.

## File Formats

TestMATE supports two file formats for test sequences:
- **JSON** (`.json`) - JavaScript Object Notation format
- **XML** (`.xml`) - eXtensible Markup Language format

## Example Files

### 1. simple_test.json
**Purpose**: Basic test sequence demonstrating fundamental test flow
**Complexity**: Beginner
**Steps**: 8 steps covering initialization, power control, measurements, validation, and cleanup

**Use Cases**:
- Learning TestMATE sequence structure
- Quick functional verification tests
- Template for simple test flows

**Key Features**:
- All four step types (action, measurement, validation, wait)
- Linear test flow
- Basic device power-up and test pattern

---

### 2. advanced_test.json
**Purpose**: Comprehensive multi-phase test sequence with real-world complexity
**Complexity**: Intermediate to Advanced
**Steps**: 36 steps organized into 7 test phases

**Test Phases**:
1. **Initialization** (4 steps) - System setup and DUT connection
2. **Calibration** (4 steps) - Measurement calibration and offset adjustment
3. **Power Sequencing** (8 steps) - Multi-rail power-up with validation
4. **Functional Testing** (6 steps) - Device ID verification and self-test
5. **Performance Testing** (5 steps) - Load testing and timing measurements
6. **Stress Testing** (5 steps) - Maximum load conditions with thermal monitoring
7. **Cleanup** (4 steps) - Safe power-down and report generation

**Use Cases**:
- Production test flows
- Qualification testing
- Reliability testing
- Multi-phase test scenarios

**Key Features**:
- Organized step naming (phase prefixes: INIT, CAL, PWR, FUNC, PERF, STRESS, CLEAN)
- Multiple power rails (3.3V, 1.8V)
- Timing constraints (wait steps)
- Thermal monitoring
- Comprehensive validation throughout

---

### 3. basic_test.xml
**Purpose**: Demonstrates XML format for test sequences
**Complexity**: Beginner
**Steps**: 15 steps covering complete test cycle

**Use Cases**:
- XML-based test management systems integration
- Legacy system compatibility
- Human-readable test documentation

**Key Features**:
- XML syntax with attributes
- Step metadata (id, name, type)
- Hierarchical structure
- Self-documenting format

---

## File Format Details

### JSON Format Structure

```json
{
  "id": "SEQUENCE-ID",
  "name": "Sequence Name",
  "version": "1.0.0",
  "steps": [
    {
      "id": "STEP-001",
      "name": "Step Description",
      "type": "action|measurement|validation|wait",
      "enabled": true
    }
  ]
}
```

**Required Fields**:
- `id` - Unique sequence identifier
- `name` - Human-readable sequence name
- `version` - Sequence version (semantic versioning recommended)
- `steps` - Array of test steps

**Step Fields**:
- `id` - Unique step identifier (within sequence)
- `name` - Step description
- `type` - Step type (see below)
- `enabled` - Boolean flag (true/false)

### XML Format Structure

```xml
<?xml version="1.0" encoding="UTF-8"?>
<sequence id="SEQUENCE-ID">
  <name>Sequence Name</name>
  <version>1.0.0</version>
  <description>Optional description</description>

  <steps>
    <step id="STEP-001" name="Step Description" type="action" />
  </steps>
</sequence>
```

## Step Types

TestMATE supports four fundamental step types:

| Type | Purpose | Example |
|------|---------|---------|
| **action** | Perform an operation or command | Power on device, configure settings, reset |
| **measurement** | Acquire data from DUT or instruments | Measure voltage, read temperature, capture waveform |
| **validation** | Verify measurement against limits | Check voltage in range, validate device ID |
| **wait** | Delay or synchronization | Wait for stabilization, delay 100ms |

## Naming Conventions

### Sequence IDs
- Use project/category prefix: `SIMPLE-001`, `ADV-001`, `SEMI-PARAM-001`
- Include version or variant if applicable
- Keep concise but meaningful

### Step IDs
Recommended patterns:
- **Simple sequences**: `STEP-001`, `STEP-002`, ... (sequential)
- **Complex sequences**: `PHASE-001`, `INIT-001`, `CAL-001`, ... (phase-prefixed)
- **Parametric tests**: `VTH-001`, `RDSON-001`, ... (parameter-prefixed)

### Step Names
- Use action verbs: "Measure", "Validate", "Configure", "Wait"
- Be specific: "Measure Supply Voltage" not "Measure"
- Include units/values where relevant: "Wait 100ms", "Validate < 10nA"

## Loading Sequences in TestMATE

### Using C++ API

```cpp
#include "core/test_sequence/SequenceFileIO.h"
#include "core/test_sequence/TestSequence.h"

// Get the file I/O singleton
auto& fileIO = TestMATE::CSequenceFileIO::GetInstance();

// Load a sequence (auto-detects format from extension)
TestMATE::CTestSequence sequence;
auto result = fileIO.LoadSequence("examples/basic_test_plan/simple_test.json", sequence);

if (result.IsSuccess()) {
    // Sequence loaded successfully
    const auto& info = sequence.GetInfo();
    std::cout << "Loaded: " << info.name << " v" << info.version << std::endl;
    std::cout << "Steps: " << sequence.GetStepCount() << std::endl;
}
```

### Explicit Format Specification

```cpp
// Force JSON parsing
fileIO.LoadSequence("sequence.txt", sequence, TestMATE::ESequenceFileFormat::kJson);

// Force XML parsing
fileIO.LoadSequence("sequence.txt", sequence, TestMATE::ESequenceFileFormat::kXml);
```

## Extending Examples

These examples provide a foundation. You can extend them by:

1. **Adding custom parameters** - Store test limits, instrument settings, etc.
2. **Implementing custom step types** - Create plugin steps for your specific needs
3. **Organizing into suites** - Group related sequences for comprehensive testing
4. **Version control** - Track sequence changes alongside test data

## See Also

- [Semiconductor Test Examples](../semiconductor/) - Advanced parametric testing
- [Custom Plugin Examples](../custom_plugin/) - Creating custom test steps
- [TestMATE Documentation](../../docs/) - Complete API reference

---

**Note**: The sequence files in this directory use the default `CLoadedTestStep` class, which provides basic execution. For production use, implement custom step classes that interact with your actual test hardware and instruments.
