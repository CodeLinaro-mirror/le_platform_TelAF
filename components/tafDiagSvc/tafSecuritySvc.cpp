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

using namespace telux::tafsvc;

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
    taf_diagSecurity_SecAccessErrorCode_t errCode,
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
