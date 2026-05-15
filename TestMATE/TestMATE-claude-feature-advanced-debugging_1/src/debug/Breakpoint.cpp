/**
 * @file Breakpoint.cpp
 * @brief Implementation of CBreakpoint class
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/Breakpoint.h"
#include <sstream>
#include <iomanip>

namespace TestMATE {

// ==================== Constructor ====================

CBreakpoint::CBreakpoint(TUInt32 in_id, EBreakpointType in_type)
    : m_id(in_id)
    , m_type(in_type)
    , m_state(EBreakpointState::kEnabled)
    , m_hitCountThreshold(0)
    , m_currentHits(0)
    , m_createdTime(std::chrono::system_clock::now()) {
}

// ==================== State Management ====================

void CBreakpoint::Enable() {
    m_state = EBreakpointState::kEnabled;
}

void CBreakpoint::Disable() {
    m_state = EBreakpointState::kDisabled;
}

void CBreakpoint::SetOneShot() {
    m_state = EBreakpointState::kOneShot;
}

bool CBreakpoint::IsEnabled() const {
    return m_state != EBreakpointState::kDisabled;
}

// ==================== Location Management ====================

void CBreakpoint::SetLocation(const TString& in_location) {
    m_location = in_location;
}

// ==================== Condition Management ====================

void CBreakpoint::SetCondition(const SBreakpointCondition& in_condition) {
    m_condition = in_condition;
}

// ==================== Hit Count Management ====================

void CBreakpoint::SetHitCount(TUInt32 in_count) {
    m_hitCountThreshold = in_count;
}

void CBreakpoint::ResetHitCount() {
    m_currentHits = 0;
    m_stats.totalHits = 0;
}

void CBreakpoint::RecordHit() {
    m_currentHits++;
    m_stats.totalHits++;

    auto now = std::chrono::system_clock::now();

    if (m_stats.totalHits == 1) {
        m_stats.firstHit = now;
    } else {
        // Calculate average time between hits
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_stats.firstHit).count();
        m_stats.averageTimeBetweenHits = static_cast<TDouble>(duration) / m_stats.totalHits;
    }

    m_stats.lastHit = now;

    // Handle one-shot breakpoints
    if (m_state == EBreakpointState::kOneShot) {
        Disable();
    }
}

// ==================== Data Watchpoint Management ====================

void CBreakpoint::SetDataWatchpoint(const SDataWatchpoint& in_watchpoint) {
    m_dataWatchpoint = in_watchpoint;
}

// ==================== Breakpoint Evaluation ====================

bool CBreakpoint::ShouldBreak() const {
    // Check if breakpoint is enabled
    if (m_state == EBreakpointState::kDisabled) {
        return false;
    }

    // Evaluate based on type
    switch (m_type) {
        case EBreakpointType::kStepEntry:
        case EBreakpointType::kStepExit:
            // Always break for simple entry/exit breakpoints
            return true;

        case EBreakpointType::kConditional:
            // Break only if condition evaluates to true
            if (m_condition && m_condition->evaluator) {
                try {
                    return m_condition->evaluator();
                } catch (...) {
                    // If condition evaluation throws, don't break
                    return false;
                }
            }
            return false;

        case EBreakpointType::kHitCount:
            // Break when hit count reaches threshold
            return m_currentHits >= m_hitCountThreshold;

        case EBreakpointType::kDataAccess:
            // Data access breakpoints are evaluated externally
            // when variable access is detected
            return true;

        case EBreakpointType::kException:
            // Exception breakpoints are triggered by exception handler
            return true;

        default:
            return false;
    }
}

// ==================== Serialization ====================

TString CBreakpoint::ToString() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":" << m_id << ",";
    oss << "\"type\":";

    // Convert type to string
    switch (m_type) {
        case EBreakpointType::kStepEntry:     oss << "\"StepEntry\""; break;
        case EBreakpointType::kStepExit:      oss << "\"StepExit\""; break;
        case EBreakpointType::kConditional:   oss << "\"Conditional\""; break;
        case EBreakpointType::kDataAccess:    oss << "\"DataAccess\""; break;
        case EBreakpointType::kException:     oss << "\"Exception\""; break;
        case EBreakpointType::kHitCount:      oss << "\"HitCount\""; break;
        default:                              oss << "\"Unknown\""; break;
    }
    oss << ",";

    // State
    oss << "\"state\":";
    switch (m_state) {
        case EBreakpointState::kEnabled:      oss << "\"Enabled\""; break;
        case EBreakpointState::kDisabled:     oss << "\"Disabled\""; break;
        case EBreakpointState::kOneShot:      oss << "\"OneShot\""; break;
        case EBreakpointState::kTemporary:    oss << "\"Temporary\""; break;
        default:                              oss << "\"Unknown\""; break;
    }
    oss << ",";

    // Location
    oss << "\"location\":\"" << m_location << "\",";

    // Hit count data
    oss << "\"hitCountThreshold\":" << m_hitCountThreshold << ",";
    oss << "\"currentHits\":" << m_currentHits << ",";

    // Condition (if present)
    if (m_condition) {
        oss << "\"condition\":\"" << m_condition->expression << "\",";
    }

    // Data watchpoint (if present)
    if (m_dataWatchpoint) {
        oss << "\"watchpoint\":{";
        oss << "\"variable\":\"" << m_dataWatchpoint->variableName << "\",";
        oss << "\"breakOnRead\":" << (m_dataWatchpoint->breakOnRead ? "true" : "false") << ",";
        oss << "\"breakOnWrite\":" << (m_dataWatchpoint->breakOnWrite ? "true" : "false");
        oss << "},";
    }

    // Statistics
    oss << "\"stats\":{";
    oss << "\"totalHits\":" << m_stats.totalHits << ",";
    oss << "\"averageTimeBetweenHits\":" << std::fixed << std::setprecision(2)
        << m_stats.averageTimeBetweenHits;
    oss << "}";

    oss << "}";
    return oss.str();
}

std::unique_ptr<CBreakpoint> CBreakpoint::FromString(const TString& in_str) {
    // This is a simplified implementation
    // A complete implementation would use a JSON library like nlohmann/json

    // For now, return nullptr to indicate that deserialization
    // requires proper JSON parsing library integration
    // This will be implemented in Week 2 when we integrate JSON library

    (void)in_str; // Suppress unused parameter warning
    return nullptr;
}

} // namespace TestMATE
