/**************************************************************************
 * File Name: CustomUARTProtocol.cpp
 * Description: Implementation of custom UART protocol wrapper
 * Author: TestMATE Development Team
 **************************************************************************/

#include "CustomUARTProtocol.h"
#include "utils/LogManager.h"
#include <format>

namespace TestMATE {

CCustomUARTProtocol::CCustomUARTProtocol(CSerialConnection* in_pConnection)
    : m_pConnection(in_pConnection)
    , m_retryCount(3)
    , m_defaultTimeoutMs(1000) {
}

CResult CCustomUARTProtocol::SendCommand(ECommand in_commandId, const TVector<TUInt8>& in_params) {
    if (!m_pConnection || !m_pConnection->IsOpen()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Serial connection not established");
    }

    if (in_params.size() > MAX_PAYLOAD_SIZE) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                std::format("Payload too large: {} bytes (max {})", in_params.size(), MAX_PAYLOAD_SIZE));
    }

    // Build packet
    SPacket packet;
    packet.header = PACKET_HEADER;
    packet.commandId = static_cast<TUInt8>(in_commandId);
    packet.length = static_cast<TUInt8>(in_params.size());
    packet.payload = in_params;

    // Calculate CRC over header + command + length + payload
    TVector<TUInt8> crcData = {packet.header, packet.commandId, packet.length};
    crcData.insert(crcData.end(), packet.payload.begin(), packet.payload.end());
    packet.crc = CalculateCRC8(crcData);

    // Serialize packet
    TVector<TUInt8> rawBytes;
    auto result = SerializePacket(packet, rawBytes);
    if (!result.IsSuccess()) {
        return result;
    }

    // Send via serial connection
    result = m_pConnection->Write(rawBytes);
    if (!result.IsSuccess()) {
        return result;
    }

    LOG_DEBUG("CustomUARTProtocol", "Sent command 0x{:02X} with {} byte payload",
              packet.commandId, packet.length);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::ReceiveResponse(TUInt8& out_status, TVector<TUInt8>& out_data, TUInt32 in_timeoutMs) {
    if (!m_pConnection || !m_pConnection->IsOpen()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Serial connection not established");
    }

    // Read minimum packet size (header + command + length + CRC = 4 bytes)
    TVector<TUInt8> rawBytes;
    auto result = m_pConnection->Read(rawBytes, in_timeoutMs);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse packet
    SPacket packet;
    result = ParsePacket(rawBytes, packet);
    if (!result.IsSuccess()) {
        return result;
    }

    out_status = packet.commandId;
    out_data = packet.payload;

    LOG_DEBUG("CustomUARTProtocol", "Received response 0x{:02X} with {} byte payload",
              packet.commandId, packet.length);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::ReadRegister(TUInt16 in_address, TUInt32& out_value) {
    // Build read register command parameters
    TVector<TUInt8> params = {
        static_cast<TUInt8>((in_address >> 8) & 0xFF),  // Address MSB
        static_cast<TUInt8>(in_address & 0xFF)          // Address LSB
    };

    // Send command with retry
    TVector<TUInt8> response;
    auto result = SendCommandWithRetry(ECommand::kReadRegister, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse 32-bit value from response (big-endian)
    if (response.size() < 4) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Invalid response size: {} bytes (expected 4)", response.size()));
    }

    out_value = (static_cast<TUInt32>(response[0]) << 24) |
                (static_cast<TUInt32>(response[1]) << 16) |
                (static_cast<TUInt32>(response[2]) << 8) |
                 static_cast<TUInt32>(response[3]);

    LOG_INFO("CustomUARTProtocol", "Read register 0x{:04X} = 0x{:08X}", in_address, out_value);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::WriteRegister(TUInt16 in_address, TUInt32 in_value) {
    // Build write register command parameters
    TVector<TUInt8> params = {
        static_cast<TUInt8>((in_address >> 8) & 0xFF),  // Address MSB
        static_cast<TUInt8>(in_address & 0xFF),         // Address LSB
        static_cast<TUInt8>((in_value >> 24) & 0xFF),   // Value byte 3 (MSB)
        static_cast<TUInt8>((in_value >> 16) & 0xFF),   // Value byte 2
        static_cast<TUInt8>((in_value >> 8) & 0xFF),    // Value byte 1
        static_cast<TUInt8>(in_value & 0xFF)            // Value byte 0 (LSB)
    };

    // Send command with retry
    TVector<TUInt8> response;
    auto result = SendCommandWithRetry(ECommand::kWriteRegister, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    LOG_INFO("CustomUARTProtocol", "Wrote register 0x{:04X} = 0x{:08X}", in_address, in_value);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::GetDeviceStatus(TUInt8& out_status) {
    TVector<TUInt8> params;  // No parameters
    TVector<TUInt8> response;

    auto result = SendCommandWithRetry(ECommand::kGetStatus, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    if (response.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError, "Empty status response");
    }

    out_status = response[0];
    LOG_INFO("CustomUARTProtocol", "Device status: 0x{:02X}", out_status);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::GetFirmwareVersion(TString& out_version) {
    TVector<TUInt8> params;  // No parameters
    TVector<TUInt8> response;

    auto result = SendCommandWithRetry(ECommand::kGetVersion, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    if (response.size() < 3) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Invalid version response size: {}", response.size()));
    }

    // Parse version (major.minor.patch)
    out_version = std::to_string(response[0]) + "." +
                  std::to_string(response[1]) + "." +
                  std::to_string(response[2]);

    LOG_INFO("CustomUARTProtocol", "Firmware version: {}", out_version);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::ResetDevice() {
    TVector<TUInt8> params;  // No parameters
    TVector<TUInt8> response;

    auto result = SendCommandWithRetry(ECommand::kReset, params, response);
    if (!result.IsSuccess()) {
        return result;
    }

    LOG_INFO("CustomUARTProtocol", "Device reset successful");

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::SendCommandWithRetry(ECommand in_commandId,
                                                    const TVector<TUInt8>& in_params,
                                                    TVector<TUInt8>& out_response) {
    CResult lastError;

    for (TUInt8 attempt = 0; attempt <= m_retryCount; ++attempt) {
        // Send command
        auto result = SendCommand(in_commandId, in_params);
        if (!result.IsSuccess()) {
            lastError = result;
            LOG_WARNING("CustomUARTProtocol", "Send failed (attempt {}/{}): {}",
                        attempt + 1, m_retryCount + 1, result.GetMessage());
            continue;
        }

        // Receive response
        TUInt8 responseCode;
        result = ReceiveResponse(responseCode, out_response, m_defaultTimeoutMs);
        if (!result.IsSuccess()) {
            lastError = result;
            LOG_WARNING("CustomUARTProtocol", "Receive failed (attempt {}/{}): {}",
                        attempt + 1, m_retryCount + 1, result.GetMessage());
            continue;
        }

        // Check response code
        if (responseCode == static_cast<TUInt8>(ECommand::kNack)) {
            lastError = TESTMATE_FAILURE(EErrorCode::kProtocolError, "Device responded with NACK");
            LOG_WARNING("CustomUARTProtocol", "Received NACK (attempt {}/{})",
                        attempt + 1, m_retryCount + 1);
            continue;
        }

        if (responseCode == static_cast<TUInt8>(ECommand::kError)) {
            TString errorMsg = out_response.empty() ? "Unknown error" :
                               TString(out_response.begin(), out_response.end());
            return TESTMATE_FAILURE(EErrorCode::kProtocolError, std::format("Device error: {}", errorMsg));
        }

        // Success
        return TESTMATE_SUCCESS();
    }

    // All retries exhausted
    return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                            std::format("Command failed after {} retries: {}",
                                        m_retryCount + 1, lastError.GetMessage()));
}

TUInt8 CCustomUARTProtocol::CalculateCRC8(const TVector<TUInt8>& in_data) {
    // CRC-8 with polynomial 0x07 (x^8 + x^2 + x + 1)
    TUInt8 crc = 0x00;

    for (TUInt8 byte : in_data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

CResult CCustomUARTProtocol::SerializePacket(const SPacket& in_packet, TVector<TUInt8>& out_bytes) {
    out_bytes.clear();
    out_bytes.reserve(4 + in_packet.payload.size());

    out_bytes.push_back(in_packet.header);
    out_bytes.push_back(in_packet.commandId);
    out_bytes.push_back(in_packet.length);
    out_bytes.insert(out_bytes.end(), in_packet.payload.begin(), in_packet.payload.end());
    out_bytes.push_back(in_packet.crc);

    return TESTMATE_SUCCESS();
}

CResult CCustomUARTProtocol::ParsePacket(const TVector<TUInt8>& in_bytes, SPacket& out_packet) {
    // Minimum packet: header + command + length + CRC = 4 bytes
    if (in_bytes.size() < 4) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Packet too short: {} bytes (minimum 4)", in_bytes.size()));
    }

    // Parse header
    out_packet.header = in_bytes[0];
    if (out_packet.header != PACKET_HEADER) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Invalid packet header: 0x{:02X} (expected 0x{:02X})",
                                            out_packet.header, PACKET_HEADER));
    }

    out_packet.commandId = in_bytes[1];
    out_packet.length = in_bytes[2];

    // Validate packet length
    size_t expectedSize = 4 + out_packet.length;  // header + cmd + len + payload + crc
    if (in_bytes.size() < expectedSize) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Incomplete packet: got {} bytes, expected {}",
                                            in_bytes.size(), expectedSize));
    }

    // Extract payload
    if (out_packet.length > 0) {
        out_packet.payload.assign(in_bytes.begin() + 3, in_bytes.begin() + 3 + out_packet.length);
    } else {
        out_packet.payload.clear();
    }

    // Extract CRC
    out_packet.crc = in_bytes[3 + out_packet.length];

    // Validate CRC
    TVector<TUInt8> crcData = {out_packet.header, out_packet.commandId, out_packet.length};
    crcData.insert(crcData.end(), out_packet.payload.begin(), out_packet.payload.end());
    TUInt8 calculatedCRC = CalculateCRC8(crcData);

    if (calculatedCRC != out_packet.crc) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("CRC mismatch: expected 0x{:02X}, got 0x{:02X}",
                                            out_packet.crc, calculatedCRC));
    }

    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
