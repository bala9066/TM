/**************************************************************************
 * File Name: postgresql_example.cpp
 * Description: Example demonstrating PostgreSQL database usage
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Shows how to use TestMATE with PostgreSQL backend for enterprise
 *   test data management with advanced features.
 *
 * Build:
 *   cmake -DTESTMATE_POSTGRESQL_SUPPORT=ON ..
 *   cmake --build .
 *   ./examples/database/postgresql_example
 **************************************************************************/

#include "database/DataStoreFactory.h"
#include "database/PostgreSqlDataStore.h"
#include "core/test_sequence/TestSequence.h"
#include "examples/test_steps/WaitStep.h"
#include "examples/test_steps/LimitCheckStep.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace TestMATE;

void PrintSeparator(const std::string& title = "") {
    std::cout << "\n========================================\n";
    if (!title.empty()) {
        std::cout << title << "\n";
        std::cout << "========================================\n";
    }
}

void DemoBasicConnection() {
    PrintSeparator("1. Basic PostgreSQL Connection");

    // Configure PostgreSQL connection
    SPostgreSqlConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";
    config.useSSL = false;  // Enable in production!
    config.schema = "public";

    // Create and open database
    auto dataStore = std::make_unique<CPostgreSqlDataStore>();
    auto result = dataStore->Open(config);

    if (!result.IsSuccess()) {
        std::cerr << "❌ Failed to connect: " << result.GetMessage() << "\n";
        std::cerr << "\nPlease ensure PostgreSQL is running and database exists:\n";
        std::cerr << "  CREATE DATABASE testmate_demo;\n";
        std::cerr << "  CREATE USER testmate_user WITH PASSWORD 'testmate_pass';\n";
        std::cerr << "  GRANT ALL PRIVILEGES ON DATABASE testmate_demo TO testmate_user;\n";
        return;
    }

    std::cout << "✓ Connected to PostgreSQL successfully\n";

    // Initialize schema
    result = dataStore->InitializeSchema();
    if (result.IsSuccess()) {
        std::cout << "✓ Database schema initialized\n";
    }

    dataStore->Close();
}

void DemoSavingTestData() {
    PrintSeparator("2. Saving Test Data");

    // Use factory for connection
    auto dataStore = CDataStoreFactory::CreatePostgreSQL();

    SPostgreSqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
    if (!pgStore) return;

    if (!pgStore->Open(config).IsSuccess()) {
        std::cerr << "❌ Connection failed\n";
        return;
    }

    // Create test data
    STestDataRecord testData;
    testData.testId = "TEST-PG-001";
    testData.sequenceId = "SEQ-POWER-SUPPLY";
    testData.deviceId = "SN-12345";
    testData.lotId = "LOT-2025-01-A";
    testData.verdict = ETestVerdict::kPass;
    testData.durationMs = 1250;
    testData.operatorName = "Alice";

    // Save to database
    auto result = dataStore->SaveTestData(testData);
    if (result.IsSuccess()) {
        std::cout << "✓ Test data saved: " << testData.testId << "\n";
        std::cout << "  Device: " << testData.deviceId << "\n";
        std::cout << "  Lot: " << testData.lotId << "\n";
        std::cout << "  Verdict: PASS\n";
    }
}

void DemoTransactionsAndSavepoints() {
    PrintSeparator("3. Transactions with Savepoints");

    auto dataStore = CDataStoreFactory::CreatePostgreSQL();
    auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
    if (!pgStore) return;

    SPostgreSqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!pgStore->Open(config).IsSuccess()) return;

    std::cout << "Testing transaction with savepoints...\n\n";

    // Begin transaction
    pgStore->BeginTransaction();
    std::cout << "✓ Transaction started\n";

    // Save first test
    STestDataRecord test1;
    test1.testId = "TEST-TXN-001";
    test1.deviceId = "DEV-001";
    test1.lotId = "LOT-TXN";
    test1.verdict = ETestVerdict::kPass;
    pgStore->SaveTestData(test1);
    std::cout << "✓ Saved test 1\n";

    // Create savepoint
    pgStore->CreateSavepoint("after_test1");
    std::cout << "✓ Created savepoint 'after_test1'\n";

    // Save second test (simulate failure)
    STestDataRecord test2;
    test2.testId = "TEST-TXN-002";
    test2.deviceId = "DEV-002";
    test2.lotId = "LOT-TXN";
    test2.verdict = ETestVerdict::kFail;
    pgStore->SaveTestData(test2);
    std::cout << "✓ Saved test 2 (FAIL)\n";

    // Rollback to savepoint (undo test 2)
    std::cout << "\n⚠ Rolling back to savepoint (undoing test 2)...\n";
    pgStore->RollbackToSavepoint("after_test1");
    std::cout << "✓ Rolled back to 'after_test1'\n";

    // Commit transaction (only test1 is saved)
    pgStore->CommitTransaction();
    std::cout << "✓ Transaction committed\n";
    std::cout << "\nResult: Only TEST-TXN-001 was saved\n";
}

void DemoBatchOperations() {
    PrintSeparator("4. Batch Operations (High Performance)");

    auto dataStore = CDataStoreFactory::CreatePostgreSQL();
    auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
    if (!pgStore) return;

    SPostgreSqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!pgStore->Open(config).IsSuccess()) return;

    const int batchSize = 100;
    std::cout << "Saving " << batchSize << " test records...\n";

    auto startTime = std::chrono::steady_clock::now();

    // Use transaction for batch insert
    pgStore->BeginTransaction();

    for (int i = 0; i < batchSize; ++i) {
        STestDataRecord test;
        test.testId = "TEST-BATCH-" + std::to_string(i + 1);
        test.deviceId = "SN-" + std::to_string(10000 + i);
        test.lotId = "LOT-BATCH-2025";
        test.verdict = (i % 10 == 0) ? ETestVerdict::kFail : ETestVerdict::kPass;
        test.durationMs = 1000 + (i * 10);
        test.operatorName = (i % 2 == 0) ? "Alice" : "Bob";

        pgStore->SaveTestData(test);
    }

    pgStore->CommitTransaction();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    std::cout << "✓ Saved " << batchSize << " records in " << duration << " ms\n";
    std::cout << "  Throughput: " << (batchSize * 1000.0 / duration) << " records/sec\n";
}

void DemoQueryingByLot() {
    PrintSeparator("5. Querying Test Data by Lot");

    auto dataStore = CDataStoreFactory::CreatePostgreSQL();
    auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
    if (!pgStore) return;

    SPostgreSqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!pgStore->Open(config).IsSuccess()) return;

    std::cout << "Querying lot 'LOT-BATCH-2025'...\n\n";

    std::vector<STestDataRecord> lotTests;
    auto result = pgStore->GetTestDataByLot("LOT-BATCH-2025", lotTests);

    if (result.IsSuccess()) {
        std::cout << "Found " << lotTests.size() << " tests in lot\n\n";

        int passCount = 0;
        int failCount = 0;

        for (const auto& test : lotTests) {
            if (test.verdict == ETestVerdict::kPass) passCount++;
            else if (test.verdict == ETestVerdict::kFail) failCount++;
        }

        std::cout << "Test Results:\n";
        std::cout << "  PASS: " << passCount << " ("
                  << (passCount * 100.0 / lotTests.size()) << "%)\n";
        std::cout << "  FAIL: " << failCount << " ("
                  << (failCount * 100.0 / lotTests.size()) << "%)\n";
        std::cout << "\nYield: " << (passCount * 100.0 / lotTests.size()) << "%\n";
    }
}

void DemoMaintenanceOperations() {
    PrintSeparator("6. Database Maintenance");

    auto dataStore = CDataStoreFactory::CreatePostgreSQL();
    auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
    if (!pgStore) return;

    SPostgreSqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!pgStore->Open(config).IsSuccess()) return;

    std::cout << "Performing database maintenance...\n\n";

    // Vacuum (reclaim space)
    std::cout << "Running VACUUM... ";
    auto result = pgStore->Vacuum();
    std::cout << (result.IsSuccess() ? "✓" : "❌") << "\n";

    // Analyze (update statistics)
    std::cout << "Running ANALYZE... ";
    result = pgStore->Analyze();
    std::cout << (result.IsSuccess() ? "✓" : "❌") << "\n";

    // Get statistics
    auto stats = pgStore->GetConnectionStats();
    std::cout << "\nConnection Statistics:\n";
    std::cout << "  Total queries: " << stats.totalQueries << "\n";
    std::cout << "  Avg query time: " << stats.avgQueryTimeMs << " ms\n";
}

int main() {
    std::cout << "TestMATE PostgreSQL Example\n";
    std::cout << "===========================\n";
    std::cout << "\nThis example demonstrates PostgreSQL database usage.\n";
    std::cout << "Prerequisites:\n";
    std::cout << "  1. PostgreSQL server running\n";
    std::cout << "  2. Database 'testmate_demo' created\n";
    std::cout << "  3. User 'testmate_user' with access\n";

    try {
        DemoBasicConnection();
        DemoSavingTestData();
        DemoTransactionsAndSavepoints();
        DemoBatchOperations();
        DemoQueryingByLot();
        DemoMaintenanceOperations();

        PrintSeparator();
        std::cout << "✓ All PostgreSQL examples completed successfully!\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
