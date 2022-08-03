/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"

#define TEST_PROFILE   2
static le_sem_Ref_t TestSemRef;
static taf_dcs_ProfileRef_t TestProfileRef = NULL;
char ApnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];

static char *callEventToString(taf_dcs_ConState_t callEvent)
{
    switch (callEvent)
    {
        case TAF_DCS_DISCONNECTED:
            return "disconnect";
        case TAF_DCS_CONNECTING:
            return "connecting";
        case TAF_DCS_CONNECTED:
            return "connected";
        case TAF_DCS_DISCONNECTING:
            return "disconnecting";
        default:
            LE_ERROR("unknown status: %d", callEvent);
            return "unknow status";
    }
    return "unknow status";
}

void data_event_handler(taf_dcs_ProfileRef_t profileRef, taf_dcs_ConState_t callEvent, const taf_dcs_StateInfo_t *infoPtr, void* contextPtr)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;

    LE_INFO("get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
        profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

    if ((callEvent == TAF_DCS_CONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_INFO("data call connected, interface : %s", interfaceName);
        le_sem_Post(TestSemRef);
    }
    else if ((callEvent == TAF_DCS_DISCONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_INFO("data call disconnected");
        le_sem_Post(TestSemRef);
    }

}

void ut_profile_list_test()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize;
    le_result_t result;

    result = taf_dcs_GetProfileList(profilesInfoPtr, &listSize);
    LE_ASSERT(result == LE_OK);
    LE_INFO("got profile list, num: %d, result: %d", listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (int i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
    }
}

void ut_default_profile_set_get_test()
{
    le_result_t result;
    uint32_t profileId;

    result = taf_dcs_SetDefaultProfileIndex(TEST_PROFILE);
    LE_ASSERT(result == LE_OK);

    profileId = taf_dcs_GetDefaultProfileIndex();
    LE_ASSERT(profileId == TEST_PROFILE);

    TestProfileRef = taf_dcs_GetProfile(profileId);
    LE_ASSERT(TestProfileRef != NULL);
}

void ut_set_auth_test()
{
    le_result_t result;

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_PAP, "pap_user", "123");
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_CHAP, "chap_user", "123");
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_NONE, "", "");
    LE_ASSERT(result == LE_OK);
}

void ut_start_session_sync_test()
{
    le_result_t result;

    result = taf_dcs_StartSession(TestProfileRef);
    LE_ASSERT(result == LE_OK);
}

void ut_stop_session_sync_test()
{
    le_result_t result;

    result = taf_dcs_StopSession(TestProfileRef);
    LE_ASSERT(result == LE_OK);
}

void ut_start_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);

    LE_INFO("**** Handler for Start Session Asynchronously (Begin)****");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_INFO("**** Handler for Start Session Asynchronously (End)****");

    le_sem_Post(TestSemRef);
}

void ut_start_session_async_test()
{
    LE_INFO("asynchronous start session %p",TestProfileRef);

    taf_dcs_StartSessionAsync(TestProfileRef, ut_start_session_async_handler_func, NULL);

    LE_INFO("asynchronous start session done");
}

void ut_stop_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);

    LE_INFO("**** Handler for Stop Session Asynchronously (Begin)****");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_INFO("**** Handler for Stop Session Asynchronously (End)****");
}

void ut_stop_session_async_test()
{
    LE_INFO("asynchronous stop session %p",TestProfileRef);

    taf_dcs_StopSessionAsync(TestProfileRef, ut_stop_session_async_handler_func, NULL);

    LE_INFO("asynchronous stop session done");
}

void ut_set_pdp_test(taf_dcs_Pdp_t pdp)
{
    le_result_t result;
    taf_dcs_Pdp_t pdpGet;

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_UNKNOWN);
    LE_ASSERT(result == LE_OK);

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_ASSERT(pdpGet == TAF_DCS_PDP_UNKNOWN);

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV4);
    LE_ASSERT(result == LE_OK);

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_ASSERT(pdpGet == TAF_DCS_PDP_IPV4);

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV6);
    LE_ASSERT(result == LE_OK);

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_ASSERT(pdpGet == TAF_DCS_PDP_IPV6);

    result = taf_dcs_SetPDP(TestProfileRef, pdp);
    LE_ASSERT(result == LE_OK);

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_ASSERT(pdpGet == pdp);
}

void ut_set_apn_test()
{
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    char *testApnStr = "";

    le_result_t result;

    result = taf_dcs_GetAPN(TestProfileRef, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_SetAPN(TestProfileRef, testApnStr);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    int cmpVal = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(cmpVal == 0);
}

void ut_restore_apn_test()
{
    le_result_t result;
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];

    result = taf_dcs_SetAPN(TestProfileRef, ApnStr_bak);
    LE_ASSERT(result == LE_OK);
    result = taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    int cmpVal = strncmp(apnStr, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(cmpVal == 0);
}

void ut_ipv4_check()
{
    le_result_t result;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];

    LE_ASSERT(taf_dcs_IsIPv4(TestProfileRef) == true);

    result = taf_dcs_GetIPv4Address(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv4 Addr: %s", ipAddr0);

    result = taf_dcs_GetIPv4GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv4 Gateway: %s", ipAddr0);

    result = taf_dcs_GetIPv4DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv4 Dns0: %s, Dns1: %s", ipAddr0, ipAddr1);
}

void ut_non_ipv4_check()
{
    LE_ASSERT(taf_dcs_IsIPv4(TestProfileRef) == false);
}

void ut_ipv6_check()
{
    le_result_t result;
    char ipAddr0[TAF_DCS_IPV6_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV6_ADDR_MAX_LEN];

    LE_ASSERT(taf_dcs_IsIPv6(TestProfileRef) == true);

    result = taf_dcs_GetIPv6Address(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv6 Addr: %s", ipAddr0);

    result = taf_dcs_GetIPv6GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv6 Gateway: %s", ipAddr0);

    result = taf_dcs_GetIPv6DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_ASSERT(result == LE_OK);
    LE_INFO("IPv6 Dns0: %s, Dns1: %s", ipAddr0, ipAddr1);
}

void ut_non_ipv6_check()
{
    LE_ASSERT(taf_dcs_IsIPv6(TestProfileRef) == false);
}

void ut_do_session_sync_test_invalid_apn()
{
    le_result_t result;
    char apnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];
    char *testApnStr = "ims";

    result = taf_dcs_GetAPN(TestProfileRef, apnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_SetAPN(TestProfileRef, testApnStr);
    LE_ASSERT(result == LE_OK);
    LE_INFO("set APN to %s, backup APN: %s", testApnStr, apnStr_bak);

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV4V6);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_StartSession(TestProfileRef);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_StopSession(TestProfileRef);
    LE_ASSERT(result == LE_OK);

    result = taf_dcs_SetAPN(TestProfileRef, apnStr_bak);
    LE_ASSERT(result == LE_OK);
}

void ut_ipv4v6_async_datacall_test()
{
    ut_set_pdp_test(TAF_DCS_PDP_IPV4V6);

    ut_set_apn_test();

    ut_start_session_async_test();

    ut_stop_session_async_test();

    ut_restore_apn_test();
}

void ut_ipv4v6_datacall_test()
{
    ut_set_pdp_test(TAF_DCS_PDP_IPV4V6);

    ut_set_apn_test();

    ut_start_session_sync_test();

    ut_ipv4_check();

    ut_ipv6_check();

    ut_stop_session_sync_test();

    ut_restore_apn_test();
}

void ut_ipv4_datacall_test()
{
    ut_set_pdp_test(TAF_DCS_PDP_IPV4);

    ut_set_apn_test();

    ut_start_session_sync_test();

    ut_ipv4_check();

    ut_non_ipv6_check();

    ut_stop_session_sync_test();

    ut_restore_apn_test();
}

void ut_ipv6_datacall_test()
{
    ut_set_pdp_test(TAF_DCS_PDP_IPV6);

    ut_set_apn_test();

    ut_start_session_sync_test();

    ut_non_ipv4_check();

    ut_ipv6_check();

    ut_stop_session_sync_test();

    ut_restore_apn_test();
}

static void* UnitTestThread(void* contextPtr)
{
    TestSemRef = le_sem_Create("taf_datacall_ut_sem", 0);

    ut_profile_list_test();

    ut_default_profile_set_get_test();

    ut_set_auth_test();

    ut_ipv4v6_datacall_test();

    ut_ipv4_datacall_test();

    ut_ipv6_datacall_test();

    ut_do_session_sync_test_invalid_apn();

    /* redo session connection test after testing invalid apn */
    ut_ipv4v6_datacall_test();

     ut_ipv4v6_async_datacall_test();

    LE_INFO("all tests are passed");

    return NULL;
}

COMPONENT_INIT
{
    UnitTestThread(NULL);
}

