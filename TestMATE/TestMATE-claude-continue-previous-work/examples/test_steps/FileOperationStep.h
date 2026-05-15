/**************************************************************************
 * File Name: FileOperationStep.h
 * Description: Test step for file I/O operations
 * Author: TestMATE Development Team
 **************************************************************************/

#pragma once

#include "core/test_sequence/ITestStep.h"

namespace TestMATE {

enum class EFileOperation {
    kRead,          // Read file contents
    kWrite,         // Write to file
    kAppend,        // Append to file
    kDelete,        // Delete file
    kExists,        // Check if file exists
    kCopy,          // Copy file
    kMove           // Move/rename file
};

/**************************************************************************
 * Class: CFileOperationStep
 * Description: Performs file system operations
 *
 * Parameters:
 *   - operation: Type of file operation (read, write, append, delete, etc.)
 *   - file_path: Path to file
 *   - content: Content for write/append operations
 *   - destination: Destination path for copy/move operations
 **************************************************************************/
class CFileOperationStep : public CTestStepBase {
public:
    explicit CFileOperationStep(const TString& in_strId = "FILE-001",
                               const TString& in_strName = "File Operation");

    ~CFileOperationStep() override = default;

    CResult Execute(SStepResult& out_result) override;

    void SetOperation(EFileOperation in_eOp);
    void SetFilePath(const TString& in_strPath);
    void SetContent(const TString& in_strContent);

    [[nodiscard]] TString GetReadContent() const { return m_strReadContent; }

private:
    EFileOperation m_eOperation{EFileOperation::kRead};
    TString m_strFilePath;
    TString m_strContent;
    TString m_strDestination;
    TString m_strReadContent;

    CResult PerformRead(SStepResult& out_result);
    CResult PerformWrite(SStepResult& out_result);
    CResult PerformAppend(SStepResult& out_result);
    CResult PerformDelete(SStepResult& out_result);
    CResult PerformExists(SStepResult& out_result);
};

} // namespace TestMATE
