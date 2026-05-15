/**************************************************************************
 * File Name: ReportingTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for Reporting, Semiconductor, and Database
 **************************************************************************/

#include "core/reporting/IReportGenerator.h"
#include "core/semiconductor/DeviceHandler.h"
#include "database/IDataStore.h"
#include <gtest/gtest.h>

using namespace TestMATE;

//=============================================================================
// Report Generator Tests
//=============================================================================

class ReportGeneratorTest : public ::testing::Test {
protected:
    STestReport CreateSampleReport() {
        STestReport report;
        report.reportId = "RPT-001";
        report.sequenceName = "TestSequence";
        report.operatorName = "Operator1";
        report.lotId = "LOT001";
        report.serialNumber = "SN001";
        report.passCount = 8;
        report.failCount = 2;
        report.totalSteps = 10;
        report.overallVerdict = ETestVerdict::kFail;

        for (int i = 0; i < 10; ++i) {
            STestResult result;
            result.stepId = "S" + std::to_string(i);
            result.stepName = "Step " + std::to_string(i);
            result.verdict = (i < 8) ? ETestVerdict::kPass : ETestVerdict::kFail;
            result.durationMs = 100 + i * 10;
            report.results.push_back(result);
        }

        return report;
    }
};

TEST_F(ReportGeneratorTest, TextGenerator_GeneratesToString) {
    CTextReportGenerator gen;
    auto report = CreateSampleReport();

    TString content;
    auto result = gen.GenerateToString(report, content);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("TEST REPORT"), TString::npos);
    EXPECT_NE(content.find("TestSequence"), TString::npos);
}

TEST_F(ReportGeneratorTest, JsonGenerator_GeneratesValidJson) {
    CJsonReportGenerator gen;
    auto report = CreateSampleReport();

    TString content;
    auto result = gen.GenerateToString(report, content);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("\"reportId\""), TString::npos);
    EXPECT_NE(content.find("\"results\""), TString::npos);
}

TEST_F(ReportGeneratorTest, CsvGenerator_GeneratesCsv) {
    CCsvReportGenerator gen;
    auto report = CreateSampleReport();

    TString content;
    auto result = gen.GenerateToString(report, content);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("StepId"), TString::npos);  // Header
}

TEST_F(ReportGeneratorTest, ReportManager_HasDefaultGenerators) {
    auto formats = CReportManager::GetInstance().GetAvailableFormats();
    EXPECT_GE(formats.size(), 3u);  // Text, JSON, CSV
}

//=============================================================================
// Device Handler Tests
//=============================================================================

TEST(DeviceHandlerTest, Constructor_HasDefaultBins) {
    CDeviceHandler handler;
    auto bins = handler.GetAllBins();
    EXPECT_GE(bins.size(), 2u);  // Pass and Fail bins
}

TEST(DeviceHandlerTest, AddTestLimit_StoresLimit) {
    CDeviceHandler handler;

    STestLimit limit;
    limit.testName = "Voltage";
    limit.lowLimit = 3.0;
    limit.highLimit = 3.6;
    limit.unit = "V";

    handler.AddTestLimit(limit);

    auto retrieved = handler.GetTestLimit("Voltage");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->unit, "V");
}

TEST(DeviceHandlerTest, EvaluateLimit_PassWithinLimits) {
    CDeviceHandler handler;

    STestLimit limit;
    limit.testName = "Voltage";
    limit.lowLimit = 3.0;
    limit.highLimit = 3.6;
    handler.AddTestLimit(limit);

    auto result = handler.EvaluateLimit("Voltage", 3.3);

    EXPECT_EQ(result.verdict, ETestVerdict::kPass);
    EXPECT_TRUE(result.passLow);
    EXPECT_TRUE(result.passHigh);
}

TEST(DeviceHandlerTest, EvaluateLimit_FailBelowLow) {
    CDeviceHandler handler;

    STestLimit limit;
    limit.testName = "Voltage";
    limit.lowLimit = 3.0;
    limit.highLimit = 3.6;
    handler.AddTestLimit(limit);

    auto result = handler.EvaluateLimit("Voltage", 2.5);

    EXPECT_EQ(result.verdict, ETestVerdict::kFail);
    EXPECT_FALSE(result.passLow);
}

TEST(DeviceHandlerTest, Reset_ClearsResults) {
    CDeviceHandler handler;

    SParametricResult result;
    result.testName = "Test";
    result.verdict = ETestVerdict::kPass;
    handler.RecordResult(result);

    EXPECT_EQ(handler.GetTestCount(), 1u);

    handler.Reset();
    EXPECT_EQ(handler.GetTestCount(), 0u);
}

//=============================================================================
// Wafer Map Tests
//=============================================================================

TEST(WaferMapTest, Constructor_SetsSize) {
    CWaferMap map(10, 10);
    EXPECT_EQ(map.GetTotalDies(), 100u);
}

TEST(WaferMapTest, SetDieBin_StoresBin) {
    CWaferMap map(10, 10);
    map.SetDieBin(5, 5, 1);

    EXPECT_EQ(map.GetDieBin(5, 5), 1u);
}

TEST(WaferMapTest, MoveToNextDie_AdvancesPosition) {
    CWaferMap map(3, 3);

    auto pos1 = map.GetCurrentPosition();
    EXPECT_EQ(pos1.first, 0);
    EXPECT_EQ(pos1.second, 0);

    map.MoveToNextDie();
    auto pos2 = map.GetCurrentPosition();
    EXPECT_EQ(pos2.first, 1);
}

TEST(WaferMapTest, GetYield_CalculatesCorrectly) {
    CWaferMap map(2, 2);
    map.SetDieBin(0, 0, 1);  // Pass
    map.SetDieBin(0, 1, 1);  // Pass
    map.SetDieBin(1, 0, 0);  // Fail
    map.SetDieBin(1, 1, 1);  // Pass

    EXPECT_DOUBLE_EQ(map.GetYield(), 75.0);
}

//=============================================================================
// Data Store Tests
//=============================================================================

TEST(MemoryDataStoreTest, Connect_Succeeds) {
    CMemoryDataStore store;
    auto result = store.Connect("");
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(store.IsConnected());
}

TEST(MemoryDataStoreTest, InsertAndRetrieve) {
    CMemoryDataStore store;
    store.Connect("");

    STestDataRecord record;
    record.sequenceName = "TestSeq";
    record.lotId = "LOT001";
    record.verdict = ETestVerdict::kPass;

    TUInt64 id;
    auto insertResult = store.InsertTestData(record, id);
    EXPECT_TRUE(insertResult.IsSuccess());
    EXPECT_GT(id, 0u);

    STestDataRecord retrieved;
    auto getResult = store.GetTestData(id, retrieved);
    EXPECT_TRUE(getResult.IsSuccess());
    EXPECT_EQ(retrieved.sequenceName, "TestSeq");
}

TEST(MemoryDataStoreTest, Delete_RemovesRecord) {
    CMemoryDataStore store;
    store.Connect("");

    STestDataRecord record;
    record.sequenceName = "Test";

    TUInt64 id;
    store.InsertTestData(record, id);
    EXPECT_EQ(store.GetRecordCount(), 1u);

    store.DeleteTestData(id);
    EXPECT_EQ(store.GetRecordCount(), 0u);
}

//=============================================================================
// Analytics Tests
//=============================================================================

TEST(AnalyticsTest, CalculateMean) {
    TVector<TDouble> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    EXPECT_DOUBLE_EQ(CAnalytics::CalculateMean(values), 3.0);
}

TEST(AnalyticsTest, CalculateMedian_Odd) {
    TVector<TDouble> values = {1.0, 3.0, 2.0};
    EXPECT_DOUBLE_EQ(CAnalytics::CalculateMedian(values), 2.0);
}

TEST(AnalyticsTest, CalculateMedian_Even) {
    TVector<TDouble> values = {1.0, 2.0, 3.0, 4.0};
    EXPECT_DOUBLE_EQ(CAnalytics::CalculateMedian(values), 2.5);
}

TEST(AnalyticsTest, CalculateYield) {
    EXPECT_DOUBLE_EQ(CAnalytics::CalculateYield(90, 100), 90.0);
    EXPECT_DOUBLE_EQ(CAnalytics::CalculateYield(0, 100), 0.0);
}

TEST(AnalyticsTest, CalculateCpk) {
    // Cpk = min((USL-mean)/(3*sigma), (mean-LSL)/(3*sigma))
    TDouble cpk = CAnalytics::CalculateCpk(5.0, 1.0, 2.0, 8.0);
    EXPECT_DOUBLE_EQ(cpk, 1.0);  // (8-5)/(3*1) = 1.0, (5-2)/(3*1) = 1.0
}

