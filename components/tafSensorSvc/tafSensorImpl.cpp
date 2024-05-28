/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <chrono>
#include <time.h>
#include <memory>
#include <future>
#include <vector>
#include <telux/sensor/SensorFactory.hpp>
#include "tafSensor.hpp"

using namespace telux::sensor;
using namespace telux::common;
using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tSensorInfoPool, TAF_SENSOR_POOL_SIZE, sizeof(taf_SensorInfo_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorListPool, TAF_SENSOR_LIST_POOL_SIZE, sizeof(taf_SensorInfoList_t));
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
            sensorInfoPtr->sensorType = TAF_SENSOR_ACCELEROMETER;
        break;

        case telux::sensor::SensorType::GYROSCOPE:
            sensorInfoPtr->sensorType = TAF_SENSOR_GYROSCOPE;
        break;

        case telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED:
            sensorInfoPtr->sensorType = TAF_SENSOR_GYROSCOPE;
        break;

        case telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED:
            sensorInfoPtr->sensorType = TAF_SENSOR_ACCELEROMETER;
        break;

        case telux::sensor::SensorType::INVALID:
            sensorInfoPtr->sensorType = TAF_SENSOR_INVALID;
        break;
    }
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
    }
    else{
        LE_INFO("Sensor manager already initialized");
    }
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
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

taf_sensor_SensorListRef_t taf_Sensor::GetAvailableSensors()
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
    sensorListPtr->sessionRef  = taf_sensor_GetClientSessionRef();
    std::vector<SensorInfo> info;
    telux::common::Status status = clientRequestPtr->mSensorManager->getAvailableSensorInfo(info);
    if(status != telux::common::Status::SUCCESS){
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
                (taf_sensor_SensorRef_t)le_ref_CreateRef(sensorMngr.tSensorInfoMap, sensorInfoPtr);
        }
        sensorListPtr->ref = (taf_sensor_SensorListRef_t)
                        le_ref_CreateRef(sensorMngr.tSensorListMap,sensorListPtr);
        return sensorListPtr->ref;
    }
    return NULL;
}

le_result_t taf_Sensor::DeleteSensorList(taf_sensor_SensorListRef_t sensorListRef)
{
    TAF_ERROR_IF_RET_VAL(sensorListRef == nullptr,LE_BAD_PARAMETER,"Null reference(sensorListRef)");
    taf_SensorInfoList_t* listPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap,sensorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
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

taf_sensor_SensorRef_t taf_Sensor::GetFirstSensor(taf_sensor_SensorListRef_t sensorListRef)
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

taf_sensor_SensorRef_t taf_Sensor::GetNextSensor(taf_sensor_SensorListRef_t sensorListRef)
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

le_result_t taf_Sensor::GetSensorId(taf_sensor_SensorRef_t sensorRef,uint32_t* sensorIdPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    *sensorIdPtr = sensorPtr->id;
    TAF_ERROR_IF_RET_VAL(*sensorIdPtr == 0, LE_FAULT, "invalid id for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorName(taf_sensor_SensorRef_t sensorRef,char* sensorName,
    size_t sensorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    snprintf(sensorName, sizeof(sensorPtr->name), "%s", sensorPtr->name);
    TAF_ERROR_IF_RET_VAL(sensorName == NULL, LE_FAULT, "invalid Name for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVendorName(taf_sensor_SensorRef_t sensorRef,
    char* sensorVendorName,size_t sensorVendorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    snprintf(sensorVendorName, sizeof(sensorPtr->vendor), "%s", sensorPtr->vendor);
    TAF_ERROR_IF_RET_VAL(sensorVendorName == NULL, LE_FAULT, "invalid vendor for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVersion(taf_sensor_SensorRef_t sensorRef,char* version,
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

le_result_t taf_Sensor::GetSensorType(taf_sensor_SensorRef_t sensorRef,
    taf_sensor_SensorType_t* sensorTypePtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    *sensorTypePtr = sensorPtr->sensorType;
    TAF_ERROR_IF_RET_VAL(*sensorTypePtr == 0, LE_FAULT, "invalid type for sensor");
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorSamplingRateInfo(taf_sensor_SensorRef_t sensorRef,
    double* samplingRatesListPtr, size_t* samplingRatesListSizePtr)
{
     taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

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

le_result_t taf_Sensor::GetSensorBatchingInfo(taf_sensor_SensorRef_t sensorRef,
    uint32_t* maxBatchCountSupportedPtr,uint32_t* minBatchCountSupportedPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    *maxBatchCountSupportedPtr = sensorPtr->maxBatchCountSupported;
    TAF_ERROR_IF_RET_VAL(*maxBatchCountSupportedPtr == 0, LE_FAULT,
        "invalid sensor maxBatchCountSupportedPtr for sensor");

    *minBatchCountSupportedPtr = sensorPtr->minBatchCountSupported;
    TAF_ERROR_IF_RET_VAL(*minBatchCountSupportedPtr == 0, LE_FAULT,
    "invalid sensor maxBatchCountSupportedPtr for sensor");

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorRangeInfo(taf_sensor_SensorRef_t sensorRef, double* rangePtr)
{

    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    *rangePtr = sensorPtr->range;
    TAF_ERROR_IF_RET_VAL(*rangePtr == 0, LE_FAULT,"invalid sensor RangePtr for sensor");

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorResolution(taf_sensor_SensorRef_t sensorRef,
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

void taf_Sensor::InitializeClient(taf_SensorClient_t* clientRequestPtr)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_FAILED;
    status = sensorMngr.SensorManagerInit(clientRequestPtr);
    if(status == telux::common::ServiceStatus::SERVICE_FAILED){
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
    le_msg_SessionRef_t sessionRef = taf_sensor_GetClientSessionRef();
    clientRequestPtr = DiscoverSessionRef(sessionRef);
    if (clientRequestPtr == NULL)
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
    if (sessionRef!=nullptr && sensorMngr.mClientRefCount > 0){
        //External Client release, decrement client ref count
        sensorMngr.mClientRefCount--;
    }
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorListMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorInfoList_t* listPtr =
                (taf_SensorInfoList_t*)le_ref_GetValue(iterRef);
        if (listPtr && listPtr->sessionRef == sessionRef)
        {
            taf_sensor_DeleteSensorList(listPtr->ref);
        }
    }

    iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorClient_t* sensorClientPtr = (taf_SensorClient_t*) le_ref_GetValue(iterRef);
        if (sensorClientPtr && sessionRef == sensorClientPtr->sessionRef)
        {
            void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_gnss_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, sensorClientPtr->sessionRef);

            sensorMngr.ReleaseClientRef(safeRefPtr);
        }
    }
}

void taf_Sensor::OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr)
{
    LE_INFO("OnClientConnection sessionRef: %p", sessionRef);
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "OnClientConnection clientRequestPtr is NULL");
    return;
}

taf_Sensor::~taf_Sensor()
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_SensorClient_t* clientPtr = (taf_SensorClient_t*) le_ref_GetValue(iterRef);
        if(clientPtr == NULL) {
            return;
        }
        if(clientPtr->mSensorManager){
            clientPtr->mSensorManager = nullptr;
        }
    }
    result = le_ref_NextNode(iterRef);
}

void taf_Sensor::Init()
{
    LE_INFO("** Init Started **");
    mClientRefCount = 0;
    tSensorInfoPool = le_mem_InitStaticPool(tSensorInfoPool, TAF_SENSOR_POOL_SIZE,
        sizeof(taf_SensorInfo_t));
    tSensorListPool = le_mem_InitStaticPool(tSensorListPool, TAF_SENSOR_LIST_POOL_SIZE,
        sizeof(taf_SensorInfoList_t));
    tSensorInfoMap = le_ref_InitStaticMap(tSensorInfoMap, TAF_SENSOR_POOL_SIZE);
    tSensorListMap = le_ref_InitStaticMap(tSensorListMap, TAF_SENSOR_LIST_POOL_SIZE);
    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap",
        TAF_SENSOR_CLIENT_ACTIVATION_MAX);
    ClientPoolRef = le_mem_InitStaticPool(ClientPoolRef,
        TAF_SENSOR_CLIENT_ACTIVATION_MAX, sizeof(taf_SensorClient_t));
    le_msg_ServiceRef_t msgService = taf_sensor_GetServiceRef();
    le_msg_AddServiceOpenHandler(msgService, OpenEventHandler, NULL);
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
}