/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

#define SENSOR_NUMS 2
taf_imuSensor_SensorRef_t sensorsArray[SENSOR_NUMS];
taf_imuSensor_DataHandlerRef_t eventHandlerRef;
le_thread_Ref_t threadRef1 =NULL;
taf_imuSensor_SensorListRef_t Head;
static le_sem_Ref_t semRef1;
static le_mutex_Ref_t mSensorMutexRef;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    double samplingRate;
    uint32_t batchCount;
} SensorConfig;

SensorConfig config;

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

void TestSensorOnEventFunc(taf_imuSensor_SensorRef_t sensorRef,taf_imuSensor_SampleRef_t ref
    ,void* contextPtr){
    le_mutex_Lock(mSensorMutexRef);
    if(sensorRef == config.sensorRef){
    char sensorName[50];
    le_result_t result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
    if(result !=LE_OK){
        LE_TEST_INFO("sensor ref not found %p", sensorRef);
        return;
    }
    double sampleRate = config.samplingRate;
    uint32_t batch = config.batchCount;
    LE_TEST_INFO("Test onEvent Retrieval for SensorName %s",sensorName);
    taf_imuSensor_DataValue_t rawData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
    taf_imuSensor_DataValue_t biasData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
    size_t  size = sizeof(rawData)/sizeof(taf_imuSensor_DataValue_t);
    result = taf_imuSensor_GetRotatedData(ref,rawData,&size,biasData,&size);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRotatedData- LE_OK. Event size %zu",size);
    uint64_t eventTimeStamp = 0;
    uint32_t count = 0;
    float samplingRateAggregate = 0.0;
    for(uint32_t i=0;i<size;i++){
        float samplingRate = 0.0;
        if (eventTimeStamp > 0) {
            ++count;
            // Instantaneous sampling rate, calculated between consecutive samples
            samplingRate = 1.0 / (rawData[i].timestamp - eventTimeStamp) * 1000000000;
        }
        samplingRateAggregate += samplingRate;
        eventTimeStamp = rawData[i].timestamp;
    }
    LE_TEST_INFO("%s [%f HZ, %d] Event [%f HZ, %"PRIu64" ns, %"PRIu64" ns]\n",sensorName,
        sampleRate,batch,samplingRateAggregate/count,rawData[0].timestamp,
        rawData[size-1].timestamp);
    result = taf_imuSensor_DeleteData(ref);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteData- LE_OK.");
    le_sem_Post(semRef1);
    }
    le_mutex_Unlock(mSensorMutexRef);
}


static void* SensorHandler(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    SensorConfig* sConfig = (SensorConfig*)ctxPtr;
    eventHandlerRef =
       taf_imuSensor_AddDataHandler(sConfig->sensorRef,TestSensorOnEventFunc,NULL);
    LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
        " is successfull");
    le_thread_Sleep(2);
    le_result_t result = taf_imuSensor_Activate(sConfig->sensorRef,sConfig->samplingRate,
        sConfig->batchCount);
    LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
    le_event_RunLoop();
    return NULL;
}

void TestActivateSensor(){
    LE_TEST_INFO("--------- Testing Activating Sensor----------");
    config.sensorRef = sensorsArray[0];
    config.samplingRate = 104.00;
    config.batchCount = 50;
    le_result_t result  =  taf_imuSensor_Activate(sensorsArray[0],104.00,50);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_Activate Info- LE_OK.");
    if(result!=LE_OK) return;
    threadRef1 = le_thread_Create("Thread1", SensorHandler,&config);
    le_thread_Start(threadRef1);
    le_thread_Sleep(30);
    taf_imuSensor_RemoveDataHandler(eventHandlerRef);
    LE_TEST_INFO("On Event Handler removed");
    le_thread_Cancel(threadRef1);
    result = taf_imuSensor_Deactivate(sensorsArray[0]);
    LE_TEST_OK(result == LE_OK,"Deactivate Accel");
    if(result!=LE_OK) return;
}

void TestSelfTest(){
    LE_TEST_INFO("--------- Testing Self Test for Sensor----------");
    le_result_t result = taf_imuSensor_SelfTest(sensorsArray[0],TAF_IMUSENSOR_POSITIVE);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_SelfTest Info- LE_OK.");
    if(result == LE_UNSUPPORTED){
        LE_TEST_INFO("Not supported on this target");
    }
    else if(result == LE_TIMEOUT){
        LE_TEST_INFO("Failed due to time out.");
    }
}

void DeleteSensorList()
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF deleting Sensor list with -taf_imuSensor_DeleteSensorList");
    result = taf_imuSensor_DeleteSensorList(Head);
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteSensorList - LE_OK");
}


COMPONENT_INIT{
    semRef1 = le_sem_Create("SemRef1", 0);
    mSensorMutexRef = le_mutex_CreateRecursive("SensorMutexCl");
    TestAvailableSensor();
    TestSetEulerAngle();
    TestActivateSensor();
    TestSelfTest();
    le_thread_Sleep(2);
    DeleteSensorList();
    exit(EXIT_SUCCESS);
}
