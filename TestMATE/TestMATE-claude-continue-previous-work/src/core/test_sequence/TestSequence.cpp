/**************************************************************************
 * File Name: TestSequence.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of test sequence container
 * Requirements: REQ-SEQ-001 to REQ-SEQ-030
 **************************************************************************/

#include "TestSequence.h"
#include <algorithm>

namespace TestMATE {

CTestSequence::CTestSequence() {
    m_info.createdDate = std::chrono::steady_clock::now();
    m_info.modifiedDate = m_info.createdDate;
}

CTestSequence::CTestSequence(const TString& in_strId, const TString& in_strName)
    : CTestSequence()
{
    m_info.id = in_strId;
    m_info.name = in_strName;
}

TUInt32 CTestSequence::AddStep(TUniquePtr<ITestStep> in_pStep) {
    if (!in_pStep) {
        return static_cast<TUInt32>(-1);
    }

    m_vecSteps.push_back(std::move(in_pStep));
    m_info.modifiedDate = std::chrono::steady_clock::now();

    return static_cast<TUInt32>(m_vecSteps.size() - 1);
}

CResult CTestSequence::InsertStep(TUInt32 in_uiIndex, TUniquePtr<ITestStep> in_pStep) {
    if (!in_pStep) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Step pointer is null");
    }

    if (in_uiIndex > m_vecSteps.size()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Index out of range");
    }

    m_vecSteps.insert(m_vecSteps.begin() + in_uiIndex, std::move(in_pStep));
    m_info.modifiedDate = std::chrono::steady_clock::now();

    return TESTMATE_SUCCESS();
}

CResult CTestSequence::RemoveStep(TUInt32 in_uiIndex) {
    if (in_uiIndex >= m_vecSteps.size()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Index out of range");
    }

    m_vecSteps.erase(m_vecSteps.begin() + in_uiIndex);
    m_info.modifiedDate = std::chrono::steady_clock::now();

    return TESTMATE_SUCCESS();
}

CResult CTestSequence::RemoveStepById(const TString& in_strId) {
    auto it = std::find_if(m_vecSteps.begin(), m_vecSteps.end(),
        [&in_strId](const TUniquePtr<ITestStep>& pStep) {
            return pStep && pStep->GetId() == in_strId;
        });

    if (it == m_vecSteps.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Step not found: " + in_strId);
    }

    m_vecSteps.erase(it);
    m_info.modifiedDate = std::chrono::steady_clock::now();

    return TESTMATE_SUCCESS();
}

ITestStep* CTestSequence::GetStep(TUInt32 in_uiIndex) {
    if (in_uiIndex >= m_vecSteps.size()) {
        return nullptr;
    }
    return m_vecSteps[in_uiIndex].get();
}

const ITestStep* CTestSequence::GetStep(TUInt32 in_uiIndex) const {
    if (in_uiIndex >= m_vecSteps.size()) {
        return nullptr;
    }
    return m_vecSteps[in_uiIndex].get();
}

ITestStep* CTestSequence::GetStepById(const TString& in_strId) {
    auto it = std::find_if(m_vecSteps.begin(), m_vecSteps.end(),
        [&in_strId](const TUniquePtr<ITestStep>& pStep) {
            return pStep && pStep->GetId() == in_strId;
        });

    if (it != m_vecSteps.end()) {
        return it->get();
    }
    return nullptr;
}

TUInt32 CTestSequence::GetStepCount() const {
    return static_cast<TUInt32>(m_vecSteps.size());
}

TUInt32 CTestSequence::GetEnabledStepCount() const {
    return static_cast<TUInt32>(std::count_if(m_vecSteps.begin(), m_vecSteps.end(),
        [](const TUniquePtr<ITestStep>& pStep) {
            return pStep && pStep->IsEnabled();
        }));
}

CResult CTestSequence::MoveStep(TUInt32 in_uiFromIndex, TUInt32 in_uiToIndex) {
    if (in_uiFromIndex >= m_vecSteps.size() || in_uiToIndex >= m_vecSteps.size()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Index out of range");
    }

    if (in_uiFromIndex == in_uiToIndex) {
        return TESTMATE_SUCCESS();
    }

    auto step = std::move(m_vecSteps[in_uiFromIndex]);
    m_vecSteps.erase(m_vecSteps.begin() + in_uiFromIndex);

    if (in_uiToIndex > in_uiFromIndex) {
        --in_uiToIndex;  // Adjust for removal
    }

    m_vecSteps.insert(m_vecSteps.begin() + in_uiToIndex, std::move(step));
    m_info.modifiedDate = std::chrono::steady_clock::now();

    return TESTMATE_SUCCESS();
}

void CTestSequence::Clear() {
    m_vecSteps.clear();
    m_info.modifiedDate = std::chrono::steady_clock::now();
}

void CTestSequence::SetVariable(const TString& in_strName, const TString& in_strValue) {
    m_mapVariables[in_strName] = in_strValue;
}

std::optional<TString> CTestSequence::GetVariable(const TString& in_strName) const {
    auto it = m_mapVariables.find(in_strName);
    if (it != m_mapVariables.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CTestSequence::ClearVariables() {
    m_mapVariables.clear();
}

const std::map<TString, TString>& CTestSequence::GetVariables() const {
    return m_mapVariables;
}

} // namespace TestMATE
