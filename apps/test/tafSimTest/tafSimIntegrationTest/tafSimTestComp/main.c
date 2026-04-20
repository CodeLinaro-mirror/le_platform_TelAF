/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "main.h"

static taf_sim_NewStateHandlerRef_t NewSimStateHandlerRef = NULL;
static taf_sim_IccidChangeHandlerRef_t IccidChangeHandlerRef = NULL;
taf_sim_RefreshChangeHandlerRef_t RefreshChangeHandlerRef = NULL;

static void DisplayAppUsage(void) {
    printf("Usage of the 'tafSimIntTest' application is:\n");
    printf("Test SIM state: app runProc tafSimIntTest --exe=tafSimIntTest -- state <slot1/slot2/unknown>\n");
    printf("Test SIM state change: app runProc tafSimIntTest --exe=tafSimIntTest -- events\n");
    printf("SIM information test: app runProc tafSimIntTest --exe=tafSimIntTest -- info <slot1/slot2/unknown>\n");
    printf("SIM informations test: app runProc tafSimIntTest --exe=tafSimIntTest -- infoAll <slot1/slot2/unknown>\n");
    printf("SIM selection test: app runProc tafSimIntTest --exe=tafSimIntTest -- select <slot1/slot2/unknown>\n");
    printf("SIM authentication test: app runProc tafSimIntTest --exe=tafSimIntTest -- enterPin <slot1/slot2/unknown> <pin1/pin2> pin\n");
    printf("SIM change pin  test: app runProc tafSimIntTest --exe=tafSimIntTest -- changePin <slot1/slot2/unknown> <pin1/pin2> old_pin new_pin\n");
    printf("SIM unblock  test: app runProc tafSimIntTest --exe=tafSimIntTest -- unblock <slot1/slot2/unknown> <puk1/puk2> puk new_pin\n");
    printf("SIM lock test: app runProc tafSimIntTest --exe=tafSimIntTest -- lock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM unlock test: app runProc tafSimIntTest --exe=tafSimIntTest -- unlock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM open logical channel test: app runProc tafSimIntTest --exe=tafSimIntTest -- openLogicalChannel <slot1/slot2/unknown> <AID>\n");
    printf("SIM close logical channel test: app runProc tafSimIntTest --exe=tafSimIntTest -- closeLogicalChannel <slot1/slot2/unknown> <Channel ID>\n");
    printf("SIM send APDU on logical channel test: app runProc tafSimIntTest --exe=tafSimIntTest -- sendApduOnChannel <slot1/slot2/unknown> <Channel ID> <CLA> <INS> <P1> <P2> <P3> [Data bytes in decimal e.g. 0 164 8 4 4 127 255 111 ...]\n");
    printf("SIM send APDU on logical channel test: app runProc tafSimIntTest --exe=tafSimIntTest -- sendApdu <slot1/slot2/unknown> <CLA> <INS> <P1> <P2> <P3> [Data bytes in decimal e.g. 0 164 0 4 2 63 0 ...]\n");
    printf("SIM send command test: app runProc tafSimIntTest --exe=tafSimIntTest -- sendCommand <slot1/slot2/unknown>\n");
    printf("SIM get app types: app runProc tafSimIntTest --exe=tafSimIntTest -- getAppType <slot1/slot2/unknown>\n");
    printf("SIM access test: app runProc tafSimIntTest --exe=tafSimIntTest -- access <slot1/slot2/unknown>\n");
    printf("SIM SetPower test: app runProc tafSimIntTest --exe=tafSimIntTest -- setPower <slot1/slot2/unknown> <ON/OFF>\n");
    printf("SIM Reset test: app runProc tafSimIntTest --exe=tafSimIntTest -- Reset <slot1/slot2/unknown>\n");
    printf("SIM EMERGENCY test: app runProc tafSimIntTest --exe=tafSimIntTest -- isEmergency <slot1/slot2/unknown>\n");
    printf("SIM Swap Profiles test: app runProc tafSimIntTest --exe=tafSimIntTest -- swapProfiles <slot1/slot2/unknown> <0/1/2/3/4/5>\n");
    printf("Get Forbidden PLMN list: app runProc tafSimIntTest --exe=tafSimIntTest -- fplmnList <slot1/slot2/unknown>\n");
    printf("Create Forbidden PLMN list: app runProc tafSimIntTest --exe=tafSimIntTest -- createFplmnList <slot1/slot2/unknown>\n");
    printf("SIM refresh: app runProc tafSimIntTest --exe=tafSimIntTest -- refresh <slot1/slot2/unknown> <Session type> <Refresh mode> <Refresh allow>\n");
}

static taf_sim_Id_t GetSimId(const char* simIdPtr) {
    if(strcmp(simIdPtr, "slot1") == 0) {
        return TAF_SIM_EXTERNAL_SLOT_1;
    } else if(strcmp(simIdPtr, "slot2") == 0) {
        return TAF_SIM_EXTERNAL_SLOT_2;
    } else if(strcmp(simIdPtr, "unknown") == 0) {
        return TAF_SIM_UNSPECIFIED;
    }
    LE_ERROR("Unable to convert '%s' to a taf_sim_Id_t", simIdPtr);
    DisplayAppUsage();
    exit(EXIT_FAILURE);
}

static le_onoff_t GetPowerStatus(const char* powerStatusPtr)
{
    if(strcmp(powerStatusPtr, "ON")==0)
    {
        return LE_ON;
    } else if(strcmp(powerStatusPtr, "OFF")==0)
    {
        return LE_OFF;
    }
    LE_ERROR("Unable to convert '%s' to a powerStatusPtr", powerStatusPtr);
    DisplayAppUsage();
    exit(EXIT_FAILURE);
}

static taf_sim_LockType_t GetLockType(const char* lockPtr) {
    if(strcmp(lockPtr,"pin1") == 0) {
        return TAF_SIM_PIN1;
    } else if(strcmp(lockPtr,"pin2") == 0) {
        return TAF_SIM_PIN2;
    } else if(strcmp(lockPtr,"puk1") == 0) {
        return TAF_SIM_PUK1;
    } else if(strcmp(lockPtr,"puk2") == 0) {
        return TAF_SIM_PUK2;
    } else if(strcmp(lockPtr,"fdn") == 0) {
        return TAF_SIM_FDN;
    }
    LE_ERROR("Unable to convert '%s' to a lockType", lockPtr);
    DisplayAppUsage();
    exit(EXIT_FAILURE);
}

static void TestNewSimStateHandler(taf_sim_Id_t simId, taf_sim_States_t simState,
        void* contextPtr){
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %s", SimStateToString(simState));
}

static void TestIccidChangeHandler(taf_sim_Id_t simId, const char* Iccid, void* contextPtr) {
    LE_INFO("Iccid Change event for SIM card: %d", simId);
    LE_INFO("ICCID is: %s", (const char*)Iccid);
}

static void TestAuthenticationResponse
(
    taf_sim_Id_t     simId,
    taf_sim_LockResponse_t responseType,
    le_result_t result,
    void* contextPtr
)
{
    LE_INFO("Authentication Response for SIM card: %d", simId);

    if (LE_OK == result) {
        switch(responseType) {
            case TAF_SIM_CHANGE_PIN:
                LE_INFO("Change pin successfull");
                break;
            case TAF_SIM_UNLOCK_BY_PIN:
                LE_INFO("Enter pin successfull");
                break;
            case TAF_SIM_SET_LOCK:
                LE_INFO("lock/unlock successfull");
                break;
            case TAF_SIM_UNLOCK_BY_PUK:
                LE_INFO("unblock successfull");
                break;
            default:
                LE_INFO("Unknown response");
                exit(EXIT_FAILURE);
        }
    } else {
            LE_INFO("Error: %s\n", LE_RESULT_TXT(result));
            LE_INFO("Remaining PIN tries: %d\n", taf_sim_GetRemainingPINTries(simId));
            exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

static void TestRefreshChangeHandler
(
    taf_sim_RefreshStatus_t status,
    void* contextPtr
)
{
    LE_INFO("TestRefreshChangeHandler: status %d\n", (int) status);

    if ((status & TAF_SIM_REFRESH_STATUS_SUCCESS) != 0) {
        printf("Refresh success.\n");
    }
    if ((status & TAF_SIM_REFRESH_STATUS_FAILURE) != 0) {
        printf("Refresh failed!\n");
    }
    if ((status & TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH) != 0) {
        printf("Profile switch success.\n");
    }
    if ((status & TAF_SIM_REFRESH_STATUS_FILE_CHANGE) != 0) {
        printf("File change success.\n");
    }

}
static int read_line(char *buf, size_t bufsz)
{
    if (fgets(buf, bufsz, stdin) == NULL) return -1;
    buf[strcspn(buf, "\r\n")] = '\0';   // remove newline
    return 0;
}
COMPONENT_INIT
{
    taf_sim_Id_t simId = taf_sim_GetSelectedCard();
    bool exitApplication = true;
    const char* testType = "";
    le_result_t res = LE_FAULT;

    taf_sim_Id_t simIdOrig = simId;

    LE_INFO("Start tafSimIntTest app.");
    int NumberOfArgs = le_arg_NumArgs();

    if (NumberOfArgs >= 1) {
        testType = le_arg_GetArg(0);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }
    }
    if (NumberOfArgs > 1) {
        const char* simIdPtr = le_arg_GetArg(1);
        if (NULL == simIdPtr)
        {
            LE_ERROR("cardIdPtr is NULL");
            exit(EXIT_FAILURE);
        }
        simId = GetSimId(simIdPtr);
    }

    LE_INFO("TafSimIntgTest testType: %s", testType);

    if (NumberOfArgs > 2) {
        taf_sim_AuthenticationResponseHandlerRef_t responseHandlerRef_t;
        responseHandlerRef_t =taf_sim_AddAuthenticationResponseHandler(TestAuthenticationResponse, NULL);
        LE_ASSERT(responseHandlerRef_t != NULL);
    }

    if (strcmp(testType, "state") == 0) {
        if (NumberOfArgs > 1)
        {
            tafSimTest_state(simId);
        }
        else
        {
            tafSimTest_allState();
        }
    } else if (strcmp(testType, "events") == 0) {
        IccidChangeHandlerRef = taf_sim_AddIccidChangeHandler(TestIccidChangeHandler, NULL);
        LE_ASSERT(IccidChangeHandlerRef!=NULL);
        NewSimStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
        LE_ASSERT(NewSimStateHandlerRef!=NULL);
        exitApplication = false;
    }
    // Test: sim identification info
    else if (strcmp(testType, "info") == 0)
    {
        if (NumberOfArgs > 1)
        {
            tafSimTest_info(simId);
        }
        else
        {
            tafSimTest_allInfo();
        }
    }
    else if (strcmp(testType, "infoAll") == 0)
    {
        tafSimTest_allInfo();
    }
    else if (strcmp(testType, "select") == 0)
    {
        tafSimTest_selection(simId);
    }
    else if (strcmp(testType, "enterPin") == 0)
    {
        const char* lockPtr = le_arg_GetArg(2);
        if (NULL == lockPtr)
        {
            LE_ERROR("lockPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_LockType_t lockType = GetLockType(lockPtr);
        const char* pinPtr = le_arg_GetArg(3);
        if (NULL == pinPtr)
        {
            LE_ERROR("pinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        res = tafSimTest_enterPin(simId,lockType,pinPtr);
        if (res == LE_OK) {
            exitApplication = false;
        } else {
            exitApplication = true;
        }
    }
    else if (strcmp(testType, "changePin") == 0)
    {
        const char* lockPtr = le_arg_GetArg(2);
        if (NULL == lockPtr)
        {
            LE_ERROR("lockPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_LockType_t lockType = GetLockType(lockPtr);
        const char* oldpinPtr = le_arg_GetArg(3);
        if (NULL == oldpinPtr)
        {
            LE_ERROR("oldpinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        const char* newpinPtr = le_arg_GetArg(4);
        if (NULL == newpinPtr)
        {
            LE_ERROR("newpinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        res = tafSimTest_Change_pin(simId, lockType, oldpinPtr, newpinPtr);
        if (res == LE_OK) {
            exitApplication = false;
        } else {
            exitApplication = true;
        }
    }
    else if (strcmp(testType, "unblock") == 0)
    {
        const char* lockPtr = le_arg_GetArg(2);
        if (NULL == lockPtr)
        {
            LE_ERROR("lockPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_LockType_t lockType = GetLockType(lockPtr);
        const char* pukPtr = le_arg_GetArg(3);
        if (NULL == pukPtr)
        {
            LE_ERROR("pukPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        const char* pinPtr = le_arg_GetArg(4);
        if (NULL == pinPtr)
        {
            LE_ERROR("pinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        res = tafSimTest_unblock_puk(simId, lockType, pukPtr, pinPtr);
        if (res == LE_OK) {
            exitApplication = false;
        } else {
            exitApplication = true;
        }
    }
    else if (strcmp(testType, "lock") == 0)
    {
        const char* lockPtr = le_arg_GetArg(2);
        if (NULL == lockPtr)
        {
            LE_ERROR("lockPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_LockType_t lockType = GetLockType(lockPtr);
        const char* pinPtr = le_arg_GetArg(3);
        if (NULL == pinPtr)
        {
            LE_ERROR("pinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        res = tafSimTest_setLock(simId,lockType,pinPtr, true);
        if (res == LE_OK) {
            exitApplication = false;
        } else {
            exitApplication = true;
        }
    }
    else if (strcmp(testType, "unlock") == 0)
    {
        const char* lockPtr = le_arg_GetArg(2);
        if (NULL == lockPtr)
        {
            LE_ERROR("lockPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_LockType_t lockType = GetLockType(lockPtr);
        const char* pinPtr = le_arg_GetArg(3);
        if (NULL == pinPtr)
        {
            LE_ERROR("pinPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        res = tafSimTest_setLock(simId,lockType,pinPtr, false);
        if (res == LE_OK) {
            exitApplication = false;
        } else {
            exitApplication = true;
        }

    }
    else if (strcmp(testType, "getAppType") == 0)
    {
        tafSimTest_GetAppTypes(simId);
    }
    else if (strcmp(testType, "access") == 0)
    {
        tafSimTest_sim_access(simId);
    }
    else if (strcmp(testType, "openLogicalChannel") == 0)
    {
        const char* aid = le_arg_GetArg(2);
        if (NULL == aid)
        {
            LE_ERROR("Aid is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        int aidLen = strlen(aid);

        LE_INFO("Length of AID: %d", aidLen);

        if (aidLen > TAF_SIM_AID_BYTES) {
            printf("Invalid length (%d) of AID. AID max length: %d.\n", aidLen, TAF_SIM_AID_BYTES);
        }

        tafSimTest_sim_openLogicalChannel(simId, aid);
    }
    else if (strcmp(testType, "closeLogicalChannel") == 0)
    {
        const char* channelIdPtr = le_arg_GetArg(2);
        if (NULL == channelIdPtr)
        {
            LE_ERROR("ChannelIdPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        uint8_t channelId = atoi(channelIdPtr);
        tafSimTest_sim_closeLogicalChannel(simId, channelId);
    }
    else if (strcmp(testType, "sendApduOnChannel") == 0)
    {
        int count = le_arg_NumArgs();
        if (count < 8) {
            LE_ERROR("Insufficient arguments provided.\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        const char* channelIdPtr = le_arg_GetArg(2);
        const char* claPtr = le_arg_GetArg(3);
        const char* insPtr = le_arg_GetArg(4);
        const char* p1Ptr = le_arg_GetArg(5);
        const char* p2Ptr = le_arg_GetArg(6);
        const char* p3Ptr = le_arg_GetArg(7);
        if (!channelIdPtr || !claPtr
            || !insPtr || !p1Ptr
            || !p2Ptr || !p3Ptr)
        {
            LE_ERROR("One or more input parameters are NULL.\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        uint8_t channelId = atoi(channelIdPtr);
        uint8_t cla = atoi(claPtr);
        uint8_t ins = atoi(insPtr);
        uint8_t p1 = atoi(p1Ptr);
        uint8_t p2 = atoi(p2Ptr);
        uint8_t p3 = atoi(p3Ptr);

        uint8_t requestApdu[TAF_SIM_APDU_MAX_BYTES] = {0};
        requestApdu[0] = cla;
        requestApdu[1] = ins;
        requestApdu[2] = p1;
        requestApdu[3] = p2;
        requestApdu[4] = p3;
        for (int i = 0; i < (count-8) && i < (TAF_SIM_APDU_MAX_BYTES-5); i++) {
            const char* bytePtr = le_arg_GetArg(i+8);
            if (!bytePtr) {
                LE_ERROR("Invalid byte argument at index %d.\n", i + 8);
                exit(EXIT_FAILURE);
            }

            int byte = atoi(bytePtr);
            if (byte < 0 || byte > 255) {
                printf("Invalid input %d (Valid range: 0 to 255).\n", byte);
                printf("Failed! Please try again...\n");
                exit(EXIT_FAILURE);
            }

            requestApdu[i+5] = (uint8_t) byte;
        }
        requestApdu[0] = channelId;
        tafSimTest_sim_sendApduOnChannel(simId, channelId, requestApdu, (count-3));
    }
    else if (strcmp(testType, "sendApdu") == 0)
    {
        int count = le_arg_NumArgs();
        if (count < 7) {
            LE_ERROR("Insufficient arguments provided.\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        const char* claPtr = le_arg_GetArg(2);
        const char* insPtr = le_arg_GetArg(3);
        const char* p1Ptr = le_arg_GetArg(4);
        const char* p2Ptr = le_arg_GetArg(5);
        const char* p3Ptr = le_arg_GetArg(6);
        if (!claPtr || !insPtr ||
            !p1Ptr || !p2Ptr || !p3Ptr)
        {
            LE_ERROR("One or more input parameters are NULL.\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        uint8_t cla = atoi(claPtr);
        uint8_t ins = atoi(insPtr);
        uint8_t p1 = atoi(p1Ptr);
        uint8_t p2 = atoi(p2Ptr);
        uint8_t p3 = atoi(p3Ptr);

        uint8_t requestApdu[TAF_SIM_APDU_MAX_BYTES] = {0};
        requestApdu[0] = cla;
        requestApdu[1] = ins;
        requestApdu[2] = p1;
        requestApdu[3] = p2;
        requestApdu[4] = p3;
        for (int i = 0; i < (count-7) && i < (TAF_SIM_APDU_MAX_BYTES-5); i++) {
            const char* bytePtr = le_arg_GetArg(i+7);
            if (!bytePtr) {
               LE_ERROR("Invalid byte argument at index %d.\n", i + 7);
               exit(EXIT_FAILURE);
            }
            int byte = atoi(bytePtr);
            if (byte < 0 || byte > 255) {
                printf("Invalid input %d (Valid range: 0 to 255).\n", byte);
                printf("Failed! Please try again...\n");
                exit(EXIT_FAILURE);
            }

            requestApdu[i+5] = (uint8_t) byte;
        }

        tafSimTest_sim_sendApdu(simId, requestApdu, (count-2));
    }
    else if (strcmp(testType, "sendCommand") == 0)
    {
        tafSimTest_sim_sendCommand(simId);
    }
    else if(strcmp(testType, "swapProfiles") == 0)
    {
        const char* manufacturerPtr = le_arg_GetArg(2);
        if (NULL == manufacturerPtr)
        {
            LE_ERROR("manufacturerPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_Manufacturer_t manufacturer = (taf_sim_Manufacturer_t) atoi(manufacturerPtr);
        tafSimTest_swapToEmergencyAndBack(simId, manufacturer);
    }
    else if (strcmp(testType, "setPower") == 0)
    {
        const char* powerStatusPtr = le_arg_GetArg(2);
        if(NULL == powerStatusPtr)
        {
            LE_ERROR("Power status argument is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        le_onoff_t powerStatus = GetPowerStatus(powerStatusPtr);
        tafSimTest_SetPowerCheck(simId, powerStatus);
    }
    else if (strncmp(testType, "Reset", 5) == 0)
    {
        LE_ASSERT_OK(taf_sim_Reset(simId));
        LE_INFO("SIM Reset successfull");
    }
    else if (strncmp(testType, "isEmergency", 11) == 0)
    {
        tafSimTest_sim_isEmergency(simId);
    }

    else if (strncmp(testType, "fplmnList", 9) == 0)
    {
        char line[64];
        int ninstance = 0;
        printf("Enter the number of instance: ");
        if (read_line(line, sizeof(line)) != 0)
        {
            LE_ERROR("Invalid instance count");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        ninstance = (int)strtol(line, NULL, 10);
        if (ninstance <= 0)
        {
            LE_ERROR("Invalid instance count");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        char mcc[ninstance][4];
        char mnc[ninstance][3];

        for (int i = 0; i < ninstance; i++)
        {
            printf("Enter mcc for fplmnList[%d]: ", i + 1);
            if (read_line(line, sizeof(line)) != 0)
            {   exitApplication = true;
                return;
            }
            if (strlen(line) < 3)
            {
                LE_ERROR("MCC too short");
                exitApplication = true;
                return;
            }
            snprintf(mcc[i], sizeof(mcc[i]), "%.3s", line);
            printf("Enter mnc for fplmnList[%d]: ", i + 1);
            if (read_line(line, sizeof(line)) != 0) {
                exitApplication = true;
                return;
            }
            if (strlen(line) < 2) {
                LE_ERROR("MNC too short");
                exitApplication = true;
                return;
            }
            snprintf(mnc[i], sizeof(mnc[i]), "%.2s", line);
        }
        exitApplication = false;
        le_result_t res = LE_FAULT;
        res = tafSimTest_createFplmnList_test(simId);
        if(res == LE_OK)
        {
            exitApplication = false;
            if (ninstance <= 0) {
                LE_ERROR("No instances to process");
                exitApplication = true;
                return;
            }
            for (int i = 0; i < ninstance; i++)
            {
                res = tafSimTest_addFplmnOperator_test(simId, mcc[i], mnc[i]);
                if (res != LE_OK)
                {
                    LE_ERROR("Failed to add FPLMN operator at instance %d", i + 1);
                    exitApplication = true;
                    return;
                }
            }
        }
        if(res == LE_OK)
        {
            res = tafSimTest_writeFplmnList_test(simId);
            if(res != LE_OK)
            {
                exitApplication = true;
                return;
            }

            res =  tafSimIntTest_ReadFPLMNList(simId);
            if(res != LE_OK)
            {
               exitApplication = true;
               return;
            }
        }

        res = tafSimTest_addFplmnOperator_test(simId, "101", "10");
        if (res == LE_OK)
        {
            res = tafSimTest_writeFplmnList_test(simId);
            if(res == LE_OK)
            {
                res =  tafSimIntTest_ReadFPLMNList(simId);
            }
            else
            {
               exitApplication = true;
               return;
            }
        }
        else
        {
            LE_ERROR("Failed to add FPLMN operator at instance");
            exitApplication = true;
            return;
        }
        tafSimTest_deleteFplmnList_test(simId);
        exitApplication = true;
    }
    else if (strcmp(testType, "refresh") == 0)
    {
        const char* sessionTypePtr = le_arg_GetArg(2);
        if (NULL == sessionTypePtr)
        {
            LE_ERROR("sessionTypePtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_SessionType_t sessionType = (taf_sim_SessionType_t) atoi(sessionTypePtr);

        const char* refreshModePtr = le_arg_GetArg(3);
        if (NULL == refreshModePtr)
        {
            LE_ERROR("refreshModePtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        taf_sim_RefreshMode_t refreshMode = (taf_sim_RefreshMode_t) atoi(refreshModePtr);

        const char* refreshAllowPtr = le_arg_GetArg(4);
        if (NULL == refreshAllowPtr)
        {
            LE_ERROR("refreshAllowPtr is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        bool refreshAllow = atoi(refreshAllowPtr) ==  0 ? false : true;

        le_result_t res = tafSimTest_refresh_test(simId, sessionType, refreshMode, refreshAllow);
        if (res == LE_OK) {
            printf("Wait for refresh status ...\n");
            RefreshChangeHandlerRef = taf_sim_AddRefreshChangeHandler(TestRefreshChangeHandler, NULL);
            LE_ASSERT(RefreshChangeHandlerRef!=NULL);
        }
        exitApplication = res != LE_OK;
    }
    else {
        DisplayAppUsage();
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        if (simIdOrig != simId && strcmp(testType, "select") != 0) {
            LE_INFO("Default card before test: %d", (int) simIdOrig);
            taf_sim_SelectCard(simIdOrig);
        }
        LE_INFO("Exit tafSimIntTest App");
        exit(EXIT_SUCCESS);
    }
}
