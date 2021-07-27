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

le_sem_Ref_t semaphore;

taf_radio_RatChangeHandlerRef_t ratChangeHandlerRef;

/*======================================================================

 FUNCTION        RadioRatChangeHandlerFunc

 DESCRIPTION     Handler for RAT change.

 DEPENDENCIES    None

 PARAMETERS      [IN] const taf_radio_RatChangeInd_t* ratChangeIndPtr:
                          Pointer of RAT change indication.
                 [IN] void* contextPtr: The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioRatChangeHandlerFunc(const taf_radio_RatChangeInd_t* ratChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for RAT Change Indication (Begin)****");
    LE_INFO("rat: %d", ratChangeIndPtr->rat);
    LE_INFO("phoneId: %d", ratChangeIndPtr->phoneId);
    LE_INFO("**** Handler for RAT Change Indication (End)****");
}

/*======================================================================

 FUNCTION        RadioRATThread

 DESCRIPTION     The radio access technology test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioRATThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    ratChangeHandlerRef = taf_radio_AddRatChangeHandler(
        (taf_radio_RatChangeHandlerFunc_t)RadioRatChangeHandlerFunc, NULL);
    LE_ASSERT(ratChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

taf_radio_NetRegStateEventHandlerRef_t netRegStateHandlerRef;

/*======================================================================

 FUNCTION        RadioNetRegStateHandlerFunc

 DESCRIPTION     Handler for network registration state.

 DEPENDENCIES    None

 PARAMETERS      [IN] const taf_radio_NetRegStateInd_t* stateIndPtr:
                          Pointer of network registration state indication.
                 [IN] void* contextPtr: The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioNetRegStateHandlerFunc(const taf_radio_NetRegStateInd_t* stateIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for Network Registration State (Begin)****");
    LE_INFO("state: %d", stateIndPtr->state);
    LE_INFO("phoneId: %d", stateIndPtr->phoneId);
    LE_INFO("**** Handler for Network Registration State (End)****");
}

/*======================================================================

 FUNCTION        RadioNetRegStateThread

 DESCRIPTION     The radio network registration state indication test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioNetRegStateThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    netRegStateHandlerRef = taf_radio_AddNetRegStateEventHandler(
        (taf_radio_NetRegStateHandlerFunc_t)RadioNetRegStateHandlerFunc, NULL);
    LE_ASSERT(netRegStateHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

taf_radio_NetRegRejectHandlerRef_t netRegRejectHandlerRef;

/*======================================================================

 FUNCTION        RadioNetRegRejectHandlerFunc

 DESCRIPTION     Handler for network registration rejection.

 DEPENDENCIES    None

 PARAMETERS      [IN] const taf_radio_NetRegRejInd_t* rejIndPtr:
                          Pointer of network registration rejection indication.
                 [IN] void* contextPtr: The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioNetRegRejectHandlerFunc(const taf_radio_NetRegRejInd_t* rejIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for Network Registration Rejection (Begin)****");
    LE_INFO("cause: %d", rejIndPtr->cause);
    LE_INFO("phoneId: %d", rejIndPtr->phoneId);
    LE_INFO("**** Handler for Network Registration Rejection (End)****");
}

/*======================================================================

 FUNCTION        RadioNetRegRejectIndThread

 DESCRIPTION     The radio network registration reject indication test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioNetRegRejectIndThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    netRegRejectHandlerRef = taf_radio_AddNetRegRejectHandler(
        (taf_radio_NetRegRejectHandlerFunc_t)RadioNetRegRejectHandlerFunc, NULL);
    LE_ASSERT(netRegRejectHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

taf_radio_PacketSwitchedChangeHandlerRef_t packetSwChangeHandlerRef;

/*======================================================================

 FUNCTION        RadioPacketSwChangeHandlerFunc

 DESCRIPTION     Handler for packet switched state.

 DEPENDENCIES    None

 PARAMETERS      [IN] taf_radio_ServiceDomainState_t* statePtr:
                          The service domain state.
                 [IN] void* contextPtr: The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioPacketSwChangeHandlerFunc(taf_radio_ServiceDomainState_t state, void* contextPtr)
{
    LE_INFO("**** Handler for Packet Switched Change (Begin)****");
    LE_INFO("state: %d", state);
    LE_INFO("**** Handler for Packet Switched Change (End)****");
}

/*======================================================================

 FUNCTION        RadioPacketSvcStateThread

 DESCRIPTION     The radio packet services state test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioPacketSvcStateThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    packetSwChangeHandlerRef = taf_radio_AddPacketSwitchedChangeHandler(
        (taf_radio_PacketSwitchedChangeHandlerFunc_t)RadioPacketSwChangeHandlerFunc, NULL);
    LE_ASSERT(packetSwChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

#define TEST_RADIO_CELL_RAT_NUM 5

taf_radio_SignalStrengthChangeHandlerRef_t ssChangeHandlerRef[TEST_RADIO_CELL_RAT_NUM];

/*======================================================================

 FUNCTION        RadioSsChangeHandlerFunc

 DESCRIPTION     Handler for signal strength change.

 DEPENDENCIES    None

 PARAMETERS      [IN] int32_t ss:       The signal strength in dbm.
                 [IN] void* contextPtr: The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioSsChangeHandlerFunc(int32_t ss, void* contextPtr)
{
    LE_INFO("**** Handler for Signal Strength Change (Begin)****");
    LE_INFO("ss: %d", ss);
    LE_INFO("**** Handler for Signal Strength Change (End)****");
}

/*======================================================================

 FUNCTION        RadioSsChangeThread

 DESCRIPTION     The radio signal strength change test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioSsChangeThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    taf_radio_CellRatMask_t ratMask[TEST_RADIO_CELL_RAT_NUM] = {
        TAF_RADIO_CELL_RAT_MASK_GSM, TAF_RADIO_CELL_RAT_MASK_CDMA, TAF_RADIO_CELL_RAT_MASK_LTE,
        TAF_RADIO_CELL_RAT_MASK_WCDMA, TAF_RADIO_CELL_RAT_MASK_TDSCDMA};

    for (size_t i = 0; i < TEST_RADIO_CELL_RAT_NUM; i++) {
        ssChangeHandlerRef[i] = taf_radio_AddSignalStrengthChangeHandler(ratMask[i],
            (taf_radio_SignalStrengthChangeHandlerFunc_t)RadioSsChangeHandlerFunc, NULL);
        LE_ASSERT(ssChangeHandlerRef[i] != NULL);
    }

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

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

 FUNCTION        RadioManRegAsyncHandlerFunc

 DESCRIPTION     Handler for setting manual registration mode asynchronously.

 DEPENDENCIES    None

 PARAMETERS      [IN] le_result_t result: Result of manual registration.
                 [IN] void* contextPtr:   The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioManRegAsyncHandlerFunc(le_result_t result, void* contextPtr)
{
    LE_INFO("**** Handler for Manual Registration Asynchronously (Begin)****");
    LE_INFO("result: %d", result);
    LE_INFO("**** Handler for Manual Registration Asynchronously (End)****");
}

/*======================================================================

 FUNCTION        RadioManRegAsyncThread

 DESCRIPTION     The radio manual registration asynchronously test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioManRegAsyncThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    const char* errMccStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"abc", "12", "12a", "460", "460"};
    const char* errMncStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"001", "001", "001", "abc", "1"};

    taf_radio_SetManualRegisterModeAsync("460", "001", RadioManRegAsyncHandlerFunc, NULL, 0);
    LE_ASSERT(taf_radio_GetPlatformSpecificRegistrationErrorCode() == 0);

    for (size_t i = 0; i < TEST_RADIO_ERROR_MCC_MNC_NUM; i++) {
        taf_radio_SetManualRegisterModeAsync(errMccStr[i], errMncStr[i], RadioManRegAsyncHandlerFunc, NULL, 0);
    }

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

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
    LE_INFO("======== 3.1 Radio Register Mode Test ========");

    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    const char* errMccStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"abc", "12", "12a", "460", "460"};
    const char* errMncStr[TEST_RADIO_ERROR_MCC_MNC_NUM] = {"001", "001", "001", "abc", "1"};
    bool mode = true;

    LE_ASSERT(taf_radio_SetAutomaticRegisterMode(0) == LE_OK);
    LE_ASSERT(taf_radio_GetPlatformSpecificRegistrationErrorCode() == 0);
    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == false);

    LE_ASSERT(taf_radio_SetManualRegisterMode("460", "001", 0) == LE_OK);
    LE_ASSERT(taf_radio_GetPlatformSpecificRegistrationErrorCode() == 0);

    for (size_t i = 0; i < TEST_RADIO_ERROR_MCC_MNC_NUM; i++) {
        LE_ASSERT(taf_radio_SetManualRegisterMode(errMccStr[i], errMncStr[i], 0) == LE_BAD_PARAMETER);
    }

    le_thread_Ref_t threadRef = le_thread_Create("RadioManRegAsyncThread",
        RadioManRegAsyncThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == true);
    LE_INFO("taf_radio_GetRegisterMode : mcc.%s mnc.%s", mccStr, mncStr);
    LE_ASSERT(strcmp("460", mccStr) == 0);
    LE_ASSERT(strcmp("1", mncStr) == 0);

    LE_ASSERT(taf_radio_SetAutomaticRegisterMode(0) == LE_OK);
    LE_ASSERT(taf_radio_GetPlatformSpecificRegistrationErrorCode() == 0);

    LE_ASSERT(taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_ASSERT(mode == false);

    LE_INFO("======== 3.2 Radio Prefered Operator Test ========");

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

    LE_ASSERT(taf_radio_DeletePreferredOperatorsList(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_DeletePreferredOperatorsList(listRef) == LE_OK);
}

/*======================================================================

 FUNCTION        Test_taf_radio_RadioAccessTechnology

 DESCRIPTION     The radio access technology test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_RadioAccessTechnology(void)
{
    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_ALL + 1, 0) == LE_BAD_PARAMETER);

    taf_radio_RatPrefMask_t ratPrefMask;
    taf_radio_Rat_t rat;
    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_ALL, 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }
    LE_ASSERT(taf_radio_GetRatPreferences(&ratPrefMask, 0) == LE_OK);
    LE_ASSERT(ratPrefMask == TAF_RADIO_RAT_PREF_MASK_ALL - 1);

    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_LTE, 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }
    LE_ASSERT(taf_radio_GetRatPreferences(&ratPrefMask, 0) == LE_OK);
    LE_ASSERT(ratPrefMask == TAF_RADIO_RAT_PREF_MASK_LTE);
    LE_ASSERT(taf_radio_GetRadioAccessTechInUse(&rat, 0) == LE_OK);
    LE_ASSERT(rat == TAF_RADIO_RAT_LTE);

    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_WCDMA, 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }
    LE_ASSERT(taf_radio_GetRatPreferences(&ratPrefMask, 0) == LE_OK);
    LE_ASSERT(ratPrefMask == TAF_RADIO_RAT_PREF_MASK_WCDMA);
    LE_ASSERT(taf_radio_GetRadioAccessTechInUse(&rat, 0) == LE_OK);
    LE_ASSERT(rat == TAF_RADIO_RAT_UMTS);
}

/*======================================================================

 FUNCTION        Test_taf_radio_PacketServicesState

 DESCRIPTION     The radio packet services state test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_PacketServicesState(void)
{
    taf_radio_ServiceDomainState_t state;
    LE_ASSERT(taf_radio_GetPacketSwitchedState(&state, 0) == LE_OK);
    LE_ASSERT(state == TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS);
}

/*======================================================================

 FUNCTION        Test_taf_radio_SignalQuality

 DESCRIPTION     The radio signal quality test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_SignalQuality(void)
{
    uint32_t quality;
    int32_t rssi;
    uint32_t ber;
    int32_t ss;
    int32_t rscp;
    int32_t rsrq;
    int32_t rsrp;
    int32_t snr;
    int32_t ecio;
    int32_t io;

    LE_ASSERT(taf_radio_GetSignalQual(&quality, 0) == LE_OK);
    LE_ASSERT(quality != 0);

    taf_radio_MetricsRef_t metricsRef = taf_radio_MeasureSignalMetrics(0);
    LE_ASSERT(metricsRef != NULL);

    LE_ASSERT(taf_radio_GetRatOfSignalMetrics(NULL) == TAF_RADIO_CELL_RAT_MASK_UNKNOWN);
    taf_radio_CellRatMask_t ratMask = taf_radio_GetRatOfSignalMetrics(metricsRef);
    LE_INFO("taf_radio_GetRatOfSignalMetrics : ratMask:0x%x", ratMask);

    LE_ASSERT(taf_radio_GetGsmSignalMetrics(NULL, &rssi, &ber) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetUmtsSignalMetrics(NULL, &ss, &ber, &rscp) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetLteSignalMetrics(NULL, &ss, &rsrq, &rsrp, &snr) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetCdmaSignalMetrics(NULL, &ss, &ecio, &snr, &io) == LE_BAD_PARAMETER);

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_GSM) {
        LE_ASSERT(taf_radio_GetGsmSignalMetrics(metricsRef, &rssi, &ber) == LE_OK);
        LE_INFO("taf_radio_GetGsmSignalMetrics : rssi:%d, ber:%d", rssi, ber);
    } else {
        LE_ASSERT(taf_radio_GetGsmSignalMetrics(metricsRef, &rssi, &ber) == LE_UNAVAILABLE);
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_WCDMA) {
        LE_ASSERT(taf_radio_GetUmtsSignalMetrics(metricsRef, &ss, &ber, &rscp) == LE_OK);
        LE_INFO("taf_radio_GetUmtsSignalMetrics : ss:%d, ber:%d", ss, ber);
    } else if (ratMask & TAF_RADIO_CELL_RAT_MASK_TDSCDMA) {
        LE_ASSERT(taf_radio_GetUmtsSignalMetrics(metricsRef, &ss, &ber, &rscp) == LE_OK);
        LE_INFO("taf_radio_GetUmtsSignalMetrics : rscp:%d", rscp);
    } else {
        LE_ASSERT(taf_radio_GetUmtsSignalMetrics(metricsRef, &ss, &ber, &rscp) == LE_UNAVAILABLE);
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_LTE) {
        LE_ASSERT(taf_radio_GetLteSignalMetrics(metricsRef, &ss, &rsrq, &rsrp, &snr) == LE_OK);
        LE_INFO("taf_radio_GetLteSignalMetrics : ss:%d, rsrq:%d, rsrp:%d, snr:%d", ss, rsrq, rsrp, snr);
    } else {
        LE_ASSERT(taf_radio_GetLteSignalMetrics(metricsRef, &ss, &rsrq, &rsrp, &snr) == LE_UNAVAILABLE);
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_CDMA) {
        LE_ASSERT(taf_radio_GetCdmaSignalMetrics(metricsRef, &ss, &ecio, &snr, &io) == LE_OK);
        LE_INFO("taf_radio_GetCdmaSignalMetrics : ss:%d, ecio:%d, snr:%d, io:%d", ss, ecio, snr, io);
    } else {
        LE_ASSERT(taf_radio_GetCdmaSignalMetrics(metricsRef, &ss, &ecio, &snr, &io) == LE_UNAVAILABLE);
    }

    LE_ASSERT(taf_radio_DeleteSignalMetrics(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_DeleteSignalMetrics(metricsRef) == LE_OK);
}

/*======================================================================

 FUNCTION        Test_taf_radio_ServingCellsLocationInformation

 DESCRIPTION     The radio serving cell's location information test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_ServingCellsLocationInformation(void)
{
    LE_INFO("======== 7.1 LTE ========");

    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_LTE, 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    uint32_t cid = taf_radio_GetServingCellId(0);
    LE_INFO("taf_radio_GetServingCellId : cid:%d", cid);
    LE_ASSERT(cid != UINT32_MAX);

    uint16_t tac = taf_radio_GetServingCellLteTracAreaCode(0);
    LE_INFO("taf_radio_GetServingCellLteTracAreaCode : tac:%d", tac);
    LE_ASSERT(tac != UINT16_MAX);

    uint32_t earfcn = taf_radio_GetServingCellEarfcn(0);
    LE_INFO("taf_radio_GetServingCellEarfcn : earfcn:%d", earfcn);
    LE_ASSERT(earfcn != UINT32_MAX);

    uint32_t ta = taf_radio_GetServingCellTimingAdvance(0);
    LE_INFO("taf_radio_GetServingCellTimingAdvance : ta:%d", ta);
    LE_ASSERT(ta != UINT32_MAX);

    uint16_t pid = taf_radio_GetPhysicalServingLteCellId(0);
    LE_INFO("taf_radio_GetPhysicalServingLteCellId : pid:%d", pid);
    LE_ASSERT(pid != UINT16_MAX);

    LE_INFO("======== 7.2 WCDMA ========");

    LE_ASSERT(taf_radio_SetRatPreferences(TAF_RADIO_RAT_PREF_MASK_WCDMA, 0) == LE_OK);
    if (le_thread_Sleep(5)) {
        LE_ERROR("Failed to sleep\n");
    }

    cid = taf_radio_GetServingCellId(0);
    LE_INFO("taf_radio_GetServingCellId : cid:%d", cid);
    LE_ASSERT(cid != UINT32_MAX);

    uint32_t lac = taf_radio_GetServingCellLocAreaCode(0);
    LE_INFO("taf_radio_GetServingCellLocAreaCode : lac:%d", lac);
    LE_ASSERT(lac != UINT32_MAX);

    uint16_t psc = taf_radio_GetServingCellScramblingCode(0);
    LE_INFO("taf_radio_GetServingCellScramblingCode : psc:%d", psc);
    LE_ASSERT(psc != UINT16_MAX);
}

/*======================================================================

 FUNCTION        Test_taf_radio_CurrrentNetworkInformation

 DESCRIPTION     The radio current network information test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_CurrrentNetworkInformation(void)
{
    char nameStr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    LE_ASSERT(taf_radio_GetCurrentNetworkName(nameStr, TAF_RADIO_NETWORK_NAME_MAX_LEN, 0) == LE_OK);
    LE_INFO("taf_radio_GetCurrentNetworkName : name:%s", nameStr);

    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    LE_ASSERT(taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr, TAF_RADIO_MNC_BYTES, 0) == LE_OK);
    LE_INFO("taf_radio_GetCurrentNetworkName : mcc:%s mnc:%s", mccStr, mncStr);
}

/*======================================================================

 FUNCTION        RadioNetScanAsyncHandlerFunc

 DESCRIPTION     Handler for network scan asynchronously.

 DEPENDENCIES    None

 PARAMETERS      [IN] taf_radio_ScanInformationListRef_t listRef:
                          Reference of the scan list.
                 [IN] void* contextPtr:   The context pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
static void RadioNetScanAsyncHandlerFunc(taf_radio_ScanInformationListRef_t listRef, void* contextPtr)
{
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    uint32_t index = 1;

    LE_INFO("**** Handler for Network Scan Asynchronously (Begin)****");
    LE_INFO("listRef: %p", listRef);
    taf_radio_ScanInformationRef_t infoRef = taf_radio_GetFirstCellularNetworkScan(listRef);
    while (infoRef != NULL) {
        LE_ASSERT(taf_radio_GetCellularNetworkName(infoRef, name, TAF_RADIO_NETWORK_NAME_MAX_LEN) == LE_OK);
        LE_INFO("%d Netwok Name    : %s", index, name);
        index++;
        infoRef = taf_radio_GetNextCellularNetworkScan(listRef);
    }
    LE_INFO("**** Handler for Network Scan Asynchronously (End)****");
}

/*======================================================================

 FUNCTION        RadioNetScanAsyncThread

 DESCRIPTION     The radio network scan asynchronously test thread.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: The context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
void* RadioNetScanAsyncThread(void* contextPtr)
{
    //  connect service in thread.
    taf_radio_ConnectService();

    taf_radio_PerformCellularNetworkScanAsync(RadioNetScanAsyncHandlerFunc, NULL, 0);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

/*======================================================================

 FUNCTION        Test_taf_radio_NetworkScan

 DESCRIPTION     The radio network scan test.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void Test_taf_radio_NetworkScan(void)
{
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    char mcc[TAF_RADIO_MCC_BYTES] = {0};;
    char mnc[TAF_RADIO_MNC_BYTES] = {0};;
    bool inUse = false;
    bool available = false;
    bool fobbiden = false;
    bool home = false;
    uint32_t index = 1;

    taf_radio_ScanInformationListRef_t listRef = taf_radio_PerformCellularNetworkScan(0);
    LE_ASSERT(listRef != NULL);

    taf_radio_ScanInformationRef_t infoRef = taf_radio_GetFirstCellularNetworkScan(listRef);

    LE_ASSERT(taf_radio_GetCellularNetworkName(NULL, name, TAF_RADIO_NETWORK_NAME_MAX_LEN) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetCellularNetworkName(infoRef, NULL, TAF_RADIO_NETWORK_NAME_MAX_LEN) == LE_BAD_PARAMETER);

    LE_ASSERT(taf_radio_GetCellularNetworkMccMnc(NULL, mcc, TAF_RADIO_MCC_BYTES, mnc, TAF_RADIO_MNC_BYTES) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetCellularNetworkMccMnc(infoRef, NULL, TAF_RADIO_MCC_BYTES, mnc, TAF_RADIO_MNC_BYTES) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_GetCellularNetworkMccMnc(infoRef, mcc, TAF_RADIO_MCC_BYTES, NULL, TAF_RADIO_MNC_BYTES) == LE_BAD_PARAMETER);

    while (infoRef != NULL) {
        LE_ASSERT(taf_radio_GetCellularNetworkName(infoRef, name, TAF_RADIO_NETWORK_NAME_MAX_LEN) == LE_OK);
        LE_ASSERT(taf_radio_GetCellularNetworkMccMnc(infoRef, mcc, TAF_RADIO_MCC_BYTES, mnc, TAF_RADIO_MNC_BYTES) == LE_OK);
        inUse = taf_radio_IsCellularNetworkInUse(infoRef);
        available = taf_radio_IsCellularNetworkAvailable(infoRef);
        fobbiden = taf_radio_IsCellularNetworkForbidden(infoRef);
        home = taf_radio_IsCellularNetworkHome(infoRef);

        LE_INFO("**** Cellular Network %d ****", index);
        LE_INFO("name    : %s", name);
        LE_INFO("mcc     : %s", mcc);
        LE_INFO("mnc     : %s", mnc);
        if (inUse) {
            LE_INFO("In use");
        } else if (available) {
            LE_INFO("Available");
        } else {
            LE_INFO("Unknown");
        }
        if (fobbiden) {
            LE_INFO("Forbbien");
        } else {
            LE_INFO("Not Forbbien");
        }
        if (home) {
            LE_INFO("Mode    : Home");
        } else {
            LE_INFO("Mode    : Roaming or Unkown");
        }

        index++;
        infoRef = taf_radio_GetNextCellularNetworkScan(listRef);
    }

    LE_ASSERT(taf_radio_DeleteCellularNetworkScan(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_radio_DeleteCellularNetworkScan(listRef) == LE_OK);

    le_thread_Ref_t threadRef = le_thread_Create("RadioNetScanAsyncThread",
        RadioNetScanAsyncThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    if (le_thread_Sleep(60)) {
        LE_ERROR("Failed to sleep\n");
    }

}

/*======================================================================

 FUNCTION        RadioTestThread

 DESCRIPTION     Test thread of the radio service.

 DEPENDENCIES    None

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: success.

 SIDE EFFECTS

======================================================================*/
static void* RadioTestThread(void* contextPtr)
{
    LE_INFO("======== Test Thread of Radio Service Start ========");

    //  connect service in thread.
    taf_radio_ConnectService();

    semaphore = le_sem_Create("tafRadioSem", 0);

    LE_INFO("======== 1. Radio Add Handlers ========");

    LE_INFO("======== 1.1 RAT Handler ========");
    le_thread_Ref_t threadRef = le_thread_Create("RadioRATThread",
        RadioRATThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.2 Network Registration State Handler ========");
    threadRef = le_thread_Create("RadioNetRegStateThread",
        RadioNetRegStateThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.3 Network Registration Rejection Handler ========");
    threadRef = le_thread_Create("RadioNetRegRejectIndThread",
        RadioNetRegRejectIndThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.4 Packet Services State Handler ========");
    threadRef = le_thread_Create("RadioPacketSvcStateThread",
        RadioPacketSvcStateThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.5 Signal Strength Handler ========");
    threadRef = le_thread_Create("RadioSsChangeThread",
        RadioSsChangeThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 2. Radio Power Management Test ========");
    Test_taf_radio_PowerManagement();

    LE_INFO("======== 3. Radio Configuration Preferences Test ========");
    Test_taf_radio_ConfigurationPreferences();

    LE_INFO("======== 4. Radio Access Technology Test ========");
    Test_taf_radio_RadioAccessTechnology();

    LE_INFO("======== 5. Radio Packet Services State Test ========");
    Test_taf_radio_PacketServicesState();

    LE_INFO("======== 6. Radio Signal Quality Test ========");
    Test_taf_radio_SignalQuality();

    LE_INFO("======== 7. Radio Serving Cell's Location Information Test ========");
    Test_taf_radio_ServingCellsLocationInformation();

    LE_INFO("======== 8. Radio Currrent Network Information Test ========");
    Test_taf_radio_CurrrentNetworkInformation();

    LE_INFO("======== 9. Radio Network Scan Test ========");
    Test_taf_radio_NetworkScan();

    LE_INFO("======== 10. Radio Remove Handlers ========");
    taf_radio_RemoveRatChangeHandler(ratChangeHandlerRef);
    taf_radio_RemoveNetRegStateEventHandler(netRegStateHandlerRef);
    taf_radio_RemoveNetRegRejectHandler(netRegRejectHandlerRef);
    taf_radio_RemovePacketSwitchedChangeHandler(packetSwChangeHandlerRef);
    for (size_t i = 0; i < TEST_RADIO_CELL_RAT_NUM; i++) {
        taf_radio_RemoveSignalStrengthChangeHandler(ssChangeHandlerRef[i]);
    }

    LE_INFO("======== Test Thread of Radio Service Exit Successfully ========");

    // exit here to stop tafRadio
    exit(EXIT_SUCCESS);

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
