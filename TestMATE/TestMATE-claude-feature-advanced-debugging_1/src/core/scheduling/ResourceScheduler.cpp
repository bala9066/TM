/**************************************************************************
 * File Name: ResourceScheduler.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of resource scheduler
 * Requirements: REQ-RES-001 to REQ-RES-023
 **************************************************************************/

#include "ResourceScheduler.h"
#include "utils/LogManager.h"
#include "utils/TimeUtils.h"

namespace TestMATE {

namespace {
    const char* kLogSource = "ResourceScheduler";
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CResourceScheduler::CResourceScheduler() = default;

CResourceScheduler::~CResourceScheduler() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapResources.clear();
    m_mapHeldResources.clear();
    m_mapWaitingFor.clear();
}

//=============================================================================
// Resource Registration
//=============================================================================

TResourceId CResourceScheduler::RegisterResource(const TString& in_strName,
                                                   EResourceAccessType in_eAccessType,
                                                   TUInt32 in_uiMaxConcurrent) {
    std::lock_guard<std::mutex> lock(m_mutex);

    TResourceId id = m_uiNextResourceId++;

    SResource resource;
    resource.id = id;
    resource.name = in_strName;
    resource.accessType = in_eAccessType;
    resource.maxConcurrent = (in_eAccessType == EResourceAccessType::kShared)
                             ? std::max(1u, in_uiMaxConcurrent)
                             : 1;

    m_mapResources[id] = std::move(resource);

    LOG_DEBUG(kLogSource, "Registered resource '{}' with ID {}", in_strName, id);

    return id;
}

CResult CResourceScheduler::UnregisterResource(TResourceId in_resourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return TESTMATE_FAILURE(EErrorCode::kResourceNotFound,
                                "Resource not found");
    }

    if (it->second.currentUsers > 0) {
        return TESTMATE_FAILURE(EErrorCode::kResourceBusy,
                                "Cannot unregister resource with active users");
    }

    m_mapResources.erase(it);
    return TESTMATE_SUCCESS();
}

//=============================================================================
// Resource Allocation
//=============================================================================

CResult CResourceScheduler::Acquire(TResourceId in_resourceId,
                                     TSocketId in_socketId,
                                     TInt64 in_timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return TESTMATE_FAILURE(EErrorCode::kResourceNotFound,
                                "Resource not found");
    }

    SResource& resource = it->second;

    // Check for potential deadlock
    if (CheckForDeadlock(in_resourceId, in_socketId)) {
        LOG_WARNING(kLogSource, "Deadlock detected for socket {} requesting resource {}",
                    in_socketId, in_resourceId);
        return TESTMATE_FAILURE(EErrorCode::kDeadlockDetected,
                                "Acquiring resource would cause deadlock");
    }

    // Mark as waiting
    m_mapWaitingFor[in_socketId].insert(in_resourceId);

    auto predicate = [&]() {
        return IsResourceAvailable(resource, in_socketId);
    };

    bool acquired = false;

    if (in_timeoutMs <= 0) {
        m_cv.wait(lock, predicate);
        acquired = true;
    } else {
        acquired = m_cv.wait_for(lock,
                                 std::chrono::milliseconds(in_timeoutMs),
                                 predicate);
    }

    // Remove from waiting
    m_mapWaitingFor[in_socketId].erase(in_resourceId);

    if (!acquired) {
        return TESTMATE_FAILURE(EErrorCode::kResourceTimeout,
                                "Timeout waiting for resource");
    }

    AcquireInternal(resource, in_socketId);

    LOG_TRACE(kLogSource, "Socket {} acquired resource {}", in_socketId, in_resourceId);

    return TESTMATE_SUCCESS();
}

bool CResourceScheduler::TryAcquire(TResourceId in_resourceId, TSocketId in_socketId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return false;
    }

    SResource& resource = it->second;

    if (!IsResourceAvailable(resource, in_socketId)) {
        return false;
    }

    AcquireInternal(resource, in_socketId);
    return true;
}

CResult CResourceScheduler::Release(TResourceId in_resourceId, TSocketId in_socketId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return TESTMATE_FAILURE(EErrorCode::kResourceNotFound,
                                "Resource not found");
    }

    SResource& resource = it->second;

    // Verify ownership
    if (resource.accessType == EResourceAccessType::kExclusive) {
        if (resource.exclusiveOwner != in_socketId) {
            return TESTMATE_FAILURE(EErrorCode::kResourceReleaseFailed,
                                    "Socket does not own this resource");
        }
    } else {
        if (resource.sharedOwners.find(in_socketId) == resource.sharedOwners.end()) {
            return TESTMATE_FAILURE(EErrorCode::kResourceReleaseFailed,
                                    "Socket does not own this resource");
        }
    }

    ReleaseInternal(resource, in_socketId);
    m_cv.notify_all();

    LOG_TRACE(kLogSource, "Socket {} released resource {}", in_socketId, in_resourceId);

    return TESTMATE_SUCCESS();
}

void CResourceScheduler::ReleaseAll(TSocketId in_socketId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapHeldResources.find(in_socketId);
    if (it == m_mapHeldResources.end()) {
        return;
    }

    for (TResourceId resourceId : it->second) {
        auto resIt = m_mapResources.find(resourceId);
        if (resIt != m_mapResources.end()) {
            ReleaseInternal(resIt->second, in_socketId);
        }
    }

    m_mapHeldResources.erase(it);
    m_cv.notify_all();

    LOG_DEBUG(kLogSource, "Released all resources for socket {}", in_socketId);
}

//=============================================================================
// Resource Set Operations
//=============================================================================

CResult CResourceScheduler::AcquireSet(const TVector<TResourceId>& in_vecResourceIds,
                                        TSocketId in_socketId,
                                        TInt64 in_timeoutMs) {
    if (in_vecResourceIds.empty()) {
        return TESTMATE_SUCCESS();
    }

    // Sort to prevent deadlock from inconsistent ordering
    TVector<TResourceId> sortedIds = in_vecResourceIds;
    std::sort(sortedIds.begin(), sortedIds.end());

    for (TResourceId resourceId : sortedIds) {
        CResult result = Acquire(resourceId, in_socketId, in_timeoutMs);
        if (result.IsFailure()) {
            // Rollback previously acquired
            for (TResourceId acquired : sortedIds) {
                if (acquired == resourceId) break;
                Release(acquired, in_socketId);
            }
            return result;
        }
    }

    return TESTMATE_SUCCESS();
}

void CResourceScheduler::ReleaseSet(const TVector<TResourceId>& in_vecResourceIds,
                                     TSocketId in_socketId) {
    for (TResourceId resourceId : in_vecResourceIds) {
        Release(resourceId, in_socketId);
    }
}

//=============================================================================
// Query Methods
//=============================================================================

bool CResourceScheduler::IsAvailable(TResourceId in_resourceId,
                                      TSocketId in_socketId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return false;
    }

    return IsResourceAvailable(it->second, in_socketId);
}

TSocketId CResourceScheduler::GetOwner(TResourceId in_resourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return kInvalidSocketId;
    }

    return it->second.exclusiveOwner;
}

TUInt32 CResourceScheduler::GetResourceCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<TUInt32>(m_mapResources.size());
}

//=============================================================================
// Deadlock Detection
//=============================================================================

bool CResourceScheduler::CheckForDeadlock(TResourceId in_resourceId,
                                           TSocketId in_socketId) const {
    // Simple cycle detection in wait-for graph
    auto it = m_mapResources.find(in_resourceId);
    if (it == m_mapResources.end()) {
        return false;
    }

    const SResource& resource = it->second;

    if (resource.accessType != EResourceAccessType::kExclusive) {
        return false;  // Shared resources don't cause deadlock
    }

    TSocketId currentOwner = resource.exclusiveOwner;
    if (currentOwner == kInvalidSocketId || currentOwner == in_socketId) {
        return false;
    }

    // Check if owner is waiting for something we hold
    std::set<TSocketId> visited;
    std::queue<TSocketId> toCheck;
    toCheck.push(currentOwner);

    while (!toCheck.empty()) {
        TSocketId checkSocket = toCheck.front();
        toCheck.pop();

        if (visited.count(checkSocket)) {
            continue;
        }
        visited.insert(checkSocket);

        auto waitIt = m_mapWaitingFor.find(checkSocket);
        if (waitIt == m_mapWaitingFor.end()) {
            continue;
        }

        for (TResourceId waitingFor : waitIt->second) {
            auto resIt = m_mapResources.find(waitingFor);
            if (resIt == m_mapResources.end()) {
                continue;
            }

            if (resIt->second.exclusiveOwner == in_socketId) {
                return true;  // Cycle detected!
            }

            if (resIt->second.exclusiveOwner != kInvalidSocketId) {
                toCheck.push(resIt->second.exclusiveOwner);
            }
        }
    }

    return false;
}

//=============================================================================
// Private Methods
//=============================================================================

bool CResourceScheduler::IsResourceAvailable(const SResource& resource,
                                              TSocketId socketId) const {
    if (resource.accessType == EResourceAccessType::kExclusive) {
        return resource.exclusiveOwner == kInvalidSocketId ||
               resource.exclusiveOwner == socketId;
    } else {
        return resource.currentUsers < resource.maxConcurrent ||
               resource.sharedOwners.count(socketId) > 0;
    }
}

void CResourceScheduler::AcquireInternal(SResource& resource, TSocketId socketId) {
    if (resource.accessType == EResourceAccessType::kExclusive) {
        resource.exclusiveOwner = socketId;
    } else {
        resource.sharedOwners.insert(socketId);
    }
    resource.currentUsers++;
    m_mapHeldResources[socketId].insert(resource.id);
}

void CResourceScheduler::ReleaseInternal(SResource& resource, TSocketId socketId) {
    if (resource.accessType == EResourceAccessType::kExclusive) {
        resource.exclusiveOwner = kInvalidSocketId;
    } else {
        resource.sharedOwners.erase(socketId);
    }
    resource.currentUsers--;
    m_mapHeldResources[socketId].erase(resource.id);
}

} // namespace TestMATE
