/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSSIMSVC_HPP_
#define TAFIVSSSIMSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/SimSvcStubDefault.hpp>
#include <tafIvssCommon.hpp>

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the IMSI for the SIM structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_sim_Id_t slotId;                ///< [IN] Slot ID
    char imsi[TAF_SIM_IMSI_BYTES];      ///< [OUT] IMSI as output.
}taf_IvssSim_GetImsi_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of the SIM card structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_sim_Id_t slotId;                ///< [IN] Slot ID
    taf_sim_States_t simState;          ///< [OUT] SIM card states
}taf_IvssSim_GetState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ivss sim method indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef;                ///< [IN] Semaphore
    le_result_t result;                 ///< [OUT] The result
    union
    {
        taf_IvssSim_GetImsi_t getImsi;
        taf_IvssSim_GetState_t getState;
    };
}taf_IvssSim_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert PhoneId type from sim to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline CommonTypes::PhoneId PhoneIdSimToIvss(taf_sim_Id_t slotId)
{
    CommonTypes::PhoneId ret = CommonTypes::PhoneId::PHONE_ID_1;
    switch (slotId)
    {
        case TAF_SIM_SLOT_ID_1:
            ret = CommonTypes::PhoneId::PHONE_ID_1;
            break;
        case TAF_SIM_SLOT_ID_2:
            ret = CommonTypes::PhoneId::PHONE_ID_2;
            break;
        default:
            LE_ERROR("PhoneIdSimToIvss : Unsupported input (%d)", static_cast<int>(slotId));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert PhoneId type from IVSS to sim
 */
//--------------------------------------------------------------------------------------------------
inline taf_sim_Id_t PhoneIdIvssToSim(CommonTypes::PhoneId phoneId)
{
    taf_sim_Id_t ret = TAF_SIM_SLOT_ID_1;
    switch (phoneId)
    {
        case CommonTypes::PhoneId::PHONE_ID_1:
            ret = TAF_SIM_SLOT_ID_1;
            break;
        case CommonTypes::PhoneId::PHONE_ID_2:
            ret = TAF_SIM_SLOT_ID_2;
            break;
        default:
            LE_ERROR("PhoneIdIvssToSim : Unsupported input (%d)", static_cast<int>(phoneId));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert states type from sim to IVSS
 */
//--------------------------------------------------------------------------------------------------
inline SimSvc::States StateSimToIvss(taf_sim_States_t state)
{
    SimSvc::States ret = SimSvc::States::PRESENT;
    switch (state)
    {
        case TAF_SIM_PRESENT:
            ret = SimSvc::States::PRESENT;
            break;
        case TAF_SIM_ABSENT:
            ret = SimSvc::States::ABSENT;
            break;
        case TAF_SIM_READY:
            ret = SimSvc::States::READY;
            break;
        case TAF_SIM_RESTRICTED:
            ret = SimSvc::States::RESTRICTED;
            break;
        case TAF_SIM_ERROR:
            ret = SimSvc::States::ERROR;
            break;
        case TAF_SIM_STATE_UNKNOWN:
            ret = SimSvc::States::STATE_UNKNOWN;
            break;
        default:
            LE_ERROR("StateSimToIvss : Unsupported input (%d)", static_cast<int>(state));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert states type from IVSS to sim
 */
//--------------------------------------------------------------------------------------------------
inline taf_sim_States_t StateIvssToSim(SimSvc::States state)
{
    taf_sim_States_t ret = TAF_SIM_PRESENT;
    switch (state)
    {
        case SimSvc::States::PRESENT:
            ret = TAF_SIM_PRESENT;
            break;
        case SimSvc::States::ABSENT:
            ret = TAF_SIM_ABSENT;
            break;
        case SimSvc::States::READY:
            ret = TAF_SIM_READY;
            break;
        case SimSvc::States::RESTRICTED:
            ret = TAF_SIM_RESTRICTED;
            break;
        case SimSvc::States::ERROR:
            ret = TAF_SIM_ERROR;
            break;
        case SimSvc::States::STATE_UNKNOWN:
            ret = TAF_SIM_STATE_UNKNOWN;
            break;
        default:
            LE_ERROR("StateIvssToSim : Unsupported input (%d)", static_cast<int>(state));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS sim service class
 */
//--------------------------------------------------------------------------------------------------
class tafIvssSimSvc: public v0_1::com::qualcomm::qti::modem::SimSvcStubDefault {
public:
    tafIvssSimSvc() {};
    virtual ~tafIvssSimSvc() {};

    // The initialization function of the Sim Service.
    void Init();

    static std::shared_ptr<tafIvssSimSvc> GetInstance();

    // ivss method function.
    virtual void GetImsi(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetImsiReply_t _reply);
    virtual void GetState(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetStateReply_t _reply);

    // ivss method function handler.
    static void GetImsiHandler(void* reportPtr);
    static void GetStateHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_Ivss_Sim_NewStateHandler(taf_sim_Id_t slotId,
        taf_sim_States_t state, void* contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t GetImsiEvent = NULL;
    le_event_Id_t GetStateEvent = NULL;

    le_event_HandlerRef_t GetImsiEventHandlerRef;
    le_event_HandlerRef_t GetStateEventHandlerRef;

    // ivss event ref.
    taf_sim_NewStateHandlerRef_t NewStateHandlerRef;
};

#endif // TAFIVSSSIMSVC_HPP_
