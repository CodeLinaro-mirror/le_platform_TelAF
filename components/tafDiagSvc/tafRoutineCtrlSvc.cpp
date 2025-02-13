/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafRoutineCtrlSvr.hpp"
#include "tafDiagBackend.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets or creates the reference to a RoutineControl service.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagRoutineCtrl_ServiceRef_t taf_diagRoutineCtrl_GetService
(
    uint16_t identifier
        ///< [IN] Routine identifier.
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.FindOrCreateService(identifier);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter ReadDID and WriteDID request.
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
le_result_t taf_diagRoutineCtrl_SetVlanId
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagRoutineCtrl_RxMsg'
 *
 * This event provides information on Rx RoutineControl message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagRoutineCtrl_RxMsgHandlerRef_t taf_diagRoutineCtrl_AddRxMsgHandler
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagRoutineCtrl_RxMsgHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.AddRxReqMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagRoutineCtrl_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagRoutineCtrl_RemoveRxMsgHandler
(
    taf_diagRoutineCtrl_RxMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    tafRCS.RemoveRxReqMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RoutineControl option record of the Rx RoutineControl message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagRoutineCtrl_GetRoutineCtrlRec
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* optionRecPtr,
        ///< [OUT] RoutineControl option record.
    size_t* optionRecSizePtr
        ///< [INOUT]
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.GetRoutineCtrlRec(rxMsgRef, optionRecPtr, optionRecSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vlan id of the Rx RoutineControl message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagRoutineCtrl_GetVlanIdFromMsg
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx RoutineControl message.
 *
 * @note
 *     - This function must be called to send a response if receiving a message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagRoutineCtrl_SendResp
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Payload data.
    size_t dataSize
        ///< [IN]
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.SendRoutineCtrlResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the RoutineControl server service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagRoutineCtrl_RemoveSvc
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();

    return tafRCS.RemoveRoutineCtrlSvc(svcRef);
}
