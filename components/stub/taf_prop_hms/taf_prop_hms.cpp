/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"

#include "taf_prop_hms.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for FS backup.
 */
//--------------------------------------------------------------------------------------------------
taf_prop_hms_FsBackupHandlerRef_t taf_prop_hms_AddFsBackupHandler
(
    taf_prop_hms_FsBackupHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
)
{
    LE_WARN("Unsupported");
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for FS backup.
 */
//--------------------------------------------------------------------------------------------------
void taf_prop_hms_RemoveFsBackupHandler
(
    taf_prop_hms_FsBackupHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    LE_WARN("Unsupported");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for FS restore.
 */
//--------------------------------------------------------------------------------------------------
taf_prop_hms_FsRestoreHandlerRef_t taf_prop_hms_AddFsRestoreHandler
(
    taf_prop_hms_FsRestoreHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                  ///< [IN] Handler context.
)
{
    LE_WARN("Unsupported");
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for FS restore.
 */
//--------------------------------------------------------------------------------------------------
void taf_prop_hms_RemoveFsRestoreHandler
(
    taf_prop_hms_FsRestoreHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    LE_WARN("Unsupported");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * HMS FS backup indication registration.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_prop_hms_FsBackupIndicationRegistration
(
    uint8_t reg	///< [IN] 0 and 1 represent registration and deregistration.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * HMS FS backup indication registration.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_prop_hms_FsRestoreIndicationRegistration
(
    uint8_t reg	///< [IN] 0 and 1 represent registration and deregistration.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_BackupMemReady
(
    taf_prop_hms_FsStatus_t status, ///< Address status of the shared memory allocated by HLOS.
    uint32_t imageBufferAddr,     ///< Backup image will be available in this buffer.
    uint32_t imageBufferSize,     ///< Size of the image buffer.
    uint32_t scratchBufferAddr,   ///< Used for image operation like compression and encryption.
    uint32_t scratchBufferSize,   ///< Size of the scratch buffer.
    uint32_t time                 ///< Time in seconds
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_RestoreMemReady
(
    taf_prop_hms_FsStatus_t status, ///< Address status of the shared memory allocated by HLOS.
    uint32_t imageBufferAddr,     ///< Backup image will be available in this buffer.
    uint32_t imageBufferSize,     ///< Size of the image buffer.
    uint32_t scratchBufferAddr,   ///< Used for image operation like compression and encryption.
    uint32_t scratchBufferSize    ///< Size of the scratch buffer.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_TriggerFsBackup
(
    void
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_ScrubFsPartitions
(
    void
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_GetBlockEraseStatus
(
    uint32_t* maxEraseCountPtr,    ///< [OUT] Maximum erasable count.
    uint32_t* fsTotalBadBlocksPtr, ///< [OUT] FS total bad blocks.
    uint32_t* nadTotalBadBlocksPtr ///< [OUT] NAD total bad blocks.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_SetEcallDeregistrationTime
(
    uint32_t time ///< [IN] Time for ECall deregistration.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_GetEcallDeregistrationTime
(
    uint32_t* timePtr ///< [OUT] Time for ECall deregistration.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the block PE status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_prop_hms_GetBlockPeStatus
(
    taf_prop_hms_BlockPeStatusRef_t* statusRefPtr ///< [OUT] Status reference.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_DeleteBlockPeStatus
(
    taf_prop_hms_BlockPeStatusRef_t statusRef ///< [IN] PE count status reference.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_GetBlockPeCountElements
(
    taf_prop_hms_BlockPeStatusRef_t statusRef, ///< [IN] Status reference.
    uint32_t* numPtr                         ///< [OUT] Number of block PE count elements.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

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
le_result_t taf_prop_hms_GetBlockPeCount
(
    taf_prop_hms_BlockPeStatusRef_t statusRef, ///< [IN] Status reference.
    uint32_t index,                          ///< [IN] Index of block PE count elements.
    uint32_t* countPtr                       ///< [OUT] Block PE count.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize HMS QMI.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_prop_hms_Initialize
(
    uint32_t sysTime, ///< [IN] Timeout for readiness of service.
    uint32_t respTime ///< [IN] Timeout for QMI response.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_prop_hms stub.");
}