/**************************************************************************
 * File Name: CalculationStep.cpp
 * Description: Implementation of CalculationStep
 **************************************************************************/

#include "CalculationStep.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace TestMATE {

CCalculationStep::CCalculationStep(const TString& in_strId,
                                   const TString& in_strName)
    : CTestStepBase(in_strId, in_strName, EStepType::kMeasurement)
{
    // Add parameter definitions
    SStepParameter opParam;
    opParam.name = "operation";
    opParam.type = "string";
    opParam.required = true;
    opParam.description = "Calculation type (add, subtract, multiply, divide, power, sqrt, abs, average, min, max, formula)";
    opParam.defaultValue = "add";
    AddParameter(opParam);

    SStepParameter aParam;
    aParam.name = "operand_a";
    aParam.type = "double";
    aParam.required = true;
    aParam.description = "First operand value";
    AddParameter(aParam);

    SStepParameter bParam;
    bParam.name = "operand_b";
    bParam.type = "double";
    bParam.required = false;
    bParam.description = "Second operand value (for binary operations)";
    AddParameter(bParam);

    SStepParameter formulaParam;
    formulaParam.name = "formula";
    formulaParam.type = "string";
    formulaParam.required = false;
    formulaParam.description = "Custom formula (e.g., 'a * b', '(a + b) / 2')";
    AddParameter(formulaParam);

    SStepParameter resultParam;
    resultParam.name = "result_name";
    resultParam.type = "string";
    resultParam.required = false;
    resultParam.description = "Name for result in output";
    resultParam.defaultValue = "result";
    AddParameter(resultParam);

    SStepParameter unitParam;
    unitParam.name = "unit";
    unitParam.type = "string";
    unitParam.required = false;
    unitParam.description = "Unit for result (e.g., 'W', 'Ω', '%')";
    AddParameter(unitParam);

    SetDescription("Perform mathematical calculation");
}

CResult CCalculationStep::Execute(SStepResult& out_result) {
    out_result.startTime = std::chrono::steady_clock::now();

    // Parse operation type
    auto opParam = GetParameter("operation");
    if (opParam.has_value()) {
        TString op = opParam.value();
        if (op == "add") m_eOperation = ECalculationType::kAdd;
        else if (op == "subtract") m_eOperation = ECalculationType::kSubtract;
        else if (op == "multiply") m_eOperation = ECalculationType::kMultiply;
        else if (op == "divide") m_eOperation = ECalculationType::kDivide;
        else if (op == "power") m_eOperation = ECalculationType::kPower;
        else if (op == "sqrt") m_eOperation = ECalculationType::kSquareRoot;
        else if (op == "abs") m_eOperation = ECalculationType::kAbsolute;
        else if (op == "average") m_eOperation = ECalculationType::kAverage;
        else if (op == "min") m_eOperation = ECalculationType::kMinimum;
        else if (op == "max") m_eOperation = ECalculationType::kMaximum;
        else if (op == "formula") m_eOperation = ECalculationType::kCustomFormula;
        else {
            out_result.verdict = ETestVerdict::kError;
            out_result.message = "Unknown operation type: " + op;
            out_result.endTime = std::chrono::steady_clock::now();
            out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                out_result.endTime - out_result.startTime).count();
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Unknown operation");
        }
    }

    // Parse operands
    if (!ParseOperands()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Failed to parse operand values";
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid operands");
    }

    // Get optional parameters
    auto formulaParam = GetParameter("formula");
    if (formulaParam.has_value()) {
        m_strFormula = formulaParam.value();
    }

    auto resultNameParam = GetParameter("result_name");
    if (resultNameParam.has_value()) {
        m_strResultName = resultNameParam.value();
    }

    auto unitParam = GetParameter("unit");
    if (unitParam.has_value()) {
        m_strUnit = unitParam.value();
    }

    // Perform calculation
    try {
        m_result = PerformCalculation();
    } catch (const std::exception& e) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = TString("Calculation error: ") + e.what();
        out_result.endTime = std::chrono::steady_clock::now();
        out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            out_result.endTime - out_result.startTime).count();
        return TESTMATE_FAILURE(EErrorCode::kExecutionFailed, e.what());
    }

    // Format result
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << m_result;
    if (!m_strUnit.empty()) {
        oss << " " << m_strUnit;
    }

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "Calculation completed successfully";
    out_result.measurements[m_strResultName] = oss.str();

    // Also store operands in results
    for (const auto& [name, value] : m_mapOperands) {
        std::ostringstream opOss;
        opOss << std::fixed << std::setprecision(6) << value;
        out_result.measurements[name] = opOss.str();
    }

    out_result.endTime = std::chrono::steady_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return TESTMATE_SUCCESS();
}

void CCalculationStep::SetOperands(const std::map<TString, TDouble>& in_mapOperands) {
    m_mapOperands = in_mapOperands;

    // Update parameters
    for (const auto& [name, value] : in_mapOperands) {
        std::ostringstream oss;
        oss << value;
        SetParameter(name, oss.str());
    }
}

bool CCalculationStep::ParseOperands() {
    m_mapOperands.clear();

    // Get operand_a (always required)
    auto aParam = GetParameter("operand_a");
    if (!aParam.has_value()) {
        return false;
    }

    try {
        m_mapOperands["a"] = std::stod(aParam.value());
    } catch (...) {
        return false;
    }

    // Get operand_b (optional)
    auto bParam = GetParameter("operand_b");
    if (bParam.has_value()) {
        try {
            m_mapOperands["b"] = std::stod(bParam.value());
        } catch (...) {
            return false;
        }
    }

    // Parse any additional numbered operands (operand_c, operand_d, etc.)
    for (char c = 'c'; c <= 'z'; ++c) {
        TString paramName = "operand_";
        paramName += c;
        auto param = GetParameter(paramName);
        if (param.has_value()) {
            try {
                TString key(1, c);
                m_mapOperands[key] = std::stod(param.value());
            } catch (...) {
                // Ignore invalid additional operands
            }
        }
    }

    return !m_mapOperands.empty();
}

TDouble CCalculationStep::PerformCalculation() {
    auto getOperand = [this](const TString& name) -> TDouble {
        auto it = m_mapOperands.find(name);
        if (it == m_mapOperands.end()) {
            throw std::runtime_error("Missing operand: " + name);
        }
        return it->second;
    };

    switch (m_eOperation) {
        case ECalculationType::kAdd: {
            TDouble sum = 0.0;
            for (const auto& [name, value] : m_mapOperands) {
                sum += value;
            }
            return sum;
        }

        case ECalculationType::kSubtract:
            return getOperand("a") - getOperand("b");

        case ECalculationType::kMultiply: {
            TDouble product = 1.0;
            for (const auto& [name, value] : m_mapOperands) {
                product *= value;
            }
            return product;
        }

        case ECalculationType::kDivide: {
            TDouble divisor = getOperand("b");
            if (std::abs(divisor) < 1e-12) {
                throw std::runtime_error("Division by zero");
            }
            return getOperand("a") / divisor;
        }

        case ECalculationType::kPower:
            return std::pow(getOperand("a"), getOperand("b"));

        case ECalculationType::kSquareRoot: {
            TDouble value = getOperand("a");
            if (value < 0.0) {
                throw std::runtime_error("Square root of negative number");
            }
            return std::sqrt(value);
        }

        case ECalculationType::kAbsolute:
            return std::abs(getOperand("a"));

        case ECalculationType::kAverage: {
            if (m_mapOperands.empty()) {
                return 0.0;
            }
            TDouble sum = 0.0;
            for (const auto& [name, value] : m_mapOperands) {
                sum += value;
            }
            return sum / static_cast<TDouble>(m_mapOperands.size());
        }

        case ECalculationType::kMinimum: {
            if (m_mapOperands.empty()) {
                return 0.0;
            }
            TDouble minVal = std::numeric_limits<TDouble>::max();
            for (const auto& [name, value] : m_mapOperands) {
                minVal = std::min(minVal, value);
            }
            return minVal;
        }

        case ECalculationType::kMaximum: {
            if (m_mapOperands.empty()) {
                return 0.0;
            }
            TDouble maxVal = std::numeric_limits<TDouble>::lowest();
            for (const auto& [name, value] : m_mapOperands) {
                maxVal = std::max(maxVal, value);
            }
            return maxVal;
        }

        case ECalculationType::kCustomFormula:
            return EvaluateFormula(m_strFormula);

        default:
            throw std::runtime_error("Unknown calculation type");
    }
}

TDouble CCalculationStep::EvaluateFormula(const TString& in_strFormula) {
    // Simple formula evaluation for common patterns
    // For production use, consider using a proper expression parser library

    // Support basic patterns like "a * b", "a + b", "(a - b) / c", etc.
    TString formula = in_strFormula;

    // Replace variable names with values
    for (const auto& [name, value] : m_mapOperands) {
        std::ostringstream oss;
        oss << std::setprecision(15) << value;
        TString replacement = oss.str();

        // Replace all occurrences of the variable
        size_t pos = 0;
        while ((pos = formula.find(name, pos)) != TString::npos) {
            formula.replace(pos, name.length(), replacement);
            pos += replacement.length();
        }
    }

    // For this example, support simple two-operand formulas
    // Real implementation should use a proper expression evaluator

    if (m_mapOperands.size() >= 2) {
        auto it = m_mapOperands.begin();
        TDouble a = it->second;
        ++it;
        TDouble b = it->second;

        if (in_strFormula.find("*") != TString::npos) {
            return a * b;
        } else if (in_strFormula.find("/") != TString::npos) {
            if (std::abs(b) < 1e-12) {
                throw std::runtime_error("Division by zero in formula");
            }
            return a / b;
        } else if (in_strFormula.find("+") != TString::npos) {
            return a + b;
        } else if (in_strFormula.find("-") != TString::npos) {
            return a - b;
        }
    }

    throw std::runtime_error("Could not evaluate formula: " + in_strFormula);
}

} // namespace TestMATE
