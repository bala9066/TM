/**************************************************************************
 * File Name: SequentialModel.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Sequential process model for single-threaded test execution.
 *              Executes test steps one after another in defined order.
 * Requirements: REQ-PM-010 to REQ-PM-018
 **************************************************************************/

#pragma once

#include "ProcessModelBase.h"
#include <thread>

namespace TestMATE {

/**************************************************************************
 * Class: CSequentialModel
 * Description: Sequential execution model. Executes test steps one at a
 *              time in the order they appear in the test sequence.
 *              Suitable for single-device testing.
 * Requirements: REQ-PM-010 to REQ-PM-018
 **************************************************************************/
class CSequentialModel : public CProcessModelBase {
public:
    CSequentialModel();
    ~CSequentialModel() override;

    //=========================================================================
    // IProcessModel Implementation
    //=========================================================================

    [[nodiscard]] EProcessModelType GetType() const override {
        return EProcessModelType::kSequential;
    }

    [[nodiscard]] TString GetName() const override {
        return "Sequential";
    }

    [[nodiscard]] TString GetDescription() const override {
        return "Single-threaded sequential test execution model";
    }

    /**************************************************************************
     * Function Name: Start
     * Description: Starts sequential test execution in background thread
     * Returns: Result indicating if execution started successfully
     * Requirements: REQ-PM-010
     **************************************************************************/
    CResult Start() override;

    /**************************************************************************
     * Function Name: WaitForCompletion
     * Description: Blocks until execution completes
     * Parameters:
     *   in_timeoutMs - Timeout in milliseconds (0 = infinite)
     * Returns: true if completed, false if timeout
     **************************************************************************/
    bool WaitForCompletion(TInt64 in_timeoutMs = 0);

    /**************************************************************************
     * Function Name: GetContext
     * Description: Returns the execution context
     * Returns: Reference to execution context
     **************************************************************************/
    [[nodiscard]] CExecutionContext& GetContext() { return m_context; }
    [[nodiscard]] const CExecutionContext& GetContext() const { return m_context; }

private:
    /**************************************************************************
     * Function Name: ExecutionThreadFunc
     * Description: Main execution thread function
     **************************************************************************/
    void ExecutionThreadFunc();

    CExecutionContext m_context;
    TUniquePtr<std::thread> m_pExecutionThread;
};

} // namespace TestMATE
