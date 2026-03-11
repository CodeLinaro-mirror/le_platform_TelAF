/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Remote SIM Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void rsimRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("rsimRetTest_RunApis");

    //1.taf_simSap_SendMessage LE_UNSUPPORTED scenario
    static uint8_t ConnectReqMsg[12] =
    {0x13, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
    static uint8_t ConnectReqLength = 12;
    res = taf_simSap_SendMessage(ConnectReqMsg,ConnectReqLength);
    LE_TEST_OK(res == LE_UNSUPPORTED,"***taf_simSap_SendMessage***-LE_UNSUPPORTED");

    //2.taf_simSap_SendMessage LE_FAULT scenario
    static uint8_t ReqMsg[12] =
    {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
    static uint8_t ReqLength = 12;
    res = taf_simSap_SendMessage(ReqMsg,ReqLength);
    LE_TEST_OK(res == LE_FAULT,"***taf_simSap_SendMessage***-LE_FAULT");
}
