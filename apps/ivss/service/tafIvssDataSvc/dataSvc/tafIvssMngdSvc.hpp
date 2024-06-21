/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSMNGDSVC_HPP_
#define TAFIVSSMNGDSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/MngdSvcStubDefault.hpp>

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss mngd method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef;                ///< [IN] Semaphore
    le_result_t result;                 ///< [OUT] The result
}taf_IvssMngd_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * IVSS radio service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssMngdSvc: public v0_1::com::qualcomm::qti::modem::MngdSvcStubDefault
{
public:
    tafIvssMngdSvc() {};
    virtual ~tafIvssMngdSvc() {};

    // The initialization function of the Mngd Service.
    void Init();
    static std::shared_ptr<tafIvssMngdSvc> GetInstance();

    // memory pools.
    le_mem_PoolRef_t EventPool;
};

#endif // TAFIVSSMNGDSVC_HPP_
