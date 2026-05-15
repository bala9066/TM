# Building TestMATE on Windows (Offline)

**System Requirements:**
- Windows 10/11
- Qt 6.8.2 (installed)
- Visual Studio 2019/2022 or MinGW
- CMake 3.15+
- Git (for cloning repository)

---

## 📋 Prerequisites (Download While Online)

Before going offline, ensure you have these tools installed:

### 1. **CMake** (v3.15 or newer)
- Download: https://cmake.org/download/
- Installer: `cmake-3.xx.x-windows-x86_64.msi`
- Install to: `C:\Program Files\CMake`
- **Add to PATH during installation**

### 2. **Visual Studio 2022** (or 2019)
- Download: https://visualstudio.microsoft.com/downloads/
- Install **"Desktop development with C++"** workload
- Includes:
  - MSVC compiler
  - Windows SDK
  - C++ CMake tools

### 3. **SQLite3** (for database support)
- **Option A - Pre-compiled DLL:**
  - Download from: https://www.sqlite.org/download.html
  - Get: `sqlite-dll-win64-x64-*.zip` (DLL)
  - Get: `sqlite-amalgamation-*.zip` (Headers)
  - Extract to: `C:\sqlite3\`

- **Option B - Use vcpkg (recommended):**
  ```cmd
  # While online, install vcpkg
  git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
  cd C:\vcpkg
  .\bootstrap-vcpkg.bat
  .\vcpkg install sqlite3:x64-windows
  ```

### 4. **Google Test** (for unit tests)
- TestMATE uses CMake's FetchContent to download Google Test
- **For offline builds**, download manually:
  ```cmd
  # While online
  git clone https://github.com/google/googletest.git C:\googletest
  cd C:\googletest
  git checkout release-1.12.1
  ```

### 5. **Qt 6.8.2** (You already have this ✓)
- Ensure Qt is in PATH or note installation directory
- Typical location: `C:\Qt\6.8.2\msvc2022_64\`

---

## 🚀 Building TestMATE (Offline Steps)

### Step 1: Set Up Environment

Open **"x64 Native Tools Command Prompt for VS 2022"** (or your compiler version)

```cmd
REM Set Qt path
set Qt6_DIR=C:\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6
set PATH=C:\Qt\6.8.2\msvc2022_64\bin;%PATH%

REM Set CMake path (if not in PATH)
set PATH=C:\Program Files\CMake\bin;%PATH%

REM Set SQLite path (if using manual installation)
set SQLite3_ROOT=C:\sqlite3
```

### Step 2: Configure CMake for Offline Build

Navigate to your TestMATE directory:

```cmd
cd C:\path\to\TestMATE
mkdir build
cd build
```

**Option A: Use system Google Test (if you downloaded it)**
```cmd
cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DTESTMATE_BUILD_TESTS=ON ^
  -DTESTMATE_BUILD_GUI=ON ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64 ^
  -DGTest_ROOT=C:\googletest ^
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

**Option B: Disable Tests (if no Google Test available)**
```cmd
cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DTESTMATE_BUILD_TESTS=OFF ^
  -DTESTMATE_BUILD_GUI=ON ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
```

**Option C: Use vcpkg toolchain**
```cmd
cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DTESTMATE_BUILD_TESTS=ON ^
  -DTESTMATE_BUILD_GUI=ON ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
```

### Step 3: Build

```cmd
REM Build Release configuration
cmake --build . --config Release -j 4

REM Or build Debug configuration
cmake --build . --config Debug -j 4
```

### Step 4: Run Tests (if built)

```cmd
cd bin\Release
testmate_unit_tests.exe
```

### Step 5: Run GUI Application (if built)

```cmd
cd bin\Release
testmate_gui.exe
```

---

## 📦 Handling Dependencies Offline

### Google Test (Offline Setup)

Create `external\googletest\` in your TestMATE directory:

```cmd
cd C:\path\to\TestMATE
mkdir external
cd external
xcopy /E /I C:\googletest googletest
```

Then modify `CMakeLists.txt` to use local Google Test:

```cmake
# In CMakeLists.txt, find the FetchContent section and replace with:
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/googletest")
    add_subdirectory(external/googletest)
else()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()
```

### SQLite3 (Manual Setup)

If you downloaded SQLite manually:

1. **Extract files:**
   ```
   C:\sqlite3\
   ├── include\
   │   └── sqlite3.h
   ├── lib\
   │   └── sqlite3.lib
   └── bin\
       └── sqlite3.dll
   ```

2. **Add to CMakeLists.txt** (if not automatically found):
   ```cmake
   # Add after project() declaration
   set(SQLite3_INCLUDE_DIR "C:/sqlite3/include")
   set(SQLite3_LIBRARY "C:/sqlite3/lib/sqlite3.lib")
   find_package(SQLite3 REQUIRED)
   ```

3. **Copy DLL to output directory** after build:
   ```cmd
   copy C:\sqlite3\bin\sqlite3.dll build\bin\Release\
   copy C:\sqlite3\bin\sqlite3.dll build\bin\Debug\
   ```

---

## 🔧 Troubleshooting

### Issue 1: CMake can't find Qt

**Solution:**
```cmd
set CMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
```

### Issue 2: "Could not find Google Test"

**Solution:** Either:
- Use `-DTESTMATE_BUILD_TESTS=OFF` to skip tests
- Set up Google Test offline (see above)

### Issue 3: SQLite3 not found

**Solution:**
```cmd
cmake .. -DSQLite3_ROOT=C:\sqlite3
```

Or use vcpkg:
```cmd
C:\vcpkg\vcpkg install sqlite3:x64-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

### Issue 4: Qt DLLs missing when running

**Solution:** Copy Qt DLLs to executable directory:
```cmd
cd build\bin\Release
windeployqt.exe testmate_gui.exe
```

### Issue 5: Compilation errors with C++20

**Solution:** Ensure you're using Visual Studio 2019 16.11+ or VS 2022:
```cmd
cmake .. -G "Visual Studio 17 2022" -A x64
```

---

## 📝 Complete Offline Build Script

Save this as `build_offline.bat`:

```batch
@echo off
REM TestMATE Offline Build Script for Windows

echo ========================================
echo TestMATE Offline Build Script
echo ========================================

REM Set paths (CUSTOMIZE THESE FOR YOUR SYSTEM)
set QT_DIR=C:\Qt\6.8.2\msvc2022_64
set SQLITE_ROOT=C:\sqlite3
set CMAKE_PATH=C:\Program Files\CMake\bin

REM Add to PATH
set PATH=%QT_DIR%\bin;%CMAKE_PATH%;%PATH%

REM Create build directory
if not exist build mkdir build
cd build

REM Configure
echo.
echo Configuring CMake...
cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DTESTMATE_BUILD_TESTS=ON ^
  -DTESTMATE_BUILD_GUI=ON ^
  -DCMAKE_PREFIX_PATH=%QT_DIR% ^
  -DSQLite3_ROOT=%SQLITE_ROOT%

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo.
echo Building TestMATE...
cmake --build . --config Release -j 4

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Copy SQLite DLL
echo.
echo Copying SQLite DLL...
copy %SQLITE_ROOT%\bin\sqlite3.dll bin\Release\

REM Deploy Qt DLLs
echo.
echo Deploying Qt DLLs...
cd bin\Release
%QT_DIR%\bin\windeployqt.exe testmate_gui.exe
cd ..\..

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables are in: build\bin\Release\
echo - testmate_gui.exe (GUI application)
echo - testmate_unit_tests.exe (Unit tests)
echo.
pause
```

Run it:
```cmd
build_offline.bat
```

---

## 🎯 Minimal Build (Core Only, No Dependencies)

If you want to build without tests or GUI:

```cmd
cd TestMATE
mkdir build
cd build

cmake .. ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DTESTMATE_BUILD_TESTS=OFF ^
  -DTESTMATE_BUILD_GUI=OFF

cmake --build . --config Release
```

This builds only the core libraries and doesn't require:
- Google Test
- Qt
- SQLite (if you disable database module)

---

## 📚 What Gets Built

After a successful build, you'll have:

```
build/
├── bin/
│   └── Release/
│       ├── testmate_gui.exe           (GUI application)
│       ├── testmate_unit_tests.exe    (Unit tests)
│       └── *.dll                       (Qt + SQLite DLLs)
├── lib/
│   └── Release/
│       ├── testmate_core.lib
│       ├── testmate_debug.lib
│       ├── testmate_reliability.lib
│       ├── testmate_profiling.lib
│       ├── testmate_rest_api.lib
│       ├── testmate_orchestration.lib
│       ├── testmate_resources.lib
│       └── testmate_database.lib
└── include/                            (Generated headers)
```

---

## 🔍 Verifying the Build

```cmd
cd build\bin\Release

REM Run unit tests
testmate_unit_tests.exe

REM Expected output:
REM [==========] Running 135 tests from 43 test suites.
REM [  PASSED  ] 135 tests.

REM Run GUI (if built)
testmate_gui.exe
```

---

## 💡 Tips for Offline Development

1. **Keep a local copy of CMake cache:**
   - After first successful configure, the build directory contains all configuration

2. **Pre-download all dependencies:**
   - Google Test
   - SQLite3
   - Qt (already installed)

3. **Use vcpkg manifest mode:**
   - Create `vcpkg.json` in project root
   - Install all dependencies at once

4. **Documentation offline:**
   - All docs are in the repo (no internet needed)

---

## 🆘 Need Help?

If you encounter issues:

1. Check `build/CMakeCache.txt` for configuration values
2. Look at `build/CMakeFiles/CMakeError.log` for errors
3. Ensure all paths in the build script match your system
4. Verify Visual Studio has C++ tools installed

---

**Questions?** Let me know if you need help with any specific step!
