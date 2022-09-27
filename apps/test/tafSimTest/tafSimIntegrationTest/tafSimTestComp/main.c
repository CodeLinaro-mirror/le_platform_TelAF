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
    printf("SIM selection test: app runProc tafSimTest --exe=tafSimTest -- select <slot1/slot2/unknown>\n");
    printf("SIM authentication test: app runProc tafSimTest --exe=tafSimTest -- enterPin <slot1/slot2/unknown> <pin1/pin2> pin\n");
    printf("SIM change pin  test: app runProc tafSimTest --exe=tafSimTest -- changePin <slot1/slot2/unknown> <pin1/pin2> old_pin new_pin\n");
    printf("SIM unblock  test: app runProc tafSimTest --exe=tafSimTest -- unblock <slot1/slot2/unknown> <puk1/puk2> puk new_pin\n");
    printf("SIM lock test: app runProc tafSimTest --exe=tafSimTest -- lock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM unlock test: app runProc tafSimTest --exe=tafSimTest -- unlock <slot1/slot2/unknown> <pin1/fdn> pin\n");
    printf("SIM access test: app runProc tafSimTest --exe=tafSimTest -- access <slot1/slot2/unknown>\n");
    printf("SIM SetPower test: app runProc tafSimTest --exe=tafSimTest -- setPower <slot1/slot2/unknown> <ON/OFF>\n");
    printf("SIM Reset test: app runProc tafSimIntgTest --exe=tafSimIntgTest -- Reset <slot1/slot2/unknown>\n");
    printf("SIM EMERGENCY test: app runProc tafSimIntgTest --exe=tafSimIntgTest -- isEmergency <slot1/slot2/unknown>\n");
    printf("SIM Swap Profiles test: app runProc tafSimIntgTest --exe=tafSimIntgTest -- swapProfiles <slot1/slot2/unknown> <0/1/2/3/4/5>\n");
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
    taf_sim_Id_t simId = 0;
    bool exitApplication = true;
    const char* testType = "";

    LE_INFO("Start tafSimIntgTest app.");
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

    if (NumberOfArgs > 2) {
        taf_sim_AuthenticationResponseHandlerRef_t responseHandlerRef_t;
        responseHandlerRef_t =taf_sim_AddAuthenticationResponseHandler(TestAuthenticationResponse, NULL);
        LE_ASSERT(responseHandlerRef_t != NULL);
    }

    if (strcmp(testType, "state") == 0) {
        tafSimTest_state(simId);
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
        tafSimTest_info(simId);
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
    else {
        DisplayAppUsage();
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        LE_INFO("Exit tafSimIntgTest App");
        exit(EXIT_SUCCESS);
    }
}
