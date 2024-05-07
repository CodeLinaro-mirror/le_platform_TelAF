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
#ifndef TAFDOIP_STACK_H
#define TAFDOIP_STACK_H

#include "legato.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define TAF_DOIP_ACTIVATION_LINE_OFF 0
#define TAF_DOIP_ACTIVATION_LINE_ON  1

#define TAF_DOIP_IFNAME_SIZE 10

#define TAF_DOIP_VIN_SIZE 17    ///< Vehicle identification number size.
#define TAF_DOIP_EID_SIZE 6     ///< Entify identification size.
#define TAF_DOIP_GID_SIZE 6     ///< Group identification size.

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of diagnostic over IP(DoIP) results.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_DOIP_RESULT_OK  = 0,            ///< Successful.
    TAF_DOIP_RESULT_HDR_ERROR,          ///< DoIP header data format error.
    TAF_DOIP_RESULT_TIMEOUT_A,          ///< Communication timeout.
    TAF_DOIP_RESULT_UNKNOWN_SA,         ///< Source address is unknown.
    TAF_DOIP_RESULT_INVALID_SA,         ///< Source address is invalid.
    TAF_DOIP_RESULT_UNKNOWN_TA,         ///< Target address is unknown.
    TAF_DOIP_RESULT_MESSAGE_TOO_LARGE,  ///< Payload size is too large.
    TAF_DOIP_RESULT_OUT_OF_MEMORY,      ///< Insufficient memory.
    TAF_DOIP_RESULT_TARGET_UNREACHABLE, ///< Target address cannot be reached.
    TAF_DOIP_RESULT_NO_LINK,            ///< No link.
    TAF_DOIP_RESULT_NO_SOCKET,          ///< No socket.
    TAF_DOIP_RESULT_NOT_INIT,           ///< Call interface order error.
    TAF_DOIP_RESULT_NOT_START,          ///< Session is not started.
    TAF_DOIP_RESULT_PARAM_ERROR,        ///< The parameter is incorrect.
    TAF_DOIP_RESULT_ROUTING_INACTIVE,   ///< Routing is inactive.
    TAF_DOIP_RESULT_NETWORK_ERROR,      ///< Network error.
    TAF_DOIP_RESULT_MESSAGE_TOO_SHORT,  ///< DoIP message is too short.
    TAF_DOIP_RESULT_UNSET,              ///< Parameter is not set.
    TAF_DOIP_RESULT_SA_REGISTERED,      ///< A tester/SA is registered.
    TAF_DOIP_RESULT_SA_DEREGISTERED,    ///< A tester/SA is deregistered.
    TAF_DOIP_RESULT_ERROR = 0xFF        ///< Common error.
}taf_doip_Result_t;

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of diagnostic over IP(DoIP) event type.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_DOIP_EVENT_CONNECTION  = 0,            ///< Connection.
    TAF_DOIP_EVENT_DISCONNECTION               ///< Disconnection.
}taf_doip_Event_t;

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of supported logical target address types.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_DOIP_TA_TYPE_PHYSICAL   = 0x00,    ///< Physical addressing.
    TAF_DOIP_TA_TYPE_FUNCTIONAL = 0x01     ///< Functional addressing.
}taf_doip_TaType_t;

//-------------------------------------------------------------------------------------------------
/**
 * Enumeration of power mode status
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_DOIP_POWER_MODE_NOT_READY     = 0,  ///< Diagnostic power is not ready.
    TAF_DOIP_POWER_MODE_READY,              ///< Diagnostic power is ready.
    TAF_DOIP_POWER_MODE_NOT_SUPPORTED       ///< Not supported diagnostic power.
}taf_doip_PowerMode_t;

//-------------------------------------------------------------------------------------------------
/**
 * Logical address information structure in DoIP communication.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t            sa;         ///< Source address of message senders.
    uint16_t            ta;         ///< Target address of message recipients.
    taf_doip_TaType_t   taType;     ///< Target address type of message recipients.
}taf_doip_AddrInfo_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t*            dataPtr;    ///< Data pointer.
    size_t              dataLen;    ///< Data length.
}taf_doip_DiagMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for DoIP entity.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_doip_Ref* taf_doip_Ref_t;

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for power mode query handler.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_doip_PowerModeQueryHandlerRef* taf_doip_PowerModeQueryHandlerRef_t;

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for diagnostic data indication handler.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_doip_DiagIndicationHandlerRef* taf_doip_DiagIndicationHandlerRef_t;

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for diagnostic data confirm handler.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_doip_DiagConfirmHandlerRef* taf_doip_DiagConfirmHandlerRef_t;

//-------------------------------------------------------------------------------------------------
/**
 * Reference type for DoIP event handler.
 */
//-------------------------------------------------------------------------------------------------
typedef struct taf_doip_EventHandlerRef* taf_doip_EventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Callback to query power mode when diagnostic power mode information request reception.
 */
//--------------------------------------------------------------------------------------------------
typedef taf_doip_PowerMode_t (*taf_doip_PowerModeQueryHandlerFunc_t)
(
    void*   userPtr ///< [IN] User-defined pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback to indicate diagnostic data or connection registered event has received.
 * It indicates connection event when result is TAF_DOIP_RESULT_SA_REGISTERED/
 * TAF_DOIP_RESULT_SA_DEREGISTERED. In this case, diagMsgPtr is NULL.
 * Otherwise, it indicates reception of diagnostic data.
 *
 * @note diagMsgPtr can be NULL if it indicates connection registered event.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_doip_DiagIndicationHandlerFunc_t)
(
    const taf_doip_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_doip_DiagMsg_t*   diagMsgPtr,     ///< [IN] Diagnostic message pointer.
    taf_doip_Result_t           result,         ///< [IN] Result of the indication execution.
    void*                       userPtr         ///< [IN] User-defined pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback to confirm the completion of dianostic data request.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_doip_DiagConfirmHandlerFunc_t)
(
    const taf_doip_AddrInfo_t*  addrInfoPtr, ///< [IN] Logical address information pointer.
    taf_doip_Result_t           result,      ///< [IN] Result of the confirm execution
    void*                       userPtr      ///< [IN] User-defined pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback to DoIP event.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_doip_EventHandlerFunc_t)
(
    taf_doip_Ref_t              doipRef,     ///< [IN] DoIP entity reference.
    taf_doip_Event_t            event,       ///< [IN] Result of the event.
    uint16_t                    remoteAddr,  ///< [IN] Remote logical address.
    void*                       userPtr      ///< [IN] User-defined pointer
);

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/**
 * Creates a DoIP entity reference with the given configuration.
 *
 * @return
 *  - Reference to the created DoIP entity.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_Ref_t taf_doip_Create
(
    const char* configPathPtr   ///< [IN] DoIP configuration path pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Get a DoIP entity reference with the source address.
 *
 * @return
 *  - Reference to the DoIP entity.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_Ref_t taf_doip_Get
(
    uint16_t sa     ///< [IN] DoIP entity source address.
);

//-------------------------------------------------------------------------------------------------
/**
 * Deletes a previously created DoIP entity and free allocated resources.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_Delete
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Starts a DoIP entity.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_Start
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Stops a DoIP entity.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_Stop
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle vehicle identification number(VIN).
 *
 * @note This function must be called before taf_doip_Start().
 *       VIN is 17 bytes length and shall follow ISO3779.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_SetVin
(
    const char*     vinPtr      ///< [IN] Vehicle identification number pointer.17bytes length.
);

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle vehicle identification number(VIN).
 *
 * @note VIN buffer size must be more than 17bytes.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_GetVin
(
    char*     vinPtr,      ///< [OUT] Vehicle identification number pointer.
    size_t    vinSize      ///< [IN] vinPtr buffer size.
);

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle group identification(GID).
 *
 * @note This function must be called before taf_doip_Start().
 *       Recommends to use MAC address as the GID. It can be defined such as 'xx:xx:xx:xx:xx:xx'
 *       or 'xxxxxxxxxxxx', x is in range of '0-F'.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_SetGid
(
    const char*     gidPtr      ///< [IN] Group identification pointer. 12bytes length.
);

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle group identification(GID).
 *
 * @note GID buffer size must be more than 12bytes.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_GetGid
(
    char*     gidPtr,      ///< [OUT] Group identification pointer.
    size_t    gidSize      ///< [IN] gidPtr buffer size.
);

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle entity identification(EID).
 *
 * @note This function must be called before taf_doip_Start().
 *       Recommends to use MAC address as the EID. It can be defined such as 'xx:xx:xx:xx:xx:xx'
 *       or 'xxxxxxxxxxxx', x is in range of '0-F'.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_SetEid
(
    const char*     eidPtr      ///< [IN] Entity identification pointer.12bytes length.
);

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle entity identification(EID).
 *
 * @note EID buffer size must be more than 12bytes.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_GetEid
(
    char*     eidPtr,      ///< [OUT] Entity identification pointer.
    size_t    eidSize      ///< [IN] eidPtr buffer size.
);

//-------------------------------------------------------------------------------------------------
/**
 * Sets the DoIP entity source address.
 *
 * @note This function must be called before taf_doip_Start().
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_SetSourceAddr
(
    taf_doip_Ref_t  doipRef,    ///< [IN] DoIP entity reference.
    uint16_t        sa          ///< [IN] Source address.
);

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to query power mode status.
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_PowerModeQueryHandlerRef_t taf_doip_AddPowerModeQueryHandler
(
    taf_doip_Ref_t                       doipRef,            ///< [IN] DoIP entity reference.
    taf_doip_PowerModeQueryHandlerFunc_t pmQueryHandlerPtr,  ///< [IN] Hander function.
    void*                                userPtr             ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Removes the query power mode status handler.
 *
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_doip_RemovePowerModeQueryHandler
(
    taf_doip_PowerModeQueryHandlerRef_t handerRef    ///< [IN] The handler reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to indicate diagnostic message or connection registered event.
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_DiagIndicationHandlerRef_t taf_doip_AddDiagIndicationHandler
(
    taf_doip_Ref_t                       doipRef,                ///< [IN] DoIP entity reference.
    taf_doip_DiagIndicationHandlerFunc_t indicationHandlerPtr,   ///< [IN] Hander function.
    void*                                userPtr                 ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Removes the diagnostic indication handler.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_doip_RemoveDiagIndicationHandler
(
    taf_doip_DiagIndicationHandlerRef_t handerRef   ///< [IN] The handler reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to confirm diagnostic message request result.
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_DiagConfirmHandlerRef_t taf_doip_AddDiagConfirmHandler
(
    taf_doip_Ref_t                    doipRef,              ///< [IN] DoIP entity reference.
    taf_doip_DiagConfirmHandlerFunc_t confirmHandlerPtr,    ///< [IN] Hander function.
    void*                             userPtr               ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Removes the diagnostic confirmation handler.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_doip_RemoveDiagConfirmHandler
(
    taf_doip_DiagConfirmHandlerRef_t handerRef  ///< [IN] The handler reference.
);

//-------------------------------------------------------------------------------------------------
/**
 * Sends a diagnostic message.
 *
 * @note Shall be called after taf_doip_Start(). If DoIP entity is not started,
 *       it will fail.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_COMM_ERROR     Sending message error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_DiagRequest
(
    const taf_doip_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_doip_DiagMsg_t*   diagMsgPtr      ///< [IN] Diagnostic message pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to notify DoIP event.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_EventHandlerRef_t taf_doip_AddEventHandler
(
    taf_doip_Ref_t                    doipRef,              ///< [IN] DoIP entity reference.
    taf_doip_EventHandlerFunc_t       handlerPtr,           ///< [IN] Hander function.
    void*                             userPtr               ///< [IN] User-defined pointer.
);

//-------------------------------------------------------------------------------------------------
/**
 * Removes the DoIP event handler.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_doip_RemoveEventHandler
(
    taf_doip_EventHandlerRef_t eventHandlerRef  ///< [IN] DoIP event handler reference.
);

#ifdef  __cplusplus
}
#endif

#endif  // TAFDOIP_STACK_H
