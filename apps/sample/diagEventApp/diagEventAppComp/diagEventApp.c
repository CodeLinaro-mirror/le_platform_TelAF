/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define EVENT_ID_0001 0001
#define DRIVING_CYCLE_ID 1 // The operation cycle id value of Event id 1
#define ENABLE_CONDITION_ID1 0 // The enable condition id value of Event id 1 and event id 2
#define SUPPLIER_FAULT_CODE_LEN 5
#define MAX_PREFAILED_NUMBER 20
#define SECOND_MAX_PREFAILED_NUMBER 8
#define PREPASSED_INDEX 5

static le_sem_Ref_t semRef;

//Diag Event
static taf_diagEvent_ServiceRef_t diagEvent0001SvcRef = NULL;
static taf_diagEvent_UdsStatusHandlerRef_t udsStatusRef = NULL;

static void* changeEventStatus()
{
    le_result_t result;
    uint8_t eventUdsStatus;
    uint16_t eventId;
    uint8_t supplierFaultCode[SUPPLIER_FAULT_CODE_LEN]={0x33, 0x34, 0x35, 0x36, 0x37};

    diagEvent0001SvcRef = taf_diagEvent_GetService(EVENT_ID_0001);
    if(diagEvent0001SvcRef == NULL)
    {
        LE_ERROR("Failed to get diagEvent service");
        return NULL;
    }

    result = taf_diagEvent_GetId(diagEvent0001SvcRef, &eventId);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event id, result=%d", result);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id : %d, UDS status : 0x%x", eventId, eventUdsStatus);

    LE_INFO("Start first operation cycle");
    //failureCounter =0
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef, TAF_DIAGEVENT_FAILED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //failureCounter = 1
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    LE_INFO("Start second operation cycle");
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //failureCounter = 2
    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PASSED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PASSED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    LE_INFO("Start third operation cycle");
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //DTC is confirmed(confirmation_threshold = 3 in YAML file ), failureCounter = 0
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    LE_INFO("Start fourth operation cycle");
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with fault code, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PASSED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PASSED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with fault code result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    //test counter based debounce , set prefailed 20 times which is more than
    //counter_failed_threshold which is 10
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    //Set prefailed from index 0 to 4, set prepassed with index 5, set prefailed again from 6 to 19
    for(int i=0; i<MAX_PREFAILED_NUMBER; i++)
    {
        if(i == PREPASSED_INDEX)
        {
            LE_INFO("Set prepassed -- eventId:%d, index:%d", eventId, i);
            result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PREPASSED);
            if(result != LE_OK)
            {
                LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREPASSED,\
                result:%d, event id:%d", result, eventId);
                return NULL;
            }
        }
        else
        {
            LE_INFO("Set prefailed -- eventId:%d, index:%d", eventId, i);
            result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PREFAILED);
            if(result != LE_OK)
            {
                LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREFAILED,\
                result:%d, event id:%d", result, eventId);
                return NULL;
            }
        }
    }

    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    //test counter based , set prefailed 8 times, counter_failed_threshold is 10
    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    for(int i=0; i<SECOND_MAX_PREFAILED_NUMBER; i++)
    {
        LE_INFO("Set prefailed -- eventId:%d, index:%d", eventId, i);
        result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PREFAILED);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREFAILED,\
             result:%d, event id:%d", result, eventId);
            return NULL;
        }
    }
    //Debounce counter = 8
    //Switch debounce behavior to RESET
    result = taf_diagEvent_ResetDebounceStatus(diagEvent0001SvcRef, TAF_DIAGEVENT_DEBOUNCE_RESET);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to reset debounce status to TAF_DIAGEVENT_DEBOUNCE_RESET,\
                result:%d, eventId:%d", result, eventId);
        return NULL;
    }

    result = taf_diagEvent_SetEnableCondition(ENABLE_CONDITION_ID1, false);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set enable condition to false, result:%d, conditionId:%d", result,
                ENABLE_CONDITION_ID1);
        return NULL;
    }

    result = taf_diagEvent_SetEnableCondition(ENABLE_CONDITION_ID1, true);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set enable condition to true, result:%d, conditionId:%d", result,
                ENABLE_CONDITION_ID1);
        return NULL;
    }

    //Debounce counter = 0
    for(int i=0; i<SECOND_MAX_PREFAILED_NUMBER; i++)
    {
        LE_INFO("Set prefailed -- eventId:%d, index:%d", eventId, i);
        result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PREFAILED);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREFAILED,\
                    result:%d, event id:%d", result, eventId);
            return NULL;
        }
     }

    result = taf_diagEvent_SetOperationCycleState(DRIVING_CYCLE_ID, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, DRIVING_CYCLE_ID);
        return NULL;
    }

    //Debounce counter = 8

    return NULL;
}

void udsStatusChangeHandler
(
        taf_diagEvent_ServiceRef_t svcRef,
        uint8_t eventUdsStatus,
        void* contextPtr
)
{
    le_result_t result;
    uint16_t eventId;

    result = taf_diagEvent_GetId(svcRef, &eventId);
    if( result != LE_OK)
    {
        LE_ERROR("Failed to get event id");
        return;
    }

    LE_INFO("********Diag event %d, change status to 0x%x********", eventId, eventUdsStatus);
}

static void* diagEventUdsStatusTheadFunc(void* ctxPtr)
{
    taf_diagEvent_ServiceRef_t diagEventRef = NULL;
    taf_diagEvent_ConnectService();

    //Get the same diag event service
    diagEventRef = taf_diagEvent_GetService(EVENT_ID_0001);
    if(diagEventRef == NULL)
    {
        LE_ERROR("Get diagEvent service");
        return NULL;
    }

    udsStatusRef = taf_diagEvent_AddUdsStatusHandler(diagEventRef,
            (taf_diagEvent_UdsStatusHandlerFunc_t)udsStatusChangeHandler, ctxPtr);

    if(udsStatusRef == NULL)
    {
        LE_ERROR("Get diagEvent service");
        return NULL;
    }

    le_sem_Post(semRef);

    le_event_RunLoop();

    return NULL;
}

COMPONENT_INIT
{
    LE_INFO("diagEventApp starting");

    le_result_t result;
    semRef = le_sem_Create("SemRef", 0);

    result = taf_diagEvent_SetEnableCondition(ENABLE_CONDITION_ID1, true);

    if(result != LE_OK)
    {
        LE_ERROR("Failed to set enable condition");
    }

    // Create event uds status change thread
    le_thread_Ref_t eventUdsStatusThreadRef = le_thread_Create("udsStatusTh",
            diagEventUdsStatusTheadFunc, NULL);

    le_thread_Start(eventUdsStatusThreadRef);
    le_sem_Wait(semRef);

    changeEventStatus();

    LE_INFO("diagEventApp end");
}
