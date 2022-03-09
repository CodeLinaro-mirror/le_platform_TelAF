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

#ifndef TAFFWUPDATE_HPP
#define TAFFWUPDATE_HPP

#include "legato.h"
#include "interfaces.h"

#include <fstream>
#include <iostream>
#include <string>

#include "tafSvcIF.hpp"

#define TAF_FWUPDATE_INSTALL_CMD_LEN 50

#define TAF_FWUPDATE_RECOVERY_LOG_FILE "/tmp/recovery.log"
#define TAF_FWUPDATE_VERSION_FILE "/etc/version"

typedef enum
{
    TAF_FWUPDATE_ERROR_NONE,
    TAF_FWUPDATE_ERROR_MRC_FAULT,
    TAF_FWUPDATE_ERROR_RCV_FAULT
} taf_FwUpdateError_t;

namespace telux {
namespace tafsvc {
    class taf_FwUpdate : public ITafSvc {
    public:
        taf_FwUpdate() {};
        ~taf_FwUpdate() {};

        static taf_FwUpdate &GetInstance();
        le_result_t SendPipeCmd(const char* cmd, const char* mod);
        taf_FwUpdateError_t Install(const char* filePath);
        void Init(void);
    };
}
}

#endif
