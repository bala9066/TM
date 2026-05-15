/**************************************************************************
 * File Name: DUTCommTestStep.cpp
 * Description: Implementation of DUT communication test steps
 * Author: TestMATE Development Team
 **************************************************************************/

#include "DUTCommTestStep.h"
#include "utils/LogManager.h"
#include <format>

namespace TestMATE {

//============================================================================
// CDUTRegisterTestStep Implementation
//============================================================================

CDUTRegisterTestStep::CDUTRegisterTestStep(const TString& in_id, const TString& in_name)
    : m_address(0)
    , m_expectedValue(0)
    , m_writeValue(0)
    , m_mask(0xFFFFFFFF)
    , m_operation(EOperation::kRead)
    , m_pUARTProtocol(nullptr)
    , m_aborted(false) {
    // Store ID and name if needed for your implementation
    (void)in_id;    // Unused in this simplified example
    (void)in_name;  // Unused in this simplified example
}

CResult CDUTRegisterTestStep::Execute(SSimpleContext& context) {
    m_aborted = false;

    if (!m_pUARTProtocol) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidOperation, "UART protocol not configured");
    }

    LOG_INFO("DUTRegisterTestStep", "Executing {} operation on register 0x{:04X}",
             m_operation == EOperation::kRead ? "READ" :
             m_operation == EOperation::kWrite ? "WRITE" : "VERIFY",
             m_address);

    CResult result;

    switch (m_operation) {
        case EOperation::kRead: {
            // Read register value
            TUInt32 value;
            result = m_pUARTProtocol->ReadRegister(m_address, value);
            if (!result.IsSuccess()) {
                return TESTMATE_FAILURE(EErrorCode::kReceiveFailed,
                                        std::format("Failed to read register 0x{:04X}: {}",
                                                    m_address, result.GetMessage()));
            }

            // Store in context for later steps
            context.SetVariable("register_address", static_cast<TInt64>(m_address));
            context.SetVariable("register_value", static_cast<TInt64>(value));

            // Apply mask and compare
            TUInt32 maskedValue = value & m_mask;
            TUInt32 maskedExpected = m_expectedValue & m_mask;

            if (maskedValue != maskedExpected) {
                return TESTMATE_FAILURE(EErrorCode::kStepFailed,
                                        std::format("Register 0x{:04X}: Expected 0x{:08X}, Got 0x{:08X} (mask 0x{:08X})",
                                                    m_address, maskedExpected, maskedValue, m_mask));
            }

            LOG_INFO("DUTRegisterTestStep", "Register 0x{:04X} = 0x{:08X} (PASS)", m_address, value);
            break;
        }

        case EOperation::kWrite: {
            // Write register value
            result = m_pUARTProtocol->WriteRegister(m_address, m_writeValue);
            if (!result.IsSuccess()) {
                return TESTMATE_FAILURE(EErrorCode::kSendFailed,
                                        std::format("Failed to write register 0x{:04X}: {}",
                                                    m_address, result.GetMessage()));
            }

            context.SetVariable("register_address", static_cast<TInt64>(m_address));
            context.SetVariable("register_value_written", static_cast<TInt64>(m_writeValue));

            LOG_INFO("DUTRegisterTestStep", "Wrote register 0x{:04X} = 0x{:08X}", m_address, m_writeValue);
            break;
        }

        case EOperation::kVerify: {
            // Write then read back to verify
            result = m_pUARTProtocol->WriteRegister(m_address, m_writeValue);
            if (!result.IsSuccess()) {
                return TESTMATE_FAILURE(EErrorCode::kSendFailed,
                                        std::format("Failed to write register 0x{:04X}: {}",
                                                    m_address, result.GetMessage()));
            }

            if (m_aborted) {
                return TESTMATE_FAILURE(EErrorCode::kStepAborted, "Test aborted");
            }

            // Read back
            TUInt32 readbackValue;
            result = m_pUARTProtocol->ReadRegister(m_address, readbackValue);
            if (!result.IsSuccess()) {
                return TESTMATE_FAILURE(EErrorCode::kReceiveFailed,
                                        std::format("Failed to read back register 0x{:04X}: {}",
                                                    m_address, result.GetMessage()));
            }

            context.SetVariable("register_address", static_cast<TInt64>(m_address));
            context.SetVariable("register_value_written", static_cast<TInt64>(m_writeValue));
            context.SetVariable("register_value_readback", static_cast<TInt64>(readbackValue));

            // Compare (with mask)
            TUInt32 maskedReadback = readbackValue & m_mask;
            TUInt32 maskedWritten = m_writeValue & m_mask;

            if (maskedReadback != maskedWritten) {
                return TESTMATE_FAILURE(EErrorCode::kStepFailed,
                                        std::format("Register 0x{:04X} verify failed: Wrote 0x{:08X}, Read 0x{:08X} (mask 0x{:08X})",
                                                    m_address, maskedWritten, maskedReadback, m_mask));
            }

            LOG_INFO("DUTRegisterTestStep", "Register 0x{:04X} write/verify = 0x{:08X} (PASS)",
                     m_address, m_writeValue);
            break;
        }
    }

    return TESTMATE_SUCCESS();
}

void CDUTRegisterTestStep::Abort() {
    m_aborted = true;
    LOG_WARNING("DUTRegisterTestStep", "Test step aborted");
}

//============================================================================
// CEthernetCommandTestStep Implementation
//============================================================================

CEthernetCommandTestStep::CEthernetCommandTestStep(const TString& in_id, const TString& in_name)
    : m_pConnection(nullptr)
    , m_packetType(0)
    , m_expectedResponseType(0)
    , m_timeoutMs(5000)
    , m_aborted(false) {
    // Store ID and name if needed for your implementation
    (void)in_id;    // Unused in this simplified example
    (void)in_name;  // Unused in this simplified example
}

CResult CEthernetCommandTestStep::Execute(SSimpleContext& context) {
    m_aborted = false;

    if (!m_pConnection || !m_pConnection->IsConnected()) {
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Ethernet connection not established");
    }

    LOG_INFO("EthernetCommandTestStep", "Sending packet type 0x{:02X} with {} byte payload",
             m_packetType, m_payload.size());

    // Send command packet
    auto result = m_pConnection->SendPacket(m_payload, m_packetType);
    if (!result.IsSuccess()) {
        return TESTMATE_FAILURE(EErrorCode::kSendFailed,
                                std::format("Failed to send packet: {}", result.GetMessage()));
    }

    // Store command in context
    context.SetVariable("command_packet_type", static_cast<TInt64>(m_packetType));
    context.SetVariable("command_payload_size", static_cast<TInt64>(m_payload.size()));

    if (m_aborted) {
        return TESTMATE_FAILURE(EErrorCode::kStepAborted, "Test aborted");
    }

    // Receive response packet
    TVector<TUInt8> responsePayload;
    TUInt8 responseType;

    result = m_pConnection->ReceivePacket(responsePayload, responseType);
    if (!result.IsSuccess()) {
        return TESTMATE_FAILURE(EErrorCode::kReceiveFailed,
                                std::format("Failed to receive response: {}", result.GetMessage()));
    }

    // Store response in context
    context.SetVariable("response_packet_type", static_cast<TInt64>(responseType));
    context.SetVariable("response_payload_size", static_cast<TInt64>(responsePayload.size()));

    // Validate response type
    if (responseType != m_expectedResponseType) {
        return TESTMATE_FAILURE(EErrorCode::kStepFailed,
                                std::format("Unexpected response type: Expected 0x{:02X}, Got 0x{:02X}",
                                            m_expectedResponseType, responseType));
    }

    LOG_INFO("EthernetCommandTestStep", "Received response type 0x{:02X} with {} bytes (PASS)",
             responseType, responsePayload.size());

    // Parse response payload (example: first 4 bytes as status code)
    if (responsePayload.size() >= 4) {
        TUInt32 statusCode = (static_cast<TUInt32>(responsePayload[0]) << 24) |
                             (static_cast<TUInt32>(responsePayload[1]) << 16) |
                             (static_cast<TUInt32>(responsePayload[2]) << 8) |
                              static_cast<TUInt32>(responsePayload[3]);
        context.SetVariable("response_status_code", static_cast<TInt64>(statusCode));

        LOG_DEBUG("EthernetCommandTestStep", "Response status code: 0x{:08X}", statusCode);
    }

    return TESTMATE_SUCCESS();
}

void CEthernetCommandTestStep::Abort() {
    m_aborted = true;
    LOG_WARNING("EthernetCommandTestStep", "Test step aborted");
}

} // namespace TestMATE
