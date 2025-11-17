/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define DEFAULT_PHONE_ID 1
#define EVENTS_POOL_SIZE   2
#define TEST_PSAP_NUMBER "10010"

static le_sem_Ref_t testSemaphoreRef;
static le_thread_Ref_t threadRef = NULL, imsServStatusThreadRef = NULL;
static taf_ecall_StateChangeHandlerRef_t handlerRef;
static taf_radio_ImsStatusChangeHandlerRef_t imsServStatusHandlerRef;
static uint8_t msdRawData[43] = {2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140, 65, 89,
            164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0, 0, 48, 20};
static uint8_t msdLength = 43;
static uint8_t oadDataFirst[8] = {8, 41, 68, 6, 128, 20, 8, 9};
static uint8_t oadDataLengthFirst = 8;
static uint8_t oadDataSec[6] = {8, 41, 68, 6, 128, 20};
static uint8_t oadDataLengthSec = 6;
bool waitT10StopForAuto = false;



static void Test_ECall_OperationMode()
{
    le_result_t res = LE_FAULT;
    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;

    res = taf_ecall_ForceOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(res == LE_OK, "taf_ecall_ForceOnlyMode done");
    res = taf_ecall_GetConfiguredOperationMode(DEFAULT_PHONE_ID, &opMode);
    LE_TEST_OK(opMode == TAF_ECALL_MODE_ECALL, "taf_eCall_GetConfiguredOperationMode done");

    res = taf_ecall_ExitOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(res == LE_OK, "taf_ecall_ExitOnlyMode done");
    res = taf_ecall_GetConfiguredOperationMode(DEFAULT_PHONE_ID, &opMode);
    LE_TEST_OK(opMode == TAF_ECALL_MODE_NORMAL, "taf_ecall_GetConfiguredOperationMode done");
}

static void Test_ECall_MSD_Information()
{
    taf_ecall_CallRef_t   eCallRef = 0x00;
    le_result_t res = LE_FAULT;

    uint32_t msdVersion = 0;
    res = taf_ecall_SetMsdVersion(4);
    LE_TEST_OK(res == LE_FAULT, "Test taf_ecall_SetMsdVersion done");

    res = taf_ecall_SetMsdVersion(3);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetMsdVersion done");
    LE_TEST_OK(taf_ecall_GetMsdVersion(&msdVersion) == LE_OK, "Test taf_ecall_GetMsdVersion done");
    LE_TEST_OK(msdVersion == 3, "Test taf_ecall_SetMsdVersion and taf_ecall_GetMsdVersion done");

    res = taf_ecall_SetMsdVersion(2);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetMsdVersion done");
    LE_TEST_OK(taf_ecall_GetMsdVersion(&msdVersion) == LE_OK, "Test taf_ecall_GetMsdVersion done");
    LE_TEST_OK(msdVersion == 2, "Test taf_ecall_SetMsdVersion and taf_ecall_GetMsdVersion done");
    LE_INFO("Set and get MSD version completed");

    taf_ecall_MsdTransmissionMode_t mode = TAF_ECALL_MSD_TX_MODE_PULL;
    res = taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PUSH);
    LE_TEST_OK(res == LE_UNSUPPORTED, "Test taf_ecall_SetMsdTxMode done");
    res = taf_ecall_GetMsdTxMode(&mode);
    LE_TEST_OK(res == LE_UNSUPPORTED, "Test taf_ecall_GetMsdTxMode done");
    LE_TEST_OK(mode == TAF_ECALL_TX_MODE_PUSH || res == LE_UNSUPPORTED, "Test taf_ecall_GetMsdTxMode done");
    LE_INFO("Set and get MSD transmission mode completed");

    char vin[TAF_ECALL_MAX_VIN_BYTES];
    res = taf_ecall_SetVIN("ECALLEXAMPLE");//invalid input, result will be failed.
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "Test taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("EOALLEXAMPLE02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "Test taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLIXAMPLE02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "Test taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLEXAMPLQ02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "Test taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLEXAMPLE02013");
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetVIN done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetVIN(vin, TAF_ECALL_MAX_VIN_BYTES);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_GetVIN done");
        if (res == LE_OK)
        {
            LE_TEST_OK(strcmp(vin, "ECALLEXAMPLE02013") == 0, "Test taf_ecall_SetVIN and taf_ecall_GetVIN done");
        }
    }
    LE_INFO("Set and get vehicle identification number completed");

    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    res = taf_ecall_SetVehicleType(vehType);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetVehicleType done");
    if (res == LE_OK)
    {
        vehType = TAF_ECALL_BUSES_AND_COACHES_CLASS_M2;
        res = taf_ecall_GetVehicleType(&vehType);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_GetVehicleType done");
        if (res == LE_OK)
        {
            LE_TEST_OK(( TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1 == vehType ), "Test taf_ecall_SetVehicleType and taf_ecall_GetVehicleType done");
        }
    }
    vehType = 30; //invalid vehicle type, result will be failed.
    LE_TEST_OK(taf_ecall_SetVehicleType(vehType) == LE_FAULT, "Test taf_ecall_SetVehicleType done");
    LE_INFO("Set and get vehicle type completed");

    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;
    res = taf_ecall_SetPropulsionType(propulsionStorage);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetPropulsionType done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetPropulsionType(&propulsionStorage);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_GetPropulsionType done");
        if (res == LE_OK)
        {
            LE_TEST_OK( TAF_ECALL_PROP_TYPE_GASOLINE_TANK == propulsionStorage, "Test taf_ecall_SetPropulsionType and taf_ecall_GetPropulsionType done");
        }
    }
    propulsionStorage = 1000;
    res = taf_ecall_SetPropulsionType(propulsionStorage);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetPropulsionType done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetPropulsionType(&propulsionStorage);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_GetPropulsionType done");
        if (res == LE_OK)
        {
            LE_TEST_OK( (TAF_ECALL_PROP_TYPE_PROPANE_GAS | TAF_ECALL_PROP_TYPE_HYDROGEN | TAF_ECALL_PROP_TYPE_OTHER) == propulsionStorage, "Test taf_ecall_SetPropulsionType and taf_ecall_GetPropulsionType done");
        }
    }
    LE_INFO("Set and get propulsion type completed");

    LE_TEST_OK((eCallRef= taf_ecall_Create()) != NULL, "Test taf_ecall_Create done");

    res = taf_ecall_SetMsdPosition(eCallRef, true, +118422000, -421902360, 0);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetMsdPosition done");
    LE_INFO("Set msd position completed");

    res = taf_ecall_SetMsdPositionN1(eCallRef, -520, 520);//Boundary check, result will be failed.
    LE_TEST_OK(res != LE_OK, "Test taf_ecall_SetMsdPositionN1 done");
    res = taf_ecall_SetMsdPositionN1(eCallRef, 511, 511);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "Test taf_ecall_SetMsdPositionN1 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -520, 520);//Boundary check, result will be failed.
    LE_TEST_OK(res != LE_OK, "Test taf_ecall_SetMsdPositionN2 done");
    res = taf_ecall_SetMsdPositionN2(eCallRef, -512, -512);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "Test taf_ecall_SetMsdPositionN2 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPassengersCount(eCallRef, 2);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "Test taf_ecall_SetMsdPassengersCount done");
    LE_INFO("Set number of passengers completed");

    res = taf_ecall_SetMsdTimeStamp(eCallRef, 4294967295);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetMsdTimeStamp done");
    LE_INFO("Set msd timestamp completed");
    res = taf_ecall_ResetMsdTimeStamp(eCallRef);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_ResetMsdTimeStamp done");
    LE_INFO("Reset msd timestamp completed");
    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdAdditionalData done");
    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1.2", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdAdditionalData done");
    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1", oadDataSec, oadDataLengthSec);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdAdditionalData done");

    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_ResetMsdAdditioanllData done");

    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, 10);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPLocationOfImpact done");
    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 90, -45, 10);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPIIDeltaV done");
    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 120, -251, 10);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPIIDeltaV done");
    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 120, -201, 251);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPIIDeltaV done");
    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, TAF_ECALL_LOI_FRONT);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPLocationOfImpact done");
    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 125, -45, 10);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPIIDeltaV done");
    res = taf_ecall_SetMsdEuroNCAPRolloverDetected(eCallRef, 1);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPRolloverDetected done");
    res = taf_ecall_ResetMsdEuroNCAPRolloverDetected(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_ResetMsdEuroNCAPRolloverDetected done");
    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_ResetMsdAdditioanllData done");
    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, TAF_ECALL_LOI_NONDRIVERSIDE);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetMsdEuroNCAPLocationOfImpact done");
    taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);
    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1.2", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res != LE_OK, "Test taf_ecall_SetMsdAdditionalData done done");

    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res != LE_OK, "Test taf_ecall_ResetMsdAdditionalData done");
    LE_INFO("Set additional data completed");
    res = taf_ecall_SetMsdTimeStamp(eCallRef, 4294967295);
    LE_TEST_OK(res == LE_DUPLICATE, "Test taf_ecall_SetMsdTimeStamp done");
    LE_INFO("Set msd timestamp completed");

    LE_INFO("Set msd information test completed");
}

static void Test_ECall_PsapNumber() {
    le_result_t res = LE_FAULT;
    res = taf_ecall_UseUSimNumbers();
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_UseUSimNumbers done");
    res = taf_ecall_SetPsapNumber(TEST_PSAP_NUMBER);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_PsapNumber done");
    char num[15];
    res = taf_ecall_GetPsapNumber(num, 15);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetGetPsapNumber done");
    LE_TEST_OK(strncmp(num, TEST_PSAP_NUMBER, sizeof(TEST_PSAP_NUMBER)) == 0, "Test taf_ecall_GetPsapNumber done");
    LE_INFO("Set and get PsapNumber completed");
}

static void Test_ECall_HlapTimer()
{
    le_result_t res = LE_FAULT;

    uint16_t minNwRegTime = 0;
    res = taf_ecall_SetNadMinNetworkRegistrationTime(13*60);
    LE_TEST_OK(res == LE_FAULT, "Test taf_ecall_SetNadMinNetworkRegistrationTime done");
    LE_INFO("SetNadDeregistrationTime as 13 hrs done");

    res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetNadMinNetworkRegistrationTime done");
    LE_INFO("GetNadMinNetworkRegistrationTime %d done", minNwRegTime);

    res = taf_ecall_SetNadMinNetworkRegistrationTime(1);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetNadMinNetworkRegistrationTime done");
    LE_INFO("SetNadMinNetworkRegistrationTime as 1 min done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
        LE_TEST_OK(res == LE_OK && minNwRegTime == 1, "Test taf_ecall_GetNadMinNetworkRegistrationTime done");
        LE_INFO("GetNadMinNetworkRegistrationTime as %d done", minNwRegTime);
    }
    LE_INFO("Set and get NadMinNetworkRegistrationTime completed");

    uint16_t deregTimeOrg = 0;
    res = taf_ecall_SetNadDeregistrationTime(13*60);
    LE_TEST_OK(res == LE_FAULT, "Test taf_ecall_SetNadDeregTime done");
    LE_INFO("SetNadDeregistrationTime as 13 hrs done");

    res = taf_ecall_SetNadDeregistrationTime(12*60);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetNadDeregTime done");
    LE_INFO("SetNadDeregistrationTime as 12 hrs done");
    res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    LE_TEST_OK(res == LE_OK && deregTimeOrg == 12*60, "Test taf_ecall_GetNadDeregTime done");
    LE_INFO("GetNadDeregTime as %d done", deregTimeOrg);

    res = taf_ecall_SetNadDeregistrationTime(2);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetNadDeregTime done");
    LE_INFO("SetNadDeregistrationTime as 2 min done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
        LE_TEST_OK(res == LE_OK && deregTimeOrg == 2, "Test taf_ecall_GetNadDeregTime done");
        LE_INFO("GetNadDeregTime as %d done", deregTimeOrg);
    }
    LE_INFO("Set and get NadDeregTime completed");

    uint16_t ccftTimeOrg = 0;
    res = taf_ecall_SetNadClearDownFallbackTime(13*60);
    LE_TEST_OK(res == LE_FAULT, "Test taf_ecall_SetNadClearDownFallbackTime done");
    LE_INFO("SetNadClearDownFallbackTime as 13 hrs done");

    res = taf_ecall_SetNadClearDownFallbackTime(12*60);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetNadClearDownFallbackTime done");
    LE_INFO("SetNadClearDownFallbackTime as 720 min done");
    res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
    LE_TEST_OK(res == LE_OK && ccftTimeOrg == 12*60, "Test taf_ecall_GetNadClearDownFallbackTime done");
    LE_INFO("GetNadClearDownFallbackTime %d done", ccftTimeOrg);

    res = taf_ecall_SetNadClearDownFallbackTime(8*60);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_SetNadClearDownFallbackTime done");
    LE_INFO("SetNadClearDownFallbackTime as 720 min done");
    res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetNadClearDownFallbackTime done");
    LE_INFO("GetNadClearDownFallbackTime %d done", ccftTimeOrg);

    res = taf_ecall_SetNadClearDownFallbackTime(1);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test taf_ecall_SetNadClearDownFallbackTime done");
    LE_INFO("SetNadClearDownFallbackTime as 1 min done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
        LE_TEST_OK(res == LE_OK && ccftTimeOrg == 1, "Test taf_ecall_GetNadClearDownFallbackTime done");
        LE_INFO("GetNadClearDownFallbackTime as %d done", ccftTimeOrg);
    }
    LE_INFO("Set and get NadClearDownFallbackTime completed");
}

static void Test_ECall_GetHlapTimerState()
{
    le_result_t res = LE_FAULT;
    taf_ecall_HlapTimerType_t hlapTimerType = TAF_ECALL_TIMER_TYPE_UNKNOWN;
    taf_ecall_HlapTimerStatus_t timerStatus;
    uint16_t elapsedTime;
    res = taf_ecall_GetHlapTimerState(hlapTimerType, &timerStatus, &elapsedTime);
    LE_TEST_OK(res == LE_FAULT, "Test taf_ecall_GetHlapTimerState done");

    hlapTimerType = TAF_ECALL_TIMER_TYPE_T2;
    res = taf_ecall_GetHlapTimerState(hlapTimerType, &timerStatus, &elapsedTime);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetHlapTimerState done");
    LE_INFO("Get eCall hlap timer T2 status as: %d, elapsedTime as: %d\n", timerStatus, elapsedTime);

    hlapTimerType = TAF_ECALL_TIMER_TYPE_T9;
    res = taf_ecall_GetHlapTimerState(hlapTimerType, &timerStatus, &elapsedTime);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetHlapTimerState done");
    LE_INFO("Get eCall hlap timer T9 status as: %d, elapsedTime as: %d\n", timerStatus, elapsedTime);

    hlapTimerType = TAF_ECALL_TIMER_TYPE_T10;
    res = taf_ecall_GetHlapTimerState(hlapTimerType, &timerStatus, &elapsedTime);
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_GetHlapTimerState done");
    LE_INFO("Get eCall hlap timer T10 status as: %d, elapsedTime as: %d\n", timerStatus, elapsedTime);
    LE_INFO("GetHlapTimerState completed");
}

static void Test_ECall_DialRedial() {
    le_result_t res = LE_FAULT;

    uint16_t dialIntervalErr[TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH] = {60, 60, 60};
    res = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(dialIntervalErr, 3);
    LE_TEST_OK(res == LE_FAULT, "SetInitialDialIntervalBetweenDialAttempts - LE_OK");
    uint16_t dialInterval[TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH] = {5, 60, 60};
    res = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(dialInterval, 3);
    LE_TEST_OK(res == LE_OK, "SetInitialDialIntervalBetweenDialAttempts - LE_OK");
    res = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(dialInterval, 5);
    LE_TEST_OK(res == LE_FAULT, "SetInitialDialIntervalBetweenDialAttempts - LE_OK");
    res = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(dialInterval, 3);
    LE_TEST_OK(res == LE_OK, "SetInitialDialIntervalBetweenDialAttempts - LE_OK");
    res = taf_ecall_SetInitialDialIntervalBetweenDialAttempts(NULL, 0);
    LE_TEST_OK(res == LE_OVERFLOW, "SetInitialDialIntervalBetweenDialAttempts - LE_OK");

    uint8_t dialAttempts = 11;
    res = taf_ecall_SetInitialDialAttempts(dialAttempts);
    LE_TEST_OK(res == LE_OVERFLOW, "SetInitialDialAttempts - LE_OK");
    dialAttempts = 2;
    res = taf_ecall_SetInitialDialAttempts(dialAttempts);
    LE_TEST_OK(res == LE_OK, "SetInitialDialAttempts - LE_OK");

    LE_INFO("Set redial attempts and interval completed");
}

static void* Test_ECall_ExportMsd
(
    taf_ecall_CallRef_t    ecallRef
)
{
    le_result_t res = taf_ecall_ExportMsd(ecallRef, msdRawData, (size_t*)&msdLength);
    LE_TEST_OK(res == LE_OK || res == LE_NOT_FOUND, "Test_ECall_ExportMsd done");
    LE_TEST_INFO("Test_ECall_ExportMsd done (res: %d)", (int) res);
    return NULL;
}

#if defined(LE_CONFIG_ENABLE_PRIVATE_ECALL)
static void Test_ECall_StartPrivate() {
    le_clk_Time_t timeToWait = {10, 0};
    //Waits the ecall inactivity process completed
    le_result_t res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
    if (res == LE_OK)
    {
        LE_INFO("IMS service is available");
        taf_ecall_CallRef_t eCallRef = NULL;
        le_result_t res = LE_FAULT;
        const char* contentType = "application/EmergencyCallData.eCall.MSD";
        const char* acceptInfo = "";

        eCallRef= taf_ecall_Create();

        res = taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);
        LE_TEST_OK(res == LE_OK, "Test taf_ecall_ImportMsd done");
        Test_ECall_ExportMsd(eCallRef);

        res = taf_ecall_StartPrivate(eCallRef, TEST_PSAP_NUMBER, contentType, acceptInfo);
        LE_TEST_OK(res == LE_OK, "Test_ECall_StartPrivate start");
        if (res != LE_OK)
        {
            LE_ERROR("taf_ecall_StartPrivate start failed");
            taf_ecall_Delete(eCallRef);
            eCallRef = NULL;
            return;
        }


        le_clk_Time_t timeToWait = {5, 0};
        //Waits the TAF_ECALL_STATE_ACTIVE event
        res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
        if (res == LE_OK)
        {
            res = taf_ecall_StartTest(eCallRef);
            LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartTest is busy");
            res = taf_ecall_StartManual(eCallRef);
            LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartManual is busy");
            res = taf_ecall_StartAutomatic(eCallRef);
            LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartAutomatic is busy");


            taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
            LE_INFO("Test taf_ecall_StartPrivate callState = %d", (int) retrievedState);

            taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
            LE_INFO("Test_ECall_StartPrivate callType = %d", (int) type);
        }

        res = taf_ecall_End(eCallRef);
        if(res == LE_OK) {
            le_sem_Wait(testSemaphoreRef);
        }

        taf_ecall_Delete(eCallRef);
        eCallRef = NULL;
    }
}
#endif

static void Test_ECall_StartTest() {
    taf_ecall_CallRef_t eCallRef = NULL;
    le_result_t res = LE_FAULT;
    const char* contentType = "application/EmergencyCallData.eCall.MSD";
    const char* acceptInfo = "";
    uint16_t deregTimeOrg = 0;
    uint16_t minNwRegTime = 0;
    uint16_t ccftTimeOrg = 0;
    taf_ecall_State_t retrievedState = TAF_ECALL_STATE_UNKNOWN;
    bool isInProgress = false;

    eCallRef= taf_ecall_Create();

    taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);
    Test_ECall_ExportMsd(eCallRef);

    Test_ECall_GetHlapTimerState();

    res = taf_ecall_StartTest(eCallRef);
    LE_TEST_OK(res == LE_OK, "Test_ECall_StartTest start");
    if (res != LE_OK)
    {
        LE_ERROR("taf_ecall_StartTest failed");
        taf_ecall_Delete(eCallRef);
        eCallRef = NULL;
        return;
    }

    res = taf_ecall_IsInProgress(eCallRef, &isInProgress);
    LE_TEST_OK(res == LE_OK, "getIsInProgress  - LE_OK");
    LE_INFO("Test IsInProgress = %d", (int) isInProgress);

    le_clk_Time_t timeToWait = {10, 0};
    //Waits the TAF_ECALL_STATE_ACTIVE event
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
    if (res == LE_OK)
    {
        res = taf_ecall_StartManual(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartManual is busy");
        res = taf_ecall_StartAutomatic(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartAutomatic is busy");
        res = taf_ecall_StartPrivate(eCallRef, TEST_PSAP_NUMBER, contentType, acceptInfo);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartPrivate is busy");

        ///Waits the TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS event
        res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
        if (res == LE_OK)
        {
            LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/SUCESS received");
        }

        retrievedState = taf_ecall_GetState(eCallRef);
        LE_INFO("Test_ECall_StartTest callState = %d", (int) retrievedState);

        taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
        LE_INFO("Test_ECall_StartTest callType = %d", (int) type);

        Test_ECall_GetHlapTimerState();

        res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadDeregTime done");
        LE_INFO("GetNadDeregTime as %d done", deregTimeOrg);
        res = taf_ecall_SetNadDeregistrationTime(2);
        LE_TEST_OK(res == LE_BUSY, "Test_ecall_SetNadDeregTime done");
        LE_INFO("SetNadDeregistrationTime done");

        res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadMinNetworkRegistrationTime done");
        LE_INFO("GetNadMinNetworkRegistrationTime %d done", minNwRegTime);
        res = taf_ecall_SetNadMinNetworkRegistrationTime(1);
        LE_TEST_OK(res == LE_BUSY, "Test_ecall_SetNadMinNetworkRegistrationTime done");
        LE_INFO("SetNadMinNetworkRegistrationTime done");

        res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
        LE_TEST_OK(res == LE_OK && ccftTimeOrg == 1, "Test_ecall_GetNadClearDownFallbackTime done");
        LE_INFO("GetNadClearDownFallbackTime %d done", ccftTimeOrg);
        res = taf_ecall_SetNadClearDownFallbackTime(1);
        LE_TEST_OK(res == LE_BUSY, "Test_ecall_SetNadClearDownFallbackTime done");
        LE_INFO("SetNadClearDownFallbackTime done");
    }

    res = taf_ecall_End(eCallRef);
    if(res == LE_OK) {
        le_sem_Wait(testSemaphoreRef);
    }

    res = taf_ecall_IsInProgress(eCallRef, &isInProgress);
    LE_TEST_OK(res == LE_OK, "getIsInProgress  - LE_OK");
    LE_INFO("Test IsInProgress = %d", (int) isInProgress);

    retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartTest callState = %d", (int) retrievedState);

    Test_ECall_GetHlapTimerState();

    //Waits the TAF_ECALL_STATE_T9_EXPIRED event
    res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    if (res != LE_OK) {
        minNwRegTime = 1;
    }
    LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED waiting");
    le_clk_Time_t timeToWaitT9 = {minNwRegTime * 60, 0};
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWaitT9);
    if (res == LE_OK)
    {
        LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED received");
    }

    taf_ecall_Delete(eCallRef);
    eCallRef = NULL;
}

static void Test_ECall_TerminateRegistration()
{
    le_result_t res = taf_ecall_TerminateRegistration();
    LE_TEST_OK(res == LE_OK, "Test taf_ecall_TerminateRegistration done");
    LE_INFO("TerminateECallRegistration completed");
}

static void Test_ECall_StartAutomatic() {
    taf_ecall_CallRef_t eCallRef = NULL;
    le_result_t res = LE_FAULT;
    const char* contentType = "application/EmergencyCallData.eCall.MSD";
    const char* acceptInfo = "";
    uint16_t deregTimeOrg = 0;
    uint16_t minNwRegTime = 0;
    uint16_t ccftTimeOrg = 0;
    taf_ecall_State_t retrievedState = TAF_ECALL_STATE_UNKNOWN;

    res = taf_ecall_ForceOnlyMode(DEFAULT_PHONE_ID);
    if (res != LE_OK)
    {
        LE_INFO("taf_ecall_ForceOnlyMode failed");
        res = taf_ecall_ExitOnlyMode(DEFAULT_PHONE_ID);
        LE_TEST_OK(res == LE_OK, "taf_ecall_ExitOnlyMode done");
        return;
    }

    eCallRef = taf_ecall_Create();

    Test_ECall_GetHlapTimerState();

    res = taf_ecall_SetMsdPosition(eCallRef, true, +118422000, -421902360, 0);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPosition done");
    LE_INFO("Set msd position completed");

    res = taf_ecall_SetMsdPositionN1(eCallRef, 511, 511);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN1 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -512, -512);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPositionN2 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPassengersCount(eCallRef, 2);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPassengersCount done");
    LE_INFO("Set number of passengers completed");

    res = taf_ecall_StartAutomatic(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test_ECall_StartAutomatic start");
    if (res != LE_OK)
    {
        LE_ERROR("taf_ecall_StartAutomatic failed");
        taf_ecall_Delete(eCallRef);
        eCallRef = NULL;
        return;
    }

    le_clk_Time_t timeToWait = {10, 0};
    //Waits the TAF_ECALL_STATE_ACTIVE event
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
    if (res == LE_OK)
    {
        res = taf_ecall_StartTest(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartTest is busy");
        res = taf_ecall_StartManual(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartManual is busy");
        res = taf_ecall_StartPrivate(eCallRef, TEST_PSAP_NUMBER, contentType, acceptInfo);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartPrivate is busy");

        //Waits the TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS event
        res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
        if (res == LE_OK)
        {
            LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/SUCESS received");
        }

        retrievedState = taf_ecall_GetState(eCallRef);
        LE_INFO("Test_ECall_StartAutomatic callState = %d", (int) retrievedState);

        taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
        LE_INFO("Test_ECall_StartAutomatic callType = %d", (int) type);

        Test_ECall_GetHlapTimerState();

        res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadDeregTime done");
        LE_INFO("GetNadDeregTime as %d done", deregTimeOrg);
        res = taf_ecall_SetNadDeregistrationTime(2);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK || res == LE_FAULT, "Test_ecall_SetNadDeregTime done");
        LE_INFO("SetNadDeregistrationTime done(res: %d)", (int) res);

        res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadMinNetworkRegistrationTime done");
        LE_INFO("GetNadMinNetworkRegistrationTime %d done", minNwRegTime);
        res = taf_ecall_SetNadMinNetworkRegistrationTime(1);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK || res == LE_FAULT, "Test_ecall_SetNadMinNetworkRegistrationTime done");
        LE_INFO("SetNadMinNetworkRegistrationTime done(res: %d)", (int) res);

        res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
        LE_TEST_OK(res == LE_OK && ccftTimeOrg == 1, "Test_ecall_GetNadClearDownFallbackTime done");
        LE_INFO("GetNadClearDownFallbackTime %d done", ccftTimeOrg);
        res = taf_ecall_SetNadClearDownFallbackTime(1);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK, "Test_ecall_SetNadClearDownFallbackTime done");
        LE_INFO("SetNadClearDownFallbackTime done(res: %d)", (int) res);
    }

    res = taf_ecall_End(eCallRef);
    if(res == LE_OK) {
        le_sem_Wait(testSemaphoreRef);
    }

    retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic callState = %d", (int) retrievedState);

    taf_ecall_TerminationReason_t endReason = taf_ecall_GetTerminationReason(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic end reason = %d", (int) endReason);

    //Waits the TAF_ECALL_STATE_T9_EXPIRED event
    res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    if(res != LE_OK) {
        minNwRegTime = 1;
    }
    LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED waiting");
    le_clk_Time_t timeToWaitT9 = {minNwRegTime * 60, 0};
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWaitT9);
    if (res == LE_OK)
    {
        LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED received");
        taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
        taf_ecall_HlapTimerStatus_t timerStatus;
        uint16_t elapsedTime;
        if ((LE_OK == taf_ecall_GetConfiguredOperationMode(DEFAULT_PHONE_ID, &opMode)) && 
            (opMode == TAF_ECALL_MODE_ECALL) &&
            (LE_OK ==taf_ecall_GetHlapTimerState(TAF_ECALL_TIMER_TYPE_T10, &timerStatus, &elapsedTime)) &&
            (timerStatus == TAF_ECALL_TIMER_STATUS_ACTIVE))
        {
            Test_ECall_TerminateRegistration();
            waitT10StopForAuto = true;
        }
        le_clk_Time_t timeToWaitT10Stop = {5, 0};
        res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWaitT10Stop);
        if (res == LE_OK)
        {
            LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T10_STOP received");
        }
    }

    res = taf_ecall_ExitOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(res == LE_OK, "taf_ecall_ExitOnlyMode done");

    taf_ecall_Delete(eCallRef);
    eCallRef = NULL;
}

static void Test_ECall_StartManual() {
    taf_ecall_CallRef_t eCallRef = NULL;
    le_result_t res = LE_FAULT;
    const char* contentType = "application/EmergencyCallData.eCall.MSD";
    const char* acceptInfo = "";
    uint16_t deregTimeOrg = 0;
    uint16_t minNwRegTime = 0;
    uint16_t ccftTimeOrg = 0;
    taf_ecall_State_t retrievedState = TAF_ECALL_STATE_UNKNOWN;

    eCallRef = taf_ecall_Create();
    res = taf_ecall_StartManual(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "Test_ECall_StartManual start");
    if (res != LE_OK)
    {
        LE_ERROR("taf_ecall_StartManual failed");
        taf_ecall_Delete(eCallRef);
        eCallRef = NULL;
        return;
    }

    le_clk_Time_t timeToWait = {10, 0};
    //Waits the TAF_ECALL_STATE_ACTIVE event
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
    if (res == LE_OK)
    {
        res = taf_ecall_StartTest(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartTest is busy");
        res = taf_ecall_StartAutomatic(eCallRef);
        LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartAutomatic is busy");
        res = taf_ecall_StartPrivate(eCallRef, TEST_PSAP_NUMBER, contentType, acceptInfo);
       LE_TEST_OK(res == LE_BUSY, "Test_ECall_StartPrivate is busy");

        ///Waits the TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS event
        res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWait);
        if (res == LE_OK)
        {
            LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED/SUCESS received");
        }

        retrievedState = taf_ecall_GetState(eCallRef);
        LE_INFO("Test_ECall_StartManual callState = %d", (int) retrievedState);

        taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
        LE_INFO("Test_ECall_StartManual callType = %d", (int) type);

        Test_ECall_GetHlapTimerState();

        res = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadDeregTime done");
        LE_INFO("GetNadDeregTime as %d done", deregTimeOrg);
        res = taf_ecall_SetNadDeregistrationTime(2);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK || res == LE_FAULT, "Test_ecall_SetNadDeregTime done");
        LE_INFO("SetNadDeregistrationTime done(res: %d)", (int) res);

        res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
        LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadMinNetworkRegistrationTime done");
        LE_INFO("GetNadMinNetworkRegistrationTime %d done", minNwRegTime);
        res = taf_ecall_SetNadMinNetworkRegistrationTime(1);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK || res == LE_FAULT , "Test_ecall_SetNadMinNetworkRegistrationTime done");
        LE_INFO("SetNadMinNetworkRegistrationTime done(res: %d)", (int) res);

        res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
        LE_TEST_OK(res == LE_OK && ccftTimeOrg == 1, "Test_ecall_GetNadClearDownFallbackTime done");
        LE_INFO("GetNadClearDownFallbackTime %d done", ccftTimeOrg);
        res = taf_ecall_SetNadClearDownFallbackTime(1);
        LE_TEST_OK(res == LE_BUSY || res == LE_OK, "Test_ecall_SetNadClearDownFallbackTime done");
        LE_INFO("SetNadClearDownFallbackTime done (res: %d)", (int) res);
    }

    res = taf_ecall_End(eCallRef);
    if(res == LE_OK) {
        le_sem_Wait(testSemaphoreRef);
    }

    taf_ecall_TerminationReason_t endReason = taf_ecall_GetTerminationReason(eCallRef);
    LE_INFO("Test taf_ecall_StartManual endReason = %d", (int) endReason);

    retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartManual callState = %d", (int) retrievedState);

    //Waits the TAF_ECALL_STATE_T9_EXPIRED event
    res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    if(res != LE_OK) {
        minNwRegTime = 1;
    }
    LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED waiting");
    le_clk_Time_t timeToWaitT9 = {minNwRegTime * 60, 0};
    res = le_sem_WaitWithTimeOut(testSemaphoreRef, timeToWaitT9);
    if (res == LE_OK)
    {
        LE_INFO("Test_ECall_StartTest TAF_ECALL_STATE_T9_EXPIRED received");
        Test_ECall_GetHlapTimerState();
    }

    taf_ecall_Delete(eCallRef);
    eCallRef = NULL;
}

static void tafECallStateHandler( taf_ecall_CallRef_t eCallReference,
        taf_ecall_State_t state, void* cntxtPtr)
{
    LE_INFO("Ecall state change event state = %d", state );
    LE_INFO("Ecall state change event reference = %p", eCallReference );

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
            le_sem_Post(testSemaphoreRef);
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
        case TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY:
        {
            LE_INFO("TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY");
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
            le_sem_Post(testSemaphoreRef);
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED");
            le_sem_Post(testSemaphoreRef);
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
        case TAF_ECALL_STATE_MSD_UPDATE_REQ:
        {
            printf("TAF_ECALL_STATE_MSD_UPDATE_REQ");
            taf_ecall_ImportMsd(eCallReference, msdRawData, msdLength);
            taf_ecall_SendMsd(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_ENDED:
        {
            LE_INFO("TAF_ECALL_STATE_ENDED");
            if (eCallReference != NULL)
            {
                taf_ecall_TerminationReason_t endReason = taf_ecall_GetTerminationReason(eCallReference);
                LE_INFO("TAF_ECALL_STATE_ENDED endReason = %d", (int) endReason);
            }
            le_sem_Post(testSemaphoreRef);
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
            if (eCallReference != NULL)
            {
                taf_ecall_TerminationReason_t endReason = taf_ecall_GetTerminationReason(eCallReference);
                LE_INFO("TAF_ECALL_STATE_ENDED_OF_REDIAL_PERIOD endReason = %d", (int) endReason);
            }
            le_sem_Post(testSemaphoreRef);
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
            le_sem_Post(testSemaphoreRef);
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
        case TAF_ECALL_STATE_T2_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T5_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T6_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T7_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T9_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_STARTED");
            if (waitT10StopForAuto == true)
            {
                waitT10StopForAuto = false;
                le_sem_Post(testSemaphoreRef);
            }
            break;
        }
        case TAF_ECALL_STATE_T10_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T2_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T5_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T6_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T7_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T9_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T10_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_INCOMING:
        {
            LE_INFO("TAF_ECALL_STATE_INCOMING");
            break;
        }
        case TAF_ECALL_STATE_T9_RESUMED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_RESUMED");
            break;
        }
        case TAF_ECALL_STATE_T10_RESUMED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_RESUMED");
            break;
        }
        default:
        {
            LE_INFO("Unknown state");
            break;
        }
    }
}

static void* Test_taf_ecall_AddHandler(void* context) {

    taf_ecall_ConnectService();

    handlerRef = taf_ecall_AddStateChangeHandler(tafECallStateHandler, NULL);
    LE_TEST_OK(handlerRef != NULL, "Test_taf_ecall_AddHandler done");
    LE_INFO("Add State change handler complete. The handlerRef = %p", handlerRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_ecall_RemoveHandler(void* param1, void* param2) {

    taf_ecall_TryConnectService();

    taf_ecall_RemoveStateChangeHandler(handlerRef);

    le_sem_Post(testSemaphoreRef);
}

static void tafECallImsServStatusHandler(taf_radio_ImsRef_t imsRef,
        taf_radio_ImsIndBitMask_t bitmask,
        uint8_t phoneId,
        void* contextPtr
)
{
    if (bitmask & TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO)
    {
        taf_radio_ImsSvcStatus_t svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
        le_result_t result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &svcStatus);
        if ((result == LE_OK) && (TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE == svcStatus) && (phoneId == DEFAULT_PHONE_ID))
        {
            le_sem_Post(testSemaphoreRef);
        }
    }
}

static void* Test_taf_ecall_ImsServStatus_AddHandler(void* context) {

    taf_radio_ConnectService();

    imsServStatusHandlerRef = taf_radio_AddImsStatusChangeHandler((taf_radio_ImsStatusChangeHandlerFunc_t)tafECallImsServStatusHandler, NULL);
    LE_TEST_OK(imsServStatusHandlerRef != NULL, "taf_radio_AddImsStatusChangeHandler is null");
    LE_INFO("Add ims service status event handler complete. The handlerRef = %p", imsServStatusHandlerRef);

    le_sem_Post(testSemaphoreRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_ecall_ImsServStatus_RemoveHandler(void* param1, void* param2) {

    taf_radio_TryConnectService();

    taf_radio_RemoveImsStatusChangeHandler(imsServStatusHandlerRef);

    le_sem_Post(testSemaphoreRef);
}

COMPONENT_INIT
{

    Test_ECall_MSD_Information();

    Test_ECall_PsapNumber();

    Test_ECall_HlapTimer();

    Test_ECall_DialRedial();

    Test_ECall_OperationMode();

    testSemaphoreRef = le_sem_Create("ECallSem", 0);

#if defined(LE_CONFIG_ENABLE_PRIVATE_ECALL)
    imsServStatusThreadRef = le_thread_Create("ImsServStatusThread", Test_taf_ecall_ImsServStatus_AddHandler, NULL);
    le_thread_Start(imsServStatusThreadRef);

    le_sem_Wait(testSemaphoreRef);

    threadRef = le_thread_Create("EctThread", Test_taf_ecall_AddHandler, NULL);
    le_thread_Start(threadRef);

    Test_ECall_StartPrivate();

    le_event_QueueFunctionToThread(imsServStatusThreadRef, Test_taf_ecall_ImsServStatus_RemoveHandler, NULL, NULL);

    le_sem_Wait(testSemaphoreRef);

    le_result_t result = le_thread_Cancel(imsServStatusThreadRef);
    LE_TEST_OK(result == LE_OK, "ImsServStatusThread cancelled");

    //Disconnect the telaf radio service
    taf_radio_DisconnectService();

#endif

    Test_ECall_StartTest();

    Test_ECall_StartManual();

    Test_ECall_StartAutomatic();

    le_event_QueueFunctionToThread(threadRef, Test_taf_ecall_RemoveHandler, NULL, NULL);

    le_sem_Wait(testSemaphoreRef);

    result = le_thread_Cancel(threadRef);
    LE_TEST_OK(result == LE_OK, "EctThread cancelled");

    //Disconnect the telaf eCall service
    taf_ecall_DisconnectService();

    le_sem_Delete(testSemaphoreRef);

    LE_INFO("ECall API Unit test execution success");

    exit(EXIT_SUCCESS);
}

