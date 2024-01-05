/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
static taf_audio_ConnectorRef_t audioInputConnectorRef = NULL;
static taf_audio_ConnectorRef_t audioOutputConnectorRef = NULL;
static taf_audio_MediaHandlerRef_t MediaHandlerRef = NULL;
static taf_audio_StreamRef_t FileAudioRef = NULL;

static taf_audio_StreamRef_t PlayerAudioRef = NULL;
static int AudioFileFd = -1;
static le_sem_Ref_t tafAudioAppSem;// tafAudioPBSem;
static le_thread_Ref_t Player_thread_ref;//, thread_ref;
static taf_audio_StreamRef_t speakerRef = NULL, I2sSpeakerRef = NULL, PcmSpeakerRef = NULL;
static taf_audio_StreamRef_t micRef = NULL, I2sMicRef = NULL, PcmMicRef = NULL;
static taf_audio_StreamRef_t mdmRxAudioRef = NULL, mdmTxAudioRef = NULL;

static char AudioFilePathWav[] = "/data/test.wav";
static char AudioFilePathWav1[] = "/data/test.amr";
static char AudioFilePathWav2[] = "/data/test.mp3";
le_clk_Time_t Timeout = { 3 , 0 };


/**
* Open Audio Stream for file playback.
* This function is to be used to play audio file, such as wav and amr
*/
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
    void*                         contextPtr
)
{
    switch(event)
    {
        case TAF_AUDIO_MEDIA_ENDED:
            LE_INFO("File event is TAF_AUDIO_MEDIA_ENDED.");
            LE_INFO(" Playback completed");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
            le_sem_Post(tafAudioAppSem);
            break;
        case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            le_sem_Post(tafAudioAppSem);
            break;
        default:
            LE_INFO("File event is %d", event);
            le_sem_Post(tafAudioAppSem);
            break;
    }
}

void* DisconnectAllAudio(void* ctxPtr)
{
    LE_INFO("DisconnectAllAudio");

    if(audioOutputConnectorRef)
    {
        if(PlayerAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerAudioRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, PlayerAudioRef);
        }
        if(speakerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", speakerRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, speakerRef);
        }
        if(mdmRxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", mdmRxAudioRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, mdmRxAudioRef);
        }
    }
    if(audioInputConnectorRef)
    {
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, FileAudioRef);
        }
        if(micRef)
        {
            LE_INFO("Disconnect %p from connector.%p", micRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, micRef);
        }
        if(mdmTxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", mdmTxAudioRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, mdmTxAudioRef);
        }
    }
    if(audioOutputConnectorRef)
    {
        taf_audio_DeleteConnector(audioOutputConnectorRef);
        audioOutputConnectorRef = NULL;
    }
    if(speakerRef)
    {
        taf_audio_Close(speakerRef);
        speakerRef = NULL;
    }
    if(FileAudioRef)
    {
        taf_audio_Close(FileAudioRef);
        FileAudioRef = NULL;
    }
    if(audioInputConnectorRef)
    {
        taf_audio_DeleteConnector(audioInputConnectorRef);
        audioInputConnectorRef = NULL;
    }
    if(micRef)
    {
        taf_audio_Close(micRef);
        micRef = NULL;
    }
    if(PlayerAudioRef)
    {
        taf_audio_Close(PlayerAudioRef);
        PlayerAudioRef = NULL;
    }
    le_sem_Post(tafAudioAppSem);
    return NULL;
}

void* Test_taf_audio_AddHandler(void* ctxPtr)
{
    le_sem_Ref_t sem=NULL;
    taf_audio_ConnectService();

    MediaHandlerRef = taf_audio_AddMediaHandler(PlayerAudioRef,
                                                 MyMediaEventHandler, NULL);
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

    if(audioInputConnectorRef)
    {
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, FileAudioRef);
        }
        if(micRef)
        {
            LE_INFO("Disconnect %p from connector.%p", micRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, micRef);
        }
        if(I2sMicRef)
        {
            LE_INFO("Disconnect %p from connector.%p", I2sMicRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, I2sMicRef);
        }
        if(PcmMicRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PcmMicRef,
                         audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, PcmMicRef);
        }
    }

    if(audioOutputConnectorRef)
    {
        if(PlayerAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerAudioRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, PlayerAudioRef);
        }
        if(speakerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", speakerRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, speakerRef);
        }
        if(I2sSpeakerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", I2sSpeakerRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, I2sSpeakerRef);
        }
        if(PcmSpeakerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PcmSpeakerRef,
                         audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, PcmSpeakerRef);
        }
    }
}

le_result_t CreateMediaHandler()
{
    LE_TEST_INFO("To test taf_audio_OpenPlayer!");
    PlayerAudioRef = taf_audio_OpenPlayer();
    LE_TEST_OK(PlayerAudioRef!=NULL, "taf_audio_OpenPlayer - Pass");
    Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                                         Test_taf_audio_AddHandler, NULL);
    le_thread_Start(Player_thread_ref);
    return LE_OK;
}

void Test_SamplePcmSamplingRate(taf_audio_StreamRef_t AudioRef)
{
    uint32_t SamplingRate = 32000;
    uint32_t* ratePtr = &SamplingRate;
    le_result_t res;

    if(AudioRef) {
       LE_TEST_INFO("To test taf_audio_SetSamplePcmSamplingRate!");
       res = taf_audio_SetSamplePcmSamplingRate(AudioRef , SamplingRate);
       LE_TEST_OK(res==LE_OK, "taf_audio_SetSamplePcmSamplingRate - Pass");

       LE_TEST_INFO("To test taf_audio_GetSamplePcmSamplingRate!");
       res = taf_audio_GetSamplePcmSamplingRate(AudioRef , ratePtr);
       LE_TEST_OK(res==LE_OK, "taf_audio_GetSamplePcmSamplingRate - Pass");
    }
}

le_result_t RecordAudioFile( taf_audio_StreamRef_t micRefe, taf_audio_Format_t formatTest = TAF_AUDIO_WAVE)
{
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_CreateConnector!");
    audioInputConnectorRef  = taf_audio_CreateConnector();
    LE_TEST_OK(audioInputConnectorRef !=NULL, "taf_audio_CreateConnector - Pass");

    LE_TEST_INFO("To test taf_audio_OpenRecorder!");
    FileAudioRef = taf_audio_OpenRecorder();
    LE_TEST_OK(FileAudioRef!=NULL, "taf_audio_OpenRecorder - Pass");

    Test_SamplePcmSamplingRate(FileAudioRef);

    if (micRefe && FileAudioRef && audioInputConnectorRef )
    {
        LE_TEST_INFO("To test Connect Mic on Input connector!");
        res = taf_audio_Connect(audioInputConnectorRef, micRefe);
        LE_TEST_OK(res==LE_OK, "Connect Mic on Input connector - Pass");

        LE_TEST_INFO("To test Connect FileRecorder on input connector!");
        res = taf_audio_Connect(audioInputConnectorRef , FileAudioRef);
        LE_TEST_OK(res==LE_OK, "Connect FileRecorder on input connector! - Pass");

        if ((AudioFileFd=open(AudioFilePathWav, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) == -1)
        {
            LE_INFO("Open file %s failure: errno.%d (%s)",\
                      AudioFilePathWav, errno, LE_ERRNO_TXT(errno));
            LE_ERROR("Open file %s failure: errno.%d (%s)",\
                      AudioFilePathWav, errno, LE_ERRNO_TXT(errno));
            le_event_QueueFunctionToThread(
                                            Player_thread_ref,
                                            (le_event_DeferredFunc_t) DisconnectAllAudio,
                                            NULL, NULL
                                          );
            return LE_FAULT;
        }
        else
        {
            LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePathWav, AudioFileFd);
            LE_TEST_INFO("To test taf_audio_SetEncodingFormat");
            res = taf_audio_SetEncodingFormat(FileAudioRef, formatTest);
            LE_TEST_OK(res==LE_OK, "taf_audio_SetEncodingFormat - Pass");

            LE_TEST_INFO("To test taf_audio_GetEncodingFormat");
            taf_audio_Format_t format;
            res = taf_audio_GetEncodingFormat(FileAudioRef, &format);
            LE_TEST_OK(res==LE_OK, "taf_audio_GetEncodingFormat - Pass");
            LE_INFO("Encoding format is %s", format == TAF_AUDIO_WAVE ? "wave" : "other");

            LE_TEST_INFO("To test taf_audio_SetSamplePcmChannelNumber");
            res = taf_audio_SetSamplePcmChannelNumber(FileAudioRef, 2);
            LE_TEST_OK(res==LE_OK, "taf_audio_SetSamplePcmChannelNumber - Pass");

            LE_TEST_INFO("To test taf_audio_GetSamplePcmChannelNumber");
            uint32_t channelNum;
            res = taf_audio_GetSamplePcmChannelNumber(FileAudioRef, &channelNum);
            LE_TEST_OK(res==LE_OK, "taf_audio_GetSamplePcmChannelNumber - Pass");
            LE_INFO("channel num is %d", channelNum);

            LE_TEST_INFO("To test taf_audio_RecordFile");
            res = taf_audio_RecordFile(FileAudioRef, AudioFileFd);
            if (res != LE_OK)
            {
                LE_ERROR("Failed to record the file %s", AudioFilePathWav);
                return LE_FAULT;
            }
            else
            {
                LE_TEST_OK(res==LE_OK, "taf_audio_RecordFile - Pass");
                LE_INFO(" Recording started %s",AudioFilePathWav);
                le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

                LE_TEST_INFO("To test taf_audio_Mute");
                res = taf_audio_Mute(PlayerAudioRef);
                LE_TEST_OK(res==LE_OK, "taf_audio_Mute - Pass");
                le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);

                LE_TEST_INFO("To test taf_audio_Unmute");
                res = taf_audio_Unmute(PlayerAudioRef);
                LE_TEST_OK(res==LE_OK, "taf_audio_Unmute - Pass");

                LE_TEST_INFO("****Stopping audio recording***");
                LE_TEST_OK((taf_audio_Stop(FileAudioRef)==LE_OK), "****taf_audio_Stop - Pass");
                close(AudioFileFd);
            }
        }
    }
    return LE_OK;
}

void Test_RecordAudioFile()
{
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_OpenMic!");
    micRef = taf_audio_OpenMic();
    LE_TEST_OK((micRef!=NULL), "taf_audio_OpenMic - Pass");
    if(micRef)
    {
        LE_TEST_INFO("To Test_Record Feature with MIC");
        res = RecordAudioFile(micRef);
        LE_TEST_OK(res==LE_OK, "Test_RecordAudioFile with MIC - Pass");
        disconnect();
    }

    LE_TEST_INFO("To test taf_audio_OpenI2sRx!");
    I2sMicRef = taf_audio_OpenI2sRx(TAF_AUDIO_I2S_STEREO);
    LE_TEST_OK((I2sMicRef!=NULL), "taf_audio_OpenI2sRx - Pass");
    if(I2sMicRef)
    {
        LE_TEST_INFO("To Test_Record Feature with I2sMic");
        res = RecordAudioFile(I2sMicRef);
        LE_TEST_OK(res==LE_OK, "Test_RecordAudioFile with I2sMic - Pass");
        disconnect();
    }

    LE_TEST_INFO("To test taf_audio_OpenPcmRx!");
    PcmMicRef = taf_audio_OpenPcmRx(0);
    LE_TEST_OK((PcmMicRef!=NULL), "taf_audio_OpenPcmRx - Pass");
    if(PcmMicRef)
    {
        LE_TEST_INFO("To Test_Record Feature with PcmMic");
        res = RecordAudioFile(PcmMicRef);
        LE_TEST_OK(res==LE_OK, "Test_RecordAudioFile with PcmMic - Pass");
        disconnect();
    }

    LE_TEST_INFO("To test taf_audio_OpenPcmRx!");
    PcmMicRef = taf_audio_OpenPcmRx(0);
    LE_TEST_OK((PcmMicRef!=NULL), "taf_audio_OpenPcmRx - Pass");
    if(PcmMicRef)
    {
        LE_TEST_INFO("Failure case Test_Record Feature with PcmMic");
        res = RecordAudioFile(PcmMicRef, TAF_AUDIO_AMR);
        LE_TEST_OK(res!=LE_OK, "Failure case Test_RecordAudioFile with PcmMic - Pass");
        disconnect();
    }
}

void Test_Volume(taf_audio_StreamRef_t AudioRef)
{
    int32_t gain = 100;
    int32_t *Gain =&gain ;
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_SetGain!");
    res = taf_audio_SetGain(AudioRef, gain);
    LE_TEST_OK(res==LE_OK, "taf_audio_SetGain - Pass");

    LE_TEST_INFO("To test taf_audio_GetGain!");
    if(Gain != nullptr)
    res = taf_audio_GetGain(AudioRef, Gain);
    LE_TEST_OK(res==LE_OK, "taf_audio_GetGain - Pass");

}

le_result_t PlayAudioFile( taf_audio_StreamRef_t speakerRefe, char AudioFilePath[])
{
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_CreateConnector!");
    audioOutputConnectorRef  = taf_audio_CreateConnector();
    LE_TEST_OK(audioOutputConnectorRef !=NULL, "taf_audio_CreateConnector - Pass");

    LE_TEST_INFO("To test taf_audio_OpenPlayer!");
    PlayerAudioRef = taf_audio_OpenPlayer();
    LE_TEST_OK(PlayerAudioRef!=NULL, "taf_audio_OpenPlayer - Pass");

    Test_SamplePcmSamplingRate(PlayerAudioRef);

    if (speakerRefe && PlayerAudioRef && audioOutputConnectorRef )
    {
        LE_TEST_INFO("To connect Speaker on Output connector");
        res = taf_audio_Connect(audioOutputConnectorRef, speakerRefe);
        LE_TEST_OK(res==LE_OK, "Connect Speaker on Output connector - Pass");

        LE_TEST_INFO("To connect FilePlayback on output connector!");
        res = taf_audio_Connect(audioOutputConnectorRef , PlayerAudioRef);
        LE_TEST_OK(res==LE_OK, "Connect FilePlayback on output connector! - Pass");

        if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
        {
            LE_INFO("Open file %s failure: errno.%d (%s)",\
                      AudioFilePath, errno, LE_ERRNO_TXT(errno));
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
            LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
            LE_TEST_INFO("To test taf_audio_PlayFile!");
            res = taf_audio_PlayFile(PlayerAudioRef, AudioFileFd);
            if (res != LE_OK)
            {
                LE_ERROR("Failed to play the file");
                return LE_FAULT;
            }
            else
            {
                LE_TEST_OK(res==LE_OK, "taf_audio_PlayFile - Pass");
                LE_INFO(" Playing started:%s",AudioFilePath);
                le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
                res = taf_audio_Stop(PlayerAudioRef);
                le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
                LE_TEST_INFO("****Stopping audio playing***");
                LE_TEST_OK(res==LE_OK, "***taf_audio_Stop - Pass");
                close(AudioFileFd);
            }
        }
    }
    return LE_OK;
}

void Test_PlayAudioFile()
{
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_OpenSpeaker!");
    speakerRef = taf_audio_OpenSpeaker();
    LE_TEST_OK((speakerRef!=NULL), "taf_audio_OpenSpeaker - Pass");

    if(speakerRef)
    {
        LE_TEST_INFO("To test PlayAudioFile with speakerRef!");
        res = PlayAudioFile(speakerRef, AudioFilePathWav);
        LE_TEST_OK(res==LE_OK, "Test_PlayAudioFile with speakerRef - Pass");
        DisconnectAllAudio(NULL);
    }

    LE_TEST_INFO("To test taf_audio_OpenI2sTx!");
    I2sSpeakerRef = taf_audio_OpenI2sTx(TAF_AUDIO_I2S_STEREO);
    LE_TEST_OK((I2sSpeakerRef!=NULL), "taf_audio_OpenI2sTx - Pass");
    if(I2sSpeakerRef)
    {
        LE_TEST_INFO("To test PlayAudioFile with I2sSpeakerRef!");
        res = PlayAudioFile(I2sSpeakerRef, AudioFilePathWav1);
        LE_TEST_OK(res==LE_OK, "Test_PlayAudioFile with I2sSpeakerRef - Pass");
        DisconnectAllAudio(NULL);
    }

    LE_TEST_INFO("To test taf_audio_OpenPcmTx!");
    PcmSpeakerRef = taf_audio_OpenPcmTx(0);
    LE_TEST_OK((PcmSpeakerRef!=NULL), "taf_audio_OpenPcmTx - Pass");
    if(PcmSpeakerRef)
    {
        LE_TEST_INFO("To test PlayAudioFile with PcmSpeakerRef!");
        res = PlayAudioFile(PcmSpeakerRef, AudioFilePathWav2);
        LE_TEST_OK(res==LE_OK, "Test_PlayAudioFile with PcmSpeakerRef - Pass");
        DisconnectAllAudio(NULL);
    }
}

void Test_NoiseSuppressorAndEchoCanceller(taf_audio_StreamRef_t AudioRef)
{
    bool status = true;
    bool *statusptr = &status;
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_EnableNoiseSuppressor!");
    res = taf_audio_EnableNoiseSuppressor(AudioRef);
    LE_TEST_OK(res==LE_OK, "taf_audio_EnableNoiseSuppressor - Pass");

    LE_TEST_INFO("To test taf_audio_EnableEchoCanceller!");
    res = taf_audio_EnableEchoCanceller(AudioRef);
    LE_TEST_OK(res==LE_OK, "taf_audio_EnableEchoCanceller - Pass");

    LE_TEST_INFO("To test taf_audio_IsNoiseSuppressorEnabled!");
    res = taf_audio_IsNoiseSuppressorEnabled(AudioRef, statusptr);
    LE_TEST_OK(res==LE_OK, "taf_audio_IsNoiseSuppressorEnabled - Pass");

    LE_TEST_INFO("To test taf_audio_IsEchoCancellerEnabled!");
    res = taf_audio_IsEchoCancellerEnabled(AudioRef, statusptr);
    LE_TEST_OK(res==LE_OK, "taf_audio_IsEchoCancellerEnabled - Pass");

    LE_TEST_INFO("To test taf_audio_DisableNoiseSuppressor!");
    res = taf_audio_DisableNoiseSuppressor(AudioRef);
    LE_TEST_OK(res==LE_OK, "taf_audio_DisableNoiseSuppressor - Pass");

    LE_TEST_INFO("To test taf_audio_DisableEchoCanceller!");
    res = taf_audio_DisableEchoCanceller(AudioRef);
    LE_TEST_OK(res==LE_OK, "taf_audio_DisableEchoCanceller - Pass");
}

void Test_Dtmf()
{
    static const char*  DtmfString = "5";
    static uint32_t     Duration = 5;
    static uint32_t     Pause = 1;
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_OpenModemVoiceRx!");
    mdmRxAudioRef = taf_audio_OpenModemVoiceRx(1); // SlotId input param
    LE_TEST_OK(mdmRxAudioRef!=NULL, "taf_audio_OpenModemVoiceRx - Pass");

    taf_audio_AddDtmfDetectorHandler(mdmRxAudioRef, MyDtmfDetectorHandler, NULL);

    // Redirect audio to Speaker.
    LE_TEST_INFO("To test taf_audio_OpenSpeaker!");
    speakerRef = taf_audio_OpenSpeaker();
    LE_TEST_OK((speakerRef!=NULL), "taf_audio_OpenSpeaker - Pass");

    LE_TEST_INFO("To test taf_audio_CreateConnector!");
    audioOutputConnectorRef = taf_audio_CreateConnector();
    LE_TEST_OK((audioOutputConnectorRef!=NULL), "taf_audio_CreateConnector - Pass");

    LE_TEST_INFO("To Connect Speaker on Output connector!");
    res = taf_audio_Connect(audioOutputConnectorRef, speakerRef);
    LE_TEST_OK((res == LE_OK), "Connect Speaker on Output connector - Pass");

    // Play DTMF on output connector.
    LE_TEST_INFO("To test taf_audio_OpenPlayer!");
    PlayerAudioRef = taf_audio_OpenPlayer();
    LE_TEST_OK((PlayerAudioRef != NULL), "taf_audio_OpenPlayer - Pass");

    if (PlayerAudioRef && audioOutputConnectorRef)
    {
        LE_TEST_INFO("To test connect Player on output connector!");
        res = taf_audio_Connect(audioOutputConnectorRef, PlayerAudioRef);
        LE_TEST_OK((res == LE_OK), "Connect Player on output connector - Pass");

        LE_TEST_INFO("To test taf_audio_PlayDtmf on PlayerAudioRef.%p", PlayerAudioRef);
        res = taf_audio_PlayDtmf(PlayerAudioRef, DtmfString, Duration, Pause);
        LE_TEST_OK((res == LE_OK), "taf_audio_PlayDtmf - Pass");
    }
    disconnect();
}

static void Test_VoiceCallAudio(taf_audio_StreamRef_t SpeakerRef , taf_audio_StreamRef_t MicRef)
{
    le_result_t res;

    LE_TEST_INFO("To test taf_audio_OpenModemVoiceRx!");
    mdmRxAudioRef = taf_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(mdmRxAudioRef!=NULL, "taf_audio_OpenModemVoiceRx - Pass");

    LE_TEST_INFO("To test taf_audio_CreateConnector!");
    audioOutputConnectorRef  = taf_audio_CreateConnector();
    LE_TEST_OK(audioOutputConnectorRef !=NULL, "taf_audio_CreateConnector - Pass");

#if LE_CONFIG_TARGET_SA525M
    LE_INFO("SA525M only");
    LE_TEST_INFO("To test taf_audio_OpenModemVoiceTx!");
    mdmTxAudioRef =  taf_audio_OpenModemVoiceTx(1);
    LE_TEST_OK(mdmTxAudioRef!=NULL, "taf_audio_OpenModemVoiceTx - Pass");

    LE_TEST_INFO("To test taf_audio_CreateConnector!");
    audioInputConnectorRef  = taf_audio_CreateConnector();
    LE_TEST_OK(audioInputConnectorRef !=NULL, "taf_audio_CreateConnector - Pass");

    Test_NoiseSuppressorAndEchoCanceller(MicRef);

    if (mdmTxAudioRef && MicRef && audioInputConnectorRef)
    {
        LE_TEST_INFO("To test Connect Mic on Input connector!");
        res = taf_audio_Connect(audioInputConnectorRef, MicRef);
        LE_TEST_OK(res==LE_OK, "Connect Mic on Input connector - Pass");

        LE_TEST_INFO("To test connect mdmTx on Input connector!");
        res = taf_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_TEST_OK(res==LE_OK, "Connect mdmTx on Input connector! - Pass");

    }
#endif
    Test_NoiseSuppressorAndEchoCanceller(SpeakerRef);
    if (mdmRxAudioRef && SpeakerRef  && audioOutputConnectorRef)
    {
        LE_INFO("SA515M only");
        LE_TEST_INFO("To connect Speaker on Output connector");
        res = taf_audio_Connect(audioOutputConnectorRef, SpeakerRef);
        LE_TEST_OK(res==LE_OK, "Connect Speaker on Output connector - Pass");

        LE_TEST_INFO("To connect mdmRx on Output connector!");
        res = taf_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_TEST_OK(res==LE_OK, "Connect mdmRx on Output connector! - Pass");

    }
    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
    Test_Volume(SpeakerRef);
    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
    Test_Dtmf();

    LE_INFO("Test_ConnectAudioToMicAndSpeaker executed successfully");
}

static void VoiceCallAudioTest()
{
    // Redirect audio to Speaker and Mic.
    LE_TEST_INFO("To test taf_audio_OpenSpeaker!");
    speakerRef = taf_audio_OpenSpeaker();
    LE_TEST_OK((speakerRef!=NULL), "taf_audio_OpenSpeaker - Pass");

    LE_TEST_INFO("To test taf_audio_OpenMic!");
    micRef = taf_audio_OpenMic();
    LE_TEST_OK((micRef!=NULL), "taf_audio_OpenMic - Pass");

    if(speakerRef && micRef)
    Test_VoiceCallAudio(speakerRef ,micRef);
    DisconnectAllAudio(NULL);

    // Redirect audio to I2sSpeaker and I2sMic.
    LE_TEST_INFO("To test taf_audio_OpenI2sTx!");
    I2sSpeakerRef = taf_audio_OpenI2sTx(TAF_AUDIO_I2S_STEREO);
    LE_TEST_OK((I2sSpeakerRef!=NULL), "taf_audio_OpenI2sTx - Pass");

    LE_TEST_INFO("To test taf_audio_OpenI2sRx!");
    I2sMicRef = taf_audio_OpenI2sRx(TAF_AUDIO_I2S_STEREO);
    LE_TEST_OK((I2sMicRef!=NULL), "taf_audio_OpenI2sRx - Pass");

    if(I2sSpeakerRef && I2sMicRef)
    Test_VoiceCallAudio(I2sSpeakerRef ,I2sMicRef);
    DisconnectAllAudio(NULL);

    // Redirect audio to PcmSpeaker and PcmMic.
    LE_TEST_INFO("To test taf_audio_OpenPcmTx!");
    PcmSpeakerRef = taf_audio_OpenPcmTx(0);
    LE_TEST_OK((PcmSpeakerRef!=NULL), "taf_audio_OpenPcmTx - Pass");

    LE_TEST_INFO("To test taf_audio_OpenPcmRx!");
    PcmMicRef = taf_audio_OpenPcmRx(0);
    LE_TEST_OK((PcmMicRef!=NULL), "taf_audio_OpenPcmRx - Pass");
    if(PcmSpeakerRef && PcmMicRef)
    Test_VoiceCallAudio(PcmSpeakerRef ,PcmMicRef);
    DisconnectAllAudio(NULL);
}

COMPONENT_INIT
{
    tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    CreateMediaHandler();
    le_sem_Wait(tafAudioAppSem);
    Test_RecordAudioFile();
    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
    Test_PlayAudioFile();
    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
    VoiceCallAudioTest();
    le_sem_WaitWithTimeOut(tafAudioAppSem ,Timeout);
    disconnect();
    le_result_t result = le_thread_Cancel(Player_thread_ref);
    LE_TEST_OK(result == LE_OK, "Test_taf_audio_RemoveHandler done");
    le_sem_Delete(tafAudioAppSem);
    LE_INFO("tafAudioUnitTests are executed successfully");
    printf("\nDone: tafAudioUnitTests executed successfully!!!\n\n");
    taf_audio_DisconnectService();
    LE_TEST_EXIT;
}
