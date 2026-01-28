/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Device Information Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void devinfoRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("devinfoRetTest_RunApis");

    //1.taf_devinfo_getimei LE_BAD_PARAMETER scenario
    res = taf_devInfo_GetImei(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_devinfo_getimei-LE_BAD_PARAMETER");

    //2.taf_devinfo_getimei LE_OK scenario
    char imei[TAF_DEVINFO_IMEI_MAX_BYTES-1];
    res = taf_devInfo_GetImei(imei,sizeof(imei));
    LE_TEST_OK(res == LE_OK,"taf_devinfo_getimei-LE_OK");
}
