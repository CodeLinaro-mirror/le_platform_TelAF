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
 *             Preferences are provided. These services are also integrated
 *             in cm tools.
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
    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(selectMode, "0", "0",
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
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
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    return LE_OK;
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
    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    endTime = std::chrono::system_clock::now();
    elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

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

    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res,
        "Wait semaphore timeout\n");

    endTime = std::chrono::system_clock::now();
    elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_DeletePreferredOperatorsList

 DESCRIPTION     Delete a reference of an operator list.

 DEPENDENCIES    Initialization of an operator list

 PARAMETERS      [IN] taf_radio_PreferredOperatorListRef_t preferredOperatorListRef: The operator list reference.

 RETURN VALUE    le_result_t
                     LE_FAULT: Fail.
                     LE_OK:    Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_DeletePreferredOperatorsList(taf_radio_PreferredOperatorListRef_t preferredOperatorListRef)
{
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(taf_Radio::prefOpListRefMap,
        preferredOperatorListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_FAULT, "Invalid para(null reference ptr)");

    taf_RadioPrefOp_t* prefOpPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->prefOpList))) != NULL)
    {
        prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t, link);
        le_mem_Release(prefOpPtr);
    }

    taf_RadioPrefOpSafeRef_t* safeRefPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOpSafeRef_t, link);
        le_ref_DeleteRef(taf_Radio::prefOpSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(taf_Radio::prefOpListRefMap, preferredOperatorListRef);

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

    taf_RadioPrefOpList_t* prefOpsList = (taf_RadioPrefOpList_t*)le_mem_ForceAlloc(taf_Radio::prefOpsListPool);
    prefOpsList->prefOpList = LE_SLS_LIST_INIT;
    prefOpsList->safeRefList = LE_SLS_LIST_INIT;
    prefOpsList->currPtr = NULL;

    taf_RadioPrefOp_t* prefOpPtr;
    for (auto info : taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo) {
        prefOpPtr = (taf_RadioPrefOp_t*)le_mem_ForceAlloc(taf_Radio::prefOpPool);
        prefOpPtr->info = info;
        prefOpPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(prefOpsList->prefOpList), &(prefOpPtr->link));
    }

    return (taf_radio_PreferredOperatorListRef_t)le_ref_CreateRef(taf_Radio::prefOpListRefMap, (void*)prefOpsList);
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
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(taf_Radio::prefOpListRefMap,
        preferredOperatorListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference(%p)", preferredOperatorListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->prefOpList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Empty list");

    taf_RadioPrefOp_t* prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t , link);
    listPtr->currPtr = linkPtr;

    taf_RadioPrefOpSafeRef_t* safeRefPtr = (taf_RadioPrefOpSafeRef_t*)le_mem_ForceAlloc(taf_Radio::prefOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(taf_Radio::prefOpSafeRefMap, (void*)prefOpPtr);
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
    taf_RadioPrefOpList_t* listPtr = (taf_RadioPrefOpList_t*)le_ref_Lookup(taf_Radio::prefOpListRefMap,
        preferredOperatorListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference(%p)", preferredOperatorListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->prefOpList), listPtr->currPtr);
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Reach to the end of list");

    taf_RadioPrefOp_t* prefOpPtr = CONTAINER_OF(linkPtr, taf_RadioPrefOp_t, link);
    listPtr->currPtr = linkPtr;

    taf_RadioPrefOpSafeRef_t* safeRefPtr = (taf_RadioPrefOpSafeRef_t*)le_mem_ForceAlloc(taf_Radio::prefOpSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(taf_Radio::prefOpSafeRefMap, (void*)prefOpPtr);
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
        "Invalid para(ncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    TAF_ERROR_IF_RET_VAL(ratMaskPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(ratMaskPtr)");

    telux::tel::PreferredNetworkInfo* prefOpInfoPtr =
        (telux::tel::PreferredNetworkInfo*)le_ref_Lookup(taf_Radio::prefOpSafeRefMap, preferredOperatorRef);

    std::string mcc = std::to_string(prefOpInfoPtr->mcc);
    std::string mnc = std::to_string(prefOpInfoPtr->mnc);

    le_utf8_Copy(mccPtr, mcc.c_str(), TAF_RADIO_MCC_BYTES, NULL);
    le_utf8_Copy(mncPtr, mnc.c_str(), TAF_RADIO_MNC_BYTES, NULL);

    *ratMaskPtr = (taf_radio_RatBitMask_t)prefOpInfoPtr->ratMask.to_ulong();

    return LE_OK;
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

