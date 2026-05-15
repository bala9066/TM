/**************************************************************************
 * File Name: LogManager.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of centralized logging system.
 * Requirements: REQ-LOG-001 to REQ-LOG-011
 **************************************************************************/

#include "LogManager.h"
#include <iostream>
#include <ctime>

namespace TestMATE {

//=============================================================================
// Helper Functions
//=============================================================================

namespace {

/**************************************************************************
 * Function Name: GetLogLevelString
 * Description: Converts log level to string representation
 * Parameters:
 *   in_eLevel - Log level to convert
 * Returns: String name of log level
 **************************************************************************/
const char* GetLogLevelString(ELogLevel in_eLevel) {
    switch (in_eLevel) {
        case ELogLevel::kTrace:   return "TRACE";
        case ELogLevel::kDebug:   return "DEBUG";
        case ELogLevel::kInfo:    return "INFO ";
        case ELogLevel::kWarning: return "WARN ";
        case ELogLevel::kError:   return "ERROR";
        case ELogLevel::kFatal:   return "FATAL";
        default:                  return "?????";
    }
}

/**************************************************************************
 * Function Name: FormatTimestamp
 * Description: Formats timestamp for log output
 * Parameters:
 *   in_timePoint - Time point to format
 * Returns: Formatted timestamp string
 **************************************************************************/
TString FormatTimestamp(TTimePoint in_timePoint) {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // anonymous namespace

//=============================================================================
// CConsoleLogHandler Implementation
//=============================================================================

void CConsoleLogHandler::Write(ELogLevel in_eLevel,
                                const TString& in_strSource,
                                const TString& in_strMessage,
                                TTimePoint in_timeStamp) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostream& os = (in_eLevel >= ELogLevel::kError) ? std::cerr : std::cout;

    os << "[" << FormatTimestamp(in_timeStamp) << "] "
       << "[" << GetLogLevelString(in_eLevel) << "] "
       << "[" << in_strSource << "] "
       << in_strMessage << std::endl;
}

void CConsoleLogHandler::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout.flush();
    std::cerr.flush();
}

//=============================================================================
// CFileLogHandler Implementation
//=============================================================================

CFileLogHandler::CFileLogHandler(const TString& in_strFilePath, bool in_bAppend)
    : m_ofFile(in_strFilePath, in_bAppend ? std::ios::app : std::ios::trunc)
{
}

CFileLogHandler::~CFileLogHandler() {
    if (m_ofFile.is_open()) {
        m_ofFile.flush();
        m_ofFile.close();
    }
}

void CFileLogHandler::Write(ELogLevel in_eLevel,
                             const TString& in_strSource,
                             const TString& in_strMessage,
                             TTimePoint in_timeStamp) {
    if (!m_ofFile.is_open()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_ofFile << "[" << FormatTimestamp(in_timeStamp) << "] "
             << "[" << GetLogLevelString(in_eLevel) << "] "
             << "[" << in_strSource << "] "
             << in_strMessage << std::endl;
}

void CFileLogHandler::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ofFile.is_open()) {
        m_ofFile.flush();
    }
}

//=============================================================================
// CLogManager Implementation
//=============================================================================

CLogManager::CLogManager()
    : m_eGlobalLevel(ELogLevel::kInfo)
    , m_bInitialized(false)
{
}

CLogManager::~CLogManager() {
    Shutdown();
}

CLogManager& CLogManager::GetInstance() {
    static CLogManager instance;
    return instance;
}

CResult CLogManager::Initialize(ELogLevel in_eDefaultLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bInitialized) {
        return CResult(EErrorCode::kAlreadyInitialized, "LogManager already initialized");
    }

    m_eGlobalLevel = in_eDefaultLevel;

    // Add default console handler
    m_vecHandlers.push_back(std::make_unique<CConsoleLogHandler>());

    m_bInitialized = true;

    return CResult::Success();
}

void CLogManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized) {
        return;
    }

    // Flush all handlers
    for (auto& pHandler : m_vecHandlers) {
        if (pHandler) {
            pHandler->Flush();
        }
    }

    m_vecHandlers.clear();
    m_bInitialized = false;
}

void CLogManager::SetGlobalLevel(ELogLevel in_eLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eGlobalLevel = in_eLevel;
}

ELogLevel CLogManager::GetGlobalLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_eGlobalLevel;
}

void CLogManager::AddHandler(TUniquePtr<ILogHandler> in_pHandler) {
    if (!in_pHandler) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_vecHandlers.push_back(std::move(in_pHandler));
}

void CLogManager::ClearHandlers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vecHandlers.clear();
}

void CLogManager::Log(ELogLevel in_eLevel,
                      const TString& in_strSource,
                      const TString& in_strMessage) {
    // Early exit for filtered messages (avoid lock)
    if (static_cast<TInt32>(in_eLevel) < static_cast<TInt32>(m_eGlobalLevel)) {
        return;
    }

    TTimePoint timeNow = TClock::now();

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pHandler : m_vecHandlers) {
        if (pHandler) {
            pHandler->Write(in_eLevel, in_strSource, in_strMessage, timeNow);
        }
    }
}

void CLogManager::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pHandler : m_vecHandlers) {
        if (pHandler) {
            pHandler->Flush();
        }
    }
}

} // namespace TestMATE
