/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdConnSim.hpp"

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