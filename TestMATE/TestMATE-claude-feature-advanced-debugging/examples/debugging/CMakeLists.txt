# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-11-23
# Description: CMake configuration for debugging examples
# **************************************************************************

add_executable(debug_example DebugExample.cpp)

target_link_libraries(debug_example
    PRIVATE
        testmate_debug
        testmate_core
        testmate_utils
)

target_include_directories(debug_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(debug_example PRIVATE cxx_std_20)

# Install the example
install(TARGETS debug_example DESTINATION bin/examples)
install(FILES DebugExample.cpp README.md DESTINATION share/testmate/examples/debugging)
