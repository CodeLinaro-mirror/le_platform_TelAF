/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdConnSim.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace telux::tafsvc;


void tafMngdConnSim::Init(void)
{
     LE_INFO("tafMngdConnSim: init");
}

tafMngdConnSim &tafMngdConnSim::GetInstance()
{
    static tafMngdConnSim instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for SIM state change.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnSim::SimStateHandler
(
    taf_sim_Id_t simId,
    taf_sim_States_t simState,
    void* contextPtr
)
{
    if(simId != TAF_SIM_SLOT_ID_1 && simId != TAF_SIM_SLOT_ID_2)
        return;

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};

    if(simState == TAF_SIM_READY)
        stateMachineEvt.event=MCS_EVT_SIM_READY;
    else
        stateMachineEvt.event=MCS_EVT_SIM_NOT_READY;

    stateMachineEvt.slotId = (uint8_t)simId;

    auto &mcsAdmin = tafMngdConnAdmin::GetInstance();
    le_event_Report(mcsAdmin.GetStateMachineEventId(), &stateMachineEvt,
                    sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Register SIM state change handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnSim::RegisterEvents()
{
    taf_sim_ConnectService();

    simtateHandlerRef = taf_sim_AddNewStateHandler(SimStateHandler, NULL);

    LE_INFO ("Sim Event callback is set");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if SIM is ready.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnSim::IsSimReady(uint8_t slotId)
{
    return taf_sim_IsReady((taf_sim_Id_t)slotId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Power on the SIM.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnSim::PowerOn(uint8_t slotId)
{
    LE_INFO("Set SIM power on");

    return taf_sim_SetPower((taf_sim_Id_t)slotId, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * Power off the SIM.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnSim::PowerOff(uint8_t slotId)
{
    LE_INFO("Set SIM power off");

    return taf_sim_SetPower((taf_sim_Id_t)slotId, LE_OFF);
}