/**************************************************************************
 * File Name: ConfigManagerTests.cpp
 * Description: Unit tests for ConfigManager component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/config/ConfigManager.h"
#include <fstream>
#include <filesystem>

namespace TestMATE {
namespace Tests {

class CConfigManagerTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_testDir = std::filesystem::temp_directory_path() / "testmate_config_tests";
        std::filesystem::create_directories(m_testDir);
        m_configMgr = &CConfigManager::GetInstance();
    }

    void TearDown() override {
        std::filesystem::remove_all(m_testDir);
    }

    std::filesystem::path m_testDir;
    CConfigManager* m_configMgr;
};

TEST_F(CConfigManagerTests, GetInstance_MultipleCalls_SameInstance) {
    auto& instance1 = CConfigManager::GetInstance();
    auto& instance2 = CConfigManager::GetInstance();

    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(CConfigManagerTests, LoadSystemConfig_ValidFile_Success) {
    auto configPath = m_testDir / "system.ini";
    std::ofstream file(configPath);
    file << "[General]\n";
    file << "ApplicationName=TestMATE\n";
    file << "Version=1.0\n";
    file << "[Logging]\n";
    file << "Level=Debug\n";
    file.close();

    auto result = m_configMgr->LoadSystemConfig(configPath.string());
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CConfigManagerTests, LoadSystemConfig_InvalidPath_Failure) {
    auto result = m_configMgr->LoadSystemConfig("/nonexistent/config.ini");
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CConfigManagerTests, GetValue_ExistingKey_ReturnsValue) {
    auto configPath = m_testDir / "test.ini";
    std::ofstream file(configPath);
    file << "[Section]\n";
    file << "Key=TestValue\n";
    file.close();

    m_configMgr->LoadSystemConfig(configPath.string());
    auto value = m_configMgr->GetString("Section.Key");

    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "TestValue");
}

TEST_F(CConfigManagerTests, GetValue_NonExistingKey_ReturnsEmpty) {
    auto value = m_configMgr->GetString("NonExistent.Key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(CConfigManagerTests, SetValue_NewKey_Stored) {
    m_configMgr->SetString("TestSection.TestKey", "TestValue");
    auto value = m_configMgr->GetString("TestSection.TestKey");

    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "TestValue");
}

TEST_F(CConfigManagerTests, LoadLimitsFile_ValidCSV_Success) {
    auto limitsPath = m_testDir / "limits.csv";
    std::ofstream file(limitsPath);
    file << "TestName,LowLimit,HighLimit,Unit\n";
    file << "Voltage,1.0,5.0,V\n";
    file << "Current,0.0,100.0,mA\n";
    file.close();

    auto result = m_configMgr->LoadLimitsFile(limitsPath.string());
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CConfigManagerTests, GetLimit_ExistingTest_ReturnsLimit) {
    SLimitDefinition limit;
    limit.testName = "TestLimit";
    limit.lowLimit = 0.0;
    limit.highLimit = 10.0;
    limit.unit = "V";

    m_configMgr->AddLimit(limit);
    auto retrieved = m_configMgr->GetLimit("TestLimit");

    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->testName, "TestLimit");
    EXPECT_TRUE(retrieved->lowLimit.has_value());
    EXPECT_TRUE(retrieved->highLimit.has_value());
    EXPECT_DOUBLE_EQ(retrieved->lowLimit.value(), 0.0);
    EXPECT_DOUBLE_EQ(retrieved->highLimit.value(), 10.0);
}

TEST_F(CConfigManagerTests, GetLimit_NonExistingTest_ReturnsEmpty) {
    auto limit = m_configMgr->GetLimit("NonExistentTest");
    EXPECT_FALSE(limit.has_value());
}

TEST_F(CConfigManagerTests, AddLimit_ValidLimit_Stored) {
    SLimitDefinition limit;
    limit.testName = "NewLimit";
    limit.lowLimit = -5.0;
    limit.highLimit = 5.0;
    limit.unit = "mV";

    m_configMgr->AddLimit(limit);
    auto retrieved = m_configMgr->GetLimit("NewLimit");

    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->unit, "mV");
}

TEST_F(CConfigManagerTests, SaveConfig_ToFile_Success) {
    m_configMgr->SetString("SaveSection.SaveKey", "SaveValue");

    auto savePath = m_testDir / "saved_config.ini";
    auto result = m_configMgr->SaveSystemConfig(savePath.string());

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(std::filesystem::exists(savePath));
}

TEST_F(CConfigManagerTests, GetIntValue_ValidNumber_ReturnsInt) {
    m_configMgr->SetInt("Numbers.IntVal", 42);
    auto value = m_configMgr->GetInt("Numbers.IntVal");

    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 42);
}

TEST_F(CConfigManagerTests, GetIntValue_InvalidNumber_ReturnsDefault) {
    m_configMgr->SetString("Numbers.BadVal", "notanumber");
    auto value = m_configMgr->GetInt("Numbers.BadVal");

    EXPECT_FALSE(value.has_value());
}

TEST_F(CConfigManagerTests, GetDoubleValue_ValidNumber_ReturnsDouble) {
    m_configMgr->SetFloat("Numbers.DoubleVal", 3.14159);
    auto value = m_configMgr->GetFloat("Numbers.DoubleVal");

    EXPECT_TRUE(value.has_value());
    EXPECT_NEAR(value.value(), 3.14159, 0.00001);
}

} // namespace Tests
} // namespace TestMATE
