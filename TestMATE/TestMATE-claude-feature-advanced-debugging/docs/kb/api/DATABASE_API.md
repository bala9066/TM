# Database API Reference

**Version:** 2.0
**Last Updated:** 2025-11-23
**Status:** Production Ready

---

## Overview

The TestMATE Database API provides a flexible, secure, and thread-safe abstraction layer for storing and retrieving test data. The API supports multiple database backends (SQLite, PostgreSQL, MySQL) through a common interface.

**Key Features:**
- ✅ **Multi-Backend Support** - SQLite, PostgreSQL, MySQL/MariaDB
- ✅ **Thread-Safe Operations** - Mutex-protected database access
- ✅ **SQL Injection Prevention** - Parameterized queries with type-safe binding
- ✅ **Transaction Support** - ACID-compliant transactions
- ✅ **Connection Pooling** - Efficient connection management (PostgreSQL/MySQL)
- ✅ **Type Safety** - Strongly-typed parameters using C++20 std::variant

---

## Table of Contents

1. [Core Interfaces](#core-interfaces)
2. [Database Backends](#database-backends)
3. [Secure Query Patterns](#secure-query-patterns)
4. [Thread Safety](#thread-safety)
5. [Connection Management](#connection-management)
6. [Data Operations](#data-operations)
7. [Best Practices](#best-practices)
8. [Examples](#examples)

---

## Core Interfaces

### IDataStore

Base interface for all database backends.

```cpp
class IDataStore {
public:
    virtual ~IDataStore() = default;

    // Connection management
    virtual CResult Connect(const TString& connectionString) = 0;
    virtual CResult Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Data operations
    virtual CResult InsertTestData(const STestDataRecord& data) = 0;
    virtual CResult GetTestData(const TString& testId, STestDataRecord& outData) = 0;
    virtual CResult UpdateTestData(const STestDataRecord& data) = 0;
    virtual CResult DeleteTestData(const TString& testId) = 0;

    // Query operations
    virtual SQueryResult Query(const TString& sqlQuery) = 0;
    virtual SQueryResult QueryParameterized(
        const TString& query,
        const TVector<TQueryParameter>& parameters) = 0;

    // Batch operations
    virtual CResult GetTestDataByLot(const TString& lotId,
                                     TVector<STestDataRecord>& outData) = 0;
    virtual CResult GetTestDataByDateRange(const TString& startDate,
                                           const TString& endDate,
                                           TVector<STestDataRecord>& outData) = 0;
};
```

### TQueryParameter

Type-safe parameter for SQL queries using C++20 `std::variant`.

```cpp
using TQueryParameter = std::variant<TString, TInt64, TDouble>;

// Examples
TQueryParameter stringParam = TString("LOT-12345");
TQueryParameter intParam = TInt64(42);
TQueryParameter doubleParam = TDouble(5.02);
```

### STestDataRecord

```cpp
struct STestDataRecord {
    TString testId;           // Unique test identifier
    TString lotId;            // Lot/batch identifier
    TString testName;         // Test step name
    TString deviceId;         // Device under test ID
    TString timestamp;        // ISO 8601 timestamp
    TDouble measuredValue;    // Measured result
    TString unit;             // Unit of measurement
    TDouble lowLimit;         // Lower specification limit
    TDouble highLimit;        // Upper specification limit
    ETestVerdict verdict;     // PASS/FAIL/ERROR
    TString errorMessage;     // Error details (if any)
    std::map<TString, TString> metadata;  // Additional key-value pairs
};
```

---

## Database Backends

### SQLite Backend (CSqliteDataStore)

**Features:**
- 🔹 Embedded database, no server required
- 🔹 Single file storage
- 🔹 Thread-safe with mutex protection
- 🔹 ACID transactions
- 🔹 Ideal for single-user, embedded systems

**Connection String:**
```cpp
// File-based database
auto db = CDataStoreFactory::Create(EDataStoreType::kSqlite);
db->Connect("test_results.db");

// In-memory database (testing)
db->Connect(":memory:");
```

**Thread Safety:**
All SQLite operations are protected by `std::mutex` to ensure thread-safe access to the single database connection:

```cpp
class CSqliteDataStore : public IDataStore {
private:
    sqlite3* m_pDatabase{nullptr};
    mutable std::mutex m_dbMutex;  // Protects all database operations

public:
    CResult Connect(const TString& connectionString) override {
        std::lock_guard<std::mutex> lock(m_dbMutex);
        // ... safe connection code
    }
};
```

### PostgreSQL Backend (CPostgreSqlDataStore)

**Features:**
- 🔹 Enterprise-grade relational database
- 🔹 Multi-user concurrent access
- 🔹 Advanced features (JSON, full-text search)
- 🔹 Connection pooling
- 🔹 Replication and high availability

**Connection String:**
```cpp
auto db = CDataStoreFactory::Create(EDataStoreType::kPostgreSQL);
db->Connect("postgresql://user:password@localhost:5432/testmate_db");
```

### MySQL/MariaDB Backend (CMySqlDataStore)

**Features:**
- 🔹 High-performance MySQL/MariaDB support
- 🔹 Clustering and replication
- 🔹 Connection pooling
- 🔹 SSL/TLS support

**Connection String:**
```cpp
auto db = CDataStoreFactory::Create(EDataStoreType::kMySQL);
db->Connect("mysql://user:password@localhost:3306/testmate_db");
```

---

## Secure Query Patterns

### ⚠️ SQL Injection Prevention

**CRITICAL:** Always use parameterized queries when incorporating user input or external data.

### ❌ INSECURE Pattern (Vulnerable to SQL Injection)

```cpp
// NEVER DO THIS - Vulnerable to SQL injection!
TString lotId = getUserInput();  // Could be: "LOT123' OR '1'='1"
auto result = db->Query("SELECT * FROM test_data WHERE lot_id = '" + lotId + "'");
```

### ✅ SECURE Pattern (Using QueryParameterized)

```cpp
// ALWAYS DO THIS - Safe from SQL injection
TString lotId = getUserInput();  // User-provided input
TVector<TQueryParameter> params = {lotId};

auto result = db->QueryParameterized(
    "SELECT * FROM test_data WHERE lot_id = ?",
    params
);
```

### QueryParameterized() Method

The `QueryParameterized()` method uses prepared statements with parameter binding to prevent SQL injection:

```cpp
SQueryResult QueryParameterized(
    const TString& in_strQuery,
    const TVector<TQueryParameter>& in_parameters
);
```

**How it Works:**
1. Query is prepared with `?` placeholders
2. Parameters are bound using type-safe `std::variant`
3. Database API validates and escapes parameters
4. Completely prevents SQL injection attacks

**Example with Multiple Parameters:**
```cpp
TString lotId = "LOT-12345";
TInt64 minValue = 100;
TDouble maxTemp = 85.5;

TVector<TQueryParameter> params = {lotId, minValue, maxTemp};

auto result = db->QueryParameterized(
    "SELECT * FROM test_data WHERE lot_id = ? AND count >= ? AND temperature <= ?",
    params
);
```

### When to Use Raw Query()

The raw `Query()` method should **only** be used with:
- ✅ Static SQL strings (no user input)
- ✅ Trusted, validated input
- ✅ Administrative operations

```cpp
// SAFE: No user input, static query
auto result = db->Query("SELECT COUNT(*) FROM test_data");

// SAFE: Trusted, validated input
TString tableName = "test_data";  // From configuration, not user input
if (ValidateTableName(tableName)) {  // Whitelist validation
    auto result = db->Query("SELECT * FROM " + tableName + " LIMIT 100");
}
```

---

## Thread Safety

### SQLite Thread Safety

SQLite uses a **single connection per process** model. All operations must be serialized:

```cpp
// All methods protected by mutex
CResult CSqliteDataStore::InsertTestData(const STestDataRecord& data) {
    std::lock_guard<std::mutex> lock(m_dbMutex);  // Automatic lock
    // ... database operations
}  // Automatic unlock via RAII
```

**Protected Methods:**
- `Connect()` / `Disconnect()`
- `InsertTestData()` / `GetTestData()` / `UpdateTestData()` / `DeleteTestData()`
- `Query()` / `QueryParameterized()`
- `GetTestDataByLot()` / `GetTestDataByDateRange()`

### PostgreSQL/MySQL Thread Safety

PostgreSQL and MySQL support **connection pooling** for better concurrency:

```cpp
// Each thread can get its own connection from the pool
auto conn1 = pool->GetConnection();  // Thread 1
auto conn2 = pool->GetConnection();  // Thread 2
// Both can execute queries concurrently
```

---

## Connection Management

### Opening a Connection

```cpp
auto db = CDataStoreFactory::Create(EDataStoreType::kSqlite);

CResult result = db->Connect("test_results.db");
if (!result.IsSuccess()) {
    LOG_ERROR("Database", "Failed to connect: {}", result.GetMessage());
    return;
}
```

### Checking Connection Status

```cpp
if (db->IsConnected()) {
    // Perform database operations
} else {
    LOG_WARNING("Database", "Not connected to database");
}
```

### Closing a Connection

```cpp
CResult result = db->Disconnect();
if (!result.IsSuccess()) {
    LOG_ERROR("Database", "Failed to disconnect: {}", result.GetMessage());
}
```

### RAII Connection Management

```cpp
class DatabaseGuard {
public:
    DatabaseGuard(IDataStore* db, const TString& connStr) : m_db(db) {
        m_db->Connect(connStr);
    }

    ~DatabaseGuard() {
        if (m_db->IsConnected()) {
            m_db->Disconnect();
        }
    }

private:
    IDataStore* m_db;
};

// Usage
{
    DatabaseGuard guard(db.get(), "test_results.db");
    // Connection is automatically managed
    db->InsertTestData(record);
}  // Automatic disconnect
```

---

## Data Operations

### Inserting Test Data

```cpp
STestDataRecord record;
record.testId = "TEST-001-" + GenerateUUID();
record.lotId = "LOT-12345";
record.testName = "VoltageCheck";
record.deviceId = "DEV-42";
record.timestamp = GetCurrentTimestamp();  // ISO 8601
record.measuredValue = 5.02;
record.unit = "V";
record.lowLimit = 4.75;
record.highLimit = 5.25;
record.verdict = ETestVerdict::kPass;

CResult result = db->InsertTestData(record);
if (!result.IsSuccess()) {
    LOG_ERROR("Database", "Insert failed: {}", result.GetMessage());
}
```

### Retrieving Test Data

```cpp
STestDataRecord record;
CResult result = db->GetTestData("TEST-001-abc123", record);

if (result.IsSuccess()) {
    std::cout << "Test: " << record.testName << std::endl;
    std::cout << "Result: " << record.measuredValue << " " << record.unit << std::endl;
    std::cout << "Verdict: " << (record.verdict == ETestVerdict::kPass ? "PASS" : "FAIL") << std::endl;
} else {
    LOG_ERROR("Database", "Retrieval failed: {}", result.GetMessage());
}
```

### Updating Test Data

```cpp
// Modify existing record
record.measuredValue = 5.05;
record.verdict = ETestVerdict::kPass;

CResult result = db->UpdateTestData(record);
```

### Deleting Test Data

```cpp
CResult result = db->DeleteTestData("TEST-001-abc123");
```

### Batch Retrieval by Lot

```cpp
TVector<STestDataRecord> lotData;
CResult result = db->GetTestDataByLot("LOT-12345", lotData);

if (result.IsSuccess()) {
    LOG_INFO("Database", "Retrieved {} records for LOT-12345", lotData.size());
    for (const auto& record : lotData) {
        // Process each record
    }
}
```

### Batch Retrieval by Date Range

```cpp
TVector<STestDataRecord> dateRangeData;
CResult result = db->GetTestDataByDateRange(
    "2025-01-01T00:00:00Z",
    "2025-01-31T23:59:59Z",
    dateRangeData
);
```

---

## Best Practices

### 1. Always Use Parameterized Queries for User Input

```cpp
// ✅ GOOD
TVector<TQueryParameter> params = {userInput};
db->QueryParameterized("SELECT * FROM test_data WHERE device_id = ?", params);

// ❌ BAD
db->Query("SELECT * FROM test_data WHERE device_id = '" + userInput + "'");
```

### 2. Handle Errors Gracefully

```cpp
CResult result = db->InsertTestData(record);
if (!result.IsSuccess()) {
    LOG_ERROR("Database", "Insert failed: {} (Error code: {})",
              result.GetMessage(), static_cast<int>(result.GetErrorCode()));

    // Implement fallback strategy
    SaveToLocalFile(record);
}
```

### 3. Use RAII for Resource Management

```cpp
// Database connection is automatically closed when guard goes out of scope
{
    DatabaseGuard guard(db.get(), "test_results.db");
    PerformDatabaseOperations();
}  // Automatic cleanup
```

### 4. Validate Input Before Database Operations

```cpp
CResult ValidateTestRecord(const STestDataRecord& record) {
    if (record.testId.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Test ID cannot be empty");
    }
    if (record.testName.length() > 255) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Test name too long");
    }
    return TESTMATE_SUCCESS();
}

// Usage
auto validation = ValidateTestRecord(record);
if (validation.IsSuccess()) {
    db->InsertTestData(record);
}
```

### 5. Use Transactions for Multiple Operations

```cpp
// Begin transaction
db->Query("BEGIN TRANSACTION");

try {
    db->InsertTestData(record1);
    db->InsertTestData(record2);
    db->InsertTestData(record3);

    // Commit if all succeeded
    db->Query("COMMIT");
} catch (const std::exception& e) {
    // Rollback on error
    db->Query("ROLLBACK");
    LOG_ERROR("Database", "Transaction failed: {}", e.what());
}
```

### 6. Implement Connection Retry Logic

```cpp
CResult ConnectWithRetry(IDataStore* db, const TString& connStr, int maxRetries = 3) {
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        CResult result = db->Connect(connStr);
        if (result.IsSuccess()) {
            return result;
        }

        LOG_WARNING("Database", "Connection attempt {} failed, retrying...", attempt + 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Max retry attempts exceeded");
}
```

### 7. Use Prepared Statements for Repeated Queries

```cpp
// For operations performed many times, prepare once and reuse
TString query = "INSERT INTO test_data (test_id, value) VALUES (?, ?)";

for (const auto& measurement : measurements) {
    TVector<TQueryParameter> params = {measurement.id, measurement.value};
    db->QueryParameterized(query, params);
}
```

---

## Examples

### Example 1: Simple Insert and Query

```cpp
#include "database/DataStoreFactory.h"
#include "utils/LogManager.h"

int main() {
    // Create SQLite database
    auto db = CDataStoreFactory::Create(EDataStoreType::kSqlite);
    db->Connect("results.db");

    // Insert test result
    STestDataRecord record;
    record.testId = "TEST-001";
    record.testName = "Power Supply Test";
    record.measuredValue = 12.05;
    record.unit = "V";
    record.verdict = ETestVerdict::kPass;

    db->InsertTestData(record);

    // Query with parameterized search
    TVector<TQueryParameter> params = {TString("Power%")};
    auto results = db->QueryParameterized(
        "SELECT * FROM test_data WHERE test_name LIKE ?",
        params
    );

    LOG_INFO("Database", "Found {} matching tests", results.rows.size());

    db->Disconnect();
    return 0;
}
```

### Example 2: Batch Processing with Thread Safety

```cpp
#include "core/threading/ThreadPool.h"
#include "database/DataStoreFactory.h"

void ProcessLot(IDataStore* db, const TString& lotId) {
    // Database operations are thread-safe (mutex protected)
    TVector<STestDataRecord> data;
    db->GetTestDataByLot(lotId, data);

    for (auto& record : data) {
        // Process each record
        record.metadata["processed"] = "true";
        db->UpdateTestData(record);
    }
}

int main() {
    auto db = CDataStoreFactory::Create(EDataStoreType::kSqlite);
    db->Connect("results.db");

    CThreadPool pool(4);  // 4 worker threads

    // Process multiple lots in parallel (database is thread-safe)
    TVector<TString> lots = {"LOT-001", "LOT-002", "LOT-003"};
    for (const auto& lot : lots) {
        pool.Submit([&db, lot]() {
            ProcessLot(db.get(), lot);
        });
    }

    pool.WaitForCompletion();
    db->Disconnect();
    return 0;
}
```

### Example 3: Secure Dynamic Filtering

```cpp
TVector<STestDataRecord> SearchTests(IDataStore* db,
                                      const TString& deviceId,
                                      const TString& testName,
                                      ETestVerdict verdict) {
    // Build query with parameterized values
    TString query = "SELECT * FROM test_data WHERE device_id = ? AND test_name = ?";
    TVector<TQueryParameter> params = {deviceId, testName};

    if (verdict != ETestVerdict::kUnknown) {
        query += " AND verdict = ?";
        params.push_back(TInt64(static_cast<int>(verdict)));
    }

    auto result = db->QueryParameterized(query, params);

    // Convert query result to vector of records
    TVector<STestDataRecord> records;
    for (const auto& row : result.rows) {
        STestDataRecord record;
        // Parse row into record
        records.push_back(record);
    }

    return records;
}
```

---

## Error Handling

### Common Error Codes

| Error Code | Description | Resolution |
|------------|-------------|------------|
| `kConnectionFailed` | Cannot connect to database | Check connection string, database permissions |
| `kQueryFailed` | SQL query execution failed | Check SQL syntax, validate parameters |
| `kInvalidParameter` | Invalid input parameter | Validate data before database call |
| `kRecordNotFound` | Requested record doesn't exist | Check if record exists before update/delete |
| `kDatabaseLocked` | Database is locked (SQLite) | Retry with backoff, check for long transactions |

### Error Handling Pattern

```cpp
CResult result = db->InsertTestData(record);

switch (result.GetErrorCode()) {
    case EErrorCode::kSuccess:
        LOG_INFO("Database", "Record inserted successfully");
        break;

    case EErrorCode::kConnectionFailed:
        LOG_ERROR("Database", "Connection lost, attempting reconnect");
        db->Disconnect();
        db->Connect(connectionString);
        break;

    case EErrorCode::kInvalidParameter:
        LOG_ERROR("Database", "Invalid data: {}", result.GetMessage());
        // Fix data and retry
        break;

    default:
        LOG_ERROR("Database", "Unexpected error: {}", result.GetMessage());
        break;
}
```

---

## Performance Considerations

### 1. Batch Operations

```cpp
// ❌ SLOW: Individual inserts
for (const auto& record : records) {
    db->InsertTestData(record);  // Separate transaction for each
}

// ✅ FAST: Batch insert with transaction
db->Query("BEGIN TRANSACTION");
for (const auto& record : records) {
    db->InsertTestData(record);
}
db->Query("COMMIT");  // Single transaction
```

### 2. Indexing

```sql
-- Create indexes for frequently queried columns
CREATE INDEX idx_lot_id ON test_data(lot_id);
CREATE INDEX idx_timestamp ON test_data(timestamp);
CREATE INDEX idx_device_verdict ON test_data(device_id, verdict);
```

### 3. Connection Pooling (PostgreSQL/MySQL)

```cpp
// Use connection pool for multi-threaded applications
auto pool = std::make_shared<ConnectionPool>(
    connectionString,
    minConnections = 5,
    maxConnections = 20
);

// Each thread gets its own connection
auto conn = pool->GetConnection();
```

---

## Security Checklist

- [x] Use `QueryParameterized()` for all user input
- [x] Validate all input before database operations
- [x] Use transactions for multi-step operations
- [x] Implement proper error handling with logging
- [x] Use RAII for connection management
- [x] Set appropriate database file permissions (SQLite)
- [x] Use SSL/TLS for remote connections (PostgreSQL/MySQL)
- [x] Never log sensitive data (passwords, PII)
- [x] Implement retry logic with exponential backoff
- [x] Monitor database operations for anomalies

---

## See Also

- [SECURITY.md](../../../SECURITY.md) - Comprehensive security guidelines
- [CORE_API.md](CORE_API.md) - Core API reference
- [Database Setup Guide](../guides/DATABASE_SETUP.md) - Database configuration
- [Tutorial: Database Integration](../tutorials/DATABASE_INTEGRATION.md)

---

**Last Updated:** 2025-11-23
**Version:** 2.0
**Status:** Production Ready
