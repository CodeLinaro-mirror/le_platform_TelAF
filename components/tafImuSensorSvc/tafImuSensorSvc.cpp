/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <telux/sensor/SensorManager.hpp>
#include "tafImuSensor.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tafsvc;

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

/*======================================================================

 FUNCTION       taf_imuSensor_GetSensorList

 DESCRIPTION    Get list of all available sensors.

 DEPENDENCIES   Initialization of sensor service

 RETURN VALUE   taf_imuSensor_SensorListRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_imuSensor_SensorListRef_t taf_imuSensor_GetSensorList
(
    void
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetAvailableSensors();
}

/*======================================================================

 FUNCTION       taf_imuSensor_DeleteSensorList

 DESCRIPTION    Delete a reference of sensor list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   le_result_t
                LE_NOT_FOUND: Fail
                LE_OK: Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_DeleteSensorList
(
    taf_imuSensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.DeleteSensorList(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetFirstSensor

 DESCRIPTION    Get first sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_imuSensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_imuSensor_SensorRef_t taf_imuSensor_GetFirstSensor
(
    taf_imuSensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetFirstSensor(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetNextSensor

 DESCRIPTION    Get next sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_imuSensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_imuSensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_imuSensor_SensorRef_t taf_imuSensor_GetNextSensor
(
    taf_imuSensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetNextSensor(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetId

 DESCRIPTION    Gets the id of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetId
(
    taf_imuSensor_SensorRef_t sensorRef,
    uint32_t* sensorIdPtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorId(sensorRef,sensorIdPtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetName

 DESCRIPTION    Gets the name of the sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetName
(
    taf_imuSensor_SensorRef_t sensorRef,
    char* sensorName,
    size_t sensorNameSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorName(sensorRef,sensorName,sensorNameSize);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetVendorName

 DESCRIPTION    Gets the vendor name of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetVendorName
(
    taf_imuSensor_SensorRef_t sensorRef,
    char* sensorVendorName,
    size_t sensorVendorNameSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorVendorName(sensorRef,sensorVendorName,sensorVendorNameSize);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetType

 DESCRIPTION    Gets the type of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetType
(
    taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_SensorType_t* sensorTypePtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorType(sensorRef,sensorTypePtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetVersion

 DESCRIPTION    Gets the version of sensor.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetVersion
(
    taf_imuSensor_SensorRef_t sensorRef,
    char* version,
    size_t versionSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorVersion(sensorRef,version,versionSize);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetSupportedSamplingRate

 DESCRIPTION    Gets info of sensor like supported sampling rates.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetSupportedSamplingRate
(
    taf_imuSensor_SensorRef_t sensorRef,
    double* samplingRatesListPtr,
    size_t* samplingRatesListSizePtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorSamplingRateInfo(sensorRef,
        samplingRatesListPtr,samplingRatesListSizePtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetSupportedBatchCount

 DESCRIPTION    Gets info of sensor like min and max batch count supported.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetSupportedBatchCount
(
    taf_imuSensor_SensorRef_t sensorRef,
    uint32_t* maxBatchCountSupportedPtr,
    uint32_t* minBatchCountSupportedPtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorBatchingInfo(sensorRef,
        maxBatchCountSupportedPtr,minBatchCountSupportedPtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetSensorRangeInfo

 DESCRIPTION    Gets Info of sensor like range.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetRange
(
    taf_imuSensor_SensorRef_t sensorRef,
    double* rangePtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorRangeInfo(sensorRef,rangePtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetSensorResolution

 DESCRIPTION    Gets info of sensor resolution.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetResolution
(
    taf_imuSensor_SensorRef_t sensorRef,
    double* resolutionPtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorResolution(sensorRef,resolutionPtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_SetRefCoordinateByEulerAngle

 DESCRIPTION    Sets the euler angle to get rotated sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_SetRefCoordinateByEulerAngle
(
    taf_imuSensor_SensorRef_t sensorRef,
    double pitch,
    double roll,
    double yaw
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.SetEulerAngle(pitch,roll,yaw);
}

/*======================================================================

 FUNCTION       taf_imuSensor_Activate

 DESCRIPTION    Activate the sensor to get sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_UNSUPPORTED: if Sampling rate or batch count not supported.
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_Activate
(
    taf_imuSensor_SensorRef_t sensorRef,
    double samplingRate,
    uint32_t batchCount
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.Activate(sensorRef,samplingRate,batchCount);
}

/*======================================================================

 FUNCTION       taf_imuSensor_Deactivate

 DESCRIPTION    Deactivate the sensor to stop getting sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_Deactivate
(
    taf_imuSensor_SensorRef_t sensorRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.Deactivate(sensorRef);
}

/*======================================================================

 FUNCTION       taf_imuSensor_AddDataHandler

 DESCRIPTION    Sends sensor data event notification.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Data event handler function.

 RETURN VALUE   taf_imuSensor_DataHandlerRef_t if registered successfully else NULL.

 SIDE EFFECTS

======================================================================*/

taf_imuSensor_DataHandlerRef_t taf_imuSensor_AddDataHandler
(
    taf_imuSensor_SensorRef_t sensorRef,
    taf_imuSensor_DataHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.AddDataHandler(sensorRef,handlerPtr,contextPtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_RemoveDataHandler

 DESCRIPTION    Removes sensor OnEvent handler.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     Data event handler reference.

 RETURN VALUE

 SIDE EFFECTS

======================================================================*/

void taf_imuSensor_RemoveDataHandler
(
    taf_imuSensor_DataHandlerRef_t  handlerRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.RemoveDataHandler(handlerRef);
}

/*======================================================================

 FUNCTION       taf_imuSensor_GetRotatedData

 DESCRIPTION    Gets the raw data and bias for sensor sample list.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SampleRef_t sampleRef: reference to SampleRef.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_GetRotatedData
(
    taf_imuSensor_SampleRef_t sampleRef,
    taf_imuSensor_DataValue_t* RawDataPtr,
    size_t* RawDataSizePtr,
    taf_imuSensor_DataValue_t* BiasDataPtr,
    size_t* BiasDataSizePtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetData(sampleRef,RawDataPtr, RawDataSizePtr,BiasDataPtr,BiasDataSizePtr);
}

/*======================================================================

 FUNCTION       taf_imuSensor_DeleteData

 DESCRIPTION    Delete the list of sensor samples.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS     [IN] taf_imuSensor_SampleListRef_t sampleList: reference to SampleList.

 RETURN VALUE   le_result_t
                LE_BAD_PARAMETER: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_imuSensor_DeleteData
(
    taf_imuSensor_SampleRef_t sampleRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.DeleteData(sampleRef);
}