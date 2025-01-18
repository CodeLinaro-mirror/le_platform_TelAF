/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSMNGDCONNSVC_HPP_
#define TAFIVSSMNGDCONNSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <cstring>
#include <CommonAPI/CommonAPI.hpp>
#include <tafIvssCommon.hpp>
#include <v2/com/qualcomm/qti/telephony/MngdConnSvcStubDefault.hpp>

#define IVSS_MAX_DATA_NUM 4

using namespace v2::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Starts a data session for the given data name structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_MNGDCONN_MAX_NAME_LEN];       ///< The data name to use.
    taf_mngdConn_DataState_t state;             ///< The data state.
    bool startEnable;                           ///< True if called startData, false if not.
    bool handleEable;                           ///< True if add DataState handle, false if not.
    bool stateEnable;                           ///< True if received DataState event, false if not.
    bool stopEnable;                            ///< True if called sttopData, false if not.
}taf_IvssMngdConn_DataInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Starts a data session for the given data name structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_MNGDCONN_MAX_NAME_LEN];       ///< [IN] The data name to use.
}taf_IvssMngdConn_StartData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Stops a data session for the given data name structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_MNGDCONN_MAX_NAME_LEN];       ///< [IN] The data name to use.
}taf_IvssMngdConn_StopData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data Ipv4 address, gateway, DNS, and mask structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_MNGDCONN_MAX_NAME_LEN];        ///< [IN] The data name to use.
    char ifName[TAF_DCS_NAME_MAX_LEN];           ///< [OUT] The interface name.
    char ipAddr[TAF_DCS_IPV4_ADDR_MAX_LEN];      ///< [OUT] The Ipv4 address.
    char gatewayAddr[TAF_DCS_IPV4_ADDR_MAX_LEN]; ///< [OUT] The Ipv4 gateway address.
    char dns1Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];    ///< [OUT] The Ipv4 primary DNS address.
    char dns2Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];    ///< [OUT] The Ipv4 secondary DNS address.
    uint32_t ipMask;                             ///< [OUT] The Ipv4 mask.
}taf_IvssMngdConn_GetDataIpv4Info_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss mngd method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef;                ///< [IN] Semaphore
    le_result_t result;                 ///< [OUT] The result
    union
    {
        taf_IvssMngdConn_StartData_t startData;
        taf_IvssMngdConn_StopData_t stopData;
        taf_IvssMngdConn_GetDataIpv4Info_t getDataIpv4Info;
    };
}taf_IvssMngdConn_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from le_result_t to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline MngdConnSvcTypes::TelephonyResultT ResultLeToIvssMngdConn(le_result_t result)
{
    MngdConnSvcTypes::TelephonyResultT ret =
        MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
    switch (result)
    {
        case LE_OK:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK;
            break;
        case LE_NOT_FOUND:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT;
            break;
        case LE_COMM_ERROR:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED;
            break;
        case LE_BUSY:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvssMngdConn : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from IVSS to le_result_t
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ResultIvssMngdConnToLe(MngdConnSvcTypes::TelephonyResultT result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK:
            ret = LE_OK;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT:
            ret = LE_FAULT;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED:
            ret = LE_CLOSED;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY:
            ret = LE_BUSY;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED:
            ret = LE_TERMINATED;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssMngdConnToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert data state type from mngdConn to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline MngdConnSvcTypes::MngdConnDataStateT DataStateMngdConnToIvss(taf_mngdConn_DataState_t state)
{
    MngdConnSvcTypes::MngdConnDataStateT ret =
        MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_UNKNOWN;
    switch (state)
    {
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            ret = MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_DISCONNECTED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTED:
            ret = MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTION_FAILED:
            ret = MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTION_FAILED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTION_STALLED:
            ret = MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTION_STALLED;
            break;
        default:
            LE_ERROR("DataStateMngdConnToIvss : Unsupported input (%d)", static_cast<int>(state));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssMngdConnSvc: public v2_0::com::qualcomm::qti::telephony::MngdConnSvcStubDefault
{
public:
    tafIvssMngdConnSvc() {};
    virtual ~tafIvssMngdConnSvc() {};

    // The initialization function of the Mngd Service.
    void Init();
    static std::shared_ptr<tafIvssMngdConnSvc> GetInstance();

    // ivss method function.
    virtual void StartData(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _name,
        StartDataReply_t _reply);
    virtual void StopData(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _name,
        StopDataReply_t _reply);
    virtual void GetDataList(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetDataListReply_t _reply);
    virtual void GetDataIpv4Info(const std::shared_ptr<CommonAPI::ClientId> _client,
        std::string _name, GetDataIpv4InfoReply_t _reply);

    // ivss method function handler.
    static void StartDataHandler(void* reportPtr);
    static void StopDataHandler(void* reportPtr);
    static void GetDataIpv4InfoHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_ivss_mngdConn_DataStateHandler(taf_mngdConn_DataRef_t dataRef,
        taf_mngdConn_DataState_t dataState, void* contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t StartDataEvent = NULL;
    le_event_Id_t StopDataEvent = NULL;
    le_event_Id_t GetDataIpv4InfoEvent = NULL;

    le_event_HandlerRef_t StartDataEventHandlerRef;
    le_event_HandlerRef_t StopDataEventHandlerRef;
    le_event_HandlerRef_t GetDataIpv4InfoEventHandlerRef;
};

#endif // TAFIVSSMNGDCONNSVC_HPP_
