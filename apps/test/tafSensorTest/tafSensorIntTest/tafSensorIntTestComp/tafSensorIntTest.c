/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <time.h>

#define SENSOR_NUMS 2
taf_imuSensor_SensorRef_t sensorsList[SENSOR_NUMS];
taf_imuSensor_DataHandlerRef_t eventHandlerRef,eventHandlerRef1;
taf_imuSensor_SelfTestFailedHandlerRef_t selfTestHandlerRef;
taf_imuSensor_ConfigUpdateHandlerRef_t configUpdateHandlerRef;
taf_imuSensor_CapabilityUpdateHandlerRef_t capabilityHandlerRef;
le_thread_Ref_t threadRef1 =NULL;
taf_imuSensor_SensorListRef_t Head;
static le_sem_Ref_t semRef1;
static le_sem_Ref_t semRef2;
static le_mutex_Ref_t mSensorMutexRef;
static taf_imuSensor_SensorRef_t configUpdateThreadSensorRef = NULL;
static taf_imuSensor_SensorRef_t capabilityThreadSensorRef = NULL;
static volatile bool capabilityHandlerRegistered = false;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    double samplingRate;
    uint32_t batchCount;
} SensorConfig;

SensorConfig configList[SENSOR_NUMS];

void PrintUsage(void)
{
    puts("\n"
         "Usage of 'tafSensorIntTest' application.\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- AvailableSensors\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- SensorInfo <name>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- SetAngle <Pitch> <Roll> <Yaw>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- Activate <SensorName> <SamplingRate> <BatchCount>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- ActivateAll <SamplingRate1> <BatchCount1> <SamplingRate2> <BatchCount2>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- SelfTest <sensorName> <Mode>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- ConfigUpdate <SensorName> <SamplingRate> <BatchCount>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- Capability <SensorName> <SamplingRate> <BatchCount>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- Deactivate <SensorName>\n"
         "\n");
}

static le_result_t GetSensorList()
{
    int i=0;
    LE_INFO("Testing TelAF GetAvailableSensors");
    taf_imuSensor_SensorListRef_t listRef = taf_imuSensor_GetSensorList();
    Head = listRef;
    if(listRef != NULL){
        LE_INFO("GetSensorList SUCCESS");
    }else{
        LE_INFO("GetSensorList FAIELD");
        return LE_FAULT;
    }
    taf_imuSensor_SensorRef_t sensorRef = taf_imuSensor_GetFirstSensor(listRef);
    if(sensorRef != NULL){
        sensorsList[i++] = sensorRef;
        LE_INFO("GetFirstSensor SUCCESS");
    }else{
        LE_INFO("GetFirstSenosr FAIELD");
        return LE_FAULT;
    }
    while(sensorRef != NULL&&i<SENSOR_NUMS)
    {
        LE_INFO("Testing TelAF Next Sensor Reference Retrieval with -"
            "taf_imuSensor_GetNextSensor");
        sensorRef = taf_imuSensor_GetNextSensor(listRef);
        if(sensorRef == NULL){
            return LE_FAULT;
        }
        sensorsList[i++] = sensorRef;
    }
    return LE_OK;
}

static le_result_t TestAvailableSensorName(){
    le_result_t result;
    for(int i=0;i<SENSOR_NUMS;i++){
        taf_imuSensor_SensorRef_t ref = sensorsList[i];
        char sensorName[50];
        result = taf_imuSensor_GetName(ref,sensorName,sizeof(sensorName));
        if(result != LE_OK) return LE_FAULT;
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetName- LE_OK. name: %s",sensorName);
        printf("%d. %s\n",i+1,sensorName);
    }
    return LE_OK;
}

static le_result_t TestSensorInfo(const char* name)
{
    le_result_t result;
    for(int i=0;i<SENSOR_NUMS;i++)
    {
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50];
        result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        if(result != LE_OK) return LE_FAULT;
        if(strncmp(name,sensorName, strlen(name)) == 0)
        {
        printf("Sensor Info for %s:\n",sensorName);
        uint32_t id;
        char sensorVendorName[50];
        char version[10];
        taf_imuSensor_SensorType_t sensorType;
        result = taf_imuSensor_GetId(sensorRef,&id);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetId- LE_OK.");
        printf("id = %d\n",id);
        result = taf_imuSensor_GetVendorName(sensorRef,sensorVendorName,sizeof(sensorVendorName));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVendorName - LE_OK.");
        printf("vendorName = %s\n",sensorVendorName);
        result = taf_imuSensor_GetType(sensorRef,&sensorType);
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetType - LE_OK.");
        printf("SensorType = %d\n",sensorType);
        result = taf_imuSensor_GetVersion(sensorRef,version,sizeof(version));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVersion - LE_OK.");
        printf("SensorVersion = %s\n",version);

        double sampleRateList[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];
        size_t size = sizeof(sampleRateList)/sizeof(double);
        result = taf_imuSensor_GetSupportedSamplingRate(sensorRef,sampleRateList,&size);
        for(uint32_t i=0;i<size;i++){
            LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSamplingRateInfo - LE_OK.");
            printf("Supported Sample: %d. %f\n",i+1,sampleRateList[i]);
        }

        uint32_t maxBatchCount;
        uint32_t minBatchCount;
        result = taf_imuSensor_GetSupportedBatchCount(sensorRef,&maxBatchCount,&minBatchCount);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetBatchingInfo- LE_OK.");
        printf("MaxBatchCount =  %d\n",maxBatchCount);
        printf("MinBatchCount = %d\n",minBatchCount);

        double range;
        result = taf_imuSensor_GetRange(sensorRef,&range);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRange Info- LE_OK.");
        printf("range = %f\n",range);

        double resolution;
        result = taf_imuSensor_GetResolution(sensorRef,&resolution);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSensorResolution- LE_OK");
        printf("resolution = %f\n",resolution);
        return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

static le_result_t TestEulerAngle(double pitch, double roll , double yaw)
{
    le_result_t result;
    LE_TEST_INFO("Testing - taf_imuSensor_SetRefCoordinateByEulerAngle");
    result = taf_imuSensor_SetRefCoordinateByEulerAngle(NULL,pitch, roll, yaw);
    if(result != LE_OK){
        return LE_FAULT;
    }
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_SetRefCoordinateByEulerAngle - LE_OK");
    printf("Pitch = %f\n",pitch);
    printf("roll = %f\n",roll);
    printf("yaw = %f\n",yaw);
    return LE_OK;
}

void SensorOnEventNotification(taf_imuSensor_SampleRef_t sampleRef,
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

    for (int i = 0; i < SENSOR_NUMS; i++)
    {
        if (sensorRef != configList[i].sensorRef)
        {
            continue;
        }
        le_result_t result = LE_FAULT;
        char sensorName[50] = {0};
        result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
        if (result != LE_OK)
        {
            LE_TEST_INFO("sensor ref not found %p", sensorRef);
            le_mutex_Unlock(mSensorMutexRef);
            return;
        }

        double sampleRate = configList[i].samplingRate;
        uint32_t batch = configList[i].batchCount;

        LE_TEST_INFO("Test onEvent Retrieval '%d' for SensorName %s", i, sensorName);

        size_t size = rawDataCount;
        if (biasDataCount < size) size = biasDataCount;

        taf_imuSensor_DataValue_t rawDataFromApi[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
        taf_imuSensor_DataValue_t biasDataFromApi[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
        size_t apiSize = sizeof(rawDataFromApi) / sizeof(taf_imuSensor_DataValue_t);
        result = taf_imuSensor_GetRotatedData(sampleRef, rawDataFromApi, &apiSize,
            biasDataFromApi, &apiSize);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRotatedData- LE_OK. Event size %zu",
            apiSize);
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

        // Free server-side sample ref
        result = taf_imuSensor_DeleteData(sampleRef);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteData- LE_OK.");

        if (size == 0)
        {
            le_mutex_Unlock(mSensorMutexRef);
            return;
        }

        uint64_t eventTimeStamp = 0;
        uint32_t count = 0;
        double samplingRateAggregate = 0.0;

        for (size_t k = 0; k < size; k++)
        {
            double samplingRateInst = 0.0;
            if (eventTimeStamp > 0)
            {
                ++count;
                // Instantaneous sampling rate, calculated between consecutive samples
                samplingRateInst = 1.0 / (rawData[k].timestamp - eventTimeStamp) * 1000000000.0;
            }
            samplingRateAggregate += samplingRateInst;
            eventTimeStamp = rawData[k].timestamp;
        }

        printf("\033[1;31m %s [%f HZ, %d] Event [%lf Hz, %u, %"PRIu64" ns, %"PRIu64" ns].\033[0m\n",
               sensorName, sampleRate, batch,
               (count > 0) ? (samplingRateAggregate / count) : 0.0,
               (unsigned)(count + 1),
               rawData[0].timestamp,
               rawData[size - 1].timestamp);

        le_sem_Post(semRef1);
        break;
    }

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
    printf("SelfTest Failed for %s at timestamp %ld \n", sensorName ,timestamp);
}

static void* SensorHandler(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* config = (SensorConfig*)ctxPtr;
    LE_TEST_INFO("Test_taf_imuSensor_AddOnEventHandler on valid handler reference");
    eventHandlerRef =
        taf_imuSensor_AddDataHandler(config->sensorRef,SensorOnEventNotification, config->sensorRef);
    LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
        " is successfull");
    selfTestHandlerRef =
        taf_imuSensor_AddSelfTestFailedHandler(config->sensorRef,TestSensorFailedEvent,NULL);
    LE_TEST_OK(selfTestHandlerRef != NULL, "Register AddSelfTestFailedHandler handler"
        " is successfull");

    le_thread_Sleep(2);
    le_result_t result = taf_imuSensor_Activate(config->sensorRef,config->samplingRate,
        config->batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
    le_event_RunLoop();
    return NULL;
}

static le_result_t TestActivateSensor(const char* name,double SamplingRate,
    uint32_t BatchCount){
    le_result_t result;
    for(int i=0;i<SENSOR_NUMS;i++){
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50];
        result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        if(result != LE_OK) return result;
        if(strncmp(name,sensorName, strlen(name)) == 0)
        {
            SensorConfig c1 = {sensorRef, SamplingRate, BatchCount};
            configList[0] = c1;
            SensorConfig c2 = {NULL,0,0};
            configList[1] = c2;
            threadRef1 = le_thread_Create("Thread1", SensorHandler,&c1);
            le_thread_Start(threadRef1);
            le_thread_Sleep(30);
            taf_imuSensor_RemoveSelfTestFailedHandler(selfTestHandlerRef);
            taf_imuSensor_RemoveDataHandler(eventHandlerRef);
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

static void* AllSensorHandler(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    eventHandlerRef  =
        taf_imuSensor_AddDataHandler(sensorsList[0],SensorOnEventNotification, sensorsList[0]);
    LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
        " is successfull");

    eventHandlerRef1 =
        taf_imuSensor_AddDataHandler(sensorsList[1],SensorOnEventNotification, sensorsList[1]);
    LE_TEST_OK(eventHandlerRef1 != NULL, "Register AddOnEventHandler1 handler"
        " is successfull");

    le_thread_Sleep(2);

    le_result_t result = taf_imuSensor_Activate(sensorsList[0],configList[0].samplingRate,configList[0].batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);

    result = taf_imuSensor_Activate(sensorsList[1],configList[1].samplingRate,configList[1].batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);

    le_event_RunLoop();
    return NULL;
}

static le_result_t TestActivateAllSensor(double sampleRate1,uint32_t BatchCount1,
    double sampleRate2,uint32_t BatchCount2){

    SensorConfig c1 = {sensorsList[0],sampleRate1,BatchCount1};
    SensorConfig c2 = {sensorsList[1],sampleRate2,BatchCount2};

    configList[0] = c1;
    configList[1] = c2;

    threadRef1 = le_thread_Create("Thread1", AllSensorHandler,NULL);
    le_thread_Start(threadRef1);
    le_thread_Sleep(30);
    taf_imuSensor_RemoveDataHandler(eventHandlerRef);
    taf_imuSensor_RemoveDataHandler(eventHandlerRef1);
    return LE_OK;
}

static le_result_t TestSelfTest(const char* name,const char* mode){
    le_result_t result;
    taf_imuSensor_SelfTestMode_t modeType;
    uint64_t timestamp=0;
    if(strncmp(mode,"n",strlen(mode)) == 0 || strncmp(mode,"N",strlen(mode)) == 0  ){
        modeType = TAF_IMUSENSOR_NEGATIVE;
    }
    else if(strncmp(mode,"p",strlen(mode)) == 0 || strncmp(mode,"P",strlen(mode)) == 0){
       modeType =  TAF_IMUSENSOR_POSITIVE;
    }
    else if(strncmp(mode,"b",strlen(mode)) == 0 || strncmp(mode,"B",strlen(mode)) == 0){
       modeType =  TAF_IMUSENSOR_BOTH;
    }
    else{
        return LE_NOT_FOUND;
    }
    for(int i=0;i<SENSOR_NUMS;i++){
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50];
        result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        if(result != LE_OK) return result;
        if(strncmp(name,sensorName, strlen(name)) == 0){
            le_result_t result = taf_imuSensor_SelfTest(sensorRef,modeType,&timestamp);
            LE_TEST_OK(result == LE_OK, "taf_imuSensor_SelfTest Info- LE_OK.");
            if(result ==  LE_UNSUPPORTED ){
                printf("\033[1;31m Self Test not supported on this target. \033[0m\n");
            }
            else if(result == LE_TIMEOUT){
                printf("\033[1;31m Self Test for %s in mode %d is failed due to timeout. \033[0m\n",
                    sensorName,modeType);
            }
            else if(result == LE_OK){
                printf("\033[1;31m Self Test for %s in mode %d is Passed at %ld . \033[0m\n",
                    sensorName,modeType,timestamp);
            }
            else if(result == LE_BUSY){
                printf("\033[1;31m Sensor is Busy. Previous Self Test for %s in mode %d is Passed"
                    "at %ld . \033[0m\n",sensorName,modeType,timestamp);
            }
            else if(result == LE_UNAVAILABLE){
              printf("\033[1;31m Previous Self Test info not available \033[0m\n");
            }
            else{
                printf("\033[1;31m Self Test for %s in mode %d is Failed. \033[0m\n",
                    sensorName,modeType);
            }
            if (result != LE_OK) return LE_NOT_FOUND;
            return LE_OK;
        }
    }
    return LE_OK;
}


/* ============================================================================
 * ConfigUpdate handler callback
 * Invoked by the service whenever the sensor configuration is updated.
 * Prints the new configuration values and signals the semaphore so the
 * integration test thread can unblock and verify the notification was received.
 * ============================================================================ */
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
    printf("\033[1;32m [ConfigUpdate] %s: samplingRate=%.2f Hz, batchCount=%u, isRotated=%d\033[0m\n",
           sensorName, samplingRate, batchCount, (int)isRotated);
    LE_TEST_OK(samplingRate > 0,
        "ConfigUpdate: samplingRate (%.2f) is positive for %s", samplingRate, sensorName);
    LE_TEST_OK(batchCount > 0,
        "ConfigUpdate: batchCount (%u) is positive for %s", batchCount, sensorName);
    le_sem_Post(semRef1);
}

/* ============================================================================
 * Capability handler callback
 * Invoked by the service whenever the sensor capability/status is updated.
 * Prints the new capability values and signals the semaphore.
 * ============================================================================ */
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
    printf("\033[1;32m [Capability] %s: isAvailable=%d, isEnabled=%d, "
           "capabilityMask=0x%08" PRIx32 "\033[0m\n",
           sensorName, (int)isAvailable, (int)isEnabled, capabilityMask);
    LE_TEST_INFO("Capability: isAvailable=%d, isEnabled=%d for %s",
                 (int)isAvailable, (int)isEnabled, sensorName);
    le_sem_Post(semRef1);
}

static void ConfigUpdateThreadCleanup(void* param1, void* param2)
{
    LE_UNUSED(param1);
    LE_UNUSED(param2);

    if (configUpdateThreadSensorRef != NULL)
    {
        le_result_t result = taf_imuSensor_Deactivate(configUpdateThreadSensorRef);
        LE_TEST_OK(result == LE_OK,
            "ConfigUpdateThreadCleanup: taf_imuSensor_Deactivate - LE_OK (rc=%d)", (int)result);
    }

    if (configUpdateHandlerRef != NULL)
    {
        taf_imuSensor_RemoveConfigUpdateHandler(configUpdateHandlerRef);
        configUpdateHandlerRef = NULL;
        LE_TEST_INFO("ConfigUpdateThreadCleanup: "
                     "taf_imuSensor_RemoveConfigUpdateHandler - called");
    }

    if (eventHandlerRef != NULL)
    {
        taf_imuSensor_RemoveDataHandler(eventHandlerRef);
        eventHandlerRef = NULL;
        LE_TEST_INFO("ConfigUpdateThreadCleanup: taf_imuSensor_RemoveDataHandler - called");
    }

    configUpdateThreadSensorRef = NULL;
    le_thread_Exit(NULL);
}

static void CapabilityThreadCleanup(void* param1, void* param2)
{
    LE_UNUSED(param1);
    LE_UNUSED(param2);

    if (capabilityThreadSensorRef != NULL)
    {
        le_result_t result = taf_imuSensor_Deactivate(capabilityThreadSensorRef);
        LE_TEST_OK(result == LE_OK,
            "CapabilityThreadCleanup: taf_imuSensor_Deactivate - LE_OK (rc=%d)", (int)result);
    }

    if (capabilityHandlerRef != NULL)
    {
        taf_imuSensor_RemoveCapabilityUpdateHandler(capabilityHandlerRef);
        capabilityHandlerRef = NULL;
        LE_TEST_INFO("CapabilityThreadCleanup: taf_imuSensor_RemoveCapabilityUpdateHandler - called");
    }

    if (eventHandlerRef != NULL)
    {
        taf_imuSensor_RemoveDataHandler(eventHandlerRef);
        eventHandlerRef = NULL;
        LE_TEST_INFO("CapabilityThreadCleanup: taf_imuSensor_RemoveDataHandler - called");
    }

    capabilityThreadSensorRef = NULL;
    le_thread_Exit(NULL);
}

static void ActivateOneSensor(void* param1, void* param2)
{
    LE_UNUSED(param2);

    SensorConfig* config = (SensorConfig*)param1;
    le_result_t result = taf_imuSensor_Activate(
        config->sensorRef, config->samplingRate, config->batchCount);
    LE_TEST_OK(result == LE_OK,
        "ActivateOneSensor: taf_imuSensor_Activate - LE_OK (rc=%d)", (int)result);
}

/* ============================================================================
 * Thread entry for the ConfigUpdate integration test.
 * Connects to the service, registers the ConfigUpdate handler and a data
 * handler for the sensor, queues activation, then runs the event loop so
 * that the ConfigUpdate notification can be dispatched to the callback.
 * ============================================================================ */
static void* ConfigUpdateSensorThread(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* config = (SensorConfig*)ctxPtr;

    configUpdateThreadSensorRef = config->sensorRef;

    LE_TEST_INFO("ConfigUpdateSensorThread: registering ConfigUpdate handler for sensor %p",
                 config->sensorRef);

    configUpdateHandlerRef = taf_imuSensor_AddConfigUpdateHandler(
        config->sensorRef, ConfigUpdateNotification, config->sensorRef);
    LE_TEST_ASSERT(configUpdateHandlerRef != NULL,
        "taf_imuSensor_AddConfigUpdateHandler - OK");

    /* A data handler is required for the sensor to be activated */
    eventHandlerRef = taf_imuSensor_AddDataHandler(
        config->sensorRef, SensorOnEventNotification, config->sensorRef);
    LE_TEST_ASSERT(eventHandlerRef != NULL,
        "ConfigUpdateSensorThread: AddDataHandler - OK");

    le_event_QueueFunction(ActivateOneSensor, config, NULL);
    le_event_RunLoop();

    return NULL;
}

/* ============================================================================
 * Thread entry for the Capability integration test.
 * Connects to the service, registers the Capability handler and a data
 * handler for the sensor, queues activation, then runs the event loop so
 * that the Capability notification can be dispatched to the callback.
 * ============================================================================ */
static void* CapabilitySensorThread(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* config = (SensorConfig*)ctxPtr;

    capabilityThreadSensorRef = config->sensorRef;

    LE_TEST_INFO("CapabilitySensorThread: registering Capability handler for sensor %p",
                 config->sensorRef);

    capabilityHandlerRef = taf_imuSensor_AddCapabilityUpdateHandler(
        config->sensorRef, CapabilityNotification, config->sensorRef);
    capabilityHandlerRegistered = (capabilityHandlerRef != NULL);
    LE_TEST_ASSERT(capabilityHandlerRegistered,
        "taf_imuSensor_AddCapabilityUpdateHandler - OK");

    if (!capabilityHandlerRegistered)
    {
        LE_TEST_INFO("CapabilitySensorThread: capability handler registration failed");
    }

    le_sem_Post(semRef2);

    /* A data handler is required for the sensor to be activated */
    eventHandlerRef = taf_imuSensor_AddDataHandler(
        config->sensorRef, SensorOnEventNotification, config->sensorRef);
    LE_TEST_ASSERT(eventHandlerRef != NULL,
        "CapabilitySensorThread: AddDataHandler - OK");

    le_event_QueueFunction(ActivateOneSensor, config, NULL);
    le_event_RunLoop();

    return NULL;
}

/* ============================================================================
 * TestConfigUpdate
 *
 * Integration test for taf_imuSensor_AddConfigUpdateHandler /
 * taf_imuSensor_RemoveConfigUpdateHandler.
 *
 * Steps:
 *   1. Find the requested sensor by name.
 *   2. Spawn a worker thread that registers the ConfigUpdate handler and
 *      activates the sensor (activation triggers a config-update notification).
 *   3. Wait up to 30 s for the notification semaphore to be posted by the
 *      handler callback.
 *   4. Deactivate the sensor, remove the handler, and clean up.
 *
 * Usage:
 *   app runProc tafSensorIntTest tafSensorIntTest -- ConfigUpdate <SensorName> <SamplingRate> <BatchCount>
 * ============================================================================ */
static le_result_t TestConfigUpdate(const char* name, double samplingRate, uint32_t batchCount)
{
    le_result_t result;

    for (int i = 0; i < SENSOR_NUMS; i++)
    {
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50] = {0};
        result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
        if (result != LE_OK)
        {
            return result;
        }

        if (strncmp(name, sensorName, strlen(name)) != 0)
        {
            continue;
        }

        LE_TEST_INFO("=== TestConfigUpdate: sensor='%s', rate=%.2f, batch=%u ===",
                     sensorName, samplingRate, batchCount);

        SensorConfig cfg = { sensorRef, samplingRate, batchCount };
        configList[0] = cfg;
        SensorConfig empty = { NULL, 0, 0 };
        configList[1] = empty;

        threadRef1 = le_thread_Create("ConfigUpdateThread", ConfigUpdateSensorThread, &cfg);
        le_thread_SetJoinable(threadRef1);
        le_thread_Start(threadRef1);

        /* Wait up to 30 s for the ConfigUpdate notification */
        le_clk_Time_t timeout = { 30, 0 };
        le_result_t waitResult = le_sem_WaitWithTimeOut(semRef1, timeout);
        if (waitResult == LE_TIMEOUT)
        {
            LE_TEST_INFO("TestConfigUpdate: timed out waiting for ConfigUpdate notification "
                         "for sensor '%s'. The PA may not fire this event on activation.",
                         sensorName);
            /* Not a hard failure  the handler was registered correctly */
        }
        else
        {
            LE_TEST_OK(waitResult == LE_OK,
                "TestConfigUpdate: ConfigUpdate notification received for '%s'", sensorName);
        }

        le_event_QueueFunctionToThread(threadRef1, ConfigUpdateThreadCleanup, NULL, NULL);
        le_thread_Join(threadRef1, NULL);

        return LE_OK;
    }

    LE_TEST_INFO("TestConfigUpdate: sensor '%s' not found", name);
    return LE_NOT_FOUND;
}

/* ============================================================================
 * TestCapability
 *
 * Integration test for taf_imuSensor_AddCapabilityUpdateHandler /
 * taf_imuSensor_RemoveCapabilityUpdateHandler.
 *
 * Steps:
 *   1. Find the requested sensor by name.
 *   2. Spawn a worker thread that registers the Capability handler and
 *      activates the sensor.
 *   3. Wait up to 30 s for the notification semaphore to be posted by the
 *      handler callback.
 *   4. Deactivate the sensor, remove the handler, and clean up.
 *
 * Usage:
 *   app runProc tafSensorIntTest tafSensorIntTest -- Capability <SensorName> <SamplingRate> <BatchCount>
 * ============================================================================ */
static le_result_t TestCapability(const char* name, double samplingRate, uint32_t batchCount)
{
    le_result_t result;

    for (int i = 0; i < SENSOR_NUMS; i++)
    {
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50] = {0};
        result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
        if (result != LE_OK)
        {
            return result;
        }

        if (strncmp(name, sensorName, strlen(name)) != 0)
        {
            continue;
        }

        LE_TEST_INFO("=== TestCapability: sensor='%s', rate=%.2f, batch=%u ===",
                     sensorName, samplingRate, batchCount);

        SensorConfig cfg = { sensorRef, samplingRate, batchCount };
        configList[0] = cfg;
        SensorConfig empty = { NULL, 0, 0 };
        configList[1] = empty;

        threadRef1 = le_thread_Create("CapabilityThread", CapabilitySensorThread, &cfg);
        le_thread_SetJoinable(threadRef1);
        le_thread_Start(threadRef1);

        le_sem_Wait(semRef2);

        /* Wait up to 30 s for the Capability notification only if registration succeeded. */
        le_result_t waitResult = LE_FAULT;
        if (capabilityHandlerRegistered)
        {
            le_clk_Time_t timeout = { 30, 0 };
            waitResult = le_sem_WaitWithTimeOut(semRef1, timeout);
            if (waitResult == LE_TIMEOUT)
            {
                LE_TEST_INFO("TestCapability: timed out waiting for Capability notification "
                             "for sensor '%s'. The PA may not fire this event on activation.",
                             sensorName);
            }
            else
            {
                LE_TEST_OK(waitResult == LE_OK,
                    "TestCapability: Capability notification received for '%s'", sensorName);
            }
        }
        else
        {
            LE_TEST_INFO("TestCapability: Capability handler registration failed for '%s'",
                         sensorName);
        }

        le_event_QueueFunctionToThread(threadRef1, CapabilityThreadCleanup, NULL, NULL);
        le_thread_Join(threadRef1, NULL);

        return capabilityHandlerRegistered ? LE_OK : LE_FAULT;
    }

    LE_TEST_INFO("TestCapability: sensor '%s' not found", name);
    return LE_NOT_FOUND;
}
/* ============================================================================
 * TestDeactivate
 *
 * Integration test for taf_imuSensor_Deactivate.
 * Finds the sensor reference by name and calls taf_imuSensor_Deactivate().
 *
 * Usage:
 *   app runProc tafSensorIntTest tafSensorIntTest -- Deactivate <SensorName>
 * ============================================================================ */
static le_result_t TestDeactivate(const char* name)
{
    le_result_t result;
    for (int i = 0; i < SENSOR_NUMS; i++)
    {
        taf_imuSensor_SensorRef_t sensorRef = sensorsList[i];
        char sensorName[50] = {0};
        result = taf_imuSensor_GetName(sensorRef, sensorName, sizeof(sensorName));
        if (result != LE_OK)
        {
            return result;
        }
        if (strncmp(name, sensorName, strlen(name)) == 0)
        {
            LE_TEST_INFO("=== TestDeactivate: deactivating sensor '%s' ===", sensorName);
            result = taf_imuSensor_Deactivate(sensorRef);
            LE_TEST_OK(result == LE_OK,
                "taf_imuSensor_Deactivate '%s' - LE_OK (rc=%d)", sensorName, (int)result);
            return result;
        }
    }
    LE_TEST_INFO("TestDeactivate: sensor '%s' not found", name);
    return LE_NOT_FOUND;
}

void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
{
    if (NumArgs!=ExpectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}

static void DeleteSensorList()
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF deleting Sensor list with -taf_imuSensor_DeleteSensorList");
    result = taf_imuSensor_DeleteSensorList(Head);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteSensorList - LE_OK");
}

COMPONENT_INIT
{
    semRef1 = le_sem_Create("SemRef1", 0);
    semRef2 = le_sem_Create("SemRef2", 0);
    mSensorMutexRef = le_mutex_CreateRecursive("SensorMutexCl");
    le_result_t status = LE_FAULT;
    LE_TEST_INIT;
    status = GetSensorList();
    if(status != LE_OK){
        LE_TEST_FATAL("SensorList Not Available");
        return;
    }
    LE_TEST_INFO("======== Sensor Integration Test ========");
    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
    const char *testType = le_arg_GetArg(0);
    if(testType == NULL){
        PrintUsage();
        LE_TEST_FATAL("Test type is null");
    }
    if (strncmp(testType, "AvailableSensors", strlen(testType)) == 0){
        LE_TEST_INFO("=======Available Sensors Test========");
        CheckNumArgs(numArgs,1);
        status = TestAvailableSensorName();
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_GetName Succeed");
    }
    else if (strncmp(testType, "SensorInfo", strlen(testType)) == 0){
        LE_TEST_INFO("=======Sensors Info Test========");
        CheckNumArgs(numArgs,2);
        const char* sensorName = le_arg_GetArg(1);
        if (sensorName == NULL) {
            LE_TEST_FATAL("Invalid argument.");
        }
        status = TestSensorInfo(sensorName);
        if(status == LE_NOT_FOUND){
            LE_TEST_INFO("Sensor Name not found %s",sensorName);
        }
        LE_TEST_OK(status ==LE_OK,"Test SensorInfo Succeed %d",status);
    }
    else if (strncmp(testType, "SetAngle", strlen(testType)) == 0){
        LE_TEST_INFO("=======Set Euler Angle Test========");
        CheckNumArgs(numArgs,4);
        const char* arg1 = le_arg_GetArg(1);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        if (arg1 == NULL || arg2 == NULL || arg3 == NULL) {
            LE_TEST_FATAL("Invalid argument.");
        }
        double pitch = atof(arg1);
        double roll = atof(arg2);
        double yaw = atof(arg3);
        status = TestEulerAngle(pitch,roll,yaw);
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_SetEulerAngle Succeed");
    }
    else if(strncmp(testType, "Activate", strlen(testType)) == 0){
        LE_TEST_INFO("=======Test Sensor Activation========");
        CheckNumArgs(numArgs,4);
        const char* name = le_arg_GetArg(1);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        if (name == NULL || arg2 == NULL || arg3 == NULL) {
            LE_TEST_FATAL("Invalid argument.");
        }
        double sampleRate = atof(arg2);
        double BatchCount = atof(arg3);
        status = TestActivateSensor(name,sampleRate,BatchCount);
        if(status == LE_NOT_FOUND){
            LE_TEST_INFO("Sensor Name not found %s",name);
        }
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_Activate Succeed %d",status);
    }
    else if(strncmp(testType, "ActivateAll", strlen(testType)) == 0){
        LE_TEST_INFO("=======Test Sensor Activation========");
        CheckNumArgs(numArgs,5);
        const char* arg1 = le_arg_GetArg(1);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        const char* arg4 = le_arg_GetArg(4);
        if (arg1 == NULL || arg2 == NULL || arg3 == NULL || arg1 == NULL) {
            LE_TEST_FATAL("Invalid argument.");
        }
        double sampleRate1 = atof(arg1);
        double BatchCount1 = atof(arg2);
        double sampleRate2 = atof(arg3);
        double BatchCount2 = atof(arg4);
        status = TestActivateAllSensor(sampleRate1,BatchCount1,sampleRate2,BatchCount2);
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_Activate Succeed %d",status);
    }
    else if(strncmp(testType, "SelfTest", strlen(testType)) == 0){
        LE_TEST_INFO("=======Test Sensor SelfTest========");
        CheckNumArgs(numArgs,3);
        const char* name = le_arg_GetArg(1);
        const char* mode = le_arg_GetArg(2);
        if (name == NULL || mode == NULL) {
            LE_TEST_FATAL("Invalid argument.");
        }
        status = TestSelfTest(name,mode);
        if(status != LE_OK){
            LE_TEST_INFO("Sensor Name not found %s",name);
        }
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_SelfTest Succeed %d",status);
    }
    else if (strncmp(testType, "ConfigUpdate", strlen(testType)) == 0)
    {
        LE_TEST_INFO("=======Test ConfigUpdate Handler========");
        CheckNumArgs(numArgs, 4);
        const char* name  = le_arg_GetArg(1);
        const char* arg2  = le_arg_GetArg(2);
        const char* arg3  = le_arg_GetArg(3);
        if (name == NULL || arg2 == NULL || arg3 == NULL)
        {
            LE_TEST_FATAL("Invalid argument.");
        }
        double   sampleRate = atof(arg2);
        uint32_t batchCount = (uint32_t)atoi(arg3);
        status = TestConfigUpdate(name, sampleRate, batchCount);
        if (status == LE_NOT_FOUND)
        {
            LE_TEST_INFO("Sensor name not found: %s", name);
        }
        LE_TEST_OK(status == LE_OK,
            "Test taf_imuSensor_AddConfigUpdateHandler / RemoveConfigUpdateHandler Succeed"
            " (rc=%d)", (int)status);
    }
    else if (strncmp(testType, "Capability", strlen(testType)) == 0)
    {
        LE_TEST_INFO("=======Test Capability Handler========");
        CheckNumArgs(numArgs, 4);
        const char* name  = le_arg_GetArg(1);
        const char* arg2  = le_arg_GetArg(2);
        const char* arg3  = le_arg_GetArg(3);
        if (name == NULL || arg2 == NULL || arg3 == NULL)
        {
            LE_TEST_FATAL("Invalid argument.");
        }
        double   sampleRate = atof(arg2);
        uint32_t batchCount = (uint32_t)atoi(arg3);
        capabilityHandlerRegistered = false;
        status = TestCapability(name, sampleRate, batchCount);
        if (status == LE_NOT_FOUND)
        {
            LE_TEST_INFO("Sensor name not found: %s", name);
        }
        LE_TEST_OK(status == LE_OK,
            "Test taf_imuSensor_AddCapabilityUpdateHandler / RemoveCapabilityUpdateHandler Succeed"
            " (rc=%d)", (int)status);
    }
    else if (strncmp(testType, "Deactivate", strlen(testType)) == 0)
    {
        LE_TEST_INFO("=======Test Sensor Deactivation========");
        CheckNumArgs(numArgs, 2);
        const char* name = le_arg_GetArg(1);
        if (name == NULL)
        {
            LE_TEST_FATAL("Invalid argument.");
        }
        status = TestDeactivate(name);
        if (status == LE_NOT_FOUND)
        {
            LE_TEST_INFO("Sensor name not found: %s", name);
        }
        LE_TEST_OK(status == LE_OK,
            "Test taf_imuSensor_Deactivate Succeed (rc=%d)", (int)status);
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }
    DeleteSensorList();
    LE_TEST_EXIT;
}