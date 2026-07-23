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
 *  - LE_OK -- The OTA-start status was accepted by the service layer and passed to MRCD.
 *  - LE_FAULT -- The OTA-start request failed in the underlying process-status update path, or an
 *                unmapped lower-layer error is converted to LE_FAULT.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_SetProcessStatus().
 *
 *  This function returns Utility::Convert::Result(taf_pa_mrc_SetProcessStatus(...)). The exact
 *  values depend on the lower-layer implementation; from the visible code paths, the propagated
 *  results are 0, -EFAULT, or -ENOSYS.
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
 *  - LE_OK -- The OTA-resume status was accepted by the service layer and passed to MRCD.
 *  - LE_FAULT -- The OTA-resume request failed in the underlying process-status update path, or an
 *                unmapped lower-layer error is converted to LE_FAULT.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_SetProcessStatus().
 *
 *  This function returns Utility::Convert::Result(taf_pa_mrc_SetProcessStatus(...)). The exact
 *  values depend on the lower-layer implementation; from the visible code paths, the propagated
 *  results are 0, -EFAULT, or -ENOSYS.
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
 *  - LE_OK -- The OTA-end status was converted and accepted by the service layer.
 *  - LE_FAULT -- The OTA-end request failed in the underlying process-status update path, or an
 *                unmapped lower-layer error is converted to LE_FAULT.
 *  - LE_BAD_PARAMETER -- status is converted to an unsupported lower-layer value, or the
 *                        underlying process-status update path returns -EINVAL.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_SetProcessStatus().
 *
 *  This function returns Utility::Convert::Result(taf_pa_mrc_SetProcessStatus(...)). The exact
 *  values depend on the lower-layer implementation; from the visible code paths, the propagated
 *  results are 0, -EINVAL, -EFAULT, or -ENOSYS.
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
 *  - LE_OK -- AB synchronization was accepted by the service layer.
 *  - LE_FAULT -- The A/B synchronization request failed in the underlying path, or an unmapped
 *                lower-layer error is converted to LE_FAULT.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_PerformABSync().
 *
 *  This function returns Utility::Convert::Result(taf_pa_mrc_PerformABSync()). The exact values
 *  depend on the lower-layer implementation; from the visible code paths, the propagated results
 *  are 0, -EFAULT, or -ENOSYS.
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
 *  - LE_OK -- The AB-sync status update was accepted by MRCD. The function also returns LE_OK when
 *             the follow-up semaphore wait times out, because that timeout is only logged and is
 *             not propagated to the caller.
 *  - LE_FAULT -- The underlying process-status update path returned -EFAULT, or another unmapped
 *                lower-layer error is converted to LE_FAULT.
 *  - LE_BAD_PARAMETER -- status is converted to an unsupported lower-layer value, or the
 *                        underlying process-status update path returns -EINVAL.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_SetProcessStatus().
 *
 *  This function propagates Utility::Convert::Result(taf_pa_mrc_SetProcessStatus(...)) for the
 *  status update only. The exact values depend on the lower-layer implementation; from the visible
 *  code paths, the propagated results are 0, -EINVAL, -EFAULT, or -ENOSYS. The later semaphore
 *  wait only logs timeout and does not add a caller-visible return code.
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

    // Wait until the daemon reports that it has processed the ABSYNC status update.
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
 *  - LE_OK -- The metrics snapshot was collected and a reference was created successfully.
 *  - LE_FAULT -- referencePtr passed the null check but the measured block count is invalid, the
 *                underlying EFS query path failed with -EFAULT, or another unmapped lower-layer
 *                error is converted to LE_FAULT.
 *  - LE_BAD_PARAMETER -- referencePtr is null, or the underlying EFS-status query path returns
 *                        -EINVAL.
 *  - LE_TIMEOUT -- The underlying taf_pa_mrc_GetEfsPeStatus() or
 *                  taf_pa_mrc_GetEfsBlockStatus() path returns -ETIMEDOUT.
 *  - LE_UNSUPPORTED -- The underlying taf_pa_mrc_GetEfsPeStatus() or
 *                      taf_pa_mrc_GetEfsBlockStatus() path returns -ENOTSUP.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_GetEfsPeStatus() or
 *                          taf_pa_mrc_GetEfsBlockStatus().
 *
 *  This function can return local validation errors and propagated results from both
 *  taf_pa_mrc_GetEfsPeStatus() and taf_pa_mrc_GetEfsBlockStatus(). The exact values depend on the
 *  lower-layer implementation; from the visible code paths, the propagated results are 0, -EINVAL,
 *  -EFAULT, -ETIMEDOUT, -ENOTSUP, or -ENOSYS.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_MeasureEfsMetrics
(
    taf_mrc_MetricsRef_t* referencePtr ///< [OUT] The EFS metrics reference used by follow-up getters.
)
{
    if (referencePtr == nullptr)
    {
        LE_ERROR("referencePtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    taf_pa_mrc_EfsPeStatus_t stats;
    pa_result_t paResult = taf_pa_mrc_GetEfsPeStatus(&stats);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get EFS P/E status.");
        return result;
    }

    if (stats.peCountLen == 0 || stats.peCountLen > TAF_PA_MRC_EFS_PARTITION_BLOCKS)
    {
        LE_ERROR("Invalid block count %d for EFS.", stats.peCountLen);
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

    // First pass: accumulate the total P/E count and track the observed range.
    uint32_t sum = 0;
    uint32_t avg = 0;
    uint32_t sd = 0;
    uint32_t max = 0;
    uint32_t min = 0xFFFFFFFF;
    for (uint32_t i = 0; i < stats.peCountLen; i++)
    {
        uint32_t peCount = stats.peCount[i];
        sum += peCount;

        if (peCount > max)
            max = peCount;

        if (peCount < min)
            min = peCount;
    }

    avg = sum / stats.peCountLen;

    // Second pass: compute the sum of squared differences from the average, then derive the
    // standard deviation rounded up to the nearest integer. Use a 64-bit accumulator and integer
    // multiplication: P/E counts can reach the tens of thousands, so a single squared deviation
    // (~diff^2) already approaches UINT32_MAX and summing across all blocks would overflow a
    // uint32_t and yield a garbage deviation.
    uint64_t ssd = 0;
    for (uint32_t i = 0; i < stats.peCountLen; i++)
    {
        uint32_t peCount = stats.peCount[i];
        uint64_t diff = (peCount >= avg) ? (peCount - avg) : (avg - peCount);
        ssd += diff * diff;
    }

    sd = (uint32_t)ceil(sqrt((double)(ssd / stats.peCountLen)));

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_mem_ForceAlloc(mrcFactory.pools.metrics);
    metricsPtr->maxCount = max;
    metricsPtr->minCount = min;
    metricsPtr->avgCount = avg;
    metricsPtr->sdValue = sd;
    metricsPtr->badBlockCount = blockStatus.totalBadBlocks;

    // Retain the per-block P/E counts so range-distribution queries can be answered later without
    // re-reading the lower layer.
    metricsPtr->blockCount = stats.peCountLen;
    for (uint32_t i = 0; i < stats.peCountLen; i++)
        metricsPtr->peCount[i] = stats.peCount[i];

    // Publish the measurements as an opaque reference so callers can retrieve individual values
    // without exposing the internal storage layout.
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
 * Get the number of blocks whose P/E count falls within the range [lower, upper).
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_GetEfsBlocksInPECountRange
(
    taf_mrc_MetricsRef_t reference, ///< [IN] The EFS metrics reference.
    uint32_t lower,                 ///< [IN] Lower bound (inclusive).
    uint32_t upper,                 ///< [IN] Upper bound (exclusive).
    uint32_t* countPtr              ///< [OUT] Number of blocks with lower <= P/E count < upper.
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

    if (lower >= upper)
    {
        LE_ERROR("Invalid range: lower %u >= upper %u", lower, upper);
        return LE_BAD_PARAMETER;
    }

    auto& mrcFactory = MRCFactory::GetInstance();
    Metrics_t* metricsPtr = (Metrics_t*)le_ref_Lookup(mrcFactory.maps.metrics, reference);
    if (metricsPtr == nullptr)
    {
        LE_ERROR("Invalid para(null reference ptr)");
        return LE_NOT_FOUND;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < metricsPtr->blockCount; i++)
    {
        if (metricsPtr->peCount[i] >= lower && metricsPtr->peCount[i] < upper)
            count++;
    }

    *countPtr = count;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the period of EFS backup.
 *
 * @return
 *  - LE_OK -- The backup period was accepted by the underlying timer configuration path.
 *  - LE_FAULT -- The underlying timer-configuration path failed with -EFAULT, or another unmapped
 *                lower-layer error is converted to LE_FAULT.
 *  - LE_BAD_PARAMETER -- The timer identifier conversion or the underlying timer-configuration path
 *                        returned -EINVAL.
 *  - LE_TIMEOUT -- The underlying taf_pa_mrc_SetTimerPeriod() path returned -ETIMEDOUT.
 *  - LE_UNSUPPORTED -- The underlying taf_pa_mrc_SetTimerPeriod() path returned -ENOTSUP.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_SetTimerPeriod().
 *
 *  This function returns Utility::Convert::Result(taf_pa_mrc_SetTimerPeriod(...)). The exact
 *  values depend on the lower-layer implementation; from the visible code paths, the propagated
 *  results are 0, -EINVAL, -EFAULT, -ETIMEDOUT, -ENOTSUP, or -ENOSYS.
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

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the GPIO toggle status.
 *
 * @return
 *  - LE_OK -- status was converted successfully and the acknowledgement was accepted by the
 *             underlying taf_pa_mrc_AckSlotToggle() path.
 *  - LE_FAULT -- The underlying GPIO-toggle acknowledgement path failed with -EFAULT, or another
 *                unmapped lower-layer error is converted to LE_FAULT.
 *  - LE_BAD_PARAMETER -- status is unsupported by Utility::Convert::Status(), statusPtr is null in
 *                        the conversion helper, or the underlying acknowledgement path returned
 *                        -EINVAL.
 *  - LE_TIMEOUT -- The underlying taf_pa_mrc_AckSlotToggle() path returned -ETIMEDOUT.
 *  - LE_UNSUPPORTED -- The underlying taf_pa_mrc_AckSlotToggle() path returned -ENOTSUP.
 *  - LE_NOT_IMPLEMENTED -- The lower-layer implementation is not available and propagates
 *                          -ENOSYS through taf_pa_mrc_AckSlotToggle().
 *
 *  This function can return values from both the local status-conversion step and the propagated
 *  result of taf_pa_mrc_AckSlotToggle(). The exact values depend on the lower-layer
 *  implementation; from the visible code paths, the propagated results are 0, -EFAULT,
 *  -ETIMEDOUT, -EINVAL, -ENOTSUP, or -ENOSYS.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mrc_SetGpioToggleStatus
(
    taf_mrc_ToggleStatus_t status ///< [IN] GPIO toggle status.
)
{
    int32_t paStatus = 0;
    le_result_t result = Utility::Convert::Status(status, &paStatus);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert toggle status %d.", status);
        return result;
    }		

    pa_result_t paResult = taf_pa_mrc_AckSlotToggle(paStatus);

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for toggling bank.
 *
 * @return
 *  - taf_mrc_ToggleBankHandlerRef_t handler reference for toggling bank.
 */
//--------------------------------------------------------------------------------------------------
taf_mrc_ToggleBankHandlerRef_t taf_mrc_AddToggleBankHandler
(
    taf_mrc_ToggleBankHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                ///< [IN] Handler context.
)
{
    auto& mrcFactory = MRCFactory::GetInstance();

    // Register a layered handler so the generic Legato event system can invoke the typed public
    // callback while preserving the caller-provided context pointer.
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ToggleBank",
        mrcFactory.events.toggleBank, Utility::LayeredFunction::ToggleBank,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_mrc_ToggleBankHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for toggling bank.
 */
//--------------------------------------------------------------------------------------------------
void taf_mrc_RemoveToggleBankHandler
(
    taf_mrc_ToggleBankHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    // The handler reference is the underlying Legato event handler returned at registration time.
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
