/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <fstream>
#include <string>
#include <unordered_set>
#include <mutex>
#include "legato.h"
#include "interfaces.h"
#include "tafFlash.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Static map for MTD reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(mtdMap, MAX_MTD_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for MTD structure.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(mtdPool, MAX_MTD_NUM, sizeof(taf_pa_flash_MtdRef_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static map for UBI reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ubiMap, MAX_UBI_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for UBI structure.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ubiPool, MAX_UBI_NUM, sizeof(taf_flash_Ubi_t));

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

bool IsValidPartitionName(const std::string& name)
{
    static std::unordered_set<std::string> partitionNameSet;
    static std::once_flag initFlag;

    std::call_once(initFlag, []()
    {
        std::ifstream file("/proc/mtd");
        if (!file.is_open())
        {
            LE_ERROR("Failed to open /proc/mtd");
            return;
        }

        std::string line;
        std::getline(file, line); // Skip header line
        while (std::getline(file, line))
        {
            auto firstQuote = line.find('"'); // position of the first ' " ' in the name
            auto lastQuote = line.rfind('"'); // position of the last  ' " ' in the name
            if (firstQuote != std::string::npos &&
                lastQuote != std::string::npos &&
                lastQuote > firstQuote)
            {
                std::string partName = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                partitionNameSet.insert(partName);
            }
        }
    });

    return partitionNameSet.find(name) != partitionNameSet.end();
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
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();

    tafFlashAccess.mtdMap = le_ref_InitStaticMap(mtdMap, MAX_MTD_NUM);
    tafFlashAccess.mtdPool = le_mem_InitStaticPool(mtdPool,
        MAX_MTD_NUM, sizeof(taf_pa_flash_MtdRef_t));

    tafFlashAccess.ubiMap = le_ref_InitStaticMap(ubiMap, MAX_UBI_NUM);
    tafFlashAccess.ubiPool = le_mem_InitStaticPool(ubiPool,
        MAX_UBI_NUM, sizeof(taf_flash_Ubi_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Intiates MTD partitions and UBI volumes for flash access.
 */
//--------------------------------------------------------------------------------------------------
void taf_flash_Init
(
    void
)
{
    pa_result_t ret = taf_pa_flash_Init();
    if (ret)
        LE_ERROR("Flash PA init ret %d.", ret);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open MTD partition, read, write and get information operation can be done with a MTD partition
 * reference.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdOpen
(
    const char* partitionNameStr,          ///< [IN] MTD partition name.
    taf_flash_OpenMode_t mode,             ///< [IN] Opening mode.
    taf_flash_PartitionRef_t* partitionRef ///< [OUT] The reference of MTD partition.
)
{
    if (partitionNameStr == NULL)
    {
        LE_ERROR("Null ptr(partitionNameStr)");
        return LE_BAD_PARAMETER;
    }

    if(!IsValidPartitionName(partitionNameStr))
    {
        LE_ERROR("Invalid partition name %s", partitionNameStr);
        return LE_BAD_PARAMETER;
    }

    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    taf_pa_flash_MtdRef_t mtdRef = nullptr;
    pa_result_t ret = taf_pa_flash_OpenMtd(partitionNameStr, TAF_PA_FLASH_BITMASK_OPEN_MODE_READ_WRITE,
        &mtdRef);
    if (ret)
    {
        LE_ERROR("Fail to open MTD %s, ret %d.", partitionNameStr, ret);
        return LE_FAULT;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr =
        (taf_pa_flash_MtdRef_t*)le_mem_ForceAlloc(tafFlashAccess.mtdPool);
    *mtdRefPtr = mtdRef;
    *partitionRef = (taf_flash_PartitionRef_t)le_ref_CreateRef(tafFlashAccess.mtdMap,
        (void*)mtdRefPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a MTD partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdClose
(
    taf_flash_PartitionRef_t partitionRef ///< [IN] The reference of MTD partition.
)
{
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    le_result_t result = LE_OK;
    pa_result_t ret =taf_pa_flash_CloseMtd(*mtdRefPtr);
    if (ret)
    {
        LE_ERROR("Fail to close MTD, ret %d.", ret);
        result = LE_FAULT;
    }

    le_ref_DeleteRef(tafFlashAccess.mtdMap, partitionRef);
    le_mem_Release(mtdRefPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD information, including the total number of erasable blocks, the number of bad blocks,
 * the size of a block and the size of a page.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdInformation
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t* blocksNumber,                ///< [OUT] Total number of erasable blocks
    uint32_t* badBlocksNumber,             ///< [OUT] Number of bad blocks.
    uint32_t* blockSize,                   ///< [OUT] The size of a block.
    uint32_t* pageSize                     ///< [OUT] The size of a page.
)
{
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    if (blocksNumber == NULL || badBlocksNumber == NULL || blockSize == NULL || pageSize == NULL)
    {
        LE_ERROR("Null input parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    /* Get mtd information */
    taf_pa_flash_MtdInfo_t info;
    pa_result_t ret = taf_pa_flash_GetMtdInfo(*mtdRefPtr, &info);
    if (ret)
    {
        LE_ERROR("Fail to get MTD information, ret %d.", ret);
        return LE_FAULT;
    }

    *pageSize = info.writeSize;
    *blockSize = info.eraseSize;
    if (*blockSize == 0)
    {
        LE_ERROR("Invalid block size.");
        return LE_FAULT;
    }

    *blocksNumber = info.size / info.eraseSize;

    *badBlocksNumber = 0;
    for (uint32_t index = 0; index < *blocksNumber; index++)
    {
        bool isGood = false;
        pa_result_t ret = taf_pa_flash_CheckMtdGoodBlock(*mtdRefPtr, index, &isGood);
        if (ret == 0 && !isGood)
        {
            (*badBlocksNumber)++;
            LE_WARN("Bad block at %d detected.", index);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase a block in MTD partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdEraseBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
)
{
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_EraseMtdBlock(*mtdRefPtr, blockIndex);
    if (ret)
    {
        LE_ERROR("Fail to erase block %d, ret = %d.", blockIndex, ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase a MTD partition, this function will skip the bad blocks.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdErase
(
    taf_flash_PartitionRef_t partitionRef ///< [IN] The reference of MTD partition.
)
{
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    /* Get mtd information */
    taf_pa_flash_MtdInfo_t info;
    pa_result_t ret = taf_pa_flash_GetMtdInfo(*mtdRefPtr, &info);
    if (ret)
    {
        LE_ERROR("Fail to get MTD information, ret %d.", ret);
        return LE_FAULT;
    }

    if (info.eraseSize == 0)
    {
        LE_ERROR("Invalid block size.");
        return LE_FAULT;
    }

    uint32_t blocksNumber = info.size / info.eraseSize;
    for (uint32_t i = 0; i < blocksNumber; i++)
    {
        bool isGood = false;
        ret = taf_pa_flash_CheckMtdGoodBlock(*mtdRefPtr, i, &isGood);
        if (ret)
            LE_ERROR("Fail to get block %d status, ret = %d.", i, ret);
        else
        {
            if (isGood)
            {
                ret = taf_pa_flash_EraseMtdBlock(*mtdRefPtr, i);
                if (ret)
                    LE_ERROR("Fail to erase block %d, ret = %d.", i, ret);
            }
            else
                LE_WARN("Bad block at %d detected.", i);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from a MTD page.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
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
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_ReadMtdPage(*mtdRefPtr, pageIndex, readData, sizePtr);
    if (ret && ret != PAGE_ERASED)
    {
        LE_ERROR("Fail to read MTD page %d.", pageIndex);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from a MTD partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
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
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    /* Get mtd information */
    taf_pa_flash_MtdInfo_t info;
    pa_result_t ret = taf_pa_flash_GetMtdInfo(*mtdRefPtr, &info);
    if (ret)
    {
        LE_ERROR("Fail to get MTD information.");
        return LE_FAULT;
    }

    if (info.writeSize == 0)
    {
        LE_ERROR("Invalid page size.");
        return LE_FAULT;
    }

    uint32_t index = offset / info.writeSize;
    uint32_t start = offset % info.writeSize;
    unsigned char* buffer = (unsigned char*)malloc(info.writeSize);
    size_t pageSize = info.writeSize;
    ret = taf_pa_flash_ReadMtdPage(*mtdRefPtr, index, buffer, &pageSize);
    if (ret && ret != PAGE_ERASED)
    {
        LE_ERROR("Fail to read the MTD page %d.", index);
        free(buffer);
        return LE_FAULT;
    }

    /* Read first page with offset */
    size_t rdSize = *sizePtr;
    if (rdSize + start <= info.writeSize)
    {
        memcpy(readData, buffer + start, rdSize);
        free(buffer);
        return LE_OK;
    }

    /* Read the reset of bytes */
    free(buffer);
    rdSize = rdSize + start - info.writeSize;
    offset = info.writeSize - start;
    index++;
    while (rdSize > 0)
    {
        if (rdSize > info.writeSize)
        {
            pageSize = info.writeSize;
            ret = taf_pa_flash_ReadMtdPage(*mtdRefPtr, index, readData + offset, &pageSize);

            rdSize -= info.writeSize;
            offset += info.writeSize;
        }
        else
        {
            ret = taf_pa_flash_ReadMtdPage(*mtdRefPtr, index, readData + offset, &rdSize);

            rdSize = 0;
        }

        if (ret && ret != PAGE_ERASED)
        {
            LE_ERROR("Fail to read MTD page %d.", index);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to a MTD page.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
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
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_WriteMtdPage(*mtdRefPtr, pageIndex, writeData, size);
    if (ret)
    {
        LE_ERROR("Fail to write MTD page %d.", pageIndex);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to a MTD partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a partition reference is not found.
 *      - LE_FAULT         On other error
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
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    /* Get mtd information */
    taf_pa_flash_MtdInfo_t info;
    pa_result_t ret = taf_pa_flash_GetMtdInfo(*mtdRefPtr, &info);
    if (ret)
    {
        LE_ERROR("Fail to get MTD information.");
        return LE_FAULT;
    }

    if (info.writeSize == 0)
    {
        LE_ERROR("Invalid page size.");
        return LE_FAULT;
    }

    uint32_t index = offset / info.writeSize;
    uint32_t start = offset % info.writeSize;
    unsigned char* buffer = (unsigned char*)malloc(info.writeSize);
    size_t pageSize = info.writeSize;
    ret = taf_pa_flash_ReadMtdPage(*mtdRefPtr, index, buffer, &pageSize);
    if (ret && ret != PAGE_ERASED)
    {
        LE_ERROR("Fail to read the MTD page %d.", index);
        free(buffer);
        return LE_FAULT;
    }

    /* Write first page with offset */
    if (size + start <= info.writeSize)
    {
        memcpy(buffer + start, writeData, size);
        ret = taf_pa_flash_WriteMtdPage(*mtdRefPtr, index, buffer, info.writeSize);
        free(buffer);
        if (ret)
        {
            LE_ERROR("Fail to read the MTD page %d.", index);
            return LE_FAULT;
        }

        return LE_OK;
    }

    /* Write the reset of bytes */
    free(buffer);
    size = size + start - info.writeSize;
    offset = info.writeSize - start;
    index++;
    while (size > 0)
    {
        if (size > info.writeSize)
        {
            ret = taf_pa_flash_WriteMtdPage(*mtdRefPtr, index, writeData + offset, info.writeSize);

            size -= info.writeSize;
            offset += info.writeSize;
        }
        else
        {
            ret = taf_pa_flash_WriteMtdPage(*mtdRefPtr, index, writeData + offset, size);

            size = 0;
        }

        if (ret)
        {
            LE_ERROR("Fail to write MTD page %d.", index);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a block is good block in MTD partition.
 *
 * @return
 *      - ture             Good block
 *      - false            Bad block or other error
 */
//--------------------------------------------------------------------------------------------------
bool taf_flash_MtdIsBlockGood
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
)
{
    if (partitionRef == NULL)
    {
        LE_ERROR("Null ptr(partitionRef)");
        return false;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_pa_flash_MtdRef_t* mtdRefPtr = (taf_pa_flash_MtdRef_t*)le_ref_Lookup(
        tafFlashAccess.mtdMap, partitionRef);
    if (mtdRefPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return false;
    }

    bool isGood = false;
    pa_result_t ret = taf_pa_flash_CheckMtdGoodBlock(*mtdRefPtr, blockIndex, &isGood);

    if (ret)
        LE_ERROR("Failed to check MTD block at %d.", blockIndex);

    return isGood;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI volume, read, write and get information operation can be done with a UBI volume
 * reference.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiOpen
(
    const char* volumeNameStr,       ///< [IN] UBI volume name.
    taf_flash_OpenMode_t mode,       ///< [IN] Opening mode.
    taf_flash_VolumeRef_t* volumeRef ///< [OUT] The reference of UBI volume.
)
{
    if (volumeNameStr == NULL)
    {
        LE_ERROR("Null ptr(volumeNameStr)");
        return LE_BAD_PARAMETER;
    }

    if(!IsValidPartitionName(volumeNameStr))
    {
        LE_ERROR("Invalid partition name %s", volumeNameStr);
        return LE_BAD_PARAMETER;
    }

    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    taf_pa_flash_UbiVolumeRef_t ubiRef = nullptr;
    pa_result_t ret = 0;
    taf_pa_flash_OpenModeBitMask_t bitmask = 0x0;
    switch (mode)
    {
        case TAF_FLASH_READ_ONLY:
            bitmask = TAF_PA_FLASH_BITMASK_OPEN_MODE_READ_ONLY;
            break;
        case TAF_FLASH_WRITE_ONLY:
            bitmask = TAF_PA_FLASH_BITMASK_OPEN_MODE_WRITE_ONLY;
            break;
        case TAF_FLASH_READ_WRITE:
            bitmask = TAF_PA_FLASH_BITMASK_OPEN_MODE_READ_WRITE;
            break;
        default:
            LE_ERROR("Invalid open mode %d for %s", mode, volumeNameStr);
            return LE_BAD_PARAMETER;
    }

    ret = taf_pa_flash_OpenUbiVolume(volumeNameStr, bitmask, &ubiRef);
    if (ret)
    {
        LE_ERROR("Fail to open UBI %s.", volumeNameStr);
        return LE_FAULT;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_mem_ForceAlloc(tafFlashAccess.ubiPool);
    ubiPtr->mode = bitmask;
    ubiPtr->ubiRef = ubiRef;
    le_utf8_Copy(ubiPtr->name, volumeNameStr, TAF_FLASH_VOLUME_NAME_MAX_BYTES, NULL);
    *volumeRef = (taf_flash_VolumeRef_t)le_ref_CreateRef(tafFlashAccess.ubiMap, (void*)ubiPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close UBI volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiClose
(
    taf_flash_VolumeRef_t volumeRef ///< [IN] The reference of UBI volume.
)
{
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_CloseUbiVolume(ubiPtr->ubiRef);

    le_ref_DeleteRef(tafFlashAccess.ubiMap, volumeRef);
    le_mem_Release(ubiPtr);

    if (ret)
    {
        LE_ERROR("Fail to close UBI.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI information, including the total number of lebs (logical erase blocks), the number of
 * free lebs and the size of the volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
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
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    if (lebNumber == NULL || freeLebNumber == NULL || volumeSize == NULL)
    {
        LE_ERROR("Null input parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    taf_pa_flash_UbiVolumeInfo_t info;
    /* Get ubi information */
    pa_result_t ret = taf_pa_flash_GetUbiVolumeInfo(ubiPtr->ubiRef, &info);
    if (ret)
    {
        LE_ERROR("Fail to get UBI volume information.");
        return LE_FAULT;
    }

    *lebNumber = info.reservedLebs;
    *freeLebNumber = info.availLebs;
    *volumeSize = info.size;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from a UBI volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 *      - LE_FAULT         On other error
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
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_ReadUbiVolume(ubiPtr->ubiRef, offset, readData, sizePtr);
    if (ret < 0)
    {
        LE_ERROR("Fail to read UBI at offset %d, ret = %d.", offset, ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the writable data length of a UBI volume.
 *
 * @note This function should be called once before writting a UBI volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiInitWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    int64_t writeSize                ///< [IN] The number of bytes set to write a UBI volume.
)
{
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_SetUbiVolumeUpdateSize(ubiPtr->ubiRef, writeSize);
    if (ret)
    {
        LE_ERROR("Fail to set UBI volume update size");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to a UBI volume.
 *
 * @note User should aware of the context when writing data to a UBI volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    const uint8_t* writeData,        ///< [IN] Buffer to be written to UBI volume.
    size_t size                      ///< [IN] Write size.
)
{
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_UpdateUbiVolume(ubiPtr->ubiRef, writeData, size);
    if (ret)
    {
        LE_ERROR("Fail to update UBI");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase UBI volume.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If a volume reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_UbiErase
(
    taf_flash_VolumeRef_t volumeRef ///< [IN] The reference of UBI volume.
)
{
    if (volumeRef == NULL)
    {
        LE_ERROR("Null ptr(volumeRef)");
        return LE_BAD_PARAMETER;
    }

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_flash_Ubi_t* ubiPtr = (taf_flash_Ubi_t*)le_ref_Lookup(tafFlashAccess.ubiMap, volumeRef);
    if (ubiPtr == NULL)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    pa_result_t ret = taf_pa_flash_CloseUbiVolume(ubiPtr->ubiRef);
    if (ret)
    {
        LE_ERROR("Fail to close UBI.");
        return LE_FAULT;
    }

    ret = taf_pa_flash_EraseUbiVolume(ubiPtr->name);
    if (ret)
    {
        LE_ERROR("Fail to erase UBI %s.", ubiPtr->name);
        return LE_FAULT;
    }

    ret = taf_pa_flash_OpenUbiVolume(ubiPtr->name, ubiPtr->mode, &ubiPtr->ubiRef);
    if (ret)
    {
        LE_ERROR("Fail to reopen UBI %s.", ubiPtr->name);
        return LE_FAULT;
    }

    return LE_OK;
}
