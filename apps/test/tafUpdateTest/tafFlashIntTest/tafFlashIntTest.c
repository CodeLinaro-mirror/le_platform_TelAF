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

#include "legato.h"
#include "interfaces.h"

#define FLASH_FILE_NAME_BYTES 256

/*======================================================================
 FUNCTION        PrintHelpMenu
 DESCRIPTION     Print help menue
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void PrintHelpMenu()
{
    LE_INFO("Please run \"app runProc tafFlashIntTest tafFlashIntTest -- [option]\"");
    LE_INFO("Description:");
    LE_INFO("help                   : Print help menu.");
    LE_INFO("mtd info [partittion]  : Show MTD partition information.");
    LE_INFO("mtd read [partittion]  : Read MTD partition and write to .mtd_data file.");
    LE_INFO("mtd write [partittion] : Write MTD partition with .mtd_data file.");
    LE_INFO("mtd erase [partittion] : Erase MTD partition blocks.");
    LE_INFO("ubi info [volume]      : Show UBI volume information.");
    LE_INFO("ubi read [volume]      : Read UBI volume and write to .bdat file.");
    LE_INFO("ubi write [volume]     : Write UBI volume with .bdat file.");
}

/*======================================================================
 FUNCTION        GetMtdInfo
 DESCRIPTION     Get MTD information
 PARAMETERS      partition : Partition name
 RETURN VALUE    void
======================================================================*/
void GetMtdInfo(const char* partition)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    le_result_t result;

    // Open MTD Test
    result = taf_flash_MtdOpen(partition, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    LE_INFO("partition:        %s", partition);
    LE_INFO("block number:     %d", blocksNumber);
    LE_INFO("bad block number: %d", badBlocksNumber);
    LE_INFO("block size:       %d", blockSize);
    LE_INFO("page size:        %d", pageSize);

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD page.
 */
//--------------------------------------------------------------------------------------------------
void ReadMtdPage
(
    const char* partition ///< [IN] MTD partition name.
)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    size_t pSize = TAF_FLASH_MTD_PAGE_MAX_READ_SIZE;
    uint8_t page[TAF_FLASH_MTD_PAGE_MAX_READ_SIZE] = { 0 };
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;
    uint32_t i;
    le_result_t result;

    // Open MTD Test
    result = taf_flash_MtdOpen(partition, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    snprintf(file, sizeof(file), "/%s.mtd_data", partition);
    result = le_fs_Open(file, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file %s.", file);
    }

    // Read MTD Page Test
    for (i = 0; i < blockSize / pageSize * blocksNumber; i++)
    {
        result = taf_flash_MtdReadPage(partitionRef, i, page, &pSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_MtdReadPage - LE_OK");

        result = le_fs_Write(fileRef, page, pSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to write file %s.", file);
        }
    }

    result = le_fs_Close(fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close file %s.", file);
    }

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD page.
 */
//--------------------------------------------------------------------------------------------------
void WriteMtdPage
(
    const char* partition ///< [IN] MTD partition name.
)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    size_t pSize = TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE;
    uint8_t page[TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE] = { 0 };
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;
    uint32_t i;
    le_result_t result;

    // Open MTD Test
    result = taf_flash_MtdOpen(partition, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    snprintf(file, sizeof(file), "/%s.mtd_data", partition);

    result = le_fs_Open(file, LE_FS_RDONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file %s.", file);
    }

    // Write MTD Page Test
    for (i = 0; i < blockSize / pageSize * blocksNumber; i++)
    {
        result = le_fs_Read(fileRef, page, &pSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read file %s.", file);
        }

        result = taf_flash_MtdWritePage(partitionRef, i, page, pSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_MtdWritePage - LE_OK");
    }

    result = le_fs_Close(fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close file %s.", file);
    }

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

/*======================================================================
 FUNCTION        EraseMtdBlock
 DESCRIPTION     Erase MTD block
 PARAMETERS      partition : Partition name
 RETURN VALUE    void
======================================================================*/
void EraseMtdBlock(const char* partition)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    uint32_t i;
    bool isGoodBlock;
    le_result_t result;

    // Open MTD Test
    result = taf_flash_MtdOpen(partition, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    // Erase MTD Block Test
    for (i = 0; i < blocksNumber; i++)
    {
        isGoodBlock = taf_flash_MtdIsBlockGood(partitionRef, i);
        LE_TEST_OK(true, "taf_flash_MtdIsBlockGood - %d", isGoodBlock);
        if (isGoodBlock)
        {
            result = taf_flash_MtdEraseBlock(partitionRef, i);
            LE_TEST_OK((result == LE_OK), "taf_flash_MtdEraseBlock - LE_OK");

            LE_INFO("Partition %s block %d erased.", partition, i);
        }
        else
        {
            LE_INFO("Bad block %d detected.", i);
        }
    }

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

/*======================================================================
 FUNCTION        GetUbiInfo
 DESCRIPTION     Get UBI information
 PARAMETERS      volume : Volume name
 RETURN VALUE    void
======================================================================*/
void GetUbiInfo(const char* volume)
{
    taf_flash_VolumeRef_t volumeRef;
    uint32_t lebNumber = 0, freeLebNumber = 0, volumeSize = 0;
    le_result_t result;

    // Open UBI Test
    result = taf_flash_UbiOpen(volume, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // UBI Information Test
    result = taf_flash_UbiInformation( \
        volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInformation - LE_OK");

    LE_INFO("volume:          %s", volume);
    LE_INFO("leb number:      %d", lebNumber);
    LE_INFO("free leb number: %d", freeLebNumber);
    LE_INFO("volume size:     %d", volumeSize);

    // Close UBI Test
    result = taf_flash_UbiClose(volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiClose - LE_OK");
}

/*======================================================================
 FUNCTION        ReadUbiLeb
 DESCRIPTION     Read UBI leb
 PARAMETERS      volume : Volume name
 RETURN VALUE    void
======================================================================*/
void ReadUbiLeb(const char* volume)
{
    taf_flash_VolumeRef_t volumeRef;
    uint32_t lebNumber = 0, freeLebNumber = 0, volumeSize = 0;
    uint8_t block[TAF_FLASH_UBI_MAX_READ_SIZE] = { 0 };
    size_t blockSize = TAF_FLASH_UBI_MAX_READ_SIZE;
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;
    uint32_t i;
    le_result_t result;

    // Open UBI Test
    result = taf_flash_UbiOpen(volume, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // UBI Information Test
    result = taf_flash_UbiInformation( \
        volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInformation - LE_OK");

    snprintf(file, sizeof(file), "/%s.ubi_data", volume);

    result = le_fs_Open(file, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file %s.", file);
    }

    // Read UBI Leb Test
    for (i = 0; i < volumeSize / TAF_FLASH_UBI_MAX_READ_SIZE; i++)
    {
        result = taf_flash_UbiRead(volumeRef, i * blockSize, block, &blockSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_UbiRead - LE_OK");

        result = le_fs_Write(fileRef, block, blockSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to write file %s.", file);
        }

        LE_INFO("Read leb %d and write to %s with size %" PRIuS, i, file, blockSize);
    }

    blockSize = volumeSize % TAF_FLASH_UBI_MAX_READ_SIZE;
    if (blockSize != 0)
    {
        result = taf_flash_UbiRead(volumeRef, volumeSize - blockSize, block, &blockSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_UbiRead - LE_OK");

        result = le_fs_Write(fileRef, block, blockSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to write file %s.", file);
        }
    }

    result = le_fs_Close(fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close file %s.", file);
    }

    // Close UBI Test
    result = taf_flash_UbiClose(volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiClose - LE_OK");
}

/*======================================================================
 FUNCTION        WriteUbiLeb
 DESCRIPTION     Write UBI block
 PARAMETERS      volume : Volume name
 RETURN VALUE    void
======================================================================*/
void WriteUbiLeb(const char* volume)
{
    taf_flash_VolumeRef_t volumeRef;
    uint32_t lebNumber = 0, freeLebNumber = 0, volumeSize = 0;
    uint8_t block[TAF_FLASH_UBI_MAX_WRITE_SIZE] = { 0 };
    size_t blockSize = TAF_FLASH_UBI_MAX_WRITE_SIZE;
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;
    uint32_t i;
    le_result_t result;

    // Open UBI Test
    result = taf_flash_UbiOpen(volume, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // UBI Information Test
    result = taf_flash_UbiInformation( \
        volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInformation - LE_OK");

    // Write UBI Leb Test
    result = taf_flash_UbiInitWrite(volumeRef, volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInitWrite - LE_OK");

    snprintf(file, sizeof(file), "/%s.ubi_data", volume);
    result = le_fs_Open(file, LE_FS_RDONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file %s.", file);
    }

    for (i = 0; i < volumeSize / TAF_FLASH_UBI_MAX_WRITE_SIZE; i++)
    {
        blockSize = TAF_FLASH_UBI_MAX_WRITE_SIZE;
        memset(block, 0xFF, blockSize);
        result = le_fs_Read(fileRef, block, &blockSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read file %s.", file);
        }

        result = taf_flash_UbiWrite(volumeRef, block, blockSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_UbiWrite - LE_OK");

        LE_INFO("Read %s and write to block %d with size %" PRIuS, file, i, blockSize);
    }

    blockSize = volumeSize % TAF_FLASH_UBI_MAX_WRITE_SIZE;
    if (blockSize != 0)
    {
        memset(block, 0xFF, blockSize);
        result = le_fs_Read(fileRef, block, &blockSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read file %s.", file);
        }

        result = taf_flash_UbiWrite(volumeRef, block, blockSize);
        LE_TEST_OK((result == LE_OK), "taf_flash_UbiWrite - LE_OK");
    }

    result = le_fs_Close(fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close file %s.", file);
    }

    // Close UBI Test
    result = taf_flash_UbiClose(volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiClose - LE_OK");
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    const char* type = le_arg_GetArg(0);

    taf_flash_Init();
    LE_TEST_OK(true, "taf_flash_Init - void");

    if (strncmp(type, "mtd", strlen("mtd")) == 0)
    {
        const char* cmd = le_arg_GetArg(1);
        const char* partition = le_arg_GetArg(2);
        if (partition != NULL)
        {
            if (strncmp(cmd, "info", strlen("info")) == 0)
            {
                LE_TEST_INFO("======== MTD Info Test ========");
                GetMtdInfo(partition);
            }
            else if (strncmp(cmd, "read", strlen("read")) == 0)
            {
                LE_TEST_INFO("======== MTD Read Test ========");
                ReadMtdPage(partition);
            }
            else if (strncmp(cmd, "write", strlen("write")) == 0)
            {
                LE_TEST_INFO("======== MTD Write Test ========");
                WriteMtdPage(partition);
            }
            else if (strncmp(cmd, "erase", strlen("erase")) == 0)
            {
                LE_TEST_INFO("======== MTD Erase Test ========");
                EraseMtdBlock(partition);
            }
        }
    }
    else if (strncmp(type, "ubi", strlen("ubi")) == 0)
    {
        const char* cmd = le_arg_GetArg(1);
        const char* volume = le_arg_GetArg(2);
        if (volume != NULL)
        {
            if (strncmp(cmd, "info", strlen("info")) == 0)
            {
                LE_TEST_INFO("======== UBI Info Test ========");
                GetUbiInfo(volume);
            }
            else if (strncmp(cmd, "read", strlen("read")) == 0)
            {
                LE_TEST_INFO("======== UBI Read Test ========");
                ReadUbiLeb(volume);
            }
            else if (strncmp(cmd, "write", strlen("write")) == 0)
            {
                LE_TEST_INFO("======== UBI Write Test ========");
                WriteUbiLeb(volume);
            }
        }
    }
    else
    {
        PrintHelpMenu();
    }

    LE_TEST_EXIT;
}