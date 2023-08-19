/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#define EVENTS_POOL_SIZE   2
#define TEST_PSAP_NUMBER "911"

static le_sem_Ref_t TestSemaphoreRef;
static le_thread_Ref_t ThreadRef;
static taf_ecall_State_t ECallState;
static taf_ecall_StateChangeHandlerRef_t HandlerRef;
static uint8_t msdRawData[43] = {2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140, 65, 89,
            164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0, 0, 48, 20};
static uint8_t msdLength = 43;

static void Test_ecall_TerminateRegistration()
{
    le_result_t result = taf_ecall_TerminateRegistration();
    LE_TEST_OK(result == LE_OK, "Test_ecall_TerminateRegistration done");
    LE_INFO("TerminateECallRegistration completed!!!\n");
}

static void* Test_ECall_ExportMsd
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_ExportMsd(ecallRef, msdRawData, (size_t*)&msdLength);
    LE_TEST_OK(result == LE_OK, "Test_ECall_ExportMsd - LE_OK");
    LE_TEST_INFO("Test_ECall_ExportMsd done");
    return NULL;
}

static void* Test_ECall_SendMsd
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_SendMsd(ecallRef);
    LE_TEST_OK(result == LE_OK, "taf_ecall_SendMsd_test");
    LE_TEST_INFO("taf_ecall_SendMsd_test done");
    return NULL;
}

static void tafECallStateHandler( taf_ecall_CallRef_t eCallReference,
        taf_ecall_State_t state, void* cntxtPtr)
{

    LE_INFO("Ecall state change event state = %d", state );
    LE_INFO("Ecall state change event reference = %p", eCallReference );
    ECallState = state;

    switch (state)
    {
        case TAF_ECALL_STATE_UNKNOWN:
        {
            LE_INFO("TAF_ECALL_STATE_UNKNOWN");
            break;
        }
        case TAF_ECALL_STATE_ALERTING:
        {
            LE_INFO("TAF_ECALL_STATE_ALERTING");
            break;
        }
        case TAF_ECALL_STATE_ACTIVE:
        {
            LE_INFO("TAF_ECALL_STATE_ACTIVE");
            Test_ECall_ExportMsd(eCallReference);
            Test_ECall_SendMsd(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_IDLE:
        {
            LE_INFO("TAF_ECALL_STATE_IDLE");
            break;
        }
        case TAF_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("TAF_ECALL_STATE_WAITING_PSAP_START_IND");
            break;
        }
        case TAF_ECALL_STATE_PSAP_START_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_PSAP_START_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_LLNACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_LL_ACK_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_LL_ACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED");
            le_sem_Post(TestSemaphoreRef);
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE");
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN");
            break;
        }
        case TAF_ECALL_STATE_ENDED:
        {
            LE_INFO("TAF_ECALL_STATE_ENDED");
            if (eCallReference != NULL)
            {
                taf_ecall_TerminationReason_t lcf = taf_ecall_GetTerminationReason(eCallReference);
                LE_INFO("TAF_ECALL_STATE_ENDED LCF = %d", (int) lcf);
            }
            Test_ecall_TerminateRegistration();
            le_sem_Post(TestSemaphoreRef);
            break;
        }
        case TAF_ECALL_STATE_RESET:
        {
            LE_INFO("TAF_ECALL_STATE_RESET");
            break;
        }
        case TAF_ECALL_STATE_COMPLETED:
        {
            LE_INFO("TAF_ECALL_STATE_COMPLETED");
            break;
        }
        case TAF_ECALL_STATE_FAILED:
        {
            LE_INFO("TAF_ECALL_STATE_FAILED");
            break;
        }
        case TAF_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("TAF_ECALL_STATE_END_OF_REDIAL_PERIOD");
            break;
        }
        case TAF_ECALL_STATE_T2_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_TIMEOUT_T3:
        {
            LE_INFO("TAF_ECALL_STATE_TIMEOUT_T3");
            break;
        }
        case TAF_ECALL_STATE_T5_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T6_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T7_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T9_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T10_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_DIALING:
        {
            LE_INFO("TAF_ECALL_STATE_DIALING");
            break;
        }
        case TAF_ECALL_STATE_NACK_OUT_OF_ORDER:
        {
            LE_INFO("TAF_ECALL_STATE_NACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_ACK_OUT_OF_ORDER:
        {
            LE_INFO("TAF_ECALL_STATE_ACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE");
            break;
        }
        default:
        {
            LE_INFO("Unknown state");
            break;
        }
    }

}

static void Test_ECall_OperatingMode()
{
    taf_ecall_ForceOnlyMode(1);
    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    LE_TEST_OK( taf_ecall_GetConfiguredOperationMode(1, &opMode) == LE_OK, "Test_ECall_OperatingMode done");
    LE_TEST_OK(opMode == TAF_ECALL_MODE_ECALL, "taf_ecall_GetConfiguredOperationMode done");
    LE_INFO("Operating mode = %d", opMode);
    taf_ecall_ForcePersistentOnlyMode(1);
    taf_ecall_GetConfiguredOperationMode(1, &opMode);
    LE_TEST_OK(opMode == TAF_ECALL_MODE_ECALL, "taf_ecall_ForcePersistentOnlyMode done");
    LE_INFO("Operating mode = %d", opMode);
    taf_ecall_ExitOnlyMode(1);
    taf_ecall_GetConfiguredOperationMode(1, &opMode);
    LE_TEST_OK(opMode == TAF_ECALL_MODE_NORMAL, "taf_ecall_ExitOnlyMode done");
    LE_INFO("Operating mode = %d", opMode);
}

static void Test_MSD_Information()
{
    taf_ecall_CallRef_t   eCallRef = 0x00;
    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    char vin[TAF_ECALL_MAX_VIN_BYTES];
    le_result_t res = LE_FAULT;
    taf_ecall_MsdTransmissionMode_t mode = TAF_ECALL_MSD_TX_MODE_PULL;

    uint32_t msdVersion = 0;
    LE_TEST_OK(taf_ecall_SetMsdVersion(4) == LE_OK, "Test_MSD_Information done");
    LE_TEST_OK(taf_ecall_GetMsdVersion(&msdVersion) == LE_OK, "taf_ecall_GetMsdVersion done");
    LE_TEST_OK(msdVersion == 4, "taf_ecall_GetMsdVersion done");
    LE_INFO("Set and Get MSD version completed");

    LE_TEST_OK(taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PUSH) == LE_OK, "taf_ecall_SetMsdTxMode done");
    LE_TEST_OK(taf_ecall_GetMsdTxMode(&mode) == LE_OK, "taf_ecall_GetMsdTxMode done");
    LE_TEST_OK(mode == TAF_ECALL_TX_MODE_PUSH, "Test_MSD_Information done");
    LE_INFO("Set and Get MSD transmission mode completed");

    LE_TEST_OK(taf_ecall_SetVIN("ECALLEXAMPLE") == LE_OK, "taf_ecall_SetVIN done");;
    LE_TEST_OK(taf_ecall_SetVIN("EOALLEXAMPLE02013") == LE_OK, "taf_ecall_SetVIN done");
    LE_TEST_OK(taf_ecall_SetVIN("ECALLIXAMPLE02013") == LE_OK, "taf_ecall_SetVIN done");
    LE_TEST_OK(taf_ecall_SetVIN("ECALLEXAMPLQ02013") == LE_OK, "taf_ecall_SetVIN done");
    LE_TEST_OK(taf_ecall_SetVIN("ECALLEXAMPLE02013") == LE_OK, "taf_ecall_SetVIN done");
    LE_TEST_OK(taf_ecall_GetVIN(vin, TAF_ECALL_MAX_VIN_BYTES) == LE_OK, "taf_ecall_GetVIN done");
    LE_TEST_OK(strcmp(vin, "ECALLEXAMPLE02013") == 0, "Test_MSD_Information done");
    LE_INFO("Set and Get Vehicle identification number completed");

    LE_TEST_OK(taf_ecall_SetVehicleType(vehType) == LE_OK, "taf_ecall_SetVehicleType done");

    vehType = TAF_ECALL_BUSES_AND_COACHES_CLASS_M2;
    LE_TEST_OK( taf_ecall_GetVehicleType(&vehType) == LE_OK, "taf_ecall_GetVehicleType done");
    LE_TEST_OK(( TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1 == vehType ), "taf_ecall_SetVehicleType done");
    vehType = 20;
    LE_TEST_OK(taf_ecall_SetVehicleType(vehType) == LE_OK, "taf_ecall_SetVehicleType done");
    LE_INFO("Set and Get Vehicle type completed");

    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;
    LE_TEST_OK(taf_ecall_SetPropulsionType(propulsionStorage) == LE_OK, "taf_ecall_SetPropulsionType done");
    LE_TEST_OK((LE_OK == taf_ecall_GetPropulsionType(&propulsionStorage)), "taf_ecall_GetPropulsionType done");
    LE_TEST_OK( TAF_ECALL_PROP_TYPE_GASOLINE_TANK == propulsionStorage, "taf_ecall_SetPropulsionType done");
    propulsionStorage = 1000;
    LE_TEST_OK(taf_ecall_SetPropulsionType(propulsionStorage) == LE_OK, "taf_ecall_SetPropulsionType done");
    LE_TEST_OK((LE_OK == taf_ecall_GetPropulsionType(&propulsionStorage)), "taf_ecall_GetPropulsionType done");
    LE_INFO("Set and Get Vehicle type completed %d", propulsionStorage);
    LE_INFO("Set and Get propulsion type completed");

    LE_TEST_OK((eCallRef= taf_ecall_Create()) != NULL, "taf_ecall_Create done");

    res = taf_ecall_SetMsdPosition(eCallRef, true, +118422000, -421902360, 0);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPosition done");
    LE_INFO("Set msd position completed");

    res = taf_ecall_SetMsdPositionN1(eCallRef, -520, 520);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN1 done");

    res = taf_ecall_SetMsdPositionN1(eCallRef, 511, 511);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN1 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -520, 520);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN2 done");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -512, -512);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN2 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPassengersCount(eCallRef, 2);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPassengersCount done");
    LE_INFO("Set number of passengers completed");

    LE_INFO("Set msd information test completed");
}

static void Test_ecall_GetNadDeregTime()
{
    uint16_t deregTimeOrg = 0;
    le_result_t res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadDeregTime done");
    LE_INFO("GetNadDeregTime done!!! DeregTime (in minutes): %d\n", deregTimeOrg);
}

static void Test_ecall_SetNadDeregTime()
{
    le_result_t res = taf_ecall_SetNadDeregistrationTime(9*60); // 9 hrs
    LE_TEST_OK(res == LE_OK, "Test_ecall_SetNadDeregTime done");
    LE_INFO("SetNadDeregistrationTime as 9 hrs completed!!!\n");
}

static void Test_ECall_StartAutomatic() {
    taf_ecall_CallRef_t eCallRef = NULL;

    eCallRef = taf_ecall_Create();

    le_result_t res = taf_ecall_StartAutomatic(eCallRef);
    taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic callState = %d", (int) retrievedState);

    Test_ECall_ExportMsd(eCallRef);
    Test_ECall_SendMsd(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    res = taf_ecall_End(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    taf_ecall_TerminationReason_t lcf = taf_ecall_GetTerminationReason(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic LCF = %d", (int) lcf);

    taf_ecall_Delete(eCallRef);

    eCallRef = NULL;
}

static void Test_ECall_StartManual() {
    taf_ecall_CallRef_t eCallRef = NULL;

    eCallRef = taf_ecall_Create();

    le_result_t res = taf_ecall_StartManual(eCallRef);
    taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartManual callState = %d", (int) retrievedState);

    Test_ECall_ExportMsd(eCallRef);
    Test_ECall_SendMsd(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    res = taf_ecall_End(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    taf_ecall_TerminationReason_t lcf = taf_ecall_GetTerminationReason(eCallRef);
    LE_INFO("Test_ECall_StartManual LCF = %d", (int) lcf);

    taf_ecall_Delete(eCallRef);

    eCallRef = NULL;
}

static void Test_ECall_StartTest() {
    taf_ecall_CallRef_t eCallRef = NULL;

    eCallRef= taf_ecall_Create();

    taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);

    le_result_t res = taf_ecall_StartTest(eCallRef);
    taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartTest callState = %d", (int) retrievedState);

    Test_ECall_ExportMsd(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    taf_ecall_Delete(eCallRef);

    eCallRef = NULL;
}

static void Test_ECall_SetGetPsapNumber() {

    le_result_t res = taf_ecall_SetPsapNumber(TEST_PSAP_NUMBER);
    LE_TEST_OK(res == LE_OK, "Test_ECall_SetGetPsapNumber done");

    char num[15];
    res = taf_ecall_GetPsapNumber(num, 15);
    LE_TEST_OK(res == LE_OK, "Test_ECall_GetGetPsapNumber done");
    LE_TEST_OK(strncmp(num, TEST_PSAP_NUMBER, sizeof(TEST_PSAP_NUMBER)) == 0, "Test_ECall_GetGetPsapNumber done");

    res = taf_ecall_UseUSimNumbers();
    LE_TEST_OK(res == LE_OK, "taf_ecall_UseUSimNumbers done");
}

static void* Test_taf_ecall_AddHandler(void* context) {

    taf_ecall_ConnectService();

    HandlerRef = taf_ecall_AddStateChangeHandler(tafECallStateHandler, NULL);
    LE_TEST_OK(HandlerRef != NULL, "Test_taf_ecall_AddHandler done");
    LE_INFO("Add State change handler complete. The HandlerRef = %p", HandlerRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_ecall_RemoveHandler(void* param1, void* param2) {

    taf_ecall_TryConnectService();

    taf_ecall_RemoveStateChangeHandler(HandlerRef);

    le_sem_Post(TestSemaphoreRef);

}

COMPONENT_INIT
{

    Test_ECall_OperatingMode();

    Test_MSD_Information();

    Test_ECall_SetGetPsapNumber();

    Test_ecall_GetNadDeregTime();
    Test_ecall_SetNadDeregTime();

    TestSemaphoreRef = le_sem_Create("ECallSem", 0);

    ThreadRef = le_thread_Create("EctThread", Test_taf_ecall_AddHandler, NULL);
    le_thread_Start(ThreadRef);

    Test_ECall_StartTest();

    Test_ECall_StartManual();

    Test_ECall_StartAutomatic();

    le_event_QueueFunctionToThread(ThreadRef, Test_taf_ecall_RemoveHandler, NULL, NULL);

    le_sem_Wait(TestSemaphoreRef);

    Test_ecall_TerminateRegistration();

    le_result_t result = le_thread_Cancel(ThreadRef);
    LE_TEST_OK(result == LE_OK, "Test_taf_ecall_RemoveHandler done");

    le_sem_Delete(TestSemaphoreRef);

    // Disconnect the telaf service.
    taf_ecall_DisconnectService();

    LE_INFO("ECall API Unit test execution success");

    exit(EXIT_SUCCESS);
}

