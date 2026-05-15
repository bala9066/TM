/**************************************************************************
 * File Name: SerialConnection.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Serial port communication implementation
 **************************************************************************/

#include "SerialConnection.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace TestMATE {

CSerialConnection::CSerialConnection(const SSerialConfig& in_config)
    : m_config(in_config)
{
}

CSerialConnection::~CSerialConnection() {
    Close();
}

CResult CSerialConnection::Open() {
    if (m_bOpen) {
        return TESTMATE_SUCCESS();
    }

#ifdef _WIN32
    m_hPort = CreateFileA(
        m_config.portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (m_hPort == INVALID_HANDLE_VALUE) {
        m_strLastError = "Failed to open port: " + m_config.portName;
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }
#else
    m_fd = open(m_config.portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (m_fd < 0) {
        m_strLastError = "Failed to open port: " + m_config.portName;
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, m_strLastError);
    }
#endif

    auto result = ConfigurePort();
    if (result.IsFailure()) {
        Close();
        return result;
    }

    m_bOpen = true;
    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Close() {
    if (!m_bOpen) {
        return TESTMATE_SUCCESS();
    }

#ifdef _WIN32
    if (m_hPort && m_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPort);
        m_hPort = nullptr;
    }
#else
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
#endif

    m_bOpen = false;
    return TESTMATE_SUCCESS();
}

bool CSerialConnection::IsOpen() const {
    return m_bOpen;
}

CResult CSerialConnection::ConfigurePort() {
#ifdef _WIN32
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hPort, &dcb)) {
        m_strLastError = "Failed to get port state";
        return TESTMATE_FAILURE(EErrorCode::kConfigurationFailed, m_strLastError);
    }

    dcb.BaudRate = m_config.baudRate;
    dcb.ByteSize = m_config.dataBits;

    switch (m_config.parity) {
        case EParity::kNone: dcb.Parity = NOPARITY; break;
        case EParity::kOdd: dcb.Parity = ODDPARITY; break;
        case EParity::kEven: dcb.Parity = EVENPARITY; break;
        default: dcb.Parity = NOPARITY;
    }

    switch (m_config.stopBits) {
        case EStopBits::kOne: dcb.StopBits = ONESTOPBIT; break;
        case EStopBits::kTwo: dcb.StopBits = TWOSTOPBITS; break;
        default: dcb.StopBits = ONESTOPBIT;
    }

    dcb.fRtsControl = m_config.flowControl ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(m_hPort, &dcb)) {
        m_strLastError = "Failed to configure port";
        return TESTMATE_FAILURE(EErrorCode::kConfigurationFailed, m_strLastError);
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(m_config.timeoutMs);
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    SetCommTimeouts(m_hPort, &timeouts);

#else
    struct termios options;
    tcgetattr(m_fd, &options);

    speed_t speed;
    switch (m_config.baudRate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default: speed = B9600;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;

    if (!m_config.flowControl) {
        options.c_cflag &= ~CRTSCTS;
    }

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = m_config.timeoutMs / 100;

    tcsetattr(m_fd, TCSANOW, &options);
#endif

    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Write(const TString& in_strData) {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Port not open");
    }

    TString data = in_strData + m_config.terminator;

#ifdef _WIN32
    DWORD bytesWritten;
    if (!WriteFile(m_hPort, data.c_str(), static_cast<DWORD>(data.length()), &bytesWritten, nullptr)) {
        m_strLastError = "Write failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#else
    ssize_t result = write(m_fd, data.c_str(), data.length());
    if (result < 0) {
        m_strLastError = "Write failed";
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, m_strLastError);
    }
#endif

    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Write(const TVector<TUInt8>& in_vecData) {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Port not open");
    }

#ifdef _WIN32
    DWORD bytesWritten;
    if (!WriteFile(m_hPort, in_vecData.data(), static_cast<DWORD>(in_vecData.size()), &bytesWritten, nullptr)) {
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, "Write failed");
    }
#else
    if (write(m_fd, in_vecData.data(), in_vecData.size()) < 0) {
        return TESTMATE_FAILURE(EErrorCode::kSendFailed, "Write failed");
    }
#endif

    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Read(TString& out_strData, TInt64 in_timeoutMs) {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Port not open");
    }

    out_strData.clear();
    char buffer[4096];
    TInt64 timeout = in_timeoutMs > 0 ? in_timeoutMs : m_config.timeoutMs;

#ifdef _WIN32
    DWORD bytesRead;
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(timeout);
    SetCommTimeouts(m_hPort, &timeouts);

    if (ReadFile(m_hPort, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        out_strData = buffer;
    }
#else
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(m_fd, &readfds);

    if (select(m_fd + 1, &readfds, nullptr, nullptr, &tv) > 0) {
        ssize_t bytesRead = read(m_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            out_strData = buffer;
        }
    }
#endif

    // Remove terminator
    if (!m_config.terminator.empty()) {
        size_t pos = out_strData.find(m_config.terminator);
        if (pos != TString::npos) {
            out_strData = out_strData.substr(0, pos);
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Read(TVector<TUInt8>& out_vecData, TUInt32 in_uiMaxBytes, TInt64 in_timeoutMs) {
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

CResult CSerialConnection::SetTimeout(TInt64 in_timeoutMs) {
    m_config.timeoutMs = in_timeoutMs;
    if (m_bOpen) {
        return ConfigurePort();
    }
    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::SetTerminator(const TString& in_strTerm) {
    m_config.terminator = in_strTerm;
    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::SetBaudRate(TUInt32 in_baudRate) {
    m_config.baudRate = in_baudRate;
    if (m_bOpen) {
        return ConfigurePort();
    }
    return TESTMATE_SUCCESS();
}

CResult CSerialConnection::Flush() {
    if (!m_bOpen) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Port not open");
    }

#ifdef _WIN32
    PurgeComm(m_hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
    tcflush(m_fd, TCIOFLUSH);
#endif

    return TESTMATE_SUCCESS();
}

TUInt32 CSerialConnection::GetBytesAvailable() const {
    if (!m_bOpen) return 0;

#ifdef _WIN32
    COMSTAT status;
    DWORD errors;
    ClearCommError(m_hPort, &errors, &status);
    return status.cbInQue;
#else
    int bytes;
    ioctl(m_fd, FIONREAD, &bytes);
    return static_cast<TUInt32>(bytes);
#endif
}

} // namespace TestMATE
