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

#define SENSOR_EVENT_HANDLER_HIGH 20
#define TAF_SENSOR_MAX_EVENTS_SIZE 100
#define TAF_SENSOR_CLIENT_ACTIVATION_MAX 23

// Assume each client has one sensor list
#define TAF_SENSOR_LIST_POOL_SIZE TAF_SENSOR_CLIENT_ACTIVATION_MAX
#define TAF_SENSOR_POOL_SIZE 10
#define NAME_MAX_SIZE 100
#define SEC_TO_NANOS 1000000000
#define MAX_TIME_OUT 5
#define SENSOR_VERSION_SIZE 20

// -------------------------------------------------------------------------------------------------
// Sensor Info (per sensor)
// -------------------------------------------------------------------------------------------------
typedef struct
{
    int      id;
    taf_imuSensor_SensorType_t sensorType;

    char     name[TAF_IMUSENSOR_NAME_MAX_SIZE];
    char     vendor[TAF_IMUSENSOR_NAME_MAX_SIZE];

    uint32_t sampleRateListSize;
    double   samplingRate[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];
    double   maxSamplingRate;

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
    tafpa::sensor::taf_pa_sensor_Event eventList[TAF_SENSOR_MAX_EVENTS_SIZE];
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

typedef struct
{
    taf_imuSensor_SensorRef_t sensorRef;
    taf_imuSensor_ConfigUpdateHandlerRef_t handlerRef;
    taf_imuSensor_ConfigUpdateHandlerFunc_t handlerFuncPtr;
    void* handlerContextPtr;
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t next;
} taf_SensorConfigUpdateHandler_t;

typedef struct
{
    taf_imuSensor_SensorRef_t sensorRef;
    taf_imuSensor_CapabilityUpdateHandlerRef_t handlerRef;
    taf_imuSensor_CapabilityUpdateHandlerFunc_t handlerFuncPtr;
    void* handlerContextPtr;
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t next;
} taf_SensorCapabilityHandler_t;

typedef struct
{
    taf_imuSensor_SensorRef_t sensorRef;
    double samplingRate;
    uint32_t batchCount;
    bool isRotated;
    le_msg_SessionRef_t sessionRef;
    tafpa::sensor::taf_pa_sensor_SensorId sensorClientId;
} taf_SensorConfigUpdate_t;

typedef struct
{
    taf_imuSensor_SensorRef_t sensorRef;
    bool isAvailable;
    bool isEnabled;
    uint32_t capabilityMask;
    le_msg_SessionRef_t sessionRef;
    tafpa::sensor::taf_pa_sensor_SensorId sensorClientId;
} taf_SensorCapability_t;

typedef struct{
    tafpa::sensor::taf_pa_sensor_SensorId sensorClient;
    tafpa::sensor::taf_pa_sensor_EventListener eventListener;
    bool isSensorActivated;
    taf_imuSensor_SensorRef_t sensorRef;
    char sensorName[TAF_IMUSENSOR_NAME_MAX_SIZE];
}taf_sensorClientInfo_t;

typedef struct
{
    void* clientRefPtr;
    le_msg_SessionRef_t sessionRef;
    uint32_t clientCount;
    taf_sensorClientInfo_t clients[TAF_SENSOR_POOL_SIZE];
}taf_SensorClient_t;

typedef struct{
    tafpa::sensor::taf_pa_sensor_BasicInfo basicInfo;
    tafpa::sensor::taf_pa_sensor_ConfigInfo configInfo;
    tafpa::sensor::taf_pa_sensor_Capabilities capInfo;
}taf_SensorPAInfo_t;

// -------------------------------------------------------------------------------------------------
// Envelope passed between main thread and worker thread (worker thread does PA only)
// -------------------------------------------------------------------------------------------------
typedef struct
{
    taf_imuSensor_ServerCmdRef_t cmdRef;
    le_result_t retCode;

    // identify client/session on main thread (for state update)
    le_msg_SessionRef_t sessionRef;

    // identify sensor in service layer
    taf_imuSensor_SensorRef_t sensorRef;

    // resolved on main thread; worker must use this only (no map access)
    tafpa::sensor::taf_pa_sensor_SensorId paClientId;

    struct { double pitch, roll, yaw; } euler;
    struct { double samplingRate; uint32_t batchCount; } activate;

    struct { taf_imuSensor_SelfTestMode_t mode; uint64_t timestamp; } selfTest;

    // If this PA op changes activation, main thread updates after worker finishes.
    bool changesActivation;
    bool desiredActiveState;
} SensorCmdInfo_t;

namespace tafsvc {
    class taf_Sensor: public ITafSvc
    {
        public:
            taf_Sensor() {};
            ~taf_Sensor();
            void Init();
            int32_t mClientRefCount = 0;

            le_mem_PoolRef_t tSensorListPool  = NULL;
            le_mem_PoolRef_t tSensorInfoPool  = NULL;
            le_mem_PoolRef_t tSensorEventPool = NULL;
            le_mem_PoolRef_t tSensorEventHandlerPool = NULL;
            le_mem_PoolRef_t tSensorEventInfoPool    = NULL;
            le_mem_PoolRef_t CmdSensorPoolRef = NULL;
            le_mem_PoolRef_t ClientPoolRef    = NULL;
            le_mem_PoolRef_t tSensorConfigUpdateHandlerPool = NULL;
            le_mem_PoolRef_t tSensorCapabilityHandlerPool   = NULL;

            le_ref_MapRef_t tSensorListMap    = NULL;
            le_ref_MapRef_t tSensorInfoMap    = NULL;
            le_ref_MapRef_t tSensorEventMap   = NULL;
            le_ref_MapRef_t tSensorEventHandlerMap        = NULL;
            le_ref_MapRef_t ClientRequestRefMap           = NULL;
            le_ref_MapRef_t tSensorConfigUpdateHandlerMap = NULL;
            le_ref_MapRef_t tSensorCapabilityHandlerMap   = NULL;

            le_event_Id_t SensorOnEventId     = NULL;
            le_event_Id_t SelfTestEventId     = NULL;
            le_event_Id_t ConfigUpdateEventId = NULL;
            le_event_Id_t CapabilityEventId   = NULL;

            le_thread_Ref_t SensorSvcThRef    = NULL;    // service main thread
            le_thread_Ref_t SensorWorkerThRef = NULL;    // PA worker thread
            static taf_Sensor &GetInstance();

            // -------- Core non-PA operations (run on service main thread) --------
            taf_imuSensor_SensorListRef_t GetAvailableSensors(le_msg_SessionRef_t sessionRef);
            taf_imuSensor_SensorRef_t GetFirstSensor(taf_imuSensor_SensorListRef_t sensorListRef);
            taf_imuSensor_SensorRef_t GetNextSensor(taf_imuSensor_SensorListRef_t sensorListRef);
            le_result_t DeleteSensorList(taf_imuSensor_SensorListRef_t sensorListRef);
            le_result_t GetSensorId(taf_imuSensor_SensorRef_t,uint32_t*);
            le_result_t GetSensorName(taf_imuSensor_SensorRef_t,char*,size_t);
            le_result_t GetSensorVendorName(taf_imuSensor_SensorRef_t, char*, size_t);
            le_result_t GetSensorType(taf_imuSensor_SensorRef_t, taf_imuSensor_SensorType_t*);
            le_result_t GetSensorVersion(taf_imuSensor_SensorRef_t, char*, size_t);
            le_result_t GetSensorSamplingRateInfo(taf_imuSensor_SensorRef_t, double*, size_t*);
            le_result_t GetSensorBatchingInfo(taf_imuSensor_SensorRef_t, uint32_t*, uint32_t*);
            le_result_t GetSensorRangeInfo(taf_imuSensor_SensorRef_t,double*);
            le_result_t GetSensorResolution(taf_imuSensor_SensorRef_t,double*);

            le_result_t GetData(taf_imuSensor_SampleRef_t sampleRef,
                                taf_imuSensor_DataValue_t* raw, size_t* rawSz,
                                taf_imuSensor_DataValue_t* bias, size_t* biasSz);
            le_result_t DeleteData(taf_imuSensor_SampleRef_t sampleRef);

            // -------- Events/handlers (run on service main thread) --------
            taf_imuSensor_DataHandlerRef_t AddDataHandler(taf_imuSensor_SensorRef_t,
                taf_imuSensor_DataHandlerFunc_t ,void*);
            void RemoveDataHandler(taf_imuSensor_DataHandlerRef_t);

            taf_imuSensor_SelfTestFailedHandlerRef_t AddSelfTestFailedHandler(
                taf_imuSensor_SensorRef_t, taf_imuSensor_SelfTestFailedHandlerFunc_t, void*);
            void RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t);

            taf_imuSensor_ConfigUpdateHandlerRef_t AddConfigUpdateHandler(
                taf_imuSensor_SensorRef_t, taf_imuSensor_ConfigUpdateHandlerFunc_t, void*);
            void RemoveConfigUpdateHandler(taf_imuSensor_ConfigUpdateHandlerRef_t);

            taf_imuSensor_CapabilityUpdateHandlerRef_t AddCapabilityHandler(
                taf_imuSensor_SensorRef_t, taf_imuSensor_CapabilityUpdateHandlerFunc_t, void*);
            void RemoveCapabilityHandler(taf_imuSensor_CapabilityUpdateHandlerRef_t);

            // -------- Client/session mgmt --------
            static taf_SensorClient_t* DiscoverSessionRef(le_msg_SessionRef_t sessionRef);
            void CleanUp(taf_SensorClient_t*);
            void ReleaseClientRef(void* refPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);

            // -------- PA operations: part-1 main thread -> part-2 worker -> report back main --------
            void SetEulerAngle(taf_imuSensor_ServerCmdRef_t cmdRef,
                                   double pitch, double roll, double yaw);

            void Activate(taf_imuSensor_ServerCmdRef_t cmdRef,
                               taf_imuSensor_SensorRef_t sensorRef,
                               double samplingRate, uint32_t batchCount,
                               le_msg_SessionRef_t sessionRef);

            void Deactivate(taf_imuSensor_ServerCmdRef_t cmdRef,
                                 taf_imuSensor_SensorRef_t sensorRef,
                                 le_msg_SessionRef_t sessionRef);

            void SelfTest(taf_imuSensor_ServerCmdRef_t cmdRef,
                               taf_imuSensor_SensorRef_t sensorRef,
                               taf_imuSensor_SelfTestMode_t mode,
                               le_msg_SessionRef_t sessionRef);

            // worker thread entry points (PA only)
            static void SetEulerAngleWorker(void* cmdPtr, void* context);
            static void ActivateWorker(void* cmdPtr, void* context);
            static void DeactivateWorker(void* cmdPtr, void* context);
            static void SelfTestWorker(void* cmdPtr, void* context);

            static void DataEventHandler(void* reportPtr);
            static void SelfTestNotifyClient(void* reportPtr, void* secondLayerHandlerFunc);
            static void ConfigUpdateNotifyClient(void* reportPtr, void* secondLayerHandlerFunc);
            static void CapabilityNotifyClient(void* reportPtr, void* secondLayerHandlerFunc);
            le_result_t GetSensorList(int8_t listSize);

            le_event_HandlerRef_t HandlerRef = NULL;
            std::vector<taf_SensorPAInfo_t> sList;
    };

    class Handler : public ITafSvc
    {
    public:
        void Init() { return; }
        static void onSelfTestFailed(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            uint64_t timestamp, std::any context);
        static void onEvent(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            std::shared_ptr<const std::vector<tafpa::sensor::taf_pa_sensor_Event>> events,
            std::any context);
        static void onConfigUpdate(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            double samplingRate, uint32_t batchCount, bool isRotated, std::any context);
        static void onCapabilityUpdate(tafpa::sensor::taf_pa_sensor_SensorId sensorId,
            tafpa::sensor::taf_pa_sensor_CapabilityInfo capabilityInfo, std::any context);
    };
}