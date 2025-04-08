/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "multiRoutMgrsTest.h"

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=============================================================");
    LE_TEST_INFO("=== telaf someip multiple routing managers testAppA BEGIN ===");
    LE_TEST_INFO("=============================================================");

    multiRoutMgrTest_DataInit();

    // TEST APP A offers service A on routing manager 0 (default routing manager).
    multiRoutMgrTest_OfferService(RT0_APP_A, NULL);
    // TEST APP A requests service B on routing manager 0 (default routing manager).
    multiRoutMgrTest_RequestService(RT0_APP_B, NULL);
    // TEST APP A offers service A on routing manager 1.
    multiRoutMgrTest_OfferService(RT1_APP_A, NULL);
    // TEST APP A requests service B on routing manager 2.
    multiRoutMgrTest_RequestService(RT2_APP_B, NULL);
}
