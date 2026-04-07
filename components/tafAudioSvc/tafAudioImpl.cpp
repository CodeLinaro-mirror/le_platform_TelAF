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
 *

Changes from Qualcomm Technologies, Inc. are provided under the following license:
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafAudio.hpp"
#include "tafAudioVhal.hpp"

using namespace tafsvc;
using namespace taf::audioVhal;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(tafAudioConnector, MAX_CONNECTOR,
        sizeof(taf_audio_Connector_t));
LE_MEM_DEFINE_STATIC_POOL(tafAudioHashmap, HASHMAP_SIZE, sizeof(taf_audio_hashMapList_t));
LE_MEM_DEFINE_STATIC_POOL(tafAudioStream, MAX_STREAM,
        sizeof(taf_audio_Stream_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef, MAX_STREAM, sizeof(taf_SessionRefNode_t));
LE_MEM_DEFINE_STATIC_POOL(tafAudioRoute, MAX_ROUTE, sizeof(taf_audio_Route_t));
LE_MEM_DEFINE_STATIC_POOL(tafEventIdPool, MAX_STREAM, sizeof(tafEventIdList));
LE_MEM_DEFINE_STATIC_POOL(tafEventHandlerRef, MAX_CONNECTOR, sizeof(EventHandlerRefNode_t));

// Define the DTMF frequency pairs
std::unordered_map<char, std::pair<int, int>> dtmfMap = {
    {'1', {697, 1209}}, {'2', {697, 1336}}, {'3', {697, 1477}}, {'A', {697, 1633}},
    {'4', {770, 1209}}, {'5', {770, 1336}}, {'6', {770, 1477}}, {'B', {770, 1633}},
    {'7', {852, 1209}}, {'8', {852, 1336}}, {'9', {852, 1477}}, {'C', {852, 1633}},
    {'*', {941, 1209}}, {'0', {941, 1336}}, {'#', {941, 1477}}, {'D', {941, 1633}}
};

// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    auto &audio = taf_Audio::GetInstance();
    audio.gCallbackPromise = promise<pa_result_t>();
}

/**
 * Returns audio instance
 */
taf_Audio &taf_Audio::GetInstance()
{
    static taf_Audio instance;
    return instance;
}

void taf_Audio::BuBStatusCB(int32_t status, void *contextptr)
{
    LE_INFO("BuBStatusCB %d", status);
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_NIL(!audioVhal.isAudioDrvAvailable(), "Audio driver is not installed!");
    hal_audio_bubStatus_t bubStatus = HAL_AUDIO_BUB_STATUS_NOT_IN_USE;
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        bubStatus = HAL_AUDIO_BUB_STATUS_IN_USE;
    }
    // Send Bub status to audio VHAL
    audioVhal.CtlReportBubStatus(bubStatus);
    LE_INFO("CtlReportBubStatus bubStatus %d", bubStatus);
}

/**
 * Advertise the audio service.
 */
void taf_Audio::AdvertiseAndRegisterHandler()
{
    taf_audio_AdvertiseService();
    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( taf_audio_GetServiceRef(),
                                   ClientSessionCloseEventHandler,
                                   NULL );
}

/**
 * Load Audio VHAL until retry count reaches the maximum value.
 */
void taf_Audio::VhalRetryHandler(le_timer_Ref_t timerRef)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    auto &audio = taf_Audio::GetInstance();
    uint32_t expiryCount = le_timer_GetExpiryCount(timerRef);
    LE_INFO("expiryCount = %d", expiryCount);
    if (expiryCount < MAX_NUM_OF_VHAL_LOAD_ATTEMPTS)
    {
        // Load Audio VHAL. IF failed to load Audio VHAL, try again later.
        if(!audioVhal.isAudioDrvAvailable())
        {
            le_result_t res = audioVhal.LoadDriver();
            if(res == LE_OK){
                LE_INFO("Audio VHAL loaded successfully, advertise the service.");
                // Loaded VHAL successfully. Delete the timer.
                le_timer_Delete(timerRef);
                audio.vhalRetryTimer = nullptr;
                audio.AdvertiseAndRegisterHandler();
            }
        }
        return;
    }
    if(!audioVhal.isAudioDrvAvailable())
    {
        //Load Audio VHAL timeout, continue without Audio VHAL module.
        LE_ERROR("Failed to load Audio VHAL module after all retry attempts!");
    }
    le_timer_Delete(timerRef);
    audio.vhalRetryTimer = nullptr;
    // Advertise service after max retries to load VHAL, service to continue without VHAL.
    LE_INFO("Advertise services after max tries to load VHAL, service to continue without VHAL");
    audio.AdvertiseAndRegisterHandler();
    audioVhal.AdvertiseVendorService();

}

/*
 * Try to connect to MPMS for 3 times with 1 sec interval.
 */
void taf_Audio::MpmsConnectHandler(le_timer_Ref_t timerRef)
{
    auto &audio = taf_Audio::GetInstance();
    uint32_t expiryCount = le_timer_GetExpiryCount(timerRef);
    le_result_t result = taf_mngdPm_TryConnectService();
    LE_INFO("MPMS retry attempt %d/3, mpmsTimerRepeat is %d", expiryCount, audio.mpmsTimerRepeat);
    if (result == LE_OK)
    {
        LE_INFO("Successfully connected to MPMS");
        audio.bubHandlerRef = taf_mngdPm_AddInfoReportHandler(
            TAF_MNGDPM_INFO_REPORT_BIT_MASK_BUB_STATUS, audio.BuBStatusCB, nullptr);
        if (audio.bubHandlerRef == nullptr)
        {
            LE_ERROR("Failed to register for BuB status");
        }
        // Register disconnect handler for future disconnections
        taf_mngdPm_SetNonExitServerDisconnectHandler(MpmsDisconnectHandler, nullptr);
        // Delete the timers
        le_timer_Delete(audio.mpmsRetryTimer);
        audio.mpmsRetryTimer = nullptr;
        audio.mpmsTimerRepeat = 0;
        if (audio.mpmsDelayTimer)
        {
            le_timer_Delete(audio.mpmsDelayTimer);
            audio.mpmsDelayTimer = nullptr;
        }
    }
    else if (expiryCount == MAX_NUM_OF_MPMS_CONN_ATTEMPTS
            && audio.mpmsTimerRepeat < MAX_NUM_OF_MPMS_TIMER_REPEAT)
    {
        LE_INFO("Stop timer and start after 32secs");
        le_timer_Stop(audio.mpmsRetryTimer);
        if (audio.mpmsDelayTimer == nullptr)
        {
            audio.mpmsDelayTimer = le_timer_Create("mpmsDelayTimer");
            if (audio.mpmsDelayTimer == nullptr)
            {
                LE_ERROR("Failed to create MPMS retry timer");
                return;
            }
            le_timer_SetHandler(audio.mpmsDelayTimer, MpmsDelayHandler);
            le_timer_SetWakeup(audio.mpmsDelayTimer, false);
            // Try again for 3 times after 32 secs interval
            le_timer_SetMsInterval(audio.mpmsDelayTimer, MPMS_DELAY_TIMER_INTERVAL);
        }
        le_timer_Start(audio.mpmsDelayTimer);
        audio.mpmsTimerRepeat++;
    }
    else if (expiryCount >= MAX_NUM_OF_MPMS_CONN_ATTEMPTS
            && audio.mpmsTimerRepeat >= MAX_NUM_OF_MPMS_TIMER_REPEAT)
    {
        LE_ERROR("Failed to connect to MPMS after maximum retries");
        le_timer_Stop(audio.mpmsRetryTimer);
        le_timer_Delete(audio.mpmsRetryTimer);
        audio.mpmsRetryTimer = nullptr;
        audio.mpmsTimerRepeat = 0;
        if (audio.mpmsDelayTimer)
        {
            le_timer_Delete(audio.mpmsDelayTimer);
            audio.mpmsDelayTimer = nullptr;
        }
    }
    else
    {
        LE_INFO("Retrying MPMS connection...");
    }
    return;
}

/*
 * Start MPMS connect timer again after 32secs.
 */
void taf_Audio::MpmsDelayHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("Stop delay timer and start retry timer");
    auto &audio = taf_Audio::GetInstance();
    le_timer_Stop(timerRef);
    audio.StartMpmsRetryTimer();
}

void taf_Audio::Init(void)
{
    LE_INFO("taf_Audio: Init");

    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();

    pa_result_t res = taf_pa_audio_Init();
    if (res != PA_OK)
    {
        LE_FATAL("Cannot initialize audio platform adaptor, err : %d", res);
    }
    else
    {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_INFO("Elapsed Time for Audio Subsystems to ready : %f", elapsedTime.count());

        // Create subsystem state change event and register handler
        subsystemStateEventId = le_event_CreateId("SubsystemStateEvent",
                sizeof(tafpa::audio::SubsystemState_e));
        subsystemStateHandlerRef = le_event_AddHandler("SubsystemStateHandler",
                subsystemStateEventId, SubsystemStateChangeHandler);
        // Register for audio subsystem state changes
        pa_result_t stateListenerResult = AddSubsystemStateChangeListener(
            SubsystemStateChangeCallback, nullptr, subSystemStatusChangelistenerId);

        if (stateListenerResult == PA_OK) {
            LE_INFO("Successfully registered for subsystem state change notifications (ID: %d)",
                    subSystemStatusChangelistenerId);
        } else {
            LE_ERROR("Failed to register subsystem state change listener, err: %d",
                    stateListenerResult);
        }
    }

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, taf_Audio::TafSigTermEventHandler);

    // Load audio VHAL driver
    auto &audioVhal = taf_AudioVhal::GetInstance();
    res = audioVhal.LoadDriver();

    if(res == LE_OK)
    {
        AdvertiseAndRegisterHandler();

        isVhalAvailable = audioVhal.isAudioDrvAvailable();
        LE_INFO("isVhalAvailable : %s", isVhalAvailable ? "true" : "false");

    }
    else // Failed to load the driver.
    {
        // Start one timer for the retry-action
        vhalRetryTimer = le_timer_Create("retry-timer-audiovhal");

        if (vhalRetryTimer == NULL)
        {
            LE_ERROR("Failed to le_timer_Create for the retry-timer");
            return;
        }

        le_timer_SetRepeat(vhalRetryTimer, MAX_NUM_OF_VHAL_LOAD_ATTEMPTS);
        le_timer_SetHandler(vhalRetryTimer, VhalRetryHandler);
        le_timer_SetWakeup(vhalRetryTimer, false);
        le_timer_SetMsInterval(vhalRetryTimer, VHAL_RETRY_TIMER_INTERVAL);
        le_timer_Start(vhalRetryTimer);
        LE_INFO("Retry timer for loading Audio VHAL start ...");
    }

    ConnectorPool  = le_mem_InitStaticPool(tafAudioConnector, MAX_CONNECTOR,
            sizeof(taf_audio_Connector_t));
    ConnectorRefMap = le_ref_CreateMap("TafAudioConnMap", MAX_CONNECTOR);
    HashMapPool  = le_mem_InitStaticPool(tafAudioHashmap, HASHMAP_SIZE,
            sizeof(taf_audio_hashMapList_t));
    StreamPool  = le_mem_InitStaticPool(tafAudioStream, MAX_STREAM,
            sizeof(taf_audio_Stream_t));
    le_mem_SetDestructor(StreamPool, DestructStream);
    StreamRefMap = le_ref_CreateMap("TafAudioStreamMap", MAX_STREAM);
    RoutePool  = le_mem_InitStaticPool(tafAudioRoute, MAX_ROUTE,
            sizeof(taf_audio_Route_t));
    RouteRefMap = le_ref_CreateMap("TafAudioRouteMap", MAX_ROUTE);
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef, MAX_STREAM,
            sizeof(taf_SessionRefNode_t));
    EventIdPool = le_mem_InitStaticPool(tafEventIdPool, MAX_STREAM, sizeof(tafEventIdList));
    EventHandlerRefNodePool = le_mem_InitStaticPool(tafEventHandlerRef, MAX_CONNECTOR,
            sizeof(EventHandlerRefNode_t));
    EventHandlerRefMap = le_ref_CreateMap("TAFEventHandlerMap", MAX_CONNECTOR);

    mRecordSemRef = le_sem_Create("tafRecordSem", 0);
    mRxRecordSemRef = le_sem_Create("tafRxRecordSem", 0);

    HashMapList = LE_DLS_LIST_INIT;

    le_sem_Ref_t mEventRegSemRef = le_sem_Create("mEventRegSemRef", 0);
    bufferHandlingThreadRef = le_thread_Create("RegisterBufferEventThread", RegisterBufferEvent,
            mEventRegSemRef);
    le_thread_SetJoinable(bufferHandlingThreadRef);
    le_thread_Start(bufferHandlingThreadRef);
    le_sem_Wait(mEventRegSemRef);
    le_sem_Delete(mEventRegSemRef);

    le_cfg_IteratorRef_t procCfg;
    procCfg = le_cfg_CreateReadTxn(AUDIO_SVC_PROC_CONFIG_PATH);
    if(procCfg != NULL) {
        maxFileBytes = le_cfg_GetInt(procCfg, MAX_FILE_BYTES_NODE_NAME, DEFAULT_MAX_FILE_BYTES);
        if (!le_cfg_NodeExists(procCfg, MAX_FILE_BYTES_NODE_NAME))
        {
            LE_INFO("Configured resource limit maxFileBytes is not available.");
        }

        if (le_cfg_IsEmpty(procCfg, MAX_FILE_BYTES_NODE_NAME))
        {
            LE_WARN("Configured resource limit maxFileBytes is empty");
        }

        if (le_cfg_GetNodeType(procCfg, MAX_FILE_BYTES_NODE_NAME) != LE_CFG_TYPE_INT)
        {
            LE_ERROR("Configured resource limit is the wrong type");
        }
        le_cfg_CancelTxn(procCfg);
    } else {
        LE_ERROR("Failed to get the proc config for tafAudioSvc");
    }
    LE_DEBUG("maxFileBytes is %d", maxFileBytes);
    le_result_t result = taf_mngdPm_TryConnectService();

    if(result == LE_OK)
    {
        bubHandlerRef = taf_mngdPm_AddInfoReportHandler(TAF_MNGDPM_INFO_REPORT_BIT_MASK_BUB_STATUS,
                BuBStatusCB, NULL);
        if(bubHandlerRef == NULL)
        {
            LE_ERROR("Failed to register for BuB status");
        }
        // Register disconnect handler for future disconnections
        taf_mngdPm_SetNonExitServerDisconnectHandler(MpmsDisconnectHandler, nullptr);
    }
    else
    {
        LE_ERROR("Failed to connect to MPMS with result %d", result);
        StartMpmsRetryTimer();
    }
}

void taf_Audio::ClientSessionCloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    le_result_t result = LE_FAULT;
    LE_DEBUG("ClientSessionCloseEventHandler sessionRef : %p", sessionRef);

    if(audio.mDtmfStarted && audio.dtmfDataRx.sessionRef == sessionRef)
    {
        LE_DEBUG("Stop on going DTMF");
        result = audio.StopDtmf(audio.dtmfDataRx.streamRef);
        TAF_ERROR_IF_RET_NIL( result != LE_OK, "Failed to stop dtmf");
    }
    if(audio.mDtmfStartedTx && audio.dtmfDataTx.sessionRef == sessionRef)
    {
        LE_DEBUG("Stop on going DTMF signalling");
        result = audio.StopSignallingDtmf(audio.dtmfDataTx.slotId);
        TAF_ERROR_IF_RET_NIL( result != LE_OK, "Failed to stop dtmf signalling");
    }

    // Close audio streams
    // This is a two stage process: parse audio stream reference map
    // once in order to close dsp frontend file play/capture streams
    // first, then parse it a second time to close remaining streams.
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(audio.StreamRefMap);
    bool isSessionMatched = false;
    taf_SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* lPtr;
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Stream_t* audioStreamPtr =
                (taf_audio_Stream_t*) le_ref_GetValue(iteratorRef);
        lPtr = le_dls_Peek(&(audioStreamPtr->sessionRefList));
        while (lPtr != NULL)
        {
            sessionRefNodePtr = CONTAINER_OF(lPtr, taf_SessionRefNode_t, refNodeLink);
            lPtr = le_dls_PeekNext(&(audioStreamPtr->sessionRefList), lPtr);
            if ( sessionRefNodePtr->sessionRef == sessionRef )
            {
                LE_DEBUG("StopAudio for audioStreamPtr %p", audioStreamPtr);
                isSessionMatched = true;
                break;
            }
        }
        if (isSessionMatched && audioStreamPtr
                && ((audioStreamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
                || (audioStreamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)))
        {
            audio.StopAudio(audioStreamPtr);
            audio.DeleteAudioStream(audioStreamPtr);
        }
    }

    iteratorRef = le_ref_GetIterator(audio.RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr && routePtr->sessionRef == sessionRef )
        {
            LE_INFO("CloseRoute for %p routeRef", iteratorRef);
            audio.CloseRoute(routePtr->routeRef);
            break;
        }
    }

    // Close audio streams
    iteratorRef = le_ref_GetIterator(audio.StreamRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Stream_t* streamPtr =
                (taf_audio_Stream_t*) le_ref_GetValue(iteratorRef);
        lPtr = le_dls_Peek(&(streamPtr->sessionRefList));
        while (lPtr != NULL)
        {
            sessionRefNodePtr = CONTAINER_OF(lPtr, taf_SessionRefNode_t, refNodeLink);
            lPtr = le_dls_PeekNext(&(streamPtr->sessionRefList), lPtr);
            if ( sessionRefNodePtr->sessionRef == sessionRef )
            {
                isSessionMatched = true;
                break;
            }
        }
        if(isSessionMatched && streamPtr) {
            audio.ReleaseStream(streamPtr, sessionRef, true);
        }
    }

    iteratorRef = le_ref_GetIterator(audio.ConnectorRefMap);

    result = le_ref_NextNode(iteratorRef);
    // Close connectors
    while ( result == LE_OK )
    {
        taf_audio_ConnectorRef_t connectorRef =
                (taf_audio_ConnectorRef_t) le_ref_GetSafeRef(iteratorRef);
        taf_audio_Connector_t* connectorPtr =
                (taf_audio_Connector_t*)le_ref_Lookup(audio.ConnectorRefMap, connectorRef);
        if (NULL == connectorPtr)
        {
            LE_ERROR("Invalid reference (%p) provided!", connectorRef);
            return;
        }

        // Get the next value in the reference maps (before releasing the node)
        result = le_ref_NextNode(iteratorRef);

        // Check if the session reference saved matchs with the current session reference.
        if (connectorPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Delete connector %p", connectorRef);
            taf_audio_DeleteConnector( connectorRef );
        }
    }
}

char taf_Audio::getDTMFChar(int lowFreq, int highFreq) {
    for (const auto& itr : dtmfMap) {
        if ((itr.second.first == lowFreq) &&
            (itr.second.second == highFreq)) {
            return itr.first;
        }
    }

    return '\0';
}

void tafDtmfListener::onDtmfToneDetection(tafpa::audio::PaDtmfTone dtmfTone) {
    LE_DEBUG("Dtmf Tone Detected");
    auto &audio = taf_Audio::GetInstance();
    LE_DEBUG("Direction is %d",static_cast<uint32_t>(dtmfTone.direction));
    LE_DEBUG("Low Frequency is %d",uint32_t(dtmfTone.lowFreq));
    LE_DEBUG("High Frequency is %d",uint32_t(dtmfTone.highFreq));
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)audio.mDtmfAudioRef;
    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_DTMF_DETECTION;
    streamEvent.event.dtmf = audio.getDTMFChar(dtmfTone.lowFreq, dtmfTone.highFreq);
    le_event_Report(streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));
}

size_t HashRef
(
 const void* safeRefPtr
)
{
    return (size_t) safeRefPtr;
}

static bool EqualsRef
(
 const void* firstRef,
 const void* secondRef
)
{
    return firstRef == secondRef;
}

le_hashmap_Ref_t taf_Audio::GetHashMap
(
 void
)
{
    taf_audio_hashMapList_t* currentPtr = NULL;

    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);

    while (lPtr != NULL)
    {
        currentPtr = CONTAINER_OF(lPtr, taf_audio_hashMapList_t, hashMapLink);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one HashMap unused (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = true;
            return currentPtr->hashMapRef;
        }
        lPtr = le_dls_PeekNext(&HashMapList, lPtr);
    }

    currentPtr = (taf_audio_hashMapList_t*)le_mem_ForceAlloc(HashMapPool);

    char ConnMapName[20];

    snprintf( ConnMapName, 20, "ConnMap%d", (int)(le_dls_NumLinks(&HashMapList)+1) );

    currentPtr->hashMapRef = le_hashmap_Create(ConnMapName, HASHMAP_SIZE, HashRef, EqualsRef);
    currentPtr->isUsed = true;
    currentPtr->hashMapLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&HashMapList,&(currentPtr->hashMapLink));

    LE_DEBUG("Create a new HashMap (%p) %s", currentPtr->hashMapRef, ConnMapName);

    return currentPtr->hashMapRef;
}

void taf_Audio::ClearHashMap
(
    le_hashmap_Ref_t hashMapRef
)
{
    LE_ASSERT(hashMapRef);

    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);
    taf_audio_hashMapList_t* currentPtr;

    while (lPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(lPtr, taf_audio_hashMapList_t, hashMapLink);

        if (currentPtr->hashMapRef == hashMapRef)
        {
            LE_DEBUG("Release HashMap (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = false;
            return;
        }
        lPtr = le_dls_PeekNext(&HashMapList,lPtr);
    }

    LE_DEBUG("Nothing in HashMap to release");
    return;
}

void taf_Audio::DeleteHashMap
(
    taf_audio_Connector_t* connPtr
)
{

    TAF_ERROR_IF_RET_NIL(connPtr == NULL,  "connPtr is nullptr!");

    le_hashmap_It_Ref_t Iterator;
    taf_audio_Stream_t const * currentStreamPtr;

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

        if (currentStreamPtr != nullptr)
        {
            le_hashmap_Remove(currentStreamPtr->connList, connPtr);
        }
    }

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioOutList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

        if (currentStreamPtr != nullptr)
        {
            le_hashmap_Remove(currentStreamPtr->connList,connPtr);
        }
    }

    le_hashmap_RemoveAll(connPtr->audioInList);
    le_hashmap_RemoveAll(connPtr->audioOutList);

    ClearHashMap(connPtr->audioInList);
    ClearHashMap(connPtr->audioOutList);
}

/**
 * Close the connectors
 */
void taf_Audio::CloseConnectorPaths
(
    taf_audio_Connector_t*   connPtr
)
{
    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");

    LE_DEBUG("CloseConnectorPaths %p", connPtr);

    le_hashmap_It_Ref_t Iterator =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    taf_audio_Stream_t* currentStreamPtr;
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr=(taf_audio_Stream_t*)le_hashmap_GetValue(Iterator);

        if(currentStreamPtr != nullptr)
        {
            StopandDelete(currentStreamPtr, connPtr->audioOutList);
        }
    }
}

taf_audio_DtmfDetectorHandlerRef_t taf_Audio::AddDtmfDetectorHandler
(
 taf_audio_StreamRef_t               streamRef,
 taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
 void*                              ctxPtr
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, NULL,"streamPtr is nullptr!");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
            NULL, "Invalid stream reference");

    TAF_ERROR_IF_RET_VAL(!mVoiceEnabled1, NULL, "Voice stream is not active");

    LE_DEBUG("AddDtmfDetectorHandler");

    if(!mDtmfListener) {
        mDtmfListener = std::make_shared<tafDtmfListener>();
        pa_result_t result = taf_pa_audio_registerDtmfListener(mDtmfListener);
        if(result != PA_OK) {
            LE_ERROR("Request to register for DTMF detection failed error : %d", (int)result);
            return NULL;
        }
        LE_DEBUG("Request to Register Voice Listener Sent" );
        mDtmfAudioRef = (taf_audio_StreamRef_t)streamPtr;
    }

    return (taf_audio_DtmfDetectorHandlerRef_t)AddStreamEventHandler(
            streamPtr,
            (le_event_HandlerFunc_t) handlerPtr,
            TAF_AUDIO_BITMASK_DTMF_DETECTION,
            ctxPtr
            );
}

/**
 * Stop and delete the current stream
 */
le_result_t taf_Audio::StopandDelete
(
    taf_audio_Stream_t*    streamPtr,
    le_hashmap_Ref_t            streamListPtr
)
{
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    le_result_t   res = LE_OK;
    taf_audio_Stream_t* inputPtr = NULL;
    taf_audio_Stream_t* outputPtr = NULL;
    taf_audio_Stream_t* currentPtr = NULL;

    LE_DEBUG("StopandDelete stream.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        currentPtr=(taf_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        if(currentPtr != nullptr)
        {
            if (streamPtr->device)
            {
                inputPtr  = streamPtr;
                outputPtr = currentPtr;
            }
            else
            {
                inputPtr  = currentPtr;
                outputPtr = streamPtr;
            }
            LE_DEBUG("inputInterface.%d with outputInterface.%d",
                inputPtr->interface, outputPtr->interface);
        }
    }

    if(streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX
            || streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        res = StopAudio(streamPtr);
        res = DeleteAudioStream(streamPtr);
    }
    else if(streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_4
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_5) {
        if(inputPtr != NULL) {
            res = StopAudio(inputPtr);
            res = DeleteAudioStream(inputPtr);
        }
    }
    else if(streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_4
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_5) {
        if(outputPtr != NULL) {
            res = StopAudio(outputPtr);
            res = DeleteAudioStream(outputPtr);
        }
    }
    return res;
}

/**
 * Create new connector for input and output stream reference
 */
taf_audio_ConnectorRef_t taf_Audio::CreateConnector
(
    void
)
{
    taf_audio_Connector_t* newconnPtr =
            (taf_audio_Connector_t*)le_mem_ForceAlloc(ConnectorPool);

    newconnPtr->audioInList   = GetHashMap();
    newconnPtr->audioOutList  = GetHashMap();
    newconnPtr->sessionRef = taf_audio_GetClientSessionRef();
    newconnPtr->connRef = (taf_audio_Connector_t*)
            le_ref_CreateRef(ConnectorRefMap, newconnPtr);
    newconnPtr->connLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&ConnectorList, &(newconnPtr->connLink));
    return newconnPtr->connRef;
}

/**
 * Delete the connecter
 */
void taf_Audio::DeleteConnector
(
    taf_audio_ConnectorRef_t connectorRef
)
{
    taf_audio_Connector_t* connPtr =
            (taf_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connectorRef);

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "Invalid connector reference!");

    CloseConnectorPaths(connPtr);

    DeleteHashMap(connPtr);

    le_dls_Remove(&ConnectorList, &(connPtr->connLink));

    le_ref_DeleteRef(ConnectorRefMap, connectorRef);

    le_mem_Release(connPtr);
}

/**
 * Connects the audio stream to the connector
 */
le_result_t taf_Audio::Connect
(
    taf_audio_ConnectorRef_t connRef,
    taf_audio_StreamRef_t    streamRef
)
{
    le_hashmap_Ref_t lPtr = NULL;
    le_result_t res;
    taf_audio_Stream_t* sPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    taf_audio_Connector_t* connPtr =
            (taf_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connRef);

    TAF_ERROR_IF_RET_VAL( connPtr == NULL, LE_BAD_PARAMETER, "connPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL( sPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");

    LE_DEBUG("StreamRef.%p (@%p) Connect [%d] '%s' to connRef.%p", streamRef, sPtr,
            sPtr->interface, (sPtr->device) ? "input" : "output", connRef);

    if ( sPtr->device )
    {
        if (le_hashmap_ContainsKey(connPtr->audioInList, sPtr))
        {
            LE_ERROR("Already connected");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connPtr->audioInList, sPtr, sPtr);
            lPtr = connPtr->audioOutList;
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connPtr->audioOutList, sPtr))
        {
            LE_ERROR("Already connected");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connPtr->audioOutList, sPtr, sPtr);
            lPtr = connPtr->audioInList;
        }
    }

    if(sPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1
            || sPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2
            || sPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3
            || sPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_4
            || sPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_5)
    {
        mSpeaker = true;
    }
    else if(sPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1
            || sPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2
            || sPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3
            || sPtr->interface == TAF_AUDIO_IF_CODEC_MIC_4
            || sPtr->interface == TAF_AUDIO_IF_CODEC_MIC_5)
    {
        mMic = true;
    }
    else if(sPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        mModemRx = true;
    }
    else if(sPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        mModemTx = true;
    }

    le_hashmap_Put(sPtr->connList, connPtr, connPtr);

    if (le_hashmap_Size(lPtr) >= 1)
    {
        if ((res = ConnectStreamPaths (sPtr, lPtr)) != LE_OK)
        {
            return res;
        }
    }

    return LE_OK;
}

/**
 * Disconnects the audio stream from the connector
 */
void taf_Audio::Disconnect
(
    taf_audio_ConnectorRef_t connRef,
    taf_audio_StreamRef_t    streamRef
)
{
    taf_audio_Stream_t* streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    taf_audio_Connector_t* connPtr =
            (taf_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connRef);

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    LE_DEBUG("Disconnect stream.%p from connector.%p", streamRef, connRef);
    if(streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_4
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_5)
    {
        mSpeaker = false;
    } else if(streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_4
            || streamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_5)
    {
        mMic = false;
    } else if(streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) {
        mModemRx = false;
        mRxSlotId = PaSlotId::SLOT_ID_INVALID;
    }
    else if(streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) {
        mModemTx = false;
        mTxSlotId = PaSlotId::SLOT_ID_INVALID;
    }

    le_ref_IterRef_t iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr && routePtr->sessionRef == taf_audio_GetClientSessionRef()
                && routePtr->mode == TAF_AUDIO_VOICE_CALL)
        {
            if (streamPtr == routePtr->modemRxPtr)
            {
                LE_DEBUG("Disconnect for modemRx stream");
                routePtr->modemRxPtr = nullptr;
                break;
            } else if (streamPtr == routePtr->modemTxPtr)
            {
                LE_DEBUG("Disconnect for modemTx stream");
                routePtr->modemTxPtr = nullptr;
                break;
            }
        }
    }

    if (streamPtr->device)
    {
        if (le_hashmap_ContainsKey(connPtr->audioInList, streamPtr))
        {
            StopandDelete (streamPtr,connPtr->audioOutList);
            le_hashmap_Remove(connPtr->audioInList,streamPtr);
            le_hashmap_Remove(streamPtr->connList,connPtr);
        }
        else
        {
            LE_ERROR("Not linked to the connector");
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connPtr->audioOutList, streamPtr))
        {
            StopandDelete (streamPtr,connPtr->audioInList);
            le_hashmap_Remove(connPtr->audioOutList,streamPtr);
            le_hashmap_Remove(streamPtr->connList,connPtr);
        }
        else
        {
            LE_ERROR("Not linked to the connector");
        }
    }
}

/**
 * Close the Stream Reference
 */
void taf_Audio::Close
(
    taf_audio_StreamRef_t streamRef
)
{
    taf_audio_Stream_t*  streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);

    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    ReleaseStream(streamPtr, taf_audio_GetClientSessionRef(), false);
}

/**
 * Release stream reference
 */
void taf_Audio::ReleaseStream
(
    taf_audio_Stream_t*  streamPtr,
    le_msg_SessionRef_t sessionRef,
    bool allReferences
)
{
    taf_SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* lPtr;

    LE_DEBUG("interface %d, sessionRef %p", streamPtr->interface, sessionRef);

    lPtr = le_dls_Peek(&(streamPtr->sessionRefList));
    while (lPtr != NULL)
    {
        sessionRefNodePtr = CONTAINER_OF(lPtr, taf_SessionRefNode_t, refNodeLink);

        lPtr = le_dls_PeekNext(&(streamPtr->sessionRefList), lPtr);

        LE_DEBUG("sessionRef %p", sessionRefNodePtr->sessionRef);

        if ( sessionRefNodePtr->sessionRef == sessionRef )
        {
            le_dls_Remove(&(streamPtr->sessionRefList),
                    &(sessionRefNodePtr->refNodeLink));

            le_mem_Release(sessionRefNodePtr);

            LE_DEBUG("Release stream %d", streamPtr->interface);
            le_mem_Release(streamPtr);

            if (!allReferences)
            {
                return;
            }
        }
    }
}

/**
 * Setup for voice outstream
 */
taf_audio_StreamRef_t taf_Audio::OpenModemVoiceRx
(
    uint32_t slotId
)
{
    TAF_ERROR_IF_RET_VAL((mTxSlotId != PaSlotId::SLOT_ID_INVALID
            && mTxSlotId != static_cast<PaSlotId>(slotId)), NULL,
            "Invalid slotID, use same slotID for Rx and Tx");

    TAF_ERROR_IF_RET_VAL(slotId > static_cast<uint32_t>(PaSlotId::SLOT_ID_MAX), NULL,
            "slotId is greater than MAX slot ID, use valid slot ID");

    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (iteratorRef && le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_AUDIO_VOICE_CALL,
                NULL, "Stream not supported with the active route");
    }

    StreamConfig_t streamConfig;
    mRxSlotId = static_cast<PaSlotId>(slotId);
    streamConfig.HwDevice = true;
    streamConfig.interface = TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;

    return CreateStream(&streamConfig);
}

/**
 * Setup for voice instream
 */
taf_audio_StreamRef_t taf_Audio::OpenModemVoiceTx
(
    uint32_t slotId, bool enableEcnr
)
{

    TAF_ERROR_IF_RET_VAL((mRxSlotId != PaSlotId::SLOT_ID_INVALID
            && mRxSlotId != static_cast<PaSlotId>(slotId)), NULL,
            "Invalid slotID, use same slotID for Rx and Tx");

    TAF_ERROR_IF_RET_VAL(slotId > static_cast<uint32_t>(PaSlotId::SLOT_ID_MAX), NULL,
            "slotId is greater than MAX slot ID, use valid slot ID");

    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_AUDIO_VOICE_CALL,
                NULL, "Stream not supported with the active route");
    }

    mTxSlotId = static_cast<PaSlotId>(slotId);
    isEcnrEnabled = enableEcnr;
    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.interface = TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;

    return CreateStream(&streamConfig);
}

/**
 * Create stream for input and output stream reference
 * if not opened already
 */
taf_audio_StreamRef_t taf_Audio::CreateStream
(
    StreamConfig_t* streamConfPtr
)
{
    LE_DEBUG("Create audio stream (%d)", streamConfPtr->interface);
    bool isOpened = false;
    taf_audio_Stream_t* streamPtr = NULL;
    le_ref_IterRef_t iterRef;
    taf_SessionRefNode_t* newSessionRefPtr;

    if (streamConfPtr->HwDevice)
    {
        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(StreamRefMap);

        while (!isOpened && (le_ref_NextNode(iterRef) == LE_OK))
        {
            streamPtr = (taf_audio_Stream_t*) le_ref_GetValue(iterRef);

            if (streamPtr && streamPtr->interface == streamConfPtr->interface
                    && streamPtr->direction == streamConfPtr->direction)
            {
                LE_INFO("streamPtr->interface %d", streamPtr->interface);
                isOpened = true;
            }
        }
    }

    if ( !isOpened )
    {
        streamPtr = (taf_audio_Stream_t*)le_mem_ForceAlloc(StreamPool);

        InitStream(streamPtr);

        streamPtr->interface = streamConfPtr->interface;
        streamPtr->device = !CHECK_OUTPUT_IF(streamPtr->interface);
        streamPtr->sessionRefList = LE_DLS_LIST_INIT;
        switch ( streamPtr->interface )
        {
            case TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
            case TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
                streamPtr->eventId = CreateEventId();
                streamPtr->direction = streamConfPtr->direction;
                streamPtr->volLevel = 0.000000;
                streamPtr->isMute = false;
                break;
            case TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
                streamPtr->eventId = CreateEventId();
                streamPtr->volLevel = 0.000000;
            case TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
                streamPtr->isMute = false;
                break;
            default:
                break;
        }
        streamPtr->streamRef = (taf_audio_StreamRef_t)
                le_ref_CreateRef(StreamRefMap, streamPtr);

        LE_DEBUG("Create streamRef %p of interface.%d",
                streamPtr->streamRef, streamPtr->interface);
    }
    else
    {
        le_mem_AddRef(streamPtr);

        LE_DEBUG("AddRef for streamRef %p of interface.%d",
                streamPtr->streamRef, streamPtr->interface);
    }

    newSessionRefPtr = (taf_SessionRefNode_t*)le_mem_ForceAlloc(SessionRefPool);
    newSessionRefPtr->sessionRef = taf_audio_GetClientSessionRef();
    newSessionRefPtr->refNodeLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&streamPtr->sessionRefList, &(newSessionRefPtr->refNodeLink));

    return streamPtr->streamRef;
}

le_event_Id_t taf_Audio::CreateEventId
(
)
{
    auto &audio = taf_Audio::GetInstance();
    tafEventIdList* curPtr = NULL;
    le_dls_Link_t*      linkPtr = le_dls_Peek(&audio.EventIdList);
    char                eventName[25];
    int32_t             eventId = 1;

    while (linkPtr!=NULL)
    {
        curPtr = CONTAINER_OF(linkPtr, tafEventIdList, next);

        if (!curPtr->inUse)
        {
            LE_DEBUG("unused eventId (%p)", curPtr->eventId);
            curPtr->inUse = true;
            return curPtr->eventId;
        }
        linkPtr = le_dls_PeekNext(&audio.EventIdList, linkPtr);

        eventId++;
    }

    snprintf(eventName, sizeof(eventName), "eventId-%d", eventId);

    curPtr = (tafEventIdList*)le_mem_ForceAlloc(audio.EventIdPool);
    curPtr->eventId = le_event_CreateId(eventName, sizeof(taf_audio_StreamEvent_t));
    curPtr->inUse = true;
    curPtr->next = LE_DLS_LINK_INIT;

    le_dls_Queue(&audio.EventIdList, &(curPtr->next));

    LE_INFO("Create a new eventId (%p)", curPtr->eventId);

    return curPtr->eventId;
}

/**
 * Initialize stream reference
 */
void taf_Audio::InitStream
(
    taf_audio_Stream_t* streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    memset(streamPtr, 0, sizeof(taf_audio_Stream_t));
    streamPtr->fd = -1;
    streamPtr->connList = GetHashMap();
}

static void DeleteEventId
(
 le_event_Id_t eventId
)
{
    auto &audio = taf_Audio::GetInstance();
    le_dls_Link_t* linkPtr = le_dls_Peek(&audio.EventIdList);

    while (linkPtr!=NULL)
    {
        tafEventIdList* curPtr = CONTAINER_OF(linkPtr,
                tafEventIdList, next);

        if (curPtr->eventId == eventId)
        {
            LE_DEBUG("Found eventId to release (%p)", curPtr->eventId);
            curPtr->inUse = false;
            return;
        }
        linkPtr = le_dls_PeekNext(&audio.EventIdList,linkPtr);
    }

    LE_DEBUG("Nothing to delete");
    return;
}

/**
 * Stream destructor
 */
void taf_Audio::DestructStream( void *objPtr )
{
    LE_DEBUG("DestructStream");
    LE_ASSERT(objPtr);
    auto &audio = taf_Audio::GetInstance();

    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)objPtr;

    // Close the active Route
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(audio.RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr && (routePtr->sinkRef == streamPtr || routePtr->sourceRef == streamPtr))
        {
            LE_INFO("CloseRoute for %p routeRef", iteratorRef);
            audio.CloseRoute(routePtr->routeRef);
            break;
        }
    }

    audio.DisconnectConnectors(streamPtr);

    le_hashmap_RemoveAll(streamPtr->connList);
    audio.ClearHashMap(streamPtr->connList);
    DeleteEventId(streamPtr->eventId);
    le_ref_DeleteRef(audio.StreamRefMap, streamPtr->streamRef);
}

/**
 * Disconnect the Stream from all connectors
 */
void taf_Audio::DisconnectConnectors
(
    taf_audio_Stream_t*     streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");
    LE_DEBUG("DisconnectConnectors streamPtr.%p", streamPtr);

    // Disconnect from all the connectors the stream is connected
    le_hashmap_It_Ref_t Iterator =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_audio_Connector_t const * currentconnPtr;

    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(Iterator);
        if(currentconnPtr != nullptr)
        {
            Disconnect(currentconnPtr->connRef, streamPtr->streamRef);
        }
    }
}

taf_audio_RouteRef_t taf_Audio::OpenRoute( taf_audio_RouteId_t routeId,
        taf_audio_Mode_t mode, taf_audio_StreamRef_t *sinkRef,
        taf_audio_StreamRef_t *sourceRef)
{
    LE_INFO("OpenRoute route : %d mode : %d", routeId, mode);

    TAF_ERROR_IF_RET_VAL(routeId < TAF_AUDIO_ROUTE_1 || routeId >= TAF_AUDIO_ROUTE_4, NULL,
            "Not supported or invalid Route ID!");

    le_ref_IterRef_t iterRef;
    taf_audio_Route_t* routePtr;
    iterRef = (le_ref_IterRef_t)le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        routePtr = (taf_audio_Route_t*) le_ref_GetValue(iterRef);
        TAF_ERROR_IF_RET_VAL(routePtr == NULL, NULL, "Invalid routePtr!");

        LE_ERROR("Another route is already active set true");
        return NULL;
    }
    routePtr = (taf_audio_Route_t*)le_mem_ForceAlloc(RoutePool);
    routePtr->routeId = routeId;
    routePtr->sessionRef = taf_audio_GetClientSessionRef();

    if(mode == TAF_AUDIO_VOICE_CALL)
    {
        routePtr->mode = TAF_AUDIO_VOICE_CALL;

        // Create Sink Ref
        StreamConfig_t sinkStreamConf;
        sinkStreamConf.HwDevice = true;
        if (routeId == TAF_AUDIO_ROUTE_1)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_1;
        else if (routeId == TAF_AUDIO_ROUTE_2)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_2;
        else if (routeId == TAF_AUDIO_ROUTE_3)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_3;
        else if (routeId == TAF_AUDIO_ROUTE_4)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_4;
        else if (routeId == TAF_AUDIO_ROUTE_5)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_5;
        *sinkRef = CreateStream(&sinkStreamConf);

        // Create Source Ref
        StreamConfig_t sourceStreamConf;
        sourceStreamConf.HwDevice = true;
        if (routeId == TAF_AUDIO_ROUTE_1)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_1;
        else if (routeId == TAF_AUDIO_ROUTE_2)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_2;
        else if (routeId == TAF_AUDIO_ROUTE_3)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_3;
        else if (routeId == TAF_AUDIO_ROUTE_4)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_4;
        else if (routeId == TAF_AUDIO_ROUTE_5)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_5;
        *sourceRef = CreateStream(&sourceStreamConf);

        routePtr->sinkRef = *sinkRef;
        routePtr->sourceRef = *sourceRef;
        paVoiceStreamConfig.deviceTypes.clear();
        paVoiceStreamConfig = {};
    } else if (mode == TAF_AUDIO_LOCAL_RECORDING)
    {
        // Create Source Ref
        StreamConfig_t sourceStreamConf;
        sourceStreamConf.HwDevice = true;
        if (routeId == TAF_AUDIO_ROUTE_1)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_1;
        else if (routeId == TAF_AUDIO_ROUTE_2)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_2;
        else if (routeId == TAF_AUDIO_ROUTE_3)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_3;
        else if (routeId == TAF_AUDIO_ROUTE_4)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_4;
        else if (routeId == TAF_AUDIO_ROUTE_5)
            sourceStreamConf.interface = TAF_AUDIO_IF_CODEC_MIC_5;
        *sourceRef = CreateStream(&sourceStreamConf);

        routePtr->mode = TAF_AUDIO_LOCAL_RECORDING;
        routePtr->sourceRef = *sourceRef;
    } else if (mode == TAF_AUDIO_LOCAL_PLAYBACK)
    {
        // Create Sink Ref
        StreamConfig_t sinkStreamConf;
        sinkStreamConf.HwDevice = true;
        if (routeId == TAF_AUDIO_ROUTE_1)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_1;
        else if (routeId == TAF_AUDIO_ROUTE_2)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_2;
        else if (routeId == TAF_AUDIO_ROUTE_3)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_3;
        else if (routeId == TAF_AUDIO_ROUTE_4)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_4;
        else if (routeId == TAF_AUDIO_ROUTE_5)
            sinkStreamConf.interface = TAF_AUDIO_IF_CODEC_SPEAKER_5;
        *sinkRef = CreateStream(&sinkStreamConf);

        routePtr->mode = TAF_AUDIO_LOCAL_PLAYBACK;
        routePtr->sinkRef = *sinkRef;
    } else if(mode == TAF_AUDIO_LOCAL_LOOPBACK)
    {
        //Setup loopback stream config.
        if(mIsLoopbackActive) {
            LE_ERROR("Loopback stream already exists. ");
            return NULL;
        }

        le_result_t res = LE_FAULT;
        PaStreamConfig config;

        config.type = PaStreamType::LOOPBACK;
        config.sampleRate = 48000;
        config.format = PaFileFormat::WAVE;
        config.channelTypeMask = PaChannelType::LEFT | PaChannelType::RIGHT;

        if (routeId == TAF_AUDIO_ROUTE_1) {
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_1);
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_1);
            LE_DEBUG("set config with device types speaker 1 & mic 1");
        } else if (routeId == TAF_AUDIO_ROUTE_2)
        {
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_2);
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_2);
            LE_DEBUG("set config with device types speaker 2 & mic 2");
        } else if (routeId == TAF_AUDIO_ROUTE_3)
        {
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_3);
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_3);
            LE_DEBUG("set config with device types speaker 3 & mic 3");
        } else if (routeId == TAF_AUDIO_ROUTE_4)
        {
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_4);
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_5);
            LE_DEBUG("set config with device types speaker 4 & mic 4");
        } else if (routeId == TAF_AUDIO_ROUTE_5)
        {
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_5);
            config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_5);
            LE_DEBUG("set config with device types speaker 5 & mic 5");
        }

        res = StartAudio(config);
        if (res != LE_OK) {
            LE_ERROR("Loopback stream failed");
            le_ref_DeleteRef(RouteRefMap, routePtr->routeRef);
            le_mem_Release(routePtr);
            return NULL;
        }
        routePtr->mode = TAF_AUDIO_LOCAL_LOOPBACK;
    }

    routePtr->routeRef = (taf_audio_RouteRef_t)le_ref_CreateRef(RouteRefMap, routePtr);
    LE_INFO("Route ref is %p", routePtr->routeRef);
    if(routePtr->mode == TAF_AUDIO_LOCAL_LOOPBACK) {
        // Set VHAL ctl route status for loopback
        setVhalRouteStatus(TAF_AUDIO_LOCAL_LOOPBACK, true);
    }
    return routePtr->routeRef;
}

le_result_t taf_Audio::CloseRoute( taf_audio_RouteRef_t routeRef )
{
    LE_INFO("CloseRoute %p", routeRef);

    taf_audio_Route_t*  routePtr =
            (taf_audio_Route_t*)le_ref_Lookup(RouteRefMap, routeRef);
    TAF_ERROR_IF_RET_VAL(routePtr == NULL, LE_BAD_PARAMETER, "Invalid Route ref!");

    if (routePtr->mode == TAF_AUDIO_VOICE_CALL) {
        // Set ctl status to VHAL before stoping voice call audio.
        setVhalRouteStatus(routePtr->mode, false);

        // Release sink and source reference
        taf_audio_Stream_t*  sinkStreamPtr =
                (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);

        taf_audio_Stream_t*  sourceStreamPtr =
                (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);

        TAF_ERROR_IF_RET_VAL( sinkStreamPtr == NULL || sourceStreamPtr == NULL, LE_FAULT,
                "sinkRef or sourceRef is invalid!");
        ReleaseStream(sinkStreamPtr, routePtr->sessionRef, false);
        ReleaseStream(sourceStreamPtr, routePtr->sessionRef, false);
    } else if (routePtr->mode == TAF_AUDIO_LOCAL_PLAYBACK)
    {
        // Release sink reference
        taf_audio_Stream_t*  sinkStreamPtr =
                (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);

        TAF_ERROR_IF_RET_VAL( sinkStreamPtr == NULL, LE_FAULT,
                "sinkRef is invalid!");
        ReleaseStream(sinkStreamPtr, routePtr->sessionRef, false);

    } else if (routePtr->mode == TAF_AUDIO_LOCAL_RECORDING)
    {
        // Release source reference
        taf_audio_Stream_t*  sourceStreamPtr =
                (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);

        TAF_ERROR_IF_RET_VAL( sourceStreamPtr == NULL, LE_FAULT,
                "sourceRef is invalid!");
        ReleaseStream(sourceStreamPtr, routePtr->sessionRef, false);
    } else if (routePtr->mode == TAF_AUDIO_LOCAL_LOOPBACK)
    {
        //Stop and delete loopback
        if(!mIsLoopbackActive){
            LE_ERROR("No loopback stream exists.");
            return LE_FAULT;
        } // add loopback bool

        CALLBACK_TO_SET_PA_RESULT;
        PaStreamConfig config = {};
        std::any context = std::make_shared<PaStreamConfig>(config);
        pa_result_t status = PA_OK;
        pa_result_t cbRes = PA_FAULT;
        config.type = PaStreamType::LOOPBACK;
        status = taf_pa_audio_StopAudio(config, cb, context);

        if (status != PA_OK) {
            LE_ERROR("Request to stop loopback failed error : %d", (int)status);
            return LE_FAULT;
        }

        cbRes = prom->get_future().get();
        if (cbRes != PA_OK) {
            LE_ERROR("Failed to stop loopback, error : %d", (int)cbRes);
            return LE_FAULT;
        }
        LE_DEBUG("Loopback stopped");

        CALLBACK1_TO_SET_PA_RESULT;
        status = taf_pa_audio_DeleteStream(config, cb1, context);

        if (status != PA_OK) {
            LE_ERROR("Request to delete loopback failed error : %d", (int)status);
            return LE_FAULT;
        }

        cbRes = prom1->get_future().get();
        if (cbRes != PA_OK) {
            LE_ERROR("Failed to delete loopback, error : %d", (int)cbRes);
            return LE_FAULT;
        }
        mIsLoopbackActive = false;
        LE_DEBUG("Loopback stream deleted");
        // Set VHAL ctl route status for loopback
        setVhalRouteStatus(routePtr->mode, false);
    }

    LE_DEBUG("Release routeRef %p", routeRef);
    le_ref_DeleteRef(RouteRefMap, routePtr->routeRef);
    le_mem_Release(routePtr);

    return LE_OK;
}

le_result_t taf_Audio::ConnectStreamPaths( taf_audio_Stream_t* streamPtr,
        le_hashmap_Ref_t streamListPtr )
{
    LE_INFO("ConnectStreamPaths");
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER, "streamPtr is nullptr!");
    taf_audio_Stream_t* inputPtr;
    taf_audio_Stream_t* outputPtr;
    taf_audio_Stream_t* currentPtr;
    le_result_t res = LE_FAULT;

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator) == LE_OK)
    {
        currentPtr = (taf_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        TAF_ERROR_IF_RET_VAL( currentPtr == nullptr, LE_BAD_PARAMETER, "currentPtr is nullptr!");

        LE_DEBUG("CurrentStream %p",currentPtr);

        if (streamPtr->device)
        {
            inputPtr  = streamPtr;
            outputPtr = currentPtr;
        }
        else
        {
            inputPtr  = currentPtr;
            outputPtr = streamPtr;
        }

        le_ref_IterRef_t iteratorRef = le_ref_GetIterator(RouteRefMap);
        while (le_ref_NextNode(iteratorRef) == LE_OK)
        {
            taf_audio_Route_t* routePtr =
                    (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
            if ( routePtr && routePtr->sessionRef == taf_audio_GetClientSessionRef()
                    && routePtr->mode == TAF_AUDIO_VOICE_CALL)
            {
                taf_audio_Stream_t* sinkPtr =
                        (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);
                if(le_hashmap_ContainsKey(streamListPtr, sinkPtr))
                {
                    if (inputPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
                    {
                        LE_DEBUG("Assign modem Rx to route ptr");
                        routePtr->modemRxPtr = inputPtr;
                        break;
                    }
                }
                taf_audio_Stream_t* sourcePtr =
                        (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);
                if(le_hashmap_ContainsKey(streamListPtr, sourcePtr))
                {
                    if (outputPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
                    {
                        LE_DEBUG("Assign modem Tx to route ptr");
                        routePtr->modemTxPtr = outputPtr;
                        break;
                    }
                }
            }
        }

        LE_DEBUG("Input [%d] and Output [%d] are tied together.",
                inputPtr->interface,
                outputPtr->interface);

        PaAudioIf outDevice = (PaAudioIf)-1, inDevice = (PaAudioIf)-1;
        // Set the config device type based on output device
        if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1) {
            outDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_1;
        }
        else if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2) {
            outDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_2;
        }
        else if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3) {
            outDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_3;
        }
        else if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_4) {
            outDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_4;
        }
        else if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_5) {
            outDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_5;
        }
        if(outDevice != -1) {
            bool isAvailable = false;
            for(PaAudioIf dev : paVoiceStreamConfig.deviceTypes) {
                if(dev == outDevice) {
                    isAvailable = true;
                    break;
                }
            }
            if(!isAvailable) {
                paVoiceStreamConfig.deviceTypes.emplace_back(outDevice);
                LE_DEBUG("set config with device type speaker %d", outDevice);
            }
        }

        // Set the config device type based on input device
        if (inputPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1) {
            inDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_1;
        }
        else if (inputPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2) {
            inDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_2;
        }
        else if (inputPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3) {
            inDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_3;
        }
        else if (inputPtr->interface == TAF_AUDIO_IF_CODEC_MIC_4) {
            inDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_4;
        }
        else if (inputPtr->interface == TAF_AUDIO_IF_CODEC_MIC_5) {
            inDevice = PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_5;
        }
        if(inDevice != -1) {
            bool isAvailable = false;
            for(PaAudioIf dev : paVoiceStreamConfig.deviceTypes) {
                if(dev == inDevice) {
                    isAvailable = true;
                    break;
                }
            }
            if(!isAvailable) {
                paVoiceStreamConfig.deviceTypes.emplace_back(inDevice);
                LE_DEBUG("set config with device type speaker %d", inDevice);
            }
        }

        if (mModemRx && mModemTx && mSpeaker && mMic && !mCallStarted)
        {
            paVoiceStreamConfig.type = PaStreamType::VOICE_CALL;
            paVoiceStreamConfig.slotId = PaSlotId::SLOT_ID_1;
            paVoiceStreamConfig.format = PaFileFormat::WAVE;
            paVoiceStreamConfig.channelTypeMask = PaChannelType::LEFT | PaChannelType::RIGHT;
            paVoiceStreamConfig.sampleRate = 16000;
           // paVoiceStreamConfig.formatParams = nullptr;

            if (isEcnrEnabled) {
                paVoiceStreamConfig.ecnrMode = true;
            } else {
                paVoiceStreamConfig.ecnrMode = false;
            }

            if(paVoiceStreamConfig.deviceTypes.size() >= 2)
            {
                res = StartAudio(paVoiceStreamConfig);
                le_ref_IterRef_t iteratorRef = le_ref_GetIterator(RouteRefMap);
                while (le_ref_NextNode(iteratorRef) == LE_OK)
                {
                    taf_audio_Route_t* routePtr =
                            (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
                    if ( routePtr && routePtr->sessionRef == taf_audio_GetClientSessionRef()
                            && routePtr->mode == TAF_AUDIO_VOICE_CALL)
                    {
                        SetVolume(routePtr->modemRxPtr->streamRef, routePtr->modemRxPtr->volLevel,
                                false);
                        if(routePtr->modemRxPtr->isMute)
                            SetMute(routePtr->modemRxPtr->streamRef, true);
                        if(routePtr->modemTxPtr->isMute)
                            SetMute(routePtr->modemTxPtr->streamRef, true);
                    }
                }
            }
            else {
                res = LE_OK;
                return res;
            }
        } else {
            res = LE_OK;
        }
    }
    return res;
}

le_result_t taf_Audio::StartAudio
(
    PaStreamConfig config
)
{
    LE_DEBUG("Create and Start audio\n");
    pa_result_t status = PA_FAULT;
    std::any context = std::make_shared<PaStreamConfig>(config);
    CALLBACK_TO_SET_PA_RESULT;
    pa_result_t audioStatus = taf_pa_audio_CreateStream(config, cb, context);
    if(audioStatus == PA_OK) {
        LE_DEBUG("Request to create stream sent" );
    } else {
        LE_ERROR("Request to create stream failed: %d", int(audioStatus));
        return LE_FAULT;
    }
    CALLBACK1_TO_SET_PA_RESULT;
    if (prom->get_future().get() == PA_OK) {
        LE_DEBUG("Audio Stream is Created" );
        if(config.type == PaStreamType::VOICE_CALL) {
            LE_DEBUG("Voice Stream is Created on slot id");

            if((config.slotId == PaSlotId::SLOT_ID_1) && !mVoiceEnabled1) {
                status = taf_pa_audio_StartAudio(config, cb1, context);
                paVoiceStreamConfig = {};
                if (status == PA_OK) {
                    LE_DEBUG("Request to start voice stream sent");
                    pa_result_t startRes = prom1->get_future().get();
                    if (PA_OK != startRes) {
                        LE_ERROR("Request to start failed error : %d", (int)startRes);
                        return LE_FAULT;
                    }
                    mVoiceEnabled1 = true;
                    mCallStarted = true;
                    // Set VHAL ctl route status for voice call
                    setVhalRouteStatus(TAF_AUDIO_VOICE_CALL, true);
                } else {
                    LE_ERROR("Request to start voice stream failed.\n");
                    return LE_FAULT;
                }
            }
        } else if(config.type == PaStreamType::CAPTURE) {
            for(PaStreamDirection dir : config.streamDir) {
                if(dir == PaStreamDirection::RX) {
                    LE_DEBUG("Create stream for incall downlink recording");
                    return LE_OK;
                }
            }
            LE_DEBUG("Audio Capture Stream is Created" );
            setVhalRouteStatus(TAF_AUDIO_LOCAL_RECORDING, true);
        } else if(config.type == PaStreamType::LOOPBACK) {
            LE_DEBUG("Audio Loopback Stream is Created" );
            //Start loopback stream.
            status = taf_pa_audio_StartAudio(config, cb1, context);
            if (status != PA_OK) {
                LE_ERROR("Request to start loopback failed.\n");
                return LE_FAULT;
            }
            status = prom1->get_future().get();
            if (status != PA_OK) {
                LE_ERROR("start loopback failed error : %d", (int)status);
                return LE_FAULT;
            }
            LE_DEBUG("Loopback started\n");
            mIsLoopbackActive = true;
        } else {
            LE_DEBUG("Unknown Stream Created" );
        }
    } else {
        LE_ERROR("Invalid configuration, failed to create the stream");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Get the player interface
 */
taf_audio_StreamRef_t taf_Audio::OpenPlayer
(
    taf_audio_Direction_t direction
)
{
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if (direction == TAF_AUDIO_TX)
            TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_AUDIO_VOICE_CALL,
                    NULL, "Stream supported with the active voice call route");
        if (direction == TAF_AUDIO_RX)
            TAF_ERROR_IF_RET_VAL(routePtr && !(routePtr->mode == TAF_AUDIO_VOICE_CALL
                    || routePtr->mode == TAF_AUDIO_LOCAL_PLAYBACK),
                    NULL, "Stream supported with the active voice call route or local playback");
    }

    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.direction = direction;
    streamConfig.interface = TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;

    return CreateStream(&streamConfig);
}

/**
 * Get the recorder interface
 */
taf_audio_StreamRef_t taf_Audio::OpenRecorder
(
    taf_audio_Direction_t direction
)
{
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if (direction == TAF_AUDIO_RX)
            TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_AUDIO_VOICE_CALL,
                    NULL, "Stream supported with the active voice call route");
        if (direction == TAF_AUDIO_TX)
            TAF_ERROR_IF_RET_VAL(routePtr && !(routePtr->mode == TAF_AUDIO_VOICE_CALL
                    || routePtr->mode == TAF_AUDIO_LOCAL_RECORDING),
                    NULL, "Stream supported with the active voice call route or local recording");
    }

    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.direction = direction;
    streamConfig.interface = TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;

    return CreateStream(&streamConfig);
}

taf_audio_MediaHandlerRef_t taf_Audio::AddMediaHandler
(
    taf_audio_StreamRef_t streamRef,
    taf_audio_MediaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    taf_audio_Stream_t* streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), NULL, "Invalid reference");

    LE_INFO("AddMediaHandler %d", streamPtr->interface);

    if ((streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
            && (streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (taf_audio_MediaHandlerRef_t)AddStreamEventHandler(streamPtr,
            (le_event_HandlerFunc_t)handlerPtr, TAF_AUDIO_BITMASK_MEDIA_EVENT,
            (void*)contextPtr);
}

StreamEventHandlerRef_t taf_Audio::AddStreamEventHandler
(
    taf_audio_Stream_t* sPtr,
    le_event_HandlerFunc_t handlerPtr,
    taf_audio_StreamEventBitMask_t streamEventBitMask,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL((sPtr == NULL) || (handlerPtr == NULL), NULL,
            "Invalid reference/handle");

    LE_DEBUG("Add handler on interface %d.", sPtr->interface);

    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_mem_ForceAlloc(
            EventHandlerRefNodePool);
    streamRefNodePtr->streamEventMask = streamEventBitMask;

    streamRefNodePtr->next = LE_DLS_LINK_INIT;

    streamRefNodePtr->streamPtr = sPtr;

    handlerRef = le_event_AddLayeredHandler("StreamEventHandler", sPtr->eventId,
                                       FirstLayerEventHandler, (void*)handlerPtr);
    streamRefNodePtr->userCtx = contextPtr;

    le_event_SetContextPtr(handlerRef, streamRefNodePtr);
    streamRefNodePtr->handlerRef = handlerRef;

    le_dls_Queue(&(sPtr->streamRefWithEventHdlrList),&(streamRefNodePtr->next));
    streamRefNodePtr->streamHandlerRef = (StreamEventHandlerRef*)
            le_ref_CreateRef(EventHandlerRefMap, streamRefNodePtr );
    return streamRefNodePtr->streamHandlerRef;
}

void taf_Audio::FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc )
{
    taf_audio_StreamEvent_t* streamEventPtr = (taf_audio_StreamEvent_t*)reportPtr;
    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_event_GetContextPtr();
    LE_DEBUG("FirstLayerEventHandler");

    TAF_ERROR_IF_RET_NIL((!streamRefNodePtr) || (!streamEventPtr) || (!streamEventPtr->streamPtr),
            "Invalid reference");

    taf_audio_Stream_t* streamPtr = streamEventPtr->streamPtr;

    switch ( streamEventPtr->streamEvent )
    {
        case TAF_AUDIO_BITMASK_MEDIA_EVENT:
            {
                taf_audio_MediaEvent_t mediaEvent = streamEventPtr->event.mediaEvent;

                LE_DEBUG("MediaEvent %d", mediaEvent);

                if (streamPtr->playFile)
                {
                    if (mediaEvent == TAF_AUDIO_MEDIA_NO_MORE_SAMPLES)
                    {
                        mediaEvent = TAF_AUDIO_MEDIA_ENDED;
                    }
                }

                taf_audio_MediaHandlerFunc_t clientHandlerFunc =
                        (taf_audio_MediaHandlerFunc_t)secondLayerHandlerFunc;
                LE_DEBUG("Notify client handler function");
                clientHandlerFunc(streamPtr->streamRef, mediaEvent, streamRefNodePtr->userCtx);
            }
            break;
        case TAF_AUDIO_BITMASK_DTMF_DETECTION:
            {
                taf_audio_DtmfDetectorHandlerFunc_t clientHandlerFunc = (
                        taf_audio_DtmfDetectorHandlerFunc_t)secondLayerHandlerFunc;
                clientHandlerFunc(streamPtr->streamRef,
                        streamEventPtr->event.dtmf,
                        streamRefNodePtr->userCtx);
            }
            break;
    }
}

void taf_Audio::RemoveMediaHandler
(
    taf_audio_MediaHandlerRef_t handlerRef
)
{
    RemoveStreamEventHandler( (StreamEventHandlerRef_t) handlerRef );
}

void taf_Audio::RemoveDtmfDetectorHandler
(
 taf_audio_DtmfDetectorHandlerRef_t handlerRef
)
{
    if (mDtmfAudioRef && mVoiceEnabled1 && mDtmfListener) {
        RemoveStreamEventHandler( (StreamEventHandlerRef_t) handlerRef );
        if (mDtmfAudioRef->dtmfEventHandler == NULL)
        {
            pa_result_t result = taf_pa_audio_deregisterDtmfListener(mDtmfListener);
            if(result != PA_OK) {
                LE_ERROR("Request to deregister for DTMF detection failed error : %d", (int)result);
                return;
            }
            mDtmfAudioRef = NULL;
            mDtmfListener = nullptr;
        }
    }
}

void taf_Audio::RemoveStreamEventHandler
(
    StreamEventHandlerRef_t handlerRef
)
{
    EventHandlerRefNode_t* streamRefNodePtr =
            (EventHandlerRefNode_t*)le_ref_Lookup(EventHandlerRefMap, handlerRef);

    TAF_ERROR_IF_RET_NIL(streamRefNodePtr == NULL, "Invalid hnadlerRef(%p)", streamRefNodePtr);

    taf_audio_Stream_t* streamPtr = streamRefNodePtr->streamPtr;

    le_event_RemoveHandler((le_event_HandlerRef_t)streamRefNodePtr->handlerRef);

    le_ref_DeleteRef(EventHandlerRefMap, streamRefNodePtr->streamHandlerRef);

    if (streamRefNodePtr->streamEventMask == TAF_AUDIO_BITMASK_DTMF_DETECTION)
    {
        uint32_t handlerCount = 0;

        le_dls_Link_t* linkPtr = le_dls_Peek(&(streamPtr->streamRefWithEventHdlrList));

        while (linkPtr != NULL)
        {
            EventHandlerRefNode_t* nodePtr;

            nodePtr = CONTAINER_OF(linkPtr, EventHandlerRefNode_t, next);

            linkPtr = le_dls_PeekNext(&streamPtr->streamRefWithEventHdlrList, linkPtr);

            if (nodePtr->streamEventMask == TAF_AUDIO_BITMASK_DTMF_DETECTION)
            {
                handlerCount++;
            }
        }

        LE_DEBUG("handlerCount %d", handlerCount);
        if (handlerCount == 1)
        {
            streamPtr->dtmfEventHandler = NULL;
        }
    }

    le_dls_Remove(&streamPtr->streamRefWithEventHdlrList,
                  &streamRefNodePtr->next);

    le_mem_Release(streamRefNodePtr);
}

le_result_t taf_Audio::RecordFile
(
    taf_audio_StreamRef_t streamRef , ///< Audio stream reference.
    const char            *srcPath    ///< The file path.
)
{
    LE_INFO("RecordFile srcPath %s", srcPath);
    taf_audio_Stream_t* streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    le_result_t res = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL(streamPtr == NULL, LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
            LE_BAD_PARAMETER, "Invalid stream reference");

    // Check if local capture is ongoing if direction is TX,
    // and if incall downlink capture is ongoing if direction is RX.
    TAF_ERROR_IF_RET_VAL((streamPtr->direction == TAF_AUDIO_TX) ? mIsRecording : mIsRxRecording,
            LE_BUSY, "Another file recording is in progress");

    if (((streamPtr->direction == TAF_AUDIO_TX) && !mIsCaptureStreamCreated)
            || ((streamPtr->direction == TAF_AUDIO_RX) && !mIsRxCaptureStreamCreated)) {
        PaStreamConfig config = {};
        config.type = PaStreamType::CAPTURE;
        config.slotId = PaSlotId::SLOT_ID_1;
        config.format = PaFileFormat::WAVE;
        config.sampleRate = DEFAULT_SAMPLERATE;
        config.channelTypeMask = PaChannelType::LEFT | PaChannelType::RIGHT;

        // Set the config device type based on output device
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_audio_Connector_t const * currentconnPtr;
        taf_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                    LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

            strmItr = (le_hashmap_It_Ref_t)
                    le_hashmap_GetIterator(currentconnPtr->audioInList);
            // Update the configuration based on the streams connected
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                        LE_BAD_PARAMETER,"outStreamPtr is nullptr!");
                if (mCallStarted && (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1
                        || outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2
                        || outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3)){
                    TAF_ERROR_IF_RET_VAL(true, LE_UNSUPPORTED,
                            "Incall uplink recording is not supported");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_1){
                    config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_1);
                    LE_DEBUG("set config with device type mic0");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_2) {
                    config.deviceTypes
                            .emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_2);
                    LE_DEBUG("set config with device type mic1");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_MIC_3) {
                    config.deviceTypes.emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SOURCE_3);
                    LE_DEBUG("set config with device type mic2");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) {
                    config.streamDir.emplace_back(PaStreamDirection::RX);
                    LE_DEBUG("set voice path to RX");
                }
            }
        }
        res = StartAudio(config);
        TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT, "Config failed");
        if(streamPtr->direction == TAF_AUDIO_TX)
            mIsCaptureStreamCreated = true; // local capture stream created.
        else
            mIsRxCaptureStreamCreated = true; // incall downlink capture stream created.
    }

    if ( srcPath != NULL )
    {
        FILE* file;
        if(streamPtr->direction == TAF_AUDIO_TX)
        {
            mFile = fopen(srcPath, "w");
            file = mFile;
        }
        else
        {
            mRxFile = fopen(srcPath, "w");
            file = mRxFile;
        }
        if(file == NULL) {
            LE_ERROR("Unable to write to file");
            return LE_FAULT;
        }
        if(file) {
            fseek(file, 0, SEEK_SET);
            if(setWavHeader(file, streamPtr) == LE_OK)
            {
                // SetVolume if local stream, as remote stream is not supported.
                if(streamPtr->direction == TAF_AUDIO_TX)
                {
                    res = SetVolume(streamRef, streamPtr->volLevel, false);
                    if (res == LE_OK)
                    {
                        LE_INFO("Successfully set the vol level to recorder stream");
                    }
                    else
                    {
                        LE_ERROR("Failed to set the vol level to recorder stream, res = %d", res);
                    }
                }
                res = startRecording(streamPtr);
                if(res != LE_OK)
                {
                    setVhalRouteStatus(TAF_AUDIO_LOCAL_RECORDING, false);
                    fclose(file);
                    file = NULL;
                    streamPtr->fd = -1;
                }
            }
            else
                return res;
        } else {
            LE_ERROR("Unable to write to file");
            return res;
        }
    }
    else
    {
        return res;
    }
    return res;
}

le_result_t taf_Audio::startRecording(taf_audio_Stream_t* streamPtr)
{
    std::shared_ptr<PaAudioCaptureStream> audioCaptureStream;
    bool* isRecording;
    uint32_t size;
    uint32_t* bufferRecordedTillNow;
    std::shared_ptr<tafpa::audio::IPaStreamBuffer> *streamBuffer;
    std::queue<std::shared_ptr<tafpa::audio::IPaStreamBuffer>> *freeBuffers;
    if(streamPtr->direction == TAF_AUDIO_TX) // Update local recording data
    {
        audioCaptureStream = std::dynamic_pointer_cast<PaAudioCaptureStream>
                (taf_pa_audio_GetCaptureStream(PaStreamDirection::TX));
        isRecording = &mIsRecording;
        bufferRecordedTillNow = &mBufferRecordedTillNow;
        streamBuffer = &mRecStreamBuffer;
        freeBuffers = &mRecFreeBuffers;
        mTxRecStreamPtr = streamPtr;
    }
    else // Update incall downlink recording data
    {
        audioCaptureStream = std::dynamic_pointer_cast<PaAudioCaptureStream>
                (taf_pa_audio_GetCaptureStream(PaStreamDirection::RX));
        isRecording = &mIsRxRecording;
        bufferRecordedTillNow = &mRxBufferRecordedTillNow;
        streamBuffer = &mRxRecStreamBuffer;
        freeBuffers = &mRxRecFreeBuffers;
        mRxRecStreamPtr = streamPtr;
    }
    *isRecording = false;
    if (!audioCaptureStream) {
        LE_ERROR("Failed to get capture stream");
        return LE_FAULT;
    }
    // Pop the previous buffers if any.
    while(!freeBuffers->empty()) {
        freeBuffers->pop();
    }
    for(int i = 0; i < TOTAL_BUFFERS; i++) {
        *streamBuffer = audioCaptureStream->getStreamBuffer();
        if(*streamBuffer != nullptr) {
            size = (*streamBuffer)->getMinSize();
            if(size == 0) {
                size =  (*streamBuffer)->getMaxSize();
            }
            (*streamBuffer)->setDataSize(size);
            (*freeBuffers).push(*streamBuffer);
        } else {
            LE_ERROR( "Failed to get Stream Buffer ");
            return LE_FAULT;
        }
    }

    *isRecording = true;
    *bufferRecordedTillNow = 0;

    LE_INFO( "Audio recording started" );

    for(int i = 0; i < TOTAL_BUFFERS; i++) {
        *streamBuffer = freeBuffers->front();
        freeBuffers->pop();
        pa_result_t status;
        if(streamPtr->direction == TAF_AUDIO_TX) {
            LE_INFO("setDataSize");
            status = audioCaptureStream->read(*streamBuffer , size,
                    &taf_Audio::ReadCallback);
        }
        else{
            status = audioCaptureStream->read(*streamBuffer, size,
                    &taf_Audio::RxReadCallback);
        }
        if(status != PA_OK) {
            LE_ERROR("read() failed with result %d",int(status));
            *isRecording = false;
            return LE_FAULT;
        }
    }

    // SetMute status if local stream, as remote stream is not supported.
    if (streamPtr->direction == TAF_AUDIO_TX && streamPtr->isMute)
    {
        le_result_t res = SetMute(streamPtr->streamRef, streamPtr->isMute);
        if (res == LE_OK)
        {
            LE_INFO("Successfully set the mute status to recorder stream");
        }
        else
        {
            LE_ERROR("Failed to set the mute status to recorder stream, res = %d", res);
        }
    }
    return LE_OK;
}

le_result_t taf_Audio::setWavHeader
(
    FILE* mFile,
    taf_audio_Stream_t *config
)
{
    WavHeader_t hdr;

    memset(&hdr, 0, sizeof(WavHeader_t));
    hdr.riffId = ID_RIFF;
    hdr.riffFmt = ID_WAVE;
    hdr.chunkId = ID_FMT;
    hdr.chunkSize = DEFAULT_BITSPERSAMPLE;
    hdr.formatTag = FORMAT_PCM;
    hdr.channelsCount = DEFAULT_NUM_CHANNELS;
    hdr.sampleRate = DEFAULT_SAMPLERATE;
    hdr.bitsPerSample = DEFAULT_BITSPERSAMPLE;
    hdr.byteRate = (  hdr.sampleRate *
                      hdr.channelsCount *
                      hdr.bitsPerSample  ) / BITS_PER_BYTE;
    hdr.blockAlign = ( hdr.bitsPerSample * hdr.channelsCount) / BITS_PER_BYTE;
    hdr.chunkDataId = ID_DATA;
    hdr.chunkDataSize = 0;
    hdr.riffSize = hdr.chunkDataSize + DEFAULT_RIFF_SIZE;
    if (fwrite(&hdr, 1, sizeof(hdr),mFile) != sizeof(hdr))
    {
        LE_ERROR("Cannot write wave header");
        return LE_FAULT;
    }
    else
    {
        LE_INFO("Wav header set with %d ch, %d Hz, %d bit, %s",
                hdr.channelsCount, hdr.sampleRate, hdr.bitsPerSample,
                hdr.formatTag == FORMAT_PCM ? "PCM" : "unknown");
        return LE_OK;
    }
}

void taf_Audio::ReadCallback(std::shared_ptr<tafpa::audio::IPaStreamBuffer> buffer,
        pa_result_t readRes)
{
    uint32_t bytesWrittenToFile = 0;
    auto &audio = taf_Audio::GetInstance();
    if (readRes != PA_OK) {
        LE_ERROR("read() returned with result %d",int(readRes));
        audio.mIsRecording = false;
        audio.mIsRecError = true;
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, size, audio.mFile);
        if(bytesWrittenToFile != size) {
            LE_ERROR("Write Size mismatch while writing to file");
        }
        audio.mBufferRecordedTillNow = audio.mBufferRecordedTillNow + size;
    }
    buffer->reset();
    audio.mRecFreeBuffers.push(buffer);
    taf_audio_BufferEvent_t bufferEvent;
    bufferEvent.bufferType = TAF_AUDIO_REC_BUFFER;
    bufferEvent.streamPtr = audio.mTxRecStreamPtr;
    le_event_Report(audio.bufferEventId, &bufferEvent, sizeof(taf_audio_BufferEvent_t));
    return;
}

void taf_Audio::RxReadCallback(std::shared_ptr<tafpa::audio::IPaStreamBuffer> buffer,
        pa_result_t readRes)
{
    uint32_t bytesWrittenToFile = 0;
    auto &audio = taf_Audio::GetInstance();
    if (readRes != PA_OK) {
        LE_ERROR("read() returned with result %d",int(readRes));
        audio.mIsRxRecording = false;
        audio.mIsRxRecError = true;
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, size, audio.mRxFile);
        if(bytesWrittenToFile != size) {
            LE_ERROR("Write Size mismatch while writing to file");
        }
        audio.mRxBufferRecordedTillNow = audio.mRxBufferRecordedTillNow + size;
    }
    buffer->reset();
    audio.mRxRecFreeBuffers.push(buffer);
    taf_audio_BufferEvent_t bufferEvent;
    bufferEvent.bufferType = TAF_AUDIO_REC_BUFFER;
    bufferEvent.streamPtr = audio.mRxRecStreamPtr;
    le_event_Report(audio.bufferEventId, &bufferEvent, sizeof(taf_audio_BufferEvent_t));
    return;
}

/**
 * Create stream for the given playback and start Audio
 */
le_result_t taf_Audio::PlayFile
(
    taf_audio_StreamRef_t  streamRef,
    const char* srcPath
)
{
    taf_audio_Stream_t* streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
            LE_BAD_PARAMETER, "Invalid stream reference");

    le_result_t res = LE_FAULT;

    // Check if local playback is ongoing if direction is RX,
    // and if incall uplink playback is ongoing if direction is TX.
    TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX ? mIsPlaying : mIsTxPlaying, LE_BUSY,
            "Another playback is in progress");

    taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
    snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), srcPath);
    playFileConfig[0].repeat = 0;
    res = PlayList( streamRef, playFileConfig,
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    if(res != LE_OK)
    {
        LE_ERROR("Failed to play the list, result is %d", res);
    }

    return res;
}

le_result_t taf_Audio::ReadPcmHeader
(
    taf_audio_Stream_t* streamPtr,
    taf_pa_audio_PlayFileInfo_t* playFileInfo
)
{
    WavHeader_t wHdr;

    if (ReadHeader(streamPtr->fd, &wHdr, sizeof(wHdr)) != sizeof(wHdr))
    {
        LE_WARN("WAV detection: cannot read header");
        return LE_FAULT;
    }

    if ((wHdr.riffId != ID_RIFF)  ||
            (wHdr.riffFmt != ID_WAVE) ||
            (wHdr.chunkId != ID_FMT)    ||
            (wHdr.formatTag != FORMAT_PCM) ||
            (wHdr.chunkSize != 16))
    {
        LE_WARN("WAV detection: unrecognized wav format");
        lseek(streamPtr->fd, -sizeof(wHdr), SEEK_CUR);
        return LE_FAULT;
    }

    playFileInfo->streamConfig.sampleRate = wHdr.sampleRate;
    playFileInfo->streamConfig.channelTypeMask = (wHdr.channelsCount == DEFAULT_NUM_CHANNELS)
            ? (PaChannelType::LEFT | PaChannelType::RIGHT) : PaChannelType::LEFT;
    playFileInfo->streamConfig.format = PaFileFormat::WAVE;


    LE_DEBUG("sample rate is %d, channelsCount %d", playFileInfo->streamConfig.sampleRate,
            wHdr.channelsCount);
    // Set the config device type based on output device connected or voice path
    // direction in case of in-call uplink playback.
    le_hashmap_It_Ref_t connItr =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_audio_Connector_t const * currentconnPtr;
    taf_audio_Stream_t const * outStreamPtr;
    le_hashmap_It_Ref_t strmItr;
    while (le_hashmap_NextNode(connItr)==LE_OK)
    {
        currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
        TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

        strmItr = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(currentconnPtr->audioOutList);
        // Update the configuration based on the streams connected
        while (le_hashmap_NextNode(strmItr)==LE_OK) {
            outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
            TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                    LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

            if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1){
                playFileInfo->streamConfig.deviceTypes.
                        emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_1);
                LE_DEBUG("set config with device type sink 1");
            }
            else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2) {
                playFileInfo->streamConfig.deviceTypes.
                        emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_2);
                LE_DEBUG("set config with device type sink 2");
            }
            else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3) {
                playFileInfo->streamConfig.deviceTypes.
                        emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_3);
                LE_DEBUG("set config with device type sink 3");
            }
            else if (outStreamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) {
                playFileInfo->streamConfig.streamDir.emplace_back(PaStreamDirection::TX);
                LE_DEBUG("set config with stream direction TX");
            }
        }
    }

    return LE_OK;
}

le_result_t taf_Audio::ReadAmrHeader
(
    taf_audio_Stream_t* streamPtr,
    taf_pa_audio_PlayFileInfo_t* playFileInfo
)
{
    taf_audio_FileFormat_t format = TAF_AUDIO_FILE_MAX;

    char header[10] = {0};
    lseek(streamPtr->fd, 0, SEEK_SET);
    if ( read(streamPtr->fd, header, 9) != 9 )
    {
        LE_WARN("AMR detection: cannot read header");
        return LE_FAULT;
    }
    if ( strncmp(header, "#!AMR", 5) == 0 )
    {
        format = TAF_AUDIO_FILE_MAX;

        if ( strncmp(header+5, "-WB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-WB Detected");
            format = TAF_AUDIO_FILE_AMR_WB;

        }
        else if ( strncmp(header+5, "-NB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_AUDIO_FILE_AMR_NB;
        }
        else if ( strncmp(header+5, "\n", 1) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_AUDIO_FILE_AMR_NB;

            lseek(streamPtr->fd, -3, SEEK_CUR);
        }
        else
        {
            LE_ERROR("Not an AMR file");
            return LE_FAULT;
        }
        if (format == TAF_AUDIO_FILE_AMR_WB)
        {
            playFileInfo->streamConfig.format = PaFileFormat::AMR_WB;
            playFileInfo->streamConfig.sampleRate = 16000;
        }
        else
        {
            playFileInfo->streamConfig.format = PaFileFormat::AMR_NB;
            playFileInfo->streamConfig.sampleRate = 8000;
        }

        playFileInfo->streamConfig.channelTypeMask = PaChannelType::LEFT;

        // Set the config device type based on output device connected or voice path
        // direction in case of in-call uplink playback.
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_audio_Connector_t const * currentconnPtr;
        taf_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                    LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

            strmItr = (le_hashmap_It_Ref_t)
                    le_hashmap_GetIterator(currentconnPtr->audioOutList);
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                        LE_BAD_PARAMETER,"outStreamPtr is nullptr!");
                if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_1){
                    playFileInfo->streamConfig.deviceTypes.
                            emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_1);
                    LE_DEBUG("set config with device type sink 1");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_2) {
                    playFileInfo->streamConfig.deviceTypes.
                            emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_2);
                    LE_DEBUG("set config with device type sink 2");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER_3) {
                    playFileInfo->streamConfig.deviceTypes.
                            emplace_back(PaAudioIf::TAF_PA_AUDIO_IF_CODEC_SINK_3);
                    LE_DEBUG("set config with device type sink 3");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) {
                    playFileInfo->streamConfig.streamDir.
                            emplace_back(PaStreamDirection::TX);
                    LE_DEBUG("set config with stream direction TX");
                }
            }
        }
    }
    else{
        LE_ERROR("Invalid header for AMR");
        return LE_FAULT;
    }
    return LE_OK;
}

/**
 * Read header info and detect file format
 */
ssize_t taf_Audio::ReadHeader
(
    int fd, void* bufPtr, size_t bufSize
)
{
    TAF_ERROR_IF_RET_VAL((bufPtr == NULL) || (fd < 0) , -1,
            "Supplied NULL string pointer / invalid file descriptor");

    int bytesRead = 0, tempBufSize = 0, readReq = bufSize;
    char *Str;

    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        Str = (char *)(bufPtr);
        Str = Str + tempBufSize;

        bytesRead = read(fd, Str, readReq);

        if ((bytesRead < 0) && (errno != EINTR) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            LE_ERROR("Error while reading file, errno: %d (%m)", errno);
            return bytesRead;
        }
        else
        {
            if (bytesRead == 0)
            {
                return tempBufSize;
            }

            tempBufSize += bytesRead;

            if (tempBufSize < (int)bufSize)
            {
                readReq = bufSize - tempBufSize;
            }
        }
    }
    while (tempBufSize < (int)bufSize);

    return tempBufSize;
}

le_result_t taf_Audio::Stop(taf_audio_StreamRef_t streamRef)
{
    taf_audio_Stream_t* streamPtr =
            (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);

    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid stream reference");

    le_result_t res = StopAudio(streamPtr);
    TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT,"Stop failed");
    return LE_OK;
}

le_result_t taf_Audio::StopAudio(taf_audio_Stream_t* streamPtr)
{
    LE_DEBUG("Stop audio stream %d\n", streamPtr->interface);
    CALLBACK_TO_SET_PA_RESULT;
    pa_result_t status = PA_FAULT;
    PaStreamConfig config;
    pa_result_t cbRes = PA_FAULT;
    std::any context = std::make_shared<PaStreamConfig>(config);
    if(streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX
            || streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        if (mVoiceEnabled1) {
            config.type = PaStreamType::VOICE_CALL;
            status = taf_pa_audio_StopAudio(config, cb, context);
        }

        if (status == PA_OK) {
            LE_DEBUG("Stop voice call audio successful");
            cbRes = prom->get_future().get();
            if (PA_OK != cbRes) {
                LE_ERROR("Request to Stop stream failed error: %d", int (cbRes));
                return LE_FAULT;
            }
            else
            {
                auto &audio = taf_Audio::GetInstance();
                audio.mVoiceEnabled1 = false;
                LE_INFO("audio stopped successfully");
            }
        }
    }
    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) {
        if ((streamPtr->direction == TAF_AUDIO_TX && mIsRecording)
                    || (streamPtr->direction == TAF_AUDIO_RX && mIsRxRecording)) {
            LE_DEBUG("Stop Recording");
            if(streamPtr->direction == TAF_AUDIO_TX)
                mIsRecording = false;
            else
                mIsRxRecording = false;
            cbRes = gCallbackPromise.get_future().get();
            if (PA_OK != cbRes) {
                LE_ERROR("Request to Stop stream failed error: %d", int (cbRes));
                return LE_FAULT;
            }
        }
    }
    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) {
        LE_DEBUG("mIsPlaying is %s", mIsPlaying ? "true" : "false");
        if (mIsPlaying || mIsTxPlaying) {
            LE_INFO("Stop Playing");
            pa_result_t res = PA_FAULT;
            PaStreamConfig streamConfig = {};
            streamConfig.type = PaStreamType::PLAY;
            if(streamPtr->direction == TAF_AUDIO_TX)
                streamConfig.streamDir.emplace_back(PaStreamDirection::TX);

            res = taf_pa_audio_StopPlayback(streamConfig);
            if(res != PA_OK)
                return LE_FAULT;

            if(streamPtr->direction == TAF_AUDIO_RX)
                mIsPlaying = false;
            else
                mIsTxPlaying = false;
            setVhalRouteStatus(TAF_AUDIO_LOCAL_PLAYBACK, false);
        }
        else
        {
            LE_ERROR("Playback is not in progress");
            return LE_FAULT;
        }
    }
    paVoiceStreamConfig = {};
    if (status == PA_OK) {
        LE_DEBUG("Stop successful");
    }
    return LE_OK;
}

le_result_t taf_Audio::DeleteAudioStream(taf_audio_Stream_t* streamPtr)
{
    LE_DEBUG("Delete stream %d\n", streamPtr->interface);
    pa_result_t status = PA_FAULT;
    pa_result_t cbRes = PA_FAULT;
    CALLBACK_TO_SET_PA_RESULT;
    PaStreamConfig config = {};
    std::any context = std::make_shared<PaStreamConfig>(config);
    if(mCallStarted && (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX
            || streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
    {
        config.type = PaStreamType::VOICE_CALL;
        status = taf_pa_audio_DeleteStream(config, cb, context);
        LE_INFO("delete stream res %d", status);
        if (status == PA_OK) {
            LE_INFO("res success waiting for callback");
            cbRes = prom->get_future().get();
            if (PA_OK != cbRes) {
                LE_ERROR("Request to delete voice stream failed error: %d", int (cbRes));
                return LE_FAULT;
            }
            else
            {
                auto &audio = taf_Audio::GetInstance();
                LE_DEBUG("deleteStream() succeeded.");
                audio.mCallStarted = false;
            }
        } else {
            LE_ERROR("Error in disabling voice audio ");
            return LE_FAULT;
        }
    }

    if (streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) {
        std::promise<bool> p;
        if(streamPtr->direction == TAF_AUDIO_TX)
        {
            config.type = PaStreamType::CAPTURE;
            status = taf_pa_audio_DeleteStream(config, cb, context);
            if (status == PA_OK)
            {
                cbRes = prom->get_future().get();
                if(cbRes == PA_OK)
                {
                    LE_DEBUG("Successfully deleted the capture stream");
                    mIsCaptureStreamCreated = false;
                }
                else
                {
                    LE_ERROR("Failed to delete capture stream, err : %d", cbRes);
                }
            }
            else {
                LE_ERROR("Error in delete capture stream");
                return LE_FAULT;
            }
        }
        else if (streamPtr->direction == TAF_AUDIO_RX)
        {
            config.type = PaStreamType::CAPTURE;
            config.streamDir.emplace_back(PaStreamDirection::RX);
            status = taf_pa_audio_DeleteStream(config, cb, context);
            if (status == PA_OK)
            {
                cbRes = prom->get_future().get();
                if(cbRes == PA_OK)
                {
                    LE_DEBUG("Successfully deleted the remote capture stream");
                    mIsRxCaptureStreamCreated = false;
                }
                else
                {
                    LE_ERROR("Failed to delete remote capture stream, err : %d", cbRes);
                }
            }
            else {
                LE_ERROR("Error in delete capture stream");
                return LE_FAULT;
            }
        }
        try
        {
            gCallbackPromise.set_value(cbRes);
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
        }
        resetCallbackPromise();
    }
    if(status == PA_OK)
        LE_INFO("status is success");
    return LE_OK;
}

le_result_t taf_Audio::setVhalRouteStatus(taf_audio_Mode_t mode, bool status)
{
    le_result_t res = LE_FAULT;
    auto &audioVhal = taf_AudioVhal::GetInstance();
    if(audioVhal.isAudioDrvAvailable())
    {
        le_ref_IterRef_t iteratorRef;
        iteratorRef = le_ref_GetIterator(RouteRefMap);
        while (le_ref_NextNode(iteratorRef) == LE_OK)
        {
            taf_audio_Route_t* routePtr =
                    (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
            if ( routePtr && routePtr->mode == mode )
            {
                LE_INFO("set VHAL status mode : %d status : %s", mode, status ? "true" : "false");
                res = audioVhal.OpenRoute(status, routePtr->routeId, routePtr->mode);
                break;
            }
        }
    }
    return res;
}

void tafPromptsStatusListener::onPlaybackStarted() {
    if(streamPtr->direction == TAF_AUDIO_RX)
        LE_INFO("Local playback started");
    else if(streamPtr->direction == TAF_AUDIO_TX)
        LE_INFO("Remote playback started");
}

void tafPromptsStatusListener::onPlaybackStopped() {
    LE_INFO("onPlaybackStopped");
    auto &audio = taf_Audio::GetInstance();
    if(streamPtr->direction == TAF_AUDIO_RX)
        audio.mIsPlaying = false;
    else if(streamPtr->direction == TAF_AUDIO_TX)
        audio.mIsTxPlaying = false;

    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_STOPPED;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));
}

void tafPromptsStatusListener::onError(int error, std::string file) {
    LE_ERROR("onError : Error %d encounter while playing the file %s", (int)error, file.c_str());
    auto &audio = taf_Audio::GetInstance();
    if(streamPtr->direction == TAF_AUDIO_RX)
        audio.mIsPlaying = false;
    else if(streamPtr->direction == TAF_AUDIO_TX)
        audio.mIsTxPlaying = false;

    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_ERROR;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));

    audio.setVhalRouteStatus(TAF_AUDIO_LOCAL_PLAYBACK, false);
}

void tafPromptsStatusListener::onFilePlayed(std::string file) {
    LE_DEBUG("onFilePlayed : %s", file.c_str());
}

void tafPromptsStatusListener::onPlaybackFinished() {
    LE_DEBUG("onPlaybackFinished");
    auto &audio = taf_Audio::GetInstance();

    if(streamPtr->direction == TAF_AUDIO_RX)
        audio.mIsPlaying = false;
    else if(streamPtr->direction == TAF_AUDIO_TX)
        audio.mIsTxPlaying = false;

    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_ENDED;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));

    audio.setVhalRouteStatus(TAF_AUDIO_LOCAL_PLAYBACK, false);

}

/**
 * Register for the buffer event
 */
void* taf_Audio::RegisterBufferEvent( void* ctxPtr) {

    LE_DEBUG("Start registering for buffer event handler");
    auto &audio = taf_Audio::GetInstance();
    le_sem_Ref_t semRef = (le_sem_Ref_t)ctxPtr;
    audio.bufferEventId = le_event_CreateId("BufferAvailableEvent",
            sizeof(taf_audio_BufferEvent_t));
    audio.bufferHandlerRef = le_event_AddHandler("bufferEvent", audio.bufferEventId,
            BufferEventHandler);
    le_sem_Post(semRef);
    le_event_RunLoop();
    return nullptr;
}

void taf_Audio::BufferEventHandler(void* ctxPtr)
{
    taf_audio_BufferEvent_t* bufferEventPtr = (taf_audio_BufferEvent_t*)ctxPtr;
    auto &audio = taf_Audio::GetInstance();
    LE_DEBUG("BufferEventHandler");
    if(bufferEventPtr->bufferType == TAF_AUDIO_REC_BUFFER)
    {
        audio.RecBufferHandler(bufferEventPtr->streamPtr);
    }
}

void taf_Audio::RemoveBufferHandler(void* param1, void* param2)
{
    auto &audio = taf_Audio::GetInstance();
    if (audio.bufferHandlerRef)
    {
        LE_INFO("remove bufferHandlerRef");
        le_event_RemoveHandler(audio.bufferHandlerRef);
        audio.bufferHandlerRef = nullptr;
    }
    le_thread_Exit(nullptr);
}

void taf_Audio::SubsystemStateChangeHandler(void* ctxPtr)
{
    tafpa::audio::SubsystemState_e* statusPtr = (tafpa::audio::SubsystemState_e*)ctxPtr;
    auto &audio = taf_Audio::GetInstance();

    LE_DEBUG("Audio subsystem state changed, status: %d", (int)*statusPtr);

    if (*statusPtr == tafpa::audio::SubsystemState_e::UNAVAILABLE)
    {
        // Clean up all audio resources
        audio.CleanupAudioResources();

        // Exit process with EXIT_UNAVAILABLE as audio subsystem is unavailable
        LE_ERROR("Audio subsystem unavailable, exiting process");
        exit(EXIT_UNAVAILABLE);
    }
}

void taf_Audio::RecBufferHandler(taf_audio_Stream_t* streamPtr)
{
    std::shared_ptr<tafpa::audio::PaAudioCaptureStream> audioCaptureStream;
    bool* isRecording;
    bool* isRecError;
    uint32_t size = 0;
    FILE* file;
    uint32_t* bufferRecordedTillNow;
    std::shared_ptr<tafpa::audio::IPaStreamBuffer> *streamBuffer;
    std::queue<std::shared_ptr<tafpa::audio::IPaStreamBuffer>> *freeBuffers;
    if(streamPtr->direction == TAF_AUDIO_TX) // Update local recording data
    {
        if (!mFile)
            return;
        audioCaptureStream = std::dynamic_pointer_cast<PaAudioCaptureStream>
                (taf_pa_audio_GetCaptureStream(PaStreamDirection::TX));
        isRecording = &mIsRecording;
        file = mFile;
        bufferRecordedTillNow = &mBufferRecordedTillNow;
        streamBuffer = &mRecStreamBuffer;
        freeBuffers = &mRecFreeBuffers;
        isRecError = &mIsRecError;
    }
    else // Update incall downlink recording data
    {
        if(!mRxFile)
            return;
        audioCaptureStream = std::dynamic_pointer_cast<PaAudioCaptureStream>
                (taf_pa_audio_GetCaptureStream(PaStreamDirection::RX));
        isRecording = &mIsRxRecording;
        file = mRxFile;
        bufferRecordedTillNow = &mRxBufferRecordedTillNow;
        streamBuffer = &mRxRecStreamBuffer;
        freeBuffers = &mRxRecFreeBuffers;
        isRecError = &mIsRecError;
    }
    // Continue recording next buffers
    if (*isRecording)
    {
        size = (*streamBuffer)->getMinSize();
        if(size == 0) {
            size =  (*streamBuffer)->getMaxSize();
        }
        (*streamBuffer)->setDataSize(size);

        // Stop recording when recorded buffer reaches maxFileBytes defined.
        if((*bufferRecordedTillNow + (uint32_t)sizeof(WavHeader_t) + (2 * size))
                    >= maxFileBytes) {
            LE_INFO("Stoping recording as file size reached maxFileBytes");
            *isRecording = false;
            taf_audio_BufferEvent_t bufferEvent;
            bufferEvent.bufferType = TAF_AUDIO_REC_BUFFER;
            bufferEvent.streamPtr = streamPtr;
            le_event_Report(bufferEventId, &bufferEvent, sizeof(taf_audio_BufferEvent_t));
            return;
        }

        if(!freeBuffers->empty()) {
            *streamBuffer = freeBuffers->front();
            freeBuffers->pop();
            pa_result_t status;
            if(streamPtr->direction == TAF_AUDIO_TX) {
            status = (audioCaptureStream)->read(*streamBuffer , size,
                    &taf_Audio::ReadCallback);
            }
            else{
                status = (audioCaptureStream)->read(*streamBuffer, size,
                        &taf_Audio::RxReadCallback);
            }
            if(status != PA_OK) {
                LE_ERROR("read() failed with result %d",int(status));
                *isRecording = false;
                *isRecError = true;
                taf_audio_BufferEvent_t bufferEvent;
                bufferEvent.bufferType = TAF_AUDIO_REC_BUFFER;
                bufferEvent.streamPtr = streamPtr;
                le_event_Report(bufferEventId, &bufferEvent, sizeof(taf_audio_BufferEvent_t));
            }
        }
        return;
    }
    int waitTime = (8*((*streamBuffer)->getMaxSize())* SECS_IN_MILLISES)/
                        (DEFAULT_SAMPLERATE*2*DEFAULT_BITSPERSAMPLE);
    waitTime = waitTime + MAX_RESPONSE_DELAY;
    while(freeBuffers->size() != TOTAL_BUFFERS) {
        usleep(waitTime * MILLISECS_IN_MICROSECS);
    }

    // Update recorded buffer size to the header after completing the recording.
    WavHeader_t hdr;
    fseek(file, ((uint8_t*)&hdr.chunkDataSize - (uint8_t*)&hdr), SEEK_SET);

    if (fwrite(bufferRecordedTillNow, 1, sizeof(*bufferRecordedTillNow),
            file) != sizeof(*bufferRecordedTillNow))
    {
        LE_ERROR("Cannot write size to wave header");
    }
    else{
        LE_INFO("Updated the header with chunkdatasize %d",*bufferRecordedTillNow);
    }
    fseek(file, ((uint8_t*)&hdr.riffSize - (uint8_t*)&hdr), SEEK_SET);
    uint32_t riffSize = *bufferRecordedTillNow + DEFAULT_RIFF_SIZE;

    if (fwrite(&riffSize, 1, sizeof(riffSize), file) != sizeof(riffSize))
    {
        LE_ERROR("Cannot write riff size to wave header");
    }
    else{
        LE_INFO("Updated the header with riffsize %d", riffSize);
    }
    fflush(file);
    fclose(file);
    if(streamPtr->direction == TAF_AUDIO_TX)
    {
        mFile = nullptr;
        mTxRecStreamPtr = nullptr;
    }
    else
    {
        mRxFile = nullptr;
        mRxRecStreamPtr = nullptr;
    }
    streamPtr->fd = -1;
    if (*bufferRecordedTillNow != 0)
    {
        LE_INFO("File Recorded SuccessFully");
    }
    else
    {
        LE_ERROR("File recording failed");
    }

    // send route status to VHAL on error/stop.
    setVhalRouteStatus(TAF_AUDIO_LOCAL_RECORDING, false);

    DeleteAudioStream(streamPtr);
    //Notify event to the client
    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_MEDIA_EVENT;
    if (*bufferRecordedTillNow == 0 || *isRecError)
    {
        streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_ERROR;
    }
    else
    {
        streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_STOPPED;
    }
    le_event_Report(streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));
    *isRecError = false;
}

le_result_t taf_Audio::PlayList
(
    taf_audio_StreamRef_t streamRef,
    const taf_audio_PlayFileConfig_t*  playFileConfigPtr,
    size_t playFileConfigSize
)
{
    LE_INFO("PlayList with size %zu", playFileConfigSize);
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
            LE_BAD_PARAMETER, "Invalid stream reference");

    // Check if local playback is ongoing if direction is RX,
    // and if incall uplink playback is ongoing if direction is TX.
    TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX ? mIsPlaying : mIsTxPlaying, LE_BUSY,
            "Another playback is in progress");

    le_result_t res = LE_FAULT;
    pbList.pbRes = LE_FAULT;
    for(size_t i = 0; i < playFileConfigSize; i++)
    {
        taf_pa_audio_PlayFileInfo_t playFileInfo = {};
        playFileInfo.absoluteFilePath = playFileConfigPtr[i].srcPath;
        LE_DEBUG("File path : %s", playFileConfigPtr[i].srcPath);
        int AudioFileFd;
        if((AudioFileFd=open(playFileInfo.absoluteFilePath.c_str(), O_RDONLY)) == -1)
        {
            LE_ERROR("File might not exist or failed to open the file %s",
                    playFileConfigPtr[i].srcPath);
            // Clean up any previously added files
            pbList.playFileInfos.clear();
            return LE_FAULT;
        } else
        {
            LE_INFO("Successfully opened file %s", playFileInfo.absoluteFilePath.c_str());
            streamPtr->fd = AudioFileFd;
        }
        res = ReadPcmHeader(streamPtr, &playFileInfo);
        if(res == LE_UNSUPPORTED)
        {
            close(streamPtr->fd);
            streamPtr->fd = -1;
            pbList.playFileInfos.clear();
            return res;
        }
        if (res != LE_OK)
        {
            res = ReadAmrHeader(streamPtr,&playFileInfo);

            if (res != LE_OK) {
                LE_ERROR("Unknown audio format");
                close(streamPtr->fd);
                streamPtr->fd = -1;
                pbList.playFileInfos.clear();
                return LE_FAULT;
            }
        }
        playFileInfo.repeat = playFileConfigPtr[i].repeat;
        close(streamPtr->fd);
        streamPtr->fd = -1;
        pbList.playFileInfos.emplace_back(playFileInfo);
    }
    pbList.numOfFilesToPlay = pbList.playFileInfos.size();;
    //check if need to add whole vector to pblist
    pbList.sessionRef = taf_audio_GetClientSessionRef();
    pa_result_t pbRes = PA_FAULT;
    if(res == LE_OK && streamPtr->direction == TAF_AUDIO_TX)
    {
        // incall uplink playback
        mIsTxPlaying = true;
        LE_DEBUG("Create stream for incall uplink playback and start playback");
        repeatedTxPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
        repeatedTxPlayerStatusListener->streamPtr = streamPtr;
        pbRes = taf_pa_audio_StartPlayback(pbList.playFileInfos, playFileConfigSize,
                repeatedTxPlayerStatusListener);
    } else if (res == LE_OK && streamPtr->direction == TAF_AUDIO_RX)
    {
        mIsPlaying = true;
        LE_DEBUG("Create stream for local playback and start playback");
        repeatedPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
        repeatedPlayerStatusListener->streamPtr = streamPtr;
        pbRes = taf_pa_audio_StartPlayback(pbList.playFileInfos, playFileConfigSize,
                repeatedPlayerStatusListener);
    }
    pbList.playFileInfos.clear();

    if (pbRes != PA_OK)
    {
        LE_ERROR("start playback failed with error, %d", pbRes);
        return LE_FAULT;
    }
    // Set volume and mute status to local stream
    if(streamPtr->direction == TAF_AUDIO_RX)
    {
        le_result_t resVol = SetVolume(streamRef, streamPtr->volLevel, false);
        if (resVol == LE_OK)
        {
            LE_INFO("Successfully set the vol level to player stream");
        }
        else
        {
            LE_ERROR("Failed to set the vol level to player stream");
        }
        if(streamPtr->isMute)
        {
            resVol = SetMute(streamRef, streamPtr->isMute);
            if (resVol == LE_OK)
            {
                LE_INFO("Successfully set the mute status to player stream");
            }
            else
            {
                LE_ERROR("Failed to set mute status to player stream");
            }
        }
    }
    if(res == LE_OK)
        setVhalRouteStatus(TAF_AUDIO_LOCAL_PLAYBACK, true);
    return res;
}

le_result_t taf_Audio::SetMute
(
    taf_audio_StreamRef_t streamRef,
    bool isMute
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    PaStreamConfig streamConfig = {};
    std::any context = std::make_shared<PaStreamConfig>(streamConfig);
    CALLBACK_TO_SET_PA_RESULT;

    le_result_t res = LE_FAULT;
    pa_result_t status = PA_OK;

    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_DEBUG("Set the mute status to player stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_TX, LE_UNSUPPORTED,
                "Mute API is not supported on remote stream");

        if(!mIsPlaying) {
            LE_DEBUG("Stream is not active, update the mute status to stream reference");
            streamPtr->isMute = isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::PLAY;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_DEBUG("Set the mute status to recorder stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX, LE_UNSUPPORTED,
                "Mute API is not supported on remote stream");

        if(!mIsRecording) {
            LE_DEBUG("Stream is not active, update the mute status to stream reference");
            streamPtr->isMute = isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::CAPTURE;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        LE_DEBUG("Set the mute status to voice RX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active, update the mute status to stream reference");
            streamPtr->isMute = isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::RX);
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        LE_DEBUG("Set the mute status to voice TX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active, update the mute status to stream reference");
            streamPtr->isMute = isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::TX);
    } else
    {
        LE_ERROR("Invalid stream reference");
        return LE_BAD_PARAMETER;
    }
    status = taf_pa_audio_SetMute(streamConfig, isMute, cb, context);
    if (status == PA_OK) {
        pa_result_t paRes = prom->get_future().get();
        if (PA_OK != paRes) {
            if (isMute) {
                LE_ERROR("Request to Mute failed error: %d", int (paRes));
            } else {
                LE_ERROR("Request to UnMute failed");
            }
            return LE_FAULT;
        }
        else
        {
            LE_DEBUG("Successfully set the mute status");
        }
        streamPtr->isMute = isMute;
        res = LE_OK;
    } else {
        if (isMute) {
            LE_ERROR("Request to Mute failed. Error: %d", int(status));
        } else {
            LE_ERROR("Request to UnMute failed");
        }
        return LE_FAULT;
    }

    return res;
}

le_result_t taf_Audio::GetMute
(
    taf_audio_StreamRef_t streamRef,
    bool *isMute
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    pa_result_t status = PA_OK;
    PaStreamConfig streamConfig;
    std::any context = std::make_shared<PaStreamConfig>(streamConfig);    
    CALLBACK_TO_SET_PA_RESULT;
    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_DEBUG("Get mute status of player stream reference");

        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_TX, LE_UNSUPPORTED,
                "Mute API is not supported on remote stream");
        if(!mIsPlaying) {
            LE_DEBUG("Stream is not active, update the mute status of stream reference");
            *isMute = streamPtr->isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::PLAY;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_DEBUG("Get mute status of recorder stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX, LE_UNSUPPORTED,
                "Mute API is not supported on remote stream");
        if(!mIsRecording) {
            LE_DEBUG("Stream is not active, update the mute status of stream reference");
            *isMute = streamPtr->isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::CAPTURE;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        LE_DEBUG("Get mute status of voice RX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active, update the mute status of stream reference");
            *isMute = streamPtr->isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::RX);
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        LE_DEBUG("Get mute status of voice TX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active, update the mute status of stream reference");
            *isMute = streamPtr->isMute;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::TX);
    } else
    {
        LE_ERROR("Invalid stream reference");
        return LE_BAD_PARAMETER;
    }
    status = taf_pa_audio_GetMute(streamConfig, isMute, cb, context);
    if(status == PA_OK) {
        if(prom->get_future().get() == PA_OK)
        {
        LE_INFO("Successfully got the mute status %s", *isMute ? "true" : "false");
        return LE_OK;
        }
    }
    return LE_FAULT;
}

le_result_t taf_Audio::SetVolume
(
    taf_audio_StreamRef_t streamRef,
    double volLevel,
    bool isClient
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");
    TAF_ERROR_IF_RET_VAL( volLevel < 0 || volLevel > 1, LE_BAD_PARAMETER, "Invalid volume level");

    // To change the mute status to false on voulme change request from client
    if(isClient && streamPtr->isMute)
    {
        LE_INFO("Unmute the stream on volume change request");
        SetMute(streamRef, false);
    }

    pa_result_t status = PA_FAULT;
    PaStreamConfig streamConfig = {};
    std::any context = std::make_shared<PaStreamConfig>(streamConfig);
    CALLBACK_TO_SET_PA_RESULT;

    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_DEBUG("Set volume to player stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_TX, LE_UNSUPPORTED,
                "Volume API is not supported on remote stream");
        if(!mIsPlaying) {
            LE_DEBUG("Stream is not active, update the volume level to stream reference");
            streamPtr->volLevel = volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::PLAY;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_DEBUG("Set volume to recorder stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX, LE_UNSUPPORTED,
                "Volume API is not supported on remote stream");
        if(!mIsCaptureStreamCreated) {
            LE_DEBUG("Stream is not active, update the volume level to stream reference");
            streamPtr->volLevel = volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::CAPTURE;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        LE_DEBUG("Set volume to modem RX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active, update the volume level to stream reference");
            streamPtr->volLevel = volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::RX);
    } else
    {
        LE_ERROR("Invalid stream reference");
        return LE_BAD_PARAMETER;
    }
    status = taf_pa_audio_SetVolume(streamConfig, volLevel, cb, context);
    if(status == PA_OK) {
        LE_INFO("Request to set volume sent");
    } else {
        LE_ERROR("Request to set volume failed");
        return LE_FAULT;
    }
    if (prom->get_future().get() == PA_OK) {
        LE_INFO("setVolume successful.");
        streamPtr->volLevel = volLevel;
    }
    else{
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_Audio::GetVolume
(
    taf_audio_StreamRef_t streamRef,
    double *volLevel
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    pa_result_t status = PA_OK;
    PaStreamConfig streamConfig = {};
    std::any context = std::make_shared<PaStreamConfig>(streamConfig);
    CALLBACK_TO_SET_PA_RESULT;

    if(streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_DEBUG("Get volume to player stream reference");

        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_TX, LE_UNSUPPORTED,
                "Volume API is not supported on remote stream");
        if(!mIsPlaying) {
            LE_DEBUG("Stream is not active,  share local stream reference volume");
            *volLevel = streamPtr->volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::PLAY;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_DEBUG("Get volume to recorder stream reference");
        TAF_ERROR_IF_RET_VAL(streamPtr->direction == TAF_AUDIO_RX, LE_UNSUPPORTED,
                "Volume API is not supported on remote stream");
        if(!mIsRecording) {
            LE_DEBUG("Stream is not active,  share local stream reference volume");
            *volLevel = streamPtr->volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::CAPTURE;
    } else if (streamPtr->interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        LE_DEBUG("Get volume to modem RX stream reference");
        if(!mVoiceEnabled1) {
            LE_DEBUG("Stream is not active,  share local stream reference volume");
            *volLevel = streamPtr->volLevel;
            return LE_OK;
        }
        streamConfig.type = PaStreamType::VOICE_CALL;
        streamConfig.streamDir.emplace_back(PaStreamDirection::RX);
    } else
    {
        LE_ERROR("Invalid stream reference");
        return LE_BAD_PARAMETER;
    }
    status = taf_pa_audio_GetVolume(streamConfig, volLevel, cb, context);
    if(status == PA_OK) {
        LE_INFO("Request to get volume sent");
    } else {
        LE_ERROR("Request to get volume failed");
        return LE_FAULT;
    }
    if (prom->get_future().get() != PA_OK) {
        LE_ERROR("Failed to get volume");
        return LE_FAULT;
    }
    return LE_OK;
}

std::pair<int, int> taf_Audio::getDTMFFrequencies(char key) {
    // Find the frequency pair for the given key
    auto itr = dtmfMap.find(key);
    if (itr != dtmfMap.end()) {
        return itr->second;
    } else {
        // Return a pair of -1 if the key is not found
        return {-1, -1};
    }
}

void* taf_Audio::playAllDtmfTones(void* dtmfTones) {
    auto &audio = taf_Audio::GetInstance();
    taf_Dtmf_t* dtmfData = (taf_Dtmf_t*)dtmfTones;
    bool playingFirstDtmf = true;
    for(auto frequencies : dtmfData->frequencyList) {
        if(audio.mVoiceEnabled1 && (audio.mDtmfStarted || playingFirstDtmf)) {
            PaDtmfTone paDtmfTone = {};
            paDtmfTone.direction = PaStreamDirection::RX;
            paDtmfTone.lowFreq = frequencies.first;
            paDtmfTone.highFreq = frequencies.second;
            std::any context = std::make_shared<PaDtmfTone>(paDtmfTone); 
            CALLBACK_TO_SET_PA_RESULT;
            pa_result_t result = PA_FAULT;
            LE_DEBUG("Playing frequencies low: %d, high: %d\n", frequencies.first,
            frequencies.second);
            uint16_t gain_new = dtmfData->dtmfGain*MAX_DTMF_GAIN;
            if(audio.mVoiceEnabled1) {
                result = taf_pa_audio_PlayDtmf(
                            paDtmfTone, dtmfData->durationRx, gain_new, cb, context);
            } else {
                LE_ERROR("No voice stream found");
                if(playingFirstDtmf) {
                    le_sem_Post(audio.mDtmfStartedSemRef);
                    playingFirstDtmf = false;
                }
                *dtmfData = {};
                return NULL;
            }

            if(result == PA_OK) {
                pa_result_t res =  prom->get_future().get();
                if (res != PA_OK) {
                    LE_ERROR("Play Dtmf Tone failed");
                    if(playingFirstDtmf) {
                        le_sem_Post(audio.mDtmfStartedSemRef);
                        playingFirstDtmf = false;
                    }
                    break;
                }
                audio.mDtmfStarted = true;
                if(playingFirstDtmf) {
                    le_sem_Post(audio.mDtmfStartedSemRef);
                    playingFirstDtmf = false;
                }
            } else {
                LE_ERROR("Request to play Dtmf Tone failed,err %d", (int)result);
                if(playingFirstDtmf) {
                    le_sem_Post(audio.mDtmfStartedSemRef);
                    playingFirstDtmf = false;
                }
                break;
            }

            if(dtmfData->durationRx == INFINITE_TONE_DURATION) {
                /* Since the duration value is set to INIFINITY, play the DTMF tone till user
                   stops it.*/
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(
                    dtmfData->pause+dtmfData->durationRx));
        } else {
            //If the voice call ends in between, exit the thread.
            if(playingFirstDtmf) {
                le_sem_Post(audio.mDtmfStartedSemRef);
                playingFirstDtmf = false;
            }
            break;
        }
    }
    audio.mDtmfStarted = false;
    *dtmfData = {};
    audio.dtmfThreadRef = nullptr;
    return NULL;
}

void* taf_Audio::playDTMFonTX(void* dtmfTones) {

    auto &audio = taf_Audio::GetInstance();
    taf_Dtmf_t* dtmfData = (taf_Dtmf_t*)dtmfTones;
    bool playingFirstDtmf = true;
    const char* dtmfCharsPtr = dtmfData->dtmfChars.c_str();

    while(*dtmfCharsPtr != '\0') {
        if((audio.mDtmfStartedTx || playingFirstDtmf)) {
            std::any context = std::make_shared<char>(*dtmfCharsPtr);
            CALLBACK_TO_SET_PA_RESULT;
            pa_result_t res = taf_pa_audio_PlaySignallingDtmfOnTx(dtmfData->slotId,
                    *dtmfCharsPtr, cb, context);
            if (res != PA_OK) {
                LE_ERROR("Play tone request failed, err %d", (int)res);
                if(playingFirstDtmf) {
                    dtmfData->result = LE_FAULT;
                    le_sem_Post(audio.mDtmfStartedSemRefTx);
                }
                *dtmfData = {};
                audio.mDtmfStartedTx = false;
                audio.mDtmfTxPaused = false;
                return NULL;
            }
            audio.mDtmfTxPaused = false;
            pa_result_t cbRes = prom->get_future().get();
            if(cbRes != PA_OK)
            {
                LE_ERROR("Failed to play dtmf signalling on slot Id: %d", dtmfData->slotId);
                dtmfData->result = (le_result_t)cbRes;
                if(playingFirstDtmf) {
                    le_sem_Post(audio.mDtmfStartedSemRefTx);
                }
                *dtmfData = {};
                return NULL;
            }
            if(playingFirstDtmf) {
                dtmfData->result = LE_OK;
                le_sem_Post(audio.mDtmfStartedSemRefTx);
                audio.mDtmfStartedTx = true;
                playingFirstDtmf = false;
            }
            LE_DEBUG("Play tone request sent successfully %c", *dtmfCharsPtr);
            std::this_thread::sleep_for(std::chrono::milliseconds(dtmfData->durationTx));
            if (!audio.mDtmfStartedTx)
            {
                LE_INFO("Dtmf signalling has stopped.");
                audio.mDtmfTxPaused = false;
                *dtmfData = {};
                return NULL;
            }
            CALLBACK1_TO_SET_PA_RESULT;
            res = taf_pa_audio_StopSignallingDtmfOnTx(dtmfData->slotId, cb1, context);
            audio.mDtmfTxPaused = true;
            if (res != PA_OK) {
                LE_ERROR("stop tone request failed, err %d", (int)res);
                *dtmfData = {};
                audio.mDtmfStartedTx = false;
                audio.mDtmfTxPaused = false;
                return NULL;
            }
            cbRes = prom1->get_future().get();
            if(cbRes != PA_OK)
            {
                LE_ERROR("Failed to stop dtmf signalling on slot Id: %d", dtmfData->slotId);
                *dtmfData = {};
                audio.mDtmfStartedTx = false;
                audio.mDtmfTxPaused = false;
                return NULL;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(dtmfData->pause));
            dtmfCharsPtr++;
        } else {
            LE_DEBUG("DTMF tone signalling stopped");
            break;
        }
    }
    LE_DEBUG("Signalling dtmf completed successfully");
    audio.mDtmfStartedTx = false;
    audio.mDtmfTxPaused = false;
    *dtmfData = {};
    audio.dtmfThreadTxRef = nullptr;
    return NULL;
}

/**
 * Play DTMF tone for TX path of voice call
 */
le_result_t taf_Audio::PlaySignallingDtmf
(
 uint32_t              slotId,
 const char*           dtmfPtr,
 uint32_t              uduration,
 uint32_t              upause
)
{
    TAF_ERROR_IF_RET_VAL(dtmfPtr == NULL, LE_BAD_PARAMETER,"dtmfPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(slotId != SLOT_ID_DEFAULT, LE_BAD_PARAMETER,"invalid slot ID");
    TAF_ERROR_IF_RET_VAL(mDtmfStartedTx == true, LE_BUSY,"A DTMF playback is in progress");

    dtmfDataTx.durationTx = uduration;
    dtmfDataTx.pause = upause;
    dtmfDataTx.dtmfChars = dtmfPtr;
    dtmfDataTx.slotId = slotId;
    dtmfDataTx.result = LE_FAULT;
    dtmfDataTx.sessionRef = taf_audio_GetClientSessionRef();

    mDtmfStartedSemRefTx = le_sem_Create("tafDtmfStartedSemRefTx", 0);
    dtmfThreadTxRef = le_thread_Create("DtmfThreadTx", playDTMFonTX, &dtmfDataTx);
    le_thread_SetJoinable(dtmfThreadTxRef);
    le_thread_Start(dtmfThreadTxRef);
    // Wait for DTMF to start successfully
    le_sem_Wait(mDtmfStartedSemRefTx);
    le_sem_Delete(mDtmfStartedSemRefTx);
    mDtmfStartedSemRefTx = nullptr;

    return dtmfDataTx.result;
}

/**
 * Play DTMF tone for RX path of voice call
 */
le_result_t taf_Audio::PlayDtmf
(
 taf_audio_StreamRef_t rStreamRef,
 const char*           dtmfPtr,
 uint16_t              uduration,
 uint32_t              upause,
 double                gain
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            rStreamRef);
    TAF_ERROR_IF_RET_VAL(streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
            LE_BAD_PARAMETER, "Invalid stream reference");
    TAF_ERROR_IF_RET_VAL(gain < 0 || gain > 1, LE_BAD_PARAMETER, "Invalid gain level");
    TAF_ERROR_IF_RET_VAL(dtmfPtr == NULL, LE_BAD_PARAMETER,"dtmfPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(mDtmfStarted == true, LE_BUSY,"A DTMF playback is in progress");

    dtmfDataRx.durationRx = uduration;
    dtmfDataRx.pause = upause;
    dtmfDataRx.dtmfGain = gain;
    dtmfDataRx.frequencyList.clear();
    dtmfDataRx.sessionRef = taf_audio_GetClientSessionRef();
    dtmfDataRx.streamRef = rStreamRef;

    if (mVoiceEnabled1) {
        while (*dtmfPtr != '\0') {
            std::pair<int, int> frequencies = getDTMFFrequencies(*dtmfPtr);
            if (frequencies.first != -1) {
                dtmfDataRx.frequencyList.emplace_back(frequencies);
            } else {
                LE_ERROR("Invalid DTMF key!");
                return LE_FAULT;
            }
            dtmfPtr++;
        }
    } else {
        LE_ERROR("Request to play Dtmf Tone failed, no active voice call.");
        return LE_FAULT;
    }
    mDtmfStartedSemRef = le_sem_Create("tafDtmfStartedSemRef", 0);
    dtmfThreadRef = le_thread_Create("DtmfThread", playAllDtmfTones, &dtmfDataRx);
    le_thread_SetJoinable(dtmfThreadRef);
    le_thread_Start(dtmfThreadRef);
    // Wait for DTMF to start successfully
    le_sem_Wait(mDtmfStartedSemRef);
    le_sem_Delete(mDtmfStartedSemRef);
    mDtmfStartedSemRef = nullptr;
    if(mDtmfStarted)
        return LE_OK;
    return LE_FAULT;
}

le_result_t taf_Audio::StopDtmf(taf_audio_StreamRef_t streamRef)
{
    TAF_ERROR_IF_RET_VAL( !mDtmfStarted, LE_BAD_PARAMETER,"No active dtmf to stop");
    pa_result_t paResult = PA_FAULT;
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
            LE_BAD_PARAMETER, "Invalid stream reference");
    // Cancel and join DTMF thread if running
    if (dtmfThreadRef)
    {
        LE_DEBUG("cancel dtmfThreadRef");
        le_thread_Cancel(dtmfThreadRef);
        le_thread_Join(dtmfThreadRef, NULL);
        dtmfThreadRef = nullptr;
    }

    std::any context = std::make_shared<taf_audio_Stream_t*>(streamPtr);
    CALLBACK_TO_SET_PA_RESULT;
    if (mVoiceEnabled1) {
        paResult = taf_pa_audio_StopDtmf(PaStreamDirection::RX, cb, NULL);
        if(paResult == PA_OK) {
            pa_result_t res = prom->get_future().get();
            if (res != PA_OK) {
                LE_ERROR("Stop Dtmf Tone failed");
                return LE_FAULT;
            }
            mDtmfStarted = false;
            dtmfDataRx = {};
        }else {
            LE_ERROR("Request to stop Dtmf Tone failed");
            return LE_FAULT;
        }
    } else {
        LE_ERROR("Request to play stop Tone failed");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_Audio::StopSignallingDtmf(uint32_t slotId) {

    TAF_ERROR_IF_RET_VAL(!mDtmfStartedTx, LE_BAD_PARAMETER, "No active dtmf signalling to stop");

    // Cancel and join DTMF TX thread if running
    if (dtmfThreadTxRef)
    {
        LE_DEBUG("Cancel dtmfThreadTxRef");
        le_thread_Cancel(dtmfThreadTxRef);
        le_thread_Join(dtmfThreadTxRef, NULL);
        dtmfThreadTxRef = nullptr;
    }
    // send stop request only when dtmf signalling is not in pause state
    if (!mDtmfTxPaused)
    {
        std::any context = std::make_shared<uint32_t>(slotId);
        CALLBACK_TO_SET_PA_RESULT;
        pa_result_t result = taf_pa_audio_StopSignallingDtmfOnTx(slotId, cb, context);
        if(result != PA_OK) {
            LE_ERROR("Failed to stop dtmf signalling on slot Id %d", slotId);
            return LE_FAULT;
        }
        result = prom->get_future().get();
        if (result != PA_OK) {
            LE_ERROR("Stop Dtmf signalling failed");
            return LE_FAULT;
        }
    }
    mDtmfStartedTx = false;
    dtmfDataTx = {};
    return LE_OK;
}

void taf_Audio::StartMpmsRetryTimer()
{
    if (mpmsRetryTimer == nullptr)
    {
        mpmsRetryTimer = le_timer_Create("mpmsRetryTimer");
        if (mpmsRetryTimer == nullptr)
        {
            LE_ERROR("Failed to create MPMS retry timer");
            return;
        }
        le_timer_SetHandler(mpmsRetryTimer, MpmsConnectHandler);
        le_timer_SetWakeup(mpmsRetryTimer, false);
        le_timer_SetMsInterval(mpmsRetryTimer, MPMS_CONN_RETRY_TIMER_INTERVAL);// 1 sec interval
    }
    le_timer_SetRepeat(mpmsRetryTimer, MAX_NUM_OF_MPMS_CONN_ATTEMPTS);
    le_timer_Start(mpmsRetryTimer);
    LE_INFO("MPMS retry timer started with 3 retries at 1-second intervals");
}

void taf_Audio::MpmsDisconnectHandler(void* contextPtr)
{
    auto &audio = taf_Audio::GetInstance();
    LE_WARN("Disconnected from MPMS service, attempting to reconnect...");

    // Start the retry timer
    audio.StartMpmsRetryTimer();
}

void taf_Audio::TafSigTermEventHandler(int tafSigNum)
{
    LE_INFO("TafSigTermEventHandler signal : %d", tafSigNum);
    auto &audio = taf_Audio::GetInstance();
    audio.CleanUpBeforeExit();
    exit(EXIT_SUCCESS);
}

void taf_Audio::CleanupAudioResources()
{
    LE_INFO("Reset audio resources");

    // reset as all functionalities will be stopped in lower layer on subsystem unavailable.
    mCallStarted = false;
    mVoiceEnabled1 = false;
    mDtmfStarted = false;
    mDtmfStartedTx = false;
    mIsRecording = false;
    mIsRxRecording = false;
    mIsPlaying = false;
    mIsTxPlaying = false;
    mIsCaptureStreamCreated = false;
    mIsRxCaptureStreamCreated = false;

    pa_result_t result = taf_pa_audio_Deinit();
    if (result != PA_OK)
    {
        LE_ERROR("Failed to deinitialize PA audio, err : %d", (int)result);
    }

    CleanUpThreadsTimers();
    LE_INFO("Audio resources cleanup completed");
}

void taf_Audio::CleanUpBeforeExit()
{
    LE_DEBUG("Starting cleanup before exit");
    pa_result_t result = PA_FAULT;
    if(mDtmfStarted)
    {
        LE_DEBUG("stopDtmfRx");
        StopDtmf(dtmfDataRx.streamRef);
    }
    if(mDtmfStartedTx)
    {
        LE_DEBUG("stopDtmfSignalling");
        StopSignallingDtmf(dtmfDataTx.slotId);
    }

    if(mDtmfAudioRef && mVoiceEnabled1 && mDtmfListener) {
        result = taf_pa_audio_deregisterDtmfListener(mDtmfListener);
        if(result != PA_OK) {
            LE_ERROR("Request to deregister for DTMF detection failed error : %d", (int)result);
        }
        mDtmfAudioRef = nullptr;
        mDtmfListener = nullptr;
    }

    if(subSystemStatusChangelistenerId != 0)
    {
        LE_DEBUG("deregister subSystemStatusChangeListener");
        result = RemoveSubsystemStateChangeListener(subSystemStatusChangelistenerId);
        if(result != PA_OK) {
            LE_ERROR("Request to deregister for subsystemStatusChange failed error : %d",
                    (int)result);
        }
        subSystemStatusChangelistenerId = 0;
    }

    // Stop active playback and recordings
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(StreamRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Stream_t* audioStreamPtr =
                (taf_audio_Stream_t*) le_ref_GetValue(iteratorRef);
        if ( (audioStreamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
                || (audioStreamPtr->interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
        {
            LE_DEBUG("stopAudio %d", audioStreamPtr->interface);
            StopAudio(audioStreamPtr);
            DeleteAudioStream(audioStreamPtr);
        }
    }

    // Close active route
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_audio_Route_t* routePtr =
                (taf_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr )
        {
            LE_DEBUG("CloseRoute for %p routeRef", iteratorRef);
            CloseRoute(routePtr->routeRef);
        }
    }

    result = taf_pa_audio_Deinit();
    if (result != PA_OK)
    {
        LE_ERROR("Failed to deinitialize PA audio, err : %d", (int)result);
    }
    CleanUpThreadsTimers();
    LE_INFO("Cleanup before exit completed");
}

void taf_Audio::CleanUpThreadsTimers()
{
    // Remove event handlers
    if (subsystemStateHandlerRef)
    {
        LE_DEBUG("remove subsystemStateHandlerRef");
        le_event_RemoveHandler(subsystemStateHandlerRef);
        subsystemStateHandlerRef = nullptr;
    }

    // Cancel and join all active threads
    if (dtmfThreadTxRef)
    {
        LE_DEBUG("Cancel dtmfThreadTxRef");
        le_thread_Cancel(dtmfThreadTxRef);
        le_thread_Join(dtmfThreadTxRef, NULL);
        dtmfThreadTxRef = nullptr;
    }

    if (bufferHandlingThreadRef)
    {
        LE_DEBUG("Cancel bufferHandlingThreadRef");
        le_event_QueueFunctionToThread(bufferHandlingThreadRef,
            RemoveBufferHandler, nullptr, nullptr);
        le_thread_Join(bufferHandlingThreadRef, NULL);
        bufferHandlingThreadRef = nullptr;
    }

    if (dtmfThreadRef)
    {
        LE_DEBUG("Cancel dtmfThreadRef");
        le_thread_Cancel(dtmfThreadRef);
        le_thread_Join(dtmfThreadRef, NULL);
        dtmfThreadRef = nullptr;
    }

    // Stop and delete timers if active
    if (vhalRetryTimer)
    {
        LE_DEBUG("stop vhalRetryTimer");
        le_timer_Stop(vhalRetryTimer);
        le_timer_Delete(vhalRetryTimer);
        vhalRetryTimer = nullptr;
    }

    if (bubHandlerRef)
    {
        LE_DEBUG("remove bub handler");
        taf_mngdPm_RemoveInfoReportHandler(bubHandlerRef);
        bubHandlerRef = nullptr;
    }

    if (mpmsRetryTimer)
    {
        LE_DEBUG("stop mpmsRetryTimer");
        le_timer_Stop(mpmsRetryTimer);
        le_timer_Delete(mpmsRetryTimer);
        mpmsRetryTimer = nullptr;
    }
    if (mpmsDelayTimer)
    {
        LE_DEBUG("stop mpmsDelayTimer");
        le_timer_Stop(mpmsDelayTimer);
        le_timer_Delete(mpmsDelayTimer);
        mpmsDelayTimer = nullptr;
    }

    // Close file handles if open
    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
    }
    if (mRxFile) {
        fclose(mRxFile);
        mRxFile = nullptr;
    }
}

void taf_Audio::SubsystemStateChangeCallback(tafpa::audio::SubsystemState_e state,
                                           std::shared_ptr<void> context)
{
    auto &audio = taf_Audio::GetInstance();
    switch(state)
    {
        case SubsystemState_e::UNAVAILABLE:
            LE_ERROR("Audio subsystem became unavailable.");
            // Report subsystem unavailable event to main thread
            le_event_Report(audio.subsystemStateEventId, &state,
                    sizeof(tafpa::audio::SubsystemState_e));
            break;

        case SubsystemState_e::AVAILABLE:
            LE_INFO("Audio subsystem is now available");
            break;

        default:
            LE_DEBUG("Unknown audio subsystem state: %d", static_cast<int>(state));
            break;
    }
}
