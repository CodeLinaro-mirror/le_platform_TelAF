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

#include "legato.h"
#include "interfaces.h"

using namespace std;

static le_sem_Ref_t tafAudioAppSem;
static taf_mngd_audio_MediaHandlerRef_t MediaHandlerRef = NULL;
taf_mngd_audio_StreamRef_t sinkRef = NULL, recorderRef = NULL, playerRef = NULL;
taf_mngd_audio_StreamRef_t sourceRef = NULL, rxStreamRef = NULL, txStreamRef = NULL;
taf_mngd_audio_RouteRef_t routeRef = NULL;
taf_mngd_audio_ConnectorRef_t rxConn = NULL, txConn = NULL, playerConnRef = NULL, connRef = NULL;
taf_mngd_audio_PlayListRef_t playListRef = NULL;
le_result_t res;
static le_thread_Ref_t Player_thread_ref, Recorder_thread_ref;
taf_mngd_audio_RouteId_t routeId;

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
            LE_INFO(" Playback completed");
            std::cout<<"****Playback completed***"<<endl;
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_STOPPED:
            LE_INFO(" Playback/capture stopped");
            std::cout<<"****Playback/capture stopped***"<<endl;
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
            std::cout<<"****Playback error***"<<endl;
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_MNGD_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            std::cout<<"****Playback no more samples***"<<endl;
            le_sem_Post(tafAudioAppSem);
            break;
        default:
            LE_INFO("File event is %d", event);
            le_sem_Post(tafAudioAppSem);
            break;
    }
    exit(EXIT_SUCCESS);
}

void* Test_taf_mngd_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
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

void Test_Mngd_Audio_Playback(const char* filePath)
{
    LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
    routeRef = taf_mngd_audio_OpenRoute( routeId, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_PLAYBACK");

    if(routeRef == NULL)
    {
        std::cout<<"****Failed to create route for playback***"<<endl;
        exit(0);
    }

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX)");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_mngd_audio_test_thread",
            Test_taf_mngd_audio_AddHandler, (void*)playerRef);
    le_thread_Start(Player_thread_ref);

    le_sem_Wait(tafAudioAppSem);

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    playerConnRef = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(playerConnRef != NULL, "Successfully created Connector ");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect sinkRef and playerConnRef");
    res = taf_mngd_audio_Connect(playerConnRef, sinkRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected sinkRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect playerRef and playerConnRef");
    res = taf_mngd_audio_Connect(playerConnRef, playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_RecordFile to record a file");
    res = taf_mngd_audio_PlayFile(playerRef, filePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file playback");

    if(res != LE_OK)
    {
        std::cout<<"****Failed to start playback***"<<endl;
        exit(0);
    }
}

void Test_Mngd_Audio_PlayList_Setup(const char* filePath)
{
    LE_TEST_INFO("Test OpenRoute for LOCAL_PLAYBACK");
    routeRef = taf_mngd_audio_OpenRoute( routeId, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_PLAYBACK when other route is not active");

    if(routeRef == NULL)
    {
        std::cout<<"****Failed to create route for playback***"<<endl;
        exit(0);
    }

    LE_TEST_INFO("Test taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX)");
    playerRef = taf_mngd_audio_OpenPlayer(TAF_MNGD_AUDIO_RX);
    LE_TEST_OK(playerRef != NULL, "Successfully opened the player stream");

    Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
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
    LE_TEST_OK(res == LE_OK, "Successfully connected recorderRef to ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_CreatePlayList to create playerListRef");
    playListRef = taf_mngd_audio_CreatePlayList();
    LE_TEST_OK(res == LE_OK, "Successfully create playerListRef");
}

void Test_Mngd_Audio_Record(const char* filePath)
{
    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with capture mode");
    routeRef = taf_mngd_audio_OpenRoute( routeId, TAF_MNGD_AUDIO_LOCAL_RECORDING,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL,
            "OpenRoute successfull for LOCAL_RECORDING");
    if(routeRef == NULL)
    {
        std::cout<<"****Failed to create route for record***"<<endl;
        exit(0);
    }

    LE_TEST_INFO("Test taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX)");
    recorderRef = taf_mngd_audio_OpenRecorder(TAF_MNGD_AUDIO_TX);
    LE_TEST_OK(recorderRef != NULL, "Successfully opened the recorder stream");

    Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
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
    res = taf_mngd_audio_RecordFile(recorderRef, filePath);
    LE_TEST_OK(res == LE_OK, "Successfully started the file recording");

    if(res != LE_OK)
    {
        std::cout<<"****Failed to start recording***"<<endl;
        exit(0);
    }
}

void Test_Mngd_Audio_Playback_Stop()
{
    LE_TEST_INFO("Test taf_mngd_audio_Stop playback");
    res = taf_mngd_audio_Stop(playerRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file playback");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect playerRef from connRef");
    taf_mngd_audio_Disconnect(playerConnRef, playerRef);
    LE_TEST_OK(true, "Successfully disconnected playerRef from ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_PLAYBACK route");
}

void Test_Mngd_Audio_Record_Stop()
{
    LE_TEST_INFO("Test taf_mngd_audio_Stop recording");
    res = taf_mngd_audio_Stop(recorderRef);
    LE_TEST_OK(res == LE_OK, "Successfully stopped the file recording");

    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect recorderRef from connRef");
    taf_mngd_audio_Disconnect(connRef, recorderRef);
    LE_TEST_OK(true, "Successfully disconnected recorderRef from ConnectorRef");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the LOACL_RECORDING route");
}

void Test_Mngd_Audio_VoiceCall()
{
    if(le_arg_GetArg(2) != NULL && (strcmp(le_arg_GetArg(2), "force") == 0))
    {
        LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0 force open");
        routeRef = taf_mngd_audio_OpenRoute( routeId, TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN,
                &sinkRef, &sourceRef);
        LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
                sourceRef);
    } else {
        LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0");
        routeRef = taf_mngd_audio_OpenRoute( routeId, TAF_MNGD_AUDIO_VOICE_CALL,
                &sinkRef, &sourceRef);
        LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
                sourceRef);
    }

    if(routeRef == NULL)
    {
        std::cout<<"****Failed to create route for voice call***"<<endl;
        exit(0);
    }

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

    if(res != LE_OK)
    {
        std::cout<<"****Failed to connect voice call audio***"<<endl;
        exit(0);
    }
}

void Test_Mngd_Audio_VoiceCall_Stop()
{
    LE_TEST_INFO("Test taf_mngd_audio_Disconnect to disconnect txConn and txStreamRef");
    taf_mngd_audio_Disconnect(txConn, txStreamRef);
    LE_TEST_OK(true, "Successfully txStreamRef disconnected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and sinkRef");
    taf_mngd_audio_Disconnect(rxConn, sinkRef);
    LE_TEST_OK(true, "Successfully sinkRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect txConn and sourceRef");
    taf_mngd_audio_Disconnect(txConn, sourceRef);
    LE_TEST_OK(true, "Successfully sourceRef connected to txConn");

    LE_TEST_INFO("Test taf_mngd_audio_Connect to connect rxConn and rxStreamRef");
    taf_mngd_audio_Disconnect(rxConn, rxStreamRef);
    LE_TEST_OK(true, "Successfully rxStreamRef connected to rxConn");

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");
}

void PrintHelp()
{
    std::cout<<endl<<"Usage details:"<<endl;
    std::cout<<"**************"<<endl;
    std::cout<<"app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- <route_id> playback/record <File Name>"<<endl;
    std::cout<<"app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- route0 voicecall"<<endl;
    std::cout<<"app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- route0 voicecall force"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- route0 playback /data/test.wav"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- route0 record /data/record.wav"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- route0 repeated_playback"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node <node_id> getNodeType"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 getNodeType"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 setPowerState ACTIVE"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 getPowerState"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 setMuteState true"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 getMuteState"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 regsiterNodeEvent"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- setVendorConfig /data/config.xml"<<endl;
    std::cout<<"Ex: app runProc tafMngdAudioConsoleApp tafMngdAudioConsoleApp -- node 1 setNodeVendorConfig /data/config.xml"<<endl;
}

void Test_Mngd_Audio_Add_File(string srcPath, int32_t repeat){
    LE_TEST_INFO("Test taf_mngd_audio_AddPlayListEntry to add a playback file");
    res = taf_mngd_audio_AddPlayListEntry(playListRef, srcPath.c_str(), repeat);
    LE_TEST_OK(res == LE_OK, "Successfully added file to playerListRef");
}

void Test_Mngd_Audio_Start_PlayFileList(){
    LE_TEST_INFO("Test taf_mngd_audio_PlayFileList to play a file list");
    res = taf_mngd_audio_PlayFileList(playerRef, playListRef);
    LE_TEST_OK(res == LE_OK, "Successfully started the file list playback");

    if(res != LE_OK)
    {
        std::cout<<"****Failed to start playback***"<<endl;
        exit(0);
    }
}

void Test_Mngd_Audio_Delete_PlayList(){
    LE_TEST_INFO("Test taf_mngd_audio_DeletePlayList to delete playerListRef");
    res = taf_mngd_audio_DeletePlayList(playListRef);
    LE_TEST_OK(res == LE_OK, "Successfully deleted playerListRef");
}

static void NodeEventCallback(uint8_t nodeId, taf_mngd_audioHw_Event_t event, void* contextPtr)
{
    const char* nodeEvent = "";
    if ( event == TAF_MNGD_AUDIOHW_MUTE )
    {
        nodeEvent = "mute";
    } else if ( event == TAF_MNGD_AUDIOHW_UNMUTE)
    {
        nodeEvent = "unmute";
    }
    printf("Node event for node %s\n", nodeEvent);
}

COMPONENT_INIT
{

    int NumberOfArgs = le_arg_NumArgs();
    tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
    if (NumberOfArgs >= 1) {
        if(strcmp(le_arg_GetArg(0), "help") == 0)
        {
            PrintHelp();
            exit(EXIT_SUCCESS);
        }
        else
        {
            if(strcmp(le_arg_GetArg(0),"route0") == 0)
            {
                routeId = TAF_MNGD_AUDIO_ROUTE_0;
            } else if(strcmp(le_arg_GetArg(0),"route1") == 0)
            {
                routeId = TAF_MNGD_AUDIO_ROUTE_1;
            } else if(strcmp(le_arg_GetArg(0),"route2") == 0)
            {
                routeId = TAF_MNGD_AUDIO_ROUTE_2;
            } else if(strcmp(le_arg_GetArg(0),"route3") == 0)
            {
                routeId = TAF_MNGD_AUDIO_ROUTE_3;
            } else if(strcmp(le_arg_GetArg(0),"route4") == 0)
            {
                routeId = TAF_MNGD_AUDIO_ROUTE_4;
            }
            if( strcmp(le_arg_GetArg(1),"playback") == 0)
            {
                Test_Mngd_Audio_Playback(le_arg_GetArg(2));
                std::cout<<"Press s to stop Audio Playback"<<endl;
                char input_str[5];
                while(le_sem_TryWait(tafAudioAppSem)!=LE_OK)
                {
                    if( fgets(input_str,sizeof(input_str),stdin) == NULL )
                    {
                        LE_ERROR("Error reading input string");
                        return;
                    }
                    if(input_str[0]=='s')
                    {
                        std::cout<<"****Stopping audio playback***"<<endl;
                        Test_Mngd_Audio_Playback_Stop();
                        break;
                    }
                }
            } else if( strcmp(le_arg_GetArg(1),"repeated_playback") == 0)
            {
                int repeat = 0;
                int numFiles = 0;
                std::string fileName = "";

                Test_Mngd_Audio_PlayList_Setup(le_arg_GetArg(2));

                cout << "Enter the number of files: ";
                cin >> numFiles;
                for(int i = 0; i<numFiles;i++){
                    cout << "Enter the file source path: ";
                    cin >> fileName;
                    cout << "Enter the repeat count: ";
                    cin >> repeat;
                    Test_Mngd_Audio_Add_File(fileName, repeat);
                }

                Test_Mngd_Audio_Start_PlayFileList();

                std::cout<<"Press s to stop Audio Playback"<<endl;
                char input_str[5];
                while(le_sem_TryWait(tafAudioAppSem)!=LE_OK)
                {
                    if( fgets(input_str,sizeof(input_str),stdin) == NULL )
                    {
                        LE_ERROR("Error reading input string");
                        return;
                    }
                    if(input_str[0]=='s')
                    {
                        std::cout<<"****Stopping audio playback***"<<endl;
                        Test_Mngd_Audio_Playback_Stop();
                        break;
                    }
                }
                Test_Mngd_Audio_Delete_PlayList();
            } else if(strcmp(le_arg_GetArg(1),"record") == 0)
            {
                Test_Mngd_Audio_Record(le_arg_GetArg(2));
                std::cout<<"Press s to stop Audio Recording"<<endl;
                char input_str[5];
                while(le_sem_TryWait(tafAudioAppSem)!=LE_OK)
                {
                    if( fgets(input_str,sizeof(input_str),stdin) == NULL )
                    {
                        LE_ERROR("Error reading input string");
                        return;
                    }
                    if(input_str[0]=='s')
                    {
                        std::cout<<"****Stopping audio recording***"<<endl;
                        Test_Mngd_Audio_Record_Stop();
                        break;
                    }
                }
            } else if(strcmp(le_arg_GetArg(1),"voicecall") == 0)
            {
                Test_Mngd_Audio_VoiceCall();
                std::cout<<"Press s to disconnect voice call audio"<<endl;
                char input_str[5];
                while(le_sem_TryWait(tafAudioAppSem)!=LE_OK)
                {
                    if( fgets(input_str,sizeof(input_str),stdin) == NULL )
                    {
                        LE_ERROR("Error reading input string");
                        return;
                    }
                    if(input_str[0]=='s')
                    {
                        std::cout<<"****Stopping voice call audio***"<<endl;
                        Test_Mngd_Audio_VoiceCall_Stop();
                        break;
                    }
                }
            }  else if (strcmp(le_arg_GetArg(0),"node") == 0)
            {
                uint8_t nodeId;
                const char* arg1 = le_arg_GetArg(1);
                le_result_t res;
                if ( arg1 != NULL )
                {
                    nodeId = atoi(arg1);
                    const char* arg2 = le_arg_GetArg(2);
                    if (arg2 == NULL)
                    {
                        LE_ERROR("invalid argument!");
                        exit(EXIT_SUCCESS);
                    }
                    if (strcmp(arg2, "getNodeType") == 0)
                    {
                        LE_INFO("Test taf_mngd_audioHw_GetNodeType");
                        taf_mngd_audioHw_NodeType_t nodeType;
                        res = taf_mngd_audioHw_GetNodeType(nodeId, &nodeType);
                        if ( nodeType == TAF_MNGD_AUDIOHW_AUDIO_CODEC )
                        {
                            printf("Audio device type is CODEC\n");
                        } else if ( nodeType == TAF_MNGD_AUDIOHW_AUDIO_PA )
                        {
                            printf("Audio device type is PA\n");
                        } else if ( nodeType == TAF_MNGD_AUDIOHW_AUDIO_A2B )
                        {
                            printf("Audio device type is A2B\n");
                        } else {
                            printf("Audio device type is not defined\n");
                        }
                        exit(EXIT_SUCCESS);
                    } else if ( strcmp(arg2, "setNodeVendorConfig") == 0)
                    {
                        const char* configPath = le_arg_GetArg(3);
                        if( configPath == NULL )
                        {
                            LE_ERROR("Invalid configPath!");
                            exit(EXIT_SUCCESS);
                        }
                        LE_INFO("Test taf_mngd_audioHw_SendNodeConfigure");
                        res = taf_mngd_audioHw_SendNodeVendorConfig(nodeId, configPath);
                        if ( res == LE_OK )
                        {
                            printf("Successfully sent the audio device configuration to VHAL\n");
                        }
                        else
                        {
                            printf("Failed to send the configuration to VHAL\n");
                        }
                        exit(EXIT_SUCCESS);
                    } else if ( strcmp(arg2, "setPowerState") == 0)
                    {
                        const char* state = le_arg_GetArg(3);
                        if( state == NULL )
                        {
                            LE_ERROR("Invalid state!");
                            exit(EXIT_SUCCESS);
                        }
                        if ( strcmp(state, "ACTIVE") == 0 )
                        {
                            LE_INFO("Set power state to ACTIVE for node %d", nodeId);
                            res = taf_mngd_audioHw_SetNodePowerState(nodeId,
                                    TAF_MNGD_AUDIOHW_ACTIVE);
                            if( res == LE_OK )
                            {
                                printf("Successfully set the audio device power state to ACTIVE\n");
                            }
                            else
                            {
                                printf("Failed to set the device power state to ACTIVE\n");
                            }
                        } else if ( strcmp(state, "SUSPEND") == 0)
                        {
                            LE_INFO("Set power state to SUSPEND for node %d", nodeId);
                            res = taf_mngd_audioHw_SetNodePowerState(nodeId,
                                    TAF_MNGD_AUDIOHW_SUSPEND);
                            if( res == LE_OK )
                            {
                                printf("Successfully set the audio device power state to SUSPEND\n");
                            }
                            else
                            {
                                printf("Failed to set the device power state to SUSPEND\n");
                            }
                        } else if ( strcmp(state, "POWER_OFF") == 0)
                        {
                            LE_INFO("Set power state to POWER_OFF for node %d", nodeId);
                            res = taf_mngd_audioHw_SetNodePowerState(nodeId,
                                    TAF_MNGD_AUDIOHW_POWER_OFF);
                            if( res == LE_OK )
                            {
                                printf("Successfully set the node power state to POWER_OFF\n");
                            }
                            else
                            {
                                printf("Failed to set the device power state to POWER_OFF\n");
                            }
                        } else
                            printf("Invalid state");
                        exit(EXIT_SUCCESS);
                    }  else if ( strcmp(arg2, "getPowerState") == 0)
                    {
                        taf_mngd_audioHw_NodePowerState_t state;
                        LE_INFO("Get power state of node %d", nodeId);
                        res = taf_mngd_audioHw_GetNodePowerState(nodeId, &state);
                        if( res == LE_OK )
                        {
                            LE_INFO("power state is %d", state);
                            const char* pwState;
                            if ( state == TAF_MNGD_AUDIOHW_ACTIVE )
                            {
                                pwState = "ACTIVE";
                            } else if ( state == TAF_MNGD_AUDIOHW_SUSPEND )
                            {
                                pwState = "SUSPEND";
                            } else if ( state == TAF_MNGD_AUDIOHW_POWER_OFF )
                            {
                                pwState = "POWER_OFF";
                            } else {
                                pwState = "INVALID";
                            }
                            printf("Node power state is %s\n", pwState);
                        }
                        else
                        {
                            printf("Failed to get the power state\n");
                        }
                        exit(EXIT_SUCCESS);
                    }  else if ( strcmp(arg2, "setMuteState") == 0)
                    {
                        const char* isMute = le_arg_GetArg(3);
                        if( isMute == NULL )
                        {
                            LE_ERROR("Invalid configPath!");
                            exit(EXIT_SUCCESS);
                        }
                        if ( strcmp(isMute, "true") == 0 )
                        {
                            LE_INFO("Set mute to %d node device", nodeId);
                            res = taf_mngd_audioHw_SetNodeMuteState( nodeId, true );
                            if ( res == LE_OK )
                            {
                                printf("Successfully muted the node\n");
                            } else {
                                printf("Failed to mute the node\n");
                            }
                            exit(EXIT_SUCCESS);
                        } else {
                            LE_INFO("Set unmute to %d node device", nodeId);
                            res = taf_mngd_audioHw_SetNodeMuteState( nodeId, false );
                            if ( res == LE_OK )
                            {
                                printf("Successfully unmuted the node\n");
                            } else {
                                printf("Failed to unmute the node\n");
                            }
                            exit(EXIT_SUCCESS);
                        }
                    }   else if ( strcmp(arg2, "getMuteState") == 0)
                    {
                        bool isMute;
                        LE_INFO("Get mute state of node %d", nodeId);
                        res = taf_mngd_audioHw_GetNodeMuteState(nodeId, &isMute);
                        if( res == LE_OK )
                        {
                            const char* muteState;
                            muteState = isMute ? "true" : "false";
                            printf("Node mute state is %s\n", muteState);
                        }
                        else
                        {
                            printf("Failed to get the mute state\n");
                        }
                        exit(EXIT_SUCCESS);
                    } else if ( strcmp(arg2, "regsiterNodeEvent") == 0)
                    {
                        taf_mngd_audioHw_NodeStateChangeHandlerRef_t handlerRef;
                        handlerRef = taf_mngd_audioHw_AddNodeStateChangeHandler(nodeId,
                                NodeEventCallback, NULL);
                        if( handlerRef != NULL )
                        {
                            printf("Successfully registered for node event\n");
                        }
                        else
                        {
                            printf("Failed to register the node event\n");
                            exit(EXIT_FAILURE);
                        }
                        return;
                    }
                }
            } else if (strcmp(le_arg_GetArg(0), "setVendorConfig") == 0)
            {
                const char* configPath = le_arg_GetArg(1);
                if (configPath == NULL)
                {
                    LE_ERROR("Invalid configPath");
                }
                res = taf_mngd_audioHw_SendVendorConfig(configPath);
                if ( res == LE_OK )
                {
                    printf("Successfully sent the vendor configuration\n");
                } else {
                    printf("Failed to send the vendor configuration\n");
                }
                exit(EXIT_SUCCESS);
            }
        }
    } else {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }
    LE_TEST_EXIT;
}
