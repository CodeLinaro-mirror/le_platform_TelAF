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

/*
 * @file       tafFlashUnitTest.c
 * @brief      This file implements unit test for Flash Access.
 */

#include "legato.h"
#include "taf_lib_flash.h"

#define FLASH_FILE_NAME_BYTES 256

#define MTD_TEST_PARTITION "abl_b"
#define UBI_TEST_VOLUME "telaf_b"

/*======================================================================
 FUNCTION        TestTafFlashInit
 DESCRIPTION     Initialization API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashInit(void)
{
    taf_flash_Init();
    LE_TEST_OK(true, "taf_flash_Init - void");
}

/*======================================================================
 FUNCTION        TestTafFlashMtdInfo
 DESCRIPTION     MTD information API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashMtdInfo(void)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    le_result_t result;

    LE_TEST_INFO("Start taf_flash_MtdInfo Test");

    // Open MTD Test
    result = taf_flash_MtdOpen(NULL, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdOpen - LE_BAD_PARAMETER");

    result = taf_flash_MtdOpen(MTD_TEST_PARTITION, TAF_FLASH_READ_WRITE, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdOpen - LE_BAD_PARAMETER");

    result = taf_flash_MtdOpen(MTD_TEST_PARTITION, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        NULL, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdInformation - LE_BAD_PARAMETER");

    result = taf_flash_MtdInformation( \
        partitionRef, NULL, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdInformation - LE_BAD_PARAMETER");

    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, NULL, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdInformation - LE_BAD_PARAMETER");

    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, NULL, &pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdInformation - LE_BAD_PARAMETER");

    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdInformation - LE_BAD_PARAMETER");

    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    LE_INFO("partition:        %s", MTD_TEST_PARTITION);
    LE_INFO("block number:     %d", blocksNumber);
    LE_INFO("bad block number: %d", badBlocksNumber);
    LE_INFO("block size:       %d", blockSize);
    LE_INFO("page size:        %d", pageSize);

    // Close MTD Test
    result = taf_flash_MtdClose(NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdClose - LE_BAD_PARAMETER");

    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

/*======================================================================
 FUNCTION        TestTafFlashMtdRead
 DESCRIPTION     MTD read API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashMtdRead(void)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    size_t bSize = 0, pSize = 0;
    bool isGoodBlock;
    le_result_t result;
    uint8_t block[TAF_FLASH_MTD_BLOCK_MAX_READ_SIZE] = { 0 };
    uint8_t page[TAF_FLASH_MTD_PAGE_MAX_READ_SIZE] = { 0 };
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;

    LE_TEST_INFO("Start taf_flash_MtdRead Test");

    // Open MTD Test
    result = taf_flash_MtdOpen(MTD_TEST_PARTITION, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    // Check MTD Block Test
    isGoodBlock = taf_flash_MtdIsBlockGood(partitionRef, 0);
    LE_TEST_OK(true, "taf_flash_MtdIsBlockGood - %d", isGoodBlock);

    // Read MTD Block Test
    result = taf_flash_MtdReadBlock(NULL, 0, block, &bSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadBlock - LE_BAD_PARAMETER");

    result = taf_flash_MtdReadBlock(partitionRef, 0, NULL, &bSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadBlock - LE_BAD_PARAMETER");

    result = taf_flash_MtdReadBlock(partitionRef, 0, block, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadBlock - LE_BAD_PARAMETER");

    LE_TEST_BEGIN_SKIP(!isGoodBlock, 1);
    result = taf_flash_MtdReadBlock(partitionRef, 0, block, &bSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdReadBlock - LE_OK");

    snprintf(file, sizeof(file), "/%s.bdat", MTD_TEST_PARTITION);
    result = le_fs_Open(file, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }
    result = le_fs_Write(fileRef, block, bSize);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to write file.");
    }
    le_fs_Close(fileRef);
    LE_INFO("Read block and write to %s with size %" PRIuS, file, bSize);
    LE_TEST_END_SKIP();

    // Read MTD Page Test
    pSize = (size_t)pageSize;
    result = taf_flash_MtdReadPage(NULL, 0, page, &pSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadPage - LE_BAD_PARAMETER");

    result = taf_flash_MtdReadPage(partitionRef, 0, NULL, &pSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadPage - LE_BAD_PARAMETER");

    result = taf_flash_MtdReadPage(partitionRef, 0, page, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdReadPage - LE_BAD_PARAMETER");

    LE_TEST_BEGIN_SKIP(!isGoodBlock, 1);
    result = taf_flash_MtdReadPage(partitionRef, 0, page, &pSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdReadPage - LE_OK");

    snprintf(file, sizeof(file), "/%s.pdat", MTD_TEST_PARTITION);
    result = le_fs_Open(file, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }
    result = le_fs_Write(fileRef, page, pSize);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to write file.");
    }
    le_fs_Close(fileRef);
    LE_INFO("Read page and write to %s with size %" PRIuS, file, pSize);
    LE_TEST_END_SKIP();

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

/*======================================================================
 FUNCTION        TestTafFlashMtdWrite
 DESCRIPTION     MTD write API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashMtdWrite(void)
{
    taf_flash_PartitionRef_t partitionRef;
    uint32_t blocksNumber = 0, badBlocksNumber = 0, blockSize = 0, pageSize = 0;
    size_t bSize = 0, pSize = 0;
    bool isGoodBlock;
    le_result_t result;
    uint8_t block[TAF_FLASH_MTD_BLOCK_MAX_WRITE_SIZE] = { 0 };
    uint8_t page[TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE] = { 0 };
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;

    LE_TEST_INFO("Start taf_flash_MtdRead Test");

    // Open MTD Test
    result = taf_flash_MtdOpen(MTD_TEST_PARTITION, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
    result = taf_flash_MtdInformation( \
        partitionRef, &blocksNumber, &badBlocksNumber, &blockSize, &pageSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdInformation - LE_OK");

    // Check MTD Block Test
    isGoodBlock = taf_flash_MtdIsBlockGood(partitionRef, 0);
    LE_TEST_OK(true, "taf_flash_MtdIsBlockGood - %d", isGoodBlock);

    // Write MTD Block Test
    result = taf_flash_MtdWriteBlock(NULL, 0, block, blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdWriteBlock - LE_BAD_PARAMETER");

    result = taf_flash_MtdWriteBlock(partitionRef, 0, NULL, blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdWriteBlock - LE_BAD_PARAMETER");

    result = taf_flash_MtdEraseBlock(NULL, 0);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdEraseBlock - LE_BAD_PARAMETER");

    LE_TEST_BEGIN_SKIP(!isGoodBlock, 2);
    snprintf(file, sizeof(file), "/%s.bdat", MTD_TEST_PARTITION);
    result = le_fs_Open(file, LE_FS_RDONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }
    result = le_fs_Read(fileRef, block, &bSize);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to read file.");
    }
    le_fs_Close(fileRef);

    result = taf_flash_MtdEraseBlock(partitionRef, 0);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdEraseBlock - LE_OK");

    result = taf_flash_MtdWriteBlock(partitionRef, 0, block, bSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdWriteBlock - LE_OK");

    LE_INFO("Read %s and write to block with size %" PRIuS, file, bSize);
    LE_TEST_END_SKIP();

    // Write MTD Page Test
    result = taf_flash_MtdWritePage(NULL, 0, page, pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdWritePage - LE_BAD_PARAMETER");

    result = taf_flash_MtdWritePage(partitionRef, 0, NULL, pageSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_MtdWritePage - LE_BAD_PARAMETER");

    LE_TEST_BEGIN_SKIP(!isGoodBlock, 1);
    snprintf(file, sizeof(file), "/%s.pdat", MTD_TEST_PARTITION);
    result = le_fs_Open(file, LE_FS_RDONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }

    pSize = (size_t)pageSize;
    result = le_fs_Read(fileRef, page, &pSize);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to read file.");
    }
    le_fs_Close(fileRef);

    result = taf_flash_MtdWritePage(partitionRef, 0, page, pSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdWritePage - LE_OK");

    LE_INFO("Read %s and write to page with size %" PRIuS, file, pSize);
    LE_TEST_END_SKIP();

    // Close MTD Test
    result = taf_flash_MtdClose(partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdClose - LE_OK");
}

/*======================================================================
 FUNCTION        TestTafFlashUbiInfo
 DESCRIPTION     UBI information API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashUbiInfo(void)
{
    taf_flash_VolumeRef_t volumeRef;
    uint32_t lebNumber = 0, freeLebNumber = 0, volumeSize = 0;
    le_result_t result;

    LE_TEST_INFO("Start taf_flash_UbiInfo Test");

    // Open UBI Test
    result = taf_flash_UbiOpen(NULL, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiOpen - LE_BAD_PARAMETER");

    result = taf_flash_UbiOpen(UBI_TEST_VOLUME, TAF_FLASH_READ_WRITE, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiOpen - LE_BAD_PARAMETER");

    result = taf_flash_UbiOpen(UBI_TEST_VOLUME, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // UBI Information Test
    result = taf_flash_UbiInformation(NULL, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiInformation - LE_BAD_PARAMETER");

    result = taf_flash_UbiInformation(volumeRef, NULL, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiInformation - LE_BAD_PARAMETER");

    result = taf_flash_UbiInformation(volumeRef, &lebNumber, NULL, &volumeSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiInformation - LE_BAD_PARAMETER");

    result = taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiInformation - LE_BAD_PARAMETER");

    result = taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInformation - LE_OK");

    LE_INFO("volume:          %s", UBI_TEST_VOLUME);
    LE_INFO("leb number:      %d", lebNumber);
    LE_INFO("free leb number: %d", freeLebNumber);
    LE_INFO("volume size:     %d", volumeSize);

    // Close UBI Test
    result = taf_flash_UbiClose(NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiClose - LE_BAD_PARAMETER");

    result = taf_flash_UbiClose(volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiClose - LE_OK");
}

/*======================================================================
 FUNCTION        TestTafFlashUbiRead
 DESCRIPTION     UBI read API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashUbiRead(void)
{
    taf_flash_VolumeRef_t volumeRef;
    le_result_t result;
    uint8_t block[TAF_FLASH_UBI_MAX_READ_SIZE] = { 0 };
    size_t blockSize = TAF_FLASH_UBI_MAX_READ_SIZE;
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;

    LE_TEST_INFO("Start taf_flash_UbiRead Test");

    // Open UBI Test
    result = taf_flash_UbiOpen(UBI_TEST_VOLUME, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // Read UBI Block Test
    result = taf_flash_UbiRead(NULL, 0, block, &blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiRead - LE_BAD_PARAMETER");

    result = taf_flash_UbiRead(volumeRef, 0, NULL, &blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiRead - LE_BAD_PARAMETER");

    result = taf_flash_UbiRead(volumeRef, 0, block, NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiRead - LE_BAD_PARAMETER");

    result = taf_flash_UbiRead(volumeRef, 0, block, &blockSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiRead - LE_OK");

    snprintf(file, sizeof(file), "/%s.bdat", UBI_TEST_VOLUME);
    result = le_fs_Open(file, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }
    result = le_fs_Write(fileRef, block, blockSize);
    if (result != LE_OK) {
        LE_ERROR("Fail to write file.");
    }
    le_fs_Close(fileRef);
    LE_INFO("Read block and write to %s with size %" PRIuS, file, blockSize);

    // Close UBI Test
    result = taf_flash_UbiClose(volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiClose - LE_OK");
}

/*======================================================================
 FUNCTION        TestTafFlashUbiWrite
 DESCRIPTION     UBI write API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFlashUbiWrite(void)
{
    taf_flash_VolumeRef_t volumeRef;
    le_result_t result;
    uint8_t block[TAF_FLASH_UBI_MAX_WRITE_SIZE] = { 0 };
    size_t blockSize = TAF_FLASH_UBI_MAX_READ_SIZE;
    char file[FLASH_FILE_NAME_BYTES] = { 0 };
    le_fs_FileRef_t fileRef;

    LE_TEST_INFO("Start taf_flash_UbiWrite Test");

    // Open UBI Test
    result = taf_flash_UbiOpen(UBI_TEST_VOLUME, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // Write UBI Block Test
    result = taf_flash_UbiInitWrite(NULL, blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiInitWrite - LE_BAD_PARAMETER");

    result = taf_flash_UbiInitWrite(volumeRef, blockSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInitWrite - LE_OK");

    snprintf(file, sizeof(file), "/%s.bdat", UBI_TEST_VOLUME);
    result = le_fs_Open(file, LE_FS_RDONLY, &fileRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open file.");
    }
    result = le_fs_Read(fileRef, block, &blockSize);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to read file.");
    }
    le_fs_Close(fileRef);

    result = taf_flash_UbiWrite(NULL, block, blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiWrite - LE_BAD_PARAMETER");

    result = taf_flash_UbiWrite(volumeRef, NULL, blockSize);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_flash_UbiWrite - LE_BAD_PARAMETER");

    result = taf_flash_UbiWrite(volumeRef, block, blockSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiWrite - LE_OK");

    LE_INFO("Read %s and write to block with size %" PRIuS, file, blockSize);

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
    LE_TEST_PLAN(59);

    LE_TEST_INFO("======== Flash MTD Intialization Test ========");
    TestTafFlashInit();
    LE_TEST_INFO("======== Flash MTD Information Test ========");
    TestTafFlashMtdInfo();
    LE_TEST_INFO("======== Flash MTD Read Test ========");
    TestTafFlashMtdRead();
    LE_TEST_INFO("======== Flash MTD Write Test ========");
    TestTafFlashMtdWrite();
    LE_TEST_INFO("======== Flash UBI Information Test ========");
    TestTafFlashUbiInfo();
    LE_TEST_INFO("======== Flash UBI Read Test ========");
    TestTafFlashUbiRead();
    LE_TEST_INFO("======== Flash UBI Write Test ========");
    TestTafFlashUbiWrite();

    LE_TEST_EXIT;
}