/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIvssSensorSvc.hpp"

using namespace v1::com::qualcomm::qti::telephony;

taf_imuSensor_SensorRef_t ivssSensorRefList[IVSS_SENSOR_MAX_NUM];

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss Sensor server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssSensorSvc> tafIvssSensorSvc::GetInstance()
{
    static std::shared_ptr<tafIvssSensorSvc> instance = std::make_shared<tafIvssSensorSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'GetSensorList'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::GetSensorListHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)reportPtr;
    taf_imuSensor_SensorRef_t sensorRef = NULL;

    // Get sensor list reference
    taf_imuSensor_SensorListRef_t listRef = taf_imuSensor_GetSensorList();
    if (listRef == NULL)
    {
        LE_INFO("GetSensorList returned NULL: No sensor information available.\n");
        le_sem_Post(indPtr->semRef);
        return;
    }

    // Get sensors info
    for (uint32_t i = 0; i < IVSS_SENSOR_MAX_NUM; i++)
    {
        if (i == 0)
        {
            sensorRef = taf_imuSensor_GetFirstSensor(listRef);
        }
        else
        {
            sensorRef = taf_imuSensor_GetNextSensor(listRef);

        }

        if (sensorRef == NULL)
        {
            LE_INFO("GetSensorRef: sensor %d returned NULL.\n", i + 1);
            le_sem_Post(indPtr->semRef);
            return;
        }

        indPtr->result = taf_imuSensor_GetId(sensorRef, &indPtr->getSensorList.sensorInfo[i].id);
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_imuSensor_GetId fail - %s", LE_RESULT_TXT(indPtr->result));

        indPtr->result = taf_imuSensor_GetName(sensorRef,
            indPtr->getSensorList.sensorInfo[i].name,
            sizeof(indPtr->getSensorList.sensorInfo[i].name));
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_imuSensor_GetName fail - %s", LE_RESULT_TXT(indPtr->result));

        indPtr->result = taf_imuSensor_GetVendorName(sensorRef,
            indPtr->getSensorList.sensorInfo[i].vendorName,
            sizeof(indPtr->getSensorList.sensorInfo[i].vendorName));
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_imuSensor_GetVendorName fail - %s", LE_RESULT_TXT(indPtr->result));

        indPtr->result = taf_imuSensor_GetVersion(sensorRef,
            indPtr->getSensorList.sensorInfo[i].version,
            sizeof(indPtr->getSensorList.sensorInfo[i].version));
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_imuSensor_GetVersion fail - %s", LE_RESULT_TXT(indPtr->result));

        indPtr->result = taf_imuSensor_GetType(sensorRef,
            &indPtr->getSensorList.sensorInfo[i].type);
        TAF_ERROR_IF_COND_POST_SEM(indPtr->result != LE_OK, indPtr->semRef,
            "taf_imuSensor_GetType fail - %s", LE_RESULT_TXT(indPtr->result));

        indPtr->getSensorList.sensorNum++;
    }

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the sensor's data (Latitude, Longitude, Horizontal accuracy).
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::GetSensorList(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetSensorListReply_t _reply)
{
    // Create a generic response message object.
    LE_INFO("tafIvssSensorSvc GetSensorList \n");

    taf_IvssSensor_Ind_t* indPtr = (taf_IvssSensor_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssSensor_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetSensorListSem", 0);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetSensorListEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);

    std::vector<SensorSvcTypes::SensorInfoT> sensorInfo = {};
    for (uint32_t i = 0; i < indPtr->getSensorList.sensorNum; i++)
    {
        SensorSvcTypes::SensorInfoT info = {};
        info.setSensorId(indPtr->getSensorList.sensorInfo[i].id);
        info.setSensorName(std::string(indPtr->getSensorList.sensorInfo[i].name));
        info.setSensorVendorName(std::string(indPtr->getSensorList.sensorInfo[i].vendorName));
        info.setSensorVersion(std::string(indPtr->getSensorList.sensorInfo[i].version));
        info.setSensorType(SensorTypeToIvss(indPtr->getSensorList.sensorInfo[i].type));
        sensorInfo.push_back(info);
    }
    _reply(indPtr->getSensorList.sensorNum, sensorInfo, ResultLeToIvssSensor(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssSensorSvc::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Sensor EventPool", sizeof(taf_IvssSensor_Ind_t));

    // Init events.
    GetSensorListEvent = le_event_CreateIdWithRefCounting("GetSensorListEvent");

    // Init event handler.
    GetSensorListEventHandlerRef = le_event_AddHandler("GetSensorListEvent Handler",
        GetSensorListEvent, tafIvssSensorSvc::GetSensorListHandler);

    LE_INFO("tafIvssSensorSvc Service initialized");
};
