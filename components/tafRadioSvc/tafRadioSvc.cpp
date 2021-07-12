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
 *             automatically. The following services are provided:
 *                 Radio Power Management
 *                 Radio Configuration Preferences
 *                 Report Network Registration Reject Indication
 *                 Radio Access Technology
 *                 Network Registration
 *                 Packet Services State
 *                 Signal Quality
 *                 Serving Cell's Location Information
 *                 Current Network Information
 *                 Network Scan
 *             Services are also integrated in cm tools.
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

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

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

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

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

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(selectMode, "0", "0",
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semNetSelModeRespCb, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

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

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(selectMode, mcc, mnc,
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semNetSelModeRespCb, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_SetManualRegisterModeAsync

 DESCRIPTION     Set radio manual register mode asynchronously.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] taf_radio_ManualSelectionHandlerFunc_t handlerPtr:
                          Handler for manual selection.
                 [IN] void* contextPtr:   Context pointer.
                 [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_SetManualRegisterModeAsync
(
    const char* mccPtr,
    const char* mncPtr,
    taf_radio_ManualSelectionHandlerFunc_t handlerPtr,
    void* contextPtr,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_NIL(LE_OK != taf_RadioFunctions::taf_radio_CheckMccMnc(mccPtr, mncPtr),
        "Check mcc and mnc failed");

    taf_RadioCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_RadioCmdReq_t));
    cmdReq.cmdType = TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL;
    cmdReq.handlerFuncPtr = (void*)handlerPtr;
    cmdReq.contextPtr = contextPtr;
    cmdReq.phoneId = phoneId;
    cmdReq.mccPtr = mccPtr;
    cmdReq.mncPtr = mncPtr;

    le_event_Report(taf_Radio::radioCmdEvId, &cmdReq, sizeof(taf_RadioCmdReq_t));
}

/*======================================================================

 FUNCTION        taf_radio_GetRegisterMode

 DESCRIPTION     Get radio register mode.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [OUT] char* mccPtr:     The mobile country code.
                 [IN] size_t mccPtrSize: Mobile country code length.
                 [OUT] char* mncPtr:     The mobile network code.
                 [IN] size_t mncPtrSize: Mobile network code length.
                 [OUT] bool* isManualPtr:
                           True if radio register mode is Manual.
                           False if radio register mode is Automatic.
                 [IN] uint8_t phoneId:   The phone id.

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
    TAF_ERROR_IF_RET_VAL(isManualPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(isManualPtr)");

    TAF_ERROR_IF_RET_VAL(mccPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL(mncPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    TAF_ERROR_IF_RET_VAL(mccPtrSize < TAF_RADIO_MCC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mccPtrSize: %d < %d)", mccPtrSize, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL(mncPtrSize < TAF_RADIO_MNC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
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

/*======================================================================

 FUNCTION        taf_radio_GetPlatformSpecificRegistrationErrorCode

 DESCRIPTION     Get the error code of setting registration mode.

 DEPENDENCIES    Configuration of registration mode.

 PARAMETERS      None

 RETURN VALUE    int32_t
                     0:     No error.
                     not 0: Some error.

 SIDE EFFECTS

======================================================================*/
int32_t taf_radio_GetPlatformSpecificRegistrationErrorCode(void)
{
    return taf_RadioNetworkResponseCallback::errCode;
}

/*======================================================================

 FUNCTION        taf_radio_AddPreferredOperator

 DESCRIPTION     Add an operator of network preference.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] taf_radio_RatBitMask_t ratMask:
                          Rat bit mask.
                 [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_DUPLICATE:     Operator already exists.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_AddPreferredOperator
(
    const char* mccPtr,
    const char* mncPtr,
    taf_radio_RatBitMask_t ratMask,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != taf_RadioFunctions::taf_radio_CheckMccMnc(mccPtr, mncPtr), LE_BAD_PARAMETER,
        "Check mcc and mnc failed");

    TAF_ERROR_IF_RET_VAL(ratMask > TAF_RADIO_RAT_BIT_MASK_ALL, LE_BAD_PARAMETER,
        "Invalid para(ratMask: %d > %d)", ratMask, TAF_RADIO_RAT_BIT_MASK_ALL);

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
        taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    telux::tel::PreferredNetworkInfo preferedOp;
    preferedOp.mcc = (uint16_t)atoi(mccPtr);
    preferedOp.mnc = (uint16_t)atoi(mncPtr);
    if (ratMask == TAF_RADIO_RAT_BIT_MASK_ALL) {
        preferedOp.ratMask.set(telux::tel::RatType::GSM);
        preferedOp.ratMask.set(telux::tel::RatType::NR5G);
        preferedOp.ratMask.set(telux::tel::RatType::LTE);
        preferedOp.ratMask.set(telux::tel::RatType::UMTS);
    } else {
        preferedOp.ratMask = (telux::tel::RatType)ratMask;
    }

    std::vector<telux::tel::PreferredNetworkInfo> preferedInfo;
    for (auto op : taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo) {
        TAF_ERROR_IF_RET_VAL(op.mcc == preferedOp.mcc && op.mnc == preferedOp.mnc && op.ratMask == preferedOp.ratMask,
            LE_DUPLICATE, "Operator(mcc:%d mnc:%d ratMask:%lx) already exist", op.mnc, op.mnc, op.ratMask.to_ulong());
            preferedInfo.push_back(op);
    }

    preferedInfo.push_back(preferedOp);

    startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->setPreferredNetworks(preferedInfo, true,
    taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");
    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semPrefNetRespCb, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    endTime = std::chrono::system_clock::now();
    elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::errorCode != telux::common::ErrorCode::SUCCESS,
        LE_FAULT, "SDK error code: %d", (int)taf_RadioNetworkResponseCallback::errorCode);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_RemovePreferredOperator

 DESCRIPTION     Remove an operator of network preference.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_NOT_FOUND:     Operator not found.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_RemovePreferredOperator
(
    const char* mccPtr,
    const char* mncPtr,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != taf_RadioFunctions::taf_radio_CheckMccMnc(mccPtr, mncPtr), LE_BAD_PARAMETER,
        "Check mcc and mnc failed");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
    taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    telux::tel::PreferredNetworkInfo preferedOp;
    preferedOp.mcc = (uint16_t)atoi(mccPtr);
    preferedOp.mnc = (uint16_t)atoi(mncPtr);

    std::vector<telux::tel::PreferredNetworkInfo> preferedInfo;
    uint32_t count = 0;

    for (auto op : taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo) {
        if (op.mcc != preferedOp.mcc) {
            preferedInfo.push_back(op);
            continue;
        }

        if (op.mnc != preferedOp.mnc) {
            preferedInfo.push_back(op);
            continue;
        }

        count++;
    }

    TAF_ERROR_IF_RET_VAL(count == 0, LE_NOT_FOUND, "Operator not found");
    LE_DEBUG("%d Operators have been found", count);

    startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->setPreferredNetworks(preferedInfo, true,
    taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semPrefNetRespCb, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    endTime = std::chrono::system_clock::now();
    elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::errorCode != telux::common::ErrorCode::SUCCESS,
        LE_FAULT, "SDK error code: %d", (int)taf_RadioNetworkResponseCallback::errorCode);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_DeletePreferredOperatorsList

 DESCRIPTION     Delete a reference of an operator list.

 DEPENDENCIES    Initialization of an operator list

 PARAMETERS      [IN] taf_radio_PreferredOperatorListRef_t preferredOperatorListRef: The operator list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_DeletePreferredOperatorsList(taf_radio_PreferredOperatorListRef_t preferredOperatorListRef)
{
    TAF_ERROR_IF_RET_VAL(preferredOperatorListRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(preferredOperatorListRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(tafRadio.prefOpListRefMap,
        preferredOperatorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    taf_RadioPrefOp_t* prefOpPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->prefOpList))) != NULL) {
        prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t, link);
        le_mem_Release(prefOpPtr);
    }

    taf_RadioPrefOpSafeRef_t* safeRefPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL) {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOpSafeRef_t, link);
        le_ref_DeleteRef(tafRadio.prefOpSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(tafRadio.prefOpListRefMap, preferredOperatorListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetPreferredOperatorsList

 DESCRIPTION     Get a reference of an operator list.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    taf_radio_PreferredOperatorListRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_PreferredOperatorListRef_t taf_radio_GetPreferredOperatorsList(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), nullptr,
        "Invalid para(phoneId:%d >= networkManagersSize%d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, nullptr,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
        taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        nullptr, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo.size() == 0, nullptr,
        "Phone%d has no prefered operators", phoneId);

    taf_RadioPrefOpList_t* prefOpsList = (taf_RadioPrefOpList_t*)le_mem_ForceAlloc(tafRadio.prefOpsListPool);
    prefOpsList->prefOpList = LE_SLS_LIST_INIT;
    prefOpsList->safeRefList = LE_SLS_LIST_INIT;
    prefOpsList->currPtr = NULL;

    taf_RadioPrefOp_t* prefOpPtr;
    for (auto info : taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo) {
        prefOpPtr = (taf_RadioPrefOp_t*)le_mem_ForceAlloc(tafRadio.prefOpPool);
        prefOpPtr->info = info;
        prefOpPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(prefOpsList->prefOpList), &(prefOpPtr->link));
    }

    return (taf_radio_PreferredOperatorListRef_t)le_ref_CreateRef(tafRadio.prefOpListRefMap, (void*)prefOpsList);
}

/*======================================================================

 FUNCTION        taf_radio_GetFirstPreferredOperator

 DESCRIPTION     Get the reference of the first operator from a list.

 DEPENDENCIES    Initialization of an operator list

 PARAMETERS      [IN] taf_radio_PreferredOperatorListRef_t preferredOperatorListRef: The operator list reference.

 RETURN VALUE    taf_radio_PreferredOperatorRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_PreferredOperatorRef_t taf_radio_GetFirstPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t preferredOperatorListRef
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(tafRadio.prefOpListRefMap,
        preferredOperatorListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", preferredOperatorListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->prefOpList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Empty list");

    taf_RadioPrefOp_t* prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t , link);
    listPtr->currPtr = linkPtr;

    taf_RadioPrefOpSafeRef_t* safeRefPtr = (taf_RadioPrefOpSafeRef_t*)le_mem_ForceAlloc(tafRadio.prefOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.prefOpSafeRefMap, (void*)prefOpPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PreferredOperatorRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_radio_GetNextPreferredOperator

 DESCRIPTION     Get the reference of the next operator from a list.

 DEPENDENCIES    Initialization of an operator list

 PARAMETERS      [IN] taf_radio_PreferredOperatorListRef_t preferredOperatorListRef: The operator list reference.

 RETURN VALUE    taf_radio_PreferredOperatorRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_PreferredOperatorRef_t taf_radio_GetNextPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t  preferredOperatorListRef
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(tafRadio.prefOpListRefMap,
        preferredOperatorListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", preferredOperatorListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->prefOpList), listPtr->currPtr);
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Reach to the end of list");

    taf_RadioPrefOp_t* prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t, link);
    listPtr->currPtr = linkPtr;

    taf_RadioPrefOpSafeRef_t* safeRefPtr = (taf_RadioPrefOpSafeRef_t*)le_mem_ForceAlloc(tafRadio.prefOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.prefOpSafeRefMap, (void*)prefOpPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_PreferredOperatorRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_radio_GetPreferredOperatorDetails

 DESCRIPTION     Get the infomation of an operator from a list.

 DEPENDENCIES    Initialization of an operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_PreferredOperatorRef_t preferredOperatorRef:
                          The operator reference.
                 [OUT] char* mccPtr:                       The mobile country code.
                 [IN] size_t mccPtrSize:                   Mobile country code length.
                 [OUT] char* mncPtr:                       The mobile network code.
                 [IN] size_t mncPtrSize:                   Mobile network code length.
                 [OUT] taf_radio_RatBitMask_t* ratMaskPtr: RAT bit mask.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetPreferredOperatorDetails
(
    taf_radio_PreferredOperatorRef_t preferredOperatorRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize,
    taf_radio_RatBitMask_t* ratMaskPtr
)
{
    TAF_ERROR_IF_RET_VAL(preferredOperatorRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(preferredOperatorRef)");

    TAF_ERROR_IF_RET_VAL(mccPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL(mccPtrSize < TAF_RADIO_MCC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mccPtrSize: %d < %d)", mccPtrSize, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL(mncPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    TAF_ERROR_IF_RET_VAL(mncPtrSize < TAF_RADIO_MNC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    TAF_ERROR_IF_RET_VAL(ratMaskPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ratMaskPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioPrefOp_t* prefOpPtr = (taf_RadioPrefOp_t*)le_ref_Lookup(tafRadio.prefOpSafeRefMap, preferredOperatorRef);
    TAF_ERROR_IF_RET_VAL(prefOpPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    std::string mcc = std::to_string(prefOpPtr->info.mcc);
    std::string mnc = std::to_string(prefOpPtr->info.mnc);

    le_utf8_Copy(mccPtr, mcc.c_str(), TAF_RADIO_MCC_BYTES, NULL);
    le_utf8_Copy(mncPtr, mnc.c_str(), TAF_RADIO_MNC_BYTES, NULL);

    *ratMaskPtr = (taf_radio_RatBitMask_t)prefOpPtr->info.ratMask.to_ulong();

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_AddNetRegRejectHandler

 DESCRIPTION     Add a handler function for network registration rejection.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] taf_radio_NetRegRejectHandlerFunc_t handlerFuncPtr:
                          The handler function.
                 [IN] void* contextPtr: Context pointer.

 RETURN VALUE    taf_radio_NetRegRejectHandlerRef_t
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_radio_NetRegRejectHandlerRef_t taf_radio_AddNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NetRegRejectHandler",
        tafRadio.netRegRejectEvId, tafRadio.FirstLayerNetRegRejectHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NetRegRejectHandlerRef_t)(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_RemoveNetRegRejectHandler

 DESCRIPTION     Remove a handler function from network registration rejection.

 DEPENDENCIES    Add a handler function for network registration rejection.

 PARAMETERS      [IN] taf_radio_NetRegRejectHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemoveNetRegRejectHandler(taf_radio_NetRegRejectHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_AddRatChangeHandler

 DESCRIPTION     Add a handler function for RAT change.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] le_mrc_RatChangeHandlerFunc_t handlerFuncPtr:
                          The handler function.
                 [IN] void* contextPtr: Context pointer.

 RETURN VALUE    taf_radio_RatChangeHandlerFunc_t
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_radio_RatChangeHandlerRef_t taf_radio_AddRatChangeHandler
(
    taf_radio_RatChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("RatChangeHandler",
        tafRadio.ratChangeEvId, taf_Radio::FirstLayerRatChangeHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_RatChangeHandlerRef_t)(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_RemoveNetRegRejectHandler

 DESCRIPTION     Remove a handler function from RAT change.

 DEPENDENCIES    Add a handler function for RAT change.

 PARAMETERS      [IN] taf_radio_RatChangeHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemoveRatChangeHandler(taf_radio_RatChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetRadioAccessTechInUse

 DESCRIPTION     Get the radio technology in use.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_Rat_t* ratPtr: The radio technology in use.
                 [IN] uint8_t phoneId:          The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetRadioAccessTechInUse(taf_radio_Rat_t* ratPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(ratPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(statePtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto voiceTechRespCbFunc = std::bind(&taf_RadioVoiceRadioTechnologyCallback::voiceRadioTechnologyResponse,
        tafRadio.voiceRadioTechCb, std::placeholders::_1, std::placeholders::_2);
    auto ret = tafRadio.phones[phoneId]->requestVoiceRadioTechnology(voiceTechRespCbFunc);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(tafRadio.voiceRadioTechCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *ratPtr = (taf_radio_Rat_t)tafRadio.voiceRadioTechCb->radioTech;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_SetRatPreferences

 DESCRIPTION     Configure rat preferences.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] taf_radio_RatPrefMask_t ratMask: Rat preference bit mask.
                 [IN] uint8_t phoneId:                 The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_SetRatPreferences(taf_radio_RatPrefMask_t ratMask, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(ratMask > TAF_RADIO_RAT_PREF_MASK_ALL, LE_BAD_PARAMETER,
        "Invalid para(ratMask: %d > %d)", ratMask, TAF_RADIO_RAT_PREF_MASK_ALL);

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.servingSystemManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.servingSystemManagers.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.servingSystemManagers[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    telux::tel::RatPreference ratPref;
    if (ratMask == TAF_RADIO_RAT_PREF_MASK_ALL) {
        ratPref.set(telux::tel::PREF_CDMA_1X);
        ratPref.set(telux::tel::PREF_CDMA_EVDO);
        ratPref.set(telux::tel::PREF_GSM);
        ratPref.set(telux::tel::PREF_WCDMA);
        ratPref.set(telux::tel::PREF_LTE);
        ratPref.set(telux::tel::PREF_TDSCDMA);
        ratPref.set(telux::tel::PREF_NR5G);
    } else {
        ratPref = (telux::tel::RatPreference)ratMask;
    }

    auto ret = tafRadio.servingSystemManagers[phoneId]->setRatPreference(ratPref,
        taf_RadioServingSystemResponseCallback::servingSystemResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRatPreferences

 DESCRIPTION     Get the rat preferences.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_RatPrefMask_t* ratMaskPtr: Rat preference bit mask.
                 [IN] uint8_t phoneId:                      The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetRatPreferences(taf_radio_RatPrefMask_t* ratMaskPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(ratMaskPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(statePtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.servingSystemManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.servingSystemManagers.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.servingSystemManagers[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.servingSystemManagers[phoneId]->requestRatPreference(
        taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioRatPreferenceResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *ratMaskPtr = (taf_radio_RatPrefMask_t)taf_RadioRatPreferenceResponseCallback::ratPref.to_ulong();

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetNetRegState

 DESCRIPTION     Get the network register state.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_NetRegState_t* statePtr: Network register state.
                 [IN] uint8_t phoneId:                    The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetNetRegState(taf_radio_NetRegState_t* statePtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(statePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(statePtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestVoiceServiceState(tafRadio.voiceSrvStateCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioVoiceServiceStateCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *statePtr = (taf_radio_NetRegState_t)taf_RadioVoiceServiceStateCallback::vocSrvState;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_AddNetRegStateEventHandler

 DESCRIPTION     Add a handler function for network registration state event.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] taf_radio_NetRegStateHandlerFunc_t handlerFuncPtr:
                          The handler function.
                 [IN] void* contextPtr: Context pointer.

 RETURN VALUE    taf_radio_RatChangeHandlerFunc_t
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_radio_NetRegStateEventHandlerRef_t taf_radio_AddNetRegStateEventHandler
(
    taf_radio_NetRegStateHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NetRegStateEventHandler",
        tafRadio.netRegStateEvId, taf_Radio::FirstLayerNetRegStateEventHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NetRegStateEventHandlerRef_t)(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_RemoveNetRegStateEventHandler

 DESCRIPTION     Remove a handler function from network registration state event.

 DEPENDENCIES    Add a handler function for network registration state event.

 PARAMETERS      [IN] taf_radio_NetRegStateEventHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemoveNetRegStateEventHandler(taf_radio_NetRegStateEventHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetPacketSwitchedState

 DESCRIPTION     Get the circuit and packet switched preference state.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_ServiceDomainState_t* statePtr:
                           Circuit switched and packet switched state.
                 [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetPacketSwitchedState(taf_radio_ServiceDomainState_t* statePtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(statePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(qualityPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.servingSystemManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.servingSystemManagers.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.servingSystemManagers[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.servingSystemManagers[phoneId]->requestServiceDomainPreference(
        taf_RadioServiceDomainResponseCallback::serviceDomainResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioServiceDomainResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *statePtr = (taf_radio_ServiceDomainState_t)taf_RadioServiceDomainResponseCallback::svcDomainPref;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_AddPacketSwitchedChangeHandler

 DESCRIPTION     Add a handler function for packet swicthed state change.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr:
                          The handler function.
                 [IN] void* contextPtr: Context pointer.

 RETURN VALUE    taf_radio_PacketSwitchedChangeHandlerRef_t
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_radio_PacketSwitchedChangeHandlerRef_t taf_radio_AddPacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("PacketSwChangeHandler",
        tafRadio.packetSwChangeEvId, taf_Radio::FirstLayerPacketSwChangeHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_PacketSwitchedChangeHandlerRef_t)(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_RemovePacketSwitchedChangeHandler

 DESCRIPTION     Remove a handler function from packet switched state.

 DEPENDENCIES    Add a handler function for packet switched state.

 PARAMETERS      [IN] taf_radio_PacketSwitchedChangeHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemovePacketSwitchedChangeHandler(taf_radio_PacketSwitchedChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetSignalQual

 DESCRIPTION     Get the signal quality, according to the signal strength level
                 from different rat.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] uint32_t* qualityPtr: The signal quality.
                 [IN] uint8_t phoneId:       The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetSignalQual(uint32_t* qualityPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(qualityPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(qualityPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestSignalStrength(tafRadio.signalStrengthCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(tafRadio.signalStrengthCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->signalStrengthLevel + 1;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_MeasureSignalMetrics

 DESCRIPTION     Get the reference of measure signal metrics.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    taf_radio_MetricsRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_MetricsRef_t taf_radio_MeasureSignalMetrics(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), nullptr,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, nullptr,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, nullptr,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    taf_RadioCellMetrics_t* cellMetricsPtr = (taf_RadioCellMetrics_t*)le_mem_ForceAlloc(tafRadio.cellMetricsPool);
    memcpy(cellMetricsPtr, &taf_RadioCellInfoCallback::cellMetrics, sizeof(taf_RadioCellMetrics_t));

    return (taf_radio_MetricsRef_t)le_ref_CreateRef(tafRadio.metricsRefMap, (void*)cellMetricsPtr);
}

/*======================================================================

 FUNCTION        taf_radio_DeleteSignalMetrics

 DESCRIPTION     Delete a reference of signal metrics.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.

 RETURN VALUE    le_result_t
                     LE_NOT_FOUND: Fail.
                     LE_OK:        Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_DeleteSignalMetrics(taf_radio_MetricsRef_t metricsRef)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(metricsRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_ref_DeleteRef(tafRadio.metricsRefMap, metricsRef);
    le_mem_Release(cellMetricsPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRatOfSignalMetrics

 DESCRIPTION     Get the cell rat bitmask with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.

 RETURN VALUE    taf_radio_CellRatMask_t

 SIDE EFFECTS

======================================================================*/
taf_radio_CellRatMask_t taf_radio_GetRatOfSignalMetrics(taf_radio_MetricsRef_t metricsRef)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, TAF_RADIO_CELL_RAT_MASK_UNKNOWN,
        "Null reference(metricsRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, TAF_RADIO_CELL_RAT_MASK_UNKNOWN, "Invalid para(null reference ptr)");

    return cellMetricsPtr->cellRatMask;
}

/*======================================================================

 FUNCTION        taf_radio_GetGsmSignalMetrics

 DESCRIPTION     Get the GSM signal metircs with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.
                 [OUT] int32_t* rssiPtr:                 The signal strength.
                 [OUT] uint32_t* berPtr:                 The bit error rate.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Not found with map reference.
                     LE_UNAVAILABLE:   Not available.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetGsmSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef,
    int32_t* rssiPtr,
    uint32_t* berPtr
)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(metricsRef)");

    TAF_ERROR_IF_RET_VAL(rssiPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(rssiPtr)");

    TAF_ERROR_IF_RET_VAL(berPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(berPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(!(cellMetricsPtr->cellRatMask & TAF_RADIO_CELL_RAT_MASK_GSM),
        LE_UNAVAILABLE, "GSM unavailable");

    *rssiPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].signalStrength.dbm;
    *berPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].signalStrength.ber;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetUmtsSignalMetrics

 DESCRIPTION     Get the UMTS signal metircs with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.
                 [OUT] int32_t* ssPtr:
                           The signal strength, only applicatable for WCDMA.
                 [OUT] uint32_t* berPtr:
                           The bit error rate, only applicatable for WCDMA.
                 [OUT] int32_t* rscpPtr:
                           The reference signal code power in dBm, only applicatable for TD-SCDMA.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Not found with map reference.
                     LE_UNAVAILABLE:   Not available.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetUmtsSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef,
    int32_t* ssPtr,
    uint32_t* berPtr,
    int32_t* rscpPtr
)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(metricsRef)");

    TAF_ERROR_IF_RET_VAL(ssPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ssPtr)");

    TAF_ERROR_IF_RET_VAL(berPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(berPtr)");

    TAF_ERROR_IF_RET_VAL(rscpPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(rscpPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    if (cellMetricsPtr->cellRatMask & TAF_RADIO_CELL_RAT_MASK_WCDMA) {
        *ssPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA].signalStrength.strength;
        *berPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA].signalStrength.ber;
    } else if (cellMetricsPtr->cellRatMask & TAF_RADIO_CELL_RAT_MASK_TDSCDMA) {
        *rscpPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_TDSCDMA].signalStrength.tdscdma.rscp;
    } else {
        LE_ERROR("UMTS unavailable");
        return LE_UNAVAILABLE;
    }

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetLteSignalMetrics

 DESCRIPTION     Get the LTE signal metircs with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.
                 [OUT] int32_t* ssPtr:                   The signal strength.
                 [OUT] int32_t* rsrqPtr:                 The reference signal receive quality in dB.
                 [OUT] int32_t* rsrpPtr:                 The reference signal receive power in dBm.
                 [OUT] int32_t* snrPtr:                  The signal-to-noise ratio in dB.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Not found with map reference.
                     LE_UNAVAILABLE:   Not available.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetLteSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef,
    int32_t* ssPtr,
    int32_t* rsrqPtr,
    int32_t* rsrpPtr,
    int32_t* snrPtr
)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(metricsRef)");

    TAF_ERROR_IF_RET_VAL(ssPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ssPtr)");

    TAF_ERROR_IF_RET_VAL(rsrqPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(rsrqPtr)");

    TAF_ERROR_IF_RET_VAL(rsrpPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(rsrpPtr)");

    TAF_ERROR_IF_RET_VAL(snrPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(snrPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(!(cellMetricsPtr->cellRatMask & TAF_RADIO_CELL_RAT_MASK_LTE),
        LE_UNAVAILABLE, "LTE unavailable");

    *ssPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].signalStrength.strength;
    *rsrqPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].signalStrength.lte.rsrq;
    *rsrpPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].signalStrength.dbm;
    *snrPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].signalStrength.snr;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetCdmaSignalMetrics

 DESCRIPTION     Get the CDMA signal metircs with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.
                 [OUT] int32_t* ssPtr:                   The signal strength in dBm.
                 [OUT] int32_t* ecioPtr:                 The CDMA Ec/Io in dB.
                 [OUT] int32_t* snrPtr:                  The signal-to-noise ratio in dB.
                 [OUT] int32_t* ioPtr:                   The EVDO Ec/Io in dB.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Not found with map reference.
                     LE_UNAVAILABLE:   Not available.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetCdmaSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef,
    int32_t* ssPtr,
    int32_t* ecioPtr,
    int32_t* snrPtr,
    int32_t* ioPtr
)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(metricsRef)");

    TAF_ERROR_IF_RET_VAL(ssPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ssPtr)");

    TAF_ERROR_IF_RET_VAL(ecioPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ecioPtr)");

    TAF_ERROR_IF_RET_VAL(snrPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(snrPtr)");

    TAF_ERROR_IF_RET_VAL(ioPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ioPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioCellMetrics_t* cellMetricsPtr =
        (taf_RadioCellMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(cellMetricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(!(cellMetricsPtr->cellRatMask & TAF_RADIO_CELL_RAT_MASK_CDMA),
        LE_UNAVAILABLE, "CDMA unavailable");

    *ssPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_CDMA].signalStrength.dbm;
    *ecioPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_CDMA].signalStrength.cdma.cdmaEcio;
    *snrPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_CDMA].signalStrength.snr;
    *ioPtr = cellMetricsPtr->signalMetrics[TAF_RADIO_CELL_INFO_TYPE_CDMA].signalStrength.cdma.evdoEcio;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_AddSignalStrengthChangeHandler

 DESCRIPTION     Add a handler function for signal strength change.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] taf_radio_CellRatMask_t ratMask:
                          Cell rat bitmask.
                 [IN] taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr:
                          The handler function.
                 [IN] void* contextPtr: Context pointer.

 RETURN VALUE    taf_radio_SignalStrengthChangeHandlerRef_t
                     non-nullptr: Success
                     nullptr: Fail

 SIDE EFFECTS

======================================================================*/
taf_radio_SignalStrengthChangeHandlerRef_t taf_radio_AddSignalStrengthChangeHandler
(
    taf_radio_CellRatMask_t ratMask,
    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &tafRadio = taf_Radio::GetInstance();
    switch (ratMask) {
        case TAF_RADIO_CELL_RAT_MASK_GSM:
            handlerRef = le_event_AddLayeredHandler("GsmSsChangeHandler", tafRadio.gsmSsChangeEvId,
                taf_Radio::FirstLayerSsChangeHandler, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_CELL_RAT_MASK_CDMA:
            handlerRef = le_event_AddLayeredHandler("CdmaSsChangeHandler", tafRadio.cdmaSsChangeEvId,
                taf_Radio::FirstLayerSsChangeHandler, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_CELL_RAT_MASK_LTE:
            handlerRef = le_event_AddLayeredHandler("LteSsChangeHandler", tafRadio.lteSsChangeEvId,
                taf_Radio::FirstLayerSsChangeHandler, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_CELL_RAT_MASK_WCDMA:
            handlerRef = le_event_AddLayeredHandler("WcdmaSsChangeHandler", tafRadio.wcdmaSsChangeEvId,
                taf_Radio::FirstLayerSsChangeHandler, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_CELL_RAT_MASK_TDSCDMA:
            handlerRef = le_event_AddLayeredHandler("TdscdmaSsChangeHandler", tafRadio.tdscdmaSsChangeEvId,
                taf_Radio::FirstLayerSsChangeHandler, (void*)handlerFuncPtr);
            break;
        default:
            LE_ERROR("Invalid para(ratMask: 0x%x)", ratMask);
            return nullptr;
    }

    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_radio_SignalStrengthChangeHandlerRef_t)(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_RemoveSignalStrengthChangeHandler

 DESCRIPTION     Remove a handler function from signal strength change.

 DEPENDENCIES    Add a handler function for signal strength change.

 PARAMETERS      [IN] taf_radio_SignalStrengthChangeHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemoveSignalStrengthChangeHandler(taf_radio_SignalStrengthChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellId

 DESCRIPTION     Get the cell identity.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint32_t
                     UINT32_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint32_t taf_radio_GetServingCellId(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    taf_radio_CellRatMask_t ratMask = taf_RadioCellInfoCallback::cellMetrics.cellRatMask;
    uint32_t cid = UINT32_MAX;
    uint8_t match = 0;

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_GSM) {
        cid = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].cellId.cid;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_LTE) {
        cid = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].cellId.cid;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_WCDMA) {
        cid = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA].cellId.cid;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_TDSCDMA) {
        cid = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_TDSCDMA].cellId.cid;
        match++;
    }

    TAF_ERROR_IF_RET_VAL(match != 1, UINT32_MAX,
        "Invalid para(ratMask:0x%x match:%d)", ratMask, match);

    return cid;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellLocAreaCode

 DESCRIPTION     Get the location area code.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint32_t
                     UINT32_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint32_t taf_radio_GetServingCellLocAreaCode(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    taf_radio_CellRatMask_t ratMask = taf_RadioCellInfoCallback::cellMetrics.cellRatMask;
    uint32_t lac = UINT32_MAX;
    uint8_t match = 0;

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_GSM) {
        lac = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].cellId.lac;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_WCDMA) {
        lac = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA].cellId.lac;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_TDSCDMA) {
        lac = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_TDSCDMA].cellId.lac;
        match++;
    }

    TAF_ERROR_IF_RET_VAL(match != 1, UINT32_MAX,
        "Invalid para(ratMask:0x%x match:%d)", ratMask, match);

    return lac;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellLteTracAreaCode

 DESCRIPTION     Get the tracking area code.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint16_t
                     UINT16_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint16_t taf_radio_GetServingCellLteTracAreaCode(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(!(taf_RadioCellInfoCallback::cellMetrics.cellRatMask & TAF_RADIO_CELL_RAT_MASK_LTE), UINT16_MAX,
        "Invalid para(cellRatMask:0x%x)", taf_RadioCellInfoCallback::cellMetrics.cellRatMask);

    return (uint16_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].cellId.lte.tac;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellEarfcn

 DESCRIPTION     Get the E-UTRA Absolute Radio Frequency Channel Number.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint16_t
                     UINT16_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint32_t taf_radio_GetServingCellEarfcn(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(!(taf_RadioCellInfoCallback::cellMetrics.cellRatMask & TAF_RADIO_CELL_RAT_MASK_LTE), UINT32_MAX,
        "Invalid para(cellRatMask:0x%x)", taf_RadioCellInfoCallback::cellMetrics.cellRatMask);

    return (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].cellId.arfcn;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellTimingAdvance

 DESCRIPTION     Get the timing advance.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint32_t
                     UINT32_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint32_t taf_radio_GetServingCellTimingAdvance(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    taf_radio_CellRatMask_t ratMask = taf_RadioCellInfoCallback::cellMetrics.cellRatMask;
    uint32_t ta = UINT32_MAX;
    uint8_t match = 0;

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_GSM) {
        ta = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].signalStrength.ta;
        match++;
    }

    if (ratMask & TAF_RADIO_CELL_RAT_MASK_LTE) {
        ta = (uint32_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].signalStrength.ta;
        match++;
    }

    TAF_ERROR_IF_RET_VAL(match != 1, UINT32_MAX,
        "Invalid para(ratMask:0x%x match:%d)", ratMask, match);

    return ta;
}

/*======================================================================

 FUNCTION        taf_radio_GetPhysicalServingLteCellId

 DESCRIPTION     Get the physical cell identifier.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint16_t
                     UINT16_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint16_t taf_radio_GetPhysicalServingLteCellId(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(!(taf_RadioCellInfoCallback::cellMetrics.cellRatMask & TAF_RADIO_CELL_RAT_MASK_LTE), UINT16_MAX,
        "Invalid para(cellRatMask:0x%x)", taf_RadioCellInfoCallback::cellMetrics.cellRatMask);

    return (uint16_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE].cellId.lte.pid;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellGsmBsic

 DESCRIPTION     Get the base station identity code.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] uint8_t* bsicPtr: The base station identity code.
                 [IN] uint8_t phoneId:   The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetServingCellGsmBsic(uint8_t* bsicPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(bsicPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(bsicPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, LE_BAD_PARAMETER,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(!(taf_RadioCellInfoCallback::cellMetrics.cellRatMask & TAF_RADIO_CELL_RAT_MASK_GSM), LE_BAD_PARAMETER,
        "Invalid para(cellRatMask:0x%x)", taf_RadioCellInfoCallback::cellMetrics.cellRatMask);

    *bsicPtr = (uint8_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM].cellId.gsm.bsic;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellScramblingCode

 DESCRIPTION     Get the primary scrambling code.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    uint16_t
                     UINT16_MAX: Invalid parameters.
                     others:     Success.

 SIDE EFFECTS

======================================================================*/
uint16_t taf_radio_GetServingCellScramblingCode(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.phones.size());

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    auto ret = tafRadio.phones[phoneId]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(!(taf_RadioCellInfoCallback::cellMetrics.cellRatMask & TAF_RADIO_CELL_RAT_MASK_WCDMA), UINT16_MAX,
        "Invalid para(cellRatMask:0x%x)", taf_RadioCellInfoCallback::cellMetrics.cellRatMask);

    return (uint16_t)taf_RadioCellInfoCallback::cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA].cellId.wcdma.psc;
}

/*======================================================================

 FUNCTION        taf_radio_GetCurrentNetworkName

 DESCRIPTION     Get current network name.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] char* nameStr:     Current network name.
                 [IN] size_t nameStrSize: The network name length.
                 [IN] uint8_t phoneId:    The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetCurrentNetworkName(char* nameStr, size_t nameStrSize, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(nameStr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(nameStr)");

    TAF_ERROR_IF_RET_VAL(nameStrSize > TAF_RADIO_NETWORK_NAME_MAX_LEN, LE_BAD_PARAMETER,
        "Invalid para(nameStrSize: %d > %d)", nameStrSize, TAF_RADIO_NETWORK_NAME_MAX_LEN);

    auto &tafRadio = taf_Radio::GetInstance();

    telux::common::Status status;
    // The subscription index should be +1 adapt to SDK.
    auto subscription = tafRadio.subscriptionManager->getSubscription(phoneId + 1, &status);
    TAF_ERROR_IF_RET_VAL(subscription == nullptr, LE_FAULT,
        "Invalid para(null subscription ptr, phoneId:%d)", phoneId);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_utf8_Copy(nameStr, subscription->getCarrierName().c_str(), nameStrSize, NULL);

    return LE_OK;
}


/*======================================================================

 FUNCTION        taf_radio_GetCurrentNetworkMccMnc

 DESCRIPTION     Get current network mobile country code and mobile network code.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] char* mccStr:            The mobile country code.
                 [IN] size_t mccStrNumElements: Mobile country code length.
                 [OUT] char* mncStr:            The mobile network code.
                 [IN] size_t mncStrNumElements: Mobile network code length.
                 [IN] uint8_t phoneId:          The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetCurrentNetworkMccMnc
(
    char* mccStr,
    size_t mccStrNumElements,
    char* mncStr,
    size_t mncStrNumElements,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_VAL(mccStr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mccStr)");

    TAF_ERROR_IF_RET_VAL(mccStrNumElements < TAF_RADIO_MCC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mccStrNumElements: %d < %d)", mccStrNumElements, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL(mncStr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mncStr)");

    TAF_ERROR_IF_RET_VAL(mncStrNumElements < TAF_RADIO_MNC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mncStrNumElements: %d < %d)", mncStrNumElements, TAF_RADIO_MNC_BYTES);

    auto &tafRadio = taf_Radio::GetInstance();

    telux::common::Status status;
    // The subscription index should be +1 adapt to SDK.
    auto subscription = tafRadio.subscriptionManager->getSubscription(phoneId + 1, &status);
    TAF_ERROR_IF_RET_VAL(subscription == nullptr, LE_FAULT,
        "Invalid para(null subscription ptr, phoneId:%d)", phoneId);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    std::string mcc = std::to_string(subscription->getMcc());
    std::string mnc = std::to_string(subscription->getMnc());

    le_utf8_Copy(mccStr, mcc.c_str(), TAF_RADIO_MCC_BYTES, NULL);
    le_utf8_Copy(mncStr, mnc.c_str(), TAF_RADIO_MNC_BYTES, NULL);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_PerformCellularNetworkScan

 DESCRIPTION     Get a reference of a cellular network scan list.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    taf_radio_ScanInformationListRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_ScanInformationListRef_t taf_radio_PerformCellularNetworkScan(uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(phoneId >= tafRadio.networkManagers.size(), nullptr,
        "Invalid para(phoneId:%d >= networkManagersSize%d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, nullptr,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    TAF_ERROR_IF_RET_VAL(networkManager->performNetworkScan(
        taf_RadioPerformNetworkScanCallback::performNetworkScanResponse) != telux::common::Status::SUCCESS,
        nullptr, "Call sdk function failed");

    le_clk_Time_t timeToWait = {TAF_RADIO_SCAN_INTERVAL, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr,
        "Wait semaphore timeout\n");

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    TAF_ERROR_IF_RET_VAL(taf_RadioPerformNetworkScanCallback::opInfos.size() == 0, nullptr,
        "Phone%d has no operators after scanning", phoneId);

    taf_RadioScanOpList_t* opsList = (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);
    opsList->scanOpList = LE_SLS_LIST_INIT;
    opsList->safeRefList = LE_SLS_LIST_INIT;
    opsList->currPtr = NULL;
    opsList->num = taf_RadioPerformNetworkScanCallback::opInfos.size();

    taf_RadioScanOp_t* opPtr;
    for (auto info : taf_RadioPerformNetworkScanCallback::opInfos) {
        opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
        le_utf8_Copy(opPtr->name, info.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
        le_utf8_Copy(opPtr->mcc, info.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
        le_utf8_Copy(opPtr->mnc, info.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
        opPtr->status.inUse = info.getStatus().inUse;
        opPtr->status.roaming = info.getStatus().roaming;
        opPtr->status.forbidden = info.getStatus().forbidden;
        opPtr->status.preferred = info.getStatus().preferred;
        opPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(opsList->scanOpList), &(opPtr->link));
    }

    return (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(tafRadio.scanOpListRefMap, (void*)opsList);
}

/*======================================================================

 FUNCTION        taf_radio_PerformCellularNetworkScanAsync

 DESCRIPTION     Perform network scan asynchronously.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] taf_radio_CellularNetworkScanHandlerFunc_t handlerPtr:
                          Handler for network scan.
                 [IN] void* contextPtr:   Context pointer.
                 [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_PerformCellularNetworkScanAsync
(
    taf_radio_CellularNetworkScanHandlerFunc_t handlerPtr,
    void* contextPtr,
    uint8_t phoneId
)
{
    taf_RadioCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_RadioCmdReq_t));
    cmdReq.cmdType = TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN;
    cmdReq.handlerFuncPtr = (void*)handlerPtr;
    cmdReq.contextPtr = contextPtr;
    cmdReq.phoneId = phoneId;

    le_event_Report(taf_Radio::radioCmdEvId, &cmdReq, sizeof(taf_RadioCmdReq_t));
}

/*======================================================================

 FUNCTION        taf_radio_GetFirstCellularNetworkScan

 DESCRIPTION     Get the reference of the first operator from a scan list.

 DEPENDENCIES    Initialization of a scan operator list

 PARAMETERS      [IN] taf_radio_ScanInformationListRef_t scanInformationListRef: The scan operator list reference.

 RETURN VALUE    taf_radio_ScanInformationRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_ScanInformationRef_t taf_radio_GetFirstCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t scanInformationListRef
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOpList_t* listPtr = (taf_RadioScanOpList_t*)le_ref_Lookup(tafRadio.scanOpListRefMap,
        scanInformationListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", scanInformationListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->scanOpList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Empty list");

    taf_RadioScanOp_t* opPtr = CONTAINER_OF(linkPtr, taf_RadioScanOp_t, link);
    listPtr->currPtr = linkPtr;

    taf_RadioScanOpSafeRef_t* safeRefPtr = (taf_RadioScanOpSafeRef_t*)le_mem_ForceAlloc(tafRadio.scanOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.scanOpSafeRefMap, (void*)opPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_ScanInformationRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_radio_GetNextCellularNetworkScan

 DESCRIPTION     Get the reference of the next operator from a scan list.

 DEPENDENCIES    Initialization of a scan operator list

 PARAMETERS      [IN] taf_radio_ScanInformationListRef_t scanInformationListRef: The scan operator list reference.

 RETURN VALUE    taf_radio_ScanInformationRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_radio_ScanInformationRef_t taf_radio_GetNextCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t  scanInformationListRef
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOpList_t* listPtr = (taf_RadioScanOpList_t*)le_ref_Lookup(tafRadio.scanOpListRefMap,
        scanInformationListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", scanInformationListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->scanOpList), listPtr->currPtr);
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Reach to the end of list");

    taf_RadioScanOp_t* opPtr = CONTAINER_OF(linkPtr, taf_RadioScanOp_t, link);
    listPtr->currPtr = linkPtr;

    taf_RadioScanOpSafeRef_t* safeRefPtr = (taf_RadioScanOpSafeRef_t*)le_mem_ForceAlloc(tafRadio.scanOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.scanOpSafeRefMap, (void*)opPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_ScanInformationRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_radio_GetCellularNetworkMccMnc

 DESCRIPTION     Get the mobile country code and mobile network code of an operator from a scan list.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.
                 [OUT] char* mccPtr:     The mobile country code.
                 [IN] size_t mccPtrSize: Mobile country code length.
                 [OUT] char* mncPtr:     The mobile network code.
                 [IN] size_t mncPtrSize: Mobile network code length.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetCellularNetworkMccMnc
(
    taf_radio_ScanInformationRef_t scanInformationRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize
)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(scanInformationRef)");

    TAF_ERROR_IF_RET_VAL(mccPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL(mccPtrSize < TAF_RADIO_MCC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mccPtrSize: %d < %d)", mccPtrSize, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL(mncPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    TAF_ERROR_IF_RET_VAL(mncPtrSize < TAF_RADIO_MNC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_utf8_Copy(mccPtr, opPtr->mcc, mccPtrSize, NULL);
    le_utf8_Copy(mncPtr, opPtr->mnc, mncPtrSize, NULL);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetCellularNetworkName

 DESCRIPTION     Get the name of an operator from a scan list.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.
                 [OUT] char* namePtr:  The network name.
                 [IN] size_t nameSize: Network name length.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetCellularNetworkName
(
    taf_radio_ScanInformationRef_t scanInformationRef,
    char* namePtr,
    size_t nameSize
)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(scanInformationRef)");

    TAF_ERROR_IF_RET_VAL(namePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(namePtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_utf8_Copy(namePtr, opPtr->name, nameSize, NULL);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_IsCellularNetworkInUse

 DESCRIPTION     Check if cellular network is in use.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.

 RETURN VALUE    bool
                     ture:  In use.
                     false: Not in use.

 SIDE EFFECTS

======================================================================*/
bool taf_radio_IsCellularNetworkInUse(taf_radio_ScanInformationRef_t scanInformationRef)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, false,
        "Null reference(scanInformationRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, false, "Invalid para(null reference ptr)");

    if (opPtr->status.inUse == telux::tel::InUseStatus::CURRENT_SERVING) {
        return true;
    }

    return false;
}

/*======================================================================

 FUNCTION        taf_radio_IsCellularNetworkAvailable

 DESCRIPTION     Check if cellular network is available.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.

 RETURN VALUE    bool
                     ture:  Available.
                     false: Not available.

 SIDE EFFECTS

======================================================================*/
bool taf_radio_IsCellularNetworkAvailable(taf_radio_ScanInformationRef_t scanInformationRef)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, false,
        "Null reference(scanInformationRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, false, "Invalid para(null reference ptr)");

    if (opPtr->status.inUse == telux::tel::InUseStatus::AVAILABLE) {
        return true;
    }

    return false;
}

/*======================================================================

 FUNCTION        taf_radio_IsCellularNetworkHome

 DESCRIPTION     Check if cellular network is home.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.

 RETURN VALUE    bool
                     ture:  Home.
                     false: Roaming or unknown.

 SIDE EFFECTS

======================================================================*/
bool taf_radio_IsCellularNetworkHome(taf_radio_ScanInformationRef_t scanInformationRef)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, false,
        "Null reference(scanInformationRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, false, "Invalid para(null reference ptr)");

    if (opPtr->status.roaming == telux::tel::RoamingStatus::HOME) {
        return true;
    }

    return false;
}

/*======================================================================

 FUNCTION        taf_radio_IsCellularNetworkForbidden

 DESCRIPTION     Check if cellular network is forbidden.

 DEPENDENCIES    Initialization of a scan operator list and get a safe reference of an operator.

 PARAMETERS      [IN] taf_radio_ScanInformationRef_t scanInformationRef:
                          The operator reference.

 RETURN VALUE    bool
                     ture:  Forbidden.
                     false: Not forbidden.

 SIDE EFFECTS

======================================================================*/
bool taf_radio_IsCellularNetworkForbidden(taf_radio_ScanInformationRef_t scanInformationRef)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, false,
        "Null reference(scanInformationRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap, scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, false, "Invalid para(null reference ptr)");

    if (opPtr->status.forbidden == telux::tel::ForbiddenStatus::FORBIDDEN) {
        return true;
    }

    return false;
}

/*======================================================================

 FUNCTION        le_mrc_DeleteCellularNetworkScan

 DESCRIPTION     Delete a reference of a scan operator list.

 DEPENDENCIES    Initialization of a scan operator list

 PARAMETERS      [IN] taf_radio_ScanInformationListRef_t scanInformationListRef: The scan operator list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_DeleteCellularNetworkScan(taf_radio_ScanInformationListRef_t scanInformationListRef)
{
    TAF_ERROR_IF_RET_VAL(scanInformationListRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(scanInformationListRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOpList_t* listPtr = (taf_RadioScanOpList_t*)le_ref_Lookup(tafRadio.scanOpListRefMap,
        scanInformationListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    taf_RadioScanOp_t* opPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->scanOpList))) != NULL) {
        opPtr = CONTAINER_OF(linkPtr, taf_RadioScanOp_t, link);
        le_mem_Release(opPtr);
    }

    taf_RadioScanOpSafeRef_t* safeRefPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL) {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_RadioScanOpSafeRef_t, link);
        le_ref_DeleteRef(tafRadio.scanOpSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(tafRadio.scanOpListRefMap, scanInformationListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}
