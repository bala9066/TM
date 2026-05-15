/**************************************************************************
 * File Name: TestStepBase.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of test step base class
 * Requirements: REQ-STEP-001 to REQ-STEP-020
 **************************************************************************/

#include "ITestStep.h"

namespace TestMATE {

CTestStepBase::CTestStepBase(const TString& in_strId,
                             const TString& in_strName,
                             EStepType in_eType)
    : m_strId(in_strId)
    , m_strName(in_strName)
    , m_eType(in_eType)
{
}

TVector<SStepParameter> CTestStepBase::GetParameters() const {
    TVector<SStepParameter> params;
    for (const auto& [name, param] : m_mapParameters) {
        params.push_back(param);
    }
    return params;
}

CResult CTestStepBase::SetParameter(const TString& in_strName,
                                     const TString& in_strValue) {
    auto it = m_mapParameters.find(in_strName);
    if (it == m_mapParameters.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Parameter not found: " + in_strName);
    }

    it->second.value = in_strValue;
    return TESTMATE_SUCCESS();
}

std::optional<TString> CTestStepBase::GetParameter(const TString& in_strName) const {
    auto it = m_mapParameters.find(in_strName);
    if (it != m_mapParameters.end()) {
        return it->second.value.empty() ? it->second.defaultValue : it->second.value;
    }
    return std::nullopt;
}

void CTestStepBase::AddParameter(const SStepParameter& in_param) {
    m_mapParameters[in_param.name] = in_param;
}

} // namespace TestMATE
