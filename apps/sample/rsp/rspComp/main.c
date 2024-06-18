/*
* Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
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

#include "legato.h"
#include "interfaces.h"


static le_sem_Ref_t TestSemaphoreRef;
static taf_simRsp_ProfileDownloadHandlerRef_t ProfileDownloadHandlerRef;
static le_thread_Ref_t ThreadRef;
static taf_sim_Id_t SimId = TAF_SIM_EXTERNAL_SLOT_1;

static void PrintUsage () {
    puts("\n"
            "rsp -- eid\n"
            "rsp -- addprofile <ACTIVATION CODE> <USER CONSENT> <CONFIRMATION CODE> \n"
            "rsp -- deleteprofile <profileId> \n"
            "rsp -- enableprofile <profileId> \n"
            "rsp -- disableprofile <profileId> \n"
            "rsp -- list \n"
            "rsp -- getProfile <index> \n"
            "rsp -- nickname <profileId> <name>\n"
            "rsp -- getsmdp\n"
            "rsp -- setAddress <smdpAddress>\n"
            "\n");
}

static char* DownloadStatusToString(taf_simRsp_DownloadStatus_t status) {
    char *downloadStatus;
    switch(status) {
        case TAF_SIMRSP_DOWNLOAD_ERROR:
            downloadStatus = "DOWNLOAD_ERROR";
            break;
        case TAF_SIMRSP_DOWNLOAD_INSTALLATION_COMPLETE:
            downloadStatus = "DOWNLOAD_INSTALLATION_COMPLETE";
            break;
        default:
            downloadStatus = "UNKNOWN";
            break;
    }
    return downloadStatus;
}

static char* DownloadErrorCauseToString(taf_simRsp_DownloadErrorCause_t cause) {
    char *errorCause;
    switch(cause) {
        case TAF_SIMRSP_GENERIC:
            errorCause = "GENERIC";
            break;
        case TAF_SIMRSP_SIM:
            errorCause = "SIM";
            break;
        case TAF_SIMRSP_NETWORK:
            errorCause = "NETWORK";
            break;
        case TAF_SIMRSP_MEMORY:
            errorCause = "MEMORY";
            break;
        case TAF_SIMRSP_UNSUPPORTED_PROFILE_CLASS:
            errorCause = "UNSUPPORTED_PROFILE_CLASS";
            break;
        case TAF_SIMRSP_PPR_NOT_ALLOWED:
            errorCause = "PPR_NOT_ALLOWED";
            break;
        case TAF_SIMRSP_END_USER_REJECTION:
            errorCause = "END_USER_REJECTION";
            break;
        case TAF_SIMRSP_END_USER_POSTPONED:
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
    taf_simRsp_DownloadStatus_t       downloadStatus,
    taf_simRsp_DownloadErrorCause_t   downloadErrorCause,
    void* contextPtr
)
{
    printf("\nSlotId = %d\n", slotId);
    printf("\nDownloadStatus = %s\n", DownloadStatusToString(downloadStatus));
    printf("\nDownloadErrorCause = %s\n", DownloadErrorCauseToString(downloadErrorCause));

    le_sem_Post(TestSemaphoreRef);
}

static int GetEid() {
    char eidPtr[TAF_SIMRSP_EID_BYTES];
    size_t eidLen = TAF_SIMRSP_EID_BYTES;

    le_result_t result = taf_simRsp_GetEID(SimId, eidPtr, eidLen);
    if (result != LE_OK)
    {
        return EXIT_FAILURE;
    }
    printf("Eid = %s\n", eidPtr);
    return EXIT_SUCCESS;
}

static void* AddHandler(void* context) {

    taf_simRsp_ConnectService();

    ProfileDownloadHandlerRef = taf_simRsp_AddProfileDownloadHandler(ProfileDownloadHandler, NULL);
    LE_ASSERT(ProfileDownloadHandlerRef != NULL);
    LE_INFO("Profile Download Handler Added successfully ProfileDownloadHandlerRef = %p", ProfileDownloadHandlerRef);

    le_event_RunLoop();
    return NULL;
}

static void RemoveHandler(void* param1, void* param2) {

    LE_INFO("Remove Profile Downloader Handler MsgHandlerRef = %p", ProfileDownloadHandlerRef);
    taf_simRsp_RemoveProfileDownloadHandler(ProfileDownloadHandlerRef);

    le_sem_Post(TestSemaphoreRef);
}

static int AddProfile() {
    LE_INFO("Start Add profile test " );
    le_result_t result;
    if (le_arg_NumArgs() < 4) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    TestSemaphoreRef = le_sem_Create("RSPSem", 0);
    ThreadRef = le_thread_Create("taf_simRsp_test_thread", AddHandler, NULL);
    le_thread_Start(ThreadRef);

    const char* activationCode = le_arg_GetArg(2);
    const char* userConsentRequired = le_arg_GetArg(3);
    const char* confirmationCode = "";
    if (le_arg_NumArgs() > 4) {
        confirmationCode =  le_arg_GetArg(4);
    }
    if (activationCode == NULL) {
        printf("\n Activation code is not entered\n");
        return EXIT_FAILURE;
    }
    bool isUserConsentRequired = false;
    if (strcmp(userConsentRequired, "true") == 0) {
        isUserConsentRequired = true;
    }
    LE_ASSERT_OK(taf_simRsp_AddProfile(SimId, activationCode, confirmationCode, isUserConsentRequired));

    le_sem_Wait(TestSemaphoreRef);

    le_event_QueueFunctionToThread(ThreadRef, RemoveHandler, NULL, NULL);

    result = le_thread_Cancel(ThreadRef);
    LE_ASSERT(result == LE_OK);

    le_sem_Delete(TestSemaphoreRef);

    LE_INFO("Add profile done" );
    return EXIT_SUCCESS;
}


static int DeleteProfile() {
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* id = le_arg_GetArg(2);
    uint32_t profileId = atoi(id);
    LE_ASSERT_OK(taf_simRsp_DeleteProfile(SimId, profileId));
    LE_INFO("DeleteProfile done");
    return EXIT_SUCCESS;
}

static char* ProfileTypeToString(taf_simRsp_ProfileType_t profileType) {
    char *profile_type;
    switch(profileType) {
        case TAF_SIMRSP_REGULAR:
            profile_type = "REGULAR";
            break;
        case TAF_SIMRSP_EMERGENCY:
            profile_type = "EMERGENCY";
            break;
        default:
            profile_type = "UNKNOWN";
            break;
    }
    return profile_type;
}

static char* ProfileClassToString(taf_simRsp_ProfileClass_t profileClass) {
    char *profile_class;
    switch(profileClass) {
        case TAF_SIMRSP_TEST:
            profile_class = "TEST";
            break;
        case TAF_SIMRSP_PROVISIONING:
            profile_class = "PROVISIONING";
            break;
        case TAF_SIMRSP_OPERATIONAL:
            profile_class = "OPERATIONAL";
            break;
        default:
            profile_class = "UNKNOWN";
            break;
    }
    return profile_class;
}

static int GetProfileList() {
    taf_simRsp_ProfileListNodeRef_t    profileListPtr[TAF_SIMRSP_MAX_PROFILE];
    size_t count = 0;
    uint8_t i = 0;
    le_result_t     res;
    char            iccid[TAF_SIMRSP_ICCID_BYTES];
    char            nickName[TAF_SIMRSP_NICKNAME_BYTES];
    char            name[TAF_SIMRSP_NAME_BYTES];
    char            spn[TAF_SIMRSP_SPN_LEN];

    memset(iccid, 0, TAF_SIMRSP_ICCID_BYTES);
    memset(nickName, 0, TAF_SIMRSP_NICKNAME_BYTES);
    memset(name, 0, TAF_SIMRSP_NAME_BYTES);
    memset(spn, 0, TAF_SIMRSP_SPN_LEN);

    LE_INFO("GetProfileList: Retrieving profile list...");

    res = taf_simRsp_GetProfileList(SimId, profileListPtr, &count);

    LE_INFO("Get profile list: result %d, no of profiles: %" PRIuS, (int) res, count);

    if (res != LE_OK) {
        printf("No profile found!\n");
        return EXIT_FAILURE;
    }

    for (i = 0; i < count; i++) {
        if (profileListPtr[i] != NULL) {
            res = taf_simRsp_GetIccid(profileListPtr[i], iccid, sizeof(iccid));
            LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID done");
            res = taf_simRsp_GetNickName(profileListPtr[i], nickName, sizeof(nickName));
            LE_TEST_OK(res == LE_OK, "taf_simRsp_GetNickName done");
            res = taf_simRsp_GetName(profileListPtr[i], name, sizeof(name));
            LE_TEST_OK(res == LE_OK, "taf_simRsp_GetName done");
            res = taf_simRsp_GetSpn(profileListPtr[i], spn, sizeof(spn));
            LE_TEST_OK(res == LE_OK, "taf_simRsp_GetSpn done");

            printf("Profile# %d: \n", i+1);
            printf("ProfileId = %d,  ProfileType = %s,  iccid = %s,  isActive = %d, nickName = %s ",
                taf_simRsp_GetProfileIndex(profileListPtr[i]), ProfileTypeToString(taf_simRsp_GetProfileType(profileListPtr[i])),
                iccid, taf_simRsp_GetProfileActiveStatus(profileListPtr[i]), nickName);
            printf(" name = %s, spn = %s,  iconType = %d,  profileClass = %s, profileMask = %d \n ",
                name, spn,  taf_simRsp_GetIconType(profileListPtr[i]),
                ProfileClassToString(taf_simRsp_GetProfileClass(profileListPtr[i])), taf_simRsp_GetMask(profileListPtr[i]));
        }
    }

    return EXIT_SUCCESS;
}

static int GetProfileByIndex() {
    taf_simRsp_ProfileListNodeRef_t    profileListNodeRef;
    le_result_t     res;
    char            iccid[TAF_SIMRSP_ICCID_BYTES];
    char            nickName[TAF_SIMRSP_NICKNAME_BYTES];
    char            name[TAF_SIMRSP_NAME_BYTES];
    char            spn[TAF_SIMRSP_SPN_LEN];

    memset(iccid, 0, TAF_SIMRSP_ICCID_BYTES);
    memset(nickName, 0, TAF_SIMRSP_NICKNAME_BYTES);
    memset(name, 0, TAF_SIMRSP_NAME_BYTES);
    memset(spn, 0, TAF_SIMRSP_SPN_LEN);
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* id = le_arg_GetArg(2);
    uint32_t profileId = atoi(id);

    LE_INFO("GetProfileByIndex: Retrieving profile of index %d", profileId);

    profileListNodeRef = taf_simRsp_GetProfile(profileId);
    if (profileListNodeRef != NULL) {
        res = taf_simRsp_GetIccid(profileListNodeRef, iccid, sizeof(iccid));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID done");
        res = taf_simRsp_GetNickName(profileListNodeRef, nickName, sizeof(nickName));
        LE_TEST_OK(res == LE_OK, "taf_simRsp_GetNickName done");
        res = taf_simRsp_GetName(profileListNodeRef, name, sizeof(name));
        LE_TEST_OK(res == LE_OK, "taf_simRsp_GetName done");
        res = taf_simRsp_GetSpn(profileListNodeRef, spn, sizeof(spn));
        LE_TEST_OK(res == LE_OK, "taf_simRsp_GetSpn done");

        printf("Profile# %d: \n", profileId);
        printf("ProfileId = %d,  ProfileType = %s,  iccid = %s,  isActive = %d, nickName = %s ",
            taf_simRsp_GetProfileIndex(profileListNodeRef), ProfileTypeToString(taf_simRsp_GetProfileType(profileListNodeRef)),
            iccid, taf_simRsp_GetProfileActiveStatus(profileListNodeRef), nickName);
        printf(" name = %s, spn = %s,  iconType = %d,  profileClass = %s, profileMask = %d \n",
            name, spn,  taf_simRsp_GetIconType(profileListNodeRef),
            ProfileClassToString(taf_simRsp_GetProfileClass(profileListNodeRef)), taf_simRsp_GetMask(profileListNodeRef));
    }
    LE_INFO("GetProfileByIndex tests completed.");
    return EXIT_SUCCESS;
}

static int EnableProfile() {
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* id = le_arg_GetArg(2);
    uint32_t profileId = atoi(id);
    LE_ASSERT_OK(taf_simRsp_SetProfile(SimId, profileId, true));
    printf("\nProfile %d enabled \n", profileId);
    return EXIT_SUCCESS;
}

static int DisableProfile() {
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* id = le_arg_GetArg(2);
    uint32_t profileId = atoi(id);
    LE_ASSERT_OK(taf_simRsp_SetProfile(SimId, profileId, false));
    printf("\nProfile %d disabled \n", profileId);
    return EXIT_SUCCESS;
}

static int GetServerAddress() {
    char smdpAddress[TAF_SIMRSP_SMDP_LEN];
    char smdsAddress[TAF_SIMRSP_SMDP_LEN];
     memset(smdpAddress, 0 , TAF_SIMRSP_SMDP_LEN);
     memset(smdsAddress, 0 , TAF_SIMRSP_SMDP_LEN);
    taf_simRsp_GetServerAddress(SimId, smdpAddress , sizeof(smdpAddress), smdsAddress, sizeof(smdsAddress));
    printf("GetServerAddress smdpAddress = %s\n",smdpAddress);
    printf("GetServerAddress smdsAddress = %s\n",smdsAddress);
    return EXIT_SUCCESS;
}

static int SetServerAddress() {
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* smdpAddress = le_arg_GetArg(2);
    LE_ASSERT_OK(taf_simRsp_SetServerAddress(SimId, smdpAddress));
    printf("\nServer address set successfully\n");
    return EXIT_SUCCESS;
}

static int UpdateNickName() {
    if (le_arg_NumArgs() < 4) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* id = le_arg_GetArg(2);
    uint32_t profileId = atoi(id);
    const char* nickName = le_arg_GetArg(3);
    LE_ASSERT_OK(taf_simRsp_UpdateNickName(SimId, profileId, nickName));
    printf("\nNickname updated successfully\n");
    return EXIT_SUCCESS;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    SimId = taf_sim_GetSelectedCard();

    if (le_arg_NumArgs() == 0 )
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }

    const char* command = le_arg_GetArg(1);
    if (command == NULL || strcmp(command, "help") == 0)
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }

    if (strcmp(command, "eid") == 0)
    {
        status = GetEid();
    }
    else if (strcmp(command, "addprofile") == 0)
    {
        status = AddProfile();
    }
    else if (strcmp(command, "deleteprofile") == 0)
    {
        status = DeleteProfile();
    }
    else if (strcmp(command, "enableprofile") == 0)
    {
        status = EnableProfile();
    }
    else if (strcmp(command, "disableprofile") == 0)
    {
        status = DisableProfile();
    }
    else if (strcmp(command, "list") == 0)
    {
        status = GetProfileList();
    }
    else if (strcmp(command, "getProfile") == 0)
    {
        status = GetProfileByIndex();
    }
    else if (strcmp(command, "nickname") == 0)
    {
        status = UpdateNickName();
    }
    else if (strcmp(command, "getsmdp") == 0)
    {
        status = GetServerAddress();
    }
    else if (strcmp(command, "setAddress") == 0)
    {
        status = SetServerAddress();
    }
    exit(status);
}
