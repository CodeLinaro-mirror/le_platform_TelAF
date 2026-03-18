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
#include <unistd.h>

using namespace tafsvc;
using namespace std;

static taf_sim_info_t simList[TAF_SIM_ID_MAX];
static int fplmnListIndex = 0;
static taf_sim_FPLMNListRef_t fplmnListRefs = nullptr;

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
    if (!cardInfo) {
        LE_ERROR("Received null cardInfo");
        return;
    }
    auto &sim = taf_sim::GetInstance();
    sim_event_t simEvent;
    simEvent.simId = (taf_sim_Id_t)cardInfo->slotId;
    simEvent.state =  (taf_sim_States_t)cardInfo->state;
    LE_INFO("Card info changed for slotid: %d, State: %d",  simEvent.simId , simEvent.state);
    if(simEvent.state == TAF_SIM_PRESENT)
    {
        sim.CheckAndSendRefreshEvent((taf_sim_Id_t)simEvent.simId);
    }
    else if (simEvent.state == TAF_SIM_ABSENT)
    {
        taf_sim_info_t* simPtr = sim.GetSimContext(simEvent.simId);
        if (simPtr) {
            sim.UpdateLocalSimState(simPtr, nullptr);
        }
    }
    le_event_Report(sim.NewStateEventId, &simEvent, sizeof(simEvent));
}

void Handler::onSubscriptionInfoChanged
(
    const std::shared_ptr<taf_pa_sim_Iccid_t>& iccidDataInfo
)
{
    LE_INFO("onSubscriptionInfoChanged received from PA");
    if (!iccidDataInfo) {
        LE_ERROR("Received null iccidDataInfo from PA layer");
        return;
    }
    taf_sim_Id_t simId = (taf_sim_Id_t)iccidDataInfo->simId;
    if (iccidDataInfo->ICCID.empty()) {
        LE_WARN("Received empty ICCID for simId %d", simId);
    }
    auto &sim = taf_sim::GetInstance();
    taf_sim_info_t* simPtr = sim.GetSimContext(simId);
    if (!simPtr) {
        LE_ERROR("Failed to get SimContext for simId: %d. Aborting update.", simId);
        return;
    }
    sim.UpdateLocalSimState(simPtr, iccidDataInfo);
    sim_iccid_event_t simIccidEvent;
    memset(&simIccidEvent, 0, sizeof(simIccidEvent));
    simIccidEvent.simId = simId;
    le_result_t res = le_utf8_Copy(simIccidEvent.ICCID, iccidDataInfo->ICCID.c_str(), sizeof(simIccidEvent.ICCID), NULL);
    if (res != LE_OK) {
        LE_WARN("ICCID truncated while copying");
    }
    LE_DEBUG("simIccidEvent simId: %d, ICCID: %s", simIccidEvent.simId, simIccidEvent.ICCID);
    // 6. Report Event
    le_event_Report(sim.IccidChangeEventId, &simIccidEvent, sizeof(simIccidEvent));
    if(!sim.IsPsEventInProgress) {
        sim.IsPsEventInProgress = true;
        sim.CheckAndSendProfileSwitchEvent();
    }
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
            simResponsePtr.responseType,
            simResponsePtr.result);
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
            simResponsePtr.responseType,
            simResponsePtr.result);
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
            simResponsePtr.responseType,
            simResponsePtr.result);
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
    le_mem_ExpandPool(FPLMNNodePool, TAF_SIM_FPLMN_MAX_LISTS * TAF_SIM_FPLMN_MAX_OPERATORS_PER_LIST);
    FPLMNListPool = le_mem_CreatePool("FPLMNListPool", sizeof(taf_sim_FPLMNList_t));
    le_mem_ExpandPool(FPLMNListPool, TAF_SIM_FPLMN_MAX_LISTS);
    FPLMNListRefMap = le_ref_CreateMap("FPLMNListRefMap", TAF_SIM_FPLMN_MAX_LISTS);

    SessionPool = le_mem_CreatePool("SessionPool", sizeof(taf_sim_Session_t));
    le_mem_ExpandPool(SessionPool, 12);
    SessionRefMap = le_ref_CreateMap("SessionRefMapRefMap", 10);
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
    if(taf_pa_sim_RegisterEventListener(&eventListener,nullptr) !=  PA_OK)
    {
        LE_ERROR("Listener register failed for");
        return;
    }
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
    int slotCount = 0;
    pa_result_t paResult = taf_pa_sim_getSlotCount(&slotCount);
    if (paResult != TAF_PA_SIM_RESULT_OK)
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

void onRefreshEvent(taf_pa_sim_RefreshChangeInd_t ind, void* contextPtr) {
   auto &sim = taf_sim::GetInstance();

   LE_INFO("onRefreshEvent: contextPtr: %p", contextPtr);

   sim.NotifyRefreshEvent(&ind,contextPtr);
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
void taf_sim::CheckAndSendProfileSwitchEvent() {
    auto &sim = taf_sim::GetInstance();

    char iccid1[TAF_SIM_ICCID_BYTES];
    char iccid2[TAF_SIM_ICCID_BYTES];
    char iccid[TAF_SIM_ICCID_BYTES]  = {0};
    sim_refresh_event_t simRefreshEvent;
    simRefreshEvent.refreshStatus = 0;

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

        LE_INFO("ClientSessionRef %p, refreshResetStart: %d", sessionPtr->clientSessionRef, (int) sessionPtr->refreshResetStart);

            //Send profile switch notification.
        if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV && iccid1[0] != '\0'
              && sessionPtr->simProfileIccid1[0] != '\0')
        {
            if (strncmp(iccid1, sessionPtr->simProfileIccid1, TAF_SIM_ICCID_BYTES) != 0)
            {
                LE_INFO("Notify: current iccid1: %s, previous iccid1: %s and result: %s", iccid1, sessionPtr->simProfileIccid1, LE_RESULT_TXT(result));
                simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
                le_utf8_Copy(sessionPtr->simProfileIccid1, iccid1, TAF_SIM_ICCID_BYTES, NULL);
                le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
            }
        }
        if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV  &&
                 sessionPtr->simProfileIccid2[0] != '\0' && iccid2[0] != '\0')
        {
            if (strncmp(iccid2, sessionPtr->simProfileIccid2, TAF_SIM_ICCID_BYTES) != 0)
            {
                LE_INFO("Notify: current iccid2: %s, previous iccid2: %s and result: %s", iccid2, sessionPtr->simProfileIccid2, LE_RESULT_TXT(result) );
                simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
                le_utf8_Copy(sessionPtr->simProfileIccid2, iccid2, TAF_SIM_ICCID_BYTES, NULL);
                le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
            }
        }
        result = le_ref_NextNode(iterRef);
    }
    sim.IsPsEventInProgress = false;

}

void taf_sim::CheckAndSendRefreshEvent(taf_sim_Id_t SimId) {
    auto &sim = taf_sim::GetInstance();
    le_result_t result = LE_FAULT;
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("CheckAndSendRefreshEvent sessionPtr null!");
            result = le_ref_NextNode(iterRef);
            continue;
        }
        if(isSingleActive ||
            (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV && SimId == TAF_SIM_SLOT_ID_1) ||
            (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV && SimId == TAF_SIM_SLOT_ID_2))
        {
            if (sessionPtr->refreshResetStart)
            {
                LE_INFO("Notify RefreshEvent for SimId : %d,SessionType : %d", SimId,sessionPtr->sessionType);
                sessionPtr->refreshResetStart = false;
                le_sem_Post(sessionPtr->semaphore);
            }
        }
        result = le_ref_NextNode(iterRef);
    }
}

void taf_sim::NotifyRefreshEvent(taf_pa_sim_RefreshChangeInd_t* ind, void* contextPtr) {
    LE_INFO("RefreshEvent: sessionType = %d, refreshMode = %d, refreshStage = %d", ind->sessionType, ind->refreshMode, ind->refreshStage);
    le_result_t res = LE_FAULT;
    taf_sim_Session_t* clientRequestPtr = NULL;
    sim_refresh_event_t simRefreshEvent;
    simRefreshEvent.refreshStatus = 0;
    clientRequestPtr = DiscoverSessionRef((taf_sim_RefreshRef_t) contextPtr);

    LE_INFO("NotifyRefreshEvent contextPtr:%p, clientRequestPtr:%p RefreshVoteSent_Slot1:%d RefreshVoteSent_Slot2:%d", contextPtr, clientRequestPtr,
              RefreshVoteSent_Slot1, RefreshVoteSent_Slot2);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "clientRequestPtr is NULL");

    if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK) {
        // In WAIT_FOR_OK stage of SIM refresh,refresh vote hasn't been sent for slot 1/slot 2
        if((clientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV && !RefreshVoteSent_Slot1) ||
              (clientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV && !RefreshVoteSent_Slot2))
        {
            LE_INFO("Request Refresh_ok with refreshAllow: %d", (int) clientRequestPtr->refreshAllow);
            le_result_t result = CheckRefreshAllow(ind);
            if(result == LE_FAULT)
            {
                pa_result_t res = taf_pa_sim_RefreshOk(ind->sessionType, &clientRequestPtr-> refreshAllow);
                LE_INFO("Refresh_ok as false %d", res);
            }
            else
            {
                pa_result_t res = taf_pa_sim_RefreshOk(ind->sessionType, &clientRequestPtr-> refreshAllow);
                LE_INFO("Refresh_ok as true %d", res);
            }
            if(clientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV)
            {
                RefreshVoteSent_Slot1 = true;
            }
            if(clientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV)
            {
                RefreshVoteSent_Slot2 = true;
            }
        }
        return;
    }

    else if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_START && ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN) {
        pa_result_t res = taf_pa_sim_RefreshComplete(ind->sessionType);
        LE_INFO("RefreshComplete: result: %d",res);
        return;
    } else if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_START && ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_RESET) {
        LE_INFO("RefreshStart for reset mode");
        clientRequestPtr->refreshResetStart = true;
        le_clk_Time_t timeToWait = {5, 0};
        res = le_sem_WaitWithTimeOut(clientRequestPtr->semaphore, timeToWait);
        if (res == LE_OK )
        {
           simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
        }
        else
        {
           simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_FAILURE;
        }
        le_event_Report(clientRequestPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        clientRequestPtr->refreshResetStart = false;
        ResetRefreshVote(clientRequestPtr);
        LE_INFO("Notify simRefreshEvent:refreshStatus : %d", simRefreshEvent.refreshStatus);
        return;
        }

    bool notifyClient = (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_SUCCESS)
            || (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE);

    if (notifyClient) {

        bool isFileChanged = (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN)
                || (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FULL_FCN)
                || (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN);

        LE_INFO("refreshResetStart: %d and isFileChanged: %d", (int) clientRequestPtr->refreshResetStart, (int) isFileChanged);

        if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE) {
            simRefreshEvent.refreshStatus = ConvertPaRefreshStageToTafRefreshStatus(ind->refreshStage);
        } else {
            //Add the refresh success.
            simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
            if (isFileChanged) {
                simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_FILE_CHANGE;
            }
        }
        if (clientRequestPtr != NULL) {
            le_event_Report(clientRequestPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        }
    }
    ResetRefreshVote(clientRequestPtr);
}

void taf_sim::FirstLayerNewRefreshChangeHandler(void* reportPtr, void* secondLayerHandlerFunc) {
    sim_refresh_event_t* simRefreshPtr = (sim_refresh_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simRefreshPtr == NULL, "simRefreshPtr is NULL");

    taf_sim_RefreshChangeHandlerFunc_t clientHandlerFunc =
        (taf_sim_RefreshChangeHandlerFunc_t)secondLayerHandlerFunc;
    clientHandlerFunc(simRefreshPtr->refreshStatus, le_event_GetContextPtr());
}

taf_sim_RefreshChangeHandlerRef_t taf_sim::AddRefreshChangeHandler(taf_sim_RefreshChangeHandlerFunc_t handlerPtr, void* contextPtr) {
    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Refresh Change handler");
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL");
        return NULL;
    }

    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    handlerRef = le_event_AddLayeredHandler("RefreshChangeHandler", clientRequestPtr->RefreshChangeEventId,
            FirstLayerNewRefreshChangeHandler, (void*)handlerPtr);

    clientRequestPtr->paHandlerRef = taf_pa_sim_AddRefreshChangeHandler((taf_pa_sim_RefreshChangeHandlerFunc_t)&onRefreshEvent, sessionRef);

    LE_INFO("taf_pa_sim_AddRefreshChangeHandler done. paHandlerRef: %p, handlerRef: %p", clientRequestPtr->paHandlerRef, handlerRef);

    return (taf_sim_RefreshChangeHandlerRef_t)(handlerRef);
}

void taf_sim::RemoveRefreshChangeHandler(taf_sim_RefreshChangeHandlerRef_t handlerRef) {
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_NIL( NULL == clientRequestPtr, "clientRequestPtr is NULL");
    taf_pa_sim_RemoveRefreshChangeHandler(clientRequestPtr->paHandlerRef);
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    le_ref_DeleteRef(SessionRefMap, clientRequestPtr->ref);
    mClientRefCount--;
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
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    LE_INFO("CreateSession client session ref %p", sessionRef);

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    LE_INFO("After DiscoverSessionRef client session ref %p", clientRequestPtr);

    if (clientRequestPtr != nullptr) {
        LE_INFO("CreateSession: Already created the refresh Session, so use the existing one.");

        *refreshSessionRef = (taf_sim_RefreshRef_t) clientRequestPtr->ref;
        clientRequestPtr->sessionType = sessionType;

        return LE_OK;
    }

    taf_sim_Session_t* res  = (taf_sim_Session_t* )le_mem_ForceAlloc(SessionPool);
    if(res == NULL) {
        LE_INFO("Create SessionPool failed!");
        return LE_FAULT;
    }
    LE_INFO("Create new Session");
    res->link = LE_DLS_LIST_INIT;
    res->ref = (taf_sim_RefreshRef_t)le_ref_CreateRef(SessionRefMap, res);

    res->clientSessionRef = (le_msg_SessionRef_t) sessionRef;

    *refreshSessionRef = (taf_sim_RefreshRef_t)(res->ref);
    res->sessionType = sessionType;
    res->refreshAllow = true;
    res->refreshMode = (taf_sim_RefreshMode_t) 0xffff;
    res->refreshResetStart = false;
    res->refreshRegFilesSize = 0;
    res->RefreshChangeEventId = le_event_CreateId("ClientRefreshEventId", sizeof(sim_refresh_event_t));
    res->semaphore = le_sem_Create("IccidCheckSem", 0);
    LE_INFO("res->sessionRef %p, *reference %p", res->ref, *refreshSessionRef);

    char iccid[TAF_SIM_ICCID_BYTES]  = {0};
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

    for (int i = 0; i < (int) filesSize; i++) {
        clientRequestPtr->refreshRegFiles[i].file_id = filesPtr[i].file_id;
        le_utf8_Copy((char*) clientRequestPtr->refreshRegFiles[i].path, (char*) filesPtr[i].path, sizeof(filesPtr[i].path), NULL);

        LE_INFO("File_id: %d and path: %s", clientRequestPtr->refreshRegFiles[i].file_id, clientRequestPtr->refreshRegFiles[i].path);
    }

    clientRequestPtr->refreshRegFilesSize = filesSize;

    return LE_OK;
}

le_result_t taf_sim::SetRefreshMode(taf_sim_RefreshRef_t refreshSessionRef, taf_sim_RefreshMode_t refreshMode) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    clientRequestPtr->refreshMode = refreshMode;

    return LE_OK;
}

le_result_t taf_sim::SetRefreshAllow(taf_sim_RefreshRef_t refreshSessionRef, bool isRefreshAllowed) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    taf_pa_sim_RefreshFile_t refreshPAFiles[clientRequestPtr->refreshRegFilesSize];

    for (int i = 0; i < (int) clientRequestPtr->refreshRegFilesSize; i++) {
        int pathStrLen = 0;
        refreshPAFiles[i].file_id = clientRequestPtr->refreshRegFiles[i].file_id;
        if(clientRequestPtr->refreshRegFiles[i].path != NULL) {
            pathStrLen = strlen(clientRequestPtr->refreshRegFiles[i].path);
            refreshPAFiles[i].path_len = pathStrLen/2;
        } else {
            refreshPAFiles[i].path_len = 0;
        }

        //If input clientRequestPtr->refreshRegFiles[i].path is 3F007FFF.
        //Then refreshPAFiles.path[0] = 0(00), refreshPAFiles.path[1]=63(3F), refreshPAFiles.path[2] = 255(FF) and refreshPAFiles.path[3] = 127(7F)

        uint32_t pathValue =  std::stoul(clientRequestPtr->refreshRegFiles[i].path, nullptr, 16);

        LE_INFO("pathValue string: %s, in hex: %x and input path len: %d", clientRequestPtr->refreshRegFiles[i].path, pathValue, pathStrLen);

        if (refreshPAFiles[i].path_len == 2) {
            refreshPAFiles[i].path[0] = (pathValue & 0x000000ff);
            refreshPAFiles[i].path[1] = (pathValue & 0x0000ff00) >> 8;
        } else if (refreshPAFiles[i].path_len == 4) {
            refreshPAFiles[i].path[0] = (pathValue & 0x00ff0000) >> 16;
            refreshPAFiles[i].path[1] = (pathValue & 0xff000000) >> 24;
            refreshPAFiles[i].path[2] = (pathValue & 0x000000ff);
            refreshPAFiles[i].path[3] = (pathValue & 0x0000ff00) >> 8;
        }
        LE_INFO("PA file path0 ~ path3 in hex: %x %x %x %x", refreshPAFiles[i].path[0], refreshPAFiles[i].path[1], refreshPAFiles[i].path[2], refreshPAFiles[i].path[3]);

        LE_INFO("PA file path0 ~ path3 in dec: %u %u %u %u", refreshPAFiles[i].path[0], refreshPAFiles[i].path[1], refreshPAFiles[i].path[2], refreshPAFiles[i].path[3]);

        LE_INFO("PA File_id: %d and path_len: %d", refreshPAFiles[i].file_id, refreshPAFiles[i].path_len);
    }

    pa_result_t res = taf_pa_sim_RefreshRegister(ConvertTafSessionTypeToPaSessionType(clientRequestPtr->sessionType),
            clientRequestPtr->refreshRegFilesSize,
            refreshPAFiles);
    le_result_t result =Utility::Convert::Result(res);

    LE_INFO("Refresh register done: result: %d", res);

    clientRequestPtr->refreshAllow = isRefreshAllowed;
    return result;
}

le_result_t taf_sim::selectSimSlot(taf_sim_Id_t simId) {
    if (!isValidSimId(simId)) {
        LE_WARN("Invalid simId: %d", (int)simId);
        return LE_FAULT;
    }
    pa_result_t paResult = taf_pa_sim_selectSimSlot((taf_pa_sim_Id_t)simId);
    if (paResult == TAF_PA_SIM_RESULT_OK)
    {
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
    int mcc = 0;
    int mnc = 0;
    LE_INFO("getHomeNetworkMccMnc for simId %d", simId);
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    pa_result_t paResult = taf_pa_sim_GetHomeNetworkMccMnc((taf_pa_sim_Id_t)simId, &mcc, &mnc);
    if (paResult != TAF_PA_SIM_RESULT_OK)
    {
        LE_ERROR("Failed to get HomeNetworkMccMnc via PA for simId %d", simId);
        return LE_FAULT;
    }
    LE_INFO("Retrieved MCC: %d, MNC: %d", mcc, mnc);
    le_utf8_Copy(mccPtr, std::to_string(mcc).c_str(), mccPtrSize, NULL);
    le_utf8_Copy(mncPtr, std::to_string(mnc).c_str(), mncPtrSize, NULL);
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

le_result_t  taf_sim::GetEID( taf_sim_Id_t slotId, char* eidPtr, size_t eidLen) {
    return LE_UNSUPPORTED;
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
            if (isSingleActive)
            {
                *slotOut = TAF_PA_SIM_SLOT_1;
                return LE_OK;
            } else {
                LE_ERROR("MapSimIdToPaSlot: not supported on this slot");
                return LE_BAD_PARAMETER;
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
    uint8_t n = taf_pa_sim_GetProfileNum(paSlot);
    for (uint8_t i = 0; i < n; ++i) {
        taf_pa_sim_ProfileInfo_t info = taf_pa_sim_GetProfile(paSlot, i);
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

    uint8_t profileCount = taf_pa_sim_GetProfileNum(paSlot);
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



taf_sim_FPLMNListRef_t taf_sim::CreateInternalFPLMNList
(
)
{
    taf_sim_FPLMNList_t* res = (taf_sim_FPLMNList_t* )le_mem_ForceAlloc(FPLMNListPool);
    if(res == NULL) {
        LE_INFO("CreateInternalFPLMNList failed!");
        return NULL;
    }
    res->link = LE_DLS_LIST_INIT;
    res->ref = (taf_sim_FPLMNListRef_t)le_ref_CreateRef(FPLMNListRefMap, res);
    return (taf_sim_FPLMNListRef_t)(res->ref);
}

taf_sim_FPLMNListRef_t taf_sim::CreateFPLMNList
(
)
{
    if (fplmnListRefs != nullptr) {
        taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, fplmnListRefs);
        if (!ListReference) {
            return NULL;
        }
        LE_INFO("CreateFPLMNList: Already created fplmnListRefs, so use the existing one.");
        return (taf_sim_FPLMNListRef_t)(ListReference->ref);
    }

    return CreateInternalFPLMNList();
}

le_result_t taf_sim::AddFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    char* mncPtr
)
{
    if ((mccPtr == NULL) || (mncPtr == NULL)) {
        LE_INFO("MCC or MNC pointer is NULL");
        return LE_OVERFLOW;
    }
    fplmnListRefs = FPLMNListRef;
    return AddFPLMNOperatorInternal(FPLMNListRef, mccPtr, mncPtr);
}

le_result_t taf_sim::AddFPLMNOperatorInternal
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    char* mncPtr
)
{
    if (!IsValidMCCAndMNC(mccPtr, mncPtr)) {
        return LE_OVERFLOW;
    }

    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL)) {
        LE_INFO("Issue with ListReference ");
        return LE_OVERFLOW;
    }
    FPLMNNode_t* nodeFPLMN = (FPLMNNode_t*)le_mem_ForceAlloc(FPLMNNodePool);
    if (nodeFPLMN == NULL) {
        return LE_FAULT;
    }

    le_utf8_Copy(nodeFPLMN->mcc, mccPtr, sizeof(nodeFPLMN->mcc), NULL);
    le_utf8_Copy(nodeFPLMN->mnc, mncPtr, sizeof(nodeFPLMN->mnc), NULL);
    nodeFPLMN->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&(ListReference->link), &(nodeFPLMN->link));
    return LE_OK;
}

le_result_t taf_sim::GetFirstFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    size_t mccLen,
    char* mncPtr,
    size_t mncLen
)
{
    if ((mccPtr == NULL) || (mncPtr == NULL)) {
        LE_INFO("MCC or MNC pointer is NULL");
        return LE_OVERFLOW;
    }
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL)) {
        LE_INFO("Issue with ListReference ");
        return LE_OVERFLOW;
    }
    le_dls_Link_t* linkPtr = le_dls_Peek(&(ListReference->link));
    if (linkPtr == NULL) {
        return LE_FAULT;
    }
    FPLMNNode_t* node = CONTAINER_OF(linkPtr, FPLMNNode_t, link);
    if (node == NULL) {
        return LE_FAULT;
    }
    le_utf8_Copy(mccPtr, node->mcc, mccLen, &mccLen);
    le_utf8_Copy(mncPtr, node->mnc, mncLen, &mncLen);
    fplmnListIndex = 1;
    return LE_OK;
}

le_result_t taf_sim::GetNextFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    size_t mccLen,
    char* mncPtr,
    size_t mncLen
)
{
    if ((mccPtr == NULL) || (mncPtr == NULL)) {
        LE_INFO("MCC or MNC pointer is NULL");
        return LE_OVERFLOW;
    }
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL)) {
        LE_INFO("Issue with ListReference ");
        return LE_OVERFLOW;
    }
    le_dls_Link_t* linkPtr = le_dls_Peek(&(ListReference->link));
    if (linkPtr == NULL) {
        return LE_FAULT;
    }
    FPLMNNode_t* node;

    for (int i = 0; i < fplmnListIndex && linkPtr!=NULL; i++) {
        linkPtr = le_dls_PeekNext(&(ListReference->link), (linkPtr));
    }

    if(linkPtr == NULL) {
        return LE_FAULT;
    }
    node = CONTAINER_OF(linkPtr, FPLMNNode_t, link);
    if (node == NULL) {
        return LE_FAULT;
    }
    le_utf8_Copy(mccPtr, node->mcc, mccLen, &mccLen);
    le_utf8_Copy(mncPtr, node->mnc, mncLen, &mncLen);
    fplmnListIndex++;
    return LE_OK;
}

taf_sim_FPLMNListRef_t taf_sim::ReadFPLMNList(
    taf_sim_Id_t simId
)
{
    //First select the file using APDU commands
    //Then read from it in binary form and use payload to get the response as a hex string
    uint8_t selectFPLMNApdu[] = {0x00, 0xA4, 0x08, 0x04, 0x04, 0x7F, 0xFF, 0x6F, 0x7B};
    uint8_t responseAPDU[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t responseLength = 0;
    uint8_t channel = 0;
    LE_INFO("Entered here");
    if((selectSimSlot(simId))!=LE_OK) {
        return NULL;
    }
    LE_INFO("Sim slot selected");
    le_result_t res = OpenLogicalChannel((taf_sim_Id_t)slot, TAF_SIM_APPTYPE_USIM, &channel);
    if(res != LE_OK) {
        return NULL;
    }
    LE_INFO("Logical channel opened channel id: %d", channel);
    selectFPLMNApdu[0] = channel;
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, selectFPLMNApdu, sizeof(selectFPLMNApdu), responseAPDU, &responseLength);

    if(res != LE_OK || (uint8_t)responseAPDU[responseLength-2] != 0x61) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
        LE_INFO("ReadFplmnList: CloseLogicalChannel channel_id: %d res: %d", channel, res);
        return NULL;
    }
    LE_DEBUG("ReadFplmnList: After selectFPLMNApdu channel id: %d", channel);
    LE_INFO("selectFPLMNApdu sw1: %d, sw2: %d", (uint8_t)responseAPDU[responseLength-2], (uint8_t)responseAPDU[responseLength-1]);

    uint8_t readBinaryFPLMNApdu[] = {0x00, 0xB0, 0x00, 0x00, 0x00};
    readBinaryFPLMNApdu[0] = channel;
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, readBinaryFPLMNApdu, sizeof(readBinaryFPLMNApdu), responseAPDU, &responseLength);

    if(res != LE_OK || (uint8_t)responseAPDU[responseLength-2] != 0x90 || (uint8_t)responseAPDU[responseLength-1] != 0x00) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
        LE_INFO("ReadFplmnList: CloseLogicalChannel channel_id: %d res: %d", channel, res);
        return NULL;
    }
    LE_INFO("ReadFplmnList: After readFPLMNApdu channel id: %d", channel);
    res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
    if(res != LE_OK) {
        return NULL;
    }

    taf_sim_FPLMNListRef_t listRef = CreateInternalFPLMNList();
    int size = responseLength - 2;

    for(int i=0; i<size/3; i++) {
        int k=3*i;
        char mcc[4], mnc[4];

        //1st byte:mcc[1] mcc[0]
        //2th byte:mnc[2] mcc[2]
        //3th byte:mnc[1] mnc[0]
        //42 F6 18 = mcc:246 mnc:81
        //60 f5 34 = mcc:65 mnc:43
        //e.g. mcc:246 mnc:81 = 42 F6 18. Here mcc[0] = 2, mcc[1] = 4, mcc[2] = 6 and mnc[0] = 8, mnc[1] = 1
        //e.g. mcc:65 mnc:43 = 40 65 91. Here mcc[0] = 0, mcc[1] = 6, mcc[2] = 5 and mnc[0] = 4, mnc[1] = 3

        if ((k+2) >= TAF_SIM_RESPONSE_MAX_BYTES)
        {
            LE_ERROR("Exceeds the max response bytes");
            return NULL;
        }
        if (responseAPDU[k] == 0xFF && responseAPDU[k+1] == 0xFF && responseAPDU[k+2] == 0xFF) {
            LE_INFO("Skipping invalid MCC/MNC due to all FF values");
            continue;
        }
        mcc[0] = (responseAPDU[k] & 0x0F) + '0';
        mcc[1] = ((responseAPDU[k] & 0xF0) >> 4) + '0';
        mcc[2] = (responseAPDU[k+1] & 0x0F) + '0';

        mnc[0] = (responseAPDU[k+2] & 0x0F) + '0';
        mnc[1] = ((responseAPDU[k+2] & 0xF0) >> 4) + '0';

        if(((responseAPDU[k+1] & 0xF0) >> 4) == 15) {
            mnc[2] = '\0';
        } else {
            mnc[2]=((responseAPDU[k+1] & 0xF0) >> 4) + '0';
        }
        mcc[3] = '\0';
        mnc[3] = '\0';

        res = AddFPLMNOperatorInternal(listRef, mcc, mnc);
        if(res!=LE_OK) {
            DeleteFPLMNList(listRef);
            return NULL;
        }
        LE_INFO("FPLMN #%d - MCC:%s MNC:%s", i+1, mcc, mnc);
    }

    return listRef;
}

void taf_sim::DeleteFPLMNList
(
    taf_sim_FPLMNListRef_t FPLMNListRef
)
{
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if(ListReference == NULL) {
        LE_WARN("Cannot find the FPLMNList with ref: %p", FPLMNListRef);
        return;
    }

    // Release all nodes in the list before releasing the list itself
    while(!le_dls_IsEmpty(&(ListReference->link))) {
        le_dls_Link_t* link = le_dls_Peek(&(ListReference->link));
        if (link != NULL) {
            le_dls_Remove(&(ListReference->link), link);
            FPLMNNode_t* node = CONTAINER_OF(link, FPLMNNode_t, link);
            if (node != NULL)
            {
                le_mem_Release(node);
            }
        }
    }
    le_ref_DeleteRef(FPLMNListRefMap, FPLMNListRef);
    fplmnListIndex = 0;
    le_mem_Release(ListReference);

    // Clear fplmnListRefs if it points to the deleted list
    if (fplmnListRefs == FPLMNListRef) {
        fplmnListRefs = nullptr;
    }
}

le_result_t taf_sim::WriteFPLMNList
(
    taf_sim_Id_t simId,
    taf_sim_FPLMNListRef_t FPLMNListRef
)
{
    //Select the EF
    uint8_t selectFPLMNApdu[] = {0x00, 0xA4, 0x08, 0x04, 0x04, 0x7F, 0xFF, 0x6F, 0x7B};
    uint8_t responseAPDU[TAF_SIM_RESPONSE_MAX_BYTES];
    size_t responseLength = 0;
    uint8_t channel = 0;
    if(selectSimSlot(simId) != LE_OK) {
        return LE_FAULT;
    }
    le_result_t res = OpenLogicalChannel((taf_sim_Id_t)slot, TAF_SIM_APPTYPE_USIM, &channel);
    if(res != LE_OK) {
        return LE_FAULT;
    }
    LE_INFO("WriteFPLMNList: OpenLogicalChannel channel id: %d", channel);
    const uint8_t channel_id = channel;
    selectFPLMNApdu[0] = channel;
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, selectFPLMNApdu, sizeof(selectFPLMNApdu), responseAPDU, &responseLength);
    if(res != LE_OK || (uint8_t)responseAPDU[responseLength-2] != 0x61) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }
    LE_DEBUG("WriteFPLMNList: After selectFPLMNApdu channel id: %d and channel_id: %d", channel, channel_id);
    LE_INFO("selectFPLMNApdu sw1: %d, sw2: %d", (uint8_t)responseAPDU[responseLength-2], (uint8_t)responseAPDU[responseLength-1]);

    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if(ListReference == NULL) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }

    std::vector<uint8_t> writeFPLMNListApdu = {0x00, 0xD6, 0x00, 0x00, 0x00};

    auto nodeLink=le_dls_Peek(&(ListReference->link));

    while (nodeLink != NULL) {
        FPLMNNode_t* node=CONTAINER_OF(nodeLink, FPLMNNode_t, link);
        LE_INFO("WriteFPLMNList: node->mcc:%s, node->mnc:%s", node->mcc, node->mnc);

        //1st byte:mcc[1] mcc[0]
        //2th byte:mnc[2] mcc[2]
        //3th byte:mnc[1] mnc[0]
        //mcc:246 mnc:81 = 42 F6 18
        //mcc:65 mnc:43 = 60 f5 34
        //e.g. mcc:246 mnc:81 = 42 F6 18. Here mcc[0] = 2, mcc[1] = 4, mcc[2] = 6 and mnc[0] = 8, mnc[1] = 1
        //e.g. mcc:65 mnc:43 = 40 65 91. Here mcc[0] = 0, mcc[1] = 6, mcc[2] = 5 and mnc[0] = 4, mnc[1] = 3

        int mccInt = atoi(node->mcc);
        int mncInt = atoi(node->mnc);
        uint8_t mcc0 = (int) mccInt/100;
        uint8_t mcc1 = (int) (mccInt%100)/10;
        uint8_t mcc2 = (int) mccInt%10;
        uint8_t mnc0, mnc1, mnc2;
        if (mncInt > 99) {
            mnc0 = (int) mncInt/100;
            mnc1 = (int) (mncInt%100)/10;
            mnc2 = (int) mncInt%10;
        } else {
            mnc0 = (int) mncInt/10;
            mnc1 = (int) mncInt%10;
            mnc2 = 0x0F;
        }

        LE_DEBUG("WriteFPLMNList: mcc0:%d, mcc1:%d, mcc2:%d and mnc0:%d, mnc1:%d, mnc2:%d", (int) mcc0, (int) mcc1,
                (int) mcc2, (int) mnc0, (int) mnc1, (int) mnc2);

        writeFPLMNListApdu.emplace_back(mcc1*16+mcc0);
        writeFPLMNListApdu.emplace_back(mnc2*16+mcc2);
        writeFPLMNListApdu.emplace_back(mnc1*16+mnc0);

        nodeLink = le_dls_PeekNext(&(ListReference->link), nodeLink);
    }
    uint8_t sizeOfwriteFPLMNListApdu = writeFPLMNListApdu.size();
    LE_INFO("WriteFPLMNList: Total no of data(p3): %d", sizeOfwriteFPLMNListApdu);
    writeFPLMNListApdu.at(4) = sizeOfwriteFPLMNListApdu - 5;
    writeFPLMNListApdu[0] = channel_id;
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel_id, writeFPLMNListApdu.data(), sizeOfwriteFPLMNListApdu, responseAPDU, &responseLength);
    if(res != LE_OK || (uint8_t)responseAPDU[responseLength-2] != 0x90 || (uint8_t)responseAPDU[responseLength-1] != 0x00) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }

    res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
    LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
    if(res != LE_OK) {
        return LE_FAULT;
    }

    return LE_OK;
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
                return LE_OK;
            }
        }
    }
    LE_ERROR("GetSlotCount failed because multi sim sub system is not ready");
    return LE_FAULT;
}

le_result_t taf_sim::CheckRefreshAllow(taf_pa_sim_RefreshChangeInd_t* ind)
{
    auto &sim = taf_sim::GetInstance();
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);

        TAF_ERROR_IF_RET_VAL( NULL == sessionPtr, LE_FAULT, "SessionPtr is NULL");
        // Check if the refresh mode received from the modem  matches with any of the refresh modes requested by the client
        if (((1 << ind->refreshMode) & sessionPtr->refreshMode) != 0)
        {
            if(!sessionPtr->refreshAllow)
            {
                LE_INFO("For sessionPtr: %p refreshMode: %d not allow", sessionPtr, (int)sessionPtr->refreshMode);
                return LE_FAULT;
            }
        }
        result = le_ref_NextNode(iterRef);
    }
    return LE_OK;
}

void taf_sim::ResetRefreshVote(taf_sim_Session_t* ClientRequestPtr)
{
   if(ClientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV){
        RefreshVoteSent_Slot1 = false;
        LE_DEBUG("RefreshVoteSent_Slot1 is %d:",RefreshVoteSent_Slot1);
    }
    if(ClientRequestPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV){
        RefreshVoteSent_Slot2 = false;
        LE_DEBUG("RefreshVoteSent_Slot2 is %d:",RefreshVoteSent_Slot2);
    }
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

    uint8_t profileCount = taf_pa_sim_GetProfileNum(paSlot);
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

void taf_sim::UpdateLocalSimState(taf_sim_info_t* simPtr, const std::shared_ptr<taf_pa_sim_Iccid_t>& iccidDataInfo)
{
    if (!simPtr) {
        LE_ERROR("simPtr is NULL, cannot update local SIM state");
        return;
    }
    if (!iccidDataInfo ||iccidDataInfo->ICCID.empty()) {
        simPtr->ICCID[0] = '\0';
        simPtr->IMSI[0] = '\0';
        simPtr->phoneNumber[0] = '\0';
        simPtr->pinTryCount = 3;
        simPtr->pukTryCount = 10;
        LE_INFO("Cleared local SIM info for simId %d", simPtr->simId);
    }
    else {
        taf_sim_Id_t simId = (taf_sim_Id_t)iccidDataInfo->simId;
        le_utf8_Copy(simPtr->ICCID, iccidDataInfo->ICCID.c_str(), TAF_SIM_ICCID_BYTES, NULL);
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
