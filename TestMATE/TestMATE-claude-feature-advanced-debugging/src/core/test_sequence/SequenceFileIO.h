/**************************************************************************
 * File Name: SequenceFileIO.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Test sequence file parsing and serialization.
 * Requirements: REQ-SEQ-031 to REQ-SEQ-050
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include "core/test_sequence/TestSequence.h"
#include <map>

namespace TestMATE {

/**************************************************************************
 * Enum: ESequenceFileFormat
 * Description: Supported sequence file formats
 **************************************************************************/
enum class ESequenceFileFormat {
    kXml,
    kJson,
    kAuto  // Detect from extension
};

/**************************************************************************
 * Interface: ISequenceParser
 * Description: Interface for sequence file parsers
 **************************************************************************/
class ISequenceParser {
public:
    virtual ~ISequenceParser() = default;

    virtual CResult Parse(const TString& in_strContent,
                          CTestSequence& out_sequence) = 0;
    virtual CResult Serialize(const CTestSequence& in_sequence,
                               TString& out_strContent) = 0;
    [[nodiscard]] virtual ESequenceFileFormat GetFormat() const = 0;
};

/**************************************************************************
 * Class: CJsonSequenceParser
 * Description: JSON format sequence parser
 **************************************************************************/
class CJsonSequenceParser : public ISequenceParser {
public:
    CResult Parse(const TString& in_strContent, CTestSequence& out_sequence) override;
    CResult Serialize(const CTestSequence& in_sequence, TString& out_strContent) override;
    [[nodiscard]] ESequenceFileFormat GetFormat() const override { return ESequenceFileFormat::kJson; }

private:
    CResult ParseStep(const TString& in_strStepJson, TUniquePtr<ITestStep>& out_pStep);
    TString SerializeStep(const ITestStep* in_pStep);
};

/**************************************************************************
 * Class: CXmlSequenceParser
 * Description: XML format sequence parser
 **************************************************************************/
class CXmlSequenceParser : public ISequenceParser {
public:
    CResult Parse(const TString& in_strContent, CTestSequence& out_sequence) override;
    CResult Serialize(const CTestSequence& in_sequence, TString& out_strContent) override;
    [[nodiscard]] ESequenceFileFormat GetFormat() const override { return ESequenceFileFormat::kXml; }

private:
    TString EscapeXml(const TString& in_str);
    TString UnescapeXml(const TString& in_str);
};

/**************************************************************************
 * Class: CSequenceFileIO
 * Description: High-level sequence file operations
 **************************************************************************/
class CSequenceFileIO {
public:
    static CSequenceFileIO& GetInstance();

    CSequenceFileIO(const CSequenceFileIO&) = delete;
    CSequenceFileIO& operator=(const CSequenceFileIO&) = delete;

    /**************************************************************************
     * Function Name: LoadSequence
     * Description: Loads a sequence from file
     **************************************************************************/
    CResult LoadSequence(const TString& in_strPath,
                         CTestSequence& out_sequence,
                         ESequenceFileFormat in_eFormat = ESequenceFileFormat::kAuto);

    /**************************************************************************
     * Function Name: SaveSequence
     * Description: Saves a sequence to file
     **************************************************************************/
    CResult SaveSequence(const TString& in_strPath,
                         const CTestSequence& in_sequence,
                         ESequenceFileFormat in_eFormat = ESequenceFileFormat::kAuto);

    /**************************************************************************
     * Function Name: ParseFromString
     * Description: Parses sequence from string content
     **************************************************************************/
    CResult ParseFromString(const TString& in_strContent,
                            CTestSequence& out_sequence,
                            ESequenceFileFormat in_eFormat);

    /**************************************************************************
     * Function Name: SerializeToString
     * Description: Serializes sequence to string
     **************************************************************************/
    CResult SerializeToString(const CTestSequence& in_sequence,
                               TString& out_strContent,
                               ESequenceFileFormat in_eFormat);

    void RegisterParser(TSharedPtr<ISequenceParser> in_pParser);

private:
    CSequenceFileIO();
    ~CSequenceFileIO() = default;

    ESequenceFileFormat DetectFormat(const TString& in_strPath);
    ISequenceParser* GetParser(ESequenceFileFormat in_eFormat);

    std::map<ESequenceFileFormat, TSharedPtr<ISequenceParser>> m_mapParsers;
};

} // namespace TestMATE
