/**************************************************************************
 * File Name: QueryParameterizedTests.cpp
 * Description: Unit tests for SQL injection prevention via QueryParameterized
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 **************************************************************************/

#include <gtest/gtest.h>
#include "database/SqliteDataStore.h"
#include <filesystem>

using namespace TestMATE;

class QueryParameterizedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use in-memory database for fast tests
        m_dataStore = std::make_unique<CSqliteDataStore>();
        auto result = m_dataStore->Connect(":memory:");
        ASSERT_TRUE(result.IsSuccess()) << "Failed to connect: " << result.GetMessage();

        // Initialize schema
        result = m_dataStore->InitializeSchema();
        ASSERT_TRUE(result.IsSuccess()) << "Failed to initialize schema: " << result.GetMessage();
    }

    void TearDown() override {
        if (m_dataStore && m_dataStore->IsConnected()) {
            m_dataStore->Disconnect();
        }
    }

    TUniquePtr<CSqliteDataStore> m_dataStore;
};

/**************************************************************************
 * Test: Parameterized Query with String Parameter
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithStringParameter_BindsCorrectly) {
    // Insert test data
    STestDataRecord record;
    record.lotId = "LOT-12345";
    record.sequenceName = "PowerSupplyTest";
    record.deviceId = "DEV-001";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    auto result = m_dataStore->InsertTestData(record, recordId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with parameterized string
    TVector<CSqliteDataStore::TQueryParameter> params = {TString("LOT-12345")};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    ASSERT_EQ(queryResult.rows.size(), 1u) << "Expected 1 row";
    EXPECT_FALSE(queryResult.rows[0].empty());
}

/**************************************************************************
 * Test: Parameterized Query with Int64 Parameter
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithInt64Parameter_BindsCorrectly) {
    // Insert test data with specific record ID
    STestDataRecord record;
    record.lotId = "LOT-99999";
    record.sequenceName = "Test";
    record.deviceId = "DEV-INT";
    record.verdict = ETestVerdict::kPass;

    TUInt64 insertedId = 0;
    auto result = m_dataStore->InsertTestData(record, insertedId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with parameterized int64
    TVector<CSqliteDataStore::TQueryParameter> params = {TInt64(insertedId)};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    ASSERT_EQ(queryResult.rows.size(), 1u) << "Expected 1 row";
}

/**************************************************************************
 * Test: Parameterized Query with Double Parameter
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithDoubleParameter_BindsCorrectly) {
    // This test verifies double parameter binding
    // Even though test_data table doesn't have a double column,
    // we can test the parameter binding mechanism

    TVector<CSqliteDataStore::TQueryParameter> params = {TDouble(42.5)};

    // Query with double parameter (will return no results, but shouldn't crash)
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE id = ?",
        params
    );

    // Should succeed even with no results
    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 0u);
}

/**************************************************************************
 * Test: Parameterized Query with Multiple Parameters
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithMultipleParameters_BindsAllCorrectly) {
    // Insert test data
    STestDataRecord record1, record2, record3;

    record1.lotId = "LOT-AAA";
    record1.sequenceName = "Test1";
    record1.deviceId = "DEV-001";
    record1.verdict = ETestVerdict::kPass;

    record2.lotId = "LOT-BBB";
    record2.sequenceName = "Test2";
    record2.deviceId = "DEV-002";
    record2.verdict = ETestVerdict::kFail;

    record3.lotId = "LOT-CCC";
    record3.sequenceName = "Test3";
    record3.deviceId = "DEV-003";
    record3.verdict = ETestVerdict::kPass;

    TUInt64 id1, id2, id3;
    m_dataStore->InsertTestData(record1, id1);
    m_dataStore->InsertTestData(record2, id2);
    m_dataStore->InsertTestData(record3, id3);

    // Query with multiple parameters
    TVector<CSqliteDataStore::TQueryParameter> params = {
        TString("LOT-AAA"),
        TString("LOT-CCC")
    };

    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ? OR lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 2u) << "Expected 2 rows (LOT-AAA and LOT-CCC)";
}

/**************************************************************************
 * Test: SQL Injection Prevention
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithSQLInjectionAttempt_IsSafe) {
    // Insert normal test data
    STestDataRecord record;
    record.lotId = "LOT-SAFE";
    record.sequenceName = "NormalTest";
    record.deviceId = "DEV-001";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    m_dataStore->InsertTestData(record, recordId);

    // Attempt SQL injection via parameterized query
    // This should be treated as a literal string, not SQL code
    TString maliciousInput = "LOT-SAFE' OR '1'='1";

    TVector<CSqliteDataStore::TQueryParameter> params = {maliciousInput};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    // Query should succeed but return 0 rows (injection prevented)
    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 0u)
        << "SQL injection NOT prevented! Malicious query returned data";

    // Verify that only our safe record exists
    auto safeQuery = m_dataStore->QueryParameterized(
        "SELECT COUNT(*) FROM test_data",
        {}
    );
    ASSERT_TRUE(safeQuery.success);
    EXPECT_EQ(safeQuery.rows.size(), 1u);
    // Should have exactly 1 record (our safe one)
}

/**************************************************************************
 * Test: SQL Injection with UNION Attack
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithUnionInjectionAttempt_IsSafe) {
    // Insert test data
    STestDataRecord record;
    record.lotId = "LOT-123";
    record.sequenceName = "Test";
    record.deviceId = "DEV-001";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    m_dataStore->InsertTestData(record, recordId);

    // Attempt UNION-based SQL injection
    TString unionAttack = "LOT-123' UNION SELECT * FROM test_data--";

    TVector<CSqliteDataStore::TQueryParameter> params = {unionAttack};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    // Should treat as literal string, return 0 rows
    ASSERT_TRUE(queryResult.success);
    EXPECT_EQ(queryResult.rows.size(), 0u)
        << "UNION injection NOT prevented!";
}

/**************************************************************************
 * Test: SQL Injection with Comment Attack
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithCommentInjectionAttempt_IsSafe) {
    // Insert test data
    STestDataRecord record;
    record.lotId = "LOT-456";
    record.sequenceName = "Test";
    record.deviceId = "DEV-002";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    m_dataStore->InsertTestData(record, recordId);

    // Attempt comment-based injection
    TString commentAttack = "LOT-456' --";

    TVector<CSqliteDataStore::TQueryParameter> params = {commentAttack};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ? AND device_id = 'DEV-002'",
        params
    );

    // Should treat as literal string, return 0 rows
    ASSERT_TRUE(queryResult.success);
    EXPECT_EQ(queryResult.rows.size(), 0u)
        << "Comment injection NOT prevented!";
}

/**************************************************************************
 * Test: Empty Parameter List
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithEmptyParameterList_ReturnsAllRows) {
    // Insert multiple records
    for (int i = 0; i < 5; ++i) {
        STestDataRecord record;
        record.lotId = "LOT-" + std::to_string(i);
        record.sequenceName = "Test";
        record.deviceId = "DEV-" + std::to_string(i);
        record.verdict = ETestVerdict::kPass;

        TUInt64 recordId = 0;
        m_dataStore->InsertTestData(record, recordId);
    }

    // Query with no parameters
    TVector<CSqliteDataStore::TQueryParameter> params;
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data",
        params
    );

    ASSERT_TRUE(queryResult.success);
    EXPECT_EQ(queryResult.rows.size(), 5u);
}

/**************************************************************************
 * Test: Complex Query with Mixed Parameter Types
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithMixedParameterTypes_HandlesCorrectly) {
    // Insert test data
    STestDataRecord record;
    record.lotId = "LOT-MIXED";
    record.sequenceName = "MixedTest";
    record.deviceId = "DEV-999";
    record.verdict = ETestVerdict::kPass;

    TUInt64 insertedId = 0;
    m_dataStore->InsertTestData(record, insertedId);

    // Query with mixed types: string, int64, string
    TVector<CSqliteDataStore::TQueryParameter> params = {
        TString("LOT-MIXED"),
        TInt64(insertedId),
        TString("MixedTest")
    };

    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ? AND id = ? AND sequence_name = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 1u);
}

/**************************************************************************
 * Test: Special Characters in Parameters
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithSpecialCharacters_EscapesCorrectly) {
    // Insert test data with special characters
    STestDataRecord record;
    record.lotId = "LOT-'quotes\"both";
    record.sequenceName = "Test<>Special&Chars";
    record.deviceId = "DEV-001";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    auto result = m_dataStore->InsertTestData(record, recordId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with special characters (should be properly escaped)
    TVector<CSqliteDataStore::TQueryParameter> params = {TString("LOT-'quotes\"both")};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 1u) << "Special characters not handled correctly";
}

/**************************************************************************
 * Test: Unicode Characters in Parameters
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithUnicodeCharacters_HandlesCorrectly) {
    // Insert test data with Unicode characters
    STestDataRecord record;
    record.lotId = "LOT-日本語-测试-🚀";
    record.sequenceName = "UnicodeTest";
    record.deviceId = "DEV-UTF8";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    auto result = m_dataStore->InsertTestData(record, recordId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with Unicode
    TVector<CSqliteDataStore::TQueryParameter> params = {TString("LOT-日本語-测试-🚀")};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 1u) << "Unicode not handled correctly";
}

/**************************************************************************
 * Test: Very Long String Parameter
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithVeryLongString_HandlesCorrectly) {
    // Create a very long lot ID (1000 characters)
    TString longLotId(1000, 'X');
    longLotId = "LOT-" + longLotId;

    STestDataRecord record;
    record.lotId = longLotId;
    record.sequenceName = "LongStringTest";
    record.deviceId = "DEV-LONG";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    auto result = m_dataStore->InsertTestData(record, recordId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with long string
    TVector<CSqliteDataStore::TQueryParameter> params = {longLotId};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success) << "Query failed: " << queryResult.errorMessage;
    EXPECT_EQ(queryResult.rows.size(), 1u);
}

/**************************************************************************
 * Test: Null/Empty String Parameter
 **************************************************************************/
TEST_F(QueryParameterizedTest, WithEmptyString_HandlesCorrectly) {
    // Insert with empty lot ID
    STestDataRecord record;
    record.lotId = "";
    record.sequenceName = "EmptyTest";
    record.deviceId = "DEV-EMPTY";
    record.verdict = ETestVerdict::kPass;

    TUInt64 recordId = 0;
    auto result = m_dataStore->InsertTestData(record, recordId);
    ASSERT_TRUE(result.IsSuccess());

    // Query with empty string
    TVector<CSqliteDataStore::TQueryParameter> params = {TString("")};
    auto queryResult = m_dataStore->QueryParameterized(
        "SELECT * FROM test_data WHERE lot_id = ?",
        params
    );

    ASSERT_TRUE(queryResult.success);
    EXPECT_EQ(queryResult.rows.size(), 1u);
}

/**************************************************************************
 * Test: Performance - Parameterized vs Raw Query
 * NOTE: This is a basic performance check, not a comprehensive benchmark
 **************************************************************************/
TEST_F(QueryParameterizedTest, Performance_IsReasonable) {
    // Insert test data
    const int NUM_RECORDS = 100;
    for (int i = 0; i < NUM_RECORDS; ++i) {
        STestDataRecord record;
        record.lotId = "LOT-PERF-" + std::to_string(i);
        record.sequenceName = "PerfTest";
        record.deviceId = "DEV-" + std::to_string(i);
        record.verdict = ETestVerdict::kPass;

        TUInt64 recordId = 0;
        m_dataStore->InsertTestData(record, recordId);
    }

    // Time parameterized queries
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_RECORDS; ++i) {
        TVector<CSqliteDataStore::TQueryParameter> params = {
            TString("LOT-PERF-" + std::to_string(i))
        };
        auto result = m_dataStore->QueryParameterized(
            "SELECT * FROM test_data WHERE lot_id = ?",
            params
        );
        ASSERT_TRUE(result.success);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in reasonable time (< 1 second for 100 queries)
    EXPECT_LT(duration.count(), 1000)
        << "Parameterized queries too slow: " << duration.count() << "ms";
}
