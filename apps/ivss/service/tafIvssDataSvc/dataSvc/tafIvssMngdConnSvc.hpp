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
#include <v0/com/qualcomm/qti/modem/MngdConnSvcStubDefault.hpp>
#include <tafIvssCommon.hpp>

#define IVSS_MAX_DATA_NUM 4

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Starts a data session for the given data name structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_MNGDCONN_MAX_NAME_LEN];       ///< The data name to use.
    bool enable;                                ///< True if start, false if stop.
    taf_mngdConn_DataState_t state;             ///< The data state.
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
    };
}taf_IvssMngdConn_Ind_t;


//--------------------------------------------------------------------------------------------------
/**
 * Convert data state type from mngdConn to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline MngdConnSvc::DataState DataStateMngdConnToIvss(taf_mngdConn_DataState_t state)
{
    MngdConnSvc::DataState ret = MngdConnSvc::DataState::DATA_DISCONNECTED;
    switch (state)
    {
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            ret = MngdConnSvc::DataState::DATA_DISCONNECTED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTED:
            ret = MngdConnSvc::DataState::DATA_CONNECTED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTION_FAILED:
            ret = MngdConnSvc::DataState::DATA_CONNECTION_FAILED;
            break;
        case TAF_MNGDCONN_DATA_CONNECTION_STALLED:
            ret = MngdConnSvc::DataState::DATA_CONNECTION_STALLED;
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
class tafIvssMngdConnSvc: public v0_1::com::qualcomm::qti::modem::MngdConnSvcStubDefault
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

    // ivss method function handler.
    static void StartDataHandler(void* reportPtr);
    static void StopDataHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_ivss_mngdConn_DataStateHandler(taf_mngdConn_DataRef_t dataRef,
        taf_mngdConn_DataState_t dataState, void* contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t StartDataEvent = NULL;
    le_event_Id_t StopDataEvent = NULL;

    le_event_HandlerRef_t StartDataEventHandlerRef;
    le_event_HandlerRef_t StopDataEventHandlerRef;
};

#endif // TAFIVSSMNGDCONNSVC_HPP_
