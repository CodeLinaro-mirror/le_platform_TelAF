/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <time.h>

#define MAX_SYSTEM_CMD_LENGTH 200
#define SENSOR_NUMS 2
taf_imuSensor_SensorRef_t sensorsList[SENSOR_NUMS];
taf_imuSensor_DataHandlerRef_t eventHandlerRef,eventHandlerRef1;
taf_imuSensor_SelfTestFailedHandlerRef_t selfTestHandlerRef;
le_thread_Ref_t threadRef1 =NULL;
taf_imuSensor_SensorListRef_t Head;
static le_sem_Ref_t semRef1;
static le_mutex_Ref_t mSensorMutexRef;
le_event_Id_t ReactivateEventId;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    double samplingRate;
    uint32_t batchCount;
} SensorConfig;

typedef struct{
    //1 . Add HANDLER
    //2 . Activate
    //2 . Deactivate
    //4 . Remove HANDLER
    //5 . DeleteSensorList
    int step;
} taf_ReactivateTestCase_t;

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
         "app runProc tafSensorIntTest tafSensorIntTest -- Reactivate\n"
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

void TestSensorOnEventFunc(taf_imuSensor_SensorRef_t sensorRef,taf_imuSensor_SampleRef_t ref,
    void* contextPtr){
    le_mutex_Lock(mSensorMutexRef);
    for(int i=0;i<SENSOR_NUMS;i++){
        if(sensorRef == configList[i].sensorRef){
        char sensorName[50];
        le_result_t result = taf_imuSensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        if(result !=LE_OK){
            LE_TEST_INFO("sensor ref not found %p", sensorRef);
            le_mutex_Unlock(mSensorMutexRef);
            return;
        }
        double sampleRate = configList[i].samplingRate;
        uint32_t batch = configList[i].batchCount;
        LE_TEST_INFO("Test onEvent Retrieval for SensorName %s",sensorName);
        taf_imuSensor_DataValue_t rawData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
        taf_imuSensor_DataValue_t biasData[TAF_IMUSENSOR_MAX_SUPPORTED_BATCH_COUNT];
        size_t  size = sizeof(rawData)/sizeof(taf_imuSensor_DataValue_t);
        result = taf_imuSensor_GetRotatedData(ref,rawData,&size,biasData,&size);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_GetRotatedData- LE_OK. Event size %zu",size);
        result = taf_imuSensor_DeleteData(ref);
        LE_TEST_OK(result == LE_OK, "taf_imuSensor_DeleteData- LE_OK.");
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
        printf("\033[1;31m %s [%f HZ, %d] Event [%lf Hz, %d, %"PRIu64" ns, %"PRIu64" ns].\033[0m\n",
          sensorName,sampleRate,batch,samplingRateAggregate/(count+1),count+1,rawData[0].timestamp,
          rawData[size-1].timestamp);
        le_sem_Post(semRef1);
        }
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
    eventHandlerRef = taf_imuSensor_AddDataHandler(config->sensorRef,TestSensorOnEventFunc,NULL);
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
            SensorConfig c1 = {sensorRef,SamplingRate,BatchCount};
            c1.sensorRef = sensorRef;
            c1.samplingRate = SamplingRate;
            c1.batchCount = BatchCount;
            configList[0] = c1;
            SensorConfig c2 = {NULL,0,0};
            configList[1] = c2;
            result  =  taf_imuSensor_Activate(sensorRef,SamplingRate,BatchCount);
            LE_TEST_OK(result == LE_OK,"Activate %s",name);
            if(result!=LE_OK) return result;
            le_thread_Sleep(3);
            threadRef1 = le_thread_Create("Thread1", SensorHandler,&c1);
            le_thread_Start(threadRef1);
            le_thread_Sleep(30);
            taf_imuSensor_RemoveSelfTestFailedHandler(selfTestHandlerRef);
            taf_imuSensor_RemoveDataHandler(eventHandlerRef);
            result = taf_imuSensor_Deactivate(sensorRef);
            LE_TEST_OK(result == LE_OK,"Deactivate %s",name);
            if(result!=LE_OK) return result;
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

static void* AllSensorHandler(void* ctxPtr)
{
    taf_imuSensor_ConnectService();
    eventHandlerRef = taf_imuSensor_AddDataHandler(sensorsList[0],TestSensorOnEventFunc,NULL);
    LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
        " is successfull");

    eventHandlerRef1 = taf_imuSensor_AddDataHandler(sensorsList[1],TestSensorOnEventFunc,NULL);
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
            if(result!=LE_OK) return LE_NOT_FOUND;
        }
    }
    return LE_OK;
}

static void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
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
    LE_TEST_EXIT;
}

static void ReactivateMainFunc(void* reportPtr)
{
    taf_ReactivateTestCase_t* ptr = (taf_ReactivateTestCase_t*) reportPtr;
    le_result_t result;
    if(ptr->step==1){
        eventHandlerRef = taf_imuSensor_AddDataHandler(sensorsList[0],TestSensorOnEventFunc,NULL);
        LE_TEST_OK(eventHandlerRef != NULL, "Register AddOnEventHandler handler"
        " is successfull");
        eventHandlerRef1 = taf_imuSensor_AddDataHandler(sensorsList[1],TestSensorOnEventFunc,NULL);
        LE_TEST_OK(eventHandlerRef1 != NULL, "Register AddOnEventHandler1 handler"
        " is successfull");
    }
    else if(ptr->step==2)
    {
        SensorConfig c1 = {sensorsList[0],104,50};
        SensorConfig c2 = {sensorsList[1],104,50};
        configList[0] = c1;
        configList[1] = c2;
        result = taf_imuSensor_Activate(sensorsList[0],104.0,50);
        LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
        result = taf_imuSensor_Activate(sensorsList[1],104.0,50);
        LE_INFO("SensorHandler Result of activating sensor: %d", (int)result);
    }
    else if(ptr->step==3)
    {
        result = taf_imuSensor_Deactivate(sensorsList[0]);
        LE_INFO("SensorHandler Result of deactivating sensor: %d", (int)result);
        result = taf_imuSensor_Deactivate(sensorsList[1]);
        LE_INFO("SensorHandler Result of deactivating sensor: %d", (int)result);
    }
    else if(ptr->step==4){
        taf_imuSensor_RemoveDataHandler(eventHandlerRef);
        taf_imuSensor_RemoveDataHandler(eventHandlerRef1);
    }
    else if(ptr->step == 5)
    {
        DeleteSensorList();
    }
}

static void* ReactivateEventHandler(void* contextPtr)
{
    taf_ReactivateTestCase_t event;
    event.step=1;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(2);
    event.step=2;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(30);
    event.step=3;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(2);
    event.step=4;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(2);
    event.step=1;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(2);
    event.step=2;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    le_thread_Sleep(30);
    event.step=5;
    le_event_Report(ReactivateEventId,&event,sizeof(event));
    return NULL;
}

static void TestReactivateSensor()
{
    threadRef1 = le_thread_Create("Thread1", ReactivateEventHandler,NULL);
    le_thread_Start(threadRef1);
}


COMPONENT_INIT
{
    semRef1 = le_sem_Create("SemRef1", 0);
    mSensorMutexRef = le_mutex_CreateRecursive("SensorMutexCl");
    ReactivateEventId = le_event_CreateId("ReactivateEventId",sizeof(taf_ReactivateTestCase_t));
    le_event_AddHandler("ReactivateEventHandler",
        ReactivateEventId,ReactivateMainFunc);
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
        DeleteSensorList();
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
        DeleteSensorList();
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
        DeleteSensorList();
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
        DeleteSensorList();
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
        DeleteSensorList();
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
        DeleteSensorList();
    }
    else if(strncmp(testType, "Reactivate", strlen(testType)) == 0){
        TestReactivateSensor();
    }
    else{
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }
}