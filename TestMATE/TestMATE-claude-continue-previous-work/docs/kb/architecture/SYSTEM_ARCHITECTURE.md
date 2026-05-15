# TestMATE System Architecture

**Comprehensive overview of TestMATE's design and architecture**

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Process Models](#process-models)
4. [Execution Flow](#execution-flow)
5. [Threading Model](#threading-model)
6. [Resource Management](#resource-management)
7. [Data Flow](#data-flow)
8. [Plugin Architecture](#plugin-architecture)
9. [Database Layer](#database-layer)
10. [Design Patterns](#design-patterns)

---

## Architecture Overview

### High-Level Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                    Application Layer                           │
│  ┌──────────────────┐              ┌──────────────────┐       │
│  │   Qt GUI App     │              │  Custom Apps     │       │
│  └────────┬─────────┘              └────────┬─────────┘       │
└───────────┼──────────────────────────────────┼────────────────┘
            │                                  │
┌───────────▼──────────────────────────────────▼────────────────┐
│                        API Facade                              │
│              (ITestMATEApi - Clean interface)                  │
└──────┬──────────────┬──────────────┬──────────────┬───────────┘
       │              │              │              │
┌──────▼──────┐ ┌────▼──────┐ ┌────▼──────┐ ┌─────▼──────┐
│    Core     │ │ Database  │ │    UI     │ │ Utilities  │
│   Engine    │ │   Layer   │ │  Adapter  │ │            │
│             │ │           │ │           │ │            │
│ • Process   │ │ • SQLite  │ │ • Adapter │ │ • Logging  │
│   Models    │ │ • Postgres│ │   Pattern │ │ • Strings  │
│ • Executor  │ │ • MySQL   │ │           │ │ • Profiler │
│ • Threading │ │ • Factory │ │           │ │            │
│ • Scheduler │ │           │ │           │ │            │
│ • Plugins   │ │           │ │           │ │            │
│ • Reports   │ │           │ │           │ │            │
└─────────────┘ └───────────┘ └───────────┘ └────────────┘
```

### Design Principles

1. **Modularity** - Clear separation of concerns
2. **Extensibility** - Plugin architecture for customization
3. **Performance** - Multi-threaded parallel execution
4. **Reliability** - Comprehensive error handling and recovery
5. **Testability** - 261 unit tests, 7 integration tests
6. **Portability** - Cross-platform (Windows, Linux, macOS)
7. **Maintainability** - Clean code, SDG guidelines

---

## Core Components

### Component Diagram

```
testmate_core/
├── process_models/          # Execution strategies
│   ├── IProcessModel       # Interface
│   ├── SequentialModel     # One device, linear
│   ├── ParallelModel       # Multi-socket parallel
│   └── BatchModel          # Group processing
│
├── execution/              # Test execution
│   ├── TestExecutor        # Orchestrates execution
│   └── ExecutionContext    # Per-socket state
│
├── test_sequence/          # Test definitions
│   ├── TestSequence        # Container for steps
│   ├── ITestStep           # Step interface
│   └── SequenceFileIO      # JSON/XML I/O
│
├── threading/              # Concurrency
│   ├── ThreadPool          # Worker threads
│   ├── SyncPoint           # Barriers
│   └── Barrier             # Synchronization
│
├── scheduling/             # Resource mgmt
│   └── ResourceScheduler   # Fair allocation
│
├── plugins/                # Extensibility
│   ├── PluginManager       # Dynamic loading
│   └── IPlugin             # Plugin interfaces
│
├── instruments/            # Hardware abstraction
│   ├── InstrumentManager   # Registry
│   └── IInstrument         # Device interface
│
├── communication/          # I/O protocols
│   ├── IConnection         # Protocol interface
│   └── SerialConnection    # Serial/UART
│
├── config/                 # Configuration
│   └── ConfigManager       # Settings management
│
├── reporting/              # Test reports
│   ├── IReportGenerator    # Report interface
│   └── HtmlReportGenerator # HTML output
│
└── semiconductor/          # IC testing
    ├── DeviceHandler       # DUT management
    └── StdfWriter          # STDF format
```

### Component Responsibilities

| Component | Responsibility |
|-----------|----------------|
| **ProcessModel** | Defines execution strategy (sequential/parallel/batch) |
| **TestExecutor** | Orchestrates test execution using a process model |
| **ExecutionContext** | Maintains state for a single test instance |
| **TestSequence** | Container for ordered test steps |
| **ITestStep** | Interface for individual test operations |
| **ThreadPool** | Manages worker threads for parallel execution |
| **ResourceScheduler** | Allocates shared resources (instruments, sockets) |
| **PluginManager** | Loads and manages plugin lifecycle |
| **InstrumentManager** | Registry and factory for instrument drivers |
| **ConfigManager** | Hierarchical configuration storage |
| **ReportGenerator** | Creates test reports in various formats |

---

## Process Models

### Process Model Hierarchy

```
                 IProcessModel (Interface)
                        │
          ┌─────────────┼─────────────┐
          │             │             │
    SequentialModel  ParallelModel  BatchModel
```

### Sequential Model Architecture

```
┌─────────────────────────────────────┐
│      Sequential Process Model        │
│                                      │
│  ┌────────────────────────────────┐ │
│  │   Single Execution Context     │ │
│  │   • Socket ID = 0              │ │
│  │   • Single thread              │ │
│  │   • Linear step execution      │ │
│  └────────────────────────────────┘ │
│                                      │
│  Execution Flow:                     │
│  Step 1 → Step 2 → Step 3 → ...     │
│                                      │
└──────────────────────────────────────┘
```

### Parallel Model Architecture

```
┌──────────────────────────────────────────────────────────┐
│           Parallel Process Model (N Sockets)             │
│                                                          │
│  ┌─────────────┐  ┌─────────────┐       ┌────────────┐ │
│  │  Socket 0   │  │  Socket 1   │  ...  │  Socket N  │ │
│  │             │  │             │       │            │ │
│  │  Context 0  │  │  Context 1  │       │  Context N │ │
│  │  Thread 0   │  │  Thread 1   │       │  Thread N  │ │
│  │  Steps 1-M  │  │  Steps 1-M  │       │  Steps 1-M │ │
│  └─────────────┘  └─────────────┘       └────────────┘ │
│         │                │                      │       │
│         └────────────────┼──────────────────────┘       │
│                          │                              │
│                   Resource Scheduler                    │
│              (Manages shared resources)                 │
└──────────────────────────────────────────────────────────┘

Synchronization Points:
  • Sockets can synchronize at specific steps
  • Resource allocation coordinates access
  • Independent completion times
```

### Batch Model Architecture

```
┌──────────────────────────────────────────────────────────┐
│              Batch Process Model                         │
│                                                          │
│  Shared Setup (once for all devices)                    │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Initialize test fixtures                          │ │
│  │  Allocate shared resources                         │ │
│  └────────────────────────────────────────────────────┘ │
│                          │                              │
│  ┌──────────────────────┼──────────────────────────┐   │
│  │                      │                          │   │
│  │  Device 1       Device 2  ...    Device N       │   │
│  │  (parallel or sequential per-device tests)      │   │
│  └────────────────────────────────────────────────┘    │
│                          │                              │
│  Shared Cleanup (once for all devices)                 │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Release shared resources                          │ │
│  │  Generate aggregate report                         │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

---

## Execution Flow

### Complete Execution Sequence

```
┌─────────────────────────────────────────────────────────────┐
│                    Test Execution Flow                      │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 1. INITIALIZATION                                           │
│    • Load test sequence from file or construct in code      │
│    • Validate sequence (check all steps valid)              │
│    • Select process model (Sequential/Parallel/Batch)       │
│    • Initialize process model                               │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 2. PRE-EXECUTION                                            │
│    • Allocate required resources                            │
│    • Create execution contexts (one per socket)             │
│    • Initialize instruments                                 │
│    • Setup callbacks (onStepComplete, onError, etc.)        │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 3. EXECUTION (Process Model Dependent)                      │
│                                                             │
│ Sequential:                Parallel:                        │
│   For each step:             For each socket (parallel):   │
│     Execute step               For each step:              │
│     Store result                 Execute step              │
│     Check verdict                Store result              │
│                                  Synchronize if needed     │
│                                                             │
│ Batch:                                                      │
│   Execute shared setup                                     │
│   For each device (parallel/seq):                          │
│     Execute per-device steps                               │
│   Execute shared cleanup                                   │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 4. POST-EXECUTION                                           │
│    • Aggregate results from all contexts                    │
│    • Release allocated resources                            │
│    • Close instrument connections                           │
│    • Calculate overall verdict                              │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 5. REPORTING                                                │
│    • Generate test report (HTML, STDF, etc.)                │
│    • Store results in database                              │
│    • Invoke user callbacks (onComplete)                     │
│    • Return final status                                    │
└─────────────────────────────────────────────────────────────┘
```

### Step Execution Detail

```
┌────────────────────────────────────────────┐
│         Single Step Execution              │
└────────────────────────────────────────────┘
                 │
                 ▼
┌────────────────────────────────────────────┐
│ 1. Pre-execution                           │
│    • Check if step enabled                 │
│    • Record start time                     │
│    • Invoke OnStepStart callback           │
└──────────────┬─────────────────────────────┘
               ▼
┌────────────────────────────────────────────┐
│ 2. Execution                               │
│    • Call step->Execute(result)            │
│    • Handle any errors/exceptions          │
│    • Support abort if requested            │
└──────────────┬─────────────────────────────┘
               ▼
┌────────────────────────────────────────────┐
│ 3. Post-execution                          │
│    • Record end time, duration             │
│    • Store result in context               │
│    • Invoke OnStepComplete callback        │
│    • Update progress                       │
└──────────────┬─────────────────────────────┘
               ▼
┌────────────────────────────────────────────┐
│ 4. Verdict Handling                        │
│    • If Pass → Continue                    │
│    • If Fail → Log, optionally abort       │
│    • If Error → Invoke OnError, abort      │
│    • If Skipped → Continue                 │
└────────────────────────────────────────────┘
```

---

## Threading Model

### Thread Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                         Main Thread                            │
│  • UI event loop (if Qt GUI)                                   │
│  • Process model coordination                                  │
│  • Callback invocation                                         │
└─────────────────┬──────────────────────────────────────────────┘
                  │
                  ▼
┌────────────────────────────────────────────────────────────────┐
│                    Thread Pool (N threads)                     │
│                                                                │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐ │
│  │  Worker 1 │  │  Worker 2 │  │  Worker 3 │  │  Worker N │ │
│  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘ │
│        │              │              │              │        │
│        └──────────────┼──────────────┼──────────────┘        │
│                       │              │                       │
│                  ┌────▼──────────────▼────┐                  │
│                  │     Task Queue         │                  │
│                  │  (Thread-safe queue)   │                  │
│                  └────────────────────────┘                  │
└────────────────────────────────────────────────────────────────┘

Thread Safety:
  • Task queue protected by mutex
  • Each execution context is thread-local
  • Shared resources use locks (ResourceScheduler)
  • Atomic operations for counters
```

### Synchronization Primitives

```
┌────────────────────────────────────────────────────────────┐
│                  Synchronization Tools                     │
└────────────────────────────────────────────────────────────┘

1. Mutex (QMutex)
   • Protects shared data structures
   • RAII lock guards (std::lock_guard)

2. Read-Write Lock (QReadWriteLock)
   • Multiple readers, single writer
   • Used for configuration access

3. Semaphore (QSemaphore)
   • Limits concurrent access
   • Resource counting

4. Sync Point (Custom)
   • Barrier for socket synchronization
   • Timeout support

5. Condition Variable
   • Thread notification
   • Wait for events

Example - Sync Point:
┌─────────┐   ┌─────────┐   ┌─────────┐
│Socket 0 │   │Socket 1 │   │Socket 2 │
└────┬────┘   └────┬────┘   └────┬────┘
     │             │             │
     │   Step 1    │   Step 1    │  Step 1
     ├─────────────┼─────────────┼─────────
     │   Step 2    │   Step 2    │  Step 2
     ├─────────────┼─────────────┼─────────
     │             │             │
     └─────────────┴─────────────┴──────► Sync Point
                                          (All wait here)
                   ▼
     ┌─────────────┬─────────────┬─────────
     │   Step 3    │   Step 3    │  Step 3
     │  (synced)   │  (synced)   │ (synced)
```

---

## Resource Management

### Resource Scheduler Architecture

```
┌──────────────────────────────────────────────────────────┐
│              Resource Scheduler                          │
│                                                          │
│  Resource Registry                                       │
│  ┌────────────────────────────────────────────────────┐ │
│  │ DMM-001:  Owner=None,    State=Available          │ │
│  │ SCOPE-01: Owner=Socket2, State=Allocated          │ │
│  │ PSU-001:  Owner=Socket1, State=Allocated          │ │
│  │ LOAD-01:  Owner=None,    State=Available          │ │
│  └────────────────────────────────────────────────────┘ │
│                                                          │
│  Wait Queue (per resource)                              │
│  ┌────────────────────────────────────────────────────┐ │
│  │ DMM-001:  [Socket3, Socket4]                      │ │
│  │ SCOPE-01: [Socket0]                               │ │
│  └────────────────────────────────────────────────────┘ │
│                                                          │
│  Allocation Algorithm                                   │
│  • Fair scheduling (FIFO with priority)                 │
│  • Deadlock detection                                   │
│  • Timeout handling                                     │
└──────────────────────────────────────────────────────────┘
```

### Resource Allocation Flow

```
Socket requests resource
        │
        ▼
  Is available? ──Yes──► Allocate immediately
        │                        │
        No                       ▼
        │                  Return Success
        ▼
  Add to wait queue
        │
        ▼
  Wait with timeout
        │
    ┌───┴───┐
    │       │
Resource  Timeout
released  expired
    │       │
    ▼       ▼
Allocate  Return
         kTimeout
```

### Deadlock Detection

```
Detection Algorithm (Periodic):
1. Build resource dependency graph
2. Detect cycles (deadlock indicators)
3. If cycle found:
   a. Log deadlock participants
   b. Apply resolution strategy:
      • Timeout-based release
      • Priority-based preemption
      • Random victim selection

Example Deadlock:
Socket 0: Holds DMM-001, Wants SCOPE-01
Socket 1: Holds SCOPE-01, Wants DMM-001
          ↑                      ↓
          └──────────────────────┘
                  Cycle!
```

---

## Data Flow

### Test Data Flow

```
┌─────────────┐
│ Test Input  │ (JSON/XML file or C++ API)
└──────┬──────┘
       │
       ▼
┌──────────────────┐
│  Test Sequence   │ (Parsed, validated)
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│ Process Model    │ (Execution strategy)
└──────┬───────────┘
       │
       ▼
┌───────────────────────────────────────────────┐
│        Execution (per socket/device)          │
│                                               │
│  Step 1 → Measurement → 5.02V                │
│  Step 2 → Calculation → 12.5W                │
│  Step 3 → Limit Check → PASS                 │
│  ...                                          │
└──────┬────────────────────────────────────────┘
       │
       ▼
┌──────────────────┐
│ Step Results     │ (In-memory, per context)
└──────┬───────────┘
       │
       ├──────────┬──────────┬────────────┐
       ▼          ▼          ▼            ▼
┌──────────┐ ┌─────────┐ ┌────────┐ ┌─────────┐
│ Database │ │  Report │ │  Log   │ │ Callback│
│  (SQLite │ │  (HTML  │ │  File  │ │ Functions
│   /SQL)  │ │  /STDF) │ │        │ │         │
└──────────┘ └─────────┘ └────────┘ └─────────┘
```

### Variable Data Flow

```
Step Execution:
┌──────────────────────────────────────────────┐
│ Step 1: Measure Voltage                      │
│   Execute() → result.measuredValue = 5.02    │
│   Store in context: "voltage" = 5.02         │
└──────────────┬───────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────┐
│ Step 2: Measure Current                      │
│   Execute() → result.measuredValue = 2.48    │
│   Store in context: "current" = 2.48         │
└──────────────┬───────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────┐
│ Step 3: Calculate Power                      │
│   Read from context: voltage = 5.02          │
│   Read from context: current = 2.48          │
│   Calculate: power = voltage * current       │
│   Store in context: "power" = 12.4496        │
└──────────────┬───────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────┐
│ Step 4: Validate Power                       │
│   Read from context: power = 12.4496         │
│   Check: 10.0 <= 12.4496 <= 15.0 → PASS     │
└──────────────────────────────────────────────┘
```

---

## Plugin Architecture

### Plugin System Design

```
┌────────────────────────────────────────────────────────┐
│                   Plugin Manager                       │
│                                                        │
│  ┌──────────────────────────────────────────────────┐ │
│  │          Plugin Registry                         │ │
│  │  ┌────────────────┐  ┌────────────────┐         │ │
│  │  │SimpleMultimeter│  │CustomMeasure  │  ...    │ │
│  │  │  (Instrument)  │  │  (TestStep)   │         │ │
│  │  └────────────────┘  └────────────────┘         │ │
│  └──────────────────────────────────────────────────┘ │
│                                                        │
│  Plugin Lifecycle:                                    │
│  1. Discovery (scan plugin directory)                 │
│  2. Load (dlopen/LoadLibrary)                        │
│  3. Validate (check API version)                      │
│  4. Initialize (call plugin Init())                   │
│  5. Register (add to registry)                        │
│  6. Use (create instances as needed)                  │
│  7. Unload (cleanup and dlclose)                      │
└────────────────────────────────────────────────────────┘
```

### Plugin Types

```
IPlugin (Base Interface)
    │
    ├── IInstrumentPlugin
    │   • Connect/Disconnect
    │   • Send/Receive commands
    │   • Measure functions
    │
    ├── ITestStepPlugin
    │   • Execute test operation
    │   • Parameter validation
    │   • Result generation
    │
    ├── IReportGeneratorPlugin
    │   • Format test results
    │   • Generate report files
    │
    ├── IDataStorePlugin
    │   • Database operations
    │   • Query interface
    │
    ├── IProcessModelPlugin
    │   • Custom execution strategy
    │
    └── IConnectionPlugin
        • Custom communication protocol
```

---

## Database Layer

### Database Architecture

```
┌────────────────────────────────────────────────────────┐
│               Data Store Factory                       │
│  (Creates appropriate database backend)                │
└─────────────┬──────────────────────────────────────────┘
              │
     ┌────────┼────────┬─────────────┐
     ▼        ▼        ▼             ▼
┌─────────┐ ┌──────┐ ┌──────┐  ┌─────────┐
│ SQLite  │ │ Postgre│ │ MySQL│  │ Custom  │
│DataStore│ │  SQL   │ │/Maria│  │ Plugin  │
└─────────┘ └────────┘ └──DB──┘  └─────────┘
     │          │         │           │
     └──────────┴─────────┴───────────┘
                  │
                  ▼
         IDataStore Interface
         • StoreTestData()
         • GetTestDataById()
         • GetTestDataByLot()
         • Query()
         • BeginTransaction()
         • CommitTransaction()
```

### Database Schema

```sql
-- Main test data table
CREATE TABLE test_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    test_id TEXT NOT NULL,
    device_id TEXT,
    lot_number TEXT,
    wafer_number TEXT,
    result TEXT CHECK(result IN ('PASS', 'FAIL', 'ERROR', 'SKIPPED')),
    start_time TEXT,
    end_time TEXT,
    test_duration REAL,
    operator_id TEXT,
    test_program_version TEXT
);

-- Measurements table
CREATE TABLE measurements (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    test_data_id INTEGER,
    step_id TEXT,
    parameter_name TEXT,
    measured_value REAL,
    unit TEXT,
    limit_min REAL,
    limit_max REAL,
    verdict TEXT,
    FOREIGN KEY (test_data_id) REFERENCES test_data(id)
);

-- Indexes for performance
CREATE INDEX idx_test_id ON test_data(test_id);
CREATE INDEX idx_lot_number ON test_data(lot_number);
CREATE INDEX idx_device_id ON test_data(device_id);
CREATE INDEX idx_result ON test_data(result);
```

---

## Design Patterns

### Patterns Used

| Pattern | Where Used | Purpose |
|---------|------------|---------|
| **Singleton** | ConfigManager, LogManager, PluginManager | Single instance access |
| **Factory** | DataStoreFactory, ProcessModelFactory | Object creation abstraction |
| **Strategy** | IProcessModel implementations | Interchangeable algorithms |
| **Observer** | Callback system | Event notification |
| **Template Method** | TestStepBase | Define algorithm skeleton |
| **Adapter** | UIAdapter | Interface compatibility |
| **Repository** | IDataStore | Data access abstraction |
| **RAII** | Resource locks, ProfileScope | Automatic cleanup |
| **Command** | Test steps | Encapsulate operations |
| **Facade** | ITestMATEApi | Simplified interface |

### Example: Strategy Pattern (Process Models)

```cpp
// Strategy interface
class IProcessModel {
public:
    virtual CResult<void> Execute(CTestSequence& seq) = 0;
};

// Concrete strategies
class CSequentialModel : public IProcessModel { ... };
class CParallelModel : public IProcessModel { ... };
class CBatchModel : public IProcessModel { ... };

// Context
class CTestExecutor {
    TSharedPtr<IProcessModel> m_pModel;
public:
    void SetProcessModel(TSharedPtr<IProcessModel> model) {
        m_pModel = model;
    }
    CResult<void> ExecuteSequence(CTestSequence& seq) {
        return m_pModel->Execute(seq);
    }
};
```

---

## Performance Considerations

### Optimization Strategies

1. **Thread Pool Reuse**
   - Worker threads persist across tests
   - Minimizes thread creation overhead

2. **Database Batching**
   - Use transactions for bulk inserts
   - Reduces I/O and improves throughput

3. **Lock-Free Data Structures**
   - Atomic operations where possible
   - Reduces contention

4. **Resource Pre-allocation**
   - Pre-create execution contexts
   - Avoid allocations during execution

5. **Lazy Initialization**
   - Plugins loaded on-demand
   - Reduces startup time

---

## See Also

- [Process Models](PROCESS_MODELS.md) - Detailed process model design
- [Threading](THREADING.md) - Concurrency architecture
- [Resource Management](RESOURCE_MANAGEMENT.md) - Scheduling algorithms
- [Plugin System](PLUGIN_SYSTEM.md) - Extensibility details
- [Data Flow](DATA_FLOW.md) - Data movement and storage

---

*Last updated: 2025-11-23 | TestMATE v2.0*
