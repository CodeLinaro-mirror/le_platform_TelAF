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

#include "taf_lib_flash.h"

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_lib_flash stub");
}

//--------------------------------------------------------------------------------------------------
/**
 * Intiates flash access reference map and memory pool.
 */
//--------------------------------------------------------------------------------------------------
void taf_flash_Init
(
    void
)
{

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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD block.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdReadBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex,                   ///< [IN] Logical block index.
    uint8_t* readData,                     ///< [OUT] Buffer read from MTD block.
    size_t* sizePtr                        ///< [INOUT] Read size.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD block.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_flash_MtdWriteBlock
(
    taf_flash_PartitionRef_t partitionRef, ///< [IN] The reference of MTD partition.
    uint32_t blockIndex,                   ///< [IN] Logical block index.
    const uint8_t* writeData,              ///< [IN] Buffer to be written to MTD block.
    size_t size                            ///< [IN] Write size.
)
{
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
    return true;
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
    long long writeSize              ///< [IN] The number of bytes set to write a UBI volume.
)
{
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
    return LE_OK;
}