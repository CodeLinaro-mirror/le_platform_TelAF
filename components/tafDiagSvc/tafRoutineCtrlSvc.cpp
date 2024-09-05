/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafRoutineCtrlSvr.hpp"
#include "tafDiagBackend.hpp"

using namespace telux::tafsvc;

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
