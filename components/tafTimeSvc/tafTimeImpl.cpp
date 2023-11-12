/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

using namespace telux::tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Counter for receiving GNSS UTC report.
 */
//--------------------------------------------------------------------------------------------------
static int gnss_counter = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Object to store time source configuration.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_TIME_MAX_SOURCE_NUMBER (TAF_TIME_SRC_NAME_UNKNOWN*3)
TimeSources TimeSourceConf(TAF_TIME_MAX_SOURCE_NUMBER);

//--------------------------------------------------------------------------------------------------
/**
 * Lock and condition used for time manager.
 */
//--------------------------------------------------------------------------------------------------
std::mutex mtx;
std::condition_variable cv;

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

    if (utc == 0) {
        LE_WARN("Received wrong UTC time\n");
        return;
    }

    if (gnss_counter > 1 && utc % 1000 == 0)
     {
        timeVal.sec = (utc / 1000);
        timeVal.nanosec = (utc % 1000)*1000*1000;

        gnss_counter--;
        tafTime.UpdateLocalGnssTime(timeVal);
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

        /* Add new time source here */

        case TAF_TIME_SRC_NAME_UNKNOWN:
            return "unknown";

    }

    return "unknown";
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
    const json_t *serviceData
)
{
    le_result_t result = LE_NOT_FOUND;//Not found is allowed
    json_t *arrayData,*itemData, *sourceCfgArray;
    const char* value;
    int priority;
    int i, j;
    long int arraySize = 0;

    sourceCfgArray = json_object_get(serviceData, TAF_TIME_SERVICE_SOURCE_STR);
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
        LE_WARN("Source number: %ld, but we only accept: %d\n", arraySize, TAF_TIME_MAX_SOURCE_NUMBER);
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
    const json_t *serviceData
)
{
    json_t *itemData;
    const char* value;
    long int pollingInterval, toleranceMillsec;

    itemData = json_object_get(serviceData, TAF_TIME_INTERVAL_SETTING_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: PollingInterval string was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    sscanf(value,"%ld", &pollingInterval);
    serviceCfg.pollingInterval = pollingInterval;

    itemData = json_object_get(serviceData, TAF_TIME_TOLERANCES_SETTING_STR);
    if (!json_is_string(itemData))
    {
        LE_WARN("Warning: Tolerances string was not found\n");
        return LE_NOT_FOUND;
    }

    value = json_string_value(itemData);
    sscanf(value,"%ld", &toleranceMillsec);
    serviceCfg.toleranceMillsec = toleranceMillsec;

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
    json_t *root, *serviceData;
    json_error_t error;

    root = json_load_file(filePathPtr, 0, &error);
    if (root == NULL)
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

    serviceData = json_object_get(root, TAF_TIME_SERVICE_HEADER_STR);
    if (!json_is_object(serviceData))
    {
        LE_WARN("Warning: Cannot get the TimeService configuration\n");
        json_decref(root);
        return result;
    }

    result = ReadSourceConf(serviceCfg, serviceData);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Cannot read time source configuration\n");
        json_decref(root);
        return result;
    }

    result = ReadTimeConf(serviceCfg, serviceData);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Cannot read time configuration\n");
        json_decref(root);
        return result;
    }

    json_decref(root);
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
 * Maintain the buffer for GNSS local time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::ReadWriteDeltaTime
(
    taf_time_TimeSpec_t* timeVal,
    taf_TimeReadWrite_t ReadWriteType
)
{
    le_result_t result = LE_OK;

    pthread_mutex_lock(&ProtectlocalTime_mutex);

    switch (ReadWriteType)
    {
        case TAF_TIME_DATA_READ:
            // Copy local maintained data 'DeltaTimeData' to buf 'timeVal'
            memcpy(timeVal, DeltaTimeData, sizeof(taf_time_TimeSpec_t));
            break;

        case TAF_TIME_DATA_UPDATE:
            // Update new time to local buffer 'DeltaTimeData'.
            memcpy(DeltaTimeData, timeVal, sizeof(taf_time_TimeSpec_t));
            break;

        case TAF_TIME_DATA_CLEAN:
            // Clean local buffer
            memset(timeVal, 0, sizeof(taf_time_TimeSpec_t));
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
le_result_t taf_Time::UpdateLocalGnssTime
(
    taf_time_TimeSpec_t newGnssTime
)
{
    le_result_t result;
    uint64_t deltaMilliSec, milliSecThreshold;
    taf_time_TimeSpec_t bootTime, oldDeltaTime, newDeltaTime, largerDtTime;

    oldDeltaTime.sec = 0;
    oldDeltaTime.nanosec = 0;

    if (newGnssTime.sec <= 0)
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

    result = ReadWriteDeltaTime(&oldDeltaTime, TAF_TIME_DATA_READ);
    if (result)
    {
        LE_ERROR("Read detla time failed\n");
        return result;
    }
    newDeltaTime = taf_time_Sub(newGnssTime, bootTime);
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
        result =  ReadWriteDeltaTime(&newDeltaTime, TAF_TIME_DATA_UPDATE);
        if (result)
        {
            LE_ERROR("Update detla time failed\n");
            return result;
        }

        LE_INFO("Update GNSS time to: sec %ld , nsec %ld. DT milliSec %ld, Threshold %ld\n",
            newGnssTime.sec, newGnssTime.nanosec, deltaMilliSec, milliSecThreshold);
    }

    return LE_OK;
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
    taf_time_TimeSpec_t* timeVal
)
{
    struct timespec bootTime;

    if ( clock_gettime(CLOCK_BOOTTIME, &bootTime) < 0 )
    {
        return LE_FAULT;
    }
    timeVal->sec = bootTime.tv_sec;
    timeVal->nanosec = bootTime.tv_nsec;
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
    taf_time_TimeSpec_t* timeVal
)
{
    struct timespec systemTime;

    if ( clock_gettime(CLOCK_REALTIME, &systemTime) < 0 )
    {
        return LE_FAULT;
    }

    timeVal->sec = systemTime.tv_sec;
    timeVal->nanosec = systemTime.tv_nsec;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get local maintained GNSS time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_NOT_FOUND -- If no data available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::GetGnssTime
(
    taf_time_TimeSpec_t* timeVal
)
{
    le_result_t result;
    taf_time_TimeSpec_t bootTime, deltaTime;

    result = GetBootTime(&bootTime);
    if (result)
    {
        LE_ERROR("Unable to read system boot time\n");
        return result;
    }

    result = ReadWriteDeltaTime(&deltaTime, TAF_TIME_DATA_READ);
    if (result)
    {
        LE_ERROR("Read detla time failed\n");
        return result;
    }

    if (DeltaTimeData->sec <= 0)
    {
        LE_WARN("GNSS time not ready\n");
        return LE_NOT_FOUND;
    }

    LE_DEBUG("GetGnssTime: dt sec %ld , nsec %ld, Dt sec %ld , nsec %ld\n",
        deltaTime.sec, deltaTime.nanosec, DeltaTimeData->sec, DeltaTimeData->nanosec);

    *timeVal = taf_time_Add(bootTime, deltaTime);
    return result;
}

le_result_t taf_Time::GetRtcTime
(
    taf_time_TimeSpec_t* timeVal
)
{
    LE_ERROR("Unsupported function called - %s\n", __func__);
    return LE_NOT_IMPLEMENTED;
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
        return LE_OK;
    }
    return LE_TIMEOUT;
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
    le_result_t result = LE_OK;
    int position = 0;
    uint64_t deltaMilliSec, milliSecThreshold;
    taf_time_TimeSpec_t systemTime, tmpTime;
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
        LE_DEBUG("Set time not need. delta %ld, threshold: %ld\n",
            deltaMilliSec, milliSecThreshold);
        return LE_OK;
    }
    LE_INFO("Update sys time to: sec %ld , nsec %ld, ExApp: %d. old: sec %ld , nsec %ld\n",
            timeVal.sec, timeVal.nanosec, ackTimeSvc, systemTime.sec, systemTime.nanosec);

    newTime.tv_sec = timeVal.sec;
    newTime.tv_nsec = timeVal.nanosec;
    if (clock_settime(CLOCK_REALTIME, &newTime) < 0)
    {
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

    if (SetTimeSt->preActiveTimeSource != timeSource)
    {
        TimeSourceChangeNotify(SetTimeSt->preActiveTimeSource, timeSource);
        SetTimeSt->preActiveTimeSource = timeSource;
    }

    if (ackTimeSvc)
    {
        SetTimeSt->externalSetTime = true;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set time with different time source.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 *     - LE_NOT_PERMITTED -- This service does not have permission.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_NOT_IMPLEMENTED -- Feature not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::SetTime
(
    const char* sourceNameStr,
    taf_time_TimeSources_t sourceIndex
)
{
    le_result_t result = LE_NOT_FOUND;
    taf_time_TimeSpec_t time;

    switch (sourceIndex)
    {
        case TAF_TIME_SRC_NAME_RTC:
            result = GetRtcTime(&time);
            break;

        case TAF_TIME_SRC_NAME_GNSS:
            result = GetGnssTime(&time);
            break;

        case TAF_TIME_SRC_NAME_EX_APP:
            return GetExSetTimeStatus();

        default:
            result = LE_NOT_FOUND;
            break;
    }

    if (result)
    {
        LE_ERROR("Get %s time source failed\n", sourceNameStr);
        return result;
    }

    result = SetSystemTime(time, sourceIndex, false);
    if (result)
    {
        LE_ERROR("Set %s time to system failed\n", sourceNameStr);
        return result;
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
    LE_INFO("TimeSourceChange Old: %s, New: %s\n",
            SourceNameIndexToStr(PreTimeSource), SourceNameIndexToStr(NewTimeSource));

    taf_TimeSourceStatus_t* statusPtr =
       (taf_TimeSourceStatus_t*)le_mem_ForceAlloc(timeSourceChangePool);
    statusPtr->preSource = PreTimeSource;
    statusPtr->newSource = NewTimeSource;
    le_event_ReportWithRefCounting(timeSourceChangeId, (void*)statusPtr);
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
    taf_time_TimeSources_t* latestActiveTime
)
{
    le_result_t result = LE_UNAVAILABLE;
    taf_time_TimeSources_t sourceName;
    int i;

    if (serviceCfg.source.empty())
    {
        LE_ERROR("Time source configuration not found\n");
        return LE_BAD_PARAMETER;
    }

    for (i = 0; i < (int)serviceCfg.source.size(); i++)
    {
        if (serviceCfg.source[i].setSystemTime)
        {
            sourceName = SourceNameStrToIndex(serviceCfg.source[i].sourceName.c_str());
            result = SetTime(serviceCfg.source[i].sourceName.c_str(), sourceName);
            if (result == LE_OK)
            {
                //Record the successed source
                *latestActiveTime = sourceName;
                LE_DEBUG("latestActiveTime: %d, %s\n", sourceName,
                                        serviceCfg.source[i].sourceName.c_str());
                break;
            }
        }

        LE_DEBUG("Current source: %s, Set flag: %d, Tatol source: %ld, result: %d\n",
                                        serviceCfg.source[i].sourceName.c_str(),
                                        serviceCfg.source[i].setSystemTime,
                                        serviceCfg.source.size(), result);
    }

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
 *     - LE_NOT_FOUND -- Time source not available.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::SystemTimeUpdateTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    taf_Time& tafTime = taf_Time::GetInstance();
    le_result_t result = LE_NOT_FOUND;
    taf_time_TimeSources_t latestActiveTime = TAF_TIME_SRC_NAME_UNKNOWN;


    result = tafTime.SetTimeBaseOnConfig(TimeSourceConf, &latestActiveTime);
    if (result != LE_OK)
    {
        LE_WARN("Warning: Sync time failed, will try again after %ld seconds\n",
                                                TimeSourceConf.pollingInterval);
    }

    if (latestActiveTime == TAF_TIME_SRC_NAME_UNKNOWN
        && tafTime.SetTimeSt->preActiveTimeSource == TAF_TIME_SRC_NAME_UNKNOWN)
    {
        // No available time source
        tafTime.TimeSourceChangeNotify(tafTime.SetTimeSt->preActiveTimeSource, latestActiveTime);
    }

    if (tafTime.SetTimeSt->preActiveTimeSource != latestActiveTime)
    {
        tafTime.SetTimeSt->preActiveTimeSource = latestActiveTime;
    }
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
    le_result_t result;
    taf_Time& tafTime = taf_Time::GetInstance();

    // Check if the GNSS time source is required
    if (TimeSourceConf.IsSourceExist(tafTime.SourceNameIndexToStr(TAF_TIME_SRC_NAME_GNSS)))
    {
        result= tafTime.RegGnssSyncTimeTask();
        if (result == LE_OK)
        {
            // Create timer to update the local GNSS time
            LE_INFO("Starting GNSS time update timer, interval: %d millisec\n",
                                                TAF_TIME_SYNC_GNSS_TIME_INTERVAL);
            tafTime.syncGnssTimerRef = le_timer_Create("SyncGnssTimer");
            le_timer_SetMsInterval(tafTime.syncGnssTimerRef, TAF_TIME_SYNC_GNSS_TIME_INTERVAL);
            le_timer_SetRepeat(tafTime.syncGnssTimerRef, 0);
            le_timer_SetHandler(tafTime.syncGnssTimerRef, GetGnssTimeTimerHandler);
            le_timer_SetWakeup(tafTime.syncGnssTimerRef, false);
            le_timer_Start(tafTime.syncGnssTimerRef);
        }
        else
        {
            LE_WARN("Warning: RegGnssSyncTimeTask failed\n");
        }

    }

    if (!TimeSourceConf.source.empty())
    {
        if (TimeSourceConf.pollingInterval <= 0)
        {
            TimeSourceConf.pollingInterval = TAF_TIME_SECOND_PER_LOOP_DEFAULT;
        }

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
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for time source status change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::LayerTimeSourceChangeHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_time_TimeSourceChangeHandlerFunc_t handlerFunc =
        (taf_time_TimeSourceChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_TimeSourceStatus_t* statusPtr = (taf_TimeSourceStatus_t*)reportPtr;
        handlerFunc(statusPtr->preSource, statusPtr->newSource, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create the time manager.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_UNAVAILABLE -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Time::RegGnssSyncTimeTask(void)
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

    TimeTypeMask mask;
    mask.set(SupportedTimeType::GNSS_UTC_TIME);

    // FIXME: Since register and deregister the listener from ITimeManager will
    // bring memory leak, so only rigister it at the first initialization and
    // then do not exit if no error.

    auto myStatus = timeManager->registerListener(gnssTimeListener, mask);
    if (myStatus != Status::SUCCESS)
    {
        LE_WARN("Failed to register time listener\n");
        return LE_UNAVAILABLE;
    }
    LE_INFO("gnssTimeListener started\n");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * A timer handler to periodically start the GNSS listener according to some condition.
 */
//--------------------------------------------------------------------------------------------------
void taf_Time::GetGnssTimeTimerHandler(le_timer_Ref_t timerRef)
{
    if (gnss_counter > 1)
    {
        LE_WARN("Previous GNSS sync time task not exit\n");
        return;
    }
    else
    {
        gnss_counter = TAF_TIME_RECEIVE_GNSS_TIME_COUNT;
    }

    // TODO: Since register and deregister the listener from ITimeManager will
    // bring memory leak issue, so here not implement the deregister function,
    // need to improve it when this issue get fix.
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
    SetTimeStatusPool = le_mem_CreatePool("TimeSvc SetStatusPool ", sizeof(SetTimeStatus));
    SetTimeSt = (SetTimeStatus *)le_mem_ForceAlloc(SetTimeStatusPool);
    memset(SetTimeSt, 0, sizeof(struct SetTimeStatus));

    DeltaTimeDataPool = le_mem_CreatePool("TimeSvc DeltaTimeData ", sizeof(taf_time_TimeSpec_t));
    DeltaTimeData = (taf_time_TimeSpec_t *)le_mem_ForceAlloc(DeltaTimeDataPool);
    result = ReadWriteDeltaTime(DeltaTimeData, TAF_TIME_DATA_CLEAN);
    if (result)
    {
        LE_WARN("Clean delta time failed\n");
    }

    timeSourceChangePool = le_mem_CreatePool("timeSourceChangePool", sizeof(taf_TimeSourceStatus_t));
    timeSourceChangeId = le_event_CreateIdWithRefCounting("TimeSourceStatus");

    SetTimeSt->preActiveTimeSource = TAF_TIME_SRC_NAME_UNKNOWN;

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

}
