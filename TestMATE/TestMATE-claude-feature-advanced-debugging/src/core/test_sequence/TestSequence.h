/**************************************************************************
 * File Name: TestSequence.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Test sequence container - holds ordered collection of steps.
 * Requirements: REQ-SEQ-001 to REQ-SEQ-030
 **************************************************************************/

#pragma once

#include "ITestStep.h"
#include <memory>
#include <vector>

namespace TestMATE {

/**************************************************************************
 * Struct: SSequenceInfo
 * Description: Metadata for a test sequence
 **************************************************************************/
struct SSequenceInfo {
    TString id;
    TString name;
    TString version;
    TString author;
    TString description;
    TWallClock createdDate;
    TWallClock modifiedDate;
    std::map<TString, TString> properties;
};

/**************************************************************************
 * Class: CTestSequence
 * Description: Container for test steps with execution ordering
 * Requirements: REQ-SEQ-001 to REQ-SEQ-030
 **************************************************************************/
class CTestSequence {
public:
    CTestSequence();
    explicit CTestSequence(const TString& in_strId, const TString& in_strName);
    ~CTestSequence() = default;

    //=========================================================================
    // Identification
    //=========================================================================

    [[nodiscard]] const SSequenceInfo& GetInfo() const { return m_info; }
    void SetInfo(const SSequenceInfo& in_info) { m_info = in_info; }

    [[nodiscard]] TString GetId() const { return m_info.id; }
    [[nodiscard]] TString GetName() const { return m_info.name; }

    //=========================================================================
    // Step Management
    //=========================================================================

    /**************************************************************************
     * Function Name: AddStep
     * Description: Adds a step to the end of the sequence
     * Parameters:
     *   in_pStep - Step to add (takes ownership)
     * Returns: Index of added step
     **************************************************************************/
    TUInt32 AddStep(TUniquePtr<ITestStep> in_pStep);

    /**************************************************************************
     * Function Name: InsertStep
     * Description: Inserts a step at specified index
     * Parameters:
     *   in_uiIndex - Index to insert at
     *   in_pStep - Step to insert
     * Returns: Result indicating success or failure
     **************************************************************************/
    CResult InsertStep(TUInt32 in_uiIndex, TUniquePtr<ITestStep> in_pStep);

    /**************************************************************************
     * Function Name: RemoveStep
     * Description: Removes step at specified index
     **************************************************************************/
    CResult RemoveStep(TUInt32 in_uiIndex);

    /**************************************************************************
     * Function Name: RemoveStepById
     * Description: Removes step by ID
     **************************************************************************/
    CResult RemoveStepById(const TString& in_strId);

    /**************************************************************************
     * Function Name: GetStep
     * Description: Gets step at specified index
     **************************************************************************/
    [[nodiscard]] ITestStep* GetStep(TUInt32 in_uiIndex);
    [[nodiscard]] const ITestStep* GetStep(TUInt32 in_uiIndex) const;

    /**************************************************************************
     * Function Name: GetStepById
     * Description: Gets step by ID
     **************************************************************************/
    [[nodiscard]] ITestStep* GetStepById(const TString& in_strId);

    /**************************************************************************
     * Function Name: GetStepCount
     * Description: Returns number of steps in sequence
     **************************************************************************/
    [[nodiscard]] TUInt32 GetStepCount() const;

    /**************************************************************************
     * Function Name: GetEnabledStepCount
     * Description: Returns number of enabled steps
     **************************************************************************/
    [[nodiscard]] TUInt32 GetEnabledStepCount() const;

    /**************************************************************************
     * Function Name: MoveStep
     * Description: Moves step from one index to another
     **************************************************************************/
    CResult MoveStep(TUInt32 in_uiFromIndex, TUInt32 in_uiToIndex);

    /**************************************************************************
     * Function Name: Clear
     * Description: Removes all steps
     **************************************************************************/
    void Clear();

    //=========================================================================
    // Variables
    //=========================================================================

    void SetVariable(const TString& in_strName, const TString& in_strValue);
    [[nodiscard]] std::optional<TString> GetVariable(const TString& in_strName) const;
    void ClearVariables();
    [[nodiscard]] const std::map<TString, TString>& GetVariables() const;

    //=========================================================================
    // Iteration
    //=========================================================================

    using TStepIterator = std::vector<TUniquePtr<ITestStep>>::iterator;
    using TConstStepIterator = std::vector<TUniquePtr<ITestStep>>::const_iterator;

    TStepIterator begin() { return m_vecSteps.begin(); }
    TStepIterator end() { return m_vecSteps.end(); }
    [[nodiscard]] TConstStepIterator begin() const { return m_vecSteps.begin(); }
    [[nodiscard]] TConstStepIterator end() const { return m_vecSteps.end(); }

private:
    SSequenceInfo m_info;
    std::vector<TUniquePtr<ITestStep>> m_vecSteps;
    std::map<TString, TString> m_mapVariables;
};

} // namespace TestMATE
