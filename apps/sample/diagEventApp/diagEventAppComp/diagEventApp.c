/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "diag_ids.h"

#ifdef LE_CONFIG_DIAG_FEATURE_A
#define Event_Sample_Big Event_ID_0x01
#define Condition_tooBig 5
#define Condition_toSmall 6
#endif

#define SUPPLIER_FAULT_CODE_LEN 5
#define MAX_PREFAILED_NUMBER 20
#define SECOND_MAX_PREFAILED_NUMBER 8
#define PREPASSED_INDEX 5

#define DTC_CODE_AB0000 0xab0000 //Defined in sample YAML file, event id 1 is in this DTC

static le_sem_Ref_t semRef;

//Diag Event
static taf_diagEvent_ServiceRef_t diagEvent0001SvcRef = NULL;
static taf_diagEvent_UdsStatusHandlerRef_t udsStatusRef = NULL;

//Diag DTC
static taf_diagDTC_ServiceRef_t diagDtcAB0000SvcRef = NULL;
static taf_diagDTC_StatusHandlerRef_t dtcStatusRef = NULL;

//Diag all DTC
static taf_diagDTC_AllServiceRef_t diagDtcAllSvcRef = NULL;
static taf_diagDTC_AllStatusHandlerRef_t allDtcStatusRef = NULL;

const char *dataTypeToString(taf_diagDTC_DataType_t dataType)
{
    switch (dataType)
    {
        case TAF_DIAGDTC_SNAPSHOT_DATA:
            return "Snapshot data";
        case TAF_DIAGDTC_EXTENDED_DATA:
            return "Extended data";
        default:
            LE_ERROR("unknown data type");
            return "unknow data type";
    }
}

//Sample for reading DTC data(snapshot/extended data)
static void getDTCData(taf_diagDTC_ServiceRef_t dtcSvcRef)
{
    le_result_t result;
    uint32_t dtcCode;
    taf_diagDTC_DataType_t dataType;
    uint16_t dataId;
    uint8_t dataValue[TAF_DIAGDTC_DATA_VALUE_MAX_BYTES];
    size_t dataLen = 0;
    uint8_t recordNumber;

    taf_diagDTC_DataListRef_t listRef=taf_diagDTC_GetDataList(dtcSvcRef);

    if(listRef !=NULL)
    {
        taf_diagDTC_DataRef_t dtcDataRef = taf_diagDTC_GetFirstData(listRef);
        while(dtcDataRef != NULL)
        {
            result = taf_diagDTC_GetDataDtcCode(dtcDataRef, &dtcCode);
            if(result == LE_OK)
            {
                LE_INFO("Get DTC code:0x%x", dtcCode);
            }
            else
            {
                LE_ERROR("Failed to get DTC code");
            }

            result = taf_diagDTC_GetRecordNumber(dtcDataRef, &recordNumber);
            if(result == LE_OK)
            {
                LE_INFO("Get DTC data record number:%d", recordNumber);
            }
            else
            {
                LE_ERROR("Failed to get DTC record number");
            }

            result = taf_diagDTC_GetDataType(dtcDataRef, &dataType);
            if(result == LE_OK)
            {
                LE_INFO("Get DTC data type:%s", dataTypeToString(dataType));
            }
            else
            {
                LE_ERROR("Failed to get DTC type");
            }

            if(dataType == TAF_DIAGDTC_SNAPSHOT_DATA)
            {
                result = taf_diagDTC_GetDataId(dtcDataRef, &dataId);
                if(result == LE_OK)
                {
                    LE_INFO("Get snapshot DID:0x%x", dataId);
                }
                else
                {
                    LE_ERROR("Failed to get snapshot DID");
                }
            }

            result = taf_diagDTC_GetDataValue(dtcDataRef, dataValue, &dataLen);
            if(result == LE_OK)
            {
                for( int i=0; i< (int)dataLen; i++)
                {
                    LE_INFO("Data[%d] = 0x%x", i, dataValue[i]);
                }
            }
            else
            {
                LE_ERROR("Failed to get DTC data");
            }

            dtcDataRef=taf_diagDTC_GetNextData(listRef);
        }

        result = taf_diagDTC_DeleteDataList(listRef);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to delete DTC data list");
        }
    }

    return;
}

//Sample for event API call and DTC API call
static void* changeEventStatus()
{
    le_result_t result;
    uint8_t eventUdsStatus, dtcStatus;
    uint16_t eventId;
    uint32_t dtcCode;
    uint8_t supplierFaultCode[SUPPLIER_FAULT_CODE_LEN]={0x33, 0x34, 0x35, 0x36, 0x37};
    uint8_t supplierFaultCode2[SUPPLIER_FAULT_CODE_LEN]={0x11, 0x12, 0x13, 0x14, 0x15};
    bool suppressionStatus;
    taf_diagDTC_ActivationStatus_t activationStatus;

    //Get the diag event service
    diagEvent0001SvcRef = taf_diagEvent_GetService(Event_Sample_Big);
    if(diagEvent0001SvcRef == NULL)
    {
        LE_ERROR("Failed to get diagEvent service");
        return NULL;
    }

    //Get the diag DTC service
    diagDtcAB0000SvcRef = taf_diagDTC_GetService(DTC_CODE_AB0000);
    if(diagDtcAB0000SvcRef == NULL)
    {
        LE_ERROR("Get diag DTC service");
        return NULL;
    }

    //Get the diag all DTC service
    diagDtcAllSvcRef = taf_diagDTC_GetAllService();
    if(diagDtcAllSvcRef == NULL)
    {
        LE_ERROR("Get diag all DTC service");
        return NULL;
    }

    //Call diagEvent API to get event ID and event UDS status
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

    // Call DTC API
    //Get the activation status and store the old status.
    result = taf_diagDTC_GetActivationStatus(diagDtcAB0000SvcRef, &activationStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get activation status for DTC code : 0x%x, result : %d", dtcCode,
                result);
        return NULL;
    }

    //Activate the DTC
    result = taf_diagDTC_SetActivationStatus(diagDtcAB0000SvcRef, TAF_DIAGDTC_ACTIVE);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to activate the DTC, code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    //Get the suppression status and store the old status.
    result = taf_diagDTC_GetSuppression(diagDtcAB0000SvcRef, &suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get suppression for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    //Cancel the DTC suppression
    result = taf_diagDTC_SetSuppression(diagDtcAB0000SvcRef, false);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to cancel the DTC suppression, code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    //Call diagDTC API to get DTC code and DTC status
    result = taf_diagDTC_GetCode(diagDtcAB0000SvcRef, &dtcCode);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get DTC code, result=%d", result);
        return NULL;
    }

    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to read DTC status for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    LE_INFO("DTC code : 0x%x, DTC status : 0x%x", dtcCode, dtcStatus);

    LE_INFO("Start first operation cycle");

    //failureCounter =0
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //failureCounter is 1
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    //Get event UDS status after first operation cycle
    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    //Read DTC status after first operation cycle
    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to read DTC status for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    LE_INFO("DTC code : 0x%x, DTC status : 0x%x", dtcCode, dtcStatus);

    //Suppression test
    //Set all DTC suppression status with true.
    result = taf_diagDTC_SetAllSuppression(diagDtcAllSvcRef, true);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set all DTC suppression with true, result:%d", result);
        return NULL;
    }

    //Get the suppression status.
    bool tmpSuppressionStatus;
    result = taf_diagDTC_GetSuppression(diagDtcAB0000SvcRef, &tmpSuppressionStatus);
    if(result != LE_OK || tmpSuppressionStatus != true)
    {
        LE_ERROR("Failed to get suppression for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    //Set all DTC suppression status with false.
    result = taf_diagDTC_SetAllSuppression(diagDtcAllSvcRef, false);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set all DTC suppression with false, result:%d", result);
        return NULL;
    }

    LE_INFO("Set all suppression successfully");

    //Suppress the DTC AB0000
    result = taf_diagDTC_SetSuppression(diagDtcAB0000SvcRef, true);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to suppress the DTC, code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    //Get DTC status after suppression.
    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    //DTC is suppressed, result should be LE_UNAVAILABLE.
    if(result != LE_UNAVAILABLE)
    {
        LE_ERROR("Suppression status is incorrect for DTC code:0x%x, result:%d", dtcCode, result);
        return NULL;
    }

    //Clear DTC after suppression status.
    result = taf_diagDTC_ClearInfo(diagDtcAB0000SvcRef);
    //DTC is suppressed, result should be LE_UNAVAILABLE.
    if(result != LE_UNAVAILABLE)
    {
        LE_ERROR("Suppression status is wrong for DTC code:0x%x, result:%d", dtcCode, result);
        return NULL;
    }

    //Cancel the DTC suppression
    result = taf_diagDTC_SetSuppression(diagDtcAB0000SvcRef, false);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to cancel the suppression for the DTC, code : 0x%x, result : %d", dtcCode,
                result);
        return NULL;
    }

    //Get DTC status.
    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to read status for DTC code:0x%x, result:%d", dtcCode, result);
        return NULL;
    }

    LE_INFO("DTC code : 0x%x, DTC status : 0x%x", dtcCode, dtcStatus);
    //Clear DTC.
    result = taf_diagDTC_ClearInfo(diagDtcAB0000SvcRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to clear the DTC, code:0x%x, result:%d", dtcCode, result);
        return NULL;
    }

    //Clear DTC.
    result = taf_diagDTC_ClearAllInfo(diagDtcAllSvcRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to clear all DTC result:%d", result);
        return NULL;
    }

    //failureCounter is 0 after the clear

    LE_INFO("Start second operation cycle");
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode2, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //failureCounter is 1
    result=taf_diagEvent_SetStatus(diagEvent0001SvcRef,TAF_DIAGEVENT_PASSED);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PASSED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    LE_INFO("Start third operation cycle");
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //failureCounter is 2
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    LE_INFO("Start fourth operation cycle");
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,TAF_DIAGEVENT_FAILED,
            supplierFaultCode2, SUPPLIER_FAULT_CODE_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set event status with TAF_DIAGEVENT_FAILED, result:%d, event id:%d",
                result, eventId);
        return NULL;
    }

    //DTC is confirmed(confirmation_threshold = 3 in YAML file ), failureCounter is 0
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    //Read DTC status after the fourth operation cycle
    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to read DTC status for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    LE_INFO("DTC code : 0x%x, DTC status : 0x%x", dtcCode, dtcStatus);

    LE_INFO("Start fifth operation cycle");
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
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

    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    result = taf_diagEvent_GetUdsStatus(diagEvent0001SvcRef, &eventUdsStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get event UDS status for event id : %d, result : %d", eventId, result);
        return NULL;
    }

    LE_INFO("Event id:%d, UDS status : 0x%x", eventId, eventUdsStatus);

    //Read DTC status after fifth operation cycle
    result = taf_diagDTC_ReadStatus(diagDtcAB0000SvcRef, &dtcStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to read DTC status for DTC code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    LE_INFO("DTC code : 0x%x, DTC status : 0x%x", dtcCode, dtcStatus);

    //test counter based debounce , set prefailed 20 times which is more than
    //counter_failed_threshold which is 10
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
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
            result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,
                    TAF_DIAGEVENT_PREFAILED, supplierFaultCode2, SUPPLIER_FAULT_CODE_LEN);
            if(result != LE_OK)
            {
                LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREFAILED,\
                result:%d, event id:%d", result, eventId);
                return NULL;
            }
        }
    }

    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    //test counter based , set prefailed 8 times, counter_failed_threshold is 10
    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_START);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to start operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    for(int i=0; i<SECOND_MAX_PREFAILED_NUMBER; i++)
    {
        LE_INFO("Set prefailed -- eventId:%d, index:%d", eventId, i);
        result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,
                TAF_DIAGEVENT_PREFAILED, supplierFaultCode2, SUPPLIER_FAULT_CODE_LEN);
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

    // set enable condition as false
    le_result_t res_5, res_6;
    res_5 = taf_diag_SetEnableCondition(Condition_tooBig, false);
    res_6 = taf_diag_SetEnableCondition(Condition_toSmall, false);
    if((res_5 != LE_OK) || (res_6 != LE_OK))
    {
        LE_ERROR("Failed to set enable condition to false");
        return NULL;
    }

    result = taf_diag_SetEnableCondition(Condition_tooBig, true);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set enable condition to true, result:%d, conditionId:%d", result,
                Condition_tooBig);
        return NULL;
    }

    //Debounce counter = 0
    for(int i=0; i<SECOND_MAX_PREFAILED_NUMBER; i++)
    {
        LE_INFO("Set prefailed -- eventId:%d, index:%d", eventId, i);
        result=taf_diagEvent_SetStatusWithSupplierFaultCode(diagEvent0001SvcRef,
                TAF_DIAGEVENT_PREFAILED, supplierFaultCode2, SUPPLIER_FAULT_CODE_LEN);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to set event status with TAF_DIAGEVENT_PREFAILED,\
                    result:%d, event id:%d", result, eventId);
            return NULL;
        }
     }

    result = taf_diagEvent_SetOperationCycleState(OperationCycle_DC, TAF_DIAGEVENT_CYCLE_STOP);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to stop operation cycle, result:%d, eventId:%d, operation cycle id:%d",
                result, eventId, OperationCycle_DC);
        return NULL;
    }

    //Debounce counter = 8

    getDTCData(diagDtcAB0000SvcRef);

    //Recover the suppression status for the DTC
    result = taf_diagDTC_SetSuppression(diagDtcAB0000SvcRef, suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to recover the suppression for DTC code : 0x%x, result : %d", dtcCode,
                result);
        return NULL;
    }

    //Recover the activation status for the DTC
    result = taf_diagDTC_SetActivationStatus(diagDtcAB0000SvcRef, activationStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to recover the DTC activation, code : 0x%x, result : %d", dtcCode, result);
        return NULL;
    }

    return NULL;
}

//Event UDS status change handler
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

//Enable condition state change handler
void enableCondStateChangeHandler
(
        taf_diagEvent_ServiceRef_t svcRef,
        bool state,
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

    LE_INFO("********Diag event %d, Enable Condition state change to %d********", eventId, state);
}

//DTC status change handler
void dtcStatusChangeHandler
(
        taf_diagDTC_ServiceRef_t svcRef,
        uint8_t dtcStatus,
        void* contextPtr
)
{
    le_result_t result;
    uint32_t dtcCode;

    result = taf_diagDTC_GetCode(svcRef, &dtcCode);
    if( result != LE_OK)
    {
        LE_ERROR("Failed to get DTC code");
        return;
    }

    LE_INFO("########Diag DTC 0x%x, change status to 0x%x########", dtcCode, dtcStatus);
}

//all DTC status change handler
void allDtcStatusChangeHandler
(
        taf_diagDTC_AllServiceRef_t svcRef,
        uint32_t dtcCode,
        uint8_t dtcStatus,
        void* contextPtr
)
{
    LE_INFO("########Diag one of all DTC 0x%x, change status to 0x%x########", dtcCode, dtcStatus);
}

//Diag event UDS status thread
static void* diagEventUdsStatusTheadFunc(void* ctxPtr)
{
    taf_diagEvent_ServiceRef_t diagEventRef = NULL;
    taf_diagEvent_ConnectService();

    //Get the same diag event service
    diagEventRef = taf_diagEvent_GetService(Event_Sample_Big);
    if(diagEventRef == NULL)
    {
        LE_ERROR("Get diagEvent service");
        return NULL;
    }

    udsStatusRef = taf_diagEvent_AddUdsStatusHandler(diagEventRef,
            (taf_diagEvent_UdsStatusHandlerFunc_t)udsStatusChangeHandler, ctxPtr);

    if(udsStatusRef == NULL)
    {
        LE_ERROR("Add event UDS status handler");
        return NULL;
    }

    le_sem_Post(semRef);

    le_event_RunLoop();

    return NULL;
}

//Enable Condition state thread
static void* enableCondStateTheadFunc(void* ctxPtr)
{
    taf_diagEvent_ServiceRef_t diagEventRef = NULL;
    taf_diagEvent_ConnectService();

    //Get the same diag event service
    diagEventRef = taf_diagEvent_GetService(Event_Sample_Big);
    if(diagEventRef == NULL)
    {
        LE_ERROR("Get diagEvent service");
        return NULL;
    }

    taf_diagEvent_EnableCondStateHandlerRef_t enableCondStateRef =
            taf_diagEvent_AddEnableCondStateHandler(diagEventRef,
            (taf_diagEvent_EnableCondStateHandlerFunc_t)enableCondStateChangeHandler, ctxPtr);

    if(enableCondStateRef == NULL)
    {
        LE_ERROR("Add Enable Condition state handler");
        return NULL;
    }

    le_sem_Post(semRef);

    le_event_RunLoop();

    return NULL;
}

//Diag DTC status thread
static void* diagDtcStatusTheadFunc(void* ctxPtr)
{
    taf_diagDTC_ServiceRef_t diagDtcRef = NULL;
    taf_diagDTC_ConnectService();

    //Get the diag DTC service
    diagDtcRef = taf_diagDTC_GetService(DTC_CODE_AB0000);
    if(diagDtcRef == NULL)
    {
        LE_ERROR("Get diag DTC service");
        return NULL;
    }

    dtcStatusRef = taf_diagDTC_AddStatusHandler(diagDtcRef,
            (taf_diagDTC_StatusHandlerFunc_t)dtcStatusChangeHandler, ctxPtr);

    if(dtcStatusRef == NULL)
    {
        LE_ERROR("Add DTC status handler");
        return NULL;
    }

    le_sem_Post(semRef);

    le_event_RunLoop();

    return NULL;
}

//Diag all DTC status thread
static void* diagAllDtcStatusTheadFunc(void* ctxPtr)
{
    taf_diagDTC_AllServiceRef_t diagAllDtcRef = NULL;
    taf_diagDTC_ConnectService();

    //Get the diag all DTC service
    diagAllDtcRef = taf_diagDTC_GetAllService();
    if(diagAllDtcRef == NULL)
    {
        LE_ERROR("Get diag all DTC service");
        return NULL;
    }

    allDtcStatusRef = taf_diagDTC_AddAllStatusHandler(diagAllDtcRef,
            (taf_diagDTC_AllStatusHandlerFunc_t)allDtcStatusChangeHandler, ctxPtr);

    if(allDtcStatusRef == NULL)
    {
        LE_ERROR("Add all DTC status handler");
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

    result = taf_diag_SetEnableCondition(Condition_tooBig, true);

    if(result != LE_OK)
    {
        LE_ERROR("Failed to set enable condition");
    }

    bool status = taf_diag_GetEnableConditionStatus(Condition_tooBig);
    LE_DEBUG("Enable condition status of %d is %d", Condition_tooBig, status);

    // Create event uds status change thread
    le_thread_Ref_t eventUdsStatusThreadRef = le_thread_Create("udsStatusTh",
            diagEventUdsStatusTheadFunc, NULL);

    le_thread_Start(eventUdsStatusThreadRef);
    le_sem_Wait(semRef);

    // Create enable condition state change thread
    le_thread_Ref_t enableCondStateThreadRef = le_thread_Create("enCondStateTh",
            enableCondStateTheadFunc, NULL);

    le_thread_Start(enableCondStateThreadRef);
    le_sem_Wait(semRef);

    // Create DTC status change thread
    le_thread_Ref_t dtcStatusThreadRef = le_thread_Create("dtcStatusTh",
            diagDtcStatusTheadFunc, NULL);

    le_thread_Start(dtcStatusThreadRef);
    le_sem_Wait(semRef);

    // Create all DTC status change thread
    le_thread_Ref_t allDtcStatusThreadRef = le_thread_Create("alldtcStatusTh",
            diagAllDtcStatusTheadFunc, NULL);

    le_thread_Start(allDtcStatusThreadRef);
    le_sem_Wait(semRef);

    changeEventStatus();

    LE_INFO("diagEventApp end");
}
