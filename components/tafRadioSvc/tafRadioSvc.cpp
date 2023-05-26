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
#include "taf_pa_radio.hpp"

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
    LE_INFO("tafRadio Service Init...");
    auto &tafRadio = taf_Radio::GetInstance();
    tafRadio.Init();
    LE_INFO("tafRadio Service Ready...");
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

    telux::tel::OperatingMode mode =
        power == LE_ON ? telux::tel::OperatingMode::ONLINE : telux::tel::OperatingMode::AIRPLANE;

    auto respenseCb = std::bind(&taf_RadioSetOperatingModeCallback::setOperatingModeResponse,
        tafRadio.setOperatingModeCb, std::placeholders::_1);

    auto ret = tafRadio.phoneManager->setOperatingMode(mode, respenseCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(tafRadio.setOperatingModeCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout.");

    TAF_ERROR_IF_RET_VAL(tafRadio.setOperatingModeCb->result != LE_OK,
        tafRadio.setOperatingModeCb->result, "Fail to set radio power.");

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
    TAF_ERROR_IF_RET_VAL(powerPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(powerPtr)");

    auto &tafRadio = taf_Radio::GetInstance();

    auto ret = tafRadio.phoneManager->requestOperatingMode(tafRadio.getOperatingModeCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(tafRadio.getOperatingModeCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(tafRadio.setOperatingModeCb->result != LE_OK,
        tafRadio.setOperatingModeCb->result, "Fail to get radio power.");

    if (tafRadio.getOperatingModeCb->opMode == telux::tel::OperatingMode::ONLINE)
    {
        *powerPtr = LE_ON;
    }
    else
    {
        *powerPtr = LE_OFF;
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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(
        telux::tel::NetworkSelectionMode::AUTOMATIC, "0", "0",
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) !=
        telux::common::Status::SUCCESS, LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::selModeSem, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::selModeRes != LE_OK,
        taf_RadioNetworkResponseCallback::selModeRes, "Fail to set automatic register mode.");

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
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    std::string mcc(mccPtr);
    std::string mnc(mncPtr);

    TAF_ERROR_IF_RET_VAL(networkManager->setNetworkSelectionMode(
        telux::tel::NetworkSelectionMode::MANUAL, mcc, mnc,
        &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) !=
        telux::common::Status::SUCCESS, LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::selModeSem, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::selModeRes != LE_OK,
        taf_RadioNetworkResponseCallback::selModeRes, "Fail to set manual register mode.");

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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetRegisterMode(isManualPtr, mccPtr, mccPtrSize,
        mncPtr, mncPtrSize, phoneId);
}

/*======================================================================

 FUNCTION        taf_radio_GetPlatformSpecificRegistrationErrorCode

 DESCRIPTION     Get the error code of setting registration mode.

 DEPENDENCIES    Configuration of registration mode.

 PARAMETERS      None

 RETURN VALUE    int32_t: Refer to enum taf_radio_NetRejCause_t

 SIDE EFFECTS

======================================================================*/
int32_t taf_radio_GetPlatformSpecificRegistrationErrorCode(void)
{
    return taf_pa_radio_GetPlatformSpecificRegistrationErrorCode();
}

/*======================================================================

 FUNCTION        taf_radio_AddPreferredOperator

 DESCRIPTION     Add an operator of network preference.

 DEPENDENCIES    Initialization of Radio Service

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.
                 [IN] taf_radio_RatBitMask_t ratMask:
                          Rat bit mask. only support GSM/UMTS/LTE/NR5G
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
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
        taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioPreferredNetworksResponseCallback::result != LE_OK,
        taf_RadioPreferredNetworksResponseCallback::result, "Fail to get preferred operator.");

    telux::tel::PreferredNetworkInfo preferedOp;
    preferedOp.mcc = (uint16_t)atoi(mccPtr);
    preferedOp.mnc = (uint16_t)atoi(mncPtr);
    if (ratMask == TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        preferedOp.ratMask.set(telux::tel::RatType::GSM);
        preferedOp.ratMask.set(telux::tel::RatType::NR5G);
        preferedOp.ratMask.set(telux::tel::RatType::LTE);
        preferedOp.ratMask.set(telux::tel::RatType::UMTS);
    } 
    else
    {
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_GSM)
        {
            preferedOp.ratMask.set(telux::tel::RatType::GSM);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
        {
            preferedOp.ratMask.set(telux::tel::RatType::UMTS);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_LTE)
        {
            preferedOp.ratMask.set(telux::tel::RatType::LTE);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
        {
            preferedOp.ratMask.set(telux::tel::RatType::NR5G);
        }
    }

    std::vector<telux::tel::PreferredNetworkInfo> preferedInfo;
    for (auto op : taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo) {
        TAF_ERROR_IF_RET_VAL(op.mcc == preferedOp.mcc && op.mnc == preferedOp.mnc && op.ratMask == preferedOp.ratMask,
            LE_DUPLICATE, "Operator(mcc:%d mnc:%d ratMask:%lx) already exist", op.mnc, op.mnc, op.ratMask.to_ulong());
            preferedInfo.push_back(op);
    }

    preferedInfo.push_back(preferedOp);

    TAF_ERROR_IF_RET_VAL(networkManager->setPreferredNetworks(preferedInfo, true,
    taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::prefNetSem, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::prefNetRes != LE_OK,
        taf_RadioNetworkResponseCallback::prefNetRes, "Fail to add preferred operator.");

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
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, LE_FAULT,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
    taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioPreferredNetworksResponseCallback::result != LE_OK,
        taf_RadioPreferredNetworksResponseCallback::result, "Fail to get preferred operator.");

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

    TAF_ERROR_IF_RET_VAL(networkManager->setPreferredNetworks(preferedInfo, true,
    taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb) != telux::common::Status::SUCCESS,
        LE_FAULT, "Call sdk function failed");

    res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::prefNetSem, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioNetworkResponseCallback::prefNetRes != LE_OK,
        taf_RadioNetworkResponseCallback::prefNetRes, "Fail to remove preferred operator.");

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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), NULL,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == nullptr, nullptr,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(networkManager->requestPreferredNetworks(
        taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse) != telux::common::Status::SUCCESS,
        nullptr, "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPreferredNetworksResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr,
        "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioPreferredNetworksResponseCallback::result != LE_OK,
        NULL, "Fail to get preferred operator.");

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

    taf_radio_RatBitMask_t ratMask = 0x0;
    if (prefOpPtr->info.ratMask[telux::tel::RatType::GSM])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_GSM;
    }
    if (prefOpPtr->info.ratMask[telux::tel::RatType::UMTS])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_UMTS;
    }
    if (prefOpPtr->info.ratMask[telux::tel::RatType::LTE])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_LTE;
    }
    if (prefOpPtr->info.ratMask[telux::tel::RatType::NR5G])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_NR5G;
    }

    *ratMaskPtr = ratMask;

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
    return taf_pa_radio_AddNetRegRejectHandler(handlerFuncPtr, contextPtr);
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
    taf_pa_radio_RemoveNetRegRejectHandler(handlerRef);
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
    return taf_pa_radio_AddRatChangeHandler(handlerFuncPtr, contextPtr);
}

/*======================================================================

 FUNCTION        taf_radio_RemoveRatChangeHandler

 DESCRIPTION     Remove a handler function from RAT change.

 DEPENDENCIES    Add a handler function for RAT change.

 PARAMETERS      [IN] taf_radio_RatChangeHandlerRef_t handlerRef:
                          The handler reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_radio_RemoveRatChangeHandler(taf_radio_RatChangeHandlerRef_t handlerRef)
{
    taf_pa_radio_RemoveRatChangeHandler(handlerRef);
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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetRadioAccessTechInUse(ratPtr, phoneId);
}

/*======================================================================

 FUNCTION        taf_radio_SetRatPreferences

 DESCRIPTION     Configure rat preferences.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [IN] taf_radio_RatBitMask_t ratMask: Rat preference bit mask.
                 [IN] uint8_t phoneId:                 The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_SetRatPreferences(taf_radio_RatBitMask_t ratMask, uint8_t phoneId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.servingSystemManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.servingSystemManagers[phoneId - 1] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    telux::tel::RatPreference ratPref;
    if (ratMask == TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        ratPref.set(telux::tel::PREF_CDMA_1X);
        ratPref.set(telux::tel::PREF_CDMA_EVDO);
        ratPref.set(telux::tel::PREF_GSM);
        ratPref.set(telux::tel::PREF_WCDMA);
        ratPref.set(telux::tel::PREF_LTE);
        ratPref.set(telux::tel::PREF_TDSCDMA);
        ratPref.set(telux::tel::PREF_NR5G);
    }
    else
    {
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_GSM)
        {
            ratPref = ratPref.set(telux::tel::PREF_GSM);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
        {
            ratPref = ratPref.set(telux::tel::PREF_WCDMA);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_CDMA)
        {
            ratPref.set(telux::tel::PREF_CDMA_1X);
            ratPref.set(telux::tel::PREF_CDMA_EVDO);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
        {
            ratPref.set(telux::tel::PREF_TDSCDMA);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_LTE)
        {
            ratPref.set(telux::tel::PREF_LTE);
        }
        if (ratMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
        {
            ratPref.set(telux::tel::PREF_NR5G);
        }
    }

    auto ret = tafRadio.servingSystemManagers[phoneId - 1]->setRatPreference(ratPref,
        taf_RadioServingSystemResponseCallback::servingSystemResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioServingSystemResponseCallback::semaphore,
        timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioServingSystemResponseCallback::result != LE_OK,
        taf_RadioServingSystemResponseCallback::result, "Fail to set rat preference.");

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRatPreferences

 DESCRIPTION     Get the rat preferences.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_RatBitMask_t* ratMaskPtr: Rat preference bit mask.
                 [IN] uint8_t phoneId:                      The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetRatPreferences(taf_radio_RatBitMask_t* ratMaskPtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(ratMaskPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(statePtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.servingSystemManagers.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.servingSystemManagers[phoneId - 1] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.servingSystemManagers[phoneId - 1]->requestRatPreference(
        taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioRatPreferenceResponseCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioRatPreferenceResponseCallback::result != LE_OK,
        taf_RadioRatPreferenceResponseCallback::result, "Fail to get rat preference.");

    taf_radio_RatBitMask_t ratMask = 0x0;
    telux::tel::RatPreference pref = taf_RadioRatPreferenceResponseCallback::ratPref;

    if (pref[telux::tel::PREF_GSM])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_GSM;
    }

    if (pref[telux::tel::PREF_CDMA_1X] || pref[telux::tel::PREF_CDMA_EVDO])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_CDMA;
    }

    if (pref[telux::tel::PREF_WCDMA])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_UMTS;
    }

    if (pref[telux::tel::PREF_TDSCDMA])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_TDSCDMA;
    }

    if (pref[telux::tel::PREF_LTE])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_LTE;
    }

    if (pref[telux::tel::PREF_NR5G])
    {
        ratMask |= TAF_RADIO_RAT_BIT_MASK_NR5G;
    }

    *ratMaskPtr = ratMask;

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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetNetRegState(statePtr, phoneId);
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
    return taf_pa_radio_AddNetRegStateEventHandler(handlerFuncPtr, contextPtr);
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
    taf_pa_radio_RemoveNetRegStateEventHandler(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetPacketSwitchedState

 DESCRIPTION     Get the circuit and packet switched preference state.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] taf_radio_NetRegState_t* statePtr: Packet switched state.
                 [IN] uint8_t phoneId: The phone id.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_radio_GetPacketSwitchedState(taf_radio_NetRegState_t* statePtr, uint8_t phoneId)
{
    TAF_ERROR_IF_RET_VAL(statePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(statePtr)");

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetPacketSwitchedState(statePtr, phoneId);
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
    return taf_pa_radio_AddPacketSwitchedChangeHandler(handlerFuncPtr, contextPtr);
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
    taf_pa_radio_RemovePacketSwitchedChangeHandler(handlerRef);
}

/*======================================================================

 FUNCTION        taf_radio_GetSignalQual

 DESCRIPTION     Get the signal quality, according to the signal strength level
                 from different rat.

 DEPENDENCIES    Initialization of the radio service.

 PARAMETERS      [OUT] uint32_t* qualityPtr: The signal quality.
                                             Valid from 1 to 5, 0 is unknown.
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    taf_radio_Rat_t rat = TAF_RADIO_RAT_UNKNOWN;
    le_result_t res = taf_pa_radio_GetRadioAccessTechInUse(&rat, phoneId);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Fail to get RAT in use.");

    auto ret = tafRadio.phones[phoneId - 1]->requestSignalStrength(tafRadio.signalStrengthCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    res = le_sem_WaitWithTimeOut(tafRadio.signalStrengthCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(tafRadio.signalStrengthCb->result != LE_OK,
        tafRadio.signalStrengthCb->result, "Fail to get signal quality.");

    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
            *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->ssMetrics.gsm.sslv;
            break;
        case TAF_RADIO_RAT_UMTS:
            *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->ssMetrics.umts.sslv;
            break;
        case TAF_RADIO_RAT_CDMA:
            *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->ssMetrics.cdma.sslv;
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            return LE_UNAVAILABLE;
        case TAF_RADIO_RAT_LTE:
            *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->ssMetrics.lte.sslv;
            break;
        case TAF_RADIO_RAT_NR5G:
            *qualityPtr = (uint32_t)tafRadio.signalStrengthCb->ssMetrics.nr5g.sslv;
            break;
        default:
            LE_ERROR("Invalid RAT(%d).", rat);
            return LE_FAULT;
    }

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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), nullptr,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, nullptr,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestSignalStrength(tafRadio.signalStrengthCb);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, nullptr,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(tafRadio.signalStrengthCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(tafRadio.signalStrengthCb->result != LE_OK,
        NULL, "Fail to get signal metrics.");

    taf_RadioSignalMetrics_t* metricsPtr = (taf_RadioSignalMetrics_t*)le_mem_ForceAlloc(tafRadio.metricsPool);
    memcpy(metricsPtr, &tafRadio.signalStrengthCb->ssMetrics, sizeof(taf_RadioSignalMetrics_t));
    metricsPtr->phoneId = phoneId;

    return (taf_radio_MetricsRef_t)le_ref_CreateRef(tafRadio.metricsRefMap, (void*)metricsPtr);
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
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_ref_DeleteRef(tafRadio.metricsRefMap, metricsRef);
    le_mem_Release(metricsPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetRatOfSignalMetrics

 DESCRIPTION     Get the cell rat bitmask with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.

 RETURN VALUE    taf_radio_RatBitMask_t

 SIDE EFFECTS

======================================================================*/
taf_radio_RatBitMask_t taf_radio_GetRatOfSignalMetrics(taf_radio_MetricsRef_t metricsRef)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == nullptr, 0x0, "Null reference(metricsRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, 0x0, "Invalid para(null reference ptr)");

    return metricsPtr->ratMask;
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
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *rssiPtr = metricsPtr->gsm.ss;
    *berPtr = metricsPtr->gsm.ber;

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
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    uint8_t phoneId = metricsPtr->phoneId;
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    if (metricsPtr->ratMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
    {
        *ssPtr = metricsPtr->umts.ss;
        *berPtr = metricsPtr->umts.ber;
    }
    else if (metricsPtr->ratMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
    {
        *rscpPtr = metricsPtr->tdscdma.rscp;
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
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *ssPtr = metricsPtr->lte.ss;
    *rsrqPtr = metricsPtr->lte.rsrq;
    *rsrpPtr = metricsPtr->lte.rsrp;
    *snrPtr = metricsPtr->lte.snr;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_GetCdmaSignalMetrics

 DESCRIPTION     Get the CDMA signal metircs with a metrics reference.

 DEPENDENCIES    Initialization of signal metrics.

 PARAMETERS      [IN] taf_radio_MetricsRef_t metricsRef: The signal metrics reference.
                 [OUT] int32_t* ssPtr:                   The signal strength in dBm.
                 [OUT] int32_t* ecioPtr:                 The CDMA Ec/Io in dB.
                 [OUT] int32_t* snrPtr:                  The EVDO signal-to-noise ratio in dB.
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
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *ssPtr = metricsPtr->cdma.ss;
    *ecioPtr = metricsPtr->cdma.ecio;
    *snrPtr = metricsPtr->cdma.snr;
    *ioPtr = metricsPtr->cdma.io;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_radio_AddSignalStrengthChangeHandler

 DESCRIPTION     Add a handler function for signal strength change.

 DEPENDENCIES    Initialization of Radio Service.

 PARAMETERS      [IN] taf_radio_Rat_t rat:
                          Radio Access Technology.
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
    taf_radio_Rat_t rat,
    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return taf_pa_radio_AddSignalStrengthChangeHandler(rat, handlerFuncPtr, contextPtr);
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
    taf_pa_radio_RemoveSignalStrengthChangeHandler(handlerRef);
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT32_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT32_MAX,
        "No serving cell.");

    switch (taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat)
    {
        case TAF_RADIO_RAT_GSM:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->gsm.cid;
        case TAF_RADIO_RAT_UMTS:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->umts.cid;
        case TAF_RADIO_RAT_TDSCDMA:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->tdscdma.cid;
        case TAF_RADIO_RAT_LTE:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->lte.cid;
        default:
            LE_ERROR("Invalid RAT(%d)", taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat);
    }

    return UINT32_MAX;
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT32_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT32_MAX,
        "No serving cell.");

    switch (taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat)
    {
        case TAF_RADIO_RAT_GSM:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->gsm.lac;
        case TAF_RADIO_RAT_UMTS:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->umts.lac;
        case TAF_RADIO_RAT_TDSCDMA:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->tdscdma.lac;
        default:
            LE_ERROR("Invalid RAT(%d)", taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat);
    }

    return UINT32_MAX;
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT16_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT16_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_LTE, UINT16_MAX, "Serving cell is not LTE.");

    return (uint16_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->lte.tac;
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT32_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT32_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_LTE, UINT32_MAX, "Serving cell is not LTE.");

    return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->lte.earfcn;
}

/*======================================================================

 FUNCTION        taf_radio_GetServingCellTimingAdvance

 DESCRIPTION     Get the timing advance, only support GSM/LTE

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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT32_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT32_MAX,
        "No serving cell.");

    switch (taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat)
    {
        case TAF_RADIO_RAT_GSM:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->gsm.ta;
        case TAF_RADIO_RAT_LTE:
            return (uint32_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->lte.ta;
        default:
            LE_ERROR("Invalid RAT(%d)", taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat);
    }

    return UINT32_MAX;
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT16_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT16_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_LTE, UINT16_MAX, "Serving cell is not LTE.");

    return (uint16_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->lte.pcid;
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, LE_FAULT,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, LE_FAULT,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        taf_RadioCellInfoCallback::result, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), LE_FAULT,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_GSM, LE_FAULT, "Serving cell is not GSM.");

    *bsicPtr = (uint8_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->gsm.bsic;

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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT16_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT16_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT16_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT16_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::result != LE_OK,
        UINT16_MAX, "Fail to get cell information.");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT16_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_UMTS, UINT16_MAX, "Serving cell is not UMTS.");

    return (uint16_t)taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->umts.psc;
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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetCurrentNetworkName(nameStr, nameStrSize, phoneId);
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

    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_GetCurrentNetworkMccMnc(mccStr, mccStrNumElements, mncStr,
        mncStrNumElements, phoneId);
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
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.networkManagers.size(), NULL,
        "Invalid para(phoneId:%d)", phoneId);

    auto networkManager = tafRadio.networkManagers[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkManager == NULL, NULL,
        "Invalid para(null network manager ptr, phoneId:%d)", phoneId);

    auto networkListener = tafRadio.networkListeners[phoneId - 1];
    TAF_ERROR_IF_RET_VAL(networkListener == NULL, NULL,
        "Invalid para(null network listener ptr, phoneId:%d)", phoneId);
    networkListener->opInfos.clear();

    auto status = networkManager->registerListener(networkListener);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, NULL,
        "Fail to register listener with phoneId:%d)", phoneId);

    telux::tel::NetworkScanInfo info;
    info.scanType = telux::tel::NetworkScanType::ALL_RATS;
    if (networkManager->performNetworkScan(info,
        taf_RadioPerformNetworkScanCallback::performNetworkScanResponse) !=
            telux::common::Status::SUCCESS)
    {
        LE_ERROR("Call sdk function failed");
        status = networkManager->deregisterListener(networkListener);
        TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, NULL,
            "Fail to deregister listener with phoneId:%d)", phoneId);
    };

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore,
        timeToWait);
    if (res != LE_OK || taf_RadioPerformNetworkScanCallback::result != LE_OK)
    {
        LE_ERROR("Perform network scan failed.");
        status = networkManager->deregisterListener(networkListener);
        TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, NULL,
            "Fail to deregister listener with phoneId:%d)", phoneId);
    }

    le_clk_Time_t timeToScan = {TAF_RADIO_SCAN_INTERVAL, 0};
    res = le_sem_WaitWithTimeOut(networkListener->semaphore, timeToScan);
    if (res != LE_OK)
    {
        LE_ERROR("Wait semaphore timeout");
        status = networkManager->deregisterListener(networkListener);
        TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, NULL,
            "Fail to deregister listener with phoneId:%d)", phoneId);
    };

    status = networkManager->deregisterListener(networkListener);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, NULL,
        "Fail to deregister listener with phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(networkListener->opInfos.size() == 0, NULL,
        "Phone%d has no operators after scanning", phoneId);

    taf_RadioScanOpList_t* opsList = (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);
    opsList->scanOpList = LE_SLS_LIST_INIT;
    opsList->safeRefList = LE_SLS_LIST_INIT;
    opsList->currPtr = NULL;

    taf_RadioScanOp_t* opPtr;
    for (auto info : networkListener->opInfos) {
        opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
        le_utf8_Copy(opPtr->name, info.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
        le_utf8_Copy(opPtr->mcc, info.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
        le_utf8_Copy(opPtr->mnc, info.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
        opPtr->status.inUse = info.getStatus().inUse;
        opPtr->status.roaming = info.getStatus().roaming;
        opPtr->status.forbidden = info.getStatus().forbidden;
        opPtr->status.preferred = info.getStatus().preferred;
        opPtr->rat = info.getRat();
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
    TAF_ERROR_IF_RET_NIL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM,
        "Invalid para(phoneId:%d)", phoneId);

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

 FUNCTION        taf_radio_DeleteCellularNetworkScan

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

//--------------------------------------------------------------------------------------------------
/**
 * Set signal strength indication thresholds.
 *
 * @return
 *  - LE_BAD_PARAMETER Invalid parameters.
 *  - LE_OK            On success.
 *  - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndThresholds
(
    taf_radio_SigType_t sigType, ///< [IN] Signal type.
    int32_t lowerRangeThreshold, ///< [IN] Lower range threshold in 0.1 dBm.
    int32_t upperRangeThreshold, ///< [IN] Upper range threshold in 0.1 dBm.
    uint8_t phoneId              ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_SetSignalStrengthIndThresholds(sigType, lowerRangeThreshold,
        upperRangeThreshold, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set signal strength indication delta.
 *
 * @return
 *  - LE_BAD_PARAMETER Invalid parameters.
 *  - LE_OK            On success.
 *  - LE_FAULT         On failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndDelta
(
    taf_radio_SigType_t sigType, ///< [IN] Signal type.
    uint16_t delta,              ///< [IN] Signal delta.
    uint8_t phoneId              ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_SetSignalStrengthIndDelta(sigType, delta, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get NR Cell ID.
 *
 * @return
 *  - INT64_MAX Internal error.
 *  - Others     NR Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetServingNrCellId
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT64_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT64_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(
        taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT64_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT64_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT64_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_NR5G, LE_FAULT, "Serving cell is not NR5G.");

    return taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->nr5g.cid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get NR Tracking Area Code.
 *
 * @return
 *  - INT32_MAX Internal error.
 *  - Others    NR Tracking Area Code.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrTracAreaCode
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), INT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, INT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(
        taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, INT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, INT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), INT32_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_NR5G, INT32_MAX, "Serving cell is not NR5G.");

    return taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->nr5g.tac;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get NR Absolute RF Channel Number.
 *
 * @return
 *  - INT32_MAX Internal error.
 *  - -1 Unknown.
 *  - Others    NR Absolute RF Channel Number.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrArfcn
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), INT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, INT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(
        taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, INT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, INT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), INT32_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_NR5G, INT32_MAX, "Serving cell is not NR5G.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->nr5g.arfcn == -1,
        INT32_MAX, "Arfcn is unknown.");

    return taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->nr5g.arfcn;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get NR5G Physical Cell ID.
 *
 * @return
 *  - UINT32_MAX Internal error.
 *  - Others Physical Cell ID..
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalServingNrCellId
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), UINT32_MAX,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == nullptr, UINT32_MAX,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(
        taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, UINT32_MAX,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, UINT32_MAX, "Wait semaphore timeout");

    TAF_ERROR_IF_RET_VAL(!taf_RadioCellInfoCallback::cellListInfo.servingCell.size(), UINT32_MAX,
        "No serving cell.");

    TAF_ERROR_IF_RET_VAL(taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->rat !=
        TAF_RADIO_RAT_NR5G, UINT32_MAX, "Serving cell is not NR5G.");

    return taf_RadioCellInfoCallback::cellListInfo.servingCell[0]->nr5g.pcid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get NR5G signal metrics
 *
 * @return
 *  - LE_BAD_PARAMETER Invalid parameters.
 *  - LE_OK            On success.
 *  - LE_NOT_FOUND     Reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNr5gSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] Signal metrics reference.
    int32_t* rsrqPtr,                  ///< [OUT] Reference Signal Received Quality.
    int32_t* rsrpPtr,                  ///< [OUT] Reference Signal Received Power.
    int32_t* snrPtr                    ///< [OUT] Signal to Noise Ratio.
)
{
    TAF_ERROR_IF_RET_VAL(metricsRef == NULL, LE_BAD_PARAMETER, "Null reference(metricsRef)");

    TAF_ERROR_IF_RET_VAL(rsrqPtr == NULL, LE_BAD_PARAMETER, "Null ptr(rsrqPtr)");

    TAF_ERROR_IF_RET_VAL(rsrpPtr == NULL, LE_BAD_PARAMETER, "Null ptr(rsrpPtr)");

    TAF_ERROR_IF_RET_VAL(snrPtr == NULL, LE_BAD_PARAMETER, "Null ptr(snrPtr)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioSignalMetrics_t* metricsPtr =
        (taf_RadioSignalMetrics_t*)le_ref_Lookup(tafRadio.metricsRefMap, metricsRef);
    TAF_ERROR_IF_RET_VAL(metricsPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *rsrqPtr = metricsPtr->nr5g.rsrq;
    *rsrpPtr = metricsPtr->nr5g.rsrp;
    *snrPtr = metricsPtr->nr5g.snr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Neighbor Cells Information
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others Neighbor cells reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NeighborCellsRef_t taf_radio_GetNeighborCellsInfo
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > tafRadio.phones.size(), NULL,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(tafRadio.phones[phoneId - 1] == NULL, NULL,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    auto ret = tafRadio.phones[phoneId - 1]->requestCellInfo(
        taf_RadioCellInfoCallback::cellInfoListResponse);
    TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS, NULL,
        "Call sdk function failed");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(taf_RadioCellInfoCallback::semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout");

    taf_RadioNgbrCells_t* ngbrCellsPtr =
        (taf_RadioNgbrCells_t*)le_mem_ForceAlloc(tafRadio.ngbrCellsPool);
    ngbrCellsPtr->cellInfoList = LE_SLS_LIST_INIT;
    ngbrCellsPtr->safeRefList = LE_SLS_LIST_INIT;
    ngbrCellsPtr->currPtr = NULL;

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr;
    for (auto cell : taf_RadioCellInfoCallback::cellListInfo.neighborCell)
    {
        ngbrCellInfoPtr = (taf_RadioNgbrCellInfo_t*)le_mem_ForceAlloc(tafRadio.ngbrCellInfoPool);
        ngbrCellInfoPtr->cell.rat = cell->rat;
        ngbrCellInfoPtr->cell.ss = cell->ss;
        switch (cell->rat)
        {
            case TAF_RADIO_RAT_GSM:
                ngbrCellInfoPtr->cell.gsm = cell->gsm;
                break;
            case TAF_RADIO_RAT_CDMA:
                ngbrCellInfoPtr->cell.cdma = cell->cdma;
                break;
            case TAF_RADIO_RAT_UMTS:
                ngbrCellInfoPtr->cell.umts = cell->umts;
                break;
            case TAF_RADIO_RAT_TDSCDMA:
                ngbrCellInfoPtr->cell.tdscdma = cell->tdscdma;
                break;
            case TAF_RADIO_RAT_LTE:
                ngbrCellInfoPtr->cell.lte = cell->lte;
                break;
            case TAF_RADIO_RAT_NR5G:
                ngbrCellInfoPtr->cell.nr5g = cell->nr5g;
                break;
            default:
                break;
        }
        ngbrCellInfoPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(ngbrCellsPtr->cellInfoList), &(ngbrCellInfoPtr->link));
    }

    return (taf_radio_NeighborCellsRef_t)le_ref_CreateRef(tafRadio.ngbrCellsRefMap,
        (void*)ngbrCellsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete neighbor cells information
 *
 * @return
 *  - LE_NOT_FOUND     Reference not found.
 *  - LE_OK            On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteNeighborCellsInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef ///< [IN] Neighbor cells reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCells_t* ngbrCellsPtr =
        (taf_RadioNgbrCells_t*)le_ref_Lookup(tafRadio.ngbrCellsRefMap, ngbrCellsRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellsPtr == NULL, LE_NOT_FOUND, "Invalid para(null ptr)");

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(ngbrCellsPtr->cellInfoList))) != NULL)
    {
        ngbrCellInfoPtr = CONTAINER_OF(linkPtr, taf_RadioNgbrCellInfo_t, link);
        le_mem_Release(ngbrCellInfoPtr);
    }

    taf_RadioNgbrCellInfoSafeRef_t* safeRefPtr;
    while ((linkPtr = le_sls_Pop(&(ngbrCellsPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_RadioNgbrCellInfoSafeRef_t, link);
        le_ref_DeleteRef(tafRadio.ngbrCellInfoSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(tafRadio.ngbrCellsRefMap, ngbrCellsRef);

    le_mem_Release(ngbrCellsPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first neighbor cell information
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others Neighbor cell information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetFirstNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef ///< [IN] Neighbor cells reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCells_t* ngbrCellsPtr =
        (taf_RadioNgbrCells_t*)le_ref_Lookup(tafRadio.ngbrCellsRefMap, ngbrCellsRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellsPtr == NULL, NULL, "Invalid para(null ptr)");

    le_sls_Link_t* linkPtr = le_sls_Peek(&(ngbrCellsPtr->cellInfoList));
    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr = CONTAINER_OF(linkPtr, taf_RadioNgbrCellInfo_t, link);
    ngbrCellsPtr->currPtr = linkPtr;

    taf_RadioNgbrCellInfoSafeRef_t* safeRefPtr =
       (taf_RadioNgbrCellInfoSafeRef_t*)le_mem_ForceAlloc(tafRadio.ngbrCellInfoSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.ngbrCellInfoSafeRefMap, (void*)ngbrCellInfoPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(ngbrCellsPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_CellInfoRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next neighbor cell information
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others Neighbor cell information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetNextNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef ///< [IN] Neighbor cells reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCells_t* ngbrCellsPtr =
        (taf_RadioNgbrCells_t*)le_ref_Lookup(tafRadio.ngbrCellsRefMap, ngbrCellsRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellsPtr == NULL, NULL, "Invalid para(null ptr)");

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(ngbrCellsPtr->cellInfoList), ngbrCellsPtr->currPtr);
    if (linkPtr == NULL)
    {
        LE_WARN("Reach to the end of list.");
        return NULL;
    }

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr = CONTAINER_OF(linkPtr, taf_RadioNgbrCellInfo_t, link);
    ngbrCellsPtr->currPtr = linkPtr;

    taf_RadioNgbrCellInfoSafeRef_t* safeRefPtr =
       (taf_RadioNgbrCellInfoSafeRef_t*)le_mem_ForceAlloc(tafRadio.ngbrCellInfoSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafRadio.ngbrCellInfoSafeRefMap, (void*)ngbrCellInfoPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(ngbrCellsPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_CellInfoRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor Cell ID.
 *
 * @return
 *  - UINT64_MAX Invalid Radio Access Technology or neighbor cell information reference.
 *  - Others     Neighbor Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetNeighborCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, UINT64_MAX, "Invalid para(null ptr)");

    switch (ngbrCellInfoPtr->cell.rat)
    {
        case TAF_RADIO_RAT_GSM:
            return (uint64_t)ngbrCellInfoPtr->cell.gsm.cid;
        case TAF_RADIO_RAT_UMTS:
            return (uint64_t)ngbrCellInfoPtr->cell.umts.cid;
        case TAF_RADIO_RAT_TDSCDMA:
            return (uint64_t)ngbrCellInfoPtr->cell.tdscdma.cid;
        case TAF_RADIO_RAT_LTE:
            return (uint64_t)ngbrCellInfoPtr->cell.lte.cid;
        case TAF_RADIO_RAT_NR5G:
            return (uint64_t)ngbrCellInfoPtr->cell.nr5g.cid;
        default:
            LE_ERROR("Invalid RAT(%d)", ngbrCellInfoPtr->cell.rat);
    }

    return UINT64_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell Location Area Code.
 *
 * @return
 *  - UINT32_MAX Invalid Radio Access Technology or neighbor cell information reference.
 *  - Others     Neighbor cell Location Area Code.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetNeighborCellLocAreaCode
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, UINT32_MAX, "Invalid para(null ptr)");

    switch (ngbrCellInfoPtr->cell.rat)
    {
        case TAF_RADIO_RAT_GSM:
            return (uint32_t)ngbrCellInfoPtr->cell.gsm.lac;
        case TAF_RADIO_RAT_UMTS:
            return (uint32_t)ngbrCellInfoPtr->cell.umts.lac;
        case TAF_RADIO_RAT_TDSCDMA:
            return (uint32_t)ngbrCellInfoPtr->cell.tdscdma.lac;
        default:
            LE_ERROR("Invalid RAT(%d)", ngbrCellInfoPtr->cell.rat);
    }

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell signal strengh.
 *
 * @return
 *  - INT32_MAX Invalid Radio Access Technology or neighbor cell information reference.
 *  - Others    Neighbor cell signal strengh.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetNeighborCellRxLevel
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, INT32_MAX, "Invalid para(null ptr)");

    return (int32_t)ngbrCellInfoPtr->cell.ss;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell Radio Access Technology
 *
 * @return
 *  - TAF_RADIO_RAT_UNKNOWN Invalid neighbor cell information reference.
 *  - Others                Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetNeighborCellRat
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, TAF_RADIO_RAT_UNKNOWN, "Invalid para(null ptr)");

    return ngbrCellInfoPtr->cell.rat;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell GSM Base Station Identity Code.
 *
 * @return
 *  - LE_FAULT     Invalid Radio Access Technology.
 *  - LE_OK        On success.
 *  - LE_NOT_FOUND Reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNeighborCellGsmBsic
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef, ///< [IN] Neighbor cell information reference.
    uint8_t* bsicPtr                         ///< [OUT] Base Station Identity Code.
)
{
    TAF_ERROR_IF_RET_VAL(bsicPtr == NULL, LE_FAULT, "Invalid para(null ptr)");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, LE_NOT_FOUND, "Invalid para(null ptr)");

    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr->cell.rat != TAF_RADIO_RAT_GSM, LE_FAULT, "Not GSM cell.");

    *bsicPtr = (uint8_t)ngbrCellInfoPtr->cell.gsm.bsic;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell LTE Physical Cell ID.
 *
 * @return
 *  - UINT16_MAX Invalid Radio Access Technology or neighbor cell information reference.
 *  - Others Physical Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPhysicalNeighborLteCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, UINT16_MAX, "Invalid para(null ptr)");

    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr->cell.rat != TAF_RADIO_RAT_LTE, UINT16_MAX,
        "Not LTE cell.");

    return (uint16_t)ngbrCellInfoPtr->cell.lte.pcid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor cell NR5G Physical Cell ID.
 *
 * @return
 *  - UINT32_MAX Invalid Radio Access Technology or neighbor cell information reference.
 *  - Others Physical Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalNeighborNrCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef ///< [IN] Neighbor cell information reference.
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNgbrCellInfo_t* ngbrCellInfoPtr =
        (taf_RadioNgbrCellInfo_t*)le_ref_Lookup(tafRadio.ngbrCellInfoSafeRefMap, ngbrCellInfoRef);
    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr == NULL, UINT32_MAX, "Invalid para(null ptr)");

    TAF_ERROR_IF_RET_VAL(ngbrCellInfoPtr->cell.rat != TAF_RADIO_RAT_NR5G, UINT32_MAX,
        "Not NR5G cell.");

    return ngbrCellInfoPtr->cell.nr5g.pcid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band capabilities.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandCapabilities
(
    taf_radio_BandBitMask_t* bandMaskPtr, ///< [OUT] 2G/3G band capabilities.
    uint8_t phoneId                       ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(bandMaskPtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskPtr)");

    return taf_pa_radio_GetBandCapabilities(bandMaskPtr, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band capabilities.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandCapabilities
(
    uint64_t* bandMaskPtr,   ///< [OUT] LTE band capabilities.
    size_t* bandMaskSizePtr, ///< [OUT] The size of LTE band capabilities.
    uint8_t phoneId          ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(bandMaskPtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskPtr)");

    TAF_ERROR_IF_RET_VAL(bandMaskSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskSizePtr)");

    return taf_pa_radio_GetLteBandCapabilities(bandMaskPtr, bandMaskSizePtr, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set 2G/3G band preferences.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetBandPreferences
(
    taf_radio_BandBitMask_t bandMask, ///< [IN] 2G/3G band preferences.
    uint8_t phoneId                   ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_SetBandPreferences(bandMask, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band preferences.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandPreferences
(
    taf_radio_BandBitMask_t* bandMaskPtr, ///< [OUT] 2G/3G band preferences.
    uint8_t phoneId                       ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(bandMaskPtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskPtr)");

    return taf_pa_radio_GetBandPreferences(bandMaskPtr, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set LTE band preferences.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetLteBandPreferences
(
    const uint64_t* bandMask, ///< [IN] LTE band preferences.
    size_t bandMaskSize,      ///< [IN] The size of LTE band preferences.
    uint8_t phoneId           ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(bandMask == NULL, LE_BAD_PARAMETER, "Null ptr(bandMask)");

    TAF_ERROR_IF_RET_VAL(bandMaskSize < TAF_RADIO_LTE_BAND_GROUP_NUM, LE_BAD_PARAMETER,
        "Invalid para(bandMaskSize:%d)", bandMaskSize);

    return taf_pa_radio_SetLteBandPreferences(bandMask, bandMaskSize, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band preferences.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandPreferences
(
    uint64_t* bandMaskPtr,   ///< [OUT] LTE band preferences.
    size_t* bandMaskSizePtr, ///< [OUT] The size of LTE band preferences.
    uint8_t phoneId          ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, LE_BAD_PARAMETER,
        "Invalid para(phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_VAL(bandMaskPtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskPtr)");

    TAF_ERROR_IF_RET_VAL(bandMaskSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(bandMaskSizePtr)");

    return taf_pa_radio_GetLteBandPreferences(bandMaskPtr, bandMaskSizePtr, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Cet cellular network radio access technology.
 *
 * @return
 *  - TAF_RADIO_RAT_UNKNOWN Invalid parameters or internal errors.
 *  - Others                Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetCellularNetworkRat
(
    taf_radio_ScanInformationRef_t scanInformationRef ///< [IN] Network scan information reference.
)
{
    TAF_ERROR_IF_RET_VAL(scanInformationRef == nullptr, TAF_RADIO_RAT_UNKNOWN,
        "Null reference(scanInformationRef)");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_ref_Lookup(tafRadio.scanOpSafeRefMap,
        scanInformationRef);
    TAF_ERROR_IF_RET_VAL(opPtr == nullptr, TAF_RADIO_RAT_UNKNOWN, "Invalid para(null ptr)");

    switch (opPtr->rat)
    {
        case telux::tel::RadioTechnology::RADIO_TECH_GSM:
            return TAF_RADIO_RAT_GSM;
        case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
            return TAF_RADIO_RAT_UMTS;
        case telux::tel::RadioTechnology::RADIO_TECH_1xRTT:
        case telux::tel::RadioTechnology::RADIO_TECH_EVDO_0:
        case telux::tel::RadioTechnology::RADIO_TECH_EVDO_A:
            return TAF_RADIO_RAT_CDMA;
        case telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA:
            return TAF_RADIO_RAT_TDSCDMA;
        case telux::tel::RadioTechnology::RADIO_TECH_LTE:
        case telux::tel::RadioTechnology::RADIO_TECH_LTE_CA:
            return TAF_RADIO_RAT_LTE;
        case telux::tel::RadioTechnology::RADIO_TECH_NR5G:
            return TAF_RADIO_RAT_NR5G;
        default:
            LE_WARN("Unknown RAT(%d).", (int)opPtr->rat);
    }

    return TAF_RADIO_RAT_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform network scan with Pysical Cell ID.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others Handler reference of performing network scan with Pysical Cell ID.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationListRef_t taf_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask, ///< [IN] Radio Access Technology bitmask, not support NR5G.
    uint8_t phoneId                 ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_VAL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM, NULL,
        "Invalid para(phoneId:%d)", phoneId);

    return taf_pa_radio_PerformPciNetworkScan(ratMask, phoneId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform PCI network scan asynchronously.
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_PerformPciNetworkScanAsync
(
    taf_radio_RatBitMask_t ratMask,                   ///< [IN] Radio Access Technology bitmask.
    taf_radio_PciNetworkScanHandlerFunc_t handlerPtr, ///< [IN] Handler function for PCI network scan.
    void* contextPtr,                                 ///< [IN] Handler context.
    uint8_t phoneId                                   ///< [IN] Phone ID.
)
{
    TAF_ERROR_IF_RET_NIL(!phoneId || phoneId > TAF_RADIO_PHONE_NUM,
        "Invalid para(phoneId:%d)", phoneId);

    taf_RadioCmdReq_t cmdReq;
    memset(&cmdReq, 0, sizeof(taf_RadioCmdReq_t));
    cmdReq.cmdType = TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN;
    cmdReq.handlerFuncPtr = (void*)handlerPtr;
    cmdReq.contextPtr = contextPtr;
    cmdReq.phoneId = phoneId;
    cmdReq.ratMask = ratMask;

    le_event_Report(taf_Radio::radioCmdEvId, &cmdReq, sizeof(taf_RadioCmdReq_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information reference.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others PCI network scan information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
        ///< [IN] PCI network scan information list reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationListRef == NULL, NULL,
        "Null reference(pciScanInformationListRef)");

    return taf_pa_radio_GetFirstPciScanInfo(pciScanInformationListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information reference.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others PCI network scan information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
        ///< [IN] PCI network scan information list reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationListRef == NULL, NULL,
        "Null reference(pciScanInformationListRef)");

    return taf_pa_radio_GetNextPciScanInfo(pciScanInformationListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN network information reference.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others PLMN network scan information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationRef == NULL, NULL,
        "Null reference(pciScanInformationRef)");

    return taf_pa_radio_GetFirstPlmnInfo(pciScanInformationRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN network information reference.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others PLMN network scan information reference.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationRef == NULL, NULL,
        "Null reference(pciScanInformationRef)");

    return taf_pa_radio_GetNextPlmnInfo(pciScanInformationRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Cell ID.
 *
 * @return
 *  - UINT16_MAX Invalid parameters or internal errors.
 *  - Others     Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationRef == NULL, UINT16_MAX,
        "Null reference(pciScanInformationRef)");

    return taf_pa_radio_GetPciScanCellId(pciScanInformationRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Global Cell ID.
 *
 * @return
 *  - UINT32_MAX Invalid parameters or internal errors.
 *  - Others     Global Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationRef == NULL, UINT32_MAX,
        "Null reference(pciScanInformationRef)");

    return taf_pa_radio_GetPciScanGlobalCellId(pciScanInformationRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MCC and MNC of PLMN information from PCI network scan.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef, ///< [IN] PLMN network scan information reference.
    char* mccPtr,                           ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                      ///< [IN] The size of Mobile Country Code string.
    char* mncPtr,                           ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                       ///< [IN] The size of Mobile Network Code string.
)
{
    TAF_ERROR_IF_RET_VAL(plmnRef == NULL, LE_BAD_PARAMETER, "Null reference(plmnRef)");

    TAF_ERROR_IF_RET_VAL(mccPtr == NULL, LE_BAD_PARAMETER, "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL(mncPtr == NULL, LE_BAD_PARAMETER, "Null ptr(mncPtr)");

    TAF_ERROR_IF_RET_VAL(mccPtrSize < TAF_RADIO_MCC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mccPtrSize: %d < %d)", mccPtrSize, TAF_RADIO_MCC_BYTES);

    TAF_ERROR_IF_RET_VAL(mncPtrSize < TAF_RADIO_MNC_BYTES, LE_BAD_PARAMETER,
        "Invalid para(mncPtrSize: %d < %d)", mncPtrSize, TAF_RADIO_MNC_BYTES);

    return taf_pa_radio_GetPciScanMccMnc(plmnRef, mccPtr, mccPtrSize, mncPtr, mncPtrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete PCI network scan.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
        ///< [IN] PCI network scan information list reference.
)
{
    TAF_ERROR_IF_RET_VAL(pciScanInformationListRef == NULL, LE_BAD_PARAMETER,
        "Null reference(pciScanInformationListRef)");

    return taf_pa_radio_DeletePciNetworkScan(pciScanInformationListRef);
}
