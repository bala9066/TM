/**************************************************************************
 * File Name: database_restore_tool.cpp
 * Description: Database restore utility for TestMATE
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Restores TestMATE databases from JSON or SQL backup files
 *
 * Build:
 *   cmake -DTESTMATE_POSTGRESQL_SUPPORT=ON -DTESTMATE_MYSQL_SUPPORT=ON ..
 *   cmake --build .
 *
 * Usage:
 *   ./database_restore_tool <database> <backup_file> [--format=json|sql]
 *
 *   Examples:
 *     ./database_restore_tool sqlite:test_data.db backup.json
 *     ./database_restore_tool postgresql:localhost/testmate backup.sql --format=sql
 *     ./database_restore_tool mysql:localhost/testmate backup.json --format=json
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
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>

using namespace TestMATE;

enum class ERestoreFormat {
    kJSON,
    kSQL
};

/**************************************************************************
 * Connection String Parser (reused)
 **************************************************************************/
struct SConnectionInfo {
    EDataStoreType type;
    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string password;
    std::string filePath;
};

SConnectionInfo ParseConnectionString(const std::string& connStr) {
    SConnectionInfo info;

    if (connStr.find("sqlite:") == 0) {
        info.type = EDataStoreType::kSQLite;
        info.filePath = connStr.substr(7);
    }
    else if (connStr.find("postgresql:") == 0) {
        info.type = EDataStoreType::kPostgreSQL;
        std::string rest = connStr.substr(11);
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
        std::cout << "PostgreSQL User: ";
        std::getline(std::cin, info.user);
        std::cout << "PostgreSQL Password: ";
        std::getline(std::cin, info.password);
    }
    else if (connStr.find("mysql:") == 0) {
        info.type = EDataStoreType::kMySQL;
        std::string rest = connStr.substr(6);
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
        std::cout << "MySQL User: ";
        std::getline(std::cin, info.user);
        std::cout << "MySQL Password: ";
        std::getline(std::cin, info.password);
    }

    return info;
}

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
 * Simple JSON Parser (for basic test data restoration)
 **************************************************************************/
std::string ExtractJSONValue(const std::string& line, const std::string& key) {
    size_t keyPos = line.find("\"" + key + "\":");
    if (keyPos == std::string::npos) return "";

    size_t valueStart = line.find('"', keyPos + key.length() + 3);
    if (valueStart == std::string::npos) {
        // Number value
        valueStart = line.find(':', keyPos + key.length() + 2) + 1;
        size_t valueEnd = line.find_first_of(",\n}", valueStart);
        std::string value = line.substr(valueStart, valueEnd - valueStart);
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t,") + 1);
        return value;
    }

    size_t valueEnd = line.find('"', valueStart + 1);
    return line.substr(valueStart + 1, valueEnd - valueStart - 1);
}

bool RestoreFromJSON(IDataStore* dataStore, const std::string& backupFile) {
    std::cout << "Restoring from JSON backup...\n";

    std::ifstream inFile(backupFile);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open backup file: " << backupFile << "\n";
        return false;
    }

    std::string line;
    int restoredCount = 0;
    int errorCount = 0;
    bool inTestResults = false;

    STestDataRecord currentTest;
    bool hasTestData = false;

    dataStore->BeginTransaction();

    while (std::getline(inFile, line)) {
        if (line.find("\"test_results\"") != std::string::npos) {
            inTestResults = true;
            continue;
        }

        if (!inTestResults) continue;

        // Parse test record
        std::string testId = ExtractJSONValue(line, "test_id");
        if (!testId.empty()) {
            if (hasTestData) {
                // Save previous test
                auto result = dataStore->SaveTestData(currentTest);
                if (result.IsSuccess()) {
                    restoredCount++;
                } else {
                    errorCount++;
                }
            }

            // Start new test
            currentTest = STestDataRecord();
            currentTest.testId = testId;
            hasTestData = true;
        }

        if (hasTestData) {
            std::string value;

            value = ExtractJSONValue(line, "sequence_id");
            if (!value.empty()) currentTest.sequenceId = value;

            value = ExtractJSONValue(line, "device_id");
            if (!value.empty()) currentTest.deviceId = value;

            value = ExtractJSONValue(line, "lot_id");
            if (!value.empty()) currentTest.lotId = value;

            value = ExtractJSONValue(line, "verdict");
            if (!value.empty()) currentTest.verdict = static_cast<ETestVerdict>(std::stoi(value));

            value = ExtractJSONValue(line, "duration_ms");
            if (!value.empty()) currentTest.durationMs = std::stoi(value);

            value = ExtractJSONValue(line, "operator_name");
            if (!value.empty()) currentTest.operatorName = value;
        }

        if ((restoredCount + 1) % 100 == 0) {
            std::cout << "Progress: " << restoredCount << " records\r" << std::flush;
        }
    }

    // Save last test
    if (hasTestData) {
        auto result = dataStore->SaveTestData(currentTest);
        if (result.IsSuccess()) {
            restoredCount++;
        } else {
            errorCount++;
        }
    }

    dataStore->CommitTransaction();

    inFile.close();

    std::cout << "\n✓ Restore complete: " << restoredCount << " records restored\n";
    if (errorCount > 0) {
        std::cout << "⚠ Errors: " << errorCount << " records failed\n";
    }

    return errorCount == 0;
}

bool RestoreFromSQL(IDataStore* dataStore, const std::string& backupFile) {
    std::cout << "Restoring from SQL backup...\n";

    std::ifstream inFile(backupFile);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open backup file: " << backupFile << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string sqlContent = buffer.str();

    inFile.close();

    // Execute SQL directly (simplified version)
    SQueryResult result;
    auto execResult = dataStore->Query(sqlContent, result);

    if (execResult.IsSuccess()) {
        std::cout << "✓ SQL restore complete\n";
        return true;
    } else {
        std::cerr << "❌ SQL restore failed: " << execResult.GetMessage() << "\n";
        return false;
    }
}

/**************************************************************************
 * Main
 **************************************************************************/
int main(int argc, char* argv[]) {
    std::cout << "TestMATE Database Restore Tool\n";
    std::cout << "===============================\n\n";

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <database> <backup_file> [--format=json|sql]\n\n";
        std::cout << "Arguments:\n";
        std::cout << "  database:     Connection string (same format as backup tool)\n";
        std::cout << "  backup_file:  Path to backup file to restore\n";
        std::cout << "  --format:     Backup format (json or sql, default: auto-detect)\n\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " sqlite:test_data.db backup.json\n";
        std::cout << "  " << argv[0] << " postgresql:localhost/testmate backup.sql --format=sql\n";
        std::cout << "  " << argv[0] << " mysql:localhost/testmate backup.json --format=json\n";
        return 1;
    }

    std::string dbConnStr = argv[1];
    std::string backupFile = argv[2];
    ERestoreFormat format = ERestoreFormat::kJSON;

    // Auto-detect format from file extension
    if (backupFile.find(".sql") != std::string::npos) {
        format = ERestoreFormat::kSQL;
    } else if (backupFile.find(".json") != std::string::npos) {
        format = ERestoreFormat::kJSON;
    }

    // Override with command-line argument if provided
    if (argc >= 4) {
        std::string formatArg = argv[3];
        if (formatArg.find("--format=sql") == 0) {
            format = ERestoreFormat::kSQL;
        } else if (formatArg.find("--format=json") == 0) {
            format = ERestoreFormat::kJSON;
        }
    }

    std::cout << "Database: " << dbConnStr << "\n";
    std::cout << "Backup file: " << backupFile << "\n";
    std::cout << "Format: " << (format == ERestoreFormat::kJSON ? "JSON" : "SQL") << "\n\n";

    std::cout << "⚠ WARNING: This will add data to the database.\n";
    std::cout << "Make sure the database is empty or you want to merge data.\n";
    std::cout << "Continue? (y/n): ";

    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") {
        std::cout << "Restore cancelled.\n";
        return 0;
    }

    // Parse connection string
    auto dbInfo = ParseConnectionString(dbConnStr);

    // Connect to database
    std::cout << "\nConnecting to database...\n";
    auto dataStore = OpenDatabase(dbInfo);
    if (!dataStore) {
        return 1;
    }
    std::cout << "✓ Connected\n\n";

    // Perform restore
    auto startTime = std::chrono::steady_clock::now();

    bool success = false;
    if (format == ERestoreFormat::kJSON) {
        success = RestoreFromJSON(dataStore.get(), backupFile);
    } else {
        success = RestoreFromSQL(dataStore.get(), backupFile);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    std::cout << "Duration: " << duration << " ms\n";

    // Close database
    dataStore->Close();

    return success ? 0 : 1;
}
