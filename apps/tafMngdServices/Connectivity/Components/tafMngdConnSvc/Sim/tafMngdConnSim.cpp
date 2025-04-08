/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdConnSim.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace tafsvc;


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