# Semiconductor Test Examples

This directory contains example test sequences for semiconductor device testing, demonstrating TestMATE's capabilities for parametric and functional testing.

## Files

### parametric_test.json
**Device Type**: Power MOSFET
**Test Type**: Comprehensive parametric characterization
**Steps**: 44 steps across 9 test categories

**Test Categories**:
1. **Setup** - Handler and test head configuration
2. **Continuity** - Pin contact verification
3. **Leakage** - IDSS and IGSS measurements
4. **Threshold Voltage (VTH)** - Gate threshold characterization
5. **On-Resistance (RDS(on))** - Drain-source resistance
6. **Breakdown Voltage (BVDSS)** - Maximum voltage rating
7. **Capacitance** - Input, output, and reverse transfer capacitance
8. **Body Diode** - Forward voltage and reverse recovery time
9. **Finalization** - Device binning and STDF data logging

**Key Parameters Measured**:
| Parameter | Symbol | Specification | Test Condition |
|-----------|--------|---------------|----------------|
| Drain-Source Leakage | IDSS | < 10nA | VDS = rated, VGS = 0V |
| Gate-Source Leakage | IGSS | < 1nA | VGS = ±20V |
| Threshold Voltage | VTH | 2.0V - 4.0V | ID = 250μA |
| On-Resistance | RDS(on) | < 100mΩ | VGS = 10V, ID = 10A |
| Breakdown Voltage | BVDSS | > 600V | ID = 250μA, VGS = 0V |
| Input Capacitance | CISS | Per datasheet | VDS = 25V, f = 1MHz |
| Output Capacitance | COSS | Per datasheet | VDS = 25V, f = 1MHz |
| Reverse Transfer Capacitance | CRSS | Per datasheet | VDS = 25V, f = 1MHz |
| Diode Forward Voltage | VF | < 1.5V | IF = 10A |
| Reverse Recovery Time | trr | < 100ns | Per datasheet conditions |

## Semiconductor Testing Features

### STDF Integration
TestMATE includes built-in support for Standard Test Data Format (STDF) V4:
- Automatic PTR (Parametric Test Record) generation
- Device binning based on test results
- Lot tracking and wafer mapping
- Production data export for yield analysis

### Typical Test Flow
```
1. Device Loading (Handler Integration)
   ↓
2. Contact Verification (Continuity)
   ↓
3. DC Parametric Tests (Leakage, VTH, RDS(on), BVDSS)
   ↓
4. AC Parametric Tests (Capacitance, Switching)
   ↓
5. Functional Tests (Body Diode)
   ↓
6. Binning Decision
   ↓
7. STDF Data Logging
   ↓
8. Device Unload
```

### Instrument Requirements

For the parametric_test.json sequence, you'll need:
- **SMU (Source Measure Unit)** - For I-V characterization
  - Voltage range: 0 to 1000V
  - Current range: 1nA to 50A
  - Typical: Keithley 2657A or similar

- **LCR Meter** - For capacitance measurements
  - Frequency range: 100Hz to 1MHz
  - Capacitance range: 1pF to 100μF
  - Typical: Agilent 4284A or similar

- **Curve Tracer** (Optional) - For breakdown voltage sweep
  - High-voltage capability: Up to 1500V
  - Current limiting: < 10mA

- **Test Handler** - For automated device handling
  - Supports standard packages (TO-220, TO-247, etc.)
  - Programmable contact force
  - Binning capability: Typically 8+ bins

## Customizing for Your Devices

### Adapting parametric_test.json

1. **Update Test Limits**:
   - Modify validation step names to reflect your specifications
   - Example: Change "Validate < 100mΩ" to your RDS(on) limit

2. **Add/Remove Tests**:
   - Not testing capacitance? Remove CAP-001 through CAP-006
   - Need gate charge? Add QG test steps

3. **Adjust Test Conditions**:
   - Update voltage/current forcing levels in step names
   - Modify wait times for device settling

4. **Add Custom Parameters**:
```json
{
  "id": "CUSTOM-001",
  "name": "Measure Gate Charge",
  "type": "measurement",
  "enabled": true,
  "parameters": {
    "VDS": 400,
    "ID": 10,
    "VGS_target": 10,
    "limit_min": 50,
    "limit_max": 100,
    "unit": "nC"
  }
}
```

## Example: Creating a Diode Test Sequence

```json
{
  "id": "DIODE-001",
  "name": "Diode Parametric Test",
  "version": "1.0.0",
  "steps": [
    {"id": "CONT-001", "name": "Continuity Test", "type": "validation", "enabled": true},
    {"id": "VF-001", "name": "Forward Voltage - Force IF=1A", "type": "action", "enabled": true},
    {"id": "VF-002", "name": "Forward Voltage - Measure VF", "type": "measurement", "enabled": true},
    {"id": "VF-003", "name": "Forward Voltage - Validate VF < 0.7V", "type": "validation", "enabled": true},
    {"id": "IR-001", "name": "Reverse Leakage - Apply VR=50V", "type": "action", "enabled": true},
    {"id": "IR-002", "name": "Reverse Leakage - Measure IR", "type": "measurement", "enabled": true},
    {"id": "IR-003", "name": "Reverse Leakage - Validate IR < 10uA", "type": "validation", "enabled": true},
    {"id": "BV-001", "name": "Breakdown - Sweep Voltage", "type": "measurement", "enabled": true},
    {"id": "BV-002", "name": "Breakdown - Validate BV > 75V", "type": "validation", "enabled": true}
  ]
}
```

## Integration with TestMATE Core

### Using CDeviceHandler

```cpp
#include "core/semiconductor/DeviceHandler.h"
#include "core/semiconductor/StdfWriter.h"

TestMATE::CDeviceHandler handler;
TestMATE::CStdfWriter stdfWriter;

// Configure device info
TestMATE::SDeviceInfo deviceInfo;
deviceInfo.partNumber = "IRFB4115";
deviceInfo.lotId = "LOT2024A";
deviceInfo.waferPosition = {5, 7};  // Row 5, Col 7

// Run test sequence
auto result = testSequence.Execute(context);

// Write STDF record
if (result.verdict == TestMATE::ETestVerdict::kPass) {
    stdfWriter.WriteParametricTestRecord(deviceInfo, measurements);
}
```

## Best Practices

1. **Test Order**: Run critical tests first (continuity, leakage) before high-power tests
2. **Safety Limits**: Always set instrument compliance limits to prevent device damage
3. **Settling Time**: Include adequate wait steps after changing stimulus conditions
4. **Calibration**: Perform regular instrument calibration and offset compensation
5. **Data Logging**: Use STDF format for production data to enable yield analysis
6. **Binning Strategy**: Define clear pass/fail criteria for each parameter
7. **Test Time**: Optimize test sequence to minimize total test time while maintaining coverage

## See Also

- [STDF Writer Documentation](../../docs/semiconductor/stdf_writer.md)
- [Device Handler API](../../docs/semiconductor/device_handler.md)
- [Instrument Plugin Development](../custom_plugin/)
- [Basic Test Examples](../basic_test_plan/)

---

**Note**: These are example sequences. Actual parametric test conditions and limits must be based on device datasheets and your production requirements.
