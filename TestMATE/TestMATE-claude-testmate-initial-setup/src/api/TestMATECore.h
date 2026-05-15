/**************************************************************************
 * File Name: TestMATECore.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Main TestMATE core implementation providing the API.
 * Requirements: REQ-API-001 to REQ-API-030
 **************************************************************************/

#pragma once

#include "ITestMATEApi.h"
#include <map>
#include <mutex>
#include <atomic>

namespace TestMATE {

/**************************************************************************
 * Class: CTestMATECore
 * Description: Singleton implementation of the TestMATE API
 * Requirements: REQ-API-001 to REQ-API-030
 **************************************************************************/
class CTestMATECore : public ITestMATEApi {
public:
    /**************************************************************************
     * Function Name: GetInstance
     * Description: Returns singleton instance
     **************************************************************************/
    static CTestMATECore& GetInstance();

    // Non-copyable
    CTestMATECore(const CTestMATECore&) = delete;
    CTestMATECore& operator=(const CTestMATECore&) = delete;

    //=========================================================================
    // ITestMATEApi Implementation
    //=========================================================================

    CResult Initialize() override;
    CResult Shutdown() override;
    [[nodiscard]] bool IsInitialized() const override;
    [[nodiscard]] TString GetVersion() const override;

    CResult LoadSequence(const TString& in_strPath) override;
    CResult StartTest(const STestConfiguration& in_config) override;
    CResult StopTest() override;
    CResult PauseTest() override;
    CResult ResumeTest() override;
    [[nodiscard]] STestStatus GetTestStatus() const override;

    TUInt64 Subscribe(EApiEvent in_eEvent, FApiEventCallback in_callback) override;
    void Unsubscribe(TUInt64 in_subscriptionId) override;
    void SetProgressCallback(FProgressCallback in_callback) override;
    void SetLogCallback(FLogCallback in_callback) override;

    void SetVariable(const TString& in_strName, const TString& in_strValue) override;
    [[nodiscard]] std::optional<TString> GetVariable(const TString& in_strName) const override;
    void ClearVariables() override;

    CResult LoadPlugin(const TString& in_strPath) override;
    [[nodiscard]] TVector<TString> GetLoadedPlugins() const override;

private:
    CTestMATECore() = default;
    ~CTestMATECore() = default;

    void NotifyEvent(EApiEvent in_eEvent, const TString& in_strData);
    void NotifyProgress(TDouble in_fProgress, const TString& in_strMessage);

    std::atomic<bool> m_bInitialized{false};

    mutable std::mutex m_mutex;
    mutable std::mutex m_callbackMutex;

    std::map<TString, TString> m_mapVariables;

    // Event subscriptions
    std::atomic<TUInt64> m_uiNextSubId{1};
    std::map<TUInt64, std::pair<EApiEvent, FApiEventCallback>> m_mapSubscriptions;
    FProgressCallback m_progressCallback;
    FLogCallback m_logCallback;

    // Current state
    STestStatus m_status;
    TString m_strCurrentSequencePath;
};

} // namespace TestMATE
