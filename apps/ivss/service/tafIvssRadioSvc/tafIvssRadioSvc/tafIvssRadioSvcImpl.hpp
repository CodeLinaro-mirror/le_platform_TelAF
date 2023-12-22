/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSRADIOSVCSTUBIMPL_HPP_
#define TAFIVSSRADIOSVCSTUBIMPL_HPP_

#include "legato.h"
#include "interfaces.h"
#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/RadioSvcStubDefault.hpp>
#include <tafIvssCommon.hpp>

using namespace v0::com::qualcomm::qti::modem;

typedef struct
{
    le_sem_Ref_t semRef;    ///< Semaphore
    uint8_t phoneId;        ///< Phone ID.
    le_onoff_t power;       ///< Power state
    taf_radio_Rat_t rat;    ///< Radio Access Technology.
    int32_t ss;             ///< Signal strength in dBm.
    int32_t rsrp;           ///< Reference signal receive quality in dB.
    le_result_t result;     ///< The result
}IvssRadioSvc_method_t;

inline RadioSvc::Rat RatRadioToIvss(taf_radio_Rat_t rat)
{
    RadioSvc::Rat ret = RadioSvc::Rat::RAT_UNKNOWN;
    switch (rat)
    {
        case TAF_RADIO_RAT_UNKNOWN:
            ret = RadioSvc::Rat::RAT_UNKNOWN;
            break;
        case TAF_RADIO_RAT_GSM:
            ret = RadioSvc::Rat::RAT_GSM;
            break;
        case TAF_RADIO_RAT_GPRS:
            ret = RadioSvc::Rat::RAT_GPRS;
            break;
        case TAF_RADIO_RAT_EDGE:
            ret = RadioSvc::Rat::RAT_EDGE;
            break;
        case TAF_RADIO_RAT_EHRPD:
            ret = RadioSvc::Rat::RAT_EHRPD;
            break;
        case TAF_RADIO_RAT_UMTS:
            ret = RadioSvc::Rat::RAT_UMTS;
            break;
        case TAF_RADIO_RAT_HSPA:
            ret = RadioSvc::Rat::RAT_HSPA;
            break;
        case TAF_RADIO_RAT_HSDPA:
            ret = RadioSvc::Rat::RAT_HSDPA;
            break;
        case TAF_RADIO_RAT_HSUPA:
            ret = RadioSvc::Rat::RAT_HSUPA;
            break;
        case TAF_RADIO_RAT_HSPAP:
            ret = RadioSvc::Rat::RAT_HSPAP;
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            ret = RadioSvc::Rat::RAT_TDSCDMA;
            break;
        case TAF_RADIO_RAT_LTE:
            ret = RadioSvc::Rat::RAT_LTE;
            break;
        case TAF_RADIO_RAT_LTE_CA:
            ret = RadioSvc::Rat::RAT_LTE_CA;
            break;
        case TAF_RADIO_RAT_NR5G:
            ret = RadioSvc::Rat::RAT_NR5G;
            break;
        default:
            LE_ERROR("RatRadioToIvss : Unsupported input (%d)", static_cast<int>(rat));
            break;
    }
    return ret;
}

inline taf_radio_Rat_t RatIvssToRadio(RadioSvc::Rat rat)
{
    taf_radio_Rat_t ret = TAF_RADIO_RAT_UNKNOWN;
    switch (rat)
    {
        case RadioSvc::Rat::RAT_UNKNOWN:
            ret = TAF_RADIO_RAT_UNKNOWN;
            break;
        case RadioSvc::Rat::RAT_GSM:
            ret = TAF_RADIO_RAT_GSM;
            break;
        case RadioSvc::Rat::RAT_GPRS:
            ret = TAF_RADIO_RAT_GPRS;
            break;
        case RadioSvc::Rat::RAT_EDGE:
            ret = TAF_RADIO_RAT_EDGE;
            break;
        case RadioSvc::Rat::RAT_EHRPD:
            ret = TAF_RADIO_RAT_EHRPD;
            break;
        case RadioSvc::Rat::RAT_UMTS:
            ret = TAF_RADIO_RAT_UMTS;
            break;
        case RadioSvc::Rat::RAT_HSPA:
            ret = TAF_RADIO_RAT_HSPA;
            break;
        case RadioSvc::Rat::RAT_HSDPA:
            ret = TAF_RADIO_RAT_HSDPA;
            break;
        case RadioSvc::Rat::RAT_HSUPA:
            ret = TAF_RADIO_RAT_HSUPA;
            break;
        case RadioSvc::Rat::RAT_HSPAP:
            ret = TAF_RADIO_RAT_HSPAP;
            break;
        case RadioSvc::Rat::RAT_TDSCDMA:
            ret = TAF_RADIO_RAT_TDSCDMA;
            break;
        case RadioSvc::Rat::RAT_LTE:
            ret = TAF_RADIO_RAT_LTE;
            break;
        case RadioSvc::Rat::RAT_LTE_CA:
            ret = TAF_RADIO_RAT_LTE_CA;
            break;
        case RadioSvc::Rat::RAT_NR5G:
            ret = TAF_RADIO_RAT_NR5G;
            break;
        default:
            LE_ERROR("RatIvssToRadio : Unsupported input (%d)", static_cast<int>(rat));
            break;
    }
    return ret;
}

class tafIvssRadioSvcStubImpl: public v0_1::com::qualcomm::qti::modem::RadioSvcStubDefault
{
public:
    tafIvssRadioSvcStubImpl() {};
    virtual ~tafIvssRadioSvcStubImpl() {};

    void Init();
    // Static member functions.
    static tafIvssRadioSvcStubImpl &GetInstance();
    std::shared_ptr<tafIvssRadioSvcStubImpl> IvssSevice;

    virtual void SetRadioPower(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, CommonTypes::OnOffType _power, SetRadioPowerReply_t _reply);
    virtual void GetRadioPower(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetRadioPowerReply_t _reply);
    virtual void GetSignalStrength(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, RadioSvc::Rat _rat, GetSignalStrengthReply_t _reply);

    static void SetRadioPowerHandler(void* reportPtr);
    static void GetRadioPowerHandler(void* reportPtr);
    static void GetSignalStrengthHandler(void* reportPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // commonapi interface.
    le_event_Id_t SetRadioPowerEvent = NULL;
    le_event_Id_t GetRadioPowerEvent = NULL;
    le_event_Id_t GetSignalStrengthEvent = NULL;

    le_event_HandlerRef_t SetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetSignalStrengthEventHandlerRef;

    taf_radio_RatChangeHandlerRef_t RatChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t GsmSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t UmtsSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t TdscdmaSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t LteSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t Nr5gSsChangeHandlerRef;
};

#endif // TAFIVSSRADIOSVCSTUBIMPL_HPP_
