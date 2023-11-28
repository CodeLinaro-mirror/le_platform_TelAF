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

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafUpdateSvr.hpp"

using namespace telux::tafsvc;

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
