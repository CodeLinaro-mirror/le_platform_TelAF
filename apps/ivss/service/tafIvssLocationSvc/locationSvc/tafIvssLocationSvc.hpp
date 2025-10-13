/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFIVSSLOCATIONSVC_HPP_
#define TAFIVSSLOCATIONSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <tafIvssCommon.hpp>
#include <v1/com/qualcomm/qti/telephony/LocationSvcStubDefault.hpp>

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the location capabilities mask structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint64_t capabilitiesMask;  ///< [OUT] The GNSS capability information.
}taf_IvssLocation_GetCapabilities_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss location method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef; ///< [IN] Semaphore
    le_result_t result;  ///< [OUT] The result
    union
    {
        taf_IvssLocation_GetCapabilities_t getCapabilities;
    };
}taf_IvssLocation_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from le_result_t to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline LocationSvcTypes::TelephonyResultT ResultLeToIvssLocation(le_result_t result)
{
    LocationSvcTypes::TelephonyResultT ret =
        LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
    switch (result)
    {
        case LE_OK:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK;
            break;
        case LE_NOT_FOUND:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT;
            break;
        case LE_COMM_ERROR:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED;
            break;
        case LE_BUSY:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvssLocation : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from IVSS to le_result_t
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ResultIvssLocationToLe(LocationSvcTypes::TelephonyResultT result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK:
            ret = LE_OK;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT:
            ret = LE_FAULT;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED:
            ret = LE_CLOSED;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY:
            ret = LE_BUSY;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED:
            ret = LE_TERMINATED;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssLocationToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert location capabilities bitmask type from locGnss to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline uint64_t CapBitMaskLocToIvss(taf_locGnss_LocCapabilityType_t locCapMask)
{
    uint64_t ret = LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_UNKNOWN;

    if (locCapMask & TAF_LOCGNSS_TIME_BASED_TRACKING)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_TIME_BASED_TRACKING);
    }
    if (locCapMask & TAF_LOCGNSS_DISTANCE_BASED_TRACKING)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_DISTANCE_BASED_TRACKING);
    }
    if (locCapMask & TAF_LOCGNSS_GNSS_MEASUREMENTS)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_GNSS_MEASUREMENTS);
    }
    if (locCapMask & TAF_LOCGNSS_CONSTELLATION_ENABLEMENT)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_CONSTELLATION_ENABLEMENT);
    }
    if (locCapMask & TAF_LOCGNSS_CARRIER_PHASE)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_CARRIER_PHASE);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_GNSS_SINGLE_FREQUENCY)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_GNSS_SINGLE_FREQUENCY);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_GNSS_MULTI_FREQUENCY)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_GNSS_MULTI_FREQUENCY);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_VPE)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_VPE);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_CV2X_LOCATION_BASIC)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_CV2X_LOCATION_BASIC);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_CV2X_LOCATION_PREMIUM)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_CV2X_LOCATION_PREMIUM);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_PPE)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_PPE);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_QDR2)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_QDR2);
    }
    if (locCapMask & TAF_LOCGNSS_QWES_QDR3)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_QDR3);
    }
    if (locCapMask & TAF_LOCGNSS_TIME_BASED_BATCHING)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_TIME_BASED_BATCHING);
    }
    if (locCapMask & TAF_LOCGNSS_DISTANCE_BASED_BATCHING)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_DISTANCE_BASED_BATCHING);
    }
    if (locCapMask & TAF_LOCGNSS_OUTDOOR_TRIP_BATCHING)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_OUTDOOR_TRIP_BATCHING);
    }
    if (locCapMask & TAF_LOCGNSS_SV_POLYNOMIAL)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_SV_POLYNOMIAL);
    }
    if (locCapMask & TAF_LOCGNSS_NLOS_ML20)
    {
        ret |= static_cast<uint64_t>
            (LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_NLOS_ML20);
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssLocationSvc: public v1_0::com::qualcomm::qti::telephony::LocationSvcStubDefault
{
public:
    tafIvssLocationSvc() {};
    virtual ~tafIvssLocationSvc() {};

    // The initialization function of the Location Service.
    void Init();
    static std::shared_ptr<tafIvssLocationSvc> GetInstance();

    // ivss method function.
    virtual void GetCapabilities(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetCapabilitiesReply_t _reply);

    // ivss method function handler.
    static void GetCapabilitiesHandler(void* reportPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t GetCapabilitiesEvent = NULL;

    le_event_HandlerRef_t GetCapabilitiesEventHandlerRef;
};

#endif // TAFIVSSLOCATIONSVC_HPP_
