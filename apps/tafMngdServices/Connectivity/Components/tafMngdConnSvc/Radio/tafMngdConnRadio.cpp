/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdConnRadio.hpp"

using namespace tafsvc;

void tafMngdConnRadio::Init(void)
{
     LE_INFO("tafMngdConnRadio: init");
}

tafMngdConnRadio &tafMngdConnRadio::GetInstance()
{
    static tafMngdConnRadio instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start up radio.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnRadio::StartUp(uint8_t phoneId)
{
    // Step 1 : PowerOn
    if(!IsPowerOn(phoneId))
    {
        if(PowerOn(phoneId) != LE_OK)
        {
            LE_ERROR("Radio powerOn failed");
            return LE_FAULT;
        }
    }

    //Step 2 : Set Auto Register Mode
    if(!IsAutoRegMode(phoneId))
    {
        if(SetAutoRegMode(phoneId) != LE_OK)
        {
            LE_ERROR("Radio SetAutoRegMode failed");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Power on the radio.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnRadio::PowerOn(uint8_t phoneId)
{
    LE_INFO("Set radio power on");

    return taf_radio_SetRadioPower(LE_ON, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Power off the radio.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnRadio::PowerOff(uint8_t phoneId)
{
    LE_INFO("Set radio power off");

    return taf_radio_SetRadioPower(LE_OFF, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the radio is poweron.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnRadio::IsPowerOn(uint8_t phoneId)
{
    bool ret = false;
    le_result_t result;
    le_onoff_t state;

    result = taf_radio_GetRadioPower(&state, phoneId);

    if (result != LE_OK)
    {
        return ret;
    }

    switch (state)
    {
        case LE_OFF:
            ret = false;
            break;
        case LE_ON:
            ret = true;
            break;
        default:
            ret = false;
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set radio register mode to automatic.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnRadio::SetAutoRegMode(uint8_t phoneId)
{
    LE_INFO("Set radio autoRegMode");

    return taf_radio_SetAutomaticRegisterMode(phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if radio register mode is automatic.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnRadio::IsAutoRegMode(uint8_t phoneId)
{
    bool mode = true;
    le_result_t result;
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};

    result = taf_radio_GetRegisterMode(&mode, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
                                       TAF_RADIO_MNC_BYTES, phoneId);
    if (result != LE_OK)
    {
        return false;
    }
    if(mode == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the network is registered.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnRadio::IsNetworkRegistered(uint8_t phoneId)
{
    le_result_t result;
    taf_radio_NetRegState_t state;

    result = taf_radio_GetPacketSwitchedState(&state, phoneId);
    if(result != LE_OK)
    {
        LE_ERROR("taf_radio_GetPacketSwitchedState failed: %d", result);
        // Since GetPacketSwitchedState() failed, check with GetNetRegState()
        result = taf_radio_GetNetRegState(&state, phoneId);
        if(result != LE_OK)
        {
            LE_ERROR("taf_radio_GetNetRegState failed: %d", result);
            return false;
        }
    }

    if(state == TAF_RADIO_NET_REG_STATE_HOME || state == TAF_RADIO_NET_REG_STATE_ROAMING)
        return true;

    return false;
}