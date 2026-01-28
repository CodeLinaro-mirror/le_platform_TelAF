/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Sensor Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void sensorRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("sensorRetTest_RunApis");

    //1.taf_imuSensor_GetFirstSensor NULL scenario
    taf_imuSensor_SensorRef_t sensor;
    sensor = taf_imuSensor_GetFirstSensor(NULL);
    LE_TEST_OK(sensor == NULL,"taf_imuSensor_GetFirstSensor-NULL");

    //2.taf_imuSensor_GetNextSensor NULL scenario
    sensor = taf_imuSensor_GetNextSensor(NULL);
    LE_TEST_OK(sensor == NULL,"taf_imuSensor_GetNextSensor-NULL");

    //3.taf_imuSensor_GetId LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetId-LE_BAD_PARAMETER");

    //4.taf_imuSensor_GetName LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetName-LE_BAD_PARAMETER");

    //5.taf_imuSensor_GetVendorName LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetVendorName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetVendorName-LE_BAD_PARAMETER");

    //6.taf_imuSensor_GetVersion LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetVersion(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetVersion-LE_BAD_PARAMETER");

    //7.taf_imuSensor_GetType LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetType(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetType-LE_BAD_PARAMETER");

    //8.taf_imuSensor_GetSupportedSamplingRate LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetSupportedSamplingRate(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetSupportedSamplingRate-LE_BAD_PARAMETER");

    //9.taf_imuSensor_GetSupportedBatchCount LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetSupportedBatchCount(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetSupportedBatchCount-LE_BAD_PARAMETER");

    //10.taf_imuSensor_GetRange LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetRange(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetRange-LE_BAD_PARAMETER");

    //11.taf_imuSensor_GetResolution LE_BAD_PARAMETER scenario
    res = taf_imuSensor_GetResolution(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_GetResolution-LE_BAD_PARAMETER");

    //12.taf_imuSensor_DeleteSensorList LE_BAD_PARAMETER scenario
    res = taf_imuSensor_DeleteSensorList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_DeleteSensorList-LE_BAD_PARAMETER");

    //13.taf_imuSensor_Activate LE_FAULT scenario
    res = taf_imuSensor_Activate(NULL,0,0);
    LE_TEST_OK(res == LE_FAULT,"taf_imuSensor_Activate-LE_FAULT");

    //14.taf_imuSensor_Deactivate LE_FAULT scenario
    res = taf_imuSensor_Deactivate(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_imuSensor_Deactivate-LE_FAULT");

    //15.taf_imuSensor_SelfTest LE_FAULT scenario
    res = taf_imuSensor_SelfTest(NULL,TAF_IMUSENSOR_POSITIVE,0);
    LE_TEST_OK(res == LE_FAULT,"taf_imuSensor_SelfTest-LE_FAULT");

    //16.taf_imuSensor_DeleteData LE_BAD_PARAMETER scenario
    res = taf_imuSensor_DeleteData(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_imuSensor_DeleteData-LE_BAD_PARAMETER");

}
