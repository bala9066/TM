/**************************************************************************
 * File Name: StdfWriterTests.cpp
 * Description: Unit tests for STDF Writer component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/semiconductor/StdfWriter.h"
#include <filesystem>
#include <fstream>

namespace TestMATE {
namespace Tests {

class CStdfWriterTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_testDir = std::filesystem::temp_directory_path() / "testmate_stdf_tests";
        std::filesystem::create_directories(m_testDir);
        m_writer = std::make_unique<CStdfWriter>();
    }

    void TearDown() override {
        m_writer.reset();
        std::filesystem::remove_all(m_testDir);
    }

    std::filesystem::path m_testDir;
    std::unique_ptr<CStdfWriter> m_writer;
};

TEST_F(CStdfWriterTests, Open_ValidPath_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    auto result = m_writer->Open(stdfPath.string());

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(m_writer->IsOpen());
}

TEST_F(CStdfWriterTests, Open_AlreadyOpen_Failure) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->Open(stdfPath.string());
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, Close_OpenFile_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->Close();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(m_writer->IsOpen());
}

TEST_F(CStdfWriterTests, WriteFar_ValidHeader_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    SStdfHeader header;
    header.cpuType = 2;
    header.stdfVersion = 4;

    auto result = m_writer->WriteFar(header);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteFar_NotOpen_Failure) {
    SStdfHeader header;
    auto result = m_writer->WriteFar(header);
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteMir_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    SStdfMir mir;
    mir.lotId = "LOT001";
    mir.partType = "IC-TEST";
    mir.nodeNam = "TESTER01";
    mir.operNam = "OPERATOR1";
    mir.facilId = "FAB1";
    mir.floorId = "FLOOR1";

    auto result = m_writer->WriteMir(mir);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WritePir_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WritePir(1, 1);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WritePrr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    SStdfPrr prr;
    prr.headNum = 1;
    prr.siteNum = 1;
    prr.partFlag = 0;
    prr.numTest = 10;
    prr.hardBin = 1;
    prr.softBin = 1;
    prr.partId = "PART001";

    auto result = m_writer->WritePrr(prr);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WritePtr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    SStdfPtr ptr;
    ptr.testNum = 1;
    ptr.headNum = 1;
    ptr.siteNum = 1;
    ptr.testFlag = 0;
    ptr.result = 3.14f;
    ptr.testTxt = "Voltage Test";
    ptr.units = "V";
    ptr.loLimit = 0.0f;
    ptr.hiLimit = 5.0f;

    auto result = m_writer->WritePtr(ptr);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteHbr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WriteHbr(1, 100, 'P', "Pass Bin");
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteSbr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WriteSbr(1, 100, 'P', "Pass Bin");
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteWir_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WriteWir("WAFER001");
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteWrr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WriteWrr(1000, 950);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, WriteMrr_ValidData_Success) {
    auto stdfPath = m_testDir / "test.stdf";
    m_writer->Open(stdfPath.string());

    auto result = m_writer->WriteMrr();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CStdfWriterTests, CompleteFileSequence_ValidData_CreatesFile) {
    auto stdfPath = m_testDir / "complete.stdf";
    m_writer->Open(stdfPath.string());

    SStdfHeader header{2, 4};
    m_writer->WriteFar(header);

    SStdfMir mir;
    mir.lotId = "LOT001";
    mir.partType = "DEVICE";
    m_writer->WriteMir(mir);

    m_writer->WritePir(1, 1);

    SStdfPtr ptr;
    ptr.testNum = 1;
    ptr.result = 2.5f;
    m_writer->WritePtr(ptr);

    SStdfPrr prr;
    prr.partFlag = 0;
    prr.numTest = 1;
    m_writer->WritePrr(prr);

    m_writer->WriteMrr();
    m_writer->Close();

    EXPECT_TRUE(std::filesystem::exists(stdfPath));
    EXPECT_GT(std::filesystem::file_size(stdfPath), 0u);
}

TEST_F(CStdfWriterTests, FileFormat_LittleEndian_CorrectBytes) {
    auto stdfPath = m_testDir / "endian.stdf";
    m_writer->Open(stdfPath.string());

    SStdfHeader header{2, 4};
    m_writer->WriteFar(header);
    m_writer->Close();

    std::ifstream file(stdfPath, std::ios::binary);
    char buffer[6];
    file.read(buffer, 6);

    // FAR record: 2 bytes length (2), type (0), subtype (10), cpuType (2), version (4)
    EXPECT_EQ(static_cast<unsigned char>(buffer[4]), 2u);  // CPU type
    EXPECT_EQ(static_cast<unsigned char>(buffer[5]), 4u);  // STDF version
}

} // namespace Tests
} // namespace TestMATE
