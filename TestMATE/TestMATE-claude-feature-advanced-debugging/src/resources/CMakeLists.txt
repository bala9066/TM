# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for TestMATE resource management
# **************************************************************************

# Resource management library
set(RESOURCE_SOURCES
    InstrumentPool.cpp
)

add_library(testmate_resources STATIC ${RESOURCE_SOURCES})

target_include_directories(testmate_resources
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(testmate_resources
    PUBLIC
        testmate_core
)

# Set compile features
target_compile_features(testmate_resources PUBLIC cxx_std_20)
