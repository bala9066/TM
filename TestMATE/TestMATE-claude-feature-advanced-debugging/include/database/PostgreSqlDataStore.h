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
#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
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
     * Connection Management (IDataStore interface)
     **************************************************************************/

    CResult Connect(const TString& in_strConnectionString) override;
    CResult Disconnect() override;
    [[nodiscard]] bool IsConnected() const override;
    CResult InitializeSchema() override;

    /**************************************************************************
     * Data Operations (IDataStore interface)
     **************************************************************************/

    CResult InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) override;
    CResult GetTestData(TUInt64 in_recordId, STestDataRecord& out_record) override;
    CResult UpdateTestData(const STestDataRecord& in_record) override;
    CResult DeleteTestData(TUInt64 in_recordId) override;

    /**************************************************************************
     * Query Operations (IDataStore interface)
     **************************************************************************/

    SQueryResult Query(const TString& in_strQuery) override;

    CResult GetTestDataByLot(const TString& in_strLotId,
                            TVector<STestDataRecord>& out_records) override;

    CResult GetTestDataByDateRange(const TTimePoint& in_start,
                                   const TTimePoint& in_end,
                                   TVector<STestDataRecord>& out_records) override;

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
