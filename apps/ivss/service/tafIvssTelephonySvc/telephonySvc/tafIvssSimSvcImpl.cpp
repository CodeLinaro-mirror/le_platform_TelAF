/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssSimSvc.hpp"

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Sim server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssSimSvc> tafIvssSimSvc::GetInstance()
{
    static std::shared_ptr<tafIvssSimSvc> instance = std::make_shared<tafIvssSimSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetImsi'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::GetImsiHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)reportPtr;
    indPtr->result = taf_sim_GetIMSI(indPtr->getImsi.slotId, indPtr->getImsi.imsi,
        sizeof(indPtr->getImsi.imsi));
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef, "taf_sim_GetIMSI fail - %s",
        LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the IMSI for the SIM.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::GetImsi(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetImsiReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssSimSvc GetImsi \n");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSim_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetImsiSem", 0);
    indPtr->getImsi.slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetImsiEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    _reply(std::string(indPtr->getImsi.imsi), ResultLeToIvssSim(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetState'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::GetStateHandler
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
void tafIvssSimSvc::GetState(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetStateReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssSimSvc GetState \n");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSim_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetStateSem", 0);
    indPtr->getState.slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetStateEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    _reply(StateSimToIvss(indPtr->getState.simState), ResultLeToIvssSim(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetIccid'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::GetIccidHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)reportPtr;
    indPtr->result = taf_sim_GetICCID(indPtr->GetIccid.slotId, indPtr->GetIccid.iccid,
        sizeof(indPtr->GetIccid.iccid));

    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef, "taf_sim_GetIMSI fail - %s",
        LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the SIM's ICCID.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::GetIccid(const std::shared_ptr<CommonAPI::ClientId> _client,
        SimSvcTypes::PhoneIdT _phoneId, GetIccidReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssSimSvc GetIccid \n");

    taf_IvssSim_Ind_t* indPtr = (taf_IvssSim_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSim_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetIccidSem", 0);
    indPtr->GetIccid.slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetIccidEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    _reply(std::string(indPtr->GetIccid.iccid), ResultLeToIvssSim(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Sim New State change.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::taf_Ivss_Sim_NewStateHandler
(
    taf_sim_Id_t slotId,                       ///< Slot ID.
    taf_sim_States_t state,                    ///< State.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_DEBUG("tafIvssSimSvc NewState Event");

    auto ivssSim = tafIvssSimSvc::GetInstance();
    ivssSim->fireSimStateEvent(SimSvcTypes::ValueState::VALUE_STATE_VALID, PhoneIdSimToIvss(slotId),
        StateSimToIvss(state));
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSimSvc::Init
(
    void
)
{
    // Create memory pools.
    EventPool = le_mem_CreatePool("Ivss Sim EventPool", sizeof(taf_IvssSim_Ind_t));

    // Create the event.
    GetImsiEvent = le_event_CreateIdWithRefCounting("GetImsiEvent");
    GetStateEvent = le_event_CreateIdWithRefCounting("GetStateEvent");
    GetIccidEvent = le_event_CreateIdWithRefCounting("GetIccidEvent");

    // Add event handler.
    GetImsiEventHandlerRef = le_event_AddHandler("GetImsiEvent Handler", GetImsiEvent,
        tafIvssSimSvc::GetImsiHandler);
    GetStateEventHandlerRef = le_event_AddHandler("GetStateEvent Handler", GetStateEvent,
        tafIvssSimSvc::GetStateHandler);
    GetIccidEventHandlerRef = le_event_AddHandler("GetIccidEvent Handler", GetIccidEvent,
        tafIvssSimSvc::GetIccidHandler);

    // Init commonapi event.
    NewStateHandlerRef = taf_sim_AddNewStateHandler(
        (taf_sim_NewStateHandlerFunc_t)taf_Ivss_Sim_NewStateHandler, NULL);

    LE_INFO("tafIvssSimSvc Service initialized");
};
