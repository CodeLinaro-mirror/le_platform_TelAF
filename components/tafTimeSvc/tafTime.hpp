/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_TIME_HEADER
#define TAF_TIME_HEADER

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "taf_gptpTime.h"

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

#define TAF_TIME_SERVICE_HEADER_STR        "TimeService"
#define TAF_TIME_INTERVAL_SETTING_STR      "PollingInterval"
#define TAF_TIME_ALLOWOVERRIDE_STR         "AllowOverrideAfterFail"
#define TAF_TIME_VALIDCLIENTLIST_STR       "ValidClientList"
#define TAF_TIME_GPTPDEVICENAME_STR        "GptpDeviceName"
#define TAF_TIME_SERVICE_SOURCE_STR        "Sources"

#define TAF_TIME_RTC_DEV_NAME "/dev/rtc0"

#define TAF_TIME_DELTA_TIME_DIR "/etc/deltatime"
#define TAF_TIME_DELTA_TIME_PATH TAF_TIME_DELTA_TIME_DIR "/dlt_msec"

//-------------------------------------------------------------------------------------------------
/**
 * Macro definition for time source.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_TIME_STR_MAX 16
#define TAF_TIME_SECOND_PER_LOOP_DEFAULT   65
#define TAF_TIME_SECOND_PER_COUNT_DEFAULT  1

#define TAF_TIME_NSEC_PER_SEC             (1000000000)
#define TAF_TIME_THRESHOLD_MILLISEC       (2000)

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

//-------------------------------------------------------------------------------------------------
/**
 * Macro definition for valid status event type range.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_TIME_EVENT_TYPE_LOWER_BOUND 0
#define TAF_TIME_EVENT_TYPE_UPPER_BOUND 3

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
    TAF_TIME_CONF_TOLMILLSEC,    ///< tolerance millsec
    TAF_TIME_CONF_SETTIMECOUNTER,    ///< tolerance millsec

    TAF_TIME_CONF_MAX_ITEM
}
taf_Time_SrcAttr_t;

//--------------------------------------------------------------------------------------------------
/**
 * Identifies the type of fail loops increase or clean.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    FAIL_LOOP_NUM_INCREASE,        ///< Increase the fail loops number
    FAIL_LOOP_NUM_CLEAN            ///< Reset the fail loops number
}
taf_TimeFailLoopAction_t;

struct SetTimeStatus{
    bool externalSetTime;  ///< Indicate if the time set externally
    bool asyncRtcSetTime;  ///< Indicate the status for async RTC set time
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
    uint16_t year;                   ///< Year.
    uint8_t month;                   ///< Month. 1 is January and 12 is December.
    uint8_t day;                     ///< Day. Range: 1 to 31.
    uint8_t hour;                    ///< Hour. Range: 0 to 23.
    uint8_t minute;                  ///< Minute. Range: 0 to 59.
    uint8_t second;                  ///< Second. Range: 0 to 59.
    uint8_t dayOfWeek;               ///< Day of the week. 0 is Monday and 6 is Sunday.
    int8_t timeZone;                 ///< Offset between UTC and local time in units of 15 minutes.
                                     ///  Actual value = field value * 15 minutes.
    uint8_t dstAdj;                  ///< Daylight saving adjustment in hours to obtain local time.
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
    uint8_t dayOfWeek;                   ///< Day of the week. 0 is Monday and 6 is Sunday.
    int8_t timeZone;                     ///< Offset between UTC and local time in units of 15 minutes (
                                         ///  signed value). Actual value = field value * 15 minutes.
    uint8_t dstAdj;                      ///< Daylight saving adjustment in hours to obtain local
                                         ///  time. Possible values: 0, 1, and 2.
    char nitzTime[NITZ_STR_BUF_MAX];     ///< Network Identity and Time Zone(NITZ) information in
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
    taf_time_TimeSources_t sourceId;                 ///< Time source ID.
    taf_time_TimeRef_t ref;                          ///< own reference.
    le_msg_SessionRef_t sessionRef;                  ///< Client that connected to the service.
    taf_DateTimeInf_t dateTimeInf;                   ///< Date time information.

    taf_time_TimeValueChangeHandlerRef_t handlerRef; ///< Handler reference.
    taf_time_TimeValueChangeHandlerFunc_t func;      ///< Handler function.
    void* context;                                   ///< Handler context.
} taf_TimeInf_t;

typedef struct
{
    taf_time_TimeSources_t sourceId;             ///< Time source ID.
    taf_time_TimeSources_t systemSourceId;       ///< System time source ID.
    taf_time_SourceRef_t ref;                    ///< own reference.
    bool sourceValidity;                         ///< The validity for current time source.
    bool isSyncedWithStorage;                    ///< The flag to indicate the storage has been touched.
    bool isSyncedWithSetCmd;                     ///< Indicate the 'SetValidity' API has been called ever.
    int32_t failedLoops = -1;                    ///< Number of loop failure for time source.
    bool isAvailable;
    int8_t timeZone = 0;                         ///< Offset between UTC and local time in units
                                                 ///  of 15 minutes(signed value).
                                                 ///  Actual value = field value * 15 minutes.
    uint8_t dstAdj = 0;                          ///< Daylight saving adjustment in hours to obtain
                                                 ///  local time. Possible values: 0, 1, and 2.
    taf_mngdStorSecData_DataRef_t secStrgdataRef = nullptr; ///< Managed storage service reference
                                                 /// for storing
    le_msg_SessionRef_t sessionRef;              ///< Client that connected to the service.
    taf_time_StatusEventType_t eventType;        ///< Type of event to which client want to
                                                 /// subscribe for.
    taf_time_TimeSourceStatusHandlerRef_t handlerRef = NULL;      ///< Handler reference.
    taf_time_TimeSourceStatusHandlerFunc_t handlerFunc = NULL;    ///< Handler function.
    void* context;                                                ///< Handler context.
} taf_SourceInf_t;

//--------------------------------------------------------------------------------------------------
/**
 * The structure for event handler.
 */
//--------------------------------------------------------------------------------------------------

typedef struct
{
    taf_time_SourceRef_t sourceRef;
    taf_SourceInf_t* sourcePtr;
    void* ref;
    bool status;
    taf_time_StatusEventType_t eventType;
}SourceStatusChange_Event_t;

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

struct NetworkInfoUpdateArgs_t
{
    uint8_t networkNumber;
    telux::tel::NetworkTimeInfo info;   ///< [IN] Network time information.
    telux::common::ErrorCode error;    ///< [IN] Error code.
};

struct ValidityParams
{
    taf_SourceInf_t* sourcePtr;
    bool validity;
};


//--------------------------------------------------------------------------------------------------
/**
 * Class defination.
 */
//--------------------------------------------------------------------------------------------------

    namespace tafsvc
    {
        class Source {
        public:
            int priority;
            bool setSystemTime;
            std::string sourceName;
            long int toleranceMillsec;
            long int setTimeCounter;

            Source
            (
                int pri, bool flag, const std::string& name,long int tolMillsec,long int setTimeCnt
            ) :
              priority(pri), setSystemTime(flag), sourceName(name), toleranceMillsec(tolMillsec),
              setTimeCounter(setTimeCnt) {}
        };

        class TimeSources {
        public:
            std::vector<Source> source;
            long int pollingInterval;
            int64_t allowOverrideAfterFail;
            std::vector<std::string> validClientList;
            std::string gptpDeviceName;
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

            // Add a toleranceMillsec to TimeSources
            void addToleranceMillsec(int position, long int tolMillsec) {
                addSizeToSource(position);
                if (position >= 0 && position < (int)source.size()) {
                    source[position].toleranceMillsec = tolMillsec;
                }
            }

            // Add a SetTimeCounter to TimeSources
            void addSetTimeCounter(int position, long int setTimeCnt) {
                addSizeToSource(position);
                if (position >= 0 && position < (int)source.size()) {
                    source[position].setTimeCounter = setTimeCnt;
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
                    source.push_back(Source(0, 0, "", TAF_TIME_THRESHOLD_MILLISEC, 0));
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
                    LE_INFO("Name: %s, priority: %d, setTimeFlag: %d, ToleranceMillsec: %ld, "
                        "SetTimeCounter: %ld\n",
                        item.sourceName.c_str(), item.priority, item.setSystemTime,
                        item.toleranceMillsec, item.setTimeCounter);
                }
                if (pollingInterval) {
                    LE_INFO("PollingInterval: %ld\n", pollingInterval);
                }
                LE_INFO("allowOverrideAfterFail: %" PRId64 "\n", allowOverrideAfterFail);

                for (auto item : validClientList) {
                    LE_INFO("Client: %s\n", item.c_str());
                }
                if(!gptpDeviceName.empty())
                {
                    LE_INFO("GptpDeviceName %s\n", gptpDeviceName.c_str());
                }

                LE_INFO("Time source size: %zu\n", source.size());
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
                ~taf_Time()
                {
                    LE_INFO("taf_Time destructor called");
                };

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
                le_result_t GetRtcTime(taf_time_TimeSpec_t* timeValPtr, bool isAllowGetInternalRTCTime);
                le_result_t GetGnssTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetExSetTimeStatus(void);
                le_result_t GetAsyncRtcSetTimeStatus(void);
                le_result_t GetSystemTime(taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetInternalRtcTime(taf_time_TimeSpec_t* timeVal);

                le_result_t ReadDeltaTimeFromStorage(int64_t* deltaTimeMSec);
                le_result_t UpdateDeltaTimeToStorage(taf_time_TimeSpec_t timeVal);

                le_result_t GetNetworkTime(taf_time_TimeSpec_t* timeValPtr,
                                                              taf_time_TimeSources_t sourceId);

                le_result_t UpdateRefTimeInfo(taf_TimeInf_t* timeSrcRefPrt,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t UpdateDateTimeInfo(taf_TimeInf_t* timeSrcRefPrt,
                                                              taf_time_TimeSpec_t* timeValPtr);

                le_result_t CheckSourceTime(taf_time_TimeSpec_t* timePtr,
                                                           taf_time_TimeSources_t sourceIndex);
                taf_time_TimeRef_t GetTimeRef(taf_time_TimeSources_t sourceId);
                le_result_t GetTime(taf_time_TimeRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetRefSystemTime(taf_time_TimeRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                le_result_t GetRefGptpTime(taf_time_TimeRef_t timeSrcRef,
                                                              taf_time_TimeSpec_t* timeValPtr);
                taf_TimeInf_t* SearchSourceInfList(taf_time_TimeSources_t sourceId,
                    le_msg_SessionRef_t sessionRef, bool handlerFlag);
                taf_TimeNetTimeInfo_t* SearchNetTimeInfList(taf_time_TimeSources_t sourceId);

                bool IsThresholdSetTimeAllow(taf_time_TimeSpec_t timeVal,
                                                taf_time_TimeSpec_t systemTime,
                                                    taf_time_TimeSources_t timeSource);
                le_result_t SetSystemTime(taf_time_TimeSpec_t timeVal,
                                           taf_time_TimeSources_t sourceName, bool ackTimeSvc);
                le_result_t SetTimeToRtc(taf_time_TimeSpec_t timeVal);

                le_result_t RegGnssTimeListener(void);
                void DeregGnssTimeListener(void);
                void TimeSourceChangeNotify(taf_time_TimeSources_t PreTimeSource,
                                             taf_time_TimeSources_t NewTimeSource);

                le_result_t SetTimeBaseOnConfig(TimeSources serviceCfg,
                                 uint64_t* timeSrcStatusMap);

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
                le_result_t ReleaseTimeRef(taf_time_TimeRef_t timeSrcRef);

                le_event_Id_t timeSourceChangeId;

                le_timer_Ref_t syncTimeTimerRef = NULL;
                le_timer_Ref_t sysTimeUdTimerRef = NULL;


                le_mem_PoolRef_t SetTimeStatusPool = NULL;
                le_mem_PoolRef_t timeSourceChangePool = NULL;

                taf_time_TimeSpec_t* GnssDeltaTime = NULL;
                le_mem_PoolRef_t GnssDeltaTimePool = NULL;


                le_ref_MapRef_t TimeRefMap;
                le_mem_PoolRef_t TimePool = NULL;

                le_ref_MapRef_t SrcRefMap;
                le_mem_PoolRef_t SrcPool = NULL;

                le_mem_PoolRef_t NetworkDeltaTimePool = NULL;
                le_mem_PoolRef_t NetworkDeltaTime2Pool = NULL;

                le_mem_PoolRef_t netTimeInfoPool = NULL;
                le_ref_MapRef_t netTimeInfoRefMap = NULL;

                taf_time_TimeSpec_t* NetworkDeltaTime = NULL;
                taf_time_TimeSpec_t* NetworkDeltaTime2 = NULL;

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

                le_event_Id_t timeSourceStatusEventId;
                taf_time_TimeSourceStatusHandlerRef_t AddTimeSourceStatusHandler(
                    taf_time_SourceRef_t SrcRef, taf_time_StatusEventType_t eventType,
                    taf_time_TimeSourceStatusHandlerFunc_t handlerPtr, void* contextPtr);
                taf_time_SourceRef_t GetSourceRef(taf_time_TimeSources_t sourceId);
                taf_SourceInf_t* SearchAvailableSourceInfList(taf_time_TimeSources_t sourceId);
                void printSourceInfo();
                le_result_t GetFailedLoops(taf_time_SourceRef_t sourceRef, int32_t* failedLoops,
                    int64_t* loopIntervalSec);
                bool IsAvailable(taf_time_SourceRef_t sourceRef);
                le_result_t GetSystemTimeSourceID(taf_time_TimeSources_t* timeSource);
                void SourceAvailabilityUpdate(le_result_t result,taf_time_TimeSources_t sourceIndex);
                le_result_t ReleaseSourceRef(taf_time_SourceRef_t SrcRef);
                void RemoveTimeSourceStatusHandler(taf_time_TimeSourceStatusHandlerRef_t handlerRef);

                le_result_t GetTimeZone(taf_time_SourceRef_t sourceRef, int8_t* timeZone);
                le_result_t GetTimeDayAdj(taf_time_SourceRef_t sourceRef, uint8_t* dayltSavAdj);
                le_result_t UpdateNetworkTimeZoneInfo(telux::tel::NetworkTimeInfo info,
                    taf_time_TimeSources_t sourceIndex);
                void UpdateFailedLoops(taf_time_TimeSources_t sourceIndex,
                    taf_TimeFailLoopAction_t action);
                void InitTimeSource(void);
                void InitializeSystemTimeAttr(void);
                bool IsSourceValid(taf_time_SourceRef_t sourceRef);
                le_result_t SetValidity(taf_time_SourceRef_t sourceRef, bool validity);
                le_result_t CheckSetValidityPermission(void);
                void ReportValidityChange(taf_SourceInf_t* sourcePtr);
                le_result_t WriteValidtyToSecStorage(taf_SourceInf_t* sourcePtr, bool newvalidity);
                le_result_t ReadValidityFromSecStorage(taf_SourceInf_t* sourcePtr, bool* validity);
                void ReleasePtpDevice(void);
                void RegisterPtpDevice(void);
                uint64_t PrevSrcAvailabiltyMap = 0x0;
                struct SetTimeStatus* SetTimeSt = NULL;
                NetworkInfoUpdateArgs_t NetworkUpdateInfo1 = {};
                NetworkInfoUpdateArgs_t NetworkUpdateInfo2 = {};
                le_thread_Ref_t mainThreadRef = NULL;
                int sigTermSignalNum = -1;

            private:
                std::shared_ptr<ITimeListener> gnssTimeListener = nullptr;
                std::shared_ptr<ITimeManager> timeManager;
                TimeTypeMask SupportTimeMask;
                int64_t AllowOverrideAfterFail = -1;
                pthread_mutex_t ProtectlocalTime_mutex;
                taf_gptpTime_Ref_t gptpTimeRef = NULL;
        };
    }
#endif
