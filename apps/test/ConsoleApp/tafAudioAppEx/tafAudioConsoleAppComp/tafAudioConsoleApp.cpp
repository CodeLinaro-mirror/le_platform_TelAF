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

#include <iostream>
#include <string>
#include <limits>

#include "legato.h"
#include "interfaces.h"

#define MAX_NUMBER_OF_INPUT     10
#define MAX_LEN_OF_EACH_INPUT     28
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
static taf_audio_MediaHandlerRef_t playerHandlerRef = NULL, recorderHandlerRef = NULL;
taf_audio_StreamRef_t sinkRef = NULL, recorderRef = NULL, playerRef = NULL;
taf_audio_StreamRef_t sourceRef = NULL, rxStreamRef = NULL, txStreamRef = NULL;
taf_audio_RouteRef_t routeRef = NULL;
taf_audio_ConnectorRef_t rxConn = NULL, txConn = NULL, playerConnRef = NULL, connRef = NULL;
taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef;
le_result_t res;
static le_thread_Ref_t Player_thread_ref, Recorder_thread_ref, node_thread_ref;
taf_audio_RouteId_t routeId = (taf_audio_RouteId_t)-1;
bool isVoiceActive = false, isPbActive = false, isRpbActive = false, isRecordingActive = false;
bool isVoiceStreamCreated = false, isPbStreamCreated = false, isRecordStreamCreated = false,
        isRpbStreamCreated = false, isLbStreamCreated = false;

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
            else if (streamRef == recorderRef)
                cout<<"****Capture stopped***"<<endl;
            else
                LE_INFO(" Unknown stream playback/capture stopped");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
            cout<<"****Playback error***"<<endl;
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
    else if (streamRef == recorderRef)
        isRecordingActive = false;
}

void* Test_taf_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audio_ConnectService();

    taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
    if (streamRef == playerRef) {
        playerHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == recorderRef) {
        recorderHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
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

void Test_Audio_Playback_Stream(bool createRoute)
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

    LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_RX)");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_audio_test_thread",
            Test_taf_audio_AddHandler, (void*)playerRef);
    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_CreateConnector");
    playerConnRef = taf_audio_CreateConnector();
    LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_audio_Connect to connect sinkRef and playerConnRef");
    res = taf_audio_Connect(playerConnRef, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Connect to connect playerRef and playerConnRef");
    res = taf_audio_Connect(playerConnRef, playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to ConnectorRef");
    isPbStreamCreated = true;
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

void Test_Audio_Playback_Start( string filePath )
{
    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(playerRef, filePath.c_str());
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    if(res != LE_OK)
    {
        cout<<"****Failed to start playback***"<<endl;
        return;
    }
    cout<<"****Successfully started playback***"<<endl;
    isPbActive = true;
}

void Test_Audio_PlayList_Setup( bool createRoute )
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

    LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_RX)");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
            Test_taf_audio_AddHandler, (void*)playerRef);
    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_CreateConnector");
    taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();
    LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_audio_Connect to connect sinkRef and playerConnRef");
    res = taf_audio_Connect(playerConnRef, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Connect to connect playerRef and playerConnRef");
    res = taf_audio_Connect(playerConnRef, playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to ConnectorRef");

    isRpbStreamCreated = true;
}

void Test_Audio_Record_Stream(bool createRoute)
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

    LE_TEST_INFO("Test taf_audio_OpenRecorder(TAF_AUDIO_TX)");
    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);
    LE_TEST_OK(recorderRef != NULL, "Successfully opened the recorder stream");

    Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
            Test_taf_audio_AddHandler, (void*)recorderRef);
    le_thread_Start(Recorder_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_CreateConnector");
    taf_audio_ConnectorRef_t connRef = taf_audio_CreateConnector();
    LE_TEST_OK(connRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_audio_Connect to connect sourceRef and connRef");
    res = taf_audio_Connect(connRef, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sourceRef to ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Connect to connect recorderRef and connRef");
    res = taf_audio_Connect(connRef, recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to ConnectorRef");
    isRecordStreamCreated = true;
}

void Test_Audio_Record_Start(string filePath)
{
    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(recorderRef, filePath.c_str());
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    if(res != LE_OK)
    {
        cout<<"****Failed to start recording***"<<endl;
        return;
    }
    cout<<"****Successfully started recording***"<<endl;
    isRecordingActive  = true;
}

void Test_Audio_Playback_Stop()
{
    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    isPbActive = false;
    isRpbActive = false;
}

void Test_Audio_Playback_Delete(bool closeRoute)
{
    if(isPbActive)
        Test_Audio_Playback_Stop();

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
    taf_audio_Disconnect(playerConnRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    if(closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");
    }
    taf_audio_RemoveMediaHandler(playerHandlerRef);
    isPbStreamCreated = false;
}

void Test_Audio_Record_Stop()
{
    LE_TEST_INFO("Test taf_audio_Stop recording");
    res = taf_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");
    isRecordingActive = false;
}

void Test_Audio_Record_Delete(bool closeRoute)
{
    if(isRecordingActive)
        Test_Audio_Record_Stop();
    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect recorderRef from connRef");
    taf_audio_Disconnect(connRef, recorderRef);
    LE_TEST_OK(true, "Successfully disconnected recorderRef from ConnectorRef");

    if (closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_RECORDING route");
    }
    taf_audio_RemoveMediaHandler(recorderHandlerRef);
    isRecordStreamCreated = false;
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

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and sinkRef");
    taf_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect txConn and sourceRef");
    taf_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and rxStreamRef");
    taf_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef connected to rxConn");
    isVoiceActive = false;
}

void Test_Audio_VoiceCall_Delete()
{
    if(isVoiceActive)
        Test_Audio_VoiceCall_Stop();

    if(isRecordStreamCreated)
        Test_Audio_Record_Delete(false);

    if(isPbStreamCreated || isRpbStreamCreated)
        Test_Audio_Playback_Delete(false);

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
        else
            cout<<"stop voice"<<endl;
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
    }
}

void Test_Mngd_Audio_Start_PlayFileList(taf_audio_PlayFileConfig_t* fileConfig, size_t listSize)
{
    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list");
    res = taf_audio_PlayFileList(playerRef, fileConfig, listSize);
    LE_TEST_OK(res == LE_OK, "Successfully started the file list playback");

    if(res != LE_OK)
    {
        cout<<"****Failed to start playback***"<<endl;
        return;
    }
    cout<<"****Successfully repeated file playback is started***"<<endl;
    isRpbActive = true;
}

void Test_Audio_Delete_PlayList(bool closeRoute){

    if(isRpbActive)
        Test_Audio_Playback_Stop();

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
    taf_audio_Disconnect(playerConnRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    if(closeRoute)
    {
        LE_TEST_INFO("Test taf_audio_CloseRoute");
        res = taf_audio_CloseRoute(routeRef);
        LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");
    }
    taf_audio_RemoveMediaHandler(playerHandlerRef);
    isRpbStreamCreated = false;
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
    string data;
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

    } else
        cout<<"Invalid input"<<endl;
    if(p == NULL)
        LE_INFO("Not able to get the input");
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
                if(isVoiceStreamCreated)
                    Test_Audio_Playback_Stream(false);
                else {
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_Playback_Stream(true);
                }
            }
            else if(strncmp(inputStr, "3", 1) == 0
                    || strncmp(inputStr, "Create repeat_pb stream", 23) == 0)
            {
                if(isVoiceStreamCreated)
                    Test_Audio_PlayList_Setup(false);
                else {
                    LE_INFO("Start repeated file playback");
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_PlayList_Setup(true);
                }
            }
            else if(strncmp(inputStr, "4", 1) == 0
                    || strncmp(inputStr, "Create record stream", 20) == 0)
            {
                LE_INFO("Start recording stream");
                if(isVoiceStreamCreated)
                    Test_Audio_Record_Stream(false);
                else {
                    cout << "Enter route ID:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    ConvertToRouteId(number);
                    Test_Audio_Record_Stream(true);
                }
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
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated) && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_SetVolume on rxStreamRef");
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to rxStreamRef");
                    } else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on playerRef");
                        res = taf_audio_SetVolume(playerRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to playerRef");
                    } else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on recorderRef");
                        res = taf_audio_SetVolume(recorderRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to recorderRef");
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on playerRef");
                        res = taf_audio_SetVolume(playerRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to playerRef");
                    }
                    else if(number == 1)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on rxStreamRef");
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to rxStreamRef");
                    }

                }
                else if (isVoiceStreamCreated && isRecordStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on recorderRef");
                        res = taf_audio_SetVolume(recorderRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to recorderRef");
                    }
                    else if (number == 1)
                    {
                        LE_TEST_INFO("Test taf_audio_SetVolume on rxStreamRef");
                        res = taf_audio_SetVolume(rxStreamRef, volLevel);
                        LE_TEST_OK(res == LE_OK, "Successfully set volume to rxStreamRef");
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    LE_TEST_INFO("Test taf_audio_SetVolume on playerRef");
                    res = taf_audio_SetVolume(playerRef, volLevel);
                    LE_TEST_OK(res == LE_OK, "Successfully set volume to playerRef");
                }
                else if (isRecordStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_SetVolume on recorderRef");
                    res = taf_audio_SetVolume(recorderRef, volLevel);
                    LE_TEST_OK(res == LE_OK, "Successfully set volume to recorderRef");
                }
                else if (isVoiceStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_SetVolume on rxStreamRef");
                    res = taf_audio_SetVolume(rxStreamRef, volLevel);
                    LE_TEST_OK(res == LE_OK, "Successfully set volume to rxStreamRef");
                }
                if(res == LE_OK)
                    cout<< "Successfully set the volume" << endl;
                else
                    cout<< "Failed to set the volume" << endl;
            }
            else if (strncmp(inputStr, "getVolume", 9) == 0)
            {
                double getVolLevel = 0.0;
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated) && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of rxStreamRef");
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of rxStreamRef %f", getVolLevel);
                    } else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of playerRef");
                        res = taf_audio_GetVolume(playerRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of player %f", getVolLevel);
                    } else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
                        res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of recoder %f", getVolLevel);
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated))
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - player" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get vol of playerRef");
                        res = taf_audio_GetVolume(playerRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of player %f", getVolLevel);
                    }
                    else if (number == 1)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of rxStreamRef");
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of rxStreamRef %f", getVolLevel);
                    }
                }
                else if (isVoiceStreamCreated && isRecordStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - recorder:" << endl;
                    cout << "Enter input:";
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
                        res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of recoder %f", getVolLevel);
                    }
                    else if (number == 1)
                    {
                        LE_TEST_INFO("Test taf_audio_GetVolume to get volume of rxStreamRef");
                        res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                        LE_TEST_OK(res == LE_OK,
                                "Successfully get the vol level of rxStreamRef %f", getVolLevel);
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    LE_TEST_INFO("Test taf_audio_GetVolume to get vol of playerRef");
                    res = taf_audio_GetVolume(playerRef, &getVolLevel);
                    LE_TEST_OK(res == LE_OK,
                            "Successfully get the vol level of player %f", getVolLevel);
                }
                else if (isRecordStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
                    res = taf_audio_GetVolume(recorderRef, &getVolLevel);
                    LE_TEST_OK(res == LE_OK,
                            "Successfully get the vol level of recorder %f", getVolLevel);
                }
                else if (isVoiceStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of rxStreamRef");
                    res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
                    LE_TEST_OK(res == LE_OK,
                            "Successfully get the vol level of rxStreamRef %f", getVolLevel);
                }
                cout << "Volume level is " << getVolLevel << endl;
            }
            else if (strncmp(inputStr, "setMute", 7) == 0)
            {
                LE_INFO("setMute arg2 is %s", arg2);
                bool isMute;
                cout << "Enter 1 to mute and 0 to unmute:";
                cin >> number;
                IS_CIN_FAILURE;
                isMute = number==1 ? true : false;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated) && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice RX");
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to rxStreamRef");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice TX");
                        res = taf_audio_SetMute(txStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to txStreamRef");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute playerRef");
                        res = taf_audio_SetMute(playerRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully playerRef is muted");
                    }
                    else if (number == 4)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute recorderRef");
                        res = taf_audio_SetMute(recorderRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully recorderRef is muted");
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated))
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
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice RX");
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to rxStreamRef");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice TX");
                        res = taf_audio_SetMute(txStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to txStreamRef");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute playerRef");
                        res = taf_audio_SetMute(playerRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully playerRef is muted");
                    }
                }
                else if(isVoiceStreamCreated && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice RX");
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to rxStreamRef");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice TX");
                        res = taf_audio_SetMute(txStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to txStreamRef");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute recorderRef");
                        res = taf_audio_SetMute(recorderRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully recorderRef is muted");
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    LE_TEST_INFO("Test taf_audio_SetMute to mute playerRef");
                    res = taf_audio_SetMute(playerRef, isMute);
                    LE_TEST_OK(res == LE_OK, "Successfully playerRef is muted");
                }
                else if (isRecordStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_SetMute to mute recorderRef");
                    res = taf_audio_SetMute(recorderRef, isMute);
                    LE_TEST_OK(res == LE_OK, "Successfully recorderRef is muted");
                }
                else if (isVoiceStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1) {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice RX");
                        res = taf_audio_SetMute(rxStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to rxStreamRef");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice TX");
                        res = taf_audio_SetMute(txStreamRef, isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully set mute status to txStreamRef");
                    }
                }
            }
            else if (strncmp(inputStr, "getMute", 7) == 0)
            {
                bool isMute = false;
                if (isVoiceStreamCreated
                        && (isPbStreamCreated || isRpbStreamCreated) && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice RX");
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice TX");
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of playerRef");
                        res = taf_audio_GetMute(playerRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status of player");
                    }
                    else if (number == 4)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of recorderRef");
                        res = taf_audio_GetMute(recorderRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status of recorder");
                    }
                }
                else if (isVoiceStreamCreated && (isPbStreamCreated || isRpbStreamCreated))
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
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice RX");
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice TX");
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of playerRef");
                        res = taf_audio_GetMute(playerRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status of player");
                    }
                }
                else if(isVoiceStreamCreated && isRecordStreamCreated)
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
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice RX");
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute to mute modem voice TX");
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get mute status of txStreamRef");
                    }
                    else if (number == 3)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of recorderRef");
                        res = taf_audio_GetMute(recorderRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status of recorder");
                    }
                }
                else if ((isPbStreamCreated || isRpbStreamCreated))
                {
                    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of playerRef");
                    res = taf_audio_GetMute(playerRef, &isMute);
                    LE_TEST_OK(res == LE_OK, "Successfully get the mute status of player");
                }
                else if (isRecordStreamCreated)
                {
                    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of recorderRef");
                    res = taf_audio_GetMute(recorderRef, &isMute);
                    LE_TEST_OK(res == LE_OK, "Successfully get the mute status of recorder");
                }
                else if (isVoiceStreamCreated)
                {
                    cout << "1 - modem RX" << endl;
                    cout << "2 - modem TX" << endl;
                    cin >> number;
                    IS_CIN_FAILURE;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
                    if (number == 1) {
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice RX");
                        res = taf_audio_GetMute(rxStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                    else if (number == 2)
                    {
                        LE_TEST_INFO("Test taf_audio_GetMute of modem voice TX");
                        res = taf_audio_GetMute(txStreamRef, &isMute);
                        LE_TEST_OK(res == LE_OK, "Successfully get the mute status");
                    }
                }
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
                Test_Audio_Playback_Start(fileName);
            }
            else if (strncmp(inputStr, "start recording", 15) == 0)
            {
                cout << "Enter file path:";
                cin >> fileName;
                IS_CIN_FAILURE;
                p = fgets(inputStr, sizeof(inputStr), stdin);
                Test_Audio_Record_Start(fileName);
            }
            else if (strncmp(inputStr, "stop playback", 13) == 0)
            {
                    Test_Audio_Playback_Stop();
            }
            else if (strncmp(inputStr, "stop recording", 14) == 0)
            {
                    Test_Audio_Record_Stop();
            }
            else if (strncmp(inputStr, "stop voice", 10) == 0)
            {
                Test_Audio_VoiceCall_Stop();
            }
            else if (strncmp(inputStr, "start repeat_playback", 21) == 0)
            {
                int numFiles = 0;
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
                    cin >> playFileConfig[i].srcPath;
                    p = fgets(inputStr, sizeof(inputStr), stdin);
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
                Test_Mngd_Audio_Start_PlayFileList(playFileConfig, numFiles);
            }
            else if (strncmp(inputStr, "Delete player stream", 20) == 0)
            {
                if(isVoiceActive)
                    Test_Audio_Playback_Delete(false);
                else
                    Test_Audio_Playback_Delete(true);
            }
            else if (strncmp(inputStr, "Delete repeated_player stream", 20) == 0)
            {
                if(isVoiceActive)
                    Test_Audio_Delete_PlayList(false);
                else
                    Test_Audio_Delete_PlayList(true);
            }
            else if (strncmp(inputStr, "Delete recorder stream", 22) == 0)
            {
                if(isVoiceActive)
                    Test_Audio_Record_Delete(false);
                else
                    Test_Audio_Record_Delete(true);
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
