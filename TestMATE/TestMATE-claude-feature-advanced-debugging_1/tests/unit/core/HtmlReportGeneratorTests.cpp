/**************************************************************************
 * File Name: HtmlReportGeneratorTests.cpp
 * Description: Unit tests for HtmlReportGenerator component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/reporting/HtmlReportGenerator.h"
#include <fstream>
#include <filesystem>

namespace TestMATE {
namespace Tests {

class CHtmlReportGeneratorTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_testDir = std::filesystem::temp_directory_path() / "testmate_html_tests";
        std::filesystem::create_directories(m_testDir);
        m_generator = std::make_unique<CHtmlReportGenerator>();
    }

    void TearDown() override {
        std::filesystem::remove_all(m_testDir);
    }

    STestReport CreateSampleReport() {
        STestReport report;
        report.reportId = "RPT-001";
        report.sequenceName = "TestSequence";
        report.operatorName = "TestOperator";
        report.lotId = "LOT-123";
        report.serialNumber = "SN-456";
        report.totalSteps = 5;
        report.passCount = 4;
        report.failCount = 1;
        report.overallVerdict = ETestVerdict::kFail;

        STestResult step1{.stepId = "S1", .stepName = "Step1", .verdict = ETestVerdict::kPass, .durationMs = 100};
        STestResult step2{.stepId = "S2", .stepName = "Step2", .verdict = ETestVerdict::kPass, .durationMs = 150};
        STestResult step3{.stepId = "S3", .stepName = "Step3", .verdict = ETestVerdict::kFail, .durationMs = 200};
        report.results = {step1, step2, step3};

        return report;
    }

    std::filesystem::path m_testDir;
    std::unique_ptr<CHtmlReportGenerator> m_generator;
};

TEST_F(CHtmlReportGeneratorTests, Generate_ValidReport_CreatesFile) {
    auto report = CreateSampleReport();
    auto outputPath = m_testDir / "report.html";

    auto result = m_generator->Generate(report, outputPath.string());

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(std::filesystem::exists(outputPath));
}

TEST_F(CHtmlReportGeneratorTests, Generate_ValidReport_ContainsHtmlStructure) {
    auto report = CreateSampleReport();
    auto outputPath = m_testDir / "report.html";

    m_generator->Generate(report, outputPath.string());

    std::ifstream file(outputPath);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(content.find("<html"), std::string::npos);
    EXPECT_NE(content.find("</html>"), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, Generate_ValidReport_ContainsReportData) {
    auto report = CreateSampleReport();
    auto outputPath = m_testDir / "report.html";

    m_generator->Generate(report, outputPath.string());

    std::ifstream file(outputPath);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("RPT-001"), std::string::npos);
    EXPECT_NE(content.find("TestSequence"), std::string::npos);
    EXPECT_NE(content.find("TestOperator"), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, SetTitle_CustomTitle_AppearsInOutput) {
    m_generator->SetTitle("Custom Report Title");

    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("Custom Report Title"), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, SetCompanyName_CustomName_AppearsInOutput) {
    m_generator->SetCompanyName("TestMATE Inc.");

    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("TestMATE Inc."), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, GenerateToString_ValidReport_ReturnsContent) {
    auto report = CreateSampleReport();
    TString content;

    auto result = m_generator->GenerateToString(report, content);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(content.empty());
}

TEST_F(CHtmlReportGeneratorTests, Generate_ContainsPassFailCounts) {
    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("4"), std::string::npos);  // passCount
    EXPECT_NE(content.find("1"), std::string::npos);  // failCount
}

TEST_F(CHtmlReportGeneratorTests, Generate_ContainsStylesheet) {
    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("<style>"), std::string::npos);
    EXPECT_NE(content.find("</style>"), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, Generate_ContainsResultsTable) {
    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("<table>"), std::string::npos);
    EXPECT_NE(content.find("Step1"), std::string::npos);
    EXPECT_NE(content.find("Step2"), std::string::npos);
}

TEST_F(CHtmlReportGeneratorTests, Generate_InvalidPath_Failure) {
    auto report = CreateSampleReport();
    auto result = m_generator->Generate(report, "/nonexistent/dir/report.html");

    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CHtmlReportGeneratorTests, SetLogoPath_CustomLogo_IncludedInOutput) {
    m_generator->SetLogoPath("/path/to/logo.png");

    auto report = CreateSampleReport();
    TString content;
    m_generator->GenerateToString(report, content);

    EXPECT_NE(content.find("/path/to/logo.png"), std::string::npos);
}

} // namespace Tests
} // namespace TestMATE
