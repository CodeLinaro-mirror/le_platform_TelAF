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
static bool testToBeCounted = true;
static uint8_t oadDataFirst[8] = {8, 41, 68, 6, 128, 20, 8, 9};
static uint8_t oadDataLengthFirst = 8;
static uint8_t oadDataSec[6] = {8, 41, 68, 6, 128, 20};
static uint8_t oadDataLengthSec = 6;

static void Test_ecall_TerminateRegistration()
{
    le_result_t result = taf_ecall_TerminateRegistration();
    LE_TEST_OK(testToBeCounted, "Test_ecall_TerminateRegistration done");
    LE_INFO("TerminateECallRegistration completed (%d)!!!\n", (int) result);
}

static void* Test_ECall_ExportMsd
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_ExportMsd(ecallRef, msdRawData, (size_t*)&msdLength);
    LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND, "Test_ECall_ExportMsd done");
    LE_TEST_INFO("Test_ECall_ExportMsd done (res: %d)", (int) result);
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
    LE_TEST_OK(testToBeCounted, "taf_ecall_ForceOnlyMode done");
    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    le_result_t result = taf_ecall_GetConfiguredOperationMode(1, &opMode);
    LE_INFO("Get Operating mode = %d and res: %d", opMode, (int) result);
    LE_TEST_OK(testToBeCounted, "Test_ECall_OperatingMode done");
    LE_INFO("Operating mode = %d", opMode);
    result = taf_ecall_ForcePersistentOnlyMode(1);
    LE_TEST_OK(testToBeCounted, "taf_ecall_ForcePersistentOnlyMode done");
    result = taf_ecall_GetConfiguredOperationMode(1, &opMode);
    LE_TEST_OK(testToBeCounted, "taf_ecall_GetConfiguredOperationMode done");
    LE_INFO("Operating mode = %d and res: %d", opMode, (int) result);
    result = taf_ecall_ExitOnlyMode(1);
    LE_TEST_OK(testToBeCounted, "taf_ecall_ExitOnlyMode done");
    LE_INFO("Operating mode = %d and res: %d", opMode, (int) result);
    result = taf_ecall_GetConfiguredOperationMode(1, &opMode);
    LE_TEST_OK(testToBeCounted, "taf_ecall_GetConfiguredOperationMode done");
    LE_INFO("Operating mode = %d and res: %d", opMode, (int) result);
}

static void Test_MSD_Information()
{
    taf_ecall_CallRef_t   eCallRef = 0x00;
    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    char vin[TAF_ECALL_MAX_VIN_BYTES];
    le_result_t res = LE_FAULT;
    taf_ecall_MsdTransmissionMode_t mode = TAF_ECALL_MSD_TX_MODE_PULL;

    uint32_t msdVersion = 0;
    res = taf_ecall_SetMsdVersion(4);
    LE_TEST_OK(res == LE_FAULT, "Test_MSD_Information done");
    res = taf_ecall_SetMsdVersion(2);
    LE_TEST_OK(res == LE_OK, "Test_MSD_Information done");
    LE_TEST_OK(taf_ecall_GetMsdVersion(&msdVersion) == LE_OK, "taf_ecall_GetMsdVersion done");
    LE_TEST_OK(msdVersion == 2, "taf_ecall_GetMsdVersion done");
    res = taf_ecall_SetMsdVersion(3);
    LE_TEST_OK(res == LE_OK, "Test_MSD_Information done");
    LE_TEST_OK(taf_ecall_GetMsdVersion(&msdVersion) == LE_OK, "taf_ecall_GetMsdVersion done");
    LE_TEST_OK(msdVersion == 3, "taf_ecall_GetMsdVersion done");
    LE_INFO("Set and Get MSD version completed");

    res = taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PUSH);
    LE_TEST_OK(res == LE_OK || res == LE_UNSUPPORTED, "taf_ecall_SetMsdTxMode done");
    res = taf_ecall_GetMsdTxMode(&mode);
    LE_TEST_OK(res == LE_OK || res == LE_UNSUPPORTED, "taf_ecall_GetMsdTxMode done");
    LE_TEST_OK(mode == TAF_ECALL_TX_MODE_PUSH || res == LE_UNSUPPORTED, "Test_MSD_Information done");
    LE_INFO("Set and Get MSD transmission mode completed");

    res = taf_ecall_SetVIN("ECALLEXAMPLE");//invalid input, result will be failed.
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("EOALLEXAMPLE02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLIXAMPLE02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLEXAMPLQ02013");
    LE_TEST_OK(res == LE_FAULT || res == LE_BAD_PARAMETER, "taf_ecall_SetVIN done");
    res = taf_ecall_SetVIN("ECALLEXAMPLE02013");
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetVIN done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetVIN(vin, TAF_ECALL_MAX_VIN_BYTES);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_GetVIN done");
        if (res == LE_OK)
        {
            LE_TEST_OK(strcmp(vin, "ECALLEXAMPLE02013") == 0, "Test_MSD_Information done");
            LE_INFO("Set and Get Vehicle identification number completed");
        }
    }
    res = taf_ecall_SetVehicleType(vehType);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetVehicleType done");
    if (res == LE_OK)
    {
        vehType = TAF_ECALL_BUSES_AND_COACHES_CLASS_M2;
        res = taf_ecall_GetVehicleType(&vehType);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_GetVehicleType done");
        if (res == LE_OK)
        {
            LE_TEST_OK(( TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1 == vehType ), "taf_ecall_SetVehicleType done");
        }
    }
    vehType = 30; //invalid vehicle type, result will be failed.
    LE_TEST_OK(taf_ecall_SetVehicleType(vehType) == LE_FAULT, "taf_ecall_SetVehicleType done");
    LE_INFO("Set and Get Vehicle type completed");

    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;
    res = taf_ecall_SetPropulsionType(propulsionStorage);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetPropulsionType done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetPropulsionType(&propulsionStorage);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_GetPropulsionType done");
        if (res == LE_OK)
        {
            LE_TEST_OK( TAF_ECALL_PROP_TYPE_GASOLINE_TANK == propulsionStorage, "taf_ecall_SetPropulsionType done");
        }
    }
    propulsionStorage = 1000;
    res = taf_ecall_SetPropulsionType(propulsionStorage);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetPropulsionType done");
    if (res == LE_OK)
    {
        res = taf_ecall_GetPropulsionType(&propulsionStorage);
        LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_GetPropulsionType done");
    }
    LE_INFO("Set and Get Vehicle type completed %d", propulsionStorage);

    LE_INFO("Set and Get propulsion type completed");

    LE_TEST_OK((eCallRef= taf_ecall_Create()) != NULL, "taf_ecall_Create done");

    res = taf_ecall_SetMsdPosition(eCallRef, true, +118422000, -421902360, 0);
    LE_TEST_OK(res == LE_OK, "taf_ecall_SetMsdPosition done");
    LE_INFO("Set msd position completed");

    res = taf_ecall_SetMsdPositionN1(eCallRef, -520, 520);//Boundary check, result will be failed.
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdPositionN1 done");

    res = taf_ecall_SetMsdPositionN1(eCallRef, 511, 511);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "taf_ecall_SetMsdPositionN1 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -520, 520);//Boundary check, result will be failed.
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdPositionN2 done");

    res = taf_ecall_SetMsdPositionN2(eCallRef, -512, -512);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "taf_ecall_SetMsdPositionN2 done");
    LE_INFO("Set delta  msd position completed");

    res = taf_ecall_SetMsdPassengersCount(eCallRef, 2);
    LE_TEST_OK(res == LE_OK || res == LE_DUPLICATE || res == LE_FAULT, "taf_ecall_SetMsdPassengersCount done");
    LE_INFO("Set number of passengers completed");

    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdAdditionalData done");

    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1.2", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdAdditionalData done");

    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1", oadDataSec, oadDataLengthSec);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdAdditionalData done");

    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_ResetMsdAdditioanllData");

    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, 10);
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdEuroNCAPLocationOfImpact");

    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 90, -45, 10);
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdEuroNCAPIIDeltaV");

    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 120, -251, 10);
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdEuroNCAPIIDeltaV");

    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 120, -201, 251);
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdEuroNCAPIIDeltaV");

    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, TAF_ECALL_LOI_FRONT);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdEuroNCAPLocationOfImpact");

    res = taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 125, -45, 10);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdEuroNCAPIIDeltaV");

    res = taf_ecall_SetMsdEuroNCAPRolloverDetected(eCallRef, 1);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdEuroNCAPRolloverDetected");

    res = taf_ecall_ResetMsdEuroNCAPRolloverDetected(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_ResetMsdEuroNCAPRolloverDetected");

    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_ResetMsdAdditioanllData");

    res = taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, TAF_ECALL_LOI_NONDRIVERSIDE);
    LE_TEST_OK(res == LE_OK || res == LE_FAULT, "taf_ecall_SetMsdEuroNCAPLocationOfImpact");

    taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);
    res = taf_ecall_SetMsdAdditionalData(eCallRef, "8.1.2", oadDataFirst, oadDataLengthFirst);
    LE_TEST_OK(res != LE_OK, "taf_ecall_SetMsdAdditionalData done");

    res = taf_ecall_ResetMsdAdditionalData(eCallRef);
    LE_TEST_OK(res != LE_OK, "taf_ecall_ResetMsdAdditionalData");

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

static void Test_ecall_GetNadClearDownFallbackTime()
{
    uint16_t ccftTimeOrg = 0;
    le_result_t res = taf_ecall_GetNadClearDownFallbackTime(&ccftTimeOrg);
    LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadClearDownFallbackTime done");
    LE_INFO("GetNadClearDownFallbackTime done!!! ccftTime (in minutes): %d\n", ccftTimeOrg);
}

static void Test_ecall_SetNadClearDownFallbackTime()
{
    le_result_t res = taf_ecall_SetNadClearDownFallbackTime(10); // 10 min
    LE_TEST_OK(res == LE_OK, "Test_ecall_SetNadClearDownFallbackTime done");
    LE_INFO("SetNadClearDownFallbackTime as 10 min completed!!!\n");
}

static void Test_ecall_GetNadMinNetworkRegistrationTime()
{
    uint16_t minNwRegTime = 0;
    le_result_t res = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    LE_TEST_OK(res == LE_OK, "Test_ecall_GetNadMinNetworkRegistrationTime done");
    LE_INFO("GetNadMinNetworkRegistrationTime done!!! minNwRegTime (in minutes): %d\n", minNwRegTime);
}

static void Test_ecall_SetNadMinNetworkRegistrationTime()
{
    le_result_t res = taf_ecall_SetNadMinNetworkRegistrationTime(60); // 60 min
    LE_TEST_OK(res == LE_OK, "Test_ecall_SetNadMinNetworkRegistrationTime done");
    LE_INFO("SetNadMinNetworkRegistrationTime as 60 min completed!!!\n");
}

static void Test_ECall_StartAutomatic() {
    taf_ecall_CallRef_t eCallRef = NULL;

    eCallRef = taf_ecall_Create();

    le_result_t res = taf_ecall_StartAutomatic(eCallRef);
    taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic callState = %d", (int) retrievedState);

    taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
    LE_INFO("Test_ECall_StartAutomatic callType = %d", (int) type);

    Test_ECall_ExportMsd(eCallRef);

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

    taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
    LE_INFO("Test_ECall_StartManual callType = %d", (int) type);

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

    taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
    LE_INFO("Test_ECall_StartTest callType = %d", (int) type);

    Test_ECall_ExportMsd(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    taf_ecall_Delete(eCallRef);

    eCallRef = NULL;
}
#if defined(LE_CONFIG_ENABLE_PRIVATE_ECALL)
static void Test_ECall_StartPrivate() {
    taf_ecall_CallRef_t eCallRef = NULL;

    eCallRef= taf_ecall_Create();

    taf_ecall_ImportMsd(eCallRef, msdRawData, msdLength);

    const char* contentType = "application/EmergencyCallData.eCall.MSD";
    const char* acceptInfo = "";

    le_result_t res = taf_ecall_StartPrivate(eCallRef, TEST_PSAP_NUMBER, contentType, acceptInfo);
    taf_ecall_State_t retrievedState = taf_ecall_GetState(eCallRef);
    LE_INFO("Test_ECall_StartPrivate callState = %d", (int) retrievedState);

    taf_ecall_Type_t type = taf_ecall_GetType(eCallRef);
    LE_INFO("Test_ECall_StartPrivate callType = %d", (int) type);

    le_result_t setResult = LE_FAULT;
    setResult = taf_ecall_SetMsdPosition(eCallRef, true, +118422000, -421902360, 0);
    LE_TEST_OK(setResult != LE_OK, "taf_ecall_SetMsdPosition done");
    LE_INFO("Set msd position completed");

    setResult = taf_ecall_SetMsdPositionN1(eCallRef, 511, 511);
    LE_TEST_OK(setResult != LE_OK, "taf_ecall_SetMsdPositionN1 done");
    LE_INFO("Set delta  msd position completed");

    setResult = taf_ecall_SetMsdPositionN2(eCallRef, -512, -512);
    LE_TEST_OK(setResult != LE_OK, "taf_ecall_SetMsdPositionN2 done");
    LE_INFO("Set delta  msd position completed");

    setResult = taf_ecall_SetMsdPassengersCount(eCallRef, 2);
    LE_TEST_OK(setResult != LE_OK, "taf_ecall_SetMsdPassengersCount done");
    LE_INFO("Set number of passengers completed");

    Test_ECall_ExportMsd(eCallRef);

    if(res == LE_OK) {
        le_sem_Wait(TestSemaphoreRef);
    }

    taf_ecall_Delete(eCallRef);

    eCallRef = NULL;
}
#endif
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
    Test_ecall_SetNadClearDownFallbackTime();
    Test_ecall_GetNadClearDownFallbackTime();
    Test_ecall_SetNadMinNetworkRegistrationTime();
    Test_ecall_GetNadMinNetworkRegistrationTime();

    TestSemaphoreRef = le_sem_Create("ECallSem", 0);

    ThreadRef = le_thread_Create("EctThread", Test_taf_ecall_AddHandler, NULL);
    le_thread_Start(ThreadRef);

#if defined(LE_CONFIG_ENABLE_PRIVATE_ECALL)
    Test_ECall_StartPrivate();
#endif

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

