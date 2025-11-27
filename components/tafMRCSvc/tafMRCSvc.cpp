/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <math.h>

#include "tafMRC.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the MRC Daemon to indicate that OTA has been started.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendOtaStartMsg
(
    void
)
{
    pa_result_t paResult = taf_pa_mrc_SetProcessStatus(TAF_PA_MRC_PROCESS_OTA,
        TAF_PA_MRC_STATUS_INITIATED);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
        LE_ERROR("Failed to set OTA initiated status.");

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the MRC Daemon to indicate that OTA has been resumed.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendOtaResumeMsg
(
    void
)
{
    pa_result_t paResult = taf_pa_mrc_SetProcessStatus(TAF_PA_MRC_PROCESS_OTA,
        TAF_PA_MRC_STATUS_RESUMED);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
        LE_ERROR("Failed to set OTA resumed status.");

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the MRC Daemon to indicate that OTA has been ended.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendOtaEndMsg
(
    taf_mrc_OtaOperationStatus_t status ///< [IN] The status of OTA.
)
{
    taf_pa_mrc_Status_t paStatus = Utility::Convert::Status(status);
    pa_result_t paResult = taf_pa_mrc_SetProcessStatus(TAF_PA_MRC_PROCESS_OTA, paStatus);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
        LE_ERROR("Failed to set OTA ended status.");

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the MRC Daemon to perform AB sync.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendOtaAbsyncMsg
(
    void
)
{
    pa_result_t paResult = taf_pa_mrc_PerformABSync();
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
        LE_ERROR("Failed to perform AB sync.");

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the MRC Daemon to indicate AB sync status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SendSyncStatusMsg
(
    taf_mrc_SyncStatus_t status ///< [IN] The status of AB sync.
)
{
    taf_pa_mrc_Status_t paStatus = Utility::Convert::Status(status);
    pa_result_t paResult = taf_pa_mrc_SetProcessStatus(TAF_PA_MRC_PROCESS_ABSYNC, paStatus);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set AB sync status.");
        return result;
    }

    le_clk_Time_t time = { .sec = RESP_TIMEOUT };
    auto& mrcFactory = MRCFactory::GetInstance();
    result = le_sem_WaitWithTimeOut(mrcFactory.semaphores.abSync, time);
    if (result != LE_OK)
        LE_ERROR("Timeout for MRC to handle AB sync status.");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Measures the EFS metrics.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
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

    taf_pa_mrc_EfsPeStatus_t status;
    pa_result_t paResult = taf_pa_mrc_GetEfsPeStatus(&status);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get EFS PE status.");
        return result;
    }

    if (status.peCountLen == 0 || status.peCountLen > TAF_PA_MRC_EFS_PARTITION_BLOCKS)
    {
        LE_ERROR("Invalid block count %d for EFS.", status.peCountLen);
        return LE_FAULT;
    }

    taf_pa_mrc_EfsBlockStatus_t blockStatus;
    paResult = taf_pa_mrc_GetEfsBlockStatus(&blockStatus);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get EFS block status.");
        return result;
    }

    uint32_t sum = 0;
    uint32_t avg = 0;
    uint32_t sd = 0;
    uint32_t max = 0;
    uint32_t min = 0xFFFFFFFF;
    for (uint32_t i = 0; i < status.peCountLen; i++)
    {
        sum += status.peCount[i];

        if (status.peCount[i] > max)
            max = status.peCount[i];

        if (status.peCount[i] < min)
            min = status.peCount[i];
    }

    avg = sum / status.peCountLen;

    uint32_t ssd = 0;
    for (uint32_t i = 0; i < status.peCountLen; i++)
    {
        if (status.peCount[i] >= avg)
            ssd += pow(status.peCount[i] - avg, 2);
        else
            ssd += pow(avg - status.peCount[i], 2);
    }

    sd = (uint32_t)ceil(sqrt(ssd / status.peCountLen));

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_mem_ForceAlloc(mrcFactory.pools.metrics);
    metricsPtr->maxCount = max;
    metricsPtr->minCount = min;
    metricsPtr->avgCount = avg;
    metricsPtr->sdValue = sd;
    metricsPtr->badBlockCount = blockStatus.totalBadBlocks;
    *referencePtr = (taf_mrc_MetricsRef_t)le_ref_CreateRef(mrcFactory.maps.metrics,
        (void*)metricsPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the EFS metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(mrcFactory.maps.metrics, reference);
    le_mem_Release(metricsPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the maximum program and erase count in EFS.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
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
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
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
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
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
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
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
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
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

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    *countPtr = metricsPtr->badBlockCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the period of EFS backup.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SetEfsBackupPeriod
(
    uint32_t period ///< Period in second.
)
{
    pa_result_t result = taf_pa_mrc_SetTimerPeriod(TAF_PA_MRC_TIMER_EFS_BACKUP, period);
    return Utility::Convert::Result(result);
}