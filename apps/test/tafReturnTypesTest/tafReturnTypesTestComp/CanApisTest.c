/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate CAN Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void canRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("canRetTest_RunApis");

    //1.taf_can_CreateCanInf NULL scenario
    const char* infNamePtr = "can0";
    taf_can_CanInterfaceRef_t can;
    can = taf_can_CreateCanInf(infNamePtr,TAF_CAN_BCM_SOCK);
    LE_TEST_OK(can == NULL,"taf_can_CreateCanInf-NULL");

    //2.taf_can_SetFilter LE_BAD_PARAMETER, scenario
    res = taf_can_SetFilter(NULL,0x123);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_SetFilter-LE_BAD_PARAMETER,");

    //3.taf_can_EnableLoopback LE_BAD_PARAMETER, scenario
    res = taf_can_EnableLoopback(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_EnableLoopback-LE_BAD_PARAMETER,");

    //4.taf_can_DisableLoopback LE_BAD_PARAMETER, scenario
    res = taf_can_DisableLoopback(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_DisableLoopback-LE_BAD_PARAMETER,");

    //5.taf_can_EnableRcvOwnMsg LE_BAD_PARAMETER, scenario
    res = taf_can_EnableRcvOwnMsg(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_EnableRcvOwnMsg-LE_BAD_PARAMETER,");

    //6.taf_can_DisableRcvOwnMsg LE_BAD_PARAMETER, scenario
    res = taf_can_DisableRcvOwnMsg(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_DisableRcvOwnMsg-LE_BAD_PARAMETER,");

    //7.taf_can_IsFdSupported false, scenario
    bool fd;
    fd = taf_can_IsFdSupported(NULL);
    LE_TEST_OK(fd == false,"taf_can_IsFdSupported-false");

    //8.taf_can_EnableFdFrame LE_BAD_PARAMETER, scenario
    res = taf_can_EnableFdFrame(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_EnableFdFrame-LE_BAD_PARAMETER");

    //9.taf_can_GetFdStatus false, scenario
    fd = taf_can_GetFdStatus(NULL);
    LE_TEST_OK(fd == false,"taf_can_GetFdStatus-false");

    //10.taf_can_CreateCanFrame NULL scenario
    taf_can_CanFrameRef_t frame;
    frame = taf_can_CreateCanFrame(NULL,0x123);
    LE_TEST_OK(frame == NULL,"taf_can_CreateCanFrame-NULL");

    //11.taf_can_SetPayload LE_BAD_PARAMETER scenario
    uint8_t data[TAF_CAN_DATA_MAX_LENGTH];
    res = taf_can_SetPayload(NULL,data,TAF_CAN_DATA_MAX_LENGTH);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_SetPayload-LE_BAD_PARAMETER");

    //12.taf_can_SetFrameType LE_BAD_PARAMETER scenario
    res = taf_can_SetFrameType(NULL,TAF_CAN_CAN_FD_FRAME);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_SetFrameType-LE_BAD_PARAMETER");

    //13.taf_can_SendFrame LE_BAD_PARAMETER scenario
    res = taf_can_SendFrame(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_SendFrame-LE_BAD_PARAMETER");

    //14.taf_can_DeleteCanInf LE_BAD_PARAMETER scenario
    res = taf_can_DeleteCanInf(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_DeleteCanInf-LE_BAD_PARAMETER");

    //15.taf_can_DeleteCanFrame LE_BAD_PARAMETER scenario
    res = taf_can_DeleteCanFrame(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_can_DeleteCanFrame-LE_BAD_PARAMETER");

}
