/**************************************************************************
 * File Name: PerformanceProfiler.h
 * Description: Performance profiling and benchmarking utilities
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Provides tools for measuring, analyzing, and reporting performance
 *   metrics for TestMATE components, test steps, and sequences.
 *
 * Features:
 *   - High-resolution timing measurements
 *   - Statistical analysis (min, max, avg, stddev)
 *   - Memory usage tracking
 *   - Performance regression detection
 *   - HTML/JSON report generation
 *   - Thread-safe operation
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Performance Measurement Point
 **************************************************************************/
struct SPerformanceMeasurement {
    TString name;                                      // Measurement name
    std::chrono::steady_clock::time_point startTime;   // Start timestamp
    std::chrono::steady_clock::time_point endTime;     // End timestamp
    TUInt64 durationNs{0};                            // Duration in nanoseconds
    TUInt64 memoryUsedBytes{0};                       // Memory used
    TString category;                                  // Measurement category
    std::map<TString, TString> metadata;              // Additional metadata
};

/**************************************************************************
 * Performance Statistics
 **************************************************************************/
struct SPerformanceStats {
    TString name;                       // Metric name
    TUInt64 count{0};                  // Number of measurements
    TUInt64 totalNs{0};                // Total time (ns)
    TUInt64 minNs{UINT64_MAX};         // Minimum time (ns)
    TUInt64 maxNs{0};                  // Maximum time (ns)
    TDouble avgNs{0.0};                // Average time (ns)
    TDouble stdDevNs{0.0};             // Standard deviation (ns)
    TDouble medianNs{0.0};             // Median time (ns)
    TDouble percentile95Ns{0.0};       // 95th percentile (ns)
    TDouble percentile99Ns{0.0};       // 99th percentile (ns)

    // Convenience getters in different units
    [[nodiscard]] TDouble GetAvgUs() const { return avgNs / 1000.0; }
    [[nodiscard]] TDouble GetAvgMs() const { return avgNs / 1000000.0; }
    [[nodiscard]] TDouble GetAvgS() const { return avgNs / 1000000000.0; }
};

/**************************************************************************
 * Performance Report Configuration
 **************************************************************************/
struct SPerformanceReportConfig {
    bool includeIndividualMeasurements{false};  // Include all measurements
    bool includeStatistics{true};                // Include statistical analysis
    bool includeMemoryUsage{true};               // Include memory metrics
    bool includeTimeline{false};                 // Include timeline data
    TString outputFormat{"html"};                // Output format (html, json, csv)
    TString title{"TestMATE Performance Report"}; // Report title
};

/**************************************************************************
 * Class: CPerformanceProfiler
 * Description: Main performance profiling interface
 *
 * Thread Safety: All methods are thread-safe
 *
 * Usage Example:
 *
 *   // Start profiling
 *   auto& profiler = CPerformanceProfiler::GetInstance();
 *   profiler.StartProfiling("MyOperation");
 *
 *   // ... perform operation ...
 *
 *   // End profiling
 *   profiler.StopProfiling("MyOperation");
 *
 *   // Generate report
 *   profiler.GenerateReport("performance_report.html");
 *
 **************************************************************************/
class CPerformanceProfiler {
public:
    // Singleton access
    static CPerformanceProfiler& GetInstance();

    // Delete copy/move constructors
    CPerformanceProfiler(const CPerformanceProfiler&) = delete;
    CPerformanceProfiler& operator=(const CPerformanceProfiler&) = delete;
    CPerformanceProfiler(CPerformanceProfiler&&) = delete;
    CPerformanceProfiler& operator=(CPerformanceProfiler&&) = delete;

    /**************************************************************************
     * Profiling Control
     **************************************************************************/

    // Start profiling a named operation
    void StartProfiling(const TString& in_strName,
                       const TString& in_strCategory = "");

    // Stop profiling a named operation
    void StopProfiling(const TString& in_strName);

    // Profile a function/lambda automatically (RAII)
    template<typename Func>
    auto Profile(const TString& in_strName, Func&& in_func) {
        StartProfiling(in_strName);
        auto result = std::forward<Func>(in_func)();
        StopProfiling(in_strName);
        return result;
    }

    // Profile a void function
    template<typename Func>
    void ProfileVoid(const TString& in_strName, Func&& in_func) {
        StartProfiling(in_strName);
        std::forward<Func>(in_func)();
        StopProfiling(in_strName);
    }

    /**************************************************************************
     * Data Collection
     **************************************************************************/

    // Add custom measurement
    void AddMeasurement(const SPerformanceMeasurement& in_measurement);

    // Record custom metric
    void RecordMetric(const TString& in_strName, TUInt64 in_valueNs,
                     const TString& in_strCategory = "");

    // Mark a checkpoint
    void Checkpoint(const TString& in_strName);

    /**************************************************************************
     * Statistics & Analysis
     **************************************************************************/

    // Calculate statistics for a specific metric
    [[nodiscard]] SPerformanceStats CalculateStats(const TString& in_strName) const;

    // Get all statistics grouped by category
    [[nodiscard]] std::map<TString, std::vector<SPerformanceStats>>
        GetStatsByCategory() const;

    // Get all measurements
    [[nodiscard]] const std::vector<SPerformanceMeasurement>&
        GetMeasurements() const { return m_measurements; }

    // Get measurements by name
    [[nodiscard]] std::vector<SPerformanceMeasurement>
        GetMeasurements(const TString& in_strName) const;

    /**************************************************************************
     * Reporting
     **************************************************************************/

    // Generate performance report
    CResult GenerateReport(const TString& in_strFilePath,
                          const SPerformanceReportConfig& in_config = {});

    // Generate HTML report
    CResult GenerateHtmlReport(const TString& in_strFilePath,
                              const SPerformanceReportConfig& in_config = {});

    // Generate JSON report
    CResult GenerateJsonReport(const TString& in_strFilePath,
                              const SPerformanceReportConfig& in_config = {});

    // Generate CSV report
    CResult GenerateCsvReport(const TString& in_strFilePath);

    // Print summary to console
    void PrintSummary() const;

    /**************************************************************************
     * Configuration & Control
     **************************************************************************/

    // Enable/disable profiling
    void SetEnabled(bool in_bEnabled) { m_bEnabled = in_bEnabled; }
    [[nodiscard]] bool IsEnabled() const { return m_bEnabled; }

    // Enable/disable memory tracking
    void SetMemoryTrackingEnabled(bool in_bEnabled) {
        m_bMemoryTrackingEnabled = in_bEnabled;
    }

    // Clear all collected data
    void Clear();

    // Reset statistics
    void Reset();

    /**************************************************************************
     * Baseline & Regression Detection
     **************************************************************************/

    // Set performance baseline
    void SetBaseline(const TString& in_strName, TUInt64 in_baselineNs);

    // Check if measurement exceeds baseline (regression detection)
    [[nodiscard]] bool ExceedsBaseline(const TString& in_strName,
                                       TDouble in_thresholdPercent = 10.0) const;

    // Get regression report
    [[nodiscard]] std::vector<TString> GetRegressions(
        TDouble in_thresholdPercent = 10.0) const;

private:
    CPerformanceProfiler() = default;
    ~CPerformanceProfiler() = default;

    // Get current memory usage
    [[nodiscard]] TUInt64 GetCurrentMemoryUsage() const;

    // Generate HTML content
    [[nodiscard]] TString GenerateHtmlContent(
        const SPerformanceReportConfig& in_config) const;

    // Generate JSON content
    [[nodiscard]] TString GenerateJsonContent(
        const SPerformanceReportConfig& in_config) const;

    // Calculate percentile
    [[nodiscard]] static TDouble CalculatePercentile(
        const std::vector<TUInt64>& in_values, TDouble in_percentile);

    // Member variables
    mutable std::mutex m_mutex;
    std::vector<SPerformanceMeasurement> m_measurements;
    std::map<TString, std::chrono::steady_clock::time_point> m_activeProfiles;
    std::map<TString, std::vector<TUInt64>> m_metricsByName;
    std::map<TString, TUInt64> m_baselines;
    bool m_bEnabled{true};
    bool m_bMemoryTrackingEnabled{false};
};

/**************************************************************************
 * Class: CProfileScope
 * Description: RAII-based profiling scope
 *
 * Usage:
 *   {
 *       CProfileScope profile("MyOperation");
 *       // ... code to profile ...
 *   } // Automatically stops profiling when scope exits
 **************************************************************************/
class CProfileScope {
public:
    explicit CProfileScope(const TString& in_strName,
                          const TString& in_strCategory = "")
        : m_strName(in_strName)
    {
        CPerformanceProfiler::GetInstance().StartProfiling(in_strName, in_strCategory);
    }

    ~CProfileScope() {
        CPerformanceProfiler::GetInstance().StopProfiling(m_strName);
    }

    // Delete copy/move
    CProfileScope(const CProfileScope&) = delete;
    CProfileScope& operator=(const CProfileScope&) = delete;
    CProfileScope(CProfileScope&&) = delete;
    CProfileScope& operator=(CProfileScope&&) = delete;

private:
    TString m_strName;
};

/**************************************************************************
 * Convenience Macros
 **************************************************************************/

// Profile a scope automatically
#define PROFILE_SCOPE(name) \
    CProfileScope _profile_scope_##__LINE__(name)

// Profile a scope with category
#define PROFILE_SCOPE_CAT(name, category) \
    CProfileScope _profile_scope_##__LINE__(name, category)

// Profile a function
#define PROFILE_FUNCTION() \
    CProfileScope _profile_scope_##__LINE__(__FUNCTION__)

} // namespace TestMATE
