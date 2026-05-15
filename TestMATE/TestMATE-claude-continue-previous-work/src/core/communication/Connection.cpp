/**************************************************************************
 * File Name: Connection.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Communication connection implementations
 * Requirements: REQ-COMM-001 to REQ-COMM-020
 **************************************************************************/

#include "IConnection.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

#include <cstring>

namespace TestMATE {

//=============================================================================
// CTcpConnection Implementation
//=============================================================================

CTcpConnection::CTcpConnection(const TString& in_strHost, TUInt16 in_port)
    : m_strHost(in_strHost)
    , m_port(in_port)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

CTcpConnection::~CTcpConnection() {
    Close();
#ifdef _WIN32
    WSACleanup();
#endif
}

CResult CTcpConnection::Open() {
    if (m_bConnected) {
        return TESTMATE_SUCCESS();
    }

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        m_strLastError = "Failed to create socket";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);

    struct addrinfo* result = nullptr;
    if (getaddrinfo(m_strHost.c_str(), nullptr, nullptr, &result) != 0) {
        closesocket(sock);
        m_strLastError = "Failed to resolve hostname";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    serverAddr.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
    freeaddrinfo(result);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        m_strLastError = "Failed to connect";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    m_socket = reinterpret_cast<void*>(sock);
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        m_strLastError = "Failed to create socket";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);

    struct hostent* host = gethostbyname(m_strHost.c_str());
    if (!host) {
        close(sock);
        m_strLastError = "Failed to resolve hostname";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    std::memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sock);
        m_strLastError = "Failed to connect";
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }

    m_socket = sock;
#endif

    m_bConnected = true;
    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::Close() {
    if (!m_bConnected) {
        return TESTMATE_SUCCESS();
    }

#ifdef _WIN32
    if (m_socket) {
        closesocket(reinterpret_cast<SOCKET>(m_socket));
        m_socket = nullptr;
    }
#else
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
#endif

    m_bConnected = false;
    return TESTMATE_SUCCESS();
}

bool CTcpConnection::IsOpen() const {
    return m_bConnected;
}

CResult CTcpConnection::Write(const TString& in_strData) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    TString data = in_strData + m_strTerminator;

#ifdef _WIN32
    int result = send(reinterpret_cast<SOCKET>(m_socket), data.c_str(),
                      static_cast<int>(data.length()), 0);
    if (result == SOCKET_ERROR) {
        m_strLastError = "Send failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#else
    ssize_t result = send(m_socket, data.c_str(), data.length(), 0);
    if (result < 0) {
        m_strLastError = "Send failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#endif

    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::Write(const TVector<TUInt8>& in_vecData) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

#ifdef _WIN32
    int result = send(reinterpret_cast<SOCKET>(m_socket),
                      reinterpret_cast<const char*>(in_vecData.data()),
                      static_cast<int>(in_vecData.size()), 0);
    if (result == SOCKET_ERROR) {
        m_strLastError = "Send failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#else
    ssize_t result = send(m_socket, in_vecData.data(), in_vecData.size(), 0);
    if (result < 0) {
        m_strLastError = "Send failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#endif

    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::Read(TString& out_strData, TInt64 in_timeoutMs) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    TInt64 timeout = in_timeoutMs > 0 ? in_timeoutMs : m_timeoutMs;
    out_strData.clear();

    char buffer[4096];
    TString accumulated;

#ifdef _WIN32
    SOCKET sock = reinterpret_cast<SOCKET>(m_socket);

    // Set receive timeout
    DWORD timeoutDw = static_cast<DWORD>(timeout);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutDw, sizeof(timeoutDw));

    while (true) {
        int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        accumulated += buffer;

        // Check for terminator
        if (!m_strTerminator.empty() &&
            accumulated.find(m_strTerminator) != TString::npos) {
            break;
        }
    }
#else
    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;

    while (true) {
        int pollResult = poll(&pfd, 1, static_cast<int>(timeout));
        if (pollResult <= 0) {
            break;  // Timeout or error
        }

        ssize_t bytesRead = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        accumulated += buffer;

        // Check for terminator
        if (!m_strTerminator.empty() &&
            accumulated.find(m_strTerminator) != TString::npos) {
            break;
        }
    }
#endif

    // Remove terminator from response
    if (!m_strTerminator.empty()) {
        size_t pos = accumulated.find(m_strTerminator);
        if (pos != TString::npos) {
            accumulated = accumulated.substr(0, pos);
        }
    }

    out_strData = accumulated;
    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes,
                              TInt64 in_timeoutMs) {
    TString strData;
    auto result = Read(strData, in_timeoutMs);
    if (result.IsFailure()) {
        return result;
    }

    out_vecData.assign(strData.begin(), strData.end());
    if (out_vecData.size() > in_uiMaxBytes) {
        out_vecData.resize(in_uiMaxBytes);
    }

    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::SetTimeout(TInt64 in_timeoutMs) {
    m_timeoutMs = in_timeoutMs;
    return TESTMATE_SUCCESS();
}

CResult CTcpConnection::SetTerminator(const TString& in_strTerm) {
    m_strTerminator = in_strTerm;
    return TESTMATE_SUCCESS();
}

TString CTcpConnection::GetAddress() const {
    return m_strHost + ":" + std::to_string(m_port);
}

//=============================================================================
// CSimulatedConnection Implementation
//=============================================================================

CSimulatedConnection::CSimulatedConnection() = default;

CResult CSimulatedConnection::Open() {
    m_bOpen = true;
    return TESTMATE_SUCCESS();
}

CResult CSimulatedConnection::Close() {
    m_bOpen = false;
    return TESTMATE_SUCCESS();
}

CResult CSimulatedConnection::Write(const TString& in_strData) {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }
    m_strLastCommand = in_strData;

    // Auto-generate responses for standard commands
    if (in_strData.find("*IDN?") != TString::npos) {
        m_strNextResponse = "SIMULATED,INSTRUMENT,SN001,1.0";
    } else if (in_strData.find("*TST?") != TString::npos) {
        m_strNextResponse = "0";
    } else if (in_strData.find("SYST:ERR?") != TString::npos) {
        m_strNextResponse = "0,\"No error\"";
    }

    return TESTMATE_SUCCESS();
}

CResult CSimulatedConnection::Write(const TVector<TUInt8>& in_vecData) {
    return Write(TString(in_vecData.begin(), in_vecData.end()));
}

CResult CSimulatedConnection::Read(TString& out_strData, TInt64 /*in_timeoutMs*/) {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }
    out_strData = m_strNextResponse;
    m_strNextResponse.clear();
    return TESTMATE_SUCCESS();
}

CResult CSimulatedConnection::Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes,
                                    TInt64 in_timeoutMs) {
    TString strData;
    auto result = Read(strData, in_timeoutMs);
    if (result.IsSuccess()) {
        out_vecData.assign(strData.begin(), strData.end());
        if (out_vecData.size() > in_uiMaxBytes) {
            out_vecData.resize(in_uiMaxBytes);
        }
    }
    return result;
}

CResult CSimulatedConnection::SetTimeout(TInt64 in_timeoutMs) {
    m_timeoutMs = in_timeoutMs;
    return TESTMATE_SUCCESS();
}

CResult CSimulatedConnection::SetTerminator(const TString& /*in_strTerm*/) {
    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
