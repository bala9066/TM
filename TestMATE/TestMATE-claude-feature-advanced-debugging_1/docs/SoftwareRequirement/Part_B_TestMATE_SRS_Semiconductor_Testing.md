# TestMATE Enhanced Software Requirements Specification
## Part 3B: Semiconductor Testing Features

Version 2.0

---

## 6. Pin Mapping System

### 6.1 Pin Map Data Model

#### 6.1.1 Pin Definition

**REQ-PIN-001:** The system SHALL support definition of pins with attributes:
- Pin name (unique identifier)
- Pin type (Digital I/O, Analog, Power, Ground, RF, Differential)
- Direction (Input, Output, Bidirectional, Power)
- Electrical specifications (voltage range, current limits)
- Description

**REQ-PIN-002:** Pin names SHALL be unique within a pin map.

**REQ-PIN-003:** Pins SHALL support grouping into logical groups (e.g., data bus, address bus).

**REQ-PIN-004:** Pin groups SHALL support hierarchical organization.

**REQ-PIN-005:** The system SHALL validate pin electrical specifications against instrument capabilities.

#### 6.1.2 Channel Definition

**REQ-PIN-006:** The system SHALL support instrument channel definitions with attributes:
- Instrument name
- Channel number/identifier
- Channel type (matching pin types)
- Physical connector information
- Calibration data

**REQ-PIN-007:** Channels SHALL be associated with specific instrument instances.

**REQ-PIN-008:** The system SHALL validate channel availability before mapping.

**REQ-PIN-009:** Channels SHALL support multiplexed configurations.

#### 6.1.3 Pin-to-Channel Mapping

**REQ-PIN-010:** The system SHALL support mapping pins to instrument channels.

**REQ-PIN-011:** Each pin SHALL map to exactly one channel per site.

**REQ-PIN-012:** Multiple pins MAY map to the same channel if multiplexing is supported.

**REQ-PIN-013:** The system SHALL validate electrical compatibility between pins and channels.

**REQ-PIN-014:** Pin-to-channel mappings SHALL support site-specific overrides.

### 6.2 Pin Map Editor

#### 6.2.1 Graphical Interface

**REQ-PIN-015:** The system SHALL provide a graphical pin map editor.

**REQ-PIN-016:** The editor SHALL display:
- Pin list (filterable and sortable)
- Channel list (per instrument)
- Mapping connections
- Site configurations

**REQ-PIN-017:** The editor SHALL support drag-and-drop pin-to-channel mapping.

**REQ-PIN-018:** The editor SHALL highlight mapping conflicts and errors.

**REQ-PIN-019:** The editor SHALL support zoom and pan for large pin maps (1000+ pins).

#### 6.2.2 Editing Operations

**REQ-PIN-020:** The editor SHALL support operations:
- Add/remove pins
- Add/remove channels
- Create/modify mappings
- Define pin groups
- Configure sites
- Set electrical specifications

**REQ-PIN-021:** The editor SHALL provide undo/redo functionality (minimum 50 operations).

**REQ-PIN-022:** The editor SHALL support copy/paste of pin definitions and mappings.

**REQ-PIN-023:** The editor SHALL support search and filter across pins and channels.

#### 6.2.3 Validation

**REQ-PIN-024:** The editor SHALL validate pin maps in real-time.

**REQ-PIN-025:** Validation SHALL check:
- Unique pin names
- Valid channel references
- Electrical compatibility
- Complete site mappings
- No conflicting resource allocations

**REQ-PIN-026:** Validation errors SHALL be displayed with location information.

**REQ-PIN-027:** The editor SHALL prevent saving invalid pin maps.

### 6.3 Site Configuration

#### 6.3.1 Site Definition

**REQ-PIN-028:** The system SHALL support multiple test sites per test head.

**REQ-PIN-029:** Each site SHALL have:
- Unique site ID (integer, 0 to N-1)
- Site name
- Enable/disable state
- Per-site pin-to-channel mappings
- Per-site DUT configuration

**REQ-PIN-030:** The system SHALL support 1 to 32 sites per test head.

**REQ-PIN-031:** Sites SHALL be independently enable/disable without affecting others.

#### 6.3.2 Site Replication

**REQ-PIN-032:** The pin map editor SHALL support site replication.

**REQ-PIN-033:** Site replication SHALL copy pin-to-channel mappings from one site to others.

**REQ-PIN-034:** Site replication SHALL support offset-based channel assignment for systematic wiring.

**REQ-PIN-035:** Site replication SHALL allow selective copying (e.g., only digital pins).

### 6.4 Pin Map Import/Export

#### 6.4.1 File Formats

**REQ-PIN-036:** The system SHALL support pin map file formats:
- Native XML format
- CSV format (for spreadsheet editing)
- JSON format (for programmatic access)

**REQ-PIN-037:** Pin map files SHALL include complete site configurations.

**REQ-PIN-038:** Imported pin maps SHALL be validated before acceptance.

**REQ-PIN-039:** Export SHALL support selective export (e.g., single site, specific pin types).

#### 6.4.2 Template System

**REQ-PIN-040:** The system SHALL support pin map templates.

**REQ-PIN-041:** Templates SHALL define common pin configurations for reuse.

**REQ-PIN-042:** Templates SHALL support parameterization (e.g., bus width, channel offsets).

**REQ-PIN-043:** Users SHALL be able to create custom templates from existing pin maps.

---

## 7. Site-Based Testing

### 7.1 Site Manager

#### 7.1.1 Site Lifecycle

**REQ-SITE-001:** The system SHALL provide a site manager for multi-site testing.

**REQ-SITE-002:** The site manager SHALL control site lifecycle:
- Site initialization
- Resource allocation
- Test execution
- Result collection
- Resource deallocation
- Site cleanup

**REQ-SITE-003:** Each site SHALL execute independently in its own thread.

**REQ-SITE-004:** Site lifecycle events SHALL be logged with site ID.

#### 7.1.2 Site Resource Allocation

**REQ-SITE-005:** Each site SHALL have dedicated resource allocations defined by pin map.

**REQ-SITE-006:** Resource allocation SHALL occur before site testing begins.

**REQ-SITE-007:** Resource allocation failures SHALL prevent site execution but SHALL NOT affect other sites.

**REQ-SITE-008:** Resources SHALL be released after site testing completes.

**REQ-SITE-009:** The system SHALL support shared resources accessible by multiple sites with proper locking.

### 7.2 Per-Site Execution

#### 7.2.1 Site Context

**REQ-SITE-010:** Each site SHALL maintain an independent execution context containing:
- Site ID
- DUT information
- Pin map reference
- Allocated instruments and channels
- Site-specific variables
- Execution state

**REQ-SITE-011:** Site contexts SHALL be thread-safe.

**REQ-SITE-012:** Test steps SHALL access resources through site context.

**REQ-SITE-013:** Site context SHALL provide APIs to access mapped instruments by logical pin names.

#### 7.2.2 Site-Specific Test Steps

**REQ-SITE-014:** Test steps SHALL be able to query site ID.

**REQ-SITE-015:** Test steps SHALL access instruments using logical pin names resolved through pin map.

**REQ-SITE-016:** Test steps SHALL write results tagged with site ID.

**REQ-SITE-017:** Test steps SHALL support site-specific parameter overrides.

### 7.3 Site Result Collection

#### 7.3.1 Per-Site Results

**REQ-SITE-018:** Test results SHALL be tagged with site ID.

**REQ-SITE-019:** The system SHALL maintain separate result sets per site.

**REQ-SITE-020:** Per-site results SHALL include:
- All test step results
- Site-level verdict (Pass/Fail)
- Execution time
- Resource usage statistics

**REQ-SITE-021:** Per-site results SHALL be accessible during and after execution.

#### 7.3.2 Aggregate Results

**REQ-SITE-022:** The system SHALL aggregate results across all sites.

**REQ-SITE-023:** Aggregate results SHALL include:
- Total sites tested
- Sites passed
- Sites failed
- Per-test pass/fail counts across sites
- Statistical summaries (mean, std dev, min, max)

**REQ-SITE-024:** Aggregate results SHALL be updated in real-time during execution.

---

## 8. Parametric Testing

### 8.1 Parametric Test Framework

#### 8.1.1 Parameter Definition

**REQ-PARAM-001:** The system SHALL support parametric tests with multiple parameters.

**REQ-PARAM-002:** Each parameter SHALL have:
- Parameter name
- Data type (integer, float, string, boolean)
- Sweep configuration (start, stop, step OR list of values)
- Units of measurement
- Description

**REQ-PARAM-003:** Parameters SHALL support sweep types:
- Linear sweep
- Logarithmic sweep
- List-based sweep (discrete values)
- Random sweep (within range)

**REQ-PARAM-004:** The system SHALL support up to 10 parameters per parametric test.

#### 8.1.2 Nested Sweeps

**REQ-PARAM-005:** The system SHALL support nested parameter sweeps (loops within loops).

**REQ-PARAM-006:** Nesting depth SHALL support at least 5 levels.

**REQ-PARAM-007:** The system SHALL calculate total iteration count for nested sweeps.

**REQ-PARAM-008:** Sweep order SHALL be configurable (e.g., outermost to innermost).

#### 8.1.3 Sweep Execution

**REQ-PARAM-009:** Parametric tests SHALL execute once per parameter combination.

**REQ-PARAM-010:** Current parameter values SHALL be accessible to test code.

**REQ-PARAM-011:** The system SHALL support sweep abort on first failure (optional).

**REQ-PARAM-012:** Sweep progress SHALL be reported in UI (current iteration / total iterations).

### 8.2 Measurement and Limits

#### 8.2.1 Measurement Definition

**REQ-PARAM-013:** Parametric tests SHALL define measurements with attributes:
- Measurement name
- Data type
- Units
- Logging preference (all values, failures only, statistical summary)
- Description

**REQ-PARAM-014:** The system SHALL support multiple measurements per parametric test.

**REQ-PARAM-015:** Measurement values SHALL be logged with associated parameter values.

#### 8.2.2 Test Limits

**REQ-PARAM-016:** Each measurement SHALL support test limits:
- Lower limit (optional)
- Upper limit (optional)
- Comparison type (within range, outside range, equal, not equal)

**REQ-PARAM-017:** Limits SHALL be configurable per parameter combination if needed.

**REQ-PARAM-018:** The system SHALL evaluate measurements against limits automatically.

**REQ-PARAM-019:** Limit violations SHALL generate fail verdicts with detailed information.

### 8.3 Statistical Analysis

#### 8.3.1 Statistical Metrics

**REQ-PARAM-020:** The system SHALL calculate statistical metrics for parametric results:
- Mean (average)
- Standard deviation
- Minimum value
- Maximum value
- Median
- Range
- Count
- Pass rate

**REQ-PARAM-021:** Statistics SHALL be calculated per measurement across all iterations.

**REQ-PARAM-022:** Statistics SHALL be calculated per site in multi-site testing.

**REQ-PARAM-023:** Statistical results SHALL be included in test reports.

#### 8.3.2 Cp/Cpk Calculation

**REQ-PARAM-024:** The system SHALL calculate process capability indices:
- Cp (process capability)
- Cpk (process capability index accounting for centering)

**REQ-PARAM-025:** Cp/Cpk calculations SHALL require specification limits (LSL, USL).

**REQ-PARAM-026:** Cp/Cpk results SHALL be displayed and logged.

**REQ-PARAM-027:** The system SHALL support configurable Cp/Cpk pass/fail thresholds.

### 8.4 Data Visualization

#### 8.4.1 Real-time Plots

**REQ-PARAM-028:** The system SHALL display real-time plots for parametric data:
- 2D line plots (parameter vs measurement)
- Scatter plots
- Histograms
- Box plots

**REQ-PARAM-029:** Plots SHALL update during test execution.

**REQ-PARAM-030:** Plots SHALL support multiple measurements on same axes.

**REQ-PARAM-031:** Plot limits SHALL be configurable (auto-scale or fixed).

#### 8.4.2 Data Export

**REQ-PARAM-032:** Parametric data SHALL be exportable to:
- CSV format
- Excel format
- JSON format
- Database

**REQ-PARAM-033:** Exported data SHALL include:
- All parameter values
- All measurement values
- Limit pass/fail status
- Timestamps

---

## 9. Test Limits Management

### 9.1 Limits File System

#### 9.1.1 Limits File Format

**REQ-LIM-001:** The system SHALL support limits files for managing test specifications.

**REQ-LIM-002:** Limits files SHALL define:
- Test names
- Measurement names
- Lower limits
- Upper limits
- Units
- Limit types (specification, screening, etc.)

**REQ-LIM-003:** Limits files SHALL support hierarchical organization (test groups, test categories).

**REQ-LIM-004:** Limits files SHALL be stored in XML or CSV format.

#### 9.1.2 Limits File Loading

**REQ-LIM-005:** The system SHALL load limits files at test plan initialization.

**REQ-LIM-006:** Test steps SHALL query limits by test/measurement name.

**REQ-LIM-007:** Missing limits SHALL generate warnings but SHALL NOT prevent execution.

**REQ-LIM-008:** The system SHALL support multiple active limits files (e.g., device-specific, generic).

**REQ-LIM-009:** Limits precedence SHALL be configurable when multiple files define the same limit.

### 9.2 Limits Editor

#### 9.2.1 Editor Interface

**REQ-LIM-010:** The system SHALL provide a graphical limits editor.

**REQ-LIM-011:** The editor SHALL display limits in tabular form (filterable and sortable).

**REQ-LIM-012:** The editor SHALL support operations:
- Add/remove test limits
- Edit limit values
- Import limits from spreadsheet
- Export limits to various formats

**REQ-LIM-013:** The editor SHALL highlight inconsistencies (e.g., lower > upper limit).

#### 9.2.2 Limits Import

**REQ-LIM-014:** The editor SHALL import limits from:
- CSV files
- Excel spreadsheets (.xlsx, .xls)
- Tab-delimited text files

**REQ-LIM-015:** Import SHALL support column mapping configuration.

**REQ-LIM-016:** Import SHALL validate data types and ranges.

**REQ-LIM-017:** Import errors SHALL be reported with row/column information.

#### 9.2.3 Limits Validation

**REQ-LIM-018:** The limits editor SHALL validate:
- Numeric limit values are valid
- Lower limits ≤ upper limits
- Units are specified
- Required fields are populated

**REQ-LIM-019:** Validation errors SHALL prevent saving.

**REQ-LIM-020:** Validation warnings SHALL allow saving with confirmation.

### 9.3 Limits Integration

#### 9.3.1 Test Step Integration

**REQ-LIM-021:** Test steps SHALL automatically query limits for configured measurements.

**REQ-LIM-022:** Limits SHALL be applied to measurement results automatically.

**REQ-LIM-023:** Test steps SHALL support manual limit overrides.

**REQ-LIM-024:** The system SHALL log which limits file was used for each test.

#### 9.3.2 Dynamic Limits

**REQ-LIM-025:** The system SHALL support dynamic limits calculated at runtime.

**REQ-LIM-026:** Dynamic limits SHALL support expressions using:
- Other measurement values
- DUT properties
- Environmental conditions

**REQ-LIM-027:** Dynamic limit expressions SHALL be validated before execution.

---

## 10. Binning System

### 10.1 Bin Definition

#### 10.1.1 Bin Configuration

**REQ-BIN-001:** The system SHALL support device binning based on test results.

**REQ-BIN-002:** Each bin SHALL have:
- Bin number (unique integer)
- Bin name
- Pass/fail status
- Classification rules
- Description

**REQ-BIN-003:** The system SHALL support at least 256 bins.

**REQ-BIN-004:** Bin 1 SHALL be reserved for pass-all devices by convention.

**REQ-BIN-005:** Higher bin numbers SHALL typically indicate more severe failures.

#### 10.1.2 Bin Priority

**REQ-BIN-006:** Bins SHALL have priority ordering.

**REQ-BIN-007:** When a device matches multiple bin criteria, the highest priority bin SHALL be assigned.

**REQ-BIN-008:** Bin priorities SHALL be configurable.

### 10.2 Binning Rules

#### 10.2.1 Rule Definition

**REQ-BIN-009:** Binning rules SHALL support conditions:
- Test name pattern matching
- Measurement value ranges
- Verdict (pass, fail, error, abort)
- Logical operators (AND, OR, NOT)
- Comparison operators (=, ≠, <, >, ≤, ≥)

**REQ-BIN-010:** Rules SHALL support nested conditions.

**REQ-BIN-011:** Rules SHALL reference test step results by name or path.

#### 10.2.2 Multi-Level Binning

**REQ-BIN-012:** The system SHALL support multi-level binning:
- Hard bins (physical device bins)
- Soft bins (classification subcategories)

**REQ-BIN-013:** Soft bins SHALL map to hard bins through configuration.

**REQ-BIN-014:** The system SHALL support many-to-one soft-to-hard bin mapping.

### 10.3 Bin Summary and Reporting

#### 10.3.1 Bin Statistics

**REQ-BIN-015:** The system SHALL collect bin statistics:
- Count per bin
- Percentage per bin
- First device in bin
- Last device in bin

**REQ-BIN-016:** Bin statistics SHALL be updated in real-time.

**REQ-BIN-017:** Bin statistics SHALL be displayed in UI.

**REQ-BIN-018:** Bin statistics SHALL be included in test reports.

#### 10.3.2 Yield Analysis

**REQ-BIN-019:** The system SHALL calculate yield metrics:
- Overall yield (pass / total)
- Bin-specific yields
- Cumulative yield

**REQ-BIN-020:** Yield calculations SHALL exclude aborted tests (optional configuration).

**REQ-BIN-021:** Yield trends SHALL be displayable over time/lots.

---

## 11. RF Testing Capabilities

### 11.1 RF Instrument Support

#### 11.1.1 RF Instrument Types

**REQ-RF-001:** The system SHALL support RF instrument types:
- Signal generators (VSG)
- Signal analyzers (VSA)
- Spectrum analyzers
- Network analyzers (VNA)
- Power meters

**REQ-RF-002:** RF instruments SHALL integrate with pin mapping system.

**REQ-RF-003:** RF instruments SHALL support site-based configurations.

#### 11.1.2 RF Channel Mapping

**REQ-RF-004:** RF channels SHALL be mappable to device RF pins.

**REQ-RF-005:** The system SHALL validate RF channel connectivity.

**REQ-RF-006:** RF signal paths SHALL be documented in pin map.

### 11.2 RF Test Steps

#### 11.2.1 Standard RF Measurements

**REQ-RF-007:** The system SHALL provide RF test step templates:
- Transmit power measurement
- Receive sensitivity measurement
- Frequency error measurement
- Modulation quality (EVM, ACPR, etc.)
- Spurious emissions
- Harmonic distortion
- Phase noise

**REQ-RF-008:** RF test steps SHALL support standard wireless protocols (WiFi, Bluetooth, Cellular).

**REQ-RF-009:** RF measurements SHALL use calibrated signal paths when configured.

#### 11.2.2 RF Sweeps

**REQ-RF-010:** RF test steps SHALL support frequency sweeps.

**REQ-RF-011:** RF test steps SHALL support power sweeps.

**REQ-RF-012:** Sweep results SHALL be plottable in real-time.

**REQ-RF-013:** Sweep data SHALL include frequency/power, measurement value, and pass/fail status.

### 11.3 RF Calibration

#### 11.3.1 Calibration Data

**REQ-RF-014:** The system SHALL store RF path calibration data per site.

**REQ-RF-015:** Calibration data SHALL include:
- Frequency-dependent loss
- Phase offset
- Calibration date
- Valid frequency range

**REQ-RF-016:** The system SHALL apply calibration corrections to measurements automatically.

**REQ-RF-017:** Expired calibrations SHALL generate warnings.

---

## Document Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 2.0 | 2025-01-XX | TestMATE Team | Added semiconductor testing features |
