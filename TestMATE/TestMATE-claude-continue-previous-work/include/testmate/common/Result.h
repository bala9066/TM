/**************************************************************************
 * File Name: Result.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Error handling result class for TestMATE system.
 *              Implements standardized error codes per SDG guidelines.
 * Requirements: REQ-CORE-017 to REQ-CORE-019
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include <string>
#include <source_location>

namespace TestMATE {

//=============================================================================
// Error Code Definitions (Negative values per SDG guidelines)
//=============================================================================

/**************************************************************************
 * Enum: EErrorCode
 * Description: Standard error codes for TestMATE operations.
 *              Negative values indicate errors per SDG guidelines.
 **************************************************************************/
enum class EErrorCode : TInt32 {
    // Success
    kSuccess = 0,

    // General errors (-1 to -99)
    kUnknownError = -1,
    kInvalidParameter = -2,
    kNullPointer = -3,
    kOutOfMemory = -4,
    kNotImplemented = -5,
    kNotInitialized = -6,
    kAlreadyInitialized = -7,
    kInvalidState = -8,
    kOperationCancelled = -9,
    kTimeout = -10,
    kNotFound = -11,
    kAlreadyExists = -12,
    kAccessDenied = -13,
    kInvalidOperation = -14,

    // File/IO errors (-100 to -199)
    kFileNotFound = -100,
    kFileOpenFailed = -101,
    kFileReadFailed = -102,
    kFileWriteFailed = -103,
    kFileCloseFailed = -104,
    kInvalidFilePath = -105,
    kInvalidFileFormat = -106,
    kDirectoryNotFound = -107,
    kPermissionDenied = -108,

    // Configuration errors (-200 to -299)
    kConfigNotFound = -200,
    kConfigParseFailed = -201,
    kConfigValidationFailed = -202,
    kConfigValueInvalid = -203,
    kConfigMissingRequired = -204,

    // Resource errors (-300 to -399)
    kResourceNotFound = -300,
    kResourceBusy = -301,
    kResourceLockFailed = -302,
    kResourceReleaseFailed = -303,
    kResourceTimeout = -304,
    kDeadlockDetected = -305,
    kResourceConflict = -306,

    // Thread errors (-400 to -499)
    kThreadCreationFailed = -400,
    kThreadJoinFailed = -401,
    kMutexError = -402,
    kConditionVariableError = -403,
    kSyncPointTimeout = -404,

    // Plugin errors (-500 to -599)
    kPluginLoadFailed = -500,
    kPluginNotFound = -501,
    kPluginVersionMismatch = -502,
    kPluginInitFailed = -503,
    kPluginInvalid = -504,

    // Execution errors (-600 to -699)
    kExecutionFailed = -600,
    kStepFailed = -601,
    kStepTimeout = -602,
    kStepAborted = -603,
    kSequenceNotFound = -604,
    kInvalidTestPlan = -605,

    // Communication errors (-700 to -799)
    kConnectionFailed = -700,
    kConnectionLost = -701,
    kSendFailed = -702,
    kReceiveFailed = -703,
    kProtocolError = -704,

    // Database errors (-800 to -899)
    kDatabaseConnectionFailed = -800,
    kDatabaseQueryFailed = -801,
    kDatabaseWriteFailed = -802,
    kDatabaseTransactionFailed = -803,

    // License errors (-900 to -999)
    kLicenseInvalid = -900,
    kLicenseExpired = -901,
    kLicenseFeatureNotAvailable = -902,
    kLicenseServerUnavailable = -903
};

/**************************************************************************
 * Enum: EErrorSeverity
 * Description: Error severity classification
 * Requirements: REQ-CORE-017
 **************************************************************************/
enum class EErrorSeverity : TInt32 {
    kFatal = 0,     ///< System unusable, restart required
    kCritical = 1,  ///< Major function impaired
    kError = 2,     ///< Operation failed but recoverable
    kWarning = 3,   ///< Potential issue
    kInfo = 4       ///< Notable event
};

//=============================================================================
// Result Class
//=============================================================================

/**************************************************************************
 * Class: CResult
 * Description: Represents operation result with error code and message.
 *              Used for standardized error handling throughout the system.
 * Requirements: REQ-CORE-017 to REQ-CORE-019
 **************************************************************************/
class CResult {
public:
    /**************************************************************************
     * Function Name: CResult (default constructor)
     * Description: Creates a success result
     **************************************************************************/
    CResult() noexcept
        : m_eCode(EErrorCode::kSuccess)
        , m_strMessage()
        , m_eSeverity(EErrorSeverity::kInfo)
    {}

    /**************************************************************************
     * Function Name: CResult
     * Description: Creates a result with specified error code
     * Parameters:
     *   in_eCode - Error code
     **************************************************************************/
    explicit CResult(EErrorCode in_eCode) noexcept
        : m_eCode(in_eCode)
        , m_strMessage()
        , m_eSeverity(DetermineSeverity(in_eCode))
    {}

    /**************************************************************************
     * Function Name: CResult
     * Description: Creates a result with error code and message
     * Parameters:
     *   in_eCode - Error code
     *   in_strMessage - Error message
     **************************************************************************/
    CResult(EErrorCode in_eCode, TString in_strMessage) noexcept
        : m_eCode(in_eCode)
        , m_strMessage(std::move(in_strMessage))
        , m_eSeverity(DetermineSeverity(in_eCode))
    {}

    /**************************************************************************
     * Function Name: CResult
     * Description: Creates a result with full details including source location
     * Parameters:
     *   in_eCode - Error code
     *   in_strMessage - Error message
     *   in_location - Source location where error occurred
     **************************************************************************/
    CResult(EErrorCode in_eCode,
            TString in_strMessage,
            std::source_location in_location) noexcept
        : m_eCode(in_eCode)
        , m_strMessage(std::move(in_strMessage))
        , m_eSeverity(DetermineSeverity(in_eCode))
        , m_location(in_location)
    {}

    // Accessors
    [[nodiscard]] EErrorCode GetCode() const noexcept { return m_eCode; }
    [[nodiscard]] const TString& GetMessage() const noexcept { return m_strMessage; }
    [[nodiscard]] EErrorSeverity GetSeverity() const noexcept { return m_eSeverity; }
    [[nodiscard]] const std::source_location& GetLocation() const noexcept { return m_location; }

    /**************************************************************************
     * Function Name: IsSuccess
     * Description: Checks if result indicates success
     * Returns: true if operation succeeded
     **************************************************************************/
    [[nodiscard]] bool IsSuccess() const noexcept {
        return m_eCode == EErrorCode::kSuccess;
    }

    /**************************************************************************
     * Function Name: IsFailure
     * Description: Checks if result indicates failure
     * Returns: true if operation failed
     **************************************************************************/
    [[nodiscard]] bool IsFailure() const noexcept {
        return m_eCode != EErrorCode::kSuccess;
    }

    /**************************************************************************
     * Function Name: operator bool
     * Description: Implicit conversion to bool for easy checking
     * Returns: true if success
     **************************************************************************/
    explicit operator bool() const noexcept {
        return IsSuccess();
    }

    // Static factory methods
    [[nodiscard]] static CResult Success() noexcept {
        return CResult(EErrorCode::kSuccess);
    }

    [[nodiscard]] static CResult Failure(EErrorCode in_eCode,
                                         TString in_strMessage = "",
                                         std::source_location in_loc = std::source_location::current()) {
        return CResult(in_eCode, std::move(in_strMessage), in_loc);
    }

private:
    EErrorCode m_eCode;
    TString m_strMessage;
    EErrorSeverity m_eSeverity;
    std::source_location m_location;

    /**************************************************************************
     * Function Name: DetermineSeverity
     * Description: Determines severity based on error code
     * Parameters:
     *   in_eCode - Error code to evaluate
     * Returns: Appropriate severity level
     **************************************************************************/
    [[nodiscard]] static EErrorSeverity DetermineSeverity(EErrorCode in_eCode) noexcept {
        if (in_eCode == EErrorCode::kSuccess) {
            return EErrorSeverity::kInfo;
        }

        TInt32 iCode = static_cast<TInt32>(in_eCode);

        // Fatal errors
        if (iCode <= -900 || iCode == static_cast<TInt32>(EErrorCode::kOutOfMemory)) {
            return EErrorSeverity::kFatal;
        }

        // Critical errors
        if (iCode <= -600) {
            return EErrorSeverity::kCritical;
        }

        // Standard errors
        return EErrorSeverity::kError;
    }
};

//=============================================================================
// Result Macros
//=============================================================================

/// Check result and return on failure
#define TESTMATE_CHECK_RESULT(result) \
    do { \
        if (auto _r = (result); _r.IsFailure()) { \
            return _r; \
        } \
    } while(0)

/// Create failure result with current location
#define TESTMATE_FAILURE(code, msg) \
    CResult::Failure(code, msg, std::source_location::current())

/// Return success
#define TESTMATE_SUCCESS() \
    CResult::Success()

//=============================================================================
// Helper Functions
//=============================================================================

/**************************************************************************
 * Function Name: GetErrorCodeName
 * Description: Converts error code to string name
 * Parameters:
 *   in_eCode - Error code
 * Returns: String representation of error code
 **************************************************************************/
[[nodiscard]] inline TString GetErrorCodeName(EErrorCode in_eCode) {
    switch (in_eCode) {
        case EErrorCode::kSuccess: return "Success";
        case EErrorCode::kUnknownError: return "UnknownError";
        case EErrorCode::kInvalidParameter: return "InvalidParameter";
        case EErrorCode::kNullPointer: return "NullPointer";
        case EErrorCode::kTimeout: return "Timeout";
        case EErrorCode::kNotFound: return "NotFound";
        case EErrorCode::kFileNotFound: return "FileNotFound";
        case EErrorCode::kResourceBusy: return "ResourceBusy";
        case EErrorCode::kDeadlockDetected: return "DeadlockDetected";
        case EErrorCode::kPluginLoadFailed: return "PluginLoadFailed";
        case EErrorCode::kExecutionFailed: return "ExecutionFailed";
        case EErrorCode::kLicenseInvalid: return "LicenseInvalid";
        default: return "Error_" + std::to_string(static_cast<TInt32>(in_eCode));
    }
}

} // namespace TestMATE
