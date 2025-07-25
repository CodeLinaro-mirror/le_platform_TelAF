/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PA_FLASH_H
#define TAF_PA_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "legato.h"
#include <mtd/mtd-user.h>

//--------------------------------------------------------------------------------------------------
/**
 * MTD reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pa_flash_MtdRef* taf_pa_flash_MtdRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * UBI reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pa_flash_UbiRef* taf_pa_flash_UbiRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for flash access.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_flash_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open MTD.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_OpenMtd
(
    const char* namePtr,             ///< [IN] Partition name.
    taf_pa_flash_MtdRef_t* mtdRefPtr ///< [OUT] MTD reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close MTD.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_flash_CloseMtd
(
    taf_pa_flash_MtdRef_t mtdRef     ///< [IN] MTD reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get MTD information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_GetMtdInfo
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    mtd_info_t* infoPtr              ///< [OUT] MTD information.
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if a block is good.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_IsGoodBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a block as bad.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_MarkBadBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Erase a block.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_EraseBlock
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    size_t blockNum                  ///< [IN] The block number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read MTD page.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_ReadMtdPage
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    unsigned char* buffer,           ///< [OUT] Buffer for reading bytes.
    unsigned int pageNum,            ///< [IN] The page number.
    size_t len                       ///< [IN] Length of bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write MTD page.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_WriteMtdPage
(
    taf_pa_flash_MtdRef_t mtdRef,    ///< [IN] MTD reference.
    const unsigned char* buffer,     ///< [IN] Buffer for writting bytes.
    unsigned int pageNum,            ///< [IN] The page number.
    size_t len                       ///< [IN] Length of bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Copy MTD.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_CopyMtd
(
    const char* srcNamePtr,          ///< [IN] Source partition name.
    const char* dstNamePtr,          ///< [IN] Destinate partition name.
    size_t imageSize                 ///< [IN] Image size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_OpenUbi
(
    const char* namePtr,             ///< [IN] Partition name.
    bool isReadOnly,                 ///< [IN] True if it is Read-Only.
    taf_pa_flash_UbiRef_t* ubiRefPtr ///< [OUT] UBI reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_CloseUbi
(
    taf_pa_flash_UbiRef_t ubiRef     ///< [IN] UBI reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set UBI volume upgrade size.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_SetUbiVolUpSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    long long size                   ///< [IN] Upgrade size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_ReadUbi
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint8_t* buffer,                 ///< [OUT] Buffer for reading bytes.
    unsigned int offset,             ///< [IN] Offset.
    size_t len                       ///< [IN] Length of bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_WriteUbi
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    const uint8_t* buffer,           ///< [OUT] Buffer for writting bytes.
    size_t len                       ///< [IN] Length of bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Erase UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_EraseUbi
(
    const char* namePtr              ///< [IN] Partition name.
);

//--------------------------------------------------------------------------------------------------
/**
 * Copy UBI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_CopyUbi
(
    const char* srcNamePtr,          ///< [IN] Source partition name.
    const char* dstNamePtr,          ///< [IN] Destinate partition name.
    unsigned char* buffer,           ///< [IN] Buffer for each copy loop.
    size_t bufferSize,               ///< [IN] Buffer size.
    size_t imageSize                 ///< [IN] Image size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume size.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_GetUbiVolSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* sizePtr                ///< [OUT] UBI volume size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume LEB size.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_GetUbiVolLebSize
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* sizePtr                ///< [OUT] UBI volume LEB size.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume reserved LEB number.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_GetUbiVolReservedLebNum
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* lebNumPtr              ///< [OUT] UBI device available LEB number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI device available LEB number.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_pa_flash_GetUbiDevAvailableLebNum
(
    taf_pa_flash_UbiRef_t ubiRef,    ///< [IN] UBI reference.
    uint32_t* lebNumPtr              ///< [OUT] UBI device available LEB number.
);

#ifdef __cplusplus
}
#endif

#endif /* TAF_PA_FLASH_H */