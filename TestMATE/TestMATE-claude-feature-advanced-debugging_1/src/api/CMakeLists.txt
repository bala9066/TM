# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: CMake configuration for TestMATE API library
# **************************************************************************

set(TARGET_NAME testmate_api)

set(SOURCES
    TestMATECore.cpp
)

set(HEADERS
    ITestMATEApi.h
    TestMATECore.h
)

add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

target_link_libraries(${TARGET_NAME}
    PUBLIC
        testmate_core
        testmate_utils
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
