/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "taf_pa_mrc.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * MRC indication registration.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_IndicationRegistration
(
    taf_pa_mrc_IndBitMask_t bitmask, ///< [IN] MRC indication bitmask.
    uint8_t reg                      ///< [IN] 0 and 1 represent registration and deregistration.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with ecall status.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_NotifyEcallStatus
(
    taf_pa_mrc_EcallStatus_t status ///< [IN] Ecall status.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with OTA status.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_NotifyOtaStatus
(
    taf_pa_mrc_OtaStatus_t status ///< [IN] OTA status.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify MRC with AB sync status.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_NotifyABSyncStatus
(
    taf_pa_mrc_ABSyncStatus_t status ///< [IN] AB sync status.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform AB sync.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_PerformABSync
(
    void
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform EFS backup.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_PerformEFSBackup
(
    void
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MRC state.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_GetMrcState
(
    taf_pa_mrc_MrcState_t* state ///< [OUT] MRC state.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get flash type.
 *
 * @return
 *  - LE_UNSUPPORTED -- Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_GetFlashType
(
    taf_pa_mrc_FlashType_t* type ///< [OUT] Flash type.
)
{
    LE_WARN("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_mrc_LayerOpStatusHandler
(
    void* reportPtr,       ///< [IN] Report.
    void* layerHandlerFunc ///< [IN] Handler function.
)
{
    LE_WARN("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_mrc_OpStatusHandlerRef_t taf_pa_mrc_AddOpStatusHandler
(
    taf_pa_mrc_OpStatusHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
)
{
    LE_WARN("Unsupported");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_mrc_RemoveOpStatusHandler
(
    taf_pa_mrc_OpStatusHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    LE_WARN("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_mrc_LayerTimeExpiryHandler
(
    void* reportPtr,       ///< [IN] Report.
    void* layerHandlerFunc ///< [IN] Handler function.
)
{
    LE_WARN("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_mrc_TimeExpiryHandlerRef_t taf_pa_mrc_AddTimeExpiryHandler
(
    taf_pa_mrc_TimeExpiryHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                   ///< [IN] Handler context.
)
{
    LE_WARN("Unsupported");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for time to expiry.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_mrc_RemoveTimeExpiryHandler
(
    taf_pa_mrc_TimeExpiryHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    LE_WARN("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize MRC QMI.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_mrc_Initialize
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
    LE_INFO("taf_pa_mrc stub.");
}