# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: CMake configuration for TestMATE utilities library
# **************************************************************************

set(TARGET_NAME testmate_utils)

set(SOURCES
    LogManager.cpp
    PerformanceProfiler.cpp
)

set(HEADERS
    LogManager.h
    StringUtils.h
    TimeUtils.h
)

add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Export for other modules
set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
