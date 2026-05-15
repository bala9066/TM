/**************************************************************************
 * File Name: RetryPolicyTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Unit tests for retry policy and circuit breaker
 **************************************************************************/

#include <gtest/gtest.h>
#include "testmate/reliability/RetryPolicy.h"
#include <atomic>
#include <thread>
#include <chrono>

using namespace TestMATE;

//=============================================================================
// Test Fixtures
//=============================================================================

class RetryPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_callCount = 0;
        m_shouldSucceed = false;
    }

    CResult SuccessOperation() {
        m_callCount++;
        return TESTMATE_SUCCESS();
    }

    CResult FailureOperation() {
        m_callCount++;
        return TESTMATE_FAILURE(EErrorCode::kRetryable, "Transient failure");
    }

    CResult ConditionalOperation() {
        m_callCount++;
        if (m_shouldSucceed || m_callCount >= 3) {
            return TESTMATE_SUCCESS();
        }
        return TESTMATE_FAILURE(EErrorCode::kRetryable, "Not ready yet");
    }

    CResult TimeoutOperation() {
        m_callCount++;
        return TESTMATE_FAILURE(EErrorCode::kTimeout, "Operation timed out");
    }

    CResult NonRetryableOperation() {
        m_callCount++;
        return TESTMATE_FAILURE(EErrorCode::kInvalidArgument, "Invalid argument");
    }

    std::atomic<TUInt32> m_callCount{0};
    std::atomic<bool> m_shouldSucceed{false};
};

class CircuitBreakerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_callCount = 0;
    }

    CResult SuccessOperation() {
        m_callCount++;
        return TESTMATE_SUCCESS();
    }

    CResult FailureOperation() {
        m_callCount++;
        return TESTMATE_FAILURE(EErrorCode::kConnectionFailed, "Connection failed");
    }

    std::atomic<TUInt32> m_callCount{0};
};

//=============================================================================
// Basic Retry Tests
//=============================================================================

TEST_F(RetryPolicyTest, SuccessOnFirstTry) {
    SRetryPolicyConfig config;
    config.maxRetries = 3;

    CRetryPolicy policy(config);

    auto result = policy.ExecuteWithRetry([this]() { return SuccessOperation(); });

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_callCount, 1);
    EXPECT_EQ(policy.GetLastAttemptCount(), 1);

    auto stats = policy.GetStatistics();
    EXPECT_EQ(stats.totalOperations, 1);
    EXPECT_EQ(stats.successOnFirstTry, 1);
    EXPECT_EQ(stats.succeededAfterRetry, 0);
    EXPECT_EQ(stats.failedAfterAllRetries, 0);
}

TEST_F(RetryPolicyTest, SuccessAfterRetry) {
    SRetryPolicyConfig config;
    config.maxRetries = 3;
    config.initialDelayMs = 10;

    CRetryPolicy policy(config);

    auto result = policy.ExecuteWithRetry([this]() { return ConditionalOperation(); });

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_callCount, 3);  // Fails on attempt 1 and 2, succeeds on 3
    EXPECT_EQ(policy.GetLastAttemptCount(), 3);

    auto stats = policy.GetStatistics();
    EXPECT_EQ(stats.totalOperations, 1);
    EXPECT_EQ(stats.successOnFirstTry, 0);
    EXPECT_EQ(stats.succeededAfterRetry, 1);
    EXPECT_EQ(stats.totalRetries, 2);  // 2 retries after initial attempt
}

TEST_F(RetryPolicyTest, FailureAfterAllRetries) {
    SRetryPolicyConfig config;
    config.maxRetries = 3;
    config.initialDelayMs = 10;

    CRetryPolicy policy(config);

    auto result = policy.ExecuteWithRetry([this]() { return FailureOperation(); });

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(m_callCount, 4);  // 1 initial + 3 retries
    EXPECT_EQ(policy.GetLastAttemptCount(), 4);

    auto stats = policy.GetStatistics();
    EXPECT_EQ(stats.totalOperations, 1);
    EXPECT_EQ(stats.successOnFirstTry, 0);
    EXPECT_EQ(stats.succeededAfterRetry, 0);
    EXPECT_EQ(stats.failedAfterAllRetries, 1);
    EXPECT_EQ(stats.totalRetries, 3);
}

TEST_F(RetryPolicyTest, NullOperationReturnsError) {
    CRetryPolicy policy;

    auto result = policy.ExecuteWithRetry(nullptr);

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidArgument);
}

//=============================================================================
// Retry Strategy Tests
//=============================================================================

TEST_F(RetryPolicyTest, FixedDelayStrategy) {
    SRetryPolicyConfig config;
    config.strategy = ERetryStrategy::kFixed;
    config.initialDelayMs = 100;
    config.maxRetries = 3;

    CRetryPolicy policy(config);

    EXPECT_EQ(policy.CalculateDelay(0), 100);
    EXPECT_EQ(policy.CalculateDelay(1), 100);
    EXPECT_EQ(policy.CalculateDelay(2), 100);
}

TEST_F(RetryPolicyTest, LinearDelayStrategy) {
    SRetryPolicyConfig config;
    config.strategy = ERetryStrategy::kLinear;
    config.initialDelayMs = 100;
    config.maxRetries = 3;

    CRetryPolicy policy(config);

    EXPECT_EQ(policy.CalculateDelay(0), 100);   // 100 * 1
    EXPECT_EQ(policy.CalculateDelay(1), 200);   // 100 * 2
    EXPECT_EQ(policy.CalculateDelay(2), 300);   // 100 * 3
}

TEST_F(RetryPolicyTest, ExponentialDelayStrategy) {
    SRetryPolicyConfig config;
    config.strategy = ERetryStrategy::kExponential;
    config.initialDelayMs = 100;
    config.backoffMultiplier = 2.0;
    config.maxRetries = 4;

    CRetryPolicy policy(config);

    EXPECT_EQ(policy.CalculateDelay(0), 100);   // 100 * 2^0 = 100
    EXPECT_EQ(policy.CalculateDelay(1), 200);   // 100 * 2^1 = 200
    EXPECT_EQ(policy.CalculateDelay(2), 400);   // 100 * 2^2 = 400
    EXPECT_EQ(policy.CalculateDelay(3), 800);   // 100 * 2^3 = 800
}

TEST_F(RetryPolicyTest, MaxDelayEnforced) {
    SRetryPolicyConfig config;
    config.strategy = ERetryStrategy::kExponential;
    config.initialDelayMs = 100;
    config.backoffMultiplier = 2.0;
    config.maxDelayMs = 500;

    CRetryPolicy policy(config);

    EXPECT_EQ(policy.CalculateDelay(0), 100);
    EXPECT_EQ(policy.CalculateDelay(1), 200);
    EXPECT_EQ(policy.CalculateDelay(2), 400);
    EXPECT_EQ(policy.CalculateDelay(3), 500);  // Capped at max
    EXPECT_EQ(policy.CalculateDelay(4), 500);  // Still capped
}

TEST_F(RetryPolicyTest, JitteredDelayVariation) {
    SRetryPolicyConfig config;
    config.strategy = ERetryStrategy::kJittered;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0;
    config.jitterFactor = 0.2;  // ±20% variation

    CRetryPolicy policy(config);

    // Run multiple times to verify jitter variation
    bool hasVariation = false;
    TUInt32 firstDelay = policy.CalculateDelay(1);

    for (int i = 0; i < 10; ++i) {
        TUInt32 delay = policy.CalculateDelay(1);
        // Should be within ±20% of 2000ms (base value for attempt 1)
        EXPECT_GE(delay, 1600);  // 2000 - 400
        EXPECT_LE(delay, 2400);  // 2000 + 400

        if (delay != firstDelay) {
            hasVariation = true;
        }
    }

    // With 10 attempts, we should see some variation (not guaranteed but very likely)
    EXPECT_TRUE(hasVariation);
}

//=============================================================================
// Error Retryability Tests
//=============================================================================

TEST_F(RetryPolicyTest, TimeoutIsRetryableByDefault) {
    SRetryPolicyConfig config;
    config.retryOnTimeout = true;

    CRetryPolicy policy(config);

    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kTimeout));
}

TEST_F(RetryPolicyTest, TimeoutNotRetryableWhenDisabled) {
    SRetryPolicyConfig config;
    config.retryOnTimeout = false;

    CRetryPolicy policy(config);

    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kTimeout));
}

TEST_F(RetryPolicyTest, ConnectionErrorsRetryable) {
    SRetryPolicyConfig config;
    config.retryOnConnectionError = true;

    CRetryPolicy policy(config);

    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kConnectionFailed));
    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kConnectionLost));
}

TEST_F(RetryPolicyTest, TransientErrorsRetryable) {
    SRetryPolicyConfig config;
    config.retryOnTransientError = true;

    CRetryPolicy policy(config);

    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kTemporaryFailure));
    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kResourceBusy));
    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kRetryable));
}

TEST_F(RetryPolicyTest, NonRetryableErrors) {
    CRetryPolicy policy;

    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kInvalidArgument));
    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kNotFound));
    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kPermissionDenied));
    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kNotImplemented));
}

TEST_F(RetryPolicyTest, CustomRetryableErrors) {
    SRetryPolicyConfig config;
    config.retryableErrors = {EErrorCode::kInvalidState, EErrorCode::kStepFailed};

    CRetryPolicy policy(config);

    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kInvalidState));
    EXPECT_TRUE(policy.IsRetryable(EErrorCode::kStepFailed));
    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kTimeout));  // Not in list
}

TEST_F(RetryPolicyTest, NonRetryableOverridesRetryable) {
    SRetryPolicyConfig config;
    config.retryOnTimeout = true;
    config.nonRetryableErrors = {EErrorCode::kTimeout};

    CRetryPolicy policy(config);

    EXPECT_FALSE(policy.IsRetryable(EErrorCode::kTimeout));
}

TEST_F(RetryPolicyTest, StopRetryingOnNonRetryableError) {
    SRetryPolicyConfig config;
    config.maxRetries = 5;
    config.initialDelayMs = 10;

    CRetryPolicy policy(config);

    auto result = policy.ExecuteWithRetry([this]() { return NonRetryableOperation(); });

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(m_callCount, 1);  // Should not retry invalid argument error
}

//=============================================================================
// Statistics Tests
//=============================================================================

TEST_F(RetryPolicyTest, StatisticsTracking) {
    SRetryPolicyConfig config;
    config.maxRetries = 2;
    config.initialDelayMs = 10;

    CRetryPolicy policy(config);

    // Operation 1: Success on first try
    policy.ExecuteWithRetry([this]() { return SuccessOperation(); });

    // Operation 2: Success after retry
    m_callCount = 0;
    policy.ExecuteWithRetry([this]() { return ConditionalOperation(); });

    // Operation 3: Failure after all retries
    m_callCount = 0;
    m_shouldSucceed = false;
    policy.ExecuteWithRetry([this]() { return FailureOperation(); });

    auto stats = policy.GetStatistics();
    EXPECT_EQ(stats.totalOperations, 3);
    EXPECT_EQ(stats.successOnFirstTry, 1);
    EXPECT_EQ(stats.succeededAfterRetry, 1);
    EXPECT_EQ(stats.failedAfterAllRetries, 1);
    EXPECT_EQ(stats.totalRetries, 4);  // 0 + 2 + 2
    EXPECT_DOUBLE_EQ(stats.averageRetriesPerOperation, 4.0 / 3.0);
    EXPECT_DOUBLE_EQ(stats.successRateAfterRetry, 2.0 / 3.0);
}

TEST_F(RetryPolicyTest, ResetStatistics) {
    CRetryPolicy policy;

    policy.ExecuteWithRetry([this]() { return SuccessOperation(); });

    auto stats1 = policy.GetStatistics();
    EXPECT_EQ(stats1.totalOperations, 1);

    policy.ResetStatistics();

    auto stats2 = policy.GetStatistics();
    EXPECT_EQ(stats2.totalOperations, 0);
    EXPECT_EQ(stats2.successOnFirstTry, 0);
    EXPECT_EQ(stats2.succeededAfterRetry, 0);
    EXPECT_EQ(stats2.failedAfterAllRetries, 0);
}

//=============================================================================
// Callback Tests
//=============================================================================

TEST_F(RetryPolicyTest, RetryCallback) {
    SRetryPolicyConfig config;
    config.maxRetries = 3;
    config.initialDelayMs = 10;

    CRetryPolicy policy(config);

    TUInt32 callbackCount = 0;
    TUInt32 lastAttemptNumber = 0;

    policy.SetRetryCallback([&](const SRetryAttempt& attempt) {
        callbackCount++;
        lastAttemptNumber = attempt.attemptNumber;
    });

    policy.ExecuteWithRetry([this]() { return FailureOperation(); });

    EXPECT_EQ(callbackCount, 3);  // Called before each retry (not initial attempt)
    EXPECT_EQ(lastAttemptNumber, 2);  // Last retry is attempt 2 (0-indexed)
}

//=============================================================================
// Configuration Tests
//=============================================================================

TEST_F(RetryPolicyTest, UpdateConfiguration) {
    SRetryPolicyConfig config1;
    config1.maxRetries = 2;

    CRetryPolicy policy(config1);

    EXPECT_EQ(policy.GetConfig().maxRetries, 2);

    SRetryPolicyConfig config2;
    config2.maxRetries = 5;

    policy.UpdateConfig(config2);

    EXPECT_EQ(policy.GetConfig().maxRetries, 5);
}

//=============================================================================
// Circuit Breaker Tests
//=============================================================================

TEST_F(CircuitBreakerTest, InitialStateIsClosed) {
    SCircuitBreakerConfig config;
    CCircuitBreaker breaker(config);

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kClosed);
}

TEST_F(CircuitBreakerTest, SuccessfulOperationsKeepCircuitClosed) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 3;

    CCircuitBreaker breaker(config);

    for (int i = 0; i < 5; ++i) {
        auto result = breaker.Execute([this]() { return SuccessOperation(); });
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kClosed);
    }

    EXPECT_EQ(m_callCount, 5);
}

TEST_F(CircuitBreakerTest, FailuresOpenCircuit) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 3;

    CCircuitBreaker breaker(config);

    // Fail 3 times
    for (int i = 0; i < 3; ++i) {
        auto result = breaker.Execute([this]() { return FailureOperation(); });
        EXPECT_FALSE(result.IsSuccess());
    }

    // Circuit should now be open
    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kOpen);
    EXPECT_EQ(m_callCount, 3);
}

TEST_F(CircuitBreakerTest, OpenCircuitRejectsOperations) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.timeoutMs = 1000;  // 1 second

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kOpen);

    // Try to execute - should fail fast without calling operation
    auto result = breaker.Execute([this]() { return SuccessOperation(); });

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kCircuitBreakerOpen);
    EXPECT_EQ(m_callCount, 2);  // Should not have incremented
}

TEST_F(CircuitBreakerTest, TransitionToHalfOpenAfterTimeout) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.timeoutMs = 100;  // 100ms timeout

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kOpen);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Next attempt should transition to half-open
    auto result = breaker.Execute([this]() { return SuccessOperation(); });

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kHalfOpen);
}

TEST_F(CircuitBreakerTest, HalfOpenToClosedOnSuccess) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.successThreshold = 2;
    config.timeoutMs = 50;

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    // Wait and transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    breaker.Execute([this]() { return SuccessOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kHalfOpen);

    // Second success should close circuit
    breaker.Execute([this]() { return SuccessOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kClosed);
}

TEST_F(CircuitBreakerTest, HalfOpenToOpenOnFailure) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.timeoutMs = 50;

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    // Wait and transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    breaker.Execute([this]() { return SuccessOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kHalfOpen);

    // Failure should reopen circuit
    breaker.Execute([this]() { return FailureOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kOpen);
}

TEST_F(CircuitBreakerTest, ResetCircuitBreaker) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kOpen);

    // Reset
    breaker.Reset();

    EXPECT_EQ(breaker.GetState(), CCircuitBreaker::EState::kClosed);

    // Should work normally
    auto result = breaker.Execute([this]() { return SuccessOperation(); });
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(CircuitBreakerTest, HalfOpenMaxAttempts) {
    SCircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.successThreshold = 10;  // High threshold to keep circuit in half-open
    config.halfOpenMaxAttempts = 2;
    config.timeoutMs = 50;

    CCircuitBreaker breaker(config);

    // Open the circuit
    breaker.Execute([this]() { return FailureOperation(); });
    breaker.Execute([this]() { return FailureOperation(); });

    // Wait and transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TUInt32 attemptsBefore = m_callCount;

    // Try 3 times (max is 2)
    for (int i = 0; i < 3; ++i) {
        breaker.Execute([this]() { return SuccessOperation(); });
    }

    // Only 2 should have been allowed (circuit stays in half-open due to high successThreshold)
    EXPECT_EQ(m_callCount - attemptsBefore, 2);
}

// Note: main() is provided by gtest_main library in CMakeLists.txt
