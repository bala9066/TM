/**************************************************************************
 * File Name: DataStore.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Data store and analytics implementations
 * Requirements: REQ-DB-001 to REQ-DB-030
 **************************************************************************/

#include "IDataStore.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace TestMATE {

//=============================================================================
// CMemoryDataStore
//=============================================================================

CResult CMemoryDataStore::Connect(const TString& /*in_strConnectionString*/) {
    m_bConnected = true;
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::Disconnect() {
    m_bConnected = false;
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::InitializeSchema() {
    // Memory store doesn't need schema initialization
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    out_id = m_uiNextId++;
    STestDataRecord record = in_record;
    record.recordId = out_id;
    m_mapRecords[out_id] = record;

    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::UpdateTestData(const STestDataRecord& in_record) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    auto it = m_mapRecords.find(in_record.recordId);
    if (it == m_mapRecords.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Record not found");
    }

    it->second = in_record;
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::DeleteTestData(TUInt64 in_recordId) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    auto it = m_mapRecords.find(in_recordId);
    if (it == m_mapRecords.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Record not found");
    }

    m_mapRecords.erase(it);
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::GetTestData(TUInt64 in_recordId, STestDataRecord& out_record) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    auto it = m_mapRecords.find(in_recordId);
    if (it == m_mapRecords.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Record not found");
    }

    out_record = it->second;
    return TESTMATE_SUCCESS();
}

SQueryResult CMemoryDataStore::Query(const TString& /*in_strQuery*/) {
    SQueryResult result;
    result.success = true;
    // Memory store doesn't support SQL queries
    return result;
}

CResult CMemoryDataStore::GetTestDataByLot(const TString& in_strLotId,
                                            TVector<STestDataRecord>& out_records) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    out_records.clear();
    for (const auto& [id, record] : m_mapRecords) {
        if (record.lotId == in_strLotId) {
            out_records.push_back(record);
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::GetTestDataByDateRange(const TTimePoint& in_start,
                                                  const TTimePoint& in_end,
                                                  TVector<STestDataRecord>& out_records) {
    if (!m_bConnected) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Not connected");
    }

    out_records.clear();
    for (const auto& [id, record] : m_mapRecords) {
        if (record.timestamp >= in_start && record.timestamp <= in_end) {
            out_records.push_back(record);
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::BeginTransaction() {
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::CommitTransaction() {
    return TESTMATE_SUCCESS();
}

CResult CMemoryDataStore::RollbackTransaction() {
    return TESTMATE_SUCCESS();
}

//=============================================================================
// CAnalytics
//=============================================================================

TDouble CAnalytics::CalculateMean(const TVector<TDouble>& in_vecValues) {
    if (in_vecValues.empty()) return 0.0;
    return std::accumulate(in_vecValues.begin(), in_vecValues.end(), 0.0) / in_vecValues.size();
}

TDouble CAnalytics::CalculateStdDev(const TVector<TDouble>& in_vecValues) {
    if (in_vecValues.size() < 2) return 0.0;

    TDouble mean = CalculateMean(in_vecValues);
    TDouble sumSq = 0.0;

    for (TDouble val : in_vecValues) {
        TDouble diff = val - mean;
        sumSq += diff * diff;
    }

    return std::sqrt(sumSq / (in_vecValues.size() - 1));
}

TDouble CAnalytics::CalculateMin(const TVector<TDouble>& in_vecValues) {
    if (in_vecValues.empty()) return 0.0;
    return *std::min_element(in_vecValues.begin(), in_vecValues.end());
}

TDouble CAnalytics::CalculateMax(const TVector<TDouble>& in_vecValues) {
    if (in_vecValues.empty()) return 0.0;
    return *std::max_element(in_vecValues.begin(), in_vecValues.end());
}

TDouble CAnalytics::CalculateMedian(TVector<TDouble> in_vecValues) {
    if (in_vecValues.empty()) return 0.0;

    std::sort(in_vecValues.begin(), in_vecValues.end());
    size_t n = in_vecValues.size();

    if (n % 2 == 0) {
        return (in_vecValues[n/2 - 1] + in_vecValues[n/2]) / 2.0;
    }
    return in_vecValues[n/2];
}

TDouble CAnalytics::CalculateYield(TUInt32 in_pass, TUInt32 in_total) {
    if (in_total == 0) return 0.0;
    return static_cast<TDouble>(in_pass) / in_total * 100.0;
}

TDouble CAnalytics::CalculateFPY(const TVector<ETestVerdict>& in_verdicts) {
    if (in_verdicts.empty()) return 0.0;

    TUInt32 pass = 0;
    for (ETestVerdict v : in_verdicts) {
        if (v == ETestVerdict::kPass) ++pass;
    }

    return CalculateYield(pass, static_cast<TUInt32>(in_verdicts.size()));
}

TDouble CAnalytics::CalculateCpk(TDouble in_mean, TDouble in_stdDev,
                                   TDouble in_lsl, TDouble in_usl) {
    if (in_stdDev == 0.0) return 0.0;

    TDouble cpuVal = (in_usl - in_mean) / (3.0 * in_stdDev);
    TDouble cplVal = (in_mean - in_lsl) / (3.0 * in_stdDev);

    return std::min(cpuVal, cplVal);
}

TVector<CAnalytics::SParetoItem> CAnalytics::CalculatePareto(
    const std::map<TString, TUInt32>& in_mapCounts) {

    TVector<SParetoItem> items;
    TUInt32 total = 0;

    for (const auto& [cat, count] : in_mapCounts) {
        total += count;
        SParetoItem item;
        item.category = cat;
        item.count = count;
        items.push_back(item);
    }

    // Sort descending by count
    std::sort(items.begin(), items.end(),
        [](const SParetoItem& a, const SParetoItem& b) {
            return a.count > b.count;
        });

    // Calculate percentages
    TDouble cumulative = 0.0;
    for (auto& item : items) {
        item.percentage = (total > 0) ? static_cast<TDouble>(item.count) / total * 100.0 : 0.0;
        cumulative += item.percentage;
        item.cumulative = cumulative;
    }

    return items;
}

//=============================================================================
// CDataManager
//=============================================================================

CDataManager& CDataManager::GetInstance() {
    static CDataManager instance;
    return instance;
}

void CDataManager::SetDataStore(TSharedPtr<IDataStore> in_pStore) {
    m_pDataStore = in_pStore;
}

CResult CDataManager::SaveTestReport(const STestReport& in_report) {
    if (!m_pDataStore || !m_pDataStore->IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "No data store connected");
    }

    STestDataRecord record;
    record.sequenceName = in_report.sequenceName;
    record.lotId = in_report.lotId;
    record.deviceId = in_report.serialNumber;
    record.timestamp = in_report.startTime;
    record.verdict = in_report.overallVerdict;
    record.durationMs = in_report.totalDurationMs;

    record.metadata["operator"] = in_report.operatorName;
    record.metadata["reportId"] = in_report.reportId;
    record.metadata["passCount"] = std::to_string(in_report.passCount);
    record.metadata["failCount"] = std::to_string(in_report.failCount);

    TUInt64 id;
    return m_pDataStore->InsertTestData(record, id);
}

CResult CDataManager::LoadTestReport(TUInt64 in_recordId, STestReport& out_report) {
    if (!m_pDataStore || !m_pDataStore->IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "No data store connected");
    }

    STestDataRecord record;
    auto result = m_pDataStore->GetTestData(in_recordId, record);
    if (result.IsFailure()) return result;

    out_report.sequenceName = record.sequenceName;
    out_report.lotId = record.lotId;
    out_report.serialNumber = record.deviceId;
    out_report.startTime = record.timestamp;
    out_report.overallVerdict = record.verdict;
    out_report.totalDurationMs = record.durationMs;

    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
