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

using namespace telux::tafsvc;

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
    taf_diagReset_Type_t resetType
)
{
    LE_DEBUG("taf_diagReset_GetService");
    auto &reset = taf_ResetSvr::GetInstance();
    return reset.GetService(resetType);
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
