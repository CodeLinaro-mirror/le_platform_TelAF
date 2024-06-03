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
#include "tafSecBackendSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagSecBackend_SesTypeCheck'
 *
 * This event provides information on Rx session control type.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecBackend_SesTypeCheckHandlerRef_t taf_diagSecBackend_AddSesTypeCheckHandler
(
    taf_diagSecBackend_SesTypeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecBackend_AddSesTypeCheckHandler");
    auto &secBackend = taf_SecBackend::GetInstance();
    return secBackend.AddSesTypeCheckHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagSecBackend_SesTypeCheck'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagSecBackend_RemoveSesTypeCheckHandler
(
    taf_diagSecBackend_SesTypeCheckHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecBackend_RemoveSesTypeCheckHandler");
    auto &secBackend = taf_SecBackend::GetInstance();
    return secBackend.RemoveSesTypeCheckHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the condition check of session control type.
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
le_result_t taf_diagSecBackend_SendSesTypeCheckResp
(
    taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef,
        ///< [IN] Session type reference.
    taf_diagSecBackend_SesControlErrorCode_t errCode
        ///< [IN] Error code type.
)
{
    LE_DEBUG("taf_diagSecBackend_SendSesTypeCheckResp");
    auto &secBackend = taf_SecBackend::GetInstance();
    return secBackend.SendSesTypeCheckResp(sesTypeRef, errCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagSecurityBackend_SesChange'
 *
 * This event provides information on session control type change.
 */
//--------------------------------------------------------------------------------------------------
taf_diagSecBackend_SesChangeHandlerRef_t taf_diagSecBackend_AddSesChangeHandler
(
    taf_diagSecBackend_SesChangeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecBackend_AddSesChangeHandler");
    auto &secBackend = taf_SecBackend::GetInstance();
    return secBackend.AddSesChangeHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagSecBackend_SesChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagSecBackend_RemoveSesChangeHandler
(
    taf_diagSecBackend_SesChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagSecBackend_RemoveSesChangeHandler");
    auto &secBackend = taf_SecBackend::GetInstance();
    return secBackend.RemoveSesChangeHandler(handlerRef);
}
