/**************************************************************************
 * File Name: TestExecutor.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Test execution engine implementation
 **************************************************************************/

#include "TestExecutor.h"
#include "utils/LogManager.h"
#include <chrono>
#include <thread>

namespace TestMATE {

CTestExecutor::CTestExecutor() = default;
CTestExecutor::~CTestExecutor() {
    if (IsRunning()) {
        Stop();
    }
}

CResult CTestExecutor::Execute(CTestSequence& io_sequence, const SExecutionConfig& in_config) {
    if (IsRunning()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Execution already in progress");
    }

    m_config = in_config;
    m_bAbortRequested = false;
    m_bPauseRequested = false;
    m_vecResults.clear();
    m_startTime = std::chrono::steady_clock::now();

    // Initialize status
    m_status.state = EExecutionState::kRunning;
    m_status.currentStepIndex = 0;
    m_status.totalSteps = io_sequence.GetStepCount();
    m_status.passCount = 0;
    m_status.failCount = 0;
    m_status.skipCount = 0;
    m_status.progress = 0.0;

    // Initialize report
    m_report = STestReport();
    m_report.sequenceName = io_sequence.GetName();
    m_report.startTime = m_startTime;
    m_report.totalSteps = m_status.totalSteps;

    m_eState = EExecutionState::kRunning;

    CLogManager::GetInstance().LogInfo("TestExecutor",
        "Starting execution: {} ({} steps)", io_sequence.GetName(), m_status.totalSteps);

    CResult result;
    switch (in_config.processModel) {
        case EProcessModelType::kSequential:
        default:
            result = ExecuteSequential(io_sequence);
            break;
    }

    bool success = result.IsSuccess() && m_status.failCount == 0;
    FinalizeReport(success);

    m_eState = m_bAbortRequested ? EExecutionState::kAborted : EExecutionState::kCompleted;

    if (m_executionCompleteCallback) {
        m_executionCompleteCallback(success, m_report);
    }

    CLogManager::GetInstance().LogInfo("TestExecutor",
        "Execution complete: {} pass, {} fail", m_status.passCount, m_status.failCount);

    return result;
}

CResult CTestExecutor::ExecuteSequential(CTestSequence& io_sequence) {
    for (TUInt32 i = 0; i < io_sequence.GetStepCount(); ++i) {
        if (m_bAbortRequested) {
            return TESTMATE_FAILURE(EErrorCode::kStepAborted, "Execution aborted");
        }

        // Handle pause
        while (m_bPauseRequested && !m_bAbortRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        ITestStep* pStep = io_sequence.GetStep(i);
        if (!pStep) continue;

        // Skip disabled steps
        if (!pStep->IsEnabled() && m_config.skipDisabledSteps) {
            STestResult skipResult;
            skipResult.stepId = pStep->GetId();
            skipResult.stepName = pStep->GetName();
            skipResult.verdict = ETestVerdict::kSkipped;
            skipResult.message = "Step disabled";

            m_vecResults.push_back(skipResult);
            m_status.skipCount++;
            continue;
        }

        auto result = ExecuteStep(pStep, i);

        if (result.IsFailure() && m_config.stopOnFirstFailure) {
            return result;
        }

        UpdateProgress();
    }

    return TESTMATE_SUCCESS();
}

CResult CTestExecutor::ExecuteStep(ITestStep* io_pStep, TUInt32 in_uiIndex) {
    m_status.currentStepIndex = in_uiIndex;
    m_status.currentStepName = io_pStep->GetName();

    NotifyStepStart(in_uiIndex, io_pStep->GetName());

    SStepResult stepResult;
    stepResult.startTime = std::chrono::steady_clock::now();

    CResult execResult = io_pStep->Execute(stepResult);

    stepResult.endTime = std::chrono::steady_clock::now();
    stepResult.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        stepResult.endTime - stepResult.startTime).count();

    if (execResult.IsFailure()) {
        stepResult.verdict = ETestVerdict::kError;
        stepResult.message = execResult.GetMessage();
    }

    // Convert to STestResult for reporting
    STestResult result;
    result.stepId = io_pStep->GetId();
    result.stepName = io_pStep->GetName();
    result.verdict = stepResult.verdict;
    result.message = stepResult.message;
    result.startTime = stepResult.startTime;
    result.endTime = stepResult.endTime;
    result.durationMs = stepResult.durationMs;
    result.measurements = stepResult.measurements;

    m_vecResults.push_back(result);

    switch (result.verdict) {
        case ETestVerdict::kPass:
            m_status.passCount++;
            break;
        case ETestVerdict::kFail:
        case ETestVerdict::kError:
            m_status.failCount++;
            break;
        case ETestVerdict::kSkipped:
            m_status.skipCount++;
            break;
        default:
            break;
    }

    NotifyStepComplete(in_uiIndex, result);

    return (result.verdict == ETestVerdict::kPass || result.verdict == ETestVerdict::kSkipped)
        ? TESTMATE_SUCCESS()
        : TESTMATE_FAILURE(EErrorCode::kStepFailed, result.message);
}

CResult CTestExecutor::Stop() {
    m_bAbortRequested = true;
    m_bPauseRequested = false;
    return TESTMATE_SUCCESS();
}

CResult CTestExecutor::Pause() {
    if (!IsRunning()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Not running");
    }
    m_bPauseRequested = true;
    m_eState = EExecutionState::kPaused;
    return TESTMATE_SUCCESS();
}

CResult CTestExecutor::Resume() {
    if (!IsPaused()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Not paused");
    }
    m_bPauseRequested = false;
    m_eState = EExecutionState::kRunning;
    return TESTMATE_SUCCESS();
}

SExecutionStatus CTestExecutor::GetStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

bool CTestExecutor::IsRunning() const {
    return m_eState == EExecutionState::kRunning;
}

bool CTestExecutor::IsPaused() const {
    return m_eState == EExecutionState::kPaused;
}

void CTestExecutor::UpdateProgress() {
    if (m_status.totalSteps > 0) {
        m_status.progress = static_cast<TDouble>(m_status.currentStepIndex + 1) / m_status.totalSteps;
    }

    auto now = std::chrono::steady_clock::now();
    m_status.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_startTime).count();

    NotifyProgress(m_status.progress, m_status.currentStepName);
}

void CTestExecutor::NotifyStepStart(TUInt32 in_uiIndex, const TString& in_strName) {
    if (m_stepStartCallback) {
        m_stepStartCallback(in_uiIndex, in_strName);
    }
}

void CTestExecutor::NotifyStepComplete(TUInt32 in_uiIndex, const STestResult& in_result) {
    if (m_stepCompleteCallback) {
        m_stepCompleteCallback(in_uiIndex, in_result);
    }
}

void CTestExecutor::NotifyProgress(TDouble in_fProgress, const TString& in_strMessage) {
    if (m_progressCallback) {
        m_progressCallback(in_fProgress, in_strMessage);
    }
}

void CTestExecutor::FinalizeReport(bool in_bSuccess) {
    m_report.endTime = std::chrono::steady_clock::now();
    m_report.totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        m_report.endTime - m_report.startTime).count();

    m_report.passCount = m_status.passCount;
    m_report.failCount = m_status.failCount;
    m_report.skipCount = m_status.skipCount;
    m_report.overallVerdict = in_bSuccess ? ETestVerdict::kPass : ETestVerdict::kFail;

    for (const auto& result : m_vecResults) {
        STestResult reportResult;
        reportResult.stepId = result.stepId;
        reportResult.stepName = result.stepName;
        reportResult.verdict = result.verdict;
        reportResult.message = result.message;
        reportResult.durationMs = result.durationMs;
        reportResult.startTime = result.startTime;
        reportResult.endTime = result.endTime;
        m_report.results.push_back(reportResult);
    }
}

void CTestExecutor::SetStepStartCallback(FStepStartCallback in_callback) {
    m_stepStartCallback = std::move(in_callback);
}

void CTestExecutor::SetStepCompleteCallback(FStepCompleteCallback in_callback) {
    m_stepCompleteCallback = std::move(in_callback);
}

void CTestExecutor::SetProgressCallback(FProgressCallback in_callback) {
    m_progressCallback = std::move(in_callback);
}

void CTestExecutor::SetExecutionCompleteCallback(FExecutionCompleteCallback in_callback) {
    m_executionCompleteCallback = std::move(in_callback);
}

} // namespace TestMATE
