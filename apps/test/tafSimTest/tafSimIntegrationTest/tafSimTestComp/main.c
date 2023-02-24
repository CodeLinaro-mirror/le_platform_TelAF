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

static taf_sim_NewStateHandlerRef_t NewSimStateHandlerRef = NULL;
static taf_sim_IccidChangeHandlerRef_t IccidChangeHandlerRef = NULL;

static void DisplayAppUsage(void) {
    printf("Usage of the 'tafsimTest' application is:\n");
    printf("Test SIM state: app runProc tafSimTest --exe=tafSimTest -- state <slot1/slot2/unknown>\n");
    printf("Test SIM state change: app runProc tafSimTest --exe=tafSimTest -- events\n");
    printf("SIM information test: app runProc tafSimTest --exe=tafSimTest -- info <slot1/slot2/unknown>\n");
    printf("SIM informations test: app runProc tafSimTest --exe=tafSimTest -- infoAll <slot1/slot2/unknown>\n");
    printf("SIM selection test: app runProc tafSimTest --exe=tafSimTest -- select <slot1/slot2/unknown>\n");
    printf("SIM authentication test: app runProc tafSimTest --exe=tafSimTest -- enterPin <slot1/slot2/unknown> <pin1/pin2> pin\n");
    printf("SIM change pin  test: app runProc tafSimTest --exe=tafSimTest -- changePin <slot1/slot2/unknown> <pin1/pin2> old_pin new_pin\n");
    printf("SIM unblock  test: app runProc tafSimTest --exe=tafSimTest -- unblock <slot1/slot2/unknown> <puk1/puk2> puk new_pin\n");
    printf("SIM lock test: app runProc tafSimTest --exe=tafSimTest -- lock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM unlock test: app runProc tafSimTest --exe=tafSimTest -- unlock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM access test: app runProc tafSimTest --exe=tafSimTest -- access <slot1/slot2/unknown>\n");
    printf("SIM SetPower test: app runProc tafSimTest --exe=tafSimTest -- setPower <slot1/slot2/unknown> <ON/OFF>\n");
    printf("SIM Reset test: app runProc tafSimIntTest --exe=tafSimIntTest -- Reset <slot1/slot2/unknown>\n");
    printf("SIM EMERGENCY test: app runProc tafSimIntTest --exe=tafSimIntTest -- isEmergency <slot1/slot2/unknown>\n");
    printf("SIM Swap Profiles test: app runProc tafSimIntTest --exe=tafSimIntTest -- swapProfiles <slot1/slot2/unknown> <0/1/2/3/4/5>\n");
    printf("Get Forbidden PLMN list: app runProc tafSimIntTest --exe=tafSimIntTest -- fplmnList <slot1/slot2/unknown>\n");
    printf("Create Forbidden PLMN list: app runProc tafSimIntTest --exe=tafSimIntTest -- createFplmnList <slot1/slot2/unknown>\n");
    printf("Add Forbidden PLMN operator: app runProc tafSimIntTest --exe=tafSimIntTest -- addFplmnOp <slot1/slot2/unknown> <mcc> <mnc>\n");
    printf("Write Forbidden PLMN list: app runProc tafSimIntTest --exe=tafSimIntTest -- writeFplmnOp <slot1/slot2/unknown> <mcc> <mnc>\n");
    printf("Write Forbidden PLMNs list: app runProc tafSimIntTest --exe=tafSimIntTest -- writeFplmnList <slot1/slot2/unknown>\n");
    printf("Get First FPLMN operator: app runProc tafSimIntTest --exe=tafSimIntTest -- firstFplmnOp <slot1/slot2/unknown>\n");
    printf("Get Next FPLMN operator: app runProc tafSimIntTest --exe=tafSimIntTest -- nextFplmnOp <slot1/slot2/unknown>\n");
    printf("Delete Next FPLMN List: app runProc tafSimIntTest --exe=tafSimIntTest -- deleteFplmnList <slot1/slot2/unknown>\n");
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
    exit(EXIT_SUCCESS);
}

static void TestIccidChangeHandler(taf_sim_Id_t simId, const char* Iccid, void* contextPtr) {
    LE_INFO("Iccid Change event for SIM card: %d", simId);
    LE_INFO("ICCID is: %s", (const char*)Iccid);
    exit(EXIT_SUCCESS);
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

COMPONENT_INIT
{
    taf_sim_Id_t simId = taf_sim_GetSelectedCard();
    bool exitApplication = true;
    const char* testType = "";

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
        exitApplication = false;
        tafSimTest_enterPin(simId,lockType,pinPtr);
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
        tafSimTest_Change_pin(simId, lockType, oldpinPtr, newpinPtr);
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
        tafSimTest_unblock_puk(simId, lockType, pukPtr, pinPtr);
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
        exitApplication = false;
        tafSimTest_setLock(simId,lockType,pinPtr, true);
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
        exitApplication = false;
        tafSimTest_setLock(simId,lockType,pinPtr, false);

    }else if (strcmp(testType, "access") == 0)
    {
        tafSimTest_sim_access(simId);
    }else if(strcmp(testType, "swapProfiles") == 0)
    {
        taf_sim_Manufacturer_t manufacturer = (taf_sim_Manufacturer_t)le_arg_GetArg(2);
        tafSimTest_swapToEmergencyAndBack(simId, manufacturer);
    }
    else if (strcmp(testType, "setPower") == 0)
    {
        const char* powerStatusPtr = le_arg_GetArg(2);
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
        tafSimTest_fplmnList_test(simId);
    }
    else if (strncmp(testType, "createFplmnList", 15) == 0)
    {
        tafSimTest_createFplmnList_test(simId);
    }
    else if (strncmp(testType, "addFplmnOp", 10) == 0)
    {
        const char* mcc = le_arg_GetArg(2);
        if (NULL == mcc)
        {
            LE_ERROR("mcc is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        const char* mnc = le_arg_GetArg(3);
        if (NULL == mnc)
        {
            LE_ERROR("mnc is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        tafSimTest_addFplmnOperator_test(simId, mcc, mnc);
    }
    else if (strcmp(testType, "writeFplmnOp") == 0)
    {
        const char* mccPtr = le_arg_GetArg(2);
        if (NULL == mccPtr)
        {
            LE_ERROR("mcc is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        const char* mncPtr = le_arg_GetArg(3);
        if (NULL == mncPtr)
        {
            LE_ERROR("mnc is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
        tafSimTest_writeFplmnList_test(simId, mccPtr, mncPtr);
    }
    else if (strcmp(testType, "writeFplmnList") == 0)
    {
        tafSimTest_writeFplmnLists_test(simId);
    }
    else if (strncmp(testType, "firstFplmnOp", 13) == 0)
    {
        tafSimTest_getFirstFplmnOperator_test(simId);
    }
    else if (strncmp(testType, "nextFplmnOp", 11) == 0)
    {
        tafSimTest_getNextFplmnOperator_test(simId);
    }
    else if (strncmp(testType, "deleteFplmnList", 15) == 0)
    {
        tafSimTest_deleteFplmnList_test(simId);
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
