/**************************************************************************
 * File Name: InstrumentPoolTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Unit tests for instrument resource pool
 **************************************************************************/

#include <gtest/gtest.h>
#include "testmate/resources/InstrumentPool.h"
#include "core/instruments/IInstrument.h"
#include <thread>
#include <chrono>

using namespace TestMATE;

// Concrete mock instrument for testing - implements all pure virtual methods
class MockInstrument : public IInstrument {
public:
    MockInstrument() = default;

    // Identification
    SInstrumentInfo GetInfo() const override { return m_info; }
    TString GetId() const override { return "MOCK-001"; }
    EInstrumentType GetType() const override { return EInstrumentType::kCustom; }

    // Connection
    CResult Connect(const SConnectionConfig&) override { return CResult::Success(); }
    CResult Disconnect() override { return CResult::Success(); }
    bool IsConnected() const override { return true; }
    EInstrumentState GetState() const override { return EInstrumentState::kConnected; }

    // Communication
    CResult Write(const TString&) override { return CResult::Success(); }
    CResult Read(TString&, TInt64 = 0) override { return CResult::Success(); }
    CResult Query(const TString&, TString&, TInt64 = 0) override { return CResult::Success(); }

    // Standard Commands
    CResult Reset() override { return CResult::Success(); }
    CResult Clear() override { return CResult::Success(); }
    CResult Identify(TString& out) override { out = "Mock"; return CResult::Success(); }
    CResult SelfTest(TInt32& result) override { result = 0; return CResult::Success(); }

    // Error Handling
    TString GetLastError() const override { return ""; }
    CResult GetSystemError(TInt32& code, TString& msg) override {
        code = 0; msg = ""; return CResult::Success();
    }

private:
    SInstrumentInfo m_info;
};

//=============================================================================
// Test Fixture
//=============================================================================

class InstrumentPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get fresh pool instance
        auto& pool = CInstrumentPool::GetInstance();
        pool.ReleaseAll();
        pool.Clear();  // Clear all instruments for test isolation

        // Create mock instruments
        m_dmm1 = std::make_unique<MockInstrument>();
        m_dmm2 = std::make_unique<MockInstrument>();
        m_scope1 = std::make_unique<MockInstrument>();
        m_psu1 = std::make_unique<MockInstrument>();
    }

    void TearDown() override {
        // Clean up pool
        auto& pool = CInstrumentPool::GetInstance();
        pool.ReleaseAll();
        pool.Clear();
    }

    std::unique_ptr<MockInstrument> m_dmm1;
    std::unique_ptr<MockInstrument> m_dmm2;
    std::unique_ptr<MockInstrument> m_scope1;
    std::unique_ptr<MockInstrument> m_psu1;
};

//=============================================================================
// Registration Tests
//=============================================================================

TEST_F(InstrumentPoolTest, RegisterInstrument_ValidInstrument_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    auto result = pool.RegisterInstrument(
        m_dmm1.get(),
        "DMM-1",
        "DMM",
        "Keysight 34461A",
        "SN12345",
        "Lab-A");

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(pool.IsRegistered("DMM-1"));
}

TEST_F(InstrumentPoolTest, GetStatistics_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");
    pool.RegisterInstrument(m_dmm2.get(), "DMM-2", "DMM", "Model2", "SN2");

    auto stats = pool.GetStatistics();
    EXPECT_EQ(stats.totalInstruments, 2);
    EXPECT_EQ(stats.availableInstruments, 2);
}

TEST_F(InstrumentPoolTest, ReserveAndRelease_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");

    auto* instrument = pool.ReserveInstrument("DMM", "user1", 1000);
    ASSERT_NE(instrument, nullptr);
    EXPECT_EQ(instrument, m_dmm1.get());
    EXPECT_FALSE(pool.IsAvailable("DMM-1"));

    auto result = pool.ReleaseInstrument(instrument);
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(pool.IsAvailable("DMM-1"));
}

TEST_F(InstrumentPoolTest, GetAvailableInstruments_FilterByType_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");
    pool.RegisterInstrument(m_dmm2.get(), "DMM-2", "DMM", "Model2", "SN2");
    pool.RegisterInstrument(m_scope1.get(), "SCOPE-1", "Scope", "Model3", "SN3");

    auto dmmList = pool.GetAvailableInstruments("DMM");
    EXPECT_EQ(dmmList.size(), 2);

    auto all = pool.GetAvailableInstruments();
    EXPECT_EQ(all.size(), 3);
}

TEST_F(InstrumentPoolTest, ReserveInstrumentById_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");
    pool.RegisterInstrument(m_dmm2.get(), "DMM-2", "DMM", "Model2", "SN2");

    auto* instrument = pool.ReserveInstrumentById("DMM-2", "user1", 1000);

    ASSERT_NE(instrument, nullptr);
    EXPECT_EQ(instrument, m_dmm2.get());
    EXPECT_FALSE(pool.IsAvailable("DMM-2"));
    EXPECT_TRUE(pool.IsAvailable("DMM-1"));
}

TEST_F(InstrumentPoolTest, ReleaseInstrumentById_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");
    pool.ReserveInstrument("DMM", "user1", 1000);

    auto result = pool.ReleaseInstrumentById("DMM-1");
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(pool.IsAvailable("DMM-1"));
}

TEST_F(InstrumentPoolTest, MultipleReservations_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");
    pool.RegisterInstrument(m_dmm2.get(), "DMM-2", "DMM", "Model2", "SN2");

    auto* inst1 = pool.ReserveInstrument("DMM", "user1", 1000);
    auto* inst2 = pool.ReserveInstrument("DMM", "user2", 1000);

    EXPECT_NE(inst1, nullptr);
    EXPECT_NE(inst2, nullptr);
    EXPECT_NE(inst1, inst2);
}

TEST_F(InstrumentPoolTest, SetInstrumentState_Success) {
    auto& pool = CInstrumentPool::GetInstance();

    pool.RegisterInstrument(m_dmm1.get(), "DMM-1", "DMM", "Model1", "SN1");

    auto result = pool.SetInstrumentState("DMM-1", EPoolInstrumentState::kMaintenance);
    EXPECT_TRUE(result.IsSuccess());

    auto info = pool.GetInstrumentInfo("DMM-1");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, EPoolInstrumentState::kMaintenance);
}
