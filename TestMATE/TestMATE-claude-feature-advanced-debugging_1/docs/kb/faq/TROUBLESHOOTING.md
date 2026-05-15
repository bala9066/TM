# Troubleshooting Guide

**Solutions to common TestMATE problems**

---

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Errors](#runtime-errors)
3. [Test Failures](#test-failures)
4. [Database Problems](#database-problems)
5. [Qt GUI Issues](#qt-gui-issues)
6. [Performance Problems](#performance-problems)
7. [Plugin Issues](#plugin-issues)
8. [Network & Communication](#network--communication)

---

## Build Issues

### Problem: CMake fails with "Could not find SQLite3"

**Symptoms:**
```
CMake Error: Could not find SQLite3
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# macOS
brew install sqlite

# Windows
# Download from https://www.sqlite.org/download.html
```

**Verify installation:**
```bash
pkg-config --modversion sqlite3
```

---

### Problem: "C++20 required" compilation error

**Symptoms:**
```
error: #error This file requires compiler and library support for the ISO C++ 2020 standard
```

**Solution:**
Update your compiler:
```bash
# Ubuntu - install GCC 10+
sudo apt-get install g++-10
cmake .. -DCMAKE_CXX_COMPILER=g++-10

# macOS - update Xcode
xcode-select --install

# Or use Homebrew GCC
brew install gcc@11
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

---

### Problem: Qt not found during CMake configuration

**Symptoms:**
```
Could not find Qt5 or Qt6
Skipping Qt GUI build
```

**Solution:**
```bash
# Specify Qt location
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.4.0/gcc_64

# macOS with Homebrew
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)

# Windows
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.4.0/msvc2019_64
```

---

### Problem: Linker errors with "undefined reference"

**Symptoms:**
```
undefined reference to `TestMATE::CLogManager::GetInstance()'
```

**Solution:**
Ensure you're linking all required libraries:
```cmake
target_link_libraries(my_app
    PRIVATE
        testmate_core
        testmate_utils
        testmate_database  # If using database
        testmate_test_steps  # If using test steps
)
```

---

### Problem: Build hangs or is very slow

**Symptoms:**
- Build takes >10 minutes
- System becomes unresponsive

**Solution:**
1. Reduce parallel jobs:
```bash
cmake --build . -j2  # Instead of -j4 or -j8
```

2. Build in Release mode (faster):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

3. Close other applications to free RAM

---

## Runtime Errors

### Problem: "Segmentation fault" on startup

**Symptoms:**
```
Segmentation fault (core dumped)
```

**Diagnosis:**
Run with debugger:
```bash
gdb ./bin/testmate_qt
(gdb) run
# After crash:
(gdb) backtrace
```

**Common Causes:**
1. **Null pointer access** - Check initialization
2. **Stack overflow** - Reduce recursion depth
3. **Invalid memory access** - Run with valgrind:
```bash
valgrind --leak-check=full ./bin/my_app
```

---

### Problem: "Failed to initialize process model"

**Symptoms:**
```
Error: Failed to initialize process model: kNotInitialized
```

**Solution:**
Ensure proper initialization sequence:
```cpp
CSequentialModel model;
auto result = model.Initialize();  // Must call before Execute
if (!result.IsSuccess()) {
    std::cerr << "Init failed: " << result.GetMessage() << std::endl;
}
```

---

### Problem: "Resource busy" timeout errors

**Symptoms:**
```
Error: kResourceBusy - Timeout waiting for resource
```

**Solution:**
1. Increase timeout:
```cpp
scheduler.AllocateResource("DMM-001", socketId,
    std::chrono::seconds(60));  // Longer timeout
```

2. Check for deadlocks:
```cpp
// Enable deadlock detection
scheduler.EnableDeadlockDetection(true);
```

3. Ensure resources are released:
```cpp
// Always release in finally/RAII
class ResourceGuard {
    ~ResourceGuard() {
        scheduler.ReleaseResource(resourceId, socketId);
    }
};
```

---

### Problem: Tests hang indefinitely

**Symptoms:**
- Application freezes
- No progress updates
- CPU at 100%

**Diagnosis:**
1. **Check for deadlocks:**
```bash
# Linux: Check thread status
gdb -p <pid>
(gdb) thread apply all bt
```

2. **Enable logging:**
```cpp
auto& log = CLogManager::GetInstance();
log.SetLogLevel(ELogLevel::kDebug);
log.SetLogFile("debug.log");
```

3. **Add timeouts:**
```cpp
// Set execution timeout
model.SetTimeout(std::chrono::minutes(5));
```

---

## Test Failures

### Problem: Unit tests fail with "Assertion failed"

**Symptoms:**
```
[ RUN ] ExecutionContextTest.SetAndGetVariable
Assertion failed: result.has_value()
[  FAILED  ] ExecutionContextTest.SetAndGetVariable
```

**Solution:**
1. Run specific test with verbose output:
```bash
./testmate_unit_tests --gtest_filter=ExecutionContextTest.* --verbose
```

2. Check test prerequisites:
```cpp
// Ensure proper setup
TEST_F(ExecutionContextTest, SetAndGetVariable) {
    // Verify context is initialized
    ASSERT_NE(context, nullptr);
    // ...
}
```

---

### Problem: Integration tests fail intermittently

**Symptoms:**
- Tests pass sometimes, fail other times
- Timing-related failures

**Solution:**
1. **Increase timeouts:**
```cpp
ASSERT_TRUE(WaitForCondition([]() { return IsReady(); },
    std::chrono::seconds(10)));  // Longer timeout
```

2. **Add synchronization:**
```cpp
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

3. **Use mock objects:**
```cpp
// Instead of real hardware
auto mockDMM = std::make_shared<MockMultimeter>();
EXPECT_CALL(*mockDMM, Measure()).WillOnce(Return(5.0));
```

---

## Database Problems

### Problem: "Database locked" errors

**Symptoms:**
```
Error: kDatabaseQueryFailed - database is locked
```

**Solution:**
1. **Enable WAL mode (SQLite):**
```cpp
db->Query("PRAGMA journal_mode=WAL");
db->Query("PRAGMA busy_timeout=5000");
```

2. **Use transactions:**
```cpp
db->BeginTransaction();
// ... multiple operations ...
db->CommitTransaction();
```

3. **Close other connections:**
```bash
# Find processes using database
lsof | grep testmate.db
# Kill if necessary
```

---

### Problem: PostgreSQL connection fails

**Symptoms:**
```
Error: kConnectionFailed - could not connect to server
```

**Solution:**
1. **Verify PostgreSQL is running:**
```bash
sudo systemctl status postgresql
# Start if needed
sudo systemctl start postgresql
```

2. **Check connection string:**
```cpp
// Correct format
TString connStr = "host=localhost port=5432 dbname=testmate user=admin password=secret";
```

3. **Test connection manually:**
```bash
psql -h localhost -U admin -d testmate
```

4. **Check firewall:**
```bash
# Allow PostgreSQL port
sudo ufw allow 5432/tcp
```

---

### Problem: Database queries are slow

**Symptoms:**
- Query takes >1 second
- Application becomes unresponsive during queries

**Solution:**
1. **Add indexes:**
```sql
CREATE INDEX idx_test_id ON test_data(test_id);
CREATE INDEX idx_lot_number ON test_data(lot_number);
```

2. **Use batched inserts:**
```cpp
db->BeginTransaction();
for (const auto& record : records) {
    db->StoreTestData(record);
}
db->CommitTransaction();
```

3. **Enable query profiling:**
```cpp
db->Query("EXPLAIN ANALYZE SELECT * FROM test_data WHERE lot_number='LOT-123'");
```

---

## Qt GUI Issues

### Problem: GUI doesn't start - immediate crash

**Symptoms:**
```
Segmentation fault
or
Could not initialize OpenGL
```

**Solution:**
1. **Check Qt libraries:**
```bash
ldd ./bin/testmate_qt
# Verify all Qt libraries are found
```

2. **Set library path:**
```bash
export LD_LIBRARY_PATH=/path/to/Qt/lib:$LD_LIBRARY_PATH
./bin/testmate_qt
```

3. **Disable OpenGL (software rendering):**
```bash
export QT_XCB_GL_INTEGRATION=none
./bin/testmate_qt
```

---

### Problem: Qt GUI is blank or widgets missing

**Symptoms:**
- Window appears but is empty
- Some widgets don't show

**Solution:**
1. **Check Qt plugins:**
```bash
export QT_DEBUG_PLUGINS=1
./bin/testmate_qt
# Look for plugin loading errors
```

2. **Set plugin path:**
```bash
export QT_PLUGIN_PATH=/path/to/Qt/plugins
```

3. **Reinstall Qt:**
```bash
sudo apt-get install --reinstall qt5-default
```

---

### Problem: GUI is very slow or laggy

**Symptoms:**
- Slow rendering
- High CPU usage
- Unresponsive interface

**Solution:**
1. **Enable hardware acceleration:**
```bash
export QT_XCB_GL_INTEGRATION=xcb_egl
```

2. **Reduce update frequency:**
```cpp
// In ExecutionMonitorWidget
m_pTimer->setInterval(100);  // Update every 100ms instead of 16ms
```

3. **Limit table rows:**
```cpp
// Only show recent results
model->setRowLimit(1000);
```

---

## Performance Problems

### Problem: Tests run much slower than expected

**Symptoms:**
- Single test takes minutes instead of seconds
- Parallel execution slower than sequential

**Diagnosis:**
```cpp
#include "utils/PerformanceProfiler.h"

PROFILE_SCOPE("TestExecution");
// Your test code
profiler.GenerateHtmlReport("profile.html");
```

**Solutions:**
1. **Check I/O bottlenecks:**
```cpp
// Use async I/O
async::Future<> future = async::run([]() {
    WriteToDatabase();
});
```

2. **Optimize database access:**
```cpp
// Batch operations
db->BeginTransaction();
for (...) { db->Insert(); }
db->CommitTransaction();
```

3. **Reduce logging:**
```cpp
log.SetLogLevel(ELogLevel::kWarning);  // Only warnings and errors
```

---

### Problem: High memory usage

**Symptoms:**
```
Out of memory
or
System swap usage high
```

**Solution:**
1. **Profile memory usage:**
```bash
valgrind --tool=massif ./bin/my_app
ms_print massif.out.*
```

2. **Clear result cache:**
```cpp
executionContext.ClearStepResults();
```

3. **Use memory pools for frequent allocations**

4. **Limit concurrent sockets:**
```cpp
// Reduce from 32 to 8 if memory-constrained
CParallelModel model(8);
```

---

## Plugin Issues

### Problem: Plugin fails to load

**Symptoms:**
```
Error: Failed to load plugin: /path/to/plugin.so
```

**Solution:**
1. **Check plugin exists:**
```bash
ls -l /path/to/plugin.so
file /path/to/plugin.so  # Verify it's a valid shared library
```

2. **Check dependencies:**
```bash
ldd /path/to/plugin.so
# Ensure all dependencies are found
```

3. **Verify ABI compatibility:**
```bash
# Must match TestMATE build
nm -D plugin.so | grep TestMATE
```

4. **Check permissions:**
```bash
chmod 755 /path/to/plugin.so
```

---

### Problem: Plugin loads but crashes

**Symptoms:**
- Segfault when using plugin
- "Symbol not found" errors

**Solution:**
1. **Check plugin interface version:**
```cpp
// In plugin
extern "C" {
    int GetAPIVersion() { return 2; }  // Must match TestMATE version
}
```

2. **Verify exports:**
```bash
nm -D plugin.so | grep CreatePlugin
# Should show exported symbols
```

3. **Build plugin with same compiler/flags:**
```cmake
# Use same C++ standard and flags
set(CMAKE_CXX_STANDARD 20)
```

---

## Network & Communication

### Problem: Serial port communication fails

**Symptoms:**
```
Error: kConnectionFailed - Unable to open serial port
```

**Solution:**
1. **Check port exists:**
```bash
ls -l /dev/ttyUSB*
ls -l /dev/ttyACM*
```

2. **Fix permissions:**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

3. **Check if port is in use:**
```bash
lsof | grep ttyUSB0
# Kill process if needed
```

4. **Test with minicom:**
```bash
minicom -D /dev/ttyUSB0 -b 9600
```

---

### Problem: GPIB/VISA instrument not found

**Symptoms:**
```
Error: Instrument not found at GPIB::1
```

**Solution:**
1. **Verify VISA installation:**
```bash
# NI-VISA
visaconf  # or ni-visa-config

# List devices
vxilist
```

2. **Test connection:**
```bash
# Using VISA Interactive Control
visaic
> open GPIB0::1::INSTR
> write "*IDN?"
> read
```

3. **Check cable connections**
4. **Verify instrument address matches**

---

## Diagnostic Tools

### Enable Debug Logging

```cpp
auto& log = CLogManager::GetInstance();
log.SetLogLevel(ELogLevel::kDebug);
log.SetLogFile("testmate_debug.log");
```

### Performance Profiling

```cpp
#include "utils/PerformanceProfiler.h"

auto& profiler = CPerformanceProfiler::GetInstance();
PROFILE_SCOPE("Operation");
// ... code ...
profiler.GenerateHtmlReport("performance.html");
```

### Memory Leak Detection

```bash
# Linux
valgrind --leak-check=full --show-leak-kinds=all ./my_app

# macOS
leaks --atExit -- ./my_app
```

### Thread Analysis

```bash
# Check for deadlocks
gdb -p <pid>
(gdb) thread apply all bt
```

---

## Getting Help

If you're still experiencing issues:

1. **Check logs:** Look in `testmate_debug.log`
2. **Search documentation:** [Knowledge Base](../INDEX.md)
3. **Review examples:** [Examples directory](../../../examples/)
4. **File a bug report:** Include:
   - TestMATE version
   - OS and compiler version
   - Minimal reproduction steps
   - Error messages and logs
   - Stack trace if available

---

## See Also

- [FAQ](FAQ.md) - Frequently asked questions
- [Known Issues](KNOWN_ISSUES.md) - Current limitations
- [API Reference](../api/CORE_API.md) - API documentation

---

*Last updated: 2025-11-23 | TestMATE v2.0*
