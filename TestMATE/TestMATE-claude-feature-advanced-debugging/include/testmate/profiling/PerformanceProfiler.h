/**
 * @file PerformanceProfiler.h
 * @brief Performance profiling system for test execution analysis
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the performance profiling system that tracks timing,
 * memory usage, and identifies bottlenecks in test execution.
 */

#ifndef TESTMATE_PROFILING_PERFORMANCE_PROFILER_H
#define TESTMATE_PROFILING_PERFORMANCE_PROFILER_H

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <optional>

namespace TestMATE {

/**
 * @brief Performance metric types
 */
enum class EMetricType {
    kDuration,          ///< Time duration measurement
    kMemoryUsage,       ///< Memory usage in bytes
    kCPUUsage,          ///< CPU usage percentage
    kCallCount,         ///< Number of calls/invocations
    kThroughput,        ///< Operations per second
    kLatency            ///< Response time
};

/**
 * @brief Performance sample data point
 */
struct SPerformanceSample {
    TString name;                               ///< Sample name/identifier
    EMetricType type{EMetricType::kDuration};   ///< Metric type
    TDouble value{0.0};                         ///< Measured value
    TTimePoint timestamp;                       ///< When sample was taken
    TString unit;                               ///< Unit of measurement
    TMap<TString, TString> metadata;            ///< Additional metadata
};

/**
 * @brief Performance statistics for a metric
 */
struct SPerformanceStats {
    TString name;                   ///< Metric name
    EMetricType type;               ///< Metric type
    TUInt32 sampleCount{0};         ///< Number of samples
    TDouble minValue{0.0};          ///< Minimum value
    TDouble maxValue{0.0};          ///< Maximum value
    TDouble meanValue{0.0};         ///< Mean/average value
    TDouble medianValue{0.0};       ///< Median value
    TDouble stdDeviation{0.0};      ///< Standard deviation
    TDouble p95Value{0.0};          ///< 95th percentile
    TDouble p99Value{0.0};          ///< 99th percentile
    TDouble totalValue{0.0};        ///< Sum of all values
};

/**
 * @brief Bottleneck detection result
 */
struct SBottleneck {
    TString location;               ///< Where bottleneck occurs
    TString description;            ///< Description of bottleneck
    EMetricType metricType;         ///< Type of metric causing bottleneck
    TDouble actualValue{0.0};       ///< Actual measured value
    TDouble expectedValue{0.0};     ///< Expected/baseline value
    TDouble severity{0.0};          ///< Severity score (0.0 to 1.0)
    TVector<TString> recommendations; ///< Optimization recommendations
};

/**
 * @brief Performance profile report
 */
struct SPerformanceReport {
    TString profileName;                        ///< Profile name
    TTimePoint startTime;                       ///< Profiling start time
    TTimePoint endTime;                         ///< Profiling end time
    TDouble totalDurationMs{0.0};               ///< Total profiling duration
    TMap<TString, SPerformanceStats> statistics; ///< Statistics by metric name
    TVector<SBottleneck> bottlenecks;           ///< Detected bottlenecks
    TMap<TString, TDouble> summary;             ///< Summary metrics
};

/**
 * @brief Profiler configuration
 */
struct SProfilerConfig {
    bool enableMemoryProfiling{true};           ///< Track memory usage
    bool enableCPUProfiling{false};             ///< Track CPU usage (expensive)
    bool enableCallCounting{true};              ///< Count function calls
    bool autoDetectBottlenecks{true};           ///< Auto-detect bottlenecks
    TDouble bottleneckThreshold{2.0};           ///< Threshold for bottleneck detection (multiplier)
    TUInt32 maxSamplesPerMetric{10000};         ///< Max samples to keep per metric
    bool enableDetailedTimings{true};           ///< Track detailed timing info
};

/**
 * @brief Performance profiling callback
 */
using FProfilerCallback = std::function<void(const SPerformanceSample&)>;

/**
 * @brief Performance profiler for test execution
 *
 * CPerformanceProfiler provides comprehensive performance measurement
 * capabilities including timing analysis, memory tracking, and bottleneck
 * detection.
 *
 * Example usage:
 * @code
 * SProfilerConfig config;
 * config.autoDetectBottlenecks = true;
 *
 * CPerformanceProfiler profiler("MyTest", config);
 *
 * profiler.Start();
 *
 * {
 *     auto scope = profiler.CreateScopedTimer("TestStep1");
 *     // ... test code ...
 * }
 *
 * profiler.RecordMemoryUsage("AfterLoad", GetCurrentMemoryUsage());
 *
 * profiler.Stop();
 * auto report = profiler.GenerateReport();
 *
 * for (const auto& bottleneck : report.bottlenecks) {
 *     std::cout << "Bottleneck: " << bottleneck.description << "\n";
 * }
 * @endcode
 */
class CPerformanceProfiler {
public:
    // Forward declaration of scoped timer
    class CScopedTimer;

    /**
     * @brief Construct profiler with name and configuration
     * @param in_profileName Name for this profiling session
     * @param in_config Profiler configuration
     */
    explicit CPerformanceProfiler(
        const TString& in_profileName,
        const SProfilerConfig& in_config = {});

    /**
     * @brief Start profiling session
     */
    void Start();

    /**
     * @brief Stop profiling session
     */
    void Stop();

    /**
     * @brief Check if profiler is running
     * @return true if profiling active
     */
    [[nodiscard]] bool IsRunning() const { return m_isRunning; }

    /**
     * @brief Record duration sample
     * @param in_name Sample name
     * @param in_durationMs Duration in milliseconds
     */
    void RecordDuration(const TString& in_name, TDouble in_durationMs);

    /**
     * @brief Record memory usage sample
     * @param in_name Sample name
     * @param in_bytes Memory usage in bytes
     */
    void RecordMemoryUsage(const TString& in_name, TUInt64 in_bytes);

    /**
     * @brief Record CPU usage sample
     * @param in_name Sample name
     * @param in_percentage CPU usage percentage (0-100)
     */
    void RecordCPUUsage(const TString& in_name, TDouble in_percentage);

    /**
     * @brief Increment call counter
     * @param in_name Counter name
     */
    void IncrementCallCount(const TString& in_name);

    /**
     * @brief Record custom metric
     * @param in_sample Custom performance sample
     */
    void RecordSample(const SPerformanceSample& in_sample);

    /**
     * @brief Create scoped timer that auto-records on destruction
     * @param in_name Timer name
     * @return RAII timer object
     */
    [[nodiscard]] CScopedTimer CreateScopedTimer(const TString& in_name);

    /**
     * @brief Generate performance report
     * @return Complete performance report
     */
    [[nodiscard]] SPerformanceReport GenerateReport();

    /**
     * @brief Get statistics for specific metric
     * @param in_metricName Metric name
     * @return Statistics or nullopt if not found
     */
    [[nodiscard]] std::optional<SPerformanceStats> GetStatistics(
        const TString& in_metricName) const;

    /**
     * @brief Detect bottlenecks in current profile
     * @return Vector of detected bottlenecks
     */
    [[nodiscard]] TVector<SBottleneck> DetectBottlenecks() const;

    /**
     * @brief Set callback for real-time sample notifications
     * @param in_callback Callback function
     */
    void SetCallback(FProfilerCallback in_callback);

    /**
     * @brief Clear all collected samples
     */
    void ClearSamples();

    /**
     * @brief Get current memory usage of process
     * @return Memory usage in bytes
     */
    [[nodiscard]] static TUInt64 GetCurrentMemoryUsage();

    /**
     * @brief Get current CPU usage percentage
     * @return CPU usage (0-100)
     */
    [[nodiscard]] static TDouble GetCurrentCPUUsage();

    /**
     * @brief Scoped timer class for RAII timing
     */
    class CScopedTimer {
    public:
        CScopedTimer(CPerformanceProfiler* in_profiler, const TString& in_name);
        ~CScopedTimer();

        // Non-copyable, movable
        CScopedTimer(const CScopedTimer&) = delete;
        CScopedTimer& operator=(const CScopedTimer&) = delete;
        CScopedTimer(CScopedTimer&&) noexcept = default;
        CScopedTimer& operator=(CScopedTimer&&) noexcept = default;

        /**
         * @brief Stop timer early (before destruction)
         */
        void Stop();

    private:
        CPerformanceProfiler* m_profiler;
        TString m_name;
        TTimePoint m_startTime;
        bool m_stopped{false};
    };

private:
    /**
     * @brief Calculate statistics for samples
     * @param in_samples Vector of samples
     * @param in_name Metric name
     * @param in_type Metric type
     * @return Calculated statistics
     */
    SPerformanceStats CalculateStatistics(
        const TVector<SPerformanceSample>& in_samples,
        const TString& in_name,
        EMetricType in_type) const;

    /**
     * @brief Notify callback if set
     * @param in_sample Sample to notify
     */
    void NotifyCallback(const SPerformanceSample& in_sample);

    TString m_profileName;
    SProfilerConfig m_config;
    std::atomic<bool> m_isRunning{false};
    TTimePoint m_startTime;
    TTimePoint m_endTime;

    mutable std::mutex m_mutex;
    TMap<TString, TVector<SPerformanceSample>> m_samples;  ///< Samples grouped by name
    TMap<TString, TUInt64> m_callCounts;                   ///< Call counters
    FProfilerCallback m_callback;
};

} // namespace TestMATE

#endif // TESTMATE_PROFILING_PERFORMANCE_PROFILER_H
