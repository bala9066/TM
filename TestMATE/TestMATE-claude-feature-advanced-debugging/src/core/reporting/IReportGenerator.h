/**************************************************************************
 * File Name: IReportGenerator.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Report generation interface for test results.
 * Requirements: REQ-RPT-001 to REQ-RPT-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <map>

namespace TestMATE {

/**************************************************************************
 * Enum: EReportFormat
 * Description: Supported report output formats
 **************************************************************************/
enum class EReportFormat {
    kText,
    kHtml,
    kXml,
    kJson,
    kCsv,
    kPdf,
    kCustom
};

/**************************************************************************
 * Struct: STestResult
 * Description: Individual test result data
 **************************************************************************/
struct STestResult {
    TString stepId;
    TString stepName;
    ETestVerdict verdict{ETestVerdict::kNone};
    TString message;
    TTimePoint startTime;
    TTimePoint endTime;
    TInt64 durationMs{0};
    std::map<TString, TString> measurements;
    std::map<TString, TString> parameters;
};

/**************************************************************************
 * Struct: STestReport
 * Description: Complete test execution report
 **************************************************************************/
struct STestReport {
    TString reportId;
    TString sequenceName;
    TString operatorName;
    TString lotId;
    TString serialNumber;

    TTimePoint startTime;
    TTimePoint endTime;
    TInt64 totalDurationMs{0};

    ETestVerdict overallVerdict{ETestVerdict::kNone};
    TUInt32 totalSteps{0};
    TUInt32 passCount{0};
    TUInt32 failCount{0};
    TUInt32 skipCount{0};

    TVector<STestResult> results;
    std::map<TString, TString> metadata;
};

/**************************************************************************
 * Interface: IReportGenerator
 * Description: Base interface for report generators
 **************************************************************************/
class IReportGenerator {
public:
    virtual ~IReportGenerator() = default;

    [[nodiscard]] virtual EReportFormat GetFormat() const = 0;
    [[nodiscard]] virtual TString GetFormatName() const = 0;
    [[nodiscard]] virtual TString GetFileExtension() const = 0;

    virtual CResult Generate(const STestReport& in_report,
                              const TString& in_strOutputPath) = 0;
    virtual CResult GenerateToString(const STestReport& in_report,
                                      TString& out_strContent) = 0;
};

/**************************************************************************
 * Class: CTextReportGenerator
 * Description: Plain text report generator
 **************************************************************************/
class CTextReportGenerator : public IReportGenerator {
public:
    [[nodiscard]] EReportFormat GetFormat() const override { return EReportFormat::kText; }
    [[nodiscard]] TString GetFormatName() const override { return "Text"; }
    [[nodiscard]] TString GetFileExtension() const override { return ".txt"; }

    CResult Generate(const STestReport& in_report, const TString& in_strOutputPath) override;
    CResult GenerateToString(const STestReport& in_report, TString& out_strContent) override;
};

/**************************************************************************
 * Class: CJsonReportGenerator
 * Description: JSON format report generator
 **************************************************************************/
class CJsonReportGenerator : public IReportGenerator {
public:
    [[nodiscard]] EReportFormat GetFormat() const override { return EReportFormat::kJson; }
    [[nodiscard]] TString GetFormatName() const override { return "JSON"; }
    [[nodiscard]] TString GetFileExtension() const override { return ".json"; }

    CResult Generate(const STestReport& in_report, const TString& in_strOutputPath) override;
    CResult GenerateToString(const STestReport& in_report, TString& out_strContent) override;

    void SetPrettyPrint(bool in_bPretty) { m_bPrettyPrint = in_bPretty; }

private:
    bool m_bPrettyPrint{true};
};

/**************************************************************************
 * Class: CCsvReportGenerator
 * Description: CSV format report generator
 **************************************************************************/
class CCsvReportGenerator : public IReportGenerator {
public:
    [[nodiscard]] EReportFormat GetFormat() const override { return EReportFormat::kCsv; }
    [[nodiscard]] TString GetFormatName() const override { return "CSV"; }
    [[nodiscard]] TString GetFileExtension() const override { return ".csv"; }

    CResult Generate(const STestReport& in_report, const TString& in_strOutputPath) override;
    CResult GenerateToString(const STestReport& in_report, TString& out_strContent) override;

    void SetDelimiter(char in_cDelim) { m_cDelimiter = in_cDelim; }

private:
    char m_cDelimiter{','};
};

/**************************************************************************
 * Class: CReportManager
 * Description: Singleton for managing report generation
 **************************************************************************/
class CReportManager {
public:
    static CReportManager& GetInstance();

    CReportManager(const CReportManager&) = delete;
    CReportManager& operator=(const CReportManager&) = delete;

    void RegisterGenerator(TSharedPtr<IReportGenerator> in_pGenerator);
    [[nodiscard]] IReportGenerator* GetGenerator(EReportFormat in_eFormat);
    [[nodiscard]] TVector<EReportFormat> GetAvailableFormats() const;

    CResult GenerateReport(const STestReport& in_report,
                           EReportFormat in_eFormat,
                           const TString& in_strOutputPath);

    // Report building helpers
    STestReport CreateReport(const TString& in_strSequenceName);
    void AddResult(STestReport& io_report, const STestResult& in_result);
    void FinalizeReport(STestReport& io_report);

private:
    CReportManager();
    ~CReportManager() = default;

    std::map<EReportFormat, TSharedPtr<IReportGenerator>> m_mapGenerators;
};

} // namespace TestMATE
