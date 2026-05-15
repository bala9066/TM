/**************************************************************************
 * File Name: SerialConnectionTests.cpp
 * Description: Unit tests for SerialConnection component
 **************************************************************************/

#include <gtest/gtest.h>
#include "core/communication/SerialConnection.h"

namespace TestMATE {
namespace Tests {

class CSerialConnectionTests : public ::testing::Test {
protected:
    void SetUp() override {
        SSerialConfig config;
        config.portName = "/dev/ttyUSB0";
        config.baudRate = 9600;
        m_connection = std::make_unique<CSerialConnection>(config);
    }

    std::unique_ptr<CSerialConnection> m_connection;
};

TEST_F(CSerialConnectionTests, Constructor_Default_NotConnected) {
    EXPECT_FALSE(m_connection->IsOpen());
}

TEST_F(CSerialConnectionTests, GetAddress_ReturnsPortName) {
    EXPECT_EQ(m_connection->GetAddress(), "/dev/ttyUSB0");
}

TEST_F(CSerialConnectionTests, SetBaudRate_ValidRate_Success) {
    auto result = m_connection->SetBaudRate(115200);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSerialConnectionTests, SetBaudRate_CommonRates_Accepted) {
    std::vector<TUInt32> validRates = {9600, 19200, 38400, 57600, 115200};
    for (auto rate : validRates) {
        auto result = m_connection->SetBaudRate(rate);
        EXPECT_TRUE(result.IsSuccess()) << "Failed for rate: " << rate;
    }
}

// Config is passed in constructor, not settable afterward
TEST_F(CSerialConnectionTests, Config_SetViaConstructor_Stored) {
    SSerialConfig config;
    config.portName = "/dev/ttyUSB1";
    config.baudRate = 115200;
    config.dataBits = 8;
    config.parity = EParity::kNone;
    config.stopBits = EStopBits::kOne;

    auto conn = std::make_unique<CSerialConnection>(config);
    EXPECT_EQ(conn->GetAddress(), "/dev/ttyUSB1");
}

TEST_F(CSerialConnectionTests, Open_InvalidPort_Failure) {
    SSerialConfig config;
    config.portName = "/dev/nonexistent_port_12345";
    auto conn = std::make_unique<CSerialConnection>(config);
    auto result = conn->Open();
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSerialConnectionTests, Close_NotOpen_Success) {
    auto result = m_connection->Close();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CSerialConnectionTests, Write_NotConnected_Failure) {
    TString data = "test";
    auto result = m_connection->Write(data);
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(CSerialConnectionTests, Read_NotConnected_Failure) {
    TString data;
    auto result = m_connection->Read(data, 100);
    EXPECT_FALSE(result.IsSuccess());
}

// NOTE: GetType() method doesn't exist in IConnection interface
// TEST_F(CSerialConnectionTests, GetType_ReturnsSerial) {
//     EXPECT_EQ(m_connection->GetType(), EConnectionType::kSerial);
// }

// NOTE: Configure() doesn't exist - config is passed in constructor
// TEST_F(CSerialConnectionTests, Configure_FullConfig_Success) {
//     SSerialConfig config;
//     config.portName = "/dev/ttyUSB0";
//     config.baudRate = 115200;
//     config.dataBits = 8;
//     config.stopBits = EStopBits::kOne;
//     config.parity = EParity::kNone;
//
//     auto result = m_connection->Configure(config);
//     EXPECT_TRUE(result.IsSuccess());
//     EXPECT_EQ(m_connection->GetPortName(), "/dev/ttyUSB0");
//     EXPECT_EQ(m_connection->GetBaudRate(), 115200);
// }

// NOTE: SetFlowControl() doesn't exist - flowControl is in config
// TEST_F(CSerialConnectionTests, SetFlowControl_Hardware_Stored) {
//     auto result = m_connection->SetFlowControl(EFlowControl::kHardware);
//     EXPECT_TRUE(result.IsSuccess());
// }
//
// TEST_F(CSerialConnectionTests, SetFlowControl_Software_Stored) {
//     auto result = m_connection->SetFlowControl(EFlowControl::kSoftware);
//     EXPECT_TRUE(result.IsSuccess());
// }
//
// TEST_F(CSerialConnectionTests, SetFlowControl_None_Stored) {
//     auto result = m_connection->SetFlowControl(EFlowControl::kNone);
//     EXPECT_TRUE(result.IsSuccess());
// }

TEST_F(CSerialConnectionTests, GetLastError_NoOperation_Empty) {
    EXPECT_TRUE(m_connection->GetLastError().empty());
}

} // namespace Tests
} // namespace TestMATE
