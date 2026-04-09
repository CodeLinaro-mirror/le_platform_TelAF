/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "le_singlyLinkedList.h"
#include "tafSvcIF.hpp"
#include "mutex"
#include "tafSensorPa.hpp"

#define SENSOR_EVENT_HANDLER_HIGH 11
#define TAF_SENSOR_MAX_EVENTS_SIZE 100
#define TAF_SENSOR_CLIENT_ACTIVATION_MAX 23
#define TAF_SENSOR_LIST_POOL_SIZE 20
#define TAF_SENSOR_POOL_SIZE 10
#define NAME_MAX_SIZE 100
#define SEC_TO_NANOS 1000000000
#define MAX_TIME_OUT 5

typedef struct
{
    int id;
    taf_imuSensor_SensorType_t sensorType;
    char name[20];
    char vendor[50];
    uint32_t sampleRateListSize;
    double samplingRate[10];
    double maxSamplingRate;
    uint32_t maxBatchCountSupported;
    uint32_t minBatchCountSupported;
    int range;
    int version;
    double resolution;
    double maxRange;
    le_sls_Link_t link;
    le_sls_Link_t* currPtr;
    taf_imuSensor_SensorRef_t ref;
}taf_SensorInfo_t;

typedef struct
{
    uint32_t sensorListSize;
    le_sls_List_t SensorsList;
    le_sls_Link_t* currPtr;
    le_msg_SessionRef_t sessionRef;
    taf_imuSensor_SensorListRef_t ref;
}taf_SensorInfoList_t;

typedef struct{
    tafpa::sensor::taf_pa_sensor_SensorId sensorClientId;
    le_msg_SessionRef_t sessionRef;
    std::shared_ptr<const std::vector<tafpa::sensor::taf_pa_sensor_Event>> eventList;
    uint32_t listSize;
}taf_SensorEventList_t;

typedef struct{
    taf_imuSensor_SampleRef_t ref;
    taf_SensorEventList_t* eventPtr;
    le_msg_SessionRef_t sessionRef;
}taf_SensorEventInfo_t;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    taf_imuSensor_DataHandlerRef_t handlerRef;
    taf_imuSensor_DataHandlerFunc_t handlerFuncPtr;
    void* handlerContextPtr;
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t next;
}taf_SensorEventHandler_t;

typedef struct
{
    uint64_t timestamp;
    le_msg_SessionRef_t sessionRef;
    tafpa::sensor::taf_pa_sensor_SensorId sensorClientId;
}
taf_SensorSelfTest_t;

typedef struct{
    tafpa::sensor::taf_pa_sensor_SensorId sensorClient;
    tafpa::sensor::taf_pa_sensor_EventListener eventListener;
    bool isSensorActivated;
    taf_imuSensor_SensorRef_t sensorRef;
    char sensorName[NAME_MAX_SIZE];
}taf_sensorClientInfo_t;

typedef struct
{
    void* clientRefPtr;
    le_msg_SessionRef_t sessionRef;
    std::vector<std::shared_ptr<taf_sensorClientInfo_t>> clients;
}taf_SensorClient_t;

typedef struct{
    tafpa::sensor::taf_pa_sensor_BasicInfo basicInfo;
    tafpa::sensor::taf_pa_sensor_ConfigInfo configInfo;
    tafpa::sensor::taf_pa_sensor_Capabilities capInfo;
}taf_SensorPAInfo_t;

namespace tafsvc {
    class taf_Sensor: public ITafSvc
    {
        public:
            taf_Sensor() {};
            ~taf_Sensor();
            void Init();
            int32_t mClientRefCount;
            int32_t numOfSelfTestEventHandler;
            int32_t numofSensorEventHandlers;
            le_mem_PoolRef_t tSensorListPool;
            le_mem_PoolRef_t tSensorInfoPool;
            le_mem_PoolRef_t tSensorEventPool;
            le_mem_PoolRef_t tSensorEventHandlerPool;
            le_mem_PoolRef_t tSensorEventInfoPool;
            le_ref_MapRef_t tSensorListMap;
            le_ref_MapRef_t tSensorInfoMap;
            le_ref_MapRef_t tSensorEventHandlerMap;
            le_ref_MapRef_t tSensorEventMap;
            le_event_Id_t SensorOnEventId;
            le_event_Id_t SelfTestEventId;
            static taf_Sensor &GetInstance();
            le_result_t SetEulerAngle(double,double,double);
            static le_result_t InitializeSensorClientList(taf_SensorClient_t* clientRequestPtr);
            static taf_SensorClient_t* DiscoverSessionRef(le_msg_SessionRef_t sessionRef);
            static taf_SensorClient_t* AcquireSessionRef(void);
            void ReleaseClientRef(void* RefPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            taf_imuSensor_SensorRef_t GetFirstSensor(taf_imuSensor_SensorListRef_t SensorListRef);
            taf_imuSensor_SensorRef_t GetNextSensor(taf_imuSensor_SensorListRef_t SensorListRef);
            le_result_t DeleteSensorList(taf_imuSensor_SensorListRef_t SensorListRef);
            le_result_t GetSensorId(taf_imuSensor_SensorRef_t,uint32_t*);
            le_result_t GetSensorName(taf_imuSensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorVendorName(taf_imuSensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorType(taf_imuSensor_SensorRef_t,taf_imuSensor_SensorType_t*);
            le_result_t GetSensorVersion(taf_imuSensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorSamplingRateInfo(taf_imuSensor_SensorRef_t,double*,size_t*);
            le_result_t GetSensorBatchingInfo(taf_imuSensor_SensorRef_t,uint32_t*,uint32_t*);
            le_result_t GetSensorRangeInfo(taf_imuSensor_SensorRef_t,double*);
            le_result_t GetSensorResolution(taf_imuSensor_SensorRef_t,double*);
            taf_imuSensor_SelfTestFailedHandlerRef_t AddSelfTestFailedHandler
                (taf_imuSensor_SensorRef_t,taf_imuSensor_SelfTestFailedHandlerFunc_t,void*);
            void RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t);
            static void FirstLayerSelfTestHandler(void*,void*);
            le_result_t Activate(taf_imuSensor_SensorRef_t,double ,uint32_t);
            le_result_t SelfTest(taf_imuSensor_SensorRef_t,taf_imuSensor_SelfTestMode_t,uint64_t*);
            le_result_t Deactivate(taf_imuSensor_SensorRef_t sensorRef);
            void CleanUp(taf_SensorClient_t*);
            taf_imuSensor_DataHandlerRef_t AddDataHandler(taf_imuSensor_SensorRef_t,
                taf_imuSensor_DataHandlerFunc_t ,void*);
            void RemoveDataHandler(taf_imuSensor_DataHandlerRef_t);
            static void SensorDataEvent(void* reportPtr,void* secondLayerHandlerFunc);
            le_result_t GetData(taf_imuSensor_SampleRef_t,taf_imuSensor_DataValue_t*,size_t*
            ,taf_imuSensor_DataValue_t*,size_t*);
            taf_imuSensor_SensorListRef_t GetAvailableSensors();
            le_result_t DeleteData(taf_imuSensor_SampleRef_t);
            static void DataEventHandler(void* reportPtr);
            le_result_t GetSensorList(int8_t listSize);

        private:
            le_mem_PoolRef_t ClientPoolRef;
            le_event_HandlerRef_t HandlerRef;
            le_ref_MapRef_t ClientRequestRefMap;
            std::mutex mtx;
            std::vector<taf_SensorPAInfo_t> sList;
    };

    class Handler : public ITafSvc
    {
    public:
        void Init() {
            return;
        }
        static void onSelfTestFailed(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            uint64_t timestamp,std::any context);
        static void onEvent(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            std::shared_ptr<const std::vector<tafpa::sensor::taf_pa_sensor_Event>> events,
            std::any context);
    };
}
