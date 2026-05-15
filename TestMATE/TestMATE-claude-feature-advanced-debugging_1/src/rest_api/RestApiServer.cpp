/**************************************************************************
 * File Name: RestApiServer.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: REST API server implementation
 **************************************************************************/

#include "testmate/rest_api/RestApiServer.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace TestMATE {

//=============================================================================
// Helper Functions
//=============================================================================

namespace {
    /**
     * @brief Generate random hex string
     */
    TString GenerateRandomHex(TUInt32 in_length) {
        static const char hex[] = "0123456789abcdef";
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        TString result;
        result.reserve(in_length);

        for (TUInt32 i = 0; i < in_length; ++i) {
            result += hex[dis(gen)];
        }

        return result;
    }

    /**
     * @brief Simple password hashing (SHA-256 would be used in production)
     */
    TString HashPassword(const TString& in_password) {
        // Simple hash for demonstration - use proper crypto in production
        std::hash<TString> hasher;
        std::ostringstream oss;
        oss << std::hex << hasher(in_password + "salt_12345");
        return oss.str();
    }

    /**
     * @brief Get HTTP method string
     */
    TString HttpMethodToString(EHttpMethod in_method) {
        switch (in_method) {
            case EHttpMethod::kGet:    return "GET";
            case EHttpMethod::kPost:   return "POST";
            case EHttpMethod::kPut:    return "PUT";
            case EHttpMethod::kDelete: return "DELETE";
            case EHttpMethod::kPatch:  return "PATCH";
            default:                   return "UNKNOWN";
        }
    }
}

//=============================================================================
// CRestApiServer Implementation
//=============================================================================

CRestApiServer::CRestApiServer(const SApiServerConfig& in_config)
    : m_config(in_config)
{
    InitializeDefaultRoutes();
}

CRestApiServer::~CRestApiServer() {
    if (m_isRunning) {
        [[maybe_unused]] auto result = Stop();
    }
}

CResult CRestApiServer::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Server is already running");
    }

    m_isRunning = true;

    // In a real implementation, this would start the HTTP server
    // For testing/simulation, we just mark as running

    return TESTMATE_SUCCESS();
}

CResult CRestApiServer::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Server is not running");
    }

    m_isRunning = false;

    // In a real implementation, this would stop the HTTP server

    return TESTMATE_SUCCESS();
}

void CRestApiServer::RegisterRoute(
    EHttpMethod in_method,
    const TString& in_path,
    FRouteHandler in_handler) {

    std::lock_guard<std::mutex> lock(m_mutex);

    TString methodStr = HttpMethodToString(in_method);
    m_routes[methodStr][in_path] = std::move(in_handler);
}

void CRestApiServer::RegisterWebSocket(
    const TString& in_path,
    FWebSocketHandler in_handler) {

    std::lock_guard<std::mutex> lock(m_mutex);

    m_webSocketHandlers[in_path] = std::move(in_handler);
}

void CRestApiServer::AddUser(const TString& in_username, const TString& in_password) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_users[in_username] = HashPassword(in_password);
}

TString CRestApiServer::Authenticate(const TString& in_username, const TString& in_password) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_authenticationAttempts++;

    auto it = m_users.find(in_username);
    if (it == m_users.end()) {
        return "";  // User not found
    }

    TString hashedPassword = HashPassword(in_password);
    if (it->second != hashedPassword) {
        return "";  // Wrong password
    }

    // Generate token
    TString token = GenerateToken(in_username);

    // Store token with expiration
    SAuthCredentials creds;
    creds.username = in_username;
    creds.token = token;
    creds.expirationTime = std::chrono::steady_clock::now() +
        std::chrono::minutes(m_config.tokenExpirationMinutes);

    m_tokens[token] = creds;

    return token;
}

bool CRestApiServer::ValidateToken(const TString& in_token) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_tokens.find(in_token);
    if (it == m_tokens.end()) {
        return false;
    }

    // Check if token expired
    auto now = std::chrono::steady_clock::now();
    if (now > it->second.expirationTime) {
        return false;
    }

    return true;
}

void CRestApiServer::BroadcastWebSocket(const TString& in_message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // In a real implementation, this would send to all connected WebSocket clients
    // For now, we just track that a broadcast was requested

    m_webSocketConnections++;
}

TMap<TString, TUInt64> CRestApiServer::GetStatistics() const {
    TMap<TString, TUInt64> stats;

    stats["total_requests"] = m_totalRequests.load();
    stats["successful_requests"] = m_successfulRequests.load();
    stats["failed_requests"] = m_failedRequests.load();
    stats["authentication_attempts"] = m_authenticationAttempts.load();
    stats["websocket_connections"] = m_webSocketConnections.load();

    std::lock_guard<std::mutex> lock(m_mutex);
    stats["active_executions"] = static_cast<TUInt64>(m_executions.size());
    stats["registered_routes"] = 0;
    for (const auto& [method, paths] : m_routes) {
        stats["registered_routes"] += paths.size();
    }

    return stats;
}

TString CRestApiServer::ExecuteTest(const STestExecutionRequest& in_request) {
    std::lock_guard<std::mutex> lock(m_mutex);

    TString executionId = GenerateExecutionId();

    STestExecutionInfo info;
    info.executionId = executionId;
    info.testSequenceId = in_request.testSequenceId;
    info.status = in_request.async ? ETestExecutionStatus::kQueued : ETestExecutionStatus::kRunning;
    info.startTime = std::chrono::steady_clock::now();
    info.progressPercent = 0.0;
    info.currentStep = "Initializing";

    m_executions[executionId] = info;

    return executionId;
}

std::optional<STestExecutionInfo> CRestApiServer::GetExecutionStatus(
    const TString& in_executionId) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_executions.find(in_executionId);
    if (it == m_executions.end()) {
        return std::nullopt;
    }

    return it->second;
}

CResult CRestApiServer::CancelExecution(const TString& in_executionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_executions.find(in_executionId);
    if (it == m_executions.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound, "Execution not found");
    }

    if (it->second.status == ETestExecutionStatus::kCompleted ||
        it->second.status == ETestExecutionStatus::kFailed ||
        it->second.status == ETestExecutionStatus::kCancelled) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState, "Execution already finished");
    }

    it->second.status = ETestExecutionStatus::kCancelled;
    it->second.endTime = std::chrono::steady_clock::now();

    return TESTMATE_SUCCESS();
}

TVector<STestExecutionInfo> CRestApiServer::GetAllExecutions() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<STestExecutionInfo> executions;
    executions.reserve(m_executions.size());

    for (const auto& [id, info] : m_executions) {
        executions.push_back(info);
    }

    return executions;
}

void CRestApiServer::InitializeDefaultRoutes() {
    // Health check endpoint
    RegisterRoute(EHttpMethod::kGet, "/api/health",
        [this](const SHttpRequest& req) {
            SHttpResponse res;
            res.SetJson("{\"status\": \"healthy\", \"version\": \"2.0.0\"}");
            m_successfulRequests++;
            return res;
        });

    // Authentication endpoint
    RegisterRoute(EHttpMethod::kPost, "/api/auth/login",
        [this](const SHttpRequest& req) {
            SHttpResponse res;

            // Parse username and password from body (simplified JSON parsing)
            // In production, use a proper JSON library

            TString username, password;
            // Simple extraction (not robust, for demonstration)
            size_t userPos = req.body.find("\"username\"");
            size_t passPos = req.body.find("\"password\"");

            if (userPos != TString::npos && passPos != TString::npos) {
                size_t userStart = req.body.find(":", userPos) + 2;
                size_t userEnd = req.body.find("\"", userStart + 1);
                username = req.body.substr(userStart + 1, userEnd - userStart - 1);

                size_t passStart = req.body.find(":", passPos) + 2;
                size_t passEnd = req.body.find("\"", passStart + 1);
                password = req.body.substr(passStart + 1, passEnd - passStart - 1);
            }

            if (username.empty() || password.empty()) {
                res.SetError(EHttpStatus::kBadRequest, "Missing username or password");
                m_failedRequests++;
                return res;
            }

            TString token = Authenticate(username, password);

            if (token.empty()) {
                res.SetError(EHttpStatus::kUnauthorized, "Invalid credentials");
                m_failedRequests++;
                return res;
            }

            res.SetJson("{\"token\": \"" + token + "\"}");
            m_successfulRequests++;
            return res;
        });

    // Execute test endpoint
    RegisterRoute(EHttpMethod::kPost, "/api/tests/execute",
        [this](const SHttpRequest& req) {
            SHttpResponse res;

            if (m_config.enableAuth && !IsAuthenticated(req)) {
                res.SetError(EHttpStatus::kUnauthorized, "Authentication required");
                m_failedRequests++;
                return res;
            }

            // Parse request (simplified)
            STestExecutionRequest execReq;
            size_t seqPos = req.body.find("\"testSequenceId\"");
            if (seqPos != TString::npos) {
                size_t start = req.body.find(":", seqPos) + 2;
                size_t end = req.body.find("\"", start + 1);
                execReq.testSequenceId = req.body.substr(start + 1, end - start - 1);
            }

            if (execReq.testSequenceId.empty()) {
                res.SetError(EHttpStatus::kBadRequest, "Missing testSequenceId");
                m_failedRequests++;
                return res;
            }

            TString executionId = ExecuteTest(execReq);

            res.status = EHttpStatus::kAccepted;
            res.SetJson("{\"executionId\": \"" + executionId + "\"}");
            m_successfulRequests++;
            return res;
        });

    // Get execution status endpoint
    RegisterRoute(EHttpMethod::kGet, "/api/tests/executions/{id}",
        [this](const SHttpRequest& req) {
            SHttpResponse res;

            if (m_config.enableAuth && !IsAuthenticated(req)) {
                res.SetError(EHttpStatus::kUnauthorized, "Authentication required");
                m_failedRequests++;
                return res;
            }

            // Extract execution ID from path
            TString executionId;
            size_t lastSlash = req.path.find_last_of('/');
            if (lastSlash != TString::npos) {
                executionId = req.path.substr(lastSlash + 1);
            }

            auto info = GetExecutionStatus(executionId);

            if (!info) {
                res.SetError(EHttpStatus::kNotFound, "Execution not found");
                m_failedRequests++;
                return res;
            }

            res.SetJson(JsonHelper::ExecutionInfoToJson(*info));
            m_successfulRequests++;
            return res;
        });

    // List all executions endpoint
    RegisterRoute(EHttpMethod::kGet, "/api/tests/executions",
        [this](const SHttpRequest& req) {
            SHttpResponse res;

            if (m_config.enableAuth && !IsAuthenticated(req)) {
                res.SetError(EHttpStatus::kUnauthorized, "Authentication required");
                m_failedRequests++;
                return res;
            }

            auto executions = GetAllExecutions();

            TString json = "[";
            for (size_t i = 0; i < executions.size(); ++i) {
                if (i > 0) json += ",";
                json += JsonHelper::ExecutionInfoToJson(executions[i]);
            }
            json += "]";

            res.SetJson(json);
            m_successfulRequests++;
            return res;
        });

    // Cancel execution endpoint
    RegisterRoute(EHttpMethod::kDelete, "/api/tests/executions/{id}",
        [this](const SHttpRequest& req) {
            SHttpResponse res;

            if (m_config.enableAuth && !IsAuthenticated(req)) {
                res.SetError(EHttpStatus::kUnauthorized, "Authentication required");
                m_failedRequests++;
                return res;
            }

            // Extract execution ID from path
            TString executionId;
            size_t lastSlash = req.path.find_last_of('/');
            if (lastSlash != TString::npos) {
                executionId = req.path.substr(lastSlash + 1);
            }

            auto result = CancelExecution(executionId);

            if (!result.IsSuccess()) {
                if (result.GetCode() == EErrorCode::kNotFound) {
                    res.SetError(EHttpStatus::kNotFound, result.GetMessage());
                } else {
                    res.SetError(EHttpStatus::kBadRequest, result.GetMessage());
                }
                m_failedRequests++;
                return res;
            }

            res.status = EHttpStatus::kNoContent;
            m_successfulRequests++;
            return res;
        });
}

SHttpResponse CRestApiServer::HandleRequest(const SHttpRequest& in_request) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_totalRequests++;

    TString method = HttpMethodToString(in_request.method);

    auto methodIt = m_routes.find(method);
    if (methodIt == m_routes.end()) {
        SHttpResponse res;
        res.SetError(EHttpStatus::kMethodNotAllowed, "Method not allowed");
        m_failedRequests++;
        return res;
    }

    // Try exact path match first
    auto pathIt = methodIt->second.find(in_request.path);
    if (pathIt != methodIt->second.end()) {
        return pathIt->second(in_request);
    }

    // Try pattern matching for paths with {id}
    for (const auto& [path, handler] : methodIt->second) {
        if (path.find("{id}") != TString::npos) {
            TString pattern = path.substr(0, path.find("{id}"));
            if (in_request.path.find(pattern) == 0) {
                return handler(in_request);
            }
        }
    }

    SHttpResponse res;
    res.SetError(EHttpStatus::kNotFound, "Endpoint not found");
    m_failedRequests++;
    return res;
}

bool CRestApiServer::IsAuthenticated(const SHttpRequest& in_request) const {
    auto it = in_request.headers.find("Authorization");
    if (it == in_request.headers.end()) {
        return false;
    }

    // Extract token from "Bearer <token>"
    TString authHeader = it->second;
    if (authHeader.find("Bearer ") != 0) {
        return false;
    }

    TString token = authHeader.substr(7);  // Skip "Bearer "
    return ValidateToken(token);
}

TString CRestApiServer::GenerateToken(const TString& in_username) {
    return "token_" + in_username + "_" + GenerateRandomHex(32);
}

TString CRestApiServer::GenerateExecutionId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    return "exec_" + std::to_string(timestamp) + "_" + GenerateRandomHex(8);
}

} // namespace TestMATE
