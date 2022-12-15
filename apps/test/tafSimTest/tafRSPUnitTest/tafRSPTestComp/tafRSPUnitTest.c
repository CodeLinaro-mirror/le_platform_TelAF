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

#define SLOT_ID  TAF_SIM_EXTERNAL_SLOT_1
#define ACTIVATION_CODE "LPA:1$mc4-poc5.otlabs.fr$339981214014959437"
#define PROFILE_NICKNAME "test"
static char SMDP_ADDRESS[] = "testAddress";

static le_sem_Ref_t TestSemaphoreRef;
static taf_rsp_ProfileDownloadHandlerRef_t ProfileDownloadHandlerRef;
static le_thread_Ref_t ThreadRef;
static uint32_t PROFILE_ID = 2;

static char* DownloadStatusToString(taf_rsp_DownloadStatus_t status) {
    char *downloadStatus;
    switch(status) {
        case TAF_RSP_DOWNLOAD_ERROR:
            downloadStatus = "DOWNLOAD_ERROR";
            break;
        case TAF_RSP_DOWNLOAD_INSTALLATION_COMPLETE:
            downloadStatus = "DOWNLOAD_INSTALLATION_COMPLETE";
            break;
        default:
            downloadStatus = "UNKNOWN";
            break;
    }
    return downloadStatus;
}

static char* DownloadErrorCauseToString(taf_rsp_DownloadErrorCause_t cause) {
    char *errorCause;
    switch(cause) {
        case TAF_RSP_GENERIC:
            errorCause = "GENERIC";
            break;
        case TAF_RSP_SIM:
            errorCause = "SIM";
            break;
        case TAF_RSP_NETWORK:
            errorCause = "NETWORK";
            break;
        case TAF_RSP_MEMORY:
            errorCause = "MEMORY";
            break;
        case TAF_RSP_UNSUPPORTED_PROFILE_CLASS:
            errorCause = "UNSUPPORTED_PROFILE_CLASS";
            break;
        case TAF_RSP_PPR_NOT_ALLOWED:
            errorCause = "PPR_NOT_ALLOWED";
            break;
        case TAF_RSP_END_USER_REJECTION:
            errorCause = "END_USER_REJECTION";
            break;
        case TAF_RSP_END_USER_POSTPONED:
            errorCause = "END_USER_POSTPONED";
            break;
        default:
            errorCause = "UNKNOWN";
            break;
    }
    return errorCause;
}

static void ProfileDownloadHandler
(
    taf_sim_Id_t                   slotId,
    taf_rsp_DownloadStatus_t       downloadStatus,
    taf_rsp_DownloadErrorCause_t   downloadErrorCause,
    void* contextPtr
)
{
    LE_INFO("ProfileDownloadHandler slotId = %d", slotId);
    LE_INFO("ProfileDownloadHandler downloadStatus = %s", DownloadStatusToString(downloadStatus));
    LE_INFO("ProfileDownloadHandler downloadErrorCause = %s", DownloadErrorCauseToString(downloadErrorCause));

    le_sem_Post(TestSemaphoreRef);
}

static void Test_taf_rsp_GetEid() {
    LE_INFO("Test_taf_rsp_GetEid start");
    char eidPtr[TAF_RSP_EID_BYTES];
    size_t eidLen = TAF_RSP_EID_BYTES;

    LE_TEST_OK(taf_rsp_GetEID(SLOT_ID, eidPtr, eidLen), "Test_taf_rsp_GetEid done");
    LE_INFO("Eid = %s", eidPtr);
    LE_INFO("Test_taf_rsp_GetEid done");
}

static void* Test_taf_rsp_AddHandler(void* context) {

    taf_rsp_ConnectService();

    ProfileDownloadHandlerRef = taf_rsp_AddProfileDownloadHandler(ProfileDownloadHandler, NULL);
    LE_ASSERT(ProfileDownloadHandlerRef != NULL);
    LE_INFO("Profile Download Handler Added successfully ProfileDownloadHandlerRef = %p", ProfileDownloadHandlerRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_rsp_RemoveHandler(void* param1, void* param2) {

    LE_INFO("Remove Profile Downloader Handler MsgHandlerRef = %p", ProfileDownloadHandlerRef);
    taf_rsp_RemoveProfileDownloadHandler(ProfileDownloadHandlerRef);

    le_sem_Post(TestSemaphoreRef);
}

static void Test_taf_rsp_AddProfile() {
    LE_INFO("Start Add profile test " );
    le_result_t result;
    TestSemaphoreRef = le_sem_Create("RSPSem", 0);
    ThreadRef = le_thread_Create("taf_rsp_test_thread", Test_taf_rsp_AddHandler, NULL);
    le_thread_Start(ThreadRef);
    ProfileDownloadHandlerRef = taf_rsp_AddProfileDownloadHandler(ProfileDownloadHandler, NULL);
    LE_ASSERT(ProfileDownloadHandlerRef != NULL);

    LE_ASSERT_OK(taf_rsp_AddProfile((taf_sim_Id_t)TAF_SIM_EXTERNAL_SLOT_1, ACTIVATION_CODE,"",false));

    le_sem_Wait(TestSemaphoreRef);

    le_event_QueueFunctionToThread(ThreadRef, Test_taf_rsp_RemoveHandler, NULL, NULL);

    result = le_thread_Cancel(ThreadRef);
    LE_ASSERT(result == LE_OK);

    le_sem_Delete(TestSemaphoreRef);

    LE_INFO("Add profile test done" );
}


static void Test_taf_rsp_DeleteProfile() {
    LE_DEBUG("Test_taf_rsp_DeleteProfile start");
    LE_ASSERT_OK(taf_rsp_DeleteProfile(SLOT_ID, PROFILE_ID));
    LE_DEBUG("Test_taf_rsp_DeleteProfile done");
}

static char* ProfileTypeToString(taf_rsp_ProfileType_t profileType) {
    char *profile_type;
    switch(profileType) {
        case TAF_RSP_REGULAR:
            profile_type = "REGULAR";
            break;
        case TAF_RSP_EMERGENCY:
            profile_type = "EMERGENCY";
            break;
        default:
            profile_type = "UNKNOWN";
            break;
    }
    return profile_type;
}

static char* ProfileClassToString(taf_rsp_ProfileClass_t profileClass) {
    char *profile_class;
    switch(profileClass) {
        case TAF_RSP_TEST:
            profile_class = "TEST";
            break;
        case TAF_RSP_PROVISIONING:
            profile_class = "PROVISIONING";
            break;
        case TAF_RSP_OPERATIONAL:
            profile_class = "OPERATIONAL";
            break;
        default:
            profile_class = "UNKNOWN";
            break;
    }
    return profile_class;
}

static void Test_taf_rsp_GetProfileList() {
    taf_rsp_ProfileListNodeRef_t    profileListPtr[TAF_RSP_MAX_PROFILE];
    size_t count;
    uint8_t i = 0;
    le_result_t     res;
    char            iccid[TAF_RSP_ICCID_BYTES];
    char            nickName[TAF_RSP_NICKNAME_BYTES];
    char            name[TAF_RSP_NAME_BYTES];
    char            spn[TAF_RSP_SPN_LEN];

    memset(iccid, 0, TAF_RSP_ICCID_BYTES);
    memset(nickName, 0, TAF_RSP_NICKNAME_BYTES);
    memset(name, 0, TAF_RSP_NAME_BYTES);
    memset(spn, 0, TAF_RSP_SPN_LEN);

    LE_INFO("Test_taf_rsp_GetProfileList: Retrieving profile list...");

    res = taf_rsp_GetProfileList(SLOT_ID, profileListPtr, &count);

    LE_INFO("Get profile list: result %d, no of profile: %d", (int) res, count);

    for (i = 0; i < count; i++) {
        if (profileListPtr[i] != NULL) {
            res = taf_rsp_GetIccid(profileListPtr[i], iccid, sizeof(iccid));
            LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID done");
            res = taf_rsp_GetNickName(profileListPtr[i], nickName, sizeof(nickName));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetNickName done");
            res = taf_rsp_GetName(profileListPtr[i], name, sizeof(name));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetName done");
            res = taf_rsp_GetSpn(profileListPtr[i], spn, sizeof(spn));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetSpn done");

            LE_INFO("Profile# %d: ", i+1);
            LE_INFO("ProfileId = %d ,  ProfileType = %s,  iccid = %s,  isActive = %d, nickName = %s ",
                taf_rsp_GetProfileIndex(profileListPtr[i]), ProfileTypeToString(taf_rsp_GetProfileType(profileListPtr[i])),
                iccid, taf_rsp_GetProfileActiveStatus(profileListPtr[i]), nickName);
            LE_INFO(" name = %s, spn = %s,  iconType = %d,  profileClass = %s, profileMask = %d \n ",
                name, spn,  taf_rsp_GetIconType(profileListPtr[i]),
                ProfileClassToString(taf_rsp_GetProfileClass(profileListPtr[i])), taf_rsp_GetMask(profileListPtr[i]));
        }
    }

}

static void Test_taf_rsp_GetProfileByIndex() {
    taf_rsp_ProfileListNodeRef_t    profileListNodeRef;
    uint8_t i = 0;
    le_result_t     res;
    char            iccid[TAF_RSP_ICCID_BYTES];
    char            nickName[TAF_RSP_NICKNAME_BYTES];
    char            name[TAF_RSP_NAME_BYTES];
    char            spn[TAF_RSP_SPN_LEN];

    memset(iccid, 0, TAF_RSP_ICCID_BYTES);
    memset(nickName, 0, TAF_RSP_NICKNAME_BYTES);
    memset(name, 0, TAF_RSP_NAME_BYTES);
    memset(spn, 0, TAF_RSP_SPN_LEN);

    LE_INFO("Test_taf_rsp_GetProfileByIndex: Retrieving first 3 profiles by index...");

    for (i = 0; i < 3; i++) {
        profileListNodeRef = taf_rsp_GetProfile(i);
        if (profileListNodeRef != NULL) {
            res = taf_rsp_GetIccid(profileListNodeRef, iccid, sizeof(iccid));
            LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID done");
            res = taf_rsp_GetNickName(profileListNodeRef, nickName, sizeof(nickName));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetNickName done");
            res = taf_rsp_GetName(profileListNodeRef, name, sizeof(name));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetName done");
            res = taf_rsp_GetSpn(profileListNodeRef, spn, sizeof(spn));
            LE_TEST_OK(res == LE_OK, "taf_rsp_GetSpn done");

            LE_INFO("Profile# %d:", i+1);
            LE_INFO("ProfileId = %d,  ProfileType = %s,  iccid = %s,  isActive = %d, nickName = %s ",
                taf_rsp_GetProfileIndex(profileListNodeRef), ProfileTypeToString(taf_rsp_GetProfileType(profileListNodeRef)),
                iccid, taf_rsp_GetProfileActiveStatus(profileListNodeRef), nickName);
            LE_INFO(" name = %s, spn = %s,  iconType = %d,  profileClass = %s, profileMask = %d \n ",
                name, spn,  taf_rsp_GetIconType(profileListNodeRef),
                ProfileClassToString(taf_rsp_GetProfileClass(profileListNodeRef)), taf_rsp_GetMask(profileListNodeRef));
        }
    }
    LE_INFO("Test_taf_rsp_GetProfileByIndex tests completed.");
}

static void Test_taf_rsp_EnableProfile() {
    LE_ASSERT_OK(taf_rsp_SetProfile((taf_sim_Id_t)TAF_SIM_EXTERNAL_SLOT_1, PROFILE_ID, true));
    LE_DEBUG("Test_taf_rsp_EnableProfile done");

}

static void Test_taf_rsp_DisableProfile() {
    LE_ASSERT_OK(taf_rsp_SetProfile((taf_sim_Id_t)TAF_SIM_EXTERNAL_SLOT_1, PROFILE_ID, false));
    LE_DEBUG("Test_taf_rsp_DisableProfile done");

}

static void Test_taf_rsp_GetServerAddress() {
    char smdpAddress[50];
    char smdsAddress[50];
     memset(smdpAddress, 0 , 50);
     memset(smdsAddress, 0 , 50);
    taf_rsp_GetServerAddress(SLOT_ID, smdpAddress , sizeof(smdpAddress), smdsAddress, sizeof(smdsAddress));
    LE_DEBUG("Test_taf_rsp_GetServerAddress smdpAddress = %s ",smdpAddress);
    LE_DEBUG("Test_taf_rsp_GetServerAddress smdsAddress = %s ",smdsAddress);
}

static void Test_taf_rsp_SetServerAddress() {
    LE_ASSERT_OK(taf_rsp_SetServerAddress(SLOT_ID, SMDP_ADDRESS));
    LE_DEBUG("Test_taf_rsp_SetServerAddress done");
}

static void Test_taf_rsp_UpdateNickName() {
    LE_ASSERT_OK(taf_rsp_UpdateNickName(SLOT_ID, PROFILE_ID, PROFILE_NICKNAME));
    LE_DEBUG("Test_taf_rsp_UpdateNickName done");
}

static void* StartUnitTestThread(void* contextPtr)
{

    // Test Read Eid
    Test_taf_rsp_GetEid();

    // Test read profile list
    Test_taf_rsp_GetProfileList();

    // Test get profile by index
    Test_taf_rsp_GetProfileByIndex();

    // Test add profile
    Test_taf_rsp_AddProfile();

    // Test Enable profile
    Test_taf_rsp_EnableProfile();

    // Test read profile list
    Test_taf_rsp_GetProfileList();

    // Test Disable Profile
    Test_taf_rsp_DisableProfile();
    // Test read profile list
    Test_taf_rsp_GetProfileList();

    // Test update nickname feature
    Test_taf_rsp_UpdateNickName();

    // Test delete profile
    Test_taf_rsp_DeleteProfile();

    // Test Read Server address
    Test_taf_rsp_GetServerAddress();

    // Test Set Server address
    Test_taf_rsp_SetServerAddress();

    LE_DEBUG("Test completed");
    return NULL;

}

COMPONENT_INIT
{
    StartUnitTestThread(NULL);
    exit(EXIT_SUCCESS);
}
