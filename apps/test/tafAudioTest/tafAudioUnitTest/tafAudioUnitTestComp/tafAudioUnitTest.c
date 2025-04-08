/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

le_result_t res;
const char* wavfilePath = "/data/test.wav";
const char* amrfilePath = "/data/test.amr";
const char* recordfilePath = "/data/record.wav";
int repeat = 1;
static le_sem_Ref_t tafAudioAppSem;
static taf_audio_MediaHandlerRef_t MediaHandlerRef = NULL;
le_clk_Time_t Timeout = { 3 , 0 };
le_clk_Time_t StopTimeout = { 5 , 0 };
static le_thread_Ref_t Player_thread_ref, Recorder_thread_ref, Dtmf_detect_thread_ref;
taf_audio_StreamRef_t recorderRef = NULL, playerRef = NULL, playerRef1 = NULL, txPlayerRef = NULL,
        recorderRef1 = NULL, sinkRef = NULL, sourceRef = NULL, rxStreamRef = NULL,
        txStreamRef = NULL, sinkRef1 = NULL, sourceRef1 = NULL, rxRecorderRef = NULL;
taf_audio_RouteRef_t routeRef = NULL, routeRef1 = NULL;
taf_audio_ConnectorRef_t rxConn = NULL, txConn = NULL, connRef = NULL;
static taf_audio_DtmfDetectorHandlerRef_t dtmfDetectHandlerRef = NULL;

static void MyDtmfDetectorHandler
(
    taf_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler detects %c", dtmf);
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
            LE_TEST_OK(true," Playback completed");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_STOPPED:
            LE_TEST_OK(true," Playback/capture stopped");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_TEST_OK(true, "File event is TAF_AUDIO_MEDIA_ERROR.");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_TEST_OK(true, "File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            le_sem_Post(tafAudioAppSem);
            break;
        default:
            LE_TEST_OK(true, "File event is %d", event);
            le_sem_Post(tafAudioAppSem);
            break;
    }
}

void TEST_OPEN_ROUTE()
{
    LE_TEST_INFO("Test taf_audio_OpenRoute with ROUTE_1");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceRx with active voice call route");
    rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef != NULL,
            "Successfully created rxStreamRef with active voice call route");

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceTx with active voice call route");
    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef != NULL,
            "Successfully created txStreamRef with active voice call route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(RX) with active voice call route");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the player stream with RX with active voice call route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(TX) with active voice call route");
    playerRef1 = taf_audio_OpenPlayer(TAF_AUDIO_TX);
    LE_TEST_OK(playerRef1 != NULL,
            "Successfully opened the player stream with TX with active voice call route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder(RX) with active voice call route");
    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
    LE_TEST_OK(recorderRef != NULL,
            "Successfully opened the recorder stream with RX with active voice call route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder(TX) with active voice call route");
    recorderRef1 = taf_audio_OpenRecorder(TAF_AUDIO_TX);
    LE_TEST_OK(recorderRef1 != NULL,
            "Successfully opened the recorder stream with TX with active voice call route");

    LE_TEST_INFO("Test taf_audio_CloseRoute ROUTE_1 voice call");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_1 voice call");

    LE_TEST_INFO("Test taf_audio_Close");
    taf_audio_Close(rxStreamRef);
    taf_audio_Close(txStreamRef);
    taf_audio_Close(playerRef);
    taf_audio_Close(playerRef1);
    taf_audio_Close(recorderRef);
    taf_audio_Close(recorderRef1);
    LE_TEST_OK(true, "Closed all the stream refereces created for voice call");

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1 Playback");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull for playback sinkRef %p", sinkRef);

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceRx with active playback route");
    rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef == NULL,
            "Successfully failed to get rxStreamRef with active playback route");

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceTx with active playback route");
    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef == NULL,
            "Successfully failed to get txStreamRef with active playback route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(RX) with active playback route");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the player stream with RX with active playback route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(TX) with active playback route");
    playerRef1 = taf_audio_OpenPlayer(TAF_AUDIO_TX);
    LE_TEST_OK(playerRef1 == NULL,
            "Successfully failed to get TX player reference with active playback route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder RX with active playback route");
    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
    LE_TEST_OK(recorderRef == NULL,
            "Successfully failed to get RX recorder reference with active playback route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder TX with active playback route");
    recorderRef1 = taf_audio_OpenRecorder(TAF_AUDIO_TX);
    LE_TEST_OK(recorderRef1 == NULL,
            "Successfully failed to get TX recorder reference with active playback route");

    LE_TEST_INFO("Test taf_audio_Close");
    taf_audio_Close(playerRef);
    LE_TEST_OK(true, "Closed all the stream refereces created for playback");

    LE_TEST_INFO("Test taf_audio_CloseRoute ROUTE_1 playback");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_1 playback");

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1 Recording");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_RECORDING,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p", sourceRef);

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceRx with active recording route");
    rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef == NULL,
            "Successfully failed to get rxStreamRef with active recording route");

    LE_TEST_INFO("Test taf_audio_OpenModemVoiceTx with active recording route");
    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef == NULL,
            "Successfully failed to get txStreamRef with active recording route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(RX) with active recording route");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef == NULL,
            "Successfully failed to get player stream with RX with active recording route");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(TX) with active recording route");
    playerRef1 = taf_audio_OpenPlayer(TAF_AUDIO_TX);
    LE_TEST_OK(playerRef1 == NULL,
            "Successfully failed to get TX player reference with active recording route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder(RX) with active recording route");
    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
    LE_TEST_OK(recorderRef == NULL,
            "Successfully failed to get RX recorder reference with active recording route");

    LE_TEST_INFO("Test taf_audio_OpenRecorder(TX) with active recording route");
    recorderRef1 = taf_audio_OpenRecorder(TAF_AUDIO_TX);
    LE_TEST_OK(recorderRef1 != NULL,
            "Successfully failed to get TX recorder reference with active recording route");

    LE_TEST_INFO("Test taf_audio_Close recording route");
    taf_audio_Close(recorderRef1);
    LE_TEST_OK(true, "Closed all the stream refereces created for recording");

    LE_TEST_INFO("Test taf_audio_CloseRoute ROUTE_1 recording");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_1 recording");

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1 loopback");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_LOOPBACK,
            NULL, NULL);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull routeRef %p", routeRef);

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_CloseRoute ROUTE_1 loopback");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_1 loopback");
}

void* Test_taf_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audio_ConnectService();

    taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
    if (streamRef == playerRef) {
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == recorderRef) {
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
    } else if (streamRef == rxStreamRef) {
        dtmfDetectHandlerRef = taf_audio_AddDtmfDetectorHandler(streamRef, MyDtmfDetectorHandler,
                NULL);
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

void TEST_AUDIO_PLAYBACK()
{
    LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_PLAYBACK when other route is not active");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(RX)");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_audio_ut_thread",
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
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(playerRef, amrfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    double volLevel = 1.0;
    LE_TEST_INFO("Test taf_audio_SetVolume on playerRef");
    res = taf_audio_SetVolume(playerRef, volLevel);
    LE_TEST_OK(res == LE_OK, "Successfully set volume to playerRef");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of playerRef");
    res = taf_audio_GetVolume(playerRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of player %f", volLevel);

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of playerRef");
    res = taf_audio_GetVolume(playerRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of player %f", volLevel);

    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(playerRef, wavfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(playerRef, wavfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of playerRef");
    res = taf_audio_GetVolume(playerRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of player %f", volLevel);

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_SetMute to mute playerRef");
    res = taf_audio_SetMute(playerRef, true);
    LE_TEST_OK(res == LE_OK, "Successfully playerRef is muted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of playerRef");
    bool isMute;
    res = taf_audio_GetMute(playerRef, &isMute);
    LE_TEST_OK(isMute, "Successfully get the mute status as true");

    LE_TEST_INFO("Test taf_audio_SetMute to unmute playerRef");
    res = taf_audio_SetMute(playerRef, false);
    LE_TEST_OK(res == LE_OK, "Successfully playerRef is unmuted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of playerRef");
    res = taf_audio_GetMute(playerRef, &isMute);
    LE_TEST_OK(!isMute, "Successfully get the mute status as false");

    LE_TEST_INFO("Test taf_audio_SetMute to mute playerRef");
    res = taf_audio_SetMute(playerRef, true);
    LE_TEST_OK(res == LE_OK, "Successfully playerRef is muted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of playerRef");
    res = taf_audio_GetMute(playerRef, &isMute);
    LE_TEST_OK(isMute, "Successfully get the mute status as true");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
    taf_audio_Disconnect(connRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Close to close playerRef");
    taf_audio_Close(playerRef);
    LE_TEST_OK(true, "Successfully closed playerRef");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");

    LE_TEST_INFO("Test taf_audio_RemoveMediaHandler");
    taf_audio_RemoveMediaHandler(MediaHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for the media callback");

}

void TEST_AUDIO_PLAYBACK_FILE_LIST()
{
    LE_TEST_INFO("Test OpenRoute for PLAYBACK_FILE_LIST");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_PLAYBACK when other route is not active");

    LE_TEST_INFO("Test taf_audio_OpenPlayer(RX)");
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_audio_ut_thread",
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
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

    taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
    snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), "%s", wavfilePath);
    playFileConfig[0].repeat = repeat;

    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list size %zu",
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    res = taf_audio_PlayFileList(playerRef, playFileConfig,
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    LE_TEST_OK(res == LE_OK, "Successfully started the file list playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), "%s", amrfilePath);
    playFileConfig[0].repeat = repeat;

    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list size %zu",
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    res = taf_audio_PlayFileList(playerRef, playFileConfig,
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    LE_TEST_OK(res == LE_OK, "Successfully started the file list playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
    taf_audio_Disconnect(connRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Close to close playerRef");
    taf_audio_Close(playerRef);
    LE_TEST_OK(true, "Successfully closed playerRef");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");

    LE_TEST_INFO("Test taf_audio_RemoveMediaHandler");
    taf_audio_RemoveMediaHandler(MediaHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for the media callback");
}

void TEST_AUDIO_RECORD()
{
    LE_TEST_INFO("Test taf_audio_OpenRoute API with capture mode");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_RECORDING,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_RECORDING when other route is not active");

    LE_TEST_INFO("Test taf_audio_OpenRecorder(TX)");
    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);
    LE_TEST_OK(recorderRef != NULL, "Successfully opened the recorder stream");

    Recorder_thread_ref = le_thread_Create("taf_audio_ut_thread",
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

    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(recorderRef, recordfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Stop recording");
    res = taf_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(recorderRef, recordfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_SetMute to mute recorderRef");
    res = taf_audio_SetMute(recorderRef, true);
    LE_TEST_OK(res == LE_OK, "Successfully recorderRef is muted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of recorderRef");
    bool isMute;
    res = taf_audio_GetMute(recorderRef, &isMute);
    LE_TEST_OK(isMute, "Successfully get the mute status as true");

    LE_TEST_INFO("Test taf_audio_SetMute to unmute recorderRef");
    res = taf_audio_SetMute(recorderRef, false);
    LE_TEST_OK(res == LE_OK, "Successfully recorderRef is unmuted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of recorderRef");
    res = taf_audio_GetMute(recorderRef, &isMute);
    LE_TEST_OK(!isMute, "Successfully get the mute status as false");

    LE_TEST_INFO("Test taf_audio_SetMute to mute sinkRef");
    res = taf_audio_SetMute(sinkRef, true);
    LE_TEST_OK(res == LE_FAULT, "Successfully failed to mute NULL reference");

    LE_TEST_INFO("Test taf_audio_SetMute to mute sourceRef");
    res = taf_audio_SetMute(sourceRef, true);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Successfully failed to mute sourceRef");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of recorderRef");
    res = taf_audio_GetMute(sourceRef, &isMute);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Successfully failed to get the mute status of sourceRef");

    double volLevel = 1.0;
    LE_TEST_INFO("Test taf_audio_SetVolume on recorderRef");
    res = taf_audio_SetVolume(recorderRef, volLevel);
    LE_TEST_OK(res == LE_OK, "Successfully set volume to recorderRef");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
    res = taf_audio_GetVolume(recorderRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of recorder %f", volLevel);

    LE_TEST_INFO("Test taf_audio_SetVolume of sinkRef");
    res = taf_audio_SetVolume(sinkRef, volLevel);
    LE_TEST_OK(res == LE_FAULT, "Successfully failed to set volume to NULL reference");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Stop recording");
    res = taf_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
    res = taf_audio_GetVolume(recorderRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of recorder %f", volLevel);

    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(recorderRef, recordfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of recorderRef");
    res = taf_audio_GetVolume(recorderRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of recorder %f", volLevel);

    LE_TEST_INFO("Test taf_audio_SetVolume on sourceRef");
    res = taf_audio_SetVolume(sourceRef, volLevel);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Successfully failed to set volume to sourceRef");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of sourceRef");
    res = taf_audio_GetVolume(sourceRef, &volLevel);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Sucessfully failed to get the volume level of sourceRef");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Stop recording");
    res = taf_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect recorderRef from connRef");
    taf_audio_Disconnect(connRef, recorderRef);
    LE_TEST_OK(true, "Successfully disconnected recorderRef from ConnectorRef");

    LE_TEST_INFO("Test taf_audio_Close to close recorderRef");
    taf_audio_Close(recorderRef);
    LE_TEST_OK(true, "Successfully closed recorderRef");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_RECORDING route");

    LE_TEST_INFO("Test taf_audio_RemoveMediaHandler");
    taf_audio_RemoveMediaHandler(MediaHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for the media callback");
}

void TEST_ACTIVE_VOICE_PLAYBACK(bool isRemote)
{
    LE_TEST_INFO("Test taf_audio_CreateConnector");
    taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();
    LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

    if(!isRemote)
    {
        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_RX)");
        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        LE_TEST_OK(playerRef != NULL, "Successfully opened the RX player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect sinkRef and playerConnRef");
        res = taf_audio_Connect(playerConnRef, sinkRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to playerConnRef");
    }
    else
    {
        LE_TEST_INFO("Test taf_audio_OpenPlayer(TAF_AUDIO_TX)");
        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_TX);
        LE_TEST_OK(playerRef != NULL, "Successfully opened the TX player stream");

        double volLevel = 1.0;
        LE_TEST_INFO("Test taf_audio_SetVolume");
        res = taf_audio_SetVolume(playerRef, volLevel);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetVolume to the TX player stream");

        LE_TEST_INFO("Test taf_audio_GetVolume");
        res = taf_audio_GetVolume(playerRef, &volLevel);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetVolume to the TX player stream");

        LE_TEST_INFO("Test taf_audio_SetMute");
        res = taf_audio_SetMute(playerRef, true);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetMute to the TX player stream");

        bool isMute;
        LE_TEST_INFO("Test taf_audio_GetMute");
        res = taf_audio_GetMute(playerRef, &isMute);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for GetMute to the TX player stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect playerConnRef and txStreamRef");
        res = taf_audio_Connect(playerConnRef, txStreamRef);
        LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to playerConnRef");
    }

    LE_TEST_INFO("Test taf_audio_Connect to connect playerRef and playerConnRef");
    res = taf_audio_Connect(playerConnRef, playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

    Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
            Test_taf_audio_AddHandler, (void*)playerRef);
    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_PlayFile to play a file");
    res = taf_audio_PlayFile(playerRef, wavfilePath);

    LE_TEST_OK(res == LE_OK, "Successfully started the incall uplink playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the incall uplink playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, StopTimeout);

    taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
    snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), "%s", amrfilePath);
    playFileConfig[0].repeat = repeat;

    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list size %zd",
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    res = taf_audio_PlayFileList(playerRef, playFileConfig,
            sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
    LE_TEST_OK(res == LE_OK, "Successfully started the incall file list playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_Stop playback");
    res = taf_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect playerRef from connRef");
    taf_audio_Disconnect(playerConnRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from playerConnRef");

    LE_TEST_INFO("Test taf_audio_Close to close playerRef");
    taf_audio_Close(playerRef);
    LE_TEST_OK(true, "Successfully closed playerRef");

    LE_TEST_INFO("Test taf_audio_RemoveMediaHandler");
    taf_audio_RemoveMediaHandler(MediaHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for the media callback");

    LE_TEST_INFO("Test taf_audio_DeleteConnector");
    taf_audio_DeleteConnector(playerConnRef);
    LE_TEST_OK(true, "Successfully deleted the player connector reference");

}

void TEST_INCALL_AUDIO_RECORDING(bool isRemote)
{
    LE_TEST_INFO("Test taf_audio_CreateConnector");
    taf_audio_ConnectorRef_t connRef = taf_audio_CreateConnector();
    LE_TEST_OK(connRef != NULL, "Successfully created Connector ");

    if (!isRemote)
    {
        LE_TEST_INFO("Test taf_audio_OpenRecorder(TAF_AUDIO_TX)");
        recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);
        LE_TEST_OK(recorderRef != NULL, "Successfully opened the TX recorder stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect sourceRef and connRef");
        res = taf_audio_Connect(connRef, sourceRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected sourceRef to connRef");

    }
    else
    {
        LE_TEST_INFO("Test taf_audio_OpenRecorder(TAF_AUDIO_RX)");
        recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
        LE_TEST_OK(recorderRef != NULL, "Successfully opened the RX recorder stream");

        double volLevel = 1.0;
        LE_TEST_INFO("Test taf_audio_SetVolume");
        res = taf_audio_SetVolume(recorderRef, volLevel);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetVolume to the RX recorder stream");

        LE_TEST_INFO("Test taf_audio_GetVolume");
        res = taf_audio_GetVolume(recorderRef, &volLevel);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetVolume to the RX recorder stream");

        LE_TEST_INFO("Test taf_audio_SetMute");
        res = taf_audio_SetMute(recorderRef, true);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for SetMute to the RX recorder stream");

        bool isMute;
        LE_TEST_INFO("Test taf_audio_GetMute");
        res = taf_audio_GetMute(recorderRef, &isMute);
        LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully returned UNSUPPORTED for GetMute to the RX recorder stream");

        LE_TEST_INFO("Test taf_audio_Connect to connect rxStreamRef and connRef");
        res = taf_audio_Connect(connRef, rxStreamRef);
        LE_TEST_OK(res == LE_OK, "Successfully connected rxStreamRef to connRef");
    }

    LE_TEST_INFO("Test taf_audio_Connect to connect recorderRef and connRef");
    res = taf_audio_Connect(connRef, recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to connRef");

    Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
            Test_taf_audio_AddHandler, (void*)recorderRef);
    le_thread_Start(Recorder_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_RecordFile to record a file");
    res = taf_audio_RecordFile(recorderRef, recordfilePath);
    if(isRemote)
    {
        LE_TEST_OK(res == LE_OK, "Successfully started the incall downlink file recording");

        le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

        LE_TEST_INFO("Test taf_audio_Stop recording");
        res = taf_audio_Stop(recorderRef);
        LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

        le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);
    }
    else
    {
        LE_TEST_OK(res == LE_UNSUPPORTED, "Failed to start the incall uplink file recording");
    }

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect recorderRef from connRef");
    taf_audio_Disconnect(connRef, recorderRef);
    LE_TEST_OK(true, "Successfully disconnected recorderRef from connRef");

    LE_TEST_INFO("Test taf_audio_Close to close recorderRef");
    taf_audio_Close(recorderRef);
    LE_TEST_OK(true, "Successfully closed recorderRef");

    LE_TEST_INFO("Test taf_audio_RemoveMediaHandler");
    taf_audio_RemoveMediaHandler(MediaHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for the media callback");

    LE_TEST_INFO("Test taf_audio_DeleteConnector");
    taf_audio_DeleteConnector(connRef);
    LE_TEST_OK(true, "Successfully deleted the recorder connector reference");
}

void TEST_AUDIO_VOICE_CONNECTION()
{
    static const char*  DtmfString = "5";
    static uint16_t     Duration = 5000;
    static uint32_t     Pause = 1000;
    static uint32_t     SlotId = 1;
    static double       gain = 1.0;
    le_result_t res;

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_1");
    routeRef1 = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL, "OpenRoute successfull failed for voicecall when other route is active");

    LE_TEST_INFO("Test taf_audio_OpenRoute API with playback mode");
    routeRef1 = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL,
            "OpenRoute successfull failed for LOCAL_PLAYBACK when other route is active");

    LE_TEST_INFO("Test taf_audio_OpenRoute API with record mode");
    routeRef1 = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_RECORDING,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL,
            "OpenRoute successfull failed for LOCAL_RECORDING when other route is active");

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
    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef != NULL, "Successfully created txStreamRef %p", txStreamRef);

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

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_AddDtmfDetectorHandler");
    Dtmf_detect_thread_ref = le_thread_Create("taf_audio_dtmf_thread",
            Test_taf_audio_AddHandler, (void*)rxStreamRef);
    LE_TEST_OK(
        (Dtmf_detect_thread_ref != NULL), "Successfully registered for DTMF detection event- Pass");
    le_thread_Start(Dtmf_detect_thread_ref);
    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_audio_RemoveDtmfDetectorHandler");
    taf_audio_RemoveDtmfDetectorHandler(dtmfDetectHandlerRef);
    LE_TEST_OK(true, "Successfully deregistered for DTMF detection event- Pass");

    LE_TEST_INFO("To test taf_audio_PlayDtmf on rxStreamRef.%p", rxStreamRef);
    res = taf_audio_PlayDtmf(rxStreamRef, DtmfString, Duration, Pause, gain);
    LE_TEST_OK((res == LE_OK), "taf_audio_PlayDtmf - Pass");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("To test taf_audio_StopDtmf on rxStreamRef.%p", rxStreamRef);
    res = taf_audio_StopDtmf(rxStreamRef);
    LE_TEST_OK((res == LE_OK), "taf_audio_StopDtmf - Pass");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("To test taf_audio_PlaySignallingDtmf.");
    res = taf_audio_PlaySignallingDtmf(SlotId, DtmfString, Duration, Pause);
    LE_TEST_OK((res == LE_OK), "taf_audio_PlaySignallingDtmf - Pass");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("To test taf_audio_StopSignallingDtmf");
    res = taf_audio_StopSignallingDtmf(SlotId);
    LE_TEST_OK((res == LE_OK), "taf_audio_StopSignallingDtmf - Pass");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    double volLevel = 1.0;
    LE_TEST_INFO("Test taf_audio_SetVolume on rxStreamRef");
    res = taf_audio_SetVolume(rxStreamRef, volLevel);
    LE_TEST_OK(res == LE_OK, "Successfully set volume to rxStreamRef");

    LE_TEST_INFO("Test taf_audio_GetVolume to get volume of rxStreamRef");
    res = taf_audio_GetVolume(rxStreamRef, &volLevel);
    LE_TEST_OK(volLevel == 1.0, "Successfully get the volume level of rxStreamRef %f", volLevel);

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice TX");
    res = taf_audio_SetMute(txStreamRef, true);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef is muted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of modem voice TX");
    bool isMute;
    res = taf_audio_GetMute(txStreamRef, &isMute);
    LE_TEST_OK(isMute, "Successfully get the mute status as true");

    LE_TEST_INFO("Test taf_audio_SetMute to unmute modem voice TX");
    res = taf_audio_SetMute(txStreamRef, false);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef is unmuted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of modem voice TX");
    res = taf_audio_GetMute(txStreamRef, &isMute);
    LE_TEST_OK(!isMute, "Successfully get the mute status as false");

    LE_TEST_INFO("Test taf_audio_SetMute to mute modem voice RX");
    res = taf_audio_SetMute(rxStreamRef, true);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef is muted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of modem voice RX");
    res = taf_audio_GetMute(rxStreamRef, &isMute);
    LE_TEST_OK(isMute, "Successfully get the mute status as true");

    LE_TEST_INFO("Test taf_audio_SetMute to unmute modem voice RX");
    res = taf_audio_SetMute(rxStreamRef, false);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef is unmuted");

    LE_TEST_INFO("Test taf_audio_GetMute to get mute status of modem voice RX");
    res = taf_audio_GetMute(rxStreamRef, &isMute);
    LE_TEST_OK(!isMute, "Successfully get the mute status as false");

    TEST_ACTIVE_VOICE_PLAYBACK(false);

    TEST_ACTIVE_VOICE_PLAYBACK(true);

    TEST_INCALL_AUDIO_RECORDING(false);

    TEST_INCALL_AUDIO_RECORDING(true);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected from txConn");

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect sinkRef from rxConn");
    taf_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to disconnect sourceRef from txConn");
    taf_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and rxStreamRef");
    taf_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_2");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_2, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

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

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txStreamRef from txConn");
    taf_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected from txConn");

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect sinkRef from rxConn");
    taf_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect sourceRef from txConn");
    taf_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect rxStreamRef from rxConn");
    taf_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_audio_OpenRoute API ROUTE_3");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_3, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

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

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected to txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and sinkRef");
    taf_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect txConn and sourceRef");
    taf_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_audio_Connect to connect rxConn and rxStreamRef");
    taf_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_audio_Close to close txStreamRef");
    taf_audio_Close(txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef closed");

    LE_TEST_INFO("Test taf_audio_Close to close rxStreamRef");
    taf_audio_Close(rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef closed");

    LE_TEST_INFO("Test taf_audio_CloseRoute");
    res = taf_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_audio_DeleteConnector txConn");
    taf_audio_DeleteConnector(txConn);
    LE_TEST_OK(true, "Successfully txConn deleted");

    LE_TEST_INFO("Test taf_audio_DeleteConnector to connect rxConn and rxStreamRef");
    taf_audio_DeleteConnector(rxConn);
    LE_TEST_OK(true, "Successfully rxConn deleted");

    LE_TEST_INFO("Test taf_audio_OpenRoute API");
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_4, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef == NULL, "OpenRoute Failed for ROUTE_4");
}

static void NodeStateChangeCallback(uint8_t audioNodeId, taf_audioVendor_Event_t event,
    void* contextPtr)
{
    LE_INFO("Received node state change callback for audio device %d for event %d",
    audioNodeId, event);
}

void TEST_AUDIO_VHAL_DEV_APIS()
{
    le_result_t res;
    const char* configurePath = "/data/audioConfigure.xml";
    taf_audioVendor_NodePowerState_t state;
    bool muteState;
    double gainPerc = 0.5;
    double getGainPerc;
    taf_audioVendor_Direction_t direction = TAF_AUDIOVENDOR_RX;
    taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef;

    LE_TEST_INFO("Test taf_audioVendor_GetNodeType");
    taf_audioVendor_NodeType_t nodeType;
    res = taf_audioVendor_GetNodeType(0x1, &nodeType);
    LE_TEST_OK(nodeType != TAF_AUDIOVENDOR_INVALID, "Successfully got the audio device type");

    LE_TEST_INFO("Test taf_audioVendor_SendNodeConfigure");
    res = taf_audioVendor_SendNodeVendorConfig(0x1, configurePath);
    LE_TEST_OK(res == LE_OK, "Successfully send the audio device configuration to VHAL");

    LE_TEST_INFO("Test taf_audioVendor_SendHWConfig");
    res = taf_audioVendor_SendVendorConfig(configurePath);
    LE_TEST_OK(res == LE_OK, "Successfully sent the configuration to VHAL");

    LE_TEST_INFO("Test taf_audioVendor_SetNodePowerState SUSPEND");
    res = taf_audioVendor_SetNodePowerState(0x1, TAF_AUDIOVENDOR_SUSPEND);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device power state");

    LE_TEST_INFO("Test taf_audioVendor_GetNodePowerState");
    res = taf_audioVendor_GetNodePowerState(0x1, &state);
    LE_TEST_OK(state == TAF_AUDIOVENDOR_SUSPEND,
            "Successfully got the audio device power state SUSPEND, res is %d", res);

    LE_TEST_INFO("Test taf_audioVendor_SetNodePowerState POWER_OFF");
    res = taf_audioVendor_SetNodePowerState(0x1, TAF_AUDIOVENDOR_POWER_OFF);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device power state");

    LE_TEST_INFO("Test taf_audioVendor_GetNodePowerState");
    res = taf_audioVendor_GetNodePowerState(0x1, &state);
    LE_TEST_OK(state == TAF_AUDIOVENDOR_POWER_OFF,
            "Successfully got the audio device power state POWER_OFF, res is %d", res);

    LE_TEST_INFO("Test taf_audioVendor_SetNodePowerState ACTIVE");
    res = taf_audioVendor_SetNodePowerState(0x1, TAF_AUDIOVENDOR_ACTIVE);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device power state");

    LE_TEST_INFO("Test taf_audioVendor_GetNodePowerState");
    res = taf_audioVendor_GetNodePowerState(0x1, &state);
    LE_TEST_OK(state == TAF_AUDIOVENDOR_ACTIVE,
            "Successfully got the audio device power state ACTIVE, res is %d", res);

    LE_TEST_INFO("Test taf_audioVendor_SetNodeMuteStatus");
    res = taf_audioVendor_SetNodeMuteState(0x1, true);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device to mute state");

    LE_TEST_INFO("Test taf_audioVendor_GetNodeMuteStatus");
    res = taf_audioVendor_GetNodeMuteState(0x1, &muteState);
    LE_TEST_OK(muteState, "Successfully got the audio device mute state as mute res is %d", res);

    LE_TEST_INFO("Test taf_audioVendor_SetNodeMuteStatus");
    res = taf_audioVendor_SetNodeMuteState(0x1, false);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device to unmute state");

    LE_TEST_INFO("Test taf_audioVendor_GetNodeMuteStatus");
    res = taf_audioVendor_GetNodeMuteState(0x1, &muteState);
    LE_TEST_OK(!muteState, "Successfully got the device mute state as unmute res is %d", res);

    LE_TEST_INFO("Test taf_audioVendor_SetNodeGain");
    res = taf_audioVendor_SetNodeGain(0x1, direction, gainPerc);
    LE_TEST_OK(res == LE_OK, "Successfully set the audio device gain");

    LE_TEST_INFO("Test taf_audioVendor_GetNodeGain");
    res = taf_audioVendor_GetNodeGain(0x1, direction, &getGainPerc);
    LE_TEST_OK(res == LE_OK, "Successfully got the gain of audio device %f", getGainPerc);

    direction = TAF_AUDIOVENDOR_TX;
    LE_TEST_INFO("Test taf_audioVendor_SetNodeGain");
    res = taf_audioVendor_SetNodeGain(0x1, direction, gainPerc);
    LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully tested setting audio device gain for unsupported use-case");

    LE_TEST_INFO("Test taf_audioVendor_GetNodeGain");
    res = taf_audioVendor_GetNodeGain(0x1, direction, &getGainPerc);
    LE_TEST_OK(res == LE_UNSUPPORTED,
                "Successfully tested getting the gain of audio device for unsupported use-case");

    LE_TEST_INFO("Test taf_audioVendor_AddNodeStateChangeHandler");
    handlerRef = taf_audioVendor_AddNodeStateChangeHandler(0x1, NodeStateChangeCallback, NULL);
    LE_TEST_OK(handlerRef != NULL, "Successfully registered for node state change callback");

    LE_TEST_INFO("Test taf_audioVendor_RemoveNodeStateChangeHandler");
    taf_audioVendor_RemoveNodeStateChangeHandler(handlerRef);
    LE_TEST_OK(true, "Successfully deregistered for node state change callback");
}

COMPONENT_INIT
{

    tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);

    TEST_AUDIO_VOICE_CONNECTION();

    TEST_OPEN_ROUTE();

    TEST_AUDIO_PLAYBACK();

    TEST_AUDIO_PLAYBACK_FILE_LIST();

    TEST_AUDIO_RECORD();

    TEST_AUDIO_VHAL_DEV_APIS();

    LE_TEST_EXIT;
}
