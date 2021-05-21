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
 * @file       tafRadioSvc.cpp
 * @brief      This file provides the radio service as interfaces described
 *             in tafRadioSvc.api. The radio service will initialized
 *             automatically. Radio Power Management / Radio Configuration
 *             Preferences are provided.
 */

#include <chrono>

#include "tafRadio.hpp"

using namespace telux::tafsvc;

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     The initialization of Radio Sevice Component.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
COMPONENT_INIT
{
    LE_INFO("tafRadio Service Init...\n");
    auto &tafRadio = taf_Radio::GetInstance();
    tafRadio.Init();
    LE_INFO("tafRadio Service Ready...\n");
}

/*======================================================================

 FUNCTION        taf_radio_SetRadioPower

 DESCRIPTION     Switch of the radio power.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] le_onoff_t power: Power on/off the radio
                 [IN] uint8_t phoneId:  The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_SetRadioPower(le_onoff_t power, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(power > LE_ON, LE_BAD_PARAMETER,
        "Invalid para(power = %d)", power);

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    auto ret = tafRadio.phones[phoneId]->setRadioPower((bool)power, tafRadio.radioPowerCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRadioPower

 DESCRIPTION     Get the power status of radio.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [OUT] le_onoff_t* powerPtr:
                          LE_ON if the radio is power on.
                          LE_OFF if the radio is power off.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetRadioPower(le_onoff_t* powerPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(powerPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(powerPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    auto radioState = tafRadio.phones[phoneId]->getRadioState();
    switch(radioState) {
        case telux::tel::RadioState::RADIO_STATE_OFF:
            *powerPtr = LE_OFF;
            break;
        case telux::tel::RadioState::RADIO_STATE_UNAVAILABLE:
            LE_ERROR("Radio state is unavailable\n");
            return LE_FAULT;
        case telux::tel::RadioState::RADIO_STATE_ON:
            *powerPtr = LE_ON;
            break;
        default:
            LE_ERROR("Radio state is unknown\n");
            return LE_FAULT;
    }

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_SetAutomaticRegisterMode

 DESCRIPTION     Set radio registers to network automatically.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_SetAutomaticRegisterMode(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::AUTOMATIC;
    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(selectMode, "0", "0",
        &taf_RadioNetworkResponsecallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_SetManualRegisterMode

 DESCRIPTION     Set radio registers to network manually.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_SetManualRegisterMode(const char* mccPtr, const char* mncPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != taf_RadioFunctions::taf_radio_CheckMccMnc(mccPtr, mncPtr), LE_BAD_PARAMETER,
        "Check mcc and mnc failed");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::MANUAL;
    std::string mcc(mccPtr);
    std::string mnc(mncPtr);
    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(selectMode, mcc, mnc,
        &taf_RadioNetworkResponsecallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRegisterMode

 DESCRIPTION     Get radio register mode.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] size_t mccPtrSize:  Mobile country code length.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] size_t mncPtrSize:  Mobile network code length.
                 [OUT] bool* isManualPtr:
                           True if radio register mode is Manual.
                           False if radio register mode is Automatic.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetRegisterMode
(
    bool* isManualPtr,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_VAL((isManualPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(isManualPtr)");

    TAF_ERROR_IF_RET_VAL((mccPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL((mncPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    TAF_ERROR_IF_RET_VAL((mccPtrSize < TAF_RADIO_MCC_BYTES), LE_BAD_PARAMETER,
        "Invalid para(mccPtrSize: %d < %d)", mccPtrSize, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL((mncPtrSize < TAF_RADIO_MNC_BYTES), LE_BAD_PARAMETER,
        "Invalid para(mncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL((phoneId >= tafRadio.networkManagers.size()), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(
        networkManager->requestNetworkSelectionMode(taf_RadioSelectionModeResponseCallback::selectionModeResponse) != \
        telux::common::Status::SUCCESS, LE_FAULT, "Call sdk function failed");

    telux::common::Status status;
    // The subscription index should be +1 adapt to SDK.
    auto subscription = tafRadio.subscriptionManager->getSubscription(phoneId + 1, &status);
    TAF_ERROR_IF_RET_VAL(subscription == nullptr, LE_FAULT,
        "Invalid para(null subscription ptr, phoneId:%d)", phoneId);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    std::string mcc = std::to_string(subscription->getMcc());
    std::string mnc = std::to_string(subscription->getMnc());

    le_utf8_Copy(mccPtr, mcc.c_str(), TAF_RADIO_MCC_BYTES, NULL);
    le_utf8_Copy(mncPtr, mnc.c_str(), TAF_RADIO_MNC_BYTES, NULL);

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioSelectionModeResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *isManualPtr = taf_RadioSelectionModeResponseCallback::isRegModeMannual;

    return LE_OK;
}
