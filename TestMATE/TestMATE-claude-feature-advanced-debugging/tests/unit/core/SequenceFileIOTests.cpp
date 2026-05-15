/**************************************************************************
 * File Name: SequenceFileIOTests.cpp
 * Description: Unit tests for SequenceFileIO component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/test_sequence/SequenceFileIO.h"
#include <fstream>
#include <filesystem>

namespace TestMATE {
namespace Tests {

class CSequenceFileIOTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_testDir = std::filesystem::temp_directory_path() / "testmate_seq_tests";
        std::filesystem::create_directories(m_testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(m_testDir);
    }

    std::filesystem::path m_testDir;
};

TEST_F(CSequenceFileIOTests, LoadJsonSequence_ValidFile_Success) {
    // Create test JSON file
    auto jsonPath = m_testDir / "test_sequence.json";
    std::ofstream file(jsonPath);
    file << R"({
        "name": "TestSequence",
        "version": "1.0",
        "description": "Test description",
        "steps": [
            {"id": "step1", "name": "Step 1", "type": "measurement"},
            {"id": "step2", "name": "Step 2", "type": "action"}
        ]
    })";
    file.close();

    CTestSequence sequence;
    auto result = CSequenceFileIO::GetInstance().LoadSequence(jsonPath.string(), sequence, ESequenceFileFormat::kJson);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(sequence.GetName(), "TestSequence");
    EXPECT_EQ(sequence.GetInfo().version, "1.0");
}

TEST_F(CSequenceFileIOTests, LoadJsonSequence_InvalidPath_Failure) {
    CTestSequence sequence;
    auto result = CSequenceFileIO::GetInstance().LoadSequence("/nonexistent/path.json", sequence, ESequenceFileFormat::kJson);

    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSequenceFileIOTests, SaveJsonSequence_ValidSequence_Success) {
    CTestSequence sequence("save_test", "SaveTest");

    // Modify sequence info
    auto info = sequence.GetInfo();
    info.version = "2.0";
    info.description = "Saved sequence";
    sequence.SetInfo(info);

    auto jsonPath = m_testDir / "saved_sequence.json";
    auto result = CSequenceFileIO::GetInstance().SaveSequence(jsonPath.string(), sequence, ESequenceFileFormat::kJson);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(std::filesystem::exists(jsonPath));
}

TEST_F(CSequenceFileIOTests, LoadXmlSequence_ValidFile_Success) {
    auto xmlPath = m_testDir / "test_sequence.xml";
    std::ofstream file(xmlPath);
    file << R"(<?xml version="1.0"?>
<sequence>
    <name>XMLSequence</name>
    <version>1.0</version>
    <description>XML test</description>
    <steps>
        <step id="s1" name="XMLStep1" type="test"/>
    </steps>
</sequence>)";
    file.close();

    CTestSequence sequence;
    auto result = CSequenceFileIO::GetInstance().LoadSequence(xmlPath.string(), sequence, ESequenceFileFormat::kXml);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(sequence.GetName(), "XMLSequence");
}

TEST_F(CSequenceFileIOTests, SaveXmlSequence_ValidSequence_Success) {
    CTestSequence sequence("xml_save", "XMLSaveTest");

    // Modify sequence info
    auto info = sequence.GetInfo();
    info.version = "1.0";
    sequence.SetInfo(info);

    auto xmlPath = m_testDir / "saved_sequence.xml";
    auto result = CSequenceFileIO::GetInstance().SaveSequence(xmlPath.string(), sequence, ESequenceFileFormat::kXml);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(std::filesystem::exists(xmlPath));
}

// DetectFormat is private - tested indirectly through LoadSequence with kAuto
TEST_F(CSequenceFileIOTests, AutoDetect_JsonExtension_LoadsCorrectly) {
    auto jsonPath = m_testDir / "test.json";
    std::ofstream(jsonPath) << R"({"id":"test","name":"Test","steps":[]})";

    CTestSequence sequence;
    auto result = CSequenceFileIO::GetInstance().LoadSequence(jsonPath.string(), sequence);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSequenceFileIOTests, AutoDetect_XmlExtension_LoadsCorrectly) {
    auto xmlPath = m_testDir / "test.xml";
    std::ofstream(xmlPath) << R"(<sequence id="test" name="Test"></sequence>)";

    CTestSequence sequence;
    auto result = CSequenceFileIO::GetInstance().LoadSequence(xmlPath.string(), sequence);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSequenceFileIOTests, RoundTrip_JsonFormat_DataPreserved) {
    CTestSequence original("test_id", "RoundTripTest");

    // Modify sequence info
    auto info = original.GetInfo();
    info.version = "3.0";
    info.description = "Testing round trip";
    original.SetInfo(info);

    auto jsonPath = m_testDir / "roundtrip.json";

    auto saveResult = CSequenceFileIO::GetInstance().SaveSequence(jsonPath.string(), original, ESequenceFileFormat::kJson);
    ASSERT_TRUE(saveResult.IsSuccess());

    CTestSequence loaded;
    auto loadResult = CSequenceFileIO::GetInstance().LoadSequence(jsonPath.string(), loaded, ESequenceFileFormat::kJson);
    ASSERT_TRUE(loadResult.IsSuccess());

    EXPECT_EQ(loaded.GetName(), original.GetName());
    EXPECT_EQ(loaded.GetInfo().version, original.GetInfo().version);
    EXPECT_EQ(loaded.GetInfo().description, original.GetInfo().description);
}

} // namespace Tests
} // namespace TestMATE
