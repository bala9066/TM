/**************************************************************************
 * File Name: StdfWriter.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: STDF (Standard Test Data Format) file writer
 * Requirements: REQ-SEMI-041 to REQ-SEMI-060
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <fstream>
#include <vector>

namespace TestMATE {

/**************************************************************************
 * STDF Record Types
 **************************************************************************/
enum class EStdfRecordType : TUInt8 {
    kFar = 0,   // File Attributes Record
    kMir = 1,   // Master Information Record
    kMrr = 2,   // Master Results Record
    kPcr = 3,   // Part Count Record
    kHbr = 4,   // Hardware Bin Record
    kSbr = 5,   // Software Bin Record
    kPir = 6,   // Part Information Record
    kPrr = 7,   // Part Results Record
    kPtr = 15,  // Parametric Test Record
    kFtr = 20,  // Functional Test Record
    kWir = 62,  // Wafer Information Record
    kWrr = 63   // Wafer Results Record
};

/**************************************************************************
 * STDF Data Structures
 **************************************************************************/
struct SStdfHeader {
    TUInt8 cpuType{2};      // 2 = Intel/Little-endian
    TUInt8 stdfVersion{4};  // STDF V4
};

struct SStdfMir {
    TString lotId;
    TString partType;
    TString nodeNam;
    TString testCod;
    TString jobNam;
    TString operNam;
    TString facilId;
    TString floorId;
    TTimePoint setupTime;
    TTimePoint startTime;
};

struct SStdfPtr {
    TUInt32 testNum{0};
    TUInt8 headNum{1};
    TUInt8 siteNum{1};
    TUInt8 testFlag{0};
    TFloat result{0.0f};
    TString testTxt;
    TString units;
    TFloat loLimit{0.0f};
    TFloat hiLimit{0.0f};
};

struct SStdfPrr {
    TUInt8 headNum{1};
    TUInt8 siteNum{1};
    TUInt8 partFlag{0};
    TUInt16 numTest{0};
    TUInt16 hardBin{0};
    TUInt16 softBin{0};
    TInt16 xCoord{-32768};
    TInt16 yCoord{-32768};
    TString partId;
};

/**************************************************************************
 * Class: CStdfWriter
 * Description: Writes STDF V4 formatted test data files
 **************************************************************************/
class CStdfWriter {
public:
    CStdfWriter() = default;
    ~CStdfWriter();

    [[nodiscard]] CResult Open(const TString& in_strPath);
    [[nodiscard]] CResult Close();
    [[nodiscard]] bool IsOpen() const { return m_file.is_open(); }

    // Write record types
    [[nodiscard]] CResult WriteFar(const SStdfHeader& in_header);
    [[nodiscard]] CResult WriteMir(const SStdfMir& in_mir);
    [[nodiscard]] CResult WriteMrr();
    [[nodiscard]] CResult WritePir(TUInt8 in_headNum, TUInt8 in_siteNum);
    [[nodiscard]] CResult WritePrr(const SStdfPrr& in_prr);
    [[nodiscard]] CResult WritePtr(const SStdfPtr& in_ptr);
    [[nodiscard]] CResult WriteHbr(TUInt16 in_binNum, TUInt32 in_count, char in_passFlag, const TString& in_name);
    [[nodiscard]] CResult WriteSbr(TUInt16 in_binNum, TUInt32 in_count, char in_passFlag, const TString& in_name);
    [[nodiscard]] CResult WriteWir(const TString& in_waferId);
    [[nodiscard]] CResult WriteWrr(TUInt32 in_partCount, TUInt32 in_goodCount);

private:
    void WriteU1(TUInt8 in_val);
    void WriteU2(TUInt16 in_val);
    void WriteU4(TUInt32 in_val);
    void WriteI2(TInt16 in_val);
    void WriteR4(TFloat in_val);
    void WriteCn(const TString& in_str);
    void WriteRecord(EStdfRecordType in_type, TUInt8 in_subType);
    void FinalizeRecord();

    std::ofstream m_file;
    std::vector<TUInt8> m_recordBuffer;
    TUInt32 m_partCount{0};
    TUInt32 m_goodCount{0};
};

} // namespace TestMATE
