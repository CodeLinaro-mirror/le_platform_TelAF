/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "ECallApisTest.h"

static taf_ecall_CallRef_t ECallRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate ECall Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void ecallRetTest_RunApis
(
   void
)
{
    le_result_t result;
    LE_TEST_INFO("ecallRetTest_RunApis");

    //1.taf_ecall_ForceOnlyMode - LE_BAD_PARAMETER scenario
    result = taf_ecall_ForceOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_ForceOnlyMode***-LE_BAD_PARAMETER ");

    //2.taf_ecall_ForcePersistentOnlyMode - LE_BAD_PARAMETER scenario
    result = taf_ecall_ForcePersistentOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_ForcePersistentOnlyMode***-LE_BAD_PARAMETER ");

    //3.taf_ecall_ExitOnlyMode - LE_BAD_PARAMETER scenario
    result = taf_ecall_ExitOnlyMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_ExitOnlyMode***-LE_BAD_PARAMETER ");

    //4.taf_ecall_GetConfiguredOperationMode - LE_BAD_PARAMETER scenario
    result = taf_ecall_GetConfiguredOperationMode(DEFAULT_PHONE_ID,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_GetConfiguredOperationMode***-LE_BAD_PARAMETER ");

    //5.taf_ecall_SetMsdVersion - LE_FAULT scenario
    result = taf_ecall_SetMsdVersion(INVALID);
    LE_TEST_OK(result == LE_FAULT , "***taf_ecall_SetMsdVersion***-LE_FAULT ");

    //6.taf_ecall_GetMsdVersion - LE_BAD_PARAMETER scenario
    result = taf_ecall_GetMsdVersion(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_SetMsdVersion***-LE_BAD_PARAMETER ");

    //7.taf_ecall_GetVehicleType - LE_BAD_PARAMETER scenario
    result = taf_ecall_GetVehicleType(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_GetVehicleType***-LE_BAD_PARAMETER ");

    //8.taf_ecall_GetVIN - LE_BAD_PARAMETER scenario
    result = taf_ecall_GetVIN(NULL,5);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ecall_GetVIN***-LE_BAD_PARAMETER ");

    //9.taf_ecall_GetPropulsionType - LE_FAULT scenario
    result = taf_ecall_GetPropulsionType(NULL);
    LE_TEST_OK(result == LE_FAULT , "***taf_ecall_GetPropulsionType***-LE_FAULT ");

    //10.taf_ecall_SetMsdTxMode - LE_UNSUPPORTED scenario
    result = taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PULL);
    LE_TEST_OK(result == LE_UNSUPPORTED , "***taf_ecall_SetMsdTxMode***-LE_UNSUPPORTED ");

    //11.taf_ecall_GetMsdTxMode - LE_UNSUPPORTED scenario
    result = taf_ecall_GetMsdTxMode(NULL);
    LE_TEST_OK(result == LE_UNSUPPORTED , "***taf_ecall_GetMsdTxMode***-LE_UNSUPPORTED ");

    //12.taf_ecall_SetMsdPositionN1 - LE_FAULT scenario
    ECallRef = taf_ecall_Create();
    result = taf_ecall_SetMsdPositionN1(ECallRef, 512, 512);
    LE_TEST_OK(result == LE_FAULT , "***taf_ecall_SetMsdPositionN1***-LE_FAULT ");

    //13.taf_ecall_SetMsdPositionN2 - LE_FAULT scenario
    result = taf_ecall_SetMsdPositionN2(ECallRef, 512, 512);
    LE_TEST_OK(result == LE_FAULT , "***taf_ecall_SetMsdPositionN2***-LE_FAULT ");

    //14.taf_ecall_ExportMsd - LE_FAULT scenario
    result = taf_ecall_ExportMsd(ECallRef,NULL,0);
    LE_TEST_OK(result == LE_FAULT , "***taf_ecall_ExportMsd***-LE_FAULT ");

    //15.taf_ecall_ExportMsd - LE_NOT_FOUND scenario
    uint8_t msdRawDataExport[TAF_ECALL_MAX_MSD_LENGTH];
    size_t msdLengthExport = 0;
    result = taf_ecall_ExportMsd(ECallRef, msdRawDataExport,&msdLengthExport);
    LE_TEST_OK(result == LE_NOT_FOUND,"***taf_ecall_ExportMsd***-LE_NOT_FOUND ");

    //16.taf_ecall_GetPsapNumber - LE_BAD_PARAMETER scenario
    result = taf_ecall_GetPsapNumber(NULL,TAF_SIM_PHONE_NUM_MAX_LEN);
    LE_TEST_OK(result == LE_BAD_PARAMETER,"***taf_ecall_GetPsapNumber***-LE_BAD_PARAMETER ");

    //17.taf_ecall_GetNadDeregistrationTime - LE_FAULT scenario
    result = taf_ecall_GetNadDeregistrationTime(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_ecall_GetNadDeregistrationTime***-LE_FAULT ");

    //18.taf_ecall_GetNadClearDownFallbackTime - LE_FAULT scenario
    result = taf_ecall_GetNadClearDownFallbackTime(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_ecall_GetNadClearDownFallbackTime***-LE_FAULT ");

    //19.taf_ecall_GetNadMinNetworkRegistrationTime - LE_FAULT scenario
    result = taf_ecall_GetNadMinNetworkRegistrationTime(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_ecall_GetNadMinNetworkRegistrationTime***-LE_FAULT ");

}
