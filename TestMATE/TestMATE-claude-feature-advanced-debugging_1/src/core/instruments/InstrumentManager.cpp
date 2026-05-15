/**************************************************************************
 * File Name: InstrumentManager.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of instrument manager
 * Requirements: REQ-INST-031 to REQ-INST-050
 **************************************************************************/

#include "InstrumentManager.h"
#include "utils/LogManager.h"

namespace TestMATE {

//=============================================================================
// CInstrumentManager Implementation
//=============================================================================

CInstrumentManager& CInstrumentManager::GetInstance() {
    static CInstrumentManager instance;
    return instance;
}

CInstrumentManager::~CInstrumentManager() {
    DisconnectAll();
}

CResult CInstrumentManager::RegisterInstrument(TSharedPtr<IInstrument> in_pInstrument) {
    if (!in_pInstrument) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Null instrument");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    TString id = in_pInstrument->GetId();
    if (m_mapInstruments.count(id) > 0) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists,
                                "Instrument already registered: " + id);
    }

    m_mapInstruments[id] = in_pInstrument;

    CLogManager::GetInstance().LogInfo("InstrumentManager",
        "Registered instrument: {}", id);

    return TESTMATE_SUCCESS();
}

CResult CInstrumentManager::UnregisterInstrument(const TString& in_strId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapInstruments.find(in_strId);
    if (it == m_mapInstruments.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Instrument not found: " + in_strId);
    }

    // Disconnect first if connected
    if (it->second && it->second->IsConnected()) {
        it->second->Disconnect();
    }

    m_mapInstruments.erase(it);

    CLogManager::GetInstance().LogInfo("InstrumentManager",
        "Unregistered instrument: {}", in_strId);

    return TESTMATE_SUCCESS();
}

void CInstrumentManager::UnregisterAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, pInstrument] : m_mapInstruments) {
        if (pInstrument && pInstrument->IsConnected()) {
            pInstrument->Disconnect();
        }
    }

    m_mapInstruments.clear();
}

IInstrument* CInstrumentManager::GetInstrument(const TString& in_strId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapInstruments.find(in_strId);
    if (it != m_mapInstruments.end()) {
        return it->second.get();
    }
    return nullptr;
}

TVector<TString> CInstrumentManager::GetInstrumentIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<TString> ids;
    for (const auto& [id, pInstrument] : m_mapInstruments) {
        ids.push_back(id);
    }
    return ids;
}

TVector<SInstrumentInfo> CInstrumentManager::GetInstrumentList() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<SInstrumentInfo> list;
    for (const auto& [id, pInstrument] : m_mapInstruments) {
        if (pInstrument) {
            list.push_back(pInstrument->GetInfo());
        }
    }
    return list;
}

TUInt32 CInstrumentManager::GetInstrumentCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<TUInt32>(m_mapInstruments.size());
}

CResult CInstrumentManager::ConnectInstrument(const TString& in_strId,
                                               const SConnectionConfig& in_config) {
    IInstrument* pInstrument = GetInstrument(in_strId);
    if (!pInstrument) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Instrument not found: " + in_strId);
    }

    return pInstrument->Connect(in_config);
}

CResult CInstrumentManager::DisconnectInstrument(const TString& in_strId) {
    IInstrument* pInstrument = GetInstrument(in_strId);
    if (!pInstrument) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Instrument not found: " + in_strId);
    }

    return pInstrument->Disconnect();
}

CResult CInstrumentManager::DisconnectAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, pInstrument] : m_mapInstruments) {
        if (pInstrument && pInstrument->IsConnected()) {
            pInstrument->Disconnect();
        }
    }

    return TESTMATE_SUCCESS();
}

bool CInstrumentManager::IsInstrumentConnected(const TString& in_strId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapInstruments.find(in_strId);
    if (it != m_mapInstruments.end() && it->second) {
        return it->second->IsConnected();
    }
    return false;
}

TSharedPtr<IInstrument> CInstrumentManager::CreateSimulatedInstrument(
    const TString& in_strId, EInstrumentType in_eType) {

    return std::make_shared<CSimulatedInstrument>(in_strId, in_eType);
}

//=============================================================================
// CSimulatedInstrument Implementation
//=============================================================================

CSimulatedInstrument::CSimulatedInstrument(const TString& in_strId, EInstrumentType in_eType)
    : CInstrumentBase(in_strId, in_eType)
{
    m_info.manufacturer = "SIMULATED";
    m_info.model = "SIM-" + std::to_string(static_cast<int>(in_eType));
    m_info.serialNumber = "SN" + in_strId;
    m_info.firmwareVersion = "1.0";
    m_info.connectionType = EConnectionType::kSimulated;

    // Set up default responses
    m_mapResponses["*IDN?"] = m_info.manufacturer + "," + m_info.model + "," +
                              m_info.serialNumber + "," + m_info.firmwareVersion;
    m_mapResponses["*TST?"] = "0";
    m_mapResponses["SYST:ERR?"] = "0,\"No error\"";
}

CResult CSimulatedInstrument::Connect(const SConnectionConfig& in_config) {
    m_config = in_config;
    m_info.address = in_config.address;
    SetState(EInstrumentState::kConnected);

    CLogManager::GetInstance().LogInfo("SimulatedInstrument",
        "Connected: {}", m_info.id);

    return TESTMATE_SUCCESS();
}

CResult CSimulatedInstrument::Disconnect() {
    SetState(EInstrumentState::kDisconnected);

    CLogManager::GetInstance().LogInfo("SimulatedInstrument",
        "Disconnected: {}", m_info.id);

    return TESTMATE_SUCCESS();
}

CResult CSimulatedInstrument::Write(const TString& in_strCommand) {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    m_strLastCommand = in_strCommand;

    // Check for query response
    auto it = m_mapResponses.find(in_strCommand);
    if (it != m_mapResponses.end()) {
        m_strPendingResponse = it->second;
    }

    return TESTMATE_SUCCESS();
}

CResult CSimulatedInstrument::Read(TString& out_strResponse, TInt64 /*in_timeoutMs*/) {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    out_strResponse = m_strPendingResponse;
    m_strPendingResponse.clear();

    return TESTMATE_SUCCESS();
}

CResult CSimulatedInstrument::Query(const TString& in_strCommand,
                                     TString& out_strResponse,
                                     TInt64 in_timeoutMs) {
    auto result = Write(in_strCommand);
    if (result.IsFailure()) {
        return result;
    }
    return Read(out_strResponse, in_timeoutMs);
}

void CSimulatedInstrument::SetResponse(const TString& in_strCmd,
                                        const TString& in_strResponse) {
    m_mapResponses[in_strCmd] = in_strResponse;
}

} // namespace TestMATE
