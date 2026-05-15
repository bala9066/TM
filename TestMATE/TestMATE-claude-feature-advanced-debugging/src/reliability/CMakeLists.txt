# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for reliability module
# **************************************************************************

# Reliability module library
add_library(testmate_reliability
    RetryPolicy.cpp
)

target_include_directories(testmate_reliability
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(testmate_reliability PUBLIC cxx_std_20)

# Installation
install(TARGETS testmate_reliability
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
