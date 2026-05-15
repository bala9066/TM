# TestMATE Project Folder Structure

Version 1.0
Author: TestMATE Development Team
Date: 2025-01-XX

---

## Complete Project Structure

```
TestMATE/
|
|-- CMakeLists.txt                      # Root CMake configuration
|-- README.md                            # Project overview and quick start
|-- LICENSE                              # Software license
|-- .gitignore                           # Git ignore rules
|-- .clang-format                        # Code formatting rules
|-- .clang-tidy                          # Static analysis rules
|
|-- cmake/                               # CMake modules and scripts
|   |-- CompilerFlags.cmake              # Platform-specific compiler flags
|   |-- FindQt6.cmake                    # Qt6 detection
|   |-- FindgRPC.cmake                   # gRPC detection
|   |-- FindPostgreSQL.cmake             # PostgreSQL detection
|   |-- FindPython.cmake                 # Python detection
|   |-- TestMATEVersion.cmake            # Version management
|   |-- CodeCoverage.cmake               # Code coverage support
|   `-- Packaging.cmake                  # CPack configuration
|
|-- docs/                                # Documentation
|   |-- SoftwareRequirement/             # SRS documents (Parts A-I)
|   |   |-- Part_A_TestMATE_SRS-Core_Process_Models.md
|   |   |-- Part_B_TestMATE_SRS_Semiconductor_Testing.md
|   |   |-- Part_C_TestMATE_SRS_Reporting_Analytics.md
|   |   |-- Part_D_TestMATE_SRS_Deployment_Licensing.md
|   |   |-- Part_E_TestMATE_SRS_Python_Integration.md
|   |   |-- Part_F_TestMATE_SRS_User_Interface_Enhancements.md
|   |   |-- Part_G_TestMATE_SRS_Integration_System_Requirements.md
|   |   |-- Part_H_TestMATE_SRS_Test_Steps_System.md
|   |   `-- Part_I_TestMATE_SRS_Configuration-Based_Device_System.md
|   |
|   |-- CodingGuideline/                 # Coding standards
|   |   `-- SDG-Coding-Guidelines_3.txt
|   |
|   |-- architecture/                    # Architecture documents
|   |   |-- TestMATE_Architecture.md     # System architecture
|   |   |-- Folder_Structure.md          # This document
|   |   |-- Implementation_Plan.md       # Phased implementation plan
|   |   `-- diagrams/                    # Architecture diagrams
|   |       |-- system_overview.puml
|   |       |-- component_diagram.puml
|   |       `-- sequence_diagrams/
|   |
|   |-- api/                             # API documentation
|   |   |-- public_api.md                # Public API reference
|   |   |-- plugin_api.md                # Plugin development API
|   |   `-- remote_api.md                # gRPC/REST API reference
|   |
|   |-- user_guide/                      # User documentation
|   |   |-- getting_started.md
|   |   |-- test_plan_creation.md
|   |   |-- parallel_testing.md
|   |   |-- pin_mapping.md
|   |   `-- reporting.md
|   |
|   `-- developer_guide/                 # Developer documentation
|       |-- building.md                  # Build instructions
|       |-- plugin_development.md        # Creating plugins
|       |-- coding_standards.md          # Coding guidelines summary
|       `-- testing.md                   # Test guidelines
|
|-- include/                             # Public headers
|   `-- testmate/
|       |-- api/                         # Public API headers
|       |   |-- ITestExecutionAPI.h
|       |   |-- ITestPlanAPI.h
|       |   |-- IResourceAPI.h
|       |   |-- IReportingAPI.h
|       |   |-- IConfigurationAPI.h
|       |   `-- APITypes.h
|       |
|       |-- interfaces/                  # Interface definitions
|       |   |-- ITestStep.h
|       |   |-- IInstrument.h
|       |   |-- IDut.h
|       |   |-- IProcessModel.h
|       |   |-- IEventSubscriber.h
|       |   |-- IPlugin.h
|       |   `-- ICapability.h
|       |
|       `-- common/                      # Common types and utilities
|           |-- Types.h                  # Common type definitions
|           |-- Result.h                 # Error result class
|           |-- Version.h                # Version information
|           `-- Export.h                 # DLL export macros
|
|-- src/                                 # Source code
|   |-- main.cpp                         # Application entry point
|   |
|   |-- core/                            # Core business logic (NO Qt)
|   |   |-- CMakeLists.txt
|   |   |-- TestMATECore.h               # Main core singleton
|   |   |-- TestMATECore.cpp
|   |   |
|   |   |-- process_models/              # Process model implementations
|   |   |   |-- IProcessModel.h          # Interface
|   |   |   |-- ProcessModelBase.h       # Abstract base
|   |   |   |-- ProcessModelBase.cpp
|   |   |   |-- SequentialModel.h        # Sequential execution
|   |   |   |-- SequentialModel.cpp
|   |   |   |-- ParallelModel.h          # Multi-socket parallel
|   |   |   |-- ParallelModel.cpp
|   |   |   |-- BatchModel.h             # Batch execution
|   |   |   |-- BatchModel.cpp
|   |   |   |-- ProcessModelFactory.h
|   |   |   |-- ProcessModelFactory.cpp
|   |   |   `-- ExecutionContext.h
|   |   |
|   |   |-- threading/                   # Threading utilities
|   |   |   |-- ThreadPool.h
|   |   |   |-- ThreadPool.cpp
|   |   |   |-- TaskQueue.h
|   |   |   |-- TaskQueue.cpp
|   |   |   |-- SyncPoint.h
|   |   |   |-- SyncPoint.cpp
|   |   |   |-- Barrier.h
|   |   |   |-- Barrier.cpp
|   |   |   |-- Semaphore.h
|   |   |   `-- Semaphore.cpp
|   |   |
|   |   |-- scheduling/                  # Resource scheduling
|   |   |   |-- ResourceScheduler.h
|   |   |   |-- ResourceScheduler.cpp
|   |   |   |-- ResourceRequest.h
|   |   |   |-- ResourceSet.h
|   |   |   |-- DeadlockDetector.h
|   |   |   `-- DeadlockDetector.cpp
|   |   |
|   |   |-- execution/                   # Test execution engine
|   |   |   |-- TestExecutionEngine.h
|   |   |   |-- TestExecutionEngine.cpp
|   |   |   |-- StepExecutor.h
|   |   |   |-- StepExecutor.cpp
|   |   |   |-- CallbackManager.h
|   |   |   `-- CallbackManager.cpp
|   |   |
|   |   |-- managers/                    # System managers
|   |   |   |-- PluginManager.h
|   |   |   |-- PluginManager.cpp
|   |   |   |-- ConfigManager.h
|   |   |   |-- ConfigManager.cpp
|   |   |   |-- LicenseManager.h
|   |   |   |-- LicenseManager.cpp
|   |   |   |-- ResultManager.h
|   |   |   |-- ResultManager.cpp
|   |   |   |-- ResourceManager.h
|   |   |   `-- ResourceManager.cpp
|   |   |
|   |   |-- events/                      # Event system
|   |   |   |-- EventPublisher.h
|   |   |   |-- EventPublisher.cpp
|   |   |   |-- EventTypes.h
|   |   |   `-- EventData.h
|   |   |
|   |   `-- semiconductor/               # Semiconductor testing
|   |       |-- PinMap.h
|   |       |-- PinMap.cpp
|   |       |-- SiteManager.h
|   |       |-- SiteManager.cpp
|   |       |-- BinningEngine.h
|   |       |-- BinningEngine.cpp
|   |       |-- LimitsManager.h
|   |       `-- LimitsManager.cpp
|   |
|   |-- api/                             # API/Facade layer
|   |   |-- CMakeLists.txt
|   |   |-- ITestExecutionAPI.h
|   |   |-- ITestPlanAPI.h
|   |   |-- IResourceAPI.h
|   |   |-- IReportingAPI.h
|   |   |-- implementations/
|   |   |   |-- TestExecutionAPIImpl.h
|   |   |   |-- TestExecutionAPIImpl.cpp
|   |   |   |-- TestPlanAPIImpl.h
|   |   |   |-- TestPlanAPIImpl.cpp
|   |   |   |-- ResourceAPIImpl.h
|   |   |   |-- ResourceAPIImpl.cpp
|   |   |   |-- ReportingAPIImpl.h
|   |   |   `-- ReportingAPIImpl.cpp
|   |   `-- APIFactory.h
|   |
|   |-- models/                          # Data models (Pure C++)
|   |   |-- CMakeLists.txt
|   |   |-- TestPlan.h
|   |   |-- TestPlan.cpp
|   |   |-- TestSequence.h
|   |   |-- TestSequence.cpp
|   |   |-- TestStep.h
|   |   |-- TestStep.cpp
|   |   |-- TestResult.h
|   |   |-- TestResult.cpp
|   |   |-- Measurement.h
|   |   |-- Measurement.cpp
|   |   |-- Component.h                  # Base for Instrument/DUT
|   |   |-- Component.cpp
|   |   |-- Instrument.h
|   |   |-- Instrument.cpp
|   |   |-- Dut.h
|   |   |-- Dut.cpp
|   |   |-- Resource.h
|   |   |-- Resource.cpp
|   |   |-- PinMapData.h                 # Pin map data structures
|   |   |-- LimitData.h                  # Limit definitions
|   |   `-- BinData.h                    # Binning definitions
|   |
|   |-- plugins/                         # Plugin system
|   |   |-- CMakeLists.txt
|   |   |-- interfaces/
|   |   |   |-- ITestStep.h
|   |   |   |-- IInstrument.h
|   |   |   |-- IDut.h
|   |   |   |-- IPlugin.h
|   |   |   `-- ICapability.h
|   |   |
|   |   |-- base/
|   |   |   |-- TestStepBase.h
|   |   |   |-- TestStepBase.cpp
|   |   |   |-- InstrumentBase.h
|   |   |   |-- InstrumentBase.cpp
|   |   |   |-- DutBase.h
|   |   |   `-- DutBase.cpp
|   |   |
|   |   |-- loader/
|   |   |   |-- PluginLoader.h
|   |   |   |-- PluginLoader.cpp
|   |   |   |-- PluginRegistry.h
|   |   |   `-- PluginRegistry.cpp
|   |   |
|   |   |-- properties/
|   |   |   |-- PropertySystem.h
|   |   |   |-- PropertySystem.cpp
|   |   |   |-- PropertyAttribute.h
|   |   |   `-- PropertyValidator.h
|   |   |
|   |   `-- standard_plugins/            # Built-in plugins
|   |       |-- test_steps/
|   |       |   |-- PassStep.{h,cpp}
|   |       |   |-- FailStep.{h,cpp}
|   |       |   |-- MessageStep.{h,cpp}
|   |       |   |-- DelayStep.{h,cpp}
|   |       |   |-- NumericLimitTest.{h,cpp}
|   |       |   |-- IfThenElseStep.{h,cpp}
|   |       |   |-- RepeatStep.{h,cpp}
|   |       |   |-- WhileLoopStep.{h,cpp}
|   |       |   |-- SetVariableStep.{h,cpp}
|   |       |   `-- CallSequenceStep.{h,cpp}
|   |       |
|   |       |-- instruments/
|   |       |   |-- ScpiInstrument.{h,cpp}
|   |       |   |-- GenericDmm.{h,cpp}
|   |       |   |-- GenericPowerSupply.{h,cpp}
|   |       |   `-- GenericSignalGenerator.{h,cpp}
|   |       |
|   |       `-- duts/
|   |           |-- SerialDut.{h,cpp}
|   |           `-- NetworkDut.{h,cpp}
|   |
|   |-- config_devices/                  # Configuration-based devices
|   |   |-- CMakeLists.txt
|   |   |-- ConfigDeviceFactory.h
|   |   |-- ConfigDeviceFactory.cpp
|   |   |-- ConfigParser.h
|   |   |-- ConfigParser.cpp
|   |   |-- protocols/
|   |   |   |-- IProtocolHandler.h
|   |   |   |-- ScpiProtocol.{h,cpp}
|   |   |   |-- ModbusProtocol.{h,cpp}
|   |   |   |-- CustomBinaryProtocol.{h,cpp}
|   |   |   `-- CustomTextProtocol.{h,cpp}
|   |   |
|   |   |-- communication/
|   |   |   |-- ICommunicationProvider.h
|   |   |   |-- SerialProvider.{h,cpp}
|   |   |   |-- TcpProvider.{h,cpp}
|   |   |   |-- UdpProvider.{h,cpp}
|   |   |   |-- VisaProvider.{h,cpp}
|   |   |   `-- UsbProvider.{h,cpp}
|   |   |
|   |   `-- ui_generator/
|   |       |-- DynamicUIGenerator.h
|   |       `-- DynamicUIGenerator.cpp
|   |
|   |-- ui/                              # UI layer (Qt)
|   |   |-- CMakeLists.txt
|   |   |
|   |   |-- adapters/                    # Qt adapters for core
|   |   |   |-- ExecutionAdapter.h
|   |   |   |-- ExecutionAdapter.cpp
|   |   |   |-- TestPlanAdapter.h
|   |   |   |-- TestPlanAdapter.cpp
|   |   |   |-- ResourceAdapter.h
|   |   |   |-- ResourceAdapter.cpp
|   |   |   |-- ModelConverters.h        # Core <-> Qt type conversion
|   |   |   `-- ModelConverters.cpp
|   |   |
|   |   |-- main_window/
|   |   |   |-- MainWindow.h
|   |   |   |-- MainWindow.cpp
|   |   |   |-- MainWindow.ui
|   |   |   |-- ExecutionToolbar.{h,cpp}
|   |   |   |-- StatusBarManager.{h,cpp}
|   |   |   `-- SocketStatusPanel.{h,cpp}
|   |   |
|   |   |-- test_plan_editor/
|   |   |   |-- TestPlanEditor.h
|   |   |   |-- TestPlanEditor.cpp
|   |   |   |-- TestPlanEditor.ui
|   |   |   |-- TestTreeView.{h,cpp}
|   |   |   |-- StepPropertyEditor.{h,cpp}
|   |   |   |-- ProcessModelConfig.{h,cpp}
|   |   |   `-- ParameterFlowView.{h,cpp}
|   |   |
|   |   |-- pin_map_editor/
|   |   |   |-- PinMapEditor.h
|   |   |   |-- PinMapEditor.cpp
|   |   |   |-- PinMapCanvas.{h,cpp}
|   |   |   |-- PinListView.{h,cpp}
|   |   |   |-- ChannelListView.{h,cpp}
|   |   |   `-- SiteTabWidget.{h,cpp}
|   |   |
|   |   |-- limits_editor/
|   |   |   |-- LimitsEditor.h
|   |   |   |-- LimitsEditor.cpp
|   |   |   `-- LimitsTableModel.{h,cpp}
|   |   |
|   |   |-- report_designer/
|   |   |   |-- ReportDesigner.h
|   |   |   |-- ReportDesigner.cpp
|   |   |   |-- ReportCanvas.{h,cpp}
|   |   |   |-- ElementPalette.{h,cpp}
|   |   |   `-- PropertyPanel.{h,cpp}
|   |   |
|   |   |-- operator_interface/
|   |   |   |-- OperatorView.h
|   |   |   |-- OperatorView.cpp
|   |   |   |-- OperatorView.ui
|   |   |   `-- BarcodeInput.{h,cpp}
|   |   |
|   |   |-- dashboards/
|   |   |   |-- ExecutionDashboard.{h,cpp}
|   |   |   |-- ResourceMonitor.{h,cpp}
|   |   |   `-- AnalyticsDashboard.{h,cpp}
|   |   |
|   |   |-- dialogs/
|   |   |   |-- LicenseInfoDialog.{h,cpp}
|   |   |   |-- EnvironmentManager.{h,cpp}
|   |   |   |-- ConfigProfileDialog.{h,cpp}
|   |   |   `-- AboutDialog.{h,cpp}
|   |   |
|   |   |-- widgets/                     # Reusable Qt widgets
|   |   |   |-- LiveChart.{h,cpp}
|   |   |   |-- ProgressWidget.{h,cpp}
|   |   |   |-- SearchableComboBox.{h,cpp}
|   |   |   `-- PropertyGrid.{h,cpp}
|   |   |
|   |   `-- models/                      # Qt item models
|   |       |-- TestPlanModel.{h,cpp}
|   |       |-- ResultsTableModel.{h,cpp}
|   |       `-- ResourceTableModel.{h,cpp}
|   |
|   |-- scripting/                       # Python integration
|   |   |-- CMakeLists.txt
|   |   |-- PythonEngine.h
|   |   |-- PythonEngine.cpp
|   |   |-- VirtualEnvManager.h
|   |   |-- VirtualEnvManager.cpp
|   |   |-- PackageManager.h
|   |   |-- PackageManager.cpp
|   |   |-- PythonTestStep.h
|   |   |-- PythonTestStep.cpp
|   |   |-- bindings/                    # Python bindings for TestMATE API
|   |   |   |-- TestMATEModule.cpp
|   |   |   |-- InstrumentBindings.cpp
|   |   |   |-- DutBindings.cpp
|   |   |   `-- ResultBindings.cpp
|   |   `-- python_api/                  # Python package (testmate)
|   |       |-- __init__.py
|   |       |-- core.py
|   |       |-- instruments.py
|   |       |-- duts.py
|   |       `-- results.py
|   |
|   |-- remote/                          # gRPC services
|   |   |-- CMakeLists.txt
|   |   |-- protos/
|   |   |   |-- test_execution.proto
|   |   |   |-- test_plan.proto
|   |   |   |-- reporting.proto
|   |   |   `-- configuration.proto
|   |   |
|   |   |-- server/
|   |   |   |-- GrpcServer.h
|   |   |   |-- GrpcServer.cpp
|   |   |   |-- TestExecutionServiceImpl.{h,cpp}
|   |   |   `-- ReportServiceImpl.{h,cpp}
|   |   |
|   |   `-- client/
|   |       |-- GrpcClient.h
|   |       |-- GrpcClient.cpp
|   |       `-- RemoteTestRunner.{h,cpp}
|   |
|   |-- reporting/                       # Report generation
|   |   |-- CMakeLists.txt
|   |   |-- ReportEngine.h
|   |   |-- ReportEngine.cpp
|   |   |-- TemplateParser.h
|   |   |-- TemplateParser.cpp
|   |   |-- generators/
|   |   |   |-- IReportGenerator.h
|   |   |   |-- PdfGenerator.{h,cpp}
|   |   |   |-- HtmlGenerator.{h,cpp}
|   |   |   |-- XmlGenerator.{h,cpp}
|   |   |   |-- AtmlGenerator.{h,cpp}
|   |   |   |-- ExcelGenerator.{h,cpp}
|   |   |   `-- TextGenerator.{h,cpp}
|   |   |
|   |   `-- templates/                   # Default report templates
|   |       |-- standard_report.xml
|   |       |-- summary_report.xml
|   |       `-- debug_report.xml
|   |
|   |-- database/                        # Database layer
|   |   |-- CMakeLists.txt
|   |   |-- IDatabaseProvider.h
|   |   |-- DatabaseManager.h
|   |   |-- DatabaseManager.cpp
|   |   |-- providers/
|   |   |   |-- PostgreSqlProvider.{h,cpp}
|   |   |   |-- MySqlProvider.{h,cpp}
|   |   |   |-- SqliteProvider.{h,cpp}
|   |   |   `-- SqlServerProvider.{h,cpp}
|   |   |
|   |   |-- schema/
|   |   |   |-- schema_v1.sql
|   |   |   `-- migrations/
|   |   |       |-- v1_to_v2.sql
|   |   |       `-- v2_to_v3.sql
|   |   |
|   |   `-- repositories/
|   |       |-- TestRunRepository.{h,cpp}
|   |       |-- TestResultRepository.{h,cpp}
|   |       `-- ConfigRepository.{h,cpp}
|   |
|   |-- analytics/                       # Analytics engine
|   |   |-- CMakeLists.txt
|   |   |-- StatisticsEngine.h
|   |   |-- StatisticsEngine.cpp
|   |   |-- ProcessCapability.{h,cpp}
|   |   |-- TrendAnalysis.{h,cpp}
|   |   |-- ControlCharts.{h,cpp}
|   |   `-- CorrelationAnalysis.{h,cpp}
|   |
|   |-- utils/                           # Utilities
|   |   |-- CMakeLists.txt
|   |   |-- LogManager.h
|   |   |-- LogManager.cpp
|   |   |-- FileUtils.h
|   |   |-- FileUtils.cpp
|   |   |-- StringUtils.h
|   |   |-- StringUtils.cpp
|   |   |-- TimeUtils.h
|   |   |-- TimeUtils.cpp
|   |   |-- XmlUtils.h
|   |   |-- XmlUtils.cpp
|   |   |-- JsonUtils.h
|   |   |-- JsonUtils.cpp
|   |   |-- CryptoUtils.h                # Encryption utilities
|   |   |-- CryptoUtils.cpp
|   |   `-- ExpressionParser.h           # Expression evaluation
|   |
|   `-- platform/                        # Platform-specific code
|       |-- CMakeLists.txt
|       |-- Platform.h
|       |-- windows/
|       |   `-- PlatformWindows.cpp
|       |-- linux/
|       |   `-- PlatformLinux.cpp
|       `-- macos/
|           `-- PlatformMacOS.cpp
|
|-- tests/                               # Test code
|   |-- CMakeLists.txt
|   |-- unit/                            # Unit tests
|   |   |-- core/
|   |   |   |-- ProcessModelTests.cpp
|   |   |   |-- ThreadPoolTests.cpp
|   |   |   |-- ResourceSchedulerTests.cpp
|   |   |   `-- ExecutionEngineTests.cpp
|   |   |
|   |   |-- models/
|   |   |   |-- TestPlanTests.cpp
|   |   |   |-- TestResultTests.cpp
|   |   |   `-- MeasurementTests.cpp
|   |   |
|   |   |-- plugins/
|   |   |   |-- TestStepBaseTests.cpp
|   |   |   |-- PropertySystemTests.cpp
|   |   |   `-- PluginLoaderTests.cpp
|   |   |
|   |   `-- utils/
|   |       |-- LogManagerTests.cpp
|   |       |-- StringUtilsTests.cpp
|   |       `-- ExpressionParserTests.cpp
|   |
|   |-- integration/                     # Integration tests
|   |   |-- ExecutionIntegrationTests.cpp
|   |   |-- DatabaseIntegrationTests.cpp
|   |   `-- PluginIntegrationTests.cpp
|   |
|   |-- system/                          # System/E2E tests
|   |   |-- FullTestRunTests.cpp
|   |   `-- ParallelExecutionTests.cpp
|   |
|   `-- fixtures/                        # Test fixtures and data
|       |-- sample_test_plan.xml
|       |-- sample_pin_map.xml
|       `-- sample_limits.csv
|
|-- resources/                           # Application resources
|   |-- icons/
|   |   |-- app_icon.ico
|   |   |-- app_icon.png
|   |   |-- toolbar/
|   |   `-- status/
|   |
|   |-- images/
|   |   |-- splash.png
|   |   `-- logo.png
|   |
|   |-- translations/
|   |   |-- testmate_en.ts
|   |   |-- testmate_de.ts
|   |   `-- testmate_zh.ts
|   |
|   |-- stylesheets/
|   |   |-- default.qss
|   |   |-- dark.qss
|   |   `-- light.qss
|   |
|   |-- schemas/                         # XML/JSON schemas
|   |   |-- test_plan.xsd
|   |   |-- pin_map.xsd
|   |   |-- device_config.xsd
|   |   `-- report_template.xsd
|   |
|   `-- default_config/                  # Default configuration files
|       |-- application.config.xml
|       |-- logging.config.xml
|       `-- database.config.xml
|
|-- tools/                               # Development tools
|   |-- plugin_generator/                # Tool to scaffold new plugins
|   |   |-- CMakeLists.txt
|   |   |-- plugin_generator.cpp
|   |   `-- templates/
|   |
|   |-- config_editor/                   # Standalone config editor
|   |   |-- CMakeLists.txt
|   |   `-- config_editor.cpp
|   |
|   `-- deployment_builder/              # Deployment package builder
|       |-- CMakeLists.txt
|       `-- deployment_builder.cpp
|
|-- examples/                            # Example projects
|   |-- basic_test_plan/
|   |   |-- README.md
|   |   |-- basic_test.testplan
|   |   `-- run_test.py
|   |
|   |-- custom_plugin/
|   |   |-- README.md
|   |   |-- CMakeLists.txt
|   |   |-- MyCustomStep.h
|   |   `-- MyCustomStep.cpp
|   |
|   |-- scripting/
|   |   |-- README.md
|   |   |-- python_test_step.py
|   |   `-- data_analysis.py
|   |
|   `-- semiconductor/
|       |-- README.md
|       |-- multi_site_test.testplan
|       |-- device_pin_map.xml
|       `-- limits.csv
|
|-- scripts/                             # Build/deployment scripts
|   |-- build.sh                         # Linux/macOS build script
|   |-- build.bat                        # Windows build script
|   |-- deploy.py                        # Deployment script
|   |-- run_tests.sh                     # Test runner
|   `-- generate_docs.sh                 # Documentation generator
|
`-- third_party/                         # External dependencies
    |-- README.md                        # Dependency documentation
    |-- googletest/                      # Unit test framework (submodule)
    |-- spdlog/                          # Logging library (submodule)
    |-- pybind11/                        # Python bindings (submodule)
    `-- nlohmann_json/                   # JSON library (submodule)
```

---

## Directory Descriptions

### `/cmake/`
CMake modules for finding dependencies, setting compiler flags, and configuring builds.

### `/docs/`
All project documentation including requirements, architecture, user guides, and API references.

### `/include/testmate/`
Public header files that external plugins and applications can include. This is the stable API.

### `/src/core/`
Core business logic with NO Qt dependencies. Contains process models, threading, scheduling, and managers.

### `/src/api/`
API facade layer providing clean interfaces between UI and core. Implements observer pattern for events.

### `/src/models/`
Pure C++ data models representing test plans, results, instruments, etc.

### `/src/plugins/`
Plugin system including interfaces, base classes, loader, and standard built-in plugins.

### `/src/ui/`
Qt-based user interface. Contains adapters that translate between Qt and core types.

### `/src/scripting/`
Python embedding and virtual environment management for Python test steps.

### `/src/remote/`
gRPC service definitions and implementations for remote test execution.

### `/src/reporting/`
Report generation engine with multiple format generators (PDF, HTML, XML, etc.).

### `/src/database/`
Database abstraction layer supporting multiple backends (PostgreSQL, MySQL, SQLite, etc.).

### `/tests/`
All test code organized by test type: unit, integration, and system tests.

### `/resources/`
Application resources including icons, translations, stylesheets, and schemas.

### `/tools/`
Development tools like plugin generator and configuration editor.

### `/examples/`
Example projects demonstrating TestMATE features.

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-01-XX | TestMATE Team | Initial folder structure |
