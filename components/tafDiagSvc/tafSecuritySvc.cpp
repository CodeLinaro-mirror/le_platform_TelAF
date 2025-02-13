/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafSecuritySvc.cpp
 * @brief      This file provides the telaf security service as interfaces described in
 *             taf_diagSecurity.api. The Diag Security service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafSecuritySvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a security service, if there's no security service, a new one will be
 * created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecurity_ServiceRef_t taf_diagSecurity_GetService
(
    void
)
{
    LE_DEBUG("taf_diagSecurity_GetService");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.GetService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter security request.
 * if the VLAN ID is not found or not exist, it will return error.
 * This function shall be called before registering RxSesTypeHandler and RxSecAccessMsgHandler.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_SetVlanId
(
    taf_diagSecurity_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID
)
{
    LE_DEBUG("taf_diagSecurity_SetVlanId, vlan id is 0x%x", vlanId);
    auto &security = taf_SecuritySvr::GetInstance();

    return security.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagSecurity_RxSesTypeCheck'
 *
 * This event provides information on Rx session control type.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecurity_RxSesTypeCheckHandlerRef_t taf_diagSecurity_AddRxSesTypeCheckHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagSecurity_RxSesTypeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecurity_AddRxSesTypeCheckHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.AddRxSesTypeCheckHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagSecurity_RxSesTypeCheck'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagSecurity_RemoveRxSesTypeCheckHandler
(
    taf_diagSecurity_RxSesTypeCheckHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecurity_RemoveRxSesTypeCheckHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.RemoveRxSesTypeCheckHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the condition check of Rx session control type.
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
le_result_t taf_diagSecurity_SendSesTypeCheckResp
(
    taf_diagSecurity_RxSesTypeCheckRef_t rxSesTypeRef,
        ///< [IN] Received session type reference.
    uint8_t errCode
        ///< [IN] Error code type.
)
{
    LE_DEBUG("taf_diagSecurity_SendSesTypeCheckResp");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.SendSesTypeCheckResp(rxSesTypeRef, errCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagSecurity_SesChange'
 *
 * This event provides information on session control type change.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecurity_SesChangeHandlerRef_t taf_diagSecurity_AddSesChangeHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagSecurity_SesChangeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecurity_AddSesChangeHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.AddSesChangeHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagSecurity_SesChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagSecurity_RemoveSesChangeHandler
(
    taf_diagSecurity_SesChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecurity_RemoveSesChangeHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.RemoveSesChangeHandler(handlerRef);
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
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_SelectTargetVlanID
(
    taf_diagSecurity_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    LE_DEBUG("taf_diagSecurity_SelectTargetVlanID");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.SelectTargetVlanID(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the current session control type.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_GetCurrentSesType
(
    taf_diagSecurity_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint8_t* currentTypePtr
        ///< [OUT] current session type.
)
{
    LE_DEBUG("taf_diagSecurity_GetCurrentSesType");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.GetCurrentSesType(svcRef, currentTypePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases a session change notification message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid msgRef or invalid service of the msgRef.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_ReleaseSesChangeMsg
(
    taf_diagSecurity_SesChangeRef_t sesChangeRef
        ///< [IN] Session change reference.
)
{
    LE_DEBUG("taf_diagSecurity_ReleaseSesChangeMsg");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.ReleaseSesChangeMsg(sesChangeRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagSecurity_RxSecAccessMsg'
 *
 * This event provides information on Rx security access message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecurity_RxSecAccessMsgHandlerRef_t taf_diagSecurity_AddRxSecAccessMsgHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
    taf_diagSecurity_RxSecAccessMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("taf_diagSecurity_AddRxSecAccessMsgHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.AddRxSecAccessMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagSecurity_RxSecAccessMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagSecurity_RemoveRxSecAccessMsgHandler
(
    taf_diagSecurity_RxSecAccessMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("taf_diagSecurity_RemoveRxSecAccessMsgHandler");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.RemoveRxSecAccessMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the securityAccessDataRecord/securityKey payload length of the Rx security access message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_GetSecAccessPayloadLen
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint16_t* payloadLenPtr
)
{
    LE_DEBUG("taf_diagSecurity_GetSecAccessPayloadLen");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.GetSecAccessPayloadLen(rxMsgRef, payloadLenPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the securityAccessDataRecord/securityKey payload of the Rx security access message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *     - LE_OVERFLOW -- Payload size is too small.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_GetSecAccessPayload
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t* payloadPtr,
    size_t* payloadSizePtr
)
{
    LE_DEBUG("taf_diagSecurity_GetSecAccessPayload");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.GetSecAccessPayload(rxMsgRef, payloadPtr, payloadSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx security access message.
 *
 * @note This function must be called to send a response if receiving a message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_SendSecAccessResp
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_DEBUG("taf_diagSecurity_SendSecAccessResp");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.SendSecAccessResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vlan id of the Rx SessionControl/SecurityAccess message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_GetVlanIdFromMsg
(
    taf_diagSecurity_RxMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    LE_DEBUG("taf_diagSecurity_GetService");
    auto &security = taf_SecuritySvr::GetInstance();

    return security.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the security service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagSecurity_RemoveSvc
(
    taf_diagSecurity_ServiceRef_t svcRef
)
{
    LE_DEBUG("taf_diagSecurity_RemoveSvc");
    auto &security = taf_SecuritySvr::GetInstance();
    return security.RemoveSvc(svcRef);
}
