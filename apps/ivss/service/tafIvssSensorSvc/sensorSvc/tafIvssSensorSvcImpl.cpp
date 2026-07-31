/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIvssSensorSvc.hpp"
#include <cmath>

using namespace v1::com::qualcomm::qti::telephony;

// Pi constant (for radian to degree conversion)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Calculate GPTP vs boot time offset (nanoseconds).
 * Reads /dev/ptp0 via taf_gptpTime_GetTimeValue once per batch, applied per event.
 */
//--------------------------------------------------------------------------------------------------
static int64_t GetGptpBootOffset(taf_gptpTime_Ref_t gptpTimeRef)
{
    if (gptpTimeRef == NULL)
    {
        LE_DEBUG("GetGptpBootOffset: gptpTimeRef is NULL, returning 0");
        return 0;
    }

    // Read current GPTP clock (hardware /dev/ptp0)
    struct timespec gptpNow = {0, 0};
    le_result_t res = taf_gptpTime_GetTimeValue(gptpTimeRef, &gptpNow);
    if (res != LE_OK)
    {
        LE_DEBUG("GetGptpBootOffset: taf_gptpTime_GetTimeValue failed (%s)", LE_RESULT_TXT(res));
        return 0;
    }

    // Read current boot time (same source as IMU event timestamps)
    struct timespec bootNow = {0, 0};
    clock_gettime(CLOCK_BOOTTIME, &bootNow);

    // Calculate and return GPTP vs boot time offset (nanoseconds)
    uint64_t gptpNowNs = (uint64_t)gptpNow.tv_sec * 1000000000ULL + (uint64_t)gptpNow.tv_nsec;
    uint64_t bootNowNs = (uint64_t)bootNow.tv_sec * 1000000000ULL + (uint64_t)bootNow.tv_nsec;
    return (int64_t)gptpNowNs - (int64_t)bootNowNs;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get service singleton instance
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssSensorSvc> tafIvssSensorSvc::GetInstance()
{
    static std::shared_ptr<tafIvssSensorSvc> instance = std::make_shared<tafIvssSensorSvc>();
    return instance;
}

// ============================================================================
// CommonAPI method implementations
// ============================================================================

//--------------------------------------------------------------------------------------------------
/**
 * Register sensor client.
 * Creates a new client session supporting concurrent multi-client access.
 * First client registration broadcasts service ready state.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::RegisterSensorClientReq(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    RegisterSensorClientReqReply_t _reply)
{
    LE_INFO("tafIvssSensorSvc RegisterSensorClient, clientCount=%d->%d",
        clientCount.load(), clientCount.load() + 1);
    clientCount++;

    // First client registration: broadcast service ready state
    if (clientCount == 1)
    {
        serviceReady = true;
        LE_INFO("SensorCapabilities: mask=READY (first client registered)");
        fireSensorCapabilitiesEvent(
            SensorSvcTypes::SensorServiceStateMaskT::SENSOR_SERVICE_STATE_MASK_T_READY);
    }

    LE_INFO("tafIvssSensorSvc RegisterSensorClient reply: SensorReturnT=SUCCESS");
    _reply(SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * DeRegister Legato event handler function.
 * Executes in Legato main thread to safely call taf_imuSensor_* cleanup.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::DeRegisterReqHandler(void* reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)reportPtr;
    indPtr->result = LE_OK;

    auto instance = tafIvssSensorSvc::GetInstance();

    // Stop all enabled IMU sensors
    // Keep dataHandlerRef (registered once in Init()), only Deactivate to stop data flow
    for (auto& pair : instance->sensorStateMap)
    {
        taf_IvssSensor_SensorState_t& state = pair.second;
        if (state.isEnabled && state.sensorRef != NULL)
        {
            le_result_t deactRes = taf_imuSensor_Deactivate(state.sensorRef);
            if (deactRes != LE_OK)
            {
                LE_WARN("DeRegisterReqHandler: Deactivate failed for sensorId=%d - %s",
                    pair.first, LE_RESULT_TXT(deactRes));
            }
            state.isEnabled = false;
            state.enableRefCount = 0;
            state.isConfigured = false;
            state.samplingRate = 0.0;
            state.batchCount = 0;
        }
    }

    // Stop heading sensor (keep positionHandlerRef registered for next client session)
    if (instance->headingState.isEnabled)
    {
        le_result_t stopRes = taf_locGnss_Stop();
        if (stopRes != LE_OK)
        {
            LE_WARN("DeRegisterReqHandler: taf_locGnss_Stop failed - %s",
                LE_RESULT_TXT(stopRes));
        }
        instance->headingState.isEnabled = false;
        instance->headingState.enableRefCount = 0;
    }

    instance->serviceReady = false;
    LE_INFO("SensorCapabilities: mask=UNKNOWN (last client deregistered)");
    instance->fireSensorCapabilitiesEvent(
        SensorSvcTypes::SensorServiceStateMaskT::SENSOR_SERVICE_STATE_MASK_T_UNKNOWN);
    LE_INFO("DeRegisterReqHandler: All sensors stopped");

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister sensor client.
 * Deletes client session and cleans up sensor states.
 * Last client deregistration stops all data acquisition.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::DeRegisterSensorClientReq(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    DeRegisterSensorClientReqReply_t _reply)
{
    if (clientCount <= 0)
    {
        LE_WARN("tafIvssSensorSvc DeRegisterSensorClient: no registered clients, skip");
        LE_INFO("tafIvssSensorSvc DeRegisterSensorClient reply: clientCount=%d SensorReturnT=SUCCESS",
            (int32_t)clientCount.load());
        _reply(SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS);
        return;
    }

    LE_INFO("tafIvssSensorSvc DeRegisterSensorClient, clientCount=%d->%d",
        (int32_t)clientCount.load(), (int32_t)(clientCount.load() - 1));
    --clientCount;

    // Last client: route cleanup through Legato event loop
    if (clientCount == 0)
    {
        taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)le_mem_ForceAlloc(EventPool);
        *indPtr = taf_IvssSensor_Ind_t{};
        indPtr->semRef = le_sem_Create("Ivss DeRegisterSem", 0);

        le_event_ReportWithRefCounting(DeRegisterReqEvent, (void*)indPtr);
        le_sem_Wait(indPtr->semRef);

        le_sem_Delete(indPtr->semRef);
        le_mem_Release(indPtr);
    }

    LE_INFO("tafIvssSensorSvc DeRegisterSensorClient reply: clientCount=%d SensorReturnT=SUCCESS",
        (int32_t)clientCount.load());
    _reply(SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * GetSensorList Legato event handler function
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::GetSensorListReqHandler(void* reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)reportPtr;
    indPtr->result = LE_OK;
    indPtr->getSensorList.sensorNum = 0;

    auto instance = tafIvssSensorSvc::GetInstance();

    if (instance->sensorStateMap.empty())
    {
        LE_INFO("GetSensorListHandler: No IMU sensors in sensorStateMap");
        // Still append heading sensor (see below)
    }

    // Use sensorRef stored in Init(), no need to call GetSensorList() again.
    // Each sensor is processed independently; failure skips that sensor only.
    uint32_t idx = 0;
    for (auto& pair : instance->sensorStateMap)
    {
        if (idx >= IVSS_SENSOR_MAX_NUM) break;

        taf_imuSensor_SensorRef_t sensorRef = pair.second.sensorRef;
        if (sensorRef == NULL) continue;

        taf_IvssSensor_SensorInfoEx_t* info = &indPtr->getSensorList.sensorInfo[idx];
        memset(info, 0, sizeof(*info));
        info->isHeading = false;

        // Get identity fields: skip sensor on any failure, does not affect others
        le_result_t r = LE_OK;

        r = taf_imuSensor_GetId(sensorRef, &info->id);
        if (r != LE_OK)
        {
            LE_ERROR("GetId fail sensorRef=%p - %s", sensorRef, LE_RESULT_TXT(r));
            continue;
        }

        r = taf_imuSensor_GetName(sensorRef, info->name, sizeof(info->name));
        if (r != LE_OK)
        {
            LE_ERROR("GetName fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        r = taf_imuSensor_GetVendorName(sensorRef, info->vendorName, sizeof(info->vendorName));
        if (r != LE_OK)
        {
            LE_ERROR("GetVendorName fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        r = taf_imuSensor_GetVersion(sensorRef, info->version, sizeof(info->version));
        if (r != LE_OK)
        {
            LE_ERROR("GetVersion fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        r = taf_imuSensor_GetType(sensorRef, &info->type);
        if (r != LE_OK)
        {
            LE_ERROR("GetType fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        // Get capability parameters: skip sensor on failure
        info->odrCount = TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE;
        r = taf_imuSensor_GetSupportedSamplingRate(sensorRef, info->odr, &info->odrCount);
        if (r != LE_OK)
        {
            LE_ERROR("GetSupportedSamplingRate fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }
        if (info->odrCount > 0)
        {
            info->maxSamplingRate = info->odr[0];
            for (size_t i = 1; i < info->odrCount; i++)
            {
                if (info->odr[i] > info->maxSamplingRate)
                {
                    info->maxSamplingRate = info->odr[i];
                }
            }
        }

        r = taf_imuSensor_GetSupportedBatchCount(sensorRef,
            &info->maxBatchCount, &info->minBatchCount);
        if (r != LE_OK)
        {
            LE_ERROR("GetSupportedBatchCount fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        r = taf_imuSensor_GetRange(sensorRef, &info->range);
        if (r != LE_OK)
        {
            LE_ERROR("GetRange fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }
        // Unit conversion: accelerometer G->m/s^2, gyroscope dps->rad/s
        if (info->type == TAF_IMUSENSOR_ACCELEROMETER)
        {
            info->range = info->range * 9.80665;
        }
        else if (info->type == TAF_IMUSENSOR_GYROSCOPE)
        {
            info->range = info->range * (M_PI / 180.0);
        }

        r = taf_imuSensor_GetResolution(sensorRef, &info->resolution);
        if (r != LE_OK)
        {
            LE_ERROR("GetResolution fail id=%u - %s", info->id, LE_RESULT_TXT(r));
            continue;
        }

        indPtr->getSensorList.sensorNum++;
        idx++;
    }

    // Append heading sensor (fixed parameters, from taf_locGnss DR engine)
    // idx max is IVSS_SENSOR_MAX_NUM=2, sensorInfo[IVSS_SENSOR_MAX_NUM_EX=3] is safe
    taf_IvssSensor_SensorInfoEx_t* headInfo = &indPtr->getSensorList.sensorInfo[idx];
    memset(headInfo, 0, sizeof(*headInfo));
    headInfo->isHeading = true;
    headInfo->id = IVSS_SENSOR_HEADING_ID;
    snprintf(headInfo->name, sizeof(headInfo->name), "Heading");
    snprintf(headInfo->vendorName, sizeof(headInfo->vendorName), "Qcom");
    snprintf(headInfo->version, sizeof(headInfo->version), "1");
    headInfo->type = TAF_IMUSENSOR_INVALID; // Special: mapped to HEADING in GetSensorList
    headInfo->maxSamplingRate = IVSS_SENSOR_HEADING_RATE_HZ;
    headInfo->minBatchCount = 1;
    headInfo->maxBatchCount = 1;
    headInfo->odr[0] = IVSS_SENSOR_HEADING_RATE_HZ;
    headInfo->odrCount = 1;
    headInfo->resolution = 0.0;
    headInfo->range = 0.0;
    indPtr->getSensorList.sensorNum++;

    indPtr->result = LE_OK;
    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get sensor list (CommonAPI method implementation).
 * Returns full capability info for all sensors including heading sensor.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::GetSensorListReq(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    GetSensorListReqReply_t _reply)
{
    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)le_mem_ForceAlloc(EventPool);
    *indPtr = taf_IvssSensor_Ind_t{};
    indPtr->semRef = le_sem_Create("Ivss GetSensorListSem", 0);

    le_event_ReportWithRefCounting(GetSensorListReqEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    std::vector<SensorSvcTypes::SensorInfoT> sensorInfoVec = {};
    for (uint32_t i = 0; i < indPtr->getSensorList.sensorNum; i++)
    {
        taf_IvssSensor_SensorInfoEx_t* src = &indPtr->getSensorList.sensorInfo[i];
        SensorSvcTypes::SensorInfoT info = {};

        // sensorVersion: taf_imuSensor returns String, converted to Int32 via atoi()
        info.setSensorVersion((int32_t)atoi(src->version));
        info.setName(std::string(src->name));
        info.setVendor(std::string(src->vendorName));
        info.setSensorId((int32_t)src->id);
        info.setMaxSamplingRate((float)src->maxSamplingRate);
        info.setMinBatchCount((int32_t)src->minBatchCount);
        info.setMaxBatchCount((int32_t)src->maxBatchCount);
        info.setResolution((float)src->resolution);
        info.setMaxRange((float)src->range);

        // Set ODR list
        std::vector<float> odrVec;
        for (size_t j = 0; j < src->odrCount; j++)
        {
            odrVec.push_back((float)src->odr[j]);
        }
        info.setOdr(odrVec);

        // Set sensor type (Int32)
        if (src->isHeading)
        {
            info.setSensorType(static_cast<int32_t>(
                SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_HEADING));
        }
        else
        {
            info.setSensorType(static_cast<int32_t>(SensorTypeToIvss(src->type)));
        }

        sensorInfoVec.push_back(info);
    }

    LE_INFO("tafIvssSensorSvc GetSensorList reply: count=%d",
        (int32_t)indPtr->getSensorList.sensorNum);
    // GetSensorListReq has no SensorResponse return value
    _reply(sensorInfoVec, (int32_t)indPtr->getSensorList.sensorNum);

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * SensorConfig Legato event handler function.
 * Configures sensor sampling rate and batch count.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::SensorConfigReqHandler(void* reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)reportPtr;
    int32_t sensorId = indPtr->sensorConfig.sensorId;
    float samplingRate = indPtr->sensorConfig.samplingRate;
    int32_t batchCount = indPtr->sensorConfig.batchCount;

    auto instance = tafIvssSensorSvc::GetInstance();

    // Heading sensor: fixed 10Hz, only store configuration
    if (sensorId == IVSS_SENSOR_HEADING_ID)
    {
        // Heading uses fixed params regardless of client request; log actual broadcast values
        LE_INFO("SensorConfigUpdate: sensorId=%d samplingRate=%.2f batchCount=1 (heading fixed)",
            IVSS_SENSOR_HEADING_ID, (float)IVSS_SENSOR_HEADING_RATE_HZ);
        instance->fireSensorConfigUpdateEvent(IVSS_SENSOR_HEADING_ID,
            (float)IVSS_SENSOR_HEADING_RATE_HZ, 1);
        indPtr->result = LE_OK;
        le_sem_Post(indPtr->semRef);
        return;
    }

    // IMU sensor: look up sensor reference
    auto it = instance->sensorStateMap.find(sensorId);
    indPtr->result = (it == instance->sensorStateMap.end()) ? LE_BAD_PARAMETER : LE_OK;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorConfigHandler: sensorId=%d not found in sensorStateMap", sensorId);

    taf_IvssSensor_SensorState_t& state = it->second;
    indPtr->result = (state.sensorRef == NULL) ? LE_FAULT : LE_OK;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorConfigHandler: sensorRef is NULL for sensorId=%d", sensorId);

    // Validate samplingRate: must exactly match a value in the ODR list (error < 1e-6)
    double odrList[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE] = {};
    size_t odrCount = TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE;
    bool validRate = false;
    if (taf_imuSensor_GetSupportedSamplingRate(state.sensorRef, odrList, &odrCount) == LE_OK)
    {
        for (size_t i = 0; i < odrCount; i++)
        {
            if (fabs(odrList[i] - (double)samplingRate) < 1e-6)
            {
                validRate = true;
                break;
            }
        }
    }
    indPtr->result = validRate ? LE_OK : LE_BAD_PARAMETER;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorConfigHandler: samplingRate=%.2f not in ODR list for sensorId=%d",
        samplingRate, sensorId);

    // Validate batchCount: must be in [min, max] range and a multiple of 10
    uint32_t maxBatch = 0, minBatch = 0;
    bool validBatch = false;
    if (taf_imuSensor_GetSupportedBatchCount(state.sensorRef, &maxBatch, &minBatch) == LE_OK)
    {
        validBatch = ((uint32_t)batchCount >= minBatch &&
                      (uint32_t)batchCount <= maxBatch &&
                      (batchCount % 10) == 0);
    }
    indPtr->result = validBatch ? LE_OK : LE_BAD_PARAMETER;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorConfigHandler: batchCount=%d invalid for sensorId=%d "
        "(must be in [min,max] and multiple of 10)", batchCount, sensorId);

    // Store configuration
    state.samplingRate = samplingRate;
    state.batchCount = (uint32_t)batchCount;
    state.isConfigured = true;

    // Send ConfigUpdate event (same params as SensorConfigReq entry log)
    LE_INFO("SensorConfigUpdate: sensorId=%d samplingRate=%.2f batchCount=%d",
        sensorId, samplingRate, batchCount);
    instance->fireSensorConfigUpdateEvent(sensorId, samplingRate, (int32_t)batchCount);

    // If sensor is enabled, Deactivate then Activate to apply new configuration
    if (state.isEnabled)
    {
        le_result_t res = taf_imuSensor_Deactivate(state.sensorRef);
        if (res != LE_OK)
        {
            LE_WARN("SensorConfigHandler: Deactivate failed for sensorId=%d - %s",
                sensorId, LE_RESULT_TXT(res));
        }
        res = taf_imuSensor_Activate(state.sensorRef,
            (double)samplingRate, (uint32_t)batchCount);
        if (res != LE_OK)
        {
            LE_ERROR("SensorConfigHandler: Activate failed for sensorId=%d - %s",
                sensorId, LE_RESULT_TXT(res));
        }
    }

    indPtr->result = LE_OK;
    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure sensor (CommonAPI method implementation)
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::SensorConfigReq(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    int32_t _sensorId, float _samplingRate, int32_t _batchCount,
    SensorConfigReqReply_t _reply)
{
    LE_INFO("tafIvssSensorSvc SensorConfig: sensorId=%d, samplingRate=%.2f, batchCount=%d",
        _sensorId, _samplingRate, _batchCount);

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)le_mem_ForceAlloc(EventPool);
    *indPtr = taf_IvssSensor_Ind_t{};
    indPtr->semRef = le_sem_Create("Ivss SensorConfigSem", 0);
    indPtr->sensorConfig.sensorId = _sensorId;
    indPtr->sensorConfig.samplingRate = _samplingRate;
    indPtr->sensorConfig.batchCount = _batchCount;

    le_event_ReportWithRefCounting(SensorConfigReqEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    SensorSvcTypes::SensorReturnT retCode = ResultLeToSensorReturn(indPtr->result);
    LE_INFO("tafIvssSensorSvc SensorConfig reply: sensorId=%d le_result=%d SensorReturnT=%d",
        _sensorId, static_cast<int>(indPtr->result), static_cast<int>(retCode));
    _reply(retCode);

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * SensorControl Legato event handler function.
 * Enables or disables a sensor.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::SensorControlReqHandler(void* reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)reportPtr;
    int32_t sensorId = indPtr->sensorControl.sensorId;
    bool enable = (indPtr->sensorControl.sensorState ==
        static_cast<int32_t>(SensorSvcTypes::SensorStateT::SENSOR_STATE_T_ENABLE));

    auto instance = tafIvssSensorSvc::GetInstance();

    // ---- Heading sensor control ----
    if (sensorId == IVSS_SENSOR_HEADING_ID)
    {
        if (enable)
        {
            instance->headingState.enableRefCount++;
            if (!instance->headingState.isEnabled)
            {
                // First enable: start GNSS session
                // Step 1: Set acquisition rate to 100ms (10Hz) before Start
                le_result_t res = taf_locGnss_SetAcquisitionRate(100);
                if (res != LE_OK)
                {
                    LE_WARN("SensorControlHandler: SetAcquisitionRate(100ms) failed - %s "
                        "(may already be started, continuing)", LE_RESULT_TXT(res));
                    // Continue on failure (may already be ACTIVE), do not abort
                }
                // Step 2: Start GNSS position session
                res = taf_locGnss_Start();
                if (res != LE_OK && res != LE_DUPLICATE)
                {
                    instance->headingState.enableRefCount--;
                    indPtr->result = LE_FAULT;
                    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                        "SensorControlHandler: taf_locGnss_Start failed - %s",
                        LE_RESULT_TXT(res));
                }

                instance->headingState.isEnabled = true;
                LE_INFO("SensorControlHandler: Heading sensor enabled (10Hz, acqRate=100ms)");
            }
            else
            {
                LE_INFO("SensorControlHandler: Heading sensor already enabled, refCount=%d",
                    instance->headingState.enableRefCount);
            }
        }
        else
        {
            if (instance->headingState.enableRefCount > 0)
            {
                instance->headingState.enableRefCount--;
            }
            // Only truly stop when reference count reaches zero
            if (instance->headingState.enableRefCount <= 0 && instance->headingState.isEnabled)
            {
                le_result_t res = taf_locGnss_Stop();
                if (res != LE_OK)
                {
                    LE_WARN("SensorControlHandler: taf_locGnss_Stop failed - %s",
                        LE_RESULT_TXT(res));
                }
                instance->headingState.isEnabled = false;
                LE_INFO("SensorControlHandler: Heading sensor disabled");
            }
            else if (instance->headingState.enableRefCount > 0)
            {
                LE_INFO("SensorControlHandler: Heading sensor refCount=%d, not stopping yet",
                    instance->headingState.enableRefCount);
            }
        }
        indPtr->result = LE_OK;
        le_sem_Post(indPtr->semRef);
        return;
    }

    // ---- IMU sensor control ----
    auto it = instance->sensorStateMap.find(sensorId);
    indPtr->result = (it == instance->sensorStateMap.end()) ? LE_BAD_PARAMETER : LE_OK;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorControlHandler: sensorId=%d not found", sensorId);

    taf_IvssSensor_SensorState_t& state = it->second;
    indPtr->result = (state.sensorRef == NULL) ? LE_FAULT : LE_OK;
    TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
        "SensorControlHandler: sensorRef is NULL for sensorId=%d", sensorId);

    if (enable)
    {
        state.enableRefCount++;
        if (!state.isEnabled)
        {
            // SensorConfigReq must be called first to configure sampling rate and batch count
            if (!state.isConfigured)
            {
                state.enableRefCount--;
                indPtr->result = LE_BAD_PARAMETER;
                TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                    "SensorControlHandler: sensorId=%d not configured, "
                    "call SensorConfigReq first", sensorId);
            }

            le_result_t res = taf_imuSensor_Activate(state.sensorRef,
                state.samplingRate, state.batchCount);
            if (res != LE_OK)
            {
                state.enableRefCount--;
                indPtr->result = res;
                TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
                    "SensorControlHandler: taf_imuSensor_Activate failed for sensorId=%d - %s",
                    sensorId, LE_RESULT_TXT(res));
            }

            state.isEnabled = true;
            LE_INFO("SensorControlHandler: IMU sensor %d enabled (%.2fHz, batch=%u)",
                sensorId, state.samplingRate, state.batchCount);
        }
        else
        {
            LE_INFO("SensorControlHandler: IMU sensor %d already enabled, refCount=%d",
                sensorId, state.enableRefCount);
        }
    }
    else
    {
        if (state.enableRefCount > 0)
        {
            state.enableRefCount--;
        }
        // Only truly stop when reference count reaches zero
        if (state.enableRefCount <= 0 && state.isEnabled)
        {
            le_result_t res = taf_imuSensor_Deactivate(state.sensorRef);
            if (res != LE_OK)
            {
                LE_WARN("SensorControlHandler: Deactivate failed for sensorId=%d - %s",
                    sensorId, LE_RESULT_TXT(res));
            }
            state.isEnabled = false;
            LE_INFO("SensorControlHandler: IMU sensor %d disabled", sensorId);
        }
        else if (state.enableRefCount > 0)
        {
            LE_INFO("SensorControlHandler: IMU sensor %d refCount=%d, not stopping yet",
                sensorId, state.enableRefCount);
        }
    }

    indPtr->result = LE_OK;
    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable sensor (CommonAPI method implementation)
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::SensorControlReq(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    int32_t _sensorId, SensorSvcTypes::SensorStateT _sensorState,
    SensorControlReqReply_t _reply)
{
    LE_INFO("tafIvssSensorSvc SensorControl: sensorId=%d, state=%d",
        _sensorId, static_cast<int>(_sensorState));

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)le_mem_ForceAlloc(EventPool);
    *indPtr = taf_IvssSensor_Ind_t{};
    indPtr->semRef = le_sem_Create("Ivss SensorControlSem", 0);
    indPtr->sensorControl.sensorId = _sensorId;
    // Convert SensorStateT to int32_t (avoids non-trivial type issues in union)
    indPtr->sensorControl.sensorState = static_cast<int32_t>(_sensorState);

    le_event_ReportWithRefCounting(SensorControlReqEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    SensorSvcTypes::SensorReturnT retCode = ResultLeToSensorReturn(indPtr->result);
    LE_INFO("tafIvssSensorSvc SensorControl reply: sensorId=%d le_result=%d SensorReturnT=%d",
        _sensorId, static_cast<int>(indPtr->result), static_cast<int>(retCode));
    _reply(retCode);

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
}

// ============================================================================
// taf_imuSensor async callback implementations
// ============================================================================

//--------------------------------------------------------------------------------------------------
/**
 * IMU sensor data callback.
 * Converts taf_imuSensor data to SensorImuEventT and triggers SensorImuDataRead broadcast.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::OnSensorDataHandler(
    taf_imuSensor_SampleRef_t sampleRef,
    const taf_imuSensor_DataValue_t* rawData, size_t rawDataCount,
    const taf_imuSensor_DataValue_t* biasData, size_t biasDataCount,
    void* contextPtr)
{
    if (rawData == NULL || rawDataCount == 0)
    {
        LE_DEBUG("OnSensorDataHandler: rawData is NULL or empty");
        if (sampleRef != NULL)
        {
            le_result_t delRes = taf_imuSensor_DeleteData(sampleRef);
            if (delRes != LE_OK)
            {
                LE_DEBUG("OnSensorDataHandler: DeleteData failed - %s", LE_RESULT_TXT(delRes));
            }
        }
        return;
    }

    // Get sensorId from contextPtr
    int32_t sensorId = (contextPtr != NULL) ? *(int32_t*)contextPtr : 0;

    auto instance = tafIvssSensorSvc::GetInstance();

    // Look up sensor type
    SensorSvcTypes::SensorTypeT sensorType = SensorSvcTypes::SensorTypeT::SENSOR_TYPE_T_UNKNOWN;
    auto it = instance->sensorStateMap.find(sensorId);
    if (it != instance->sensorStateMap.end() && it->second.sensorRef != NULL)
    {
        taf_imuSensor_SensorType_t tafType;
        if (taf_imuSensor_GetType(it->second.sensorRef, &tafType) == LE_OK)
        {
            sensorType = SensorTypeToIvss(tafType);
        }
    }

    // Calculate GPTP-boot offset (read clock once per batch, apply per event)
    int64_t gptpBootOffset = GetGptpBootOffset(instance->gptpTimeRef);

    // Build SensorImuEventT array
    std::vector<SensorSvcTypes::SensorImuEventT> events;
    size_t count = (rawDataCount < (size_t)IVSS_SENSOR_MAX_BATCH_COUNT) ?
        rawDataCount : (size_t)IVSS_SENSOR_MAX_BATCH_COUNT;

    for (size_t i = 0; i < count; i++)
    {
        SensorSvcTypes::SensorImuEventT event = {};
        event.setSensorId(sensorId);
        event.setType(static_cast<int32_t>(sensorType));
        event.setTimestamp(rawData[i].timestamp);

        // Calculate GPTP timestamp per event; clamp to 0 if offset makes it negative
        int64_t gptpTs = (int64_t)rawData[i].timestamp + gptpBootOffset;
        event.setGptpTimestamp((gptpTs > 0) ? (uint64_t)gptpTs : 0ULL);

        // Uncalibrated (raw) data and bias data
        SensorSvcTypes::SensorUncalibratedEventT uncalib = {};
        uncalib.setXUncalib((float)rawData[i].x);
        uncalib.setYUncalib((float)rawData[i].y);
        uncalib.setZUncalib((float)rawData[i].z);

        if (biasData != NULL && i < biasDataCount)
        {
            uncalib.setXBias((float)biasData[i].x);
            uncalib.setYBias((float)biasData[i].y);
            uncalib.setZBias((float)biasData[i].z);
        }
        event.setData(uncalib);
        events.push_back(event);
    }

    LE_DEBUG("SensorImuDataRead: sensorId=%d count=%zu", sensorId, events.size());
    // Trigger SensorImuDataRead broadcast
    instance->fireSensorImuDataReadEvent(events, (int32_t)events.size());

    // Release sample reference
    if (sampleRef != NULL)
    {
        le_result_t delRes = taf_imuSensor_DeleteData(sampleRef);
        if (delRes != LE_OK)
        {
            LE_DEBUG("OnSensorDataHandler: DeleteData failed - %s", LE_RESULT_TXT(delRes));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Sensor capability state change callback.
 * Aggregates IMU sensor availability and triggers SensorCapabilities broadcast.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::OnCapabilityUpdateHandler(
    taf_imuSensor_SensorRef_t sensorRef,
    bool isAvailable, bool isEnabled, uint32_t capabilityMask,
    void* contextPtr)
{
    LE_INFO("OnCapabilityUpdateHandler: isAvailable=%d, isEnabled=%d, mask=0x%08X",
        isAvailable, isEnabled, capabilityMask);

    auto instance = tafIvssSensorSvc::GetInstance();

    // Aggregate availability: service is ready if any sensor is available
    bool anyAvailable = isAvailable;
    for (auto& pair : instance->sensorStateMap)
    {
        if (pair.second.sensorRef != NULL && pair.second.isEnabled)
        {
            anyAvailable = true;
            break;
        }
    }

    SensorSvcTypes::SensorServiceStateMaskT mask =
        anyAvailable ?
        SensorSvcTypes::SensorServiceStateMaskT::SENSOR_SERVICE_STATE_MASK_T_READY :
        SensorSvcTypes::SensorServiceStateMaskT::SENSOR_SERVICE_STATE_MASK_T_UNKNOWN;

    LE_INFO("SensorCapabilities: mask=%d (capability update)", static_cast<int>(mask));
    instance->fireSensorCapabilitiesEvent(mask);
}

// ============================================================================
// taf_locGnss async callback implementation (heading sensor)
// ============================================================================

//--------------------------------------------------------------------------------------------------
/**
 * GNSS position callback (heading sensor data source).
 * Converts body frame yaw (radians) to heading (degrees) and triggers
 * SensorHeadingDataRead broadcast.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::OnPositionHandler(
    taf_locGnss_SampleRef_t positionSampleRef,
    void* contextPtr)
{
    if (positionSampleRef == NULL)
    {
        LE_DEBUG("OnPositionHandler: positionSampleRef is NULL");
        return;
    }

    // Get body frame data (DR engine yaw angle)
    // No bodyFrameDataMask filtering; yaw=0.0 when invalid, client decides validity
    taf_locGnss_KinematicsData_t bodyData = {};
    le_result_t res = taf_locGnss_GetBodyFrameData(positionSampleRef, &bodyData);
    if (res != LE_OK)
    {
        LE_DEBUG("OnPositionHandler: GetBodyFrameData failed - %s", LE_RESULT_TXT(res));
        taf_locGnss_ReleaseSampleRef(positionSampleRef);
        return;
    }

    // Get elapsed real time timestamp
    uint64_t realTime = 0, realTimeUnc = 0;
    res = taf_locGnss_GetRealTimeInformation(positionSampleRef, &realTime, &realTimeUnc);
    if (res != LE_OK)
    {
        LE_WARN("OnPositionHandler: GetRealTimeInformation failed - %s", LE_RESULT_TXT(res));
        realTime = 0;
    }

    // Get GPTP timestamp (direct from taf_locGnss, accurate value)
    uint64_t gPtpTime = 0, gPtpTimeUnc = 0;
    res = taf_locGnss_GetGptpTime(positionSampleRef, &gPtpTime, &gPtpTimeUnc);
    if (res != LE_OK)
    {
        LE_DEBUG("OnPositionHandler: GetGptpTime failed - %s", LE_RESULT_TXT(res));
        gPtpTime = 0;
    }

    // Convert radians to degrees
    float headingDeg  = (float)(bodyData.yaw    * (180.0 / M_PI));
    float accuracyDeg = (float)(bodyData.yawUnc * (180.0 / M_PI));

    // Build SensorHeadEventT
    SensorSvcTypes::SensorHeadingEventT headingData = {};
    headingData.setHeading(headingDeg);
    headingData.setAccuracy(accuracyDeg);

    SensorSvcTypes::SensorHeadEventT event = {};
    event.setSensorId(IVSS_SENSOR_HEADING_ID);
    event.setType(IVSS_SENSOR_HEADING_TYPE);
    event.setTimestamp(realTime);
    event.setGptpTimestamp(gPtpTime);
    event.setData(headingData);

    std::vector<SensorSvcTypes::SensorHeadEventT> events = {event};

    LE_DEBUG("SensorHeadingDataRead: heading=%.2f accuracy=%.2f ts=%llu gptp=%llu",
        headingDeg, accuracyDeg,
        (unsigned long long)realTime, (unsigned long long)gPtpTime);
    // Trigger SensorHeadingDataRead broadcast
    auto instance = tafIvssSensorSvc::GetInstance();
    instance->fireSensorHeadingDataReadEvent(events, 1);

    // Release position sample reference
    taf_locGnss_ReleaseSampleRef(positionSampleRef);
}

// ============================================================================
// Initialization
// ============================================================================

//--------------------------------------------------------------------------------------------------
/**
 * Service initialization.
 * Initializes memory pool, events, event handlers, and builds sensor state map.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::Init(void)
{
    // Initialize memory pool
    EventPool = le_mem_CreatePool("Ivss Sensor EventPool", sizeof(taf_IvssSensor_Ind_t));

    // Initialize event IDs
    GetSensorListReqEvent = le_event_CreateIdWithRefCounting("GetSensorListReqEvent");
    SensorConfigReqEvent  = le_event_CreateIdWithRefCounting("SensorConfigReqEvent");
    SensorControlReqEvent = le_event_CreateIdWithRefCounting("SensorControlReqEvent");
    DeRegisterReqEvent    = le_event_CreateIdWithRefCounting("DeRegisterReqEvent");

    // Register event handler functions
    GetSensorListReqEventHandlerRef = le_event_AddHandler("GetSensorListReqEvent Handler",
        GetSensorListReqEvent, tafIvssSensorSvc::GetSensorListReqHandler);
    SensorConfigReqEventHandlerRef = le_event_AddHandler("SensorConfigReqEvent Handler",
        SensorConfigReqEvent, tafIvssSensorSvc::SensorConfigReqHandler);
    SensorControlReqEventHandlerRef = le_event_AddHandler("SensorControlReqEvent Handler",
        SensorControlReqEvent, tafIvssSensorSvc::SensorControlReqHandler);
    DeRegisterReqEventHandlerRef = le_event_AddHandler("DeRegisterReqEvent Handler",
        DeRegisterReqEvent, tafIvssSensorSvc::DeRegisterReqHandler);

    // Initialize GPTP hardware clock reference (/dev/ptp0)
    gptpTimeRef = taf_gptpTime_CreateRef("/dev/ptp0");
    if (gptpTimeRef == NULL)
    {
        // /dev/ptp0 not accessible; all IMU GPTP timestamps will be 0
        LE_ERROR("tafIvssSensorSvc Init: Failed to create GPTP time ref for /dev/ptp0, "
            "IMU GPTP timestamps will be 0");
    }
    else
    {
        LE_INFO("tafIvssSensorSvc Init: GPTP time ref created for /dev/ptp0");
    }

    // Initialize heading sensor state
    memset(&headingState, 0, sizeof(headingState));
    sensorListRef = NULL;

    // Register heading sensor position callback once; Start/Stop controls data flow
    headingState.positionHandlerRef = taf_locGnss_AddPositionHandler(
        tafIvssSensorSvc::OnPositionHandler, NULL);
    if (headingState.positionHandlerRef == NULL)
    {
        // positionHandlerRef registration failed; heading sensor will not work
        LE_ERROR("tafIvssSensorSvc Init: Failed to add position handler for heading sensor");
    }
    else
    {
        LE_INFO("tafIvssSensorSvc Init: Position handler registered for heading sensor");
    }

    // Build IMU sensor state map (keep sensorListRef to maintain sensorRef validity)
    sensorListRef = taf_imuSensor_GetSensorList();
    if (sensorListRef != NULL)
    {
        taf_imuSensor_SensorRef_t sensorRef = taf_imuSensor_GetFirstSensor(sensorListRef);
        while (sensorRef != NULL)
        {
            uint32_t sensorId = 0;
            if (taf_imuSensor_GetId(sensorRef, &sensorId) == LE_OK)
            {
                taf_IvssSensor_SensorState_t state = {};
                state.sensorRef = sensorRef;
                state.isConfigured = false;
                state.isEnabled = false;
                state.enableRefCount = 0;
                state.dataHandlerRef = NULL;
                state.capabilityHandlerRef = NULL;

                // Insert into map first, then get key address (key lifetime tied to map)
                sensorStateMap[(int32_t)sensorId] = state;
                int32_t* ctxSensorId = const_cast<int32_t*>(
                    &sensorStateMap.find((int32_t)sensorId)->first);

                // dataHandlerRef registered once in Init()
                sensorStateMap[(int32_t)sensorId].dataHandlerRef =
                    taf_imuSensor_AddDataHandler(sensorRef,
                        tafIvssSensorSvc::OnSensorDataHandler, (void*)ctxSensorId);

                sensorStateMap[(int32_t)sensorId].capabilityHandlerRef =
                    taf_imuSensor_AddCapabilityUpdateHandler(sensorRef,
                        tafIvssSensorSvc::OnCapabilityUpdateHandler, (void*)ctxSensorId);

                LE_INFO("tafIvssSensorSvc: Registered sensor sensorId=%u", sensorId);
            }
            sensorRef = taf_imuSensor_GetNextSensor(sensorListRef);
        }
    }
    else
    {
        // No IMU sensors found; accelerometer/gyroscope functionality unavailable
        LE_ERROR("tafIvssSensorSvc Init: No IMU sensors found");
    }

    LE_INFO("tafIvssSensorSvc Service initialized, %zu IMU sensors registered",
        sensorStateMap.size());
}
