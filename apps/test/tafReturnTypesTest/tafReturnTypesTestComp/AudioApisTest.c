/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Audio Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void audioRetTest_RunApis
(
   void
)
{
    le_result_t result;
    static const char*  DtmfString = "5";
    const char* configurePath = "/data/audioConfigure.xml";
    taf_audioVendor_Direction_t direction = TAF_AUDIOVENDOR_RX;
    LE_TEST_INFO("audioRetTest_RunApis");

   //1. taf_audio_DeleteConnector - Failure scenario
    taf_audio_DeleteConnector(NULL);
    LE_TEST_OK(true, "taf_audio_DeleteConnector=Failed to delete");

    //2. taf_audio_Close - Failure scenario
    taf_audio_Close(NULL);
    LE_TEST_OK(true, "taf_audio_Close=Failed to close");

    //3. taf_Audio_Connect - LE_BAD_PARAMETER scenario
    result = taf_audio_Connect(NULL,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_Audio_Connect***-LE_BAD_PARAMETER");

    //4. taf_audio_Disconnectt - Failure scenario scenario
    taf_audio_Disconnect(NULL,NULL);
    LE_TEST_OK(true, "taf_audio_Disconnect=NULL reference");

    //5.taf_audio_OpenRoute- NULL scenario
    taf_audio_RouteRef_t routeRef = taf_audio_OpenRoute( -1, TAF_AUDIO_VOICE_CALL,
            NULL,NULL);
    LE_TEST_OK(routeRef==NULL, "taf_audio_OpenRoute=NULL reference");

    //6.taf_audio_CloseRoute- LE_BAD_PARAMETER scenario
    result = taf_audio_CloseRoute(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_CloseRoute- LE_BAD_PARAMETER");

    //7.taf_audio_Stop- LE_FAULT scenario
    result = taf_audio_Stop(NULL);
    LE_TEST_OK(result == LE_FAULT, "taf_audio_Stop- LE_FAULT");

    //8.taf_audio_PlayFileList- LE_FAULT scenario
    result = taf_audio_PlayFileList(NULL,NULL,(size_t) 0);
    LE_TEST_OK(result == LE_FAULT, "taf_audio_PlayFileList- LE_FAULT");

    //9.taf_audio_SetMute- LE_FAULT scenario
    result = taf_audio_SetMute(NULL,false);
    LE_TEST_OK(result == LE_FAULT, "taf_audio_SetMute- LE_FAULT");

    //10.taf_audio_GetMute- LE_BAD_PARAMETER scenario
    result = taf_audio_GetMute(NULL,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_GetMute- LE_BAD_PARAMETER");

    //11.taf_audio_GetVolume- LE_BAD_PARAMETER scenario
    result = taf_audio_GetVolume(NULL,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_GetVolume- LE_BAD_PARAMETER");

    //12.taf_audio_PlaySignallingDtmf- LE_BAD_PARAMETER scenario
    result = taf_audio_PlaySignallingDtmf(0,DtmfString,0,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_PlaySignallingDtmf- LE_BAD_PARAMETER");

    //13.taf_audio_PlayDtmf- LE_BAD_PARAMETER scenario
    result = taf_audio_PlayDtmf(0,DtmfString,0,0,2);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_PlayDtmf- LE_BAD_PARAMETER");

    //14.taf_audio_StopDtmf- LE_BAD_PARAMETER scenario
    result = taf_audio_StopDtmf(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_StopDtmf- LE_BAD_PARAMETER");

    //15.taf_audio_StopSignallingDtmf- LE_BAD_PARAMETER scenario
    result = taf_audio_StopSignallingDtmf(0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_audio_StopSignallingDtmf- LE_BAD_PARAMETER");

    //16.taf_audioVendor_GetNodeType- LE_UNSUPPORTED_LE_BAD_PARAMETER scenario
    result = taf_audioVendor_GetNodeType(0x1,NULL);
    LE_TEST_OK(((LE_UNSUPPORTED == result) || (LE_BAD_PARAMETER == result)),"taf_audioVendor_GetNodeType -LE_UNSUPPORTED_LE_BAD_PARAMETER");

    //17.taf_audioVendor_SendNodeVendorConfig- LE_UNSUPPORTED_LE_BAD_PARAMETER scenario
    result = taf_audioVendor_SendNodeVendorConfig(0x1, configurePath);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_audioVendor_SendNodeVendorConfig- LE_UNSUPPORTED");

    //18.taf_audioVendor_SendVendorConfig- LE_UNSUPPORTED scenario
    result = taf_audioVendor_SendVendorConfig(configurePath);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_audioVendor_SendVendorConfig- LE_UNSUPPORTED");

    //20.taf_audioVendor_SetNodePowerState- LE_UNSUPPORTED scenario
    result = taf_audioVendor_SetNodePowerState(0x1, TAF_AUDIOVENDOR_ACTIVE);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_audioVendor_SetNodePowerState- LE_UNSUPPORTED");

    //21.taf_audioVendor_GetNodePowerState- LE_UNSUPPORTED_LE_BAD_PARAMETER scenario
    result = taf_audioVendor_GetNodePowerState(0x1, NULL);
    LE_TEST_OK(((LE_UNSUPPORTED == result) || (LE_BAD_PARAMETER == result)),"taf_audioVendor_GetNodePowerState -LE_UNSUPPORTED_LE_BAD_PARAMETER");

    //22.taf_audioVendor_SetNodeMuteState- LE_UNSUPPORTED scenario
    result = taf_audioVendor_SetNodeMuteState(0x1, false);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_audioVendor_SetNodeMuteState- LE_UNSUPPORTED");

    //23.taf_audioVendor_GetNodeMuteState- LE_UNSUPPORTED scenario
    result = taf_audioVendor_GetNodeMuteState(0x1, NULL);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_audioVendor_GetNodeMuteState- LE_UNSUPPORTED");

    //24.taf_audioVendor_SetNodeGain- LE_BAD_PARAMETER scenario
    result = taf_audioVendor_SetNodeGain(0x1, direction,2);
    LE_TEST_OK(((LE_UNSUPPORTED == result) || (LE_BAD_PARAMETER == result)),"taf_audioVendor_SetNodeGain -LE_UNSUPPORTED_LE_BAD_PARAMETER");

}