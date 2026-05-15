/**************************************************************************
 * File Name: SqliteDataStore.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: SQLite-based data store implementation
 * Requirements: REQ-DB-001 to REQ-DB-020
 **************************************************************************/

#pragma once

#include "IDataStore.h"
#include <memory>
#include <mutex>
#include <variant>

namespace TestMATE {

/**************************************************************************
 * Struct: SQueryFilter
 * Description: Filter for advanced test data queries
 **************************************************************************/
struct SQueryFilter {
    TString lotId;
    TString sequenceName;
    TUInt32 limit{0};
};

/**************************************************************************
 * Class: CSqliteDataStore
 * Description: SQLite database implementation for test data persistence
 **************************************************************************/
class CSqliteDataStore : public IDataStore {
public:
    CSqliteDataStore() = default;
    ~CSqliteDataStore() override;

    // IDataStore interface
    // IDataStore interface
    [[nodiscard]] CResult Connect(const TString& in_strConnectionString) override;
    [[nodiscard]] CResult Disconnect() override;
    [[nodiscard]] bool IsConnected() const override { return m_bConnected; }
    [[nodiscard]] CResult InitializeSchema() override;

    [[nodiscard]] CResult InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) override;
    [[nodiscard]] CResult UpdateTestData(const STestDataRecord& in_record) override;
    [[nodiscard]] CResult DeleteTestData(TUInt64 in_recordId) override;
    [[nodiscard]] CResult GetTestData(TUInt64 in_recordId, STestDataRecord& out_record) override;

    // SECURITY: Query() accepts raw SQL - use only with trusted/validated input
    // For dynamic queries with user input, use QueryParameterized() instead
    [[nodiscard]] SQueryResult Query(const TString& in_strQuery) override;

    // Safe parameterized query (prevents SQL injection)
    using TQueryParameter = std::variant<TString, TInt64, TDouble>;
    [[nodiscard]] SQueryResult QueryParameterized(const TString& in_strQuery,
                                                    const TVector<TQueryParameter>& in_parameters);

    [[nodiscard]] CResult GetTestDataByLot(const TString& in_strLotId, TVector<STestDataRecord>& out_records) override;
    [[nodiscard]] CResult GetTestDataByDateRange(const TWallClock& in_start, const TWallClock& in_end, TVector<STestDataRecord>& out_records) override;

    [[nodiscard]] CResult BeginTransaction() override;
    [[nodiscard]] CResult CommitTransaction() override;
    [[nodiscard]] CResult RollbackTransaction() override;

    // Additional utility methods
    [[nodiscard]] CResult QueryTestData(const SQueryFilter& in_filter, TVector<STestDataRecord>& out_records);
    [[nodiscard]] TUInt64 GetRecordCount() const { return m_recordCount; }

private:
    CResult CreateTables();
    CResult ExecuteSQL(const TString& in_strSql);

    void* m_pDatabase{nullptr};  // sqlite3* handle
    bool m_bConnected{false};
    TUInt64 m_recordCount{0};
    TString m_strDbPath;
    mutable std::mutex m_dbMutex;  // Thread safety for database operations
};

} // namespace TestMATE
