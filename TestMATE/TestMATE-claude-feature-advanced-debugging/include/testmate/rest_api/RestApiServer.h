/**
 * @file RestApiServer.h
 * @brief REST API server for remote test execution and monitoring
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the REST API server that provides HTTP endpoints
 * for test execution, status monitoring, and result retrieval.
 */

#ifndef TESTMATE_API_REST_API_SERVER_H
#define TESTMATE_API_REST_API_SERVER_H

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <functional>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

namespace TestMATE {

/**
 * @brief HTTP methods
 */
enum class EHttpMethod {
    kGet,
    kPost,
    kPut,
    kDelete,
    kPatch
};

/**
 * @brief HTTP status codes
 */
enum class EHttpStatus {
    kOk = 200,
    kCreated = 201,
    kAccepted = 202,
    kNoContent = 204,
    kBadRequest = 400,
    kUnauthorized = 401,
    kForbidden = 403,
    kNotFound = 404,
    kMethodNotAllowed = 405,
    kConflict = 409,
    kInternalServerError = 500,
    kNotImplemented = 501,
    kServiceUnavailable = 503
};

/**
 * @brief HTTP request
 */
struct SHttpRequest {
    EHttpMethod method{EHttpMethod::kGet};
    TString path;
    TMap<TString, TString> headers;
    TMap<TString, TString> queryParams;
    TString body;
    TString clientIp;
};

/**
 * @brief JSON string escaping (declared early so SHttpResponse can use it)
 */
namespace JsonHelper {
    /**
     * @brief Escape a string for safe embedding inside a JSON string literal.
     *        Escapes quotes, backslashes and all control characters (< 0x20).
     */
    inline TString EscapeJson(const TString& in_str) {
        static const char hexDigits[] = "0123456789abcdef";
        TString result;
        result.reserve(in_str.size());

        for (char c : in_str) {
            unsigned char uc = static_cast<unsigned char>(c);
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b";  break;
                case '\f': result += "\\f";  break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:
                    if (uc < 0x20) {
                        result += "\\u00";
                        result += hexDigits[(uc >> 4) & 0xF];
                        result += hexDigits[uc & 0xF];
                    } else {
                        result += c;
                    }
                    break;
            }
        }

        return result;
    }
}

/**
 * @brief HTTP response
 */
struct SHttpResponse {
    EHttpStatus status{EHttpStatus::kOk};
    TMap<TString, TString> headers;
    TString body;

    // Helper methods
    void SetJson(const TString& in_json) {
        body = in_json;
        headers["Content-Type"] = "application/json";
    }

    void SetText(const TString& in_text) {
        body = in_text;
        headers["Content-Type"] = "text/plain";
    }

    void SetError(EHttpStatus in_status, const TString& in_message) {
        status = in_status;
        // Escape the message: it often contains attacker-influenced data
        // (ids, parsed fields) that would otherwise break out of the JSON.
        SetJson("{\"error\": \"" + JsonHelper::EscapeJson(in_message) + "\"}");
    }
};

/**
 * @brief Route handler function
 */
using FRouteHandler = std::function<SHttpResponse(const SHttpRequest&)>;

/**
 * @brief WebSocket message
 */
struct SWebSocketMessage {
    TString clientId;
    TString message;
    TTimePoint timestamp;
};

/**
 * @brief WebSocket event handler
 */
using FWebSocketHandler = std::function<void(const SWebSocketMessage&)>;

/**
 * @brief Authentication credentials
 */
struct SAuthCredentials {
    TString username;
    TString password;
    TString token;
    TTimePoint expirationTime;
};

/**
 * @brief API server configuration
 */
struct SApiServerConfig {
    TString host{"127.0.0.1"};  // Secure default: loopback only. Set to
                                // "0.0.0.0" explicitly to expose externally.
    TUInt16 port{8080};
    TUInt32 threadPoolSize{4};
    bool enableAuth{true};
    bool enableWebSocket{true};
    bool enableCors{true};
    TString corsOrigin{"*"};
    TUInt32 tokenExpirationMinutes{60};
    TUInt32 maxRequestSizeBytes{10 * 1024 * 1024};  // 10MB
};

/**
 * @brief Test execution request
 */
struct STestExecutionRequest {
    TString testSequenceId;
    TMap<TString, TString> parameters;
    bool async{true};
};

/**
 * @brief Test execution status
 */
enum class ETestExecutionStatus {
    kQueued,
    kRunning,
    kCompleted,
    kFailed,
    kCancelled
};

/**
 * @brief Test execution info
 */
struct STestExecutionInfo {
    TString executionId;
    TString testSequenceId;
    ETestExecutionStatus status{ETestExecutionStatus::kQueued};
    TTimePoint startTime;
    TTimePoint endTime;
    TDouble progressPercent{0.0};
    TString currentStep;
    TMap<TString, TString> results;
};

/**
 * @brief REST API Server
 *
 * Provides HTTP REST API for test execution and monitoring.
 * Supports authentication, WebSocket connections, and CORS.
 *
 * Example usage:
 * @code
 * SApiServerConfig config;
 * config.port = 8080;
 * config.enableAuth = true;
 *
 * CRestApiServer server(config);
 *
 * // Register routes
 * server.RegisterRoute(EHttpMethod::kGet, "/api/health",
 *     [](const SHttpRequest& req) {
 *         SHttpResponse res;
 *         res.SetJson("{\"status\": \"healthy\"}");
 *         return res;
 *     });
 *
 * // Start server
 * server.Start();
 *
 * // ... server running ...
 *
 * server.Stop();
 * @endcode
 */
class CRestApiServer {
public:
    /**
     * @brief Construct API server with configuration
     * @param in_config Server configuration
     */
    explicit CRestApiServer(const SApiServerConfig& in_config = {});

    /**
     * @brief Destructor
     */
    ~CRestApiServer();

    // Non-copyable, non-movable
    CRestApiServer(const CRestApiServer&) = delete;
    CRestApiServer& operator=(const CRestApiServer&) = delete;
    CRestApiServer(CRestApiServer&&) = delete;
    CRestApiServer& operator=(CRestApiServer&&) = delete;

    /**
     * @brief Start the API server
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult Start();

    /**
     * @brief Stop the API server
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult Stop();

    /**
     * @brief Check if server is running
     * @return true if server is running
     */
    [[nodiscard]] bool IsRunning() const { return m_isRunning; }

    /**
     * @brief Register route handler
     * @param in_method HTTP method
     * @param in_path URL path
     * @param in_handler Handler function
     */
    void RegisterRoute(EHttpMethod in_method, const TString& in_path, FRouteHandler in_handler);

    /**
     * @brief Register WebSocket handler
     * @param in_path WebSocket path
     * @param in_handler Handler function
     */
    void RegisterWebSocket(const TString& in_path, FWebSocketHandler in_handler);

    /**
     * @brief Add user for authentication
     * @param in_username Username
     * @param in_password Password
     */
    void AddUser(const TString& in_username, const TString& in_password);

    /**
     * @brief Authenticate user and generate token
     * @param in_username Username
     * @param in_password Password
     * @return Authentication token or empty string on failure
     */
    [[nodiscard]] TString Authenticate(const TString& in_username, const TString& in_password);

    /**
     * @brief Validate authentication token
     * @param in_token Authentication token
     * @return true if token is valid
     */
    [[nodiscard]] bool ValidateToken(const TString& in_token) const;

    /**
     * @brief Send WebSocket message to all clients
     * @param in_message Message to send
     */
    void BroadcastWebSocket(const TString& in_message);

    /**
     * @brief Get server statistics
     * @return Map of statistics
     */
    [[nodiscard]] TMap<TString, TUInt64> GetStatistics() const;

    /**
     * @brief Execute test sequence
     * @param in_request Test execution request
     * @return Execution ID
     */
    [[nodiscard]] TString ExecuteTest(const STestExecutionRequest& in_request);

    /**
     * @brief Get test execution status
     * @param in_executionId Execution ID
     * @return Execution info or nullopt if not found
     */
    [[nodiscard]] std::optional<STestExecutionInfo> GetExecutionStatus(const TString& in_executionId) const;

    /**
     * @brief Cancel test execution
     * @param in_executionId Execution ID
     * @return Result indicating success or failure
     */
    [[nodiscard]] CResult CancelExecution(const TString& in_executionId);

    /**
     * @brief Get all test executions
     * @return Vector of execution info
     */
    [[nodiscard]] TVector<STestExecutionInfo> GetAllExecutions() const;

private:
    /**
     * @brief Initialize default routes
     */
    void InitializeDefaultRoutes();

    /**
     * @brief Handle HTTP request
     * @param in_request HTTP request
     * @return HTTP response
     */
    SHttpResponse HandleRequest(const SHttpRequest& in_request);

    /**
     * @brief Check if request is authenticated
     * @param in_request HTTP request
     * @return true if authenticated
     */
    bool IsAuthenticated(const SHttpRequest& in_request) const;

    /**
     * @brief Generate authentication token
     * @param in_username Username
     * @return Generated token
     */
    TString GenerateToken(const TString& in_username);

    /**
     * @brief Generate unique execution ID
     * @return Execution ID
     */
    TString GenerateExecutionId();

    SApiServerConfig m_config;
    std::atomic<bool> m_isRunning{false};

    mutable std::mutex m_mutex;
    TMap<TString, TMap<TString, FRouteHandler>> m_routes;  // [method][path] -> handler
    TMap<TString, FWebSocketHandler> m_webSocketHandlers;
    TMap<TString, TString> m_users;  // username -> "salt:hash"
    TMap<TString, SAuthCredentials> m_tokens;  // token -> credentials
    TMap<TString, STestExecutionInfo> m_executions;  // executionId -> info

    // Brute-force protection: per-username failed-attempt count and the
    // time until which further login attempts are rejected.
    struct SLoginThrottle {
        TUInt32 failedAttempts{0};
        TTimePoint lockoutUntil{};
    };
    TMap<TString, SLoginThrottle> m_loginThrottle;

    // Statistics
    std::atomic<TUInt64> m_totalRequests{0};
    std::atomic<TUInt64> m_successfulRequests{0};
    std::atomic<TUInt64> m_failedRequests{0};
    std::atomic<TUInt64> m_authenticationAttempts{0};
    std::atomic<TUInt64> m_webSocketConnections{0};
};

/**
 * @brief JSON helper functions (EscapeJson is defined earlier in this header)
 */
namespace JsonHelper {
    /**
     * @brief Convert execution status to JSON
     * @param in_info Execution info
     * @return JSON string
     */
    inline TString ExecutionInfoToJson(const STestExecutionInfo& in_info) {
        TString json = "{"
            "\"executionId\": \"" + EscapeJson(in_info.executionId) + "\","
            "\"testSequenceId\": \"" + EscapeJson(in_info.testSequenceId) + "\","
            "\"status\": \"";

        switch (in_info.status) {
            case ETestExecutionStatus::kQueued:    json += "queued";    break;
            case ETestExecutionStatus::kRunning:   json += "running";   break;
            case ETestExecutionStatus::kCompleted: json += "completed"; break;
            case ETestExecutionStatus::kFailed:    json += "failed";    break;
            case ETestExecutionStatus::kCancelled: json += "cancelled"; break;
        }

        json += "\","
            "\"progressPercent\": " + std::to_string(in_info.progressPercent) + ","
            "\"currentStep\": \"" + EscapeJson(in_info.currentStep) + "\""
            "}";

        return json;
    }
}

} // namespace TestMATE

#endif // TESTMATE_API_REST_API_SERVER_H
