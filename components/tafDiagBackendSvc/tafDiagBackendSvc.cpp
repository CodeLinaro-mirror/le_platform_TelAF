/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <iostream>
#include <string>
#include <memory>
#include "tafDiagBackendSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagBackend_DiagEvent'
 *
 * This event provides information about the received Service message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagBackend_DiagEventHandlerRef_t taf_diagBackend_AddDiagEventHandler
(
    taf_diagBackend_DiagHandlerFunc_t handlerPtr,
        ///< [IN] IntegrationHandler
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("taf_diagBackend_AddDiagEventHandler");
    auto &diagBackend = taf_DiagBackend::GetInstance();
    return diagBackend.AddDiagEventHandler(handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagBackend_DiagEvent'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagBackend_RemoveDiagEventHandler
(
    taf_diagBackend_DiagEventHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("taf_diagBackend_RemoveDiagEventHandler");
    auto &diagBackend = taf_DiagBackend::GetInstance();
    return diagBackend.RemoveDiagEventHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sends a Response 
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_COMM_ERROR     Sending message error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagBackend_SendResp
(
    taf_diagBackend_DiagInfRef_t ref,
        ///< [IN] reference
    const taf_diagBackend_AddrInfo_t * LE_NONNULL addrInfoPtr,
        ///< [IN] Diagnostic message pointer.
    uint8_t serviceID,
        ///< [IN] Service ID
    uint8_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Data payload
    size_t dataSize
        ///< [IN]
)
{
    LE_INFO("taf_diagBackend_SendResp");
    auto &diagBackend = taf_DiagBackend::GetInstance();
    return diagBackend.SendResp(ref, addrInfoPtr, serviceID, errCode, dataPtr, dataSize);
}
