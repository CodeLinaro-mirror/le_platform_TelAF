/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Prints remote MTD partition information.
 */
//--------------------------------------------------------------------------------------------------
void RemoteMtdInfo
(
    const char* partName ///< [IN] MTD partition name.
)
{
    uint32_t blocksNumber = 0;
    uint32_t badBlocksNumber = 0;
    uint32_t blockSize = 0;
    uint32_t pageSize = 0;

    rpc_taf_flash_PartitionRef_t partitionRef;
    le_result_t result = rpc_taf_flash_MtdOpen(partName, RPC_TAF_FLASH_READ_WRITE, &partitionRef);
    if (result != LE_OK)
    {
        printf("Can not open remote MTD partition %s.", partName);
        return;
    }

    result = rpc_taf_flash_MtdInformation(partitionRef, &blocksNumber, &badBlocksNumber,
        &blockSize, &pageSize);
    if (result != LE_OK)
    {
        printf("Can not get remote MTD partition %s information.", partName);
        return;
    }

    printf("\nRemote MTD partition %s information :\n", partName);
    printf("Block number     : %d\n", blocksNumber);
    printf("Bad block number : %d\n", badBlocksNumber);
    printf("Block size       : %d\n", blockSize);
    printf("Page size        : %d\n", pageSize);

    if (badBlocksNumber != 0)
    {
        printf("Scanning blocks...\n");
        uint32_t i;
        for (i = 0; i < blocksNumber; i++)
        {
            if (!rpc_taf_flash_MtdIsBlockGood(partitionRef, i))
            {
                printf("Detect bad block at %d.\n", i);
            }
        }
    }

    result = rpc_taf_flash_MtdClose(partitionRef);
    if (result != LE_OK)
    {
        printf("Can not close remote MTD partition %s.", partName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints remote UBI volume information.
 */
//--------------------------------------------------------------------------------------------------
void RemoteUbiInfo
(
    const char* volumeName ///< [IN] UBI volume name.
)
{
    uint32_t lebNumber = 0;
    uint32_t freeLebNumber = 0;
    uint32_t volumeSize = 0;

    rpc_taf_flash_VolumeRef_t volumeRef;
    le_result_t result = rpc_taf_flash_UbiOpen(volumeName, RPC_TAF_FLASH_READ_WRITE, &volumeRef);
    if (result != LE_OK)
    {
        printf("Can not open remote UBI volume %s.", volumeName);
        return;
    }

    result = rpc_taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber,
        &volumeSize);
    if (result != LE_OK)
    {
        printf("Can not get remote UBI volume %s information.", volumeName);
        return;
    }

    printf("\nRemote UBI volume %s information :\n", volumeName);
    printf("LEB  number     : %d\n", lebNumber);
    printf("Free LEB number : %d\n", freeLebNumber);
    printf("Volume size     : %d\n", volumeSize);

    result = rpc_taf_flash_UbiClose(volumeRef);
    if (result != LE_OK)
    {
        printf("Can not close remote UBI volume %s.", volumeName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase remote MTD partition.
 */
//--------------------------------------------------------------------------------------------------
void EraseRemoteMtd
(
    const char* partName ///< [IN] MTD partition name.
)
{
    rpc_taf_flash_PartitionRef_t partitionRef;

    le_result_t result = rpc_taf_flash_MtdOpen(partName, RPC_TAF_FLASH_READ_WRITE, &partitionRef);
    if (result != LE_OK)
    {
        printf("Can not open remote MTD partition %s.", partName);
        return;
    }

    printf("\nErasing remote MTD partition %s...\n", partName);

    result = rpc_taf_flash_MtdErase(partitionRef);
    if (result != LE_OK)
    {
        printf("Can not erase remote MTD partition %s.", partName);
    }
    else
    {
        printf("Remote MTD partition %s is erased.\n", partName);
    }

    result = rpc_taf_flash_MtdClose(partitionRef);
    if (result != LE_OK)
    {
        printf("Can not close remote MTD partition %s.", partName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read remote MTD partition.
 */
//--------------------------------------------------------------------------------------------------
void ReadRemoteMtd
(
    const char* partName, ///< [IN] MTD partition name.
    const char* filePath  ///< [IN] File to store partition data.
)
{
    uint32_t blocksNumber = 0;
    uint32_t badBlocksNumber = 0;
    uint32_t blockSize = 0;
    uint32_t pageSize = 0;
    rpc_taf_flash_PartitionRef_t partitionRef;
    uint8_t mtdData[RPC_TAF_FLASH_MTD_MAX_READ_SIZE];

    le_result_t result = rpc_taf_flash_MtdOpen(partName, RPC_TAF_FLASH_READ_WRITE, &partitionRef);
    if (result != LE_OK)
    {
        printf("Can not open remote MTD partition %s.", partName);
        return;
    }

    result = rpc_taf_flash_MtdInformation(partitionRef, &blocksNumber, &badBlocksNumber,
        &blockSize, &pageSize);
    if (result != LE_OK)
    {
        printf("Can not get remote MTD partition %s information.", partName);
        return;
    }

    if (access(filePath, 0) == 0)
    {
        printf("Removing existing file %s.", filePath);
        unlink(filePath);
    }

    int fd = open(filePath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        printf("Can not open file %s.", filePath);
        return;
    }

    uint32_t i;
    size_t pSize;
    uint32_t percent = 100;
    uint32_t leftBytes = blocksNumber * blockSize % RPC_TAF_FLASH_MTD_MAX_READ_SIZE;
    uint32_t dataBlockNum = blocksNumber * blockSize / RPC_TAF_FLASH_MTD_MAX_READ_SIZE;

    for (i = 0; i < dataBlockNum; i++)
    {
        pSize = RPC_TAF_FLASH_MTD_MAX_READ_SIZE;
        result = rpc_taf_flash_MtdRead(partitionRef,
            i * RPC_TAF_FLASH_MTD_MAX_READ_SIZE, mtdData, &pSize);
        if (result != LE_OK)
        {
            printf("Can not to read remote MTD partition %s at index %d.", partName, i);
            return;
        }

        int rc;
        do
        {
            rc = write(fd, mtdData, RPC_TAF_FLASH_MTD_MAX_READ_SIZE);
        } while ((-1 == rc) && (EINTR == errno));

        if (i * 100 / dataBlockNum != percent)
        {
            percent = i * 100 / dataBlockNum;
            printf("Reading remote MTD partition %s %d%% ...\n", partName, percent);
        }
    }

    if (leftBytes != 0)
    {
        printf("Reading %d bytes from remote MTD partition %s.", leftBytes, partName);
        pSize = leftBytes;
        result = rpc_taf_flash_MtdRead(partitionRef,
            dataBlockNum * RPC_TAF_FLASH_MTD_MAX_READ_SIZE, mtdData, &pSize);
        if (result != LE_OK)
        {
            printf("Can not to read the rest of remote MTD partition %s.", partName);
            return;
        }

        int rc;
        do
        {
            rc = write(fd, mtdData, leftBytes);
        } while ((-1 == rc) && (EINTR == errno));
    }

    close(fd);

    result = rpc_taf_flash_MtdClose(partitionRef);
    if (result != LE_OK)
    {
        printf("Can not close remote MTD partition %s.", partName);
    }

    printf("Reading remote MTD partition %s completed.\n", partName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read remote UBI volume.
 */
//--------------------------------------------------------------------------------------------------
void ReadRemoteUbi
(
    const char* partName, ///< [IN] UBI volume name.
    const char* filePath  ///< [IN] File to store volume data.
)
{
    uint32_t lebNumber = 0;
    uint32_t freeLebNumber = 0;
    uint32_t volumeSize = 0;
    uint8_t ubiData[RPC_TAF_FLASH_UBI_MAX_READ_SIZE];
    rpc_taf_flash_VolumeRef_t volumeRef;

    le_result_t result = rpc_taf_flash_UbiOpen(partName, RPC_TAF_FLASH_READ_WRITE, &volumeRef);
    if (result != LE_OK)
    {
        printf("Can not open remote UBI volume %s.", partName);
        return;
    }

    result = rpc_taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    if (result != LE_OK)
    {
        printf("Can not get remote UBI volume %s information.", partName);
        return;
    }

    if (access(filePath, 0) == 0)
    {
        printf("Removing existing file %s.", filePath);
        unlink(filePath);
    }

    int fd = open(filePath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        printf("Can not open file %s.", filePath);
        return;
    }

    uint32_t i;
    uint32_t percent = 100;
    for (i = 0; i < volumeSize / RPC_TAF_FLASH_UBI_MAX_READ_SIZE; i++)
    {
        size_t pSize = RPC_TAF_FLASH_UBI_MAX_READ_SIZE;
        result = rpc_taf_flash_UbiRead(volumeRef, i * RPC_TAF_FLASH_UBI_MAX_READ_SIZE,
            ubiData, &pSize);
        if (result != LE_OK)
        {
            printf("Can not to read remote UBI volume %s at page %d.", partName, i);
            return;
        }

        int rc;
        do
        {
            rc = write(fd, ubiData, RPC_TAF_FLASH_UBI_MAX_READ_SIZE);
        } while ((-1 == rc) && (EINTR == errno));

        if (i * 100 / (volumeSize / TAF_FLASH_UBI_MAX_READ_SIZE) != percent)
        {
            percent = i * 100 / (volumeSize / TAF_FLASH_UBI_MAX_READ_SIZE);
            printf("Reading remote UBI volume %s %d%% ...\n", partName, percent);
        }

    }

    uint32_t leftBytes = volumeSize % RPC_TAF_FLASH_UBI_MAX_READ_SIZE;
    if (leftBytes != 0)
    {
        printf("Reading %d bytes from remote UBI volume %s.\n", leftBytes, partName);
        size_t pSize = leftBytes;
        result = rpc_taf_flash_UbiRead(volumeRef, i * RPC_TAF_FLASH_UBI_MAX_READ_SIZE,
            ubiData, &pSize);
        if (result != LE_OK)
        {
            printf("Can not to read the rest of remote UBI volume %s.", partName);
            return;
        }

        int rc;
        do
        {
            rc = write(fd, ubiData, leftBytes);
        } while ((-1 == rc) && (EINTR == errno));
    }

    result = rpc_taf_flash_UbiClose(volumeRef);
    if (result != LE_OK)
    {
        printf("Can not close remote UBI volume %s.", partName);
    }

    close(fd);

    printf("Reading remote UBI volume %s completed.\n", partName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write remote MTD partition.
 */
//--------------------------------------------------------------------------------------------------
void WriteRemoteMtd
(
    const char* partName, ///< [IN] MTD partition name.
    const char* filePath  ///< [IN] MTD image file.
)
{
    uint32_t flashNum = 0;
    uint32_t i = 0;
    uint32_t percent = 100;
    uint8_t mtdData[RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE];

    FILE *fp = fopen(filePath, "r");
    if (fp == NULL)
    {
        printf("File %s not exists.", filePath);
        return;
    }

    struct stat status;
    if (stat(filePath, &status) != 0)
    {
        printf("Can not get file %s status.", filePath);
        fclose(fp);
        return;
    }

    rpc_taf_flash_PartitionRef_t partitionRef;
    le_result_t result = rpc_taf_flash_MtdOpen(partName, RPC_TAF_FLASH_READ_WRITE, &partitionRef);
    if (result != LE_OK)
    {
        printf("Can not open remote MTD partition %s.", partName);
        fclose(fp);
        return;
    }

    if (status.st_size % RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE)
    {
        printf("MTD image %s is not aligned with flash size.\n", filePath);
        flashNum = status.st_size / RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE + 1;
    }
    else
    {
        flashNum = status.st_size / RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE;
    }

    for (i = 0; i < flashNum; i++)
    {
        int ret = fread(mtdData, 1, RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE, fp);
        if (ret < RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE)
        {
            memset(mtdData + ret, 0xFF, RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE - ret);
            printf("Padding 0xFF in remote MTD %s index %d, start at %d.\n", partName, i, ret);
        }

        result = rpc_taf_flash_MtdWrite(partitionRef, i * RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE, mtdData,
            RPC_TAF_FLASH_MTD_MAX_WRITE_SIZE);
        if (result != LE_OK)
        {
            printf("Can not to write remote MTD partition %s at index %d.", partName, i);
            fclose(fp);
            return;
        }

        if (i * 100 / flashNum != percent)
        {
            percent = i * 100 / flashNum;
            printf("Writing remote MTD partition %s %d%% ...\n", partName, percent);
        }
    }

    result = rpc_taf_flash_MtdClose(partitionRef);
    if (result != LE_OK)
    {
       printf("Can not close remote MTD partition %s.", partName);
    }

    printf("Writing remote MTD partition %s completed.\n", partName);

    fclose(fp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write remote UBI volume.
 */
//--------------------------------------------------------------------------------------------------
void WriteRemoteUbi
(
    const char* partName, ///< [IN] UBI volume name.
    const char* filePath  ///< [IN] UBI image file.
)
{
    uint32_t lebNumber = 0;
    uint32_t freeLebNumber = 0;
    uint32_t volumeSize = 0;
    uint32_t i = 0;
	uint32_t flashPageNum = 0;
	uint32_t paddingPageNum = 0;
    uint32_t percent = 100;
    uint8_t ubiData[RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE];

    FILE *fp = fopen(filePath, "r");
    if (fp == NULL)
    {
        printf("File %s not exists.", filePath);
        return;
    }

    struct stat status;
    if (stat(filePath, &status) != 0)
    {
        printf("Can not get file %s status.", filePath);
        fclose(fp);
        return;
    }

    rpc_taf_flash_VolumeRef_t volumeRef;
    le_result_t result = rpc_taf_flash_UbiOpen(partName, RPC_TAF_FLASH_READ_WRITE, &volumeRef);
    if (result != LE_OK)
    {
        printf("Can not open local UBI volume %s.", partName);
        fclose(fp);
        return;
    }

    result = rpc_taf_flash_UbiInformation(volumeRef, &lebNumber, &freeLebNumber, &volumeSize);
    if (result != LE_OK)
    {
        printf("Can not get remote UBI volume %s information.", partName);
        fclose(fp);
        return;
    }

    if (status.st_size % RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE)
    {
        printf("UBI image %s is not aligned with page size.", filePath);
        flashPageNum = status.st_size / RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE + 1;
    }
    else
    {
        flashPageNum = status.st_size / RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE;
    }

    paddingPageNum = volumeSize / RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE - flashPageNum;

    printf("Remote UBI volume %s has %d pages to be written, %d pages is padding with 0xFF.\n",
        partName, flashPageNum, paddingPageNum);

    result = rpc_taf_flash_UbiInitWrite(volumeRef, volumeSize);
    if (result != LE_OK)
    {
        printf("Can not set write size for remote UBI volume %s.", partName);
        fclose(fp);
        return;
    }

    for (i = 0; i < flashPageNum; i++)
    {
        int ret = fread(ubiData, 1, RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE, fp);
        if (ret < RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE)
        {
            memset(ubiData + ret, 0xFF, RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE - ret);
            printf("Padding 0xFF in remote UBI %s page %d, start at %d.", partName, i, ret);
        }

        result = rpc_taf_flash_UbiWrite(volumeRef, ubiData, RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE);
        if (result != LE_OK)
        {
            printf("Can not to write remote UBI %s at page %d.", partName, i);
            fclose(fp);
            return;
        }

        if (i * 100 / (flashPageNum + paddingPageNum) != percent)
        {
            percent = i * 100 / (flashPageNum + paddingPageNum);
            printf("Writing remote UBI volume %s %d%% ...\n", partName, percent);
        }
    }

    memset(ubiData, 0xFF, RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE);
    for (i = 0; i < paddingPageNum; i++)
    {
        result = rpc_taf_flash_UbiWrite(volumeRef, ubiData, RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE);
        if (result != LE_OK)
        {
            printf("Can not to write remote UBI volume %s at page %d.", partName, i);
            fclose(fp);
            return;
        }

        if ((i + flashPageNum) * 100 / (flashPageNum + paddingPageNum) != percent)
        {
            percent = (i + flashPageNum) * 100 / (flashPageNum + paddingPageNum);
            printf("Padding remote UBI volume %s %d%% ...\n", partName, percent);
        }
    }

    uint32_t leftBytes = volumeSize % RPC_TAF_FLASH_UBI_MAX_WRITE_SIZE;
    if (leftBytes != 0)
    {
        printf("Writting %d bytes to remote UBI volume %s.\n", leftBytes, partName);
        result = rpc_taf_flash_UbiWrite(volumeRef, ubiData, leftBytes);
        if (result != LE_OK)
        {
            printf("Can not to write the rest of remote UBI volume %s.", partName);
            fclose(fp);
            return;
        }
    }

    result = rpc_taf_flash_UbiClose(volumeRef);
    if (result != LE_OK)
    {
        printf("Can not close remote UBI volume %s.", partName);
    }

    printf("Writing remote UBI volume %s completed.\n", partName);

    fclose(fp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints partition information.
 */
//--------------------------------------------------------------------------------------------------
void ctrlFlash_Info
(
    ctrlFlash_Partition_t partType, ///< [IN] Partition type.
    const char* partName            ///< [IN] Partition name.
)
{
    if (partType == CTRLFLASH_MTD)
    {
        RemoteMtdInfo(partName);
    }
    else if (partType == CTRLFLASH_UBI)
    {
        RemoteUbiInfo(partName);
    }
    else
    {
        printf("Invalid partition type.\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase partition.
 */
//--------------------------------------------------------------------------------------------------
void ctrlFlash_Erase
(
    ctrlFlash_Partition_t partType, ///< [IN] Partition type.
    const char* partName            ///< [IN] Partition name.
)
{
    if (partType != CTRLFLASH_MTD)
    {
        printf("Invalid partition type to erase.");
        return;
    }

    EraseRemoteMtd(partName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from partition.
 */
//--------------------------------------------------------------------------------------------------
void ctrlFlash_Read
(
    ctrlFlash_Partition_t partType, ///< [IN] Partition type.
    const char* partName,           ///< [IN] Partition name.
    const char* filePath            ///< [IN] File which stores partition data.
)
{
    if (partType == CTRLFLASH_MTD)
    {
        ReadRemoteMtd(partName, filePath);
    }
    else if (partType == CTRLFLASH_UBI)
    {
        ReadRemoteUbi(partName, filePath);
    }
    else
    {
        printf("Invalid partition type.\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to partition.
 */
//--------------------------------------------------------------------------------------------------
void ctrlFlash_Write
(
    ctrlFlash_Partition_t partType, ///< [IN] Partition type.
    const char* partName,           ///< [IN] Partition name.
    const char* filePath            ///< [IN] File which stores partition data.
)
{
    if (partType == CTRLFLASH_MTD)
    {
        WriteRemoteMtd(partName, filePath);
    }
    else if (partType == CTRLFLASH_UBI)
    {
        WriteRemoteUbi(partName, filePath);
    }
    else
    {
        printf("Invalid partition type.\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint16_t systemId = taf_someipClnt_GetClientId();
    LE_INFO("Current flash system ID is %d.", systemId);

    LE_INFO("Remote flash initialization.");
    rpc_taf_flash_Init();
}
