/**************************************************************************
 * File Name: DeviceHandler.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Semiconductor device handler implementation
 * Requirements: REQ-SEMI-001 to REQ-SEMI-040
 **************************************************************************/

#include "DeviceHandler.h"
#include <algorithm>

namespace TestMATE {

//=============================================================================
// CDeviceHandler
//=============================================================================

CDeviceHandler::CDeviceHandler() {
    // Add default pass/fail bins
    SBinDefinition passBin;
    passBin.binNumber = 1;
    passBin.binName = "PASS";
    passBin.category = EBinCategory::kPass;
    passBin.isPassBin = true;
    AddBinDefinition(passBin);

    SBinDefinition failBin;
    failBin.binNumber = 0;
    failBin.binName = "FAIL";
    failBin.category = EBinCategory::kFail;
    failBin.isPassBin = false;
    AddBinDefinition(failBin);
}

void CDeviceHandler::AddBinDefinition(const SBinDefinition& in_bin) {
    m_mapBins[in_bin.binNumber] = in_bin;
}

std::optional<SBinDefinition> CDeviceHandler::GetBinDefinition(TUInt32 in_binNum) const {
    auto it = m_mapBins.find(in_binNum);
    if (it != m_mapBins.end()) {
        return it->second;
    }
    return std::nullopt;
}

TVector<SBinDefinition> CDeviceHandler::GetAllBins() const {
    TVector<SBinDefinition> bins;
    for (const auto& [num, bin] : m_mapBins) {
        bins.push_back(bin);
    }
    return bins;
}

void CDeviceHandler::ClearBins() {
    m_mapBins.clear();
}

void CDeviceHandler::AddTestLimit(const STestLimit& in_limit) {
    m_mapLimits[in_limit.testName] = in_limit;
}

std::optional<STestLimit> CDeviceHandler::GetTestLimit(const TString& in_strTestName) const {
    auto it = m_mapLimits.find(in_strTestName);
    if (it != m_mapLimits.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CDeviceHandler::ClearLimits() {
    m_mapLimits.clear();
}

SParametricResult CDeviceHandler::EvaluateLimit(const TString& in_strTestName, TDouble in_fValue) {
    SParametricResult result;
    result.testName = in_strTestName;
    result.value = in_fValue;
    result.verdict = ETestVerdict::kPass;

    auto limitOpt = GetTestLimit(in_strTestName);
    if (limitOpt) {
        const auto& limit = *limitOpt;
        result.unit = limit.unit;

        if (limit.lowLimit && in_fValue < *limit.lowLimit) {
            result.passLow = false;
            result.verdict = ETestVerdict::kFail;
        }

        if (limit.highLimit && in_fValue > *limit.highLimit) {
            result.passHigh = false;
            result.verdict = ETestVerdict::kFail;
        }
    }

    return result;
}

void CDeviceHandler::RecordResult(const SParametricResult& in_result) {
    m_vecResults.push_back(in_result);

    // Update bin if test failed
    if (in_result.verdict == ETestVerdict::kFail) {
        auto limitOpt = GetTestLimit(in_result.testName);
        if (limitOpt && limitOpt->failBin > 0) {
            SetDeviceBin(limitOpt->failBin);
        }
    }
}

void CDeviceHandler::SetDeviceBin(TUInt32 in_binNum) {
    // Only downgrade bin (fail takes priority)
    if (!IsPassBin(in_binNum) || m_uiCurrentBin == 0) {
        m_uiCurrentBin = in_binNum;
    }
}

bool CDeviceHandler::IsPassBin(TUInt32 in_binNum) const {
    auto binOpt = GetBinDefinition(in_binNum);
    return binOpt && binOpt->isPassBin;
}

TUInt32 CDeviceHandler::GetPassCount() const {
    return static_cast<TUInt32>(std::count_if(m_vecResults.begin(), m_vecResults.end(),
        [](const SParametricResult& r) { return r.verdict == ETestVerdict::kPass; }));
}

TUInt32 CDeviceHandler::GetFailCount() const {
    return static_cast<TUInt32>(std::count_if(m_vecResults.begin(), m_vecResults.end(),
        [](const SParametricResult& r) { return r.verdict == ETestVerdict::kFail; }));
}

void CDeviceHandler::Reset() {
    m_vecResults.clear();
    m_uiCurrentBin = 1;  // Default to pass bin
}

//=============================================================================
// CWaferMap
//=============================================================================

CWaferMap::CWaferMap(TInt32 in_iRows, TInt32 in_iCols)
    : m_iRows(in_iRows)
    , m_iCols(in_iCols)
{
}

void CWaferMap::SetDieBin(TInt32 in_iX, TInt32 in_iY, TUInt32 in_binNum) {
    m_mapDieBins[{in_iX, in_iY}] = in_binNum;
}

TUInt32 CWaferMap::GetDieBin(TInt32 in_iX, TInt32 in_iY) const {
    auto it = m_mapDieBins.find({in_iX, in_iY});
    return (it != m_mapDieBins.end()) ? it->second : 0xFFFFFFFF;  // Untested
}

bool CWaferMap::MoveToNextDie() {
    do {
        m_iCurrentX++;
        if (m_iCurrentX >= m_iCols) {
            m_iCurrentX = 0;
            m_iCurrentY++;
        }
        if (m_iCurrentY >= m_iRows) {
            return false;  // End of wafer
        }
    } while (IsDieSkipped(m_iCurrentX, m_iCurrentY));

    return true;
}

bool CWaferMap::MoveTo(TInt32 in_iX, TInt32 in_iY) {
    if (in_iX < 0 || in_iX >= m_iCols || in_iY < 0 || in_iY >= m_iRows) {
        return false;
    }
    m_iCurrentX = in_iX;
    m_iCurrentY = in_iY;
    return true;
}

std::pair<TInt32, TInt32> CWaferMap::GetCurrentPosition() const {
    return {m_iCurrentX, m_iCurrentY};
}

TUInt32 CWaferMap::GetTotalDies() const {
    return static_cast<TUInt32>(m_iRows * m_iCols - m_setSkipDies.size());
}

TUInt32 CWaferMap::GetTestedDies() const {
    return static_cast<TUInt32>(m_mapDieBins.size());
}

TUInt32 CWaferMap::GetPassDies() const {
    TUInt32 count = 0;
    for (const auto& [pos, bin] : m_mapDieBins) {
        if (bin == 1) ++count;  // Bin 1 is typically pass
    }
    return count;
}

TDouble CWaferMap::GetYield() const {
    TUInt32 tested = GetTestedDies();
    if (tested == 0) return 0.0;
    return static_cast<TDouble>(GetPassDies()) / tested * 100.0;
}

void CWaferMap::SetSkipPattern(const TVector<std::pair<TInt32, TInt32>>& in_skipList) {
    m_setSkipDies.clear();
    for (const auto& pos : in_skipList) {
        m_setSkipDies.insert(pos);
    }
}

bool CWaferMap::IsDieSkipped(TInt32 in_iX, TInt32 in_iY) const {
    return m_setSkipDies.count({in_iX, in_iY}) > 0;
}

} // namespace TestMATE
