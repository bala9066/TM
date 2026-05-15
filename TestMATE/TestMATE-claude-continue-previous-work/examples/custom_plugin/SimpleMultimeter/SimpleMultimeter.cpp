/**************************************************************************
 * File Name: SimpleMultimeter.cpp
 * Description: Implementation of SimpleMultimeter plugin
 **************************************************************************/

#include "SimpleMultimeter.h"
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace TestMATE;

namespace TestMATEPlugins {

CSimpleMultimeter::CSimpleMultimeter()
    : m_eState(EPluginState::kUnloaded)
    , m_bConnected(false)
    , m_eMode(EMeasureMode::kVoltageDC)
    , m_bSimulated(true)
    , m_randomGen(std::random_device{}())
    , m_distribution(0.95, 1.05)  // ±5% variation for simulated readings
{
    // Initialize plugin info
    m_info.id = "simple-multimeter";
    m_info.name = "Simple Multimeter Plugin";
    m_info.version = "1.0.0";
    m_info.author = "TestMATE Example";
    m_info.description = "Example multimeter plugin demonstrating instrument plugin development";
    m_info.type = EPluginType::kInstrument;
    m_info.minHostVersion = "1.0.0";
}

//=============================================================================
// IPlugin Interface Implementation
//=============================================================================

SPluginInfo CSimpleMultimeter::GetInfo() const {
    return m_info;
}

CResult CSimpleMultimeter::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_eState != EPluginState::kUnloaded) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState, "Plugin already initialized");
    }

    // Perform initialization
    m_eState = EPluginState::kInitialized;
    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bConnected) {
        Disconnect();
    }

    m_eState = EPluginState::kUnloaded;
    return TESTMATE_SUCCESS();
}

EPluginState CSimpleMultimeter::GetState() const {
    return m_eState;
}

TString CSimpleMultimeter::GetLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_strLastError;
}

//=============================================================================
// IInstrumentPlugin Interface Implementation
//=============================================================================

CResult CSimpleMultimeter::Connect(const TString& in_strAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kAlreadyConnected, "Already connected to instrument");
    }

    m_strAddress = in_strAddress;

    // Check if this is a simulated device
    if (in_strAddress.find("SIM:") == 0 || in_strAddress == "SIMULATED") {
        m_bSimulated = true;
        m_bConnected = true;
        m_eState = EPluginState::kActive;
        return TESTMATE_SUCCESS();
    }

    // For real hardware, implement actual connection logic here
    // Example: Open serial port, TCP connection, VISA session, etc.

    // For this example, we'll simulate connection
    m_bSimulated = true;
    m_bConnected = true;
    m_eState = EPluginState::kActive;

    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    // Close connection resources
    m_bConnected = false;
    m_eState = EPluginState::kInitialized;

    return TESTMATE_SUCCESS();
}

bool CSimpleMultimeter::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bConnected;
}

CResult CSimpleMultimeter::Write(const TString& in_strCommand) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    // For simulated device, just validate command format
    if (m_bSimulated) {
        if (in_strCommand.empty()) {
            SetError("Empty command");
            return TESTMATE_ERROR(EErrorCode::kInvalidParameter, "Empty command");
        }
        return TESTMATE_SUCCESS();
    }

    // For real hardware, send command to instrument
    // Implementation depends on connection type (serial, GPIB, TCP, etc.)

    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::Read(TString& out_strResponse, TInt64 in_timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    if (m_bSimulated) {
        // Generate simulated response based on current mode
        TDouble value = GenerateSimulatedReading();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        out_strResponse = oss.str();
        return TESTMATE_SUCCESS();
    }

    // For real hardware, read from instrument

    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::Query(const TString& in_strCommand,
                                TString& out_strResponse,
                                TInt64 in_timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    // Process SCPI command
    return ProcessSCPICommand(in_strCommand, out_strResponse);
}

//=============================================================================
// Multimeter-Specific Methods
//=============================================================================

CResult CSimpleMultimeter::MeasureVoltage(TDouble& out_voltage) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    m_eMode = EMeasureMode::kVoltageDC;

    if (m_bSimulated) {
        // Simulated voltage reading (0-10V nominal)
        out_voltage = 5.0 * m_distribution(m_randomGen);
    } else {
        // Real hardware measurement
        // Send commands, wait for reading, parse response
        out_voltage = 0.0;
    }

    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::MeasureCurrent(TDouble& out_current) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    m_eMode = EMeasureMode::kCurrentDC;

    if (m_bSimulated) {
        // Simulated current reading (0-1A nominal)
        out_current = 0.5 * m_distribution(m_randomGen);
    } else {
        // Real hardware measurement
        out_current = 0.0;
    }

    return TESTMATE_SUCCESS();
}

CResult CSimpleMultimeter::MeasureResistance(TDouble& out_resistance) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to instrument");
    }

    m_eMode = EMeasureMode::kResistance;

    if (m_bSimulated) {
        // Simulated resistance reading (1kΩ nominal)
        out_resistance = 1000.0 * m_distribution(m_randomGen);
    } else {
        // Real hardware measurement
        out_resistance = 0.0;
    }

    return TESTMATE_SUCCESS();
}

//=============================================================================
// Private Helper Methods
//=============================================================================

void CSimpleMultimeter::SetError(const TString& in_strError) {
    m_strLastError = in_strError;
}

TDouble CSimpleMultimeter::GenerateSimulatedReading() {
    switch (m_eMode) {
        case EMeasureMode::kVoltageDC:
            return 5.0 * m_distribution(m_randomGen);
        case EMeasureMode::kCurrentDC:
            return 0.5 * m_distribution(m_randomGen);
        case EMeasureMode::kResistance:
            return 1000.0 * m_distribution(m_randomGen);
        default:
            return 0.0;
    }
}

CResult CSimpleMultimeter::ProcessSCPICommand(const TString& in_strCommand,
                                             TString& out_strResponse) {
    // Implement basic SCPI command parsing
    if (in_strCommand == "*IDN?") {
        out_strResponse = "TestMATE,SimpleMultimeter,SN12345,v1.0.0";
        return TESTMATE_SUCCESS();
    }

    if (in_strCommand == "*RST") {
        m_eMode = EMeasureMode::kVoltageDC;
        out_strResponse = "";
        return TESTMATE_SUCCESS();
    }

    if (in_strCommand == "MEAS:VOLT:DC?") {
        TDouble voltage;
        auto result = MeasureVoltage(voltage);
        if (result.IsSuccess()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << voltage;
            out_strResponse = oss.str();
        }
        return result;
    }

    if (in_strCommand == "MEAS:CURR:DC?") {
        TDouble current;
        auto result = MeasureCurrent(current);
        if (result.IsSuccess()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << current;
            out_strResponse = oss.str();
        }
        return result;
    }

    if (in_strCommand == "MEAS:RES?") {
        TDouble resistance;
        auto result = MeasureResistance(resistance);
        if (result.IsSuccess()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << resistance;
            out_strResponse = oss.str();
        }
        return result;
    }

    return TESTMATE_ERROR(EErrorCode::kInvalidParameter, "Unknown SCPI command");
}

} // namespace TestMATEPlugins
