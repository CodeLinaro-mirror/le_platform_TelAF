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

#include "tafMngdConnRadio.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace telux::tafsvc;

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
 * Handler for GSM signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::GsmSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone ID %d GSM rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for UMTS signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::UmtsSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone ID %d UMTS rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::CdmaSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone ID %d CDMA rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for TDSCDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::TdscdmaSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone ID %d TDSCDMA rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for LTE signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::LteSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone %d LTE rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::Nr5gSsChangeHandler
(
    int32_t ss,
    int32_t rsrp,
    uint8_t phoneId,
    void* contextPtr
)
{
    LE_DEBUG("Phone %d NR5G rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Packet switch state.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::PackSwStateHandler
(
    taf_radio_NetRegStateInd_t* packSwStateIndPtr, ///< [IN] Packet switched state indication.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    if(packSwStateIndPtr == NULL)
        return;

    LE_INFO("phone: %d packet switch state: %d", packSwStateIndPtr->phoneId,
        packSwStateIndPtr->state);
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();

    stateMachineEvent_t stateMachineEvt = {TAF_MNGD_CONN_EVT_INIT, 0};

    if(packSwStateIndPtr->state == TAF_RADIO_NET_REG_STATE_HOME ||
       packSwStateIndPtr->state == TAF_RADIO_NET_REG_STATE_ROAMING)
    {
        stateMachineEvt.event=TAF_MNGD_CONN_EVT_NETWORK_REG_STATE;
    }
    else
    {
        stateMachineEvt.event=TAF_MNGD_CONN_EVT_NETWORK_UNREG_STATE;
    }

    stateMachineEvt.phoneId = packSwStateIndPtr->phoneId;

    le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                                                    sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Register radio event handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnRadio::RegisterEvents()
{
    taf_radio_ConnectService();

    gsmSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_GSM,
                            (taf_radio_SignalStrengthChangeHandlerFunc_t)GsmSsChangeHandler, NULL);

    if(gsmSsChangeHandlerRef == NULL)
        LE_ERROR("Adding GSM signal strength change handler failed");

    umtsSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_UMTS,
                            (taf_radio_SignalStrengthChangeHandlerFunc_t)UmtsSsChangeHandler, NULL);

    if(umtsSsChangeHandlerRef == NULL)
        LE_ERROR("Adding UMTS signal strength change handler failed");

    cdmaSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_CDMA,
                            (taf_radio_SignalStrengthChangeHandlerFunc_t)CdmaSsChangeHandler, NULL);

    if(cdmaSsChangeHandlerRef == NULL)
        LE_ERROR("Adding CDMA signal strength change handler failed");

    tdscdmaSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_TDSCDMA,
                         (taf_radio_SignalStrengthChangeHandlerFunc_t)TdscdmaSsChangeHandler, NULL);

    if(tdscdmaSsChangeHandlerRef == NULL)
        LE_ERROR("Adding TDSCDMA signal strength change handler failed");

    lteSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
                            (taf_radio_SignalStrengthChangeHandlerFunc_t)LteSsChangeHandler, NULL);

    if(lteSsChangeHandlerRef == NULL)
        LE_ERROR("Adding LTE signal strength change handler failed");

    nr5gSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
                            (taf_radio_SignalStrengthChangeHandlerFunc_t)Nr5gSsChangeHandler, NULL);

    if(nr5gSsChangeHandlerRef == NULL)
        LE_ERROR("Adding NR5G signal strength change handler failed");

    packSwStateHandlerRef = taf_radio_AddPacketSwitchedChangeHandler(
                            (taf_radio_PacketSwitchedChangeHandlerFunc_t)PackSwStateHandler, NULL);

    if(packSwStateHandlerRef == NULL)
        LE_ERROR("Adding packet switched change handler failed");

    LE_INFO ("Radio Event callback is set");
    return;
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
    LE_INFO("Set radio poweron");

    return taf_radio_SetRadioPower(LE_ON, phoneId);
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
 * Set ALL rat for radio.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnRadio::SetAllRat(uint8_t phoneId)
{
    le_result_t result;

    result= taf_radio_SetRatPreferences(TAF_RADIO_RAT_BIT_MASK_ALL, phoneId);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if all RAT is set.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnRadio::IsAllRatSet(uint8_t phoneId)
{
    le_result_t result;
    taf_radio_RatBitMask_t ratMask = 0x0;

    result = taf_radio_GetRatPreferences(&ratMask, phoneId);
    if (result != LE_OK)
    {
        return false;
    }
    if(ratMask == (TAF_RADIO_RAT_BIT_MASK_GSM | TAF_RADIO_RAT_BIT_MASK_UMTS |
                   TAF_RADIO_RAT_BIT_MASK_CDMA | TAF_RADIO_RAT_BIT_MASK_TDSCDMA |
                   TAF_RADIO_RAT_BIT_MASK_LTE | TAF_RADIO_RAT_BIT_MASK_NR5G))
    {
        LE_INFO("All mask is set");
        return true;
    }
    else
    {
        LE_INFO("Not all mask is set");
        return false;
    }
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
        LE_ERROR("Get network reg info failed");
        return false;
    }

    if(state == TAF_RADIO_NET_REG_STATE_HOME || state == TAF_RADIO_NET_REG_STATE_ROAMING)
        return true;

    return false;
}