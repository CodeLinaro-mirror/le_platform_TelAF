/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Sends an OTA start message to the MRC service.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_Start
(
    void
)
{
    le_result_t result = taf_mrc_SendOtaStartMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaStartMsg - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends an OTA resume message to the MRC service.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_Resume
(
    void
)
{
    le_result_t result = taf_mrc_SendOtaResumeMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaResumeMsg - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends an OTA end message with a specified status.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_End
(
    tafMRCIntTest_OtaOpStatus_t status ///< [IN] OTA operation status.
)
{
    le_result_t result;
    if (status == TAFMRCINTTEST_OTA_OP_STATUS_SUCCESS)
    {
        result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS);
    }
    else
    {
        result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE);
    }
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends an OTA synchronization message.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_Sync
(
    tafMRCIntTest_SyncStatus_t status ///< [IN] Sync status.
)
{
    le_result_t result;
    if (status == TAFMRCINTTEST_SYNC_STATUS_INIT)
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_INIT);
    }
    else if (status == TAFMRCINTTEST_SYNC_STATUS_SUCCESS)
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_SUCCESS);
    }
    else
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_FAILURE);
    }
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends an OTA synchronization message (Forced/Absync).
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_Absync
(
    void
)
{
    le_result_t result = taf_mrc_SendOtaAbsyncMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaAbsyncMsg - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the time interval for the EFS backup period.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_SetEfsBackupPeriod
(
    uint32_t period ///< [IN] Time in seconds for the backup period.
)
{
    le_result_t result = taf_mrc_SetEfsBackupPeriod((long)period);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SetEfsBackupPeriod - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the status of the EFS partition.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_GetEfsMetrics
(
    uint32_t* maxPE,      ///< [OUT] Max PE count.
    uint32_t* minPE,      ///< [OUT] Min PE count.
    uint32_t* avgPE,      ///< [OUT] Average PE count.
    uint32_t* stdDevPE,   ///< [OUT] PE Standard Deviation.
    uint32_t* badBlocks   ///< [OUT] Bad blocks count.
)
{
    taf_mrc_MetricsRef_t metrics = NULL;
    le_result_t result = taf_mrc_MeasureEfsMetrics(&metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_MeasureEfsMetrics - LE_OK");

    if (result == LE_OK)
    {
        taf_mrc_GetEfsMaxPECount(metrics, maxPE);
        taf_mrc_GetEfsMinPECount(metrics, minPE);
        taf_mrc_GetEfsAvgPECount(metrics, avgPE);
        taf_mrc_GetEfsPEStandardDeviation(metrics, stdDevPE);
        taf_mrc_GetEfsBadBlocks(metrics, badBlocks);

        taf_mrc_DeleteEfsMetrics(metrics);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the number of EFS blocks whose PE count falls within the range [lower, upper).
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMRCIntTest_GetEfsBlocksInPECountRange
(
    uint32_t lower,       ///< [IN] Lower bound (inclusive).
    uint32_t upper,       ///< [IN] Upper bound (exclusive).
    uint32_t* count       ///< [OUT] Number of blocks with lower <= PE count < upper.
)
{
    taf_mrc_MetricsRef_t metrics = NULL;
    le_result_t result = taf_mrc_MeasureEfsMetrics(&metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_MeasureEfsMetrics - LE_OK");

    if (result != LE_OK)
    {
        return result;
    }

    result = taf_mrc_GetEfsBlocksInPECountRange(metrics, lower, upper, count);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsBlocksInPECountRange - LE_OK");

    taf_mrc_DeleteEfsMetrics(metrics);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the GPIO toggle status.
 */
//--------------------------------------------------------------------------------------------------
void tafMRCIntTest_SetGpioToggleStatus
(
    tafMRCIntTest_ToggleStatus_t status ///< [IN] The GPIO toggle status.
)
{
    taf_mrc_ToggleStatus_t mrcStatus;
    
    switch(status)
    {
        case TAFMRCINTTEST_TOGGLE_STATUS_REQUESTED:
            mrcStatus = TAF_MRC_TOGGLE_STATUS_REQUESTED;
            break;
        case TAFMRCINTTEST_TOGGLE_STATUS_SUCCEEDED:
            mrcStatus = TAF_MRC_TOGGLE_STATUS_SUCCEEDED;
            break;
        case TAFMRCINTTEST_TOGGLE_STATUS_FAILED:
            mrcStatus = TAF_MRC_TOGGLE_STATUS_FAILED;
            break;
        default:
            mrcStatus = TAF_MRC_TOGGLE_STATUS_UNKNOWN;
            break;
    }

    le_result_t result = taf_mrc_SetGpioToggleStatus(mrcStatus);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SetGpioToggleStatus - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for ToggleBank event.
 */
//--------------------------------------------------------------------------------------------------
static void MrcToggleBankHandler
(
    taf_mrc_ToggleStatus_t status,
    taf_mrc_Bank_t bank,
    taf_mrc_Error_t error,
    void* context
)
{
    LE_INFO("Received ToggleBank Event: Status=%d, Bank=%d, Error=%d", status, bank, error);
}

COMPONENT_INIT
{
    taf_mrc_AddToggleBankHandler(MrcToggleBankHandler, NULL);
}