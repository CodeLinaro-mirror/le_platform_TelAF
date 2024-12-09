/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_EVENT_SVR_HPP
#define TAF_EVENT_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "configuration.hpp"

using namespace std;

#define MAX_ENABLE_CONDITION_NUM 64
#define MAX_OPERATION_CYCLE_NUM 32
#define MAX_TEST_FAILURE_COUNTER 255
#define MAX_OCCURRENCE_COUNTER 255
#define DEFAULT_EVENT_SVC_REF_CNT 32
#define TAF_EVENT_MAX_SESSION_REF 8
#define MAX_FAULT_DETECTION_COUNTER 127
#define MIN_FAULT_DETECTION_COUNTER -128
#define MIDDLE_COUNTER_BASED_PARAM_VALUE 1
#define MAX_COUNTER_BASED_PARAM_VALUE 32768
#define MIN_COUNTER_BASED_PARAM_VALUE -32768
#define MIN_TIME_BASED_PARAM_VALUE 0.001
#define MAX_TIME_BASED_PARAM_VALUE 3600
#define FEATURE_A_PROGRAMMING_SESSION 0x2
#define FEATURE_A_FOTA_SESSION 0x42
//--------------------------------------------------------------------------------------------------
/**
 * Diag Event Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace telux
{
    namespace tafsvc
    {

        typedef enum
        {
            TAF_DIAGEVENT_DEBOUNCE_MONITOR_INTERNAL = 0x0,
            TAF_DIAGEVENT_DEBOUNCE_COUNTER_BASED = 0x01,
            TAF_DIAGEVENT_DEBOUNCE_TIME_BASED = 0x02
        }taf_diagEvent_DebounceType_t;

        typedef enum
        {
            TAF_DIAGEVENT_PROCESS_OCCCTR_TF = 0x0,
            TAF_DIAGEVENT_PROCESS_OCCCTR_CDTC = 0x01
        }taf_diagEvent_Process_Occur_Counter_Type_t;

        typedef enum
        {
            TAF_DIAGEVENT_TIMER_PREFAILED = 0x0,
            TAF_DIAGEVENT_TIMER_PREPASSED = 0x01,
            TAF_DIAGEVENT_TIMER_UNKNOWN = 0x02
        }taf_diagEvent_Running_Timer_Type_t;
#ifdef LE_CONFIG_DIAG_FEATURE_A
        typedef enum
        {
            TAF_DIAGEVENT_APPLICATION_DTC = 0x0,
            TAF_DIAGEVENT_REPROGRAMMING_DTC = 0x01,
            TAF_DIAGEVENT_UNKNOWN_DTC = 0x2
        }taf_diagEvent_DTC_Type_t;
#endif
        typedef struct
        {
            uint32_t    dtc;
            uint8_t     fdc;    // DTC Fault Detection Counter
            le_dls_Link_t link;
        }taf_DiagEvent_FDCInfo_t;

        typedef struct
        {
            uint16_t eventId;
            le_dls_Link_t link;
        }taf_DiagEvent_EventIdInfo_t;

        typedef struct
        {
            taf_diagEvent_ServiceRef_t  svcRef;
            uint8_t  eventUdsStatus;
        } taf_diagEvent_UdsStatus_t;

        typedef struct
        {
            taf_diagEvent_ServiceRef_t  svcRef;
            bool  state;
        } taf_diagEvent_EnableCondState_t;

        //Counterbased debounce config structure
        typedef struct {
            int16_t  decrementStepSize;
            int16_t  incrementStepSize;
            int16_t  failedThreshold;
            int16_t  passedThreshold;
            bool  jumpDown;
            bool  jumpUp;
            int16_t  jumpDownValue;
            int16_t  jumpUpValue;
            int16_t  fdcThreshold;
        } taf_diagEvent_DebounceCounterBasedConfig;

        //Timebased debounce config structure
        typedef struct {
            uint32_t  failedThreshold;
            uint32_t  passedThreshold;
            uint32_t  fdcThreshold;
        } taf_diagEvent_DebounceTimeBasedConfig;

        typedef struct
        {
            le_msg_SessionRef_t sessionRef;
            le_dls_Link_t       link;
        } taf_diagEvent_SessionRef_t;

        typedef struct
        {
            uint32_t dtcCode;
            uint8_t dtcStatus;
            taf_diagEvent_Process_Occur_Counter_Type_t occurrenceCounterProcessing;
            uint8_t occurrenceCounter;
            int16_t faultDetectionCounter;
            bool activationStatus;
            bool suppressionStatus;
#ifdef LE_CONFIG_DIAG_FEATURE_A
            taf_diagEvent_DTC_Type_t dtcType;
#endif
            le_dls_Link_t link;
            le_dls_List_t dtcEventIdList; // The list of event id in this DTC
        }taf_diagEvent_DtcCtx_t;

        typedef struct
        {
            uint16_t eventId;
            le_event_Id_t udsStatusEventId;// event for UDS status notification
            le_event_Id_t enableCondStateEventId;// event for Enable Condition state notification
            uint32_t dtcCode;
            taf_diagEvent_DtcCtx_t *dtcCtxPtr;
            uint8_t operationCycleId;// operation cycle id
            bool eventEnableStatus;
            uint8_t eventUdsStatus; // event UDS status
            uint8_t failureCounter;// failure counter/trip counter
            uint8_t confirmationThreshold; //Confirmation threshold
            uint8_t supplierFaultCode[TAF_DIAGEVENT_SUPPLIER_FAULT_CODE_MAX_LEN];
            size_t supplierFaultCodeSize;
            taf_diagEvent_StatusType_t eventFaultStatus;//PASSED,FAILED,UNKNOWN
            int16_t debounceCounter;// debounce counter
            bool fdcTriggerFlag; // FDC trigger flag
            taf_diagEvent_DebounceResetStatus_t debounceBehavior_; //Congigured and changed by API
            uint8_t debounceType;// debounce type
            taf_diagEvent_DebounceCounterBasedConfig debounceCounterBasedConfig;
            uint32_t timeBasedFdcThresholdStorageValue;
            taf_diagEvent_DebounceTimeBasedConfig debounceTimeBasedConfig;
            le_timer_Ref_t timerRef;
            le_timer_Ref_t fdcTimerRef;
            taf_diagEvent_Running_Timer_Type_t timerType;
            int64_t startTime;
            le_dls_List_t sessionRefList; // The list of clients
            taf_diagEvent_ServiceRef_t svcRef;
            le_dls_Link_t link;
        }taf_diagEvent_EventCtx_t;

        class taf_EventSvr : public ITafSvc
        {
            public:
                taf_EventSvr() {};
                ~taf_EventSvr() {}
                void Init();
                static taf_EventSvr& GetInstance();

                taf_diagEvent_ServiceRef_t GetService(uint16_t eventId);
                le_result_t GetId(taf_diagEvent_ServiceRef_t svcRef, uint16_t* eventIdPtr);

                le_event_Id_t GetUdsStatusEvent(taf_diagEvent_ServiceRef_t svcRef);
                le_event_Id_t GetEnableCondStateEvent(taf_diagEvent_ServiceRef_t svcRef);
                le_result_t SetStatus(taf_diagEvent_ServiceRef_t svcRef,
                        taf_diagEvent_StatusType_t eventStatus);
                le_result_t SetStatusWithSupplierFaultCode(taf_diagEvent_ServiceRef_t svcRef,
                        taf_diagEvent_StatusType_t eventStatus, const uint8_t* supplierFaultCodePtr,
                        size_t supplierFaultCodeSize);
#ifdef LE_CONFIG_DIAG_FEATURE_A
                le_result_t UpdateEventOnPassedCustomerN(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnFailedCustomerN(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnPrePassedCustomerN(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnPreFailedCustomerN(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnConfirmedCustomerN(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnTestNotCmpltCustomerN(
                        taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateDtcForCustomerN(taf_diagEvent_DtcCtx_t *dtcCtxPtr);
#endif
                le_result_t ResetDebounceStatus(taf_diagEvent_ServiceRef_t svcRef,
                        taf_diagEvent_DebounceResetStatus_t status);
                le_result_t GetUdsStatus(taf_diagEvent_ServiceRef_t svcRef,
                        uint8_t* eventUdsStatusPtr);
                le_result_t RemoveSvc(taf_diagEvent_ServiceRef_t svcRef);
                le_result_t SetEventEnableStatus(uint8_t enableConditionID);
                le_result_t SetOperationCycleState(uint8_t operationCycleId,
                        taf_diagEvent_OperationCycleState_t state);

                taf_diagEvent_EventCtx_t* GetEventCtxById(uint16_t eventId);
                static void FirstLayerUdsStatusHandler(void* reportPtr,
                        void* secondLayerHandlerFunc);
                static void FirstLayerEnableCondStateHandler(void* reportPtr,
                        void* secondLayerHandlerFunc);

                //Interface function for DTC interface module and DTC service module
                le_result_t ClearDtc(uint32_t dtcCode);
                le_result_t EnableDTCSetting(uint32_t dtcCode);
                le_result_t DisableDTCSetting(uint32_t dtcCode);
                //Interface function for DTC interface module
                void ReportDTCFaultDetectionCounter(le_dls_List_t* fdcInfoListPtr);
                //Interface function for DTC service module
                le_result_t SetDTCSuppression(uint32_t dtcCode, bool suppressionStatus);
                le_result_t SetAllDTCSuppression(bool suppressionStatus);
                le_result_t GetFaultDetectionCounter(uint32_t dtcCode,
                        uint8_t* faultDetectionCounterPtr);
                le_mem_PoolRef_t EventPool = NULL;
                le_dls_List_t EventCtxList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t DtcPool = NULL;
                le_dls_List_t DtcCtxList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t EventIdPool = NULL;
                le_dls_List_t fdcInfoList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t FdcInfoPool = NULL;
                le_thread_Ref_t mainThrRef = NULL; // Current main thread.

            private:

                taf_diagEvent_EventCtx_t* GetServiceObj( uint16_t eventId);
                void InitEventContext(const std::pair<uint32_t, std::shared_ptr<cfg::Node>>& dtc,
                        const std::pair<uint16_t, std::shared_ptr<cfg::Node>>& event);
                void EventConfiguration(cfg::Node & node);
                taf_diagEvent_DtcCtx_t* InitDtcContext(uint32_t dtcCode, uint16_t eventId);
                void InitEventIdListForDtc(taf_diagEvent_DtcCtx_t* dtcCtxPtr, uint16_t eventId);
                taf_diagEvent_DtcCtx_t* GetDtcCtxByCode(uint32_t dtcCode);

                bool IsEventConditionOK(taf_diagEvent_EventCtx_t* eventCtxPtr);
                taf_diagEvent_EventCtx_t* GetEventCtx(taf_diagEvent_ServiceRef_t svcRef);

                void ReportEventUdsStatus(taf_diagEvent_EventCtx_t* eventCtxPtr);
                void ReportEnableCondState(taf_diagEvent_EventCtx_t* eventCtxPtr, bool state);
                void ReportDtcStatus(taf_diagEvent_DtcCtx_t* dtcCtxPtr);
                le_result_t StoreAndReportDTCStatus(taf_diagEvent_DtcCtx_t* dtcCtxPtr);
                le_result_t StoreAndReportEventUdsStatus(taf_diagEvent_EventCtx_t* eventCtxPtr);

                le_result_t StartOperationCycle(uint8_t operationCycleId);
                le_result_t StopOperationCycle(uint8_t operationCycleId);

                le_result_t AddSessionToEventCtx(taf_diagEvent_EventCtx_t* eventCtxPtr,
                        le_msg_SessionRef_t sessionRef);
                le_result_t RemoveSessionFromEventCtx(taf_diagEvent_EventCtx_t* eventCtxPtr,
                        le_msg_SessionRef_t sessionRef);
                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);

                static void TimeBasedDebounceTimerHandler(le_timer_Ref_t  timerRef);
                static void FdcTimerHandler(le_timer_Ref_t  timerRef);
                le_result_t DebounceEvent(taf_diagEvent_EventCtx_t* eventCtxPtr,
                        taf_diagEvent_StatusType_t eventStatus);
                le_result_t DebounceCounterBased(taf_diagEvent_EventCtx_t* eventCtxPtr,
                        taf_diagEvent_StatusType_t eventStatus);
                le_result_t DebounceTimeBased(taf_diagEvent_EventCtx_t* eventCtxPtr,
                        taf_diagEvent_StatusType_t eventStatus);
                void ResetDebounceCounter(taf_diagEvent_EventCtx_t* eventCtxPtr);
                uint8_t CalcTimeBasedDebounceFDC(taf_diagEvent_EventCtx_t* eventCtxPtr);
                uint8_t CalcCounterBasedDebounceFDC(taf_diagEvent_EventCtx_t* eventCtxPtr);

                le_result_t UpdateEventOnFailed(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateEventOnPassed(taf_diagEvent_EventCtx_t* eventCtxPtr);
                le_result_t UpdateDtcOnFailed(taf_diagEvent_DtcCtx_t* dtcCtxPtr);
                le_result_t UpdateDtcOnPassed(taf_diagEvent_DtcCtx_t* dtcCtxPtr);

                le_result_t ClearSingleDtc(uint32_t dtcCode);
                le_result_t ClearAllDtc();
                static void ClearDTCAndEventData(void* param1Ptr, void* param2Ptr);

                void TriggerSnapshotData(uint8_t oldEventUdsStatus,
                        taf_diagEvent_EventCtx_t* eventCtxPtr);

                void UpdateAllDtcSuppressionStatus(bool suppressionStatus);

                le_ref_MapRef_t SvcRefMap;
                le_mem_PoolRef_t SessionRefPool = NULL;
                le_mem_PoolRef_t EventUdsStatusPool;
                le_mem_PoolRef_t EnableCondStatePool;
                taf_diagEvent_OperationCycleState_t OperationCycleStates[MAX_OPERATION_CYCLE_NUM];

        };
    }
}
#endif /* #ifndef TAF_EVENT_SVR_HPP */
