/**************************************************************************
 * File Name: ExecutionContext.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Execution context implementation
 * Requirements: REQ-PM-058 to REQ-PM-065
 **************************************************************************/

#include "ExecutionContext.h"

namespace TestMATE {

CExecutionContext::CExecutionContext(TSocketId in_uiSocketId, TSiteId in_uiSiteId)
    : m_uiSocketId(in_uiSocketId)
    , m_uiSiteId(in_uiSiteId)
    , m_uiExecutionId(0)
    , m_bAbortRequested(false)
    , m_eCurrentVerdict(ETestVerdict::kNone)
    , m_uiPassCount(0)
    , m_uiFailCount(0)
    , m_startTime(std::chrono::steady_clock::now())
{
}

} // namespace TestMATE
