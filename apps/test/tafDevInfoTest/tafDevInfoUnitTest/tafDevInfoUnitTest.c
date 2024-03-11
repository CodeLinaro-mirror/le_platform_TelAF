/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "interfaces.h"
#include "legato.h"

/*======================================================================

 FUNCTION        Test_taf_info_GetIMEI

 DESCRIPTION     Test to get IMEI of the device

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_info_GetImei() {
    char imei[TAF_INFO_IMEI_MAX_BYTES];
    le_result_t result = taf_info_GetImei(imei, sizeof(imei));
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetImei: End");
#endif
#ifndef LE_CONFIG_GET_IMEI_SUPPORT
    LE_TEST_OK(result == LE_UNSUPPORTED, "UNSUPPORTED on this Platform: Test taf_info_GetImei End");
#endif
}

__attribute__((unused)) static void Test_taf_info_GetModel() {
    char model[TAF_INFO_MODEL_MAX_BYTES];
    le_result_t result = taf_info_GetModel(model, sizeof(model));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetModel: End");
}


__attribute__((unused)) static void Test_taf_info_GetKernelVersion() {
    char version[TAF_INFO_VERSION_MAX_BYTES];
    le_result_t result = taf_info_GetKernelVersion(version, sizeof(version));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetKernelVersion: End");
}

__attribute__((unused)) static void Test_taf_info_GetModemVersion() {
    char modem[TAF_INFO_MODEM_MAX_BYTES];
    le_result_t result = taf_info_GetModemVersion(modem, sizeof(modem));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetModemVersion: End");
}

__attribute__((unused)) static void Test_taf_info_GetTZVersion() {
    char tz[TAF_INFO_TZ_MAX_BYTES];
    le_result_t result = taf_info_GetTZVersion(tz, sizeof(tz));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetTZVersion: End");
}

static void Test_taf_info_GetTelafVersion() {
    char telafVersion[TAF_INFO_TELAF_VERSION_MAX_BYTES];
    le_result_t result = taf_info_GetTelafVersion(telafVersion, sizeof(telafVersion));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetTelafVersion: End");
}

static void Test_taf_info_GetRootfsVersion() {
    char rootfsVersion[TAF_INFO_ROOTFS_VERSION_MAX_BYTES];
    le_result_t result = taf_info_GetRootfsVersion(rootfsVersion, sizeof(rootfsVersion));
    LE_TEST_OK(result == LE_OK, "Test taf_info_GetRootfsVersion: End");
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT {
    LE_TEST_INFO("======== Device Info Unit Test Start ========");

    LE_TEST_INFO("===== Test_taf_info_GetImei =====");
    Test_taf_info_GetImei();

    LE_TEST_INFO("======= Test_taf_info_GetModel========");
    Test_taf_info_GetModel();

    LE_TEST_INFO("==== Test_taf_info_GetModemVersion===========");
    Test_taf_info_GetModemVersion();

    LE_TEST_INFO("======= Test_taf_info_GetKernelVersion========");
    Test_taf_info_GetKernelVersion();

    LE_TEST_INFO("==== Test_taf_info_GetTZVersion===========");
    Test_taf_info_GetTZVersion();

    LE_TEST_INFO("==== Test taf_info_GetTelafVersion======");
    Test_taf_info_GetTelafVersion();

    LE_TEST_INFO("==== Test taf_info_GetRootfsVersion======");
    Test_taf_info_GetRootfsVersion();

    LE_TEST_INFO("======== Device Info Unit Test End ========");
    exit(EXIT_SUCCESS);
}
