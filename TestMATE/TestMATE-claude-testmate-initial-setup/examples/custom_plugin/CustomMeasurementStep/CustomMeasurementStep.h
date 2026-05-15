/**************************************************************************
 * File Name: CustomMeasurementStep.h
 * Description: Example test step plugin for custom measurements
 **************************************************************************/

#pragma once

#include "core/plugins/IPlugin.h"
#include <mutex>

namespace TestMATEPlugins {

class CCustomMeasurementStep : public TestMATE::ITestStepPlugin {
public:
    CCustomMeasurementStep();
    ~CCustomMeasurementStep() override = default;

    // IPlugin Interface
    [[nodiscard]] TestMATE::SPluginInfo GetInfo() const override;
    TestMATE::CResult Initialize() override;
    TestMATE::CResult Shutdown() override;
    [[nodiscard]] TestMATE::EPluginState GetState() const override;
    [[nodiscard]] TestMATE::TString GetLastError() const override;

    // ITestStepPlugin Interface
    TestMATE::ETestVerdict Execute(
        const std::map<TestMATE::TString, TestMATE::TString>& in_mapParams,
        std::map<TestMATE::TString, TestMATE::TString>& out_mapResults) override;

    [[nodiscard]] TestMATE::TVector<TestMATE::TString> GetParameterDefinitions() const override;

private:
    TestMATE::SPluginInfo m_info;
    TestMATE::EPluginState m_eState;
    TestMATE::TString m_strLastError;
    mutable std::mutex m_mutex;

    void SetError(const TestMATE::TString& in_strError);
    TestMATE::TDouble PerformCalculation(const TestMATE::TString& in_strFormula,
                                        const std::map<TestMATE::TString, TestMATE::TString>& in_mapParams);
};

} // namespace TestMATEPlugins

TESTMATE_DECLARE_PLUGIN(TestMATEPlugins::CCustomMeasurementStep)
