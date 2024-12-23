/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssRadioSvc.hpp"

using namespace v2::com::qualcomm::qti::telephony;

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
 * Add handler function for method 'GetGsmSignalMetrics'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetGsmSignalMetricsHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;

    taf_radio_MetricsRef_t metrics =
        taf_radio_MeasureSignalMetrics(indPtr->GetGsmSignalMetrics.phoneId);
    if (metrics == nullptr)
    {
        indPtr->result = LE_FAULT;
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_MeasureSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetGsmSignalMetrics(metrics, &indPtr->GetGsmSignalMetrics.rssi,
        &indPtr->GetGsmSignalMetrics.ber);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetGsmSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetGsmSignalMetrics
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetGsmSignalMetricsReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetGsmSignalMetrics \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));

    indPtr->semRef = le_sem_Create("Ivss GetGsmSignalMetricsSem", 0);
    indPtr->GetGsmSignalMetrics.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetGsmSignalMetricsEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(indPtr->GetGsmSignalMetrics.rssi, indPtr->GetGsmSignalMetrics.ber,
        ResultLeToIvssRadio(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetUmtsSignalMetrics'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetUmtsSignalMetricsHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;

    taf_radio_MetricsRef_t metrics =
        taf_radio_MeasureSignalMetrics(indPtr->GetUmtsSignalMetrics.phoneId);
    if (metrics == nullptr)
    {
        indPtr->result = LE_FAULT;
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_MeasureSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetUmtsSignalMetrics(metrics, &indPtr->GetUmtsSignalMetrics.ss,
        &indPtr->GetUmtsSignalMetrics.ber, &indPtr->GetUmtsSignalMetrics.rscp);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetUmtsSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetUmtsSignalMetrics
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetUmtsSignalMetricsReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetUmtsSignalMetrics \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));

    indPtr->semRef = le_sem_Create("Ivss GetUmtsSignalMetricsSem", 0);
    indPtr->GetUmtsSignalMetrics.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetUmtsSignalMetricsEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(indPtr->GetUmtsSignalMetrics.ss, indPtr->GetUmtsSignalMetrics.ber,
        indPtr->GetUmtsSignalMetrics.rscp, ResultLeToIvssRadio(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetLteSignalMetrics'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetLteSignalMetricsHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;

    taf_radio_MetricsRef_t metrics =
        taf_radio_MeasureSignalMetrics(indPtr->GetLteSignalMetrics.phoneId);
    if (metrics == nullptr)
    {
        indPtr->result = LE_FAULT;
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_MeasureSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetLteSignalMetrics(metrics, &indPtr->GetLteSignalMetrics.ss,
        &indPtr->GetLteSignalMetrics.rsrq, &indPtr->GetLteSignalMetrics.rsrp,
        &indPtr->GetLteSignalMetrics.snr);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetLteSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetLteSignalMetrics
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetLteSignalMetricsReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetLteSignalMetrics \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));

    indPtr->semRef = le_sem_Create("Ivss GetLteSignalMetricsSem", 0);
    indPtr->GetLteSignalMetrics.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetLteSignalMetricsEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(indPtr->GetLteSignalMetrics.ss, indPtr->GetLteSignalMetrics.rsrq,
        indPtr->GetLteSignalMetrics.rsrp, indPtr->GetLteSignalMetrics.snr,
        ResultLeToIvssRadio(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetNr5gSignalMetrics'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNr5gSignalMetricsHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;

    taf_radio_MetricsRef_t metrics =
        taf_radio_MeasureSignalMetrics(indPtr->GetNr5gSignalMetrics.phoneId);
    if (metrics == nullptr)
    {
        indPtr->result = LE_FAULT;
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_MeasureSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetNr5gSignalMetrics(metrics, &indPtr->GetNr5gSignalMetrics.rsrq,
        &indPtr->GetNr5gSignalMetrics.rsrp, &indPtr->GetNr5gSignalMetrics.snr);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "GetNr5gSignalMetrics failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNr5gSignalMetrics
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetNr5gSignalMetricsReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetNr5gSignalMetrics \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));

    indPtr->semRef = le_sem_Create("Ivss GetNr5gSignalMetricsSem", 0);
    indPtr->GetNr5gSignalMetrics.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetNr5gSignalMetricsEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(indPtr->GetNr5gSignalMetrics.rsrq, indPtr->GetNr5gSignalMetrics.rsrp,
        indPtr->GetNr5gSignalMetrics.snr, ResultLeToIvssRadio(indPtr->result));

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
        indPtr->getRegisterMode.mcc, sizeof(indPtr->getRegisterMode.mcc),
        indPtr->getRegisterMode.mnc, sizeof(indPtr->getRegisterMode.mnc),
        indPtr->getRegisterMode.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetRegisterMode failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration mode.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRegisterMode
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetRegisterModeReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetRegisterMode \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetRegisterModeSem", 0);
    indPtr->getRegisterMode.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRegisterModeEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(indPtr->getRegisterMode.isManual, std::string(indPtr->getRegisterMode.mcc),
        std::string(indPtr->getRegisterMode.mnc), ResultLeToIvssRadio(indPtr->result));


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
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_SetAutomaticRegisterMode failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers to network using automatic mode.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetAutomaticRegisterMode
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    SetAutomaticRegisterModeReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc SetAutomaticRegisterMode \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss SetAutomaticRegisterModeSem", 0);
    indPtr->setAutomaticRegisterMode.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(SetAutomaticRegisterModeEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvssRadio(indPtr->result));

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
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetHardwareSimConfig failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_radio_GetHardwareSimRatCapabilities(
        &indPtr->getHardwareConfig.deviceRatCapMask, &indPtr->getHardwareConfig.simRatCapMask,
        indPtr->getHardwareConfig.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetHardwareSimRatCapabilities failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetHardwareConfig
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetHardwareConfigReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetHardwareConfig \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetHardwareConfigSem", 0);
    indPtr->getHardwareConfig.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetHardwareConfigEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply( indPtr->getHardwareConfig.totalSimCount, indPtr->getHardwareConfig.maxActiveSims,
        RatBitMaskRadioToIvss(indPtr->getHardwareConfig.deviceRatCapMask),
        RatBitMaskRadioToIvss(indPtr->getHardwareConfig.simRatCapMask),
        ResultLeToIvssRadio(indPtr->result));

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
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetRatPreferences failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT preferences.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetRatPreferences
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetRatPreferencesReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetRatPreferences \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetRatPreferencesSem", 0);
    indPtr->getRatPreferences.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRatPreferencesEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(RatBitMaskRadioToIvss(indPtr->getRatPreferences.ratMask),
        ResultLeToIvssRadio(indPtr->result));

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
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetCurrentNetworkLongName failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_radio_GetCurrentNetworkName(indPtr->getCurrentNetworkName.shortName,
        TAF_RADIO_NETWORK_NAME_MAX_LEN, indPtr->getCurrentNetworkName.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetCurrentNetworkName failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetCurrentNetworkName
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetCurrentNetworkNameReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetCurrentNetworkName \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetCurrentNetworkNameSem", 0);
    indPtr->getCurrentNetworkName.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetCurrentNetworkNameEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(std::string(indPtr->getCurrentNetworkName.longName),
        std::string(indPtr->getCurrentNetworkName.shortName), ResultLeToIvssRadio(indPtr->result));

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
    indPtr->result = taf_radio_GetRadioAccessTechInUse(&indPtr->getNetRegState.rat,
        indPtr->getNetRegState.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetRadioAccessTechInUse failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->getNetRegState.cellId = taf_radio_GetServingCellId(indPtr->getNetRegState.phoneId);
    if (indPtr->getNetRegState.cellId == UINT32_MAX)
    {
        indPtr->result = LE_FAULT;
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_GetServingCellId failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_radio_GetCurrentNetworkMccMnc(indPtr->getNetRegState.mcc,
        sizeof(indPtr->getNetRegState.mcc), indPtr->getNetRegState.mnc,
        sizeof(indPtr->getNetRegState.mnc), indPtr->getNetRegState.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetCurrentNetworkMccMnc failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_radio_GetNetRegState(&indPtr->getNetRegState.netReg,
        indPtr->getNetRegState.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetNetRegState failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNetRegState
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetNetRegStateReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetNetRegState \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetNetRegStateSem", 0);
    indPtr->getNetRegState.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetNetRegStateEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(RatRadioToIvss(indPtr->getNetRegState.rat), indPtr->getNetRegState.cellId,
        std::string(indPtr->getNetRegState.mcc), std::string(indPtr->getNetRegState.mnc),
        NetRegRadioToIvss(indPtr->getNetRegState.netReg), ResultLeToIvssRadio(indPtr->result));

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
    taf_radio_NREndcAvailability_t endcStatus;
    indPtr->result = taf_radio_GetNrDualConnectivityStatus(
        &endcStatus,
        &indPtr->getNrDualConnectivityStatus.statusDcnr,
        indPtr->getNrDualConnectivityStatus.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetNrDualConnectivityStatus failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetNrDualConnectivityStatus
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetNrDualConnectivityStatusReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetNrDualConnectivityStatus \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetNrDualConnectivityStatusSem", 0);
    indPtr->getNrDualConnectivityStatus.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetNrDualConnectivityStatusEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(NRDcnrRadioToIvss(indPtr->getNrDualConnectivityStatus.statusDcnr),
        ResultLeToIvssRadio(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'SetSignalStrengthReportingCriteria'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetSignalStrengthReportingCriteriaHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    switch (indPtr->setSignalStrengthReportingCriteria.ind.type)
    {
        case TAF_IVSS_RADIO_SIG_THRESHOLD:
            indPtr->result = taf_radio_SetSignalStrengthIndThresholds(
                indPtr->setSignalStrengthReportingCriteria.sigType,
                indPtr->setSignalStrengthReportingCriteria.ind.lowerRange,
                indPtr->setSignalStrengthReportingCriteria.ind.upperRange,
                indPtr->setSignalStrengthReportingCriteria.phoneId);
            TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                "taf_radio_SetSignalStrengthIndThresholds failed - %s",
                LE_RESULT_TXT(indPtr->result));
            break;
        case TAF_IVSS_RADIO_SIG_DELTA:
            indPtr->result = taf_radio_SetSignalStrengthIndDelta(
                indPtr->setSignalStrengthReportingCriteria.sigType,
                indPtr->setSignalStrengthReportingCriteria.ind.delta,
                indPtr->setSignalStrengthReportingCriteria.phoneId);
            TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                "taf_radio_SetSignalStrengthIndDelta failed - %s", LE_RESULT_TXT(indPtr->result));
            break;
        default:
            indPtr->result = LE_UNSUPPORTED;
            TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                "Unsupported signal type : %d.",
                (int32_t)indPtr->setSignalStrengthReportingCriteria.sigType);
    }

    if (indPtr->setSignalStrengthReportingCriteria.hyst.setThreshold)
    {
        indPtr->result = taf_radio_SetSignalStrengthIndHysteresis(
            indPtr->setSignalStrengthReportingCriteria.sigType,
            indPtr->setSignalStrengthReportingCriteria.hyst.threshold,
            indPtr->setSignalStrengthReportingCriteria.phoneId);
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_SetSignalStrengthIndHysteresis failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    if (indPtr->setSignalStrengthReportingCriteria.hyst.setTimer)
    {
        indPtr->result = taf_radio_SetSignalStrengthIndHysteresisTimer(
            indPtr->setSignalStrengthReportingCriteria.hyst.timer,
            indPtr->setSignalStrengthReportingCriteria.phoneId);
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_radio_SetSignalStrengthIndHysteresisTimer failed - %s",
            LE_RESULT_TXT(indPtr->result));
    }

    le_sem_Post(indPtr->semRef);;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets signal reporting criteria.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::SetSignalStrengthReportingCriteria
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    RadioSvcTypes::RadioSigTypeT _sigType,
    RadioSvcTypes::RadioSigStrengthIndicationT _ind,
    RadioSvcTypes::RadioSigStrengthHysteresisT _hyst,
    SetSignalStrengthReportingCriteriaReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc SetSignalStrengthReportingCriteria \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss SetSignalStrengthReportingCriteriaSem", 0);
    indPtr->setSignalStrengthReportingCriteria.phoneId = PhoneIdIvssRadioToUint8(_phoneId);
    indPtr->setSignalStrengthReportingCriteria.sigType = SigTypeIvssToRadio(_sigType);
    indPtr->setSignalStrengthReportingCriteria.ind.type = SigIndTypeIvssToRadio(_ind.getType());
    indPtr->setSignalStrengthReportingCriteria.ind.lowerRange = _ind.getLowerRange();
    indPtr->setSignalStrengthReportingCriteria.ind.upperRange = _ind.getUpperRange();
    indPtr->setSignalStrengthReportingCriteria.ind.delta = _ind.getDelta();
    indPtr->setSignalStrengthReportingCriteria.hyst.setThreshold = _hyst.getSetThreshold();
    indPtr->setSignalStrengthReportingCriteria.hyst.threshold = _hyst.getThreshold();
    indPtr->setSignalStrengthReportingCriteria.hyst.setTimer = _hyst.getSetTimer();
    indPtr->setSignalStrengthReportingCriteria.hyst.timer = _hyst.getTimer();

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(SetSignalStrengthReportingCriteriaEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvssRadio(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetPacketSwitchedState'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetPacketSwitchedStateHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)reportPtr;
    indPtr->result = taf_radio_GetPacketSwitchedState(&indPtr->getPacketSwitchedState.netState,
        indPtr->getPacketSwitchedState.phoneId);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_radio_GetPacketSwitchedState failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::GetPacketSwitchedState
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RadioSvcTypes::PhoneIdT _phoneId,
    GetPacketSwitchedStateReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssRadioSvc GetPacketSwitchedState \n");

    taf_IvssRadio_Ind_t* indPtr = (taf_IvssRadio_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssRadio_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetPacketSwitchedStateSem", 0);
    indPtr->getPacketSwitchedState.phoneId = PhoneIdIvssRadioToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetPacketSwitchedStateEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(NetRegRadioToIvss(indPtr->getPacketSwitchedState.netState),
        ResultLeToIvssRadio(indPtr->result));

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
    LE_DEBUG("tafIvssRadioSvc GsmSsChange Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        PhoneIdUint8ToIvssRadio(phoneId), RadioSvcTypes::RadioRatT::RADIO_RAT_T_GSM, ss, rsrp);
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
    LE_DEBUG("tafIvssRadioSvc GsmSsChange Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        PhoneIdUint8ToIvssRadio(phoneId), RadioSvcTypes::RadioRatT::RADIO_RAT_T_UMTS, ss, rsrp);
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
    LE_DEBUG("tafIvssRadioSvc LteSsChange Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        PhoneIdUint8ToIvssRadio(phoneId), RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE, ss, rsrp);
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
    LE_DEBUG("tafIvssRadioSvc Nr5gSsChange Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireSignalStrengthEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        PhoneIdUint8ToIvssRadio(phoneId), RadioSvcTypes::RadioRatT::RADIO_RAT_T_NR5G, ss, rsrp);
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for operating mode changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_StateChangeHandler
(
    taf_radio_OpMode_t mode, ///< [IN] Operating mode.
    void* contextPtr         ///< [IN] Handler context.
)
{
    LE_DEBUG("tafIvssRadioSvc StateChange Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireRadioStateEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        StatesRadioToIvss(mode));
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CellInfo changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssRadioSvc::taf_ivss_radio_CellInfoChangeHandler
(
    taf_radio_CellInfoStatus_t cellStatus, ///< [IN] Cell change enum for serving / neighbor / both.
    uint8_t phoneId,                       ///< [IN] Phone ID.
    void* contextPtr                       ///< [IN] Handler context.
)
{
    LE_DEBUG("tafIvssRadioSvc CellInfo Event");

    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    ivssRadio->fireCellInfoEvent(RadioSvcTypes::ValueState::VALUE_STATE_VALID,
        PhoneIdUint8ToIvssRadio(phoneId), CellInfoRadioToIvss(cellStatus));
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
    GetGsmSignalMetricsEvent = le_event_CreateIdWithRefCounting("GetGsmSignalMetricsEvent");
    GetUmtsSignalMetricsEvent = le_event_CreateIdWithRefCounting("GetUmtsSignalMetricsEvent");
    GetLteSignalMetricsEvent = le_event_CreateIdWithRefCounting("GetLteSignalMetricsEvent");
    GetNr5gSignalMetricsEvent = le_event_CreateIdWithRefCounting("GetNr5gSignalMetricsEvent");
    GetRegisterModeEvent = le_event_CreateIdWithRefCounting("GetRegisterModeEvent");
    SetAutomaticRegisterModeEvent = le_event_CreateIdWithRefCounting(
        "SetAutomaticRegisterModeEvent");
    GetHardwareConfigEvent = le_event_CreateIdWithRefCounting("GetHardwareConfigEvent");
    GetRatPreferencesEvent = le_event_CreateIdWithRefCounting("GetRatPreferencesEvent");
    GetCurrentNetworkNameEvent = le_event_CreateIdWithRefCounting("GetCurrentNetworkNameEvent");
    GetNetRegStateEvent = le_event_CreateIdWithRefCounting("GetNetRegStateEvent");
    GetNrDualConnectivityStatusEvent = le_event_CreateIdWithRefCounting(
        "GetNrDualConnectivityStatusEvent");
    SetSignalStrengthReportingCriteriaEvent = le_event_CreateIdWithRefCounting(
        "SetSignalStrengthReportingCriteriaEvent");
    GetPacketSwitchedStateEvent = le_event_CreateIdWithRefCounting("GetPacketSwitchedStateEvent");

    // Init event handler.
    GetGsmSignalMetricsEventHandlerRef = le_event_AddHandler("GetGsmSignalMetricsEvent Handler",
        GetGsmSignalMetricsEvent, tafIvssRadioSvc::GetGsmSignalMetricsHandler);
    GetUmtsSignalMetricsEventHandlerRef = le_event_AddHandler("GetUmtsSignalMetricsEvent Handler",
        GetUmtsSignalMetricsEvent, tafIvssRadioSvc::GetUmtsSignalMetricsHandler);
    GetLteSignalMetricsEventHandlerRef = le_event_AddHandler("GetLteSignalMetricsEvent Handler",
        GetLteSignalMetricsEvent, tafIvssRadioSvc::GetLteSignalMetricsHandler);
    GetNr5gSignalMetricsEventHandlerRef = le_event_AddHandler("GetNr5gSignalMetricsEvent Handler",
        GetNr5gSignalMetricsEvent, tafIvssRadioSvc::GetNr5gSignalMetricsHandler);
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
    SetSignalStrengthReportingCriteriaEventHandlerRef = le_event_AddHandler(
        "SetSignalStrengthReportingCriteriaEvent Handler", SetSignalStrengthReportingCriteriaEvent,
        tafIvssRadioSvc::SetSignalStrengthReportingCriteriaHandler);
    GetPacketSwitchedStateEventHandlerRef = le_event_AddHandler(
        "GetPacketSwitchedStateEvent Handler", GetPacketSwitchedStateEvent,
        tafIvssRadioSvc::GetPacketSwitchedStateHandler);

    // Init commonapi event.
    GsmSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_GSM,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_GsmSsChangeHandler, NULL);
    UmtsSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_UMTS,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_UmtsSsChangeHandler, NULL);
    LteSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_LteSsChangeHandler, NULL);
    Nr5gSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
        (taf_radio_SignalStrengthChangeHandlerFunc_t)taf_ivss_radio_Nr5gSsChangeHandler, NULL);
    StateChangeHandlerRef = taf_radio_AddOpModeChangeHandler(
        (taf_radio_OpModeChangeHandlerFunc_t)taf_ivss_radio_StateChangeHandler, NULL);
    CellInfoChangeHandlerRef = taf_radio_AddCellInfoChangeHandler(
        (taf_radio_CellInfoChangeHandlerFunc_t)taf_ivss_radio_CellInfoChangeHandler, NULL);

    LE_INFO("tafIvssRadioSvc Service initialized");
};
