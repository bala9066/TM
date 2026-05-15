/**************************************************************************
 * File Name: LimitCheckStep.h
 * Description: Test step for validating measurements against limits
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"
#include <limits>

namespace TestMATE {

/**************************************************************************
 * Enum: ELimitType
 * Description: Types of limit checks
 **************************************************************************/
enum class ELimitType {
    kRange,         // Min <= value <= Max
    kMinOnly,       // value >= Min
    kMaxOnly,       // value <= Max
    kEquals,        // value == target (with tolerance)
    kNotEquals,     // value != target (with tolerance)
    kCustom         // Custom comparison logic
};

/**************************************************************************
 * Class: CLimitCheckStep
 * Description: Validates a measurement value against specified limits
 *
 * Parameters:
 *   - value: Value to check (required)
 *   - limit_min: Minimum acceptable value (optional)
 *   - limit_max: Maximum acceptable value (optional)
 *   - tolerance: Tolerance for equality checks (optional, default: 0.001)
 *   - limit_type: Type of limit check (optional, default: range)
 *   - unit: Measurement unit for display (optional)
 **************************************************************************/
class CLimitCheckStep : public CTestStepBase {
public:
    explicit CLimitCheckStep(const TString& in_strId = "LIMIT-001",
                            const TString& in_strName = "Limit Check");

    ~CLimitCheckStep() override = default;

    /**************************************************************************
     * Function Name: Execute
     * Description: Performs the limit check
     **************************************************************************/
    CResult Execute(SStepResult& out_result) override;

    /**************************************************************************
     * Function Name: SetLimits
     * Description: Sets the limit values
     **************************************************************************/
    void SetLimits(TDouble in_min, TDouble in_max);

    /**************************************************************************
     * Function Name: SetMinLimit
     * Description: Sets only the minimum limit
     **************************************************************************/
    void SetMinLimit(TDouble in_min);

    /**************************************************************************
     * Function Name: SetMaxLimit
     * Description: Sets only the maximum limit
     **************************************************************************/
    void SetMaxLimit(TDouble in_max);

    /**************************************************************************
     * Function Name: SetValue
     * Description: Sets the value to be checked
     **************************************************************************/
    void SetValue(TDouble in_value);

    /**************************************************************************
     * Function Name: SetTolerance
     * Description: Sets tolerance for equality checks
     **************************************************************************/
    void SetTolerance(TDouble in_tolerance) { m_tolerance = in_tolerance; }

    /**************************************************************************
     * Function Name: SetUnit
     * Description: Sets the measurement unit for display
     **************************************************************************/
    void SetUnit(const TString& in_strUnit) { m_strUnit = in_strUnit; }

    /**************************************************************************
     * Function Name: SetLimitType
     * Description: Sets the type of limit check to perform
     **************************************************************************/
    void SetLimitType(ELimitType in_eType) { m_eLimitType = in_eType; }

private:
    TDouble m_value{0.0};
    TDouble m_limitMin{std::numeric_limits<TDouble>::lowest()};
    TDouble m_limitMax{std::numeric_limits<TDouble>::max()};
    TDouble m_tolerance{0.001};
    TString m_strUnit;
    ELimitType m_eLimitType{ELimitType::kRange};

    bool CheckLimit(TDouble in_value, TString& out_strMessage);
    TString FormatValue(TDouble in_value);
};

} // namespace TestMATE
