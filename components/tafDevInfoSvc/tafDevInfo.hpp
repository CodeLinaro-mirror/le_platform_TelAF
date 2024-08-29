/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file        tafDevInfo.hpp
 * @brief       Interface for device information service object.
                The functions in this file are impletmented internally.
 */

#ifndef TAFDEVINFO_HPP
#define TAFDEVINFO_HPP

#include "interfaces.h"
#include "legato.h"
#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include <future>
#include <telux/platform/PlatformFactory.hpp>

using namespace telux::common;
using namespace telux::tafsvc;
using namespace telux::platform;
namespace telux {
namespace tafsvc {
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    class tafdevinfoServiceStatusListener : public telux::platform::IDeviceInfoListener {
    public:
        void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override;
    };
#endif
    class taf_devInfo : public ITafSvc {
    public:
        taf_devInfo() = default;
        ~taf_devInfo() = default;

        static taf_devInfo& GetInstance();
        void Init();

        le_result_t GetDeviceModel(char* modelPtr, size_t numElements);
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
        le_result_t GetIMEI(char* imeiPtr, size_t numElements);
        std::shared_ptr<telux::platform::IDeviceInfoListener> devinfoServiceStatusListener
            = nullptr;
        std::shared_ptr<IDeviceInfoManager> deviceInfoManager = nullptr;
#endif
    };
}
}

#endif
