/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DTC_SERVER_HPP
#define TAF_DTC_SERVER_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "configuration.hpp"
using namespace std;

#define DEFAULT_DTC_SVC_REF_CNT 32
#define TAF_DTC_MAX_SESSION_REF 16
#define ALL_DTC_GROUP 0xFFFFFF
#define MAX_DTC_DATA_VALUE_LENGTH 256

#define TAF_DIAGDTC_DTC_DATA_LISTS_MAX_NUM 4
#define TAF_DIAGDTC_DTC_DATA_MAX_NUM 64

#define TAF_DTC_DATA_VALUE_TIPICAL_BYTES 64
#define TAF_DTC_DATA_VALUE_MAX_NUM 1
#define TAF_DTC_DATA_VALUE_TIPICAL_NUM 8

#define DEFAULT_ALL_DTC_SVC_REF_CNT 1
//--------------------------------------------------------------------------------------------------
/**
 * Diag DTC Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace telux
{
    namespace tafsvc
    {
        // used to send event when dtc status change
        typedef struct
        {
            taf_diagDTC_ServiceRef_t  svcRef;
            uint8_t  dtcStatus;
        } taf_diagDTC_Status_t;

        typedef struct
        {
            taf_diagDTC_AllServiceRef_t  svcRef;
            uint32_t dtcCode;
            uint8_t  dtcStatus;
        } taf_diagDTC_AllStatus_t;

        typedef struct
        {
            le_msg_SessionRef_t sessionRef;
            le_dls_Link_t       link;
        } taf_diagDTC_SessionRef_t;

        typedef struct
        {
            uint32_t dtcCode;
            le_event_Id_t dtcStatusEventId;// event for notification
            bool suppressionStatus;
            le_dls_List_t sessionRefList; // The list of clients
            taf_diagDTC_ServiceRef_t svcRef;
            le_dls_Link_t link;
        }taf_diagDTC_DtcCtx_t;

        typedef struct
        {
            le_event_Id_t allDtcStatusEventId;// event for notification
            le_dls_List_t sessionRefList; // The list of clients
            taf_diagDTC_AllServiceRef_t svcRef;
        }taf_diagDTC_AllDtcCtx_t;

        /*
        * The structure of DTC data list.
        */
        typedef struct
        {
            le_sls_List_t dtcDataList;
            le_sls_List_t safeRefList;
            le_sls_Link_t* currPtr;
        } taf_DtcDataList_t;

        /*
        * The structure of DTC data info.
        */
        typedef struct
        {
            uint32_t dtcCode;
            taf_diagDTC_DataType_t dataType;
            uint8_t recordNum;
            uint16_t dataId;
            uint16_t dataLen;
            uint8_t* dataValue;
        } taf_DtcDataInfo_t;

        /*
        * The structure of DTC data with link.
        */
        typedef struct
        {
            taf_DtcDataInfo_t info;
            le_sls_Link_t link;
        } taf_DtcData_t;

        /*
        * The struct of safe reference for DTC data.
        */
        typedef struct
        {
            void* safeRef;
            le_sls_Link_t link;
        } taf_DtcDataSafeRef_t;

        class taf_DTCSvr : public ITafSvc
        {
            public:
                taf_DTCSvr() {};
                ~taf_DTCSvr() {}
                void Init();
                static taf_DTCSvr& GetInstance();
                //For specified DTC
                taf_diagDTC_ServiceRef_t GetService(uint32_t dtcCode);
                le_result_t GetCode(taf_diagDTC_ServiceRef_t svcRef, uint32_t* dtcCodePtr);
                le_result_t ReadStatus(taf_diagDTC_ServiceRef_t svcRef, uint8_t* statusPtr);
                le_result_t SetActivationStatus(taf_diagDTC_ServiceRef_t svcRef,
                        taf_diagDTC_ActivationStatus_t status);
                le_result_t GetActivationStatus(taf_diagDTC_ServiceRef_t svcRef,
                        taf_diagDTC_ActivationStatus_t* statusPtr);
                le_result_t SetSuppression(taf_diagDTC_ServiceRef_t svcRef, bool suppressionStatus);
                le_result_t GetSuppression(taf_diagDTC_ServiceRef_t svcRef,
                        bool* suppressionStatusPtr);
                le_result_t ClearInfo(taf_diagDTC_ServiceRef_t svcRef);
                le_result_t RemoveSvc(taf_diagDTC_ServiceRef_t svcRef);
                le_event_Id_t GetDtcStatusEvent(taf_diagDTC_ServiceRef_t svcRef);

                taf_diagDTC_DataListRef_t GetDataList(taf_diagDTC_ServiceRef_t svcRef);
                taf_diagDTC_DataRef_t GetFirstData(taf_diagDTC_DataListRef_t dataListRef);
                taf_diagDTC_DataRef_t GetNextData(taf_diagDTC_DataListRef_t dataListRef);
                le_result_t DeleteDataList(taf_diagDTC_DataListRef_t dataListRef);
                le_result_t GetDataDtcCode(taf_diagDTC_DataRef_t dataRef, uint32_t* dtcCodePtr);
                le_result_t GetDataType(taf_diagDTC_DataRef_t dataRef,
                        taf_diagDTC_DataType_t* dataTypePtr);
                le_result_t GetRecordNumber(taf_diagDTC_DataRef_t dataRef,
                        uint8_t* recordNumberPtr);
                le_result_t GetDataId(taf_diagDTC_DataRef_t dataRef, uint16_t* dataIdPtr);
                le_result_t GetDataValue(taf_diagDTC_DataRef_t dataRef, uint8_t* valuePtr,
                        size_t* valueSizePtr);

                static void FirstLayerStatusHandler(void* reportPtr, void* secondLayerHandlerFunc);
                //Interface function for tafEvent module
                void ReportDTCStatus(uint32_t dtcCode, uint8_t dtcStatus);

                //For all DTC
                taf_diagDTC_AllServiceRef_t GetAllService();
                le_event_Id_t GetAllDtcStatusEvent(taf_diagDTC_AllServiceRef_t svcRef);
                le_result_t ClearAllInfo(taf_diagDTC_AllServiceRef_t svcRef);
                le_result_t SetAllSuppression(taf_diagDTC_AllServiceRef_t svcRef,
                        bool suppressionStatus);
                le_result_t RemoveAllSvc(taf_diagDTC_AllServiceRef_t svcRef);
                static void FirstLayerAllDtcStatusHandler(void* reportPtr,
                        void* secondLayerHandlerFunc);

            private:

                taf_diagDTC_DtcCtx_t* GetDtcCtxByCode(uint32_t dtcCode);
                le_result_t AddSessionToDtcCtx(taf_diagDTC_DtcCtx_t* dtcCtxPtr,
                        le_msg_SessionRef_t sessionRef);
                le_result_t RemoveSessionFromDtcCtx(taf_diagDTC_DtcCtx_t* dtcCtxPtr,
                        le_msg_SessionRef_t sessionRef);
                //For all DTC
                le_result_t AddSessionToAllDtcCtx(le_msg_SessionRef_t sessionRef);
                le_result_t RemoveSessionFromAllDtcCtx(le_msg_SessionRef_t sessionRef);
                void InitAllDtcCtx();
                void UpdateAllDtcSuppressionStatus(bool suppressionStatus);

                taf_diagDTC_DtcCtx_t* GetDtcCtx(taf_diagDTC_ServiceRef_t svcRef);
                void DtcConfiguration(cfg::Node & node);
                void InitDtcCtx(uint32_t dtcCode);
                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);

                le_ref_MapRef_t SvcRefMap;
                le_mem_PoolRef_t SessionRefPool = NULL;
                le_mem_PoolRef_t DtcStatusPool;
                le_mem_PoolRef_t DtcPool = NULL;
                le_dls_List_t DtcCtxList = LE_DLS_LIST_INIT;

                le_mem_PoolRef_t DtcDataListPool;
                le_mem_PoolRef_t DtcDataPool;
                le_mem_PoolRef_t DtcDataSafeRefPool;
                le_mem_PoolRef_t  DtcDataValuePool;

                le_ref_MapRef_t DtcDataListRefMap;
                le_ref_MapRef_t DtcDataSafeRefMap;
                //For all DTC
                le_mem_PoolRef_t AllDtcStatusPool;
                le_ref_MapRef_t AllSvcRefMap;
                taf_diagDTC_AllDtcCtx_t AllDtcCtx;

        };
    }
}
#endif /* #ifndef TAF_DTC_SERVER_HPP */
