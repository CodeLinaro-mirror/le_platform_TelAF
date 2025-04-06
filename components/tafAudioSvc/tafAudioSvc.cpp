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
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafAudio.hpp"
#include "tafAudioVhal.hpp"

using namespace tafsvc;
using namespace taf::audioVhal;

/**
* FUNCTION     : CreateConnector
* DESCRIPTION  : Creates the connector for given I/O
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Connector reference
*/
taf_audio_ConnectorRef_t taf_audio_CreateConnector
(
    void
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.CreateConnector();
}

/**
* FUNCTION     : DeleteConnector
* DESCRIPTION  : Deletes the connctor path
* DEPENDECY    :
* PARAMETERS   : Connector reference
* RETURN VALUES:
*/
void taf_audio_DeleteConnector
(
 taf_audio_ConnectorRef_t connectorRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.DeleteConnector(connectorRef);
}

/**
* FUNCTION     : Connect
* DESCRIPTION  : Connect the stream and Connector
* DEPENDECY    :
* PARAMETERS   : Connector and Stream
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_audio_Connect
(
 taf_audio_ConnectorRef_t connectorRef,
 taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.Connect(connectorRef, streamRef);
}

/**
* FUNCTION     : Disconnect
* DESCRIPTION  : Disconnects the connector and stream
* DEPENDECY    :
* PARAMETERS   : Connector and Stream
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
void taf_audio_Disconnect
(
 taf_audio_ConnectorRef_t connectorRef,
 taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    audio.Disconnect(connectorRef, streamRef);
}

/**
* FUNCTION     : Close
* DESCRIPTION  : Close stream reference
* DEPENDECY    :
* PARAMETERS   : Audio stream
* RETURN VALUES:
*/
void taf_audio_Close
(
 taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    audio.Close(streamRef);
}

/**
* FUNCTION     : OpenModemVoiceRx
* DESCRIPTION  : Gets the reference of outStream
* DEPENDECY    :
* PARAMETERS   : SlotId
* RETURN VALUES: Reference of OutStream, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenModemVoiceRx
(
    uint32_t slotId
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenModemVoiceRx(slotId);
}

/**
* FUNCTION     : OpenModemVoiceTx
* DESCRIPTION  : Gets the reference of VoiceTx Path
* DEPENDECY    :
* PARAMETERS   : SlotId, ECNR configuration
* RETURN VALUES: Reference of a Stream, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenModemVoiceTx
(
    uint32_t slotId, bool enableEcnr
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenModemVoiceTx(slotId, enableEcnr);
}

/**
* FUNCTION     : OpenRoute
* DESCRIPTION  : Opens the audio route
* DEPENDECY    :
* PARAMETERS   : route, mode, sinkRef, sourceRef
* RETURN VALUES: LE_OK on success, LE_BUSY if another route is opened,
*                LE_BAD_PARAMETER on bad params, LE_FAULT on error.
*/
taf_audio_RouteRef_t taf_audio_OpenRoute
(
    taf_audio_RouteId_t route,
    taf_audio_Mode_t mode,
    taf_audio_StreamRef_t *sinkRef,
    taf_audio_StreamRef_t *sourceRef
)
{
    auto &audio = taf_Audio::GetInstance();
    TAF_ERROR_IF_RET_VAL(mode != TAF_AUDIO_LOCAL_LOOPBACK
            && (sinkRef == NULL || sourceRef == NULL), NULL,
            "sinkRef or sourceRef pointer is NULL");
    return audio.OpenRoute(route, mode, sinkRef, sourceRef);
}

/**
* FUNCTION     : CloseRoute
* DESCRIPTION  : Closes the audio route
* DEPENDECY    :
* PARAMETERS   : Route reference created on OpenRoute
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER if route is not opened, LE_FAULT on error.
*/
le_result_t taf_audio_CloseRoute
(
    taf_audio_RouteRef_t routeRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.CloseRoute(routeRef);
}

/**
* FUNCTION     : OpenPlayer
* DESCRIPTION  : Opens the stream for player
* DEPENDECY    :
* PARAMETERS   : direction
* RETURN VALUES: Stream reference on success and null on failure.
*/
taf_audio_StreamRef_t taf_audio_OpenPlayer
(
    taf_audio_Direction_t direction
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenPlayer(direction);
}

/**
* FUNCTION     : PlayFile
* DESCRIPTION  : Plays the audio file
* DEPENDECY    :
* PARAMETERS   : Player stream reference and audio file path
* RETURN VALUES: LE_OK on success and LE_FAULT on failure.
*/
le_result_t taf_audio_PlayFile
(
    taf_audio_StreamRef_t streamRef,
    const char *srcPath
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.PlayFile(streamRef, srcPath);
}

/**
* FUNCTION     : OpenRecorder
* DESCRIPTION  : Opens the stream for recorder
* DEPENDECY    :
* PARAMETERS   : direction
* RETURN VALUES: Stream reference on success and null on failure.
*/
taf_audio_StreamRef_t taf_audio_OpenRecorder
(
    taf_audio_Direction_t direction
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenRecorder(direction);
}

/**
* FUNCTION     : RecordFile
* DESCRIPTION  : Records the audio file
* DEPENDECY    :
* PARAMETERS   : Recorder stream reference and audio file path
* RETURN VALUES: LE_OK on success and LE_FAULT on failure.
*/
le_result_t taf_audio_RecordFile
(
    taf_audio_StreamRef_t streamRef,
    const char *srcPath
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.RecordFile(streamRef, srcPath);
}

/**
* FUNCTION     : Stop
* DESCRIPTION  : Stops the active playback/record
* DEPENDECY    :
* PARAMETERS   : Player/recorder stream reference
* RETURN VALUES: LE_OK on success and LE_FAULT on failure.
*/
le_result_t taf_audio_Stop
(
    taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.Stop(streamRef);
}

taf_audio_MediaHandlerRef_t taf_audio_AddMediaHandler
(
   taf_audio_StreamRef_t streamRef,
   taf_audio_MediaHandlerFunc_t handlerPtr,
   void* contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.AddMediaHandler(streamRef, handlerPtr, contextPtr);
}

void taf_audio_RemoveMediaHandler
(
   taf_audio_MediaHandlerRef_t handlerRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.RemoveMediaHandler(handlerRef);
}

le_result_t taf_audioVendor_GetNodeType
(
    uint8_t audioNodeId,
    taf_audioVendor_NodeType_t *nodeType
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.GetNodeType(audioNodeId, nodeType);
}

le_result_t taf_audioVendor_SendNodeVendorConfig
(
    uint8_t audioNodeId,
    const char* configPath
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.SendNodeVendorConfig(audioNodeId, configPath);
}

le_result_t taf_audioVendor_SendVendorConfig
(
    const char* configPath
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.SendVendorConfig(configPath);
}

le_result_t taf_audioVendor_SetNodePowerState
(
    uint8_t audioNodeId,
    taf_audioVendor_NodePowerState_t state
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.SetNodePowerState(audioNodeId, state);
}

le_result_t taf_audioVendor_GetNodePowerState
(
    uint8_t audioNodeId,
    taf_audioVendor_NodePowerState_t *state
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.GetNodePowerState(audioNodeId, state);
}

le_result_t taf_audioVendor_SetNodeMuteState
(
    uint8_t audioNodeId,
    bool mute
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.SetNodeMuteState(audioNodeId, mute);
}

le_result_t taf_audioVendor_GetNodeMuteState
(
    uint8_t audioNodeId,
    bool *isMuted
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");
    return audioVhal.GetNodeMuteState(audioNodeId, isMuted);
}

le_result_t taf_audioVendor_SetNodeGain
(
    uint8_t nodeId,
    taf_audioVendor_Direction_t direction,
    double gain
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");

    TAF_ERROR_IF_RET_VAL( gain < 0 || gain > 1, LE_BAD_PARAMETER, "Invalid gain level");

    taf_audioVendor_NodeType_t nodeType = TAF_AUDIOVENDOR_INVALID;
    taf_audioVendor_GetNodeType(nodeId, &nodeType);
    TAF_ERROR_IF_RET_VAL(nodeType == TAF_AUDIOVENDOR_INVALID, LE_BAD_PARAMETER, "Invalid node ID");

    if(direction == TAF_AUDIOVENDOR_RX){
        if(nodeType == TAF_AUDIOVENDOR_AUDIO_A2B){
            return LE_UNSUPPORTED;
        }
    } else {
        if(nodeType == TAF_AUDIOVENDOR_AUDIO_A2B || nodeType == TAF_AUDIOVENDOR_AUDIO_PA){
            return LE_UNSUPPORTED;
        }
    }

    return audioVhal.SetNodeGain(nodeId, direction, gain);
}

le_result_t taf_audioVendor_GetNodeGain
(
    uint8_t nodeId,
    taf_audioVendor_Direction_t direction,
    double *gain
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), LE_UNSUPPORTED,
            "Audio drive is not available!");

    taf_audioVendor_NodeType_t nodeType = TAF_AUDIOVENDOR_INVALID;
    taf_audioVendor_GetNodeType(nodeId, &nodeType);
    TAF_ERROR_IF_RET_VAL(nodeType == TAF_AUDIOVENDOR_INVALID, LE_BAD_PARAMETER, "Invalid node ID");

    if(direction == TAF_AUDIOVENDOR_RX){
        if(nodeType == TAF_AUDIOVENDOR_AUDIO_A2B){
            return LE_UNSUPPORTED;
        }
    } else {
        if(nodeType == TAF_AUDIOVENDOR_AUDIO_A2B || nodeType == TAF_AUDIOVENDOR_AUDIO_PA){
            return LE_UNSUPPORTED;
        }
    }

    return audioVhal.GetNodeGain(nodeId, direction, gain);
}

taf_audioVendor_NodeStateChangeHandlerRef_t taf_audioVendor_AddNodeStateChangeHandler
(
    uint8_t audioNodeId,
    taf_audioVendor_NodeStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_VAL(!audioVhal.isAudioDrvAvailable(), NULL,
            "Audio drive is not available!");
    return audioVhal.AddNodeStateChangeHandler(audioNodeId, handlerPtr, contextPtr);
}

void taf_audioVendor_RemoveNodeStateChangeHandler
(
    taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef
)
{
    auto &audioVhal = taf_AudioVhal::GetInstance();
    TAF_ERROR_IF_RET_NIL(!audioVhal.isAudioDrvAvailable(), "Audio drive is not available!");
    audioVhal.RemoveNodeStateChangeHandler(handlerRef);
}

/**
* FUNCTION     : SetMute
* DESCRIPTION  : Sets the mute status of modem RX/TX, player, recorder streams.
* DEPENDECY    :
* PARAMETERS   : Player stream reference and mute status
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid stream reference
*                and LE_FAULT on failure.
*/
le_result_t taf_audio_SetMute
(
    taf_audio_StreamRef_t streamRef,
    bool isMute
)
{
    LE_DEBUG("taf_audio_SetMute : %s", isMute ? "true" : "false");
    auto &audio = taf_Audio::GetInstance();
    return audio.SetMute(streamRef, isMute);
}

/**
* FUNCTION     : GetMute
* DESCRIPTION  : Gets the mute status of modem RX/TX, player, recorder streams.
* DEPENDECY    :
* PARAMETERS   : Player stream reference and address of bool
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid stream reference
*                and LE_FAULT on failure.
*/
le_result_t taf_audio_GetMute
(
    taf_audio_StreamRef_t streamRef,
    bool *isMute
)
{
    LE_DEBUG("taf_audio_GetMute");
    TAF_ERROR_IF_RET_VAL(isMute == NULL, LE_BAD_PARAMETER, "isMute pointer is NULL");
    auto &audio = taf_Audio::GetInstance();
    return audio.GetMute(streamRef, isMute);
}

/**
* FUNCTION     : SetVolume
* DESCRIPTION  : Sets the volume level of modem RX, player, recorder streams.
* DEPENDECY    :
* PARAMETERS   : Player stream reference and volume level
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid stream reference
*                and LE_FAULT on failure.
*/
le_result_t taf_audio_SetVolume
(
    taf_audio_StreamRef_t streamRef,
    double volumeLevel
)
{
    LE_DEBUG("taf_audio_SetVolume: %f", volumeLevel);
    auto &audio = taf_Audio::GetInstance();
    return audio.SetVolume(streamRef, volumeLevel, true);
}

/**
* FUNCTION     : GetVolume
* DESCRIPTION  : Gets the volume level of modem RX, player, recorder streams.
* DEPENDECY    :
* PARAMETERS   : Player stream reference and address of double
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid stream reference
*                and LE_FAULT on failure.
*/
le_result_t taf_audio_GetVolume
(
    taf_audio_StreamRef_t streamRef,
    double *volumeLevel
)
{
    LE_DEBUG("taf_audio_GetVolume");
    TAF_ERROR_IF_RET_VAL(volumeLevel == NULL, LE_BAD_PARAMETER, "volumeLevel pointer is NULL");
    auto &audio = taf_Audio::GetInstance();
    return audio.GetVolume(streamRef, volumeLevel);
}

/**
* FUNCTION     : PlayFileList
* DESCRIPTION  : Plays all the audio file in the list as per the configuration.
* DEPENDECY    :
* PARAMETERS   : Player stream reference, address of PlayFileConfig and size of play list.
* RETURN VALUES: LE_OK on success, LE_BUSY when other playback is active,
*                LE_BAD_PARAMETER on invalid stream reference and LE_FAULT on failure.
*/
le_result_t taf_audio_PlayFileList
(
    taf_audio_StreamRef_t streamRef,
    const taf_audio_PlayFileConfig_t*  playFileConfigPtr,
    size_t playFileConfigSize
)
{
    LE_INFO("taf_mngd_audio_PlayFileList");
    auto &audio = taf_Audio::GetInstance();
    return audio.PlayList(streamRef, playFileConfigPtr, playFileConfigSize);
}

/**
 * FUNCTION     : PlayDtmf
 * DESCRIPTION  : Plays Dtmf tone on RX path for VoiceStream
 * DEPENDECY    : Active Voice Stream
 * PARAMETERS   : Modem Rx stream reference, DTMF char, duration, pause and gain
 * RETURN VALUES: LE_OK on success,
 *                LE_BAD_PARAMETER on invalid stream reference and LE_FAULT on failure.
 */
le_result_t taf_audio_PlayDtmf
(
taf_audio_StreamRef_t streamRef,
const char*           dtmfPtr,
uint16_t              duration,
uint32_t              pause,
double                gain
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.PlayDtmf(streamRef, dtmfPtr, duration, pause, gain);
}

/**
 * FUNCTION     : PlaySignallingDtmf
 * DESCRIPTION  : Plays Dtmf tone on TX path for voice call
 * DEPENDECY    : Active Voice call
 * PARAMETERS   : SlotId, DTMF chars, duration, pause
 * RETURN VALUES: LE_OK on success, LE_UNSUPPORTED when no active RF call,
 *                LE_BAD_PARAMETER on invalid parameters ,LE_BUSY when DTMF playback is in progress
 *                and LE_FAULT on failure.
 */
le_result_t taf_audio_PlaySignallingDtmf
(
uint32_t              slotId,
const char*           dtmfPtr,
uint32_t              duration,
uint32_t              pause
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.PlaySignallingDtmf(slotId, dtmfPtr, duration, pause);
}

/**
 * FUNCTION     : StopDtmf
 * DESCRIPTION  : Stop Dtmf on TX path for voice call
 * DEPENDECY    : Active Voice call
 * PARAMETERS   : Slot Id
 * RETURN VALUES: LE_OK on success, LE_UNSUPPORTED when no active RF call,
 *                LE_BAD_PARAMETER on invalid slotId and LE_FAULT on failure.
 */
le_result_t taf_audio_StopSignallingDtmf
(
uint32_t              slotId
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.StopSignallingDtmf(slotId);
}

/**
 * FUNCTION     : StopDtmf
 * DESCRIPTION  : Stop Dtmf
 * DEPENDECY    : Active Voice Stream
 * PARAMETERS   : Modem Rx stream reference
 * RETURN VALUES: LE_OK on success,
 *                LE_BAD_PARAMETER on invalid stream reference and LE_FAULT on failure.
 */
le_result_t taf_audio_StopDtmf
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.StopDtmf(streamRef);
}

taf_audio_DtmfDetectorHandlerRef_t taf_audio_AddDtmfDetectorHandler
(
 taf_audio_StreamRef_t               streamRef,
 taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    return (taf_audio_DtmfDetectorHandlerRef_t) audio.AddDtmfDetectorHandler(streamRef,
            handlerPtr, contextPtr);
}

void taf_audio_RemoveDtmfDetectorHandler
(
 taf_audio_DtmfDetectorHandlerRef_t handlerRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.RemoveDtmfDetectorHandler(handlerRef);
}

COMPONENT_INIT
{
    LE_INFO("tafAudioSvc COMPONENT init...");

    auto &audio = taf_Audio::GetInstance();
    audio.Init();

    LE_INFO("COMPONENT end init");
}
