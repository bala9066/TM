/**************************************************************************
 * File Name: EndToEndTests.cpp
 * Description: End-to-end integration tests for TestMATE
 * Author: TestMATE Development Team
 *
 * These tests verify complete workflows from sequence loading to report
 * generation, ensuring all components work together correctly.
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/test_sequence/SequenceFileIO.h"
#include "core/test_sequence/TestSequence.h"
#include "core/test_executor/TestExecutor.h"
#include "core/execution_context/ExecutionContext.h"
#include "database/SqliteDataStore.h"
#include "reporting/HtmlReportGenerator.h"
#include "examples/test_steps/WaitStep.h"
#include "examples/test_steps/LimitCheckStep.h"
#include "examples/test_steps/CalculationStep.h"
#include "examples/test_steps/FileOperationStep.h"
#include <fstream>
#include <filesystem>

using namespace TestMATE;

/**************************************************************************
 * Test Fixture: EndToEndTests
 * Description: Setup/teardown for integration tests
 **************************************************************************/
class EndToEndTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test outputs
        m_strTempDir = "/tmp/testmate_integration_tests";
        std::filesystem::create_directories(m_strTempDir);
    }

    void TearDown() override {
        // Cleanup temp directory
        if (std::filesystem::exists(m_strTempDir)) {
            std::filesystem::remove_all(m_strTempDir);
        }
    }

    TString m_strTempDir;
};

/**************************************************************************
 * Test: CompleteSequenceWorkflow
 * Description: Test complete workflow from sequence creation to execution
 **************************************************************************/
TEST_F(EndToEndTests, CompleteSequenceWorkflow) {
    // 1. Create test sequence programmatically
    CTestSequence sequence;
    SSequenceInfo info;
    info.id = "E2E-TEST-001";
    info.name = "End-to-End Integration Test";
    info.version = "1.0.0";
    sequence.SetInfo(info);

    // 2. Add test steps
    auto waitStep = std::make_unique<CWaitStep>("WAIT-001", "Initial Delay", 100);
    sequence.AddStep(std::move(waitStep));

    auto calcStep = std::make_unique<CCalculationStep>("CALC-001", "Calculate Power");
    calcStep->SetOperation(ECalculationType::kMultiply);
    std::map<TString, TDouble> operands;
    operands["a"] = 12.0;  // Voltage
    operands["b"] = 2.5;   // Current
    calcStep->SetOperands(operands);
    sequence.AddStep(std::move(calcStep));

    auto limitStep = std::make_unique<CLimitCheckStep>("LIMIT-001", "Validate Power");
    limitStep->SetValue(30.0);  // Expected power
    limitStep->SetLimits(25.0, 35.0);
    sequence.AddStep(std::move(limitStep));

    // 3. Verify sequence structure
    EXPECT_EQ(sequence.GetStepCount(), 3u);
    EXPECT_EQ(sequence.GetInfo().id, "E2E-TEST-001");

    // 4. Execute steps individually
    SStepResult result;
    for (TUInt32 i = 0; i < sequence.GetStepCount(); ++i) {
        ITestStep* pStep = sequence.GetStep(i);
        ASSERT_NE(pStep, nullptr);

        auto execResult = pStep->Execute(result);
        EXPECT_TRUE(execResult.IsSuccess()) << "Step " << i << " execution failed";
        EXPECT_NE(result.verdict, ETestVerdict::kError) << "Step " << i << " returned error verdict";
    }

    // 5. Verify final results
    EXPECT_EQ(result.verdict, ETestVerdict::kPass);
}

/**************************************************************************
 * Test: SequenceFileIOWorkflow
 * Description: Test JSON sequence save/load/execute workflow
 **************************************************************************/
TEST_F(EndToEndTests, SequenceFileIOWorkflow) {
    TString sequencePath = m_strTempDir + "/test_sequence.json";

    // 1. Create and save sequence
    CTestSequence originalSequence;
    SSequenceInfo info;
    info.id = "FILE-IO-TEST";
    info.name = "File I/O Test Sequence";
    info.version = "2.0.0";
    originalSequence.SetInfo(info);

    auto& fileIO = CSequenceFileIO::GetInstance();
    auto saveResult = fileIO.SaveSequence(sequencePath, originalSequence, ESequenceFileFormat::kJson);
    ASSERT_TRUE(saveResult.IsSuccess()) << "Failed to save sequence: " << saveResult.GetMessage();

    // 2. Verify file exists
    EXPECT_TRUE(std::filesystem::exists(sequencePath));

    // 3. Load sequence
    CTestSequence loadedSequence;
    auto loadResult = fileIO.LoadSequence(sequencePath, loadedSequence);
    ASSERT_TRUE(loadResult.IsSuccess()) << "Failed to load sequence: " << loadResult.GetMessage();

    // 4. Verify loaded data matches
    const auto& loadedInfo = loadedSequence.GetInfo();
    EXPECT_EQ(loadedInfo.id, info.id);
    EXPECT_EQ(loadedInfo.name, info.name);
    EXPECT_EQ(loadedInfo.version, info.version);
}

/**************************************************************************
 * Test: DatabaseIntegration
 * Description: Test database save/retrieve workflow
 **************************************************************************/
TEST_F(EndToEndTests, DatabaseIntegration) {
    TString dbPath = m_strTempDir + "/test.db";

    // 1. Initialize database
    CSqliteDataStore dataStore;
    auto initResult = dataStore.Initialize(dbPath);
    ASSERT_TRUE(initResult.IsSuccess()) << "Failed to initialize database";

    // 2. Create test data
    STestDataRecord record;
    record.testId = "DB-TEST-001";
    record.sequenceName = "Database Integration Test";
    record.verdict = ETestVerdict::kPass;
    record.deviceId = "DUT-12345";
    record.lotId = "LOT-2024-001";

    // Add measurements
    record.measurements["voltage"] = 12.05;
    record.measurements["current"] = 2.48;
    record.measurements["power"] = 29.88;

    // 3. Save to database
    auto saveResult = dataStore.SaveTestData(record);
    ASSERT_TRUE(saveResult.IsSuccess()) << "Failed to save test data";

    // 4. Retrieve by test ID
    STestDataRecord retrieved;
    auto getResult = dataStore.GetTestData(record.testId, retrieved);
    ASSERT_TRUE(getResult.IsSuccess()) << "Failed to retrieve test data";

    // 5. Verify retrieved data
    EXPECT_EQ(retrieved.testId, record.testId);
    EXPECT_EQ(retrieved.sequenceName, record.sequenceName);
    EXPECT_EQ(retrieved.verdict, record.verdict);
    EXPECT_EQ(retrieved.deviceId, record.deviceId);
    EXPECT_DOUBLE_EQ(retrieved.measurements["voltage"], 12.05);
    EXPECT_DOUBLE_EQ(retrieved.measurements["current"], 2.48);
    EXPECT_DOUBLE_EQ(retrieved.measurements["power"], 29.88);

    // 6. Query by lot ID
    TVector<STestDataRecord> lotRecords;
    auto queryResult = dataStore.GetTestDataByLot(record.lotId, lotRecords);
    ASSERT_TRUE(queryResult.IsSuccess());
    EXPECT_GE(lotRecords.size(), 1u);

    // 7. Cleanup
    dataStore.Close();
}

/**************************************************************************
 * Test: ReportGenerationWorkflow
 * Description: Test HTML report generation from test results
 **************************************************************************/
TEST_F(EndToEndTests, ReportGenerationWorkflow) {
    TString reportPath = m_strTempDir + "/test_report.html";

    // 1. Create test data
    STestDataRecord testData;
    testData.testId = "REPORT-TEST-001";
    testData.sequenceName = "Report Generation Test";
    testData.verdict = ETestVerdict::kPass;
    testData.deviceId = "DUT-67890";
    testData.startTime = std::chrono::system_clock::now();
    testData.endTime = testData.startTime + std::chrono::seconds(5);
    testData.measurements["voltage"] = 5.02;
    testData.measurements["current"] = 1.25;
    testData.measurements["resistance"] = 4.016;

    // 2. Generate HTML report
    CHtmlReportGenerator reportGen;
    auto beginResult = reportGen.BeginReport(reportPath);
    ASSERT_TRUE(beginResult.IsSuccess()) << "Failed to begin report";

    reportGen.AddSection("Test Summary");
    reportGen.AddData("Test ID", testData.testId);
    reportGen.AddData("Sequence", testData.sequenceName);
    reportGen.AddData("Device", testData.deviceId);
    reportGen.AddData("Verdict", testData.verdict == ETestVerdict::kPass ? "PASS" : "FAIL");

    reportGen.AddSection("Measurements");
    for (const auto& [name, value] : testData.measurements) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << value;
        reportGen.AddData(name, oss.str());
    }

    auto endResult = reportGen.EndReport();
    ASSERT_TRUE(endResult.IsSuccess()) << "Failed to end report";

    // 3. Verify report file exists
    EXPECT_TRUE(std::filesystem::exists(reportPath));

    // 4. Verify report contains expected data
    std::ifstream reportFile(reportPath);
    ASSERT_TRUE(reportFile.is_open());

    std::stringstream buffer;
    buffer << reportFile.rdbuf();
    TString reportContent = buffer.str();

    EXPECT_NE(reportContent.find("REPORT-TEST-001"), TString::npos);
    EXPECT_NE(reportContent.find("DUT-67890"), TString::npos);
    EXPECT_NE(reportContent.find("PASS"), TString::npos);
    EXPECT_NE(reportContent.find("5.020"), TString::npos);
}

/**************************************************************************
 * Test: MultiStepIntegrationWorkflow
 * Description: Test complex multi-step sequence with all step types
 **************************************************************************/
TEST_F(EndToEndTests, MultiStepIntegrationWorkflow) {
    // Create comprehensive test sequence
    CTestSequence sequence;
    SSequenceInfo info;
    info.id = "MULTI-STEP-001";
    info.name = "Multi-Step Integration Test";
    info.version = "1.0.0";
    sequence.SetInfo(info);

    // Add diverse steps

    // 1. Wait step
    auto wait1 = std::make_unique<CWaitStep>("WAIT-001", "Initial Delay", 50);
    sequence.AddStep(std::move(wait1));

    // 2. Calculation step (voltage * current = power)
    auto calc1 = std::make_unique<CCalculationStep>("CALC-001", "Calculate Power");
    calc1->SetOperation(ECalculationType::kMultiply);
    std::map<TString, TDouble> powerOps;
    powerOps["a"] = 12.0;  // Voltage
    powerOps["b"] = 2.5;   // Current
    calc1->SetOperands(powerOps);
    calc1->SetUnit("W");
    sequence.AddStep(std::move(calc1));

    // 3. Limit check on power
    auto limit1 = std::make_unique<CLimitCheckStep>("LIMIT-001", "Validate Power");
    limit1->SetValue(30.0);
    limit1->SetLimits(28.0, 32.0);
    limit1->SetUnit("W");
    sequence.AddStep(std::move(limit1));

    // 4. Another calculation (resistance = voltage / current)
    auto calc2 = std::make_unique<CCalculationStep>("CALC-002", "Calculate Resistance");
    calc2->SetOperation(ECalculationType::kDivide);
    std::map<TString, TDouble> resistanceOps;
    resistanceOps["a"] = 12.0;
    resistanceOps["b"] = 2.5;
    calc2->SetOperands(resistanceOps);
    calc2->SetUnit("Ω");
    sequence.AddStep(std::move(calc2));

    // 5. Limit check on resistance
    auto limit2 = std::make_unique<CLimitCheckStep>("LIMIT-002", "Validate Resistance");
    limit2->SetValue(4.8);
    limit2->SetLimits(4.5, 5.0);
    limit2->SetUnit("Ω");
    sequence.AddStep(std::move(limit2));

    // 6. File operation - log results
    auto file1 = std::make_unique<CFileOperationStep>("FILE-001", "Log Results");
    file1->SetOperation(EFileOperation::kWrite);
    file1->SetFilePath(m_strTempDir + "/multi_step_results.txt");
    file1->SetContent("Test completed successfully\nPower: 30.0W\nResistance: 4.8Ω\n");
    sequence.AddStep(std::move(file1));

    // 7. Final wait
    auto wait2 = std::make_unique<CWaitStep>("WAIT-002", "Final Delay", 50);
    sequence.AddStep(std::move(wait2));

    // Execute all steps
    EXPECT_EQ(sequence.GetStepCount(), 7u);

    int passCount = 0;
    int totalSteps = 0;
    SStepResult result;

    for (TUInt32 i = 0; i < sequence.GetStepCount(); ++i) {
        ITestStep* pStep = sequence.GetStep(i);
        ASSERT_NE(pStep, nullptr) << "Step " << i << " is null";

        auto execResult = pStep->Execute(result);
        EXPECT_TRUE(execResult.IsSuccess()) << "Step " << i << " (" << pStep->GetName() << ") failed";

        totalSteps++;
        if (result.verdict == ETestVerdict::kPass) {
            passCount++;
        }
    }

    // Verify all steps passed
    EXPECT_EQ(passCount, totalSteps) << "Not all steps passed";
    EXPECT_TRUE(std::filesystem::exists(m_strTempDir + "/multi_step_results.txt"));
}

/**************************************************************************
 * Test: ErrorHandlingWorkflow
 * Description: Verify proper error handling throughout the system
 **************************************************************************/
TEST_F(EndToEndTests, ErrorHandlingWorkflow) {
    // 1. Test invalid file load
    CTestSequence sequence;
    auto& fileIO = CSequenceFileIO::GetInstance();
    auto result = fileIO.LoadSequence("/nonexistent/path/sequence.json", sequence);
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_NE(result.GetErrorCode(), EErrorCode::kSuccess);

    // 2. Test limit check failure
    CLimitCheckStep limitCheck("LIMIT-FAIL", "Expected Failure");
    limitCheck.SetValue(100.0);  // Way out of range
    limitCheck.SetLimits(1.0, 10.0);

    SStepResult stepResult;
    auto execResult = limitCheck.Execute(stepResult);
    EXPECT_TRUE(execResult.IsSuccess());  // Execution succeeded
    EXPECT_EQ(stepResult.verdict, ETestVerdict::kFail);  // But test failed

    // 3. Test calculation with division by zero
    CCalculationStep divByZero("CALC-ERROR", "Division by Zero");
    divByZero.SetOperation(ECalculationType::kDivide);
    std::map<TString, TDouble> ops;
    ops["a"] = 10.0;
    ops["b"] = 0.0;  // Division by zero
    divByZero.SetOperands(ops);

    execResult = divByZero.Execute(stepResult);
    EXPECT_FALSE(execResult.IsSuccess());  // Should fail
    EXPECT_EQ(stepResult.verdict, ETestVerdict::kError);
}

/**************************************************************************
 * Test: PerformanceBaseline
 * Description: Establish performance baselines for common operations
 **************************************************************************/
TEST_F(EndToEndTests, PerformanceBaseline) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;

    // 1. Measure step creation overhead
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        CWaitStep step("PERF-" + std::to_string(i), "Performance Test", 1);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    double avgCreation = duration / 1000.0;

    EXPECT_LT(avgCreation, 100.0) << "Step creation too slow: " << avgCreation << "μs";
    std::cout << "Average step creation time: " << avgCreation << " μs" << std::endl;

    // 2. Measure calculation performance
    CCalculationStep calc("PERF-CALC", "Performance Calculation");
    calc.SetOperation(ECalculationType::kMultiply);
    std::map<TString, TDouble> ops;
    ops["a"] = 12.0;
    ops["b"] = 2.5;
    calc.SetOperands(ops);

    start = high_resolution_clock::now();
    SStepResult result;
    for (int i = 0; i < 10000; ++i) {
        calc.Execute(result);
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    double avgCalc = duration / 10000.0;

    EXPECT_LT(avgCalc, 50.0) << "Calculation too slow: " << avgCalc << "μs";
    std::cout << "Average calculation time: " << avgCalc << " μs" << std::endl;

    // 3. Measure limit check performance
    CLimitCheckStep limit("PERF-LIMIT", "Performance Limit Check");
    limit.SetValue(5.0);
    limit.SetLimits(4.5, 5.5);

    start = high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        limit.Execute(result);
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    double avgLimit = duration / 10000.0;

    EXPECT_LT(avgLimit, 50.0) << "Limit check too slow: " << avgLimit << "μs";
    std::cout << "Average limit check time: " << avgLimit << " μs" << std::endl;
}

/**************************************************************************
 * Main function
 **************************************************************************/
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
