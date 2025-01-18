/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssInfoSvc.hpp"

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss Info server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssInfoSvc> tafIvssInfoSvc::GetInstance()
{
    static std::shared_ptr<tafIvssInfoSvc> instance = std::make_shared<tafIvssInfoSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetImei'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssInfoSvc::GetImeiHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssInfo_Ind_t* indPtr = (taf_IvssInfo_Ind_t*)reportPtr;
    indPtr->result = taf_devInfo_GetImei(indPtr->getImei.imei, TAF_DEVINFO_IMEI_MAX_BYTES);
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "taf_devInfo_GetImei fail - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the International Mobile Equipment Identity (IMEI).
 */
//--------------------------------------------------------------------------------------------------
void tafIvssInfoSvc::GetImei(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetImeiReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssInfoSvc GetImei \n");

    taf_IvssInfo_Ind_t* indPtr = (taf_IvssInfo_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssInfo_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetImeiSem", 0);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetImeiEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(std::string(indPtr->getImei.imei), ResultLeToIvssInfo(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssInfoSvc::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Info EventPool", sizeof(taf_IvssInfo_Ind_t));

    // Init events.
    GetImeiEvent = le_event_CreateIdWithRefCounting("GetImeiEvent");

    // Init event handler.
    GetImeiEventHandlerRef = le_event_AddHandler("GetImeiEvent Handler",
        GetImeiEvent, tafIvssInfoSvc::GetImeiHandler);

    LE_INFO("tafIvssInfoSvc Service initialized");
};
