/**************************************************************************
 * File Name: MySqlDataStore.h
 * Description: MySQL/MariaDB implementation of IDataStore interface
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Provides MySQL/MariaDB database backend for test data storage with
 *   support for replication, clustering, and high availability.
 *
 * Features:
 *   - Full IDataStore interface implementation
 *   - Connection pooling support
 *   - Transaction management with ACID properties
 *   - Prepared statements for performance and security
 *   - SSL/TLS connection support
 *   - Auto-reconnect on connection loss
 **************************************************************************/

#pragma once

#include "database/IDataStore.h"
#include "utils/TestMateTypes.h"
#include "utils/Result.h"
#include <memory>
#include <mutex>

// Forward declaration (MySQL C API)
typedef struct st_mysql MYSQL;
typedef struct st_mysql_res MYSQL_RES;
typedef struct st_mysql_stmt MYSQL_STMT;

namespace TestMATE {

/**************************************************************************
 * Struct: SMySqlConfig
 * Description: MySQL/MariaDB connection configuration
 **************************************************************************/
struct SMySqlConfig {
    TString host{"localhost"};
    TInt32 port{3306};
    TString database{"testmate"};
    TString user{"testmate_user"};
    TString password;
    TString charset{"utf8mb4"};
    bool useSSL{false};
    TString sslCA;              // SSL CA certificate path
    TString sslCert;            // SSL client certificate path
    TString sslKey;             // SSL client key path
    TInt32 connectionTimeout{30};  // seconds
    bool autoReconnect{true};
};

/**************************************************************************
 * Class: CMySqlDataStore
 * Description: MySQL/MariaDB database backend implementation
 *
 * Thread Safety: Thread-safe with mutex protection
 *
 * Usage Example:
 *
 *   SMySqlConfig config;
 *   config.host = "db.example.com";
 *   config.database = "testmate_prod";
 *   config.user = "testmate_user";
 *   config.password = "secure_password";
 *   config.useSSL = true;
 *
 *   CMySqlDataStore dataStore;
 *   auto result = dataStore.Open(config);
 *   if (result.IsSuccess()) {
 *       // Save test data
 *       dataStore.SaveTestData(testData);
 *   }
 **************************************************************************/
class CMySqlDataStore : public IDataStore {
public:
    CMySqlDataStore();
    ~CMySqlDataStore() override;

    // Delete copy/move
    CMySqlDataStore(const CMySqlDataStore&) = delete;
    CMySqlDataStore& operator=(const CMySqlDataStore&) = delete;
    CMySqlDataStore(CMySqlDataStore&&) = delete;
    CMySqlDataStore& operator=(CMySqlDataStore&&) = delete;

    /**************************************************************************
     * Connection Management
     **************************************************************************/

    // Open database connection with configuration
    CResult Open(const SMySqlConfig& config);

    // IDataStore interface - uses default config
    CResult Open(const TString& in_strConnectionString) override;

    CResult Close() override;
    [[nodiscard]] bool IsOpen() const override;

    /**************************************************************************
     * Data Operations (IDataStore interface)
     **************************************************************************/

    CResult SaveTestData(const STestDataRecord& in_data) override;
    CResult GetTestData(const TString& in_strTestId, STestDataRecord& out_data) override;
    CResult UpdateTestData(const STestDataRecord& in_data) override;
    CResult DeleteTestData(const TString& in_strTestId) override;

    /**************************************************************************
     * Query Operations
     **************************************************************************/

    CResult GetTestDataByLot(const TString& in_strLotId,
                            std::vector<STestDataRecord>& out_data) override;

    CResult GetTestDataByDateRange(const TString& in_strStartDate,
                                   const TString& in_strEndDate,
                                   std::vector<STestDataRecord>& out_data) override;

    CResult Query(const TString& in_strQuery, SQueryResult& out_result) override;

    /**************************************************************************
     * Transaction Support
     **************************************************************************/

    CResult BeginTransaction() override;
    CResult CommitTransaction() override;
    CResult RollbackTransaction() override;

    /**************************************************************************
     * Schema Management
     **************************************************************************/

    // Initialize database schema
    CResult InitializeSchema() override;

    // Check if schema exists
    [[nodiscard]] bool SchemaExists() const;

    // Get schema version
    [[nodiscard]] TInt32 GetSchemaVersion() const;

    /**************************************************************************
     * MySQL-Specific Features
     **************************************************************************/

    // Get connection statistics
    struct SConnectionStats {
        TInt64 totalQueries{0};
        TInt64 totalConnections{0};
        TDouble avgQueryTimeMs{0.0};
        TInt64 rowsAffected{0};
    };
    [[nodiscard]] SConnectionStats GetConnectionStats() const;

    // Optimize table
    CResult OptimizeTable(const TString& in_strTableName);

    // Analyze table
    CResult AnalyzeTable(const TString& in_strTableName);

    // Repair table (MyISAM only)
    CResult RepairTable(const TString& in_strTableName);

    // Check table integrity
    CResult CheckTable(const TString& in_strTableName);

    // Get server version
    [[nodiscard]] TString GetServerVersion() const;

    // Get server status
    [[nodiscard]] TString GetServerStatus() const;

private:
    // Connection
    MYSQL* m_pConnection;
    SMySqlConfig m_config;
    mutable std::mutex m_mutex;
    bool m_bInTransaction;

    // Statistics
    TInt64 m_queryCount;
    TInt64 m_connectionCount;
    TDouble m_totalQueryTimeMs;
    TInt64 m_rowsAffected;

    // Helper methods
    CResult ExecuteQuery(const TString& in_strQuery);
    CResult ExecuteQueryWithResult(const TString& in_strQuery, MYSQL_RES** out_pResult);

    TString EscapeString(const TString& in_str) const;
    CResult ParseTestData(MYSQL_RES* in_pResult, STestDataRecord& out_data);

    bool CheckConnection();
    CResult Reconnect();

    void SetSSLOptions();
};

} // namespace TestMATE
