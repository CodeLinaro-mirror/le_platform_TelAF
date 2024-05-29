/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <time.h>

#define MAX_SYSTEM_CMD_LENGTH 200
#define SENSOR_NUMS 2
taf_imuSensor_SensorRef_t sensorsList[SENSOR_NUMS];
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

void PrintUsage(void)
{
    puts("\n"
         "Usage of 'tafSensorIntTest' application.\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- AvailableSensors\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- SensorInfo <name>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- SetAngle <Pitch> <Roll> <Yaw>\n"
         "app runProc tafSensorIntTest tafSensorIntTest -- Activate <SensorName> <SamplingRate> <BatchCount>"
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
    for(int i=0;i<SENSOR_NUMS;i++){
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
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetId- LE_OK. id: %d",id);
        printf("id = %d\n",id);
        result = taf_imuSensor_GetVendorName(sensorRef,sensorVendorName,sizeof(sensorVendorName));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVendorName- LE_OK. vendor = %s",sensorVendorName);
        printf("vendorName = %s\n",sensorVendorName);
        result = taf_imuSensor_GetType(sensorRef,&sensorType);
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetType - LE_OK. type = %d",sensorType);
        printf("SensorType = %d\n",sensorType);
        result = taf_imuSensor_GetVersion(sensorRef,version,sizeof(version));
        LE_TEST_OK(result == LE_OK,"taf_imuSensor_GetVersion - LE_OK. version = %s",version);
        printf("SensorVersion = %s\n",version);

        double sampleRateList[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];
        size_t size = sizeof(sampleRateList)/sizeof(double);
        result = taf_imuSensor_GetSupportedSamplingRate(sensorRef,sampleRateList,&size);
        for(uint32_t i=0;i<size;i++){
            LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSamplingRateInfo - LE_OK. Sample%d %f",
                i+1,sampleRateList[i]);
            printf("Supported Sample: %d. %f\n",i+1,sampleRateList[i]);
        }

        uint32_t maxBatchCount;
        uint32_t minBatchCount;
        result = taf_imuSensor_GetSupportedBatchCount(sensorRef,&maxBatchCount,&minBatchCount);
         LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetBatchingInfo- LE_OK."
        " MaxBatchCount =  %d , MinBatchCount = %d",maxBatchCount,minBatchCount);
        printf("MaxBatchCount =  %d\n",maxBatchCount);
        printf("MinBatchCount = %d\n",minBatchCount);

        double range;
        result = taf_imuSensor_GetRange(sensorRef,&range);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRange Info- LE_OK."
        " range = %f",range);
        printf("range = %f\n",range);

        double resolution;
        result = taf_imuSensor_GetResolution(sensorRef,&resolution);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetSensorResolution- LE_OK.%f",resolution);
        printf("resolution = %f\n",resolution);
        return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

static le_result_t TestEulerAngle(double pitch, double roll , double yaw)
{
    le_result_t result;
    LE_TEST_INFO("Testing Setting euler angle for Sensor with -taf_imuSensor_SetRefCoordinateByEulerAngle");
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

void TestSensorOnEventFunc(taf_imuSensor_SensorRef_t sensorRef,taf_imuSensor_SampleRef_t ref,
    void* contextPtr){
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
    LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRotatedData- LE_OK. Event size %ld",size);
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
    printf("\033[1;31m %s [%f HZ, %d] Event [%lf Hz, %"PRIu64" ns, %"PRIu64" ns].\033[0m\n",
        sensorName,sampleRate,batch,samplingRateAggregate/count, rawData[0].timestamp,
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
    SensorConfig* config = (SensorConfig*)ctxPtr;
    LE_TEST_INFO("Test_taf_imuSensor_AddOnEventHandler on valid handler reference");
    eventHandlerRef = taf_imuSensor_AddDataHandler(config->sensorRef,TestSensorOnEventFunc,NULL);
    LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
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
            config.sensorRef = sensorRef;
            config.samplingRate = SamplingRate;
            config.batchCount = BatchCount;
            result  =  taf_imuSensor_Activate(sensorRef,SamplingRate,BatchCount);
            LE_TEST_OK(result == LE_OK,"Activate %s",name);
            if(result!=LE_OK) return result;
            le_thread_Sleep(3);
            threadRef1 = le_thread_Create("Thread1", SensorHandler,&config);
            le_thread_Start(threadRef1);
            le_thread_Sleep(30);
            taf_imuSensor_RemoveDataHandler(eventHandlerRef);
            le_thread_Cancel(threadRef1);
            result = taf_imuSensor_Deactivate(sensorRef);
            LE_TEST_OK(result == LE_OK,"Deactivate %s",name);
            if(result!=LE_OK) return result;
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
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
        status = TestSensorInfo(sensorName);
        if(status == LE_NOT_FOUND){
            LE_TEST_INFO("Sensor Name not found %s",sensorName);
        }
        LE_TEST_OK(status ==LE_OK,"Test SensorInfo Succeed %d",status);
    }
    else if (strncmp(testType, "SetAngle", strlen(testType)) == 0){
        LE_TEST_INFO("=======Set Euler Angle Test========");
        CheckNumArgs(numArgs,4);
        double pitch = atof(le_arg_GetArg(1));
        double roll = atof(le_arg_GetArg(2));
        double yaw = atof(le_arg_GetArg(3));
        status = TestEulerAngle(pitch,roll,yaw);
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_SetEulerAngle Succeed");
    }
    else if(strncmp(testType, "Activate", strlen(testType)) == 0){
        LE_TEST_INFO("=======Test Sensor Activation========");
        CheckNumArgs(numArgs,4);
        const char* name = le_arg_GetArg(1);
        double sampleRate = atof(le_arg_GetArg(2));
        double BatchCount = atof(le_arg_GetArg(3));
        status = TestActivateSensor(name,sampleRate,BatchCount);
        if(status == LE_NOT_FOUND){
            LE_TEST_INFO("Sensor Name not found %s",name);
        }
        LE_TEST_OK(status ==LE_OK,"Test taf_imuSensor_Activate Succeed %d",status);
    }
    else{
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }
    DeleteSensorList();
    LE_TEST_EXIT;
}