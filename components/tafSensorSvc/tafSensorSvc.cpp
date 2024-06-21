/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <telux/sensor/SensorManager.hpp>
#include "tafSensor.hpp"
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

 FUNCTION       taf_sensor_GetSensorList

 DESCRIPTION    Get list of all available sensors.

 DEPENDENCIES   Initialization of sensor service

 RETURN VALUE   taf_sensor_SensorListRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_sensor_SensorListRef_t taf_sensor_GetSensorList
(
    void
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetAvailableSensors();
}

/*======================================================================

 FUNCTION       taf_sensor_DeleteSensorList

 DESCRIPTION    Delete a reference of sensor list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_sensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   le_result_t
                LE_NOT_FOUND: Fail
                LE_OK: Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_DeleteSensorList
(
    taf_sensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.DeleteSensorList(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_sensor_GetFirstSensor

 DESCRIPTION    Get first sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_sensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_sensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_sensor_SensorRef_t taf_sensor_GetFirstSensor
(
    taf_sensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetFirstSensor(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_sensor_GetNextSensor

 DESCRIPTION    Get next sensor reference from available sensors list.

 DEPENDENCIES   Initialization of SensorList

 PARAMETERS      [IN] taf_sensor_SensorListRef_t sensorListRef: reference to SensorList.

 RETURN VALUE   taf_sensor_SensorRef_t
                nullptr:     Fail
                non-nullptr: Success

 SIDE EFFECTS

======================================================================*/

taf_sensor_SensorRef_t taf_sensor_GetNextSensor
(
    taf_sensor_SensorListRef_t sensorListRef
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetNextSensor(sensorListRef);
}

/*======================================================================

 FUNCTION       taf_sensor_GetId

 DESCRIPTION    Gets the id of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetId
(
    taf_sensor_SensorRef_t sensorRef,
    uint32_t* sensorIdPtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorId(sensorRef,sensorIdPtr);
}

/*======================================================================

 FUNCTION       taf_sensor_GetName

 DESCRIPTION    Gets the name of the sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetName
(
    taf_sensor_SensorRef_t sensorRef,
    char* sensorName,
    size_t sensorNameSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorName(sensorRef,sensorName,sensorNameSize);
}

/*======================================================================

 FUNCTION       taf_sensor_GetVendorName

 DESCRIPTION    Gets the vendor name of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetVendorName
(
    taf_sensor_SensorRef_t sensorRef,
    char* sensorVendorName,
    size_t sensorVendorNameSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorVendorName(sensorRef,sensorVendorName,sensorVendorNameSize);
}

/*======================================================================

 FUNCTION       taf_sensor_GetType

 DESCRIPTION    Gets the type of sensor.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetType
(
    taf_sensor_SensorRef_t sensorRef,
    taf_sensor_SensorType_t* sensorTypePtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorType(sensorRef,sensorTypePtr);
}

/*======================================================================

 FUNCTION       taf_sensor_GetVersion

 DESCRIPTION    Gets the version of sensor.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetVersion
(
    taf_sensor_SensorRef_t sensorRef,
    char* version,
    size_t versionSize
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorVersion(sensorRef,version,versionSize);
}

/*======================================================================

 FUNCTION       taf_sensor_GetSensorSamplingRateInfo

 DESCRIPTION    Gets info of sensor like supported sampling rates.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetSamplingRateInfo
(
    taf_sensor_SensorRef_t sensorRef,
    double* samplingRatesListPtr,
    size_t* samplingRatesListSizePtr
)
{
     auto& sensorMngr = taf_Sensor::GetInstance();
     return sensorMngr.GetSensorSamplingRateInfo(sensorRef,
        samplingRatesListPtr,samplingRatesListSizePtr);
}

/*======================================================================

 FUNCTION       taf_sensor_GetSensorBatchingInfo

 DESCRIPTION    Gets info of sensor like min and max batch count supported.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetBatchingInfo
(
    taf_sensor_SensorRef_t sensorRef,
    uint32_t* maxBatchCountSupportedPtr,
    uint32_t* minBatchCountSupportedPtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorBatchingInfo(sensorRef,
        maxBatchCountSupportedPtr,minBatchCountSupportedPtr);
}

/*======================================================================

 FUNCTION       taf_sensor_GetSensorRangeInfo

 DESCRIPTION    Gets Info of sensor like range.

 DEPENDENCIES   Initialization of sensor list

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetRange
(
    taf_sensor_SensorRef_t sensorRef,
    double* rangePtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorRangeInfo(sensorRef,rangePtr);
}

/*======================================================================

 FUNCTION       taf_sensor_GetSensorResolution

 DESCRIPTION    Gets info of sensor resolution.

 DEPENDENCIES   Initialization of sensor list.

 PARAMETERS      [IN] taf_sensor_SensorRef_t sensorRef: reference to Sensor.

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_GetResolution
(
    taf_sensor_SensorRef_t sensorRef,
    double* resolutionPtr
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.GetSensorResolution(sensorRef,resolutionPtr);
}

/*======================================================================

 FUNCTION       taf_sensor_SetEulerAngle

 DESCRIPTION    Sets the euler angle to get rotated sensor data.

 DEPENDENCIES   Initialization of sensor service.

 PARAMETERS

 RETURN VALUE   le_result_t
                LE_FAULT: Fail
                LE_OK:    Success

 SIDE EFFECTS

======================================================================*/

le_result_t taf_sensor_SetEulerAngle
(
    taf_sensor_SensorRef_t sensorRef,
    double pitch,
    double roll,
    double yaw
)
{
    auto& sensorMngr = taf_Sensor::GetInstance();
    return sensorMngr.SetEulerAngle(pitch,roll,yaw);
}