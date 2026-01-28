/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate WLAN Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void wlanRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("wlanRetTest_RunApis");

    //1. taf_wlan_GetState LE_BAD_PARAMETER scenario
    res = taf_wlan_GetState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetState***-LE_BAD_PARAMETER");

    //2. taf_wlan_SetMode LE_BAD_PARAMETER scenario
    res = taf_wlan_SetMode(NULL,6);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_SetMode***-LE_BAD_PARAMETER");

    //3. taf_wlan_GetMode LE_BAD_PARAMETER scenario
    res = taf_wlan_GetMode(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetMode***-LE_BAD_PARAMETER");

    //4. taf_wlan_GetIntfInfo LE_BAD_PARAMETER scenario
    res = taf_wlan_GetIntfInfo(NULL,NULL,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetIntfInfo***-LE_BAD_PARAMETER");

    //5. taf_wlan_GetBandIntState LE_BAD_PARAMETER scenario
    res = taf_wlan_GetBandIntState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetBandIntState***-LE_BAD_PARAMETER");

    //6. taf_wlan_SetBandIntState LE_BAD_PARAMETER scenario
    res = taf_wlan_SetBandIntState(NULL,2);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_SetBandIntState***-LE_BAD_PARAMETER");

    //7. taf_wlan_GetBandIntPriority LE_BAD_PARAMETER scenario
    res = taf_wlan_GetBandIntPriority(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetBandIntPriority***-LE_BAD_PARAMETER");

    //8. taf_wlan_SetBandIntPriority LE_BAD_PARAMETER scenario
    res = taf_wlan_SetBandIntPriority(NULL,3);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_SetBandIntPriority***-LE_BAD_PARAMETER");

    //9. taf_wlan_GetBandIntWaitTime  scenario
    res = taf_wlan_GetBandIntWaitTime(NULL,3,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_GetBandIntWaitTime***-LE_BAD_PARAMETER");

    //10. taf_wlan_SetBandIntWaitTime LE_BAD_PARAMETER scenario
    res = taf_wlan_SetBandIntWaitTime(NULL,3,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlan_SetBandIntWaitTime***-LE_BAD_PARAMETER");

    //11. taf_wlanAP_GetWlanAP NULL scenario
    taf_wlanAp_WlanAPRef_t wlan;
    wlan = taf_wlanAp_GetWlanAP(3,"wlan");
    LE_TEST_OK(wlan == NULL,"***taf_wlanAP_GetWlanAP***-NULL");

    //12. taf_wlanAP_Start LE_FAULT scenario
    res = taf_wlanAp_Start(NULL);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAP_Start***-LE_FAULT");

    //13. taf_wlanAp_Stop LE_FAULT scenario
    res = taf_wlanAp_Stop(NULL);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_Stop***-LE_FAULT");

    //14. taf_wlanAp_Restart LE_FAULT scenario
    res = taf_wlanAp_Restart(NULL);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_Restart***-LE_FAULT");

    //15. taf_wlapAp_SetConfig LE_FAULT scenario
    taf_wlanAp_WlanAPConfig_t config;
    res = taf_wlanAp_SetConfig(NULL,&config);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlapAp_SetConfig***-LE_FAULT");

    //16. taf_wlanAp_GetConfig LE_BAD_PARAMETER scenario
    res = taf_wlanAp_GetConfig(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanAp_GetConfig***-LE_BAD_PARAMETER");

    //17. taf_wlanAp_GetConfig LE_FAULT scenario
    res = taf_wlanAp_GetConfig(NULL,&config);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_GetConfig***-LE_FAULT");

    //18. taf_wlanAp_SetSecurityConfig LE_FAULT scenario
    taf_wlanAp_WlanAPSecurityConfig_t sec;
    res = taf_wlanAp_SetSecurityConfig(NULL,&sec);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_SetSecurityConfig***-LE_FAULT");

    //19. taf_wlanAp_GetSecurityConfig LE_BAD_PARAMETER scenario
    res = taf_wlanAp_GetSecurityConfig(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanAp_GetSecurityConfig***-LE_BAD_PARAMETER");

    //20. taf_wlanAp_GetSecurityConfig LE_FAULT scenario
    res = taf_wlanAp_GetSecurityConfig(NULL,&sec);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_GetSecurityConfig***-LE_FAULT");

    //21. taf_wlanAp_GetStatus LE_BAD_PARAMETER scenario
    res = taf_wlanAp_GetStatus(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanAp_GetStatus***-LE_BAD_PARAMETER");

    //22. taf_wlanAp_GetStatus LE_FAULT scenario
    taf_wlanAp_WlanAPStatus_t stat;
    res = taf_wlanAp_GetStatus(NULL,&stat);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_GetStatus***-LE_FAULT");

    //23. taf_wlanAp_GetConnectedDevices LE_BAD_PARAMETER scenario
    res = taf_wlanAp_GetConnectedDevices(NULL,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanAp_GetConnectedDevices***-LE_BAD_PARAMETER");

    //24. taf_wlanAp_GetConnectedDevices LE_FAULT scenario
    uint16_t numDevices = 0;
    taf_wlanAp_WlanAPConnectedDeviceInfo_t DevInfo[TAF_WLANAP_MAX_CONNECTED_DEVICES];
    size_t DevInfoSize = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    res = taf_wlanAp_GetConnectedDevices(NULL,&numDevices,DevInfo,&DevInfoSize);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanAp_GetConnectedDevices***-LE_FAULT");

    //25. taf_wlanSta_GetWlanSTA NULL scenario
    taf_wlanSta_WlanSTARef_t sta;
    sta = taf_wlanSta_GetWlanSTA(0,"wlanst");
    LE_TEST_OK(sta == NULL,"***taf_wlanSta_GetWlanSTA***-NULL");

    //26. taf_wlanSta_Start LE_BAD_PARAMETER scenario
    res = taf_wlanSta_Start(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_Start***-LE_BAD_PARAMETER");

    //27. taf_wlanSta_Stop LE_BAD_PARAMETER scenario
    res = taf_wlanSta_Stop(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_Stop***-LE_BAD_PARAMETER");

    //28. taf_wlanSta_Restart LE_BAD_PARAMETER scenario
    res = taf_wlanSta_Restart(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_Restart***-LE_BAD_PARAMETER");

    //29. taf_wlanSta_SetMode LE_FAULT scenario
    res = taf_wlanSta_SetMode(NULL,(taf_wlanSta_Mode_t)NULL);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanSta_SetMode***-LE_FAULT");

    //30. taf_wlanSta_GetMode LE_FAULT scenario
    taf_wlanSta_Mode_t Mode;
    res = taf_wlanSta_GetMode(NULL,&Mode);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanSta_GetMode***-LE_FAULT");

    //31. taf_wlanSta_SetIPConfig LE_FAULT scenario
    taf_wlanSta_IPType_t IPType = TAF_WLANSTA_IPTYPE_STATIC;
    taf_wlanSta_IPConfig_t IpConfig;
    res = taf_wlanSta_SetIPConfig(NULL,IPType,&IpConfig);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanSta_SetIPConfig***-LE_FAULT");

    //32. taf_wlanSta_GetIPConfig LE_BAD_PARAMETER scenario
    res = taf_wlanSta_GetIPConfig(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_GetIPConfig***-LE_BAD_PARAMETER");

    //33. taf_wlanSta_GetIPConfig LE_FAULT scenario
    taf_wlanSta_IPConfig_t StaStaticIPConfig = { { 0 }, { 0 }, { 0 }, { 0 } };
    res = taf_wlanSta_GetIPConfig(NULL,&IPType,&StaStaticIPConfig);
    LE_TEST_OK(res == LE_FAULT,"***taf_wlanSta_GetIPConfig***-LE_FAULT");

    //34. taf_wlanSta_GetStatus LE_BAD_PARAMETER scenario
    res = taf_wlanSta_GetStatus(NULL,NULL,NULL,0,NULL,0,NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_GetStatus***-LE_BAD_PARAMETER");

    //35. taf_wlanSta_DoAPScan LE_BAD_PARAMETER scenario
    res = taf_wlanSta_DoAPScan(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_DoAPScan***-LE_BAD_PARAMETER");

    //35. taf_wlanSta_DoAPScan LE_BAD_PARAMETER scenario
    res = taf_wlanSta_DoAPScan(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_DoAPScan***-LE_BAD_PARAMETER");

    //36. taf_wlanSta_SetWpa2Psk LE_BAD_PARAMETER scenario
    taf_wlanSta_APInfo_t APInfoConnect = {};
    res = taf_wlanSta_SetWpa2Psk(NULL,&APInfoConnect,"wpa");
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_SetWpa2Psk***-LE_BAD_PARAMETER");

    //37. taf_wlanSta_Connect LE_BAD_PARAMETER scenario
    res = taf_wlanSta_Connect(NULL,&APInfoConnect);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_Connect***-LE_BAD_PARAMETER");

    //38. taf_wlanSta_Disconnect LE_BAD_PARAMETER scenario
    res = taf_wlanSta_Disconnect(NULL,&APInfoConnect);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_wlanSta_Disconnect***-LE_BAD_PARAMETER");

}
