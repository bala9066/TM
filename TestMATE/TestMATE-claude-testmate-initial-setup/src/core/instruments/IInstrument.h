/**************************************************************************
 * File Name: IInstrument.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Instrument interface for hardware communication.
 * Requirements: REQ-INST-001 to REQ-INST-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: EInstrumentType
 * Description: Types of instruments supported
 **************************************************************************/
enum class EInstrumentType {
    kUnknown,
    kMultimeter,
    kOscilloscope,
    kPowerSupply,
    kSignalGenerator,
    kLogicAnalyzer,
    kSpectrumAnalyzer,
    kNetworkAnalyzer,
    kSourceMeasureUnit,
    kCustom
};

/**************************************************************************
 * Enum: EConnectionType
 * Description: Communication interface types
 **************************************************************************/
enum class EConnectionType {
    kNone,
    kSerial,
    kTcpIp,
    kGpib,
    kUsb,
    kVxi11,
    kPxi,
    kSimulated
};

/**************************************************************************
 * Enum: EInstrumentState
 * Description: Instrument connection states
 **************************************************************************/
enum class EInstrumentState {
    kDisconnected,
    kConnecting,
    kConnected,
    kBusy,
    kError
};

/**************************************************************************
 * Struct: SInstrumentInfo
 * Description: Instrument identification information
 **************************************************************************/
struct SInstrumentInfo {
    TString id;
    TString name;
    TString manufacturer;
    TString model;
    TString serialNumber;
    TString firmwareVersion;
    EInstrumentType type{EInstrumentType::kUnknown};
    EConnectionType connectionType{EConnectionType::kNone};
    TString address;
};

/**************************************************************************
 * Struct: SConnectionConfig
 * Description: Connection configuration parameters
 **************************************************************************/
struct SConnectionConfig {
    EConnectionType type{EConnectionType::kNone};
    TString address;
    TUInt32 port{0};
    TUInt32 baudRate{9600};
    TUInt32 timeout{5000};
    TString terminator{"\n"};
    std::map<TString, TString> options;
};

/**************************************************************************
 * Interface: IInstrument
 * Description: Base interface for all instruments
 * Requirements: REQ-INST-001 to REQ-INST-030
 **************************************************************************/
class IInstrument {
public:
    virtual ~IInstrument() = default;

    //=========================================================================
    // Identification
    //=========================================================================

    [[nodiscard]] virtual SInstrumentInfo GetInfo() const = 0;
    [[nodiscard]] virtual TString GetId() const = 0;
    [[nodiscard]] virtual EInstrumentType GetType() const = 0;

    //=========================================================================
    // Connection
    //=========================================================================

    virtual CResult Connect(const SConnectionConfig& in_config) = 0;
    virtual CResult Disconnect() = 0;
    [[nodiscard]] virtual bool IsConnected() const = 0;
    [[nodiscard]] virtual EInstrumentState GetState() const = 0;

    //=========================================================================
    // Communication
    //=========================================================================

    virtual CResult Write(const TString& in_strCommand) = 0;
    virtual CResult Read(TString& out_strResponse, TInt64 in_timeoutMs = 0) = 0;
    virtual CResult Query(const TString& in_strCommand,
                          TString& out_strResponse,
                          TInt64 in_timeoutMs = 0) = 0;

    //=========================================================================
    // Standard Commands
    //=========================================================================

    virtual CResult Reset() = 0;
    virtual CResult Clear() = 0;
    virtual CResult Identify(TString& out_strIdnResponse) = 0;
    virtual CResult SelfTest(TInt32& out_result) = 0;

    //=========================================================================
    // Error Handling
    //=========================================================================

    [[nodiscard]] virtual TString GetLastError() const = 0;
    virtual CResult GetSystemError(TInt32& out_code, TString& out_message) = 0;
};

/**************************************************************************
 * Class: CInstrumentBase
 * Description: Base implementation with common functionality
 **************************************************************************/
class CInstrumentBase : public IInstrument {
public:
    CInstrumentBase(const TString& in_strId, EInstrumentType in_eType);
    ~CInstrumentBase() override = default;

    [[nodiscard]] SInstrumentInfo GetInfo() const override { return m_info; }
    [[nodiscard]] TString GetId() const override { return m_info.id; }
    [[nodiscard]] EInstrumentType GetType() const override { return m_info.type; }
    [[nodiscard]] EInstrumentState GetState() const override { return m_eState; }
    [[nodiscard]] bool IsConnected() const override;
    [[nodiscard]] TString GetLastError() const override { return m_strLastError; }

    CResult Reset() override;
    CResult Clear() override;
    CResult Identify(TString& out_strIdnResponse) override;
    CResult SelfTest(TInt32& out_result) override;
    CResult GetSystemError(TInt32& out_code, TString& out_message) override;

protected:
    void SetState(EInstrumentState in_eState) { m_eState = in_eState; }
    void SetLastError(const TString& in_strError) { m_strLastError = in_strError; }
    void UpdateInfo(const TString& in_strIdnResponse);

    SInstrumentInfo m_info;
    EInstrumentState m_eState{EInstrumentState::kDisconnected};
    TString m_strLastError;
    SConnectionConfig m_config;
};

} // namespace TestMATE
