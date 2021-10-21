/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "tafUpdate.hpp"

using namespace telux::tafsvc;

COMPONENT_INIT
{
    LE_INFO("tafUpdate Service Init...\n");
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.Init();
    LE_INFO("tafUpdate Service Ready...\n");
}

le_result_t taf_update_Download(taf_update_ImageType_t type)
{
    taf_UpdateCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_UpdateCmdReq_t));
    cmdReq.cmdType = TAF_UPDATE_CMD_TYPE_ASYNC_DOWNLOAD;
    cmdReq.imageType = type;
    le_event_Report(taf_Update::updateCmdEvId, &cmdReq, sizeof(taf_UpdateCmdReq_t));
    return LE_OK;
}

le_result_t taf_update_Install(void)
{
    TAF_ERROR_IF_RET_VAL(access(TAF_UPDATE_INSATLL_PAKCAGE, 0), LE_NOT_FOUND,
        "%s not found", TAF_UPDATE_INSATLL_PAKCAGE);

    taf_UpdateCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_UpdateCmdReq_t));
    cmdReq.cmdType = TAF_UPDATE_CMD_TYPE_ASYNC_INSTALL;
    le_event_Report(taf_Update::updateCmdEvId, &cmdReq, sizeof(taf_UpdateCmdReq_t));

    return LE_OK;
}

le_result_t taf_update_RebootToActive()
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_pa_ReportState_t rState = TAF_UPDATE_PA_REPORT_REBOOT;
    int retry = 5;
    while (retry) {
        int ret = taf_update_pa_Report(tafUpdate.daSessionID, rState);
        if (ret) {
            LE_ERROR("Download agent report failed, retry = %d, ret = %d.", 6 - retry, ret);
        } else {
            LE_INFO("Download agent report success.");
            break;
        }
        retry--;
        le_thread_Sleep(1);
    }

    system("/sbin/reboot");

    // Just keep waiting.
    while (true) {
       ;
    }

    return LE_OK;
}
le_result_t taf_update_GetFirmwareVersion(char* versionPtr, size_t versionNumElements)
{
    std::ifstream fin(TAF_UPDATE_VERSION_FILE);
    std::string verstr;
    getline(fin, verstr);
    le_utf8_Copy(versionPtr, verstr.c_str(), TAF_UPDATE_MAX_VERS_LEN, NULL);
    return LE_OK;
}

le_result_t taf_update_Sync()
{
    return LE_UNSUPPORTED;
}

taf_update_StateHandlerRef_t taf_update_AddStateHandler
(
    taf_update_StateHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UpdateStateHandler",
        tafUpdate.updateStateEvId, taf_Update::FirstLayerStateHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_update_StateHandlerRef_t)handlerRef;
}

void taf_update_RemoveStateHandler(taf_update_StateHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
