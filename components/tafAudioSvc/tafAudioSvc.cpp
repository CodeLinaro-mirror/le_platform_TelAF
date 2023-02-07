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
 *
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>
#include "tafAudio.hpp"

using namespace telux::common;
using namespace telux::audio;
using namespace telux::tafsvc;


COMPONENT_INIT
{
    LE_INFO("tafAudio Service Init...\n");
    auto &audio = taf_Audio::GetInstance();
    audio.Init();

    LE_INFO(" Audio service Ready...\n");

}

/**
 * New API defined in telAf Audio Service
 */

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
* FUNCTION     : OpenSpeaker
* DESCRIPTION  : Open Speaker
* DEPENDECY    :
* PARAMETERS   : NIL
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenSpeaker
(
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenSpeaker();
}

/**
* FUNCTION     : OpenMic
* DESCRIPTION  : Open MicroPhone
* DEPENDECY    :
* PARAMETERS   : NIL
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenMic
(
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenMic();
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
* PARAMETERS   : SlotId
* RETURN VALUES: Reference of a Stream, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenModemVoiceTx
(
uint32_t slotId
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenModemVoiceTx(slotId);
}

/**
* FUNCTION     : PlayDtmf
* DESCRIPTION  : Plays Dtmf tone for Inband. Applicable for VoiceStream
* DEPENDECY    : Active Stream
* PARAMETERS   : Dtmf frequency, duration and gain
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_audio_PlayDtmf
(
taf_audio_StreamRef_t streamRef,
const char*          dtmfPtr,
uint32_t             duration,
uint32_t             pause
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.PlayDtmf(streamRef, dtmfPtr, duration, pause);
}

/**
* FUNCTION     : Mute
* DESCRIPTION  : Mutes the volumes for the given stream
* DEPENDECY    : Active Stream
* PARAMETERS   : Stream Reference for Audio
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_audio_Mute
(
taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    StreamMute mute = {};
    mute.enable = true;
    return audio.Mute(streamRef, mute);
}

/**
* FUNCTION     : UnMute
* DESCRIPTION  : UnMutes the volumes for the given stream
* DEPENDECY    : Active Stream
* PARAMETERS   : Stream Reference for Audio
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_audio_Unmute
(
taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    StreamMute mute = {};
    mute.enable = false;
    return audio.Mute(streamRef, mute);
}

/**
 * FUNCTION     : OpenPlayer
 * DESCRIPTION  : Gets the reference of Playing
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: Reference of a Stream, NULL on error
 */
taf_audio_StreamRef_t taf_audio_OpenPlayer
(
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenPlayer();
}

/**
 * FUNCTION     : PlayFile
 * DESCRIPTION  : Play a file on a playback stream
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio and File descriptor
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_PlayFile
(
 taf_audio_StreamRef_t    streamRef,
 int fd
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.PlayFile(streamRef, fd);
}

/**
 * FUNCTION     : Stop
 * DESCRIPTION  : Stop the file playback/recording
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_Stop
(
taf_audio_StreamRef_t    streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.Stop(streamRef);
}

/**
 * FUNCTION     : SetGain
 * DESCRIPTION  : Set the volume
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_SetGain
(
taf_audio_StreamRef_t    streamRef,
int32_t  gain
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.SetVolume(streamRef, gain);
}

/**
 * FUNCTION     : GetGain
 * DESCRIPTION  : Get stream volume
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_GetGain
(
taf_audio_StreamRef_t    streamRef,
int32_t  *gain
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.GetVolume(streamRef, gain);
}

/**
 * FUNCTION     : StopDtmf
 * DESCRIPTION  : Stop Dtmf
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
void taf_audio_StopDtmf
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.StopDtmf(streamRef);
}

/**
 * FUNCTION     : EnableNoiseSuppressor
 * DESCRIPTION  : Enable NoiseSuppressor
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_EnableNoiseSuppressor
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.EnableNoiseSuppressor(streamRef);
}

/**
 * FUNCTION     : EnableEchoCanceller
 * DESCRIPTION  : Enable EchoCanceller
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_EnableEchoCanceller
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.EnableEchoCanceller(streamRef);
}

/**
 * FUNCTION     : DisableNoiseSuppressor
 * DESCRIPTION  : Disable NoiseSuppressor
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_DisableNoiseSuppressor
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.DisableNoiseSuppressor(streamRef);
}

/**
 * FUNCTION     : DisableEchoCanceller
 * DESCRIPTION  : Disable EchoCanceller
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_DisableEchoCanceller
(
taf_audio_StreamRef_t streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.DisableEchoCanceller(streamRef);
}

/**
 * FUNCTION     : IsNoiseSuppressorEnabled
 * DESCRIPTION  : Get status for Noise Suppressor
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_IsNoiseSuppressorEnabled
(
taf_audio_StreamRef_t streamRef,
bool* status
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.IsNoiseSuppressorEnabled(streamRef, status);
}

/**
 * FUNCTION     : IsEchoCancellerEnabled
 * DESCRIPTION  : Get status for EchoCanceller
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_IsEchoCancellerEnabled
(
taf_audio_StreamRef_t streamRef,
bool* status
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.IsEchoCancellerEnabled(streamRef, status);
}

/**
 * FUNCTION     : AddMediaHandler
 * DESCRIPTION  : Send media events notifications
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
taf_audio_MediaHandlerRef_t taf_audio_AddMediaHandler
(
    taf_audio_StreamRef_t streamRef,
    taf_audio_MediaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    return (taf_audio_MediaHandlerRef_t) audio.AddMediaHandler(streamRef, handlerPtr, contextPtr);
}

/**
 * FUNCTION     : AddDtmfDetectorHandler
 * DESCRIPTION  : Detect DTMF from far end
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
taf_audio_DtmfDetectorHandlerRef_t taf_audio_AddDtmfDetectorHandler
(
 taf_audio_StreamRef_t               streamRef,
 taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
 void* contextPtr
 )
{
    auto &audio = taf_Audio::GetInstance();
    return (taf_audio_DtmfDetectorHandlerRef_t) audio.AddDtmfDetectorHandler(streamRef, handlerPtr, contextPtr);
}

/**
* FUNCTION     : OpenI2sRx
* DESCRIPTION  : Open I2s interface Rx
* DEPENDECY    :
* PARAMETERS   : channel mode
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenI2sRx
(
    taf_audio_I2SChannel_t mode  ///< [IN] The channel mode.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenI2sRx(mode);

}

/**
* FUNCTION     : OpenI2sTx
* DESCRIPTION  : Open I2s interface Tx
* DEPENDECY    :
* PARAMETERS   : channel mode
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenI2sTx
(
    taf_audio_I2SChannel_t mode  ///< [IN] The channel mode.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenI2sTx(mode);

}

/**
* FUNCTION     : OpenPcmRx
* DESCRIPTION  : Open Pcm Rx interface
* DEPENDECY    :
* PARAMETERS   : time slot number
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenPcmRx
(
    uint32_t timeslot  ///< [IN] The time slot number.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenPcmRx(timeslot);

}

/**
* FUNCTION     : OpenPcmTx
* DESCRIPTION  : Open Pcm Tx interface
* DEPENDECY    :
* PARAMETERS   : time slot number
* RETURN VALUES: Stream Reference, NULL on error
*/
taf_audio_StreamRef_t taf_audio_OpenPcmTx
(
    uint32_t timeslot  ///< [IN] The time slot number.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenPcmTx(timeslot);

}

/**
* FUNCTION     : SetSamplePcmSamplingRate
* DESCRIPTION  : Set sampling rate for the stream
* DEPENDECY    :
* PARAMETERS   : Stream refernce and sampling rate
* RETURN VALUES: LE_OK on success, LE_FAULT on error
*/
le_result_t taf_audio_SetSamplePcmSamplingRate
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    uint32_t                 samplingRate  ///< [IN] The sampling rate.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.SetSamplePcmSamplingRate(streamRef, samplingRate);

}

/**
 * FUNCTION     : GetSamplePcmSamplingRate
 * DESCRIPTION  : Get sampling rate of the stream
 * DEPENDECY    :
 * PARAMETERS   : Stream refernce
 * RETURN VALUES: LE_OK on success, LE_FAULT on error
 */
le_result_t taf_audio_GetSamplePcmSamplingRate
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    uint32_t                 *samplingRate  ///< [OUT] The sampling rate.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.GetSamplePcmSamplingRate(streamRef, samplingRate);

}

/**
 * FUNCTION     : SetSamplePcmChannelNumber
 * DESCRIPTION  : Set channel number for the recorder stream
 * DEPENDECY    :
 * PARAMETERS   : Stream refernce and channel number
 * RETURN VALUES: LE_OK on success, LE_FAULT on error
 */
le_result_t taf_audio_SetSamplePcmChannelNumber
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    uint32_t                 channelNum  ///< [IN] The Channel number.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.SetSamplePcmChannelNumber(streamRef, channelNum);

}

/**
 * FUNCTION     : GetSamplePcmChannelNumber
 * DESCRIPTION  : Get channel number of the recorder stream
 * DEPENDECY    :
 * PARAMETERS   : Stream refernce
 * RETURN VALUES: LE_OK on success, LE_FAULT on error
 */
le_result_t taf_audio_GetSamplePcmChannelNumber
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    uint32_t                 *channelNum  ///< [OUT] The Channel number.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.GetSamplePcmChannelNumber(streamRef, channelNum);

}

/**
 * FUNCTION     : SetEncodingFormat
 * DESCRIPTION  : Set encoding format for the recorder stream
 * DEPENDECY    :
 * PARAMETERS   : Stream refernce and encoding format
 * RETURN VALUES: LE_OK on success, LE_FAULT on error
 */
le_result_t taf_audio_SetEncodingFormat
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    taf_audio_Format_t       format  ///< [IN] The Encoding format.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.SetEncodingFormat(streamRef, format);

}

/**
 * FUNCTION     : GetEncodingFormat
 * DESCRIPTION  : Get encoding format of the recorder stream
 * DEPENDECY    :
 * PARAMETERS   : Stream refernce
 * RETURN VALUES: LE_OK on success, LE_FAULT on error
 */
le_result_t taf_audio_GetEncodingFormat
(
    taf_audio_StreamRef_t    streamRef,  ///< [IN] The Stream Ref.
    taf_audio_Format_t       *format  ///< [OUT] The sampling rate.
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.GetEncodingFormat(streamRef, format);

}

/**
 * FUNCTION     : OpenRecorder
 * DESCRIPTION  : Gets the reference of Recording stream
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: Reference of a Stream, NULL on error
 */
taf_audio_StreamRef_t taf_audio_OpenRecorder
(
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.OpenRecorder();
}

/**
 * FUNCTION     : RecordFile
 * DESCRIPTION  : Records a file on a record stream
 * DEPENDECY    :
 * PARAMETERS   : Stream Reference for Audio and File descriptor
 * RETURN VALUES: LE_OK on success, LE_FAULT for all errors
 */
le_result_t taf_audio_RecordFile
(
 taf_audio_StreamRef_t    streamRef,
 int fd
)
{
    auto &audio = taf_Audio::GetInstance();
    return audio.RecordFile(streamRef, fd);
}
