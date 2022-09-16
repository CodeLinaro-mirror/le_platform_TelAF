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

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"
#include "tafAppMgmt.hpp"

using namespace telux::tafsvc;

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Update service component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_INFO("tafUpdate Service Init...\n");
    LE_INFO("tafUpdate Component Init...\n");
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.Init();
    LE_INFO("tafUpdate Component Ready...\n");
    LE_INFO("tafAppMgmt Component Init...\n");
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    tafAppMgmt.Init();
    LE_INFO("tafAppMgmt Component Ready...\n");
    LE_INFO("tafFwUpdate Component Init...\n");
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.Init();
    LE_INFO("tafFwUpdate Component Ready...\n");
    LE_INFO("tafUpdate Service Ready...\n");
}

/*======================================================================
 FUNCTION        taf_update_Download
 DESCRIPTION     Download update package
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_update_Download()
{
    taf_UpdateUsrReq_t usrReq;
    usrReq.event = TAF_UPDATE_REQ_DOWNLOAD;
    le_event_Report(taf_Update::requestEvId, &usrReq, sizeof(taf_UpdateUsrReq_t));
}

/*======================================================================
 FUNCTION        taf_update_Install
 DESCRIPTION     Install update package
 PARAMETERS      [IN] ota: FOTA or SOTA
                 [IN] name: Update package name
 RETURN VALUE    le_result_t: Result of install request
======================================================================*/
le_result_t taf_update_Install(taf_update_OTA_t ota, const char* name)
{
    taf_UpdateUsrReq_t usrReq;
    usrReq.event = TAF_UPDATE_REQ_INSTALL;
    usrReq.ota = ota;
    le_utf8_Copy(usrReq.name, name, TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
    le_event_Report(taf_Update::requestEvId, &usrReq, sizeof(taf_UpdateUsrReq_t));

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_update_AddStateHandler
 DESCRIPTION     Add state handler for update
 PARAMETERS      [IN] handlerFuncPtr: Update state handler
                 [IN] contextPtr: Context
 RETURN VALUE    taf_update_StateHandlerRef_t: Handler reference
======================================================================*/
taf_update_StateHandlerRef_t taf_update_AddStateHandler
(
    taf_update_StateHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UpdateStateHandler",
        tafUpdate.stateEvId, taf_Update::StateLayeredHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_update_StateHandlerRef_t)handlerRef;
}

/*======================================================================
 FUNCTION        taf_update_RemoveStateHandler
 DESCRIPTION     Remove state handler for update
 PARAMETERS      [IN] taf_update_StateHandlerRef_t: Handler reference
 RETURN VALUE    void
======================================================================*/
void taf_update_RemoveStateHandler(taf_update_StateHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
