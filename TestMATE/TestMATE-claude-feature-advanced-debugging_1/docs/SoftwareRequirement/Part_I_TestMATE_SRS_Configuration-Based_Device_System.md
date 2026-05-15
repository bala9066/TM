# TestMATE Enhanced Software Requirements Specification
## Part 3I: Configuration-Based Device System

Version 2.0

---

## 43. Configuration-Based Device Approach

### 43.1 Overview and Architecture

#### 43.1.1 Configuration-Based Philosophy

**REQ-CFG-DEV-001:** The system SHALL support configuration-based device definition for rapid device integration.

**REQ-CFG-DEV-002:** Configuration-based devices SHALL eliminate the need for custom C++ code for standard devices.

**REQ-CFG-DEV-003:** Device configuration SHALL be declarative (what, not how).

**REQ-CFG-DEV-004:** Configuration-based approach SHALL support:
- Instruments (DMM, oscilloscope, power supply, etc.)
- DUTs (any device under test)
- Custom hardware

**REQ-CFG-DEV-005:** The system SHALL provide both configuration-based and code-based device support simultaneously.

#### 43.1.2 Configuration File Format

**REQ-CFG-DEV-006:** Device configurations SHALL be stored as XML files.

**REQ-CFG-DEV-007:** XML configuration files SHALL include:
- Device metadata (type, model, manufacturer, version)
- Communication configuration
- Command definitions
- Capability definitions
- UI layout definition
- Default values

**REQ-CFG-DEV-008:** The system SHALL validate XML configuration files against XSD schema.

**REQ-CFG-DEV-009:** XML parsing errors SHALL be reported with:
- File name
- Line number
- Column number
- Error description
- Suggested correction

**REQ-CFG-DEV-010:** Configuration files SHALL support XML comments for documentation.

**REQ-CFG-DEV-011:** Configuration files SHALL support XML includes for reusable sections.

### 43.2 Device Metadata

#### 43.2.1 Basic Metadata

**REQ-CFG-DEV-012:** Device configuration SHALL include metadata section with:
- Device type (Instrument, DUT, Fixture, etc.)
- Model number
- Manufacturer name
- Hardware revision
- Firmware version compatibility
- Configuration version
- Author/maintainer
- Creation date
- Description

**REQ-CFG-DEV-013:** Device type SHALL determine base capabilities and interfaces.

**REQ-CFG-DEV-014:** Model number SHALL be unique identifier for device configuration.

**REQ-CFG-DEV-015:** Version information SHALL use semantic versioning (MAJOR.MINOR.PATCH).

#### 43.2.2 Metadata Validation

**REQ-CFG-DEV-016:** The system SHALL validate metadata completeness before loading configuration.

**REQ-CFG-DEV-017:** Required metadata fields SHALL include:
- DeviceType
- Model
- Manufacturer
- ConfigVersion

**REQ-CFG-DEV-018:** Optional metadata SHALL be supported for extended information.

**REQ-CFG-DEV-019:** Metadata SHALL be accessible via device API for display and logging.

### 43.3 Communication Configuration

#### 43.3.1 Communication Types

**REQ-CFG-DEV-020:** Configuration SHALL define communication type:
- Serial (RS-232, RS-485, RS-422)
- Ethernet (TCP, UDP, Raw Socket)
- USB (USBTMC, Virtual COM, HID)
- GPIB (IEEE-488)
- VISA (any VISA-supported interface)
- Custom (user-defined protocol)

**REQ-CFG-DEV-021:** Communication configuration SHALL include:
- Connection type
- Connection parameters (baud rate, IP address, etc.)
- Timeout values
- Retry configuration
- Error handling behavior

**REQ-CFG-DEV-022:** Serial communication SHALL configure:
- Port name/number
- Baud rate
- Data bits (5, 6, 7, 8)
- Parity (None, Even, Odd, Mark, Space)
- Stop bits (1, 1.5, 2)
- Flow control (None, Hardware, Software)
- Termination characters

**REQ-CFG-DEV-023:** Ethernet communication SHALL configure:
- Protocol (TCP, UDP, Raw)
- IP address
- Port number
- Connection timeout
- Keep-alive settings

**REQ-CFG-DEV-024:** VISA communication SHALL configure:
- Resource string
- Timeout
- Termination character
- Buffer size

#### 43.3.2 Communication Providers

**REQ-CFG-DEV-025:** The system SHALL instantiate appropriate communication provider based on configuration.

**REQ-CFG-DEV-026:** Communication providers SHALL implement ICommunicationProvider interface.

**REQ-CFG-DEV-027:** Provider factory SHALL support registration of custom providers.

**REQ-CFG-DEV-028:** Communication provider SHALL be testable independently of device logic.

### 43.4 Protocol Handler System

#### 43.4.1 Protocol Types

**REQ-CFG-DEV-029:** Configuration SHALL specify protocol type:
- SCPI (Standard Commands for Programmable Instruments)
- Modbus RTU
- Modbus TCP
- Custom Binary
- Custom Text
- JSON-based
- XML-based

**REQ-CFG-DEV-030:** SCPI protocol SHALL support:
- Command formatting
- Query/response pattern
- Error query (SYST:ERR?)
- Status register checking

**REQ-CFG-DEV-031:** Modbus protocol SHALL support:
- Function codes (read/write coils, registers)
- Slave address configuration
- Register mapping
- Data type conversions

**REQ-CFG-DEV-032:** Custom protocols SHALL be fully definable in configuration.

#### 43.4.2 Custom Binary Protocol Definition

**REQ-CFG-DEV-033:** Custom binary protocols SHALL define packet structure with:
- Header format
- Payload format
- Footer format
- Field definitions (name, type, position, length)
- Byte order (little-endian, big-endian)

**REQ-CFG-DEV-034:** Binary fields SHALL support types:
- uint8, uint16, uint32, uint64
- int8, int16, int32, int64
- float32, float64
- string (fixed or variable length)
- binary (raw bytes)
- arrays of above types

**REQ-CFG-DEV-035:** Field values SHALL support:
- Fixed values (constants in packet)
- Parameter references (from command parameters)
- Calculated values (expressions)
- Checksums (CRC, sum, XOR, etc.)

**REQ-CFG-DEV-036:** Binary protocol SHALL support:
- Bit fields (fields smaller than byte)
- Padding bytes
- Alignment requirements

#### 43.4.3 Custom Text Protocol Definition

**REQ-CFG-DEV-037:** Custom text protocols SHALL define:
- Command format template
- Response format pattern
- Field delimiters
- Escape sequences
- Line terminators

**REQ-CFG-DEV-038:** Text protocol SHALL support:
- String substitution (parameter insertion)
- Regular expressions for parsing
- Multi-line responses

#### 43.4.4 Protocol Functions

**REQ-CFG-DEV-039:** Protocol configuration SHALL support custom functions for:
- Checksum calculation
- Data encoding/decoding
- Value conversion
- Response parsing
- Error detection

**REQ-CFG-DEV-040:** Custom functions SHALL be defined as:
- JavaScript (for simple functions)
- Python scripts
- C++ plugins (for complex functions)

**REQ-CFG-DEV-041:** Function definitions SHALL include:
- Function name
- Input parameters
- Return type
- Implementation code

**REQ-CFG-DEV-042:** Common functions SHALL be provided as library:
- CRC-16, CRC-32 calculation
- Checksum calculation (sum, XOR)
- BCD encoding/decoding
- ASCII to binary conversion
- Hex string formatting

### 43.5 Command Definitions

#### 43.5.1 Command Structure

**REQ-CFG-DEV-043:** Configuration SHALL define all device commands with:
- Command name (logical identifier)
- Command parameters (inputs)
- Command implementation (how to send)
- Response parsing (how to read reply)
- Timeout
- Retry count

**REQ-CFG-DEV-044:** Command name SHALL be unique within device configuration.

**REQ-CFG-DEV-045:** Command parameters SHALL specify:
- Parameter name
- Parameter type
- Default value (optional)
- Validation rules (range, regex, etc.)

**REQ-CFG-DEV-046:** Command implementation SHALL specify protocol-specific details.

#### 43.5.2 SCPI Command Definition

**REQ-CFG-DEV-047:** SCPI commands SHALL define:
- SCPI command string (with parameter placeholders)
- Query flag (command vs query)
- Expected response format

**REQ-CFG-DEV-048:** SCPI command strings SHALL support parameter substitution using {paramName} syntax.

**REQ-CFG-DEV-049:** SCPI queries SHALL specify response parser:
- Data type (numeric, string, boolean)
- Units
- Format (decimal, hex, scientific notation)

#### 43.5.3 Binary Command Definition

**REQ-CFG-DEV-050:** Binary commands SHALL reference packet format definition.

**REQ-CFG-DEV-051:** Binary command SHALL map parameters to packet fields.

**REQ-CFG-DEV-052:** Binary response SHALL specify:
- Expected packet format
- Field extraction rules
- Value conversions

#### 43.5.4 Command Response Parsing

**REQ-CFG-DEV-053:** Response parsing SHALL support:
- Regular expression extraction
- Fixed position extraction
- Delimiter-based parsing
- JSON/XML parsing

**REQ-CFG-DEV-054:** Parsed responses SHALL be converted to appropriate data types.

**REQ-CFG-DEV-055:** Response parsing errors SHALL be handled gracefully with detailed error messages.

**REQ-CFG-DEV-056:** Response SHALL support multiple return values (tuple/struct).

### 43.6 Capability Definitions

#### 43.6.1 Capability Configuration

**REQ-CFG-DEV-057:** Configuration SHALL define device capabilities:
- Capability type (e.g., PowerSupplyCapability, OscilloscopeCapability)
- Capability parameters (configuration)
- Capability implementation (command mappings)

**REQ-CFG-DEV-058:** Capability type SHALL reference standard capability interfaces.

**REQ-CFG-DEV-059:** Capability implementation SHALL map capability methods to device commands.

**REQ-CFG-DEV-060:** Multiple capabilities SHALL be supported per device.

#### 43.6.2 Capability Method Mapping

**REQ-CFG-DEV-061:** Capability methods SHALL be mapped to device commands:
- Method name
- Target command
- Parameter mapping (method param → command param)
- Return value mapping

**REQ-CFG-DEV-062:** Parameter mapping SHALL support:
- Direct mapping (same name)
- Name translation (method param X → command param Y)
- Value transformation (units conversion, scaling)
- Constant insertion (fixed parameter values)

**REQ-CFG-DEV-063:** Return value mapping SHALL support:
- Direct return (command result)
- Field extraction (from structured response)
- Value transformation

**REQ-CFG-DEV-064:** Capability implementation SHALL support composite operations (multiple commands for one method).

### 43.7 Dynamic UI Generation

#### 43.7.1 UI Configuration

**REQ-CFG-DEV-065:** Configuration SHALL define user interface layout:
- Tabs (top-level grouping)
- Groups (within tabs)
- Controls (individual UI elements)

**REQ-CFG-DEV-066:** UI definition SHALL be hierarchical (tabs contain groups contain controls).

**REQ-CFG-DEV-067:** UI generation SHALL be automatic at device instantiation.

**REQ-CFG-DEV-068:** Generated UI SHALL be displayed in device control panel.

#### 43.7.2 Tab Definition

**REQ-CFG-DEV-069:** Tabs SHALL have:
- Name (displayed in UI)
- Description (tooltip)
- Icon (optional)
- Enabled condition (expression, optional)

**REQ-CFG-DEV-070:** Tabs SHALL organize related functionality.

**REQ-CFG-DEV-071:** Tabs SHALL be created dynamically based on configuration.

#### 43.7.3 Group Definition

**REQ-CFG-DEV-072:** Groups SHALL have:
- Name (displayed as group title)
- Description (tooltip)
- Layout (vertical, horizontal, grid)

**REQ-CFG-DEV-073:** Groups SHALL contain controls.

**REQ-CFG-DEV-074:** Group layout SHALL determine control arrangement.

#### 43.7.4 Control Types

**REQ-CFG-DEV-075:** UI configuration SHALL support control types:
- Label (static text)
- LineEdit (single-line text input)
- TextEdit (multi-line text input)
- SpinBox (integer input)
- DoubleSpinBox (floating-point input)
- ComboBox (dropdown selection)
- CheckBox (boolean)
- RadioButton (mutually exclusive selection)
- PushButton (action trigger)
- Slider (numeric range input)
- IPAddressEdit (IP address input)
- FilePathEdit (file/directory selection)
- LED (status indicator)
- ProgressBar (progress indication)

**REQ-CFG-DEV-076:** Each control type SHALL have specific configuration options.

#### 43.7.5 Control Properties

**REQ-CFG-DEV-077:** All controls SHALL support common properties:
- Name (unique identifier)
- Label (display text)
- Tooltip (help text)
- Enabled (expression-based)
- Visible (expression-based)

**REQ-CFG-DEV-078:** Input controls SHALL support additional properties:
- Default value
- Validation rules
- Min/max values
- Step size (for numeric)
- Read-only flag

**REQ-CFG-DEV-079:** Controls SHALL bind to device properties or commands.

#### 43.7.6 Control Binding

**REQ-CFG-DEV-080:** Controls SHALL support binding types:
- Property binding (read/write device property)
- Command binding (button triggers command)
- Status binding (LED shows device status)

**REQ-CFG-DEV-081:** Property binding SHALL support:
- Automatic updates (device → UI)
- Change propagation (UI → device)
- Validation before write

**REQ-CFG-DEV-082:** Command binding SHALL specify:
- Command to execute
- Parameter sources (from other controls)
- Success/failure handling

**REQ-CFG-DEV-083:** Binding SHALL handle type conversions automatically.

#### 43.7.7 Dynamic Behavior

**REQ-CFG-DEV-084:** UI elements SHALL support dynamic behavior:
- Show/hide based on conditions
- Enable/disable based on conditions
- Update ranges based on other values

**REQ-CFG-DEV-085:** Conditions SHALL be expressions referencing:
- Device properties
- Other control values
- Device state

**REQ-CFG-DEV-086:** Expressions SHALL be evaluated automatically when dependencies change.

### 43.8 Device Factory and Loading

#### 43.8.1 Configuration Discovery

**REQ-CFG-DEV-087:** The system SHALL discover device configuration files in:
- Application device configuration directory
- User device configuration directory
- Custom directories (configurable)

**REQ-CFG-DEV-088:** Configuration discovery SHALL occur at:
- Application startup
- On-demand refresh
- When directory contents change

**REQ-CFG-DEV-089:** Discovered configurations SHALL be indexed for quick lookup.

#### 43.8.2 Configuration Loading

**REQ-CFG-DEV-090:** Configuration loading SHALL:
- Parse XML file
- Validate against schema
- Check version compatibility
- Resolve includes/references
- Build internal representation

**REQ-CFG-DEV-091:** Loading errors SHALL be reported to user with:
- Configuration file path
- Error location
- Error description
- Suggested fixes

**REQ-CFG-DEV-092:** Successfully loaded configurations SHALL be cached for performance.

**REQ-CFG-DEV-093:** Configuration cache SHALL be invalidated when file changes.

#### 43.8.3 Device Instantiation

**REQ-CFG-DEV-094:** The device factory SHALL create device instances from configuration:
- Select configuration by model number
- Create communication provider
- Create protocol handler
- Initialize capabilities
- Generate UI
- Register with device manager

**REQ-CFG-DEV-095:** Instantiation SHALL validate:
- Required configuration sections present
- Communication provider available
- Protocol handler available

**REQ-CFG-DEV-096:** Instantiation failures SHALL rollback partial creation.

**REQ-CFG-DEV-097:** Multiple instances of same configuration SHALL be supported.

### 43.9 Configuration Inheritance and Templates

#### 43.9.1 Configuration Inheritance

**REQ-CFG-DEV-098:** Configurations SHALL support inheritance from base configurations.

**REQ-CFG-DEV-099:** Derived configuration SHALL:
- Inherit all base sections
- Override specific sections
- Add new sections

**REQ-CFG-DEV-100:** Inheritance SHALL be specified with base configuration reference.

**REQ-CFG-DEV-101:** Multiple inheritance levels SHALL be supported (at least 5 levels).

**REQ-CFG-DEV-102:** The system SHALL resolve inheritance chain at load time.

#### 43.9.2 Configuration Templates

**REQ-CFG-DEV-103:** The system SHALL provide configuration templates for common device types:
- SCPI instrument template
- Modbus device template
- Serial device template
- Network device template

**REQ-CFG-DEV-104:** Templates SHALL be parameterizable with placeholders.

**REQ-CFG-DEV-105:** Template instantiation SHALL replace placeholders with actual values.

**REQ-CFG-DEV-106:** Users SHALL be able to create custom templates.

### 43.10 Configuration Editor Tool

#### 43.10.1 Editor Features

**REQ-CFG-DEV-107:** The system SHALL provide configuration editor application.

**REQ-CFG-DEV-108:** Editor SHALL support:
- Syntax highlighting
- Auto-completion
- Schema validation (real-time)
- XML formatting
- Error highlighting

**REQ-CFG-DEV-109:** Editor SHALL have visual mode for:
- UI layout design (WYSIWYG)
- Command definition wizard
- Protocol format builder

**REQ-CFG-DEV-110:** Editor SHALL allow testing configuration without TestMATE.

#### 43.10.2 Configuration Testing

**REQ-CFG-DEV-111:** Editor SHALL support testing:
- Communication connectivity
- Command execution
- Response parsing
- UI generation

**REQ-CFG-DEV-112:** Test mode SHALL provide:
- Command terminal (send arbitrary commands)
- Response inspector
- Protocol analyzer
- UI preview

**REQ-CFG-DEV-113:** Test results SHALL be logged for debugging.

### 43.11 Configuration Validation

#### 43.11.1 Schema Validation

**REQ-CFG-DEV-114:** Configuration SHALL be validated against XSD schema.

**REQ-CFG-DEV-115:** Schema validation SHALL check:
- XML structure
- Required elements
- Element relationships
- Data types
- Value constraints

**REQ-CFG-DEV-116:** Schema validation errors SHALL be detailed and actionable.

#### 43.11.2 Semantic Validation

**REQ-CFG-DEV-117:** Configuration SHALL be validated semantically:
- Referenced commands exist
- Parameter types match
- Capability mappings valid
- UI bindings valid
- Expression syntax correct

**REQ-CFG-DEV-118:** Semantic validation SHALL occur after schema validation.

**REQ-CFG-DEV-119:** Semantic warnings SHALL not prevent loading (unless critical).

#### 43.11.3 Runtime Validation

**REQ-CFG-DEV-120:** Device SHALL validate at runtime:
- Communication connectivity
- Command responsiveness
- Expected data formats

**REQ-CFG-DEV-121:** Runtime validation SHALL be optional (for device discovery/testing).

### 43.12 Configuration Versioning and Migration

#### 43.12.1 Version Management

**REQ-CFG-DEV-122:** Configuration files SHALL include version information.

**REQ-CFG-DEV-123:** Version SHALL follow semantic versioning (MAJOR.MINOR.PATCH).

**REQ-CFG-DEV-124:** The system SHALL support multiple configuration versions simultaneously.

**REQ-CFG-DEV-125:** Version compatibility SHALL be checked at load time.

#### 43.12.2 Configuration Migration

**REQ-CFG-DEV-126:** The system SHALL provide migration tools for configuration updates.

**REQ-CFG-DEV-127:** Migration SHALL be automatic when possible.

**REQ-CFG-DEV-128:** Migration SHALL preserve custom modifications where possible.

**REQ-CFG-DEV-129:** Migration failures SHALL provide rollback capability.

### 43.13 Performance Considerations

#### 43.13.1 Configuration Caching

**REQ-CFG-DEV-130:** Parsed configurations SHALL be cached in memory.

**REQ-CFG-DEV-131:** Cache SHALL be shared across device instances of same type.

**REQ-CFG-DEV-132:** Cache invalidation SHALL occur when configuration file changes.

#### 43.13.2 Instantiation Performance

**REQ-CFG-DEV-133:** Device instantiation from configuration SHALL complete in < 100ms.

**REQ-CFG-DEV-134:** UI generation SHALL be deferred until actually displayed (lazy loading).

**REQ-CFG-DEV-135:** Command lookup SHALL use hash tables for O(1) access.

### 43.14 Configuration Security

#### 43.14.1 Configuration Integrity

**REQ-CFG-DEV-136:** Configuration files SHALL support digital signatures.

**REQ-CFG-DEV-137:** Signature verification SHALL be optional but recommended for production.

**REQ-CFG-DEV-138:** Unsigned configurations SHALL generate warnings in secure mode.

#### 43.14.2 Configuration Access Control

**REQ-CFG-DEV-139:** Configuration file access SHALL respect file system permissions.

**REQ-CFG-DEV-140:** The system SHALL not execute arbitrary code from configurations (sandbox).

**REQ-CFG-DEV-141:** Custom functions SHALL be sandboxed to prevent system access.

---

## Document Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 2.0 | 2025-01-XX | TestMATE Team | Added configuration-based device system |

---

## Requirements Summary

| Category | Requirement Range | Count |
|----------|------------------|-------|
| Overview & Architecture | REQ-CFG-DEV-001 to REQ-CFG-DEV-011 | 11 |
| Device Metadata | REQ-CFG-DEV-012 to REQ-CFG-DEV-019 | 8 |
| Communication Configuration | REQ-CFG-DEV-020 to REQ-CFG-DEV-028 | 9 |
| Protocol Handler System | REQ-CFG-DEV-029 to REQ-CFG-DEV-042 | 14 |
| Command Definitions | REQ-CFG-DEV-043 to REQ-CFG-DEV-056 | 14 |
| Capability Definitions | REQ-CFG-DEV-057 to REQ-CFG-DEV-064 | 8 |
| Dynamic UI Generation | REQ-CFG-DEV-065 to REQ-CFG-DEV-086 | 22 |
| Device Factory & Loading | REQ-CFG-DEV-087 to REQ-CFG-DEV-097 | 11 |
| Inheritance & Templates | REQ-CFG-DEV-098 to REQ-CFG-DEV-106 | 9 |
| Configuration Editor | REQ-CFG-DEV-107 to REQ-CFG-DEV-113 | 7 |
| Configuration Validation | REQ-CFG-DEV-114 to REQ-CFG-DEV-121 | 8 |
| Versioning & Migration | REQ-CFG-DEV-122 to REQ-CFG-DEV-129 | 8 |
| Performance | REQ-CFG-DEV-130 to REQ-CFG-DEV-135 | 6 |
| Security | REQ-CFG-DEV-136 to REQ-CFG-DEV-141 | 6 |
| **TOTAL** | **141 Requirements** |
