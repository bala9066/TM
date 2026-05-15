/**************************************************************************
 * File Name: WaitStep.h
 * Description: Test step for time delays and synchronization
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"
#include <chrono>
#include <thread>

namespace TestMATE {

/**************************************************************************
 * Class: CWaitStep
 * Description: Implements time delay functionality
 *
 * Parameters:
 *   - duration_ms: Wait duration in milliseconds (required)
 *   - allow_abort: Allow step to be aborted during wait (default: true)
 *   - description: Optional wait description
 **************************************************************************/
class CWaitStep : public CTestStepBase {
public:
    explicit CWaitStep(const TString& in_strId = "WAIT-001",
                      const TString& in_strName = "Wait Step",
                      TInt64 in_durationMs = 1000);

    ~CWaitStep() override = default;

    /**************************************************************************
     * Function Name: Execute
     * Description: Performs the wait/delay
     **************************************************************************/
    CResult Execute(SStepResult& out_result) override;

    /**************************************************************************
     * Function Name: SetDuration
     * Description: Sets the wait duration
     **************************************************************************/
    void SetDuration(TInt64 in_durationMs);

    /**************************************************************************
     * Function Name: GetDuration
     * Description: Gets the configured wait duration
     **************************************************************************/
    [[nodiscard]] TInt64 GetDuration() const { return m_durationMs; }

    /**************************************************************************
     * Function Name: SetAllowAbort
     * Description: Sets whether wait can be interrupted
     **************************************************************************/
    void SetAllowAbort(bool in_bAllow) { m_bAllowAbort = in_bAllow; }

private:
    TInt64 m_durationMs;
    bool m_bAllowAbort{true};
    static constexpr TInt64 kAbortCheckIntervalMs = 100;  // Check abort every 100ms
};

} // namespace TestMATE
