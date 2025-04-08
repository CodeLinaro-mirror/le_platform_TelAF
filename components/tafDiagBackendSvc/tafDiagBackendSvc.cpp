/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafDiagBackendSvr.hpp"

using namespace tafsvc;

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
