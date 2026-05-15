/**************************************************************************
 * File Name: IConnection.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Communication connection interface.
 * Requirements: REQ-COMM-001 to REQ-COMM-020
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"

namespace TestMATE {

/**************************************************************************
 * Interface: IConnection
 * Description: Base interface for communication connections
 **************************************************************************/
class IConnection {
public:
    virtual ~IConnection() = default;

    virtual CResult Open() = 0;
    virtual CResult Close() = 0;
    [[nodiscard]] virtual bool IsOpen() const = 0;

    virtual CResult Write(const TString& in_strData) = 0;
    virtual CResult Write(const TVector<TUInt8>& in_vecData) = 0;
    virtual CResult Read(TString& out_strData, TInt64 in_timeoutMs = 0) = 0;
    virtual CResult Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes, TInt64 in_timeoutMs = 0) = 0;

    virtual CResult SetTimeout(TInt64 in_timeoutMs) = 0;
    virtual CResult SetTerminator(const TString& in_strTerm) = 0;

    [[nodiscard]] virtual TString GetAddress() const = 0;
    [[nodiscard]] virtual TString GetLastError() const = 0;
};

/**************************************************************************
 * Class: CTcpConnection
 * Description: TCP/IP socket connection
 **************************************************************************/
class CTcpConnection : public IConnection {
public:
    CTcpConnection(const TString& in_strHost, TUInt16 in_port);
    ~CTcpConnection() override;

    CResult Open() override;
    CResult Close() override;
    [[nodiscard]] bool IsOpen() const override;

    CResult Write(const TString& in_strData) override;
    CResult Write(const TVector<TUInt8>& in_vecData) override;
    CResult Read(TString& out_strData, TInt64 in_timeoutMs = 0) override;
    CResult Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes, TInt64 in_timeoutMs = 0) override;

    CResult SetTimeout(TInt64 in_timeoutMs) override;
    CResult SetTerminator(const TString& in_strTerm) override;

    [[nodiscard]] TString GetAddress() const override;
    [[nodiscard]] TString GetLastError() const override { return m_strLastError; }

private:
    TString m_strHost;
    TUInt16 m_port;
    TInt64 m_timeoutMs{5000};
    TString m_strTerminator{"\n"};
    TString m_strLastError;

#ifdef _WIN32
    void* m_socket{nullptr};
#else
    int m_socket{-1};
#endif
    bool m_bConnected{false};
};

/**************************************************************************
 * Class: CSimulatedConnection
 * Description: Simulated connection for testing
 **************************************************************************/
class CSimulatedConnection : public IConnection {
public:
    CSimulatedConnection();
    ~CSimulatedConnection() override = default;

    CResult Open() override;
    CResult Close() override;
    [[nodiscard]] bool IsOpen() const override { return m_bOpen; }

    CResult Write(const TString& in_strData) override;
    CResult Write(const TVector<TUInt8>& in_vecData) override;
    CResult Read(TString& out_strData, TInt64 in_timeoutMs = 0) override;
    CResult Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes, TInt64 in_timeoutMs = 0) override;

    CResult SetTimeout(TInt64 in_timeoutMs) override;
    CResult SetTerminator(const TString& in_strTerm) override;

    [[nodiscard]] TString GetAddress() const override { return "SIMULATED"; }
    [[nodiscard]] TString GetLastError() const override { return ""; }

    // Test helpers
    void SetResponse(const TString& in_strResponse) { m_strNextResponse = in_strResponse; }
    [[nodiscard]] TString GetLastCommand() const { return m_strLastCommand; }

private:
    bool m_bOpen{false};
    TString m_strLastCommand;
    TString m_strNextResponse;
    TInt64 m_timeoutMs{5000};
};

} // namespace TestMATE
