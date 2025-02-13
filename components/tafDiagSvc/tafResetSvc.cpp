/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafResetSvc.cpp
 * @brief      This file provides the taf ECU Reset service as interfaces described
 *             in taf_diagReset.api. The Diag Reset service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafResetSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a Reset service.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagReset_ServiceRef_t taf_diagReset_GetService
(
    uint8_t resetType
)
{
    LE_DEBUG("taf_diagReset_GetService");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.GetService(resetType);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter ECUReset request.
 * if the VLAN ID is not found or not exist, it will return error.
 * This function shall be called before registering RxMsgHandler.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagReset_SetVlanId
(
    taf_diagReset_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID
)
{
    LE_DEBUG("taf_diagReset_GetService");
    auto &reset = taf_ResetSvr::GetInstance();

    return reset.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagReset_RxMsg'
 *
 * This event provides information on Rx Reset message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagReset_RxMsgHandlerRef_t taf_diagReset_AddRxMsgHandler
(
    taf_diagReset_ServiceRef_t svcRef,
    taf_diagReset_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("taf_diagReset_AddRxMsgHandler");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.AddRxMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagReset_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagReset_RemoveRxMsgHandler
(
    taf_diagReset_RxMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("taf_diagReset_RemoveRxMsgHandler");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.RemoveRxMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vlan id of the Rx ECUReset message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagReset_GetVlanIdFromMsg
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    LE_DEBUG("taf_diagReset_GetService");
    auto &reset = taf_ResetSvr::GetInstance();

    return reset.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx Reset message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagReset_SendResp
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    uint8_t errCode
)
{
    LE_DEBUG("taf_diagReset_SendResp");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.SendResp(rxMsgRef, errCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the Reset service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagReset_RemoveSvc
(
    taf_diagReset_ServiceRef_t svcRef
)
{
    LE_DEBUG("taf_diagReset_RemoveSvc");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.RemoveSvc(svcRef);
}
