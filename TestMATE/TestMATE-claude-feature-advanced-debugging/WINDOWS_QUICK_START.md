# TestMATE Windows Quick Start Guide

**For Qt 6.8.2 users on Windows (Offline capable)**

---

## 🚀 Fastest Way to Build (3 Steps)

### Step 1: Update Paths in `build_windows.bat`

Open `build_windows.bat` in a text editor and update these lines:

```batch
REM Update this to match YOUR Qt installation
set QT_ROOT=C:\Qt\6.8.2\msvc2022_64

REM Update if SQLite is manually installed
set SQLITE_ROOT=C:\sqlite3

REM Update if CMake is not in system PATH
set CMAKE_PATH=C:\Program Files\CMake\bin
```

### Step 2: Run the Build Script

Double-click `build_windows.bat` or run from command prompt:

```cmd
build_windows.bat
```

Select your preferred build option:
- **[1] Full Build** - Everything (recommended first time)
- **[2] Core Only** - Libraries only, no tests
- **[3] Core + Tests** - Libraries and tests, no GUI

### Step 3: Run Your Application

```cmd
cd build\bin\Release
testmate_unit_tests.exe    REM Run tests
testmate_gui.exe           REM Run GUI
```

**That's it! 🎉**

---

## 📋 Prerequisites Check

### Required (Must Have)

- ✅ **Qt 6.8.2** - You have this installed
- ✅ **Visual Studio 2019/2022** - With C++ workload
  - Install from: https://visualstudio.microsoft.com/
  - Select "Desktop development with C++"

- ✅ **CMake 3.15+**
  - Check: `cmake --version`
  - Install from: https://cmake.org/download/

### Optional (For Full Features)

- ⭐ **SQLite3** - For database features
  - Option A: Install via vcpkg (recommended)
  - Option B: Manual download from sqlite.org

- ⭐ **Google Test** - For running unit tests
  - Auto-downloaded by CMake (online)
  - Or manually place in `external/googletest/` (offline)

---

## 🔧 Common Issues & Solutions

### Issue 1: "Qt6 not found"

**Error:**
```
CMake Error: Could not find Qt6
```

**Solution:**
```cmd
REM Check Qt installation
dir C:\Qt\6.8.2\msvc2022_64

REM Set Qt path
set Qt6_DIR=C:\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64

REM Or update in build_windows.bat
```

### Issue 2: "CMake not found"

**Error:**
```
'cmake' is not recognized as an internal or external command
```

**Solution:**
```cmd
REM Add CMake to PATH temporarily
set PATH=C:\Program Files\CMake\bin;%PATH%

REM Or install CMake and check "Add to PATH"
```

### Issue 3: "Visual Studio not found"

**Error:**
```
CMake Error: Could not find Visual Studio
```

**Solution:**
1. Install Visual Studio 2022 Community (free)
2. Select "Desktop development with C++"
3. Or use VS 2019 (update script: `set VS_VERSION=2019`)

### Issue 4: "Google Test not found" (Offline)

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
REM Download once while online
git clone https://github.com/google/googletest.git external\googletest
cd external\googletest
git checkout release-1.12.1

REM Then build offline
cmake .. -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

### Issue 5: Missing DLLs when running

**Error:**
```
The code execution cannot proceed because Qt6Core.dll was not found
```

**Solution:**
```cmd
REM Deploy Qt DLLs automatically
cd build\bin\Release
C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe testmate_gui.exe

REM Or add Qt to PATH
set PATH=C:\Qt\6.8.2\msvc2022_64\bin;%PATH%
```

### Issue 6: SQLite3 errors

**Error:**
```
SQLite3 library not found
```

**Solution A - Use vcpkg (recommended):**
```cmd
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install sqlite3:x64-windows

cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

**Solution B - Manual install:**
1. Download from: https://www.sqlite.org/download.html
   - `sqlite-dll-win64-x64-*.zip` (DLL)
   - `sqlite-amalgamation-*.zip` (Headers)
2. Extract to `C:\sqlite3\`
3. Structure:
   ```
   C:\sqlite3\
   ├── include\sqlite3.h
   ├── lib\sqlite3.lib
   └── bin\sqlite3.dll
   ```

---

## 📁 Project Structure After Build

```
TestMATE/
├── build/
│   ├── bin/
│   │   ├── Release/
│   │   │   ├── testmate_gui.exe          ← Your GUI app
│   │   │   ├── testmate_unit_tests.exe   ← Unit tests
│   │   │   └── *.dll                      ← Qt & other DLLs
│   │   └── Debug/                         ← Debug builds
│   └── lib/
│       └── Release/
│           ├── testmate_core.lib
│           ├── testmate_debug.lib
│           ├── testmate_reliability.lib
│           └── ...                         ← All libraries
├── include/                                ← Public headers
├── src/                                    ← Source code
└── tests/                                  ← Test code
```

---

## 🎯 Build Types Explained

### Option 1: Full Build (Recommended)
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=ON -DTESTMATE_BUILD_GUI=ON
```
**Includes:**
- ✅ Core libraries
- ✅ Unit tests (135 tests)
- ✅ GUI application
- ✅ All examples

**Use when:** First time build, development, full testing

### Option 2: Core Only
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=OFF -DTESTMATE_BUILD_GUI=OFF
```
**Includes:**
- ✅ Core libraries only
- ❌ No tests
- ❌ No GUI

**Use when:** Offline, minimal build, library integration

### Option 3: Core + Tests
```cmd
cmake .. -DTESTMATE_BUILD_TESTS=ON -DTESTMATE_BUILD_GUI=OFF
```
**Includes:**
- ✅ Core libraries
- ✅ Unit tests
- ❌ No GUI

**Use when:** Testing without GUI dependencies

---

## 🔍 Verifying Your Build

### Check 1: Files Exist
```cmd
dir build\bin\Release\testmate_*.exe
```

Expected output:
```
testmate_gui.exe
testmate_unit_tests.exe
```

### Check 2: Run Tests
```cmd
cd build\bin\Release
testmate_unit_tests.exe
```

Expected output:
```
[==========] Running 135 tests from 43 test suites.
...
[  PASSED  ] 135 tests.
```

### Check 3: Run GUI
```cmd
cd build\bin\Release
testmate_gui.exe
```

Expected: GUI window opens

---

## 🌐 Offline Build Preparation

If you need to build without internet:

### While Online:
```cmd
REM 1. Clone repository
git clone <repo-url> TestMATE
cd TestMATE

REM 2. Download Google Test
mkdir external
cd external
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.12.1
cd ..\..

REM 3. Install vcpkg and SQLite
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install sqlite3:x64-windows
```

### Now Offline:
```cmd
cd TestMATE
build_windows.bat
```

Everything will build from local sources!

---

## 💡 Pro Tips

### Tip 1: Faster Builds
```cmd
REM Use parallel builds
cmake --build . --config Release -j 8

REM Or in Visual Studio
MSBuild.exe TestMATE.sln /m /p:Configuration=Release
```

### Tip 2: Clean Build
```cmd
REM Delete build directory and start fresh
rmdir /s /q build
mkdir build
cd build
cmake ..
```

### Tip 3: Debug vs Release
```cmd
REM Release: Optimized, faster, smaller
cmake --build . --config Release

REM Debug: Symbols, slower, easier to debug
cmake --build . --config Debug
```

### Tip 4: Install to System
```cmd
REM After build
cmake --install . --prefix C:\TestMATE
```

---

## 📞 Getting Help

If you're stuck:

1. **Check error message** - Usually tells you what's missing
2. **Check paths** - Ensure Qt, CMake, VS paths are correct
3. **Try minimal build** - Use Core Only option
4. **Check logs** - Look in `build/CMakeFiles/CMakeError.log`

Common solutions:
- Update paths in `build_windows.bat`
- Install Visual Studio C++ tools
- Add CMake to PATH
- Use vcpkg for dependencies

---

## ✅ Success Checklist

Before you start:
- [ ] Qt 6.8.2 installed
- [ ] Visual Studio 2019/2022 with C++
- [ ] CMake 3.15+ installed
- [ ] Paths updated in `build_windows.bat`

After building:
- [ ] Build completes without errors
- [ ] Unit tests run and pass (if built)
- [ ] GUI opens (if built)
- [ ] No missing DLL errors

---

**Ready to build? Run `build_windows.bat` and you're set! 🚀**

For detailed offline instructions, see: [BUILD_WINDOWS_OFFLINE.md](BUILD_WINDOWS_OFFLINE.md)
