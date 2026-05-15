/**************************************************************************
 * File Name: SyncPoint.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Synchronization primitives for parallel execution.
 * Requirements: REQ-THR-021 to REQ-THR-029
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace TestMATE {

/**************************************************************************
 * Class: CSyncPoint
 * Description: Synchronization point where multiple threads wait until
 *              all participants arrive before proceeding.
 * Requirements: REQ-THR-021 to REQ-THR-025
 **************************************************************************/
class CSyncPoint {
public:
    /**************************************************************************
     * Function Name: CSyncPoint
     * Description: Creates sync point for specified number of participants
     * Parameters:
     *   in_uiParticipants - Number of threads that must arrive
     **************************************************************************/
    explicit CSyncPoint(TUInt32 in_uiParticipants);

    ~CSyncPoint() = default;

    // Non-copyable
    CSyncPoint(const CSyncPoint&) = delete;
    CSyncPoint& operator=(const CSyncPoint&) = delete;

    /**************************************************************************
     * Function Name: Wait
     * Description: Waits at sync point until all participants arrive
     * Returns: Result indicating success or failure
     * Requirements: REQ-THR-022
     **************************************************************************/
    CResult Wait();

    /**************************************************************************
     * Function Name: WaitFor
     * Description: Waits with timeout
     * Parameters:
     *   in_timeoutMs - Maximum time to wait in milliseconds
     * Returns: Result with kTimeout if timed out
     * Requirements: REQ-THR-023
     **************************************************************************/
    CResult WaitFor(TInt64 in_timeoutMs);

    /**************************************************************************
     * Function Name: Reset
     * Description: Resets sync point for reuse
     * Requirements: REQ-THR-024
     **************************************************************************/
    void Reset();

    /**************************************************************************
     * Function Name: GetArrivedCount
     * Description: Returns number of threads currently waiting
     * Returns: Arrived count
     **************************************************************************/
    [[nodiscard]] TUInt32 GetArrivedCount() const { return m_uiArrived.load(); }

    /**************************************************************************
     * Function Name: GetParticipantCount
     * Description: Returns total expected participants
     * Returns: Participant count
     **************************************************************************/
    [[nodiscard]] TUInt32 GetParticipantCount() const { return m_uiParticipants; }

private:
    TUInt32 m_uiParticipants;
    std::atomic<TUInt32> m_uiArrived{0};
    std::atomic<TUInt32> m_uiGeneration{0};
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

/**************************************************************************
 * Class: CBarrier
 * Description: Reusable barrier that can be used multiple times.
 * Requirements: REQ-THR-026 to REQ-THR-029
 **************************************************************************/
class CBarrier {
public:
    /**************************************************************************
     * Function Name: CBarrier
     * Description: Creates barrier for specified count
     * Parameters:
     *   in_uiCount - Number of threads to synchronize
     **************************************************************************/
    explicit CBarrier(TUInt32 in_uiCount);

    ~CBarrier() = default;

    CBarrier(const CBarrier&) = delete;
    CBarrier& operator=(const CBarrier&) = delete;

    /**************************************************************************
     * Function Name: Arrive
     * Description: Arrives at barrier and waits for all others
     * Returns: true if this thread was the last to arrive
     * Requirements: REQ-THR-027
     **************************************************************************/
    bool Arrive();

    /**************************************************************************
     * Function Name: ArriveAndWait
     * Description: Arrives and waits with timeout
     * Parameters:
     *   in_timeoutMs - Maximum wait time
     * Returns: Result with success or timeout
     * Requirements: REQ-THR-028
     **************************************************************************/
    CResult ArriveAndWait(TInt64 in_timeoutMs);

    /**************************************************************************
     * Function Name: Reset
     * Description: Resets barrier for new synchronization cycle
     * Parameters:
     *   in_uiNewCount - Optional new count (0 = keep current)
     **************************************************************************/
    void Reset(TUInt32 in_uiNewCount = 0);

    [[nodiscard]] TUInt32 GetCount() const { return m_uiCount; }
    [[nodiscard]] TUInt32 GetWaiting() const { return m_uiWaiting.load(); }

private:
    TUInt32 m_uiCount;
    std::atomic<TUInt32> m_uiWaiting{0};
    std::atomic<TUInt32> m_uiGeneration{0};
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace TestMATE
