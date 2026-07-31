/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

extern "C"
{
    #include "legato.h"
    #include "interfaces.h"
}

#include <map>
#include <string>
#include <functional>
#include <memory>

namespace ecall
{

// Callbacks delivered on the caller's thread (via le_event_QueueFunctionToThread).
using HoldPowerResultCb    = std::function<void(le_result_t result, int id)>;
using UpdatePowerResultCb  = std::function<void(le_result_t result, int newId)>;
using ReleasePowerResultCb = std::function<void(le_result_t result, int id)>;

// -----------------------------------------------------------------------------
// ApiMngdPm
//
// Thread-safe async wrapper around taf_mngdPm (managed power service).
//
// Design:
//   - A dedicated worker thread owns the taf_mngdPm IPC connection and
//     Legato RunLoop.  All taf_mngdPm_* calls execute on that thread.
//   - Public methods (HoldPowerAsync / UpdateHoldPowerReasonAsync /
//     ReleasePowerAsync) enqueue a request to the worker thread and return
//     immediately (fire-and-forget from the caller's perspective).
//   - Result callbacks are posted back to the CALLER'S thread via
//     le_event_QueueFunctionToThread, so callers never need to worry about
//     thread safety when reading the result.
//
// Wakeup source lifecycle:
//   HoldPowerAsync      -> taf_mngdPm_CreateWakeupSource + StayAwake
//                          -> callback(LE_OK, id)  or  callback(LE_FAULT, -1)
//   UpdateHoldPowerReasonAsync
//                       -> CreateWakeupSource(new) + StayAwake + Delete(old)
//                          -> callback(LE_OK, newId)  or  callback(LE_FAULT, oldId)
//   ReleasePowerAsync   -> taf_mngdPm_DeleteWakeupSource
//                          -> callback(LE_OK, id)  or  callback(LE_NOT_FOUND, id)
//
// Destructor:
//   Sends a StopLoop request to the worker, waits for it to acknowledge,
//   then joins the thread.  Any remaining wakeup sources in wsMap_ are
//   deleted before the thread exits.
// -----------------------------------------------------------------------------
class ApiMngdPm
{
public:
    static ApiMngdPm& GetInstance();

    // Stay-awake reasons understood by taf_mngdPm.
    enum class StayAwakeReason
    {
        None           = -1,
        EcallActive    = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE,
        CallbackActive = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_CALLBACK,
    };

    // Asynchronously acquire a new wakeup source.
    // cb is invoked on the caller's thread with (LE_OK, id) on success
    // or (LE_FAULT, -1) on failure.
    void HoldPowerAsync(StayAwakeReason    reason,
                        const char*        tag,
                        HoldPowerResultCb  cb);

    // Asynchronously replace an existing wakeup source with a new reason.
    // cb is invoked on the caller's thread with (LE_OK, newId) on success.
    void UpdateHoldPowerReasonAsync(int                 oldId,
                                    StayAwakeReason     newReason,
                                    const char*         tag,
                                    UpdatePowerResultCb cb);

    // Asynchronously release a wakeup source.
    // cb is invoked on the caller's thread with (LE_OK, id) on success.
    void ReleasePowerAsync(int                  id,
                           ReleasePowerResultCb cb);

private:
    ApiMngdPm();
    ~ApiMngdPm();

    ApiMngdPm(const ApiMngdPm&)            = delete;
    ApiMngdPm& operator=(const ApiMngdPm&) = delete;

    // ------------------------------------------------------------------
    // Worker-thread request types
    // ------------------------------------------------------------------
    enum class Op { Hold, Update, Release, StopLoop };

    struct HoldReqData
    {
        StayAwakeReason   reason{};
        char              tag[64]{};
        HoldPowerResultCb cb;
        le_thread_Ref_t   callerThread{nullptr};
    };

    struct UpdateReqData
    {
        int                 oldId{-1};
        StayAwakeReason     newReason{};
        char                tag[64]{};
        UpdatePowerResultCb cb;
        le_thread_Ref_t     callerThread{nullptr};
    };

    struct ReleaseReqData
    {
        int                  id{-1};
        ReleasePowerResultCb cb;
        le_thread_Ref_t      callerThread{nullptr};
    };

    struct Req
    {
        Op         op{};
        ApiMngdPm* self{nullptr};
        std::unique_ptr<HoldReqData>    hold;
        std::unique_ptr<UpdateReqData>  update;
        std::unique_ptr<ReleaseReqData> release;
    };

    // Worker thread.
    static void* WorkerThreadFn(void* ctx);
    void         WorkerInitOnThisThread();
    static void  DispatchOp(void* ctx, void* param2);

    // Operations executed on the worker thread.
    void DoHoldPower(std::unique_ptr<HoldReqData>    req);
    void DoUpdateHoldPower(std::unique_ptr<UpdateReqData>  req);
    void DoReleasePower(std::unique_ptr<ReleaseReqData>    req);
    void DoStopLoop();

    // Wakeup-source map helpers (worker thread only, no lock needed).
    taf_mngdPm_wsRef_t TakeWs(int id);
    int                StoreWs(taf_mngdPm_wsRef_t ws);
    void               DeleteAllWs();

    le_thread_Ref_t                   workerThreadRef_{nullptr};
    le_sem_Ref_t                      stopSem_{nullptr};  // posted by StopLoop

    // wsMap_ and nextId_ are accessed ONLY on the worker thread.
    // No mutex needed: all taf_mngdPm_* calls and map mutations happen
    // on the worker thread; callbacks are posted back to caller threads.
    int                               nextId_{1};
    std::map<int, taf_mngdPm_wsRef_t> wsMap_;
};

} // namespace ecall

