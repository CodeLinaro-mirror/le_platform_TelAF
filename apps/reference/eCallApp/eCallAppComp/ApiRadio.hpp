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

// ApiRadio
// --------
// Thread-safe synchronous wrapper around the taf_radio service.
//
// Design:
//   - All taf_radio_* calls execute only on the internal worker thread.
//   - Public methods are synchronous: they queue a request, block on a
//     semaphore, and return the result once the worker has finished.
//   - taf_radio_MetricsRef_t objects are always deleted on the worker thread
//     before the semaphore is posted, so no ref leaks are possible.
//
// Supported queries:
//   - Current RAT in use
//   - Current PLMN (MCC / MNC)
//   - Network short / long name
//   - RAT-specific signal metrics (GSM / UMTS / LTE / NR5G)
class ApiRadio
{
public:
    static ApiRadio& GetInstance();

    // Get current RAT in use.
    le_result_t GetRatInUse(taf_radio_Rat_t* rat, uint8_t phoneId);

    // Get current PLMN (MCC / MNC).
    le_result_t GetCurrentPlmn(char*   mccBuf,
                               size_t  mccBufLen,
                               char*   mncBuf,
                               size_t  mncBufLen,
                               uint8_t phoneId);

    // Get current network short name.
    le_result_t GetCurrentNetworkName(char*   nameBuf,
                                      size_t  nameBufLen,
                                      uint8_t phoneId);

    // Get current network long name.
    le_result_t GetCurrentNetworkLongName(char*   nameBuf,
                                          size_t  nameBufLen,
                                          uint8_t phoneId);

    // Get GSM signal metrics (RSSI in dBm).
    le_result_t GetGsmSignalMetrics(int32_t* rssi, uint8_t phoneId);

    // Get UMTS signal metrics (ss / rscp in dBm).
    le_result_t GetUmtsSignalMetrics(int32_t* ss,
                                     int32_t* rscp,
                                     uint8_t  phoneId);

    // Get LTE signal metrics (ss / rsrq / rsrp / snr).
    le_result_t GetLteSignalMetrics(int32_t* ss,
                                    int32_t* rsrq,
                                    int32_t* rsrp,
                                    int32_t* snr,
                                    uint8_t  phoneId);

    // Get NR5G signal metrics (rsrq / rsrp / snr).
    le_result_t GetNr5gSignalMetrics(int32_t* rsrq,
                                     int32_t* rsrp,
                                     int32_t* snr,
                                     uint8_t  phoneId);

private:
    ApiRadio();
    ~ApiRadio();

    ApiRadio(const ApiRadio&)            = delete;
    ApiRadio& operator=(const ApiRadio&) = delete;

    enum class Op
    {
        GetRatInUse,
        GetCurrentPlmn,
        GetCurrentNetworkName,
        GetCurrentNetworkLongName,
        GetGsmSignalMetrics,
        GetUmtsSignalMetrics,
        GetLteSignalMetrics,
        GetNr5gSignalMetrics,
        StopLoop,
    };

    // All per-request state lives here.  Only the fields relevant to the
    // chosen Op need to be filled in by the caller.
    struct Req
    {
        Op           op{};
        le_result_t  result{LE_FAULT};
        uint8_t      phoneId{0};
        ApiRadio*    self{nullptr};
        le_sem_Ref_t sem{nullptr};

        // GetRatInUse
        taf_radio_Rat_t* ratOut{nullptr};

        // GetCurrentPlmn
        char*  mccBuf{nullptr};
        size_t mccBufLen{0};
        char*  mncBuf{nullptr};
        size_t mncBufLen{0};

        // GetCurrentNetworkName / GetCurrentNetworkLongName
        char*  nameBuf{nullptr};
        size_t nameBufLen{0};

        // GSM
        int32_t* gsmRssiOut{nullptr};

        // UMTS
        int32_t* umtsSsOut{nullptr};
        int32_t* umtsRscpOut{nullptr};

        // LTE
        int32_t* lteSsOut{nullptr};
        int32_t* lteRsrqOut{nullptr};
        int32_t* lteRsrpOut{nullptr};
        int32_t* lteSnrOut{nullptr};

        // NR5G
        int32_t* nrRsrqOut{nullptr};
        int32_t* nrRsrpOut{nullptr};
        int32_t* nrSnrOut{nullptr};
    };

    static void* WorkerThreadFn(void* ctx);
    void         WorkerInitOnThisThread();
    static void  DispatchOp(void* ctx, void* param2);

    // Queue req on the worker thread, block until done, return result.
    // Takes ownership of req and deletes it before returning.
    le_result_t RunOnWorkerSync(Req* req);

    le_thread_Ref_t workerThreadRef_{nullptr};
};

} // namespace ecall

