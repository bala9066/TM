/**************************************************************************
 * File Name: database_migration_tool.cpp
 * Description: Database migration utility for TestMATE
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Migrates test data between different database backends:
 *   - SQLite → PostgreSQL
 *   - SQLite → MySQL
 *   - PostgreSQL → MySQL
 *   - MySQL → PostgreSQL
 *
 * Build:
 *   cmake -DTESTMATE_POSTGRESQL_SUPPORT=ON -DTESTMATE_MYSQL_SUPPORT=ON ..
 *   cmake --build .
 *
 * Usage:
 *   ./database_migration_tool <source> <destination>
 *
 *   Examples:
 *     ./database_migration_tool sqlite:test_data.db postgresql:localhost/testmate
 *     ./database_migration_tool sqlite:test_data.db mysql:localhost/testmate
 *     ./database_migration_tool postgresql:server1/db1 postgresql:server2/db2
 **************************************************************************/

#include "database/DataStoreFactory.h"
#include "database/IDataStore.h"
#include "database/SqliteDataStore.h"

#ifdef TESTMATE_POSTGRESQL_SUPPORT
#include "database/PostgreSqlDataStore.h"
#endif

#ifdef TESTMATE_MYSQL_SUPPORT
#include "database/MySqlDataStore.h"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace TestMATE;

/**************************************************************************
 * Connection String Parser
 **************************************************************************/
struct SConnectionInfo {
    EDataStoreType type;
    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string password;
    std::string filePath;  // For SQLite
};

SConnectionInfo ParseConnectionString(const std::string& connStr) {
    SConnectionInfo info;

    if (connStr.find("sqlite:") == 0) {
        info.type = EDataStoreType::kSQLite;
        info.filePath = connStr.substr(7);  // Remove "sqlite:"
    }
    else if (connStr.find("postgresql:") == 0) {
        info.type = EDataStoreType::kPostgreSQL;
        // Format: postgresql:host/database or postgresql:host:port/database
        std::string rest = connStr.substr(11);  // Remove "postgresql:"

        size_t slashPos = rest.find('/');
        if (slashPos != std::string::npos) {
            std::string hostPart = rest.substr(0, slashPos);
            info.database = rest.substr(slashPos + 1);

            size_t colonPos = hostPart.find(':');
            if (colonPos != std::string::npos) {
                info.host = hostPart.substr(0, colonPos);
                info.port = std::stoi(hostPart.substr(colonPos + 1));
            } else {
                info.host = hostPart;
                info.port = 5432;
            }
        }

        // Prompt for credentials
        std::cout << "PostgreSQL User: ";
        std::getline(std::cin, info.user);
        std::cout << "PostgreSQL Password: ";
        std::getline(std::cin, info.password);
    }
    else if (connStr.find("mysql:") == 0) {
        info.type = EDataStoreType::kMySQL;
        // Format: mysql:host/database or mysql:host:port/database
        std::string rest = connStr.substr(6);  // Remove "mysql:"

        size_t slashPos = rest.find('/');
        if (slashPos != std::string::npos) {
            std::string hostPart = rest.substr(0, slashPos);
            info.database = rest.substr(slashPos + 1);

            size_t colonPos = hostPart.find(':');
            if (colonPos != std::string::npos) {
                info.host = hostPart.substr(0, colonPos);
                info.port = std::stoi(hostPart.substr(colonPos + 1));
            } else {
                info.host = hostPart;
                info.port = 3306;
            }
        }

        // Prompt for credentials
        std::cout << "MySQL User: ";
        std::getline(std::cin, info.user);
        std::cout << "MySQL Password: ";
        std::getline(std::cin, info.password);
    }
    else {
        std::cerr << "Unknown database type in connection string: " << connStr << "\n";
        info.type = EDataStoreType::kSQLite;
    }

    return info;
}

/**************************************************************************
 * Database Connection Helper
 **************************************************************************/
std::unique_ptr<IDataStore> OpenDatabase(const SConnectionInfo& info) {
    std::unique_ptr<IDataStore> dataStore;

    switch (info.type) {
        case EDataStoreType::kSQLite: {
            dataStore = CDataStoreFactory::CreateSQLite();
            auto result = dataStore->Open(info.filePath);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open SQLite: " << result.GetMessage() << "\n";
                return nullptr;
            }
            break;
        }

#ifdef TESTMATE_POSTGRESQL_SUPPORT
        case EDataStoreType::kPostgreSQL: {
            dataStore = CDataStoreFactory::CreatePostgreSQL();
            auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());

            SPostgreSqlConfig config;
            config.host = info.host;
            config.port = info.port;
            config.database = info.database;
            config.user = info.user;
            config.password = info.password;

            auto result = pgStore->Open(config);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open PostgreSQL: " << result.GetMessage() << "\n";
                return nullptr;
            }

            // Initialize schema if needed
            pgStore->InitializeSchema();
            break;
        }
#endif

#ifdef TESTMATE_MYSQL_SUPPORT
        case EDataStoreType::kMySQL: {
            dataStore = CDataStoreFactory::CreateMySQL();
            auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());

            SMySqlConfig config;
            config.host = info.host;
            config.port = info.port;
            config.database = info.database;
            config.user = info.user;
            config.password = info.password;

            auto result = mysqlStore->Open(config);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open MySQL: " << result.GetMessage() << "\n";
                return nullptr;
            }

            // Initialize schema if needed
            mysqlStore->InitializeSchema();
            break;
        }
#endif

        default:
            std::cerr << "Database backend not compiled in!\n";
            return nullptr;
    }

    return dataStore;
}

/**************************************************************************
 * Migration Function
 **************************************************************************/
bool MigrateData(IDataStore* source, IDataStore* destination) {
    std::cout << "\nStarting migration...\n";
    std::cout << "=========================================\n\n";

    auto startTime = std::chrono::steady_clock::now();

    // Query all test data from source
    // Note: This is a simplified version. In a real tool, you'd want to:
    // 1. Query in batches to handle large datasets
    // 2. Get test data by date ranges or other criteria
    // 3. Migrate step results and measurements too

    SQueryResult queryResult;
    auto result = source->Query("SELECT * FROM test_results", queryResult);

    if (!result.IsSuccess()) {
        std::cerr << "Failed to query source database: " << result.GetMessage() << "\n";
        return false;
    }

    int totalRecords = queryResult.rows.size();
    int migratedCount = 0;
    int errorCount = 0;

    std::cout << "Found " << totalRecords << " test records to migrate\n\n";

    // Begin transaction on destination for better performance
    destination->BeginTransaction();

    // Migrate each record
    for (size_t i = 0; i < queryResult.rows.size(); ++i) {
        const auto& row = queryResult.rows[i];

        STestDataRecord testData;

        // Parse row data (simplified - assumes specific column order)
        if (row.size() >= 8) {
            testData.testId = row[1];        // test_id
            testData.sequenceId = row[2];    // sequence_id
            testData.deviceId = row[3];      // device_id
            testData.lotId = row[4];         // lot_id
            testData.verdict = static_cast<ETestVerdict>(std::stoi(row[5]));  // verdict
            testData.durationMs = std::stoi(row[6]);  // duration_ms
            testData.operatorName = row[7];  // operator_name
        }

        // Save to destination
        auto saveResult = destination->SaveTestData(testData);

        if (saveResult.IsSuccess()) {
            migratedCount++;

            // Progress indicator
            if ((i + 1) % 100 == 0) {
                std::cout << "Progress: " << (i + 1) << "/" << totalRecords
                          << " (" << ((i + 1) * 100 / totalRecords) << "%)\r" << std::flush;
            }
        } else {
            errorCount++;
            std::cerr << "\nError migrating " << testData.testId << ": "
                      << saveResult.GetMessage() << "\n";
        }
    }

    // Commit transaction
    destination->CommitTransaction();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    std::cout << "\n\n=========================================\n";
    std::cout << "Migration Complete!\n";
    std::cout << "=========================================\n";
    std::cout << "Total records: " << totalRecords << "\n";
    std::cout << "Migrated: " << migratedCount << "\n";
    std::cout << "Errors: " << errorCount << "\n";
    std::cout << "Duration: " << duration << " ms\n";
    std::cout << "Throughput: " << (migratedCount * 1000.0 / duration) << " records/sec\n";

    return errorCount == 0;
}

/**************************************************************************
 * Main
 **************************************************************************/
int main(int argc, char* argv[]) {
    std::cout << "TestMATE Database Migration Tool\n";
    std::cout << "=================================\n\n";

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <source> <destination>\n\n";
        std::cout << "Connection String Formats:\n";
        std::cout << "  SQLite:     sqlite:<path>\n";
        std::cout << "              Example: sqlite:test_data.db\n\n";
        std::cout << "  PostgreSQL: postgresql:<host>/<database>\n";
        std::cout << "              postgresql:<host>:<port>/<database>\n";
        std::cout << "              Example: postgresql:localhost/testmate\n\n";
        std::cout << "  MySQL:      mysql:<host>/<database>\n";
        std::cout << "              mysql:<host>:<port>/<database>\n";
        std::cout << "              Example: mysql:localhost/testmate\n\n";
        std::cout << "Examples:\n";
        std::cout << "  Migrate SQLite to PostgreSQL:\n";
        std::cout << "    " << argv[0] << " sqlite:test_data.db postgresql:localhost/testmate\n\n";
        std::cout << "  Migrate SQLite to MySQL:\n";
        std::cout << "    " << argv[0] << " sqlite:test_data.db mysql:localhost/testmate\n\n";
        std::cout << "  Migrate PostgreSQL to MySQL:\n";
        std::cout << "    " << argv[0] << " postgresql:server1/db1 mysql:server2/db2\n\n";
        return 1;
    }

    std::string sourceConnStr = argv[1];
    std::string destConnStr = argv[2];

    std::cout << "Source: " << sourceConnStr << "\n";
    std::cout << "Destination: " << destConnStr << "\n\n";

    // Parse connection strings
    std::cout << "Parsing source connection...\n";
    auto sourceInfo = ParseConnectionString(sourceConnStr);

    std::cout << "Parsing destination connection...\n";
    auto destInfo = ParseConnectionString(destConnStr);

    // Connect to databases
    std::cout << "\nConnecting to source database...\n";
    auto sourceDB = OpenDatabase(sourceInfo);
    if (!sourceDB) {
        return 1;
    }
    std::cout << "✓ Connected to source\n";

    std::cout << "\nConnecting to destination database...\n";
    auto destDB = OpenDatabase(destInfo);
    if (!destDB) {
        return 1;
    }
    std::cout << "✓ Connected to destination\n";

    // Perform migration
    bool success = MigrateData(sourceDB.get(), destDB.get());

    // Close databases
    sourceDB->Close();
    destDB->Close();

    return success ? 0 : 1;
}
