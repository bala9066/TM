/**************************************************************************
 * File Name: ReportGenerator.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Report generator implementations
 * Requirements: REQ-RPT-001 to REQ-RPT-030
 **************************************************************************/

#include "IReportGenerator.h"
#include "utils/LogManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace TestMATE {

namespace {

TString TimePointToString(const TTimePoint& /*tp*/) {
    // Since TTimePoint uses steady_clock, we use current system time for display
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

TString VerdictToString(ETestVerdict verdict) {
    switch (verdict) {
        case ETestVerdict::kPass: return "PASS";
        case ETestVerdict::kFail: return "FAIL";
        case ETestVerdict::kSkipped: return "SKIP";
        case ETestVerdict::kError: return "ERROR";
        case ETestVerdict::kAborted: return "ABORT";
        default: return "NONE";
    }
}

TString EscapeJson(const TString& str) {
    TString result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

TString EscapeCsv(const TString& str, char delim) {
    bool needsQuotes = str.find(delim) != TString::npos ||
                       str.find('"') != TString::npos ||
                       str.find('\n') != TString::npos;
    if (!needsQuotes) return str;

    TString result = "\"";
    for (char c : str) {
        if (c == '"') result += "\"\"";
        else result += c;
    }
    result += "\"";
    return result;
}

} // anonymous namespace

//=============================================================================
// CTextReportGenerator
//=============================================================================

CResult CTextReportGenerator::Generate(const STestReport& in_report,
                                        const TString& in_strOutputPath) {
    TString content;
    auto result = GenerateToString(in_report, content);
    if (result.IsFailure()) return result;

    std::ofstream file(in_strOutputPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to write report: " + in_strOutputPath);
    }
    file << content;
    return TESTMATE_SUCCESS();
}

CResult CTextReportGenerator::GenerateToString(const STestReport& in_report,
                                                TString& out_strContent) {
    std::ostringstream oss;

    oss << "================================================================================\n";
    oss << "                          TEST REPORT\n";
    oss << "================================================================================\n\n";

    oss << "Report ID:      " << in_report.reportId << "\n";
    oss << "Sequence:       " << in_report.sequenceName << "\n";
    oss << "Operator:       " << in_report.operatorName << "\n";
    oss << "Lot ID:         " << in_report.lotId << "\n";
    oss << "Serial Number:  " << in_report.serialNumber << "\n";
    oss << "Start Time:     " << TimePointToString(in_report.startTime) << "\n";
    oss << "End Time:       " << TimePointToString(in_report.endTime) << "\n";
    oss << "Duration:       " << in_report.totalDurationMs << " ms\n\n";

    oss << "--------------------------------------------------------------------------------\n";
    oss << "                          SUMMARY\n";
    oss << "--------------------------------------------------------------------------------\n";
    oss << "Overall Verdict: " << VerdictToString(in_report.overallVerdict) << "\n";
    oss << "Total Steps:     " << in_report.totalSteps << "\n";
    oss << "Passed:          " << in_report.passCount << "\n";
    oss << "Failed:          " << in_report.failCount << "\n";
    oss << "Skipped:         " << in_report.skipCount << "\n\n";

    oss << "--------------------------------------------------------------------------------\n";
    oss << "                          RESULTS\n";
    oss << "--------------------------------------------------------------------------------\n\n";

    for (const auto& result : in_report.results) {
        oss << "Step: " << result.stepName << " [" << result.stepId << "]\n";
        oss << "  Verdict:  " << VerdictToString(result.verdict) << "\n";
        oss << "  Duration: " << result.durationMs << " ms\n";
        if (!result.message.empty()) {
            oss << "  Message:  " << result.message << "\n";
        }
        for (const auto& [key, value] : result.measurements) {
            oss << "  " << key << ": " << value << "\n";
        }
        oss << "\n";
    }

    oss << "================================================================================\n";

    out_strContent = oss.str();
    return TESTMATE_SUCCESS();
}

//=============================================================================
// CJsonReportGenerator
//=============================================================================

CResult CJsonReportGenerator::Generate(const STestReport& in_report,
                                        const TString& in_strOutputPath) {
    TString content;
    auto result = GenerateToString(in_report, content);
    if (result.IsFailure()) return result;

    std::ofstream file(in_strOutputPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to write report: " + in_strOutputPath);
    }
    file << content;
    return TESTMATE_SUCCESS();
}

CResult CJsonReportGenerator::GenerateToString(const STestReport& in_report,
                                                TString& out_strContent) {
    std::ostringstream oss;
    TString indent = m_bPrettyPrint ? "  " : "";
    TString nl = m_bPrettyPrint ? "\n" : "";

    oss << "{" << nl;
    oss << indent << "\"reportId\": \"" << EscapeJson(in_report.reportId) << "\"," << nl;
    oss << indent << "\"sequenceName\": \"" << EscapeJson(in_report.sequenceName) << "\"," << nl;
    oss << indent << "\"operatorName\": \"" << EscapeJson(in_report.operatorName) << "\"," << nl;
    oss << indent << "\"lotId\": \"" << EscapeJson(in_report.lotId) << "\"," << nl;
    oss << indent << "\"serialNumber\": \"" << EscapeJson(in_report.serialNumber) << "\"," << nl;
    oss << indent << "\"startTime\": \"" << TimePointToString(in_report.startTime) << "\"," << nl;
    oss << indent << "\"endTime\": \"" << TimePointToString(in_report.endTime) << "\"," << nl;
    oss << indent << "\"totalDurationMs\": " << in_report.totalDurationMs << "," << nl;
    oss << indent << "\"overallVerdict\": \"" << VerdictToString(in_report.overallVerdict) << "\"," << nl;
    oss << indent << "\"totalSteps\": " << in_report.totalSteps << "," << nl;
    oss << indent << "\"passCount\": " << in_report.passCount << "," << nl;
    oss << indent << "\"failCount\": " << in_report.failCount << "," << nl;
    oss << indent << "\"skipCount\": " << in_report.skipCount << "," << nl;

    oss << indent << "\"results\": [" << nl;
    for (size_t i = 0; i < in_report.results.size(); ++i) {
        const auto& r = in_report.results[i];
        oss << indent << indent << "{" << nl;
        oss << indent << indent << indent << "\"stepId\": \"" << EscapeJson(r.stepId) << "\"," << nl;
        oss << indent << indent << indent << "\"stepName\": \"" << EscapeJson(r.stepName) << "\"," << nl;
        oss << indent << indent << indent << "\"verdict\": \"" << VerdictToString(r.verdict) << "\"," << nl;
        oss << indent << indent << indent << "\"durationMs\": " << r.durationMs << "," << nl;
        oss << indent << indent << indent << "\"message\": \"" << EscapeJson(r.message) << "\"" << nl;
        oss << indent << indent << "}" << (i < in_report.results.size() - 1 ? "," : "") << nl;
    }
    oss << indent << "]" << nl;
    oss << "}" << nl;

    out_strContent = oss.str();
    return TESTMATE_SUCCESS();
}

//=============================================================================
// CCsvReportGenerator
//=============================================================================

CResult CCsvReportGenerator::Generate(const STestReport& in_report,
                                       const TString& in_strOutputPath) {
    TString content;
    auto result = GenerateToString(in_report, content);
    if (result.IsFailure()) return result;

    std::ofstream file(in_strOutputPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed,
                                "Failed to write report: " + in_strOutputPath);
    }
    file << content;
    return TESTMATE_SUCCESS();
}

CResult CCsvReportGenerator::GenerateToString(const STestReport& in_report,
                                               TString& out_strContent) {
    std::ostringstream oss;

    // Header
    oss << "StepId" << m_cDelimiter << "StepName" << m_cDelimiter
        << "Verdict" << m_cDelimiter << "DurationMs" << m_cDelimiter << "Message\n";

    // Data rows
    for (const auto& r : in_report.results) {
        oss << EscapeCsv(r.stepId, m_cDelimiter) << m_cDelimiter
            << EscapeCsv(r.stepName, m_cDelimiter) << m_cDelimiter
            << VerdictToString(r.verdict) << m_cDelimiter
            << r.durationMs << m_cDelimiter
            << EscapeCsv(r.message, m_cDelimiter) << "\n";
    }

    out_strContent = oss.str();
    return TESTMATE_SUCCESS();
}

//=============================================================================
// CReportManager
//=============================================================================

CReportManager& CReportManager::GetInstance() {
    static CReportManager instance;
    return instance;
}

CReportManager::CReportManager() {
    // Register default generators
    RegisterGenerator(std::make_shared<CTextReportGenerator>());
    RegisterGenerator(std::make_shared<CJsonReportGenerator>());
    RegisterGenerator(std::make_shared<CCsvReportGenerator>());
}

void CReportManager::RegisterGenerator(TSharedPtr<IReportGenerator> in_pGenerator) {
    if (in_pGenerator) {
        m_mapGenerators[in_pGenerator->GetFormat()] = in_pGenerator;
    }
}

IReportGenerator* CReportManager::GetGenerator(EReportFormat in_eFormat) {
    auto it = m_mapGenerators.find(in_eFormat);
    return (it != m_mapGenerators.end()) ? it->second.get() : nullptr;
}

TVector<EReportFormat> CReportManager::GetAvailableFormats() const {
    TVector<EReportFormat> formats;
    for (const auto& [format, gen] : m_mapGenerators) {
        formats.push_back(format);
    }
    return formats;
}

CResult CReportManager::GenerateReport(const STestReport& in_report,
                                        EReportFormat in_eFormat,
                                        const TString& in_strOutputPath) {
    IReportGenerator* pGen = GetGenerator(in_eFormat);
    if (!pGen) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Report format not supported");
    }
    return pGen->Generate(in_report, in_strOutputPath);
}

STestReport CReportManager::CreateReport(const TString& in_strSequenceName) {
    STestReport report;
    report.reportId = "RPT-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    report.sequenceName = in_strSequenceName;
    report.startTime = std::chrono::steady_clock::now();
    return report;
}

void CReportManager::AddResult(STestReport& io_report, const STestResult& in_result) {
    io_report.results.push_back(in_result);
    io_report.totalSteps++;

    switch (in_result.verdict) {
        case ETestVerdict::kPass: io_report.passCount++; break;
        case ETestVerdict::kFail: io_report.failCount++; break;
        case ETestVerdict::kSkipped: io_report.skipCount++; break;
        default: break;
    }
}

void CReportManager::FinalizeReport(STestReport& io_report) {
    io_report.endTime = std::chrono::steady_clock::now();
    io_report.totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        io_report.endTime - io_report.startTime).count();

    io_report.overallVerdict = (io_report.failCount == 0)
        ? ETestVerdict::kPass : ETestVerdict::kFail;
}

} // namespace TestMATE
