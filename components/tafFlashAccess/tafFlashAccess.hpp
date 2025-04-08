/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_FLASH_ACCESS_HPP
#define TAF_FLASH_ACCESS_HPP

#include "legato.h"

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Maximum line length.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_MAX_LINE_LEN 64

//--------------------------------------------------------------------------------------------------
/**
 * Path length for device.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_DEV_PATH_LEN 16

//--------------------------------------------------------------------------------------------------
/**
 * Path length for ubi device info.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_UBI_DEV_INFO_PATH_LEN 64

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of partitions.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_PARTITION_MAX_NUM 80

//--------------------------------------------------------------------------------------------------
/**
 * Maximum name length of partitions.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN 32

//--------------------------------------------------------------------------------------------------
/**
 * MTD block size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_MTD_BLOCK_SIZE 0x40000

//--------------------------------------------------------------------------------------------------
/**
 * UBI block size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_LIB_FLASH_UBI_BLOCK_SIZE 0x3e000

//--------------------------------------------------------------------------------------------------
/**
 * Bank enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    NOT_DUAL_BANK,
    DUAL_BANK_A,
    DUAL_BANK_B
} taf_lib_flash_Bank_t;

//--------------------------------------------------------------------------------------------------
/**
 * Partition structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t index;                                  ///< Partition index.
    char name[TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN]; ///< Partition name.
    uint32_t ubiVolCount;                            ///< UBI volume count on the partition.
    char mtdDevPath[TAF_LIB_FLASH_DEV_PATH_LEN];     ///< MTD device path.
    char ubiDevPath[TAF_LIB_FLASH_DEV_PATH_LEN];     ///< UBI device path.
    int mtdFd;                                       ///< File descriptor for MTD device.
    int ubiFd;                                       ///< File descriptor for UBI device.
    mode_t mode;                                     ///< Device open mode.
    taf_lib_flash_Bank_t bank;                       ///< The bank of partition.
    uint32_t mirrorIndex;                            ///< Mirror partition index, valid for
                                                     ///  dual bank.
    uint32_t size;                                   ///< Total size of the partition.
    uint32_t eraseSize;                              ///< Erase block size of the partition.
} taf_lib_flash_Partition_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure of partition list.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t number;                                                      ///< Partition number.
    taf_lib_flash_Partition_t partition[TAF_LIB_FLASH_PARTITION_MAX_NUM]; ///< Partition.
} taf_lib_flash_PartitionList_t;

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
le_result_t taf_lib_flash_GetPartitionList
(
    taf_lib_flash_PartitionList_t *listPtr ///< [OUT] Partition list.
);

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
le_result_t taf_lib_flash_OpenPartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [INOUT] Partition.
    mode_t mode,                             ///< [IN] Open mode.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_ClosePartition
(
    taf_lib_flash_Partition_t *partitionPtr ///< [IN] Partition.
);

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
le_result_t taf_lib_flash_GetMtdWriteSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr,                       ///< [OUT] Minimal writable flash unit size.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_GetMtdEraseSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr,                       ///< [OUT] Erase block size of the partition.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_GetMtdSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t *sizePtr,                       ///< [OUT] Partition size.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_IsMtdBadBlock
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t blockIndex,                     ///< [IN] Block index.
    bool* isBadBlock,                        ///< [OUT] True if bad block, false if good block.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_EraseMtdBlock
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t blockIndex,                     ///< [IN] Block index.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_ReadPartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t offset,                         ///< [IN] Partition offset.
    uint8_t *dataPtr,                        ///< [OUT] Buffer read from partition.
    size_t *sizePtr,                         ///< [INOUT] Buffer size.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_WritePartition
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Partition.
    uint32_t offset,                         ///< [IN] Partition offset.
    const uint8_t *dataPtr,                  ///< [OUT] Buffer to be written on partition.
    size_t size,                             ///< [IN] Buffer size.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_GetUbiVolSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *sizePtr                        ///< [OUT] Volume size.
);

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
le_result_t taf_lib_flash_GetUbiVolResvLebNum
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *numPtr                         ///< [OUT] Reserved LEB number.
);

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
le_result_t taf_lib_flash_GetUbiAvailLebNum
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    uint32_t *numPtr                         ///< [OUT] Available LEB number.
);

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
le_result_t taf_lib_flash_SetUbiVolUpSize
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    int64_t size,                            ///< [IN] Volume update size.
    int *errCode                             ///< [OUT] Error code.
);

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
le_result_t taf_lib_flash_EraseUbiVol
(
    taf_lib_flash_Partition_t *partitionPtr, ///< [IN] Volume.
    int *errCode                             ///< [OUT] Error code.
);

#ifdef __cplusplus
}
#endif

#endif