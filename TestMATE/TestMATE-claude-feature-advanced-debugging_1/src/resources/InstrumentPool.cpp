/**************************************************************************
 * File Name: InstrumentPool.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Implementation of instrument resource pool
 **************************************************************************/

#include "testmate/resources/InstrumentPool.h"
#include <algorithm>
#include <chrono>

namespace TestMATE {

//=============================================================================
// Singleton Instance
//=============================================================================

CInstrumentPool& CInstrumentPool::GetInstance() {
    static CInstrumentPool instance;
    return instance;
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CInstrumentPool::CInstrumentPool() {
    // Initialize with default configuration
    m_config.defaultReservationTimeoutMs = 5000;
    m_config.maxReservationDurationMs = 3600000;  // 1 hour
    m_config.enableAutoRelease = true;
    m_config.enableHealthMonitoring = true;
    m_config.calibrationCheckIntervalMs = 86400000;  // 24 hours
}

CInstrumentPool::~CInstrumentPool() {
    ReleaseAll();
}

//=============================================================================
// Configuration
//=============================================================================

void CInstrumentPool::Configure(const SPoolConfig& in_config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = in_config;
}

//=============================================================================
// Registration
//=============================================================================

CResult CInstrumentPool::RegisterInstrument(
    IInstrument* in_pInstrument,
    const TString& in_instrumentId,
    const TString& in_type,
    const TString& in_model,
    const TString& in_serialNumber,
    const TString& in_location) {

    if (!in_pInstrument) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Instrument pointer is null");
    }

    if (in_instrumentId.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Instrument ID is empty");
    }

    if (in_type.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Instrument type is empty");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if already registered
    if (m_instruments.find(in_instrumentId) != m_instruments.end()) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists,
            "Instrument already registered: " + in_instrumentId);
    }

    // Register instrument
    m_instruments[in_instrumentId] = in_pInstrument;

    // Create instrument info
    SPoolInstrumentInfo info;
    info.instrumentId = in_instrumentId;
    info.type = in_type;
    info.model = in_model;
    info.serialNumber = in_serialNumber;
    info.location = in_location;
    info.state = EPoolInstrumentState::kAvailable;
    info.currentUser = "";
    info.health.isHealthy = true;
    info.health.lastUsed = std::chrono::steady_clock::now();
    info.health.lastCalibration = std::chrono::steady_clock::now();
    info.health.nextCalibrationDue = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(m_config.calibrationCheckIntervalMs);

    m_instrumentInfo[in_instrumentId] = info;

    return TESTMATE_SUCCESS();
}

CResult CInstrumentPool::UnregisterInstrument(const TString& in_instrumentId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
            "Instrument not found: " + in_instrumentId);
    }

    // Check if in use
    if (it->second.state == EPoolInstrumentState::kInUse ||
        it->second.state == EPoolInstrumentState::kReserved) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
            "Cannot unregister instrument in use: " + in_instrumentId);
    }

    // Remove from maps
    m_instruments.erase(in_instrumentId);
    m_instrumentInfo.erase(in_instrumentId);

    return TESTMATE_SUCCESS();
}

//=============================================================================
// Reservation
//=============================================================================

IInstrument* CInstrumentPool::ReserveInstrument(
    const TString& in_type,
    const TString& in_userId,
    TUInt32 in_timeoutMs,
    TUInt32 in_durationMs) {

    if (in_type.empty()) {
        return nullptr;
    }

    if (in_userId.empty()) {
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(in_timeoutMs);

    while (true) {
        // Check for expired reservations
        CheckExpiredReservations();

        // Find available instrument of requested type
        for (auto& [id, info] : m_instrumentInfo) {
            if (info.type == in_type && info.state == EPoolInstrumentState::kAvailable) {
                // Reserve it
                info.state = EPoolInstrumentState::kReserved;
                info.currentUser = in_userId;
                info.reservedAt = std::chrono::steady_clock::now();

                TUInt32 duration = in_durationMs > 0 ? in_durationMs : m_config.maxReservationDurationMs;
                info.reservedUntil = info.reservedAt + std::chrono::milliseconds(duration);

                // Update state to in use
                info.state = EPoolInstrumentState::kInUse;
                info.health.lastUsed = info.reservedAt;
                info.health.totalUseCount++;

                m_totalReservations++;

                // Return instrument pointer
                return m_instruments[id];
            }
        }

        // No instrument available - check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);

        if (in_timeoutMs == 0 || elapsed >= timeout) {
            // Timeout
            m_timeoutReservations++;
            return nullptr;
        }

        // Wait for notification or timeout
        auto remainingTime = timeout - elapsed;
        m_cv.wait_for(lock, remainingTime);
    }
}

IInstrument* CInstrumentPool::ReserveInstrumentById(
    const TString& in_instrumentId,
    const TString& in_userId,
    TUInt32 in_timeoutMs,
    TUInt32 in_durationMs) {

    if (in_instrumentId.empty()) {
        return nullptr;
    }

    if (in_userId.empty()) {
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return nullptr;  // Not found
    }

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(in_timeoutMs);

    while (true) {
        // Check for expired reservations
        CheckExpiredReservations();

        // Check if available
        if (it->second.state == EPoolInstrumentState::kAvailable) {
            // Reserve it
            it->second.state = EPoolInstrumentState::kReserved;
            it->second.currentUser = in_userId;
            it->second.reservedAt = std::chrono::steady_clock::now();

            TUInt32 duration = in_durationMs > 0 ? in_durationMs : m_config.maxReservationDurationMs;
            it->second.reservedUntil = it->second.reservedAt + std::chrono::milliseconds(duration);

            // Update state to in use
            it->second.state = EPoolInstrumentState::kInUse;
            it->second.health.lastUsed = it->second.reservedAt;
            it->second.health.totalUseCount++;

            m_totalReservations++;

            // Return instrument pointer
            return m_instruments[in_instrumentId];
        }

        // Not available - check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);

        if (in_timeoutMs == 0 || elapsed >= timeout) {
            // Timeout
            m_timeoutReservations++;
            return nullptr;
        }

        // Wait for notification or timeout
        auto remainingTime = timeout - elapsed;
        m_cv.wait_for(lock, remainingTime);
    }
}

//=============================================================================
// Release
//=============================================================================

CResult CInstrumentPool::ReleaseInstrument(IInstrument* in_pInstrument) {
    if (!in_pInstrument) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Instrument pointer is null");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Find instrument ID by pointer
    TString instrumentId = FindInstrumentId(in_pInstrument);
    if (instrumentId.empty()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Instrument not found in pool");
    }

    auto it = m_instrumentInfo.find(instrumentId);
    if (it == m_instrumentInfo.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Instrument info not found");
    }

    // Calculate usage time
    auto now = std::chrono::steady_clock::now();
    auto usageTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - it->second.reservedAt).count() / 1000.0;

    it->second.health.totalUseTimeSeconds += usageTime;

    // Release
    it->second.state = EPoolInstrumentState::kAvailable;
    it->second.currentUser = "";
    it->second.reservedAt = TTimePoint();
    it->second.reservedUntil = TTimePoint();

    m_totalReleases++;

    // Notify waiting threads
    m_cv.notify_all();

    return TESTMATE_SUCCESS();
}

CResult CInstrumentPool::ReleaseInstrumentById(const TString& in_instrumentId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
            "Instrument not found: " + in_instrumentId);
    }

    // Calculate usage time
    if (it->second.state == EPoolInstrumentState::kInUse ||
        it->second.state == EPoolInstrumentState::kReserved) {

        auto now = std::chrono::steady_clock::now();
        auto usageTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - it->second.reservedAt).count() / 1000.0;

        it->second.health.totalUseTimeSeconds += usageTime;
    }

    // Release
    it->second.state = EPoolInstrumentState::kAvailable;
    it->second.currentUser = "";
    it->second.reservedAt = TTimePoint();
    it->second.reservedUntil = TTimePoint();

    m_totalReleases++;

    // Notify waiting threads
    m_cv.notify_all();

    return TESTMATE_SUCCESS();
}

//=============================================================================
// Query Methods
//=============================================================================

TVector<SPoolInstrumentInfo> CInstrumentPool::GetAvailableInstruments(
    const TString& in_type) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<SPoolInstrumentInfo> available;

    for (const auto& [id, info] : m_instrumentInfo) {
        if (info.state == EPoolInstrumentState::kAvailable) {
            if (in_type.empty() || info.type == in_type) {
                available.push_back(info);
            }
        }
    }

    return available;
}

TVector<SPoolInstrumentInfo> CInstrumentPool::GetAllInstruments() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<SPoolInstrumentInfo> all;
    all.reserve(m_instrumentInfo.size());

    for (const auto& [id, info] : m_instrumentInfo) {
        all.push_back(info);
    }

    return all;
}

std::optional<SPoolInstrumentInfo> CInstrumentPool::GetInstrumentInfo(
    const TString& in_instrumentId) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool CInstrumentPool::IsAvailable(const TString& in_instrumentId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return false;
    }

    return it->second.state == EPoolInstrumentState::kAvailable;
}

bool CInstrumentPool::IsRegistered(const TString& in_instrumentId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_instruments.find(in_instrumentId) != m_instruments.end();
}

//=============================================================================
// State Management
//=============================================================================

CResult CInstrumentPool::SetInstrumentState(
    const TString& in_instrumentId,
    EPoolInstrumentState in_state) {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
            "Instrument not found: " + in_instrumentId);
    }

    it->second.state = in_state;

    return TESTMATE_SUCCESS();
}

CResult CInstrumentPool::UpdateInstrumentHealth(
    const TString& in_instrumentId,
    bool in_isHealthy,
    const TString& in_errorMessage) {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_instrumentInfo.find(in_instrumentId);
    if (it == m_instrumentInfo.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
            "Instrument not found: " + in_instrumentId);
    }

    it->second.health.isHealthy = in_isHealthy;
    it->second.health.lastError = in_errorMessage;

    if (!in_isHealthy) {
        it->second.state = EPoolInstrumentState::kError;
    }

    return TESTMATE_SUCCESS();
}

//=============================================================================
// Statistics
//=============================================================================

SPoolStatistics CInstrumentPool::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    SPoolStatistics stats;
    stats.totalInstruments = static_cast<TUInt32>(m_instruments.size());
    stats.totalReservations = m_totalReservations.load();
    stats.totalReleases = m_totalReleases.load();
    stats.timeoutReservations = m_timeoutReservations.load();

    // Count by state
    for (const auto& [id, info] : m_instrumentInfo) {
        switch (info.state) {
            case EPoolInstrumentState::kAvailable:
                stats.availableInstruments++;
                break;
            case EPoolInstrumentState::kReserved:
                stats.reservedInstruments++;
                break;
            case EPoolInstrumentState::kInUse:
                stats.inUseInstruments++;
                break;
            case EPoolInstrumentState::kError:
            case EPoolInstrumentState::kCalibrationDue:
            case EPoolInstrumentState::kMaintenance:
                stats.errorInstruments++;
                break;
        }
    }

    // Calculate average reservation time
    TDouble totalTime = 0.0;
    TUInt32 completedReservations = 0;

    for (const auto& [id, info] : m_instrumentInfo) {
        if (info.health.totalUseCount > 0) {
            totalTime += info.health.totalUseTimeSeconds;
            completedReservations += info.health.totalUseCount;
        }
    }

    if (completedReservations > 0) {
        stats.averageReservationTimeSeconds = totalTime / completedReservations;
    }

    return stats;
}

//=============================================================================
// Cleanup
//=============================================================================

CResult CInstrumentPool::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if any instruments are in use
    for (const auto& [id, info] : m_instrumentInfo) {
        if (info.state == EPoolInstrumentState::kInUse ||
            info.state == EPoolInstrumentState::kReserved) {
            return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                "Cannot clear pool while instruments are in use");
        }
    }

    m_instruments.clear();
    m_instrumentInfo.clear();

    return TESTMATE_SUCCESS();
}

void CInstrumentPool::ReleaseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Force release all instruments
    for (auto& [id, info] : m_instrumentInfo) {
        info.state = EPoolInstrumentState::kAvailable;
        info.currentUser = "";
        info.reservedAt = TTimePoint();
        info.reservedUntil = TTimePoint();
    }

    m_cv.notify_all();
}

//=============================================================================
// Private Helper Methods
//=============================================================================

void CInstrumentPool::CheckExpiredReservations() {
    if (!m_config.enableAutoRelease) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    for (auto& [id, info] : m_instrumentInfo) {
        if ((info.state == EPoolInstrumentState::kReserved ||
             info.state == EPoolInstrumentState::kInUse) &&
            now > info.reservedUntil) {

            // Auto-release expired reservation
            auto usageTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - info.reservedAt).count() / 1000.0;

            info.health.totalUseTimeSeconds += usageTime;
            info.state = EPoolInstrumentState::kAvailable;
            info.currentUser = "";
            info.reservedAt = TTimePoint();
            info.reservedUntil = TTimePoint();

            m_totalReleases++;
        }
    }
}

TString CInstrumentPool::FindInstrumentId(IInstrument* in_pInstrument) const {
    for (const auto& [id, ptr] : m_instruments) {
        if (ptr == in_pInstrument) {
            return id;
        }
    }
    return "";
}

} // namespace TestMATE
