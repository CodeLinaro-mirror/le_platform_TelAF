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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023-24 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "main.h"

taf_sim_FPLMNListRef_t FplmnListRef;
taf_sim_RefreshRef_t refreshSessionRef;

static void TestNewSimStateHandler
(
    taf_sim_Id_t     simId,
    taf_sim_States_t simState,
    void*           contextPtr
)
{
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %s", SimStateToString(simState));
}

//Function to convert sim state to string
char* SimStateToString(taf_sim_States_t state) {
    char *cardState;
    switch(state) {
        case TAF_SIM_INSERTED:
            cardState = "TAF_SIM_INSERTED";
            break;
        case TAF_SIM_ABSENT:
            cardState = "TAF_SIM_ABSENT";
            break;
        case TAF_SIM_READY:
            cardState = "TAF_SIM_READY";
            break;
        case TAF_SIM_ERROR:
            cardState = "TAF_SIM_ERROR";
            break;
        default:
            cardState = "TAF_SIM_STATE_UNKNOWN";
            break;
    }
    return cardState;
}

//Function to convert sim app type to string
char* SimAppTypeToString(taf_sim_AppType_t appType) {
    char *appTypeStr;
    switch(appType) {
        case TAF_SIM_APPTYPE_SIM:
            appTypeStr = "TAF_SIM_APPTYPE_SIM";
            break;
        case TAF_SIM_APPTYPE_USIM:
            appTypeStr = "TAF_SIM_APPTYPE_USIM";
            break;
        case TAF_SIM_APPTYPE_CSIM:
            appTypeStr = "TAF_SIM_APPTYPE_CSIM";
            break;
        case TAF_SIM_APPTYPE_RUIM:
            appTypeStr = "TAF_SIM_APPTYPE_RUIM";
            break;
        case TAF_SIM_APPTYPE_ISIM:
            appTypeStr = "TAF_SIM_APPTYPE_ISIM";
            break;
        default:
            appTypeStr = "TAF_SIM_APPTYPE_UNKNOWN";
            break;
    }
    return appTypeStr;
}

char* tafSimTest_SimStateToString(taf_sim_States_t state)
{

    char* stateString = "";

    switch (state)
    {
        case TAF_SIM_INSERTED:
            stateString = "SIM card is inserted but locked.";
            break;
        case TAF_SIM_ABSENT:
            stateString = "SIM card is absent.";
            break;
        case TAF_SIM_READY:
            stateString = "SIM card is inserted and unlocked.";
            break;
        case TAF_SIM_BLOCKED:
            stateString = "SIM card is blocked.";
            break;
        case TAF_SIM_BUSY:
            stateString = "SIM card is busy.";
            break;
        case TAF_SIM_ERROR:
            stateString = "SIM card error.";
            break;
        case TAF_SIM_POWER_DOWN:
            stateString = "SIM card is powered down.";
            break;
        default:
            stateString = "Unknown SIM state.";
            break;
    }

    return stateString;
}

//Function to test sim state
void tafSimTest_state
(
    taf_sim_Id_t simId
)
{
    taf_sim_States_t             state;
    taf_sim_NewStateHandlerRef_t testNewStateHandlerRef;

    testNewStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
    LE_TEST_OK(NULL != testNewStateHandlerRef, "taf_sim_AddNewStateHandler");

    state = taf_sim_GetState(simId);

    LE_INFO("test: state %d", state);

    LE_TEST_OK((state >= TAF_SIM_INSERTED) && (state <= TAF_SIM_ERROR), "taf_sim_GetState");
    printf("Type: %s\n", simId == TAF_SIM_EXTERNAL_SLOT_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");

    printf("State: %s\n", SimStateToString(state));
    printf("Is SIM card Ready: %s\n", taf_sim_IsReady(simId) ? "true" : "false");
    printf("Is SIM card Present: %s\n", taf_sim_IsPresent(simId) ? "true" : "false");
}

//Function to test sim state
void tafSimTest_allState
(
)
{
    le_result_t     res;
    taf_sim_States_t             state;
    taf_sim_NewStateHandlerRef_t testNewStateHandlerRef;

    int simCount = 0;

    res = taf_sim_GetSlotCount(&simCount);
    taf_sim_Id_t simId = taf_sim_GetSelectedCard();
    taf_sim_Id_t simidOrg = simId;

    testNewStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
    LE_TEST_OK(NULL != testNewStateHandlerRef, "taf_sim_AddNewStateHandler");
    LE_INFO("taf_sim_GetSlotCount, res: %d", (int) res);

    printf("Total SIM Slot: %d\n", simCount);
    printf("===============================================\n");
    for (int i = 0; i < simCount; i++) {
        if (i == 1) {
            simId = (simidOrg == TAF_SIM_EXTERNAL_SLOT_1) ? TAF_SIM_EXTERNAL_SLOT_2 : TAF_SIM_EXTERNAL_SLOT_1;
        }

        state = taf_sim_GetState(simId);

        LE_INFO("test: state %d", state);

        LE_TEST_OK((state >= TAF_SIM_INSERTED) && (state <= TAF_SIM_ERROR), "taf_sim_GetState");
        printf("Type: %s\n", simId == TAF_SIM_EXTERNAL_SLOT_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");

        printf("State: %s\n", SimStateToString(state));
        printf("Default SIM: %s\n", simidOrg == simId ? "Yes": "No");
        printf("Is SIM card Ready: %s\n", taf_sim_IsReady(simId) ? "true" : "false");
        printf("Is SIM card Present: %s\n", taf_sim_IsPresent(simId) ? "true" : "false");
        printf("===============================================\n");
    }
    taf_sim_SelectCard(simidOrg);
}

//Function to test sim identification info like ICCID, IMSI, Phone number, operator name
// MCC and MNC
void tafSimTest_info
(
    taf_sim_Id_t simId
)
{
    le_result_t     res;
    char            iccid[TAF_SIM_ICCID_BYTES];
    char            imsi[TAF_SIM_IMSI_BYTES];
    char            eid[TAF_SIM_EID_BYTES];
    char            phoneNumber[TAF_SIM_PHONE_NUM_MAX_BYTES];
    char            operatorName[50];
    char            mcc[4];
    char            mnc[4];

    taf_sim_Id_t defaultSimId = taf_sim_GetSelectedCard();

    memset(iccid, 0, TAF_SIM_ICCID_BYTES);
    memset(imsi, 0, TAF_SIM_IMSI_BYTES);
    memset(eid, 0, TAF_SIM_EID_BYTES);
    memset(phoneNumber, 0, TAF_SIM_PHONE_NUM_MAX_BYTES);
    memset(operatorName, 0, 50);
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    LE_INFO("SimId %d", simId);

    printf("Type: %s\n", simId == TAF_SIM_EXTERNAL_SLOT_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");
    printf("Default SIM: %s\n", defaultSimId == simId ? "Yes": "No");

    bool isSimPreent = taf_sim_IsPresent(simId);
    printf("SIM Availability: %s\n", isSimPreent ? "Yes": "No");

    printf("SIM State: %s\n", tafSimTest_SimStateToString(taf_sim_GetState(simId)));

    bool isSimReady = taf_sim_IsReady(simId);
    printf("SIM Ready: %s\n", isSimReady ? "True": "False");

    // Get SIM ICCID
    res = taf_sim_GetICCID(simId, iccid, sizeof(iccid));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID");
    printf("ICCID: '%s'\n", iccid);

    res = taf_sim_GetIMSI(simId, imsi, sizeof(imsi));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetIMSI");
    printf("IMSI: '%s'\n", imsi);

    res = taf_sim_GetSubscriberPhoneNumber(simId, phoneNumber, sizeof(phoneNumber));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetSubscriberPhoneNumber");
    printf("PhoneNumber: '%s'\n", phoneNumber);

    res = taf_sim_GetHomeNetworkOperator(simId, operatorName, sizeof(operatorName));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkOperator");
    printf("Network Operator name: '%s'\n", operatorName);

    res = taf_sim_GetEID(simId, eid, sizeof(eid));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetEID");
    printf("EID: '%s'\n", eid);

    res = taf_sim_GetHomeNetworkMccMnc(simId, mcc, sizeof(mcc), mnc, sizeof(mnc));
    LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkMccMnc");
    printf("SIM Card MCC: '%s'\n", mcc);
    printf("SIM Card MNC: '%s'\n", mnc);
    printf("===============================================\n");
}

void tafSimTest_allInfo
(
)
{
    le_result_t     res;
    char            iccid[TAF_SIM_ICCID_BYTES];
    char            imsi[TAF_SIM_IMSI_BYTES];
    char            eid[TAF_SIM_EID_BYTES];
    char            phoneNumber[TAF_SIM_PHONE_NUM_MAX_BYTES];
    char            operatorName[50];
    char            mcc[4];
    char            mnc[4];

    int simCount = 0;

    res = taf_sim_GetSlotCount(&simCount);
    taf_sim_Id_t simId = taf_sim_GetSelectedCard();
    taf_sim_Id_t simidOrg = simId;

    printf("Total SIM Slot: %d\n", simCount);
    printf("===============================================\n");
    for (int i = 0; i < simCount; i++) {
        if (i == 1) {
            simId = (simidOrg == TAF_SIM_EXTERNAL_SLOT_1) ? TAF_SIM_EXTERNAL_SLOT_2 : TAF_SIM_EXTERNAL_SLOT_1;
        }

        memset(iccid, 0, TAF_SIM_ICCID_BYTES);
        memset(imsi, 0, TAF_SIM_IMSI_BYTES);
        memset(eid, 0, TAF_SIM_EID_BYTES);
        memset(phoneNumber, 0, TAF_SIM_PHONE_NUM_MAX_BYTES);
        memset(operatorName, 0, 50);
        memset(mcc, 0, 4);
        memset(mnc, 0, 4);

        LE_INFO("SimId %d", simId);

        printf("Type: %s\n", simId == TAF_SIM_EXTERNAL_SLOT_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");
        if (simCount > 1) {
            printf("Default SIM: %s\n", i == 0 ? "Yes": "No");
        }

        bool isSimPreent = taf_sim_IsPresent(simId);
        printf("SIM Availability: %s\n", isSimPreent ? "Yes": "No");

        printf("SIM State: %s\n", tafSimTest_SimStateToString(taf_sim_GetState(simId)));

        bool isSimReady = taf_sim_IsReady(simId);
        printf("SIM Ready: %s\n", isSimReady ? "True": "False");

        // Get SIM ICCID
        res = taf_sim_GetICCID(simId, iccid, sizeof(iccid));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID");
        printf("ICCID: '%s'\n", iccid);

        res = taf_sim_GetIMSI(simId, imsi, sizeof(imsi));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetIMSI");
        printf("IMSI: '%s'\n", imsi);

        res = taf_sim_GetSubscriberPhoneNumber(simId, phoneNumber, sizeof(phoneNumber));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetSubscriberPhoneNumber");
        printf("PhoneNumber: '%s'\n", phoneNumber);

        res = taf_sim_GetHomeNetworkOperator(simId, operatorName, sizeof(operatorName));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkOperator");
        printf("Network Operator name: '%s'\n", operatorName);

        res = taf_sim_GetEID(simId, eid, sizeof(eid));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetEID");
        printf("EID: '%s'\n", eid);

        res = taf_sim_GetHomeNetworkMccMnc(simId, mcc, sizeof(mcc), mnc, sizeof(mnc));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkMccMnc");
        printf("SIM Card MCC: '%s'\n", mcc);
        printf("SIM Card MNC: '%s'\n", mnc);
        printf("===============================================\n");
    }
    taf_sim_SelectCard(simidOrg);
}

void tafSimTest_selection
(
    taf_sim_Id_t slot
)
{
    taf_sim_Id_t slotId = taf_sim_GetSelectedCard();
    printf("\n Current SIM slot id = %d\n" ,slotId);

    le_result_t res = taf_sim_SelectCard(slot);
    LE_TEST_OK(res == LE_OK, "tafSimTest_selection");
    slotId = taf_sim_GetSelectedCard();
    printf("\n After selecting %d Current SIM slot id = %d\n" ,slot, slotId);
}

le_result_t tafSimTest_enterPin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pinPtr
)
{
    le_result_t res;
    res = taf_sim_EnterPIN(simId, lockType, pinPtr);
    LE_TEST_OK(res == LE_OK, "tafSimTest_enterPin");
    LE_INFO("EnterPIN done");
    return res;
}

le_result_t tafSimTest_setLock
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pinPtr,
    bool lock
)
{
    le_result_t res;
    if (lock) {
        res = taf_sim_Lock(simId, lockType, pinPtr);
        LE_TEST_OK(res == LE_OK, "taf_sim_Lock");
        LE_INFO("Set lock request sent successfully");
    } else {
        res = taf_sim_Unlock(simId, lockType, pinPtr);
        LE_TEST_OK(res == LE_OK, "taf_sim_Unlock");
        LE_INFO("Unlock request sent successfully");
    }

    return res;
}

le_result_t tafSimTest_Change_pin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  oldpinPtr,
    const char*  newpinPtr
)
{
    le_result_t res;
    res = taf_sim_ChangePIN(simId, lockType, oldpinPtr, newpinPtr);
    LE_TEST_OK(res == LE_OK, "tafSimTest_Change_pin");
    return res;
}

le_result_t tafSimTest_unblock_puk
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pukPtr,
    const char*  newpinPtr
)
{
    le_result_t res;
    res = taf_sim_Unblock(simId, lockType, pukPtr, newpinPtr);
    LE_TEST_OK(res == LE_OK, "tafSimTest_unblock_puk");
    return res;
}

void tafSimTest_GetAppTypes
(
    taf_sim_Id_t simId
)
{
    le_result_t res;
    taf_sim_AppType_t appType[TAF_SIM_MAX_APP_TYPE];
    size_t appTypeNumElements = TAF_SIM_MAX_APP_TYPE;

    res = taf_sim_GetAppTypes(simId, appType, &appTypeNumElements);
    LE_TEST_OK(res == LE_OK, "tafSimTest_GetAppTypes");

    if (res == LE_OK) {
        printf("Total number of apps in card: %d\n", (int) appTypeNumElements);
    } else {
        printf("API GetCardAppTypes failed! Error: %s\n", LE_RESULT_TXT(res));
        return;
    }

    for (int i = 0; i < (int) appTypeNumElements; i++) {
        printf("App type: %s\n", SimAppTypeToString(appType[i]));
    }
}

void tafSimTest_sim_openLogicalChannel
(
    taf_sim_Id_t simId,
    const char* aid
)
{
    uint8_t channel = 0;

    le_result_t res  = taf_sim_OpenLogicalChannelByAid(simId, aid, &channel);
    if (res == LE_OK) {
        printf("OpenLogicalChannelByAid success. Channel: %d\n", channel);
    } else {
        printf("OpenLogicalChannelByAid failed!\n");
    }
}

void tafSimTest_sim_closeLogicalChannel
(
    taf_sim_Id_t simId,
    uint8_t channelId
)
{
    le_result_t res  = taf_sim_CloseLogicalChannel(simId, channelId);
    if (res == LE_OK) {
        printf("CloseLogicalChannel of channel #%d is success.\n", channelId);
    } else {
       printf("CloseLogicalChannel of channel #%d is failed!\n", channelId);
    }
}

void tafSimTest_sim_access
(
    taf_sim_Id_t simId
)
{
     //APDU to open Master File
    uint8_t selectMFAPDU[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x3F, 0x00};
    uint8_t responseAPDU[100];
    size_t responseLength = 100;
    uint8_t channel = 0;

    // Open a logical channel
    LE_ASSERT_OK(taf_sim_OpenLogicalChannel(simId, TAF_SIM_APPTYPE_USIM, &channel));
    LE_TEST_OK(channel, "tafSimTest_sim_access");

    LE_ASSERT_OK(taf_sim_SendApduOnChannel(simId,
                                          channel,
                                          selectMFAPDU,
                                          sizeof(selectMFAPDU),
                                          responseAPDU,
                                          &responseLength));
    LE_INFO("APDU response sw1 = 0x%02X",responseAPDU[0]);
    LE_INFO("APDU response sw2 = 0x%02X",responseAPDU[1]);

    // Close the logical channel
    LE_ASSERT_OK(taf_sim_CloseLogicalChannel(simId,channel));
    le_result_t reqStatus;
    taf_sim_Command_t command = (taf_sim_Command_t)0xc0;
    char fileIdentifier[5]={'2', 'f', 'e', '2', '\0'};
    LE_INFO("fileIdeentifier");
    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t p3 = 15;
    LE_INFO("p1,p2,p3");
    uint8_t data[1];
    data[0] = '\0';
    LE_INFO("DATA");
    uint8_t sw1=0, sw2=0;
    LE_INFO("sw1sw2");
    uint8_t response[100];
    LE_INFO("responsee");
    size_t responseSize = sizeof(response)/sizeof(response[0]);
    LE_INFO("responseSize");
    char filePath[5] = {'3', 'F', '0', '0', '\0'};
    LE_INFO("filePath");
    reqStatus = taf_sim_SendCommand(simId, command, fileIdentifier, p1, p2, p3, data, sizeof(data)/sizeof(data[0]), filePath, &sw1, &sw2, response, &responseSize);
    LE_INFO("REQSTATUS");
    if(reqStatus != LE_OK) {
        LE_INFO("reqStatus is %d", reqStatus);
        return;
    }
    LE_INFO("SendCommand API working");
    LE_INFO("APDU response sw1 = 0x%02X",sw1);
    LE_INFO("APDU response sw2 = 0x%02X",sw2);
}

void tafSimTest_SetPowerCheck
(
    taf_sim_Id_t simId,
    le_onoff_t powerStatus
)
{
    le_result_t r;
    r = taf_sim_SetPower(simId, powerStatus);
    if(r != LE_OK)
    {
        LE_INFO("SetPower : failed to change to %d , SetPower returned \'%d\'",powerStatus, r);
        return;
    }
    LE_INFO("SetPower API working fine");
}

void tafSimTest_sim_isEmergency
(
    taf_sim_Id_t simId
)
{
    bool state = false;
    le_result_t r = taf_sim_IsEmergencyCallSubscriptionSelected(simId, &state);

    if(r == LE_OK) {
        printf("Query Success! Emergency call subscription is%s active.\n", state ? "" : " not");
    } else {
        printf("Query emergency call subscription is failed! Error: %s\n", LE_RESULT_TXT(r));
    }
}

void tafSimTest_swapToEmergencyAndBack
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    LE_INFO("SwapToEmergencyAndBack simId: %d, manufacturer: %d", (int) simId, (int) manufacturer);

    le_result_t r = taf_sim_LocalSwapToEmergencyCallSubscription(simId, manufacturer);
    LE_INFO("SwapToEmergency %s, ErrorCode: %s", r == LE_OK ? "success" : "failed", LE_RESULT_TXT(r));
    printf("SwapToEmergency %s, ErrorCode: %s\n", r == LE_OK ? "success" : "failed", LE_RESULT_TXT(r));

    r = taf_sim_LocalSwapToCommercialSubscription(simId, manufacturer);

    LE_INFO("SwapToRegular %s, ErrorCode: %s", r == LE_OK ? "success" : "failed", LE_RESULT_TXT(r));
    printf("SwapToRegular %s, ErrorCode: %s\n", r == LE_OK ? "success" : "failed", LE_RESULT_TXT(r));
}

void tafSimTest_fplmnList_test(taf_sim_Id_t simId){
    le_result_t res;
    char            mcc[4];
    char            mnc[4];

    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    taf_sim_FPLMNListRef_t FPLMNList = taf_sim_ReadFPLMNList(simId);
    LE_TEST_OK(FPLMNList!=NULL, "tafSimTest_fplmnList_test");
    if (FPLMNList!=NULL) {
        LE_INFO("FPLMNList Read function working");
        res = taf_sim_GetFirstFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
        if (res == LE_OK) {
            printf("FPLMN list #1: mcc %s, mnc %s\n", mcc, mnc);
        }
        for (int i = 0; i < 7; i++) {
            memset(mcc, 0, 4);
            memset(mnc, 0, 4);
            res = taf_sim_GetNextFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
            if (res == LE_OK) {
                printf("FPLMN list #%d: mcc %s, mnc %s\n", i+2, mcc, mnc);
            }
        }
    }
    printf("tafSimTest_fplmnList completed. Result: %s\n", (FPLMNList!=NULL) ? "PASS":"FAILED");
}

void tafSimTest_createFplmnList_test(taf_sim_Id_t simId) {
    FplmnListRef = taf_sim_CreateFPLMNList();
    LE_TEST_OK(FplmnListRef!=NULL, "tafSimTest_createFplmnList_test");
    LE_INFO("taf_sim_CreateFPLMNList end. FplmnListRef: %p\n", FplmnListRef);
    printf("createFplmnList_test completed. Result: %s\n", (FplmnListRef!=NULL) ? "PASS":"FAILED");
}

void tafSimTest_addFplmnOperator_test(taf_sim_Id_t simId, const char*  mcc,
    const char*  mnc) {
    if (FplmnListRef==NULL) {
        LE_INFO("tafSimTest_addFplmnOperator_test FplmnListRef is null, so create it\n");
        FplmnListRef = taf_sim_CreateFPLMNList();
    }
    le_result_t res = taf_sim_AddFPLMNOperator(FplmnListRef, mcc, mnc);
    LE_TEST_OK(res == LE_OK, "tafSimTest_addFplmnOperator_test");
    LE_INFO("tafSimTest_addFplmnOperator_test end\n");
    printf("tafSimTest_addFplmnOperator completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");
}

void tafSimTest_writeFplmnList_test(taf_sim_Id_t simId, const char*  mcc,
    const char*  mnc) {
    if (FplmnListRef==NULL) {
        LE_INFO("tafSimTest_writeFplmnList_test FplmnListRef is null, so create it\n");
        FplmnListRef = taf_sim_CreateFPLMNList();
    }
    le_result_t res = taf_sim_AddFPLMNOperator(FplmnListRef, mcc, mnc);
    res = taf_sim_WriteFPLMNList(simId, FplmnListRef);
    LE_TEST_OK(res == LE_OK, "tafSimTest_writeFplmnList_test");
    LE_INFO("tafSimTest_writeFplmnList_test end\n");
    printf("tafSimTest_writeFplmnList completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");
}

void tafSimTest_writeFplmnLists_test(taf_sim_Id_t simId) {
    int n;
    le_result_t res = LE_FAULT;
    char mcc[8], mccS[4];
    char mnc[8], mncS[4];
    char num[6];
    int mccInt, mncInt;
    char *p;

    if (FplmnListRef==NULL) {
        LE_INFO("tafSimTest_writeFplmnList_test FplmnListRef is null, so create it\n");
        FplmnListRef = taf_sim_CreateFPLMNList();
    }

    fflush(stdout);
    printf("How many FPLMNs (mcc mnc) want to write: ");
    p = fgets(num,sizeof(num),stdin);
    n = atoi(num);

    for (int i = 0; i < n; i++) {
        printf("Enter mcc of PLMN %d: ", i+1);
        p = fgets(mcc,sizeof(mcc),stdin);
        mccInt = atoi(mcc);
        if (p == NULL || mccInt < 1 || mccInt > 999) {
            LE_INFO("Wrong input! mcc %s for PLMN%d (mccInt: %d). Try again!", mcc, i+1, mccInt);
            printf("Wrong input! mcc %s for PLMN%d. Try rerun test again!\n", mcc, i+1);
            return;
        }

        printf("Enter mnc of PLMN %d: ", i+1);
        p = fgets(mnc,sizeof(mnc),stdin);
        mncInt = atoi(mnc);
        if (p == NULL || mncInt < 1 || mncInt > 999) {
            LE_INFO("Wrong input! mnc %s for PLMN%d (mncInt: %d). Try again!", mnc, i+1, mncInt);
            printf("Wrong input! mnc %s for PLMN%d. Try rerun test again!\n", mnc, i+1);
            return;
        }

        if (NULL == mcc || NULL == mnc)
        {
            printf("WRONG input! Incorrect mcc mnc of PLMN#%d\n", i+1);
            printf("Re run the test again.\n");
            return;
        } else {
            snprintf(mccS, 4, "%d", mccInt );
            snprintf(mncS, 4, "%d", mncInt );
            LE_INFO("Input MCC MNC of PLMN#%d: %s %s\n", i+1, mccS, mncS);
            res = taf_sim_AddFPLMNOperator(FplmnListRef, mccS, mncS);
        }
    }
    if (res == LE_OK) {
        res = taf_sim_WriteFPLMNList(simId, FplmnListRef);
        LE_TEST_OK(res == LE_OK, "tafSimTest_writeFplmnList_test");
        LE_INFO("tafSimTest_writeFplmnList_test end\n");
        printf("tafSimTest_writeFplmnList completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");
    }
}

void tafSimTest_getFirstFplmnOperator_test(taf_sim_Id_t simId) {
    char            mcc[4];
    char            mnc[4];
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    if (FplmnListRef==NULL) {
        LE_INFO("tafSimTest_getFirstFplmnOperator_test FplmnListRef is null, so ReadFPLMNList\n");
        FplmnListRef = taf_sim_ReadFPLMNList(simId);
    }
    le_result_t res = taf_sim_GetFirstFPLMNOperator(FplmnListRef, mcc, sizeof(mcc), mnc, sizeof(mnc));
    LE_TEST_OK(res == LE_OK, "tafSimTest_getFirstFplmnOperator_test");
    if (res == LE_OK) {
        printf("First FPLMN Operator: mcc %s, mnc %s\n", mcc, mnc);
    }
    LE_INFO("tafSimTest_getFirstFplmnOperator_test end\n");
    printf("getFirstFplmnOperator_test completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");
}

void tafSimTest_getNextFplmnOperator_test(taf_sim_Id_t simId) {
    char            mcc[4];
    char            mnc[4];
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    if (FplmnListRef==NULL) {
        LE_INFO("tafSimTest_getNextFplmnOperator_test FplmnListRef is null, so ReadFPLMNList\n");
        FplmnListRef = taf_sim_ReadFPLMNList(simId);
    }
    le_result_t res = taf_sim_GetNextFPLMNOperator(FplmnListRef, mcc, sizeof(mcc), mnc, sizeof(mnc));
    LE_TEST_OK(res == LE_OK, "tafSimTest_getNextFplmnOperator_test");
    if (res == LE_OK) {
        printf("Next FPLMN Operator: mcc %s, mnc %s\n", mcc, mnc);
    }
    LE_INFO("tafSimTest_getNextFplmnOperator_test end\n");
    printf("getNextFplmnOperator_test completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");
}

void tafSimTest_deleteFplmnList_test(taf_sim_Id_t simId) {
    if (FplmnListRef==NULL) {
        FplmnListRef = taf_sim_CreateFPLMNList();
    }
    taf_sim_DeleteFPLMNList(FplmnListRef);
    LE_TEST_OK(true, "tafSimTest_deleteFplmnList_test");
    LE_INFO("tafSimTest_deleteFplmnList_test end\n");
    printf("tafSimTest_deleteFplmnList_test completed. Result: PASS\n");
}

le_result_t tafSimTest_refresh_test(taf_sim_Id_t simId, taf_sim_SessionType_t sessionType, taf_sim_RefreshMode_t refreshMode, bool refreshAllow) {
    le_result_t res = taf_sim_CreateSession(sessionType, &refreshSessionRef);
    LE_TEST_OK(res == LE_OK, "taf_sim_CreateSession");
    LE_INFO("CreateSession refreshSessionRef: %p", refreshSessionRef);

    char input_str[32];
    int i = 0;
    taf_sim_RefreshRegFile_t refresfFiles[TAF_SIM_MAX_SIM_REFRESH_FILES];

    printf("Do want to input refresh file? (y/n): ");
    char *p = fgets(input_str,sizeof(input_str),stdin);

    if (p != NULL && input_str[0]=='y') {
        do {
            printf("Input file ID (Ex: Input 28486 for file 0x6F46): ");
            p = fgets(input_str,sizeof(input_str),stdin);
            refresfFiles[i].file_id = atoi(input_str);

            printf("Input file path(e.g: 3f007fff): ");
            p = fgets(input_str,sizeof(input_str),stdin);
            le_utf8_Copy((char*) refresfFiles[i].path, (char*) input_str, sizeof(input_str), NULL);

            printf("Do want to input another refresh file (y/n): ");
            p = fgets(input_str,sizeof(input_str),stdin);

            LE_INFO("SL# %d File id: %d, file path: %s", i, refresfFiles[i].file_id, refresfFiles[i].path);
            i++;
        } while(input_str[0]!='n');

        if (i > 0) {
            res |= taf_sim_SetRefreshRegisterFiles(refreshSessionRef, refresfFiles, i);
        }
    }

    res |= taf_sim_SetRefreshMode(refreshSessionRef, refreshMode);
    LE_TEST_OK(res == LE_OK, "taf_sim_SetRefreshMode");

    res |= taf_sim_SetRefreshAllow(refreshSessionRef, refreshAllow);
    LE_TEST_OK(res == LE_OK, "taf_sim_SetRefreshAllow");

    printf("Refresh request %s\n", res == LE_OK ? "success.":"failed!");

    return res;
}
