# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for REST API module
# **************************************************************************

# REST API module library
add_library(testmate_rest_api
    RestApiServer.cpp
)

target_include_directories(testmate_rest_api
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(testmate_rest_api PUBLIC cxx_std_20)

# Installation
install(TARGETS testmate_rest_api
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
