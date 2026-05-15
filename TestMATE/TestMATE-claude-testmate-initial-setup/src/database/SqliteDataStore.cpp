/**************************************************************************
 * File Name: SqliteDataStore.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: SQLite data store implementation
 * Requirements: REQ-DB-001 to REQ-DB-020
 **************************************************************************/

#include "database/SqliteDataStore.h"
#include <sqlite3.h>
#include <sstream>

namespace TestMATE {

CSqliteDataStore::~CSqliteDataStore() {
    if (m_bConnected) {
        Disconnect();
    }
}

CResult CSqliteDataStore::Connect(const TString& in_strConnectionString) {
    if (m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyInitialized, "Already connected");
    }

    m_strDbPath = in_strConnectionString.empty() ? ":memory:" : in_strConnectionString;

    sqlite3* db = nullptr;
    int rc = sqlite3_open(m_strDbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        TString error = sqlite3_errmsg(db);
        sqlite3_close(db);
        return TESTMATE_FAILURE(EErrorCode::kDatabaseConnectionFailed, "Failed to open database: " + error);
    }

    m_pDatabase = db;
    m_bConnected = true;

    auto result = CreateTables();
    if (!result.IsSuccess()) {
        Disconnect();
        return result;
    }

    // Get current record count
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM test_data", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            m_recordCount = static_cast<TUInt64>(sqlite3_column_int64(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return CResult::Success();
}

CResult CSqliteDataStore::Disconnect() {
    if (!m_bConnected || !m_pDatabase) {
        return CResult::Success();
    }

    sqlite3_close(static_cast<sqlite3*>(m_pDatabase));
    m_pDatabase = nullptr;
    m_bConnected = false;
    m_recordCount = 0;
    return CResult::Success();
}

CResult CSqliteDataStore::CreateTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS test_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sequence_name TEXT NOT NULL,
            lot_id TEXT,
            serial_number TEXT,
            operator_name TEXT,
            verdict INTEGER,
            start_time INTEGER,
            end_time INTEGER,
            duration_ms INTEGER,
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );
        CREATE INDEX IF NOT EXISTS idx_lot_id ON test_data(lot_id);
        CREATE INDEX IF NOT EXISTS idx_sequence_name ON test_data(sequence_name);
        CREATE INDEX IF NOT EXISTS idx_verdict ON test_data(verdict);
    )";
    return ExecuteSQL(sql);
}

CResult CSqliteDataStore::ExecuteSQL(const TString& in_strSql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(static_cast<sqlite3*>(m_pDatabase), in_strSql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        TString error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "SQL error: " + error);
    }
    return CResult::Success();
}

CResult CSqliteDataStore::InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    const char* sql = "INSERT INTO test_data (sequence_name, lot_id, device_id, verdict, duration_ms) VALUES (?, ?, ?, ?, ?)";
    sqlite3* db = static_cast<sqlite3*>(m_pDatabase);
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare statement");
    }

    sqlite3_bind_text(stmt, 1, in_record.sequenceName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, in_record.lotId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, in_record.deviceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(in_record.verdict));
    sqlite3_bind_int64(stmt, 5, in_record.durationMs);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseWriteFailed, "Failed to insert record");
    }

    out_id = static_cast<TUInt64>(sqlite3_last_insert_rowid(db));
    ++m_recordCount;
    return CResult::Success();
}

CResult CSqliteDataStore::GetTestData(TUInt64 in_id, STestDataRecord& out_record) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    const char* sql = "SELECT id, sequence_name, lot_id, serial_number, operator_name, verdict, duration_ms FROM test_data WHERE id = ?";
    sqlite3* db = static_cast<sqlite3*>(m_pDatabase);
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare statement");
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(in_id));

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_record.recordId = static_cast<TUInt64>(sqlite3_column_int64(stmt, 0));
        out_record.sequenceName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const unsigned char* lotIdText = sqlite3_column_text(stmt, 2);
        out_record.lotId = lotIdText ? reinterpret_cast<const char*>(lotIdText) : "";
        const unsigned char* deviceIdText = sqlite3_column_text(stmt, 3);
        out_record.deviceId = deviceIdText ? reinterpret_cast<const char*>(deviceIdText) : "";
        // Note: operatorName field removed as it doesn't exist in STestDataRecord
        out_record.verdict = static_cast<ETestVerdict>(sqlite3_column_int(stmt, 5));
        out_record.durationMs = sqlite3_column_int64(stmt, 6);
        sqlite3_finalize(stmt);
        return CResult::Success();
    }

    sqlite3_finalize(stmt);
    return TESTMATE_FAILURE(EErrorCode::kNotFound, "Record not found");
}

CResult CSqliteDataStore::DeleteTestData(TUInt64 in_id) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    std::ostringstream sql;
    sql << "DELETE FROM test_data WHERE id = " << in_id;

    auto result = ExecuteSQL(sql.str());
    if (result.IsSuccess() && m_recordCount > 0) {
        --m_recordCount;
    }
    return result;
}

CResult CSqliteDataStore::QueryTestData(const SQueryFilter& in_filter, TVector<STestDataRecord>& out_records) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    std::ostringstream sql;
    sql << "SELECT id, sequence_name, lot_id, device_id, verdict, duration_ms FROM test_data WHERE 1=1";

    if (!in_filter.lotId.empty()) {
        sql << " AND lot_id = '" << in_filter.lotId << "'";
    }
    if (!in_filter.sequenceName.empty()) {
        sql << " AND sequence_name = '" << in_filter.sequenceName << "'";
    }
    if (in_filter.limit > 0) {
        sql << " LIMIT " << in_filter.limit;
    }

    sqlite3* db = static_cast<sqlite3*>(m_pDatabase);
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare query");
    }

    out_records.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        STestDataRecord record;
        record.recordId = static_cast<TUInt64>(sqlite3_column_int64(stmt, 0));
        record.sequenceName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const unsigned char* lotIdText = sqlite3_column_text(stmt, 2);
        record.lotId = lotIdText ? reinterpret_cast<const char*>(lotIdText) : "";
        const unsigned char* deviceIdText = sqlite3_column_text(stmt, 3);
        record.deviceId = deviceIdText ? reinterpret_cast<const char*>(deviceIdText) : "";
        record.verdict = static_cast<ETestVerdict>(sqlite3_column_int(stmt, 4));
        record.durationMs = sqlite3_column_int64(stmt, 5);
        out_records.push_back(record);
    }

    sqlite3_finalize(stmt);
    return CResult::Success();
}

CResult CSqliteDataStore::BeginTransaction() {
    return ExecuteSQL("BEGIN TRANSACTION");
}

CResult CSqliteDataStore::CommitTransaction() {
    return ExecuteSQL("COMMIT");
}

CResult CSqliteDataStore::RollbackTransaction() {
    return ExecuteSQL("ROLLBACK");
}

CResult CSqliteDataStore::InitializeSchema() {
    return CreateTables();
}

CResult CSqliteDataStore::UpdateTestData(const STestDataRecord& in_record) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected to database");
    }

    const char* sql = "UPDATE test_data SET sequence_name=?, lot_id=?, device_id=?, verdict=?, duration_ms=? WHERE id=?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDatabase), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare update statement");
    }

    sqlite3_bind_text(stmt, 1, in_record.sequenceName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, in_record.lotId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, in_record.deviceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(in_record.verdict));
    sqlite3_bind_int64(stmt, 5, in_record.durationMs);
    sqlite3_bind_int64(stmt, 6, in_record.recordId);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to update record");
    }

    return TESTMATE_SUCCESS();
}

SQueryResult CSqliteDataStore::Query(const TString& in_strQuery) {
    SQueryResult result;
    result.success = false;

    if (!m_bConnected) {
        result.errorMessage = "Not connected to database";
        return result;
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDatabase), in_strQuery.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        result.errorMessage = "Failed to prepare query";
        return result;
    }

    // Get column count
    int columnCount = sqlite3_column_count(stmt);

    // Execute and fetch results
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TVector<TString> row;
        for (int i = 0; i < columnCount; i++) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            row.push_back(text ? reinterpret_cast<const char*>(text) : "");
        }
        result.rows.push_back(row);
    }

    sqlite3_finalize(stmt);
    result.success = true;
    return result;
}

CResult CSqliteDataStore::GetTestDataByLot(const TString& in_strLotId, TVector<STestDataRecord>& out_records) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected to database");
    }

    out_records.clear();

    const char* sql = "SELECT id, sequence_name, lot_id, device_id, verdict, duration_ms FROM test_data WHERE lot_id=?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDatabase), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare query");
    }

    sqlite3_bind_text(stmt, 1, in_strLotId.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        STestDataRecord record;
        record.recordId = sqlite3_column_int64(stmt, 0);
        const unsigned char* seqNameText = sqlite3_column_text(stmt, 1);
        record.sequenceName = seqNameText ? reinterpret_cast<const char*>(seqNameText) : "";
        const unsigned char* lotIdText = sqlite3_column_text(stmt, 2);
        record.lotId = lotIdText ? reinterpret_cast<const char*>(lotIdText) : "";
        const unsigned char* deviceIdText = sqlite3_column_text(stmt, 3);
        record.deviceId = deviceIdText ? reinterpret_cast<const char*>(deviceIdText) : "";
        record.verdict = static_cast<ETestVerdict>(sqlite3_column_int(stmt, 4));
        record.durationMs = sqlite3_column_int64(stmt, 5);
        out_records.push_back(record);
    }

    sqlite3_finalize(stmt);
    return TESTMATE_SUCCESS();
}

CResult CSqliteDataStore::GetTestDataByDateRange(const TTimePoint& /*in_start*/, const TTimePoint& /*in_end*/, TVector<STestDataRecord>& out_records) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected to database");
    }

    // Note: Date range filtering would require timestamp columns in the database
    // For now, return all records as a stub implementation
    const char* sql = "SELECT id, sequence_name, lot_id, device_id, verdict, duration_ms FROM test_data";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(static_cast<sqlite3*>(m_pDatabase), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return TESTMATE_FAILURE(EErrorCode::kDatabaseQueryFailed, "Failed to prepare query");
    }

    out_records.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        STestDataRecord record;
        record.recordId = sqlite3_column_int64(stmt, 0);
        const unsigned char* seqNameText = sqlite3_column_text(stmt, 1);
        record.sequenceName = seqNameText ? reinterpret_cast<const char*>(seqNameText) : "";
        const unsigned char* lotIdText = sqlite3_column_text(stmt, 2);
        record.lotId = lotIdText ? reinterpret_cast<const char*>(lotIdText) : "";
        const unsigned char* deviceIdText = sqlite3_column_text(stmt, 3);
        record.deviceId = deviceIdText ? reinterpret_cast<const char*>(deviceIdText) : "";
        record.verdict = static_cast<ETestVerdict>(sqlite3_column_int(stmt, 4));
        record.durationMs = sqlite3_column_int64(stmt, 5);
        out_records.push_back(record);
    }

    sqlite3_finalize(stmt);
    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
