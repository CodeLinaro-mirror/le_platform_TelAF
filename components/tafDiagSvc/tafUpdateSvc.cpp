/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafUpdateSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets or creates the reference to a Diag update service.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagUpdate_ServiceRef_t taf_diagUpdate_GetService
(
    void
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.CreateUpdateSvc();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter RequestFileTransfer/TransferData/
 * RequestTransferExit request.
 * if the VLAN ID is not found or not exist, it will return error.
 * This function shall be called before registering RxFileXferMsgHandler,
 * RxXferDataMsgHandler and RxXferExitMsgHandler.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_SetVlanId
(
    taf_diagUpdate_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagUpdate_RxFileXferMsg'
 *
 * This event provides information on RequestFileTransfer message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagUpdate_RxFileXferMsgHandlerRef_t taf_diagUpdate_AddRxFileXferMsgHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagUpdate_RxFileXferMsgHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.AddRxFileXferReqHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagUpdate_RxFileXferMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagUpdate_RemoveRxFileXferMsgHandler
(
    taf_diagUpdate_RxFileXferMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    tafUpdateSvr.RemoveRxFileXferReqHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the file path and name of the Rx RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetFilePathAndName
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint8_t* fileNamePtr,
        ///< [OUT] File path and name.
    size_t* fileNameSizePtr
        ///< [INOUT]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetFilePathAndName(rxMsgRef, fileNamePtr, fileNameSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data format ID of the Rx RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetDataFormatID
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint8_t* dataFormatIDPtr
        ///< [OUT] Data format ID.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetDataFormatID(rxMsgRef, dataFormatIDPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of the uncompressed file of the Rx RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetUnCompFileSize
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint32_t* unCompFileSizePtr
        ///< [OUT] Size of the uncompressed file.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetUnCompFileSize(rxMsgRef, unCompFileSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of the compressed file of the Rx RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetCompFileSize
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint32_t* compFileSizePtr
        ///< [OUT] Size of the compressed file.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetCompFileSize(rxMsgRef, compFileSizePtr);
}

le_result_t taf_diagUpdate_SetFilePosition
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint64_t filePosition
)
{
    return taf_UpdateSvr::GetInstance().SetFilePosition(rxMsgRef,
                                                        filePosition);
}

le_result_t taf_diagUpdate_SetFileSizeOrDirInfoLength
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint64_t fileSizeUncompressedOrDirInfoLength,
    uint64_t fileSizeCompressed
)
{
    return taf_UpdateSvr::GetInstance().SetFileSizeOrDirInfoLength(
                                          rxMsgRef,
                                          fileSizeUncompressedOrDirInfoLength,
                                          fileSizeCompressed);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx RequestFileTransfer message.
 *
 * @note
 *     - This function must be called to send a response
 *       if receiving a RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_SendFileXferResp
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    taf_diagUpdate_FileXferErrorCode_t errCode
        ///< [IN] Error code type.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.SendFileXferResp(rxMsgRef, errCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagUpdate_RxXferDataMsg'
 *
 * This event provides information on TransferData message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagUpdate_RxXferDataMsgHandlerRef_t taf_diagUpdate_AddRxXferDataMsgHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagUpdate_RxXferDataMsgHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.AddRxXferDataReqHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagUpdate_RxXferDataMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagUpdate_RemoveRxXferDataMsgHandler
(
    taf_diagUpdate_RxXferDataMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    tafUpdateSvr.RemoveRxXferDataReqHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the block sequence counter of the Rx TransferData message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetblockSeqCount
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* countPtr
        ///< [OUT] Block sequence counter.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetblockSeqCount(rxMsgRef, countPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the the data request record length of the Rx TransferData message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetXferDataParamRecLen
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* xferDataRecLenPtr
        ///< [OUT] Transfer data request parameter record length.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetXferDataParamRecLen(rxMsgRef, xferDataRecLenPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the transfer data request record of the Rx TransferData message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetXferDataParamRec
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* xferDataRecPtr,
        ///< [OUT] Transfer data request parameter record.
    size_t* xferDataRecSizePtr
        ///< [INOUT]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetXferDataParamRec(rxMsgRef, xferDataRecPtr, xferDataRecSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx TransferData message.
 *
 * @note
 *     - This function must be called to send a response
 *       if receiving a TransferData message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_SendXferDataResp
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    taf_diagUpdate_XferDataErrorCode_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Payload data.
    size_t dataSize
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.SendXferDataResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagUpdate_RxXferExitMsg'
 *
 * This event provides information on RequestTransferExit message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagUpdate_RxXferExitMsgHandlerRef_t taf_diagUpdate_AddRxXferExitMsgHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagUpdate_RxXferExitMsgHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.AddRxXferExitReqHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagUpdate_RxXferExitMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagUpdate_RemoveRxXferExitMsgHandler
(
    taf_diagUpdate_RxXferExitMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.RemoveRxXferExitReqHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the length of the received RequestTransferExit message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetXferExitParamRecLen
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* exitDataRecLenPtr
        ///< [OUT] Transfer exit request parameter record length.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetXferExitParamRecLen(rxMsgRef, exitDataRecLenPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the transfer exit request parameter record of the Rx RequestTransferExit message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetXferExitParamRec
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* exitDataRecPtr,
        ///< [OUT] Transfer exit request parameter record.
    size_t* exitDataRecSizePtr
        ///< [INOUT]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetXferExitParamRec(rxMsgRef, exitDataRecPtr, exitDataRecSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx RequestTransferExit message.
 *
 * @note
 *     - This function must be called to send a response
 *       if receiving a RequestFileTransfer message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_FAULT -- Failed.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_SendXferExitResp
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    taf_diagUpdate_XferExitErrorCode_t errCode,
        ///< [IN] Error code type.
    const uint8_t* dataPtr,
        ///< [IN] Payload data.
    size_t dataSize
        ///< [IN]
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.SendXferExitResp(rxMsgRef, errCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vlan id of the Request RequestFileTransfer/TransferData/RequestTransferExit message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_GetVlanIdFromMsg
(
    taf_diagUpdate_RxMsgRef_t rxMsgRef,
        ///< [IN] ReadDID or WriteDID received message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagUpdate_NrcStatus'
 *
 * This event provides information on NRC.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
taf_diagUpdate_NrcStatusHandlerRef_t taf_diagUpdate_AddNrcStatusHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagUpdate_NrcStatusHandlerFunc_t handlerPtr,
        ///< [IN] Nrc status handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &tafUpdateSvr = taf_UpdateSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NRCStatusHandler",
        tafUpdateSvr.NrcEventId, tafUpdateSvr.FirstLayerNrcStatusHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagUpdate_NrcStatusHandlerRef_t)(handlerRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagUpdate_NrcStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagUpdate_RemoveNrcStatusHandler
(
    taf_diagUpdate_NrcStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases a NRC status notification message.
 *
 * @instaging
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid statusRef or invalid service of the statusRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_ReleaseNrcStatusMsg
(
    taf_diagUpdate_NrcStatusRef_t statusRef
        ///< [IN] Tester state reference.
)
{
    auto &tafUpdateSvr = taf_UpdateSvr::GetInstance();
    return tafUpdateSvr.ReleaseNrcStatusMsg(statusRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the Update server service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagUpdate_RemoveSvc
(
    taf_diagUpdate_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();

    return tafUpdateSvr.RemoveUpdateSvc(svcRef);
}
