/**************************************************************************
 * File Name: RetryPolicy.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Test retry and recovery system implementation
 **************************************************************************/

#include "testmate/reliability/RetryPolicy.h"
#include <thread>
#include <random>
#include <algorithm>
#include <cmath>

namespace TestMATE {

//=============================================================================
// CRetryPolicy Implementation
//=============================================================================

CRetryPolicy::CRetryPolicy(const SRetryPolicyConfig& in_config)
    : m_config(in_config)
{
}

CResult CRetryPolicy::ExecuteWithRetry(std::function<CResult()> in_operation) {
    if (!in_operation) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Operation cannot be null");
    }

    m_lastAttempt = SRetryAttempt();
    m_statistics.totalOperations++;

    CResult lastResult;
    TUInt32 attemptCount = 0;
    bool succeeded = false;

    // First attempt (not a retry)
    m_lastAttempt.attemptNumber = 0;
    m_lastAttempt.totalAttempts = 1;
    m_lastAttempt.attemptTime = std::chrono::steady_clock::now();

    lastResult = in_operation();

    if (lastResult.IsSuccess()) {
        m_statistics.successOnFirstTry++;
        UpdateStatistics(true, 0);
        return lastResult;
    }

    // Check if error is retryable
    if (!IsRetryable(lastResult.GetCode())) {
        m_statistics.failedAfterAllRetries++;
        UpdateStatistics(false, 0);
        return lastResult;
    }

    m_lastAttempt.lastResult = lastResult;
    m_lastAttempt.lastError = lastResult.GetMessage();

    // Retry loop
    for (TUInt32 retryCount = 0; retryCount < m_config.maxRetries; ++retryCount) {
        attemptCount++;

        // Calculate delay for this retry
        TUInt32 delayMs = CalculateDelay(retryCount);
        m_lastAttempt.delayMs = delayMs;

        // Notify callback before delay
        if (m_retryCallback) {
            m_retryCallback(m_lastAttempt);
        }

        // Apply delay
        ApplyDelay(delayMs);

        // Retry the operation
        m_lastAttempt.attemptNumber = retryCount + 1;
        m_lastAttempt.totalAttempts = attemptCount + 1;
        m_lastAttempt.attemptTime = std::chrono::steady_clock::now();

        lastResult = in_operation();

        m_lastAttempt.lastResult = lastResult;
        m_lastAttempt.lastError = lastResult.GetMessage();

        if (lastResult.IsSuccess()) {
            succeeded = true;
            m_statistics.succeededAfterRetry++;
            m_statistics.totalRetries += attemptCount;
            UpdateStatistics(true, attemptCount);
            return lastResult;
        }

        // Check if we should continue retrying
        if (!IsRetryable(lastResult.GetCode())) {
            break;
        }
    }

    // All retries exhausted
    m_statistics.failedAfterAllRetries++;
    m_statistics.totalRetries += attemptCount;
    UpdateStatistics(false, attemptCount);

    return lastResult;
}

bool CRetryPolicy::IsRetryable(EErrorCode in_errorCode) const {
    // Check non-retryable errors first
    for (const auto& code : m_config.nonRetryableErrors) {
        if (code == in_errorCode) {
            return false;
        }
    }

    // Check specific retryable errors
    if (!m_config.retryableErrors.empty()) {
        for (const auto& code : m_config.retryableErrors) {
            if (code == in_errorCode) {
                return true;
            }
        }
        return false;  // Not in retryable list
    }

    // Check transient error categories
    switch (in_errorCode) {
        case EErrorCode::kTimeout:
            return m_config.retryOnTimeout;

        case EErrorCode::kConnectionFailed:
        case EErrorCode::kConnectionLost:
            return m_config.retryOnConnectionError;

        case EErrorCode::kTemporaryFailure:
        case EErrorCode::kResourceBusy:
        case EErrorCode::kRetryable:
            return m_config.retryOnTransientError;

        // Non-retryable errors
        case EErrorCode::kInvalidArgument:
        case EErrorCode::kNotFound:
        case EErrorCode::kPermissionDenied:
        case EErrorCode::kNotImplemented:
            return false;

        default:
            // By default, retry on unknown errors if transient retry is enabled
            return m_config.retryOnTransientError;
    }
}

TUInt32 CRetryPolicy::CalculateDelay(TUInt32 in_attemptNumber) const {
    TUInt32 baseDelay = 0;

    switch (m_config.strategy) {
        case ERetryStrategy::kFixed:
            baseDelay = m_config.initialDelayMs;
            break;

        case ERetryStrategy::kLinear:
            baseDelay = m_config.initialDelayMs * (in_attemptNumber + 1);
            break;

        case ERetryStrategy::kExponential:
        case ERetryStrategy::kJittered: {
            TDouble multiplier = std::pow(m_config.backoffMultiplier, in_attemptNumber);
            baseDelay = static_cast<TUInt32>(m_config.initialDelayMs * multiplier);
            break;
        }
    }

    // Cap at maximum delay
    baseDelay = std::min(baseDelay, m_config.maxDelayMs);

    // Add jitter if configured
    if (m_config.strategy == ERetryStrategy::kJittered) {
        baseDelay = AddJitter(baseDelay);
    }

    return baseDelay;
}

void CRetryPolicy::SetRetryCallback(FRetryCallback in_callback) {
    m_retryCallback = std::move(in_callback);
}

void CRetryPolicy::ResetStatistics() {
    m_statistics = SRetryStatistics();
}

void CRetryPolicy::UpdateConfig(const SRetryPolicyConfig& in_config) {
    m_config = in_config;
}

void CRetryPolicy::ApplyDelay(TUInt32 in_delayMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(in_delayMs));
}

TUInt32 CRetryPolicy::AddJitter(TUInt32 in_baseDelay) const {
    if (m_config.jitterFactor <= 0.0) {
        return in_baseDelay;
    }

    // Random number generator
    static thread_local std::mt19937 generator(std::random_device{}());

    // Calculate jitter range
    TDouble jitterRange = in_baseDelay * m_config.jitterFactor;

    // Uniform distribution: [-jitterRange, +jitterRange]
    std::uniform_real_distribution<TDouble> distribution(-jitterRange, jitterRange);

    TDouble jitter = distribution(generator);
    TInt32 jitteredDelay = static_cast<TInt32>(in_baseDelay + jitter);

    // Ensure non-negative
    return static_cast<TUInt32>(std::max(0, jitteredDelay));
}

void CRetryPolicy::UpdateStatistics(bool in_succeeded, TUInt32 in_retries) {
    if (m_statistics.totalOperations > 0) {
        m_statistics.averageRetriesPerOperation =
            static_cast<TDouble>(m_statistics.totalRetries) / m_statistics.totalOperations;

        TUInt32 totalSuccesses = m_statistics.successOnFirstTry + m_statistics.succeededAfterRetry;
        m_statistics.successRateAfterRetry =
            static_cast<TDouble>(totalSuccesses) / m_statistics.totalOperations;
    }
}

//=============================================================================
// CCircuitBreaker Implementation
//=============================================================================

CCircuitBreaker::CCircuitBreaker(const SCircuitBreakerConfig& in_config)
    : m_config(in_config)
{
}

CResult CCircuitBreaker::Execute(std::function<CResult()> in_operation) {
    if (!in_operation) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Operation cannot be null");
    }

    if (!ShouldAttempt()) {
        return TESTMATE_FAILURE(EErrorCode::kCircuitBreakerOpen,
            "Circuit breaker is open, not attempting operation");
    }

    CResult result = in_operation();

    if (result.IsSuccess()) {
        RecordSuccess();
    } else {
        RecordFailure();
    }

    return result;
}

void CCircuitBreaker::Reset() {
    m_state = EState::kClosed;
    m_failureCount = 0;
    m_successCount = 0;
    m_halfOpenAttempts = 0;
}

bool CCircuitBreaker::ShouldAttempt() {
    switch (m_state) {
        case EState::kClosed:
            // Normal operation
            return true;

        case EState::kOpen: {
            // Check if timeout has elapsed
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_lastFailureTime).count();

            if (elapsed >= m_config.timeoutMs) {
                // Try half-open state
                m_state = EState::kHalfOpen;
                m_halfOpenAttempts = 1;  // Count this first attempt
                return true;
            }
            return false;
        }

        case EState::kHalfOpen:
            // Allow limited attempts in half-open
            if (m_halfOpenAttempts < m_config.halfOpenMaxAttempts) {
                m_halfOpenAttempts++;
                return true;
            }
            return false;
    }

    return false;
}

void CCircuitBreaker::RecordSuccess() {
    switch (m_state) {
        case EState::kClosed:
            // Reset failure count on success
            m_failureCount = 0;
            break;

        case EState::kHalfOpen:
            m_successCount++;
            if (m_successCount >= m_config.successThreshold) {
                // Circuit recovered, close it
                m_state = EState::kClosed;
                m_failureCount = 0;
                m_successCount = 0;
                m_halfOpenAttempts = 0;
            }
            break;

        case EState::kOpen:
            // Should not happen, but handle gracefully
            break;
    }
}

void CCircuitBreaker::RecordFailure() {
    m_lastFailureTime = std::chrono::steady_clock::now();

    switch (m_state) {
        case EState::kClosed:
            m_failureCount++;
            if (m_failureCount >= m_config.failureThreshold) {
                // Open the circuit
                m_state = EState::kOpen;
            }
            break;

        case EState::kHalfOpen:
            // Failure in half-open, go back to open
            m_state = EState::kOpen;
            m_successCount = 0;
            m_halfOpenAttempts = 0;
            break;

        case EState::kOpen:
            // Already open, just update timestamp
            break;
    }
}

} // namespace TestMATE
