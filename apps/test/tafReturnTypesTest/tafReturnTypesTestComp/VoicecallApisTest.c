/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Voice Call Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void voicecallRetTest_RunApis
(
   void
)
{
    le_result_t res;
    static char  DestinationNum[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES+1];
    static taf_voicecall_CallRef_t TafCallRef;
    taf_voicecall_CallRef_t callref;
    LE_TEST_INFO("voicecallRetTest_RunApis");

    //1.taf_voicecall_Start NULL scenario
    callref = taf_voicecall_Start(DestinationNum, 3);
    LE_TEST_OK(callref == NULL,"taf_voicecall_Start-NULL");

    //2.taf_voicecall_End LE_NOT_FOUND scenario
    res = taf_voicecall_End(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_End-LE_NOT_FOUND");

    //3.taf_voicecall_Delete LE_NOT_FOUND scenario
    res = taf_voicecall_Delete(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_Delete-LE_NOT_FOUND");

    //4.taf_voicecall_Answer LE_NOT_FOUND scenario
    res = taf_voicecall_Answer(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_Answer-LE_NOT_FOUND");

    //5.taf_voicecall_GetEndCause LE_FAULT scenario
   // TafCallRef = taf_voicecall_Start(DestinationNum, 1);
    res = taf_voicecall_GetEndCause(TafCallRef,NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_voicecall_GetEndCause-LE_FAULT");

    //6.taf_voicecall_Hold LE_NOT_FOUND scenario
    res = taf_voicecall_Hold(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_Hold-LE_NOT_FOUND");

    //7.taf_voicecall_Resume LE_NOT_FOUND scenario
    res = taf_voicecall_Resume(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_Resume-LE_NOT_FOUND");

    //8.taf_voicecall_Swap LE_NOT_FOUND scenario
    res = taf_voicecall_Swap(NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_voicecall_Swap-LE_NOT_FOUND");

}
