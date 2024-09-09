/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * callback to receive Device information service status change
 */
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
void tafdevinfoServiceStatusListener::onServiceStatusChange(ServiceStatus status) {
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LE_INFO("Service Status : UNAVAILABLE");
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        LE_INFO("Service Status : AVAILABLE");
    }
}
#endif

taf_devInfo& taf_devInfo::GetInstance() {
    static taf_devInfo obj;
    return obj;
}

#ifdef LE_CONFIG_GET_IMEI_SUPPORT
le_result_t taf_devInfo::GetIMEI(char* imeiPtr, size_t numElements) {
    LE_INFO("taf_devInfo::GetIMEI");

    std::string imei = "";
    telux::common::Status status = deviceInfoManager->getIMEI(imei);
    TAF_ERROR_IF_RET_VAL(status != Status::SUCCESS, LE_FAULT,
        "request for IMEI failed(status = %d)", static_cast<int>(status));
    le_utf8_Copy(imeiPtr, imei.c_str(), numElements, nullptr);

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
    // Get platform factory.
    auto& platformFactory = PlatformFactory::getInstance();

    std::promise<ServiceStatus> p;
    auto cb = [&p](ServiceStatus status) {
        LE_INFO("Received service status: %d", static_cast<int>(status));
        p.set_value(status);
    };

    deviceInfoManager = platformFactory.getDeviceInfoManager(cb);
    if (deviceInfoManager == nullptr) {
        LE_FATAL("Failed to get Device Info Manager instance");
    }

    LE_INFO("Obtained deviceInfo manager");

    // Wait until initialization is complete.
    p.get_future().get();
    if (deviceInfoManager->getServiceStatus() != ServiceStatus::SERVICE_AVAILABLE) {
        LE_FATAL("DeviceInfo service not available");
    }

    // Register for Device information service status change
    devinfoServiceStatusListener = std::make_shared<tafdevinfoServiceStatusListener>();
    telux::common::Status status
        = deviceInfoManager->registerListener(devinfoServiceStatusListener);
    if (status != telux::common::Status::SUCCESS) {
        LE_ERROR("Failed to register for service state change ");
    }
#endif
}
