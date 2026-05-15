# TestMATE Best Practices

**Version:** 2.0
**Last Updated:** 2025-11-23
**Status:** Production Ready

---

## Overview

This document outlines recommended best practices for developing robust, secure, and maintainable test automation solutions with TestMATE.

---

## Table of Contents

1. [Security Best Practices](#security-best-practices)
2. [Thread Safety Patterns](#thread-safety-patterns)
3. [Error Handling](#error-handling)
4. [Resource Management](#resource-management)
5. [Database Operations](#database-operations)
6. [Test Design](#test-design)
7. [Performance Optimization](#performance-optimization)
8. [Code Quality](#code-quality)

---

## Security Best Practices

### 1. Always Use Parameterized Queries

**Why:** Prevents SQL injection attacks.

```cpp
// ❌ VULNERABLE to SQL injection
TString deviceId = GetUserInput();
db->Query("SELECT * FROM test_data WHERE device_id = '" + deviceId + "'");

// ✅ SECURE with parameterized query
TString deviceId = GetUserInput();
TVector<TQueryParameter> params = {deviceId};
db->QueryParameterized("SELECT * FROM test_data WHERE device_id = ?", params);
```

**Rule:** Use `QueryParameterized()` for any query involving external input (user input, file data, network data, instrument responses).

### 2. Validate All External Input

```cpp
CResult ValidateDeviceId(const TString& deviceId) {
    // Check length
    if (deviceId.length() > 50) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Device ID too long");
    }

    // Check format (alphanumeric and hyphens only)
    if (!std::regex_match(deviceId, std::regex("^[A-Za-z0-9-]+$"))) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid device ID format");
    }

    return TESTMATE_SUCCESS();
}

// Usage
auto validation = ValidateDeviceId(userInput);
if (validation.IsSuccess()) {
    // Safe to use
} else {
    LOG_ERROR("Input", "Validation failed: {}", validation.GetMessage());
}
```

### 3. Sanitize File Paths

**Prevent path traversal attacks:**

```cpp
CResult ValidateFilePath(const TString& path) {
    // Check for path traversal attempts
    if (path.find("..") != TString::npos) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Path traversal not allowed");
    }

    // Use absolute paths
    std::filesystem::path absPath = std::filesystem::absolute(path);

    // Ensure path is within allowed directory
    std::filesystem::path allowedDir = "/home/testmate/data";
    auto [it, end] = std::mismatch(allowedDir.begin(), allowedDir.end(),
                                    absPath.begin());
    if (it != allowedDir.end()) {
        return TESTMATE_FAILURE(EErrorCode::kAccessDenied,
                                "Path outside allowed directory");
    }

    return TESTMATE_SUCCESS();
}
```

### 4. Never Log Sensitive Information

```cpp
// ❌ DON'T log passwords, keys, PII
LOG_INFO("Auth", "User password: {}", password);  // NEVER!
LOG_INFO("Database", "Connection string: {}", connStr);  // May contain password!

// ✅ DO log sanitized information
LOG_INFO("Auth", "Login attempt for user: {}", username);
LOG_INFO("Database", "Connected to database: {}", dbName);  // No credentials
```

**Never log:**
- Passwords or credentials
- API keys or tokens
- Personal Identifiable Information (PII)
- Credit card numbers
- Social security numbers

### 5. Use Secure Default Configurations

```cpp
SSecurityConfig GetDefaultSecurityConfig() {
    SSecurityConfig config;
    config.enableSSL = true;              // Always use encryption
    config.validateCertificates = true;   // Verify SSL certificates
    config.minPasswordLength = 12;        // Strong password requirement
    config.sessionTimeout = 900;          // 15 minutes
    config.maxLoginAttempts = 3;          // Prevent brute force
    return config;
}
```

---

## Thread Safety Patterns

### 1. Use RAII for Mutex Locks

```cpp
class ThreadSafeCounter {
public:
    void Increment() {
        std::lock_guard<std::mutex> lock(m_mutex);  // RAII lock
        ++m_count;
    }  // Automatic unlock

    int GetCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

private:
    mutable std::mutex m_mutex;
    int m_count{0};
};
```

**Why:** RAII ensures mutex is always unlocked, even if exceptions occur.

### 2. Protect All Shared Resource Access

```cpp
class CSqliteDataStore : public IDataStore {
public:
    CResult InsertTestData(const STestDataRecord& data) override {
        std::lock_guard<std::mutex> lock(m_dbMutex);  // Protect database access
        // ... SQLite operations
    }

    SQueryResult Query(const TString& query) override {
        std::lock_guard<std::mutex> lock(m_dbMutex);  // Protect database access
        // ... SQLite operations
    }

private:
    sqlite3* m_pDatabase{nullptr};
    mutable std::mutex m_dbMutex;  // Protects m_pDatabase
};
```

### 3. Avoid Deadlocks with Lock Ordering

```cpp
class ResourceManager {
public:
    void TransferResource(int fromId, int toId) {
        // Always lock in consistent order (ascending ID) to prevent deadlock
        if (fromId < toId) {
            std::lock_guard<std::mutex> lock1(m_mutexes[fromId]);
            std::lock_guard<std::mutex> lock2(m_mutexes[toId]);
            // ... transfer logic
        } else {
            std::lock_guard<std::mutex> lock1(m_mutexes[toId]);
            std::lock_guard<std::mutex> lock2(m_mutexes[fromId]);
            // ... transfer logic
        }
    }

private:
    std::array<std::mutex, 100> m_mutexes;
};
```

### 4. Use std::scoped_lock for Multiple Mutexes

```cpp
void TransferData(Resource& from, Resource& to) {
    // Locks both mutexes atomically, prevents deadlock
    std::scoped_lock lock(from.mutex, to.mutex);
    from.data -= 100;
    to.data += 100;
}
```

### 5. Prefer Immutable Data in Multi-Threaded Code

```cpp
// ✅ GOOD: Immutable configuration shared across threads
const SConfig config = LoadConfig();  // const = immutable

pool.Submit([config]() {  // Copy by value
    ProcessData(config);  // Safe, no locks needed
});
```

---

## Error Handling

### 1. Always Check CResult Return Values

```cpp
// ❌ BAD: Ignoring errors
db->InsertTestData(record);

// ✅ GOOD: Checking errors
CResult result = db->InsertTestData(record);
if (!result.IsSuccess()) {
    LOG_ERROR("Database", "Insert failed: {}", result.GetMessage());
    return result;  // Propagate error
}
```

### 2. Use TESTMATE_SUCCESS() and TESTMATE_FAILURE()

```cpp
CResult ValidateInput(const TString& input) {
    if (input.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Input cannot be empty");
    }

    if (input.length() > MAX_LENGTH) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Input exceeds maximum length");
    }

    return TESTMATE_SUCCESS();
}
```

### 3. Log Exception Details

```cpp
// ❌ BAD: Silent exception swallowing
try {
    ProcessData();
} catch (...) {
    // Silently ignored - debugging nightmare!
}

// ✅ GOOD: Log with context
try {
    ProcessData();
} catch (const std::exception& e) {
    LOG_ERROR("Processing", "Failed to process data: {}", e.what());
    return TESTMATE_FAILURE(EErrorCode::kProcessingFailed, e.what());
} catch (...) {
    LOG_ERROR("Processing", "Unknown exception in ProcessData()");
    return TESTMATE_FAILURE(EErrorCode::kUnknownError, "Unknown exception");
}
```

### 4. Provide Context in Error Messages

```cpp
// ❌ BAD: Vague error message
return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid input");

// ✅ GOOD: Specific error message with context
return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
    "Device ID '" + deviceId + "' contains invalid characters. "
    "Only alphanumeric and hyphens allowed.");
```

### 5. Use Exceptions for Exceptional Conditions Only

```cpp
// ✅ GOOD: Use CResult for expected errors
CResult LoadConfig(const TString& path) {
    if (!FileExists(path)) {
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "Config file not found");
    }
    // ... load config
    return TESTMATE_SUCCESS();
}

// ✅ GOOD: Use exceptions for programming errors
void SetValue(int index, double value) {
    if (index >= m_values.size()) {
        throw std::out_of_range("Index " + std::to_string(index) + " out of range");
    }
    m_values[index] = value;
}
```

---

## Resource Management

### 1. Use Smart Pointers

```cpp
// ❌ BAD: Raw pointers and manual memory management
IDataStore* db = new CSqliteDataStore();
// ... use db ...
delete db;  // Easy to forget, leaks if exception occurs

// ✅ GOOD: Smart pointers with automatic cleanup
TUniquePtr<IDataStore> db = std::make_unique<CSqliteDataStore>();
// ... use db ...
// Automatic cleanup, exception-safe
```

### 2. Prefer std::make_unique and std::make_shared

```cpp
// ❌ AVOID: Direct new
auto ptr = TUniquePtr<Instrument>(new CMultimeter());

// ✅ PREFER: make_unique (exception-safe, more efficient)
auto ptr = std::make_unique<CMultimeter>();

// ✅ For shared ownership
auto shared = std::make_shared<CTestSequence>();
```

### 3. Use RAII for All Resources

```cpp
class FileGuard {
public:
    explicit FileGuard(const TString& filename)
        : m_file(fopen(filename.c_str(), "w")) {
        if (!m_file) {
            throw std::runtime_error("Failed to open file");
        }
    }

    ~FileGuard() {
        if (m_file) {
            fclose(m_file);  // Automatic cleanup
        }
    }

    // Delete copy operations
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    FILE* Get() { return m_file; }

private:
    FILE* m_file;
};

// Usage
{
    FileGuard file("output.txt");
    fprintf(file.Get(), "Test results\n");
}  // File automatically closed
```

### 4. Set Resource Limits

```cpp
// Thread pool with bounded queue
static constexpr size_t kMaxQueueSize = 10000;

if (m_queueTasks.size() >= kMaxQueueSize) {
    throw std::runtime_error("Thread pool queue is full");
}

// Connection pool with max connections
static constexpr size_t kMaxConnections = 20;

if (m_activeConnections >= kMaxConnections) {
    return TESTMATE_FAILURE(EErrorCode::kResourceExhausted,
                            "Maximum connections reached");
}
```

---

## Database Operations

### 1. Use Transactions for Multiple Operations

```cpp
CResult BatchInsert(const TVector<STestDataRecord>& records) {
    db->Query("BEGIN TRANSACTION");

    try {
        for (const auto& record : records) {
            auto result = db->InsertTestData(record);
            if (!result.IsSuccess()) {
                db->Query("ROLLBACK");
                return result;
            }
        }
        db->Query("COMMIT");
        return TESTMATE_SUCCESS();
    } catch (const std::exception& e) {
        db->Query("ROLLBACK");
        return TESTMATE_FAILURE(EErrorCode::kDatabaseError, e.what());
    }
}
```

### 2. Implement Connection Retry Logic

```cpp
CResult ConnectWithRetry(IDataStore* db, const TString& connStr) {
    const int MAX_RETRIES = 3;
    const int RETRY_DELAY_MS = 1000;

    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) {
        auto result = db->Connect(connStr);
        if (result.IsSuccess()) {
            return result;
        }

        LOG_WARNING("Database", "Connection attempt {} failed: {}",
                    attempt, result.GetMessage());

        if (attempt < MAX_RETRIES) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS * attempt));
        }
    }

    return TESTMATE_FAILURE(EErrorCode::kConnectionFailed,
                            "Failed to connect after " + std::to_string(MAX_RETRIES) + " attempts");
}
```

### 3. Close Connections Properly

```cpp
class DatabaseSession {
public:
    DatabaseSession(TUniquePtr<IDataStore> db, const TString& connStr)
        : m_db(std::move(db)) {
        auto result = m_db->Connect(connStr);
        if (!result.IsSuccess()) {
            throw std::runtime_error("Failed to connect: " + result.GetMessage());
        }
    }

    ~DatabaseSession() {
        if (m_db->IsConnected()) {
            m_db->Disconnect();
        }
    }

    IDataStore* Get() { return m_db.get(); }

private:
    TUniquePtr<IDataStore> m_db;
};
```

---

## Test Design

### 1. Keep Tests Independent

```cpp
// ❌ BAD: Tests depend on order
TEST(MyTests, Test1) {
    globalCounter = 5;  // Sets global state
}

TEST(MyTests, Test2) {
    EXPECT_EQ(globalCounter, 5);  // Depends on Test1!
}

// ✅ GOOD: Each test is independent
TEST(MyTests, Test1) {
    int counter = 5;
    EXPECT_EQ(counter, 5);
}

TEST(MyTests, Test2) {
    int counter = 5;  // Own state
    EXPECT_EQ(counter, 5);
}
```

### 2. Use Descriptive Test Names

```cpp
// ❌ BAD: Vague test name
TEST(Database, Test1) { ... }

// ✅ GOOD: Descriptive test name
TEST(Database, InsertTestData_WithValidRecord_ReturnsSuccess) { ... }
TEST(Database, QueryParameterized_WithSQLInjection_IsSafe) { ... }
```

### 3. Test Error Paths

```cpp
TEST(Database, InsertTestData_WhenDisconnected_ReturnsError) {
    CSqliteDataStore db;
    // Don't connect
    STestDataRecord record;
    auto result = db.InsertTestData(record);

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetErrorCode(), EErrorCode::kNotConnected);
}
```

### 4. Use Setup and Teardown

```cpp
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_db = std::make_unique<CSqliteDataStore>();
        m_db->Connect(":memory:");  // In-memory test database
    }

    void TearDown() override {
        m_db->Disconnect();
        m_db.reset();
    }

    TUniquePtr<CSqliteDataStore> m_db;
};

TEST_F(DatabaseTest, InsertAndRetrieve) {
    // Database is ready to use
    STestDataRecord record;
    m_db->InsertTestData(record);
    // ... test logic
}
```

---

## Performance Optimization

### 1. Use Batch Operations

```cpp
// ❌ SLOW: Individual operations
for (const auto& record : records) {
    db->InsertTestData(record);  // Separate transaction each
}

// ✅ FAST: Batch with single transaction
db->Query("BEGIN TRANSACTION");
for (const auto& record : records) {
    db->InsertTestData(record);
}
db->Query("COMMIT");
```

### 2. Reuse Connections and Resources

```cpp
// ❌ INEFFICIENT: Creating new connection for each operation
void ProcessRecords(const TVector<STestDataRecord>& records) {
    for (const auto& record : records) {
        auto db = CreateDatabase();
        db->Connect("results.db");
        db->InsertTestData(record);
        db->Disconnect();
    }
}

// ✅ EFFICIENT: Reuse single connection
void ProcessRecords(const TVector<STestDataRecord>& records) {
    auto db = CreateDatabase();
    db->Connect("results.db");
    for (const auto& record : records) {
        db->InsertTestData(record);
    }
    db->Disconnect();
}
```

### 3. Use Appropriate Thread Pool Size

```cpp
// Get optimal thread count
unsigned int threadCount = std::thread::hardware_concurrency();
if (threadCount == 0) threadCount = 4;  // Fallback

// Don't over-subscribe
CThreadPool pool(threadCount);  // Not threadCount * 10!
```

### 4. Profile Before Optimizing

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

profiler.StartProfiling("DatabaseOperation");
db->InsertTestData(record);
profiler.StopProfiling("DatabaseOperation");

// Generate report to identify bottlenecks
profiler.GenerateReport("performance_report.html");
```

---

## Code Quality

### 1. Use const Correctness

```cpp
class CTestSequence {
public:
    // ✅ Const methods for read-only operations
    TSize GetStepCount() const { return m_steps.size(); }
    const TString& GetName() const { return m_name; }

    // ✅ Const parameters when not modifying
    void AddStep(const TString& name, const SStepConfig& config) {
        m_steps.push_back({name, config});
    }

private:
    TString m_name;
    TVector<SStep> m_steps;
};
```

### 2. Avoid Magic Numbers

```cpp
// ❌ BAD: Magic numbers
if (temperature > 85.0) {  // What is 85.0?
    return TESTMATE_FAILURE(...);
}

// ✅ GOOD: Named constants
static constexpr TDouble MAX_OPERATING_TEMP = 85.0;  // Celsius

if (temperature > MAX_OPERATING_TEMP) {
    return TESTMATE_FAILURE(EErrorCode::kTemperatureExceeded,
                            "Temperature exceeds maximum operating limit");
}
```

### 3. Use Meaningful Variable Names

```cpp
// ❌ BAD: Cryptic names
int d = 86400;
auto v = db->Query("SELECT * FROM t");

// ✅ GOOD: Descriptive names
int secondsPerDay = 86400;
auto testResults = db->Query("SELECT * FROM test_data");
```

### 4. Keep Functions Small and Focused

```cpp
// ✅ GOOD: Single responsibility
CResult ValidateInput(const STestInput& input);
CResult ConnectToInstrument(const TString& address);
CResult ExecuteTest(const STestConfig& config);
CResult SaveResults(const TVector<STestResult>& results);

// ❌ BAD: Does everything
CResult DoEverything(/* many parameters */) {
    // 500 lines of mixed responsibilities
}
```

---

## Additional Resources

- [SECURITY.md](../../../SECURITY.md) - Comprehensive security guidelines
- [Database API](../api/DATABASE_API.md) - Database API reference
- [Cheat Sheet](CHEAT_SHEET.md) - Quick reference
- [Code Review Report](../../../CODE_REVIEW_REPORT.md) - Code quality review findings

---

**Last Updated:** 2025-11-23
**Version:** 2.0
**Status:** Production Ready
