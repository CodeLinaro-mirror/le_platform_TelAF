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
    LE_INFO("tafAppMgmt Component Init...\n");
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    tafAppMgmt.Init();
    LE_INFO("tafAppMgmt Component Ready...\n");
    LE_INFO("tafFwUpdate Component Init...\n");
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.Init();
    LE_INFO("tafFwUpdate Component Ready...\n");
    LE_INFO("tafUpdate Component Init...\n");
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.Init();
    LE_INFO("tafUpdate Component Ready...\n");
    LE_INFO("tafUpdate Service Ready...\n");
}

void taf_update_Download()
{
    taf_UpdateCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_UpdateCmdReq_t));
    cmdReq.cmdType = TAF_UPDATE_CMD_TYPE_DOWNLOAD;
    le_event_Report(taf_Update::updateCmdEvId, &cmdReq, sizeof(taf_UpdateCmdReq_t));
}

le_result_t taf_update_Install(taf_update_Package_t packageType, const char* packageName)
{
    TAF_ERROR_IF_RET_VAL(!((packageType == TAF_UPDATE_PACKAGE_FOTA) || (packageType == TAF_UPDATE_PACKAGE_SOTA)), LE_FAULT,
        "Invalid package type %d.", packageType);

    taf_UpdateCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_UpdateCmdReq_t));
    cmdReq.cmdType = TAF_UPDATE_CMD_TYPE_INSTALL;
    cmdReq.pkgType = packageType;
    cmdReq.pkgName = packageName;
    le_event_Report(taf_Update::updateCmdEvId, &cmdReq, sizeof(taf_UpdateCmdReq_t));

    return LE_OK;
}

taf_update_StateHandlerRef_t taf_update_AddStateHandler
(
    taf_update_StateHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UpdateStateHandler",
        tafUpdate.updateStateEvId, taf_Update::UpdateStateLayeredHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_update_StateHandlerRef_t)handlerRef;
}

void taf_update_RemoveStateHandler(taf_update_StateHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
