/**************************************************************************
 * File Name: PostgreSqlDataStore.cpp
 * Description: PostgreSQL data store implementation
 **************************************************************************/

#include "database/PostgreSqlDataStore.h"
#include "utils/LogManager.h"
#include <sstream>
#include <iomanip>
#include <ctime>

// PostgreSQL C API (libpq)
#ifdef TESTMATE_POSTGRESQL_SUPPORT
#include <libpq-fe.h>
#else
// Stub types when PostgreSQL not available
typedef void PGconn;
typedef void PGresult;
#endif

namespace TestMATE {

/**************************************************************************
 * Constructor / Destructor
 **************************************************************************/

CPostgreSqlDataStore::CPostgreSqlDataStore()
    : m_pConnection(nullptr)
    , m_bInTransaction(false)
    , m_queryCount(0)
    , m_totalQueryTimeMs(0.0)
{
}

CPostgreSqlDataStore::~CPostgreSqlDataStore() {
    Close();
}

/**************************************************************************
 * Connection Management
 **************************************************************************/

CResult CPostgreSqlDataStore::Open(const SPostgreSqlConfig& config) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented,
                        "PostgreSQL support not compiled in. "
                        "Rebuild with -DTESTMATE_POSTGRESQL_SUPPORT=ON");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pConnection) {
        return TESTMATE_ERROR(EErrorCode::kAlreadyConnected,
                            "Already connected to database");
    }

    m_config = config;

    // Build connection string
    TString connStr = BuildConnectionString();

    // Connect to database
    m_pConnection = PQconnectdb(connStr.c_str());

    if (!m_pConnection) {
        return TESTMATE_ERROR(EErrorCode::kDatabaseConnectionFailed,
                            "Failed to allocate connection");
    }

    // Check connection status
    if (PQstatus(m_pConnection) != CONNECTION_OK) {
        TString error = PQerrorMessage(m_pConnection);
        PQfinish(m_pConnection);
        m_pConnection = nullptr;
        return TESTMATE_ERROR(EErrorCode::kDatabaseConnectionFailed,
                            "Connection failed: " + error);
    }

    // Set client encoding to UTF8
    PQexec(m_pConnection, "SET client_encoding = 'UTF8'");

    // Set schema if specified
    if (!m_config.schema.empty() && m_config.schema != "public") {
        TString setSchema = "SET search_path TO " + m_config.schema + ",public";
        auto result = ExecuteQuery(setSchema);
        if (!result.IsSuccess()) {
            LOG_WARNING("Failed to set schema: " + result.GetMessage());
        }
    }

    LOG_INFO("Connected to PostgreSQL database: " + m_config.database);
    return TESTMATE_SUCCESS();
#endif
}

CResult CPostgreSqlDataStore::Open(const TString& in_strConnectionString) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented,
                        "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pConnection) {
        return TESTMATE_ERROR(EErrorCode::kAlreadyConnected,
                            "Already connected to database");
    }

    m_pConnection = PQconnectdb(in_strConnectionString.c_str());

    if (!m_pConnection || PQstatus(m_pConnection) != CONNECTION_OK) {
        TString error = m_pConnection ? PQerrorMessage(m_pConnection) : "Connection failed";
        if (m_pConnection) PQfinish(m_pConnection);
        m_pConnection = nullptr;
        return TESTMATE_ERROR(EErrorCode::kDatabaseConnectionFailed, error);
    }

    LOG_INFO("Connected to PostgreSQL database");
    return TESTMATE_SUCCESS();
#endif
}

CResult CPostgreSqlDataStore::Close() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pConnection) {
        if (m_bInTransaction) {
            RollbackTransaction();
        }
        PQfinish(m_pConnection);
        m_pConnection = nullptr;
        LOG_INFO("Disconnected from PostgreSQL database");
    }
#endif
    return TESTMATE_SUCCESS();
}

bool CPostgreSqlDataStore::IsOpen() const {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pConnection && PQstatus(m_pConnection) == CONNECTION_OK;
#else
    return false;
#endif
}

/**************************************************************************
 * Data Operations
 **************************************************************************/

CResult CPostgreSqlDataStore::SaveTestData(const STestDataRecord& in_data) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    // Build INSERT query
    std::ostringstream query;
    query << "INSERT INTO test_results (test_id, sequence_id, device_id, lot_id, "
          << "timestamp, verdict, duration_ms, operator_name) VALUES ("
          << "'" << EscapeString(in_data.testId) << "', "
          << "'" << EscapeString(in_data.sequenceId) << "', "
          << "'" << EscapeString(in_data.deviceId) << "', "
          << "'" << EscapeString(in_data.lotId) << "', "
          << "NOW(), "  // Use PostgreSQL's NOW() function
          << static_cast<int>(in_data.verdict) << ", "
          << in_data.durationMs << ", "
          << "'" << EscapeString(in_data.operatorName) << "')";

    auto result = ExecuteQuery(query.str());
    if (!result.IsSuccess()) {
        return result;
    }

    // TODO: Save measurements and step results in separate tables

    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

CResult CPostgreSqlDataStore::GetTestData(const TString& in_strTestId,
                                         STestDataRecord& out_data) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE test_id = '"
          << EscapeString(in_strTestId) << "'";

    PGresult* pgResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &pgResult);
    if (!result.IsSuccess()) {
        return result;
    }

    if (PQntuples(pgResult) == 0) {
        PQclear(pgResult);
        return TESTMATE_ERROR(EErrorCode::kNotFound,
                            "Test ID not found: " + in_strTestId);
    }

    result = ParseTestData(pgResult, 0, out_data);
    PQclear(pgResult);

    m_queryCount++;
    return result;
#endif
}

CResult CPostgreSqlDataStore::UpdateTestData(const STestDataRecord& in_data) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "UPDATE test_results SET "
          << "sequence_id = '" << EscapeString(in_data.sequenceId) << "', "
          << "device_id = '" << EscapeString(in_data.deviceId) << "', "
          << "lot_id = '" << EscapeString(in_data.lotId) << "', "
          << "verdict = " << static_cast<int>(in_data.verdict) << ", "
          << "duration_ms = " << in_data.durationMs << ", "
          << "operator_name = '" << EscapeString(in_data.operatorName) << "' "
          << "WHERE test_id = '" << EscapeString(in_data.testId) << "'";

    auto result = ExecuteQuery(query.str());
    m_queryCount++;
    return result;
#endif
}

CResult CPostgreSqlDataStore::DeleteTestData(const TString& in_strTestId) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "DELETE FROM test_results WHERE test_id = '"
          << EscapeString(in_strTestId) << "'";

    auto result = ExecuteQuery(query.str());
    m_queryCount++;
    return result;
#endif
}

/**************************************************************************
 * Query Operations
 **************************************************************************/

CResult CPostgreSqlDataStore::GetTestDataByLot(const TString& in_strLotId,
                                              std::vector<STestDataRecord>& out_data) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE lot_id = '"
          << EscapeString(in_strLotId) << "' ORDER BY timestamp DESC";

    PGresult* pgResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &pgResult);
    if (!result.IsSuccess()) {
        return result;
    }

    int rowCount = PQntuples(pgResult);
    out_data.clear();
    out_data.reserve(rowCount);

    for (int i = 0; i < rowCount; ++i) {
        STestDataRecord record;
        auto parseResult = ParseTestData(pgResult, i, record);
        if (parseResult.IsSuccess()) {
            out_data.push_back(record);
        }
    }

    PQclear(pgResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

CResult CPostgreSqlDataStore::GetTestDataByDateRange(const TString& in_strStartDate,
                                                     const TString& in_strEndDate,
                                                     std::vector<STestDataRecord>& out_data) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE timestamp BETWEEN '"
          << EscapeString(in_strStartDate) << "' AND '"
          << EscapeString(in_strEndDate) << "' ORDER BY timestamp DESC";

    PGresult* pgResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &pgResult);
    if (!result.IsSuccess()) {
        return result;
    }

    int rowCount = PQntuples(pgResult);
    out_data.clear();
    out_data.reserve(rowCount);

    for (int i = 0; i < rowCount; ++i) {
        STestDataRecord record;
        auto parseResult = ParseTestData(pgResult, i, record);
        if (parseResult.IsSuccess()) {
            out_data.push_back(record);
        }
    }

    PQclear(pgResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

CResult CPostgreSqlDataStore::Query(const TString& in_strQuery, SQueryResult& out_result) {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    PGresult* pgResult = nullptr;
    auto result = ExecuteQueryWithResult(in_strQuery, &pgResult);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse results
    int rowCount = PQntuples(pgResult);
    int colCount = PQnfields(pgResult);

    out_result.rows.clear();
    out_result.rows.reserve(rowCount);

    for (int row = 0; row < rowCount; ++row) {
        std::map<TString, TString> rowData;
        for (int col = 0; col < colCount; ++col) {
            TString colName = PQfname(pgResult, col);
            TString value = PQgetvalue(pgResult, row, col);
            rowData[colName] = value;
        }
        out_result.rows.push_back(rowData);
    }

    PQclear(pgResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

/**************************************************************************
 * Transaction Support
 **************************************************************************/

CResult CPostgreSqlDataStore::BeginTransaction() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState,
                            "Transaction already in progress");
    }

    auto result = ExecuteQuery("BEGIN");
    if (result.IsSuccess()) {
        m_bInTransaction = true;
    }
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::CommitTransaction() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState, "No transaction in progress");
    }

    auto result = ExecuteQuery("COMMIT");
    m_bInTransaction = false;
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::RollbackTransaction() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState, "No transaction in progress");
    }

    auto result = ExecuteQuery("ROLLBACK");
    m_bInTransaction = false;
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::CreateSavepoint(const TString& in_strName) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("SAVEPOINT " + in_strName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::RollbackToSavepoint(const TString& in_strName) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("ROLLBACK TO SAVEPOINT " + in_strName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::ReleaseSavepoint(const TString& in_strName) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("RELEASE SAVEPOINT " + in_strName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

/**************************************************************************
 * Schema Management
 **************************************************************************/

CResult CPostgreSqlDataStore::InitializeSchema() {
#ifndef TESTMATE_POSTGRESQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    // Create tables
    TString schema = R"(
        CREATE TABLE IF NOT EXISTS test_results (
            test_id VARCHAR(100) PRIMARY KEY,
            sequence_id VARCHAR(100),
            device_id VARCHAR(100),
            lot_id VARCHAR(100),
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            verdict INTEGER,
            duration_ms BIGINT,
            operator_name VARCHAR(100)
        );

        CREATE TABLE IF NOT EXISTS measurements (
            id SERIAL PRIMARY KEY,
            test_id VARCHAR(100) REFERENCES test_results(test_id) ON DELETE CASCADE,
            name VARCHAR(100),
            value DOUBLE PRECISION,
            unit VARCHAR(50),
            min_limit DOUBLE PRECISION,
            max_limit DOUBLE PRECISION
        );

        CREATE TABLE IF NOT EXISTS step_results (
            id SERIAL PRIMARY KEY,
            test_id VARCHAR(100) REFERENCES test_results(test_id) ON DELETE CASCADE,
            step_id VARCHAR(100),
            step_name VARCHAR(200),
            verdict INTEGER,
            duration_ms BIGINT,
            message TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_lot_id ON test_results(lot_id);
        CREATE INDEX IF NOT EXISTS idx_device_id ON test_results(device_id);
        CREATE INDEX IF NOT EXISTS idx_timestamp ON test_results(timestamp);
        CREATE INDEX IF NOT EXISTS idx_test_id_measurements ON measurements(test_id);
        CREATE INDEX IF NOT EXISTS idx_test_id_step_results ON step_results(test_id);
    )";

    auto result = ExecuteQuery(schema);
    if (result.IsSuccess()) {
        LOG_INFO("PostgreSQL schema initialized successfully");
    }
    return result;
#endif
}

bool CPostgreSqlDataStore::SchemaExists() const {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    // Check if test_results table exists
    const char* query = "SELECT EXISTS (SELECT FROM information_schema.tables "
                       "WHERE table_name = 'test_results')";
    PGresult* result = PQexec(m_pConnection, query);
    if (result && PQresultStatus(result) == PGRES_TUPLES_OK) {
        bool exists = (strcmp(PQgetvalue(result, 0, 0), "t") == 0);
        PQclear(result);
        return exists;
    }
    if (result) PQclear(result);
#endif
    return false;
}

TInt32 CPostgreSqlDataStore::GetSchemaVersion() const {
    // TODO: Implement schema versioning table
    return 1;
}

CResult CPostgreSqlDataStore::MigrateSchema() {
    // TODO: Implement schema migration logic
    return TESTMATE_SUCCESS();
}

/**************************************************************************
 * PostgreSQL-Specific Features
 **************************************************************************/

CPostgreSqlDataStore::SConnectionStats CPostgreSqlDataStore::GetConnectionStats() const {
    SConnectionStats stats;
    stats.totalQueries = m_queryCount;
    if (m_queryCount > 0) {
        stats.avgQueryTimeMs = m_totalQueryTimeMs / m_queryCount;
    }
    return stats;
}

CResult CPostgreSqlDataStore::Vacuum(bool in_bFull) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    TString query = in_bFull ? "VACUUM FULL" : "VACUUM";
    return ExecuteQuery(query);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::Analyze() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("ANALYZE");
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::CreateIndexes() {
    return InitializeSchema();  // Indexes are created in schema
}

/**************************************************************************
 * Private Helper Methods
 **************************************************************************/

CResult CPostgreSqlDataStore::ExecuteQuery(const TString& in_strQuery) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    auto start = std::chrono::steady_clock::now();

    PGresult* result = PQexec(m_pConnection, in_strQuery.c_str());
    ExecStatusType status = PQresultStatus(result);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_totalQueryTimeMs += duration.count() / 1000.0;

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        TString error = PQerrorMessage(m_pConnection);
        PQclear(result);
        return TESTMATE_ERROR(EErrorCode::kDatabaseQueryFailed,
                            "Query failed: " + error);
    }

    PQclear(result);
    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

CResult CPostgreSqlDataStore::ExecuteQueryWithResult(const TString& in_strQuery,
                                                     PGresult** out_pResult) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    auto start = std::chrono::steady_clock::now();

    *out_pResult = PQexec(m_pConnection, in_strQuery.c_str());
    ExecStatusType status = PQresultStatus(*out_pResult);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_totalQueryTimeMs += duration.count() / 1000.0;

    if (status != PGRES_TUPLES_OK) {
        TString error = PQerrorMessage(m_pConnection);
        PQclear(*out_pResult);
        *out_pResult = nullptr;
        return TESTMATE_ERROR(EErrorCode::kDatabaseQueryFailed,
                            "Query failed: " + error);
    }

    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

TString CPostgreSqlDataStore::BuildConnectionString() const {
    std::ostringstream conn;
    conn << "host=" << m_config.host
         << " port=" << m_config.port
         << " dbname=" << m_config.database
         << " user=" << m_config.user;

    if (!m_config.password.empty()) {
        conn << " password=" << m_config.password;
    }

    if (m_config.useSSL) {
        conn << " sslmode=require";
    }

    conn << " connect_timeout=" << m_config.connectionTimeout;

    return conn.str();
}

TString CPostgreSqlDataStore::EscapeString(const TString& in_str) const {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    if (!m_pConnection) return in_str;

    char* escaped = new char[in_str.length() * 2 + 1];
    PQescapeStringConn(m_pConnection, escaped, in_str.c_str(), in_str.length(), nullptr);
    TString result(escaped);
    delete[] escaped;
    return result;
#else
    return in_str;
#endif
}

CResult CPostgreSqlDataStore::ParseTestData(PGresult* in_pResult, int in_row,
                                           STestDataRecord& out_data) {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    out_data.testId = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "test_id"));
    out_data.sequenceId = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "sequence_id"));
    out_data.deviceId = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "device_id"));
    out_data.lotId = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "lot_id"));
    out_data.operatorName = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "operator_name"));

    // Parse verdict
    TString verdictStr = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "verdict"));
    out_data.verdict = static_cast<ETestVerdict>(std::stoi(verdictStr));

    // Parse duration
    TString durationStr = PQgetvalue(in_pResult, in_row, PQfnumber(in_pResult, "duration_ms"));
    out_data.durationMs = std::stoll(durationStr);

    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

bool CPostgreSqlDataStore::CheckConnection() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    if (!m_pConnection) return false;
    if (PQstatus(m_pConnection) != CONNECTION_OK) {
        LOG_WARNING("Lost connection to PostgreSQL, attempting reconnect...");
        return Reconnect().IsSuccess();
    }
    return true;
#else
    return false;
#endif
}

CResult CPostgreSqlDataStore::Reconnect() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    Close();
    return Open(m_config);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "PostgreSQL support not enabled");
#endif
}

} // namespace TestMATE
