/**************************************************************************
 * File Name: SyncPoint.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of synchronization primitives
 * Requirements: REQ-THR-021 to REQ-THR-029
 **************************************************************************/

#include "SyncPoint.h"

namespace TestMATE {

//=============================================================================
// CSyncPoint Implementation
//=============================================================================

CSyncPoint::CSyncPoint(TUInt32 in_uiParticipants)
    : m_uiParticipants(in_uiParticipants)
{
    if (m_uiParticipants == 0) {
        m_uiParticipants = 1;
    }
}

CResult CSyncPoint::Wait() {
    return WaitFor(0);  // 0 = infinite wait
}

CResult CSyncPoint::WaitFor(TInt64 in_timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    TUInt32 myGeneration = m_uiGeneration.load();

    if (++m_uiArrived == m_uiParticipants) {
        // Last to arrive - release all
        ++m_uiGeneration;
        m_uiArrived = 0;
        m_cv.notify_all();
        return TESTMATE_SUCCESS();
    }

    // Wait for others
    if (in_timeoutMs <= 0) {
        m_cv.wait(lock, [&]() {
            return m_uiGeneration.load() != myGeneration;
        });
    } else {
        bool result = m_cv.wait_for(lock,
                                    std::chrono::milliseconds(in_timeoutMs),
                                    [&]() {
            return m_uiGeneration.load() != myGeneration;
        });

        if (!result) {
            --m_uiArrived;
            return TESTMATE_FAILURE(EErrorCode::kSyncPointTimeout,
                                    "Sync point wait timed out");
        }
    }

    return TESTMATE_SUCCESS();
}

void CSyncPoint::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uiArrived = 0;
    ++m_uiGeneration;
}

//=============================================================================
// CBarrier Implementation
//=============================================================================

CBarrier::CBarrier(TUInt32 in_uiCount)
    : m_uiCount(in_uiCount)
{
    if (m_uiCount == 0) {
        m_uiCount = 1;
    }
}

bool CBarrier::Arrive() {
    std::unique_lock<std::mutex> lock(m_mutex);

    TUInt32 myGeneration = m_uiGeneration.load();
    bool isLast = (++m_uiWaiting == m_uiCount);

    if (isLast) {
        ++m_uiGeneration;
        m_uiWaiting = 0;
        m_cv.notify_all();
        return true;
    }

    m_cv.wait(lock, [&]() {
        return m_uiGeneration.load() != myGeneration;
    });

    return false;
}

CResult CBarrier::ArriveAndWait(TInt64 in_timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    TUInt32 myGeneration = m_uiGeneration.load();

    if (++m_uiWaiting == m_uiCount) {
        ++m_uiGeneration;
        m_uiWaiting = 0;
        m_cv.notify_all();
        return TESTMATE_SUCCESS();
    }

    if (in_timeoutMs <= 0) {
        m_cv.wait(lock, [&]() {
            return m_uiGeneration.load() != myGeneration;
        });
    } else {
        bool result = m_cv.wait_for(lock,
                                    std::chrono::milliseconds(in_timeoutMs),
                                    [&]() {
            return m_uiGeneration.load() != myGeneration;
        });

        if (!result) {
            --m_uiWaiting;
            return TESTMATE_FAILURE(EErrorCode::kTimeout,
                                    "Barrier wait timed out");
        }
    }

    return TESTMATE_SUCCESS();
}

void CBarrier::Reset(TUInt32 in_uiNewCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (in_uiNewCount > 0) {
        m_uiCount = in_uiNewCount;
    }
    m_uiWaiting = 0;
    ++m_uiGeneration;
}

} // namespace TestMATE
