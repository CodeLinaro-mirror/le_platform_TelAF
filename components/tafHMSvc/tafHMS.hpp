/*
* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/* @file       tafHMS.hpp
* @brief      Interface for health monitor Service object. The functions
*             in this file are impletmented internally.
*/

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include <telux/platform/SubsystemFactory.hpp>
#include <telux/platform/SubsystemManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include <telux/tel/PhoneManager.hpp>

#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneListener.hpp>
#include <atomic>

using namespace std;

// Max number of Handler
#define MAX_CORES 8
#define MAX_FIELDS 10

#define TAF_HMS_MAX_LIST_POOL_SIZE 100
#define MAX_PATH_LENGTH 128
#define BUFFER_SIZE 64

#define UBI_CLASS_PATH "/sys/class/ubi"
#define UBI_SYSFS_PATH  "/sys/class/ubi/ubi%d/"
#define UBI_DEV_BB_COUNT "bad_peb_count"
#define UBI_DEV_E_COUNT "max_ec"

#define UBI_VOL_PATH  "/sys/class/ubi/ubi%d_%d/"
#define UBI_VOL_NAME "name"
#define UBI_VOL_SIZE "data_bytes"
#define UBI_VOL_MAJ_MIN "dev"

#define MTD_CLASS_PATH "/sys/class/mtd"
#define MTD_DEV_PATH "/sys/class/mtd/mtd%d/"
#define MTD_DEV_NAME "name"
#define MTD_DEV_SIZE "size"
#define MTD_DEV_MAJ_MIN "dev"

#define UBI_DEV_BB_COUNT_PATH    UBI_SYSFS_PATH UBI_DEV_BB_COUNT
#define UBI_DEV_E_COUNT_PATH     UBI_SYSFS_PATH UBI_DEV_E_COUNT
#define UBI_VOL_NAME_PATH        UBI_VOL_PATH UBI_VOL_NAME
#define UBI_VOL_SIZE_PATH        UBI_VOL_PATH UBI_VOL_SIZE
#define UBI_VOL_MAJ_MIN_PATH     UBI_VOL_PATH UBI_VOL_MAJ_MIN
#define MTD_DEV_NAME_PATH        MTD_DEV_PATH MTD_DEV_NAME
#define MTD_DEV_SIZE_PATH        MTD_DEV_PATH MTD_DEV_SIZE
#define MTD_DEV_MAJ_MIN_PATH     MTD_DEV_PATH MTD_DEV_MAJ_MIN

#define UNUSED(arg) (arg = arg)

#define TAF_HMS_MAX_REF_POOL_SIZE 10
#define TAF_HMS_MAX_EVENT_POOL_SIZE (TAF_HMS_MAX_REF_POOL_SIZE * 3)
#define TAF_HMS_MODEM_RESET_TIMER 120000
#define TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_LOW 1
#define TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_MEDIUM 2
#define TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_HIGH 3
#define TAF_HMS_SUBSYSTEM_MANAGER_TIMEOUT 30
#define TAF_HMS_PHONE_MANAGER_TIMEOUT     10

// For reset reason
#define TAF_HMS_BOOT_REASON_PATH "/sys/kernel/reboot_reason/reason"

// For reset sub-reason
#define TAF_HMS_BOOT_SUB_REASON_PATH "/data/telaf/bootReason"

//-------------------------------------------------------------------------------------------------
/**
* Important: MODEM_CHECK_STATUS_INTERVAL is used for counting the response of modem status, please
* also change the response counter in the request function if this interval got change.
*
* Note, the modem needs about 11 seconds for reset (send_data 75 37 03 00 00)
*/
//-------------------------------------------------------------------------------------------------
#define MODEM_CHECK_STATUS_INTERVAL 3000   //Time interval for checking the modem status
#define COUNTER_RESPONSE_TIME_OUT   2 //Report event to client if the counter equal to this macro
#define COUNTER_EVENT_REPORT_DONE   3 //Stop requesting the status if event reported to the client

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold CPU load information for each core
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t user;
    uint32_t nice;
    uint32_t system;
    uint32_t idle;
    uint32_t iowait;
    uint32_t irq;
    uint32_t softirq;
    uint32_t steal;
    uint32_t guest;
    uint32_t guest_nice;
}taf_hms_CPUCore_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the UBI device Info
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t devId;
    uint32_t eraseCnt;
    uint32_t badBlockCnt;
    le_sls_Link_t link;
    taf_hms_UbiDevInfoRef_t ref;
}taf_hms_ubiDevInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the UBI device list
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t ubiDevInfoListSize;
    le_sls_List_t ubiDevInfoList;
    le_sls_Link_t* currPtr;
    le_msg_SessionRef_t sessionRef;
    taf_hms_UbiDevInfoListRef_t ref;
}taf_hms_ubiDevInfoList_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the UBI volume Info
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t ubiVolSize;
    uint32_t ubiVolId;
    uint32_t ubiVolMajNum;
    uint32_t ubiVolMinNum;
    char ubiVolumeName[50];
    le_sls_Link_t link;
    taf_hms_UbiVolInfoRef_t ref;
}taf_hms_ubiVolInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the UBI volume list
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t ubiVolInfoListSize;
    le_sls_List_t ubiVolInfoList;
    le_sls_Link_t* currPtr;
    taf_hms_UbiDevInfoListRef_t ref;
}taf_hms_ubiVolInfoList_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the MTD Info
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t mtdBlockSize;
    uint32_t mtdDevId;
    uint32_t mtdBlockCnt;
    uint32_t mtdDevMajNum;
    uint32_t mtdDevMinNum;
    char mtdDevPath[50];
    char mtdDevName[50];
    le_sls_Link_t link;
    taf_hms_MtdDevInfoRef_t ref;
}taf_hms_mtdInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the MTD Info list
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t mtdInfoListSize;
    le_sls_List_t mtdInfoList;
    le_sls_Link_t* currPtr;
    le_msg_SessionRef_t sessionRef;
    taf_hms_MtdDevInfoListRef_t ref;
}taf_hms_mtdInfoList_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the Modem Info
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_hms_ModemEvtHandlerRef_t  handlerRef = NULL;
    taf_hms_ModemEvtHandlerFunc_t handlerFunc = NULL;
    taf_hms_ModemEvtBitmask_t     reqEventBits;  // Client only needs the registered event type.
    le_msg_SessionRef_t           sessionRef = NULL;
    uint32_t                      clientId = 0;       // Monotonic ID for log correlation.
    void* contextPtr;
}taf_hms_modemInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the Modem Event Info
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_hms_ModemEvtType_t      eventType;
    taf_hms_ModemEvtSeverity_t  eventLevel;
    taf_hms_ModemEventRef_t     ref;
    taf_hms_modemInfo_t*        modemInfo;
}taf_hms_modemEventInfo_t;


typedef enum
{
    MODEM_ON_CHANGE_STATUS_UNKNOWN,
    MODEM_ON_CHANGE_STATUS_UNAVAILABLE,
    MODEM_ON_CHANGE_STATUS_OPERATIONAL
}
taf_hms_operationStatus_t;

typedef struct
{
    int operationalStatus;
}taf_hms_modemOperaInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Internal payload struct used to marshal operatingModeResponse data to the main thread.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int operatingMode;  ///< telux::tel::OperatingMode cast to int
    int error;          ///< telux::common::ErrorCode cast to int
} taf_hms_operModeRespData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Health Monitor Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace tafsvc {
    class tafHmsListener : public telux::platform::ISubsystemListener {
        public:
        tafHmsListener() {};
        ~tafHmsListener() {};
        int counter {0};
        uint64_t AllModemEventMap = 0x0;
        uint8_t ModemCrashCounter = 0;
        static std::atomic<bool> ModemAvailability;

        static tafHmsListener& GetInstance()
        {
            static tafHmsListener instance;
            return instance;
        }
        void onStateChange(telux::common::SubsystemInfo subsystemInfo,
            telux::common::OperationalStatus newOperationalStatus) override;

        void StartResetTimer(void);
        void DeleteResetTime(void);
    };

    class ModemStatus : public telux::tel::IOperatingModeCallback,
                        public std::enable_shared_from_this<ModemStatus> {
    public:
        ModemStatus() {};
        ~ModemStatus() {};
        // Called by main thread to start/stop the worker thread and main timer.
        void StartWorkerThread(void);
        void StopWorkerThread(void);

        // Called by main thread to dispatch modem status events to clients.
        void ReportModemStatus(taf_hms_ModemEvtType_t eventType,
                               taf_hms_ModemEvtSeverity_t eventLevel);

        // SDK callback - called from telux SDK thread, marshals to main thread.
        void operatingModeResponse(telux::tel::OperatingMode operatingMode,
                                   telux::common::ErrorCode error) override;

        // Called on the worker thread - queued by the main-thread timer each tick.
        void DoRequestOperatingMode(void);

        le_thread_Ref_t GetWorkerThread(void) const { return workerThread_; }

    private:
        // Worker thread entry point and PhoneInit helper (run on worker thread).
        static void* WorkerThreadFunc(void* ctx);
        bool         PhoneInit(void);

        std::shared_ptr<telux::tel::IPhoneManager> phoneManager_{nullptr};

        // Worker thread handle.
        le_thread_Ref_t   workerThread_{nullptr};
        // Main-thread timer (3s repeating) - owns pendingCount.
        le_timer_Ref_t    mainTimer_{nullptr};
    };

    class taf_Hms: public ITafSvc
    {
        public:
            taf_Hms() {};
            ~taf_Hms() {};
            static taf_Hms& GetInstance();
            void Init();
            void AdvertiseService();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
            le_result_t GetCpuLoad(double* cpuCurrentLoadPtr);
            uint32_t GetCpuCoreNum(void);
            le_result_t GetIndvCoreUsage(uint32_t coreID, double* cpuUsagePtr);
            le_result_t GetRamMemInfo(uint32_t* ramTotalMemPtr, uint32_t* ramUsedMemPtr,
                uint32_t* ramFreeMemPtr);
            taf_hms_UbiDevInfoListRef_t GetUbiDevInfoList();
            le_result_t DeleteUbiDevInfoList(taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef);
            taf_hms_UbiDevInfoRef_t GetFirstUbiDevInfo(
                taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef);
            taf_hms_UbiDevInfoRef_t GetNextUbiDevInfo(
                taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef);
            le_result_t GetUbiDevId(taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
                uint32_t* ubiDevIdPtr);
            le_result_t GetUbiDevMaxEraseCnt(taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
                uint32_t* ubiEraseCntPtr);
            le_result_t GetUbiDevBadBlkCnt(taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
                uint32_t* ubiBbCntPtr);
            taf_hms_UbiVolInfoRef_t GetFirstUbiVolInfo(
                taf_hms_UbiDevInfoRef_t ubiDevInfoRef);
            taf_hms_UbiVolInfoRef_t GetNextUbiVolInfo(
                taf_hms_UbiDevInfoRef_t ubiDevInfoRef);
            le_result_t GetUbiVolId(taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
                uint32_t* ubiVolIdPtr);
            le_result_t GetUbiVolName(taf_hms_UbiVolInfoRef_t ubiVolInfoRef, char* ubiVolName,
                size_t ubiVolNameSize);
            le_result_t GetUbiVolSize(taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
                uint32_t* ubiVolSizePtr);
            taf_hms_MtdDevInfoListRef_t GetMtdDevInfoList();
            le_result_t DeleteMtdDevInfoList(taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef);
            taf_hms_MtdDevInfoRef_t GetFirstMtdDevInfo(taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef);
            taf_hms_MtdDevInfoRef_t GetNextMtdDevInfo(taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef);
            le_result_t GetMtdDevName(taf_hms_MtdDevInfoRef_t mtdDevInfoRef,char* mtdName,
                size_t mtdNameSize);
            le_result_t GetMtdDevBlkSize(taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
                uint32_t* mtdBlkSizePtr);
            le_result_t GetMtdDevBlkCnt(taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
                uint32_t* mtdBlkCntPtr);
            le_result_t GetMtdDevId(taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
                uint32_t* mtdDevIdPtr);
            taf_hms_ModemEvtHandlerRef_t AddModemEvtHandler(
                                       taf_hms_ModemEvtBitmask_t reqEventbits,
                                       taf_hms_ModemEvtHandlerFunc_t handlerPtr, void* contextPtr);
            static void ModemStatusChNotifyClient(void* reportPtr);
            void RemoveModemEvtHandler(taf_hms_ModemEvtHandlerRef_t handlerRef);
            bool IsModemEventMapEmpty(void);
            le_result_t ReleaseModemEvt(taf_hms_ModemEventRef_t  eventRef);

            le_event_Id_t MdStatusChNotifyCliId;
            le_event_Id_t MdStatusOnChangeCBId;
            le_event_Id_t OperModeRespId;
            le_ref_MapRef_t ModemInfoRefMap;
            le_ref_MapRef_t ModemEventInfoRefMap;
            le_mem_PoolRef_t ModemEventInfoPool;

            taf_hms_Reset_t ParseBootReason(const std::string& reasonStrRaw);
            le_result_t ReadSubReason(const std::string& filePath, char* subReasonStr,
                size_t subReasonSize);
            le_result_t GetResetInformation(taf_hms_Reset_t* resetPtr,
                char* resetSpecificInfoStr, size_t resetSpecificInfoStrSize);

        private:
            le_mem_PoolRef_t UbiDevListPool;
            le_mem_PoolRef_t UbiDevInfoPool;
            le_mem_PoolRef_t UbiVolListPool;
            le_mem_PoolRef_t UbiVolInfoPool;
            le_mem_PoolRef_t MtdListPool;
            le_mem_PoolRef_t MtdInfoPool;
            le_mem_PoolRef_t ModemInfoPool;

            le_ref_MapRef_t UbiDevListRefMap;
            le_ref_MapRef_t UbiDevRefMap;
            le_ref_MapRef_t UbiVolListRefMap;
            le_ref_MapRef_t UbiVolRefMap;
            le_ref_MapRef_t MtdListRefMap;
            le_ref_MapRef_t MtdRefMap;

            std::shared_ptr<telux::platform::ISubsystemManager> subsystemMgr;
            std::shared_ptr<tafHmsListener> stateListener;
    };
  }
