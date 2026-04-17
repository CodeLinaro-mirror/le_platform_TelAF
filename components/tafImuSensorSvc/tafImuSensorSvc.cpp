/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <telux/sensor/SensorManager.hpp>
#include "tafImuSensor.hpp"
#include "tafSvcIF.hpp"

using namespace tafsvc;

/**
* The initialization of TelAF sensor component.
*/

COMPONENT_INIT
{
    LE_INFO("tafSensor Service Init...\n");
    auto &sensorMngr = taf_Sensor::GetInstance();
    sensorMngr.Init();
    LE_INFO("tafSensor Service Ready...\n");
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetSensorList

 DESCRIPTION    Get list of all available sensors.

 DEPENDENCIES   Initialization of sensor service

 RETURN VALUE   taf_imuSensor_SensorListRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetSensorList(taf_imuSensor_ServerCmdRef_t cmdRef)
{
    auto& m = taf_Sensor::GetInstance();
    le_msg_SessionRef_t sref = taf_imuSensor_GetClientSessionRef();
    taf_imuSensor_SensorListRef_t listRef = m.GetAvailableSensors(sref);
    taf_imuSensor_GetSensorListRespond(cmdRef, listRef);
}


/*==================================================================================================

 FUNCTION       taf_imuSensor_DeleteSensorList

 DESCRIPTION    Delete a reference of sensor list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   le_result_t
                LE_NOT_FOUND: Fail
                LE_OK: Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_DeleteSensorList(taf_imuSensor_ServerCmdRef_t cmdRef,
                                   taf_imuSensor_SensorListRef_t sensorListRef)
{
    auto& m = taf_Sensor::GetInstance();
    le_result_t rc = m.DeleteSensorList(sensorListRef);
    taf_imuSensor_DeleteSensorListRespond(cmdRef, rc);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetFirstSensor

 DESCRIPTION    Get first sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_imuSensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetFirstSensor(taf_imuSensor_ServerCmdRef_t cmdRef,
                                 taf_imuSensor_SensorListRef_t sensorListRef)
{
    auto& m = taf_Sensor::GetInstance();
    taf_imuSensor_SensorRef_t s = m.GetFirstSensor(sensorListRef);
    taf_imuSensor_GetFirstSensorRespond(cmdRef, s);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetNextSensor

 DESCRIPTION    Get next sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_imuSensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetNextSensor(taf_imuSensor_ServerCmdRef_t cmdRef,
                                taf_imuSensor_SensorListRef_t sensorListRef)
{
    auto& m = taf_Sensor::GetInstance();
    taf_imuSensor_SensorRef_t s = m.GetNextSensor(sensorListRef);
    taf_imuSensor_GetNextSensorRespond(cmdRef, s);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetId

 DESCRIPTION    Gets the id of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetId(taf_imuSensor_ServerCmdRef_t cmdRef,
                        taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    uint32_t id = 0;
    le_result_t rc = m.GetSensorId(sensorRef, &id);
    taf_imuSensor_GetIdRespond(cmdRef, rc, id);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetName

 DESCRIPTION    Gets the name of the sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetName(taf_imuSensor_ServerCmdRef_t cmdRef,
                          taf_imuSensor_SensorRef_t sensorRef,
                          size_t sensorNameSize)
{
    auto& m = taf_Sensor::GetInstance();
    char nameBuf[TAF_IMUSENSOR_NAME_MAX_SIZE] = {0};
    size_t cap = (sensorNameSize > sizeof(nameBuf)) ? sizeof(nameBuf) : sensorNameSize;
    le_result_t rc = m.GetSensorName(sensorRef, nameBuf, cap);
    taf_imuSensor_GetNameRespond(cmdRef, rc, nameBuf);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetVendorName

 DESCRIPTION    Gets the vendor name of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetVendorName(taf_imuSensor_ServerCmdRef_t cmdRef,
                                taf_imuSensor_SensorRef_t sensorRef,
                                size_t vendorNameSize)
{
    auto& m = taf_Sensor::GetInstance();
    char vendorBuf[TAF_IMUSENSOR_NAME_MAX_SIZE] = {0};
    size_t cap = (vendorNameSize > sizeof(vendorBuf)) ? sizeof(vendorBuf) : vendorNameSize;
    le_result_t rc = m.GetSensorVendorName(sensorRef, vendorBuf, cap);
    taf_imuSensor_GetVendorNameRespond(cmdRef, rc, vendorBuf);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetType

 DESCRIPTION    Gets the type of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetType(taf_imuSensor_ServerCmdRef_t cmdRef,
                          taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    taf_imuSensor_SensorType_t t = TAF_IMUSENSOR_INVALID;
    le_result_t rc = m.GetSensorType(sensorRef, &t);
    taf_imuSensor_GetTypeRespond(cmdRef, rc, t);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetVersion

 DESCRIPTION    Gets the version of sensor.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetVersion(taf_imuSensor_ServerCmdRef_t cmdRef,
                             taf_imuSensor_SensorRef_t sensorRef,
                             size_t versionSize)
{
    auto& m = taf_Sensor::GetInstance();
    char verBuf[SENSOR_VERSION_SIZE] = {0};
    size_t cap = (versionSize > sizeof(verBuf)) ? sizeof(verBuf) : versionSize;
    le_result_t rc = m.GetSensorVersion(sensorRef, verBuf, cap);
    taf_imuSensor_GetVersionRespond(cmdRef, rc, verBuf);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetSupportedSamplingRate

 DESCRIPTION    Gets info of sensor like supported sampling rates.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetSupportedSamplingRate(taf_imuSensor_ServerCmdRef_t cmdRef,
                                           taf_imuSensor_SensorRef_t sensorRef,
                                           size_t samplingRatesListCapacity)
{
    auto& m = taf_Sensor::GetInstance();
    double rates[TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE] = {0};
    size_t cap = (samplingRatesListCapacity > (size_t)TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE)
                 ? (size_t)TAF_IMUSENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE
                 : samplingRatesListCapacity;
    le_result_t rc = m.GetSensorSamplingRateInfo(sensorRef, rates, &cap);
    taf_imuSensor_GetSupportedSamplingRateRespond(cmdRef, rc, rates, (uint32_t)cap);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetSupportedBatchCount

 DESCRIPTION    Gets info of sensor like min and max batch count supported.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetSupportedBatchCount(taf_imuSensor_ServerCmdRef_t cmdRef,
                                         taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    uint32_t maxC = 0, minC = 0;
    le_result_t rc = m.GetSensorBatchingInfo(sensorRef, &maxC, &minC);
    taf_imuSensor_GetSupportedBatchCountRespond(cmdRef, rc, maxC, minC);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetSensorRangeInfo

 DESCRIPTION    Gets Info of sensor like range.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetRange(taf_imuSensor_ServerCmdRef_t cmdRef,
                           taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    double r = 0;
    le_result_t rc = m.GetSensorRangeInfo(sensorRef, &r);
    taf_imuSensor_GetRangeRespond(cmdRef, rc, r);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetSensorResolution

 DESCRIPTION    Gets info of sensor resolution.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetResolution(taf_imuSensor_ServerCmdRef_t cmdRef,
                                taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    double res = 0;
    le_result_t rc = m.GetSensorResolution(sensorRef, &res);
    taf_imuSensor_GetResolutionRespond(cmdRef, rc, res);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_SetRefCoordinateByEulerAngle

 DESCRIPTION    Sets the euler angle to get rotated sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_SetRefCoordinateByEulerAngle(taf_imuSensor_ServerCmdRef_t cmdRef,
                                               taf_imuSensor_SensorRef_t sensorRef,
                                               double pitch, double roll, double yaw)
{
    LE_UNUSED(sensorRef);
    auto& m = taf_Sensor::GetInstance();
    m.SetEulerAngle(cmdRef, pitch, roll, yaw);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_Activate

 DESCRIPTION    Activate the sensor to get sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_UNSUPPORTED: if Sampling rate or batch count not supported.
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_Activate(taf_imuSensor_ServerCmdRef_t cmdRef,
                           taf_imuSensor_SensorRef_t sensorRef,
                           double samplingRate,
                           uint32_t batchCount)
{
    auto& m = taf_Sensor::GetInstance();
    m.Activate(cmdRef, sensorRef, samplingRate, batchCount, taf_imuSensor_GetClientSessionRef());
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_Deactivate

 DESCRIPTION    Deactivate the sensor to stop getting sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_Deactivate(taf_imuSensor_ServerCmdRef_t cmdRef,
                             taf_imuSensor_SensorRef_t sensorRef)
{
    auto& m = taf_Sensor::GetInstance();
    m.Deactivate(cmdRef, sensorRef, taf_imuSensor_GetClientSessionRef());
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_AddDataHandler

 DESCRIPTION    Sends sensor data event notification.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Data event handler function.

 RETURN VALUE   taf_imuSensor_DataHandlerRef_t if registered successfully else NULL.

 SIDE EFFECTS

==================================================================================================*/

taf_imuSensor_DataHandlerRef_t taf_imuSensor_AddDataHandler(taf_imuSensor_SensorRef_t sensorRef,
                                                           taf_imuSensor_DataHandlerFunc_t handlerPtr,
                                                           void* contextPtr)
{
    auto& m = taf_Sensor::GetInstance();
    return m.AddDataHandler(sensorRef, handlerPtr, contextPtr);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_RemoveDataHandler

 DESCRIPTION    Removes sensor OnEvent handler.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Data event handler reference.

 RETURN VALUE

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_RemoveDataHandler(taf_imuSensor_DataHandlerRef_t handlerRef)
{
    auto& m = taf_Sensor::GetInstance();
    m.RemoveDataHandler(handlerRef);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_GetRotatedData

 DESCRIPTION    Gets the raw data and bias for sensor sample list.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SampleRef_t sampleRef: reference to SampleRef.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_GetRotatedData(taf_imuSensor_ServerCmdRef_t cmdRef,
                                 taf_imuSensor_SampleRef_t sampleRef,
                                 size_t rawDataCapacity,
                                 size_t biasDataCapacity)
{
    auto& m = taf_Sensor::GetInstance();

    const size_t maxCap = (size_t)TAF_SENSOR_MAX_EVENTS_SIZE;
    size_t rawCap  = (rawDataCapacity  > maxCap) ? maxCap : rawDataCapacity;
    size_t biasCap = (biasDataCapacity > maxCap) ? maxCap : biasDataCapacity;

    if ((rawCap == 0) || (biasCap == 0))
    {
        taf_imuSensor_GetRotatedDataRespond(cmdRef, LE_BAD_PARAMETER, NULL, 0, NULL, 0);
        return;
    }

    taf_imuSensor_DataValue_t rawBuf[TAF_SENSOR_MAX_EVENTS_SIZE];
    taf_imuSensor_DataValue_t biasBuf[TAF_SENSOR_MAX_EVENTS_SIZE];

    le_result_t rc = m.GetData(sampleRef, rawBuf, &rawCap, biasBuf, &biasCap);
    taf_imuSensor_GetRotatedDataRespond(cmdRef, rc, rawBuf, rawCap, biasBuf, biasCap);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_DeleteData

 DESCRIPTION    Delete the list of sensor samples.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SampleListRef_t sampleList: reference to SampleList.

 RETURN VALUE   le_result_t
                LE_BAD_PARAMETER: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_DeleteData(taf_imuSensor_ServerCmdRef_t cmdRef,
                             taf_imuSensor_SampleRef_t sampleRef)
{
    auto& m = taf_Sensor::GetInstance();
    le_result_t rc = m.DeleteData(sampleRef);
    taf_imuSensor_DeleteDataRespond(cmdRef, rc);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_SelfTest

 DESCRIPTION    Initiate self test for sensor.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

==================================================================================================*/

void taf_imuSensor_SelfTest(taf_imuSensor_ServerCmdRef_t cmdRef,
                           taf_imuSensor_SensorRef_t sensorRef,
                           taf_imuSensor_SelfTestMode_t mode)
{
    auto& m = taf_Sensor::GetInstance();
    m.SelfTest(cmdRef, sensorRef, mode, taf_imuSensor_GetClientSessionRef());
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_AddSelfTestFailedHandler

 DESCRIPTION    Sends sensor self test failed notification.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Self test failed handler function.

 RETURN VALUE   taf_imuSensor_SelfTestFailedHandlerRef_t if registered successfully else NULL.

 SIDE EFFECTS

==================================================================================================*/
taf_imuSensor_SelfTestFailedHandlerRef_t taf_imuSensor_AddSelfTestFailedHandler(
    taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_SelfTestFailedHandlerFunc_t handlerPtr,
    void* contextPtr)
{
    auto& m = taf_Sensor::GetInstance();
    return m.AddSelfTestFailedHandler(sensorRef, handlerPtr, contextPtr);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_RemoveSelfTestFailedHandler

 DESCRIPTION    Removes selfTest Handler.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Self test handler reference.

 RETURN VALUE

 SIDE EFFECTS

==================================================================================================*/
void taf_imuSensor_RemoveSelfTestFailedHandler(taf_imuSensor_SelfTestFailedHandlerRef_t handlerRef)
{
    auto& m = taf_Sensor::GetInstance();
    m.RemoveSelfTestFailedHandler(handlerRef);
}

/*==================================================================================================

 FUNCTION       taf_imuSensor_ReleaseSelfTestRef

 DESCRIPTION    Release Self test event reference.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Self test event reference.

 RETURN VALUE

 SIDE EFFECTS

==================================================================================================*/
void taf_imuSensor_ReleaseSelfTestRef(taf_imuSensor_ServerCmdRef_t cmdRef,
                                     taf_imuSensor_SelfTestEventRef_t eventRef)
{
    LE_UNUSED(eventRef);
    taf_imuSensor_ReleaseSelfTestRefRespond(cmdRef, LE_OK);
}
