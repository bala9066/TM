/**************************************************************************
 * File Name: PostgreSqlDataStore.h
 * Description: PostgreSQL implementation of IDataStore interface
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Provides PostgreSQL database backend for test data storage with
 *   support for enterprise features like replication, concurrent access,
 *   and advanced querying.
 *
 * Features:
 *   - Full IDataStore interface implementation
 *   - Connection pooling support
 *   - Transaction management with savepoints
 *   - Prepared statements for performance
 *   - SSL/TLS connection support
 *   - Schema versioning
 **************************************************************************/

#pragma once

#include "database/IDataStore.h"
#include "utils/TestMateTypes.h"
#include "utils/Result.h"
#include <memory>
#include <mutex>

// Forward declaration (libpq)
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

namespace TestMATE {

/**************************************************************************
 * Struct: SPostgreSqlConfig
 * Description: PostgreSQL connection configuration
 **************************************************************************/
struct SPostgreSqlConfig {
    TString host{"localhost"};
    TInt32 port{5432};
    TString database{"testmate"};
    TString user{"testmate_user"};
    TString password;
    TString schema{"public"};
    bool useSSL{false};
    TInt32 connectionTimeout{30};  // seconds
    TInt32 maxConnections{10};     // for pooling
};

/**************************************************************************
 * Class: CPostgreSqlDataStore
 * Description: PostgreSQL database backend implementation
 *
 * Thread Safety: Thread-safe with mutex protection
 *
 * Usage Example:
 *
 *   SPostgreSqlConfig config;
 *   config.host = "db.example.com";
 *   config.database = "testmate_prod";
 *   config.user = "testmate_user";
 *   config.password = "secure_password";
 *   config.useSSL = true;
 *
 *   CPostgreSqlDataStore dataStore;
 *   auto result = dataStore.Open(config);
 *   if (result.IsSuccess()) {
 *       // Save test data
 *       dataStore.SaveTestData(testData);
 *   }
 **************************************************************************/
class CPostgreSqlDataStore : public IDataStore {
public:
    CPostgreSqlDataStore();
    ~CPostgreSqlDataStore() override;

    // Delete copy/move
    CPostgreSqlDataStore(const CPostgreSqlDataStore&) = delete;
    CPostgreSqlDataStore& operator=(const CPostgreSqlDataStore&) = delete;
    CPostgreSqlDataStore(CPostgreSqlDataStore&&) = delete;
    CPostgreSqlDataStore& operator=(CPostgreSqlDataStore&&) = delete;

    /**************************************************************************
     * Connection Management
     **************************************************************************/

    // Open database connection with configuration
    CResult Open(const SPostgreSqlConfig& config);

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

    // PostgreSQL-specific: Savepoints
    CResult CreateSavepoint(const TString& in_strName);
    CResult RollbackToSavepoint(const TString& in_strName);
    CResult ReleaseSavepoint(const TString& in_strName);

    /**************************************************************************
     * Schema Management
     **************************************************************************/

    // Initialize database schema
    CResult InitializeSchema() override;

    // Check if schema exists
    [[nodiscard]] bool SchemaExists() const;

    // Get schema version
    [[nodiscard]] TInt32 GetSchemaVersion() const;

    // Migrate schema to latest version
    CResult MigrateSchema();

    /**************************************************************************
     * PostgreSQL-Specific Features
     **************************************************************************/

    // Get connection statistics
    struct SConnectionStats {
        TInt32 activeConnections{0};
        TInt32 idleConnections{0};
        TInt64 totalQueries{0};
        TDouble avgQueryTimeMs{0.0};
    };
    [[nodiscard]] SConnectionStats GetConnectionStats() const;

    // Vacuum database (optimize)
    CResult Vacuum(bool in_bFull = false);

    // Analyze tables (update statistics)
    CResult Analyze();

    // Create indexes for performance
    CResult CreateIndexes();

private:
    // Connection
    PGconn* m_pConnection;
    SPostgreSqlConfig m_config;
    mutable std::mutex m_mutex;
    bool m_bInTransaction;

    // Statistics
    TInt64 m_queryCount;
    TDouble m_totalQueryTimeMs;

    // Helper methods
    CResult ExecuteQuery(const TString& in_strQuery);
    CResult ExecuteQueryWithResult(const TString& in_strQuery, PGresult** out_pResult);
    CResult PrepareStatement(const TString& in_strName, const TString& in_strQuery);

    TString BuildConnectionString() const;
    TString EscapeString(const TString& in_str) const;

    CResult ParseTestData(PGresult* in_pResult, int in_row, STestDataRecord& out_data);

    bool CheckConnection();
    CResult Reconnect();
};

} // namespace TestMATE
