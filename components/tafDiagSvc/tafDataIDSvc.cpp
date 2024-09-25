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
 * @file       tafRWDIDSvc.cpp
 * @brief      This file provides the telaf Read/Write DID service as interfaces described
 *             in taf_diagDataID .api. The Diag RWDID service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafDataIDSvr.hpp"

using namespace telux::tafsvc;

//-------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a DataID service. If there is no service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDataID_ServiceRef_t taf_diagDataID_GetService
(
)
{
    LE_DEBUG("taf_diagDataID_GetService");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.GetService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter ReadDID and WriteDID request.
 * if the VLAN ID is not found or not exist, it will return error.
 * This function shall be called before registering RxReadDIDMsgHandler and RxWriteDIDMsgHandler.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDataID_SetVlanId
(
    taf_diagDataID_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDataID_RxReadDIDMsg'
 *
 * This event provides information about the Rx ReadDID message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDataID_RxReadDIDMsgHandlerRef_t taf_diagDataID_AddRxReadDIDMsgHandler
(
    taf_diagDataID_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDataID_RxReadDIDMsgHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDataID_AddRxReadDIDMsgHandler");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.AddRxReadDIDMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDataID_RxReadDIDMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDataID_RemoveRxReadDIDMsgHandler
(
    taf_diagDataID_RxReadDIDMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagRWDID_RemoveRxReadDIDMsgHandler");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.RemoveRxReadDIDMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx ReadDID message.
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
le_result_t taf_diagDataID_SendReadDIDResp
(
    taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Data payload.
    size_t dataSize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDataID_SendReadDIDResp");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.SendReadDIDResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDataID_RxWriteDIDMsg'
 *
 * This event provides information about the Rx ReadDID message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDataID_RxWriteDIDMsgHandlerRef_t taf_diagDataID_AddRxWriteDIDMsgHandler
(
    taf_diagDataID_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDataID_RxWriteDIDMsgHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDataID_AddRxWriteDIDMsgHandler");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.AddRxWriteDIDMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDataID_RxWriteDIDMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDataID_RemoveRxWriteDIDMsgHandler
(
    taf_diagDataID_RxWriteDIDMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDataID_RemoveRxWriteDIDMsgHandler");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.RemoveRxWriteDIDMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data record of the Rx WriteDID message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_OVERFLOW -- Payload size is too small.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDataID_GetWriteDataRecord
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* dataRecordPtr,
        ///< [OUT] Data record.
    size_t* dataRecordSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagDataID_GetWriteDataRecord");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.GetWriteDataRecord(rxMsgRef, dataRecordPtr, dataRecordSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx WriteDID message.
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
le_result_t taf_diagDataID_SendWriteDIDResp
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t errCode,
        ///< [IN] Error code type.
    uint16_t dataId
        ///< [IN] data identifier.
)
{
    LE_DEBUG("taf_diagDataID_SendWriteDIDResp");
#ifndef LE_CONFIG_DIAG_VSTACK
    auto &did = taf_DataIDSvr::GetInstance();
    return did.SendWriteDIDResp(rxMsgRef, errCode, dataId);
#else
    return LE_UNSUPPORTED;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vlan id of the Request ReadDID or WriteDID message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDataID_GetVlanIdFromMsg
(
    taf_diagDataID_RxMsgRef_t rxMsgRef,
        ///< [IN] ReadDID or WriteDID received message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    LE_DEBUG("taf_diagDataID_GetVlanIdFromMsg");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the DID service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDataID_RemoveSvc
(
    taf_diagDataID_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    LE_DEBUG("taf_diagDataID_RemoveSvc");
    auto &did = taf_DataIDSvr::GetInstance();
    return did.RemoveSvc(svcRef);
}
