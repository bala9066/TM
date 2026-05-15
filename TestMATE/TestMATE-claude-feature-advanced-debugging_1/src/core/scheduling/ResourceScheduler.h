/**************************************************************************
 * File Name: ResourceScheduler.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Resource scheduler for managing shared and exclusive
 *              resource access in parallel testing.
 * Requirements: REQ-RES-001 to REQ-RES-023
 **************************************************************************/

#pragma once

#include "testmate/common/Types.h"
#include "testmate/common/Result.h"
#include <mutex>
#include <condition_variable>
#include <set>
#include <queue>

namespace TestMATE {

/**************************************************************************
 * Struct: SResource
 * Description: Represents a managed resource
 **************************************************************************/
struct SResource {
    TResourceId id{0};
    TString name;
    EResourceAccessType accessType{EResourceAccessType::kExclusive};
    TUInt32 maxConcurrent{1};  ///< Max concurrent users (for shared)

    // Current state
    TUInt32 currentUsers{0};
    TSocketId exclusiveOwner{kInvalidSocketId};
    std::set<TSocketId> sharedOwners;
};

/**************************************************************************
 * Struct: SResourceRequest
 * Description: Request for resource allocation
 **************************************************************************/
struct SResourceRequest {
    TResourceId resourceId;
    TSocketId requesterId;
    EResourceAccessType accessType;
    TTimePoint requestTime;
    TInt64 timeoutMs;
};

/**************************************************************************
 * Class: CResourceScheduler
 * Description: Manages resource allocation and deallocation with
 *              deadlock detection and fair scheduling.
 * Requirements: REQ-RES-001 to REQ-RES-023
 **************************************************************************/
class CResourceScheduler {
public:
    CResourceScheduler();
    ~CResourceScheduler();

    CResourceScheduler(const CResourceScheduler&) = delete;
    CResourceScheduler& operator=(const CResourceScheduler&) = delete;

    //=========================================================================
    // Resource Registration
    //=========================================================================

    /**************************************************************************
     * Function Name: RegisterResource
     * Description: Registers a new resource with the scheduler
     * Parameters:
     *   in_strName - Resource name
     *   in_eAccessType - Exclusive or shared access
     *   in_uiMaxConcurrent - Max concurrent users (for shared)
     * Returns: Resource ID
     * Requirements: REQ-RES-001
     **************************************************************************/
    TResourceId RegisterResource(const TString& in_strName,
                                  EResourceAccessType in_eAccessType = EResourceAccessType::kExclusive,
                                  TUInt32 in_uiMaxConcurrent = 1);

    /**************************************************************************
     * Function Name: UnregisterResource
     * Description: Removes a resource from the scheduler
     * Parameters:
     *   in_resourceId - Resource to remove
     * Returns: Result indicating success or failure
     **************************************************************************/
    CResult UnregisterResource(TResourceId in_resourceId);

    //=========================================================================
    // Resource Allocation
    //=========================================================================

    /**************************************************************************
     * Function Name: Acquire
     * Description: Acquires a resource (blocking)
     * Parameters:
     *   in_resourceId - Resource to acquire
     *   in_socketId - Requester socket ID
     *   in_timeoutMs - Timeout (0 = infinite)
     * Returns: Result with success or error
     * Requirements: REQ-RES-002, REQ-RES-003
     **************************************************************************/
    CResult Acquire(TResourceId in_resourceId,
                    TSocketId in_socketId,
                    TInt64 in_timeoutMs = 0);

    /**************************************************************************
     * Function Name: TryAcquire
     * Description: Attempts to acquire resource without blocking
     * Parameters:
     *   in_resourceId - Resource to acquire
     *   in_socketId - Requester socket ID
     * Returns: true if acquired, false if not available
     * Requirements: REQ-RES-004
     **************************************************************************/
    bool TryAcquire(TResourceId in_resourceId, TSocketId in_socketId);

    /**************************************************************************
     * Function Name: Release
     * Description: Releases a previously acquired resource
     * Parameters:
     *   in_resourceId - Resource to release
     *   in_socketId - Owner socket ID
     * Returns: Result indicating success or failure
     * Requirements: REQ-RES-005
     **************************************************************************/
    CResult Release(TResourceId in_resourceId, TSocketId in_socketId);

    /**************************************************************************
     * Function Name: ReleaseAll
     * Description: Releases all resources held by a socket
     * Parameters:
     *   in_socketId - Socket to release resources for
     * Requirements: REQ-RES-006
     **************************************************************************/
    void ReleaseAll(TSocketId in_socketId);

    //=========================================================================
    // Resource Set Operations
    //=========================================================================

    /**************************************************************************
     * Function Name: AcquireSet
     * Description: Atomically acquires multiple resources
     * Parameters:
     *   in_vecResourceIds - Resources to acquire
     *   in_socketId - Requester socket ID
     *   in_timeoutMs - Timeout
     * Returns: Result indicating success or failure
     * Requirements: REQ-RES-007, REQ-RES-008
     **************************************************************************/
    CResult AcquireSet(const TVector<TResourceId>& in_vecResourceIds,
                       TSocketId in_socketId,
                       TInt64 in_timeoutMs = 0);

    /**************************************************************************
     * Function Name: ReleaseSet
     * Description: Releases multiple resources
     * Parameters:
     *   in_vecResourceIds - Resources to release
     *   in_socketId - Owner socket ID
     * Requirements: REQ-RES-009
     **************************************************************************/
    void ReleaseSet(const TVector<TResourceId>& in_vecResourceIds,
                    TSocketId in_socketId);

    //=========================================================================
    // Query Methods
    //=========================================================================

    /**************************************************************************
     * Function Name: IsAvailable
     * Description: Checks if resource is available for acquisition
     * Parameters:
     *   in_resourceId - Resource to check
     *   in_socketId - Potential acquirer
     * Returns: true if available
     * Requirements: REQ-RES-010
     **************************************************************************/
    [[nodiscard]] bool IsAvailable(TResourceId in_resourceId,
                                    TSocketId in_socketId) const;

    /**************************************************************************
     * Function Name: GetOwner
     * Description: Gets current owner of exclusive resource
     * Parameters:
     *   in_resourceId - Resource to query
     * Returns: Owner socket ID or kInvalidSocketId
     **************************************************************************/
    [[nodiscard]] TSocketId GetOwner(TResourceId in_resourceId) const;

    /**************************************************************************
     * Function Name: GetResourceCount
     * Description: Returns total registered resources
     * Returns: Resource count
     **************************************************************************/
    [[nodiscard]] TUInt32 GetResourceCount() const;

    //=========================================================================
    // Deadlock Detection
    //=========================================================================

    /**************************************************************************
     * Function Name: CheckForDeadlock
     * Description: Checks if acquiring resource would cause deadlock
     * Parameters:
     *   in_resourceId - Resource to check
     *   in_socketId - Potential acquirer
     * Returns: true if deadlock would occur
     * Requirements: REQ-RES-015 to REQ-RES-019
     **************************************************************************/
    [[nodiscard]] bool CheckForDeadlock(TResourceId in_resourceId,
                                         TSocketId in_socketId) const;

private:
    bool IsResourceAvailable(const SResource& resource, TSocketId socketId) const;
    void AcquireInternal(SResource& resource, TSocketId socketId);
    void ReleaseInternal(SResource& resource, TSocketId socketId);

    TMap<TResourceId, SResource> m_mapResources;
    TMap<TSocketId, std::set<TResourceId>> m_mapHeldResources;
    TMap<TSocketId, std::set<TResourceId>> m_mapWaitingFor;

    TResourceId m_uiNextResourceId{1};
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace TestMATE
