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

#define TAF_CONFIG_SSIM_TEST
#define TAF_CONFIG_PHONE_ID_1_TEST
//#define TAF_CONFIG_PHONE_ID_2_TEST

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

#define TEST_PROFILE         1 // profile id 1, used for phoneid 2
#define TEST_PROFILE_FIFTH   5 // profile id 5, used for phoneid 1

#define PHONE_ID_1     1
#define PHONE_ID_2     2

static le_sem_Ref_t TestSemRef;
static taf_dcs_ProfileRef_t TestProfileRef = NULL , TestProfileRef2 = NULL ;
le_thread_Ref_t dataSessionThRef = NULL, dataSessionThRef2 = NULL;
static taf_dcs_SessionStateHandlerRef_t TestSessionStateRef = NULL, TestSessionStateRef2 = NULL;
static taf_dcs_RoamingStatusHandlerRef_t TestRoamingStatusRef = NULL;
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
             profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

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
        LE_TEST_OK(profileId == TEST_PROFILE_FIFTH, "Disconnected: profile(%d)", profileId);

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);

        LE_TEST_END_SKIP();

    }

}

static void* ut_taf_data_session_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestSessionStateRef = taf_dcs_AddSessionStateHandler(TestProfileRef, (taf_dcs_SessionStateHandlerFunc_t)data_event_handler, ctxPtr);

    LE_TEST_OK(TestSessionStateRef != NULL, "ut_taf_data_session_handler - void");

    le_sem_Post(TestSemRef);

    le_event_RunLoop();

    return NULL;
}
#endif

#ifdef TAF_CONFIG_PHONE_ID_2_TEST
void data_event_handler2(taf_dcs_ProfileRef_t profileRef, taf_dcs_ConState_t callEvent, const taf_dcs_StateInfo_t *infoPtr, void* contextPtr)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;
    uint32_t profileId=0;
    uint8_t phoneId;
    le_result_t result = LE_OK;

    LE_INFO("get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
        profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

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
        LE_TEST_OK(profileId == TEST_PROFILE, "Disconnected: profile(%d)", profileId);

        result = taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);

    }

}

static void* ut_taf_data_session_handler2(void* ctxPtr)
{
    taf_dcs_ConnectService();

    TestSessionStateRef2 = taf_dcs_AddSessionStateHandler(TestProfileRef2, (taf_dcs_SessionStateHandlerFunc_t)data_event_handler2, ctxPtr);

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
    for (int i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
    }

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);
    result = taf_dcs_GetProfileListEx(PHONE_ID_1,profilesInfoPtr, &listSize);

    LE_TEST_OK(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d) - OK", PHONE_ID_1);
    LE_INFO("-----got profile list for phone id %d, num: %" PRIuS ", result: %d",
             PHONE_ID_1, listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (int i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
    }

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_GetProfileListEx(PHONE_ID_2,profilesInfoPtr, &listSize);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d) - OK", PHONE_ID_2);
    LE_INFO("-----got profile list for phone id %d, num: %" PRIuS ", result: %d",
             PHONE_ID_2, listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (int i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s", profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
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

    TestProfileRef = taf_dcs_GetProfile(TEST_PROFILE_FIFTH);
    LE_TEST_OK(TestProfileRef != NULL, "taf_dcs_GetProfile - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef);
    LE_TEST_OK(profileId == TEST_PROFILE_FIFTH, "taf_dcs_GetProfileIndex - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

    uint8_t phoneId;

    TestProfileRef = taf_dcs_GetProfileEx(PHONE_ID_1, TEST_PROFILE_FIFTH);
    LE_TEST_OK(TestProfileRef != NULL, "taf_dcs_GetProfileEx - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef);
    LE_TEST_OK(profileId == TEST_PROFILE_FIFTH, "taf_dcs_GetProfileIndex - OK");

    result = taf_dcs_GetPhoneId(TestProfileRef, &phoneId);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetPhoneId phoneId(%d) OK", phoneId);

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    uint8_t secPhoneId;

    TestProfileRef2 = taf_dcs_GetProfileEx(PHONE_ID_2, TEST_PROFILE);
    LE_TEST_OK(TestProfileRef2 != NULL, "taf_dcs_GetProfileEx - OK");

    profileId = taf_dcs_GetProfileIndex(TestProfileRef2);
    LE_TEST_OK(profileId == TEST_PROFILE, "taf_dcs_GetProfileIndex - OK");

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

    result = taf_dcs_GetAuthentication(TestProfileRef2, &authType, usrName,TAF_DCS_USER_NAME_MAX_LEN,
                                       password, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAuthentication - OK");
    LE_TEST_OK(authType == TAF_DCS_AUTH_PAP, "Check auth type - OK");

    cmpVal = strncmp(usrName, "pap_user", TAF_DCS_USER_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth user name - OK");

    cmpVal = strncmp(password, "123", TAF_DCS_PASSWORD_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check auth password - OK");

    result = taf_dcs_SetAuthentication(TestProfileRef2, TAF_DCS_AUTH_CHAP, "chap_user", "123");
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAuthentication - OK");

    result = taf_dcs_GetAuthentication(TestProfileRef2, &authType, usrName,TAF_DCS_USER_NAME_MAX_LEN,
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

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_GetSessionState(TestProfileRef, &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_DISCONNECTED, "taf_dcs_GetSessionState - OK");

    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession - OK");

    result = taf_dcs_StartSession(TestProfileRef);
    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - DUPLICATE");

    result = taf_dcs_GetSessionState(TestProfileRef, &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_CONNECTED, "taf_dcs_GetSessionState - OK");

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef, &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_StartSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetSessionState(TestProfileRef2, &state);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetSessionState for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetDataBearerTechnology(TestProfileRef2, &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_stop_session_sync_test()
{
    le_result_t result;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_StopSession(TestProfileRef);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    result = taf_dcs_StopSession(TestProfileRef2);
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_stop_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
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

void ut_stop_session_async_handler_func2(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
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

void ut_start_session_async_handler_func(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
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

void ut_start_session_async_handler_func2(taf_dcs_ProfileRef_t profileRef, le_result_t result, void* contextPtr)
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

    result = taf_dcs_SetPDP(TestProfileRef, TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP UNKNOWN- OK");

    pdpGet = taf_dcs_GetPDP(TestProfileRef);
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_UNKNOWN, "taf_dcs_GetPDP UNKNOWN- OK");

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
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_UNKNOWN, "taf_dcs_GetPDP UNKNOWN for phoneid(%d)- OK", PHONE_ID_2);

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
    char *testApnStr = "";
    taf_dcs_ApnType_t apnType;

    le_result_t result;

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

    result = taf_dcs_GetIPv4DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef2) == true, "taf_dcs_IsIPv4 for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv4Address(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv4GatewayAddress(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv4DNSAddresses(TestProfileRef2, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();

}

void ut_non_ipv4_check()
{
    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef) == false, "ut_non_ipv4_check - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv4(TestProfileRef2) == false, "ut_non_ipv4_check for phoneid(%d)- OK", PHONE_ID_2);

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

    result = taf_dcs_GetIPv6DNSAddresses(TestProfileRef, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef2) == true, "taf_dcs_IsIPv6 for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv6Address(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6Address for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv6GatewayAddress(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6GatewayAddress for phoneid(%d)- OK", PHONE_ID_2);

    result = taf_dcs_GetIPv6DNSAddresses(TestProfileRef2, ipAddr0, TAF_DCS_IPV6_ADDR_MAX_LEN, ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses for phoneid(%d)- OK", PHONE_ID_2);

    LE_TEST_END_SKIP();
}

void ut_non_ipv6_check()
{
    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef) == false, "ut_non_ipv6_check - OK");

    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(!PHONE_ID_2_TEST, 1);

    LE_TEST_OK(taf_dcs_IsIPv6(TestProfileRef2) == false, "ut_non_ipv6_check for phoneid(%d)- OK", PHONE_ID_2);

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

    LE_INFO("====all tests are passed");
    return NULL;
}

COMPONENT_INIT
{
    UnitTestThread(NULL);
}
