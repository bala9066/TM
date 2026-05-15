cmake_minimum_required(VERSION 3.15)

# Integration Tests
set(INTEGRATION_TEST_SOURCES
    EndToEndTests.cpp
)

# Create integration test executable
add_executable(testmate_integration_tests ${INTEGRATION_TEST_SOURCES})

# Include directories
target_include_directories(testmate_integration_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/examples
)

# Link libraries
target_link_libraries(testmate_integration_tests
    PRIVATE
        testmate_core
        testmate_database
        testmate_utils
        testmate_api
        testmate_test_steps
        gtest
        gtest_main
)

# Add integration tests to CTest
add_test(NAME IntegrationTests COMMAND testmate_integration_tests)

# Set test properties
set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 60
    LABELS "integration"
)
