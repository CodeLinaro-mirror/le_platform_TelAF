/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <math.h>

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
    if (referencePtr == nullptr)
    {
        LE_ERROR("referencePtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    uint32_t eraseCount = 0;
    uint32_t efsBadBlocks = 0;
    uint32_t nadBadBlocks = 0;
    le_result_t result = taf_prop_hms_GetBlockEraseStatus(&eraseCount, &efsBadBlocks, &nadBadBlocks);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get block erase status.");
        return result;
    }

    taf_prop_hms_BlockPeStatusRef_t statusRef = nullptr;
    result = taf_prop_hms_GetBlockPeStatus(&statusRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get blcok PE count metrics.");
        return result;
    }

    uint32_t elements = 0;
    result = taf_prop_hms_GetBlockPeCountElements(statusRef, &elements);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get blcok PE count elements.");
        return result;
    }

    if (elements == 0)
    {
        LE_ERROR("No PE count elements.");
        return LE_FAULT;
    }

    uint32_t* array = (uint32_t*)malloc(elements * sizeof(uint32_t));
    uint32_t sum = 0;
    uint32_t avg = 0;
    uint32_t sd = 0;
    uint32_t max = 0;
    uint32_t min = 0xFFFFFFFF;
    for (uint32_t i = 0; i < elements; i++)
    {
        uint32_t count = 0;
        result = taf_prop_hms_GetBlockPeCount(statusRef, i, &count);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to get blcok PE count at %d.", i);
            free(array);
            result = taf_prop_hms_DeleteBlockPeStatus(statusRef);
            if (result != LE_OK)
                LE_ERROR("Fail to delete blcok PE count metrics.");

            return result;
        }

        array[i] = count;
        sum += count;

        if (count > max)
            max = count;

        if (count < min)
            min = count;
    }

    avg = sum / elements;

    result = taf_prop_hms_DeleteBlockPeStatus(statusRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to delete blcok PE count metrics.");
        free(array);
        return result;
    }

    uint32_t ssd = 0;
    for (uint32_t i = 0; i < elements; i++)
    {
        if (array[i] >= avg)
            ssd += pow(array[i] - avg, 2);
        else
            ssd += pow(avg - array[i], 2);
    }

    free(array);

    sd = (uint32_t)ceil(sqrt(ssd / elements));

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr = (taf_MrcEfsMetrics_t*)le_mem_ForceAlloc(tafMrc.metricsPool);
    metricsPtr->maxCount = max;
    metricsPtr->minCount = min;
    metricsPtr->avgCount = avg;
    metricsPtr->sdValue = sd;
    metricsPtr->badBlockCount = efsBadBlocks;
    *referencePtr = (taf_mrc_MetricsRef_t)le_ref_CreateRef(tafMrc.metricsRefMap, (void*)metricsPtr);

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(tafMrc.metricsRefMap, reference);
    le_mem_Release(metricsPtr);

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    if (countPtr == nullptr)
    {
        LE_ERROR("countPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *countPtr = metricsPtr->maxCount;

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    if (countPtr == nullptr)
    {
        LE_ERROR("countPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *countPtr = metricsPtr->minCount;

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    if (countPtr == nullptr)
    {
        LE_ERROR("countPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *countPtr = metricsPtr->avgCount;

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    if (sdPtr == nullptr)
    {
        LE_ERROR("sdPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *sdPtr = metricsPtr->sdValue;

    return LE_OK;
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
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr");
        return LE_BAD_PARAMETER;
    }

    if (countPtr == nullptr)
    {
        LE_ERROR("countPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    auto &tafMrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr =
        (taf_MrcEfsMetrics_t*)le_ref_Lookup(tafMrc.metricsRefMap, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *countPtr = metricsPtr->badBlockCount;

    return LE_OK;
}