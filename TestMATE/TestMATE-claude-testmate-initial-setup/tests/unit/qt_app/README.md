# Qt GUI Unit Tests

This directory contains QTest-based unit tests for the Qt GUI components of TestMATE.

## Overview

The Qt tests are built using Qt's QTest framework, which provides:
- Test fixture setup/cleanup
- Signal/slot testing with QSignalSpy
- Widget component verification
- UI interaction testing

## Test Files

### 1. MainWindowTests.cpp
Comprehensive tests for the main application window:
- **Constructor & Initialization**: Window creation and setup
- **UI Components**: Menu bar, toolbars, status bar, dock widgets
- **Menu Actions**: File, Edit, Sequence, Execution, View, Tools, Help
- **Toolbar Actions**: File, Edit, and Execution toolbars
- **Dock Widgets**: Execution Monitor and Report Viewer
- **Window State**: Title, size, settings persistence
- **Signal/Slot Connections**: Component communication

**Test Count**: 25+ test methods

### 2. SequenceEditorWidgetTests.cpp
Tests for the test sequence editor widget:
- **Constructor & UI**: Widget initialization
- **Components**: Tree view, toolbar, properties panel
- **Sequence Info**: Name, version, description fields
- **Functionality**: New sequence, validation
- **Signals**: sequenceModified, stepSelected, validationChanged
- **Configuration**: Tree view settings, toolbar actions

**Test Count**: 15+ test methods

### 3. ExecutionMonitorWidgetTests.cpp
Tests for the execution monitoring widget:
- **UI Components**: Status label, progress bar, buttons, results table
- **Execution Control**: Start, pause, stop, abort
- **Debug Mode**: Debug execution functionality
- **Signals**: executionStarted, executionCompleted, stepCompleted
- **State Management**: Idle, running, paused, stopped states
- **Button States**: Enabled/disabled based on execution state

**Test Count**: 20+ test methods

### 4. DialogTests.cpp
Tests for all Qt dialogs (combined test file):

#### AboutDialog Tests
- Dialog creation and structure
- Version and title labels
- Button box functionality

#### ConfigurationDialog Tests
- Tab widget with General, Execution, Database tabs
- Configuration controls for each tab
- Settings persistence

#### StepPropertiesDialog Tests
- General tab: Name, type, description, enabled checkbox
- Parameters tab: Parameter table
- Step type combo box with predefined types

#### ReportViewerWidget Tests
- Text browser component
- Toolbar functionality
- Report loading

**Test Count**: 30+ test methods across all dialogs

### 5. ModelTests.cpp
Tests for Qt model classes (combined test file):

#### SequenceTreeModel Tests (30+ tests)
- **Constructor**: With null/valid sequence
- **Row Count**: Various sequence states (empty, with steps)
- **Column Count**: Always 3 (Step Name, Type, Status)
- **Data Retrieval**: DisplayRole for all columns
- **Header Data**: Horizontal and vertical headers
- **Index/Parent**: Index creation, validation, parent always invalid
- **Sequence Management**: setSequence(), refresh() with signal emission
- **Model Interface**: QAbstractItemModel compatibility

#### ResultsTableModel Tests (10+ tests)
- **Constructor**: With/without parent
- **Row Count**: Currently returns 0 (placeholder implementation)
- **Column Count**: Always 3
- **Data Retrieval**: Currently returns QVariant (placeholder)
- **Model Interface**: QAbstractTableModel compatibility
- **Type Casting**: Verify casting to abstract base classes

**Test Count**: 40+ test methods across both models

## Building the Tests

### Prerequisites
- Qt5 (5.15+) or Qt6
- Qt Test module
- CMake 3.15+

### Build Configuration

The Qt tests are **optional** and only built when Qt is detected:

```bash
cd build
cmake ..
# Output should show:
# -- Qt found - building Qt GUI tests
cmake --build .
```

If Qt is not available:
```
-- Qt not found - skipping Qt GUI tests (optional)
```

### Running the Tests

After building, run individual test executables:

```bash
# Run all Qt tests via CTest
cd build
ctest -R Qt

# Run individual test suites
./qt_mainwindow_tests
./qt_sequenceeditor_tests
./qt_executionmonitor_tests
./qt_dialog_tests
./qt_model_tests

# Run with verbose output
./qt_mainwindow_tests -v2
./qt_model_tests -v2

# Run specific test function
./qt_mainwindow_tests testMenuBarExists
./qt_model_tests testRowCountWithEmptySequence
```

## Test Architecture

### QTest Framework Features Used

1. **Test Fixtures**: `init()` and `cleanup()` for each test
2. **Test Cases**: `initTestCase()` and `cleanupTestCase()` for suite setup
3. **Assertions**: QVERIFY, QCOMPARE, QVERIFY2
4. **Signal Testing**: QSignalSpy for signal emission verification
5. **Mock Objects**: Qt's meta-object system for introspection

### Test Pattern

Each test file follows this pattern:

```cpp
class WidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();    // Run once before all tests
    void cleanupTestCase(); // Run once after all tests
    void init();            // Run before each test
    void cleanup();         // Run after each test

    void testSomething();   // Individual test methods

private:
    Widget *m_pWidget;      // Test subject
};

QTEST_MAIN(WidgetTests)
#include "WidgetTests.moc"
```

## Code Coverage

To generate code coverage for Qt tests (requires lcov):

```bash
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make
make coverage_qt
```

## Continuous Integration

The Qt tests are integrated with CTest and can be run in CI pipelines:

```yaml
# Example GitHub Actions
- name: Run Qt Tests
  run: |
    cd build
    ctest -R Qt --output-on-failure
```

## Adding New Tests

To add a new Qt test file:

1. Create test file in `tests/unit/qt_app/`
2. Follow the QTest pattern shown above
3. Add to `tests/CMakeLists.txt`:

```cmake
add_executable(qt_newwidget_tests unit/qt_app/NewWidgetTests.cpp)
target_link_libraries(qt_newwidget_tests
    PRIVATE
        ${QT_LIBS}
        testmate_core
        testmate_utils
        testmate_api
)
target_include_directories(qt_newwidget_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME QtNewWidgetTests COMMAND qt_newwidget_tests)
```

## Best Practices

1. **Isolation**: Each test should be independent
2. **Cleanup**: Always delete widgets in `cleanup()`
3. **Signals**: Use QSignalSpy for async signal testing
4. **Naming**: Use descriptive test method names (testWhatIsBeingTested)
5. **Assertions**: One logical assertion per test when possible
6. **Documentation**: Document complex test scenarios

## Troubleshooting

### Qt Not Found
```
-- Qt not found - skipping Qt GUI tests (optional)
```
**Solution**: Install Qt5 or Qt6 development packages

### MOC Errors
```
error: undefined reference to `vtable for ClassName'
```
**Solution**: Ensure `#include "TestFile.moc"` at end of test file

### Widget Not Found
```
QVERIFY(widget != nullptr) failed
```
**Solution**: Check .ui file object names match test expectations

## References

- [Qt Test Documentation](https://doc.qt.io/qt-6/qtest-overview.html)
- [QTest Tutorial](https://doc.qt.io/qt-6/qtest-tutorial.html)
- [QSignalSpy Documentation](https://doc.qt.io/qt-6/qsignalspy.html)
