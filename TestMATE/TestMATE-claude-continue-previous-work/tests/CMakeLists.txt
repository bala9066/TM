# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: CMake configuration for TestMATE tests
# **************************************************************************

# Find GoogleTest
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # If not found, try to fetch it
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

# Unit tests executable
set(UNIT_TEST_SOURCES
    unit/utils/StringUtilsTests.cpp
    unit/utils/LogManagerTests.cpp
    unit/core/ProcessModelTests.cpp
    unit/core/ThreadingTests.cpp
    unit/core/ParallelBatchTests.cpp
    unit/core/PluginApiTests.cpp
    unit/core/TestSequenceTests.cpp
    unit/core/InstrumentTests.cpp
    unit/core/ReportingTests.cpp
    unit/core/SequenceFileIOTests.cpp
    unit/core/TestExecutorTests.cpp
    unit/core/ConfigManagerTests.cpp
    unit/core/SerialConnectionTests.cpp
    unit/core/HtmlReportGeneratorTests.cpp
    unit/core/StdfWriterTests.cpp
    unit/database/SqliteDataStoreTests.cpp
    unit/database/QueryParameterizedTests.cpp
)

add_executable(testmate_unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(testmate_unit_tests
    PRIVATE
        testmate_database
        testmate_ui
        testmate_api
        testmate_core
        testmate_utils
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(testmate_unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(testmate_unit_tests)

# **************************************************************************
# Qt GUI Tests (optional - only built if Qt is available)
# **************************************************************************

# Try to find Qt6 first, then Qt5
find_package(Qt6 COMPONENTS Core Widgets Test QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Widgets Test QUIET)
endif()

if(Qt6_FOUND OR Qt5_FOUND)
    message(STATUS "Qt found - building Qt GUI tests")

    # Enable Qt's automated features
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    # Qt test source files
    set(QT_TEST_SOURCES
        unit/qt_app/MainWindowTests.cpp
        unit/qt_app/SequenceEditorWidgetTests.cpp
        unit/qt_app/ExecutionMonitorWidgetTests.cpp
        unit/qt_app/DialogTests.cpp
        unit/qt_app/ModelTests.cpp
    )

    # Determine Qt version
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
        set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::Test)
    else()
        set(QT_VERSION_MAJOR 5)
        set(QT_LIBS Qt5::Core Qt5::Widgets Qt5::Test)
    endif()

    # Create test executables (one per test file for better isolation)

    # MainWindow tests
    add_executable(qt_mainwindow_tests unit/qt_app/MainWindowTests.cpp)
    target_link_libraries(qt_mainwindow_tests
        PRIVATE
            ${QT_LIBS}
            testmate_core
            testmate_utils
            testmate_api
    )
    target_include_directories(qt_mainwindow_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME QtMainWindowTests COMMAND qt_mainwindow_tests)

    # SequenceEditorWidget tests
    add_executable(qt_sequenceeditor_tests unit/qt_app/SequenceEditorWidgetTests.cpp)
    target_link_libraries(qt_sequenceeditor_tests
        PRIVATE
            ${QT_LIBS}
            testmate_core
            testmate_utils
            testmate_api
    )
    target_include_directories(qt_sequenceeditor_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME QtSequenceEditorTests COMMAND qt_sequenceeditor_tests)

    # ExecutionMonitorWidget tests
    add_executable(qt_executionmonitor_tests unit/qt_app/ExecutionMonitorWidgetTests.cpp)
    target_link_libraries(qt_executionmonitor_tests
        PRIVATE
            ${QT_LIBS}
            testmate_core
            testmate_utils
            testmate_api
    )
    target_include_directories(qt_executionmonitor_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME QtExecutionMonitorTests COMMAND qt_executionmonitor_tests)

    # Dialog tests
    add_executable(qt_dialog_tests unit/qt_app/DialogTests.cpp)
    target_link_libraries(qt_dialog_tests
        PRIVATE
            ${QT_LIBS}
            testmate_core
            testmate_utils
            testmate_api
    )
    target_include_directories(qt_dialog_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME QtDialogTests COMMAND qt_dialog_tests)

    # Model tests
    add_executable(qt_model_tests unit/qt_app/ModelTests.cpp)
    target_link_libraries(qt_model_tests
        PRIVATE
            ${QT_LIBS}
            testmate_core
            testmate_utils
            testmate_api
    )
    target_include_directories(qt_model_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME QtModelTests COMMAND qt_model_tests)

    message(STATUS "Qt GUI tests configured successfully")
else()
    message(STATUS "Qt not found - skipping Qt GUI tests (optional)")
endif()

# Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    message(STATUS "Adding integration tests")
    add_subdirectory(integration)
endif()

