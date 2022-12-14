/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>
#include <cstring>

#include "legato.h"
#include "interfaces.h"

using namespace std;

static le_sem_Ref_t TestSemRef, semaphore;
static le_thread_Ref_t threadRef = NULL;
static taf_dcs_ProfileRef_t TestProfileRef = NULL;
static taf_dcs_SessionStateHandlerRef_t TestSessionStateRef = NULL;
static int TC_No = 1;
char ApnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];

static const char *callEventToString(taf_dcs_ConState_t callEvent)
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

const char* return_val(le_result_t result)
{
    const char* ret_val;
    switch (result)
    {
        case 0:
            ret_val = "LE_OK";
            break;
        case -1:
            ret_val = "LE_NOT_FOUND";
            break;
        case -2:
            ret_val = "LE_NOT_POSSIBLE";
            break;
        case -3:
            ret_val = "LE_OUT_OF_RANGE";
            break;
        case -4:
            ret_val = "LE_NO_MEMORY";
            break;
        case -5:
            ret_val = "LE_NOT_PERMITTED";
            break;
        case -6:
            ret_val = "LE_FAULT";
            break;
        case -7:
            ret_val = "LE_COMM_ERROR";
            break;
        case -8:
            ret_val = "LE_TIMEOUT";
            break;
        case -9:
            ret_val = "LE_OVERFLOW";
            break;
        case -10:
            ret_val = "LE_UNDERFLOW";
            break;
        case -11:
            ret_val = "LE_WOULD_BLOCK";
            break;
        case -12:
            ret_val = "LE_DEADLOCK";
            break;
        case -13:
            ret_val = "LE_FORMAT_ERROR";
            break;
        case -14:
            ret_val = "LE_DUPLICATE";
            break;
        case -15:
            ret_val = "LE_BAD_PARAMETER";
            break;
        case -16:
            ret_val = "LE_CLOSED";
            break;
        case -17:
            ret_val = "LE_BUSY";
            break;
        case -18:
            ret_val = "LE_UNSUPPORTED";
            break;
        case -19:
            ret_val = "LE_IO_ERROR";
            break;
        case -20:
            ret_val = "LE_NOT_IMPLEMENTED";
            break;
        case -21:
            ret_val = "LE_UNAVAILABLE";
            break;
        case -22:
            ret_val = "LE_TERMINATED";
            break;
        case -23:
            ret_val = "LE_IN_PROGRESS";
            break;
        case -24:
            ret_val = "LE_SUSPENDED";
            break;
        default:
            ret_val = "LE_OK";
            break;
    }
    return ret_val;
}

void report(le_result_t expected_result, le_result_t actual_result, string API_Name)
{
    if (expected_result == actual_result)
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<<return_val(expected_result)<<" - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<<return_val(expected_result)<<" - Fail"<<endl;
    }
    TC_No += 1;
}

static void StartSessionAsyncHandlerFunc
(
    taf_dcs_ProfileRef_t profileRef,
    le_result_t result,
    void* contextPtr
)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("**** Handler for Start Session Asynchronously (Begin)****");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_INFO("**** Handler for Start Session Asynchronously (End)****");
    le_sem_Post(TestSemRef);
}

static void* start_session_async_test()
{
    LE_TEST_INFO("Starting asynchronous data session");
    taf_dcs_StartSessionAsync(TestProfileRef, StartSessionAsyncHandlerFunc, NULL);
    return NULL;
}

static void StopSessionAsyncHandlerFunc
(
    taf_dcs_ProfileRef_t profileRef,
    le_result_t result,
    void* contextPtr
)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("**** Handler for Stop Session Asynchronously (Begin)****");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_INFO("**** Handler for Stop Session Asynchronously (End)****");
    le_sem_Post(TestSemRef);
}

static void* stop_session_async_test()
{
    LE_TEST_INFO("Stopping asynchronous data session");
    taf_dcs_StopSessionAsync(TestProfileRef, StopSessionAsyncHandlerFunc, NULL);
    return NULL;
}

void data_event_handler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    le_result_t result;
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;

    LE_TEST_INFO("Get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
        profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

    if ((callEvent == TAF_DCS_CONNECTED) && (infoPtr->ipType == expectIpType))
    {
        result = taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_TEST_OK(result == LE_OK,"taf_dcs_GetInterfaceName - LE_OK");
        LE_TEST_INFO("Data call connected, interface : %s", interfaceName);
    }
    else if ((callEvent == TAF_DCS_DISCONNECTED) && (infoPtr->ipType == expectIpType))
    {
        result = taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_TEST_OK(result == LE_NOT_POSSIBLE,"taf_dcs_GetInterfaceName - LE_OK");
        LE_TEST_INFO("Data call disconnected");
    }
    else if ((callEvent == TAF_DCS_CONNECTING) && (infoPtr->ipType == expectIpType))
    {
        LE_TEST_INFO("Data call connecting");
    }
    else if ((callEvent == TAF_DCS_DISCONNECTING) && (infoPtr->ipType == expectIpType))
    {
        LE_TEST_INFO("Data call disconnecting");
    }

}

void profile_list_test()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize;
    le_result_t result;

    // Test Case
    result = taf_dcs_GetProfileList(profilesInfoPtr, &listSize);
    LE_TEST_OK(result == LE_OK,"taf_dcs_GetProfileList - LE_OK");
    report(LE_OK,result,"taf_dcs_GetProfileList");

    LE_TEST_INFO("got profile list, num: %d, result: %d", listSize, result);
    LE_TEST_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (size_t i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_TEST_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech,
                     profileInfoPtr->name);
    }
}

void default_profile_set_get_test(uint32_t TEST_PROFILE)
{
    // Test Case
    le_result_t result;
    result=taf_dcs_SetDefaultProfileIndex(TEST_PROFILE);
    LE_TEST_OK(result == LE_OK,"taf_dcs_SetDefaultProfileIndex - LE_OK");
    report(LE_OK,result,"taf_dcs_SetDefaultProfileIndex");

    // Test Case
    uint32_t profileId;
    profileId = taf_dcs_GetDefaultProfileIndex();
    LE_TEST_OK(profileId == TEST_PROFILE,
               "Checking if the Default profile index is the one we set");
    if(profileId == TEST_PROFILE)
    {
        std::cout<<TC_No<<". taf_dcs_GetDefaultProfileIndex - returns default profile index - Pass"
                 <<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetDefaultProfileIndex - returns data profile index - Fail"
                 <<endl;
    }
    TC_No += 1;

    // Test Case
    LE_TEST_OK((TestProfileRef=taf_dcs_GetProfile(profileId))!= NULL, "taf_dcs_GetProfile - !NULL");
    if(TestProfileRef != NULL)
    {
        std::cout<<TC_No<<". taf_dcs_GetProfile - !NULL - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetProfile - !NULL - Fail"<<endl;
    }
    TC_No += 1;
}

void set_auth_test()
{
    le_result_t result;

    LE_TEST_OK((result=taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_PAP,
               "pap_user", "123")) == LE_OK, "taf_dcs_SetAuthentication - LE_OK");
    LE_TEST_INFO("TAFDATAUT - taf_dcs_SetAuthentication - Return Value - %d", result);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Pass" <<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Fail" <<endl;
    }
    TC_No += 1;

    LE_TEST_OK((result=taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_CHAP,
               "chap_user", "123")) == LE_OK, "taf_dcs_SetAuthentication - LE_OK");
    LE_TEST_INFO("TAFDATAUT - taf_dcs_SetAuthentication - Return Value - %d", result);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Pass" <<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Fail" <<endl;
    }
    TC_No += 1;

    LE_TEST_OK((result=taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_NONE,
               "", "")) == LE_OK, "taf_dcs_SetAuthentication - LE_OK");
    LE_TEST_INFO("TAFDATAUT - taf_dcs_SetAuthentication - Return Value - %d", result);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Pass" <<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_SetAuthentication - LE_OK - Fail" <<endl;
    }
    TC_No += 1;
}

static void* taf_data_session_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    // Test Case
    TestSessionStateRef = taf_dcs_AddSessionStateHandler(TestProfileRef,
                          (taf_dcs_SessionStateHandlerFunc_t)data_event_handler, ctxPtr);
    LE_TEST_OK(TestSessionStateRef != NULL, "taf_dcs_AddSessionStateHandler - !NULL");
    if(TestSessionStateRef != NULL)
    {
        std::cout<<TC_No<<". taf_dcs_AddSessionStateHandler - !NULL - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_AddSessionStateHandler - !NULL - Fail"<<endl;
    }
    TC_No += 1;

    le_event_RunLoop();

    return NULL;
}

static void* remove_handler(void* ctxPtr)
{
    LE_TEST_INFO("Inside remove_handler");
    taf_dcs_SessionStateHandlerRef_t* session_handler_ref =
                                      (taf_dcs_SessionStateHandlerRef_t*) ctxPtr;
    taf_dcs_SessionStateHandlerRef_t session_handler = *session_handler_ref;
    taf_dcs_RemoveSessionStateHandler(session_handler);
    le_sem_Post(semaphore);
    return NULL;
}

void set_pdp_test(taf_dcs_Pdp_t pdp)
{
    taf_dcs_Pdp_t pdpGet;
    le_result_t result;

    // Test Case
    result = taf_dcs_SetPDP(TestProfileRef, pdp);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP - LE_OK");
    report(LE_OK,result,"taf_dcs_SetPDP");

    // Test Case
    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_TEST_OK((pdpGet = taf_dcs_GetPDP(TestProfileRef)) == pdp, "taf_dcs_GetPDP");
    if(pdpGet == pdp)
    {
        std::cout<<TC_No<<". taf_dcs_GetPDP - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetPDP - Fail"<<endl;
    }
    TC_No += 1;
}

void set_apn_test(const char *testApnStr)
{
    // Test Case
    le_result_t result;
    result=taf_dcs_GetAPN(TestProfileRef, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_GetAPN");

    // Test Case
    result=taf_dcs_SetAPN(TestProfileRef, testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_SetAPN");

    // Test Case
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    result=taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_GetAPN");
    std::cout<<"*** APN: "<<apnStr<<endl;

    // Test Case
    taf_dcs_ApnType_t apnType;
    result=taf_dcs_GetApnTypes(TestProfileRef, &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - LE_OK");
    report(LE_OK,result,"taf_dcs_GetApnTypes");
    std::cout<<"*** ApnTypes: "<<apnType<<endl;

    // Test Case
    int return_value;
    return_value = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK( return_value == 0, "Checking if APN set properly");
    if(return_value == 0)
    {
        std::cout<<TC_No<<". Checking if APN set properly - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if APN set properly - Fail"<<endl;
    }
    TC_No += 1;
}

void restore_apn_test()
{
    // Test Case
    le_result_t result;
    result=taf_dcs_SetAPN(TestProfileRef, ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_SetAPN");

    // Test Case
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    result=taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_GetAPN");

    // Test Case
    int return_value;
    return_value = strncmp(apnStr, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK( return_value == 0, "Resetting APN to default");
    if(return_value == 0)
    {
        std::cout<<TC_No<<". Resetting APN to default - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Resetting APN to default - Fail"<<endl;
    }
    TC_No += 1;
}

void get_roaming_status_test()
{
    bool isRoaming = false;
    taf_dcs_RoamingType_t type;
    uint8_t phoneId = 1;

    le_result_t result = taf_dcs_GetRoamingStatus(phoneId, &isRoaming, &type);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetRoamingStatus - OK");
    report(LE_OK,result,"taf_dcs_GetRoamingStatus");
    std::cout<<"*** isRoaming: "<<isRoaming<<"*** type: "<<type<<endl;
}

static void* ipv4_check(void* ipType)
{
    le_result_t result;
    bool value;
    taf_dcs_Pdp_t* PDPType = (taf_dcs_Pdp_t*) ipType;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];

    // Test Case
    if(*PDPType == TAF_DCS_PDP_IPV4V6 || *PDPType == TAF_DCS_PDP_IPV4)
    {
        LE_TEST_OK((value=taf_dcs_IsIPv4(TestProfileRef)) == true, "taf_dcs_IsIPv4 - TRUE");
        if(value == true)
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv4 - TRUE - Pass"<<endl;
        }
        else
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv4 - TRUE - Fail"<<endl;
        }
    }
    else
    {
        LE_TEST_OK((value=taf_dcs_IsIPv4(TestProfileRef)) == false, "taf_dcs_IsIPv4 - FALSE");
        if(value == false)
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv4 - FALSE - Pass"<<endl;
        }
        else
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv4 - FALSE - Fail"<<endl;
        }
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv4Address(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address - LE_OK");
    LE_TEST_INFO("IPv4 Addr: %s", ipAddr0);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4Address - LE_OK - Pass - "<<ipAddr0<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4Address - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv4GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
    LE_TEST_INFO("IPv4 Gateway: %s", ipAddr0);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4GatewayAddress - LE_OK - Pass - "<<ipAddr0<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4GatewayAddress - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv4DNSAddresses(TestProfileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
    LE_TEST_INFO("IPv4 Dns0: %s, Dns1: %s", ipAddr0, ipAddr1);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4DNSAddresses - LE_OK - Pass - "<<ipAddr1<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv4DNSAddresses - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    return NULL;
}

static void* ipv6_check(void* ipType)
{
    le_result_t result;
    taf_dcs_Pdp_t* PDPType = (taf_dcs_Pdp_t*) ipType;
    bool value;
    char ipAddr0[TAF_DCS_IPV6_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV6_ADDR_MAX_LEN];

    // Test Case
    if(*PDPType == TAF_DCS_PDP_IPV4V6 || *PDPType == TAF_DCS_PDP_IPV6)
    {
        LE_TEST_OK((value=taf_dcs_IsIPv6(TestProfileRef)) == true, "taf_dcs_IsIPv6 - TRUE");
        if(value == true)
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv6 - TRUE - Pass"<<endl;
        }
        else
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv6 - TRUE - Fail"<<endl;
        }
    }
    else
    {
        LE_TEST_OK((value=taf_dcs_IsIPv6(TestProfileRef)) == false, "taf_dcs_IsIPv6 - FALSE");
        if(value == false)
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv6 - FALSE - Pass"<<endl;
        }
        else
        {
            std::cout<<TC_No<<". taf_dcs_IsIPv6 - FALSE - Fail"<<endl;
        }
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv6Address(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6Address - LE_OK");
    LE_TEST_INFO("IPv6 Addr: %s", ipAddr0);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6Address - LE_OK - Pass - "<<ipAddr0<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6Address - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv6GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6GatewayAddress - LE_OK");
    LE_TEST_INFO("IPv6 Gateway: %s", ipAddr0);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6GatewayAddress - LE_OK - Pass - "<<ipAddr0<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6GatewayAddress - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    // Test Case
    result=taf_dcs_GetIPv6DNSAddresses(TestProfileRef, ipAddr0,
                                       TAF_DCS_IPV6_ADDR_MAX_LEN,
                                       ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses - LE_OK");
    LE_TEST_INFO("IPv6 Dns0: %s, Dns1: %s", ipAddr0, ipAddr1);
    if(result == LE_OK)
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6DNSAddresses - LE_OK - Pass - "<<ipAddr1<<endl;
    }
    else
    {
        std::cout<<TC_No<<". taf_dcs_GetIPv6DNSAddresses - LE_OK - Fail"<<endl;
    }
    TC_No += 1;

    return NULL;
}

void do_session_sync_test_invalid_apn()
{
    le_result_t result;
    char apnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];
    const char *testApnStr = "ims";

    std::cout<<"***** Inside do_session_sync_test_invalid_apn() *****"<<endl;

    // Test Case
    result = taf_dcs_GetAPN(TestProfileRef, apnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_GetAPN");

    // Test Case
    result = taf_dcs_SetAPN(TestProfileRef, testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - LE_OK");
    LE_TEST_INFO("set APN to %s, backup APN: %s", testApnStr, apnStr_bak);
    report(LE_OK,result,"taf_dcs_SetAPN");

    // Test Case
    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV4V6);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP - LE_OK");
    report(LE_OK,result,"taf_dcs_SetPDP");

    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession - LE_OK");
    report(LE_OK,result,"taf_dcs_StartSession");

    result = taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession - LE_OK");
    report(LE_OK,result,"taf_dcs_StopSession");

    result = taf_dcs_SetAPN(TestProfileRef, apnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - LE_OK");
    report(LE_OK,result,"taf_dcs_SetAPN");
}

COMPONENT_INIT
{
    le_result_t result;
    taf_dcs_Pdp_t ipType;
    const char* apnPtr = "jionet";
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
    if (NumberOfArgs >= 1)
    {
        apnPtr = le_arg_GetArg(0);
        if (NULL == apnPtr)
        {
            LE_ERROR("parameter is NULL");
            exit(EXIT_FAILURE);
        }
    }
    else // if no arguements passed in the command line argument
    {
        LE_ERROR("NumberOfArgs: %d", NumberOfArgs);
        std::cout <<"No Parameters passed, provide apn name. Exiting the application"<< endl;
        exit(EXIT_FAILURE);
    }

    LE_TEST_PLAN(10);

    uint32_t profile_index =1;

    TestSemRef = le_sem_Create("tafDataAppSem", 0);

    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Pre-Condition:" << endl;
    std::cout <<"************************************************" << endl;
    default_profile_set_get_test(profile_index);

    set_auth_test();

    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6" << endl;
    std::cout <<"************************************************" << endl;
    ipType = TAF_DCS_PDP_IPV4V6;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result ==  LE_OK, "taf_dcs_StartSession -  LE_OK");
    report(LE_OK,result,"taf_dcs_StartSession");
    ipv4_check(&ipType);
    ipv6_check(&ipType);
    result=taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
    report(LE_OK,result,"stop_session_sync_test");

    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV4 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4");
    std::cout <<endl;
    std::cout <<"**********************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4" << endl;
    std::cout <<"**********************************************" << endl;
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV4;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result ==  LE_OK, "taf_dcs_StartSession -  LE_OK");
    report(LE_OK,result,"taf_dcs_StartSession");
    ipv4_check(&ipType);
    ipv6_check(&ipType);
    result=taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
    report(LE_OK,result,"stop_session_sync_test");

    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV6 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV6");
    std::cout <<endl;
    std::cout <<"**********************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_IPV6" << endl;
    std::cout <<"**********************************************" << endl;
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV6;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result ==  LE_OK, "taf_dcs_StartSession -  LE_OK");
    report(LE_OK,result,"taf_dcs_StartSession");
    ipv4_check(&ipType);
    ipv6_check(&ipType);
    result=taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
    report(LE_OK,result,"stop_session_sync_test");

    /* Taf Data Call with PDP - TAF_DCS_PDP_UNKNOWN */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_UNKNOWN");
    std::cout <<endl;
    std::cout <<"*************************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_UNKNOWN" << endl;
    std::cout <<"*************************************************" << endl;
    TC_No = 1;
    ipType = TAF_DCS_PDP_UNKNOWN;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "taf_dcs_StartSession - LE_OUT_OF_RANGE");
    report(LE_OUT_OF_RANGE,result,"taf_dcs_StartSession");
    ipv4_check(&ipType);
    ipv6_check(&ipType);
    result=taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "stop_session_sync_test - LE_OUT_OF_RANGE");
    report(LE_OUT_OF_RANGE,result,"stop_session_sync_test");
    restore_apn_test();
    get_roaming_status_test();

    /* Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6 */
    LE_TEST_INFO("Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    std::cout <<endl;
    std::cout <<"*************************************************" << endl;
    std::cout <<"Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6" << endl;
    std::cout <<"*************************************************" << endl;
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV4V6;
    set_pdp_test(ipType);
    if(threadRef == NULL)
    {
        threadRef = le_thread_Create("taf_datacall_state_thread",
                                     taf_data_session_handler, &ipType);
        le_thread_Start(threadRef);
    }
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)start_session_async_test,
                                   NULL, NULL);
    le_sem_Wait(TestSemRef);
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)ipv4_check, &ipType, NULL);
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)ipv6_check, &ipType, NULL);
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)stop_session_async_test,
                                   NULL, NULL);
    le_sem_Wait(TestSemRef);
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)remove_handler,
                                   &TestSessionStateRef, NULL);

    LE_TEST_EXIT;
}
