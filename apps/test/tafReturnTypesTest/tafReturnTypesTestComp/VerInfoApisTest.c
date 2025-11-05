/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Version information Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void verinfoRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("verinfoRetTest_RunApis");

    //1.taf_verInfo_GetKernelVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetKernelVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetKernelVersion***-LE_BAD_PARAMETER");

    //2.taf_verInfo_GetFirmwareVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetFirmwareVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetFirmwareVersion***-LE_BAD_PARAMETER");

    //3.taf_verInfo_GetTZVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetTZVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetTZVersion***-LE_BAD_PARAMETER");

    //4.taf_verInfo_GetTelAFVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetTelAFVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetTelAFVersion***-LE_BAD_PARAMETER");

    //5.taf_verInfo_GetRootFSVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetRootFSVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetRootFSVersion***-LE_BAD_PARAMETER");

    //6.taf_verInfo_GetLXCVersion LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetLXCVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetLXCVersion***-LE_BAD_PARAMETER");

    //7.taf_verInfo_GetTelAFHash LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetTelAFHash(TAF_VERINFO_BANK_B,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetTelAFHash***-LE_BAD_PARAMETER");

    //8.taf_verInfo_GetBootHash LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetBootHash(TAF_VERINFO_BANK_B,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetBootHash***-LE_BAD_PARAMETER");

    //9.taf_verInfo_GetRootFSHash LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetRootFSHash(TAF_VERINFO_BANK_B,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetRootFSHash***-LE_BAD_PARAMETER");

    //10.taf_verInfo_GetFirmwareHash LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetFirmwareHash(TAF_VERINFO_BANK_B,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetFirmwareHash***-LE_BAD_PARAMETER");

    //11.taf_verInfo_GetLXCHash LE_BAD_PARAMETER scenario
    res = taf_verInfo_GetLXCHash(TAF_VERINFO_BANK_B,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_verInfo_GetLXCHash***-LE_BAD_PARAMETER");

}
