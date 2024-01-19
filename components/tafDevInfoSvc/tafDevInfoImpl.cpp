/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDevInfoImpl.cpp
 * @brief      This file describes the implementation for device info service
 */

#include "tafDevInfo.hpp"

/**
 * callback to receive Device information service status change
 */
void tafdevinfoServiceStatusListener::onServiceStatusChange(ServiceStatus status) {
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LE_INFO("Service Status : UNAVAILABLE");
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        LE_INFO("Service Status : AVAILABLE");
    }
}

taf_info& taf_info::GetInstance() {
    static taf_info obj;
    return obj;
}

le_result_t taf_info::GetIMEI(char* imeiPtr, size_t numElements) {
    LE_INFO("taf_info::GetIMEI");

    std::string imei = "";
    telux::common::Status status = deviceInfoManager->getIMEI(imei);
    TAF_ERROR_IF_RET_VAL(status != Status::SUCCESS, LE_FAULT,
        "request for IMEI failed(status = %d)", static_cast<int>(status));

    le_utf8_Copy(imeiPtr, imei.c_str(), numElements, nullptr);

    LE_INFO("Retrieved IMEI successfully: %s", imeiPtr);

    return LE_OK;
}

void taf_info::Init() {
    LE_INFO("taf_info::Init");

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
}
