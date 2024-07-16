/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "le_singlyLinkedList.h"
#include <map>
#include <telux/sensor/SensorManager.hpp>
#include "telux/common/CommonDefines.hpp"
#include "telux/sensor/SensorDefines.hpp"
#include "telux/sensor/SensorClient.hpp"
#include "tafSvcIF.hpp"

#define SENSOR_EVENT_HANDLER_HIGH 11
#define TAF_SENSOR_MAX_EVENTS_SIZE 100
#define TAF_SENSOR_CLIENT_ACTIVATION_MAX 23
#define TAF_SENSOR_LIST_POOL_SIZE 20
#define TAF_SENSOR_POOL_SIZE 10
#define NAME_MAX_SIZE 50

using namespace telux::sensor;


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
    uint64_t timestamp;
    double x;
    double y;
    double z;
    double xb;
    double yb;
    double zb;
}taf_SensorEvent_t;

typedef struct{
    taf_imuSensor_SensorRef_t sensorRef;
    le_msg_SessionRef_t sessionRef;
    std::vector<std::shared_ptr<taf_SensorEvent_t>> eventList;
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

class tafSensorListener: public ISensorEventListener
{
    public:
        void onEvent(std::shared_ptr<std::vector<SensorEvent>> events) override;
        void onConfigurationUpdate(SensorConfiguration configuration) override;
        le_msg_SessionRef_t* clientSessionRef;
        ~tafSensorListener() {};
};

typedef struct
{
    void* clientRefPtr;
    le_msg_SessionRef_t sessionRef;
    std::shared_ptr<ISensorManager> mSensorManager;
    std::shared_ptr<tafSensorListener> eventListener;
    le_event_Id_t SensorOnEventId;
    taf_SensorEventList_t lastEvent;
    std::vector<std::shared_ptr<ISensorClient>> mSensorClient;
    uint32_t mBatchCount;
    bool isCalibrated;
    le_event_HandlerRef_t HandlerRef;
    bool isSensorActivated;
    taf_imuSensor_SensorRef_t CurrentSensorRef;
    std::shared_ptr<ISensorClient> currentSensorClient;
    le_mutex_Ref_t mSensorMutexRef;
}taf_SensorClient_t;

namespace telux {
namespace tafsvc {
    class taf_Sensor: public ITafSvc
    {
        public:
            taf_Sensor() {};
            ~taf_Sensor();
            void Init();
            int32_t mClientRefCount;
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
            static taf_Sensor &GetInstance();
            le_result_t SetEulerAngle(double,double,double);
            static void InitializeClient(taf_SensorClient_t* clientRequestPtr);
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
            le_result_t Activate(taf_imuSensor_SensorRef_t,double ,uint32_t);
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

        private:
            le_mem_PoolRef_t ClientPoolRef;
            le_ref_MapRef_t ClientRequestRefMap;
            telux::common::ServiceStatus SensorManagerInit(taf_SensorClient_t* clientRequestPtr);
    };
}
}