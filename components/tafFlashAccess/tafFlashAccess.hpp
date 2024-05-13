/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_FLASH_ACCESS_HPP
#define TAF_FLASH_ACCESS_HPP

#include <string>
#include <map>
#include <mtd/mtd-user.h>

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of MTD partitions.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_MTD_PARTITION_MAX_NUM 64

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of UBI volumes.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_UBI_VOLUME_MAX_NUM 32

//--------------------------------------------------------------------------------------------------
/**
 * Path length for device.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_DEV_PATH_LEN 16

//--------------------------------------------------------------------------------------------------
/**
 * Path length for UBI volume directory.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_UBI_VOL_DIR_PATH_LEN 32

//--------------------------------------------------------------------------------------------------
/**
 * MTD partition information under /proc.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_PROC_MTD "/proc/mtd"

//--------------------------------------------------------------------------------------------------
/**
 * UBI directory.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_SYS_CLASS_UBI "/sys/devices/virtual/ubi"

//--------------------------------------------------------------------------------------------------
/**
 * File to get UBI volume size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_SYS_CLASS_UBI_VOL_SIZE "/sys/class/ubi/ubi%d_%d/data_bytes"

//--------------------------------------------------------------------------------------------------
/**
 * File to get UBI volume erase block numbers.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_SYS_CLASS_UBI_LEB_NUM "/sys/class/ubi/ubi%d_%d/reserved_ebs"

//--------------------------------------------------------------------------------------------------
/**
 * File to get UBI volume free erasable block numbers.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_SYS_CLASS_UBI_FREE_LEB_NUM "/sys/class/ubi/ubi%d_%d/device/avail_eraseblocks"

//--------------------------------------------------------------------------------------------------
/**
 * MTD device information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char devPath[TAF_FLASH_DEV_PATH_LEN];
    uint32_t mtdSize;
    uint32_t mtdEraseSize;
} taf_FlashMtdDevInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * UBI device information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char devPath[TAF_FLASH_DEV_PATH_LEN];
    uint32_t deviceID;
    uint32_t volumeID;
    uint32_t lebNum;
    uint32_t freeLebNum;
    uint32_t volSize;
} taf_FlashUbiDevInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * MTD information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int fd;
    mtd_info_t info;
} taf_FlashMtdInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * UBI information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int fd;
    taf_FlashUbiDevInfo_t info;
} taf_FlashUbiInfo_t;

namespace telux
{
    namespace tafsvc
    {
        class taf_FlashAccess : public ITafSvc
        {
            public:
                taf_FlashAccess() {};
                ~taf_FlashAccess() {};

                static taf_FlashAccess &GetInstance();

                void Init();

                void InitMtdPartitions();
                void InitUbiVolumes();

                void GetNumFromFile(const char* filePath, uint32_t* number);
                void GetUbiID(const char* devPath, uint32_t* deviceID, uint32_t* volumeID);

                le_result_t IsMtdBadBlock(taf_FlashMtdInfo_t* mtdInfo, uint32_t blockIndex,
                    bool* isBad);

                le_mem_PoolRef_t mtdPool;
                le_mem_PoolRef_t ubiPool;

                le_ref_MapRef_t mtdRefMap;
                le_ref_MapRef_t ubiRefMap;

                std::map<std::string, taf_FlashMtdDevInfo_t> mtdDevMap;
                std::map<std::string, taf_FlashUbiDevInfo_t> ubiDevMap;
        };
    }
}

#endif