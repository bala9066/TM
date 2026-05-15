cmake_minimum_required(VERSION 3.15)
project(CustomMeasurementStep VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    CustomMeasurementStep.cpp
    CustomMeasurementStep.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        testmate_core
        testmate_utils
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)
