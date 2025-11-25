/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Health Minitor Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void hmsRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("hmsRetTest_RunApis");

    //1.taf_hms_GetCpuLoad LE_BAD_PARAMETER scenario
    res = taf_hms_GetCpuLoad(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetCpuLoad-LE_BAD_PARAMETER");

    //2.taf_hms_GetIndvCoreUsage LE_BAD_PARAMETER scenario
    res = taf_hms_GetIndvCoreUsage(1,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetIndvCoreUsage-LE_BAD_PARAMETER");

    //3.taf_hms_GetRamMemInfo LE_BAD_PARAMETER scenario
    res = taf_hms_GetRamMemInfo(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetRamMemInfo-LE_BAD_PARAMETER");

    //4.taf_hms_GetFirstUbiDevInfo NULL scenario
    taf_hms_UbiDevInfoRef_t hms;
    hms = taf_hms_GetFirstUbiDevInfo(NULL);
    LE_TEST_OK(hms == NULL,"taf_hms_GetFirstUbiDevInfo-NULL");

    //5.taf_hms_DeleteUbiDevInfoList LE_NOT_FOUND scenario
    res = taf_hms_DeleteUbiDevInfoList(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_hms_DeleteUbiDevInfoList-LE_NOT_FOUND");

    //6.taf_hms_GetNextUbiDevInfo NULL scenario
    hms = taf_hms_GetNextUbiDevInfo(NULL);
    LE_TEST_OK(hms == NULL,"taf_hms_GetNextUbiDevInfo-NULL");

    //7.taf_hms_GetUbiDevId LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiDevId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiDevId-LE_BAD_PARAMETER");

    //8.taf_hms_GetUbiDevMaxEraseCnt LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiDevMaxEraseCnt(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiDevMaxEraseCnt-LE_BAD_PARAMETER");

    //9.taf_hms_GetUbiDevBadBlkCnt LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiDevBadBlkCnt(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiDevBadBlkCnt-LE_BAD_PARAMETER");

    //10.taf_hms_GetFirstUbiVolInfo NULL scenario
    taf_hms_UbiVolInfoRef_t ubi;
    ubi = taf_hms_GetFirstUbiVolInfo(NULL);
    LE_TEST_OK(ubi == NULL,"taf_hms_GetFirstUbiVolInfo-NULL");

    //11.taf_hms_GetNextUbiVolInfo NULL scenario
    ubi = taf_hms_GetNextUbiVolInfo(NULL);
    LE_TEST_OK(ubi == NULL,"taf_hms_GetNextUbiVolInfo-NULL");

    //12.taf_hms_GetUbiVolId LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiVolId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiVolId-LE_BAD_PARAMETER");

    //13.taf_hms_GetUbiVolName LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiVolName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiVolName-LE_BAD_PARAMETER");

    //14.taf_hms_GetUbiVolSize LE_BAD_PARAMETER scenario
    res = taf_hms_GetUbiVolSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetUbiVolSize-LE_BAD_PARAMETER");

    //15.taf_hms_DeleteMtdDevInfoList LE_NOT_FOUND scenario
    res = taf_hms_DeleteMtdDevInfoList(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_hms_DeleteMtdDevInfoList-LE_NOT_FOUND");

    //16.taf_hms_GetFirstMtdDevInfo NULL scenario
    taf_hms_MtdDevInfoRef_t mtd;
    mtd = taf_hms_GetFirstMtdDevInfo(NULL);
    LE_TEST_OK(mtd == NULL,"taf_hms_GetFirstMtdDevInfo-NULL");

    //17.taf_hms_GetNextMtdDevInfo NULL scenario
    mtd = taf_hms_GetNextMtdDevInfo(NULL);
    LE_TEST_OK(mtd == NULL,"taf_hms_GetNextMtdDevInfo-NULL");

    //18.taf_hms_GetMtdDevName LE_BAD_PARAMETER scenario
    res = taf_hms_GetMtdDevName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetMtdDevName-LE_BAD_PARAMETER");

    //19.taf_hms_GetMtdDevBlkSize LE_BAD_PARAMETER scenario
    res = taf_hms_GetMtdDevBlkSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetMtdDevBlkSize-LE_BAD_PARAMETER");

    //20.taf_hms_GetMtdDevId LE_BAD_PARAMETER scenario
    res = taf_hms_GetMtdDevId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetMtdDevId-LE_BAD_PARAMETER");

    //21.taf_hms_GetMtdDevBlkCnt LE_BAD_PARAMETER scenario
    res = taf_hms_GetMtdDevBlkCnt(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_hms_GetMtdDevBlkCnt-LE_BAD_PARAMETER");

    //22.taf_hms_ReleaseModemEvt LE_NOT_FOUND scenario
    res = taf_hms_ReleaseModemEvt(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_hms_ReleaseModemEvt-LE_NOT_FOUND");

    //23.taf_hms_GetResetInformation LE_FAULT scenario
    res = taf_hms_GetResetInformation(NULL,NULL,0);
    LE_TEST_OK(res == LE_FAULT,"taf_hms_GetResetInformation-LE_FAULT");

}
