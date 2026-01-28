/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Data connnection Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void dcsRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("dcsRetTest_RunApis");

    //1.taf_dcs_StartSession LE_BAD_PARAMETER scenario
    res = taf_dcs_StartSession(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_StartSession-LE_BAD_PARAMETER");

    //2.taf_dcs_StopSession LE_BAD_PARAMETER scenario
    res = taf_dcs_StopSession(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_StopSession-LE_BAD_PARAMETER");

    //3.taf_dcs_GetInterfaceName LE_BAD_PARAMETER scenario
    res = taf_dcs_GetInterfaceName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetInterfaceName-LE_BAD_PARAMETER");

    //4.taf_dcs_GetRoamingStatus LE_BAD_PARAMETER scenario
    res = taf_dcs_GetRoamingStatus(0,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetRoamingStatus-LE_BAD_PARAMETER");

    //5.taf_dcs_GetIPv4Address LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv4Address(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv4Address-LE_BAD_PARAMETER");

    //6.taf_dcs_GetIPv4GatewayAddres LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv4GatewayAddress(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv4GatewayAddres-LE_BAD_PARAMETER");

    //7.taf_dcs_GetIPv4DNSAddresses LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv4DNSAddresses(NULL,NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv4DNSAddresses-LE_BAD_PARAMETER");

    //8.taf_dcs_GetIPv6Address LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv6Address(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv6Address-LE_BAD_PARAMETER");

    //9.taf_dcs_GetIPv6GatewayAddress LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv6GatewayAddress(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv6GatewayAddress-LE_BAD_PARAMETER");

    //10.taf_dcs_GetIPv6DNSAddresses LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv6DNSAddresses(NULL,NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv6DNSAddresses-LE_BAD_PARAMETER");

    //11.taf_dcs_GetSessionState LE_BAD_PARAMETER scenario
    res = taf_dcs_GetSessionState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetSessionState-LE_BAD_PARAMETER");

    //12.taf_dcs_GetDataBearerTechnology LE_BAD_PARAMETER scenario
    res = taf_dcs_GetDataBearerTechnology(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetDataBearerTechnology-LE_BAD_PARAMETER");

    //13.taf_dcs_GetProfileList LE_BAD_PARAMETER scenario
    res = taf_dcs_GetProfileList(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetProfileList-LE_BAD_PARAMETER");

    //14.taf_dcs_GetProfileListEx LE_BAD_PARAMETER scenario
    res = taf_dcs_GetProfileListEx(0,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetProfileListEx-LE_BAD_PARAMETER");

    //15.taf_dcs_GetDefaultPhoneIdAndProfileId LE_BAD_PARAMETER scenario
    res = taf_dcs_GetDefaultPhoneIdAndProfileId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetDefaultPhoneIdAndProfileId-LE_BAD_PARAMETER");

    //16.taf_dcs_GetDefaultProfileIndexEx LE_BAD_PARAMETER scenario
    res = taf_dcs_GetDefaultProfileIndexEx(0,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetDefaultProfileIndexEx-LE_BAD_PARAMETER");

    //17.taf_dcs_GetProfileEx NULL scenario
    taf_dcs_ProfileRef_t dcs;
    dcs = taf_dcs_GetProfileEx(3,0);
    LE_TEST_OK(dcs == NULL,"taf_dcs_GetProfileEx-NULL");

    //18.taf_dcs_CreateProfile LE_BAD_PARAMETER scenario
    res = taf_dcs_CreateProfile(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_CreateProfile-LE_BAD_PARAMETER");

    //19.taf_dcs_DeleteProfile LE_BAD_PARAMETER scenario
    res = taf_dcs_DeleteProfile(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_DeleteProfile-LE_BAD_PARAMETER");

    //20.taf_dcs_GetProfileIndex 0 scenario
    uint32_t index;
    index = taf_dcs_GetProfileIndex(NULL);
    LE_TEST_OK(index == 0,"taf_dcs_GetProfileIndex-0");

    //21.taf_dcs_GetProfileId LE_BAD_PARAMETER scenario
    res = taf_dcs_GetProfileId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetProfileId-LE_BAD_PARAMETER");

    //22.taf_dcs_GetPhoneId LE_BAD_PARAMETER scenario
    res = taf_dcs_GetPhoneId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetPhoneId-LE_BAD_PARAMETER");

    //23.taf_dcs_SetAPN LE_BAD_PARAMETER  scenario
    res = taf_dcs_SetAPN(NULL,"apn");
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_dcs_SetAPN-LE_BAD_PARAMETER ");

    //24.taf_dcs_GetAPN LE_BAD_PARAMETER  scenario
    res = taf_dcs_GetAPN(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_dcs_GetAPN-LE_BAD_PARAMETER ");

    //25.taf_dcs_SetProfileName LE_BAD_PARAMETER scenario
    res = taf_dcs_SetProfileName(NULL,"profile");
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_SetProfileName-LE_BAD_PARAMETER");

    //26.taf_dcs_GetProfileName LE_BAD_PARAMETER scenario
    res = taf_dcs_GetProfileName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetProfileName-LE_BAD_PARAMETER");

    //27.taf_dcs_SetTechPreference LE_BAD_PARAMETER scenario
    res = taf_dcs_SetTechPreference(NULL,TAF_DCS_TECH_3GPP);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_SetTechPreference-LE_BAD_PARAMETER");

    //28.taf_dcs_GetTechPreference LE_BAD_PARAMETER scenario
    res = taf_dcs_GetTechPreference(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetTechPreference-LE_BAD_PARAMETER");

    //29.taf_dcs_SetApnTypes LE_BAD_PARAMETER scenario
    res = taf_dcs_SetApnTypes(NULL,TAF_DCS_APN_TYPE_UNSPECIFIED);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_SetApnTypes-LE_BAD_PARAMETER");

    //30.taf_dcs_GetApnTypes LE_BAD_PARAMETER scenario
    res = taf_dcs_GetApnTypes(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetApnTypes-LE_BAD_PARAMETER");

    //31.taf_dcs_SetPDP LE_BAD_PARAMETER scenario
    res = taf_dcs_SetPDP(NULL,TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_SetPDP-LE_BAD_PARAMETER");

    //32.taf_dcs_SetAuthentication LE_BAD_PARAMETER scenario
    res = taf_dcs_SetAuthentication(NULL,TAF_DCS_AUTH_PAP,"username","pass");
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_SetAuthentication-LE_BAD_PARAMETER");

    //33.taf_dcs_GetAuthentication LE_BAD_PARAMETER scenario
    res = taf_dcs_GetAuthentication(NULL,NULL,NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetAuthentication-LE_BAD_PARAMETER");

    //34.taf_dcs_GetProfileIdByInterfaceName LE_BAD_PARAMETER scenario
    res = taf_dcs_GetProfileIdByInterfaceName("ProfileID",NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetProfileIdByInterfaceName-LE_BAD_PARAMETER");

    //35.taf_dcs_GetPhoneIdByInterfaceName LE_BAD_PARAMETER scenario
    res = taf_dcs_GetPhoneIdByInterfaceName("PhoneID",NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetPhoneIdByInterfaceName-LE_BAD_PARAMETER");

    //36.taf_dcs_GetIPv6SubnetMask LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv6SubnetMask(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv6SubnetMask-LE_BAD_PARAMETER");

    //37.taf_dcs_GetIPv4SubnetMask LE_BAD_PARAMETER scenario
    res = taf_dcs_GetIPv4SubnetMask(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetIPv4SubnetMask-LE_BAD_PARAMETER");

    //38.taf_dcs_GetAPNThrottledStatus LE_BAD_PARAMETER scenario
    res = taf_dcs_GetAPNThrottledStatus(NULL,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetAPNThrottledStatus-LE_BAD_PARAMETER");

    //39.taf_dcs_GetAPNThrottledPLMN LE_BAD_PARAMETER scenario
    res = taf_dcs_GetAPNThrottledPLMN(NULL,NULL,NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetAPNThrottledPLMN-LE_BAD_PARAMETER");

    //40.taf_dcs_GetMaxDataBitRates LE_BAD_PARAMETER scenario
    res = taf_dcs_GetMaxDataBitRates(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetMaxDataBitRates-LE_BAD_PARAMETER");

    //41.taf_dcs_GetCallEndReason LE_BAD_PARAMETER scenario
    res = taf_dcs_GetCallEndReason(NULL,TAF_DCS_PDP_UNKNOWN,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetCallEndReason-LE_BAD_PARAMETER");

    //42.taf_dcs_GetMtu LE_BAD_PARAMETER scenario
    res = taf_dcs_GetMtu(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_dcs_GetMtu-LE_BAD_PARAMETER");

    //43.taf_mdc_StartSession LE_BAD_PARAMETER scenario
    res = taf_mdc_StartSession(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_mdc_StartSession-LE_BAD_PARAMETER");

    //44.taf_mdc_StartSessionAsync LE_BAD_PARAMETER scenario
    res = taf_mdc_StartSessionAsync(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_mdc_StartSessionAsync-LE_BAD_PARAMETER");

    //45.taf_mdc_StopSession LE_BAD_PARAMETER scenario
    res = taf_mdc_StopSession(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_mdc_StopSession-LE_BAD_PARAMETER");

    //46.taf_mdc_StopSessionAsync LE_BAD_PARAMETER scenario
    res = taf_mdc_StopSessionAsync(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_mdc_StopSessionAsync-LE_BAD_PARAMETER");

}
