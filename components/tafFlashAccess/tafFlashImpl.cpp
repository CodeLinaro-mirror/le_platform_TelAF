/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include "tafFlashAccess.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for MTD partitions.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(mtdPool, TAF_FLASH_MTD_PARTITION_MAX_NUM, sizeof(taf_FlashMtdInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for UBI volumes.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ubiPool, TAF_FLASH_UBI_VOLUME_MAX_NUM, sizeof(taf_FlashUbiInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static map for MTD partition reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(mtdRefMap, TAF_FLASH_MTD_PARTITION_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for UBI volume reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ubiRefMap, TAF_FLASH_UBI_VOLUME_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Get instance of flash access.
 *
 * @return Instance of flash access.
 */
//--------------------------------------------------------------------------------------------------
taf_FlashAccess &taf_FlashAccess::GetInstance
(
    void
)
{
    static taf_FlashAccess instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Intialize MTD partitions. This function will traverse all mtd partitions in the /proc directory
 * and store the corresponding mtd partition name and size, erasable block size, device path and 
 * other information.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::InitMtdPartitions
(
    void
)
{
    if (!mtdDevMap.empty())
    {
        mtdDevMap.clear();
        LE_INFO("Clear previous MTD information.");
    }

    ifstream procMtd(TAF_FLASH_PROC_MTD);
    string line;
    if (procMtd.is_open())
    {
        string prefix = "mtd";
        while (getline(procMtd, line))
        {
            // Check MTD prefix.
            if (line.compare(0, prefix.size(), prefix) == 0)
            {
                size_t pos = 0;
                string mtdDev, mtdName;
                taf_FlashMtdDevInfo_t info;

                // Get MTD device number.
                if ((pos = line.find(" ")) != string::npos)
                {
                    mtdDev = "/dev/" + line.substr(0, pos - 1);
                    le_utf8_Copy(info.devPath, mtdDev.c_str(), TAF_FLASH_DEV_PATH_LEN, NULL);
                    LE_DEBUG("MTD device : %s ", info.devPath);
                    line.erase(0, pos + 1);
                }
                else
                {
                    LE_ERROR("Fail to get MTD device number.");
                    break;
                }

                // Get MTD size.
                if ((pos = line.find(" ")) != string::npos)
                {
                    stringstream ss;
                    ss << hex << line.substr(0, pos);
                    ss >> info.mtdSize;
                    line.erase(0, pos + 1);
                }
                else
                {
                    LE_ERROR("Fail to get MTD size.");
                    break;
                }

                // Get MTD erase size.
                if ((pos = line.find(" ")) != string::npos)
                {
                    stringstream ss;
                    ss << hex << line.substr(0, pos);
                    ss >> info.mtdEraseSize;
                    line.erase(0, pos + 1);
                }
                else
                {
                    LE_ERROR("Fail to get MTD erase size.");
                    break;
                }

                // Get MTD name.
                if (line.size() > 2)
                {
                    mtdName = line.substr(1, line.size() - 2);
                    LE_DEBUG("MTD name : %s ", mtdName.c_str());
                }
                else
                {
                    LE_ERROR("Fail to get MTD name.");
                    break;
                }

                mtdDevMap.insert(make_pair(mtdName, info));
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI device and volume ID from the device path.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::GetUbiID
(
    const char* devPath, ///< [IN] UBI device path.
    uint32_t* deviceID,  ///< [OUT] UBI device ID.
    uint32_t* volumeID   ///< [OUT] UBI volume ID.
)
{
    int32_t i = 0;
    *volumeID = 0;
    for (i = strlen(devPath) - 1; i >= 0; i--)
    {
        if (devPath[i] == '_')
            break;

        if (devPath[i] >= '0' && devPath[i] <= '9')
        {
            *volumeID = (*volumeID) * 10 + devPath[i] - '0';
        }
    }

    *deviceID = 0;
    i--;
    while (i >= 0)
    {
        if (devPath[i] >= '0' && devPath[i] <= '9')
        {
            *deviceID = (*deviceID) * 10 + devPath[i] - '0';
            i--;
        }
        else
        {
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Intialize UBI volumes, This function will traverse all ubi volumes in the /sys/class/ directory,
 * and store the corresponding ubi volume name and size, device id and volume id, number of lebs
 * and available lebs, as well as device path and other information.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::InitUbiVolumes
(
    void
)
{
    if (!ubiDevMap.empty())
    {
        ubiDevMap.clear();
        LE_INFO("Clear previous UBI information.");
    }

    char* pathArrayPtr[] = {(char*)TAF_FLASH_SYS_CLASS_UBI, NULL};
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);
    FTSENT* entPtr;
    char* baseName;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
            case FTS_DP:
            case FTS_DEFAULT:
                break;
            case FTS_F:
                baseName = le_path_GetBasenamePtr(entPtr->fts_path, "/");
                if (strncmp(baseName, "name", strlen("name")) == 0)
                {
                    ifstream ubiVol(entPtr->fts_path);
                    string ubiVolName;
                    if (getline(ubiVol, ubiVolName))
                    {
                        taf_FlashUbiDevInfo_t info;
                        char dirName[TAF_FLASH_UBI_VOL_DIR_PATH_LEN] = "";
                        le_utf8_Copy(dirName, entPtr->fts_path, strlen(entPtr->fts_path) -
                            strlen("name"), NULL);
                        char* ubiBaseName = le_path_GetBasenamePtr(dirName, "/");
                        string ubiDev = "/dev/" + string(ubiBaseName);
                        le_utf8_Copy(info.devPath, ubiDev.c_str(), TAF_FLASH_DEV_PATH_LEN, NULL);

                        GetUbiID(info.devPath, &info.deviceID, &info.volumeID);

                        char volSizeFile[PATH_MAX] = "";
                        snprintf(volSizeFile, sizeof(volSizeFile) - 1,
                            TAF_FLASH_SYS_CLASS_UBI_VOL_SIZE, info.deviceID, info.volumeID);
                        GetNumFromFile(volSizeFile, &info.volSize);

                        char lebNumfile[PATH_MAX] = "";
                        snprintf(lebNumfile, sizeof(lebNumfile) - 1,
                            TAF_FLASH_SYS_CLASS_UBI_LEB_NUM, info.deviceID, info.volumeID);
                        GetNumFromFile(lebNumfile, &info.lebNum);

                        char freeLebNumfile[PATH_MAX] = "";
                        snprintf(freeLebNumfile, sizeof(freeLebNumfile) - 1,
                            TAF_FLASH_SYS_CLASS_UBI_FREE_LEB_NUM, info.deviceID, info.volumeID);
                        GetNumFromFile(freeLebNumfile, &info.freeLebNum);

                        LE_DEBUG("UBI volume %s dev : %s", ubiVolName.c_str(), info.devPath);
                        LE_DEBUG("UBI device id : %d volume id : %d ", info.deviceID, info.volumeID);
                        LE_DEBUG("UBI volume size : %d", info.volSize);
                        LE_DEBUG("UBI volume leb number : %d", info.lebNum);
                        LE_DEBUG("UBI volume free leb : %d", info.freeLebNum);

                        ubiDevMap.insert(make_pair(ubiVolName, info));
                    }
                    else
                    {
                        LE_ERROR("Fail to read UBI volume name from %s.", entPtr->fts_path);
                    }
                }
                break;
            case FTS_SL:
            case FTS_SLNONE:
                break;
            case FTS_DC:
            case FTS_DNR:
            case FTS_NS:
            case FTS_ERR:
            default:
                LE_ERROR("Unexpected file type, %d, on file %s.", entPtr->fts_info,
                    entPtr->fts_path);
                break;
        }
    }

    fts_close(ftsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get number from file.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::GetNumFromFile
(
    const char* filePath, ///< [IN] File path.
    uint32_t* number      ///< [OUT] number.
)
{
    FILE* fp = fopen(filePath, "r");
    if (fp == NULL)
    {
        LE_ERROR("Fail to get number from %s.", filePath);
        return;
    }

    int rc = fscanf(fp, "%d\n", number);
    LE_DEBUG("RC : %d", rc);
    fclose(fp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a block is bad block of MTD partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FlashAccess::IsMtdBadBlock
(
    taf_FlashMtdInfo_t* mtdInfo, ///< [IN] MTD information.
    uint32_t blockIndex,         ///< [IN] MTD block index.
    bool* isBad                  ///< [OUT] True if it is bad block.
)
{

    if (mtdInfo == NULL)
    {
        LE_ERROR("Invalid MTD information.");
        return LE_FAULT;
    }

    if (mtdInfo->info.erasesize == 0)
    {
        LE_ERROR("Invalid para(block size is 0)");
        return LE_FAULT;
    }

    if (blockIndex >= mtdInfo->info.size / mtdInfo->info.erasesize)
    {
        LE_ERROR("Invalid block index %d.", blockIndex);
        return LE_FAULT;
    }

    loff_t page = (loff_t)(blockIndex * mtdInfo->info.erasesize);
    int ret = ioctl(mtdInfo->fd, MEMGETBADBLOCK, &page);
    if (ret != 0 && !(ret == -1 && errno == EOPNOTSUPP))
    {
        *isBad = true;
    }
    else
    {
        *isBad = false;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::Init
(
    void
)
{
    // 1. Initiate the memory pool.
    mtdPool = le_mem_InitStaticPool(mtdPool, TAF_FLASH_MTD_PARTITION_MAX_NUM,
        sizeof(taf_FlashMtdInfo_t));
    ubiPool = le_mem_InitStaticPool(ubiPool, TAF_FLASH_UBI_VOLUME_MAX_NUM,
        sizeof(taf_FlashUbiInfo_t));

    // 2. Initiate the reference map.
    mtdRefMap = le_ref_InitStaticMap(mtdRefMap, TAF_FLASH_MTD_PARTITION_MAX_NUM);
    ubiRefMap = le_ref_InitStaticMap(ubiRefMap, TAF_FLASH_UBI_VOLUME_MAX_NUM);
}