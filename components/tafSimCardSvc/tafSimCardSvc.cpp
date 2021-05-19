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

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "tafSimCard.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;


COMPONENT_INIT
{
    LE_INFO("tafSimcard Service Init...\n");
    auto &sim = taf_sim::GetInstance();
    sim.Init();
    LE_INFO(" Sim Card service Ready...\n");

}

taf_sim_NewStateHandlerRef_t taf_sim_AddNewStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    auto &sim = taf_sim::GetInstance();
    handlerRef = (le_event_HandlerRef_t)sim.AddStateHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_sim_NewStateHandlerRef_t)(handlerRef);

}

void taf_sim_RemoveNewStateHandler(taf_sim_NewStateHandlerRef_t handlerRef)
{
    auto &sim = taf_sim::GetInstance();
    sim.RemoveStateHandler(handlerRef);
}

taf_sim_States_t taf_sim_GetState (taf_sim_Id_t slotId) {
    LE_INFO("tafSimCard getState \n");
    auto &sim = taf_sim::GetInstance();
    return (taf_sim_States_t)sim.getState(slotId);
}

bool taf_sim_IsPresent(taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_States_t state = sim.getState(slotId);
    if ((state == TAF_SIM_PRESENT) ||
            (state == TAF_SIM_READY) ||
            (state == TAF_SIM_RESTRICTED))
    {
        return true;
    } else {
        return false;
    }

}

bool taf_sim_IsReady(taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_States_t state = sim.getState(slotId);
    if (state == TAF_SIM_READY) {
        return true;
    } else {
        return false;
    }
}
