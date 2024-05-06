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

#define TAF_SENSOR_CLIENT_ACTIVATION_MAX 10
#define TAF_SENSOR_LIST_POOL_SIZE 20
#define TAF_SENSOR_POOL_SIZE 10
#define NAME_MAX_SIZE 50

using namespace telux::sensor;


typedef struct
{
    int id;
    taf_sensor_SensorType_t sensorType;
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
    taf_sensor_SensorRef_t ref;
}taf_SensorInfo_t;

typedef struct
{
    uint32_t sensorListSize;
    le_sls_List_t SensorsList;
    le_sls_Link_t* currPtr;
    le_msg_SessionRef_t sessionRef;
    taf_sensor_SensorListRef_t ref;
}taf_SensorInfoList_t;

typedef struct
{
    void* clientRefPtr;
    le_msg_SessionRef_t sessionRef;
    std::shared_ptr<ISensorManager> mSensorManager;
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
            le_mem_PoolRef_t tSensorListPool;
            le_mem_PoolRef_t tSensorInfoPool;
            le_ref_MapRef_t tSensorListMap;
            le_ref_MapRef_t tSensorInfoMap;
            static taf_Sensor &GetInstance();
            le_result_t SetEulerAngle(double,double,double);
            static void InitializeClient(taf_SensorClient_t* clientRequestPtr);
            static taf_SensorClient_t* DiscoverSessionRef(le_msg_SessionRef_t sessionRef);
            static taf_SensorClient_t* AcquireSessionRef(void);
            void ReleaseClientRef(void* RefPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            taf_sensor_SensorRef_t GetFirstSensor(taf_sensor_SensorListRef_t SensorListRef);
            taf_sensor_SensorRef_t GetNextSensor(taf_sensor_SensorListRef_t SensorListRef);
            le_result_t DeleteSensorList(taf_sensor_SensorListRef_t SensorListRef);
            le_result_t GetSensorId(taf_sensor_SensorRef_t,uint32_t*);
            le_result_t GetSensorName(taf_sensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorVendorName(taf_sensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorType(taf_sensor_SensorRef_t,taf_sensor_SensorType_t*);
            le_result_t GetSensorVersion(taf_sensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorSamplingRateInfo(taf_sensor_SensorRef_t,double*,size_t*);
            le_result_t GetSensorBatchingInfo(taf_sensor_SensorRef_t,uint32_t*,uint32_t*);
            le_result_t GetSensorRangeInfo(taf_sensor_SensorRef_t,double*);
            le_result_t GetSensorResolution(taf_sensor_SensorRef_t,double*);
            taf_sensor_SensorListRef_t GetAvailableSensors();

        private:
            le_mem_PoolRef_t ClientPoolRef;
            le_ref_MapRef_t ClientRequestRefMap;
            telux::common::ServiceStatus SensorManagerInit(taf_SensorClient_t* clientRequestPtr);
    };
}
}