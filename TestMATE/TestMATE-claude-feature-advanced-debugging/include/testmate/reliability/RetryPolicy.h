/**
 * @file RetryPolicy.h
 * @brief Test retry and recovery system for production reliability
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the retry and recovery system that allows tests to
 * automatically retry on transient failures, improving test reliability
 * in production environments.
 */

#ifndef TESTMATE_RELIABILITY_RETRY_POLICY_H
#define TESTMATE_RELIABILITY_RETRY_POLICY_H

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <chrono>
#include <vector>

namespace TestMATE {

/**
 * @brief Retry strategy types
 */
enum class ERetryStrategy {
    kFixed,           ///< Fixed delay between retries
    kLinear,          ///< Linearly increasing delay
    kExponential,     ///< Exponentially increasing delay (exponential backoff)
    kJittered         ///< Exponential with random jitter to avoid thundering herd
};

/**
 * @brief Retry policy configuration
 */
struct SRetryPolicyConfig {
    TUInt32 maxRetries{3};                           ///< Maximum number of retry attempts
    TUInt32 initialDelayMs{100};                     ///< Initial delay in milliseconds
    TUInt32 maxDelayMs{5000};                        ///< Maximum delay in milliseconds
    TDouble backoffMultiplier{2.0};                  ///< Multiplier for exponential backoff
    TDouble jitterFactor{0.1};                       ///< Jitter factor (0.0 to 1.0)
    ERetryStrategy strategy{ERetryStrategy::kExponential};
    bool retryOnTimeout{true};                       ///< Retry on timeout errors
    bool retryOnConnectionError{true};               ///< Retry on connection errors
    bool retryOnTransientError{true};                ///< Retry on transient errors
    std::vector<EErrorCode> retryableErrors;         ///< Specific error codes to retry
    std::vector<EErrorCode> nonRetryableErrors;      ///< Error codes that should never retry
};

/**
 * @brief Retry attempt information
 */
struct SRetryAttempt {
    TUInt32 attemptNumber{0};                        ///< Current attempt number (0 = first try)
    TUInt32 totalAttempts{0};                        ///< Total attempts made
    TUInt32 delayMs{0};                              ///< Delay before this attempt
    TTimePoint attemptTime;                          ///< Time of this attempt
    CResult lastResult;                              ///< Result of last attempt
    TString lastError;                               ///< Error message from last attempt
};

/**
 * @brief Retry statistics
 */
struct SRetryStatistics {
    TUInt32 totalOperations{0};                      ///< Total operations attempted
    TUInt32 successOnFirstTry{0};                    ///< Succeeded without retry
    TUInt32 succeededAfterRetry{0};                  ///< Succeeded after retrying
    TUInt32 failedAfterAllRetries{0};                ///< Failed after all retries
    TUInt32 totalRetries{0};                         ///< Total retry attempts made
    TDouble averageRetriesPerOperation{0.0};         ///< Average retries per operation
    TDouble successRateAfterRetry{0.0};              ///< Success rate with retries
};

/**
 * @brief Callback for retry events
 */
using FRetryCallback = std::function<void(const SRetryAttempt&)>;

/**
 * @brief Test retry and recovery policy manager
 *
 * CRetryPolicy provides automatic retry logic for test steps that may fail
 * due to transient errors. It supports multiple retry strategies and
 * configurable error handling.
 *
 * Example usage:
 * @code
 * SRetryPolicyConfig config;
 * config.maxRetries = 3;
 * config.strategy = ERetryStrategy::kExponential;
 * config.initialDelayMs = 100;
 *
 * CRetryPolicy retryPolicy(config);
 *
 * // Execute with automatic retry
 * auto result = retryPolicy.ExecuteWithRetry([&]() {
 *     return PerformNetworkOperation();
 * });
 *
 * if (result.IsSuccess()) {
 *     std::cout << "Operation succeeded after "
 *               << retryPolicy.GetLastAttemptCount() << " attempts\n";
 * }
 * @endcode
 */
class CRetryPolicy {
public:
    /**
     * @brief Construct retry policy with configuration
     * @param in_config Retry policy configuration
     */
    explicit CRetryPolicy(const SRetryPolicyConfig& in_config = {});

    /**
     * @brief Execute operation with automatic retry
     * @param in_operation Function to execute
     * @return Result of operation (success or final failure)
     */
    CResult ExecuteWithRetry(std::function<CResult()> in_operation);

    /**
     * @brief Check if an error code is retryable
     * @param in_errorCode Error code to check
     * @return true if error is retryable
     */
    [[nodiscard]] bool IsRetryable(EErrorCode in_errorCode) const;

    /**
     * @brief Calculate delay for next retry attempt
     * @param in_attemptNumber Current attempt number (0-based)
     * @return Delay in milliseconds
     */
    [[nodiscard]] TUInt32 CalculateDelay(TUInt32 in_attemptNumber) const;

    /**
     * @brief Set callback for retry events
     * @param in_callback Callback function
     */
    void SetRetryCallback(FRetryCallback in_callback);

    /**
     * @brief Get last attempt information
     * @return Last retry attempt info
     */
    [[nodiscard]] const SRetryAttempt& GetLastAttempt() const { return m_lastAttempt; }

    /**
     * @brief Get number of attempts for last operation
     * @return Attempt count
     */
    [[nodiscard]] TUInt32 GetLastAttemptCount() const { return m_lastAttempt.totalAttempts; }

    /**
     * @brief Get retry statistics
     * @return Statistics structure
     */
    [[nodiscard]] SRetryStatistics GetStatistics() const { return m_statistics; }

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

    /**
     * @brief Update configuration
     * @param in_config New configuration
     */
    void UpdateConfig(const SRetryPolicyConfig& in_config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    [[nodiscard]] const SRetryPolicyConfig& GetConfig() const { return m_config; }

private:
    /**
     * @brief Apply delay before retry
     * @param in_delayMs Delay in milliseconds
     */
    void ApplyDelay(TUInt32 in_delayMs);

    /**
     * @brief Add jitter to delay
     * @param in_baseDelay Base delay in milliseconds
     * @return Jittered delay
     */
    [[nodiscard]] TUInt32 AddJitter(TUInt32 in_baseDelay) const;

    /**
     * @brief Update statistics after operation
     * @param in_succeeded Whether operation succeeded
     * @param in_retries Number of retries performed
     */
    void UpdateStatistics(bool in_succeeded, TUInt32 in_retries);

    SRetryPolicyConfig m_config;
    SRetryAttempt m_lastAttempt;
    SRetryStatistics m_statistics;
    FRetryCallback m_retryCallback;
};

/**
 * @brief Circuit breaker configuration
 */
struct SCircuitBreakerConfig {
    TUInt32 failureThreshold{5};                 ///< Failures before opening circuit
    TUInt32 successThreshold{2};                 ///< Successes in half-open to close
    TUInt32 timeoutMs{60000};                    ///< Time before half-open attempt
    TUInt32 halfOpenMaxAttempts{3};              ///< Max attempts in half-open state
};

/**
 * @brief Helper class for circuit breaker pattern
 *
 * CCircuitBreaker prevents repeated attempts to execute operations
 * that are likely to fail, allowing the system to recover gracefully.
 */
class CCircuitBreaker {
public:
    enum class EState {
        kClosed,      ///< Normal operation, requests pass through
        kOpen,        ///< Failure threshold exceeded, requests fail fast
        kHalfOpen     ///< Testing if system has recovered
    };

    explicit CCircuitBreaker(const SCircuitBreakerConfig& in_config = {});

    /**
     * @brief Execute operation through circuit breaker
     * @param in_operation Function to execute
     * @return Result of operation
     */
    CResult Execute(std::function<CResult()> in_operation);

    /**
     * @brief Get current circuit state
     * @return Current state
     */
    [[nodiscard]] EState GetState() const { return m_state; }

    /**
     * @brief Reset circuit breaker
     */
    void Reset();

private:
    void RecordSuccess();
    void RecordFailure();
    bool ShouldAttempt();

    SCircuitBreakerConfig m_config;
    EState m_state{EState::kClosed};
    TUInt32 m_failureCount{0};
    TUInt32 m_successCount{0};
    TTimePoint m_lastFailureTime;
    TUInt32 m_halfOpenAttempts{0};
};

} // namespace TestMATE

#endif // TESTMATE_RELIABILITY_RETRY_POLICY_H
