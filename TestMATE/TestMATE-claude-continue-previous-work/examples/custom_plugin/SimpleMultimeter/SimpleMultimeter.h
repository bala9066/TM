/**************************************************************************
 * File Name: SimpleMultimeter.h
 * Description: Example instrument plugin demonstrating a simple multimeter
 * Author: TestMATE Example
 *
 * This is a template/example showing how to create custom instrument plugins
 * for TestMATE. It implements a simulated multimeter that can measure
 * voltage, current, and resistance.
 **************************************************************************/

#pragma once

#include "core/plugins/IPlugin.h"
#include "core/instruments/IInstrument.h"
#include "core/communication/IConnection.h"
#include <memory>
#include <mutex>
#include <random>

namespace TestMATEPlugins {

/**************************************************************************
 * Class: CSimpleMultimeter
 * Description: Example multimeter plugin with basic measurement capabilities
 *
 * Features:
 * - DC Voltage measurement (mV to kV range)
 * - DC Current measurement (μA to A range)
 * - Resistance measurement (Ω to MΩ range)
 * - Simulated measurements for testing without hardware
 * - Standard SCPI command interface
 **************************************************************************/
class CSimpleMultimeter : public TestMATE::IInstrumentPlugin {
public:
    CSimpleMultimeter();
    ~CSimpleMultimeter() override = default;

    //=========================================================================
    // IPlugin Interface
    //=========================================================================

    [[nodiscard]] TestMATE::SPluginInfo GetInfo() const override;
    TestMATE::CResult Initialize() override;
    TestMATE::CResult Shutdown() override;
    [[nodiscard]] TestMATE::EPluginState GetState() const override;
    [[nodiscard]] TestMATE::TString GetLastError() const override;

    //=========================================================================
    // IInstrumentPlugin Interface
    //=========================================================================

    TestMATE::CResult Connect(const TestMATE::TString& in_strAddress) override;
    TestMATE::CResult Disconnect() override;
    [[nodiscard]] bool IsConnected() const override;

    TestMATE::CResult Write(const TestMATE::TString& in_strCommand) override;
    TestMATE::CResult Read(TestMATE::TString& out_strResponse, TestMATE::TInt64 in_timeoutMs) override;
    TestMATE::CResult Query(const TestMATE::TString& in_strCommand,
                           TestMATE::TString& out_strResponse,
                           TestMATE::TInt64 in_timeoutMs) override;

    //=========================================================================
    // Multimeter-Specific Methods
    //=========================================================================

    /// Measure DC voltage
    TestMATE::CResult MeasureVoltage(TestMATE::TDouble& out_voltage);

    /// Measure DC current
    TestMATE::CResult MeasureCurrent(TestMATE::TDouble& out_current);

    /// Measure resistance
    TestMATE::CResult MeasureResistance(TestMATE::TDouble& out_resistance);

private:
    // Plugin state
    TestMATE::SPluginInfo m_info;
    TestMATE::EPluginState m_eState;
    TestMATE::TString m_strLastError;

    // Connection state
    bool m_bConnected;
    TestMATE::TString m_strAddress;

    // Measurement configuration
    enum class EMeasureMode {
        kVoltageDC,
        kCurrentDC,
        kResistance
    };
    EMeasureMode m_eMode;

    // Simulation (for testing without hardware)
    bool m_bSimulated;
    std::mt19937 m_randomGen;
    std::uniform_real_distribution<> m_distribution;

    // Thread safety
    mutable std::mutex m_mutex;

    // Helper methods
    void SetError(const TestMATE::TString& in_strError);
    TestMATE::TDouble GenerateSimulatedReading();
    TestMATE::CResult ProcessSCPICommand(const TestMATE::TString& in_strCommand,
                                        TestMATE::TString& out_strResponse);
};

} // namespace TestMATEPlugins

// Plugin export macro
TESTMATE_DECLARE_PLUGIN(TestMATEPlugins::CSimpleMultimeter)
