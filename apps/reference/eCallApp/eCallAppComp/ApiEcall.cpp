/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiEcall.hpp"
#include "EcallEventBus.hpp"
#include "EcallDefs.hpp"

#include <algorithm>
#include <new>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

namespace ecall
{

#define LOG_NAME "ApiEcall"

// =============================================================================
// Singleton
// =============================================================================
ApiEcall& ApiEcall::GetInstance()
{
    static ApiEcall inst;
    return inst;
}

// =============================================================================
// Ctor / Dtor
// =============================================================================
ApiEcall::ApiEcall()
{
    stopSem_ = le_sem_Create("ApiEcallStopSem", 0);
    LE_ASSERT(stopSem_ != nullptr);

    workerThreadRef_ = le_thread_Create("ApiEcallWorker",
                                        &ApiEcall::WorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_Start(workerThreadRef_);
    LE_INFO("[%s] worker thread started", LOG_NAME);
}

ApiEcall::~ApiEcall()
{
    Req* req = new (std::nothrow) Req;
    if (req)
    {
        req->op   = Op::StopLoop;
        req->self = this;
        req->sem  = le_sem_Create("ApiEcallStopReqSem", 0);
        if (req->sem)
        {
            le_event_QueueFunctionToThread(workerThreadRef_,
                                           &ApiEcall::DispatchOp,
                                           req, nullptr);
            le_sem_Wait(stopSem_);
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
void* ApiEcall::WorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiEcall*>(ctx);
    if (!self) return nullptr;
    self->WorkerInitOnThisThread();
    LE_INFO("[%s] entering RunLoop", LOG_NAME);
    le_event_RunLoop();
    LE_INFO("[%s] RunLoop exited", LOG_NAME);
    return nullptr;
}

void ApiEcall::WorkerInitOnThisThread()
{
    taf_ecall_ConnectService();
    callRef_ = taf_ecall_Create();
    LE_ASSERT(callRef_ != nullptr);
    handlerRef_ = taf_ecall_AddStateChangeHandler(&ApiEcall::EcallStateCb, this);
    LE_INFO("[%s] taf_ecall connected, callRef=%p", LOG_NAME, callRef_);
}

// =============================================================================
// DispatchOp  (worker thread)
// =============================================================================
void ApiEcall::DispatchOp(void* ctx, void* /*param2*/)
{
    Req* req = static_cast<Req*>(ctx);
    if (!req || !req->self) return;
    auto* self = req->self;

    le_result_t r = LE_OK;

    switch (req->op)
    {
        case Op::StartManual:
            LE_INFO("[%s] StartManual", LOG_NAME);
            r = taf_ecall_StartManual(self->callRef_);
            break;

        case Op::StartAutomatic:
            LE_INFO("[%s] StartAutomatic", LOG_NAME);
            r = taf_ecall_StartAutomatic(self->callRef_);
            break;

        case Op::StartTest:
            LE_INFO("[%s] StartTest", LOG_NAME);
            r = taf_ecall_StartTest(self->callRef_);
            break;

        case Op::End:
            LE_INFO("[%s] End", LOG_NAME);
            r = taf_ecall_End(self->callRef_);
            break;

        case Op::Answer:
            LE_INFO("[%s] Answer", LOG_NAME);
            r = taf_ecall_Answer(self->callRef_);
            break;

        case Op::UpdateMsdPosition:
        {
            const MsdPosition& pos = req->pos;
            LE_DEBUG("[%s] UpdateMsdPosition trusted=%d lat=%d lon=%d dir=%d",
                     LOG_NAME, pos.isTrusted ? 1 : 0,
                     pos.latitudeMas, pos.longitudeMas, pos.direction);

            r = taf_ecall_SetMsdPosition(self->callRef_,
                                         pos.isTrusted,
                                         pos.latitudeMas,
                                         pos.longitudeMas,
                                         pos.direction);
            if (r != LE_OK) break;

            if (pos.hasN1)
            {
                r = taf_ecall_SetMsdPositionN1(self->callRef_,
                                               pos.latDeltaN1,
                                               pos.lonDeltaN1);
                LE_DEBUG("[%s] SetMsdPositionN1 rc=%d latΔ=%d lonΔ=%d",
                         LOG_NAME, r, pos.latDeltaN1, pos.lonDeltaN1);
            }
            if (r != LE_OK) break;

            if (pos.hasN2)
            {
                r = taf_ecall_SetMsdPositionN2(self->callRef_,
                                               pos.latDeltaN2,
                                               pos.lonDeltaN2);
                LE_DEBUG("[%s] SetMsdPositionN2 rc=%d latΔ=%d lonΔ=%d",
                         LOG_NAME, r, pos.latDeltaN2, pos.lonDeltaN2);
            }
            break;
        }

        case Op::SendMsd:
            LE_INFO("[%s] SendMsd", LOG_NAME);
            r = taf_ecall_SendMsd(self->callRef_);
            break;

        case Op::GetMsdContent:
            LE_INFO("[%s] GetMsdContent", LOG_NAME);
            req->msdLen = sizeof(req->msdBuf);
            r = taf_ecall_ExportMsd(self->callRef_, req->msdBuf, &req->msdLen);
            break;

        case Op::SetVin:
        {
            // Validate: trim, uppercase, check length=17, check charset.
            std::string vin(req->vinBuf, req->vinLen);
            vin.erase(0, vin.find_first_not_of(" \t\r\n"));
            vin.erase(vin.find_last_not_of(" \t\r\n") + 1);
            std::transform(vin.begin(), vin.end(), vin.begin(),
                           [](unsigned char c){ return std::toupper(c); });

            if (vin.size() != 17)
            {
                LE_ERROR("[%s] SetVin: invalid length=%zu (expect 17)",
                         LOG_NAME, vin.size());
                r = LE_BAD_PARAMETER;
                break;
            }

            auto isValidVinChar = [](char c)
            {
                if (c >= '0' && c <= '9') return true;
                if (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q')
                    return true;
                return false;
            };
            if (!std::all_of(vin.begin(), vin.end(), isValidVinChar))
            {
                LE_ERROR("[%s] SetVin: invalid characters", LOG_NAME);
                r = LE_BAD_PARAMETER;
                break;
            }

            r = taf_ecall_SetVIN(vin.c_str());
            if (r != LE_OK)
            {
                LE_ERROR("[%s] taf_ecall_SetVIN failed rc=%d", LOG_NAME, r);
                break;
            }

            std::memset(self->vinCached_, 0, sizeof(self->vinCached_));
            std::memcpy(self->vinCached_, vin.c_str(),
                        std::min(vin.size(), sizeof(self->vinCached_) - 1));
            LE_INFO("[%s] VIN set to '%s'", LOG_NAME, self->vinCached_);
            break;
        }

        case Op::SetHlapTimerConfig:
        {
            LE_INFO("[%s] SetHlapTimerConfig ccft=%u minNwReg=%u deReg=%u",
                    LOG_NAME, req->ccftTime, req->minNwRegTime, req->deRegTime);

            r = taf_ecall_SetNadClearDownFallbackTime(req->ccftTime);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetNadClearDownFallbackTime failed rc=%d", LOG_NAME, r);
                break;
            }
            r = taf_ecall_SetNadMinNetworkRegistrationTime(req->minNwRegTime);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetNadMinNetworkRegistrationTime failed rc=%d", LOG_NAME, r);
                break;
            }
            r = taf_ecall_SetNadDeregistrationTime(req->deRegTime);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetNadDeregistrationTime failed rc=%d", LOG_NAME, r);
                break;
            }
            LE_INFO("[%s] HLAP timer config applied", LOG_NAME);
            break;
        }

        case Op::SetRedialConfig:
        {
            LE_INFO("[%s] SetRedialConfig attempts=%u duration=%u",
                    LOG_NAME, req->redialAttempts, req->redialDuration);

            r = taf_ecall_SetInitialDialAttempts(req->redialAttempts);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetInitialDialAttempts failed rc=%d", LOG_NAME, r);
                break;
            }
            uint16_t interval[1] = {req->redialDuration};
            r = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(interval, 1);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetInitialDialInterval failed rc=%d", LOG_NAME, r);
                break;
            }
            LE_INFO("[%s] redial config applied", LOG_NAME);
            break;
        }

        case Op::SetMsdVersion:
        {
            const uint32_t v = req->msdVersion;
            if (v != 2 && v != 3)
            {
                LE_ERROR("[%s] SetMsdVersion: invalid version=%u", LOG_NAME, v);
                r = LE_BAD_PARAMETER;
                break;
            }
            r = taf_ecall_SetMsdVersion(v);
            if (r != LE_OK)
                LE_ERROR("[%s] taf_ecall_SetMsdVersion(%u) failed rc=%d", LOG_NAME, v, r);
            else
                LE_INFO("[%s] MSD version set to %u", LOG_NAME, v);
            break;
        }

        case Op::SetPsapNumber:
        {
            if (req->psapUseUsim || req->psapLen == 0)
            {
                r = taf_ecall_UseUSimNumbers();
                if (r != LE_OK)
                    LE_ERROR("[%s] UseUSimNumbers failed rc=%d", LOG_NAME, r);
                else
                    LE_INFO("[%s] PSAP source: USIM", LOG_NAME);
            }
            else
            {
                req->psapNum[sizeof(req->psapNum) - 1] = '\0';
                r = taf_ecall_SetPsapNumber(req->psapNum);
                if (r != LE_OK)
                    LE_ERROR("[%s] SetPsapNumber('%s') failed rc=%d",
                             LOG_NAME, req->psapNum, r);
                else
                    LE_INFO("[%s] PSAP set to '%s'", LOG_NAME, req->psapNum);
            }
            break;
        }

        case Op::SetVehicleInfo:
        {
            r = taf_ecall_SetVehicleType(req->vehicleType);
            if (r != LE_OK)
            {
                LE_ERROR("[%s] SetVehicleType failed rc=%d", LOG_NAME, r);
                break;
            }
            r = taf_ecall_SetPropulsionType(req->propulsionType);
            if (r != LE_OK)
                LE_ERROR("[%s] SetPropulsionType failed rc=%d", LOG_NAME, r);
            else
                LE_INFO("[%s] VehicleInfo applied type=%d propulsion=0x%X",
                        LOG_NAME,
                        static_cast<int>(req->vehicleType),
                        static_cast<unsigned>(req->propulsionType));
            break;
        }

        case Op::QueryConfig:
        {
            taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
            uint32_t msdVer = 0;
            char psapNum[64] = {};
            size_t n = sizeof(psapNum);

            if (taf_ecall_GetConfiguredOperationMode(1, &opMode) == LE_OK)
                LE_INFO("[%s] OPMODE=%d", LOG_NAME, static_cast<int>(opMode));
            if (taf_ecall_GetMsdVersion(&msdVer) == LE_OK)
                LE_INFO("[%s] MSD_VERSION=%u", LOG_NAME, msdVer);
            if (taf_ecall_GetPsapNumber(psapNum, n) == LE_OK)
                LE_INFO("[%s] PSAP=%s", LOG_NAME, psapNum);
            r = LE_OK;
            break;
        }

        case Op::StopLoop:
            LE_INFO("[%s] StopLoop: removing handler and exiting RunLoop", LOG_NAME);
            if (self->handlerRef_)
            {
                taf_ecall_RemoveStateChangeHandler(self->handlerRef_);
                self->handlerRef_ = nullptr;
            }
            r = LE_OK;
            req->result = r;
            if (req->sem) le_sem_Post(req->sem);
            le_sem_Post(self->stopSem_);
            return;   // RunLoop will exit when worker thread returns

        default:
            LE_ERROR("[%s] Unknown op=%d", LOG_NAME, static_cast<int>(req->op));
            r = LE_FAULT;
            break;
    }

    req->result = r;
    if (req->sem) le_sem_Post(req->sem);
}

// =============================================================================
// RunOnWorkerSync  (single unified helper)
// =============================================================================
le_result_t ApiEcall::RunOnWorkerSync(Req* req)
{
    if (!req) return LE_BAD_PARAMETER;

    req->self = this;
    req->sem  = le_sem_Create("ApiEcallReqSem", 0);
    if (!req->sem)
    {
        delete req;
        return LE_FAULT;
    }

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiEcall::DispatchOp, req, nullptr);
    le_sem_Wait(req->sem);
    const le_result_t rc = req->result;
    le_sem_Delete(req->sem);
    delete req;
    return rc;
}

// =============================================================================
// Public API
// =============================================================================

le_result_t ApiEcall::StartManual()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::StartManual;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::StartAutomatic()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::StartAutomatic;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::StartTest()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::StartTest;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::End()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::End;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::Answer()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::Answer;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::UpdateMsdPosition(const MsdPosition& pos)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op  = Op::UpdateMsdPosition;
    req->pos = pos;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::SendMsd()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::SendMsd;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::GetMsdContent(std::vector<uint8_t>& outMsd)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::GetMsdContent;

    // RunOnWorkerSync deletes req, so capture msdBuf/msdLen before that.
    // We need to read them after Wait but before delete — use a local copy.
    // Solution: allocate on stack, copy out after Wait.
    // RunOnWorkerSync owns req; we need the msd data before it's deleted.
    // Use a wrapper that reads the data out first.
    req->self = this;
    req->sem  = le_sem_Create("ApiEcallReqSem", 0);
    if (!req->sem) { delete req; return LE_FAULT; }

    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiEcall::DispatchOp, req, nullptr);
    le_sem_Wait(req->sem);
    const le_result_t rc = req->result;
    if (rc == LE_OK)
        outMsd.assign(req->msdBuf, req->msdBuf + req->msdLen);
    le_sem_Delete(req->sem);
    delete req;
    return rc;
}

le_result_t ApiEcall::SetVin(const char* vin)
{
    if (!vin) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op     = Op::SetVin;
    req->vinLen = std::min(std::strlen(vin), sizeof(req->vinBuf) - 1);
    std::memcpy(req->vinBuf, vin, req->vinLen);
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::GetVin(char* outBuf, size_t outLen) const
{
    if (!outBuf || outLen == 0) return LE_BAD_PARAMETER;
    const size_t n = sstrnlen(vinCached_, sizeof(vinCached_));
    if (n == 0) return LE_NOT_FOUND;
    const size_t copyLen = std::min(n, outLen - 1);
    std::memset(outBuf, 0, outLen);
    std::memcpy(outBuf, vinCached_, copyLen);
    return LE_OK;
}

le_result_t ApiEcall::SetHlapTimerConfig(uint16_t ccftTime,
                                         uint16_t minNwRegTime,
                                         uint16_t deRegTime)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op           = Op::SetHlapTimerConfig;
    req->ccftTime     = ccftTime;
    req->minNwRegTime = minNwRegTime;
    req->deRegTime    = deRegTime;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::SetRedialConfig(uint16_t attempts,
                                      uint16_t dialDurationSeconds)
{
    if (attempts == 0 || dialDurationSeconds == 0) return LE_BAD_PARAMETER;
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op             = Op::SetRedialConfig;
    req->redialAttempts = attempts;
    req->redialDuration = dialDurationSeconds;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::SetMsdVersion(uint32_t version)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op         = Op::SetMsdVersion;
    req->msdVersion = version;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::SetPsapNumber(const char* numberOrNull)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::SetPsapNumber;
    if (numberOrNull && numberOrNull[0] != '\0')
    {
        req->psapUseUsim = false;
        req->psapLen = std::min(std::strlen(numberOrNull),
                                sizeof(req->psapNum) - 1);
        std::memcpy(req->psapNum, numberOrNull, req->psapLen);
    }
    else
    {
        req->psapUseUsim = true;
    }
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::SetVehicleInfo(taf_ecall_MsdVehicleType_t       vehicleType,
                                     taf_ecall_PropulsionStorageType_t propulsionType)
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op             = Op::SetVehicleInfo;
    req->vehicleType    = vehicleType;
    req->propulsionType = propulsionType;
    return RunOnWorkerSync(req);
}

le_result_t ApiEcall::QueryConfig()
{
    Req* req = new (std::nothrow) Req; if (!req) return LE_NO_MEMORY;
    req->op = Op::QueryConfig;
    return RunOnWorkerSync(req);
}

// =============================================================================
// State handling & Bus integration
// =============================================================================
void ApiEcall::HandleState(taf_ecall_State_t st)
{
    auto& bus = EcallEventBus::GetInstance();
    const uint64_t ts = ecall::NowUtcMs();

    switch (st)
    {
        // Phase events
        case TAF_ECALL_STATE_DIALING:
            bus.PublishPhase({EcPhase::Dialing, ts}); break;
        case TAF_ECALL_STATE_ALERTING:
            bus.PublishPhase({EcPhase::Alerting, ts}); break;
        case TAF_ECALL_STATE_ACTIVE:
            bus.PublishPhase({EcPhase::Active, ts}); break;
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED:
            bus.PublishPhase({EcPhase::OutbandMsdStart, ts}); break;
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS:
            bus.PublishPhase({EcPhase::OutbandMsdSuccess, ts}); break;
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE:
            bus.PublishPhase({EcPhase::OutbandMsdFail, ts}); break;
        case TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED:
            bus.PublishPhase({EcPhase::MsdStart, ts}); break;
        case TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS:
            bus.PublishPhase({EcPhase::MsdSuccess, ts}); break;
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
            bus.PublishPhase({EcPhase::MsdFail, ts}); break;
        case TAF_ECALL_STATE_INCOMING:
            bus.PublishPhase({EcPhase::Incoming, ts}); break;
        case TAF_ECALL_STATE_ENDED:
        case TAF_ECALL_STATE_END_OF_REDIAL_PERIOD:
            bus.PublishPhase({EcPhase::Teardown, ts}); break;

        // PSAP signals
        case TAF_ECALL_STATE_PSAP_START_RECEIVED:
            bus.PublishPsap({PsapSig::Start, ts}); break;
        case TAF_ECALL_STATE_LL_ACK_RECEIVED:
            bus.PublishPsap({PsapSig::LlAck, ts}); break;
        case TAF_ECALL_STATE_LLNACK_RECEIVED:
            bus.PublishPsap({PsapSig::LlNack, ts}); break;
        case TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
            bus.PublishPsap({PsapSig::AlAckPositive, ts}); break;
        case TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
            bus.PublishPsap({PsapSig::AlAckClearDown, ts}); break;
        case TAF_ECALL_STATE_MSD_UPDATE_REQ:
            bus.PublishPsap({PsapSig::MsdPullReq, ts}); break;

        // Timer events
        case TAF_ECALL_STATE_T2_STARTED:  bus.PublishTimer({EcTimerId::T2,  EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T5_STARTED:  bus.PublishTimer({EcTimerId::T5,  EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T6_STARTED:  bus.PublishTimer({EcTimerId::T6,  EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T7_STARTED:  bus.PublishTimer({EcTimerId::T7,  EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T9_STARTED:  bus.PublishTimer({EcTimerId::T9,  EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T10_STARTED: bus.PublishTimer({EcTimerId::T10, EcTimerAction::Start,  ts}); break;
        case TAF_ECALL_STATE_T2_STOPPED:  bus.PublishTimer({EcTimerId::T2,  EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T5_STOPPED:  bus.PublishTimer({EcTimerId::T5,  EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T6_STOPPED:  bus.PublishTimer({EcTimerId::T6,  EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T7_STOPPED:  bus.PublishTimer({EcTimerId::T7,  EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T9_STOPPED:  bus.PublishTimer({EcTimerId::T9,  EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T10_STOPPED: bus.PublishTimer({EcTimerId::T10, EcTimerAction::Stop,   ts}); break;
        case TAF_ECALL_STATE_T2_EXPIRED:  bus.PublishTimer({EcTimerId::T2,  EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T5_EXPIRED:  bus.PublishTimer({EcTimerId::T5,  EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T6_EXPIRED:  bus.PublishTimer({EcTimerId::T6,  EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T7_EXPIRED:  bus.PublishTimer({EcTimerId::T7,  EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T9_EXPIRED:  bus.PublishTimer({EcTimerId::T9,  EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T10_EXPIRED: bus.PublishTimer({EcTimerId::T10, EcTimerAction::Expire, ts}); break;
        case TAF_ECALL_STATE_T9_RESUMED:  bus.PublishTimer({EcTimerId::T9,  EcTimerAction::Resume, ts}); break;

        default:
            LE_WARN("[%s] Unhandled taf_ecall_State_t=%d", LOG_NAME, static_cast<int>(st));
            break;
    }
}

void ApiEcall::EcallStateCb(taf_ecall_CallRef_t /*ref*/,
                            taf_ecall_State_t   st,
                            void*               ctx)
{
    auto* self = static_cast<ApiEcall*>(ctx);
    if (self) self->HandleState(st);
}

} // namespace ecall

