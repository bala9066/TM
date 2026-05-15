cmake_minimum_required(VERSION 3.15)
project(SimpleMultimeter VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    SimpleMultimeter.cpp
    SimpleMultimeter.h
)

# Create shared library (plugin)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src  # TestMATE source headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include  # TestMATE public headers
)

# Link TestMATE libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        testmate_core
        testmate_utils
)

# Set output directory for plugin
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Installation
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

# Platform-specific settings
if(WIN32)
    # Windows: .dll extension
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    # macOS: .dylib extension
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    # Linux: .so extension
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()
