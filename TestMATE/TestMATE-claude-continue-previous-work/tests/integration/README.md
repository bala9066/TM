# TestMATE Integration Tests

## Overview

This directory contains end-to-end integration tests that validate complete workflows across all TestMATE components. Unlike unit tests that test individual components in isolation, these integration tests verify that all components work together correctly in real-world scenarios.

## Test Coverage

The integration test suite (`EndToEndTests.cpp`) includes the following test cases:

### 1. CompleteSequenceWorkflow
**Purpose:** Validates the complete test sequence execution workflow

**What it tests:**
- Creating a test sequence with multiple steps
- Adding steps: Wait, Calculation, Limit Check
- Executing the complete sequence
- Verifying all steps execute in order
- Checking Pass/Fail verdicts

**Expected behavior:**
- All 3 steps execute successfully
- Wait step completes in ~100ms
- Calculation step produces correct result (addition)
- Limit check validates against specified limits
- All steps report Pass verdict

### 2. SequenceFileIOWorkflow
**Purpose:** Validates sequence persistence and loading

**What it tests:**
- Creating a test sequence
- Saving sequence to JSON file
- Loading sequence from JSON file
- Verifying data integrity after load

**Expected behavior:**
- JSON file created successfully
- Sequence loads without errors
- All steps preserved (ID, name, parameters)
- No data corruption during save/load cycle

**File location:** `/tmp/test_sequence_io.json`

### 3. DatabaseIntegration
**Purpose:** Validates database storage and retrieval

**What it tests:**
- Opening SQLite database connection
- Saving test sequence data
- Retrieving data by test ID
- Querying data by lot ID
- Verifying data integrity

**Expected behavior:**
- Database operations succeed
- Data persists correctly
- Queries return expected results
- No data loss during storage

**Database location:** `/tmp/integration_test.db`

### 4. ReportGenerationWorkflow
**Purpose:** Validates HTML report generation

**What it tests:**
- Creating test result data
- Generating HTML report
- Verifying file creation
- Validating report content

**Expected behavior:**
- Report file created successfully
- File size > 100 bytes (non-empty)
- Contains expected HTML elements
- Properly formatted report

**Report location:** `/tmp/test_report.html`

### 5. MultiStepIntegrationWorkflow
**Purpose:** Validates complex multi-step sequences

**What it tests:**
- Creating sequence with 7 diverse step types:
  1. Wait step (100ms delay)
  2. Calculation step (addition: 10 + 5)
  3. Limit check (validate result in range)
  4. File write operation
  5. File exists check
  6. File read operation
  7. File delete operation

**Expected behavior:**
- All 7 steps execute successfully
- Each step reports Pass verdict
- File operations work correctly
- Data flows between steps

**Test file location:** `/tmp/multi_step_test.txt`

### 6. ErrorHandlingWorkflow
**Purpose:** Validates proper error handling

**What it tests:**
- Loading non-existent sequence file
- Executing limit check with failing value
- Division by zero in calculation

**Expected behavior:**
- Invalid file load returns error (not exception)
- Limit check correctly identifies failures
- Division by zero handled gracefully
- Error messages are descriptive

### 7. PerformanceBaseline
**Purpose:** Establishes performance benchmarks

**What it measures:**
- Step creation overhead (1000 iterations)
- Calculation step performance (1000 iterations)
- Limit check performance (1000 iterations)

**Performance thresholds:**
- Step creation: < 100μs per step
- Calculation execution: < 100μs per operation
- Limit check execution: < 100μs per validation

**Note:** These are baseline measurements, not strict requirements. Use these to detect performance regressions.

## Building Integration Tests

### Prerequisites
- CMake 3.15 or later
- C++20 compatible compiler
- GoogleTest library (auto-fetched if not installed)
- All TestMATE libraries built

### Build Commands

```bash
# From project root
mkdir -p build
cd build

# Configure with tests enabled
cmake ..

# Build integration tests
cmake --build . --target testmate_integration_tests

# Or build everything
cmake --build .
```

### Build Output
The integration test executable will be located at:
```
build/tests/integration/testmate_integration_tests
```

## Running Integration Tests

### Run All Integration Tests

```bash
# From build directory
./tests/integration/testmate_integration_tests

# Or using CTest
ctest -R Integration -V
```

### Run Specific Test Case

```bash
# Run only one test
./tests/integration/testmate_integration_tests --gtest_filter=EndToEndTests.CompleteSequenceWorkflow

# Run multiple specific tests
./tests/integration/testmate_integration_tests --gtest_filter=EndToEndTests.Database*
```

### Verbose Output

```bash
# Show detailed test output
./tests/integration/testmate_integration_tests --gtest_verbose

# Show very detailed output
GTEST_VERBOSE=1 ./tests/integration/testmate_integration_tests
```

### Performance Tests Only

```bash
./tests/integration/testmate_integration_tests --gtest_filter=*Performance*
```

## Expected Output

### Successful Run
```
[==========] Running 7 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 7 tests from EndToEndTests
[ RUN      ] EndToEndTests.CompleteSequenceWorkflow
[       OK ] EndToEndTests.CompleteSequenceWorkflow (105 ms)
[ RUN      ] EndToEndTests.SequenceFileIOWorkflow
[       OK ] EndToEndTests.SequenceFileIOWorkflow (12 ms)
[ RUN      ] EndToEndTests.DatabaseIntegration
[       OK ] EndToEndTests.DatabaseIntegration (45 ms)
[ RUN      ] EndToEndTests.ReportGenerationWorkflow
[       OK ] EndToEndTests.ReportGenerationWorkflow (23 ms)
[ RUN      ] EndToEndTests.MultiStepIntegrationWorkflow
[       OK ] EndToEndTests.MultiStepIntegrationWorkflow (158 ms)
[ RUN      ] EndToEndTests.ErrorHandlingWorkflow
[       OK ] EndToEndTests.ErrorHandlingWorkflow (8 ms)
[ RUN      ] EndToEndTests.PerformanceBaseline
Step creation: 45μs avg
Calculation: 38μs avg
Limit check: 22μs avg
[       OK ] EndToEndTests.PerformanceBaseline (234 ms)
[----------] 7 tests from EndToEndTests (585 ms total)

[----------] Global test environment tear-down
[==========] 7 tests from 1 test suite ran. (585 ms total)
[  PASSED  ] 7 tests.
```

## Troubleshooting

### Test Fails: "Failed to create temporary file"

**Problem:** Integration tests need write access to `/tmp` directory

**Solutions:**
1. Verify `/tmp` directory exists and is writable
2. On Windows, tests use `%TEMP%` directory
3. Check disk space availability
4. Run tests with appropriate permissions

### Test Fails: "Database connection failed"

**Problem:** SQLite database cannot be created

**Solutions:**
1. Verify SQLite library is linked (`testmate_database`)
2. Check `/tmp` directory permissions
3. Ensure no conflicting database file exists
4. Check for SQLite version compatibility

### Test Fails: Performance thresholds exceeded

**Problem:** System is slower than expected baseline

**Note:** This is not a critical failure. Performance baselines are guidelines.

**Possible causes:**
1. Debug build (use Release build for accurate benchmarks)
2. System under heavy load
3. Virtual machine or container overhead
4. Slower CPU architecture

**Solution:**
```bash
# Build in Release mode for performance tests
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./tests/integration/testmate_integration_tests --gtest_filter=*Performance*
```

### Test Fails: "Sequence file validation failed"

**Problem:** JSON sequence save/load detected corruption

**Solutions:**
1. Check file system integrity
2. Verify JSON parsing library (nlohmann/json)
3. Check for special characters in test data
4. Review file permissions

### Integration Tests Timeout

**Problem:** Tests take longer than CTest timeout (300s)

**Solutions:**
1. Check system performance
2. Reduce test iterations in PerformanceBaseline
3. Increase CTest timeout in CMakeLists.txt:
```cmake
set_tests_properties(Integration PROPERTIES TIMEOUT 600)
```

### All Tests Fail: "Library not found"

**Problem:** TestMATE libraries not built or not in search path

**Solutions:**
1. Build all libraries first:
```bash
cmake --build . --target testmate_core
cmake --build . --target testmate_utils
cmake --build . --target testmate_api
cmake --build . --target testmate_database
```

2. Check library linking in CMakeLists.txt
3. Verify LD_LIBRARY_PATH (Linux) or PATH (Windows)

## Test Data Files

Integration tests create temporary files during execution:

| File | Purpose | Cleanup |
|------|---------|---------|
| `/tmp/test_sequence_io.json` | Sequence save/load test | Auto-deleted |
| `/tmp/integration_test.db` | Database integration test | Auto-deleted |
| `/tmp/test_report.html` | Report generation test | Auto-deleted |
| `/tmp/multi_step_test.txt` | Multi-step workflow test | Auto-deleted |

**Note:** Files are automatically cleaned up by test teardown. If tests crash, you may need to manually delete these files.

## Adding New Integration Tests

### Test Template

```cpp
TEST_F(EndToEndTests, YourNewIntegrationTest) {
    // Arrange: Set up test environment
    CTestSequence sequence;
    // ... setup code ...

    // Act: Perform operations
    auto result = sequence.Execute();

    // Assert: Verify results
    ASSERT_TRUE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kSuccess);
}
```

### Best Practices

1. **Test Complete Workflows:** Integration tests should test multiple components together
2. **Use Realistic Data:** Test with real-world scenarios
3. **Clean Up Resources:** Use test fixture teardown to clean temporary files
4. **Meaningful Names:** Test names should describe what workflow is being tested
5. **Clear Assertions:** Use descriptive assertion messages
6. **Performance Awareness:** Keep tests fast (<1s each when possible)
7. **Isolation:** Each test should be independent and repeatable

### Adding to Build System

After creating a new integration test file:

1. Add to `tests/integration/CMakeLists.txt`:
```cmake
set(INTEGRATION_TEST_SOURCES
    EndToEndTests.cpp
    YourNewTest.cpp  # Add here
)
```

2. Rebuild:
```bash
cmake --build . --target testmate_integration_tests
```

## Integration with CI/CD

### GitHub Actions Example

```yaml
- name: Run Integration Tests
  run: |
    cd build
    ctest -R Integration --output-on-failure
```

### Jenkins Pipeline Example

```groovy
stage('Integration Tests') {
    steps {
        sh '''
            cd build
            ctest -R Integration -V
        '''
    }
}
```

## Performance Benchmarking

To establish performance baselines for your specific hardware:

```bash
# Build in Release mode
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target testmate_integration_tests

# Run performance tests 10 times
for i in {1..10}; do
    ./tests/integration/testmate_integration_tests \
        --gtest_filter=*Performance* \
        --gtest_repeat=1
done
```

Analyze the results to establish your hardware-specific baseline thresholds.

## Further Information

- **Unit Tests:** See `tests/unit/` for component-level tests
- **Test Steps:** See `examples/test_steps/` for example implementations
- **Documentation:** See `QUICKSTART.md` for usage examples
- **API Reference:** See `docs/API.md` for detailed API documentation

## Support

For issues or questions about integration tests:
1. Check troubleshooting section above
2. Review test output for specific error messages
3. Consult unit tests for component-specific behavior
4. Review source code comments in `EndToEndTests.cpp`
