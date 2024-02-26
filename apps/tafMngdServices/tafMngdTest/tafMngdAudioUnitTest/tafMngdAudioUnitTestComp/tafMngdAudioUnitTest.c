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

COMPONENT_INIT
{
    taf_mngd_audio_StreamRef_t sinkRef = NULL, sinkRef1 = NULL;;
    taf_mngd_audio_StreamRef_t sourceRef = NULL, sourceRef1 = NULL;
    taf_mngd_audio_RouteRef_t routeRef = NULL, routeRef1 = NULL;
    le_result_t res;

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with FORCE mode for ROUTE_0");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0,
            TAF_MNGD_AUDIO_VOICE_CALL_FORCE_OPEN, &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef != NULL, "OpenRoute successfull sinkRef %p sourceRef %p", sinkRef,
            sourceRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API with playback mode");
    routeRef1 = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_0, TAF_MNGD_AUDIO_LOCAL_PLAYBACK,
            &sinkRef1, &sourceRef1);
    LE_TEST_OK(routeRef1 == NULL,
            "OpenRoute successfull failed for LOCAL_PLAYBACK when other route is active");

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    taf_mngd_audio_ConnectorRef_t rxConn = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", rxConn);

    LE_TEST_INFO("Test taf_mngd_audio_CreateConnector");
    taf_mngd_audio_ConnectorRef_t txConn = taf_mngd_audio_CreateConnector();
    LE_TEST_OK(rxConn != NULL, "Successfully created Connector %p", txConn);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceRx");
    taf_mngd_audio_StreamRef_t rxStreamRef = taf_mngd_audio_OpenModemVoiceRx(1);
    LE_TEST_OK(rxStreamRef != NULL, "Successfully created rxStreamRef %p", rxStreamRef);

    LE_TEST_INFO("Test taf_mngd_audio_OpenModemVoiceTx");
    taf_mngd_audio_StreamRef_t txStreamRef = taf_mngd_audio_OpenModemVoiceTx(1, true);
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

    sleep(3);

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

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

    sleep(3);

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

    sleep(3);

    LE_TEST_INFO("Test taf_mngd_audio_CloseRoute");
    res = taf_mngd_audio_CloseRoute(routeRef);
    LE_TEST_OK(res == LE_OK, "Successfully closed the route");

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

    LE_TEST_INFO("Test taf_mngd_audio_OpenRoute API");
    routeRef = taf_mngd_audio_OpenRoute( TAF_MNGD_AUDIO_ROUTE_3, TAF_MNGD_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    LE_TEST_OK(routeRef == NULL, "OpenRoute Failed for ROUTE_3");

    LE_TEST_EXIT;
}