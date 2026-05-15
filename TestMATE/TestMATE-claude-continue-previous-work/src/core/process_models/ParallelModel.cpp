/**************************************************************************
 * File Name: ParallelModel.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of parallel process model
 * Requirements: REQ-PM-019 to REQ-PM-038
 **************************************************************************/

#include "ParallelModel.h"
#include "utils/LogManager.h"

namespace TestMATE {

CParallelModel::CParallelModel(TUInt32 in_uiSocketCount)
    : m_uiSocketCount(in_uiSocketCount)
{
    if (m_uiSocketCount == 0) {
        m_uiSocketCount = std::thread::hardware_concurrency();
        if (m_uiSocketCount == 0) {
            m_uiSocketCount = 4;  // Fallback
        }
    }

    InitializeSockets();

    CLogManager::GetInstance().LogInfo("ParallelModel",
        "Initialized with {} sockets", m_uiSocketCount);
}

CParallelModel::~CParallelModel() {
    if (GetState() == EProcessModelState::kRunning) {
        Stop();
    }
}

void CParallelModel::InitializeSockets() {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    m_mapSockets.clear();
    m_mapContexts.clear();

    for (TUInt32 i = 0; i < m_uiSocketCount; ++i) {
        TSocketId socketId = static_cast<TSocketId>(i + 1);

        SSocketInfo info;
        info.socketId = socketId;
        info.name = "Socket_" + std::to_string(socketId);
        info.enabled = true;

        m_mapSockets[socketId] = info;
        m_mapContexts.try_emplace(socketId, socketId);
    }
}

CResult CParallelModel::SetSocketCount(TUInt32 in_uiCount) {
    if (GetState() == EProcessModelState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Cannot change socket count while running");
    }

    if (in_uiCount == 0) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidParameter,
                                "Socket count must be > 0");
    }

    m_uiSocketCount = in_uiCount;
    InitializeSockets();

    return TESTMATE_SUCCESS();
}

std::optional<SSocketInfo> CParallelModel::GetSocketInfo(TSocketId in_socketId) const {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    auto it = m_mapSockets.find(in_socketId);
    if (it != m_mapSockets.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CParallelModel::EnableSocket(TSocketId in_socketId, bool in_bEnable) {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    auto it = m_mapSockets.find(in_socketId);
    if (it != m_mapSockets.end()) {
        it->second.enabled = in_bEnable;
    }
}

TUInt32 CParallelModel::GetEnabledSocketCount() const {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    TUInt32 count = 0;
    for (const auto& [id, info] : m_mapSockets) {
        if (info.enabled) {
            ++count;
        }
    }
    return count;
}

CResult CParallelModel::Start() {
    if (GetState() == EProcessModelState::kRunning) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Model already running");
    }

    TUInt32 enabledCount = GetEnabledSocketCount();
    if (enabledCount == 0) {
        return TESTMATE_FAILURE(EErrorCode::kConfigValidationFailed,
                                "No sockets enabled");
    }

    // Reset state
    m_uiSyncCount = 0;
    m_uiCompletedSockets = 0;

    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        for (auto& [id, info] : m_mapSockets) {
            info.state = EExecutionState::kIdle;
            info.currentStepIndex = 0;
            info.completedSteps = 0;
            info.passCount = 0;
            info.failCount = 0;
        }
        for (auto& [id, ctx] : m_mapContexts) {
            ctx.ClearAllVariables();
            ctx.ClearAbortRequest();
        }
    }

    SetState(EProcessModelState::kRunning);

    // Create thread pool sized for enabled sockets
    m_pThreadPool = std::make_unique<CThreadPool>(enabledCount);
    m_pResourceScheduler = std::make_unique<CResourceScheduler>();

    // Submit execution task for each enabled socket
    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        for (const auto& [id, info] : m_mapSockets) {
            if (info.enabled) {
                m_pThreadPool->Submit([this, socketId = id]() {
                    ExecutionThreadFunc(socketId);
                });
            }
        }
    }

    CLogManager::GetInstance().LogInfo("ParallelModel",
        "Started execution with {} sockets", enabledCount);

    return TESTMATE_SUCCESS();
}

CResult CParallelModel::Stop() {
    if (GetState() != EProcessModelState::kRunning &&
        GetState() != EProcessModelState::kPaused) {
        return TESTMATE_FAILURE(EErrorCode::kInvalidState,
                                "Model not running");
    }

    // Request abort on all contexts
    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        for (auto& [id, ctx] : m_mapContexts) {
            ctx.RequestAbort();
        }
    }

    // Wake up any waiting threads
    m_syncCV.notify_all();

    // Shutdown thread pool
    if (m_pThreadPool) {
        m_pThreadPool->Shutdown(true);  // Force shutdown
        m_pThreadPool.reset();
    }

    SetState(EProcessModelState::kIdle);

    CLogManager::GetInstance().LogInfo("ParallelModel", "Stopped");

    return TESTMATE_SUCCESS();
}

void CParallelModel::ExecutionThreadFunc(TSocketId in_socketId) {
    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        m_mapSockets[in_socketId].state = EExecutionState::kRunning;
    }

    auto& context = m_mapContexts[in_socketId];

    // Simulate test step execution
    const TUInt32 kNumSteps = 10;  // Placeholder - would come from test sequence

    for (TUInt32 step = 0; step < kNumSteps; ++step) {
        if (context.IsAbortRequested()) {
            break;
        }

        // Handle pause
        while (GetState() == EProcessModelState::kPaused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (context.IsAbortRequested()) {
                break;
            }
        }

        if (context.IsAbortRequested()) {
            break;
        }

        // Update current step
        {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            m_mapSockets[in_socketId].currentStepIndex = step;
        }

        // Execute step (placeholder - actual step execution would go here)

        {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            m_mapSockets[in_socketId].completedSteps++;
            m_mapSockets[in_socketId].passCount++;
        }

        // Synchronize at step boundary if enabled
        if (m_bSyncAtSteps) {
            SyncBarrier();
        }
    }

    // Mark socket as complete
    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        m_mapSockets[in_socketId].state = EExecutionState::kCompleted;
    }

    ++m_uiCompletedSockets;

    // Check if all sockets complete
    if (m_uiCompletedSockets.load() >= GetEnabledSocketCount()) {
        SetState(EProcessModelState::kCompleted);
    }
}

void CParallelModel::SyncBarrier() {
    TUInt32 enabledCount = GetEnabledSocketCount();

    std::unique_lock<std::mutex> lock(m_syncMutex);

    ++m_uiSyncCount;

    if (m_uiSyncCount.load() >= enabledCount) {
        // Last thread to arrive - release all
        m_uiSyncCount = 0;
        m_syncCV.notify_all();
    } else {
        // Wait for others
        m_syncCV.wait(lock, [this, enabledCount]() {
            return m_uiSyncCount.load() == 0 ||
                   m_mapContexts.begin()->second.IsAbortRequested();
        });
    }
}

bool CParallelModel::WaitForCompletion(TInt64 in_timeoutMs) {
    if (!m_pThreadPool) {
        return true;
    }

    if (in_timeoutMs <= 0) {
        m_pThreadPool->WaitForAll();
        return true;
    }

    auto start = std::chrono::steady_clock::now();
    while (GetState() == EProcessModelState::kRunning) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (elapsed >= in_timeoutMs) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;
}

TUInt32 CParallelModel::GetTotalPassCount() const {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    TUInt32 total = 0;
    for (const auto& [id, info] : m_mapSockets) {
        total += info.passCount;
    }
    return total;
}

TUInt32 CParallelModel::GetTotalFailCount() const {
    std::lock_guard<std::mutex> lock(m_socketMutex);

    TUInt32 total = 0;
    for (const auto& [id, info] : m_mapSockets) {
        total += info.failCount;
    }
    return total;
}

} // namespace TestMATE
