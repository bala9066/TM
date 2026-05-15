# Frequently Asked Questions (FAQ)

**Common questions about TestMATE**

---

## Table of Contents

1. [General Questions](#general-questions)
2. [Installation & Setup](#installation--setup)
3. [Building & Compilation](#building--compilation)
4. [Test Sequences](#test-sequences)
5. [Process Models](#process-models)
6. [Database](#database)
7. [Plugins & Extensions](#plugins--extensions)
8. [Qt GUI](#qt-gui)
9. [Performance](#performance)
10. [Licensing & Deployment](#licensing--deployment)

---

## General Questions

### Q: What is TestMATE?

**A:** TestMATE (Test Management and Automation Tool Environment) is a comprehensive C++20 framework for automated testing of electronic devices and systems. It provides:
- Multiple execution models (Sequential, Parallel, Batch)
- Plugin architecture for instruments and test steps
- Database integration (SQLite, PostgreSQL, MySQL)
- Qt-based GUI application
- Comprehensive reporting and analytics

### Q: What platforms does TestMATE support?

**A:** TestMATE is cross-platform and supports:
- **Linux** (Ubuntu 20.04+, Debian, Fedora, CentOS)
- **Windows** (Windows 10+, Server 2019+)
- **macOS** (10.15+, Big Sur, Monterey, Ventura)

### Q: What C++ standard is required?

**A:** TestMATE requires **C++20** or later. Supported compilers:
- GCC 10+
- Clang 12+
- MSVC 2019+ (Visual Studio 2019 or later)

### Q: Is TestMATE production-ready?

**A:** Yes! TestMATE v2.0 is production-ready with:
- 261 unit tests (100% pass rate)
- 7 integration tests
- Zero compilation warnings
- Comprehensive documentation
- Real-world tested architecture

### Q: Is TestMATE open source?

**A:** TestMATE is currently proprietary software. Check the LICENSE file for terms and conditions.

---

## Installation & Setup

### Q: What are the minimum dependencies?

**A:** Required dependencies:
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15 or higher
- SQLite3

Optional dependencies:
- Qt5 5.15+ or Qt6 (for GUI)
- PostgreSQL client library (for PostgreSQL support)
- MySQL client library (for MySQL support)

### Q: How do I install dependencies on Ubuntu?

**A:**
```bash
# Required
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev

# Optional - Qt5
sudo apt-get install qt5-default qttools5-dev

# Optional - PostgreSQL
sudo apt-get install libpq-dev

# Optional - MySQL
sudo apt-get install libmysqlclient-dev
```

### Q: How do I install dependencies on macOS?

**A:**
```bash
# Using Homebrew
brew install cmake sqlite

# Optional - Qt
brew install qt@6

# Optional - PostgreSQL
brew install postgresql

# Optional - MySQL
brew install mysql-client
```

### Q: How do I install dependencies on Windows?

**A:**
- Install Visual Studio 2019 or later with C++ tools
- Download and install CMake from https://cmake.org
- SQLite is typically bundled with the build
- For Qt: Download from https://www.qt.io

---

## Building & Compilation

### Q: How do I build TestMATE?

**A:**
```bash
git clone https://github.com/sathishk35/TestMATE.git
cd TestMATE
mkdir build && cd build
cmake ..
cmake --build . -j4
```

### Q: Build fails with "C++20 required" error. What should I do?

**A:** Update your compiler:
- **GCC:** `sudo apt-get install g++-10` (or higher)
- **Clang:** `sudo apt-get install clang-12` (or higher)
- **MSVC:** Install Visual Studio 2019 or later

Then specify the compiler:
```bash
cmake .. -DCMAKE_CXX_COMPILER=g++-10
```

### Q: How do I build in Release mode?

**A:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Q: How do I build with Qt support?

**A:**
```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build .
```

On macOS with Homebrew Qt6:
```bash
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
```

### Q: CMake can't find SQLite3. How do I fix this?

**A:** Install SQLite development files:
```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# macOS
brew install sqlite

# Windows
# Download from https://www.sqlite.org/download.html
```

### Q: How do I enable verbose build output?

**A:**
```bash
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build .
```

### Q: Build succeeds but tests fail to link. What's wrong?

**A:** Ensure GoogleTest is properly installed or CMake is downloading it correctly. Check:
```bash
cmake .. -DBUILD_TESTING=ON
```

---

## Test Sequences

### Q: What file formats are supported for test sequences?

**A:** TestMATE supports:
- **JSON** (.json) - Recommended, human-readable
- **XML** (.xml) - Supported for compatibility

### Q: How do I create a test sequence?

**A:** Three ways:
1. **Programmatically in C++:**
```cpp
CTestSequence sequence;
sequence.AddStep(std::make_shared<CWaitStep>("WAIT-001", "Delay", 100));
```

2. **JSON file:**
```json
{
  "id": "TEST-001",
  "name": "My Test",
  "steps": [
    {"id": "WAIT-001", "name": "Delay", "type": "wait"}
  ]
}
```

3. **Qt GUI:** Use the visual sequence editor

### Q: How do I load a test sequence from a file?

**A:**
```cpp
auto& fileIO = CSequenceFileIO::GetInstance();
CTestSequence sequence;
auto result = fileIO.LoadSequence("test.json", sequence);
```

### Q: Can I use variables in test sequences?

**A:** Yes! Use `${variable}` syntax:
```json
{
  "parameters": {
    "value": "${MEAS-001.voltage}"
  }
}
```

### Q: How many steps can a sequence have?

**A:** Tested with 10,000+ steps. Practical limit depends on available memory.

---

## Process Models

### Q: What's the difference between Sequential, Parallel, and Batch models?

**A:**
- **Sequential:** Tests one device at a time, steps execute in order
- **Parallel:** Tests multiple devices simultaneously (up to 32 sockets)
- **Batch:** Tests multiple devices with shared setup/cleanup

### Q: How do I choose which process model to use?

**A:** Decision tree:
- **One device?** → Sequential
- **Multiple devices, independent tests?** → Parallel
- **Multiple devices, shared resources?** → Batch

### Q: How many parallel sockets are supported?

**A:** Up to 32 sockets. Tested efficiently with 8-16 sockets.

### Q: Can I switch process models at runtime?

**A:** Yes:
```cpp
CTestExecutor executor;
auto model = std::make_shared<CParallelModel>(4);
executor.SetProcessModel(model);
```

### Q: What happens if one socket fails in parallel mode?

**A:** By default, other sockets continue. You can configure "abort all on failure" mode.

---

## Database

### Q: Which databases are supported?

**A:**
- **SQLite** (default) - Embedded, zero-configuration
- **PostgreSQL** - Enterprise-grade relational database
- **MySQL/MariaDB** - Popular open-source database

### Q: How do I choose a database backend?

**A:**
```cpp
auto& factory = CDataStoreFactory::GetInstance();

// SQLite
auto db = factory.CreateDataStore(EDatabaseType::kSqlite, "results.db");

// PostgreSQL
auto db = factory.CreateDataStore(
    EDatabaseType::kPostgreSql,
    "host=localhost dbname=testmate user=admin"
);

// MySQL
auto db = factory.CreateDataStore(
    EDatabaseType::kMySql,
    "host=localhost;database=testmate;user=admin"
);
```

### Q: Where is the SQLite database file stored?

**A:** By default in the application directory as `testmate.db`. Configurable:
```cpp
db = factory.CreateDataStore(EDatabaseType::kSqlite, "/path/to/db.sqlite");
```

### Q: How do I backup/restore a database?

**A:** Use the provided tools in `examples/database/`:
```bash
# Backup
./database_backup_tool source.db backup.sql

# Restore
./database_restore_tool backup.sql restored.db
```

### Q: Can I use custom SQL queries?

**A:** Yes:
```cpp
TString sql = "SELECT * FROM test_data WHERE result = 'PASS'";
auto results = db->Query(sql);
```

### Q: How do I migrate from SQLite to PostgreSQL?

**A:** Use the migration tool:
```bash
./database_migration_tool --from sqlite:old.db --to postgresql:connection_string
```

---

## Plugins & Extensions

### Q: What types of plugins are supported?

**A:**
1. **Instrument Plugins** - Hardware drivers (DMMs, scopes, power supplies)
2. **Test Step Plugins** - Custom test operations
3. **Report Generator Plugins** - Custom report formats
4. **Data Store Plugins** - Custom database backends
5. **Process Model Plugins** - Custom execution strategies
6. **Connection Plugins** - Custom communication protocols

### Q: How do I create a custom plugin?

**A:** See the [Plugin Development Guide](../../../examples/custom_plugin/README.md). Basic steps:
1. Inherit from appropriate plugin interface (e.g., `IInstrumentPlugin`)
2. Implement required methods
3. Build as shared library (.so, .dll, .dylib)
4. Place in plugins directory

### Q: Where do I put plugin files?

**A:** Default plugin directory:
- Linux: `/usr/local/lib/testmate/plugins/`
- Windows: `C:\Program Files\TestMATE\plugins\`
- macOS: `/Library/Application Support/TestMATE/plugins/`

Configurable via:
```cpp
config.SetValue("plugins.directory", "/custom/path");
```

### Q: Can plugins be loaded at runtime?

**A:** Yes, the plugin manager supports dynamic loading:
```cpp
auto& pluginMgr = CPluginManager::GetInstance();
auto result = pluginMgr.LoadPlugin("/path/to/plugin.so");
```

### Q: Are there example plugins?

**A:** Yes! See `examples/custom_plugin/`:
- **SimpleMultimeter** - SCPI instrument plugin
- **CustomMeasurementStep** - Test step plugin

---

## Qt GUI

### Q: Do I need Qt to use TestMATE?

**A:** No! TestMATE core works without Qt. Qt is only needed for the graphical interface.

### Q: Which Qt version is supported?

**A:** Qt5 5.15+ or Qt6. Both are supported.

### Q: How do I build the Qt GUI?

**A:**
```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build .
./bin/testmate_qt
```

### Q: The Qt GUI doesn't build. What should I check?

**A:**
1. Qt is installed: `qmake --version`
2. CMake finds Qt: Check CMake output for "Found Qt5" or "Found Qt6"
3. Set `CMAKE_PREFIX_PATH` to Qt installation directory

### Q: Can I use the Qt GUI remotely?

**A:** Yes, over X11 forwarding (Linux) or Remote Desktop (Windows):
```bash
ssh -X user@host
./testmate_qt
```

### Q: Where are GUI settings saved?

**A:**
- Linux: `~/.config/TestMATE/`
- Windows: `%APPDATA%\TestMATE\`
- macOS: `~/Library/Preferences/com.testmate.TestMATE/`

---

## Performance

### Q: How fast is TestMATE?

**A:** Benchmarks:
- Process model initialization: <100ms
- Step execution overhead: <1ms per step
- Parallel efficiency: >90% with 8 sockets
- Database write (SQLite): <2ms per record

### Q: How do I profile performance?

**A:** Use the built-in profiler:
```cpp
#include "utils/PerformanceProfiler.h"

PROFILE_SCOPE("MyOperation");
DoWork();

// Generate report
CPerformanceProfiler::GetInstance().GenerateHtmlReport("profile.html");
```

### Q: What's the maximum number of test steps I can have?

**A:** Tested with 10,000+ steps. Limited by available memory.

### Q: How much memory does TestMATE use?

**A:** Typical usage: <100MB for most applications. Scales with:
- Number of test steps
- Parallel sockets
- Cached results

### Q: Can I optimize database performance?

**A:** Yes:
1. Use transactions for batch inserts
2. Index frequently queried columns
3. Use PostgreSQL/MySQL for large datasets
4. Enable WAL mode for SQLite

---

## Licensing & Deployment

### Q: Can I use TestMATE commercially?

**A:** Check the LICENSE file for terms and conditions.

### Q: How do I deploy TestMATE?

**A:**
```bash
# Install to system
sudo cmake --install build

# Or create package
cmake --build build --target package
```

### Q: Can I redistribute TestMATE?

**A:** Check the LICENSE file for redistribution terms.

### Q: Is there a plugin marketplace?

**A:** Not yet, but planned for future releases.

---

## Common Errors

### Q: "undefined reference to TestMATE::..." when linking

**A:** Link against the appropriate libraries:
```cmake
target_link_libraries(my_app
    PRIVATE
        testmate_core
        testmate_utils
)
```

### Q: "error: 'std::source_location' has not been declared"

**A:** Your compiler doesn't fully support C++20. Update to:
- GCC 10+
- Clang 12+
- MSVC 2019+

### Q: Tests fail with "Database locked"

**A:** Close other applications accessing the database, or use WAL mode:
```cpp
db->Query("PRAGMA journal_mode=WAL");
```

### Q: Qt GUI crashes on startup

**A:** Check:
1. Qt libraries are in library path
2. Qt plugins directory is accessible
3. Graphics drivers are up to date

---

## Getting Help

### Q: Where can I find more documentation?

**A:**
- [Quick Start Guide](../../../QUICKSTART.md)
- [User Manual](../../USER_MANUAL.md)
- [API Reference](../api/CORE_API.md)
- [Knowledge Base](../INDEX.md)

### Q: How do I report a bug?

**A:** File an issue on GitHub with:
- TestMATE version
- Operating system
- Compiler version
- Minimal reproduction steps
- Error messages/logs

### Q: Can I request a feature?

**A:** Yes! Submit a GitHub issue with the "enhancement" label.

### Q: Is there a community forum?

**A:** Check GitHub Discussions for community support.

---

## See Also

- [Troubleshooting Guide](TROUBLESHOOTING.md) - Solutions to common problems
- [Known Issues](KNOWN_ISSUES.md) - Current limitations
- [Quick Start](../../../QUICKSTART.md) - Getting started tutorial

---

*Last updated: 2025-11-23 | TestMATE v2.0*
