/**
 * @file BreakpointTests.cpp
 * @brief Unit tests for CBreakpoint and CBreakpointManager classes
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/Breakpoint.h"
#include "testmate/debug/BreakpointManager.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace TestMATE;

// ==================== CBreakpoint Tests ====================

class BreakpointTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for each test
    }

    void TearDown() override {
        // Common cleanup
    }
};

// Test 1: Basic breakpoint creation
TEST_F(BreakpointTests, BasicBreakpointCreation) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    EXPECT_EQ(bp.GetId(), 1);
    EXPECT_EQ(bp.GetType(), EBreakpointType::kStepEntry);
    EXPECT_TRUE(bp.IsEnabled());
    EXPECT_EQ(bp.GetState(), EBreakpointState::kEnabled);
}

// Test 2: Enable/Disable functionality
TEST_F(BreakpointTests, EnableDisableBreakpoint) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    EXPECT_TRUE(bp.IsEnabled());

    bp.Disable();
    EXPECT_FALSE(bp.IsEnabled());
    EXPECT_EQ(bp.GetState(), EBreakpointState::kDisabled);

    bp.Enable();
    EXPECT_TRUE(bp.IsEnabled());
    EXPECT_EQ(bp.GetState(), EBreakpointState::kEnabled);
}

// Test 3: One-shot breakpoint
TEST_F(BreakpointTests, OneShotBreakpoint) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.SetOneShot();

    EXPECT_EQ(bp.GetState(), EBreakpointState::kOneShot);
    EXPECT_TRUE(bp.IsEnabled()); // One-shot is still enabled

    // First hit should trigger and disable
    bp.RecordHit();
    EXPECT_FALSE(bp.IsEnabled()); // Should be disabled after one hit
}

// Test 4: Location management
TEST_F(BreakpointTests, LocationManagement) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    bp.SetLocation("STEP-010");
    EXPECT_EQ(bp.GetLocation(), "STEP-010");

    bp.SetLocation("STEP-020");
    EXPECT_EQ(bp.GetLocation(), "STEP-020");
}

// Test 5: Hit count tracking
TEST_F(BreakpointTests, HitCountTracking) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    EXPECT_EQ(bp.GetCurrentHits(), 0);

    bp.RecordHit();
    EXPECT_EQ(bp.GetCurrentHits(), 1);

    bp.RecordHit();
    bp.RecordHit();
    EXPECT_EQ(bp.GetCurrentHits(), 3);

    bp.ResetHitCount();
    EXPECT_EQ(bp.GetCurrentHits(), 0);
}

// Test 6: Hit count breakpoint triggers after N hits
TEST_F(BreakpointTests, HitCountBreakpointTriggersAfterNHits) {
    CBreakpoint bp(1, EBreakpointType::kHitCount);
    bp.SetHitCount(3);

    EXPECT_EQ(bp.GetHitCountThreshold(), 3);

    bp.RecordHit();
    EXPECT_FALSE(bp.ShouldBreak()); // Hit 1 of 3

    bp.RecordHit();
    EXPECT_FALSE(bp.ShouldBreak()); // Hit 2 of 3

    bp.RecordHit();
    EXPECT_TRUE(bp.ShouldBreak());  // Hit 3 of 3 - should break

    bp.RecordHit();
    EXPECT_TRUE(bp.ShouldBreak());  // Hit 4 - should still break
}

// Test 7: Conditional breakpoint with simple condition
TEST_F(BreakpointTests, ConditionalBreakpointSimple) {
    CBreakpoint bp(1, EBreakpointType::kConditional);

    int testValue = 0;
    SBreakpointCondition condition;
    condition.expression = "testValue > 5";
    condition.evaluator = [&testValue]() { return testValue > 5; };

    bp.SetCondition(condition);

    testValue = 3;
    EXPECT_FALSE(bp.ShouldBreak()); // Condition not met

    testValue = 7;
    EXPECT_TRUE(bp.ShouldBreak());  // Condition met

    testValue = 5;
    EXPECT_FALSE(bp.ShouldBreak()); // Boundary: not met (> not >=)
}

// Test 8: Conditional breakpoint with complex condition
TEST_F(BreakpointTests, ConditionalBreakpointComplex) {
    CBreakpoint bp(1, EBreakpointType::kConditional);

    double voltage = 0.0;
    int iteration = 0;

    SBreakpointCondition condition;
    condition.expression = "voltage > 3.3 AND iteration > 10";
    condition.evaluator = [&voltage, &iteration]() {
        return (voltage > 3.3) && (iteration > 10);
    };

    bp.SetCondition(condition);

    // Neither condition met
    voltage = 2.0;
    iteration = 5;
    EXPECT_FALSE(bp.ShouldBreak());

    // Only voltage met
    voltage = 5.0;
    iteration = 5;
    EXPECT_FALSE(bp.ShouldBreak());

    // Only iteration met
    voltage = 2.0;
    iteration = 15;
    EXPECT_FALSE(bp.ShouldBreak());

    // Both conditions met
    voltage = 5.0;
    iteration = 15;
    EXPECT_TRUE(bp.ShouldBreak());
}

// Test 9: Disabled breakpoint never triggers
TEST_F(BreakpointTests, DisabledBreakpointNeverTriggers) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.Disable();

    EXPECT_FALSE(bp.ShouldBreak());

    // Even with hit count
    bp.RecordHit();
    bp.RecordHit();
    EXPECT_FALSE(bp.ShouldBreak());
}

// Test 10: Data watchpoint configuration
TEST_F(BreakpointTests, DataWatchpointConfiguration) {
    CBreakpoint bp(1, EBreakpointType::kDataAccess);

    SDataWatchpoint watchpoint;
    watchpoint.variableName = "voltage";
    watchpoint.breakOnRead = false;
    watchpoint.breakOnWrite = true;

    bp.SetDataWatchpoint(watchpoint);

    auto retrieved = bp.GetDataWatchpoint();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->variableName, "voltage");
    EXPECT_FALSE(retrieved->breakOnRead);
    EXPECT_TRUE(retrieved->breakOnWrite);
}

// Test 11: Statistics tracking
TEST_F(BreakpointTests, StatisticsTracking) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    auto stats = bp.GetStats();
    EXPECT_EQ(stats.totalHits, 0);

    bp.RecordHit();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bp.RecordHit();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bp.RecordHit();

    stats = bp.GetStats();
    EXPECT_EQ(stats.totalHits, 3);
    EXPECT_GT(stats.averageTimeBetweenHits, 0.0); // Should have non-zero average
}

// Test 12: Serialization
TEST_F(BreakpointTests, Serialization) {
    CBreakpoint bp(1, EBreakpointType::kConditional);
    bp.SetLocation("STEP-010");
    bp.SetHitCount(5);

    TString serialized = bp.ToString();

    // Verify JSON contains key information
    EXPECT_NE(serialized.find("\"id\":1"), TString::npos);
    EXPECT_NE(serialized.find("\"location\":\"STEP-010\""), TString::npos);
    EXPECT_NE(serialized.find("\"Conditional\""), TString::npos);
}

// ==================== CBreakpointManager Tests ====================

class BreakpointManagerTests : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<CBreakpointManager>();
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<CBreakpointManager> manager;
};

// Test 13: Add and retrieve breakpoint
TEST_F(BreakpointManagerTests, AddAndRetrieveBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    EXPECT_GT(bpId, 0);

    auto bp = manager->GetBreakpoint(bpId);
    ASSERT_NE(bp, nullptr);
    EXPECT_EQ(bp->GetId(), bpId);
    EXPECT_EQ(bp->GetLocation(), "STEP-010");
    EXPECT_EQ(bp->GetType(), EBreakpointType::kStepEntry);
}

// Test 14: Remove breakpoint
TEST_F(BreakpointManagerTests, RemoveBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    EXPECT_GT(bpId, 0);

    EXPECT_TRUE(manager->RemoveBreakpoint(bpId));

    auto bp = manager->GetBreakpoint(bpId);
    EXPECT_EQ(bp, nullptr);
}

// Test 15: Get breakpoints at location
TEST_F(BreakpointManagerTests, GetBreakpointsAtLocation) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepExit, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-020");

    auto bps = manager->GetBreakpointsAt("STEP-010");
    EXPECT_EQ(bps.size(), 2);

    bps = manager->GetBreakpointsAt("STEP-020");
    EXPECT_EQ(bps.size(), 1);

    bps = manager->GetBreakpointsAt("STEP-030");
    EXPECT_EQ(bps.size(), 0);
}

// Test 16: Remove breakpoints at location
TEST_F(BreakpointManagerTests, RemoveBreakpointsAtLocation) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepExit, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-020");

    auto removed = manager->RemoveBreakpointsAt("STEP-010");
    EXPECT_EQ(removed, 2);

    auto bps = manager->GetBreakpointsAt("STEP-010");
    EXPECT_EQ(bps.size(), 0);

    bps = manager->GetBreakpointsAt("STEP-020");
    EXPECT_EQ(bps.size(), 1); // Should still exist
}

// Test 17: Clear all breakpoints
TEST_F(BreakpointManagerTests, ClearAllBreakpoints) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-020");
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-030");

    EXPECT_EQ(manager->GetBreakpointCount(), 3);

    manager->ClearAllBreakpoints();
    EXPECT_EQ(manager->GetBreakpointCount(), 0);
}

// Test 18: Enable/disable breakpoint through manager
TEST_F(BreakpointManagerTests, EnableDisableBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");

    auto bp = manager->GetBreakpoint(bpId);
    ASSERT_NE(bp, nullptr);
    EXPECT_TRUE(bp->IsEnabled());

    manager->DisableBreakpoint(bpId);
    EXPECT_FALSE(bp->IsEnabled());

    manager->EnableBreakpoint(bpId);
    EXPECT_TRUE(bp->IsEnabled());
}

// Test 19: Enable/disable all breakpoints
TEST_F(BreakpointManagerTests, EnableDisableAllBreakpoints) {
    auto bp1Id = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    auto bp2Id = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-020");

    manager->DisableAllBreakpoints();

    EXPECT_FALSE(manager->GetBreakpoint(bp1Id)->IsEnabled());
    EXPECT_FALSE(manager->GetBreakpoint(bp2Id)->IsEnabled());

    manager->EnableAllBreakpoints();

    EXPECT_TRUE(manager->GetBreakpoint(bp1Id)->IsEnabled());
    EXPECT_TRUE(manager->GetBreakpoint(bp2Id)->IsEnabled());
}

// Test 20: Set conditional breakpoint through manager
TEST_F(BreakpointManagerTests, SetConditionalBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kConditional, "STEP-010");

    int testValue = 0;
    SBreakpointCondition condition;
    condition.expression = "testValue > 10";
    condition.evaluator = [&testValue]() { return testValue > 10; };

    EXPECT_TRUE(manager->SetBreakpointCondition(bpId, condition));

    auto bp = manager->GetBreakpoint(bpId);
    ASSERT_NE(bp, nullptr);

    testValue = 5;
    EXPECT_FALSE(bp->ShouldBreak());

    testValue = 15;
    EXPECT_TRUE(bp->ShouldBreak());
}

// Test 21: Set hit count breakpoint through manager
TEST_F(BreakpointManagerTests, SetHitCountBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kHitCount, "STEP-010");

    EXPECT_TRUE(manager->SetBreakpointHitCount(bpId, 5));

    auto bp = manager->GetBreakpoint(bpId);
    ASSERT_NE(bp, nullptr);
    EXPECT_EQ(bp->GetHitCountThreshold(), 5);
}

// Test 22: Check breakpoint triggers correctly
TEST_F(BreakpointManagerTests, CheckBreakpointTriggers) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");

    // Should trigger at STEP-010
    auto triggeredId = manager->CheckBreakpoint("STEP-010");
    EXPECT_GT(triggeredId, 0);

    // Should not trigger at different step
    triggeredId = manager->CheckBreakpoint("STEP-020");
    EXPECT_EQ(triggeredId, 0);
}

// Test 23: Check conditional breakpoint
TEST_F(BreakpointManagerTests, CheckConditionalBreakpoint) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kConditional, "STEP-010");

    double voltage = 0.0;
    SBreakpointCondition condition;
    condition.expression = "voltage > 5.0";
    condition.evaluator = [&voltage]() { return voltage > 5.0; };

    manager->SetBreakpointCondition(bpId, condition);

    // Should not trigger when condition is false
    voltage = 3.0;
    auto triggeredId = manager->CheckBreakpoint("STEP-010");
    EXPECT_EQ(triggeredId, 0);

    // Should trigger when condition is true
    voltage = 7.0;
    triggeredId = manager->CheckBreakpoint("STEP-010");
    EXPECT_GT(triggeredId, 0);
}

// Test 24: Statistics
TEST_F(BreakpointManagerTests, Statistics) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-020");
    auto bp3Id = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-030");

    manager->DisableBreakpoint(bp3Id);

    auto stats = manager->GetStats();
    EXPECT_EQ(stats.totalBreakpoints, 3);
    EXPECT_EQ(stats.enabledBreakpoints, 2);
    EXPECT_EQ(stats.disabledBreakpoints, 1);
}

// Test 25: Generate report
TEST_F(BreakpointManagerTests, GenerateReport) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kConditional, "STEP-020");

    auto report = manager->GenerateReport();

    EXPECT_NE(report.find("Total Breakpoints: 2"), TString::npos);
    EXPECT_NE(report.find("STEP-010"), TString::npos);
    EXPECT_NE(report.find("STEP-020"), TString::npos);
}

// Test 26: List breakpoints
TEST_F(BreakpointManagerTests, ListBreakpoints) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepExit, "STEP-020");

    auto list = manager->ListBreakpoints();
    EXPECT_EQ(list.size(), 2);
}

// Test 27: Data breakpoint checking
TEST_F(BreakpointManagerTests, DataBreakpointChecking) {
    auto bpId = manager->AddBreakpoint(EBreakpointType::kDataAccess, "STEP-010");

    SDataWatchpoint watchpoint;
    watchpoint.variableName = "voltage";
    watchpoint.breakOnRead = false;
    watchpoint.breakOnWrite = true;

    manager->SetBreakpointWatchpoint(bpId, watchpoint);

    // Should not trigger on read
    auto triggeredId = manager->CheckDataBreakpoint("voltage", false);
    EXPECT_EQ(triggeredId, 0);

    // Should trigger on write
    triggeredId = manager->CheckDataBreakpoint("voltage", true);
    EXPECT_GT(triggeredId, 0);
}

// Test 28: Cannot set wrong condition type
TEST_F(BreakpointManagerTests, CannotSetWrongConditionType) {
    // Create a step entry breakpoint (not conditional)
    auto bpId = manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");

    SBreakpointCondition condition;
    condition.expression = "test";
    condition.evaluator = []() { return true; };

    // Should fail to set condition on non-conditional breakpoint
    EXPECT_FALSE(manager->SetBreakpointCondition(bpId, condition));
}

// Test 29: Save to file
TEST_F(BreakpointManagerTests, SaveToFile) {
    manager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
    manager->AddBreakpoint(EBreakpointType::kStepExit, "STEP-020");

    bool saved = manager->SaveToFile("/tmp/test_breakpoints.json");
    EXPECT_TRUE(saved);
}

// Test 30: Thread safety - concurrent access
TEST_F(BreakpointManagerTests, ThreadSafetyConcurrentAccess) {
    const int numThreads = 10;
    const int opsPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([this, opsPerThread, t]() {
            for (int i = 0; i < opsPerThread; i++) {
                auto location = "STEP-" + std::to_string(t * 1000 + i);
                auto bpId = manager->AddBreakpoint(EBreakpointType::kStepEntry, location);

                if (i % 2 == 0) {
                    manager->DisableBreakpoint(bpId);
                }

                auto result = manager->CheckBreakpoint(location);
                (void)result; // Intentionally unused in thread safety test
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all breakpoints were added
    EXPECT_EQ(manager->GetBreakpointCount(), numThreads * opsPerThread);
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
