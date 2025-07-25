/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDiagSvc.cpp
 * @brief      This file provides the telaf enbale condition service as interfaces described in
 *             taf_diag.api. The Diag service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafDiagSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Sets an enable condition.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_SetEnableCondition
(
    uint8_t enableConditionID,
        ///< [IN] Enable condition ID.
    bool conditionFulfilled
        ///< [IN] Enable condition status.
)
{
    LE_DEBUG("taf_diag_SetEnableCondition");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.SetEnableCondition(enableConditionID, conditionFulfilled);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets an enable condition status.
 *
 * @return
 *     - TRUE -- Succeeded.
 *     - FALSE -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
bool taf_diag_GetEnableConditionStatus
(
    uint8_t enableConditionID
        ///< [IN] Enable condition ID.
)
{
    LE_DEBUG("taf_diag_GetEnableCondition");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.GetEnableConditionStatus(enableConditionID);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a Diag service, if there's no Diag service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diag_ServiceRef_t taf_diag_GetService
(
)
{
    LE_DEBUG("taf_diag_GetService");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.GetService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service. If the VLAN ID is not found or does not exist, it will return an
 * error.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_SetVlanId
(
    taf_diag_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    LE_DEBUG("taf_diag_SetVlanId");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Select traget VLAN ID.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_NOT_FOUND -- Vlan ID not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_SelectTargetVlanID
(
    taf_diag_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    LE_DEBUG("taf_diag_SelectTargetVlanID");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.SelectTargetVlanID(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diag_TesterState'
 *
 * This event provides information on tester present state change.
 */
//--------------------------------------------------------------------------------------------------
taf_diag_TesterStateHandlerRef_t taf_diag_AddTesterStateHandler
(
    taf_diag_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diag_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN] Tester present state change handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diag_AddTesterStateHandler");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.AddTesterStateHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diag_TesterState'
 */
//--------------------------------------------------------------------------------------------------
void taf_diag_RemoveTesterStateHandler
(
    taf_diag_TesterStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diag_RemoveTesterStateHandler");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.RemoveTesterStateHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 ** Releases a Tester state notification message.
 **
 ** @return
 **     - LE_OK -- Succeeded.
 **     - LE_BAD_PARAMETER -- Invalid stateRef or invalid service of the stateRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_ReleaseTesterStateMsg
(
    taf_diag_TesterStateRef_t stateRef
        ///< [IN] Tester state reference.
)
{
    LE_DEBUG("taf_diag_ReleaseTesterStateMsg");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.ReleaseTesterStateMsg(stateRef);
}

void taf_diag_CancelFileXferAsync
(
    taf_diag_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diag_CancelFileXferCallbackFunc_t handlerPtr,
        ///< [IN] The handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diag_CancelFileXferAsync");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.CancelFileXferAsync(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pauses diag service to not receive any diagnostic requests.
 *
 * @instaging
 *
 * @result
 *     - LE_OK -- Succeeded.
 *     - LE_IN_PROGRESS -- Succeeded, but a diagnostic request is being handled.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_Pause
(
    taf_diag_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.Pause(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resumes diag service to receive diagnostic requests.
 *
 * @instaging
 *
 * @result
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_Resume
(
    taf_diag_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.Resume(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the server service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_RemoveSvc
(
    taf_diag_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    LE_DEBUG("taf_diag_RemoveSvc");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.RemoveSvc(svcRef);
}
