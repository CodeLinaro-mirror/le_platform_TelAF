/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TAF_LIB_FLASH_H
#define TAF_LIB_FLASH_H

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum partition name length with null character.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_PARTITION_NAME_MAX_BYTES 128

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to write data to MTD block.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_MTD_BLOCK_MAX_WRITE_SIZE 262144

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to read data from MTD block.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_MTD_BLOCK_MAX_READ_SIZE 262144

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to write data to MTD page.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE 4096

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to read data from MTD page.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_MTD_PAGE_MAX_READ_SIZE 4096

//--------------------------------------------------------------------------------------------------
/**
 * Maximum volume name length with null character.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_VOLUME_NAME_MAX_BYTES 64

//--------------------------------------------------------------------------------------------------
/**
 * Maximum volume type length with null character.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_VOLUME_TYPE_MAX_BYTES 128

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to read data from UBI volume.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_UBI_MAX_READ_SIZE 253952

//--------------------------------------------------------------------------------------------------
/**
 * Maximum byte size for buffer to write data to UBI volume.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FLASH_UBI_MAX_WRITE_SIZE 253952

//--------------------------------------------------------------------------------------------------
/**
 * The reference of a partition.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_flash_Partition* taf_flash_PartitionRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * The reference of a volume.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_flash_Volume* taf_flash_VolumeRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Open mode enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_FLASH_READ_ONLY = 1,
        ///< Read only mode.
    TAF_FLASH_WRITE_ONLY = 2,
        ///< Write only mode.
    TAF_FLASH_READ_WRITE = 3
        ///< Read and write mode.
}
taf_flash_OpenMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Intiates flash access reference map and memory pool.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_flash_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open MTD partition.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdOpen
(
    const char* partitionNameStr,          ///< [IN] MTD partition name.
    taf_flash_OpenMode_t mode,             ///< [IN] Opening mode.
    taf_flash_PartitionRef_t* partitionRef ///< [OUT] The reference of MTD partition.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close MTD partition.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdClose
(
    taf_flash_PartitionRef_t partitionRef ///< [IN] The reference of MTD partition.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdInformation
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t* blocksNumber,                ///< [OUT] Total blocks number.
    uint32_t* badBlocksNumber,             ///< [OUT] Bad blocks number.
    uint32_t* blockSize,                   ///< [OUT] Block size.
    uint32_t* pageSize                     ///< [OUT] Page size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Erase MTD block.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdEraseBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD block.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdReadBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex,                   ///< [IN] Logical block index.
    uint8_t* readData,                     ///< [OUT] Buffer read from MTD block.
    size_t* sizePtr                        ///< [INOUT] Read size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD block.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdWriteBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex,                   ///< [IN] Logical block index.
    const uint8_t* writeData,              ///< [IN] Buffer to be written to MTD block.
    size_t size                            ///< [IN] Write size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD page.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdReadPage
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t pageIndex,                    ///< [IN] Page index.
    uint8_t* readData,                     ///< [OUT] Buffer read from MTD page.
    size_t* sizePtr                        ///< [INOUT] Read size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD page.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_MtdWritePage
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t pageIndex,                    ///< [IN] Page index.
    const uint8_t* writeData,              ///< [IN] Buffer to be written to MTD block.
    size_t size                            ///< [IN] Write size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Check good block.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool taf_flash_MtdIsBlockGood
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex                    ///< [IN] Logical block index.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI volume.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiOpen
(
    const char* volumeNameStr,       ///< [IN] UBI volume name.
    taf_flash_OpenMode_t mode,       ///< [IN] Opening mode.
    taf_flash_VolumeRef_t* volumeRef ///< [OUT] The reference of UBI volume.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close UBI volume.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiClose
(
    taf_flash_VolumeRef_t volumeRef ///< [IN] The reference of UBI volume.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiInformation
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    uint32_t* lebNumber,             ///< [OUT] Total logical erase blocks number.
    uint32_t* freeLebNumber,         ///< [OUT] Free logical erase blocks number.
    uint32_t* volumeSize             ///< [OUT] Volume size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read UBI volume.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiRead
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    uint32_t offset,                 ///< [IN] The offset of UBI volume.
    uint8_t* readData,               ///< [OUT] Buffer read from UBI volume.
    size_t* sizePtr                  ///< [INOUT] Read size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate for writing UBI volume.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiInitWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    int64_t writeSize                ///< [IN] The number of bytes set to write a UBI volume.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write UBI volume.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_flash_UbiWrite
(
    taf_flash_VolumeRef_t volumeRef, ///< [IN] The reference of UBI volume.
    const uint8_t* writeData,        ///< [IN] Buffer to be written to UBI volume.
    size_t size                      ///< [IN] Write size.
);

#endif
