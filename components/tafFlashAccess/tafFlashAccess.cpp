/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <string.h>
#include <mtd/mtd-user.h>
#include <mtd/ubi-user.h>

#include "tafFlashAccess.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Get number from file.
 */
//--------------------------------------------------------------------------------------------------
void taf_lib_flash_GetNumFromFile
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
 * Check suffix of a string.
 *
 * @return
 *      - ture  If the string has suffix.
 *      - false If the string does not have suffix.
 */
//--------------------------------------------------------------------------------------------------
bool taf_lib_flash_HasSuffix
(
    const char* srcStr,   ///< [IN] Source string.
    const char* suffix ///< [IN] Suffix.
)
{
    size_t srcStrlen = strlen(srcStr);
    size_t suffixLen = strlen(suffix);

    if (suffixLen > srcStrlen)
        return false;

    if (strncmp(srcStr + srcStrlen - suffixLen, suffix, suffixLen) != 0)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get partition list.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If the list is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetPartitionList
(
    taf_lib_flash_PartitionList_t *listPtr ///< [OUT] Partition list.
)
{
    if (listPtr == NULL)
    {
        LE_ERROR("Partition list is NULL.");
        return LE_BAD_PARAMETER;
    }

    FILE *fp = fopen("/proc/mtd", "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open /proc/mtd.");
        return LE_FAULT;
    }

    uint32_t i = 0;
    char line[TAF_LIB_FLASH_MAX_LINE_LEN] = "";
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (strstr(line, "mtd") == NULL)
            continue;

        if (i >= TAF_LIB_FLASH_PARTITION_MAX_NUM)
        {
            LE_ERROR("Exceed partition number limit.");
            fclose(fp);
            return LE_FAULT;
        }

        char *saveptr;
        char *token = strtok_r(line, " ", &saveptr);
        if (token != NULL)
        {
            token[strlen(token) - 1] = '\0';
            le_utf8_Copy(listPtr->partition[i].mtdDevPath, "/dev/",
                TAF_LIB_FLASH_DEV_PATH_LEN, NULL);
            le_utf8_Append(listPtr->partition[i].mtdDevPath, token,
                TAF_LIB_FLASH_DEV_PATH_LEN, NULL);
        }

        token = strtok_r(NULL, " ", &saveptr);
        if (token != NULL)
        {
            listPtr->partition[i].size = strtol(token, NULL, 16);
        }

        token = strtok_r(NULL, " ", &saveptr);
        if (token != NULL)
        {
            listPtr->partition[i].eraseSize = strtol(token, NULL, 16);
            token = strtok_r(NULL, " ", &saveptr);
            if (token != NULL)
            {
                listPtr->partition[i].index = i;
                token[strlen(token) - 2] = '\0';
                le_utf8_Copy(listPtr->partition[i].name, token + 1,
                    TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN, NULL);

                i++;
            }
        }
    }

    fclose(fp);

    char* pathArrayPtr[] = {(char*)"/sys/devices/virtual/ubi", NULL};
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);
    FTSENT* entPtr;
    char* baseName;
    uint32_t j;
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
                    fp = fopen(entPtr->fts_path, "r");
                    if (fp != NULL)
                    {
                        if (fgets(line, sizeof(line), fp) != NULL)
                        {
                            line[strlen(line) - 1] = '\0';
                            for (j = 0; j < i; j++)
                            {
                                if (strncmp(line, listPtr->partition[j].name, strlen(line)) == 0)
                                    break;
                            }

                            if (j >= i)
                            {
                                LE_ERROR("Can not find MTD device for UBI volume %s.", line);
                                fclose(fp);
                                fts_close(ftsPtr);
                                return LE_FAULT;
                            }

                            char dirName[TAF_LIB_FLASH_UBI_DEV_INFO_PATH_LEN] = "";
                            le_path_GetDir(entPtr->fts_path, "/", dirName, sizeof(dirName));
                            dirName[strlen(dirName) - 1] = '\0';
                            char* ubiBase = le_path_GetBasenamePtr(dirName, "/");
                            le_utf8_Copy(listPtr->partition[j].ubiDevPath, "/dev/",
                                TAF_LIB_FLASH_DEV_PATH_LEN, NULL);
                            le_utf8_Append(listPtr->partition[j].ubiDevPath, ubiBase,
                                TAF_LIB_FLASH_DEV_PATH_LEN, NULL);

                            char infoFile[TAF_LIB_FLASH_UBI_DEV_INFO_PATH_LEN] = "";
                            le_utf8_Copy(infoFile, dirName, sizeof(infoFile), NULL);
                            le_utf8_Append(infoFile, "/usable_eb_size", sizeof(infoFile), NULL);
                            taf_lib_flash_GetNumFromFile(infoFile,
                                &listPtr->partition[j].eraseSize);

                            le_utf8_Copy(infoFile, dirName, sizeof(infoFile), NULL);
                            le_utf8_Append(infoFile, "/data_bytes", sizeof(infoFile), NULL);
                            taf_lib_flash_GetNumFromFile(infoFile, &listPtr->partition[j].size);
                        }

                        fclose(fp);
                    }
                    else
                    {
                        LE_ERROR("Can not open %s.", entPtr->fts_path);
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

    listPtr->number = i;
    for (i = 0; i < listPtr->number; i++)
    {
        listPtr->partition[i].mtdFd = -1;
        listPtr->partition[i].ubiFd = -1;
        listPtr->partition[i].mirrorIndex = TAF_LIB_FLASH_PARTITION_MAX_NUM;

        if (taf_lib_flash_HasSuffix(listPtr->partition[i].name, "_b"))
        {
            listPtr->partition[i].bank = DUAL_BANK_B;
        }
        else
        {
            listPtr->partition[i].bank = NOT_DUAL_BANK;
            continue;
        }

        for (j = 0; j < listPtr->number; j++)
        {
            if (i == j)
                continue;

            if (strncmp(listPtr->partition[i].name, listPtr->partition[j].name,
                strlen(listPtr->partition[i].name) - 2) == 0)
            {
                listPtr->partition[i].mirrorIndex = j;
                listPtr->partition[j].mirrorIndex = i;
                listPtr->partition[j].bank = DUAL_BANK_A;
                break;
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a partition.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_OpenPartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [INOUT] Partition.
    mode_t mode                              ///< [IN] Open mode.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
    {
        if (partitionPtr->mtdFd >= 0)
        {
            LE_WARN("MTD partition %s is openned.", partitionPtr->name);
            return LE_OK;
        }

        partitionPtr->mtdFd = open(partitionPtr->mtdDevPath, mode);
        partitionPtr->mode = mode;
        if (partitionPtr->mtdFd < 0)
        {
            LE_ERROR("Fail to open %s.", partitionPtr->mtdDevPath);
            return LE_FAULT;
        }
    }
    else
    {
        if (partitionPtr->ubiFd >= 0)
        {
            LE_WARN("UBI volume %s is openned.", partitionPtr->name);
            return LE_OK;
        }

        partitionPtr->ubiFd = open(partitionPtr->ubiDevPath, mode);
        partitionPtr->mode = mode;
        if (partitionPtr->ubiFd < 0)
        {
            LE_ERROR("Fail to open %s.", partitionPtr->ubiDevPath);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a partition.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_ClosePartition
(
    taf_lib_flash_Partition_t *partitionPtr ///< [INOUT] Partition.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
    {
        if (partitionPtr->mtdFd < 0)
        {
            LE_WARN("MTD partition %s is closed.", partitionPtr->name);
        }
        else
        {
            close(partitionPtr->mtdFd);
            partitionPtr->mtdFd = -1;
        }
    }
    else
    {
        if (partitionPtr->ubiFd < 0)
        {
            LE_WARN("UBI volume %s is closed.", partitionPtr->name);
        }
        else
        {
            close(partitionPtr->ubiFd);
            partitionPtr->ubiFd = -1;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD partition write size.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetMtdWriteSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr                        ///< [OUT] Minimal writable flash unit size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (sizePtr == NULL)
    {
        LE_ERROR("Size is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    mtd_info_t mtdInfo;
    int ret = ioctl(partitionPtr->mtdFd, MEMGETINFO, &mtdInfo);
    if (ret)
    {
        LE_ERROR("Fail to iotcl(MEMGETINFO) with fd%d,", partitionPtr->mtdFd);
        return LE_FAULT;
    }

    *sizePtr = mtdInfo.writesize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD partition erase size.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetMtdEraseSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr                        ///< [OUT] Erase block size of the partition.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (sizePtr == NULL)
    {
        LE_ERROR("Size is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    mtd_info_t mtdInfo;
    int ret = ioctl(partitionPtr->mtdFd, MEMGETINFO, &mtdInfo);
    if (ret)
    {
        LE_ERROR("Fail to iotcl(MEMGETINFO) with fd%d,", partitionPtr->mtdFd);
        return LE_FAULT;
    }

    *sizePtr = mtdInfo.erasesize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD partition size.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetMtdSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr                        ///< [OUT] Partition size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (sizePtr == NULL)
    {
        LE_ERROR("Size is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    mtd_info_t mtdInfo;
    int ret = ioctl(partitionPtr->mtdFd, MEMGETINFO, &mtdInfo);
    if (ret)
    {
        LE_ERROR("Fail to iotcl(MEMGETINFO) with fd%d,", partitionPtr->mtdFd);
        return LE_FAULT;
    }

    *sizePtr = mtdInfo.size;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a block is bad block in MTD partition..
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_IsMtdBadBlock
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [INOUT] Partition.
    uint32_t blockIndex,                     ///< [IN] Block index.
    bool* isBadBlock                         ///< [OUT] True if bad block, false if good block.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    loff_t page = (loff_t)(blockIndex * partitionPtr->eraseSize);
    int ret = ioctl(partitionPtr->mtdFd, MEMGETBADBLOCK, &page);
    if (ret != 0 && !(ret == -1 && errno == EOPNOTSUPP))
    {
        *isBadBlock = true;
    }
    else
    {
        *isBadBlock = false;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase a block in MTD partition.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_EraseMtdBlock
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t blockIndex                      ///< [IN] Block index.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    erase_info_t eraseInfo;
    memset(&eraseInfo, 0, sizeof(erase_info_t));
    eraseInfo.start = blockIndex * partitionPtr->eraseSize;
    eraseInfo.length = partitionPtr->eraseSize;
    int ret = ioctl(partitionPtr->mtdFd, MEMERASE, &eraseInfo);
    if (ret < 0)
    {
        LE_ERROR("Fail to iotcl(MEMERASE) with fd%d,", partitionPtr->mtdFd);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read partition.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_ReadPartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t offset,                         ///< [IN] Partition offset.
    uint8_t *dataPtr,                        ///< [OUT] Buffer read from partition.
    size_t *sizePtr                          ///< [INOUT] Buffer size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (dataPtr == NULL)
    {
        LE_ERROR("Buffer for reading data is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (sizePtr == NULL)
    {
        LE_ERROR("Read size is NULL.");
        return LE_BAD_PARAMETER;
    }

    int fd = -1;
    if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
    {
        fd = partitionPtr->mtdFd;
    }
    else
    {
        fd = partitionPtr->ubiFd;
    }

    if (fd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    lseek(fd, offset, SEEK_SET);
    int ret = read(fd, dataPtr, *sizePtr);
    if (ret < 0)
    {
        LE_ERROR("Fail to read with fd%d,", fd);
        return LE_FAULT;
    }

    if (*sizePtr != (size_t)ret)
    {
        LE_INFO("%d bytes at offset %d is read, < %" PRIuS " bytes.", ret, offset, *sizePtr);
        *sizePtr = ret;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write partition.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_WritePartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t offset,                         ///< [IN] Partition offset.
    const uint8_t *dataPtr,                  ///< [OUT] Buffer to be written on partition.
    size_t size                              ///< [IN] Buffer size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (dataPtr == NULL)
    {
        LE_ERROR("Buffer for writting data is NULL.");
        return LE_BAD_PARAMETER;
    }

    int fd = -1;
    if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
    {
        if (partitionPtr->mtdFd < 0)
        {
            LE_ERROR("MTD partition %s is closed.", partitionPtr->name);
            return LE_FAULT;
        }
        fd = partitionPtr->mtdFd;
        lseek(partitionPtr->mtdFd, offset, SEEK_SET);
    }
    else
    {
        if (partitionPtr->ubiFd < 0)
        {
            LE_ERROR("UBI volume %s is closed.", partitionPtr->name);
            return LE_FAULT;
        }
        fd = partitionPtr->ubiFd;
    }  

    int ret = write(fd, dataPtr, size);
    if (ret < 0)
    {
        LE_ERROR("Fail to write with fd%d,", fd);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume size.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetUbiVolSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *sizePtr                        ///< [OUT] Volume size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (sizePtr == NULL)
    {
        LE_ERROR("Size is NULL.");
        return LE_BAD_PARAMETER;
    }

    char *ubiBase = le_path_GetBasenamePtr(partitionPtr->ubiDevPath, "/");
    if (ubiBase == NULL)
    {
        LE_ERROR("Can not get UBI device base name from %s.", partitionPtr->ubiDevPath);
        return LE_FAULT;
    }

    char file[PATH_MAX] = "";
    snprintf(file, sizeof(file) - 1, "/sys/class/ubi/%s/data_bytes", ubiBase);

    taf_lib_flash_GetNumFromFile(file, &partitionPtr->size);
    *sizePtr = partitionPtr->size;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI reserved LEB number.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetUbiVolResvLebNum
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *numPtr                         ///< [OUT] Reserved LEB number.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    char *ubiBase = le_path_GetBasenamePtr(partitionPtr->ubiDevPath, "/");
    if (ubiBase == NULL)
    {
        LE_ERROR("Can not get UBI device base name from %s.", partitionPtr->ubiDevPath);
        return LE_FAULT;
    }

    char file[PATH_MAX] = "";
    snprintf(file, sizeof(file) - 1, "/sys/class/ubi/%s/reserved_ebs", ubiBase);

    taf_lib_flash_GetNumFromFile(file, numPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI available LEB number.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_GetUbiAvailLebNum
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *numPtr                         ///< [OUT] Available LEB number.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    char *ubiBase = le_path_GetBasenamePtr(partitionPtr->ubiDevPath, "/");
    if (ubiBase == NULL)
    {
        LE_ERROR("Can not get UBI device base name from %s.", partitionPtr->ubiDevPath);
        return LE_FAULT;
    }

    char file[PATH_MAX] = "";
    snprintf(file, sizeof(file) - 1, "/sys/class/ubi/%s/device/avail_eraseblocks", ubiBase);

    taf_lib_flash_GetNumFromFile(file, numPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set UBI volume update size.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_SetUbiVolUpSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    int64_t size                             ///< [IN] Volume update size.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->ubiFd < 0)
    {
        LE_ERROR("Partition is closed.");
        return LE_FAULT;
    }

    int ret = ioctl(partitionPtr->ubiFd, UBI_IOCVOLUP, &size);
    if (ret)
    {
        LE_ERROR("Fail to iotcl(UBI_IOCVOLUP) with fd%d,", partitionPtr->ubiFd);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase UBI volume.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_BAD_PARAMETER If partition is NULL.
 *      - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_lib_flash_EraseUbiVol
(
    taf_lib_flash_Partition_t *partitionPtr ///< [IN] Volume.
)
{
    if (partitionPtr == NULL)
    {
        LE_ERROR("Partition is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE ||
        partitionPtr->eraseSize == 0)
    {
        LE_ERROR("Not UBI volume.");
        return LE_FAULT;
    }

    bool isUbiOpen = false;
    if (partitionPtr->ubiFd > 0)
    {
        isUbiOpen = true;
        close(partitionPtr->ubiFd);
        partitionPtr->ubiFd = -1;
    }

    partitionPtr->mtdFd = open(partitionPtr->mtdDevPath, O_RDWR);
    if (partitionPtr->mtdFd < 0)
    {
        LE_ERROR("Fail to open %s.", partitionPtr->mtdDevPath);
        if (partitionPtr->ubiFd > 0)
        {
            partitionPtr->ubiFd = open(partitionPtr->ubiDevPath, partitionPtr->mode);
        }
        return LE_FAULT;
    }

    uint32_t i;
    int ret;
    struct erase_info_user64 eraseInfo;
    for (i = 0; i < partitionPtr->size / partitionPtr->eraseSize; i++)
    {
        eraseInfo.start = (uint64_t)i * partitionPtr->eraseSize;
        eraseInfo.length = (uint64_t)partitionPtr->eraseSize;
        ret = ioctl(partitionPtr->mtdFd, MEMERASE64, &eraseInfo);
        if (ret)
        {
            LE_ERROR("Fail to iotcl(MEMERASE64) at leb %d with fd%d,", i, partitionPtr->mtdFd);
        }
    }

    close(partitionPtr->mtdFd);
    partitionPtr->mtdFd = -1;

    if (isUbiOpen)
    {
        partitionPtr->ubiFd = open(partitionPtr->ubiDevPath, partitionPtr->mode);
    }

    return LE_OK;
}