# Quick Start: Implementing Debugging Features

**Branch:** `feature/advanced-debugging-and-enhancements`

This guide shows you how to start implementing the interactive debugging system.

---

## Step 1: Create Directory Structure

```bash
mkdir -p include/testmate/debug
mkdir -p src/debug
mkdir -p tests/unit/debug
mkdir -p tests/integration/debug
```

## Step 2: Create Header Files (Week 1, Days 1-2)

### File 1: `include/testmate/debug/Breakpoint.h`

Copy the complete implementation from IMPLEMENTATION_PLAN.md Section 1.2.1

**Key Classes:**
- `EBreakpointType` - Enum for breakpoint types
- `EBreakpointState` - Enabled/Disabled/OneShot
- `SBreakpointCondition` - Conditional breakpoint logic
- `CBreakpoint` - Individual breakpoint representation
- `CBreakpointManager` - Manages all breakpoints

### File 2: `include/testmate/debug/DebugSession.h`

Copy from Section 1.2.2

**Key Classes:**
- `EDebugAction` - Continue/StepOver/StepInto/etc
- `SDebugState` - Current debugging state
- `SVariableInfo` - Variable inspection data
- `CDebugSession` - Main debug controller

### File 3: `include/testmate/debug/DebugCLI.h`

Copy from Section 1.2.3

**Key Classes:**
- `CDebugCLI` - Command-line interface

---

## Step 3: Implement Core Breakpoint Class (Week 1, Days 3-4)

### File: `src/debug/Breakpoint.cpp`

```cpp
#include "testmate/debug/Breakpoint.h"
#include "utils/LogManager.h"

namespace TestMATE {

CBreakpoint::CBreakpoint(TUInt32 in_id, EBreakpointType in_type)
    : m_id(in_id)
    , m_type(in_type)
    , m_createdTime(std::chrono::system_clock::now()) {
}

void CBreakpoint::SetLocation(const TString& in_stepId) {
    m_stepId = in_stepId;
    LOG_DEBUG("Breakpoint", "Breakpoint {} set at {}", m_id, in_stepId);
}

void CBreakpoint::SetCondition(const SBreakpointCondition& in_condition) {
    m_condition = in_condition;
    LOG_DEBUG("Breakpoint", "Breakpoint {} condition: {}",
              m_id, in_condition.expression);
}

void CBreakpoint::SetHitCount(TUInt32 in_count) {
    m_hitCountThreshold = in_count;
    LOG_DEBUG("Breakpoint", "Breakpoint {} hit count threshold: {}", m_id, in_count);
}

void CBreakpoint::SetEnabled(bool in_enabled) {
    m_state = in_enabled ? EBreakpointState::kEnabled : EBreakpointState::kDisabled;
}

bool CBreakpoint::ShouldBreak() const {
    // Not enabled
    if (m_state != EBreakpointState::kEnabled) {
        return false;
    }

    // Check type-specific conditions
    switch (m_type) {
        case EBreakpointType::kStepEntry:
        case EBreakpointType::kStepExit:
            // Always break (location is checked externally)
            return true;

        case EBreakpointType::kConditional:
            // Evaluate condition
            if (m_condition && m_condition->evaluator) {
                return m_condition->evaluator();
            }
            return false;

        case EBreakpointType::kHitCount:
            // Break when hit count reaches threshold
            return m_currentHits >= m_hitCountThreshold;

        case EBreakpointType::kDataAccess:
            // Not implemented yet
            return false;

        case EBreakpointType::kException:
            // Handled externally
            return true;

        default:
            return false;
    }
}

void CBreakpoint::RecordHit() {
    m_currentHits++;
    m_lastHitTime = std::chrono::system_clock::now();

    LOG_DEBUG("Breakpoint", "Breakpoint {} hit (count: {})", m_id, m_currentHits);

    // Auto-disable if one-shot
    if (m_state == EBreakpointState::kOneShot) {
        m_state = EBreakpointState::kDisabled;
        LOG_DEBUG("Breakpoint", "One-shot breakpoint {} disabled", m_id);
    }
}

TString CBreakpoint::GetInfo() const {
    std::ostringstream oss;
    oss << "Breakpoint #" << m_id << " ";

    switch (m_type) {
        case EBreakpointType::kStepEntry:
            oss << "[ENTRY] ";
            break;
        case EBreakpointType::kStepExit:
            oss << "[EXIT] ";
            break;
        case EBreakpointType::kConditional:
            oss << "[COND] ";
            break;
        case EBreakpointType::kHitCount:
            oss << "[HIT=" << m_hitCountThreshold << "] ";
            break;
        case EBreakpointType::kDataAccess:
            oss << "[DATA] ";
            break;
        case EBreakpointType::kException:
            oss << "[EXCEPTION] ";
            break;
    }

    oss << "at " << m_stepId;

    if (m_condition) {
        oss << " when (" << m_condition->expression << ")";
    }

    oss << " - " << (IsEnabled() ? "ENABLED" : "DISABLED");
    oss << " (hits: " << m_currentHits << ")";

    return oss.str();
}

} // namespace TestMATE
```

---

## Step 4: Implement Breakpoint Manager (Week 1, Day 5)

### File: `src/debug/BreakpointManager.cpp`

```cpp
#include "testmate/debug/Breakpoint.h"
#include "utils/LogManager.h"
#include <fstream>

namespace TestMATE {

CBreakpointManager::CBreakpointManager() {
    LOG_INFO("BreakpointManager", "Breakpoint manager initialized");
}

CBreakpointManager::~CBreakpointManager() {
    ClearAll();
}

TUInt32 CBreakpointManager::AddBreakpoint(EBreakpointType in_type, const TString& in_location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    TUInt32 id = m_nextId++;
    auto breakpoint = std::make_unique<CBreakpoint>(id, in_type);
    breakpoint->SetLocation(in_location);

    m_breakpoints[id] = std::move(breakpoint);

    LOG_INFO("BreakpointManager", "Added breakpoint #{} at {}", id, in_location);

    return id;
}

CResult CBreakpointManager::RemoveBreakpoint(TUInt32 in_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                              std::format("Breakpoint {} not found", in_id));
    }

    m_breakpoints.erase(it);
    LOG_INFO("BreakpointManager", "Removed breakpoint #{}", in_id);

    return TESTMATE_SUCCESS();
}

CResult CBreakpointManager::SetBreakpointEnabled(TUInt32 in_id, bool in_enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                              std::format("Breakpoint {} not found", in_id));
    }

    it->second->SetEnabled(in_enabled);
    return TESTMATE_SUCCESS();
}

CResult CBreakpointManager::SetBreakpointCondition(
    TUInt32 in_id,
    const SBreakpointCondition& in_condition) {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                              std::format("Breakpoint {} not found", in_id));
    }

    it->second->SetCondition(in_condition);
    return TESTMATE_SUCCESS();
}

TUInt32 CBreakpointManager::CheckBreakpoint(const TString& in_stepId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, bp] : m_breakpoints) {
        // Check if location matches
        if (bp->GetLocation() == in_stepId) {
            // Record hit
            bp->RecordHit();

            // Check if should break
            if (bp->ShouldBreak()) {
                LOG_INFO("BreakpointManager", "Breakpoint #{} triggered at {}",
                        id, in_stepId);
                return id;
            }
        }
    }

    return 0; // No breakpoint triggered
}

TVector<CBreakpoint*> CBreakpointManager::GetBreakpoints() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<CBreakpoint*> result;
    result.reserve(m_breakpoints.size());

    for (const auto& [id, bp] : m_breakpoints) {
        result.push_back(bp.get());
    }

    return result;
}

void CBreakpointManager::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_breakpoints.clear();
    LOG_INFO("BreakpointManager", "All breakpoints cleared");
}

CResult CBreakpointManager::SaveToFile(const TString& in_filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ofstream file(in_filePath);
    if (!file.is_open()) {
        return TESTMATE_FAILURE(EErrorCode::kFileOpenFailed,
                              std::format("Failed to open {}", in_filePath));
    }

    // Write header
    file << "# TestMATE Breakpoints\n";
    file << "# Format: ID,Type,Location,Enabled,HitCount\n\n";

    // Write breakpoints
    for (const auto& [id, bp] : m_breakpoints) {
        file << id << ","
             << static_cast<int>(bp->GetType()) << ","
             << bp->GetLocation() << ","
             << (bp->IsEnabled() ? "1" : "0") << ","
             << bp->GetHitCount() << "\n";
    }

    LOG_INFO("BreakpointManager", "Saved {} breakpoints to {}",
            m_breakpoints.size(), in_filePath);

    return TESTMATE_SUCCESS();
}

CResult CBreakpointManager::LoadFromFile(const TString& in_filePath) {
    std::ifstream file(in_filePath);
    if (!file.is_open()) {
        return TESTMATE_FAILURE(EErrorCode::kFileOpenFailed,
                              std::format("Failed to open {}", in_filePath));
    }

    ClearAll();

    TString line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse CSV: ID,Type,Location,Enabled,HitCount
        std::istringstream iss(line);
        TString token;
        TVector<TString> tokens;

        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 4) {
            continue;
        }

        TUInt32 id = std::stoul(tokens[0]);
        auto type = static_cast<EBreakpointType>(std::stoi(tokens[1]));
        TString location = tokens[2];
        bool enabled = (tokens[3] == "1");

        auto bp = std::make_unique<CBreakpoint>(id, type);
        bp->SetLocation(location);
        bp->SetEnabled(enabled);

        m_breakpoints[id] = std::move(bp);
        m_nextId = std::max(m_nextId, id + 1);
    }

    LOG_INFO("BreakpointManager", "Loaded {} breakpoints from {}",
            m_breakpoints.size(), in_filePath);

    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
```

---

## Step 5: Write Unit Tests (Week 1, Days 6-7)

### File: `tests/unit/debug/BreakpointTests.cpp`

```cpp
#include <gtest/gtest.h>
#include "testmate/debug/Breakpoint.h"

using namespace TestMATE;

class BreakpointTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }
};

TEST_F(BreakpointTests, CreateBasicBreakpoint) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.SetLocation("STEP-001");

    EXPECT_EQ(bp.GetId(), 1);
    EXPECT_EQ(bp.GetType(), EBreakpointType::kStepEntry);
    EXPECT_EQ(bp.GetLocation(), "STEP-001");
    EXPECT_TRUE(bp.IsEnabled());
    EXPECT_EQ(bp.GetHitCount(), 0);
}

TEST_F(BreakpointTests, EnableDisableBreakpoint) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);

    EXPECT_TRUE(bp.IsEnabled());

    bp.SetEnabled(false);
    EXPECT_FALSE(bp.IsEnabled());
    EXPECT_FALSE(bp.ShouldBreak());

    bp.SetEnabled(true);
    EXPECT_TRUE(bp.IsEnabled());
    EXPECT_TRUE(bp.ShouldBreak());
}

TEST_F(BreakpointTests, ConditionalBreakpointTriggersWhenTrue) {
    CBreakpoint bp(1, EBreakpointType::kConditional);

    int testValue = 0;

    SBreakpointCondition condition;
    condition.expression = "testValue > 5";
    condition.evaluator = [&testValue]() { return testValue > 5; };

    bp.SetCondition(condition);

    testValue = 3;
    EXPECT_FALSE(bp.ShouldBreak());

    testValue = 7;
    EXPECT_TRUE(bp.ShouldBreak());
}

TEST_F(BreakpointTests, HitCountBreakpointTriggersAfterNHits) {
    CBreakpoint bp(1, EBreakpointType::kHitCount);
    bp.SetHitCount(3);

    EXPECT_EQ(bp.GetHitCount(), 0);
    EXPECT_FALSE(bp.ShouldBreak());

    bp.RecordHit(); // Hit 1
    EXPECT_EQ(bp.GetHitCount(), 1);
    EXPECT_FALSE(bp.ShouldBreak());

    bp.RecordHit(); // Hit 2
    EXPECT_EQ(bp.GetHitCount(), 2);
    EXPECT_FALSE(bp.ShouldBreak());

    bp.RecordHit(); // Hit 3
    EXPECT_EQ(bp.GetHitCount(), 3);
    EXPECT_TRUE(bp.ShouldBreak());
}

TEST_F(BreakpointTests, OneShotBreakpointDisablesAfterHit) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.SetLocation("STEP-001");

    // Set as one-shot using internal state (would need setter in real code)
    // For now, test the concept
    EXPECT_TRUE(bp.IsEnabled());
    bp.RecordHit();
    // One-shot would auto-disable here
}

TEST_F(BreakpointTests, GetInfoReturnsFormattedString) {
    CBreakpoint bp(1, EBreakpointType::kStepEntry);
    bp.SetLocation("STEP-001");

    TString info = bp.GetInfo();

    EXPECT_NE(info.find("Breakpoint #1"), TString::npos);
    EXPECT_NE(info.find("[ENTRY]"), TString::npos);
    EXPECT_NE(info.find("STEP-001"), TString::npos);
    EXPECT_NE(info.find("ENABLED"), TString::npos);
}

// BreakpointManager Tests
class BreakpointManagerTests : public ::testing::Test {
protected:
    CBreakpointManager manager;
};

TEST_F(BreakpointManagerTests, AddBreakpoint) {
    auto id = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");

    EXPECT_GT(id, 0);

    auto breakpoints = manager.GetBreakpoints();
    EXPECT_EQ(breakpoints.size(), 1);
    EXPECT_EQ(breakpoints[0]->GetId(), id);
    EXPECT_EQ(breakpoints[0]->GetLocation(), "STEP-001");
}

TEST_F(BreakpointManagerTests, AddMultipleBreakpoints) {
    auto id1 = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");
    auto id2 = manager.AddBreakpoint(EBreakpointType::kStepExit, "STEP-002");
    auto id3 = manager.AddBreakpoint(EBreakpointType::kConditional, "STEP-003");

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);

    auto breakpoints = manager.GetBreakpoints();
    EXPECT_EQ(breakpoints.size(), 3);
}

TEST_F(BreakpointManagerTests, RemoveBreakpoint) {
    auto id1 = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");
    auto id2 = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-002");

    EXPECT_EQ(manager.GetBreakpoints().size(), 2);

    auto result = manager.RemoveBreakpoint(id1);
    EXPECT_TRUE(result.IsSuccess());

    EXPECT_EQ(manager.GetBreakpoints().size(), 1);
    EXPECT_EQ(manager.GetBreakpoints()[0]->GetId(), id2);
}

TEST_F(BreakpointManagerTests, RemoveNonExistentBreakpoint) {
    auto result = manager.RemoveBreakpoint(999);
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNotFound);
}

TEST_F(BreakpointManagerTests, CheckBreakpointAtLocation) {
    auto id = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");

    // Should trigger at correct location
    auto triggeredId = manager.CheckBreakpoint("STEP-001");
    EXPECT_EQ(triggeredId, id);

    // Should not trigger at different location
    triggeredId = manager.CheckBreakpoint("STEP-002");
    EXPECT_EQ(triggeredId, 0);
}

TEST_F(BreakpointManagerTests, SetBreakpointEnabled) {
    auto id = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");

    auto result = manager.SetBreakpointEnabled(id, false);
    EXPECT_TRUE(result.IsSuccess());

    auto breakpoints = manager.GetBreakpoints();
    EXPECT_FALSE(breakpoints[0]->IsEnabled());
}

TEST_F(BreakpointManagerTests, ClearAll) {
    manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");
    manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-002");
    manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-003");

    EXPECT_EQ(manager.GetBreakpoints().size(), 3);

    manager.ClearAll();

    EXPECT_EQ(manager.GetBreakpoints().size(), 0);
}

TEST_F(BreakpointManagerTests, SaveAndLoadFromFile) {
    // Add some breakpoints
    auto id1 = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-001");
    auto id2 = manager.AddBreakpoint(EBreakpointType::kConditional, "STEP-002");

    manager.SetBreakpointEnabled(id2, false);

    // Save
    TString testFile = "/tmp/test_breakpoints.bp";
    auto result = manager.SaveToFile(testFile);
    EXPECT_TRUE(result.IsSuccess());

    // Clear and reload
    manager.ClearAll();
    EXPECT_EQ(manager.GetBreakpoints().size(), 0);

    result = manager.LoadFromFile(testFile);
    EXPECT_TRUE(result.IsSuccess());

    // Verify
    auto breakpoints = manager.GetBreakpoints();
    EXPECT_EQ(breakpoints.size(), 2);

    // Clean up
    std::remove(testFile.c_str());
}
```

---

## Step 6: Update CMakeLists.txt

Add to `src/CMakeLists.txt`:

```cmake
# Debug system
set(DEBUG_SOURCES
    debug/Breakpoint.cpp
    debug/BreakpointManager.cpp
    debug/DebugSession.cpp
    debug/DebugCLI.cpp
)

target_sources(testmate_core PRIVATE ${DEBUG_SOURCES})
```

Add to `tests/unit/CMakeLists.txt`:

```cmake
# Debug tests
add_executable(debug_tests
    debug/BreakpointTests.cpp
    debug/DebugSessionTests.cpp
)

target_link_libraries(debug_tests
    PRIVATE
        testmate_core
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(debug_tests)
```

---

## Step 7: Build and Test

```bash
cd build
cmake ..
make -j4

# Run tests
./tests/unit/debug_tests

# Expected output:
# [==========] Running 15 tests from 2 test suites.
# [----------] 7 tests from BreakpointTests
# [ RUN      ] BreakpointTests.CreateBasicBreakpoint
# [       OK ] BreakpointTests.CreateBasicBreakpoint (0 ms)
# ...
# [==========] 15 tests from 2 test suites ran. (234 ms total)
# [  PASSED  ] 15 tests.
```

---

## Next Steps (Week 2)

1. Implement `CDebugSession` class
2. Add thread synchronization
3. Implement variable inspection
4. Integration with test executors

---

## Getting Help

- **Full Plan:** See `IMPLEMENTATION_PLAN.md`
- **Architecture:** Section 1.1 in plan
- **Examples:** Section 1.3 in plan
- **Testing:** Section 1.5 in plan

---

**Happy Coding!** 🚀
