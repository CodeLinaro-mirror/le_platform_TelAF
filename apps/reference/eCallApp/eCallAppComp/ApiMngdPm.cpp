/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiMngdPm.hpp"

#include <new>
#include <cstring>

namespace ecall
{

#define LOG_NAME "ApiMngdPm"

// =============================================================================
// Callback-result structs posted back to the caller thread
// =============================================================================
namespace
{

struct HoldResult
{
    HoldPowerResultCb cb;
    le_result_t       result{LE_FAULT};
    int               id{-1};
};

struct UpdateResult
{
    UpdatePowerResultCb cb;
    le_result_t         result{LE_FAULT};
    int                 newId{-1};
};

struct ReleaseResult
{
    ReleasePowerResultCb cb;
    le_result_t          result{LE_FAULT};
    int                  id{-1};
};

// Deferred functions posted to the caller thread.
void PostHoldResult(void* ctx, void*)
{
    auto* r = static_cast<HoldResult*>(ctx);
    if (r && r->cb) r->cb(r->result, r->id);
    delete r;
}

void PostUpdateResult(void* ctx, void*)
{
    auto* r = static_cast<UpdateResult*>(ctx);
    if (r && r->cb) r->cb(r->result, r->newId);
    delete r;
}

void PostReleaseResult(void* ctx, void*)
{
    auto* r = static_cast<ReleaseResult*>(ctx);
    if (r && r->cb) r->cb(r->result, r->id);
    delete r;
}

} // anonymous namespace

// =============================================================================
// Singleton
// =============================================================================
ApiMngdPm& ApiMngdPm::GetInstance()
{
    static ApiMngdPm instance;
    return instance;
}

// =============================================================================
// Ctor / Dtor
// =============================================================================
ApiMngdPm::ApiMngdPm()
{
    stopSem_ = le_sem_Create("ApiMngdPmStopSem", 0);
    LE_ASSERT(stopSem_ != nullptr);

    workerThreadRef_ = le_thread_Create("ApiMngdPmWorker",
                                        &ApiMngdPm::WorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_Start(workerThreadRef_);
    LE_INFO("[%s] worker thread started", LOG_NAME);
}

ApiMngdPm::~ApiMngdPm()
{
    // Enqueue a StopLoop request; the worker will delete all remaining
    // wakeup sources, post stopSem_, then return from the worker thread.
    auto* req = new (std::nothrow) Req;
    if (req)
    {
        req->op   = Op::StopLoop;
        req->self = this;
        le_event_QueueFunctionToThread(workerThreadRef_,
                                       &ApiMngdPm::DispatchOp,
                                       req, nullptr);
        le_sem_Wait(stopSem_);
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
void* ApiMngdPm::WorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiMngdPm*>(ctx);
    if (!self) return nullptr;

    self->WorkerInitOnThisThread();
    LE_INFO("[%s] entering RunLoop", LOG_NAME);
    le_event_RunLoop();
    LE_INFO("[%s] RunLoop exited", LOG_NAME);
    return nullptr;
}

void ApiMngdPm::WorkerInitOnThisThread()
{
    taf_mngdPm_ConnectService();
    LE_INFO("[%s] taf_mngdPm connected", LOG_NAME);
}

// =============================================================================
// DispatchOp  (worker thread)
// =============================================================================
void ApiMngdPm::DispatchOp(void* ctx, void* /*param2*/)
{
    auto* req = static_cast<Req*>(ctx);
    if (!req || !req->self)
    {
        LE_ERROR("[%s] DispatchOp: null req or self", LOG_NAME);
        delete req;
        return;
    }

    ApiMngdPm* self = req->self;

    switch (req->op)
    {
        case Op::Hold:    self->DoHoldPower(std::move(req->hold));         break;
        case Op::Update:  self->DoUpdateHoldPower(std::move(req->update)); break;
        case Op::Release: self->DoReleasePower(std::move(req->release));   break;
                case Op::StopLoop:
            self->DoStopLoop();
            delete req;
            return;   // worker thread exits RunLoop after all sources removed
        default:
            LE_ERROR("[%s] DispatchOp: unknown op=%d", LOG_NAME,
                     static_cast<int>(req->op));
            break;
    }

    delete req;
}

// =============================================================================
// Worker-thread operations
// =============================================================================

void ApiMngdPm::DoHoldPower(std::unique_ptr<HoldReqData> req)
{
    if (!req)
    {
        LE_ERROR("[%s] DoHoldPower: null req", LOG_NAME);
        return;
    }

    taf_mngdPm_wsRef_t ws = taf_mngdPm_CreateWakeupSource(
        static_cast<taf_mngdPm_StayAwakeReason_t>(req->reason),
        TAF_MNGDPM_WS_OPT_DEFAULT,
        req->tag);

    le_result_t result = LE_FAULT;
    int         id     = -1;

    if (!ws)
    {
        LE_ERROR("[%s] DoHoldPower: CreateWakeupSource failed", LOG_NAME);
    }
    else
    {
        le_result_t stayRc = taf_mngdPm_StayAwake(ws);
        if (stayRc != LE_OK)
        {
            LE_ERROR("[%s] DoHoldPower: StayAwake failed rc=%d", LOG_NAME, stayRc);
            le_result_t delRc = taf_mngdPm_DeleteWakeupSource(ws);
            if (delRc != LE_OK)
            {
                LE_WARN("[%s] DoHoldPower: cleanup DeleteWakeupSource failed rc=%d",
                        LOG_NAME, delRc);
            }
            result = stayRc;
        }
        else
        {
            id     = StoreWs(ws);
            result = LE_OK;
            LE_INFO("[%s] DoHoldPower: id=%d reason=%d",
                    LOG_NAME, id, static_cast<int>(req->reason));
        }
    }

    // Post result back to the caller's thread.
    if (req->cb && req->callerThread)
    {
        auto* r = new (std::nothrow) HoldResult{std::move(req->cb), result, id};
        if (r)
        {
            le_event_QueueFunctionToThread(req->callerThread,
                                           PostHoldResult, r, nullptr);
        }
    }
}

void ApiMngdPm::DoUpdateHoldPower(std::unique_ptr<UpdateReqData> req)
{
    if (!req)
    {
        LE_ERROR("[%s] DoUpdateHoldPower: null req", LOG_NAME);
        return;
    }

    taf_mngdPm_wsRef_t oldWs = TakeWs(req->oldId);
    if (!oldWs)
    {
        LE_WARN("[%s] DoUpdateHoldPower: no ws for oldId=%d", LOG_NAME, req->oldId);
        if (req->cb && req->callerThread)
        {
            auto* r = new (std::nothrow) UpdateResult{
                std::move(req->cb), LE_NOT_FOUND, -1};
            if (r)
                le_event_QueueFunctionToThread(req->callerThread,
                                               PostUpdateResult, r, nullptr);
        }
        return;
    }

    taf_mngdPm_wsRef_t newWs = taf_mngdPm_CreateWakeupSource(
        static_cast<taf_mngdPm_StayAwakeReason_t>(req->newReason),
        TAF_MNGDPM_WS_OPT_DEFAULT,
        req->tag);

    le_result_t result = LE_FAULT;
    int         newId  = -1;

    if (!newWs)
    {
        LE_ERROR("[%s] DoUpdateHoldPower: CreateWakeupSource(new) failed; "
                 "restoring oldId=%d", LOG_NAME, req->oldId);
        // Restore old ws so the caller still has a valid hold.
        wsMap_[req->oldId] = oldWs;
        newId  = req->oldId;
        result = LE_FAULT;
    }
    else
    {
        le_result_t stayRc = taf_mngdPm_StayAwake(newWs);
        if (stayRc != LE_OK)
        {
            LE_ERROR("[%s] DoUpdateHoldPower: StayAwake(new) failed rc=%d; "
                     "restoring oldId=%d", LOG_NAME, stayRc, req->oldId);
            le_result_t delNewRc = taf_mngdPm_DeleteWakeupSource(newWs);
            if (delNewRc != LE_OK)
            {
                LE_WARN("[%s] DoUpdateHoldPower: cleanup DeleteWakeupSource(new) failed rc=%d",
                        LOG_NAME, delNewRc);
            }
            wsMap_[req->oldId] = oldWs;
            newId  = req->oldId;
            result = stayRc;
        }
        else
        {
            newId  = StoreWs(newWs);
            result = LE_OK;

            // New source is now active (StayAwake succeeded).
            // Relax old source first, then delete it.
            // Order: StayAwake(new) -> Relax(old) -> Delete(old).
            // Because StayAwake(new) already completed above, wsCount
            // goes new+old -> new during Relax(old), never hits zero.
            le_result_t relaxRc = taf_mngdPm_Relax(oldWs);
            if (relaxRc != LE_OK)
            {
                LE_WARN("[%s] DoUpdateHoldPower: Relax(oldId=%d) failed rc=%d",
                        LOG_NAME, req->oldId, relaxRc);
            }

            le_result_t delOldRc = taf_mngdPm_DeleteWakeupSource(oldWs);
            if (delOldRc != LE_OK)
            {
                LE_WARN("[%s] DoUpdateHoldPower: DeleteWakeupSource(oldId=%d) failed rc=%d",
                        LOG_NAME, req->oldId, delOldRc);
            }

            LE_INFO("[%s] DoUpdateHoldPower: oldId=%d -> newId=%d reason=%d",
                    LOG_NAME, req->oldId, newId,
                    static_cast<int>(req->newReason));
        }
    }

    if (req->cb && req->callerThread)
    {
        auto* r = new (std::nothrow) UpdateResult{
            std::move(req->cb), result, newId};
        if (r)
            le_event_QueueFunctionToThread(req->callerThread,
                                           PostUpdateResult, r, nullptr);
    }
}

void ApiMngdPm::DoReleasePower(std::unique_ptr<ReleaseReqData> req)
{
    if (!req)
    {
        LE_ERROR("[%s] DoReleasePower: null req", LOG_NAME);
        return;
    }

    taf_mngdPm_wsRef_t ws = TakeWs(req->id);

    le_result_t result = LE_NOT_FOUND;
    if (ws)
    {
        // Correct MPMS lifecycle is Relax first, then DeleteWakeupSource.
        // Still attempt Delete even if Relax fails, because the source may
        // already be relaxed and deletable.
        le_result_t relaxRc = taf_mngdPm_Relax(ws);
        if (relaxRc != LE_OK)
        {
            LE_WARN("[%s] DoReleasePower: Relax failed rc=%d id=%d; trying Delete anyway",
                    LOG_NAME, relaxRc, req->id);
        }

        le_result_t delRc = taf_mngdPm_DeleteWakeupSource(ws);
        if (delRc != LE_OK)
        {
            LE_ERROR("[%s] DoReleasePower: DeleteWakeupSource failed rc=%d id=%d",
                     LOG_NAME, delRc, req->id);
            // Restore the ws so the caller can retry release later.
            wsMap_[req->id] = ws;
            result = delRc;
        }
        else
        {
            result = LE_OK;
            LE_INFO("[%s] DoReleasePower: id=%d relaxed(rc=%d) and deleted",
                    LOG_NAME, req->id, relaxRc);
        }
    }
    else
    {
        LE_WARN("[%s] DoReleasePower: no ws for id=%d", LOG_NAME, req->id);
    }

    if (req->cb && req->callerThread)
    {
        auto* r = new (std::nothrow) ReleaseResult{
            std::move(req->cb), result, req->id};
        if (r)
            le_event_QueueFunctionToThread(req->callerThread,
                                           PostReleaseResult, r, nullptr);
    }
}

void ApiMngdPm::DoStopLoop()
{
    LE_INFO("[%s] DoStopLoop: deleting %zu wakeup sources",
            LOG_NAME, wsMap_.size());
        DeleteAllWs();
    le_sem_Post(stopSem_);
    // Worker thread will return from le_event_RunLoop naturally
    // when there are no more event sources registered.
}

// =============================================================================
// Wakeup-source map helpers  (worker thread only)
// =============================================================================

taf_mngdPm_wsRef_t ApiMngdPm::TakeWs(int id)
{
    auto it = wsMap_.find(id);
    if (it == wsMap_.end()) return nullptr;
    taf_mngdPm_wsRef_t ws = it->second;
    wsMap_.erase(it);
    return ws;
}

int ApiMngdPm::StoreWs(taf_mngdPm_wsRef_t ws)
{
    int id = nextId_++;
    wsMap_[id] = ws;
    return id;
}

void ApiMngdPm::DeleteAllWs()
{
    for (auto& kv : wsMap_)
    {
        if (kv.second)
        {
            le_result_t relaxRc = taf_mngdPm_Relax(kv.second);
            if (relaxRc != LE_OK)
            {
                LE_WARN("[%s] DeleteAllWs: Relax failed rc=%d id=%d",
                        LOG_NAME, relaxRc, kv.first);
            }

            le_result_t delRc = taf_mngdPm_DeleteWakeupSource(kv.second);
            if (delRc != LE_OK)
            {
                LE_WARN("[%s] DeleteAllWs: DeleteWakeupSource failed rc=%d id=%d",
                        LOG_NAME, delRc, kv.first);
            }
        }
    }
    wsMap_.clear();
}

// =============================================================================
// Public async API
// =============================================================================

void ApiMngdPm::HoldPowerAsync(StayAwakeReason   reason,
                               const char*       tag,
                               HoldPowerResultCb cb)
{
    auto* req = new (std::nothrow) Req;
    if (!req)
    {
        LE_ERROR("[%s] HoldPowerAsync: OOM", LOG_NAME);
        if (cb) cb(LE_NO_MEMORY, -1);
        return;
    }

    req->op   = Op::Hold;
    req->self = this;
    req->hold = std::make_unique<HoldReqData>();
    req->hold->reason       = reason;
    req->hold->callerThread = le_thread_GetCurrent();
    req->hold->cb           = std::move(cb);
    if (tag)
        snprintf(req->hold->tag, sizeof(req->hold->tag), "%s", tag);

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiMngdPm::DispatchOp, req, nullptr);
}

void ApiMngdPm::UpdateHoldPowerReasonAsync(int                 oldId,
                                           StayAwakeReason     newReason,
                                           const char*         tag,
                                           UpdatePowerResultCb cb)
{
    auto* req = new (std::nothrow) Req;
    if (!req)
    {
        LE_ERROR("[%s] UpdateHoldPowerReasonAsync: OOM", LOG_NAME);
        if (cb) cb(LE_NO_MEMORY, -1);
        return;
    }

    req->op     = Op::Update;
    req->self   = this;
    req->update = std::make_unique<UpdateReqData>();
    req->update->oldId        = oldId;
    req->update->newReason    = newReason;
    req->update->callerThread = le_thread_GetCurrent();
    req->update->cb           = std::move(cb);
    if (tag)
        snprintf(req->update->tag, sizeof(req->update->tag), "%s", tag);;

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiMngdPm::DispatchOp, req, nullptr);
}

void ApiMngdPm::ReleasePowerAsync(int                  id,
                                  ReleasePowerResultCb cb)
{
    auto* req = new (std::nothrow) Req;
    if (!req)
    {
        LE_ERROR("[%s] ReleasePowerAsync: OOM", LOG_NAME);
        if (cb) cb(LE_NO_MEMORY, id);
        return;
    }

    req->op      = Op::Release;
    req->self    = this;
    req->release = std::make_unique<ReleaseReqData>();
    req->release->id           = id;
    req->release->callerThread = le_thread_GetCurrent();
    req->release->cb           = std::move(cb);

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiMngdPm::DispatchOp, req, nullptr);
}

} // namespace ecall

