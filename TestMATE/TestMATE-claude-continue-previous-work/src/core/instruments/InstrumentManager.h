/**************************************************************************
 * File Name: InstrumentManager.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Singleton manager for instrument lifecycle.
 * Requirements: REQ-INST-031 to REQ-INST-050
 **************************************************************************/

#pragma once

#include "IInstrument.h"
#include <map>
#include <mutex>
#include <memory>

namespace TestMATE {

/**************************************************************************
 * Class: CInstrumentManager
 * Description: Manages instrument instances and connections
 **************************************************************************/
class CInstrumentManager {
public:
    static CInstrumentManager& GetInstance();

    CInstrumentManager(const CInstrumentManager&) = delete;
    CInstrumentManager& operator=(const CInstrumentManager&) = delete;

    //=========================================================================
    // Instrument Management
    //=========================================================================

    CResult RegisterInstrument(TSharedPtr<IInstrument> in_pInstrument);
    CResult UnregisterInstrument(const TString& in_strId);
    void UnregisterAll();

    [[nodiscard]] IInstrument* GetInstrument(const TString& in_strId);
    [[nodiscard]] TVector<TString> GetInstrumentIds() const;
    [[nodiscard]] TVector<SInstrumentInfo> GetInstrumentList() const;
    [[nodiscard]] TUInt32 GetInstrumentCount() const;

    //=========================================================================
    // Connection Management
    //=========================================================================

    CResult ConnectInstrument(const TString& in_strId, const SConnectionConfig& in_config);
    CResult DisconnectInstrument(const TString& in_strId);
    CResult DisconnectAll();

    [[nodiscard]] bool IsInstrumentConnected(const TString& in_strId) const;

    //=========================================================================
    // Factory Methods
    //=========================================================================

    TSharedPtr<IInstrument> CreateSimulatedInstrument(const TString& in_strId,
                                                       EInstrumentType in_eType);

private:
    CInstrumentManager() = default;
    ~CInstrumentManager();

    mutable std::mutex m_mutex;
    std::map<TString, TSharedPtr<IInstrument>> m_mapInstruments;
};

/**************************************************************************
 * Class: CSimulatedInstrument
 * Description: Simulated instrument for testing
 **************************************************************************/
class CSimulatedInstrument : public CInstrumentBase {
public:
    CSimulatedInstrument(const TString& in_strId, EInstrumentType in_eType);
    ~CSimulatedInstrument() override = default;

    CResult Connect(const SConnectionConfig& in_config) override;
    CResult Disconnect() override;

    CResult Write(const TString& in_strCommand) override;
    CResult Read(TString& out_strResponse, TInt64 in_timeoutMs = 0) override;
    CResult Query(const TString& in_strCommand,
                  TString& out_strResponse,
                  TInt64 in_timeoutMs = 0) override;

    // Test helpers
    void SetResponse(const TString& in_strCmd, const TString& in_strResponse);

private:
    std::map<TString, TString> m_mapResponses;
    TString m_strLastCommand;
    TString m_strPendingResponse;
};

} // namespace TestMATE
