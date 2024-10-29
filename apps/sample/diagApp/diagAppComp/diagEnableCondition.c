/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diagPrivate.h"

#define MAX_ENABLE_CONDITION_ID 14

le_result_t diag_Init(void)
{
    LE_TEST_INFO("Init");
    le_result_t result = LE_OK;

    // Set enable condition
#ifdef DIAG_ENABLE_CONDITION_TEST
    for (int enableID = 1; enableID <= MAX_ENABLE_CONDITION_ID; enableID++)
    {
        result = taf_diag_SetEnableCondition(enableID, false);
        if (result !=LE_OK)
        {
            LE_ERROR("Failed to set enable condition for %d", enableID);
            break;
        }
    }
#else
    for (int enableID = 1; enableID <= MAX_ENABLE_CONDITION_ID; enableID++)
    {
        result = taf_diag_SetEnableCondition(enableID, true);
        if (result !=LE_OK)
        {
            LE_ERROR("Failed to set enable condition for %d", enableID);
            break;
        }
    }
#endif

    return LE_OK;
}