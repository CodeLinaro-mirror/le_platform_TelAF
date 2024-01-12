/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
static le_thread_Ref_t ThreadRef;

static taf_sim_AuthenticationResponseHandlerRef_t AuthResponseHandlerRef = NULL;
static taf_sim_NewStateHandlerRef_t NewSimStateHandlerRef = NULL;
static taf_sim_IccidChangeHandlerRef_t IccidChangeHandlerRef = NULL;

//Function to convert sim state to string
const char* SimStateToString(taf_sim_States_t state) {
    const char *cardState;
    switch(state) {
        case TAF_SIM_INSERTED: //Same as TAF_SIM_PRESENT(0)
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
        case TAF_SIM_RESTRICTED:
            cardState = "TAF_SIM_RESTRICTED";
            break;
        case TAF_SIM_BUSY:
            cardState = "TAF_SIM_BUSY";
            break;
        default:
            cardState = "TAF_SIM_STATE_UNKNOWN";
            break;
    }
    return cardState;
}

static void TestNewSimStateHandler
(
    taf_sim_Id_t     simId,
    taf_sim_States_t simState,
    void*            contextPtr
)
{
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %s", SimStateToString(simState));
}

const char* tafSimUnitTest_SimStateToString(taf_sim_States_t state)
{

    const char *stateString;

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
void tafSimUnitTest_state
(
    taf_sim_Id_t simId
)
{
    taf_sim_States_t state = taf_sim_GetState(simId);

    LE_INFO("Test: state %d", state);

    LE_TEST_OK((state >= TAF_SIM_INSERTED) && (state <= TAF_SIM_ERROR), "tafSimUnitTest_state: state is vaild");

    printf("Type: %s\n", simId == TAF_SIM_SLOT_ID_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");

    printf("State: %s\n", SimStateToString(state));
    printf("Is SIM card Ready: %s\n", taf_sim_IsReady(simId) ? "true" : "false");
    printf("Is SIM card Present: %s \n", taf_sim_IsPresent(simId) ? "true" : "false");
}

//Function to test sim identification info like ICCID, IMSI, Phone number, operator name
// MCC and MNC
void tafSimUnitTest_info
(
    taf_sim_Id_t SimId
)
{
    le_result_t     res;
    char            iccid[TAF_SIM_ICCID_BYTES];
    char            eid[TAF_SIM_EID_BYTES];
    char            imsi[TAF_SIM_IMSI_BYTES];
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
            simId = (simidOrg == TAF_SIM_SLOT_ID_1) ? TAF_SIM_SLOT_ID_2 : TAF_SIM_SLOT_ID_1;
        }

        memset(iccid, 0, TAF_SIM_ICCID_BYTES);
        memset(eid, 0, TAF_SIM_EID_BYTES);
        memset(imsi, 0, TAF_SIM_IMSI_BYTES);
        memset(phoneNumber, 0, TAF_SIM_PHONE_NUM_MAX_BYTES);
        memset(operatorName, 0, 50);
        memset(mcc, 0, 4);
        memset(mnc, 0, 4);

        LE_INFO("SimId %d", simId);

        printf("Type: %s\n", simId == TAF_SIM_SLOT_ID_1 ? "TAF_SIM_EXTERNAL_SLOT_1": "TAF_SIM_EXTERNAL_SLOT_2");

        if (simCount > 1) {
            printf("Default SIM: %s\n", i == 0 ? "Yes": "No");
        }

        bool isSimPreent = taf_sim_IsPresent(simId);
        printf("SIM Availability: %s\n", isSimPreent ? "Yes": "No");

        printf("SIM State: %s\n", tafSimUnitTest_SimStateToString(taf_sim_GetState(simId)));

        bool isSimReady = taf_sim_IsReady(simId);
        printf("SIM Ready: %s\n", isSimReady ? "True": "False");

        // Get SIM ICCID
        res = taf_sim_GetICCID(simId, iccid, sizeof(iccid));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetICCID");
        printf("SIM Card ICCID: '%s'\n", iccid);

        taf_sim_GetEID(simId, eid, sizeof(eid));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetEID");
        printf("\nSIM Card EID: '%s'\n", eid);

        res = taf_sim_GetIMSI(simId, imsi, sizeof(imsi));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetIMSI");
        printf("SIM Card IMSI: '%s'\n", imsi);

        res = taf_sim_GetSubscriberPhoneNumber(simId, phoneNumber, sizeof(phoneNumber));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetSubscriberPhoneNumber");
        printf("SIM Card PhoneNumber: '%s'\n", phoneNumber);

        res = taf_sim_GetHomeNetworkOperator(simId, operatorName, sizeof(operatorName));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkOperator");
        printf("SIM Card Network Operator name: '%s'\n", operatorName);

        res = taf_sim_GetHomeNetworkMccMnc(simId, mcc, sizeof(mcc), mnc, sizeof(mnc));
        LE_TEST_OK(res == LE_OK, "taf_sim_GetHomeNetworkMccMnc");
        printf("SIM Card MCC: '%s'\n", mcc);
        printf("SIM Card MNC: '%s'\n", mnc);
        printf("===============================================\n");
    }
    taf_sim_SelectCard(simidOrg);
}

void tafSimUnitTest_selection
(
    taf_sim_Id_t slot
)
{
    bool isAutoSelEnabled;
    taf_sim_Id_t slotId = taf_sim_GetSelectedCard();
    printf("\nCurrent SIM slot id = %d\n",(int)slotId);

    le_result_t res = taf_sim_SelectCard(slot);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_selection");
    slotId = taf_sim_GetSelectedCard();
    printf("\nAfter selecting %d Current SIM slot id = %d\n", (int)slot, (int)slotId);

    res = taf_sim_SetAutomaticSelection(true);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_selection");
    printf("\nSet Automatic SIM slot\n" );

    res = taf_sim_GetAutomaticSelection(&isAutoSelEnabled);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_selection");
    printf("\nIs Automatic Selection enabled? %d\n", (int) isAutoSelEnabled);
}

void tafSimUnitTest_enterPin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pinPtr
)
{
    le_result_t res = LE_NOT_IMPLEMENTED;
    res = taf_sim_EnterPIN(simId, lockType, pinPtr);

    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_enterPin");
    LE_INFO("EnterPIN done");
}

void tafSimUnitTest_setLock
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pinPtr,
    bool lock
)
{
    le_result_t res = LE_NOT_IMPLEMENTED;
    if (lock) {
        res = taf_sim_Lock(simId, lockType, pinPtr);
        LE_TEST_OK(res == LE_OK, "taf_sim_Lock");
        LE_INFO("Set lock request sent successfully");
    } else {
        res = taf_sim_Unlock(simId, lockType, pinPtr);
        LE_TEST_OK(res == LE_OK, "taf_sim_Unlock");
        LE_INFO("Unlock request sent successfully");
    }
}

void tafSimUnitTest_Change_pin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  oldpinPtr,
    const char*  newpinPtr
)
{
    le_result_t res = LE_NOT_IMPLEMENTED;
    res = taf_sim_ChangePIN(simId, lockType, oldpinPtr, newpinPtr);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_Change_pin");
}

void tafSimUnitTest_unblock_puk
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pukPtr,
    const char*  newpinPtr
)
{
    le_result_t res = LE_NOT_IMPLEMENTED;
    res = taf_sim_Unblock(simId, lockType, pukPtr, newpinPtr);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_unblock_puk");

    uint32_t remainingPukTriesPtr;
    res = taf_sim_GetRemainingPUKTries(simId, &remainingPukTriesPtr);
    LE_INFO("taf_sim_GetRemainingPUKTries: Remaining PUK tries = %d", remainingPukTriesPtr);
    LE_TEST_OK(res == LE_OK, "taf_sim_GetRemainingPUKTries");
}

void tafSimUnitTest_sendApdu
(
    taf_sim_Id_t simId
)
{
    uint8_t selectAPDU[] = {0x00, 0xA4, 0x00, 0x04, 0x02, 0x3F, 0x00};
    uint8_t responseAPDU[100];
    size_t responseLength = 100;

    LE_TEST_OK(LE_OK == taf_sim_SendApdu(simId,
                                          selectAPDU,
                                          sizeof(selectAPDU),
                                          responseAPDU,
                                          &responseLength), "taf_sim_SendApdu");

    LE_INFO("tafSimUnitTest_sendApdu: APDU response sw1 = 0x%02X",responseAPDU[0]);
    LE_INFO("tafSimUnitTest_sendApdu: APDU response sw2 = 0x%02X",responseAPDU[1]);
}


void tafSimUnitTest_sim_access
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
    LE_TEST_OK(LE_OK == taf_sim_OpenLogicalChannel(simId, TAF_SIM_APPTYPE_USIM, &channel), "taf_sim_OpenLogicalChannel");
    LE_TEST_OK(channel, "channel validity check");

    LE_TEST_OK(LE_OK == taf_sim_SendApduOnChannel(simId,
                                          channel,
                                          selectMFAPDU,
                                          sizeof(selectMFAPDU),
                                          responseAPDU,
                                          &responseLength), "taf_sim_SendApduOnChannel");

    LE_INFO("SendApduOnChannel: APDU response sw1 = 0x%02X",responseAPDU[0]);
    LE_INFO("SendApduOnChannel: APDU response sw2 = 0x%02X",responseAPDU[1]);

    // Close the logical channel
    LE_TEST_OK(LE_OK == taf_sim_CloseLogicalChannel(simId,channel), "taf_sim_CloseLogicalChannel");
    le_result_t reqStatus;
    taf_sim_Command_t command = (taf_sim_Command_t)0xc0;
    char fileIdentifier[5]={'2', 'f', 'e', '2', '\0'};
    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t p3 = 15;
    uint8_t data[1];
    data[0] = '\0';
    uint8_t sw1=0, sw2=0;
    uint8_t response[100];
    size_t responseSize = sizeof(response)/sizeof(response[0]);
    char filePath[5] = {'3', 'F', '0', '0', '\0'};
    reqStatus = taf_sim_SendCommand(simId, command, fileIdentifier, p1, p2, p3, data, sizeof(data)/sizeof(data[0]), filePath, &sw1, &sw2, response, &responseSize);
    if(reqStatus != LE_OK) {
        LE_INFO("reqStatus is %d", reqStatus);
        return;
    }
    LE_INFO("SendApduOnChannel success!!!");
    LE_INFO("APDU response sw1 = 0x%02X",sw1);
    LE_INFO("APDU response sw2 = 0x%02X",sw2);
}

void tafSimUnitTest_SetPowerCheck
(
    taf_sim_Id_t simId,
    le_onoff_t powerStatus
)
{
    le_result_t r;
    r = taf_sim_SetPower(simId, powerStatus);
    if(r != LE_OK)
    {
        LE_INFO("SetPower: Failed to change power: %d, result: \'%d\'",powerStatus, r);
        return;
    }
    LE_INFO("SetPower success!!!");
}

void tafSimUnitTest_sim_isEmergency
(
    taf_sim_Id_t simId
)
{
    bool state = false;
    LE_TEST_OK(LE_OK == taf_sim_IsEmergencyCallSubscriptionSelected(simId, &state), "taf_sim_IsEmergencyCallSubscriptionSelected");
    LE_INFO("Emergency Check success!!!");
}

void tafSimUnitTest_swapToEmergencyAndBack
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    LE_TEST_OK(LE_OK == taf_sim_LocalSwapToEmergencyCallSubscription(simId, manufacturer), "taf_sim_LocalSwapToEmergencyCallSubscription");
    LE_INFO("SwapToEmergency is success");
    LE_TEST_OK(LE_OK == taf_sim_LocalSwapToCommercialSubscription(simId, manufacturer), "taf_sim_LocalSwapToCommercialSubscription");
    LE_INFO("SwapToRegular is success");
}

void tafSimUnitTest_fplmnList_test(taf_sim_Id_t simId, const char* mccInput, const char* mncInput) {
    le_result_t res;
    char            mcc[4];
    char            mnc[4];

    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    taf_sim_FPLMNListRef_t FPLMNList = taf_sim_ReadFPLMNList(simId);
    LE_TEST_OK(FPLMNList!=NULL, "tafSimUnitTest_ReadFPLMNList_test");

    res = taf_sim_GetFirstFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
    if (res == LE_OK) {
        LE_INFO("FPLMN list #1: mcc %s, mnc %s\n", mcc, mnc);
    }
    LE_TEST_OK(LE_OK == res, "tafSimUnitTest_GetFirstFPLMNOperator_test");
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);
    res = taf_sim_GetNextFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
    if (res == LE_OK) {
        LE_INFO("FPLMN list #2: mcc %s, mnc %s\n", mcc, mnc);
    }
    LE_TEST_OK(LE_OK == res, "tafSimUnitTest_GetNextFPLMNOperator_test");
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);
    res = taf_sim_GetNextFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
    if (res == LE_OK) {
        LE_INFO("FPLMN list #3: mcc %s, mnc %s\n", mcc, mnc);
    }
    LE_TEST_OK(LE_OK == res, "tafSimUnitTest_GetNextFPLMNOperator_test");
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);
    res = taf_sim_GetNextFPLMNOperator(FPLMNList, mcc, sizeof(mcc), mnc, sizeof(mnc));
    if (res == LE_OK) {
        LE_INFO("FPLMN list #4: mcc %s, mnc %s\n", mcc, mnc);
    }

    LE_INFO("tafSimUnitTest_readFplmnList_test completed. Result: %s\n", (FPLMNList!=NULL) ? "PASS":"FAILED");
    LE_TEST_OK(FPLMNList!=NULL, "tafSimUnitTest_readFplmnList_test");

    taf_sim_FPLMNListRef_t fplmnList = taf_sim_CreateFPLMNList();
    LE_TEST_OK(fplmnList!=NULL, "tafSimUnitTest_createFplmnList_test");
    LE_INFO("taf_sim_CreateFPLMNList end. FplmnListRef: %p\n", fplmnList);
    LE_INFO("createFplmnList_test completed. Result: %s\n", (fplmnList!=NULL) ? "PASS":"FAILED");
    LE_TEST_OK(fplmnList!=NULL, "tafSimUnitTest_createFplmnList_test");

    res = taf_sim_AddFPLMNOperator(fplmnList, mccInput, mncInput);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_addFplmnOperator_test");
    LE_INFO("tafSimUnitTest_addFplmnOperator completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");

    res = taf_sim_WriteFPLMNList(simId, fplmnList);
    LE_TEST_OK(res == LE_OK, "tafSimUnitTest_writeFplmnList_test");
    LE_INFO("tafSimUnitTest_writeFplmnList completed. Result: %s\n", res == LE_OK ? "PASS":"FAILED");

    taf_sim_DeleteFPLMNList(fplmnList);
    LE_TEST_OK(true, "tafSimUnitTest_deleteFplmnList_test");
    LE_INFO("tafSimUnitTest_deleteFplmnList_test end\n");
}

static void TestIccidChangeHandler(taf_sim_Id_t simId, const char* Iccid, void* contextPtr) {
    LE_INFO("Iccid Change event for SIM card: %d", simId);
    LE_INFO("ICCID is: %s", (const char*)Iccid);
}

static void SessionDisconnectHandler(void* contextPtr) {
    LE_INFO("SessionDisconnectHandler: Client disconnected");
}

static void TestAuthenticationResponse
(
    taf_sim_Id_t     simId,
    taf_sim_LockResponse_t responseType,
    le_result_t result,
    void* contextPtr
)
{
    LE_INFO("Authentication Response for SIM card: %d, Response type: %d", simId, (int) responseType);

    if (LE_OK == result) {
        switch(responseType) {
            case TAF_SIM_CHANGE_PIN:
                LE_INFO("Change pin successfull");
                printf("\nChange pin successfull\n");
                break;
            case TAF_SIM_UNLOCK_BY_PIN:
                LE_INFO("Enter pin successfull");
                printf("\nEnter pin successfull\n");
                break;
            case TAF_SIM_SET_LOCK:
                LE_INFO("lock/unlock successfull");
                printf("\nLock/unlock successfull\n");
                break;
            case TAF_SIM_UNLOCK_BY_PUK:
                LE_INFO("unblock successfull");
                printf("\nTAF_SIM_UNLOCK_BY_PUK successfull\n");
                break;
            default:
                LE_INFO("Unknown response");
                break;
        }
    } else {
        LE_INFO("Error: %s\n", LE_RESULT_TXT(result));
        printf("\nResponseType: %d, Result Error: %s\n", (int) responseType, LE_RESULT_TXT(result));
    }
    LE_INFO("Remaining PIN tries: %d\n", taf_sim_GetRemainingPINTries(simId));
}

static void* Test_taf_Sim_Auth_AddHandler(void* context) {

    taf_sim_ConnectService();

    AuthResponseHandlerRef = taf_sim_AddAuthenticationResponseHandler(TestAuthenticationResponse, NULL);
    LE_TEST_OK(AuthResponseHandlerRef != NULL, "taf_sim_AuthenticationResponseHandlerRef_t");

    le_event_RunLoop();
    return NULL;
}

static taf_sim_Id_t get_slot_id(const char* slotId) {
    if (strcmp(slotId, "slot2") == 0)
    {
        return (taf_sim_Id_t) TAF_SIM_EXTERNAL_SLOT_2;
    }
    else
    {
        return (taf_sim_Id_t) TAF_SIM_EXTERNAL_SLOT_1;
    }
}

COMPONENT_INIT
{
    taf_sim_Id_t simId = taf_sim_GetSelectedCard();
    taf_sim_LockType_t lockType = TAF_SIM_PIN1;
    taf_sim_Manufacturer_t manufacturer = TAF_SIM_GEMALTO;
    le_onoff_t powerStatus = LE_ON;
    const char* slotIdPtr = NULL;
    const char* pinPtr = "1234";
    const char* newPinPtr = "1234";
    const char* pukPtr = "12345678";
    const char* mcc = "000";
    const char* mnc = "00";

    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
    if (NumberOfArgs > 5) {
        slotIdPtr = le_arg_GetArg(0);
        pinPtr = le_arg_GetArg(1);
        newPinPtr = le_arg_GetArg(2);
        pukPtr = le_arg_GetArg(3);
        mcc = le_arg_GetArg(4);
        mnc = le_arg_GetArg(5);
        if (NULL == slotIdPtr)
        {
            LE_ERROR("slotId input is NULL, input correct slot id. Check usages for details.");
            printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
        } else {
            simId = get_slot_id(slotIdPtr);
            LE_INFO("Input slotId: %s, pin: %s, pin: %s, puk: %s", slotIdPtr, pinPtr, newPinPtr, pukPtr);
        }
    }
    else if (NumberOfArgs > 3) {
        slotIdPtr = le_arg_GetArg(0);
        pinPtr = le_arg_GetArg(1);
        newPinPtr = le_arg_GetArg(2);
        pukPtr = le_arg_GetArg(3);
        if (NULL == slotIdPtr)
        {
            LE_ERROR("slotId input is NULL, input correct slot id. Check usages for details.");
            printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
        } else {
            simId = get_slot_id(slotIdPtr);
            LE_INFO("Input slotId: %s, pin: %s, pin: %s, puk: %s", slotIdPtr, pinPtr, newPinPtr, pukPtr);
        }
    } else if (NumberOfArgs > 2) {
        slotIdPtr = le_arg_GetArg(0);
        pinPtr = le_arg_GetArg(1);
        newPinPtr = le_arg_GetArg(2);
        if (NULL == slotIdPtr)
        {
            LE_ERROR("slotId input is NULL, input correct slot id. Check usages for details.");
            printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
        } else {
            simId = get_slot_id(slotIdPtr);
            LE_INFO("Input slotId: %s, pin: %s, pin: %s", slotIdPtr, pinPtr, newPinPtr);
        }
    } else if (NumberOfArgs > 1) {
        slotIdPtr = le_arg_GetArg(0);
        pinPtr = le_arg_GetArg(1);
        if (NULL == slotIdPtr)
        {
            LE_ERROR("New PIN input is NULL, input correct New pin. Check usages for details.");
            printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
        } else {
            simId = get_slot_id(slotIdPtr);
            LE_INFO("Input slotId: %s, pin: %s", slotIdPtr, pinPtr);
        }
    } else if (NumberOfArgs > 0) {
        slotIdPtr = le_arg_GetArg(0);
        if (NULL == slotIdPtr)
        {
            LE_ERROR("slotId input is NULL, input correct slot id. Check usages for details.");
            printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
            exit(EXIT_SUCCESS);
        } else {
            simId = get_slot_id(slotIdPtr);
            LE_INFO("Input slotId: %s", slotIdPtr);
        }
    } else {
        LE_ERROR("Invalid input, input correct slotId, PIN, New PIN, PUK etc. Check usages for details.");
        printf("\n./tafSimUnitTest <slot1/slot2> <PIN> <NewPIN> <PUK> <FPLMN MCC> <FPLMN MNC>\n");
        exit(EXIT_SUCCESS);
    }
    if(mcc && mnc)
    {
       int mccInt = atoi(mcc);
       int mncInt = atoi(mnc);
       if (mccInt < 1 || mccInt > 999 || mncInt < 1 || mncInt > 999) {
            LE_INFO("No or wrong mcc mnc input! mcc: %s mnc: %s, so continue with default mcc-mnc (634-98).", mcc, mnc);
            mcc = "634";
            mnc = "98";
        }
    }

    LE_INFO("Start tafSimIntgTest app.");

    TestSemaphoreRef = le_sem_Create("SimSem", 0);
    ThreadRef = le_thread_Create("taf_sim_test_thread", Test_taf_Sim_Auth_AddHandler, NULL);
    le_thread_Start(ThreadRef);

    tafSimUnitTest_state(simId);

    IccidChangeHandlerRef = taf_sim_AddIccidChangeHandler(TestIccidChangeHandler, NULL);
    LE_TEST_OK(IccidChangeHandlerRef!=NULL, "taf_sim_AddIccidChangeHandler");
    NewSimStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
    LE_TEST_OK(NewSimStateHandlerRef!=NULL, "taf_sim_AddNewStateHandler");

    taf_sim_SetServerDisconnectHandler(SessionDisconnectHandler, NULL);

    if(simId && pinPtr && newPinPtr && pukPtr)
    {
        tafSimUnitTest_info(simId);

        tafSimUnitTest_selection(simId);

        tafSimUnitTest_enterPin(simId,lockType,pinPtr);

        tafSimUnitTest_Change_pin(simId, lockType, pinPtr, newPinPtr);

        tafSimUnitTest_unblock_puk(simId, lockType, pukPtr, newPinPtr);

        tafSimUnitTest_setLock(simId,lockType,newPinPtr, true);

        tafSimUnitTest_setLock(simId,lockType,newPinPtr, false);

        tafSimUnitTest_sendApdu(simId);

        tafSimUnitTest_sim_access(simId);

        tafSimUnitTest_swapToEmergencyAndBack(simId, manufacturer);

        tafSimUnitTest_SetPowerCheck(simId, powerStatus);

        LE_TEST_OK(LE_OK == taf_sim_Reset(simId), "taf_sim_Reset");

        tafSimUnitTest_sim_isEmergency(simId);

        tafSimUnitTest_fplmnList_test(simId, mcc, mnc);
    }
    taf_sim_RemoveAuthenticationResponseHandler(AuthResponseHandlerRef);

    taf_sim_RemoveIccidChangeHandler(IccidChangeHandlerRef);

    taf_sim_TryConnectService();

    taf_sim_RemoveNewStateHandler(NewSimStateHandlerRef);

    le_result_t result = le_thread_Cancel(ThreadRef);
    LE_TEST_OK(result == LE_OK, "Test_taf_Sim_RemoveHandler done");

    le_sem_Delete(TestSemaphoreRef);

    LE_INFO("tafSimUnitTests are executed successfully");
    printf("\nDone: tafSimUnitTests executed successfully!!!\n\n");

    taf_sim_DisconnectService();
    exit(EXIT_SUCCESS);
}
