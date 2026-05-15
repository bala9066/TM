# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: CMake configuration for TestMATE core library
# **************************************************************************

set(TARGET_NAME testmate_core)

set(PROCESS_MODEL_SOURCES
    process_models/ProcessModelBase.cpp
    process_models/SequentialModel.cpp
    process_models/ParallelModel.cpp
    process_models/BatchModel.cpp
    process_models/ExecutionContext.cpp
)

set(PROCESS_MODEL_HEADERS
    process_models/IProcessModel.h
    process_models/ProcessModelBase.h
    process_models/SequentialModel.h
    process_models/ParallelModel.h
    process_models/BatchModel.h
    process_models/ExecutionContext.h
)

set(THREADING_SOURCES
    threading/ThreadPool.cpp
    threading/SyncPoint.cpp
)

set(THREADING_HEADERS
    threading/ThreadPool.h
    threading/SyncPoint.h
)

set(SCHEDULING_SOURCES
    scheduling/ResourceScheduler.cpp
)

set(SCHEDULING_HEADERS
    scheduling/ResourceScheduler.h
)

set(PLUGIN_SOURCES
    plugins/PluginManager.cpp
)

set(PLUGIN_HEADERS
    plugins/IPlugin.h
    plugins/PluginManager.h
)

set(TEST_SEQUENCE_SOURCES
    test_sequence/TestStepBase.cpp
    test_sequence/TestSequence.cpp
    test_sequence/SequenceFileIO.cpp
)

set(TEST_SEQUENCE_HEADERS
    test_sequence/ITestStep.h
    test_sequence/TestSequence.h
    test_sequence/SequenceFileIO.h
)

set(EXECUTION_SOURCES
    execution/TestExecutor.cpp
)

set(EXECUTION_HEADERS
    execution/TestExecutor.h
)

set(CONFIG_SOURCES
    config/ConfigManager.cpp
)

set(CONFIG_HEADERS
    config/ConfigManager.h
)

set(DATA_SOURCES
    data/DataBinding.cpp
)

set(DATA_HEADERS
    data/DataBinding.h
)

set(INSTRUMENT_SOURCES
    instruments/InstrumentBase.cpp
    instruments/InstrumentManager.cpp
)

set(INSTRUMENT_HEADERS
    instruments/IInstrument.h
    instruments/InstrumentManager.h
)

set(COMMUNICATION_SOURCES
    communication/Connection.cpp
    communication/SerialConnection.cpp
)

set(COMMUNICATION_HEADERS
    communication/IConnection.h
    communication/SerialConnection.h
)

set(REPORTING_SOURCES
    reporting/ReportGenerator.cpp
    reporting/HtmlReportGenerator.cpp
)

set(REPORTING_HEADERS
    reporting/IReportGenerator.h
    reporting/HtmlReportGenerator.h
)

set(SEMICONDUCTOR_SOURCES
    semiconductor/DeviceHandler.cpp
    semiconductor/StdfWriter.cpp
)

set(SEMICONDUCTOR_HEADERS
    semiconductor/DeviceHandler.h
    semiconductor/StdfWriter.h
)

set(SOURCES
    ${PROCESS_MODEL_SOURCES}
    ${THREADING_SOURCES}
    ${SCHEDULING_SOURCES}
    ${PLUGIN_SOURCES}
    ${TEST_SEQUENCE_SOURCES}
    ${EXECUTION_SOURCES}
    ${CONFIG_SOURCES}
    ${DATA_SOURCES}
    ${INSTRUMENT_SOURCES}
    ${COMMUNICATION_SOURCES}
    ${REPORTING_SOURCES}
    ${SEMICONDUCTOR_SOURCES}
)

set(HEADERS
    ${PROCESS_MODEL_HEADERS}
    ${THREADING_HEADERS}
    ${SCHEDULING_HEADERS}
    ${PLUGIN_HEADERS}
    ${TEST_SEQUENCE_HEADERS}
    ${EXECUTION_HEADERS}
    ${CONFIG_HEADERS}
    ${DATA_HEADERS}
    ${INSTRUMENT_HEADERS}
    ${COMMUNICATION_HEADERS}
    ${REPORTING_HEADERS}
    ${SEMICONDUCTOR_HEADERS}
)

add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

target_link_libraries(${TARGET_NAME}
    PUBLIC
        testmate_utils
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)

# Platform-specific threading
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} PUBLIC Threads::Threads)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
