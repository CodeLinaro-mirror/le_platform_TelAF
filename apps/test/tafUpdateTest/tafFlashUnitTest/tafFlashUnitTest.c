/*
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

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
    result = taf_flash_MtdOpen(MTD_TEST_PARTITION, TAF_FLASH_READ_WRITE, &partitionRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdOpen - LE_OK");

    // MTD Information Test
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
    size_t pSize = 0;
    bool isGoodBlock;
    le_result_t result;
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

    // Read MTD Page Test
    pSize = (size_t)pageSize;
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
    size_t pSize = 0;
    bool isGoodBlock;
    le_result_t result;
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

    result = taf_flash_MtdEraseBlock(partitionRef, 0);
    LE_TEST_OK((result == LE_OK), "taf_flash_MtdEraseBlock - LE_OK");

    // Write MTD Page Test
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
    result = taf_flash_UbiOpen(UBI_TEST_VOLUME, TAF_FLASH_READ_WRITE, &volumeRef);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiOpen - LE_OK");

    // UBI Information Test
    result = taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    LE_TEST_OK((result == LE_OK), "taf_flash_UbiInformation - LE_OK");

    LE_INFO("volume:          %s", UBI_TEST_VOLUME);
    LE_INFO("leb number:      %d", lebNumber);
    LE_INFO("free leb number: %d", freeLebNumber);
    LE_INFO("volume size:     %d", volumeSize);

    // Close UBI Test
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
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

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