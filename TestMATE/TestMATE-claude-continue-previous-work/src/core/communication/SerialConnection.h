/**************************************************************************
 * File Name: SerialConnection.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Serial port communication implementation.
 * Requirements: REQ-COMM-021 to REQ-COMM-040
 **************************************************************************/

#pragma once

#include "IConnection.h"

namespace TestMATE {

/**************************************************************************
 * Enum: EParity
 * Description: Serial port parity settings
 **************************************************************************/
enum class EParity {
    kNone,
    kOdd,
    kEven,
    kMark,
    kSpace
};

/**************************************************************************
 * Enum: EStopBits
 * Description: Serial port stop bits
 **************************************************************************/
enum class EStopBits {
    kOne,
    kOnePointFive,
    kTwo
};

/**************************************************************************
 * Struct: SSerialConfig
 * Description: Serial port configuration
 **************************************************************************/
struct SSerialConfig {
    TString portName;
    TUInt32 baudRate{9600};
    TUInt8 dataBits{8};
    EParity parity{EParity::kNone};
    EStopBits stopBits{EStopBits::kOne};
    bool flowControl{false};
    TInt64 timeoutMs{5000};
    TString terminator{"\n"};
};

/**************************************************************************
 * Class: CSerialConnection
 * Description: Serial port connection implementation
 **************************************************************************/
class CSerialConnection : public IConnection {
public:
    explicit CSerialConnection(const SSerialConfig& in_config);
    ~CSerialConnection() override;

    CResult Open() override;
    CResult Close() override;
    [[nodiscard]] bool IsOpen() const override;

    CResult Write(const TString& in_strData) override;
    CResult Write(const TVector<TUInt8>& in_vecData) override;
    CResult Read(TString& out_strData, TInt64 in_timeoutMs = 0) override;
    CResult Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes, TInt64 in_timeoutMs = 0) override;

    CResult SetTimeout(TInt64 in_timeoutMs) override;
    CResult SetTerminator(const TString& in_strTerm) override;

    [[nodiscard]] TString GetAddress() const override { return m_config.portName; }
    [[nodiscard]] TString GetLastError() const override { return m_strLastError; }

    // Serial-specific
    CResult SetBaudRate(TUInt32 in_baudRate);
    CResult Flush();
    [[nodiscard]] TUInt32 GetBytesAvailable() const;

private:
    CResult ConfigurePort();

    SSerialConfig m_config;
    TString m_strLastError;
    bool m_bOpen{false};

#ifdef _WIN32
    void* m_hPort{nullptr};
#else
    int m_fd{-1};
#endif
};

} // namespace TestMATE
