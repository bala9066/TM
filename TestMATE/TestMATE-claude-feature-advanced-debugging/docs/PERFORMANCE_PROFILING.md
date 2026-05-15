# TestMATE Performance Profiling Guide

## Overview

TestMATE includes a comprehensive performance profiling system that allows you to measure, analyze, and optimize the performance of your test sequences and components. This guide explains how to use the performance profiling utilities effectively.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Basic Usage](#basic-usage)
3. [Advanced Features](#advanced-features)
4. [Report Generation](#report-generation)
5. [Regression Detection](#regression-detection)
6. [Best Practices](#best-practices)
7. [API Reference](#api-reference)

## Quick Start

### Simple Profiling Example

```cpp
#include "utils/PerformanceProfiler.h"

int main() {
    auto& profiler = CPerformanceProfiler::GetInstance();

    // Profile an operation
    profiler.StartProfiling("MyOperation");

    // ... your code here ...
    PerformSomeWork();

    profiler.StopProfiling("MyOperation");

    // Print summary
    profiler.PrintSummary();

    return 0;
}
```

### Using RAII (Recommended)

```cpp
#include "utils/PerformanceProfiler.h"

void ProcessData() {
    CProfileScope profile("ProcessData");

    // ... code is automatically profiled ...
    // Profiling stops when scope exits
}
```

### Using Convenience Macros

```cpp
#include "utils/PerformanceProfiler.h"

void MyFunction() {
    PROFILE_FUNCTION();  // Profiles entire function

    // ... function code ...
}

void AnotherFunction() {
    {
        PROFILE_SCOPE("CriticalSection");
        // ... critical code ...
    }

    // ... other code (not profiled) ...
}
```

## Basic Usage

### Method 1: Manual Start/Stop

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

profiler.StartProfiling("DatabaseQuery");
auto results = database.Query("SELECT * FROM tests");
profiler.StopProfiling("DatabaseQuery");
```

### Method 2: RAII with CProfileScope

```cpp
void ExecuteTestSequence() {
    CProfileScope profile("ExecuteTestSequence", "Core");

    // All code in this scope is profiled
    for (auto& step : sequence.GetSteps()) {
        step.Execute(result);
    }
}
```

### Method 3: Profile Lambda/Function

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

// Profile a function with return value
auto result = profiler.Profile("Calculation", []() {
    return PerformComplexCalculation();
});

// Profile a void function
profiler.ProfileVoid("Initialization", []() {
    InitializeSystem();
});
```

## Advanced Features

### Categorizing Measurements

Organize your measurements into logical categories:

```cpp
void TestApplication() {
    CProfileScope dbProfile("LoadData", "Database");
    CProfileScope calcProfile("ProcessData", "Computation");
    CProfileScope ioProfile("SaveResults", "FileIO");

    // Get statistics by category later
    auto statsByCategory = profiler.GetStatsByCategory();
}
```

### Recording Custom Metrics

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

// Manually record a metric (in nanoseconds)
TUInt64 customDuration = MeasureSomething();
profiler.RecordMetric("CustomMetric", customDuration, "Category");
```

### Performance Checkpoints

Add checkpoints to see intermediate results:

```cpp
void LongRunningTest() {
    auto& profiler = CPerformanceProfiler::GetInstance();

    Phase1();
    profiler.Checkpoint("After Phase 1");  // Prints summary

    Phase2();
    profiler.Checkpoint("After Phase 2");  // Prints summary

    Phase3();
    profiler.Checkpoint("After Phase 3");  // Prints summary
}
```

### Memory Tracking

Enable memory usage tracking:

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

// Enable memory tracking
profiler.SetMemoryTrackingEnabled(true);

{
    CProfileScope profile("MemoryIntensiveOperation");
    // ... allocate memory ...
}

// Memory usage is included in reports
```

## Report Generation

### HTML Report

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

// Configure report
SPerformanceReportConfig config;
config.title = "Test Sequence Performance Report";
config.includeStatistics = true;
config.includeMemoryUsage = true;
config.outputFormat = "html";

// Generate report
profiler.GenerateReport("performance_report.html", config);
```

**HTML Report Example:**
- Color-coded performance metrics
- Statistical tables (min, max, avg, stddev, percentiles)
- Memory usage graphs
- Sorted by category

### JSON Report

```cpp
SPerformanceReportConfig config;
config.outputFormat = "json";

profiler.GenerateReport("performance_data.json", config);
```

**JSON Structure:**
```json
{
  "title": "Performance Report",
  "timestamp": "Jan 22 2025 10:30:00",
  "statistics": [
    {
      "name": "ExecuteTestStep",
      "count": 1000,
      "min_ns": 15000,
      "avg_ns": 23500,
      "max_ns": 45000,
      "stddev_ns": 3200,
      "median_ns": 22000,
      "p95_ns": 28000,
      "p99_ns": 32000
    }
  ]
}
```

### CSV Report

```cpp
profiler.GenerateCsvReport("performance_data.csv");
```

**CSV Format:**
```csv
Metric,Count,Min(μs),Max(μs),Avg(μs),StdDev(μs),Median(μs),P95(μs),P99(μs)
ExecuteTestStep,1000,15.0,45.0,23.5,3.2,22.0,28.0,32.0
LoadSequence,500,120.0,350.0,180.5,25.3,175.0,230.0,280.0
```

### Console Summary

```cpp
profiler.PrintSummary();
```

**Output:**
```
========== Performance Summary ==========
Total Measurements: 1500
Unique Metrics: 3

            ExecuteTestStep:    23.50 μs avg (1000 samples)
              LoadSequence:   180.50 μs avg (500 samples)
              GenerateReport:  5234.20 μs avg (10 samples)
=========================================
```

## Regression Detection

### Setting Performance Baselines

```cpp
auto& profiler = CPerformanceProfiler::GetInstance();

// Set baseline (in nanoseconds)
profiler.SetBaseline("CriticalOperation", 50000);  // 50 μs baseline

// Run tests...
for (int i = 0; i < 100; i++) {
    CProfileScope profile("CriticalOperation");
    CriticalOperation();
}

// Check for regressions (10% threshold)
if (profiler.ExceedsBaseline("CriticalOperation", 10.0)) {
    std::cerr << "WARNING: Performance regression detected!\n";
}
```

### Getting Regression Report

```cpp
// Get all regressions with 15% threshold
auto regressions = profiler.GetRegressions(15.0);

if (!regressions.empty()) {
    std::cout << "Performance Regressions Detected:\n";
    for (const auto& regression : regressions) {
        std::cout << "  - " << regression << "\n";
    }
}
```

**Output:**
```
Performance Regressions Detected:
  - CriticalOperation: 18.5% over baseline (59.25 μs vs 50.00 μs)
  - DatabaseQuery: 22.3% over baseline (122.30 μs vs 100.00 μs)
```

## Best Practices

### 1. Profile in Release Mode

```bash
# Build in Release mode for accurate measurements
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### 2. Use Appropriate Granularity

```cpp
// ❌ Too granular (overhead dominates)
for (int i = 0; i < 1000000; i++) {
    PROFILE_SCOPE("SingleIteration");
    DoTinyOperation();
}

// ✅ Better: Profile the loop
{
    PROFILE_SCOPE("MillionIterations");
    for (int i = 0; i < 1000000; i++) {
        DoTinyOperation();
    }
}
```

### 3. Profile Representative Workloads

```cpp
// Profile with realistic data sizes
{
    PROFILE_SCOPE("ProcessLargeDataset");
    ProcessDataset(largeRealWorldData);  // Use real data
}
```

### 4. Use Categories for Organization

```cpp
// Group related operations
CProfileScope("Connect", "Network");
CProfileScope("Query", "Database");
CProfileScope("Render", "UI");
```

### 5. Collect Sufficient Samples

```cpp
// Get statistically significant results
for (int i = 0; i < 1000; i++) {  // Many iterations
    CProfileScope profile("Operation");
    PerformOperation();
}

auto stats = profiler.CalculateStats("Operation");
// Now you have reliable min/max/avg/stddev/percentiles
```

### 6. Clear Between Tests

```cpp
// Clear old data before new test run
profiler.Clear();

// Run your test suite
RunTests();

// Generate fresh report
profiler.GenerateReport("current_run.html");
```

## Practical Examples

### Example 1: Profile Test Sequence Execution

```cpp
#include "utils/PerformanceProfiler.h"
#include "core/test_sequence/TestSequence.h"

void ProfileTestSequence() {
    auto& profiler = CPerformanceProfiler::GetInstance();

    CTestSequence sequence;
    // ... add steps to sequence ...

    // Profile overall execution
    {
        PROFILE_SCOPE("SequenceExecution");
        sequence.Execute();
    }

    // Profile individual steps
    for (auto& step : sequence.GetSteps()) {
        TString stepName = "Step_" + step->GetName();
        PROFILE_SCOPE(stepName);

        SStepResult result;
        step->Execute(result);
    }

    // Generate report
    profiler.GenerateHtmlReport("sequence_performance.html");
}
```

### Example 2: Compare Different Algorithms

```cpp
void CompareAlgorithms() {
    auto& profiler = CPerformanceProfiler::GetInstance();

    std::vector<int> testData = GenerateTestData(10000);

    // Test Algorithm A
    for (int i = 0; i < 100; i++) {
        PROFILE_SCOPE("AlgorithmA");
        SortAlgorithmA(testData);
    }

    // Test Algorithm B
    for (int i = 0; i < 100; i++) {
        PROFILE_SCOPE("AlgorithmB");
        SortAlgorithmB(testData);
    }

    // Compare results
    auto statsA = profiler.CalculateStats("AlgorithmA");
    auto statsB = profiler.CalculateStats("AlgorithmB");

    std::cout << "Algorithm A: " << statsA.GetAvgMs() << " ms\n";
    std::cout << "Algorithm B: " << statsB.GetAvgMs() << " ms\n";

    if (statsA.avgNs < statsB.avgNs) {
        std::cout << "Algorithm A is faster!\n";
    } else {
        std::cout << "Algorithm B is faster!\n";
    }
}
```

### Example 3: Continuous Integration Performance Tests

```cpp
#include <gtest/gtest.h>
#include "utils/PerformanceProfiler.h"

TEST(PerformanceTests, CriticalPathPerformance) {
    auto& profiler = CPerformanceProfiler::GetInstance();

    // Set performance requirements
    profiler.SetBaseline("CriticalOperation", 100000);  // 100 μs max

    // Run test
    for (int i = 0; i < 1000; i++) {
        PROFILE_SCOPE("CriticalOperation");
        CriticalOperation();
    }

    // Check performance requirement (10% tolerance)
    EXPECT_FALSE(profiler.ExceedsBaseline("CriticalOperation", 10.0))
        << "Performance regression detected!";

    auto stats = profiler.CalculateStats("CriticalOperation");
    EXPECT_LT(stats.percentile99Ns, 120000)  // P99 < 120 μs
        << "P99 latency too high: " << (stats.percentile99Ns / 1000.0) << " μs";
}
```

### Example 4: Profiling Test Step Implementations

```cpp
void ProfileTestSteps() {
    auto& profiler = CPerformanceProfiler::GetInstance();
    profiler.SetMemoryTrackingEnabled(true);

    // Profile different step types
    const int iterations = 1000;

    // Wait Step
    for (int i = 0; i < iterations; i++) {
        CWaitStep waitStep("WAIT-001", "Wait", 1);
        PROFILE_SCOPE_CAT("WaitStep_Creation", "StepCreation");
        SStepResult result;
        waitStep.Execute(result);
    }

    // Limit Check Step
    for (int i = 0; i < iterations; i++) {
        CLimitCheckStep limitCheck("LIMIT-001", "Check");
        PROFILE_SCOPE_CAT("LimitCheckStep_Execution", "StepExecution");
        limitCheck.SetValue(5.0);
        limitCheck.SetLimits(0.0, 10.0);
        SStepResult result;
        limitCheck.Execute(result);
    }

    // Generate comparison report
    SPerformanceReportConfig config;
    config.title = "Test Step Performance Analysis";
    profiler.GenerateHtmlReport("test_steps_performance.html", config);
}
```

## API Reference

### Main Profiler Class

```cpp
CPerformanceProfiler& profiler = CPerformanceProfiler::GetInstance();
```

#### Profiling Methods

| Method | Description |
|--------|-------------|
| `StartProfiling(name, category)` | Start profiling named operation |
| `StopProfiling(name)` | Stop profiling named operation |
| `Profile(name, func)` | Profile function with return value |
| `ProfileVoid(name, func)` | Profile void function |

#### Data Collection

| Method | Description |
|--------|-------------|
| `RecordMetric(name, valueNs, category)` | Record custom metric |
| `Checkpoint(name)` | Print intermediate summary |
| `AddMeasurement(measurement)` | Add custom measurement |

#### Statistics

| Method | Description |
|--------|-------------|
| `CalculateStats(name)` | Get statistics for metric |
| `GetStatsByCategory()` | Get all stats grouped by category |
| `GetMeasurements()` | Get all measurements |
| `GetMeasurements(name)` | Get measurements for specific metric |

#### Reporting

| Method | Description |
|--------|-------------|
| `GenerateReport(path, config)` | Generate report (HTML/JSON/CSV) |
| `GenerateHtmlReport(path, config)` | Generate HTML report |
| `GenerateJsonReport(path, config)` | Generate JSON report |
| `GenerateCsvReport(path)` | Generate CSV report |
| `PrintSummary()` | Print summary to console |

#### Configuration

| Method | Description |
|--------|-------------|
| `SetEnabled(enabled)` | Enable/disable profiling |
| `SetMemoryTrackingEnabled(enabled)` | Enable/disable memory tracking |
| `Clear()` | Clear all measurements |
| `Reset()` | Reset profiler (clear + baselines) |

#### Regression Detection

| Method | Description |
|--------|-------------|
| `SetBaseline(name, baselineNs)` | Set performance baseline |
| `ExceedsBaseline(name, threshold%)` | Check if exceeds baseline |
| `GetRegressions(threshold%)` | Get all regression reports |

### Helper Classes

#### CProfileScope (RAII)

```cpp
{
    CProfileScope profile("OperationName");
    // ... automatically profiled code ...
} // Profiling stops here
```

#### SPerformanceStats

```cpp
struct SPerformanceStats {
    TUInt64 count;         // Number of samples
    TUInt64 minNs;         // Minimum time
    TUInt64 maxNs;         // Maximum time
    TDouble avgNs;         // Average time
    TDouble stdDevNs;      // Standard deviation
    TDouble medianNs;      // Median time
    TDouble percentile95Ns; // 95th percentile
    TDouble percentile99Ns; // 99th percentile

    TDouble GetAvgUs();    // Average in microseconds
    TDouble GetAvgMs();    // Average in milliseconds
};
```

## Troubleshooting

### Issue: Profiling overhead too high

**Solution:** Use coarser granularity, profile larger code blocks

### Issue: Inconsistent measurements

**Solution:** Run more iterations, use Release build, close background apps

### Issue: Memory tracking not working

**Solution:** Ensure `SetMemoryTrackingEnabled(true)` is called before profiling

### Issue: Reports empty

**Solution:** Verify profiling is enabled: `profiler.SetEnabled(true)`

## Further Reading

- Integration Tests: `tests/integration/EndToEndTests.cpp` (PerformanceBaseline test)
- Example Usage: See test step implementations
- Source Code: `include/utils/PerformanceProfiler.h`, `src/utils/PerformanceProfiler.cpp`

## Summary

The TestMATE Performance Profiler provides:
- ✅ High-resolution timing (nanosecond precision)
- ✅ Statistical analysis (min/max/avg/stddev/percentiles)
- ✅ Multiple report formats (HTML/JSON/CSV)
- ✅ Memory usage tracking
- ✅ Regression detection with baselines
- ✅ Thread-safe operation
- ✅ Easy-to-use RAII and macro interfaces
- ✅ Category-based organization

Use it to ensure your test sequences run efficiently and detect performance regressions early!
