/**************************************************************************
 * File Name: MySqlDataStore.cpp
 * Description: MySQL/MariaDB data store implementation
 **************************************************************************/

#include "database/MySqlDataStore.h"
#include "utils/LogManager.h"
#include <sstream>
#include <chrono>

// MySQL C API
#ifdef TESTMATE_MYSQL_SUPPORT
#include <mysql/mysql.h>
#else
// Stub types when MySQL not available
typedef void MYSQL;
typedef void MYSQL_RES;
typedef void MYSQL_STMT;
typedef char** MYSQL_ROW;
#endif

namespace TestMATE {

/**************************************************************************
 * Constructor / Destructor
 **************************************************************************/

CMySqlDataStore::CMySqlDataStore()
    : m_pConnection(nullptr)
    , m_bInTransaction(false)
    , m_queryCount(0)
    , m_connectionCount(0)
    , m_totalQueryTimeMs(0.0)
    , m_rowsAffected(0)
{
#ifdef TESTMATE_MYSQL_SUPPORT
    mysql_library_init(0, nullptr, nullptr);
#endif
}

CMySqlDataStore::~CMySqlDataStore() {
    Close();
#ifdef TESTMATE_MYSQL_SUPPORT
    mysql_library_end();
#endif
}

/**************************************************************************
 * Connection Management
 **************************************************************************/

CResult CMySqlDataStore::Open(const SMySqlConfig& config) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented,
                        "MySQL support not compiled in. "
                        "Rebuild with -DTESTMATE_MYSQL_SUPPORT=ON");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pConnection) {
        return TESTMATE_ERROR(EErrorCode::kAlreadyConnected,
                            "Already connected to database");
    }

    m_config = config;

    // Initialize MySQL connection
    m_pConnection = mysql_init(nullptr);
    if (!m_pConnection) {
        return TESTMATE_ERROR(EErrorCode::kDatabaseConnectionFailed,
                            "Failed to initialize MySQL connection");
    }

    // Set options
    if (m_config.autoReconnect) {
        my_bool reconnect = 1;
        mysql_options(m_pConnection, MYSQL_OPT_RECONNECT, &reconnect);
    }

    unsigned int timeout = m_config.connectionTimeout;
    mysql_options(m_pConnection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // Set charset
    mysql_options(m_pConnection, MYSQL_SET_CHARSET_NAME, m_config.charset.c_str());

    // Configure SSL if requested
    if (m_config.useSSL) {
        SetSSLOptions();
    }

    // Connect to database
    if (!mysql_real_connect(m_pConnection,
                           m_config.host.c_str(),
                           m_config.user.c_str(),
                           m_config.password.c_str(),
                           m_config.database.c_str(),
                           m_config.port,
                           nullptr,
                           CLIENT_MULTI_STATEMENTS)) {
        TString error = mysql_error(m_pConnection);
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
        return TESTMATE_ERROR(EErrorCode::kDatabaseConnectionFailed,
                            "Connection failed: " + error);
    }

    m_connectionCount++;
    LOG_INFO("Connected to MySQL database: " + m_config.database);
    return TESTMATE_SUCCESS();
#endif
}

CResult CMySqlDataStore::Open(const TString& in_strConnectionString) {
    // Parse connection string and extract parameters
    // Format: "host=localhost;port=3306;database=testmate;user=root;password=pass"
    SMySqlConfig config;

    // Simple parser (in production, use more robust parsing)
    // For now, use default config

    return Open(config);
}

CResult CMySqlDataStore::Close() {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pConnection) {
        if (m_bInTransaction) {
            RollbackTransaction();
        }
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
        LOG_INFO("Disconnected from MySQL database");
    }
#endif
    return TESTMATE_SUCCESS();
}

bool CMySqlDataStore::IsOpen() const {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pConnection && mysql_ping(m_pConnection) == 0;
#else
    return false;
#endif
}

/**************************************************************************
 * Data Operations
 **************************************************************************/

CResult CMySqlDataStore::SaveTestData(const STestDataRecord& in_data) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
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
          << "NOW(), "
          << static_cast<int>(in_data.verdict) << ", "
          << in_data.durationMs << ", "
          << "'" << EscapeString(in_data.operatorName) << "')";

    auto result = ExecuteQuery(query.str());
    if (result.IsSuccess()) {
        m_rowsAffected += mysql_affected_rows(m_pConnection);
    }

    m_queryCount++;
    return result;
#endif
}

CResult CMySqlDataStore::GetTestData(const TString& in_strTestId,
                                    STestDataRecord& out_data) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE test_id = '"
          << EscapeString(in_strTestId) << "'";

    MYSQL_RES* mysqlResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &mysqlResult);
    if (!result.IsSuccess()) {
        return result;
    }

    if (mysql_num_rows(mysqlResult) == 0) {
        mysql_free_result(mysqlResult);
        return TESTMATE_ERROR(EErrorCode::kNotFound,
                            "Test ID not found: " + in_strTestId);
    }

    result = ParseTestData(mysqlResult, out_data);
    mysql_free_result(mysqlResult);

    m_queryCount++;
    return result;
#endif
}

CResult CMySqlDataStore::UpdateTestData(const STestDataRecord& in_data) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
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
    if (result.IsSuccess()) {
        m_rowsAffected += mysql_affected_rows(m_pConnection);
    }

    m_queryCount++;
    return result;
#endif
}

CResult CMySqlDataStore::DeleteTestData(const TString& in_strTestId) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "DELETE FROM test_results WHERE test_id = '"
          << EscapeString(in_strTestId) << "'";

    auto result = ExecuteQuery(query.str());
    if (result.IsSuccess()) {
        m_rowsAffected += mysql_affected_rows(m_pConnection);
    }

    m_queryCount++;
    return result;
#endif
}

/**************************************************************************
 * Query Operations
 **************************************************************************/

CResult CMySqlDataStore::GetTestDataByLot(const TString& in_strLotId,
                                         std::vector<STestDataRecord>& out_data) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE lot_id = '"
          << EscapeString(in_strLotId) << "' ORDER BY timestamp DESC";

    MYSQL_RES* mysqlResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &mysqlResult);
    if (!result.IsSuccess()) {
        return result;
    }

    out_data.clear();
    while (MYSQL_ROW row = mysql_fetch_row(mysqlResult)) {
        STestDataRecord record;
        // Parse row data (simplified)
        record.testId = row[0] ? row[0] : "";
        record.sequenceId = row[1] ? row[1] : "";
        record.deviceId = row[2] ? row[2] : "";
        record.lotId = row[3] ? row[3] : "";
        // ... parse other fields ...
        out_data.push_back(record);
    }

    mysql_free_result(mysqlResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

CResult CMySqlDataStore::GetTestDataByDateRange(const TString& in_strStartDate,
                                               const TString& in_strEndDate,
                                               std::vector<STestDataRecord>& out_data) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    std::ostringstream query;
    query << "SELECT * FROM test_results WHERE timestamp BETWEEN '"
          << EscapeString(in_strStartDate) << "' AND '"
          << EscapeString(in_strEndDate) << "' ORDER BY timestamp DESC";

    MYSQL_RES* mysqlResult = nullptr;
    auto result = ExecuteQueryWithResult(query.str(), &mysqlResult);
    if (!result.IsSuccess()) {
        return result;
    }

    out_data.clear();
    while (MYSQL_ROW row = mysql_fetch_row(mysqlResult)) {
        STestDataRecord record;
        // Parse row (implementation similar to GetTestDataByLot)
        out_data.push_back(record);
    }

    mysql_free_result(mysqlResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

CResult CMySqlDataStore::Query(const TString& in_strQuery, SQueryResult& out_result) {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    MYSQL_RES* mysqlResult = nullptr;
    auto result = ExecuteQueryWithResult(in_strQuery, &mysqlResult);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse results
    int numFields = mysql_num_fields(mysqlResult);
    MYSQL_FIELD* fields = mysql_fetch_fields(mysqlResult);

    out_result.rows.clear();
    while (MYSQL_ROW row = mysql_fetch_row(mysqlResult)) {
        std::map<TString, TString> rowData;
        for (int i = 0; i < numFields; ++i) {
            TString fieldName = fields[i].name;
            TString value = row[i] ? row[i] : "";
            rowData[fieldName] = value;
        }
        out_result.rows.push_back(rowData);
    }

    mysql_free_result(mysqlResult);
    m_queryCount++;
    return TESTMATE_SUCCESS();
#endif
}

/**************************************************************************
 * Transaction Support
 **************************************************************************/

CResult CMySqlDataStore::BeginTransaction() {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState,
                            "Transaction already in progress");
    }

    auto result = ExecuteQuery("START TRANSACTION");
    if (result.IsSuccess()) {
        m_bInTransaction = true;
    }
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::CommitTransaction() {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState, "No transaction in progress");
    }

    auto result = ExecuteQuery("COMMIT");
    m_bInTransaction = false;
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::RollbackTransaction() {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInTransaction) {
        return TESTMATE_ERROR(EErrorCode::kInvalidState, "No transaction in progress");
    }

    auto result = ExecuteQuery("ROLLBACK");
    m_bInTransaction = false;
    return result;
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

/**************************************************************************
 * Schema Management
 **************************************************************************/

CResult CMySqlDataStore::InitializeSchema() {
#ifndef TESTMATE_MYSQL_SUPPORT
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#else
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!CheckConnection()) {
        return TESTMATE_ERROR(EErrorCode::kNotConnected, "Not connected to database");
    }

    // Create tables (MySQL/InnoDB engine)
    TString schema = R"(
        CREATE TABLE IF NOT EXISTS test_results (
            test_id VARCHAR(100) PRIMARY KEY,
            sequence_id VARCHAR(100),
            device_id VARCHAR(100),
            lot_id VARCHAR(100),
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            verdict INT,
            duration_ms BIGINT,
            operator_name VARCHAR(100),
            INDEX idx_lot_id (lot_id),
            INDEX idx_device_id (device_id),
            INDEX idx_timestamp (timestamp)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

        CREATE TABLE IF NOT EXISTS measurements (
            id INT AUTO_INCREMENT PRIMARY KEY,
            test_id VARCHAR(100),
            name VARCHAR(100),
            value DOUBLE,
            unit VARCHAR(50),
            min_limit DOUBLE,
            max_limit DOUBLE,
            INDEX idx_test_id (test_id),
            FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

        CREATE TABLE IF NOT EXISTS step_results (
            id INT AUTO_INCREMENT PRIMARY KEY,
            test_id VARCHAR(100),
            step_id VARCHAR(100),
            step_name VARCHAR(200),
            verdict INT,
            duration_ms BIGINT,
            message TEXT,
            INDEX idx_test_id (test_id),
            FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";

    auto result = ExecuteQuery(schema);
    if (result.IsSuccess()) {
        LOG_INFO("MySQL schema initialized successfully");
    }
    return result;
#endif
}

bool CMySqlDataStore::SchemaExists() const {
#ifdef TESTMATE_MYSQL_SUPPORT
    const char* query = "SHOW TABLES LIKE 'test_results'";
    MYSQL_RES* result = mysql_store_result(m_pConnection);
    if (result) {
        bool exists = (mysql_num_rows(result) > 0);
        mysql_free_result(result);
        return exists;
    }
#endif
    return false;
}

TInt32 CMySqlDataStore::GetSchemaVersion() const {
    // TODO: Implement schema versioning
    return 1;
}

/**************************************************************************
 * MySQL-Specific Features
 **************************************************************************/

CMySqlDataStore::SConnectionStats CMySqlDataStore::GetConnectionStats() const {
    SConnectionStats stats;
    stats.totalQueries = m_queryCount;
    stats.totalConnections = m_connectionCount;
    stats.rowsAffected = m_rowsAffected;
    if (m_queryCount > 0) {
        stats.avgQueryTimeMs = m_totalQueryTimeMs / m_queryCount;
    }
    return stats;
}

CResult CMySqlDataStore::OptimizeTable(const TString& in_strTableName) {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("OPTIMIZE TABLE " + in_strTableName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::AnalyzeTable(const TString& in_strTableName) {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("ANALYZE TABLE " + in_strTableName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::RepairTable(const TString& in_strTableName) {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("REPAIR TABLE " + in_strTableName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::CheckTable(const TString& in_strTableName) {
#ifdef TESTMATE_MYSQL_SUPPORT
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExecuteQuery("CHECK TABLE " + in_strTableName);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

TString CMySqlDataStore::GetServerVersion() const {
#ifdef TESTMATE_MYSQL_SUPPORT
    if (m_pConnection) {
        return mysql_get_server_info(m_pConnection);
    }
#endif
    return "Unknown";
}

TString CMySqlDataStore::GetServerStatus() const {
#ifdef TESTMATE_MYSQL_SUPPORT
    if (m_pConnection) {
        const char* status = mysql_stat(m_pConnection);
        return status ? status : "Unknown";
    }
#endif
    return "Not connected";
}

/**************************************************************************
 * Private Helper Methods
 **************************************************************************/

CResult CMySqlDataStore::ExecuteQuery(const TString& in_strQuery) {
#ifdef TESTMATE_MYSQL_SUPPORT
    auto start = std::chrono::steady_clock::now();

    int result = mysql_query(m_pConnection, in_strQuery.c_str());

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_totalQueryTimeMs += duration.count() / 1000.0;

    if (result != 0) {
        TString error = mysql_error(m_pConnection);
        return TESTMATE_ERROR(EErrorCode::kDatabaseQueryFailed,
                            "Query failed: " + error);
    }

    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

CResult CMySqlDataStore::ExecuteQueryWithResult(const TString& in_strQuery,
                                               MYSQL_RES** out_pResult) {
#ifdef TESTMATE_MYSQL_SUPPORT
    auto start = std::chrono::steady_clock::now();

    if (mysql_query(m_pConnection, in_strQuery.c_str()) != 0) {
        TString error = mysql_error(m_pConnection);
        return TESTMATE_ERROR(EErrorCode::kDatabaseQueryFailed,
                            "Query failed: " + error);
    }

    *out_pResult = mysql_store_result(m_pConnection);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_totalQueryTimeMs += duration.count() / 1000.0;

    if (!*out_pResult) {
        return TESTMATE_ERROR(EErrorCode::kDatabaseQueryFailed,
                            "Failed to get result set");
    }

    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

TString CMySqlDataStore::EscapeString(const TString& in_str) const {
#ifdef TESTMATE_MYSQL_SUPPORT
    if (!m_pConnection) return in_str;

    char* escaped = new char[in_str.length() * 2 + 1];
    mysql_real_escape_string(m_pConnection, escaped, in_str.c_str(), in_str.length());
    TString result(escaped);
    delete[] escaped;
    return result;
#else
    return in_str;
#endif
}

CResult CMySqlDataStore::ParseTestData(MYSQL_RES* in_pResult,
                                      STestDataRecord& out_data) {
#ifdef TESTMATE_MYSQL_SUPPORT
    MYSQL_ROW row = mysql_fetch_row(in_pResult);
    if (!row) {
        return TESTMATE_ERROR(EErrorCode::kNotFound, "No data to parse");
    }

    // Parse row data (column order from CREATE TABLE)
    out_data.testId = row[0] ? row[0] : "";
    out_data.sequenceId = row[1] ? row[1] : "";
    out_data.deviceId = row[2] ? row[2] : "";
    out_data.lotId = row[3] ? row[3] : "";
    // timestamp at row[4]
    out_data.verdict = row[5] ? static_cast<ETestVerdict>(std::stoi(row[5])) : ETestVerdict::kError;
    out_data.durationMs = row[6] ? std::stoll(row[6]) : 0;
    out_data.operatorName = row[7] ? row[7] : "";

    return TESTMATE_SUCCESS();
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

bool CMySqlDataStore::CheckConnection() {
#ifdef TESTMATE_MYSQL_SUPPORT
    if (!m_pConnection) return false;
    if (mysql_ping(m_pConnection) != 0) {
        if (m_config.autoReconnect) {
            LOG_WARNING("Lost connection to MySQL, attempting reconnect...");
            return Reconnect().IsSuccess();
        }
        return false;
    }
    return true;
#else
    return false;
#endif
}

CResult CMySqlDataStore::Reconnect() {
#ifdef TESTMATE_MYSQL_SUPPORT
    Close();
    return Open(m_config);
#else
    return TESTMATE_ERROR(EErrorCode::kNotImplemented, "MySQL support not enabled");
#endif
}

void CMySqlDataStore::SetSSLOptions() {
#ifdef TESTMATE_MYSQL_SUPPORT
    if (!m_config.sslCA.empty()) {
        mysql_ssl_set(m_pConnection,
                     m_config.sslKey.empty() ? nullptr : m_config.sslKey.c_str(),
                     m_config.sslCert.empty() ? nullptr : m_config.sslCert.c_str(),
                     m_config.sslCA.c_str(),
                     nullptr,
                     nullptr);
    }
#endif
}

} // namespace TestMATE
