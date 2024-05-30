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

taf_info& taf_info::GetInstance() {
    static taf_info obj;
    return obj;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get version from Plug-In.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_info::GetVersionFromPlugIn
(
    taf_pi_version_Comp_t component, ///< [IN] Component
    char* versionPtr,                ///< [OUT] Version string.
    size_t versionSize               ///< [IN] Size of the version string.
)
{
    if (versionInfPtr->getFormat == NULL)
    {
        LE_ERROR("Version format undefined.");
        return LE_FAULT;
    }

    uint32_t format = (*(versionInfPtr->getFormat))();
    if (format & TAF_PI_VERSION_FORMAT_MAJOR_MINOR_PATCH)
    {
        if (versionInfPtr->getMajor != NULL && versionInfPtr->getMinor != NULL &&
            versionInfPtr->getPatch != NULL)
        {
            char major[TAF_PI_VERSION_MAJOR_MAX_BYTES] = "";
            char minor[TAF_PI_VERSION_MINOR_MAX_BYTES] = "";
            char patch[TAF_PI_VERSION_PATCH_MAX_BYTES] = "";

            int ret = (*(versionInfPtr->getMajor))(component, major, sizeof(major));
            if (!ret)
            {
                LE_INFO("Major version: %s", major);
                le_utf8_Copy(versionPtr, major, versionSize, NULL);
                le_utf8_Append(versionPtr, ".", versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get major version.");
                return LE_FAULT;
            }

            ret = (*(versionInfPtr->getMinor))(component, minor, sizeof(minor));
            if (!ret)
            {
                LE_INFO("Minor version: %s", minor);
                le_utf8_Append(versionPtr, minor, versionSize, NULL);
                le_utf8_Append(versionPtr, ".", versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get monor version.");
                return LE_FAULT;
            }

            ret = (*(versionInfPtr->getPatch))(component, patch, sizeof(patch));
            if (!ret)
            {
                LE_INFO("Patch version: %s", patch);
                le_utf8_Append(versionPtr, patch, versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get patch version.");
                return LE_FAULT;
            }
        }
    }

    return LE_OK;
}

#ifdef LE_CONFIG_GET_IMEI_SUPPORT
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
#endif

le_result_t taf_info::GetDeviceModel(char* modelPtr, size_t numElements) {
    LE_DEBUG("taf_info::GetDeviceModel");
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


std::string getDataFromFile(std::ifstream & input, const std::string key) {
        size_t pos;
        std::string line = "";
        std::string version = "";
        input.clear();
        input.seekg(0, std::ios::beg);
        while (getline(input, line)) {
            pos = line.find(key.c_str());
            if (pos != std::string::npos) {
                pos = line.find(":");
                if (pos != std::string::npos) {
                    version = line.substr(pos);
                    pos = version.find("\"");
                    if (pos != std::string::npos) {
                        version = version.substr(pos + 1);
                        pos = version.find("\"");
                        if (pos != std::string::npos) {
                            version = version.substr(0, pos);
                        }
                    }
                }
                break;
            }
        }
        return version;
}

le_result_t taf_info::GetModemVersion(char* modemPtr, size_t numElements) {
    if (versionInfPtr == NULL)
    {
        LE_DEBUG("taf_info: GetModemVersion");
        std::string modem = "";
        std::string path = "/firmware/image/Ver_Info.txt";
        std::ifstream input(path.c_str(), std::ifstream::in);
        if (input && !input.eof()) {
            modem = getDataFromFile(input, "modem");
        }else{
            LE_ERROR("Error opening file");
            return LE_FAULT;
        }
        le_utf8_Copy(modemPtr, modem.c_str(), numElements, nullptr);
        LE_DEBUG("Retrieved Modem Version successfully: %s", modemPtr);
    }
    else
    {
        auto& tafDevInfo = taf_info::GetInstance();
        return tafDevInfo.GetVersionFromPlugIn(TAF_PI_VERSION_COMP_FIRMWARE, modemPtr,
            numElements);
    }
    return LE_OK;
}

le_result_t taf_info::GetTzVersion(char* tzPtr, size_t numElements) {
    if (versionInfPtr == NULL)
    {
        LE_DEBUG("taf_info: Get TZ Version");
        std::string tz = "";
        std::string path = "/firmware/image/Ver_Info.txt";
        std::ifstream input(path.c_str(), std::ifstream::in);
        if (input && !input.eof()) {
            tz = getDataFromFile(input, "tz");
        }else{
            LE_ERROR("Error opening file");
            return LE_FAULT;
        }
        le_utf8_Copy(tzPtr, tz.c_str(), numElements, nullptr);
        LE_DEBUG("Retrieved TZ Version successfully: %s", tzPtr);
    }
    else
    {
        auto& tafDevInfo = taf_info::GetInstance();
        return tafDevInfo.GetVersionFromPlugIn(TAF_PI_VERSION_COMP_TZ, tzPtr, numElements);
    }
    return LE_OK;
}

le_result_t taf_info::GetKernelVersion(char* versionPtr, size_t numElements) {
    if (versionInfPtr == NULL)
    {
        LE_DEBUG("taf_info: GetKernelVersion");
        std::string version = "";
        std::string boot = "";
        fstream file;
        std::string path = "/firmware/image/Ver_Info.txt";
        std::ifstream input(path.c_str(), std::ifstream::in);
        file.open("/proc/version",ios::in);
        if (file.is_open()){
            string line="";
            std::regex versionPattern("Linux version (\\d+\\.\\d+)");
            while(getline(file, line)){
                std::smatch match;
                if(std::regex_search(line,match,versionPattern)){
                    version = match[1].str();
                }
            }
            file.close();
        }else{
            LE_ERROR("Error opening kernel version file");
            return LE_FAULT;
        }
        if (input && !input.eof()) {
            boot = getDataFromFile(input, "boot");
        }else{
            LE_ERROR("Error opening version info file");
            return LE_FAULT;
        }
        version.append(" | ");
        version.append(boot);
        le_utf8_Copy(versionPtr, version.c_str(), numElements, nullptr);
        LE_DEBUG("Retrieved Kernel Version successfully: %s", versionPtr);
    }
    else
    {
        auto& tafDevInfo = taf_info::GetInstance();
        return tafDevInfo.GetVersionFromPlugIn(TAF_PI_VERSION_COMP_BOOT, versionPtr, numElements);
    }
    return LE_OK;
}

le_result_t taf_info::GetTelafVersion(char* telafVersionPtr, size_t numElements) {
    if (versionInfPtr == NULL)
    {
        LE_DEBUG("taf_info::GetTelafVersion");
        std::string telafVersion="";
        fstream file;
        size_t pos;
        file.open("/legato/systems/current/version",ios::in);
        if (file.is_open()){
            getline(file,telafVersion);
            pos = telafVersion.find("_");
            if(pos != std::string::npos){
                telafVersion = telafVersion.substr(0,pos);
            }
            file.close();
        }else{
            LE_ERROR("Error opening file");
            return LE_FAULT;
        }
        le_utf8_Copy(telafVersionPtr, telafVersion.c_str(), numElements, nullptr);
        LE_DEBUG("Retrieved TelAF Version successfully: %s", telafVersionPtr);
    }
    else
    {
        auto& tafDevInfo = taf_info::GetInstance();
        return tafDevInfo.GetVersionFromPlugIn(TAF_PI_VERSION_COMP_TELAF, telafVersionPtr,
            numElements);
    }
    return LE_OK;
}

le_result_t taf_info::GetRootfsVersion(char* rootfsVersionPtr, size_t numElements) {
    if (versionInfPtr == NULL)
    {
        LE_DEBUG("taf_info::GetRootfsVersion");
        std::string rootfsVersion="";
        fstream file;
        size_t pos;
        file.open("/etc/version",ios::in);
        if (file.is_open()){
            getline(file,rootfsVersion);
            pos = rootfsVersion.find("-");
            if(pos != std::string::npos){
                rootfsVersion = rootfsVersion.substr(0,pos);
            }
            file.close();
        }else{
            LE_ERROR("Error opening file");
            return LE_FAULT;
        }
        le_utf8_Copy(rootfsVersionPtr, rootfsVersion.c_str(), numElements, nullptr);
        LE_DEBUG("Retrieved RootFS Version successfully: %s", rootfsVersionPtr);
    }
    else
    {
        auto& tafDevInfo = taf_info::GetInstance();
        return tafDevInfo.GetVersionFromPlugIn(TAF_PI_VERSION_COMP_ROOTFS, rootfsVersionPtr,
            numElements);
    }
    return LE_OK;
}

void taf_info::Init() {
    LE_INFO("taf_info::Init");

    // Load plugin.
    versionInfPtr = (version_Inf_t *)taf_devMgr_LoadDrv(TAF_VERSION_MODULE_NAME, NULL);
    if (versionInfPtr == NULL)
    {
        LE_WARN("Version PI module is not loaded.");
    }
    else
    {
        if (versionInfPtr->init != NULL)
        {
            LE_INFO("Version PI init...");
            (*(versionInfPtr->init))();
        }
    }

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
