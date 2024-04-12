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

/**
 * @file tafUDSStack.h
 */

#ifndef TAFUDSSTACK_H
#define TAFUDSSTACK_H

#include "legato.h"

#ifdef  __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for UDS data indication handler.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_uds_DiagIndicationHandlerRef* taf_uds_DiagIndicationHandlerRef_t;

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of supported UDS service IDs.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    SID_DIAGNOSTIC_SESSION_CONTROL = 0x10,
    SID_CLEAR_DIAGNOSTIC_INFO = 0x14,
    SID_READ_DTC_INFO = 0x19,
    SID_READ_DATA_BY_IDENTIFIER = 0x22,
    SID_SECURITY_ACCESS = 0x27,
    SID_WRITE_DATA_BY_IDENTIFIER = 0x2E,
    SID_ROUTINE_CONTROL = 0x31,
    SID_TRANSFER_DATA = 0x36,
    SID_REQUEST_TRANSFER_EXIT = 0x37,
    SID_REQUEST_FILE_TRANSFER = 0x38,
    SID_CONTROL_DTC_SETTING = 0x85
}taf_uds_ServiceId_t;

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of supported logical target address types.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UDS_TA_TYPE_PHYSICAL   = 0x00,    ///< Physical addressing.
    TAF_UDS_TA_TYPE_FUNCTIONAL = 0x01     ///< Functional addressing.
}taf_uds_TaType_t;

//-------------------------------------------------------------------------------------------------
/**
 * Logical address information structure in uds communication.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t            sa;         ///< Source address of message senders.
    uint16_t            ta;         ///< Target address of message recipients.
    taf_uds_TaType_t   taType;      ///< Target address type of message recipients.
}taf_uds_AddrInfo_t;

typedef struct
{
    uint8_t*            dataPtr;    ///< Data pointer.
    size_t              dataLen;    ///< Data length.
}taf_uds_DiagMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Sends a diagnostic message.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_COMM_ERROR     Sending message error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_uds_SendDiagResp
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,     ///< [IN] Diagnostic message pointer.
    taf_uds_ServiceId_t serviceId,             ///< [IN] Service Id.
    uint8_t err                                ///< [IN] Error code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback to indicate uds message.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_uds_DiagIndicationHandlerFunc_t)
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,     ///< [IN] Diagnostic message pointer.
    le_result_t                result,         ///< [IN] The result.
    void*                      userPtr         ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to indicate UDS message.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_uds_DiagIndicationHandlerRef_t taf_uds_AddDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerFunc_t  indicationHandlerPtr,    ///< [IN] Hander function.
    void*                                userPtr                  ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Start a UDS.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_uds_Start(const char* configPathPtr);

//-------------------------------------------------------------------------------------------------
/**
 * Removes the UDS indication handler.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_uds_RemoveDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerRef_t handerRef    ///< [IN] The handler reference.
);

#ifdef  __cplusplus
}
#endif
#endif
