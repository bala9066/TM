/**************************************************************************
 * File Name: SequenceFileIO.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Sequence file I/O implementation
 **************************************************************************/

#include "SequenceFileIO.h"
#include "utils/LogManager.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace TestMATE {

// Simple test step for loaded sequences
class CLoadedTestStep : public CTestStepBase {
public:
    CLoadedTestStep(const TString& id, const TString& name, EStepType type)
        : CTestStepBase(id, name, type) {}

    CResult Execute(SStepResult& out_result) override {
        out_result.verdict = ETestVerdict::kPass;
        out_result.message = "Executed: " + GetName();
        return TESTMATE_SUCCESS();
    }
};

//=============================================================================
// CJsonSequenceParser
//=============================================================================

CResult CJsonSequenceParser::Parse(const TString& in_strContent, CTestSequence& out_sequence) {
    out_sequence.Clear();

    // Simple JSON parsing (production would use proper JSON library)
    std::regex idRegex(R"xxx("id"\s*:\s*"([^"]*)")xxx");
    std::regex nameRegex(R"xxx("name"\s*:\s*"([^"]*)")xxx");
    std::regex versionRegex(R"xxx("version"\s*:\s*"([^"]*)")xxx");
    std::regex stepsRegex(R"xxx("steps"\s*:\s*\[([^\]]*)\])xxx");

    std::smatch match;

    SSequenceInfo info;
    if (std::regex_search(in_strContent, match, idRegex)) {
        info.id = match[1].str();
    }
    if (std::regex_search(in_strContent, match, nameRegex)) {
        info.name = match[1].str();
    }
    if (std::regex_search(in_strContent, match, versionRegex)) {
        info.version = match[1].str();
    }
    out_sequence.SetInfo(info);

    // Parse steps
    if (std::regex_search(in_strContent, match, stepsRegex)) {
        TString stepsContent = match[1].str();
        std::regex stepRegex(R"xxx(\{[^}]+\})xxx");
        auto stepsBegin = std::sregex_iterator(stepsContent.begin(), stepsContent.end(), stepRegex);
        auto stepsEnd = std::sregex_iterator();

        for (auto it = stepsBegin; it != stepsEnd; ++it) {
            TUniquePtr<ITestStep> pStep;
            if (ParseStep(it->str(), pStep).IsSuccess() && pStep) {
                out_sequence.AddStep(std::move(pStep));
            }
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CJsonSequenceParser::ParseStep(const TString& in_strStepJson, TUniquePtr<ITestStep>& out_pStep) {
    std::regex idRegex(R"xxx("id"\s*:\s*"([^"]*)")xxx");
    std::regex nameRegex(R"xxx("name"\s*:\s*"([^"]*)")xxx");
    std::regex typeRegex(R"xxx("type"\s*:\s*"([^"]*)")xxx");

    std::smatch match;
    TString id, name;
    EStepType type = EStepType::kAction;

    if (std::regex_search(in_strStepJson, match, idRegex)) {
        id = match[1].str();
    }
    if (std::regex_search(in_strStepJson, match, nameRegex)) {
        name = match[1].str();
    }
    if (std::regex_search(in_strStepJson, match, typeRegex)) {
        TString typeStr = match[1].str();
        if (typeStr == "validation") type = EStepType::kValidation;
        else if (typeStr == "measurement") type = EStepType::kMeasurement;
        else if (typeStr == "wait") type = EStepType::kWait;
    }

    if (!id.empty()) {
        out_pStep = std::make_unique<CLoadedTestStep>(id, name, type);
    }

    return TESTMATE_SUCCESS();
}

CResult CJsonSequenceParser::Serialize(const CTestSequence& in_sequence, TString& out_strContent) {
    std::ostringstream oss;
    const auto& info = in_sequence.GetInfo();

    oss << "{\n";
    oss << "  \"id\": \"" << info.id << "\",\n";
    oss << "  \"name\": \"" << info.name << "\",\n";
    oss << "  \"version\": \"" << info.version << "\",\n";
    oss << "  \"steps\": [\n";

    for (TUInt32 i = 0; i < in_sequence.GetStepCount(); ++i) {
        const ITestStep* pStep = in_sequence.GetStep(i);
        if (pStep) {
            if (i > 0) oss << ",\n";
            oss << "    " << SerializeStep(pStep);
        }
    }

    oss << "\n  ]\n";
    oss << "}\n";

    out_strContent = oss.str();
    return TESTMATE_SUCCESS();
}

TString CJsonSequenceParser::SerializeStep(const ITestStep* in_pStep) {
    std::ostringstream oss;
    TString typeStr = "action";
    switch (in_pStep->GetType()) {
        case EStepType::kValidation: typeStr = "validation"; break;
        case EStepType::kMeasurement: typeStr = "measurement"; break;
        case EStepType::kWait: typeStr = "wait"; break;
        default: break;
    }

    oss << "{\"id\": \"" << in_pStep->GetId()
        << "\", \"name\": \"" << in_pStep->GetName()
        << "\", \"type\": \"" << typeStr
        << "\", \"enabled\": " << (in_pStep->IsEnabled() ? "true" : "false") << "}";

    return oss.str();
}

//=============================================================================
// CXmlSequenceParser
//=============================================================================

CResult CXmlSequenceParser::Parse(const TString& in_strContent, CTestSequence& out_sequence) {
    out_sequence.Clear();

    std::regex seqIdRegex(R"xxx(<sequence[^>]*id="([^"]*)">)xxx");
    std::regex seqNameRegex(R"xxx(<name>([^<]*)</name>)xxx");
    std::regex stepRegex(R"xxx(<step\s+id="([^"]*)"\s+name="([^"]*)"\s*(?:type="([^"]*)")?\s*/>)xxx");

    std::smatch match;
    SSequenceInfo info;

    if (std::regex_search(in_strContent, match, seqIdRegex)) {
        info.id = UnescapeXml(match[1].str());
    }
    if (std::regex_search(in_strContent, match, seqNameRegex)) {
        info.name = UnescapeXml(match[1].str());
    }
    out_sequence.SetInfo(info);

    auto stepsBegin = std::sregex_iterator(in_strContent.begin(), in_strContent.end(), stepRegex);
    auto stepsEnd = std::sregex_iterator();

    for (auto it = stepsBegin; it != stepsEnd; ++it) {
        TString id = UnescapeXml((*it)[1].str());
        TString name = UnescapeXml((*it)[2].str());
        TString typeStr = it->size() > 3 ? (*it)[3].str() : "action";

        EStepType type = EStepType::kAction;
        if (typeStr == "validation") type = EStepType::kValidation;
        else if (typeStr == "measurement") type = EStepType::kMeasurement;

        out_sequence.AddStep(std::make_unique<CLoadedTestStep>(id, name, type));
    }

    return TESTMATE_SUCCESS();
}

CResult CXmlSequenceParser::Serialize(const CTestSequence& in_sequence, TString& out_strContent) {
    std::ostringstream oss;
    const auto& info = in_sequence.GetInfo();

    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    oss << "<sequence id=\"" << EscapeXml(info.id)
        << "\" name=\"" << EscapeXml(info.name)
        << "\" version=\"" << EscapeXml(info.version) << "\">\n";
    oss << "  <steps>\n";

    for (TUInt32 i = 0; i < in_sequence.GetStepCount(); ++i) {
        const ITestStep* pStep = in_sequence.GetStep(i);
        if (pStep) {
            TString typeStr = "action";
            switch (pStep->GetType()) {
                case EStepType::kValidation: typeStr = "validation"; break;
                case EStepType::kMeasurement: typeStr = "measurement"; break;
                default: break;
            }
            oss << "    <step id=\"" << EscapeXml(pStep->GetId())
                << "\" name=\"" << EscapeXml(pStep->GetName())
                << "\" type=\"" << typeStr << "\" />\n";
        }
    }

    oss << "  </steps>\n";
    oss << "</sequence>\n";

    out_strContent = oss.str();
    return TESTMATE_SUCCESS();
}

TString CXmlSequenceParser::EscapeXml(const TString& in_str) {
    TString result;
    for (char c : in_str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c;
        }
    }
    return result;
}

TString CXmlSequenceParser::UnescapeXml(const TString& in_str) {
    TString result = in_str;
    size_t pos;
    while ((pos = result.find("&amp;")) != TString::npos) result.replace(pos, 5, "&");
    while ((pos = result.find("&lt;")) != TString::npos) result.replace(pos, 4, "<");
    while ((pos = result.find("&gt;")) != TString::npos) result.replace(pos, 4, ">");
    while ((pos = result.find("&quot;")) != TString::npos) result.replace(pos, 6, "\"");
    while ((pos = result.find("&apos;")) != TString::npos) result.replace(pos, 6, "'");
    return result;
}

//=============================================================================
// CSequenceFileIO
//=============================================================================

CSequenceFileIO& CSequenceFileIO::GetInstance() {
    static CSequenceFileIO instance;
    return instance;
}

CSequenceFileIO::CSequenceFileIO() {
    RegisterParser(std::make_shared<CJsonSequenceParser>());
    RegisterParser(std::make_shared<CXmlSequenceParser>());
}

void CSequenceFileIO::RegisterParser(TSharedPtr<ISequenceParser> in_pParser) {
    if (in_pParser) {
        m_mapParsers[in_pParser->GetFormat()] = in_pParser;
    }
}

ISequenceParser* CSequenceFileIO::GetParser(ESequenceFileFormat in_eFormat) {
    auto it = m_mapParsers.find(in_eFormat);
    return (it != m_mapParsers.end()) ? it->second.get() : nullptr;
}

ESequenceFileFormat CSequenceFileIO::DetectFormat(const TString& in_strPath) {
    size_t dotPos = in_strPath.rfind('.');
    if (dotPos != TString::npos) {
        TString ext = in_strPath.substr(dotPos);
        if (ext == ".json") return ESequenceFileFormat::kJson;
        if (ext == ".xml") return ESequenceFileFormat::kXml;
    }
    return ESequenceFileFormat::kJson;  // Default
}

CResult CSequenceFileIO::LoadSequence(const TString& in_strPath,
                                       CTestSequence& out_sequence,
                                       ESequenceFileFormat in_eFormat) {
    std::ifstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "Cannot open: " + in_strPath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    ESequenceFileFormat format = (in_eFormat == ESequenceFileFormat::kAuto)
        ? DetectFormat(in_strPath) : in_eFormat;

    return ParseFromString(buffer.str(), out_sequence, format);
}

CResult CSequenceFileIO::SaveSequence(const TString& in_strPath,
                                       const CTestSequence& in_sequence,
                                       ESequenceFileFormat in_eFormat) {
    ESequenceFileFormat format = (in_eFormat == ESequenceFileFormat::kAuto)
        ? DetectFormat(in_strPath) : in_eFormat;

    TString content;
    auto result = SerializeToString(in_sequence, content, format);
    if (result.IsFailure()) return result;

    std::ofstream file(in_strPath);
    if (!file) {
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Cannot write: " + in_strPath);
    }

    file << content;
    return TESTMATE_SUCCESS();
}

CResult CSequenceFileIO::ParseFromString(const TString& in_strContent,
                                          CTestSequence& out_sequence,
                                          ESequenceFileFormat in_eFormat) {
    ISequenceParser* pParser = GetParser(in_eFormat);
    if (!pParser) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "No parser for format");
    }
    return pParser->Parse(in_strContent, out_sequence);
}

CResult CSequenceFileIO::SerializeToString(const CTestSequence& in_sequence,
                                            TString& out_strContent,
                                            ESequenceFileFormat in_eFormat) {
    ISequenceParser* pParser = GetParser(in_eFormat);
    if (!pParser) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "No parser for format");
    }
    return pParser->Serialize(in_sequence, out_strContent);
}

} // namespace TestMATE
