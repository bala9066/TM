# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for profiling module
# **************************************************************************

# Profiling module library
add_library(testmate_profiling
    PerformanceProfiler.cpp
)

target_include_directories(testmate_profiling
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(testmate_profiling PUBLIC cxx_std_20)

# Installation
install(TARGETS testmate_profiling
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
