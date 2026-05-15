/**************************************************************************
 * File Name: InstrumentTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for Instrument and Communication layers
 **************************************************************************/

#include "core/instruments/IInstrument.h"
#include "core/instruments/InstrumentManager.h"
#include "core/communication/IConnection.h"
#include <gtest/gtest.h>

using namespace TestMATE;

//=============================================================================
// SimulatedConnection Tests
//=============================================================================

TEST(SimulatedConnectionTest, Open_Succeeds) {
    CSimulatedConnection conn;
    EXPECT_FALSE(conn.IsOpen());

    auto result = conn.Open();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(conn.IsOpen());
}

TEST(SimulatedConnectionTest, Close_Succeeds) {
    CSimulatedConnection conn;
    conn.Open();

    auto result = conn.Close();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(conn.IsOpen());
}

TEST(SimulatedConnectionTest, Write_StoresCommand) {
    CSimulatedConnection conn;
    conn.Open();

    conn.Write("TEST_CMD");
    EXPECT_EQ(conn.GetLastCommand(), "TEST_CMD");
}

TEST(SimulatedConnectionTest, SetResponse_ReturnsOnRead) {
    CSimulatedConnection conn;
    conn.Open();

    conn.SetResponse("expected_response");
    conn.Write("QUERY?");

    TString response;
    conn.Read(response);
    EXPECT_EQ(response, "expected_response");
}

TEST(SimulatedConnectionTest, IDN_ReturnsSimulatedResponse) {
    CSimulatedConnection conn;
    conn.Open();

    conn.Write("*IDN?");

    TString response;
    conn.Read(response);
    EXPECT_FALSE(response.empty());
    EXPECT_NE(response.find("SIMULATED"), TString::npos);
}

//=============================================================================
// SimulatedInstrument Tests
//=============================================================================

TEST(SimulatedInstrumentTest, Constructor_SetsProperties) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    EXPECT_EQ(inst.GetId(), "DMM1");
    EXPECT_EQ(inst.GetType(), EInstrumentType::kMultimeter);
    EXPECT_FALSE(inst.IsConnected());
}

TEST(SimulatedInstrumentTest, Connect_SetsConnectedState) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    SConnectionConfig config;
    config.type = EConnectionType::kSimulated;

    auto result = inst.Connect(config);
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(inst.IsConnected());
}

TEST(SimulatedInstrumentTest, Disconnect_SetsDisconnectedState) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    SConnectionConfig config;
    inst.Connect(config);

    auto result = inst.Disconnect();
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(inst.IsConnected());
}

TEST(SimulatedInstrumentTest, Query_IDN_ReturnsInfo) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    SConnectionConfig config;
    inst.Connect(config);

    TString response;
    auto result = inst.Query("*IDN?", response);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(response.empty());
}

TEST(SimulatedInstrumentTest, Query_WhenNotConnected_Fails) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    TString response;
    auto result = inst.Query("*IDN?", response);

    EXPECT_TRUE(result.IsFailure());
}

TEST(SimulatedInstrumentTest, SetResponse_CustomResponse) {
    CSimulatedInstrument inst("DMM1", EInstrumentType::kMultimeter);

    SConnectionConfig config;
    inst.Connect(config);

    inst.SetResponse("MEAS:VOLT?", "3.300");

    TString response;
    inst.Query("MEAS:VOLT?", response);

    EXPECT_EQ(response, "3.300");
}

//=============================================================================
// InstrumentManager Tests
//=============================================================================

class InstrumentManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        CInstrumentManager::GetInstance().UnregisterAll();
    }

    void TearDown() override {
        CInstrumentManager::GetInstance().UnregisterAll();
    }
};

TEST_F(InstrumentManagerTest, GetInstance_ReturnsSameInstance) {
    auto& inst1 = CInstrumentManager::GetInstance();
    auto& inst2 = CInstrumentManager::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

TEST_F(InstrumentManagerTest, RegisterInstrument_Succeeds) {
    auto pInst = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);

    auto result = CInstrumentManager::GetInstance().RegisterInstrument(pInst);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(CInstrumentManager::GetInstance().GetInstrumentCount(), 1u);
}

TEST_F(InstrumentManagerTest, RegisterDuplicate_Fails) {
    auto pInst1 = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);
    auto pInst2 = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);

    CInstrumentManager::GetInstance().RegisterInstrument(pInst1);
    auto result = CInstrumentManager::GetInstance().RegisterInstrument(pInst2);

    EXPECT_TRUE(result.IsFailure());
}

TEST_F(InstrumentManagerTest, GetInstrument_ReturnsRegistered) {
    auto pInst = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);
    CInstrumentManager::GetInstance().RegisterInstrument(pInst);

    auto pRetrieved = CInstrumentManager::GetInstance().GetInstrument("DMM1");

    ASSERT_NE(pRetrieved, nullptr);
    EXPECT_EQ(pRetrieved->GetId(), "DMM1");
}

TEST_F(InstrumentManagerTest, GetInstrument_Unknown_ReturnsNull) {
    auto pInst = CInstrumentManager::GetInstance().GetInstrument("UNKNOWN");
    EXPECT_EQ(pInst, nullptr);
}

TEST_F(InstrumentManagerTest, UnregisterInstrument_Removes) {
    auto pInst = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);
    CInstrumentManager::GetInstance().RegisterInstrument(pInst);

    auto result = CInstrumentManager::GetInstance().UnregisterInstrument("DMM1");

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(CInstrumentManager::GetInstance().GetInstrumentCount(), 0u);
}

TEST_F(InstrumentManagerTest, CreateSimulatedInstrument_ReturnsValid) {
    auto pInst = CInstrumentManager::GetInstance().CreateSimulatedInstrument(
        "SIM1", EInstrumentType::kOscilloscope);

    ASSERT_NE(pInst, nullptr);
    EXPECT_EQ(pInst->GetId(), "SIM1");
    EXPECT_EQ(pInst->GetType(), EInstrumentType::kOscilloscope);
}

TEST_F(InstrumentManagerTest, ConnectInstrument_ConnectsRegistered) {
    auto pInst = std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter);
    CInstrumentManager::GetInstance().RegisterInstrument(pInst);

    SConnectionConfig config;
    auto result = CInstrumentManager::GetInstance().ConnectInstrument("DMM1", config);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_TRUE(CInstrumentManager::GetInstance().IsInstrumentConnected("DMM1"));
}

TEST_F(InstrumentManagerTest, GetInstrumentIds_ReturnsAll) {
    CInstrumentManager::GetInstance().RegisterInstrument(
        std::make_shared<CSimulatedInstrument>("DMM1", EInstrumentType::kMultimeter));
    CInstrumentManager::GetInstance().RegisterInstrument(
        std::make_shared<CSimulatedInstrument>("SCOPE1", EInstrumentType::kOscilloscope));

    auto ids = CInstrumentManager::GetInstance().GetInstrumentIds();

    EXPECT_EQ(ids.size(), 2u);
}

