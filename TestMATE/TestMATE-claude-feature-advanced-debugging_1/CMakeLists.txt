# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: Root CMake configuration for TestMATE project
# **************************************************************************

cmake_minimum_required(VERSION 3.21)

project(TestMATE
    VERSION 2.0.0
    DESCRIPTION "Test Management and Automation Tool Environment"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(TESTMATE_BUILD_TESTS "Build unit tests" ON)
option(TESTMATE_BUILD_DOCS "Build documentation" OFF)
option(TESTMATE_BUILD_EXAMPLES "Build example projects" OFF)
option(TESTMATE_ENABLE_COVERAGE "Enable code coverage" OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Platform detection
if(WIN32)
    add_definitions(-DTESTMATE_PLATFORM_WINDOWS)
elseif(APPLE)
    add_definitions(-DTESTMATE_PLATFORM_MACOS)
elseif(UNIX)
    add_definitions(-DTESTMATE_PLATFORM_LINUX)
endif()

# Find dependencies
# TODO: Enable these as components are implemented
# find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
# find_package(gRPC REQUIRED)
# find_package(PostgreSQL)
# find_package(Python3 COMPONENTS Interpreter Development)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Subdirectories - Enable as implemented
add_subdirectory(src/utils)
# add_subdirectory(src/models)
add_subdirectory(src/core)
add_subdirectory(src/api)
add_subdirectory(src/ui)
add_subdirectory(src/database)
add_subdirectory(src/debug)
add_subdirectory(src/reliability)
add_subdirectory(src/profiling)
add_subdirectory(src/rest_api)
add_subdirectory(src/orchestration)
add_subdirectory(src/resources)
add_subdirectory(src/qt_app)
# add_subdirectory(src/plugins)
# add_subdirectory(src/config_devices)
# add_subdirectory(src/scripting)
# add_subdirectory(src/remote)
# add_subdirectory(src/analytics)

# Tests
if(TESTMATE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(TESTMATE_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # add_subdirectory(docs)
    endif()
endif()

# Examples
if(TESTMATE_BUILD_EXAMPLES)
    add_subdirectory(examples/database)
    add_subdirectory(examples/custom_communication)
    add_subdirectory(examples/debugging)
endif()

# Installation
install(DIRECTORY include/testmate DESTINATION include)

# CPack configuration
set(CPACK_PACKAGE_NAME "TestMATE")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "TestMATE Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Test Management and Automation Tool Environment")
include(CPack)
