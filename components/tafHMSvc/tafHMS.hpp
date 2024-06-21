/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/* @file       tafHMS.hpp
* @brief      Interface for health monitor Service object. The functions
*             in this file are impletmented internally.
*/

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"


// Max number of Handler
#define MAX_HMS_HANLDER 32

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

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the HmsInfo
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    double cpuLoadInfo;
    uint32_t ramMemfreeInfo;
} tafHmsInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold CPU load information for each core
*/
//-------------------------------------------------------------------------------------------------
struct CPUCore {
    uint32_t user;
    uint32_t nice;
    uint32_t system;
    uint32_t idle;
    uint32_t iowait;
    uint32_t irq;
    uint32_t softirq;
    uint32_t steal;
    uint32_t guest;
};

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
    taf_hms_MtdDevInfoListRef_t ref;
}taf_hms_mtdInfoList_t;


namespace telux {
namespace tafsvc {
class taf_Hms: public ITafSvc
    {
        public:
            taf_Hms() {};
            ~taf_Hms() {};
            static taf_Hms& GetInstance();
            void Init();
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
            le_result_t GetUbiVolNum(taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
                uint32_t* ubiVolMajNumPtr, uint32_t* ubiVolMinNumPtr);
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

            le_mem_PoolRef_t ubiDevListPool;
            le_mem_PoolRef_t ubiDevInfoPool;
            le_mem_PoolRef_t ubiVolListPool;
            le_mem_PoolRef_t ubiVolInfoPool;
            le_mem_PoolRef_t mtdListPool;
            le_mem_PoolRef_t mtdInfoPool;

            le_ref_MapRef_t ubiDevListRefMap;
            le_ref_MapRef_t ubiDevRefMap;
            le_ref_MapRef_t ubiVolListRefMap;
            le_ref_MapRef_t ubiVolRefMap;
            le_ref_MapRef_t mtdListRefMap;
            le_ref_MapRef_t mtdRefMap;
    };
  }
}
