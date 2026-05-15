/**
 * @file BreakpointManager.cpp
 * @brief Implementation of CBreakpointManager class
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/BreakpointManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace TestMATE {

// ==================== Constructor & Destructor ====================

CBreakpointManager::CBreakpointManager(const SBreakpointManagerConfig& in_config)
    : m_config(in_config)
    , m_nextId(1) {
}

CBreakpointManager::~CBreakpointManager() {
    // Auto-save if enabled
    if (m_config.autoSaveEnabled && !m_config.autoSaveFile.empty()) {
        SaveToFile(m_config.autoSaveFile);
    }
}

// ==================== Breakpoint Creation & Deletion ====================

TUInt32 CBreakpointManager::AddBreakpoint(EBreakpointType in_type, const TString& in_location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if we've reached the maximum
    if (m_breakpoints.size() >= m_config.maxBreakpoints) {
        return 0; // Failed: too many breakpoints
    }

    // Check for duplicate locations if not allowed
    if (!m_config.allowDuplicateLocations) {
        for (const auto& [id, bp] : m_breakpoints) {
            if (bp->GetLocation() == in_location) {
                return 0; // Failed: duplicate location
            }
        }
    }

    // Create new breakpoint
    TUInt32 newId = m_nextId++;
    auto breakpoint = std::make_shared<CBreakpoint>(newId, in_type);
    breakpoint->SetLocation(in_location);

    // Store the breakpoint
    m_breakpoints[newId] = std::move(breakpoint);

    // Update statistics
    UpdateStats();

    return newId;
}

bool CBreakpointManager::RemoveBreakpoint(TUInt32 in_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false; // Not found
    }

    m_breakpoints.erase(it);
    UpdateStats();
    return true;
}

TUInt32 CBreakpointManager::RemoveBreakpointsAt(const TString& in_location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    TUInt32 removedCount = 0;

    // Collect IDs to remove (can't erase while iterating)
    TVector<TUInt32> idsToRemove;
    for (const auto& [id, bp] : m_breakpoints) {
        if (bp->GetLocation() == in_location) {
            idsToRemove.push_back(id);
        }
    }

    // Remove the breakpoints
    for (TUInt32 id : idsToRemove) {
        m_breakpoints.erase(id);
        removedCount++;
    }

    if (removedCount > 0) {
        UpdateStats();
    }

    return removedCount;
}

void CBreakpointManager::ClearAllBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_breakpoints.clear();
    UpdateStats();
}

// ==================== Breakpoint Access ====================

std::shared_ptr<CBreakpoint> CBreakpointManager::GetBreakpoint(TUInt32 in_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it != m_breakpoints.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<const CBreakpoint> CBreakpointManager::GetBreakpoint(TUInt32 in_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it != m_breakpoints.end()) {
        return it->second;
    }
    return nullptr;
}

TVector<std::shared_ptr<CBreakpoint>> CBreakpointManager::GetBreakpointsAt(const TString& in_location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<std::shared_ptr<CBreakpoint>> result;
    for (auto& [id, bp] : m_breakpoints) {
        if (bp->GetLocation() == in_location) {
            result.push_back(bp);
        }
    }
    return result;
}

TVector<std::shared_ptr<CBreakpoint>> CBreakpointManager::GetAllBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<std::shared_ptr<CBreakpoint>> result;
    result.reserve(m_breakpoints.size());
    for (auto& [id, bp] : m_breakpoints) {
        result.push_back(bp);
    }
    return result;
}

TUInt32 CBreakpointManager::GetBreakpointCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<TUInt32>(m_breakpoints.size());
}

// ==================== Breakpoint State Management ====================

bool CBreakpointManager::EnableBreakpoint(TUInt32 in_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false;
    }

    it->second->Enable();
    UpdateStats();
    return true;
}

bool CBreakpointManager::DisableBreakpoint(TUInt32 in_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false;
    }

    it->second->Disable();
    UpdateStats();
    return true;
}

void CBreakpointManager::EnableAllBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, bp] : m_breakpoints) {
        bp->Enable();
    }
    UpdateStats();
}

void CBreakpointManager::DisableAllBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, bp] : m_breakpoints) {
        bp->Disable();
    }
    UpdateStats();
}

// ==================== Breakpoint Configuration ====================

bool CBreakpointManager::SetBreakpointCondition(TUInt32 in_id, const SBreakpointCondition& in_condition) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false;
    }

    // Verify this is a conditional breakpoint
    if (it->second->GetType() != EBreakpointType::kConditional) {
        return false;
    }

    it->second->SetCondition(in_condition);
    return true;
}

bool CBreakpointManager::SetBreakpointHitCount(TUInt32 in_id, TUInt32 in_hitCount) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false;
    }

    // Verify this is a hit count breakpoint
    if (it->second->GetType() != EBreakpointType::kHitCount) {
        return false;
    }

    it->second->SetHitCount(in_hitCount);
    return true;
}

bool CBreakpointManager::SetBreakpointWatchpoint(TUInt32 in_id, const SDataWatchpoint& in_watchpoint) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(in_id);
    if (it == m_breakpoints.end()) {
        return false;
    }

    // Verify this is a data access breakpoint
    if (it->second->GetType() != EBreakpointType::kDataAccess) {
        return false;
    }

    it->second->SetDataWatchpoint(in_watchpoint);
    return true;
}

// ==================== Breakpoint Checking ====================

TUInt32 CBreakpointManager::CheckBreakpoint(const TString& in_stepId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Find all breakpoints at this location
    for (auto& [id, bp] : m_breakpoints) {
        if (bp->GetLocation() == in_stepId) {
            // Record the hit
            bp->RecordHit();

            // Check if should break
            if (bp->ShouldBreak()) {
                m_stats.activeBreakpoint = id;
                return id;
            }
        }
    }

    return 0; // No breakpoint triggered
}

TUInt32 CBreakpointManager::CheckDataBreakpoint(const TString& in_variableName, bool in_isWrite) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Find data access breakpoints for this variable
    for (auto& [id, bp] : m_breakpoints) {
        if (bp->GetType() != EBreakpointType::kDataAccess) {
            continue;
        }

        auto watchpoint = bp->GetDataWatchpoint();
        if (!watchpoint) {
            continue;
        }

        if (watchpoint->variableName != in_variableName) {
            continue;
        }

        // Check if should break on this access type
        bool shouldBreak = (in_isWrite && watchpoint->breakOnWrite) ||
                          (!in_isWrite && watchpoint->breakOnRead);

        if (shouldBreak && bp->IsEnabled()) {
            bp->RecordHit();
            m_stats.activeBreakpoint = id;
            return id;
        }
    }

    return 0; // No data breakpoint triggered
}

// ==================== Statistics & Reporting ====================

SBreakpointManagerStats CBreakpointManager::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

TString CBreakpointManager::GenerateReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "=== Breakpoint Manager Report ===\n";
    oss << "Total Breakpoints: " << m_stats.totalBreakpoints << "\n";
    oss << "Enabled: " << m_stats.enabledBreakpoints << "\n";
    oss << "Disabled: " << m_stats.disabledBreakpoints << "\n";
    oss << "Total Hits: " << m_stats.totalHits << "\n\n";

    if (m_breakpoints.empty()) {
        oss << "No breakpoints set.\n";
    } else {
        oss << "Breakpoints:\n";
        for (const auto& [id, bp] : m_breakpoints) {
            oss << "  [" << id << "] ";
            oss << (bp->IsEnabled() ? "✓" : "✗") << " ";
            oss << bp->GetLocation() << " ";

            switch (bp->GetType()) {
                case EBreakpointType::kStepEntry:     oss << "(Entry)"; break;
                case EBreakpointType::kStepExit:      oss << "(Exit)"; break;
                case EBreakpointType::kConditional:   oss << "(Conditional)"; break;
                case EBreakpointType::kDataAccess:    oss << "(Data)"; break;
                case EBreakpointType::kException:     oss << "(Exception)"; break;
                case EBreakpointType::kHitCount:      oss << "(HitCount)"; break;
            }

            auto stats = bp->GetStats();
            oss << " - Hits: " << stats.totalHits;
            oss << "\n";
        }
    }

    return oss.str();
}

TVector<TString> CBreakpointManager::ListBreakpoints() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<TString> result;
    for (const auto& [id, bp] : m_breakpoints) {
        std::ostringstream oss;
        oss << "[" << id << "] ";
        oss << (bp->IsEnabled() ? "Enabled " : "Disabled ");
        oss << bp->GetLocation();

        if (bp->GetType() == EBreakpointType::kConditional) {
            auto cond = bp->GetCondition();
            if (cond) {
                oss << " when (" << cond->expression << ")";
            }
        } else if (bp->GetType() == EBreakpointType::kHitCount) {
            oss << " after " << bp->GetHitCountThreshold() << " hits";
            oss << " (current: " << bp->GetCurrentHits() << ")";
        }

        result.push_back(oss.str());
    }
    return result;
}

// ==================== Persistence ====================

bool CBreakpointManager::SaveToFile(const TString& in_filePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ofstream file(in_filePath);
    if (!file.is_open()) {
        return false;
    }

    // Write JSON array of breakpoints
    file << "[\n";
    bool first = true;
    for (const auto& [id, bp] : m_breakpoints) {
        if (!first) {
            file << ",\n";
        }
        file << "  " << bp->ToString();
        first = false;
    }
    file << "\n]\n";

    file.close();
    return true;
}

TUInt32 CBreakpointManager::LoadFromFile(const TString& in_filePath, bool in_clearExisting) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // This is a placeholder implementation
    // A complete implementation would use a JSON library like nlohmann/json
    // to properly parse the file and restore breakpoints

    // For now, return 0 to indicate that loading requires JSON library integration
    // This will be implemented in Week 2

    (void)in_filePath;
    (void)in_clearExisting;
    return 0;
}

// ==================== Private Methods ====================

void CBreakpointManager::UpdateStats() {
    // Assumes mutex is already locked by caller

    m_stats.totalBreakpoints = static_cast<TUInt32>(m_breakpoints.size());
    m_stats.enabledBreakpoints = 0;
    m_stats.disabledBreakpoints = 0;
    m_stats.totalHits = 0;

    for (const auto& [id, bp] : m_breakpoints) {
        if (bp->IsEnabled()) {
            m_stats.enabledBreakpoints++;
        } else {
            m_stats.disabledBreakpoints++;
        }

        m_stats.totalHits += bp->GetStats().totalHits;
    }
}

} // namespace TestMATE
