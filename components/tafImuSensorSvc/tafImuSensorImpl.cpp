/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <time.h>
#include <memory>
#include <future>
#include <vector>
#include <telux/sensor/SensorFactory.hpp>
#include "tafImuSensor.hpp"

using namespace telux::sensor;
using namespace telux::common;
using namespace telux::tafsvc;

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

taf_Sensor &taf_Sensor::GetInstance()
{
    static taf_Sensor instance;
    return instance;
}

void ConvertSensorType(taf_SensorInfo_t* sensorInfoPtr,SensorInfo info)
{
    telux::sensor::SensorType type = info.type;
    switch(type){
        case telux::sensor::SensorType::ACCELEROMETER:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_ACCELEROMETER;
        break;

        case telux::sensor::SensorType::GYROSCOPE:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_GYROSCOPE;
        break;

        case telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_GYROSCOPE;
        break;

        case telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_ACCELEROMETER;
        break;

        case telux::sensor::SensorType::INVALID:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_INVALID;
        break;
    }
}

le_result_t taf_Sensor::InitializeSensorList(taf_SensorClient_t* clientRequestPtr){
    auto& sensorMngr = taf_Sensor::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;
    if(clientRequestPtr->mSensorManager != nullptr && sensorMngr.sList.size() == 0){
        status = clientRequestPtr->mSensorManager->getAvailableSensorInfo(sensorMngr.sList);
    }
    return status == telux::common::Status::SUCCESS ? LE_OK : LE_FAULT;
}

telux::common::ServiceStatus taf_Sensor::SensorManagerInit(taf_SensorClient_t* clientRequestPtr)
{
    LE_DEBUG("** SensorManagerInit **");
    if(clientRequestPtr->mSensorManager == nullptr){
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom;
    //  Get the SensorFactory and SensorManager instances.
    auto &sensorFactory = telux::sensor::SensorFactory::getInstance();
    clientRequestPtr->mSensorManager = sensorFactory.getSensorManager(
        [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (!clientRequestPtr->mSensorManager) {
        LE_FATAL("Failed to get SensorManager");
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    //  Check if sensor subsystem is ready
    //  If sensor subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = clientRequestPtr->mSensorManager->getServiceStatus();
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LE_INFO("Sensor subsystem is not ready, Please wait ...");
    }
    managerStatus = prom.get_future().get();
    //  Exit the application, if SDK is unable to initialize sensor subsystems
    if (managerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_INFO("Elapsed Time for Sensor Subsystems to ready : %lf",elapsedTime.count());
    } else {
        LE_FATAL("ERROR - Unable to initialize sensor subsystem");
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    LE_INFO("SensorManagerInit for sessionRef: %p",  clientRequestPtr->sessionRef);
    clientRequestPtr->eventListener = std::make_shared<tafSensorListener>();
    clientRequestPtr->eventListener->clientSessionRef = &clientRequestPtr->sessionRef;
    clientRequestPtr->SensorOnEventId = le_event_CreateIdWithRefCounting("sensorOnEventId");
    clientRequestPtr->SelfTestEventId =
        le_event_CreateId("SelfTestEventId",sizeof(taf_SensorSelfTest_t));
    clientRequestPtr->HandlerRef = le_event_AddHandler("SensorEventId",
        clientRequestPtr->SensorOnEventId, taf_Sensor::DataEventHandler);
    le_result_t result = InitializeSensorList(clientRequestPtr);
    if(result != LE_OK){
        LE_INFO("SensorList created succesfully");
    }
    }else{
        LE_INFO("Sensor manager already initialized");
    }
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
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
    CopyLastEvent(&clientRequestPtr->lastEvent,currentEventList);
    if(!sensorMngr.numofSensorEventHandlers)
    {
        LE_DEBUG("No Event handlers, exit Handler Function");
        currentEventList->eventList.clear();
        le_mem_Release(currentEventList);
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
        if(evtHandlerPtr->sessionRef != currentEventList->sessionRef){
            LE_DEBUG("DataEventHandler session ref does not match! ReqPtr.sessionRef: %p,"
                " event.sessionRef: %p", evtHandlerPtr->sessionRef, currentEventList->sessionRef);
        }else if(evtHandlerPtr->sensorRef != currentEventList->sensorRef){
            LE_DEBUG("DataEventHandler sensor ref didnt match %p",evtHandlerPtr->sensorRef);
        }else{
        eventInfo = (taf_SensorEventInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventInfoPool);
        memset(eventInfo, 0, sizeof(taf_SensorEventInfo_t));
        eventInfo->eventPtr = (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
        CopyLastEvent(eventInfo->eventPtr,&clientRequestPtr->lastEvent);
        eventInfo->sessionRef = evtHandlerPtr->sessionRef;
        eventInfo->ref =
            (taf_imuSensor_SampleRef_t)le_ref_CreateRef(sensorMngr.tSensorEventMap,eventInfo);
        evtHandlerPtr->handlerFuncPtr(currentEventList->sensorRef,
            eventInfo->ref,evtHandlerPtr->handlerContextPtr);
        LE_DEBUG("Data reported with ref %p",eventInfo->ref);
        }
    }
    currentEventList->eventList.clear();
    le_mem_Release(currentEventList);
}

le_result_t taf_Sensor::SetEulerAngle(double pitch ,double roll , double yaw)
{
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    Status status = telux::common::Status::FAILED;
    EulerAngleConfig eulerAngleConfig;
    eulerAngleConfig.pitch = pitch;
    eulerAngleConfig.roll = roll;
    eulerAngleConfig.yaw = yaw;
    status =  clientRequestPtr->mSensorManager->setEulerAngleConfig(eulerAngleConfig);
    if(status != telux::common::Status::SUCCESS){
        LE_INFO("Not able to set euler angle for client");
        return LE_FAULT;
    }
    return LE_OK;
}

taf_imuSensor_SensorListRef_t taf_Sensor::GetAvailableSensors()
{
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");
    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    auto& sensorMngr = taf_Sensor::GetInstance();
      taf_SensorInfoList_t* sensorListPtr =
        (taf_SensorInfoList_t*)le_mem_ForceAlloc(sensorMngr.tSensorListPool);
    memset(sensorListPtr,0,sizeof(taf_SensorInfoList_t));
    sensorListPtr->SensorsList = LE_SLS_LIST_INIT;
    sensorListPtr->currPtr = NULL;
    sensorListPtr->sessionRef  = taf_imuSensor_GetClientSessionRef();
    std::vector<SensorInfo> info(sensorMngr.sList);
    if(sensorMngr.sList.size()==0){
        LE_INFO("Not able to get Available Sensor List");
        return NULL;
    }
    sensorListPtr->sensorListSize = info.size();
    if(info.size()>0){
        taf_SensorInfo_t* sensorInfoPtr;
        for(size_t i=0;i<info.size();i++){
            sensorInfoPtr = (taf_SensorInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorInfoPool);
            memset(sensorInfoPtr,0,sizeof(taf_SensorInfoList_t));
            sensorInfoPtr->currPtr = NULL;
            sensorInfoPtr->id  = info[i].id;
            le_utf8_Copy(sensorInfoPtr->name, info[i].name.c_str(),
                NAME_MAX_SIZE , NULL);
            le_utf8_Copy(sensorInfoPtr->vendor, info[i].vendor.c_str(),
                NAME_MAX_SIZE , NULL);
            for(size_t j=0;j<info[i].samplingRates.size();j++){
                sensorInfoPtr->samplingRate[j] = info[i].samplingRates[j];
            }
            ConvertSensorType(sensorInfoPtr,info[i]);
            sensorInfoPtr->sampleRateListSize = info[i].samplingRates.size();
            sensorInfoPtr->maxSamplingRate = info[i].maxSamplingRate;
            sensorInfoPtr->maxBatchCountSupported = info[i].maxBatchCountSupported;
            sensorInfoPtr->minBatchCountSupported = info[i].minBatchCountSupported;
            sensorInfoPtr->range = info[i].range;
            sensorInfoPtr->version = info[i].version;
            sensorInfoPtr->resolution = info[i].resolution;
            sensorInfoPtr->maxRange = info[i].maxRange;
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
    TAF_ERROR_IF_RET_VAL(sensorListRef == nullptr,LE_BAD_PARAMETER,"Null reference(sensorListRef)");
    taf_SensorInfoList_t* listPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap,sensorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
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
    if(clientRequestPtr->isSensorActivated){
        LE_DEBUG("Already Sensor is Active");
        return LE_UNAVAILABLE;
    }
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);
    if(!isValidInput(sensorPtr,samplingRate,batchCount)){
        return LE_UNSUPPORTED;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    status = clientRequestPtr->mSensorManager->getSensorClient(
        clientRequestPtr->currentSensorClient,sensorPtr->name);
    if(status != telux::common::Status::SUCCESS){
        LE_FATAL("unable to create sensor client for %s",sensorPtr->name);
    }
    status =
        clientRequestPtr->currentSensorClient->registerListener(clientRequestPtr->eventListener);
    if(status != telux::common::Status::SUCCESS ){
       LE_DEBUG("Listener register failed for %s with status code %d",
            sensorPtr->name,static_cast<int>(status));
        return LE_FAULT;
    }
    clientRequestPtr->CurrentSensorRef = sensorRef;
    SensorType type  = clientRequestPtr->currentSensorClient->getSensorInfo().type;
    if(type == telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED ||
        type == telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED){
    clientRequestPtr->isCalibrated = false;
    }
    else{
        clientRequestPtr->isCalibrated = true;
    }
    SensorConfiguration s;
    s.samplingRate = samplingRate;
    s.batchCount = batchCount;
    s.isRotated = true;
    s.validityMask.set(SensorConfigParams::SAMPLING_RATE);
    s.validityMask.set(SensorConfigParams::BATCH_COUNT);
    s.validityMask.set(SensorConfigParams::ROTATE);
    status = clientRequestPtr->currentSensorClient->configure(s);
    if(status != telux::common::Status::SUCCESS){
        LE_DEBUG("Sensor Configuration failed for %s with status code %d",
            sensorPtr->name,static_cast<int>(status));
        return LE_FAULT;
    }
    le_thread_Sleep(1);
    status = clientRequestPtr->currentSensorClient->activate();
    if(status != telux::common::Status::SUCCESS){
        LE_DEBUG("Sensor activation failed for %s with status code %d",sensorPtr->name,
        static_cast<int>(status));
        return LE_FAULT;
    }
    clientRequestPtr->isSensorActivated = true;
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
    telux::common::Status status = telux::common::Status::FAILED;
    status = clientRequestPtr->mSensorManager->getSensorClient(
        clientRequestPtr->selfTestClient,sensorPtr->name);
    if(status != telux::common::Status::SUCCESS){
        LE_FATAL("unable to create sensor client for %s",sensorPtr->name);
    }
    SelfTestType type;
    if(mode == TAF_IMUSENSOR_POSITIVE){
        type  = SelfTestType::POSITIVE;
    }
    else if(mode == TAF_IMUSENSOR_NEGATIVE){
        type  = SelfTestType::NEGATIVE;
    }
    else if(mode == TAF_IMUSENSOR_BOTH){
        type = SelfTestType::ALL;
    }
    else{
        return LE_FAULT;
    }
    std::promise<le_result_t> p1;
    auto cb1 = [&p1,&timestamp](telux::common::ErrorCode error,
        SelfTestResultParams selfTestResultParams) {
        if(error == telux::common::ErrorCode::SUCCESS) {
            *timestamp = selfTestResultParams.timestamp_;
            if(selfTestResultParams.sensorResultType_ == SensorResultType::CURRENT){
                p1.set_value(LE_OK);
            }
            else{
                p1.set_value(LE_BUSY);
            }
        }
        else if (error == telux::common::ErrorCode::INFO_UNAVAILABLE){
            p1.set_value(LE_UNAVAILABLE);
        }
        else {
            p1.set_value(LE_FAULT);
        }
    };
    status = clientRequestPtr->selfTestClient->selfTest(type,cb1);
    if(status == telux::common::Status::SUCCESS){
        std::future<le_result_t> futResult = p1.get_future();
        //wait for result with 3 second time out.
        if(futResult.wait_for(std::chrono::seconds(3)) == std::future_status::ready){
            le_result_t selfResult = futResult.get();
            if(selfResult != LE_OK){
                LE_ERROR("self test failed");
                clientRequestPtr->selfTestClient = NULL;
                return selfResult;
            }
        }else{
            LE_ERROR("Timeout waiting for result..");
            clientRequestPtr->selfTestClient = NULL;
            return LE_TIMEOUT;
        }
    }
    else if(status == telux::common::Status::NOTSUPPORTED){
        LE_ERROR("Not supported on this target");
        clientRequestPtr->selfTestClient = NULL;
        return  LE_UNSUPPORTED;
    }
    else{
        LE_ERROR("unable to start self test");
        clientRequestPtr->selfTestClient = NULL;
        return LE_FAULT;
    }
    clientRequestPtr->selfTestClient = NULL;
    return LE_OK;
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
    le_event_HandlerRef_t handlerRef;
    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL(NULL == clientRequestPtr, NULL,"AddSelfTestFailedHandler: clientRequestPtr"
        " is NULL");
    handlerRef = le_event_AddLayeredHandler("SelfTestHandler", clientRequestPtr->SelfTestEventId,
        FirstLayerSelfTestHandler, (void*)handlerPtr);
    numOfSelfTestEventHandler++;
    le_event_SetContextPtr(handlerRef,contextPtr);
    return (taf_imuSensor_SelfTestFailedHandlerRef_t)handlerRef;
}

void taf_Sensor::RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if(numOfSelfTestEventHandler>0){
        numOfSelfTestEventHandler--;
    }
}

void tafSensorListener::onSelfTestFailed(){
    LE_INFO("onSelfTestFailed");
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(*clientSessionRef);
    if (NULL == clientRequestPtr) {
        LE_DEBUG("onSelfTestFailed did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mSensorMutexRef);
    if(sensorMngr.numOfSelfTestEventHandler){
        taf_SensorSelfTest_t event;
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        event.timestamp = (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
        event.cSensorRef = clientRequestPtr->CurrentSensorRef;
        le_event_Report(clientRequestPtr->SelfTestEventId,&event,sizeof(event));
    }
    else{
        LE_INFO("Self Test Handler not found");
    }
    le_mutex_Unlock(clientRequestPtr->mSensorMutexRef);
}

le_result_t taf_Sensor::Deactivate(taf_imuSensor_SensorRef_t sensorRef){
    LE_DEBUG("Deactivate Sensor");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,LE_FAULT, "clientRequestPtr is NULL");
    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    char sensorName[50];
    le_result_t result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
    TAF_ERROR_IF_RET_VAL( result == LE_FAULT,LE_NOT_FOUND, "Invalid Reference for sensor");
    telux::common::Status status = telux::common::Status::FAILED;
    status = clientRequestPtr->currentSensorClient->deactivate();
    if(status != telux::common::Status::SUCCESS){
        LE_DEBUG("Deactivation of %s failed",sensorName);
        return LE_FAULT;
    }
    status = clientRequestPtr->currentSensorClient->
        deregisterListener(clientRequestPtr->eventListener);
    if(status != telux::common::Status::SUCCESS){
        LE_ERROR("Unable to deregister client");
        return LE_FAULT;
    }
    clientRequestPtr->currentSensorClient = nullptr;
    clientRequestPtr->isSensorActivated = false;
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

void tafSensorListener::onEvent(std::shared_ptr<std::vector<SensorEvent>> events){
    LE_DEBUG("onEvent for *sessionRef: %p", *clientSessionRef);
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(*clientSessionRef);
    if (NULL == clientRequestPtr) {
        LE_DEBUG("onEvent did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mSensorMutexRef);
    if(sensorMngr.numofSensorEventHandlers){
        taf_SensorEventList_t* triggeredSensorEvent =
            (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
        for (SensorEvent s : *(events.get())){
            std::shared_ptr<taf_SensorEvent_t> eventData;
            try{
                eventData = std::make_shared<taf_SensorEvent_t>();
            } catch(const std::exception &e){
                LE_FATAL("Not able to intialize eventData with exception %s",e.what());
            }
            eventData->timestamp = s.timestamp;
        if(clientRequestPtr->isCalibrated==false){
            eventData->x = s.uncalibrated.data.x;
            eventData->y = s.uncalibrated.data.y;
            eventData->z= s.uncalibrated.data.z;
            eventData->xb= s.uncalibrated.bias.x;
            eventData->yb = s.uncalibrated.bias.y;
            eventData->zb = s.uncalibrated.bias.z;
        } else {
            eventData->x = s.calibrated.x;
            eventData->y= s.calibrated.y;
            eventData->z= s.calibrated.z;
            eventData->xb= 0;
            eventData->yb= 0;
            eventData->zb= 0;
        }
        triggeredSensorEvent->eventList.push_back(eventData);
    }
    triggeredSensorEvent->sensorRef = clientRequestPtr->CurrentSensorRef;
    triggeredSensorEvent->listSize = events->size();
    triggeredSensorEvent->sessionRef = clientRequestPtr->sessionRef;
    le_event_ReportWithRefCounting(clientRequestPtr->SensorOnEventId,triggeredSensorEvent);
    }else{
         LE_DEBUG("handler not found");
    }
    le_mutex_Unlock(clientRequestPtr->mSensorMutexRef);
}

void tafSensorListener::onConfigurationUpdate(SensorConfiguration configuration){
    LE_DEBUG("onConfiguration for *sessionRef: %p", *clientSessionRef);
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(*clientSessionRef);
    if (NULL == clientRequestPtr) {
        LE_DEBUG("onConfigurationUpdate did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mSensorMutexRef);
    char sensorName[50];
    le_result_t result = taf_imuSensor_GetName(clientRequestPtr->CurrentSensorRef,
        sensorName,sizeof(sensorName));
    if(result != LE_OK){
        LE_DEBUG("Not able to get sensor name");
    }
    LE_DEBUG("*sensor configuration copied:%s %f %d %d*",sensorName,configuration.samplingRate,
        configuration.batchCount,configuration.isRotated);
    le_mutex_Unlock(clientRequestPtr->mSensorMutexRef);
}

le_result_t taf_Sensor::GetData( taf_imuSensor_SampleRef_t eventList,taf_imuSensor_DataValue_t*
    RawData,size_t* RawDataSizePtr,taf_imuSensor_DataValue_t*BiasData,size_t* BiasDataSizePtr){
    LE_DEBUG("GetData of list with ref %p",eventList);
    taf_SensorEventInfo_t* ptr =
    (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventList);
    TAF_ERROR_IF_RET_VAL(ptr == NULL, LE_NOT_FOUND,
        "Invalid reference (%p) provided!", ptr);
    size_t j=0;
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
    TAF_ERROR_IF_RET_VAL(eventListRef == nullptr,LE_BAD_PARAMETER,"Null reference(eventListRef)");
    taf_SensorEventInfo_t* listPtr =
        (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
    listPtr->eventPtr->eventList.clear();
    le_ref_DeleteRef(tSensorEventMap, eventListRef);
    le_mem_Release(listPtr->eventPtr);
    le_mem_Release(listPtr);
    return LE_OK;
}

void taf_Sensor::InitializeClient(taf_SensorClient_t* clientRequestPtr)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    clientRequestPtr->isCalibrated = 0;
    clientRequestPtr->isSensorActivated = 0;
    clientRequestPtr->mBatchCount = 0;
    clientRequestPtr->CurrentSensorRef = NULL;
    clientRequestPtr->mSensorMutexRef = le_mutex_CreateRecursive("SensorMutexCl");
    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_FAILED;
    status = sensorMngr.SensorManagerInit(clientRequestPtr);
    if(status != telux::common::ServiceStatus::SERVICE_AVAILABLE){
        LE_ERROR("SensorManager not available");
    }
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
        InitializeClient(clientRequestPtr);
        void* reqRefPtr = le_ref_CreateRef(sensorMngr.ClientRequestRefMap, clientRequestPtr);
        if (sessionRef!=nullptr) {
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
    if(clientPtr->currentSensorClient){
        telux::common::Status status = telux::common::Status::FAILED;
        if(clientPtr->isSensorActivated == true){
            status = clientPtr->currentSensorClient->deactivate();
            if(status == telux::common::Status::SUCCESS){
                clientPtr->isSensorActivated = false;
                LE_INFO("Sensor Deactivated");
            }
        }
        status = clientPtr->currentSensorClient->deregisterListener(clientPtr->eventListener);
        if(status == telux::common::Status::SUCCESS){
                LE_INFO("client deregister");
        }
        clientPtr->currentSensorClient = nullptr;
    }

    if(clientPtr->mSensorManager){
        clientPtr->mSensorManager = nullptr;
    }

    if(clientPtr->eventListener){
        clientPtr->eventListener = nullptr;
    }

    if(clientPtr->CurrentSensorRef){
        clientPtr->CurrentSensorRef = nullptr;
    }
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
        if(dataEventPtr ==NULL){
            return ;
        }
        if(dataEventPtr->sessionRef == sessionRef){
            taf_imuSensor_SampleRef_t  safeRef =
                (taf_imuSensor_SampleRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_imuSensor_DeleteData 0x%p, Session 0x%p", safeRef, sessionRef);
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
    auto &sensorMngr = taf_Sensor::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        LE_DEBUG("~taf_sensor %d", result);
        taf_SensorClient_t* clientPtr = (taf_SensorClient_t*) le_ref_GetValue(iterRef);
        sensorMngr.CleanUp(clientPtr);
        result = le_ref_NextNode(iterRef);
    }
}

void taf_Sensor::Init()
{
    LE_INFO("** Init Started **");
    mClientRefCount = 0;
    numOfSelfTestEventHandler=0;
    numofSensorEventHandlers = 0;
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
