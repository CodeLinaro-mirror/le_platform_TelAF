/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "interfaces.h"
#include "legato.h"

__attribute__((unused)) static void Test_taf_devInfo_GetImei() {
    char imei[TAF_DEVINFO_IMEI_MAX_BYTES];
    le_result_t result = taf_devInfo_GetImei(imei, sizeof(imei));
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    LE_TEST_OK(result == LE_OK, "Test taf_devInfo_GetImei: End");
#endif
#ifndef LE_CONFIG_GET_IMEI_SUPPORT
    LE_TEST_OK(result == LE_UNSUPPORTED, "UNSUPPORTED on this Platform: Test taf_devInfo_GetImei End");
#endif
}

__attribute__((unused)) static void Test_taf_devInfo_GetModel() {
    char model[TAF_DEVINFO_MODEL_MAX_BYTES];
    le_result_t result = taf_devInfo_GetModel(model, sizeof(model));
    LE_TEST_OK(result == LE_OK, "Test taf_devInfo_GetModel: End");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT {
    LE_TEST_INFO("======== Device Info Unit Test Start ========");

    LE_TEST_INFO("===== Test_taf_devInfo_GetImei =====");
    Test_taf_devInfo_GetImei();

    LE_TEST_INFO("======= Test_taf_devInfo_GetModel========");
    Test_taf_devInfo_GetModel();

    LE_TEST_INFO("======== Device Info Unit Test End ========");
    exit(EXIT_SUCCESS);
}
