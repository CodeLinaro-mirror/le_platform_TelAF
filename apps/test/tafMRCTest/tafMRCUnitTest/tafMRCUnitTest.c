/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

#define EFS_BACKUP_PERIOD 3000

static void TestActivities
(
    void
)
{
    le_result_t result = taf_mrc_SendOtaAbsyncMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaAbsyncMsg - LE_OK");

    result = taf_mrc_SendOtaResumeMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaResumeMsg - LE_OK");

    result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");

    result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_INIT);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");

    result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_SUCCESS);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
}

static void TestEfsMetrics
(
    void
)
{
    taf_mrc_MetricsRef_t metrics = NULL;
    le_result_t result = taf_mrc_MeasureEfsMetrics(&metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_MeasureEfsMetrics - LE_OK");

    uint32_t max = 0, min = 0, avg = 0, sd = 0, badblocks = 0;
    result = taf_mrc_GetEfsMaxPECount(metrics, &max);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsMaxPECount - LE_OK");

    result = taf_mrc_GetEfsMinPECount(metrics, &min);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsMinPECount - LE_OK");

    result = taf_mrc_GetEfsAvgPECount(metrics, &avg);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsAvgPECount - LE_OK");

    result = taf_mrc_GetEfsPEStandardDeviation(metrics, &sd);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsPEStandardDeviation - LE_OK");

    result = taf_mrc_GetEfsBadBlocks(metrics, &badblocks);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsBadBlocks - LE_OK");

    LE_INFO("PE Max: %d", max);
    LE_INFO("PE Min: %d", min);
    LE_INFO("PE Average: %d", avg);
    LE_INFO("PE Standard Deviation: %d", sd);
    LE_INFO("Bad blocks: %d", badblocks);

    result = taf_mrc_DeleteEfsMetrics(metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_DeleteEfsMetrics - LE_OK");
}

static void TestPeriodConfiguration
(
    void
)
{
    le_result_t result = taf_mrc_SetEfsBackupPeriod(EFS_BACKUP_PERIOD);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SetEfsBackupPeriod - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("======== Activities Test ========");
    TestActivities();
    LE_TEST_INFO("======== EFS Metrics Test ========");
    TestEfsMetrics();
    LE_TEST_INFO("======== Period Configuration Test ========");
    TestPeriodConfiguration();

    LE_TEST_EXIT;
}
