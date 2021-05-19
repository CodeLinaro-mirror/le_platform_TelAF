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
