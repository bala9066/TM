# TestMATE System Architecture Document

Version 1.0
Author: TestMATE Development Team
Date: 2025-01-XX

---

## 1. System Overview

### 1.1 Introduction

TestMATE (Test Management and Automation Tool Environment) is a professional-grade, cross-platform test management and automation system designed for hardware and software testing, with special emphasis on semiconductor testing capabilities.

### 1.2 High-Level Architecture

```
+------------------------------------------------------------------+
|                         TestMATE Application                      |
+------------------------------------------------------------------+
|                                                                   |
|  +------------------------+    +-----------------------------+    |
|  |      UI Layer (Qt)     |    |    Remote Services (gRPC)   |    |
|  |  - MainWindow          |    |    - TestExecutionService   |    |
|  |  - TestPlanEditor      |    |    - ReportService          |    |
|  |  - PinMapEditor        |    |    - ConfigService          |    |
|  |  - ReportDesigner      |    +-----------------------------+    |
|  +----------+-------------+                                       |
|             |                                                     |
|             v                                                     |
|  +------------------------+                                       |
|  |    Qt Adapters Layer   |                                       |
|  |  (UI-Core Translation) |                                       |
|  +----------+-------------+                                       |
|             |                                                     |
+-------------v-----------------------------------------------------+
|                         API/Facade Layer                          |
|  +---------------+  +---------------+  +---------------+          |
|  |ITestExecution|  |ITestPlanAPI   |  |IReportingAPI  |          |
|  |    API       |  |               |  |               |          |
|  +---------------+  +---------------+  +---------------+          |
+------------------------------------------------------------------+
|                        Core Layer (Pure C++)                      |
|  +----------------------------------------------------------------+
|  |                     TestMATECore (Singleton)                   |
|  +----------------------------------------------------------------+
|  |                                                                |
|  |  +------------------+  +------------------+  +---------------+ |
|  |  | Process Models   |  | Thread Pool      |  | Resource      | |
|  |  | - Sequential     |  | - Task Queue     |  | Scheduler     | |
|  |  | - Parallel       |  | - Workers        |  | - Allocation  | |
|  |  | - Batch          |  | - Sync Points    |  | - Deadlock    | |
|  |  +------------------+  +------------------+  +---------------+ |
|  |                                                                |
|  |  +------------------+  +------------------+  +---------------+ |
|  |  | Plugin Manager   |  | Config Manager   |  | License Mgr   | |
|  |  | - TestSteps      |  | - Profiles       |  | - Validation  | |
|  |  | - Instruments    |  | - Station        |  | - Feature Gate| |
|  |  | - DUTs           |  | - Environment    |  +---------------+ |
|  |  +------------------+  +------------------+                    |
|  |                                                                |
|  |  +------------------+  +------------------+  +---------------+ |
|  |  | Test Execution   |  | Result Manager   |  | Event System  | |
|  |  | Engine           |  | - Storage        |  | - Publisher   | |
|  |  | - Context        |  | - Query          |  | - Subscribers | |
|  |  | - Callbacks      |  | - Export         |  +---------------+ |
|  |  +------------------+  +------------------+                    |
|  +----------------------------------------------------------------+
+------------------------------------------------------------------+
|                      Data/Models Layer                            |
|  +---------------+  +---------------+  +---------------+          |
|  | TestPlan      |  | TestResult    |  | Component     |          |
|  | TestSequence  |  | Measurement   |  | Instrument    |          |
|  | TestStep      |  | Statistics    |  | DUT           |          |
|  +---------------+  +---------------+  +---------------+          |
+------------------------------------------------------------------+
|                     Infrastructure Layer                          |
|  +---------------+  +---------------+  +---------------+          |
|  | Database      |  | Scripting     |  | Reporting     |          |
|  | (PostgreSQL)  |  | (Python)      |  | Engine        |          |
|  +---------------+  +---------------+  +---------------+          |
+------------------------------------------------------------------+
```

### 1.3 Key Architectural Principles

1. **GUI-Core Separation**: Core business logic MUST NOT depend on Qt or any UI framework
2. **Plugin-Based Extensibility**: Test steps, instruments, and DUTs are plugins
3. **Configuration-Driven**: Devices and behaviors configurable via XML/JSON
4. **Cross-Platform**: Windows, Linux, macOS support
5. **Thread-Safe by Design**: All shared resources protected
6. **Pure C++17/20**: No .NET, no COM dependencies

---

## 2. Architectural Patterns

### 2.1 Singleton Pattern

Used for system-wide managers:
- `CTestMATECore` - Main application core
- `CLogManager` - Centralized logging
- `CConfigManager` - Configuration management
- `CLicenseManager` - License validation

### 2.2 Factory Pattern

Used for object creation:
- `CProcessModelFactory` - Creates process model instances
- `CTestStepFactory` - Creates test step instances
- `CDeviceFactory` - Creates instrument/DUT instances
- `CReportFactory` - Creates report generators

### 2.3 Observer Pattern

Used for event-driven communication (NO Qt signals in core):
- `IEventPublisher` / `IEventSubscriber` interfaces
- Event types: ExecutionEvents, ResultEvents, ResourceEvents
- Core components publish events, UI adapters subscribe

### 2.4 Facade Pattern

API layer provides simplified interfaces to complex subsystems:
- `ITestExecutionAPI` - Test execution operations
- `ITestPlanAPI` - Test plan management
- `IResourceAPI` - Resource management
- `IReportingAPI` - Report generation

### 2.5 Strategy Pattern

Interchangeable algorithms:
- Process models (Sequential, Parallel, Batch)
- Scheduling algorithms (FIFO, Priority, Fair)
- Report formats (PDF, HTML, XML, ATML)

### 2.6 Command Pattern

Test step execution:
- Each test step encapsulates execution logic
- Supports undo/redo for editor operations
- Enables step queuing and scheduling

---

## 3. Core Components

### 3.1 TestMATECore (Singleton)

The central orchestrator responsible for:
- System initialization sequence
- Subsystem lifecycle management
- Global state coordination
- Shutdown cleanup

```cpp
class CTestMATECore {
public:
    static CTestMATECore* GetInstance();

    // Lifecycle
    EResult Initialize(const std::string& in_strConfigPath);
    void Shutdown();

    // Subsystem access
    CPluginManager* GetPluginManager();
    CResourceScheduler* GetResourceScheduler();
    CProcessModelFactory* GetProcessModelFactory();
    CConfigManager* GetConfigManager();
    CLicenseManager* GetLicenseManager();
    CEventPublisher* GetEventPublisher();
};
```

### 3.2 Process Model Framework

Supports multiple execution models:

```
IProcessModel (Interface)
    |
    +-- CProcessModelBase (Abstract)
            |
            +-- CSequentialModel (Single-device testing)
            +-- CParallelModel (Multi-socket parallel testing)
            +-- CBatchModel (Batch/lot testing)
```

**Key Requirements**: REQ-PM-001 to REQ-PM-073

### 3.3 Plugin System

Three plugin categories:
1. **Test Steps** - Implement `ITestStep` interface
2. **Instruments** - Implement `IInstrument` interface
3. **DUTs** - Implement `IDut` interface

Plugin lifecycle:
- Discovery (scan plugin directories)
- Loading (dynamic library loading)
- Registration (factory registration)
- Instantiation (on-demand creation)

**Key Requirements**: REQ-TS-001 to REQ-TS-144

### 3.4 Resource Scheduler

Manages shared and exclusive resources:
- Resource allocation/deallocation
- Deadlock detection and prevention
- Fair scheduling to prevent starvation
- Resource sets for atomic allocation

**Key Requirements**: REQ-RES-001 to REQ-RES-039

### 3.5 Test Execution Engine

Responsible for:
- Execution context management
- Step lifecycle (PreRun, Run, PostRun)
- Callback invocation
- Result collection and publishing

---

## 4. Technology Stack

### 4.1 Core Technologies

| Component | Technology | Version | Purpose |
|-----------|-----------|---------|---------|
| Language | C++ | 17/20 | Core implementation |
| UI Framework | Qt | 6.2+ | GUI only |
| Build System | CMake | 3.21+ | Cross-platform builds |
| RPC | gRPC | Latest | Remote services |
| Database | PostgreSQL | 14+ | Primary database |
| Database | SQLite | 3.x | Embedded/offline |
| Scripting | Python | 3.8+ | Embedded scripting |

### 4.2 C++17/20 Features Used

- `std::optional` - Optional values
- `std::variant` - Type-safe unions
- `std::filesystem` - File operations
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- `std::string_view` - Non-owning string references
- Structured bindings
- Constexpr if
- Concepts (C++20) - Type constraints

### 4.3 Qt Usage (UI Only)

Qt classes allowed ONLY in `src/ui/` and `src/ui/adapters/`:
- QMainWindow, QWidget, QDialog
- QAbstractItemModel for data binding
- Qt signals/slots for UI events only
- QThread for UI background tasks

**STRICTLY FORBIDDEN in Core**:
- QString (use std::string)
- QList, QVector (use std::vector)
- QMap (use std::map/unordered_map)
- Any Q-prefixed class in business logic

---

## 5. Design Decisions

### 5.1 Why Qt for UI?

| Factor | Decision |
|--------|----------|
| Cross-platform | Native look on Windows, Linux, macOS |
| Mature ecosystem | Extensive widget library |
| Designer tools | Qt Designer for rapid UI development |
| Performance | Native rendering, OpenGL support |
| License | LGPL allows commercial use |

### 5.2 Threading Model

- **Main Thread**: UI event loop (Qt)
- **Core Thread Pool**: Configurable workers (2-64)
- **Per-Socket Threads**: One per test socket in parallel mode
- **Background Services**: Database, reporting, logging

Thread communication:
- Core: `std::mutex`, `std::condition_variable`
- Core-to-UI: Qt Adapters translate to Qt signals
- UI-to-Core: API calls (thread-safe)

### 5.3 Memory Management

- **Smart Pointers**: `std::unique_ptr` for ownership, `std::shared_ptr` for shared
- **RAII**: All resources acquired through constructors
- **No raw `new`/`delete`**: Factory functions return smart pointers
- **No global objects**: Use singletons with explicit initialization

### 5.4 Error Handling Strategy

```cpp
enum class EErrorSeverity {
    kFatal,     // System unusable, restart required
    kCritical,  // Major function impaired
    kError,     // Operation failed, recoverable
    kWarning,   // Potential issue
    kInfo       // Notable event
};

class CResult {
    EErrorCode m_eCode;
    std::string m_strMessage;
    std::source_location m_location;
};
```

Error codes are negative integers per SDG guidelines.

---

## 6. Module Dependencies

### 6.1 Dependency Rules

1. **No circular dependencies**
2. **Core never depends on UI**
3. **Models never depend on managers**
4. **Lower layers never depend on higher layers**

### 6.2 Dependency Graph

```
                    +------------+
                    |     UI     |
                    +-----+------+
                          |
                    +-----v------+
                    | Qt Adapters|
                    +-----+------+
                          |
                    +-----v------+
                    |    API     |
                    +-----+------+
                          |
          +---------------+---------------+
          |               |               |
    +-----v-----+   +-----v-----+   +-----v-----+
    |  Managers |   |  Engines  |   |  Services |
    +-----------+   +-----------+   +-----------+
          |               |               |
          +---------------+---------------+
                          |
                    +-----v------+
                    |   Models   |
                    +-----+------+
                          |
                    +-----v------+
                    |   Utils    |
                    +------------+
```

### 6.3 Build Order

1. `utils` - Utilities, logging, common types
2. `models` - Data models (TestPlan, TestResult, etc.)
3. `core` - Business logic, managers, engines
4. `api` - Facade interfaces and implementations
5. `plugins` - Plugin system and standard plugins
6. `ui/adapters` - Qt adapters
7. `ui` - Qt UI components
8. `remote` - gRPC services
9. `main` - Application entry point

---

## 7. Data Flow

### 7.1 Test Execution Flow

```
User Action (UI)
      |
      v
Qt Adapter (Thread-safe call)
      |
      v
ITestExecutionAPI.StartTest()
      |
      v
CTestExecutionEngine.Execute()
      |
      +---> CProcessModel.Start()
      |           |
      |           v
      |     CThreadPool.Submit(task)
      |           |
      |           v
      |     CTestStep.Run() [per socket]
      |           |
      |           v
      |     CResultManager.PublishResult()
      |           |
      +<----------+
      |
      v
CEventPublisher.Publish(ExecutionComplete)
      |
      v
Qt Adapter (receives event, emits Qt signal)
      |
      v
UI Update
```

### 7.2 Event Flow (Observer Pattern)

```cpp
// Core publishes events
m_pEventPublisher->Publish(EEventType::kStepComplete, stepData);

// Qt Adapter subscribes
class CExecutionAdapter : public IEventSubscriber {
    void OnEvent(EEventType type, const CEventData& data) override {
        if (type == EEventType::kStepComplete) {
            emit stepCompleted(convertToQt(data)); // Qt signal
        }
    }
};
```

---

## 8. Security Architecture

### 8.1 Authentication

- Local user database or LDAP/Active Directory
- Session tokens with configurable timeout
- Password hashing (bcrypt/Argon2)

### 8.2 Authorization (RBAC)

Roles: Administrator, Engineer, Technician, Operator, Viewer

### 8.3 Data Protection

- TLS 1.2+ for network communication
- Encrypted sensitive data at rest
- License file encryption

---

## 9. Performance Targets

| Metric | Target |
|--------|--------|
| Step execution overhead | < 5ms |
| Resource allocation (uncontested) | < 5ms |
| UI response time | < 100ms (95th percentile) |
| Parallel scaling (8 sockets) | < 10% overhead |
| Database query (typical) | < 100ms |
| Test plan loading (5000 steps) | < 2 seconds |

---

## 10. Cross-Platform Considerations

### 10.1 Platform Abstraction

```cpp
namespace Platform {
    std::filesystem::path GetAppDataPath();
    std::filesystem::path GetTempPath();
    std::string GetMachineId();
    bool IsAdmin();
}
```

### 10.2 Platform-Specific Code

- Isolated in `src/platform/` directory
- Compile-time selection via CMake
- Runtime detection where needed

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-01-XX | TestMATE Team | Initial architecture document |
