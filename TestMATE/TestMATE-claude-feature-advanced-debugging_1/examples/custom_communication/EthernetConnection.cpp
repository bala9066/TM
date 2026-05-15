/**************************************************************************
 * File Name: EthernetConnection.cpp
 * Description: Implementation of Ethernet connection for DUT communication
 * Author: TestMATE Development Team
 **************************************************************************/

#include "EthernetConnection.h"
#include "utils/LogManager.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <format>

namespace TestMATE {

CEthernetConnection::CEthernetConnection()
    : m_socket(-1)
    , m_bConnected(false)
    , m_protocol(EProtocol::kTCP)
    , m_timeoutMs(5000) {
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
}

CEthernetConnection::~CEthernetConnection() {
    if (m_bConnected) {
        Disconnect();
    }
}

CResult CEthernetConnection::Connect(const TString& in_strAddress) {
    // Parse address format: "tcp://192.168.1.100:5000" or "udp://192.168.1.100:5000"
    if (in_strAddress.find("tcp://") == 0) {
        size_t colonPos = in_strAddress.find(':', 6);
        if (colonPos == TString::npos) {
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid TCP address format");
        }
        TString ip = in_strAddress.substr(6, colonPos - 6);
        TUInt16 port = static_cast<TUInt16>(std::stoi(in_strAddress.substr(colonPos + 1)));
        return ConnectTCP(ip, port);
    } else if (in_strAddress.find("udp://") == 0) {
        size_t colonPos = in_strAddress.find(':', 6);
        if (colonPos == TString::npos) {
            return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Invalid UDP address format");
        }
        TString ip = in_strAddress.substr(6, colonPos - 6);
        TUInt16 port = static_cast<TUInt16>(std::stoi(in_strAddress.substr(colonPos + 1)));
        return ConnectUDP(ip, port);
    }
    return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Address must start with tcp:// or udp://");
}

CResult CEthernetConnection::ConnectTCP(const TString& in_ipAddress, TUInt16 in_port) {
    LOG_INFO("EthernetConnection", "Connecting to TCP {}:{}", in_ipAddress, in_port);

    // Create TCP socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Failed to create TCP socket");
    }

    m_protocol = EProtocol::kTCP;

    // Configure server address
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(in_port);

    if (inet_pton(AF_INET, in_ipAddress.c_str(), &m_serverAddr.sin_addr) <= 0) {
        close(m_socket);
        m_socket = -1;
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, std::format("Invalid IP address: {}", in_ipAddress));
    }

    // Connect to DUT
    if (connect(m_socket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        close(m_socket);
        m_socket = -1;
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, std::format("Connection refused by {}:{}", in_ipAddress, in_port));
    }

    // Set default timeout
    SetTimeout(m_timeoutMs);

    m_bConnected = true;
    LOG_INFO("EthernetConnection", "Connected successfully to {}:{}", in_ipAddress, in_port);
    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::ConnectUDP(const TString& in_ipAddress, TUInt16 in_port) {
    LOG_INFO("EthernetConnection", "Connecting to UDP {}:{}", in_ipAddress, in_port);

    // Create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Failed to create UDP socket");
    }

    m_protocol = EProtocol::kUDP;

    // Configure server address
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(in_port);

    if (inet_pton(AF_INET, in_ipAddress.c_str(), &m_serverAddr.sin_addr) <= 0) {
        close(m_socket);
        m_socket = -1;
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, std::format("Invalid IP address: {}", in_ipAddress));
    }

    // Set default timeout
    SetTimeout(m_timeoutMs);

    m_bConnected = true;
    LOG_INFO("EthernetConnection", "UDP endpoint configured for {}:{}", in_ipAddress, in_port);
    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::Disconnect() {
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
        LOG_INFO("EthernetConnection", "Disconnected");
    }
    m_bConnected = false;
    return TESTMATE_SUCCESS();
}

bool CEthernetConnection::IsConnected() const {
    return m_bConnected;
}

CResult CEthernetConnection::Send(const TVector<TUInt8>& in_data) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    ssize_t bytesSent;
    if (m_protocol == EProtocol::kTCP) {
        bytesSent = send(m_socket, in_data.data(), in_data.size(), 0);
    } else {
        bytesSent = sendto(m_socket, in_data.data(), in_data.size(), 0,
                          (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    }

    if (bytesSent < 0) {
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, "Send failed");
    }

    if (static_cast<size_t>(bytesSent) != in_data.size()) {
        return TESTMATE_FAILURE(EErrorCode::kSendFailed,
                                std::format("Incomplete send: {} of {} bytes", bytesSent, in_data.size()));
    }

    LOG_DEBUG("EthernetConnection", "Sent {} bytes", bytesSent);
    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::Receive(TVector<TUInt8>& out_data) {
    return ReceiveWithTimeout(out_data, m_timeoutMs);
}

CResult CEthernetConnection::ReceiveWithTimeout(TVector<TUInt8>& out_data, TUInt32 in_timeoutMs) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    // Set timeout
    struct timeval tv;
    tv.tv_sec = in_timeoutMs / 1000;
    tv.tv_usec = (in_timeoutMs % 1000) * 1000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Receive data
    TUInt8 buffer[4096];
    ssize_t bytesReceived;

    if (m_protocol == EProtocol::kTCP) {
        bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);
    } else {
        socklen_t addrLen = sizeof(m_serverAddr);
        bytesReceived = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                (struct sockaddr*)&m_serverAddr, &addrLen);
    }

    if (bytesReceived < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return TESTMATE_FAILURE(EErrorCode::kTimeout, std::format("Receive timeout ({}ms)", in_timeoutMs));
        }
        return TESTMATE_FAILURE(EErrorCode::kReceiveFailed, "Receive failed");
    }

    if (bytesReceived == 0 && m_protocol == EProtocol::kTCP) {
        m_bConnected = false;
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Connection closed by remote");
    }

    out_data.assign(buffer, buffer + bytesReceived);
    LOG_DEBUG("EthernetConnection", "Received {} bytes", bytesReceived);
    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::SendPacket(const TVector<TUInt8>& in_payload, TUInt8 in_packetType) {
    if (in_payload.size() > MAX_PACKET_SIZE) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                std::format("Payload too large: {} bytes (max {})", in_payload.size(), MAX_PACKET_SIZE));
    }

    TVector<TUInt8> packet;

    // Build packet header
    SPacketHeader header;
    header.startByte = PACKET_START_BYTE;
    header.packetType = in_packetType;
    header.length = static_cast<TUInt16>(in_payload.size());
    header.checksum = CalculateChecksum(in_payload);

    // Serialize header
    packet.push_back(header.startByte);
    packet.push_back(header.packetType);
    packet.push_back((header.length >> 8) & 0xFF);  // MSB
    packet.push_back(header.length & 0xFF);         // LSB
    packet.push_back(header.checksum);

    // Append payload
    packet.insert(packet.end(), in_payload.begin(), in_payload.end());

    LOG_DEBUG("EthernetConnection", "Sending packet: type=0x{:02X}, length={}, checksum=0x{:02X}",
              header.packetType, header.length, header.checksum);

    return Send(packet);
}

CResult CEthernetConnection::ReceivePacket(TVector<TUInt8>& out_payload, TUInt8& out_packetType) {
    TVector<TUInt8> rawData;
    auto result = ReceiveWithTimeout(rawData, m_timeoutMs);
    if (!result.IsSuccess()) {
        return result;
    }

    // Parse header (minimum 5 bytes)
    if (rawData.size() < 5) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Packet too short: {} bytes (minimum 5)", rawData.size()));
    }

    SPacketHeader header;
    header.startByte = rawData[0];
    header.packetType = rawData[1];
    header.length = (static_cast<TUInt16>(rawData[2]) << 8) | rawData[3];
    header.checksum = rawData[4];

    // Validate header
    if (!ValidateHeader(header)) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError, "Invalid packet header");
    }

    // Validate packet length
    if (rawData.size() < 5 + header.length) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Incomplete packet: got {} bytes, expected {}",
                                            rawData.size(), 5 + header.length));
    }

    // Extract payload
    out_payload.assign(rawData.begin() + 5, rawData.begin() + 5 + header.length);
    out_packetType = header.packetType;

    // Validate checksum
    TUInt8 calculatedChecksum = CalculateChecksum(out_payload);
    if (calculatedChecksum != header.checksum) {
        return TESTMATE_FAILURE(EErrorCode::kProtocolError,
                                std::format("Checksum mismatch: expected 0x{:02X}, got 0x{:02X}",
                                            header.checksum, calculatedChecksum));
    }

    LOG_DEBUG("EthernetConnection", "Received packet: type=0x{:02X}, length={}, checksum=0x{:02X}",
              header.packetType, header.length, header.checksum);

    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::SetTimeout(TUInt32 in_timeoutMs) {
    m_timeoutMs = in_timeoutMs;

    if (m_socket >= 0) {
        struct timeval tv;
        tv.tv_sec = in_timeoutMs / 1000;
        tv.tv_usec = (in_timeoutMs % 1000) * 1000;

        if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            return TESTMATE_FAILURE(EErrorCode::kConfigValidationFailed, "Failed to set receive timeout");
        }

        if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            return TESTMATE_FAILURE(EErrorCode::kConfigValidationFailed, "Failed to set send timeout");
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CEthernetConnection::SetKeepAlive(bool in_enable) {
    if (m_socket < 0) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    if (m_protocol != EProtocol::kTCP) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidOperation, "Keep-alive only supported for TCP");
    }

    int enable = in_enable ? 1 : 0;
    if (setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) < 0) {
        return TESTMATE_FAILURE(EErrorCode::kConfigValidationFailed, "Failed to set keep-alive");
    }

    return TESTMATE_SUCCESS();
}

TUInt8 CEthernetConnection::CalculateChecksum(const TVector<TUInt8>& in_data) {
    TUInt8 checksum = 0;
    for (TUInt8 byte : in_data) {
        checksum ^= byte;  // Simple XOR checksum
    }
    return checksum;
}

bool CEthernetConnection::ValidateHeader(const SPacketHeader& in_header) {
    if (in_header.startByte != PACKET_START_BYTE) {
        LOG_WARNING("EthernetConnection", "Invalid start byte: 0x{:02X} (expected 0x{:02X})",
                    in_header.startByte, PACKET_START_BYTE);
        return false;
    }

    if (in_header.length > MAX_PACKET_SIZE) {
        LOG_WARNING("EthernetConnection", "Payload too large: {} bytes (max {})",
                    in_header.length, MAX_PACKET_SIZE);
        return false;
    }

    return true;
}

} // namespace TestMATE
