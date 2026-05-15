/**************************************************************************
 * File Name: IUIAdapter.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: UI adapter interface for Qt integration.
 *              Provides abstraction between core and UI layers.
 * Requirements: REQ-UI-001 to REQ-UI-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include "api/ITestMATEApi.h"
#include "core/reporting/IReportGenerator.h"
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: EUIEvent
 * Description: Events that can be sent to the UI
 **************************************************************************/
enum class EUIEvent {
    kTestStarted,
    kTestCompleted,
    kTestPaused,
    kTestResumed,
    kStepStarted,
    kStepCompleted,
    kProgressUpdate,
    kLogMessage,
    kErrorOccurred,
    kInstrumentConnected,
    kInstrumentDisconnected,
    kDataUpdate
};

/**************************************************************************
 * Struct: SUIEventData
 * Description: Data associated with UI events
 **************************************************************************/
struct SUIEventData {
    EUIEvent eventType;
    TString message;
    TDouble progress{0.0};
    TInt32 stepIndex{-1};
    ETestVerdict verdict{ETestVerdict::kNone};
    std::map<TString, TString> data;
};

/**************************************************************************
 * Interface: IUIAdapter
 * Description: Adapter interface for UI framework integration
 **************************************************************************/
class IUIAdapter {
public:
    virtual ~IUIAdapter() = default;

    // Event handling
    virtual void OnEvent(const SUIEventData& in_eventData) = 0;

    // Progress updates
    virtual void UpdateProgress(TDouble in_fProgress, const TString& in_strMessage) = 0;

    // Log display
    virtual void AppendLog(ELogLevel in_eLevel, const TString& in_strMessage) = 0;

    // Status updates
    virtual void UpdateStatus(const TString& in_strStatus) = 0;

    // Data display
    virtual void UpdateTestResults(const TVector<STestResult>& in_results) = 0;

    // Error handling
    virtual void ShowError(const TString& in_strTitle, const TString& in_strMessage) = 0;
    virtual void ShowWarning(const TString& in_strTitle, const TString& in_strMessage) = 0;
    virtual void ShowInfo(const TString& in_strTitle, const TString& in_strMessage) = 0;

    // Confirmation dialogs
    virtual bool AskConfirmation(const TString& in_strTitle, const TString& in_strMessage) = 0;
};

/**************************************************************************
 * Class: CUIBridge
 * Description: Bridge between core API and UI adapter
 **************************************************************************/
class CUIBridge {
public:
    static CUIBridge& GetInstance();

    CUIBridge(const CUIBridge&) = delete;
    CUIBridge& operator=(const CUIBridge&) = delete;

    void SetAdapter(IUIAdapter* in_pAdapter) { m_pAdapter = in_pAdapter; }
    [[nodiscard]] IUIAdapter* GetAdapter() const { return m_pAdapter; }

    // Connect to API events
    void ConnectToApi(ITestMATEApi* in_pApi);
    void Disconnect();

    // Send events to UI
    void SendEvent(const SUIEventData& in_eventData);
    void SendProgress(TDouble in_fProgress, const TString& in_strMessage);
    void SendLog(ELogLevel in_eLevel, const TString& in_strMessage);

private:
    CUIBridge() = default;
    ~CUIBridge() = default;

    IUIAdapter* m_pAdapter{nullptr};
    ITestMATEApi* m_pApi{nullptr};
    TVector<TUInt64> m_vecSubscriptionIds;
};

/**************************************************************************
 * Class: CConsoleUIAdapter
 * Description: Simple console-based UI adapter for testing
 **************************************************************************/
class CConsoleUIAdapter : public IUIAdapter {
public:
    void OnEvent(const SUIEventData& in_eventData) override;
    void UpdateProgress(TDouble in_fProgress, const TString& in_strMessage) override;
    void AppendLog(ELogLevel in_eLevel, const TString& in_strMessage) override;
    void UpdateStatus(const TString& in_strStatus) override;
    void UpdateTestResults(const TVector<STestResult>& in_results) override;
    void ShowError(const TString& in_strTitle, const TString& in_strMessage) override;
    void ShowWarning(const TString& in_strTitle, const TString& in_strMessage) override;
    void ShowInfo(const TString& in_strTitle, const TString& in_strMessage) override;
    bool AskConfirmation(const TString& in_strTitle, const TString& in_strMessage) override;
};

} // namespace TestMATE
