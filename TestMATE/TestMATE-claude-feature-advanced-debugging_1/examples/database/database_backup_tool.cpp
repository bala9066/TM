/**************************************************************************
 * File Name: database_backup_tool.cpp
 * Description: Database backup utility for TestMATE
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Creates backups of TestMATE databases in JSON or SQL format
 *
 * Build:
 *   cmake -DTESTMATE_POSTGRESQL_SUPPORT=ON -DTESTMATE_MYSQL_SUPPORT=ON ..
 *   cmake --build .
 *
 * Usage:
 *   ./database_backup_tool <database> <output_file> [--format=json|sql]
 *
 *   Examples:
 *     ./database_backup_tool sqlite:test_data.db backup.json
 *     ./database_backup_tool postgresql:localhost/testmate backup.sql --format=sql
 *     ./database_backup_tool mysql:localhost/testmate backup.json --format=json
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
#include <iomanip>
#include <sstream>

using namespace TestMATE;

enum class EBackupFormat {
    kJSON,
    kSQL
};

/**************************************************************************
 * Connection String Parser (reused from migration tool)
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
 * JSON Escaping Helper
 **************************************************************************/
std::string EscapeJSON(const std::string& str) {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:   escaped << c; break;
        }
    }
    return escaped.str();
}

/**************************************************************************
 * Backup Functions
 **************************************************************************/
bool BackupToJSON(IDataStore* dataStore, const std::string& outputFile) {
    std::cout << "Creating JSON backup...\n";

    SQueryResult queryResult;
    auto result = dataStore->Query("SELECT * FROM test_results", queryResult);

    if (!result.IsSuccess()) {
        std::cerr << "Failed to query database: " << result.GetMessage() << "\n";
        return false;
    }

    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputFile << "\n";
        return false;
    }

    // Write JSON header
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    outFile << "{\n";
    outFile << "  \"backup_info\": {\n";
    outFile << "    \"created_at\": \"" << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << "\",\n";
    outFile << "    \"tool\": \"TestMATE Database Backup Tool\",\n";
    outFile << "    \"version\": \"2.0.0\",\n";
    outFile << "    \"record_count\": " << queryResult.rows.size() << "\n";
    outFile << "  },\n";
    outFile << "  \"test_results\": [\n";

    // Write test records
    for (size_t i = 0; i < queryResult.rows.size(); ++i) {
        const auto& row = queryResult.rows[i];

        outFile << "    {\n";
        if (row.size() > 1) outFile << "      \"test_id\": \"" << EscapeJSON(row[1]) << "\",\n";
        if (row.size() > 2) outFile << "      \"sequence_id\": \"" << EscapeJSON(row[2]) << "\",\n";
        if (row.size() > 3) outFile << "      \"device_id\": \"" << EscapeJSON(row[3]) << "\",\n";
        if (row.size() > 4) outFile << "      \"lot_id\": \"" << EscapeJSON(row[4]) << "\",\n";
        if (row.size() > 5) outFile << "      \"verdict\": " << row[5] << ",\n";
        if (row.size() > 6) outFile << "      \"duration_ms\": " << row[6] << ",\n";
        if (row.size() > 7) outFile << "      \"operator_name\": \"" << EscapeJSON(row[7]) << "\"\n";
        outFile << "    }";

        if (i < queryResult.rows.size() - 1) {
            outFile << ",";
        }
        outFile << "\n";

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << queryResult.rows.size() << "\r" << std::flush;
        }
    }

    outFile << "  ]\n";
    outFile << "}\n";

    outFile.close();

    std::cout << "\n✓ Backup complete: " << queryResult.rows.size() << " records saved to " << outputFile << "\n";
    return true;
}

bool BackupToSQL(IDataStore* dataStore, const std::string& outputFile) {
    std::cout << "Creating SQL backup...\n";

    SQueryResult queryResult;
    auto result = dataStore->Query("SELECT * FROM test_results", queryResult);

    if (!result.IsSuccess()) {
        std::cerr << "Failed to query database: " << result.GetMessage() << "\n";
        return false;
    }

    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputFile << "\n";
        return false;
    }

    // Write SQL header
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    outFile << "-- TestMATE Database Backup\n";
    outFile << "-- Created: " << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << "\n";
    outFile << "-- Records: " << queryResult.rows.size() << "\n";
    outFile << "\n";

    outFile << "-- Disable foreign key checks during restore\n";
    outFile << "SET FOREIGN_KEY_CHECKS = 0;\n\n";

    // Write INSERT statements
    for (size_t i = 0; i < queryResult.rows.size(); ++i) {
        const auto& row = queryResult.rows[i];

        if (i == 0 || i % 1000 == 0) {
            if (i > 0) outFile << ";\n\n";
            outFile << "INSERT INTO test_results (test_id, sequence_id, device_id, lot_id, verdict, duration_ms, operator_name) VALUES\n";
        } else {
            outFile << ",\n";
        }

        outFile << "  ('" << (row.size() > 1 ? row[1] : "") << "', "
                << "'" << (row.size() > 2 ? row[2] : "") << "', "
                << "'" << (row.size() > 3 ? row[3] : "") << "', "
                << "'" << (row.size() > 4 ? row[4] : "") << "', "
                << (row.size() > 5 ? row[5] : "0") << ", "
                << (row.size() > 6 ? row[6] : "0") << ", "
                << "'" << (row.size() > 7 ? row[7] : "") << "')";

        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << queryResult.rows.size() << "\r" << std::flush;
        }
    }

    outFile << ";\n\n";
    outFile << "-- Re-enable foreign key checks\n";
    outFile << "SET FOREIGN_KEY_CHECKS = 1;\n";

    outFile.close();

    std::cout << "\n✓ Backup complete: " << queryResult.rows.size() << " records saved to " << outputFile << "\n";
    return true;
}

/**************************************************************************
 * Main
 **************************************************************************/
int main(int argc, char* argv[]) {
    std::cout << "TestMATE Database Backup Tool\n";
    std::cout << "==============================\n\n";

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <database> <output_file> [--format=json|sql]\n\n";
        std::cout << "Arguments:\n";
        std::cout << "  database:     Connection string (same format as migration tool)\n";
        std::cout << "  output_file:  Path to output backup file\n";
        std::cout << "  --format:     Backup format (json or sql, default: json)\n\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " sqlite:test_data.db backup.json\n";
        std::cout << "  " << argv[0] << " postgresql:localhost/testmate backup.sql --format=sql\n";
        std::cout << "  " << argv[0] << " mysql:localhost/testmate backup.json --format=json\n";
        return 1;
    }

    std::string dbConnStr = argv[1];
    std::string outputFile = argv[2];
    EBackupFormat format = EBackupFormat::kJSON;

    // Parse format argument
    if (argc >= 4) {
        std::string formatArg = argv[3];
        if (formatArg.find("--format=sql") == 0) {
            format = EBackupFormat::kSQL;
        } else if (formatArg.find("--format=json") == 0) {
            format = EBackupFormat::kJSON;
        }
    }

    std::cout << "Database: " << dbConnStr << "\n";
    std::cout << "Output: " << outputFile << "\n";
    std::cout << "Format: " << (format == EBackupFormat::kJSON ? "JSON" : "SQL") << "\n\n";

    // Parse connection string
    auto dbInfo = ParseConnectionString(dbConnStr);

    // Connect to database
    std::cout << "Connecting to database...\n";
    auto dataStore = OpenDatabase(dbInfo);
    if (!dataStore) {
        return 1;
    }
    std::cout << "✓ Connected\n\n";

    // Perform backup
    auto startTime = std::chrono::steady_clock::now();

    bool success = false;
    if (format == EBackupFormat::kJSON) {
        success = BackupToJSON(dataStore.get(), outputFile);
    } else {
        success = BackupToSQL(dataStore.get(), outputFile);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    std::cout << "Duration: " << duration << " ms\n";

    // Close database
    dataStore->Close();

    return success ? 0 : 1;
}
