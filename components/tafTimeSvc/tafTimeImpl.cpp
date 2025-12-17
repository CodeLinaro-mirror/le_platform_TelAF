/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafTime.hpp"
#include "taf_gptpTime.h"

using namespace tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Object to store time source configuration.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_TIME_MAX_SOURCE_NUMBER (TAF_TIME_SRC_NAME_UNKNOWN*3)
TimeSources TimeSourceConf(TAF_TIME_MAX_SOURCE_NUMBER);
taf_SourceInf_t *LatestTimeSourceInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Global variables for logic control.
 */
//--------------------------------------------------------------------------------------------------
bool GnssErrStatusUpdateFlag = true;
le_result_t GnssBaseDataIntStatus = LE_UNAVAILABLE;
le_result_t InitGnssManagerStatus = LE_UNAVAILABLE;
le_result_t NetworkBaseDataIntStatus = LE_UNAVAILABLE;
le_result_t InitNetworkManagerStatus = LE_UNAVAILABLE;

le_result_t RtcAsyncBaseDataIntStatus = LE_UNAVAILABLE;

le_result_t MssConnectStatusMainThread = LE_FAULT;

taf_time_setRTCCb_t tafsvc::taf_Time::setRTCCBtoClient;
taf_time_getRTCCb_t tafsvc::taf_Time::getRTCCBtoClient;

//--------------------------------------------------------------------------------------------------
/**
 * Lock and condition used for time manager.
 */
//--------------------------------------------------------------------------------------------------
std::mutex mtx;
std::condition_variable cv;

//--------------------------------------------------------------------------------------------------
/**
 * Initiate listener for serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_TimeServingSystemListener::taf_TimeServingSystemListener
(
    uint8_t phoneId ///< [IN] Phone ID.
) : phone(phoneId)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for network time changes.
 */
//--------------------------------------------------------------------------------------------------
void taf_TimeServingSystemListener::onNetworkTimeChanged
(
    telux::tel::NetworkTimeInfo info ///< [IN] Network time information.
)
{
    taf_time_TimeSpec_t timeVal = {0};
    taf_time_TimeSources_t sourceId;
    le_result_t result = LE_OK;

    if (InitNetworkManagerStatus != LE_OK)
    {
        // This is used to avoid 'pure virtual method called' crash issue during shut down.
        // 1. Don't use the destructor of "taf_Time::".
        // 2. Need to exit at once before sending another event to event loop.
        LE_DEBUG("Flag 'InitNetworkManagerStatus' was disabled, do nothing.");
        return;
    }

    auto &tafTime = taf_Time::GetInstance();

    LE_INFO("Phone %d, NITZ:%s\n", phone, info.nitzTime.c_str());
    result = tafTime.ConvertNetworkTimeToSec(info, &timeVal);
    if (tafTime.phoneManager->getPhoneIdFromSlotId(phone)  == 1)
    {
        sourceId = TAF_TIME_SRC_NAME_NETWORK;
        if (LE_OK == result)
        {
            tafTime.UpdateLocalTimeCache(timeVal, sourceId, tafTime.NetworkDeltaTime);
        }
    }
    else
    {
        sourceId = TAF_TIME_SRC_NAME_NETWORK2;
        if (LE_OK == result)
        {
            tafTime.UpdateLocalTimeCache(timeVal, sourceId, tafTime.NetworkDeltaTime2);
        }
    }

    tafTime.StoreDateTimeInfo(info, sourceId);
    tafTime.ReportTimeValueChange(sourceId, timeVal, &info);
}
void onGnssUtcTimeUpdateHandler(void* param)
{
    taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_GNSS;
    auto &tafTime = taf_Time::GetInstance();
    le_result_t* status = (le_result_t*) param;
    tafTime.SourceStatusUpdate(*status, sourceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * GNSS UTC time notification for local GNSS time update.
 */
//--------------------------------------------------------------------------------------------------

void taf_TimeGnssListener::onGnssUtcTimeUpdate
(
    const uint64_t utc
)
{
    taf_time_TimeSpec_t timeVal;

    if (InitGnssManagerStatus != LE_OK)
    {
        // This is used to avoid 'pure virtual method called' crash issue during shut down.
        // 1. Don't use the destructor of "taf_Time::".
        // 2. Need to exit at once before sending another event to event loop.
        LE_DEBUG("Flag 'InitGnssManagerStatus' was disabled, do nothing.");
        return;
    }

    auto &tafTime = taf_Time::GetInstance();
    le_result_t status;
    if (utc == 0)
    {
       if(GnssErrStatusUpdateFlag == true)
       {
           GnssErrStatusUpdateFlag = false;
           status = LE_FAULT;
            le_event_QueueFunctionToThread(tafTime.mainThreadRef,
                (le_event_DeferredFunc_t)onGnssUtcTimeUpdateHandler, &status, NULL);
        }
    }

    else if(utc > 0)
    {
        status = LE_OK;
        le_event_QueueFunctionToThread(tafTime.mainThreadRef,
            (le_event_DeferredFunc_t)onGnssUtcTimeUpdateHandler,&status, NULL);
        tafTime.UpdateFailedLoops(TAF_TIME_SRC_NAME_GNSS, FAIL_LOOP_NUM_CLEAN);
        timeVal.sec = (utc / 1000);
        timeVal.nanosec = (utc % 1000)*1000*1000;

        LE_DEBUG("Received gnss UTC time: %" PRIu64 "\n", timeVal.sec);
        tafTime.UpdateLocalTimeCache(timeVal, TAF_TIME_SRC_NAME_GNSS, tafTime.GnssDeltaTime);
        tafTime.ReportTimeValueChange(TAF_TIME_SRC_NAME_GNSS, timeVal, NULL);
        tafTime.DeregGnssTimeListener();
        GnssErrStatusUpdateFlag = true;

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get time service instance for client class.
 */
//--------------------------------------------------------------------------------------------------
taf_Time &taf_Time::GetInstance()
{
    static taf_Time instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert time source index name to string.
 */
//--------------------------------------------------------------------------------------------------
const char* taf_Time::SourceNameIndexToStr
(
    taf_time_TimeSources_t sourceName
)
// -------------------------------------------------------------------------------------------------
{
    switch (sourceName)
    {
        case TAF_TIME_SRC_NAME_RTC:
            return "RTC";

        case TAF_TIME_SRC_NAME_GNSS:
            return "GNSS";

        case TAF_TIME_SRC_NAME_EX_APP:
            return "ExAPP";

        case TAF_TIME_SRC_NAME_NETWORK:
            return "NETWORK";

        case TAF_TIME_SRC_NAME_NETWORK2:
            return "NETWORK2";

        /* Add new time source here */

        case TAF_TIME_SRC_NAME_UNKNOWN:
            return "UNKNOWN";

        case TAF_TIME_SRC_NAME_SYSTEM:
            return "SYSTEM";

    }

    return "UNKNOWN";
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert time source name string to index type.
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeSources_t taf_Time::SourceNameStrToIndex
(
    const char* typeNamePtr  ///< The index of the command line argument to read.
)
// -------------------------------------------------------------------------------------------------
{
    // Check the given name against what we're expecting.
    if (strncmp(typeNamePtr, "RTC", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_RTC;
    }
    else if (strncmp(typeNamePtr, "GNSS", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_GNSS;
    }
    else if (strncmp(typeNamePtr, "ExAPP", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_EX_APP;
    }
    else if (strncmp(typeNamePtr, "NETWORK", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_NETWORK;
    }
    else if (strncmp(typeNamePtr, "NETWORK2", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_NETWORK2;
    }
    else if (strncmp(typeNamePtr, "SYSTEM", TAF_TIME_STR_MAX) == 0)
    {
        return TAF_TIME_SRC_NAME_SYSTEM;
    }

    return TAF_TIME_SRC_NAME_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert time source attribute index name to string.
 */
//--------------------------------------------------------------------------------------------------
const char* taf_Time::SourceAttrToStr
(
    taf_Time_SrcAttr_t sourceConf
)
// -------------------------------------------------------------------------------------------------
{
    switch (sourceConf)
    {
        case TAF_TIME_CONF_SOURCE:
            return "Source";

        case TAF_TIME_CONF_SETTIME:
            return "SetTime";

        case TAF_TIME_CONF_PRIORI:
            return "Priority";

        case TAF_TIME_CONF_TOLMILLSEC:
            return "ToleranceMillsec";

        case TAF_TIME_CONF_SETTIMECOUNTER:
            return "SetTimeCounter";

        /* Add new source item here */

        case TAF_TIME_CONF_MAX_ITEM:
            return "unknown";
    }
    return "unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 * Read source configuration items from JSON file.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Related items or file cannot be found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ReadSourceConf
(
    TimeSources& serviceCfg,
    const json_t *serviceDataPtr
)
{
    le_result_t result = LE_NOT_FOUND;//Not found is allowed
    json_t *arrayData,*itemData, *sourceCfgArray;
    const char* value;
    int priority;
    long int toleranceMillsec, setTimeCounter= -1;
    int i, j;
    long int arraySize = 0;

    sourceCfgArray = json_object_get(serviceDataPtr, TAF_TIME_SERVICE_SOURCE_STR);
    if (!json_is_array(sourceCfgArray))
    {
        LE_WARN("Warning: serviceData is not set or not an array\n");
        return LE_NOT_FOUND;
    }

    arraySize = json_array_size(sourceCfgArray);
    LE_INFO("%s arraySize: %ld\n", TAF_TIME_SERVICE_SOURCE_STR, arraySize);

    if (arraySize > TAF_TIME_MAX_SOURCE_NUMBER)
    {
        arraySize = TAF_TIME_MAX_SOURCE_NUMBER;
        LE_WARN("Source number: %ld, but we only accept: %d\n",
                                    arraySize, TAF_TIME_MAX_SOURCE_NUMBER);
    }
    serviceCfg.sourceArrySize = arraySize;
    for (i = 0; i < arraySize; i++)
    {
        arrayData = json_array_get(sourceCfgArray, i);
        if (!json_is_object(arrayData))
        {
            LE_INFO("Error: data %d is not an object", i);
            result = LE_FAULT;
            break;
        }
        for (j = 0; j < (int)TAF_TIME_CONF_MAX_ITEM; j++)
        {
            itemData = json_object_get(arrayData, SourceAttrToStr((taf_Time_SrcAttr_t)j));

            if (json_is_string(itemData))
            {
                value = json_string_value(itemData);
                LE_INFO("%s:%s\n", SourceAttrToStr((taf_Time_SrcAttr_t)j), value);

                if (j == TAF_TIME_CONF_SOURCE)
                {
                    serviceCfg.addSourceName(i, value);
                }
                if (j == TAF_TIME_CONF_SETTIME)
                {
                    if (0 == strncmp(value, "true", 4))
                        serviceCfg.addSetTimeFlag(i, 1);
                    else
                        serviceCfg.addSetTimeFlag(i, 0);
                }
                if (j == TAF_TIME_CONF_PRIORI)
                {
                    sscanf(value,"%d", &priority);
                    serviceCfg.addPriority(i, priority);
                }
                if (j == TAF_TIME_CONF_TOLMILLSEC)
                {
                    sscanf(value,"%ld", &toleranceMillsec);
                    serviceCfg.addToleranceMillsec(i, toleranceMillsec);
                }
                if (j == TAF_TIME_CONF_SETTIMECOUNTER)
                {
                    sscanf(value,"%ld", &setTimeCounter);
                    serviceCfg.addSetTimeCounter(i, setTimeCounter);
                }
                result = LE_OK;
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read time related configuration items from JSON file.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Related items or file cannot be found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ReadTimeConf
(
    TimeSources& serviceCfg,
    const json_t *serviceDataPtr
)
{
    json_t *itemData;
    const char* value;
    long int pollingInterval;
    int64_t allowOverrideAfterFail = -1;
    std::vector<std::string> validClientList;
    std::string gptpDeviceName;

    itemData = json_object_get(serviceDataPtr, TAF_TIME_INTERVAL_SETTING_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: PollingInterval string was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    sscanf(value,"%ld", &pollingInterval);
    serviceCfg.pollingInterval = pollingInterval;

    itemData = json_object_get(serviceDataPtr, TAF_TIME_ALLOWOVERRIDE_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: AllowOverrideAfterFail string was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    sscanf(value, "%" PRId64 "", &allowOverrideAfterFail);
    serviceCfg.allowOverrideAfterFail = allowOverrideAfterFail;
    AllowOverrideAfterFail = allowOverrideAfterFail;

    itemData = json_object_get(serviceDataPtr, TAF_TIME_VALIDCLIENTLIST_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: Valid Client list was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    std::stringstream ss(value);
    std::string client;
    while(ss >> client)
    {
        validClientList.push_back(client.c_str());
    }

    serviceCfg.validClientList = validClientList;
    itemData = json_object_get(serviceDataPtr, TAF_TIME_GPTPDEVICENAME_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: Gptp device name string was not found\n");
        return LE_NOT_FOUND;
    }
    value = json_string_value(itemData);
    serviceCfg.gptpDeviceName = value;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the JSON file and read related configuration items from it.
 * If the file/item not exist, will skip and continue.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Related items or file cannot be found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::LoadJsonConfiguration
(
    TimeSources& serviceCfg,  ///< buffer to store the configuration items
    const char* filePathPtr   ///< Load the JSON from a file at this path.
)
{
    le_result_t result = LE_NOT_FOUND;//Not found is allowed
    json_t *rootPtr, *serviceDataPtr;
    json_error_t error;

    rootPtr = json_load_file(filePathPtr, 0, &error);
    if (rootPtr == NULL)
    {
        fprintf(stderr,
        "JSON import error: line: %d, column: %d, position: %d, source: '%s', error: %s",
        error.line,
        error.column,
        error.position,
        error.source,
        error.text);

        return result;
    }

    serviceDataPtr = json_object_get(rootPtr, TAF_TIME_SERVICE_HEADER_STR);
    if (!json_is_object(serviceDataPtr))
    {
        LE_WARN("Warning: Cannot get the TimeService configuration\n");
        json_decref(rootPtr);
        return result;
    }

    result = ReadSourceConf(serviceCfg, serviceDataPtr);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Cannot read time source configuration\n");
        json_decref(rootPtr);
        return result;
    }

    result = ReadTimeConf(serviceCfg, serviceDataPtr);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Cannot read time configuration\n");
        json_decref(rootPtr);
        return result;
    }

    json_decref(rootPtr);
    return result;
}

void taf_Time::DeleteNotSupportedSource
(
    TimeSources& serviceCfg
)
{
    int i;
    std::vector<std::string> srcName;

    for (i = 0; i < serviceCfg.sourceArrySize; i++)
    {
        if (SourceNameStrToIndex(serviceCfg.source[i].sourceName.c_str())
                                            == TAF_TIME_SRC_NAME_UNKNOWN)
        {
            srcName.push_back(serviceCfg.source[i].sourceName);
        }
    }
    serviceCfg.deleteSourceByNameList(srcName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Subtract two time values and return the result.
 *
 * @return
 *      The result of (timeA - timeB)
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeSpec_t taf_Time::taf_time_Sub
(
    taf_time_TimeSpec_t timeA,
    taf_time_TimeSpec_t timeB
)
{
    taf_time_TimeSpec_t result;
    result.sec = timeA.sec - timeB.sec;

    if ( timeA.nanosec < timeB.nanosec )
    {
        // Move one second to nsec
        result.sec--;
        timeA.nanosec += TAF_TIME_NSEC_PER_SEC;
    }
    result.nanosec = timeA.nanosec - timeB.nanosec;
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add two time values together and return the result.
 *
 * @return
 *      The result of (timeA + timeB)
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeSpec_t taf_Time::taf_time_Add
(
    taf_time_TimeSpec_t timeA,
    taf_time_TimeSpec_t timeB
)
{
    taf_time_TimeSpec_t result;

    result.sec = timeA.sec + timeB.sec;
    result.nanosec = timeA.nanosec + timeB.nanosec;

    if (result.nanosec >= TAF_TIME_NSEC_PER_SEC)
    {
        // Move one second from nsec to sec
        result.nanosec -= TAF_TIME_NSEC_PER_SEC;
        result.sec++;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Compare two time values and return the result.
 *
 * @return
 *      - TRUE -- if (timeA > timeB)
 *      - FALSE -- if (timeA < timeB)
 */
//--------------------------------------------------------------------------------------------------
bool taf_Time::TimeGreaterThan
(
    taf_time_TimeSpec_t timeA,
    taf_time_TimeSpec_t timeB
)
{
    if (timeA.sec == timeB.sec)
    {
        return (timeA.nanosec > timeB.nanosec);
    }
    return (timeA.sec > timeB.sec);
}

//--------------------------------------------------------------------------------------------------
/**
 * Maintain the buffer for time source.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ReadWriteDeltaTime
(
    taf_time_TimeSpec_t* timeValPtr,
    taf_time_TimeSpec_t* deltaTimeDataPtr,
    taf_TimeReadWrite_t ReadWriteType
)
{
    le_result_t result = LE_OK;

    if (deltaTimeDataPtr == NULL && ReadWriteType != TAF_TIME_DATA_CLEAN)
    {
        LE_ERROR("Bad parameter, does it initialized ?\n");
        return LE_BAD_PARAMETER;
    }

    pthread_mutex_lock(&ProtectlocalTime_mutex);

    switch (ReadWriteType)
    {
        case TAF_TIME_DATA_READ:
            // Copy local maintained data 'deltaTimeData' to buf 'timeValPtr'
            memcpy(timeValPtr, deltaTimeDataPtr, sizeof(taf_time_TimeSpec_t));
            break;

        case TAF_TIME_DATA_UPDATE:
            // Update new time to local buffer 'deltaTimeData'.
            memcpy(deltaTimeDataPtr, timeValPtr, sizeof(taf_time_TimeSpec_t));
            break;

        case TAF_TIME_DATA_CLEAN:
            // Clean local buffer
            memset(timeValPtr, 0, sizeof(taf_time_TimeSpec_t));
            break;

        default:
            result = LE_FAULT;
            LE_ERROR("Unownk type %d\n", ReadWriteType);
            break;
    }

    pthread_mutex_unlock(&ProtectlocalTime_mutex);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update the delta (UTC - Boot) time to the buffer.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::UpdateLocalTimeCache
(
    taf_time_TimeSpec_t newTime,
    taf_time_TimeSources_t sourceName,
    taf_time_TimeSpec_t* deltaTimeDataBufferPtr
)
{
    le_result_t result;
    uint64_t deltaMilliSec, milliSecThreshold;
    taf_time_TimeSpec_t bootTime, oldDeltaTime, newDeltaTime, largerDtTime;

    oldDeltaTime.sec = 0;
    oldDeltaTime.nanosec = 0;

    if (newTime.sec <= 0 || deltaTimeDataBufferPtr == NULL)
    {
        LE_ERROR("Parameter not correct\n");
        return LE_BAD_PARAMETER;
    }

    result = GetBootTime(&bootTime);
    if (result)
    {
        LE_ERROR("Get boot time failed\n");
        return result;
    }

    result = ReadWriteDeltaTime(&oldDeltaTime,
                            deltaTimeDataBufferPtr, TAF_TIME_DATA_READ);
    if (result)
    {
        LE_ERROR("Read detla time failed\n");
        return result;
    }
    newDeltaTime = taf_time_Sub(newTime, bootTime);
    if (TimeGreaterThan(newDeltaTime, oldDeltaTime))
    {
        largerDtTime = taf_time_Sub(newDeltaTime, oldDeltaTime);
    }
    else
    {
        largerDtTime = taf_time_Sub(oldDeltaTime, newDeltaTime);
    }

    int position = TimeSourceConf.findSourcePosition(
                                SourceNameIndexToStr(sourceName));

    if (position < 0)
    {
        LE_ERROR("Source does not exist!");
        return LE_NOT_FOUND;
    }

    if (TimeSourceConf.source[position].toleranceMillsec <= 0)
    {
        milliSecThreshold = TAF_TIME_THRESHOLD_MILLISEC;
    }
    else
    {
        milliSecThreshold = TimeSourceConf.source[position].toleranceMillsec;
    }

    deltaMilliSec = largerDtTime.sec * 1000 + largerDtTime.nanosec/1000/1000;
    if (deltaMilliSec > milliSecThreshold)
    {
        result =  ReadWriteDeltaTime(&newDeltaTime,
                                deltaTimeDataBufferPtr, TAF_TIME_DATA_UPDATE);
        if (result)
        {
            LE_ERROR("Update detla time failed\n");
            return result;
        }

        LE_DEBUG("Update time for %s to: sec %" PRIu64 ", nsec %" PRIu64 ","
            "DT mSec %" PRIu64 ", Threshold %" PRIu64 "", SourceNameIndexToStr(sourceName),
            newTime.sec, newTime.nanosec, deltaMilliSec, milliSecThreshold);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get time from local maintained time source.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_NOT_FOUND -- If no data available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetTimeFromLocalCache
(
    taf_time_TimeSpec_t* timeValPtr,
    taf_time_TimeSpec_t* deltaTimeDataPtr,
    taf_time_TimeSources_t sourceName
)
{
    le_result_t result;
    taf_time_TimeSpec_t bootTime, deltaTime;

    if (deltaTimeDataPtr == NULL || timeValPtr == NULL)
    {
        LE_ERROR("Not initialized %s buffer\n", SourceNameIndexToStr(sourceName));
        return LE_UNAVAILABLE;
    }

    timeValPtr->sec = 0;
    timeValPtr->nanosec = 0;

    result = GetBootTime(&bootTime);
    if (result)
    {
        LE_ERROR("Unable to read system boot time\n");
        return result;
    }

    result = ReadWriteDeltaTime(&deltaTime, deltaTimeDataPtr, TAF_TIME_DATA_READ);
    if (result)
    {
        LE_ERROR("Read %s detla time failed\n", SourceNameIndexToStr(sourceName));
        return result;
    }

    if (deltaTime.sec <= 0)
    {
        LE_DEBUG("%s time not ready\n", SourceNameIndexToStr(sourceName));
        return LE_UNAVAILABLE;
    }

    LE_DEBUG("Get %s time success: dt sec "
                   "%" PRIu64 " , nsec %" PRIu64 ", Dt sec %" PRIu64 " , nsec %" PRIu64 "\n",
                    SourceNameIndexToStr(sourceName),
         deltaTime.sec, deltaTime.nanosec, deltaTimeDataPtr->sec, deltaTimeDataPtr->nanosec);

    *timeValPtr = taf_time_Add(bootTime, deltaTime);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get system boot time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetBootTime
(
    taf_time_TimeSpec_t* timeValPtr
)
{
    struct timespec bootTime;

    if ( clock_gettime(CLOCK_BOOTTIME, &bootTime) < 0 )
    {
        LE_ERROR("Get boot time failed");
        return LE_FAULT;
    }
    timeValPtr->sec = bootTime.tv_sec;
    timeValPtr->nanosec = bootTime.tv_nsec;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get system CLOCK_REAL time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetSystemTime
(
    taf_time_TimeSpec_t* timeValPtr
)
{
    struct timespec systemTime;

    if ( clock_gettime(CLOCK_REALTIME, &systemTime) < 0 )
    {
        return LE_FAULT;
    }

    timeValPtr->sec = systemTime.tv_sec;
    timeValPtr->nanosec = systemTime.tv_nsec;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get local maintained GNSS time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_UNAVAILABLE -- If no data available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetGnssTime
(
    taf_time_TimeSpec_t* timeValPtr
)
{
    return GetTimeFromLocalCache(timeValPtr, GnssDeltaTime, TAF_TIME_SRC_NAME_GNSS);

}

//--------------------------------------------------------------------------------------------------
/**
 * Get local maintained network time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_NOT_FOUND -- If no data available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetNetworkTime
(
    taf_time_TimeSpec_t* timeValPtr,
    taf_time_TimeSources_t sourceId
)
{
    if(sourceId == TAF_TIME_SRC_NAME_NETWORK)
    {
        return GetTimeFromLocalCache(timeValPtr, NetworkDeltaTime, sourceId);
    }
    else
    {
        return GetTimeFromLocalCache(timeValPtr, NetworkDeltaTime2, sourceId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Read time from RTC device or VHAL interface.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetRtcTime
(
    taf_time_TimeSpec_t* timeValPtr,
    bool isAllowGetInternalRTCTime
)
{
    le_result_t result = LE_FAULT;
    timeValPtr->sec = 0;
    timeValPtr->nanosec = 0;

    if (isDrvPresent)
    {
        if ((*(timeInf->getRtcTimeReqAsync)) != nullptr)
        {
            result = GetTimeFromLocalCache(timeValPtr, RtcAsyncDeltaTimePtr, TAF_TIME_SRC_NAME_RTC);
        }
        else if ((*(timeInf->getRtcTimeHAL)) != nullptr)
        {
            struct TimeSpec obj;
            result = (*(timeInf->getRtcTimeHAL))(&obj);
            if (result < 0 || obj.sec <= 0)
            {
                LE_ERROR("getRtcTimeHAL failed, obj.sec: %" PRIu64 "", obj.sec);
                return LE_FAULT;
            }
            timeValPtr->sec = obj.sec;
            timeValPtr->nanosec = obj.nanosec;
        }
        else
        {
            LE_ERROR("getRtcTimeHAL and getRtcTimeReqAsync are not initialized");
        }
    }
    else if(isAllowGetInternalRTCTime)
    {
        result = GetInternalRtcTime(timeValPtr);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get external set time status for current run loop.
 *
 * @return
 *     - LE_OK -- External set time occurred.
 *     - LE_TIMEOUT -- External set time did not occur.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetExSetTimeStatus(void)
{
    if (SetTimeSt->externalSetTime)
    {
        //Clear the flag here, and it needs to be set 'true' through API
        //'setSystemTime' by external function
        SetTimeSt->externalSetTime = false;
        SourceStatusUpdate(LE_FAULT, TAF_TIME_SRC_NAME_EX_APP);
        return LE_OK;
    }
    return LE_TIMEOUT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a source instance object by source ID, return the object pointer.
 */
//--------------------------------------------------------------------------------------------------

taf_TimeInf_t* taf_Time::SearchSourceInfList
(
    taf_time_TimeSources_t sourceId,  ///< Time source ID.
    le_msg_SessionRef_t sessionRef,
    bool handlerFlag
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(TimeRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_TimeInf_t* srcTimePtr = (taf_TimeInf_t*)le_ref_GetValue(iterRef);
        if ((srcTimePtr != NULL)
            && (srcTimePtr->sourceId == sourceId)
            && (srcTimePtr->sessionRef == sessionRef)
            && ((srcTimePtr->handlerRef && handlerFlag) || (!srcTimePtr->handlerRef && !handlerFlag))
            )
        {
            return srcTimePtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a network time instance object by source ID, return the object pointer.
 */
//--------------------------------------------------------------------------------------------------
taf_TimeNetTimeInfo_t* taf_Time::SearchNetTimeInfList
(
    taf_time_TimeSources_t sourceId  ///< Time source ID.
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(netTimeInfoRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_TimeNetTimeInfo_t* srcTimePtr = (taf_TimeNetTimeInfo_t*)le_ref_GetValue(iterRef);
        if ((srcTimePtr != NULL) && (srcTimePtr->sourceId == sourceId))
        {
            return srcTimePtr;
        }
    }
    return NULL;
}

le_result_t taf_Time::UpdateRefTimeInfo
(
    taf_TimeInf_t* timeSrcRefPrt,
    taf_time_TimeSpec_t* timeValPtr
)
{
    le_result_t result;
    taf_time_TimeSpec_t sysTime;
    timespec gPtpimeVal;

    if (timeSrcRefPrt == NULL)
    {
        LE_ERROR("srcTimePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    //skip for system time src
    if(timeSrcRefPrt->sourceId != TAF_TIME_SRC_NAME_SYSTEM)
    {
        result = GetSystemTime(&sysTime);
        if (result != LE_OK)
        {
            timeSrcRefPrt->dateTimeInf.referRealTime = {0};
            LE_ERROR("Get reference system time for %s failed",
                            SourceNameIndexToStr(timeSrcRefPrt->sourceId));
            return result;
        }
        timeSrcRefPrt->dateTimeInf.referRealTime.sec = sysTime.sec;
        timeSrcRefPrt->dateTimeInf.referRealTime.nanosec = sysTime.nanosec;
    }
    if(gptpTimeRef != NULL)
    {
        result = taf_gptpTime_GetTimeValue(gptpTimeRef, &gPtpimeVal);

        if (result != LE_OK)
        {
            timeSrcRefPrt->dateTimeInf.referPtpTime = {0};
            LE_WARN("Get reference gptp time for %s failed",
                            SourceNameIndexToStr(timeSrcRefPrt->sourceId));
        }
        else
        {
            timeSrcRefPrt->dateTimeInf.referPtpTime.sec = gPtpimeVal.tv_sec;
            timeSrcRefPrt->dateTimeInf.referPtpTime.nanosec = gPtpimeVal.tv_nsec;
        }
    }

    return LE_OK;
}

le_result_t taf_Time::UpdateDateTimeInfo
(
    taf_TimeInf_t* timeSrcRefPrt,
    taf_time_TimeSpec_t* timeValPtr
)
{
    int ret;
    time_t secs;
    struct tm *time_info;
    std::string timeZoneStr;
    std::string nitzTimeStr = "";

    if (timeSrcRefPrt == NULL)
    {
        LE_ERROR("timeSrcRefPrt is NULL.");
        return LE_BAD_PARAMETER;
    }

    taf_TimeNetTimeInfo_t* netInfoPtr = SearchNetTimeInfList(timeSrcRefPrt->sourceId);
    if (netInfoPtr == NULL || netInfoPtr->timeInfo.nitzTime[0] == '\0')
    {
        LE_ERROR("Network time is not ready");
        return LE_UNAVAILABLE;
    }

    timeSrcRefPrt->dateTimeInf.timeZone = netInfoPtr->timeInfo.timeZone;
    timeSrcRefPrt->dateTimeInf.dstAdj = netInfoPtr->timeInfo.dstAdj;
    timeSrcRefPrt->dateTimeInf.dayOfWeek = netInfoPtr->timeInfo.dayOfWeek;
    nitzTimeStr = std::string(netInfoPtr->timeInfo.nitzTime);

    size_t first_colon_pos = nitzTimeStr.find(":");
    if ((first_colon_pos > 0) && (first_colon_pos + 6 < nitzTimeStr.size()))
    {
        secs = timeValPtr->sec;

        time_info = gmtime(&secs);
        timeZoneStr = nitzTimeStr.substr(first_colon_pos + 6);

        /* The convertion for date time and "secs":
        time_info.tm_year = year - 1900;  // Year - 1900
        time_info.tm_mon = month - 1;     // Month (0-11, where 0 is January)
        time_info.tm_mday = day;          // Day of the month (1-31)
        time_info.tm_hour = hour;         // Hour    (0-23)
        time_info.tm_min = minute;        // Minutes (0-59)
        time_info.tm_sec = second;        // Seconds (0-61, including leap seconds)
        */
        if (time_info != NULL)
        {
            ret = snprintf(timeSrcRefPrt->dateTimeInf.nitzTime, NITZ_STR_BUF_MAX,
                   "%04d/%02d/%02d,%02d:%02d:%02d%s", time_info->tm_year + 1900,
                  time_info->tm_mon + 1, time_info->tm_mday, time_info->tm_hour,
                     time_info->tm_min, time_info->tm_sec, timeZoneStr.c_str());

            if (ret >= NITZ_STR_BUF_MAX)
            {
                LE_ERROR("snprintf failed for nitzTime\n");
                return LE_FAULT;
            }
            LE_DEBUG("Old: %s\n", nitzTimeStr.c_str());
            LE_INFO("New: %s\n", timeSrcRefPrt->dateTimeInf.nitzTime);
        }
    }
    else
    {
        LE_ERROR("Parameter not initialized\n");
        return LE_BAD_PARAMETER;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the reference of a time source, create a new one if not exist.
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeRef_t taf_Time::GetTimeRef
(
    taf_time_TimeSources_t sourceId  ///< Time source ID.
)
{
     if (sourceId == TAF_TIME_SRC_NAME_EX_APP)
    {
        LE_ERROR("Not supported time source ID.");
        return NULL;
    }

    if (!TimeSourceConf.IsSourceExist(SourceNameIndexToStr(sourceId)) &&
        sourceId != TAF_TIME_SRC_NAME_SYSTEM)
    {
        LE_ERROR("Given time source is not registered in JSON file.");
        return NULL;
    }

    le_msg_SessionRef_t sessionRef = taf_time_GetClientSessionRef();

    taf_TimeInf_t* srcTimePtr =
        (taf_TimeInf_t*)SearchSourceInfList(sourceId, sessionRef, false);
    // Create a source object if it doesn't exist in the list.
    if (srcTimePtr == NULL )
    {
        srcTimePtr = (taf_TimeInf_t*)le_mem_ForceAlloc(TimePool);
        memset(srcTimePtr, 0, sizeof(taf_TimeInf_t));

        srcTimePtr->sourceId = sourceId;
        srcTimePtr->sessionRef = sessionRef;
        srcTimePtr->ref = (taf_time_TimeRef_t)le_ref_CreateRef(TimeRefMap, srcTimePtr);
    }

    LE_INFO("timeSrcRef %p, client session %p, sourceId 0x%x, name %s",
                srcTimePtr->ref, sessionRef, sourceId, SourceNameIndexToStr(sourceId));
    return srcTimePtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the time source and fill related data to the reference object.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    le_result_t result;
    taf_TimeInf_t* srcTimePtr =
        (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, timeSrcRef);
    if (srcTimePtr == NULL)
    {
        LE_ERROR("Rerence object was not found, please create a object first.");
        return LE_BAD_PARAMETER;
    }

    result = UpdateRefTimeInfo(srcTimePtr, timeValPtr);
    if (result != LE_OK)
    {
        LE_ERROR("Update time for %s time source failed\n",
                              SourceNameIndexToStr(srcTimePtr->sourceId));
        return result;
    }

    if (srcTimePtr->sourceId == TAF_TIME_SRC_NAME_RTC)
    {
        // Here the RTC is a sync API
        result = GetRtcTime(timeValPtr, true);
    }
    else if(srcTimePtr->sourceId == TAF_TIME_SRC_NAME_SYSTEM)
    {
        // Get the system time
        result = GetSystemTime(timeValPtr);
        srcTimePtr->dateTimeInf.referRealTime.sec = timeValPtr->sec;
        srcTimePtr->dateTimeInf.referRealTime.nanosec = timeValPtr->nanosec;
    }
    else
    {
        result = CheckSourceTime(timeValPtr, srcTimePtr->sourceId);
    }
    if (result != LE_OK)
    {
        LE_ERROR("Get %s time source failed\n", SourceNameIndexToStr(srcTimePtr->sourceId));
        return result;
    }
    srcTimePtr->dateTimeInf.sourceUtcTime.sec = timeValPtr->sec;
    srcTimePtr->dateTimeInf.sourceUtcTime.nanosec = timeValPtr->nanosec;

    if (srcTimePtr->sourceId == TAF_TIME_SRC_NAME_NETWORK
        || srcTimePtr->sourceId == TAF_TIME_SRC_NAME_NETWORK2)
    {
        result = UpdateDateTimeInfo(srcTimePtr, timeValPtr);
        if (result != LE_OK)
        {
            LE_ERROR("Get %s time source failed\n", SourceNameIndexToStr(srcTimePtr->sourceId));
            return result;
        }
    }

    taf_SourceInf_t* srcValidityTimePtr =
        (taf_SourceInf_t*)SearchSourceMap(srcTimePtr->sourceId);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, LE_NOT_FOUND, "Source Reference is not found!");
    TAF_ERROR_IF_RET_VAL(srcValidityTimePtr == NULL, LE_FAULT, "srcValidityTimePtr is NULL!");
    srcTimePtr->dateTimeInf.sourceValidity = srcValidityTimePtr->sourceValidity;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get reference system time when related time source was created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetRefSystemTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    // Find the reference object in the list.
    taf_TimeInf_t* srcTimePtr =
        (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, timeSrcRef);
    if (srcTimePtr == NULL)
    {
        LE_ERROR("srcTimePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Check if the system time is available.
    if (srcTimePtr->dateTimeInf.referRealTime.sec == 0)
    {
        LE_ERROR("The reference ptp time of %s is not available",
                                              SourceNameIndexToStr(srcTimePtr->sourceId));
        return LE_UNAVAILABLE;
    }

    timeValPtr->sec = srcTimePtr->dateTimeInf.referRealTime.sec;
    timeValPtr->nanosec = srcTimePtr->dateTimeInf.referRealTime.nanosec;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get reference gptp time when related time source was created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetRefGptpTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    // Find the reference object in the list.
    taf_TimeInf_t* srcTimePtr =
        (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, timeSrcRef);
    if (srcTimePtr == NULL)
    {
        LE_ERROR("srcTimePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Check if the system time is available.
    if (srcTimePtr->dateTimeInf.referPtpTime.sec == 0)
    {
        LE_ERROR("The reference ptp time of %s is not available",
                                              SourceNameIndexToStr(srcTimePtr->sourceId));
        return LE_UNAVAILABLE;
    }

    timeValPtr->sec = srcTimePtr->dateTimeInf.referPtpTime.sec;
    timeValPtr->nanosec = srcTimePtr->dateTimeInf.referPtpTime.nanosec;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases a time source reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ReleaseTimeRef
(
    taf_time_TimeRef_t timeSrcRef
)
{
    taf_TimeInf_t* srcTimePtr =
        (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, timeSrcRef);
    if (srcTimePtr == NULL)
    {
        LE_ERROR("srcTimePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Removed timeSrcRef(%p).", timeSrcRef);

    le_ref_DeleteRef(TimeRefMap, timeSrcRef);
    le_mem_Release(srcTimePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add reference time handler for a given time source.
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeValueChangeHandlerRef_t taf_Time::AddTimeValueChangeHandler
(
    taf_time_TimeSources_t sourceId,
    taf_time_TimeValueChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_msg_SessionRef_t sessionRef = taf_time_GetClientSessionRef();
    if ( !(sourceId >= TAF_TIME_SRC_NAME_RTC && sourceId < TAF_TIME_SRC_NAME_UNKNOWN)
        || (sourceId == TAF_TIME_SRC_NAME_EX_APP))
    {
        LE_ERROR("Not supported time source ID.");
        return NULL;
    }

    taf_TimeInf_t* srsPtr =
                        SearchSourceInfList(sourceId, sessionRef, true);
    // Only one handler is allowed for every session.
    if (srsPtr != NULL && srsPtr->handlerRef != NULL)
    {
        LE_ERROR("Handler for source(0x%x) is already registered.", srsPtr->sourceId);
        return NULL;
    }

    // Create and set the Time Source Reference Handler.
    taf_TimeInf_t* tsrHandlerPtr =
                        (taf_TimeInf_t*)le_mem_ForceAlloc(TimePool);
    memset(tsrHandlerPtr, 0, sizeof(taf_TimeInf_t));

    // Init the fields.
    tsrHandlerPtr->sourceId = sourceId;
    tsrHandlerPtr->func = handlerPtr;
    tsrHandlerPtr->context = contextPtr;
    tsrHandlerPtr->sessionRef = sessionRef;
    // Note, to share the same map 'TimeRefMap' for different sourceIds and handlers
    // the related type for handler and sourceId need to be 'actually' same.
    // So here need to covert it to 'taf_time_TimeValueChangeHandlerRef_t'.
    tsrHandlerPtr->handlerRef =
        (taf_time_TimeValueChangeHandlerRef_t)le_ref_CreateRef(TimeRefMap, tsrHandlerPtr);

    LE_INFO("Add func (%p), handlerRef(%p), sessionRef(%p), sourceId(0x%x)",
                  tsrHandlerPtr->func, tsrHandlerPtr->handlerRef,
                                        sessionRef, sourceId);
    return tsrHandlerPtr->handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the reference handler for a given time source.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::RemoveTimeValueChangeHandler
(
    taf_time_TimeValueChangeHandlerRef_t handlerRef
)
{
    taf_TimeInf_t* tsrHandlerPtr =
        (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, handlerRef);

    if (tsrHandlerPtr != NULL)
    {
        // Do sanity check.
        LE_ASSERT(tsrHandlerPtr->handlerRef == handlerRef);

        taf_TimeInf_t* srcTimePtr =
            (taf_TimeInf_t*)le_ref_Lookup(TimeRefMap, tsrHandlerPtr->handlerRef);

        if (srcTimePtr != NULL)
        {
            srcTimePtr->handlerRef = NULL;
        }

        LE_INFO("Removed tsrHandlerRef(%p).", handlerRef);

        // Free the handler.
        le_ref_DeleteRef(TimeRefMap, handlerRef);
        le_mem_Release(tsrHandlerPtr);
    }
    else
    {
        LE_ERROR("Invalid tsrHandlerRef(%p).", handlerRef);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process the registered client function.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::NotifyRefTimeClient
(
    TS_Event_t* tsEventPtr
)
{
    taf_TimeInf_t* srcTimePtr = NULL;
    void* eventRef = tsEventPtr->ref;

    TimeSourceRef_Event_t* tsrEventPtr =
        (TimeSourceRef_Event_t*)le_ref_Lookup(TsrEventMap, eventRef);

    if(tsrEventPtr != NULL)
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(TimeRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            //Send the notification to all the handlers for this same sourceId
            srcTimePtr = (taf_TimeInf_t*)le_ref_GetValue(iterRef);
            if ((srcTimePtr != NULL) && (srcTimePtr->sourceId == tsrEventPtr->sourceId)
                && (srcTimePtr->handlerRef != NULL)
                && (srcTimePtr->func != NULL))
            {
                // Copy the information from notification to data structure
                memcpy(&(srcTimePtr->dateTimeInf), &(tsrEventPtr->dateTimeInf),
                                                       sizeof(taf_DateTimeInf_t));
                // Note, the type for handler and sourceId are actually same
                srcTimePtr->func((taf_time_TimeRef_t)srcTimePtr->handlerRef,
                      &(srcTimePtr->dateTimeInf.sourceUtcTime), srcTimePtr->context);

                LE_DEBUG("Run func %p, handlerRef %p, eventRef %p, sourceId(0x%x).",
                                            srcTimePtr->func, srcTimePtr->handlerRef,
                                                    eventRef, srcTimePtr->sourceId);
            }
        }
    }

    le_ref_DeleteRef(TsrEventMap, eventRef);
    le_mem_Release(tsrEventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Update reference time for current handler event.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::CreateRefTimeForHandler
(
    TimeSourceRef_Event_t* tsrEventPrt,
    taf_time_TimeSpec_t timeVal,
    telux::tel::NetworkTimeInfo* info
)
{
    le_result_t result;
    taf_time_TimeSpec_t sysTime;
    timespec gPtpimeVal;

    if (info != NULL)
    {
        tsrEventPrt->dateTimeInf.dayOfWeek = info->dayOfWeek;
        tsrEventPrt->dateTimeInf.timeZone = info->timeZone;
        tsrEventPrt->dateTimeInf.dstAdj = info->dstAdj;
        le_utf8_Copy(tsrEventPrt->dateTimeInf.nitzTime, info->nitzTime.c_str(),
                                                   NITZ_STR_BUF_MAX, NULL);
    }

    tsrEventPrt->dateTimeInf.sourceValidity = true;
    tsrEventPrt->dateTimeInf.sourceUtcTime.sec = timeVal.sec;
    tsrEventPrt->dateTimeInf.sourceUtcTime.nanosec = timeVal.nanosec;

    result = GetSystemTime(&sysTime);
    if (result != LE_OK)
    {
        LE_ERROR("Get reference system time for handler failed");
        return result;
    }
    tsrEventPrt->dateTimeInf.referRealTime.sec = sysTime.sec;
    tsrEventPrt->dateTimeInf.referRealTime.nanosec = sysTime.nanosec;

    if(gptpTimeRef != NULL)
    {
        result = taf_gptpTime_GetTimeValue(gptpTimeRef, &gPtpimeVal);

        if (result != LE_OK)
        {
            LE_WARN("Get reference gptp time for handler failed");
        }
        else
        {
            tsrEventPrt->dateTimeInf.referPtpTime.sec = gPtpimeVal.tv_sec;
            tsrEventPrt->dateTimeInf.referPtpTime.nanosec = gPtpimeVal.tv_nsec;

            LE_DEBUG("Reference ptp time is %" PRIu64 ".%" PRIu64 "",
            tsrEventPrt->dateTimeInf.referPtpTime.sec,
            tsrEventPrt->dateTimeInf.referPtpTime.nanosec);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event handler for the time source which has reference time object.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::EventTimeValueChangeHandler
(
    void* reportPtr
)
{
    TS_Event_t* tsEvent = (TS_Event_t*)reportPtr;
    auto &tafTime = taf_Time::GetInstance();

    if (reportPtr != NULL)
    {
        tafTime.NotifyRefTimeClient(tsEvent);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the notification data to an object and report it to the handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::ReportTimeValueChange
(
    taf_time_TimeSources_t sourceId,
    taf_time_TimeSpec_t timeVal,
    telux::tel::NetworkTimeInfo* info
)
{
    le_result_t result;
    taf_TimeInf_t* srcTimePtr = NULL;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(TimeRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        srcTimePtr = (taf_TimeInf_t*)le_ref_GetValue(iterRef);
        if ((srcTimePtr != NULL) && (srcTimePtr->sourceId == sourceId)
            && (srcTimePtr->handlerRef != NULL))
        {
            LE_DEBUG(" (%p) for service(0x%x) is created.",
                                        srcTimePtr->handlerRef, sourceId);
            break;
        }
    }

    if (srcTimePtr == NULL || srcTimePtr->handlerRef == NULL)
    {
        return;
    }

    // Create memory object to save the data from notification.
    TimeSourceRef_Event_t* tsrEventPrt =
                 (TimeSourceRef_Event_t*)le_mem_ForceAlloc(TsrEventPool);
    memset(tsrEventPrt, 0, sizeof(TimeSourceRef_Event_t));

    tsrEventPrt->sourceId = sourceId;
    result = CreateRefTimeForHandler(tsrEventPrt, timeVal, info);
    if (result != LE_OK)
    {
        LE_ERROR("Update %s handler ref time failed\n",
                                         SourceNameIndexToStr(sourceId));
        return;
    }
    // Create a reference for this notification.
    tsrEventPrt->ref = le_ref_CreateRef(TsrEventMap, tsrEventPrt);

    LE_DEBUG("Notification tsrEventPrt->ref(%p), Source(0x%x).",
                               tsrEventPrt->ref, tsrEventPrt->sourceId);
    TS_Event_t tsEvent;
    tsEvent.sourceId = sourceId;
    tsEvent.ref = tsrEventPrt->ref;

    // Report to the notification handler.
    le_event_Report(RefTimeEventId, &tsEvent, sizeof(TS_Event_t));

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Compare the delta time of system time and new time with threshold.
 *
 * @return
 *     - true          -- Update system time is needed.
 *     - false         -- Not need to update system time.
 */
//--------------------------------------------------------------------------------------------------
bool taf_Time::IsThresholdSetTimeAllow
(
    taf_time_TimeSpec_t newTimeVal,
    taf_time_TimeSpec_t oldTime,
    taf_time_TimeSources_t timeSource
)
{
    uint64_t deltaMilliSec, milliSecThreshold;
    taf_time_TimeSpec_t tmpTime;

    if (TimeGreaterThan(newTimeVal, oldTime))
    {
        tmpTime = taf_time_Sub(newTimeVal, oldTime);
    }
    else
    {
        tmpTime = taf_time_Sub(oldTime, newTimeVal);
    }

    int position = TimeSourceConf.findSourcePosition(SourceNameIndexToStr(timeSource));

    if (position < 0)
    {
        LE_ERROR("Source does not exist!");
        return false;
    }

    if (TimeSourceConf.source[position].toleranceMillsec <= 0)
    {
        milliSecThreshold = TAF_TIME_THRESHOLD_MILLISEC;
    }
    else
    {
        milliSecThreshold = TimeSourceConf.source[position].toleranceMillsec;
    }

    deltaMilliSec = tmpTime.sec * 1000 + tmpTime.nanosec/1000/1000;
    if (deltaMilliSec < milliSecThreshold)
    {
        LE_DEBUG("Set time not need. delta %" PRIu64 ", threshold: %" PRIu64 ", SRC: %s",
            deltaMilliSec, milliSecThreshold, SourceNameIndexToStr(timeSource));
        return false;
    }

    return true;
}

#define DELTA_THRESHOLD_MS 1000
le_result_t taf_Time::ReadDeltaTimeFromStorage
(
    int64_t* deltaTimeMSec
)
{
    int fd = open(TAF_TIME_DELTA_TIME_PATH, O_RDONLY);
    if (fd == -1) {
        LE_ERROR("Open file: %s failed, %s", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
        *deltaTimeMSec = 0;
        return LE_FAULT;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        LE_ERROR("lseek failed: %s", strerror(errno));
        *deltaTimeMSec = 0;
        close(fd);
        return LE_FAULT;
    }

    ssize_t bytesRead = read(fd, deltaTimeMSec, sizeof(int64_t));
    if (bytesRead != sizeof(int64_t)) {
        LE_ERROR("Read file %s failed, bytes read: %zd", TAF_TIME_DELTA_TIME_PATH, bytesRead);
        *deltaTimeMSec = 0;
        close(fd);
        return LE_FAULT;
    }

    close(fd);
    return LE_OK;
}

le_result_t taf_Time::UpdateDeltaTimeToStorage
(
    taf_time_TimeSpec_t timeVal
)
{
    taf_time_TimeSpec_t rtcTimeVal;
    le_result_t result = GetInternalRtcTime(&rtcTimeVal);
    if (result != LE_OK)
    {
        LE_ERROR("Read RTC failed %d", result);
        return result;
    }

    int64_t oldDeltaMSec = 0;
    if (ReadDeltaTimeFromStorage(&oldDeltaMSec) != LE_OK)
    {
        oldDeltaMSec = 0;
        LE_WARN("Read delta time from storage failed");
    }

    int64_t newDelta_Msec = (timeVal.sec * 1000LL + timeVal.nanosec / 1000000LL) -
                            (rtcTimeVal.sec * 1000LL + rtcTimeVal.nanosec / 1000000LL);

    int64_t diff = newDelta_Msec - oldDeltaMSec;
    if ((diff < 0 ? -diff : diff) > DELTA_THRESHOLD_MS)
    {
        int fd = open(TAF_TIME_DELTA_TIME_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1)
        {
            LE_ERROR("Open file: %s failed, %s", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
            return LE_FAULT;
        }
        if (lseek(fd, 0, SEEK_SET) == -1) {
            LE_ERROR("lseek failed: %s", strerror(errno));
            close(fd);
            return LE_FAULT;
        }

        ssize_t written = write(fd, &newDelta_Msec, sizeof(int64_t));
        if (written != sizeof(int64_t))
        {
            LE_ERROR("Write file: %s failed, %s", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
            close(fd);
            return LE_FAULT;
        }

        fsync(fd);
        close(fd);
        LE_INFO("Successfully updated dlt_sec to %" PRId64 " ms", newDelta_Msec);
    }

    LE_DEBUG("RTC sec %" PRIu64 ", System sec %" PRIu64 ", oldDlt sec %" PRId64 ", newDlt sec %"
              PRId64, rtcTimeVal.sec, timeVal.sec, oldDeltaMSec / 1000, newDelta_Msec / 1000);

    return LE_OK;
}

bool isSecLableCreated(taf_time_TimeSources_t sourceId)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    le_result_t  res = taf_mngdStorSecData_CreateData(tafTime.SourceNameIndexToStr(sourceId));
    if(res == LE_OK || res == LE_DUPLICATE)
    {
        return true;
    }
    return false;
}

bool isSecStorageConnected(void)
{
    if (MssConnectStatusMainThread == LE_OK)
    {
        return true;
    }

    MssConnectStatusMainThread = taf_mngdStorSecData_TryConnectService();
    if(MssConnectStatusMainThread == LE_OK)
    {
        LE_INFO("Successfully connected to secure storage service");
        return true;
    }
    return false;
}

void taf_Time::UpdateSystemTimeRefInfo
(
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSources_t timeSource
)
{
    // System time source get changed should notify the client if any.
    if (LatestTimeSourceInfo->systemSourceId != timeSource)
    {
        LE_INFO("Switched time source from %s to %s",
        SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId),SourceNameIndexToStr(timeSource));

        TimeSourceChangeNotify(LatestTimeSourceInfo->systemSourceId, timeSource);
        LatestTimeSourceInfo->systemSourceId = timeSource;

        //Current time source was successfully set to system. Re-set the Override number to allow
        //lower priority time source to set system time when 'AllowOverrideAfterFail' drop to '0'.
        AllowOverrideAfterFail = TimeSourceConf.allowOverrideAfterFail;
    }
    LatestTimeSourceInfo->failedLoops = 0;
    LatestTimeSourceInfo->isAvailable = true;

    if((access(TAF_TIME_DELTA_TIME_DIR, F_OK) != -1))
    {
        UpdateDeltaTimeToStorage(timeVal);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set system 'CLOCK_REAL' time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_NOT_PERMITTED -- This service does not have permission.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetRealTime
(
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSources_t timeSource
)
{
    taf_time_TimeSpec_t systemTime;
    struct timespec newTime;

    le_result_t result = GetSystemTime(&systemTime);
    if (result != LE_OK)
    {
        LE_ERROR("Get system time failed");
        return result;
    }

    if (IsThresholdSetTimeAllow(timeVal, systemTime, timeSource))
    {
        newTime.tv_sec = timeVal.sec;
        newTime.tv_nsec = timeVal.nanosec;

        if (clock_settime(CLOCK_REALTIME, &newTime) < 0)
        {
            LE_ERROR("Update sys time to:  "
            "%lld.%ld, from:%" PRIu64 ".%" PRIu64 ". SRC: %s",
                (long long)newTime.tv_sec, newTime.tv_nsec,
                systemTime.sec, systemTime.nanosec, SourceNameIndexToStr(timeSource));

            switch (errno)
            {
                case EPERM:
                    LE_ERROR("Setting CLOCK_REALTIME not permitted");
                    return LE_NOT_PERMITTED;

                case EINVAL:
                    LE_ERROR("Invalid parameter to set CLOCK_REALTIME");
                    return LE_BAD_PARAMETER;

                default:
                    LE_ERROR("Unable to set CLOCK_REALTIME (errno = %d)", errno);
                    return LE_FAULT;
            }
        }
        LE_INFO("Update sys time to:  "
                    "%lld.%ld, from:%" PRIu64 ".%" PRIu64 ". SRC: %s",
            (long long)newTime.tv_sec, newTime.tv_nsec, systemTime.sec, systemTime.nanosec,
                                                     SourceNameIndexToStr(timeSource));
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetSystemTime
(
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSources_t timeSource
)
{
    int position = 0;

    position = TimeSourceConf.findSourcePosition(SourceNameIndexToStr(timeSource));
    if (position < 0)
    {
        LE_ERROR("Position of %s was not found", SourceNameIndexToStr(timeSource));
        return LE_NOT_FOUND;
    }

    UpdateFailedLoops(timeSource, FAIL_LOOP_NUM_CLEAN);
    SourceStatusUpdate(LE_OK, timeSource);

    le_result_t result = SetRealTime(timeVal, timeSource);
    if (result != LE_OK)
    {
        return result;
    }

    if(TimeSourceConf.source[position].setTimeCounter > 0)
    {
        TimeSourceConf.source[position].setTimeCounter-- ;
        LE_DEBUG("setTimeCounter of %s drop to %ld", SourceNameIndexToStr(timeSource),
            TimeSourceConf.source[position].setTimeCounter);
    }

    if (timeSource == TAF_TIME_SRC_NAME_EX_APP)
    {
        SetTimeSt->externalSetTime = true;
    }

    UpdateSystemTimeRefInfo(timeVal, timeSource);

    return LE_OK;
}

le_result_t syncValidityToMSS(taf_SourceInf_t* destSrcPtr, taf_SourceInf_t* newSrcPtr)
{
    auto& tafTime = taf_Time::GetInstance();

    TAF_ERROR_IF_RET_VAL(newSrcPtr == NULL, LE_BAD_PARAMETER, "newSrcPtr is NULL");
    TAF_ERROR_IF_RET_VAL(destSrcPtr == NULL, LE_BAD_PARAMETER, "oldSrcPtr is NULL");

    if(!isSecStorageConnected())
    {
        LE_WARN("Secure storage was not connected");
        return LE_FAULT;
    }

    if(LE_OK !=
        tafTime.WriteValidtyToSecStorage(destSrcPtr, newSrcPtr->sourceValidity))
    {
        return LE_FAULT;
    }
    destSrcPtr->isSyncedWithStorage = true;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Update both validity and time from system to RTC in a specify logic:
 *
 *  Important: Please study below logic clearly before you touch reference code.
 *
 *  1. If system time's validity = false:
 *     Write system time's validity to MSS as RTC's validity, if succ set the system time to RTC,
 *     if failed print error log.
 *
 *  2. If system time's validity = true:
 *     Set the system time to RTC, if succ write the system time's validity to MSS as RTC's validity
 *
 *  With the above steps, we can prevent: "If a reboot accurs the system time read from RTC which is
 *  NOT valid, but was marked as valid."
 *
 *  Notes:
 *  The validity in RAM for RTC can be updated only when RTC time was set successfully, in this
 *  case, both the RTC time and validity read by client are matched.
 */
//--------------------------------------------------------------------------------------------------
void RtcTrustTimeUpdateHandler(void)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    taf_time_TimeSpec_t systemTime, rtcTime, bootTime;
    int64_t rtcMsec = 0;

    taf_SourceInf_t* rtcPtr = tafTime.SearchSourceMap(TAF_TIME_SRC_NAME_RTC);
    if(rtcPtr == NULL)
    {
        LE_ERROR("Can't find rtc reference pointer");
        return;
    }

    le_result_t result = tafTime.GetSystemTime(&systemTime);
    if (result != LE_OK)
    {
        LE_ERROR("Get system time failed");
        return;
    }

    if (LE_OK != tafTime.GetBootTime(&bootTime))
    {
        LE_ERROR("Get boot time failed");
        return;
    }

    rtcMsec = (bootTime.sec*1000LL) + (bootTime.nanosec/1000000LL) + tafTime.rtcDeltaMsec;
    rtcTime.sec = rtcMsec/1000LL;
    rtcTime.nanosec =  rtcMsec%1000LL*1000000LL;

    bool needSetTime = tafTime.IsThresholdSetTimeAllow(systemTime, rtcTime, TAF_TIME_SRC_NAME_RTC);

    LE_DEBUG("Validity of RTC in ram: %d, system: %d, setTime: %d, isSyncedWithStorage: %d",
                         rtcPtr->sourceValidity, LatestTimeSourceInfo->sourceValidity,
                                            needSetTime, rtcPtr->isSyncedWithStorage);

    if(rtcPtr->sourceValidity == LatestTimeSourceInfo->sourceValidity
        && rtcPtr->isSyncedWithStorage && !needSetTime)
    {
        // Nothing is new, stop here.
        return;
    }

    // Needs to update the new validity to MSS if they are not same.
    if (LatestTimeSourceInfo->sourceValidity != rtcPtr->sourceValidity)
    {
        rtcPtr->isSyncedWithStorage = false;
    }

    if(LatestTimeSourceInfo->sourceValidity) // System time's validity = true:
    {
       // Set the system time value to RTC, if succ write the system time's validity to MSS as RTC's
       // validity.
       //
       // Source validity is 'true', so use below steps:
       // Step1: Set time
       // Step2: Set validity to RAM
       // Step3: Set validity to MSS

        if (needSetTime)
        {
            if (LE_OK !=
                tafTime.SetRtcTimeReqAsync(&systemTime, NULL, &tafTime.setRtcTrustTimeRespCB, NULL))
            {
                LE_WARN("Set %s time for RTC failed", tafTime.SourceNameIndexToStr(rtcPtr->sourceId));
                return;
            }
        }
        else
        {
            // The time is under threshold, not needs to update, CB function "setRtcTrustTimeRespCB"
            // won't be triggered, needs to call it here to handle validity status.
            tafTime.setRtcTrustTimeRespCB(LE_OK);
        }
    }
    else// System time's validity = false:
    {
       // Write system time's validity to MSS as RTC's validity, if succ set the system
       // time value to RTC, if failed print error log.
       //
       // Source validity is 'false', so use below steps:
       // Step1: Set validity to MSS
       // Step2: Set time
       // Step3: Set validity to RAM

        if (rtcPtr->isSyncedWithStorage == false)
        {
            if(LE_OK != syncValidityToMSS(rtcPtr, LatestTimeSourceInfo))
            {
                LE_WARN("Secure storage was not connected");
                return;
            }
        }

        if (needSetTime)
        {
            if (LE_OK !=
                tafTime.SetRtcTimeReqAsync(&systemTime, NULL, &tafTime.setRtcTrustTimeRespCB, NULL))
            {
                LE_WARN("Set time for RTC failed");
                return;
            }
        }
        else
        {
            // The time is under threshold, not needs to update, CB function "setRtcTrustTimeRespCB"
            // won't be triggered, needs to call it here to handle validity status.
            tafTime.setRtcTrustTimeRespCB(LE_OK);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Update the time source's validity in ram, and assign it's validity to system if the system is
 *  using this time source.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UpdateValidityInRam
(
    taf_time_TimeSources_t sourceId,
    bool newValidity
)
{
    auto &tafTime = taf_Time::GetInstance();

    taf_SourceInf_t* sourcePtr = tafTime.SearchSourceMap(sourceId);
    if(sourcePtr == NULL)
    {
        LE_ERROR("sourcePtr is NULL");
        return LE_NOT_FOUND;
    }

    // Handle sourceId's validity
    if (sourcePtr->sourceValidity != newValidity)
    {
        sourcePtr->sourceValidity = newValidity;
        tafTime.ReportValidityChange(sourcePtr);
    }

    // Handle system time's validity
    if (LatestTimeSourceInfo->systemSourceId == sourceId &&
        LatestTimeSourceInfo->sourceValidity != newValidity)
    {
        LatestTimeSourceInfo->sourceValidity = newValidity;
        tafTime.ReportValidityChange(LatestTimeSourceInfo);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Updates both the time and the validity for system. And then trigger an event to update this
 *  information for RTC with below logic:
 *
 *  Important: Please study below logic clearly before you touch reference code.
 *
 *  1. If source's validity = false:
 *  1st step update system time & set system time's validity in RAM.  2nd step try to write system
 *  time's validity to MSS as RTC's validity. If write succ set RTC time. If failed print an error.
 *  This logic covers the following use cases:
 *    a. Device restart occurs before 2nd step -> the new validity and the RTC time couldn't be
 *       updated, so the system will start with the previous time and validity read from RTC (
 *       before other higher priority time source is ready).
 *    b. Device occurs in the 2nd step after a successful write of RTC validity to MSS and
 *       before the RTC time is updated -> the validity of RTC was updated to false and RTC time
 *       couldn't be updated -> the system will start with the previous RTC time, but with validity
 *       false (in this case, the system time is not trust, will prevent secure APP use the wrong
 *       system time).
 *
 *  2. If source's validity = true:
 *  1st step update system time & set system time's validity in RAM. If system time successfully
 *  set, 2nd step set time for RTC and if OK then write validity to MSS for RTC.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UpdateTimeAndValidity
(
    taf_time_TimeSources_t sourceId,
    taf_time_TimeSpec_t timeVal,
    bool validity,
    bool external  ///< Indicates whether this was called by external (E.g client).
)
{
    auto &tafTime = taf_Time::GetInstance();
    le_result_t result = LE_FAULT;

    taf_SourceInf_t* sourcePtr = tafTime.SearchSourceMap(sourceId);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_NOT_FOUND, "sourcePtr is null.");

    result = tafTime.SetSystemTime(timeVal, sourceId);
    if (result != LE_OK)
    {
        return result;
    }

    LE_DEBUG("Validity of %s curr %d, new %d, system: %d",
       tafTime.SourceNameIndexToStr(sourcePtr->sourceId), sourcePtr->sourceValidity,
                                              validity, LatestTimeSourceInfo->sourceValidity);

    result = UpdateValidityInRam(sourceId, validity);
    if (result != LE_OK)
    {
        return result;
    }

    if (external)
    {
        // If the input was from 'external', that means a new status is comming from external,
        // otherwise the status is from RAM (old data). Since the validity already successfully
        // updated, set the "isSyncedWithSetCmd" to 'true'.
        sourcePtr->isSyncedWithSetCmd = true;
    }

    if (sourceId != TAF_TIME_SRC_NAME_RTC
        && RtcAsyncBaseDataIntStatus == LE_OK)
    {
        le_event_QueueFunctionToThread(tafTime.mainThreadRef,
        (le_event_DeferredFunc_t)RtcTrustTimeUpdateHandler, NULL, NULL);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the system time and validity if the client has the permission.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetTrustTime
(
    taf_time_SourceRef_t sourceRef,
    taf_time_TimeSpec_t timeVal,
    bool validity
)
{
    taf_SourceInf_t* sourcePtr
        = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Source Reference is null");
    if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_EX_APP)
    {
        LE_ERROR("This API doesn't support this time source ID: '%s'",
                                       SourceNameIndexToStr(sourcePtr->sourceId));
        return LE_BAD_PARAMETER;
    }

    if (timeVal.sec <= 0)
    {
        LE_ERROR("Invalid time value: %" PRIu64 "", timeVal.sec);
        return LE_BAD_PARAMETER;
    }

    le_result_t isClientValid = CheckSetValidityPermission();
    if(isClientValid != LE_OK)
    {
        LE_ERROR("Client is not allowed to change source validity.");
        return LE_NOT_PERMITTED;
    }

    if(!isNewTimeSrcSetTimeAllowed(sourcePtr->sourceId))
    {
        LE_ERROR("Set time is not allowed");
        return LE_NOT_PERMITTED;
    }

    return UpdateTimeAndValidity(sourcePtr->sourceId, timeVal, validity, true);
}

le_result_t taf_Time::UpdateSystemTime
(
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSources_t timeSource
)
{
    taf_SourceInf_t* sourcePtr = SearchSourceMap(timeSource);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_NOT_FOUND, "sourcePtr is null.");

    return UpdateTimeAndValidity(timeSource, timeVal, sourcePtr->sourceValidity, false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read time from specify time source.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_TIMEOUT -- Time out occurred.
 *     - LE_UNAVAILABLE -- If no data available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::CheckSourceTime
(
    taf_time_TimeSpec_t* timePtr,
    taf_time_TimeSources_t sourceIndex
)
{
    le_result_t result;
    timePtr->sec = 0;
    timePtr->nanosec = 0;

    switch (sourceIndex)
    {
        case TAF_TIME_SRC_NAME_RTC:
            result = GetRtcTime(timePtr, false);
            break;

        case TAF_TIME_SRC_NAME_GNSS:
            result = GetGnssTime(timePtr);
            break;

        case TAF_TIME_SRC_NAME_EX_APP:
            result = GetExSetTimeStatus();
            break;

        case TAF_TIME_SRC_NAME_NETWORK:
            result = GetNetworkTime(timePtr, TAF_TIME_SRC_NAME_NETWORK);
            break;

        case TAF_TIME_SRC_NAME_NETWORK2:
            result = GetNetworkTime(timePtr, TAF_TIME_SRC_NAME_NETWORK2);
            break;

        default:
            result = LE_NOT_FOUND;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process event reports when the running time source got change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::TimeSourceChangeNotify
(
    taf_time_TimeSources_t PreTimeSource,
    taf_time_TimeSources_t NewTimeSource
)
{
    if (NewTimeSource == PreTimeSource &&
        NewTimeSource == TAF_TIME_SRC_NAME_UNKNOWN)
    {
        LE_DEBUG("TimeSourceChange, no active time source\n");
    }
    else
    {
        LE_DEBUG("TimeSourceChange Old: %s, New: %s\n",
                SourceNameIndexToStr(PreTimeSource), SourceNameIndexToStr(NewTimeSource));
    }

    taf_TimeSourceStatus_t* statusPtr =
       (taf_TimeSourceStatus_t*)le_mem_ForceAlloc(timeSourceChangePool);
    statusPtr->preSource = PreTimeSource;
    statusPtr->newSource = NewTimeSource;
    le_event_ReportWithRefCounting(timeSourceChangeId, (void*)statusPtr);
}

void taf_Time::SourceStatusUpdate(le_result_t result, taf_time_TimeSources_t sourceIndex)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    bool previousAvailablility = (tafTime.PrevSrcAvailabiltyMap >> sourceIndex) & 1;
    taf_SourceInf_t* sourcePtr = tafTime.SearchSourceMap(sourceIndex);
    TAF_ERROR_IF_RET_NIL(sourcePtr == NULL, "Source reference not found");
    bool oldValidity = sourcePtr->sourceValidity;

      if (result == LE_OK)
      {
          sourcePtr->isAvailable = true;
          tafTime.PrevSrcAvailabiltyMap = tafTime.PrevSrcAvailabiltyMap | (1 << sourceIndex);
          if
          (
              sourcePtr->sourceId != TAF_TIME_SRC_NAME_RTC &&
              sourcePtr->sourceId != TAF_TIME_SRC_NAME_EX_APP
          )
          {
              sourcePtr->sourceValidity = true;
          }
      }
      else
      {
          sourcePtr->isAvailable = false;
          tafTime.PrevSrcAvailabiltyMap = tafTime.PrevSrcAvailabiltyMap & (~(1 << sourceIndex));
          if
          (
              sourcePtr->sourceId != TAF_TIME_SRC_NAME_RTC &&
              sourcePtr->sourceId != TAF_TIME_SRC_NAME_EX_APP
          )
          {
              sourcePtr->sourceValidity = false;
          }
      }

      if (previousAvailablility != sourcePtr->isAvailable)
      {
        SourceStatusChange_Event_t evt;
        evt.sourcePtr = sourcePtr;
        evt.status = sourcePtr->isAvailable;
        evt.eventType = TAF_TIME_STATUS_EVENT_AVAILABILITY;
        le_event_Report(timeSourceStatusEventId, &evt, sizeof(evt));
      }

      if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_RTC &&
        sourcePtr->sourceId != TAF_TIME_SRC_NAME_EX_APP)
      {
        // The ExAPP's validity was designed by "SetTrustTime" API.
        // The RTC's validity was designed by system time's time source.

          if(oldValidity != sourcePtr->sourceValidity)
          {
            ReportValidityChange(sourcePtr);
          }
      }
}

void taf_Time::ReportValidityChange(taf_SourceInf_t* sourcePtr)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    SourceStatusChange_Event_t evt;
    if(sourcePtr == NULL)
    {
        LE_ERROR("sourcePtr is NULL");
        return;
    }

    LE_INFO("Validity of %s change to %d",
    SourceNameIndexToStr(sourcePtr->sourceId), sourcePtr->sourceValidity);

    evt.sourcePtr = sourcePtr;
    evt.status = sourcePtr->sourceValidity;
    evt.eventType = TAF_TIME_STATUS_EVENT_VALIDITY;
    le_event_Report(tafTime.timeSourceStatusEventId, &evt, sizeof(evt));
}

void taf_Time::UpdateFailedLoops
(
    taf_time_TimeSources_t sourceIndex,
    taf_TimeFailLoopAction_t action
)
{
    taf_Time& tafTime = taf_Time::GetInstance();

    taf_SourceInf_t* sourcePtr = tafTime.SearchSourceMap(sourceIndex);
    if (sourcePtr == NULL)
    {
        LE_ERROR("SourceRefPtr not found\n");
        return;
    }

    if (action == FAIL_LOOP_NUM_CLEAN || sourcePtr->failedLoops == -1)
    {
        sourcePtr->failedLoops = 0;
    }

    if (action == FAIL_LOOP_NUM_INCREASE)
    {
        sourcePtr->failedLoops++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Process the set time action according to the JSON configuration.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_TERMINATED -- Eixt initialization set time run loop.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_NOT_FOUND -- Time source not available.
 *     - LE_NOT_IMPLEMENTED -- Feature not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetTimeBaseOnConfig
(
    TimeSources serviceCfg,
    uint64_t* timeSrcStatusMap
)
{
    le_result_t result = LE_UNAVAILABLE;
    uint64_t TimeSourceStatusMap = 0x0;
    taf_time_TimeSources_t sourceIndex;
    taf_time_TimeSpec_t time;
    int i;

    // Flag "setStatus" is used to prevent the system time being overwrite by low priority
    // time source, the priority of the time source in vector "source" is: 0 > 1 > 2 ...
    bool setStatus = false;
    if (serviceCfg.source.empty())
    {
        LE_ERROR("Time source configuration not found\n");
        return LE_BAD_PARAMETER;
    }
    // 1. Check the status for all supported time sources.
    // 2. Get the time from the first high priority and active time source.
    // 3. Set the time to system.
    for (i = 0; i < (int)serviceCfg.source.size(); i++)
    {
        sourceIndex = SourceNameStrToIndex(serviceCfg.source[i].sourceName.c_str());

        result = CheckSourceTime(&time, sourceIndex);

        if (result != LE_OK || time.sec <= 0)
        {
            UpdateFailedLoops(sourceIndex, FAIL_LOOP_NUM_INCREASE);
            LE_DEBUG("Get %s time source failed %d, sec: %" PRIu64 "",
                        serviceCfg.source[i].sourceName.c_str(), result, time.sec);
            continue;
        }
        // Set the bit map for the available time source
        TimeSourceStatusMap = TimeSourceStatusMap | (1 << sourceIndex);
        if (sourceIndex == TAF_TIME_SRC_NAME_EX_APP)
        {
            // ExAPP set time successful out of this function loop
            setStatus = true;
        }
        else if (!setStatus && isNewTimeSrcSetTimeAllowed(sourceIndex))
        {
            result = UpdateSystemTime(time, sourceIndex);
            if (result == LE_OK)
            {
                setStatus = true;
            }
        }

        LE_DEBUG("Tatol: %zu, latest: %s, current: %s, priority: %d,"
            " set allow: %d, status: %d, result: %d\n",
            serviceCfg.source.size(), SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId),
            serviceCfg.source[i].sourceName.c_str(), i,
            serviceCfg.source[i].setSystemTime, setStatus, result);
    }

    *timeSrcStatusMap = TimeSourceStatusMap;

    if (setStatus)
    {
        return LE_OK;
    }

    if (AllowOverrideAfterFail > 0)
    {
        AllowOverrideAfterFail--;
    }
    LatestTimeSourceInfo->failedLoops++;
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sync the time to the system according to the JSON configuration items. There may be many different
 * time sources with different priorities and different time out settings.
 *
 * NOTE: This run loop will NOT terminate if the time sources can be found in the JSON configuration.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_UNAVAILABLE -- Time source not available.
 */
//--------------------------------------------------------------------------------------------------
void SystemTimeUpdateTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    le_result_t result = LE_UNAVAILABLE;
    uint64_t timeSrcStatusMap = 0x0;

    result = tafTime.SetTimeBaseOnConfig(TimeSourceConf, &timeSrcStatusMap);
    if (result != LE_OK)
    {
        LE_DEBUG("Warning: Sync time failed, will try again after %ld seconds\n",
            TimeSourceConf.pollingInterval);
    }
    else
    {
        LE_DEBUG("Time sources status: 0x%08" PRIx64 "\n", timeSrcStatusMap);
    }

    if (timeSrcStatusMap == 0x0)
    {
        // No available time source
        tafTime.TimeSourceChangeNotify(TAF_TIME_SRC_NAME_UNKNOWN, TAF_TIME_SRC_NAME_UNKNOWN);
    }
}

void taf_Time::InitializeSystemTimeAttr(void)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    LatestTimeSourceInfo = (taf_SourceInf_t*)le_mem_ForceAlloc(tafTime.SrcPool);
    LatestTimeSourceInfo->failedLoops = -1;
    LatestTimeSourceInfo->sourceId = TAF_TIME_SRC_NAME_SYSTEM;
    LatestTimeSourceInfo->systemSourceId = TAF_TIME_SRC_NAME_UNKNOWN;
    LatestTimeSourceInfo->isAvailable = false;
    LatestTimeSourceInfo->sourceValidity = false;
    LatestTimeSourceInfo->handlerRef = NULL;
    LatestTimeSourceInfo->handlerFunc = NULL;
    LatestTimeSourceInfo->isBaseStruct = true;
    LatestTimeSourceInfo->sessionRef = taf_time_GetClientSessionRef();
    LatestTimeSourceInfo->ref =
        (taf_time_SourceRef_t)le_ref_CreateRef(tafTime.SrcRefMap, LatestTimeSourceInfo);
    LatestTimeSourceInfo->secStrgdataRef = nullptr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Advertising time service to clients.
 */
//--------------------------------------------------------------------------------------------------
void AdvertiseTimeService(void)
{
    taf_time_AdvertiseService();
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sync RTC trust time and system validity.
 *
 *  Important:
 *  1. When first connected with MSS and the system time source is RTC:
 *     If RTC's validity was not updated by any API, update RTC's validity and assign the RTC's
 *     validity to system time.
 *  2. When system time was updated by other time source (not RTC):
 *     Assign the currently system time and its validity pair for RTC.
 */
//--------------------------------------------------------------------------------------------------
void SyncRtcTrustInfoWithMSSHandler
(
    le_timer_Ref_t timerRef
)
{
    bool mssStoragedValidity = false;
    le_result_t result = LE_FAULT;
    uint32_t time = 0;

    auto &tafTime = taf_Time::GetInstance();
    if(!isSecStorageConnected())
    {
        return;
    }

    // Base on currently requirement, it only needs to store RTC's validity to MSS.
    taf_SourceInf_t* rtcSrcPtr = tafTime.SearchSourceMap(TAF_TIME_SRC_NAME_RTC);
    if(rtcSrcPtr == NULL)
    {
        LE_ERROR("Not found %s pointer", tafTime.SourceNameIndexToStr(TAF_TIME_SRC_NAME_RTC));

        if (tafTime.syncSecStorageRef)
        {
            le_timer_Stop(tafTime.syncSecStorageRef);
        }
        return;
    }

    if(! rtcSrcPtr->isSyncedWithStorage)
    {
        // If the validity was not set by cmd, then the one in MSS is the right value.
        result = tafTime.ReadValidityFromSecStorage(rtcSrcPtr, &mssStoragedValidity);
        if ( LE_OK == result)
        {
            // 1. Assign RTC's validity to system if the system time source is RTC.
            // 2. Update the RTC validity in ram if the system time source is unknown
            if (LatestTimeSourceInfo->systemSourceId == TAF_TIME_SRC_NAME_RTC
                || LatestTimeSourceInfo->systemSourceId == TAF_TIME_SRC_NAME_UNKNOWN)
            {
                result = UpdateValidityInRam(rtcSrcPtr->sourceId, mssStoragedValidity);
                if (LE_OK == result)
                {
                    rtcSrcPtr->isSyncedWithStorage = true;
                }
            }
            // Sync system time info to RTC if the system time was set by other time source.
            else
            {
                // Here needs to cover:
                // In case before the connection is ready an API "SetTrustTime" was call, the trust
                // info should NOT be synced to RTC. Now the connection is ready, try to sync both
                // the time and validity to RTC.
                RtcTrustTimeUpdateHandler();
            }
        }
    }

    if (timerRef)
    {
        time = le_timer_GetExpiryCount(timerRef);
    }
    LE_DEBUG("Sync MSS, tried: (%d-%d), synced: %d, result: %d",
                           INIT_SYNC_VALIDI_WITH_MSS_COUNTER, (int)(time),
                             rtcSrcPtr->isSyncedWithStorage, (int)result);

    if (rtcSrcPtr->isSyncedWithStorage || LE_OK == result)
    {
        if (tafTime.syncSecStorageRef)
        {
            le_timer_Stop(tafTime.syncSecStorageRef);
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::InitAsyncRtcEvtHandler(void)
{
    RtcEvtHandlerId =
        le_event_CreateId("RtcEvtHandlerId", sizeof(RtcEvent_t));
    le_event_AddHandler("RtcEvtHandlerIdRef", RtcEvtHandlerId, RtcCbEventHandler);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Attempts to sync trust info between MSS, RTC and system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t InitAsyncRtcAndSystemTrustInfo(void)
{
    auto &tafTime = taf_Time::GetInstance();
    taf_SourceInf_t* rtcSrcPtr = tafTime.SearchSourceMap(TAF_TIME_SRC_NAME_RTC);
    TAF_ERROR_IF_RET_VAL(rtcSrcPtr == NULL, LE_BAD_PARAMETER, "rtcSrcPtr is NULL");

    if (RtcAsyncBaseDataIntStatus != LE_OK)
    {
        LE_WARN("Async RTC is not initialized");
        return LE_FAULT;
    }

    // Try to sync with MSS
    SyncRtcTrustInfoWithMSSHandler(NULL);

    // The MSS is often not ready on time; use a timer to retry.
    if (rtcSrcPtr->isSyncedWithStorage == false)
    {
        tafTime.syncSecStorageRef = le_timer_Create("syncSecStorageRef");
        le_timer_SetMsInterval(tafTime.syncSecStorageRef, 3*1000);
        le_timer_SetRepeat(tafTime.syncSecStorageRef, INIT_SYNC_VALIDI_WITH_MSS_COUNTER);
        le_timer_SetHandler(tafTime.syncSecStorageRef, SyncRtcTrustInfoWithMSSHandler);
        le_timer_SetWakeup(tafTime.syncSecStorageRef, false);
        le_timer_Start(tafTime.syncSecStorageRef);
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initialize all the basic time sources' information.
 */
//--------------------------------------------------------------------------------------------------
void InitTimeSource(void)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    tafTime.TimeRefMap = le_ref_CreateMap("TimeRefMap", TAF_TIME_SRC_NAME_UNKNOWN);
    tafTime.TimePool = le_mem_CreatePool("Time Pool", sizeof(taf_TimeInf_t));
    tafTime.SrcRefMap = le_ref_CreateMap("SrcRefMap", TAF_TIME_SRC_NAME_UNKNOWN + 1);
    tafTime.SrcPool = le_mem_CreatePool("Available Source Pool", sizeof(taf_SourceInf_t));
    tafTime.rtcDeltaMsec = 0;

    // 1. Initialize system time source attribute
    tafTime.InitializeSystemTimeAttr();

    // 2. Initialize ptp device
    tafTime.RegisterPtpDevice();

    if (TimeSourceConf.source.size() <= 0)
    {
        LE_WARN("No time source can be found");
        return;
    }

    // 3. Initialize none-system time source attribute
    std::vector<Source> SourceList = TimeSourceConf.source;
    for (auto item : SourceList)
    {
        taf_SourceInf_t* src = (taf_SourceInf_t*)le_mem_ForceAlloc(tafTime.SrcPool);
        src->sourceId = tafTime.SourceNameStrToIndex(item.sourceName.c_str());
        src->failedLoops = -1;
        src->systemSourceId = TAF_TIME_SRC_NAME_UNKNOWN;
        src->isAvailable = false;
        src->handlerRef = NULL;
        src->handlerFunc = NULL;
        src->sourceValidity = false;
        src->isSyncedWithStorage = false;
        src->isSyncedWithSetCmd = false;
        src->isBaseStruct = true;
        src->sessionRef = NULL;
        src->ref = (taf_time_SourceRef_t)le_ref_CreateRef(tafTime.SrcRefMap, src);
        src->secStrgdataRef = NULL;
    }

    // 4. Init GNSS time source base data.
    GnssBaseDataIntStatus = tafTime.InitGnssBaseData();

    // 5. Init Network time source base data.
    NetworkBaseDataIntStatus = tafTime.InitNetworkBaseData();

    // 6. Init RTC time source base data.
    RtcAsyncBaseDataIntStatus = tafTime.InitAsyncRtcBaseData();
}

void taf_Time::ReleasePtpDevice(void)
{
    auto &tafTime = taf_Time::GetInstance();

    if(tafTime.gptpTimeRef != NULL)
    {
        le_result_t res = taf_gptpTime_DeleteRef(tafTime.gptpTimeRef);
        if(res == LE_OK)
        {
            tafTime.gptpTimeRef = NULL;
        }
        else
        {
            LE_ERROR("Not able to delete gptp time reference");
        }
    }
}

void taf_Time::RegisterPtpDevice(void)
{
    auto &tafTime = taf_Time::GetInstance();
    LE_INFO("Creating gptpTimeRef");

    if(tafTime.gptpTimeRef == NULL)
    {
        tafTime.gptpTimeRef = taf_gptpTime_CreateRef(TimeSourceConf.gptpDeviceName.c_str());
        if(tafTime.gptpTimeRef == NULL)
        {
            LE_WARN("Create gptpTimeRef failed.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal handler for SIGTERM to clear up the resource.
 */
//--------------------------------------------------------------------------------------------------
static void TafSigTermEventHandler
(
    int sigNum
)
{
    auto &tafTime = taf_Time::GetInstance();

    LE_INFO("TafSigTermEventHandler :%d", sigNum);

    tafTime.ReleasePtpDevice();

    if (tafTime.syncTimeTimerRef)
    {
        le_timer_Stop(tafTime.syncTimeTimerRef);
    }

    if (tafTime.sysTimeUdTimerRef)
    {
        le_timer_Stop(tafTime.sysTimeUdTimerRef);
    }

    if (tafTime.syncSecStorageRef)
    {
        le_timer_Stop(tafTime.syncSecStorageRef);
    }

    if (InitNetworkManagerStatus == LE_OK)
    {
        InitNetworkManagerStatus = LE_UNAVAILABLE;
        tafTime.DeregNetworkTimeListener();
    }

    if (InitGnssManagerStatus == LE_OK)
    {
        InitGnssManagerStatus = LE_UNAVAILABLE;
        tafTime.DeregGnssTimeListener();
    }

    if(MssConnectStatusMainThread == LE_OK)
    {
        LE_INFO("Disconnecting from MSS");
        MssConnectStatusMainThread = LE_UNAVAILABLE;
        taf_mngdStorSecData_DisconnectService();
    }
}

void DeferSigTermToMainThread(void* param)
{
    int* sigNum = (int*) param;
    TafSigTermEventHandler(*sigNum);
}

static void TafSigTermEventHandlerForSyncTimeTh
(
    int sigNum
)
{
    auto &tafTime = taf_Time::GetInstance();
    tafTime.sigTermSignalNum = sigNum;
    LE_INFO("TafSigTermEventHandlerForSyncTimeTh :%d", tafTime.sigTermSignalNum);
    le_event_QueueFunctionToThread(tafTime.mainThreadRef,
        (le_event_DeferredFunc_t)DeferSigTermToMainThread,&tafTime.sigTermSignalNum, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Active time related tasks according to the JSON configuration.
 *
 * @return
 *     - NULL -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
void *taf_Time::SyncTimeTasks(void* contextPtr)
{
     // Setup signal's event handler.
     le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandlerForSyncTimeTh);

    le_result_t regGnssTimeStatus = LE_UNAVAILABLE;
    le_result_t regNetworkTimeStatus = LE_UNAVAILABLE;
    long int interval;
    taf_Time& tafTime = taf_Time::GetInstance();

    // The RTC time is critical for boot up time, try to sync it as quickly as possible.
    if (RtcAsyncBaseDataIntStatus == LE_OK)
    {
        tafTime.GetRtcTimeReqAsync(nullptr, NULL);
    }

    if (NetworkBaseDataIntStatus == LE_OK)
    {
        InitNetworkManagerStatus = tafTime.InitNetworkManager();
        if (InitNetworkManagerStatus == LE_OK)
        {
            regNetworkTimeStatus = tafTime.RegNetworkTimeListener();
            if (regNetworkTimeStatus != LE_OK)
            {
                LE_WARN("Warning: regNetworkTimeStatus failed\n");
            }
            else
            {
                tafTime.RequestNetworkTime();
            }
        }
    }
    if (GnssBaseDataIntStatus == LE_OK)
    {
        InitGnssManagerStatus = tafTime.InitGnssManager();

        if (InitGnssManagerStatus == LE_OK)
        {
            regGnssTimeStatus = tafTime.RegGnssTimeListener();
            if (regGnssTimeStatus != LE_OK)
            {
                LE_WARN("Warning: regGnssTimeListener failed\n");
            }
        }
    }
    if ( regNetworkTimeStatus == LE_OK
        || regGnssTimeStatus == LE_OK
       )
    {
        interval = TimeSourceConf.pollingInterval * 1000;
        if (interval > TAF_TIME_SYNC_TIME_TIMER_INTERVAL)
        {
            interval = interval - 2000; //Avoid starting this timer in same time with other
        }

        // Create timer to update the local GNSS time
        tafTime.syncTimeTimerRef = le_timer_Create("syncTimeTimer");
        le_timer_SetMsInterval(tafTime.syncTimeTimerRef, interval);
        le_timer_SetRepeat(tafTime.syncTimeTimerRef, 0);
        le_timer_SetHandler(tafTime.syncTimeTimerRef, SyncTimeTimerHandler);
        le_timer_SetWakeup(tafTime.syncTimeTimerRef, false);
        le_timer_Start(tafTime.syncTimeTimerRef);
    }

    le_event_QueueFunctionToThread(tafTime.mainThreadRef,
        (le_event_DeferredFunc_t)AdvertiseTimeService,NULL, NULL);
    le_event_RunLoop();

    LE_WARN("Warning: SyncTimeTasks exit!\n");
}

void StartSetTimeForSystem(void)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    if (TimeSourceConf.source.empty())
    {
        LE_WARN("No time source found");
        return;
    }

    if (TimeSourceConf.pollingInterval <= 0)
    {
        TimeSourceConf.pollingInterval = TAF_TIME_SECOND_PER_LOOP_DEFAULT;
    }
    // Update the system time as quickly as possible.
    SystemTimeUpdateTimerHandler(NULL);

    LE_INFO("Starting sync time timer, interval: %ld sec", TimeSourceConf.pollingInterval);
    tafTime.sysTimeUdTimerRef = le_timer_Create("sysTimeUpdateTimer");
    le_timer_SetMsInterval(tafTime.sysTimeUdTimerRef, (TimeSourceConf.pollingInterval)*1000);
    le_timer_SetRepeat(tafTime.sysTimeUdTimerRef, 0);
    le_timer_SetHandler(tafTime.sysTimeUdTimerRef, SystemTimeUpdateTimerHandler);
    le_timer_SetWakeup(tafTime.sysTimeUdTimerRef, false);
    le_timer_Start(tafTime.sysTimeUdTimerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for time source status change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::LayerTimeSourceChangeHandler
(
    void* reportPtr,          ///< [IN] Report pointer.
    void* layerHandlerFuncPtr ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_time_TimeSourceChangeHandlerFunc_t handlerFunc =
        (taf_time_TimeSourceChangeHandlerFunc_t)layerHandlerFuncPtr;
    if (handlerFunc)
    {
        taf_TimeSourceStatus_t* statusPtr = (taf_TimeSourceStatus_t*)reportPtr;
        handlerFunc(statusPtr->preSource, statusPtr->newSource, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register the GNSS time listener.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_UNAVAILABLE -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::RegGnssTimeListener(void)
{
    if (gnssTimeListener == nullptr)
    {
        gnssTimeListener = std::make_shared<taf_TimeGnssListener>();
        auto myStatus = timeManager->registerListener(gnssTimeListener, SupportTimeMask);
        if (myStatus != Status::SUCCESS)
        {
            LE_ERROR("Failed to register time listener\n");
            gnssTimeListener = nullptr;
            return LE_UNAVAILABLE;
        }
        LE_DEBUG("gnssTimeListener was started\n");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * De-Register the GNSS time listener.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::DeregGnssTimeListener(void)
{
    if (timeManager && gnssTimeListener != nullptr)
    {
        timeManager->deregisterListener(gnssTimeListener, SupportTimeMask);
        gnssTimeListener = nullptr;

        LE_DEBUG("GnssTimeListener was deregistered\n");
    }
}

void taf_Time::SyncTimeTimerHandler(le_timer_Ref_t timerRef)
{
    auto &tafTime = taf_Time::GetInstance();

    if (RtcAsyncBaseDataIntStatus == LE_OK)
    {
        tafTime.GetRtcTimeReqAsync(nullptr, NULL);
    }

    if (InitGnssManagerStatus == LE_OK)
    {
        tafTime.RegGnssTimeListener();
    }

    if (InitNetworkManagerStatus == LE_OK)
    {
        tafTime.RequestNetworkTime();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert the date time to seconds since epoch.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ConvertDateTimeToSec
(
    struct tm dateTime,
    taf_time_TimeSpec_t* timeValPtr
)
{
    int64_t secs = 0;
    secs = (int64_t )timegm(&dateTime);
    if (secs > 0)
    {
        timeValPtr->sec = secs;
        timeValPtr->nanosec = 0;
    }
    else
    {
        LE_WARN("Convert UTC time to seconds failed: %d, %s\n", errno, strerror(errno));
        LE_INFO("Year      : %d", dateTime.tm_year);
        LE_INFO("Month     : %d", dateTime.tm_mon);
        LE_INFO("Day       : %d", dateTime.tm_mday);
        LE_INFO("Hour      : %d", dateTime.tm_hour);
        LE_INFO("Minute    : %d", dateTime.tm_min);
        LE_INFO("Second    : %d", dateTime.tm_sec);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert the network time to seconds since epoch.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ConvertNetworkTimeToSec
(
    telux::tel::NetworkTimeInfo info,
    taf_time_TimeSpec_t* timeValPtr
)
{
    le_result_t result;
    struct tm dateTime;

    // Initialize date time
    dateTime.tm_year = info.year - 1900;  // Year - 1900
    dateTime.tm_mon = info.month - 1;     // Month (0-11, where 0 is January)
    dateTime.tm_mday = info.day;          // Day of the month (1-31)
    dateTime.tm_hour = info.hour;         // Hour    (0-23)
    dateTime.tm_min = info.minute;        // Minutes (0-59)
    dateTime.tm_sec = info.second;        // Seconds (0-61, including leap seconds)

    // Convert UTC time to seconds since epoch
    result = ConvertDateTimeToSec(dateTime, timeValPtr);
    if (result != LE_OK)
    {
        LE_ERROR("Convert network time failed\n");
        return result;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Store the date time information.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::StoreDateTimeInfo
(
    telux::tel::NetworkTimeInfo info,
    taf_time_TimeSources_t sourceId
)
{
    LE_DEBUG("sourceId %d, NITZ:%s\n", sourceId, info.nitzTime.c_str());
    taf_TimeNetTimeInfo_t* netInfoPtr = SearchNetTimeInfList(sourceId);
    // Create a source object if it doesn't exist in the list.
    if (netInfoPtr == NULL)
    {
        netInfoPtr = (taf_TimeNetTimeInfo_t*)le_mem_ForceAlloc(netTimeInfoPool);
        memset(netInfoPtr, 0, sizeof(taf_TimeNetTimeInfo_t));

        void* netTimeInfRef = le_ref_CreateRef(netTimeInfoRefMap, netInfoPtr);
        if (netTimeInfRef == NULL)
        {
            LE_ERROR("Create reference for netTimeInfoRefMap failed");
            return;
        }
    }
    netInfoPtr->sourceId = sourceId;
    netInfoPtr->timeInfo.dayOfWeek = info.dayOfWeek;
    netInfoPtr->timeInfo.dstAdj = info.dstAdj;
    netInfoPtr->timeInfo.timeZone = info.timeZone;
    le_utf8_Copy(netInfoPtr->timeInfo.nitzTime, info.nitzTime.c_str(),
                                                       NITZ_STR_BUF_MAX, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for synching network time.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::NetworkTimeResponseUpdate
(
    uint8_t phoneId,
    telux::tel::NetworkTimeInfo info, ///< [IN] Network time information.
    telux::common::ErrorCode error    ///< [IN] Error code.
)
{
    taf_time_TimeSpec_t timeVal = {0};
    taf_time_TimeSources_t sourceId;
    le_result_t result;
    static bool initFlag = true;
    auto &tafTime = taf_Time::GetInstance();
    if(phoneManager->getSlotIdFromPhoneId(phoneId) == 1)
    {
        sourceId = TAF_TIME_SRC_NAME_NETWORK;
    }
    else
    {
        sourceId = TAF_TIME_SRC_NAME_NETWORK2;
    }

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_DEBUG("Register network time for phone %d, Error(%d)", phoneId, (int)error);
        tafTime.SourceStatusUpdate(LE_FAULT, sourceId);
        return;
    }

    result = ConvertNetworkTimeToSec(info, &timeVal);
    if (LE_OK == result)
    {
        UpdateFailedLoops(sourceId, FAIL_LOOP_NUM_CLEAN);
        if(sourceId == TAF_TIME_SRC_NAME_NETWORK)
        {
            UpdateLocalTimeCache(timeVal, sourceId, NetworkDeltaTime);
        }
        else
        {
            UpdateLocalTimeCache(timeVal, sourceId, NetworkDeltaTime2);
        }
        tafTime.SourceStatusUpdate(result, sourceId);
    }

    StoreDateTimeInfo(info, sourceId);

    if (initFlag)
    {
        // Report a event to client for the first time initialization.
        initFlag = false;
        tafTime.ReportTimeValueChange(sourceId, timeVal, &info);
    }
}

le_result_t taf_Time::UpdateNetworkTimeZoneInfo
(
    telux::tel::NetworkTimeInfo info,
    taf_time_TimeSources_t sourceIndex
)
{
    taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)SearchSourceMap(sourceIndex);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, LE_FAULT, "Pointer for %d is not found", sourceIndex);

    srcTimePtr->timeZone = info.timeZone;
    srcTimePtr->dstAdj = info.dstAdj;
    return LE_OK;
}

void NetworkTimeResponseUpdateHandler(void* param)
{
    auto &tafTime = taf_Time::GetInstance();
    NetworkInfoUpdateArgs_t* networkInfo = (NetworkInfoUpdateArgs_t*) param;
    tafTime.NetworkTimeResponseUpdate(networkInfo->networkNumber,
         networkInfo->info, networkInfo->error);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for synching network time.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::SyncNetworkTimeResponse
(
    telux::tel::NetworkTimeInfo info, ///< [IN] Network time information.
    telux::common::ErrorCode error    ///< [IN] Error code.
)
{
    auto &tafTime = taf_Time::GetInstance();
    tafTime.NetworkUpdateInfo1.networkNumber = 1;
    tafTime.NetworkUpdateInfo1.error = error;
    tafTime.NetworkUpdateInfo1.info = info;

    le_event_QueueFunctionToThread(tafTime.mainThreadRef,
        (le_event_DeferredFunc_t)NetworkTimeResponseUpdateHandler,
        &tafTime.NetworkUpdateInfo1, NULL);

    le_result_t result = tafTime.UpdateNetworkTimeZoneInfo(info, TAF_TIME_SRC_NAME_NETWORK);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to update network source details!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for synching network time.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::SyncNetworkTimeResponse2
(
    telux::tel::NetworkTimeInfo info, ///< [IN] Network time information.
    telux::common::ErrorCode error    ///< [IN] Error code.
)
{
    auto &tafTime = taf_Time::GetInstance();
    tafTime.NetworkUpdateInfo2.networkNumber = 2;
    tafTime.NetworkUpdateInfo2.error = error;
    tafTime.NetworkUpdateInfo2.info = info;

    le_event_QueueFunctionToThread(tafTime.mainThreadRef,
        (le_event_DeferredFunc_t)NetworkTimeResponseUpdateHandler,
        &tafTime.NetworkUpdateInfo2, NULL);

    le_result_t result = tafTime.UpdateNetworkTimeZoneInfo(info, TAF_TIME_SRC_NAME_NETWORK2);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to update network source details!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Request network time.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::RequestNetworkTime(    void)
{
    auto &tafTime = taf_Time::GetInstance();
    telux::common::Status ret = telux::common::Status::FAILED;

    if (tafTime.servSysListeners.empty())
    {
        LE_DEBUG("There is no active servSysListeners");
        return;
    }

    for (size_t i = 0; i < tafTime.servingSystemManagers.size(); i++)
    {
        if (tafTime.servingSystemManagers[i] != nullptr)
        {
            if (i == 0)
            {
                if (tafTime.NetworkDeltaTime != NULL)
                {
                    ret = tafTime.servingSystemManagers[i]->requestNetworkTime(
                                                        tafTime.SyncNetworkTimeResponse);
                }
            }
            else
            {
                if (tafTime.NetworkDeltaTime2 != NULL)
                {
                    // Since currently only support 2 phones, so all other phones (if any)
                    // will report to phone 2.
                    ret = tafTime.servingSystemManagers[i]->requestNetworkTime(
                                                       tafTime.SyncNetworkTimeResponse2);
                }
            }
            if (ret == telux::common::Status::SUCCESS)
            {
                LE_DEBUG("Total phones: %zu, synching network time from phone %zu",
                                          tafTime.servingSystemManagers.size(), i+1);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Register network time Listener.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::RegNetworkTimeListener
(
    void
)
{
    le_result_t result = LE_FAULT;

    auto &tafTime = taf_Time::GetInstance();
    telux::common::Status status;

    for (size_t i = 0; i < tafTime.servingSystemManagers.size(); i++)
    {

        LE_DEBUG("Trying to Register the servSysListener, size: %zu, phoneID: %zu",
                                                       servingSystemManagers.size(), i+1);
        auto servSysListener = std::make_shared<taf_TimeServingSystemListener>(i+1);

        status = tafTime.servingSystemManagers[i]->registerListener(servSysListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Failed to register serving system listener.");
            if (result != LE_OK)
            {
                // Not to set fail if one of the registration is ok.
                result = LE_FAULT;
            }
        }
        else
        {
            tafTime.servSysListeners.emplace_back(servSysListener);
            result = LE_OK;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * De-Register network time Listener.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::DeregNetworkTimeListener
(
    void
)
{
    auto &tafTime = taf_Time::GetInstance();
    LE_DEBUG("Tring to deregister the servSysListeners");

    for (size_t i = 0; i < tafTime.servingSystemManagers.size()
                       && i < servSysListeners.size(); i++)
    {
        if (tafTime.servingSystemManagers[i] != nullptr
            && tafTime.servSysListeners[i] != nullptr)
        {
            tafTime.servingSystemManagers[i]->deregisterListener(
                tafTime.servSysListeners[i]);
            tafTime.servSysListeners[i] = nullptr;
        }
    }
    tafTime.servSysListeners.clear();
}

//--------------------------------------------------------------------------------------------------
/**
 * Network base data initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitNetworkBaseData(void)
{

    le_result_t result;
    auto& tafTime = taf_Time::GetInstance();

    if (!TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_NETWORK))
        && !TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_NETWORK2)))
    {
        LE_WARN("Both NETWORK and NETWORK2 are not exist");
        return LE_UNAVAILABLE;
    }

    if (TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_NETWORK)))
    {
        NetworkDeltaTimePool = le_mem_CreatePool("NetworkDeltaTime ",
                                                             sizeof(taf_time_TimeSpec_t));
        NetworkDeltaTime = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(NetworkDeltaTimePool);
        memset(NetworkDeltaTime, 0, sizeof(taf_time_TimeSpec_t));
        result = ReadWriteDeltaTime(NetworkDeltaTime, 0, TAF_TIME_DATA_CLEAN);
        if (result != LE_OK)
        {
            LE_FATAL("Clean network delta time failed for NETWORK");
            return result;
        }
    }

    if (TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_NETWORK2)))
    {
        NetworkDeltaTime2Pool = le_mem_CreatePool("NetworkDeltaTime2 ",
                                                             sizeof(taf_time_TimeSpec_t));
        NetworkDeltaTime2 = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(NetworkDeltaTime2Pool);
        memset(NetworkDeltaTime2, 0, sizeof(taf_time_TimeSpec_t));
        result = ReadWriteDeltaTime(NetworkDeltaTime2, 0, TAF_TIME_DATA_CLEAN);
        if (result != LE_OK)
        {
            LE_FATAL("Clean network delta time failed for NETWORK2");
            return result;
        }
    }

    netTimeInfoPool = le_mem_CreatePool("netTimeInfoPool", sizeof(taf_TimeNetTimeInfo_t));
    netTimeInfoRefMap = le_ref_CreateMap("netTimeInfoRefMap", DEFAULT_PHONE_NUM_MAX);

    TsrEventPool = le_mem_CreatePool("TsrEventPool", sizeof(TimeSourceRef_Event_t));
    TsrEventMap = le_ref_CreateMap("TsrEventMap", DEFAULT_TSR_EVENT_CNT);
    // Create the event and add event handler.
    RefTimeEventId = le_event_CreateId("RefTimeEventId", sizeof(TS_Event_t));
    RefTimeEventHandlerRef = le_event_AddHandler("RefTimeEventHandlerRef",
                                    RefTimeEventId, tafTime.EventTimeValueChangeHandler);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Network time manager initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitNetworkManager(void)
{
    le_result_t result;

    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    phoneManager = phoneFactory.getPhoneManager();

    // Check if telephony subsystem is ready
    bool subSystemStatus = phoneManager->isSubsystemReady();
    if (!subSystemStatus)
    {
        LE_INFO("Telephony subsystem wait to be ready...");
        future<bool> f = phoneManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    result = LE_FAULT;
    if (subSystemStatus)
    {
        // Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS)
        {
            LE_INFO("phoneIds.size: %zu\n", phoneIds.size());
            for (size_t index = 1; index <= phoneIds.size(); index++)
            {
                auto servingSystemManager
                    = telux::tel::PhoneFactory::getInstance().getServingSystemManager(
                                              phoneManager->getSlotIdFromPhoneId(index));
                if (servingSystemManager != nullptr)
                {
                    // Add serving system manager to vector using phoneId index as
                    // vector's index
                    servingSystemManagers.emplace_back(servingSystemManager);
                }
            }
        }

        LE_INFO("servingSystemManagers.size: %zu\n", servingSystemManagers.size());
        for (size_t index = 0; index < servingSystemManagers.size(); index++)
        {
            // Check if serving subsystem is ready
            bool servingSystemStatus = servingSystemManagers[index]->isSubsystemReady();
            if (!servingSystemStatus)
            {
                LE_INFO("Serving subsystem wait to be ready...");
                std::future<bool> f = servingSystemManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                servingSystemStatus = f.get();
            }

            if (servingSystemStatus)
            {
                // At least one of the phones is working
                result = LE_OK;
            }
            else
            {
                LE_FATAL("Fail to init %" PRIuS " serving subsystem", index);
            }
        }
    }

    if (result != LE_OK)
    {
        LE_FATAL("Fail to init telephony subsystem");
        return result;
    }

    LE_INFO("Init telephony subsystem successful");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * GNSS manager initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitGnssManager(void)
{
    auto &platformFactory = PlatformFactory::getInstance();
    bool statusUpdated = false;
    auto servicStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&statusUpdated, &servicStatus](telux::common::ServiceStatus status)
    {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdated = true;
        servicStatus = status;
        cv.notify_all();
    };

    timeManager = platformFactory.getTimeManager(statusCb);
    if (timeManager)
    {
        // Wait for time manager to be ready
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&statusUpdated] { return statusUpdated; });
    }

    if (servicStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_INFO("Time manager is ready\n");
    }
    else
    {
        LE_WARN("Unable to initialize time manager\n");
        return LE_UNAVAILABLE;
    }

    SupportTimeMask.set(SupportedTimeType::GNSS_UTC_TIME);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * GNSS base data initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitGnssBaseData(void)
{
    if (!TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_GNSS)))
    {
        LE_WARN("GNSS time source is not exist");
        return LE_UNAVAILABLE;
    }

    le_result_t result;
    GnssDeltaTimePool = le_mem_CreatePool("TimeSvc GnssDeltaTime ", sizeof(taf_time_TimeSpec_t));
    GnssDeltaTime = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(GnssDeltaTimePool);

    memset(GnssDeltaTime, 0, sizeof(taf_time_TimeSpec_t));
    result = ReadWriteDeltaTime(GnssDeltaTime, 0, TAF_TIME_DATA_CLEAN);
    if (result != LE_OK)
    {
        LE_WARN("Clean GNSS delta time failed");
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Async RTC base data initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitAsyncRtcBaseData(void)
{
    if (!TimeSourceConf.IsSourceExist(SourceNameIndexToStr(TAF_TIME_SRC_NAME_RTC)))
    {
        LE_WARN("RTC time source is not exist");
        return LE_UNAVAILABLE;
    }

    if (isDrvPresent == false)
    {
        LE_DEBUG("VHAL driver was not loaded");
        return LE_UNAVAILABLE;
    }

    if (*(timeInf->getRtcTimeReqAsync) == nullptr)
    {
        LE_DEBUG("The async API in VHAL is not implemented");
        return LE_UNAVAILABLE;
    }

    le_result_t result;
    RtcAsyncDeltaTimePool = le_mem_CreatePool("RtcAsyncDeltaTimePtr", sizeof(taf_time_TimeSpec_t));
    RtcAsyncDeltaTimePtr = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(RtcAsyncDeltaTimePool);

    memset(RtcAsyncDeltaTimePtr, 0, sizeof(taf_time_TimeSpec_t));
    result = ReadWriteDeltaTime(RtcAsyncDeltaTimePtr, 0, TAF_TIME_DATA_CLEAN);
    if (result != LE_OK)
    {
        LE_WARN("Clean RTC delta time failed");
    }

    InitAsyncRtcEvtHandler();
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes.
 */
//--------------------------------------------------------------------------------------------------
void PowerStateChangeHandler
(
    taf_pm_State_t state, ///< [IN] PM state.
    void* contextPtr      ///< [IN] Handler context.
)
{
    auto &tafTime = taf_Time::GetInstance();
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_DEBUG("Power state change to RESUME");
        if (InitNetworkManagerStatus == LE_OK)
        {
            tafTime.RegNetworkTimeListener();
        }
        tafTime.RegisterPtpDevice();
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_DEBUG("Power state change to SUSPEND");
        if (InitNetworkManagerStatus == LE_OK)
        {
            tafTime.DeregNetworkTimeListener();
        }

        if (InitGnssManagerStatus == LE_OK)
        {
            tafTime.DeregGnssTimeListener();
        }
        tafTime.ReleasePtpDevice();
    }
}

bool taf_Time::isNewTimeSrcSetTimeAllowed(taf_time_TimeSources_t newTimeSource)
{
    int position = TimeSourceConf.findSourcePosition(SourceNameIndexToStr(newTimeSource));
    if (position < 0)
    {
        LE_ERROR("%s is not found", SourceNameIndexToStr(newTimeSource));
        return false;
    }

    if (TimeSourceConf.source[position].setTimeCounter == 0)
    {
        LE_DEBUG("Set time is not allowed, setTimeCounter is: '0'");
        return false;
    }

    if (!TimeSourceConf.source[position].setSystemTime)
    {
        LE_DEBUG("Set time is not allowed, 'SetTime' was not 'true'.");
        return false;
    }

    if (LatestTimeSourceInfo->systemSourceId == TAF_TIME_SRC_NAME_UNKNOWN)
    {
        // 1. Current system time was not sync by any of the time source, need to be sync
        return true;
    }

    int newPriorityNum = TimeSourceConf.source[position].priority;

    position =
        TimeSourceConf.findSourcePosition(SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId));
    if (position < 0)
    {
        LE_ERROR("%s is not found", SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId));
        return false;
    }
    int currPriorityNum = TimeSourceConf.source[position].priority;

    LE_DEBUG("Checking priority of %s, curr %d, new %d, AllowOverrideAfterFail %" PRId64 "",
      SourceNameIndexToStr(newTimeSource), currPriorityNum, newPriorityNum, AllowOverrideAfterFail);

    // Note, the small priority number will have higher priority
    if (newPriorityNum <= currPriorityNum)
    {
        // 2. Then new time source has higher priority, the update is acceptable
        return true;
    }

    if (newPriorityNum > currPriorityNum && AllowOverrideAfterFail == 0)
    {
        // 3. Curr time source has higher priority and was timeout,
        //   the system time need to be updated
        return true;
    }

    return false;
}


void taf_Time::getRtcTimeRespCB(struct TimeSpec timeVal, le_result_t response)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    RtcEvent_t evt{};
    evt.cbType = RTC_SYNC_CB;
    evt.status = response;
    evt.timeVal = timeVal;
    le_event_Report(tafTime.RtcEvtHandlerId, &evt, sizeof(evt));
}

void taf_Time::getRtcTimeRespCbEvtHandler(const struct TimeSpec& timeVal, le_result_t response)
{
    auto& time = taf_Time::GetInstance();
    static bool initFlag = true;

    taf_time_TimeSpec_t rtcTime{}, bootTime{};
    le_result_t result = LE_FAULT;
    const taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_RTC;

    rtcTime.sec = timeVal.sec;
    rtcTime.nanosec = timeVal.nanosec;

    if (time.GetBootTime(&bootTime) == LE_OK)
    {
        time.rtcDeltaMsec = (rtcTime.sec  * 1000LL + rtcTime.nanosec  / 1000000LL)
                          - (bootTime.sec * 1000LL + bootTime.nanosec / 1000000LL);
    }

    LE_DEBUG("getRtcTimeRespCB response: %d, rtcTime.sec: %" PRIu64 ", rtcDeltaMsec: %" PRId64 "",
             response, rtcTime.sec, time.rtcDeltaMsec);

    // This part is for client only.
    if (getRTCCBtoClient.getRTCCallbackFunc)
    {
        getRTCCBtoClient.getRTCCallbackFunc(&rtcTime, response, getRTCCBtoClient.getRTCCtxPtr);
    }

    // This is part is for time service internal use.
    if (response != LE_OK || rtcTime.sec == 0)
    {
        time.SourceStatusUpdate(LE_FAULT, sourceId);
        LE_ERROR("Response for getting RTC time is NOT OK %d, time is: %" PRIu64 "",
        response, rtcTime.sec);
        return;
    }

    time.UpdateLocalTimeCache(rtcTime, sourceId, time.RtcAsyncDeltaTimePtr);
    time.UpdateFailedLoops(sourceId, FAIL_LOOP_NUM_CLEAN);
    time.SourceStatusUpdate(response, sourceId);

    if (initFlag)
    {
        // Try to update the 'time' to system at once, if it is the first time received from RTC.
        initFlag = false;
        if (time.isNewTimeSrcSetTimeAllowed(TAF_TIME_SRC_NAME_RTC))
        {
            result = time.UpdateSystemTime(rtcTime, TAF_TIME_SRC_NAME_RTC);
            if (result != LE_OK)
            {
                LE_DEBUG("Set RTC to system failed %d", result);
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This is a event handler triggered by 'setRtcTrustTimeRespCB' to indicate the set time status
 * of RTC.
 *
 * Note, this function was also used to handle RTC validity status.
 */
//--------------------------------------------------------------------------------------------------
void setRtcTrustTimeRespCbEvtHandler(le_result_t response)
{
    auto& tafTime = taf_Time::GetInstance();

    if (LE_OK != response)
    {
        LE_ERROR("Set RTC time failed");
        return;
    }

    taf_SourceInf_t* rtcPtr = tafTime.SearchSourceMap(TAF_TIME_SRC_NAME_RTC);
    if (rtcPtr == NULL)
    {
        LE_ERROR("rtcPtr is NULL");
        return;
    }

    LE_DEBUG("Set RTC response: %d, isSyncedWithStorage: %d",
                                 (int)response, rtcPtr->isSyncedWithStorage);

    // The validity in RAM for RTC can be updated only when RTC time was set successfully,
    // in this case, both the RTC time and validity read by client are matched.
    UpdateValidityInRam(rtcPtr->sourceId, LatestTimeSourceInfo->sourceValidity);

    // Update the validity to MSS for RTC if not
    if (rtcPtr->isSyncedWithStorage == false)
    {
      syncValidityToMSS(rtcPtr, LatestTimeSourceInfo);
    }
}

void taf_Time::setRtcTrustTimeRespCB(le_result_t response)
{
    taf_Time& tafTime = taf_Time::GetInstance();

    RtcEvent_t evt{};
    evt.cbType = RTC_ASYNC_CB;
    evt.status = response;
    le_event_Report(tafTime.RtcEvtHandlerId, &evt, sizeof(evt));
}

//--------------------------------------------------------------------------------------------------
/**
 * Async RTC callback handler for "setRtcTrustTimeRespCB" and "getRtcTimeRespCB".
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::RtcCbEventHandler(void* context)
{
    RtcEvent_t* evt = static_cast<RtcEvent_t*>(context);
    if (evt->cbType == RTC_ASYNC_CB)
    {
        setRtcTrustTimeRespCbEvtHandler(evt->status);
    }
    else if (evt->cbType == RTC_SYNC_CB)
    {
        getRtcTimeRespCbEvtHandler(evt->timeVal, evt->status);
    }
}

le_result_t taf_Time::GetInternalRtcTime
(
    taf_time_TimeSpec_t* timeVal
)
{
    int fd, ret;
    struct tm rtc_tm;

    memset(&rtc_tm, 0, sizeof(struct tm));
    do
    {
        fd = TEMP_FAILURE_RETRY(open(TAF_TIME_RTC_DEV_NAME, O_WRONLY));
        if (fd < 0)
        {
            fd = -errno;
        }
    } while (fd == -EBUSY);

    if (fd < 0)
    {
        LE_ERROR("Open %s failed\n", TAF_TIME_RTC_DEV_NAME);
        return LE_FAULT;
    }

    do
    {
        ret = TEMP_FAILURE_RETRY(ioctl(fd, RTC_RD_TIME, &rtc_tm));
        if (ret < 0) {
            ret = -errno;
        }
    } while (ret == -EBUSY);
    close(fd);

    if (ret < 0)
    {
        LE_ERROR("Read %s failed\n", TAF_TIME_RTC_DEV_NAME);
        return LE_FAULT;
    }

    LE_DEBUG("RTC Time:  %04d-%02d-%02d, %02d:%02d:%02d\n",
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    timeVal->sec = mktime(&rtc_tm) + rtc_tm.tm_gmtoff;
    if (timeVal->sec < 0)
    {
        LE_ERROR("Invalid RTC seconds = %" PRIu64 "\n", timeVal->sec);
        return LE_FAULT;
    }
    timeVal->nanosec = 0;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Update the time to RTC device or VHAL interface.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
 //--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetTimeToRtc
(
    taf_time_TimeSpec_t timeVal
)
{
    int ret = 0;

    if (isDrvPresent)
    {
        struct TimeSpec time;
        time.sec = timeVal.sec;
        time.nanosec = timeVal.nanosec;

        if ((timeInf == nullptr) || (*(timeInf->setRtcTimeHAL)) == nullptr)
        {
            LE_ERROR("setRtcTimeHAL not initialized");
            return LE_FAULT;
        }
        ret = (*(timeInf->setRtcTimeHAL))(time);
        if (ret < 0)
        {
            return LE_FAULT;
        }
    }
    else
    {
        LE_DEBUG("SetTimeToRtc not supported");
        return LE_UNSUPPORTED;
    }
    return LE_OK;
}

le_result_t taf_Time::GetRtcTimeReqAsync(
    taf_time_AsyncGetTimeReqHandlerFunc_t toClientHandlerPtr,
    void* contextPtr
)
{
    le_result_t result = LE_UNSUPPORTED;

    if (isDrvPresent)
    {
        if ((timeInf == nullptr) || ((*(timeInf->getRtcTimeReqAsync)) == nullptr))
        {
            LE_ERROR("getRtcTimeHAL not initialized - Async");
            return LE_BAD_PARAMETER;
        }

        (taf_Time::getRTCCBtoClient).getRTCCallbackFunc = toClientHandlerPtr;
        (tafsvc::taf_Time::getRTCCBtoClient).getRTCCtxPtr = contextPtr;
        (tafsvc::taf_Time::getRTCCBtoClient).sessionRef = taf_time_GetClientSessionRef();

        result = (*(timeInf->getRtcTimeReqAsync))(taf_Time::getRtcTimeRespCB);
    }
    else
    {
        LE_DEBUG("GetRtcTimeReqAsync not supported");
        return LE_UNSUPPORTED;
    }

    return result;
}

le_result_t taf_Time::SetRtcTimeReqAsync
(
    const taf_time_TimeSpec_t* timeValPtr,
    taf_time_AsyncSetTimeReqHandlerFunc_t toClientHandlerPtr,
    SetRtcHalCb cbFromClient,
    void* contextPtr
)
{
    le_result_t result = LE_UNSUPPORTED;

    if (isDrvPresent)
    {
        if ((timeInf == NULL) || ((*(timeInf->setRtcTimeReqAsync)) == NULL))
        {
            LE_ERROR("setRtcTimeReqAsync not initialized");
            return LE_FAULT;
        }

        (tafsvc::taf_Time::setRTCCBtoClient).setRTCCallbackFunc = toClientHandlerPtr;
        (tafsvc::taf_Time::setRTCCBtoClient).setRTCCtxPtr = contextPtr;
        (tafsvc::taf_Time::setRTCCBtoClient).sessionRef = taf_time_GetClientSessionRef();

        struct TimeSpec timeSpec;
        timeSpec.sec = timeValPtr->sec;
        timeSpec.nanosec = timeValPtr->nanosec;

        result = (*(timeInf->setRtcTimeReqAsync))(&timeSpec, cbFromClient);
    }
    else
    {
        LE_DEBUG("SetRtcTimeReqAsync not supported");
        return LE_UNSUPPORTED;
    }

    return result;
}

taf_SourceInf_t* taf_Time::SearchSourceMap(
    taf_time_TimeSources_t sourceId,
    le_msg_SessionRef_t sessionRef,
    bool checkSessionRef)
{
    auto& tafTime = taf_Time::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafTime.SrcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)le_ref_GetValue(iterRef);
        if (srcTimePtr == NULL)
            continue;

        if (checkSessionRef)
        {
            if (!srcTimePtr->isBaseStruct &&
                srcTimePtr->sourceId == sourceId &&
                srcTimePtr->sessionRef == sessionRef)
            {
                return srcTimePtr;
            }
        }
        else
        {
            if (srcTimePtr->isBaseStruct &&
                srcTimePtr->sourceId == sourceId)
            {
                return srcTimePtr;
            }
        }
    }
    return NULL;
}


taf_time_SourceRef_t taf_Time::GetSourceRef
(
    taf_time_TimeSources_t sourceId
)
{
    taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)SearchSourceMap(sourceId);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, NULL, "Source Reference is not found!");

    LE_INFO("Source Ref %p for %s, Id: (0x%x)",
        srcTimePtr->ref, SourceNameIndexToStr(srcTimePtr->sourceId), sourceId);

    return srcTimePtr->ref;
}

le_result_t taf_Time::GetFailedLoops
(
    taf_time_SourceRef_t sourceRef,
    int32_t* failedLoops,
    int64_t* loopIntervalSec
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, LE_FAULT, "Source Reference is NULL.");
    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Source Reference is not registered.");
    *failedLoops = sourcePtr->failedLoops;
    *loopIntervalSec = TimeSourceConf.pollingInterval;
    return LE_OK;
}

bool taf_Time::IsAvailable
(
    taf_time_SourceRef_t sourceRef
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, false, "Source Reference is NULL.");
    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, false, "Source Reference is not registered.");
    return sourcePtr->isAvailable;
}

le_result_t taf_Time::GetSystemTimeSourceID
(
    taf_time_TimeSources_t* timeSource
)
{
    TAF_ERROR_IF_RET_VAL(timeSource == NULL, LE_FAULT, "Time Source is NULL.");

    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)SearchSourceMap(TAF_TIME_SRC_NAME_SYSTEM);

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Time Source is not registered.");

    *timeSource = sourcePtr->systemSourceId;
    return LE_OK;
}

void timeSourceStatusHandler(void* reportPtr)
{
    auto& time = taf_Time::GetInstance();
    SourceStatusChange_Event_t* evt = (SourceStatusChange_Event_t*)reportPtr;
    taf_SourceInf_t* sourcePtr = evt->sourcePtr;
    bool sourceStatus = evt->status;
    taf_time_StatusEventType_t eventType = evt->eventType;

    LE_DEBUG("Time source: %s type: %d change status to %d",
      time.SourceNameIndexToStr(sourcePtr->sourceId), (int)eventType, sourceStatus);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(time.SrcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SourceInf_t* mapSrcTimePtr = (taf_SourceInf_t*)le_ref_GetValue(iterRef);
        if (mapSrcTimePtr != NULL &&
            mapSrcTimePtr->sourceId == sourcePtr->sourceId &&
            mapSrcTimePtr->handlerRef != NULL &&
            mapSrcTimePtr->handlerFunc != NULL &&
            mapSrcTimePtr->eventType == eventType
        )
        {
           if (sourceStatus)
            {
                if(mapSrcTimePtr->eventType == TAF_TIME_STATUS_EVENT_AVAILABILITY)
                {
                    LE_INFO("Time source: %s is Available!",
                    time.SourceNameIndexToStr(mapSrcTimePtr->sourceId));
                }
                else if(mapSrcTimePtr->eventType == TAF_TIME_STATUS_EVENT_VALIDITY)
                {
                    LE_INFO("Time source: %s is valid!",
                    time.SourceNameIndexToStr(mapSrcTimePtr->sourceId));
                }
            }
            else
            {
                if(mapSrcTimePtr->eventType == TAF_TIME_STATUS_EVENT_AVAILABILITY)
                {
                    LE_INFO("Time source: %s is NOT Available!",
                    time.SourceNameIndexToStr(mapSrcTimePtr->sourceId));
                }
                else if(mapSrcTimePtr->eventType == TAF_TIME_STATUS_EVENT_VALIDITY)
                {
                    LE_INFO("Time source: %s is NOT valid!",
                    time.SourceNameIndexToStr(mapSrcTimePtr->sourceId));
                }
            }
            mapSrcTimePtr->handlerFunc(mapSrcTimePtr->ref, mapSrcTimePtr->eventType,
                                              sourceStatus, mapSrcTimePtr->context);
        }
    }
    return;
}

taf_time_TimeSourceStatusHandlerRef_t taf_Time::AddTimeSourceStatusHandler
(
    taf_time_SourceRef_t srcRef,
    taf_time_StatusEventType_t statusEventType,
    taf_time_TimeSourceStatusHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafTime = taf_Time::GetInstance();
    TAF_ERROR_IF_RET_VAL(srcRef == NULL, NULL, "Source Reference is NULL.");

    taf_SourceInf_t* baseSrcTimePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, srcRef);

    TAF_ERROR_IF_RET_VAL(baseSrcTimePtr == NULL, NULL, "Source Reference not found!.");

    TAF_ERROR_IF_RET_VAL((baseSrcTimePtr->sourceId == TAF_TIME_SRC_NAME_SYSTEM &&
        statusEventType == TAF_TIME_STATUS_EVENT_AVAILABILITY),
    NULL, "Cannot register handler for source(0x%x). Not Supported", baseSrcTimePtr->sourceId);

    if (statusEventType == TAF_TIME_EVENT_TYPE_LOWER_BOUND ||
       statusEventType > TAF_TIME_EVENT_TYPE_UPPER_BOUND)
    {
        LE_ERROR("Please provide a valid event type.");
        return NULL;
    }
    le_msg_SessionRef_t clientSessionRef = taf_time_GetClientSessionRef();

    if(SearchSourceMap(baseSrcTimePtr->sourceId, clientSessionRef, true) != NULL)
    {
        LE_ERROR("Only one handler is allowed per session.");
        return NULL;
    }

    taf_SourceInf_t* eventSrcPtr = (taf_SourceInf_t*)le_mem_ForceAlloc(tafTime.SrcPool);
    eventSrcPtr->sourceId = baseSrcTimePtr->sourceId;
    eventSrcPtr->failedLoops = baseSrcTimePtr->failedLoops;
    eventSrcPtr->systemSourceId = TAF_TIME_SRC_NAME_UNKNOWN;
    eventSrcPtr->isAvailable = baseSrcTimePtr->isAvailable;
    eventSrcPtr->sourceValidity = baseSrcTimePtr->sourceValidity;
    eventSrcPtr->isSyncedWithStorage = baseSrcTimePtr->isSyncedWithStorage;
    eventSrcPtr->isSyncedWithSetCmd = baseSrcTimePtr->isSyncedWithSetCmd;
    eventSrcPtr->sessionRef = clientSessionRef;
    eventSrcPtr->context = contextPtr;
    eventSrcPtr->isBaseStruct = false;
    eventSrcPtr->ref = baseSrcTimePtr->ref;

    eventSrcPtr->handlerFunc = handlerFuncPtr;
    eventSrcPtr->eventType = statusEventType;
    eventSrcPtr->handlerRef =
        (taf_time_TimeSourceStatusHandlerRef_t)le_ref_CreateRef(SrcRefMap, eventSrcPtr);

    LE_INFO("Registering handler reference %p for %s", eventSrcPtr->handlerRef,
        SourceNameIndexToStr(eventSrcPtr->sourceId));
    return eventSrcPtr->handlerRef;
}

void taf_Time::RemoveTimeSourceStatusHandler
(
    taf_time_TimeSourceStatusHandlerRef_t handlerRef
)
{
    taf_SourceInf_t* srcStatusFuncPtr =
        (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, handlerRef);

    if (srcStatusFuncPtr != NULL)
    {
        // Do sanity check.
        LE_ASSERT(srcStatusFuncPtr->handlerRef == handlerRef);

        taf_SourceInf_t* srcTimePtr =
            (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, srcStatusFuncPtr->handlerRef);

        if (srcTimePtr != NULL)
        {
            srcTimePtr->handlerRef = NULL;
            srcTimePtr->handlerFunc = NULL;
        }

        LE_INFO("Removed srcStatusFuncRef(%p).", handlerRef);

        // Free the handler.
        le_ref_DeleteRef(SrcRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("Invalid srcStatusFuncRef(%p).", handlerRef);
    }

    return;
}

le_result_t taf_Time::GetTimeZone
(
    taf_time_SourceRef_t sourceRef,
    int8_t* timeZone
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, LE_FAULT, "Source Reference is NULL.");

    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Source Reference is not registered.");

    if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_NETWORK &&
        sourcePtr->sourceId != TAF_TIME_SRC_NAME_NETWORK2)
    {
        LE_ERROR("TimeZone is not supported for the given source reference!");
        return LE_BAD_PARAMETER;
    }

    // Is there a need to double check the return value?

    if(-48 <= sourcePtr->timeZone && sourcePtr->timeZone <= 48
    && sourcePtr->isAvailable == true)
    {
        *timeZone = sourcePtr->timeZone;
        return LE_OK;
    }
    else
    {
        LE_ERROR("Network unavailable. Unable to get timezone!");
        *timeZone = 0;
        return LE_FAULT;
    }

    return LE_FAULT;
}

le_result_t taf_Time::GetTimeDayAdj
(
    taf_time_SourceRef_t sourceRef,
    uint8_t* dayltSavAdj
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, LE_FAULT, "Source Reference is NULL.");

    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Source Reference is not registered.");

    if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_NETWORK &&
        sourcePtr->sourceId != TAF_TIME_SRC_NAME_NETWORK2)
    {
        LE_ERROR("TimeZone is not supported for the given source reference!");
        return LE_BAD_PARAMETER;
    }
    if(sourcePtr->isAvailable == true)
    {
        *dayltSavAdj = sourcePtr->dstAdj;
        return LE_OK;
    }
    return LE_FAULT;
}

bool taf_Time::IsSourceValid
(
    taf_time_SourceRef_t sourceRef
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, false, "Source Reference is NULL.");
    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, false, "Source Reference is not registered.");

    if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_RTC)
    {
        // Eventually, return the 'ram-value' in all ways.
        // Note: for the improper-source requests, such as: NETWORK..
        //       just return the ram-value.
        return sourcePtr->sourceValidity;
    }

    if (sourcePtr->isSyncedWithSetCmd == true)
    {
        LE_DEBUG("Return [validity], after SetCmd=ture");
    }
    else
    {
        if (sourcePtr->isSyncedWithStorage == true)
        {
            LE_DEBUG("Return [validity], after !SetCmd && SyncedMss");
        }
        else
        {
            LE_DEBUG("Return [validity], after !SetCmd && !SyncedMss");
            if (isSecStorageConnected())
            {
                taf_Time& tafTime = taf_Time::GetInstance();

                // SyncedMss == false, try to connect the storage once.
                bool validity = false;
                le_result_t rst =
                    tafTime.ReadValidityFromSecStorage(sourcePtr, &validity);
                if (rst == LE_OK)
                {
                    LE_DEBUG("!SetCmd && !SyncedMss, touched the storage");

                    // Mark the flag to reflect the storage has been touched.
                    sourcePtr->isSyncedWithStorage = true;
                    sourcePtr->sourceValidity = validity;
                }
            }
            else
            {
                LE_DEBUG("!SetCmd && !SyncedMss, can't access the storage");
            }
        }
    }

    return sourcePtr->sourceValidity;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the client has the permisson to change the validity.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::CheckSetValidityPermission(void)
{
    le_msg_SessionRef_t clientSessionRef = taf_time_GetClientSessionRef();
    pid_t pid;
    char appName[100] = {0};

    if (LE_OK != le_msg_GetClientProcessId(clientSessionRef, &pid))
    {
        LE_ERROR("Error, Failed to get client pid.");
        return LE_FAULT;
    }

    if(le_appInfo_GetName(pid, appName, sizeof(appName)) == LE_OK)
    {
        LE_INFO("Client appName: %s", appName);
        for(uint i = 0; i < TimeSourceConf.validClientList.size(); i++)
        {
            if(strcmp(appName, TimeSourceConf.validClientList[i].c_str()) == 0)
            {
               LE_INFO("App is in the client valid list");
               return LE_OK;
               break;
            }
        }
    }
    return LE_FAULT;
}

le_result_t taf_Time::WriteValidtyToSecStorage(taf_SourceInf_t* sourcePtr, bool newvalidity)
{
    le_result_t res = LE_FAULT;
    uint8_t validityToSet = newvalidity == true ? 1 : 0;

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_BAD_PARAMETER, "sourcePtr is NULL");
    if(!isSecLableCreated(sourcePtr->sourceId))
    {
        LE_ERROR("Create secure lable for %s failed", SourceNameIndexToStr(sourcePtr->sourceId));
        return LE_FAULT;
    }

    taf_mngdStorSecData_DataRef_t dataRef =
        taf_mngdStorSecData_GetDataRef(SourceNameIndexToStr(sourcePtr->sourceId));
    TAF_ERROR_IF_RET_VAL(dataRef == nullptr, LE_NOT_FOUND, "data ref does not exist");

    // Updating reference for time source if NULL
    if(sourcePtr->secStrgdataRef == nullptr)
    {
        sourcePtr->secStrgdataRef = dataRef;
    }

    res = taf_mngdStorSecData_WriteDataStart(dataRef);
    if(res != LE_OK)
    {
        LE_ERROR("Cannot start writing validity in secure storage.");
        goto writeErr;
    }
    res = taf_mngdStorSecData_WriteDataChunk(dataRef, &validityToSet, sizeof(validityToSet));

    if(res != LE_OK)
    {
        LE_ERROR("Cannot write validity in secure storage.");
        goto writeErr;
    }
    res = taf_mngdStorSecData_WriteDataEnd(dataRef);

    if(res != LE_OK)
    {
        LE_ERROR("Cannot end writing validity in secure storage.");
        goto writeErr;
    }
    LE_INFO("Validity of %s is '%s' successfully written to MSS.",
        SourceNameIndexToStr(sourcePtr->sourceId), validityToSet == true ? "true" : "false");

    writeErr:
        return res;
}

le_result_t taf_Time::ReadValidityFromSecStorage
(
    taf_SourceInf_t* sourcePtr,
    bool* validity
)
{
    le_result_t res;
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_BAD_PARAMETER, "sourcePtr is NULL");

    taf_mngdStorSecData_DataRef_t dataRef =
        taf_mngdStorSecData_GetDataRef(SourceNameIndexToStr(sourcePtr->sourceId));
    if (dataRef == NULL)
    {
        LE_ERROR("MSS storage reference not found for %s time source",
            SourceNameIndexToStr(sourcePtr->sourceId));
        return LE_NOT_FOUND;
    }

    // Updating reference for time source if NULL
    if(sourcePtr->secStrgdataRef == nullptr)
    {
        sourcePtr->secStrgdataRef = dataRef;
    }

    uint8_t readBuf;
    size_t readLen = sizeof(readBuf);

    res = taf_mngdStorSecData_ReadDataFirstChunk(dataRef, &readBuf, &readLen);
    if(res != LE_OK)
    {
        LE_WARN("Failed to read validity from secure storage.");
        return res;
    }

    *validity = readBuf == 1 ? true : false;

    // Output the validity value for trace.
    LE_INFO("Validity of %s is '%s' (size:%" PRIuS ") read successfully from MSS",
                                        SourceNameIndexToStr(sourcePtr->sourceId),
                                   *validity == true ? "true" : "false", readLen);
    return res;
}

/*======================================================================

 FUNCTION        taf_Time::Init

 DESCRIPTION     Initialization of the Time Service

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Time::Init(void)
{
    le_result_t result;

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);
    mainThreadRef = le_thread_GetCurrent();

    // 1. Create memory pools and initialization
    SetTimeStatusPool = le_mem_CreatePool("TimeSvc SetStatusPool", sizeof(SetTimeStatus));
    SetTimeSt = (SetTimeStatus *)le_mem_ForceAlloc(SetTimeStatusPool);
    memset(SetTimeSt, 0, sizeof(struct SetTimeStatus));

    timeSourceChangePool = le_mem_CreatePool("timeSourceChangePool",
                                              sizeof(taf_TimeSourceStatus_t));
    timeSourceChangeId = le_event_CreateIdWithRefCounting("TimeSourceStatus");

    // 2. Load JSON configurations
    result = LoadJsonConfiguration(TimeSourceConf, TAF_TIME_SERVICE_CONF_FILE);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Read configuration failed\n");
    }
    else
    {
        DeleteNotSupportedSource(TimeSourceConf);

        // Move the high priority time source to the beginning of the array
        TimeSourceConf.sortSourceByPriority();

        // Print out all the time source information
        TimeSourceConf.printSourceDetails();
    }

    InitTimeSource();
    // 3. Create thread for runtime sync time.
    le_thread_Ref_t threadRunTimeSyncRef = le_thread_Create("SyncTimeThread", SyncTimeTasks, NULL);
    le_thread_Start(threadRunTimeSyncRef);

    // 4. Add power state change handle.
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);

    // 5. Create event ID for time source status change.
    timeSourceStatusEventId =
        le_event_CreateId("timeSourceStatusEventId", sizeof(SourceStatusChange_Event_t));
    le_event_AddHandler("TimeSourceStatusHandlerRef",
        timeSourceStatusEventId, timeSourceStatusHandler);

    // 6.Initialize async RTC and system time's validity, report the event 'timeSourceStatusEventId'
    // if the status get changed.
    InitAsyncRtcAndSystemTrustInfo();

    // 7.Start to get time from time source and set to system in loop.
    StartSetTimeForSystem();

}

