/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>

#include "tafRemoteSim.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;


COMPONENT_INIT
{
    LE_INFO("tafRemoteSim Service Init...\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.Init();
    LE_INFO(" Remote Sim Card service Ready...\n");
}

taf_simRsim_MessageHandlerRef_t taf_simRsim_AddMessageHandler
(
    taf_simRsim_MessageHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &rSim = taf_simRsim::GetInstance();
    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL");
    handlerRef = (le_event_HandlerRef_t)rSim.AddMessageHandler(handlerPtr, contextPtr);

    return (taf_simRsim_MessageHandlerRef_t)(handlerRef);

}

void taf_simRsim_RemoveMessageHandler
(
    taf_simRsim_MessageHandlerRef_t handlerRef
)
{
    auto &rSim = taf_simRsim::GetInstance();
    rSim.RemoveMessageHandler(handlerRef);
}


le_result_t taf_simRsim_SendMessage
(
    const uint8_t* msgPtr,
    size_t messageNumElements,
    taf_simRsim_CallbackHandlerFunc_t  callback,
    void* contextPtr
)
{
    auto &rSim = taf_simRsim::GetInstance();
    return rSim.SendMessage(msgPtr, messageNumElements, callback, contextPtr);
}

