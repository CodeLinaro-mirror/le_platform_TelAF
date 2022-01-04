/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

namespace telux {
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
}

#endif
