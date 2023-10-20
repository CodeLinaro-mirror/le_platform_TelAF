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
#include "tafDiagBackend.hpp"

using namespace telux::tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF DiagUpdate server.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagBackend& taf_DiagBackend::GetInstance()
{
    static taf_DiagBackend instance;
    return instance;
}

void taf_DiagBackend::UdsIndicationHanler
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,
    const taf_uds_DiagMsg_t* diagMsgPtr,
    le_result_t result,
    void* userPtr
)
{
    uint8_t sid;
    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();

    LE_DEBUG("Enter UdsIndicationHanler");

    if (addrInfoPtr == NULL || diagMsgPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    if (result != LE_OK)
    {
        LE_ERROR("The result of this message is incorrect.(%d)", result);
        return;
    }

    if (diagMsgPtr->dataLen == 0)
    {
        LE_ERROR("Message length%" PRIuS " is insufficient.", diagMsgPtr->dataLen);
        return;
    }

    // Get the Service Id and indicate the message to correct service.
    sid = diagMsgPtr->dataPtr[0];

    auto elem = backend.svcMap.find(sid);
    if (elem == backend.svcMap.end())
    {
        LE_ERROR("This service(%d) handler is unregistered", sid);
        return;
    }

    taf_UDSInterface* inf = elem->second;
    if (inf != NULL)
    {
        inf->UDSMsgHandler(addrInfoPtr, sid, diagMsgPtr->dataPtr, diagMsgPtr->dataLen);
    }
}

le_result_t taf_DiagBackend::RegisterUdsService
(
    uint8_t sid,
    taf_UDSInterface* service
)
{
    if (service == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    if (regstFlag == 0)
    {
        // Bring up UDS stack
        if (InitUdsStack() != LE_OK)
        {
            LE_ERROR("UDS stack can not start.");
            return LE_COMM_ERROR;
        }
        regstFlag = 1;
    }

    svcMap.insert(make_pair(sid, service));

    return LE_OK;
}

void taf_DiagBackend::UnregisterUdsService
(
    uint8_t sid
)
{
    svcMap.erase(sid);

    if (svcMap.empty())
    {
        DeInitUdsStack();
    }
}

le_result_t taf_DiagBackend::InitUdsStack
(
)
{
    le_result_t ret;

    ret = taf_uds_Start(TAF_STACK_CONFIG_PATH);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to start uds stack.(%d)", ret);
        return ret;
    }

    // Register the callback for receiving uds message.
    udsIndHandlerRef = taf_uds_AddDiagIndicationHandler(UdsIndicationHanler, NULL);
    if (udsIndHandlerRef == NULL)
    {
        LE_ERROR("Add diag message reception handler failure.");
        return LE_FAULT;
    }

    return LE_OK;
}

void taf_DiagBackend::DeInitUdsStack
(
)
{
    if (udsIndHandlerRef != NULL)
    {
        taf_uds_RemoveDiagIndicationHandler(udsIndHandlerRef);
        regstFlag = 0;
    }
}

le_result_t taf_DiagBackend::RespDiagPositive
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    const uint8_t* dataPtr,
    size_t dataLen
)
{
    if (addrInfoPtr == NULL || dataPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    taf_uds_DiagMsg_t msg;
    msg.dataPtr = (uint8_t*)dataPtr;
    msg.dataLen = dataLen;

    return taf_uds_SendDiagResp(addrInfoPtr, &msg, (taf_uds_ServiceId_t)sid, 0);
}

le_result_t taf_DiagBackend::RespDiagPositive
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr
)
{
    if (addrInfoPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    return taf_uds_SendDiagResp(addrInfoPtr, NULL, (taf_uds_ServiceId_t)sid, 0);
}

le_result_t taf_DiagBackend::RespDiagNegative
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    uint8_t nrc
)
{
    if (addrInfoPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    return taf_uds_SendDiagResp(addrInfoPtr, NULL, (taf_uds_ServiceId_t)sid, nrc);
}

void taf_DiagBackend::Init
(
    void
)
{
    if (InitUdsStack() == LE_OK)
    {
        regstFlag = 1;
    }
}
