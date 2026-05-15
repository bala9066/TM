/**************************************************************************
 * File Name: ITestStep.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Interface for test steps - the fundamental execution unit.
 * Requirements: REQ-STEP-001 to REQ-STEP-020
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <atomic>
#include <map>
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: EStepType
 * Description: Types of test steps
 **************************************************************************/
enum class EStepType {
    kAction,        // Performs an action
    kValidation,    // Validates a condition
    kMeasurement,   // Takes a measurement
    kSequence,      // Contains sub-steps
    kConditional,   // If/else branching
    kLoop,          // Looping construct
    kCall,          // Calls another sequence
    kWait,          // Time delay
    kSync,          // Synchronization point
    kCustom         // Plugin-defined step
};

/**************************************************************************
 * Struct: SStepParameter
 * Description: Parameter definition for a test step
 **************************************************************************/
struct SStepParameter {
    TString name;
    TString value;
    TString type;           // string, int, float, bool, etc.
    TString defaultValue;
    bool required{false};
    TString description;
};

/**************************************************************************
 * Struct: SStepResult
 * Description: Result of step execution
 **************************************************************************/
struct SStepResult {
    ETestVerdict verdict{ETestVerdict::kNone};
    TString message;
    std::map<TString, TString> measurements;
    TInt64 durationMs{0};
    TTimePoint startTime;
    TTimePoint endTime;
};

/**************************************************************************
 * Interface: ITestStep
 * Description: Base interface for all test steps
 * Requirements: REQ-STEP-001 to REQ-STEP-020
 **************************************************************************/
class ITestStep {
public:
    virtual ~ITestStep() = default;

    //=========================================================================
    // Identification
    //=========================================================================

    /**************************************************************************
     * Function Name: GetId
     * Description: Returns unique step identifier
     **************************************************************************/
    [[nodiscard]] virtual TString GetId() const = 0;

    /**************************************************************************
     * Function Name: GetName
     * Description: Returns step display name
     **************************************************************************/
    [[nodiscard]] virtual TString GetName() const = 0;

    /**************************************************************************
     * Function Name: GetType
     * Description: Returns step type
     **************************************************************************/
    [[nodiscard]] virtual EStepType GetType() const = 0;

    /**************************************************************************
     * Function Name: GetDescription
     * Description: Returns step description
     **************************************************************************/
    [[nodiscard]] virtual TString GetDescription() const = 0;

    //=========================================================================
    // Parameters
    //=========================================================================

    /**************************************************************************
     * Function Name: GetParameters
     * Description: Returns parameter definitions
     **************************************************************************/
    [[nodiscard]] virtual TVector<SStepParameter> GetParameters() const = 0;

    /**************************************************************************
     * Function Name: SetParameter
     * Description: Sets a parameter value
     **************************************************************************/
    virtual CResult SetParameter(const TString& in_strName,
                                  const TString& in_strValue) = 0;

    /**************************************************************************
     * Function Name: GetParameter
     * Description: Gets a parameter value
     **************************************************************************/
    [[nodiscard]] virtual std::optional<TString> GetParameter(
        const TString& in_strName) const = 0;

    //=========================================================================
    // Execution
    //=========================================================================

    /**************************************************************************
     * Function Name: Execute
     * Description: Executes the test step
     * Parameters:
     *   out_result - Step execution result
     * Returns: Result indicating execution status
     **************************************************************************/
    virtual CResult Execute(SStepResult& out_result) = 0;

    /**************************************************************************
     * Function Name: Abort
     * Description: Aborts step execution
     **************************************************************************/
    virtual void Abort() = 0;

    //=========================================================================
    // State
    //=========================================================================

    /**************************************************************************
     * Function Name: IsEnabled
     * Description: Returns whether step is enabled
     **************************************************************************/
    [[nodiscard]] virtual bool IsEnabled() const = 0;

    /**************************************************************************
     * Function Name: SetEnabled
     * Description: Enables or disables the step
     **************************************************************************/
    virtual void SetEnabled(bool in_bEnabled) = 0;
};

/**************************************************************************
 * Class: CTestStepBase
 * Description: Base implementation of ITestStep
 **************************************************************************/
class CTestStepBase : public ITestStep {
public:
    CTestStepBase(const TString& in_strId, const TString& in_strName, EStepType in_eType);
    ~CTestStepBase() override = default;

    [[nodiscard]] TString GetId() const override { return m_strId; }
    [[nodiscard]] TString GetName() const override { return m_strName; }
    [[nodiscard]] EStepType GetType() const override { return m_eType; }
    [[nodiscard]] TString GetDescription() const override { return m_strDescription; }

    [[nodiscard]] TVector<SStepParameter> GetParameters() const override;
    CResult SetParameter(const TString& in_strName, const TString& in_strValue) override;
    [[nodiscard]] std::optional<TString> GetParameter(const TString& in_strName) const override;

    void Abort() override { m_bAborted = true; }
    [[nodiscard]] bool IsEnabled() const override { return m_bEnabled; }
    void SetEnabled(bool in_bEnabled) override { m_bEnabled = in_bEnabled; }

    void SetDescription(const TString& in_strDesc) { m_strDescription = in_strDesc; }

protected:
    void AddParameter(const SStepParameter& in_param);
    [[nodiscard]] bool IsAborted() const { return m_bAborted; }
    void ResetAbort() { m_bAborted = false; }

    TString m_strId;
    TString m_strName;
    TString m_strDescription;
    EStepType m_eType;
    bool m_bEnabled{true};
    std::atomic<bool> m_bAborted{false};
    std::map<TString, SStepParameter> m_mapParameters;
};

} // namespace TestMATE
