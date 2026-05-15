# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for TestMATE debug library
# **************************************************************************

set(TARGET_NAME testmate_debug)

set(DEBUG_SOURCES
    Breakpoint.cpp
    BreakpointManager.cpp
    DebugSession.cpp
    DebugCLI.cpp
)

set(DEBUG_HEADERS
    ${CMAKE_SOURCE_DIR}/include/testmate/debug/Breakpoint.h
    ${CMAKE_SOURCE_DIR}/include/testmate/debug/BreakpointManager.h
    ${CMAKE_SOURCE_DIR}/include/testmate/debug/DebugSession.h
    ${CMAKE_SOURCE_DIR}/include/testmate/debug/DebugCLI.h
)

add_library(${TARGET_NAME} STATIC ${DEBUG_SOURCES} ${DEBUG_HEADERS})

target_link_libraries(${TARGET_NAME}
    PUBLIC
        testmate_utils
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)

# Thread support for std::mutex and std::thread
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} PUBLIC Threads::Threads)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
