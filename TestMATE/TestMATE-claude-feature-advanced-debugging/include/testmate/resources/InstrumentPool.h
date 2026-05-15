/**
 * @file InstrumentPool.h
 * @brief Shared instrument resource management and pooling
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the instrument resource pool system that manages
 * shared instrument resources across multiple test sequences with
 * reservation, locking, and availability tracking.
 */

#ifndef TESTMATE_RESOURCES_INSTRUMENT_POOL_H
#define TESTMATE_RESOURCES_INSTRUMENT_POOL_H

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace TestMATE {

// Forward declaration
class IInstrument;

/**
 * @brief Pool instrument state enumeration
 */
enum class EPoolInstrumentState {
    kAvailable,        ///< Ready for use
    kReserved,         ///< Reserved but not yet in use
    kInUse,            ///< Currently being used
    kError,            ///< Error state, needs attention
    kCalibrationDue,   ///< Calibration is due
    kMaintenance       ///< Under maintenance
};

/**
 * @brief Instrument health status
 */
struct SInstrumentHealth {
    bool isHealthy{true};
    TString lastError;
    TTimePoint lastUsed;
    TTimePoint lastCalibration;
    TTimePoint nextCalibrationDue;
    TUInt32 totalUseCount{0};
    TDouble totalUseTimeSeconds{0.0};
};

/**
 * @brief Pool instrument information
 */
struct SPoolInstrumentInfo {
    TString instrumentId;
    TString type;              ///< "DMM", "Scope", "PowerSupply", etc.
    TString model;
    TString serialNumber;
    TString location;          ///< Physical location
    EPoolInstrumentState state{EPoolInstrumentState::kAvailable};
    TString currentUser;       ///< Empty if available
    TTimePoint reservedAt;
    TTimePoint reservedUntil;
    SInstrumentHealth health;
};

/**
 * @brief Pool configuration
 */
struct SPoolConfig {
    TUInt32 defaultReservationTimeoutMs{5000};
    TUInt32 maxReservationDurationMs{3600000};  ///< 1 hour max
    bool enableAutoRelease{true};                ///< Auto-release on timeout
    bool enableHealthMonitoring{true};
    TUInt32 calibrationCheckIntervalMs{86400000}; ///< 24 hours
};

/**
 * @brief Pool statistics
 */
struct SPoolStatistics {
    TUInt32 totalInstruments{0};
    TUInt32 availableInstruments{0};
    TUInt32 reservedInstruments{0};
    TUInt32 inUseInstruments{0};
    TUInt32 errorInstruments{0};
    TUInt32 totalReservations{0};
    TUInt32 totalReleases{0};
    TUInt32 timeoutReservations{0};
    TDouble averageReservationTimeSeconds{0.0};
};

/**
 * @brief Instrument Resource Pool
 *
 * Manages shared instrument resources with:
 * - Singleton pattern for global access
 * - Thread-safe operations
 * - Reservation and locking
 * - Timeout handling
 * - Health monitoring
 * - Usage statistics
 *
 * Example usage:
 * @code
 * auto& pool = CInstrumentPool::GetInstance();
 *
 * // Register instruments
 * pool.RegisterInstrument(dmmPtr, "DMM-1", "DMM", "Keysight 34461A", "SN12345");
 * pool.RegisterInstrument(psuPtr, "PSU-1", "PowerSupply", "Keysight E36312A", "SN67890");
 *
 * // Reserve instrument
 * auto* dmm = pool.ReserveInstrument("DMM", 5000);
 * if (dmm) {
 *     // Use instrument
 *     dmm->Initialize();
 *     // ... perform measurements ...
 *
 *     // Release when done
 *     pool.ReleaseInstrument(dmmPtr);
 * }
 *
 * // Check availability
 * auto available = pool.GetAvailableInstruments("DMM");
 *
 * // Get statistics
 * auto stats = pool.GetStatistics();
 * @endcode
 */
class CInstrumentPool {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton pool
     */
    static CInstrumentPool& GetInstance();

    // Delete copy/move constructors and assignment operators
    CInstrumentPool(const CInstrumentPool&) = delete;
    CInstrumentPool& operator=(const CInstrumentPool&) = delete;
    CInstrumentPool(CInstrumentPool&&) = delete;
    CInstrumentPool& operator=(CInstrumentPool&&) = delete;

    /**
     * @brief Configure the pool
     * @param in_config Pool configuration
     */
    void Configure(const SPoolConfig& in_config);

    /**
     * @brief Register instrument in pool
     * @param in_pInstrument Pointer to instrument
     * @param in_instrumentId Unique identifier
     * @param in_type Instrument type (DMM, Scope, etc.)
     * @param in_model Instrument model
     * @param in_serialNumber Serial number
     * @param in_location Physical location (optional)
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult RegisterInstrument(
        IInstrument* in_pInstrument,
        const TString& in_instrumentId,
        const TString& in_type,
        const TString& in_model,
        const TString& in_serialNumber,
        const TString& in_location = "");

    /**
     * @brief Unregister instrument from pool
     * @param in_instrumentId Instrument identifier
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult UnregisterInstrument(const TString& in_instrumentId);

    /**
     * @brief Reserve instrument for exclusive use
     * @param in_type Instrument type to reserve
     * @param in_userId User/sequence identifier
     * @param in_timeoutMs Timeout in milliseconds (0 = no wait)
     * @param in_durationMs Reservation duration (0 = default)
     * @return Instrument pointer or nullptr if not available
     */
    [[nodiscard]] IInstrument* ReserveInstrument(
        const TString& in_type,
        const TString& in_userId,
        TUInt32 in_timeoutMs = 5000,
        TUInt32 in_durationMs = 0);

    /**
     * @brief Reserve specific instrument by ID
     * @param in_instrumentId Specific instrument ID
     * @param in_userId User/sequence identifier
     * @param in_timeoutMs Timeout in milliseconds
     * @param in_durationMs Reservation duration (0 = default)
     * @return Instrument pointer or nullptr if not available
     */
    [[nodiscard]] IInstrument* ReserveInstrumentById(
        const TString& in_instrumentId,
        const TString& in_userId,
        TUInt32 in_timeoutMs = 5000,
        TUInt32 in_durationMs = 0);

    /**
     * @brief Release instrument back to pool
     * @param in_pInstrument Instrument pointer to release
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult ReleaseInstrument(IInstrument* in_pInstrument);

    /**
     * @brief Release instrument by ID
     * @param in_instrumentId Instrument identifier
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult ReleaseInstrumentById(const TString& in_instrumentId);

    /**
     * @brief Get available instruments of type
     * @param in_type Instrument type filter (empty = all)
     * @return Vector of available instrument info
     */
    [[nodiscard]] TVector<SPoolInstrumentInfo> GetAvailableInstruments(
        const TString& in_type = "") const;

    /**
     * @brief Get all instruments
     * @return Vector of all instrument info
     */
    [[nodiscard]] TVector<SPoolInstrumentInfo> GetAllInstruments() const;

    /**
     * @brief Get instrument information
     * @param in_instrumentId Instrument identifier
     * @return Instrument info or nullopt if not found
     */
    [[nodiscard]] std::optional<SPoolInstrumentInfo> GetInstrumentInfo(
        const TString& in_instrumentId) const;

    /**
     * @brief Check if instrument is available
     * @param in_instrumentId Instrument identifier
     * @return true if available
     */
    [[nodiscard]] bool IsAvailable(const TString& in_instrumentId) const;

    /**
     * @brief Check if instrument exists
     * @param in_instrumentId Instrument identifier
     * @return true if registered
     */
    [[nodiscard]] bool IsRegistered(const TString& in_instrumentId) const;

    /**
     * @brief Set instrument state
     * @param in_instrumentId Instrument identifier
     * @param in_state New state
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult SetInstrumentState(
        const TString& in_instrumentId,
        EPoolInstrumentState in_state);

    /**
     * @brief Update instrument health
     * @param in_instrumentId Instrument identifier
     * @param in_isHealthy Health status
     * @param in_errorMessage Error message (if unhealthy)
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult UpdateInstrumentHealth(
        const TString& in_instrumentId,
        bool in_isHealthy,
        const TString& in_errorMessage = "");

    /**
     * @brief Get pool statistics
     * @return Pool statistics
     */
    [[nodiscard]] SPoolStatistics GetStatistics() const;

    /**
     * @brief Clear all instruments from pool
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult Clear();

    /**
     * @brief Release all instruments (for cleanup)
     */
    void ReleaseAll();

private:
    /**
     * @brief Private constructor for singleton
     */
    CInstrumentPool();

    /**
     * @brief Destructor
     */
    ~CInstrumentPool();

    /**
     * @brief Check and auto-release expired reservations
     */
    void CheckExpiredReservations();

    /**
     * @brief Find instrument by pointer
     * @param in_pInstrument Instrument pointer
     * @return Instrument ID or empty string
     */
    TString FindInstrumentId(IInstrument* in_pInstrument) const;

    SPoolConfig m_config;
    std::unordered_map<TString, IInstrument*> m_instruments;
    std::unordered_map<TString, SPoolInstrumentInfo> m_instrumentInfo;
    std::unordered_map<TString, TString> m_instrumentIdByType;  // type -> first available ID

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    // Statistics
    std::atomic<TUInt32> m_totalReservations{0};
    std::atomic<TUInt32> m_totalReleases{0};
    std::atomic<TUInt32> m_timeoutReservations{0};
};

/**
 * @brief Convert instrument state to string
 * @param in_state Instrument state
 * @return String representation
 */
inline TString PoolInstrumentStateToString(EPoolInstrumentState in_state) {
    switch (in_state) {
        case EPoolInstrumentState::kAvailable:      return "Available";
        case EPoolInstrumentState::kReserved:       return "Reserved";
        case EPoolInstrumentState::kInUse:          return "InUse";
        case EPoolInstrumentState::kError:          return "Error";
        case EPoolInstrumentState::kCalibrationDue: return "CalibrationDue";
        case EPoolInstrumentState::kMaintenance:    return "Maintenance";
        default:                                 return "Unknown";
    }
}

} // namespace TestMATE

#endif // TESTMATE_RESOURCES_INSTRUMENT_POOL_H
