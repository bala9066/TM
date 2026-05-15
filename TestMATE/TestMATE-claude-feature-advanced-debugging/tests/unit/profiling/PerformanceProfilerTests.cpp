/**************************************************************************
 * File Name: PerformanceProfilerTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Unit tests for performance profiler
 **************************************************************************/

#include <gtest/gtest.h>
#include "testmate/profiling/PerformanceProfiler.h"
#include <thread>
#include <chrono>

using namespace TestMATE;

//=============================================================================
// Test Fixtures
//=============================================================================

class PerformanceProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile");
    }

    std::unique_ptr<CPerformanceProfiler> m_profiler;
};

//=============================================================================
// Basic Functionality Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, StartStop) {
    EXPECT_FALSE(m_profiler->IsRunning());

    m_profiler->Start();
    EXPECT_TRUE(m_profiler->IsRunning());

    m_profiler->Stop();
    EXPECT_FALSE(m_profiler->IsRunning());
}

TEST_F(PerformanceProfilerTest, RecordDuration) {
    m_profiler->Start();

    m_profiler->RecordDuration("Operation1", 100.5);
    m_profiler->RecordDuration("Operation1", 150.3);
    m_profiler->RecordDuration("Operation1", 125.7);

    auto stats = m_profiler->GetStatistics("Operation1");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 3);
    EXPECT_DOUBLE_EQ(stats->minValue, 100.5);
    EXPECT_DOUBLE_EQ(stats->maxValue, 150.3);

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, RecordMemoryUsage) {
    SProfilerConfig config;
    config.enableMemoryProfiling = true;

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    m_profiler->RecordMemoryUsage("MemCheck1", 1024 * 1024);  // 1 MB
    m_profiler->RecordMemoryUsage("MemCheck1", 2 * 1024 * 1024);  // 2 MB

    auto stats = m_profiler->GetStatistics("MemCheck1");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 2);
    EXPECT_EQ(stats->type, EMetricType::kMemoryUsage);

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, CallCounting) {
    SProfilerConfig config;
    config.enableCallCounting = true;

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    for (int i = 0; i < 10; ++i) {
        m_profiler->IncrementCallCount("Function1");
    }

    auto report = m_profiler->GenerateReport();

    EXPECT_EQ(report.summary["Function1_CallCount"], 10.0);

    m_profiler->Stop();
}

//=============================================================================
// Scoped Timer Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, ScopedTimerBasic) {
    m_profiler->Start();

    {
        auto timer = m_profiler->CreateScopedTimer("ScopedOp");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }  // Timer auto-stops on destruction

    auto stats = m_profiler->GetStatistics("ScopedOp");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 1);
    EXPECT_GE(stats->meanValue, 40.0);  // At least 40ms (some tolerance for scheduling)
    EXPECT_LE(stats->meanValue, 100.0);  // But not more than 100ms

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, ScopedTimerMultiple) {
    m_profiler->Start();

    for (int i = 0; i < 5; ++i) {
        auto timer = m_profiler->CreateScopedTimer("Operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto stats = m_profiler->GetStatistics("Operation");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 5);

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, ScopedTimerEarlyStop) {
    m_profiler->Start();

    auto timer = m_profiler->CreateScopedTimer("EarlyStop");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    timer.Stop();  // Explicit stop

    // Additional work that shouldn't be timed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = m_profiler->GetStatistics("EarlyStop");

    ASSERT_TRUE(stats.has_value());
    EXPECT_LE(stats->meanValue, 40.0);  // Should be around 20ms, not 70ms

    m_profiler->Stop();
}

//=============================================================================
// Statistics Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, StatisticsCalculation) {
    m_profiler->Start();

    // Record samples: 10, 20, 30, 40, 50
    for (int i = 1; i <= 5; ++i) {
        m_profiler->RecordDuration("TestOp", i * 10.0);
    }

    auto stats = m_profiler->GetStatistics("TestOp");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 5);
    EXPECT_DOUBLE_EQ(stats->minValue, 10.0);
    EXPECT_DOUBLE_EQ(stats->maxValue, 50.0);
    EXPECT_DOUBLE_EQ(stats->meanValue, 30.0);  // (10+20+30+40+50)/5 = 30
    EXPECT_DOUBLE_EQ(stats->medianValue, 30.0);
    EXPECT_DOUBLE_EQ(stats->totalValue, 150.0);

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, PercentilesCalculation) {
    m_profiler->Start();

    // Record 100 samples: 1, 2, 3, ..., 100
    for (int i = 1; i <= 100; ++i) {
        m_profiler->RecordDuration("TestOp", static_cast<TDouble>(i));
    }

    auto stats = m_profiler->GetStatistics("TestOp");

    ASSERT_TRUE(stats.has_value());
    EXPECT_NEAR(stats->p95Value, 95.0, 1.0);  // 95th percentile
    EXPECT_NEAR(stats->p99Value, 99.0, 1.0);  // 99th percentile

    m_profiler->Stop();
}

//=============================================================================
// Report Generation Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, GenerateBasicReport) {
    SProfilerConfig config;
    config.enableMemoryProfiling = false;  // Disable auto memory recording

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    m_profiler->RecordDuration("Op1", 100.0);
    m_profiler->RecordDuration("Op2", 200.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    m_profiler->Stop();

    auto report = m_profiler->GenerateReport();

    EXPECT_EQ(report.profileName, "TestProfile");
    EXPECT_GE(report.totalDurationMs, 50.0);
    EXPECT_EQ(report.statistics.size(), 2);  // Op1 and Op2
    EXPECT_TRUE(report.statistics.count("Op1") > 0);
    EXPECT_TRUE(report.statistics.count("Op2") > 0);
}

//=============================================================================
// Bottleneck Detection Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, BottleneckDetection) {
    SProfilerConfig config;
    config.autoDetectBottlenecks = true;
    config.bottleneckThreshold = 2.0;  // 2x average
    config.enableMemoryProfiling = false;  // Focus on duration metrics only

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    // Record normal operations
    for (int i = 0; i < 5; ++i) {
        m_profiler->RecordDuration("FastOp1", 10.0);
        m_profiler->RecordDuration("FastOp2", 12.0);
    }

    // Record slow operation (much slower to exceed 2x average threshold)
    for (int i = 0; i < 5; ++i) {
        m_profiler->RecordDuration("SlowOp", 200.0);
    }

    m_profiler->Stop();

    auto bottlenecks = m_profiler->DetectBottlenecks();

    EXPECT_GE(bottlenecks.size(), 1);  // Should detect SlowOp

    if (!bottlenecks.empty()) {
        EXPECT_EQ(bottlenecks[0].location, "SlowOp");
        EXPECT_GT(bottlenecks[0].severity, 0.0);
        EXPECT_FALSE(bottlenecks[0].recommendations.empty());
    }
}

TEST_F(PerformanceProfilerTest, NoBottlenecksWhenAllEqual) {
    SProfilerConfig config;
    config.autoDetectBottlenecks = true;

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    // All operations take similar time
    m_profiler->RecordDuration("Op1", 50.0);
    m_profiler->RecordDuration("Op2", 55.0);
    m_profiler->RecordDuration("Op3", 45.0);

    m_profiler->Stop();

    auto bottlenecks = m_profiler->DetectBottlenecks();

    EXPECT_EQ(bottlenecks.size(), 0);  // No outliers
}

//=============================================================================
// Callback Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, CallbackInvocation) {
    m_profiler->Start();

    int callbackCount = 0;
    TString lastSampleName;

    m_profiler->SetCallback([&](const SPerformanceSample& sample) {
        callbackCount++;
        lastSampleName = sample.name;
    });

    m_profiler->RecordDuration("Op1", 100.0);
    m_profiler->RecordDuration("Op2", 200.0);

    EXPECT_EQ(callbackCount, 2);
    EXPECT_EQ(lastSampleName, "Op2");

    m_profiler->Stop();
}

//=============================================================================
// Configuration Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, DisableMemoryProfiling) {
    SProfilerConfig config;
    config.enableMemoryProfiling = false;

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    m_profiler->RecordMemoryUsage("MemOp", 1024);

    auto stats = m_profiler->GetStatistics("MemOp");

    EXPECT_FALSE(stats.has_value());  // Should not record when disabled

    m_profiler->Stop();
}

TEST_F(PerformanceProfilerTest, MaxSamplesLimit) {
    SProfilerConfig config;
    config.maxSamplesPerMetric = 5;

    m_profiler = std::make_unique<CPerformanceProfiler>("TestProfile", config);
    m_profiler->Start();

    // Record 10 samples
    for (int i = 0; i < 10; ++i) {
        m_profiler->RecordDuration("LimitedOp", i * 10.0);
    }

    auto stats = m_profiler->GetStatistics("LimitedOp");

    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->sampleCount, 5);  // Should only keep last 5

    m_profiler->Stop();
}

//=============================================================================
// Clear Samples Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, ClearSamples) {
    m_profiler->Start();

    m_profiler->RecordDuration("Op1", 100.0);
    m_profiler->IncrementCallCount("Func1");

    auto stats1 = m_profiler->GetStatistics("Op1");
    ASSERT_TRUE(stats1.has_value());

    m_profiler->ClearSamples();

    auto stats2 = m_profiler->GetStatistics("Op1");
    EXPECT_FALSE(stats2.has_value());

    auto report = m_profiler->GenerateReport();
    EXPECT_TRUE(report.statistics.empty());
    EXPECT_TRUE(report.summary.empty());

    m_profiler->Stop();
}

//=============================================================================
// System Metrics Tests
//=============================================================================

TEST_F(PerformanceProfilerTest, GetMemoryUsage) {
    TUInt64 memory = CPerformanceProfiler::GetCurrentMemoryUsage();

    // Should return non-zero on most systems
    // Can't assert specific value, but should be reasonable
    EXPECT_GT(memory, 0);
    EXPECT_LT(memory, static_cast<TUInt64>(100) * 1024 * 1024 * 1024);  // Less than 100GB
}

// Note: main() is provided by gtest_main library
