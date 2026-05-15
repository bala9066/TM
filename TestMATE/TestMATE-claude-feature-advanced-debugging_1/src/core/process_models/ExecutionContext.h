/**************************************************************************
 * File Name: ExecutionContext.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Execution context for test step execution.
 *              Provides access to resources, variables, and results.
 * Requirements: REQ-PM-058 to REQ-PM-065
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <any>
#include <atomic>
#include <mutex>

namespace TestMATE {

/**************************************************************************
 * Class: CExecutionContext
 * Description: Context object passed to test steps during execution.
 *              Provides thread-safe access to variables, results, and
 *              execution state.
 * Requirements: REQ-PM-058 to REQ-PM-065
 **************************************************************************/
class CExecutionContext {
public:
    /**************************************************************************
     * Function Name: CExecutionContext
     * Description: Creates execution context
     * Parameters:
     *   in_uiSocketId - Socket ID (0 for sequential)
     *   in_uiSiteId - Site ID (0 for non-semiconductor)
     **************************************************************************/
    explicit CExecutionContext(TSocketId in_uiSocketId = 0, TSiteId in_uiSiteId = 0);

    ~CExecutionContext() = default;

    // Non-copyable but movable
    CExecutionContext(const CExecutionContext&) = delete;
    CExecutionContext& operator=(const CExecutionContext&) = delete;
    CExecutionContext(CExecutionContext&&) = default;
    CExecutionContext& operator=(CExecutionContext&&) = default;

    //=========================================================================
    // Identity
    //=========================================================================

    [[nodiscard]] TSocketId GetSocketId() const { return m_uiSocketId; }
    [[nodiscard]] TSiteId GetSiteId() const { return m_uiSiteId; }
    [[nodiscard]] TExecutionId GetExecutionId() const { return m_uiExecutionId; }

    void SetExecutionId(TExecutionId in_uiId) { m_uiExecutionId = in_uiId; }

    //=========================================================================
    // Variables (Thread-Safe)
    //=========================================================================

    /**************************************************************************
     * Function Name: SetVariable
     * Description: Sets a variable value
     * Parameters:
     *   in_strName - Variable name
     *   in_value - Variable value
     * Requirements: REQ-PM-060
     **************************************************************************/
    template<typename T>
    void SetVariable(const TString& in_strName, T in_value) {
        std::lock_guard<std::mutex> lock(m_mutexVars);
        m_mapVariables[in_strName] = std::move(in_value);
    }

    /**************************************************************************
     * Function Name: GetVariable
     * Description: Gets a variable value
     * Parameters:
     *   in_strName - Variable name
     * Returns: Optional containing value if found
     * Requirements: REQ-PM-060
     **************************************************************************/
    template<typename T>
    [[nodiscard]] TOptional<T> GetVariable(const TString& in_strName) const {
        std::lock_guard<std::mutex> lock(m_mutexVars);
        auto it = m_mapVariables.find(in_strName);
        if (it != m_mapVariables.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    /**************************************************************************
     * Function Name: HasVariable
     * Description: Checks if variable exists
     * Parameters:
     *   in_strName - Variable name
     * Returns: true if variable exists
     **************************************************************************/
    [[nodiscard]] bool HasVariable(const TString& in_strName) const {
        std::lock_guard<std::mutex> lock(m_mutexVars);
        return m_mapVariables.find(in_strName) != m_mapVariables.end();
    }

    /**************************************************************************
     * Function Name: ClearVariable
     * Description: Removes a variable
     * Parameters:
     *   in_strName - Variable name
     **************************************************************************/
    void ClearVariable(const TString& in_strName) {
        std::lock_guard<std::mutex> lock(m_mutexVars);
        m_mapVariables.erase(in_strName);
    }

    /**************************************************************************
     * Function Name: ClearAllVariables
     * Description: Removes all variables
     **************************************************************************/
    void ClearAllVariables() {
        std::lock_guard<std::mutex> lock(m_mutexVars);
        m_mapVariables.clear();
    }

    //=========================================================================
    // Execution Control
    //=========================================================================

    /**************************************************************************
     * Function Name: RequestAbort
     * Description: Requests abort of current execution
     * Requirements: REQ-PM-062
     **************************************************************************/
    void RequestAbort() { m_bAbortRequested = true; }

    /**************************************************************************
     * Function Name: IsAbortRequested
     * Description: Checks if abort was requested
     * Returns: true if abort requested
     **************************************************************************/
    [[nodiscard]] bool IsAbortRequested() const { return m_bAbortRequested; }

    /**************************************************************************
     * Function Name: ClearAbortRequest
     * Description: Clears the abort request flag
     **************************************************************************/
    void ClearAbortRequest() { m_bAbortRequested = false; }

    //=========================================================================
    // Step Results
    //=========================================================================

    /**************************************************************************
     * Function Name: SetCurrentStepVerdict
     * Description: Sets verdict for current step
     * Parameters:
     *   in_eVerdict - Test verdict
     **************************************************************************/
    void SetCurrentStepVerdict(ETestVerdict in_eVerdict) { m_eCurrentVerdict = in_eVerdict; }

    /**************************************************************************
     * Function Name: GetCurrentStepVerdict
     * Description: Gets verdict for current step
     * Returns: Current step verdict
     **************************************************************************/
    [[nodiscard]] ETestVerdict GetCurrentStepVerdict() const { return m_eCurrentVerdict; }

    /**************************************************************************
     * Function Name: IncrementPassCount
     * Description: Increments pass counter
     **************************************************************************/
    void IncrementPassCount() { ++m_uiPassCount; }

    /**************************************************************************
     * Function Name: IncrementFailCount
     * Description: Increments fail counter
     **************************************************************************/
    void IncrementFailCount() { ++m_uiFailCount; }

    [[nodiscard]] TUInt32 GetPassCount() const { return m_uiPassCount; }
    [[nodiscard]] TUInt32 GetFailCount() const { return m_uiFailCount; }

    //=========================================================================
    // Timing
    //=========================================================================

    void SetStartTime(TTimePoint in_time) { m_startTime = in_time; }
    [[nodiscard]] TTimePoint GetStartTime() const { return m_startTime; }

private:
    TSocketId m_uiSocketId;
    TSiteId m_uiSiteId;
    TExecutionId m_uiExecutionId{0};

    TMap<TString, std::any> m_mapVariables;
    mutable std::mutex m_mutexVars;

    std::atomic<bool> m_bAbortRequested{false};
    ETestVerdict m_eCurrentVerdict{ETestVerdict::kNone};

    TUInt32 m_uiPassCount{0};
    TUInt32 m_uiFailCount{0};

    TTimePoint m_startTime;
};

} // namespace TestMATE
