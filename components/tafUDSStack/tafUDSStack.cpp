/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"
#include "tafUDSStack.h"
#include "tafUDSCommunicationMgr.hpp"

using namespace taf::uds;

le_result_t taf_uds_SendDiagResp
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,     ///< [IN] Diagnostic message pointer.
    taf_uds_ServiceId_t serviceId,             ///< [IN] Service Id.
    uint8_t err                                ///< [IN] Error code.
)
{
    LE_DEBUG("taf_uds_SendDiagResp");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_BAD_PARAMETER;
    }

    memcpy(&udsCmMgr.udsRespAddrInfo, addrInfoPtr, sizeof(*addrInfoPtr));

    if (diagMsgPtr != NULL)
    {
        return udsCmMgr.SendUDSResp(addrInfoPtr->sa, addrInfoPtr->ta, addrInfoPtr->taType,
                serviceId, err, diagMsgPtr->dataPtr, diagMsgPtr->dataLen);
    }
    else
    {
        return udsCmMgr.SendUDSResp(addrInfoPtr->sa, addrInfoPtr->ta, addrInfoPtr->taType,
                serviceId, err, NULL, 0);
    }
}

taf_uds_DiagIndicationHandlerRef_t taf_uds_AddDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerFunc_t  indicationHandlerPtr,   ///< [IN] Hander function.
    void*                                userPtr                 ///< [IN] User-defined pointer.
)
{
    LE_DEBUG("taf_uds_AddDiagIndicationHandler");

    void* handlerRef;
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    if(indicationHandlerPtr == NULL)
    {
        LE_ERROR("indicationHandlerPtr is Null");
        return NULL;
    }

    if(udsCmMgr.UdsAddDiagIndicationHandler() != LE_OK)
    {
        LE_ERROR("Add doip indication");
        return NULL;
    }

    // Remove previous handler reference
    if (udsCmMgr.udsIndicationHandler.safeRef != NULL &&
        le_ref_Lookup(udsCmMgr.udsHandlerRefMap, udsCmMgr.udsIndicationHandler.safeRef))
    {
        le_ref_DeleteRef(udsCmMgr.udsHandlerRefMap, udsCmMgr.udsIndicationHandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(udsCmMgr.udsHandlerRefMap, &udsCmMgr.udsIndicationHandler);
    udsCmMgr.udsIndicationHandler.funcPtr =
            (taf_doip_DiagIndicationHandlerFunc_t)indicationHandlerPtr;
    udsCmMgr.udsIndicationHandler.ctxPtr = userPtr;
    udsCmMgr.udsIndicationHandler.safeRef = handlerRef;

    return (taf_uds_DiagIndicationHandlerRef_t)handlerRef;
}

void taf_uds_RemoveDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerRef_t handerRef   ///< [IN] The handler reference.
)
{
    LE_DEBUG("taf_uds_RemoveDiagIndicationHandler");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    taf_UDSIndicationHandler_t* handlerPtr =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsCmMgr.udsHandlerRefMap, handerRef);

    if (handlerPtr != NULL)
    {
        le_ref_DeleteRef(udsCmMgr.udsHandlerRefMap, handlerPtr->safeRef);
        handlerPtr->safeRef = NULL;
    }

    return;
}

le_result_t taf_uds_Start
(
    const char* configPathPtr
)
{
    LE_DEBUG("taf_uds_Start");
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    return udsCmMgr.UdsStart(configPathPtr);
}

COMPONENT_INIT
{
    LE_INFO("UDS component init once start...");

    auto &udsCmMgr = UdsCommunicationMgr::GetInstance();
    udsCmMgr.Init();

    LE_INFO("UDS component init end...");
}
