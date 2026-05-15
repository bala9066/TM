/**************************************************************************
 * File Name: PerformanceProfiler.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Performance profiling system implementation
 **************************************************************************/

#include "testmate/profiling/PerformanceProfiler.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <unistd.h>
    #include <sys/resource.h>
    #include <sys/times.h>
    #include <fstream>
#endif

namespace TestMATE {

//=============================================================================
// CPerformanceProfiler Implementation
//=============================================================================

CPerformanceProfiler::CPerformanceProfiler(
    const TString& in_profileName,
    const SProfilerConfig& in_config)
    : m_profileName(in_profileName)
    , m_config(in_config)
{
}

void CPerformanceProfiler::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_startTime = std::chrono::steady_clock::now();
    m_isRunning = true;
    m_samples.clear();
    m_callCounts.clear();

    // Record initial memory usage if enabled
    if (m_config.enableMemoryProfiling) {
        SPerformanceSample sample;
        sample.name = "InitialMemory";
        sample.type = EMetricType::kMemoryUsage;
        sample.value = static_cast<TDouble>(GetCurrentMemoryUsage());
        sample.timestamp = m_startTime;
        sample.unit = "bytes";

        m_samples[sample.name].push_back(sample);
    }
}

void CPerformanceProfiler::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_endTime = std::chrono::steady_clock::now();
    m_isRunning = false;

    // Record final memory usage if enabled
    if (m_config.enableMemoryProfiling) {
        SPerformanceSample sample;
        sample.name = "FinalMemory";
        sample.type = EMetricType::kMemoryUsage;
        sample.value = static_cast<TDouble>(GetCurrentMemoryUsage());
        sample.timestamp = m_endTime;
        sample.unit = "bytes";

        m_samples[sample.name].push_back(sample);
    }
}

void CPerformanceProfiler::RecordDuration(const TString& in_name, TDouble in_durationMs) {
    SPerformanceSample sample;
    sample.name = in_name;
    sample.type = EMetricType::kDuration;
    sample.value = in_durationMs;
    sample.timestamp = std::chrono::steady_clock::now();
    sample.unit = "ms";

    RecordSample(sample);
}

void CPerformanceProfiler::RecordMemoryUsage(const TString& in_name, TUInt64 in_bytes) {
    if (!m_config.enableMemoryProfiling) {
        return;
    }

    SPerformanceSample sample;
    sample.name = in_name;
    sample.type = EMetricType::kMemoryUsage;
    sample.value = static_cast<TDouble>(in_bytes);
    sample.timestamp = std::chrono::steady_clock::now();
    sample.unit = "bytes";

    RecordSample(sample);
}

void CPerformanceProfiler::RecordCPUUsage(const TString& in_name, TDouble in_percentage) {
    if (!m_config.enableCPUProfiling) {
        return;
    }

    SPerformanceSample sample;
    sample.name = in_name;
    sample.type = EMetricType::kCPUUsage;
    sample.value = in_percentage;
    sample.timestamp = std::chrono::steady_clock::now();
    sample.unit = "%";

    RecordSample(sample);
}

void CPerformanceProfiler::IncrementCallCount(const TString& in_name) {
    if (!m_config.enableCallCounting) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_callCounts[in_name]++;
}

void CPerformanceProfiler::RecordSample(const SPerformanceSample& in_sample) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& samples = m_samples[in_sample.name];

    // Enforce max samples limit
    if (samples.size() >= m_config.maxSamplesPerMetric) {
        samples.erase(samples.begin());  // Remove oldest sample
    }

    samples.push_back(in_sample);

    // Notify callback if set (without lock to avoid deadlock)
    NotifyCallback(in_sample);
}

CPerformanceProfiler::CScopedTimer CPerformanceProfiler::CreateScopedTimer(
    const TString& in_name) {
    return CScopedTimer(this, in_name);
}

SPerformanceReport CPerformanceProfiler::GenerateReport() {
    std::lock_guard<std::mutex> lock(m_mutex);

    SPerformanceReport report;
    report.profileName = m_profileName;
    report.startTime = m_startTime;
    report.endTime = m_endTime;

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        m_endTime - m_startTime);
    report.totalDurationMs = static_cast<TDouble>(duration.count());

    // Calculate statistics for each metric
    for (const auto& [name, samples] : m_samples) {
        if (!samples.empty()) {
            auto stats = CalculateStatistics(samples, name, samples[0].type);
            report.statistics[name] = stats;
        }
    }

    // Add call counts to summary
    for (const auto& [name, count] : m_callCounts) {
        report.summary[name + "_CallCount"] = static_cast<TDouble>(count);
    }

    // Detect bottlenecks if enabled
    if (m_config.autoDetectBottlenecks) {
        report.bottlenecks = DetectBottlenecks();
    }

    return report;
}

std::optional<SPerformanceStats> CPerformanceProfiler::GetStatistics(
    const TString& in_metricName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_samples.find(in_metricName);
    if (it == m_samples.end() || it->second.empty()) {
        return std::nullopt;
    }

    return CalculateStatistics(it->second, in_metricName, it->second[0].type);
}

TVector<SBottleneck> CPerformanceProfiler::DetectBottlenecks() const {
    TVector<SBottleneck> bottlenecks;

    // Analyze duration metrics for bottlenecks
    TVector<std::pair<TString, TDouble>> durations;

    for (const auto& [name, samples] : m_samples) {
        if (samples.empty() || samples[0].type != EMetricType::kDuration) {
            continue;
        }

        auto stats = CalculateStatistics(samples, name, EMetricType::kDuration);
        durations.push_back({name, stats.meanValue});
    }

    if (durations.size() < 2) {
        return bottlenecks;  // Need at least 2 metrics to compare
    }

    // Calculate overall mean
    TDouble overallMean = 0.0;
    for (const auto& [name, duration] : durations) {
        overallMean += duration;
    }
    overallMean /= durations.size();

    // Detect outliers (items significantly slower than average)
    for (const auto& [name, duration] : durations) {
        if (duration > overallMean * m_config.bottleneckThreshold) {
            SBottleneck bottleneck;
            bottleneck.location = name;
            bottleneck.description = "Execution time significantly higher than average";
            bottleneck.metricType = EMetricType::kDuration;
            bottleneck.actualValue = duration;
            bottleneck.expectedValue = overallMean;
            bottleneck.severity = std::min(1.0, (duration / overallMean) / 10.0);

            // Add recommendations
            bottleneck.recommendations.push_back("Profile this operation in detail");
            bottleneck.recommendations.push_back("Consider optimization or caching");
            if (duration > 1000.0) {
                bottleneck.recommendations.push_back("Consider asynchronous execution");
            }

            bottlenecks.push_back(bottleneck);
        }
    }

    // Sort bottlenecks by severity (highest first)
    std::sort(bottlenecks.begin(), bottlenecks.end(),
        [](const SBottleneck& a, const SBottleneck& b) {
            return a.severity > b.severity;
        });

    return bottlenecks;
}

void CPerformanceProfiler::SetCallback(FProfilerCallback in_callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(in_callback);
}

void CPerformanceProfiler::ClearSamples() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_samples.clear();
    m_callCounts.clear();
}

TUInt64 CPerformanceProfiler::GetCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                            sizeof(pmc))) {
        return static_cast<TUInt64>(pmc.WorkingSetSize);
    }
    return 0;
#else
    // Linux/Unix: Read from /proc/self/status
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        return 0;
    }

    TString line;
    while (std::getline(statusFile, line)) {
        if (line.find("VmRSS:") == 0) {
            // Extract memory size in KB
            size_t pos = line.find_first_of("0123456789");
            if (pos != TString::npos) {
                TUInt64 memKB = std::stoull(line.substr(pos));
                return memKB * 1024;  // Convert to bytes
            }
        }
    }

    return 0;
#endif
}

TDouble CPerformanceProfiler::GetCurrentCPUUsage() {
#ifdef _WIN32
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static int numProcessors = 0;
    static HANDLE self = GetCurrentProcess();

    if (numProcessors == 0) {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;

        FILETIME ftime, fsys, fuser;
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&lastCPU, &ftime, sizeof(FILETIME));

        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
        memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
        return 0.0;
    }

    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    double percent = (sys.QuadPart - lastSysCPU.QuadPart) +
                     (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;

    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    return percent * 100.0;
#else
    // Linux/Unix: Read from /proc/self/stat
    static clock_t lastCPU = 0;
    static clock_t lastSysCPU = 0;
    static clock_t lastUserCPU = 0;

    std::ifstream statFile("/proc/self/stat");
    if (!statFile.is_open()) {
        return 0.0;
    }

    TString line;
    std::getline(statFile, line);

    // Parse stat file (fields 14 and 15 are utime and stime)
    std::istringstream iss(line);
    TString token;
    clock_t utime = 0, stime = 0;

    for (int i = 0; i < 15 && iss >> token; ++i) {
        if (i == 13) utime = std::stol(token);
        if (i == 14) stime = std::stol(token);
    }

    clock_t now = times(NULL);
    clock_t total = (utime + stime) - (lastUserCPU + lastSysCPU);
    clock_t elapsed = now - lastCPU;

    lastCPU = now;
    lastUserCPU = utime;
    lastSysCPU = stime;

    if (elapsed > 0) {
        return (static_cast<TDouble>(total) / elapsed) * 100.0;
    }

    return 0.0;
#endif
}

SPerformanceStats CPerformanceProfiler::CalculateStatistics(
    const TVector<SPerformanceSample>& in_samples,
    const TString& in_name,
    EMetricType in_type) const {

    SPerformanceStats stats;
    stats.name = in_name;
    stats.type = in_type;
    stats.sampleCount = static_cast<TUInt32>(in_samples.size());

    if (in_samples.empty()) {
        return stats;
    }

    // Extract values
    TVector<TDouble> values;
    values.reserve(in_samples.size());
    for (const auto& sample : in_samples) {
        values.push_back(sample.value);
    }

    // Sort for percentile calculations
    auto sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());

    // Min/Max
    stats.minValue = sortedValues.front();
    stats.maxValue = sortedValues.back();

    // Sum/Total
    stats.totalValue = std::accumulate(values.begin(), values.end(), 0.0);

    // Mean
    stats.meanValue = stats.totalValue / values.size();

    // Median
    size_t mid = sortedValues.size() / 2;
    if (sortedValues.size() % 2 == 0) {
        stats.medianValue = (sortedValues[mid - 1] + sortedValues[mid]) / 2.0;
    } else {
        stats.medianValue = sortedValues[mid];
    }

    // Standard deviation
    TDouble variance = 0.0;
    for (const auto& value : values) {
        TDouble diff = value - stats.meanValue;
        variance += diff * diff;
    }
    variance /= values.size();
    stats.stdDeviation = std::sqrt(variance);

    // Percentiles
    auto getPercentile = [&sortedValues](TDouble percentile) -> TDouble {
        size_t index = static_cast<size_t>(
            (sortedValues.size() - 1) * percentile);
        return sortedValues[index];
    };

    stats.p95Value = getPercentile(0.95);
    stats.p99Value = getPercentile(0.99);

    return stats;
}

void CPerformanceProfiler::NotifyCallback(const SPerformanceSample& in_sample) {
    if (m_callback) {
        // Call without lock to avoid deadlock
        m_callback(in_sample);
    }
}

//=============================================================================
// CScopedTimer Implementation
//=============================================================================

CPerformanceProfiler::CScopedTimer::CScopedTimer(
    CPerformanceProfiler* in_profiler,
    const TString& in_name)
    : m_profiler(in_profiler)
    , m_name(in_name)
    , m_startTime(std::chrono::steady_clock::now())
{
}

CPerformanceProfiler::CScopedTimer::~CScopedTimer() {
    if (!m_stopped) {
        Stop();
    }
}

void CPerformanceProfiler::CScopedTimer::Stop() {
    if (m_stopped || !m_profiler) {
        return;
    }

    m_stopped = true;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - m_startTime);

    TDouble durationMs = static_cast<TDouble>(duration.count()) / 1000.0;
    m_profiler->RecordDuration(m_name, durationMs);
}

} // namespace TestMATE
