/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tafMngdAudio.hpp"
#include "tafMngdAudioVhal.hpp"

using namespace telux::tafsvc;
using namespace taf::audioVhal;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(tafMngdAudioConnector, MAX_CONNECTOR,
        sizeof(taf_mngd_audio_Connector_t));
LE_MEM_DEFINE_STATIC_POOL(tafMngdAudioHashmap, HASHMAP_SIZE, sizeof(taf_mngd_audio_hashMapList_t));
LE_MEM_DEFINE_STATIC_POOL(tafMngdAudioStream, MAX_STREAM,
        sizeof(taf_mngd_audio_Stream_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef, MAX_STREAM, sizeof(taf_SessionRefNode_t));
LE_MEM_DEFINE_STATIC_POOL(tafMngdAudioRoute, MAX_ROUTE, sizeof(taf_mngd_audio_Route_t));
LE_MEM_DEFINE_STATIC_POOL(tafEventIdPool, MAX_STREAM, sizeof(tafEventIdList));
LE_MEM_DEFINE_STATIC_POOL(tafEventHandlerRef, MAX_CONNECTOR, sizeof(EventHandlerRefNode_t));
LE_MEM_DEFINE_STATIC_POOL(tafPlaybackListRef, MAX_NUM_OF_PLAYLIST, sizeof(taf_PlaybackList_t));

// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.gCallbackPromise = promise<ErrorCode>();
}

/**
 * Returns managed audio instance
 */
taf_MngdAudio &taf_MngdAudio::GetInstance()
{
    static taf_MngdAudio instance;
    return instance;
}

void taf_MngdAudio::Init(void)
{
    LE_INFO("taf_MngdAudio: Init");

    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();

    auto &audioFactory = AudioFactory::getInstance();

    mAudioManager = audioFactory.getAudioManager();
    bool isReady = false;
    if (mAudioManager) {
        isReady = mAudioManager->isSubsystemReady();
    } else {
        LE_FATAL("Invalid Audio Manager");
        return;
    }
    telux::common::ErrorCode ec;

    ec = AudioFactory::getInstance().getAudioPlayer(mAudioPlayer);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("can't get IAudioPlayer");
        return;
    }

    if (!isReady) {
        LE_INFO("Audio subsystem is not ready, Please wait ...");
        std::future<bool> f = mAudioManager->onSubsystemReady();
        isReady = f.get();
    }

    if (isReady) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_INFO("Elapsed Time for Audio Subsystems to ready : %f", elapsedTime.count());
    } else {
        LE_FATAL(" *** ERROR - Unable to initialize audio subsystem");
        return;
    }

    // Load audio VHAL driver
    auto &audioVhal = taf_MngdAudioVhal::GetInstance();
    audioVhal.Init();

    isVhalAvailable = audioVhal.isAudioDrvAvailable();
    LE_INFO("isVhalAvailable : %s", isVhalAvailable ? "true" : "false");

    ConnectorPool  = le_mem_InitStaticPool(tafMngdAudioConnector, MAX_CONNECTOR,
            sizeof(taf_mngd_audio_Connector_t));
    ConnectorRefMap = le_ref_CreateMap("TafMngdAudioConnMap", MAX_CONNECTOR);
    HashMapPool  = le_mem_InitStaticPool(tafMngdAudioHashmap, HASHMAP_SIZE,
            sizeof(taf_mngd_audio_hashMapList_t));
    StreamPool  = le_mem_InitStaticPool(tafMngdAudioStream, MAX_STREAM,
            sizeof(taf_mngd_audio_Stream_t));
    le_mem_SetDestructor(StreamPool, DestructStream);
    StreamRefMap = le_ref_CreateMap("TafMngdAudioStreamMap", MAX_STREAM);
    RoutePool  = le_mem_InitStaticPool(tafMngdAudioRoute, MAX_ROUTE,
            sizeof(taf_mngd_audio_Route_t));
    RouteRefMap = le_ref_CreateMap("TafMngdAudioRouteMap", MAX_ROUTE);
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef, MAX_STREAM,
            sizeof(taf_SessionRefNode_t));
    EventIdPool = le_mem_InitStaticPool(tafEventIdPool, MAX_STREAM, sizeof(tafEventIdList));
    EventHandlerRefNodePool = le_mem_InitStaticPool(tafEventHandlerRef, MAX_CONNECTOR,
            sizeof(EventHandlerRefNode_t));
    EventHandlerRefMap = le_ref_CreateMap("TAFEventHandlerMap", MAX_CONNECTOR);
    PlaybackListPool = le_mem_InitStaticPool(tafPlaybackListRef, MAX_NUM_OF_PLAYLIST,
            sizeof(taf_PlaybackList_t));
    PlaybackListRefMap = le_ref_CreateMap("TAFPlaybackListMap", MAX_NUM_OF_PLAYLIST);

    mSemRef = le_sem_Create("tafSem", 0);

    HashMapList = LE_DLS_LIST_INIT;

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( taf_mngd_audio_GetServiceRef(),
                                   ClientSessionCloseEventHandler,
                                   NULL );
}

void taf_MngdAudio::ClientSessionCloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();

    LE_DEBUG("ClientSessionCloseEventHandler sessionRef : %p", sessionRef);

    // Close audio streams
    // This is a two stage process: parse audio stream reference map
    // once in order to close dsp frontend file play/capture streams
    // first, then parse it a second time to close remaining streams.
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(mngdAudio.StreamRefMap);
    bool isSessionMatched = false;
    taf_SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* lPtr;
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Stream_t* audioStreamPtr =
                (taf_mngd_audio_Stream_t*) le_ref_GetValue(iteratorRef);
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
                && ((audioStreamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
                || (audioStreamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)))
        {
            mngdAudio.StopAudio(audioStreamPtr);
            mngdAudio.DeleteAudioStream(audioStreamPtr);
        }
    }

    iteratorRef = le_ref_GetIterator(mngdAudio.RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr && routePtr->sessionRef == sessionRef )
        {
            LE_INFO("CloseRoute for %p routeRef", iteratorRef);
            mngdAudio.CloseRoute(routePtr->routeRef);
            break;
        }
    }

    // Close audio streams
    iteratorRef = le_ref_GetIterator(mngdAudio.StreamRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Stream_t* streamPtr =
                (taf_mngd_audio_Stream_t*) le_ref_GetValue(iteratorRef);
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
            mngdAudio.ReleaseStream(streamPtr, sessionRef, true);
        }
    }

    iteratorRef = le_ref_GetIterator(mngdAudio.ConnectorRefMap);

    le_result_t result = le_ref_NextNode(iteratorRef);
    // Close connectors
    while ( result == LE_OK )
    {
        taf_mngd_audio_ConnectorRef_t connectorRef =
                (taf_mngd_audio_ConnectorRef_t) le_ref_GetSafeRef(iteratorRef);
        taf_mngd_audio_Connector_t* connectorPtr =
                (taf_mngd_audio_Connector_t*)le_ref_Lookup(mngdAudio.ConnectorRefMap, connectorRef);
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
            taf_mngd_audio_DeleteConnector( connectorRef );
        }
    }

    iteratorRef = le_ref_GetIterator(mngdAudio.PlaybackListRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_PlaybackList_t* playListPtr = (taf_PlaybackList_t*) le_ref_GetValue(iteratorRef);

        if ( playListPtr && playListPtr->sessionRef == sessionRef )
        {
            LE_INFO("Delete PlayList for %p playListRef", iteratorRef);
            mngdAudio.DeletePlayList(playListPtr->playListRef);
            break;
        }
    }

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

le_hashmap_Ref_t taf_MngdAudio::GetHashMap
(
 void
)
{
    taf_mngd_audio_hashMapList_t* currentPtr = NULL;

    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);

    while (lPtr != NULL)
    {
        currentPtr = CONTAINER_OF(lPtr, taf_mngd_audio_hashMapList_t, hashMapLink);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one HashMap unused (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = true;
            return currentPtr->hashMapRef;
        }
        lPtr = le_dls_PeekNext(&HashMapList, lPtr);
    }

    currentPtr = (taf_mngd_audio_hashMapList_t*)le_mem_ForceAlloc(HashMapPool);

    char ConnMapName[20];

    snprintf( ConnMapName, 20, "ConnMap%d", (int)(le_dls_NumLinks(&HashMapList)+1) );

    currentPtr->hashMapRef = le_hashmap_Create(ConnMapName, HASHMAP_SIZE, HashRef, EqualsRef);
    currentPtr->isUsed = true;
    currentPtr->hashMapLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&HashMapList,&(currentPtr->hashMapLink));

    LE_DEBUG("Create a new HashMap (%p) %s", currentPtr->hashMapRef, ConnMapName);

    return currentPtr->hashMapRef;
}

void taf_MngdAudio::ClearHashMap
(
    le_hashmap_Ref_t hashMapRef
)
{
    LE_ASSERT(hashMapRef);

    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);
    taf_mngd_audio_hashMapList_t* currentPtr;

    while (lPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(lPtr, taf_mngd_audio_hashMapList_t, hashMapLink);

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

void taf_MngdAudio::DeleteHashMap
(
    taf_mngd_audio_Connector_t* connPtr
)
{

    TAF_ERROR_IF_RET_NIL(connPtr == NULL,  "connPtr is nullptr!");

    le_hashmap_It_Ref_t Iterator;
    taf_mngd_audio_Stream_t const * currentStreamPtr;

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

        if (currentStreamPtr != nullptr)
        {
            le_hashmap_Remove(currentStreamPtr->connList, connPtr);
        }
    }

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioOutList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

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
void taf_MngdAudio::CloseConnectorPaths
(
    taf_mngd_audio_Connector_t*   connPtr
)
{
    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");

    LE_DEBUG("CloseConnectorPaths %p", connPtr);

    le_hashmap_It_Ref_t Iterator =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    taf_mngd_audio_Stream_t* currentStreamPtr;
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr=(taf_mngd_audio_Stream_t*)le_hashmap_GetValue(Iterator);

        if(currentStreamPtr != nullptr)
        {
            StopandDelete(currentStreamPtr, connPtr->audioOutList);
        }
    }
}

/**
 * Stop and delete the current stream
 */
le_result_t taf_MngdAudio::StopandDelete
(
    taf_mngd_audio_Stream_t*    streamPtr,
    le_hashmap_Ref_t            streamListPtr
)
{
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    le_result_t   res = LE_OK;
    taf_mngd_audio_Stream_t* inputPtr;
    taf_mngd_audio_Stream_t* outputPtr;
    taf_mngd_audio_Stream_t* currentPtr;

    LE_DEBUG("StopandDelete stream.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        currentPtr=(taf_mngd_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

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
    if(streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4) {
        res = StopAudio(inputPtr);
        res = DeleteAudioStream(inputPtr);
    }
    else if(streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_0
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_1
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_2
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_3
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_4) {
        res = StopAudio(outputPtr);
        res = DeleteAudioStream(outputPtr);
    }
    return res;
}

/**
 * Create new connector for input and output stream reference
 */
taf_mngd_audio_ConnectorRef_t taf_MngdAudio::CreateConnector
(
    void
)
{
    taf_mngd_audio_Connector_t* newconnPtr =
            (taf_mngd_audio_Connector_t*)le_mem_ForceAlloc(ConnectorPool);

    newconnPtr->audioInList   = GetHashMap();
    newconnPtr->audioOutList  = GetHashMap();
    newconnPtr->sessionRef = taf_mngd_audio_GetClientSessionRef();
    newconnPtr->connRef = (taf_mngd_audio_Connector_t*)
            le_ref_CreateRef(ConnectorRefMap, newconnPtr);
    newconnPtr->connLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&ConnectorList, &(newconnPtr->connLink));
    return newconnPtr->connRef;
}

/**
 * Delete the connecter
 */
void taf_MngdAudio::DeleteConnector
(
    taf_mngd_audio_ConnectorRef_t connectorRef
)
{
    taf_mngd_audio_Connector_t* connPtr =
            (taf_mngd_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connectorRef);

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
le_result_t taf_MngdAudio::Connect
(
    taf_mngd_audio_ConnectorRef_t connRef,
    taf_mngd_audio_StreamRef_t    streamRef
)
{
    le_hashmap_Ref_t lPtr = NULL;
    le_result_t res;
    taf_mngd_audio_Stream_t* sPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    taf_mngd_audio_Connector_t* connPtr =
            (taf_mngd_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connRef);

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

    if(sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4)
    {
        mSpeaker = true;
    }
    else if(sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_0
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_1
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_2
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_3
            || sPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_4)
    {
        mMic = true;
    }
    else if(sPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        mModemRx = true;
    }
    else if(sPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
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
void taf_MngdAudio::Disconnect
(
    taf_mngd_audio_ConnectorRef_t connRef,
    taf_mngd_audio_StreamRef_t    streamRef
)
{
    taf_mngd_audio_Stream_t* streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    taf_mngd_audio_Connector_t* connPtr =
            (taf_mngd_audio_Connector_t*)le_ref_Lookup(ConnectorRefMap, connRef);

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    LE_DEBUG("Disconnect stream.%p from connector.%p", streamRef, connRef);
    if(streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4)
    {
        mSpeaker = false;
    } else if(streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_0
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_1
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_2
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_3
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_4)
    {
        mMic = false;
    } else if(streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) {
        mModemRx = false;
        mRxSlotId = INVALID_SLOT_ID;
    }
    else if(streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) {
        mModemTx = false;
        mTxSlotId = INVALID_SLOT_ID;
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
void taf_MngdAudio::Close
(
    taf_mngd_audio_StreamRef_t streamRef
)
{
    taf_mngd_audio_Stream_t*  streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);

    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    ReleaseStream(streamPtr, taf_mngd_audio_GetClientSessionRef(), false);
}

/**
 * Release stream reference
 */
void taf_MngdAudio::ReleaseStream
(
    taf_mngd_audio_Stream_t*  streamPtr,
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
taf_mngd_audio_StreamRef_t taf_MngdAudio::OpenModemVoiceRx
(
    uint32_t slotId
)
{
    TAF_ERROR_IF_RET_VAL((mTxSlotId != INVALID_SLOT_ID && mTxSlotId != (SlotId)slotId), NULL,
            "Invalid slotID, use same slotID for Rx and Tx");

    TAF_ERROR_IF_RET_VAL(slotId > MAX_SLOT_ID, NULL,
            "slotId is greater than MAX slot ID, use valid slot ID");

    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        LE_INFO("routePtr mode is %d", routePtr->mode);
        TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_MNGD_AUDIO_VOICE_CALL,
                NULL, "Stream not supported with the active route");
    }

    StreamConfig_t streamConfig;
    mRxSlotId = (SlotId)slotId;
    streamConfig.HwDevice = true;
    streamConfig.interface = TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;

    return CreateStream(&streamConfig);
}

/**
 * Setup for voice instream
 */
taf_mngd_audio_StreamRef_t taf_MngdAudio::OpenModemVoiceTx
(
    uint32_t slotId, bool enableEcnr
)
{

    TAF_ERROR_IF_RET_VAL((mRxSlotId != INVALID_SLOT_ID && mRxSlotId != (SlotId)slotId), NULL,
            "Invalid slotID, use same slotID for Rx and Tx");

    TAF_ERROR_IF_RET_VAL(slotId > MAX_SLOT_ID, NULL,
            "slotId is greater than MAX slot ID, use valid slot ID");

    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_MNGD_AUDIO_VOICE_CALL,
                NULL, "Stream not supported with the active route");
    }

    mTxSlotId = (SlotId)slotId;
    isEcnrEnabled = enableEcnr;
    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.interface = TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;

    return CreateStream(&streamConfig);
}

/**
 * Create stream for input and output stream reference
 * if not opened already
 */
taf_mngd_audio_StreamRef_t taf_MngdAudio::CreateStream
(
    StreamConfig_t* streamConfPtr
)
{
    LE_DEBUG("Create audio stream (%d)", streamConfPtr->interface);
    bool isOpened = false;
    taf_mngd_audio_Stream_t* streamPtr = NULL;
    le_ref_IterRef_t iterRef;
    taf_SessionRefNode_t* newSessionRefPtr;

    if (streamConfPtr->HwDevice)
    {
        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(StreamRefMap);

        while (!isOpened && (le_ref_NextNode(iterRef) == LE_OK))
        {
            streamPtr = (taf_mngd_audio_Stream_t*) le_ref_GetValue(iterRef);

            if (streamPtr && streamPtr->interface == streamConfPtr->interface)
            {
                LE_INFO("streamPtr->interface %d", streamPtr->interface);
                isOpened = true;
            }
        }
    }

    if ( !isOpened )
    {
        streamPtr = (taf_mngd_audio_Stream_t*)le_mem_ForceAlloc(StreamPool);

        InitStream(streamPtr);

        streamPtr->interface = streamConfPtr->interface;
        streamPtr->device = !CHECK_OUTPUT_IF(streamPtr->interface);
        streamPtr->sessionRefList = LE_DLS_LIST_INIT;
        switch ( streamPtr->interface )
        {
            case TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
            case TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
                streamPtr->eventId = CreateEventId();
                streamPtr->direction = streamConfPtr->direction;
                break;
            default:
                break;
        }
        streamPtr->streamRef = (taf_mngd_audio_StreamRef_t)
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
    newSessionRefPtr->sessionRef = taf_mngd_audio_GetClientSessionRef();
    newSessionRefPtr->refNodeLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&streamPtr->sessionRefList, &(newSessionRefPtr->refNodeLink));

    return streamPtr->streamRef;
}

le_event_Id_t taf_MngdAudio::CreateEventId
(
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    tafEventIdList* curPtr = NULL;
    le_dls_Link_t*      linkPtr = le_dls_Peek(&mngdAudio.EventIdList);
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
        linkPtr = le_dls_PeekNext(&mngdAudio.EventIdList, linkPtr);

        eventId++;
    }

    snprintf(eventName, sizeof(eventName), "eventId-%d", eventId);

    curPtr = (tafEventIdList*)le_mem_ForceAlloc(mngdAudio.EventIdPool);
    curPtr->eventId = le_event_CreateId(eventName, sizeof(taf_mngd_audio_StreamEvent_t));
    curPtr->inUse = true;
    curPtr->next = LE_DLS_LINK_INIT;

    le_dls_Queue(&mngdAudio.EventIdList, &(curPtr->next));

    LE_INFO("Create a new eventId (%p)", curPtr->eventId);

    return curPtr->eventId;
}

/**
 * Initialize stream reference
 */
void taf_MngdAudio::InitStream
(
    taf_mngd_audio_Stream_t* streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    memset(streamPtr, 0, sizeof(taf_mngd_audio_Stream_t));
    streamPtr->fd = -1;
    streamPtr->connList = GetHashMap();
}

static void DeleteEventId
(
 le_event_Id_t eventId
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    le_dls_Link_t* linkPtr = le_dls_Peek(&mngdAudio.EventIdList);

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
        linkPtr = le_dls_PeekNext(&mngdAudio.EventIdList,linkPtr);
    }

    LE_DEBUG("Nothing to delete");
    return;
}

/**
 * Stream destructor
 */
void taf_MngdAudio::DestructStream( void *objPtr )
{
    LE_DEBUG("DestructStream");
    LE_ASSERT(objPtr);
    auto &mngdAudio = taf_MngdAudio::GetInstance();

    taf_mngd_audio_Stream_t* streamPtr = (taf_mngd_audio_Stream_t*)objPtr;

    // Close the active Route
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(mngdAudio.RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if ( routePtr && (routePtr->sinkRef == streamPtr || routePtr->sourceRef == streamPtr))
        {
            LE_INFO("CloseRoute for %p routeRef", iteratorRef);
            mngdAudio.CloseRoute(routePtr->routeRef);
            break;
        }
    }

    mngdAudio.DisconnectConnectors(streamPtr);

    le_hashmap_RemoveAll(streamPtr->connList);
    mngdAudio.ClearHashMap(streamPtr->connList);
    DeleteEventId(streamPtr->eventId);
    le_ref_DeleteRef(mngdAudio.StreamRefMap, streamPtr->streamRef);
}

/**
 * Disconnect the Stream from all connectors
 */
void taf_MngdAudio::DisconnectConnectors
(
    taf_mngd_audio_Stream_t*     streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");
    LE_DEBUG("DisconnectConnectors streamPtr.%p", streamPtr);

    // Disconnect from all the connectors the stream is connected
    le_hashmap_It_Ref_t Iterator =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_mngd_audio_Connector_t const * currentconnPtr;

    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(Iterator);
        if(currentconnPtr != nullptr)
        {
            Disconnect(currentconnPtr->connRef, streamPtr->streamRef);
        }
    }
}

taf_mngd_audio_RouteRef_t taf_MngdAudio::OpenRoute( taf_mngd_audio_RouteId_t routeId,
        taf_mngd_audio_Mode_t mode, taf_mngd_audio_StreamRef_t *sinkRef,
        taf_mngd_audio_StreamRef_t *sourceRef)
{
    LE_INFO("OpenRoute route : %d mode : %d", routeId, mode);

    TAF_ERROR_IF_RET_VAL(routeId >= TAF_MNGD_AUDIO_ROUTE_3, NULL,
            "Not supported or invalid Route ID!");

    le_ref_IterRef_t iterRef;
    taf_mngd_audio_Route_t* routePtr;
    iterRef = (le_ref_IterRef_t)le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        routePtr = (taf_mngd_audio_Route_t*) le_ref_GetValue(iterRef);
        TAF_ERROR_IF_RET_VAL(routePtr == NULL, NULL, "Invalid routePtr!");
        if(mode == TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN)
        {
            LE_INFO("Force close the previous opened route");
            CloseRoute(routePtr->routeRef);
            break;
        }
        else
        {
            LE_ERROR("Another route is already active set true");
            return NULL;
        }
    }
    routePtr = (taf_mngd_audio_Route_t*)le_mem_ForceAlloc(RoutePool);
    routePtr->routeId = routeId;
    routePtr->sessionRef = taf_mngd_audio_GetClientSessionRef();

    if(mode == TAF_MNGD_AUDIO_VOICE_CALL || mode == TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN)
    {
        routePtr->mode = TAF_MNGD_AUDIO_VOICE_CALL;

        // Create Sink Ref
        StreamConfig_t sinkStreamConf;
        sinkStreamConf.HwDevice = true;
        if (routeId == TAF_MNGD_AUDIO_ROUTE_0)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_1)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_2)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_3)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_4)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4;
        *sinkRef = CreateStream(&sinkStreamConf);

        // Create Source Ref
        StreamConfig_t sourceStreamConf;
        sourceStreamConf.HwDevice = true;
        if (routeId == TAF_MNGD_AUDIO_ROUTE_0)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_0;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_1)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_1;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_2)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_2;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_3)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_3;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_4)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_4;
        *sourceRef = CreateStream(&sourceStreamConf);

        routePtr->sinkRef = *sinkRef;
        routePtr->sourceRef = *sourceRef;
    } else if (mode == TAF_MNGD_AUDIO_LOCAL_RECORDING)
    {
        // Create Source Ref
        StreamConfig_t sourceStreamConf;
        sourceStreamConf.HwDevice = true;
        if (routeId == TAF_MNGD_AUDIO_ROUTE_0)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_0;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_1)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_1;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_2)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_2;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_3)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_3;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_4)
            sourceStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_MIC_4;
        *sourceRef = CreateStream(&sourceStreamConf);

        routePtr->mode = TAF_MNGD_AUDIO_LOCAL_RECORDING;
        routePtr->sourceRef = *sourceRef;
    } else if (mode == TAF_MNGD_AUDIO_LOCAL_PLAYBACK)
    {
        // Create Sink Ref
        StreamConfig_t sinkStreamConf;
        sinkStreamConf.HwDevice = true;
        if (routeId == TAF_MNGD_AUDIO_ROUTE_0)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_1)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_2)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_3)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3;
        else if (routeId == TAF_MNGD_AUDIO_ROUTE_4)
            sinkStreamConf.interface = TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4;
        *sinkRef = CreateStream(&sinkStreamConf);

        routePtr->mode = TAF_MNGD_AUDIO_LOCAL_PLAYBACK;
        routePtr->sinkRef = *sinkRef;
    }

    routePtr->routeRef = (taf_mngd_audio_RouteRef_t)le_ref_CreateRef(RouteRefMap, routePtr);
    LE_INFO("Route ref is %p", routePtr->routeRef);
    return routePtr->routeRef;
}

le_result_t taf_MngdAudio::CloseRoute( taf_mngd_audio_RouteRef_t routeRef )
{
    LE_INFO("CloseRoute %p", routeRef);

    taf_mngd_audio_Route_t*  routePtr =
            (taf_mngd_audio_Route_t*)le_ref_Lookup(RouteRefMap, routeRef);
    TAF_ERROR_IF_RET_VAL(routePtr == NULL, LE_BAD_PARAMETER, "Invalid Route ref!");

    if (routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL
            || routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN) {
        // Set ctl status to VHAL before stoping voice call audio.
        setVhalRouteStatus(routePtr->mode, false);

        // Release sink and source reference
        taf_mngd_audio_Stream_t*  sinkStreamPtr =
                (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);

        taf_mngd_audio_Stream_t*  sourceStreamPtr =
                (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);

        TAF_ERROR_IF_RET_VAL( sinkStreamPtr == NULL || sourceStreamPtr == NULL, LE_FAULT,
                "sinkRef or sourceRef is invalid!");
        ReleaseStream(sinkStreamPtr, routePtr->sessionRef, false);
        ReleaseStream(sourceStreamPtr, routePtr->sessionRef, false);
    } else if (routePtr->mode == TAF_MNGD_AUDIO_LOCAL_PLAYBACK)
    {
        // Release sink reference
        taf_mngd_audio_Stream_t*  sinkStreamPtr =
                (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);

        TAF_ERROR_IF_RET_VAL( sinkStreamPtr == NULL, LE_FAULT,
                "sinkRef is invalid!");
        ReleaseStream(sinkStreamPtr, routePtr->sessionRef, false);

    } else if (routePtr->mode == TAF_MNGD_AUDIO_LOCAL_RECORDING)
    {
        // Release source reference
        taf_mngd_audio_Stream_t*  sourceStreamPtr =
                (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);

        TAF_ERROR_IF_RET_VAL( sourceStreamPtr == NULL, LE_FAULT,
                "sourceRef is invalid!");
        ReleaseStream(sourceStreamPtr, routePtr->sessionRef, false);
    }

    LE_DEBUG("Release routeRef %p", routeRef);
    le_ref_DeleteRef(RouteRefMap, routePtr->routeRef);
    le_mem_Release(routePtr);

    return LE_OK;
}

le_result_t taf_MngdAudio::ConnectStreamPaths( taf_mngd_audio_Stream_t* streamPtr,
        le_hashmap_Ref_t streamListPtr )
{
    LE_INFO("ConnectStreamPaths");
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER, "streamPtr is nullptr!");
    taf_mngd_audio_Stream_t* inputPtr;
    taf_mngd_audio_Stream_t* outputPtr;
    taf_mngd_audio_Stream_t* currentPtr;
    le_result_t res = LE_FAULT;

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator) == LE_OK)
    {
        currentPtr = (taf_mngd_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

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

        LE_DEBUG("Input [%d] and Output [%d] are tied together.",
                inputPtr->interface,
                outputPtr->interface);

        // Set the config device type based on output device
        if (outputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
            LE_DEBUG("set config with device type speaker %d", DEVICE_TYPE_SINK_0);
        }
        else if (outputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
            LE_DEBUG("set config with device type speaker %d", DEVICE_TYPE_SINK_1);
        }
        else if (outputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
            LE_DEBUG("set config with device type speaker %d", DEVICE_TYPE_SINK_2);
        }
        else if (outputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_3);
            LE_DEBUG("set config with device type speaker %d", DEVICE_TYPE_SINK_3);
        }
        else if (outputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_4);
            LE_DEBUG("set config with device type speaker %d", DEVICE_TYPE_SINK_4);
        }

        // Set the config device type based on input device
        if (inputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_0) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_0);
            LE_DEBUG("set config with device type mic %d", DEVICE_TYPE_SOURCE_0);
        }
        else if (inputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_1) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_1);
            LE_DEBUG("set config with device type mic %d", DEVICE_TYPE_SOURCE_1);
        }
        else if (inputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_2) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_2);
            LE_DEBUG("set config with device type mic %d", DEVICE_TYPE_SOURCE_2);
        }
        else if (inputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_3) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_3);
            LE_DEBUG("set config with device type mic %d", DEVICE_TYPE_SOURCE_3);
        }
        else if (inputPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_4) {
            voiceStreamConfig.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_4);
            LE_DEBUG("set config with device type mic %d", DEVICE_TYPE_SOURCE_4);
        }

        if (mModemRx && mModemTx && mSpeaker && mMic && !mCallStarted)
        {
            voiceStreamConfig.type = StreamType::VOICE_CALL;
            voiceStreamConfig.slotId = SLOT_ID_1;
            voiceStreamConfig.format = AudioFormat::PCM_16BIT_SIGNED;
            voiceStreamConfig.channelTypeMask = ChannelType::LEFT | ChannelType::RIGHT;
            voiceStreamConfig.formatParams = nullptr;

            if (isEcnrEnabled) {
                voiceStreamConfig.ecnrMode = EcnrMode::ENABLE;
            } else {
                voiceStreamConfig.ecnrMode = EcnrMode::DISABLE;
            }

            if(voiceStreamConfig.deviceTypes.size() >= 2)
            {
                if (voiceStreamConfig.sampleRate == 0)
                {
                    voiceStreamConfig.sampleRate = 16000;
                    LE_INFO("setting default sampling rate as 16000");
                }
                res = StartAudio(voiceStreamConfig);
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

void taf_MngdAudio::StartAudioCallback(ErrorCode error)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        mngdAudio.mVoiceEnabled1 = true;
        LE_INFO("audio started successfully");
    }
    mngdAudio.gCallbackPromise.set_value(error);
    return;
}

void taf_MngdAudio::DeleteVoiceCallback(ErrorCode error) {
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("deleteStream() succeeded.");
        mngdAudio.mAudioVoiceStream.reset();
        mngdAudio.mAudioVoiceStream = nullptr;
        mngdAudio.mCallStarted = false;
    }
    mngdAudio.gCallbackPromise.set_value(error);
    return;
}

void taf_MngdAudio::StopAudioCallback(ErrorCode error)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        mngdAudio.mVoiceEnabled1 = false;
        LE_INFO("audio stopped successfully");
    }
    mngdAudio.gCallbackPromise.set_value(error);
    return;
}

le_result_t taf_MngdAudio::StartAudio
(
    StreamConfig config
)
{
    LE_DEBUG("Create and Start audio\n");
    resetCallbackPromise();
    auto status = Status::FAILED;
    std::promise<bool> p;
    std::shared_ptr<telux::audio::IAudioStream> tafAudioStream;
    if (!mAudioManager){
        LE_ERROR("Invalid Audio Manager ");
        return LE_FAULT;
    }

    Status audioStatus = mAudioManager->createStream(config,
            [&p,&tafAudioStream,this](std::shared_ptr<telux::audio::IAudioStream> &stream,
                telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            tafAudioStream = stream;
            p.set_value(true);
        } else {
            p.set_value(false);
            LE_INFO("failed to Create a stream");
        }
    });
    if(audioStatus == Status::SUCCESS) {
        LE_DEBUG("Request to create stream sent" );
    } else {
        LE_ERROR("Request to create stream failed: %d", int(audioStatus));
        return LE_FAULT;
    }

    if (p.get_future().get()) {
        LE_DEBUG("Audio Stream is Created" );
        if(tafAudioStream->getType() == StreamType::VOICE_CALL) {
            if (config.slotId == SLOT_ID_1) {
                mAudioVoiceStream = std::dynamic_pointer_cast<
                    telux::audio::IAudioVoiceStream>(tafAudioStream);
            }
            LE_DEBUG("Voice Stream is Created on slot id %d ",config.slotId );

        } else if(tafAudioStream->getType() == StreamType::CAPTURE) {
            mAudioCaptureStream = std::dynamic_pointer_cast<
                    telux::audio::IAudioCaptureStream>(tafAudioStream);
            LE_DEBUG("Audio Capture Stream is Created" );
            setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_RECORDING, true);
        } else {
            LE_DEBUG("Unknown Stream Created" );
        }
    } else {
        LE_ERROR("Invalid configuration, failed to create the stream");
        return LE_FAULT;
    }

    if(mAudioVoiceStream && (config.slotId == SLOT_ID_1) && !mVoiceEnabled1) {
        status = mAudioVoiceStream->startAudio(StartAudioCallback);
        voiceStreamConfig = {};
    }

    if (mAudioVoiceStream) {
        if (status == Status::SUCCESS) {
            LE_DEBUG("Request to start voice stream sent");
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to start failed");
                return LE_FAULT;
            }
            mCallStarted = true;
            // Set VHAL ctl route status for voice call
            setVhalRouteStatus(TAF_MNGD_AUDIO_VOICE_CALL, true);
        } else {
            LE_ERROR("Request to start voice stream failed.\n");
            return LE_FAULT;
        }
    }
    return LE_OK;
}

/**
 * Get the player interface
 */
taf_mngd_audio_StreamRef_t taf_MngdAudio::OpenPlayer
(
    taf_mngd_audio_Direction_t direction
)
{
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if (direction == TAF_MNGD_AUDIO_TX)
            TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_MNGD_AUDIO_VOICE_CALL,
                    NULL, "Stream supported with the active voice call route");
        if (direction == TAF_MNGD_AUDIO_RX)
            TAF_ERROR_IF_RET_VAL(routePtr && !(routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL
                    || routePtr->mode == TAF_MNGD_AUDIO_LOCAL_PLAYBACK),
                    NULL, "Stream supported with the active voice call route or local playback");
    }

    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.direction = direction;
    streamConfig.interface = TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;

    return CreateStream(&streamConfig);
}

/**
 * Get the recorder interface
 */
taf_mngd_audio_StreamRef_t taf_MngdAudio::OpenRecorder
(
    taf_mngd_audio_Direction_t direction
)
{
    le_ref_IterRef_t iteratorRef;
    iteratorRef = le_ref_GetIterator(RouteRefMap);
    while (le_ref_NextNode(iteratorRef) == LE_OK)
    {
        taf_mngd_audio_Route_t* routePtr =
                (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
        if (direction == TAF_MNGD_AUDIO_RX)
            TAF_ERROR_IF_RET_VAL(routePtr && routePtr->mode != TAF_MNGD_AUDIO_VOICE_CALL,
                    NULL, "Stream supported with the active voice call route");
        if (direction == TAF_MNGD_AUDIO_TX)
            TAF_ERROR_IF_RET_VAL(routePtr && !(routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL
                    || routePtr->mode == TAF_MNGD_AUDIO_LOCAL_RECORDING),
                    NULL, "Stream supported with the active voice call route or local recording");
    }

    StreamConfig_t streamConfig;
    streamConfig.HwDevice = true;
    streamConfig.direction = direction;
    streamConfig.interface = TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;

    return CreateStream(&streamConfig);
}

taf_mngd_audio_MediaHandlerRef_t taf_MngdAudio::AddMediaHandler
(
    taf_mngd_audio_StreamRef_t streamRef,
    taf_mngd_audio_MediaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    taf_mngd_audio_Stream_t* streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    LE_INFO("AddMediaHandler %d", streamPtr->interface);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), NULL, "Invalid reference");
    if ((streamPtr->interface != TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
            && (streamPtr->interface != TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (taf_mngd_audio_MediaHandlerRef_t)AddStreamEventHandler(streamPtr,
            (le_event_HandlerFunc_t)handlerPtr, TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT,
            (void*)contextPtr);
}

StreamEventHandlerRef_t taf_MngdAudio::AddStreamEventHandler
(
    taf_mngd_audio_Stream_t* sPtr,
    le_event_HandlerFunc_t handlerPtr,
    taf_mngd_audio_StreamEventBitMask_t streamEventBitMask,
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

void taf_MngdAudio::FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc )
{
    taf_mngd_audio_StreamEvent_t* streamEventPtr = (taf_mngd_audio_StreamEvent_t*)reportPtr;
    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_event_GetContextPtr();
    LE_DEBUG("FirstLayerEventHandler");

    TAF_ERROR_IF_RET_NIL((!streamRefNodePtr) || (!streamEventPtr) || (!streamEventPtr->streamPtr),
            "Invalid reference");

    taf_mngd_audio_Stream_t* streamPtr = streamEventPtr->streamPtr;

    switch ( streamEventPtr->streamEvent )
    {
        case TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT:
            {
                taf_mngd_audio_MediaEvent_t mediaEvent = streamEventPtr->event.mediaEvent;

                LE_DEBUG("MediaEvent %d", mediaEvent);

                if (streamPtr->playFile)
                {
                    if (mediaEvent == TAF_MNGD_AUDIO_MEDIA_NO_MORE_SAMPLES)
                    {
                        mediaEvent = TAF_MNGD_AUDIO_MEDIA_ENDED;
                    }
                }

                taf_mngd_audio_MediaHandlerFunc_t clientHandlerFunc =
                        (taf_mngd_audio_MediaHandlerFunc_t)secondLayerHandlerFunc;
                LE_DEBUG("Notify client handler function");
                clientHandlerFunc(streamPtr->streamRef, mediaEvent, streamRefNodePtr->userCtx);
            }
            break;
    }
}

void taf_MngdAudio::RemoveMediaHandler
(
    taf_mngd_audio_MediaHandlerRef_t handlerRef
)
{
    RemoveStreamEventHandler( (StreamEventHandlerRef_t) handlerRef );
}

void taf_MngdAudio::RemoveStreamEventHandler
(
    StreamEventHandlerRef_t handlerRef
)
{
    EventHandlerRefNode_t* streamRefNodePtr =
            (EventHandlerRefNode_t*)le_ref_Lookup(EventHandlerRefMap, handlerRef);

    TAF_ERROR_IF_RET_NIL(streamRefNodePtr == NULL, "Invalid hnadlerRef(%p)", streamRefNodePtr);

    taf_mngd_audio_Stream_t* streamPtr = streamRefNodePtr->streamPtr;

    le_event_RemoveHandler((le_event_HandlerRef_t)streamRefNodePtr->handlerRef);

    le_ref_DeleteRef(EventHandlerRefMap, streamRefNodePtr->streamHandlerRef);

    le_dls_Remove(&streamPtr->streamRefWithEventHdlrList,
                  &streamRefNodePtr->next);

    le_mem_Release(streamRefNodePtr);
}

le_result_t taf_MngdAudio::RecordFile
(
    taf_mngd_audio_StreamRef_t streamRef , ///< Audio stream reference.
    const char                 *srcPath    ///< The file path.
)
{
    LE_INFO("RecordFile srcPath %s", srcPath);
    taf_mngd_audio_Stream_t* streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    le_result_t res = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL(streamPtr == NULL, LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
            LE_BAD_PARAMETER, "Invalid stream reference");

    TAF_ERROR_IF_RET_VAL(mIsRecording, LE_BUSY, "Another file recording is in progress");

    if (!mIsCaptureStreamCreated) {
        StreamConfig config = {};
        config.type = StreamType::CAPTURE;
        config.slotId = DEFAULT_SLOT_ID;
        config.format = AudioFormat::PCM_16BIT_SIGNED;
        mFileFormat = config.format;
        config.sampleRate = DEFAULT_SAMPLERATE;
        config.channelTypeMask = ChannelType::LEFT | ChannelType::RIGHT;

        // Set the config device type based on output device
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_mngd_audio_Connector_t const * currentconnPtr;
        taf_mngd_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                    LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

            strmItr = (le_hashmap_It_Ref_t)
                    le_hashmap_GetIterator(currentconnPtr->audioInList);
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                        LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

                if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_0){
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_0);
                    LE_DEBUG("set config with device type mic0");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_1) {
                    config.deviceTypes
                            .emplace_back((DeviceType)DEVICE_TYPE_SOURCE_1);
                    LE_DEBUG("set config with device type mic1");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_MIC_2) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SOURCE_2);
                    LE_DEBUG("set config with device type mic2");
                }
            }
        }
        res = StartAudio(config);
        TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT, "Config failed");
        mIsCaptureStreamCreated = true;
    }

    if ( srcPath != NULL )
    {
        mFile = fopen(srcPath, "w");
        if(mFile == NULL) {
            LE_ERROR("Unable to write to file");
            return LE_FAULT;
        }
        if(mFile) {
            fseek(mFile, 0, SEEK_SET);
            if(setWavHeader(mFile, streamPtr) == LE_OK)
                le_thread_Start(le_thread_Create("RecordThread", Record, streamPtr));
            else
                return LE_FAULT;
        } else {
            LE_ERROR("Unable to write to file");
            return LE_FAULT;
        }
    }
    else
    {
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_MngdAudio::setWavHeader
(
    FILE* mFile,
    taf_mngd_audio_Stream_t *config
)
{
    WavHeader_t hdr;

    memset(&hdr, 0, sizeof(WavHeader_t));
    hdr.riffId = ID_RIFF;
    hdr.riffFmt = ID_WAVE;
    hdr.chunkId = ID_FMT;
    hdr.chunkSize = DEFAULT_BITSPERSAMPLE;
    hdr.formatTag = FORMAT_PCM;
    hdr.channelsCount = 2;
    hdr.sampleRate = DEFAULT_SAMPLERATE;
    hdr.bitsPerSample = DEFAULT_BITSPERSAMPLE;
    hdr.byteRate = (  hdr.sampleRate *
                      hdr.channelsCount *
                      hdr.bitsPerSample  ) / 8;;
    hdr.blockAlign = ( hdr.bitsPerSample * hdr.channelsCount) / 8;
    hdr.chunkDataId = ID_DATA;
    hdr.chunkDataSize = 0;
    hdr.riffSize = hdr.chunkDataSize + 44 - 8;
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

/**
 * Record the file
 */
void* taf_MngdAudio::Record( void* ctxPtr) {

    auto &mngdAudio = taf_MngdAudio::GetInstance();
    uint32_t size = 0;

    taf_mngd_audio_Stream_t* streamPtr = (taf_mngd_audio_Stream_t*)ctxPtr;

    mngdAudio.mIsRecording = false;
    if(mngdAudio.mAudioCaptureStream) {
        while(!mngdAudio.mFreeBuffers.empty()) {
            mngdAudio.mFreeBuffers.pop();
        }

        for(int i = 0; i < TOTAL_BUFFERS; i++) {
            mngdAudio.mStreamBuffer = mngdAudio.mAudioCaptureStream->getStreamBuffer();

            if(mngdAudio.mStreamBuffer != nullptr) {
                size = mngdAudio.mStreamBuffer->getMinSize();
                if(size == 0) {
                    size =  mngdAudio.mStreamBuffer->getMaxSize();
                }
                mngdAudio.mStreamBuffer->setDataSize(size);
                mngdAudio.mFreeBuffers.push(mngdAudio.mStreamBuffer);
            } else {
                LE_DEBUG( "Failed to get Stream Buffer ");
                fclose(mngdAudio.mFile);
                mngdAudio.mFile = NULL;
                streamPtr->fd = -1;
                return NULL;
            }
        }

        mngdAudio.mIsRecording = true;
        mngdAudio.mEmptyPipeline = true;
        mngdAudio.mBufferRecordedTillNow = 0;

        LE_INFO( "Audio recording started" );
        while (mngdAudio.mIsRecording)
        {
            if(!mngdAudio.mFreeBuffers.empty()) {
                mngdAudio.mStreamBuffer = mngdAudio.mFreeBuffers.front();
                mngdAudio.mFreeBuffers.pop();
                telux::common::Status status =
                        mngdAudio.mAudioCaptureStream->read(mngdAudio.mStreamBuffer, size,
                        &taf_MngdAudio::ReadCallback);
                if(status != telux::common::Status::SUCCESS) {
                    LE_ERROR("read() failed with error %d",int(status));
                }
            } else {
                le_sem_Wait(mngdAudio.mSemRef);
            }
        }
        int waitTime = (8*(mngdAudio.mStreamBuffer->getMaxSize())*1000)/
                            (DEFAULT_SAMPLERATE*2*DEFAULT_BITSPERSAMPLE);
        waitTime = waitTime + 100;
        while(mngdAudio.mFreeBuffers.size() != TOTAL_BUFFERS) {
            le_clk_Time_t timeToWait = {0, waitTime * 1000};
            le_sem_WaitWithTimeOut(mngdAudio.mSemRef, timeToWait);
        }
        mngdAudio.mFileFormat = AudioFormat::UNKNOWN;

        // Update recorded buffer size to the header after completing teh recording.
        WavHeader_t hdr;
        fseek(mngdAudio.mFile, ((uint8_t*)&hdr.chunkDataSize - (uint8_t*)&hdr), SEEK_SET);

        if (fwrite(&mngdAudio.mBufferRecordedTillNow, 1, sizeof(mngdAudio.mBufferRecordedTillNow),
                mngdAudio.mFile) != sizeof(mngdAudio.mBufferRecordedTillNow))
        {
            LE_ERROR("Cannot write size to wave header");
        }
        else{
            LE_INFO("Updated the header with chunkdatasize %d",mngdAudio.mBufferRecordedTillNow);
        }
        fseek(mngdAudio.mFile, ((uint8_t*)&hdr.riffSize - (uint8_t*)&hdr), SEEK_SET);
        uint32_t riffSize = mngdAudio.mBufferRecordedTillNow + 44 - 8;

        if (fwrite(&riffSize, 1, sizeof(riffSize), mngdAudio.mFile) != sizeof(riffSize))
        {
            LE_ERROR("Cannot write riff size to wave header");
        }
        else{
            LE_INFO("Updated the header with riffsize %d", riffSize);
        }
        fflush(mngdAudio.mFile);
        fclose(mngdAudio.mFile);
        mngdAudio.mFile= NULL;
        streamPtr->fd = -1;
        LE_INFO("File Recorded SuccessFully");

        taf_mngd_audio_StreamEvent_t streamEvent;
        streamEvent.streamPtr = streamPtr;
        streamEvent.streamEvent = TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT;
        streamEvent.event.mediaEvent = TAF_MNGD_AUDIO_MEDIA_STOPPED;
        le_event_Report(streamPtr->eventId, &streamEvent,
                sizeof(taf_mngd_audio_StreamEvent_t));
        if(mngdAudio.mIsCaptureStreamCreated) {
            mngdAudio.DeleteAudioStream(streamPtr);
        }
    }

    return NULL;
}

void taf_MngdAudio::ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error)
{
    uint32_t bytesWrittenToFile = 0;
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("read() returned with error %d",int(error));
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, size, mngdAudio.mFile);
        if(bytesWrittenToFile != size) {
            LE_ERROR("Write Size mismatch while writing to file");
        }
        mngdAudio.mBufferRecordedTillNow = mngdAudio.mBufferRecordedTillNow + size;
    }
    buffer->reset();
    mngdAudio.mFreeBuffers.push(buffer);
    le_sem_Post(mngdAudio.mSemRef);
    return;
}

/**
 * Create stream for the given playback and start Audio
 */
le_result_t taf_MngdAudio::PlayFile
(
    taf_mngd_audio_StreamRef_t  streamRef,
    const char* srcPath
)
{
    taf_mngd_audio_Stream_t* streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
            LE_BAD_PARAMETER, "Invalid stream reference");

    le_result_t res = LE_FAULT;

    TAF_ERROR_IF_RET_VAL(mIsPlaying, LE_BUSY, "Another playback is in progress");

    int AudioFileFd;
    if((AudioFileFd=open(srcPath, O_RDONLY)) == -1)
    {
        LE_ERROR("File might not exist or failed to open the file %s", srcPath);
        return LE_FAULT;
    } else
    {
        LE_INFO("Successfully opened file %s", srcPath);
        streamPtr->fd = AudioFileFd;
    }
    playerStreamPtr = streamPtr;
    res = PlayWave(streamPtr, srcPath);

    if (res != LE_OK && mFileFormat == AudioFormat::UNKNOWN)
    {
        res = PlayAmr(streamPtr, srcPath);

        if (res != LE_OK && mFileFormat == AudioFormat::UNKNOWN) {
            LE_INFO( "Use Default Config ");
            StreamConfig config = {};
            config.type = StreamType::PLAY;
            config.slotId = DEFAULT_SLOT_ID;
            config.format = AudioFormat::PCM_16BIT_SIGNED;
            config.sampleRate = DEFAULT_SAMPLERATE;
            config.channelTypeMask = ChannelType::LEFT;
            mFileFormat = config.format;

            // Set the config device type based on output device
            le_hashmap_It_Ref_t connItr =
                    (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
            taf_mngd_audio_Connector_t const * currentconnPtr;
            taf_mngd_audio_Stream_t const * outStreamPtr;
            le_hashmap_It_Ref_t strmItr;
            while (le_hashmap_NextNode(connItr)==LE_OK)
            {
                currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
                TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                        LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

                strmItr = (le_hashmap_It_Ref_t)
                        le_hashmap_GetIterator(currentconnPtr->audioOutList);
                while (le_hashmap_NextNode(strmItr)==LE_OK) {
                    outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                    TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                            LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

                    if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0){
                        config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
                        LE_DEBUG("set config with device type speaker 0");
                    }
                    else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
                        config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
                        LE_DEBUG("set config with device type speaker 1");
                    }
                    else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
                        config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
                        LE_DEBUG("set config with device type speaker 2");
                    }
                }
            }
            telux::common::ErrorCode ec;
            telux::audio::PlaybackFile pbFiles1{};
            std::vector<telux::audio::PlaybackFile> filesToPlay;

            pbFiles1.absoluteFilePath = srcPath;
            pbFiles1.repeatInfo.type = telux::audio::RepeatType::COUNT;
            pbFiles1.repeatInfo.count = 1;
            filesToPlay.push_back(pbFiles1);

            repeatedPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
            ec = mAudioPlayer->startPlayback(config, filesToPlay, repeatedPlayerStatusListener);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                LE_ERROR("failed start, err %d", static_cast<int>(ec));
                return LE_FAULT;
            }
            mIsPlaying = true;
            close(streamPtr->fd);
            res = LE_OK;
        }
    }
    if(res != LE_OK)
    {
        playerStreamPtr = nullptr;
    } else {
        auto &mngdAudio = taf_MngdAudio::GetInstance();
        mngdAudio.setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_PLAYBACK, true);
    }

    return res;
}

le_result_t taf_MngdAudio::PlayWave
(
    taf_mngd_audio_Stream_t* streamPtr,
    const char *srcPath
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

    StreamConfig config = {};
    config.type = StreamType::PLAY;
    config.sampleRate = wHdr.sampleRate;
    config.channelTypeMask = (wHdr.channelsCount == 2)
            ? (ChannelType::LEFT | ChannelType::RIGHT) : ChannelType::LEFT;
    config.format = AudioFormat::PCM_16BIT_SIGNED;

    // Set the config device type based on output device
    le_hashmap_It_Ref_t connItr =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_mngd_audio_Connector_t const * currentconnPtr;
    taf_mngd_audio_Stream_t const * outStreamPtr;
    le_hashmap_It_Ref_t strmItr;
    while (le_hashmap_NextNode(connItr)==LE_OK)
    {
        currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
        TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

        strmItr = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(currentconnPtr->audioOutList);
        while (le_hashmap_NextNode(strmItr)==LE_OK) {
            outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
            TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                    LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

            if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0){
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
                LE_DEBUG("set config with device type speaker 0");
            }
            else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
                LE_DEBUG("set config with device type speaker 1");
            }
            else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
                LE_DEBUG("set config with device type speaker 2");
            }
        }
    }
    mFileFormat = config.format;

    telux::common::ErrorCode ec;
    telux::audio::PlaybackFile pbFiles1{};
    std::vector<telux::audio::PlaybackFile> filesToPlay;

    pbFiles1.absoluteFilePath = srcPath;
    pbFiles1.repeatInfo.type = telux::audio::RepeatType::COUNT;
    pbFiles1.repeatInfo.count = 1;
    filesToPlay.push_back(pbFiles1);

    repeatedPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
    ec = mAudioPlayer->startPlayback(config, filesToPlay, repeatedPlayerStatusListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("failed start, err %d", static_cast<int>(ec));
        return LE_FAULT;
    }
    LE_INFO("Wav file detected and playing");
    mIsPlaying = true;
    close(streamPtr->fd);
    return LE_OK;
}

le_result_t taf_MngdAudio::ReadPcmHeader
(
    taf_mngd_audio_Stream_t* streamPtr,
    const char *srcPath,
    StreamConfig &config
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

    config.type = StreamType::PLAY;
    config.sampleRate = wHdr.sampleRate;
    config.channelTypeMask = (wHdr.channelsCount == 2)
            ? (ChannelType::LEFT | ChannelType::RIGHT) : ChannelType::LEFT;
    config.format = AudioFormat::PCM_16BIT_SIGNED;

    // Set the config device type based on output device
    le_hashmap_It_Ref_t connItr =
            (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_mngd_audio_Connector_t const * currentconnPtr;
    taf_mngd_audio_Stream_t const * outStreamPtr;
    le_hashmap_It_Ref_t strmItr;
    while (le_hashmap_NextNode(connItr)==LE_OK)
    {
        currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
        TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

        strmItr = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(currentconnPtr->audioOutList);
        while (le_hashmap_NextNode(strmItr)==LE_OK) {
            outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
            TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                    LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

            if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0){
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
                LE_DEBUG("set config with device type speaker 0");
            }
            else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
                LE_DEBUG("set config with device type speaker 1");
            }
            else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
                LE_DEBUG("set config with device type speaker 2");
            }
        }
    }
    mFileFormat = config.format;
    return LE_OK;
}

/**
 * Get AMR file format info and Play
 */
le_result_t taf_MngdAudio::PlayAmr
(
    taf_mngd_audio_Stream_t* streamPtr,
    const char* srcPath
)
{
    taf_mngd_audio_FileFormat_t format = TAF_MNGD_AUDIO_FILE_MAX;

    StreamConfig config = {};
    config.type = StreamType::PLAY;
    char header[10] = {0};

    if ( read(streamPtr->fd, header, 9) != 9 )
    {
        LE_WARN("AMR detection: cannot read header");
        return LE_FAULT;
    }
    if ( strncmp(header, "#!AMR", 5) == 0 )
    {
        format = TAF_MNGD_AUDIO_FILE_MAX;

        if ( strncmp(header+5, "-WB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-WB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_WB;
        }
        else if ( strncmp(header+5, "-NB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_NB;
        }
        else if ( strncmp(header+5, "\n", 1) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_NB;
            lseek(streamPtr->fd, -3, SEEK_CUR);
        }
        else
        {
            LE_ERROR("Not an AMR file");
            return LE_FAULT;
        }
        if (format == TAF_MNGD_AUDIO_FILE_AMR_WB)
        {
            config.format = AudioFormat::AMRWB;
            config.sampleRate = 16000;
        }
        else
        {
            config.format = AudioFormat::AMRNB;
            config.sampleRate = 8000;
        }
        config.channelTypeMask = 1;

        // Set the config device type based on output device
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_mngd_audio_Connector_t const * currentconnPtr;
        taf_mngd_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                    LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

            strmItr = (le_hashmap_It_Ref_t)
                    le_hashmap_GetIterator(currentconnPtr->audioOutList);
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                        LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

                if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0){
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
                    LE_DEBUG("set config with device type speaker 0");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
                    LE_DEBUG("set config with device type speaker 1");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
                    LE_DEBUG("set config with device type speaker 2");
                }
            }
        }
        mFileFormat = config.format;
        telux::common::ErrorCode ec;
        telux::audio::PlaybackFile pbFiles1{};
        std::vector<telux::audio::PlaybackFile> filesToPlay;

        pbFiles1.absoluteFilePath = srcPath;
        pbFiles1.repeatInfo.type = telux::audio::RepeatType::COUNT;
        pbFiles1.repeatInfo.count = 1;
        filesToPlay.push_back(pbFiles1);

        repeatedPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
        ec = mAudioPlayer->startPlayback(config, filesToPlay, repeatedPlayerStatusListener);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LE_ERROR("failed start, err %d", static_cast<int>(ec));
            return LE_FAULT;
        }
        mIsPlaying = true;
        close(streamPtr->fd);
        return LE_OK;
    }
    return LE_FAULT;
}

/**
 * Get AMR file format info and Play
 */

le_result_t taf_MngdAudio::ReadAmrHeader
(
    taf_mngd_audio_Stream_t* streamPtr,
    const char* srcPath,
    StreamConfig &config
)
{
    taf_mngd_audio_FileFormat_t format = TAF_MNGD_AUDIO_FILE_MAX;

    telux::audio::AmrwbpParams amrParams{};
    config.type = StreamType::PLAY;
    amrParams.bitWidth = 16;
    config.formatParams = &amrParams;
    char header[10] = {0};

    if ( read(streamPtr->fd, header, 9) != 9 )
    {
        LE_WARN("AMR detection: cannot read header");
        return LE_FAULT;
    }
    if ( strncmp(header, "#!AMR", 5) == 0 )
    {
        format = TAF_MNGD_AUDIO_FILE_MAX;

        if ( strncmp(header+5, "-WB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-WB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_WB;
        }
        else if ( strncmp(header+5, "-NB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_NB;
        }
        else if ( strncmp(header+5, "\n", 1) == 0 )
        {
            LE_DEBUG("AMR-NB Detected");
            format = TAF_MNGD_AUDIO_FILE_AMR_NB;
            lseek(streamPtr->fd, -3, SEEK_CUR);
        }
        else
        {
            LE_ERROR("Not an AMR file");
            return LE_FAULT;
        }
        LE_INFO("****AMR detected format is %d", format);
        if (format == TAF_MNGD_AUDIO_FILE_AMR_WB)
        {
            config.format = AudioFormat::AMRWB;
            config.sampleRate = 16000;
        }
        else
        {
            config.format = AudioFormat::AMRNB;
            config.sampleRate = 8000;
        }

        config.channelTypeMask = 1;

        // Set the config device type based on output device
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_mngd_audio_Connector_t const * currentconnPtr;
        taf_mngd_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_mngd_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            TAF_ERROR_IF_RET_VAL( currentconnPtr == NULL,
                    LE_BAD_PARAMETER,"currentconnPtr is nullptr!");

            strmItr = (le_hashmap_It_Ref_t)
                    le_hashmap_GetIterator(currentconnPtr->audioOutList);
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_mngd_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                TAF_ERROR_IF_RET_VAL( outStreamPtr == NULL,
                        LE_BAD_PARAMETER,"outStreamPtr is nullptr!");

                if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0){
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_0);
                    LE_DEBUG("set config with device type speaker 0");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_1);
                    LE_DEBUG("set config with device type speaker 1");
                }
                else if (outStreamPtr->interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_SINK_2);
                    LE_DEBUG("set config with device type speaker 2");
                }
            }
        }
        mFileFormat = config.format;
    }

    return LE_OK;
}

/**
 * Read header info and detect file format
 */
ssize_t taf_MngdAudio::ReadHeader
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

le_result_t taf_MngdAudio::Stop(taf_mngd_audio_StreamRef_t streamRef)
{
    taf_mngd_audio_Stream_t* streamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, streamRef);

    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid stream reference");

    le_result_t res = StopAudio(streamPtr);
    TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT,"Stop failed");
    return LE_OK;
}

le_result_t taf_MngdAudio::StopAudio(taf_mngd_audio_Stream_t* streamPtr)
{
    LE_DEBUG("Stop audio stream %d\n", streamPtr->interface);
    resetCallbackPromise();
    auto status = Status::FAILED;
    if(streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX)
    {
        if (mAudioVoiceStream && mVoiceEnabled1) {
            status = mAudioVoiceStream->stopAudio(StopAudioCallback);
        }

        if (status == Status::SUCCESS) {
            LE_DEBUG("Stop voice call audio successful");
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to Stop stream failed error: %d", int (error));
                return LE_FAULT;
            }
        }
    }
    if(streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) {
        if (mAudioCaptureStream && mIsRecording) {
            LE_DEBUG("Stop Recording");
            mIsRecording = false;
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to Stop stream failed error: %d", int (error));
                return LE_FAULT;
            }
            setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_RECORDING, false);
        }
    }
    if(streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) {
        LE_DEBUG("mIsPlaying is %s", mIsPlaying ? "true" : "false");
        if (mIsPlaying) {
            LE_INFO("Stop Playing");
            telux::common::ErrorCode ec;
            ec = mAudioPlayer->stopPlayback();
            if (ec != telux::common::ErrorCode::SUCCESS) {
                if (ec == telux::common::ErrorCode::INVALID_STATE) {
                    LE_ERROR("no playback in progress");
                    return LE_FAULT;
                }
                LE_ERROR("failed stoping playback, err %d",static_cast<int>(ec));
                return LE_FAULT;
            }
            setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_PLAYBACK, false);
            mIsPlaying = false;
            mFileFormat = AudioFormat::UNKNOWN;
        }
        else
        {
            LE_ERROR("Playback is not in progress");
            return LE_FAULT;
        }
    }
    voiceStreamConfig = {};
    if (status == Status::SUCCESS) {
        LE_DEBUG("Stop successful");
    }

    return LE_OK;
}

le_result_t taf_MngdAudio::DeleteAudioStream(taf_mngd_audio_Stream_t* streamPtr)
{
    LE_DEBUG("Delete stream %d\n", streamPtr->interface);
    auto status = Status::FAILED;

    if(mAudioVoiceStream && (streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX
            || streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
    {
        resetCallbackPromise();
        status = mAudioManager->deleteStream(mAudioVoiceStream, DeleteVoiceCallback);
        if (status == Status::SUCCESS) {
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to delete voice stream failed error: %d", int (error));
                return LE_FAULT;
            }
        } else {
            LE_ERROR("Error in disabling voice audio ");
            return LE_FAULT;
        }
    }

    if (streamPtr->interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE && mAudioCaptureStream) {
        mIsCaptureStreamCreated = false;
        status = mAudioManager->deleteStream(mAudioCaptureStream, DeleteCaptureCallback);
    }
    if(status == Status::SUCCESS)
        LE_INFO("status is success");
    return LE_OK;
}

void taf_MngdAudio::DeleteCaptureCallback(ErrorCode error) {
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("DeleteCaptureCallback() succeeded");
        mngdAudio.mAudioCaptureStream.reset();
        mngdAudio.mAudioCaptureStream = nullptr;
    } else {
        LE_DEBUG("Delete CaptureStream error: %d", int (error));
    }
    mngdAudio.gCallbackPromise.set_value(error);
    return;
}

le_result_t taf_MngdAudio::setVhalRouteStatus(taf_mngd_audio_Mode_t mode, bool status)
{
    auto &audioVhal = taf_MngdAudioVhal::GetInstance();
    if(audioVhal.isAudioDrvAvailable())
    {
        le_ref_IterRef_t iteratorRef;
        iteratorRef = le_ref_GetIterator(RouteRefMap);
        while (le_ref_NextNode(iteratorRef) == LE_OK)
        {
            taf_mngd_audio_Route_t* routePtr =
                    (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
            if ( routePtr && routePtr->mode == mode )
            {
                LE_INFO("set VHAL status mode : %d status : %s", mode, status ? "true" : "false");
                return audioVhal.OpenRoute(status, routePtr->routeId, routePtr->mode);
            }
        }
    }
    return LE_FAULT;
}

void tafPromptsStatusListener::onPlaybackStarted() {
    LE_INFO("onPlaybackStarted");
}

void tafPromptsStatusListener::onPlaybackStopped() {
    LE_INFO("onPlaybackStopped");
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.mIsPlaying = false;
    mngdAudio.mFileFormat = AudioFormat::UNKNOWN;

    // Report STOP event to client
    taf_mngd_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = mngdAudio.playerStreamPtr;
    streamEvent.streamEvent = TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_MNGD_AUDIO_MEDIA_STOPPED;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_mngd_audio_StreamEvent_t));
    taf_PlaybackList_t* playListPtr = (taf_PlaybackList_t*)le_ref_Lookup(
            mngdAudio.PlaybackListRefMap, mngdAudio.currPlayListRef);
    TAF_ERROR_IF_RET_NIL( playListPtr == NULL, "playListPtr is nullptr!");
    playListPtr->isPlaybackInProgress = false;
    mngdAudio.currPlayListRef = NULL;
}

void tafPromptsStatusListener::onError(telux::common::ErrorCode error, std::string file) {
    LE_ERROR("onError : Error encounter while playing the file %s", file.c_str());
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.mIsPlaying = false;
    mngdAudio.mFileFormat = AudioFormat::UNKNOWN;

    // Report ERROR event to client
    taf_mngd_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = mngdAudio.playerStreamPtr;
    streamEvent.streamEvent = TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_MNGD_AUDIO_MEDIA_ERROR;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_mngd_audio_StreamEvent_t));
    mngdAudio.setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_PLAYBACK, false);
    taf_PlaybackList_t* playListPtr = (taf_PlaybackList_t*)le_ref_Lookup(
            mngdAudio.PlaybackListRefMap, mngdAudio.currPlayListRef);
    TAF_ERROR_IF_RET_NIL( playListPtr == NULL, "playListPtr is nullptr!");
    playListPtr->isPlaybackInProgress = false;
    mngdAudio.currPlayListRef = NULL;
}

void tafPromptsStatusListener::onFilePlayed(std::string file) {
    LE_DEBUG("onFilePlayed : %s", file.c_str());
}

void tafPromptsStatusListener::onPlaybackFinished() {
    LE_DEBUG("onPlaybackFinished");
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.mIsPlaying = false;
    mngdAudio.mFileFormat = AudioFormat::UNKNOWN;

    // Report Finished event to client
    taf_mngd_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = mngdAudio.playerStreamPtr;
    streamEvent.streamEvent = TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = TAF_MNGD_AUDIO_MEDIA_ENDED;
    le_event_Report(streamEvent.streamPtr->eventId, &streamEvent,
            sizeof(taf_mngd_audio_StreamEvent_t));
    mngdAudio.setVhalRouteStatus(TAF_MNGD_AUDIO_LOCAL_PLAYBACK, false);
    taf_PlaybackList_t* playListPtr = (taf_PlaybackList_t*)le_ref_Lookup(
            mngdAudio.PlaybackListRefMap, mngdAudio.currPlayListRef);
    TAF_ERROR_IF_RET_NIL( playListPtr == NULL, "playListPtr is nullptr!");
    playListPtr->isPlaybackInProgress = false;
    mngdAudio.currPlayListRef = NULL;
}

taf_mngd_audio_PlayListRef_t taf_MngdAudio::CreatePlayList
(
)
{
    taf_PlaybackList_t* playListptr = NULL;

    playListptr = (taf_PlaybackList_t*)le_mem_ForceAlloc(PlaybackListPool);
    TAF_ERROR_IF_RET_VAL( playListptr == NULL, NULL, "playListptr is nullptr!");

    playListptr->playListRef = (taf_mngd_audio_PlayListRef_t)le_ref_CreateRef(PlaybackListRefMap,
            playListptr);
    LE_DEBUG("Create playListRef %p", playListptr->playListRef);

    playListptr->sessionRef = taf_mngd_audio_GetClientSessionRef();

    return playListptr->playListRef;
}

le_result_t taf_MngdAudio::AddPlayListEntry
(
    taf_mngd_audio_PlayListRef_t playListRef, const char * scrPath, int32_t repeat
)
{
    taf_PlaybackList_t* playListPtr = (taf_PlaybackList_t*)le_ref_Lookup(PlaybackListRefMap,
            playListRef);
    TAF_ERROR_IF_RET_VAL( playListPtr == NULL, LE_BAD_PARAMETER, "playListptr is invalid!");

    //Check if playback is in progress.
    TAF_ERROR_IF_RET_VAL(playListPtr->isPlaybackInProgress, LE_BUSY, "Playback is in progress");

    if(playListPtr->numOfFilesToPlay == MAX_NUM_OF_PLAYBACK_FILES) {
        LE_ERROR("Maximum number of files added to the Playlist");
        return LE_FAULT;
    }

    taf_PlaybackFile_t playbackFile{};
    playbackFile.absoluteFilePath = scrPath;
    playbackFile.repeat = repeat;

    playListPtr->filesToPlay[playListPtr->numOfFilesToPlay] = playbackFile;
    playListPtr->numOfFilesToPlay++;

    return LE_OK;
}

le_result_t taf_MngdAudio::DeletePlayList
(
    taf_mngd_audio_PlayListRef_t playListRef
)
{
    taf_PlaybackList_t* playListptr = (taf_PlaybackList_t*)le_ref_Lookup(PlaybackListRefMap,
            playListRef);
    TAF_ERROR_IF_RET_VAL( playListptr == NULL, LE_BAD_PARAMETER, "playListptr is invalid!");

    //Check if playback is in progress.
    TAF_ERROR_IF_RET_VAL(playListptr->isPlaybackInProgress, LE_BUSY, "Playback is in progress");

    le_ref_DeleteRef(PlaybackListRefMap, playListptr->playListRef);
    le_mem_Release(playListptr);

    return LE_OK;
}

le_result_t taf_MngdAudio::PlayFileList
(
 taf_mngd_audio_StreamRef_t streamRef, taf_mngd_audio_PlayListRef_t playListRef
)
{
    taf_mngd_audio_Stream_t* streamPtr = (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap,
            streamRef);
    TAF_ERROR_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    TAF_ERROR_IF_RET_VAL(streamPtr->interface != TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
            LE_BAD_PARAMETER, "Invalid stream reference");

    le_result_t res = LE_FAULT;

    TAF_ERROR_IF_RET_VAL(mIsPlaying, LE_BUSY, "Another playback is in progress");

    taf_PlaybackList_t* playListptr = (taf_PlaybackList_t*)le_ref_Lookup(PlaybackListRefMap,
            playListRef);
    TAF_ERROR_IF_RET_VAL( playListptr == NULL, LE_BAD_PARAMETER, "playListptr is invalid!");

    char const* srcPath = playListptr->filesToPlay[0].absoluteFilePath.c_str();

    int AudioFileFd;
    if((AudioFileFd=open(srcPath, O_RDONLY)) == -1)
    {
        LE_ERROR("File might not exist or failed to open the file %s", srcPath);
        return LE_FAULT;
    } else
    {
        LE_INFO("Successfully opened file %s", srcPath);
        streamPtr->fd = AudioFileFd;
    }

    playerStreamPtr = streamPtr;
    StreamConfig config = {};

    res = ReadPcmHeader(streamPtr, srcPath, config);

    if (res != LE_OK && mFileFormat == AudioFormat::UNKNOWN)
    {
        res = ReadAmrHeader(streamPtr, srcPath, config);

        if (res != LE_OK && mFileFormat == AudioFormat::UNKNOWN) {
            LE_ERROR( " Unknown audio format");
            playerStreamPtr = nullptr;
            return LE_FAULT;
        }
    }

    telux::common::ErrorCode ec;
    std::vector<telux::audio::PlaybackFile> filesToPlay;

    for(uint32_t i = 0; i<playListptr->numOfFilesToPlay; i++) {
        telux::audio::PlaybackFile pbFile{};
        pbFile.absoluteFilePath = playListptr->filesToPlay[i].absoluteFilePath;

        if(playListptr->filesToPlay[i].repeat == -1){
            pbFile.repeatInfo.type = telux::audio::RepeatType::INDEFINITELY;
        } else{
            pbFile.repeatInfo.type = telux::audio::RepeatType::COUNT;
            pbFile.repeatInfo.count = playListptr->filesToPlay[i].repeat + 1;
        }

        filesToPlay.push_back(pbFile);
    }

    repeatedPlayerStatusListener = std::make_shared<tafPromptsStatusListener>();
    ec = mAudioPlayer->startPlayback(config, filesToPlay, repeatedPlayerStatusListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("failed start, err %d", static_cast<int>(ec));
        return LE_FAULT;
    }

    LE_INFO(" Repeat playback started");
    mIsPlaying = true;
    currPlayListRef = playListptr->playListRef;
    playListptr->isPlaybackInProgress = true;
    close(streamPtr->fd);

    return LE_OK;
}
