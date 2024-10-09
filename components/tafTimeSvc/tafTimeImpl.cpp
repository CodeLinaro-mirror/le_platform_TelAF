/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tafTime.hpp"
#include "taf_gptpTime.h"

using namespace telux::tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Object to store time source configuration.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_TIME_MAX_SOURCE_NUMBER (TAF_TIME_SRC_NAME_UNKNOWN*3)
TimeSources TimeSourceConf(TAF_TIME_MAX_SOURCE_NUMBER);
taf_SourceInf_t *LatestTimeSourceInfo;
bool GnssStatusUpdateFlag = true;

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
    auto &tafTime = taf_Time::GetInstance();
    taf_time_TimeSpec_t timeVal = {0};
    taf_time_TimeSources_t sourceId;
    le_result_t result = LE_OK;

    LE_INFO("Phone %d, NITZ:%s\n", phone, info.nitzTime.c_str());
    result = tafTime.ConvertNetworkTimeToSec(info, &timeVal);
    if (phone == 1)
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
    uint64_t* utc = (uint64_t*) param;

     if (*utc == 0) {
        if(GnssStatusUpdateFlag == true)
        {
            GnssStatusUpdateFlag = false;
            tafTime.SourceAvailabilityUpdate(LE_FAULT, sourceId);
        }
        return;
    }
    GnssStatusUpdateFlag = true;

    tafTime.SourceAvailabilityUpdate(LE_OK, sourceId);
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
    auto &tafTime = taf_Time::GetInstance();
    uint64_t arg = utc;
    le_event_QueueFunctionToThread(tafTime.mainThreadRef, (le_event_DeferredFunc_t)onGnssUtcTimeUpdateHandler,&arg, NULL);
    if(utc > 0)
    {
        tafTime.UpdateFailedLoops(TAF_TIME_SRC_NAME_GNSS, FAIL_LOOP_NUM_CLEAN);
        timeVal.sec = (utc / 1000);
        timeVal.nanosec = (utc % 1000)*1000*1000;

        LE_DEBUG("Received gnss UTC time: %" PRIu64 "\n", timeVal.sec);
        tafTime.UpdateLocalTimeCache(timeVal, TAF_TIME_SRC_NAME_GNSS, tafTime.GnssDeltaTime);
        tafTime.ReportTimeValueChange(TAF_TIME_SRC_NAME_GNSS, timeVal, NULL);
        tafTime.DeregGnssTimeListener();
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
    long int pollingInterval, toleranceMillsec;
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

    itemData = json_object_get(serviceDataPtr, TAF_TIME_TOLERANCES_SETTING_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: Tolerances string was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    sscanf(value,"%ld", &toleranceMillsec);
    serviceCfg.toleranceMillsec = toleranceMillsec;

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
            // Copy local maintained data 'deltaTimeData' to buf 'timeVal'
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

    if (TimeSourceConf.toleranceMillsec <= 0)
    {
        milliSecThreshold = TAF_TIME_THRESHOLD_MILLISEC;
    }
    else
    {
        milliSecThreshold = TimeSourceConf.toleranceMillsec;
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

        LE_DEBUG("Update GNSS time to: sec "
            "%" PRIu64 " , nsec %" PRIu64 ". DT milliSec %" PRIu64 ", Threshold %" PRIu64 "\n",
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

    if (deltaTimeDataPtr == NULL)
    {
        LE_ERROR("Not initialized %s buffer\n", SourceNameIndexToStr(sourceName));
        return LE_UNAVAILABLE;
    }

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
    taf_time_TimeSpec_t* timeValPtr
)
{
    le_result_t result = LE_FAULT;
    if (isDrvPresent)
    {
        if ((*(timeInf->getRtcTimeHAL)) == nullptr)
        {
            LE_ERROR("getRtcTimeHAL not initialized");
            return LE_FAULT;
        }

        struct TimeSpec obj;
        result = (*(timeInf->getRtcTimeHAL))(&obj);
        if (result < 0)
        {
            LE_ERROR("getRtcTimeHAL return failed");
            return LE_FAULT;
        }
        timeValPtr->sec = obj.sec;
        timeValPtr->nanosec = obj.nanosec;
        return LE_OK;
    }
    else
    {
        result = GetInternalRtcTime(timeValPtr);
        if(result < 0)
        {
            return result;
        }
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
        SourceAvailabilityUpdate(LE_FAULT, TAF_TIME_SRC_NAME_EX_APP);
        return LE_OK;
    }
    return LE_TIMEOUT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get RTC async callback function set time status for current run loop.
 *
 * @return
 *     - LE_OK -- RTC async callback function set time occurred.
 *     - LE_TIMEOUT -- RTC async callback set time did not occur.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetAsyncRtcSetTimeStatus(void)
{
    if (SetTimeSt->asyncRtcSetTime)
    {
        // Clear the flag here, and it needs to be set 'true' in the callback function of
        //  RTC async set time API
        SetTimeSt->asyncRtcSetTime = false;
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
     if ( !(sourceId >= TAF_TIME_SRC_NAME_RTC && sourceId < TAF_TIME_SRC_NAME_UNKNOWN)
        || (sourceId == TAF_TIME_SRC_NAME_EX_APP))
    {
        if((sourceId != TAF_TIME_SRC_NAME_SYSTEM))
        {
            LE_ERROR("Not supported time source ID.");
            return NULL;
        }
    }

    if (!TimeSourceConf.IsSourceExist(SourceNameIndexToStr(sourceId)))
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

    LE_INFO("timeSrcRef %p for client sessionRef %p, sourceId (0x%x).",
                                                   srcTimePtr->ref, sessionRef, sourceId);
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
        result = GetRtcTime(timeValPtr);
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
        (taf_SourceInf_t*)SearchAvailableSourceInfList(srcTimePtr->sourceId);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, LE_NOT_FOUND, "Source Reference is not found!");
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
        LE_DEBUG("No client was connected\n");
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
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSpec_t systemTime
)
{
    uint64_t deltaMilliSec, milliSecThreshold;
    taf_time_TimeSpec_t tmpTime;

    if (TimeGreaterThan(timeVal, systemTime))
    {
        tmpTime = taf_time_Sub(timeVal, systemTime);
    }
    else
    {
        tmpTime = taf_time_Sub(systemTime, timeVal);
    }

    if (TimeSourceConf.toleranceMillsec <= 0)
    {
        milliSecThreshold = TAF_TIME_THRESHOLD_MILLISEC;
    }
    else
    {
        milliSecThreshold = TimeSourceConf.toleranceMillsec;
    }

    deltaMilliSec = tmpTime.sec * 1000 + tmpTime.nanosec/1000/1000;
    if (deltaMilliSec < milliSecThreshold)
    {
        LE_DEBUG("Set time not need. delta %" PRIu64 ", threshold: %" PRIu64 "\n",
            deltaMilliSec, milliSecThreshold);
        return false;
    }

    return true;
}

le_result_t taf_Time::UpdateDeltaTimeToStorage
(
    taf_time_TimeSpec_t timeVal
)
{
    le_result_t result = LE_OK;
    uint64_t oldDelta_Msec = 0, newDelta_Msec = 0, dataMsec = 0;
    taf_time_TimeSpec_t rtcTimeVal;

    int fd = open(TAF_TIME_DELTA_TIME_PATH, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        LE_ERROR("Open file: %s failed, %s\n", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
        return LE_FAULT;
    }

    result = GetInternalRtcTime(&rtcTimeVal);
    if(result != LE_OK)
    {
        LE_ERROR("Read RTC failed %d\n", result);
        close(fd);
        return result;
    }

    if (rtcTimeVal.sec > timeVal.sec)
    {
        newDelta_Msec = (rtcTimeVal.sec * 1000) + (rtcTimeVal.nanosec/1000)
                      - (timeVal.sec * 1000) - (timeVal.nanosec/1000);
    }
    else
    {
        newDelta_Msec = (timeVal.sec * 1000) + (timeVal.nanosec/1000)
                      - (rtcTimeVal.sec * 1000) - (rtcTimeVal.nanosec/1000);
    }

    ssize_t bytesRead = read(fd, &dataMsec, sizeof(uint64_t));
    if (bytesRead <= 0)
    {
        LE_DEBUG("Read file %s failed, bytes read: %zd\n", TAF_TIME_DELTA_TIME_PATH, bytesRead);
        if (write(fd, &newDelta_Msec, sizeof(uint64_t)) < 0)
        {
            LE_ERROR("Write file: %s failed, %s\n", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
        }
        close(fd);
        return LE_FAULT;
    }
    else
    {
        oldDelta_Msec = dataMsec;
        if ((newDelta_Msec > oldDelta_Msec + 1000) || (oldDelta_Msec > newDelta_Msec + 1000))
        {
            dataMsec = newDelta_Msec;
            /* Move the file pointer to the beginning */
            lseek(fd, 0, SEEK_SET);
            if (write(fd, &dataMsec, sizeof(uint64_t)) < 0)
            {
                LE_ERROR("Write file: %s failed, %s\n", TAF_TIME_DELTA_TIME_PATH, strerror(errno));
                close(fd);
                return LE_FAULT;
            }
        }
    }

    LE_DEBUG("RTC sec %" PRIu64 ", System sec %" PRIu64 ", oldDlt sec %" PRIu64 ", newDlt sec "
             "%" PRIu64 "\n", rtcTimeVal.sec, timeVal.sec, oldDelta_Msec/1000, newDelta_Msec/1000);

    close(fd);

    return LE_OK;
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
le_result_t taf_Time::SetSystemTime
(
    taf_time_TimeSpec_t timeVal,
    taf_time_TimeSources_t timeSource,
    bool ackTimeSvc
)
{
    auto &tafTime = taf_Time::GetInstance();
    le_result_t result = LE_OK;
    int position = 0;
    taf_time_TimeSpec_t systemTime;
    struct timespec newTime;

    if (ackTimeSvc)
    {
        position = TimeSourceConf.findSourcePosition(
                                SourceNameIndexToStr(TAF_TIME_SRC_NAME_EX_APP));
        if (position < 0)
        {
            LE_ERROR("ackTimeSvc was set to %d, but %s is not found\n",
                    ackTimeSvc, SourceNameIndexToStr(TAF_TIME_SRC_NAME_EX_APP));
            return LE_NOT_FOUND;
        }

        UpdateFailedLoops(TAF_TIME_SRC_NAME_EX_APP, FAIL_LOOP_NUM_CLEAN);
        SourceAvailabilityUpdate(LE_OK, TAF_TIME_SRC_NAME_EX_APP);

        if (!TimeSourceConf.source[position].setSystemTime)
        {
            LE_ERROR("Set system time policy is not enabled, set-flag: %d\n",
                                TimeSourceConf.source[position].setSystemTime);
            return LE_NOT_PERMITTED;
        }
    }
    result = GetSystemTime(&systemTime);
    if (result != LE_OK)
    {
        LE_ERROR("Get system time failed\n");
        return result;
    }
    if (IsThresholdSetTimeAllow(timeVal, systemTime))
    {
        newTime.tv_sec = timeVal.sec;
        newTime.tv_nsec = timeVal.nanosec;

        if (clock_settime(CLOCK_REALTIME, &newTime) < 0)
        {
            LE_ERROR("Update sys time to:  "
            "%lld.%ld, from:%" PRIu64 ".%" PRIu64 ". SRC: %s, ackTimeSvc %d\n",
                (long long)newTime.tv_sec, newTime.tv_nsec, systemTime.sec, systemTime.nanosec,
                SourceNameIndexToStr(timeSource), ackTimeSvc);
            switch (errno)
            {
                case EPERM:
                    LE_ERROR("Setting CLOCK_REALTIME not permitted\n");
                    return LE_NOT_PERMITTED;

                case EINVAL:
                    LE_ERROR("Invalid parameter to set CLOCK_REALTIME\n");
                    return LE_BAD_PARAMETER;

                default:
                    LE_ERROR("Unable to set CLOCK_REALTIME (errno = %d)\n", errno);
                    return LE_FAULT;
            }
        }
        if(timeSource != TAF_TIME_SRC_NAME_RTC)
        {
            if (LE_OK != SetRtcTimeReqAsync(&timeVal, nullptr, NULL))
            {
                LE_WARN("Set %s time for RTC failed\n", SourceNameIndexToStr(timeSource));
            }
        }
        LE_INFO("Update sys time to:  "
                    "%lld.%ld, from:%" PRIu64 ".%" PRIu64 ". SRC: %s\n",
            (long long)newTime.tv_sec, newTime.tv_nsec, systemTime.sec, systemTime.nanosec,
                                                     SourceNameIndexToStr(timeSource));

    }

    if(access(TAF_TIME_DELTA_TIME_DIR, F_OK) != -1)
    {
        UpdateDeltaTimeToStorage(timeVal);
    }

    if (LatestTimeSourceInfo->systemSourceId != timeSource)
    {
        LE_INFO("Switching time source from %s to %s",
        SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId),SourceNameIndexToStr(timeSource));

        TimeSourceChangeNotify(LatestTimeSourceInfo->systemSourceId, timeSource);
        LatestTimeSourceInfo->systemSourceId = timeSource;
        AllowOverrideAfterFail = TimeSourceConf.allowOverrideAfterFail;
    }

    taf_SourceInf_t* sourcePtr = tafTime.SearchAvailableSourceInfList(timeSource);
    if(sourcePtr != NULL &&
       LatestTimeSourceInfo->sourceValidity != sourcePtr->sourceValidity)
    {
        tafTime.WriteValidtyToSecStorage(LatestTimeSourceInfo,sourcePtr->sourceValidity);
        LatestTimeSourceInfo->sourceValidity = sourcePtr->sourceValidity;
        if(LatestTimeSourceInfo->handlerFunc != NULL &&
            LatestTimeSourceInfo->eventType == TAF_TIME_STATUS_EVENT_VALIDITY)
        {
            ReportValidityChange(LatestTimeSourceInfo);
        }
    }

    if (ackTimeSvc && (timeSource == TAF_TIME_SRC_NAME_EX_APP))
    {
        SetTimeSt->externalSetTime = true;
    }
    else if (timeSource == TAF_TIME_SRC_NAME_RTC)
    {
        SetTimeSt->asyncRtcSetTime = true;
    }

    LatestTimeSourceInfo->failedLoops = 0;
    LatestTimeSourceInfo->isAvailable = true;
    return LE_OK;
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
    switch (sourceIndex)
    {
        case TAF_TIME_SRC_NAME_RTC:
            result = GetRtcTimeReqAsync(nullptr, NULL);
            if (result == LE_OK)
            {
                result = GetAsyncRtcSetTimeStatus();
            }
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
    UpdateFailedLoops(sourceIndex, FAIL_LOOP_NUM_INCREASE);

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

void taf_Time::SourceAvailabilityUpdate(le_result_t result, taf_time_TimeSources_t sourceIndex)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    bool previousAvailablility = (tafTime.PrevSrcAvailabiltyMap >> sourceIndex) & 1;
    taf_SourceInf_t* sourcePtr = tafTime.SearchAvailableSourceInfList(sourceIndex);
    bool oldValidity = false;

    if (sourcePtr != NULL)
    {
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
                oldValidity = sourcePtr->sourceValidity;
                if(oldValidity != true)
                {
                    tafTime.WriteValidtyToSecStorage(sourcePtr, true);
                }
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
                oldValidity = sourcePtr->sourceValidity;
                if(oldValidity != false)
                {
                    tafTime.WriteValidtyToSecStorage(sourcePtr, false);
                }
                sourcePtr->sourceValidity = false;
            }
        }

        LE_DEBUG("For source %s: previousAvailablility %d and currentAvailability %d ",
            SourceNameIndexToStr(sourceIndex), previousAvailablility, sourcePtr->isAvailable);

        // Check if time source status is changed or not
        if (previousAvailablility != sourcePtr->isAvailable)
        {
            // Check if a handler is registered for the time source by user
            if
            (
                sourcePtr->handlerFunc != NULL &&
                (sourcePtr->eventType & TAF_TIME_STATUS_EVENT_AVAILABILITY) != 0
            )
            {
                SourceStatusChange_Event_t evt;
                evt.sourcePtr = sourcePtr;
                evt.status = sourcePtr->isAvailable;
                evt.eventType = TAF_TIME_STATUS_EVENT_AVAILABILITY;
                le_event_Report(timeSourceStatusEventId, &evt, sizeof(evt));
            }
        }

        if
        (
            oldValidity != sourcePtr->sourceValidity &&
            sourcePtr->handlerFunc != NULL &&
            (sourcePtr->eventType & TAF_TIME_STATUS_EVENT_VALIDITY) != 0
        )
        {
            ReportValidityChange(sourcePtr);
        }
    }
}

void taf_Time::ReportValidityChange(taf_SourceInf_t* sourcePtr)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    SourceStatusChange_Event_t evt;
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

    taf_SourceInf_t* sourcePtr = tafTime.SearchAvailableSourceInfList(sourceIndex);
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
    //1. Check the status for all supported time sources.
    //2. Get the time from the first high priority and active time source.
    //3. Set the time to system.
    for (i = 0; i < (int)serviceCfg.source.size(); i++)
    {
        sourceIndex = SourceNameStrToIndex(serviceCfg.source[i].sourceName.c_str());

        result = CheckSourceTime(&time, sourceIndex);

        if (result != LE_OK)
        {
            LE_DEBUG("Get %s time source failed\n", serviceCfg.source[i].sourceName.c_str());
            continue;
        }
        // Set the bit map for the available time source
        TimeSourceStatusMap = TimeSourceStatusMap | (1 << sourceIndex);
        if (sourceIndex == TAF_TIME_SRC_NAME_EX_APP
            || sourceIndex == TAF_TIME_SRC_NAME_RTC)
        {
            //ExAPP or RTC set time successful out of this function loop
            setStatus = true;
        }
        else if (!setStatus && isNewTimeSrcSetTimeAllowed(sourceIndex))
        {
            LE_DEBUG("Setting the new time with source %s",
                SourceNameIndexToStr(sourceIndex));
            result = SetSystemTime(time, sourceIndex, false);
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
void taf_Time::SystemTimeUpdateTimerHandler
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

void taf_Time::InitializeSystemTimeAttr(le_result_t connectStatus)
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
    if(connectStatus == LE_OK)
    {
        LatestTimeSourceInfo->secStrgdataRef =
        taf_mngdStorSec_CreateData(SourceNameIndexToStr(LatestTimeSourceInfo->sourceId));
        if(LatestTimeSourceInfo->secStrgdataRef == NULL)
        {
            LatestTimeSourceInfo->secStrgdataRef =
                taf_mngdStorSec_GetDataRef(SourceNameIndexToStr(LatestTimeSourceInfo->sourceId));
        }
    }
    LatestTimeSourceInfo->sessionRef = taf_time_GetClientSessionRef();
    LatestTimeSourceInfo->ref = (taf_time_SourceRef_t)le_ref_CreateRef(tafTime.SrcRefMap, LatestTimeSourceInfo);
    return;
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
    le_result_t connectStatus = taf_mngdStorSec_TryConnectService();
    le_result_t regGnssTimeStatus = LE_UNAVAILABLE;
    le_result_t regNetworkTimeStatus = LE_UNAVAILABLE;
    long int interval;
    taf_Time& tafTime = taf_Time::GetInstance();
    // Create reference maps.
    tafTime.TimeRefMap = le_ref_CreateMap("TimeRefMap", TAF_TIME_SRC_NAME_UNKNOWN);
    tafTime.TimePool = le_mem_CreatePool("Time Pool", sizeof(taf_TimeInf_t));
    tafTime.SrcRefMap = le_ref_CreateMap("SrcRefMap", TAF_TIME_SRC_NAME_UNKNOWN + 1);
    tafTime.SrcPool = le_mem_CreatePool("Available Source Pool", sizeof(taf_SourceInf_t));

    //Initialise system time structure and register in the SrcRefMap
    tafTime.InitializeSystemTimeAttr(connectStatus);

    //Populate the available source information
    if (TimeSourceConf.source.size() > 0)
    {
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
            src->sessionRef = taf_time_GetClientSessionRef();
            if(connectStatus == LE_OK)
            {
                src->secStrgdataRef =
                taf_mngdStorSec_CreateData(tafTime.SourceNameIndexToStr(src->sourceId));
                if(src->secStrgdataRef == NULL)
                {
                    src->secStrgdataRef =
                        taf_mngdStorSec_GetDataRef(tafTime.SourceNameIndexToStr(src->sourceId));
                }
            }
            src->ref = (taf_time_SourceRef_t)le_ref_CreateRef(tafTime.SrcRefMap, src);
        }
    }
    tafTime.printSourceInfo();

    tafTime.gptpTimeRef = taf_gptpTime_CreateRef(TimeSourceConf.gptpDeviceName.c_str());

    // Check if the network time source is required
    if (TimeSourceConf.IsSourceExist(tafTime.SourceNameIndexToStr(TAF_TIME_SRC_NAME_NETWORK)))
    {
        tafTime.InitNetworkTimeStatus = tafTime.InitNetworkTime();
        if (tafTime.InitNetworkTimeStatus == LE_OK)
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
    // Check if the GNSS time source is required
    if (TimeSourceConf.IsSourceExist(tafTime.SourceNameIndexToStr(TAF_TIME_SRC_NAME_GNSS)))
    {
        tafTime.InitGnssTimeStatus = tafTime.InitGnssTime();

        if (tafTime.InitGnssTimeStatus == LE_OK)
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
    if (!TimeSourceConf.source.empty())
    {
        if (TimeSourceConf.pollingInterval <= 0)
        {
            TimeSourceConf.pollingInterval = TAF_TIME_SECOND_PER_LOOP_DEFAULT;
        }
        //Update the system time as quickly as possible.
        SystemTimeUpdateTimerHandler(NULL);
        // Create timer to update the system time
        LE_INFO("Starting sync time timer, interval: %ld sec\n", TimeSourceConf.pollingInterval);
        tafTime.sysTimeUdTimerRef = le_timer_Create("sysTimeUpdateTimer");
        le_timer_SetMsInterval(tafTime.sysTimeUdTimerRef, (TimeSourceConf.pollingInterval)*1000);
        le_timer_SetRepeat(tafTime.sysTimeUdTimerRef, 0);
        le_timer_SetHandler(tafTime.sysTimeUdTimerRef, SystemTimeUpdateTimerHandler);
        le_timer_SetWakeup(tafTime.sysTimeUdTimerRef, false);
        le_timer_Start(tafTime.sysTimeUdTimerRef);
    }
    else
    {
        LE_WARN("No time source found\n");
    }

    le_event_RunLoop();

    LE_WARN("Warning: SyncTimeTasks exit!\n");
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

    if (tafTime.InitGnssTimeStatus == LE_OK)
    {
        tafTime.RegGnssTimeListener();
    }

    if (tafTime.InitNetworkTimeStatus == LE_OK)
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
    if(phoneId == 1)
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
        tafTime.SourceAvailabilityUpdate(LE_FAULT, sourceId);
        return;
    }

    result = ConvertNetworkTimeToSec(info, &timeVal);
    if (LE_OK == result)
    {
        UpdateFailedLoops(sourceId, FAIL_LOOP_NUM_CLEAN);
        UpdateLocalTimeCache(timeVal, sourceId, NetworkDeltaTime);
        tafTime.SourceAvailabilityUpdate(result, sourceId);
    }

    StoreDateTimeInfo(info, sourceId);

    if (initFlag)
    {
        //Report a event to client for the first time initialization.
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
    taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)SearchAvailableSourceInfList(sourceIndex);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, LE_FAULT, "Source Reference is not found!");

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
    telux::common::Status ret;

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
                ret = tafTime.servingSystemManagers[i]->requestNetworkTime(
                                                    tafTime.SyncNetworkTimeResponse);
            }
            else
            {
                //Since currently only support 2 phones, so all other phones (if any)
                //will report to phone 2.
                ret = tafTime.servingSystemManagers[i]->requestNetworkTime(
                                                   tafTime.SyncNetworkTimeResponse2);
            }
            if (ret == telux::common::Status::SUCCESS)
            {
                LE_DEBUG("Total phones: %zu, synching network time from phone %zu\n",
                                          tafTime.servingSystemManagers.size(), i+1);
                break;
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

        LE_DEBUG("Trying to Register the servSysListener, size: %zu\n",
                                                       servingSystemManagers.size());
        auto servSysListener = std::make_shared<taf_TimeServingSystemListener>(
                                phoneManager->getPhoneIdFromSlotId(i+1));

        status = tafTime.servingSystemManagers[i]->registerListener(servSysListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Failed to register serving system listener.");
            if (result != LE_OK)
            {
                //Not to set fail if one of the registration is ok.
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

    for (size_t i = 0; i < tafTime.servingSystemManagers.size(); i++)
    {
        if (tafTime.servingSystemManagers[i] != nullptr
            && tafTime.servSysListeners[i] != nullptr)
        {
            tafTime.servingSystemManagers[i]->deregisterListener(
                tafTime.servSysListeners[i]);
        }
    }
    tafTime.servSysListeners.clear();
}

//--------------------------------------------------------------------------------------------------
/**
 * Network time initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitNetworkTime(void)
{
    le_result_t result;
    auto& tafTime = taf_Time::GetInstance();

    NetworkDeltaTimePool = le_mem_CreatePool("NetworkDeltaTime ",
                                                         sizeof(taf_time_TimeSpec_t));
    NetworkDeltaTime = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(NetworkDeltaTimePool);
    memset(NetworkDeltaTime, 0, sizeof(taf_time_TimeSpec_t));
    result = ReadWriteDeltaTime(NetworkDeltaTime, 0, TAF_TIME_DATA_CLEAN);
    if (result != LE_OK)
    {
        LE_ERROR("Clean network delta time failed\n");
        return result;
    }

    NetworkDeltaTime2Pool = le_mem_CreatePool("NetworkDeltaTime2 ",
                                                         sizeof(taf_time_TimeSpec_t));
    NetworkDeltaTime2 = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(NetworkDeltaTime2Pool);
    memset(NetworkDeltaTime2, 0, sizeof(taf_time_TimeSpec_t));
    result = ReadWriteDeltaTime(NetworkDeltaTime2, 0, TAF_TIME_DATA_CLEAN);
    if (result != LE_OK)
    {
        LE_ERROR("Clean network delta time failed\n");
        return result;
    }

    netTimeInfoPool = le_mem_CreatePool("netTimeInfoPool", sizeof(taf_TimeNetTimeInfo_t));
    netTimeInfoRefMap = le_ref_CreateMap("netTimeInfoRefMap", DEFAULT_PHONE_NUM_MAX);

    TsrEventPool = le_mem_CreatePool("TsrEventPool", sizeof(TimeSourceRef_Event_t));
    TsrEventMap = le_ref_CreateMap("TsrEventMap", DEFAULT_TSR_EVENT_CNT);
    // Create the event and add event handler.
    RefTimeEventId = le_event_CreateId("RefTimeEventId", sizeof(TS_Event_t));
    RefTimeEventHandlerRef = le_event_AddHandler("RefTimeEventHandlerRef",
                                    RefTimeEventId, tafTime.EventTimeValueChangeHandler);

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
                    //Add serving system manager to vector using phoneId index as
                    //vector's index
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
        LE_ERROR("Fail to init telephony subsystem");
        return result;
    }

    LE_INFO("Init telephony subsystem successful");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * GNSS time initialization.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::InitGnssTime(void)
{
    le_result_t result;
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

    GnssDeltaTimePool = le_mem_CreatePool("TimeSvc GnssDeltaTime ", sizeof(taf_time_TimeSpec_t));
    GnssDeltaTime = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(GnssDeltaTimePool);

    memset(GnssDeltaTime, 0, sizeof(taf_time_TimeSpec_t));
    result = ReadWriteDeltaTime(GnssDeltaTime, 0, TAF_TIME_DATA_CLEAN);
    if (result)
    {
        LE_WARN("Clean GNSS delta time failed\n");
    }
    return LE_OK;
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
        if (tafTime.InitNetworkTimeStatus == LE_OK)
        {
            tafTime.RegNetworkTimeListener();
        }
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_DEBUG("Power state change to SUSPEND");
        if (tafTime.InitNetworkTimeStatus == LE_OK)
        {
            tafTime.DeregNetworkTimeListener();
        }

        if (tafTime.InitGnssTimeStatus == LE_OK)
        {
            tafTime.DeregGnssTimeListener();
        }
    }
}

bool taf_Time::isNewTimeSrcSetTimeAllowed(taf_time_TimeSources_t newTimeSource)
{
    int position = TimeSourceConf.findSourcePosition(SourceNameIndexToStr(newTimeSource));
    if (position < 0)
    {
        LE_ERROR("%s is not found\n", SourceNameIndexToStr(newTimeSource));
        return false;
    }

    if (!TimeSourceConf.source[position].setSystemTime)
    {
        LE_DEBUG("Set time configuration 'SetTime' was not 'true'.");
        return false;
    }

    if (LatestTimeSourceInfo->systemSourceId == TAF_TIME_SRC_NAME_UNKNOWN)
    {
        // Current system time was not sync by any of the time source, need to be sync
        return true;
    }

    int newPriorityNum = TimeSourceConf.source[position].priority;

    position =
        TimeSourceConf.findSourcePosition(SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId));
    if (position < 0)
    {
        LE_ERROR("%s is not found\n", SourceNameIndexToStr(LatestTimeSourceInfo->systemSourceId));
        return false;
    }
    int currPriorityNum = TimeSourceConf.source[position].priority;

    LE_DEBUG("currPriorityNum %d, newPriorityNum %d, AllowOverrideAfterFail %" PRIu64 "\n",
        currPriorityNum, newPriorityNum, AllowOverrideAfterFail);

    // Note, the small priority number will have higher priority
    if (newPriorityNum <= currPriorityNum)
    {
        //2. Then new time source has higher priority, the update is acceptable
        return true;
    }

    if (newPriorityNum > currPriorityNum && AllowOverrideAfterFail == 0)
    {
        //3. Curr time source has higher priority and was timeout,
        //   the system time need to be updated
        return true;
    }

    return false;
}


void taf_Time::getRTCRespCB(struct TimeSpec timeVal, le_result_t response)
{
    auto& time = taf_Time::GetInstance();

    taf_time_TimeSpec_t timeSpec = { 0 };
    le_result_t result = LE_TERMINATED;
    taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_RTC;

    timeSpec.sec = timeVal.sec;
    timeSpec.nanosec = timeVal.nanosec;

    if (getRTCCB.getRTCCallbackFunc)
    {
        getRTCCB.getRTCCallbackFunc(&timeSpec, response, getRTCCB.getRTCCtxPtr);
    }

    if (response != LE_OK || timeSpec.sec == 0)
    {
        LE_ERROR("Response for getting RTC time is NOT OK %d, time is: %" PRIu64 "\n",
        response, timeSpec.sec);
        time.SourceAvailabilityUpdate(LE_FAULT, sourceId);
        return;
    }

    time.UpdateFailedLoops(sourceId, FAIL_LOOP_NUM_CLEAN);
    time.SourceAvailabilityUpdate(response, sourceId);

    if (time.isNewTimeSrcSetTimeAllowed(TAF_TIME_SRC_NAME_RTC))
    {
        result = time.SetSystemTime(timeSpec, TAF_TIME_SRC_NAME_RTC, false);
        if (result != LE_OK)
        {
            LE_DEBUG("Set RTC to system failed %d\n", result);
            return;
        }
    }

    return;
}

void taf_Time::setRTCRespCB(le_result_t response)
{
    if (response != LE_OK)
    {
        LE_DEBUG("Set RTC response not ok %d\n", response);
    }

    if (setRTCCB.setRTCCallbackFunc)
    {
        setRTCCB.setRTCCallbackFunc(response, setRTCCB.setRTCCtxPtr);
    }

    return;
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

le_result_t taf_Time::GetRtcTimeReqAsync(taf_time_AsyncGetTimeReqHandlerFunc_t handlerPtr,
    void* contextPtr)
{
    le_result_t result = LE_UNSUPPORTED;

    if (isDrvPresent)
    {
        if ((timeInf == nullptr) || ((*(timeInf->getRtcTimeReqAsync)) == nullptr))
        {
            LE_ERROR("getRtcTimeHAL not initialized - Async");
            return LE_FAULT;
        }

        (telux::tafsvc::taf_Time::getRTCCB).getRTCCallbackFunc = handlerPtr;
        (telux::tafsvc::taf_Time::getRTCCB).getRTCCtxPtr = contextPtr;
        (telux::tafsvc::taf_Time::getRTCCB).sessionRef = taf_time_GetClientSessionRef();

        result = (*(timeInf->getRtcTimeReqAsync))(taf_Time::getRTCRespCB);
    }
    else
    {
        LE_DEBUG("GetRtcTimeReqAsync not supported");
        return LE_UNSUPPORTED;
    }

    return result;
}

le_result_t taf_Time::SetRtcTimeReqAsync(const taf_time_TimeSpec_t* timeValPtr,
    taf_time_AsyncSetTimeReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    le_result_t result = LE_UNSUPPORTED;

    if (isDrvPresent)
    {
        if ((timeInf == nullptr) || ((*(timeInf->setRtcTimeReqAsync)) == nullptr))
        {
            LE_ERROR("setRtcTimeReqAsync not initialized");
            return LE_FAULT;
        }

        (telux::tafsvc::taf_Time::setRTCCB).setRTCCallbackFunc = handlerPtr;
        (telux::tafsvc::taf_Time::setRTCCB).setRTCCtxPtr = contextPtr;
        (telux::tafsvc::taf_Time::setRTCCB).sessionRef = taf_time_GetClientSessionRef();

        struct TimeSpec timeSpec;
        timeSpec.sec = timeValPtr->sec;
        timeSpec.nanosec = timeValPtr->nanosec;

        result = (*(timeInf->setRtcTimeReqAsync))(&timeSpec, taf_Time::setRTCRespCB);
    }
    else
    {
        LE_DEBUG("SetRtcTimeReqAsync not supported");
        return LE_UNSUPPORTED;
    }

    return result;
}

void taf_Time::printSourceInfo()
{
    auto& time = taf_Time::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(time.SrcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_GetValue(iterRef);
        LE_INFO("Source ID: %s", time.SourceNameIndexToStr(sourcePtr->sourceId));
        LE_INFO("Failed Loop: %d", sourcePtr->failedLoops);

        LE_INFO("Reference is: %p", sourcePtr->ref);
        if (sourcePtr->isAvailable == true)
        {
            LE_INFO("Source is available");
        }
        else
        {
            LE_INFO("Source is NOT available");
        }
        if (sourcePtr->handlerRef != NULL)
        {
            LE_INFO("Handler Reference is: %p", sourcePtr->handlerRef);
        }
        else
        {
            LE_INFO("Handler Reference does not exit");
        }
        if (sourcePtr->handlerFunc != NULL)
        {
            LE_INFO("Handler Function is: %p", sourcePtr->handlerFunc);
        }
        else
        {
            LE_INFO("Function does not exit");
        }

    }
}

taf_SourceInf_t* taf_Time::SearchAvailableSourceInfList(taf_time_TimeSources_t sourceId)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SrcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)le_ref_GetValue(iterRef);
        if ((srcTimePtr != NULL)
            && (srcTimePtr->sourceId == sourceId))
        {
            return srcTimePtr;
        }
    }
    return NULL;
}

taf_time_SourceRef_t taf_Time::GetSourceRef
(
    taf_time_TimeSources_t sourceId
)
{
    taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)SearchAvailableSourceInfList(sourceId);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, NULL, "Source Reference is not found!");

    LE_INFO("Source Ref %p for client, sourceId (0x%x).",
        srcTimePtr->ref, sourceId);

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

    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)SearchAvailableSourceInfList(TAF_TIME_SRC_NAME_SYSTEM);

    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Time Source is not registered.");

    *timeSource = sourcePtr->systemSourceId;
    return LE_OK;
}

void timeSourceStatusHandler(void* reportPtr)
{
    auto& time = taf_Time::GetInstance();
    SourceStatusChange_Event_t* evt = (SourceStatusChange_Event_t*)reportPtr;
    taf_SourceInf_t* sourcePtr = evt->sourcePtr;
    bool isAvailable = evt->status;
    taf_time_StatusEventType_t eventType = evt->eventType;

    if (sourcePtr != NULL
        && sourcePtr->handlerRef != NULL
        && sourcePtr->handlerFunc != NULL)
    {
        if (isAvailable)
        {
            LE_DEBUG("Time source %s is Available!",
                time.SourceNameIndexToStr(sourcePtr->sourceId));
        }
        else
        {
            LE_DEBUG("Time source %s is NOT Available!",
                time.SourceNameIndexToStr(sourcePtr->sourceId));
        }
        sourcePtr->handlerFunc(sourcePtr->ref, eventType, isAvailable, sourcePtr->context);
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
    TAF_ERROR_IF_RET_VAL(srcRef == NULL, NULL, "Source Reference is NULL.");

    taf_SourceInf_t* srcTimePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, srcRef);

    TAF_ERROR_IF_RET_VAL(srcTimePtr == NULL, NULL, "Source Reference not found!.");

    TAF_ERROR_IF_RET_VAL((srcTimePtr->sourceId == TAF_TIME_SRC_NAME_SYSTEM && statusEventType == TAF_TIME_STATUS_EVENT_AVAILABILITY),
    NULL, "Cannot register handler for source(0x%x). Not Supported", srcTimePtr->sourceId);

    if (statusEventType == TAF_TIME_EVENT_TYPE_LOWER_BOUND ||
       statusEventType > TAF_TIME_EVENT_TYPE_UPPER_BOUND)
    {
        LE_ERROR("Please provide a valid event type.");
        return NULL;
    }

    if (srcTimePtr->handlerFunc != NULL)
    {
        return srcTimePtr->handlerRef;
    }

    srcTimePtr->handlerFunc = handlerFuncPtr;
    srcTimePtr->context = contextPtr;
    srcTimePtr->eventType = statusEventType;

    srcTimePtr->handlerRef =
        (taf_time_TimeSourceStatusHandlerRef_t)le_ref_CreateRef(SrcRefMap, srcTimePtr);

    LE_INFO("Registering handler reference %p for %s", srcTimePtr->handlerRef,
        SourceNameIndexToStr(srcTimePtr->sourceId));
    return srcTimePtr->handlerRef;
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
        LE_ERROR("Invalid tsrHandlerRef(%p).", handlerRef);
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
    return sourcePtr->sourceValidity;
}

le_result_t taf_Time::CheckSetValidityPermission()
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
    le_result_t connectStatus = taf_mngdStorSec_TryConnectService();
    if(connectStatus != LE_OK)
    {
        return LE_FAULT;
    }
    le_result_t res = LE_FAULT;
    taf_mngdStorSec_DataRef_t dataRef = sourcePtr->secStrgdataRef;
    uint8_t validityToSet = newvalidity == true ? 1 : 0;
    if(dataRef == NULL)
    {
        LE_DEBUG("Unable to get data reference for storing validity in secure storage");
        return LE_NOT_FOUND;
    }
    res = taf_mngdStorSec_WriteDataStart(dataRef);
    if(res != LE_OK)
    {
        LE_DEBUG("Cannot start writing validity in secure storage.");
        return res;
    }
    res = taf_mngdStorSec_WriteDataChunk(dataRef, &validityToSet, sizeof(validityToSet));

    if(res != LE_OK)
    {
        LE_DEBUG("Cannot write validity in secure storage.");
        return res;
    }
    res = taf_mngdStorSec_WriteDataEnd(dataRef);

    if(res != LE_OK)
    {
        LE_DEBUG("Cannot end writing validity in secure storage.");
        return res;
    }
    return res;
}

le_result_t taf_Time::ReadValidityFromSecStorage(taf_SourceInf_t* sourcePtr, bool* validity)
{
    le_result_t connectStatus = taf_mngdStorSec_TryConnectService();
    if(connectStatus != LE_OK)
    {
        return LE_FAULT;
    }
    le_result_t res;
    taf_mngdStorSec_DataRef_t dataRef = sourcePtr->secStrgdataRef;
    if(dataRef == NULL)
    {
        LE_WARN("Unable to read validity in secure storage.No ref available for time source");
        return LE_NOT_FOUND;
    }

    uint8_t readBuf;
    size_t readLen = sizeof(readBuf);

    res = taf_mngdStorSec_ReadDataFirstChunk(dataRef, &readBuf, &readLen);
    if(res != LE_OK)
    {
        LE_WARN("Failed to read validity from secure storage.");
        return res;
    }
    *validity = readBuf == 1 ? true : false;

    LE_INFO("readLen = %" PRIuS, readLen);
    return res;
}

le_result_t taf_Time::SetValidity
(
    taf_time_SourceRef_t sourceRef,
    bool newvalidity
)
{
    TAF_ERROR_IF_RET_VAL(sourceRef == NULL, LE_FAULT, "Source Reference is NULL.");
    taf_SourceInf_t* sourcePtr = (taf_SourceInf_t*)le_ref_Lookup(SrcRefMap, sourceRef);
    TAF_ERROR_IF_RET_VAL(sourcePtr == NULL, LE_FAULT, "Source Reference is not registered.");

    if (sourcePtr->sourceId != TAF_TIME_SRC_NAME_RTC &&
        sourcePtr->sourceId != TAF_TIME_SRC_NAME_EX_APP )
    {
        LE_ERROR("Not allowed to change validity for this time source!");
        return LE_BAD_PARAMETER;
    }

    //Check if the client can change the validity of time source or not
    le_result_t isClientValid = CheckSetValidityPermission();
    if(isClientValid != LE_OK)
    {
        LE_ERROR("Client is not allowed to change source validity.");
        return LE_BAD_PARAMETER;
    }
    //Check if the validity is changed and trigger notification accordingly
    bool oldValidity = sourcePtr->sourceValidity;

    if(oldValidity != newvalidity)
    {
        WriteValidtyToSecStorage(sourcePtr, newvalidity);
        sourcePtr->sourceValidity = newvalidity;
        if(sourcePtr->handlerFunc != NULL &&
           (sourcePtr->eventType & TAF_TIME_STATUS_EVENT_VALIDITY) != 0)
        {
            ReportValidityChange(sourcePtr);
        }
    }
    if(sourcePtr->sourceId == TAF_TIME_SRC_NAME_EX_APP &&
       LatestTimeSourceInfo->systemSourceId == TAF_TIME_SRC_NAME_EX_APP &&
       LatestTimeSourceInfo->sourceValidity != sourcePtr->sourceValidity)
    {
        LatestTimeSourceInfo->sourceValidity = sourcePtr->sourceValidity;
        WriteValidtyToSecStorage(LatestTimeSourceInfo, LatestTimeSourceInfo->sourceValidity);
    }
    return LE_OK;
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

    mainThreadRef = le_thread_GetCurrent();

}

taf_time_setRTCCb_t telux::tafsvc::taf_Time::setRTCCB;
taf_time_getRTCCb_t telux::tafsvc::taf_Time::getRTCCB;
