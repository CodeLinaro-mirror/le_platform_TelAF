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

#ifndef TAF_TIME_HEADER
#define TAF_TIME_HEADER

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

// For reading json configuration file
#include "jansson.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

// For using VHAL
#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafHalTime.h"

// For RTC
#include <linux/rtc.h>

//For gnss time listener
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>
#include <condition_variable>

//For network time
#include <telux/common/CommonDefines.hpp>
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneListener.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <time.h>

#define TAF_TIME_THREAD_STACK_SIZE 0x20000
#define TAF_TIME_SERVICE_CONF_FILE       "tafTimeSvc.json"

#define TAF_TIME_SERVICE_HEADER_STR      "TimeService"
#define TAF_TIME_INTERVAL_SETTING_STR    "PollingInterval"
#define TAF_TIME_TOLERANCES_SETTING_STR  "ToleranceMillsec"
#define TAF_TIME_ALLOWOVERRIDE_STR      "AllowOverrideAfterFail"
#define TAF_TIME_SERVICE_SOURCE_STR      "Sources"

#define TAF_TIME_RTC_DEV_NAME "/dev/rtc0"
//-------------------------------------------------------------------------------------------------
/**
 * Macro definition for time source.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_TIME_STR_MAX 16
#define TAF_TIME_SECOND_PER_LOOP_DEFAULT   65
#define TAF_TIME_SECOND_PER_COUNT_DEFAULT  1

#define TAF_TIME_NSEC_PER_SEC             (1000000000)
#define TAF_TIME_THRESHOLD_MILLISEC       (200)

#define TAF_TIME_RECEIVE_GNSS_TIME_COUNT   5
#define TAF_TIME_SYNC_TIME_TIMER_INTERVAL (61000)

//-------------------------------------------------------------------------------------------------
/**
 * Macro definition for network time.
 */
//-------------------------------------------------------------------------------------------------
#define DEFAULT_SIM_SLOT_ID        1
#define DEFAULT_PHONE_NUM_MAX      2
#define NITZ_STR_BUF_MAX           60
#define DEFAULT_TSR_EVENT_CNT      16
#define DEFAULT_TSR_HANDLER_CNT    TAF_TIME_SRC_NAME_UNKNOWN

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

//--------------------------------------------------------------------------------------------------
/**
 * Network time information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t year;                 ///< Year.
    uint8_t month;                 ///< Month. 1 is January and 12 is December.
    uint8_t day;                   ///< Day. Range: 1 to 31.
    uint8_t hour;                  ///< Hour. Range: 0 to 23.
    uint8_t minute;                ///< Minute. Range: 0 to 59.
    uint8_t second;                ///< Second. Range: 0 to 59.
    uint8_t dayOfWeek;             ///< Day of the week. 0 is Monday and 6 is Sunday.
    int8_t timeZone;               ///< Offset between UTC and local time in units of 15 minutes.
                                   ///  Actual value = field value * 15 minutes.
    uint8_t dstAdj;                ///< Daylight saving adjustment in hours to obtain local time.
                                   ///  Possible values: 0, 1, and 2.
    char nitzTime[NITZ_STR_BUF_MAX]; ///< Network Identity and Time Zone(NITZ) information in
                                   ///  form "yyyy/mm/dd,hh:mm:ss(+/-)tzh:tzm,dt"
}taf_time_NetTimeInfo_t;

typedef struct
{
    taf_time_TimeSources_t sourceId;
    taf_time_NetTimeInfo_t timeInfo;
} taf_TimeNetTimeInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Time source reference information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t dayOfWeek;        ///< Day of the week. 0 is Monday and 6 is Sunday.
    int8_t timeZone;          ///< Offset between UTC and local time in units of 15 minutes (
                              ///  signed value). Actual value = field value * 15 minutes.
    uint8_t dstAdj;                  ///< Daylight saving adjustment in hours to obtain local
                                     ///  time. Possible values: 0, 1, and 2.
    char nitzTime[NITZ_STR_BUF_MAX]; ///< Network Identity and Time Zone(NITZ) information in
                                     ///  the form "yyyy/mm/dd,hh:mm:ss(+/-)tzh:tzm,dt".
    bool sourceValidity;                 ///< The validity for current time source.

    taf_time_TimeSpec_t sourceUtcTime;   ///< Time of curr source in seconds/nanoseconds
                                         ///  since epoch.
    uint64_t sourceUtcTimeUnc;           ///< Source time Uncertainty.

    taf_time_TimeSpec_t referRealTime;   ///< Elapsed system time created when sourceUtcTime
                                         ///  was updated.
    uint64_t referRealTimeUnc;           ///< System time Uncertainty.

    taf_time_TimeSpec_t referPtpTime;    ///< Elapsed ptp time created when sourceUtcTime
                                         ///  was updated.
    uint64_t referPtpTimeUnc;            ///< GPTP time Uncertainty.

} taf_DateTimeInf_t;

typedef struct
{
    taf_time_TimeSources_t sourceId;               ///< Time source ID.
    taf_time_TimeSourceRef_t ref;                  ///< own reference.
    le_msg_SessionRef_t sessionRef;                ///< Client that connected to the service.
    taf_DateTimeInf_t dateTimeInf;                 ///< Date time information.

    taf_time_TimeValueChangeHandlerRef_t handlerRef; ///< Handler reference.
    taf_time_TimeValueChangeHandlerFunc_t func;            ///< Handler function.
    void* context;                                 ///< Handler context.
} taf_TimeSourceInf_t;

//--------------------------------------------------------------------------------------------------
/**
 * The structure for event handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_time_TimeSources_t sourceId;
    void* ref;
}TS_Event_t;

typedef struct
{
    taf_time_TimeSources_t sourceId;              ///< Time source ID.
    void* ref;                                    ///< own reference.
    taf_DateTimeInf_t dateTimeInf;                ///< Date time information.
}TimeSourceRef_Event_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* getRTCCtxPtr;
    taf_time_AsyncGetTimeReqHandlerFunc_t getRTCCallbackFunc;
}taf_time_getRTCCb_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* setRTCCtxPtr;
    taf_time_AsyncSetTimeReqHandlerFunc_t setRTCCallbackFunc;
}taf_time_setRTCCb_t;

//--------------------------------------------------------------------------------------------------
/**
 * The structure for getting info about current sytem timesource info.
 */
 //--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_time_TimeSources_t source = TAF_TIME_SRC_NAME_UNKNOWN;
    bool validity;
    uint64_t loopCount;
}taf_timeSource_Info;

//--------------------------------------------------------------------------------------------------
/**
 * Class defination.
 */
//--------------------------------------------------------------------------------------------------

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
            int64_t allowOverrideAfterFail;
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
                if (allowOverrideAfterFail) {
                    LE_INFO("allowOverrideAfterFail: %ld\n", allowOverrideAfterFail);
                }
                LE_INFO("allowOverrideAfterFail: %ld\n", allowOverrideAfterFail);
                LE_INFO("Time source size: %ld\n", source.size());
            }
        };

        class taf_TimeGnssListener : public telux::platform::ITimeListener
        {
            public:
                void onGnssUtcTimeUpdate(const uint64_t utc) override;
        };
        class taf_TimeServingSystemListener : public telux::tel::IServingSystemListener
        {
            public:
                uint8_t phone = DEFAULT_SIM_SLOT_ID;
                taf_TimeServingSystemListener(uint8_t phone);
                void onNetworkTimeChanged(telux::tel::NetworkTimeInfo info) override;
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

                le_result_t ReadSourceConf(TimeSources& serviceCfg,
                                                                     const json_t *serviceDataPtr);
                le_result_t ReadTimeConf(TimeSources& serviceCfg,
                                                                     const json_t *serviceDataPtr);
                le_result_t LoadJsonConfiguration(TimeSources& serviceCfg,
                                                                          const char* filePathPtr);
                void DeleteNotSupportedSource(TimeSources& serviceCfg);

                taf_time_TimeSpec_t taf_time_Sub(taf_time_TimeSpec_t timeA,
                                                                        taf_time_TimeSpec_t timeB);
                taf_time_TimeSpec_t taf_time_Add(taf_time_TimeSpec_t timeA,
                                                                        taf_time_TimeSpec_t timeB);
                bool TimeGreaterThan(taf_time_TimeSpec_t timeA,taf_time_TimeSpec_t timeB);

                le_result_t ReadWriteDeltaTime(taf_time_TimeSpec_t* timeValPtr,
                         taf_time_TimeSpec_t* deltaTimeDataPtr, taf_TimeReadWrite_t ReadWriteType);

                le_result_t UpdateLocalTimeCache(taf_time_TimeSpec_t newTime,
                   taf_time_TimeSources_t sourceName, taf_time_TimeSpec_t* deltaTimeDataBufferPtr);

                le_result_t GetTimeFromLocalCache(taf_time_TimeSpec_t* timeValPtr,
                         taf_time_TimeSpec_t* deltaTimeDataPtr, taf_time_TimeSources_t sourceName);

                le_result_t GetBootTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetRtcTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetGnssTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetExSetTimeStatus(void);
                le_result_t GetSystemTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetInternalRtcTime(taf_time_TimeSpec_t* timeVal);
                le_result_t GetNetworkTime(taf_time_TimeSpec_t* timeValPtr,
                                                              taf_time_TimeSources_t sourceId);

                le_result_t UpdateRefTimeInfo(taf_TimeSourceInf_t* timeSrcRefPrt,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t UpdateDateTimeInfo(taf_TimeSourceInf_t* timeSrcRefPrt,
                                                              taf_time_TimeSpec_t* timeValPtr);

                le_result_t CheckSourceTime(taf_time_TimeSpec_t* timePtr,
                                                           taf_time_TimeSources_t sourceIndex);
                taf_time_TimeSourceRef_t GetTimeRef(taf_time_TimeSources_t sourceId);
                le_result_t GetTime(taf_time_TimeSourceRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetRefSystemTime(taf_time_TimeSourceRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetRefGptpTime(taf_time_TimeSourceRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                taf_TimeSourceInf_t* SearchSourceInfList(taf_time_TimeSources_t sourceId,
                                             le_msg_SessionRef_t sessionRef, bool handlerFlag);
                taf_TimeNetTimeInfo_t* SearchNetTimeInfList(taf_time_TimeSources_t sourceId);

                bool IsNecessaryUpdateSystemTime(taf_time_TimeSpec_t timeVal,
                                                               taf_time_TimeSpec_t systemTime);
                le_result_t SetSystemTime(taf_time_TimeSpec_t timeVal,
                                           taf_time_TimeSources_t sourceName, bool ackTimeSvc);
                le_result_t SetTimeToRtc(taf_time_TimeSpec_t timeVal);

                le_result_t RegGnssTimeListener(void);
                void DeregGnssTimeListener(void);
                void TimeSourceChangeNotify(taf_time_TimeSources_t PreTimeSource,
                                             taf_time_TimeSources_t NewTimeSource);

                le_result_t SetTimeBaseOnConfig(TimeSources serviceCfg,
                                 taf_time_TimeSources_t* latestActiveTimePtr);

                static void* SyncTimeTasks(void* contextPtr);
                static void SyncTimeTimerHandler(le_timer_Ref_t timerRef);
                static void SystemTimeUpdateTimerHandler(le_timer_Ref_t timerRef);
                static void SyncGnssTime(void);
                static void LayerTimeSourceChangeHandler(void* reportPtr,
                                                                        void* layerHandlerFuncPtr);

                static void RequestNetworkTime(void);
                void NetworkTimeResponseUpdate(uint8_t phoneId,
                    telux::tel::NetworkTimeInfo info, telux::common::ErrorCode error);
                static void SyncNetworkTimeResponse(telux::tel::NetworkTimeInfo info,
                                                                   telux::common::ErrorCode error);
                static void SyncNetworkTimeResponse2(telux::tel::NetworkTimeInfo info,
                                                                   telux::common::ErrorCode error);
                le_result_t ConvertDateTimeToSec(struct tm dateTime,
                                                                  taf_time_TimeSpec_t* timeValPtr);

                le_result_t ConvertNetworkTimeToSec(telux::tel::NetworkTimeInfo info,
                                                                  taf_time_TimeSpec_t* timeValPtr);
                le_result_t RegNetworkTimeListener(void);
                void DeregNetworkTimeListener(void);

                le_result_t InitGnssTime(void);
                le_result_t InitNetworkTime(void);

                taf_time_TimeValueChangeHandlerRef_t AddTimeValueChangeHandler(
                             taf_time_TimeSources_t sourceId,
                             taf_time_TimeValueChangeHandlerFunc_t handlerPtr, void* contextPtr);

                void RemoveTimeValueChangeHandler(taf_time_TimeValueChangeHandlerRef_t handlerRef);
                void NotifyRefTimeClient(TS_Event_t* tsEventPtr);
                void StoreDateTimeInfo(telux::tel::NetworkTimeInfo info,
                                                                  taf_time_TimeSources_t sourceId);
                void ReportTimeValueChange(taf_time_TimeSources_t sourceId,
                                   taf_time_TimeSpec_t timeVal, telux::tel::NetworkTimeInfo* info);
                le_result_t CreateRefTimeForHandler(TimeSourceRef_Event_t* tsrEventPrt,
                                   taf_time_TimeSpec_t timeVal, telux::tel::NetworkTimeInfo* info);
                static void EventTimeValueChangeHandler(void* reportPtr);
                le_result_t ReleaseTimeRef(taf_time_TimeSourceRef_t timeSrcRef);

                le_event_Id_t timeSourceChangeId;

                le_timer_Ref_t syncTimeTimerRef;
                le_timer_Ref_t sysTimeUdTimerRef;


                le_mem_PoolRef_t SetTimeStatusPool = NULL;
                le_mem_PoolRef_t timeSourceChangePool = NULL;

                taf_time_TimeSpec_t* GnssDeltaTime = NULL;
                le_result_t InitGnssTimeStatus = LE_UNAVAILABLE;
                le_mem_PoolRef_t GnssDeltaTimePool = NULL;


                le_ref_MapRef_t SrcTimeRefMap;
                le_mem_PoolRef_t SrcTimePool = NULL;

                le_mem_PoolRef_t NetworkDeltaTimePool = NULL;
                le_mem_PoolRef_t NetworkDeltaTime2Pool = NULL;

                le_mem_PoolRef_t netTimeInfoPool = NULL;
                le_ref_MapRef_t netTimeInfoRefMap = NULL;

                taf_time_TimeSpec_t* NetworkDeltaTime = NULL;
                taf_time_TimeSpec_t* NetworkDeltaTime2 = NULL;

                le_result_t InitNetworkTimeStatus = LE_UNAVAILABLE;
                le_event_Id_t RefTimeEventId;
                le_event_HandlerRef_t RefTimeEventHandlerRef;

                // Time Source Reference event object.
                le_ref_MapRef_t TsrEventMap;
                le_mem_PoolRef_t TsrEventPool;

                //For getting network time and notification
                std::shared_ptr<telux::tel::IPhoneManager> phoneManager;
                //std::vector<std::shared_ptr<telux::tel::IPhone>> phones;
                std::vector<std::shared_ptr<taf_TimeServingSystemListener>> servSysListeners;
                std::vector<std::shared_ptr<telux::tel::IServingSystemManager>> servingSystemManagers;

                time_Inf_t* timeInf = nullptr;
                bool isDrvPresent = false;
                static taf_time_getRTCCb_t getRTCCB;
                static taf_time_setRTCCb_t setRTCCB;
                static void getRTCRespCB(struct TimeSpec timeVal, le_result_t result);
                static void setRTCRespCB(le_result_t result);
                le_result_t SetRtcTimeReqAsync(const taf_time_TimeSpec_t* timeValPtr,
                    taf_time_AsyncSetTimeReqHandlerFunc_t handlerPtr, void* contextPtr);
                le_result_t GetRtcTimeReqAsync(taf_time_AsyncGetTimeReqHandlerFunc_t handlerPtr,
                    void* contextPtr);
                bool isNewTimeSrcSetTimeAllowed(taf_time_TimeSources_t newTimeSource);

            private:
                std::shared_ptr<ITimeListener> gnssTimeListener = nullptr;
                std::shared_ptr<ITimeManager> timeManager;
                TimeTypeMask SupportTimeMask;

                struct SetTimeStatus* SetTimeSt = NULL;
                uint64_t TimeSourceStatusMap = 0x0;
                int64_t AllowOverrideAfterFail = -1;
                pthread_mutex_t ProtectlocalTime_mutex;
        };
    }
}
#endif
