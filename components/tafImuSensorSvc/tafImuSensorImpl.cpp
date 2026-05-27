/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <cmath>
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
LE_MEM_DEFINE_STATIC_POOL(tSensorConfigUpdateHandlerPool, SENSOR_EVENT_HANDLER_HIGH,
    sizeof(taf_SensorConfigUpdateHandler_t));
LE_MEM_DEFINE_STATIC_POOL(tSensorCapabilityHandlerPool, SENSOR_EVENT_HANDLER_HIGH,
    sizeof(taf_SensorCapabilityHandler_t));
LE_REF_DEFINE_STATIC_MAP(tSensorInfoMap, TAF_SENSOR_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tSensorListMap, TAF_SENSOR_LIST_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
// Safe std::any -> le_msg_SessionRef_t conversion (prevents std::bad_any_cast crash).
//--------------------------------------------------------------------------------------------------
static bool TryGetSessionRef(const std::any& ctx, le_msg_SessionRef_t* out)
{
    if (!out) return false;
    *out = NULL;
    try {
        *out = std::any_cast<le_msg_SessionRef_t>(ctx);
        return (*out != NULL);
    } catch (const std::bad_any_cast&) {
        return false;
    } catch (...) {
        return false;
    }
}

void Handler::onSelfTestFailed(tafpa::sensor::taf_pa_sensor_SensorId sensorId, uint64_t timestamp,
    std::any context){
    LE_DEBUG("onSelfTestFailed");

    le_msg_SessionRef_t sessionRef = NULL;
    if (!TryGetSessionRef(context, &sessionRef)) {
        LE_ERROR("Bad any_cast for sessionRef");
        return;
    }

    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorSelfTest_t event;
    event.timestamp = timestamp;
    event.sessionRef = sessionRef;
    event.sensorClientId = sensorId;
    le_event_Report(sensorMngr.SelfTestEventId,&event,sizeof(event));
}

void Handler::onEvent(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
    std::shared_ptr<const std::vector<tafpa::sensor::taf_pa_sensor_Event>> events,
    std::any context)
{
    le_msg_SessionRef_t sessionRef = NULL;
    if (!TryGetSessionRef(context, &sessionRef)) {
        LE_ERROR("Bad any_cast for sessionRef");
        return;
    }

    if (!events) {
        LE_ERROR("Events is NULL");
        return;
    }

    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorEventList_t* triggeredSensorEvent =
        (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
    triggeredSensorEvent->sensorClientId = sensorId;
    triggeredSensorEvent->listSize = (events->size() > TAF_SENSOR_MAX_EVENTS_SIZE)
        ? TAF_SENSOR_MAX_EVENTS_SIZE
        : (uint32_t)events->size();
    triggeredSensorEvent->sessionRef = sessionRef;
    for (uint32_t i = 0; i < triggeredSensorEvent->listSize; ++i) {
        triggeredSensorEvent->eventList[i].timestamp = (*events)[i].timestamp;
        triggeredSensorEvent->eventList[i].x = (*events)[i].x;
        triggeredSensorEvent->eventList[i].y = (*events)[i].y;
        triggeredSensorEvent->eventList[i].z = (*events)[i].z;
        triggeredSensorEvent->eventList[i].xb = (*events)[i].xb;
        triggeredSensorEvent->eventList[i].yb = (*events)[i].yb;
        triggeredSensorEvent->eventList[i].zb = (*events)[i].zb;
    }
    le_event_ReportWithRefCounting(sensorMngr.SensorOnEventId,triggeredSensorEvent);
}

void Handler::onConfigUpdate(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
    double samplingRate, uint32_t batchCount, bool isRotated, std::any context)
{
    LE_DEBUG("onConfigUpdate");

    le_msg_SessionRef_t sessionRef = NULL;
    if (!TryGetSessionRef(context, &sessionRef)) {
        LE_ERROR("Bad any_cast for sessionRef");
        return;
    }

    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorConfigUpdate_t event = {};
    event.sensorClientId = sensorId;
    event.samplingRate = samplingRate;
    event.batchCount = batchCount;
    event.isRotated = isRotated;
    event.sessionRef = sessionRef;
    le_event_Report(sensorMngr.ConfigUpdateEventId, &event, sizeof(event));
}

void Handler::onCapabilityUpdate(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
    tafpa::sensor::taf_pa_sensor_CapabilityInfo capabilityInfo, std::any context)
{
    LE_DEBUG("onCapabilityUpdate");

    le_msg_SessionRef_t sessionRef = NULL;
    if (!TryGetSessionRef(context, &sessionRef)) {
        LE_ERROR("Bad any_cast for sessionRef");
        return;
    }

    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorCapability_t event = {};
    event.sensorClientId = sensorId;
    event.isAvailable = capabilityInfo.isAvailable;
    event.isEnabled = capabilityInfo.isEnabled;
    event.capabilityMask = capabilityInfo.capabilityMask;
    event.sessionRef = sessionRef;
    le_event_Report(sensorMngr.CapabilityEventId, &event, sizeof(event));
}

taf_Sensor &taf_Sensor::GetInstance()
{
    static taf_Sensor instance;
    return instance;
}

void ConvertSensorType(taf_SensorInfo_t* sensorInfoPtr,tafpa::sensor::taf_pa_sensor_BasicInfo info)
{
    if (!sensorInfoPtr) {
        LE_ERROR("ConvertSensorType: sensorInfoPtr is NULL");
        return;
    }

    tafpa::sensor::taf_pa_sensor_SensorType type = info.sensorType;
    switch(type){
        case tafpa::sensor::taf_pa_sensor_SensorType::ACCELEROMETER:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_ACCELEROMETER;
            break;

        case tafpa::sensor::taf_pa_sensor_SensorType::GYROSCOPE:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_GYROSCOPE;
            break;

        case tafpa::sensor::taf_pa_sensor_SensorType::INVALID:
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_INVALID;
            break;

        default:
            LE_WARN("ConvertSensorType: unknown sensor type %d, treating as INVALID", (int)type);
            sensorInfoPtr->sensorType = TAF_IMUSENSOR_INVALID;
            break;
    }
}

le_result_t InitializeSensorClientList(taf_SensorClient_t* clientRequestPtr)
{
    auto& sensorMngr = taf_Sensor::GetInstance();

    if (!clientRequestPtr)
    {
        LE_ERROR("InitializeSensorClientList: clientRequestPtr is NULL");
        return LE_BAD_PARAMETER;
    }
    clientRequestPtr->clientCount = 0;

    for(size_t i=0;i<sensorMngr.sList.size();i++)
    {
        if (clientRequestPtr->clientCount >= TAF_SENSOR_POOL_SIZE) {
            LE_ERROR("Max client count reached");
            break;
        }
        taf_sensorClientInfo_t* clientInfo =
            &clientRequestPtr->clients[clientRequestPtr->clientCount];
        clientInfo->isSensorActivated = false;
        clientInfo->sensorClient = 0;
        pa_result_t clientRes = tafpa::sensor::taf_pa_sensor_GetSensorClient(
            sensorMngr.sList[i].basicInfo.sensorName,
            clientInfo->sensorClient);
        if ((clientRes != PA_OK) || (clientInfo->sensorClient == 0)){
            LE_ERROR("unable to create Reference for %s in session %p, res=%d",
                                    sensorMngr.sList[i].basicInfo.sensorName.c_str(),
                                    clientRequestPtr->sessionRef, (int)clientRes);
            continue;
        }
        clientInfo->eventListener.onEvent = &Handler::onEvent;
        clientInfo->eventListener.onSelfTestFailed = &Handler::onSelfTestFailed;
        if(tafpa::sensor::taf_pa_sensor_AddListener(clientInfo->sensorClient,
            &clientInfo->eventListener,std::any(clientRequestPtr->sessionRef)) !=  PA_OK){
            LE_ERROR("Listener register failed for %s in session %p",
                sensorMngr.sList[i].basicInfo.sensorName.c_str(), clientRequestPtr->sessionRef);
            continue;
        }

        LE_INFO("Add listener for %lu successfully", clientInfo->sensorClient);

        le_utf8_Copy(clientInfo->sensorName, sensorMngr.sList[i].basicInfo.
            sensorName.c_str(), sizeof(clientInfo->sensorName), NULL);
        clientRequestPtr->clientCount++;
    }

    if (clientRequestPtr->clientCount == 0)
    {
        LE_ERROR("InitializeSensorClientList: no sensors registered for this session");
        return LE_FAULT;
    }
    return LE_OK;
}

taf_SensorClient_t* AcquireSessionRef(le_msg_SessionRef_t sessionRef)
{
    auto &sensorMngr = taf_Sensor::GetInstance();
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = sensorMngr.DiscoverSessionRef(sessionRef);
    if (clientRequestPtr == NULL && sensorMngr.mClientRefCount<TAF_SENSOR_CLIENT_ACTIVATION_MAX)
    {
        clientRequestPtr = (taf_SensorClient_t*)le_mem_ForceAlloc(sensorMngr.ClientPoolRef);
        clientRequestPtr->clientRefPtr = NULL;
        clientRequestPtr->sessionRef = sessionRef;
        clientRequestPtr->clientCount = 0;

        if (InitializeSensorClientList(clientRequestPtr) != LE_OK)
        {
            le_mem_Release(clientRequestPtr);
            return NULL;
        }

        void* reqRefPtr = le_ref_CreateRef(sensorMngr.ClientRequestRefMap, clientRequestPtr);
        if (sessionRef != NULL)
        {
            sensorMngr.mClientRefCount++;
        }

        LE_INFO("Create new %p for sessionRef %p, total count %d", clientRequestPtr,
                                                    sessionRef, sensorMngr.mClientRefCount);

        clientRequestPtr->clientRefPtr = reqRefPtr;
    }
    else if (clientRequestPtr == NULL)
    {
        LE_DEBUG("AcquireSessionRef: max client count (%d) reached, sessionRef %p",
            TAF_SENSOR_CLIENT_ACTIVATION_MAX, sessionRef);
    }
    return clientRequestPtr;
}

bool GetPaClientId(le_msg_SessionRef_t sessionRef,
                             taf_imuSensor_SensorRef_t sensorRef,
                             tafpa::sensor::taf_pa_sensor_SensorId* outId)
{
    auto& m = taf_Sensor::GetInstance();

    if (!outId) return false;
    *outId = 0;

    taf_SensorClient_t* client = AcquireSessionRef(sessionRef);
    if (!client)
    {
        LE_ERROR("GetPaClientId: failed to acquire session for sessionRef %p", sessionRef);
        return false;
    }

    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(m.tSensorInfoMap, sensorRef);
    if (!sensorPtr)
    {
        LE_ERROR("GetPaClientId: invalid sensorRef %p", sensorRef);
        return false;
    }

    for (uint32_t i = 0; i < client->clientCount; ++i)
    {
        auto* ci = &client->clients[i];
        if (strcmp(sensorPtr->name, ci->sensorName) == 0)
        {
            *outId = ci->sensorClient;
            return (*outId != 0);
        }
    }
    LE_ERROR("GetPaClientId: sensor '%s' not found in client list for sessionRef %p",
        sensorPtr->name, sessionRef);
    return false;
}


void CopyLastEvent(taf_SensorEventList_t* LastEventPtr,taf_SensorEventList_t* currentEventPtr)
{
    if (!LastEventPtr || !currentEventPtr)
    {
        LE_ERROR("CopyLastEvent: NULL ptr Last=%p Cur=%p", LastEventPtr, currentEventPtr);
        return;
    }

    LastEventPtr->sensorClientId = currentEventPtr->sensorClientId;
    LastEventPtr->sessionRef = currentEventPtr->sessionRef;

    uint32_t n = currentEventPtr->listSize;
    if (n > TAF_SENSOR_MAX_EVENTS_SIZE) n = TAF_SENSOR_MAX_EVENTS_SIZE;
    LastEventPtr->listSize = n;

    for (uint32_t i = 0; i < n; ++i)
    {
        LastEventPtr->eventList[i] = currentEventPtr->eventList[i];
    }
}

void taf_Sensor::DataEventHandler(void* reportPtr){
    taf_SensorEventHandler_t* evtHandlerPtr;
    taf_SensorEventInfo_t* eventInfo = NULL;
    taf_SensorEventList_t* currentEventList = (taf_SensorEventList_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL( currentEventList == NULL, "currentEventList is Null");
    taf_SensorClient_t* clientRequestPtr = NULL;
    auto& sensorMngr = taf_Sensor::GetInstance();
    clientRequestPtr = sensorMngr.DiscoverSessionRef(currentEventList->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("EventHandler did not find sessionRef %p", currentEventList->sessionRef);
        le_mem_Release(currentEventList);
        return;
    }

    taf_imuSensor_SensorRef_t sensorRef = NULL;
    for(uint32_t i = 0; i < clientRequestPtr->clientCount; ++i){
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if(clientInfoPtr->sensorClient == currentEventList->sensorClientId){
            sensorRef = clientInfoPtr->sensorRef;
            break;
        }
    }

    if (sensorRef == NULL) {
        LE_ERROR("SensorRef not found for sensorClientId %" PRIu64 ".",
                                                   currentEventList->sensorClientId);
        le_mem_Release(currentEventList);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorEventHandlerMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        evtHandlerPtr = (taf_SensorEventHandler_t*)le_ref_GetValue(iterRef);
        if(evtHandlerPtr == NULL) {
            continue;
        }

        if (!evtHandlerPtr->handlerFuncPtr)
        {
            LE_ERROR("DataEventHandler: handlerFuncPtr is NULL (handler=%p)", evtHandlerPtr);
            continue;
        }

        if(evtHandlerPtr->sessionRef == currentEventList->sessionRef &&
            evtHandlerPtr->sensorRef == sensorRef){
            taf_imuSensor_DataValue_t rawData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT] = {0};
            taf_imuSensor_DataValue_t biasData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT] = {0};

            eventInfo = (taf_SensorEventInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventInfoPool);
            memset(eventInfo, 0, sizeof(taf_SensorEventInfo_t));
            eventInfo->eventPtr =
                (taf_SensorEventList_t*)le_mem_ForceAlloc(sensorMngr.tSensorEventPool);
            CopyLastEvent(eventInfo->eventPtr,currentEventList);
            eventInfo->sessionRef = evtHandlerPtr->sessionRef;
            eventInfo->ref =
            (taf_imuSensor_SampleRef_t)le_ref_CreateRef(sensorMngr.tSensorEventMap,eventInfo);

            size_t dataCount = currentEventList->listSize;
            if (dataCount > TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT)
            {
                dataCount = TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT;
            }

            for (size_t i = 0; i < dataCount; ++i)
            {
                rawData[i].timestamp = currentEventList->eventList[i].timestamp;
                rawData[i].x = currentEventList->eventList[i].x;
                rawData[i].y = currentEventList->eventList[i].y;
                rawData[i].z = currentEventList->eventList[i].z;
                biasData[i].timestamp = currentEventList->eventList[i].timestamp;
                biasData[i].x = currentEventList->eventList[i].xb;
                biasData[i].y = currentEventList->eventList[i].yb;
                biasData[i].z = currentEventList->eventList[i].zb;
            }

            evtHandlerPtr->handlerFuncPtr(eventInfo->ref,
                rawData, dataCount, biasData, dataCount, evtHandlerPtr->handlerContextPtr);

            LE_DEBUG("Data reported with ref %p for sensor %p with session %p",eventInfo->ref,
                           evtHandlerPtr->sensorRef,evtHandlerPtr->sessionRef);
        }
    }
    le_mem_Release(currentEventList);
}

static void SetEulerAngleSvcRespond(void* cmdPtr, void*)
{
    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)cmdPtr;

    taf_imuSensor_SetRefCoordinateByEulerAngleRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_Sensor::SetEulerAngleWorker(void* cmdPtr, void*)
{
    auto& sens = taf_Sensor::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)cmdPtr;

    cPtr->retCode = LE_OK;
    if (tafpa::sensor::taf_pa_sensor_SetEulerAngle(0,
        cPtr->euler.pitch, cPtr->euler.roll, cPtr->euler.yaw) != PA_OK)
    {
        LE_ERROR("SetEulerAngleWorker: PA call failed (pitch=%lf, roll=%lf, yaw=%lf)",
            cPtr->euler.pitch, cPtr->euler.roll, cPtr->euler.yaw);
        cPtr->retCode = LE_FAULT;
    }
    le_event_QueueFunctionToThread(sens.SensorSvcThRef, SetEulerAngleSvcRespond, cPtr, NULL);
}

void taf_Sensor::SetEulerAngle(taf_imuSensor_ServerCmdRef_t cmdRef,
                                   double pitch, double roll, double yaw)
{
    LE_DEBUG("StartSetEulerAngle pitch=%lf, roll=%lf, yaw=%lf", pitch, roll, yaw);
    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)le_mem_ForceAlloc(CmdSensorPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = taf_imuSensor_GetClientSessionRef();
    cPtr->euler.pitch = pitch;
    cPtr->euler.roll  = roll;
    cPtr->euler.yaw   = yaw;

    le_event_QueueFunctionToThread(SensorWorkerThRef, SetEulerAngleWorker, cPtr, NULL);
}

taf_imuSensor_SensorListRef_t taf_Sensor::GetAvailableSensors(le_msg_SessionRef_t sessionRef)
{
    auto& sensorMngr = taf_Sensor::GetInstance();

      taf_SensorInfoList_t* sensorListPtr =
        (taf_SensorInfoList_t*)le_mem_ForceAlloc(sensorMngr.tSensorListPool);
    memset(sensorListPtr,0,sizeof(taf_SensorInfoList_t));

    sensorListPtr->SensorsList = LE_SLS_LIST_INIT;
    sensorListPtr->currPtr = NULL;
    sensorListPtr->sessionRef  = sessionRef;
    sensorListPtr->sensorListSize = sList.size();

    if (sList.empty()) {
        LE_WARN("GetAvailableSensors: sensor list is empty");
        le_mem_Release(sensorListPtr);
        return NULL;
    }

    taf_SensorInfo_t* sensorInfoPtr;
    for(size_t i=0;i<sList.size();i++)
    {
        sensorInfoPtr = (taf_SensorInfo_t*)le_mem_ForceAlloc(sensorMngr.tSensorInfoPool);
        memset(sensorInfoPtr,0,sizeof(taf_SensorInfo_t));
        sensorInfoPtr->currPtr = NULL;
        sensorInfoPtr->id  = sList[i].basicInfo.id;
        le_utf8_Copy(sensorInfoPtr->name, sList[i].basicInfo.sensorName.c_str(),
            sizeof(sensorInfoPtr->name), NULL);
        le_utf8_Copy(sensorInfoPtr->vendor, sList[i].basicInfo.vendorName.c_str(),
            sizeof(sensorInfoPtr->vendor), NULL);
        size_t rateCount = sList[i].configInfo.samplingRateList.size();
        if (rateCount > TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE)
        {
            rateCount = TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE;
        }
        for(size_t j=0; j<rateCount; j++){
            sensorInfoPtr->samplingRate[j] = sList[i].configInfo.samplingRateList[j];
        }
        ConvertSensorType(sensorInfoPtr,sList[i].basicInfo);
        sensorInfoPtr->sampleRateListSize = (uint32_t)rateCount;
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

le_result_t taf_Sensor::DeleteSensorList(taf_imuSensor_SensorListRef_t sensorListRef)
{
    TAF_ERROR_IF_RET_VAL(sensorListRef == NULL, LE_BAD_PARAMETER, "Null reference(sensorListRef)");

    taf_SensorInfoList_t* listPtr =
        (taf_SensorInfoList_t*)le_ref_Lookup(tSensorListMap, sensorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");

    LE_DEBUG("DeleteSensorList : %p", sensorListRef);

    taf_SensorInfo_t* tSensorPtr;
    le_sls_Link_t* tSensorLinkPtr;

    while ((tSensorLinkPtr = le_sls_Pop(&(listPtr->SensorsList))) != NULL) {
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
    TAF_ERROR_IF_RET_VAL(sensorIdPtr == NULL, LE_BAD_PARAMETER, "sensorIdPtr is NULL");
    *sensorIdPtr = sensorPtr->id;
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorName(taf_imuSensor_SensorRef_t sensorRef,char* sensorName,
    size_t sensorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(sensorName == NULL || sensorNameSize == 0,
                        LE_BAD_PARAMETER, "Invalid sensorName buffer");

    le_utf8_Copy(sensorName, sensorPtr->name, sensorNameSize, NULL);

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVendorName(taf_imuSensor_SensorRef_t sensorRef,
    char* sensorVendorName,size_t sensorVendorNameSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(sensorVendorName == NULL || sensorVendorNameSize == 0,
                         LE_BAD_PARAMETER, "Invalid vendorName buffer");

    le_utf8_Copy(sensorVendorName, sensorPtr->vendor, sensorVendorNameSize, NULL);

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorVersion(taf_imuSensor_SensorRef_t sensorRef,char* version,
    size_t versionSize)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(version == NULL || versionSize == 0,
                         LE_BAD_PARAMETER, "Invalid version buffer");

    snprintf(version, versionSize, "%d", sensorPtr->version);

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorType(taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_SensorType_t* sensorTypePtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);
    TAF_ERROR_IF_RET_VAL(sensorTypePtr == NULL, LE_BAD_PARAMETER, "sensorTypePtr is NULL");
    *sensorTypePtr = sensorPtr->sensorType;
    return LE_OK;
}

le_result_t taf_Sensor::GetSensorSamplingRateInfo(taf_imuSensor_SensorRef_t sensorRef,
    double* samplingRatesListPtr, size_t* samplingRatesListSizePtr)
{
     taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);
    TAF_ERROR_IF_RET_VAL(samplingRatesListPtr == NULL, LE_BAD_PARAMETER,
        "samplingRatesListPtr is NULL");
    TAF_ERROR_IF_RET_VAL(samplingRatesListSizePtr == NULL, LE_BAD_PARAMETER,
        "samplingRatesListSizePtr is NULL");

    size_t count = sensorPtr->sampleRateListSize;
    if (count > TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE)
    {
        count = TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE;
    }

    size_t j=0;
    for(size_t i=0; (i<count) && (i<*samplingRatesListSizePtr); i++){
        if(sensorPtr->samplingRate[i] <= sensorPtr->maxSamplingRate){
            samplingRatesListPtr[j++] = sensorPtr->samplingRate[i];
        }
    }
    *samplingRatesListSizePtr = j;
    if (j == 0)
    {
        LE_ERROR("GetSensorSamplingRateInfo: no valid sampling rates found for sensorRef %p",
            sensorRef);
    }
    return (j > 0) ? LE_OK : LE_FAULT;
}

le_result_t taf_Sensor::GetSensorBatchingInfo(taf_imuSensor_SensorRef_t sensorRef,
    uint32_t* maxBatchCountSupportedPtr,uint32_t* minBatchCountSupportedPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(maxBatchCountSupportedPtr == NULL, LE_BAD_PARAMETER,
        "maxBatchCountSupportedPtr is NULL");
    TAF_ERROR_IF_RET_VAL(minBatchCountSupportedPtr == NULL, LE_BAD_PARAMETER,
        "minBatchCountSupportedPtr is NULL");

    *maxBatchCountSupportedPtr = sensorPtr->maxBatchCountSupported;
    *minBatchCountSupportedPtr = sensorPtr->minBatchCountSupported;

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorRangeInfo(taf_imuSensor_SensorRef_t sensorRef, double* rangePtr)
{

    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,
        LE_FAULT,"Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(rangePtr == NULL, LE_BAD_PARAMETER, "rangePtr is NULL");
    *rangePtr = sensorPtr->range;

    return LE_OK;
}

le_result_t taf_Sensor::GetSensorResolution(taf_imuSensor_SensorRef_t sensorRef,
    double* sensorResolutionPtr)
{
    taf_SensorInfo_t* sensorPtr =
        (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);

    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",sensorPtr);

    TAF_ERROR_IF_RET_VAL(sensorResolutionPtr == NULL, LE_BAD_PARAMETER,
        "sensorResolutionPtr is NULL");
    *sensorResolutionPtr = sensorPtr->resolution;
    return LE_OK;
}

static void ActivateSvcRespond(void* cmdPtr, void*)
{
    auto* cPtr = (SensorCmdInfo_t*)cmdPtr;

    if (cPtr->changesActivation && cPtr->retCode == LE_OK)
    {
        taf_SensorClient_t* client = taf_Sensor::DiscoverSessionRef(cPtr->sessionRef);
        if (client)
        {
            for (uint32_t i = 0; i < client->clientCount; ++i)
            {
                auto* ci = &client->clients[i];
                if (ci->sensorClient == cPtr->paClientId)
                {
                    ci->isSensorActivated = cPtr->desiredActiveState;
                    break;
                }
            }
        }
    }

    taf_imuSensor_ActivateRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_Sensor::ActivateWorker(void* cmdPtr, void*)
{
    auto& sens = taf_Sensor::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    auto* cPtr = (SensorCmdInfo_t*)cmdPtr;

    cPtr->retCode = LE_OK;
    if (tafpa::sensor::taf_pa_sensor_SetConfig(cPtr->paClientId,
                                              cPtr->activate.samplingRate,
                                              cPtr->activate.batchCount) != PA_OK)
    {
        LE_ERROR("ActivateWorker: PA set config failed for paClientId %" PRIu64
            " (rate=%lf, batch=%u)", cPtr->paClientId,
            cPtr->activate.samplingRate, cPtr->activate.batchCount);
        cPtr->retCode = LE_FAULT;
    }
    else if (tafpa::sensor::taf_pa_sensor_Activate(cPtr->paClientId) != PA_OK)
    {
        LE_ERROR("ActivateWorker: PA activate failed for paClientId %" PRIu64,
            cPtr->paClientId);
        cPtr->retCode = LE_FAULT;
    }
    else
    {
        LE_INFO("ActivateWorker ... ");
    }

    le_event_QueueFunctionToThread(sens.SensorSvcThRef, ActivateSvcRespond, cPtr, NULL);
}

void taf_Sensor::Activate(taf_imuSensor_ServerCmdRef_t cmdRef,
                             taf_imuSensor_SensorRef_t sensorRef,
                             double samplingRate, uint32_t batchCount,
                             le_msg_SessionRef_t sessionRef)
{
    tafpa::sensor::taf_pa_sensor_SensorId paId = 0;
    if (!GetPaClientId(sessionRef, sensorRef, &paId))
    {
        LE_ERROR("StartActivate: sensor ref %p not found for session %p", sensorRef, sessionRef);
        taf_imuSensor_ActivateRespond(cmdRef, LE_NOT_FOUND);
        return;
    }

    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    if (!sensorPtr)
    {
        LE_ERROR("StartActivate: invalid sensorRef %p", sensorRef);
        taf_imuSensor_ActivateRespond(cmdRef, LE_NOT_FOUND);
        return;
    }

    auto isValidInputLocal = [sensorPtr](double rate, uint32_t bc) -> bool {
        const double eps = 1e-6;
        uint32_t n = sensorPtr->sampleRateListSize;
        uint32_t maxN =
            (uint32_t)(sizeof(sensorPtr->samplingRate)/sizeof(sensorPtr->samplingRate[0]));
        if (n > maxN) n = maxN;
        bool ok = false;
        for (uint32_t i=0;i<n;i++){
            if (std::fabs(sensorPtr->samplingRate[i] - rate) < eps) { ok = true; break; }
        }
        if (!ok) return false;
        if (bc > sensorPtr->maxBatchCountSupported ||
            bc < sensorPtr->minBatchCountSupported ||
            (bc % 10) != 0) return false;
        return true;
    };

    if (!isValidInputLocal(samplingRate, batchCount))
    {
        LE_ERROR("StartActivate: invalid samplingRate %lf or batchCount %u for sensorRef %p",
            samplingRate, batchCount, sensorRef);
        taf_imuSensor_ActivateRespond(cmdRef, LE_UNSUPPORTED);
        return;
    }

    taf_SensorClient_t* client = AcquireSessionRef(sessionRef);
    if (!client)
    {
        LE_ERROR("StartActivate: failed to acquire session %p", sessionRef);
        taf_imuSensor_ActivateRespond(cmdRef, LE_FAULT);
        return;
    }

    LE_INFO("Activate: sensorRef %p, sensorClientPtr->sessionRef %p, num of active client %d",
            sensorRef, client->sessionRef, mClientRefCount);

    for (uint32_t i = 0; i < client->clientCount; ++i)
    {
        auto* ci = &client->clients[i];
        if (ci->sensorClient == paId)
        {
            if (ci->isSensorActivated)
            {
                LE_WARN("StartActivate: sensor ref %p is already activated", sensorRef);
                taf_imuSensor_ActivateRespond(cmdRef, LE_UNAVAILABLE);
                return;
            }
            break;
        }
    }

    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)le_mem_ForceAlloc(CmdSensorPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = sessionRef;
    cPtr->sensorRef = sensorRef;
    cPtr->paClientId = paId;
    cPtr->activate.samplingRate = samplingRate;
    cPtr->activate.batchCount = batchCount;
    cPtr->changesActivation = true;
    cPtr->desiredActiveState = true;

    le_event_QueueFunctionToThread(SensorWorkerThRef, ActivateWorker, cPtr, NULL);
}

static void SelfTestSvcRespond(void* cmdPtr, void*)
{
    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)cmdPtr;

    taf_imuSensor_SelfTestRespond(cPtr->cmdRef, cPtr->retCode, cPtr->selfTest.timestamp);
    le_mem_Release(cPtr);
}

void taf_Sensor::SelfTestWorker(void* cmdPtr, void*)
{
    auto& sens = taf_Sensor::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)cmdPtr;

    tafpa::sensor::taf_pa_sensor_SelfTestMode type =
        (cPtr->selfTest.mode == TAF_IMUSENSOR_POSITIVE) ? tafpa::sensor::taf_pa_sensor_SelfTestMode::POSITIVE :
        (cPtr->selfTest.mode == TAF_IMUSENSOR_NEGATIVE) ? tafpa::sensor::taf_pa_sensor_SelfTestMode::NEGATIVE :
                                                          tafpa::sensor::taf_pa_sensor_SelfTestMode::BOTH;

    auto promiseptr = std::make_shared<std::promise<le_result_t>>();
    auto timestampPtr = std::make_shared<uint64_t>(0);
    le_msg_SessionRef_t sref = cPtr->sessionRef;

    auto cb = [sref, timestampPtr, promiseptr](
                  tafpa::sensor::taf_pa_sensor_SensorId,
                  pa_result_t result, uint64_t ts, std::any context)
    {
        try
        {
            le_msg_SessionRef_t session = std::any_cast<le_msg_SessionRef_t>(context);
            if (session != sref) {
                promiseptr->set_value(LE_FAULT);
                return;
            }

            *timestampPtr = ts;
            le_result_t rc = (result == PA_OK) ? LE_OK :
                             (result == PA_BUSY) ? LE_BUSY : LE_FAULT;
            promiseptr->set_value(rc);
        }
        catch (...)
        {
            LE_ERROR("Unknown error in self test callback.");
            promiseptr->set_value(LE_FAULT);
        }
    };

    pa_result_t res =
        tafpa::sensor::taf_pa_sensor_SelfTestAsync(cPtr->paClientId, type, cb, std::any(sref));
    if (res != PA_OK)
    {
        LE_ERROR("SelfTestWorker: PA self-test failed for paClientId %" PRIu64, cPtr->paClientId);
        cPtr->retCode = LE_FAULT;
        cPtr->selfTest.timestamp = 0;
        le_event_QueueFunctionToThread(sens.SensorSvcThRef, SelfTestSvcRespond, cPtr, NULL);
        return;
    }

    auto fut = promiseptr->get_future();
    if (fut.wait_for(std::chrono::seconds(MAX_TIME_OUT)) == std::future_status::ready)
    {
        cPtr->retCode = fut.get();
        cPtr->selfTest.timestamp = *timestampPtr;
    }
    else
    {
        LE_ERROR("SelfTestWorker: self-test timed out for paClientId %" PRIu64, cPtr->paClientId);
        cPtr->retCode = LE_TIMEOUT;
        cPtr->selfTest.timestamp = 0;
    }

    le_event_QueueFunctionToThread(sens.SensorSvcThRef, SelfTestSvcRespond, cPtr, NULL);
}

void taf_Sensor::SelfTest(taf_imuSensor_ServerCmdRef_t cmdRef,
                              taf_imuSensor_SensorRef_t sensorRef,
                              taf_imuSensor_SelfTestMode_t mode,
                              le_msg_SessionRef_t sessionRef)
{
    tafpa::sensor::taf_pa_sensor_SensorId paId = 0;
    if (!GetPaClientId(sessionRef, sensorRef, &paId))
    {
        LE_ERROR("StartSelfTest: sensor ref %p not found for session %p", sensorRef, sessionRef);
        taf_imuSensor_SelfTestRespond(cmdRef, LE_NOT_FOUND, 0);
        return;
    }

    // validate mode on main thread
    if (mode != TAF_IMUSENSOR_POSITIVE &&
        mode != TAF_IMUSENSOR_NEGATIVE &&
        mode != TAF_IMUSENSOR_BOTH)
    {
        LE_ERROR("StartSelfTest: invalid self-test mode %d for sensorRef %p", (int)mode, sensorRef);
        taf_imuSensor_SelfTestRespond(cmdRef, LE_BAD_PARAMETER, 0);
        return;
    }

    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)le_mem_ForceAlloc(CmdSensorPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = sessionRef;
    cPtr->sensorRef = sensorRef;
    cPtr->paClientId = paId;
    cPtr->selfTest.mode = mode;
    cPtr->selfTest.timestamp = 0;

    le_event_QueueFunctionToThread(SensorWorkerThRef, SelfTestWorker, cPtr, NULL);
}

void taf_Sensor::SelfTestNotifyClient(void* reportPtr,void* secondLayerHandlerFunc){
    taf_SensorSelfTest_t* evtPtr = (taf_SensorSelfTest_t*) reportPtr;
    TAF_ERROR_IF_RET_NIL(evtPtr == NULL,"evtptr is NULL");
    taf_imuSensor_SelfTestFailedHandlerFunc_t clientHandlerFunc =
        (taf_imuSensor_SelfTestFailedHandlerFunc_t)secondLayerHandlerFunc;
        taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = DiscoverSessionRef(evtPtr->sessionRef);
    TAF_ERROR_IF_RET_NIL(clientRequestPtr == NULL,"clientRequestPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == NULL, "clientHandlerFunc is NULL");
    for(uint32_t i = 0; i < clientRequestPtr->clientCount; ++i){
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if(clientInfoPtr->sensorClient == evtPtr->sensorClientId)
        {

            clientHandlerFunc(NULL, clientInfoPtr->sensorRef, evtPtr->timestamp,
                                                        le_event_GetContextPtr());

            break;
        }
    }
}

taf_imuSensor_SelfTestFailedHandlerRef_t taf_Sensor::AddSelfTestFailedHandler
    (taf_imuSensor_SensorRef_t sensorRef,taf_imuSensor_SelfTestFailedHandlerFunc_t handlerPtr,
    void* contextPtr){

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");
    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef(taf_imuSensor_GetClientSessionRef());
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,NULL,
            "clientRequestPtr is NULL, Client count reach max");

    LE_INFO("Start: sensorClientPtr %p, sensorClientPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);
    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,NULL,"Invalid reference (%p) provided!",sensorPtr);

    for(uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if(strcmp(sensorPtr->name,clientInfoPtr->sensorName)==0){
            clientInfoPtr->sensorRef = sensorRef;

            le_event_HandlerRef_t handlerRef;
            handlerRef = le_event_AddLayeredHandler("SelfTestHandler",
                            SelfTestEventId, SelfTestNotifyClient, (void*)handlerPtr);
            if (handlerRef == NULL)
            {
                LE_ERROR("AddSelfTestFailedHandler: failed to create layered handler");
                return NULL;
            }
            le_event_SetContextPtr(handlerRef, contextPtr);
            return (taf_imuSensor_SelfTestFailedHandlerRef_t)handlerRef;
        }
    }
    LE_ERROR("Client not found for SensorRef %p in session %p",
        sensorRef,clientRequestPtr->sessionRef);
    return NULL;
}

void taf_Sensor::RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t handlerRef)
{
    if (handlerRef == NULL)
    {
        LE_WARN("RemoveSelfTestFailedHandler: handlerRef is NULL");
        return;
    }

    LE_DEBUG("RemoveSelfTestFailedHandler: handlerRef %p", handlerRef);
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

// -------------------------------------------------------------------------------------------------
// ConfigUpdate event dispatch: runs on the service main thread.
// Iterates all registered ConfigUpdate handlers and calls the matching one.
// -------------------------------------------------------------------------------------------------
void taf_Sensor::ConfigUpdateNotifyClient(void* reportPtr, void* secondLayerHandlerFunc)
{
    taf_SensorConfigUpdate_t* evtPtr = (taf_SensorConfigUpdate_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(evtPtr == NULL, "ConfigUpdateNotifyClient: evtPtr is NULL");

    LE_INFO("ConfigUpdateNotifyClient: Event ...");
    taf_imuSensor_ConfigUpdateHandlerFunc_t clientHandlerFunc =
        (taf_imuSensor_ConfigUpdateHandlerFunc_t)secondLayerHandlerFunc;
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == NULL, "ConfigUpdateNotifyClient: handler is NULL");

    taf_SensorClient_t* clientRequestPtr = DiscoverSessionRef(evtPtr->sessionRef);
    TAF_ERROR_IF_RET_NIL(clientRequestPtr == NULL,
        "ConfigUpdateNotifyClient: clientRequestPtr is NULL");

    for (uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if (clientInfoPtr->sensorClient == evtPtr->sensorClientId)
        {
            clientHandlerFunc(clientInfoPtr->sensorRef,
                              evtPtr->samplingRate,
                              evtPtr->batchCount,
                              evtPtr->isRotated,
                              le_event_GetContextPtr());
            break;
        }
    }
}

taf_imuSensor_ConfigUpdateHandlerRef_t taf_Sensor::AddConfigUpdateHandler(
    taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_ConfigUpdateHandlerFunc_t handlerPtr,
    void* contextPtr)
{
    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL,
        "AddConfigUpdateHandler: handlerPtr is NULL");

    taf_SensorClient_t* clientRequestPtr = AcquireSessionRef(taf_imuSensor_GetClientSessionRef());
    TAF_ERROR_IF_RET_VAL(clientRequestPtr == NULL, NULL,
        "AddConfigUpdateHandler: clientRequestPtr is NULL");

    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, NULL,
        "AddConfigUpdateHandler: invalid sensorRef (%p)", sensorRef);

    for (uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if (strcmp(sensorPtr->name, clientInfoPtr->sensorName) == 0)
        {
            clientInfoPtr->sensorRef = sensorRef;

            // Register the PA-level config update callback for this sensor client
            auto cb = [](tafpa::sensor::taf_pa_sensor_SensorId sid,
                         double sr, uint32_t bc, bool rot, std::any ctx)
            {
                Handler::onConfigUpdate(sid, sr, bc, rot, ctx);
            };
            pa_result_t paRes = tafpa::sensor::taf_pa_sensor_AddConfigUpdateHandler(
                clientInfoPtr->sensorClient, cb,
                std::any(clientRequestPtr->sessionRef));
            if (paRes != PA_OK)
            {
                LE_ERROR("AddConfigUpdateHandler: PA registration failed for sensor %s",
                    clientInfoPtr->sensorName);
                return NULL;
            }

            le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                "ConfigUpdateHandler",
                ConfigUpdateEventId,
                ConfigUpdateNotifyClient,
                (void*)handlerPtr);
            if (handlerRef == NULL)
            {
                LE_ERROR("AddConfigUpdateHandler: failed to create layered handler");
                return NULL;
            }
            le_event_SetContextPtr(handlerRef, contextPtr);

            // Track handler for session cleanup in CloseEventHandler
            taf_SensorConfigUpdateHandler_t* handlerInfoPtr =
                (taf_SensorConfigUpdateHandler_t*)le_mem_ForceAlloc(
                    tSensorConfigUpdateHandlerPool);
            memset(handlerInfoPtr, 0, sizeof(taf_SensorConfigUpdateHandler_t));
            handlerInfoPtr->sensorRef  = sensorRef;
            handlerInfoPtr->handlerRef = (taf_imuSensor_ConfigUpdateHandlerRef_t)handlerRef;
            handlerInfoPtr->sessionRef = clientRequestPtr->sessionRef;
            le_ref_CreateRef(tSensorConfigUpdateHandlerMap, handlerInfoPtr);

            LE_INFO("AddConfigUpdateHandler: registered for sensor %s, session %p",
                clientInfoPtr->sensorName, clientRequestPtr->sessionRef);
            return (taf_imuSensor_ConfigUpdateHandlerRef_t)handlerRef;
        }
    }

    LE_ERROR("AddConfigUpdateHandler: sensor %p not found in session %p",
        sensorRef, clientRequestPtr->sessionRef);
    return NULL;
}

void taf_Sensor::RemoveConfigUpdateHandler(
    taf_imuSensor_ConfigUpdateHandlerRef_t handlerRef)
{
    if (handlerRef == NULL)
    {
        LE_WARN("RemoveConfigUpdateHandler: handlerRef is NULL");
        return;
    }
    LE_DEBUG("RemoveConfigUpdateHandler: handlerRef %p", handlerRef);

    // Remove tracking entry from map
    le_ref_IterRef_t iterRef = le_ref_GetIterator(tSensorConfigUpdateHandlerMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        auto* h = (taf_SensorConfigUpdateHandler_t*)le_ref_GetValue(iterRef);
        if (h && h->handlerRef == handlerRef)
        {
            void* safeRef = (void*)le_ref_GetSafeRef(iterRef);
            le_ref_DeleteRef(tSensorConfigUpdateHandlerMap, safeRef);
            le_mem_Release(h);
            break;
        }
    }

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

// -------------------------------------------------------------------------------------------------
// Capability event dispatch: runs on the service main thread.
// Iterates all registered Capability handlers and calls the matching one.
// -------------------------------------------------------------------------------------------------
void taf_Sensor::CapabilityNotifyClient(void* reportPtr, void* secondLayerHandlerFunc)
{
    taf_SensorCapability_t* evtPtr = (taf_SensorCapability_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(evtPtr == NULL, "CapabilityNotifyClient: evtPtr is NULL");

    LE_INFO("CapabilityNotifyClient: Event ...");

    taf_imuSensor_CapabilityUpdateHandlerFunc_t clientHandlerFunc =
        (taf_imuSensor_CapabilityUpdateHandlerFunc_t)secondLayerHandlerFunc;
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == NULL,
        "CapabilityNotifyClient: clientHandlerFunc is NULL");

    taf_SensorClient_t* clientRequestPtr = DiscoverSessionRef(evtPtr->sessionRef);
    TAF_ERROR_IF_RET_NIL(clientRequestPtr == NULL,
        "CapabilityNotifyClient: clientRequestPtr is NULL");

    for (uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if (clientInfoPtr->sensorClient == evtPtr->sensorClientId)
        {
            clientHandlerFunc(clientInfoPtr->sensorRef,
                              evtPtr->isAvailable,
                              evtPtr->isEnabled,
                              evtPtr->capabilityMask,
                              le_event_GetContextPtr());
            break;
        }
    }
}

taf_imuSensor_CapabilityUpdateHandlerRef_t taf_Sensor::AddCapabilityHandler(
    taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_CapabilityUpdateHandlerFunc_t handlerPtr,
    void* contextPtr)
{
    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL,
        "AddCapabilityHandler: handlerPtr is NULL");

    taf_SensorClient_t* clientRequestPtr = AcquireSessionRef(taf_imuSensor_GetClientSessionRef());
    TAF_ERROR_IF_RET_VAL(clientRequestPtr == NULL, NULL,
        "AddCapabilityHandler: clientRequestPtr is NULL");

    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL, NULL,
        "AddCapabilityHandler: invalid sensorRef (%p)", sensorRef);

    for (uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if (strcmp(sensorPtr->name, clientInfoPtr->sensorName) == 0)
        {
            clientInfoPtr->sensorRef = sensorRef;

            le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                "CapabilityUpdateHandler",
                CapabilityEventId,
                CapabilityNotifyClient,
                (void*)handlerPtr);
            if (handlerRef == NULL)
            {
                LE_ERROR("AddCapabilityHandler: failed to create layered handler");
                return NULL;
            }
            le_event_SetContextPtr(handlerRef, contextPtr);

            // Register the PA-level capability callback for this sensor client.
            // Some PA variants do not implement capability update notifications.
            // In that case, keep the handler registration and immediately synthesize
            // one capability event from the static sensor information so the client
            // still receives a valid capability callback.
            auto cb = [](tafpa::sensor::taf_pa_sensor_SensorId sid,
                         tafpa::sensor::taf_pa_sensor_CapabilityInfo capInfo,
                         std::any ctx)
            {
                Handler::onCapabilityUpdate(sid, capInfo, ctx);
            };
            pa_result_t paRes = tafpa::sensor::taf_pa_sensor_AddCapabilityHandler(
                clientInfoPtr->sensorClient, cb,
                std::any(clientRequestPtr->sessionRef));

            if (paRes != PA_OK)
            {
                LE_WARN("AddCapabilityHandler: PA registration unsupported/failed for sensor %s"
                        " (paRes=%d). Falling back to synthetic capability notification.",
                        clientInfoPtr->sensorName, (int)paRes);

                taf_SensorCapability_t event = {};
                event.sensorRef = sensorRef;
                event.isAvailable = true;
                event.isEnabled = clientInfoPtr->isSensorActivated;
                event.capabilityMask = 0;
                event.sessionRef = clientRequestPtr->sessionRef;
                event.sensorClientId = clientInfoPtr->sensorClient;
                le_event_Report(CapabilityEventId, &event, sizeof(event));
            }

            // Track handler for session cleanup in CloseEventHandler
            taf_SensorCapabilityHandler_t* capHandlerInfoPtr =
                (taf_SensorCapabilityHandler_t*)le_mem_ForceAlloc(
                    tSensorCapabilityHandlerPool);
            memset(capHandlerInfoPtr, 0, sizeof(taf_SensorCapabilityHandler_t));
            capHandlerInfoPtr->sensorRef  = sensorRef;
            capHandlerInfoPtr->handlerRef = (taf_imuSensor_CapabilityUpdateHandlerRef_t)handlerRef;
            capHandlerInfoPtr->sessionRef = clientRequestPtr->sessionRef;
            le_ref_CreateRef(tSensorCapabilityHandlerMap, capHandlerInfoPtr);

            LE_INFO("AddCapabilityHandler: registered for sensor %s, session %p",
                clientInfoPtr->sensorName, clientRequestPtr->sessionRef);
            return (taf_imuSensor_CapabilityUpdateHandlerRef_t)handlerRef;
        }
    }

    LE_ERROR("AddCapabilityHandler: sensor %p not found in session %p",
        sensorRef, clientRequestPtr->sessionRef);
    return NULL;
}

void taf_Sensor::RemoveCapabilityHandler(
    taf_imuSensor_CapabilityUpdateHandlerRef_t handlerRef)
{
    if (handlerRef == NULL)
    {
        LE_WARN("RemoveCapabilityHandler: handlerRef is NULL");
        return;
    }
    LE_DEBUG("RemoveCapabilityHandler: handlerRef %p", handlerRef);

    // Remove tracking entry from map
    le_ref_IterRef_t iterRef = le_ref_GetIterator(tSensorCapabilityHandlerMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        auto* h = (taf_SensorCapabilityHandler_t*)le_ref_GetValue(iterRef);
        if (h && h->handlerRef == handlerRef)
        {
            void* safeRef = (void*)le_ref_GetSafeRef(iterRef);
            le_ref_DeleteRef(tSensorCapabilityHandlerMap, safeRef);
            le_mem_Release(h);
            break;
        }
    }

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

static void DeactivateSvcRespond(void* cmdPtr, void*)
{
    auto* cPtr = (SensorCmdInfo_t*)cmdPtr;

    if (cPtr->changesActivation && cPtr->retCode == LE_OK)
    {
        taf_SensorClient_t* client = taf_Sensor::DiscoverSessionRef(cPtr->sessionRef);
        if (client)
        {
            for (uint32_t i = 0; i < client->clientCount; ++i)
            {
                auto* ci = &client->clients[i];
                if (ci->sensorClient == cPtr->paClientId)
                {
                    ci->isSensorActivated = cPtr->desiredActiveState;
                    break;
                }
            }
        }
    }

    taf_imuSensor_DeactivateRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_Sensor::DeactivateWorker(void* cmdPtr, void*)
{
    auto& sens = taf_Sensor::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    auto* cPtr = (SensorCmdInfo_t*)cmdPtr;

    pa_result_t deactRes = tafpa::sensor::taf_pa_sensor_Deactivate(cPtr->paClientId);
    if (deactRes != PA_OK)
    {
        LE_ERROR("DeactivateWorker: deactivate failed for paClientId %" PRIu64, cPtr->paClientId);
    }
    cPtr->retCode = (deactRes == PA_OK) ? LE_OK : LE_FAULT;
    le_event_QueueFunctionToThread(sens.SensorSvcThRef, DeactivateSvcRespond, cPtr, NULL);
}

void taf_Sensor::Deactivate(taf_imuSensor_ServerCmdRef_t cmdRef,
                                taf_imuSensor_SensorRef_t sensorRef,
                                le_msg_SessionRef_t sessionRef)
{
    tafpa::sensor::taf_pa_sensor_SensorId paId = 0;
    if (!GetPaClientId(sessionRef, sensorRef, &paId))
    {
        LE_ERROR("StartDeactivate: sensor ref %p not found for session %p", sensorRef, sessionRef);
        taf_imuSensor_DeactivateRespond(cmdRef, LE_NOT_FOUND);
        return;
    }

    taf_SensorClient_t* client = AcquireSessionRef(sessionRef);
    if (!client)
    {
        LE_ERROR("StartDeactivate: failed to acquire session %p", sessionRef);
        taf_imuSensor_DeactivateRespond(cmdRef, LE_FAULT);
        return;
    }

    LE_INFO("Deactivate: sensorRef %p, sensorClientPtr->sessionRef %p, num of active client %d",
            sensorRef, client->sessionRef, mClientRefCount);

    for (uint32_t i = 0; i < client->clientCount; ++i)
    {
        auto* ci = &client->clients[i];
        if (ci->sensorClient == paId)
        {
            if (!ci->isSensorActivated)
            {
                LE_WARN("StartDeactivate: sensor ref %p is not activated", sensorRef);
                taf_imuSensor_DeactivateRespond(cmdRef, LE_UNAVAILABLE);
                return;
            }
            break;
        }
    }

    SensorCmdInfo_t* cPtr = (SensorCmdInfo_t*)le_mem_ForceAlloc(CmdSensorPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = sessionRef;
    cPtr->sensorRef = sensorRef;
    cPtr->paClientId = paId;
    cPtr->changesActivation = true;
    cPtr->desiredActiveState = false;

    le_event_QueueFunctionToThread(SensorWorkerThRef, DeactivateWorker, cPtr, NULL);
}

taf_imuSensor_DataHandlerRef_t taf_Sensor::AddDataHandler(taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_DataHandlerFunc_t handlerPtr,void* contextPtr)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");

    taf_SensorClient_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef(taf_imuSensor_GetClientSessionRef());
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr,NULL,
                                                "clientRequestPtr is NULL, Client count reach max");

    LE_INFO("AddDataHandler: sensorRef %p, sensorClientPtr->sessionRef %p, num of active client %d",
        sensorRef, clientRequestPtr->sessionRef, mClientRefCount);

    taf_SensorInfo_t* sensorPtr = (taf_SensorInfo_t*)le_ref_Lookup(tSensorInfoMap, sensorRef);
    TAF_ERROR_IF_RET_VAL(sensorPtr == NULL,NULL,"Invalid reference (%p) provided!",sensorPtr);

    for(uint32_t i = 0; i < clientRequestPtr->clientCount; ++i)
    {
        auto* clientInfoPtr = &clientRequestPtr->clients[i];
        if(strcmp(sensorPtr->name,clientInfoPtr->sensorName)==0){
            clientInfoPtr->sensorRef = sensorRef;
            taf_SensorEventHandler_t*  eventHandlerPtr =
            (taf_SensorEventHandler_t*)le_mem_ForceAlloc(tSensorEventHandlerPool);
            memset(eventHandlerPtr, 0, sizeof(taf_SensorEventHandler_t));
            eventHandlerPtr->sensorRef = sensorRef;
            eventHandlerPtr->next = LE_DLS_LINK_INIT;
            eventHandlerPtr->handlerFuncPtr = handlerPtr;
            eventHandlerPtr->handlerContextPtr = contextPtr;
            eventHandlerPtr->sessionRef = taf_imuSensor_GetClientSessionRef();
            eventHandlerPtr->handlerRef =
            (taf_imuSensor_DataHandlerRef_t)le_ref_CreateRef(tSensorEventHandlerMap,eventHandlerPtr);

            return eventHandlerPtr->handlerRef;
        }
    }

    LE_ERROR("Client not found for SensorRef %p in session %p",
        sensorRef,clientRequestPtr->sessionRef);
    return NULL;
}

void taf_Sensor::RemoveDataHandler(taf_imuSensor_DataHandlerRef_t handlerRef){
    taf_SensorEventHandler_t* evtHandlerPtr =
        (taf_SensorEventHandler_t*)le_ref_Lookup(tSensorEventHandlerMap,handlerRef);
    if(evtHandlerPtr != NULL)
    {
        if(evtHandlerPtr->handlerRef != handlerRef){
            LE_INFO("Handler ref %p not found in map",handlerRef);
            return ;
        }

        LE_DEBUG("Removed evtHandlerRef(%p) for evtHandlerPtr(%p).",
                                    evtHandlerPtr->handlerRef, evtHandlerPtr);

        le_ref_DeleteRef(tSensorEventHandlerMap,handlerRef);
        le_mem_Release(evtHandlerPtr);
    }
    else
    {
        LE_INFO("eventHandlerPtr is null");
    }
}

le_result_t taf_Sensor::GetData( taf_imuSensor_SampleRef_t eventList,taf_imuSensor_DataValue_t*
    RawData,size_t* RawDataSizePtr,taf_imuSensor_DataValue_t*BiasData,size_t* BiasDataSizePtr)
{
    taf_SensorEventInfo_t* ptr =
    (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventList);

    TAF_ERROR_IF_RET_VAL(ptr == NULL || ptr->eventPtr == NULL, LE_NOT_FOUND,
        "Invalid reference (%p) provided!", ptr);

    TAF_ERROR_IF_RET_VAL(RawData == NULL || BiasData == NULL ||
        RawDataSizePtr == NULL || BiasDataSizePtr == NULL,
        LE_BAD_PARAMETER, "Output buffers or sizes are NULL");

    if(*RawDataSizePtr < ptr->eventPtr->listSize || *BiasDataSizePtr < ptr->eventPtr->listSize){
        LE_ERROR("Output array size is less then total no of events");
        return LE_BAD_PARAMETER;
    }

    if(*RawDataSizePtr != *BiasDataSizePtr){
        LE_ERROR("Bias Size ptr not same as raw size ptr.");
        return LE_BAD_PARAMETER;
    }
    size_t j = 0;
    for (uint32_t i = 0; i < ptr->eventPtr->listSize; ++i)
    {
        auto& eventData = ptr->eventPtr->eventList[i];
        RawData[j].timestamp = eventData.timestamp;
        RawData[j].x = eventData.x;
        RawData[j].y = eventData.y;
        RawData[j].z = eventData.z;
        BiasData[j].timestamp = eventData.timestamp;
        BiasData[j].x = eventData.xb;
        BiasData[j].y = eventData.yb;
        BiasData[j].z = eventData.zb;
        j++;

        if (j == ptr->eventPtr->listSize){
            break;
        }

        if (j > *RawDataSizePtr || j > *BiasDataSizePtr) {
            LE_ERROR("Output buffers too small, Raw: %zu, Bias: %zu, request: %u",
                            *RawDataSizePtr, *BiasDataSizePtr, ptr->eventPtr->listSize);
            break;
        }
    }
    *RawDataSizePtr = j;
    *BiasDataSizePtr = j;
    return LE_OK;
}

le_result_t taf_Sensor::DeleteData(taf_imuSensor_SampleRef_t eventListRef){
    TAF_ERROR_IF_RET_VAL(eventListRef == NULL,LE_BAD_PARAMETER,"Null reference(eventListRef)");
    taf_SensorEventInfo_t* listPtr =
        (taf_SensorEventInfo_t*)le_ref_Lookup(tSensorEventMap,eventListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
    TAF_ERROR_IF_RET_VAL(listPtr->eventPtr == NULL, LE_BAD_PARAMETER, "eventPtr is NULL");
    le_ref_DeleteRef(tSensorEventMap, eventListRef);
    le_mem_Release(listPtr->eventPtr);
    le_mem_Release(listPtr);
    return LE_OK;
}

taf_SensorClient_t* taf_Sensor::DiscoverSessionRef(le_msg_SessionRef_t sessionRef)
{
    auto& sensorMngr = taf_Sensor::GetInstance();

    // Enforce single-thread ownership for this function to avoid race condition issue.
    // Here this function is design ONLY for main thread to use.
    if (sensorMngr.SensorSvcThRef &&
        le_thread_GetCurrent() != sensorMngr.SensorSvcThRef)
    {
        LE_ERROR("DiscoverSessionRef called from non-service thread. "
                 "current=%p svc=%p", le_thread_GetCurrent(), sensorMngr.SensorSvcThRef);
        return NULL;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SensorClient_t* clientPtr =
            (taf_SensorClient_t*)le_ref_GetValue(iterRef);

        if (!clientPtr)
        {
            LE_ERROR("ClientPtr is Null");
            continue;
        }

        if (sessionRef == clientPtr->sessionRef)
        {
            return clientPtr;
        }
    }
    return NULL;
}

void taf_Sensor::CleanUp(taf_SensorClient_t* clientPtr){
    if(clientPtr == NULL) {
        LE_ERROR("ClientPtr is Null");
        return;
    }
    for(uint32_t i = 0; i < clientPtr->clientCount; ++i){
        auto* ci = &clientPtr->clients[i];
        if(ci->isSensorActivated == true){
            if(tafpa::sensor::taf_pa_sensor_Deactivate(ci->sensorClient) != PA_OK){
                LE_ERROR("Unable to deactivate sensor for client %lu", ci->sensorClient);
            }
            ci->isSensorActivated = false;
        }
        if(tafpa::sensor::taf_pa_sensor_RemoveListener(ci->sensorClient) != PA_OK){
            LE_ERROR("Unable to remove listener for client %lu", ci->sensorClient);
        }
        if(tafpa::sensor::taf_pa_sensor_ReleaseSensorClient(ci->sensorClient) != PA_OK){
            LE_ERROR("Unable to delete reference for client %lu", ci->sensorClient);
        }
    }
    clientPtr->clientCount = 0;
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
    LE_UNUSED(contextPtr);
    LE_INFO("CloseEventHandler: sessionRef %p", sessionRef);
    auto& sensorMngr = taf_Sensor::GetInstance();

    // 1) Collect and delete sensor lists for this session.
    std::vector<taf_imuSensor_SensorListRef_t> listRefs;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorListMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* listPtr = (taf_SensorInfoList_t*)le_ref_GetValue(iterRef);
            if (listPtr && listPtr->sessionRef == sessionRef)
            {
                listRefs.push_back(listPtr->ref);
            }
        }
    }
    for (auto ref : listRefs)
    {
        (void)sensorMngr.DeleteSensorList(ref);
    }

    // 2) Collect and delete samples for this session.
    std::vector<taf_imuSensor_SampleRef_t> sampleRefs;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorEventMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* dataEventPtr = (taf_SensorEventInfo_t*)le_ref_GetValue(iterRef);
            if (!dataEventPtr || !dataEventPtr->eventPtr)
            {
                continue;
            }
            if (dataEventPtr->sessionRef == sessionRef)
            {
                auto safeRef = (taf_imuSensor_SampleRef_t)le_ref_GetSafeRef(iterRef);
                sampleRefs.push_back(safeRef);
            }
        }
    }
    for (auto ref : sampleRefs)
    {
        (void)sensorMngr.DeleteData(ref);
    }

    // 3) Collect and remove data handlers for this session.
    std::vector<taf_imuSensor_DataHandlerRef_t> handlerRefs;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorEventHandlerMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* h = (taf_SensorEventHandler_t*)le_ref_GetValue(iterRef);
            if (h && h->sessionRef == sessionRef)
            {
                auto safeRef = (taf_imuSensor_DataHandlerRef_t)le_ref_GetSafeRef(iterRef);
                handlerRefs.push_back(safeRef);
            }
        }
    }
    for (auto ref : handlerRefs)
    {
        sensorMngr.RemoveDataHandler(ref);
    }

    // 4) Collect and remove ConfigUpdate handlers for this session.
    std::vector<taf_imuSensor_ConfigUpdateHandlerRef_t> configUpdateRefs;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorConfigUpdateHandlerMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* h = (taf_SensorConfigUpdateHandler_t*)le_ref_GetValue(iterRef);
            if (h && h->sessionRef == sessionRef)
            {
                configUpdateRefs.push_back(h->handlerRef);
            }
        }
    }
    for (auto ref : configUpdateRefs)
    {
        sensorMngr.RemoveConfigUpdateHandler(ref);
    }

    // 5) Collect and remove Capability handlers for this session.
    std::vector<taf_imuSensor_CapabilityUpdateHandlerRef_t> capabilityRefs;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.tSensorCapabilityHandlerMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* h = (taf_SensorCapabilityHandler_t*)le_ref_GetValue(iterRef);
            if (h && h->sessionRef == sessionRef)
            {
                capabilityRefs.push_back(h->handlerRef);
            }
        }
    }
    for (auto ref : capabilityRefs)
    {
        sensorMngr.RemoveCapabilityHandler(ref);
    }

    // 6) Remove client session entry.
    void* clientSafeRefPtr = NULL;
    taf_SensorClient_t* clientPtr = NULL;
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(sensorMngr.ClientRequestRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            auto* p = (taf_SensorClient_t*)le_ref_GetValue(iterRef);
            if (p && p->sessionRef == sessionRef)
            {
                clientPtr = p;
                clientSafeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
                break;
            }
        }
    }

    if (clientPtr && clientSafeRefPtr)
    {
        if (sensorMngr.mClientRefCount > 0)
        {
            // External client release, decrement client ref count.
            sensorMngr.mClientRefCount--;
        }
        sensorMngr.CleanUp(clientPtr);
        sensorMngr.ReleaseClientRef(clientSafeRefPtr);
    }
}

taf_Sensor::~taf_Sensor()
{
    LE_INFO("~taf_Sensor");
}

le_result_t taf_Sensor::GetSensorList(int8_t listSize){
    for(int8_t i=0;i<listSize;i++){
        taf_SensorPAInfo_t info;
        pa_result_t res =
            tafpa::sensor::taf_pa_sensor_GetSensorInfo(i,info.basicInfo,info.configInfo,info.capInfo);
        if(res != PA_OK){
            LE_ERROR("unable to get sensor info for index %d",i);
            return LE_FAULT;
        }
        sList.push_back(info);
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main function for worker thread.
 */
//--------------------------------------------------------------------------------------------------
static void* SensorWorkerThread(void* context)
{
    LE_UNUSED(context);

    LE_INFO("Worker thread is now running.");
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Termination handler.
 */
//--------------------------------------------------------------------------------------------------
static void SensorTerminateHandler(int sigNum)
{
    LE_UNUSED(sigNum);

    auto& sensorMngr = taf_Sensor::GetInstance();
    LE_INFO("Terminating SensorSvc ...");

    if (sensorMngr.SensorWorkerThRef != NULL)
    {
        le_thread_Cancel(sensorMngr.SensorWorkerThRef);
        le_thread_Join(sensorMngr.SensorWorkerThRef, NULL);
    }
    exit(EXIT_SUCCESS);
}

void taf_Sensor::Init()
{
    LE_INFO("** Init Started **");
    mClientRefCount = 0;

    int8_t listSize=0;
    if(tafpa::sensor::taf_pa_sensor_Init(listSize) != LE_OK){
        LE_FATAL("Sensor Platform unable to initialize");
        return;
    }
    if(GetSensorList(listSize) != LE_OK){
        LE_FATAL("Unable to get SensorInfo");
        return;
    }
    auto& sensorMngr = taf_Sensor::GetInstance();
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
    tSensorConfigUpdateHandlerPool = le_mem_InitStaticPool(tSensorConfigUpdateHandlerPool,
        SENSOR_EVENT_HANDLER_HIGH, sizeof(taf_SensorConfigUpdateHandler_t));
    tSensorCapabilityHandlerPool = le_mem_InitStaticPool(tSensorCapabilityHandlerPool,
        SENSOR_EVENT_HANDLER_HIGH, sizeof(taf_SensorCapabilityHandler_t));
    CmdSensorPoolRef = le_mem_CreatePool("CmdSensorPoolRef", sizeof(SensorCmdInfo_t));
    tSensorEventMap = le_ref_CreateMap("tSensorEventMap",TAF_SENSOR_MAX_EVENTS_SIZE);
    tSensorEventHandlerMap = le_ref_CreateMap("EventHandlerRefMap",SENSOR_EVENT_HANDLER_HIGH);
    tSensorConfigUpdateHandlerMap = le_ref_CreateMap("ConfigUpdateHandlerMap",
        SENSOR_EVENT_HANDLER_HIGH);
    tSensorCapabilityHandlerMap = le_ref_CreateMap("CapabilityHandlerMap",
        SENSOR_EVENT_HANDLER_HIGH);
    sensorMngr.SensorOnEventId = le_event_CreateIdWithRefCounting("sensorOnEventId");
    sensorMngr.SelfTestEventId = le_event_CreateId("SelfTestEventId",sizeof(taf_SensorSelfTest_t));
    sensorMngr.ConfigUpdateEventId = le_event_CreateId("ConfigUpdateEventId",
        sizeof(taf_SensorConfigUpdate_t));
    sensorMngr.CapabilityEventId = le_event_CreateId("CapabilityEventId",
        sizeof(taf_SensorCapability_t));
    sensorMngr.HandlerRef = le_event_AddHandler("OnEventHandler",
            sensorMngr.SensorOnEventId, taf_Sensor::DataEventHandler);

    SensorSvcThRef = le_thread_GetCurrent();
    SensorWorkerThRef = le_thread_Create("SensorWorkerThread", SensorWorkerThread, NULL);
    if (SensorWorkerThRef == NULL)
    {
        LE_FATAL("Init: failed to create SensorWorkerThread");
        return;
    }

    le_thread_SetJoinable(SensorWorkerThRef);
    le_thread_Start(SensorWorkerThRef);

    le_sig_SetEventHandler(SIGTERM, SensorTerminateHandler);

    le_msg_ServiceRef_t msgService = taf_imuSensor_GetServiceRef();
    if (msgService == NULL)
    {
        LE_FATAL("Init: taf_imuSensor_GetServiceRef returned NULL");
        return;
    }
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
}
