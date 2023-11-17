/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#define DEFAULT_PHONE_ID 1

le_sem_Ref_t scanSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for manual network registation.
 */
//--------------------------------------------------------------------------------------------------
static void ManualRegHandler
(
    le_result_t result, ///< [IN] Result of manual network registation.
    void* contextPtr    ///< [IN] Handler context.
)
{
    LE_INFO("result: %d", result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void NetRegStateHandler
(
    taf_radio_NetRegStateInd_t* netRegStateIndPtr, ///< [IN] Network registation state indication.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    LE_INFO("phone: %d network regristration state: %d", netRegStateIndPtr->phoneId,
        netRegStateIndPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Packet switch state.
 */
//--------------------------------------------------------------------------------------------------
void PackSwStateHandler
(
    taf_radio_NetRegStateInd_t* packSwStateIndPtr, ///< [IN] Packet switched state indication.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    LE_INFO("phone: %d packet switch state: %d", packSwStateIndPtr->phoneId,
        packSwStateIndPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration rejection.
 */
//--------------------------------------------------------------------------------------------------
void NetRegRejectHandler
(
    taf_radio_NetRegRejInd_t* netRegRejIndPtr, ///< [IN] Indication on network rejection.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_INFO("MCC : %s , MNC : %s", netRegRejIndPtr->mcc, netRegRejIndPtr->mnc);
    LE_INFO("phone: %d cause: %d mcc:%s mnc:%s rat:%d domain:%d", netRegRejIndPtr->phoneId,
        netRegRejIndPtr->cause, netRegRejIndPtr->mcc, netRegRejIndPtr->mnc, netRegRejIndPtr->rat,
        netRegRejIndPtr->domain);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void RatChangeHandler
(
    taf_radio_RatChangeInd_t* ratChangeIndPtr, ///< [IN] Indication on RAT change.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d RAT change %d.", ratChangeIndPtr->phoneId, ratChangeIndPtr->rat);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network status change.
 */
//--------------------------------------------------------------------------------------------------
void NetStatusChangeHandler
(
    taf_radio_NetStatusRef_t netStatusRef,   ///< [IN] Network status reference.
    taf_radio_NetStatusIndBitMask_t bitmask, ///< [IN] Network status indication bitmask.
    uint8_t phoneId,                         ///< [IN] Phone ID.
    void* contextPtr                         ///< [IN] Handler context.
)
{
    if (bitmask & TAF_RADIO_NET_STATUS_IND_BIT_MASK_RAT_SVC_STATUS)
    {
        taf_radio_RatSvcStatus_t status;
        le_result_t result = taf_radio_GetRatSvcStatus(netStatusRef, &status);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetRatSvcStatus - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for GSM signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void GsmSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d GSM rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for UMTS signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void UmtsSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d UMTS rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void CdmaSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d CDMA rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for LTE signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void LteSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d LTE rsrp : %d dB", phoneId, rsrp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void Nr5gSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d NR5G rsrp : %d dB", phoneId, rsrp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on GSM signal indication.
 */
//--------------------------------------------------------------------------------------------------
void GsmSignalConfiguration
(
    long phoneId ///< [IN] Phone ID.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_GSM_RSSI,
        -1110, -510, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_GSM_RSSI, 10, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on UMTS signal indication.
 */
//--------------------------------------------------------------------------------------------------
void UmtsSignalConfiguration
(
    long phoneId ///< [IN] Phone ID.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_UMTS_RSSI,
        -1130, -510, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_UMTS_RSSI, 10, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on LTE signal indication.
 */
//--------------------------------------------------------------------------------------------------
void LteSignalConfiguration
(
    long phoneId ///< [IN] Phone ID.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_LTE_RSRP,
        -1400, -440, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_LTE_RSRP, 10, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on NR5G signal indication.
 */
//--------------------------------------------------------------------------------------------------
void Nr5gSignalConfiguration
(
    long phoneId ///< [IN] Phone ID.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_NR5G_RSRP,
        -1400, -440, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_NR5G_RSRP, 10, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network scan.
 */
//--------------------------------------------------------------------------------------------------
static void NetworkScanHandler
(
    taf_radio_ScanInformationListRef_t listRef, ///< [IN] Network scan information list reference.
    void* contextPtr                            ///< [IN] Handler context.
)
{
    le_result_t result = taf_radio_DeleteCellularNetworkScan(listRef);
    LE_TEST_OK(result == LE_OK, "taf_radio_DeleteCellularNetworkScan - LE_OK");

    le_sem_Post(scanSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Pysical Cell Identity network scan.
 */
//--------------------------------------------------------------------------------------------------
static void PciNetworkScanHandler
(
    taf_radio_PciScanInformationListRef_t listRef, ///< [IN] PCI scan list reference.
    uint8_t phoneId,                               ///< [IN] Phone ID.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    le_result_t result = taf_radio_DeletePciNetworkScan(listRef);
    LE_TEST_OK(result == LE_OK, "taf_radio_DeletePciNetworkScan - LE_OK");

    le_sem_Post(scanSemaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Network scan test thread.
 */
//--------------------------------------------------------------------------------------------------
void* NetworkScanTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    taf_radio_PerformCellularNetworkScanAsync(NetworkScanHandler, NULL, DEFAULT_PHONE_ID);

    LE_TEST_OK(true, "taf_radio_PerformCellularNetworkScanAsync - void");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create network scan thread.
 */
//--------------------------------------------------------------------------------------------------
void CreateNetworkScanTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("NetworkScanTestThread", NetworkScanTestThread,
        (void*)semaphore);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pysical Cell Identity network scan test thread.
 */
//--------------------------------------------------------------------------------------------------
void* PciNetworkScanTestThread
(
    void* contextPtr
)
{
    // Connect to service.
    taf_radio_ConnectService();

    taf_radio_PerformPciNetworkScanAsync(TAF_RADIO_RAT_BIT_MASK_LTE, PciNetworkScanHandler,
        NULL, DEFAULT_PHONE_ID);

    LE_TEST_OK(true, "taf_radio_PerformPciNetworkScanAsync - void");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create Pysical Cell Identity network scan thread.
 */
//--------------------------------------------------------------------------------------------------
void CreatePciNetworkScanTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("PciNetworkScanTestThread",
        PciNetworkScanTestThread, (void*)semaphore);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for oeprating mode chanegs.
 */
//--------------------------------------------------------------------------------------------------
void OpModeChangeHandler
(
    taf_radio_OpMode_t mode, ///< [IN] Operating mode.
    void* contextPtr         ///< [IN] Handler context.
)
{
    switch (mode)
    {
        case TAF_RADIO_OP_MODE_ONLINE:
            LE_INFO("Operating Mode : Online.");
            break;
        case TAF_RADIO_OP_MODE_AIRPLANE:
            LE_INFO("Operating Mode : Ariplane.");
            break;
        case TAF_RADIO_OP_MODE_FACTORY_TEST:
            LE_INFO("Operating Mode : Factory Test.");
            break;
        case TAF_RADIO_OP_MODE_OFFLINE:
            LE_INFO("Operating Mode : Offline.");
            break;
        case TAF_RADIO_OP_MODE_RESETTING:
            LE_INFO("Operating Mode : Resetting.");
            break;
        case TAF_RADIO_OP_MODE_SHUTTING_DOWN:
            LE_INFO("Operating Mode : Shutdown.");
            break;
        case TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER:
            LE_INFO("Operating Mode : Persistent Low Power.");
            break;
        default:
            LE_INFO("Operating Mode : Unknown.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test power on/off and power status.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioPower
(
    void
)
{
    taf_radio_OpModeChangeHandlerRef_t opModeChangeHandlerRef = taf_radio_AddOpModeChangeHandler(
        (taf_radio_OpModeChangeHandlerFunc_t)OpModeChangeHandler, NULL);
    LE_TEST_OK(opModeChangeHandlerRef != NULL, "taf_radio_AddOpModeChangeHandler - OK");

    le_onoff_t power;
    le_result_t result = taf_radio_SetRadioPower(LE_OFF, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetRadioPower - LE_OK");

    result = taf_radio_SetRadioPower(LE_ON, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetRadioPower - LE_OK");

    result = taf_radio_GetRadioPower(&power, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioPower - LE_OK");


    taf_radio_RemoveOpModeChangeHandler(opModeChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveOpModeChangeHandler - OK");
    // wait for network reconnection.
    le_thread_Sleep(5);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test network registation, including handlers for network registation state, packet switched
 * state, network registation rejection, automatic/manual network registation, getting registration
 * mode, network registation, packet switched state and platform specific error code of
 * registration.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioNetworkRegistration
(
    void
)
{
    taf_radio_NetRegStateEventHandlerRef_t netRegStateHandlerRef =
        taf_radio_AddNetRegStateEventHandler((taf_radio_NetRegStateHandlerFunc_t)NetRegStateHandler,
        NULL);
    LE_TEST_OK(netRegStateHandlerRef != NULL, "taf_radio_AddNetRegStateEventHandler - !NULL");

    taf_radio_PacketSwitchedChangeHandlerRef_t packSwStateHandlerRef =
        taf_radio_AddPacketSwitchedChangeHandler(
        (taf_radio_PacketSwitchedChangeHandlerFunc_t)PackSwStateHandler, NULL);
    LE_TEST_OK(packSwStateHandlerRef != NULL, "taf_radio_AddPacketSwitchedChangeHandler - !NULL");

    taf_radio_NetRegRejectHandlerRef_t netRegRejHandlerRef =
        taf_radio_AddNetRegRejectHandler((taf_radio_NetRegRejectHandlerFunc_t)NetRegRejectHandler,
        NULL);
    LE_TEST_OK(netRegRejHandlerRef != NULL, "taf_radio_AddNetRegRejectHandler - !NULL");

    le_result_t result = taf_radio_SetAutomaticRegisterMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetAutomaticRegisterMode - LE_OK");

    // wait for network registation.
    le_thread_Sleep(5);

    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    result = taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkMccMnc - LE_OK");

    result = taf_radio_SetManualRegisterMode(mccStr, mncStr, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetManualRegisterMode - LE_OK");

    taf_radio_SetManualRegisterModeAsync(mccStr, mncStr, ManualRegHandler, NULL, DEFAULT_PHONE_ID);
    LE_TEST_OK(true, "taf_radio_SetManualRegisterMode - void");

    result = taf_radio_SetAutomaticRegisterMode(DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetAutomaticRegisterMode - LE_OK");

    taf_radio_NetRegState_t regState;
    result = taf_radio_GetNetRegState(&regState, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetNetRegState - LE_OK");

    result = taf_radio_GetPacketSwitchedState(&regState, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetPacketSwitchedState - LE_OK");

    bool isManual;
    result = taf_radio_GetRegisterMode(&isManual, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRegisterMode - LE_OK");

    int32_t errCode = taf_radio_GetPlatformSpecificRegistrationErrorCode();
    LE_TEST_OK(true, "taf_radio_GetPlatformSpecificRegistrationErrorCode - %d", errCode);

    taf_radio_RemoveNetRegStateEventHandler(netRegStateHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveNetRegStateEventHandler - void");

    taf_radio_RemovePacketSwitchedChangeHandler(packSwStateHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemovePacketSwitchedChangeHandler - void");

    taf_radio_RemoveNetRegRejectHandler(netRegRejHandlerRef);
    LE_TEST_OK(true, "taf_pa_radio_RemoveNetRegRejectHandler - void");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test Radio Access Technology, including handlers for RAT change, getting/setting RAT
 * preferences, and getting RAT in use.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioAccessTechnoloy
(
    void
)
{
    taf_radio_RatChangeHandlerRef_t ratChangeHandlerRef =
        taf_radio_AddRatChangeHandler((taf_radio_RatChangeHandlerFunc_t)RatChangeHandler, NULL);
    LE_TEST_OK(ratChangeHandlerRef != NULL, "taf_radio_AddRatChangeHandler - !NULL");

    taf_radio_NetStatusChangeHandlerRef_t netStatusChangeHandlerRef =
        taf_radio_AddNetStatusChangeHandler(
        (taf_radio_NetStatusHandlerFunc_t)NetStatusChangeHandler, NULL);
    LE_TEST_OK(netStatusChangeHandlerRef != NULL, "taf_radio_AddNetStatusChangeHandler - !NULL");

    taf_radio_RatBitMask_t ratMask;
    le_result_t result = taf_radio_GetRatPreferences(&ratMask, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRatPreferences - LE_OK");

    ratMask = TAF_RADIO_RAT_BIT_MASK_LTE;
    result = taf_radio_SetRatPreferences(ratMask, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetRatPreferences - LE_OK");

    // wait for network registation.
    le_thread_Sleep(5);

    taf_radio_Rat_t rat;
    result = taf_radio_GetRadioAccessTechInUse(&rat, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioAccessTechInUse - LE_OK");

    taf_radio_NetStatusRef_t netRef = taf_radio_GetNetStatus(DEFAULT_PHONE_ID);
    LE_TEST_OK(netRef != NULL, "taf_radio_GetNetStatus - OK");

    taf_radio_RatSvcStatus_t svcStatus = TAF_RADIO_RAT_SVC_STATUS_UNKNOWN;
    result = taf_radio_GetRatSvcStatus(netRef, &svcStatus);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRatSvcStatus - OK");

    taf_radio_CsCap_t cap = TAF_RADIO_CS_CAP_UNKNOWN;
    result = taf_radio_GetLteCsCap(netRef, &cap);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetLteCsCap - OK");

    taf_radio_RemoveRatChangeHandler(ratChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveRatChangeHandler - void");

    taf_radio_RemoveNetStatusChangeHandler(netStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveNetStatusChangeHandler - void");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test band, including getting/setting band preferences, and getting band capabilities.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioBand
(
    void
)
{
    taf_radio_BandBitMask_t bandMask = 0x0;
    uint64_t lteBand[TAF_RADIO_LTE_BAND_GROUP_NUM] = {0};
    size_t lteBandSize = 0;

    le_result_t result = taf_radio_GetBandPreferences(&bandMask, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetBandPreferences - LE_OK");

    result = taf_radio_SetBandPreferences(bandMask, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetBandPreferences - LE_OK");

    result = taf_radio_GetBandCapabilities(&bandMask, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetBandCapabilities - LE_OK");

    result = taf_radio_GetLteBandPreferences(lteBand, &lteBandSize, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetLteBandPreferences - LE_OK");

    result = taf_radio_SetLteBandPreferences(lteBand, TAF_RADIO_LTE_BAND_GROUP_NUM,
        DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetLteBandPreferences - LE_OK");

    result = taf_radio_GetLteBandCapabilities(lteBand, &lteBandSize, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetLteBandCapabilities - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test operator preferences, including adding/removing preferred operators, traversing preferred
 * operators list
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioOperatorPreferences
(
    void
)
{
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    le_result_t result = taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkMccMnc - LE_OK");
    LE_INFO("mcc : %s, mnc : %s.", mccStr, mncStr);

    result = taf_radio_AddPreferredOperator(mccStr, mncStr,
        TAF_RADIO_RAT_BIT_MASK_LTE, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_AddPreferredOperator - LE_OK");

    result = taf_radio_AddPreferredOperator(mccStr, mncStr,
        TAF_RADIO_RAT_BIT_MASK_NR5G, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_AddPreferredOperator - LE_OK");

    taf_radio_PreferredOperatorListRef_t listRef =
        taf_radio_GetPreferredOperatorsList(DEFAULT_PHONE_ID);
    LE_TEST_OK(listRef != NULL, "taf_radio_GetPreferredOperatorsList - !NULL");

    taf_radio_PreferredOperatorRef_t opRef = taf_radio_GetFirstPreferredOperator(listRef);
    LE_TEST_OK(opRef != NULL, "taf_radio_GetFirstPreferredOperator - !NULL");

    taf_radio_RatBitMask_t ratMask;
    result = taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES,
        mncStr, TAF_RADIO_MNC_BYTES, &ratMask);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetPreferredOperatorDetails - LE_OK");

    opRef = taf_radio_GetNextPreferredOperator(listRef);
    LE_TEST_OK(opRef != NULL, "taf_radio_GetNextPreferredOperator - !NULL");

    taf_radio_DeletePreferredOperatorsList(listRef);
    LE_TEST_OK(true, "taf_radio_DeletePreferredOperatorsList - void");

    result = taf_radio_RemovePreferredOperator(mccStr, mncStr, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_RemovePreferredOperator - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test serving status, including Radio Access Technology in use, current network name Mobile
 * Country Code and Mobile Network Code. For GSM network, testing Cell ID, Location Area Code and
 * Base Station Identity Code. For UMTS networkm, testing Primary Scrambling Code. For LTE newtork,
 * testing Tracking Area Code, E-UTRA Absolute Radio Frequency Channel Number, Timing Adcance and
 * Physical Serving Cell ID.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioServingStatus
(
    void
)
{
    le_result_t result;
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};

    taf_radio_Rat_t rat;
    result = taf_radio_GetRadioAccessTechInUse(&rat, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioAccessTechInUse - LE_OK");

    uint32_t cellId;
    uint32_t lac;
    uint8_t bsic = 0;

    uint16_t psc;

    uint16_t tac;
    uint32_t earFcn;
    uint32_t ta;
    uint16_t pscid;

    uint64_t nrCid;
    int32_t arFcn;
    int32_t nrTac;
    uint32_t pcid;

    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
            cellId = taf_radio_GetServingCellId(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellId - uint32_t");
            LE_INFO("cell id : %d.", cellId);

            lac = taf_radio_GetServingCellLocAreaCode(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellLocAreaCode - uint32_t");
            LE_INFO("location area code : %d.", lac);

            result = taf_radio_GetServingCellGsmBsic(&bsic, DEFAULT_PHONE_ID);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellGsmBsic - LE_OK");
            break;
        case TAF_RADIO_RAT_UMTS:
            psc = taf_radio_GetServingCellScramblingCode(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellScramblingCode - uint16_t");
            LE_INFO("primary ScramblingCode : %d.", psc);
            break;
        case TAF_RADIO_RAT_LTE:
            tac = taf_radio_GetServingCellLteTracAreaCode(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellLteTracAreaCode - uint16_t");
            LE_INFO("trac area code : %d.", tac);

            earFcn = taf_radio_GetServingCellEarfcn(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellEarfcn - uint32_t");
            LE_INFO("e-utra absolute radio frequency channel number : %d.", earFcn);

            ta = taf_radio_GetServingCellTimingAdvance(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellTimingAdvance - uint32_t");
            LE_INFO("timing advance : %d.", ta);

            pscid = taf_radio_GetPhysicalServingLteCellId(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetPhysicalServingLteCellId - uint16_t");
            LE_INFO("pysical serving cell id : %d.", pscid);
            break;
        case TAF_RADIO_RAT_NR5G:
            nrCid = taf_radio_GetServingNrCellId(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingNrCellId - uint64_t");
            LE_INFO("cell id : %" PRIuS, (size_t)nrCid);

            arFcn = taf_radio_GetServingCellNrArfcn(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrArfcn - int32_t");
            LE_INFO("nr absolute radio frequency channel number : %d.", arFcn);

            nrTac = taf_radio_GetServingCellNrTracAreaCode(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrTracAreaCode - int32_t");
            LE_INFO("trac area code : %d.", nrTac);

            pcid = taf_radio_GetPhysicalServingNrCellId(DEFAULT_PHONE_ID);
            LE_TEST_OK(true, "taf_radio_GetPhysicalServingNrCellId - OK");
            LE_INFO("pysical serving cell id : %d.", pcid);
            break;
        default:
            LE_INFO("Unavailble RAT %d for serving system.", rat);
            break;
    }

    result = taf_radio_GetCurrentNetworkName(name, TAF_RADIO_NETWORK_NAME_MAX_LEN,
        DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkName - LE_OK");

    result = taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkMccMnc - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test neighboring cell information, including Radio Access Technology, Cell ID, Location Area
 * Code, Signal Strength, Base Station Identity Code, Ec/Io and Physical Cell ID of each neighboring
 * cell.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioNeighborCells
(
    void
)
{
    taf_radio_NeighborCellsRef_t ngbrCellsRef = taf_radio_GetNeighborCellsInfo(DEFAULT_PHONE_ID);
    LE_TEST_OK(ngbrCellsRef != NULL, "taf_radio_GetNeighborCellsInfo - !NULL");

    if (ngbrCellsRef)
    {
        taf_radio_CellInfoRef_t cellInfoRef = taf_radio_GetFirstNeighborCellInfo(ngbrCellsRef);

        uint64_t cid;
        uint32_t lac;
        int32_t rxlevel;
        uint8_t bsic;
        uint16_t pcid;
        uint32_t nrpcid;
        taf_radio_Rat_t rat;
        le_result_t result;

        while (cellInfoRef)
        {
            rat = taf_radio_GetNeighborCellRat(cellInfoRef);
            LE_TEST_OK(true, "taf_radio_GetNeighborCellRat - taf_radio_Rat_t");

            switch (rat)
            {
                case TAF_RADIO_RAT_GSM:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - uint64_t");
                    LE_INFO("cid : %" PRIuS, (size_t)cid);
                    lac = taf_radio_GetNeighborCellLocAreaCode(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellLocAreaCode - uint32_t");
                    LE_INFO("lac : %d.", lac);
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("rxlevel : %d.", rxlevel);
                    result = taf_radio_GetNeighborCellGsmBsic(cellInfoRef, &bsic);
                    LE_TEST_OK(result == LE_OK, "taf_radio_GetNeighborCellGsmBsic - LE_OK");
                    break;
                case TAF_RADIO_RAT_UMTS:
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("rxlevel : %d.", rxlevel);
                    break;
                case TAF_RADIO_RAT_CDMA:
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("rxlevel : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_TDSCDMA:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - uint64_t");
                    LE_INFO("cid : %" PRIuS, (size_t)cid);
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("rxlevel : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_NR5G:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - uint64_t");
                    LE_INFO("cid : %" PRIuS, (size_t)cid);
                    nrpcid = taf_radio_GetPhysicalNeighborNrCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetPhysicalNeighborNrCellId - uint32_t");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("nrpcid : %d", nrpcid);
                    LE_INFO("rxlevel : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_LTE:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - uint64_t");
                    LE_INFO("cid : %" PRIuS, (size_t)cid);
                    pcid = taf_radio_GetPhysicalNeighborLteCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellUmtsEcIo - uint16_t");
                    LE_INFO("pcid : %d.", pcid);
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - int32_t");
                    LE_INFO("rxlevel : %d.", rxlevel);
                    break;
                default:
                    break;
            }

            cellInfoRef = taf_radio_GetNextNeighborCellInfo(ngbrCellsRef);
        }

        result = taf_radio_DeleteNeighborCellsInfo(ngbrCellsRef);
        LE_TEST_OK(result == LE_OK, "taf_radio_DeleteNeighborCellsInfo - LE_OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test signal strength, including handlers for signal changes, setting thresolds/delta/hysterisis
 * for signal indications and getting signal metrics.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioSignal
(
    void
)
{
    taf_radio_SignalStrengthChangeHandlerRef_t gsmSsChangeHandlerRef =
        taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_GSM,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)GsmSsChangeHandler, NULL);
    LE_TEST_OK(gsmSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    taf_radio_SignalStrengthChangeHandlerRef_t umtsSsChangeHandlerRef =
        taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_UMTS,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)UmtsSsChangeHandler, NULL);
    LE_TEST_OK(umtsSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    taf_radio_SignalStrengthChangeHandlerRef_t cdmaSsChangeHandlerRef =
        taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_CDMA,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)CdmaSsChangeHandler, NULL);
    LE_TEST_OK(cdmaSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    taf_radio_SignalStrengthChangeHandlerRef_t lteSsChangeHandlerRef =
        taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)LteSsChangeHandler, NULL);
    LE_TEST_OK(lteSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    taf_radio_SignalStrengthChangeHandlerRef_t nr5gSsChangeHandlerRef =
        taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)Nr5gSsChangeHandler, NULL);
    LE_TEST_OK(nr5gSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    uint32_t quality = 0;
    le_result_t result = taf_radio_GetSignalQual(&quality, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetSignalQual - LE_OK");

    taf_radio_MetricsRef_t metrics = taf_radio_MeasureSignalMetrics(DEFAULT_PHONE_ID);
    LE_TEST_OK(metrics != NULL, "taf_radio_MeasureSignalMetrics - !NULL");

    taf_radio_RatBitMask_t ratMask = taf_radio_GetRatOfSignalMetrics(metrics);
    LE_TEST_OK(true, "taf_radio_GetRatOfSignalMetrics - taf_radio_RatBitMask_t");

    if (ratMask & TAF_RADIO_RAT_BIT_MASK_GSM)
    {
        int32_t rssi;
        uint32_t ber;
        result = taf_radio_GetGsmSignalMetrics(metrics, &rssi, &ber);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetGsmSignalMetrics - LE_OK");
    }

    if (ratMask & (TAF_RADIO_RAT_BIT_MASK_UMTS | TAF_RADIO_RAT_BIT_MASK_TDSCDMA))
    {
        int32_t ss;
        uint32_t bler;
        int32_t rscp;
        result = taf_radio_GetUmtsSignalMetrics(metrics, &ss, &bler, &rscp);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetUmtsSignalMetrics - LE_OK");
    }

    if (ratMask & TAF_RADIO_RAT_BIT_MASK_LTE)
    {
        int32_t ss;
        int32_t rsrq;
        int32_t rsrp;
        int32_t snr;
        result = taf_radio_GetLteSignalMetrics(metrics, &ss, &rsrq, &rsrp, &snr);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetLteSignalMetrics - LE_OK");
    }

    if (ratMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
    {
        int32_t rsrq;
        int32_t rsrp;
        int32_t snr;
        result = taf_radio_GetNr5gSignalMetrics(metrics, &rsrq, &rsrp, &snr);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetNr5gSignalMetrics - LE_OK");
    }

    result = taf_radio_DeleteSignalMetrics(metrics);
    LE_TEST_OK(result == LE_OK, "taf_radio_DeleteSignalMetrics - LE_OK");

    GsmSignalConfiguration(DEFAULT_PHONE_ID);
    UmtsSignalConfiguration(DEFAULT_PHONE_ID);
    LteSignalConfiguration(DEFAULT_PHONE_ID);
    Nr5gSignalConfiguration(DEFAULT_PHONE_ID);

    taf_radio_RemoveSignalStrengthChangeHandler(gsmSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(umtsSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(cdmaSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(lteSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(nr5gSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test network scan, including regular/Pysical Cell Identity network scan in sync/async mode. For
 * regular network scan, getting RAT/Moblile Country Code and Mobile Network Code/name/status of
 * each network. For Pysical Cell Identity network scan, getting Cell ID/Global Cell ID, of each
 * network, and Moblile Country Code and Mobile Network Code of each PLMN network.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioNetworkScan
(
    void
)
{
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    taf_radio_Rat_t rat;
    le_result_t result;

    bool inUse = false;
    bool available = false;
    bool fobbiden = false;
    bool home = false;

    scanSemaphore = le_sem_Create("scanSemaphore", 0);

    result = taf_radio_SetRatPreferences(TAF_RADIO_RAT_BIT_MASK_LTE, DEFAULT_PHONE_ID);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetRatPreferences - LE_OK");
    // wait for LTE network reconnection.
    le_thread_Sleep(5);

    taf_radio_ScanInformationListRef_t listRef =
        taf_radio_PerformCellularNetworkScan(DEFAULT_PHONE_ID);
    LE_TEST_OK(listRef != NULL, "taf_radio_PerformCellularNetworkScan - !NULL");

    if (listRef != NULL)
    {
        taf_radio_ScanInformationRef_t infoRef = taf_radio_GetFirstCellularNetworkScan(listRef);
        LE_TEST_OK(infoRef != NULL, "taf_radio_GetFirstCellularNetworkScan - !NULL");
        while (infoRef)
        {
            result = taf_radio_GetCellularNetworkName(infoRef, name,
                TAF_RADIO_NETWORK_NAME_MAX_LEN);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetCellularNetworkName - LE_OK");

            rat = taf_radio_GetCellularNetworkRat(infoRef);
            LE_TEST_OK(true, "taf_radio_GetCellularNetworkRat - taf_radio_Rat_t");
            LE_INFO("rat : %d.", rat);

            result = taf_radio_GetCellularNetworkMccMnc(infoRef, mccStr, TAF_RADIO_MCC_BYTES,
                mncStr, TAF_RADIO_MNC_BYTES);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetCellularNetworkMccMnc - LE_OK");

            inUse = taf_radio_IsCellularNetworkInUse(infoRef);
            LE_TEST_OK(true, "taf_radio_IsCellularNetworkInUse - bool");
            LE_INFO("in use : %d.", inUse);

            available = taf_radio_IsCellularNetworkAvailable(infoRef);
            LE_TEST_OK(true, "taf_radio_IsCellularNetworkAvailable - bool");
            LE_INFO("available : %d.", available);

            fobbiden = taf_radio_IsCellularNetworkForbidden(infoRef);
            LE_TEST_OK(true, "taf_radio_IsCellularNetworkForbidden - bool");
            LE_INFO("fobbiden : %d.", fobbiden);

            home = taf_radio_IsCellularNetworkHome(infoRef);
            LE_TEST_OK(true, "taf_radio_IsCellularNetworkHome - bool");
            LE_INFO("home : %d.", home);

            infoRef = taf_radio_GetNextCellularNetworkScan(listRef);
        }

        result = taf_radio_DeleteCellularNetworkScan(listRef);
        LE_TEST_OK(result == LE_OK, "taf_radio_DeleteCellularNetworkScan - LE_OK");
    }

    CreateNetworkScanTestThread();
    le_sem_Wait(scanSemaphore);

    taf_radio_PciScanInformationListRef_t pciListRef =
        taf_radio_PerformPciNetworkScan(TAF_RADIO_RAT_BIT_MASK_LTE, DEFAULT_PHONE_ID);
    LE_TEST_OK(pciListRef != NULL, "taf_radio_PerformPciNetworkScan - !NULL");

    if (pciListRef != NULL)
    {
        taf_radio_PciScanInformationRef_t pciInfoRef = taf_radio_GetFirstPciScanInfo(pciListRef);
        LE_TEST_OK(pciInfoRef != NULL, "taf_radio_GetFirstPciScanInfo - !NULL");

        uint16_t cell_id = 0;
        uint32_t global_cell_id = 0;
        taf_radio_PlmnInformationRef_t plmnRef;

        while (pciInfoRef)
        {
            cell_id = taf_radio_GetPciScanCellId(pciInfoRef);
            LE_TEST_OK(true, "taf_radio_GetPciScanCellId - uint16_t");
            LE_INFO("cell_id : %d.", cell_id);

            global_cell_id = taf_radio_GetPciScanGlobalCellId(pciInfoRef);
            LE_TEST_OK(true, "taf_radio_GetPciScanGlobalCellId - uint32_t");
            LE_INFO("global_cell_id : %d.", global_cell_id);

            plmnRef = taf_radio_GetFirstPlmnInfo(pciInfoRef);
            LE_TEST_OK(plmnRef != NULL, "taf_radio_GetFirstPlmnInfo - !NULL");
            while (plmnRef)
            {
                result = taf_radio_GetPciScanMccMnc(plmnRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
                    TAF_RADIO_MNC_BYTES);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetPciScanMccMnc - LE_OK");

                plmnRef = taf_radio_GetNextPlmnInfo(pciInfoRef);
            }

            pciInfoRef = taf_radio_GetNextPciScanInfo(pciListRef);
        }

        result = taf_radio_DeletePciNetworkScan(pciListRef);
        LE_TEST_OK(result == LE_OK, "taf_radio_DeletePciNetworkScan - LE_OK");
    }

    CreatePciNetworkScanTestThread();
    le_sem_Wait(scanSemaphore);
    le_sem_Delete(scanSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void ImsRegStateHandler
(
    taf_radio_ImsRegStatus_t status, ///< [IN] IMS registation state.
    uint8_t phoneId,                 ///< [IN] Phone ID.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    switch (status)
    {
        case TAF_RADIO_IMS_REG_STATUS_REGISTERED:
            LE_INFO("Phone %d IMS : Registered.", phoneId);
            break;
        case TAF_RADIO_IMS_REG_STATUS_NOT_REGISTERED:
            LE_INFO("Phone %d IMS : Not registered.", phoneId);
            break;
        default:
            LE_INFO("Phone %d IMS : Unknown.", phoneId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for IMS status.
 */
//--------------------------------------------------------------------------------------------------
void ImsStatusHandler
(
    taf_radio_ImsRef_t imsRef,         ///< [IN] IMS reference.
    taf_radio_ImsIndBitMask_t bitmask, ///< [IN] Indication bitmask.
    uint8_t phoneId,                   ///< [IN] Phone ID.
    void* contextPtr                   ///< [IN] Handler context.
)
{
    le_result_t result = LE_OK;

    if (bitmask & TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO)
    {
        taf_radio_ImsSvcStatus_t svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
        result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &svcStatus);
        if (result != LE_UNSUPPORTED)
        {
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
        }
    }

    if (bitmask & TAF_RADIO_IMS_IND_BIT_MASK_PDP_ERROR)
    {
        taf_radio_PdpError_t pdpError = TAF_RADIO_PDP_ERROR_UNKNOWN;
        result = taf_radio_GetImsPdpError(imsRef, &pdpError);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetImsPdpError - LE_OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test IMS.
 */
//--------------------------------------------------------------------------------------------------
void TestTafRadioIms
(
    void
)
{
    taf_radio_ImsRegStatusChangeHandlerRef_t imsRegStatusChangeHandlerRef =
        taf_radio_AddImsRegStatusChangeHandler(
        (taf_radio_ImsRegStatusChangeHandlerFunc_t)ImsRegStateHandler, NULL);
    LE_TEST_OK(imsRegStatusChangeHandlerRef != NULL,
        "taf_radio_AddImsRegStatusChangeHandler - !NULL");

    taf_radio_ImsStatusChangeHandlerRef_t imsStatusChangeHandlerRef =
        taf_radio_AddImsStatusChangeHandler(
        (taf_radio_ImsStatusChangeHandlerFunc_t)ImsStatusHandler, NULL);
    LE_TEST_OK(imsStatusChangeHandlerRef != NULL, "taf_radio_AddImsStatusChangeHandler - !NULL");

    taf_radio_ImsRegStatus_t regStatus = TAF_RADIO_IMS_REG_STATUS_NOT_REGISTERED;
    le_result_t result = taf_radio_GetImsRegStatus(&regStatus, DEFAULT_PHONE_ID);

    taf_radio_ImsRef_t imsRef = taf_radio_GetIms(DEFAULT_PHONE_ID);
    LE_TEST_OK(imsRef != NULL, "taf_radio_GetIms - LE_OK");

    taf_radio_ImsSvcStatus_t svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
    result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &svcStatus);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
    }

    bool enable = false;
    result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &enable);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");

    result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, enable);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");

    char userAgent[TAF_RADIO_IMS_USER_AGENT_BYTES] = "tafRadioUnitTest";
    result = taf_radio_SetImsUserAgent(imsRef, userAgent);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetImsUserAgent - LE_OK");
    }

    result = taf_radio_GetImsUserAgent(imsRef, userAgent, TAF_RADIO_IMS_USER_AGENT_BYTES);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_GetImsUserAgent - LE_OK");
    }

    taf_radio_RemoveImsRegStatusChangeHandler(imsRegStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveImsRegStatusChangeHandler - void");

    taf_radio_RemoveImsStatusChangeHandler(imsStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveImsStatusChangeHandler - void");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_INFO("======== Radio Power Test ========");
    TestTafRadioPower();
    LE_TEST_INFO("======== Radio Network Registration Test ========");
    TestTafRadioNetworkRegistration();
    LE_TEST_INFO("======== Radio Access Technology Test ========");
    TestTafRadioAccessTechnoloy();
    LE_TEST_INFO("======== Radio Band Test ========");
    TestTafRadioBand();
    LE_TEST_INFO("======== Radio Operator Preferences Test ========");
    TestTafRadioOperatorPreferences();
    LE_TEST_INFO("======== Radio Serving Status Test ========");
    TestTafRadioServingStatus();
    LE_TEST_INFO("======== Radio Neighboring Cells Test ========");
    TestTafRadioNeighborCells();
    LE_TEST_INFO("======== Radio Signal Test ========");
    TestTafRadioSignal();
    LE_TEST_INFO("======== Radio Network Scan Test ========");
    TestTafRadioNetworkScan();
    LE_TEST_INFO("======== Radio IMS Test ========");
    TestTafRadioIms();

    LE_TEST_EXIT;
}
