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
#include <tafIvssCommon.hpp>
#include <v2/com/qualcomm/qti/telephony/RadioSvcStubDefault.hpp>

using namespace v2::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * GSM signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId; ///< [IN] Phone ID.
    int32_t rssi;    ///< [OUT] Received signal strength indicator in dBm.
    uint32_t ber;    ///< [OUT] Bit error rate, valid from 0 to 7.
}taf_IvssRadio_GetGsmSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Umts signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId; ///< [IN] Phone ID.
    int32_t ss;      ///< [OUT] Signal strength in dBm.
    uint32_t ber;    ///< [OUT] WCDMA bit error rate, valid from 0 to 7, 0x7FFFFFFF is unavailable.
    int32_t rscp;    ///< [OUT] Receive signal channel power in dBm.
}taf_IvssRadio_GetUmtsSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Lte signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId; ///< [IN] Phone ID.
    int32_t ss;      ///< [OUT] Signal strength in dBm.
    int32_t rsrq;    ///< [OUT] Reference signal receive quality in dB.
    int32_t rsrp;    ///< [OUT] Reference signal receive power in dBm.
    int32_t snr;     ///< [OUT] Signal-to-noise ratio in units of 0.1 dB.
}taf_IvssRadio_GetLteSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Nr5g signal metrics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId; ///< [IN] Phone ID.
    int32_t rsrq;    ///< [OUT] Reference Signal Receive Quality in dB.
    int32_t rsrp;    ///< [OUT] Reference Signal Receive Power in dBm.
    int32_t snr;     ///< [OUT] Signal-to-Noise Ratio in units of 0.1 dB.
}taf_IvssRadio_GetNr5gSignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration mode structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;               ///< [IN] Phone ID.
    bool isManual;                 ///< [OUT] True if manual, false if automatic.
    char mcc[TAF_RADIO_MCC_BYTES]; ///< [OUT] MCC.
    char mnc[TAF_RADIO_MNC_BYTES]; ///< [OUT] MNC.
}taf_IvssRadio_GetRegisterMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Registers to network using automatic mode structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId; ///< [IN] Phone ID.
}taf_IvssRadio_SetAutomaticRegisterMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets SIM maximum counts and RAT capabilities structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                         ///< [IN] Phone ID.
    uint8_t totalSimCount;                   ///< [OUT] The max number of sims supported simultaneously.
    uint8_t maxActiveSims;                   ///< [OUT] The max number of sims that can be active
    taf_radio_RatBitMask_t deviceRatCapMask; ///< [OUT] Device rat capability bitmask.
    taf_radio_RatBitMask_t simRatCapMask;    ///< [OUT] Sim rat capability bitmask.
}taf_IvssRadio_GetHardwareConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT preferences structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                ///< [IN] Phone ID.
    taf_radio_RatBitMask_t ratMask; ///< [OUT] Device rat capability bitmask.
}taf_IvssRadio_GetRatPreferences_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the long name and short name of the network structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                                ///< [IN] Phone ID.
    char longName[TAF_RADIO_NETWORK_NAME_MAX_LEN];  ///< [OUT] Long network name.
    char shortName[TAF_RADIO_NETWORK_NAME_MAX_LEN]; ///< [OUT] Short network name.
}taf_IvssRadio_GetCurrentNetworkName_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network registration state structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                ///< [IN] Phone ID.
    taf_radio_Rat_t rat;            ///< [OUT] RAT in use.
    uint32_t cellId;                ///< [OUT] Cell ID.
    char mcc[TAF_RADIO_MCC_BYTES];  ///< [OUT] The mobile country code.
    char mnc[TAF_RADIO_MNC_BYTES];  ///< [OUT] The mobile network code
    taf_radio_NetRegState_t netReg; ///< [OUT] Network registration state.
}taf_IvssRadio_GetNetRegState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DCNR and ENDC mode status structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                          ///< [IN] Phone ID.
    taf_radio_NRDcnrRestriction_t statusDcnr; ///< [OUT] Dcnr status.
}taf_IvssRadio_GetNrDualConnectivityStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength indication type enum
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_IVSS_RADIO_SIG_THRESHOLD, ///< [IN] Report signal strength based on threshold.
    TAF_IVSS_RADIO_SIG_DELTA      ///< [IN] Report signal strength based on delta.
}taf_IvssRadio_SigIndicationType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_IvssRadio_SigIndicationType_t type; ///< [IN] Use threshold or delta as the indication.
    int32_t lowerRange;                     ///< [IN] Lower range threshold in 0.1 dBm.
    int32_t upperRange;                     ///< [IN] Upper range threshold in 0.1 dBm.
    uint16_t delta;                         ///< [IN] Delta in uints of 0.1 dBm.
}taf_IvssRadio_SigStrengthIndication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength reporting hysteresis structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool setThreshold;  ///< [IN] True if set a hysteresis threshold, false if not.
    uint16_t threshold; ///< [IN] Hysteresis dBm in units of 0.1 dBm, only to specify rat.
    bool setTimer;      ///< [IN] True if set a hysteresis timer, false if not.
    uint16_t timer;     ///< [IN] Hysteresis time in milliseconds, to all RATs.
}taf_IvssRadio_SigStrengthHysteresis_t;

//--------------------------------------------------------------------------------------------------
/**
 * Sets signal reporting criteria structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;                            ///< [IN] Phone ID.
    taf_radio_SigType_t sigType;                ///< [IN] Signal type.
    taf_IvssRadio_SigStrengthIndication_t ind;  ///< [IN] Signal strength indication..
    taf_IvssRadio_SigStrengthHysteresis_t hyst; ///< [IN] Signal strength hysteresis.
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
    le_sem_Ref_t semRef; ///< [IN] Semaphore
    le_result_t result;  ///< [OUT] The result
    union
    {
        taf_IvssRadio_GetGsmSignalMetrics_t GetGsmSignalMetrics;
        taf_IvssRadio_GetUmtsSignalMetrics_t GetUmtsSignalMetrics;
        taf_IvssRadio_GetLteSignalMetrics_t GetLteSignalMetrics;
        taf_IvssRadio_GetNr5gSignalMetrics_t GetNr5gSignalMetrics;
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
 * Convert PhoneId type from uint8 to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::PhoneIdT PhoneIdUint8ToIvssRadio(uint8_t phoneId)
{
    RadioSvcTypes::PhoneIdT ret = RadioSvcTypes::PhoneIdT::PHONE_ID_T_UNKNOWN;
    switch (phoneId)
    {
        case 1:
            ret = RadioSvcTypes::PhoneIdT::PHONE_ID_T_1;
            break;
        case 2:
            ret = RadioSvcTypes::PhoneIdT::PHONE_ID_T_2;
            break;
        default:
            LE_ERROR("PhoneIdUint8ToIvssRadio : Unsupported input (%d)", static_cast<int>(phoneId));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert PhoneId type from IVSS to uint8
 */
//--------------------------------------------------------------------------------------------------
static inline uint8_t PhoneIdIvssRadioToUint8(RadioSvcTypes::PhoneIdT phoneId)
{
    uint8_t ret = 1;
    switch (phoneId)
    {
        case RadioSvcTypes::PhoneIdT::PHONE_ID_T_1:
            ret = 1;
            break;
        case RadioSvcTypes::PhoneIdT::PHONE_ID_T_2:
            ret = 2;
            break;
        default:
            LE_ERROR("PhoneIdIvssRadioToUint8 : Unsupported input (%d)", static_cast<int>(phoneId));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from le_result_t to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::TelephonyResultT ResultLeToIvssRadio(le_result_t result)
{
    RadioSvcTypes::TelephonyResultT ret =
        RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
    switch (result)
    {
        case LE_OK:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK;
            break;
        case LE_NOT_FOUND:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT;
            break;
        case LE_COMM_ERROR:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED;
            break;
        case LE_BUSY:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvssRadio : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from IVSS to le_result_t
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ResultIvssRadioToLe(RadioSvcTypes::TelephonyResultT result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK:
            ret = LE_OK;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT:
            ret = LE_FAULT;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED:
            ret = LE_CLOSED;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY:
            ret = LE_BUSY;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED:
            ret = LE_TERMINATED;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssRadioToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert RAT bitmask type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline uint32_t RatBitMaskRadioToIvss(taf_radio_RatBitMask_t ratBitMask)
{
    uint32_t ret = RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_UNKNOWN;

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_GSM);
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_UMTS);
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_CDMA);
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_TDSCDMA);
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_LTE);
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_NR5G);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_GSM)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_GSM);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_UMTS);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_CDMA)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_CDMA);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_TDSCDMA);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_LTE)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_LTE);
    }
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
    {
        ret |= static_cast<uint32_t>
            (RadioSvcTypes::RadioRatBitMaskValueT::RADIO_RAT_BIT_MASK_VALUE_T_NR5G);
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert rat type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::RadioRatT RatRadioToIvss(taf_radio_Rat_t rat)
{
    RadioSvcTypes::RadioRatT ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_UNKNOWN;
    switch (rat)
    {
        case TAF_RADIO_RAT_UNKNOWN:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_UNKNOWN;
            break;
        case TAF_RADIO_RAT_GSM:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_GSM;
            break;
        case TAF_RADIO_RAT_GPRS:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_GPRS;
            break;
        case TAF_RADIO_RAT_EDGE:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_EDGE;
            break;
        case TAF_RADIO_RAT_EHRPD:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_EHRPD;
            break;
        case TAF_RADIO_RAT_UMTS:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_UMTS;
            break;
        case TAF_RADIO_RAT_HSPA:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSPA;
            break;
        case TAF_RADIO_RAT_HSDPA:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSDPA;
            break;
        case TAF_RADIO_RAT_HSUPA:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSUPA;
            break;
        case TAF_RADIO_RAT_HSPAP:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSPAP;
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_TDSCDMA;
            break;
        case TAF_RADIO_RAT_LTE:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE;
            break;
        case TAF_RADIO_RAT_LTE_CA:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE_CA;
            break;
        case TAF_RADIO_RAT_NR5G:
            ret = RadioSvcTypes::RadioRatT::RADIO_RAT_T_NR5G;
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
static inline taf_radio_Rat_t RatIvssToRadio(RadioSvcTypes::RadioRatT rat)
{
    taf_radio_Rat_t ret = TAF_RADIO_RAT_UNKNOWN;
    switch (rat)
    {
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_UNKNOWN:
            ret = TAF_RADIO_RAT_UNKNOWN;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_GSM:
            ret = TAF_RADIO_RAT_GSM;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_GPRS:
            ret = TAF_RADIO_RAT_GPRS;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_EDGE:
            ret = TAF_RADIO_RAT_EDGE;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_EHRPD:
            ret = TAF_RADIO_RAT_EHRPD;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_UMTS:
            ret = TAF_RADIO_RAT_UMTS;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSPA:
            ret = TAF_RADIO_RAT_HSPA;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSDPA:
            ret = TAF_RADIO_RAT_HSDPA;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSUPA:
            ret = TAF_RADIO_RAT_HSUPA;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_HSPAP:
            ret = TAF_RADIO_RAT_HSPAP;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_TDSCDMA:
            ret = TAF_RADIO_RAT_TDSCDMA;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE:
            ret = TAF_RADIO_RAT_LTE;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE_CA:
            ret = TAF_RADIO_RAT_LTE_CA;
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_NR5G:
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
 * Convert network registration state type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::RadioNetRegStateT NetRegRadioToIvss(taf_radio_NetRegState_t netReg)
{
    RadioSvcTypes::RadioNetRegStateT ret =
        RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_UNKNOWN;
    switch (netReg)
    {
        case TAF_RADIO_NET_REG_STATE_NONE:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_NONE;
            break;
        case TAF_RADIO_NET_REG_STATE_HOME:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_HOME;
            break;
        case TAF_RADIO_NET_REG_STATE_SEARCHING:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_SEARCHING;
            break;
        case TAF_RADIO_NET_REG_STATE_DENIED:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_DENIED;
            break;
        case TAF_RADIO_NET_REG_STATE_ROAMING:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_ROAMING;
            break;
        case TAF_RADIO_NET_REG_STATE_UNKNOWN:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_UNKNOWN;
            break;
        case TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE:
        case TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE:
            ret = RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_EMERGENCY_AVAILABLE;
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
static inline RadioSvcTypes::RadioNrDcnrRestrictionT NRDcnrRadioToIvss
(
    taf_radio_NRDcnrRestriction_t statusDcnr
)
{
    RadioSvcTypes::RadioNrDcnrRestrictionT ret =
        RadioSvcTypes::RadioNrDcnrRestrictionT::RADIO_NR_DCNR_RESTRICTION_T_UNKNOWN;
    switch (statusDcnr)
    {
        case TAF_RADIO_NR_DCNR_UNKNOWN:
            ret = RadioSvcTypes::RadioNrDcnrRestrictionT::RADIO_NR_DCNR_RESTRICTION_T_UNKNOWN;
            break;
        case TAF_RADIO_NR_DCNR_RESTRICTED:
            ret = RadioSvcTypes::RadioNrDcnrRestrictionT::RADIO_NR_DCNR_RESTRICTION_T_RESTRICTED;
            break;
        case TAF_RADIO_NR_DCNR_UNRESTRICTED:
            ret = RadioSvcTypes::RadioNrDcnrRestrictionT::RADIO_NR_DCNR_RESTRICTION_T_UNRESTRICTED;
            break;
        default:
            LE_ERROR("NRDcnrRadioToIvss : Unsupported input (%d)", static_cast<int>(statusDcnr));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert sig type from IVSS to radio
 */
//--------------------------------------------------------------------------------------------------
static inline taf_radio_SigType_t SigTypeIvssToRadio(RadioSvcTypes::RadioSigTypeT sigType)
{
    taf_radio_SigType_t ret = TAF_RADIO_SIG_TYPE_GSM_RSSI;
    switch (sigType)
    {
        case RadioSvcTypes::RadioSigTypeT::RADIO_SIG_TYPE_T_GSM_RSSI:
            ret = TAF_RADIO_SIG_TYPE_GSM_RSSI;
            break;
        case RadioSvcTypes::RadioSigTypeT::RADIO_SIG_TYPE_T_UMTS_RSSI:
            ret = TAF_RADIO_SIG_TYPE_UMTS_RSSI;
            break;
        case RadioSvcTypes::RadioSigTypeT::RADIO_SIG_TYPE_T_LTE_RSRP:
            ret = TAF_RADIO_SIG_TYPE_LTE_RSRP;
            break;
        case RadioSvcTypes::RadioSigTypeT::RADIO_SIG_TYPE_T_NR5G_RSRP:
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
static inline taf_IvssRadio_SigIndicationType_t SigIndTypeIvssToRadio
(
    RadioSvcTypes::RadioSigIndicationTypeT type
)
{
    taf_IvssRadio_SigIndicationType_t ret = TAF_IVSS_RADIO_SIG_THRESHOLD;
    switch (type)
    {
        case RadioSvcTypes::RadioSigIndicationTypeT::RADIO_SIG_INDICATION_TYPE_T_THRESHOLD:
            ret = TAF_IVSS_RADIO_SIG_THRESHOLD;
            break;
        case RadioSvcTypes::RadioSigIndicationTypeT::RADIO_SIG_INDICATION_TYPE_T_DELTA:
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
 * Convert states type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::RadioStateT StatesRadioToIvss(taf_radio_OpMode_t mode)
{
    RadioSvcTypes::RadioStateT ret = RadioSvcTypes::RadioStateT::RADIO_STATE_T_UNKNOWN;
    switch (mode)
    {
        case TAF_RADIO_OP_MODE_ONLINE:
            ret = RadioSvcTypes::RadioStateT::RADIO_STATE_T_ON;
            break;
        case TAF_RADIO_OP_MODE_AIRPLANE:
        case TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER:
            ret = RadioSvcTypes::RadioStateT::RADIO_STATE_T_OFF;
            break;
        case TAF_RADIO_OP_MODE_FACTORY_TEST:
        case TAF_RADIO_OP_MODE_OFFLINE:
        case TAF_RADIO_OP_MODE_RESETTING:
        case TAF_RADIO_OP_MODE_SHUTTING_DOWN:
            ret = RadioSvcTypes::RadioStateT::RADIO_STATE_T_UNAVAILABLE;
            break;
        default:
            LE_ERROR("StatesRadioToIvss : Unsupported input (%d)", static_cast<int>(mode));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Cell Info change status type from radio to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline RadioSvcTypes::RadioCellInfoStatusT CellInfoRadioToIvss
(
    taf_radio_CellInfoStatus_t status
)
{
    RadioSvcTypes::RadioCellInfoStatusT ret =
        RadioSvcTypes::RadioCellInfoStatusT::RADIO_CELL_INFO_STATUS_T_UNKNOWN;
    switch (status)
    {
        case TAF_RADIO_CELL_SERVING_CHANGED:
            ret = RadioSvcTypes::RadioCellInfoStatusT::RADIO_CELL_INFO_STATUS_T_SERVING_CHANGED;
            break;
        case TAF_RADIO_CELL_NEIGHBOR_CHANGED:
            ret = RadioSvcTypes::RadioCellInfoStatusT::RADIO_CELL_INFO_STATUS_T_NEIGHBOR_CHANGED;
            break;
        case TAF_RADIO_CELL_SERVING_AND_NEIGHBOR_CHANGED:
            ret = RadioSvcTypes::RadioCellInfoStatusT::
                RADIO_CELL_INFO_STATUS_T_SERVING_AND_NEIGHBOR_CHANGED;
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
class tafIvssRadioSvc: public v2_0::com::qualcomm::qti::telephony::RadioSvcStubDefault
{
public:
    tafIvssRadioSvc() {};
    virtual ~tafIvssRadioSvc() {};

    // The initialization function of the Radio Service.
    void Init();

    static std::shared_ptr<tafIvssRadioSvc> GetInstance();

    // ivss method function.
    virtual void GetGsmSignalMetrics(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetGsmSignalMetricsReply_t _reply);
    virtual void GetUmtsSignalMetrics(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetUmtsSignalMetricsReply_t _reply);
    virtual void GetLteSignalMetrics(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetLteSignalMetricsReply_t _reply);
    virtual void GetNr5gSignalMetrics(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetNr5gSignalMetricsReply_t _reply);
    virtual void GetRegisterMode(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetRegisterModeReply_t _reply);
    virtual void SetAutomaticRegisterMode(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, SetAutomaticRegisterModeReply_t _reply);
    virtual void GetHardwareConfig(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetHardwareConfigReply_t _reply);
    virtual void GetRatPreferences(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetRatPreferencesReply_t _reply);
    virtual void GetCurrentNetworkName(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetCurrentNetworkNameReply_t _reply);
    virtual void GetNetRegState(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetNetRegStateReply_t _reply);
    virtual void GetNrDualConnectivityStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetNrDualConnectivityStatusReply_t _reply);
    virtual void SetSignalStrengthReportingCriteria(
        const std::shared_ptr<CommonAPI::ClientId> _client, RadioSvcTypes::PhoneIdT _phoneId,
        RadioSvcTypes::RadioSigTypeT _sigType, RadioSvcTypes::RadioSigStrengthIndicationT _ind,
        RadioSvcTypes::RadioSigStrengthHysteresisT _hyst,
        SetSignalStrengthReportingCriteriaReply_t _reply);
    virtual void GetPacketSwitchedState(const std::shared_ptr<CommonAPI::ClientId> _client,
        RadioSvcTypes::PhoneIdT _phoneId, GetPacketSwitchedStateReply_t _reply);

    // ivss method function handler.
    static void GetGsmSignalMetricsHandler(void* reportPtr);
    static void GetUmtsSignalMetricsHandler(void* reportPtr);
    static void GetLteSignalMetricsHandler(void* reportPtr);
    static void GetNr5gSignalMetricsHandler(void* reportPtr);
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
    static void taf_ivss_radio_StateChangeHandler(taf_radio_OpMode_t mode, void *contextPtr);
    static void taf_ivss_radio_CellInfoChangeHandler(taf_radio_CellInfoStatus_t cellStatus,
        uint8_t phoneId, void* contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t GetGsmSignalMetricsEvent = NULL;
    le_event_Id_t GetUmtsSignalMetricsEvent = NULL;
    le_event_Id_t GetLteSignalMetricsEvent = NULL;
    le_event_Id_t GetNr5gSignalMetricsEvent = NULL;
    le_event_Id_t GetRegisterModeEvent = NULL;
    le_event_Id_t SetAutomaticRegisterModeEvent = NULL;
    le_event_Id_t GetHardwareConfigEvent = NULL;
    le_event_Id_t GetRatPreferencesEvent = NULL;
    le_event_Id_t GetCurrentNetworkNameEvent = NULL;
    le_event_Id_t GetNetRegStateEvent = NULL;
    le_event_Id_t GetNrDualConnectivityStatusEvent = NULL;
    le_event_Id_t SetSignalStrengthReportingCriteriaEvent = NULL;
    le_event_Id_t GetPacketSwitchedStateEvent = NULL;

    le_event_HandlerRef_t GetGsmSignalMetricsEventHandlerRef;
    le_event_HandlerRef_t GetUmtsSignalMetricsEventHandlerRef;
    le_event_HandlerRef_t GetLteSignalMetricsEventHandlerRef;
    le_event_HandlerRef_t GetNr5gSignalMetricsEventHandlerRef;
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
    taf_radio_SignalStrengthChangeHandlerRef_t GsmSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t UmtsSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t LteSsChangeHandlerRef;
    taf_radio_SignalStrengthChangeHandlerRef_t Nr5gSsChangeHandlerRef;
    taf_radio_OpModeChangeHandlerRef_t StateChangeHandlerRef;
    taf_radio_CellInfoChangeHandlerRef_t CellInfoChangeHandlerRef;
};

#endif // TAFIVSSRADIOSVC_HPP_
