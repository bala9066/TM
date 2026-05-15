/**************************************************************************
 * File Name: InstrumentBase.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Base implementation of instrument interface
 * Requirements: REQ-INST-001 to REQ-INST-030
 **************************************************************************/

#include "IInstrument.h"
#include "utils/LogManager.h"

namespace TestMATE {

CInstrumentBase::CInstrumentBase(const TString& in_strId, EInstrumentType in_eType) {
    m_info.id = in_strId;
    m_info.type = in_eType;
}

bool CInstrumentBase::IsConnected() const {
    return m_eState == EInstrumentState::kConnected ||
           m_eState == EInstrumentState::kBusy;
}

CResult CInstrumentBase::Reset() {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }
    return Write("*RST");
}

CResult CInstrumentBase::Clear() {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }
    return Write("*CLS");
}

CResult CInstrumentBase::Identify(TString& out_strIdnResponse) {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }

    auto result = Query("*IDN?", out_strIdnResponse);
    if (result.IsSuccess()) {
        UpdateInfo(out_strIdnResponse);
    }
    return result;
}

CResult CInstrumentBase::SelfTest(TInt32& out_result) {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }

    TString response;
    auto result = Query("*TST?", response);
    if (result.IsSuccess()) {
        try {
            out_result = std::stoi(response);
        } catch (...) {
            out_result = -1;
        }
    }
    return result;
}

CResult CInstrumentBase::GetSystemError(TInt32& out_code, TString& out_message) {
    if (!IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Instrument not connected");
    }

    TString response;
    auto result = Query("SYST:ERR?", response);
    if (result.IsSuccess()) {
        // Parse error response (typically "0,"No error"" or "-100,"Command error"")
        size_t commaPos = response.find(',');
        if (commaPos != TString::npos) {
            try {
                out_code = std::stoi(response.substr(0, commaPos));
                out_message = response.substr(commaPos + 1);
                // Remove quotes if present
                if (!out_message.empty() && out_message.front() == '"') {
                    out_message = out_message.substr(1);
                }
                if (!out_message.empty() && out_message.back() == '"') {
                    out_message.pop_back();
                }
            } catch (...) {
                out_code = 0;
                out_message = response;
            }
        }
    }
    return result;
}

void CInstrumentBase::UpdateInfo(const TString& in_strIdnResponse) {
    // Parse IDN response (typically "Manufacturer,Model,Serial,Firmware")
    TVector<TString> parts;
    size_t start = 0;
    size_t end = 0;

    while ((end = in_strIdnResponse.find(',', start)) != TString::npos) {
        parts.push_back(in_strIdnResponse.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(in_strIdnResponse.substr(start));

    if (parts.size() >= 1) m_info.manufacturer = parts[0];
    if (parts.size() >= 2) m_info.model = parts[1];
    if (parts.size() >= 3) m_info.serialNumber = parts[2];
    if (parts.size() >= 4) m_info.firmwareVersion = parts[3];

    // Trim whitespace
    auto trim = [](TString& s) {
        while (!s.empty() && std::isspace(s.front())) s.erase(0, 1);
        while (!s.empty() && std::isspace(s.back())) s.pop_back();
    };

    trim(m_info.manufacturer);
    trim(m_info.model);
    trim(m_info.serialNumber);
    trim(m_info.firmwareVersion);
}

} // namespace TestMATE
