/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafUDSStack.h
 */

#ifndef TAFUDSSTACK_H
#define TAFUDSSTACK_H

#include "legato.h"

#define MAX_INTERFACE_NAME_LEN 30
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
    SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER = 0x2F,
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
 * Enumeration of data type.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UDS_DATA_TYPE_ROLE   = 0x00,              ///< Role.
    TAF_UDS_DATA_TYPE_FILEXFER_STATE = 0x01,      ///< File transfer state
    TAF_UDS_DATA_TYPE_DIAG_PAUSE = 0x02,          ///< Pause diag service
    TAF_UDS_DATA_TYPE_DIAG_RESUME = 0x03          ///< Resume diag service
}taf_uds_DataType_t;

//-------------------------------------------------------------------------------------------------
/**
 * Logical address information structure in uds communication.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t            sa;         ///< Source address of message senders.
    uint16_t            ta;         ///< Target address of message recipients.
    taf_uds_TaType_t    taType;     ///< Target address type of message recipients.
    uint16_t            vlanId;     ///< VLAN ID. =0 if the interface is not vlan port.
    char                ifName[MAX_INTERFACE_NAME_LEN]; ///< Interface name.
}taf_uds_AddrInfo_t;

typedef struct
{
    uint8_t*            dataPtr;    ///< Data pointer.
    size_t              dataLen;    ///< Data length.
}taf_uds_DiagMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * FileXfer state info.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t            vlanId; ///< VLAN ID. =0 if the interface is not vlan port.
    bool                state;  ///< File transfer state.
    char                ifName[MAX_INTERFACE_NAME_LEN]; ///< Interface name.
    le_dls_Link_t       link;
}taf_uds_FileXferState_t;

//-------------------------------------------------------------------------------------------------
/**
 * Vlan Id info.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t            vlanId;
    le_dls_Link_t       link;
}taf_uds_VlanId_t;

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

//-------------------------------------------------------------------------------------------------
/**
 * Sets data to UDS stack.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_COMM_ERROR     Sending message error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_uds_SetData
(
    taf_uds_AddrInfo_t*  addrInfoPtr,             ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,        ///< [IN] Data pointer.
    taf_uds_DataType_t dataType                   ///< [IN] Data type.
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
 * Stop all DoIP connections.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_uds_Stop();

//-------------------------------------------------------------------------------------------------
/**
 * Removes the UDS indication handler.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_uds_RemoveDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerRef_t handerRef    ///< [IN] The handler reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Gets file transfer state.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_uds_GetFileXferActiveStateList
(
    le_dls_List_t* fileXferStateListPtr    ///< [IN] The file transfer state list.
);

//-------------------------------------------------------------------------------------------------
/**
 * Gets Vlan ID list. If the interface is not vlan port then VLAN list will provide VLAN ID = 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_uds_GetVlanIdList
(
    le_dls_List_t* vlanIDListPtr    ///< [IN] Vlan ID list.
);

#ifdef  __cplusplus
}
#endif
#endif
