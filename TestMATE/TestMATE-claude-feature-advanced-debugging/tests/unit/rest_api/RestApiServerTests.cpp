/**************************************************************************
 * File Name: RestApiServerTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: Unit tests for REST API server
 **************************************************************************/

#include <gtest/gtest.h>
#include "testmate/rest_api/RestApiServer.h"

using namespace TestMATE;

//=============================================================================
// Test Fixtures
//=============================================================================

class RestApiServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SApiServerConfig config;
        config.enableAuth = false;  // Disable auth for most tests
        config.port = 9090;

        m_server = std::make_unique<CRestApiServer>(config);
    }

    std::unique_ptr<CRestApiServer> m_server;
};

//=============================================================================
// Server Lifecycle Tests
//=============================================================================

TEST_F(RestApiServerTest, StartStop) {
    EXPECT_FALSE(m_server->IsRunning());

    auto startResult = m_server->Start();
    EXPECT_TRUE(startResult.IsSuccess());
    EXPECT_TRUE(m_server->IsRunning());

    auto stopResult = m_server->Stop();
    EXPECT_TRUE(stopResult.IsSuccess());
    EXPECT_FALSE(m_server->IsRunning());
}

TEST_F(RestApiServerTest, StartAlreadyRunning) {
    m_server->Start();
    EXPECT_TRUE(m_server->IsRunning());

    auto result = m_server->Start();
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);

    m_server->Stop();
}

TEST_F(RestApiServerTest, StopNotRunning) {
    EXPECT_FALSE(m_server->IsRunning());

    auto result = m_server->Stop();
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kInvalidState);
}

//=============================================================================
// Authentication Tests
//=============================================================================

TEST_F(RestApiServerTest, AddUserAndAuthenticate) {
    m_server->AddUser("testuser", "testpass");

    TString token = m_server->Authenticate("testuser", "testpass");

    EXPECT_FALSE(token.empty());
    EXPECT_TRUE(m_server->ValidateToken(token));
}

TEST_F(RestApiServerTest, AuthenticateInvalidCredentials) {
    m_server->AddUser("testuser", "testpass");

    TString token = m_server->Authenticate("testuser", "wrongpass");

    EXPECT_TRUE(token.empty());
}

TEST_F(RestApiServerTest, AuthenticateNonExistentUser) {
    TString token = m_server->Authenticate("nonexistent", "password");

    EXPECT_TRUE(token.empty());
}

TEST_F(RestApiServerTest, ValidateInvalidToken) {
    bool valid = m_server->ValidateToken("invalid_token");

    EXPECT_FALSE(valid);
}

TEST_F(RestApiServerTest, MultipleUsers) {
    m_server->AddUser("user1", "pass1");
    m_server->AddUser("user2", "pass2");

    TString token1 = m_server->Authenticate("user1", "pass1");
    TString token2 = m_server->Authenticate("user2", "pass2");

    EXPECT_FALSE(token1.empty());
    EXPECT_FALSE(token2.empty());
    EXPECT_NE(token1, token2);

    EXPECT_TRUE(m_server->ValidateToken(token1));
    EXPECT_TRUE(m_server->ValidateToken(token2));
}

//=============================================================================
// Route Registration Tests
//=============================================================================

TEST_F(RestApiServerTest, RegisterCustomRoute) {
    bool handlerCalled = false;

    m_server->RegisterRoute(EHttpMethod::kGet, "/custom/endpoint",
        [&handlerCalled](const SHttpRequest& req) {
            handlerCalled = true;
            SHttpResponse res;
            res.SetJson("{\"message\": \"custom endpoint\"}");
            return res;
        });

    // Verify route was registered by checking statistics
    auto stats = m_server->GetStatistics();
    EXPECT_GT(stats["registered_routes"], 0);
}

TEST_F(RestApiServerTest, RegisterMultipleRoutes) {
    m_server->RegisterRoute(EHttpMethod::kGet, "/route1",
        [](const SHttpRequest& req) { return SHttpResponse(); });

    m_server->RegisterRoute(EHttpMethod::kPost, "/route2",
        [](const SHttpRequest& req) { return SHttpResponse(); });

    m_server->RegisterRoute(EHttpMethod::kPut, "/route3",
        [](const SHttpRequest& req) { return SHttpResponse(); });

    auto stats = m_server->GetStatistics();
    EXPECT_GE(stats["registered_routes"], 3);
}

//=============================================================================
// Test Execution Tests
//=============================================================================

TEST_F(RestApiServerTest, ExecuteTest) {
    STestExecutionRequest req;
    req.testSequenceId = "TEST-001";
    req.async = true;

    TString executionId = m_server->ExecuteTest(req);

    EXPECT_FALSE(executionId.empty());
    EXPECT_TRUE(executionId.find("exec_") == 0);
}

TEST_F(RestApiServerTest, GetExecutionStatus) {
    STestExecutionRequest req;
    req.testSequenceId = "TEST-002";

    TString executionId = m_server->ExecuteTest(req);

    auto status = m_server->GetExecutionStatus(executionId);

    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->executionId, executionId);
    EXPECT_EQ(status->testSequenceId, "TEST-002");
    EXPECT_EQ(status->status, ETestExecutionStatus::kQueued);
}

TEST_F(RestApiServerTest, GetExecutionStatusNotFound) {
    auto status = m_server->GetExecutionStatus("nonexistent_id");

    EXPECT_FALSE(status.has_value());
}

TEST_F(RestApiServerTest, GetAllExecutions) {
    // Execute multiple tests
    for (int i = 0; i < 3; ++i) {
        STestExecutionRequest req;
        req.testSequenceId = "TEST-" + std::to_string(i);
        m_server->ExecuteTest(req);
    }

    auto executions = m_server->GetAllExecutions();

    EXPECT_EQ(executions.size(), 3);
}

TEST_F(RestApiServerTest, CancelExecution) {
    STestExecutionRequest req;
    req.testSequenceId = "TEST-003";

    TString executionId = m_server->ExecuteTest(req);

    auto result = m_server->CancelExecution(executionId);

    EXPECT_TRUE(result.IsSuccess());

    auto status = m_server->GetExecutionStatus(executionId);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->status, ETestExecutionStatus::kCancelled);
}

TEST_F(RestApiServerTest, CancelNonExistentExecution) {
    auto result = m_server->CancelExecution("nonexistent_id");

    EXPECT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetCode(), EErrorCode::kNotFound);
}

//=============================================================================
// Statistics Tests
//=============================================================================

TEST_F(RestApiServerTest, InitialStatistics) {
    auto stats = m_server->GetStatistics();

    EXPECT_EQ(stats["total_requests"], 0);
    EXPECT_EQ(stats["successful_requests"], 0);
    EXPECT_EQ(stats["failed_requests"], 0);
    EXPECT_EQ(stats["authentication_attempts"], 0);
}

TEST_F(RestApiServerTest, StatisticsAfterOperations) {
    // Add user and authenticate
    m_server->AddUser("user1", "pass1");
    m_server->Authenticate("user1", "pass1");

    // Execute test
    STestExecutionRequest req;
    req.testSequenceId = "TEST-001";
    m_server->ExecuteTest(req);

    auto stats = m_server->GetStatistics();

    EXPECT_EQ(stats["authentication_attempts"], 1);
    EXPECT_EQ(stats["active_executions"], 1);
}

//=============================================================================
// WebSocket Tests
//=============================================================================

TEST_F(RestApiServerTest, RegisterWebSocket) {
    bool handlerCalled = false;

    m_server->RegisterWebSocket("/ws/notifications",
        [&handlerCalled](const SWebSocketMessage& msg) {
            handlerCalled = true;
        });

    // Can't directly test handler without actual WebSocket connection
    // Just verify registration doesn't crash
}

TEST_F(RestApiServerTest, BroadcastWebSocket) {
    m_server->BroadcastWebSocket("{\"event\": \"test\"}");

    auto stats = m_server->GetStatistics();
    EXPECT_GT(stats["websocket_connections"], 0);
}

//=============================================================================
// JSON Helper Tests
//=============================================================================

TEST_F(RestApiServerTest, JsonEscaping) {
    TString input = "Hello \"World\"\nNew Line\tTab\\Backslash";
    TString escaped = JsonHelper::EscapeJson(input);

    EXPECT_NE(escaped.find("\\\""), TString::npos);  // Quotes escaped
    EXPECT_NE(escaped.find("\\n"), TString::npos);   // Newline escaped
    EXPECT_NE(escaped.find("\\t"), TString::npos);   // Tab escaped
    EXPECT_NE(escaped.find("\\\\"), TString::npos);  // Backslash escaped
}

TEST_F(RestApiServerTest, ExecutionInfoToJson) {
    STestExecutionInfo info;
    info.executionId = "exec_123";
    info.testSequenceId = "TEST-001";
    info.status = ETestExecutionStatus::kRunning;
    info.progressPercent = 45.5;
    info.currentStep = "Step 2";

    TString json = JsonHelper::ExecutionInfoToJson(info);

    EXPECT_NE(json.find("exec_123"), TString::npos);
    EXPECT_NE(json.find("TEST-001"), TString::npos);
    EXPECT_NE(json.find("running"), TString::npos);
    EXPECT_NE(json.find("45.5"), TString::npos);
    EXPECT_NE(json.find("Step 2"), TString::npos);
}

TEST_F(RestApiServerTest, ExecutionStatusMapping) {
    STestExecutionInfo info;

    info.status = ETestExecutionStatus::kQueued;
    EXPECT_NE(JsonHelper::ExecutionInfoToJson(info).find("queued"), TString::npos);

    info.status = ETestExecutionStatus::kRunning;
    EXPECT_NE(JsonHelper::ExecutionInfoToJson(info).find("running"), TString::npos);

    info.status = ETestExecutionStatus::kCompleted;
    EXPECT_NE(JsonHelper::ExecutionInfoToJson(info).find("completed"), TString::npos);

    info.status = ETestExecutionStatus::kFailed;
    EXPECT_NE(JsonHelper::ExecutionInfoToJson(info).find("failed"), TString::npos);

    info.status = ETestExecutionStatus::kCancelled;
    EXPECT_NE(JsonHelper::ExecutionInfoToJson(info).find("cancelled"), TString::npos);
}

//=============================================================================
// HTTP Response Helper Tests
//=============================================================================

TEST_F(RestApiServerTest, HttpResponseSetJson) {
    SHttpResponse res;
    res.SetJson("{\"key\": \"value\"}");

    EXPECT_EQ(res.body, "{\"key\": \"value\"}");
    EXPECT_EQ(res.headers["Content-Type"], "application/json");
}

TEST_F(RestApiServerTest, HttpResponseSetText) {
    SHttpResponse res;
    res.SetText("Plain text response");

    EXPECT_EQ(res.body, "Plain text response");
    EXPECT_EQ(res.headers["Content-Type"], "text/plain");
}

TEST_F(RestApiServerTest, HttpResponseSetError) {
    SHttpResponse res;
    res.SetError(EHttpStatus::kNotFound, "Resource not found");

    EXPECT_EQ(res.status, EHttpStatus::kNotFound);
    EXPECT_NE(res.body.find("Resource not found"), TString::npos);
    EXPECT_EQ(res.headers["Content-Type"], "application/json");
}

//=============================================================================
// Configuration Tests
//=============================================================================

TEST_F(RestApiServerTest, CustomConfiguration) {
    SApiServerConfig config;
    config.host = "127.0.0.1";
    config.port = 8888;
    config.threadPoolSize = 8;
    config.enableAuth = true;
    config.enableWebSocket = false;
    config.tokenExpirationMinutes = 120;

    CRestApiServer server(config);

    // Verify server accepts configuration (can't directly access config)
    auto result = server.Start();
    EXPECT_TRUE(result.IsSuccess());
    server.Stop();
}

//=============================================================================
// Concurrency Tests
//=============================================================================

TEST_F(RestApiServerTest, ConcurrentExecutions) {
    TVector<TString> executionIds;

    // Execute multiple tests concurrently (simulated)
    for (int i = 0; i < 10; ++i) {
        STestExecutionRequest req;
        req.testSequenceId = "TEST-" + std::to_string(i);
        executionIds.push_back(m_server->ExecuteTest(req));
    }

    // Verify all executions were created
    EXPECT_EQ(executionIds.size(), 10);

    // Verify all have unique IDs
    std::set<TString> uniqueIds(executionIds.begin(), executionIds.end());
    EXPECT_EQ(uniqueIds.size(), 10);

    // Verify we can get status for all
    for (const auto& id : executionIds) {
        auto status = m_server->GetExecutionStatus(id);
        EXPECT_TRUE(status.has_value());
    }
}

// Note: main() is provided by gtest_main library
