/*
 *  Copyright (c) 2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFMRC_HPP
#define TAFMRC_HPP

#include "legato.h"
#include "interfaces.h"

#include <future>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/FsDefines.hpp>
#include <telux/platform/FsManager.hpp>

#include "tafSvcIF.hpp"

typedef enum
{
    TAF_MRC_OTA_MSG_TYPE_START,
    TAF_MRC_OTA_MSG_TYPE_RESUME,
    TAF_MRC_OTA_MSG_TYPE_END_SUCCESS,
    TAF_MRC_OTA_MSG_TYPE_END_FAILURE,
    TAF_MRC_OTA_MSG_TYPE_ABSYNC
} taf_MrcOtaMsgType_t;

namespace tafsvc {
    class taf_MrcOtaOperationsListener : public telux::platform::IFsListener {
    public:
        virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override;
    };

    class taf_Mrc : public ITafSvc {
    public:
        taf_Mrc() {};
        ~taf_Mrc() {};

        static taf_Mrc &GetInstance();
        void Init(void);

        le_result_t SendOtaMsg(taf_MrcOtaMsgType_t type);
        std::shared_ptr<telux::platform::IFsManager> fsManager;
    private:
        std::shared_ptr<taf_MrcOtaOperationsListener> otaOperationsListener;
    };
}

#endif
