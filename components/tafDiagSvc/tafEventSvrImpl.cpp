/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafEventSvr.hpp"
#include "tafDTCSvr.hpp"
#include "tafDataAccessComp.h"
#include "tafSnapshotSvc.hpp"
#include "tafDiagSvr.hpp"
#include <algorithm>
#include <chrono>

using namespace telux::tafsvc;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(tafSessionRef, TAF_EVENT_MAX_SESSION_REF,
        sizeof(taf_diagEvent_SessionRef_t));

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF Event Management server.
 */
//--------------------------------------------------------------------------------------------------
taf_EventSvr &taf_EventSvr::GetInstance
(
)
{
    static taf_EventSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_ServiceRef_t taf_EventSvr::GetService
(
    uint16_t eventId
)
{
    // Search the service.
    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtxById(eventId);

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, NULL, "Event id is not supported");

    le_msg_SessionRef_t sessionRef = taf_diagEvent_GetClientSessionRef();
    //Service reference is not created
    if(eventCtxPtr->svcRef== NULL)
    {
        // Create a Safe Reference for this service object
        taf_diagEvent_ServiceRef_t eventRef = (taf_diagEvent_ServiceRef_t)le_ref_CreateRef(
                SvcRefMap, eventCtxPtr);
        TAF_ERROR_IF_RET_VAL(eventRef == NULL, NULL, "cannot alloc eventRef");

        eventCtxPtr->svcRef = eventRef;

        eventCtxPtr->sessionRefList = LE_DLS_LIST_INIT;

        LE_DEBUG("svcRef %p of client %p is created for event id %d.",
                eventCtxPtr->svcRef, sessionRef, eventId);
    }
    else
    {
        LE_DEBUG("server ref %p already created",eventCtxPtr->svcRef );
    }

    // Add client reference to the session list
    AddSessionToEventCtx(eventCtxPtr, sessionRef);

    return eventCtxPtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event Id by the given service reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::GetId
(
    taf_diagEvent_ServiceRef_t svcRef,
    uint16_t* eventIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    TAF_ERROR_IF_RET_VAL(eventIdPtr == NULL, LE_BAD_PARAMETER, "eventIdPtr is null");

    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "Fail to get event context");

    *eventIdPtr = eventCtxPtr->eventId;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set event status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
    taf_diagEvent_StatusType_t eventStatus
)
{
    uint8_t* supplierFaultCodePtr = NULL;
    size_t supplierFaultCodeSize = 0;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    return SetStatusWithSupplierFaultCode(svcRef, eventStatus, supplierFaultCodePtr,
            supplierFaultCodeSize);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set event status with supplier fault code.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetStatusWithSupplierFaultCode
(
    taf_diagEvent_ServiceRef_t svcRef,
    taf_diagEvent_StatusType_t eventStatus,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
#ifdef LE_CONFIG_DIAG_FEATURE_A
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(eventStatus == TAF_DIAGEVENT_UNKNOWN, LE_FAULT,
            "Incorrect event status");

    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtx(svcRef);
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "This event is not supported");

    LE_DEBUG("event id=%d, dtc code=0x%x, dtc type=%d", eventCtxPtr->eventId,
        eventCtxPtr->dtcCtxPtr->dtcCode, eventCtxPtr->dtcCtxPtr->dtcType);
    //Check condition
    if(!IsEventConditionOK(eventCtxPtr))
    {
        LE_ERROR("Condition check is not OK.");
        return LE_FAULT;
    }

    //Store supplier fault code
    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_REPROGRAMMING_DTC ||
            supplierFaultCodeSize <= 0 || supplierFaultCodePtr == NULL)
    {
        eventCtxPtr->supplierFaultCodeSize = 0;
        memset(eventCtxPtr->supplierFaultCode, 0, TAF_DIAGEVENT_SUPPLIER_FAULT_CODE_MAX_LEN);
    }
    else
    {
        memcpy(eventCtxPtr->supplierFaultCode, supplierFaultCodePtr, supplierFaultCodeSize);
        eventCtxPtr->supplierFaultCodeSize = supplierFaultCodeSize;
    }

    switch(eventStatus)
    {
        case TAF_DIAGEVENT_PASSED:
            result = UpdateEventOnPassedCustomerN(eventCtxPtr);
            break;
        case TAF_DIAGEVENT_FAILED:
            result = UpdateEventOnFailedCustomerN(eventCtxPtr);
            break;
        break;
        case TAF_DIAGEVENT_PREPASSED:
            result = UpdateEventOnPrePassedCustomerN(eventCtxPtr);
            break;
        break;
        case TAF_DIAGEVENT_PREFAILED:
            result = UpdateEventOnPreFailedCustomerN(eventCtxPtr);
            break;
        break;
        case TAF_DIAGEVENT_CONFIRMED:
            result = UpdateEventOnConfirmedCustomerN(eventCtxPtr);
            break;
        break;
        case TAF_DIAGEVENT_TEST_NOT_COMPLETED:
            result = UpdateEventOnTestNotCmpltCustomerN(eventCtxPtr);
            break;
        break;
        case TAF_DIAGEVENT_UNKNOWN:
            LE_ERROR("Incorrect event fault status");
            result = LE_FAULT;
        break;
    }

    return result;
#else
    le_result_t result;
    taf_SnapshotSvr& ss = taf_SnapshotSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(eventStatus == TAF_DIAGEVENT_UNKNOWN, LE_FAULT,
            "Incorrect event status");

    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtx(svcRef);
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "This event is not supported");

    taf_diagEvent_StatusType_t oldEventFaultStatus = eventCtxPtr->eventFaultStatus;

    //Check condition
    if(!IsEventConditionOK(eventCtxPtr))
    {
        LE_ERROR("Condition check is not OK.");
        return LE_FAULT;
    }

    //Store supplier fault code
    if(supplierFaultCodeSize > 0 && supplierFaultCodePtr != NULL)
    {
        memcpy(eventCtxPtr->supplierFaultCode, supplierFaultCodePtr, supplierFaultCodeSize);
        eventCtxPtr->supplierFaultCodeSize = supplierFaultCodeSize;
    }
    else
    {
        eventCtxPtr->supplierFaultCodeSize = 0;
        memset(eventCtxPtr->supplierFaultCode, 0, TAF_DIAGEVENT_SUPPLIER_FAULT_CODE_MAX_LEN);
    }

    LE_DEBUG("----Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
            cfg::DEM_TRIGGER_ON_EVERY_TEST_FAILED);
    ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_EVERY_TEST_FAILED,
            eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);

    //debounce the event
    result = DebounceEvent(eventCtxPtr, eventStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to debounce.");
        return result;
    }

    LE_DEBUG("old fault status =%d, new status=%d", oldEventFaultStatus,
            eventCtxPtr->eventFaultStatus);

    if(oldEventFaultStatus != eventCtxPtr->eventFaultStatus)
    {
        switch(eventCtxPtr->eventFaultStatus)
        {
            case TAF_DIAGEVENT_FAILED:
                result = UpdateEventOnFailed(eventCtxPtr);
                break;
            case TAF_DIAGEVENT_PASSED:
                result = UpdateEventOnPassed(eventCtxPtr);
                break;
            case TAF_DIAGEVENT_PREFAILED:
            case TAF_DIAGEVENT_PREPASSED:
                result = LE_OK;
                break;
            default:
                LE_ERROR("Incorrect event fault status");
                result = LE_FAULT;
                break;
        }
    }

    return result;
#endif
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event UDS status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::GetUdsStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
    uint8_t* eventUdsStatusPtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(eventUdsStatusPtr == NULL, LE_BAD_PARAMETER, "eventUdsStatusPtr is null");

    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    *eventUdsStatusPtr = eventCtxPtr->eventUdsStatus;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event context by event id.
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_EventCtx_t * taf_EventSvr::GetEventCtxById
(
    uint16_t eventId
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&EventCtxList);
    while (linkPtr)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_EventCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&EventCtxList, linkPtr);

        if (eventCtxPtr->eventId == eventId)
        {
            LE_DEBUG("Get eventCtxPtr %p by id %d", eventCtxPtr, eventId);
            return eventCtxPtr;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC context by DTC code.
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_DtcCtx_t * taf_EventSvr::GetDtcCtxByCode
(
    uint32_t dtcCode
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DtcCtxList);
    while (linkPtr)
    {
        taf_diagEvent_DtcCtx_t* dtcCtx = CONTAINER_OF(linkPtr, taf_diagEvent_DtcCtx_t, link);
        linkPtr = le_dls_PeekNext(&DtcCtxList, linkPtr);
        if (dtcCtx->dtcCode == dtcCode)
        {
            LE_DEBUG("Get dtcCtx %p by id %d", dtcCtx, dtcCode);
            return dtcCtx;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Check the condition for the event.
 */
//-------------------------------------------------------------------------------------------------
bool taf_EventSvr::IsEventConditionOK
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
#ifndef LE_CONFIG_DIAG_FEATURE_A
    //Check operation cycle
    if(OperationCycleStates[eventCtxPtr->operationCycleId] != TAF_DIAGEVENT_CYCLE_START)
    {
        LE_ERROR("Operation cycle is not started.");
        return false;
    }
#endif
    //enable condition
    if (!eventCtxPtr->eventEnableStatus)
    {
        LE_ERROR("Enable condition is not fullfilled.");
        return false;
    }

    //DTC activated
    if(eventCtxPtr->dtcCtxPtr == NULL || (!eventCtxPtr->dtcCtxPtr->activationStatus))
    {
        LE_ERROR("DTC is inactive.");
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the allocated memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::RemoveSvc
(
    taf_diagEvent_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_FAULT, "svcRef is null");
    le_msg_SessionRef_t sessionRef = taf_diagEvent_GetClientSessionRef();

    //Find event context one by one
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = (taf_diagEvent_EventCtx_t*)le_ref_GetValue(iterRef);

        if (eventCtxPtr != NULL && eventCtxPtr->svcRef == svcRef)
        {
            //Remove client session reference from event session reference list
            if( RemoveSessionFromEventCtx(eventCtxPtr, sessionRef) == LE_OK)
                LE_DEBUG(" remove session %p, from eventCtxPtr %p with event id %d",
                        sessionRef, eventCtxPtr, eventCtxPtr->eventId);
        }

        //If session number of links is 0, release event context
        if( le_dls_NumLinks(&eventCtxPtr->sessionRefList)  == 0)
        {
            // Clear service object
            LE_DEBUG(" clear event id %d context", eventCtxPtr->eventId);
            le_ref_DeleteRef(SvcRefMap, (void*)eventCtxPtr->svcRef);
            eventCtxPtr->svcRef = NULL;
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets enable condition status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetEventEnableStatus
(
    uint8_t enableConditionID
)
{
    LE_DEBUG("SetEventEnableStatus");

    le_dls_Link_t* linkPtr = NULL;

    // Diag service instance
    auto &diag = taf_DiagSvr::GetInstance();

    linkPtr = le_dls_Peek(&EventCtxList);
    while (linkPtr)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_EventCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&EventCtxList, linkPtr);

        // Check Enable condition
        LE_DEBUG("Check enable condition for event Id 0x%x", eventCtxPtr->eventId);
        bool enableStatus = false; // Default enable status.
        try
        {
            LE_DEBUG("Enable condition status check");
            cfg::Node & eventNode = cfg::get_event_node(eventCtxPtr->eventId);
            cfg::Node & enableNode = eventNode.get_child("data_enable_condition");

            for (const auto & enable: enableNode)
            {
                // Get the defined enable operation type: "and" or "or"
                std::string enableOperation = enable.first;
                if (enableOperation == "and")
                {
                    LE_INFO("Check enable condition status based on AND operation");
                    cfg::Node & optNodeList = enableNode.get_child("and");
                    enableStatus = true;

                    // Check enable id belong to this event id or not.
                    bool isEnableIdAvailable = false;
                    for (const auto & optNode: optNodeList)
                    {
                        uint8_t enableId = optNode.second.get_value<uint8_t>();
                        if (enableId == enableConditionID)
                        {
                            isEnableIdAvailable = true;
                            break;
                        }
                    }

                    if (!isEnableIdAvailable)
                    {
                        LE_DEBUG("Enable Id 0x%x does not belong to this event 0x%x",
                                enableConditionID, eventCtxPtr->eventId);
                        break;
                    }

                    // Check Enable status based on all enable id belong to this event.
                    for (const auto & optNode: optNodeList)
                    {
                        uint8_t enableId = optNode.second.get_value<uint8_t>();
                        LE_DEBUG("Enable condition id = 0x%x", enableId);

                        if (!diag.GetEnableConditionStatus(enableId))
                        {
                            enableStatus = false;
                            LE_INFO("enable id %d status is false", enableId);
                            break;
                        }
                    }
                }
                else if (enableOperation == "or")
                {
                    LE_INFO("Check enable condition status based on OR operation");
                    cfg::Node & optNodeList = enableNode.get_child("or");
                    enableStatus = false;

                    // Check enable id belong to this event id or not.
                    bool isEnableIdAvailable = false;
                    for (const auto & optNode: optNodeList)
                    {
                        uint8_t enableId = optNode.second.get_value<uint8_t>();
                        if (enableId == enableConditionID)
                        {
                            isEnableIdAvailable = true;
                            break;
                        }
                    }

                    if (!isEnableIdAvailable)
                    {
                        LE_DEBUG("Enable Id 0x%x does not belong to this event 0x%x",
                                enableConditionID, eventCtxPtr->eventId);
                        break;
                    }

                    for (const auto & optNode: optNodeList)
                    {
                        uint8_t enableId = optNode.second.get_value<uint8_t>();
                        LE_DEBUG("Enable condition id = 0x%x", enableId);

                        if(diag.GetEnableConditionStatus(enableId))
                        {
                            enableStatus = true;
                            LE_INFO("enable id %d status is true", enableId);
                            break;
                        }
                    }
                }

                if(enableStatus != eventCtxPtr->eventEnableStatus)
                {
                    LE_DEBUG("Notify Enable Condition state");
                    ReportEnableCondState(eventCtxPtr, enableStatus);
                }
                // Set the overall enable status for eventId
                if(enableStatus)
                {
                    eventCtxPtr->eventEnableStatus = true;
                    LE_DEBUG("Enable condition for event id 0x%x is true", eventCtxPtr->eventId);
                }
                else
                {
                    eventCtxPtr->eventEnableStatus = false;
                    LE_DEBUG("Enable condition for event id 0x%x is false", eventCtxPtr->eventId);
                }
            }
        }
        catch (const std::exception& e)
        {
            LE_WARN("Enable condition id not found for eventID: 0x%x, Exception: %s",
                    eventCtxPtr->eventId, e.what());
        }

        // Debouncing behavior on enable condition
        if (eventCtxPtr->eventEnableStatus == false
                && (eventCtxPtr->debounceBehavior_ == TAF_DIAGEVENT_DEBOUNCE_RESET))
        {
            LE_DEBUG("Clear the counter for event id %d", eventCtxPtr->eventId);
            ResetDebounceCounter(eventCtxPtr);
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets operation cycle.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetOperationCycleState
(
    uint8_t operationCycleId,
    taf_diagEvent_OperationCycleState_t state
)
{
#ifndef LE_CONFIG_DIAG_FEATURE_A
    le_result_t result;

    if(operationCycleId >= MAX_OPERATION_CYCLE_NUM)
    {
        LE_ERROR("operationCycleId is too big");
        return LE_FAULT;
    }

    switch(state)
    {
        case TAF_DIAGEVENT_CYCLE_RESTART:
            if(OperationCycleStates[operationCycleId] == TAF_DIAGEVENT_CYCLE_START)
            {
                result = StopOperationCycle(operationCycleId);
                if(result != LE_OK)
                {
                    LE_ERROR("Failed to stop operation cycle");
                    return result;
                }

                result = StartOperationCycle(operationCycleId);
                if(result != LE_OK)
                {
                    LE_ERROR("Failed to start operation cycle");
                }
                return result;
            }
            else
            {
                LE_ERROR("Operation cycle is not started");
                return LE_FAULT;
            }
        break;
        case TAF_DIAGEVENT_CYCLE_START:
            if(OperationCycleStates[operationCycleId] != TAF_DIAGEVENT_CYCLE_START)
            {
                // Init status value when operation cycle start
                result = StartOperationCycle(operationCycleId);
                if(result != LE_OK)
                {
                    LE_ERROR("Failed to start operation cycle");
                    return result;
                }
                OperationCycleStates[operationCycleId] = TAF_DIAGEVENT_CYCLE_START;
            }
        break;
        case TAF_DIAGEVENT_CYCLE_STOP:
            if(OperationCycleStates[operationCycleId] != TAF_DIAGEVENT_CYCLE_STOP)
            {
                result = StopOperationCycle(operationCycleId);
                if(result != LE_OK)
                {
                    LE_ERROR("Failed to stop operation cycle");
                    return result;
                }
                OperationCycleStates[operationCycleId] = TAF_DIAGEVENT_CYCLE_STOP;
            }
        break;
    }

#endif
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Start operation cycle.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::StartOperationCycle
(
    uint8_t operationCycleId
)
{

    le_dls_Link_t* linkPtr = NULL;
    taf_diagEvent_DtcCtx_t *dtcCtxPtr = NULL;
    uint8_t oldEventUdsStatus, oldDtcStatus;
    le_result_t result;

    LE_DEBUG("StartOperationCycle, Id:%d", operationCycleId);
    linkPtr = le_dls_Peek(&EventCtxList);
    while (linkPtr)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_EventCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&EventCtxList, linkPtr);

        if (eventCtxPtr->operationCycleId == operationCycleId)
        {
            oldEventUdsStatus = eventCtxPtr->eventUdsStatus;
            eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_UNKNOWN;
            ResetDebounceCounter(eventCtxPtr);
            eventCtxPtr->fdcTriggerFlag = false;

            //Set bit 1 to value 0 for event UDS status
            eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TFTOC);
            //Set bit 6 to value 1 for event UDS status
            eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCTOC;
            LE_DEBUG("EventId:%d, status:0x%x->0x%x", eventCtxPtr->eventId, oldEventUdsStatus,
                    eventCtxPtr->eventUdsStatus);

            //Event uds status changed, store the data into database and send notification
            if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
            {
                //Store data and report event UDS status change
                result = StoreAndReportEventUdsStatus(eventCtxPtr);
                if(result != LE_OK)
                {
                    LE_ERROR("Failed to store event data");
                    return result;
                }
            }

            dtcCtxPtr = eventCtxPtr->dtcCtxPtr;
            if(dtcCtxPtr != NULL)
            {
                oldDtcStatus = dtcCtxPtr->dtcStatus;
                //Set bit 1 to value 0 for DTC status
                dtcCtxPtr->dtcStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TFTOC);
                //Set bit 6 to value 1 for DTC status
                dtcCtxPtr->dtcStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCTOC;
                LE_DEBUG("DTC code:0x%x, status:0x%x->0x%x", dtcCtxPtr->dtcCode, oldDtcStatus,
                        dtcCtxPtr->dtcStatus);
                //DTC status changed, store the data into database and send notification
                if(oldDtcStatus != dtcCtxPtr->dtcStatus)
                {
                    //Store data and report DTC status change
                    result = StoreAndReportDTCStatus(dtcCtxPtr);
                    if(result != LE_OK)
                    {
                        LE_ERROR("Failed to store DTC data");
                        return result;
                    }
                }
            }
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Stop operation cycle.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::StopOperationCycle
(
    uint8_t operationCycleId
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus, oldDtcStatus;
    le_dls_Link_t* linkPtr = NULL;
    taf_diagEvent_DtcCtx_t *dtcCtxPtr = NULL;

    LE_DEBUG("StopOperationCycle, Id:%d", operationCycleId);
    linkPtr = le_dls_Peek(&EventCtxList);
    while (linkPtr)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_EventCtx_t,
                link);

        linkPtr = le_dls_PeekNext(&EventCtxList, linkPtr);
        if (eventCtxPtr->operationCycleId == operationCycleId)
        {
            if( ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TNCTOC) == 0) &&
                    ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC) == 0))
            {
                if((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_PDTC) != 0)
                {
                    oldEventUdsStatus = eventCtxPtr->eventUdsStatus;
                    //Set bit 2 to value 0
                    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_PDTC);
                    LE_DEBUG("EventId:%d, status:0x%x->0x%x", eventCtxPtr->eventId,
                            oldEventUdsStatus, eventCtxPtr->eventUdsStatus);

                    //Event uds status changed, store the data into database and send notification
                    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
                    {
                        //Store data and report event UDS status change
                        result = StoreAndReportEventUdsStatus(eventCtxPtr);
                        if(result != LE_OK)
                        {
                            LE_ERROR("Failed to store event data");
                            return result;
                        }
                    }
                }
            }

            dtcCtxPtr = eventCtxPtr->dtcCtxPtr;
            if(eventCtxPtr->dtcCtxPtr != NULL)
            {
                if( ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TNCTOC) == 0) &&
                        ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC) == 0) &&
                        ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_PDTC) != 0))
                {
                    oldDtcStatus = dtcCtxPtr->dtcStatus;
                    //Set bit 2 to value 0
                    dtcCtxPtr->dtcStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_PDTC);
                    LE_DEBUG("DTC code:0x%x, status:0x%x->0x%x", dtcCtxPtr->dtcCode, oldDtcStatus,
                            dtcCtxPtr->dtcStatus);
                    //DTC status changed, store the data into database and send notification
                    if(oldDtcStatus != dtcCtxPtr->dtcStatus)
                    {
                        //Store data and report DTC status change
                        result = StoreAndReportDTCStatus(dtcCtxPtr);
                        if(result != LE_OK)
                        {
                            LE_ERROR("Failed to store DTC data");
                            return result;
                        }
                    }
                }
            }
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Reset debounce counter.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::ResetDebounceCounter
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "eventCtxPtr is null");
    eventCtxPtr->debounceCounter = 0;

#ifdef LE_CONFIG_DIAG_FEATURE_A
    le_result_t result;
    result = taf_DataAccess_SetEventFailedCounter(eventCtxPtr->eventId, 0);
    if(result != LE_OK)
    {
        LE_CRIT("Store fault detection counter error");
    }
#endif
    LE_DEBUG("Reset debounce counter for event id:%d", eventCtxPtr->eventId);
    if(eventCtxPtr->debounceType == TAF_DIAGEVENT_DEBOUNCE_TIME_BASED)
    {
        //If timebased debounce timer is running, stop it
        if(le_timer_IsRunning(eventCtxPtr->timerRef))
        {
            le_timer_Stop(eventCtxPtr->timerRef);
            eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_UNKNOWN;
        }

        //If FDC threshold timer is running, stop it.
        if(le_timer_IsRunning(eventCtxPtr->fdcTimerRef))
        {
            le_timer_Stop(eventCtxPtr->fdcTimerRef);
        }
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Reset debounce status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::ResetDebounceStatus
(
    taf_diagEvent_ServiceRef_t svcRef,
    taf_diagEvent_DebounceResetStatus_t status
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    taf_diagEvent_EventCtx_t* eventCtxPtr = GetEventCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    eventCtxPtr->debounceBehavior_= status;

    //Enable condition is not fullfilled, clear the counter
    if((status == TAF_DIAGEVENT_DEBOUNCE_RESET) &&
            (eventCtxPtr->eventEnableStatus == false))
    {
        ResetDebounceCounter(eventCtxPtr);
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Debounce event based counter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::DebounceCounterBased
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    taf_diagEvent_StatusType_t eventStatus
)
{
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is NULL");
    LE_DEBUG("Before:debounce counter=%d", eventCtxPtr->debounceCounter);

    switch (eventStatus) {
    case TAF_DIAGEVENT_PASSED:
        eventCtxPtr->debounceCounter = eventCtxPtr->debounceCounterBasedConfig.passedThreshold;
        break;
    case TAF_DIAGEVENT_FAILED:
        eventCtxPtr->debounceCounter = eventCtxPtr->debounceCounterBasedConfig.failedThreshold;
        break;
    case TAF_DIAGEVENT_PREPASSED:
        LE_DEBUG("Counterbased:prepassed");
        if (eventCtxPtr->debounceCounter > eventCtxPtr->debounceCounterBasedConfig.passedThreshold)
        {
            if ((eventCtxPtr->debounceCounterBasedConfig.jumpDown == true) &&
                    (eventCtxPtr->debounceCounter >
                    eventCtxPtr->debounceCounterBasedConfig.jumpDownValue))
            {
                eventCtxPtr->debounceCounter =
                        eventCtxPtr->debounceCounterBasedConfig.jumpDownValue;

                LE_DEBUG("Set debounce counter to jumpdown value %d",eventCtxPtr->debounceCounter);
            }
            if (eventCtxPtr->debounceCounter >=
                    (eventCtxPtr->debounceCounterBasedConfig.passedThreshold +
                    eventCtxPtr->debounceCounterBasedConfig.decrementStepSize))
            {
                LE_DEBUG("debounce counter--  =%d",
                        eventCtxPtr->debounceCounterBasedConfig.decrementStepSize);
                eventCtxPtr->debounceCounter -=
                        eventCtxPtr->debounceCounterBasedConfig.decrementStepSize;

            }
            else
            {
                eventCtxPtr->debounceCounter =
                        eventCtxPtr->debounceCounterBasedConfig.passedThreshold;

            }
        }
        break;
    case TAF_DIAGEVENT_PREFAILED:
        LE_DEBUG("Counterbased:Prefailed");
        if (eventCtxPtr->debounceCounter < eventCtxPtr->debounceCounterBasedConfig.failedThreshold)
        {
            if ((eventCtxPtr->debounceCounterBasedConfig.jumpUp == true) &&
                    (eventCtxPtr->debounceCounter <
                    eventCtxPtr->debounceCounterBasedConfig.jumpUpValue))
            {
                eventCtxPtr->debounceCounter = eventCtxPtr->debounceCounterBasedConfig.jumpUpValue;

            }
            if (eventCtxPtr->debounceCounter <=
                    (eventCtxPtr->debounceCounterBasedConfig.failedThreshold -
                    eventCtxPtr->debounceCounterBasedConfig.incrementStepSize))
            {
                LE_DEBUG("debounce counter++  =%d",
                        eventCtxPtr->debounceCounterBasedConfig.incrementStepSize);
                eventCtxPtr->debounceCounter +=
                        eventCtxPtr->debounceCounterBasedConfig.incrementStepSize;
            }
            else
            {
                eventCtxPtr->debounceCounter =
                        eventCtxPtr->debounceCounterBasedConfig.failedThreshold;
            }

            //Check the counter and trigger the snapshot data
            if(!eventCtxPtr->fdcTriggerFlag && eventCtxPtr->debounceCounter >=
                    eventCtxPtr->debounceCounterBasedConfig.fdcThreshold)
            {
                eventCtxPtr->fdcTriggerFlag = true;// Only trigger once
                taf_SnapshotSvr& ss = taf_SnapshotSvr::GetInstance();
                LE_INFO("Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
                        cfg::DEM_TRIGGER_ON_FDC_THRESHOLD);
                ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_FDC_THRESHOLD,
                        eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);
            }

        }
        break;
    default:
        LE_ERROR("Wrong debounce type");
        return LE_FAULT;
    }

    if (eventCtxPtr->debounceCounter <= eventCtxPtr->debounceCounterBasedConfig.passedThreshold)
    {
        //Return passed after debouncing
        LE_DEBUG("Counterbased:reached passed");
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_PASSED;
    }
    else if (eventCtxPtr->debounceCounter >=
            eventCtxPtr->debounceCounterBasedConfig.failedThreshold)
    {
        //Return failed after debouncing
        LE_DEBUG("Counterbased:reached failed");
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_FAILED;
    }
    else
    {
        //Return pre_failed or pre_passed
        eventCtxPtr->eventFaultStatus = eventStatus;
    }

    LE_DEBUG("After:EventId=%d, debounce counter=%d", eventCtxPtr->eventId,
            eventCtxPtr->debounceCounter);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Debounce event based time.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::DebounceTimeBased
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    taf_diagEvent_StatusType_t eventStatus
)
{
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is NULL");

    LE_DEBUG("DebounceTimeBased:EventId:%d, Before:debounce counter=%d", eventCtxPtr->eventId,
            eventCtxPtr->debounceCounter);

    switch (eventStatus) {
    case TAF_DIAGEVENT_PASSED:
        eventCtxPtr->debounceCounter = MIN_FAULT_DETECTION_COUNTER;

        if(le_timer_IsRunning(eventCtxPtr->timerRef))
        {
            le_timer_Stop(eventCtxPtr->timerRef);
        }

        //If FDC threshold timer is running, stop it.
        if(le_timer_IsRunning(eventCtxPtr->fdcTimerRef))
        {
            le_timer_Stop(eventCtxPtr->fdcTimerRef);
        }

        break;
    case TAF_DIAGEVENT_FAILED:
        eventCtxPtr->debounceCounter = MAX_FAULT_DETECTION_COUNTER;

        if(le_timer_IsRunning(eventCtxPtr->timerRef))
        {
            le_timer_Stop(eventCtxPtr->timerRef);
        }

        //If FDC threshold timer is running, stop it.
        if(le_timer_IsRunning(eventCtxPtr->fdcTimerRef))
        {
            le_timer_Stop(eventCtxPtr->fdcTimerRef);
        }

        break;
    case TAF_DIAGEVENT_PREPASSED:
        LE_DEBUG("EventId:%d, Timebased:prepassed", eventCtxPtr->eventId);

        if(le_timer_IsRunning(eventCtxPtr->timerRef))
        {

            if(eventCtxPtr->timerType == TAF_DIAGEVENT_TIMER_PREPASSED)
            {
                LE_DEBUG("EventId:%d, Timer is running with prepassed type", eventCtxPtr->eventId);
            }
            else
            {
                LE_DEBUG("EventId:%d, Timer is running with prefailed type", eventCtxPtr->eventId);
                // Set passed threshold
                le_timer_SetMsInterval(eventCtxPtr->timerRef,
                        eventCtxPtr->debounceTimeBasedConfig.passedThreshold);
                // Set running timer type
                eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_PREPASSED;
                // Set counter to 0
                eventCtxPtr->debounceCounter = 0;

                //Restart the timer
                le_timer_Restart(eventCtxPtr->timerRef);
                LE_DEBUG("EventId:%d, restart the timer", eventCtxPtr->eventId);
                eventCtxPtr->startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
            }
        }
        else
        {
            if(eventCtxPtr->debounceCounter != MIN_FAULT_DETECTION_COUNTER)
            {
                //Timer is not started, Set passed threshold
                le_timer_SetMsInterval(eventCtxPtr->timerRef,
                        eventCtxPtr->debounceTimeBasedConfig.passedThreshold);
                // Set running timer type
                eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_PREPASSED;
                // Set counter to 0
                eventCtxPtr->debounceCounter = 0;

                //Start the timer for prefailed/prepassed
                LE_DEBUG("EventId:%d,---Start the timer", eventCtxPtr->eventId);
                le_timer_Start(eventCtxPtr->timerRef);
                eventCtxPtr->startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
            }
        }

        //FDC is only used in prefailed status. if FDC threshold timer is running, stop it.
        if(le_timer_IsRunning(eventCtxPtr->fdcTimerRef))
        {
            le_timer_Stop(eventCtxPtr->fdcTimerRef);
        }

        break;
    case TAF_DIAGEVENT_PREFAILED:
        LE_DEBUG("EventId:%d,Timebased:Prefailed", eventCtxPtr->eventId);

        if(le_timer_IsRunning(eventCtxPtr->timerRef))
        {
            if(eventCtxPtr->timerType == TAF_DIAGEVENT_TIMER_PREFAILED)
            {
                LE_DEBUG("EventId:%d,Timer is running with prefailed type", eventCtxPtr->eventId);
            }
            else
            {
                LE_DEBUG("EventId:%d,Timer is running with prepassed type", eventCtxPtr->eventId);
                // Set failed threshold
                le_timer_SetMsInterval(eventCtxPtr->timerRef,
                        eventCtxPtr->debounceTimeBasedConfig.failedThreshold);
                // Set running timer type
                eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_PREFAILED;
                // Set counter to 0
                eventCtxPtr->debounceCounter = 0;

                //Restart the timer
                LE_DEBUG("EventId:%d, restart the timer", eventCtxPtr->eventId);
                le_timer_Restart(eventCtxPtr->timerRef);
                eventCtxPtr->startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

            }
        }
        else
        {
            if(eventCtxPtr->debounceCounter != MAX_FAULT_DETECTION_COUNTER)
            {
                //Timer is not started
                // Set failed threshold
                le_timer_SetMsInterval(eventCtxPtr->timerRef,
                        eventCtxPtr->debounceTimeBasedConfig.failedThreshold);
                // Set running timer type
                eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_PREFAILED;
                // Set counter to 0
                eventCtxPtr->debounceCounter = 0;

                //Start the timer
                LE_DEBUG("EventId:%d,---Start the timer", eventCtxPtr->eventId);
                le_timer_Start(eventCtxPtr->timerRef);
                eventCtxPtr->startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

            }
        }

        //If FDC timer is not running, start it.
        if(!le_timer_IsRunning(eventCtxPtr->fdcTimerRef))
        {
            le_timer_Start(eventCtxPtr->fdcTimerRef);
        }

        break;
    default:
        LE_ERROR("EventId:%d,Wrong debounce type", eventCtxPtr->eventId);
        return LE_FAULT;
    }

    if (eventCtxPtr->debounceCounter == MIN_FAULT_DETECTION_COUNTER)
    {
        //Return passed after debouncing
        LE_DEBUG("EventId:%d, Timebased:reached passed", eventCtxPtr->eventId);
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_PASSED;
    }
    else if (eventCtxPtr->debounceCounter == MAX_FAULT_DETECTION_COUNTER)
    {
        //Return failed after debouncing
        LE_DEBUG("EventId:%d, Timebased:reached failed", eventCtxPtr->eventId);
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_FAILED;
    }
    else
    {
        //Return pre_failed or pre_passed
        eventCtxPtr->eventFaultStatus = eventStatus;
    }

    LE_DEBUG("EventId:%d, After:debounce timer=%d", eventCtxPtr->eventId,
            eventCtxPtr->debounceCounter);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Debounce event according to the debounce type.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::DebounceEvent
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    taf_diagEvent_StatusType_t eventStatus
)
{
    le_result_t result;

    LE_DEBUG("DebounceEvent");
    switch(eventCtxPtr->debounceType)
    {
        case TAF_DIAGEVENT_DEBOUNCE_MONITOR_INTERNAL:

            if(eventStatus != TAF_DIAGEVENT_PASSED && eventStatus != TAF_DIAGEVENT_FAILED)
            {
                LE_ERROR("Wrong event status for monitor internal debounce");
                result = LE_FAULT;
            }
            else
            {
                eventCtxPtr->eventFaultStatus = eventStatus;
                result = LE_OK;
            }

        break;
        case TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED:
            result =  DebounceCounterBased(eventCtxPtr, eventStatus);
        break;
        case TAF_DIAGEVENT_DEBOUNCE_TIME_BASED:
            result =  DebounceTimeBased(eventCtxPtr, eventStatus);
        break;
        default:
            LE_ERROR("Wrong debounce type");
            result =  LE_FAULT;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnFailed
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    LE_DEBUG("UpdateEventOnFailed");
    //Set bit 0 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TF;
    //Set bit 1 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TFTOC;
    //Set bit 2 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_PDTC;
    //Set bit 5 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TFSLC;

    //Set bit 4 to value 0
    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TNCSLC);
    //Set bit 6 to value 0
    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TNCTOC);

    LE_DEBUG("failure counter = %d",eventCtxPtr->failureCounter);
    //Counter is only incremented to 1 for one operation cycle //figure D.9
    //if bit TAF_DIAGEVENT_UDS_STATUS_TFTOC was set to 1, it means counter was already incremented
    if((oldEventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC) == 0)
    {
        if(eventCtxPtr->failureCounter < MAX_TEST_FAILURE_COUNTER)
        {
            LE_DEBUG("failureCounter++");
            eventCtxPtr->failureCounter++;
        }
    }

    //Counter reaches the threshold
    if(eventCtxPtr->failureCounter >= eventCtxPtr->confirmationThreshold)
    {
        //DTC is confirmed, set bit 3 to value 1
        LE_DEBUG("confirmed event status for event id=%d",eventCtxPtr->eventId);
        eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_CDTC;
        eventCtxPtr->failureCounter=0;
    }

    LE_DEBUG("Event Failed:eventId:%d, status:0x%x->0x%x", eventCtxPtr->eventId,
            oldEventUdsStatus, eventCtxPtr->eventUdsStatus);
    //Event uds status changed, call data handle module to store
    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }

        TriggerSnapshotData(oldEventUdsStatus, eventCtxPtr);
    }

    return UpdateDtcOnFailed(eventCtxPtr->dtcCtxPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Update DTC status when test failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateDtcOnFailed
(
    taf_diagEvent_DtcCtx_t *dtcCtxPtr
)
{
// Update dtc status
    uint8_t oldDtcStatus =0;
    uint8_t oldOccurrenceCounter;
    le_result_t result;
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr;

    LE_DEBUG("UpdateDtcOnFailed");
    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_FAULT, "dtcCtxPtr is null");

    oldOccurrenceCounter = dtcCtxPtr->occurrenceCounter;
    oldDtcStatus = dtcCtxPtr->dtcStatus;
    //Calculate DTC status by events status
    dtcCtxPtr->dtcStatus = 0;

    eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
    while (eventIdLinkPtr)
    {
        taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                taf_DiagEvent_EventIdInfo_t, link);
        eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);
        eventCtxPtr = GetEventCtxById(eventIdInfoPtr->eventId);
        LE_DEBUG("DTC failed:event id:%d, dtcCode:0x%x", eventCtxPtr->eventId,
                eventCtxPtr->dtcCode);
        //Find the event with the same dtc code
        dtcCtxPtr->dtcStatus |= eventCtxPtr->eventUdsStatus;
    }

    if (dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TFSLC)
        dtcCtxPtr->dtcStatus &= ~TAF_DIAGEVENT_UDS_STATUS_TNCSLC;

    if (dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC)
        dtcCtxPtr->dtcStatus &= ~TAF_DIAGEVENT_UDS_STATUS_TNCTOC;

    //Test failed
    LE_DEBUG("DTC Failed:status:0x%x->0x%x, Processing=%d, occurrence counter=%d",
            oldDtcStatus, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounterProcessing,
            dtcCtxPtr->occurrenceCounter);
    if(dtcCtxPtr->occurrenceCounterProcessing  == TAF_DIAGEVENT_PROCESS_OCCCTR_TF)
    {
        //Bit 0 transition from 0 to 1
        if(((oldDtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) == 0) &&
                ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) != 0) &&
                (dtcCtxPtr->occurrenceCounter < MAX_OCCURRENCE_COUNTER))
        {
            LE_DEBUG("Tested Failed:Occurrence counter ++");
            dtcCtxPtr->occurrenceCounter++;
        }
    }
    else if(dtcCtxPtr->occurrenceCounterProcessing  ==  TAF_DIAGEVENT_PROCESS_OCCCTR_CDTC)
    {
        //Bit 0 transition from 0 to 1 and bit 3 is 1
        if(((oldDtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) == 0) &&
                ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) != 0) &&
                ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) != 0) &&
                (dtcCtxPtr->occurrenceCounter < MAX_OCCURRENCE_COUNTER))
        {
            LE_DEBUG("Confirmed DTC:Occurrence counter ++");
            dtcCtxPtr->occurrenceCounter++;
        }
    }
    else
    {
        LE_ERROR("Fault occurrence type");
        return LE_FAULT;
    }

    LE_DEBUG("DTC Failed: DTC:0x%x, status:0x%x->0x%x, occurrence counter:%d->%d",
            dtcCtxPtr->dtcCode, oldDtcStatus, dtcCtxPtr->dtcStatus,
            oldOccurrenceCounter, dtcCtxPtr->occurrenceCounter);
    //Counter or dtc status changed
    if((oldOccurrenceCounter != dtcCtxPtr->occurrenceCounter) ||
            (dtcCtxPtr->dtcStatus != oldDtcStatus))
    {
        //Call data handle module to store the DTC uds status
        LE_INFO("Database:Store DTC code:0x%x, status:0x%x, occurrence counter:%d",
                dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
        result = taf_DataAccess_SetDTCStatus(dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus,
                dtcCtxPtr->occurrenceCounter);
        if(result != LE_OK)
        {
            LE_CRIT("Can't store data into database, dtcCode:0x%x, status:0x%x",
                    dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus);
            return result;
        }
    }

    if(dtcCtxPtr->dtcStatus != oldDtcStatus)
    {
        ReportDtcStatus(dtcCtxPtr);
    }

    LE_DEBUG("Latest: DTC code=0x%x, status=0x%x,occurrencecounter=%d",
            dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test passed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnPassed
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    LE_DEBUG("UpdateEventOnPassed");
    //Set bit 0 to value 0
    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TF);
    //Set bit 4 to value 0
    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TNCSLC);
    //Set bit 6 to value 0
    eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TNCTOC);

    LE_DEBUG("Event passed:eventId:%d, status=0x%x->0x%x", eventCtxPtr->eventId, oldEventUdsStatus,
            eventCtxPtr->eventUdsStatus);

    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }
    }

    return UpdateDtcOnPassed(eventCtxPtr->dtcCtxPtr);

}

//-------------------------------------------------------------------------------------------------
/**
 * Update DTC status when test passed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateDtcOnPassed
(
    taf_diagEvent_DtcCtx_t* dtcCtxPtr
)
{
    uint8_t oldDtcStatus = 0;
    le_result_t result;
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr;

    LE_DEBUG("UpdateDtcOnPassed");
    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_BAD_PARAMETER, "dtcCtxPtr is null");
    oldDtcStatus = dtcCtxPtr->dtcStatus;
    //Calculate DTC status by events status
    dtcCtxPtr->dtcStatus = 0;

    eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
    while (eventIdLinkPtr)
    {
        taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                taf_DiagEvent_EventIdInfo_t, link);
        eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);
        eventCtxPtr = GetEventCtxById(eventIdInfoPtr->eventId);
        LE_DEBUG("DTC passed: eventId:%d, dtcCode:0x%x, eventStatus:0x%x, dtcStatus:0x%x",
                eventIdInfoPtr->eventId, dtcCtxPtr->dtcCode, eventCtxPtr->eventUdsStatus,
                dtcCtxPtr->dtcStatus);
        //Find the event with the same dtc code
        dtcCtxPtr->dtcStatus |= eventCtxPtr->eventUdsStatus;
    }

    if (dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TFSLC)
        dtcCtxPtr->dtcStatus &= ~TAF_DIAGEVENT_UDS_STATUS_TNCSLC;

    if (dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC)
        dtcCtxPtr->dtcStatus &= ~TAF_DIAGEVENT_UDS_STATUS_TNCTOC;

    LE_DEBUG("DTC passed: DTC code:0x%x, status:0x%x->0x%x", dtcCtxPtr->dtcCode, oldDtcStatus,
            dtcCtxPtr->dtcStatus);
    //Counter or dtc status changed, call data handle module to store
    if(dtcCtxPtr->dtcStatus != oldDtcStatus)
    {
        //Store data and report DTC status change
        result = StoreAndReportDTCStatus(dtcCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store DTC data");
            return result;
        }
    }

    return LE_OK;
}

#ifdef LE_CONFIG_DIAG_FEATURE_A
//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test failed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnFailedCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    LE_DEBUG("UpdateEventOnFailedCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        eventCtxPtr->eventUdsStatus = 0x01;//set bit0 to 1, others to 0
    }
    else if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_APPLICATION_DTC)
    {
        //Set bit 0 to value 1, don't touch bit 3
        eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TF;
    }
    else
    {
        LE_ERROR("DTC type error");
        return LE_FAULT;
    }

    //Event uds status changed, call data handle module to store
    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        LE_INFO("Event failed:eventId:%d, status=0x%x->0x%x", eventCtxPtr->eventId,
                oldEventUdsStatus, eventCtxPtr->eventUdsStatus);
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }

        TriggerSnapshotData(oldEventUdsStatus, eventCtxPtr);

        return UpdateDtcForCustomerN(eventCtxPtr->dtcCtxPtr);
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update DTC status for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateDtcForCustomerN
(
    taf_diagEvent_DtcCtx_t *dtcCtxPtr
)
{
// Update dtc status
    uint8_t oldDtcStatus =0;
    uint8_t oldOccurrenceCounter;
    le_result_t result;
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr;

    LE_DEBUG("UpdateDtcForCustomerN");
    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_FAULT, "dtcCtxPtr is null");

    oldOccurrenceCounter = dtcCtxPtr->occurrenceCounter;
    oldDtcStatus = dtcCtxPtr->dtcStatus;
    //Calculate DTC status by events status
    dtcCtxPtr->dtcStatus = 0;

    eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
    while (eventIdLinkPtr)
    {
        taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                taf_DiagEvent_EventIdInfo_t, link);
        eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);
        eventCtxPtr = GetEventCtxById(eventIdInfoPtr->eventId);
        LE_DEBUG("DTC failed:event id:%d, dtcCode:0x%x", eventCtxPtr->eventId,
                eventCtxPtr->dtcCode);
        //Find the event with the same dtc code
        dtcCtxPtr->dtcStatus |= eventCtxPtr->eventUdsStatus;
    }

    LE_DEBUG("DTC:status:0x%x->0x%x, Processing=%d, occurrence counter=%d",
            oldDtcStatus, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounterProcessing,
            dtcCtxPtr->occurrenceCounter);
    if(dtcCtxPtr->occurrenceCounterProcessing  == TAF_DIAGEVENT_PROCESS_OCCCTR_TF)
    {
        //Bit 0 transition from 0 to 1
        if(((oldDtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) == 0) &&
                ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) != 0) &&
                (dtcCtxPtr->occurrenceCounter < MAX_OCCURRENCE_COUNTER))
        {
            LE_INFO("Tested Failed:Occurrence counter ++");
            dtcCtxPtr->occurrenceCounter++;
        }
    }
    else if(dtcCtxPtr->occurrenceCounterProcessing  ==  TAF_DIAGEVENT_PROCESS_OCCCTR_CDTC)
    {


        if(dtcCtxPtr->occurrenceCounter < MAX_OCCURRENCE_COUNTER)
        {
            //Bit 0 transition from 0 to 1 and bit 3 is 1, or bit 3 transition from 0 to 1
            if((((oldDtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) == 0) &&
                    ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_TF) != 0) &&
                    ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) != 0) ) ||
                    (((oldDtcStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) == 0) &&
                    ((dtcCtxPtr->dtcStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) != 0)))
            {
                LE_INFO("Confirmed DTC:Occurrence counter ++");
                dtcCtxPtr->occurrenceCounter++;
            }
        }
    }
    else
    {
        LE_ERROR("Fault occurrence type");
        return LE_FAULT;
    }

    LE_DEBUG("DTC Failed: DTC:0x%x, status:0x%x->0x%x, occurrence counter:%d->%d",
            dtcCtxPtr->dtcCode, oldDtcStatus, dtcCtxPtr->dtcStatus,
            oldOccurrenceCounter, dtcCtxPtr->occurrenceCounter);
    //Counter or dtc status changed
    if((oldOccurrenceCounter != dtcCtxPtr->occurrenceCounter) ||
            (dtcCtxPtr->dtcStatus != oldDtcStatus))
    {
        //Call data handle module to store the DTC uds status
        LE_INFO("Database:Store DTC code:0x%x, status:0x%x, occurrence counter:%d",
                dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
        result = taf_DataAccess_SetDTCStatus(dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus,
                dtcCtxPtr->occurrenceCounter);
        if(result != LE_OK)
        {
            LE_CRIT("Can't store data into database, dtcCode:0x%x, status:0x%x",
                    dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus);
            return result;
        }
    }

    if(dtcCtxPtr->dtcStatus != oldDtcStatus)
    {
        ReportDtcStatus(dtcCtxPtr);
    }

    LE_INFO("Latest: DTC code=0x%x, status=0x%x,occurrencecounter=%d",
            dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test passed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnPassedCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    LE_DEBUG("UpdateEventOnPassedCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        eventCtxPtr->eventUdsStatus = 0x00;//set all bits to 0
    }
    else if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_APPLICATION_DTC)
    {
        //Set bit 0 to value 0, don't touch bit 3
        eventCtxPtr->eventUdsStatus &= ~(TAF_DIAGEVENT_UDS_STATUS_TF);
    }
    else
    {
        LE_ERROR("DTC type error");
        return LE_FAULT;
    }

    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        LE_INFO("Event passed:eventId:%d, status=0x%x->0x%x", eventCtxPtr->eventId,
                oldEventUdsStatus, eventCtxPtr->eventUdsStatus);
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }

        return UpdateDtcForCustomerN(eventCtxPtr->dtcCtxPtr);
    }

    return LE_OK;

}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test pre passed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnPrePassedCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    LE_DEBUG("UpdateEventOnPrePassedCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_APPLICATION_DTC)
    {
        LE_INFO("PRE_FAILED is not used for application DTC");
        return LE_OK;
    }

    if(eventCtxPtr->dtcCtxPtr->dtcType != TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        LE_ERROR("PRE_PASSED is not supported for this DTC type");
        return LE_FAULT;
    }

    if(eventCtxPtr->debounceType != TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED)
    {
        LE_ERROR("PRE_PASSED is not supported for this debounce type");
        return LE_FAULT;
    }

    //Debounce
    if (eventCtxPtr->debounceCounter > eventCtxPtr->debounceCounterBasedConfig.passedThreshold)
    {
        if ((eventCtxPtr->debounceCounterBasedConfig.jumpDown == true) &&
                (eventCtxPtr->debounceCounter >
                eventCtxPtr->debounceCounterBasedConfig.jumpDownValue))
        {
            eventCtxPtr->debounceCounter =
                    eventCtxPtr->debounceCounterBasedConfig.jumpDownValue;

            LE_DEBUG("Set debounce counter to jumpdown value %d",eventCtxPtr->debounceCounter);
        }
        if (eventCtxPtr->debounceCounter >=
                (eventCtxPtr->debounceCounterBasedConfig.passedThreshold +
                eventCtxPtr->debounceCounterBasedConfig.decrementStepSize))
        {
            LE_DEBUG("debounce counter--  =%d",
                    eventCtxPtr->debounceCounterBasedConfig.decrementStepSize);
            eventCtxPtr->debounceCounter -=
                    eventCtxPtr->debounceCounterBasedConfig.decrementStepSize;

        }
        else
        {
            eventCtxPtr->debounceCounter =
                    eventCtxPtr->debounceCounterBasedConfig.passedThreshold;

        }

        //For Nissan, store fault detection counter with 0 in DB
        result = taf_DataAccess_SetEventFailedCounter(eventCtxPtr->eventId, 0);
        if(result != LE_OK)
        {
            LE_CRIT("Store fault detection counter error");
        }

        //After debounce, if reached the threshold, it is passed
        if (eventCtxPtr->debounceCounter <= eventCtxPtr->debounceCounterBasedConfig.passedThreshold)
        {
            return UpdateEventOnPassedCustomerN(eventCtxPtr);
        }
    }
    else
    {
        LE_INFO("Already passed, do nothing");
    }

    return LE_OK;

}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test pre failed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnPreFailedCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is null");

    LE_DEBUG("UpdateEventOnPreFailedCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_APPLICATION_DTC)
    {
        LE_INFO("PRE_FAILED is not used for application DTC");
        return LE_OK;
    }

    if(eventCtxPtr->dtcCtxPtr->dtcType != TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        LE_ERROR("PRE_FAILED is not supported for this DTC type");
        return LE_FAULT;
    }

    if(eventCtxPtr->debounceType != TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED)
    {
        LE_ERROR("PRE_FAILED is not supported for this debounce type");
        return LE_FAULT;
    }

    //Debounce
    if (eventCtxPtr->debounceCounter < eventCtxPtr->debounceCounterBasedConfig.failedThreshold)
    {
        if ((eventCtxPtr->debounceCounterBasedConfig.jumpUp == true) &&
                (eventCtxPtr->debounceCounter <
                eventCtxPtr->debounceCounterBasedConfig.jumpUpValue))
        {
            eventCtxPtr->debounceCounter = eventCtxPtr->debounceCounterBasedConfig.jumpUpValue;

        }
        if (eventCtxPtr->debounceCounter <=
                (eventCtxPtr->debounceCounterBasedConfig.failedThreshold -
                eventCtxPtr->debounceCounterBasedConfig.incrementStepSize))
        {
            LE_DEBUG("debounce counter++  =%d",
                    eventCtxPtr->debounceCounterBasedConfig.incrementStepSize);
            eventCtxPtr->debounceCounter +=
                    eventCtxPtr->debounceCounterBasedConfig.incrementStepSize;
        }
        else
        {
            eventCtxPtr->debounceCounter =
                    eventCtxPtr->debounceCounterBasedConfig.failedThreshold;
        }

        //For Nissan, store fault detection counter in DB
        float ratio = (float)MAX_FAULT_DETECTION_COUNTER/eventCtxPtr->debounceCounterBasedConfig.
                failedThreshold;
        uint8_t faultDetectionCounter = eventCtxPtr->debounceCounter * ratio;
        LE_DEBUG("eventId:%d,debounceCounter=%d, ratio=%f", eventCtxPtr->eventId,
                eventCtxPtr->debounceCounter, ratio);

        result = taf_DataAccess_SetEventFailedCounter(eventCtxPtr->eventId, faultDetectionCounter);
        if(result != LE_OK)
        {
            LE_CRIT("Store fault detection counter error");
        }

        //After debounce, if reached the threshold, it is failed
        if (eventCtxPtr->debounceCounter >= eventCtxPtr->debounceCounterBasedConfig.failedThreshold)
        {
            return UpdateEventOnFailedCustomerN(eventCtxPtr);
        }
    }
    else
    {
        LE_DEBUG("Already falied, do nothing");
    }

    return LE_OK;

}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test confirmed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnConfirmedCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    LE_DEBUG("UpdateEventOnConfirmedCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType != TAF_DIAGEVENT_APPLICATION_DTC)
    {
        LE_ERROR("CONFIRMED is not supported for this DTC type");
        return LE_FAULT;
    }

    //Set bit 0 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TF;
    //Set bit 3 to value 1
    eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_CDTC;

    //Event uds status changed, call data handle module to store
    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        LE_INFO("Event changed:eventId:%d, status=0x%x->0x%x", eventCtxPtr->eventId,
                oldEventUdsStatus, eventCtxPtr->eventUdsStatus);
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }

        TriggerSnapshotData(oldEventUdsStatus, eventCtxPtr);

        return UpdateDtcForCustomerN(eventCtxPtr->dtcCtxPtr);
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update event status when test confirmed for Nissan.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::UpdateEventOnTestNotCmpltCustomerN
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;
    uint8_t oldEventUdsStatus = eventCtxPtr->eventUdsStatus;

    LE_DEBUG("UpdateEventOnTestNotCmpltCustomerN");

    if(eventCtxPtr->dtcCtxPtr->dtcType != TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        LE_ERROR("TEST_NOT_COMPLETED is not supported for this DTC type");
        return LE_FAULT;
    }

    //Set bit 0 to value 0, Set bit 4 to value 1
    eventCtxPtr->eventUdsStatus = 0x10;

    //Event uds status changed, call data handle module to store
    if(oldEventUdsStatus != eventCtxPtr->eventUdsStatus)
    {
        LE_INFO("Event changed:eventId:%d, status=0x%x->0x%x", eventCtxPtr->eventId,
                oldEventUdsStatus, eventCtxPtr->eventUdsStatus);
        //Store data and report event UDS status change
        result = StoreAndReportEventUdsStatus(eventCtxPtr);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to store event data");
            return result;
        }

        TriggerSnapshotData(oldEventUdsStatus, eventCtxPtr);

        return UpdateDtcForCustomerN(eventCtxPtr->dtcCtxPtr);
    }

    return LE_OK;
}
#endif

void taf_EventSvr::TriggerSnapshotData
(
    uint8_t oldEventUdsStatus,
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    taf_SnapshotSvr& ss = taf_SnapshotSvr::GetInstance();
    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "this event context is NULL");

#ifdef LE_CONFIG_DIAG_FEATURE_A
    if(eventCtxPtr->dtcCtxPtr->dtcType == TAF_DIAGEVENT_REPROGRAMMING_DTC)
    {
        LE_INFO("Don't trigger snapshot for reprogramming DTC");
        return;
    }
#endif

    //Bit 3(confirmedDTC) changes from 0 to 1, trigger snapshot data
    if(((oldEventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) == 0) &&
            ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_CDTC) != 0))
    {
        LE_INFO("Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
                cfg::DEM_TRIGGER_ON_CONFIRMED);
        ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_CONFIRMED,
                eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);
    }

    //Bit 2(pendingDTC) changes from 0 to 1, trigger snapshot data
    if(((oldEventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_PDTC) == 0) &&
            ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_PDTC) != 0))
    {
        LE_INFO("Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
                cfg::DEM_TRIGGER_ON_PENDING);
        ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_PENDING,
                eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);
    }

    //Bit 0(testFailed) changes from 0 to 1, trigger snapshot data
    if(((oldEventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TF) == 0) &&
            ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TF) != 0))
    {
        LE_INFO("Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
                cfg::DEM_TRIGGER_ON_TEST_FAILED);
        ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_TEST_FAILED,
                eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);
    }

    //Bit 1(testFailedThisOperationCycle) changes from 0 to 1, trigger snapshot data
    if(((oldEventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC) == 0) &&
            ((eventCtxPtr->eventUdsStatus & TAF_DIAGEVENT_UDS_STATUS_TFTOC) != 0))
    {
        LE_INFO("----Trigger snapshot: DTC = 0x%x, type = %d", eventCtxPtr->dtcCode,
                cfg::DEM_TRIGGER_ON_TEST_FAILED_THIS_OPERATION_CYCLE);
        ss.triggerSnapshot(eventCtxPtr->dtcCode,
                cfg::DEM_TRIGGER_ON_TEST_FAILED_THIS_OPERATION_CYCLE,
                eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Add client session into event context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::AddSessionToEventCtx
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_BAD_PARAMETER, "this event context is NULL");

    linkPtr = le_dls_Peek(&(eventCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_diagEvent_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr,
                taf_diagEvent_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(eventCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Session(%p) has been added to eventctx eventId %d", sessionRef,
                    eventCtxPtr->eventId);
            return LE_DUPLICATE;
        }
    }

    LE_DEBUG("add session %p for event id %d", sessionRef, eventCtxPtr->eventId );
    taf_diagEvent_SessionRef_t* newSessionRefPtr =
            (taf_diagEvent_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);

    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&eventCtxPtr->sessionRefList, &(newSessionRefPtr->link));

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove client session from event context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::RemoveSessionFromEventCtx
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_BAD_PARAMETER, "this event context is NULL");

    linkPtr = le_dls_Peek(&(eventCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_diagEvent_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr,
                taf_diagEvent_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(eventCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("remove ref(%p) from eventId(%d)", sessionRef, eventCtxPtr->eventId);
            le_dls_Remove(&(eventCtxPtr->sessionRefList), &(sessionRefPtr->link));
            le_mem_Release(sessionRefPtr);
            return LE_OK;
        }
    }

    LE_DEBUG("Cannot found session context with ref(%p) from eventId(%d)", sessionRef,
            eventCtxPtr->eventId);

    return LE_NOT_FOUND;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event context by service reference.
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_EventCtx_t* taf_EventSvr::GetEventCtx
(
    taf_diagEvent_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is NULL");

    taf_diagEvent_EventCtx_t* eventCtxPtr = (taf_diagEvent_EventCtx_t*)le_ref_Lookup(SvcRefMap,
            svcRef);
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, NULL, "Invalid eventCtxPtr");

    return eventCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerUdsStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::FirstLayerUdsStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagEvent_UdsStatus_t* udsStatusEvent = (taf_diagEvent_UdsStatus_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagEvent_UdsStatusHandlerFunc_t handlerFunc =
                                    (taf_diagEvent_UdsStatusHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(udsStatusEvent->svcRef, udsStatusEvent->eventUdsStatus, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerEnableCondStateHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::FirstLayerEnableCondStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagEvent_EnableCondState_t* enableCondStateEvent =
            (taf_diagEvent_EnableCondState_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagEvent_EnableCondStateHandlerFunc_t handlerFunc =
            (taf_diagEvent_EnableCondStateHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(enableCondStateEvent->svcRef, enableCondStateEvent->state,
            le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call data handle module to store data into DB and report Event UDS status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::StoreAndReportEventUdsStatus
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, LE_FAULT, "eventCtxPtr is NULL");

    LE_INFO("Database:Store and report eventId:%d, status:0x%x", eventCtxPtr->eventId,
            eventCtxPtr->eventUdsStatus);
    result = taf_DataAccess_SetEventStatus(eventCtxPtr->eventId, eventCtxPtr->eventUdsStatus);
    if(result != LE_OK)
    {
        LE_CRIT("Can't store data into database, EventId:%d, status:0x%x",
                eventCtxPtr->eventId, eventCtxPtr->eventUdsStatus);
        return result;
    }

    //Report Event UDS status notification
    ReportEventUdsStatus(eventCtxPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report event UDS status.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::ReportEventUdsStatus
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "Null pointer");

    if(eventCtxPtr->svcRef == NULL)
    {
        LE_DEBUG("No handler registered for eventId:%d", eventCtxPtr->eventId);
        return;
    }

    taf_diagEvent_UdsStatus_t* udsStatusIndPtr =
                (taf_diagEvent_UdsStatus_t*)le_mem_ForceAlloc(EventUdsStatusPool);

    udsStatusIndPtr->svcRef = eventCtxPtr->svcRef;
    udsStatusIndPtr->eventUdsStatus = eventCtxPtr->eventUdsStatus;

    le_event_ReportWithRefCounting(eventCtxPtr->udsStatusEventId, (void*)udsStatusIndPtr);

}

//--------------------------------------------------------------------------------------------------
/**
 * Report Enable Condition state.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::ReportEnableCondState
(
    taf_diagEvent_EventCtx_t* eventCtxPtr,
    bool state
)
{
    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "Null pointer");

    if(eventCtxPtr->svcRef == NULL)
    {
        LE_DEBUG("No handler registered for eventId:%d", eventCtxPtr->eventId);
        return;
    }

    taf_diagEvent_EnableCondState_t* enableCondStateIndPtr =
                (taf_diagEvent_EnableCondState_t*)le_mem_ForceAlloc(EnableCondStatePool);

    enableCondStateIndPtr->svcRef = eventCtxPtr->svcRef;
    enableCondStateIndPtr->state = state;

    le_event_ReportWithRefCounting(eventCtxPtr->enableCondStateEventId,
            (void*)enableCondStateIndPtr);

}

//--------------------------------------------------------------------------------------------------
/**
 * Call data handle module to store data into DB and report DTC  status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::StoreAndReportDTCStatus
(
    taf_diagEvent_DtcCtx_t* dtcCtxPtr
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_FAULT, "dtcCtxPtr is NULL");
    //Store data

    LE_INFO("Database:Store DTC code:0x%x, status:0x%x, occurrence counter:%d",
            dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
    result = taf_DataAccess_SetDTCStatus(dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus,
            dtcCtxPtr->occurrenceCounter);
    if(result != LE_OK)
    {
        LE_CRIT("Can't store data into database, dtcCode:0x%x, status:0x%x",
                dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus);
        return result;
    }
    //Report DTC status notification
    ReportDtcStatus(dtcCtxPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report DTC status.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::ReportDtcStatus
(
    taf_diagEvent_DtcCtx_t* dtcCtxPtr
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "Null pointer");

    diagDTC.ReportDTCStatus(dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus);

}

//--------------------------------------------------------------------------------------------------
/**
 * Get uds status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_EventSvr::GetUdsStatusEvent
(
    taf_diagEvent_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is NULL");

    taf_diagEvent_EventCtx_t* eventCtxPtr = (taf_diagEvent_EventCtx_t*)le_ref_Lookup(SvcRefMap,
            svcRef);
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, NULL, "Invalid eventCtxPtr");

    return eventCtxPtr->udsStatusEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get enable condition state event ID, used to notify the Enable Condition state change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_EventSvr::GetEnableCondStateEvent
(
    taf_diagEvent_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is NULL");

    taf_diagEvent_EventCtx_t* eventCtxPtr = (taf_diagEvent_EventCtx_t*)le_ref_Lookup(SvcRefMap,
            svcRef);
    TAF_ERROR_IF_RET_VAL(eventCtxPtr == NULL, NULL, "Invalid eventCtxPtr");

    return eventCtxPtr->enableCondStateEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for time based debounce failedThreshold/passedThreshold reached.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::TimeBasedDebounceTimerHandler
(
    le_timer_Ref_t  timerRef
)
{
    auto& diagEvent = taf_EventSvr::GetInstance();
    uint8_t oldEventFaultStatus;

    taf_diagEvent_EventCtx_t* eventCtxPtr =
            (taf_diagEvent_EventCtx_t*)le_timer_GetContextPtr(timerRef);

    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "Failed to get timer context");

    oldEventFaultStatus = eventCtxPtr->eventFaultStatus;

    if(eventCtxPtr->timerType == TAF_DIAGEVENT_TIMER_PREFAILED)
    {
        LE_DEBUG("EventId:%d,Timeout with failed type", eventCtxPtr->eventId);
        eventCtxPtr->debounceCounter = MAX_FAULT_DETECTION_COUNTER;
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_FAILED;
    }
    else if(eventCtxPtr->timerType == TAF_DIAGEVENT_TIMER_PREPASSED)
    {
        LE_DEBUG("EventId:%d,Timeout with failed type", eventCtxPtr->eventId);
        eventCtxPtr->debounceCounter = MIN_FAULT_DETECTION_COUNTER;
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_PASSED;
    }
    else
    {
        LE_DEBUG("EventId:%d, incorrect timer type", eventCtxPtr->eventId);
    }

    if(oldEventFaultStatus != eventCtxPtr->eventFaultStatus)
    {
        if(eventCtxPtr->eventFaultStatus == TAF_DIAGEVENT_FAILED)
        {
            diagEvent.UpdateEventOnFailed(eventCtxPtr);
        }
        else if(eventCtxPtr->eventFaultStatus == TAF_DIAGEVENT_PASSED)
        {
            diagEvent.UpdateEventOnPassed(eventCtxPtr);
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for fault detection counter threshold reached.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::FdcTimerHandler
(
    le_timer_Ref_t  timerRef
)
{
    taf_SnapshotSvr& ss = taf_SnapshotSvr::GetInstance();
    taf_diagEvent_EventCtx_t* eventCtxPtr =
            (taf_diagEvent_EventCtx_t*)le_timer_GetContextPtr(timerRef);

    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "Failed to get timer context");

    LE_INFO("Trigger snapshot: DTC code=0x%x, threshold=%d", eventCtxPtr->dtcCode,
            cfg::DEM_TRIGGER_ON_FDC_THRESHOLD);
    ss.triggerSnapshot(eventCtxPtr->dtcCode, cfg::DEM_TRIGGER_ON_FDC_THRESHOLD,
            eventCtxPtr->supplierFaultCode, eventCtxPtr->supplierFaultCodeSize);

}

//-------------------------------------------------------------------------------------------------
/**
 * Intialize event context.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::InitEventContext
(
    const std::pair<uint32_t, std::shared_ptr<cfg::Node>> &dtc,
    const std::pair<uint16_t, std::shared_ptr<cfg::Node>> &event
)
{
    taf_diagEvent_EventCtx_t* eventCtxPtr = NULL;
    auto& diagEvent = taf_EventSvr::GetInstance();
    char eventStatusName[32] = {0};
    uint32_t dtcCode = dtc.first;
    uint16_t eventId = event.first;

    eventCtxPtr = (taf_diagEvent_EventCtx_t *)le_mem_ForceAlloc(diagEvent.EventPool);
    TAF_ERROR_IF_RET_NIL(eventCtxPtr == NULL, "cannot alloc eventCtxPtr");

    memset(eventCtxPtr, 0, sizeof(taf_diagEvent_EventCtx_t));
    LE_INFO("create context for event id %d", eventId);

    eventCtxPtr->sessionRefList = LE_DLS_LIST_INIT;

    //Create event id for diag event
    snprintf(eventStatusName, sizeof(eventStatusName)-1, "eventCtx-%d", eventId);
    eventCtxPtr->udsStatusEventId = le_event_CreateIdWithRefCounting(eventStatusName);
    //Create event id for enable condition
    snprintf(eventStatusName, sizeof(eventStatusName)-1, "enableCond-%d", eventId);
    eventCtxPtr->enableCondStateEventId = le_event_CreateIdWithRefCounting(eventStatusName);
    eventCtxPtr->eventId = eventId;
    eventCtxPtr->dtcCode = dtcCode;

    std::string operationCycle = event.second->get<string>("operation_cycle");
    eventCtxPtr->operationCycleId = uint8_t(cfg::s_to_operation_cycle_type(operationCycle));
    LE_INFO("operation cycle id : %d", eventCtxPtr->operationCycleId);

#ifdef LE_CONFIG_DIAG_FEATURE_A
    eventCtxPtr->eventEnableStatus = true;
#else
    eventCtxPtr->eventEnableStatus = false;
#endif

    eventCtxPtr->confirmationThreshold = event.second->get<int>("confirmation_threshold");

    //get debounce config
    std::string debounceAlgorism = event.second->get<string>("debounce_algorithm");
    LE_INFO(" debounce algorism : %s",debounceAlgorism.c_str());

    cfg::Node & node = cfg::top_debounce_algorithm<string>("short_name", debounceAlgorism.c_str());
    LE_INFO("debounce_type : %s", node.get<string>("base").c_str());

    if(node.get<string>("base") == "Counter")
    {
        cfg::Counter_t counterDebounce;
        cfg::top_debounce_algorithm<string>("short_name", debounceAlgorism.c_str(),
                &counterDebounce);

        eventCtxPtr->debounceType = TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED;

        if(counterDebounce.debounce_behavior == "reset")
        {
            LE_INFO("debounce_behavior : RESET");
            eventCtxPtr->debounceBehavior_ = TAF_DIAGEVENT_DEBOUNCE_RESET;
        }
        else
        {
            LE_INFO("debounce_behavior : FREEZE");
            eventCtxPtr->debounceBehavior_ = TAF_DIAGEVENT_DEBOUNCE_FREEZE;
        }

        eventCtxPtr->debounceCounterBasedConfig.decrementStepSize =
                counterDebounce.counter_decrement_step_size;

        if((eventCtxPtr->debounceCounterBasedConfig.decrementStepSize <
                MIDDLE_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.decrementStepSize >
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("decrementStepSize is incorrect for EventId:%d",eventId);
        }

        eventCtxPtr->debounceCounterBasedConfig.incrementStepSize =
                counterDebounce.counter_increment_step_size;

        if((eventCtxPtr->debounceCounterBasedConfig.incrementStepSize <
                MIDDLE_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.incrementStepSize >=
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("incrementStepSize is incorrect for EventId:%d",eventId);
        }

        eventCtxPtr->debounceCounterBasedConfig.failedThreshold =
                counterDebounce.counter_failed_threshold;

        if((eventCtxPtr->debounceCounterBasedConfig.failedThreshold <
                MIDDLE_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.failedThreshold >=
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("failedThreshold is incorrect for EventId:%d",eventId);
        }

        eventCtxPtr->debounceCounterBasedConfig.passedThreshold =
                counterDebounce.counter_passed_threshold;

        if((eventCtxPtr->debounceCounterBasedConfig.passedThreshold <
                MIN_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.passedThreshold >= 0))
        {
            LE_FATAL("passedThreshold is incorrect for EventId:%d",eventId);
        }

        eventCtxPtr->debounceCounterBasedConfig.jumpDown = counterDebounce.counter_jump_down;
        eventCtxPtr->debounceCounterBasedConfig.jumpUp = counterDebounce.counter_jump_up;

        eventCtxPtr->debounceCounterBasedConfig.jumpDownValue =
                counterDebounce.counter_jump_down_value;

        if((eventCtxPtr->debounceCounterBasedConfig.jumpDownValue <
                MIN_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.jumpDownValue >=
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("jumpDownValue is incorrect");
        }

        eventCtxPtr->debounceCounterBasedConfig.jumpUpValue =
                counterDebounce.counter_jump_up_value;

        if((eventCtxPtr->debounceCounterBasedConfig.jumpUpValue <
                MIN_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.jumpUpValue >=
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("jumpUpValue is incorrect");
        }

        eventCtxPtr->debounceCounterBasedConfig.fdcThreshold =
                counterDebounce.counter_fdc_threshold;

        if((eventCtxPtr->debounceCounterBasedConfig.fdcThreshold <
                MIDDLE_COUNTER_BASED_PARAM_VALUE) ||
                (eventCtxPtr->debounceCounterBasedConfig.fdcThreshold >=
                MAX_COUNTER_BASED_PARAM_VALUE))
        {
            LE_FATAL("fdcThreshold is incorrect for EventId:%d",eventId);
        }

 #ifdef LE_CONFIG_DIAG_FEATURE_A
        uint8_t faultDetectionCounter = taf_DataAccess_GetEventFailedCounter(eventId);
        float ratio = (float)MAX_FAULT_DETECTION_COUNTER/eventCtxPtr->debounceCounterBasedConfig.
                failedThreshold;
        eventCtxPtr->debounceCounter = faultDetectionCounter/ratio;
        LE_INFO("fdc=%d, debouncecounter=%d", faultDetectionCounter, eventCtxPtr->debounceCounter);
#endif
    }
    else if(node.get<string>("base") == "Timer")
    {
        cfg::Timer_t timeDebounce;
        float timeFailedThreshold;
        float timePassedThreshold;
        float fdcThreshold;

        cfg::top_debounce_algorithm<string>("short_name",  debounceAlgorism.c_str(), &timeDebounce);

        eventCtxPtr->debounceType = TAF_DIAGEVENT_DEBOUNCE_TIME_BASED;

        if(timeDebounce.debounce_behavior == "reset")
        {
            LE_INFO("debounce_behavior : RESET");
            eventCtxPtr->debounceBehavior_ = TAF_DIAGEVENT_DEBOUNCE_RESET;
        }
        else
        {
            LE_INFO("debounce_behavior : FREEZE");
            eventCtxPtr->debounceBehavior_ = TAF_DIAGEVENT_DEBOUNCE_FREEZE;
        }

        timeFailedThreshold = timeDebounce.time_failed_threshold;
        LE_INFO("timeFailedThreshold : %f", timeFailedThreshold);
        //Check the value range
        if((timeFailedThreshold < MIN_TIME_BASED_PARAM_VALUE) ||
                (timeFailedThreshold > MAX_TIME_BASED_PARAM_VALUE))
        {
            LE_FATAL("failedThreshold is incorrect");
        }
        //Change to milli second
        eventCtxPtr->debounceTimeBasedConfig.failedThreshold = timeFailedThreshold * 1000;
        LE_INFO("failedThreshold : %d", eventCtxPtr->debounceTimeBasedConfig.failedThreshold);

        timePassedThreshold = timeDebounce.time_passed_threshold;
        //Check the value range
        if((timePassedThreshold < MIN_TIME_BASED_PARAM_VALUE) ||
                (timePassedThreshold > MAX_TIME_BASED_PARAM_VALUE))
        {
            LE_FATAL("passedThreshold is incorrect");
        }
        //Change to milli second
        eventCtxPtr->debounceTimeBasedConfig.passedThreshold = timePassedThreshold * 1000;
        LE_INFO("passedThreshold : %d", eventCtxPtr->debounceTimeBasedConfig.passedThreshold);

        fdcThreshold = timeDebounce.time_fdc_threshold;
        LE_INFO("fdcThreshold : %f", fdcThreshold);
        //Check the value range
        if((fdcThreshold < MIN_TIME_BASED_PARAM_VALUE) ||
                (fdcThreshold > MAX_TIME_BASED_PARAM_VALUE))
        {
            LE_FATAL("fdcThreshold is incorrect");
        }
        //Change to milli second
        eventCtxPtr->debounceTimeBasedConfig.fdcThreshold = fdcThreshold * 1000;
        LE_INFO("fdcThreshold : %d", eventCtxPtr->debounceTimeBasedConfig.fdcThreshold);

        eventCtxPtr->timerType = TAF_DIAGEVENT_TIMER_UNKNOWN;

        // Create debounce timer.
        char timerName[32] = {0};
        snprintf(timerName, sizeof(timerName)-1, "debounceTimer-%d", eventId);

        eventCtxPtr->timerRef = le_timer_Create(timerName);
        le_timer_SetHandler(eventCtxPtr->timerRef, TimeBasedDebounceTimerHandler);
        le_timer_SetRepeat(eventCtxPtr->timerRef, 1);
        le_timer_SetContextPtr(eventCtxPtr->timerRef, eventCtxPtr);

        // Create fault detection counter timer for timer based debounce.
        char fdcTimerName[32] = {0};
        snprintf(fdcTimerName, sizeof(fdcTimerName)-1, "fdcTimer-%d", eventId);

        eventCtxPtr->fdcTimerRef = le_timer_Create(fdcTimerName);
        le_timer_SetMsInterval(eventCtxPtr->fdcTimerRef,
                eventCtxPtr->debounceTimeBasedConfig.fdcThreshold);
        le_timer_SetHandler(eventCtxPtr->fdcTimerRef, FdcTimerHandler);
        le_timer_SetRepeat(eventCtxPtr->fdcTimerRef, 1);
        le_timer_SetContextPtr(eventCtxPtr->fdcTimerRef, eventCtxPtr);

    }
    else if(node.get<string>("base") == "Custom")
    {
        eventCtxPtr->debounceType = TAF_DIAGEVENT_DEBOUNCE_MONITOR_INTERNAL;
    }
    else
    {
        LE_FATAL("Debounce type is not supported");
    }

    eventCtxPtr->dtcCtxPtr = GetDtcCtxByCode(eventCtxPtr->dtcCode);
    if(eventCtxPtr->dtcCtxPtr == NULL)
    {
        eventCtxPtr->dtcCtxPtr = InitDtcContext(eventCtxPtr->dtcCode, eventCtxPtr->eventId);
        LE_INFO("Create dtc context by dtc code=0x%x", eventCtxPtr->dtcCode);
    }
    else
    {
        LE_INFO("Dtc CTX already created");
    }

    eventCtxPtr->failureCounter = 0;
#ifndef LE_CONFIG_DIAG_FEATURE_A
    eventCtxPtr->debounceCounter = 0;
#endif
    eventCtxPtr->fdcTriggerFlag = false;
    eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_UNKNOWN;
    //get event UDS status from database
    eventCtxPtr->eventUdsStatus = taf_DataAccess_GetEventStatus(eventId);
    LE_INFO("Database: Get EventId:%d, event UDS status in DB:0x%x", eventId,
            eventCtxPtr->eventUdsStatus);
    eventCtxPtr->svcRef = NULL;
    eventCtxPtr->link = LE_DLS_LINK_INIT;

    // Add event id list into dtc context list
    InitEventIdListForDtc(eventCtxPtr->dtcCtxPtr,eventCtxPtr->eventId);

    // add this event context to list
    le_dls_Queue(&diagEvent.EventCtxList, &eventCtxPtr->link);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Intialize event id list for DTC.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::InitEventIdListForDtc
(
    taf_diagEvent_DtcCtx_t* dtcCtxPtr,
    uint16_t eventId
)
{
    taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = NULL;
    auto& diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "dtcCtxPtr is NULL");
    eventIdInfoPtr = (taf_DiagEvent_EventIdInfo_t *)le_mem_ForceAlloc(diagEvent.EventIdPool);
    TAF_ERROR_IF_RET_NIL(eventIdInfoPtr == NULL, "cannot alloc eventIdInfoPtr");

    LE_INFO("Init event list for DTC code:0x%x, eventId:%d", dtcCtxPtr->dtcCode, eventId);
    memset(eventIdInfoPtr, 0, sizeof(taf_DiagEvent_EventIdInfo_t));

    eventIdInfoPtr->eventId = eventId;
    eventIdInfoPtr->link = LE_DLS_LINK_INIT;

    // add this DTC context to list
    le_dls_Queue(&dtcCtxPtr->dtcEventIdList, &eventIdInfoPtr->link);

}

//-------------------------------------------------------------------------------------------------
/**
 * Intialize DTC context.
 */
//-------------------------------------------------------------------------------------------------
taf_diagEvent_DtcCtx_t* taf_EventSvr::InitDtcContext
(
    uint32_t dtcCode,
    uint16_t eventId
)
{
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = NULL;
    auto& diagEvent = taf_EventSvr::GetInstance();

    dtcCtxPtr = (taf_diagEvent_DtcCtx_t *)le_mem_ForceAlloc(diagEvent.DtcPool);
    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "cannot alloc dtcCtxPtr");

    memset(dtcCtxPtr, 0, sizeof(taf_diagEvent_DtcCtx_t));

    dtcCtxPtr->dtcCode = dtcCode;

    //get DTC status from database
    dtcCtxPtr->dtcStatus = taf_DataAccess_GetDTCStatus(dtcCode);
    LE_INFO("Database: get DTC code:0x%x, DTC status in DB:0x%x", dtcCode, dtcCtxPtr->dtcStatus);
    //get occurrence counter from database
    dtcCtxPtr->occurrenceCounter = taf_DataAccess_GetDTCOccurrenceCounter(dtcCode);
    LE_INFO("Database: get DTC code:0x%x, occurrence counter in DB:0x%x", dtcCode,
            dtcCtxPtr->occurrenceCounter);

    //Get activation status from database
    dtcCtxPtr->activationStatus = taf_DataAccess_GetDTCActivation(dtcCode);
    LE_INFO("Database: get DTC code:0x%x, activation status in DB:0x%x", dtcCode,
            dtcCtxPtr->activationStatus);

    //Get suppression status from database
    dtcCtxPtr->suppressionStatus = taf_DataAccess_GetDTCSuppression(dtcCode);
    LE_INFO("Database: get DTC code:0x%x, suppression status in DB:0x%x", dtcCode,
            dtcCtxPtr->suppressionStatus);

    //Get occurrenceCounterProcessing
    cfg::Node & root = cfg::get_root_node();
    cfg::Node & common = root.get_child("common_props");
    cfg::common_props_t commonProps;

    commonProps.occurrence_counter_processing =
            common.get<std::string>("occurrence_counter_processing");

    if(commonProps.occurrence_counter_processing == "TEST-FAILED-BIT")
    {
        LE_INFO("occurrence_counter_processing : TEST-FAILED-BIT");
        dtcCtxPtr->occurrenceCounterProcessing = TAF_DIAGEVENT_PROCESS_OCCCTR_TF;
    }
    else
    {
        LE_INFO("occurrence_counter_processing : %s",
                commonProps.occurrence_counter_processing.c_str());
        dtcCtxPtr->occurrenceCounterProcessing = TAF_DIAGEVENT_PROCESS_OCCCTR_CDTC;
    }

#ifdef LE_CONFIG_DIAG_FEATURE_A
    //Get DTC type
    try
    {
        cfg::Node & dtcNode = cfg::get_dtc_node(dtcCode);
        cfg::Node & sesType = dtcNode.get_child("access.session");

        dtcCtxPtr->dtcType = TAF_DIAGEVENT_APPLICATION_DTC;
        for (const auto & session: sesType)
        {
            string type = session.second.get_value<string>("");

            cfg::Node & sesNode = cfg::top_diagnostic_session<string>("short_name", type);
            int session_id = sesNode.get<int>("id");

            if ((uint8_t)session_id == FEATURE_A_PROGRAMMING_SESSION ||
                    (uint8_t)session_id == FEATURE_A_FOTA_SESSION)
            {
                dtcCtxPtr->dtcType = TAF_DIAGEVENT_REPROGRAMMING_DTC;
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Exception: %s", e.what());
        dtcCtxPtr->dtcType = TAF_DIAGEVENT_UNKNOWN_DTC;
    }
    LE_INFO("----dtc code=0x%x, dtc type=%d", dtcCode, dtcCtxPtr->dtcType);
#endif
    dtcCtxPtr->link = LE_DLS_LINK_INIT;
    dtcCtxPtr->dtcEventIdList = LE_DLS_LIST_INIT;

    // add this DTC context to list
    le_dls_Queue(&diagEvent.DtcCtxList, &dtcCtxPtr->link);

    return dtcCtxPtr;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear DTC and event data.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::ClearDTCAndEventData
(
    void* param1Ptr,
    void* param2Ptr
)
{
#ifndef LE_CONFIG_DIAG_FEATURE_A
    le_result_t result;
#endif
    uint8_t oldDtcStatus = 0;
    uint8_t oldEventStatus = 0;
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr;
    auto &diagEvent = taf_EventSvr::GetInstance();
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = (taf_diagEvent_DtcCtx_t*)param1Ptr;

    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "dtcCtxPtr is NULL");

    LE_DEBUG("Clear DTC, code = 0x%x", dtcCtxPtr->dtcCode);
    //Clear DTC data in memory
    oldDtcStatus = dtcCtxPtr->dtcStatus;

    //Clear occurrence counter
    dtcCtxPtr->occurrenceCounter=0;
    //Clear DTC status
    dtcCtxPtr->dtcStatus = 0;

#ifndef LE_CONFIG_DIAG_FEATURE_A
    //Set bit 4 to value 1
    dtcCtxPtr->dtcStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCSLC;
    //Set bit 6 to value 1
    dtcCtxPtr->dtcStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCTOC;
#endif

    if (dtcCtxPtr->dtcStatus != oldDtcStatus)
    {
        //Send DTC status change indication,--To do
        LE_DEBUG("DTC status changed, code = 0x%x", dtcCtxPtr->dtcCode);
        diagEvent.ReportDtcStatus(dtcCtxPtr);
    }

#ifndef LE_CONFIG_DIAG_FEATURE_A
    //The data is already cleared in data base, need to set it.
    LE_DEBUG("Database:Store DTC code:0x%x, status:0x%x, occurrence counter:%d",
            dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus, dtcCtxPtr->occurrenceCounter);
    result = taf_DataAccess_SetDTCStatus(dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus,
            dtcCtxPtr->occurrenceCounter);
    if(result != LE_OK)
    {
        LE_CRIT("Can't store data into database, dtcCode:0x%x, status:0x%x",
                dtcCtxPtr->dtcCode, dtcCtxPtr->dtcStatus);
    }
#endif
    //Clear events of this DTC
    eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
    while (eventIdLinkPtr)
    {
        taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                taf_DiagEvent_EventIdInfo_t, link);
        eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);

        eventCtxPtr = diagEvent.GetEventCtxById(eventIdInfoPtr->eventId);
        if(eventCtxPtr == NULL)
        {
            LE_ERROR("Can't get event context by eventId:%d", eventIdInfoPtr->eventId);
            continue;
        }

        oldEventStatus = eventCtxPtr->eventUdsStatus;
        //Clear Event status
        eventCtxPtr->eventUdsStatus = 0;
#ifndef LE_CONFIG_DIAG_FEATURE_A
        //Set bit 4 to value 1
        eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCSLC;
        //Set bit 6 to value 1
        eventCtxPtr->eventUdsStatus |= TAF_DIAGEVENT_UDS_STATUS_TNCTOC;
#endif
        if (eventCtxPtr->eventUdsStatus != oldEventStatus)
        {
            LE_INFO("Event status changed, event Id = %d", eventCtxPtr->eventId);
            diagEvent.ReportEventUdsStatus(eventCtxPtr);
        }

#ifndef LE_CONFIG_DIAG_FEATURE_A
        //The data is already cleared in database, need to store it
        LE_DEBUG("Database:Store and report eventId:%d, status:0x%x", eventCtxPtr->eventId,
                eventCtxPtr->eventUdsStatus);
        result = taf_DataAccess_SetEventStatus(eventCtxPtr->eventId, eventCtxPtr->eventUdsStatus);
        if(result != LE_OK)
        {
            LE_CRIT("Can't store data into database, EventId:%d, status:0x%x",
                    eventCtxPtr->eventId, eventCtxPtr->eventUdsStatus);
        }
#endif

        //Clear other data
        eventCtxPtr->failureCounter = 0;
        eventCtxPtr->eventFaultStatus = TAF_DIAGEVENT_UNKNOWN;
#ifndef LE_CONFIG_DIAG_FEATURE_A
        diagEvent.ResetDebounceCounter(eventCtxPtr);
#endif
    }

}

//-------------------------------------------------------------------------------------------------
/**
 * Clear all DTC.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::ClearAllDtc
(
)
{
    le_dls_Link_t* linkPtr = NULL;

    //Call data handle module api to delete data from database;
    le_result_t result = taf_DataAccess_DeleteAllData();
    LE_INFO("Database: delete all data");

    if(result != LE_OK)
    {
        LE_ERROR("Failed to delete all data from DB, result =%d", result);
        return result;
    }

    //Init dtc data in memory
    linkPtr = le_dls_Peek(&DtcCtxList);
    while (linkPtr)
    {
        taf_diagEvent_DtcCtx_t* dtcCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_DtcCtx_t, link);
        linkPtr = le_dls_PeekNext(&DtcCtxList, linkPtr);

        le_event_QueueFunctionToThread(mainThrRef, ClearDTCAndEventData, dtcCtxPtr, NULL);
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear single DTC.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::ClearSingleDtc
(
    uint32_t dtcCode
)
{
    le_result_t result;
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "DTC code is not supported");

    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_INFO("DTC code:0x%x is suppressed", dtcCode);
        return LE_UNAVAILABLE;
    }

    //Call data handle module api to delete data from database;
    LE_INFO("Database: delete dtc info, DTC code:0x%x", dtcCode);
    result = taf_DataAccess_DeleteData(dtcCode);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to delete data from DB, dtcCode=0x%x, result =%d", dtcCode, result);
        return result;
    }

    le_event_QueueFunctionToThread(mainThrRef, ClearDTCAndEventData, dtcCtxPtr, NULL);

    return LE_OK;

}

//-------------------------------------------------------------------------------------------------
/**
 * Clear DTC, this interface function is called by DTC service and DTC API
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::ClearDtc
(
    uint32_t dtcCode
)
{

    LE_DEBUG("ClearDtc: DTC CODE:0x%x", dtcCode);
    //All group
    if(dtcCode == 0xFFFFFF)
        return ClearAllDtc();
    else
        return ClearSingleDtc(dtcCode);
}

//-------------------------------------------------------------------------------------------------
/**
 * Disable DTC setting
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::DisableDTCSetting
(
    uint32_t dtcCode
)
{
    le_result_t result;
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "DTC code is not supported");

    //Call data handle module to set activation status
    LE_INFO("Database: disable DTC setting, DTC code:0x%x", dtcCode);
    result = taf_DataAccess_SetDTCActivation(dtcCode, false);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set activation status");
        return result;
    }

    dtcCtxPtr->activationStatus = false;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Enable DTC setting
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::EnableDTCSetting
(
    uint32_t dtcCode
)
{
    le_result_t result;
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "DTC code is not supported");

    //Call data access module to set activation status
    LE_INFO("Database: enable DTC setting, DTC code:0x%x", dtcCode);
    result = taf_DataAccess_SetDTCActivation(dtcCode, true);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set activation status");
        return result;
    }

    dtcCtxPtr->activationStatus = true;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set DTC suppression
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetDTCSuppression
(
    uint32_t dtcCode,
    bool suppressionStatus
)
{
    le_result_t result;
    taf_diagEvent_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "DTC code is not supported");

    //Call data access module to set suppression status
    LE_INFO("Database: set suppression DTC code:0x%x, status:%d", dtcCode, suppressionStatus);

    result = taf_DataAccess_SetDTCSuppression(dtcCode, suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set suppression status");
        return result;
    }

    dtcCtxPtr->suppressionStatus = suppressionStatus;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set all DTC suppression
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::SetAllDTCSuppression
(
    bool suppressionStatus
)
{
    le_result_t result;

    //Call data access module to set suppression status
    LE_INFO("Database: set suppression for all DTC, status:%d", suppressionStatus);

    result = taf_DataAccess_SetAllDTCSuppression(suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set suppression status for all DTC");
        return result;
    }

    UpdateAllDtcSuppressionStatus(suppressionStatus);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get fault detection
 * This api is provided for the application
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_EventSvr::GetFaultDetectionCounter
(
    uint32_t dtcCode,
    uint8_t* faultDetectionCounterPtr
)
{
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr = NULL;
    uint8_t eventFDC = 0;
    uint8_t dtcFDC = 0;

    TAF_ERROR_IF_RET_VAL(faultDetectionCounterPtr == NULL, LE_BAD_PARAMETER, "counterPtr is null");

    taf_diagEvent_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_BAD_PARAMETER, "dtcCtxPtr is null");
    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_INFO("DTC code:0x%x is suppressed", dtcCtxPtr->dtcCode);
        return LE_FAULT;
    }

    dtcFDC = 0;
    //Get event list of this DTC
    eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
    while (eventIdLinkPtr)
    {
        taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                taf_DiagEvent_EventIdInfo_t, link);
        eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);
        LE_DEBUG("event id =%d",eventIdInfoPtr->eventId);

        eventCtxPtr = GetEventCtxById(eventIdInfoPtr->eventId);
        if(eventCtxPtr == NULL)
            continue;

        if(eventCtxPtr->debounceType !=TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED)
            continue;

        eventFDC = CalcCounterBasedDebounceFDC(eventCtxPtr);//Get FDC by debounceCounter
        //Get the maximal counter
        if(dtcFDC < eventFDC)
            dtcFDC = eventFDC;
    }

    LE_DEBUG("fault detection counter = %d", dtcFDC);
    *faultDetectionCounterPtr = dtcFDC;
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update suppression status for all DTC.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::UpdateAllDtcSuppressionStatus
(
    bool suppressionStatus
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DtcCtxList);
    while (linkPtr)
    {
        taf_diagEvent_DtcCtx_t* dtcCtxPtr = CONTAINER_OF(linkPtr, taf_diagEvent_DtcCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&DtcCtxList, linkPtr);

        dtcCtxPtr->suppressionStatus = suppressionStatus;
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets fault detection counter, this interface function is called by DTC service.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::ReportDTCFaultDetectionCounter
(
    le_dls_List_t* fdcInfoListPtr
)
{
    le_dls_Link_t* dtcLinkPtr = NULL;
    le_dls_Link_t* eventIdLinkPtr = NULL;
    taf_diagEvent_EventCtx_t* eventCtxPtr = NULL;
    uint8_t eventFDC = 0;
    uint8_t dtcFDC = 0;

    //Get dtc list
    dtcLinkPtr = le_dls_Peek(&DtcCtxList);
    while (dtcLinkPtr)
    {

        taf_diagEvent_DtcCtx_t* dtcCtxPtr = CONTAINER_OF(dtcLinkPtr, taf_diagEvent_DtcCtx_t, link);

        dtcLinkPtr = le_dls_PeekNext(&DtcCtxList, dtcLinkPtr);

        LE_DEBUG("DTC CODE = 0x%x",dtcCtxPtr->dtcCode);

        //Check if DTC is suppressed
        if(dtcCtxPtr->suppressionStatus == true)
        {
            LE_INFO("DTC code:0x%x is suppressed", dtcCtxPtr->dtcCode);
            continue;
        }

        dtcFDC = 0;
        //Get event list of this DTC
        eventIdLinkPtr = le_dls_Peek(&dtcCtxPtr->dtcEventIdList);
        while (eventIdLinkPtr)
        {
            taf_DiagEvent_EventIdInfo_t* eventIdInfoPtr = CONTAINER_OF(eventIdLinkPtr,
                    taf_DiagEvent_EventIdInfo_t, link);
            eventIdLinkPtr = le_dls_PeekNext(&dtcCtxPtr->dtcEventIdList, eventIdLinkPtr);
            LE_DEBUG("event id =%d",eventIdInfoPtr->eventId);

            eventCtxPtr = GetEventCtxById(eventIdInfoPtr->eventId);
            if(eventCtxPtr == NULL)
                continue;

            switch(eventCtxPtr->debounceType)
            {
                case TAF_DIAGEVENT_DEBOUNCE_MONITOR_INTERNAL:

                    LE_DEBUG("calculate internal fdc");

                break;
                case TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED:
                    LE_DEBUG("calculate counter based fdc");
                    eventFDC = CalcCounterBasedDebounceFDC(eventCtxPtr);//Get FDC by debounceCounter
                    //Get the maximal counter
                    if(dtcFDC < eventFDC)
                        dtcFDC = eventFDC;
                break;
                case TAF_DIAGEVENT_DEBOUNCE_TIME_BASED:
                    LE_DEBUG("calculate time based fdc");
                    eventFDC =  CalcTimeBasedDebounceFDC(eventCtxPtr);//Get FDC by debounceCounter
                    //Get the maximal counter
                    if(dtcFDC < eventFDC)
                        dtcFDC = eventFDC;
                break;
                default:
                    LE_ERROR("Wrong debounce type");

            }
        }

        if(dtcFDC > 0)
        {
            LE_INFO("Add DTC code=0x%x, FDC =%d into list", dtcCtxPtr->dtcCode ,dtcFDC);
            taf_DiagEvent_FDCInfo_t* fdcInfoPtr = NULL;

            //Need to be released by DTC service module
            fdcInfoPtr = (taf_DiagEvent_FDCInfo_t *)le_mem_ForceAlloc(FdcInfoPool);

            fdcInfoPtr->dtc = dtcCtxPtr->dtcCode;
            fdcInfoPtr->fdc = dtcFDC;
            fdcInfoPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(fdcInfoListPtr, &(fdcInfoPtr->link));

        }

    }

    return ;
}

//-------------------------------------------------------------------------------------------------
/**
 * Calculates counterbased debounce fault detection counter.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_EventSvr::CalcCounterBasedDebounceFDC
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    float ratio = 0;

    LE_DEBUG("event id =%d, debounce counter = %d, failedThreshold =%d",
            eventCtxPtr->eventId,eventCtxPtr->debounceCounter,
            eventCtxPtr->debounceCounterBasedConfig.failedThreshold);

    //Only send prefailed counter
    if(eventCtxPtr->debounceCounter > 0 &&
            eventCtxPtr->debounceCounter < eventCtxPtr->debounceCounterBasedConfig.failedThreshold)
    {
        //Get the linear ratio
        ratio=(float)MAX_FAULT_DETECTION_COUNTER/eventCtxPtr->debounceCounterBasedConfig.
                failedThreshold;
        LE_DEBUG("eventId:%d,debounceCounter=%d, ratio=%f", eventCtxPtr->eventId,
                eventCtxPtr->debounceCounter, ratio);
        //Calculate FDC with the ratio
        return eventCtxPtr->debounceCounter * ratio;
    }
    else
    {
        LE_DEBUG("eventId:%d, not prefailed", eventCtxPtr->eventId);
        return 0;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Calculates timebased debounce fault detection counter.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_EventSvr::CalcTimeBasedDebounceFDC
(
    taf_diagEvent_EventCtx_t* eventCtxPtr
)
{
    uint64_t curTime;
    uint64_t elapsedTime;
    float ratio = 0;

    //Timer is running and it's prefailed, calculate the fault detection counter
    if(le_timer_IsRunning(eventCtxPtr->timerRef) &&
            eventCtxPtr->timerType == TAF_DIAGEVENT_TIMER_PREFAILED)
    {
        curTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
        elapsedTime = curTime - eventCtxPtr->startTime;

        LE_DEBUG("eventId:%d, start time is %" PRIu64 ", cur time is %" PRIu64 "",
                eventCtxPtr->eventId, eventCtxPtr->startTime, curTime);
        //Elasped time is not valid
        if(elapsedTime == 0 ||
                elapsedTime == eventCtxPtr->debounceTimeBasedConfig.failedThreshold)
        {
            LE_DEBUG("eventId:%d, Elapsed time is %" PRIu64 "", eventCtxPtr->eventId, elapsedTime);
            return 0;
        }
        //Get the linear ratio, calculate ratio with milli-second
        ratio=(float)(MAX_FAULT_DETECTION_COUNTER*1000)/eventCtxPtr->debounceTimeBasedConfig.
                failedThreshold;
        LE_DEBUG("eventId:%d, elasped time=%" PRIu64 ", ratio=%f",eventCtxPtr->eventId, elapsedTime,
                ratio);
        //Calculate FDC with the ratio
        elapsedTime = elapsedTime/1000;//Change milli-second to second
        return elapsedTime * ratio;

    }
    else
    {
        LE_DEBUG("eventId:%d,Timer is not running or prepassed", eventCtxPtr->eventId);

        return 0;
    }

}

//-------------------------------------------------------------------------------------------------
/**
 * Handle the client disconnection.
 */
//-------------------------------------------------------------------------------------------------
void taf_EventSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    auto& diagEvent = taf_EventSvr::GetInstance();
    LE_DEBUG(" Client %p disconnected", sessionRef);
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is nullptr!");

    //Find event context one by one
    le_ref_IterRef_t iterRef = le_ref_GetIterator(diagEvent.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_diagEvent_EventCtx_t* eventCtxPtr = (taf_diagEvent_EventCtx_t*)le_ref_GetValue(iterRef);

        if (eventCtxPtr != NULL)
        {
            //Remove client session reference from event session reference list
            if( diagEvent.RemoveSessionFromEventCtx(eventCtxPtr, sessionRef) == LE_OK)
                LE_DEBUG("remove event from context, event id %d", eventCtxPtr->eventId);
        }

        //If session number of links is 0, release event context
        if( le_dls_NumLinks(&eventCtxPtr->sessionRefList)  == 0)
        {
            // Clear service object
            LE_INFO(" clear event id %d context", eventCtxPtr->eventId);
            le_ref_DeleteRef(diagEvent.SvcRefMap, (void*)eventCtxPtr->svcRef);
            eventCtxPtr->svcRef = NULL;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get DTC list and event list from configuration module, and initialize the contexts.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::EventConfiguration(cfg::Node & node)
{
    uint32_t dtcCode;
    uint16_t eventId;

    std::map<uint32_t, std::shared_ptr<cfg::Node>> dtc_map = cfg::get_dtc_nodes();

    for (const auto & dtc: dtc_map)
    {
        dtcCode= dtc.first;
        LE_INFO("dtc code=0x%x",dtcCode);

        if((dtcCode & 0xff000000) != 0)
        {
            LE_FATAL("Incorrect DTC code:%d",dtcCode);
        }

        std::map<uint16_t, std::shared_ptr<cfg::Node>> ev_map = cfg::get_event_nodes(dtc.first);
        for (const auto & event : ev_map)
        {
            eventId = event.first;

            if(eventId == 0)
            {
                LE_FATAL("Incorrect event id:%d", eventId);
            }

            InitEventContext(dtc, event);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_EventSvr::Init
(
    void
)
{
    LE_INFO("taf_EventSvr Init!");

    // Create diag event memory pools.
    EventPool = le_mem_CreatePool("EventSvcPool", sizeof(taf_diagEvent_EventCtx_t));

    // Create DTC memory pools.
    DtcPool = le_mem_CreatePool("DtcSvcPool", sizeof(taf_diagEvent_DtcCtx_t));

    // Create fdc information pools.
    FdcInfoPool = le_mem_CreatePool("FdcInfoPool", sizeof(taf_DiagEvent_FDCInfo_t));

    // Create event id information pools.
    EventIdPool = le_mem_CreatePool("EventIdPool", sizeof(taf_DiagEvent_EventIdInfo_t));

    // Create event uds status pools.
    EventUdsStatusPool = le_mem_CreatePool("eventUdsStatusPool", sizeof(taf_diagEvent_UdsStatus_t));

    // Create enable condition state pools.
    EnableCondStatePool = le_mem_CreatePool("enableCondStatePool",
            sizeof(taf_diagEvent_EnableCondState_t));

    // Create session reference pools.
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef, TAF_EVENT_MAX_SESSION_REF,
            sizeof(taf_diagEvent_SessionRef_t));

    // Create reference maps.
    SvcRefMap = le_ref_CreateMap("EventSvcRefMap", DEFAULT_EVENT_SVC_REF_CNT);

    // Set client session close handler.
    le_msg_AddServiceCloseHandler(taf_diagEvent_GetServiceRef(), OnClientDisconnection, NULL);

    // Get the main thread reference.
    mainThrRef = le_thread_GetCurrent();

    //Initialize the data from configuration module.
    try
    {
        cfg::Node & root = cfg::get_root_node();
        EventConfiguration(root);
    }
    catch (const std::exception& e)
    {
        LE_FATAL("Exception: %s", e.what() );
    }

    LE_INFO("Diag Event Service started!");
}

