/**************************************************************************
 * File Name: SqliteDataStoreTests.cpp
 * Description: Unit tests for SqliteDataStore component
 **************************************************************************/

#include <gtest/gtest.h>
#include "database/SqliteDataStore.h"
#include <filesystem>

namespace TestMATE {
namespace Tests {

class CSqliteDataStoreTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_testDir = std::filesystem::temp_directory_path() / "testmate_db_tests";
        std::filesystem::create_directories(m_testDir);
        m_dataStore = std::make_unique<CSqliteDataStore>();
    }

    void TearDown() override {
        m_dataStore.reset();
        std::filesystem::remove_all(m_testDir);
    }

    STestDataRecord CreateSampleRecord() {
        STestDataRecord record;
        record.sequenceName = "TestSequence";
        record.lotId = "LOT-001";
        record.deviceId = "SN-123";
        // operatorName field doesn't exist in STestDataRecord
        record.verdict = ETestVerdict::kPass;
        record.durationMs = 1500;
        return record;
    }

    std::filesystem::path m_testDir;
    std::unique_ptr<CSqliteDataStore> m_dataStore;
};

TEST_F(CSqliteDataStoreTests, Connect_InMemory_Success) {
    auto result = m_dataStore->Connect(":memory:");
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(m_dataStore->IsConnected());
}

TEST_F(CSqliteDataStoreTests, Connect_FileDatabase_Success) {
    auto dbPath = m_testDir / "test.db";
    auto result = m_dataStore->Connect(dbPath.string());

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(m_dataStore->IsConnected());
    EXPECT_TRUE(std::filesystem::exists(dbPath));
}

TEST_F(CSqliteDataStoreTests, Disconnect_Connected_Success) {
    m_dataStore->Connect(":memory:");
    auto result = m_dataStore->Disconnect();

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(m_dataStore->IsConnected());
}

TEST_F(CSqliteDataStoreTests, InsertTestData_ValidRecord_Success) {
    m_dataStore->Connect(":memory:");
    auto record = CreateSampleRecord();
    TUInt64 id = 0;

    auto result = m_dataStore->InsertTestData(record, id);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_GT(id, 0u);
}

TEST_F(CSqliteDataStoreTests, GetTestData_ExistingRecord_ReturnsData) {
    m_dataStore->Connect(":memory:");
    auto record = CreateSampleRecord();
    TUInt64 id = 0;
    m_dataStore->InsertTestData(record, id);

    STestDataRecord retrieved;
    auto result = m_dataStore->GetTestData(id, retrieved);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(retrieved.sequenceName, record.sequenceName);
    EXPECT_EQ(retrieved.lotId, record.lotId);
    EXPECT_EQ(retrieved.deviceId, record.deviceId);
}

TEST_F(CSqliteDataStoreTests, GetTestData_NonExistingId_Failure) {
    m_dataStore->Connect(":memory:");

    STestDataRecord record;
    auto result = m_dataStore->GetTestData(99999, record);

    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, DeleteTestData_ExistingRecord_Success) {
    m_dataStore->Connect(":memory:");
    auto record = CreateSampleRecord();
    TUInt64 id = 0;
    m_dataStore->InsertTestData(record, id);

    auto result = m_dataStore->DeleteTestData(id);

    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, QueryTestData_ByLotId_ReturnsMatches) {
    m_dataStore->Connect(":memory:");

    auto record1 = CreateSampleRecord();
    record1.lotId = "LOT-A";
    auto record2 = CreateSampleRecord();
    record2.lotId = "LOT-B";
    auto record3 = CreateSampleRecord();
    record3.lotId = "LOT-A";

    TUInt64 id;
    m_dataStore->InsertTestData(record1, id);
    m_dataStore->InsertTestData(record2, id);
    m_dataStore->InsertTestData(record3, id);

    SQueryFilter filter;
    filter.lotId = "LOT-A";
    TVector<STestDataRecord> results;

    auto result = m_dataStore->QueryTestData(filter, results);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(CSqliteDataStoreTests, GetRecordCount_AfterInserts_ReturnsCorrectCount) {
    m_dataStore->Connect(":memory:");

    TUInt64 id;
    for (int i = 0; i < 5; ++i) {
        m_dataStore->InsertTestData(CreateSampleRecord(), id);
    }

    EXPECT_EQ(m_dataStore->GetRecordCount(), 5u);
}

TEST_F(CSqliteDataStoreTests, BeginTransaction_Success) {
    m_dataStore->Connect(":memory:");
    auto result = m_dataStore->BeginTransaction();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, CommitTransaction_Success) {
    m_dataStore->Connect(":memory:");
    m_dataStore->BeginTransaction();
    auto result = m_dataStore->CommitTransaction();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, RollbackTransaction_Success) {
    m_dataStore->Connect(":memory:");
    m_dataStore->BeginTransaction();
    auto result = m_dataStore->RollbackTransaction();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, Transaction_RollbackUndoesInsert) {
    m_dataStore->Connect(":memory:");

    m_dataStore->BeginTransaction();
    TUInt64 id;
    m_dataStore->InsertTestData(CreateSampleRecord(), id);
    m_dataStore->RollbackTransaction();

    STestDataRecord record;
    auto result = m_dataStore->GetTestData(id, record);
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, InsertTestData_NotConnected_Failure) {
    auto record = CreateSampleRecord();
    TUInt64 id;
    auto result = m_dataStore->InsertTestData(record, id);
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSqliteDataStoreTests, QueryTestData_WithLimit_RespectsLimit) {
    m_dataStore->Connect(":memory:");

    TUInt64 id;
    for (int i = 0; i < 10; ++i) {
        m_dataStore->InsertTestData(CreateSampleRecord(), id);
    }

    SQueryFilter filter;
    filter.limit = 3;
    TVector<STestDataRecord> results;

    m_dataStore->QueryTestData(filter, results);
    EXPECT_EQ(results.size(), 3u);
}

} // namespace Tests
} // namespace TestMATE
