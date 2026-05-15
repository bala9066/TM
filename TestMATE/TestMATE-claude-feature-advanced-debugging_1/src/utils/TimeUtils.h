/**************************************************************************
 * File Name: TimeUtils.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Time utility functions for TestMATE application.
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace TestMATE {
namespace TimeUtils {

/**************************************************************************
 * Function Name: Now
 * Description: Gets current steady clock time point
 * Returns: Current time point
 **************************************************************************/
[[nodiscard]] inline TTimePoint Now() {
    return TClock::now();
}

/**************************************************************************
 * Function Name: ElapsedMs
 * Description: Calculates elapsed milliseconds since time point
 * Parameters:
 *   in_startTime - Start time point
 * Returns: Elapsed time in milliseconds
 **************************************************************************/
[[nodiscard]] inline TInt64 ElapsedMs(TTimePoint in_startTime) {
    auto elapsed = TClock::now() - in_startTime;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

/**************************************************************************
 * Function Name: ElapsedUs
 * Description: Calculates elapsed microseconds since time point
 * Parameters:
 *   in_startTime - Start time point
 * Returns: Elapsed time in microseconds
 **************************************************************************/
[[nodiscard]] inline TInt64 ElapsedUs(TTimePoint in_startTime) {
    auto elapsed = TClock::now() - in_startTime;
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

/**************************************************************************
 * Function Name: DurationMs
 * Description: Calculates duration between two time points in milliseconds
 * Parameters:
 *   in_startTime - Start time point
 *   in_endTime - End time point
 * Returns: Duration in milliseconds
 **************************************************************************/
[[nodiscard]] inline TInt64 DurationMs(TTimePoint in_startTime, TTimePoint in_endTime) {
    auto duration = in_endTime - in_startTime;
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

/**************************************************************************
 * Function Name: GetCurrentTimestamp
 * Description: Gets current system time as formatted string
 * Returns: Formatted timestamp string (YYYY-MM-DD HH:MM:SS.mmm)
 **************************************************************************/
[[nodiscard]] inline TString GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

/**************************************************************************
 * Function Name: GetDateString
 * Description: Gets current date as string
 * Returns: Date string (YYYY-MM-DD)
 **************************************************************************/
[[nodiscard]] inline TString GetDateString() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d");
    return oss.str();
}

/**************************************************************************
 * Function Name: GetTimeString
 * Description: Gets current time as string
 * Returns: Time string (HH:MM:SS)
 **************************************************************************/
[[nodiscard]] inline TString GetTimeString() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%H:%M:%S");
    return oss.str();
}

/**************************************************************************
 * Function Name: SleepMs
 * Description: Sleeps for specified milliseconds
 * Parameters:
 *   in_iMs - Milliseconds to sleep
 **************************************************************************/
inline void SleepMs(TInt64 in_iMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(in_iMs));
}

/**************************************************************************
 * Function Name: SleepUs
 * Description: Sleeps for specified microseconds
 * Parameters:
 *   in_iUs - Microseconds to sleep
 **************************************************************************/
inline void SleepUs(TInt64 in_iUs) {
    std::this_thread::sleep_for(std::chrono::microseconds(in_iUs));
}

/**************************************************************************
 * Class: CScopedTimer
 * Description: RAII-based timer for measuring code execution time
 **************************************************************************/
class CScopedTimer {
public:
    /**************************************************************************
     * Function Name: CScopedTimer
     * Description: Starts the timer
     **************************************************************************/
    CScopedTimer() : m_startTime(Now()) {}

    /**************************************************************************
     * Function Name: Reset
     * Description: Resets the timer to current time
     **************************************************************************/
    void Reset() { m_startTime = Now(); }

    /**************************************************************************
     * Function Name: GetElapsedMs
     * Description: Gets elapsed time in milliseconds
     * Returns: Elapsed milliseconds
     **************************************************************************/
    [[nodiscard]] TInt64 GetElapsedMs() const { return ElapsedMs(m_startTime); }

    /**************************************************************************
     * Function Name: GetElapsedUs
     * Description: Gets elapsed time in microseconds
     * Returns: Elapsed microseconds
     **************************************************************************/
    [[nodiscard]] TInt64 GetElapsedUs() const { return ElapsedUs(m_startTime); }

private:
    TTimePoint m_startTime;
};

} // namespace TimeUtils
} // namespace TestMATE
