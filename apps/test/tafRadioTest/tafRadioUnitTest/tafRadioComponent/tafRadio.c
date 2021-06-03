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
 * @file       tafRadio.cpp
 * @brief      This file includes test functions of the Radio Service.
 */

#include "legato.h"
#include "interfaces.h"

/*======================================================================

 FUNCTION        Test_taf_radio_PowerManagement

 DESCRIPTION     The radio power management test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_PowerManagement(void)
{
    le_onoff_t power;
    LE_ASSERT(taf_radio_SetRadioPower(LE_OFF, 0) == LE_OK);

    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    LE_ASSERT(taf_radio_GetRadioPower(&power, 0) == LE_OK);
    LE_ASSERT(power == LE_OFF);

    LE_ASSERT(taf_radio_SetRadioPower(LE_ON, 0) == LE_OK);

    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    LE_ASSERT(taf_radio_GetRadioPower(&power, 0) == LE_OK);
    LE_ASSERT(power == LE_ON);
}

#define TEST_RADIO_OPERATOR_NUM 4
#define TEST_RADIO_ERROR_MCC_MNC_NUM 5

/*======================================================================

 FUNCTION        Test_taf_radio_ConfigurationPreferences

 DESCRIPTION     The radio configuration preferences test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_ConfigurationPreferences(void)
{
    LE_INFO("======== 2.1 Radio Register Mode Test ========");

    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    const char* errMccStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"abc", "12", "12a", "460", "460"};
    const char* errMncStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"001", "001", "001", "abc", "1"};
    bool mode = true;

    LE_ASSERT(taf_radio_SetAutomaticRegisterMode(0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }
    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == false);

    LE_ASSERT(taf_radio_SetManualRegisterMode("460", "001", 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    for (size_t i = 0; i < TEST_RADIO_ERROR_MCC_MNC_NUM; i++) {
        LE_ASSERT(taf_radio_SetManualRegisterMode(errMccStr[i], errMncStr[i], 0) == LE_BAD_PARAMETER);
    }

    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == true);
    LE_INFO("taf_radio_GetRegisterMode : mcc.%s mnc.%s", mccStr, mncStr);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("1", mncStr) == 0);

    LE_ASSERT(taf_radio_SetAutomaticRegisterMode(0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == false);

    LE_INFO("======== 2.2 Radio Prefered Operator Test ========");

    const char* opMccStr[TEST_RADIO_OPERATOR_NUM] = {"460", "460", "460", "460"};
    const char* opMncStr[TEST_RADIO_OPERATOR_NUM] = {"001", "009", "001", "009"};
    taf_radio_RatBitMask_t opRatBitMask[TEST_RADIO_OPERATOR_NUM] =
        {TAF_RADIO_RAT_BIT_MASK_LTE, TAF_RADIO_RAT_BIT_MASK_LTE, TAF_RADIO_RAT_BIT_MASK_UMTS, TAF_RADIO_RAT_BIT_MASK_UMTS};

    for (size_t i = 0; i < TEST_RADIO_ERROR_MCC_MNC_NUM; i++) {
        LE_ASSERT(taf_radio_AddPreferredOperator(errMccStr[i], errMncStr[i], TAF_RADIO_RAT_BIT_MASK_ALL, 0) == LE_BAD_PARAMETER);
    }
    LE_ASSERT(taf_radio_AddPreferredOperator("460", "001", TAF_RADIO_RAT_BIT_MASK_ALL + 1, 0) == LE_BAD_PARAMETER);

    for (size_t i = 0; i < TEST_RADIO_OPERATOR_NUM; i++) {
        LE_ASSERT(taf_radio_AddPreferredOperator(opMccStr[i], opMncStr[i], opRatBitMask[i], 0) == LE_OK);
    }

    taf_radio_PreferredOperatorListRef_t listRef = taf_radio_GetPreferredOperatorsList(0);
    LE_ASSERT(listRef != NULL);

    taf_radio_RatBitMask_t ratMask;
    taf_radio_PreferredOperatorRef_t opRef = taf_radio_GetFirstPreferredOperator(listRef);
    LE_ASSERT(opRef != NULL);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(NULL, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, NULL, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, NULL, TAF_RADIO_MNC_BYTES, &ratMask) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_OK);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("1", mncStr) == 0);
    LE_ASSERT(ratMask == TAF_RADIO_RAT_BIT_MASK_LTE);

    opRef = taf_radio_GetNextPreferredOperator(listRef);
    LE_ASSERT(listRef != NULL);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_OK);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("9", mncStr) == 0);
    LE_ASSERT(ratMask == TAF_RADIO_RAT_BIT_MASK_LTE);

    opRef = taf_radio_GetNextPreferredOperator(listRef);
    LE_ASSERT(listRef != NULL);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_OK);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("1", mncStr) == 0);
    LE_ASSERT(ratMask == TAF_RADIO_RAT_BIT_MASK_UMTS);

    opRef = taf_radio_GetNextPreferredOperator(listRef);
    LE_ASSERT(listRef != NULL);
    LE_ASSERT(taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, &ratMask) == LE_OK);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("9", mncStr) == 0);
    LE_ASSERT(ratMask == TAF_RADIO_RAT_BIT_MASK_UMTS);

    opRef = taf_radio_GetNextPreferredOperator(listRef);
    LE_ASSERT(opRef == NULL);

    for (size_t i = 0; i < TEST_RADIO_ERROR_MCC_MNC_NUM; i++) {
        LE_ASSERT(taf_radio_RemovePreferredOperator(errMccStr[i], errMncStr[i], 0) == LE_BAD_PARAMETER);
    }

    LE_ASSERT(taf_radio_RemovePreferredOperator("460", "001", 0) == LE_OK);
    LE_ASSERT(taf_radio_RemovePreferredOperator("460", "009", 0) == LE_OK);

    LE_ASSERT(taf_radio_RemovePreferredOperator("460", "009", 0) == LE_NOT_FOUND);

    LE_ASSERT(taf_radio_DeletePreferredOperatorsList(listRef) == LE_OK);
}

/*======================================================================

 FUNCTION        RadioTestThread

 DESCRIPTION     Test thread of the radio service.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* context: the context from thread creatation.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
static void* RadioTestThread(void* context)
{
    LE_INFO("======== Test Thread of Radio Service Start ========");

    //  connect service in thread.
    taf_radio_ConnectService();

    LE_INFO("======== 1. Radio Power Management Test ========");
    Test_taf_radio_PowerManagement();

    LE_INFO("======== 2. Radio Configuration Preferences Test ========");
    Test_taf_radio_ConfigurationPreferences();

    LE_INFO("======== Test Thread of Radio Service Exit Successfully ========");

    // exit here to stop tafRadio
    exit(0);

    return NULL;
}

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     The initialization of Radio Sevice Test Component.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("RadioTestThread", RadioTestThread, NULL));
}
