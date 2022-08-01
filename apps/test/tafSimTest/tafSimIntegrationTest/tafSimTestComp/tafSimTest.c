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
#include "main.h"

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

//Function to test sim state
void tafSimTest_state
(
    taf_sim_Id_t simId
)
{
    taf_sim_States_t             state;
    taf_sim_NewStateHandlerRef_t testNewStateHandlerRef;

    testNewStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
    LE_ASSERT(NULL != testNewStateHandlerRef);

    state = taf_sim_GetState(simId);

    LE_INFO("test: state %d", state);

    LE_ASSERT((state >= TAF_SIM_INSERTED) && (state <= TAF_SIM_ERROR));

    printf("\n Sim Card.%d state = %s\n" , simId ,SimStateToString(state));
    printf("\n Is SIM card Ready = %s\n", taf_sim_IsReady(simId) ? "true" : "false");
    printf("\n Is SIM card Present = %s \n", taf_sim_IsPresent(simId) ? "true" : "false");
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
    char            phoneNumber[TAF_SIM_PHONE_NUM_MAX_BYTES];
    char            operatorName[50];
    char            mcc[4];
    char            mnc[4];

    memset(iccid, 0, TAF_SIM_ICCID_BYTES);
    memset(imsi, 0, TAF_SIM_IMSI_BYTES);
    memset(phoneNumber, 0, TAF_SIM_PHONE_NUM_MAX_BYTES);
    memset(operatorName, 0, 50);
    memset(mcc, 0, 4);
    memset(mnc, 0, 4);

    LE_INFO("SimId %d", simId);

    // Get SIM ICCID
    res = taf_sim_GetICCID(simId, iccid, sizeof(iccid));
    LE_ASSERT(res == LE_OK);
    printf("\nSIM Card ICCID: '%s'\n", iccid);

    res = taf_sim_GetIMSI(simId, imsi, sizeof(imsi));
    LE_ASSERT(res == LE_OK);
    printf("\nSIM Card IMSI: '%s'\n", imsi);

    res = taf_sim_GetSubscriberPhoneNumber(simId, phoneNumber, sizeof(phoneNumber));
    LE_ASSERT(res == LE_OK);
    printf("\nSIM Card PhoneNumber: '%s'\n", phoneNumber);

    res = taf_sim_GetHomeNetworkOperator(simId, operatorName, sizeof(operatorName));
    LE_ASSERT(res == LE_OK);
    printf("\nSIM Card Network Operator name: '%s'\n", operatorName);

    res = taf_sim_GetHomeNetworkMccMnc(simId, mcc, sizeof(mcc), mnc, sizeof(mnc));
    LE_ASSERT(res == LE_OK);
    printf("\nSIM Card MCC: '%s'\n", mcc);
    printf("\nSIM Card MNC: '%s'\n", mnc);
}
void tafSimTest_selection
(
    taf_sim_Id_t slot
)
{
    taf_sim_Id_t slotId = taf_sim_GetSelectedCard();
    printf("\n Current SIM slot id = %d\n" ,slotId);

    le_result_t res = taf_sim_SelectCard(slot);
    LE_ASSERT(res == LE_OK);
    slotId = taf_sim_GetSelectedCard();
    printf("\n After selecting %d Current SIM slot id = %d\n" ,slot, slotId);
}

void tafSimTest_enterPin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pinPtr
)
{
    le_result_t res;
    res = taf_sim_EnterPIN(simId, lockType, pinPtr);
    LE_ASSERT(res == LE_OK);
    LE_INFO("EnterPIN done");
}

void tafSimTest_setLock
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
        LE_ASSERT(res == LE_OK);
        LE_INFO("Set lock request sent successfully");
    } else {
        res = taf_sim_Unlock(simId, lockType, pinPtr);
        LE_ASSERT(res == LE_OK);
        LE_INFO("Unlock request sent successfully");
    }
}

void tafSimTest_Change_pin
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  oldpinPtr,
    const char*  newpinPtr
)
{
    le_result_t res;
    res = taf_sim_ChangePIN(simId, lockType, oldpinPtr, newpinPtr);
    LE_ASSERT(res == LE_OK);
}

void tafSimTest_unblock_puk
(
    taf_sim_Id_t simId,
    taf_sim_LockType_t lockType,
    const char*  pukPtr,
    const char*  newpinPtr
)
{
    le_result_t res;
    res = taf_sim_Unblock(simId, lockType, pukPtr, newpinPtr);
    LE_ASSERT(res == LE_OK);
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
    LE_ASSERT(channel);

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
