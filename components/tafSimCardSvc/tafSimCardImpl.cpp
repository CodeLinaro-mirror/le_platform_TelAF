/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include "legato.h"
#include "interfaces.h"
#include "tafSimCard.hpp"
#include <cstdlib>
#include <algorithm>

using namespace tafsvc;
using namespace std;

static taf_sim_info_t simList[TAF_SIM_ID_MAX];
static int fplmnListIndex = 0;
le_thread_Ref_t taf_sim::mainThread = NULL;
static le_mem_PoolRef_t g_simEventPool = NULL;
static le_mem_PoolRef_t g_simIccidEventPool = NULL;
static le_mem_PoolRef_t g_internalRefreshEventPool = NULL;
static const uint32_t REFRESH_TRANSACTION_TIMEOUT_MS = 30000;

typedef struct
{
    bool refreshOkSent;
    bool refreshCompleteSent;
    bool transactionActive;
    bool ignoreLateEvents;
    taf_pa_sim_SessionType_t activeSessionType;
    taf_pa_sim_RefreshMode_t activeMode;
    le_timer_Ref_t transactionTimer;
    std::vector<taf_sim_RefreshRef_t> participantRefs;
    int  pendingEndCount;      // How many participants still await END notification this round.
    int  pendingCompleteCount; // How many participants still need to receive START before RefreshComplete.
} RefreshTransactionState_t;

static RefreshTransactionState_t g_refreshStatePri = {};
static RefreshTransactionState_t g_refreshStateSec = {};

static RefreshTransactionState_t* GetRefreshTransactionState(taf_pa_sim_SessionType_t sessionType)
{
    if (sessionType == TAF_PA_SIM_SESSION_TYPE_PRI_GW_PROV)
    {
        return &g_refreshStatePri;
    }
    if (sessionType == TAF_PA_SIM_SESSION_TYPE_SEC_GW_PROV)
    {
        return &g_refreshStateSec;
    }
    return nullptr;
}

static bool IsFcnLikeMode(taf_pa_sim_RefreshMode_t mode)
{
    return (mode == TAF_PA_SIM_REFRESH_MODE_FCN) ||
           (mode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN) ||
           (mode == TAF_PA_SIM_REFRESH_MODE_INIT_FULL_FCN);
}

static void StartRefreshTransactionTimer(RefreshTransactionState_t* refreshState)
{
    if (!refreshState || !refreshState->transactionTimer)
    {
        return;
    }
    le_timer_Stop(refreshState->transactionTimer);
    le_timer_Start(refreshState->transactionTimer);
}

static void ResetRefreshTransactionState(RefreshTransactionState_t* refreshState,
                                         bool clearParticipantFlags = true)
{
    if (!refreshState)
    {
        return;
    }
    if (clearParticipantFlags)
    {
        std::vector<taf_sim_RefreshRef_t> participantRefs = refreshState->participantRefs;
        for (auto sessionRef : participantRefs)
        {
            taf_sim_Session_t* sessionPtr = taf_sim::DiscoverSessionRef(sessionRef);
            if (!sessionPtr)
            {
                continue;
            }
            if (sessionPtr->refreshTimer)
            {
                le_timer_Stop(sessionPtr->refreshTimer);
            }
            sessionPtr->refreshResetStart = false;
            sessionPtr->refreshResetDone = false;
            sessionPtr->refreshParticipant = false;
            sessionPtr->refreshStartHandled = false;
            sessionPtr->refreshEndHandled = false;
            sessionPtr->refreshInitFcnPending = false;
            sessionPtr->refreshFileChangeReported = false;
        }
    }
    if (refreshState->transactionTimer)
    {
        le_timer_Stop(refreshState->transactionTimer);
    }
    refreshState->refreshOkSent = false;
    refreshState->refreshCompleteSent = false;
    refreshState->transactionActive = false;
    refreshState->activeSessionType = TAF_PA_SIM_SESSION_TYPE_UNKNOWN;
    refreshState->activeMode = TAF_PA_SIM_REFRESH_MODE_RESET;
    refreshState->participantRefs.clear();
    refreshState->pendingEndCount = 0;
    refreshState->pendingCompleteCount = 0;
}

static void ClearRefreshParticipantFlagsLocked(const RefreshTransactionState_t* refreshState)
{
    if (!refreshState)
    {
        return;
    }
    for (auto sessionRef : refreshState->participantRefs)
    {
        for (auto* sessionPtr : taf_sim::refresh_client)
        {
            if (!sessionPtr || sessionPtr->ref != sessionRef)
            {
                continue;
            }
            if (sessionPtr->refreshTimer)
            {
                le_timer_Stop(sessionPtr->refreshTimer);
            }
            sessionPtr->refreshResetStart = false;
            sessionPtr->refreshResetDone = false;
            sessionPtr->refreshParticipant = false;
            sessionPtr->refreshStartHandled = false;
            sessionPtr->refreshEndHandled = false;
            sessionPtr->refreshInitFcnPending = false;
            sessionPtr->refreshFileChangeReported = false;
            break;
        }
    }
}

static void RefreshTransactionTimeoutHandler(le_timer_Ref_t timerRef)
{
    RefreshTransactionState_t* refreshState =
        (RefreshTransactionState_t*)le_timer_GetContextPtr(timerRef);
    if (!refreshState || !refreshState->transactionActive)
    {
        return;
    }
    LE_WARN("Refresh transaction timeout: sessionType=%d mode=%d participants=%zu pendingEnd=%d pendingComplete=%d",
            refreshState->activeSessionType, refreshState->activeMode,
            refreshState->participantRefs.size(), refreshState->pendingEndCount,
            refreshState->pendingCompleteCount);
    std::vector<taf_sim_RefreshRef_t> participantRefs = refreshState->participantRefs;
    for (auto sessionRef : participantRefs)
    {
        taf_sim_Session_t* sessionPtr = taf_sim::DiscoverSessionRef(sessionRef);
        if (!sessionPtr || sessionPtr->refreshEndHandled)
        {
            continue;
        }
        sessionPtr->refreshEndHandled = true;
        sim_refresh_event_t clientEvt = {0};
        clientEvt.refreshStatus = TAF_SIM_REFRESH_STATUS_FAILURE;
        LE_INFO("Refresh transaction timeout: notify FAILURE for session:%p", sessionPtr);
        le_event_Report(sessionPtr->RefreshChangeEventId, &clientEvt, sizeof(clientEvt));
    }
    ResetRefreshTransactionState(refreshState);
    refreshState->ignoreLateEvents = true;
}

static bool IsRefreshTransactionActive(taf_pa_sim_SessionType_t sessionType)
{
    RefreshTransactionState_t* refreshState = GetRefreshTransactionState(sessionType);
    return refreshState && refreshState->transactionActive;
}

static bool IsClientInterestedInRefresh(const taf_sim_Session_t* sessionPtr,
                                        const taf_pa_sim_RefreshChangeInd_t* ind,
                                        bool isInitFcnContinuation)
{
    if (!sessionPtr || !ind)
    {
        return false;
    }
    if (ind->refreshMode < TAF_PA_SIM_REFRESH_MODE_RESET ||
        ind->refreshMode > TAF_PA_SIM_REFRESH_MODE_3G_RESET)
    {
        return false;
    }
    if (isInitFcnContinuation)
    {
        return sessionPtr->refreshInitFcnPending;
    }
    uint32_t modeMask = (1u << (uint32_t)ind->refreshMode);
    return ((modeMask & sessionPtr->refreshMode) != 0);
}

static bool IsRefreshSessionTypeActive(taf_sim_SessionType_t sessionType)
{
    taf_pa_sim_SessionType_t paSessionType = taf_sim::ConvertTafSessionTypeToPaSessionType(sessionType);
    return IsRefreshTransactionActive(paSessionType);
}

static void RemoveSessionFromRefreshTransaction(taf_sim_Session_t* session)
{
    if (!session)
    {
        return;
    }
    RefreshTransactionState_t* refreshState =
        GetRefreshTransactionState(taf_sim::ConvertTafSessionTypeToPaSessionType(session->sessionType));
    if (!refreshState || !refreshState->transactionActive)
    {
        return;
    }
    bool wasSnapshotParticipant =
        (std::find(refreshState->participantRefs.begin(), refreshState->participantRefs.end(), session->ref) !=
         refreshState->participantRefs.end());
    refreshState->participantRefs.erase(std::remove(refreshState->participantRefs.begin(),
                                                    refreshState->participantRefs.end(),
                                                    session->ref),
                                        refreshState->participantRefs.end());
    if (wasSnapshotParticipant && session->refreshParticipant && !session->refreshStartHandled &&
        !refreshState->refreshCompleteSent && IsFcnLikeMode(refreshState->activeMode))
    {
        refreshState->pendingCompleteCount--;
        LE_INFO("Disconnect/Delete: pendingCompleteCount=%d for sessionType=%d",
                refreshState->pendingCompleteCount, session->sessionType);
        if (refreshState->pendingCompleteCount <= 0)
        {
            refreshState->refreshCompleteSent = true;
            refreshState->pendingCompleteCount = 0;
            pa_result_t res = taf_pa_sim_RefreshComplete(
                taf_sim::ConvertTafSessionTypeToPaSessionType(session->sessionType));
            LE_INFO("Disconnect/Delete: RefreshComplete sent for remaining clients result=%d", res);
        }
    }
    if (wasSnapshotParticipant && session->refreshParticipant && !session->refreshEndHandled)
    {
        refreshState->pendingEndCount--;
        LE_INFO("Disconnect/Delete: pendingEndCount=%d for sessionType=%d",
                refreshState->pendingEndCount, session->sessionType);
        if (refreshState->pendingEndCount <= 0)
        {
            ResetRefreshTransactionState(refreshState, false);
            LE_INFO("Disconnect/Delete: all participants gone/done, transaction reset for sessionType=%d",
                    session->sessionType);
        }
    }
    session->refreshParticipant = false;
    session->refreshStartHandled = false;
    session->refreshEndHandled = false;
    session->refreshInitFcnPending = false;
    session->refreshFileChangeReported = false;
}

std::vector<taf_sim_Session_t*> taf_sim::refresh_client;

taf_pa_common_LogLevel_t Utility::Convert::Level
(
    le_log_Level_t level
)
{
    switch (level)
    {
        case LE_LOG_DEBUG:
            return TAF_PA_COMMON_LOG_LEVEL_DEBUG;
        case LE_LOG_INFO:
            return TAF_PA_COMMON_LOG_LEVEL_INFO;
        case LE_LOG_WARN:
            return TAF_PA_COMMON_LOG_LEVEL_WARN;
        case LE_LOG_ERR:
            return TAF_PA_COMMON_LOG_LEVEL_ERROR;
        case LE_LOG_CRIT:
            return TAF_PA_COMMON_LOG_LEVEL_CRIT;
        case LE_LOG_EMERG:
            return TAF_PA_COMMON_LOG_LEVEL_EMERG;
        default:
            LE_INFO("Unknown level %d.", level);
    }

    return TAF_PA_COMMON_LOG_LEVEL_INFO;
}

le_result_t Utility::Convert::Result
(
    int32_t result
)
{
    switch (result)
    {
        case TAF_PA_SIM_RESULT_OK:
            return LE_OK;
        case TAF_PA_SIM_RESULT_FAULT:
            return LE_FAULT;
        case TAF_PA_SIM_RESULT_BAD_PARAMETER:
            return LE_BAD_PARAMETER ;
        case TAF_PA_SIM_RESULT_UNSUPPORTED:
            return LE_UNSUPPORTED;
        case TAF_PA_SIM_RESULT_TIMEOUT:
            return LE_TIMEOUT;
        case PA_NOT_IMPLEMENTED:
            return LE_NOT_IMPLEMENTED;
        default:
            LE_DEBUG("Unknown result %d.", result);
    }
    return LE_FAULT;
}

taf_pa_sim_AppType_t taf_sim::ConvertTafappTypeToPaappType(taf_sim_AppType_t appType) {
    switch (appType)
    {
        case TAF_SIM_APPTYPE_USIM:
          return TAF_PA_APPTYPE_USIM;

        case TAF_SIM_APPTYPE_SIM:
            return TAF_PA_APPTYPE_SIM;

        case TAF_SIM_APPTYPE_ISIM:
            return TAF_PA_APPTYPE_ISIM;

        default:
            LE_INFO("Unsupported appType");
            return TAF_PA_APPTYPE_UNKNOWN;
    }
}

void Handler::onCardInfoChanged
(
    const std::shared_ptr<taf_pa_sim_CardInfo_t>& cardInfo
)
{
    if (!cardInfo)
    {
        LE_ERROR("Received null cardInfo");
        return;
    }
    sim_event_t* simEvent = (sim_event_t*)le_mem_ForceAlloc(g_simEventPool);
    memset(simEvent, 0, sizeof(*simEvent));
    simEvent->simId = (taf_sim_Id_t)cardInfo->slotId;
    simEvent->state =  (taf_sim_States_t)cardInfo->state;
    LE_INFO("Card info changed for slotid: %d, State: %d",simEvent->simId,simEvent->state);
    le_event_QueueFunctionToThread(taf_sim::mainThread,taf_sim::HandleCardInfoChanged_Queued, simEvent,nullptr);
}

void Handler::onSubscriptionInfoChanged(const std::shared_ptr<taf_pa_sim_Iccid_t>& iccidDataInfo)
{
    LE_INFO("onSubscriptionInfoChanged received from PA");
    if (!iccidDataInfo)
    {
        LE_ERROR("Received null iccidDataInfo from PA layer");
        return;
    }
    if (iccidDataInfo->ICCID.empty()) {
        LE_WARN("Received empty ICCID ");
    }
    sim_iccid_event_t* evt = (sim_iccid_event_t*)le_mem_ForceAlloc(g_simIccidEventPool);
    memset(evt, 0, sizeof(*evt));
    evt->simId = (taf_sim_Id_t)iccidDataInfo->simId;
    le_result_t copyResult = le_utf8_Copy(evt->ICCID,iccidDataInfo->ICCID.c_str(),sizeof(evt->ICCID),nullptr);
    if (copyResult == LE_OVERFLOW)
    {
        LE_WARN("ICCID string truncated during copy");
    }
    le_event_QueueFunctionToThread(taf_sim::mainThread,taf_sim::HandleSubscriptionInfoChanged_Queued,evt,nullptr);
}

void Handler::ChangeCardPinResponseCb
(
    const std::shared_ptr<taf_pa_sim_ResponseInfo_t>& responseInfo
)
{
    if (!responseInfo) {
        LE_ERROR("ChangeCardPinResponseCb received null responseInfo");
        return;
    }
    auto &sim = taf_sim::GetInstance();
    sim_response_event_t simResponsePtr;
    memset(&simResponsePtr, 0, sizeof(simResponsePtr));
    simResponsePtr.simId = (taf_sim_Id_t) responseInfo->simId;
    if (!taf_sim::isValidSimId(simResponsePtr.simId)) {
        LE_WARN("Invalid simId: %d", (int)simResponsePtr.simId);
        return;
    }
    simResponsePtr.responseType = (taf_sim_LockResponse_t)responseInfo->responseType;
    simResponsePtr.result = Utility::Convert::Result(responseInfo->result);
    LE_INFO("ChangeCardPinResponse: simId=%d type=%d result=%d",simResponsePtr.simId,
            simResponsePtr.responseType,
            simResponsePtr.result);
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void Handler::unlockCardByPukResponseCb
(
    const std::shared_ptr<taf_pa_sim_UnlockCardPukResponseInfo_t>& responseInfo
)
{
    if (!responseInfo) {
        LE_ERROR("unlockCardByPukResponseCb received null responseInfo");
        return;
    }
    auto &sim = taf_sim::GetInstance();
    sim_response_event_t simResponsePtr;
    memset(&simResponsePtr, 0, sizeof(simResponsePtr));
    simResponsePtr.simId = (taf_sim_Id_t) responseInfo->simId;
    if (!taf_sim::isValidSimId(simResponsePtr.simId)) {
        LE_WARN("Invalid simId: %d", (int)simResponsePtr.simId);
        return;
    }
    simResponsePtr.responseType = (taf_sim_LockResponse_t)responseInfo->responseType;
    simResponsePtr.result = Utility::Convert::Result(responseInfo->result);
    LE_INFO("unlockCardByPukResponseCb: simId=%d type=%d result=%d",simResponsePtr.simId,
            simResponsePtr.responseType,simResponsePtr.result);
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void Handler::unlockCardByPinResponseCb
(
    const std::shared_ptr<taf_pa_sim_UnlockCardResponseInfo_t>& responseInfo
)
{
    if (!responseInfo) {
        LE_ERROR("unlockCardByPinResponseCb received null responseInfo");
        return;
    }
    auto &sim = taf_sim::GetInstance();
    sim_response_event_t simResponsePtr;
    memset(&simResponsePtr, 0, sizeof(simResponsePtr));
    simResponsePtr.simId = (taf_sim_Id_t) responseInfo->simId;
    if (!taf_sim::isValidSimId(simResponsePtr.simId)) {
        LE_WARN("Invalid simId: %d", (int)simResponsePtr.simId);
        return;
    }
    simResponsePtr.responseType = (taf_sim_LockResponse_t)responseInfo->responseType;
    simResponsePtr.result = Utility::Convert::Result(responseInfo->result);
    LE_INFO("unlockCardByPinResponseCb: simId=%d type=%d result=%d",simResponsePtr.simId,
            simResponsePtr.responseType,simResponsePtr.result);
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void Handler::setCardLockResponseCb
(
    const std::shared_ptr<taf_pa_sim_CardLockResponseInfo_t>& responseInfo
)
{
    if (!responseInfo) {
        LE_ERROR("setCardLockResponseCb received null responseInfo");
        return;
    }
    auto &sim = taf_sim::GetInstance();
    sim_response_event_t simResponsePtr;
    memset(&simResponsePtr, 0, sizeof(simResponsePtr));
    simResponsePtr.simId = (taf_sim_Id_t) responseInfo->simId;
    if (!taf_sim::isValidSimId(simResponsePtr.simId)) {
        LE_WARN("Invalid simId: %d", (int)simResponsePtr.simId);
        return;
    }
    simResponsePtr.responseType = (taf_sim_LockResponse_t)responseInfo->responseType;
    simResponsePtr.result = Utility::Convert::Result(responseInfo->result);
    LE_INFO("setCardLockResponseCb: simId=%d type=%d result=%d",simResponsePtr.simId,
            simResponsePtr.responseType,simResponsePtr.result);
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Register listeners.
 */
//--------------------------------------------------------------------------------------------------
void RegisterListeners()
{
    pa_result_t result = taf_pa_sim_RegisterListeners();
    if (result != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to register listeners via PA OSS API.");
    }
    LE_INFO("registered all listeners succesfully");
}

void taf_sim::Init(void)
{
    FPLMNNodePool = le_mem_CreatePool("FPLMNNodePool", sizeof(FPLMNNode_t));
    le_mem_ExpandPool(FPLMNNodePool, TAF_SIM_FPLMN_MAX_OPERATORS_PER_LIST);
    FPLMNListPool = le_mem_CreatePool("FPLMNListPool", sizeof(taf_sim_FPLMNList_t));
    le_mem_ExpandPool(FPLMNListPool, TAF_SIM_FPLMN_MAX_LISTS);
    FPLMNListRefMap = le_ref_CreateMap("FPLMNListRefMap", TAF_SIM_FPLMN_MAX_LISTS);

    SessionPool = le_mem_CreatePool("SessionPool", sizeof(taf_sim_Session_t));
    le_mem_ExpandPool(SessionPool, 12);
    SessionRefMap = le_ref_CreateMap("SessionRefMapRefMap", 10);
    g_simEventPool = le_mem_CreatePool("SimQueuedEventPool", sizeof(sim_event_t));
    le_mem_ExpandPool(g_simEventPool, 16);
    g_simIccidEventPool = le_mem_CreatePool("SimIccidQueuedEventPool", sizeof(sim_iccid_event_t));
    le_mem_ExpandPool(g_simIccidEventPool, 16);
    g_internalRefreshEventPool = le_mem_CreatePool("InternalRefreshEventPool", sizeof(InternalRefreshEvent_t));
    le_mem_ExpandPool(g_internalRefreshEventPool, 16);
    g_refreshStatePri.transactionTimer = le_timer_Create("RefreshTxnTimerPri");
    g_refreshStateSec.transactionTimer = le_timer_Create("RefreshTxnTimerSec");
    if (!g_refreshStatePri.transactionTimer || !g_refreshStateSec.transactionTimer)
    {
        LE_ERROR("Failed to create refresh transaction timers");
        return;
    }
    le_timer_SetWakeup(g_refreshStatePri.transactionTimer, false);
    le_timer_SetWakeup(g_refreshStateSec.transactionTimer, false);
    le_timer_SetMsInterval(g_refreshStatePri.transactionTimer, REFRESH_TRANSACTION_TIMEOUT_MS);
    le_timer_SetMsInterval(g_refreshStateSec.transactionTimer, REFRESH_TRANSACTION_TIMEOUT_MS);
    le_timer_SetRepeat(g_refreshStatePri.transactionTimer, 1);
    le_timer_SetRepeat(g_refreshStateSec.transactionTimer, 1);
    le_timer_SetContextPtr(g_refreshStatePri.transactionTimer, &g_refreshStatePri);
    le_timer_SetContextPtr(g_refreshStateSec.transactionTimer, &g_refreshStateSec);
    le_timer_SetHandler(g_refreshStatePri.transactionTimer, RefreshTransactionTimeoutHandler);
    le_timer_SetHandler(g_refreshStateSec.transactionTimer, RefreshTransactionTimeoutHandler);
    fplmnListIndex = 0;

    NewStateEventId = le_event_CreateId("NewStateEventId", sizeof(sim_event_t));
    ResponseEventId = le_event_CreateId("ResponseEventId", sizeof(sim_response_event_t));
    IccidChangeEventId = le_event_CreateId("IccidChangeEventId", sizeof(sim_iccid_event_t));
    RegisterListeners();
    eventListener.onSubscriptionInfoChanged = &Handler::onSubscriptionInfoChanged;
    eventListener.onCardInfoChanged = &Handler::onCardInfoChanged;
    eventListener.ChangeCardPinResponseCb = &Handler::ChangeCardPinResponseCb;
    eventListener.unlockCardByPinResponseCb = &Handler::unlockCardByPinResponseCb;
    eventListener.unlockCardByPukResponseCb = &Handler::unlockCardByPukResponseCb;
    eventListener.setCardLockResponseCb = &Handler::setCardLockResponseCb;
    pa_result_t regEvtRes = taf_pa_sim_RegisterEventListener(&eventListener,nullptr);
    if (regEvtRes != PA_OK)
    {
        LE_ERROR("Listener register failed for");
        return;
    }
    le_msg_AddServiceCloseHandler(taf_sim_GetServiceRef(), OnClientDisconnect, NULL);
}

taf_sim &taf_sim::GetInstance()
{
    static taf_sim instance;
    return instance;
}

taf_sim_States_t taf_sim::getState(taf_sim_Id_t simId)
{
    taf_pa_sim_States_t state = TAF_PA_SIM_STATE_UNKNOWN;
    pa_result_t result = taf_pa_sim_GetState((taf_pa_sim_Id_t) simId, &state);
    if (result != TAF_PA_SIM_RESULT_OK) {
        LE_ERROR("taf_pa_sim_GetState failed or returned error for simId: %d", simId);
        state = TAF_PA_SIM_STATE_UNKNOWN;
    }
    taf_sim_States_t sim_state = Utility::Convert::taf_Common_State_Result(state);
    LE_INFO("taf_pa_sim_GetState returned simId: %d, state is: %d", simId, sim_state);
    return sim_state;
}

taf_sim_NewStateHandlerRef_t taf_sim::AddStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add New State Handler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewStateHandler", NewStateEventId,
            FirstLayerNewSimStateHandler, (void*)handlerPtr);

    return (taf_sim_NewStateHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveStateHandler(taf_sim_NewStateHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerNewSimStateHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    sim_event_t* simEventPtr = (sim_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simEventPtr == NULL,"simEventPtr is NULL");

    taf_sim_NewStateHandlerFunc_t clientHandlerFunc =
        (taf_sim_NewStateHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simEventPtr->simId, simEventPtr->state, le_event_GetContextPtr());
}

taf_sim_IccidChangeHandlerRef_t taf_sim:: AddIccidChangeHandler(taf_sim_IccidChangeHandlerFunc_t handlerPtr) {
    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Iccid Change handler");
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("IccidChangeHandler", IccidChangeEventId,
            FirstLayerIccidChangeHandler, (void*)handlerPtr);

    return (taf_sim_IccidChangeHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveIccidChangeHandler(taf_sim_IccidChangeHandlerRef_t handlerRef){
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerIccidChangeHandler(void* reportPtr,
        void* secondLayerHandlerFunc){
    sim_iccid_event_t* simEventPtr = (sim_iccid_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simEventPtr == NULL, "simEventPtr is NULL");

    taf_sim_IccidChangeHandlerFunc_t clientHandlerFunc =
        (taf_sim_IccidChangeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simEventPtr->simId, simEventPtr->ICCID, le_event_GetContextPtr());
}

taf_sim_info_t* taf_sim::GetSimContext(taf_sim_Id_t simId) {
    simId = (TAF_SIM_UNSPECIFIED == simId) ? (taf_sim_Id_t)slot : simId;
    return simId > 0 ? &simList[simId - 1] : &simList[0];
}

bool taf_sim::isValidSimId(taf_sim_Id_t simId) {
    int slotCount = 1;
    auto &sim = taf_sim::GetInstance();
    le_result_t result = sim.getSlotCount(&slotCount);
    if(result != LE_OK)
    {
        LE_INFO("Fail to get slot count via PA OSS API.");
        return false;
    }
    LE_INFO("isValidSimId: slot count: %d, input simId: %d", slotCount, (int)simId);
    if ((simId > 0 && simId <= slotCount) || simId == TAF_SIM_UNSPECIFIED ) {
        return true;
    }
    return false;
}

void onRefreshEvent(taf_pa_sim_RefreshChangeInd_t ind, void* contextPtr)
{
    LE_INFO("OnRefreshEvent for %p:",contextPtr);
    if (!taf_sim::mainThread)
    {
        LE_ERROR("onRefreshEvent: mainThread is NULL, dropping event");
        return;
    }
    InternalRefreshEvent_t* evt = (InternalRefreshEvent_t*)le_mem_ForceAlloc(g_internalRefreshEventPool);
    memset(evt, 0, sizeof(*evt));
    evt->ind = ind;
    le_event_QueueFunctionToThread(taf_sim::mainThread, taf_sim::HandleRefreshEvent_Queued, evt, nullptr);
}

taf_pa_sim_SessionType_t taf_sim::ConvertTafSessionTypeToPaSessionType(taf_sim_SessionType_t sessionType) {
    switch (sessionType)
    {
        case TAF_SIM_SESSION_TYPE_PRI_GW_PROV:
            return TAF_PA_SIM_SESSION_TYPE_PRI_GW_PROV;
        case TAF_SIM_SESSION_TYPE_SEC_GW_PROV:
            return TAF_PA_SIM_SESSION_TYPE_SEC_GW_PROV;
        default:
            LE_WARN("Unknown refresh session type %d.", sessionType);
    }
    return TAF_PA_SIM_SESSION_TYPE_PRI_GW_PROV;
}

taf_sim_RefreshStatus_t taf_sim::ConvertPaRefreshStageToTafRefreshStatus
(
    taf_pa_sim_RefreshStage_t refreshStage
)
{
    switch (refreshStage)
    {
        case TAF_PA_SIM_REFRESH_STAGE_END_WITH_SUCCESS:
            return TAF_SIM_REFRESH_STATUS_SUCCESS;
        case TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE:
            return TAF_SIM_REFRESH_STATUS_FAILURE;
        default:
            LE_WARN("Unknown refresh stage %d.", refreshStage);
    }
    return TAF_SIM_REFRESH_STATUS_FAILURE;
}

static bool CheckAndUpdateSessionProfileIccid(taf_sim_Session_t* sessionPtr,
                                              const char* iccid1,
                                              const char* iccid2)
{
    if (!sessionPtr)
    {
        return false;
    }
    char* previousIccid = NULL;
    const char* currentIccid = NULL;
    if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV)
    {
        previousIccid = sessionPtr->simProfileIccid1;
        currentIccid = iccid1;
    }
    else if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV)
    {
        previousIccid = sessionPtr->simProfileIccid2;
        currentIccid = iccid2;
    }
    else
    {
        return false;
    }
    if (!currentIccid || currentIccid[0] == '\0')
    {
        return false;
    }
    bool profileSwitched = previousIccid[0] != '\0' &&
        strncmp(currentIccid, previousIccid, TAF_SIM_ICCID_BYTES) != 0;
    if (previousIccid[0] == '\0' || profileSwitched)
    {
        LE_INFO("Update profile ICCID for session:%p old:%s new:%s profileSwitched:%d",
                sessionPtr, previousIccid, currentIccid, (int)profileSwitched);
        le_utf8_Copy(previousIccid, currentIccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    return profileSwitched;
}

void taf_sim::CheckAndSendProfileSwitchEvent() {
    auto &sim = taf_sim::GetInstance();

    char iccid1[TAF_SIM_ICCID_BYTES];
    char iccid2[TAF_SIM_ICCID_BYTES];
    char iccid[TAF_SIM_ICCID_BYTES]  = {0};

    LE_DEBUG("ICCID change and profile swap");

    le_result_t result = LE_FAULT;
    memset(iccid1, 0, TAF_SIM_ICCID_BYTES);
    memset(iccid2, 0, TAF_SIM_ICCID_BYTES);

    if (getICCID(TAF_SIM_SLOT_ID_1, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(iccid1, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    memset(iccid, 0, sizeof(iccid));
    if (getICCID(TAF_SIM_SLOT_ID_2, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(iccid2, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }

    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("CheckAndSendProfileSwitchEvent sessionPtr null!");
            result = le_ref_NextNode(iterRef);
            continue;
        }

        LE_INFO("ClientSessionRef %p, refreshResetStart: %d", sessionPtr->clientSessionRef,
                (int)sessionPtr->refreshResetStart);

        // Send profile switch notification.
        // SUCCESS | PROFILE_SWITCH.
        if (sessionPtr->refreshResetStart)
        {
            result = le_ref_NextNode(iterRef);
            continue;
        }

        if (CheckAndUpdateSessionProfileIccid(sessionPtr, iccid1, iccid2))
        {
            sim_refresh_event_t simRefreshEvent = {};
            simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
            LE_INFO("Notify PROFILE_SWITCH for session:%p result:%s", sessionPtr, LE_RESULT_TXT(result));
            le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        }
        result = le_ref_NextNode(iterRef);
    }
}

void taf_sim::CheckAndSendRefreshEvent(taf_sim_Id_t SimId) {
    auto &sim = taf_sim::GetInstance();
    int slotCount =1;
    char iccid1[TAF_SIM_ICCID_BYTES] = {0};
    char iccid2[TAF_SIM_ICCID_BYTES] = {0};
    char iccid[TAF_SIM_ICCID_BYTES] = {0};
    if (getICCID(TAF_SIM_SLOT_ID_1, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(iccid1, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    memset(iccid, 0, sizeof(iccid));
    if (getICCID(TAF_SIM_SLOT_ID_2, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(iccid2, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    le_result_t result = sim.getSlotCount(&slotCount);
    if(result != LE_OK)
    {
        LE_INFO("Fail to get slot count via PA OSS API.");
    }
    else
    {
        LE_INFO("CheckAndSendRefreshEvent: slotCount=%d, isSingleActive=%s",slotCount, isSingleActive ? "true" : "false");
    }
    std::vector<taf_sim_Session_t*> readySessions;
    {
        std::unique_lock<std::mutex> lock(sim.eventMutex);
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* sessionPtr =(taf_sim_Session_t*)le_ref_GetValue(iterRef);
            if (!sessionPtr)
                continue;
            bool match =isSingleActive ||(sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV &&
                 SimId == TAF_SIM_SLOT_ID_1) ||(sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV &&SimId == TAF_SIM_SLOT_ID_2);
            if (match && sessionPtr->refreshResetStart)
            {
                sessionPtr->refreshResetStart = false;
                sessionPtr->refreshResetDone  = true;
                readySessions.push_back(sessionPtr);
            }
        }
    }
    for (auto* sessionPtr : readySessions)
    {
        LE_INFO("Notify RefreshEvent for SimId:%d SessionType:%d",SimId, sessionPtr->sessionType);
        if (sessionPtr->refreshTimer)
        {
            le_timer_Stop(sessionPtr->refreshTimer);
        }
        sim_refresh_event_t simRefreshEvent = {};
        simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_SUCCESS;
        if (CheckAndUpdateSessionProfileIccid(sessionPtr, iccid1, iccid2))
        {
            simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
            LE_INFO("RESET ready with profile switch for session:%p", sessionPtr);
        }
        le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        sim.ResetVoteSend(sessionPtr);
    }
}

void taf_sim::FirstLayerNewRefreshChangeHandler(void* reportPtr, void* secondLayerHandlerFunc) {
    sim_refresh_event_t* simRefreshPtr = (sim_refresh_event_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(simRefreshPtr == NULL, "simRefreshPtr is NULL");
    taf_sim_RefreshChangeHandlerFunc_t clientHandlerFunc =
        (taf_sim_RefreshChangeHandlerFunc_t)secondLayerHandlerFunc;
    clientHandlerFunc(simRefreshPtr->refreshStatus, le_event_GetContextPtr());
}

taf_sim_RefreshChangeHandlerRef_t taf_sim::AddRefreshChangeHandler(taf_sim_RefreshChangeHandlerFunc_t handlerPtr,
            void* contextPtr)
{
    LE_INFO("Add Refresh Change handler");
    le_event_HandlerRef_t handlerRef;
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL");
        return NULL;
    }

    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    std::lock_guard<std::mutex> lock(eventMutex);
    handlerRef = le_event_AddLayeredHandler("RefreshChangeHandler", clientRequestPtr->RefreshChangeEventId,
            FirstLayerNewRefreshChangeHandler, (void*)handlerPtr);
    if (handlerRef == NULL)
    {
        LE_ERROR("AddRefreshChangeHandler: le_event_AddLayeredHandler failed");
        return NULL;
    }
    pa_result_t addRes = taf_pa_sim_AddRefreshChangeHandler((taf_pa_sim_RefreshChangeHandlerFunc_t)&onRefreshEvent,
                  sessionRef,&clientRequestPtr->paHandlerRef);
    if (addRes != PA_OK)
    {
        LE_ERROR("taf_pa_sim_AddRefreshChangeHandler returned: %d", (int)addRes);
        le_event_RemoveHandler(handlerRef);
        return NULL;
    }

    LE_INFO("taf_pa_sim_AddRefreshChangeHandler done. paHandlerRef: %p, handlerRef: %p",
        clientRequestPtr->paHandlerRef, handlerRef);
    clientRequestPtr->clientHandlerRef = handlerRef;
    taf_sim::refresh_client.push_back(clientRequestPtr);
    LE_INFO("Handler added. client count=%zu", refresh_client.size());
    return (taf_sim_RefreshChangeHandlerRef_t)(handlerRef);
}

void taf_sim::RemoveRefreshChangeHandler(taf_sim_RefreshChangeHandlerRef_t handlerRef) {
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();
    clientRequestPtr = DiscoverSessionRef(sessionRef);
    TAF_ERROR_IF_RET_NIL( NULL == clientRequestPtr, "clientRequestPtr is NULL");
    std::lock_guard<std::mutex> lock(eventMutex);
    if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType))
    {
        LE_WARN("RemoveRefreshChangeHandler ignored: refresh transaction active for sessionType=%d",
                clientRequestPtr->sessionType);
        return;
    }
    if (clientRequestPtr->clientHandlerRef != NULL)
    {
        le_event_RemoveHandler(clientRequestPtr->clientHandlerRef);
        clientRequestPtr->clientHandlerRef = NULL;
    }
    if (clientRequestPtr->paHandlerRef != NULL)
    {
    pa_result_t removeRes = taf_pa_sim_RemoveRefreshChangeHandler(clientRequestPtr->paHandlerRef);
    if (removeRes != PA_OK)
    {
        LE_WARN("taf_pa_sim_RemoveRefreshChangeHandler returned: %d", (int)removeRes);
        }
        clientRequestPtr->paHandlerRef = NULL;
    }
    refresh_client.erase(std::remove(refresh_client.begin(), refresh_client.end(), clientRequestPtr), refresh_client.end());
    LE_INFO("Handler removed. Remaining clients: %zu", refresh_client.size());
}

void taf_sim::DestroySessionLocked(taf_sim_Session_t* session)
{
    if (!session) return;
    RemoveSessionFromRefreshTransaction(session);
    if (session->clientHandlerRef != NULL)
    {
        le_event_RemoveHandler(session->clientHandlerRef);
        session->clientHandlerRef = NULL;
    }
    if (session->paHandlerRef != NULL)
    {
        pa_result_t removeRes = taf_pa_sim_RemoveRefreshChangeHandler(session->paHandlerRef);
        if (removeRes != PA_OK)
        {
            LE_WARN("DestroySessionLocked: taf_pa_sim_RemoveRefreshChangeHandler returned: %d", (int)removeRes);
        }
        session->paHandlerRef = NULL;
    }
    refresh_client.erase(std::remove(refresh_client.begin(), refresh_client.end(), session), refresh_client.end());
    if (session->refreshTimer)
    {
        le_timer_Stop(session->refreshTimer);
    }
    // Single, consistent teardown path (deletes timer, ref and releases memory)
    CleanupSession(session);
    mClientRefCount--;
}

le_result_t taf_sim::DeleteSession(taf_sim_RefreshRef_t refreshSessionRef)
{
    taf_sim_Session_t* clientRequestPtr = DiscoverSessionRef(refreshSessionRef);
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    taf_sim_SessionType_t sessionType = clientRequestPtr->sessionType;
    {
        std::lock_guard<std::mutex> lock(eventMutex);
        if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType))
        {
            LE_WARN("DeleteSession rejected: refresh transaction active for sessionType=%d", clientRequestPtr->sessionType);
            return LE_BUSY;
        }
        DestroySessionLocked(clientRequestPtr);
        LE_INFO("Session deleted. Remaining clients: %zu", refresh_client.size());
    }
    le_result_t refreshResult = RefreshRegisterFilesForSessionType(sessionType);
    if (refreshResult != LE_OK)
    {
        LE_WARN("DeleteSession: failed to refresh PA register files for sessionType=%d result=%d",
                sessionType, refreshResult);
    }
    return LE_OK;
}

void taf_sim::OnClientDisconnect(le_msg_SessionRef_t clientSessionRef, void* contextPtr)
{
    auto &sim = taf_sim::GetInstance();
    LE_INFO("OnClientDisconnect: clientSessionRef=%p", clientSessionRef);
    bool priAffected = false;
    bool secAffected = false;
    {
        std::lock_guard<std::mutex> lock(sim.eventMutex);
        std::vector<taf_sim_Session_t*> toDelete;
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
            if (sessionPtr && sessionPtr->clientSessionRef == clientSessionRef)
            {
                toDelete.push_back(sessionPtr);
            }
        }
        for (auto* sessionPtr : toDelete)
        {
            taf_sim_SessionType_t sessionType = sessionPtr->sessionType;
            if (sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV)
            {
                priAffected = true;
            }
            else if (sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV)
            {
                secAffected = true;
            }
            LE_INFO("OnClientDisconnect: cleaning up session %p", sessionPtr);
            sim.DestroySessionLocked(sessionPtr);
        }
    }
    if (priAffected)
    {
        le_result_t refreshResult = sim.RefreshRegisterFilesForSessionType(TAF_SIM_SESSION_TYPE_PRI_GW_PROV);
        if (refreshResult != LE_OK)
        {
            LE_WARN("OnClientDisconnect: failed to refresh PA register files for PRI_GW_PROV result=%d",
                    refreshResult);
        }
    }
    if (secAffected)
    {
        le_result_t refreshResult = sim.RefreshRegisterFilesForSessionType(TAF_SIM_SESSION_TYPE_SEC_GW_PROV);
        if (refreshResult != LE_OK)
        {
            LE_WARN("OnClientDisconnect: failed to refresh PA register files for SEC_GW_PROV result=%d",
                    refreshResult);
        }
    }
}

taf_sim_Session_t* taf_sim::DiscoverSessionRef
(
    taf_sim_RefreshRef_t sessionRef
)
{
    auto &sim = taf_sim::GetInstance();
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("DiscoverSessionRef sessionPtr null!");
            return NULL;
        }

        LE_DEBUG("SessionRef %p, clientSessionRef %p", sessionRef, sessionPtr->clientSessionRef);

        if (sessionRef == sessionPtr->ref || sessionRef == (taf_sim_RefreshRef_t) sessionPtr->clientSessionRef)
        {
             return sessionPtr;
        }
        result = le_ref_NextNode(iterRef);
    }
    LE_INFO("DiscoverSessionRef sessionPtr is null!");
    return NULL;
}

le_result_t taf_sim::CreateSession(taf_sim_SessionType_t sessionType, taf_sim_RefreshRef_t* refreshSessionRef) {
    char iccid[TAF_SIM_ICCID_BYTES] = {0};
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();
    LE_INFO("CreateSession client session ref %p", sessionRef);
    clientRequestPtr = DiscoverSessionRef(sessionRef);
    LE_INFO("After DiscoverSessionRef client session ref %p", clientRequestPtr);
    if (clientRequestPtr != nullptr) {
        if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType) || IsRefreshSessionTypeActive(sessionType))
        {
            LE_WARN("CreateSession rejected: refresh transaction active for old/new sessionType=%d/%d",
                    clientRequestPtr->sessionType, sessionType);
            return LE_BUSY;
        }
        LE_INFO("CreateSession: Already created the refresh Session, so use the existing one.");
        *refreshSessionRef = (taf_sim_RefreshRef_t) clientRequestPtr->ref;
        clientRequestPtr->sessionType = sessionType;
        return LE_OK;
    }
    if (IsRefreshSessionTypeActive(sessionType))
    {
        LE_WARN("CreateSession rejected: refresh transaction active for sessionType=%d", sessionType);
        return LE_BUSY;
    }
    taf_sim_Session_t* res  = (taf_sim_Session_t* )le_mem_ForceAlloc(SessionPool);
    if(res == NULL) {
        LE_INFO("Create SessionPool failed!");
        return LE_FAULT;
    }
    memset(res, 0, sizeof(*res));
    res->clientHandlerRef = NULL;
    LE_INFO("Create new Session");
    res->link = LE_DLS_LIST_INIT;
    res->ref = (taf_sim_RefreshRef_t)le_ref_CreateRef(SessionRefMap, res);
    res->clientSessionRef = (le_msg_SessionRef_t) sessionRef;
    *refreshSessionRef = (taf_sim_RefreshRef_t)(res->ref);
    res->sessionType = sessionType;
    res->refreshAllow = true;
    res->refreshMode = (taf_sim_RefreshMode_t) 0x7F; // Default: match all refresh modes (bits 0..6)
    res->refreshTimer = le_timer_Create("RefreshTimer");
    if (!res->refreshTimer)
    {
       CleanupSession(res);
       return LE_FAULT;
    }
    le_timer_SetWakeup(res->refreshTimer, false);
    le_timer_SetMsInterval(res->refreshTimer, 5000); // 5 sec
    le_timer_SetRepeat(res->refreshTimer, 1);
    le_timer_SetContextPtr(res->refreshTimer, res);
    le_timer_SetHandler(res->refreshTimer, RefreshTimeoutHandler);
    res->RefreshChangeEventId = le_event_CreateId("ClientRefreshEventId", sizeof(sim_refresh_event_t));
    if (!res->RefreshChangeEventId)
    {
       CleanupSession(res);
       return LE_FAULT;
    }
    LE_INFO("res->sessionRef %p, *reference %p", res->ref, *refreshSessionRef);
    memset(res->simProfileIccid1, 0, TAF_SIM_ICCID_BYTES);
    memset(res->simProfileIccid2, 0, TAF_SIM_ICCID_BYTES);
    if (getICCID(TAF_SIM_SLOT_ID_1, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(res->simProfileIccid1, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    memset(iccid, 0, sizeof(iccid));
    if (getICCID(TAF_SIM_SLOT_ID_2, iccid, sizeof(iccid)) == LE_OK)
    {
        le_utf8_Copy(res->simProfileIccid2, iccid, TAF_SIM_ICCID_BYTES, NULL);
    }
    LE_INFO("Refresh create session done: iccid1: %s, iccid2: %s", res->simProfileIccid1, res->simProfileIccid2);
    if (sessionRef!=nullptr) {
        //External client increase Client ref count
        mClientRefCount++;
    }
    LE_INFO("SessionRef %p was not found, Created new Client session, total count %d", sessionRef, mClientRefCount);
    return LE_OK;
}

le_result_t taf_sim::SetRefreshRegisterFiles(taf_sim_RefreshRef_t refreshSessionRef, const taf_sim_RefreshRegFile_t* filesPtr, size_t filesSize) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType))
    {
        LE_WARN("SetRefreshRegisterFiles rejected: refresh transaction active for sessionType=%d",
                clientRequestPtr->sessionType);
        return LE_BUSY;
    }
    if (filesSize > TAF_SIM_MAX_SIM_REFRESH_FILES) {
        LE_ERROR("filesSize %zu exceeds max %d", filesSize, TAF_SIM_MAX_SIM_REFRESH_FILES);
        return LE_BAD_PARAMETER;
    }

    for (int i = 0; i < (int) filesSize; i++) {
        clientRequestPtr->refreshRegFiles[i].file_id = filesPtr[i].file_id;
        le_utf8_Copy((char*) clientRequestPtr->refreshRegFiles[i].path, (char*) filesPtr[i].path, sizeof(filesPtr[i].path), NULL);

        LE_INFO("File_id: %d and path: %s", clientRequestPtr->refreshRegFiles[i].file_id, clientRequestPtr->refreshRegFiles[i].path);
    }

    clientRequestPtr->refreshRegFilesSize = filesSize;

    return LE_OK;
}

le_result_t taf_sim::RefreshRegisterFilesForSessionType(taf_sim_SessionType_t sessionType)
{
    if (IsRefreshSessionTypeActive(sessionType))
    {
        LE_WARN("RefreshRegisterFilesForSessionType rejected: refresh transaction active for sessionType=%d",
                sessionType);
        return LE_BUSY;
    }
    std::vector<taf_sim_RefreshRegFile_t> uniqueFiles;
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        le_ref_IterRef_t iterRef = le_ref_GetIterator(SessionRefMap);
        le_result_t result = le_ref_NextNode(iterRef);
        while (LE_OK == result)
        {
            taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*)le_ref_GetValue(iterRef);
            if (sessionPtr && sessionPtr->sessionType == sessionType)
            {
                for (size_t i = 0; i < sessionPtr->refreshRegFilesSize; ++i)
                {
                    bool exists = false;
                    for (const auto& file : uniqueFiles)
                    {
                        if (file.file_id == sessionPtr->refreshRegFiles[i].file_id &&
                            strncmp(file.path, sessionPtr->refreshRegFiles[i].path, sizeof(file.path)) == 0)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                    {
                        uniqueFiles.push_back(sessionPtr->refreshRegFiles[i]);
                    }
                }
            }
            result = le_ref_NextNode(iterRef);
        }
    }
    std::vector<taf_pa_sim_RefreshFile_t> refreshPAFiles;
    for (size_t i = 0; i < uniqueFiles.size(); ++i)
    {
        taf_pa_sim_RefreshFile_t paFile;
        memset(&paFile, 0, sizeof(paFile));
        int pathStrLen = strlen(uniqueFiles[i].path);
        paFile.file_id = uniqueFiles[i].file_id;
        paFile.path_len = pathStrLen / 2;
        if (pathStrLen > 0)
        {
            char* endPtr = nullptr;
            uint32_t pathValue = (uint32_t)strtoul(uniqueFiles[i].path, &endPtr, 16);
            if (!endPtr || *endPtr != '\0')
            {
                LE_WARN("Invalid refresh file path while refreshing PA registration: %s",
                        uniqueFiles[i].path);
                continue;
            }
            if (paFile.path_len == 2)
            {
                paFile.path[0] = (pathValue & 0x000000ff);
                paFile.path[1] = (pathValue & 0x0000ff00) >> 8;
            }
            else if (paFile.path_len == 4)
            {
                paFile.path[0] = (pathValue & 0x00ff0000) >> 16;
                paFile.path[1] = (pathValue & 0xff000000) >> 24;
                paFile.path[2] = (pathValue & 0x000000ff);
                paFile.path[3] = (pathValue & 0x0000ff00) >> 8;
            }
            else
            {
                LE_WARN("Unsupported refresh file path length while refreshing PA registration: %d for path: %s",
                        pathStrLen, uniqueFiles[i].path);
                continue;
            }
        }
        LE_INFO("Refresh re-register PA File_id: %d path_len: %d path: %x %x %x %x",
                paFile.file_id, paFile.path_len, paFile.path[0], paFile.path[1],
                paFile.path[2], paFile.path[3]);
        refreshPAFiles.push_back(paFile);
    }
    pa_result_t res = taf_pa_sim_RefreshRegister(ConvertTafSessionTypeToPaSessionType(sessionType),
            refreshPAFiles.size(),
            refreshPAFiles.empty() ? nullptr : refreshPAFiles.data());
    le_result_t result = Utility::Convert::Result(res);
    LE_INFO("Refresh re-register done for sessionType=%d with union file count=%zu result=%d",
            sessionType, uniqueFiles.size(), res);
    return result;
}

le_result_t taf_sim::SetRefreshMode(taf_sim_RefreshRef_t refreshSessionRef, taf_sim_RefreshMode_t refreshMode) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType))
    {
        LE_WARN("SetRefreshMode rejected: refresh transaction active for sessionType=%d",
                clientRequestPtr->sessionType);
        return LE_BUSY;
    }
    clientRequestPtr->refreshMode = refreshMode;
    return LE_OK;
}

le_result_t taf_sim::SetRefreshAllow(taf_sim_RefreshRef_t refreshSessionRef, bool isRefreshAllowed) {

    taf_sim_Session_t* clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    if (IsRefreshSessionTypeActive(clientRequestPtr->sessionType))
    {
        LE_WARN("SetRefreshAllow rejected: refresh transaction active for sessionType=%d",
                clientRequestPtr->sessionType);
        return LE_BUSY;
    }

    std::vector<taf_sim_RefreshRegFile_t> uniqueFiles;
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        le_ref_IterRef_t iterRef = le_ref_GetIterator(SessionRefMap);
        le_result_t result = le_ref_NextNode(iterRef);
        while (LE_OK == result)
        {
            taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*)le_ref_GetValue(iterRef);
            if (sessionPtr && sessionPtr->sessionType == clientRequestPtr->sessionType)
            {
                for (size_t i = 0; i < sessionPtr->refreshRegFilesSize; ++i)
                {
                    bool exists = false;
                    for (const auto& file : uniqueFiles)
                    {
                        if (file.file_id == sessionPtr->refreshRegFiles[i].file_id &&
                            strncmp(file.path, sessionPtr->refreshRegFiles[i].path, sizeof(file.path)) == 0)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                    {
                        uniqueFiles.push_back(sessionPtr->refreshRegFiles[i]);
                    }
                }
            }
            result = le_ref_NextNode(iterRef);
        }
    }

    std::vector<taf_pa_sim_RefreshFile_t> refreshPAFiles;
    for (size_t i = 0; i < uniqueFiles.size(); ++i)
    {
        taf_pa_sim_RefreshFile_t paFile;
        memset(&paFile, 0, sizeof(paFile));
        int pathStrLen = strlen(uniqueFiles[i].path);
        paFile.file_id = uniqueFiles[i].file_id;
        paFile.path_len = pathStrLen / 2;

        if (pathStrLen > 0)
        {
            char* endPtr = nullptr;
            uint32_t pathValue = (uint32_t)strtoul(uniqueFiles[i].path, &endPtr, 16);
            if (!endPtr || *endPtr != '\0')
            {
                LE_WARN("Invalid refresh file path: %s", uniqueFiles[i].path);
                continue;
            }
            LE_INFO("pathValue string: %s, in hex: %x and input path len: %d", uniqueFiles[i].path, pathValue, pathStrLen);

            if (paFile.path_len == 2)
            {
                paFile.path[0] = (pathValue & 0x000000ff);
                paFile.path[1] = (pathValue & 0x0000ff00) >> 8;
            }
            else if (paFile.path_len == 4)
            {
                paFile.path[0] = (pathValue & 0x00ff0000) >> 16;
                paFile.path[1] = (pathValue & 0xff000000) >> 24;
                paFile.path[2] = (pathValue & 0x000000ff);
                paFile.path[3] = (pathValue & 0x0000ff00) >> 8;
            }
            else
            {
                LE_WARN("Unsupported refresh file path length: %d for path: %s", pathStrLen, uniqueFiles[i].path);
                continue;
            }
        }
        LE_INFO("PA file path0 ~ path3 in hex: %x %x %x %x", paFile.path[0], paFile.path[1], paFile.path[2], paFile.path[3]);

        LE_INFO("PA file path0 ~ path3 in dec: %u %u %u %u", paFile.path[0], paFile.path[1], paFile.path[2], paFile.path[3]);

        LE_INFO("PA File_id: %d and path_len: %d", paFile.file_id, paFile.path_len);
        refreshPAFiles.push_back(paFile);
    }

    pa_result_t res = taf_pa_sim_RefreshRegister(ConvertTafSessionTypeToPaSessionType(clientRequestPtr->sessionType),
            refreshPAFiles.size(),
            refreshPAFiles.empty() ? nullptr : refreshPAFiles.data());
    le_result_t result =Utility::Convert::Result(res);

    LE_INFO("Refresh register done for sessionType=%d with union file count=%zu result=%d",
            clientRequestPtr->sessionType, uniqueFiles.size(), res);

    if (result == LE_OK)
    {
    clientRequestPtr->refreshAllow = isRefreshAllowed;
    }
    return result;
}

bool taf_sim::CheckRefreshAllow(taf_pa_sim_RefreshChangeInd_t* ind)
{
    auto &sim = taf_sim::GetInstance();
    if (!ind)
    {
        LE_WARN("CheckRefreshAllow: ind is NULL, defaulting to allow");
        return true;
    }
    if (ind->refreshMode < TAF_PA_SIM_REFRESH_MODE_RESET ||
        ind->refreshMode > TAF_PA_SIM_REFRESH_MODE_3G_RESET)
    {
        LE_WARN("CheckRefreshAllow: invalid refreshMode=%d, defaulting to allow", ind->refreshMode);
        return true;
    }
    RefreshTransactionState_t* refreshState = GetRefreshTransactionState(ind->sessionType);
    if (!refreshState || refreshState->participantRefs.empty())
    {
        LE_INFO("CheckRefreshAllow: no participant snapshot for sessionType=%d, defaulting to allow",
                ind->sessionType);
        return true;
    }
    std::vector<taf_sim_RefreshRef_t> participantRefs = refreshState->participantRefs;
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    for (auto sessionRef : participantRefs)
    {
        taf_sim_Session_t* sessionPtr = NULL;
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
        le_result_t result = le_ref_NextNode(iterRef);
        while (LE_OK == result)
        {
            taf_sim_Session_t* candidatePtr = (taf_sim_Session_t*)le_ref_GetValue(iterRef);
            if (candidatePtr && candidatePtr->ref == sessionRef)
            {
                sessionPtr = candidatePtr;
                break;
            }
            result = le_ref_NextNode(iterRef);
        }
        if (!sessionPtr)
        {
            LE_WARN("CheckRefreshAllow: participant sessionRef=%p disappeared, ignoring", sessionRef);
            continue;
        }
        if (sim.ConvertTafSessionTypeToPaSessionType(sessionPtr->sessionType) != ind->sessionType)
        {
            LE_WARN("CheckRefreshAllow: participant sessionType changed, sessionRef=%p", sessionRef);
            continue;
        }
        if (!sessionPtr->refreshAllow)
        {
            LE_INFO("CheckRefreshAllow: participant session:%p refreshMode:0x%x not allow",
                    sessionPtr, (unsigned int)sessionPtr->refreshMode);
            return false;
        }
    }
    return true;
}

void taf_sim::InternalRefreshHandler(void* reportPtr, void* contextPtr)
{
    InternalRefreshEvent_t* evt = (InternalRefreshEvent_t*)reportPtr;
    taf_sim_Session_t* session = (taf_sim_Session_t*)contextPtr;
    sim_refresh_event_t clientEvt = {0};
    taf_pa_sim_RefreshChangeInd_t* ind = &evt->ind;
    LE_INFO("InternalRefreshHandler: stage=%d mode=%d sessionType=%d for session:%p",
            ind->refreshStage, ind->refreshMode, ind->sessionType, session);
    if (!session || ConvertTafSessionTypeToPaSessionType(session->sessionType) != ind->sessionType)
    {
        LE_WARN("InternalRefreshHandler: ignore unmatched refresh event for session:%p", session);
        return;
    }
    auto &sim = taf_sim::GetInstance();
    if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK)
    {
        session->refreshInitFcnPending = false;
        session->refreshFileChangeReported = false;
        session->refreshParticipant = true;
        session->refreshStartHandled = false;
        session->refreshEndHandled = false;
        RefreshTransactionState_t* refreshState = GetRefreshTransactionState(ind->sessionType);
        if (!refreshState)
        {
            LE_WARN("WAIT_FOR_OK for unsupported sessionType=%d", ind->sessionType);
            return;
        }
        if (refreshState->refreshOkSent)
        {
            LE_INFO("RefreshOk already sent for sessionType=%d", ind->sessionType);
            return;
        }
        bool finalAllow = sim.CheckRefreshAllow(ind);
        refreshState->refreshOkSent = true;
        pa_result_t res = taf_pa_sim_RefreshOk(ind->sessionType, &finalAllow);
        LE_INFO("RefreshOk sent for sessionType=%d allow=%d result=%d", ind->sessionType, finalAllow, res);
        if (!finalAllow)
        {
            ResetRefreshTransactionState(refreshState);
            LE_INFO("Refresh denied by client policy; transaction reset for sessionType=%d", ind->sessionType);
        }
        return;
    }
    if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_START)
    {
        bool isFcnLike = (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN) ||
                         (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN) ||
                         (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FULL_FCN);
        if (isFcnLike)
        {
            RefreshTransactionState_t* refreshState = GetRefreshTransactionState(ind->sessionType);
            if (refreshState && !refreshState->refreshCompleteSent)
            {
                if (!session->refreshParticipant)
                {
                    session->refreshParticipant = true;
                }
                if (!session->refreshStartHandled)
                {
                    session->refreshStartHandled = true;
                    refreshState->pendingCompleteCount--;
                    LE_INFO("START FCN-like: pendingCompleteCount=%d for sessionType=%d",
                            refreshState->pendingCompleteCount, ind->sessionType);
                    if (refreshState->pendingCompleteCount <= 0)
                    {
                        refreshState->refreshCompleteSent = true;
                        refreshState->pendingCompleteCount = 0;
                        pa_result_t res = taf_pa_sim_RefreshComplete(ind->sessionType);
                        LE_INFO("RefreshComplete sent (all clients ready) for FCN-like mode=%d sessionType=%d result=%d",
                                ind->refreshMode, ind->sessionType, res);
                    }
                }
                else
                {
                    LE_INFO("START already handled for session:%p", session);
                }
            }
            else
            {
                LE_INFO("RefreshComplete already sent for sessionType=%d", ind->sessionType);
            }
        }
        else if (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_RESET)
        {
            session->refreshParticipant = true;
            session->refreshStartHandled = true;
            session->refreshResetStart = true;
            session->refreshResetDone = false;
            LE_INFO("RESET mode timer started for session:%p", session);
            le_timer_Start(session->refreshTimer);
        }
        return;
    }
    if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_SUCCESS)
    {
        bool isFcnLike = (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN) ||
                         (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FULL_FCN) ||
                         (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN);
        if (isFcnLike)
        {
           if (session->refreshFileChangeReported)
           {
                LE_INFO("FILE_CHANGE already reported for session:%p, skip duplicate mode=%d",session, ind->refreshMode);
                return;
           }
           if (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN)
           {
                LE_WARN("INIT_FCN: no FCN continuation expected → handling immediately for session:%p", session);
                clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
                clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_FILE_CHANGE;
                session->refreshFileChangeReported = true;
                session->refreshInitFcnPending = false;
                le_event_Report(session->RefreshChangeEventId, &clientEvt, sizeof(clientEvt));
                sim.ResetVoteSend(session);
                LE_INFO("INIT_FCN: completed via ResetVoteSend for session:%p", session);
                return;
            }
            session->refreshFileChangeReported = true;
            session->refreshInitFcnPending = false;
            clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
            clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_FILE_CHANGE;
            LE_INFO("File Change Notification (SUCCESS|FILE_CHANGE) for session:%p mode=%d", session, ind->refreshMode);
            le_event_Report(session->RefreshChangeEventId, &clientEvt, sizeof(clientEvt));
            sim.ResetVoteSend(session);
            return;
        }
        clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
        LE_INFO("Notified success: mode=%d for session:%p", ind->refreshMode, session);
        le_event_Report(session->RefreshChangeEventId, &clientEvt, sizeof(clientEvt));
        sim.ResetVoteSend(session);
        return;
    }
    if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE)
    {
        clientEvt.refreshStatus |= TAF_SIM_REFRESH_STATUS_FAILURE;
        session->refreshInitFcnPending = false;
        session->refreshFileChangeReported = false;
        LE_INFO("Notified failure: mode=%d for %p:", ind->refreshMode, session);
        le_event_Report(session->RefreshChangeEventId, &clientEvt, sizeof(clientEvt));
        sim.ResetVoteSend(session);
        return;
    }
}

void taf_sim::RefreshTimeoutHandler(le_timer_Ref_t timerRef)
{
    auto* session =(taf_sim_Session_t*)le_timer_GetContextPtr(timerRef);
    auto &sim = taf_sim::GetInstance();
    if (!session)
    {
        LE_ERROR("RefreshTimeoutHandler: session is NULL");
        return;
    }
    LE_INFO("RESET timeout fired");
    if (!session->refreshResetStart || session->refreshResetDone)
    {
        LE_WARN("Timeout ignored already handled)");
        return;
    }
    session->refreshResetDone = true;
    session->refreshResetStart = false;
    session->refreshInitFcnPending = false;
    session->refreshFileChangeReported = false;
    sim_refresh_event_t evt = {0};
    evt.refreshStatus = TAF_SIM_REFRESH_STATUS_FAILURE;
    le_event_Report(session->RefreshChangeEventId,&evt,sizeof(evt));
    sim.ResetVoteSend(session);
    LE_INFO("Timeout handled failure reported");
}

void taf_sim::HandleRefreshEvent_Queued(void* param1, void* param2)
{
    InternalRefreshEvent_t* evt = static_cast<InternalRefreshEvent_t*>(param1);
    if (!evt)
    {
        LE_ERROR("HandleRefreshEvent_Queued:evt is NULL");
        return;
    }
    LE_INFO("HandleRefreshEvent_Queued: stage=%d mode=%d sessionType=%d",
            evt->ind.refreshStage, evt->ind.refreshMode, evt->ind.sessionType);
    bool isInitFcnContinuation = false;
    std::vector<taf_sim_RefreshRef_t> currentTargets;
    std::vector<taf_sim_RefreshRef_t> targetSessions;
    RefreshTransactionState_t* refreshState = GetRefreshTransactionState(evt->ind.sessionType);
    if (refreshState && refreshState->ignoreLateEvents &&
        evt->ind.refreshStage != TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK)
    {
        LE_WARN("Dropping late refresh event after transaction timeout: stage=%d mode=%d sessionType=%d",
                evt->ind.refreshStage, evt->ind.refreshMode, evt->ind.sessionType);
        le_mem_Release(evt);
        return;
    }
    {
        auto &sim = taf_sim::GetInstance();
        std::lock_guard<std::mutex> lock(sim.eventMutex);
        if (evt->ind.refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN &&
            evt->ind.refreshStage != TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK)
        {
            for (auto* sessionPtr : taf_sim::refresh_client)
            {
                if (sessionPtr &&
                    ConvertTafSessionTypeToPaSessionType(sessionPtr->sessionType) == evt->ind.sessionType &&
                    sessionPtr->refreshInitFcnPending)
                {
                    isInitFcnContinuation = true;
                    break;
                }
            }
        }
        for (auto* sessionPtr : taf_sim::refresh_client)
        {
            if (!sessionPtr || ConvertTafSessionTypeToPaSessionType(sessionPtr->sessionType) != evt->ind.sessionType)
            {
                continue;
            }
            if (!IsClientInterestedInRefresh(sessionPtr, &evt->ind, isInitFcnContinuation))
            {
                LE_INFO("Skip session:%p for refreshMode=%d clientModeMask=0x%x initFcnPending=%d continuation=%d",
                        sessionPtr, evt->ind.refreshMode, (unsigned int)sessionPtr->refreshMode,
                        (int)sessionPtr->refreshInitFcnPending, (int)isInitFcnContinuation);
                continue;
            }
            currentTargets.push_back(sessionPtr->ref);
        }
        if (refreshState && evt->ind.refreshStage == TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK)
        {
            refreshState->ignoreLateEvents = false;
            if (refreshState->transactionActive)
            {
                LE_WARN("New WAIT_FOR_OK while previous refresh transaction is active: oldMode=%d oldParticipants=%zu sessionType=%d. Resetting old transaction.",
                        refreshState->activeMode, refreshState->participantRefs.size(), evt->ind.sessionType);
                ClearRefreshParticipantFlagsLocked(refreshState);
                ResetRefreshTransactionState(refreshState, false);
            }
            refreshState->transactionActive = true;
            refreshState->activeSessionType = evt->ind.sessionType;
            refreshState->activeMode = evt->ind.refreshMode;
            refreshState->participantRefs = currentTargets;
            refreshState->pendingEndCount = (int)refreshState->participantRefs.size();
            refreshState->pendingCompleteCount = (int)refreshState->participantRefs.size();
            targetSessions = refreshState->participantRefs;
            StartRefreshTransactionTimer(refreshState);
            LE_INFO("WAIT_FOR_OK: snapshotCount=%zu pendingEndCount=%d pendingCompleteCount=%d for sessionType=%d",
                    refreshState->participantRefs.size(), refreshState->pendingEndCount,
                    refreshState->pendingCompleteCount, evt->ind.sessionType);
        }
        else if (refreshState && refreshState->transactionActive)
        {
            targetSessions = refreshState->participantRefs;
            LE_INFO("Using refresh participant snapshot: count=%zu stage=%d mode=%d sessionType=%d",
                    targetSessions.size(), evt->ind.refreshStage, evt->ind.refreshMode, evt->ind.sessionType);
        }
        else
        {
            targetSessions = currentTargets;
            if (refreshState && evt->ind.refreshStage == TAF_PA_SIM_REFRESH_STAGE_START &&
                IsFcnLikeMode(evt->ind.refreshMode) && !targetSessions.empty())
            {
                refreshState->transactionActive = true;
                refreshState->activeSessionType = evt->ind.sessionType;
                refreshState->activeMode = evt->ind.refreshMode;
                refreshState->participantRefs = targetSessions;
                refreshState->pendingEndCount = (int)targetSessions.size();
                refreshState->pendingCompleteCount = (int)targetSessions.size();
                StartRefreshTransactionTimer(refreshState);
                LE_INFO("START fallback snapshotCount=%zu for sessionType=%d",
                        targetSessions.size(), evt->ind.sessionType);
            }
        }
    }
    if (evt->ind.refreshStage == TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK && targetSessions.empty())
    {
        bool finalAllow = true;
        pa_result_t res = taf_pa_sim_RefreshOk(evt->ind.sessionType, &finalAllow);
        ResetRefreshTransactionState(refreshState);
        LE_INFO("WAIT_FOR_OK: no interested clients, RefreshOk allow=%d result=%d for sessionType=%d",
                finalAllow, res, evt->ind.sessionType);
        le_mem_Release(evt);
        return;
    }
    for (auto sessionRef : targetSessions)
    {
        taf_sim_Session_t* clientRequestPtr = taf_sim::DiscoverSessionRef(sessionRef);
        if (!clientRequestPtr)
        {
            LE_WARN("HandleRefreshEvent_Queued: sessionRef=%p was removed before dispatch", sessionRef);
            continue;
        }
        InternalRefreshHandler(evt, clientRequestPtr);
    }
    le_mem_Release(evt);
}

void taf_sim::CleanupSession(taf_sim_Session_t* session)
{
    if (!session) return;
    if (session->ref)
    {
        le_ref_DeleteRef(SessionRefMap, session->ref);
        session->ref = NULL;
    }
    if (session->refreshTimer)
    {
        le_timer_Delete(session->refreshTimer);
        session->refreshTimer = NULL;
    }
    le_mem_Release(session);
}

void taf_sim::ResetVoteSend(taf_sim_Session_t* session)
{
    if (!session)
    {
        LE_ERROR("ResetVoteSend called with null session");
        return;
    }
    RefreshTransactionState_t* refreshState = GetRefreshTransactionState(ConvertTafSessionTypeToPaSessionType(session->sessionType));
    if (refreshState)
    {
        if (!refreshState->transactionActive)
        {
            LE_INFO("ResetVoteSend: no active transaction for sessionType=%d", session->sessionType);
            return;
        }
        if (!session->refreshEndHandled)
        {
            session->refreshEndHandled = true;
            if (refreshState->pendingEndCount > 0)
            {
                refreshState->pendingEndCount--;
            }
        }
        LE_INFO("ResetVoteSend: pendingEndCount=%d for sessionType=%d",
                refreshState->pendingEndCount, session->sessionType);
        if (refreshState->pendingEndCount <= 0)
        {
            ResetRefreshTransactionState(refreshState);
            LE_INFO("ResetVoteSend: all clients done, vote/complete state reset for sessionType=%d", session->sessionType);
        }
    }
    else
    {
        LE_WARN("ResetVoteSend: Unhandled sessionType: %d - no vote reset performed",session->sessionType);
    }
}

le_result_t taf_sim::selectSimSlot(taf_sim_Id_t simId) {
    if (!isValidSimId(simId)) {
        LE_WARN("Invalid simId: %d", (int)simId);
        return LE_FAULT;
    }
    pa_result_t paResult = taf_pa_sim_selectSimSlot((taf_pa_sim_Id_t)simId);
    if (paResult == TAF_PA_SIM_RESULT_OK)
    {
        auto &sim = taf_sim::GetInstance();
        sim.slot= simId;
        return LE_OK;
    }
    else if (paResult == TAF_PA_SIM_RESULT_TIMEOUT)
    {
        LE_ERROR("Timeout waiting to select Sim Slot %d", simId);
        return LE_TIMEOUT;
    }
    else
    {
        LE_ERROR("Failed to select Sim Slot %d via PA (Error %d)", simId, paResult);
        return LE_FAULT;
    }
}

le_result_t taf_sim::getICCID(taf_sim_Id_t simId, char *iccid, int length)
{
    std::string iccIdStr = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetIccid((taf_pa_sim_Id_t)simId, iccIdStr);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to get ICCID via PA OSS API for simId %d", simId);
        return LE_FAULT;

    }
    LE_INFO("iccIdStr: %s", iccIdStr.c_str());
    return le_utf8_Copy(iccid, iccIdStr.c_str(), length, NULL);
}

le_result_t taf_sim::getSubscriberPhoneNumber(taf_sim_Id_t simId, char *phoneNumber, int length) {
    string phoneNumberString = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetSubscriberPhoneNumber(
                                 (taf_pa_sim_Id_t) simId,phoneNumberString);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to register subscription listener via PA OSS API.");
        return LE_FAULT;
    }
    LE_INFO("phoneNumberString.c_str()-> %s",phoneNumberString.c_str());
    return le_utf8_Copy(phoneNumber, phoneNumberString.c_str(), length, NULL);
}

le_result_t taf_sim::getIMSI(taf_sim_Id_t simId, char *imsi, int length) {
    string imsiString = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetImsi((taf_pa_sim_Id_t)simId,imsiString);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
       LE_ERROR("Fail to get IMSI via PA OSS API.");
       return LE_FAULT;
    }
    LE_INFO("imsiString.c_str()-> %s",imsiString.c_str());
    return le_utf8_Copy(imsi, imsiString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkOperator(taf_sim_Id_t simId, char *name, int length) {
    string nameString = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetCarrierName((taf_pa_sim_Id_t)simId,nameString);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to get carrier name via PA OSS API.");
        return LE_FAULT;
    }
    LE_INFO("nameString.c_str()-> %s",nameString.c_str());
    return le_utf8_Copy(name, nameString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkMccMnc(taf_sim_Id_t simId, char *mccPtr,
        int mccPtrSize, char *mncPtr, int mncPtrSize)
{
    std::string mcc;
    std::string mnc;
    LE_INFO("getHomeNetworkMccMnc for simId %d", simId);
    if (selectSimSlot(simId) != LE_OK) {
        LE_ERROR("Invalid simId or failed to select SIM slot");
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetHomeNetworkMccMncStr((taf_pa_sim_Id_t)simId, mcc, mnc);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Failed to get HomeNetworkMccMnc via PA for simId %d", simId);
        return LE_FAULT;
    }
    LE_INFO("Retrieved MCC: %s, MNC: %s", mcc.c_str(), mnc.c_str());
    if (mcc.empty() || mnc.empty())
    {
        LE_ERROR("PA returned empty MCC or MNC for simId %d", simId);
        return LE_FAULT;
    }
    if (le_utf8_Copy(mccPtr, mcc.c_str(), mccPtrSize, nullptr) != LE_OK)
    {
        LE_ERROR("Failed to copy MCC to output buffer");
        return LE_FAULT;
    }
    if (le_utf8_Copy(mncPtr, mnc.c_str(), mncPtrSize, nullptr) != LE_OK)
    {
        LE_ERROR("Failed to copy MNC to output buffer");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::UnlockCardByPin(taf_sim_Id_t simId,taf_sim_LockType_t lockType,
        const char* pinPtr)
{
    if(selectSimSlot(simId) != LE_OK)
    {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_UnlockCardByPin((taf_pa_sim_LockType_t)lockType, pinPtr,
                nullptr,std::any());

    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to unlock card by PIN via PA OSS API.");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::ChangeCardPin( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char* oldpinPtr, const char* newpinPtr)
{
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_ChangeCardPin((taf_pa_sim_LockType_t)lockType,oldpinPtr,newpinPtr,
                          nullptr,std::any());
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("fail to change card Pin");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::UnlockCardByPuk(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pukPtr, const char* newpinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_UnlockCardByPuk((taf_pa_sim_LockType_t)lockType,pukPtr,newpinPtr,
                           nullptr,std::any());
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to UnlockCardByPuk via PA OSS API.");
        return LE_FAULT;
    }
    return LE_OK;
}

taf_sim_States_t Utility::Convert::taf_Common_State_Result
(
    taf_pa_sim_States_t state
)
{
    switch (state)
    {
        case TAF_PA_SIM_PRESENT:
            return TAF_SIM_PRESENT;
        case TAF_PA_SIM_ABSENT:
            return TAF_SIM_ABSENT;
        case TAF_PA_SIM_READY:
            return TAF_SIM_READY;
        case TAF_PA_SIM_BLOCKED:
            return TAF_SIM_BLOCKED;
        case TAF_PA_SIM_BUSY:
            return TAF_SIM_BUSY;
        case TAF_PA_SIM_POWER_DOWN:
            return TAF_SIM_POWER_DOWN;
        case TAF_PA_SIM_STATE_UNKNOWN:
            return TAF_SIM_STATE_UNKNOWN;
        case TAF_PA_SIM_RESTRICTED:
            return TAF_SIM_RESTRICTED;
        case TAF_PA_SIM_ERROR:
            return TAF_SIM_ERROR;
        default:
            LE_DEBUG("Unknown state %d.", state);
    }
    return TAF_SIM_STATE_UNKNOWN;
}

le_result_t taf_sim::SetCardLock(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pinPtr, bool lockEnable)
{
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    if(lockEnable)
    {
        pa_result_t paResult = taf_pa_sim_SetCardLock((taf_pa_sim_LockType_t)lockType,pinPtr,
                          nullptr,std::any());
        if (paResult != TAF_PA_SIM_RESULT_OK)
        {
            LE_ERROR("fail to SetCardLock");
            return LE_FAULT;
        }
        return LE_OK;
    }
    else
    {
        pa_result_t paResult =taf_pa_sim_SetCardUnLock((taf_pa_sim_LockType_t)lockType,pinPtr,
                          nullptr,std::any());
        if (paResult != TAF_PA_SIM_RESULT_OK)
        {
            LE_ERROR("fail to SetCardUnLock");
            return LE_FAULT;
        }
        return LE_OK;
    }
}

int32_t taf_sim::GetRemainingPINTries(taf_sim_Id_t simId) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    int32_t retryCount=-1;
    pa_result_t paResult =taf_pa_sim_GetRemainingPINTries((taf_pa_sim_Id_t) simId, &retryCount);
    if(paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Failed GetRemainingPINTries");
        return retryCount;
    }
    return retryCount;
}

le_result_t taf_sim::GetRemainingPukTries(taf_sim_Id_t simId,uint32_t* remainingPukTriesPtr)
{
    if (!remainingPukTriesPtr) {
        LE_ERROR("remainingPukTriesPtr is NULL");
        return LE_BAD_PARAMETER;
    }

    if (selectSimSlot(simId) != LE_OK) {
        LE_ERROR("Failed to select SIM slot %d", simId);
        return LE_BAD_PARAMETER;
    }

    uint32_t remainingPukTries = 0;
    pa_result_t paResult = taf_pa_sim_GetRemainingPukTries((taf_pa_sim_Id_t) simId,&remainingPukTries);

    le_result_t result = Utility::Convert::Result(paResult);

    if (result == LE_OK) {
        *remainingPukTriesPtr = remainingPukTries;
        LE_INFO("Remaining PUK tries for simId %d: %u",simId, remainingPukTries);
    } else {
        LE_WARN("Failed to get remaining PUK tries for simId %d (paResult=%d)",
                simId, paResult);
    }
    return result;
}

taf_sim_AuthenticationResponseHandlerRef_t taf_sim::AddAuthenticationResponseHandler(
        taf_sim_AuthenticationResponseHandlerFunc_t handlerPtr, void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add AuthenticationResponseHandler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("AuthenticationResponseHandler",
            ResponseEventId,
            FirstLayerAuthenticationResponseHandler,
            (void*)handlerPtr);

    return (taf_sim_AuthenticationResponseHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveAuthenticationResponseHandler(taf_sim_AuthenticationResponseHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerAuthenticationResponseHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    sim_response_event_t* simResponsePtr = (sim_response_event_t*)reportPtr;
    if (!simResponsePtr)
    {
        LE_ERROR("Null pointer provided!");
        return;
    }
    LE_INFO("FirstLayerNewSimStateHandler simId = %d", simResponsePtr->simId);
    taf_sim_AuthenticationResponseHandlerFunc_t clientHandlerFunc =
        (taf_sim_AuthenticationResponseHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simResponsePtr->simId, simResponsePtr->responseType,
            simResponsePtr->result, le_event_GetContextPtr());
}

le_result_t taf_sim::GetEID(taf_sim_Id_t simId, char *eidPtr, size_t eidLen)
{
    eidPtr[0] = '\0';
    std::string eidStr;
    pa_result_t paResult =taf_pa_sim_GetEID((taf_pa_sim_Id_t) simId, eidStr);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK) {
        LE_ERROR("taf_pa_sim_GetEID failed for simId: %d, result: %d", simId, result);
        return LE_FAULT;
    }
    if (eidStr.size() + 1 > eidLen) {
        LE_ERROR("EID buffer overflow for simId: %d, required: %zu, provided: %zu",simId, eidStr.size() + 1, eidLen);
        return LE_OVERFLOW;
    }
    le_utf8_Copy(eidPtr, eidStr.c_str(), eidLen, nullptr);
    LE_INFO("taf_sim::GetEID successful for simId: %d, EID: %s", simId, eidPtr);
    return LE_OK;
}

le_result_t taf_sim::SetAutomaticSelection( bool enable) {
    EnableAutoSelection = enable;
    return LE_OK;
}

le_result_t taf_sim::GetAutomaticSelection( bool* enablePtr) {
    *enablePtr = EnableAutoSelection;
    return LE_OK;
}

le_result_t taf_sim::GetAppTypes(taf_sim_Id_t slotId, taf_sim_AppType_t* appTypePtr, size_t* appTypeNumElementsPtr) {
    *appTypeNumElementsPtr = 0;
    if (selectSimSlot(slotId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }
    pa_result_t paResult = taf_pa_sim_GetAppTypes((taf_pa_sim_AppType_t*)appTypePtr,appTypeNumElementsPtr);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Fail to GetAppTypes via PA OSS API.");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::OpenLogicalChannel( taf_sim_Id_t simId, taf_sim_AppType_t appType, uint8_t* channelPtr) {
    if (channelPtr == nullptr)
    {
        LE_INFO("channelPtr is null");
        return LE_BAD_PARAMETER;
    }

    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }
    taf_pa_sim_AppType_t  paAppType = ConvertTafappTypeToPaappType(appType);
    if(paAppType != TAF_PA_APPTYPE_UNKNOWN)
    {
        pa_result_t paResult = taf_pa_sim_OpenLogicalChannel(paAppType,channelPtr,nullptr,{});
        le_result_t result =Utility::Convert::Result(paResult);
        return result;
    }
    return LE_FAULT;
}

le_result_t taf_sim::OpenLogicalChannelByAid( taf_sim_Id_t simId, const char* aid, uint8_t* channelPtr) {
    if (channelPtr == nullptr)
    {
        LE_INFO("channelPtr is null");
        return LE_BAD_PARAMETER;
    }
    if (aid == nullptr || aid[0] == '\0')
    {
        LE_INFO("Invalid aid");
        return LE_BAD_PARAMETER;
    }
    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }
    pa_result_t paResult = taf_pa_sim_OpenLogicalChannelByAid(aid,channelPtr,nullptr,{});
    le_result_t result =Utility::Convert::Result(paResult);
    return result;
}

le_result_t taf_sim::CloseLogicalChannel( taf_sim_Id_t simId, uint8_t channel) {
    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    pa_result_t paResult = taf_pa_sim_CloseLogicalChannel(channel,nullptr,{});
    le_result_t result =Utility::Convert::Result(paResult);
    return result;
}

le_result_t taf_sim::SendApduOnChannel( taf_sim_Id_t simId, uint8_t channel,
            const uint8_t* commandApduPtr, size_t commandApduNumElements,
             uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr){

    if ((commandApduPtr == nullptr) ||(responseApduPtr == nullptr) ||
            (responseApduNumElementsPtr == nullptr))
    {
        return LE_BAD_PARAMETER;
    }

    if (commandApduNumElements < 5)
    {
        LE_ERROR("Invalid APDU length");
        return LE_BAD_PARAMETER;
    }

    uint8_t cla, instruction, p1, p2, p3;
    std::vector<uint8_t> data;
    cla = commandApduPtr[0];
    instruction = commandApduPtr[1];
    p1 = commandApduPtr[2];
    p2 = commandApduPtr[3];
    p3 = commandApduPtr[4];

    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    LE_DEBUG("SendApduOnChannel: Data size(p3) = %d and commandApduNumElements: %d,channel id: %d", (int)p3, (int)commandApduNumElements,channel);

    if (commandApduNumElements > 5) {
        for(int i = 0; i < p3; i++) {
          data.emplace_back(commandApduPtr[i+ 5]);
        }
    }

    pa_result_t paResult = taf_pa_sim_SendApduOnLogicalChannel(channel,responseApduPtr,responseApduNumElementsPtr,
                            p1,p2,p3,cla,instruction,data,nullptr,{});
    le_result_t result =Utility::Convert::Result(paResult);
    return result;
}

le_result_t taf_sim::SendApdu( taf_sim_Id_t simId,const uint8_t* commandApduPtr, size_t commandApduNumElements,
             uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr){
    if ((commandApduPtr == nullptr) ||(responseApduPtr == nullptr) ||
            (responseApduNumElementsPtr == nullptr))
    {
        return LE_BAD_PARAMETER;
    }

    if (commandApduNumElements < 5)
    {
        LE_ERROR("Invalid APDU length");
        return LE_BAD_PARAMETER;
    }

    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    uint8_t cla, instruction, p1, p2, p3;
    std::vector<uint8_t> data;
    cla = commandApduPtr[0];
    instruction = commandApduPtr[1];
    p1 = commandApduPtr[2];
    p2 = commandApduPtr[3];
    p3 = commandApduPtr[4];

    if (commandApduNumElements > 5) {
       for(int i = 0; i < p3; i++) {
           data.emplace_back(commandApduPtr[i+ 5]);
        }
    }
    pa_result_t paResult = taf_pa_sim_SendApdu(responseApduPtr,responseApduNumElementsPtr, p1, p2, p3, cla, instruction,data,
         nullptr,{});
    le_result_t result =Utility::Convert::Result(paResult);
    return result;
}

le_result_t taf_sim::SendCommand(
    taf_sim_Id_t simId, taf_sim_Command_t command,
    const char* fileIdentifierPtr,
    uint8_t *p1, uint8_t *p2,
    uint8_t *p3,     const uint8_t* dataPtr,
    size_t dataNumElements,const char* pathPtr,
    uint8_t *sw1,uint8_t *sw2,
    uint8_t* responsePtr, size_t* responseNumElementsPtr
)
{
    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Issue with simId");
        return LE_NOT_FOUND;
    }
    TAF_ERROR_IF_RET_VAL(fileIdentifierPtr == NULL, LE_BAD_PARAMETER, "fileIdentifierPtr is NULL");
    TAF_ERROR_IF_RET_VAL(p1 == NULL, LE_BAD_PARAMETER, "p1 is NULL");
    TAF_ERROR_IF_RET_VAL(p2 == NULL, LE_BAD_PARAMETER, "p2 is NULL");
    TAF_ERROR_IF_RET_VAL(p3 == NULL, LE_BAD_PARAMETER, "p3 is NULL");
    TAF_ERROR_IF_RET_VAL(sw1 == NULL, LE_BAD_PARAMETER, "sw1 is NULL");
    TAF_ERROR_IF_RET_VAL(sw2 == NULL, LE_BAD_PARAMETER, "sw2 is NULL");
    TAF_ERROR_IF_RET_VAL(responsePtr == NULL, LE_BAD_PARAMETER, "responsePtr is NULL");
    TAF_ERROR_IF_RET_VAL(responseNumElementsPtr == NULL, LE_BAD_PARAMETER, "responseNumElementsPtr is NULL");

    if ((dataNumElements > 0) && (dataPtr == NULL))
    {
        LE_ERROR("dataPtr is NULL while dataNumElements > 0");
        return LE_BAD_PARAMETER;
    }
    char *endPtr = nullptr;
    unsigned long value = std::strtoul(fileIdentifierPtr, &endPtr, 16);

    if ((endPtr == fileIdentifierPtr) || (*endPtr != '\0') || (value > 0xFFFF))
    {
        LE_ERROR("Invalid file identifier: %s", fileIdentifierPtr);
        return LE_BAD_PARAMETER;
    }

    uint16_t field = static_cast<uint16_t>(value);
    LE_INFO("field: %u", field);

    pa_result_t paResult = taf_pa_sim_ExchangeSimIO(static_cast<taf_pa_sim_Command_t>(command),p1,p2,p3,dataPtr,
        dataNumElements,(pathPtr != nullptr) ? pathPtr : "",sw1,sw2,responsePtr,responseNumElementsPtr,field,nullptr,{});

    return Utility::Convert::Result(paResult);
}

le_result_t taf_sim::SetPower(taf_sim_Id_t simId, le_onoff_t powerState)
{
    if (selectSimSlot(simId) != LE_OK)
    {
        LE_INFO("Sim ID %d Invalid", simId);
        return LE_BAD_PARAMETER;
    }
    if(!(powerState==LE_OFF || powerState==LE_ON))
    {
        LE_INFO("Invalid powerState given %d", powerState);
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_SetPower((taf_pa_sim_Id_t)simId,(taf_pa_sim_power_state_t)powerState);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Set Power operation failed,simId %d , powerState %d",
                simId, powerState);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::Reset(taf_sim_Id_t simId)
{
    LE_INFO("Resetting sim card");
    le_result_t r=SetPower(simId, LE_OFF);
    if(r!=LE_OK){
        LE_INFO("Powering off while resetting failed");
        return LE_FAULT;
    }
    r = SetPower(simId, LE_ON);
    if(r!=LE_OK){
        LE_INFO("Powering on while resetting failed");
        return LE_FAULT;
    }
    LE_INFO("SIM Reset successful");
    return LE_OK;
}

le_result_t taf_sim::MapSimIdToPaSlot
(
    taf_sim_Id_t simId,
    taf_pa_sim_SlotId_t *slotOut
)
{
    if (!slotOut) return LE_BAD_PARAMETER;
    switch (simId) {
        case TAF_SIM_EXTERNAL_SLOT_1:
            *slotOut = TAF_PA_SIM_SLOT_1;
            return LE_OK;
        case TAF_SIM_EXTERNAL_SLOT_2:
        {
            int slotCount =1;
            auto &sim = taf_sim::GetInstance();
            le_result_t result = sim.getSlotCount(&slotCount);
            if(result != LE_OK)
            {
                LE_INFO("Fail to get slot count via PA OSS API.");
            }
            if (isSingleActive)
            {
                *slotOut = TAF_PA_SIM_SLOT_1;
                return LE_OK;
            } else {
                LE_ERROR("MapSimIdToPaSlot: not supported on this slot");
                return LE_BAD_PARAMETER;
            }
        }
        default:
            LE_ERROR("MapSimIdToPaSlot: invalid simId=%d", (int)simId);
            return LE_BAD_PARAMETER;
    }
}

bool taf_sim::FindProfileByType(taf_pa_sim_SlotId_t paSlot,
                              taf_pa_sim_ProfileType_t wantType,
                              taf_pa_sim_ProfileInfo_t* outInfo)
{
    uint8_t n = 0;
    pa_result_t res = taf_pa_sim_GetProfileNum(paSlot, &n);
    if (res != PA_OK)
    {
        LE_WARN("taf_pa_sim_GetProfileNum returned: %d", (int)res);
        return false;
    }
    for (uint8_t i = 0; i < n; ++i) {
        taf_pa_sim_ProfileInfo_t info = {};
        pa_result_t getRes = taf_pa_sim_GetProfile(paSlot, i, &info);
        if (getRes != PA_OK)
        {
            LE_WARN("taf_pa_sim_GetProfile[%u] returned: %d", i, (int)getRes);
            continue;
        }
        if (info.type == wantType) {
            if (outInfo) {
                *outInfo = info;
            }
            return true;
        }
    }
    return false;
}

le_result_t taf_sim::IsEmergencyCallSubscriptionSelected
(
    taf_sim_Id_t simId,
    bool *isEcs
)
{
    if (!isEcs)
    {
        LE_ERROR("IsEmergencyCallSubscriptionSelected: isEcs is NULL");
        return LE_BAD_PARAMETER;
    }

    *isEcs = false;

    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_BAD_PARAMETER;
    }

    taf_pa_sim_SlotId_t paSlot;
    if (MapSimIdToPaSlot(simId, &paSlot) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    uint8_t profileCount = 0;
    pa_result_t profileNumRes = taf_pa_sim_GetProfileNum(paSlot, &profileCount);
    if (profileNumRes != PA_OK)
    {
        LE_WARN("IsEmergencyCallSubscriptionSelected: taf_pa_sim_GetProfileNum returned: %d", (int)profileNumRes);
        return LE_FAULT;
    }
    if (profileCount == 0) {
        LE_INFO("IsEmergencyCallSubscriptionSelected: profiles list is empty");
        return LE_FAULT;
    }

    taf_pa_sim_ProfileInfo_t emInfo;
    bool hasEmergency = FindProfileByType(paSlot, TAF_PA_SIM_PROFILE_TYPE_EMERGENCY, &emInfo);
    if (!hasEmergency) {
        LE_ERROR("IsEmergencyCallSubscriptionSelected: no EMERGENCY profile found (count=%u)", profileCount);
        return LE_FAULT;
    }
    *isEcs = (emInfo.state == TAF_PA_SIM_PROFILE_STATE_ACTIVE);
    return LE_OK;
}

uint8_t taf_sim::CharToDigit(char c)
{
    return (c >= '0' && c <= '9') ? (c - '0') : 0xF;
}

bool taf_sim::DecodePlmnBytes(const uint8_t* data, char* mccStr, char* mncStr)
{
    uint8_t b1 = data[0];
    uint8_t b2 = data[1];
    uint8_t b3 = data[2];

    if (b1 == 0xFF && b2 == 0xFF && b3 == 0xFF) return false;

    mccStr[0] = (char)((b1 & 0x0F) + '0');
    mccStr[1] = (char)(((b1 & 0xF0) >> 4) + '0');
    mccStr[2] = (char)((b2 & 0x0F) + '0');
    mccStr[3] = '\0';

    uint8_t mnc3_val = (b2 & 0xF0) >> 4;
    uint8_t mnc1_val = (b3 & 0x0F);
    uint8_t mnc2_val = (b3 & 0xF0) >> 4;

    mncStr[0] = (char)(mnc1_val + '0');
    mncStr[1] = (char)(mnc2_val + '0');

    if (mnc3_val != 0xF) {
        mncStr[2] = (char)(mnc3_val + '0');
        mncStr[3] = '\0';
    } else {
        mncStr[2] = '\0';
    }
    return true;
}

void taf_sim::EncodePlmnBytes(const char* mccStr, const char* mncStr, uint8_t* outData)
{
    uint8_t mcc[3];
    uint8_t mnc[3] = {0xF, 0xF, 0xF};

    for(int i=0; i<3 && mccStr[i]; i++) mcc[i] = CharToDigit(mccStr[i]);
    for(int i=0; i<3 && mncStr[i]; i++) mnc[i] = CharToDigit(mncStr[i]);

    outData[0] = (mcc[1] << 4) | mcc[0];
    outData[1] = (mnc[2] << 4) | mcc[2];
    outData[2] = (mnc[1] << 4) | mnc[0];
}

bool taf_sim::GetFileSizeFromFcp(const uint8_t* fcp, size_t fcpLen, uint16_t* fileSizePtr)
{
    if (!fcp || fcpLen < 2 || !fileSizePtr) return false;
    size_t i = 0;
    if (fcp[0] == 0x62) i = 2;

    while (i + 2 <= fcpLen) {
        uint8_t tag = fcp[i++];
        uint8_t len = fcp[i++];
        if (i + len > fcpLen) break;
        if ((tag == 0x80 || tag == 0x81) && len >= 2) {
            *fileSizePtr = ((uint16_t)fcp[i] << 8) | fcp[i + 1];
            return true;
        }
        i += len;
    }
    return false;
}

le_result_t taf_sim::SelectFileAndGetFCP(
    taf_sim_Id_t simId,
    uint8_t channel,
    const uint8_t* fileId,
    uint16_t* outFileSize
)
{
    uint8_t selectApdu[] = { channel, 0xA4, 0x08, 0x04, 0x04, 0x7F, 0xFF, fileId[0], fileId[1]};
    uint8_t respBuf[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t  respLen = 0;

    le_result_t res = SendApduOnChannel(simId, channel, selectApdu, sizeof(selectApdu), respBuf, &respLen);
    if (res != LE_OK || respLen < 2) return LE_FAULT;

    uint8_t sw1 = respBuf[respLen - 2];
    uint8_t sw2 = respBuf[respLen - 1];

    uint8_t* fcpPtr = NULL;
    size_t   fcpLen = 0;

    if (sw1 == 0x90 && sw2 == 0x00)
    {
        if (respLen > 2) {
            fcpPtr = respBuf;
            fcpLen = respLen - 2;
        }
    }
    else if (sw1 == 0x61)
    {
        uint8_t le = sw2;
        uint8_t getRespApdu[] = { channel, 0xC0, 0x00, 0x00, le };

        respLen = sizeof(respBuf);
        res = SendApduOnChannel(simId, channel, getRespApdu, sizeof(getRespApdu), respBuf, &respLen);
        if (res == LE_OK && respLen >= 2) {
            uint8_t grSw1 = respBuf[respLen - 2];
            uint8_t grSw2 = respBuf[respLen - 1];
            if (grSw1 == 0x90 && grSw2 == 0x00) {
                fcpPtr = respBuf;
                fcpLen = respLen - 2;
            } else {
                LE_ERROR("GetFCP: GET RESP SW=%02X%02X", grSw1, grSw2);
                return LE_FAULT;
            }
        } else {
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("SelectFile: Failed SW=%02X%02X", sw1, sw2);
        return LE_FAULT;
    }

    if (fcpPtr && outFileSize) {
        if (!GetFileSizeFromFcp(fcpPtr, fcpLen, outFileSize)) {
            *outFileSize = 0;
        }
    }

    return LE_OK;
}

taf_sim_FPLMNListRef_t taf_sim::CreateFPLMNList(void)
{
    taf_sim_FPLMNList_t* listRef = (taf_sim_FPLMNList_t*)le_mem_ForceAlloc(FPLMNListPool);
    listRef->list = LE_DLS_LIST_INIT;
    listRef->iter = NULL;
    listRef->ref  = (taf_sim_FPLMNListRef_t)le_ref_CreateRef(FPLMNListRefMap, listRef);
    return (taf_sim_FPLMNListRef_t)(listRef->ref);
}

le_result_t taf_sim::AddFPLMNOperator(taf_sim_FPLMNListRef_t FPLMNListRef, char* mccPtr, char* mncPtr)
{
    if (!mccPtr || !mncPtr) {
        LE_INFO("Invalid input");
        return LE_BAD_PARAMETER;
    }

    if (!IsValidMCCAndMNC(mccPtr, mncPtr))
    {
        LE_ERROR("Invalid MCC/MNC %s/%s", mccPtr, mncPtr);
        return LE_BAD_PARAMETER;
    }

    taf_sim_FPLMNList_t* listRef = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (!listRef) return LE_FAULT;

    FPLMNNode_t* node = (FPLMNNode_t*)le_mem_ForceAlloc(FPLMNNodePool);
    if (!node) return LE_FAULT;

    le_utf8_Copy(node->mcc, mccPtr, sizeof(node->mcc), NULL);
    le_utf8_Copy(node->mnc, mncPtr, sizeof(node->mnc), NULL);
    node->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&listRef->list, &node->link);
    return LE_OK;
}

le_result_t taf_sim::GetFirstFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr, size_t mccLen,
    char* mncPtr, size_t mncLen
)
{
    if (!mccPtr || !mncPtr) {
        LE_ERROR("GetFirstFPLMNOperator: MCC/MNC buffer NULL");
        return LE_BAD_PARAMETER;
    }

    taf_sim_FPLMNList_t* listRef = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (!listRef) {
        LE_ERROR("GetFirstFPLMNOperator: invalid listRef");
        return LE_FAULT;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&listRef->list);
    if (!linkPtr) {
        return LE_NOT_FOUND;
    }

    FPLMNNode_t* node = CONTAINER_OF(linkPtr, FPLMNNode_t, link);

    le_result_t res;
    res = le_utf8_Copy(mccPtr, node->mcc, mccLen, NULL);
    if (res != LE_OK) return res;
    res = le_utf8_Copy(mncPtr, node->mnc, mncLen, NULL);
    if (res != LE_OK) return res;

    listRef->iter = linkPtr;
    return LE_OK;
}

le_result_t taf_sim::GetNextFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr, size_t mccLen,
    char* mncPtr, size_t mncLen
)
{
    if (!mccPtr || !mncPtr) {
        LE_ERROR("GetNextFPLMNOperator: MCC/MNC buffer NULL");
        return LE_BAD_PARAMETER;
    }

    taf_sim_FPLMNList_t* listRef = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (!listRef) {
        LE_ERROR("GetNextFPLMNOperator: invalid listRef");
        return LE_FAULT;
    }

    if (!listRef->iter) {
        return LE_NOT_FOUND;
    }

    le_dls_Link_t* next = le_dls_PeekNext(&listRef->list, listRef->iter);
    if (!next) {
        return LE_NOT_FOUND;
    }

    FPLMNNode_t* node = CONTAINER_OF(next, FPLMNNode_t, link);

    le_result_t res;
    res = le_utf8_Copy(mccPtr, node->mcc, mccLen, NULL);
    if (res != LE_OK) return res;
    res = le_utf8_Copy(mncPtr, node->mnc, mncLen, NULL);
    if (res != LE_OK) return res;

    listRef->iter = next;
    return LE_OK;
}

void taf_sim::DeleteFPLMNList(taf_sim_FPLMNListRef_t FPLMNListRef)
{
    taf_sim_FPLMNList_t* listRef = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (!listRef) return;

    while (!le_dls_IsEmpty(&listRef->list))
    {
        le_dls_Link_t* link = le_dls_Peek(&listRef->list);
        le_dls_Remove(&listRef->list, link);

        FPLMNNode_t* node = CONTAINER_OF(link, FPLMNNode_t, link);
        le_mem_Release(node);
    }

    listRef->iter = NULL;
    le_ref_DeleteRef(FPLMNListRefMap, FPLMNListRef);
    le_mem_Release(listRef);
}

taf_sim_FPLMNListRef_t taf_sim::ReadFPLMNList(taf_sim_Id_t simId)
{
    taf_sim_FPLMNListRef_t listRef = CreateFPLMNList();
    if (!listRef) return NULL;

    if (selectSimSlot(simId) != LE_OK) return NULL;

    uint8_t channel = 0;
    if (OpenLogicalChannel(simId, TAF_SIM_APPTYPE_USIM, &channel) != LE_OK) return NULL;

    uint8_t fileId[] = { 0x6F, 0x7B };
    uint16_t fileSize = 0;

    if (SelectFileAndGetFCP(simId, channel, fileId, &fileSize) != LE_OK) {
        LE_ERROR("ReadFPLMNList: Select Failed");
        CloseLogicalChannel(simId, channel);
        return NULL;
    }

    uint8_t p3 = 0x00;
    if (fileSize > 0 && fileSize < 256) {
        p3 = (uint8_t)fileSize;
    }

    uint8_t readBinaryApdu[] = { channel, 0xB0, 0x00, 0x00, p3 };
    uint8_t respBuf[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t  respLen = sizeof(respBuf);

    le_result_t res = SendApduOnChannel(simId, channel, readBinaryApdu, sizeof(readBinaryApdu), respBuf, &respLen);

    CloseLogicalChannel(simId, channel);

    if (res != LE_OK || respLen < 2) {
        LE_ERROR("ReadFPLMNList: APDU Send Failed");
        return NULL;
    }

    uint8_t sw1 = respBuf[respLen - 2];
    uint8_t sw2 = respBuf[respLen - 1];

    if (sw1 != 0x90 || sw2 != 0x00) {
        LE_ERROR("ReadFPLMNList: READ BINARY SW=%02X%02X", sw1, sw2);
        return NULL;
    }

    size_t payloadLen = respLen - 2;
    if (payloadLen < 3) {
        return CreateFPLMNList();
    }

    uint32_t nodeCount = 0;

    for (size_t i = 0; i + 3 <= payloadLen; i += 3)
    {
        uint8_t b1 = respBuf[i];
        uint8_t b2 = respBuf[i+1];
        uint8_t b3 = respBuf[i+2];

        if (b1 == 0xFF && b2 == 0xFF && b3 == 0xFF) {
            LE_INFO("ReadFPLMNList: skip FF entry at index %zu", i/3);
            continue;
        }

        char mcc[4] = {0};
        char mnc[4] = {0};

        if (!DecodePlmnBytes(&respBuf[i], mcc, mnc)) {
            continue;
        }

        if (!IsValidMCCAndMNC(mcc, mnc))
        {
            LE_WARN("ReadFPLMNList: invalid decoded MCC/MNC %s/%s, skip", mcc, mnc);
            continue;
        }

        if (nodeCount >= TAF_SIM_FPLMN_MAX_OPERATORS_PER_LIST) {
            LE_WARN("ReadFPLMNList: Limit reached (%u), truncating rest", TAF_SIM_FPLMN_MAX_OPERATORS_PER_LIST);
            break;
        }

        if (AddFPLMNOperator(listRef, mcc, mnc) != LE_OK) {
            LE_ERROR("ReadFPLMNList: Add failed");
            DeleteFPLMNList(listRef);
            return NULL;
        }

        nodeCount++;
        LE_INFO("ReadFPLMNList: Found #%u %s/%s", nodeCount, mcc, mnc);
    }

    return listRef;
}

le_result_t taf_sim::ClearFPLMNToFF(taf_sim_Id_t simId)
{
    LE_INFO("ClearFPLMNToFF: Start");
    if (selectSimSlot(simId) != LE_OK) return LE_FAULT;

    uint8_t channel = 0;
    if (OpenLogicalChannel(simId, TAF_SIM_APPTYPE_USIM, &channel) != LE_OK) return LE_FAULT;

    uint8_t fileId[] = { 0x6F, 0x7B };
    uint16_t fileSize = 0;

    if (SelectFileAndGetFCP(simId, channel, fileId, &fileSize) != LE_OK) {
        CloseLogicalChannel(simId, channel);
        return LE_FAULT;
    }

    uint16_t writeLen = (fileSize > 0 && fileSize < 255) ? fileSize : 255;
    std::vector<uint8_t> data(writeLen, 0xFF);

    std::vector<uint8_t> apdu;
    apdu.reserve(5 + data.size());
    apdu.push_back(channel);
    apdu.push_back(0xD6);
    apdu.push_back(0x00);
    apdu.push_back(0x00);
    apdu.push_back((uint8_t)data.size());
    apdu.insert(apdu.end(), data.begin(), data.end());

    uint8_t resp[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t  rLen = sizeof(resp);
    le_result_t res = SendApduOnChannel(simId, channel, apdu.data(), apdu.size(), resp, &rLen);

    CloseLogicalChannel(simId, channel);
    return res;
}

le_result_t taf_sim::WriteFPLMNList
(
    taf_sim_Id_t           simId,
    taf_sim_FPLMNListRef_t FPLMNListRef
)
{
    taf_sim_FPLMNList_t* listPtr = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (!listPtr) {
        LE_ERROR("WriteFPLMNList: invalid listRef");
        return LE_FAULT;
    }

    if (le_dls_IsEmpty(&listPtr->list))
    {
        LE_INFO("WriteFPLMNList: list empty, redirecting to ClearFPLMNToFF");
        return ClearFPLMNToFF(simId);
    }

    if (selectSimSlot(simId) != LE_OK) return LE_FAULT;

    uint8_t channel = 0;
    if (OpenLogicalChannel(simId, TAF_SIM_APPTYPE_USIM, &channel) != LE_OK) return LE_FAULT;

    uint8_t fileId[] = { 0x6F, 0x7B };
    uint16_t fileSize = 0;

    if (SelectFileAndGetFCP(simId, channel, fileId, &fileSize) != LE_OK) {
        LE_ERROR("WriteFPLMNList: Select Failed");
        CloseLogicalChannel(simId, channel);
        return LE_FAULT;
    }

    if (fileSize == 0) {
        LE_WARN("WriteFPLMNList: Unknown fileSize, defaulting to 0 behavior");
    }

    uint32_t listCount = 0;
    for (le_dls_Link_t* l = le_dls_Peek(&listPtr->list); l != NULL; l = le_dls_PeekNext(&listPtr->list, l)) {
        ++listCount;
    }

    uint32_t maxRecords = (fileSize > 0) ? (fileSize / 3) : listCount;
    uint32_t recordsToWrite = (listCount < maxRecords) ? listCount : maxRecords;

    LE_INFO("WriteFPLMNList: listCount=%u fileSize=%u maxRecords=%u toWrite=%u",
            listCount, fileSize, maxRecords, recordsToWrite);

    std::vector<uint8_t> data;
    data.reserve(fileSize > 0 ? fileSize : (3 * recordsToWrite));

    uint32_t writtenRecords = 0;
    for (le_dls_Link_t* nodeLink = le_dls_Peek(&listPtr->list);
         nodeLink != NULL && writtenRecords < recordsToWrite;
         nodeLink = le_dls_PeekNext(&listPtr->list, nodeLink))
    {
        FPLMNNode_t* node = CONTAINER_OF(nodeLink, FPLMNNode_t, link);

        uint8_t encoded[3];
        EncodePlmnBytes(node->mcc, node->mnc, encoded);

        data.push_back(encoded[0]);
        data.push_back(encoded[1]);
        data.push_back(encoded[2]);

        ++writtenRecords;
    }

    if (fileSize > 0)
    {
        if (data.size() < fileSize) {
            data.resize(fileSize, 0xFF);
        } else if (data.size() > fileSize) {
            data.resize(fileSize);
        }
    }

    if (data.empty() || data.size() > 255)
    {
        LE_ERROR("WriteFPLMNList: Data size %zu invalid for Short APDU (Max 255)", data.size());
        CloseLogicalChannel(simId, channel);
        return LE_FAULT;
    }

    std::vector<uint8_t> writeApdu;
    writeApdu.reserve(5 + data.size());
    writeApdu.push_back(channel);
    writeApdu.push_back(0xD6);
    writeApdu.push_back(0x00);
    writeApdu.push_back(0x00);
    writeApdu.push_back((uint8_t)data.size());
    writeApdu.insert(writeApdu.end(), data.begin(), data.end());

    uint8_t respBuf[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t respLen = sizeof(respBuf);

    le_result_t res = SendApduOnChannel(simId, channel, writeApdu.data(), writeApdu.size(), respBuf, &respLen);
    if (res != LE_OK || respLen < 2) {
        CloseLogicalChannel(simId, channel);
        return LE_FAULT;
    }

    uint8_t sw1 = respBuf[respLen - 2];
    uint8_t sw2 = respBuf[respLen - 1];
    CloseLogicalChannel(simId, channel);

    if (sw1 == 0x90 && sw2 == 0x00) {
        return LE_OK;
    } else {
        LE_ERROR("WriteFPLMNList: Write Failed SW=%02X%02X", sw1, sw2);
        return LE_FAULT;
    }
}

le_result_t taf_sim::getSlotCount(int *count) {
    bool isReady;
    if (count == NULL) {
        LE_ERROR("GetSlotCount failed! as count is NULL");
        return LE_FAULT;
    }
    pa_result_t result = taf_pa_sim_IsSubsystemReady(&isReady);
    *count = 1; //Single SIM by default
    if(result == TAF_PA_SIM_RESULT_OK)
    {
        if (isReady)
        {
            int slotCount;
            pa_result_t paResult = taf_pa_sim_getSlotCount(&slotCount);
            if (paResult != TAF_PA_SIM_RESULT_OK)
            {
                LE_ERROR("Fail to get slot count via PA OSS API.");
                return LE_FAULT;
            }
            else
            {
                LE_INFO("getSlotCount: success, Slot Count: %d", slotCount);
                *count = slotCount;
                if(*count == 1)
                {
                    isSingleActive = true;
                }
                return LE_OK;
            }
        }
    }
    LE_ERROR("GetSlotCount failed because multi sim sub system is not ready");
    return LE_FAULT;
}

bool taf_sim::IsValidMCCAndMNC(const char* mccPtr, const char* mncPtr)
{
    if (mccPtr == NULL || mncPtr == NULL) {
        LE_INFO("MCC or MNC pointer is NULL");
        return false;
    }
    // Validate MCC
    if (strlen(mccPtr) != 3) {
        LE_INFO("Invalid MCC length");
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        if (!isdigit(mccPtr[i])) {
            LE_DEBUG("Invalid MCC digit %c", mccPtr[i]);
            return false;
        }
    }
    // Validate MNC
    size_t len = strlen(mncPtr);
    if (len != 2 && len != 3) {
        LE_INFO("Invalid MNC length");
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!isdigit(mncPtr[i])) {
            LE_INFO("Invalid MNC digit %c",mncPtr[i]);
            return false;
        }
    }
    LE_INFO("Valid MCC:%s and MNC:%s", mccPtr,mncPtr);
    return true;
}

le_result_t taf_sim::SwapSubscriptionInternal
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer,
    bool toEmergency
)
{
    if (manufacturer == TAF_SIM_MORPHO || manufacturer == TAF_SIM_VALID
            || manufacturer >= TAF_SIM_MANUFACTURER_MAX)
    {
        return LE_UNSUPPORTED;
    }
    if (toEmergency) {
        if (selectSimSlot(simId) != LE_OK) {
            return LE_BAD_PARAMETER;
        }
    }

    taf_pa_sim_SlotId_t paSlot;
    if (MapSimIdToPaSlot(simId, &paSlot) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    uint8_t profileCount = 0;
    pa_result_t profileNumRes2 = taf_pa_sim_GetProfileNum(paSlot, &profileCount);
    if (profileNumRes2 != PA_OK)
    {
        LE_WARN("taf_pa_sim_GetProfileNum returned: %d", (int)profileNumRes2);
        return LE_FAULT;
    }
    if (profileCount == 0) {
        LE_INFO("profiles list is empty");
        return LE_FAULT;
    }

    if (profileCount != 2) {
        LE_WARN("non-typical profileCount=%u (expected 2 for many proprietary cards)",profileCount);
    }

    taf_pa_sim_ProfileInfo_t regInfo;
    taf_pa_sim_ProfileInfo_t emInfo;

    bool hasRegular   = FindProfileByType(paSlot, TAF_PA_SIM_PROFILE_TYPE_REGULAR,   &regInfo);
    bool hasEmergency = FindProfileByType(paSlot, TAF_PA_SIM_PROFILE_TYPE_EMERGENCY, &emInfo);

    if (!hasRegular || !hasEmergency) {
        LE_ERROR("missing required profiles (regular=%d, emergency=%d)", hasRegular, hasEmergency);
        return LE_FAULT;
    }

    taf_pa_sim_ProfileInfo_t* targetInfo = toEmergency ? &emInfo : &regInfo;

    if (targetInfo->state == TAF_PA_SIM_PROFILE_STATE_ACTIVE) {
        LE_INFO("Target profile already active");
        return LE_OK;
    }

    if (targetInfo->profileId == TAF_PA_SIM_PROFILE_ID_UNKNOWN) {
        LE_ERROR("Target profileId is unknown");
        return LE_FAULT;
    }
    pa_result_t paRes = taf_pa_sim_SetActiveProfile(paSlot, targetInfo->profileId);
    if (paRes != TAF_PA_SIM_RESULT_OK) {
        LE_ERROR("SetActiveProfile failed, paRes=%d",(int)paRes);
        return LE_FAULT;
    }
    LE_INFO("SetActiveProfile requested successfully (slot=%d, profileId=%d)",(int)paSlot, (int)targetInfo->profileId);
    return LE_OK;
}

le_result_t taf_sim::LocalSwapToEmergencyCallSubscription
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    LE_INFO("LocalSwapToEmergencyCallSubscription for sim:%d", (int)simId);
    return SwapSubscriptionInternal(simId, manufacturer, true);
}

le_result_t taf_sim::LocalSwapToCommercialCallSubscription
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    LE_INFO("LocalSwapToCommercialCallSubscription for sim:%d", (int)simId);
    return SwapSubscriptionInternal(simId, manufacturer, false);
}

void taf_sim::UpdateLocalSimState(taf_sim_info_t* simPtr, const sim_iccid_event_t* iccidDataInfo)
{
    if (!simPtr) {
        LE_ERROR("simPtr is NULL, cannot update local SIM state");
        return;
    }
    if (!iccidDataInfo ||iccidDataInfo->ICCID[0] == '\0') {
        simPtr->ICCID[0] = '\0';
        simPtr->IMSI[0] = '\0';
        simPtr->phoneNumber[0] = '\0';
        simPtr->pinTryCount = 3;
        simPtr->pukTryCount = 10;
        LE_INFO("Cleared local SIM info for simId %d", simPtr->simId);
    }
    else {
        taf_sim_Id_t simId = (taf_sim_Id_t)iccidDataInfo->simId;
        std::string imsiStr;
        if (taf_pa_sim_GetImsi((taf_pa_sim_Id_t)simId, imsiStr) == TAF_PA_SIM_RESULT_OK) {
            le_utf8_Copy(simPtr->IMSI, imsiStr.c_str(), TAF_SIM_IMSI_BYTES, NULL);
        } else {
            LE_WARN("Failed to fetch IMSI from PA for simId %d", simId);
            simPtr->IMSI[0] = '\0';
        }
        std::string phoneStr;
        if (taf_pa_sim_GetSubscriberPhoneNumber((taf_pa_sim_Id_t)simId, phoneStr) == TAF_PA_SIM_RESULT_OK) {
            le_utf8_Copy(simPtr->phoneNumber, phoneStr.c_str(), TAF_SIM_PHONE_NUM_MAX_BYTES, NULL);
        } else {
            LE_WARN("Failed to fetch Phone Number from PA for simId %d", simId);
            simPtr->phoneNumber[0] = '\0';
        }
    }
}

void taf_sim::HandleCardInfoChanged_Queued(void* param1, void* param2)
{
    sim_event_t* evt = static_cast<sim_event_t*>(param1);
    if (!evt)
    {
        LE_ERROR("HandleCardInfoChanged_Queued: evt is NULL");
        return;
    }
    auto& sim = taf_sim::GetInstance();
    if (evt->state == TAF_SIM_ABSENT)
    {
        LE_INFO("CardInfoChanged for simId:%d state:%d:", evt->simId, evt->state);
        taf_sim_info_t* simPtr = sim.GetSimContext(evt->simId);
        if (simPtr)
        {
            sim.UpdateLocalSimState(simPtr, nullptr);
        }
    }
    else if (evt->state == TAF_SIM_PRESENT)
    {
        LE_INFO("CardInfoChanged for simId:%d state:%d:", evt->simId, evt->state);
        sim.CheckAndSendRefreshEvent(evt->simId);
    }
    le_event_Report(sim.NewStateEventId, evt, sizeof(*evt));
    le_mem_Release(evt);
}

void taf_sim::HandleSubscriptionInfoChanged_Queued(void* param1, void* param2)
{
    sim_iccid_event_t* evt = static_cast<sim_iccid_event_t*>(param1);
    if (!evt)
    {
        LE_ERROR("HandleSubscriptionInfoChanged_Queued: evt is NULL");
        return;
    }
    auto& sim = taf_sim::GetInstance();
    LE_INFO("HandleSubscriptionInfoChanged_Queued: simId=%d ICCID=%s",evt->simId, evt->ICCID);
    taf_sim_info_t* simPtr = sim.GetSimContext(evt->simId);
    if (!simPtr) {
        LE_ERROR("Failed to get SimContext for simId: %d. Aborting update.", evt->simId);
        le_mem_Release(evt);
        return;
    }
    le_utf8_Copy(simPtr->ICCID,evt->ICCID,TAF_SIM_ICCID_BYTES,NULL);
    sim.UpdateLocalSimState(simPtr,evt);
    le_event_Report(sim.IccidChangeEventId, evt, sizeof(*evt));
    sim.CheckAndSendProfileSwitchEvent();
    le_mem_Release(evt);
}