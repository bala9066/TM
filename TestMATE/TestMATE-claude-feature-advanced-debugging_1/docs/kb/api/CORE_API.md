# Core API Reference

**TestMATE Core Execution Engine API**

This document provides comprehensive reference for the TestMATE core execution engine APIs including process models, test execution, threading, and resource management.

---

## Table of Contents

1. [Common Types](#common-types)
2. [Result & Error Handling](#result--error-handling)
3. [Process Models](#process-models)
4. [Execution Context](#execution-context)
5. [Test Executor](#test-executor)
6. [Test Sequence & Steps](#test-sequence--steps)
7. [Threading & Synchronization](#threading--synchronization)
8. [Resource Scheduling](#resource-scheduling)

---

## Common Types

### Header: `include/testmate/common/Types.h`

Standardized type aliases following SDG coding guidelines.

#### Basic Types

```cpp
// Integer types
using TInt8 = int8_t;
using TInt16 = int16_t;
using TInt32 = int32_t;
using TInt64 = int64_t;

using TUInt8 = uint8_t;
using TUInt16 = uint16_t;
using TUInt32 = uint32_t;
using TUInt64 = uint64_t;

// Floating point
using TFloat = float;
using TDouble = double;

// String types
using TString = std::string;
using TStringView = std::string_view;
```

#### Time Types

```cpp
using TClock = std::chrono::steady_clock;
using TTimePoint = std::chrono::time_point<TClock>;
using TDuration = std::chrono::milliseconds;
using TMicroseconds = std::chrono::microseconds;
using TNanoseconds = std::chrono::nanoseconds;
```

**Example:**
```cpp
auto start = TClock::now();
// ... perform operation ...
auto end = TClock::now();
auto duration = std::chrono::duration_cast<TDuration>(end - start);
std::cout << "Operation took: " << duration.count() << "ms" << std::endl;
```

#### Container Types

```cpp
template<typename T>
using TVector = std::vector<T>;

template<typename K, typename V>
using TMap = std::map<K, V>;

template<typename T>
using TOptional = std::optional<T>;

template<typename... Ts>
using TVariant = std::variant<Ts...>;
```

#### Smart Pointers

```cpp
template<typename T>
using TUniquePtr = std::unique_ptr<T>;

template<typename T>
using TSharedPtr = std::shared_ptr<T>;

template<typename T>
using TWeakPtr = std::weak_ptr<T>;
```

#### Callback Types

```cpp
template<typename... Args>
using TCallback = std::function<void(Args...)>;

using TVoidCallback = std::function<void()>;
using TBoolCallback = std::function<bool()>;
```

---

## Result & Error Handling

### Header: `include/testmate/common/Result.h`

The `CResult<T>` pattern for error handling without exceptions.

#### CResult<T> Class

```cpp
template<typename T = void>
class CResult {
public:
    // Success result
    static CResult Success(T value);
    static CResult Success();  // For CResult<void>

    // Error result
    static CResult Error(EErrorCode code, const TString& message);

    // Check status
    bool IsSuccess() const;
    bool IsError() const;

    // Get value (throws if error)
    const T& GetValue() const;
    T& GetValue();

    // Get error information
    EErrorCode GetErrorCode() const;
    const TString& GetMessage() const;
};
```

#### Error Codes

```cpp
enum class EErrorCode : TUInt32 {
    kSuccess = 0,
    kGenericError,
    kInvalidParameter,
    kNotFound,
    kAlreadyExists,
    kNotInitialized,
    kConnectionFailed,
    kTimeoutExpired,
    kResourceBusy,
    kInsufficientResources,
    kOperationFailed,
    kNotSupported,
    kDatabaseQueryFailed,
    kFileNotFound,
    kParseError,
    kValidationError
};
```

#### Usage Example

```cpp
CResult<TDouble> ReadVoltage() {
    if (!IsConnected()) {
        return CResult<TDouble>::Error(
            EErrorCode::kConnectionFailed,
            "Instrument not connected"
        );
    }

    TDouble voltage = 5.0;  // Read from instrument
    return CResult<TDouble>::Success(voltage);
}

// Using the result
auto result = ReadVoltage();
if (result.IsSuccess()) {
    std::cout << "Voltage: " << result.GetValue() << "V" << std::endl;
} else {
    std::cerr << "Error: " << result.GetMessage() << std::endl;
}
```

---

## Process Models

### Header: `src/core/process_models/IProcessModel.h`

Process models define how test sequences are executed.

#### IProcessModel Interface

```cpp
class IProcessModel {
public:
    virtual ~IProcessModel() = default;

    // Lifecycle
    virtual CResult<void> Initialize() = 0;
    virtual CResult<void> Execute(CTestSequence& sequence) = 0;
    virtual CResult<void> Pause() = 0;
    virtual CResult<void> Resume() = 0;
    virtual CResult<void> Abort() = 0;
    virtual CResult<void> Shutdown() = 0;

    // State queries
    virtual EProcessModelState GetState() const = 0;
    virtual TDouble GetProgress() const = 0;

    // Configuration
    virtual void SetCallback(ECallbackType type, TVoidCallback callback) = 0;
};
```

#### Process Model States

```cpp
enum class EProcessModelState : TUInt8 {
    kIdle,          // Not running
    kInitializing,  // Preparing to execute
    kRunning,       // Actively executing
    kPaused,        // Execution paused
    kCompleted,     // Finished successfully
    kAborted,       // Manually stopped
    kError          // Error occurred
};
```

#### Callback Types

```cpp
enum class ECallbackType : TUInt8 {
    kOnProcessModelStart,
    kOnProcessModelComplete,
    kOnSocketStart,      // Parallel/Batch only
    kOnSocketComplete,   // Parallel/Batch only
    kOnTestStart,
    kOnTestComplete,
    kOnStepStart,
    kOnStepComplete,
    kOnError,
    kOnAbort
};
```

### Sequential Process Model

**Header:** `src/core/process_models/SequentialModel.h`

Executes test steps in linear order, one at a time.

```cpp
class CSequentialModel : public ProcessModelBase {
public:
    CSequentialModel();
    ~CSequentialModel() override;

    CResult<void> Initialize() override;
    CResult<void> Execute(CTestSequence& sequence) override;
    // ... other IProcessModel methods
};
```

**Example:**
```cpp
CSequentialModel sequentialModel;
CTestSequence sequence;
// ... load sequence ...

sequentialModel.Initialize();
auto result = sequentialModel.Execute(sequence);
if (result.IsSuccess()) {
    std::cout << "Test completed successfully" << std::endl;
}
```

### Parallel Process Model

**Header:** `src/core/process_models/ParallelModel.h`

Executes tests on multiple sockets (devices) simultaneously.

```cpp
class CParallelModel : public ProcessModelBase {
public:
    explicit CParallelModel(TUInt32 numSockets);
    ~CParallelModel() override;

    CResult<void> Initialize() override;
    CResult<void> Execute(CTestSequence& sequence) override;

    // Parallel-specific methods
    void SetNumSockets(TUInt32 num);
    TUInt32 GetNumSockets() const;
    EExecutionState GetSocketState(TUInt32 socketId) const;
};
```

**Socket States:**
```cpp
enum class EExecutionState : TUInt8 {
    kIdle,
    kInitializing,
    kTesting,
    kPaused,
    kCompleted,
    kError,
    kAborted
};
```

**Example:**
```cpp
// Test 4 devices in parallel
CParallelModel parallelModel(4);
CTestSequence sequence;
// ... load sequence ...

// Set up callbacks for each socket
parallelModel.SetCallback(ECallbackType::kOnSocketComplete,
    [&](TUInt32 socketId) {
        std::cout << "Socket " << socketId << " completed" << std::endl;
    });

parallelModel.Initialize();
parallelModel.Execute(sequence);
```

### Batch Process Model

**Header:** `src/core/process_models/BatchModel.h`

Executes tests on multiple devices with shared setup/cleanup.

```cpp
class CBatchModel : public ProcessModelBase {
public:
    explicit CBatchModel(TUInt32 batchSize);
    ~CBatchModel() override;

    CResult<void> Initialize() override;
    CResult<void> Execute(CTestSequence& sequence) override;

    // Batch-specific methods
    void SetBatchSize(TUInt32 size);
    TUInt32 GetBatchSize() const;
    TUInt32 GetCompletedCount() const;
};
```

**Example:**
```cpp
// Test 10 devices as a batch
CBatchModel batchModel(10);
CTestSequence sequence;

batchModel.Initialize();
batchModel.Execute(sequence);

std::cout << "Completed: " << batchModel.GetCompletedCount()
          << " / " << batchModel.GetBatchSize() << std::endl;
```

---

## Execution Context

### Header: `src/core/process_models/ExecutionContext.h`

Contains execution state and data for a test instance.

#### CExecutionContext Class

```cpp
class CExecutionContext {
public:
    CExecutionContext(TUInt32 socketId, const STestInfo& testInfo);

    // Socket/Site identification
    TUInt32 GetSocketId() const;
    void SetSocketId(TUInt32 id);

    // Test information
    const STestInfo& GetTestInfo() const;
    void SetTestInfo(const STestInfo& info);

    // State management
    EExecutionState GetState() const;
    void SetState(EExecutionState state);

    // Timestamps
    TTimePoint GetStartTime() const;
    TTimePoint GetEndTime() const;
    TDuration GetElapsedTime() const;

    // Local variables
    void SetVariable(const TString& name, const TVariant<>& value);
    TOptional<TVariant<>> GetVariable(const TString& name) const;
    bool HasVariable(const TString& name) const;

    // Results
    void AddStepResult(const SStepResult& result);
    const TVector<SStepResult>& GetStepResults() const;

    // Device information
    void SetDeviceId(const TString& id);
    TString GetDeviceId() const;
};
```

#### Test Information Structure

```cpp
struct STestInfo {
    TString id;            // Unique test ID
    TString name;          // Human-readable name
    TString version;       // Version string
    TString description;   // Optional description
    TMap<TString, TString> metadata;  // Additional key-value data
};
```

**Example:**
```cpp
STestInfo testInfo;
testInfo.id = "PWR-001";
testInfo.name = "Power Supply Test";
testInfo.version = "1.0.0";

CExecutionContext context(0, testInfo);  // Socket 0
context.SetDeviceId("DUT-12345");

// Store intermediate results
context.SetVariable("voltage", 5.02);
context.SetVariable("current", 2.48);

// Retrieve later
auto voltage = context.GetVariable("voltage");
if (voltage.has_value()) {
    // Use the value
}
```

---

## Test Executor

### Header: `src/core/execution/TestExecutor.h`

Manages test execution using process models.

#### CTestExecutor Class

```cpp
class CTestExecutor {
public:
    CTestExecutor();
    ~CTestExecutor();

    // Process model selection
    void SetProcessModel(TSharedPtr<IProcessModel> model);
    TSharedPtr<IProcessModel> GetProcessModel() const;

    // Execution control
    CResult<void> ExecuteSequence(CTestSequence& sequence);
    CResult<void> Pause();
    CResult<void> Resume();
    CResult<void> Abort();

    // State queries
    EProcessModelState GetState() const;
    TDouble GetProgress() const;

    // Callbacks
    void SetCallback(ECallbackType type, TVoidCallback callback);
};
```

**Example:**
```cpp
CTestExecutor executor;

// Use sequential execution
auto sequentialModel = std::make_shared<CSequentialModel>();
executor.SetProcessModel(sequentialModel);

// Set up callbacks
executor.SetCallback(ECallbackType::kOnStepComplete,
    []() {
        std::cout << "Step completed" << std::endl;
    });

// Execute
CTestSequence sequence;
// ... load sequence ...
auto result = executor.ExecuteSequence(sequence);
```

---

## Test Sequence & Steps

### Test Sequence

**Header:** `src/core/test_sequence/TestSequence.h`

```cpp
class CTestSequence {
public:
    CTestSequence();
    explicit CTestSequence(const STestInfo& info);

    // Test information
    const STestInfo& GetInfo() const;
    void SetInfo(const STestInfo& info);

    // Step management
    void AddStep(TSharedPtr<ITestStep> step);
    void InsertStep(TUInt32 index, TSharedPtr<ITestStep> step);
    void RemoveStep(TUInt32 index);
    TSharedPtr<ITestStep> GetStep(TUInt32 index) const;
    TUInt32 GetStepCount() const;
    void ClearSteps();

    // Sequence operations
    CResult<void> Validate() const;
};
```

### Test Step Interface

**Header:** `src/core/test_sequence/ITestStep.h`

```cpp
class ITestStep {
public:
    virtual ~ITestStep() = default;

    // Identification
    virtual TString GetId() const = 0;
    virtual TString GetName() const = 0;
    virtual TString GetType() const = 0;

    // Execution
    virtual CResult<void> Execute(SStepResult& result) = 0;
    virtual CResult<void> Abort() = 0;

    // Configuration
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
};
```

#### Step Result Structure

```cpp
struct SStepResult {
    TString stepId;              // Step identifier
    TString stepName;            // Step name
    ETestVerdict verdict;        // Pass/Fail/Error/Skipped
    TString message;             // Result message
    TDouble measuredValue;       // Measured value (if applicable)
    TString unit;                // Unit of measurement
    TTimePoint startTime;        // When step started
    TTimePoint endTime;          // When step ended
    TDuration durationMs;        // Execution duration
    TMap<TString, TString> data; // Additional data
};
```

#### Test Verdict Enum

```cpp
enum class ETestVerdict : TUInt8 {
    kPass,     // Test passed
    kFail,     // Test failed
    kError,    // Error occurred
    kSkipped   // Test was skipped
};
```

**Example:**
```cpp
CTestSequence sequence;
STestInfo info;
info.id = "TEST-001";
info.name = "My Test";
sequence.SetInfo(info);

// Add steps
sequence.AddStep(std::make_shared<CWaitStep>("WAIT-001", "Delay", 100));
sequence.AddStep(std::make_shared<CLimitCheckStep>("LIMIT-001", "Check"));

// Validate and execute
auto validationResult = sequence.Validate();
if (validationResult.IsSuccess()) {
    CTestExecutor executor;
    executor.ExecuteSequence(sequence);
}
```

---

## Threading & Synchronization

### Thread Pool

**Header:** `src/core/threading/ThreadPool.h`

```cpp
class CThreadPool {
public:
    explicit CThreadPool(TUInt32 numThreads);
    ~CThreadPool();

    // Task submission
    template<typename F, typename... Args>
    auto Submit(F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    // Control
    void Start();
    void Stop();
    void WaitForAll();

    // Status
    TUInt32 GetThreadCount() const;
    TUInt32 GetQueueSize() const;
    bool IsRunning() const;
};
```

**Example:**
```cpp
CThreadPool pool(4);  // 4 worker threads
pool.Start();

// Submit tasks
auto future1 = pool.Submit([]() { return 42; });
auto future2 = pool.Submit([](int x, int y) { return x + y; }, 10, 20);

// Wait for results
int result1 = future1.get();  // 42
int result2 = future2.get();  // 30

pool.Stop();
```

### Synchronization Point

**Header:** `src/core/threading/SyncPoint.h`

Barrier for coordinating multiple sockets.

```cpp
class CSyncPoint {
public:
    explicit CSyncPoint(TUInt32 count);

    // Wait for all participants
    bool WaitFor(TDuration timeout = TDuration(30000));

    // Reset for reuse
    void Reset(TUInt32 count);

    // Status
    TUInt32 GetWaitingCount() const;
    TUInt32 GetRequiredCount() const;
};
```

**Example:**
```cpp
CSyncPoint syncPoint(4);  // 4 sockets must synchronize

// In each socket thread:
void SocketThread(TUInt32 socketId, CSyncPoint& sync) {
    // Do independent work
    DoWork(socketId);

    // Wait for all sockets
    if (!sync.WaitFor(std::chrono::seconds(30))) {
        std::cerr << "Sync timeout!" << std::endl;
    }

    // Continue together
    DoSynchronizedWork(socketId);
}
```

---

## Resource Scheduling

### Header: `src/core/scheduling/ResourceScheduler.h`

Manages access to shared resources.

#### CResourceScheduler Class

```cpp
class CResourceScheduler {
public:
    CResourceScheduler();

    // Resource registration
    void RegisterResource(const TString& resourceId, bool exclusive = true);
    void UnregisterResource(const TString& resourceId);

    // Allocation
    CResult<void> AllocateResource(
        const TString& resourceId,
        TUInt32 requesterId,
        TDuration timeout = TDuration(5000)
    );

    CResult<void> ReleaseResource(
        const TString& resourceId,
        TUInt32 requesterId
    );

    // Batch allocation
    CResult<void> AllocateResources(
        const TVector<TString>& resourceIds,
        TUInt32 requesterId,
        TDuration timeout = TDuration(5000)
    );

    // Status
    bool IsResourceAvailable(const TString& resourceId) const;
    TOptional<TUInt32> GetResourceOwner(const TString& resourceId) const;
};
```

**Example:**
```cpp
CResourceScheduler scheduler;

// Register resources
scheduler.RegisterResource("DMM-001", true);   // Exclusive
scheduler.RegisterResource("SCOPE-001", true);

// Socket 0 requests DMM
auto result = scheduler.AllocateResource("DMM-001", 0);
if (result.IsSuccess()) {
    // Use DMM
    UseDMM();

    // Release when done
    scheduler.ReleaseResource("DMM-001", 0);
}

// Batch allocation
TVector<TString> resources = {"DMM-001", "SCOPE-001"};
result = scheduler.AllocateResources(resources, 1);
```

---

## See Also

- [Test Sequence API](TEST_SEQUENCE_API.md) - Detailed sequence management
- [Plugin API](PLUGIN_API.md) - Plugin development interfaces
- [Database API](DATABASE_API.md) - Data storage
- [Configuration API](CONFIGURATION_API.md) - Settings management
- [Examples](../../examples/) - Code examples

---

*Last updated: 2025-11-23 | TestMATE v2.0*
