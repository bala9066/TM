# TestMATE MinGW Build Guide (Windows)

**Quick start for building TestMATE with MinGW and Qt 6.8.2 on Windows**

---

## 🚀 Fastest Way (3 Steps)

### Step 1: Verify Your Qt Installation

Check your Qt installation path. Common locations:
```
C:\Qt\6.8.2\mingw_64          ← Most common
C:\Qt\6.8.2\mingw1120_64      ← With GCC version
C:\Qt\6.8.2\mingw_81          ← Older version
```

**Find your Qt path:**
```cmd
dir /s /b C:\Qt\qmake.exe
```

Look for the path containing `mingw` (not `msvc`).

### Step 2: Run Build Script

1. Open `build_windows_mingw.bat` in text editor
2. Update the first line under "CONFIGURATION":
   ```batch
   set QT_ROOT=C:\Qt\6.8.2\mingw_64    ← Update to YOUR path
   ```

3. Double-click `build_windows_mingw.bat` and select:
   - **[1] Release Build** (recommended)
   - **[2] Debug Build** (for development)
   - **[3] Full Build with Tests** (complete)
   - **[4] Core Only** (minimal, offline-friendly)

### Step 3: Run Your Application

```cmd
cd build\bin
testmate_unit_tests.exe    REM Run 135 unit tests
testmate_gui.exe           REM Run GUI application
```

**Done! 🎉**

---

## 📋 Prerequisites

### Required

✅ **Qt 6.8.2 with MinGW** (You have this!)
- Must include MinGW compiler
- Usually: `C:\Qt\6.8.2\mingw_64`

✅ **CMake 3.15+**
```cmd
cmake --version
```
Download: https://cmake.org/download/

✅ **MinGW Compiler** (Usually comes with Qt)
- Included in Qt installation: `C:\Qt\Tools\mingw1120_64`
- Or standalone: https://www.mingw-w64.org/

### Optional

⭐ **SQLite3** (for database features)
- Auto-detected if in system PATH
- Or specify: `set SQLITE_ROOT=C:\sqlite3`

⭐ **Git** (for downloading Google Test)
- Only needed if building tests online
- Download: https://git-scm.com/download/win

---

## 🛠️ Manual Build (Command Line)

If you prefer manual control:

### Open Command Prompt

```cmd
REM Add MinGW and Qt to PATH
set PATH=C:\Qt\6.8.2\mingw_64\bin;%PATH%
set PATH=C:\Qt\Tools\mingw1120_64\bin;%PATH%

REM Verify tools are available
g++ --version
mingw32-make --version
cmake --version
```

### Configure with CMake

```cmd
cd C:\path\to\TestMATE
mkdir build
cd build

REM Configure (choose ONE option)

REM Option 1: Full build (everything)
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64 ^
  -DTESTMATE_BUILD_TESTS=ON ^
  -DTESTMATE_BUILD_GUI=ON

REM Option 2: Core only (offline-friendly)
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64 ^
  -DTESTMATE_BUILD_TESTS=OFF ^
  -DTESTMATE_BUILD_GUI=OFF

REM Option 3: Release without tests
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64 ^
  -DTESTMATE_BUILD_TESTS=OFF ^
  -DTESTMATE_BUILD_GUI=ON
```

### Build

```cmd
REM Build with 4 parallel jobs (faster)
mingw32-make -j4

REM Or build with single thread (more stable)
mingw32-make
```

### Deploy Qt DLLs

```cmd
cd bin
C:\Qt\6.8.2\mingw_64\bin\windeployqt.exe testmate_gui.exe
```

### Run

```cmd
REM Run tests
testmate_unit_tests.exe

REM Run GUI
testmate_gui.exe
```

---

## 🔧 Key Differences: MinGW vs MSVC

| Aspect | MinGW | MSVC |
|--------|-------|------|
| **Compiler** | GCC (g++) | Microsoft C++ |
| **CMake Generator** | `MinGW Makefiles` | `Visual Studio` |
| **Build Tool** | `mingw32-make` | `MSBuild.exe` |
| **Qt Binary** | `mingw_64` | `msvc2022_64` |
| **Output Dir** | `build/bin/` | `build/bin/Release/` |
| **Library Ext** | `.a` (static) | `.lib` (static) |

---

## ⚠️ Common Issues & Solutions

### Issue 1: "g++ not found"

**Error:**
```
'g++' is not recognized as an internal or external command
```

**Solution:**
```cmd
REM Find MinGW installation
dir /s /b C:\Qt\g++.exe

REM Add to PATH (example)
set PATH=C:\Qt\Tools\mingw1120_64\bin;%PATH%

REM Verify
g++ --version
```

### Issue 2: "Qt6 not found"

**Error:**
```
CMake Error: Could not find Qt6
```

**Solution:**
```cmd
REM Check Qt path
dir C:\Qt\6.8.2\mingw_64\bin\qmake.exe

REM Use correct path in cmake
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64
```

### Issue 3: "mingw32-make not found"

**Error:**
```
'mingw32-make' is not recognized
```

**Solution A - Use make:**
```cmd
REM If make.exe exists instead
make -j4
```

**Solution B - Create alias:**
```cmd
REM Copy mingw32-make to make
copy C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe make.exe
```

**Solution C - Install MinGW:**
```cmd
REM Download MinGW-w64 from:
https://www.mingw-w64.org/downloads/
```

### Issue 4: Missing Qt DLLs

**Error when running GUI:**
```
The code execution cannot proceed because Qt6Core.dll was not found
```

**Solution A - Deploy DLLs:**
```cmd
cd build\bin
C:\Qt\6.8.2\mingw_64\bin\windeployqt.exe testmate_gui.exe
```

**Solution B - Add Qt to PATH:**
```cmd
set PATH=C:\Qt\6.8.2\mingw_64\bin;%PATH%
testmate_gui.exe
```

### Issue 5: Google Test download fails (offline)

**Error:**
```
Failed to download Google Test
```

**Solution A - Disable tests:**
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=OFF
```

**Solution B - Use local Google Test:**
```cmd
REM Download once (while online)
mkdir external
cd external
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.12.1
cd ..\..

REM Build offline
cmake .. -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

### Issue 6: Build errors with C++20

**Error:**
```
error: 'concept' does not name a type
```

**Solution:**
```cmd
REM Ensure MinGW GCC 10+
g++ --version

REM Should show: g++ (GCC) 10.0 or higher
REM If older, update Qt installation to include newer MinGW
```

### Issue 7: Slow single-threaded build

**Problem:** Build takes 10+ minutes

**Solution - Use parallel jobs:**
```cmd
REM Instead of
mingw32-make

REM Use
mingw32-make -j4    REM 4 parallel jobs
mingw32-make -j8    REM 8 parallel jobs (if you have 8+ cores)
```

---

## 📁 Build Output Structure

After successful build:

```
build/
├── bin/
│   ├── testmate_gui.exe              ← Your GUI application
│   ├── testmate_unit_tests.exe       ← Unit tests (135 tests)
│   ├── libtestmate_core.dll.a        ← Import library
│   └── Qt6*.dll                       ← Qt DLLs (after windeployqt)
│
├── lib/
│   ├── libtestmate_core.a            ← Static library
│   ├── libtestmate_debug.a
│   ├── libtestmate_reliability.a
│   ├── libtestmate_profiling.a
│   ├── libtestmate_rest_api.a
│   ├── libtestmate_orchestration.a
│   └── libtestmate_resources.a
│
└── CMakeCache.txt                     ← Build configuration
```

---

## 🎯 Build Options Explained

### Debug vs Release

**Debug Build:**
```cmd
cmake .. -DCMAKE_BUILD_TYPE=Debug
```
- Includes debug symbols
- No optimization
- Larger binaries
- Easier to debug with GDB
- Slower execution

**Release Build:**
```cmd
cmake .. -DCMAKE_BUILD_TYPE=Release
```
- Optimized (-O3)
- Smaller binaries
- Faster execution
- No debug symbols
- **Recommended for production**

### With/Without Tests

**With Tests:**
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=ON
```
- Builds 135 unit tests
- Requires Google Test (auto-downloaded)
- Adds ~2 minutes to build time

**Without Tests:**
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=OFF
```
- No test suite
- Faster build
- Offline-friendly (no Google Test needed)

### With/Without GUI

**With GUI:**
```cmd
cmake .. -DTESTMATE_BUILD_GUI=ON
```
- Builds Qt-based GUI application
- Requires Qt deployment

**Without GUI:**
```cmd
cmake .. -DTESTMATE_BUILD_GUI=OFF
```
- Core libraries only
- No Qt required (but still uses CMAKE_PREFIX_PATH)

---

## 🔍 Verifying Your Build

### Step 1: Check files exist
```cmd
dir build\bin\testmate_*.exe
```

Expected output:
```
testmate_gui.exe
testmate_unit_tests.exe
```

### Step 2: Run tests
```cmd
cd build\bin
testmate_unit_tests.exe
```

Expected output:
```
[==========] Running 135 tests from 43 test suites.
...
[  PASSED  ] 135 tests.
```

### Step 3: Check libraries
```cmd
dir build\lib\*.a
```

Expected: 7-8 `.a` files (static libraries)

### Step 4: Run GUI
```cmd
cd build\bin
testmate_gui.exe
```

Expected: GUI window opens without DLL errors

---

## 🌐 Offline Build Setup

### Preparation (While Online)

```cmd
REM 1. Clone repository
git clone <your-repo-url> TestMATE
cd TestMATE

REM 2. Download Google Test for offline use
mkdir external
cd external
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.12.1
cd ..\..

REM 3. (Optional) Install SQLite via MSYS2/pacman
REM Or download from sqlite.org
```

### Build (Offline)

```cmd
cd TestMATE

REM Update paths in build_windows_mingw.bat
notepad build_windows_mingw.bat

REM Run build
build_windows_mingw.bat
```

Select option `[1]` or `[3]` and build proceeds offline!

---

## 💡 Pro Tips

### Tip 1: Create reusable environment script

Save as `setup_env.bat`:
```batch
@echo off
set QT_ROOT=C:\Qt\6.8.2\mingw_64
set MINGW_BIN=C:\Qt\Tools\mingw1120_64\bin
set PATH=%MINGW_BIN%;%QT_ROOT%\bin;%PATH%
echo Environment configured for TestMATE build
```

Use it:
```cmd
setup_env.bat
cd TestMATE\build
mingw32-make -j4
```

### Tip 2: Faster incremental builds

```cmd
REM After first full build, rebuild only changed files
cd build
mingw32-make -j4    REM Much faster!
```

### Tip 3: Clean and rebuild

```cmd
cd build
mingw32-make clean         REM Clean build artifacts
mingw32-make -j4           REM Rebuild from scratch
```

### Tip 4: Build specific target

```cmd
REM Build only core library
mingw32-make testmate_core

REM Build only tests
mingw32-make testmate_unit_tests

REM Build only GUI
mingw32-make testmate_gui
```

### Tip 5: Verbose build output

```cmd
REM See full compiler commands
mingw32-make VERBOSE=1
```

### Tip 6: Use ccache for faster rebuilds

```cmd
REM Install ccache (via MSYS2 or standalone)
pacman -S ccache

REM Configure CMake to use it
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

---

## 🆘 Getting Help

If stuck:

1. **Check error message** - Usually tells you what's wrong
2. **Verify paths** - Qt and MinGW paths must be correct
3. **Check tool versions:**
   ```cmd
   g++ --version      REM Need GCC 10+
   cmake --version    REM Need CMake 3.15+
   ```
4. **Try minimal build:**
   ```cmd
   cmake .. -DTESTMATE_BUILD_TESTS=OFF -DTESTMATE_BUILD_GUI=OFF
   ```
5. **Check CMake log:**
   ```cmd
   type build\CMakeFiles\CMakeError.log
   ```

---

## ✅ Success Checklist

Before building:
- [ ] Qt 6.8.2 MinGW installed
- [ ] MinGW in PATH (g++, mingw32-make)
- [ ] CMake 3.15+ installed
- [ ] Paths updated in build script

After building:
- [ ] Build completes without errors
- [ ] testmate_unit_tests.exe runs (if built)
- [ ] testmate_gui.exe opens (if built)
- [ ] No missing DLL errors

---

## 🎯 Quick Commands Reference

```cmd
REM Full automated build
build_windows_mingw.bat

REM Manual Release build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64
mingw32-make -j4

REM Manual Debug build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\mingw_64
mingw32-make -j4

REM Deploy Qt DLLs
cd bin && C:\Qt\6.8.2\mingw_64\bin\windeployqt.exe testmate_gui.exe

REM Run tests
cd build\bin && testmate_unit_tests.exe

REM Clean rebuild
cd build && mingw32-make clean && mingw32-make -j4
```

---

**Ready to build with MinGW? Run `build_windows_mingw.bat` now! 🚀**
