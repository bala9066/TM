/**************************************************************************
 * File Name: IDataStore.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Database abstraction layer for test data storage.
 * Requirements: REQ-DB-001 to REQ-DB-030
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include "core/reporting/IReportGenerator.h"
#include <functional>

namespace TestMATE {

/**************************************************************************
 * Enum: EDataStoreType
 * Description: Types of data stores supported
 **************************************************************************/
enum class EDataStoreType {
    kSqlite,
    kPostgreSQL,
    kMySQL,
    kMemory,
    kFile
};

/**************************************************************************
 * Struct: SQueryResult
 * Description: Result of a database query
 **************************************************************************/
struct SQueryResult {
    bool success{false};
    TString errorMessage;
    TVector<TVector<TString>> rows;
    TVector<TString> columnNames;
    TUInt64 affectedRows{0};
    TUInt64 lastInsertId{0};
};

/**************************************************************************
 * Struct: STestDataRecord
 * Description: Test data record for storage
 **************************************************************************/
struct STestDataRecord {
    TUInt64 recordId{0};
    TString sequenceName;
    TString deviceId;
    TString lotId;
    TTimePoint timestamp;
    ETestVerdict verdict{ETestVerdict::kNone};
    TInt64 durationMs{0};
    std::map<TString, TString> measurements;
    std::map<TString, TString> metadata;
};

/**************************************************************************
 * Interface: IDataStore
 * Description: Abstract interface for data storage backends
 **************************************************************************/
class IDataStore {
public:
    virtual ~IDataStore() = default;

    // Connection
    virtual CResult Connect(const TString& in_strConnectionString) = 0;
    virtual CResult Disconnect() = 0;
    [[nodiscard]] virtual bool IsConnected() const = 0;

    // Schema
    virtual CResult InitializeSchema() = 0;

    // CRUD Operations
    virtual CResult InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) = 0;
    virtual CResult UpdateTestData(const STestDataRecord& in_record) = 0;
    virtual CResult DeleteTestData(TUInt64 in_recordId) = 0;
    virtual CResult GetTestData(TUInt64 in_recordId, STestDataRecord& out_record) = 0;

    // Queries
    virtual SQueryResult Query(const TString& in_strQuery) = 0;
    virtual CResult GetTestDataByLot(const TString& in_strLotId,
                                      TVector<STestDataRecord>& out_records) = 0;
    virtual CResult GetTestDataByDateRange(const TTimePoint& in_start,
                                           const TTimePoint& in_end,
                                           TVector<STestDataRecord>& out_records) = 0;

    // Transactions
    virtual CResult BeginTransaction() = 0;
    virtual CResult CommitTransaction() = 0;
    virtual CResult RollbackTransaction() = 0;
};

/**************************************************************************
 * Class: CMemoryDataStore
 * Description: In-memory data store for testing
 **************************************************************************/
class CMemoryDataStore : public IDataStore {
public:
    CResult Connect(const TString& in_strConnectionString) override;
    CResult Disconnect() override;
    [[nodiscard]] bool IsConnected() const override { return m_bConnected; }

    CResult InitializeSchema() override;

    CResult InsertTestData(const STestDataRecord& in_record, TUInt64& out_id) override;
    CResult UpdateTestData(const STestDataRecord& in_record) override;
    CResult DeleteTestData(TUInt64 in_recordId) override;
    CResult GetTestData(TUInt64 in_recordId, STestDataRecord& out_record) override;

    SQueryResult Query(const TString& in_strQuery) override;
    CResult GetTestDataByLot(const TString& in_strLotId,
                              TVector<STestDataRecord>& out_records) override;
    CResult GetTestDataByDateRange(const TTimePoint& in_start,
                                   const TTimePoint& in_end,
                                   TVector<STestDataRecord>& out_records) override;

    CResult BeginTransaction() override;
    CResult CommitTransaction() override;
    CResult RollbackTransaction() override;

    // Test helpers
    [[nodiscard]] TUInt64 GetRecordCount() const { return static_cast<TUInt64>(m_mapRecords.size()); }

private:
    bool m_bConnected{false};
    TUInt64 m_uiNextId{1};
    std::map<TUInt64, STestDataRecord> m_mapRecords;
};

/**************************************************************************
 * Class: CAnalytics
 * Description: Analytics engine for test data analysis
 **************************************************************************/
class CAnalytics {
public:
    // Statistical functions
    static TDouble CalculateMean(const TVector<TDouble>& in_vecValues);
    static TDouble CalculateStdDev(const TVector<TDouble>& in_vecValues);
    static TDouble CalculateMin(const TVector<TDouble>& in_vecValues);
    static TDouble CalculateMax(const TVector<TDouble>& in_vecValues);
    static TDouble CalculateMedian(TVector<TDouble> in_vecValues);

    // Yield calculations
    static TDouble CalculateYield(TUInt32 in_pass, TUInt32 in_total);
    static TDouble CalculateFPY(const TVector<ETestVerdict>& in_verdicts);

    // Cpk analysis
    static TDouble CalculateCpk(TDouble in_mean, TDouble in_stdDev,
                                  TDouble in_lsl, TDouble in_usl);

    // Pareto analysis
    struct SParetoItem {
        TString category;
        TUInt32 count{0};
        TDouble percentage{0.0};
        TDouble cumulative{0.0};
    };
    static TVector<SParetoItem> CalculatePareto(
        const std::map<TString, TUInt32>& in_mapCounts);
};

/**************************************************************************
 * Class: CDataManager
 * Description: Singleton for data storage management
 **************************************************************************/
class CDataManager {
public:
    static CDataManager& GetInstance();

    CDataManager(const CDataManager&) = delete;
    CDataManager& operator=(const CDataManager&) = delete;

    void SetDataStore(TSharedPtr<IDataStore> in_pStore);
    [[nodiscard]] IDataStore* GetDataStore() const { return m_pDataStore.get(); }

    // High-level operations
    CResult SaveTestReport(const STestReport& in_report);
    CResult LoadTestReport(TUInt64 in_recordId, STestReport& out_report);

private:
    CDataManager() = default;
    ~CDataManager() = default;

    TSharedPtr<IDataStore> m_pDataStore;
};

} // namespace TestMATE
