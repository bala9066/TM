/**************************************************************************
 * File Name: DUTCommTestStep.h
 * Description: Example test step using custom communication protocols
 * Author: TestMATE Development Team
 *
 * This demonstrates how to create test steps that use the custom
 * Ethernet or UART protocols to communicate with your DUT.
 **************************************************************************/

#pragma once

#include "EthernetConnection.h"
#include "CustomUARTProtocol.h"
#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <unordered_map>
#include <variant>

namespace TestMATE {

/**
 * @brief Simple context for storing test step variables
 *
 * This is a simplified example context for demonstration purposes.
 * In production code, you would use TestMATE's full ExecutionContext.
 */
struct SSimpleContext {
    std::unordered_map<TString, std::variant<TInt64, TDouble, TString>> variables;

    template<typename T>
    void SetVariable(const TString& name, const T& value) {
        variables[name] = value;
    }

    template<typename T>
    std::optional<T> GetVariable(const TString& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) {
            if (auto* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return std::nullopt;
    }
};

/**
 * @class CDUTRegisterTestStep
 * @brief Example class for reading/writing DUT registers
 *
 * This demonstrates how to:
 * - Read/write registers via custom UART protocol
 * - Validate register values
 * - Store results in a context
 * - Handle errors properly
 *
 * NOTE: This is a simplified example. For production use, implement
 * the full ITestStep interface from core/test_sequence/ITestStep.h
 */
class CDUTRegisterTestStep {
public:
    /**
     * @brief Test operation type
     */
    enum class EOperation {
        kRead,   ///< Read register
        kWrite,  ///< Write register
        kVerify  ///< Write then read back to verify
    };

    /**
     * @brief Constructor
     * @param in_id Test step ID
     * @param in_name Test step name
     */
    CDUTRegisterTestStep(const TString& in_id, const TString& in_name);

    /**
     * @brief Destructor
     */
    ~CDUTRegisterTestStep() = default;

    // Execution interface (simplified for example)
    CResult Execute(SSimpleContext& context);
    void Abort();

    /**
     * @brief Set register address to access
     * @param in_address Register address (16-bit)
     */
    void SetRegisterAddress(TUInt16 in_address) { m_address = in_address; }

    /**
     * @brief Set expected register value for verification
     * @param in_value Expected value (32-bit)
     */
    void SetExpectedValue(TUInt32 in_value) { m_expectedValue = in_value; }

    /**
     * @brief Set value to write to register
     * @param in_value Value to write (32-bit)
     */
    void SetWriteValue(TUInt32 in_value) { m_writeValue = in_value; }

    /**
     * @brief Set bit mask for value comparison
     * @param in_mask Mask to apply (0xFFFFFFFF = compare all bits)
     */
    void SetMask(TUInt32 in_mask) { m_mask = in_mask; }

    /**
     * @brief Set test operation
     * @param in_operation Operation type (Read/Write/Verify)
     */
    void SetOperation(EOperation in_operation) { m_operation = in_operation; }

    /**
     * @brief Set UART protocol handler
     * @param in_pProtocol UART protocol instance
     */
    void SetUARTProtocol(CCustomUARTProtocol* in_pProtocol) { m_pUARTProtocol = in_pProtocol; }

private:
    TUInt16 m_address;                  ///< Register address
    TUInt32 m_expectedValue;            ///< Expected value for verification
    TUInt32 m_writeValue;               ///< Value to write
    TUInt32 m_mask;                     ///< Bit mask for comparison
    EOperation m_operation;             ///< Operation type
    CCustomUARTProtocol* m_pUARTProtocol; ///< UART protocol handler
    bool m_aborted;                     ///< Abort flag
};

/**
 * @class CEthernetCommandTestStep
 * @brief Example class for sending commands via Ethernet
 *
 * This demonstrates how to:
 * - Send custom packets over Ethernet
 * - Receive and validate responses
 * - Handle timeouts
 *
 * NOTE: This is a simplified example. For production use, implement
 * the full ITestStep interface from core/test_sequence/ITestStep.h
 */
class CEthernetCommandTestStep {
public:
    /**
     * @brief Constructor
     * @param in_id Test step ID
     * @param in_name Test step name
     */
    CEthernetCommandTestStep(const TString& in_id, const TString& in_name);

    /**
     * @brief Destructor
     */
    ~CEthernetCommandTestStep() = default;

    // Execution interface (simplified for example)
    CResult Execute(SSimpleContext& context);
    void Abort();

    /**
     * @brief Set Ethernet connection instance
     * @param in_pConnection Ethernet connection
     */
    void SetConnection(CEthernetConnection* in_pConnection) { m_pConnection = in_pConnection; }

    /**
     * @brief Set command packet type
     * @param in_packetType Packet type ID
     */
    void SetPacketType(TUInt8 in_packetType) { m_packetType = in_packetType; }

    /**
     * @brief Set command payload
     * @param in_payload Payload bytes
     */
    void SetPayload(const TVector<TUInt8>& in_payload) { m_payload = in_payload; }

    /**
     * @brief Set expected response packet type
     * @param in_expectedType Expected response type
     */
    void SetExpectedResponseType(TUInt8 in_expectedType) { m_expectedResponseType = in_expectedType; }

    /**
     * @brief Set response timeout
     * @param in_timeoutMs Timeout in milliseconds
     */
    void SetTimeout(TUInt32 in_timeoutMs) { m_timeoutMs = in_timeoutMs; }

private:
    CEthernetConnection* m_pConnection; ///< Ethernet connection
    TUInt8 m_packetType;                ///< Command packet type
    TVector<TUInt8> m_payload;          ///< Command payload
    TUInt8 m_expectedResponseType;      ///< Expected response type
    TUInt32 m_timeoutMs;                ///< Response timeout
    bool m_aborted;                     ///< Abort flag
};

} // namespace TestMATE
