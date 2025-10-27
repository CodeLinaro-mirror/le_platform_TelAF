/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <string>
#include <cstring>
#include <atomic>

#include "legato.h"
#include "interfaces.h"

using namespace std;

static le_sem_Ref_t TestSemRef;
static le_thread_Ref_t threadRef = NULL;
static taf_dcs_ProfileRef_t TestProfileRef = NULL;
static taf_dcs_SessionStateHandlerRef_t TestSessionStateRef = NULL;
static int TC_No = 1;
char ApnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];

static std::atomic<bool> bIPv4Connected{false};
static std::atomic<bool> bIPv6Connected{false};
static std::atomic<le_result_t> asyncResult{LE_FAULT};

static const char *callEventToString(taf_dcs_ConState_t callEvent)
{
    switch (callEvent)
    {
        case TAF_DCS_DISCONNECTED:
            return "disconnected";
        case TAF_DCS_CONNECTING:
            return "connecting";
        case TAF_DCS_CONNECTED:
            return "connected";
        case TAF_DCS_DISCONNECTING:
            return "disconnecting";
        default:
            LE_TEST_INFO ("ERR: Unknown status: %d", callEvent);
            return "ERR: UNKNOWN";
    }
}

static const char *pdpTypeToString(taf_dcs_Pdp_t pdp)
{
    switch (pdp)
    {
    case TAF_DCS_PDP_IPV4:
        return "IPV4";
    case TAF_DCS_PDP_IPV6:
        return "IPV6";
    case TAF_DCS_PDP_IPV4V6:
        return "IPV4V6";
    case TAF_DCS_PDP_UNKNOWN:
        return "UNKNOWN";
    default:
        LE_TEST_INFO("ERR: Unsupported pdp: %d", pdp);
        return "ERR: UNSUPPORTED";
    }
}

static void report(le_result_t expected_result, le_result_t actual_result, string API_Name)
{
    if (expected_result == actual_result)
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<< LE_RESULT_TXT(expected_result)<<" - Pass"<<endl;
    }
    else
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<< LE_RESULT_TXT(actual_result)<<" - Fail"<<endl;
    }
    TC_No += 1;
}

static void report(bool expected_result, bool actual_result, string API_Name)
{
    if (expected_result == actual_result)
    {
        std::cout << TC_No << ". " << API_Name << " - " << " - Pass" << endl;
    }
    else
    {
        std::cout << TC_No << ". " << API_Name << " - " << " - Fail" << endl;
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
    LE_TEST_INFO("**** Handler for Start Session Asynchronously (Begin)****");

    LE_TEST_OK(LE_OK == result, "taf_dcs_StartSessionAsync IPv4v6: %d", result);

    asyncResult.store(result);

    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);

    LE_TEST_INFO("profileId= %d, result: %d", profileId, result);
    LE_TEST_INFO("**** Handler for Start Session Asynchronously (End)****");
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
    LE_TEST_INFO("**** Handler for Stop Session Asynchronously (Begin)****");

    LE_TEST_OK(LE_OK == result, "taf_dcs_StopSessionAsync IPv4v6: %d", result);

    asyncResult.store(result);

    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_TEST_INFO("profileId= %d, result: %d", profileId, result);
    LE_TEST_INFO("**** Handler for Stop Session Asynchronously (End)****");
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
    LE_UNUSED(contextPtr);
    le_result_t result;
    char interfaceName[64];
    uint32_t profileId = 0;

    result = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileId: %d", result);

    LE_TEST_INFO("Profile: %d(%p), callEvent: %s, PDP: %s", profileId, profileRef,
                                callEventToString(callEvent), pdpTypeToString(infoPtr->ipType));

    LE_TEST_OK((TAF_DCS_PDP_IPV4 == infoPtr->ipType) || (TAF_DCS_PDP_IPV6 == infoPtr->ipType),
                                                "data event handler PDP should be IPv4 or IPv6" );

    if (callEvent == TAF_DCS_CONNECTED)
    {
        result = taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_TEST_OK(result == LE_OK,"taf_dcs_GetInterfaceName - LE_OK");
        LE_TEST_INFO("Data call connected, interface : %s", interfaceName);
        if (TAF_DCS_PDP_IPV4 == infoPtr->ipType)
        {
            LE_TEST_INFO("Call Connected: TAF_DCS_PDP_IPV4");
            std::cout << "Call Connected: TAF_DCS_PDP_IPV4" << endl;
            bIPv4Connected.store(true);
        }
        else if (TAF_DCS_PDP_IPV6 == infoPtr->ipType)
        {
            LE_TEST_INFO("Call Connected: TAF_DCS_PDP_IPV6");
            std::cout << "Call Connected: TAF_DCS_PDP_IPV6" << endl;
            bIPv6Connected.store(true);
        }
        else
        {
            LE_TEST_INFO("ERR: Call Connected: PDP type not supported: %d", infoPtr->ipType);
            std::cout << "ERR: Call Connected: PDP type not supported: " << infoPtr->ipType << endl;
        }
        return;
    }

    if ((callEvent == TAF_DCS_DISCONNECTED))
    {
        result = taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);
        LE_TEST_OK(result == LE_UNAVAILABLE, "taf_dcs_GetInterfaceName - LE_UNAVAILABLE");
        if (TAF_DCS_PDP_IPV4 == infoPtr->ipType)
        {
            LE_TEST_INFO("Call Disconnected: TAF_DCS_PDP_IPV4");
            std::cout << "Call Disconnected: TAF_DCS_PDP_IPV4" << endl;
            bIPv4Connected.store(false);
        }
        else if (TAF_DCS_PDP_IPV6 == infoPtr->ipType)
        {
            LE_TEST_INFO("Call Disconnected: TAF_DCS_PDP_IPV6");
            std::cout << "Call Disconnected: TAF_DCS_PDP_IPV6" << endl;
            bIPv6Connected.store(false);
        }
        else
        {
            LE_TEST_INFO("ERR: Call Disconnected: PDP type not supported: %d", infoPtr->ipType);
            std::cout << "ERR: Call Disconnected: PDP type not supported: " << infoPtr->ipType <<
                                                                                               endl;
        }
        return;
    }
    return;
}

void profile_list_test()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;

    // Test Case
    result = taf_dcs_GetProfileList(profilesInfoPtr, &listSize);
    LE_TEST_OK(result == LE_OK,"taf_dcs_GetProfileList - LE_OK");
    report(LE_OK,result,"taf_dcs_GetProfileList");

    LE_TEST_INFO("got profile list, num: %" PRIuS ", result: %d", listSize, result);
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
    LE_TEST_OK((TestProfileRef=taf_dcs_GetProfile(TEST_PROFILE))!= NULL, "taf_dcs_GetProfile - !NULL");
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
    LE_UNUSED(ctxPtr);
    taf_dcs_ConnectService();

    // Test Case
    TestSessionStateRef = taf_dcs_AddSessionStateHandler(TestProfileRef,
                          (taf_dcs_SessionStateHandlerFunc_t)data_event_handler, nullptr);
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
    LE_UNUSED(ctxPtr);
    if (TestSessionStateRef)
        taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);
    le_sem_Post(TestSemRef);
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

    // Test Case - Get APN type
    taf_dcs_ApnType_t apnType;
    result=taf_dcs_GetApnTypes(TestProfileRef, &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - LE_OK");
    report(LE_OK,result,"taf_dcs_GetApnTypes");
    std::cout<<"*** ApnTypes: "<<apnType<<endl;

    // Test Case - Set APN type
    taf_dcs_ApnType_t apnType_set = TAF_DCS_APN_TYPE_DEFAULT | TAF_DCS_APN_TYPE_IMS |
                                                                          TAF_DCS_APN_TYPE_FOTA;
    std::cout << "*** ApnTypes to set: " << apnType_set << endl;
    result = taf_dcs_SetApnTypes(TestProfileRef, apnType_set);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes - LE_OK");
    report(LE_OK, result, "taf_dcs_SetApnTypes");

    // Test Case - Get APN type and check if it matches what was set
    taf_dcs_ApnType_t apnType_get;
    result = taf_dcs_GetApnTypes(TestProfileRef, &apnType_get);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - LE_OK");
    std::cout << "*** ApnTypes get: " << apnType_get << endl;
    // Check if the APN type has been set properly
    if (apnType_get == apnType_set)
    {
        report(LE_OK, result, "taf_dcs_SetApnTypes check");
        std::cout << TC_No << "*** ApnTypes set/get check passed" << endl;
    }
    else
    {
        report(LE_FAULT, result, "taf_dcs_SetApnTypes check");
        std::cout << TC_No  <<"*** ApnTypes set/get check failed" << endl;
    }
    TC_No += 1;

    // Test Case - Set APN type to unspecified
    std::cout << "*** ApnTypes to set: TAF_DCS_APN_TYPE_UNSPECIFIED" << endl;
    result = taf_dcs_SetApnTypes(TestProfileRef, TAF_DCS_APN_TYPE_UNSPECIFIED);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes to unspecified - LE_OK");
    report(LE_OK, result, "taf_dcs_SetApnTypes to unspecified");

    TC_No += 1;

    // Test Case - Set APN type back to what was first read
    std::cout << "*** ApnTypes to set: " << apnType << endl;
    result = taf_dcs_SetApnTypes(TestProfileRef, apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes to original - LE_OK");
    report(LE_OK, result, "taf_dcs_SetApnTypes to original");

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
        // Return from here as other tests are not valid.
        TC_No += 1;
        return nullptr;
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
        std::cout << TC_No << ". taf_dcs_GetIPv4DNSAddresses - LE_OK - Pass - " << ipAddr0 << endl;
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
        TC_No += 1;
        // Return from here as other tests are not valid.
        return nullptr;
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
        std::cout << TC_No << ". taf_dcs_GetIPv6DNSAddresses - LE_OK - Pass - " << ipAddr0 << endl;
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
    int iCount = 0;

    int NumberOfArgs = le_arg_NumArgs();
    LE_TEST_ASSERT (NumberOfArgs >= 1, "At least one argument required: %d", NumberOfArgs);
    const char *apnPtr = le_arg_GetArg(0);
    LE_TEST_ASSERT(nullptr != apnPtr, "APN parameter: %s", apnPtr ? apnPtr : "(null)");

    le_clk_Time_t timeout60Sec = {60, 0}; // 1 minute.
    uint32_t profile_index =5;// Use profile 5 to test, because profile 1 used by xtra-daemon
    TestSemRef = le_sem_Create("tafDataAppSem", 0);

    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    std::cout << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << "Prerequisites: " << std::endl
              << " 1. NAD should be registered." << std::endl
              << " 2. APN used should support both IPv4 and IPv4 data calls." << std::endl;
    std::cout << "************************************************" << std::endl;
    default_profile_set_get_test(profile_index);

    set_auth_test();

    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV4 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4");
    std::cout <<endl;
    std::cout <<"**********************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4" << endl;
    std::cout <<"**********************************************" << endl;
    LE_TEST_INFO ("**********************************************");
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4");
    LE_TEST_INFO("**********************************************");
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV4;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession -  LE_OK");
    report(LE_OK,result,"taf_dcs_StartSession");
    if (LE_OK == result)
    {
        ipv4_check(&ipType);
        ipv6_check(&ipType);
        std::cout << "Waiting 10s before stopping data session" << endl;
        LE_TEST_INFO("Waiting 10s before stopping data session");
        sleep(10); // Sleep for 10s and then stop session.
        result = taf_dcs_StopSession(TestProfileRef);
        LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
        report(LE_OK,result,"stop_session_sync_test");

        std::cout << "Waiting 10s before next test" << endl;
        LE_TEST_INFO("Waiting 10s before next test");
        sleep(10); // Sleep for 10s
    }
    else
    {
        LE_TEST_INFO("ERR: Skipping IPv4 tests as taf_dcs_StartSession failed.");
        std::cout << "ERR: Skipping IPv4 tests as taf_dcs_StartSession failed." << endl;
    }

    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV6 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV6");
    std::cout <<endl;
    std::cout <<"**********************************************" << endl;
    std::cout <<"Test Taf Data Call with PDP - TAF_DCS_PDP_IPV6" << endl;
    std::cout <<"**********************************************" << endl;
    LE_TEST_INFO("**********************************************");
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV6");
    LE_TEST_INFO("**********************************************");
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV6;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);
    // profile_list_test();
    result=taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession -  LE_OK");
    if (LE_OK == result)
    {
        report(LE_OK,result,"taf_dcs_StartSession");
        ipv4_check(&ipType);
        ipv6_check(&ipType);
        std::cout << "Waiting 10s before stopping data session" << endl;
        LE_TEST_INFO("Waiting 10s before stopping data session");
        sleep(10); // Sleep for 10s and then stop session.
        result=taf_dcs_StopSession(TestProfileRef);
        LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
        report(LE_OK,result,"stop_session_sync_test");
        std::cout << "Waiting 10s before next test" << endl;
        LE_TEST_INFO("Waiting 10s before next test");
        sleep(10); // Sleep for 10s
    }
    else
    {
        LE_TEST_INFO("ERR: Skipping IPv6 tests as taf_dcs_StartSession failed.");
        std::cout << "ERR: Skipping IPv6 tests as taf_dcs_StartSession failed." << endl;
    }

    if (threadRef == NULL)
    {
        threadRef = le_thread_Create("taf_datacall_state_thread",taf_data_session_handler, nullptr);
        le_thread_Start(threadRef);
    }
    /* Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6 */
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    std::cout << endl;
    std::cout << "************************************************" << endl;
    std::cout << "Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6" << endl;
    std::cout << "************************************************" << endl;
    LE_TEST_INFO("**********************************************");
    LE_TEST_INFO("Test Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    LE_TEST_INFO("**********************************************");
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV4V6;
    set_pdp_test(ipType);
    set_apn_test(apnPtr);

    bIPv4Connected.store(false);
    bIPv6Connected.store(false);
    // Start and ensure both IPv4 and IPv6 are connected
    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(LE_OK == result, "taf_dcs_StartSession IPv4v6: %d", result);
    report(LE_OK, result, "taf_dcs_StartSession IPv4v6");

    if (LE_OK == result)
    {
        iCount = 0;
        do
        {
            // Wait for events to be generated for max 60s.
            if (60 == iCount)
            {
                LE_TEST_INFO("ERR: taf_dcs_StartSession IPv4v6 event timeout.");
                break;
            }
            iCount++;
            sleep(1);
        } while (!bIPv4Connected.load() || !bIPv6Connected.load());
        LE_TEST_OK(bIPv4Connected.load(), "taf_dcs_StartSession IPv4v6 -  IPv4");
        report(true, bIPv4Connected.load(), "taf_dcs_StartSession IPv4v6 -  IPv4");
        ipv4_check(&ipType);

        LE_TEST_OK(bIPv6Connected.load(), "taf_dcs_StartSession IPv4v6 -  IPv6");
        report(true, bIPv6Connected.load(), "taf_dcs_StartSession IPv4v6 -  IPv6");
        ipv6_check(&ipType);

        std::cout << "Waiting 10s before stopping data session" << endl;
        LE_TEST_INFO("Waiting 10s before stopping data session");
        sleep(10); // Sleep for 10s and then stop session.

        bIPv4Connected.store(true);
        bIPv6Connected.store(true);
        result = taf_dcs_StopSession(TestProfileRef);
        LE_TEST_OK(result == LE_OK, "stop_session_sync_test - LE_OK");
        report(LE_OK, result, "stop_session_sync_test");
        // Wait for disconnected events
        iCount = 0;
        do
        {
            // Wait for events to be generated for max 60s.
            if (60 == iCount)
            {
                LE_TEST_INFO("ERR: taf_dcs_StopSession IPv4v6 event timeout.");
                break;
            }
            iCount++;
            sleep(1);
        } while (bIPv4Connected.load() || bIPv6Connected.load());
        LE_TEST_OK(!bIPv4Connected.load(), "taf_dcs_StopSession IPv4v6 -  IPv4");
        LE_TEST_OK(!bIPv6Connected.load(), "taf_dcs_StopSession IPv4v6 -  IPv6");

        std::cout << "Waiting 10s before next test" << endl;
        LE_TEST_INFO("Waiting 10s before next test");
        sleep(10); // Sleep for 10s
    }
    else
    {
        LE_TEST_INFO("ERR: Skipping IPv4v6 tests as taf_dcs_StartSession failed.");
        std::cout << "ERR: Skipping IPv4v6 tests as taf_dcs_StartSession failed." << endl;
    }

    /* Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6 */
    LE_TEST_INFO("Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    std::cout <<endl;
    std::cout <<"*************************************************" << endl;
    std::cout <<"Taf Async Data Call with PDP - TAF_DCS_PDP_IPV4V6" << endl;
    std::cout <<"*************************************************" << endl;
    LE_TEST_INFO("**********************************************");
    LE_TEST_INFO("Test Async Taf Data Call with PDP - TAF_DCS_PDP_IPV4V6");
    LE_TEST_INFO("**********************************************");
    TC_No = 1;
    ipType = TAF_DCS_PDP_IPV4V6;
    set_pdp_test(ipType);

    bIPv4Connected.store(false);
    bIPv6Connected.store(false);
    asyncResult.store(LE_FAULT);
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)start_session_async_test,
                                                                                        NULL, NULL);
    result = le_sem_WaitWithTimeOut(TestSemRef, timeout60Sec);
    LE_TEST_INFO("taf_dcs_StartSessionAsync le_sem_WaitWithTimeOut: %d", result);

    result = asyncResult.load();
    LE_TEST_OK(LE_OK == result, "taf_dcs_StartSessionAsync IPv4v6 -  LE_OK");
    report(LE_OK, result, "taf_dcs_StartSessionAsync IPv4v6");

    TC_No += 1;

    if (LE_OK == result)
    {
        do
        {
            // Wait for events to be generated for max 60s.
            if (60 == iCount)
            {
                LE_TEST_INFO("ERR: taf_dcs_StartSessionAsync IPv4v6 event timeout.");
                break;
            }
            iCount++;
            sleep(1);
        } while (!bIPv4Connected.load() || !bIPv6Connected.load());
        LE_TEST_OK(bIPv4Connected.load(), "taf_dcs_StartSessionAsync IPv4v6 -  IPv4");
        report(true, bIPv4Connected.load(), "taf_dcs_StartSessionAsync IPv4v6 -  IPv4");
        le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)ipv4_check, &ipType, NULL);

        LE_TEST_OK(bIPv6Connected.load(), "taf_dcs_StartSessionAsync IPv4v6 -  IPv6");
        report(true, bIPv6Connected.load(), "taf_dcs_StartSessionAsync IPv4v6 -  IPv6");
        ipv6_check(&ipType);
        le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)ipv6_check, &ipType, NULL);

        std::cout << "Waiting 10s before stopping data session" << endl;
        LE_TEST_INFO("Waiting 10s before stopping data session");
        sleep(10); // Sleep for 10s and then stop session.

        bIPv4Connected.store(true);
        bIPv6Connected.store(true);

        asyncResult.store(LE_FAULT);
        le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)stop_session_async_test,
                                       NULL, NULL);
        result = le_sem_WaitWithTimeOut(TestSemRef, timeout60Sec);
        LE_TEST_INFO("taf_dcs_StopSessionAsync le_sem_WaitWithTimeOut: %d", result);

        result = asyncResult.load();
        LE_TEST_OK(LE_OK == result, "taf_dcs_StopSessionAsync IPv4v6 -  LE_OK");
        report(LE_OK, result, "taf_dcs_StopSessionAsync IPv4v6");

        // Wait for disconnected events
        iCount = 0;
        do
        {
            // Wait for events to be generated for max 60s.
            if (60 == iCount)
            {
                LE_TEST_INFO("ERR: taf_dcs_StopSessionAsync IPv4v6 event timeout.");
                break;
            }
            iCount++;
            sleep(1);
        } while (bIPv4Connected.load() || bIPv6Connected.load());
        LE_TEST_OK(!bIPv4Connected.load(), "taf_dcs_StopSessionAsync IPv4v6 -  IPv4");
        LE_TEST_OK(!bIPv6Connected.load(), "taf_dcs_StopSessionAsync IPv4v6 -  IPv6");
    }
    else
    {
        LE_TEST_INFO("ERR: Skipping IPv4v6 tests as taf_dcs_StartSessionAsync failed.");
        std::cout << "ERR: Skipping IPv4v6 tests as taf_dcs_StartSessionAsync failed." << endl;
    }

    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)remove_handler,NULL, NULL);
    le_sem_WaitWithTimeOut(TestSemRef, timeout60Sec);

    LE_TEST_EXIT;
}
