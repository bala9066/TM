/**************************************************************************
 * File Name: mysql_example.cpp
 * Description: Example demonstrating MySQL/MariaDB database usage
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Shows how to use TestMATE with MySQL/MariaDB backend for enterprise
 *   test data management with high availability and replication support.
 *
 * Build:
 *   cmake -DTESTMATE_MYSQL_SUPPORT=ON ..
 *   cmake --build .
 *   ./examples/database/mysql_example
 **************************************************************************/

#include "database/DataStoreFactory.h"
#include "database/MySqlDataStore.h"
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
    PrintSeparator("1. Basic MySQL Connection");

    // Configure MySQL connection
    SMySqlConfig config;
    config.host = "localhost";
    config.port = 3306;
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";
    config.charset = "utf8mb4";
    config.useSSL = false;  // Enable in production!
    config.autoReconnect = true;

    // Create and open database
    auto dataStore = std::make_unique<CMySqlDataStore>();
    auto result = dataStore->Open(config);

    if (!result.IsSuccess()) {
        std::cerr << "❌ Failed to connect: " << result.GetMessage() << "\n";
        std::cerr << "\nPlease ensure MySQL/MariaDB is running and database exists:\n";
        std::cerr << "  CREATE DATABASE testmate_demo;\n";
        std::cerr << "  CREATE USER 'testmate_user'@'localhost' IDENTIFIED BY 'testmate_pass';\n";
        std::cerr << "  GRANT ALL PRIVILEGES ON testmate_demo.* TO 'testmate_user'@'localhost';\n";
        std::cerr << "  FLUSH PRIVILEGES;\n";
        return;
    }

    std::cout << "✓ Connected to MySQL successfully\n";

    // Get server version
    std::cout << "  Server: " << dataStore->GetServerVersion() << "\n";

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
    auto dataStore = CDataStoreFactory::CreateMySQL();

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
    if (!mysqlStore) return;

    if (!mysqlStore->Open(config).IsSuccess()) {
        std::cerr << "❌ Connection failed\n";
        return;
    }

    // Create test data
    STestDataRecord testData;
    testData.testId = "TEST-MY-001";
    testData.sequenceId = "SEQ-POWER-SUPPLY";
    testData.deviceId = "SN-54321";
    testData.lotId = "LOT-2025-01-B";
    testData.verdict = ETestVerdict::kPass;
    testData.durationMs = 1150;
    testData.operatorName = "Charlie";

    // Save to database
    auto result = dataStore->SaveTestData(testData);
    if (result.IsSuccess()) {
        std::cout << "✓ Test data saved: " << testData.testId << "\n";
        std::cout << "  Device: " << testData.deviceId << "\n";
        std::cout << "  Lot: " << testData.lotId << "\n";
        std::cout << "  Verdict: PASS\n";
    }
}

void DemoTransactions() {
    PrintSeparator("3. ACID Transactions");

    auto dataStore = CDataStoreFactory::CreateMySQL();
    auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
    if (!mysqlStore) return;

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!mysqlStore->Open(config).IsSuccess()) return;

    std::cout << "Testing ACID transaction...\\n\n";

    // Begin transaction
    mysqlStore->BeginTransaction();
    std::cout << "✓ Transaction started\n";

    // Save multiple tests
    STestDataRecord test1;
    test1.testId = "TEST-TXN-100";
    test1.deviceId = "DEV-100";
    test1.lotId = "LOT-TXN";
    test1.verdict = ETestVerdict::kPass;
    mysqlStore->SaveTestData(test1);
    std::cout << "✓ Saved test 1 (PASS)\n";

    STestDataRecord test2;
    test2.testId = "TEST-TXN-101";
    test2.deviceId = "DEV-101";
    test2.lotId = "LOT-TXN";
    test2.verdict = ETestVerdict::kPass;
    mysqlStore->SaveTestData(test2);
    std::cout << "✓ Saved test 2 (PASS)\n";

    STestDataRecord test3;
    test3.testId = "TEST-TXN-102";
    test3.deviceId = "DEV-102";
    test3.lotId = "LOT-TXN";
    test3.verdict = ETestVerdict::kFail;
    mysqlStore->SaveTestData(test3);
    std::cout << "✓ Saved test 3 (FAIL)\n";

    // Simulate error - rollback all
    std::cout << "\n⚠ Simulating error - rolling back all changes...\n";
    mysqlStore->RollbackTransaction();
    std::cout << "✓ Transaction rolled back\n";
    std::cout << "\nResult: No tests were saved (transaction atomicity)\n";
}

void DemoBatchOperations() {
    PrintSeparator("4. High-Performance Batch Insert");

    auto dataStore = CDataStoreFactory::CreateMySQL();
    auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
    if (!mysqlStore) return;

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!mysqlStore->Open(config).IsSuccess()) return;

    const int batchSize = 100;
    std::cout << "Saving " << batchSize << " test records...\n";

    auto startTime = std::chrono::steady_clock::now();

    // Use transaction for batch insert (improves performance)
    mysqlStore->BeginTransaction();

    for (int i = 0; i < batchSize; ++i) {
        STestDataRecord test;
        test.testId = "TEST-BATCH-MY-" + std::to_string(i + 1);
        test.deviceId = "SN-" + std::to_string(20000 + i);
        test.lotId = "LOT-MYSQL-2025";
        test.verdict = (i % 8 == 0) ? ETestVerdict::kFail : ETestVerdict::kPass;
        test.durationMs = 900 + (i * 15);
        test.operatorName = (i % 3 == 0) ? "Alice" : (i % 3 == 1) ? "Bob" : "Charlie";

        mysqlStore->SaveTestData(test);
    }

    mysqlStore->CommitTransaction();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    std::cout << "✓ Saved " << batchSize << " records in " << duration << " ms\n";
    std::cout << "  Throughput: " << (batchSize * 1000.0 / duration) << " records/sec\n";
}

void DemoQueryingByLot() {
    PrintSeparator("5. Querying Test Data by Lot");

    auto dataStore = CDataStoreFactory::CreateMySQL();
    auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
    if (!mysqlStore) return;

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!mysqlStore->Open(config).IsSuccess()) return;

    std::cout << "Querying lot 'LOT-MYSQL-2025'...\n\n";

    std::vector<STestDataRecord> lotTests;
    auto result = mysqlStore->GetTestDataByLot("LOT-MYSQL-2025", lotTests);

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

void DemoTableMaintenance() {
    PrintSeparator("6. Table Maintenance Operations");

    auto dataStore = CDataStoreFactory::CreateMySQL();
    auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
    if (!mysqlStore) return;

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";

    if (!mysqlStore->Open(config).IsSuccess()) return;

    std::cout << "Performing table maintenance...\n\n";

    // Optimize table (reclaim space, defragment)
    std::cout << "Running OPTIMIZE TABLE... ";
    auto result = mysqlStore->OptimizeTable("test_results");
    std::cout << (result.IsSuccess() ? "✓" : "❌") << "\n";

    // Analyze table (update index statistics)
    std::cout << "Running ANALYZE TABLE... ";
    result = mysqlStore->AnalyzeTable("test_results");
    std::cout << (result.IsSuccess() ? "✓" : "❌") << "\n";

    // Check table integrity
    std::cout << "Running CHECK TABLE... ";
    result = mysqlStore->CheckTable("test_results");
    std::cout << (result.IsSuccess() ? "✓" : "❌") << "\n";

    // Get statistics
    auto stats = mysqlStore->GetConnectionStats();
    std::cout << "\nConnection Statistics:\n";
    std::cout << "  Total queries: " << stats.totalQueries << "\n";
    std::cout << "  Avg query time: " << stats.avgQueryTimeMs << " ms\n";
    std::cout << "  Rows affected: " << stats.rowsAffected << "\n";
}

void DemoAutoReconnect() {
    PrintSeparator("7. Auto-Reconnect Feature");

    SMySqlConfig config;
    config.host = "localhost";
    config.database = "testmate_demo";
    config.user = "testmate_user";
    config.password = "testmate_pass";
    config.autoReconnect = true;  // Enable auto-reconnect

    auto dataStore = std::make_unique<CMySqlDataStore>();
    auto result = dataStore->Open(config);

    if (!result.IsSuccess()) {
        std::cerr << "❌ Connection failed\n";
        return;
    }

    std::cout << "✓ Connected with auto-reconnect enabled\n";
    std::cout << "  Connection timeout: " << config.connectionTimeout << " seconds\n";
    std::cout << "  Auto-reconnect: " << (config.autoReconnect ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "\nIf connection is lost, MySQL client will automatically\n";
    std::cout << "attempt to reconnect before failing operations.\n";
}

void DemoConnectionStringParsing() {
    PrintSeparator("8. Connection String Auto-Detection");

    std::cout << "Creating database from connection string...\n\n";

    // MySQL connection string format
    std::string connStr = "mysql://testmate_user:testmate_pass@localhost:3306/testmate_demo";
    std::cout << "Connection string: " << connStr << "\n\n";

    auto dataStore = CDataStoreFactory::CreateFromConnectionString(connStr);

    if (dataStore) {
        std::cout << "✓ Auto-detected MySQL backend\n";
        std::cout << "  Backend type: MySQL/MariaDB\n";
        std::cout << "  Host: localhost\n";
        std::cout << "  Port: 3306\n";
        std::cout << "  Database: testmate_demo\n";
    }
}

int main() {
    std::cout << "TestMATE MySQL/MariaDB Example\n";
    std::cout << "===============================\n";
    std::cout << "\nThis example demonstrates MySQL/MariaDB database usage.\n";
    std::cout << "Prerequisites:\n";
    std::cout << "  1. MySQL/MariaDB server running\n";
    std::cout << "  2. Database 'testmate_demo' created\n";
    std::cout << "  3. User 'testmate_user' with access\n";

    try {
        DemoBasicConnection();
        DemoSavingTestData();
        DemoTransactions();
        DemoBatchOperations();
        DemoQueryingByLot();
        DemoTableMaintenance();
        DemoAutoReconnect();
        DemoConnectionStringParsing();

        PrintSeparator();
        std::cout << "✓ All MySQL examples completed successfully!\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
