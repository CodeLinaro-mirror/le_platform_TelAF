/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssRadioSvc.hpp"

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss Radio server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssRadioSvc> tafIvssRadioSvc::GetInstance()
{
    static std::shared_ptr<tafIvssRadioSvc> instance = std::make_shared<tafIvssRadioSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'SetRadioPower'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetRadioPowerHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_SetRadioPower(indPtr->setRadioPower.power,
        indPtr->setRadioPower.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_SetRadioPower fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetRadioPower
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    const CommonTypes::OnOffType _power,
    SetRadioPowerReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss SetRadioPowerSem", 0);
    indPtr->setRadioPower.phoneId = PhoneIdIvssToUint8(_phoneId);
    indPtr->setRadioPower.power = OnoffIvssToLe(_power);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(SetRadioPowerEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result));

    LE_DEBUG("tafIvssRadioSvc SetRadioPower: %s",
        ((indPtr->setRadioPower.power == LE_ON) ? "ON" : "OFF"));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetRadioPower'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRadioPowerHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetRadioPower(&indPtr->getRadioPower.power,
        indPtr->getRadioPower.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetRadioPower fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRadioPower
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetRadioPowerReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetRadioPowerSem", 0);
    indPtr->getRadioPower.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRadioPowerEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), OnoffLeToIvss(indPtr->getRadioPower.power));

    LE_DEBUG("tafIvssRadioSvc GetRadioPower: %s",
        ((indPtr->getRadioPower.power == LE_ON) ? "ON" : "OFF"));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetSignalStrength'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetSignalStrengthHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;

    taf_radio_MetricsRef_t metrics =
        taf_radio_MeasureSignalMetrics(indPtr->getSignalStrength.phoneId);
    if (metrics == nullptr)
    {
        indPtr->result = LE_FAULT;
        le_sem_Post(indPtr->semRef);
        LE_ERROR("taf_radio_MeasureSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));
        return;
    }

    switch (indPtr->getSignalStrength.rat)
    {
        case TAF_RADIO_RAT_GSM:
            uint32_t ber;
            indPtr->result = taf_radio_GetGsmSignalMetrics(metrics, &indPtr->getSignalStrength.ss,
                &ber);
            break;
        case TAF_RADIO_RAT_UMTS:
            uint32_t bler;
            int32_t rscp;
            indPtr->result = taf_radio_GetUmtsSignalMetrics(metrics, &indPtr->getSignalStrength.ss,
                &bler, &rscp);
            break;
        case TAF_RADIO_RAT_LTE:
            int32_t rsrq;
            int32_t snr;
            indPtr->result = taf_radio_GetLteSignalMetrics(metrics, &indPtr->getSignalStrength.ss,
                &rsrq, &indPtr->getSignalStrength.rsrp, &snr);
            break;
        case TAF_RADIO_RAT_NR5G:
            indPtr->result = taf_radio_GetNr5gSignalMetrics(metrics, &rsrq,
                &indPtr->getSignalStrength.rsrp, &snr);
            break;
        default:
            indPtr->result = LE_BAD_PARAMETER;
            LE_ERROR("Invalid rat(%d) type", (int32_t)indPtr->getSignalStrength.rat);
            break;
    }
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "GetSignalMetrics fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetSignalStrength
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    RadioSvc::Rat _rat,
    GetSignalStrengthReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));

    indPtr->semRef = le_sem_Create("Ivss GetSignalStrengthSem", 0);
    indPtr->getSignalStrength.phoneId = PhoneIdIvssToUint8(_phoneId);
    indPtr->getSignalStrength.rat = RatIvssToRadio(_rat);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetSignalStrengthEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), indPtr->getSignalStrength.ss,
        indPtr->getSignalStrength.rsrp);

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetRegisterMode'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRegisterModeHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetRegisterMode(&indPtr->getRegisterMode.isManual,
        indPtr->getRegisterMode.mcc, TAF_RADIO_MCC_BYTES,
        indPtr->getRegisterMode.mnc, TAF_RADIO_MNC_BYTES, indPtr->getRegisterMode.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetRegisterMode fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration mode.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRegisterMode
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetRegisterModeReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetRegisterModeSem", 0);
    indPtr->getRegisterMode.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRegisterModeEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), indPtr->getRegisterMode.isManual,
        std::string(indPtr->getRegisterMode.mcc), std::string(indPtr->getRegisterMode.mnc));


    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'SetAutomaticRegisterMode'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetAutomaticRegisterModeHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_SetAutomaticRegisterMode(indPtr->setAutomaticRegisterMode.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_SetAutomaticRegisterMode fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers to network using automatic mode.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetAutomaticRegisterMode
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    SetAutomaticRegisterModeReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss SetAutomaticRegisterModeSem", 0);
    indPtr->setAutomaticRegisterMode.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(SetAutomaticRegisterModeEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetHardwareConfig'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetHardwareConfigHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetHardwareSimConfig(&indPtr->getHardwareConfig.totalSimCount,
        &indPtr->getHardwareConfig.maxActiveSims);
    if (indPtr->result != LE_OK)
    {
        le_sem_Post(indPtr->semRef);
        TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetHardwareSimConfig fail - %s",
            LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetHardwareSimRatCapabilities(
        &indPtr->getHardwareConfig.deviceRatCapMask, &indPtr->getHardwareConfig.simRatCapMask,
        indPtr->getHardwareConfig.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK,
        "taf_radio_GetHardwareSimRatCapabilities fail - %s", LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetHardwareConfig
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetHardwareConfigReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetHardwareConfigSem", 0);
    indPtr->getHardwareConfig.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetHardwareConfigEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), indPtr->getHardwareConfig.totalSimCount,
        indPtr->getHardwareConfig.maxActiveSims,
        RatBitMaskRadioToIvss(indPtr->getHardwareConfig.deviceRatCapMask),
        RatBitMaskRadioToIvss(indPtr->getHardwareConfig.simRatCapMask));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetRatPreferences'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRatPreferencesHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetRatPreferences(&indPtr->getRatPreferences.ratMask,
        indPtr->getRatPreferences.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetRatPreferences fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT preferences.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRatPreferences
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetRatPreferencesReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetRatPreferencesSem", 0);
    indPtr->getRatPreferences.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRatPreferencesEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result),
        RatBitMaskRadioToIvss(indPtr->getRatPreferences.ratMask));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetCurrentNetworkName'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetCurrentNetworkNameHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetCurrentNetworkLongName(indPtr->getCurrentNetworkName.longName,
        TAF_RADIO_NETWORK_NAME_MAX_LEN, indPtr->getCurrentNetworkName.phoneId);
    if (indPtr->result != LE_OK)
    {
        le_sem_Post(indPtr->semRef);
        TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK,
            "taf_radio_GetCurrentNetworkLongName fail - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetCurrentNetworkName(indPtr->getCurrentNetworkName.shortName,
        TAF_RADIO_NETWORK_NAME_MAX_LEN, indPtr->getCurrentNetworkName.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetCurrentNetworkName fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetCurrentNetworkName
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetCurrentNetworkNameReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetCurrentNetworkNameSem", 0);
    indPtr->getCurrentNetworkName.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetCurrentNetworkNameEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), std::string(indPtr->getCurrentNetworkName.longName),
        std::string(indPtr->getCurrentNetworkName.shortName));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetNetRegState'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNetRegStateHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetNetRegState(&indPtr->getNetRegState.netReg,
        indPtr->getNetRegState.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetNetRegState fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNetRegState
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetNetRegStateReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetNetRegStateSem", 0);
    indPtr->getNetRegState.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetNetRegStateEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), NetRegRadioToIvss(indPtr->getNetRegState.netReg));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetNrDualConnectivityStatus'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNrDualConnectivityStatusHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetNrDualConnectivityStatus(
        &indPtr->getNrDualConnectivityStatus.statusEndc,
        &indPtr->getNrDualConnectivityStatus.statusDcnr,
        indPtr->getNrDualConnectivityStatus.phoneId);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_radio_GetNrDualConnectivityStatus fail - %s",
        LE_RESULT_TXT(indPtr->result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNrDualConnectivityStatus
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    CommonTypes::PhoneId _phoneId,
    GetNrDualConnectivityStatusReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetNrDualConnectivityStatusSem", 0);
    indPtr->getNrDualConnectivityStatus.phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetNrDualConnectivityStatusEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result),
        NREndcRadioToIvss(indPtr->getNrDualConnectivityStatus.statusEndc),
        NRDcnrRadioToIvss(indPtr->getNrDualConnectivityStatus.statusDcnr));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for GSM signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_GsmSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvc::Rat::RAT_GSM, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc GsmSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for UMTS signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_UmtsSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvc::Rat::RAT_UMTS, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc UmtsSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for TDSCDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_TdscdmaSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvc::Rat::RAT_TDSCDMA, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc TdscdmaSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for LTE signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_LteSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvc::Rat::RAT_LTE, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc LteSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_Nr5gSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvc::Rat::RAT_NR5G, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc Nr5gSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_RatChangeHandler
(
    taf_radio_RatChangeInd_t* ratChangeIndPtr, ///< [IN] Indication on RAT change.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireRadioRatEvent(RatRadioToIvss(ratChangeIndPtr->rat),
        PhoneIdUint8ToIvss(ratChangeIndPtr->phoneId));
    LE_DEBUG("tafIvssRadioSvc RatChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for operating mode changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_StateChangeHandler
(
    taf_radio_OpMode_t mode,    ///< [IN] Operating mode.
    void* contextPtr            ///< [IN] Handler context.
)
{
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireRadioStateEvent(StatesRadioToIvss(mode));
    LE_DEBUG("tafIvssRadioSvc StateChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Radio EventPool", sizeof(taf_IvssRadio_Ind_t));

    // Init events.
    SetRadioPowerEvent = le_event_CreateIdWithRefCounting("SetRadioPowerEvent");
    GetRadioPowerEvent = le_event_CreateIdWithRefCounting("GetRadioPowerEvent");
    GetSignalStrengthEvent = le_event_CreateIdWithRefCounting("GetSignalStrengthEvent");
    GetRegisterModeEvent = le_event_CreateIdWithRefCounting("GetRegisterModeEvent");
    SetAutomaticRegisterModeEvent = le_event_CreateIdWithRefCounting(
        "SetAutomaticRegisterModeEvent");
    GetHardwareConfigEvent = le_event_CreateIdWithRefCounting("GetHardwareConfigEvent");
    GetRatPreferencesEvent = le_event_CreateIdWithRefCounting("GetRatPreferencesEvent");
    GetCurrentNetworkNameEvent = le_event_CreateIdWithRefCounting("GetCurrentNetworkNameEvent");
    GetNetRegStateEvent = le_event_CreateIdWithRefCounting("GetNetRegStateEvent");
    GetNrDualConnectivityStatusEvent = le_event_CreateIdWithRefCounting(
        "GetNrDualConnectivityStatusEvent");

    // Init event handler.
    SetRadioPowerEventHandlerRef = le_event_AddHandler("SetRadioPowerEvent Handler",
        SetRadioPowerEvent, tafIvssRadioSvc::SetRadioPowerHandler);
    GetRadioPowerEventHandlerRef = le_event_AddHandler("GetRadioPowerEvent Handler",
        GetRadioPowerEvent, tafIvssRadioSvc::GetRadioPowerHandler);
    GetSignalStrengthEventHandlerRef = le_event_AddHandler("GetSignalStrengthEvent Handler",
        GetSignalStrengthEvent, tafIvssRadioSvc::GetSignalStrengthHandler);
    GetRegisterModeEventHandlerRef = le_event_AddHandler("GetRegisterModeEvent Handler",
        GetRegisterModeEvent, tafIvssRadioSvc::GetRegisterModeHandler);
    SetAutomaticRegisterModeEventHandlerRef = le_event_AddHandler(
        "SetAutomaticRegisterModeEvent Handler", SetAutomaticRegisterModeEvent,
        tafIvssRadioSvc::SetAutomaticRegisterModeHandler);
    GetHardwareConfigEventHandlerRef = le_event_AddHandler("GetHardwareConfigEvent Handler",
        GetHardwareConfigEvent, tafIvssRadioSvc::GetHardwareConfigHandler);
    GetRatPreferencesEventHandlerRef = le_event_AddHandler("GetRatPreferencesEvent Handler",
        GetRatPreferencesEvent, tafIvssRadioSvc::GetRatPreferencesHandler);
    GetCurrentNetworkNameEventHandlerRef = le_event_AddHandler("GetCurrentNetworkNameEvent Handler",
        GetCurrentNetworkNameEvent, tafIvssRadioSvc::GetCurrentNetworkNameHandler);
    GetNetRegStateEventHandlerRef = le_event_AddHandler("GetNetRegStateEvent Handler",
        GetNetRegStateEvent, tafIvssRadioSvc::GetNetRegStateHandler);
    GetNrDualConnectivityStatusEventHandlerRef = le_event_AddHandler(
        "GetNrDualConnectivityStatusEvent Handler", GetNrDualConnectivityStatusEvent,
        tafIvssRadioSvc::GetNrDualConnectivityStatusHandler);

    // Init commonapi event.
    RatChangeHandlerRef = taf_radio_AddRatChangeHandler(
        (taf_radio_RatChangeHandlerFunc_t)taf_ivss_radio_RatChangeHandler, NULL);
    GsmSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_GSM,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_GsmSsChangeHandler, NULL);
    UmtsSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_UMTS,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_UmtsSsChangeHandler, NULL);
    TdscdmaSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_TDSCDMA,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_TdscdmaSsChangeHandler, NULL);
    LteSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_LteSsChangeHandler, NULL);
    Nr5gSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_Nr5gSsChangeHandler, NULL);
    StateChangeHandlerRef = taf_radio_AddOpModeChangeHandler(
        (taf_radio_OpModeChangeHandlerFunc_t)taf_ivss_radio_StateChangeHandler, NULL);

    LE_INFO("tafIvssRadioSvc Service initialized");
};
