# TestMATE Enhanced Software Requirements Specification
## Part 3H: Test Steps System

Version 2.0

---

## 37. Test Step Framework

### 37.1 Test Step Interface and Base Classes

#### 37.1.1 Core Test Step Interface

**REQ-TS-001:** All test steps SHALL implement the ITestStep interface.

**REQ-TS-002:** The ITestStep interface SHALL define methods:
- `PreRun()` - Setup before execution
- `Run()` - Main execution logic
- `PostRun()` - Cleanup after execution
- `GetName()` - Step name
- `GetDescription()` - Step description
- `GetProperties()` - Step properties/parameters
- `Validate()` - Validate configuration

**REQ-TS-003:** Test steps SHALL support hierarchical organization (parent-child relationships).

**REQ-TS-004:** Test steps SHALL support enable/disable state without removal from test plan.

**REQ-TS-005:** Test steps SHALL support execution conditionally based on runtime evaluation.

#### 37.1.2 Test Step Base Class

**REQ-TS-006:** The system SHALL provide a CTestStepBase class implementing common functionality.

**REQ-TS-007:** CTestStepBase SHALL handle:
- Property management
- Result publishing
- Timing measurements
- Error handling
- Logging integration
- Resource access

**REQ-TS-008:** Derived test steps SHALL only need to implement `RunImplementation()` method.

**REQ-TS-009:** Base class SHALL provide helper methods for common operations:
- Publishing measurements
- Accessing instruments by name
- Accessing DUTs by name
- Reading/writing variables
- Logging messages

### 37.2 Test Step Lifecycle

#### 37.2.1 Execution Phases

**REQ-TS-010:** Test step execution SHALL follow phases:
1. **Validation** - Check configuration validity
2. **Resource Allocation** - Lock required resources
3. **PreRun** - Setup and initialization
4. **Run** - Main execution
5. **PostRun** - Cleanup and deallocation
6. **Result Publishing** - Report results

**REQ-TS-011:** Each phase SHALL have configurable timeout.

**REQ-TS-012:** Phase failures SHALL be categorized:
- Validation failure → Step skipped with error
- PreRun failure → Step failed, PostRun still executes
- Run failure → Step failed, PostRun still executes
- PostRun failure → Step completed but cleanup failed (warning)

**REQ-TS-013:** The system SHALL log entry/exit of each phase with timestamps.

#### 37.2.2 Lifecycle Hooks

**REQ-TS-014:** Test steps SHALL support lifecycle hooks:
- `OnPreRunStart()` / `OnPreRunComplete()`
- `OnRunStart()` / `OnRunComplete()`
- `OnPostRunStart()` / `OnPostRunComplete()`
- `OnError()`
- `OnAbort()`

**REQ-TS-015:** Hooks SHALL be callable by test plan for global actions.

**REQ-TS-016:** Multiple hooks SHALL be chainable (executed in registration order).

**REQ-TS-017:** Hook exceptions SHALL be caught and logged but SHALL NOT abort execution.

### 37.3 Test Step Properties System

#### 37.3.1 Property Definition

**REQ-TS-018:** Test steps SHALL define properties using property system.

**REQ-TS-019:** Properties SHALL have attributes:
- Name (unique within step)
- Type (int, double, string, bool, enum, file path, etc.)
- Display name (user-friendly)
- Description / tooltip
- Default value
- Validation rules (min, max, regex, etc.)
- Category (for grouping in UI)
- Units (for numeric properties)

**REQ-TS-020:** Properties SHALL support data types:
- Primitive types (int, float, double, bool, string)
- Enumerations
- File/directory paths
- Arrays/lists
- Nested structures (complex types)
- References to other objects (instruments, DUTs)

**REQ-TS-021:** Properties SHALL support expressions and formulas:
- Reference other properties
- Reference test plan variables
- Mathematical expressions
- Conditional expressions

#### 37.3.2 Property Attributes

**REQ-TS-022:** Properties SHALL support attributes for UI generation:
- `[Display(Name="...", Description="...")]`
- `[Range(Min=..., Max=...)]`
- `[Unit("V", "A", "Hz", etc.)]`
- `[FilePath(Filter="*.txt")]`
- `[Required]`
- `[ReadOnly]`
- `[Advanced]` (hidden by default)
- `[Category("General", "Timing", etc.)]`

**REQ-TS-023:** Property validation SHALL occur:
- When property value changes (real-time)
- Before test execution
- During test plan loading

**REQ-TS-024:** Invalid properties SHALL:
- Display error indicators in UI
- Prevent test execution (if critical)
- Generate validation report

#### 37.3.3 Property Binding

**REQ-TS-025:** Properties SHALL support binding to:
- Test plan variables (read/write)
- Other step properties (read-only)
- Instrument properties (read-only)
- DUT properties (read-only)

**REQ-TS-026:** Bound properties SHALL update automatically when source changes.

**REQ-TS-027:** Circular bindings SHALL be detected and prevented.

**REQ-TS-028:** Property bindings SHALL be validated at test plan load time.

### 37.4 Test Step Parameters vs Properties

#### 37.4.1 Input Parameters

**REQ-TS-029:** Test steps SHALL support input parameters passed at runtime:
- From previous test steps (output → input)
- From test plan variables
- From operator prompts
- From external systems

**REQ-TS-030:** Input parameters SHALL have types and validation.

**REQ-TS-031:** Missing required input parameters SHALL fail test step.

**REQ-TS-032:** Input parameters SHALL support default values (optional parameters).

#### 37.4.2 Output Parameters

**REQ-TS-033:** Test steps SHALL produce output parameters:
- Measurement values
- Calculated values
- Status information
- Generated data

**REQ-TS-034:** Output parameters SHALL be strongly typed.

**REQ-TS-035:** Output parameters SHALL be accessible to subsequent test steps.

**REQ-TS-036:** Output parameters SHALL be stored in test results.

#### 37.4.3 Parameter Flow

**REQ-TS-037:** The system SHALL support parameter flow between test steps:
- Direct connection (step A output → step B input)
- Via test plan variables
- Via data tables

**REQ-TS-038:** Parameter flow SHALL be visualized in test plan editor.

**REQ-TS-039:** Type mismatches in parameter flow SHALL be detected at edit time.

**REQ-TS-040:** The system SHALL support type conversion where safe (int → double, etc.).

---

## 38. Standard Test Step Library

### 38.1 Fundamental Test Steps

#### 38.1.1 Basic Steps

**REQ-TS-041:** The system SHALL provide standard fundamental test steps:

1. **Pass Step**
   - Always passes
   - Used for placeholders or structure

2. **Fail Step**
   - Always fails
   - Used for testing error handling

3. **Message/Log Step**
   - Logs custom message
   - Properties: Message text, log level

4. **Comment Step**
   - Documentation only (not executed)
   - Properties: Comment text

5. **Placeholder Step**
   - Marks location for future implementation
   - Generates warning when executed

**REQ-TS-042:** Basic steps SHALL have minimal overhead (< 1ms execution time).

#### 38.1.2 Numeric Steps

**REQ-TS-043:** The system SHALL provide numeric test steps:

1. **Numeric Limit Test**
   - Compare value against limits
   - Properties: Value, lower limit, upper limit, comparison type
   - Verdict: Pass if within limits

2. **Numeric Range Test**
   - Test if value in specified range
   - Properties: Value, min, max, inclusive/exclusive

3. **Multiple Numeric Limit Test**
   - Test multiple values against respective limits
   - Properties: Array of values and limits

**REQ-TS-044:** Numeric steps SHALL support units and automatic unit conversion.

**REQ-TS-045:** Numeric steps SHALL handle special values (NaN, Infinity) appropriately.

#### 38.1.3 String Steps

**REQ-TS-046:** The system SHALL provide string test steps:

1. **String Compare**
   - Compare two strings
   - Properties: String1, String2, comparison type (exact, case-insensitive, regex)

2. **String Match**
   - Match string against pattern
   - Properties: String, pattern, pattern type (wildcard, regex)

3. **String Extract**
   - Extract substring
   - Properties: Source string, extraction method (regex, position)

**REQ-TS-047:** String steps SHALL support Unicode strings.

### 38.2 Flow Control Steps

#### 38.2.1 Conditional Steps

**REQ-TS-048:** The system SHALL provide conditional flow control steps:

1. **If/Then/Else Step**
   - Execute child steps conditionally
   - Properties: Condition expression
   - Children: Then branch, Else branch (optional)

2. **Switch/Case Step**
   - Multi-way branch based on value
   - Properties: Switch expression
   - Children: Case branches with values

**REQ-TS-049:** Condition expressions SHALL support:
- Comparison operators (==, !=, <, >, <=, >=)
- Logical operators (&&, ||, !)
- Property references
- Function calls

**REQ-TS-050:** Nested conditionals SHALL be supported (at least 10 levels deep).

**REQ-TS-051:** Unevaluated branches SHALL not consume execution time.

#### 38.2.2 Loop Steps

**REQ-TS-052:** The system SHALL provide loop steps:

1. **Repeat Step**
   - Execute children fixed number of times
   - Properties: Iteration count

2. **While Loop Step**
   - Execute children while condition true
   - Properties: Condition expression, max iterations (safety)

3. **For Each Step**
   - Iterate over collection
   - Properties: Collection, loop variable name

4. **Repeat Until Pass/Fail**
   - Retry until desired verdict
   - Properties: Max attempts, desired verdict

**REQ-TS-053:** Loop steps SHALL provide loop control:
- Break (exit loop early)
- Continue (skip to next iteration)
- Current iteration number accessible to children

**REQ-TS-054:** Infinite loops SHALL be prevented with max iteration safety limit.

**REQ-TS-055:** Loop steps SHALL aggregate child step results appropriately.

#### 38.2.3 Sequence Control Steps

**REQ-TS-056:** The system SHALL provide sequence control steps:

1. **Call Sequence Step**
   - Execute another test sequence
   - Properties: Sequence name/path, parameter mapping

2. **Goto/Label Steps**
   - Jump to labeled location (use cautiously)
   - Properties: Label name

3. **Return Step**
   - Early exit from sequence
   - Properties: Return value (optional)

**REQ-TS-057:** Recursive sequence calls SHALL be limited to prevent stack overflow.

**REQ-TS-058:** Sequence calls SHALL support parameter passing (in and out).

### 38.3 Timing and Delay Steps

#### 38.3.1 Delay Steps

**REQ-TS-059:** The system SHALL provide timing steps:

1. **Fixed Delay**
   - Wait for specified duration
   - Properties: Duration, time unit

2. **Wait Until**
   - Wait until condition becomes true
   - Properties: Condition expression, timeout, check interval

3. **Wait For Signal**
   - Wait for external signal/event
   - Properties: Signal name, timeout

**REQ-TS-060:** Delay steps SHALL support time units (ms, s, min, hr).

**REQ-TS-061:** Delay accuracy SHALL be within ±5ms for delays > 100ms.

**REQ-TS-062:** Long delays SHALL be interruptible (abort/pause).

#### 38.3.2 Timing Measurement Steps

**REQ-TS-063:** The system SHALL provide timing measurement steps:

1. **Start Timer**
   - Begin timing measurement
   - Properties: Timer name

2. **Stop Timer**
   - End timing measurement and record duration
   - Properties: Timer name, expected duration (optional), limits

3. **Measure Execution Time**
   - Measure duration of child steps
   - Properties: Child steps, limits

**REQ-TS-064:** Timing measurements SHALL use high-resolution timers (microsecond precision).

**REQ-TS-065:** Multiple simultaneous timers SHALL be supported (at least 100).

### 38.4 Data Manipulation Steps

#### 38.4.1 Variable Steps

**REQ-TS-066:** The system SHALL provide variable manipulation steps:

1. **Set Variable**
   - Assign value to variable
   - Properties: Variable name, value/expression

2. **Read Variable**
   - Read variable value
   - Properties: Variable name, output parameter

3. **Increment/Decrement Variable**
   - Modify numeric variable
   - Properties: Variable name, delta

4. **Clear Variable**
   - Remove variable from scope
   - Properties: Variable name

**REQ-TS-067:** Variables SHALL support scopes:
- Local (test step)
- Sequence (current sequence)
- Global (entire test plan)

**REQ-TS-068:** Variable type mismatches SHALL be detected and reported.

#### 38.4.2 Expression Evaluation Steps

**REQ-TS-069:** The system SHALL provide expression evaluation step:
- Evaluate mathematical or logical expressions
- Properties: Expression, output variable
- Support functions (sin, cos, log, sqrt, etc.)

**REQ-TS-070:** Expression evaluation SHALL support:
- Standard math operators (+, -, *, /, %, ^)
- Standard math functions
- String operations (concatenate, substring)
- Type conversions
- Conditional operator (? :)

#### 38.4.3 Array/List Steps

**REQ-TS-071:** The system SHALL provide array manipulation steps:

1. **Create Array**
   - Initialize array with values
   - Properties: Initial values

2. **Array Append**
   - Add element to array
   - Properties: Array name, value

3. **Array Get Element**
   - Retrieve element by index
   - Properties: Array name, index

4. **Array Length**
   - Get array size
   - Properties: Array name

**REQ-TS-072:** Arrays SHALL be dynamically sized.

**REQ-TS-073:** Array bounds violations SHALL generate errors.

### 38.5 File I/O Steps

#### 38.5.1 File Reading Steps

**REQ-TS-074:** The system SHALL provide file reading steps:

1. **Read Text File**
   - Read entire file as string
   - Properties: File path, encoding

2. **Read CSV File**
   - Parse CSV into array/table
   - Properties: File path, delimiter, header row

3. **Read Binary File**
   - Read file as binary data
   - Properties: File path

4. **Read Line**
   - Read single line from file
   - Properties: File path, line number

**REQ-TS-075:** File reading SHALL support encodings (UTF-8, UTF-16, ASCII, etc.).

**REQ-TS-076:** File not found SHALL be handled gracefully with clear error message.

**REQ-TS-077:** Large files SHALL be read efficiently (streaming, not all in memory).

#### 38.5.2 File Writing Steps

**REQ-TS-078:** The system SHALL provide file writing steps:

1. **Write Text File**
   - Write string to file
   - Properties: File path, content, mode (overwrite/append)

2. **Write CSV File**
   - Write array/table as CSV
   - Properties: File path, data, headers

3. **Write Binary File**
   - Write binary data to file
   - Properties: File path, data

**REQ-TS-079:** File writing SHALL support atomic operations (write to temp, then rename).

**REQ-TS-080:** File writing failures SHALL be reported with detailed error messages.

**REQ-TS-081:** File paths SHALL support environment variables and path expressions.

---

## 39. Instrument Control Steps

### 39.1 Generic Instrument Steps

#### 39.1.1 Connection Steps

**REQ-TS-082:** The system SHALL provide instrument connection steps:

1. **Connect Instrument**
   - Establish connection to instrument
   - Properties: Instrument name, connection parameters

2. **Disconnect Instrument**
   - Close instrument connection
   - Properties: Instrument name

3. **Reset Instrument**
   - Reset instrument to known state
   - Properties: Instrument name, reset type

**REQ-TS-083:** Connection steps SHALL validate instrument availability before execution.

**REQ-TS-084:** Connection failures SHALL provide diagnostic information (IP reachable?, driver loaded?, etc.).

#### 39.1.2 Command Steps

**REQ-TS-085:** The system SHALL provide generic command steps:

1. **Send SCPI Command**
   - Send SCPI command to instrument
   - Properties: Instrument name, command string

2. **Query SCPI**
   - Send query and read response
   - Properties: Instrument name, query string, output variable

3. **Write/Read**
   - Generic write/read operations
   - Properties: Instrument name, data, read length

**REQ-TS-086:** SCPI steps SHALL validate command syntax (optional).

**REQ-TS-087:** Command timeout SHALL be configurable per step.

**REQ-TS-088:** Command errors returned by instrument SHALL be captured and reported.

### 39.2 Specific Instrument Type Steps

#### 39.2.1 Digital Multimeter (DMM) Steps

**REQ-TS-089:** The system SHALL provide DMM-specific steps:

1. **Measure Voltage**
   - Properties: Instrument, range, resolution, measurement type (DC/AC)

2. **Measure Current**
   - Properties: Instrument, range, resolution, measurement type (DC/AC)

3. **Measure Resistance**
   - Properties: Instrument, range, 2-wire/4-wire

4. **Measure Continuity**
   - Properties: Instrument, threshold

**REQ-TS-090:** DMM steps SHALL auto-range unless range specified.

**REQ-TS-091:** Measurement results SHALL include value, unit, and timestamp.

#### 39.2.2 Oscilloscope Steps

**REQ-TS-092:** The system SHALL provide oscilloscope steps:

1. **Configure Oscilloscope**
   - Properties: Instrument, timebase, trigger, channels

2. **Capture Waveform**
   - Properties: Instrument, channel, number of points

3. **Measure Parameter**
   - Properties: Instrument, channel, parameter (amplitude, frequency, rise time, etc.)

4. **Save Waveform**
   - Properties: Instrument, file path, format

**REQ-TS-093:** Captured waveforms SHALL be accessible for analysis.

**REQ-TS-094:** Waveform measurements SHALL support standard parameters (amplitude, frequency, period, duty cycle, etc.).

#### 39.2.3 Power Supply Steps

**REQ-TS-095:** The system SHALL provide power supply steps:

1. **Set Voltage**
   - Properties: Instrument, channel, voltage, current limit

2. **Set Current**
   - Properties: Instrument, channel, current, voltage limit

3. **Output Enable/Disable**
   - Properties: Instrument, channel, enable state

4. **Measure Output**
   - Properties: Instrument, channel, measurement type (voltage/current)

**REQ-TS-096:** Power supply steps SHALL verify output reached setpoint before continuing.

**REQ-TS-097:** Overcurrent/overvoltage conditions SHALL be detected and reported.

#### 39.2.4 Signal Generator Steps

**REQ-TS-098:** The system SHALL provide signal generator steps:

1. **Generate Sine Wave**
   - Properties: Instrument, frequency, amplitude, offset

2. **Generate Square Wave**
   - Properties: Instrument, frequency, amplitude, duty cycle

3. **Generate Arbitrary Waveform**
   - Properties: Instrument, waveform data, sample rate

4. **Output Enable/Disable**
   - Properties: Instrument, enable state

**REQ-TS-099:** Signal generator steps SHALL support standard waveforms (sine, square, triangle, ramp, etc.).

#### 39.2.5 RF Instrument Steps

**REQ-TS-100:** The system SHALL provide RF instrument steps:

1. **Generate RF Signal**
   - Properties: Instrument, frequency, power, modulation

2. **Measure RF Power**
   - Properties: Instrument, frequency, averaging

3. **Analyze Spectrum**
   - Properties: Instrument, start freq, stop freq, resolution BW

4. **Measure EVM**
   - Properties: Instrument, standard (WiFi, Bluetooth, LTE, etc.)

**REQ-TS-101:** RF steps SHALL support common wireless standards configurations.

---

## 40. DUT Control Steps

### 40.1 DUT Connection Steps

#### 40.1.1 Basic DUT Steps

**REQ-TS-102:** The system SHALL provide DUT control steps:

1. **Connect DUT**
   - Establish connection to DUT
   - Properties: DUT name

2. **Disconnect DUT**
   - Close DUT connection
   - Properties: DUT name

3. **Reset DUT**
   - Reset DUT to known state
   - Properties: DUT name, reset type

4. **Power DUT**
   - Control DUT power
   - Properties: DUT name, power state (on/off)

**REQ-TS-103:** DUT steps SHALL validate DUT availability before execution.

**REQ-TS-104:** DUT connection failures SHALL provide detailed error information.

### 40.2 DUT Communication Steps

#### 40.2.1 Serial Communication

**REQ-TS-105:** The system SHALL provide serial communication steps:

1. **Send Serial Data**
   - Properties: DUT name, data, encoding

2. **Receive Serial Data**
   - Properties: DUT name, expected length, timeout

3. **Serial Command/Response**
   - Send command and wait for response
   - Properties: DUT name, command, expected response pattern

**REQ-TS-106:** Serial steps SHALL support common encodings (ASCII, hex, binary).

**REQ-TS-107:** Serial steps SHALL handle partial reads and timeouts appropriately.

#### 40.2.2 Network Communication

**REQ-TS-108:** The system SHALL provide network communication steps:

1. **Send TCP Data**
   - Properties: DUT name, data

2. **Send UDP Packet**
   - Properties: DUT name, data, destination

3. **HTTP Request**
   - Properties: DUT name, method (GET/POST/etc.), URL, headers, body

**REQ-TS-109:** Network steps SHALL support standard protocols (HTTP, HTTPS, TCP, UDP).

**REQ-TS-110:** Network errors SHALL be handled with automatic retry (configurable).

### 40.3 DUT-Specific Steps

#### 40.3.1 Firmware/Software Steps

**REQ-TS-111:** The system SHALL provide firmware control steps:

1. **Program Firmware**
   - Properties: DUT name, firmware file, programming method

2. **Verify Firmware**
   - Properties: DUT name, expected version/checksum

3. **Execute Command**
   - Run command on DUT (if DUT has CLI)
   - Properties: DUT name, command

**REQ-TS-112:** Firmware programming SHALL verify success before continuing.

**REQ-TS-113:** Firmware programming failures SHALL preserve logs for debugging.

---

## 41. Test Step Development

### 41.1 Custom Test Step Creation

#### 41.1.1 Development Process

**REQ-TS-114:** The system SHALL provide tools for creating custom test steps:
- Test step wizard
- Code templates
- Example implementations
- API documentation

**REQ-TS-115:** Custom test steps SHALL inherit from CTestStepBase.

**REQ-TS-116:** Custom test steps SHALL register properties using property system.

**REQ-TS-117:** Custom test steps SHALL be distributed as plugins.

#### 41.1.2 Test Step Templates

**REQ-TS-118:** The system SHALL provide test step templates for:
- Basic measurement step
- Instrument control step
- DUT communication step
- Data processing step
- Multi-step sequence

**REQ-TS-119:** Templates SHALL include:
- Complete source code structure
- Property definitions
- Resource usage examples
- Error handling patterns
- Documentation examples

**REQ-TS-120:** Templates SHALL follow TestMATE coding standards.

### 41.2 Test Step Debugging

#### 41.2.1 Debug Features

**REQ-TS-121:** Test steps SHALL support debugging features:
- Breakpoints
- Single-step execution
- Variable inspection
- Expression evaluation

**REQ-TS-122:** Debug mode SHALL be enable-able per test step or globally.

**REQ-TS-123:** Test step source code SHALL be accessible during debugging (if available).

**REQ-TS-124:** Debug information SHALL include:
- Current execution line
- Local variables
- Property values
- Call stack

#### 41.2.2 Logging and Diagnostics

**REQ-TS-125:** Test steps SHALL support diagnostic logging:
- Trace level (verbose execution details)
- Debug level (useful for debugging)
- Info level (informational messages)
- Warning level (potential issues)
- Error level (errors)

**REQ-TS-126:** Log messages SHALL include:
- Timestamp
- Test step name
- Log level
- Message

**REQ-TS-127:** Logging SHALL be performance-conscious (lazy evaluation of expensive operations).

### 41.3 Test Step Validation

#### 41.3.1 Static Validation

**REQ-TS-128:** Test steps SHALL validate configuration statically:
- Required properties set
- Property values in valid ranges
- Resource references valid
- Type compatibility

**REQ-TS-129:** Validation errors SHALL prevent test execution.

**REQ-TS-130:** Validation warnings SHALL allow execution with user confirmation.

**REQ-TS-131:** Validation SHALL occur:
- On property change (real-time in editor)
- On test plan save
- Before test execution

#### 41.3.2 Runtime Validation

**REQ-TS-132:** Test steps SHALL validate at runtime:
- Resources available
- Input parameters valid
- Preconditions met

**REQ-TS-133:** Runtime validation failures SHALL fail test step appropriately.

**REQ-TS-134:** Validation errors SHALL include detailed diagnostic information.

---

## 42. Test Step Documentation

### 42.1 Inline Documentation

#### 42.1.1 Property Documentation

**REQ-TS-135:** All test step properties SHALL have documentation including:
- Description
- Valid range/values
- Units (if applicable)
- Default value
- Example values

**REQ-TS-136:** Property documentation SHALL be displayed as tooltips in UI.

**REQ-TS-137:** Property documentation SHALL be included in API documentation.

#### 42.1.2 Step Documentation

**REQ-TS-138:** All test steps SHALL have documentation including:
- Purpose and description
- Usage instructions
- Property descriptions
- Examples
- Limitations and restrictions
- Related steps

**REQ-TS-139:** Step documentation SHALL be accessible from:
- Context menu in test plan editor
- Help system
- API documentation

### 42.2 Examples and Tutorials

#### 42.2.1 Example Library

**REQ-TS-140:** The system SHALL provide example test steps demonstrating:
- Basic operations
- Instrument control
- Data analysis
- Error handling
- Best practices

**REQ-TS-141:** Examples SHALL be complete and runnable.

**REQ-TS-142:** Examples SHALL include explanatory comments.

#### 42.2.2 Tutorial Content

**REQ-TS-143:** The system SHALL provide tutorials for:
- Creating first custom test step
- Integrating with instruments
- Advanced property usage
- Performance optimization
- Debugging techniques

**REQ-TS-144:** Tutorials SHALL include step-by-step instructions and screenshots.

---

## Document Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 2.0 | 2025-01-XX | TestMATE Team | Added comprehensive test steps requirements |

---

## Requirements Summary for Test Steps

| Category | Requirement Range | Count |
|----------|------------------|-------|
| Test Step Framework | REQ-TS-001 to REQ-TS-040 | 40 |
| Standard Test Step Library | REQ-TS-041 to REQ-TS-081 | 41 |
| Instrument Control Steps | REQ-TS-082 to REQ-TS-101 | 20 |
| DUT Control Steps | REQ-TS-102 to REQ-TS-113 | 12 |
| Test Step Development | REQ-TS-114 to REQ-TS-134 | 21 |
| Test Step Documentation | REQ-TS-135 to REQ-TS-144 | 10 |
| **TOTAL** | **144 Requirements** |

This brings the **complete TestMATE Enhanced SRS total to ~1,224 requirements**.
