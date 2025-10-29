/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIvssLocationSvc.hpp"

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss Location server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssLocationSvc> tafIvssLocationSvc::GetInstance()
{
    static std::shared_ptr<tafIvssLocationSvc> instance = std::make_shared<tafIvssLocationSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetCapabilities'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssLocationSvc::GetCapabilitiesHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssLocation_Ind_t* indPtr = (taf_IvssLocation_Ind_t*)reportPtr;
    indPtr->result = taf_locGnss_GetCapabilities(&indPtr->getCapabilities.capabilitiesMask);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_locGnss_GetCapabilities fail - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the location's data (Latitude, Longitude, Horizontal accuracy).
 */
//--------------------------------------------------------------------------------------------------
void tafIvssLocationSvc::GetCapabilities(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetCapabilitiesReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssLocationSvc GetCapabilities \n");

    taf_IvssLocation_Ind_t* indPtr = (taf_IvssLocation_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssLocation_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetCapabilitiesSem", 0);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetCapabilitiesEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(CapBitMaskLocToIvss(indPtr->getCapabilities.capabilitiesMask),
        ResultLeToIvssLocation(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssLocationSvc::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Location EventPool", sizeof(taf_IvssLocation_Ind_t));

    // Init events.
    GetCapabilitiesEvent = le_event_CreateIdWithRefCounting("GetCapabilitiesEvent");

    // Init event handler.
    GetCapabilitiesEventHandlerRef = le_event_AddHandler("GetCapabilitiesEvent Handler",
        GetCapabilitiesEvent, tafIvssLocationSvc::GetCapabilitiesHandler);

    LE_INFO("tafIvssLocationSvc Service initialized");
};
