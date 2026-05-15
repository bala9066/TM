/**************************************************************************
 * File Name: DeviceHandler.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Semiconductor device handling for wafer and package testing.
 * Requirements: REQ-SEMI-001 to REQ-SEMI-040
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <map>
#include <set>

namespace TestMATE {

/**************************************************************************
 * Enum: EDeviceType
 * Description: Types of semiconductor devices
 **************************************************************************/
enum class EDeviceType {
    kUnknown,
    kWafer,
    kPackage,
    kModule,
    kDie
};

/**************************************************************************
 * Enum: EBinCategory
 * Description: Binning categories for device sorting
 **************************************************************************/
enum class EBinCategory {
    kPass,
    kFail,
    kRetest,
    kEngineering,
    kReference
};

/**************************************************************************
 * Struct: SDeviceInfo
 * Description: Device identification information
 **************************************************************************/
struct SDeviceInfo {
    TString deviceId;
    TString lotId;
    TString waferId;
    TInt32 dieX{0};
    TInt32 dieY{0};
    EDeviceType type{EDeviceType::kUnknown};
    TString partNumber;
    TString revision;
};

/**************************************************************************
 * Struct: SBinDefinition
 * Description: Defines a bin for device classification
 **************************************************************************/
struct SBinDefinition {
    TUInt32 binNumber{0};
    TString binName;
    EBinCategory category{EBinCategory::kFail};
    TString description;
    bool isPassBin{false};
};

/**************************************************************************
 * Struct: STestLimit
 * Description: Test limits for parametric testing
 **************************************************************************/
struct STestLimit {
    TString testName;
    TString unit;
    std::optional<TDouble> lowLimit;
    std::optional<TDouble> highLimit;
    std::optional<TDouble> nominal;
    TUInt32 failBin{0};
};

/**************************************************************************
 * Struct: SParametricResult
 * Description: Result of a parametric measurement
 **************************************************************************/
struct SParametricResult {
    TString testName;
    TDouble value{0.0};
    TString unit;
    bool passLow{true};
    bool passHigh{true};
    ETestVerdict verdict{ETestVerdict::kNone};
};

/**************************************************************************
 * Class: CDeviceHandler
 * Description: Manages semiconductor device testing operations
 **************************************************************************/
class CDeviceHandler {
public:
    CDeviceHandler();
    ~CDeviceHandler() = default;

    // Device management
    void SetDeviceInfo(const SDeviceInfo& in_info) { m_deviceInfo = in_info; }
    [[nodiscard]] const SDeviceInfo& GetDeviceInfo() const { return m_deviceInfo; }

    // Bin management
    void AddBinDefinition(const SBinDefinition& in_bin);
    [[nodiscard]] std::optional<SBinDefinition> GetBinDefinition(TUInt32 in_binNum) const;
    [[nodiscard]] TVector<SBinDefinition> GetAllBins() const;
    void ClearBins();

    // Test limits
    void AddTestLimit(const STestLimit& in_limit);
    [[nodiscard]] std::optional<STestLimit> GetTestLimit(const TString& in_strTestName) const;
    void ClearLimits();

    // Parametric testing
    SParametricResult EvaluateLimit(const TString& in_strTestName, TDouble in_fValue);
    void RecordResult(const SParametricResult& in_result);

    // Binning
    void SetDeviceBin(TUInt32 in_binNum);
    [[nodiscard]] TUInt32 GetDeviceBin() const { return m_uiCurrentBin; }
    [[nodiscard]] bool IsPassBin(TUInt32 in_binNum) const;

    // Statistics
    [[nodiscard]] TUInt32 GetTestCount() const { return static_cast<TUInt32>(m_vecResults.size()); }
    [[nodiscard]] TUInt32 GetPassCount() const;
    [[nodiscard]] TUInt32 GetFailCount() const;
    [[nodiscard]] const TVector<SParametricResult>& GetResults() const { return m_vecResults; }

    void Reset();

private:
    SDeviceInfo m_deviceInfo;
    std::map<TUInt32, SBinDefinition> m_mapBins;
    std::map<TString, STestLimit> m_mapLimits;
    TVector<SParametricResult> m_vecResults;
    TUInt32 m_uiCurrentBin{0};
};

/**************************************************************************
 * Class: CWaferMap
 * Description: Manages wafer die map for probing
 **************************************************************************/
class CWaferMap {
public:
    CWaferMap(TInt32 in_iRows, TInt32 in_iCols);
    ~CWaferMap() = default;

    // Die access
    void SetDieBin(TInt32 in_iX, TInt32 in_iY, TUInt32 in_binNum);
    [[nodiscard]] TUInt32 GetDieBin(TInt32 in_iX, TInt32 in_iY) const;

    // Navigation
    bool MoveToNextDie();
    bool MoveTo(TInt32 in_iX, TInt32 in_iY);
    [[nodiscard]] std::pair<TInt32, TInt32> GetCurrentPosition() const;

    // Statistics
    [[nodiscard]] TUInt32 GetTotalDies() const;
    [[nodiscard]] TUInt32 GetTestedDies() const;
    [[nodiscard]] TUInt32 GetPassDies() const;
    [[nodiscard]] TDouble GetYield() const;

    // Skip patterns
    void SetSkipPattern(const TVector<std::pair<TInt32, TInt32>>& in_skipList);
    [[nodiscard]] bool IsDieSkipped(TInt32 in_iX, TInt32 in_iY) const;

private:
    TInt32 m_iRows;
    TInt32 m_iCols;
    TInt32 m_iCurrentX{0};
    TInt32 m_iCurrentY{0};
    std::map<std::pair<TInt32, TInt32>, TUInt32> m_mapDieBins;
    std::set<std::pair<TInt32, TInt32>> m_setSkipDies;
};

} // namespace TestMATE
