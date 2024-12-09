/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of Diag server.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagSvr &taf_DiagSvr::GetInstance
(
)
{
    static taf_DiagSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets an enable condition.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DiagSvr::SetEnableCondition
(
    uint8_t enableConditionID,
    bool conditionFulfilled
)
{
    LE_DEBUG("SetEnableCondition!");

    auto &diag = taf_DiagSvr::GetInstance();
    bool isEnableIdAvailable = false;

    // Check enable id already present.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&enableStatusList);
    while (linkPtr)
    {
        taf_DiagEnableStatus_t* enableCtxPtr = CONTAINER_OF(linkPtr, taf_DiagEnableStatus_t,
                link);
        linkPtr = le_dls_PeekNext(&enableStatusList, linkPtr);

        if (enableCtxPtr->enableConditionID == enableConditionID)
        {
            LE_DEBUG("Get enableConditionID %p by id %d", enableCtxPtr, enableConditionID);
            enableCtxPtr->conditionFulfilled = conditionFulfilled;
            isEnableIdAvailable = true;
            break;
        }
    }

    if (!isEnableIdAvailable)
    {
        taf_DiagEnableStatus_t* enableStatusPtr = NULL;
        enableStatusPtr = (taf_DiagEnableStatus_t *)le_mem_ForceAlloc(diag.EnableMemPool);

        enableStatusPtr->enableConditionID = enableConditionID;
        enableStatusPtr->conditionFulfilled = conditionFulfilled;
        enableStatusPtr->link = LE_DLS_LINK_INIT;

        // add this event context to list
        le_dls_Queue(&diag.enableStatusList, &enableStatusPtr->link);
    }

    // Set event enable condition status
    auto& diagEvent = taf_EventSvr::GetInstance();
    diagEvent.SetEventEnableStatus(enableConditionID);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get an enable condition status.
 */
//-------------------------------------------------------------------------------------------------
bool taf_DiagSvr::GetEnableConditionStatus
(
    uint8_t enableConditionID
)
{
    LE_DEBUG("GetEnableConditionStatus!");

    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&enableStatusList);
    while (linkPtr)
    {
        taf_DiagEnableStatus_t* enableCtxPtr = CONTAINER_OF(linkPtr, taf_DiagEnableStatus_t,
                link);
        linkPtr = le_dls_PeekNext(&enableStatusList, linkPtr);

        if (enableCtxPtr->enableConditionID == enableConditionID)
        {
            LE_DEBUG("Get enableConditionID %p by id %d", enableCtxPtr, enableConditionID);
            return enableCtxPtr->conditionFulfilled;
        }
    }

    LE_DEBUG("Requested enableCondition ID not found : %d", enableConditionID);
    return false;
}

//-------------------------------------------------------------------------------------------------
/**
 * Client session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &diag = taf_DiagSvr::GetInstance();

    // Clear enable condition list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&diag.enableStatusList);
    while (linkPtr != NULL)
    {
        taf_DiagEnableStatus_t* enableCtxPtr = CONTAINER_OF(linkPtr, taf_DiagEnableStatus_t,
                link);
        if (enableCtxPtr != NULL)
        {
            LE_INFO("Release enableCondition(addr=%p)", enableCtxPtr);
            // Free the message
            le_mem_Release(enableCtxPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&diag.enableStatusList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::Init
(
    void
)
{
    LE_INFO("taf_DiagSvr Init!");

    // Create memory pools.
    EnableMemPool = le_mem_CreatePool("EnableConditionMemPool", sizeof(taf_DiagEnableStatus_t));

    // Set client session close handler.
    le_msg_AddServiceCloseHandler(taf_diag_GetServiceRef(), OnClientDisconnection, NULL);
}
