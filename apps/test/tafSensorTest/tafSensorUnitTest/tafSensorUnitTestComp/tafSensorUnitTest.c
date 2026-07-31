/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "pthread.h"

#define SENSOR_NUMS 2
taf_imuSensor_SensorRef_t sensorsArray[SENSOR_NUMS];
taf_imuSensor_DataHandlerRef_t eventHandlerRef1,eventHandlerRef2;
taf_imuSensor_SelfTestFailedHandlerRef_t selfTestHandlerRef1,selfTestHandlerRef2;
taf_imuSensor_ConfigUpdateHandlerRef_t configUpdateHandlerRef1, configUpdateHandlerRef2;
taf_imuSensor_CapabilityUpdateHandlerRef_t capabilityHandlerRef1, capabilityHandlerRef2;
le_thread_Ref_t threadRef1 =NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int isDeactivate =0;
le_timer_Ref_t deactivateTimerRef;
taf_imuSensor_SensorListRef_t Head;
static le_sem_Ref_t semRef1;
static le_sem_Ref_t semRef2;
static le_mutex_Ref_t mSensorMutexRef;
static volatile bool configUpdateReceived = false;
static volatile bool capabilityReceived = false;
static taf_imuSensor_SensorRef_t configUpdateTestSensorRef = NULL;
static taf_imuSensor_SensorRef_t capabilityTestSensorRef = NULL;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    double samplingRate;
    uint32_t batchCount;
} SensorConfig;

SensorConfig configList[SENSOR_NUMS];

void TestSetEulerAngle(){
    le_result_t result;
    double pitch = 90, roll = 90, yaw = 90;
    LE_TEST_INFO("Testing Setting euler angle for Sensor with -taf_imuSensor_SetRefCoordinateByEulerAngle");
    result = taf_imuSensor_SetRefCoordinateByEulerAngle(NULL,pitch, roll, yaw);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_SetRefCoordinateByEulerAngle - LE_OK");
    LE_INFO("===== UnitTest Completed for setting euler angle =====");
}

void TestAvailableSensor()
{
    int i=0;
    le_result_t result;
    LE_TEST_INFO("Testing TelAF GetAvailableSensors");
    taf_imuSensor_SensorListRef_t listRef = taf_imuSensor_GetSensorList();
    if(listRef != NULL){
        LE_TEST_INFO("GetSensorList SUCCESS");
    }else{
        LE_TEST_INFO("GetSensorList FAIELD");
        return;
    }
    Head = listRef;
    taf_imuSensor_SensorRef_t sensorRef = taf_imuSensor_GetFirstSensor(listRef);
    if(sensorRef != NULL){
        LE_TEST_INFO("GetFirstSensor SUCCESS");
    }else{
        LE_TEST_INFO("GetFirstSenosr FAIELD");
        return;
    }
    while(sensorRef != NULL&&i<SENSOR_NUMS)
    {
        sensorsArray[i++] = sensorRef;
        uint32_t id;
        char sensorName[50];
        char sensorVendorName[50];
        char version[10];
        taf_imuSensor_SensorType_t sensorType;
        result = taf_imuSensor_GetId(sensorRef,&id);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetId- LE_OK. id: %d",id);
        result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetName- LE_OK. name: %s",sensorName);
        result = taf_imuSensor_GetVendorName(sensorRef,sensorVendorName,sizeof(sensorVendorName));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVendorName- LE_OK. vendor = %s",sensorVendorName);
        result = taf_imuSensor_GetType(sensorRef,&sensorType);
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetType - LE_OK. type = %d",sensorType);
        result = taf_imuSensor_GetVersion(sensorRef,version,sizeof(version));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVersion - LE_OK. version = %s",version);

        double sampleRateList[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];
        size_t size = sizeof(sampleRateList)/sizeof(double);
        result = taf_imuSensor_GetSupportedSamplingRate(sensorRef,sampleRateList,&size);
        for(uint32_t i=0;i<size;i++){
            LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSupportedSamplingRate - LE_OK. Sample%d %f",
                i+1,sampleRateList[i]);
        }
        uint32_t maxBatchCount;
        uint32_t minBatchCount;
        result = taf_imuSensor_GetSupportedBatchCount(sensorRef,&maxBatchCount,&minBatchCount);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSupportedBatchCount- LE_OK."
            " MaxBatchCount =  %d , MinBatchCount = %d",maxBatchCount,minBatchCount);

        double range;
        result = taf_imuSensor_GetRange(sensorRef,&range);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRange Info- LE_OK."
            " range = %f",range);

        double resolution;
        result = taf_imuSensor_GetResolution(sensorRef,&resolution);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSensorResolution- LE_OK.%f",resolution);

        if(i<SENSOR_NUMS){
            LE_TEST_INFO("Testing TelAF Next Sensor Reference Retrieval with -"
                "taf_imuSensor_GetNextSensor");
            sensorRef = taf_imuSensor_GetNextSensor(listRef);
            LE_TEST_OK((sensorRef != NULL), "taf_imuSensor_GetNextSensor - LE_OK");
        }
    }
    LE_INFO("===== UnitTest Completed for retrieving information about Sensors =====");
}
void TestSensorOnEventFunc(taf_imuSensor_SampleRef_t sampleRef,
    const taf_imuSensor_DataValue_t* rawData,  size_t rawDataCount,
    const taf_imuSensor_DataValue_t* biasData, size_t biasDataCount,
    void* contextPtr)
{
    le_mutex_Lock(mSensorMutexRef);

    taf_imuSensor_SensorRef_t sensorRef = (taf_imuSensor_SensorRef_t)contextPtr;
    if (sensorRef == NULL || rawData == NULL || biasData == NULL)
    {
        LE_TEST_INFO("Invalid callback args: sensorRef=%p raw=%p bias=%p",
                     sensorRef, rawData, biasData);
        le_mutex_Unlock(mSensorMutexRef);
        return;
    }

    char sensorName[50] = {0};
    double sampleRateCfg = 0;
    uint32_t batchCfg = 0;
    bool found = false;

    for (int i = 0; i < SENSOR_NUMS; i++)
    {
        if (sensorRef == configList[i].sensorRef)
        {
            sampleRateCfg = configList[i].samplingRate;
            batchCfg = configList[i].batchCount;
            found = true;
            break;
        }
    }

    le_result_t result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
    if (result != LE_OK || !found)
    {
        LE_TEST_INFO("sensor ref not found or config missing %p", sensorRef);
        le_mutex_Unlock(mSensorMutexRef);
        return;
    }

    size_t size = rawDataCount;
    if (biasDataCount < size) size = biasDataCount;
    if (size == 0)
    {
        LE_TEST_INFO("No samples in event for %s", sensorName);
        (void)taf_imuSensor_DeleteData(sampleRef); // Free server-side sample ref
        le_mutex_Unlock(mSensorMutexRef);
        return;
    }

    taf_imuSensor_DataValue_t rawDataFromApi[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
    taf_imuSensor_DataValue_t biasDataFromApi[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
    size_t apiSize = sizeof(rawDataFromApi) / sizeof(taf_imuSensor_DataValue_t);
    result = taf_imuSensor_GetRotatedData(sampleRef, rawDataFromApi, &apiSize,
        biasDataFromApi, &apiSize);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRotatedData - LE_OK. Event size %zu", apiSize);
    LE_TEST_OK(apiSize == size, "taf_imuSensor_GetRotatedData size matches callback data");

    for (size_t j = 0; j < apiSize; ++j)
    {
        bool rawMatched = (rawDataFromApi[j].timestamp == rawData[j].timestamp) &&
            (rawDataFromApi[j].x == rawData[j].x) &&
            (rawDataFromApi[j].y == rawData[j].y) &&
            (rawDataFromApi[j].z == rawData[j].z);
        if (!rawMatched)
        {
            LE_TEST_INFO("RawData[%zu] mismatch: api={ts=%"PRIu64", x=%lf, y=%lf, z=%lf}, "
                "callback={ts=%"PRIu64", x=%lf, y=%lf, z=%lf}",
                j,
                rawDataFromApi[j].timestamp, rawDataFromApi[j].x, rawDataFromApi[j].y,
                rawDataFromApi[j].z,
                rawData[j].timestamp, rawData[j].x, rawData[j].y, rawData[j].z);
            LE_TEST_OK(false, "RawData[%zu] from GetRotatedData matches callback data", j);
        }

        bool biasMatched = (biasDataFromApi[j].timestamp == biasData[j].timestamp) &&
            (biasDataFromApi[j].x == biasData[j].x) &&
            (biasDataFromApi[j].y == biasData[j].y) &&
            (biasDataFromApi[j].z == biasData[j].z);
        if (!biasMatched)
        {
            LE_TEST_INFO("BiasData[%zu] mismatch: api={ts=%"PRIu64", x=%lf, y=%lf, z=%lf}, "
                "callback={ts=%"PRIu64", x=%lf, y=%lf, z=%lf}",
                j,
                biasDataFromApi[j].timestamp, biasDataFromApi[j].x, biasDataFromApi[j].y,
                biasDataFromApi[j].z,
                biasData[j].timestamp, biasData[j].x, biasData[j].y, biasData[j].z);
            LE_TEST_OK(false, "BiasData[%zu] from GetRotatedData matches callback data", j);
        }
    }

    // Free the sample reference on server side (important to avoid ref-map growth)
    result = taf_imuSensor_DeleteData(sampleRef);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteData - LE_OK.");

    uint64_t eventTimeStamp = 0;
    uint32_t count = 0;
    float samplingRateAggregate = 0.0f;

    for (size_t i = 0; i < size; i++)
    {
        float samplingRate = 0.0f;
        if (eventTimeStamp > 0)
        {
            ++count;
            samplingRate = 1.0f / (rawData[i].timestamp - eventTimeStamp) * 1000000000.0f;
        }
        samplingRateAggregate += samplingRate;
        eventTimeStamp = rawData[i].timestamp;
    }

    LE_TEST_INFO("%s [%f HZ, %d] Event [%f HZ, %"PRIu64" ns, %"PRIu64" ns]\n",
                 sensorName,
                 sampleRateCfg, batchCfg,
                 (count > 0) ? (samplingRateAggregate / count) : 0.0f,
                 rawData[0].timestamp,
                 rawData[size - 1].timestamp);

    le_sem_Post(semRef1);

    le_mutex_Unlock(mSensorMutexRef);
}

void TestSensorFailedEvent(taf_imuSensor_SelfTestEventRef_t eventRef,
    taf_imuSensor_SensorRef_t sensorRef,uint64_t timestamp,void* contextPtr){
    char sensorName[50];
    le_result_t result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
    if(result !=LE_OK){
        LE_TEST_INFO("sensor ref not found %p", sensorRef);
        return;
    }
    LE_INFO("SelfTest Failed for %s at timestamp %"PRIu64" \n", sensorName ,timestamp);
}

static void TestDeactivateHandler(le_timer_Ref_t timerRef){
    LE_INFO("TestDeactivateHandler");
    pthread_mutex_lock(&mutex);
    le_result_t res = taf_imuSensor_Deactivate(configList[0].sensorRef);
    LE_TEST_OK(res == LE_OK,"Sensor deactivate successfully");
    res = taf_imuSensor_Deactivate(configList[1].sensorRef);
    LE_TEST_OK(res == LE_OK,"Sensor deactivate successfully");
    isDeactivate=1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

static void* SensorHandler(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    eventHandlerRef1 =
       taf_imuSensor_AddDataHandler(sensorsArray[0],TestSensorOnEventFunc,sensorsArray[0]);
    LE_TEST_OK(eventHandlerRef1 != NULL, "Register AddOnEventHandler1 handler"
        " is successfull");
    eventHandlerRef2 =
        taf_imuSensor_AddDataHandler(sensorsArray[1],TestSensorOnEventFunc,sensorsArray[1]);
    LE_TEST_OK(eventHandlerRef2 != NULL, "Register AddOnEventHandler2 handler"
         " is successfull");
    selfTestHandlerRef1 =
        taf_imuSensor_AddSelfTestFailedHandler(sensorsArray[0],TestSensorFailedEvent,NULL);
    LE_TEST_OK(selfTestHandlerRef1 != NULL, "Register AddSelfTestFailedHandler1 handler"
        " is successfull");
    selfTestHandlerRef2 =
        taf_imuSensor_AddSelfTestFailedHandler(sensorsArray[1],TestSensorFailedEvent,NULL);
    LE_TEST_OK(selfTestHandlerRef2 != NULL, "Register AddSelfTestFailedHandler1 handler"
        " is successfull");
    le_thread_Sleep(2);
    le_result_t result = taf_imuSensor_Activate(configList[0].sensorRef,configList[0].samplingRate,
        configList[0].batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
    result = taf_imuSensor_Activate(configList[1].sensorRef,configList[1].samplingRate,
        configList[1].batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
    deactivateTimerRef = le_timer_Create("deactivate wait timer");
    le_timer_SetMsInterval(deactivateTimerRef,20000);
    le_timer_SetHandler(deactivateTimerRef, TestDeactivateHandler);
    le_timer_Start(deactivateTimerRef);
    le_event_RunLoop();
    return NULL;
}

void TestSelfTest(int index){
    LE_TEST_INFO("--------- Testing Self Test for Sensor----------");
    uint64_t timestamp =0;
    le_result_t result = taf_imuSensor_SelfTest(sensorsArray[index],TAF_IMUSENSOR_BOTH,&timestamp);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_SelfTest Info- LE_OK. at %ld",timestamp);
    if(result == LE_UNSUPPORTED){
        LE_TEST_INFO("Not supported on this target");
    }
    else if(result == LE_BUSY){
        LE_TEST_INFO("Sensor is busy at this point of time, giving previous passed result %ld",timestamp);
    }
    else if(result == LE_TIMEOUT){
        LE_TEST_INFO("Failed due to time out.");
    }
    else if(result == LE_UNAVAILABLE){
        LE_TEST_INFO("Previous Self Test Info not available");
    }
}

void TestActivateSensor(){
    LE_TEST_INFO("--------- Testing Activating Sensor----------");
    threadRef1 = le_thread_Create("Thread1", SensorHandler,NULL);
    le_thread_Start(threadRef1);
    pthread_mutex_lock(&mutex);
    while(isDeactivate!=1){
        pthread_cond_wait(&cond,&mutex);
    }
    pthread_mutex_unlock(&mutex);
    taf_imuSensor_RemoveSelfTestFailedHandler(selfTestHandlerRef1);
    taf_imuSensor_RemoveSelfTestFailedHandler(selfTestHandlerRef2);
    taf_imuSensor_RemoveDataHandler(eventHandlerRef1);
    taf_imuSensor_RemoveDataHandler(eventHandlerRef2);
    LE_TEST_INFO("On Event Handler removed");
    isDeactivate=0;
}

void DeleteSensorList()
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF deleting Sensor list with -taf_imuSensor_DeleteSensorList");
    result = taf_imuSensor_DeleteSensorList(Head);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteSensorList - LE_OK");
}



/*==================================================================================================
 * ConfigUpdate handler callback
 *==================================================================================================*/
static void ConfigUpdateNotification(taf_imuSensor_SensorRef_t sensorRef,
    double samplingRate, uint32_t batchCount, bool isRotated, void* contextPtr)
{
    char sensorName[50] = {0};
    le_result_t result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
    if (result != LE_OK)
    {
        LE_TEST_INFO("ConfigUpdateNotification: GetName failed for sensorRef %p", sensorRef);
        return;
    }
    LE_TEST_INFO("ConfigUpdate received for %s: samplingRate=%.2f Hz, batchCount=%u, isRotated=%d",
                 sensorName, samplingRate, batchCount, (int)isRotated);
    configUpdateReceived = true;
    le_sem_Post(semRef2);
}

/*==================================================================================================
 * Capability handler callback
 *==================================================================================================*/
static void CapabilityNotification(taf_imuSensor_SensorRef_t sensorRef,
    bool isAvailable, bool isEnabled, uint32_t capabilityMask, void* contextPtr)
{
    char sensorName[50] = {0};
    le_result_t result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
    if (result != LE_OK)
    {
        LE_TEST_INFO("CapabilityNotification: GetName failed for sensorRef %p", sensorRef);
        return;
    }
    LE_TEST_INFO("Capability update received for %s: isAvailable=%d, isEnabled=%d, "
                 "capabilityMask=0x%08" PRIx32,
                 sensorName, (int)isAvailable, (int)isEnabled, capabilityMask);
    capabilityReceived = true;
    le_sem_Post(semRef2);
}

/*==================================================================================================
 * ActivateSensorQueued
 * Queued function that activates the sensor inside the worker thread's event loop.
 * Shared by both ConfigUpdate and Capability test threads.
 *==================================================================================================*/
static void ActivateSensorQueued(void* p1, void* p2)
{
    LE_UNUSED(p2);
    SensorConfig* cfg = (SensorConfig*)p1;
    le_result_t result = taf_imuSensor_Activate(
        cfg->sensorRef, cfg->samplingRate, cfg->batchCount);
    LE_TEST_INFO("ActivateSensorQueued: taf_imuSensor_Activate rc=%d", (int)result);
}

/*==================================================================================================
 * ConfigUpdate test thread: connects to the service, registers handlers for both sensors,
 * queues sensor activation, then runs the event loop to receive notifications.
 *==================================================================================================*/
static void ConfigUpdateTestCleanup(void* p1, void* p2)
{
    LE_UNUSED(p1);
    LE_UNUSED(p2);
    if (configUpdateTestSensorRef != NULL)
    {
        taf_imuSensor_Deactivate(configUpdateTestSensorRef);
        configUpdateTestSensorRef = NULL;
    }
    if (configUpdateHandlerRef1 != NULL)
    {
        taf_imuSensor_RemoveConfigUpdateHandler(configUpdateHandlerRef1);
        LE_TEST_INFO("taf_imuSensor_RemoveConfigUpdateHandler sensor[0] - called");
        configUpdateHandlerRef1 = NULL;
    }
    if (configUpdateHandlerRef2 != NULL)
    {
        taf_imuSensor_RemoveConfigUpdateHandler(configUpdateHandlerRef2);
        LE_TEST_INFO("taf_imuSensor_RemoveConfigUpdateHandler sensor[1] - called");
        configUpdateHandlerRef2 = NULL;
    }
    le_thread_Exit(NULL);
}

static void* ConfigUpdateTestThread(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* cfg = (SensorConfig*)ctxPtr;
    configUpdateTestSensorRef = cfg->sensorRef;

    configUpdateHandlerRef1 = taf_imuSensor_AddConfigUpdateHandler(
        sensorsArray[0], ConfigUpdateNotification, sensorsArray[0]);
    LE_TEST_OK(configUpdateHandlerRef1 != NULL,
        "taf_imuSensor_AddConfigUpdateHandler sensor[0] - returned non-NULL ref");

    configUpdateHandlerRef2 = taf_imuSensor_AddConfigUpdateHandler(
        sensorsArray[1], ConfigUpdateNotification, sensorsArray[1]);
    LE_TEST_OK(configUpdateHandlerRef2 != NULL,
        "taf_imuSensor_AddConfigUpdateHandler sensor[1] - returned non-NULL ref");

    le_event_QueueFunction(ActivateSensorQueued, cfg, NULL);
    le_event_RunLoop();
    return NULL;
}

/*==================================================================================================
 * TestAddRemoveConfigUpdateHandler
 * Tests registration and removal of the ConfigUpdate handler for both sensors.
 *==================================================================================================*/
void TestAddRemoveConfigUpdateHandler(void)
{
    LE_TEST_INFO("--------- Testing AddConfigUpdateHandler / RemoveConfigUpdateHandler ----------");

    le_thread_Ref_t testThread = le_thread_Create("ConfigUpdateTestThread",
                                                   ConfigUpdateTestThread, &configList[0]);
    le_thread_SetJoinable(testThread);
    le_thread_Start(testThread);

    le_clk_Time_t timeout = { 5, 0 };
    le_result_t waitResult = le_sem_WaitWithTimeOut(semRef2, timeout);
    LE_TEST_OK(waitResult == LE_OK,
        "ConfigUpdate handler callback received after registration");

    le_event_QueueFunctionToThread(testThread, ConfigUpdateTestCleanup, NULL, NULL);
    le_thread_Join(testThread, NULL);

    LE_TEST_INFO("===== UnitTest Completed for ConfigUpdate handler =====");
}

/*==================================================================================================
 * Capability test thread: connects to the service, registers handlers for both sensors,
 * queues sensor activation, then runs the event loop to receive notifications.
 *==================================================================================================*/
static void CapabilityTestCleanup(void* p1, void* p2)
{
    LE_UNUSED(p1);
    LE_UNUSED(p2);
    if (capabilityTestSensorRef != NULL)
    {
        taf_imuSensor_Deactivate(capabilityTestSensorRef);
        capabilityTestSensorRef = NULL;
    }
    if (capabilityHandlerRef1 != NULL)
    {
        taf_imuSensor_RemoveCapabilityUpdateHandler(capabilityHandlerRef1);
        LE_TEST_INFO("taf_imuSensor_RemoveCapabilityUpdateHandler sensor[0] - called");
        capabilityHandlerRef1 = NULL;
    }
    if (capabilityHandlerRef2 != NULL)
    {
        taf_imuSensor_RemoveCapabilityUpdateHandler(capabilityHandlerRef2);
        LE_TEST_INFO("taf_imuSensor_RemoveCapabilityUpdateHandler sensor[1] - called");
        capabilityHandlerRef2 = NULL;
    }
    le_thread_Exit(NULL);
}

static void* CapabilityTestThread(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* cfg = (SensorConfig*)ctxPtr;
    capabilityTestSensorRef = cfg->sensorRef;

    capabilityHandlerRef1 = taf_imuSensor_AddCapabilityUpdateHandler(
        sensorsArray[0], CapabilityNotification, sensorsArray[0]);
    LE_TEST_OK(capabilityHandlerRef1 != NULL,
        "taf_imuSensor_AddCapabilityUpdateHandler sensor[0] - returned non-NULL ref");

    capabilityHandlerRef2 = taf_imuSensor_AddCapabilityUpdateHandler(
        sensorsArray[1], CapabilityNotification, sensorsArray[1]);
    LE_TEST_OK(capabilityHandlerRef2 != NULL,
        "taf_imuSensor_AddCapabilityUpdateHandler sensor[1] - returned non-NULL ref");

    le_event_QueueFunction(ActivateSensorQueued, cfg, NULL);
    le_event_RunLoop();
    return NULL;
}

/*==================================================================================================
 * TestAddRemoveCapabilityUpdateHandler
 * Tests registration and removal of the CapabilityUpdate handler for both sensors.
 *==================================================================================================*/
void TestAddRemoveCapabilityUpdateHandler(void)
{
    LE_TEST_INFO("--------- Testing AddCapabilityUpdateHandler / RemoveCapabilityUpdateHandler ----------");

    le_thread_Ref_t testThread = le_thread_Create("CapabilityTestThread",
                                                   CapabilityTestThread, &configList[0]);
    le_thread_SetJoinable(testThread);
    le_thread_Start(testThread);

    le_clk_Time_t timeout = { 5, 0 };
    le_result_t waitResult = le_sem_WaitWithTimeOut(semRef2, timeout);
    LE_TEST_OK(waitResult == LE_OK,
        "Capability handler callback received after registration");

    le_event_QueueFunctionToThread(testThread, CapabilityTestCleanup, NULL, NULL);
    le_thread_Join(testThread, NULL);

    LE_TEST_INFO("===== UnitTest Completed for Capability handler =====");
}
COMPONENT_INIT{
    semRef1 = le_sem_Create("SemRef1", 0);
    semRef2 = le_sem_Create("SemRef2", 0);
    mSensorMutexRef = le_mutex_CreateRecursive("SensorMutexCl");
    TestAvailableSensor();
    TestSetEulerAngle();
    SensorConfig c1 = {sensorsArray[0],104.00,50};
    SensorConfig c2 = {sensorsArray[1],52.00,50};
    configList[0] = c1;
    configList[1] = c2;

    // Testing multiple sequence for activation and deactivation
    TestActivateSensor();
    TestSelfTest(0);
    TestSelfTest(1);
    // Testing ConfigUpdate and Capability handler registration/removal
    configUpdateReceived = false;
    capabilityReceived = false;
    TestAddRemoveConfigUpdateHandler();
    TestAddRemoveCapabilityUpdateHandler();
    DeleteSensorList();
    exit(EXIT_SUCCESS);
}
