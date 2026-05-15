/**************************************************************************
 * File Name: Types.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Common type definitions for TestMATE system.
 *              Provides standardized types following SDG coding guidelines.
 **************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <variant>

namespace TestMATE {

//=============================================================================
// Basic Type Aliases (SDG Compliant Naming)
//=============================================================================

using TByte = uint8_t;
using TWord = uint16_t;
using TDword = uint32_t;
using TQword = uint64_t;

using TInt8 = int8_t;
using TInt16 = int16_t;
using TInt32 = int32_t;
using TInt64 = int64_t;

using TUInt8 = uint8_t;
using TUInt16 = uint16_t;
using TUInt32 = uint32_t;
using TUInt64 = uint64_t;

using TFloat = float;
using TDouble = double;

using TString = std::string;
using TStringView = std::string_view;

//=============================================================================
// Time Types
//=============================================================================

// Monotonic clock — for measuring elapsed time, timeouts and deadlines.
// Never goes backwards, but has no relation to calendar time.
using TClock = std::chrono::steady_clock;
using TTimePoint = std::chrono::time_point<TClock>;
using TMonotonicTime = TTimePoint;  // clearer synonym for new code

// Wall-clock time — for "when did this happen": report/record timestamps,
// sequence created/modified dates. Convertible to calendar time.
using TWallClock = std::chrono::time_point<std::chrono::system_clock>;

using TDuration = std::chrono::milliseconds;
using TMicroseconds = std::chrono::microseconds;
using TNanoseconds = std::chrono::nanoseconds;

//=============================================================================
// Container Type Aliases
//=============================================================================

template<typename T>
using TVector = std::vector<T>;

template<typename K, typename V>
using TMap = std::map<K, V>;

template<typename T>
using TSet = std::set<T>;

template<typename T>
using TOptional = std::optional<T>;

template<typename... Ts>
using TVariant = std::variant<Ts...>;

//=============================================================================
// Smart Pointer Aliases
//=============================================================================

template<typename T>
using TUniquePtr = std::unique_ptr<T>;

template<typename T>
using TSharedPtr = std::shared_ptr<T>;

template<typename T>
using TWeakPtr = std::weak_ptr<T>;

//=============================================================================
// Callback Types
//=============================================================================

template<typename... Args>
using TCallback = std::function<void(Args...)>;

using TVoidCallback = std::function<void()>;
using TBoolCallback = std::function<bool()>;

//=============================================================================
// Identifier Types
//=============================================================================

using TResourceId = TUInt32;
using TSocketId = TUInt32;
using TSiteId = TUInt32;
using TStepId = TUInt64;
using TExecutionId = TUInt64;

constexpr TSocketId kInvalidSocketId = static_cast<TSocketId>(-1);
constexpr TSiteId kInvalidSiteId = static_cast<TSiteId>(-1);
constexpr TResourceId kInvalidResourceId = static_cast<TResourceId>(-1);

//=============================================================================
// Enumerations
//=============================================================================

/**************************************************************************
 * Enum: ELogLevel
 * Description: Logging severity levels
 **************************************************************************/
enum class ELogLevel : TInt32 {
    kTrace = 0,     ///< Very detailed diagnostic information
    kDebug = 1,     ///< Debug information for developers
    kInfo = 2,      ///< Informational messages
    kWarning = 3,   ///< Warning conditions
    kError = 4,     ///< Error conditions
    kFatal = 5      ///< Critical errors requiring shutdown
};

/**************************************************************************
 * Enum: ETestVerdict
 * Description: Test step execution verdict
 **************************************************************************/
enum class ETestVerdict : TInt32 {
    kNone = 0,      ///< No verdict yet
    kPass = 1,      ///< Test passed
    kFail = 2,      ///< Test failed
    kError = 3,     ///< Error during execution
    kSkipped = 4,   ///< Test was skipped
    kAborted = 5    ///< Test was aborted
};

/**************************************************************************
 * Enum: EExecutionState
 * Description: Test execution state
 **************************************************************************/
enum class EExecutionState : TInt32 {
    kIdle = 0,          ///< Not running
    kInitializing = 1,  ///< Initializing resources
    kRunning = 2,       ///< Executing tests
    kPaused = 3,        ///< Execution paused
    kCompleted = 4,     ///< Execution completed
    kError = 5,         ///< Error state
    kAborted = 6        ///< Execution aborted
};

/**************************************************************************
 * Enum: EResourceAccessType
 * Description: Resource access mode
 **************************************************************************/
enum class EResourceAccessType : TInt32 {
    kExclusive = 0,     ///< Exclusive access (only one owner)
    kShared = 1         ///< Shared access (multiple readers)
};

/**************************************************************************
 * Enum: ETaskPriority
 * Description: Task scheduling priority
 **************************************************************************/
enum class ETaskPriority : TInt32 {
    kLow = 0,
    kNormal = 1,
    kHigh = 2,
    kCritical = 3
};

//=============================================================================
// Constants
//=============================================================================

constexpr TUInt32 kMaxSockets = 32;
constexpr TUInt32 kMaxSites = 32;
constexpr TUInt32 kMaxThreadPoolSize = 64;
constexpr TUInt32 kMinThreadPoolSize = 2;
constexpr TUInt32 kDefaultThreadPoolSize = 8;

constexpr TDuration kDefaultTimeout{5000};      // 5 seconds
constexpr TDuration kDefaultStepTimeout{60000}; // 60 seconds

} // namespace TestMATE
