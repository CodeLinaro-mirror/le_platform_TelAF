/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

void TestSetEulerAngle(){
    le_result_t result;
    double pitch = 90, roll = 90, yaw = 90;
    LE_TEST_INFO("Testing Setting euler angle for Sensor with -taf_sensor_SetEulerAngle");
    result = taf_sensor_SetEulerAngle(NULL,pitch, roll, yaw);
    LE_TEST_OK(result == LE_OK, "taf_sensor_SetEulerAngle - LE_OK");
    LE_INFO("===== UnitTest Completed for setting euler angle =====");
}

void TestAvailableSensor()
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF GetAvailableSensors");
    taf_sensor_SensorListRef_t listRef = taf_sensor_GetSensorList();
    if(listRef != NULL){
        LE_TEST_INFO("GetSensorList SUCCESS");
    }else{
        LE_TEST_INFO("GetSensorList FAIELD");
    }
    taf_sensor_SensorListRef_t headTSensorListRef = listRef;
    taf_sensor_SensorRef_t sensorRef = taf_sensor_GetFirstSensor(listRef);
    if(sensorRef != NULL){
        LE_TEST_INFO("GetFirstSensor SUCCESS");
    }else{
        LE_TEST_INFO("GetFirstSenosr FAIELD");
    }
    while(sensorRef != NULL)
    {
        uint32_t id;
        char sensorName[50];
        char sensorVendorName[50];
        char version[10];
        taf_sensor_SensorType_t sensorType;
        result = taf_sensor_GetId(sensorRef,&id);
        LE_TEST_OK(result == LE_OK, "taf_sensor_GetId- LE_OK. id: %d",id);
        result = taf_sensor_GetName(sensorRef,sensorName,sizeof(sensorName));
        LE_TEST_OK(result == LE_OK, "taf_sensor_GetName- LE_OK. name: %s",sensorName);
        result = taf_sensor_GetVendorName(sensorRef,sensorVendorName,sizeof(sensorVendorName));
        LE_TEST_OK(result == LE_OK,"taf_sensor_GetVendorName- LE_OK. vendor = %s",sensorVendorName);
        result = taf_sensor_GetType(sensorRef,&sensorType);
        LE_TEST_OK(result == LE_OK,"taf_sensor_GetType - LE_OK. type = %d",sensorType);
        result = taf_sensor_GetVersion(sensorRef,version,sizeof(version));
        LE_TEST_OK(result == LE_OK,"taf_sensor_GetVersion - LE_OK. version = %s",version);

        double sampleRateList[TAF_SENSOR_MAX_NUM_SUPPORTED_SAMPLE_RATE];
        size_t size = sizeof(sampleRateList)/sizeof(double);
        result = taf_sensor_GetSamplingRateInfo(sensorRef,sampleRateList,&size);
        for(uint32_t i=0;i<size;i++){
            LE_TEST_OK(result == LE_OK, "taf_sensor_GetSamplingRateInfo - LE_OK. Sample%d %f",
                i+1,sampleRateList[i]);
        }

        uint32_t maxBatchCount;
        uint32_t minBatchCount;
        result = taf_sensor_GetBatchingInfo(sensorRef,&maxBatchCount,&minBatchCount);
        LE_TEST_OK(result == LE_OK, "taf_sensor_GetBatchingInfo- LE_OK."
        " MaxBatchCount =  %d , MinBatchCount = %d",maxBatchCount,minBatchCount);

        double range;
        result = taf_sensor_GetRange(sensorRef,&range);
        LE_TEST_OK(result == LE_OK, "taf_sensor_GetRange Info- LE_OK."
        " range = %f",range);

        double resolution;
        result = taf_sensor_GetResolution(sensorRef,&resolution);
        LE_TEST_OK(result == LE_OK, "taf_sensor_GetSensorResolution- LE_OK.%f",resolution);

        LE_TEST_INFO("Testing TelAF Next Sensor Reference Retrieval with -"
            "taf_sensor_GetNextSensor");
        sensorRef = taf_sensor_GetNextSensor(listRef);
        LE_TEST_OK((sensorRef != NULL), "taf_sensor_GetNextSensor - LE_OK");
    }
    LE_TEST_INFO("Testing TelAF deleting Sensor list with -taf_sensor_DeleteSensorList");
    result = taf_sensor_DeleteSensorList(headTSensorListRef);
    LE_TEST_OK(result == LE_OK, "taf_sensor_DeleteSensorList - LE_OK");
    LE_INFO("===== UnitTest Completed for retrieving information about Sensors =====");
    TestSetEulerAngle();
}


COMPONENT_INIT{
    taf_sensor_ConnectService();
    TestAvailableSensor();
    exit(EXIT_SUCCESS);
}
