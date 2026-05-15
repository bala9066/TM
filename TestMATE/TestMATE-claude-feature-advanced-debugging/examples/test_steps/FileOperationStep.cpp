/**************************************************************************
 * File Name: FileOperationStep.cpp
 * Description: Implementation of FileOperationStep
 **************************************************************************/

#include "FileOperationStep.h"
#include <fstream>
#include <filesystem>

namespace TestMATE {

CFileOperationStep::CFileOperationStep(const TString& in_strId,
                                       const TString& in_strName)
    : CTestStepBase(in_strId, in_strName, EStepType::kAction)
{
    SStepParameter opParam;
    opParam.name = "operation";
    opParam.type = "string";
    opParam.required = true;
    opParam.description = "File operation (read, write, append, delete, exists)";
    AddParameter(opParam);

    SStepParameter pathParam;
    pathParam.name = "file_path";
    pathParam.type = "string";
    pathParam.required = true;
    pathParam.description = "Path to file";
    AddParameter(pathParam);

    SStepParameter contentParam;
    contentParam.name = "content";
    contentParam.type = "string";
    contentParam.required = false;
    contentParam.description = "Content for write/append operations";
    AddParameter(contentParam);

    SetDescription("Perform file I/O operation");
}

CResult CFileOperationStep::Execute(SStepResult& out_result) {
    out_result.startTime = std::chrono::steady_clock::now();

    // Get operation type
    auto opParam = GetParameter("operation");
    if (!opParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing operation parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing operation");
    }

    TString op = opParam.value();
    if (op == "read") m_eOperation = EFileOperation::kRead;
    else if (op == "write") m_eOperation = EFileOperation::kWrite;
    else if (op == "append") m_eOperation = EFileOperation::kAppend;
    else if (op == "delete") m_eOperation = EFileOperation::kDelete;
    else if (op == "exists") m_eOperation = EFileOperation::kExists;
    else {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Unknown operation: " + op;
        out_result.endTime = std::chrono::steady_clock::now();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Unknown operation");
    }

    // Get file path
    auto pathParam = GetParameter("file_path");
    if (!pathParam.has_value()) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = "Missing file_path parameter";
        out_result.endTime = std::chrono::steady_clock::now();
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Missing file_path");
    }
    m_strFilePath = pathParam.value();

    // Get content if needed
    auto contentParam = GetParameter("content");
    if (contentParam.has_value()) {
        m_strContent = contentParam.value();
    }

    // Perform operation
    CResult result;
    switch (m_eOperation) {
        case EFileOperation::kRead:
            result = PerformRead(out_result);
            break;
        case EFileOperation::kWrite:
            result = PerformWrite(out_result);
            break;
        case EFileOperation::kAppend:
            result = PerformAppend(out_result);
            break;
        case EFileOperation::kDelete:
            result = PerformDelete(out_result);
            break;
        case EFileOperation::kExists:
            result = PerformExists(out_result);
            break;
        default:
            result = TESTMATE_FAILURE(EErrorCode::kInvalidParameter, "Unknown operation");
    }

    out_result.endTime = std::chrono::steady_clock::now();
    out_result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        out_result.endTime - out_result.startTime).count();

    return result;
}

void CFileOperationStep::SetOperation(EFileOperation in_eOp) {
    m_eOperation = in_eOp;
}

void CFileOperationStep::SetFilePath(const TString& in_strPath) {
    m_strFilePath = in_strPath;
    SetParameter("file_path", in_strPath);
}

void CFileOperationStep::SetContent(const TString& in_strContent) {
    m_strContent = in_strContent;
    SetParameter("content", in_strContent);
}

CResult CFileOperationStep::PerformRead(SStepResult& out_result) {
    std::ifstream file(m_strFilePath);
    if (!file.is_open()) {
        out_result.verdict = ETestVerdict::kFail;
        out_result.message = "Failed to open file for reading: " + m_strFilePath;
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "File not found");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_strReadContent = buffer.str();
    file.close();

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "File read successfully";
    out_result.measurements["content"] = m_strReadContent;
    out_result.measurements["file_size"] = std::to_string(m_strReadContent.length());

    return TESTMATE_SUCCESS();
}

CResult CFileOperationStep::PerformWrite(SStepResult& out_result) {
    std::ofstream file(m_strFilePath, std::ios::trunc);
    if (!file.is_open()) {
        out_result.verdict = ETestVerdict::kFail;
        out_result.message = "Failed to open file for writing: " + m_strFilePath;
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Write failed");
    }

    file << m_strContent;
    file.close();

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "File written successfully";
    out_result.measurements["bytes_written"] = std::to_string(m_strContent.length());

    return TESTMATE_SUCCESS();
}

CResult CFileOperationStep::PerformAppend(SStepResult& out_result) {
    std::ofstream file(m_strFilePath, std::ios::app);
    if (!file.is_open()) {
        out_result.verdict = ETestVerdict::kFail;
        out_result.message = "Failed to open file for appending: " + m_strFilePath;
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, "Append failed");
    }

    file << m_strContent;
    file.close();

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = "Content appended successfully";
    out_result.measurements["bytes_appended"] = std::to_string(m_strContent.length());

    return TESTMATE_SUCCESS();
}

CResult CFileOperationStep::PerformDelete(SStepResult& out_result) {
    try {
        if (std::filesystem::remove(m_strFilePath)) {
            out_result.verdict = ETestVerdict::kPass;
            out_result.message = "File deleted successfully";
            return TESTMATE_SUCCESS();
        } else {
            out_result.verdict = ETestVerdict::kFail;
            out_result.message = "File not found or already deleted";
            return TESTMATE_FAILURE(EErrorCode::kFileNotFound, "File not found");
        }
    } catch (const std::exception& e) {
        out_result.verdict = ETestVerdict::kError;
        out_result.message = TString("Delete failed: ") + e.what();
        return TESTMATE_FAILURE(EErrorCode::kFileWriteFailed, e.what());
    }
}

CResult CFileOperationStep::PerformExists(SStepResult& out_result) {
    bool exists = std::filesystem::exists(m_strFilePath);

    out_result.verdict = ETestVerdict::kPass;
    out_result.message = exists ? "File exists" : "File does not exist";
    out_result.measurements["exists"] = exists ? "true" : "false";

    return TESTMATE_SUCCESS();
}

} // namespace TestMATE
