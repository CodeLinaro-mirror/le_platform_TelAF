/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssSimSvc.hpp"

using namespace v0::com::qualcomm::qti::modem;

extern std::shared_ptr<tafIvssSimSvcImpl> ivssSimSvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetImsi'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvcImpl::GetImsiHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)reportPtr;
    indPtr->result = taf_sim_GetIMSI(indPtr->getImsi.slotId, indPtr->getImsi.imsi,
        TAF_SIM_IMSI_BYTES);
    le_sem_Post(indPtr->semRef);

    TAF_ERROR_IF_RET_NIL(indPtr->result != LE_OK, "taf_sim_GetIMSI fail - %s",
        LE_RESULT_TXT(indPtr->result));

}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the IMSI for the SIM.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvcImpl::GetImsi(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetImsiReply_t _reply)
{
    // Create a generic response message object.
    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSim_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetImsiSem", 0);
    indPtr->getImsi.slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetImsiEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    _reply(ResultLeToIvss(indPtr->result), std::string(indPtr->getImsi.imsi));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetState'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvcImpl::GetStateHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)reportPtr;
    indPtr->getState.simState = taf_sim_GetState(indPtr->getState.slotId);
    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of the SIM card.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvcImpl::GetState(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetStateReply_t _reply)
{
    // Create a generic response message object.
    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSim_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetStateSem", 0);
    indPtr->getState.slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetStateEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    _reply(ResultLeToIvss(indPtr->result), StateSimToIvss(indPtr->getState.simState));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Ivss_Sim_NewStateHandler
(
    taf_sim_Id_t slotId,                       ///< Slot ID.
    taf_sim_States_t state,                    ///< State.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    CommonTypes::PhoneId phoneId = PhoneIdSimToIvss(slotId);
    SimSvc::States simState = StateSimToIvss(state);
    ivssSimSvc->fireSimStateEvent(phoneId, simState);

    LE_DEBUG("tafIvssSimSvc NewState Event");
};

void tafIvssSimSvcImpl::Init
(
    void
)
{
    // Create memory pools.
    EventPool = le_mem_CreatePool("Ivss Sim EventPool", sizeof(taf_IvssSim_Ind_t));

    // Create the event.
    GetImsiEvent = le_event_CreateIdWithRefCounting("GetImsiEvent");
    GetStateEvent = le_event_CreateIdWithRefCounting("GetStateEvent");

    // Add event handler.
    GetImsiEventHandlerRef = le_event_AddHandler("GetImsiEvent Handler",
        GetImsiEvent, tafIvssSimSvcImpl::GetImsiHandler);
    GetStateEventHandlerRef = le_event_AddHandler("GetStateEvent Handler",
        GetStateEvent, tafIvssSimSvcImpl::GetStateHandler);

    // Init commonapi event.
    NewStateHandlerRef = taf_sim_AddNewStateHandler(
        (taf_sim_NewStateHandlerFunc_t)taf_Ivss_Sim_NewStateHandler, NULL);

    LE_INFO("tafIvssSimSvc Service initialized");
};
