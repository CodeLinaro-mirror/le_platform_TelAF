/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSRADIOSVCSTUBIMPL_HPP_
#define TAFIVSSRADIOSVCSTUBIMPL_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/RadioSvcStubDefault.hpp>
#include <tafIvssCommon.hpp>

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power state structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    le_onoff_t power;                   ///< [IN] Power state
}taf_IvssRadio_SetRadioPower_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power state structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    le_onoff_t power;                   ///< [OUT] Power state
}taf_IvssRadio_GetRadioPower_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    taf_radio_Rat_t rat;                ///< [IN] Radio Access Technology.
    int32_t ss;                         ///< [OUT] Signal strength in dBm.
    int32_t rsrp;                       ///< [OUT] Reference signal receive quality in dB.
}taf_IvssRadio_GetSignalStrength_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration mode structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    bool isManual;                      ///< [OUT] True if manual, false if automatic.
    char mcc[TAF_RADIO_MCC_BYTES];      ///< [OUT] MCC.
    char mnc[TAF_RADIO_MNC_BYTES];      ///< [OUT] MNC.
}taf_IvssRadio_GetRegisterMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Registers to network using automatic mode structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
}taf_IvssRadio_SetAutomaticRegisterMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss radio method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef;                ///< [IN] Semaphore
    le_result_t result;                 ///< [OUT] The result
    union
    {
        taf_IvssRadio_SetRadioPower_t setRadioPower;
        taf_IvssRadio_GetRadioPower_t getRadioPower;
        taf_IvssRadio_GetSignalStrength_t getSignalStrength;
        taf_IvssRadio_GetRegisterMode_t getRegisterMode;
        taf_IvssRadio_SetAutomaticRegisterMode_t setAutomaticRegisterMode;
    };
}taf_IvssRadio_Ind_t;

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
    virtual void GetRegisterMode(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetRegisterModeReply_t _reply);
    virtual void SetAutomaticRegisterMode(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, SetAutomaticRegisterModeReply_t _reply);

    static void SetRadioPowerHandler(void* reportPtr);
    static void GetRadioPowerHandler(void* reportPtr);
    static void GetSignalStrengthHandler(void* reportPtr);
    static void GetRegisterModeHandler(void* reportPtr);
    static void SetAutomaticRegisterModeHandler(void* reportPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // commonapi interface.
    le_event_Id_t SetRadioPowerEvent = NULL;
    le_event_Id_t GetRadioPowerEvent = NULL;
    le_event_Id_t GetSignalStrengthEvent = NULL;
    le_event_Id_t GetRegisterModeEvent = NULL;
    le_event_Id_t SetAutomaticRegisterModeEvent = NULL;

    le_event_HandlerRef_t SetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetSignalStrengthEventHandlerRef;
    le_event_HandlerRef_t GetRegisterModeEventHandlerRef;
    le_event_HandlerRef_t SetAutomaticRegisterModeEventHandlerRef;

    taf_radio_RatChangeHandlerRef_t RatChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t GsmSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t UmtsSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t TdscdmaSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t LteSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t Nr5gSsChangeHandlerRef;
};

#endif // TAFIVSSRADIOSVCSTUBIMPL_HPP_
