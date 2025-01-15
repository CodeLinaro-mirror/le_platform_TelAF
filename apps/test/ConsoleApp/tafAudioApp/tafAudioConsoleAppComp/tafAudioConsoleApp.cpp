/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <iostream>
#include <string>
#include <limits>

#include "legato.h"
#include "interfaces.h"

#define MAX_NUMBER_OF_INPUT     10
#define MAX_LEN_OF_EACH_INPUT   50
#define IS_CIN_FAILURE                                                     \
        if(cin.fail()){                                                    \
            cout << "InValidInput" << endl;                                \
            cin.clear();                                                   \
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); \
            continue;                                                      \
        }

using namespace std;

const char* arg1;
const char* arg2;
const char* arg3;
const char* arg4;

static le_sem_Ref_t tafAudioAppSem;
le_clk_Time_t Timeout = { 3 , 0 };
static taf_audio_MediaHandlerRef_t playerHandlerRef = NULL, recorderHandlerRef = NULL,
        txPlayerHandlerRef = NULL, rxRecorderHandlerRef = NULL;
static taf_audio_DtmfDetectorHandlerRef_t dtmfDetectHandlerRef = NULL;
taf_audio_StreamRef_t sinkRef = NULL, recorderRef = NULL, playerRef = NULL,  txPlayerRef = NULL,
        sourceRef = NULL, rxStreamRef = NULL, txStreamRef = NULL, rxRecorderRef = NULL;
taf_audio_RouteRef_t routeRef = NULL;
taf_audio_ConnectorRef_t rxConn = NULL, txConn = NULL, playerConnRef = NULL, connRef = NULL,
        txPlayerConnRef = NULL, rxConnRef = NULL;
taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef;
le_result_t res;
static le_thread_Ref_t Player_thread_ref, Recorder_thread_ref, node_thread_ref,
        Dtmf_detect_thread_ref;
taf_audio_RouteId_t routeId = (taf_audio_RouteId_t)-1;
bool isVoiceActive = false, isPbActive = false, isRpbActive = false, isRecordingActive = false,
        isTxPbActive = false, isTxRpbActive = false, isRxRecActive = false;
bool isVoiceStreamCreated = false, isPbStreamCreated = false, isRecordStreamCreated = false,
        isRpbStreamCreated = false, isLbStreamCreated = false, isDtmfRegistered = false,
        isDtmfToneStarted = false,  isTxPbStreamCreated = false, isTxRpbStreamCreated = false,
        isRxRecordStreamCreated = false, isDtmfToneStartedTx = false;

static void MyDtmfDetectorHandler
(
    taf_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler detects %c", dtmf);
    std::cout << "Dtmf tone detected for " << dtmf << std::endl;
}

static void MyMediaEventHandler
(
    taf_audio_StreamRef_t          streamRef,
    taf_audio_MediaEvent_t         event,
    void*                          contextPtr
)
{
    switch(event)
    {
        case TAF_AUDIO_MEDIA_ENDED:
            LE_INFO(" Playback completed");
            cout<<"****Playback completed***"<<endl;
            isPbActive = false;
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_STOPPED:
            LE_INFO(" Playback/capture stopped");
            if (streamRef == playerRef)
                cout<<"****Playback stopped***"<<endl;
            else if (streamRef == txPlayerRef)
                cout<<"****Remote playback stopped***"<<endl;
            else if (streamRef == recorderRef)
                cout<<"****Capture stopped***"<<endl;
            else if (streamRef == rxRecorderRef)
                cout<<"****Remote capture stopped***"<<endl;
            else
                LE_INFO(" Unknown stream playback/capture stopped");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
            if (streamRef == playerRef)
                cout<<"****Playback error***"<<endl;
            else if (streamRef == txPlayerRef)
                cout<<"****Remote playback error***"<<endl;
            else if (streamRef == recorderRef)
                cout<<"****Capture error***"<<endl;
            else if (streamRef == rxRecorderRef)
                cout<<"****Remote capture error***"<<endl;
            else
                LE_INFO(" Unknown stream playback/capture error");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            cout<<"****Playback no more samples***"<<endl;
            le_sem_Post(tafAudioAppSem);
            break;
        default:
            LE_INFO("File event is %d", event);
            le_sem_Post(tafAudioAppSem);
            break;
    }
    if (streamRef == playerRef) {
        isPbActive = false;
        isRpbActive = false;
    }
    else if (streamRef == txPlayerRef) {
        isTxPbActive = false;
        isTxRpbActive = false;
    }
    else if (streamRef == recorderRef)
        isRecordingActive = false;
    else if (streamRef == rxRecorderRef)
        isRxRecActive = false;
}

void* Test_taf_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audio_ConnectService();

    taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
    if (streamRef == playerRef) {
        playerHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == txPlayerRef) {
        txPlayerHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == recorderRef) {
        recorderHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == rxStreamRef) {
        dtmfDetectHandlerRef = taf_audio_AddDtmfDetectorHandler(streamRef, MyDtmfDetectorHandler,
                NULL);
    } else if (streamRef == rxRecorderRef) {
        rxRecorderHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    }
    sem = le_sem_FindSemaphore("tafAudioAppSem");
    if(sem != NULL)
    {
        le_sem_Post(sem);
    }
    else
    {
        LE_ERROR_IF((sem == NULL), "tafAudioAppSem is NULL!");
    }
    le_event_RunLoop();
    return NULL;
}

static void NodeEventCallback(uint8_t nodeId, taf_audioVendor_Event_t event, void* contextPtr)
{
    LE_INFO("NodeEventCallback for nodeId %d", nodeId);
    const char* nodeEvent = "";
    if ( event == TAF_AUDIOVENDOR_MUTE )
    {
        nodeEvent = "mute";
    } else if ( event == TAF_AUDIOVENDOR_UNMUTE)
    {
        nodeEvent = "unmute";
    } else
        nodeEvent = "undefined";
    cout<<"Node event for node "<< nodeEvent << endl;
}

void* Test_taf_audio_NodeHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audioVendor_ConnectService();
    int *input = (int*)ctxPtr;
    LE_TEST_INFO("Test taf_audioVendor_AddNodeStateChangeHandler of node %d", *input);
    handlerRef = taf_audioVendor_AddNodeStateChangeHandler(*input,
               NodeEventCallback, NULL);
    LE_TEST_OK(handlerRef != NULL, "Successfully regsiter for node event %d", *input);
    if( handlerRef != NULL )
    {
        cout<<"Successfully registered for node event"<<endl;
    }
    else
    {
        cout<<"Failed to register the node event"<<endl;
        return NULL;
    }

    sem = le_sem_FindSemaphore("tafAudioAppSem");
    if(sem != NULL)
    {
        le_sem_Post(sem);
    }
    else
    {
        LE_ERROR_IF((sem == NULL), "tafAudioAppSem is NULL!");
    }
    le_event_RunLoop();
    return NULL;
}

void Test_Audio_Playback_Stream(bool createRoute, int direction)
{
    if (createRoute) {
        LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
        routeRef = taf_audio_OpenRoute( routeId, TAF_AUDIO_LOCAL_PLAYBACK,
                &sinkRef, &sourceRef);
        LE_TEST_OK(routeRef != NULL,
                "OpenRoute successfull for LOCAL_PLAYBACK");

        if(routeRef == NULL)
        {
            cout<<"****Failed to create route for playback***"<<endl;
            LE_TEST_EXIT;
        }
    }

    if(direction == 1)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        playerConnRef = taf_audio_CreateConnector();
        LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_RX)");
        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect sinkRef and playerConnRef");
        res = taf_audio_Connect(playerConnRef, sinkRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to playerConnRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect playerRef and playerConnRef");
        res = taf_audio_Connect(playerConnRef, playerRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

        Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        isPbStreamCreated = true;
    }
    else if (direction == 2)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        txPlayerConnRef = taf_audio_CreateConnector();
        LE_TEST_OK(txPlayerConnRef != NULL, "Successfully created Connector ");

        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_TX)");
        txPlayerRef = taf_audio_OpenPlayer(TAF_AUDIO_TX);
        LE_TEST_OK(txPlayerRef != NULL, "Successfully opened the player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect txPlayerConnRef and txStreamRef");
        res = taf_audio_Connect(txPlayerConnRef, txStreamRef);
        LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txPlayerConnRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect txPlayerRef and playerConnRef");
        res = taf_audio_Connect(txPlayerConnRef, txPlayerRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected txPlayerRef to txPlayerConnRef");

        Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)txPlayerRef);
        isTxPbStreamCreated = true;
    }
    else
    {
        cout << "Invalid input";
        return;
    }

    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);
}

void Test_Audio_Loopback_Stream()
{
    LE_TEST_INFO("Test OpenRoute for LOCAL_LOOPBACK");
    routeRef = taf_audio_OpenRoute( routeId, TAF_AUDIO_LOCAL_LOOPBACK,
            NULL, NULL);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_LOOPBACK");

    if(routeRef == NULL)
    {
        cout<<"****Failed to create route for loopback***"<<endl;
        LE_TEST_EXIT;
    }
    cout<<"****Successfully started loopback***"<<endl;
    isLbStreamCreated = true;
}

void Test_Audio_Loopback_Delete()
{
    LE_TEST_INFO("Test CloseRoute for LOCAL_LOOPBACK");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOCAL_LOOPBACK route");
    if(res != LE_OK) {
        cout<<"****Failed stop loopback***"<<endl;
        return;
    }
    cout<<"****Successfully stopped loopback***"<<endl;
    isLbStreamCreated = false;
}

void Test_Audio_DTMF_Detection_Register(){
    LE_TEST_INFO("Test DTMF detection for Voice Call");
    Dtmf_detect_thread_ref = le_thread_Create("taf_audio_dtmf_thread",
            Test_taf_audio_AddHandler, (void*)rxStreamRef);
    LE_TEST_OK(Dtmf_detect_thread_ref != NULL, "Successfully regsiter for DTMF detection event ");
    if(Dtmf_detect_thread_ref != NULL )
    {
        cout<<"Successfully registered for DTMF detection event"<<endl;
        isDtmfRegistered = true;
        le_thread_Start(Dtmf_detect_thread_ref);
        le_sem_Wait(tafAudioAppSem);
    }
    else
    {
        cout<<"Failed to register the DTMF detection event"<<endl;
    }
}

void Test_Audio_DTMF_Detection_Deregister(){
    LE_TEST_INFO("Test deregister DTMF detection for Voice Call");
    taf_audio_RemoveDtmfDetectorHandler(dtmfDetectHandlerRef);
    LE_TEST_OK(true, "Successfully deregsiter for DTMF detection event ");
    isDtmfRegistered = false;
}

void Test_Audio_Play_DTMF(taf_audio_StreamRef_t streamRef, const char* dtmfPtr, uint16_t duration,
    uint32_t pause, double gain) {
    le_result_t res;
    LE_TEST_INFO("To test taf_audio_PlayDtmf on StreamRef.%p", rxStreamRef);
    res = taf_audio_PlayDtmf(streamRef, dtmfPtr, duration, pause, gain);
    LE_TEST_OK((res == LE_OK), "taf_audio_PlayDtmf - Pass");
    isDtmfToneStarted = true;
}

void Test_Audio_Play_Signalling_Dtmf(uint32_t slotId, const char* dtmfPtr, uint16_t duration,
        uint32_t pause) {
    le_result_t res;
    LE_TEST_INFO("To test taf_audio_PlaySignallingDtmf");
    res = taf_audio_PlaySignallingDtmf(slotId, dtmfPtr, duration, pause);
    LE_TEST_OK((res == LE_OK), "taf_audio_PlaySignallingDtmf - Pass");
    if((res == LE_OK)) {
        isDtmfToneStartedTx = true;
    }
}

void Test_Audio_Stop_DTMF(taf_audio_StreamRef_t streamRef) {
    le_result_t res;
    LE_TEST_INFO("To test taf_audio_StopDtmf on StreamRef.%p", streamRef);
    res = taf_audio_StopDtmf(streamRef);
    LE_TEST_OK((res == LE_OK), "taf_audio_StopDtmf - Pass");
    if((res == LE_OK)) {
        isDtmfToneStarted = false;
    }
}

void Test_Audio_Stop_Signalling_Dtmf(uint32_t slotId) {
    le_result_t res;
    LE_TEST_INFO("To test Test_Audio_Stop_Signalling_Dtmf ");
    res = taf_audio_StopSignallingDtmf(slotId);
    LE_TEST_OK((res == LE_OK), "Test_Audio_Stop_Signalling_Dtmf - Pass");
    if((res == LE_OK)) {
        isDtmfToneStartedTx = false;
    }
}

void Test_Audio_Playback_Start( taf_audio_StreamRef_t streamRef, string filePath )
{
    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(streamRef, filePath.c_str());
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    if(res != LE_OK)
    {
        cout<<"****Failed to start playback***"<<endl;
        return;
    }
    cout<<"****Successfully started playback***"<<endl;
    if(streamRef == playerRef)
        isPbActive = true;
    else if(streamRef == txPlayerRef)
        isTxPbActive = true;
}

// direction : 1 for local, 2 for remote
void Test_Audio_PlayList_Setup( bool createRoute, int direction)
{
    if (createRoute) {
        LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
        routeRef = taf_audio_OpenRoute( routeId, TAF_AUDIO_LOCAL_PLAYBACK,
                &sinkRef, &sourceRef);
        LE_TEST_OK(routeRef != NULL,
                "OpenRoute successfull for LOCAL_PLAYBACK when other route is not active");

        if(routeRef == NULL)
        {
            cout<<"****Failed to create route for playback***"<<endl;
            LE_TEST_EXIT;
        }
    }

    if(direction == 1)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        playerConnRef = taf_audio_CreateConnector();
        LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_RX)");
        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect sinkRef and playerConnRef");
        res = taf_audio_Connect(playerConnRef, sinkRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to playerConnRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect playerRef and playerConnRef");
        res = taf_audio_Connect(playerConnRef, playerRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

        Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        isRpbStreamCreated = true;
    }
    else if (direction == 2)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        txPlayerConnRef = taf_audio_CreateConnector();
        LE_TEST_OK(txPlayerConnRef != NULL, "Successfully created Connector ");

        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_TX)");
        txPlayerRef = taf_audio_OpenPlayer(TAF_AUDIO_TX);
        LE_TEST_OK(txPlayerRef != NULL, "Successfully opened the player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect txPlayerConnRef and txStreamRef");
        res = taf_audio_Connect(txPlayerConnRef, txStreamRef);
        LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txPlayerConnRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect txPlayerRef and txPlayerConnRef");
        res = taf_audio_Connect(txPlayerConnRef, txPlayerRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected txPlayerRef to txPlayerConnRef");

        Player_thread_ref = le_thread_Create("taf_audio_svc_test_txThread",
                Test_taf_audio_AddHandler, (void*)txPlayerRef);
        isTxRpbStreamCreated = true;
    }
    else
    {
        cout << "Invalid input";
        return;
    }
    le_thread_Start(Player_thread_ref);
    le_sem_Wait(tafAudioAppSem);
}

// direction : 1 for local, 2 for remote
void Test_Audio_Record_Stream(bool createRoute, int direction)
{
    if(createRoute) {
        LE_TEST_INFO("Test taf_audio_OpenRoute API with capture mode");
        routeRef = taf_audio_OpenRoute( routeId, TAF_AUDIO_LOCAL_RECORDING,
                &sinkRef, &sourceRef);
        LE_TEST_OK(routeRef != NULL,
                "OpenRoute successfull for LOCAL_RECORDING");
        if(routeRef == NULL)
        {
            cout<<"****Failed to create route for record***"<<endl;
            LE_TEST_EXIT;
        }
    }

    if (direction == 1)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        connRef = taf_audio_CreateConnector();
        LE_TEST_OK(connRef != NULL, "Successfully created Connector");

        LE_TEST_INFO("Test taf_audio_OpenRecorder(TAF_AUDIO_TX)");
        recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);
        LE_TEST_OK(recorderRef != NULL, "Successfully opened the TX recorder stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect sourceRef and connRef");
        res = taf_audio_Connect(connRef, sourceRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected sourceRef to connRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect recorderRef and connRef");
        res = taf_audio_Connect(connRef, recorderRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to connRef");

        Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)recorderRef);
        isRecordStreamCreated = true;
    }
    else if (direction == 2)
    {
        LE_TEST_INFO("Test taf_audio_CreateConnector");
        rxConnRef = taf_audio_CreateConnector();
        LE_TEST_OK(rxConnRef != NULL, "Successfully created Connector");

        LE_TEST_INFO("Test taf_audio_OpenRecorder(TAF_AUDIO_RX)");
        rxRecorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
        LE_TEST_OK(rxRecorderRef != NULL, "Successfully opened the RX recorder stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect rxStreamRef and rxConnRef");
        res = taf_audio_Connect(rxConnRef, rxStreamRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected rxStreamRef to rxConnRef");

        LE_TEST_INFO("Test taf_audio_Connect to connect rxRecorderRef and rxConnRef");
        res = taf_audio_Connect(rxConnRef, rxRecorderRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected rxRecorderRef to rxConnRef");

        Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)rxRecorderRef);
        isRxRecordStreamCreated = true;
    }
    else
    {
        cout << "Invalid input";
    }

    le_thread_Start(Recorder_thread_ref);

    le_sem_Wait(tafAudioAppSem);
}

void Test_Audio_Record_Start(taf_audio_StreamRef_t streamRef, string filePath)
{
    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(streamRef, filePath.c_str());
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    if(res != LE_OK)
    {
        cout<<"****Failed to start recording***"<<endl;
        return;
    }
    cout<<"****Successfully started recording***"<<endl;
    if (streamRef == recorderRef)
        isRecordingActive  = true;
    else if (streamRef == rxRecorderRef)
        isRxRecActive = true;
}

void Test_Audio_Playback_Stop(taf_audio_StreamRef_t streamRef)
{
    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(streamRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    if(streamRef == playerRef) {
        isPbActive = false;
        isRpbActive = false;
    }
    else if (streamRef == txPlayerRef)
    {
        isTxPbActive = false;
        isTxRpbActive = false;
    }
}

void Test_Audio_Playback_Delete(taf_audio_StreamRef_t streamRef, bool closeRoute)
{
    if(streamRef == playerRef) {
        if(isPbActive || isRpbActive)
            Test_Audio_Playback_Stop(streamRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from playerConnRef");
        taf_audio_Disconnect(playerConnRef, playerRef);
        LE_TEST_OK(true, "Successfully disconnected playerRef from playerConnRef");
        isPbStreamCreated = false;
        taf_audio_RemoveMediaHandler(playerHandlerRef);
    } else if (streamRef == txPlayerRef) {
        if(isTxPbActive || isTxRpbActive)
            Test_Audio_Playback_Stop(streamRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txPlayerRef from txPlayerConnRef");
        taf_audio_Disconnect(txPlayerConnRef, txPlayerRef);
        LE_TEST_OK(true, "Successfully disconnected txPlayerRef from txPlayerConnRef");
        isTxPbStreamCreated = false;
        taf_audio_RemoveMediaHandler(txPlayerHandlerRef);
    }

    if(closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");
    }
}

void Test_Audio_Record_Stop(taf_audio_StreamRef_t streamRef)
{
    LE_TEST_INFO("Test taf_audio_Stop recording");
    res = taf_audio_Stop(streamRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");
    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);
    if(streamRef == recorderRef)
        isRecordingActive = false;
    else if(streamRef == rxRecorderRef)
        isRxRecActive = false;
}

void Test_Audio_Record_Delete(taf_audio_StreamRef_t streamRef, bool closeRoute)
{

    if(streamRef == recorderRef) {
        if(isRecordingActive)
            Test_Audio_Record_Stop(streamRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect recorderRef from connRef");
        taf_audio_Disconnect(connRef, recorderRef);
        LE_TEST_OK(true, "Successfully disconnected recorderRef from ConnectorRef");
        isRecordStreamCreated = false;
        taf_audio_RemoveMediaHandler(recorderHandlerRef);
    } else if (streamRef == rxRecorderRef) {
        if(isRxRecActive)
            Test_Audio_Record_Stop(streamRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect rxRecorderRef from rxConnRef");
        taf_audio_Disconnect(rxConnRef, rxRecorderRef);
        LE_TEST_OK(true, "Successfully disconnected rxRecorderRef from rxConnRef");
        isRxRecordStreamCreated = false;
        taf_audio_RemoveMediaHandler(rxRecorderHandlerRef);
    }

    if (closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_RECORDING route");
    }
}

void Test_Audio_VoiceCall_Stream(bool isEcnrEnable)
{

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1");
    routeRef = taf_audio_OpenRoute( routeId, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);


    if(routeRef == NULL)
    {
        cout<<"****Failed to create route for voice call***"<<endl;
        LE_TEST_EXIT;
    }

    LE_TEST_INFO("Test taf_audio_CreateConnector");
    rxConn = taf_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", rxConn);

    LE_TEST_INFO("Test taf_audio_CreateConnector");
    txConn = taf_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", txConn);

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceRx");
    rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef != NULL, "Successfully created rxStreamRef %p", rxStreamRef);

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceTx");
    txStreamRef = taf_audio_OpenModemVoiceTx(1, isEcnrEnable);
    LE_TEST_OK(txStreamRef != NULL, "Successfully created txStreamRef %p", txStreamRef);
    isVoiceStreamCreated = true;
}

void Test_Audio_VoiceCall_Start()
{
    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and sinkRef");
    res = taf_audio_Connect(rxConn, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect txConn and sourceRef");
    res = taf_audio_Connect(txConn, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and rxStreamRef");
    res = taf_audio_Connect(rxConn, rxStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect txConn and txStreamRef");
    res = taf_audio_Connect(txConn, txStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txConn");

    if(res != LE_OK)
    {
        cout<<"****Failed to connect voice call audio***"<<endl;
        return;
    }
    cout<<"****Successfully voice call audio established***"<<endl;
    isVoiceActive = true;
}

void Test_Audio_VoiceCall_Stop()
{
    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected to txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and rxStreamRef");
    taf_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and sinkRef");
    taf_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect txConn and sourceRef");
    taf_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef connected to txConn");

    isVoiceActive = false;
}

void Test_Audio_VoiceCall_Delete()
{
    if(isVoiceActive)
        Test_Audio_VoiceCall_Stop();

    if(isRecordStreamCreated)
        Test_Audio_Record_Delete(recorderRef, false);

    if(isRxRecordStreamCreated)
        Test_Audio_Record_Delete(rxRecorderRef, false);

    if(isPbStreamCreated || isRpbStreamCreated)
        Test_Audio_Playback_Delete(playerRef, false);

    if(isTxPbStreamCreated || isTxRpbStreamCreated)
        Test_Audio_Playback_Delete(txPlayerRef, false);

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");
    isVoiceStreamCreated = false;
}

void PrintHelp()
{
    cout<<"Usage details:"<<endl;
    cout<<"**************"<<endl;
    if (isVoiceStreamCreated)
    {
        cout<<"setVolume"<<endl;
        cout<<"getVolume"<<endl;
        cout<<"setMute"<<endl;
        cout<<"getMute"<<endl;
        if(!isVoiceActive)
            cout<<"start voice"<<endl;
        else {
            cout<<"stop voice"<<endl;
            cout<<"playDtmf"<<endl;
            if(isDtmfToneStarted)
                cout<<"stopDtmf"<<endl;
            cout<<"registerDtmfDetection"<<endl;
            if(isDtmfRegistered)
                cout<<"deregisterDtmfDetection"<<endl;
        }
        cout<<"Delete voice stream"<<endl;
        if(!isRecordStreamCreated)
            cout<<"Create record stream"<<endl;
        else {
            cout<<"Delete recorder stream"<<endl;
            if(!isRecordingActive)
                cout<<"start recording"<<endl;
            else
                cout<<"stop recording"<<endl;
        }
        if(!isRxRecordStreamCreated)
            cout<<"Create remote record stream"<<endl;
        else {
            cout<<"Delete remote recorder stream"<<endl;
            if(!isRxRecActive)
                cout<<"start remote recording"<<endl;
            else
                cout<<"stop remote recording"<<endl;
        }
        if(!isPbStreamCreated)
        {
            cout<<"Create pb stream"<<endl;
        }
        else
        {
            cout<<"Delete player stream"<<endl;
            if(!isPbActive)
                cout<<"start playback"<<endl;
            else
                cout<<"stop playback"<<endl;
        }
        if(!isRpbStreamCreated)
        {
            cout<<"Create repeat_pb stream"<<endl;
        }
        else
        {
            cout<<"Delete repeated_player stream"<<endl;
            if(!isRpbActive)
                cout<<"start repeat_playback"<<endl;
            else
                cout<<"stop playback"<<endl;
        }
        if(!isTxPbStreamCreated)
        {
            cout<<"Create remote pb stream"<<endl;
        }
        else
        {
            cout<<"Delete remote player stream"<<endl;
            if(!isTxPbActive)
                cout<<"start remote playback"<<endl;
            else
                cout<<"stop remote playback"<<endl;
        }
        if(!isTxRpbStreamCreated)
        {
            cout<<"Create remote repeat_pb stream"<<endl;
        }
        else
        {
            cout<<"Delete remote repeated_player stream"<<endl;
            if(!isTxRpbActive)
                cout<<"start remote repeat_playback"<<endl;
            else
                cout<<"stop remote playback"<<endl;
        }
    }
    else if(isPbStreamCreated)
    {
        cout<<"setVolume"<<endl;
        cout<<"getVolume"<<endl;
        cout<<"setMute"<<endl;
        cout<<"getMute"<<endl;
        cout<<"Delete player stream"<<endl;
        if(!isPbActive)
            cout<<"start playback"<<endl;
        else
            cout<<"stop playback"<<endl;
    }
    else if(isRpbStreamCreated)
    {
        cout<<"setVolume"<<endl;
        cout<<"getVolume"<<endl;
        cout<<"setMute"<<endl;
        cout<<"getMute"<<endl;
        cout<<"Delete repeated_player stream"<<endl;
        if(!isRpbActive)
            cout<<"start repeat_playback"<<endl;
        else
            cout<<"stop playback"<<endl;
    }
    else if(isRecordStreamCreated)
    {
        cout<<"setVolume"<<endl;
        cout<<"getVolume"<<endl;
        cout<<"setMute"<<endl;
        cout<<"getMute"<<endl;
        cout<<"Delete recorder stream"<<endl;
        if(!isRecordingActive)
            cout<<"start recording"<<endl;
        else
            cout<<"stop recording"<<endl;
    }
    else if(isLbStreamCreated)
    {
        cout<<"stop loopback"<<endl;
    }
    else
    {
        cout<<"1 - Create voice call streams"<<endl;
        cout<<"2 - Create playback streams"<<endl;
        cout<<"3 - Create repeated file playback streams"<<endl;
        cout<<"4 - Create record streams"<<endl;
        cout<<"5 - node API testing"<<endl;
        cout<<"6 - start loopback"<<endl;
        cout<<"7 - start DTMF signalling"<<endl;
        if(isDtmfToneStartedTx) {
           cout<<"8 - stop DTMF signalling"<<endl;
        }
    }
}

void Test_Mngd_Audio_Start_PlayFileList(taf_audio_StreamRef_t streamRef,
        taf_audio_PlayFileConfig_t* fileConfig, size_t listSize)
{
    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list");
    res = taf_audio_PlayFileList(streamRef, fileConfig, listSize);
    LE_TEST_OK(res == LE_OK, "Successfully started the file list playback");

    if(res != LE_OK)
    {
        cout<<"****Failed to start playback***"<<endl;
        return;
    }
    cout<<"****Successfully repeated file playback is started***"<<endl;
    if (streamRef == playerRef)
        isRpbActive = true;
    else if (streamRef == txPlayerRef)
        isTxRpbActive = true;
}

void Test_Audio_Delete_PlayList(taf_audio_StreamRef_t streamRef, bool closeRoute){

    if(streamRef == playerRef) {
        if(isPbActive || isRpbActive)
            Test_Audio_Playback_Stop(playerRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
        taf_audio_Disconnect(playerConnRef, playerRef);
        LE_TEST_OK(true, "Successfully disconnected playerRef from playerConnRef");
        isRpbStreamCreated = false;
    }
    else if (streamRef == txPlayerRef) {
        if(isTxPbActive || isTxRpbActive)
            Test_Audio_Playback_Stop(txPlayerRef);
        LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txPlayerRef from txPlayerConnRef");
        taf_audio_Disconnect(txPlayerConnRef, txPlayerRef);
        LE_TEST_OK(true, "Successfully disconnected txPlayerRef from txPlayerConnRef");
        isTxRpbStreamCreated = false;
    }

    if(closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");
    }
    if (streamRef == playerRef) {
        taf_audio_RemoveMediaHandler(playerHandlerRef);
    }
    else if (streamRef == txPlayerRef) {
        taf_audio_RemoveMediaHandler(txPlayerHandlerRef);
    }
}

void ConvertToRouteId(int routeInput)
{
    if(routeInput == 1)
    {
        routeId = TAF_AUDIO_ROUTE_1;
    } else if(routeInput == 2)
    {
        routeId = TAF_AUDIO_ROUTE_2;
    } else if(routeInput == 3)
    {
        routeId = TAF_AUDIO_ROUTE_3;
    } else if(routeInput == 4)
    {
        routeId = TAF_AUDIO_ROUTE_4;
    } else if(routeInput == 5)
    {
        routeId = TAF_AUDIO_ROUTE_5;
    } else
    {
        routeId = (taf_audio_RouteId_t)routeInput;
        cout << "Invalid Route ID" << endl;
    }
}

void Test_Audio_NodeAPI
(
    void
)
{
    int input;
    string data, dir;
    taf_audioVendor_Direction_t direction;
    char inputStr[MAX_LEN_OF_EACH_INPUT];
    char* p = NULL;
    le_result_t res = LE_FAULT;
    cout<<endl<<"Node API testing:"<<endl;
    cout<<"**************"<<endl;
    cout<<"1-setVendorConfig"<<endl;
    cout<<"2-getNodeType"<<endl;
    cout<<"3-setNodeVendorConfig"<<endl;
    cout<<"4-setPowerState"<<endl;
    cout<<"5-getPowerState"<<endl;
    cout<<"6-setMuteState"<<endl;
    cout<<"7-getMuteState"<<endl;
    cout<<"8-regsiterNodeEvent"<<endl;
    cout<<"9-setNodeGain"<<endl;
    cout<<"10-getNodeGain"<<endl;
    cout<<"Enter Input:";
    cin>>input;

    if(input == 1) {
        cout<<"Enter config path:";
        cin>>data;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        LE_TEST_INFO("Test taf_audioVendor_SendVendorConfig");
        res = taf_audioVendor_SendVendorConfig(data.c_str());
        LE_TEST_OK(res == LE_OK, "Successfully sent the vendor config");
        if ( res == LE_OK )
        {
            cout<<"Successfully sent the vendor configuration"<<endl;
        } else {
            cout<<"Failed to send the vendor configuration"<<endl;
        }
    } else if (input == 2) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        LE_TEST_INFO("Test taf_audioVendor_GetNodeType");
        taf_audioVendor_NodeType_t nodeType;
        res = taf_audioVendor_GetNodeType(input, &nodeType);
        LE_TEST_OK(res == LE_OK, "Successful GetNodeType : %d", nodeType);
        if ( nodeType == TAF_AUDIOVENDOR_AUDIO_CODEC )
        {
            cout<<"Audio device type is CODEC"<<endl;
        } else if ( nodeType == TAF_AUDIOVENDOR_AUDIO_PA )
        {
            cout<<"Audio device type is PA"<<endl;
        } else if ( nodeType == TAF_AUDIOVENDOR_AUDIO_A2B )
        {
            cout<<"Audio device type is A2B"<<endl;
        } else
        {
            cout<<"Audio device type is not defined"<<endl;
        }
    } else if (input == 3) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        cout<<"Enter config path:";
        cin>>data;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        LE_TEST_INFO("Test taf_audioVendor_SendNodeConfigure");
        res = taf_audioVendor_SendNodeVendorConfig(input, data.c_str());
        LE_TEST_OK(res == LE_OK, "Successfully sent the vendor node config");
        if ( res == LE_OK )
        {
            cout<<"Successfully sent the audio device configuration to VHAL"<<endl;
        }
        else
        {
            cout<<"Failed to send the configuration to VHAL"<<endl;
        }
    } else if (input == 4) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        cout<<"Enter power state:"<<endl;
        cout<<"1-ACTIVE"<<endl;
        cout<<"2-SUSPEND"<<endl;
        cout<<"3-POWER_OFF"<<endl;
        cin>>data;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        taf_audioVendor_NodePowerState_t state;
        if (data[0] == '1') {
            state = TAF_AUDIOVENDOR_ACTIVE;
        } else if (data[0] == '2') {
            state = TAF_AUDIOVENDOR_SUSPEND;
        } else if (data[0] == '3') {
            state = TAF_AUDIOVENDOR_POWER_OFF;
        } else {
            cout<<"Invalid input"<<endl;
            return;
        }
        LE_TEST_INFO("Set power state to %d for node %d", state, input);
        res = taf_audioVendor_SetNodePowerState(input, state);
        LE_TEST_OK(res == LE_OK, "Successfully set the power state");
        if( res == LE_OK )
        {
            cout<<"Successfully set the node power state"<<endl;
        }
        else
        {
            cout<<"Failed to set the device power state"<<endl;
        }
    } else if (input == 5) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        taf_audioVendor_NodePowerState_t state;
        LE_TEST_INFO("Get power state of node %d", input);
        res = taf_audioVendor_GetNodePowerState(input, &state);
        LE_TEST_OK(res == LE_OK, "Successfully get the power state %d", state);
        if( res == LE_OK )
        {
            const char* pwState;
            if ( state == TAF_AUDIOVENDOR_ACTIVE )
            {
                pwState = "ACTIVE";
            } else if ( state == TAF_AUDIOVENDOR_SUSPEND )
            {
                pwState = "SUSPEND";
            } else if ( state == TAF_AUDIOVENDOR_POWER_OFF )
            {
                pwState = "POWER_OFF";
            } else {
                pwState = "INVALID";
            }
            cout<<"Node power state is " << pwState <<endl;
        }
        else
        {
            cout<<"Failed to get the power state"<<endl;
        }
    } else if (input == 6) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        cout<<"Enter 1 to mute 0 to unmute:";
        cin>>data;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        LE_TEST_INFO("Set mute to %d node device data is %c ", input, data[0]);
        res = taf_audioVendor_SetNodeMuteState( input, data[0] == '1' );
        LE_TEST_OK(res == LE_OK, "Successfully set the mute status %s",
                data[0] == '1' ? "true" : "false");
        if ( res == LE_OK )
        {
            cout<<"Successfully set the mute status to the node"<<endl;
        } else {
            cout<<"Failed to set the mute status to the node"<<endl;
        }
    } else if (input == 7) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        bool isMute;
        LE_TEST_INFO("Get mute state of node %d", input);
        res = taf_audioVendor_GetNodeMuteState(input, &isMute);
                    const char* muteState;
            muteState = isMute ? "true" : "false";
        LE_TEST_OK(res == LE_OK, "Successfully get the mute status %s", muteState);
        if( res == LE_OK )
        {
            cout<<"Node mute state is " << muteState <<endl;
        }
        else
        {
            cout<<"Failed to get the mute state"<<endl;
        }
    } else if (input == 8) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        LE_TEST_INFO("Test taf_audioVendor_AddNodeStateChangeHandler of node %d", input);
        node_thread_ref = le_thread_Create("taf_audio_svc_nodetest_thread",
                Test_taf_audio_NodeHandler, (void*)&input);
        le_thread_Start(node_thread_ref);

        le_sem_Wait(tafAudioAppSem);

    } else if (input == 9) {
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        double gainPerc;
        cout<<"Enter gain range from 0.0 to 1.0: ";
        cin>>gainPerc;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        cout<<"Enter direction 0 for RX(sink), 1 for TX(source): ";
        cin>>dir;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        if(dir[0]=='0') {
            direction = TAF_AUDIOVENDOR_RX;
        } else {
            direction = TAF_AUDIOVENDOR_TX;
        }
        LE_INFO("Set gain to %d node device data is %f ", input, gainPerc);
        res = taf_audioVendor_SetNodeGain(input, direction, gainPerc);

        if ( res == LE_OK )
        {
            LE_INFO("Successfully set the gain to audio device");
            cout<<"Successfully set the gain to audio device"<<endl;
        } else {
            LE_ERROR("Failed to set the gain to audio device");
            cout<<"Failed to set the gain to audio device"<<endl;
        }
    } else if (input == 10) {
        double getGainPerc;
        cout<<"Enter node Id:";
        cin>>input;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        cout<<"Enter direction 0 for RX(sink), 1 for TX(source): ";
        cin>>dir;
        p = fgets(inputStr, sizeof(inputStr), stdin);
        if(dir[0]=='0') {
            direction = TAF_AUDIOVENDOR_RX;
        } else {
            direction = TAF_AUDIOVENDOR_TX;
        }
        LE_INFO("Get gain of node %d", input);
        res = taf_audioVendor_GetNodeGain(input, direction, &getGainPerc);

        if( res == LE_OK )
        {
            LE_INFO("Successfully got the gain of audio device %f", getGainPerc);
            cout<<"Gain of audio device is " << getGainPerc <<endl;
        }
        else
        {
            LE_ERROR("Failed to get the gain of audio device");
            cout<<"Failed to get the gain of audio device"<<endl;
        }
    }else
        cout<<"Invalid input"<<endl;
    if(p == NULL)
        LE_ERROR("Not able to get the input");
}

void StartInputMonitoring
(
    void
)
{
    char inputStr[MAX_LEN_OF_EACH_INPUT];

    LE_DEBUG("StartInputMonitoring");

    do
    {
        printf("\033[1;32mtafAudioConsoleApp> \033[0m"); // Color GREEN
        char* p = fgets(inputStr, sizeof(inputStr), stdin);
        int number;
        string fileName = "";
        string DtmfString = "";
        uint16_t duration;
        uint32_t pause;
        double   gain;

        if (p != NULL)
        {
            if (strncmp(inputStr, "h", 1) == 0 || strncmp(inputStr, "?", 1) == 0 ||
                strncmp(inputStr, "help", 4) == 0)
            {
                PrintHelp();
            }
            else if(strncmp(inputStr, "1", 1) == 0)
            {
                LE_INFO("Create voice call stream");
                cout << "Enter route ID :";
                cin >> number;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                ConvertToRouteId(number);
                cout << "Enter 1 to enable or 0 to disable ECNR on modem TX :";
                cin >> number;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_VoiceCall_Stream(number == 1 ? true : false);
            }
            else if(strncmp(inputStr, "2", 1) == 0
                    || strncmp(inputStr, "Create pb stream", 16) == 0)
            {
                LE_INFO("Create playback stream");
                if(isVoiceStreamCreated) {
                    Test_Audio_Playback_Stream(false, 1);
                }
                else {
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_Playback_Stream(true, 1);
                }
            }
            else if(strncmp(inputStr, "Create remote pb stream", 21) == 0)
            {
                Test_Audio_Playback_Stream(false, 2);
            }
            else if(strncmp(inputStr, "3", 1) == 0
                    || strncmp(inputStr, "Create repeat_pb stream", 23) == 0)
            {
                if(isVoiceStreamCreated) {
                    Test_Audio_PlayList_Setup(false, 1);
                }
                else {
                    LE_INFO("Start repeated file playback");
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_PlayList_Setup(true, 1);
                }
            }
            else if (strncmp(inputStr, "Create remote repeat_pb stream", 30) == 0)
            {
                Test_Audio_PlayList_Setup(false, 2);
            }
            else if(strncmp(inputStr, "4", 1) == 0
                    || strncmp(inputStr, "Create record stream", 20) == 0)
            {
                LE_INFO("Start recording stream");
                if(isVoiceStreamCreated) {
                    Test_Audio_Record_Stream(false, 1);
                }
                else {
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_Record_Stream(true, 1);
                }
            }
            else if(strncmp(inputStr, "Create remote record stream", 27) == 0)
            {
                Test_Audio_Record_Stream(false, 2);
            }
            else if(strncmp(inputStr, "5", 1) == 0)
            {
                LE_INFO("Start node testing");
                Test_Audio_NodeAPI();
            }
            else if (strncmp(inputStr, "setVolume", 9) == 0)
            {
                LE_INFO("setVolume arg2 is %s", arg2);
                double volLevel;
                cout << "Enter volume range from 0.0 to 1.0:";
                cin >> volLevel;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                res = LE_FAULT;
                LE_TEST_INFO("Test set volume level");
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated || isTxPbStreamCreated
                        || isTxRpbStreamCreated)
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "3 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if(number == 1)
                    {
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                    } else if (number == 2)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetVolume(playerRef, volLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetVolume(txPlayerRef, volLevel);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_SetVolume(playerRef, volLevel);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_SetVolume(txPlayerRef, volLevel);
                        }
                    } else if (number == 3)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetVolume(recorderRef, volLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetVolume(rxRecorderRef, volLevel);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_SetVolume(recorderRef, volLevel);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_SetVolume(rxRecorderRef, volLevel);
                        }
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated
                        || isTxPbStreamCreated || isTxRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetVolume(playerRef, volLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetVolume(txPlayerRef, volLevel);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_SetVolume(playerRef, volLevel);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_SetVolume(txPlayerRef, volLevel);
                        }
                    }
                    else if(number == 1)
                    {
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                    }

                }
                else if (isVoiceStreamCreated
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetVolume(recorderRef, volLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetVolume(rxRecorderRef, volLevel);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_SetVolume(recorderRef, volLevel);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_SetVolume(rxRecorderRef, volLevel);
                        }
                    }
                    else if (number == 1)
                    {
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    res = taf_audio_SetVolume(playerRef, volLevel);
                }
                else if (isRecordStreamCreated)
                {
                    res = taf_audio_SetVolume(recorderRef, volLevel);
                }
                else if (isVoiceStreamCreated)
                {
                    res = taf_audio_SetVolume(rxStreamRef, volLevel);
                }
                LE_TEST_OK(res == LE_OK, "Successfully set the volume");
                if(res == LE_OK)
                    cout<< "Successfully set the volume" << endl;
                else
                    cout<< "Failed to set the volume" << endl;
            }
            else if (strncmp(inputStr, "getVolume", 9) == 0)
            {
                double getVolLevel = 0.0;
                res = LE_FAULT;
                LE_TEST_INFO("Test getVolume");
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated || isTxPbStreamCreated
                        || isTxRpbStreamCreated)
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "3 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if(number == 1)
                    {
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                    } else if (number == 2)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetVolume(playerRef, &getVolLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetVolume(txPlayerRef, &getVolLevel);

                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_GetVolume(playerRef, &getVolLevel);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_GetVolume(txPlayerRef, &getVolLevel);
                        }
                    } else if (number == 3)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetVolume(rxRecorderRef, &getVolLevel);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_GetVolume(rxRecorderRef, &getVolLevel);
                        }
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated
                        || isTxPbStreamCreated || isTxRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetVolume(playerRef, &getVolLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetVolume(txPlayerRef, &getVolLevel);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_GetVolume(playerRef, &getVolLevel);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_GetVolume(txPlayerRef, &getVolLevel);
                        }
                    }
                    else if (number == 1)
                    {
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                    }
                }
                else if (isVoiceStreamCreated
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetVolume(rxRecorderRef, &getVolLevel);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_GetVolume(rxRecorderRef, &getVolLevel);
                        }
                    }
                    else if (number == 1)
                    {
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    res = taf_audio_GetVolume(playerRef, &getVolLevel);
                }
                else if (isRecordStreamCreated)
                {
                    res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                }
                else if (isVoiceStreamCreated)
                {
                    res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);

                }
                LE_TEST_OK(res == LE_OK, "Successfully get the volume level");
                cout << "Volume level is " << getVolLevel << endl;
            }
            else if (strncmp(inputStr, "setMute", 7) == 0)
            {
                LE_INFO("setMute arg2 is %s", arg2);
                bool isMute;
                res = LE_FAULT;
                cout << "Enter 1 to mute and 0 to unmute:";
                cin >> number;
                IS_CIN_FAILURE;
                isMute = number==1 ? true : false;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                LE_TEST_INFO("Test taf_audio_SetMute");
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated || isTxPbStreamCreated
                        || isTxRpbStreamCreated)
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - player" << endl;
                    cout << "4 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_SetMute(txStreamRef, isMute);
                    }
                    else if (number == 3)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetMute(playerRef, isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetMute(txPlayerRef, isMute);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_SetMute(playerRef, isMute);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_SetMute(txPlayerRef, isMute);
                        }
                    }
                    else if (number == 4)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetMute(recorderRef, isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetMute(rxRecorderRef, isMute);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_SetMute(recorderRef, isMute);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_SetMute(rxRecorderRef, isMute);
                        }
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated
                        || isTxPbStreamCreated || isTxRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_SetMute(txStreamRef, isMute);
                    }
                    else if (number == 3)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetMute(playerRef, isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetMute(txPlayerRef, isMute);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_SetMute(playerRef, isMute);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_SetMute(txPlayerRef, isMute);
                        }
                    }
                }
                else if(isVoiceStreamCreated && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_SetMute(txStreamRef, isMute);
                    }
                    else if (number == 3)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_SetMute(recorderRef, isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_SetMute(rxRecorderRef, isMute);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_SetMute(recorderRef, isMute);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_SetMute(rxRecorderRef, isMute);
                        }
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    res = taf_audio_SetMute(playerRef, isMute);
                }
                else if (isRecordStreamCreated)
                {
                    res = taf_audio_SetMute(recorderRef, isMute);
                }
                else if (isVoiceStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1) {
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_SetMute(txStreamRef, isMute);
                    }
                }
                LE_TEST_OK(res == LE_OK, "Successfully set the mute status");
            }
            else if (strncmp(inputStr, "getMute", 7) == 0)
            {
                bool isMute = false;
                res = LE_FAULT;
                LE_TEST_INFO("Test taf_audio_GetMute");
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated || isTxPbStreamCreated
                        || isTxRpbStreamCreated)
                        && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - player" << endl;
                    cout << "4 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                    }
                    else if (number == 3)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetMute(playerRef, &isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetMute(txPlayerRef, &isMute);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_GetMute(playerRef, &isMute);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_GetMute(txPlayerRef, &isMute);
                        }
                    }
                    else if (number == 4)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetMute(recorderRef, &isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetMute(rxRecorderRef, &isMute);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_GetMute(recorderRef, &isMute);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_GetMute(rxRecorderRef, &isMute);
                        }
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated
                        || isTxPbStreamCreated || isTxRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                    }
                    else if (number == 3)
                    {
                        if((isPbStreamCreated || isRpbStreamCreated) &&
                                (isTxPbStreamCreated || isTxRpbStreamCreated)){
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetMute(playerRef, &isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetMute(txPlayerRef, &isMute);
                            }
                        } else if (isPbStreamCreated || isRpbStreamCreated) {
                            res = taf_audio_GetMute(playerRef, &isMute);
                        } else if (isTxPbStreamCreated || isTxRpbStreamCreated) {
                            res = taf_audio_GetMute(txPlayerRef, &isMute);
                        }
                    }
                }
                else if(isVoiceStreamCreated && (isRecordStreamCreated || isRxRecordStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cout << "3 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1)
                    {
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                    }
                    else if (number == 3)
                    {
                        if (isRecordStreamCreated && isRxRecordStreamCreated) {
                            cout << "1 - Local" << endl;
                            cout << "2 - Remote" << endl;
                            cout << "Enter input:";
                            cin >> number;
                            IS_CIN_FAILURE;
                            p = fgets(inputStr, sizeof(inputStr), stdin);
                            if(number == 1)
                            {
                                res = taf_audio_GetMute(recorderRef, &isMute);
                            }
                            else if (number == 2)
                            {
                                res = taf_audio_GetMute(rxRecorderRef, &isMute);
                            }
                        } else if (isRecordStreamCreated) {
                            res = taf_audio_GetMute(recorderRef, &isMute);
                        } else if (isRxRecordStreamCreated) {
                            res = taf_audio_GetMute(rxRecorderRef, &isMute);
                        }
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    res = taf_audio_GetMute(playerRef, &isMute);
                }
                else if (isRecordStreamCreated)
                {
                    res = taf_audio_GetMute(recorderRef, &isMute);
                }
                else if (isVoiceStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1) {
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                    }
                    else if (number == 2)
                    {
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                    }
                }
                LE_TEST_OK(res == LE_OK, "Successfully got the mute status");
                cout << "Mute status is " << isMute << endl;
            }
            else if (strncmp(inputStr, "start voice", 11) == 0)
            {
                Test_Audio_VoiceCall_Start();
            }
            else if (strncmp(inputStr, "start playback", 14) == 0)
            {
                cout << "Enter file path:";
                cin >> fileName;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Playback_Start(playerRef, fileName);
            }
            else if (strncmp(inputStr, "start remote playback", 21) == 0)
            {
                cout << "Enter file path:";
                cin >> fileName;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Playback_Start(txPlayerRef, fileName);
            }
            else if (strncmp(inputStr, "start recording", 15) == 0)
            {
                cout << "Enter file path:";
                cin >> fileName;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Record_Start(recorderRef, fileName);
            }
            else if (strncmp(inputStr, "start remote recording", 22) == 0)
            {
                cout << "Enter file path:";
                cin >> fileName;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Record_Start(rxRecorderRef, fileName);
            }
            else if (strncmp(inputStr, "stop playback", 13) == 0)
            {
                    Test_Audio_Playback_Stop(playerRef);
            }
            else if (strncmp(inputStr, "stop remote playback", 20) == 0)
            {
                    Test_Audio_Playback_Stop(txPlayerRef);
            }
            else if (strncmp(inputStr, "stop recording", 14) == 0)
            {
                    Test_Audio_Record_Stop(recorderRef);
            }
            else if (strncmp(inputStr, "stop remote recording", 21) == 0)
            {
                    Test_Audio_Record_Stop(rxRecorderRef);
            }
            else if (strncmp(inputStr, "stop voice", 10) == 0)
            {
                Test_Audio_VoiceCall_Stop();
            }
            else if (strncmp(inputStr, "start remote repeat_playback", 28) == 0
                || (strncmp(inputStr, "start repeat_playback", 21) == 0))
            {
                LE_INFO("inputstring is %s", inputStr);
                int numFiles = 0;
                bool isRemotePb = false;
                if(strncmp(inputStr, "start remote repeat_playback", 28) == 0)
                    isRemotePb = true;
                cout << "Enter the number of files: ";
                cin >> numFiles;
                IS_CIN_FAILURE;
                LE_INFO("numFile is %d", numFiles);
                p = fgets(inputStr, sizeof(inputStr), stdin);
                if(numFiles <= 0 || numFiles > 8 )
                {
                    cout << "Invalid input!!" << endl;
                    continue;
                }
                taf_audio_PlayFileConfig_t playFileConfig[numFiles] = {0};
                for(int i = 0; i<numFiles;i++){
                    cout << "Enter the file source path: ";
                    p = fgets(playFileConfig[i].srcPath, sizeof(playFileConfig[i].srcPath), stdin);
                    playFileConfig[i].srcPath[strcspn(playFileConfig[i].srcPath, "\n")] = '\0';
                    cout << "Enter the 0 to play once(repeat 0 times), enter x to play x+1 time(repeat x time)";
                    cout << "Enter the repeat count: ";
                    cin >> playFileConfig[i].repeat;
                    if(cin.fail()){
                        cout << "InValidInput" << endl;
                        cin.clear();
                        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        i--;
                        continue;
                    }
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                }
                if (!isRemotePb)
                    Test_Mngd_Audio_Start_PlayFileList(playerRef, playFileConfig, numFiles);
                else
                    Test_Mngd_Audio_Start_PlayFileList(txPlayerRef, playFileConfig, numFiles);
            }
            else if (strncmp(inputStr, "Delete player stream", 20) == 0)
            {
                if(isVoiceStreamCreated)
                    Test_Audio_Playback_Delete(playerRef, false);
                else
                    Test_Audio_Playback_Delete(playerRef, true);
            }
            else if (strncmp(inputStr, "Delete remote player stream", 27) == 0)
            {
                if(isVoiceActive) // check if this is required
                    Test_Audio_Playback_Delete(txPlayerRef, false);
            }
            else if (strncmp(inputStr, "Delete repeated_player stream", 29) == 0)
            {
                if(isVoiceStreamCreated)
                    Test_Audio_Delete_PlayList(playerRef, false);
                else
                    Test_Audio_Delete_PlayList(playerRef, true);
            }
            else if (strncmp(inputStr, "Delete remote repeated_player stream", 36) == 0)
            {
                Test_Audio_Delete_PlayList(txPlayerRef, false);
            }
            else if (strncmp(inputStr, "Delete recorder stream", 22) == 0)
            {
                if(isVoiceStreamCreated)
                    Test_Audio_Record_Delete(recorderRef, false);
                else
                    Test_Audio_Record_Delete(recorderRef, true);
            }
            else if (strncmp(inputStr, "Delete remote recorder stream", 29) == 0)
            {
                Test_Audio_Record_Delete(rxRecorderRef, false);
            }
            else if (strncmp(inputStr, "Delete voice stream", 19) == 0)
            {
                Test_Audio_VoiceCall_Delete();
            } else if (strncmp(inputStr, "6", 1) == 0){
                cout << "Enter route ID:";
                cin >> number;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                ConvertToRouteId(number);
                Test_Audio_Loopback_Stream();

            } else if (strncmp(inputStr, "stop loopback", 13) == 0){
                Test_Audio_Loopback_Delete();
            } else if (strncmp(inputStr, "playDtmf", 8) == 0) {
                cout << "Enter dtmf characters: ";
                cin >> DtmfString;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter duration:";
                cin >> duration;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter pause:";
                cin >> pause;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter gain(range: 0-1):";
                cin >> gain;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Play_DTMF(rxStreamRef, DtmfString.c_str(), duration, pause,
                        gain);

            } else if (strncmp(inputStr, "stopDtmf", 8) == 0) {
                Test_Audio_Stop_DTMF(rxStreamRef);
            } else if(strncmp(inputStr, "registerDtmfDetection", 21) == 0) {
                Test_Audio_DTMF_Detection_Register();
            } else if(strncmp(inputStr, "deregisterDtmfDetection", 23) == 0) {
                Test_Audio_DTMF_Detection_Deregister();
            } else if (strncmp(inputStr, "7", 1) == 0){
                cout << "Enter dtmf characters: ";
                cin >> DtmfString;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter duration:";
                cin >> duration;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter pause:";
                cin >> pause;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                cout << "Enter slotId:";
                cin >> number;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Play_Signalling_Dtmf(number, DtmfString.c_str(), duration, pause);
            } else if (strncmp(inputStr, "8", 1) == 0) {
                cout << "Enter slotId:";
                cin >> number;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Stop_Signalling_Dtmf(number);
            }
        }
        else
        {
            return;
        }
    } while (inputStr[0] != '0' &&inputStr[0] != 'q');
    return;
}

COMPONENT_INIT
{

    tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);

    PrintHelp();
    StartInputMonitoring();

    LE_TEST_EXIT;
}
