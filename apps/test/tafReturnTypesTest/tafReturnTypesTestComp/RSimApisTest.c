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

    //1.taf_simRsp_GetEID LE_BAD_PARAMETER scenario
    res = taf_simRsp_GetEID(0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_simRsp_GetEID***-LE_BAD_PARAMETER");

    //2.taf_simRsp_GetEID LE_OVERFLOW scenario
    char eidPtr[TAF_SIMRSP_EID_BYTES-1];
    size_t eidLen = TAF_SIMRSP_EID_BYTES-1;
    res = taf_simRsp_GetEID(0,eidPtr,eidLen);
    LE_TEST_OK(res == LE_OVERFLOW,"***taf_simRsp_GetEID***-LE_OVERFLOW");

    //3.taf_simRsp_AddProfile LE_BAD_PARAMETER scenario
    #define ACTIVATION_CODE ""
    res = taf_simRsp_AddProfile(1, ACTIVATION_CODE,"",false);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_simRsp_AddProfile***-LE_BAD_PARAMETER");

    //4.taf_simRsp_GetProfileIndex 0 scenario
    uint32_t prof;
    prof = taf_simRsp_GetProfileIndex(NULL);
    LE_TEST_OK(prof == 0,"***taf_simRsp_GetProfileIndex***0");

    //5.taf_simRsp_GetProfileType -1 scenario
    taf_simRsp_ProfileType_t type;
    type = taf_simRsp_GetProfileType(NULL);
    LE_TEST_OK(type == -1,"***taf_simRsp_GetProfileType***-1");

    //6.taf_simRsp_GetIccid LE_FAULT scenario
    char iccid[TAF_SIMRSP_ICCID_BYTES];
    res = taf_simRsp_GetIccid(NULL,iccid,sizeof(iccid));
    LE_TEST_OK(res == LE_FAULT,"***taf_simRsp_GetIccid***-LE_FAULT");

    //7.taf_simRsp_GetProfileActiveStatus false scenario
    bool status;
    status = taf_simRsp_GetProfileActiveStatus(NULL);
    LE_TEST_OK(status == false,"***taf_simRsp_GetProfileActiveStatus***-false");

    //8.taf_simRsp_GetNickName LE_FAULT scenario
    res = taf_simRsp_GetNickName(NULL,iccid,sizeof(iccid));
    LE_TEST_OK(res == LE_FAULT,"***taf_simRsp_GetNickName***-LE_FAULT");

    //9.taf_simRsp_GetName LE_FAULT scenario
    res = taf_simRsp_GetName(NULL,iccid,sizeof(iccid));
    LE_TEST_OK(res == LE_FAULT,"***taf_simRsp_GetName***-LE_FAULT");

    //10.taf_simRsp_GetSpn LE_FAULT scenario
    res = taf_simRsp_GetSpn(NULL,iccid,sizeof(iccid));
    LE_TEST_OK(res == LE_FAULT,"***taf_simRsp_GetSpn***-LE_FAULT");

    //11.taf_simRsp_GetIconType -1 scenario
    taf_simRsp_IconType_t icon;
    icon = taf_simRsp_GetIconType(NULL);
    LE_TEST_OK(icon == -1,"***taf_simRsp_GetIconType***-1");

    //12.taf_simRsp_GetProfileClass 0 scenario
    taf_simRsp_ProfileClass_t  cla;
    cla = taf_simRsp_GetProfileClass(NULL);
    LE_TEST_OK(cla == 0,"***taf_simRsp_GetProfileClass***0");

    //13.taf_simRsp_GetMask 0 scenario
    prof = taf_simRsp_GetMask(NULL);
    LE_TEST_OK(prof == 0,"***taf_simRsp_GetMask***0");

    //14.taf_simSap_SendMessage LE_UNSUPPORTED scenario
    static uint8_t ConnectReqMsg[12] =
    {0x13, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
    static uint8_t ConnectReqLength = 12;
    res = taf_simSap_SendMessage(ConnectReqMsg,ConnectReqLength);
    LE_TEST_OK(res == LE_UNSUPPORTED,"***taf_simSap_SendMessage***-LE_UNSUPPORTED");

    //14.taf_simSap_SendMessage LE_FAULT scenario
    static uint8_t ReqMsg[12] =
    {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
    static uint8_t ReqLength = 12;
    res = taf_simSap_SendMessage(ReqMsg,ReqLength);
    LE_TEST_OK(res == LE_FAULT,"***taf_simSap_SendMessage***-LE_FAULT");
}
