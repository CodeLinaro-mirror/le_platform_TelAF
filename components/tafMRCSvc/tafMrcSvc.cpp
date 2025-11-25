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

    taf_pa_mrc_Status_t paStatus;
    switch (status)
    {
        case TAF_MRC_SYNC_STATUS_INIT:
            paStatus = TAF_PA_MRC_STATUS_INITIATED;
            break;
        case TAF_MRC_SYNC_STATUS_SUCCESS:
            paStatus = TAF_PA_MRC_STATUS_SUCCEEDED;
            break;
        case TAF_MRC_SYNC_STATUS_FAILURE:
            paStatus = TAF_PA_MRC_STATUS_FAILED;
            break;
        default:
            LE_ERROR("Invalid status %d.", status);
            return LE_FAULT;
    }

    pa_result_t paResult = taf_pa_mrc_SetProcessStatus(TAF_PA_MRC_PROCESS_ABSYNC, paStatus);
    le_result_t result = Utility::Convert::Result(paResult);
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

    taf_pa_mrc_EfsPeStatus_t status;
    pa_result_t paResult = taf_pa_mrc_GetEfsPeStatus(&status);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get EFS PE status.");
        return LE_FAULT;
    }

    if (status.peCountLen == 0 && status.peCountLen > TAF_PA_MRC_EFS_PARTITION_BLOCKS)
    {
        LE_ERROR("Invalid block count %d for EFS.", status.peCountLen);
        return LE_FAULT;
    }

    taf_pa_mrc_EfsBlockStatus_t blockStatus;
    paResult = taf_pa_mrc_GetEfsBlockStatus(&blockStatus);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get EFS block status.");
        return LE_FAULT;
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

    auto& mrc = taf_Mrc::GetInstance();
    taf_MrcEfsMetrics_t* metricsPtr = (taf_MrcEfsMetrics_t*)le_mem_ForceAlloc(mrc.metricsPool);
    metricsPtr->maxCount = max;
    metricsPtr->minCount = min;
    metricsPtr->avgCount = avg;
    metricsPtr->sdValue = sd;
    metricsPtr->badBlockCount = blockStatus.totalBadBlocks;
    *referencePtr = (taf_mrc_MetricsRef_t)le_ref_CreateRef(mrc.metricsRefMap, (void*)metricsPtr);

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

//--------------------------------------------------------------------------------------------------
/**
 * Set the period of EFS backup.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
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