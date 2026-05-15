# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-22
# Description: CMake configuration for TestMATE Qt Application
# **************************************************************************

set(TARGET_NAME testmate_qt)

# Find Qt6 components
find_package(Qt6 COMPONENTS
    Core
    Widgets
    Charts
    Sql
    QUIET
)

# If Qt6 not found, try Qt5
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS
        Core
        Widgets
        Charts
        Sql
        QUIET
    )
endif()

# Skip Qt UI if neither Qt5 nor Qt6 is found
if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(STATUS "Qt not found - skipping Qt GUI application (optional)")
    return()
endif()

if(Qt6_FOUND)
    message(STATUS "Found Qt6 - building Qt GUI application")
    set(QT_VERSION_MAJOR 6)
else()
    message(STATUS "Found Qt5 - building Qt GUI application")
    set(QT_VERSION_MAJOR 5)
endif()

# Enable automatic MOC, UIC, and RCC for Qt Designer
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    main.cpp
    MainWindow.cpp
    StepPaletteWidget.cpp
    widgets/SequenceEditorWidget.cpp
    widgets/ExecutionMonitorWidget.cpp
    widgets/ReportViewerWidget.cpp
    widgets/ConfigurationDialog.cpp
    widgets/StepPropertiesDialog.cpp
    widgets/AboutDialog.cpp
    models/SequenceTreeModel.cpp
    models/ResultsTableModel.cpp
)

set(HEADERS
    MainWindow.h
    StepPaletteWidget.h
    widgets/SequenceEditorWidget.h
    widgets/ExecutionMonitorWidget.h
    widgets/ReportViewerWidget.h
    widgets/ConfigurationDialog.h
    widgets/StepPropertiesDialog.h
    widgets/AboutDialog.h
    models/SequenceTreeModel.h
    models/ResultsTableModel.h
)

set(RESOURCES
    resources/testmate.qrc
)

set(UI_FILES
    ui/MainWindow.ui
    ui/SequenceEditorWidget.ui
    ui/ExecutionMonitorWidget.ui
    ui/ReportViewerWidget.ui
    ui/ConfigurationDialog.ui
    ui/StepPropertiesDialog.ui
    ui/AboutDialog.ui
)

# UI files are processed by CMake's AUTOUIC
add_executable(${TARGET_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

if(Qt6_FOUND)
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            Qt6::Core
            Qt6::Widgets
            Qt6::Charts
            Qt6::Sql
            testmate_api
            testmate_core
            testmate_database
            testmate_ui
    )
else()
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            Qt5::Core
            Qt5::Widgets
            Qt5::Charts
            Qt5::Sql
            testmate_api
            testmate_core
            testmate_database
            testmate_ui
    )
endif()

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

# Set output directory
set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "TestMATE"
)

# Windows-specific settings
if(WIN32)
    set_target_properties(${TARGET_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# macOS-specific settings
if(APPLE)
    set_target_properties(${TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.testmate.app
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

# Installation
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
