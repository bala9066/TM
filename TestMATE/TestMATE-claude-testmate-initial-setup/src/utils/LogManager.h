/**************************************************************************
 * File Name: LogManager.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Centralized logging system for TestMATE application.
 *              Provides multi-level, multi-destination logging with
 *              minimal performance impact.
 * Requirements: REQ-LOG-001 to REQ-LOG-011
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"

#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace TestMATE {

//=============================================================================
// Forward Declarations
//=============================================================================

class ILogHandler;

//=============================================================================
// Log Handler Interface
//=============================================================================

/**************************************************************************
 * Class: ILogHandler
 * Description: Interface for log output destinations.
 *              Implement this to create custom log handlers.
 * Requirements: REQ-LOG-003 (multiple destinations)
 **************************************************************************/
class ILogHandler {
public:
    virtual ~ILogHandler() = default;

    /**************************************************************************
     * Function Name: Write
     * Description: Writes a log message to the handler's destination
     * Parameters:
     *   in_eLevel - Log level
     *   in_strSource - Source component name
     *   in_strMessage - Log message
     *   in_timeStamp - Message timestamp
     **************************************************************************/
    virtual void Write(ELogLevel in_eLevel,
                       const TString& in_strSource,
                       const TString& in_strMessage,
                       TTimePoint in_timeStamp) = 0;

    /**************************************************************************
     * Function Name: Flush
     * Description: Flushes any buffered output
     **************************************************************************/
    virtual void Flush() = 0;
};

//=============================================================================
// Standard Log Handlers
//=============================================================================

/**************************************************************************
 * Class: CConsoleLogHandler
 * Description: Outputs log messages to console (stdout/stderr)
 * Requirements: REQ-LOG-003
 **************************************************************************/
class CConsoleLogHandler : public ILogHandler {
public:
    void Write(ELogLevel in_eLevel,
               const TString& in_strSource,
               const TString& in_strMessage,
               TTimePoint in_timeStamp) override;
    void Flush() override;

private:
    std::mutex m_mutex;
};

/**************************************************************************
 * Class: CFileLogHandler
 * Description: Outputs log messages to file
 * Requirements: REQ-LOG-003
 **************************************************************************/
class CFileLogHandler : public ILogHandler {
public:
    /**************************************************************************
     * Function Name: CFileLogHandler
     * Description: Creates file log handler
     * Parameters:
     *   in_strFilePath - Path to log file
     *   in_bAppend - If true, append to existing file
     **************************************************************************/
    explicit CFileLogHandler(const TString& in_strFilePath, bool in_bAppend = true);
    ~CFileLogHandler() override;

    void Write(ELogLevel in_eLevel,
               const TString& in_strSource,
               const TString& in_strMessage,
               TTimePoint in_timeStamp) override;
    void Flush() override;

    [[nodiscard]] bool IsOpen() const { return m_ofFile.is_open(); }

private:
    std::ofstream m_ofFile;
    std::mutex m_mutex;
};

//=============================================================================
// Log Manager (Singleton)
//=============================================================================

/**************************************************************************
 * Class: CLogManager
 * Description: Central logging manager implementing singleton pattern.
 *              Thread-safe logging with multiple output handlers.
 * Requirements: REQ-LOG-001 to REQ-LOG-011
 **************************************************************************/
class CLogManager {
public:
    // Delete copy/move
    CLogManager(const CLogManager&) = delete;
    CLogManager& operator=(const CLogManager&) = delete;
    CLogManager(CLogManager&&) = delete;
    CLogManager& operator=(CLogManager&&) = delete;

    /**************************************************************************
     * Function Name: GetInstance
     * Description: Returns singleton instance of LogManager
     * Returns: Reference to CLogManager instance
     * Requirements: REQ-LOG-001
     **************************************************************************/
    static CLogManager& GetInstance();

    /**************************************************************************
     * Function Name: Initialize
     * Description: Initializes the logging system
     * Parameters:
     *   in_eDefaultLevel - Default minimum log level
     * Returns: Result indicating success or failure
     **************************************************************************/
    CResult Initialize(ELogLevel in_eDefaultLevel = ELogLevel::kInfo);

    /**************************************************************************
     * Function Name: Shutdown
     * Description: Shuts down logging system and flushes all handlers
     **************************************************************************/
    void Shutdown();

    /**************************************************************************
     * Function Name: SetGlobalLevel
     * Description: Sets minimum log level for all loggers
     * Parameters:
     *   in_eLevel - Minimum level to log
     * Requirements: REQ-LOG-002
     **************************************************************************/
    void SetGlobalLevel(ELogLevel in_eLevel);

    /**************************************************************************
     * Function Name: GetGlobalLevel
     * Description: Gets current global log level
     * Returns: Current minimum log level
     **************************************************************************/
    [[nodiscard]] ELogLevel GetGlobalLevel() const;

    /**************************************************************************
     * Function Name: AddHandler
     * Description: Registers a log output handler
     * Parameters:
     *   in_pHandler - Handler to add (takes ownership)
     * Requirements: REQ-LOG-003
     **************************************************************************/
    void AddHandler(TUniquePtr<ILogHandler> in_pHandler);

    /**************************************************************************
     * Function Name: ClearHandlers
     * Description: Removes all log handlers
     **************************************************************************/
    void ClearHandlers();

    /**************************************************************************
     * Function Name: Log
     * Description: Writes a log message
     * Parameters:
     *   in_eLevel - Log level
     *   in_strSource - Source component name
     *   in_strMessage - Log message
     * Requirements: REQ-LOG-004, REQ-LOG-005
     **************************************************************************/
    void Log(ELogLevel in_eLevel,
             const TString& in_strSource,
             const TString& in_strMessage);

    // Convenience methods with format support
    template<typename... Args>
    void LogTrace(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kTrace, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogDebug(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kDebug, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogInfo(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kInfo, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogWarning(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kWarning, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogError(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kError, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogFatal(const TString& in_strSource, const TString& in_strFormat, Args&&... args) {
        LogFormatted(ELogLevel::kFatal, in_strSource, in_strFormat, std::forward<Args>(args)...);
    }

    /**************************************************************************
     * Function Name: Flush
     * Description: Flushes all log handlers
     **************************************************************************/
    void Flush();

private:
    CLogManager();
    ~CLogManager();

    template<typename... Args>
    void LogFormatted(ELogLevel in_eLevel, const TString& in_strSource,
                      const TString& in_strFormat, Args&&... args) {
        // Early exit if level filtered
        if (static_cast<TInt32>(in_eLevel) < static_cast<TInt32>(m_eGlobalLevel)) {
            return;
        }

        // Simple formatting using stringstream
        std::ostringstream oss;
        FormatMessage(oss, in_strFormat, std::forward<Args>(args)...);
        Log(in_eLevel, in_strSource, oss.str());
    }

    // Variadic format helper
    void FormatMessage(std::ostringstream& out_oss, const TString& in_strFormat) {
        out_oss << in_strFormat;
    }

    template<typename T, typename... Rest>
    void FormatMessage(std::ostringstream& out_oss, const TString& in_strFormat,
                       T&& value, Rest&&... rest) {
        size_t pos = in_strFormat.find("{}");
        if (pos != TString::npos) {
            out_oss << in_strFormat.substr(0, pos) << std::forward<T>(value);
            FormatMessage(out_oss, in_strFormat.substr(pos + 2), std::forward<Rest>(rest)...);
        } else {
            out_oss << in_strFormat;
        }
    }

    TVector<TUniquePtr<ILogHandler>> m_vecHandlers;
    ELogLevel m_eGlobalLevel;
    mutable std::mutex m_mutex;
    bool m_bInitialized;
};

//=============================================================================
// Logging Macros
//=============================================================================

#define LOG_TRACE(source, ...) \
    TestMATE::CLogManager::GetInstance().LogTrace(source, __VA_ARGS__)

#define LOG_DEBUG(source, ...) \
    TestMATE::CLogManager::GetInstance().LogDebug(source, __VA_ARGS__)

#define LOG_INFO(source, ...) \
    TestMATE::CLogManager::GetInstance().LogInfo(source, __VA_ARGS__)

#define LOG_WARNING(source, ...) \
    TestMATE::CLogManager::GetInstance().LogWarning(source, __VA_ARGS__)

#define LOG_ERROR(source, ...) \
    TestMATE::CLogManager::GetInstance().LogError(source, __VA_ARGS__)

#define LOG_FATAL(source, ...) \
    TestMATE::CLogManager::GetInstance().LogFatal(source, __VA_ARGS__)

} // namespace TestMATE
