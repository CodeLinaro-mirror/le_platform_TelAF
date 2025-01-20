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
/*
 *  Changes from Qualcomm Innovation Center, Inc are provided under the following license:
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Refer the PrintUsage() function for usage instructions.
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <future>
#include <iostream>

// Function declarations
void tafDCSUnitTest_RunInteractiveTests();

// Test defines
#define TAF_CONFIG_SSIM_TEST
#define TAF_CONFIG_PHONE_ID_1_TEST
// #define TAF_CONFIG_PHONE_ID_2_TEST

//For SSIM
#ifdef TAF_CONFIG_SSIM_TEST
#define SSIM_TEST 1
#else
#define SSIM_TEST 0
#endif

//For DSSA and DSDA phone id 1
#ifdef TAF_CONFIG_PHONE_ID_1_TEST
#define PHONE_ID_1_TEST 1
#else
#define PHONE_ID_1_TEST 0
#endif

//For DSDA phone id 2
#ifdef TAF_CONFIG_PHONE_ID_2_TEST
#define PHONE_ID_2_TEST 1
#else
#define PHONE_ID_2_TEST 0
#endif

uint32_t TEST_PROFILE_PHONEID_1 = 5;  // profile id 5, used for phoneid 1
uint32_t TEST_PROFILE_PHONEID_2 = 1;  // profile id 1, used for phoneid 2

#define PHONE_ID_1     1
#define PHONE_ID_2     2

static le_sem_Ref_t TestSemRef;
static taf_dcs_ProfileRef_t TestProfileRef = NULL , TestProfileRef2 = NULL ;
le_thread_Ref_t dataSessionThRef = NULL, dataSessionThRef2 = NULL;
static taf_dcs_SessionStateHandlerRef_t TestSessionStateRef = NULL, TestSessionStateRef2 = NULL;
static taf_dcs_RoamingStatusHandlerRef_t TestRoamingStatusRef = NULL;
static taf_dcs_ThrottledStatusHandlerRef_t TestThrottleStatusRef = NULL;
char ApnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];

/**
 * To run interactively:
 *      app runProc tafDataCallUnitTest tafDataCallUnitTest -- Interactive
 * To run profile management tests (create/delete, start/stop data):
 *      app runProc tafDataCallUnitTest tafDataCallUnitTest -- Profile APN PDP
 *      PDP: IPV4 / IPV6 / IPV4V6
 * To run all other previous unit tests(except new profile management APIs):
 *      app runProc tafDataCallUnitTest tafDataCallUnitTest -- Full <Profile1> <Profile2>
 *          Profile1 -> Profile number to use with phone 1 tests
 *          Profile2 -> Profile number to use with phone 2 tests
 *
 */
static void PrintUsage()
{
    std::cout << std::endl
              << "To run tests interactively:"
              << std::endl
              << "\tapp runProc tafDataCallUnitTest tafDataCallUnitTest -- Interactive"
              << std::endl
              << "To run profile management tests (create/delete, start/stop data):"
              << std::endl
              << "\tapp runProc tafDataCallUnitTest tafDataCallUnitTest -- Profile APN PDP"
              << std::endl
              << "\tPDP: IPV4 / IPV6 / IPV4V6"
              << std::endl
              << "To run all unit tests(except profile management APIs):"
              << std::endl
              << "\tapp runProc tafDataCallUnitTest tafDataCallUnitTest -- Full <Profile1> <Profile2>"
              << "\t\tProfile1 -> Profile number to use with phone 1 tests"
              << std::endl
              << "\t\tProfile2 -> Profile number to use with phone 2 tests"
              << std::endl;
}

std::string callEventToString(taf_dcs_ConState_t callEvent)
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

#if defined(TAF_CONFIG_SSIM_TEST) || defined(TAF_CONFIG_PHONE_ID_1_TEST)

void data_event_handler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;
    uint32_t profileId=0;
    uint8_t phoneId;
    le_result_t result = LE_OK;

    LE_INFO("get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
             profileRef, callEventToString(callEvent).c_str(), infoPtr->ipType, expectIpType);

    if ((callEvent == TAF_DCS_CONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);

        result = taf_dcs_GetProfileIdByInterfaceName(interfaceName, &profileId);
        LE_TEST_OK(result == LE_OK, "Connected: profile(%d), ifname(%s)", profileId, interfaceName);

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = taf_dcs_GetPhoneIdByInterfaceName(interfaceName, &phoneId);
        LE_TEST_OK(result == LE_OK, "Connected: phoneId(%d), ifname(%s)", phoneId, interfaceName);

        LE_TEST_END_SKIP();

    }
    else if ((callEvent == TAF_DCS_DISCONNECTED) && (infoPtr->ipType == expectIpType))
    {
        profileId = taf_dcs_GetProfileIndex(profileRef);
        LE_TEST_OK(profileId == TEST_PROFILE_PHONEID_1, "Disconnected: profile(%d)", profileId);

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);

        LE_TEST_END_SKIP();

    }

}

static void* ut_taf_data_session_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestSessionStateRef = taf_dcs_AddSessionStateHandler(TestProfileRef,
                                (taf_dcs_SessionStateHandlerFunc_t)data_event_handler, ctxPtr);

    LE_TEST_OK(TestSessionStateRef != NULL, "ut_taf_data_session_handler - void");

    le_sem_Post(TestSemRef);

    le_event_RunLoop();

    return NULL;
}
#endif

#ifdef TAF_CONFIG_PHONE_ID_2_TEST
void data_event_handler2(taf_dcs_ProfileRef_t profileRef, taf_dcs_ConState_t callEvent,
                                        const taf_dcs_StateInfo_t *infoPtr, void* contextPtr)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;
    uint32_t profileId=0;
    uint8_t phoneId;
    le_result_t result = LE_OK;

    LE_INFO("get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
            profileRef, callEventToString(callEvent).c_str(), infoPtr->ipType, expectIpType);

    if ((callEvent == TAF_DCS_CONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);

        result = taf_dcs_GetProfileIdByInterfaceName(interfaceName, &profileId);
        LE_TEST_OK(result == LE_OK, "Connected: profile(%d), ifname(%s)", profileId, interfaceName);


        result = taf_dcs_GetPhoneIdByInterfaceName(interfaceName, &phoneId);
        LE_TEST_OK(result == LE_OK, "Connected: phoneId(%d), ifname(%s)", phoneId, interfaceName);

    }
    else if ((callEvent == TAF_DCS_DISCONNECTED) && (infoPtr->ipType == expectIpType))
    {
        profileId = taf_dcs_GetProfileIndex(profileRef);
        LE_TEST_OK(profileId == TEST_PROFILE_PHONEID_2, "Disconnected: profile(%d)", profileId);

        result = taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);

    }

}

static void* ut_taf_data_session_handler2(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestSessionStateRef2 = taf_dcs_AddSessionStateHandler(TestProfileRef2,
                                (taf_dcs_SessionStateHandlerFunc_t)data_event_handler2, ctxPtr);

    LE_TEST_OK(TestSessionStateRef2 != NULL, "ut_taf_data_session_handler2 - void");

    le_sem_Post(TestSemRef);

    le_event_RunLoop();

    return NULL;
}

#endif

static void roaming_status_handler
(
    const taf_dcs_RoamingStatusInd_t* roamingStatusIndPtr,
    void* contextPtr
){
    LE_INFO("**** Handler for roaming status Indication (Begin)****");
    LE_INFO("----phoneId : %d", (int)roamingStatusIndPtr->phoneId);
    LE_INFO("----isRoaming : %d", (int)roamingStatusIndPtr->isRoaming);
    LE_INFO("----type : %d", (int)roamingStatusIndPtr->type);

    LE_INFO("**** Handler for roaming status Indication (End)****");
}

static void throttle_status_handler
(
    taf_dcs_ProfileRef_t    profileRef,        ///< The profile reference.
    bool       isThrottled,       ///< True when APN is throttled. False when APN is unthrottled.
    uint32_t     ipv4RemainingTime, ///< The remaining IPv4 throttled time in milliseconds.
    uint32_t     ipv6RemainingTime, ///< The remaining IPv6 throttled time in milliseconds.
    void* contextPtr
){
    LE_TEST_INFO("**** Handler for throttle status Indication (Begin)****");
    LE_TEST_INFO("----isThrottled : %d", (bool)isThrottled);
    LE_TEST_INFO("----ipv4 Time : %d", (int)ipv4RemainingTime);
    LE_TEST_INFO("----ipv6 Time : %d", (int)ipv6RemainingTime);
    uint32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    uint8_t  phoneId = 0;
    taf_dcs_GetPhoneId(profileRef,&phoneId);
    LE_TEST_INFO("----profile ID : %d", (int)profileId);
    LE_TEST_INFO("----phone ID : %d", (int)phoneId);

    LE_TEST_INFO("**** Handler for throttle status Indication (End)****");
}

static void* ut_taf_roaming_status_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestRoamingStatusRef = taf_dcs_AddRoamingStatusHandler(
                           (taf_dcs_RoamingStatusHandlerFunc_t)roaming_status_handler,
                            ctxPtr);

    LE_TEST_OK(TestRoamingStatusRef != NULL, "ut_taf_roaming_status_handler - void");

    le_sem_Post(TestSemRef);

    le_event_RunLoop();

    return NULL;
}

static void* ut_taf_throttle_status_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestThrottleStatusRef = taf_dcs_AddThrottledStatusHandler(TestProfileRef,
                           (taf_dcs_ThrottledStatusHandlerFunc_t)throttle_status_handler,
                            ctxPtr);

    LE_TEST_OK(TestThrottleStatusRef != NULL, "ut_taf_throttle_status_handler - void");

    le_sem_Post(TestSemRef);

    le_event_RunLoop();

    return NULL;
}

void ut_profile_list_test()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST, 1);
    result = taf_dcs_GetProfileList(profilesInfoPtr, &listSize);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetProfileList - OK");
    LE_INFO("got profile list, num: %" PRIuS ", result: %d", listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (uint32_t i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech,
                                                                    profileInfoPtr->name);
    }

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);
    result = taf_dcs_GetProfileListEx(PHONE_ID_1,profilesInfoPtr, &listSize);

    LE_TEST_OK(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d) - OK", PHONE_ID_1);
    LE_INFO("-----got profile list for phone id %d, num: %" PRIuS ", result: %d",
             PHONE_ID_1, listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (uint32_t i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech,
                                                                            profileInfoPtr->name);
    }

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_GetProfileListEx(PHONE_ID_2,profilesInfoPtr, &listSize);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d) - OK", PHONE_ID_2);
    LE_INFO("-----got profile list for phone id %d, num: %" PRIuS ", result: %d",
             PHONE_ID_2, listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (uint32_t i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech,
                                                                            profileInfoPtr->name);
    }

    LE_TEST_END_SKIP();
}

void ut_default_profile_set_get_test()
{
    le_result_t result;
    uint32_t profileId, bakProfileId;
    uint8_t bakPhoneId;

    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&bakPhoneId, &bakProfileId);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDefaultPhoneIdAndProfileId - OK");

    LE_TEST_BEGIN_SKIP(!SSIM_TEST, 1);

    TestProfileRef = taf_dcs_GetProfile(TEST_PROFILE_PHONEID_1);
    LE_TEST_OK(TestProfileRef != NULL, "taf_dcs_GetProfile - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef);
    LE_TEST_OK(profileId == TEST_PROFILE_PHONEID_1, "taf_dcs_GetProfileIndex - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

    uint8_t phoneId;

    TestProfileRef = taf_dcs_GetProfileEx(PHONE_ID_1, TEST_PROFILE_PHONEID_1);
    LE_TEST_OK(TestProfileRef != NULL, "taf_dcs_GetProfileEx - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef);
    LE_TEST_OK(profileId == TEST_PROFILE_PHONEID_1, "taf_dcs_GetProfileIndex - OK");

    result = taf_dcs_GetPhoneId(TestProfileRef, &phoneId);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetPhoneId phoneId(%d) OK", phoneId);

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    uint8_t secPhoneId;

    TestProfileRef2 = taf_dcs_GetProfileEx(PHONE_ID_2, TEST_PROFILE_PHONEID_2);
    LE_TEST_OK(TestProfileRef2 != NULL, "taf_dcs_GetProfileEx - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef2);
    LE_TEST_OK(profileId == TEST_PROFILE_PHONEID_2, "taf_dcs_GetProfileIndex - OK");

    result = taf_dcs_GetPhoneId(TestProfileRef2, &secPhoneId);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetPhoneId phoneId(%d) OK", secPhoneId);

    LE_TEST_END_SKIP();

}

void ut_set_get_auth_test()
{
    le_result_t result;
    int cmpVal = 0;
    char usrName[TAF_DCS_USER_NAME_MAX_LEN];
    char password[TAF_DCS_PASSWORD_NAME_MAX_LEN];
    taf_dcs_Auth_t authType;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_PAP, "pap_user", "123");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    result = taf_dcs_GetAuthentication(TestProfileRef, &authType, usrName,TAF_DCS_USER_NAME_MAX_LEN,
                                       password, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAuthentication - OK");

    cmpVal = strncmp(usrName, "pap_user", TAF_DCS_USER_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth user name - OK");

    cmpVal = strncmp(password, "123", TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth password - OK");

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_CHAP, "chap_user", "123");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    result = taf_dcs_GetAuthentication(TestProfileRef, &authType, usrName,TAF_DCS_USER_NAME_MAX_LEN,
                                       password, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAuthentication - OK");
    LE_TEST_OK(authType == TAF_DCS_AUTH_CHAP, "Check auth type - OK");

    cmpVal = strncmp(usrName, "chap_user", TAF_DCS_USER_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth user name - OK");

    cmpVal = strncmp(password, "123", TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth password - OK");

    result = taf_dcs_SetAuthentication(TestProfileRef, TAF_DCS_AUTH_NONE, "", "");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_SetAuthentication(TestProfileRef2, TAF_DCS_AUTH_PAP, "pap_user", "123");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    result = taf_dcs_GetAuthentication(TestProfileRef2, &authType, usrName,
                                        TAF_DCS_USER_NAME_MAX_LEN,
                                       password, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAuthentication - OK");
    LE_TEST_OK(authType == TAF_DCS_AUTH_PAP, "Check auth type - OK");

    cmpVal = strncmp(usrName, "pap_user", TAF_DCS_USER_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth user name - OK");

    cmpVal = strncmp(password, "123", TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth password - OK");

    result = taf_dcs_SetAuthentication(TestProfileRef2, TAF_DCS_AUTH_CHAP, "chap_user", "123");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    result = taf_dcs_GetAuthentication(TestProfileRef2, &authType, usrName,
                                        TAF_DCS_USER_NAME_MAX_LEN,
                                       password, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAuthentication - OK");
    LE_TEST_OK(authType == TAF_DCS_AUTH_CHAP, "Check auth type - OK");

    cmpVal = strncmp(usrName, "chap_user", TAF_DCS_USER_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth user name - OK");

    cmpVal = strncmp(password, "123", TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth password - OK");

    result = taf_dcs_SetAuthentication(TestProfileRef2, TAF_DCS_AUTH_NONE, "", "");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    LE_TEST_END_SKIP();
}

void ut_get_roaming_status_test()
{
    bool isRoaming = false;
    taf_dcs_RoamingType_t type;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    le_result_t result = taf_dcs_GetRoamingStatus(PHONE_ID_1, &isRoaming, &type);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetRoamingStatus for phoneid(%d) - OK", PHONE_ID_1);

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    le_result_t result = taf_dcs_GetRoamingStatus(PHONE_ID_2, &isRoaming, &type);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetRoamingStatus for phoneid(%d) - OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_get_throttle_status_test()
{
    bool  areAllPLMNsThrottled;
    char mccStr[TAF_DCS_MCC_BYTES];
    char mncStr[TAF_DCS_MNC_BYTES];

    bool       isThrottled;
    uint32_t     ipv4RemainingTime;
    uint32_t     ipv6RemainingTime;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    le_result_t result = taf_dcs_GetAPNThrottledStatus(TestProfileRef,&isThrottled,&ipv4RemainingTime,
                                                                       &ipv6RemainingTime);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "taf_dcs_GetAPNThrottledStatus OK");

    LE_TEST_INFO("----isThrottled : %d", (bool)isThrottled);
    LE_TEST_INFO("----ipv4 Time : %d", (int)ipv4RemainingTime);
    LE_TEST_INFO("----ipv6 Time : %d", (int)ipv6RemainingTime);

    result = taf_dcs_GetAPNThrottledPLMN(TestProfileRef, &areAllPLMNsThrottled,
                                                       mccStr,TAF_DCS_MCC_BYTES,
                                                       mncStr,TAF_DCS_MNC_BYTES);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "taf_dcs_GetAPNThrottledPLMN OK");

    LE_TEST_INFO("----areAllPLMNsThrottled : %d", (bool)areAllPLMNsThrottled);
    LE_TEST_INFO("----MCC : %s", mccStr);
    LE_TEST_INFO("----MNC : %s", mncStr);

    LE_TEST_END_SKIP();

}

void ut_restore_apn_test()
{
    le_result_t result;
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_SetAPN(TestProfileRef, ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");
    result = taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");
    int cmpVal = strncmp(apnStr, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check apn - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_SetAPN(TestProfileRef2, ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN for phoneid(%d)- OK", PHONE_ID_2);
    result = taf_dcs_GetAPN(TestProfileRef2, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN for phoneid(%d)- OK", PHONE_ID_2);
    int cmpVal = strncmp(apnStr, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check apn - OK");

    LE_TEST_END_SKIP();
}

void ut_start_session_sync_test()
{
    le_result_t result;
    taf_dcs_ConState_t state;
    taf_dcs_DataBearerTechnology_t upTech, downTech;
    uint64_t RxBitRate, TxBitRate;
    taf_dcs_CallEndReasonType_t callEndReasonType;
    int32_t callEndReasonCode;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_GetSessionState(TestProfileRef, &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_DISCONNECTED, "taf_dcs_GetSessionState - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_UNKNOWN,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "UNKNOWN taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV4V6,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "IPV4V6 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV4,
                                                        &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "IPv4 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV6,
                                                        &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "IPv6 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetMaxDataBitRates(TestProfileRef, &RxBitRate, &TxBitRate);
    LE_TEST_OK(result == LE_UNAVAILABLE, "taf_dcs_GetMaxDataBitRates - OK");

    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession - OK");

    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - DUPLICATE");

    result = taf_dcs_GetSessionState(TestProfileRef, &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_CONNECTED, "taf_dcs_GetSessionState - OK");

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef, &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV4,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "IPv4 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV6,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE, "IPv6 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetMaxDataBitRates(TestProfileRef, &RxBitRate, &TxBitRate);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetMaxDataBitRates - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_StartSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetSessionState(TestProfileRef2, &state);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetSessionState for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef2, &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetCallEndReason(TestProfileRef2, TAF_DCS_PDP_IPV4,
                                                        &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE,
                                "IPv4 taf_dcs_GetCallEndReason for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetCallEndReason(TestProfileRef2, TAF_DCS_PDP_IPV6,
                                                        &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK || result == LE_UNAVAILABLE,
                                "IPv6 taf_dcs_GetCallEndReason for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetMaxDataBitRates(TestProfileRef2, &RxBitRate, &TxBitRate);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetMaxDataBitRates for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_stop_session_sync_test()
{
    le_result_t result;
    taf_dcs_DataBearerTechnology_t upTech, downTech;
    uint64_t RxBitRate, TxBitRate;
    taf_dcs_CallEndReasonType_t callEndReasonType;
    int32_t callEndReasonCode;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession - OK");

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef, &downTech, &upTech);
    LE_TEST_OK(result == LE_UNAVAILABLE, "taf_dcs_GetDataBearerTechnology - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV4,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK, "IPv4 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetCallEndReason(TestProfileRef, TAF_DCS_PDP_IPV6,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK, "IPv6 taf_dcs_GetCallEndReason - OK");

    result = taf_dcs_GetMaxDataBitRates(TestProfileRef, &RxBitRate, &TxBitRate);
    LE_TEST_OK(result == LE_UNAVAILABLE, "taf_dcs_GetMaxDataBitRates - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_StopSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef2, &downTech, &upTech);
    LE_TEST_OK(result == LE_UNAVAILABLE,
                                "taf_dcs_GetDataBearerTechnology for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetCallEndReason(TestProfileRef2, TAF_DCS_PDP_IPV4,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK,
               "IPv4 taf_dcs_GetCallEndReason for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetCallEndReason(TestProfileRef2, TAF_DCS_PDP_IPV6,
                                      &callEndReasonType, &callEndReasonCode);
    LE_TEST_OK(result == LE_OK,
               "IPv6 taf_dcs_GetCallEndReason for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetMaxDataBitRates(TestProfileRef2, &RxBitRate, &TxBitRate);
    LE_TEST_OK(result == LE_UNAVAILABLE,
                                    "taf_dcs_GetMaxDataBitRates for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_stop_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result,
                                                                                void* contextPtr)
{
    uint32_t profileId;
    uint8_t phoneId;
    le_result_t ret;

    LE_INFO("**** Handler for Stop Session Asynchronously (Begin)****");

    profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("profileId= %d, result: %d", profileId, result);

    ret = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(ret == LE_OK, "taf_dcs_GetPhoneId phoneid(%d)- OK", phoneId);

    LE_INFO("**** Handler for Stop Session Asynchronously (End)****");
    ut_restore_apn_test();
    LE_TEST_EXIT;
}

void ut_stop_session_async_handler_func2(taf_dcs_ProfileRef_t profileRef, le_result_t result,
                                                                                void* contextPtr)
{
    uint32_t profileId;
    uint8_t phoneId;
    le_result_t ret;

    LE_INFO("**** Handler for Stop Session Asynchronously (Begin)****");

    profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("profileId= %d, result: %d", profileId, result);

    ret = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(ret == LE_OK, "taf_dcs_GetPhoneId phoneid(%d)- OK", phoneId);

    LE_INFO("**** Handler for Stop Session Asynchronously (End)****");
    ut_restore_apn_test();
    LE_TEST_EXIT;
}

void ut_stop_session_async_test()
{

    LE_INFO("asynchronous stop session for phone id 1 %p",TestProfileRef);

    taf_dcs_StopSessionAsync(TestProfileRef, ut_stop_session_async_handler_func, NULL);

    LE_INFO("asynchronous stop session done");

}

void ut_stop_session_async_test2()
{

    LE_INFO("asynchronous stop session for phone id 2 %p",TestProfileRef2);

    taf_dcs_StopSessionAsync(TestProfileRef2, ut_stop_session_async_handler_func2, NULL);

    LE_INFO("asynchronous stop session done");

}

void ut_start_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result,
                                                                                void* contextPtr)
{
    uint32_t profileId;
    uint8_t phoneId;
    le_result_t ret;

    LE_INFO("**** Handler for Start Session Asynchronously (Begin)****");

    profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("profileId= %d, result: %d, profileRef=%p", profileId, result, profileRef);

    ret = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(ret == LE_OK, "taf_dcs_GetPhoneId: phoneId(%d) OK", phoneId);

    ut_stop_session_async_test();
}

void ut_start_session_async_handler_func2(taf_dcs_ProfileRef_t profileRef, le_result_t result,
                                                                                void* contextPtr)
{
    uint32_t profileId;
    uint8_t phoneId;
    le_result_t ret;

    LE_INFO("**** Handler for Start Session Asynchronously (Begin)****");

    profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_INFO("profileId= %d, result: %d", profileId, result);

    ret = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(ret == LE_OK, "taf_dcs_GetPhoneId: phoneId(%d) OK", phoneId);

    ut_stop_session_async_test2();
}

void ut_start_session_async_test()
{

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_INFO("asynchronous start session %p",TestProfileRef);

    taf_dcs_StartSessionAsync(TestProfileRef, ut_start_session_async_handler_func, NULL);

    LE_INFO("asynchronous start session done");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_INFO("asynchronous start session for phoneid 2 %p",TestProfileRef2);

    taf_dcs_StartSessionAsync(TestProfileRef2, ut_start_session_async_handler_func2, NULL);

    LE_INFO("asynchronous start session done");

    LE_TEST_END_SKIP();
}

void ut_set_pdp_test(taf_dcs_Pdp_t pdp)
{
    le_result_t result;
    taf_dcs_Pdp_t pdpGet;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    // Return LE_BAD_PARAMETER for TAF_DCS_PDP_UNKNOWN
    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_dcs_SetPDP UNKNOWN- OK");

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV4);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4- OK");

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV4, "taf_dcs_GetPDP IPV4- OK");

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV6);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV6- OK");

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV6, "taf_dcs_GetPDP IPV6- OK");

    result = taf_dcs_SetPDP(TestProfileRef, pdp);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP type- OK");

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_TEST_OK(pdpGet == pdp, "taf_dcs_GetPDP type- OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_SetPDP(TestProfileRef2, TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP UNKNOWN for phoneid(%d)- OK", PHONE_ID_2);

    pdpGet = taf_dcs_GetPDP(TestProfileRef2);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_UNKNOWN, "taf_dcs_GetPDP UNKNOWN for phoneid(%d)- OK",
                                                                                    PHONE_ID_2);

    result = taf_dcs_SetPDP(TestProfileRef2, TAF_DCS_PDP_IPV4);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4 for phoneid(%d)- OK", PHONE_ID_2);

    pdpGet = taf_dcs_GetPDP(TestProfileRef2);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV4, "taf_dcs_GetPDP IPV4 for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetPDP(TestProfileRef2, TAF_DCS_PDP_IPV6);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV6 for phoneid(%d)- OK", PHONE_ID_2);

    pdpGet = taf_dcs_GetPDP(TestProfileRef2);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV6, "taf_dcs_GetPDP IPV6 for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetPDP(TestProfileRef2, pdp);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP for phoneid(%d)- OK", PHONE_ID_2);

    pdpGet = taf_dcs_GetPDP(TestProfileRef2);
    LE_TEST_OK(pdpGet == pdp, "taf_dcs_GetPDP for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_set_apn_test()
{
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    char testApnStr[TAF_DCS_APN_NAME_MAX_LEN];
    taf_dcs_ApnType_t apnType, apnType_get;
    taf_dcs_ApnType_t apnType_set = TAF_DCS_APN_TYPE_DEFAULT | TAF_DCS_APN_TYPE_IMS |
                                                                        TAF_DCS_APN_TYPE_FOTA;

    le_result_t result;

    memset(apnStr, 0, TAF_DCS_APN_NAME_MAX_LEN);
    memset(testApnStr, 0, TAF_DCS_APN_NAME_MAX_LEN);

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_GetAPN(TestProfileRef, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");

    result = taf_dcs_SetAPN(TestProfileRef, testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    result = taf_dcs_GetAPN(TestProfileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");
    int cmpVal = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check apn value - OK");

    result = taf_dcs_GetApnTypes(TestProfileRef, &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - OK");

    result = taf_dcs_SetApnTypes(TestProfileRef, apnType_set);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes - OK");

    result = taf_dcs_GetApnTypes(TestProfileRef, &apnType_get);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - OK");

    result = taf_dcs_SetApnTypes(TestProfileRef, apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes - OK");

    result = taf_dcs_SetAPN(TestProfileRef, ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_GetAPN(TestProfileRef2, ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetAPN(TestProfileRef2, testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetAPN(TestProfileRef2, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN for phoneid(%d)- OK", PHONE_ID_2);
    int cmpVal = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "check apn for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetApnTypes(TestProfileRef2, &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetApnTypes(TestProfileRef2, apnType_set);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes for phoneid(%d)- OK", PHONE_ID_2);

    //Reset the APN type to get after setting.
    apnType_get = 0;
    result = taf_dcs_GetApnTypes(TestProfileRef2, &apnType_get);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetApnTypes(TestProfileRef2, apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetApnTypes for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_SetAPN(TestProfileRef2, ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN for phoneid 2- OK");

    LE_TEST_END_SKIP();

}

void ut_ipv4_check()
{
    le_result_t result;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef) == true, "taf_dcs_IsIPv4 - OK");

    result = taf_dcs_GetIPv4Address(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address - OK");

    result = taf_dcs_GetIPv4GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - OK");

    result = taf_dcs_GetIPv4DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                                ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef2) == true, "taf_dcs_IsIPv4 for phoneid(%d)- OK",
                                                                                    PHONE_ID_2);

    result = taf_dcs_GetIPv4Address(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv4GatewayAddress(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv4DNSAddresses(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                            ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();

}

void ut_non_ipv4_check()
{
    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef) == false, "ut_non_ipv4_check - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef2) == false, "ut_non_ipv4_check for phoneid(%d)- OK",
                                                                                        PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_ipv6_check()
{
    le_result_t result;
    char ipAddr0[TAF_DCS_IPV6_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV6_ADDR_MAX_LEN];

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef) == true, "taf_dcs_IsIPv6 - OK");

    result = taf_dcs_GetIPv6Address(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6Address - OK");

    result = taf_dcs_GetIPv6GatewayAddress(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6GatewayAddress - OK");

    result = taf_dcs_GetIPv6DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN,
                                                            ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef2) == true, "taf_dcs_IsIPv6 for phoneid(%d)- OK",
                                                                                        PHONE_ID_2);

    result = taf_dcs_GetIPv6Address(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6Address for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv6GatewayAddress(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6GatewayAddress for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv6DNSAddresses(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN,
                                                            ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_non_ipv6_check()
{
    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef) == false, "ut_non_ipv6_check - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef2) == false, "ut_non_ipv6_check for phoneid(%d)- OK",
                                                                                        PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_mdc_datacall_test()
{
    le_result_t result;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_IPV4V6);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4V6- OK");

    result = taf_mdc_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StartSession - OK");

    result = taf_mdc_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StopSession - OK");

    result = taf_mdc_StartSessionAsync(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StartSession - OK");

    result = taf_mdc_StopSessionAsync(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StopSession - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_SetPDP(TestProfileRef2, TAF_DCS_PDP_IPV4V6);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4V6 for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_mdc_StartSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StartSession - OK");

    result = taf_mdc_StopSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StopSession - OK");

    result = taf_mdc_StartSessionAsync(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StartSession - OK");

    result = taf_mdc_StopSessionAsync(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_mdc_StopSession - OK");

    LE_TEST_END_SKIP();
}

void ut_ipv4v6_async_datacall_test()
{
    ut_set_pdp_test(TAF_DCS_PDP_IPV4V6);

    ut_set_apn_test();

    ut_start_session_async_test();

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
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4V6;

    TestSemRef = le_sem_Create("taf_datacall_ut_sem", 0);

    ut_profile_list_test();

    ut_default_profile_set_get_test();

    ut_set_get_auth_test();

    ut_get_roaming_status_test();

    ut_get_throttle_status_test();

    ut_mdc_datacall_test();

    #if defined(TAF_CONFIG_SSIM_TEST) || defined(TAF_CONFIG_PHONE_ID_1_TEST)

    dataSessionThRef = le_thread_Create("dataSessionTh",
                                                         ut_taf_data_session_handler, &ipType);

    le_thread_Start(dataSessionThRef);

    le_sem_Wait(TestSemRef);

    #endif

    #ifdef TAF_CONFIG_PHONE_ID_2_TEST

    dataSessionThRef2 = le_thread_Create("dataSessionTh2",
                                                         ut_taf_data_session_handler2, &ipType);

    le_thread_Start(dataSessionThRef2);

    le_sem_Wait(TestSemRef);

    #endif

    le_thread_Ref_t roamingStatusThRef = le_thread_Create("RoamingStatusTh",
                                                           ut_taf_roaming_status_handler, NULL);

    le_thread_Start(roamingStatusThRef);

    le_sem_Wait(TestSemRef);

    le_thread_Ref_t throttleStatusThRef = le_thread_Create("ThrottleStatusTh",
                                                           ut_taf_throttle_status_handler, NULL);

    le_thread_Start(throttleStatusThRef);

    le_sem_Wait(TestSemRef);

    ut_ipv4v6_datacall_test();

    ut_ipv4_datacall_test();

    ut_ipv6_datacall_test();

    /* redo session connection test after testing invalid apn */
    ut_ipv4v6_datacall_test();

    ut_ipv4v6_async_datacall_test();

    sleep(3);

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);
    LE_INFO("---remove session %p",TestSessionStateRef);
    taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);
    LE_TEST_OK(le_thread_Cancel(dataSessionThRef) == LE_OK, "le_thread_Cancel session - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);
    LE_INFO("---remove session2 %p",TestSessionStateRef2);
    taf_dcs_RemoveSessionStateHandler(TestSessionStateRef2);
    LE_TEST_OK(le_thread_Cancel(dataSessionThRef2) == LE_OK, "le_thread_Cancel session - OK");

    LE_TEST_END_SKIP();

    taf_dcs_RemoveRoamingStatusHandler(TestRoamingStatusRef);

    LE_TEST_OK(le_thread_Cancel(roamingStatusThRef) == LE_OK, "le_thread_Cancel roaming - OK");

    taf_dcs_RemoveThrottledStatusHandler(TestThrottleStatusRef);

    LE_TEST_OK(le_thread_Cancel(throttleStatusThRef) == LE_OK, "le_thread_throttle roaming - OK");

    LE_TEST_INFO("Unit tests completed");
    return NULL;
}

static le_result_t testListProfileEx(taf_types_PhoneId_t phoneId)
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;

    result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
    if (LE_OK != result)
    {
        LE_TEST_INFO("taf_dcs_GetProfileListEx failed : %d", result);
        return result;
    }

    LE_TEST_INFO("-----got profile list for phone id %d, num: %zu, result: %d",
                 phoneId, listSize, result);
    LE_TEST_INFO("%-6s"
                 "%-6s"
                 "%-12s",
                 "Index", "type", "Name");
    for (uint32_t i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d"
                "%-6d"
                "%-12s",
                profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
    }
    return LE_OK;
}

//Set params to the profile ref
static void ut_profile_set_params(taf_dcs_ProfileRef_t ProfileRef, uint8_t count)
{
    le_result_t result;
    taf_dcs_Auth_t auth = TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP;
    taf_dcs_ApnType_t type = TAF_DCS_APN_TYPE_DEFAULT | TAF_DCS_APN_TYPE_IMS;
    taf_dcs_Tech_t pref = TAF_DCS_TECH_3GPP;
    taf_dcs_Pdp_t PDP = TAF_DCS_PDP_IPV4V6;
    std::string APNStr = "TestAPN" + std::to_string(count);
    std::string NameStr = "TestProfile" + std::to_string(count);
    std::string UNStr = "user" + std::to_string(count);
    std::string PWDStr = "pwd" + std::to_string(count);

    result = taf_dcs_SetAPN(ProfileRef, APNStr.c_str());
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetAPN");

    result = taf_dcs_SetProfileName(ProfileRef, NameStr.c_str());
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetProfileName");

    result = taf_dcs_SetPDP(ProfileRef, PDP);
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetPDP");

    result = taf_dcs_SetAuthentication(ProfileRef, auth, UNStr.c_str(), PWDStr.c_str());
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetAuthentication");

    result = taf_dcs_SetApnTypes(ProfileRef, type);
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetApnTypes");

    result = taf_dcs_SetTechPreference(ProfileRef, pref);
    LE_TEST_OK(LE_OK == result, "taf_dcs_SetTechPreference");
}

// Read params and verify
static void ut_profile_verify_params(taf_dcs_ProfileRef_t ProfileRef, uint8_t count)
{
    le_result_t result;
    uint32_t ProfileID = 0;
    uint8_t PhoneID = 0;

    // Expected
    taf_dcs_Auth_t auth = TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP;
    taf_dcs_ApnType_t type = TAF_DCS_APN_TYPE_DEFAULT | TAF_DCS_APN_TYPE_IMS;
    taf_dcs_Tech_t pref = TAF_DCS_TECH_3GPP;
    taf_dcs_Pdp_t PDP = TAF_DCS_PDP_IPV4V6;
    // Read
    taf_dcs_Auth_t ReadAuth = TAF_DCS_AUTH_NONE;
    taf_dcs_ApnType_t ReadType = TAF_DCS_APN_TYPE_MCX;
    taf_dcs_Tech_t ReadPref = TAF_DCS_TECH_UNKNOWN;
    taf_dcs_Pdp_t ReadPDP = TAF_DCS_PDP_UNKNOWN;

    // Expected
    std::string NameStr = "TestProfile" + std::to_string(count);
    std::string APNStr = "TestAPN" + std::to_string(count);
    std::string UNStr = "user" + std::to_string(count);
    std::string PWDStr = "pwd" + std::to_string(count);
    // Read
    char ReadNameStr[TAF_DCS_NAME_MAX_LEN] = {0};
    char ReadAPNStr[TAF_DCS_APN_NAME_MAX_LEN] = {0};
    char ReadUNStr[TAF_DCS_USER_NAME_MAX_LEN] = {0};
    char ReadPWDStr[TAF_DCS_PASSWORD_NAME_MAX_LEN] = {0};

    result = taf_dcs_GetProfileId(ProfileRef, &ProfileID);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileId: %d", ProfileID);

    result = taf_dcs_GetPhoneId(ProfileRef, &PhoneID);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetPhoneId: %d", PhoneID);

    result = taf_dcs_GetProfileName(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileName: %s", ReadNameStr);
    LE_TEST_OK(std::string(ReadNameStr) == NameStr, "Verify profile name");

    result = taf_dcs_GetAPN(ProfileRef, ReadAPNStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetAPN: %s", ReadAPNStr);
    LE_TEST_OK(std::string(ReadAPNStr) == APNStr.c_str(), "Verify APN");

    result = taf_dcs_GetApnTypes(ProfileRef, &ReadType);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetApnTypes: %d", ReadType);
    LE_TEST_OK(ReadType == type, "Verify APN type");

    result = taf_dcs_GetTechPreference(ProfileRef, &ReadPref);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetTechPreference: %d", ReadPref);
    LE_TEST_OK(ReadPref == pref, "Verify tech preference");

    result = taf_dcs_GetAuthentication(ProfileRef, &ReadAuth,
                                                ReadUNStr, TAF_DCS_USER_NAME_MAX_LEN,
                                                ReadPWDStr, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetAuthentication: %d, %s, %s", ReadAuth,
                                                               ReadUNStr, ReadPWDStr);
    LE_TEST_OK(ReadAuth == auth, "Verify auth type");
    LE_TEST_OK(std::string(ReadUNStr) == UNStr.c_str(), "Verify username");
    LE_TEST_OK(std::string(ReadPWDStr) == PWDStr.c_str(), "Verify password");

    ReadPDP = taf_dcs_GetPDP(ProfileRef);
    LE_TEST_OK(ReadPDP == PDP, "taf_dcs_GetPDP: %d, %d", ReadPDP, PDP);
}

static void ut_check_ret_with_data_apis(taf_dcs_ProfileRef_t ProfileRef, le_result_t expResult)
{
    le_result_t result;
    uint32_t    mask;
    taf_dcs_ConState_t ConState;
    bool bResult, expbResult;
    taf_dcs_DataBearerTechnology_t upLink, downLink;
    char ReadNameStr[TAF_DCS_NAME_MAX_LEN] = {0};
    char ReadNameStr2[TAF_DCS_NAME_MAX_LEN] = {0};
    taf_dcs_SessionStateHandlerRef_t StateHdlrRef;

    if (LE_OK == expResult)
    {
        expbResult = true;
    }
    else
    {
        expbResult = false;
    }

    result = taf_dcs_StartSession(ProfileRef);
    LE_TEST_OK(expResult == result, "taf_dcs_StartSession. Exp: %d, Act: %d", expResult, result);


    result = taf_dcs_StopSession(ProfileRef);
    LE_TEST_OK(expResult == result, "taf_dcs_StopSession. Exp: %d, Act: %d", expResult, result);


    StateHdlrRef = taf_dcs_AddSessionStateHandler(ProfileRef, NULL, NULL);
    if (LE_OK == expResult)
    {
        LE_TEST_OK(NULL != StateHdlrRef, "taf_dcs_AddSessionStateHandler. Exp: NonNULL, Act: %p",
                                                                                    StateHdlrRef);
    }
    else
    {
        LE_TEST_OK(NULL == StateHdlrRef, "taf_dcs_AddSessionStateHandler. Exp: NULL, Act: %p",
                                                                                    StateHdlrRef);
    }

    result = taf_dcs_GetInterfaceName(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetInterfaceName. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv4Address(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv4Address. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv4SubnetMask(ProfileRef, &mask);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv4SubnetMask. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv4GatewayAddress(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv4GatewayAddress. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv4DNSAddresses(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN,
                                                     ReadNameStr2, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv4DNSAddresses. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv6Address(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv6Address. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv6SubnetMask(ProfileRef, &mask);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv6SubnetMask. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv6GatewayAddress(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv6GatewayAddress. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetIPv6DNSAddresses(ProfileRef, ReadNameStr, TAF_DCS_NAME_MAX_LEN,
                                                     ReadNameStr2, TAF_DCS_NAME_MAX_LEN);
    LE_TEST_OK(expResult == result, "taf_dcs_GetIPv6DNSAddresses. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetSessionState(ProfileRef, &ConState);
    LE_TEST_OK(expResult == result, "taf_dcs_GetSessionState. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_dcs_GetDataBearerTechnology(ProfileRef, &upLink, &downLink);
    LE_TEST_OK(expResult == result, "taf_dcs_GetDataBearerTechnology. Exp: %d, Act: %d",
                                                                        expResult, result);

    bResult = taf_dcs_IsIPv4(ProfileRef);
    LE_TEST_OK(expbResult == bResult, "taf_dcs_IsIPv4. Exp: %d, Act: %d", expbResult, bResult);

    bResult = taf_dcs_IsIPv6(ProfileRef);
    LE_TEST_OK(expbResult == bResult, "taf_dcs_IsIPv6. Exp: %d, Act: %d", expbResult, bResult);

    result = taf_mdc_StartSession(ProfileRef);
    LE_TEST_OK(expResult == result, "taf_mdc_StartSession. Exp: %d, Act: %d",
                                                                        expResult, result);

    result = taf_mdc_StopSession(ProfileRef);
    LE_TEST_OK(expResult == result, "taf_mdc_StopSession. Exp: %d, Act: %d",
                                                                        expResult, result);
}

static void ut_profile_management_tests(void)
{
    le_result_t result;
    uint8_t step = 1;
    taf_dcs_ProfileRef_t ProfileRef = nullptr;
    taf_dcs_ProfileRef_t ProfileRef_2 = nullptr;

    // Profile create/update/delete/list test
    LE_TEST_INFO("Profile create/update/delete/list test");

    // List profiles
    result = testListProfileEx(TAF_TYPES_PHONE_ID_1);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileListEx");

    // Get a profile reference with ID TAF_DCS_UNDEFINED_PROFILE_ID
    ProfileRef = taf_dcs_GetProfileEx(TAF_TYPES_PHONE_ID_1, TAF_DCS_UNDEFINED_PROFILE_ID);
    LE_TEST_OK(nullptr != ProfileRef, "taf_dcs_GetProfileEx");
    if (nullptr == ProfileRef)
        LE_TEST_EXIT;

    // Try to get a profile ref with id TAF_DCS_UNDEFINED_PROFILE_ID again.
    // Server will return the same reference as earlier and a warning should be seen in the logs.
    ProfileRef_2 = taf_dcs_GetProfileEx(TAF_TYPES_PHONE_ID_1, TAF_DCS_UNDEFINED_PROFILE_ID);
    LE_TEST_OK(nullptr != ProfileRef_2, "taf_dcs_GetProfileEx 2");
    if (nullptr == ProfileRef_2)
        LE_TEST_EXIT;
    LE_TEST_INFO("### CHECK LE_WARN above ###");
    // Verify the same profile as before was returned
    LE_TEST_OK(ProfileRef_2 == ProfileRef, "Same profile reference returned as before");
    ProfileRef_2 = nullptr;

    // Set params first
    ut_profile_set_params(ProfileRef, step);

    // Read all set params
    ut_profile_verify_params(ProfileRef, step);

    // Ensure the service returns LE_NOT_POSSIBLE for data management APIs before the profile
    // is created
    LE_TEST_INFO("Test APIs return LE_NOT_POSSBILE as profile is not created yet");
    ut_check_ret_with_data_apis(ProfileRef, LE_NOT_POSSIBLE);

    // Create profile
    result = taf_dcs_CreateProfile(ProfileRef);
    LE_TEST_OK(LE_OK == result, "taf_dcs_CreateProfile");
    if (LE_OK != result)
        LE_TEST_EXIT;

    // List profiles. New profile should be present
    result = testListProfileEx(TAF_TYPES_PHONE_ID_1);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileListEx");

    // Read all set params after creating profile
    ut_profile_verify_params(ProfileRef, step);

    // Delete profile
    result = taf_dcs_DeleteProfile(ProfileRef);
    LE_TEST_OK(LE_OK == result, "taf_dcs_DeleteProfile");
    ProfileRef = NULL;

    // List profiles. New profile should be deleted
    result = testListProfileEx(TAF_TYPES_PHONE_ID_1);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileListEx");
}

// Promise to sync async commands
std::promise<le_result_t> AsyncAPIPromise;
void ut_asyncCmd_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
{
    LE_TEST_INFO("**** Async Handler Result: %d", result);
    AsyncAPIPromise.set_value(result);
}

static void *start_session_async_test(void *contextPtr)
{
    LE_TEST_INFO("Starting asynchronous data session");
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)contextPtr;
    taf_dcs_StartSessionAsync(ProfileRef, ut_asyncCmd_handler_func, NULL);
    return NULL;
}

static void *stop_session_async_test(void *contextPtr)
{
    LE_TEST_INFO("Starting asynchronous data session");
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)contextPtr;
    taf_dcs_StopSessionAsync(ProfileRef, ut_asyncCmd_handler_func, NULL);
    return NULL;
}

static void *async_cmd_thread_handler(void *ctxPtr)
{
    taf_dcs_ConnectService();
    le_sem_Post((le_sem_Ref_t)ctxPtr);
    le_event_RunLoop();
    return NULL;
}

// Test StartSessionAsync and StopSessionAsync withougt creating a profile and expect
// LE_NOT_POSSIBLE from the async handler
static void ut_async_cmd_tests_with_uncreated_profile()
{
    taf_dcs_ProfileRef_t ProfileRef = nullptr;
    le_result_t result, expResult = LE_NOT_POSSIBLE;
    le_thread_Ref_t asyncCmdThreadRef = NULL;
    le_sem_Ref_t asyncCmdSemRef;
    std::future<le_result_t> AsyncAPIPromiseFutureResult;

    asyncCmdSemRef = le_sem_Create("asyncCmdSem", 0);

    ProfileRef = taf_dcs_GetProfileEx(TAF_TYPES_PHONE_ID_1, TAF_DCS_UNDEFINED_PROFILE_ID);
    LE_TEST_OK(ProfileRef != NULL, "taf_dcs_GetProfileEx");
    if (ProfileRef == NULL)
        LE_TEST_EXIT;

    result = taf_dcs_StartSession(ProfileRef);
    LE_TEST_OK(expResult == result, "taf_dcs_StartSession. Exp: %d, Act: %d", expResult, result);


    asyncCmdThreadRef = le_thread_Create("async_cmd_thread", async_cmd_thread_handler,
                                                                            asyncCmdSemRef);
    le_thread_Start(asyncCmdThreadRef);
    le_sem_Wait(asyncCmdSemRef);

    AsyncAPIPromise = std::promise<le_result_t>();
    le_event_QueueFunctionToThread( asyncCmdThreadRef,
                                    (le_event_DeferredFunc_t)start_session_async_test,
                                    ProfileRef, NULL);
    AsyncAPIPromiseFutureResult = AsyncAPIPromise.get_future();
    result = AsyncAPIPromiseFutureResult.get();
    LE_TEST_OK(expResult == result, "taf_dcs_StartSessionAsync. Exp: %d, Act: %d",
                                                                            expResult, result);

    AsyncAPIPromise = std::promise<le_result_t>();
    le_event_QueueFunctionToThread(asyncCmdThreadRef,
                                   (le_event_DeferredFunc_t)stop_session_async_test,
                                   ProfileRef, NULL);
    AsyncAPIPromiseFutureResult = AsyncAPIPromise.get_future();
    result = AsyncAPIPromiseFutureResult.get();
    LE_TEST_OK(expResult == result, "taf_dcs_StopSessionAsync. Exp: %d, Act: %d",
                                                                            expResult, result);
    le_thread_Cancel(asyncCmdThreadRef);
}

static void ut_create_profile_test_data(std::string APNStr, std::string PDPStr)
{
    le_result_t result;
    taf_dcs_ProfileRef_t ProfileRef = nullptr;
    std::string NameStr = "Name" + PDPStr;

    taf_dcs_Pdp_t pdp;
    if (PDPStr == "IPV4")
    {
        pdp = TAF_DCS_PDP_IPV4;
    }
    else if (PDPStr == "IPV6")
    {
        pdp = TAF_DCS_PDP_IPV6;
    }
    else
    {
        pdp = TAF_DCS_PDP_IPV4V6;
    }

    // Get a profile reference with ID TAF_DCS_UNDEFINED_PROFILE_ID
    ProfileRef = taf_dcs_GetProfileEx(TAF_TYPES_PHONE_ID_1, TAF_DCS_UNDEFINED_PROFILE_ID);
    LE_TEST_ASSERT(nullptr != ProfileRef, "taf_dcs_GetProfileEx");

    // Set profile parameters
    result = taf_dcs_SetAPN(ProfileRef, APNStr.c_str());
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_SetAPN");

    result = taf_dcs_SetProfileName(ProfileRef, NameStr.c_str());
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_SetProfileName");

    result = taf_dcs_SetPDP(ProfileRef, pdp);
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_SetPDP");

    // Create profile
    result = taf_dcs_CreateProfile(ProfileRef);
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_CreateProfile");

    // Get the created profile ID
    uint32_t profileId;
    result = taf_dcs_GetProfileId(ProfileRef, &profileId);
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_GetProfileId");
    LE_TEST_INFO("Created profile Id: %d", profileId);

    // Start data session with the created profile
    result = taf_dcs_StartSession(ProfileRef);
    LE_TEST_OK(LE_OK == result, "taf_dcs_StartDataSession. Act: %d", result);

    // Store the start session result separately
    le_result_t startSessionResult = result;

    // Get the assigned IPv4 address
    if ((LE_OK == startSessionResult) && (TAF_DCS_PDP_IPV4 == pdp || TAF_DCS_PDP_IPV4V6 == pdp))
    {
        char IPV4Str[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
        result = taf_dcs_GetIPv4Address(ProfileRef, IPV4Str, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(LE_OK == result, "taf_dcs_GetIPv4Address. Act: %d", result);
        LE_TEST_INFO("IPv4 Address: %s", IPV4Str);
    }
    // Get the assigned IPv4 address
    if ((LE_OK == startSessionResult) && (TAF_DCS_PDP_IPV6 == pdp || TAF_DCS_PDP_IPV4V6 == pdp))
    {
        char IPV6Str[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
        result = taf_dcs_GetIPv6Address(ProfileRef, IPV6Str, TAF_DCS_IPV6_ADDR_MAX_LEN);
        LE_TEST_OK(LE_OK == result, "taf_dcs_GetIPv6Address. Act: %d", result);
        LE_TEST_INFO("IPv6 Address: %s", IPV6Str);
    }

    // If StartSession was OK, try to delete the profile and StopSession
    if (LE_OK == startSessionResult)
    {
        // Try to delete profile Should return LE_BUSY
        result = taf_dcs_DeleteProfile(ProfileRef);
        LE_TEST_ASSERT(LE_BUSY == result, "taf_dcs_DeleteProfile");
        // Stop data session
        result = taf_dcs_StopSession(ProfileRef);
        LE_TEST_ASSERT(LE_OK == result, "taf_dcs_StopSession");
    }
    // Delete profile
    result = taf_dcs_DeleteProfile(ProfileRef);
    LE_TEST_ASSERT(LE_OK == result, "taf_dcs_DeleteProfile");
}

/**
 * Entry point of this application
 */
COMPONENT_INIT
{
    size_t numArgs = le_arg_NumArgs();
    std::string testName;

    if (0 == numArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of args");
    }
    else
    {
        const char *testNameStr = le_arg_GetArg(0);
        if (NULL == testNameStr)
        {
            PrintUsage();
            LE_TEST_FATAL("Test name is empty");
        }
        testName = testNameStr;
    }

    LE_TEST_INIT;
    if ("Profile" == testName)
    {
        LE_TEST_INFO("Running profile management tests");
        ut_profile_management_tests();
        ut_async_cmd_tests_with_uncreated_profile();

        // Test data start/stop with a created profile
        if (numArgs > 1 )
        {
            if (3 == numArgs)
            {
                const char* APNStr = le_arg_GetArg(1);
                const char *PDPStr = le_arg_GetArg(2);
                if (NULL == APNStr)
                {
                    PrintUsage();
                    LE_TEST_FATAL("APN is empty");
                }
                if (NULL == PDPStr)
                {
                    PrintUsage();
                    LE_TEST_FATAL("PDP is empty");
                }
                std::string APN = APNStr;
                std::string PDP = PDPStr;

                LE_TEST_INFO("APN: %s, PDP: %s", APN.c_str(), PDP.c_str());
                if (PDP != "IPV4" && PDP != "IPV6" && PDP != "IPV4V6")
                {
                    PrintUsage();
                    LE_TEST_FATAL("Invalid PDP type: %s", PDP.c_str());
                }
                ut_create_profile_test_data(APN, PDP);
            }
            else
            {
                PrintUsage();
                LE_TEST_FATAL("Invalid number of args");
            }
        }
    }
    else if ("Full" == testName)
    {
        if (3 == numArgs)
        {
            const char *arg1Str = le_arg_GetArg(1);
            if (NULL == arg1Str)
            {
                LE_TEST_INFO("Using predefined profile ID with phone ID 1");
            }
            else
            {
                TEST_PROFILE_PHONEID_1 = std::stoul(arg1Str);
            }

            const char *arg2Str = le_arg_GetArg(2);
            if (NULL == arg2Str)
            {
                LE_TEST_INFO("Using predefined profile ID with phone ID 2");
            }
            else
            {
                TEST_PROFILE_PHONEID_2 = std::stoul(arg2Str);
            }

            LE_TEST_INFO("Profile ID used for tests with phone ID 1: %d", TEST_PROFILE_PHONEID_1);
            LE_TEST_INFO("Profile ID used for tests with phone ID 2: %d", TEST_PROFILE_PHONEID_2);
            LE_TEST_INFO("Running unit tests");
            UnitTestThread(NULL);
        }
        else
        {
            PrintUsage();
            LE_TEST_FATAL("Invalid number of args");
        }
    }
    else if ("Interactive" == testName)
    {
        // Provide a menu for the user to pick APIs from.
        LE_TEST_INFO("Showing menu..");
        tafDCSUnitTest_RunInteractiveTests();
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid test name: %s", testName.c_str());
    }
    LE_TEST_EXIT;
}
