/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssRadioSvcImpl.hpp"

using namespace v0::com::qualcomm::qti::modem;

extern std::shared_ptr<tafIvssRadioSvcStubImpl> ivssRadioSvc;

// This function gets run by telaf thread.
void tafIvssRadioSvcStubImpl::SetRadioPowerHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)reportPtr;
        requestPtr->result = taf_radio_SetRadioPower(requestPtr->power, requestPtr->phoneId);
        if (requestPtr->result != LE_OK)
        {
            LE_ERROR("SetRadioPower failed - %s", LE_RESULT_TXT(requestPtr->result));
        }
        le_sem_Post(requestPtr->semRef);
    }
}

void tafIvssRadioSvcStubImpl::SetRadioPower(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, const CommonTypes::OnOffType _power,
        SetRadioPowerReply_t _reply)
{
    // Create a generic response message object.
    IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)le_mem_ForceAlloc(EventPool);
    memset(requestPtr, 0, sizeof(IvssRadioSvc_method_t));
    requestPtr->semRef = le_sem_Create("Ivss SetRadioPowerSem", 0);
    requestPtr->phoneId = PhoneIdIvssToUint8(_phoneId);
    requestPtr->power = OnoffIvssToLe(_power);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(SetRadioPowerEvent, (void*)requestPtr);
    le_sem_Wait(requestPtr->semRef);
    _reply(ResultLeToIvss(requestPtr->result));

    LE_DEBUG("tafIvssRadioSvc SetRadioPower: %s", ((requestPtr->power == LE_ON) ? "ON" : "OFF"));

    le_sem_Delete(requestPtr->semRef);
    le_mem_Release(requestPtr);
};

// This function gets run by telaf thread.
void tafIvssRadioSvcStubImpl::GetRadioPowerHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)reportPtr;
        requestPtr->result = taf_radio_GetRadioPower(&requestPtr->power, requestPtr->phoneId);
        if (requestPtr->result != LE_OK)
        {
            LE_ERROR("GetRadioPower failed - %s", LE_RESULT_TXT(requestPtr->result));
        }
        le_sem_Post(requestPtr->semRef);
    }
}

void tafIvssRadioSvcStubImpl::GetRadioPower(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetRadioPowerReply_t _reply)
{
    // Create a generic response message object.
    IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)le_mem_ForceAlloc(EventPool);
    memset(requestPtr, 0, sizeof(IvssRadioSvc_method_t));
    requestPtr->semRef = le_sem_Create("Ivss GetRadioPowerSem", 0);
    requestPtr->phoneId = PhoneIdIvssToUint8(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetRadioPowerEvent, (void*)requestPtr);
    le_sem_Wait(requestPtr->semRef);
    _reply(ResultLeToIvss(requestPtr->result), OnoffLeToIvss(requestPtr->power));

    LE_DEBUG("tafIvssRadioSvc GetRadioPower: %s", ((requestPtr->power == LE_ON) ? "ON" : "OFF"));

    le_sem_Delete(requestPtr->semRef);
    le_mem_Release(requestPtr);
};

// This function gets run by telaf thread.
void tafIvssRadioSvcStubImpl::GetSignalStrengthHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)reportPtr;

        taf_radio_MetricsRef_t metrics = taf_radio_MeasureSignalMetrics(requestPtr->phoneId);
        if (metrics == nullptr)
        {
            requestPtr->result = LE_FAULT;
            LE_ERROR("MeasureSignalMetrics failed - %s", LE_RESULT_TXT(requestPtr->result));
            le_sem_Post(requestPtr->semRef);
            return;
        }

        switch (requestPtr->rat)
        {
            case TAF_RADIO_RAT_GSM:
                uint32_t ber;
                requestPtr->result = taf_radio_GetGsmSignalMetrics(metrics, &requestPtr->ss, &ber);
                break;
            case TAF_RADIO_RAT_UMTS:
                uint32_t bler;
                int32_t rscp;
                requestPtr->result = taf_radio_GetUmtsSignalMetrics(metrics, &requestPtr->ss, &bler,
                    &rscp);
                break;
            case TAF_RADIO_RAT_LTE:
                int32_t rsrq;
                int32_t snr;
                requestPtr->result = taf_radio_GetLteSignalMetrics(metrics, &requestPtr->ss, &rsrq,
                    &requestPtr->rsrp, &snr);
                break;
            case TAF_RADIO_RAT_NR5G:
                requestPtr->result = taf_radio_GetNr5gSignalMetrics(metrics, &rsrq,
                    &requestPtr->rsrp, &snr);
                break;
            default:
                requestPtr->result = LE_BAD_PARAMETER;
                LE_ERROR("Invalid rat(%d) type", (int32_t)requestPtr->rat);
                le_sem_Post(requestPtr->semRef);
                return;
        }
        if (requestPtr->result != LE_OK)
        {
            LE_ERROR("GetSignalMetrics failed - %s", LE_RESULT_TXT(requestPtr->result));
        }
        le_sem_Post(requestPtr->semRef);
    }
}

void tafIvssRadioSvcStubImpl::GetSignalStrength(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, RadioSvc::Rat _rat, GetSignalStrengthReply_t _reply)
{
    // Create a generic response message object.
    IvssRadioSvc_method_t* requestPtr = (IvssRadioSvc_method_t*)le_mem_ForceAlloc(EventPool);
    memset(requestPtr, 0, sizeof(IvssRadioSvc_method_t));

    requestPtr->semRef = le_sem_Create("Ivss GetSignalStrengthSem", 0);
    requestPtr->phoneId = PhoneIdIvssToUint8(_phoneId);
    requestPtr->rat = RatIvssToRadio(_rat);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetSignalStrengthEvent, (void*)requestPtr);
    le_sem_Wait(requestPtr->semRef);
    _reply(ResultLeToIvss(requestPtr->result), requestPtr->ss, requestPtr->rsrp);
    //_reply(CommonTypes::Result::BAD_PARAMETER, requestPtr->ss, requestPtr->rsrp);

    le_sem_Delete(requestPtr->semRef);
    le_mem_Release(requestPtr);
};

tafIvssRadioSvcStubImpl& tafIvssRadioSvcStubImpl::GetInstance()
{
    static tafIvssRadioSvcStubImpl instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for GSM signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_GsmSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireSignalStrengthEvent(RadioSvc::Rat::RAT_GSM, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc GsmSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for UMTS signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_UmtsSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireSignalStrengthEvent(RadioSvc::Rat::RAT_UMTS, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc UmtsSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for TDSCDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_TdscdmaSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireSignalStrengthEvent(RadioSvc::Rat::RAT_TDSCDMA, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc TdscdmaSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for LTE signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_LteSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireSignalStrengthEvent(RadioSvc::Rat::RAT_LTE, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc LteSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_Nr5gSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireSignalStrengthEvent(RadioSvc::Rat::RAT_NR5G, ss, rsrp,
        PhoneIdUint8ToIvss(phoneId));
    LE_DEBUG("tafIvssRadioSvc Nr5gSsChange Event");
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void taf_ivss_radio_RatChangeHandler
(
    taf_radio_RatChangeInd_t* ratChangeIndPtr, ///< [IN] Indication on RAT change.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    ivssRadioSvc->fireRadioRatEvent(RatRadioToIvss(ratChangeIndPtr->rat),
        PhoneIdUint8ToIvss(ratChangeIndPtr->phoneId));
    LE_DEBUG("tafIvssRadioSvc RatChange Event");
};

void tafIvssRadioSvcStubImpl::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Radio EventPool", sizeof(IvssRadioSvc_method_t));

    // Init events.
    SetRadioPowerEvent = le_event_CreateIdWithRefCounting("SetRadioPowerEvent");
    GetRadioPowerEvent = le_event_CreateIdWithRefCounting("GetRadioPowerEvent");
    GetSignalStrengthEvent = le_event_CreateIdWithRefCounting("SetRadioPowerEvent");

    // Init event handler.
    SetRadioPowerEventHandlerRef = le_event_AddHandler("SetRadioPowerEvent Handler",
        SetRadioPowerEvent, tafIvssRadioSvcStubImpl::SetRadioPowerHandler);
    GetRadioPowerEventHandlerRef = le_event_AddHandler("GetRadioPowerEvent Handler",
        GetRadioPowerEvent, tafIvssRadioSvcStubImpl::GetRadioPowerHandler);
    GetSignalStrengthEventHandlerRef = le_event_AddHandler("GetSignalStrengthEvent Handler",
        GetSignalStrengthEvent, tafIvssRadioSvcStubImpl::GetSignalStrengthHandler);

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

    LE_INFO("tafIvssRadioSvc Service initialized");
};
