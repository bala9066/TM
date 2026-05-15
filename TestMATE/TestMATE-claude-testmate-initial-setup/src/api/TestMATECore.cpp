/**************************************************************************
 * File Name: TestMATECore.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of TestMATE core API
 * Requirements: REQ-API-001 to REQ-API-030
 **************************************************************************/

#include "TestMATECore.h"
#include "utils/LogManager.h"
#include "core/plugins/PluginManager.h"

namespace TestMATE {

namespace {
    constexpr const char* kVersion = "1.0.0";
}

CTestMATECore& CTestMATECore::GetInstance() {
    static CTestMATECore instance;
    return instance;
}

CResult CTestMATECore::Initialize() {
    if (m_bInitialized) {
        return TESTMATE_SUCCESS();
    }

    CLogManager::GetInstance().LogInfo("TestMATECore", "Initializing v{}", kVersion);

    // Initialize subsystems
    // Plugin manager initializes on first access (singleton)
    // Log manager initializes on first access (singleton)

    m_bInitialized = true;

    NotifyEvent(EApiEvent::kStateChanged, "initialized");

    CLogManager::GetInstance().LogInfo("TestMATECore", "Initialization complete");

    return TESTMATE_SUCCESS();
}

CResult CTestMATECore::Shutdown() {
    if (!m_bInitialized) {
        return TESTMATE_SUCCESS();
    }

    CLogManager::GetInstance().LogInfo("TestMATECore", "Shutting down");

    // Stop any running tests
    StopTest();

    // Unload all plugins
    CPluginManager::GetInstance().UnloadAll();

    // Clear subscriptions
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_mapSubscriptions.clear();
        m_progressCallback = nullptr;
        m_logCallback = nullptr;
    }

    m_bInitialized = false;

    CLogManager::GetInstance().LogInfo("TestMATECore", "Shutdown complete");

    return TESTMATE_SUCCESS();
}

bool CTestMATECore::IsInitialized() const {
    return m_bInitialized;
}

TString CTestMATECore::GetVersion() const {
    return kVersion;
}

CResult CTestMATECore::LoadSequence(const TString& in_strPath) {
    if (!m_bInitialized) {
        return TESTMATE_FAILURE(EErrorCode::kNotInitialized,
                                "TestMATE core not initialized");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // TODO: Implement actual sequence loading
    m_strCurrentSequencePath = in_strPath;

    CLogManager::GetInstance().LogInfo("TestMATECore",
        "Loaded sequence: {}", in_strPath);

    return TESTMATE_SUCCESS();
}

CResult CTestMATECore::StartTest(const STestConfiguration& in_config) {
    if (!m_bInitialized) {
        return TESTMATE_FAILURE(EErrorCode::kNotInitialized,
                                "TestMATE core not initialized");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status.state == EExecutionState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Test already running");
    }

    // Apply configuration variables
    for (const auto& [name, value] : in_config.variables) {
        m_mapVariables[name] = value;
    }

    // Update status
    m_status.state = EExecutionState::kRunning;
    m_status.currentStep = 0;
    m_status.passCount = 0;
    m_status.failCount = 0;
    m_status.progress = 0.0;

    NotifyEvent(EApiEvent::kTestStarted, in_config.sequencePath);
    NotifyProgress(0.0, "Test started");

    CLogManager::GetInstance().LogInfo("TestMATECore",
        "Test started: {}", in_config.sequencePath);

    return TESTMATE_SUCCESS();
}

CResult CTestMATECore::StopTest() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status.state != EExecutionState::kRunning &&
        m_status.state != EExecutionState::kPaused) {
        return TESTMATE_SUCCESS();  // Not running, nothing to stop
    }

    m_status.state = EExecutionState::kAborted;

    NotifyEvent(EApiEvent::kTestCompleted, "stopped");

    CLogManager::GetInstance().LogInfo("TestMATECore", "Test stopped");

    return TESTMATE_SUCCESS();
}

CResult CTestMATECore::PauseTest() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status.state != EExecutionState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Test not running");
    }

    m_status.state = EExecutionState::kPaused;

    NotifyEvent(EApiEvent::kStateChanged, "paused");

    return TESTMATE_SUCCESS();
}

CResult CTestMATECore::ResumeTest() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status.state != EExecutionState::kPaused) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Test not paused");
    }

    m_status.state = EExecutionState::kRunning;

    NotifyEvent(EApiEvent::kStateChanged, "resumed");

    return TESTMATE_SUCCESS();
}

STestStatus CTestMATECore::GetTestStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

TUInt64 CTestMATECore::Subscribe(EApiEvent in_eEvent, FApiEventCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    TUInt64 subId = m_uiNextSubId++;
    m_mapSubscriptions[subId] = {in_eEvent, std::move(in_callback)};

    return subId;
}

void CTestMATECore::Unsubscribe(TUInt64 in_subscriptionId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_mapSubscriptions.erase(in_subscriptionId);
}

void CTestMATECore::SetProgressCallback(FProgressCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_progressCallback = std::move(in_callback);
}

void CTestMATECore::SetLogCallback(FLogCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_logCallback = std::move(in_callback);
}

void CTestMATECore::SetVariable(const TString& in_strName, const TString& in_strValue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapVariables[in_strName] = in_strValue;
}

std::optional<TString> CTestMATECore::GetVariable(const TString& in_strName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapVariables.find(in_strName);
    if (it != m_mapVariables.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CTestMATECore::ClearVariables() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapVariables.clear();
}

CResult CTestMATECore::LoadPlugin(const TString& in_strPath) {
    return CPluginManager::GetInstance().LoadPlugin(in_strPath);
}

TVector<TString> CTestMATECore::GetLoadedPlugins() const {
    return CPluginManager::GetInstance().GetLoadedPlugins();
}

void CTestMATECore::NotifyEvent(EApiEvent in_eEvent, const TString& in_strData) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    for (const auto& [id, sub] : m_mapSubscriptions) {
        if (sub.first == in_eEvent && sub.second) {
            try {
                sub.second(in_eEvent, in_strData);
            } catch (...) {
                // Ignore callback exceptions
            }
        }
    }
}

void CTestMATECore::NotifyProgress(TDouble in_fProgress, const TString& in_strMessage) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    if (m_progressCallback) {
        try {
            m_progressCallback(in_fProgress, in_strMessage);
        } catch (...) {
            // Ignore callback exceptions
        }
    }
}

} // namespace TestMATE
