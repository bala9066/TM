/**************************************************************************
 * File Name: TestExecutor.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Test execution engine - runs test sequences.
 * Requirements: REQ-EXEC-001 to REQ-EXEC-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include "core/test_sequence/TestSequence.h"
#include "core/test_sequence/ITestStep.h"
#include "core/process_models/IProcessModel.h"
#include "core/reporting/IReportGenerator.h"
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>

namespace TestMATE {

// Forward declaration for debug support
class CDebugSession;

/**************************************************************************
 * Struct: SExecutionConfig
 * Description: Configuration for test execution
 **************************************************************************/
struct SExecutionConfig {
    EProcessModelType processModel{EProcessModelType::kSequential};
    TUInt32 socketCount{1};
    bool stopOnFirstFailure{false};
    bool skipDisabledSteps{true};
    TInt64 stepTimeoutMs{60000};
    TInt64 totalTimeoutMs{0};
    std::map<TString, TString> variables;
};

/**************************************************************************
 * Struct: SExecutionStatus
 * Description: Current execution status
 **************************************************************************/
struct SExecutionStatus {
    EExecutionState state{EExecutionState::kIdle};
    TUInt32 currentStepIndex{0};
    TUInt32 totalSteps{0};
    TUInt32 passCount{0};
    TUInt32 failCount{0};
    TUInt32 skipCount{0};
    TDouble progress{0.0};
    TString currentStepName;
    TInt64 elapsedMs{0};
};

/**************************************************************************
 * Callback types
 **************************************************************************/
using FStepStartCallback = std::function<void(TUInt32, const TString&)>;
using FStepCompleteCallback = std::function<void(TUInt32, const STestResult&)>;
using FProgressCallback = std::function<void(TDouble, const TString&)>;
using FExecutionCompleteCallback = std::function<void(bool, const STestReport&)>;

/**************************************************************************
 * Class: CTestExecutor
 * Description: Executes test sequences with configurable process models
 **************************************************************************/
class CTestExecutor {
public:
    CTestExecutor();
    ~CTestExecutor();

    //=========================================================================
    // Execution Control
    //=========================================================================

    CResult Execute(CTestSequence& io_sequence, const SExecutionConfig& in_config);
    CResult Stop();
    CResult Pause();
    CResult Resume();

    [[nodiscard]] SExecutionStatus GetStatus() const;
    [[nodiscard]] bool IsRunning() const;
    [[nodiscard]] bool IsPaused() const;

    //=========================================================================
    // Results
    //=========================================================================

    // Return by value under the lock: the execution thread mutates these
    // members concurrently, so handing out a reference would be a race.
    [[nodiscard]] STestReport GetReport() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_report;
    }
    [[nodiscard]] TVector<STestResult> GetResults() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_vecResults;
    }

    //=========================================================================
    // Callbacks
    //=========================================================================

    void SetStepStartCallback(FStepStartCallback in_callback);
    void SetStepCompleteCallback(FStepCompleteCallback in_callback);
    void SetProgressCallback(FProgressCallback in_callback);
    void SetExecutionCompleteCallback(FExecutionCompleteCallback in_callback);

    //=========================================================================
    // Debug Support
    //=========================================================================

    void SetDebugSession(std::shared_ptr<CDebugSession> in_debugSession);
    [[nodiscard]] std::shared_ptr<CDebugSession> GetDebugSession() const { return m_debugSession; }

private:
    CResult ExecuteSequential(CTestSequence& io_sequence);
    CResult ExecuteStep(ITestStep* io_pStep, TUInt32 in_uiIndex);
    void UpdateProgress();
    void NotifyStepStart(TUInt32 in_uiIndex, const TString& in_strName);
    void NotifyStepComplete(TUInt32 in_uiIndex, const STestResult& in_result);
    void NotifyProgress(TDouble in_fProgress, const TString& in_strMessage);
    void FinalizeReport(bool in_bSuccess);

    mutable std::mutex m_mutex;
    std::atomic<EExecutionState> m_eState{EExecutionState::kIdle};
    std::atomic<bool> m_bAbortRequested{false};
    std::atomic<bool> m_bPauseRequested{false};

    SExecutionConfig m_config;
    SExecutionStatus m_status;
    STestReport m_report;
    TVector<STestResult> m_vecResults;
    TWallClock m_startTime;

    FStepStartCallback m_stepStartCallback;
    FStepCompleteCallback m_stepCompleteCallback;
    FProgressCallback m_progressCallback;
    FExecutionCompleteCallback m_executionCompleteCallback;

    // Debug support
    std::shared_ptr<CDebugSession> m_debugSession;
};

} // namespace TestMATE
