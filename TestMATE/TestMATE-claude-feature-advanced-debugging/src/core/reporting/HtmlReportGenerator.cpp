/**************************************************************************
 * File Name: HtmlReportGenerator.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: HTML report generator implementation
 * Requirements: REQ-RPT-031 to REQ-RPT-040
 **************************************************************************/

#include "core/reporting/HtmlReportGenerator.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace TestMATE {

namespace {
// Escapes text for safe inclusion in HTML element content and double-quoted
// attributes; prevents stored XSS from operator/lot/step fields.
TString EscapeHtml(const TString& in_text) {
    TString out;
    out.reserve(in_text.size());
    for (char c : in_text) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}
} // namespace

CResult CHtmlReportGenerator::Generate(const STestReport& in_report, const TString& in_strOutputPath) {
    TString content;
    auto result = GenerateToString(in_report, content);
    if (!result.IsSuccess()) {
        return result;
    }

    std::ofstream file(in_strOutputPath);
    if (!file.is_open()) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Failed to open file: " + in_strOutputPath);
    }

    file << content;
    file.close();
    return CResult::Success();
}

CResult CHtmlReportGenerator::GenerateToString(const STestReport& in_report, TString& out_strContent) {
    std::ostringstream oss;

    oss << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    oss << "    <meta charset=\"UTF-8\">\n";
    oss << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    oss << "    <title>" << EscapeHtml(m_strTitle) << "</title>\n";
    oss << GenerateStyles();
    oss << "</head>\n<body>\n";
    oss << GenerateHeader();
    oss << GenerateSummarySection(in_report);
    oss << GenerateResultsTable(in_report);
    oss << GenerateFooter();
    oss << "</body>\n</html>";

    out_strContent = oss.str();
    return CResult::Success();
}

TString CHtmlReportGenerator::GenerateStyles() {
    return R"(
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Arial, sans-serif; background: #f5f5f5; color: #333; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .header { background: linear-gradient(135deg, #2c3e50, #3498db); color: white; padding: 30px; border-radius: 8px 8px 0 0; }
        .header h1 { font-size: 28px; margin-bottom: 10px; }
        .summary { background: white; padding: 20px; border-bottom: 1px solid #ddd; display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; }
        .summary-item { text-align: center; padding: 15px; background: #f8f9fa; border-radius: 8px; }
        .summary-item .value { font-size: 32px; font-weight: bold; }
        .summary-item .label { color: #666; font-size: 14px; margin-top: 5px; }
        .pass { color: #27ae60; }
        .fail { color: #e74c3c; }
        .skip { color: #f39c12; }
        table { width: 100%; border-collapse: collapse; background: white; }
        th, td { padding: 12px 15px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background: #34495e; color: white; font-weight: 600; }
        tr:hover { background: #f5f5f5; }
        .verdict-pass { background: #d4edda; color: #155724; padding: 4px 12px; border-radius: 4px; }
        .verdict-fail { background: #f8d7da; color: #721c24; padding: 4px 12px; border-radius: 4px; }
        .verdict-skip { background: #fff3cd; color: #856404; padding: 4px 12px; border-radius: 4px; }
        .footer { background: #2c3e50; color: white; padding: 20px; text-align: center; border-radius: 0 0 8px 8px; }
    </style>
)";
}

TString CHtmlReportGenerator::GenerateHeader() {
    std::ostringstream oss;
    oss << "<div class=\"container\">\n";
    oss << "<div class=\"header\">\n";
    if (!m_strLogoPath.empty()) {
        oss << "    <img src=\"" << EscapeHtml(m_strLogoPath) << "\" alt=\"Logo\" style=\"height:50px;margin-bottom:10px;\">\n";
    }
    oss << "    <h1>" << EscapeHtml(m_strTitle) << "</h1>\n";
    oss << "    <p>" << EscapeHtml(m_strCompanyName) << "</p>\n";
    oss << "</div>\n";
    return oss.str();
}

TString CHtmlReportGenerator::GenerateSummarySection(const STestReport& in_report) {
    std::ostringstream oss;
    TDouble passRate = in_report.totalSteps > 0
        ? (static_cast<TDouble>(in_report.passCount) / in_report.totalSteps * 100.0) : 0.0;

    oss << "<div class=\"summary\">\n";
    oss << "    <div class=\"summary-item\"><div class=\"value\">" << in_report.totalSteps << "</div><div class=\"label\">Total Tests</div></div>\n";
    oss << "    <div class=\"summary-item\"><div class=\"value pass\">" << in_report.passCount << "</div><div class=\"label\">Passed</div></div>\n";
    oss << "    <div class=\"summary-item\"><div class=\"value fail\">" << in_report.failCount << "</div><div class=\"label\">Failed</div></div>\n";
    oss << "    <div class=\"summary-item\"><div class=\"value\">" << std::fixed << std::setprecision(1) << passRate << "%</div><div class=\"label\">Pass Rate</div></div>\n";
    oss << "    <div class=\"summary-item\"><div class=\"value " << VerdictToClass(in_report.overallVerdict) << "\">" << VerdictToString(in_report.overallVerdict) << "</div><div class=\"label\">Overall</div></div>\n";
    oss << "</div>\n";

    oss << "<div style=\"background:white;padding:20px;border-bottom:1px solid #ddd;\">\n";
    oss << "    <p><strong>Report ID:</strong> " << EscapeHtml(in_report.reportId) << "</p>\n";
    oss << "    <p><strong>Sequence:</strong> " << EscapeHtml(in_report.sequenceName) << "</p>\n";
    oss << "    <p><strong>Operator:</strong> " << EscapeHtml(in_report.operatorName) << "</p>\n";
    oss << "    <p><strong>Lot ID:</strong> " << EscapeHtml(in_report.lotId) << "</p>\n";
    oss << "    <p><strong>Serial Number:</strong> " << EscapeHtml(in_report.serialNumber) << "</p>\n";
    oss << "</div>\n";

    return oss.str();
}

TString CHtmlReportGenerator::GenerateResultsTable(const STestReport& in_report) {
    std::ostringstream oss;
    oss << "<table>\n";
    oss << "    <thead><tr><th>Step ID</th><th>Name</th><th>Verdict</th><th>Duration (ms)</th></tr></thead>\n";
    oss << "    <tbody>\n";

    for (const auto& result : in_report.results) {
        oss << "    <tr>\n";
        oss << "        <td>" << EscapeHtml(result.stepId) << "</td>\n";
        oss << "        <td>" << EscapeHtml(result.stepName) << "</td>\n";
        oss << "        <td><span class=\"verdict-" << VerdictToClass(result.verdict) << "\">" << VerdictToString(result.verdict) << "</span></td>\n";
        oss << "        <td>" << result.durationMs << "</td>\n";
        oss << "    </tr>\n";
    }

    oss << "    </tbody>\n</table>\n";
    return oss.str();
}

TString CHtmlReportGenerator::GenerateFooter() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << "<div class=\"footer\">\n";
    oss << "    <p>Generated by TestMATE on " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "</p>\n";
    oss << "</div>\n</div>\n";
    return oss.str();
}

TString CHtmlReportGenerator::VerdictToClass(ETestVerdict in_verdict) {
    switch (in_verdict) {
        case ETestVerdict::kPass: return "pass";
        case ETestVerdict::kFail: return "fail";
        case ETestVerdict::kSkipped: return "skip";
        default: return "";
    }
}

TString CHtmlReportGenerator::VerdictToString(ETestVerdict in_verdict) {
    switch (in_verdict) {
        case ETestVerdict::kPass: return "PASS";
        case ETestVerdict::kFail: return "FAIL";
        case ETestVerdict::kSkipped: return "SKIP";
        default: return "UNKNOWN";
    }
}

TString CHtmlReportGenerator::FormatTimestamp(const TWallClock& in_tp) {
    auto time = std::chrono::system_clock::to_time_t(in_tp);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &time);
#else
    localtime_r(&time, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace TestMATE
