/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "tafFlash.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Static map for partition reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(partitionRefMap, TAF_LIB_FLASH_PARTITION_MAX_NUM);

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
 * Intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_FlashAccess::Init
(
    void
)
{
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    tafFlashAccess.partitionRefMap = le_ref_InitStaticMap(partitionRefMap,
        TAF_LIB_FLASH_PARTITION_MAX_NUM);
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
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    le_result_t result = taf_lib_flash_GetPartitionList(&tafFlashAccess.partitionList);
    TAF_ERROR_IF_RET_NIL(result != LE_OK, "Fail to get partition list.");

    tafFlashAccess.partitionMap.clear();

    for (uint32_t i = 0; i < tafFlashAccess.partitionList.number; i++)
    {
        string partition(tafFlashAccess.partitionList.partition[i].name);
        tafFlashAccess.partitionMap.insert(make_pair(partition, i));
    }
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
    TAF_ERROR_IF_RET_VAL(partitionNameStr == NULL, LE_BAD_PARAMETER, "Null ptr(partitionNameStr)");

    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    string partitionStr(partitionNameStr);
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    auto it = tafFlashAccess.partitionMap.find(partitionStr);
    TAF_ERROR_IF_RET_VAL(it == tafFlashAccess.partitionMap.end(), LE_BAD_PARAMETER,
        "Invalid partition name %s.", partitionNameStr);

    le_result_t result = taf_lib_flash_OpenPartition(
        &tafFlashAccess.partitionList.partition[it->second], O_RDWR);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open partition %s", partitionNameStr);

    *partitionRef = (taf_flash_PartitionRef_t)le_ref_CreateRef(tafFlashAccess.partitionRefMap,
        (void*)&tafFlashAccess.partitionList.partition[it->second]);

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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_result_t result = taf_lib_flash_ClosePartition(partition);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close partition.");

    le_ref_DeleteRef(tafFlashAccess.partitionRefMap, partitionRef);

    return LE_OK;
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    TAF_ERROR_IF_RET_VAL(blocksNumber == NULL, LE_BAD_PARAMETER, "Null ptr(blocksNumber)");

    TAF_ERROR_IF_RET_VAL(badBlocksNumber == NULL, LE_BAD_PARAMETER, "Null ptr(badBlocksNumber)");

    TAF_ERROR_IF_RET_VAL(blockSize == NULL, LE_BAD_PARAMETER, "Null ptr(blockSize)");

    TAF_ERROR_IF_RET_VAL(pageSize == NULL, LE_BAD_PARAMETER, "Null ptr(pageSize)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    /* Get mtd information */
    le_result_t result = taf_lib_flash_GetMtdWriteSize(partition, pageSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD write size.");

    result = taf_lib_flash_GetMtdEraseSize(partition, blockSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD erase size.");
    TAF_ERROR_IF_RET_VAL(*blockSize == 0, LE_FAULT, "Invalid para(block size is 0)");

    uint32_t size;
    result = taf_lib_flash_GetMtdSize(partition, &size);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD partition size.");

    *blocksNumber = size / *blockSize;

    *badBlocksNumber = 0;
    bool isBadBlock = false;
    for (uint32_t index = 0; index < *blocksNumber; index++)
    {
        result = taf_lib_flash_IsMtdBadBlock(partition, index, &isBadBlock);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to check MTD block at %d.", index);

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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_EraseMtdBlock(partition, blockIndex);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    uint32_t blockSize = 0;
    le_result_t result = taf_lib_flash_GetMtdEraseSize(partition, &blockSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD erase size.");
    TAF_ERROR_IF_RET_VAL(blockSize == 0, LE_FAULT, "Invalid para(block size is 0)");

    uint32_t size = 0;
    result = taf_lib_flash_GetMtdSize(partition, &size);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD partition size.");

    uint32_t blockNum = size / blockSize;
    bool isBadBlock = false;
    for (uint32_t i = 0; i < blockNum; i++)
    {
        result = taf_lib_flash_IsMtdBadBlock(partition, i, &isBadBlock);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get block %d status.", i);

        if (isBadBlock)
        {
            LE_DEBUG("Detect bad block at %d.", i);
        }
        else
        {
            result = taf_lib_flash_EraseMtdBlock(partition, i);
            TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to erase block %d.", i);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_ReadPartition(partition, pageIndex * TAF_FLASH_MTD_PAGE_MAX_READ_SIZE,
        readData, sizePtr);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_ReadPartition(partition, offset, readData, sizePtr);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_WritePartition(partition, pageIndex * TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE,
        writeData, size);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, LE_BAD_PARAMETER, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_WritePartition(partition, offset, writeData, size);
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
    TAF_ERROR_IF_RET_VAL(partitionRef == NULL, false, "Null ptr(partitionRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, partitionRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, false, "Invalid para(null reference ptr)");

    bool isBadBlock = false;
    le_result_t result = taf_lib_flash_IsMtdBadBlock(partition, blockIndex, &isBadBlock);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, false, "Fail to check block status.");

    return !isBadBlock;
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
    TAF_ERROR_IF_RET_VAL(volumeNameStr == NULL, LE_BAD_PARAMETER, "Null ptr(volumeNameStr)");

    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    string partitionStr(volumeNameStr);
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    auto it = tafFlashAccess.partitionMap.find(partitionStr);
    TAF_ERROR_IF_RET_VAL(it == tafFlashAccess.partitionMap.end(), LE_BAD_PARAMETER,
        "Invalid volume name %s.", volumeNameStr);

    le_result_t result = taf_lib_flash_OpenPartition(
        &tafFlashAccess.partitionList.partition[it->second], O_RDWR);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open volume %s", volumeNameStr);

    *volumeRef = (taf_flash_VolumeRef_t)le_ref_CreateRef(tafFlashAccess.partitionRefMap,
        (void*)&tafFlashAccess.partitionList.partition[it->second]);

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
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, volumeRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_result_t result = taf_lib_flash_ClosePartition(partition);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close volume.");

    le_ref_DeleteRef(tafFlashAccess.partitionRefMap, volumeRef);

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
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    TAF_ERROR_IF_RET_VAL(lebNumber == NULL, LE_BAD_PARAMETER, "Null ptr(lebNumber)");

    TAF_ERROR_IF_RET_VAL(freeLebNumber == NULL, LE_BAD_PARAMETER, "Null ptr(freeLebNumber)");

    TAF_ERROR_IF_RET_VAL(volumeSize == NULL, LE_BAD_PARAMETER, "Null ptr(volumeSize)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, volumeRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    /* Get ubi information */
    le_result_t result = taf_lib_flash_GetUbiVolResvLebNum(partition, lebNumber);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI reserved LEB number.");

    result = taf_lib_flash_GetUbiAvailLebNum(partition, freeLebNumber);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI available LEB number.");

    result = taf_lib_flash_GetUbiVolSize(partition, volumeSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI volume size.");

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
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, volumeRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_ReadPartition(partition, offset, readData, sizePtr);
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
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, volumeRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_SetUbiVolUpSize(partition, writeSize);
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
    TAF_ERROR_IF_RET_VAL(volumeRef == NULL, LE_BAD_PARAMETER, "Null ptr(volumeRef)");

    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    taf_lib_flash_Partition_t* partition = (taf_lib_flash_Partition_t*)le_ref_Lookup(
        tafFlashAccess.partitionRefMap, volumeRef);
    TAF_ERROR_IF_RET_VAL(partition == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    return taf_lib_flash_WritePartition(partition, 0, writeData, size);
}