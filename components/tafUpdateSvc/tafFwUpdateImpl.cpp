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

#include "tafFwUpdate.hpp"

using namespace std;
using namespace telux::tafsvc;

taf_FwUpdate &taf_FwUpdate::GetInstance()
{
    static taf_FwUpdate instance;
    return instance;
}

le_result_t taf_FwUpdate::SendPipeCmd(const char* cmd, const char* mod)
{
    FILE* fp = popen(cmd, mod);
    TAF_ERROR_IF_RET_VAL(fp == NULL, LE_FAULT, "popen failed.");

    int res = pclose(fp);
    if (WIFEXITED(res)) {
        res = WEXITSTATUS(res);
    }

    TAF_ERROR_IF_RET_VAL(res != 0, LE_FAULT, "pclose errno(%d), result(%d).", errno, res);

    return LE_OK;
}

taf_FwUpdateError_t taf_FwUpdate::Install(const char* filePath)
{
#ifdef TARGET_SA515M
    TAF_ERROR_IF_RET_VAL(taf_mrc_SendOtaStartMsg() != LE_OK, TAF_FWUPDATE_ERROR_MRC_FAULT,
        "Fail to send OTA start message to MRC daemon.");
#endif

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    char instCmd[TAF_FWUPDATE_INSTALL_CMD_LEN];
    snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s", filePath);
    TAF_ERROR_IF_RET_VAL(tafFwUpdate.SendPipeCmd(instCmd, "w") != LE_OK, TAF_FWUPDATE_ERROR_RCV_FAULT,
        "Fail to send pipe cmd.");

    ifstream fin(TAF_FWUPDATE_RECOVERY_LOG_FILE);
    string strline;
    int line = 0;
    taf_FwUpdateError_t ret = TAF_FWUPDATE_ERROR_MRC_FAULT;
    while (getline(fin, strline)) {
        line++;
        if (!(strline.find("Starting recovery") == string::npos)) {
            LE_DEBUG("Found Starting recovery in line %d", line);
            ret = TAF_FWUPDATE_ERROR_RCV_FAULT;
        }

        if (!(strline.find("upgrade success") == string::npos)) {
            LE_DEBUG("Found upgrade success in line %d", line);
            ret = TAF_FWUPDATE_ERROR_NONE;
        }
    }
    fin.close();

#ifdef TARGET_SA515M
    TAF_ERROR_IF_RET_VAL(taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS) != LE_OK, TAF_FWUPDATE_ERROR_MRC_FAULT,
        "Fail to send OTA end message to MRC daemon.");
#endif

    return ret;
}

void taf_FwUpdate::Init(void)
{
}
