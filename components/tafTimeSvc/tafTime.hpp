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

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

// For reading json configuration file
#include "jansson.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

//For gnss time listener
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>
#include <condition_variable>

#define TAF_TIME_THREAD_STACK_SIZE 0x20000
#define TAF_TIME_SERVICE_CONF_FILE       "tafTimeSvc.json"

#define TAF_TIME_SERVICE_HEADER_STR      "TimeService"
#define TAF_TIME_INTERVAL_SETTING_STR    "PollingInterval"
#define TAF_TIME_TOLERANCES_SETTING_STR  "ToleranceMillsec"
#define TAF_TIME_SERVICE_SOURCE_STR      "Sources"

//--------------------------------------------------------------------------------------------------
/**
 * Macro definition for time source.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_TIME_STR_MAX 16
#define TAF_TIME_SECOND_PER_LOOP_DEFAULT   63
#define TAF_TIME_SECOND_PER_COUNT_DEFAULT  1

#define TAF_TIME_NSEC_PER_SEC             (1000000000)
#define TAF_TIME_THRESHOLD_MILLISEC       (200)

#define TAF_TIME_RECEIVE_GNSS_TIME_COUNT   5
#define TAF_TIME_SYNC_GNSS_TIME_INTERVAL  (29000)


//--------------------------------------------------------------------------------------------------
/**
 * Name space for the service.
 */
//--------------------------------------------------------------------------------------------------

using namespace telux::platform;
using namespace telux::common;


//--------------------------------------------------------------------------------------------------
/**
 * Identifies the type of data read write.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_TIME_DATA_READ,    ///< Read gnss time from buffer
    TAF_TIME_DATA_UPDATE,  ///< Update gnss time to buffer
    TAF_TIME_DATA_CLEAN    ///< Clean the buffer
}
taf_TimeReadWrite_t;

//--------------------------------------------------------------------------------------------------
/**
 * Identifies the time source attribute.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_TIME_CONF_SOURCE,    ///< Time source name
    TAF_TIME_CONF_SETTIME,   ///< Flag to indicate if set time to system or not
    TAF_TIME_CONF_PRIORI,    ///< Source priority

    TAF_TIME_CONF_MAX_ITEM
}
taf_Time_SrcAttr_t;

struct SetTimeStatus{
    bool externalSetTime;  ///< Indicate if the time set externally
    taf_time_TimeSources_t preActiveTimeSource; ///< Previously active time source
};

typedef struct
{
    taf_time_TimeSources_t preSource; ///< Previously working time source
    taf_time_TimeSources_t newSource; ///< New working time source
} taf_TimeSourceStatus_t;


namespace telux
{
    namespace tafsvc
    {
        class Source {
        public:
            int priority;
            bool setSystemTime;
            std::string sourceName;

            Source(int pri, bool flag, const std::string& name) :
                priority(pri), setSystemTime(flag), sourceName(name) {}
        };

        class TimeSources {
        public:
            std::vector<Source> source;
            long int pollingInterval;
            long int toleranceMillsec;
            int sourceArrySize;
            int sourceVectorSize;

            // Constructor for TimeSources
            TimeSources(int size) : sourceVectorSize(size) {}

            // Add a priority to TimeSources
            void addPriority(int position, int priority) {
                addSizeToSource(position);
                if (position >= 0 && position < (int)source.size()) {
                    source[position].priority = priority;
                }
            }

            // Add a setSystemTime to TimeSources
            void addSetTimeFlag(int position, bool setSystemTime) {
                addSizeToSource(position);
                if (position >= 0 && position < (int)source.size()) {
                    source[position].setSystemTime = setSystemTime;
                }
            }

            // Add a source name to TimeSources
            void addSourceName(int position, const std::string& sourceName) {
                addSizeToSource(position);
                if (position >= 0 && position < (int)source.size()) {
                    source[position].sourceName = sourceName;
                }
            }

            // Add one more room for source vector
            void addSizeToSource(int position)
            {
                if (position == (int)source.size() && position + 1 < sourceVectorSize)
                {
                    source.push_back(Source(0, 0, ""));
                }
            }

            // Sort the source based on priority in ascending order
            void sortSourceByPriority() {
                std::sort(source.begin(), source.end(), [](const Source& a, const Source& b) {
                    return a.priority < b.priority;
                });
            }

            // Check if a source exist or not by name
            bool IsSourceExist(const std::string& name) const {
                return std::any_of(source.begin(), source.end(),
                            [&name](const Source& item) { return item.sourceName == name; });
            }

            // Find the position of a source by name
            int findSourcePosition(const std::string& name) const {
                auto it = std::find_if(source.begin(), source.end(),
                            [&name](const Source& item) { return item.sourceName == name; });

                if (it != source.end()) {
                    return std::distance(source.begin(), it);
                } else {
                    return -1; // source not found
                }
            }

            // Delete a source by name
            void deleteSourceByName(const std::string& name) {
                    source.erase(std::remove_if(source.begin(), source.end(),
                            [&name](const Source& item) { return item.sourceName == name; }),
                            source.end());
            }

            // Delete source by name list
            void deleteSourceByNameList(const std::vector<std::string>& names) {
                for (const auto& name : names) {
                    deleteSourceByName(name);
                }
            }

            // Print the details of all source in time sources configuration
            void printSourceDetails() const {
                for (const Source& item : source) {
                    LE_INFO("Name: %s,\tpriority: %d, setTimeFlag: %d\n",
                                item.sourceName.c_str(), item.priority, item.setSystemTime);
                }
                if (pollingInterval) {
                    LE_INFO("PollingInterval: %ld\n", pollingInterval);
                }
                if (toleranceMillsec) {
                    LE_INFO("ToleranceMillsec: %ld\n", toleranceMillsec);
                }
                LE_INFO("Time source size: %ld\n", source.size());
            }
        };

        class taf_TimeGnssListener : public telux::platform::ITimeListener
        {
            public:
                void onGnssUtcTimeUpdate(const uint64_t utc) override;
        };
        class taf_Time : public ITafSvc
        {
            public:
                taf_Time(){};
                ~taf_Time(){};

                /*
                 * This function is used to get the taf_Time instance
                 *
                 * @returns    Static reference of instance.
                 */
                static taf_Time &GetInstance();

                /*
                 * The initialization function for the Time Service.
                 */
                void Init(void);

                const char* SourceAttrToStr(taf_Time_SrcAttr_t sourceConf);
                const char* SourceNameIndexToStr(taf_time_TimeSources_t sourceName);
                taf_time_TimeSources_t SourceNameStrToIndex(const char* typeNamePtr);

                le_result_t ReadSourceConf(TimeSources& serviceCfg, const json_t *serviceData);
                le_result_t ReadTimeConf(TimeSources& serviceCfg, const json_t *serviceData);
                le_result_t LoadJsonConfiguration(TimeSources& serviceCfg, const char* filePathPtr);
                void DeleteNotSupportedSource(TimeSources& serviceCfg);

                taf_time_TimeSpec_t taf_time_Sub(taf_time_TimeSpec_t timeA, taf_time_TimeSpec_t timeB);
                taf_time_TimeSpec_t taf_time_Add(taf_time_TimeSpec_t timeA, taf_time_TimeSpec_t timeB);
                bool TimeGreaterThan(taf_time_TimeSpec_t timeA,taf_time_TimeSpec_t timeB);
                le_result_t ReadWriteDeltaTime(taf_time_TimeSpec_t* timeVal,
                                                 taf_TimeReadWrite_t ReadWriteType);
                le_result_t UpdateLocalGnssTime(taf_time_TimeSpec_t newGnssTime);

                le_result_t GetBootTime(taf_time_TimeSpec_t* timeVal);
                le_result_t GetRtcTime(taf_time_TimeSpec_t* timeVal);
                le_result_t GetGnssTime(taf_time_TimeSpec_t* timeVal);
                le_result_t GetExSetTimeStatus(void);
                le_result_t GetSystemTime(taf_time_TimeSpec_t* timeVal);

                le_result_t SetSystemTime(taf_time_TimeSpec_t timeVal,
                                             taf_time_TimeSources_t sourceName, bool ackTimeSvc);
                le_result_t SetTime(const char* sourceNameStr, taf_time_TimeSources_t sourceIndex);

                le_result_t RegGnssSyncTimeTask(void);
                void TimeSourceChangeNotify(taf_time_TimeSources_t PreTimeSource,
                                             taf_time_TimeSources_t NewTimeSource);

                le_result_t SetTimeBaseOnConfig(TimeSources serviceCfg,
                                 taf_time_TimeSources_t* latestActiveTime);

                static void* SyncTimeTasks(void* contextPtr);
                static void SystemTimeUpdateTimerHandler(le_timer_Ref_t timerRef);
                static void GetGnssTimeTimerHandler(le_timer_Ref_t timerRef);
                static void LayerTimeSourceChangeHandler(void* reportPtr, void* layerHandlerFunc);

                le_event_Id_t timeSourceChangeId;
                le_timer_Ref_t syncGnssTimerRef;
                le_timer_Ref_t sysTimeUdTimerRef;

                le_mem_PoolRef_t SetTimeStatusPool = NULL;
                le_mem_PoolRef_t timeSourceChangePool = NULL;
                le_mem_PoolRef_t DeltaTimeDataPool = NULL;

            private:
                std::shared_ptr<ITimeListener> gnssTimeListener
                                               = std::make_shared<taf_TimeGnssListener>();

                std::shared_ptr<ITimeManager> timeManager;

                struct SetTimeStatus* SetTimeSt;
                taf_time_TimeSpec_t* DeltaTimeData;

                pthread_mutex_t ProtectlocalTime_mutex;
        };
    }
}
