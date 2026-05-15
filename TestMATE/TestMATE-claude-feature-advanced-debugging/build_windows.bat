@echo off
REM **************************************************************************
REM TestMATE Windows Build Script
REM Supports both online and offline builds
REM **************************************************************************

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   TestMATE v2.0 - Windows Build
echo ========================================
echo.

REM ========================================
REM CONFIGURATION - UPDATE THESE PATHS
REM ========================================

REM Qt 6.8.2 installation path
set QT_ROOT=C:\Qt\6.8.2\msvc2022_64
if not exist "%QT_ROOT%" (
    echo ERROR: Qt not found at %QT_ROOT%
    echo Please update QT_ROOT in this script
    pause
    exit /b 1
)

REM SQLite3 path (optional - only if manually installed)
set SQLITE_ROOT=C:\sqlite3

REM CMake path (if not in system PATH)
set CMAKE_PATH=C:\Program Files\CMake\bin

REM Visual Studio version (2019 or 2022)
set VS_VERSION=2022

REM ========================================
REM ENVIRONMENT SETUP
REM ========================================

set PATH=%QT_ROOT%\bin;%CMAKE_PATH%;%PATH%

echo [1/7] Environment configured
echo   - Qt: %QT_ROOT%
echo   - CMake: %CMAKE_PATH%
echo   - VS: %VS_VERSION%
echo.

REM ========================================
REM BUILD CONFIGURATION
REM ========================================

:menu
echo Build Options:
echo   [1] Full Build (Core + Tests + GUI)
echo   [2] Core Only (No Tests, No GUI)
echo   [3] Core + Tests (No GUI)
echo   [4] Release Build (recommended)
echo   [5] Debug Build
echo   [Q] Quit
echo.
set /p choice="Select option: "

if /i "%choice%"=="Q" exit /b 0
if "%choice%"=="1" set BUILD_TYPE=FULL
if "%choice%"=="2" set BUILD_TYPE=CORE
if "%choice%"=="3" set BUILD_TYPE=TESTS
if "%choice%"=="4" set BUILD_CONFIG=Release
if "%choice%"=="5" set BUILD_CONFIG=Debug

if "%BUILD_TYPE%"=="" set BUILD_TYPE=FULL
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release

echo.
echo [2/7] Build configuration selected
echo   - Type: %BUILD_TYPE%
echo   - Config: %BUILD_CONFIG%
echo.

REM ========================================
REM CREATE BUILD DIRECTORY
REM ========================================

if not exist build mkdir build
cd build

echo [3/7] Build directory created
echo.

REM ========================================
REM CMAKE CONFIGURATION
REM ========================================

echo [4/7] Running CMake configuration...

set CMAKE_ARGS=-G "Visual Studio 17 %VS_VERSION%" -A x64 -DCMAKE_PREFIX_PATH=%QT_ROOT%

if "%BUILD_TYPE%"=="CORE" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DTESTMATE_BUILD_TESTS=OFF -DTESTMATE_BUILD_GUI=OFF
) else if "%BUILD_TYPE%"=="TESTS" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DTESTMATE_BUILD_TESTS=ON -DTESTMATE_BUILD_GUI=OFF
) else (
    set CMAKE_ARGS=%CMAKE_ARGS% -DTESTMATE_BUILD_TESTS=ON -DTESTMATE_BUILD_GUI=ON
)

if exist "%SQLITE_ROOT%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DSQLite3_ROOT=%SQLITE_ROOT%
)

REM Try to use local Google Test if available
if exist "..\external\googletest" (
    echo   - Using local Google Test
    set CMAKE_ARGS=%CMAKE_ARGS% -DFETCHCONTENT_FULLY_DISCONNECTED=ON
)

cmake .. %CMAKE_ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common issues:
    echo   - Qt path incorrect
    echo   - SQLite3 not found
    echo   - Visual Studio not installed
    echo   - CMake version too old
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

echo [5/7] Building TestMATE (%BUILD_CONFIG%)...
echo   This may take 2-5 minutes...
echo.

cmake --build . --config %BUILD_CONFIG% -j 4

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    echo Check the error messages above.
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

echo [6/7] Post-build setup...

REM Copy SQLite DLL if exists
if exist "%SQLITE_ROOT%\bin\sqlite3.dll" (
    copy "%SQLITE_ROOT%\bin\sqlite3.dll" "bin\%BUILD_CONFIG%\" >nul 2>&1
    echo   - SQLite DLL copied
)

REM Deploy Qt DLLs if GUI was built
if "%BUILD_TYPE%"=="FULL" (
    if exist "bin\%BUILD_CONFIG%\testmate_gui.exe" (
        echo   - Deploying Qt dependencies...
        cd bin\%BUILD_CONFIG%
        "%QT_ROOT%\bin\windeployqt.exe" --no-translations testmate_gui.exe >nul 2>&1
        cd ..\..
        echo   - Qt DLLs deployed
    )
)

echo.

REM ========================================
REM BUILD SUMMARY
REM ========================================

echo [7/7] Build Summary
echo ========================================
echo.
echo Build completed successfully!
echo.
echo Executables location: build\bin\%BUILD_CONFIG%\
echo.

if "%BUILD_TYPE%"=="FULL" (
    echo Available executables:
    echo   - testmate_gui.exe          ^(GUI Application^)
    echo   - testmate_unit_tests.exe   ^(Unit Tests^)
    echo.
) else if "%BUILD_TYPE%"=="TESTS" (
    echo Available executables:
    echo   - testmate_unit_tests.exe   ^(Unit Tests^)
    echo.
) else (
    echo Libraries built:
    echo   - testmate_core.lib
    echo   - testmate_debug.lib
    echo   - testmate_reliability.lib
    echo   And more...
    echo.
)

echo To run tests:
echo   cd build\bin\%BUILD_CONFIG%
echo   testmate_unit_tests.exe
echo.

if "%BUILD_TYPE%"=="FULL" (
    echo To run GUI:
    echo   cd build\bin\%BUILD_CONFIG%
    echo   testmate_gui.exe
    echo.
)

echo ========================================

REM Ask if user wants to run tests
if "%BUILD_TYPE%"=="FULL" goto ask_run
if "%BUILD_TYPE%"=="TESTS" goto ask_run
goto end

:ask_run
echo.
set /p run_tests="Run unit tests now? (Y/N): "
if /i "%run_tests%"=="Y" (
    echo.
    echo Running tests...
    echo.
    cd bin\%BUILD_CONFIG%
    testmate_unit_tests.exe
    cd ..\..
)

:end
cd ..
echo.
echo Press any key to exit...
pause >nul
