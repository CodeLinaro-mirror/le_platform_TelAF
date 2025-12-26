/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDevInfoImpl.cpp
 * @brief      This file describes the implementation for device info service
 */

#include "tafDevInfo.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
using namespace std;

taf_devInfo& taf_devInfo::GetInstance() {
    static taf_devInfo obj;
    return obj;
}

#ifdef LE_CONFIG_GET_IMEI_SUPPORT
le_result_t taf_devInfo::GetIMEI(char* imeiPtr, size_t numElements) {
    LE_INFO("taf_devInfo::GetIMEI");

    char imeiValue[TAF_DEVINFO_IMEI_MAX_BYTES];

    pa_result_t status = taf_pa_deviceinfo_GetIMEI(imeiValue, sizeof(imeiValue));
    TAF_ERROR_IF_RET_VAL(status != PA_OK, LE_FAULT,
         "request for IMEI failed(status = %d)", static_cast<int>(status));

    LE_INFO("Retrieved IMEI successfully: %s", imeiValue);

    // Use safe string copy with proper bounds checking
    le_utf8_Copy(imeiPtr, imeiValue, numElements, nullptr);

    LE_INFO("Retrieved IMEI successfully: %s", imeiPtr);

    return LE_OK;
}
#endif

le_result_t taf_devInfo::GetDeviceModel(char* modelPtr, size_t numElements) {
    LE_DEBUG("taf_devInfo::GetDeviceModel");
    std::string model="";
    fstream file;
    file.open("/etc/hostname",ios::in);
    if (file.is_open()){
        std::string temp = "";
        while(getline(file, temp)){
            model += temp;
        }
        file.close();
    }else{
        LE_ERROR("Error opening file");
        return LE_FAULT;
    }
    le_utf8_Copy(modelPtr, model.c_str(), numElements, nullptr);
    LE_DEBUG("Retrieved MODEL successfully: %s", modelPtr);
    return LE_OK;
}

void taf_devInfo::Init() {
    LE_INFO("taf_devInfo::Init");

#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    pa_result_t  result;
    result = taf_pa_deviceinfo_Init();
    if (result != PA_OK)
    {
        LE_FATAL("Cannot initialize device info platform adaptor");
    }
#endif
    LE_INFO("System ready, start device info service!\n");
}
