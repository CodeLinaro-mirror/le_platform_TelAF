/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiRadio.hpp"

#include <new>
#include <cstring>

namespace ecall
{

#define LOG_NAME "ApiRadio"

// =============================================================================
// Singleton
// =============================================================================
ApiRadio& ApiRadio::GetInstance()
{
    static ApiRadio inst;
    return inst;
}

// =============================================================================
// Ctor / Dtor
// =============================================================================
ApiRadio::ApiRadio()
{
    workerThreadRef_ = le_thread_Create("ApiRadioWorker",
                                        &ApiRadio::WorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_Start(workerThreadRef_);
    LE_INFO("[%s] worker thread started", LOG_NAME);
}

ApiRadio::~ApiRadio()
{
    // Send StopLoop and wait for the worker to acknowledge before joining.
    // RunOnWorkerSync owns and deletes the Req.
    Req* req = new (std::nothrow) Req;
    if (req)
    {
        req->op = Op::StopLoop;
        (void)RunOnWorkerSync(req);   // blocks until worker posts sem
    }

    if (workerThreadRef_)
    {
        le_thread_Join(workerThreadRef_, nullptr);
        workerThreadRef_ = nullptr;
    }
}

// =============================================================================
// Worker thread
// =============================================================================
void* ApiRadio::WorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiRadio*>(ctx);
    if (!self) return nullptr;

    self->WorkerInitOnThisThread();
    LE_INFO("[%s] entering RunLoop", LOG_NAME);
    le_event_RunLoop();
    LE_INFO("[%s] RunLoop exited", LOG_NAME);
    return nullptr;
}

void ApiRadio::WorkerInitOnThisThread()
{
    taf_radio_ConnectService();
    LE_INFO("[%s] connected to taf_radio", LOG_NAME);
}

// =============================================================================
// DispatchOp  (runs on worker thread)
// =============================================================================
void ApiRadio::DispatchOp(void* ctx, void* /*param2*/)
{
    Req* req = static_cast<Req*>(ctx);
    if (!req || !req->self)
        return;

    le_result_t r = LE_FAULT;

    switch (req->op)
    {
        // -----------------------------------------------------------------
        case Op::GetRatInUse:
        {
            if (!req->ratOut) { r = LE_BAD_PARAMETER; break; }

            taf_radio_Rat_t rat = TAF_RADIO_RAT_UNKNOWN;
            r = taf_radio_GetRadioAccessTechInUse(&rat, req->phoneId);
            if (r == LE_OK)
            {
                *req->ratOut = rat;
                LE_DEBUG("[%s] GetRatInUse phoneId=%u rat=%d",
                         LOG_NAME, req->phoneId, static_cast<int>(rat));
            }
            else
            {
                LE_ERROR("[%s] GetRatInUse failed rc=%d phoneId=%u",
                         LOG_NAME, r, req->phoneId);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetCurrentPlmn:
        {
            if (!req->mccBuf || req->mccBufLen < TAF_RADIO_MCC_BYTES ||
                !req->mncBuf || req->mncBufLen < TAF_RADIO_MNC_BYTES)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            char mcc[TAF_RADIO_MCC_BYTES] = {};
            char mnc[TAF_RADIO_MNC_BYTES] = {};

            r = taf_radio_GetCurrentNetworkMccMnc(
                    mcc, TAF_RADIO_MCC_BYTES,
                    mnc, TAF_RADIO_MNC_BYTES,
                    req->phoneId);
            if (r == LE_OK)
            {
                snprintf(req->mccBuf, req->mccBufLen, "%s", mcc);
                req->mccBuf[req->mccBufLen - 1] = '\0';
                snprintf(req->mncBuf, req->mncBufLen, "%s", mnc);
                req->mncBuf[req->mncBufLen - 1] = '\0';
                LE_DEBUG("[%s] GetCurrentPlmn mcc=%s mnc=%s phoneId=%u",
                         LOG_NAME, mcc, mnc, req->phoneId);
            }
            else
            {
                LE_ERROR("[%s] GetCurrentNetworkMccMnc failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetCurrentNetworkName:
        {
            if (!req->nameBuf || req->nameBufLen == 0)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {};
            r = taf_radio_GetCurrentNetworkName(
                    name, TAF_RADIO_NETWORK_NAME_MAX_LEN, req->phoneId);
            if (r == LE_OK)
            {
                snprintf(req->nameBuf, req->nameBufLen, "%s", name);
                req->nameBuf[req->nameBufLen - 1] = '\0';
                LE_DEBUG("[%s] GetCurrentNetworkName name=%s", LOG_NAME, name);
            }
            else
            {
                LE_ERROR("[%s] GetCurrentNetworkName failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetCurrentNetworkLongName:
        {
            if (!req->nameBuf || req->nameBufLen == 0)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {};
            r = taf_radio_GetCurrentNetworkLongName(
                    name, TAF_RADIO_NETWORK_NAME_MAX_LEN, req->phoneId);
            if (r == LE_OK)
            {
                snprintf(req->nameBuf, req->nameBufLen, "%s", name);
                req->nameBuf[req->nameBufLen - 1] = '\0';
                LE_DEBUG("[%s] GetCurrentNetworkLongName name=%s", LOG_NAME, name);
            }
            else
            {
                LE_ERROR("[%s] GetCurrentNetworkLongName failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        // Signal metrics helpers
        //
        // Pattern for all RATs:
        //   1. MeasureSignalMetrics  -> metricsRef (must always be deleted)
        //   2. GetRatOfSignalMetrics -> check RAT bit
        //   3. Get<Rat>SignalMetrics -> read values
        //   4. DeleteSignalMetrics   -> release ref
        //
        // DeleteSignalMetrics is called on every exit path to prevent leaks.
        // -----------------------------------------------------------------
        case Op::GetGsmSignalMetrics:
        {
            if (!req->gsmRssiOut) { r = LE_BAD_PARAMETER; break; }

            taf_radio_MetricsRef_t metrics =
                taf_radio_MeasureSignalMetrics(req->phoneId);
            if (!metrics)
            {
                LE_ERROR("[%s] MeasureSignalMetrics(GSM) returned NULL", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            taf_radio_RatBitMask_t ratMask =
                taf_radio_GetRatOfSignalMetrics(metrics);
            if (!(ratMask & TAF_RADIO_RAT_BIT_MASK_GSM))
            {
                LE_DEBUG("[%s] GSM not in use (ratMask=0x%X)", LOG_NAME,
                         static_cast<unsigned>(ratMask));
                taf_radio_DeleteSignalMetrics(metrics);
                r = LE_UNAVAILABLE;
                break;
            }

            int32_t  rssi = 0;
            uint32_t ber  = 0;
            r = taf_radio_GetGsmSignalMetrics(metrics, &rssi, &ber);
            taf_radio_DeleteSignalMetrics(metrics);   // always release

            if (r == LE_OK)
            {
                *req->gsmRssiOut = rssi;
                LE_DEBUG("[%s] GSM rssi=%d ber=%u", LOG_NAME, rssi, ber);
            }
            else
            {
                LE_ERROR("[%s] GetGsmSignalMetrics failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetUmtsSignalMetrics:
        {
            if (!req->umtsSsOut || !req->umtsRscpOut)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            taf_radio_MetricsRef_t metrics =
                taf_radio_MeasureSignalMetrics(req->phoneId);
            if (!metrics)
            {
                LE_ERROR("[%s] MeasureSignalMetrics(UMTS) returned NULL", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            taf_radio_RatBitMask_t ratMask =
                taf_radio_GetRatOfSignalMetrics(metrics);
            if (!(ratMask & TAF_RADIO_RAT_BIT_MASK_UMTS))
            {
                LE_DEBUG("[%s] UMTS not in use (ratMask=0x%X)", LOG_NAME,
                         static_cast<unsigned>(ratMask));
                taf_radio_DeleteSignalMetrics(metrics);
                r = LE_UNAVAILABLE;
                break;
            }

            int32_t  ss   = 0;
            uint32_t ber  = 0;
            int32_t  rscp = 0;
            r = taf_radio_GetUmtsSignalMetrics(metrics, &ss, &ber, &rscp);
            taf_radio_DeleteSignalMetrics(metrics);   // always release

            if (r == LE_OK)
            {
                *req->umtsSsOut   = ss;
                *req->umtsRscpOut = rscp;
                LE_DEBUG("[%s] UMTS ss=%d rscp=%d ber=%u", LOG_NAME, ss, rscp, ber);
            }
            else
            {
                LE_ERROR("[%s] GetUmtsSignalMetrics failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetLteSignalMetrics:
        {
            if (!req->lteSsOut || !req->lteRsrqOut ||
                !req->lteRsrpOut || !req->lteSnrOut)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            taf_radio_MetricsRef_t metrics =
                taf_radio_MeasureSignalMetrics(req->phoneId);
            if (!metrics)
            {
                LE_ERROR("[%s] MeasureSignalMetrics(LTE) returned NULL", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            taf_radio_RatBitMask_t ratMask =
                taf_radio_GetRatOfSignalMetrics(metrics);
            if (!(ratMask & TAF_RADIO_RAT_BIT_MASK_LTE))
            {
                LE_DEBUG("[%s] LTE not in use (ratMask=0x%X)", LOG_NAME,
                         static_cast<unsigned>(ratMask));
                taf_radio_DeleteSignalMetrics(metrics);
                r = LE_UNAVAILABLE;
                break;
            }

            int32_t ss   = 0;
            int32_t rsrq = 0;
            int32_t rsrp = 0;
            int32_t snr  = 0;
            r = taf_radio_GetLteSignalMetrics(metrics, &ss, &rsrq, &rsrp, &snr);
            taf_radio_DeleteSignalMetrics(metrics);   // always release

            if (r == LE_OK)
            {
                *req->lteSsOut   = ss;
                *req->lteRsrqOut = rsrq;
                *req->lteRsrpOut = rsrp;
                *req->lteSnrOut  = snr;
                LE_DEBUG("[%s] LTE ss=%d rsrq=%d rsrp=%d snr=%d",
                         LOG_NAME, ss, rsrq, rsrp, snr);
            }
            else
            {
                LE_ERROR("[%s] GetLteSignalMetrics failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        case Op::GetNr5gSignalMetrics:
        {
            if (!req->nrRsrqOut || !req->nrRsrpOut || !req->nrSnrOut)
            {
                r = LE_BAD_PARAMETER;
                break;
            }

            taf_radio_MetricsRef_t metrics =
                taf_radio_MeasureSignalMetrics(req->phoneId);
            if (!metrics)
            {
                LE_ERROR("[%s] MeasureSignalMetrics(NR5G) returned NULL", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            taf_radio_RatBitMask_t ratMask =
                taf_radio_GetRatOfSignalMetrics(metrics);
            if (!(ratMask & TAF_RADIO_RAT_BIT_MASK_NR5G))
            {
                LE_DEBUG("[%s] NR5G not in use (ratMask=0x%X)", LOG_NAME,
                         static_cast<unsigned>(ratMask));
                taf_radio_DeleteSignalMetrics(metrics);
                r = LE_UNAVAILABLE;
                break;
            }

            int32_t rsrq = 0;
            int32_t rsrp = 0;
            int32_t snr  = 0;
            r = taf_radio_GetNr5gSignalMetrics(metrics, &rsrq, &rsrp, &snr);
            taf_radio_DeleteSignalMetrics(metrics);   // always release

            if (r == LE_OK)
            {
                *req->nrRsrqOut = rsrq;
                *req->nrRsrpOut = rsrp;
                *req->nrSnrOut  = snr;
                LE_DEBUG("[%s] NR5G rsrq=%d rsrp=%d snr=%d",
                         LOG_NAME, rsrq, rsrp, snr);
            }
            else
            {
                LE_ERROR("[%s] GetNr5gSignalMetrics failed rc=%d", LOG_NAME, r);
            }
            break;
        }

        // -----------------------------------------------------------------
        // StopLoop: post the semaphore so the caller unblocks, then ask
        // the Legato event loop to exit so the thread can be joined.
        // -----------------------------------------------------------------
        case Op::StopLoop:
        {
            LE_INFO("[%s] StopLoop: exiting RunLoop", LOG_NAME);
            r = LE_OK;
            req->result = r;
                        if (req->sem)
                le_sem_Post(req->sem);
            // Worker thread exits RunLoop naturally after all sources removed.
            return;   // skip the generic sem-post at the end
        }

        default:
            LE_ERROR("[%s] Unknown op=%d", LOG_NAME, static_cast<int>(req->op));
            r = LE_FAULT;
            break;
    }

    req->result = r;
    if (req->sem)
        le_sem_Post(req->sem);
}

// =============================================================================
// RunOnWorkerSync
// =============================================================================
le_result_t ApiRadio::RunOnWorkerSync(Req* req)
{
    if (!req)
        return LE_BAD_PARAMETER;

    req->self = this;
    req->sem  = le_sem_Create("ApiRadioReqSem", 0);
    if (!req->sem)
    {
        delete req;
        return LE_FAULT;
    }

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiRadio::DispatchOp,
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
le_result_t ApiRadio::GetRatInUse(taf_radio_Rat_t* rat, uint8_t phoneId)
{
    if (!rat) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op      = Op::GetRatInUse;
    req->phoneId = phoneId;
    req->ratOut  = rat;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetCurrentPlmn(char*   mccBuf,
                                     size_t  mccBufLen,
                                     char*   mncBuf,
                                     size_t  mncBufLen,
                                     uint8_t phoneId)
{
    if (!mccBuf || !mncBuf) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op        = Op::GetCurrentPlmn;
    req->phoneId   = phoneId;
    req->mccBuf    = mccBuf;
    req->mccBufLen = mccBufLen;
    req->mncBuf    = mncBuf;
    req->mncBufLen = mncBufLen;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetCurrentNetworkName(char*   nameBuf,
                                            size_t  nameBufLen,
                                            uint8_t phoneId)
{
    if (!nameBuf || nameBufLen == 0) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op         = Op::GetCurrentNetworkName;
    req->phoneId    = phoneId;
    req->nameBuf    = nameBuf;
    req->nameBufLen = nameBufLen;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetCurrentNetworkLongName(char*   nameBuf,
                                                size_t  nameBufLen,
                                                uint8_t phoneId)
{
    if (!nameBuf || nameBufLen == 0) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op         = Op::GetCurrentNetworkLongName;
    req->phoneId    = phoneId;
    req->nameBuf    = nameBuf;
    req->nameBufLen = nameBufLen;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetGsmSignalMetrics(int32_t* rssi, uint8_t phoneId)
{
    if (!rssi) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op         = Op::GetGsmSignalMetrics;
    req->phoneId    = phoneId;
    req->gsmRssiOut = rssi;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetUmtsSignalMetrics(int32_t* ss,
                                           int32_t* rscp,
                                           uint8_t  phoneId)
{
    if (!ss || !rscp) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op          = Op::GetUmtsSignalMetrics;
    req->phoneId     = phoneId;
    req->umtsSsOut   = ss;
    req->umtsRscpOut = rscp;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetLteSignalMetrics(int32_t* ss,
                                          int32_t* rsrq,
                                          int32_t* rsrp,
                                          int32_t* snr,
                                          uint8_t  phoneId)
{
    if (!ss || !rsrq || !rsrp || !snr) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op         = Op::GetLteSignalMetrics;
    req->phoneId    = phoneId;
    req->lteSsOut   = ss;
    req->lteRsrqOut = rsrq;
    req->lteRsrpOut = rsrp;
    req->lteSnrOut  = snr;
    return RunOnWorkerSync(req);
}

le_result_t ApiRadio::GetNr5gSignalMetrics(int32_t* rsrq,
                                           int32_t* rsrp,
                                           int32_t* snr,
                                           uint8_t  phoneId)
{
    if (!rsrq || !rsrp || !snr) return LE_BAD_PARAMETER;

    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op        = Op::GetNr5gSignalMetrics;
    req->phoneId   = phoneId;
    req->nrRsrqOut = rsrq;
    req->nrRsrpOut = rsrp;
    req->nrSnrOut  = snr;
    return RunOnWorkerSync(req);
}

} // namespace ecall
