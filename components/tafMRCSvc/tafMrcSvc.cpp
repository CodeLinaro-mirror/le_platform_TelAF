/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMrc.hpp"

using namespace tafsvc;

COMPONENT_INIT
{
    LE_INFO("tafMrc Service Init...\n");
    auto &tafMrc = taf_Mrc::GetInstance();
    tafMrc.Init();
    LE_INFO("tafMrc Service Ready...\n");
}

le_result_t taf_mrc_SendOtaStartMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_START);
}

le_result_t taf_mrc_SendOtaResumeMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_RESUME);
}

le_result_t taf_mrc_SendOtaEndMsg(taf_mrc_OtaOperationStatus_t otaStatus)
{
    auto &tafMrc = taf_Mrc::GetInstance();
    if (otaStatus == TAF_MRC_OTA_OP_STATUS_SUCCESS) {
        return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_END_SUCCESS);
    } else if (otaStatus == TAF_MRC_OTA_OP_STATUS_FAILURE) {
        return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_END_FAILURE);
    }

    return LE_BAD_PARAMETER;
}

le_result_t taf_mrc_SendOtaAbsyncMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_ABSYNC);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends sync staus to MRCD.
 *
 * @return
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendSyncStatusMsg
(
    taf_mrc_SyncStatus_t status ///< Sync status.
)
{
    auto &tafMrc = taf_Mrc::GetInstance();
    if (!tafMrc.paReady)
    {
        LE_ERROR("MRC platform adaptor is not available.");
        return LE_FAULT;
    }

    taf_pa_mrc_ABSyncStatus_t paStatus;
    switch (status)
    {
        case TAF_MRC_SYNC_STATUS_INIT:
            paStatus = TAF_PA_MRC_ABSYNC_STARTED;
            break;
        case TAF_MRC_SYNC_STATUS_SUCCESS:
            paStatus = TAF_PA_MRC_ABSYNC_WITH_SUCCESS;
            break;
        case TAF_MRC_SYNC_STATUS_FAILURE:
            paStatus = TAF_PA_MRC_ABSYNC_WITH_FAILURE;
            break;
        default:
            LE_ERROR("Invalid status %d.", status);
            return LE_FAULT;
    }

    le_result_t result = taf_pa_mrc_NotifyABSyncStatus(paStatus);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to notify AB sycn status.");
        return LE_FAULT;
    }

    le_clk_Time_t time = { .sec = TAF_MRC_MSG_RESP_TIMEOUT };
    result = le_sem_WaitWithTimeOut(tafMrc.syncSem, time);
    if (result != LE_OK)
    {
        LE_ERROR("Timeout for MRC to handle AB sync status.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Measures the EFS metrics.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_MeasureEfsMetrics
(
    taf_mrc_MetricsRef_t* referencePtr ///< [OUT] The EFS metrics reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the EFS metrics.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_DeleteEfsMetrics
(
    taf_mrc_MetricsRef_t reference ///< [IN] The EFS metrics reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the maximum program and erase count in EFS.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsMaxPECount
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t* countPtr              ///< [OUT] The maximum program and erase count in EFS.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the minimum program and erase count in EFS.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsMinPECount
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t* countPtr              ///< [OUT] The minimum program and erase count in EFS.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the average program and erase count in EFS.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsAvgPECount
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t* countPtr              ///< [OUT] The average program and erase count in EFS.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the standard deviation of program and erase count in EFS.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsPEStandardDeviation
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t* sdPtr                 ///< [OUT] The standard deviation
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the bad block count in EFS.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsBadBlocks
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t* countPtr              ///< [OUT] The bad block count in EFS.
)
{
    return LE_NOT_IMPLEMENTED;
}