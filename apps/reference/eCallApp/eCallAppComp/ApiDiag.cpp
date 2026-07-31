/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiDiag.hpp"

#include <new>

namespace ecall
{

#define LOG_NAME "ApiDiag"

// =============================================================================
// Singleton
// =============================================================================
ApiDiag& ApiDiag::GetInstance()
{
    static ApiDiag inst;
    return inst;
}

// =============================================================================
// Ctor / Dtor
// =============================================================================
ApiDiag::ApiDiag()
{
    stopSem_ = le_sem_Create("ApiDiagStopSem", 0);
    LE_ASSERT(stopSem_ != nullptr);

    workerThreadRef_ = le_thread_Create("ApiDiagWorker",
                                        &ApiDiag::WorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_Start(workerThreadRef_);
    LE_INFO("[%s] worker thread started", LOG_NAME);
}

ApiDiag::~ApiDiag()
{
    // RunOnWorkerSync owns and deletes the Req.
    Req* req = new (std::nothrow) Req;
    if (req)
    {
        req->op   = Op::StopLoop;
        req->self = this;
        req->sem  = le_sem_Create("ApiDiagStopReqSem", 0);
        if (req->sem)
        {
            le_event_QueueFunctionToThread(workerThreadRef_,
                                           &ApiDiag::DispatchOp,
                                           req, nullptr);
            le_sem_Wait(stopSem_);   // wait for ExitLoop acknowledgement
            le_sem_Delete(req->sem);
        }
        delete req;
    }

    if (workerThreadRef_)
    {
        le_thread_Join(workerThreadRef_, nullptr);
        workerThreadRef_ = nullptr;
    }

    le_sem_Delete(stopSem_);
    stopSem_ = nullptr;
}

// =============================================================================
// Worker thread
// =============================================================================
void* ApiDiag::WorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiDiag*>(ctx);
    if (!self) return nullptr;

    self->WorkerInitOnThisThread();
    LE_INFO("[%s] entering RunLoop", LOG_NAME);
    le_event_RunLoop();
    LE_INFO("[%s] RunLoop exited", LOG_NAME);
    return nullptr;
}

void ApiDiag::WorkerInitOnThisThread()
{
    taf_diagEvent_ConnectService();
    taf_diagDTC_ConnectService();
    taf_diag_ConnectService();
    LE_INFO("[%s] connected to taf_diagEvent, taf_diagDTC, taf_diag", LOG_NAME);
}

// =============================================================================
// DispatchOp  (worker thread)
// =============================================================================
void ApiDiag::DispatchOp(void* ctx, void* /*param2*/)
{
    Req* req = static_cast<Req*>(ctx);
    if (!req || !req->self) return;

    le_result_t r = LE_FAULT;

    switch (req->op)
    {
        case Op::GetEventService:
        {
            if (!req->evSvcOut) { r = LE_BAD_PARAMETER; break; }
            taf_diagEvent_ServiceRef_t ref = taf_diagEvent_GetService(req->eventId);
            if (!ref)
            {
                LE_ERROR("[%s] GetEventService failed eventId=%u", LOG_NAME, req->eventId);
                r = LE_NOT_FOUND;
            }
            else
            {
                *req->evSvcOut = ref;
                LE_DEBUG("[%s] GetEventService ok eventId=%u", LOG_NAME, req->eventId);
                r = LE_OK;
            }
            break;
        }

        case Op::GetDtcService:
        {
            if (!req->dtcSvcOut) { r = LE_BAD_PARAMETER; break; }
            taf_diagDTC_ServiceRef_t ref = taf_diagDTC_GetService(req->dtcCode);
            if (!ref)
            {
                LE_ERROR("[%s] GetDtcService failed dtc=0x%x", LOG_NAME, req->dtcCode);
                r = LE_NOT_FOUND;
            }
            else
            {
                *req->dtcSvcOut = ref;
                LE_DEBUG("[%s] GetDtcService ok dtc=0x%x", LOG_NAME, req->dtcCode);
                r = LE_OK;
            }
            break;
        }

        case Op::GetOpCycle:
        {
            if (!req->opCycleOut) { r = LE_BAD_PARAMETER; break; }
            taf_diagEvent_OpCycleRef_t ref = taf_diagEvent_GetOpCycle(req->opCycleId);
            if (!ref)
            {
                LE_ERROR("[%s] GetOpCycle failed id=%u", LOG_NAME, req->opCycleId);
                r = LE_NOT_FOUND;
            }
            else
            {
                *req->opCycleOut = ref;
                LE_DEBUG("[%s] GetOpCycle ok id=%u", LOG_NAME, req->opCycleId);
                r = LE_OK;
            }
            break;
        }

        case Op::SetOpCycleState:
        {
            if (!req->opCycleIn) { r = LE_BAD_PARAMETER; break; }
            r = taf_diagEvent_SetOpCycleState(req->opCycleIn, req->opCycleState);
            if (r != LE_OK)
                LE_ERROR("[%s] SetOpCycleState failed rc=%d state=%d",
                         LOG_NAME, r, static_cast<int>(req->opCycleState));
            break;
        }

        case Op::SetEventStatus:
        {
            if (!req->evSvcIn) { r = LE_BAD_PARAMETER; break; }
            r = taf_diagEvent_SetStatus(req->evSvcIn, req->eventStatus);
            if (r != LE_OK)
                LE_ERROR("[%s] SetEventStatus failed rc=%d status=%d",
                         LOG_NAME, r, static_cast<int>(req->eventStatus));
            break;
        }

        case Op::ReadDtcStatus:
        {
            if (!req->dtcSvcIn || !req->dtcStatusOut) { r = LE_BAD_PARAMETER; break; }
            r = taf_diagDTC_ReadStatus(req->dtcSvcIn, req->dtcStatusOut);
            if (r != LE_OK)
                LE_ERROR("[%s] ReadDtcStatus failed rc=%d", LOG_NAME, r);
            else
                LE_DEBUG("[%s] ReadDtcStatus ok status=0x%x",
                         LOG_NAME, *req->dtcStatusOut);
            break;
        }

        case Op::GetDtcCode:
        {
            if (!req->dtcSvcIn || !req->dtcCodeOut) { r = LE_BAD_PARAMETER; break; }
            r = taf_diagDTC_GetCode(req->dtcSvcIn, req->dtcCodeOut);
            if (r != LE_OK)
                LE_ERROR("[%s] GetDtcCode failed rc=%d", LOG_NAME, r);
            else
                LE_DEBUG("[%s] GetDtcCode ok code=0x%x",
                         LOG_NAME, *req->dtcCodeOut);
            break;
        }

        case Op::SetEnableCondition:
        {
            r = taf_diag_SetEnableCondition(req->enableConditionID,
                                            req->conditionFulfilled);
            if (r != LE_OK)
                LE_ERROR("[%s] SetEnableCondition failed rc=%d id=%u",
                         LOG_NAME, r, static_cast<unsigned>(req->enableConditionID));
            break;
        }

        // Post sem, then exit RunLoop so the thread can be joined.
        case Op::StopLoop:
            LE_INFO("[%s] StopLoop: exiting RunLoop", LOG_NAME);
                        r = LE_OK;
            req->result = r;
            if (req->sem) le_sem_Post(req->sem);
            le_sem_Post(req->self->stopSem_);
            return;   // worker thread exits RunLoop naturally

        default:
            LE_ERROR("[%s] Unknown op=%d", LOG_NAME, static_cast<int>(req->op));
            r = LE_FAULT;
            break;
    }

    req->result = r;
    if (req->sem) le_sem_Post(req->sem);
}

// =============================================================================
// RunOnWorkerSync
// =============================================================================
le_result_t ApiDiag::RunOnWorkerSync(Req* req)
{
    if (!req) return LE_BAD_PARAMETER;

    req->self = this;
    req->sem  = le_sem_Create("ApiDiagReqSem", 0);
    if (!req->sem)
    {
        delete req;
        return LE_FAULT;
    }

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiDiag::DispatchOp,
                                   req, nullptr);
    le_sem_Wait(req->sem);
    le_result_t rc = req->result;
    le_sem_Delete(req->sem);
    req->sem = nullptr;
    delete req;
    return rc;
}

// =============================================================================
// Public API implementations
// =============================================================================

le_result_t ApiDiag::GetEventService(uint16_t                    eventId,
                                     taf_diagEvent_ServiceRef_t* out)
{
    if (!out) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op       = Op::GetEventService;
    req->eventId  = eventId;
    req->evSvcOut = out;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::GetDtcService(uint32_t                  dtcCode,
                                   taf_diagDTC_ServiceRef_t* out)
{
    if (!out) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op        = Op::GetDtcService;
    req->dtcCode   = dtcCode;
    req->dtcSvcOut = out;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::GetOpCycle(uint8_t                     opCycleId,
                                taf_diagEvent_OpCycleRef_t* out)
{
    if (!out) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op         = Op::GetOpCycle;
    req->opCycleId  = opCycleId;
    req->opCycleOut = out;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::SetOpCycleState(taf_diagEvent_OpCycleRef_t          opCycleRef,
                                     taf_diagEvent_OperationCycleState_t state)
{
    if (!opCycleRef) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op           = Op::SetOpCycleState;
    req->opCycleIn    = opCycleRef;
    req->opCycleState = state;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::SetEventStatus(taf_diagEvent_ServiceRef_t evSvc,
                                    taf_diagEvent_StatusType_t status)
{
    if (!evSvc) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op          = Op::SetEventStatus;
    req->evSvcIn     = evSvc;
    req->eventStatus = status;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::ReadDtcStatus(taf_diagDTC_ServiceRef_t dtcSvc,
                                   uint8_t*                 dtcStatus)
{
    if (!dtcSvc || !dtcStatus) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op           = Op::ReadDtcStatus;
    req->dtcSvcIn     = dtcSvc;
    req->dtcStatusOut = dtcStatus;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::GetDtcCode(taf_diagDTC_ServiceRef_t dtcSvc,
                                uint32_t*               dtcCode)
{
    if (!dtcSvc || !dtcCode) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op         = Op::GetDtcCode;
    req->dtcSvcIn   = dtcSvc;
    req->dtcCodeOut = dtcCode;
    return RunOnWorkerSync(req);
}

le_result_t ApiDiag::SetEnableCondition(uint8_t enableConditionID,
                                        bool    conditionFulfilled)
{
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;
    req->op                 = Op::SetEnableCondition;
    req->enableConditionID  = enableConditionID;
    req->conditionFulfilled = conditionFulfilled;
    return RunOnWorkerSync(req);
}

} // namespace ecall
