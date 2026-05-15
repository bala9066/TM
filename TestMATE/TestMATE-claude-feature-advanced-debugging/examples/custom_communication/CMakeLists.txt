# **************************************************************************
# File Name: CMakeLists.txt
# Description: Build configuration for custom communication examples
# Author: TestMATE Development Team
# **************************************************************************

# Custom Communication Examples Library
# This library contains examples of custom Ethernet and UART protocols
# for DUT communication

add_library(custom_communication_examples STATIC
    EthernetConnection.cpp
    CustomUARTProtocol.cpp
    DUTCommTestStep.cpp
)

target_include_directories(custom_communication_examples
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(custom_communication_examples
    PUBLIC
        testmate_core
        testmate_utils
)

# Set C++20 standard
target_compile_features(custom_communication_examples PUBLIC cxx_std_20)

# Installation (optional - for SDK distribution)
install(TARGETS custom_communication_examples
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES
    EthernetConnection.h
    CustomUARTProtocol.h
    DUTCommTestStep.h
    DESTINATION include/examples/custom_communication
)

# Example executable (demonstrates usage)
add_executable(custom_comm_example_app
    example_main.cpp
)

target_link_libraries(custom_comm_example_app
    PRIVATE
        custom_communication_examples
        testmate_core
        testmate_utils
        testmate_database
)

target_include_directories(custom_comm_example_app
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)
