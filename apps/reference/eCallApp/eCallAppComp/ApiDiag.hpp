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

#include <cstdint>
#include <cstddef>

namespace ecall
{

// -----------------------------------------------------------------------------
// ApiDiag
//
// Thread-safe synchronous wrapper around the taf_diagEvent / taf_diagDTC /
// taf_diag services.
//
// Design:
//   - A dedicated worker thread owns all three IPC connections and the
//     Legato RunLoop.  All taf_diag*_* calls execute on that thread.
//   - Public methods are synchronous: they queue a Req, block on a semaphore,
//     and return the result once the worker has finished.
//
// Destructor:
//   Sends Op::StopLoop to the worker, waits for acknowledgement via a
//   dedicated semaphore, then joins the thread.
// -----------------------------------------------------------------------------
class ApiDiag
{
public:
    static ApiDiag& GetInstance();

    le_result_t GetEventService(uint16_t                    eventId,
                                taf_diagEvent_ServiceRef_t* out);

    le_result_t GetDtcService(uint32_t                  dtcCode,
                              taf_diagDTC_ServiceRef_t* out);

    le_result_t GetOpCycle(uint8_t                     opCycleId,
                           taf_diagEvent_OpCycleRef_t* out);

    le_result_t SetOpCycleState(taf_diagEvent_OpCycleRef_t          opCycleRef,
                                taf_diagEvent_OperationCycleState_t state);

    le_result_t SetEventStatus(taf_diagEvent_ServiceRef_t  evSvc,
                               taf_diagEvent_StatusType_t  status);

    le_result_t ReadDtcStatus(taf_diagDTC_ServiceRef_t dtcSvc,
                              uint8_t*                 dtcStatus);

    le_result_t GetDtcCode(taf_diagDTC_ServiceRef_t dtcSvc,
                           uint32_t*               dtcCode);

    le_result_t SetEnableCondition(uint8_t enableConditionID,
                                   bool    conditionFulfilled);

private:
    ApiDiag();
    ~ApiDiag();

    ApiDiag(const ApiDiag&)            = delete;
    ApiDiag& operator=(const ApiDiag&) = delete;

    enum class Op
    {
        GetEventService,
        GetDtcService,
        GetOpCycle,
        SetOpCycleState,
        SetEventStatus,
        ReadDtcStatus,
        GetDtcCode,
        SetEnableCondition,
        StopLoop,
    };

    struct Req
    {
        Op          op{};
        le_result_t result{LE_FAULT};
        ApiDiag*    self{nullptr};
        le_sem_Ref_t sem{nullptr};

        // Inputs
        uint16_t eventId{0};
        uint32_t dtcCode{0};
        uint8_t  opCycleId{0};
        taf_diagEvent_OperationCycleState_t opCycleState{};
        taf_diagEvent_StatusType_t          eventStatus{};
        uint8_t  enableConditionID{0};
        bool     conditionFulfilled{false};

        // Handle inputs
        taf_diagEvent_ServiceRef_t  evSvcIn{nullptr};
        taf_diagDTC_ServiceRef_t    dtcSvcIn{nullptr};
        taf_diagEvent_OpCycleRef_t  opCycleIn{nullptr};

        // Outputs
        taf_diagEvent_ServiceRef_t* evSvcOut{nullptr};
        taf_diagDTC_ServiceRef_t*   dtcSvcOut{nullptr};
        taf_diagEvent_OpCycleRef_t* opCycleOut{nullptr};
        uint8_t*                    dtcStatusOut{nullptr};
        uint32_t*                   dtcCodeOut{nullptr};
    };

    static void* WorkerThreadFn(void* ctx);
    void         WorkerInitOnThisThread();
    static void  DispatchOp(void* ctx, void* param2);

    // Queue req on the worker thread, block until done, return result.
    // Takes ownership of req and deletes it before returning.
    le_result_t RunOnWorkerSync(Req* req);

    le_thread_Ref_t workerThreadRef_{nullptr};
    le_sem_Ref_t    stopSem_{nullptr};
};

} // namespace ecall

