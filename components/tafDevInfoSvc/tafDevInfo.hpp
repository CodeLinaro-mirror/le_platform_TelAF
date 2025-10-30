/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
#include "taf_pa_deviceinfo.hpp"

using namespace tafpa::deviceinfo;

using namespace tafsvc;
namespace tafsvc {
    class taf_devInfo : public ITafSvc {
    public:
        taf_devInfo() = default;
        ~taf_devInfo() = default;

        static taf_devInfo& GetInstance();
        void Init();

        le_result_t GetDeviceModel(char* modelPtr, size_t numElements);
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
        le_result_t GetIMEI(char* imeiPtr, size_t numElements);
#endif
    };
}

#endif
