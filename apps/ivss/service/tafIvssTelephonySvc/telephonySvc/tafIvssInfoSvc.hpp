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
#include <v0/com/qualcomm/qti/modem/InfoSvcStubDefault.hpp>
#include <tafIvssCommon.hpp>

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the International Mobile Equipment Identity (IMEI) structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char imei[TAF_INFO_IMEI_MAX_BYTES]; ///< [OUT] IMEI number.
}taf_IvssInfo_GetImei_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss info method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef;                ///< [IN] Semaphore
    le_result_t result;                 ///< [OUT] The result
    union
    {
        taf_IvssInfo_GetImei_t getImei;
    };
}taf_IvssInfo_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssInfoSvc: public v0_1::com::qualcomm::qti::modem::InfoSvcStubDefault
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
