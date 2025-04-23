/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PA_MRC_HPP
#define TAF_PA_MRC_HPP

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * MRC indication bitmask.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_PA_MRC_IND_BIT_MASK_OTA_ABSYNC_STATUS 0x1 ///< OTA and ABSYNC status indication bitmask.
#define TAF_PA_MRC_IND_BIT_MASK_IMMINENT 0x2          ///< Imminent indication bitmask.
typedef uint32_t taf_pa_mrc_IndBitMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ecall status
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_ECALL_ONGOING = 0,
        ///< Ecall is ongoing.
    TAF_PA_MRC_ECALL_ENDED = 1
        ///< Ecall is ended.
}
taf_pa_mrc_EcallStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Flash type
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_FLASH_TYPE_MTD = 0,
        ///< NAND
    TAF_PA_MRC_FLASH_TYPE_MMC = 1
        ///< eMMC
}
taf_pa_mrc_FlashType_t;

//--------------------------------------------------------------------------------------------------
/**
 * OTA status
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_OTA_STARTED = 0,
        ///< OTA is started.
    TAF_PA_MRC_OTA_RESUMED = 1,
        ///< OTA is resumed.
    TAF_PA_MRC_OTA_ENDED_WITH_SUCCESS = 2,
        ///< OTA is ended successfully.
    TAF_PA_MRC_OTA_ENDED_WITH_FAILURE = 3
        ///< OTA is ended with failure.
}
taf_pa_mrc_OtaStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Status of mirroring the AB slots.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_ABSYNC_STARTED = 0,
        ///< Mirror slots is started.
    TAF_PA_MRC_ABSYNC_WITH_SUCCESS = 1,
        ///< Mirror slots is ended successfully.
    TAF_PA_MRC_ABSYNC_WITH_FAILURE = 2
        ///< Mirror slots is ended with failure.
}
taf_pa_mrc_ABSyncStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * MRCD state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_STATE_DISABLED = 0,
        ///< MRC is disabled.
    TAF_PA_MRC_STATE_ENABLED = 1
        ///< MRC is enabled.
}
taf_pa_mrc_MrcState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Operation from external.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_OP_UNKNOWN = 0,
        ///< Unkown operation.
    TAF_PA_MRC_OP_OTA_START = 1,
        ///< Operation for OTA start.
    TAF_PA_MRC_OP_OTA_END = 2,
        ///< Operation for OTA start.
    TAF_PA_MRC_OP_OTA_RESUME = 3,
        ///< Operation for OTA resume.
    TAF_PA_MRC_OP_ABSYNC = 4
        ///< Operation for AB sync.
}
taf_pa_mrc_Operation_t;

//--------------------------------------------------------------------------------------------------
/**
 * Operation status.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_MRC_OP_STATUS_UNKNOWN = 0,
        ///< Unkown operation status.
    TAF_PA_MRC_OP_STATUS_SUCCESS = 1,
        ///< Operation is performed with success.
    TAF_PA_MRC_OP_STATUS_FAILURE = 2
        ///< Operation is performed with failure.
}
taf_pa_mrc_OperationStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Operation indication structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_mrc_Operation_t operation;
        ///< Operation from external.
    taf_pa_mrc_OperationStatus_t status;
        ///< Operation status.
}
taf_pa_mrc_OperationIndication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'Operation Status'
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pa_mrc_OpStatusHandler* taf_pa_mrc_OpStatusHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_pa_mrc_OpStatusHandlerFunc_t)
(
        taf_pa_mrc_OperationIndication_t* indication,
        ///< Indication for operation status.
        void* contextPtr
        ///< Context
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_mrc_OpStatusHandlerRef_t taf_pa_mrc_AddOpStatusHandler
(
    taf_pa_mrc_OpStatusHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_mrc_RemoveOpStatusHandler
(
    taf_pa_mrc_OpStatusHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'Time to Expiry'
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pa_mrc_TimeExpiryHandler* taf_pa_mrc_TimeExpiryHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_pa_mrc_TimeExpiryHandlerFunc_t)
(
    const uint32_t* LE_NONNULL time,
        ///< Time to expiry.
    void* contextPtr
        ///< Context
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_mrc_TimeExpiryHandlerRef_t taf_pa_mrc_AddTimeExpiryHandler
(
    taf_pa_mrc_TimeExpiryHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                   ///< [IN] Handler context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_mrc_RemoveTimeExpiryHandler
(
    taf_pa_mrc_TimeExpiryHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * MRC indication registration.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_IndicationRegistration
(
    taf_pa_mrc_IndBitMask_t bitmask, ///< [IN] MRC indication bitmask.
    uint8_t reg	                     ///< [IN] 0 and 1 represent registration and deregistration.
);

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with ecall status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_NotifyEcallStatus
(
    taf_pa_mrc_EcallStatus_t status ///< [IN] Ecall status.
);

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with OTA status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_NotifyOtaStatus
(
    taf_pa_mrc_OtaStatus_t status ///< [IN] OTA status.
);

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with AB sync status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_NotifyABSyncStatus
(
    taf_pa_mrc_ABSyncStatus_t status ///< [IN] AB sync status.
);

//--------------------------------------------------------------------------------------------------
/**
 * Perform AB sync.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_PerformABSync
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Perform EFS backup.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_PerformEFSBackup
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get MRC state.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_GetMrcState
(
    taf_pa_mrc_MrcState_t* state ///< [OUT] MRC state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get flash type.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_GetFlashType
(
    taf_pa_mrc_FlashType_t* type ///< [OUT] Flash type.
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize MRC QMI.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_mrc_Initialize
(
    uint32_t sysTime, ///< [IN] Timeout for readiness of service.
    uint32_t respTime ///< [IN] Timeout for QMI response.
);

#endif /* TAF_PA_MRC_HPP */