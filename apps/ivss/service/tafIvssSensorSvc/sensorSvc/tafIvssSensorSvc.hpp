/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFIVSSSENSORSVC_HPP_
#define TAFIVSSSENSORSVC_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include <tafIvssCommon.hpp>
#include <v1/com/qualcomm/qti/telephony/SensorSvcStubDefault.hpp>
#include <atomic>
#include <map>
#include <time.h>
#include "taf_gptpTime.h"

// Heading sensor fixed ID
#define IVSS_SENSOR_HEADING_ID          5
// Heading sensor fixed type value
#define IVSS_SENSOR_HEADING_TYPE        42
// Heading sensor acquisition rate (Hz), taf_locGnss SetAcquisitionRate(100ms)
#define IVSS_SENSOR_HEADING_RATE_HZ     10.0
// Maximum number of IMU sensors (accelerometer + gyroscope)
#define IVSS_SENSOR_MAX_NUM             2
// Maximum total sensor count (including heading sensor)
#define IVSS_SENSOR_MAX_NUM_EX          (IVSS_SENSOR_MAX_NUM + 1)
// Maximum IMU event batch count
#define IVSS_SENSOR_MAX_BATCH_COUNT     50

using namespace v1::com::qualcomm::qti::telephony;

//--------------------------------------------------------------------------------------------------
/**
 * Extended sensor info struct (used internally by GetSensorList)
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t id;                                                     ///< Sensor ID
    char name[TAF_IMUSENSOR_NAME_MAX_SIZE];                          ///< Sensor name
    char vendorName[TAF_IMUSENSOR_NAME_MAX_SIZE];                    ///< Vendor name
    char version[TAF_IMUSENSOR_NAME_MAX_SIZE];                       ///< Version string
    taf_imuSensor_SensorType_t type;                                 ///< Sensor type
    double maxSamplingRate;                                          ///< Max sampling rate (Hz)
    uint32_t minBatchCount;                                          ///< Min batch count
    uint32_t maxBatchCount;                                          ///< Max batch count
    double odr[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];         ///< Supported ODR list
    size_t odrCount;                                                 ///< Number of valid ODR entries
    double resolution;                                               ///< Resolution
    double range;                                                    ///< Range
    bool isHeading;                                                  ///< Whether this is a heading sensor
} taf_IvssSensor_SensorInfoEx_t;

//--------------------------------------------------------------------------------------------------
/**
 * GetSensorList response data struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_IvssSensor_SensorInfoEx_t sensorInfo[IVSS_SENSOR_MAX_NUM_EX]; ///< Sensor info array
    uint32_t sensorNum;                                                ///< Total sensor count
} taf_IvssSensor_GetSensorList_t;

//--------------------------------------------------------------------------------------------------
/**
 * SensorConfig request data struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t sensorId;   ///< Sensor ID
    float samplingRate; ///< Requested sampling rate (Hz)
    int32_t batchCount; ///< Requested batch count
} taf_IvssSensor_SensorConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * SensorControl request data struct
 * Note: sensorState uses int32_t instead of SensorSvcTypes::SensorStateT to avoid
 * compilation errors from non-trivial types in unions (deleted destructor).
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t sensorId;    ///< Sensor ID
    int32_t sensorState; ///< Target state (integer value of SensorSvcTypes::SensorStateT)
} taf_IvssSensor_SensorControl_t;

//--------------------------------------------------------------------------------------------------
/**
 * Generic method call indication struct
 * Used for synchronous communication between CommonAPI thread and Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semRef; ///< Synchronization semaphore
    le_result_t result;  ///< Operation result
    union
    {
        taf_IvssSensor_GetSensorList_t getSensorList;
        taf_IvssSensor_SensorConfig_t sensorConfig;
        taf_IvssSensor_SensorControl_t sensorControl;
    };
} taf_IvssSensor_Ind_t;

//--------------------------------------------------------------------------------------------------
/**
 * IMU sensor runtime state struct (one instance per IMU sensor)
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_imuSensor_SensorRef_t sensorRef;                             ///< taf_imuSensor reference
    double samplingRate;                                             ///< Configured sampling rate (Hz)
    uint32_t batchCount;                                             ///< Configured batch count
    bool isConfigured;                                               ///< Whether configured
    bool isEnabled;                                                  ///< Whether enabled
    int32_t enableRefCount;                                          ///< Enable reference count
    taf_imuSensor_DataHandlerRef_t dataHandlerRef;                   ///< Data callback handle
    taf_imuSensor_CapabilityUpdateHandlerRef_t capabilityHandlerRef; ///< Capability callback handle
} taf_IvssSensor_SensorState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Heading sensor runtime state struct
 * Data source: taf_locGnss GetBodyFrameData (QDR DR engine yaw angle)
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isEnabled;                                      ///< Whether enabled
    int32_t enableRefCount;                              ///< Enable reference count
    taf_locGnss_PositionHandlerRef_t positionHandlerRef; ///< taf_locGnss position callback handle
} taf_IvssSensor_HeadingState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Map le_result_t to SensorSvcTypes::SensorReturnT
 */
//--------------------------------------------------------------------------------------------------
static inline SensorSvcTypes::SensorReturnT ResultLeToSensorReturn(le_result_t result)
{
    switch (result)
    {
        case LE_OK:
            return SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS;
        case LE_BAD_PARAMETER:
            return SensorSvcTypes::SensorReturnT::SENSOR_RETURN_ERROR_INVALID_INPUT_PARAMETER;
        case LE_NOT_FOUND:
            return SensorSvcTypes::SensorReturnT::SENSOR_RETURN_ERROR_NO_SENSORS_FOUND;
        default:
            // All other LE errors return UNKNOWN
            LE_ERROR("ResultLeToSensorReturn: le_result=%d -> SENSOR_RETURN_ERROR_UNKNOWN",
                static_cast<int>(result));
            return SensorSvcTypes::SensorReturnT::SENSOR_RETURN_ERROR_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Map taf_imuSensor_SensorType_t to SensorSvcTypes::SensorTypeT
 * Accelerometer and gyroscope are mapped to their uncalibrated types.
 */
//--------------------------------------------------------------------------------------------------
inline SensorSvcTypes::SensorTypeT SensorTypeToIvss(taf_imuSensor_SensorType_t sensorType)
{
    SensorSvcTypes::SensorTypeT ret = SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_UNKNOWN;
    switch (sensorType)
    {
        case TAF_IMUSENSOR_ACCELEROMETER:
            ret = SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_ACCELEROMETER_UNCALIBRATED;
            break;
        case TAF_IMUSENSOR_GYROSCOPE:
            ret = SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_GYROSCOPE_UNCALIBRATED;
            break;
        case TAF_IMUSENSOR_INVALID:
            ret = SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_UNKNOWN;
            break;
        default:
            LE_ERROR("SensorTypeToIvss : Unsupported input (%d)", static_cast<int>(sensorType));
            break;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * IVSS sensor service class
 *
 * Inherits from CommonAPI-generated SensorSvcStubDefault, implements all interfaces:
 * - Client registration/deregistration (multi-client safe, reference counting)
 * - Sensor list query
 * - Sensor configuration (sampling rate, batch count)
 * - Sensor enable/disable control
 * - Service state broadcast (SensorCapabilities)
 * - Configuration update broadcast (SensorConfigUpdate)
 * - IMU data broadcast (SensorImuDataRead, from taf_imuSensor)
 * - Heading data broadcast (SensorHeadingDataRead, from taf_locGnss DR engine)
 */
//--------------------------------------------------------------------------------------------------
class tafIvssSensorSvc: public v1_0::com::qualcomm::qti::telephony::SensorSvcStubDefault
{
public:
    tafIvssSensorSvc() : serviceReady(false) {};
    virtual ~tafIvssSensorSvc() {};

    // Service initialization
    void Init();
    static std::shared_ptr<tafIvssSensorSvc> GetInstance();

    // ---- CommonAPI method implementations ----

    // Register sensor client
    virtual void RegisterSensorClientReq(const std::shared_ptr<CommonAPI::ClientId> _client,
        RegisterSensorClientReqReply_t _reply);

    // Deregister sensor client
    virtual void DeRegisterSensorClientReq(const std::shared_ptr<CommonAPI::ClientId> _client,
        DeRegisterSensorClientReqReply_t _reply);

    // Get sensor list
    virtual void GetSensorListReq(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetSensorListReqReply_t _reply);

    // Configure sensor sampling rate and batch count
    virtual void SensorConfigReq(const std::shared_ptr<CommonAPI::ClientId> _client,
        int32_t _sensorId, float _samplingRate, int32_t _batchCount,
        SensorConfigReqReply_t _reply);

    // Enable or disable sensor
    virtual void SensorControlReq(const std::shared_ptr<CommonAPI::ClientId> _client,
        int32_t _sensorId, SensorSvcTypes::SensorStateT _sensorState,
        SensorControlReqReply_t _reply);

    // ---- Legato event loop handler functions ----

    static void GetSensorListReqHandler(void* reportPtr);
    static void SensorConfigReqHandler(void* reportPtr);
    static void SensorControlReqHandler(void* reportPtr);
    // Deregister cleanup handler (executes taf_imuSensor cleanup in Legato main thread)
    static void DeRegisterReqHandler(void* reportPtr);

    // ---- taf_imuSensor async callbacks ----

    // IMU sensor data callback -> triggers SensorImuDataRead broadcast
    // contextPtr: sensor ID (int32_t*, points to key in sensorStateMap)
    static void OnSensorDataHandler(taf_imuSensor_SampleRef_t sampleRef,
        const taf_imuSensor_DataValue_t* rawData, size_t rawDataCount,
        const taf_imuSensor_DataValue_t* biasData, size_t biasDataCount,
        void* contextPtr);

    // Sensor capability state change callback -> triggers SensorCapabilities broadcast
    // contextPtr: sensor ID (int32_t*)
    static void OnCapabilityUpdateHandler(taf_imuSensor_SensorRef_t sensorRef,
        bool isAvailable, bool isEnabled, uint32_t capabilityMask,
        void* contextPtr);

    // ---- taf_locGnss async callback (heading sensor) ----

    // GNSS position callback -> triggers SensorHeadingDataRead broadcast
    // Data: GetBodyFrameData().yaw (DR engine yaw angle)
    // Timestamp: GetRealTimeInformation() (elapsedRealTimeNs)
    // GPTP: GetGptpTime()
    static void OnPositionHandler(taf_locGnss_SampleRef_t positionSampleRef,
        void* contextPtr);

    // ---- Memory pool ----
    le_mem_PoolRef_t EventPool;

    // ---- Legato event IDs ----
    le_event_Id_t GetSensorListReqEvent;
    le_event_Id_t SensorConfigReqEvent;
    le_event_Id_t SensorControlReqEvent;
    le_event_Id_t DeRegisterReqEvent;

    // ---- Legato event handler references ----
    le_event_HandlerRef_t GetSensorListReqEventHandlerRef;
    le_event_HandlerRef_t SensorConfigReqEventHandlerRef;
    le_event_HandlerRef_t SensorControlReqEventHandlerRef;
    le_event_HandlerRef_t DeRegisterReqEventHandlerRef;

    // ---- Sensor state management ----

    // IMU sensor state map (sensorId -> state)
    std::map<int32_t, taf_IvssSensor_SensorState_t> sensorStateMap;

    // Heading sensor state
    taf_IvssSensor_HeadingState_t headingState;

    // Registered client count (atomic for thread-safe access from CommonAPI thread)
    std::atomic<int32_t> clientCount{0};

    // Overall service availability state (for SensorCapabilities broadcast)
    bool serviceReady;

    // IMU sensor list reference (obtained in Init(), keeps sensorRef valid)
    taf_imuSensor_SensorListRef_t sensorListRef;

    // GPTP hardware clock reference (/dev/ptp0) for boot-to-GPTP timestamp conversion
    taf_gptpTime_Ref_t gptpTimeRef;
};

#endif // TAFIVSSSENSORSVC_HPP_
