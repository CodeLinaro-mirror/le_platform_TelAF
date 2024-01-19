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
    LE_TEST_INFO("IMEI: %s", imei);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_info_GetImei");
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
    LE_TEST_INFO("##### Test_taf_info_GetImei OK #####");

    LE_TEST_INFO("======== Device Info Unit Test End ========");
    exit(EXIT_SUCCESS);
}
