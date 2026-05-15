/**************************************************************************
 * File Name: CalculationStep.h
 * Description: Test step for mathematical calculations and derived parameters
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"
#include <map>
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: ECalculationType
 * Description: Types of calculations supported
 **************************************************************************/
enum class ECalculationType {
    kAdd,           // a + b
    kSubtract,      // a - b
    kMultiply,      // a * b
    kDivide,        // a / b
    kPower,         // a ^ b
    kSquareRoot,    // sqrt(a)
    kAbsolute,      // abs(a)
    kAverage,       // (a + b + ...) / n
    kMinimum,       // min(a, b, ...)
    kMaximum,       // max(a, b, ...)
    kCustomFormula  // User-defined formula
};

/**************************************************************************
 * Class: CCalculationStep
 * Description: Performs mathematical calculations on input values
 *
 * Parameters:
 *   - operation: Type of calculation (add, subtract, multiply, divide, etc.)
 *   - operand_a: First operand (required)
 *   - operand_b: Second operand (optional, depends on operation)
 *   - formula: Custom formula string (for kCustomFormula type)
 *   - result_name: Name for result in output (default: "result")
 *   - unit: Unit for result value (optional)
 *
 * Supported Operations:
 *   - Basic: add, subtract, multiply, divide
 *   - Advanced: power, sqrt, abs
 *   - Statistical: average, min, max
 *   - Custom: Evaluate formula string
 *
 * Examples:
 *   Power = Voltage * Current
 *   Resistance = Voltage / Current
 *   Efficiency = (P_out / P_in) * 100
 **************************************************************************/
class CCalculationStep : public CTestStepBase {
public:
    explicit CCalculationStep(const TString& in_strId = "CALC-001",
                             const TString& in_strName = "Calculation");

    ~CCalculationStep() override = default;

    /**************************************************************************
     * Function Name: Execute
     * Description: Performs the calculation
     **************************************************************************/
    CResult Execute(SStepResult& out_result) override;

    /**************************************************************************
     * Function Name: SetOperation
     * Description: Sets the calculation type
     **************************************************************************/
    void SetOperation(ECalculationType in_eOp) { m_eOperation = in_eOp; }

    /**************************************************************************
     * Function Name: SetOperands
     * Description: Sets calculation input values
     **************************************************************************/
    void SetOperands(const std::map<TString, TDouble>& in_mapOperands);

    /**************************************************************************
     * Function Name: SetFormula
     * Description: Sets custom formula (e.g., "(a + b) / 2")
     **************************************************************************/
    void SetFormula(const TString& in_strFormula) { m_strFormula = in_strFormula; }

    /**************************************************************************
     * Function Name: GetResult
     * Description: Gets the last calculated result
     **************************************************************************/
    [[nodiscard]] TDouble GetResult() const { return m_result; }

private:
    ECalculationType m_eOperation{ECalculationType::kAdd};
    std::map<TString, TDouble> m_mapOperands;
    TString m_strFormula;
    TString m_strResultName{"result"};
    TString m_strUnit;
    TDouble m_result{0.0};

    TDouble PerformCalculation();
    TDouble EvaluateFormula(const TString& in_strFormula);
    bool ParseOperands();
};

} // namespace TestMATE
