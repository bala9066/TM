/**************************************************************************
 * File Name: WaitStep.cpp
 * Description: Implementation of WaitStep
 **************************************************************************/

#include "WaitStep.h"
#include <sstream>

namespace TestMATE {

CWaitStep::CWaitStep(const TString& in_strId,
                     const TString& in_strName,
                     TInt64 in_durationMs)
    : CTestStepBase(in_strId, in_strName, EStepType::kWait)
    , m_durationMs(in_durationMs)
{
    // Add parameter definitions
    SStepParameter durationParam;
    durationParam.name = "duration_ms";
    durationParam.type = "int64";
    durationParam.required = true;
    durationParam.description = "Wait duration in milliseconds";
    durationParam.defaultValue = "1000";
    std::ostringstream oss;
    oss << m_durationMs;
    durationParam.value = oss.str();
    AddParameter(durationParam);

    SStepParameter allowAbortParam;
    allowAbortParam.name = "allow_abort";
    allowAbortParam.type = "bool";
    allowAbortParam.required = false;
    allowAbortParam.description = "Allow step to be aborted during wait";
    allowAbortParam.defaultValue = "true";
    allowAbortParam.value = m_bAllowAbort ? "true" : "false";
    AddParameter(allowAbortParam);

    std::ostringstream descOss;
    descOss << "Wait for " << m_durationMs << " milliseconds";
    SetDescription(descOss.str());
}

CResult CWaitStep::Execute(SStepResult& out_result) {
    ResetAbort();
    out_result.startTime = std::chrono::steady_clock::now();

    // Update duration from parameter if set
    auto durationParam = GetParameter("duration_ms");
    if (durationParam.has_value()) {
        try {
            m_durationMs = std::stoll(durationParam.value());
        } catch (...) {
            out_result.verdict = ETestVerdict::kError;
            out_result.message = "Invalid duration_ms parameter";
            out_result.endTime = std::chrono::steady_clock::now();
            out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                out_result.endTime - out_result.startTime).count();
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid duration_ms");
        }
    }

    // Update allow_abort from parameter if set
    auto abortParam = GetParameter("allow_abort");
    if (abortParam.has_value()) {
        m_bAllowAbort = (abortParam.value() == "true");
    }

    // Perform the wait with abort checking
    TInt64 remainingMs = m_durationMs;
    auto startWait = std::chrono::steady_clock::now();

    while (remainingMs > 0) {
        // Check if aborted
        if (m_bAllowAbort && IsAborted()) {
            out_result.verdict = ETestVerdict::kAborted;
            out_result.message = "Wait aborted by user";
            out_result.endTime = std::chrono::steady_clock::now();
            out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                out_result.endTime - out_result.startTime).count();
            return TESTMATE_FAILURE(EErrorCode::kStepAborted, "Wait step aborted");
        }

        // Wait for shorter of: remaining time or abort check interval
        TInt64 sleepMs = (remainingMs < kAbortCheckIntervalMs) ? remainingMs : kAbortCheckIntervalMs;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

        // Update remaining time
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startWait).count();
        remainingMs = m_durationMs - elapsed;
    }

    // Wait completed successfully
    out_result.verdict = ETestVerdict::kPass;
    std::ostringstream oss;
    oss << "Waited " << m_durationMs << " ms";
    out_result.message = oss.str();
    out_result.endTime = std::chrono::steady_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return TESTMATE_SUCCESS();
}

void CWaitStep::SetDuration(TInt64 in_durationMs) {
    m_durationMs = in_durationMs;
    std::ostringstream oss;
    oss << m_durationMs;
    SetParameter("duration_ms", oss.str());
}

} // namespace TestMATE
