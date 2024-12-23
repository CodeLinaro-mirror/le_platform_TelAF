/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSINFOSVC_HPP_
#define TAFIVSSINFOSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <tafIvssCommon.hpp>
#include <v1/com/qualcomm/qti/telephony/InfoSvcStubDefault.hpp>

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the International Mobile Equipment Identity (IMEI) structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char imei[TAF_DEVINFO_IMEI_MAX_BYTES]; ///< [OUT] IMEI number.
}taf_IvssInfo_GetImei_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss info method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef; ///< [IN] Semaphore
    le_result_t result;  ///< [OUT] The result
    union
    {
        taf_IvssInfo_GetImei_t getImei;
    };
}taf_IvssInfo_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from le_result_t to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline InfoSvcTypes::TelephonyResultT ResultLeToIvssInfo(le_result_t result)
{
    InfoSvcTypes::TelephonyResultT ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
    switch (result)
    {
        case LE_OK:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK;
            break;
        case LE_NOT_FOUND:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT;
            break;
        case LE_COMM_ERROR:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED;
            break;
        case LE_BUSY:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvssInfo : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from IVSS to le_result_t
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ResultIvssInfoToLe(InfoSvcTypes::TelephonyResultT result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK:
            ret = LE_OK;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT:
            ret = LE_FAULT;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED:
            ret = LE_CLOSED;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY:
            ret = LE_BUSY;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED:
            ret = LE_TERMINATED;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssInfoToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssInfoSvc: public v1_0::com::qualcomm::qti::telephony::InfoSvcStubDefault
{
public:
    tafIvssInfoSvc() {};
    virtual ~tafIvssInfoSvc() {};

    // The initialization function of the Info Service.
    void Init();
    static std::shared_ptr<tafIvssInfoSvc> GetInstance();

    // ivss method function.
    virtual void GetImei(const std::shared_ptr<CommonAPI::ClientId> _client, GetImeiReply_t _reply);

    // ivss method function handler.
    static void GetImeiHandler(void* reportPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t GetImeiEvent = NULL;

    le_event_HandlerRef_t GetImeiEventHandlerRef;
};

#endif // TAFIVSSINFOSVC_HPP_
