/**************************************************************************
 * File Name: HtmlReportGenerator.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: HTML report generator with styling.
 * Requirements: REQ-RPT-031 to REQ-RPT-040
 **************************************************************************/

#pragma once

#include "IReportGenerator.h"

namespace TestMATE {

/**************************************************************************
 * Class: CHtmlReportGenerator
 * Description: Generates styled HTML test reports
 **************************************************************************/
class CHtmlReportGenerator : public IReportGenerator {
public:
    [[nodiscard]] EReportFormat GetFormat() const override { return EReportFormat::kHtml; }
    [[nodiscard]] TString GetFormatName() const override { return "HTML"; }
    [[nodiscard]] TString GetFileExtension() const override { return ".html"; }

    CResult Generate(const STestReport& in_report, const TString& in_strOutputPath) override;
    CResult GenerateToString(const STestReport& in_report, TString& out_strContent) override;

    void SetTitle(const TString& in_strTitle) { m_strTitle = in_strTitle; }
    void SetCompanyName(const TString& in_strName) { m_strCompanyName = in_strName; }
    void SetLogoPath(const TString& in_strPath) { m_strLogoPath = in_strPath; }
    void SetIncludeCharts(bool in_bInclude) { m_bIncludeCharts = in_bInclude; }

private:
    TString GenerateHeader();
    TString GenerateStyles();
    TString GenerateSummarySection(const STestReport& in_report);
    TString GenerateResultsTable(const STestReport& in_report);
    TString GenerateFooter();
    TString VerdictToClass(ETestVerdict in_verdict);
    TString VerdictToString(ETestVerdict in_verdict);
    TString FormatTimestamp(const TTimePoint& in_tp);

    TString m_strTitle{"Test Report"};
    TString m_strCompanyName{"TestMATE"};
    TString m_strLogoPath;
    bool m_bIncludeCharts{false};
};

} // namespace TestMATE
