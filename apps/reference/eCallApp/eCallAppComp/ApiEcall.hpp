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

#include "EcallDefs.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace ecall
{

// -----------------------------------------------------------------------------
// ApiEcall
//
// Thread-safe synchronous wrapper around the taf_ecall service.
//
// Design:
//   - A dedicated worker thread owns the taf_ecall IPC connection, the
//     taf_ecall_CallRef_t, and the Legato RunLoop.
//   - All taf_ecall_* calls execute on that thread.
//   - Public methods are synchronous: they fill a Req, queue it to the worker,
//     block on a semaphore, and return the result.
//   - taf_ecall state-change callbacks are delivered on the worker thread and
//     forwarded to EcallEventBus (phase / PSAP / timer channels).
//
// Destructor:
//   Sends Op::StopLoop, waits for acknowledgement via stopSem_, then joins.
// -----------------------------------------------------------------------------
class ApiEcall
{
public:
    static ApiEcall& GetInstance();

        // eCall control
    le_result_t StartManual();
    le_result_t StartAutomatic();
    le_result_t StartTest();
    le_result_t End();
    le_result_t Answer();

    // MSD / position
    le_result_t UpdateMsdPosition(const MsdPosition& pos);
    le_result_t SendMsd();
    le_result_t GetMsdContent(std::vector<uint8_t>& outMsd);

    // VIN
    le_result_t SetVin(const char* vin);
    // GetVin reads vinCached_ which is written only on the worker thread
    // via SetVin; safe to call from the main thread after SetVin completes.
    le_result_t GetVin(char* outBuf, size_t outLen) const;

    // Configuration
    le_result_t SetHlapTimerConfig(uint16_t ccftTime,
                                   uint16_t minNwRegTime,
                                   uint16_t deRegTime);
    le_result_t SetRedialConfig(uint16_t attempts,
                                uint16_t dialDurationSeconds);
    le_result_t SetMsdVersion(uint32_t version);
    // Pass nullptr or empty string to use USIM numbers.
    le_result_t SetPsapNumber(const char* numberOrNull);
    le_result_t SetVehicleInfo(taf_ecall_MsdVehicleType_t       vehicleType,
                               taf_ecall_PropulsionStorageType_t propulsionType);
    le_result_t QueryConfig();

private:
    ApiEcall();
    ~ApiEcall();

    ApiEcall(const ApiEcall&) = delete;
    ApiEcall& operator=(const ApiEcall&) = delete;

        enum class Op
    {
        StartManual, StartAutomatic, StartTest, End, Answer,
        UpdateMsdPosition, SendMsd, GetMsdContent,
        SetVin,
        SetHlapTimerConfig, SetRedialConfig, SetMsdVersion,
        SetPsapNumber, SetVehicleInfo,
        QueryConfig,
        StopLoop,
    };

    struct Req
    {
        Op           op{};
        ApiEcall*    self{nullptr};
        le_sem_Ref_t sem{nullptr};
        le_result_t  result{LE_FAULT};

                MsdPosition pos{};

        char   vinBuf[64]{};
        size_t vinLen{0};

        uint8_t msdBuf[150]{};
        size_t  msdLen{0};

        uint16_t ccftTime{0};
        uint16_t minNwRegTime{0};
        uint16_t deRegTime{0};

        uint16_t redialAttempts{0};
        uint16_t redialDuration{0};

        uint32_t msdVersion{0};

        bool   psapUseUsim{false};
        char   psapNum[64]{};
        size_t psapLen{0};

        taf_ecall_MsdVehicleType_t       vehicleType{};
        taf_ecall_PropulsionStorageType_t propulsionType{};
    };

        static void* WorkerThreadFn(void* ctx);
    void         WorkerInitOnThisThread();
    static void  DispatchOp(void* ctx, void* param2);

    // Single unified helper: queue req on worker, block, return result.
    // Takes ownership of req and deletes it before returning.
    le_result_t RunOnWorkerSync(Req* req);

    static void EcallStateCb(taf_ecall_CallRef_t ref,
                             taf_ecall_State_t   st,
                             void*               ctx);
    void HandleState(taf_ecall_State_t st);

    le_thread_Ref_t                   workerThreadRef_{nullptr};
    le_sem_Ref_t                      stopSem_{nullptr};
    taf_ecall_CallRef_t               callRef_{nullptr};
    taf_ecall_StateChangeHandlerRef_t handlerRef_{nullptr};

    // VIN cache: written only on the worker thread (SetVin op),
    // read only from GetVin (main thread, after SetVin completes).
    char vinCached_[64]{};
};

} // namespace ecall
