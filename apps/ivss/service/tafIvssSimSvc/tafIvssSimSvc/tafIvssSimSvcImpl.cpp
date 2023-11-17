/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssSimSvcImpl.hpp"

using namespace v0::com::qualcomm::qti::modem;

extern std::shared_ptr<tafIvssSimSvcImpl> ivssSimSvc;

// This function gets run by telaf thread.
void tafIvssSimSvcImpl::GetImsiHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        IvssSimSvc_method_t* requestPtr = (IvssSimSvc_method_t*)reportPtr;
        requestPtr->result = taf_sim_GetIMSI(requestPtr->slotId, requestPtr->imsi,
            TAF_SIM_IMSI_BYTES);
        if (requestPtr->result != LE_OK)
        {
            LE_ERROR("taf_sim_GetIMSI failed - %s", LE_RESULT_TXT(requestPtr->result));
        }
        LE_INFO("tafIvssSimSvc GetIMSI: %s \n", requestPtr->imsi);
        le_sem_Post(requestPtr->semRef);
    }
}

void tafIvssSimSvcImpl::GetImsi(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetImsiReply_t _reply)
{
    // Create a generic response message object.
    IvssSimSvc_method_t* requestPtr = (IvssSimSvc_method_t*)le_mem_ForceAlloc(EventPool);
    memset(requestPtr, 0, sizeof(IvssSimSvc_method_t));
    requestPtr->semRef = le_sem_Create("IvssSimSvc_GetImsiSem", 0);
    requestPtr->slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetImsiEvent, (void*)requestPtr);
    le_sem_Wait(requestPtr->semRef);

    std::string imsi(requestPtr->imsi);
    _reply(ResultLeToIvss(requestPtr->result), imsi);

    le_sem_Delete(requestPtr->semRef);
    le_mem_Release(requestPtr);
};


// This function gets run by telaf thread.
void tafIvssSimSvcImpl::GetStateHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        IvssSimSvc_method_t* requestPtr = (IvssSimSvc_method_t*)reportPtr;
        requestPtr->simState = taf_sim_GetState(requestPtr->slotId);
        LE_INFO("GetStateHandler \n");
        le_sem_Post(requestPtr->semRef);
    }
}

void tafIvssSimSvcImpl::GetState(const std::shared_ptr<CommonAPI::ClientId> _client,
        CommonTypes::PhoneId _phoneId, GetStateReply_t _reply)
{
    // Create a generic response message object.
    IvssSimSvc_method_t* requestPtr = (IvssSimSvc_method_t*)le_mem_ForceAlloc(EventPool);
    memset(requestPtr, 0, sizeof(IvssSimSvc_method_t));
    requestPtr->semRef = le_sem_Create("IvssSimSvc_GetStateSem", 0);
    requestPtr->slotId = PhoneIdIvssToSim(_phoneId);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetStateEvent, (void*)requestPtr);
    le_sem_Wait(requestPtr->semRef);

    _reply(ResultLeToIvss(requestPtr->result), StateSimToIvss(requestPtr->simState));

    le_sem_Delete(requestPtr->semRef);
    le_mem_Release(requestPtr);
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

    LE_INFO("tafIvssSimSvc NewState Event");
};

void tafIvssSimSvcImpl::Init
(
    void
)
{
    // Create memory pools.
    EventPool = le_mem_CreatePool("Ivss Sim EventPool", sizeof(IvssSimSvc_method_t));

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
