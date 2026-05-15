/**
 * @file BreakpointManager.h
 * @brief Central manager for all breakpoints in the debugging system
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the CBreakpointManager class which manages all breakpoints
 * in the system, providing thread-safe access and operations.
 */

#ifndef TESTMATE_DEBUG_BREAKPOINT_MANAGER_H
#define TESTMATE_DEBUG_BREAKPOINT_MANAGER_H

#include "testmate/debug/Breakpoint.h"
#include <map>
#include <vector>
#include <mutex>
#include <memory>

namespace TestMATE {

// Type aliases
template<typename T>
using TVector = std::vector<T>;
template<typename K, typename V>
using TMap = std::map<K, V>;

/**
 * @brief Statistics about all breakpoints in the system
 */
struct SBreakpointManagerStats {
    TUInt32 totalBreakpoints{0};              ///< Total number of breakpoints
    TUInt32 enabledBreakpoints{0};            ///< Number of enabled breakpoints
    TUInt32 disabledBreakpoints{0};           ///< Number of disabled breakpoints
    TUInt32 totalHits{0};                     ///< Total hits across all breakpoints
    TUInt32 activeBreakpoint{0};              ///< Currently active breakpoint (0 = none)
};

/**
 * @brief Configuration for the breakpoint manager
 */
struct SBreakpointManagerConfig {
    TUInt32 maxBreakpoints{1000};             ///< Maximum number of breakpoints allowed
    bool allowDuplicateLocations{true};       ///< Allow multiple breakpoints at same location
    bool autoSaveEnabled{false};              ///< Automatically save breakpoints to file
    TString autoSaveFile;                     ///< File path for auto-save
};

/**
 * @brief Central manager for all breakpoints in the debugging system
 *
 * CBreakpointManager provides thread-safe management of all breakpoints.
 * It handles adding, removing, enabling, disabling breakpoints, and
 * checking if execution should break at a given location.
 *
 * Example usage:
 * @code
 * CBreakpointManager manager;
 *
 * // Add a simple step entry breakpoint
 * auto bpId1 = manager.AddBreakpoint(EBreakpointType::kStepEntry, "STEP-010");
 *
 * // Add a conditional breakpoint
 * auto bpId2 = manager.AddBreakpoint(EBreakpointType::kConditional, "STEP-020");
 * SBreakpointCondition condition{"voltage > 5.0", [&]() { return GetVoltage() > 5.0; }};
 * manager.SetBreakpointCondition(bpId2, condition);
 *
 * // During test execution, check for breakpoints
 * if (auto bpId = manager.CheckBreakpoint("STEP-010"); bpId != 0) {
 *     // Breakpoint hit! Pause execution
 *     auto bp = manager.GetBreakpoint(bpId);
 *     std::cout << "Hit breakpoint at " << bp->GetLocation() << std::endl;
 * }
 * @endcode
 */
class CBreakpointManager {
public:
    /**
     * @brief Construct a new breakpoint manager
     * @param in_config Configuration for the manager
     */
    explicit CBreakpointManager(const SBreakpointManagerConfig& in_config = {});

    /**
     * @brief Destructor
     */
    ~CBreakpointManager();

    // Prevent copying (use shared_ptr if sharing needed)
    CBreakpointManager(const CBreakpointManager&) = delete;
    CBreakpointManager& operator=(const CBreakpointManager&) = delete;

    // ==================== Breakpoint Creation & Deletion ====================

    /**
     * @brief Add a new breakpoint
     * @param in_type Type of breakpoint to create
     * @param in_location Location (step ID) where breakpoint should trigger
     * @return Unique breakpoint ID (0 on failure)
     */
    TUInt32 AddBreakpoint(EBreakpointType in_type, const TString& in_location);

    /**
     * @brief Remove a breakpoint
     * @param in_id Breakpoint ID to remove
     * @return true if removed successfully, false if not found
     */
    bool RemoveBreakpoint(TUInt32 in_id);

    /**
     * @brief Remove all breakpoints at a specific location
     * @param in_location Location (step ID)
     * @return Number of breakpoints removed
     */
    TUInt32 RemoveBreakpointsAt(const TString& in_location);

    /**
     * @brief Clear all breakpoints
     */
    void ClearAllBreakpoints();

    // ==================== Breakpoint Access ====================

    /**
     * @brief Get a breakpoint by ID
     * @param in_id Breakpoint ID
     * @return Shared owning pointer to the breakpoint, or nullptr if not found.
     *         A shared_ptr is returned (rather than a raw pointer) so the
     *         breakpoint cannot be freed by a concurrent RemoveBreakpoint /
     *         ClearAllBreakpoints while the caller still holds it.
     */
    [[nodiscard]] std::shared_ptr<CBreakpoint> GetBreakpoint(TUInt32 in_id);

    /**
     * @brief Get a breakpoint by ID (const version)
     * @param in_id Breakpoint ID
     * @return Shared owning pointer to the const breakpoint, or nullptr.
     */
    [[nodiscard]] std::shared_ptr<const CBreakpoint> GetBreakpoint(TUInt32 in_id) const;

    /**
     * @brief Get all breakpoints at a specific location
     * @param in_location Location (step ID)
     * @return Vector of shared owning breakpoint pointers
     */
    [[nodiscard]] TVector<std::shared_ptr<CBreakpoint>> GetBreakpointsAt(const TString& in_location);

    /**
     * @brief Get all breakpoints
     * @return Vector of all shared owning breakpoint pointers
     */
    [[nodiscard]] TVector<std::shared_ptr<CBreakpoint>> GetAllBreakpoints();

    /**
     * @brief Get breakpoint count
     * @return Total number of breakpoints
     */
    [[nodiscard]] TUInt32 GetBreakpointCount() const;

    // ==================== Breakpoint State Management ====================

    /**
     * @brief Enable a breakpoint
     * @param in_id Breakpoint ID
     * @return true if successful, false if not found
     */
    bool EnableBreakpoint(TUInt32 in_id);

    /**
     * @brief Disable a breakpoint
     * @param in_id Breakpoint ID
     * @return true if successful, false if not found
     */
    bool DisableBreakpoint(TUInt32 in_id);

    /**
     * @brief Enable all breakpoints
     */
    void EnableAllBreakpoints();

    /**
     * @brief Disable all breakpoints
     */
    void DisableAllBreakpoints();

    // ==================== Breakpoint Configuration ====================

    /**
     * @brief Set condition for a conditional breakpoint
     * @param in_id Breakpoint ID
     * @param in_condition Condition to set
     * @return true if successful, false if not found or wrong type
     */
    bool SetBreakpointCondition(TUInt32 in_id, const SBreakpointCondition& in_condition);

    /**
     * @brief Set hit count for a hit-count breakpoint
     * @param in_id Breakpoint ID
     * @param in_hitCount Hit count threshold
     * @return true if successful, false if not found or wrong type
     */
    bool SetBreakpointHitCount(TUInt32 in_id, TUInt32 in_hitCount);

    /**
     * @brief Set data watchpoint configuration
     * @param in_id Breakpoint ID
     * @param in_watchpoint Watchpoint configuration
     * @return true if successful, false if not found or wrong type
     */
    bool SetBreakpointWatchpoint(TUInt32 in_id, const SDataWatchpoint& in_watchpoint);

    // ==================== Breakpoint Checking ====================

    /**
     * @brief Check if execution should break at a given step
     * @param in_stepId Step identifier to check
     * @return Breakpoint ID if should break, 0 otherwise
     *
     * This method:
     * 1. Finds all breakpoints at the given location
     * 2. Evaluates each breakpoint's condition
     * 3. Returns the ID of the first breakpoint that should trigger
     */
    [[nodiscard]] TUInt32 CheckBreakpoint(const TString& in_stepId);

    /**
     * @brief Check if should break on data access
     * @param in_variableName Variable being accessed
     * @param in_isWrite true if write access, false if read
     * @return Breakpoint ID if should break, 0 otherwise
     */
    [[nodiscard]] TUInt32 CheckDataBreakpoint(const TString& in_variableName, bool in_isWrite);

    // ==================== Statistics & Reporting ====================

    /**
     * @brief Get statistics about all breakpoints
     * @return Statistics structure
     */
    [[nodiscard]] SBreakpointManagerStats GetStats() const;

    /**
     * @brief Generate a text report of all breakpoints
     * @return Human-readable report string
     */
    [[nodiscard]] TString GenerateReport() const;

    /**
     * @brief List all breakpoints (formatted output)
     * @return Vector of formatted strings, one per breakpoint
     */
    [[nodiscard]] TVector<TString> ListBreakpoints() const;

    // ==================== Persistence ====================

    /**
     * @brief Save all breakpoints to a file
     * @param in_filePath Path to save file
     * @return true if successful, false on error
     */
    bool SaveToFile(const TString& in_filePath) const;

    /**
     * @brief Load breakpoints from a file
     * @param in_filePath Path to load file
     * @param in_clearExisting Clear existing breakpoints before loading
     * @return Number of breakpoints loaded (0 on error)
     */
    TUInt32 LoadFromFile(const TString& in_filePath, bool in_clearExisting = true);

private:
    /**
     * @brief Update statistics (called internally after changes)
     */
    void UpdateStats();

    // Configuration
    SBreakpointManagerConfig m_config;

    // Breakpoint storage
    TMap<TUInt32, std::shared_ptr<CBreakpoint>> m_breakpoints;  ///< All breakpoints by ID
    TUInt32 m_nextId{1};                                         ///< Next breakpoint ID to assign

    // Thread safety
    mutable std::mutex m_mutex;                                  ///< Protects all member variables

    // Statistics
    mutable SBreakpointManagerStats m_stats;                     ///< Cached statistics
};

} // namespace TestMATE

#endif // TESTMATE_DEBUG_BREAKPOINT_MANAGER_H
