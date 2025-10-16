/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <time.h>
#include <memory>
#include <future>
#include <vector>
#include "tafImuSensor.hpp"

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tSensorInfoPool, TAF_SENSOR_POOL_SIZE, sizeof(taf_SensorInfo_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorListPool, TAF_SENSOR_LIST_POOL_SIZE, sizeof(taf_SensorInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorEventPool, TAF_SENSOR_MAX_EVENTS_SIZE,
    sizeof(taf_SensorEventList_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorEventInfoPool, TAF_SENSOR_MAX_EVENTS_SIZE,
    sizeof(taf_SensorEventInfo_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorEventHandlerPool, SENSOR_EVENT_HANDLER_HIGH ,
    sizeof(taf_SensorEventHandler_t));
LE_MEM_DEFINE_STATIC_POOL(ClientPoolRef,
   TAF_SENSOR_CLIENT_ACTIVATION_MAX,sizeof(taf_SensorClient_t));
LE_REF_DEFINE_STATIC_MAP(tSensorInfoMap, TAF_SENSOR_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tSensorListMap, TAF_SENSOR_LIST_POOL_SIZE);

void Handler::onSelfTestFailed(taf_pa_sensor_Ref_t reference,uint64_t timestamp,void *contextPtr){
    LE_INFO("onSelfTestFailed");
    le_msg_SessionRef_t sessionRef = static_cast<le_msg_SessionRef_t>(contextPtr);
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(sessionRef);
    if (NULL == clientRequestPtr) {
        LE_DEBUG("onEvent did not find sessionRef: %p", sessionRef);
        return;
    }
    for(std::shared_ptr<taf_sensorClientInfo_t> clientInfo: clientRequestPtr->clients){
        if(clientInfo->sensorClient == reference){
            le_mutex_Lock(clientInfo->mSensorMutexRef);
            if(sensorMngr.numOfSelfTestEventHandler){
                taf_SensorSelfTest_t event;
                event.timestamp = timestamp;
                event.cSensorRef = clientInfo->sensorRef;
                le_event_Report(clientInfo->SelfTestEventId,&event,sizeof(event));
            }
            else{
                LE_INFO("Self Test Handler not found");
            }
            le_mutex_Unlock(clientInfo->mSensorMutexRef);
        }
    }
}

void Handler::onEvent(taf_pa_sensor_Ref_t reference,taf_pa_sensor_event_t* events,int count,
    void *contextPtr)
{
    le_msg_SessionRef_t sessionRef = static_cast<le_msg_SessionRef_t>(contextPtr);
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(sessionRef);
    if (NULL == clientRequestPtr) {
        LE_DEBUG("onEvent did not find sessionRef: %p", sessionRef);
        delete[] events;
        return;
    }
    for(std::shared_ptr<taf_sensorClientInfo_t> clientInfo: clientRequestPtr->clients)
    {
        if(clientInfo->sensorClient == reference){
            le_mutex_Lock(clientInfo->mSensorMutexRef);
            if(sensorMngr.numofSensorEventHandlers){
                taf_SensorEventList_t* triggeredSensorEvent =
                    (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
                    for(int i=0;i<count;i++){
                        std::shared_ptr<taf_SensorEvent_t> eventData;
                        try{
                            eventData = std::make_shared<taf_SensorEvent_t>();
                        } catch(const std::exception &e){
                            LE_FATAL("Not able to intialize eventData with exception %s",e.what());
                        }
                        eventData->timestamp = events[i].timestamp;
                        eventData->x = events[i].x;
                        eventData->y = events[i].y;
                        eventData->z = events[i].z;
                        eventData->xb = events[i].xb;
                        eventData->yb = events[i].yb;
                        eventData->zb = events[i].zb;
                        triggeredSensorEvent->eventList.push_back(eventData);
                    }
                    triggeredSensorEvent->sensorRef = clientInfo->sensorRef;
                    triggeredSensorEvent->listSize = count;
                    triggeredSensorEvent->sessionRef = clientRequestPtr->sessionRef;
                    le_event_ReportWithRefCounting(clientInfo->SensorOnEventId,triggeredSensorEvent);
            }
            le_mutex_Unlock(clientInfo->mSensorMutexRef);
        }
    }
    delete[] events;
}

taf_Sensor &taf_Sensor::GetInstance()
{
    static taf_Sensor instance;
    return instance;
}

void ConvertSensorType(taf_SensorInfo_t* sensorInfoPtr,taf_pa_sensor_basicInfo_t info)
{
    taf_pa_sensor_type_t type = info.sensorType;
    switch(type){
        case taf_pa_sensor_type_t::TAF_PA_SENSOR_ACCELEROMETER:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_ACCELEROMETER;
        break;

        case taf_pa_sensor_type_t::TAF_PA_SENSOR_GYROSCOPE:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_GYROSCOPE;
        break;

        case taf_pa_sensor_type_t::TAF_PA_SENSOR_INVALID:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_INVALID;
        break;
    }
}

le_result_t taf_Sensor::InitializeSensorClientList(taf_SensorClient_t* clientRequestPtr){
    LE_INFO("InitializeSensorList");
    auto& sensorMngr = taf_Sensor::GetInstance();
    for(size_t i=0;i<sensorMngr.sList.size();i++){
        std::shared_ptr<taf_sensorClientInfo_t> clientInfo =
        std::make_shared<taf_sensorClientInfo_t>();
        clientInfo->isSensorActivated = false;
        clientInfo->sensorClient =
            taf_pa_sensor_CreateReference(sensorMngr.sList[i].basicInfo.sensorName);
        if(clientInfo->sensorClient == NULL){
            LE_ERROR("unable to create Reference for %s",sensorMngr.sList[i].basicInfo.sensorName);
        }
        clientInfo->eventListener.onEvent = &Handler::onEvent;
        clientInfo->eventListener.onSelfTestFailed = &Handler::onSelfTestFailed;
        char mutexName[NAME_MAX_SIZE];
        snprintf(mutexName,sizeof(mutexName),"%sMutexCl",sensorMngr.sList[i].basicInfo.sensorName);
        clientInfo->mSensorMutexRef = le_mutex_CreateRecursive(mutexName);
        char onEventName[NAME_MAX_SIZE];
        snprintf(onEventName,sizeof(onEventName),"%sOnEventId",sensorMngr.sList[i].basicInfo.sensorName);
        clientInfo->SensorOnEventId = le_event_CreateIdWithRefCounting(onEventName);
        char selfTestEventName[NAME_MAX_SIZE];
        snprintf(selfTestEventName,sizeof(selfTestEventName),"%sselfTestEventId",
            sensorMngr.sList[i].basicInfo.sensorName);
        clientInfo->SelfTestEventId =
            le_event_CreateId(selfTestEventName,sizeof(taf_SensorSelfTest_t));
        char handlerName[NAME_MAX_SIZE];
        snprintf(handlerName,sizeof(handlerName),"%sHandlerRef",sensorMngr.sList[i].basicInfo.sensorName);
        clientInfo->HandlerRef = le_event_AddHandler(handlerName,
            clientInfo->SensorOnEventId, taf_Sensor::DataEventHandler);
        le_utf8_Copy(clientInfo->sensorName, sensorMngr.sList[i].basicInfo.sensorName,NAME_MAX_SIZE , NULL);
        clientRequestPtr->clients.push_back(clientInfo);
    }
    return LE_OK;
}

void CopyLastEvent(taf_SensorEventList_t* LastEventPtr,taf_SensorEventList_t* currentEventPtr){
    LastEventPtr->sensorRef = currentEventPtr->sensorRef;
    LastEventPtr->sessionRef = currentEventPtr->sessionRef;
    LastEventPtr->listSize = currentEventPtr->listSize;
    LastEventPtr->eventList.clear();
    for(size_t i=0;i<currentEventPtr->eventList.size();i++){
        std::shared_ptr<taf_SensorEvent_t> eventData;
        try{
            eventData = std::make_shared<taf_SensorEvent_t>();
        } catch(const std::exception &e){
            LE_FATAL("Not able to intialize eventData with exception %s",e.what());
        }
        eventData->timestamp = currentEventPtr->eventList[i]->timestamp;
        eventData->x = currentEventPtr->eventList[i]->x;
        eventData->y = currentEventPtr->eventList[i]->y;
        eventData->z= currentEventPtr->eventList[i]->z;
        eventData->xb = currentEventPtr->eventList[i]->xb;
        eventData->yb = currentEventPtr->eventList[i]->yb;
        eventData->zb = currentEventPtr->eventList[i]->zb;
        LastEventPtr->eventList.push_back(eventData);
    }
}

void taf_Sensor::DataEventHandler(void* reportPtr){
    taf_SensorEventHandler_t* evtHandlerPtr;
    taf_SensorEventInfo_t* eventInfo = NULL;
    taf_SensorEventList_t* currentEventList = (taf_SensorEventList_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL( currentEventList == NULL, "currentPosPtr is Null");
    taf_SensorClient_t* clientRequestPtr = NULL;
    auto& sensorMngr = taf_Sensor::GetInstance();
    clientRequestPtr = sensorMngr.DiscoverSessionRef(currentEventList->sessionRef);
    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "EventHandler did not find sessionRef");
    if(!sensorMngr.numofSensorEventHandlers)
    {
        LE_DEBUG("No Event handlers, exit Handler Function");
        currentEventList->eventList.clear();
        le_mem_Release(currentEventList);
        currentEventList = NULL;
        return;
    }
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorEventHandlerMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        evtHandlerPtr = (taf_SensorEventHandler_t*)le_ref_GetValue(iterRef);
        if(evtHandlerPtr == NULL) {
            LE_DEBUG("evtHandlerPtr NULL");
            return;
        }
        if(evtHandlerPtr->sessionRef == currentEventList->sessionRef &&
            evtHandlerPtr->sensorRef == currentEventList->sensorRef){
            LE_DEBUG("SessionRef and sensorRef Found");
            eventInfo = (taf_SensorEventInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventInfoPool);
            memset(eventInfo, 0, sizeof(taf_SensorEventInfo_t));
            eventInfo->eventPtr = (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
            CopyLastEvent(eventInfo->eventPtr,currentEventList);
            eventInfo->sessionRef = evtHandlerPtr->sessionRef;
            eventInfo->ref =
            (taf_imuSensor_SampleRef_t)le_ref_CreateRef(sensorMngr.tSensorEventMap,eventInfo);
            evtHandlerPtr->handlerFuncPtr(currentEventList->sensorRef,
            eventInfo->ref,evtHandlerPtr->handlerContextPtr);
            LE_DEBUG("Data reported with ref %p",eventInfo->ref);
        }
        else{
            LE_DEBUG("DataEventHandler sensor ref or session ref didnt match ");
        }
    }
    currentEventList->eventList.clear();
    le_mem_Release(currentEventList);
    currentEventList = NULL;
}

le_result_t taf_Sensor::SetEulerAngle(double pitch ,double roll , double yaw)
{
    LE_DEBUG("Set Euler Angle");
    if(taf_pa_sensor_SetEulerAngle(NULL,pitch,roll,yaw) != LE_OK){
        LE_INFO("Not able to set euler angle for client");
        return LE_FAULT;
    }
    return LE_OK;
}

taf_imuSensor_SensorListRef_t taf_Sensor::GetAvailableSensors()
{
    LE_DEBUG("Get Available Sensors");
    auto& sensorMngr = taf_Sensor::GetInstance();
      taf_SensorInfoList_t* sensorListPtr =
        (taf_SensorInfoList_t*)le_mem_ForceAlloc(sensorMngr.tSensorListPool);
    memset(sensorListPtr,0,sizeof(taf_SensorInfoList_t));
    sensorListPtr->SensorsList = LE_SLS_LIST_INIT;
    sensorListPtr->currPtr = NULL;
    sensorListPtr->sessionRef  = taf_imuSensor_GetClientSessionRef();
    sensorListPtr->sensorListSize = sList.size();
    if(sList.size()>0){
        taf_SensorInfo_t* sensorInfoPtr;
        for(size_t i=0;i<sList.size();i++){
            sensorInfoPtr = (taf_SensorInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorInfoPool);
            memset(sensorInfoPtr,0,sizeof(taf_SensorInfoList_t));
            sensorInfoPtr->currPtr = NULL;
            sensorInfoPtr->id  = sList[i].basicInfo.id;
            le_utf8_Copy(sensorInfoPtr->name, sList[i].basicInfo.sensorName,
                NAME_MAX_SIZE , NULL);
            le_utf8_Copy(sensorInfoPtr->vendor, sList[i].basicInfo.vendorName,
                NAME_MAX_SIZE , NULL);
            for(size_t j=0;j<sList[i].configInfo.sampleRateListSize;j++){
                sensorInfoPtr->samplingRate[j] = sList[i].configInfo.samplingRate[j];
            }
            ConvertSensorType(sensorInfoPtr,sList[i].basicInfo);
            sensorInfoPtr->sampleRateListSize = sList[i].configInfo.sampleRateListSize;
            sensorInfoPtr->maxSamplingRate = sList[i].configInfo.maxSamplingRate;
            sensorInfoPtr->maxBatchCountSupported = sList[i].configInfo.maxBatchCount;
            sensorInfoPtr->minBatchCountSupported = sList[i].configInfo.minBatchCount;
            sensorInfoPtr->range = sList[i].capInfo.range;
            sensorInfoPtr->version = sList[i].basicInfo.version;
            sensorInfoPtr->resolution = sList[i].capInfo.resolution;
            sensorInfoPtr->maxRange = sList[i].capInfo.maxRange;
            sensorInfoPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(sensorListPtr->SensorsList), &(sensorInfoPtr->link));
            sensorInfoPtr->ref =
              (taf_imuSensor_SensorRef_t)le_ref_CreateRef(sensorMngr.tSensorInfoMap, sensorInfoPtr);
        }
        sensorListPtr->ref = (taf_imuSensor_SensorListRef_t)
                        le_ref_CreateRef(sensorMngr.tSensorListMap,sensorListPtr);
        return sensorListPtr->ref;
    }
    return NULL;
}

le_result_t taf_Sensor::DeleteSensorList(taf_imuSensor_SensorListRef_t sensorListRef)
{
    TAF_ERROR_IF_RET_VAL(sensorListRef == NULL,LE_BAD_PARAMETER,"Null reference(sensorListRef)");
    taf_SensorInfoList_t* listPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap,sensorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteMachineList : %p",sensorListRef);
    taf_SensorInfo_t* tSensorPtr;
    le_sls_Link_t* tSensorLinkPtr;
    while ((tSensorLinkPtr = le_sls_Pop(&(listPtr->SensorsList))) != NULL){
        tSensorPtr = CONTAINER_OF(tSensorLinkPtr, taf_SensorInfo_t, link);
        le_ref_DeleteRef(tSensorInfoMap, tSensorPtr->ref);
        le_mem_Release(tSensorPtr);
    }
    le_ref_DeleteRef(tSensorListMap, sensorListRef);
    le_mem_Release(listPtr);
    return LE_OK;
}

taf_imuSensor_SensorRef_t taf_Sensor::GetFirstSensor(taf_imuSensor_SensorListRef_t sensorListRef)
{
    taf_SensorInfoList_t* sensorListPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap, sensorListRef);
    TAF_ERROR_IF_RET_VAL(sensorListPtr == NULL, NULL,
            "Invalid reference (%p) provided!",sensorListPtr);
    le_sls_Link_t* tSensorlinkPtr = le_sls_Peek(&(sensorListPtr->SensorsList));
    if (tSensorlinkPtr != NULL)
    {
        taf_SensorInfo_t* tSensorPtr = CONTAINER_OF(tSensorlinkPtr, taf_SensorInfo_t, link);
        sensorListPtr->currPtr = tSensorlinkPtr;
        return tSensorPtr->ref;
    }
    return NULL;
}

taf_imuSensor_SensorRef_t taf_Sensor::GetNextSensor(taf_imuSensor_SensorListRef_t sensorListRef)
{
    taf_SensorInfoList_t* sensorListPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap, sensorListRef);
    TAF_ERROR_IF_RET_VAL(sensorListPtr == NULL, NULL,
            "Invalid reference (%p) provided!",sensorListPtr);
    le_sls_Link_t* tSensorlinkPtr =
        le_sls_PeekNext(&(sensorListPtr->SensorsList),sensorListPtr->currPtr);
    if (tSensorlinkPtr != NULL)
    {
        taf_SensorInfo_t* tSensorPtr = CONTAINER_OF(tSensorlinkPtr, taf_SensorInfo_t, link);
        sensorListPtr->currPtr = tSensorlinkPtr;
        return tSensorPtr->ref;
    }
    return NULL;
}

le_result_t taf_Sensor::GetSensorId(taf_imuSensor_SensorRef_t sensorRef,uint32_t* sensorIdPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    *sensorIdPtr = sensorPtr->id;
    TAF_ERROR_IF_RET_VAL(*sensorIdPtr == 0, LE_FAULT, "invalid id for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorName(taf_imuSensor_SensorRef_t sensorRef,char* sensorName,
    size_t sensorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    TAF_ERROR_IF_RET_VAL(sensorName == NULL, LE_FAULT, "Cannot write data to sensorName Ptr");
    snprintf(sensorName, sizeof(sensorPtr->name), "%s", sensorPtr->name);
    TAF_ERROR_IF_RET_VAL(sensorName == NULL, LE_FAULT, "invalid Name for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVendorName(taf_imuSensor_SensorRef_t sensorRef,
    char* sensorVendorName,size_t sensorVendorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    TAF_ERROR_IF_RET_VAL(sensorVendorName == NULL, LE_FAULT,
        "Cannot write data to sensorVendorName Ptr");
    snprintf(sensorVendorName, sizeof(sensorPtr->vendor), "%s", sensorPtr->vendor);
    TAF_ERROR_IF_RET_VAL(sensorVendorName == NULL, LE_FAULT, "invalid vendor for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVersion(taf_imuSensor_SensorRef_t sensorRef,char* version,
    size_t versionSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    snprintf(version,versionSize,"%d",sensorPtr->version);
    TAF_ERROR_IF_RET_VAL(version == NULL, LE_FAULT,"invalid sensorVersionPtr for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorType(taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_SensorType_t* sensorTypePtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    *sensorTypePtr = sensorPtr->sensorType;
    TAF_ERROR_IF_RET_VAL(*sensorTypePtr == 0, LE_FAULT, "invalid type for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorSamplingRateInfo(taf_imuSensor_SensorRef_t sensorRef,
    double* samplingRatesListPtr, size_t* samplingRatesListSizePtr)
{
     taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    uint32_t* samplingRateListSizePtr = &(sensorPtr->sampleRateListSize);
    TAF_ERROR_IF_RET_VAL(*samplingRateListSizePtr == 0, LE_FAULT,
        "invalid sampleRateListSizePtr for sensor");
    size_t j=0;
    for(size_t i=0;i<*samplingRateListSizePtr && i<*samplingRatesListSizePtr; i++){
        if(sensorPtr->samplingRate[i] <= sensorPtr->maxSamplingRate){
            j++;
            samplingRatesListPtr[i] = sensorPtr->samplingRate[i];
        }
    }
    *samplingRatesListSizePtr = j;
    TAF_ERROR_IF_RET_VAL(*samplingRateListSizePtr == 0, LE_FAULT,
        "invalid sensorMaxRangePtrfor sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorBatchingInfo(taf_imuSensor_SensorRef_t sensorRef,
    uint32_t* maxBatchCountSupportedPtr,uint32_t* minBatchCountSupportedPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    *maxBatchCountSupportedPtr = sensorPtr->maxBatchCountSupported;
    TAF_ERROR_IF_RET_VAL(*maxBatchCountSupportedPtr == 0, LE_FAULT,
        "invalid sensor maxBatchCountSupportedPtr for sensor");

    *minBatchCountSupportedPtr = sensorPtr->minBatchCountSupported;
    TAF_ERROR_IF_RET_VAL(*minBatchCountSupportedPtr == 0, LE_FAULT,
    "invalid sensor maxBatchCountSupportedPtr for sensor");

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorRangeInfo(taf_imuSensor_SensorRef_t sensorRef, double* rangePtr)
{

    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    *rangePtr = sensorPtr->range;
    TAF_ERROR_IF_RET_VAL(*rangePtr == 0, LE_FAULT,"invalid sensor RangePtr for sensor");

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorResolution(taf_imuSensor_SensorRef_t sensorRef,
    double* sensorResolutionPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);

    *sensorResolutionPtr = sensorPtr->resolution;
    TAF_ERROR_IF_RET_VAL(*sensorResolutionPtr== 0, LE_FAULT,
        "invalid sensorResolutionPtr for sensor");
    return LE_OK;
}

inline bool isValidInput(taf_SensorInfo_t* sensorPtr,double samplingRate , uint32_t batchCount){
    bool isValidSample = 0;
    for(double sRate: sensorPtr->samplingRate){
        if(sRate == samplingRate) isValidSample = 1;
    }
    if(!isValidSample){
        LE_DEBUG("Sampling Rate Not Supported");
        return false;
    }
    if(batchCount>sensorPtr->maxBatchCountSupported ||
        batchCount<sensorPtr->minBatchCountSupported || batchCount%10 != 0){
        LE_DEBUG("Batch Count Not Supported");
        return false;
    }
    return true;
}

le_result_t taf_Sensor::Activate(taf_imuSensor_SensorRef_t sensorRef,double samplingRate ,
    uint32_t batchCount){
    LE_DEBUG("Activate Sensor");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,LE_FAULT, "clientRequestPtr is NULL");
    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);
    for(std::shared_ptr<taf_sensorClientInfo_t> clientInfoPtr: clientRequestPtr->clients){
        if(strcmp(sensorPtr->name,clientInfoPtr->sensorName)==0){
            if(clientInfoPtr->isSensorActivated){
                LE_DEBUG("Already Sensor is Active");
                return LE_UNAVAILABLE;
            }
            if(!isValidInput(sensorPtr,samplingRate,batchCount)){
                return LE_UNSUPPORTED;
            }
            clientInfoPtr->sensorRef = sensorRef;
            if(taf_pa_sensor_RegisterListener(clientInfoPtr->sensorClient,
                &clientInfoPtr->eventListener,(void*)clientRequestPtr->sessionRef) !=  LE_OK){
               LE_DEBUG("Listener register failed for %s",
                    sensorPtr->name);
                return LE_FAULT;
            }
            LE_INFO("Register Listener for %s",clientInfoPtr->sensorName);
            if(taf_pa_sensor_Activate(clientInfoPtr->sensorClient,samplingRate,batchCount,1)
                != LE_OK){
                LE_DEBUG("Sensor activation failed for %s",sensorPtr->name);
                return LE_FAULT;
            }
            LE_INFO("Activate sensor for %s",clientInfoPtr->sensorName);
            clientInfoPtr->isSensorActivated = true;
        }
    }
    return LE_OK;
}

le_result_t taf_Sensor::SelfTest(taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_SelfTestMode_t mode,uint64_t* timestamp){
    LE_DEBUG("Self Test");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,LE_FAULT, "clientRequestPtr is NULL");
    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);
    taf_pa_sensor_testmode_t type;
    if(mode == TAF_IMUSENSOR_POSITIVE){
        type  = taf_pa_sensor_testmode_t::TAF_PA_SENSOR_POSITIVE;
    }
    else if(mode == TAF_IMUSENSOR_NEGATIVE){
        type  = taf_pa_sensor_testmode_t::TAF_PA_SENSOR_NEGATIVE;
    }
    else if(mode == TAF_IMUSENSOR_BOTH){
        type = taf_pa_sensor_testmode_t::TAF_PA_SENSOR_BOTH;
    }
    else{
        return LE_BAD_PARAMETER;
    }
    typedef struct{
        le_result_t result;
        uint64_t timestamp;
    }taf_SelfTestResult_t;
    auto cb1 = [](taf_pa_sensor_Ref_t ref,le_result_t result,
        uint64_t timestamp_,void *contextPtr) {
            taf_SelfTestResult_t* resPtr = (taf_SelfTestResult_t*)contextPtr;
            resPtr->result = result;
            if(result == LE_OK || result == LE_BUSY) {
                resPtr->timestamp = timestamp_;
            }
    };

    le_result_t res;
    for(std::shared_ptr<taf_sensorClientInfo_t> clientInfoPtr: clientRequestPtr->clients){
        if(strcmp(sensorPtr->name,clientInfoPtr->sensorName)==0){
            taf_SelfTestResult_t resCallback = {};
            res = taf_pa_sensor_SelfTest(clientInfoPtr->sensorClient,type,cb1,&resCallback);
            if(res == LE_OK){
                *timestamp = resCallback.timestamp;
                res = resCallback.result;
            }
        }
    }
    return res;
}

void taf_Sensor::FirstLayerSelfTestHandler(void* reportPtr,void* secondLayerHandlerFunc){
    taf_SensorSelfTest_t* ptr = (taf_SensorSelfTest_t*) reportPtr;
    TAF_ERROR_IF_RET_NIL(ptr == NULL,"ptr is NULL");
    taf_imuSensor_SelfTestFailedHandlerFunc_t clientHandlerFunc =
        (taf_imuSensor_SelfTestFailedHandlerFunc_t)secondLayerHandlerFunc;
    clientHandlerFunc(NULL,ptr->cSensorRef,ptr->timestamp,le_event_GetContextPtr());
}

taf_imuSensor_SelfTestFailedHandlerRef_t taf_Sensor::AddSelfTestFailedHandler
    (taf_imuSensor_SensorRef_t sensorRef,taf_imuSensor_SelfTestFailedHandlerFunc_t handlerPtr,
    void* contextPtr){
    LE_DEBUG("AddSelfTestFailedHandler");
    le_event_HandlerRef_t handlerRef = NULL;
    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL(NULL == clientRequestPtr, NULL,"AddSelfTestFailedHandler: clientRequestPtr"
        " is NULL");
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,NULL,"Invalid reference (%p) provided!",sensorPtr);
    for(std::shared_ptr<taf_sensorClientInfo_t> clientPtr: clientRequestPtr->clients){
        if(strcmp(clientPtr->sensorName,sensorPtr->name)==0){
            handlerRef = le_event_AddLayeredHandler("SelfTestHandler",
                clientPtr->SelfTestEventId,FirstLayerSelfTestHandler, (void*)handlerPtr);
                numOfSelfTestEventHandler++;
                le_event_SetContextPtr(handlerRef,contextPtr);
        }
    }
    return (taf_imuSensor_SelfTestFailedHandlerRef_t)handlerRef;
}

void taf_Sensor::RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if(numOfSelfTestEventHandler>0){
        numOfSelfTestEventHandler--;
    }
}

le_result_t taf_Sensor::Deactivate(taf_imuSensor_SensorRef_t sensorRef){
    LE_DEBUG("Deactivate Sensor");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,LE_FAULT, "clientRequestPtr is NULL");
    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);
    for(std::shared_ptr<taf_sensorClientInfo_t> clientInfoPtr: clientRequestPtr->clients){
        if(strcmp(sensorPtr->name,clientInfoPtr->sensorName)==0){
            if(clientInfoPtr->isSensorActivated == false){
                LE_ERROR("Sensor Already deactivated");
                return LE_UNAVAILABLE;
            }
            if(taf_pa_sensor_Deactivate(clientInfoPtr->sensorClient) != LE_OK){
                LE_DEBUG("Sensor activation failed for %s",sensorPtr->name);
                return LE_FAULT;
            }
            clientInfoPtr->isSensorActivated = false;
        }
    }
    return LE_OK;
}

taf_imuSensor_DataHandlerRef_t taf_Sensor::AddDataHandler(taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_DataHandlerFunc_t handlerPtr,void* contextPtr){
    LE_DEBUG("AddDataHandler");
    taf_SensorEventHandler_t*  eventHandlerPtr =
        (taf_SensorEventHandler_t*)le_mem_ForceAlloc(tSensorEventHandlerPool);
    memset(eventHandlerPtr, 0, sizeof(taf_SensorEventHandler_t));
    eventHandlerPtr->sensorRef = sensorRef;
    eventHandlerPtr->next = LE_DLS_LINK_INIT;
    eventHandlerPtr->handlerFuncPtr = handlerPtr;
    eventHandlerPtr->handlerContextPtr = contextPtr;
    eventHandlerPtr->sessionRef = taf_imuSensor_GetClientSessionRef();
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");
    eventHandlerPtr->handlerRef =
        (taf_imuSensor_DataHandlerRef_t)le_ref_CreateRef(tSensorEventHandlerMap,eventHandlerPtr);
    numofSensorEventHandlers++;
    return eventHandlerPtr->handlerRef;
}

void taf_Sensor::RemoveDataHandler(taf_imuSensor_DataHandlerRef_t handlerRef){
    taf_SensorEventHandler_t* evtHandlerPtr =
        (taf_SensorEventHandler_t*)le_ref_Lookup(tSensorEventHandlerMap,handlerRef);
    if(evtHandlerPtr != NULL){
        if(evtHandlerPtr->handlerRef != handlerRef){
            LE_INFO("Handler ref are not same");
            return ;
        }
        numofSensorEventHandlers--;
        LE_DEBUG("Removed evtHandlerRef(%p) for evtHandlerPtr(%p) (totalCnt=0x%x).",
            evtHandlerPtr->handlerRef, evtHandlerPtr, numofSensorEventHandlers);
        le_ref_DeleteRef(tSensorEventHandlerMap,handlerRef);
        le_mem_Release(evtHandlerPtr);
    }
    else{
        LE_INFO("eventHandlerPtr is null");
    }
}

le_result_t taf_Sensor::GetData( taf_imuSensor_SampleRef_t eventList,taf_imuSensor_DataValue_t*
    RawData,size_t* RawDataSizePtr,taf_imuSensor_DataValue_t*BiasData,size_t* BiasDataSizePtr){
    LE_DEBUG("GetData of list with ref %p",eventList);
    taf_SensorEventInfo_t* ptr =
    (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventList);
    TAF_ERROR_IF_RET_VAL(ptr == NULL || ptr->eventPtr == NULL, LE_NOT_FOUND,
        "Invalid reference (%p) provided!", ptr);
    if(*RawDataSizePtr < ptr->eventPtr->listSize || *BiasDataSizePtr < ptr->eventPtr->listSize){
        LE_ERROR("Output array size is less then total no of events");
        return LE_BAD_PARAMETER;
    }
    if(*RawDataSizePtr != *BiasDataSizePtr){
        LE_ERROR("Bias Size ptr not same as raw size ptr.");
        return LE_BAD_PARAMETER;
    }
    size_t j = 0;
    for(uint32_t i=0;i<ptr->eventPtr->eventList.size();i++){
        RawData[j].timestamp = ptr->eventPtr->eventList[i]->timestamp;
        RawData[j].x = ptr->eventPtr->eventList[i]->x;
        RawData[j].y = ptr->eventPtr->eventList[i]->y;
        RawData[j].z = ptr->eventPtr->eventList[i]->z;
        BiasData[j].timestamp = ptr->eventPtr->eventList[i]->timestamp;
        BiasData[j].x = ptr->eventPtr->eventList[i]->xb;
        BiasData[j].y = ptr->eventPtr->eventList[i]->yb;
        BiasData[j].z = ptr->eventPtr->eventList[i]->zb;
        j++;
    }
    *RawDataSizePtr = j;
    *BiasDataSizePtr = j;
    return LE_OK;
}

le_result_t taf_Sensor::DeleteData(taf_imuSensor_SampleRef_t eventListRef){
    LE_DEBUG("DeleteData of list with ref %p",eventListRef);
    TAF_ERROR_IF_RET_VAL(eventListRef == NULL,LE_BAD_PARAMETER,"Null reference(eventListRef)");
    taf_SensorEventInfo_t* listPtr =
        (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
    listPtr->eventPtr->eventList.clear();
    le_ref_DeleteRef(tSensorEventMap, eventListRef);
    le_mem_Release(listPtr->eventPtr);
    le_mem_Release(listPtr);
    return LE_OK;
}

taf_SensorClient_t* taf_Sensor::DiscoverSessionRef(le_msg_SessionRef_t sessionRef)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_SensorClient_t* clientPtr = (taf_SensorClient_t*) le_ref_GetValue(iterRef);
        if(clientPtr == NULL) {
            return NULL;
        }

        LE_DEBUG("clientPtr %p, clientPtr->sessionRef %p, sessionRef %p",
                 clientPtr, clientPtr->sessionRef, sessionRef);

        if (sessionRef == clientPtr->sessionRef)
        {
             LE_DEBUG("sessionRef %p found in Client session", sessionRef);
             return clientPtr;
        }
        result = le_ref_NextNode(iterRef);
    }
    return NULL;
}

taf_SensorClient_t* taf_Sensor::AcquireSessionRef(void)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_imuSensor_GetClientSessionRef();
    clientRequestPtr = DiscoverSessionRef(sessionRef);
    if (clientRequestPtr == NULL && sensorMngr.mClientRefCount<TAF_SENSOR_CLIENT_ACTIVATION_MAX)
    {
        clientRequestPtr = (taf_SensorClient_t*)le_mem_ForceAlloc(sensorMngr.ClientPoolRef);
        clientRequestPtr->sessionRef = sessionRef;
        InitializeSensorClientList(clientRequestPtr);
        void* reqRefPtr = le_ref_CreateRef(sensorMngr.ClientRequestRefMap, clientRequestPtr);
        if (sessionRef!=NULL) {
            //External client increase Client ref count
            sensorMngr.mClientRefCount++;
        }
        LE_INFO("SessionRef %p was not found, Create Client session, total count %d",
            sessionRef, sensorMngr.mClientRefCount);
        LE_DEBUG("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);
        clientRequestPtr->clientRefPtr = reqRefPtr;
    }
    return clientRequestPtr;
}

void taf_Sensor::CleanUp(taf_SensorClient_t* clientPtr){
    if(clientPtr == NULL) {
        LE_ERROR("ClientPtr Null");
        return;
    }
    auto &sensorMngr = taf_Sensor::GetInstance();
    for(size_t i=0;i<sensorMngr.sList.size();i++){
        if(clientPtr->clients[i]->isSensorActivated == true){
            if(taf_pa_sensor_Deactivate(clientPtr->clients[i]->sensorClient) != LE_OK){
                LE_ERROR("Unable to deactivate sensor");
            }
            clientPtr->clients[i]->isSensorActivated = false;
        }
        if(taf_pa_sensor_DeleteReference(clientPtr->clients[i]->sensorClient) != LE_OK){
            LE_ERROR("Unable to delete reference");
        }
    }
    clientPtr->clients.clear();
}

void taf_Sensor::ReleaseClientRef(void* RefPtr)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    void* clientPtr = le_ref_Lookup(sensorMngr.ClientRequestRefMap, RefPtr);
    if (NULL == clientPtr)
    {
        LE_ERROR("Invalid sensor service activation reference %p", RefPtr);
    }
    else
    {
        le_ref_DeleteRef(sensorMngr.ClientRequestRefMap, RefPtr);
        LE_DEBUG("Remove Client Ctrl (%p)",RefPtr);
        le_mem_Release(clientPtr);
    }
}

void taf_Sensor::CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorListMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorInfoList_t* listPtr =
                (taf_SensorInfoList_t*)le_ref_GetValue(iterRef);
        if (listPtr && listPtr->sessionRef == sessionRef)
        {
            taf_imuSensor_DeleteSensorList(listPtr->ref);
        }
    }

    iterRef = le_ref_GetIterator(sensorMngr.tSensorEventMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorEventInfo_t* dataEventPtr =
                (taf_SensorEventInfo_t*)le_ref_GetValue(iterRef);
        if(dataEventPtr ==NULL || dataEventPtr->eventPtr == NULL){
            LE_DEBUG("Unable to dereference the ptr");
            return;
        }
        if(dataEventPtr->sessionRef == sessionRef){
            taf_imuSensor_SampleRef_t  safeRef =
                (taf_imuSensor_SampleRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_imuSensor_DeleteData 0x%p, Session 0x%p", safeRef, sessionRef);
            dataEventPtr->eventPtr->eventList.clear();
            le_ref_DeleteRef(sensorMngr.tSensorEventMap, safeRef);
            le_mem_Release(dataEventPtr->eventPtr);
            le_mem_Release(dataEventPtr);
        }
    }

    iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorClient_t* sensorClientPtr = (taf_SensorClient_t*) le_ref_GetValue(iterRef);
        if (sensorClientPtr && sessionRef == sensorClientPtr->sessionRef)
        {
            if (sensorMngr.mClientRefCount > 0){
                //External Client release, decrement client ref count
                sensorMngr.mClientRefCount--;
            }
            sensorMngr.CleanUp(sensorClientPtr);
            void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_imuSensor_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, sensorClientPtr->sessionRef);

            sensorMngr.ReleaseClientRef(safeRefPtr);
        }
    }
}

void taf_Sensor::OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr)
{
    LE_INFO("OnClientConnection sessionRef: %p", sessionRef);
    auto& sensorMngr = taf_Sensor::GetInstance();
    if(sensorMngr.mClientRefCount>=TAF_SENSOR_CLIENT_ACTIVATION_MAX) {
        LE_ERROR("Client count has reached to max limit.");
        return;
    }
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_NIL(NULL==clientRequestPtr, "OnClientConnection clientRequestPtr is NULL");
    return;
}

taf_Sensor::~taf_Sensor()
{
    LE_INFO("~taf_Sensor");
}

le_result_t taf_Sensor::GetSensorList(int8_t listSize){
    for(int8_t i=0;i<listSize;i++){
        taf_SensorPAInfo_t info;
        le_result_t res =
            taf_pa_sensor_GetSensorInfo(i,info.basicInfo,info.configInfo,info.capInfo);
        if(res != LE_OK){
            LE_ERROR("unable to get sensor info for index %d",i);
            return res;
        }
        sList.push_back(info);
    }
    return LE_OK;
}

void taf_Sensor::Init()
{
    LE_INFO("** Init Started **");
    mClientRefCount = 0;
    numOfSelfTestEventHandler=0;
    numofSensorEventHandlers = 0;
    int8_t listSize=0;
    if(taf_pa_sensor_Init(listSize) != LE_OK){
        LE_FATAL("Sensor Platform unable to initialize");
        return;
    }
    if(GetSensorList(listSize) != LE_OK){
        LE_FATAL("Unable to get SensorInfo");
        return;
    }
    tSensorInfoPool = le_mem_InitStaticPool(tSensorInfoPool, TAF_SENSOR_POOL_SIZE,
        sizeof(taf_SensorInfo_t));
    tSensorListPool = le_mem_InitStaticPool(tSensorListPool, TAF_SENSOR_LIST_POOL_SIZE,
        sizeof(taf_SensorInfoList_t));
    tSensorEventInfoPool = le_mem_InitStaticPool(tSensorEventInfoPool, TAF_SENSOR_MAX_EVENTS_SIZE,
        sizeof(taf_SensorEventInfo_t));
    tSensorInfoMap = le_ref_InitStaticMap(tSensorInfoMap, TAF_SENSOR_POOL_SIZE);
    tSensorListMap = le_ref_InitStaticMap(tSensorListMap, TAF_SENSOR_LIST_POOL_SIZE);
    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap",
        TAF_SENSOR_CLIENT_ACTIVATION_MAX);
    ClientPoolRef = le_mem_InitStaticPool(ClientPoolRef,
        TAF_SENSOR_CLIENT_ACTIVATION_MAX, sizeof(taf_SensorClient_t));
    tSensorEventPool = le_mem_InitStaticPool(tSensorEventPool,
        TAF_SENSOR_MAX_EVENTS_SIZE, sizeof(taf_SensorEventList_t));
    tSensorEventHandlerPool = le_mem_InitStaticPool(tSensorEventHandlerPool,
        SENSOR_EVENT_HANDLER_HIGH , sizeof(taf_SensorEventHandler_t));
    tSensorEventMap = le_ref_CreateMap("tSensorEventMap",TAF_SENSOR_MAX_EVENTS_SIZE);
    tSensorEventHandlerMap = le_ref_CreateMap("EventHandlerRefMap",SENSOR_EVENT_HANDLER_HIGH);
    le_msg_ServiceRef_t msgService = taf_imuSensor_GetServiceRef();
    le_msg_AddServiceOpenHandler(msgService, OpenEventHandler, NULL);
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
}
