/**************************************************************************
 * File Name: RestApiServer.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-11-23
 * Description: REST API server implementation
 **************************************************************************/

#include "testmate/rest_api/RestApiServer.h"
#include <random>
#include <chrono>
#include <cstdint>
#include <cctype>
#include <optional>

namespace TestMATE {

//=============================================================================
// Helper Functions
//=============================================================================

namespace {
    //-------------------------------------------------------------------------
    // SHA-256 (self-contained; no external crypto dependency)
    //-------------------------------------------------------------------------
    class CSha256 {
    public:
        CSha256() { Reset(); }

        void Reset() {
            m_state[0] = 0x6a09e667u; m_state[1] = 0xbb67ae85u;
            m_state[2] = 0x3c6ef372u; m_state[3] = 0xa54ff53au;
            m_state[4] = 0x510e527fu; m_state[5] = 0x9b05688cu;
            m_state[6] = 0x1f83d9abu; m_state[7] = 0x5be0cd19u;
            m_bitLen = 0;
            m_bufLen = 0;
        }

        void Update(const unsigned char* in_data, size_t in_len) {
            for (size_t i = 0; i < in_len; ++i) {
                m_buffer[m_bufLen++] = in_data[i];
                if (m_bufLen == 64) {
                    Transform(m_buffer);
                    m_bitLen += 512;
                    m_bufLen = 0;
                }
            }
        }

        void Final(unsigned char out_digest[32]) {
            uint64_t totalBits = m_bitLen + static_cast<uint64_t>(m_bufLen) * 8;
            size_t i = m_bufLen;

            m_buffer[i++] = 0x80;
            if (i > 56) {
                while (i < 64) { m_buffer[i++] = 0x00; }
                Transform(m_buffer);
                i = 0;
            }
            while (i < 56) { m_buffer[i++] = 0x00; }
            for (int b = 7; b >= 0; --b) {
                m_buffer[i++] = static_cast<unsigned char>((totalBits >> (b * 8)) & 0xFF);
            }
            Transform(m_buffer);

            for (int s = 0; s < 8; ++s) {
                out_digest[s * 4 + 0] = static_cast<unsigned char>((m_state[s] >> 24) & 0xFF);
                out_digest[s * 4 + 1] = static_cast<unsigned char>((m_state[s] >> 16) & 0xFF);
                out_digest[s * 4 + 2] = static_cast<unsigned char>((m_state[s] >> 8) & 0xFF);
                out_digest[s * 4 + 3] = static_cast<unsigned char>(m_state[s] & 0xFF);
            }
        }

    private:
        static uint32_t Rotr(uint32_t x, uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        void Transform(const unsigned char* in_block) {
            static const uint32_t k[64] = {
                0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
                0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
                0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
                0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
                0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
                0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
                0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
                0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
            };

            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(in_block[i * 4 + 0]) << 24) |
                       (static_cast<uint32_t>(in_block[i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(in_block[i * 4 + 2]) << 8)  |
                       (static_cast<uint32_t>(in_block[i * 4 + 3]));
            }
            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = Rotr(w[i-15], 7) ^ Rotr(w[i-15], 18) ^ (w[i-15] >> 3);
                uint32_t s1 = Rotr(w[i-2], 17) ^ Rotr(w[i-2], 19) ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
            uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];

            for (int i = 0; i < 64; ++i) {
                uint32_t S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + k[i] + w[i];
                uint32_t S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;
                h = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d;
            m_state[4] += e; m_state[5] += f; m_state[6] += g; m_state[7] += h;
        }

        uint32_t      m_state[8];
        uint64_t      m_bitLen;
        unsigned char m_buffer[64];
        size_t        m_bufLen;
    };

    TString ToHex(const unsigned char* in_data, size_t in_len) {
        static const char hex[] = "0123456789abcdef";
        TString out;
        out.reserve(in_len * 2);
        for (size_t i = 0; i < in_len; ++i) {
            out += hex[(in_data[i] >> 4) & 0xF];
            out += hex[in_data[i] & 0xF];
        }
        return out;
    }

    /**
     * @brief Generate cryptographically-random hex from the OS CSPRNG.
     *        std::random_device is backed by /dev/urandom on Linux.
     */
    TString GenerateSecureRandomHex(TUInt32 in_numBytes) {
        std::random_device rd;
        TString result;
        unsigned char bytes;
        result.reserve(in_numBytes * 2);
        for (TUInt32 i = 0; i < in_numBytes; ++i) {
            bytes = static_cast<unsigned char>(rd() & 0xFFu);
            result += ToHex(&bytes, 1);
        }
        return result;
    }

    /**
     * @brief Salted, iterated SHA-256 password stretch. Not bcrypt/Argon2,
     *        but a large work factor over a real cryptographic hash with a
     *        per-user salt — a major improvement over std::hash.
     */
    TString HashPassword(const TString& in_password, const TString& in_salt) {
        constexpr int kIterations = 100000;
        unsigned char digest[32];
        {
            CSha256 sha;
            TString seed = in_salt + ":" + in_password;
            sha.Update(reinterpret_cast<const unsigned char*>(seed.data()), seed.size());
            sha.Final(digest);
        }
        for (int i = 1; i < kIterations; ++i) {
            CSha256 sha;
            sha.Update(reinterpret_cast<const unsigned char*>(in_salt.data()), in_salt.size());
            sha.Update(digest, 32);
            sha.Final(digest);
        }
        return ToHex(digest, 32);
    }

    /**
     * @brief Length-independent comparison to avoid hash/token timing oracles.
     */
    bool ConstantTimeEquals(const TString& in_a, const TString& in_b) {
        if (in_a.size() != in_b.size()) {
            return false;
        }
        unsigned char diff = 0;
        for (size_t i = 0; i < in_a.size(); ++i) {
            diff |= static_cast<unsigned char>(in_a[i] ^ in_b[i]);
        }
        return diff == 0;
    }

    /**
     * @brief Bounds-checked extraction of a JSON string field "key":"value".
     *        Returns nullopt for absent/malformed input instead of crashing
     *        or allocating huge substrings on npos arithmetic.
     */
    std::optional<TString> ExtractJsonString(const TString& in_body, const TString& in_key) {
        const TString needle = "\"" + in_key + "\"";
        size_t keyPos = in_body.find(needle);
        if (keyPos == TString::npos) {
            return std::nullopt;
        }
        size_t colon = in_body.find(':', keyPos + needle.size());
        if (colon == TString::npos) {
            return std::nullopt;
        }
        size_t valOpen = in_body.find('"', colon + 1);
        if (valOpen == TString::npos) {
            return std::nullopt;
        }
        TString value;
        for (size_t i = valOpen + 1; i < in_body.size(); ++i) {
            char c = in_body[i];
            if (c == '\\') {
                if (i + 1 >= in_body.size()) {
                    return std::nullopt;
                }
                char e = in_body[++i];
                switch (e) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case 'b': value += '\b'; break;
                    case 'f': value += '\f'; break;
                    default:  value += e;    break;
                }
            } else if (c == '"') {
                return value;
            } else {
                value += c;
            }
        }
        return std::nullopt;  // Unterminated string.
    }

    /**
     * @brief Restricts ids to a safe charset before they reach downstream
     *        path/query sinks.
     */
    bool IsValidIdentifier(const TString& in_id) {
        if (in_id.empty() || in_id.size() > 128) {
            return false;
        }
        for (char c : in_id) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (!(std::isalnum(uc) || c == '_' || c == '-' || c == '.')) {
                return false;
            }
        }
        return true;
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

    // Per-user random salt; stored as "salt:hash".
    TString salt = GenerateSecureRandomHex(16);
    m_users[in_username] = salt + ":" + HashPassword(in_password, salt);
}

TString CRestApiServer::Authenticate(const TString& in_username, const TString& in_password) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_authenticationAttempts++;

    constexpr TUInt32 kMaxFailedLogins = 5;
    auto now = std::chrono::steady_clock::now();

    // Brute-force lockout: reject while a username is locked out.
    auto throttleIt = m_loginThrottle.find(in_username);
    if (throttleIt != m_loginThrottle.end() &&
        throttleIt->second.failedAttempts >= kMaxFailedLogins &&
        now < throttleIt->second.lockoutUntil) {
        return "";
    }

    auto recordFailure = [&]() {
        auto& t = m_loginThrottle[in_username];
        ++t.failedAttempts;
        if (t.failedAttempts >= kMaxFailedLogins) {
            t.lockoutUntil = std::chrono::steady_clock::now() + std::chrono::minutes(15);
        }
    };

    auto it = m_users.find(in_username);
    if (it == m_users.end()) {
        // Hash anyway so response timing does not reveal whether the user
        // exists (username-enumeration defence).
        (void)HashPassword(in_password, "timing_uniformity_salt");
        recordFailure();
        return "";
    }

    size_t sep = it->second.find(':');
    if (sep == TString::npos) {
        recordFailure();
        return "";
    }
    TString salt = it->second.substr(0, sep);
    TString storedHash = it->second.substr(sep + 1);
    TString computedHash = HashPassword(in_password, salt);

    if (!ConstantTimeEquals(computedHash, storedHash)) {
        recordFailure();
        return "";
    }

    // Success: clear throttle state for this user.
    m_loginThrottle.erase(in_username);

    // Purge expired tokens so the token map cannot grow without bound.
    for (auto tokIt = m_tokens.begin(); tokIt != m_tokens.end(); ) {
        if (now > tokIt->second.expirationTime) {
            tokIt = m_tokens.erase(tokIt);
        } else {
            ++tokIt;
        }
    }

    TString token = GenerateToken(in_username);

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

            // Bounds-checked field extraction; malformed JSON yields empty
            // optionals instead of crashing on npos arithmetic.
            TString username = ExtractJsonString(req.body, "username").value_or("");
            TString password = ExtractJsonString(req.body, "password").value_or("");

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

            // Bounds-checked field extraction.
            STestExecutionRequest execReq;
            execReq.testSequenceId = ExtractJsonString(req.body, "testSequenceId").value_or("");

            if (execReq.testSequenceId.empty()) {
                res.SetError(EHttpStatus::kBadRequest, "Missing testSequenceId");
                m_failedRequests++;
                return res;
            }

            // Restrict the id charset before it reaches downstream sinks.
            if (!IsValidIdentifier(execReq.testSequenceId)) {
                res.SetError(EHttpStatus::kBadRequest, "Invalid testSequenceId");
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
    m_totalRequests++;

    // Reject oversized bodies up front (DoS protection).
    if (in_request.body.size() > m_config.maxRequestSizeBytes) {
        SHttpResponse res;
        res.SetError(EHttpStatus::kBadRequest, "Request body too large");
        m_failedRequests++;
        return res;
    }

    TString method = HttpMethodToString(in_request.method);

    // Resolve the handler under the lock, copy it out, then RELEASE the lock
    // before invoking it. Handlers re-enter the server (Authenticate,
    // ExecuteTest, ...) which lock m_mutex; invoking them while holding it
    // would self-deadlock on the non-recursive mutex.
    FRouteHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto methodIt = m_routes.find(method);
        if (methodIt == m_routes.end()) {
            SHttpResponse res;
            res.SetError(EHttpStatus::kMethodNotAllowed, "Method not allowed");
            m_failedRequests++;
            return res;
        }

        // Exact path match first.
        auto pathIt = methodIt->second.find(in_request.path);
        if (pathIt != methodIt->second.end()) {
            handler = pathIt->second;
        } else {
            // Pattern match for "{id}" routes. The id segment must be the
            // single trailing segment — no extra '/' — to prevent route
            // confusion / path-traversal-style matching.
            for (const auto& [path, routeHandler] : methodIt->second) {
                size_t bracePos = path.find("{id}");
                if (bracePos == TString::npos) {
                    continue;
                }
                TString prefix = path.substr(0, bracePos);
                if (in_request.path.size() > prefix.size() &&
                    in_request.path.compare(0, prefix.size(), prefix) == 0 &&
                    in_request.path.find('/', prefix.size()) == TString::npos) {
                    handler = routeHandler;
                    break;
                }
            }
        }
    }

    if (!handler) {
        SHttpResponse res;
        res.SetError(EHttpStatus::kNotFound, "Endpoint not found");
        m_failedRequests++;
        return res;
    }

    return handler(in_request);
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

TString CRestApiServer::GenerateToken(const TString& /*in_username*/) {
    // Opaque, unpredictable token from the OS CSPRNG. The username is
    // intentionally NOT embedded: doing so leaks identity and aids forgery.
    // The token -> credentials map already records the owning username.
    return "tm_" + GenerateSecureRandomHex(32);
}

TString CRestApiServer::GenerateExecutionId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    return "exec_" + std::to_string(timestamp) + "_" + GenerateSecureRandomHex(8);
}

} // namespace TestMATE
