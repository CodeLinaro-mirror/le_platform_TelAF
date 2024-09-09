/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafIOCtrlSvc.cpp
 * @brief      This file provides the telaf InputOutputControlByIdentifier service as interfaces
 *             described in taf_diagIOCtrl.api. The Diag IOCtrl service will be started
 *             automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafIOCtrlSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a InputOutputControlByIdentifier service, if there's no
 * InputOutputControlByIdentifier service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagIOCtrl_ServiceRef_t taf_diagIOCtrl_GetService
(
    uint16_t dataId
        ///< [IN] Data identifier.
)
{
    LE_DEBUG("taf_diagIOCtrl_GetService");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.GetService(dataId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagIOCtrl_RxMsg'
 *
 * This event provides information on Rx InputOutputControlByIdentifier message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagIOCtrl_RxMsgHandlerRef_t taf_diagIOCtrl_AddRxMsgHandler
(
    taf_diagIOCtrl_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagIOCtrl_RxMsgHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagIOCtrl_AddRxMsgHandler");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.AddRxMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagIOCtrl_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagIOCtrl_RemoveRxMsgHandler
(
    taf_diagIOCtrl_RxMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagIOCtrl_RemoveRxMsgHandler");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.RemoveRxMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the control state of the Rx input output control parameter.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIOCtrl_GetCtrlState
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* controlStatePtr,
        ///< [OUT] Control state.
    size_t* controlStateSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagIOCtrl_GetCtrlState");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.GetCtrlState(rxMsgRef, controlStatePtr, controlStateSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the control enable mask record of the Rx InputOutputControlByIdentifier message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIOCtrl_GetCtrlEnableMaskRecd
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* maskRecordPtr,
        ///< [OUT] Control enable mask record.
    size_t* maskRecordSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagIOCtrl_GetCtrlEnableMaskRecd");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.GetCtrlEnableMaskRecd(rxMsgRef, maskRecordPtr, maskRecordSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx InputOutputControlByIdentifier message.
 *
 * @note This function must be called to send a response if receiving a message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIOCtrl_SendResp
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Payload data.
    size_t dataSize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagIOCtrl_SendResp");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.SendResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the InputOutputControlByIdentifier server service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIOCtrl_RemoveSvc
(
    taf_diagIOCtrl_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    LE_DEBUG("taf_diagIOCtrl_RemoveSvc");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    return ioCtrl.RemoveSvc(svcRef);
}
