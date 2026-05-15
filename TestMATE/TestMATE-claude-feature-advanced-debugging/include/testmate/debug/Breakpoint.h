/**
 * @file Breakpoint.h
 * @brief Interactive debugging breakpoint system for TestMATE
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the CBreakpoint class which represents a breakpoint
 * that can be set at any test step to pause execution, inspect variables,
 * and step through tests interactively.
 */

#ifndef TESTMATE_DEBUG_BREAKPOINT_H
#define TESTMATE_DEBUG_BREAKPOINT_H

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace TestMATE {

// Type aliases matching TestMATE conventions
using TString = std::string;
using TUInt32 = uint32_t;
using TUInt64 = uint64_t;
using TDouble = double;
using TTime = std::chrono::system_clock::time_point;

/**
 * @brief Types of breakpoints supported by the debugger
 */
enum class EBreakpointType {
    kStepEntry,      ///< Break when entering a test step
    kStepExit,       ///< Break when exiting a test step
    kConditional,    ///< Break only when a condition is true
    kDataAccess,     ///< Break when a variable is accessed/modified
    kException,      ///< Break when an exception occurs
    kHitCount        ///< Break after N hits
};

/**
 * @brief Current state of a breakpoint
 */
enum class EBreakpointState {
    kEnabled,        ///< Breakpoint is active
    kDisabled,       ///< Breakpoint is inactive
    kOneShot,        ///< Breakpoint triggers once then disables
    kTemporary       ///< Breakpoint will be deleted after triggering
};

/**
 * @brief Condition for conditional breakpoints
 */
struct SBreakpointCondition {
    TString expression;                          ///< Human-readable expression (e.g., "voltage > 5.0")
    std::function<bool()> evaluator;            ///< Function that evaluates the condition

    SBreakpointCondition() = default;
    SBreakpointCondition(const TString& expr, std::function<bool()> eval)
        : expression(expr), evaluator(std::move(eval)) {}
};

/**
 * @brief Data access watchpoint configuration
 */
struct SDataWatchpoint {
    TString variableName;                        ///< Name of variable to watch
    bool breakOnRead{false};                    ///< Break when variable is read
    bool breakOnWrite{true};                    ///< Break when variable is written
    std::optional<TString> oldValue;            ///< Previous value (as string)
    std::optional<TString> newValue;            ///< New value (as string)
};

/**
 * @brief Statistics about breakpoint hits
 */
struct SBreakpointStats {
    TUInt32 totalHits{0};                       ///< Total number of times hit
    TTime firstHit;                             ///< Time of first hit
    TTime lastHit;                              ///< Time of most recent hit
    TDouble averageTimeBetweenHits{0.0};       ///< Average time between hits (ms)
};

/**
 * @brief Individual breakpoint with conditions and state management
 *
 * CBreakpoint represents a single breakpoint in the test execution.
 * It supports various types of breakpoints including conditional,
 * hit-count based, and data watchpoints.
 *
 * Example usage:
 * @code
 * // Create conditional breakpoint
 * CBreakpoint bp(1, EBreakpointType::kConditional);
 * bp.SetLocation("STEP-010");
 * bp.SetCondition({"voltage > 5.0", [&]() { return GetVoltage() > 5.0; }});
 * bp.Enable();
 *
 * // Check if should break
 * if (bp.ShouldBreak()) {
 *     // Pause execution
 * }
 * @endcode
 */
class CBreakpoint {
public:
    /**
     * @brief Construct a new breakpoint
     * @param in_id Unique identifier for this breakpoint
     * @param in_type Type of breakpoint (entry, exit, conditional, etc.)
     */
    explicit CBreakpoint(TUInt32 in_id, EBreakpointType in_type);

    /**
     * @brief Virtual destructor
     */
    virtual ~CBreakpoint() = default;

    // ==================== State Management ====================

    /**
     * @brief Enable the breakpoint
     */
    void Enable();

    /**
     * @brief Disable the breakpoint
     */
    void Disable();

    /**
     * @brief Set breakpoint as one-shot (triggers once then disables)
     */
    void SetOneShot();

    /**
     * @brief Check if breakpoint is enabled
     * @return true if enabled, false otherwise
     */
    [[nodiscard]] bool IsEnabled() const;

    /**
     * @brief Get current breakpoint state
     * @return Current state (enabled, disabled, one-shot, temporary)
     */
    [[nodiscard]] EBreakpointState GetState() const { return m_state; }

    // ==================== Location Management ====================

    /**
     * @brief Set the location where this breakpoint should trigger
     * @param in_location Step ID or location identifier (e.g., "STEP-010")
     */
    void SetLocation(const TString& in_location);

    /**
     * @brief Get the breakpoint location
     * @return Location string (step ID)
     */
    [[nodiscard]] const TString& GetLocation() const { return m_location; }

    // ==================== Condition Management ====================

    /**
     * @brief Set a condition for conditional breakpoints
     * @param in_condition Condition with expression and evaluator function
     */
    void SetCondition(const SBreakpointCondition& in_condition);

    /**
     * @brief Get the current condition
     * @return Optional condition (nullopt if no condition set)
     */
    [[nodiscard]] const std::optional<SBreakpointCondition>& GetCondition() const {
        return m_condition;
    }

    // ==================== Hit Count Management ====================

    /**
     * @brief Set hit count threshold (for kHitCount breakpoints)
     * @param in_count Number of hits before breaking
     */
    void SetHitCount(TUInt32 in_count);

    /**
     * @brief Get hit count threshold
     * @return Hit count threshold
     */
    [[nodiscard]] TUInt32 GetHitCountThreshold() const { return m_hitCountThreshold; }

    /**
     * @brief Get current hit count
     * @return Number of times this breakpoint has been hit
     */
    [[nodiscard]] TUInt32 GetCurrentHits() const { return m_currentHits; }

    /**
     * @brief Reset hit count to zero
     */
    void ResetHitCount();

    /**
     * @brief Record a hit on this breakpoint
     */
    void RecordHit();

    // ==================== Data Watchpoint Management ====================

    /**
     * @brief Set data watchpoint configuration
     * @param in_watchpoint Watchpoint settings (variable name, read/write triggers)
     */
    void SetDataWatchpoint(const SDataWatchpoint& in_watchpoint);

    /**
     * @brief Get data watchpoint configuration
     * @return Optional watchpoint (nullopt if not a data watchpoint)
     */
    [[nodiscard]] const std::optional<SDataWatchpoint>& GetDataWatchpoint() const {
        return m_dataWatchpoint;
    }

    // ==================== Breakpoint Evaluation ====================

    /**
     * @brief Determine if execution should break at this point
     * @return true if should break, false otherwise
     *
     * This method evaluates:
     * - Breakpoint enabled state
     * - Condition (for conditional breakpoints)
     * - Hit count (for hit-count breakpoints)
     * - Data access (for watchpoints)
     */
    [[nodiscard]] bool ShouldBreak() const;

    // ==================== Metadata ====================

    /**
     * @brief Get unique breakpoint ID
     * @return Breakpoint identifier
     */
    [[nodiscard]] TUInt32 GetId() const { return m_id; }

    /**
     * @brief Get breakpoint type
     * @return Type (entry, exit, conditional, etc.)
     */
    [[nodiscard]] EBreakpointType GetType() const { return m_type; }

    /**
     * @brief Get breakpoint statistics
     * @return Statistics including hit count and timing
     */
    [[nodiscard]] SBreakpointStats GetStats() const { return m_stats; }

    /**
     * @brief Get creation time
     * @return Time when breakpoint was created
     */
    [[nodiscard]] TTime GetCreatedTime() const { return m_createdTime; }

    // ==================== Serialization ====================

    /**
     * @brief Serialize breakpoint to string (for saving configurations)
     * @return JSON-formatted string representation
     */
    [[nodiscard]] TString ToString() const;

    /**
     * @brief Deserialize breakpoint from string
     * @param in_str JSON-formatted string
     * @return Breakpoint instance or nullptr on error
     */
    static std::unique_ptr<CBreakpoint> FromString(const TString& in_str);

private:
    TUInt32 m_id;                                           ///< Unique identifier
    EBreakpointType m_type;                                 ///< Breakpoint type
    EBreakpointState m_state{EBreakpointState::kEnabled};  ///< Current state
    TString m_location;                                     ///< Step ID or location
    TTime m_createdTime;                                    ///< Creation timestamp

    // Conditional breakpoint data
    std::optional<SBreakpointCondition> m_condition;        ///< Optional condition

    // Hit count breakpoint data
    TUInt32 m_hitCountThreshold{0};                        ///< Hits before breaking
    TUInt32 m_currentHits{0};                              ///< Current hit count

    // Data watchpoint data
    std::optional<SDataWatchpoint> m_dataWatchpoint;        ///< Optional watchpoint

    // Statistics
    SBreakpointStats m_stats;                               ///< Hit statistics
};

} // namespace TestMATE

#endif // TESTMATE_DEBUG_BREAKPOINT_H
