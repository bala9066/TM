/**************************************************************************
 * File Name: EthernetConnection.h
 * Description: Ethernet TCP/UDP connection implementation for DUT communication
 * Author: TestMATE Development Team
 *
 * This class demonstrates how to implement custom Ethernet-based communication
 * with your DUT (Device Under Test). It supports both TCP and UDP protocols
 * and includes a custom packet framing example.
 *
 * Use Cases:
 * - RF boards with Ethernet interfaces
 * - Custom binary protocols over TCP/IP
 * - High-speed data transfer with DUT
 * - Remote DUT control and monitoring
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace TestMATE {

/**
 * @class CEthernetConnection
 * @brief Ethernet connection implementation for TCP/UDP communication
 *
 * Provides network connectivity to DUT with support for:
 * - TCP streaming connections
 * - UDP datagram communication
 * - Custom packet framing
 * - Configurable timeouts
 * - Checksum validation
 */
class CEthernetConnection {
public:
    /**
     * @brief Protocol type for Ethernet connection
     */
    enum class EProtocol {
        kTCP,   ///< TCP streaming protocol
        kUDP    ///< UDP datagram protocol
    };

    /**
     * @brief Constructor
     */
    CEthernetConnection();

    /**
     * @brief Destructor - automatically disconnects
     */
    ~CEthernetConnection();

    // Connection methods for custom DUT protocol
    CResult Connect(const TString& in_strAddress);
    CResult Disconnect();
    [[nodiscard]] bool IsConnected() const;
    CResult Send(const TVector<TUInt8>& in_data);
    CResult Receive(TVector<TUInt8>& out_data);

    /**
     * @brief Connect to DUT via TCP
     * @param in_ipAddress IP address (e.g., "192.168.1.100")
     * @param in_port Port number (e.g., 5000)
     * @return Success or error result
     */
    CResult ConnectTCP(const TString& in_ipAddress, TUInt16 in_port);

    /**
     * @brief Connect to DUT via UDP
     * @param in_ipAddress IP address
     * @param in_port Port number
     * @return Success or error result
     */
    CResult ConnectUDP(const TString& in_ipAddress, TUInt16 in_port);

    /**
     * @brief Receive data with timeout
     * @param out_data Received data buffer
     * @param in_timeoutMs Timeout in milliseconds
     * @return Success or error result
     */
    CResult ReceiveWithTimeout(TVector<TUInt8>& out_data, TUInt32 in_timeoutMs);

    /**
     * @brief Send custom packet with header/checksum
     * @param in_payload Payload data
     * @param in_packetType Packet type identifier
     * @return Success or error result
     */
    CResult SendPacket(const TVector<TUInt8>& in_payload, TUInt8 in_packetType);

    /**
     * @brief Receive custom packet with validation
     * @param out_payload Received payload data
     * @param out_packetType Received packet type
     * @return Success or error result
     */
    CResult ReceivePacket(TVector<TUInt8>& out_payload, TUInt8& out_packetType);

    /**
     * @brief Set socket timeout
     * @param in_timeoutMs Timeout in milliseconds
     * @return Success or error result
     */
    CResult SetTimeout(TUInt32 in_timeoutMs);

    /**
     * @brief Enable/disable TCP keep-alive
     * @param in_enable True to enable keep-alive
     * @return Success or error result
     */
    CResult SetKeepAlive(bool in_enable);

    /**
     * @brief Get current protocol type
     * @return Protocol type (TCP or UDP)
     */
    [[nodiscard]] EProtocol GetProtocol() const { return m_protocol; }

private:
    /**
     * @brief Custom packet header structure
     *
     * This is an example packet format. Modify this to match your DUT's protocol.
     *
     * Packet Format:
     * [StartByte][PacketType][Length MSB][Length LSB][Checksum][Payload...]
     */
    struct SPacketHeader {
        TUInt8 startByte;    ///< Start marker (0xAA)
        TUInt8 packetType;   ///< Command/response type
        TUInt16 length;      ///< Payload length in bytes
        TUInt8 checksum;     ///< XOR checksum of payload
    };

    /**
     * @brief Calculate XOR checksum
     * @param in_data Data to checksum
     * @return Calculated checksum byte
     */
    TUInt8 CalculateChecksum(const TVector<TUInt8>& in_data);

    /**
     * @brief Validate packet header
     * @param in_header Header to validate
     * @return True if valid
     */
    bool ValidateHeader(const SPacketHeader& in_header);

    int m_socket;                    ///< Socket file descriptor
    bool m_bConnected;               ///< Connection status
    EProtocol m_protocol;            ///< Current protocol (TCP/UDP)
    struct sockaddr_in m_serverAddr; ///< Server address structure
    TUInt32 m_timeoutMs;             ///< Default timeout in milliseconds

    static constexpr TUInt8 PACKET_START_BYTE = 0xAA;  ///< Packet start marker
    static constexpr TUInt32 MAX_PACKET_SIZE = 65535;  ///< Maximum packet size
};

} // namespace TestMATE
