
REM **************************************************************************
REM TestMATE Windows Build Script for MinGW
REM Supports both online and offline builds with MinGW compiler
REM **************************************************************************

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   TestMATE v2.0 - MinGW Build
echo ========================================
echo.

REM ========================================
REM CONFIGURATION - UPDATE THESE PATHS
REM ========================================

REM Qt 6.8.2 MinGW installation path
set QT_ROOT=C:\Qt\Qt6.8\6.8.2\mingw_64
if not exist "%QT_ROOT%" (
    echo ERROR: Qt MinGW not found at %QT_ROOT%
    echo.
    echo Expected path: C:\Qt\6.8.2\mingw_64
    echo Your Qt installation may use a different path.
    echo.
    echo Common Qt MinGW paths:
    echo   C:\Qt\6.8.2\mingw_64
    echo   C:\Qt\6.8.2\mingw1120_64
    echo   C:\Qt\6.8.2\mingw_81
    echo.
    set /p QT_ROOT="Enter your Qt MinGW path: "
    if not exist "!QT_ROOT!" (
        echo ERROR: Path does not exist!
        pause
        exit /b 1
    )
)

REM MinGW bin directory (should contain g++.exe, mingw32-make.exe)
REM Usually comes with Qt installation
set MINGW_BIN=C:\Qt\Qt6.8\Tools\mingw1310_64\bin
if not exist "%MINGW_BIN%\g++.exe" (
    REM Try alternate locations
    set MINGW_BIN=%QT_ROOT%\..\..\Tools\mingw1120_64\bin
    if not exist "!MINGW_BIN!\g++.exe" (
        set MINGW_BIN=%QT_ROOT%\bin
        if not exist "!MINGW_BIN!\g++.exe" (
            echo WARNING: MinGW compiler not found automatically
            echo Please ensure MinGW is in your PATH
            echo.
            set MINGW_BIN=
        )
    )
)

REM SQLite3 path (optional - only if manually installed)
set SQLITE_ROOT=D:\Sathishkumar_K\Qt\Development\external\sqlite3

REM CMake path (if not in system PATH)
set CMAKE_PATH=C:\Qt\Qt6.8\Tools\CMake_64\bin

REM ========================================
REM ENVIRONMENT SETUP
REM ========================================

REM Add to PATH
if defined MINGW_BIN (
REM    set PATH=%MINGW_BIN%;%QT_ROOT%\bin;%CMAKE_PATH%;%PATH%
	set PATH=C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files\netCDF 4.8.1\bin;C:\Program Files\HDF_Group\HDF5\1.10.6\bin\;C;C:\Program Files\IVI Foundation\VISA\Win64\Bin\;C:\Program Files\Microsoft SQL Server\150\Tools\Binn\;C:\Program Files\Microsoft SQL Server\Client SDK\ODBC\170\Tools\Binn\;C:\Program Files\Microsoft SQL Server\150\DTS\Binn\;C:\texlive\2019\bin\win32;C:\Program Files\doxygen\bin;C:\Users\sathish.kailasam\AppData\Local\Microsoft\WindowsApps;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;
) else (
REM    set PATH=%QT_ROOT%\bin;%CMAKE_PATH%;%PATH%
	set PATH=C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files\netCDF 4.8.1\bin;C:\Program Files\HDF_Group\HDF5\1.10.6\bin\;C;C:\Program Files\IVI Foundation\VISA\Win64\Bin\;C:\Program Files\Microsoft SQL Server\150\Tools\Binn\;C:\Program Files\Microsoft SQL Server\Client SDK\ODBC\170\Tools\Binn\;C:\Program Files\Microsoft SQL Server\150\DTS\Binn\;C:\texlive\2019\bin\win32;C:\Program Files\doxygen\bin;C:\Users\sathish.kailasam\AppData\Local\Microsoft\WindowsApps;C:\Qt\Qt6.8\Tools\CMake_64\bin;C:\Qt\Qt6.8\Tools\mingw1310_64\bin;C:\Qt\Qt6.8\6.8.2\mingw_64\bin;C:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;
)

echo [1/7] Environment configured
echo   - Qt: %QT_ROOT%
if defined MINGW_BIN (
    echo   - MinGW: %MINGW_BIN%
) else (
    echo   - MinGW: Using system PATH
)
echo   - CMake: %CMAKE_PATH%
echo.

REM Check for required tools
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found in PATH!
    echo Please ensure MinGW is installed and in PATH
    pause
    exit /b 1
)

where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    where make >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: mingw32-make or make not found in PATH!
        echo Please ensure MinGW is installed and in PATH
        pause
        exit /b 1
    )
    set MAKE_CMD=make
) else (
    set MAKE_CMD=mingw32-make
)

echo   - Compiler: g++ (found)
echo   - Make: %MAKE_CMD% (found)
echo.

REM ========================================
REM BUILD CONFIGURATION
REM ========================================

:menu
echo Build Options:
echo   [1] Release Build (optimized, recommended)
echo   [2] Debug Build (with debug symbols)
echo   [3] Full Build with Tests (Release)
echo   [4] Core Only (no tests, no GUI)
echo   [Q] Quit
echo.
set /p choice="Select option: "

if /i "%choice%"=="Q" exit /b 0

set BUILD_TESTS=ON
set BUILD_GUI=ON
set BUILD_TYPE=Release

if "%choice%"=="1" (
    set BUILD_TYPE=Release
    echo.
    echo Selected: Release Build
)
if "%choice%"=="2" (
    set BUILD_TYPE=Debug
    echo.
    echo Selected: Debug Build
)
if "%choice%"=="3" (
    set BUILD_TYPE=Release
    set BUILD_TESTS=ON
    echo.
    echo Selected: Full Build with Tests
)
if "%choice%"=="4" (
    set BUILD_TESTS=OFF
    set BUILD_GUI=OFF
    echo.
    echo Selected: Core Only
)

echo   - Build Type: %BUILD_TYPE%
echo   - Tests: %BUILD_TESTS%
echo   - GUI: %BUILD_GUI%
echo.

REM ========================================
REM CREATE BUILD DIRECTORY
REM ========================================

if not exist build mkdir build
cd build

echo [2/7] Build directory created
echo.

REM ========================================
REM CMAKE CONFIGURATION
REM ========================================

echo [3/7] Running CMake configuration...

set CMAKE_ARGS=-G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_PREFIX_PATH=%QT_ROOT%

set CMAKE_ARGS=%CMAKE_ARGS% -DTESTMATE_BUILD_TESTS=%BUILD_TESTS%
set CMAKE_ARGS=%CMAKE_ARGS% -DTESTMATE_BUILD_GUI=%BUILD_GUI%

if exist "%SQLITE_ROOT%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DSQLite3_ROOT=%SQLITE_ROOT%
    echo   - Using SQLite at: %SQLITE_ROOT%
)

REM Try to use local Google Test if available
if exist "..\external\googletest" (
    echo   - Using local Google Test
    set CMAKE_ARGS=%CMAKE_ARGS% -DFETCHCONTENT_FULLY_DISCONNECTED=ON
)

echo   - Generator: MinGW Makefiles
echo   - Build Type: %BUILD_TYPE%
echo.

cmake .. %CMAKE_ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common issues:
    echo   - Qt path incorrect (check QT_ROOT)
    echo   - MinGW not in PATH
    echo   - CMake version too old (need 3.15+)
    echo.
    echo Current Qt path: %QT_ROOT%
    echo Check if this path contains: bin\qmake.exe
    echo.
    pause
    cd ..
    exit /b 1
)

echo   - Configuration successful
echo.

REM ========================================
REM BUILD
REM ========================================

echo [4/7] Building TestMATE with %MAKE_CMD%...
echo   This may take 2-5 minutes...
echo.

REM Use parallel build if supported
%MAKE_CMD% -j4

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    echo.
    echo Try building without parallel jobs:
    echo   cd build
    echo   %MAKE_CMD%
    echo.
    pause
    cd ..
    exit /b 1
)

echo   - Build successful
echo.

REM ========================================
REM POST-BUILD
REM ========================================

echo [5/7] Post-build setup...

REM Copy SQLite DLL if exists
if exist "%SQLITE_ROOT%\bin\sqlite3.dll" (
    copy "%SQLITE_ROOT%\bin\sqlite3.dll" "bin\" >nul 2>&1
    echo   - SQLite DLL copied
)

REM Deploy Qt DLLs if GUI was built
if "%BUILD_GUI%"=="ON" (
    if exist "bin\testmate_gui.exe" (
        echo   - Deploying Qt dependencies...
        cd bin
        "%QT_ROOT%\bin\windeployqt.exe" --no-translations testmate_gui.exe >nul 2>&1
        cd ..
        echo   - Qt DLLs deployed
    )
)

echo.

REM ========================================
REM VERIFY BUILD
REM ========================================

echo [6/7] Verifying build...

set ALL_GOOD=1

if "%BUILD_GUI%"=="ON" (
    if exist "bin\testmate_gui.exe" (
        echo   - testmate_gui.exe: OK
    ) else (
        echo   - testmate_gui.exe: MISSING
        set ALL_GOOD=0
    )
)

if "%BUILD_TESTS%"=="ON" (
    if exist "bin\testmate_unit_tests.exe" (
        echo   - testmate_unit_tests.exe: OK
    ) else (
        echo   - testmate_unit_tests.exe: MISSING
        set ALL_GOOD=0
    )
)

if exist "lib\libtestmate_core.a" (
    echo   - Core libraries: OK
) else (
    echo   - Core libraries: MISSING
    set ALL_GOOD=0
)

echo.

if %ALL_GOOD%==0 (
    echo WARNING: Some build artifacts are missing
    echo Check the build log for errors
    echo.
)

REM ========================================
REM BUILD SUMMARY
REM ========================================

echo [7/7] Build Summary
echo ========================================
echo.
echo Build completed successfully!
echo.
echo Build Type: %BUILD_TYPE%
echo Executables location: build\bin\
echo Libraries location: build\lib\
echo.

if "%BUILD_GUI%"=="ON" (
    echo Available executables:
    echo   - testmate_gui.exe          ^(GUI Application^)
)

if "%BUILD_TESTS%"=="ON" (
    echo   - testmate_unit_tests.exe   ^(Unit Tests - 135 tests^)
)

echo.
echo Libraries built:
echo   - libtestmate_core.a
echo   - libtestmate_debug.a
echo   - libtestmate_reliability.a
echo   - libtestmate_profiling.a
echo   - libtestmate_rest_api.a
echo   - libtestmate_orchestration.a
echo   - libtestmate_resources.a
echo.

echo To run tests:
echo   cd build\bin
echo   testmate_unit_tests.exe
echo.

if "%BUILD_GUI%"=="ON" (
    echo To run GUI:
    echo   cd build\bin
    echo   testmate_gui.exe
    echo.
)

echo To rebuild:
echo   cd build
echo   %MAKE_CMD% clean
echo   %MAKE_CMD% -j4
echo.

echo ========================================

REM Ask if user wants to run tests
if "%BUILD_TESTS%"=="ON" (
    echo.
    set /p run_tests="Run unit tests now? (Y/N): "
    if /i "!run_tests!"=="Y" (
        echo.
        echo Running tests...
        echo.
        cd bin
        testmate_unit_tests.exe
        cd ..
    )
)

cd ..
echo.
echo Press any key to exit...
pause >nul
