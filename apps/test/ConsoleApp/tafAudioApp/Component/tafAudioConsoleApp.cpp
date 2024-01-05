/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
* Audio safe references
*/
static taf_audio_StreamRef_t            OutRef = NULL;
static taf_audio_ConnectorRef_t         AudioOutputConnectorRef = NULL;
static taf_audio_MediaHandlerRef_t      MediaHandlerRef = NULL;
static taf_audio_StreamRef_t            FileAudioRef = NULL;
static char                             AudioFilePath[40] = "/data/sample.wav";
static int                              AudioFileFd = -1;
static le_sem_Ref_t                     tafAudioAppSem;
static le_thread_Ref_t                  Player_thread_ref;

/**
* Open Audio Stream for file playback.
* This function is to be used to play audio file, such as wav and amr
*/

static void MyMediaEventHandler
(
    taf_audio_StreamRef_t          streamRef,
    taf_audio_MediaEvent_t         event,
    void*                         contextPtr
)
{
    switch(event)
    {
        case TAF_AUDIO_MEDIA_ENDED:
            LE_TEST_INFO("File event is TAF_AUDIO_MEDIA_ENDED.");
            std::cout <<AudioFilePath << " Playback completed" << endl;
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_TEST_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_TEST_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            le_sem_Post(tafAudioAppSem);
            break;
        default:
            LE_TEST_INFO("File event is %d", event);
            le_sem_Post(tafAudioAppSem);
            break;
    }
}

void* DisconnectAllAudio(void* ctxPtr)
{
    LE_INFO("DisconnectAllAudio");

    if(AudioOutputConnectorRef)
    {
        if(FileAudioRef)
        {
            LE_TEST_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioOutputConnectorRef);
            taf_audio_Disconnect(AudioOutputConnectorRef, FileAudioRef);
        }
        if(OutRef)
        {
            LE_TEST_INFO("Disconnect %p from connector.%p", OutRef, AudioOutputConnectorRef);
            taf_audio_Disconnect(AudioOutputConnectorRef, OutRef);
        }
    }
    if(AudioOutputConnectorRef)
    {
        taf_audio_DeleteConnector(AudioOutputConnectorRef);
        AudioOutputConnectorRef = NULL;
    }
    if(OutRef)
    {
        taf_audio_Close(OutRef);
        OutRef = NULL;
    }
    if(FileAudioRef)
    {
        taf_audio_Close(FileAudioRef);
        FileAudioRef = NULL;
    }
    le_sem_Post(tafAudioAppSem);
    return NULL;
}

void* Test_taf_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audio_ConnectService();

    MediaHandlerRef = taf_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);
    sem=le_sem_FindSemaphore("tafAudioAppSem");
    if(sem!=NULL)
    {
        le_sem_Post(sem);
    }
    else
    {
        LE_ERROR_IF((sem==NULL), "tafAudioAppSem is NULL!");
    }
    le_event_RunLoop();
    return NULL;
}

void disconnect()
{
    LE_INFO("taf_audio_Disconnect");

    if(AudioOutputConnectorRef)
    {
        if(FileAudioRef)
        {
            LE_TEST_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioOutputConnectorRef);
            taf_audio_Disconnect(AudioOutputConnectorRef, FileAudioRef);
        }
        if(OutRef)
        {
            LE_TEST_INFO("Disconnect %p from connector.%p", OutRef, AudioOutputConnectorRef);
            taf_audio_Disconnect(AudioOutputConnectorRef, OutRef);
        }
    }
}

le_result_t CreateMediaHandler()
{
    OutRef = taf_audio_OpenSpeaker();
    if(OutRef==NULL)
    {
        std::cout<<"taf_audio_OpenSpeaker returns NULL!"<<endl;
    }
    else
    {
        LE_TEST_OK((OutRef!=NULL), "taf_audio_OpenSpeaker - Pass");
    }
    LE_ERROR_IF((OutRef==NULL), "taf_audio_OpenSpeaker returns NULL!");
    AudioOutputConnectorRef  = taf_audio_CreateConnector();
    if(AudioOutputConnectorRef ==NULL)
    {
        std::cout<<"AudioOutputConnectorRef  is NULL!"<<endl;
    }
    else
    {
        LE_TEST_OK(AudioOutputConnectorRef !=NULL, "taf_audio_CreateConnector - Pass");
    }
    LE_ERROR_IF((AudioOutputConnectorRef ==NULL), "AudioOutputConnectorRef  is NULL!");
    FileAudioRef = taf_audio_OpenPlayer();
    if(FileAudioRef==NULL)
    {
        std::cout<<"OpenFilePlayback returns NULL!"<<endl;
    }
    else
    {
        LE_TEST_OK(FileAudioRef!=NULL, "taf_audio_OpenPlayer - Pass");
    }
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread", Test_taf_audio_AddHandler, NULL);
    le_thread_Start(Player_thread_ref);
    return LE_OK;
}

le_result_t OpenAudioFile()
{
    le_result_t res;

    if (OutRef && FileAudioRef && AudioOutputConnectorRef )
    {
        res = taf_audio_Connect(AudioOutputConnectorRef, OutRef);
        if(res!=LE_OK)
        std::cout<<"Failed to connect Speaker on Output connector (res "
                 <<LE_RESULT_TXT(res)<<")!"<<endl;
        else
        LE_TEST_OK(res==LE_OK, "taf_audio_Connect - Pass");
        LE_ERROR_IF((res!=LE_OK),
            "Failed to connect Speaker on Output connector (res %s)!\n", LE_RESULT_TXT(res));
        res = taf_audio_Connect(AudioOutputConnectorRef , FileAudioRef);
        if(res!=LE_OK)
        std::cout<<"Failed to connect FilePlayback on input connector!"<<endl;
        else
        LE_TEST_OK(res==LE_OK, "taf_audio_Connect - Pass");
        LE_ERROR_IF((res!=LE_OK), "Failed to connect FilePlayback on input connector!");
        if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
        {
            std::cout<<"Open file "<<AudioFilePath<<" failure: errno."<<errno
                     <<" ("<<LE_ERRNO_TXT(errno)<<")"<<endl;
            LE_ERROR("Open file %s failure: errno.%d (%s)",\
                      AudioFilePath, errno, LE_ERRNO_TXT(errno));
            le_event_QueueFunctionToThread(
                                            Player_thread_ref,
                                            (le_event_DeferredFunc_t) DisconnectAllAudio,
                                            NULL, NULL
                                          );
            return LE_FAULT;
        }
        else
        {
            LE_TEST_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
            res = taf_audio_PlayFile(FileAudioRef, AudioFileFd);
        if (res != LE_OK)
            {
                std::cout<<"Failed to play the file: "<<AudioFilePath<<endl;
                LE_ERROR("Failed to play the file");
                return LE_FAULT;
            }
            else
            {
                std::cout<<AudioFilePath<<" Playback started"<<endl;
                LE_INFO("file is now playing.");
            }
        }
    }
    return LE_OK;
}

COMPONENT_INIT
{
    const char* fileName = "";
    int index,size;
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
    if (NumberOfArgs >= 1) {
        if(strcmp(le_arg_GetArg(0),"help") == 0)
        {
            std::cout<<endl<<"Usage details:"<<endl;
            std::cout<<"**************"<<endl;
            std::cout<<"app runProc tafAudioConsoleApp tafAudioConsoleApp -- <File Name>"<<endl;
            std::cout<<"Ex: app runProc tafAudioConsoleApp tafAudioConsoleApp -- record.wav"<<endl;
            std::cout<<"Note: By default sample.wav from /data is played"<<endl<<endl;
        }
        else
        {
            fileName = le_arg_GetArg(0);
            if (NULL == fileName) {
                LE_ERROR("fileName is NULL");
                exit(EXIT_FAILURE);
            }
            else
            {
                char audioFile[40] = "/data/";
                size=0;
                while(audioFile[size]!='\0')
                {
                    size++;
                }
                index=0;
                while(fileName[index]!='\0')
                {
                    audioFile[size]=fileName[index];
                    size++;
                    index++;
                }
                audioFile[size]='\0';
                index=0;
                while(audioFile[index]!='\0')
                {
                    AudioFilePath[index] = audioFile[index];
                    index++;
                }
                AudioFilePath[index]='\0';
                tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
                CreateMediaHandler();
                le_sem_Wait(tafAudioAppSem);
                OpenAudioFile();
                le_sem_Wait(tafAudioAppSem);
                disconnect();
            }
        }
    }
    else //if no arguements passed in the command line argument
    {
        LE_ERROR("NumberOfArgs: %d", NumberOfArgs);

        tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
        CreateMediaHandler();
        le_sem_Wait(tafAudioAppSem);

        OpenAudioFile();
        le_sem_Wait(tafAudioAppSem);
        disconnect();
    }

    exit(0);
}
