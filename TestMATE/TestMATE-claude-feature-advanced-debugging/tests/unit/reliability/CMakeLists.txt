# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for reliability unit tests
# **************************************************************************

add_executable(retry_policy_tests
    RetryPolicyTests.cpp
)

target_link_libraries(retry_policy_tests
    PRIVATE
        testmate_reliability
        testmate_common
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(retry_policy_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(retry_policy_tests PRIVATE cxx_std_20)

# Add to CTest
add_test(NAME RetryPolicyTests COMMAND retry_policy_tests)
