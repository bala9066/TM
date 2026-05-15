/**************************************************************************
 * File Name: ParallelModel.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Parallel process model for multi-socket test execution.
 *              Executes tests across multiple sockets simultaneously.
 * Requirements: REQ-PM-019 to REQ-PM-038
 **************************************************************************/

#pragma once

#include "ProcessModelBase.h"
#include "core/threading/ThreadPool.h"
#include "core/scheduling/ResourceScheduler.h"
#include <map>

namespace TestMATE {

/**************************************************************************
 * Struct: SSocketInfo
 * Description: Information about a test socket
 **************************************************************************/
struct SSocketInfo {
    TSocketId socketId{kInvalidSocketId};
    TString name;
    EExecutionState state{EExecutionState::kIdle};
    TUInt32 currentStepIndex{0};
    TUInt32 completedSteps{0};
    TUInt32 passCount{0};
    TUInt32 failCount{0};
    bool enabled{true};
};

/**************************************************************************
 * Class: CParallelModel
 * Description: Parallel execution model. Executes the same test sequence
 *              across multiple sockets simultaneously.
 * Requirements: REQ-PM-019 to REQ-PM-038
 **************************************************************************/
class CParallelModel : public CProcessModelBase {
public:
    /**************************************************************************
     * Function Name: CParallelModel
     * Description: Constructor
     * Parameters:
     *   in_uiSocketCount - Number of parallel sockets (default: hardware threads)
     **************************************************************************/
    explicit CParallelModel(TUInt32 in_uiSocketCount = 0);
    ~CParallelModel() override;

    //=========================================================================
    // IProcessModel Implementation
    //=========================================================================

    [[nodiscard]] EProcessModelType GetType() const override {
        return EProcessModelType::kParallel;
    }

    [[nodiscard]] TString GetName() const override {
        return "Parallel";
    }

    [[nodiscard]] TString GetDescription() const override {
        return "Multi-socket parallel test execution model";
    }

    CResult Start() override;
    CResult Stop();

    //=========================================================================
    // Socket Management
    //=========================================================================

    /**************************************************************************
     * Function Name: GetSocketCount
     * Description: Returns number of configured sockets
     * Returns: Socket count
     **************************************************************************/
    [[nodiscard]] TUInt32 GetSocketCount() const { return m_uiSocketCount; }

    /**************************************************************************
     * Function Name: SetSocketCount
     * Description: Sets number of parallel sockets
     * Parameters:
     *   in_uiCount - New socket count
     * Returns: Result indicating success or failure
     **************************************************************************/
    CResult SetSocketCount(TUInt32 in_uiCount);

    /**************************************************************************
     * Function Name: GetSocketInfo
     * Description: Gets information about a specific socket
     * Parameters:
     *   in_socketId - Socket identifier
     * Returns: Socket information or nullopt if not found
     **************************************************************************/
    [[nodiscard]] std::optional<SSocketInfo> GetSocketInfo(TSocketId in_socketId) const;

    /**************************************************************************
     * Function Name: EnableSocket
     * Description: Enables or disables a socket
     * Parameters:
     *   in_socketId - Socket to enable/disable
     *   in_bEnable - true to enable, false to disable
     **************************************************************************/
    void EnableSocket(TSocketId in_socketId, bool in_bEnable);

    /**************************************************************************
     * Function Name: GetEnabledSocketCount
     * Description: Returns number of enabled sockets
     **************************************************************************/
    [[nodiscard]] TUInt32 GetEnabledSocketCount() const;

    //=========================================================================
    // Synchronization
    //=========================================================================

    /**************************************************************************
     * Function Name: SetSynchronizationMode
     * Description: Sets how sockets synchronize
     * Parameters:
     *   in_bSyncAtSteps - If true, all sockets sync at step boundaries
     **************************************************************************/
    void SetSynchronizationMode(bool in_bSyncAtSteps) { m_bSyncAtSteps = in_bSyncAtSteps; }

    [[nodiscard]] bool GetSynchronizationMode() const { return m_bSyncAtSteps; }

    /**************************************************************************
     * Function Name: WaitForCompletion
     * Description: Blocks until all sockets complete
     * Parameters:
     *   in_timeoutMs - Timeout in milliseconds (0 = infinite)
     * Returns: true if completed, false if timeout
     **************************************************************************/
    bool WaitForCompletion(TInt64 in_timeoutMs = 0);

    //=========================================================================
    // Statistics
    //=========================================================================

    /**************************************************************************
     * Function Name: GetTotalPassCount
     * Description: Returns total pass count across all sockets
     **************************************************************************/
    [[nodiscard]] TUInt32 GetTotalPassCount() const;

    /**************************************************************************
     * Function Name: GetTotalFailCount
     * Description: Returns total fail count across all sockets
     **************************************************************************/
    [[nodiscard]] TUInt32 GetTotalFailCount() const;

private:
    void InitializeSockets();
    void ExecutionThreadFunc(TSocketId in_socketId);
    void SyncBarrier();

    TUInt32 m_uiSocketCount;
    bool m_bSyncAtSteps{false};

    mutable std::mutex m_socketMutex;
    std::map<TSocketId, SSocketInfo> m_mapSockets;
    std::map<TSocketId, CExecutionContext> m_mapContexts;

    TUniquePtr<CThreadPool> m_pThreadPool;
    TUniquePtr<CResourceScheduler> m_pResourceScheduler;

    std::mutex m_syncMutex;
    std::condition_variable m_syncCV;
    std::atomic<TUInt32> m_uiSyncCount{0};
    // Barrier generation counter — incremented each time the barrier opens
    // so a fast thread re-entering the next cycle cannot be mistaken for a
    // still-waiting thread. Guarded by m_syncMutex.
    TUInt32 m_uiSyncGeneration{0};
    std::atomic<TUInt32> m_uiCompletedSockets{0};
};

} // namespace TestMATE
