/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"

#include "taf_pa_flash.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for flash access.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_flash_Init
(
    void
)
{
    LE_INFO("taf_pa_flash_Init stub");
}

//--------------------------------------------------------------------------------------------------
/**
 * Open MTD.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_OpenMtd
(
    const char* namePtr,             ///< [IN] Partition name.
    taf_pa_flash_MtdRef_t* mtdRefPtr ///< [OUT] MTD reference.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close MTD.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_flash_CloseMtd
(
    taf_pa_flash_MtdRef_t mtdRef     ///< [IN] MTD reference.
)
{
    LE_WARN("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD information.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_GetMtdInfo
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    mtd_info_t* infoPtr              ///< [OUT] MTD information.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a block is good.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_IsGoodBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark a block as bad.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_MarkBadBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase a block.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_EraseBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD page.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_ReadMtdPage
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    unsigned char*  buffer,          ///< [OUT] Buffer for reading bytes.
    unsigned int pageNum,            ///< [IN] The page number.
    size_t len                       ///< [IN] Length of bytes.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD page.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_WriteMtdPage
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    const unsigned char* buffer,     ///< [IN] Buffer for writting bytes.
    unsigned int pageNum,            ///< [IN] The page number.
    size_t len                       ///< [IN] Length of bytes.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy MTD.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_CopyMtd
(
    const char* srcnamePtr,          ///< [IN] Source partition name.
    const char* dstnamePtr,          ///< [IN] Destinate partition name.
    size_t imageSize                 ///< [IN] Image size.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_OpenUbi
(
    const char* namePtr,             ///< [IN] Partition name.
    bool isReadOnly,                 ///< [IN] True if it is Read-Only.
    taf_pa_flash_UbiRef_t* ubiRefPtr ///< [OUT] UBI reference.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_CloseUbi
(
    taf_pa_flash_UbiRef_t ubiRef     ///< [IN] UBI reference.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set UBI volume upgrade size.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_SetUbiVolUpSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    long long size                   ///< [IN] Upgrade size.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_ReadUbi
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint8_t* buffer,                 ///< [OUT] Buffer for reading bytes.
    unsigned int offset,             ///< [IN] Offset.
    size_t len                       ///< [IN] Length of bytes.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_WriteUbi
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    const uint8_t* buffer,           ///< [OUT] Buffer for writting bytes.
    size_t len                       ///< [IN] Length of bytes.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_CopyUbi
(
    const char* srcNamePtr,          ///< [IN] Source partition name.
    const char* dstNamePtr,          ///< [IN] Destinate partition name.
    unsigned char* buffer,           ///< [IN] Buffer for each copy loop.
    size_t bufferSize,               ///< [IN] Buffer size.
    size_t imageSize                 ///< [IN] Image size.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase UBI.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_EraseUbi
(
    const char* namePtr              ///< [IN] Partition name.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume size.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_GetUbiVolSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* sizePtr                ///< [OUT] UBI volume size.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume LEB size.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_GetUbiVolLebSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* sizePtr                ///< [OUT] UBI volume LEB size.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume reserved LEB number.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_GetUbiVolReservedLebNum
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* lebNumPtr              ///< [OUT] UBI device available LEB number.
)
{
    LE_WARN("Unsupported");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI device available LEB number.
 */
//--------------------------------------------------------------------------------------------------
int taf_pa_flash_GetUbiDevAvailableLebNum
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* lebNumPtr              ///< [OUT] UBI device available LEB number.
)
{
    LE_WARN("Unsupported");
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_pa_flash stub.");
}