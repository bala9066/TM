/**************************************************************************
 * File Name: StdfWriter.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: STDF file writer implementation
 * Requirements: REQ-SEMI-041 to REQ-SEMI-060
 **************************************************************************/

#include "core/semiconductor/StdfWriter.h"
#include <chrono>
#include <cstring>

namespace TestMATE {

CStdfWriter::~CStdfWriter() {
    if (m_file.is_open()) {
        Close();
    }
}

CResult CStdfWriter::Open(const TString& in_strPath) {
    if (m_file.is_open()) {
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists, "File already open");
    }

    m_file.open(in_strPath, std::ios::binary);
    if (!m_file.is_open()) {
        return TESTMATE_FAILURE(EErrorCode::kFileOpenFailed, "Failed to open file: " + in_strPath);
    }

    m_partCount = 0;
    m_goodCount = 0;
    return CResult::Success();
}

CResult CStdfWriter::Close() {
    if (m_file.is_open()) {
        m_file.close();
    }
    return CResult::Success();
}

void CStdfWriter::WriteU1(TUInt8 in_val) {
    m_recordBuffer.push_back(in_val);
}

void CStdfWriter::WriteU2(TUInt16 in_val) {
    m_recordBuffer.push_back(static_cast<TUInt8>(in_val & 0xFF));
    m_recordBuffer.push_back(static_cast<TUInt8>((in_val >> 8) & 0xFF));
}

void CStdfWriter::WriteU4(TUInt32 in_val) {
    m_recordBuffer.push_back(static_cast<TUInt8>(in_val & 0xFF));
    m_recordBuffer.push_back(static_cast<TUInt8>((in_val >> 8) & 0xFF));
    m_recordBuffer.push_back(static_cast<TUInt8>((in_val >> 16) & 0xFF));
    m_recordBuffer.push_back(static_cast<TUInt8>((in_val >> 24) & 0xFF));
}

void CStdfWriter::WriteI2(TInt16 in_val) {
    WriteU2(static_cast<TUInt16>(in_val));
}

void CStdfWriter::WriteR4(TFloat in_val) {
    TUInt32 bits;
    std::memcpy(&bits, &in_val, sizeof(TFloat));
    WriteU4(bits);
}

void CStdfWriter::WriteCn(const TString& in_str) {
    TUInt8 len = static_cast<TUInt8>(std::min(in_str.size(), static_cast<size_t>(255)));
    WriteU1(len);
    for (TUInt8 i = 0; i < len; ++i) {
        m_recordBuffer.push_back(static_cast<TUInt8>(in_str[i]));
    }
}

void CStdfWriter::WriteRecord(EStdfRecordType in_type, TUInt8 in_subType) {
    m_recordBuffer.clear();
    m_recordBuffer.reserve(256);
    // Reserve space for header (will be filled in FinalizeRecord)
    m_recordBuffer.push_back(0);  // REC_LEN low
    m_recordBuffer.push_back(0);  // REC_LEN high
    m_recordBuffer.push_back(static_cast<TUInt8>(in_type));
    m_recordBuffer.push_back(in_subType);
}

void CStdfWriter::FinalizeRecord() {
    TUInt16 recLen = static_cast<TUInt16>(m_recordBuffer.size() - 4);
    m_recordBuffer[0] = static_cast<TUInt8>(recLen & 0xFF);
    m_recordBuffer[1] = static_cast<TUInt8>((recLen >> 8) & 0xFF);
    m_file.write(reinterpret_cast<const char*>(m_recordBuffer.data()), m_recordBuffer.size());
}

CResult CStdfWriter::WriteFar(const SStdfHeader& in_header) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kFar, 10);
    WriteU1(in_header.cpuType);
    WriteU1(in_header.stdfVersion);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteMir(const SStdfMir& in_mir) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    WriteRecord(EStdfRecordType::kMir, 10);
    WriteU4(static_cast<TUInt32>(timestamp));  // SETUP_T
    WriteU4(static_cast<TUInt32>(timestamp));  // START_T
    WriteU1(1);   // STAT_NUM (station number)
    WriteU1(' '); // MODE_COD
    WriteU1(' '); // RTST_COD
    WriteU1(' '); // PROT_COD
    WriteU2(0);   // BURN_TIM
    WriteU1(' '); // CMOD_COD
    WriteCn(in_mir.lotId);
    WriteCn(in_mir.partType);
    WriteCn(in_mir.nodeNam);
    WriteCn(in_mir.testCod);
    WriteCn(in_mir.jobNam);
    WriteCn("");  // JOB_REV
    WriteCn("");  // SBLOT_ID
    WriteCn(in_mir.operNam);
    WriteCn("");  // EXEC_TYP
    WriteCn("");  // EXEC_VER
    WriteCn("");  // TEST_COD
    WriteCn("");  // TST_TEMP
    WriteCn("");  // USER_TXT
    WriteCn("");  // AUX_FILE
    WriteCn("");  // PKG_TYP
    WriteCn(in_mir.facilId);
    WriteCn(in_mir.floorId);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteMrr() {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    WriteRecord(EStdfRecordType::kMrr, 10);
    WriteU4(static_cast<TUInt32>(timestamp));  // FINISH_T
    WriteU1(' ');  // DISP_COD
    WriteCn("");   // USR_DESC
    WriteCn("");   // EXC_DESC
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WritePir(TUInt8 in_headNum, TUInt8 in_siteNum) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kPir, 10);
    WriteU1(in_headNum);
    WriteU1(in_siteNum);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WritePrr(const SStdfPrr& in_prr) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kPrr, 20);
    WriteU1(in_prr.headNum);
    WriteU1(in_prr.siteNum);
    WriteU1(in_prr.partFlag);
    WriteU2(in_prr.numTest);
    WriteU2(in_prr.hardBin);
    WriteU2(in_prr.softBin);
    WriteI2(in_prr.xCoord);
    WriteI2(in_prr.yCoord);
    WriteU4(0);  // TEST_T (test time in ms)
    WriteCn(in_prr.partId);
    WriteCn("");  // PART_TXT
    WriteCn("");  // PART_FIX
    FinalizeRecord();

    ++m_partCount;
    if (in_prr.partFlag == 0) {
        ++m_goodCount;
    }
    return CResult::Success();
}

CResult CStdfWriter::WritePtr(const SStdfPtr& in_ptr) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kPtr, 10);
    WriteU4(in_ptr.testNum);
    WriteU1(in_ptr.headNum);
    WriteU1(in_ptr.siteNum);
    WriteU1(in_ptr.testFlag);
    WriteU1(0xC0);  // PARM_FLG (result + limits valid)
    WriteR4(in_ptr.result);
    WriteCn(in_ptr.testTxt);
    WriteCn("");    // ALARM_ID
    WriteU1(0);     // OPT_FLAG
    WriteU1(0);     // RES_SCAL
    WriteU1(0);     // LLM_SCAL
    WriteU1(0);     // HLM_SCAL
    WriteR4(in_ptr.loLimit);
    WriteR4(in_ptr.hiLimit);
    WriteCn(in_ptr.units);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteHbr(TUInt16 in_binNum, TUInt32 in_count, char in_passFlag, const TString& in_name) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kHbr, 40);
    WriteU1(255);  // HEAD_NUM (all heads)
    WriteU1(255);  // SITE_NUM (all sites)
    WriteU2(in_binNum);
    WriteU4(in_count);
    WriteU1(static_cast<TUInt8>(in_passFlag));
    WriteCn(in_name);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteSbr(TUInt16 in_binNum, TUInt32 in_count, char in_passFlag, const TString& in_name) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    WriteRecord(EStdfRecordType::kSbr, 50);
    WriteU1(255);
    WriteU1(255);
    WriteU2(in_binNum);
    WriteU4(in_count);
    WriteU1(static_cast<TUInt8>(in_passFlag));
    WriteCn(in_name);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteWir(const TString& in_waferId) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    WriteRecord(EStdfRecordType::kWir, 10);
    WriteU1(1);  // HEAD_NUM
    WriteU1(0);  // SITE_GRP
    WriteU4(static_cast<TUInt32>(timestamp));
    WriteCn(in_waferId);
    FinalizeRecord();
    return CResult::Success();
}

CResult CStdfWriter::WriteWrr(TUInt32 in_partCount, TUInt32 in_goodCount) {
    if (!m_file.is_open()) return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "File not open");

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    WriteRecord(EStdfRecordType::kWrr, 20);
    WriteU1(1);  // HEAD_NUM
    WriteU1(0);  // SITE_GRP
    WriteU4(static_cast<TUInt32>(timestamp));
    WriteU4(in_partCount);
    WriteU4(0);  // RTST_CNT
    WriteU4(0);  // ABRT_CNT
    WriteU4(in_goodCount);
    WriteU4(0);  // FUNC_CNT
    WriteCn(""); // WAFER_ID
    WriteCn(""); // FABWF_ID
    WriteCn(""); // FRAME_ID
    WriteCn(""); // MASK_ID
    WriteCn(""); // USR_DESC
    WriteCn(""); // EXC_DESC
    FinalizeRecord();
    return CResult::Success();
}

} // namespace TestMATE
