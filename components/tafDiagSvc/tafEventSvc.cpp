/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafEventSvc.cpp
 * @brief      This file provides the telaf event management service as interfaces described
 *             in taf_diagEvent.api. The Diag Event Management service will be started automatically
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafEventSvr.hpp"

using namespace tafsvc;

//-------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a Event Management service. If there is no service, a new one will be
 * created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_ServiceRef_t taf_diagEvent_GetService
(
    uint16_t eventId
            ///< [IN]
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetService(eventId);
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the event id for the given service reference.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameter.
 *     - LE_FAULT -- Failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetId
(
   taf_diagEvent_ServiceRef_t svcRef,
   uint16_t* eventIdPtr
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetId(svcRef, eventIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets event status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_SetStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
    taf_diagEvent_StatusType_t eventStatus
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.SetStatus(svcRef, eventStatus);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets event status with supplier fault code.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_SetStatusWithSupplierFaultCode
(
    taf_diagEvent_ServiceRef_t svcRef,
    taf_diagEvent_StatusType_t eventStatus,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.SetStatusWithSupplierFaultCode(svcRef, eventStatus, supplierFaultCodePtr,
            supplierFaultCodeSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets event UDS status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetUdsStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint8_t* eventUdsStatusPtr
        ///< [OUT] Event UDS status.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetUdsStatus(svcRef, eventUdsStatusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resets debounce status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_ResetDebounceStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagEvent_DebounceResetStatus_t status
        ///< [IN] Event UDS status.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.ResetDebounceStatus(svcRef, status);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagEvent_UdsStatus'
 *
 * This event provides information on UDS status.
 */
//--------------------------------------------------------------------------------------------------
taf_diagEvent_UdsStatusHandlerRef_t taf_diagEvent_AddUdsStatusHandler
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagEvent_UdsStatusHandlerFunc_t handlerPtr,
        ///< [IN] UDS status handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t udsStatusEventId = diagEvent.GetUdsStatusEvent(svcRef);
    if(udsStatusEventId == NULL)
    {
        LE_ERROR("Uds status event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UdsStatusHandler",
        udsStatusEventId, diagEvent.FirstLayerUdsStatusHandler,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagEvent_UdsStatusHandlerRef_t)(handlerRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagEvent_UdsStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagEvent_RemoveUdsStatusHandler
(
    taf_diagEvent_UdsStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagEvent_EnableCondState'
 *
 * This event provides information on Enable Condition state.
 */
//--------------------------------------------------------------------------------------------------
taf_diagEvent_EnableCondStateHandlerRef_t taf_diagEvent_AddEnableCondStateHandler
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagEvent_EnableCondStateHandlerFunc_t handlerPtr,
        ///< [IN] Enable Condition state handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t enableCondStateEventId = diagEvent.GetEnableCondStateEvent(svcRef);
    if(enableCondStateEventId == NULL)
    {
        LE_ERROR("Enable Condition state event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("EnableCondStateHandler",
        enableCondStateEventId, diagEvent.FirstLayerEnableCondStateHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagEvent_EnableCondStateHandlerRef_t)(handlerRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagEvent_EnableCondState'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagEvent_RemoveEnableCondStateHandler
(
    taf_diagEvent_EnableCondStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the Event Management service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_RemoveSvc
(
    taf_diagEvent_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.RemoveSvc(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DTC code.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid service reference.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetDTCCode
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint32_t* dtcCodePtr
        ///< [OUT] DTC code.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetDTCCode(svcRef, dtcCodePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the Enable Condition state.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid service reference.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetEnableCondState
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    bool* statePtr
        ///< [OUT] Enable Condition state.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetEnableCondState(svcRef, statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the operation cycle ID.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid service reference.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetOperationCycleId
(
    taf_diagEvent_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint8_t* operationCycleIdPtr
        ///< [OUT] Operation cycle ID.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetOperationCycleId(svcRef, operationCycleIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the operation cycle reference for the given operation cycle ID.
 *
 * @return
 *  - NULL -- Error.
 *  - Others -- The operation cycle reference.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_diagEvent_OpCycleRef_t taf_diagEvent_GetOpCycle
(
    uint8_t opCycleId
        ///< [IN]
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetOperCycle(opCycleId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagEvent_OpCycleState'
 *
 * This event provides information on Operation Cycle state.
 */
//--------------------------------------------------------------------------------------------------
taf_diagEvent_OpCycleStateHandlerRef_t taf_diagEvent_AddOpCycleStateHandler
(
    taf_diagEvent_OpCycleRef_t opCycleRef,
        ///< [IN] Operation Cycle reference.
    taf_diagEvent_OpCycleStateHandlerFunc_t handlerPtr,
        ///< [IN] Operation Cycle state handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t operCycleStateEventId = diagEvent.GetOperCycleStateEvent(opCycleRef);
    if(operCycleStateEventId == NULL)
    {
        LE_ERROR("opCycleRef is invalid");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("OperCycleStateHandler",
        operCycleStateEventId, diagEvent.FirstLayerOperCycleStateHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagEvent_OpCycleStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagEvent_OperationCycleState'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagEvent_RemoveOpCycleStateHandler
(
    taf_diagEvent_OpCycleStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the state of an operation cycle.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_SetOpCycleState
(
    taf_diagEvent_OpCycleRef_t opCycleRef,
        ///< [IN] Operation Cycle reference.
    taf_diagEvent_OperationCycleState_t state
        ///< [IN] Operation cycle state.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.SetOperCycleState(opCycleRef, state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of an operation cycle.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetOpCycleState
(
    taf_diagEvent_OpCycleRef_t opCycleRef,
        ///< [IN] Operation Cycle reference.
    taf_diagEvent_OperationCycleState_t* statePtr
        ///< [OUT] Operation cycle state.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetOperCycleState(opCycleRef, statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the operation cycle ID for the given operation cycle reference.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid reference.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_GetOpCycleIdByRef
(
    taf_diagEvent_OpCycleRef_t opCycleRef,
        ///< [IN] Operation Cycle reference.
    uint8_t* opCycleIdPtr
        ///< [OUT] Operation cycle ID.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.GetOperCycleIdByRef(opCycleRef, opCycleIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the operation cycle with the given reference. Once an operation cycle is deleted, the
 * operation cycle reference is no longer valid.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagEvent_RemoveOpCycle
(
    taf_diagEvent_OpCycleRef_t opCycleRef
        ///< [IN] Operation Cycle reference.
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    return diagEvent.RemoveOperCycle(opCycleRef);
}
