cmake_minimum_required(VERSION 3.15)
project(TestMATE_TestSteps VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for all test steps
set(TEST_STEP_SOURCES
    WaitStep.cpp
    LimitCheckStep.cpp
    CalculationStep.cpp
    InstrumentMeasureStep.cpp
    SerialCommandStep.cpp
    FileOperationStep.cpp
)

set(TEST_STEP_HEADERS
    WaitStep.h
    LimitCheckStep.h
    CalculationStep.h
    InstrumentMeasureStep.h
    SerialCommandStep.h
    FileOperationStep.h
)

# Create library of test steps
add_library(testmate_test_steps STATIC
    ${TEST_STEP_SOURCES}
    ${TEST_STEP_HEADERS}
)

# Include directories
target_include_directories(testmate_test_steps
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link TestMATE core libraries
target_link_libraries(testmate_test_steps
    PUBLIC
        testmate_core
        testmate_utils
)

# Installation
install(TARGETS testmate_test_steps
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${TEST_STEP_HEADERS}
    DESTINATION include/testmate/test_steps
)
