/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDevInfoIntTest.c
 * @brief      Integration test functions for Device Info
 */

#include "legato.h"
#include "interfaces.h"

void PrintUsage(void)
{
    puts("\n"
         "app runProc tafDevInfoIntTest tafDevInfoIntTest -- imei\n"
         "app runProc tafDevInfoIntTest tafDevInfoIntTest -- model\n"
         "\n");
}

static le_result_t Test_GetImei()
{
    char imei[TAF_DEVINFO_IMEI_MAX_BYTES];
    le_result_t result = taf_devInfo_GetImei(imei, sizeof(imei));
    LE_INFO("taf_devInfo_GetImei Return:%d\n",result);
    printf("Imei : %s\n", imei);
    return result;
}

static le_result_t Test_GetDeviceModel()
{
    char model[TAF_DEVINFO_MODEL_MAX_BYTES];
    le_result_t result = taf_devInfo_GetModel(model, sizeof(model));
    LE_INFO("taf_devInfo_GetModel Return : %d\n", result);
    printf("Device Model : %s\n", model);
    return result;
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
{
    if (NumArgs!=ExpectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}


COMPONENT_INIT
{
    le_result_t status = LE_FAULT;

    LE_TEST_INIT;

    LE_TEST_INFO("======== Device Info Integration Test ========");
    size_t numArgs = le_arg_NumArgs();

    const char *testType = le_arg_GetArg(0);
    if (strncmp(testType, "imei", strlen(testType)) == 0)
    {
        LE_TEST_INFO("======== GetImei Test ========");
        CheckNumArgs(numArgs,1);
        status = Test_GetImei();
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
        LE_TEST_OK(status == LE_OK, "Test taf_devInfo_GetImei: End");
#endif
#ifndef LE_CONFIG_GET_IMEI_SUPPORT
        LE_TEST_OK(status == LE_UNSUPPORTED, "UNSUPPORTED :Test taf_devInfo_GetImei End");
#endif
    }
    else if (strncmp(testType, "model", strlen(testType)) == 0)
    {
        LE_TEST_INFO("======== GetDeviceModel========");
        CheckNumArgs(numArgs,1);
        status = Test_GetDeviceModel();
        LE_TEST_OK(LE_OK == status, "GetDeviceModel Test: End");
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
