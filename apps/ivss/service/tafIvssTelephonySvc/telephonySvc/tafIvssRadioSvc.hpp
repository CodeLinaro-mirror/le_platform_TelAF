/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSRADIOSVC_HPP_
#define TAFIVSSRADIOSVC_HPP_

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
 * Gets SIM maximum counts and RAT capabilities structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    uint8_t totalSimCount;              ///< [OUT] The max number of sims supported simultaneously.
    uint8_t maxActiveSims;              ///< [OUT] The max number of sims that can be active
    taf_radio_RatBitMask_t deviceRatCapMask;    ///< [OUT] Device rat capability bitmask.
    taf_radio_RatBitMask_t simRatCapMask;       ///< [OUT] Sim rat capability bitmask.
}taf_IvssRadio_GetHardwareConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT preferences structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                    ///< [IN] Phone ID.
    taf_radio_RatBitMask_t ratMask;     ///< [OUT] Device rat capability bitmask.
}taf_IvssRadio_GetRatPreferences_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the long name and short name of the network structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                    ///< [IN] Phone ID.
    char longName[TAF_RADIO_NETWORK_NAME_MAX_LEN];      ///< [OUT] Long network name.
    char shortName[TAF_RADIO_NETWORK_NAME_MAX_LEN];     ///< [OUT] Short network name.
}taf_IvssRadio_GetCurrentNetworkName_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration state structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                        ///< [IN] Phone ID.
    taf_radio_NetRegState_t netReg;         ///< [OUT] Network registration state.
}taf_IvssRadio_GetNetRegState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DCNR and ENDC mode status
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                ///< [IN] Phone ID.
    taf_radio_NREndcAvailability_t statusEndc;      ///< [OUT] Endc status.
    taf_radio_NRDcnrRestriction_t statusDcnr;       ///< [OUT] Dcnr status.
}taf_IvssRadio_GetNrDualConnectivityStatus_t;

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
        taf_IvssRadio_GetHardwareConfig_t getHardwareConfig;
        taf_IvssRadio_GetRatPreferences_t getRatPreferences;
        taf_IvssRadio_GetCurrentNetworkName_t getCurrentNetworkName;
        taf_IvssRadio_GetNetRegState_t getNetRegState;
        taf_IvssRadio_GetNrDualConnectivityStatus_t getNrDualConnectivityStatus;
    };
}taf_IvssRadio_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert rat type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
/**
 * Convert rat type from IVSS to radio
 */
//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
/**
 * Convert RAT bitmask type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::RatBitMask RatBitMaskRadioToIvss(taf_radio_RatBitMask_t ratBitMask)
{
    RadioSvc constObj;
    RadioSvc::RatBitMask ret = 0x0;
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        ret |= constObj.RAT_BIT_MASK_ALL;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_GSM)
    {
        ret |= constObj.RAT_BIT_MASK_GSM;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
    {
        ret |= constObj.RAT_BIT_MASK_UMTS;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_CDMA)
    {
        ret |= constObj.RAT_BIT_MASK_CDMA;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
    {
        ret |= constObj.RAT_BIT_MASK_TDSCDMA;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_LTE)
    {
        ret |= constObj.RAT_BIT_MASK_LTE;
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
    {
        ret |= constObj.RAT_BIT_MASK_NR5G;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert network registration state type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::NetRegState NetRegRadioToIvss(taf_radio_NetRegState_t netReg)
{
    RadioSvc::NetRegState ret = RadioSvc::NetRegState::NET_REG_STATE_UNKNOWN;
    switch (netReg)
    {
        case TAF_RADIO_NET_REG_STATE_NONE:
            ret = RadioSvc::NetRegState::NET_REG_STATE_NONE;
            break;
        case TAF_RADIO_NET_REG_STATE_HOME:
            ret = RadioSvc::NetRegState::NET_REG_STATE_HOME;
            break;
        case TAF_RADIO_NET_REG_STATE_SEARCHING:
            ret = RadioSvc::NetRegState::NET_REG_STATE_SEARCHING;
            break;
        case TAF_RADIO_NET_REG_STATE_DENIED:
            ret = RadioSvc::NetRegState::NET_REG_STATE_DENIED;
            break;
        case TAF_RADIO_NET_REG_STATE_ROAMING:
            ret = RadioSvc::NetRegState::NET_REG_STATE_ROAMING;
            break;
        case TAF_RADIO_NET_REG_STATE_UNKNOWN:
            ret = RadioSvc::NetRegState::NET_REG_STATE_UNKNOWN;
            break;
        case TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE:
            ret = RadioSvc::NetRegState::NET_REG_STATE_EMERGENCY_AVAILABLE;
            break;
        default:
            LE_ERROR("NetRegRadioToIvss : Unsupported input (%d)", static_cast<int>(netReg));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert NREndc availability status type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::NREndcAvailability NREndcRadioToIvss(taf_radio_NREndcAvailability_t statusEndc)
{
    RadioSvc::NREndcAvailability ret = RadioSvc::NREndcAvailability::NR_ENDC_UNKNOWN;
    switch (statusEndc)
    {
        case TAF_RADIO_NR_ENDC_UNKNOWN:
            ret = RadioSvc::NREndcAvailability::NR_ENDC_UNKNOWN;
            break;
        case TAF_RADIO_NR_ENDC_AVAILABLE:
            ret = RadioSvc::NREndcAvailability::NR_ENDC_AVAILABLE;
            break;
        case TAF_RADIO_NR_ENDC_UNAVAILABLE:
            ret = RadioSvc::NREndcAvailability::NR_ENDC_UNAVAILABLE;
            break;
        default:
            LE_ERROR("NREndcRadioToIvss : Unsupported input (%d)", static_cast<int>(statusEndc));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert NRDcnr restriction status type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::NRDcnrRestriction NRDcnrRadioToIvss(taf_radio_NRDcnrRestriction_t statusDcnr)
{
    RadioSvc::NRDcnrRestriction ret = RadioSvc::NRDcnrRestriction::NR_DCNR_UNKNOWN;
    switch (statusDcnr)
    {
        case TAF_RADIO_NR_DCNR_UNKNOWN:
            ret = RadioSvc::NRDcnrRestriction::NR_DCNR_UNKNOWN;
            break;
        case TAF_RADIO_NR_DCNR_RESTRICTED:
            ret = RadioSvc::NRDcnrRestriction::NR_DCNR_RESTRICTED;
            break;
        case TAF_RADIO_NR_DCNR_UNRESTRICTED:
            ret = RadioSvc::NRDcnrRestriction::NR_DCNR_UNRESTRICTED;
            break;
        default:
            LE_ERROR("NRDcnrRadioToIvss : Unsupported input (%d)", static_cast<int>(statusDcnr));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert states type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::States StatesRadioToIvss(taf_radio_OpMode_t mode)
{
    RadioSvc::States ret = RadioSvc::States::UNAVAILABLE;
    switch (mode)
    {
        case TAF_RADIO_OP_MODE_ONLINE:
            ret = RadioSvc::States::ON;
            break;
        case TAF_RADIO_OP_MODE_AIRPLANE:
        case TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER:
            ret = RadioSvc::States::OFF;
            break;
        case TAF_RADIO_OP_MODE_FACTORY_TEST:
        case TAF_RADIO_OP_MODE_OFFLINE:
        case TAF_RADIO_OP_MODE_RESETTING:
        case TAF_RADIO_OP_MODE_SHUTTING_DOWN:
            ret = RadioSvc::States::UNAVAILABLE;
            break;
        default:
            LE_ERROR("StatesRadioToIvss : Unsupported input (%d)", static_cast<int>(mode));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssRadioSvc: public v0_1::com::qualcomm::qti::modem::RadioSvcStubDefault
{
public:
    tafIvssRadioSvc() {};
    virtual ~tafIvssRadioSvc() {};

    // The initialization function of the Radio Service.
    void Init();

    static std::shared_ptr<tafIvssRadioSvc> GetInstance();

    // ivss method function.
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
    virtual void GetHardwareConfig(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetHardwareConfigReply_t _reply);
    virtual void GetRatPreferences(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetRatPreferencesReply_t _reply);
    virtual void GetCurrentNetworkName(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetCurrentNetworkNameReply_t _reply);
    virtual void GetNetRegState(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetNetRegStateReply_t _reply);
    virtual void GetNrDualConnectivityStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetNrDualConnectivityStatusReply_t _reply);

    // ivss method function handler.
    static void SetRadioPowerHandler(void* reportPtr);
    static void GetRadioPowerHandler(void* reportPtr);
    static void GetSignalStrengthHandler(void* reportPtr);
    static void GetRegisterModeHandler(void* reportPtr);
    static void SetAutomaticRegisterModeHandler(void* reportPtr);
    static void GetHardwareConfigHandler(void* reportPtr);
    static void GetRatPreferencesHandler(void* reportPtr);
    static void GetCurrentNetworkNameHandler(void* reportPtr);
    static void GetNetRegStateHandler(void* reportPtr);
    static void GetNrDualConnectivityStatusHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_ivss_radio_GsmSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_UmtsSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_TdscdmaSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_LteSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_Nr5gSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_RatChangeHandler(taf_radio_RatChangeInd_t* ratChangeIndPtr,
        void *contextPtr);
    static void taf_ivss_radio_StateChangeHandler(taf_radio_OpMode_t mode, void *contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t SetRadioPowerEvent = NULL;
    le_event_Id_t GetRadioPowerEvent = NULL;
    le_event_Id_t GetSignalStrengthEvent = NULL;
    le_event_Id_t GetRegisterModeEvent = NULL;
    le_event_Id_t SetAutomaticRegisterModeEvent = NULL;
    le_event_Id_t GetHardwareConfigEvent = NULL;
    le_event_Id_t GetRatPreferencesEvent = NULL;
    le_event_Id_t GetCurrentNetworkNameEvent = NULL;
    le_event_Id_t GetNetRegStateEvent = NULL;
    le_event_Id_t GetNrDualConnectivityStatusEvent = NULL;

    le_event_HandlerRef_t SetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetRadioPowerEventHandlerRef;
    le_event_HandlerRef_t GetSignalStrengthEventHandlerRef;
    le_event_HandlerRef_t GetRegisterModeEventHandlerRef;
    le_event_HandlerRef_t SetAutomaticRegisterModeEventHandlerRef;
    le_event_HandlerRef_t GetHardwareConfigEventHandlerRef;
    le_event_HandlerRef_t GetRatPreferencesEventHandlerRef;
    le_event_HandlerRef_t GetCurrentNetworkNameEventHandlerRef;
    le_event_HandlerRef_t GetNetRegStateEventHandlerRef;
    le_event_HandlerRef_t GetNrDualConnectivityStatusEventHandlerRef;

    // ivss event ref.
    taf_radio_RatChangeHandlerRef_t RatChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t GsmSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t UmtsSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t TdscdmaSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t LteSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t Nr5gSsChangeHandlerRef;
    taf_radio_OpModeChangeHandlerRef_t StateChangeHandlerRef;
};

#endif // TAFIVSSRADIOSVC_HPP_
