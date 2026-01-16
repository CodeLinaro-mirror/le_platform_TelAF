/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

using namespace tafsvc;

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
    auto &did = taf_DataIDSvr::GetInstance();
    return did.SendWriteDIDResp(rxMsgRef, errCode, dataId);
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
    auto &did = taf_DataIDSvr::GetInstance();
    return did.RemoveSvc(svcRef);
}
