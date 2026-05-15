# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for orchestration module
# **************************************************************************

# Orchestration module library
add_library(testmate_orchestration
    TestOrchestrator.cpp
)

target_include_directories(testmate_orchestration
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(testmate_orchestration PUBLIC cxx_std_20)

# Installation
install(TARGETS testmate_orchestration
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
