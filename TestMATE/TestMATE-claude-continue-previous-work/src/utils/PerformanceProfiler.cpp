/**************************************************************************
 * File Name: PerformanceProfiler.cpp
 * Description: Implementation of performance profiling utilities
 **************************************************************************/

#include "utils/PerformanceProfiler.h"
#include "utils/LogManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <set>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace TestMATE {

/**************************************************************************
 * Singleton Instance
 **************************************************************************/
CPerformanceProfiler& CPerformanceProfiler::GetInstance() {
    static CPerformanceProfiler instance;
    return instance;
}

/**************************************************************************
 * Profiling Control
 **************************************************************************/
void CPerformanceProfiler::StartProfiling(const TString& in_strName,
                                         const TString& in_strCategory) {
    if (!m_bEnabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    m_activeProfiles[in_strName] = now;

    LOG_DEBUG("PerformanceProfiler", "Started profiling: {}", in_strName);
}

void CPerformanceProfiler::StopProfiling(const TString& in_strName) {
    if (!m_bEnabled) return;

    auto endTime = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeProfiles.find(in_strName);
    if (it == m_activeProfiles.end()) {
        LOG_WARNING("PerformanceProfiler", "No active profiling found for: {}", in_strName);
        return;
    }

    SPerformanceMeasurement measurement;
    measurement.name = in_strName;
    measurement.startTime = it->second;
    measurement.endTime = endTime;
    measurement.durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTime - it->second).count();

    if (m_bMemoryTrackingEnabled) {
        measurement.memoryUsedBytes = GetCurrentMemoryUsage();
    }

    m_measurements.push_back(measurement);
    m_metricsByName[in_strName].push_back(measurement.durationNs);

    m_activeProfiles.erase(it);

    LOG_DEBUG("PerformanceProfiler", "Stopped profiling: {} ({} μs)", in_strName, measurement.durationNs / 1000.0);
}

/**************************************************************************
 * Data Collection
 **************************************************************************/
void CPerformanceProfiler::AddMeasurement(const SPerformanceMeasurement& in_measurement) {
    if (!m_bEnabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_measurements.push_back(in_measurement);
    m_metricsByName[in_measurement.name].push_back(in_measurement.durationNs);
}

void CPerformanceProfiler::RecordMetric(const TString& in_strName,
                                       TUInt64 in_valueNs,
                                       const TString& in_strCategory) {
    if (!m_bEnabled) return;

    SPerformanceMeasurement measurement;
    measurement.name = in_strName;
    measurement.durationNs = in_valueNs;
    measurement.category = in_strCategory;
    measurement.startTime = std::chrono::steady_clock::now();
    measurement.endTime = measurement.startTime;

    AddMeasurement(measurement);
}

void CPerformanceProfiler::Checkpoint(const TString& in_strName) {
    if (!m_bEnabled) return;

    LOG_INFO("PerformanceProfiler", "Performance checkpoint: {}", in_strName);
    PrintSummary();
}

/**************************************************************************
 * Statistics & Analysis
 **************************************************************************/
SPerformanceStats CPerformanceProfiler::CalculateStats(const TString& in_strName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    SPerformanceStats stats;
    stats.name = in_strName;

    auto it = m_metricsByName.find(in_strName);
    if (it == m_metricsByName.end() || it->second.empty()) {
        return stats;
    }

    const auto& values = it->second;
    stats.count = values.size();

    // Calculate min, max, total
    for (auto value : values) {
        stats.totalNs += value;
        stats.minNs = std::min(stats.minNs, value);
        stats.maxNs = std::max(stats.maxNs, value);
    }

    // Calculate average
    stats.avgNs = static_cast<TDouble>(stats.totalNs) / stats.count;

    // Calculate standard deviation
    TDouble variance = 0.0;
    for (auto value : values) {
        TDouble diff = static_cast<TDouble>(value) - stats.avgNs;
        variance += diff * diff;
    }
    variance /= stats.count;
    stats.stdDevNs = std::sqrt(variance);

    // Calculate percentiles
    auto sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());

    stats.medianNs = CalculatePercentile(sortedValues, 50.0);
    stats.percentile95Ns = CalculatePercentile(sortedValues, 95.0);
    stats.percentile99Ns = CalculatePercentile(sortedValues, 99.0);

    return stats;
}

std::map<TString, std::vector<SPerformanceStats>>
CPerformanceProfiler::GetStatsByCategory() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::map<TString, std::vector<SPerformanceStats>> statsByCategory;

    // Group measurements by category
    std::map<TString, std::set<TString>> metricsByCategory;
    for (const auto& measurement : m_measurements) {
        TString category = measurement.category.empty() ? "General" : measurement.category;
        metricsByCategory[category].insert(measurement.name);
    }

    // Calculate stats for each metric in each category
    for (const auto& [category, metrics] : metricsByCategory) {
        std::vector<SPerformanceStats> categoryStats;
        for (const auto& metric : metrics) {
            auto stats = CalculateStats(metric);
            if (stats.count > 0) {
                categoryStats.push_back(stats);
            }
        }
        if (!categoryStats.empty()) {
            statsByCategory[category] = categoryStats;
        }
    }

    return statsByCategory;
}

std::vector<SPerformanceMeasurement>
CPerformanceProfiler::GetMeasurements(const TString& in_strName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<SPerformanceMeasurement> result;
    for (const auto& measurement : m_measurements) {
        if (measurement.name == in_strName) {
            result.push_back(measurement);
        }
    }
    return result;
}

/**************************************************************************
 * Reporting
 **************************************************************************/
CResult CPerformanceProfiler::GenerateReport(const TString& in_strFilePath,
                                            const SPerformanceReportConfig& in_config) {
    if (in_config.outputFormat == "html") {
        return GenerateHtmlReport(in_strFilePath, in_config);
    } else if (in_config.outputFormat == "json") {
        return GenerateJsonReport(in_strFilePath, in_config);
    } else if (in_config.outputFormat == "csv") {
        return GenerateCsvReport(in_strFilePath);
    } else {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                            "Unknown report format: " + in_config.outputFormat);
    }
}

CResult CPerformanceProfiler::GenerateHtmlReport(const TString& in_strFilePath,
                                                const SPerformanceReportConfig& in_config) {
    try {
        std::ofstream file(in_strFilePath);
        if (!file.is_open()) {
            return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to open file: " + in_strFilePath);
        }

        file << GenerateHtmlContent(in_config);
        file.close();

        LOG_INFO("PerformanceProfiler", "Performance report generated: {}", in_strFilePath);
        return TESTMATE_SUCCESS();
    }
    catch (const std::exception& e) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, std::string(e.what()));
    }
}

CResult CPerformanceProfiler::GenerateJsonReport(const TString& in_strFilePath,
                                                const SPerformanceReportConfig& in_config) {
    try {
        std::ofstream file(in_strFilePath);
        if (!file.is_open()) {
            return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to open file: " + in_strFilePath);
        }

        file << GenerateJsonContent(in_config);
        file.close();

        LOG_INFO("PerformanceProfiler", "Performance report (JSON) generated: {}", in_strFilePath);
        return TESTMATE_SUCCESS();
    }
    catch (const std::exception& e) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, std::string(e.what()));
    }
}

CResult CPerformanceProfiler::GenerateCsvReport(const TString& in_strFilePath) {
    try {
        std::ofstream file(in_strFilePath);
        if (!file.is_open()) {
            return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to open file: " + in_strFilePath);
        }

        // Header
        file << "Metric,Count,Min(μs),Max(μs),Avg(μs),StdDev(μs),Median(μs),P95(μs),P99(μs)\n";

        // Data rows
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [name, _] : m_metricsByName) {
            auto stats = CalculateStats(name);
            if (stats.count > 0) {
                file << name << ","
                     << stats.count << ","
                     << (stats.minNs / 1000.0) << ","
                     << (stats.maxNs / 1000.0) << ","
                     << (stats.avgNs / 1000.0) << ","
                     << (stats.stdDevNs / 1000.0) << ","
                     << (stats.medianNs / 1000.0) << ","
                     << (stats.percentile95Ns / 1000.0) << ","
                     << (stats.percentile99Ns / 1000.0) << "\n";
            }
        }

        file.close();
        LOG_INFO("PerformanceProfiler", "Performance report (CSV) generated: {}", in_strFilePath);
        return TESTMATE_SUCCESS();
    }
    catch (const std::exception& e) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, std::string(e.what()));
    }
}

void CPerformanceProfiler::PrintSummary() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::cout << "\n========== Performance Summary ==========\n";
    std::cout << "Total Measurements: " << m_measurements.size() << "\n";
    std::cout << "Unique Metrics: " << m_metricsByName.size() << "\n\n";

    for (const auto& [name, _] : m_metricsByName) {
        auto stats = CalculateStats(name);
        if (stats.count > 0) {
            std::cout << std::setw(30) << std::left << name << ": "
                     << std::setw(8) << std::right << std::fixed << std::setprecision(2)
                     << stats.GetAvgUs() << " μs avg ("
                     << stats.count << " samples)\n";
        }
    }
    std::cout << "=========================================\n\n";
}

/**************************************************************************
 * Configuration & Control
 **************************************************************************/
void CPerformanceProfiler::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_measurements.clear();
    m_metricsByName.clear();
    m_activeProfiles.clear();
    LOG_INFO("PerformanceProfiler", "Performance profiler cleared");
}

void CPerformanceProfiler::Reset() {
    Clear();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_baselines.clear();
    LOG_INFO("PerformanceProfiler", "Performance profiler reset");
}

/**************************************************************************
 * Baseline & Regression Detection
 **************************************************************************/
void CPerformanceProfiler::SetBaseline(const TString& in_strName, TUInt64 in_baselineNs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_baselines[in_strName] = in_baselineNs;
    LOG_INFO("PerformanceProfiler", "Baseline set for {}: {} μs", in_strName, in_baselineNs / 1000.0);
}

bool CPerformanceProfiler::ExceedsBaseline(const TString& in_strName,
                                          TDouble in_thresholdPercent) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto baselineIt = m_baselines.find(in_strName);
    if (baselineIt == m_baselines.end()) {
        return false; // No baseline set
    }

    auto stats = CalculateStats(in_strName);
    if (stats.count == 0) {
        return false; // No measurements
    }

    TDouble baseline = static_cast<TDouble>(baselineIt->second);
    TDouble threshold = baseline * (1.0 + in_thresholdPercent / 100.0);

    return stats.avgNs > threshold;
}

std::vector<TString> CPerformanceProfiler::GetRegressions(
    TDouble in_thresholdPercent) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<TString> regressions;

    for (const auto& [name, baseline] : m_baselines) {
        auto stats = CalculateStats(name);
        if (stats.count == 0) continue;

        TDouble threshold = static_cast<TDouble>(baseline) *
                           (1.0 + in_thresholdPercent / 100.0);

        if (stats.avgNs > threshold) {
            TDouble percentOver = ((stats.avgNs - baseline) / baseline) * 100.0;
            std::ostringstream oss;
            oss << name << ": " << std::fixed << std::setprecision(1)
                << percentOver << "% over baseline ("
                << (stats.avgNs / 1000.0) << " μs vs "
                << (baseline / 1000.0) << " μs)";
            regressions.push_back(oss.str());
        }
    }

    return regressions;
}

/**************************************************************************
 * Private Helper Methods
 **************************************************************************/
TUInt64 CPerformanceProfiler::GetCurrentMemoryUsage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                            (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert KB to bytes
    }
    return 0;
#endif
}

TString CPerformanceProfiler::GenerateHtmlContent(
    const SPerformanceReportConfig& in_config) const {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>" << in_config.title << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << "h1 { color: #333; }\n";
    html << "h2 { color: #666; margin-top: 30px; }\n";
    html << "table { border-collapse: collapse; width: 100%; background: white; margin: 20px 0; }\n";
    html << "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    html << "th { background-color: #4CAF50; color: white; }\n";
    html << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    html << ".metric { font-weight: bold; }\n";
    html << ".good { color: #4CAF50; }\n";
    html << ".warning { color: #FF9800; }\n";
    html << ".error { color: #F44336; }\n";
    html << "</style>\n</head>\n<body>\n";

    html << "<h1>" << in_config.title << "</h1>\n";
    html << "<p>Generated: " << __DATE__ << " " << __TIME__ << "</p>\n";

    if (in_config.includeStatistics) {
        html << "<h2>Performance Statistics</h2>\n";
        html << "<table>\n";
        html << "<tr><th>Metric</th><th>Count</th><th>Min (μs)</th><th>Avg (μs)</th>";
        html << "<th>Max (μs)</th><th>StdDev (μs)</th><th>P95 (μs)</th><th>P99 (μs)</th></tr>\n";

        for (const auto& [name, _] : m_metricsByName) {
            auto stats = CalculateStats(name);
            if (stats.count > 0) {
                html << "<tr><td class='metric'>" << name << "</td>";
                html << "<td>" << stats.count << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.minNs / 1000.0) << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.avgNs / 1000.0) << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.maxNs / 1000.0) << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.stdDevNs / 1000.0) << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.percentile95Ns / 1000.0) << "</td>";
                html << "<td>" << std::fixed << std::setprecision(2)
                     << (stats.percentile99Ns / 1000.0) << "</td></tr>\n";
            }
        }
        html << "</table>\n";
    }

    html << "</body>\n</html>";
    return html.str();
}

TString CPerformanceProfiler::GenerateJsonContent(
    const SPerformanceReportConfig& in_config) const {
    std::ostringstream json;

    json << "{\n";
    json << "  \"title\": \"" << in_config.title << "\",\n";
    json << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
    json << "  \"statistics\": [\n";

    bool first = true;
    for (const auto& [name, _] : m_metricsByName) {
        auto stats = CalculateStats(name);
        if (stats.count > 0) {
            if (!first) json << ",\n";
            first = false;

            json << "    {\n";
            json << "      \"name\": \"" << name << "\",\n";
            json << "      \"count\": " << stats.count << ",\n";
            json << "      \"min_ns\": " << stats.minNs << ",\n";
            json << "      \"avg_ns\": " << stats.avgNs << ",\n";
            json << "      \"max_ns\": " << stats.maxNs << ",\n";
            json << "      \"stddev_ns\": " << stats.stdDevNs << ",\n";
            json << "      \"median_ns\": " << stats.medianNs << ",\n";
            json << "      \"p95_ns\": " << stats.percentile95Ns << ",\n";
            json << "      \"p99_ns\": " << stats.percentile99Ns << "\n";
            json << "    }";
        }
    }

    json << "\n  ]\n}\n";
    return json.str();
}

TDouble CPerformanceProfiler::CalculatePercentile(
    const std::vector<TUInt64>& in_values, TDouble in_percentile) {
    if (in_values.empty()) return 0.0;

    TDouble index = (in_percentile / 100.0) * (in_values.size() - 1);
    size_t lowerIndex = static_cast<size_t>(std::floor(index));
    size_t upperIndex = static_cast<size_t>(std::ceil(index));

    if (lowerIndex == upperIndex) {
        return static_cast<TDouble>(in_values[lowerIndex]);
    }

    TDouble weight = index - lowerIndex;
    return static_cast<TDouble>(in_values[lowerIndex]) * (1.0 - weight) +
           static_cast<TDouble>(in_values[upperIndex]) * weight;
}

} // namespace TestMATE
