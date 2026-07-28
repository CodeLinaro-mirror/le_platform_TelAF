/*
 *  Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(5);

    LE_TEST_INFO("======== MRC OTA Test ========");

    le_result_t result = taf_mrc_SendOtaResumeMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaResumeMsg - LE_OK");

    result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");

    result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_INIT);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");

    result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_SUCCESS);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");

    exit(EXIT_SUCCESS);
}
