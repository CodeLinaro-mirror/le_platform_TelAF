/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDevInfoIntTest.cpp
 * @brief      Integration test functions for Device Info
 */

#include <string>
#include <array>
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
    std::array<char, TAF_DEVINFO_IMEI_MAX_BYTES> imei{};
    le_result_t result = taf_devInfo_GetImei(imei.data(), imei.size());
    LE_INFO("taf_devInfo_GetImei Return:%d\n", result);
    printf("Imei : %s\n", imei.data());
    return result;
}

static le_result_t Test_GetDeviceModel()
{
    std::array<char, TAF_DEVINFO_MODEL_MAX_BYTES> model{};
    le_result_t result = taf_devInfo_GetModel(model.data(), model.size());
    LE_INFO("taf_devInfo_GetModel Return : %d\n", result);
    printf("Device Model : %s\n", model.data());
    return result;
}

inline void CheckNumArgs(size_t numArgs, size_t expectedNumArgs)
{
    if (numArgs != expectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}

COMPONENT_INIT
{
    LE_INFO("%s [start]", __FUNCTION__);

    le_result_t status = LE_FAULT;
    LE_TEST_INIT;

    LE_TEST_INFO("======== Device Info Integration Test ========");
    size_t numArgs = le_arg_NumArgs();

    if(numArgs == 1)
    {
        const char *testType = le_arg_GetArg(0);
        if(testType == nullptr)
        {
            LE_ERROR("testType is nullptr");
            return;
        }
        if (std::string(testType) == std::string("imei"))
        {
            LE_TEST_INFO("======== GetImei Test ========");
            CheckNumArgs(numArgs, 1);
            status = Test_GetImei();
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
            LE_TEST_OK(status == LE_OK, "Test taf_devInfo_GetImei: End");
#endif
#ifndef LE_CONFIG_GET_IMEI_SUPPORT
            LE_TEST_OK(status == LE_UNSUPPORTED, "UNSUPPORTED :Test taf_devInfo_GetImei End");
#endif
       }
       else if (std::string(testType) == std::string("model"))
       {
            LE_TEST_INFO("======== GetDeviceModel========");
            CheckNumArgs(numArgs, 1);
            status = Test_GetDeviceModel();
            LE_TEST_OK(LE_OK == status, "GetDeviceModel Test: End");
       }
       else
       {
            LE_TEST_FATAL("Invalid test type %s", testType);
       }
    }
    else
    {
        LE_ERROR("Arguments are empty!!");
        PrintUsage();
    }

    LE_TEST_EXIT;
}
