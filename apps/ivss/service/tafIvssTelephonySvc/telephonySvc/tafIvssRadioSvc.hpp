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
 * GSM signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t rssi;   ///< [OUT] Received signal strength indicator in dBm.
    uint32_t ber;   ///< [OUT] Bit error rate, valid from 0 to 7.
}taf_IvssRadio_GsmSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Umtssignal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t ss;     ///< [OUT] Signal strength in dBm.
    uint32_t ber;   ///< [OUT] WCDMA bit error rate, valid from 0 to 7, 0x7FFFFFFF is unavailable.
    int32_t rscp;   ///< [OUT] Receive signal channel power in dBm.
}taf_IvssRadio_UmtsSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Lte signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t ss;     ///< [OUT] Signal strength in dBm.
    int32_t rsrq;   ///< [OUT] Reference signal receive quality in dB.
    int32_t rsrp;   ///< [OUT] Reference signal receive power in dBm.
    int32_t snr;    ///< [OUT] Signal-to-noise ratio in units of 0.1 dB.
}taf_IvssRadio_LteSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Nr5g signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t rsrq;   ///< [OUT] Reference Signal Receive Quality in dB.
    int32_t rsrp;   ///< [OUT] Reference Signal Receive Power in dBm.
    int32_t snr;    ///< [OUT] Signal-to-Noise Ratio in units of 0.1 dB.
}taf_IvssRadio_Nr5gSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength union
 */
//--------------------------------------------------------------------------------------------------
typedef union
{
    taf_IvssRadio_GsmSignalMetrics_t gsm;       ///< [OUT] GSM signal metrics.
    taf_IvssRadio_UmtsSignalMetrics_t umts;     ///< [OUT] Umts signal metrics.
    taf_IvssRadio_LteSignalMetrics_t lte;       ///< [OUT] Lte signal metrics.
    taf_IvssRadio_Nr5gSignalMetrics_t nr5g;     ///< [OUT] Nr5g signal metrics.
}taf_IvssRadio_SignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                ///< [IN] Phone ID.
    taf_radio_Rat_t rat;                            ///< [IN] Radio Access Technology.
    taf_IvssRadio_SignalMetrics_t strength;         ///< [OUT] Signal metrics for the current rat.
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
    taf_radio_Rat_t rat;                    ///< [OUT] RAT in use.
    uint32_t cellId;                        ///< [OUT] Cell ID.
    char mcc[TAF_RADIO_MCC_BYTES];          ///< [OUT] The mobile country code.
    char mnc[TAF_RADIO_MNC_BYTES];          ///< [OUT] The mobile network code
    taf_radio_NetRegState_t netReg;         ///< [OUT] Network registration state.
}taf_IvssRadio_GetNetRegState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DCNR and ENDC mode status structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                ///< [IN] Phone ID.
    taf_radio_NRDcnrRestriction_t statusDcnr;       ///< [OUT] Dcnr status.
}taf_IvssRadio_GetNrDualConnectivityStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength indication type enum
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_IVSS_RADIO_SIG_THRESHOLD,       ///< [IN] Report signal strength based on threshold.
    TAF_IVSS_RADIO_SIG_DELTA            ///< [IN] Report signal strength based on delta.
}taf_IvssRadio_SigIndicationType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_IvssRadio_SigIndicationType_t type;     ///< [IN] Use threshold or delta as the indication.
    int32_t lowerRange;                         ///< [IN] Lower range threshold in 0.1 dBm.
    int32_t upperRange;                         ///< [IN] Upper range threshold in 0.1 dBm.
    uint16_t delta;                             ///< [IN] Delta in uints of 0.1 dBm.
}taf_IvssRadio_SigStrengthIndication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength reporting hysteresis structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool setThreshold;      ///< [IN] True if set a hysteresis threshold, false if not.
    uint16_t threshold;     ///< [IN] Hysteresis dBm in units of 0.1 dBm, only to specify rat.
    bool setTimer;          ///< [IN] True if set a hysteresis timer, false if not.
    uint16_t timer;         ///< [IN] Hysteresis time in milliseconds, to all RATs.
}taf_IvssRadio_SigStrengthHysteresis_t;

//--------------------------------------------------------------------------------------------------
/**
 * Sets signal reporting criteria structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                ///< [IN] Phone ID.
    taf_radio_SigType_t sigType;                    ///< [IN] Signal type.
    taf_IvssRadio_SigStrengthIndication_t ind;      ///< [IN] Signal strength indication..
    taf_IvssRadio_SigStrengthHysteresis_t hyst;     ///< [IN] Signal strength hysteresis.
}taf_IvssRadio_SetSignalStrengthReportingCriteria_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the packet switch state structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                  ///< [IN] Phone ID.
    taf_radio_NetRegState_t netState; ///< [OUT] Packet switch state.
}taf_IvssRadio_GetPacketSwitchedState_t;

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
        taf_IvssRadio_SetSignalStrengthReportingCriteria_t setSignalStrengthReportingCriteria;
        taf_IvssRadio_GetPacketSwitchedState_t getPacketSwitchedState;
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
    RadioSvc::RatBitMask ret = 0x0;
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_ALL);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_GSM)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_GSM);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_UMTS);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_CDMA)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_CDMA);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_TDSCDMA);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_LTE)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_LTE);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
    {
        ret |= static_cast<uint32_t>(RadioSvc::RatBitMaskValue::RAT_BIT_MASK_NR5G);
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
 * Convert sig type from IVSS to radio
 */
//--------------------------------------------------------------------------------------------------
inline taf_radio_SigType_t SigTypeIvssToRadio(RadioSvc::SigType sigType)
{
    taf_radio_SigType_t ret = TAF_RADIO_SIG_TYPE_GSM_RSSI;
    switch (sigType)
    {
        case RadioSvc::SigType::SIG_TYPE_GSM_RSSI:
            ret = TAF_RADIO_SIG_TYPE_GSM_RSSI;
            break;
        case RadioSvc::SigType::SIG_TYPE_UMTS_RSSI:
            ret = TAF_RADIO_SIG_TYPE_UMTS_RSSI;
            break;
        case RadioSvc::SigType::SIG_TYPE_LTE_RSRP:
            ret = TAF_RADIO_SIG_TYPE_LTE_RSRP;
            break;
        case RadioSvc::SigType::SIG_TYPE_NR5G_RSRP:
            ret = TAF_RADIO_SIG_TYPE_NR5G_RSRP;
            break;
        default:
            LE_ERROR("SigTypeIvssToRadio : Unsupported input (%d)", static_cast<int>(sigType));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert signal strength indication type from IVSS to radio
 */
//--------------------------------------------------------------------------------------------------
inline taf_IvssRadio_SigIndicationType_t SigIndTypeIvssToRadio(RadioSvc::SigIndicationType type)
{
    taf_IvssRadio_SigIndicationType_t ret = TAF_IVSS_RADIO_SIG_THRESHOLD;
    switch (type)
    {
        case RadioSvc::SigIndicationType::SIG_THRESHOLD:
            ret = TAF_IVSS_RADIO_SIG_THRESHOLD;
            break;
        case RadioSvc::SigIndicationType::SIG_DELTA:
            ret = TAF_IVSS_RADIO_SIG_DELTA;
            break;
        default:
            LE_ERROR("SigIndTypeIvssToRadio : Unsupported input (%d)", static_cast<int>(type));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Cell Info change status type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline RadioSvc::CellInfoStatus CellInfoRadioToIvss(taf_radio_CellInfoStatus_t status)
{
    RadioSvc::CellInfoStatus ret = RadioSvc::CellInfoStatus::CELL_SERVING_CHANGED;
    switch (status)
    {
        case TAF_RADIO_CELL_SERVING_CHANGED:
            ret = RadioSvc::CellInfoStatus::CELL_SERVING_CHANGED;
            break;
        case TAF_RADIO_CELL_NEIGHBOR_CHANGED:
            ret = RadioSvc::CellInfoStatus::CELL_NEIGHBOR_CHANGED;
            break;
        case TAF_RADIO_CELL_SERVING_AND_NEIGHBOR_CHANGED:
            ret = RadioSvc::CellInfoStatus::CELL_SERVING_AND_NEIGHBOR_CHANGED;
            break;
        default:
            LE_ERROR("CellInfoRadioToIvss : Unsupported input (%d)", static_cast<int>(status));
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
    virtual void SetSignalStrengthReportingCriteria(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, RadioSvc::SigType _sigType,
        RadioSvc::SigStrengthIndication _ind, RadioSvc::SigStrengthHysteresis _hyst,
        SetSignalStrengthReportingCriteriaReply_t _reply);
    virtual void GetPacketSwitchedState(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetPacketSwitchedStateReply_t _reply);

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
    static void SetSignalStrengthReportingCriteriaHandler(void* reportPtr);
    static void GetPacketSwitchedStateHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_ivss_radio_GsmSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_UmtsSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_LteSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_Nr5gSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId,
        void* contextPtr);
    static void taf_ivss_radio_RatChangeHandler(taf_radio_RatChangeInd_t* ratChangeIndPtr,
        void *contextPtr);
    static void taf_ivss_radio_StateChangeHandler(taf_radio_OpMode_t mode, void *contextPtr);
    static void taf_ivss_radio_CellInfoChangeHandler(taf_radio_CellInfoStatus_t cellStatus,
        uint8_t phoneId, void* contextPtr);

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
    le_event_Id_t SetSignalStrengthReportingCriteriaEvent = NULL;
    le_event_Id_t GetPacketSwitchedStateEvent = NULL;

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
    le_event_HandlerRef_t SetSignalStrengthReportingCriteriaEventHandlerRef;
    le_event_HandlerRef_t GetPacketSwitchedStateEventHandlerRef;

    // ivss event ref.
    taf_radio_RatChangeHandlerRef_t RatChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t GsmSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t UmtsSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t LteSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t Nr5gSsChangeHandlerRef;
    taf_radio_OpModeChangeHandlerRef_t StateChangeHandlerRef;
    taf_radio_CellInfoChangeHandlerRef_t CellInfoChangeHandlerRef;
};

#endif // TAFIVSSRADIOSVC_HPP_
