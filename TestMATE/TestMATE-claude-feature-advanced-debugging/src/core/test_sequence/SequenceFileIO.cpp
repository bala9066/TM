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
#include <map>
#include <vector>
#include <memory>

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
// JSON parsing support
//=============================================================================

namespace {

// Minimal recursive-descent JSON parser. Replaces the previous regex-based
// extraction, which silently mis-parsed any step containing nested objects,
// arrays, or a ']' / '}' inside a string value.
class CJsonValue {
public:
    enum class EType { kNull, kBool, kNumber, kString, kArray, kObject };

    EType type{EType::kNull};
    TString stringValue;
    std::vector<CJsonValue> arrayValue;
    std::map<TString, CJsonValue> objectValue;

    [[nodiscard]] bool IsObject() const { return type == EType::kObject; }
    [[nodiscard]] bool IsArray() const { return type == EType::kArray; }

    [[nodiscard]] const CJsonValue* Find(const TString& in_key) const {
        if (type != EType::kObject) {
            return nullptr;
        }
        auto it = objectValue.find(in_key);
        return it != objectValue.end() ? &it->second : nullptr;
    }

    [[nodiscard]] TString AsString(const TString& in_default = "") const {
        return type == EType::kString ? stringValue : in_default;
    }
};

class CJsonParser {
public:
    explicit CJsonParser(const TString& in_text) : m_text(in_text) {}

    bool Parse(CJsonValue& out_value) {
        SkipWhitespace();
        if (!ParseValue(out_value)) {
            return false;
        }
        SkipWhitespace();
        return m_pos >= m_text.size();  // reject trailing content
    }

private:
    const TString& m_text;
    size_t m_pos{0};

    void SkipWhitespace() {
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++m_pos;
            } else {
                break;
            }
        }
    }

    bool ParseValue(CJsonValue& out_value) {
        SkipWhitespace();
        if (m_pos >= m_text.size()) {
            return false;
        }
        switch (m_text[m_pos]) {
            case '{': return ParseObject(out_value);
            case '[': return ParseArray(out_value);
            case '"':
                out_value.type = CJsonValue::EType::kString;
                return ParseString(out_value.stringValue);
            case 't': case 'f': return ParseBool(out_value);
            case 'n':
                if (m_text.compare(m_pos, 4, "null") == 0) {
                    out_value.type = CJsonValue::EType::kNull;
                    m_pos += 4;
                    return true;
                }
                return false;
            default: return ParseNumber(out_value);
        }
    }

    bool ParseObject(CJsonValue& out_value) {
        out_value.type = CJsonValue::EType::kObject;
        ++m_pos;  // consume '{'
        SkipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            return true;
        }
        while (true) {
            SkipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
                return false;
            }
            TString key;
            if (!ParseString(key)) {
                return false;
            }
            SkipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
                return false;
            }
            ++m_pos;  // consume ':'
            CJsonValue value;
            if (!ParseValue(value)) {
                return false;
            }
            out_value.objectValue[key] = std::move(value);
            SkipWhitespace();
            if (m_pos >= m_text.size()) {
                return false;
            }
            if (m_text[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_text[m_pos] == '}') {
                ++m_pos;
                return true;
            }
            return false;
        }
    }

    bool ParseArray(CJsonValue& out_value) {
        out_value.type = CJsonValue::EType::kArray;
        ++m_pos;  // consume '['
        SkipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            return true;
        }
        while (true) {
            CJsonValue value;
            if (!ParseValue(value)) {
                return false;
            }
            out_value.arrayValue.push_back(std::move(value));
            SkipWhitespace();
            if (m_pos >= m_text.size()) {
                return false;
            }
            if (m_text[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_text[m_pos] == ']') {
                ++m_pos;
                return true;
            }
            return false;
        }
    }

    bool ParseString(TString& out_string) {
        if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
            return false;
        }
        ++m_pos;  // consume opening '"'
        out_string.clear();
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out_string += c;
                continue;
            }
            if (m_pos >= m_text.size()) {
                return false;
            }
            const char esc = m_text[m_pos++];
            switch (esc) {
                case '"':  out_string += '"';  break;
                case '\\': out_string += '\\'; break;
                case '/':  out_string += '/';  break;
                case 'n':  out_string += '\n'; break;
                case 't':  out_string += '\t'; break;
                case 'r':  out_string += '\r'; break;
                case 'b':  out_string += '\b'; break;
                case 'f':  out_string += '\f'; break;
                case 'u': {
                    if (m_pos + 4 > m_text.size()) {
                        return false;
                    }
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        const char h = m_text[m_pos++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') {
                            cp |= static_cast<unsigned int>(h - '0');
                        } else if (h >= 'a' && h <= 'f') {
                            cp |= static_cast<unsigned int>(h - 'a' + 10);
                        } else if (h >= 'A' && h <= 'F') {
                            cp |= static_cast<unsigned int>(h - 'A' + 10);
                        } else {
                            return false;
                        }
                    }
                    // Encode the BMP code point as UTF-8.
                    if (cp < 0x80) {
                        out_string += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out_string += static_cast<char>(0xC0 | (cp >> 6));
                        out_string += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out_string += static_cast<char>(0xE0 | (cp >> 12));
                        out_string += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out_string += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default:
                    return false;
            }
        }
        return false;  // unterminated string
    }

    bool ParseNumber(CJsonValue& out_value) {
        const size_t start = m_pos;
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' ||
                c == '.' || c == 'e' || c == 'E') {
                ++m_pos;
            } else {
                break;
            }
        }
        if (m_pos == start) {
            return false;
        }
        // Numeric values are accepted but retained only as type information;
        // the sequence schema reads string fields exclusively.
        out_value.type = CJsonValue::EType::kNumber;
        return true;
    }

    bool ParseBool(CJsonValue& out_value) {
        if (m_text.compare(m_pos, 4, "true") == 0) {
            out_value.type = CJsonValue::EType::kBool;
            m_pos += 4;
            return true;
        }
        if (m_text.compare(m_pos, 5, "false") == 0) {
            out_value.type = CJsonValue::EType::kBool;
            m_pos += 5;
            return true;
        }
        return false;
    }
};

EStepType StepTypeFromString(const TString& in_type) {
    if (in_type == "validation")  { return EStepType::kValidation; }
    if (in_type == "measurement") { return EStepType::kMeasurement; }
    if (in_type == "wait")        { return EStepType::kWait; }
    if (in_type == "sequence")    { return EStepType::kSequence; }
    if (in_type == "conditional") { return EStepType::kConditional; }
    if (in_type == "loop")        { return EStepType::kLoop; }
    if (in_type == "call")        { return EStepType::kCall; }
    if (in_type == "sync")        { return EStepType::kSync; }
    if (in_type == "custom")      { return EStepType::kCustom; }
    return EStepType::kAction;
}

TUniquePtr<ITestStep> BuildStepFromJson(const CJsonValue& in_stepObj) {
    if (!in_stepObj.IsObject()) {
        return nullptr;
    }
    const CJsonValue* idVal = in_stepObj.Find("id");
    if (!idVal) {
        return nullptr;
    }
    const TString id = idVal->AsString();
    if (id.empty()) {
        return nullptr;
    }
    TString name;
    if (const CJsonValue* nameVal = in_stepObj.Find("name")) {
        name = nameVal->AsString();
    }
    EStepType type = EStepType::kAction;
    if (const CJsonValue* typeVal = in_stepObj.Find("type")) {
        type = StepTypeFromString(typeVal->AsString());
    }
    return std::make_unique<CLoadedTestStep>(id, name, type);
}

}  // namespace

//=============================================================================
// CJsonSequenceParser
//=============================================================================

CResult CJsonSequenceParser::Parse(const TString& in_strContent, CTestSequence& out_sequence) {
    out_sequence.Clear();

    CJsonValue root;
    CJsonParser parser(in_strContent);
    if (!parser.Parse(root) || !root.IsObject()) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidFileFormat,
                                "Invalid JSON in sequence file");
    }

    SSequenceInfo info;
    if (const CJsonValue* v = root.Find("id")) {
        info.id = v->AsString();
    }
    if (const CJsonValue* v = root.Find("name")) {
        info.name = v->AsString();
    }
    if (const CJsonValue* v = root.Find("version")) {
        info.version = v->AsString();
    }
    out_sequence.SetInfo(info);

    if (const CJsonValue* steps = root.Find("steps"); steps && steps->IsArray()) {
        for (const CJsonValue& stepVal : steps->arrayValue) {
            TUniquePtr<ITestStep> pStep = BuildStepFromJson(stepVal);
            if (pStep) {
                out_sequence.AddStep(std::move(pStep));
            }
        }
    }

    return TESTMATE_SUCCESS();
}

CResult CJsonSequenceParser::ParseStep(const TString& in_strStepJson, TUniquePtr<ITestStep>& out_pStep) {
    CJsonValue stepVal;
    CJsonParser parser(in_strStepJson);
    if (!parser.Parse(stepVal)) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidFileFormat, "Invalid step JSON");
    }
    out_pStep = BuildStepFromJson(stepVal);
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
    // Single left-to-right pass: each entity is consumed exactly once.
    // The previous repeated-replace approach unescaped "&amp;" first, so
    // escaped literal text such as "&amp;lt;" was wrongly turned into "<"
    // (double unescape). std::string::compare clamps its length argument,
    // so the lookups are bounds-safe even near the end of the string.
    TString result;
    result.reserve(in_str.size());

    for (size_t i = 0; i < in_str.size(); ) {
        if (in_str[i] == '&') {
            if (in_str.compare(i, 5, "&amp;") == 0)  { result += '&';  i += 5; continue; }
            if (in_str.compare(i, 4, "&lt;") == 0)   { result += '<';  i += 4; continue; }
            if (in_str.compare(i, 4, "&gt;") == 0)   { result += '>';  i += 4; continue; }
            if (in_str.compare(i, 6, "&quot;") == 0) { result += '"';  i += 6; continue; }
            if (in_str.compare(i, 6, "&apos;") == 0) { result += '\''; i += 6; continue; }
        }
        result += in_str[i];
        ++i;
    }

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
