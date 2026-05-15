/**************************************************************************
 * File Name: CustomUARTProtocol.h
 * Description: Custom UART protocol wrapper for DUT communication
 * Author: TestMATE Development Team
 *
 * This class demonstrates how to implement a custom packet-based protocol
 * over UART/serial communication. It wraps TestMATE's SerialConnection
 * with a custom framing, CRC validation, and command/response handling.
 *
 * Use Cases:
 * - RF boards with UART debug interfaces
 * - Custom binary protocols over RS-232/RS-485
 * - Embedded device communication
 * - Register read/write operations
 **************************************************************************/

#pragma once

#include "core/communication/SerialConnection.h"
#include "testmate/common/Types.h"
#include "testmate/common/Result.h"

namespace TestMATE {

/**
 * @class CCustomUARTProtocol
 * @brief Custom UART packet protocol implementation
 *
 * Provides a packet-based protocol over UART with:
 * - Custom packet framing (header + payload + CRC)
 * - Command/response pattern
 * - CRC-8 validation
 * - Register read/write abstraction
 * - Retry logic for robustness
 */
class CCustomUARTProtocol {
public:
    /**
     * @brief Command IDs for DUT operations
     *
     * Modify these to match your DUT's command set
     */
    enum class ECommand : TUInt8 {
        kReadRegister    = 0x01,  ///< Read register command
        kWriteRegister   = 0x02,  ///< Write register command
        kGetStatus       = 0x03,  ///< Get device status
        kReset           = 0x04,  ///< Reset device
        kGetVersion      = 0x05,  ///< Get firmware version
        kCalibrate       = 0x06,  ///< Trigger calibration
        kAck             = 0x80,  ///< Acknowledgment response
        kNack            = 0x81,  ///< Negative acknowledgment
        kError           = 0xFF   ///< Error response
    };

    /**
     * @brief Constructor
     * @param in_pConnection Serial connection instance (must be connected)
     */
    explicit CCustomUARTProtocol(CSerialConnection* in_pConnection);

    /**
     * @brief Destructor
     */
    ~CCustomUARTProtocol() = default;

    /**
     * @brief Send command packet to DUT
     * @param in_commandId Command identifier
     * @param in_params Command parameters
     * @return Success or error result
     */
    CResult SendCommand(ECommand in_commandId, const TVector<TUInt8>& in_params);

    /**
     * @brief Receive response packet from DUT
     * @param out_status Response status code
     * @param out_data Response data payload
     * @param in_timeoutMs Timeout in milliseconds
     * @return Success or error result
     */
    CResult ReceiveResponse(TUInt8& out_status, TVector<TUInt8>& out_data, TUInt32 in_timeoutMs = 1000);

    /**
     * @brief Read 32-bit register from DUT
     * @param in_address Register address (16-bit)
     * @param out_value Register value (32-bit)
     * @return Success or error result
     */
    CResult ReadRegister(TUInt16 in_address, TUInt32& out_value);

    /**
     * @brief Write 32-bit register to DUT
     * @param in_address Register address (16-bit)
     * @param in_value Register value (32-bit)
     * @return Success or error result
     */
    CResult WriteRegister(TUInt16 in_address, TUInt32 in_value);

    /**
     * @brief Get device status byte
     * @param out_status Status byte
     * @return Success or error result
     */
    CResult GetDeviceStatus(TUInt8& out_status);

    /**
     * @brief Get firmware version string
     * @param out_version Version string (e.g., "1.2.3")
     * @return Success or error result
     */
    CResult GetFirmwareVersion(TString& out_version);

    /**
     * @brief Reset device
     * @return Success or error result
     */
    CResult ResetDevice();

    /**
     * @brief Set number of command retries on failure
     * @param in_retries Number of retries (default 3)
     */
    void SetRetryCount(TUInt8 in_retries) { m_retryCount = in_retries; }

    /**
     * @brief Set default timeout for responses
     * @param in_timeoutMs Timeout in milliseconds
     */
    void SetDefaultTimeout(TUInt32 in_timeoutMs) { m_defaultTimeoutMs = in_timeoutMs; }

private:
    /**
     * @brief Custom packet structure
     *
     * Packet Format:
     * [Header][CommandID][Length][Payload...][CRC8]
     *
     * - Header: 0x55 (1 byte)
     * - CommandID: Command identifier (1 byte)
     * - Length: Payload length (1 byte, 0-255)
     * - Payload: Command parameters (0-255 bytes)
     * - CRC8: CRC-8 checksum (1 byte)
     */
    struct SPacket {
        TUInt8 header;           ///< Packet start marker (0x55)
        TUInt8 commandId;        ///< Command/response ID
        TUInt8 length;           ///< Payload length
        TVector<TUInt8> payload; ///< Packet payload
        TUInt8 crc;              ///< CRC-8 checksum
    };

    /**
     * @brief Calculate CRC-8 checksum
     * @param in_data Data to checksum
     * @return CRC-8 value
     */
    TUInt8 CalculateCRC8(const TVector<TUInt8>& in_data);

    /**
     * @brief Serialize packet to byte array
     * @param in_packet Packet structure
     * @param out_bytes Serialized bytes
     * @return Success or error result
     */
    CResult SerializePacket(const SPacket& in_packet, TVector<TUInt8>& out_bytes);

    /**
     * @brief Parse byte array into packet
     * @param in_bytes Raw bytes
     * @param out_packet Parsed packet
     * @return Success or error result
     */
    CResult ParsePacket(const TVector<TUInt8>& in_bytes, SPacket& out_packet);

    /**
     * @brief Send command with retry logic
     * @param in_commandId Command ID
     * @param in_params Parameters
     * @param out_response Response data
     * @return Success or error result
     */
    CResult SendCommandWithRetry(ECommand in_commandId, const TVector<TUInt8>& in_params,
                                  TVector<TUInt8>& out_response);

    CSerialConnection* m_pConnection;  ///< Serial connection instance
    TUInt8 m_retryCount;               ///< Number of retries on failure
    TUInt32 m_defaultTimeoutMs;        ///< Default timeout in milliseconds

    static constexpr TUInt8 PACKET_HEADER = 0x55;  ///< Packet header byte
    static constexpr TUInt8 MAX_PAYLOAD_SIZE = 255; ///< Maximum payload size
};

} // namespace TestMATE
