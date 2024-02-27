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

#include "legato.h"
#include "interfaces.h"

const char* wavfilePath = "/data/test.wav";
const char* amrfilePath = "/data/test.amr";
const char* recordfilePath = "/data/record.wav";
static le_sem_Ref_t tafAudioAppSem;
static taf_mngd_audio_MediaHandlerRef_t MediaHandlerRef = NULL;
le_clk_Time_t Timeout = { 3 , 0 };
static le_thread_Ref_t Player_thread_ref, Recorder_thread_ref;
taf_mngd_audio_StreamRef_t recorderRef = NULL, playerRef = NULL, playerRef1 = NULL,
        recorderRef1 = NULL, sinkRef = NULL, sourceRef = NULL, rxStreamRef = NULL,
        txStreamRef = NULL, sinkRef1 = NULL, sourceRef1 = NULL;
taf_mngd_audio_RouteRef_t routeRef = NULL, routeRef1 = NULL;
taf_mngd_audio_ConnectorRef_t rxConn = NULL, txConn = NULL, connRef = NULL;
le_result_t res;

static void MyMediaEventHandler
(
    taf_mngd_audio_StreamRef_t          streamRef,
    taf_mngd_audio_MediaEvent_t         event,
    void*                               contextPtr
)
{
    switch(event)
    {
        case TAF_MNGD_AUDIO_MEDIA_ENDED:
            LE_TEST_OK(true," Playback completed");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_STOPPED:
            LE_TEST_OK(true," Playback/capture stopped");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_ERROR:
            LE_TEST_OK(true, "File event is TAF_AUDIO_MEDIA_ERROR.");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_NO_MORE_SAMPLES:
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
    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute with ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceRx with active voice call route");
    rxStreamRef = taf_mngd_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef != NULL,
            "Successfully created rxStreamRef with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceTx with active voice call route");
    txStreamRef = taf_mngd_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef != NULL,
            "Successfully created txStreamRef with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(RX) with active voice call route");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the player stream with RX with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(TX) with active voice call route");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the player stream with TX with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(RX) with active voice call route");
    playerRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the recorder stream with RX with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(TX) with active voice call route");
    playerRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the recorder stream with TX with active voice call route");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute ROUTE_0 voice call");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_0 voice call");

    LE_TEST_INFO("Test taf_mngd_audio_Close");
    taf_mngd_audio_Close(rxStreamRef);
    taf_mngd_audio_Close(txStreamRef);
    taf_mngd_audio_Close(playerRef);
    taf_mngd_audio_Close(playerRef1);
    taf_mngd_audio_Close(recorderRef);
    taf_mngd_audio_Close(recorderRef1);
    LE_TEST_OK(true, "Closed all the stream refereces created for voice call");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0 Playback");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull for playback sinkRef %p", sinkRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceRx with active playback route");
    rxStreamRef = taf_mngd_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef == NULL,
            "Successfully failed to get rxStreamRef with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceTx with active playback route");
    txStreamRef = taf_mngd_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef == NULL,
            "Successfully failed to get txStreamRef with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(RX) with active playback route");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL,
            "Successfully opened the player stream with RX with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(TX) with active playback route");
    playerRef1 = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(playerRef1 == NULL,
            "Successfully failed to get TX player reference with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder RX with active playback route");
    recorderRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(recorderRef == NULL,
            "Successfully failed to get RX recorder reference with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder TX with active playback route");
    recorderRef1 = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(recorderRef1 == NULL,
            "Successfully failed to get TX recorder reference with active playback route");

    LE_TEST_INFO("Test taf_mngd_audio_Close");
    taf_mngd_audio_Close(playerRef);
    LE_TEST_OK(true, "Closed all the stream refereces created for playback");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute ROUTE_0 playback");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_0 playback");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0 Recording");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_RECORDING,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p", sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceRx with active recording route");
    rxStreamRef = taf_mngd_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef == NULL,
            "Successfully failed to get rxStreamRef with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceTx with active recording route");
    txStreamRef = taf_mngd_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef == NULL,
            "Successfully failed to get txStreamRef with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(RX) with active recording route");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef == NULL,
            "Successfully failed to get player stream with RX with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(TX) with active recording route");
    playerRef1 = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(playerRef1 == NULL,
            "Successfully failed to get TX player reference with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(RX) with active recording route");
    recorderRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(recorderRef == NULL,
            "Successfully failed to get RX recorder reference with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(TX) with active recording route");
    recorderRef1 = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(recorderRef1 != NULL,
            "Successfully failed to get TX recorder reference with active recording route");

    LE_TEST_INFO("Test taf_mngd_audio_Close recording route");
    taf_mngd_audio_Close(recorderRef1);
    LE_TEST_OK(true, "Closed all the stream refereces created for recording");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute ROUTE_0 recording");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the ROUTE_0 recording");
}

void* Test_taf_mngd_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem = NULL;
    taf_mngd_audio_ConnectService();

    taf_mngd_audio_StreamRef_t streamRef = (taf_mngd_audio_StreamRef_t)ctxPtr;
    MediaHandlerRef = taf_mngd_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
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

void TEST_MNGD_AUDIO_PLAYBACK()
{
    LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_PLAYBACK when other route is not active");

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(RX)");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_mngd_audio_ut_thread",
            Test_taf_mngd_audio_AddHandler, (void*)playerRef);
    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    taf_mngd_audio_ConnectorRef_t playerConnRef = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect sinkRef and playerConnRef");
    res = taf_mngd_audio_Connect(playerConnRef, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect playerRef and playerConnRef");
    res = taf_mngd_audio_Connect(playerConnRef, playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected playerRef to playerConnRef");

    LE_TEST_INFO("Test taf_mngd_audio_PlayFile to play a file");
    res = taf_mngd_audio_PlayFile(playerRef, amrfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Stop playback");
    res = taf_mngd_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_PlayFile to play a file");
    res = taf_mngd_audio_PlayFile(playerRef, wavfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect playerRef from connRef");
    taf_mngd_audio_Disconnect(connRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_Close to close playerRef");
    taf_mngd_audio_Close(playerRef);
    LE_TEST_OK(true, "Successfully closed playerRef");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");

}

void TEST_MNGD_AUDIO_RECORD()
{
    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with capture mode");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_RECORDING,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_RECORDING when other route is not active");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(TX)");
    recorderRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(recorderRef != NULL, "Successfully opened the recorder stream");

    Recorder_thread_ref = le_thread_Create("taf_mngd_audio_ut_thread",
            Test_taf_mngd_audio_AddHandler, (void*)recorderRef);
    le_thread_Start(Recorder_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    taf_mngd_audio_ConnectorRef_t connRef = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(connRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect sourceRef and connRef");
    res = taf_mngd_audio_Connect(connRef, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sourceRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect recorderRef and connRef");
    res = taf_mngd_audio_Connect(connRef, recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_RecordFile to record a file");
    res = taf_mngd_audio_RecordFile(recorderRef, recordfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Stop recording");
    res = taf_mngd_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_RecordFile to record a file");
    res = taf_mngd_audio_RecordFile(recorderRef, recordfilePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Stop recording");
    res = taf_mngd_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect recorderRef from connRef");
    taf_mngd_audio_Disconnect(connRef, recorderRef);
    LE_TEST_OK(true, "Successfully disconnected recorderRef from ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_Close to close recorderRef");
    taf_mngd_audio_Close(recorderRef);
    LE_TEST_OK(true, "Successfully closed recorderRef");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_RECORDING route");
}

void TEST_MNGD_AUDIO_VOICE_CONNECTION()
{
    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef == NULL, "OpenRoute successfull for voicecall when other route is active");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with FORCE mode for ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0,
            TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN, &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "Force OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with playback mode");
    routeRef1 = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL,
            "OpenRoute successfull failed for LOCAL_PLAYBACK when other route is active");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with record mode");
    routeRef1 = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_RECORDING,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL,
            "OpenRoute successfull failed for LOCAL_RECORDING when other route is active");

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    rxConn = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", rxConn);

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    txConn = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", txConn);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceRx");
    rxStreamRef = taf_mngd_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef != NULL, "Successfully created rxStreamRef %p", rxStreamRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceTx");
    txStreamRef = taf_mngd_audio_OpenModemVoiceTx(1, true);
    LE_TEST_OK(txStreamRef != NULL, "Successfully created txStreamRef %p", txStreamRef);

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and sinkRef");
    res = taf_mngd_audio_Connect(rxConn, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and sourceRef");
    res = taf_mngd_audio_Connect(txConn, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    res = taf_mngd_audio_Connect(rxConn, rxStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and txStreamRef");
    res = taf_mngd_audio_Connect(txConn, txStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txConn");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_mngd_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected from txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect sinkRef from rxConn");
    taf_mngd_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to disconnect sourceRef from txConn");
    taf_mngd_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    taf_mngd_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_1");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_1, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and sinkRef");
    res = taf_mngd_audio_Connect(rxConn, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and sourceRef");
    res = taf_mngd_audio_Connect(txConn, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    res = taf_mngd_audio_Connect(rxConn, rxStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and txStreamRef");
    res = taf_mngd_audio_Connect(txConn, txStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txConn");

    le_sem_WaitWithTimeOut(tafAudioAppSem, Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect txStreamRef from txConn");
    taf_mngd_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected from txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect sinkRef from rxConn");
    taf_mngd_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect sourceRef from txConn");
    taf_mngd_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect rxStreamRef from rxConn");
    taf_mngd_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_2");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_2, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and sinkRef");
    res = taf_mngd_audio_Connect(rxConn, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and sourceRef");
    res = taf_mngd_audio_Connect(txConn, sourceRef);
    LE_TEST_OK(res == LE_OK, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    res = taf_mngd_audio_Connect(rxConn, rxStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and txStreamRef");
    res = taf_mngd_audio_Connect(txConn, txStreamRef);
    LE_TEST_OK(res == LE_OK, "Successfully txStreamRef connected to txConn");

    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_mngd_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and sinkRef");
    taf_mngd_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and sourceRef");
    taf_mngd_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef disconnected from txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    taf_mngd_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef disconnected from rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Close to close txStreamRef");
    taf_mngd_audio_Close(txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef closed");

    LE_TEST_INFO("Test taf_mngd_audio_Close to close rxStreamRef");
    taf_mngd_audio_Close(rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef closed");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

    LE_TEST_INFO("Test taf_mngd_audio_DeleteConnector txConn");
    taf_mngd_audio_DeleteConnector(txConn);
    LE_TEST_OK(true, "Successfully txConn deleted");

    LE_TEST_INFO("Test taf_mngd_audio_DeleteConnector to connect rxConn and rxStreamRef");
    taf_mngd_audio_DeleteConnector(rxConn);
    LE_TEST_OK(true, "Successfully rxConn deleted");

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_3, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef == NULL, "OpenRoute Failed for ROUTE_3");
}

COMPONENT_INIT
{

    tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);

    TEST_MNGD_AUDIO_VOICE_CONNECTION();

    TEST_OPEN_ROUTE();

    TEST_MNGD_AUDIO_PLAYBACK();

    TEST_MNGD_AUDIO_RECORD();

    LE_TEST_EXIT;
}