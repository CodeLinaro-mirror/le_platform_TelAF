/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PROP_HMS_HPP
#define TAF_PROP_HMS_HPP

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * FS state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PROP_HMS_FS_STATE_UNKNOWN = 0,
        ///< Unknown.
    TAF_PROP_HMS_FS_STATE_START = 1,
        ///< Started.
    TAF_PROP_HMS_FS_STATE_END = 2
        ///< Ended.
} taf_prop_hms_FsState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Image type
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PROP_HMS_IMAGE_TYPE_UNKNOWN = 0,
        ///< Unknown
    TAF_PROP_HMS_IMAGE_TYPE_FACTORY = 1,
        ///< Factory
    TAF_PROP_HMS_IMAGE_TYPE_RUNTIME = 2,
        ///< Runtime
    TAF_PROP_HMS_IMAGE_TYPE_SOFTWARE_ROLLBACK = 3
        ///< Software rollback
} taf_prop_hms_ImageType_t;

//--------------------------------------------------------------------------------------------------
/**
 * FS status
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PROP_HMS_FS_STATUS_UNKNOWN = 0,
        ///< Unknown.
    TAF_PROP_HMS_FS_STATUS_SUCCESS = 1,
        ///< Success.
    TAF_PROP_HMS_FS_STATUS_FAILED = 2,
        ///< Failed.
    TAF_PROP_HMS_FS_STATUS_ADDR_VALID = 3,
        ///< Address is valid.
    TAF_PROP_HMS_FS_STATUS_ADDR_INVALID = 4
        ///< Address is invalid.
} taf_prop_hms_FsStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'FS Backup'
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_prop_hms_FsBackupHandler* taf_prop_hms_FsBackupHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'FS Restore'
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_prop_hms_FsRestoreHandler* taf_prop_hms_FsRestoreHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference of block PE status.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_prop_hms_BlockPeStatusRef* taf_prop_hms_BlockPeStatusRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for FS backup.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_prop_hms_FsBackupHandlerFunc_t)
(
    const taf_prop_hms_FsState_t state,
        ///< FS backup state.
    const uint32_t imageSize,
        ///< FS backup image size.
    const taf_prop_hms_ImageType_t imageType,
        ///< FS backup image type.
    const taf_prop_hms_FsStatus_t status,
        ///< FS backup status.
    void* contextPtr
        ///< Context
);

//--------------------------------------------------------------------------------------------------
/**
 * Handler for FS restore.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_prop_hms_FsRestoreHandlerFunc_t)
(
    const taf_prop_hms_FsState_t state,
        ///< FS backup state.
    const taf_prop_hms_FsStatus_t status,
        ///< FS backup status.
    void* contextPtr
        ///< Context
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for FS backup.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_prop_hms_FsBackupHandlerRef_t taf_prop_hms_AddFsBackupHandler
(
    taf_prop_hms_FsBackupHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for FS backup.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_prop_hms_RemoveFsBackupHandler
(
    taf_prop_hms_FsBackupHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for FS restore.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_prop_hms_FsRestoreHandlerRef_t taf_prop_hms_AddFsRestoreHandler
(
    taf_prop_hms_FsRestoreHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                  ///< [IN] Handler context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for FS restore.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_prop_hms_RemoveFsRestoreHandler
(
    taf_prop_hms_FsRestoreHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * HMS FS backup indication registration.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_FsBackupIndicationRegistration
(
    uint8_t reg	///< [IN] 0 and 1 represent registration and deregistration.
);

//--------------------------------------------------------------------------------------------------
/**
 * HMS FS backup indication registration.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_FsRestoreIndicationRegistration
(
    uint8_t reg	///< [IN] 0 and 1 represent registration and deregistration.
);

//--------------------------------------------------------------------------------------------------
/**
 * Memory backup is ready.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_BackupMemReady
(
    taf_prop_hms_FsStatus_t status, ///< Address status of the shared memory allocated by HLOS.
    uint32_t imageBufferAddr,     ///< Backup image will be available in this buffer.
    uint32_t imageBufferSize,     ///< Size of the image buffer.
    uint32_t scratchBufferAddr,   ///< Used for image operation like compression and encryption.
    uint32_t scratchBufferSize,   ///< Size of the scratch buffer.
    uint32_t time                 ///< Time in seconds
);

//--------------------------------------------------------------------------------------------------
/**
 * Memory restore is ready.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_RestoreMemReady
(
    taf_prop_hms_FsStatus_t status, ///< Address status of the shared memory allocated by HLOS.
    uint32_t imageBufferAddr,     ///< Backup image will be available in this buffer.
    uint32_t imageBufferSize,     ///< Size of the image buffer.
    uint32_t scratchBufferAddr,   ///< Used for image operation like compression and encryption.
    uint32_t scratchBufferSize    ///< Size of the scratch buffer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Trigger FS backup.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_TriggerFsBackup
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Scrub FS partitions.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_ScrubFsPartitions
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get block erase status
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_GetBlockEraseStatus
(
    uint32_t* maxEraseCountPtr,    ///< [OUT] Maximum erasable count.
    uint32_t* fsTotalBadBlocksPtr, ///< [OUT] FS total bad blocks.
    uint32_t* nadTotalBadBlocksPtr ///< [OUT] NAD total bad blocks.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the time for ECall deregistration.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_SetEcallDeregistrationTime
(
    uint32_t time ///< [IN] Time for ECall deregistration.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the time for ECall deregistration.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_GetEcallDeregistrationTime
(
    uint32_t* timePtr ///< [OUT] Time for ECall deregistration.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the status reference of block PE count .
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_GetBlockPeStatus
(
    taf_prop_hms_BlockPeStatusRef_t* statusRefPtr ///< [OUT] Status reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete the status reference of block PE count .
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_DeleteBlockPeStatus
(
    taf_prop_hms_BlockPeStatusRef_t statusRef ///< [IN] Status reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of block PE count elements.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_GetBlockPeCountElements
(
    taf_prop_hms_BlockPeStatusRef_t statusRef, ///< [IN] Status reference.
    uint32_t* numPtr                         ///< [OUT] Number of block PE count elements.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the block PE count.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_GetBlockPeCount
(
    taf_prop_hms_BlockPeStatusRef_t statusRef, ///< [[IN] Status reference.
    uint32_t index,                          ///< [IN] Index of block PE count elements.
    uint32_t* countPtr                       ///< [OUT] Block PE count.
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize HMS QMI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_hms_Initialize
(
    uint32_t sysTime, ///< [IN] Timeout for readiness of service.
    uint32_t respTime ///< [IN] Timeout for QMI response.
);

#endif /* TAF_PROP_HMS_HPP */