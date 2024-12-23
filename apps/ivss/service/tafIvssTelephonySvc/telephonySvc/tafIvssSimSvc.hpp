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
#include <tafIvssCommon.hpp>
#include <v1/com/qualcomm/qti/telephony/SimSvcStubDefault.hpp>

using namespace v1::com::qualcomm::qti::telephony;

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
 * Retrieves the SIM's ICCID structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_sim_Id_t slotId;                ///< [IN] Slot ID
    char iccid[TAF_SIM_ICCID_BYTES];              ///< [OUT] ICC ID as output.
}taf_IvssSim_GetIccid_t;

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
        taf_IvssSim_GetIccid_t GetIccid;
    };
}taf_IvssSim_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * Convert PhoneId type from sim to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline SimSvcTypes::PhoneIdT PhoneIdSimToIvss(taf_sim_Id_t slotId)
{
    SimSvcTypes::PhoneIdT ret = SimSvcTypes::PhoneIdT::PHONE_ID_T_UNKNOWN;
    switch (slotId)
    {
        case TAF_SIM_SLOT_ID_1:
            ret = SimSvcTypes::PhoneIdT::PHONE_ID_T_1;
            break;
        case TAF_SIM_SLOT_ID_2:
            ret = SimSvcTypes::PhoneIdT::PHONE_ID_T_2;
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
static inline taf_sim_Id_t PhoneIdIvssToSim(SimSvcTypes::PhoneIdT phoneId)
{
    taf_sim_Id_t ret = TAF_SIM_SLOT_ID_1;
    switch (phoneId)
    {
        case SimSvcTypes::PhoneIdT::PHONE_ID_T_1:
            ret = TAF_SIM_SLOT_ID_1;
            break;
        case SimSvcTypes::PhoneIdT::PHONE_ID_T_2:
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
 * Convert Result type from le_result_t to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline SimSvcTypes::TelephonyResultT ResultLeToIvssSim(le_result_t result)
{
    SimSvcTypes::TelephonyResultT ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
    switch (result)
    {
        case LE_OK:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK;
            break;
        case LE_NOT_FOUND:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT;
            break;
        case LE_COMM_ERROR:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED;
            break;
        case LE_BUSY:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvssSim : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Result type from IVSS to le_result_t
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ResultIvssSimToLe(SimSvcTypes::TelephonyResultT result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK:
            ret = LE_OK;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FAULT:
            ret = LE_FAULT;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_CLOSED:
            ret = LE_CLOSED;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_BUSY:
            ret = LE_BUSY;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_TERMINATED:
            ret = LE_TERMINATED;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssSimToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert states type from sim to IVSS
 */
//--------------------------------------------------------------------------------------------------
static inline SimSvcTypes::SimStateT StateSimToIvss(taf_sim_States_t state)
{
    SimSvcTypes::SimStateT ret = SimSvcTypes::SimStateT::SIM_STATE_T_UNKNOWN;
    switch (state)
    {
        case TAF_SIM_PRESENT:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_PRESENT;
            break;
        case TAF_SIM_ABSENT:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_ABSENT;
            break;
        case TAF_SIM_READY:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_READY;
            break;
        case TAF_SIM_RESTRICTED:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_RESTRICTED;
            break;
        case TAF_SIM_ERROR:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_ERROR;
            break;
        case TAF_SIM_STATE_UNKNOWN:
            ret = SimSvcTypes::SimStateT::SIM_STATE_T_UNKNOWN;
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
static inline taf_sim_States_t StateIvssToSim(SimSvcTypes::SimStateT state)
{
    taf_sim_States_t ret = TAF_SIM_PRESENT;
    switch (state)
    {
        case SimSvcTypes::SimStateT::SIM_STATE_T_PRESENT:
            ret = TAF_SIM_PRESENT;
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_ABSENT:
            ret = TAF_SIM_ABSENT;
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_READY:
            ret = TAF_SIM_READY;
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_RESTRICTED:
            ret = TAF_SIM_RESTRICTED;
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_ERROR:
            ret = TAF_SIM_ERROR;
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_UNKNOWN:
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
class tafIvssSimSvc: public v1_0::com::qualcomm::qti::telephony::SimSvcStubDefault {
public:
    tafIvssSimSvc() {};
    virtual ~tafIvssSimSvc() {};

    // The initialization function of the Sim Service.
    void Init();

    static std::shared_ptr<tafIvssSimSvc> GetInstance();

    // ivss method function.
    virtual void GetImsi(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetImsiReply_t _reply);
    virtual void GetState(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetStateReply_t _reply);
    virtual void GetIccid(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetIccidReply_t _reply);

    // ivss method function handler.
    static void GetImsiHandler(void* reportPtr);
    static void GetStateHandler(void* reportPtr);
    static void GetIccidHandler(void* reportPtr);

    // ivss event function handler.
    static void taf_Ivss_Sim_NewStateHandler(taf_sim_Id_t slotId,
        taf_sim_States_t state, void* contextPtr);

    // memory pools.
    le_mem_PoolRef_t EventPool;

    // ivss method ref.
    le_event_Id_t GetImsiEvent = NULL;
    le_event_Id_t GetStateEvent = NULL;
    le_event_Id_t GetIccidEvent = NULL;

    le_event_HandlerRef_t GetImsiEventHandlerRef;
    le_event_HandlerRef_t GetStateEventHandlerRef;
    le_event_HandlerRef_t GetIccidEventHandlerRef;

    // ivss event ref.
    taf_sim_NewStateHandlerRef_t NewStateHandlerRef;
};

#endif // TAFIVSSSIMSVC_HPP_
