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

    le_ref_IterRef_t iteratorRef;
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
    bool isSessionMatched = false;
    taf_SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* lPtr;

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

    le_hashmap_It_Ref_t Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    taf_mngd_audio_Stream_t* currentStreamPtr;
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr=(taf_mngd_audio_Stream_t*)le_hashmap_GetValue(Iterator);

        if(currentStreamPtr != nullptr)
        {
            //StopandDelete(currentStreamPtr, connPtr->audioOutList);
        }
    }
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
    LE_INFO("DisconnectConnectors streamPtr.%p", streamPtr);

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

    TAF_ERROR_IF_RET_VAL(routeId >= TAF_MNGD_AUDIO_ROUTE_3, NULL, "Not supported or invalid Route ID!");

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
            LE_ERROR("Another route is already active");
            return NULL;
        }
    }

    if (mode == TAF_MNGD_AUDIO_LOCAL_PLAYBACK || mode == TAF_MNGD_AUDIO_LOCAL_RECORDING
            || mode == TAF_MNGD_AUDIO_LOCAL_LOOPBACK)
    {
        LE_INFO("Not supported yet!");
        return NULL;
    }

    routePtr = (taf_mngd_audio_Route_t*)le_mem_ForceAlloc(RoutePool);
    routePtr->routeId = routeId;
    routePtr->mode = mode;
    routePtr->sessionRef = taf_mngd_audio_GetClientSessionRef();

    if(mode == TAF_MNGD_AUDIO_VOICE_CALL || mode == TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN)
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
    }

    routePtr->routeRef = (taf_mngd_audio_RouteRef_t)le_ref_CreateRef(RouteRefMap, routePtr);

    return routePtr->routeRef;
}

le_result_t taf_MngdAudio::CloseRoute( taf_mngd_audio_RouteRef_t routeRef )
{
    LE_INFO("CloseRoute %p", routeRef);

    taf_mngd_audio_Route_t*  routePtr =
            (taf_mngd_audio_Route_t*)le_ref_Lookup(RouteRefMap, routeRef);
    TAF_ERROR_IF_RET_VAL(routePtr == NULL, LE_BAD_PARAMETER, "Invalid Route ref!");

    taf_mngd_audio_Stream_t*  sinkStreamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sinkRef);

    taf_mngd_audio_Stream_t*  sourceStreamPtr =
            (taf_mngd_audio_Stream_t*)le_ref_Lookup(StreamRefMap, routePtr->sourceRef);

    TAF_ERROR_IF_RET_VAL( sinkStreamPtr == NULL || sourceStreamPtr == NULL, LE_FAULT,
            "sinkRef or sourceRef is invalid!");

    ReleaseStream(sinkStreamPtr, taf_mngd_audio_GetClientSessionRef(), false);
    ReleaseStream(sourceStreamPtr, taf_mngd_audio_GetClientSessionRef(), false);

    resetCallbackPromise();
    auto status = Status::FAILED;

    if (routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL
            || routePtr->mode == TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN) {
        if (mAudioVoiceStream && mVoiceEnabled1) {
            status = mAudioVoiceStream->stopAudio(StopAudioCallback);
        }

        if (status == Status::SUCCESS) {
            LE_DEBUG("Stop voice call successful");
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to Stop stream failed error: %d", int (error));
                return LE_FAULT;
            }
            auto &audioVhal = taf_MngdAudioVhal::GetInstance();
            if(audioVhal.isAudioDrvAvailable())
            {
                LE_INFO("CloseRoute for %p routeRef", routeRef);
                audioVhal.OpenRoute(false, routePtr->routeId, routePtr->mode);
            }
            resetCallbackPromise();
            auto status = Status::FAILED;
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
    }
    LE_DEBUG("Release routeRef %p", routeRef);
    le_ref_DeleteRef(RouteRefMap, routeRef);
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

            if (streamPtr->echoCancellerEnabled) {
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
            auto &audioVhal = taf_MngdAudioVhal::GetInstance();
            if(audioVhal.isAudioDrvAvailable())
            {
                le_ref_IterRef_t iteratorRef;
                iteratorRef = le_ref_GetIterator(RouteRefMap);
                while (le_ref_NextNode(iteratorRef) == LE_OK)
                {
                    taf_mngd_audio_Route_t* routePtr =
                            (taf_mngd_audio_Route_t*) le_ref_GetValue(iteratorRef);
                    TAF_ERROR_IF_RET_VAL(routePtr == NULL, LE_FAULT, "Invalid routePtr!");
                    LE_INFO("OPenRoute for %p routeRef", iteratorRef);
                    le_result_t res =  audioVhal.OpenRoute(true, routePtr->routeId,
                            routePtr->mode);
                    if( res == LE_OK )
                    {
                        LE_INFO("Successfully set the control audio status in the VHAL");
                    }
                    else
                    {
                        LE_INFO("Failed to set the control audio status in the VHAL");
                    }
                    break;
                }
            }
        } else {
            LE_ERROR("Request to start voice stream failed.\n");
            return LE_FAULT;
        }
    }
    return LE_OK;
}