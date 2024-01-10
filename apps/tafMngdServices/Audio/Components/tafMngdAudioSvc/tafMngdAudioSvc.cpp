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
#include "tafMngdAudio.hpp"

using namespace telux::tafsvc;

/**
* FUNCTION     : CreateConnector
* DESCRIPTION  : Creates the connector for given I/O
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Connector reference
*/
taf_mngd_audio_ConnectorRef_t taf_mngd_audio_CreateConnector
(
    void
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.CreateConnector();
}

/**
* FUNCTION     : DeleteConnector
* DESCRIPTION  : Deletes the connctor path
* DEPENDECY    :
* PARAMETERS   : Connector reference
* RETURN VALUES:
*/
void taf_mngd_audio_DeleteConnector
(
 taf_mngd_audio_ConnectorRef_t connectorRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.DeleteConnector(connectorRef);
}

/**
* FUNCTION     : Connect
* DESCRIPTION  : Connect the stream and Connector
* DEPENDECY    :
* PARAMETERS   : Connector and Stream
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_mngd_audio_Connect
(
 taf_mngd_audio_ConnectorRef_t connectorRef,
 taf_mngd_audio_StreamRef_t    streamRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.Connect(connectorRef, streamRef);
}

/**
* FUNCTION     : Disconnect
* DESCRIPTION  : Disconnects the connector and stream
* DEPENDECY    :
* PARAMETERS   : Connector and Stream
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
void taf_mngd_audio_Disconnect
(
 taf_mngd_audio_ConnectorRef_t connectorRef,
 taf_mngd_audio_StreamRef_t    streamRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.Disconnect(connectorRef, streamRef);
}

/**
* FUNCTION     : Close
* DESCRIPTION  : Close stream reference
* DEPENDECY    :
* PARAMETERS   : Audio stream
* RETURN VALUES:
*/
void taf_mngd_audio_Close
(
 taf_mngd_audio_StreamRef_t    streamRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.Close(streamRef);
}

/**
* FUNCTION     : OpenModemVoiceRx
* DESCRIPTION  : Gets the reference of outStream
* DEPENDECY    :
* PARAMETERS   : SlotId
* RETURN VALUES: Reference of OutStream, NULL on error
*/
taf_mngd_audio_StreamRef_t taf_mngd_audio_OpenModemVoiceRx
(
    uint32_t slotId
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.OpenModemVoiceRx(slotId);
}

/**
* FUNCTION     : OpenModemVoiceTx
* DESCRIPTION  : Gets the reference of VoiceTx Path
* DEPENDECY    :
* PARAMETERS   : SlotId, ECNR configuration
* RETURN VALUES: Reference of a Stream, NULL on error
*/
taf_mngd_audio_StreamRef_t taf_mngd_audio_OpenModemVoiceTx
(
    uint32_t slotId, bool enableEcnr
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.OpenModemVoiceTx(slotId, enableEcnr);
}

/**
* FUNCTION     : OpenRoute
* DESCRIPTION  : Opens the audio route
* DEPENDECY    :
* PARAMETERS   : route, mode, sinkRef, sourceRef
* RETURN VALUES: LE_OK on success, LE_BUSY if another route is opened,
*                LE_BAD_PARAMETER on bad params, LE_FAULT on error.
*/
taf_mngd_audio_RouteRef_t taf_mngd_audio_OpenRoute
(
    taf_mngd_audio_RouteId_t route,
    taf_mngd_audio_Mode_t mode,
    taf_mngd_audio_StreamRef_t *sinkRef,
    taf_mngd_audio_StreamRef_t *sourceRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.OpenRoute(route, mode, sinkRef, sourceRef);
}
/**
* FUNCTION     : CloseRoute
* DESCRIPTION  : Closes the audio route
* DEPENDECY    :
* PARAMETERS   : Route reference created on OpenRoute
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER if route is not opened, LE_FAULT on error.
*/
le_result_t taf_mngd_audio_CloseRoute
(
    taf_mngd_audio_RouteRef_t routeRef
)
{
    auto &mngdAudio = taf_MngdAudio::GetInstance();
    return mngdAudio.CloseRoute(routeRef);
}

COMPONENT_INIT
{
    LE_INFO("tafMngdAudioSvc COMPONENT init...");

    auto &mngdAudio = taf_MngdAudio::GetInstance();
    mngdAudio.Init();

    LE_INFO("COMPONENT end init");
}
