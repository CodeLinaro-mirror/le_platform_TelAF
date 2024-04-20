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
#include "tafDIDBackendSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDIDBackend_ReadDID'
 *
 * This event provides information about the received ReadDID message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDIDBackend_ReadDIDHandlerRef_t taf_diagDIDBackend_AddReadDIDHandler
(
    taf_diagDIDBackend_ReadDIDHandlerFunc_t handlerPtr,
        ///< [IN] ReadDID message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDIDBackend_AddReadDIDHandler");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.AddReadDIDHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDIDBackend_ReadDID'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDIDBackend_RemoveReadDIDHandler
(
    taf_diagDIDBackend_ReadDIDHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDIDBackend_RemoveReadDIDHandler");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.RemoveReadDIDHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the received ReadDID message.
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
le_result_t taf_diagDIDBackend_SendReadDIDResp
(
    taf_diagDIDBackend_ReadDIDRef_t readDIDRef,
        ///< [IN] ReadDID message reference.
    taf_diagDIDBackend_ReadDIDErrorCode_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Data payload.
    size_t dataSize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDIDBackend_SendReadDIDResp");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.SendReadDIDResp(readDIDRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDIDBackend_WriteDID'
 *
 * This event provides information about the received ReadDID message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDIDBackend_WriteDIDHandlerRef_t taf_diagDIDBackend_AddWriteDIDHandler
(
    taf_diagDIDBackend_WriteDIDHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDIDBackend_AddWriteDIDHandler");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.AddWriteDIDHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDIDBackend_WriteDID'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDIDBackend_RemoveWriteDIDHandler
(
    taf_diagDIDBackend_WriteDIDHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDIDBackend_RemoveWriteDIDHandler");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.RemoveWriteDIDHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the WriteDID message.
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
le_result_t taf_diagDIDBackend_SendWriteDIDResp
(
    taf_diagDIDBackend_WriteDIDRef_t writeDIDRef,
        ///< [IN] Received message reference.
    taf_diagDIDBackend_WriteDIDErrorCode_t errCode,
        ///< [IN] Error code type.
    uint16_t dataId
        ///< [IN] data identifier.
)
{
    LE_DEBUG("taf_diagDIDBackend_SendReadDIDResp");
    auto &didBackend = taf_DIDBackend::GetInstance();
    return didBackend.SendWriteDIDResp(writeDIDRef, errCode, dataId);
}
