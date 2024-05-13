/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <mtd/ubi-user.h>

#include "tafFlashAccess.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("tafFlashAccess Init...");
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    tafFlashAccess.Init();
    LE_INFO("tafFlashAccess Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Intiates flash access.
 */
//--------------------------------------------------------------------------------------------------
void taf_flash_Init
(
    void
)
{
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    tafFlashAccess.InitMtdPartitions();
    tafFlashAccess.InitUbiVolumes();
}

//--------------------------------------------------------------------------------------------------
/**
 * Open MTD partition.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdOpen
(
    const char* partitionNameStr,          ///< [IN] MTD partition name.
    taf_flash_OpenMode_t mode,             ///< [IN] Opening mode.
    taf_flash_PartitionRef_t* partitionRef ///< [OUT] The reference of MTD partition.
)
{
    TAF_ERROR_IF_RET_VAL(partitionNameStr == NULL, LE_BAD_PARAMETER, "Null ptr(partitionNameStr)");

    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    string partitionStr(partitionNameStr);
    auto it = tafFlashAccess.mtdDevMap.find(partitionStr);
    TAF_ERROR_IF_RET_VAL(it == tafFlashAccess.mtdDevMap.end(), LE_BAD_PARAMETER,
        "Invalid partition name %s.", partitionNameStr);

    int fd = open(it->second.devPath, O_RDWR);
    TAF_ERROR_IF_RET_VAL(fd < 0, LE_FAULT, "Fail to open %s", it->second.devPath);

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_mem_ForceAlloc(tafFlashAccess.mtdPool);
    TAF_ERROR_IF_RET_VAL(mtdInfo == NULL, LE_FAULT, "Fail to allocate memory from pool.");
    mtdInfo->fd = fd;

    *partitionRef = (taf_flash_PartitionRef_t)le_ref_CreateRef(tafFlashAccess.mtdRefMap,
        (void*)mtdInfo);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close MTD partition.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdClose
(
    taf_flash_PartitionRef_t partitionRef ///< [IN] The reference of MTD partition.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    TAF_ERROR_IF_RET_VAL(mtdInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    close(mtdInfo->fd);

    le_ref_DeleteRef(tafFlashAccess.mtdRefMap, partitionRef);

    le_mem_Release(mtdInfo);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdInformation
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t* blocksNumber,                ///< [OUT] Total blocks number.
    uint32_t* badBlocksNumber,             ///< [OUT] Bad blocks number.
    uint32_t* blockSize,                   ///< [OUT] Block size.
    uint32_t* pageSize                     ///< [OUT] Page size.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(blocksNumber == NULL, LE_BAD_PARAMETER, "Null ptr(blocksNumber)");

    TAF_ERROR_IF_RET_VAL(badBlocksNumber == NULL, LE_BAD_PARAMETER, "Null ptr(badBlocksNumber)");

    TAF_ERROR_IF_RET_VAL(blockSize == NULL, LE_BAD_PARAMETER, "Null ptr(blockSize)");

    TAF_ERROR_IF_RET_VAL(pageSize == NULL, LE_BAD_PARAMETER, "Null ptr(pageSize)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    TAF_ERROR_IF_RET_VAL(mtdInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    /* Get mtd information */
    int ret = ioctl(mtdInfo->fd, MEMGETINFO, &mtdInfo->info);
    TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Fail to open fd %d", mtdInfo->fd);

    *pageSize = mtdInfo->info.writesize;

    *blockSize = mtdInfo->info.erasesize;
    TAF_ERROR_IF_RET_VAL(*blockSize == 0, LE_FAULT, "Invalid para(block size is 0)");

    *blocksNumber = mtdInfo->info.size / mtdInfo->info.erasesize;

    *badBlocksNumber = 0;
    bool isBadBlock = false;
    for (uint32_t index = 0; index < *blocksNumber; index++)
    {
        TAF_ERROR_IF_RET_VAL(tafFlashAccess.IsMtdBadBlock(mtdInfo, index, &isBadBlock) != LE_OK,
            LE_FAULT, "Fail to check MTD block at %d.", index);

        if (isBadBlock)
        {
            (*badBlocksNumber)++;
            LE_WARN("Bad block at %d detected.", index);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase MTD block.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdEraseBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    bool isBadBlock = false;
    TAF_ERROR_IF_RET_VAL(tafFlashAccess.IsMtdBadBlock(mtdInfo, blockIndex, &isBadBlock) != LE_OK,
        LE_FAULT, "Fail to check MTD block at %d.", blockIndex);

    TAF_ERROR_IF_RET_VAL(isBadBlock, LE_FAULT, "Bad block detected.");

    erase_info_t eraseInfo;
    memset(&eraseInfo, 0, sizeof(erase_info_t));
    eraseInfo.start = blockIndex * mtdInfo->info.erasesize;
    eraseInfo.length = mtdInfo->info.erasesize;
    int ret = ioctl(mtdInfo->fd, MEMERASE, &eraseInfo);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to erase block with fd %d", mtdInfo->fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdReadPage
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t pageIndex,                    ///< [IN] Page index.
    uint8_t* readData,                     ///< [OUT] Buffer read from MTD page.
    size_t* sizePtr                        ///< [INOUT] Read size.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(readData == NULL, LE_BAD_PARAMETER, "Null ptr(readData)");

    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(sizePtr)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    TAF_ERROR_IF_RET_VAL(mtdInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(pageIndex >= mtdInfo->info.size / TAF_FLASH_MTD_PAGE_MAX_READ_SIZE,
        LE_FAULT, "Invalid page index %d.", pageIndex);

    lseek(mtdInfo->fd, pageIndex * mtdInfo->info.writesize, SEEK_SET);

    int ret = read(mtdInfo->fd, readData, *sizePtr);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to read page with fd %d", mtdInfo->fd);

    if (*sizePtr != (size_t)ret)
    {
        LE_INFO("Page %d: %d bytes is read, < %" PRIuS " bytes.", pageIndex, ret, *sizePtr);
        *sizePtr = ret;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD partition.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdRead
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t offset,                       ///< [IN] The offset of MTD partition.
    uint8_t* readData,                     ///< [OUT] Buffer read from MTD data.
    size_t* sizePtr                        ///< [INOUT] Read size.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(readData == NULL, LE_BAD_PARAMETER, "Null ptr(readData)");

    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(sizePtr)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    TAF_ERROR_IF_RET_VAL(mtdInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    lseek(mtdInfo->fd, offset, SEEK_SET);

    int ret = read(mtdInfo->fd, readData, *sizePtr);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to read page with fd %d", mtdInfo->fd);

    if (*sizePtr != (size_t)ret)
    {
        LE_INFO("Offset %d: %d bytes is read, < %" PRIuS " bytes.", offset, ret, *sizePtr);
        *sizePtr = ret;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdWritePage
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t pageIndex,                    ///< [IN] Page index.
    const uint8_t* writeData,              ///< [IN] Buffer to be written to MTD block.
    size_t size                            ///< [IN] Write size.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(writeData == NULL, LE_BAD_PARAMETER, "Null ptr(writeData)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    bool isBadBlock = false;
    TAF_ERROR_IF_RET_VAL(tafFlashAccess.IsMtdBadBlock(mtdInfo, 
        pageIndex * mtdInfo->info.writesize / mtdInfo->info.erasesize, &isBadBlock) != LE_OK,
        LE_FAULT, "Fail to check MTD block at %d.", pageIndex);

    TAF_ERROR_IF_RET_VAL(isBadBlock, LE_FAULT, "Bad block detected.");

    TAF_ERROR_IF_RET_VAL(pageIndex >= mtdInfo->info.size / TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE,
        LE_FAULT, "Invalid page index %d.", pageIndex);

    lseek(mtdInfo->fd, pageIndex * mtdInfo->info.writesize, SEEK_SET);

    int ret = write(mtdInfo->fd, writeData, size);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to write page with fd %d", mtdInfo->fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdWrite
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t offset,                       ///< [IN] The offset of MTD partition.
    const uint8_t* writeData,              ///< [IN] Buffer to be written to MTD block.
    size_t size                            ///< [IN] Write size.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(writeData == NULL, LE_BAD_PARAMETER, "Null ptr(writeData)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);

    lseek(mtdInfo->fd, offset, SEEK_SET);

    int ret = write(mtdInfo->fd, writeData, size);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to write data with fd %d", mtdInfo->fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check good block.
 */
//--------------------------------------------------------------------------------------------------
bool taf_flash_MtdIsBlockGood
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
)
{
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, false, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashMtdInfo_t* mtdInfo = (taf_FlashMtdInfo_t*)le_ref_Lookup(tafFlashAccess.mtdRefMap,
        partitionRef);
    bool isBadBlock = false;
    TAF_ERROR_IF_RET_VAL(tafFlashAccess.IsMtdBadBlock(mtdInfo, blockIndex, &isBadBlock) != LE_OK,
        false, "Fail to check MTD block at %d.", blockIndex);

    return !isBadBlock;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiOpen
(
    const char* volumeNameStr,       ///< [IN] UBI volume name.
    taf_flash_OpenMode_t mode,       ///< [IN] Opening mode.
    taf_flash_VolumeRef_t* volumeRef ///< [OUT] The reference of UBI volume.
)
{
    TAF_ERROR_IF_RET_VAL(volumeNameStr == NULL, LE_BAD_PARAMETER, "Null ptr(volumeNameStr)");

    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ref(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    string volumeStr(volumeNameStr);
    auto it = tafFlashAccess.ubiDevMap.find(volumeStr);
    TAF_ERROR_IF_RET_VAL(it == tafFlashAccess.ubiDevMap.end(), LE_BAD_PARAMETER,
        "Invalid volume name %s.", volumeNameStr);

    mode_t opMode;
    switch (mode)
	{
        case TAF_FLASH_READ_ONLY:
            opMode = O_RDONLY;
            break;
        case TAF_FLASH_READ_WRITE:
            opMode = O_RDWR;
            break;
        case TAF_FLASH_WRITE_ONLY:
            opMode = O_WRONLY;
            break;
        default:
            LE_ERROR("Invalid mode %d.", mode);
            return LE_BAD_PARAMETER;
    }

    int fd = open(it->second.devPath, opMode);
    TAF_ERROR_IF_RET_VAL(fd < 0, LE_FAULT, "Fail to open %s", it->second.devPath);

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_mem_ForceAlloc(tafFlashAccess.ubiPool);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_FAULT, "Fail to allocate memory from pool.");
    ubiInfo->fd = fd;
    ubiInfo->info = it->second;

    *volumeRef = (taf_flash_VolumeRef_t)le_ref_CreateRef(tafFlashAccess.ubiRefMap, (void*)ubiInfo);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiClose
(
    taf_flash_VolumeRef_t volumeRef ///< [IN] The reference of UBI volume.
)
{
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_ref_Lookup(tafFlashAccess.ubiRefMap,
        volumeRef);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    close(ubiInfo->fd);

    le_ref_DeleteRef(tafFlashAccess.ubiRefMap, volumeRef);

    le_mem_Release(ubiInfo);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiInformation
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    uint32_t* lebNumber,             ///< [OUT] Total logical erase blocks number.
    uint32_t* freeLebNumber,         ///< [OUT] Free logical erase blocks number.
    uint32_t* volumeSize             ///< [OUT] Volume size.
)
{
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    TAF_ERROR_IF_RET_VAL(lebNumber == NULL, LE_BAD_PARAMETER, "Null ptr(lebNumber)");

    TAF_ERROR_IF_RET_VAL(freeLebNumber == NULL, LE_BAD_PARAMETER, "Null ptr(freeLebNumber)");

    TAF_ERROR_IF_RET_VAL(volumeSize == NULL, LE_BAD_PARAMETER, "Null ptr(volumeSize)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_ref_Lookup(tafFlashAccess.ubiRefMap,
        volumeRef);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *lebNumber = ubiInfo->info.lebNum;
    *freeLebNumber = ubiInfo->info.freeLebNum;
    *volumeSize = ubiInfo->info.volSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiRead
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    uint32_t offset,                 ///< [IN] The offset of UBI volume.
    uint8_t* readData,               ///< [OUT] Buffer read from UBI volume.
    size_t* sizePtr                  ///< [INOUT] Read size.
)
{
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ref(volumeRef)");

    TAF_ERROR_IF_RET_VAL(readData == NULL, LE_BAD_PARAMETER, "Null ptr(readData)");

    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(sizePtr)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_ref_Lookup(tafFlashAccess.ubiRefMap,
        volumeRef);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    lseek(ubiInfo->fd, offset, SEEK_SET);
    int ret = read(ubiInfo->fd, readData, *sizePtr);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to read with fd %d", ubiInfo->fd);

    if (*sizePtr != (size_t)ret)
    {
        LE_INFO("Offset %d: %d bytes is read, < %" PRIuS " bytes.", offset, ret, *sizePtr);
        *sizePtr = ret;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate for writing UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiInitWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    int64_t writeSize                ///< [IN] The number of bytes set to write a UBI volume.
)
{
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ref(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_ref_Lookup(tafFlashAccess.ubiRefMap,
        volumeRef);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(ioctl(ubiInfo->fd, UBI_IOCVOLUP, &writeSize), LE_FAULT,
        "Fail to set write size with fd %d", ubiInfo->fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    const uint8_t* writeData,        ///< [IN] Buffer to be written to UBI volume.
    size_t size                      ///< [IN] Write size.
)
{
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    TAF_ERROR_IF_RET_VAL(writeData == NULL, LE_BAD_PARAMETER, "Null ptr(writeData)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    taf_FlashUbiInfo_t* ubiInfo = (taf_FlashUbiInfo_t*)le_ref_Lookup(tafFlashAccess.ubiRefMap,
        volumeRef);
    TAF_ERROR_IF_RET_VAL(ubiInfo == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    int ret = write(ubiInfo->fd, writeData, size);
    TAF_ERROR_IF_RET_VAL(ret < 0, LE_FAULT, "Fail to write with fd %d", ubiInfo->fd);

    return LE_OK;
}