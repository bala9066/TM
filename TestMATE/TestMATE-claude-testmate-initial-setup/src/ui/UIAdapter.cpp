/**************************************************************************
 * File Name: UIAdapter.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: UI adapter implementations
 * Requirements: REQ-UI-001 to REQ-UI-030
 **************************************************************************/

#include "IUIAdapter.h"
#include <iostream>
#include <iomanip>

namespace TestMATE {

//=============================================================================
// CUIBridge
//=============================================================================

CUIBridge& CUIBridge::GetInstance() {
    static CUIBridge instance;
    return instance;
}

void CUIBridge::ConnectToApi(ITestMATEApi* in_pApi) {
    if (!in_pApi) return;

    m_pApi = in_pApi;

    // Subscribe to API events
    m_vecSubscriptionIds.push_back(
        m_pApi->Subscribe(EApiEvent::kTestStarted, [this](EApiEvent, const TString& data) {
            SUIEventData event;
            event.eventType = EUIEvent::kTestStarted;
            event.message = data;
            SendEvent(event);
        }));

    m_vecSubscriptionIds.push_back(
        m_pApi->Subscribe(EApiEvent::kTestCompleted, [this](EApiEvent, const TString& data) {
            SUIEventData event;
            event.eventType = EUIEvent::kTestCompleted;
            event.message = data;
            SendEvent(event);
        }));

    m_vecSubscriptionIds.push_back(
        m_pApi->Subscribe(EApiEvent::kStepCompleted, [this](EApiEvent, const TString& data) {
            SUIEventData event;
            event.eventType = EUIEvent::kStepCompleted;
            event.message = data;
            SendEvent(event);
        }));

    m_vecSubscriptionIds.push_back(
        m_pApi->Subscribe(EApiEvent::kProgressUpdate, [this](EApiEvent, const TString& data) {
            SUIEventData event;
            event.eventType = EUIEvent::kProgressUpdate;
            event.message = data;
            SendEvent(event);
        }));

    m_pApi->SetLogCallback([this](ELogLevel level, const TString& cat, const TString& msg) {
        SendLog(level, "[" + cat + "] " + msg);
    });

    m_pApi->SetProgressCallback([this](TDouble progress, const TString& msg) {
        SendProgress(progress, msg);
    });
}

void CUIBridge::Disconnect() {
    if (m_pApi) {
        for (TUInt64 id : m_vecSubscriptionIds) {
            m_pApi->Unsubscribe(id);
        }
        m_vecSubscriptionIds.clear();
        m_pApi = nullptr;
    }
}

void CUIBridge::SendEvent(const SUIEventData& in_eventData) {
    if (m_pAdapter) {
        m_pAdapter->OnEvent(in_eventData);
    }
}

void CUIBridge::SendProgress(TDouble in_fProgress, const TString& in_strMessage) {
    if (m_pAdapter) {
        m_pAdapter->UpdateProgress(in_fProgress, in_strMessage);
    }
}

void CUIBridge::SendLog(ELogLevel in_eLevel, const TString& in_strMessage) {
    if (m_pAdapter) {
        m_pAdapter->AppendLog(in_eLevel, in_strMessage);
    }
}

//=============================================================================
// CConsoleUIAdapter
//=============================================================================

void CConsoleUIAdapter::OnEvent(const SUIEventData& in_eventData) {
    TString eventName;
    switch (in_eventData.eventType) {
        case EUIEvent::kTestStarted: eventName = "TEST_STARTED"; break;
        case EUIEvent::kTestCompleted: eventName = "TEST_COMPLETED"; break;
        case EUIEvent::kStepStarted: eventName = "STEP_STARTED"; break;
        case EUIEvent::kStepCompleted: eventName = "STEP_COMPLETED"; break;
        case EUIEvent::kProgressUpdate: eventName = "PROGRESS"; break;
        default: eventName = "EVENT";
    }
    std::cout << "[" << eventName << "] " << in_eventData.message << std::endl;
}

void CConsoleUIAdapter::UpdateProgress(TDouble in_fProgress, const TString& in_strMessage) {
    std::cout << "\rProgress: " << std::fixed << std::setprecision(1)
              << (in_fProgress * 100.0) << "% - " << in_strMessage << std::flush;
}

void CConsoleUIAdapter::AppendLog(ELogLevel in_eLevel, const TString& in_strMessage) {
    TString levelStr;
    switch (in_eLevel) {
        case ELogLevel::kDebug: levelStr = "DEBUG"; break;
        case ELogLevel::kInfo: levelStr = "INFO"; break;
        case ELogLevel::kWarning: levelStr = "WARN"; break;
        case ELogLevel::kError: levelStr = "ERROR"; break;
        case ELogLevel::kFatal: levelStr = "CRIT"; break;
        default: levelStr = "LOG";
    }
    std::cout << "[" << levelStr << "] " << in_strMessage << std::endl;
}

void CConsoleUIAdapter::UpdateStatus(const TString& in_strStatus) {
    std::cout << "Status: " << in_strStatus << std::endl;
}

void CConsoleUIAdapter::UpdateTestResults(const TVector<STestResult>& in_results) {
    std::cout << "\n=== Test Results ===" << std::endl;
    for (const auto& r : in_results) {
        std::cout << r.stepName << ": "
                  << (r.verdict == ETestVerdict::kPass ? "PASS" : "FAIL")
                  << " (" << r.durationMs << "ms)" << std::endl;
    }
}

void CConsoleUIAdapter::ShowError(const TString& in_strTitle, const TString& in_strMessage) {
    std::cerr << "[ERROR] " << in_strTitle << ": " << in_strMessage << std::endl;
}

void CConsoleUIAdapter::ShowWarning(const TString& in_strTitle, const TString& in_strMessage) {
    std::cout << "[WARNING] " << in_strTitle << ": " << in_strMessage << std::endl;
}

void CConsoleUIAdapter::ShowInfo(const TString& in_strTitle, const TString& in_strMessage) {
    std::cout << "[INFO] " << in_strTitle << ": " << in_strMessage << std::endl;
}

bool CConsoleUIAdapter::AskConfirmation(const TString& in_strTitle, const TString& in_strMessage) {
    std::cout << "[CONFIRM] " << in_strTitle << ": " << in_strMessage << " (y/n): ";
    char response;
    std::cin >> response;
    return response == 'y' || response == 'Y';
}

} // namespace TestMATE
