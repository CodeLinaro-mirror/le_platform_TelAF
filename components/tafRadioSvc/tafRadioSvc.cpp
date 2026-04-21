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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafRadio.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetRadioPower
(
    le_onoff_t power, ///< [IN] Power status.
    uint8_t phone     ///< [IN] Phone.
)
{
    taf_pa_radio_OperatingMode_t mode = TAF_PA_RADIO_OPERATING_MODE_UNKNOWN;

    switch (power)
    {
        case LE_OFF:
            mode = TAF_PA_RADIO_OPERATING_MODE_LOW_POWER;
            break;
        case LE_ON:
            mode = TAF_PA_RADIO_OPERATING_MODE_ONLINE;
            break;
        default:
            LE_ERROR("Invalid power status %d.", power);
            return LE_BAD_PARAMETER;
    }

    pa_result_t result = taf_pa_radio_SetOperatingMode(0, mode);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the radio power status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRadioPower
(
    le_onoff_t* powerPtr, ///< [OUT] Power status.
    uint8_t phone         ///< [IN] Phone.
)
{
    if (powerPtr == nullptr)
    {
        LE_ERROR("powerPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_OperatingMode_t mode = TAF_PA_RADIO_OPERATING_MODE_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetOperatingMode(0, &mode);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get operating mode.");
        return result;
    }

    if (mode == TAF_PA_RADIO_OPERATING_MODE_ONLINE)
        *powerPtr = LE_ON;
    else
        *powerPtr = LE_OFF;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets automatic network selection preference.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetAutomaticRegisterMode
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);

    taf_pa_radio_NetworkSelectionPreference_t preference;
    preference.mode = TAF_PA_RADIO_NETWORK_SELECTION_MODE_AUTOMATIC;
    preference.mcc = 0;
    preference.mnc = 0;
    pa_result_t result = taf_pa_radio_SetNetworkSelectionPreference(instance, &preference);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets manual network selection preference.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_OUT_OF_RANGE -- Out of range.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetManualRegisterMode
(
    const char* mccPtr, ///< [IN] Mobile Country Code.
    const char* mncPtr, ///< [IN] Mobile Network Code.
    uint8_t phone       ///< [IN] Phone.
)
{
    return Utility::Common::ManualNetworkSelection(phone, mccPtr, mncPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets manual network selection preference asynchronously.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_SetManualRegisterModeAsync
(
    const char* mccPtr,                                    ///< [IN] Mobile Country Code.
    const char* mncPtr,                                    ///< [IN] Mobile Network Code.
    taf_radio_ManualSelectionHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr,                                      ///< [IN] Handler context.
    uint8_t phone                                          ///< [IN] Phone.
)
{
    Request_t request;
    memset(&request, 0, sizeof(Request_t));

    request.command = COMMAND_SET_NETWORK_SELECTION_PREFERENCE;
    request.handlerFuncPtr = (void*)handlerFuncPtr;
    request.contextPtr = contextPtr;
    request.phone = phone;
    le_utf8_Copy(request.preference.mcc, mccPtr, TAF_RADIO_MCC_BYTES, nullptr);
    le_utf8_Copy(request.preference.mnc, mncPtr, TAF_RADIO_MNC_BYTES, nullptr);

    le_event_Report(Factory::staticEvents.request, &request, sizeof(Request_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the network selection preference.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_OUT_OF_RANGE -- Out of range.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRegisterMode
(
    bool* isManualPtr, ///< [OUT] Ture if registered manually
    char* mccPtr,      ///< [OUT] Mobile Country Code.
    size_t mccPtrSize, ///< [IN] Mobile Country Code length.
    char* mncPtr,      ///< [OUT] Mobile Network Code.
    size_t mncPtrSize, ///< [IN] Mobile Network Code length.
    uint8_t phone      ///< [IN] Phone.
)
{
    if (isManualPtr == nullptr)
    {
        LE_ERROR("isManualPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtrSize < TAF_RADIO_MCC_BYTES)
    {
        LE_ERROR("Invalid mccPtrSize %" PRIuS " < %d.", mccPtrSize, TAF_RADIO_MCC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    if (mncPtrSize < TAF_RADIO_MNC_BYTES)
    {
        LE_ERROR("Invalid mncPtrSize %" PRIuS " < %d.", mncPtrSize, TAF_RADIO_MNC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_NetworkSelectionPreference_t preference;
    pa_result_t paResult = taf_pa_radio_GetNetworkSelectionPreference(instance, &preference);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get network selection preference.");
        return result;
    }

    switch (preference.mode)
    {
        case TAF_PA_RADIO_NETWORK_SELECTION_MODE_AUTOMATIC:
            *isManualPtr = false;
            break;
        case TAF_PA_RADIO_NETWORK_SELECTION_MODE_MANUAL:
            *isManualPtr = true;
            break;
        default:
            LE_ERROR("Unknown mode %d.", preference.mode);
            return LE_BAD_PARAMETER;
    }

    result = Utility::Convert::U16ToString(preference.mcc, mccPtr, TAF_RADIO_MCC_BYTES, false);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %d.", preference.mcc);
        return result;
    }

    result = Utility::Convert::U16ToString(preference.mnc, mncPtr, TAF_RADIO_MNC_BYTES, true);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %d.", preference.mnc);
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the error code of network selection.
 *
 * @return
 *  - int32_t value is correspond to taf_radio_NetRejCause_t
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    auto& factory = Factory::GetInstance();

    return factory.cache.netRejectCause;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a preferred operator.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (null pointer or invalid MCC/MNC string).
 *  - LE_OUT_OF_RANGE -- MCC or MNC value out of range [0, 999].
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_AddPreferredOperator
(
    const char* mccPtr,             ///< [IN] Mobile Country Code.
    const char* mncPtr,             ///< [IN] Mobile Network Code.
    taf_radio_RatBitMask_t bitmask, ///< [IN] RAT bitmask.
    uint8_t phone                   ///< [IN] Phone.
)
{
    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_PreferredNetworkConfig_t config;
    config.clearPrevious = 0;
    config.networkCount = 1;
    le_result_t result = Utility::Convert::StringToU16(mccPtr, &config.networks[0].mcc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %s.", mccPtr);
        return result;
    }

    result = Utility::Convert::StringToU16(mncPtr, &config.networks[0].mnc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %s.", mncPtr);
        return result;
    }

    config.networks[0].bitmask = Utility::Convert::Rat(bitmask);
    pa_result_t paResult = taf_pa_radio_SetPreferredNetwork(instance, &config);

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a preferred operator.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (null pointer or invalid MCC/MNC string).
 *  - LE_OUT_OF_RANGE -- MCC or MNC value out of range [0, 999].
 *  - LE_NOT_FOUND -- Operator not found in the preferred network list.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_RemovePreferredOperator
(
    const char* mccPtr, ///< [IN] Mobile Country Code.
    const char* mncPtr, ///< [IN] Mobile Network Code.
    uint8_t phone       ///< [IN] Phone. 
)
{
    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint16_t mcc = 0, mnc = 0;
    le_result_t result = Utility::Convert::StringToU16(mccPtr, &mcc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %s.", mccPtr);
        return result;
    }
    result = Utility::Convert::StringToU16(mncPtr, &mnc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %s.", mncPtr);
        return result;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_PreferredNetworks_t networks;
    pa_result_t paResult = taf_pa_radio_GetPreferredNetwork(instance, &networks);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get preferred network.");
        return result;
    }

    if (networks.nonStaticNetworksValid == 0)
    {
        LE_ERROR("No preferred networks.");
        return LE_NOT_FOUND;
    }

    taf_pa_radio_PreferredNetworkConfig_t config;
    uint32_t i = 0, j = 0;
    for (; i < networks.nonStaticNetworkCount &&
        i < TAF_PA_RADIO_PREFERRED_NETWORK_MAX_COUNT; i++)
    {
        if (networks.nonStaticNetworks[i].mcc == mcc && networks.nonStaticNetworks[i].mnc == mnc)
            continue;

        config.networks[j].mcc = networks.nonStaticNetworks[i].mcc;
        config.networks[j].mnc = networks.nonStaticNetworks[i].mnc;
        config.networks[j].bitmask = networks.nonStaticNetworks[i].bitmask;

        j++;
    }

    if (j == i)
    {
        LE_ERROR("Failed to find MCC %s, MNC %s in prefered networks.", mccPtr, mncPtr);
        return LE_NOT_FOUND;
    }

    config.clearPrevious = 1;
    config.networkCount = j;
    paResult = taf_pa_radio_SetPreferredNetwork(instance, &config);

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the preferred network list.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeletePreferredOperatorsList
(
    taf_radio_PreferredOperatorListRef_t listRef ///< [IN] The preferred network list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    PrefNet_t* netPtr = nullptr;
    le_sls_Link_t* linkPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->commonList))) != nullptr)
    {
        netPtr = CONTAINER_OF(linkPtr, PrefNet_t, link);
        le_mem_Release(netPtr);
    }

    SafeRef_t* safeRefPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != nullptr)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, SafeRef_t, link);
        le_ref_DeleteRef(factory.maps.safeRef, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(factory.maps.commonList, listRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the preferred network list.
 *
 * @return
 *  - taf_radio_PreferredOperatorListRef_t reference for the preferred network list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorListRef_t taf_radio_GetPreferredOperatorsList
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_PreferredNetworks_t networks;
    pa_result_t paResult = taf_pa_radio_GetPreferredNetwork(instance, &networks);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get preferred network.");
        return nullptr;
    }

    if (networks.nonStaticNetworksValid == 0 || networks.nonStaticNetworkCount == 0)
    {
        LE_ERROR("No preferred networks.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_mem_ForceAlloc(factory.pools.commonList);
    listPtr->commonList = LE_SLS_LIST_INIT;
    listPtr->safeRefList = LE_SLS_LIST_INIT;
    listPtr->currPtr = nullptr;

    PrefNet_t* netPtr = nullptr;
    for (uint32_t i = 0; i < networks.nonStaticNetworkCount &&
        i < TAF_PA_RADIO_PREFERRED_NETWORK_MAX_COUNT; i++)
    {
        netPtr = (PrefNet_t*)le_mem_ForceAlloc(factory.pools.prefNet);
        netPtr->prefNet = networks.nonStaticNetworks[i];
        netPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(listPtr->commonList), &(netPtr->link));
    }

    return (taf_radio_PreferredOperatorListRef_t)le_ref_CreateRef(factory.maps.commonList,
        (void*)listPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first preferred network.
 *
 * @return
 *  - taf_radio_PreferredOperatorRef_t reference for the first preferred network.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorRef_t taf_radio_GetFirstPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t listRef ///< [IN] The preferred network list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->commonList));
    if (linkPtr == nullptr)
    {
        LE_ERROR("linkPtr is nullptr.");
        return nullptr;
    }

    PrefNet_t* nodePtr = CONTAINER_OF(linkPtr, PrefNet_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)nodePtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PreferredOperatorRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next preferred network.
 *
 * @return
 *  - taf_radio_PreferredOperatorRef_t reference for the next preferred network.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorRef_t taf_radio_GetNextPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t listRef ///< [IN] The preferred network list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->commonList), listPtr->currPtr);
    if (linkPtr == nullptr)
        return nullptr;

    PrefNet_t* netPtr = CONTAINER_OF(linkPtr, PrefNet_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)netPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_PreferredOperatorRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the details of the preferred network.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_OUT_OF_RANGE -- Out of range.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPreferredOperatorDetails
(
    taf_radio_PreferredOperatorRef_t networkRef, ///< [IN] The preferred network reference.
    char* mccPtr,                                ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                           ///< [IN] Mobile Country Code length.
    char* mncPtr,                                ///< [OUT] Mobile Network Code.
    size_t mncPtrSize,                           ///< [IN] Mobile Network Code length.
    taf_radio_RatBitMask_t* bitmaskPtr           ///< [OUT] RAT bitmask.
)
{
    if (networkRef == nullptr)
    {
        LE_ERROR("networkRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtrSize < TAF_RADIO_MCC_BYTES)
    {
        LE_ERROR("Invalid mccPtrSize %" PRIuS " < %d.", mccPtrSize, TAF_RADIO_MCC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    if (mncPtrSize < TAF_RADIO_MNC_BYTES)
    {
        LE_ERROR("Invalid mncPtrSize %" PRIuS " < %d.", mncPtrSize, TAF_RADIO_MNC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    PrefNet_t* netPtr = (PrefNet_t*)le_ref_Lookup(factory.maps.safeRef, networkRef);
    if (netPtr == nullptr)
    {
        LE_ERROR("netPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_result_t result = Utility::Convert::U16ToString(netPtr->prefNet.mcc, mccPtr,
        TAF_RADIO_MCC_BYTES, false);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %d.", netPtr->prefNet.mcc);
        return result;
    }

    result = Utility::Convert::U16ToString(netPtr->prefNet.mnc, mncPtr, TAF_RADIO_MNC_BYTES,
        false);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %d.", netPtr->prefNet.mnc);
        return result;
    }

    *bitmaskPtr = Utility::Convert::Rat(netPtr->prefNet.bitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for network rejection.
 *
 * @return
 *  - taf_radio_NetRegRejectHandlerRef_t handler reference for network rejection.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetRegRejectHandlerRef_t taf_radio_AddNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                    ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NetworkRejection",
        factory.events.networkRejection, Utility::LayeredFunction::NetworkRejection,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NetRegRejectHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for network rejection.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for RAT changes.
 *
 * @return
 *  - taf_radio_RatChangeHandlerRef_t handler reference for RAT changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RatChangeHandlerRef_t taf_radio_AddRatChangeHandler
(
    taf_radio_RatChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("RatChange",
        factory.events.ratChange, Utility::LayeredFunction::RatChange, (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_RatChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for RAT changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveRatChangeHandler
(
    taf_radio_RatChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the Radio Access Technology in use.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRadioAccessTechInUse
(
    taf_radio_Rat_t* ratPtr, ///< [OUT] The Radio Access Technology in use.
    uint8_t phone            ///< [IN] Phone.
)
{
    if (ratPtr == nullptr)
    {
        LE_ERROR("ratPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetServingRat(instance, &rat);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving RAT.");
        return result;
    }

    *ratPtr = Utility::Convert::Rat(rat);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the Radio Access Technology preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetRatPreferences
(
    taf_radio_RatBitMask_t bitmask, ///< [IN] The Radio Access Technology bitmask.
    uint8_t phone                   ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_RatBitMask_t rat = Utility::Convert::Rat(bitmask);
    pa_result_t result = taf_pa_radio_SetPreferredRat(instance, rat);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the Radio Access Technology preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRatPreferences
(
    taf_radio_RatBitMask_t* bitmaskPtr, ///< [OUT] The Radio Access Technology bitmask.
    uint8_t phone                       ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_RatBitMask_t bitmask = 0x0;
    pa_result_t paResult = taf_pa_radio_GetPreferredRat(instance, &bitmask);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get preferred RAT.");
        return result;
    }

    *bitmaskPtr =  Utility::Convert::Rat(bitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the network registration state.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNetRegState
(
    taf_radio_NetRegState_t* statePtr, ///< [OUT] The network registration state.
    uint8_t phone                      ///< [IN] Phone.
)
{
    if (statePtr == nullptr)
    {
        LE_ERROR("statePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);

    taf_pa_radio_VoiceServiceInfo_t voiceInfo;

    taf_radio_NetRegState_t vState = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    taf_radio_NetRegState_t dState = TAF_RADIO_NET_REG_STATE_UNKNOWN;

    pa_result_t paResult = taf_pa_radio_GetVoiceServiceInfo(instance, &voiceInfo);
    le_result_t result = Utility::Convert::Result(paResult);
    if ( result == LE_OK) {
        vState = Utility::Convert::NetRegState(&voiceInfo);
    } else {
        LE_WARN("Failed to get Voice Service Info for phoneId %d", phone);
    }

    result = taf_radio_GetPacketSwitchedState(&dState, phone);
    if (result != LE_OK) {
        LE_WARN("Failed to get Data Service Info for phoneId %d", phone);
    }

    *statePtr = Utility::Convert::CombineNetRegState(vState, dState);

    LE_DEBUG("PhoneId %d NetRegState: Voice(%d) + Data(%d) -> Combined(%d)", 
             phone, vState, dState, *statePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for network registration state.
 *
 * @return
 *  - taf_radio_NetRegStateEventHandlerRef_t handler reference for network registration state.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetRegStateEventHandlerRef_t taf_radio_AddNetRegStateEventHandler
(
    taf_radio_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                   ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NetRegState",
        factory.events.netRegState, Utility::LayeredFunction::NetRegState, (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NetRegStateEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for network registration state.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetRegStateEventHandler
(
    taf_radio_NetRegStateEventHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the packet switched state.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPacketSwitchedState
(
    taf_radio_NetRegState_t* statePtr, ///< [OUT] The packet swicthed state.
    uint8_t phone                      ///< [IN] Phone.
)
{
    if (statePtr == nullptr)
    {
        LE_ERROR("statePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_DataServiceState_t state = TAF_PA_RADIO_DATA_SERVICE_STATE_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetDataServieState(instance, &state);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get data service state.");
        return result;
    }

    taf_pa_radio_DataRoamingStatus_t status = TAF_PA_RADIO_DATA_ROAMING_STATUS_UNKNOWN;
    paResult = taf_pa_radio_GetDataCurrRoamingStatus(instance, &status);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get data current roaming status.");
        return result;
    }

    *statePtr =  Utility::Convert::NetRegState(state, status);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for packet swicthed state.
 *
 * @return
 *  - taf_radio_PacketSwitchedChangeHandlerRef_t handler reference for packet swicthed state.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PacketSwitchedChangeHandlerRef_t taf_radio_AddPacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                            ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("PacketSwitchedState",
        factory.events.packetSwitchedState, Utility::LayeredFunction::NetRegState,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_PacketSwitchedChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for packet swicthed state.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemovePacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerRef_t handlerRef  ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the service domain.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServiceDomain
(
    taf_radio_ServiceDomainState_t* domainPtr, ///< [OUT] The service domain.
    uint8_t phone                              ///< [IN] Phone.
)
{
    if (domainPtr == nullptr)
    {
        LE_ERROR("domainPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetServingRat(instance, &rat);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving RAT.");
        return result;
    }

    taf_pa_radio_ServiceDomain_t domain = TAF_PA_RADIO_SERVICE_DOMAIN_UNKNOWN;
    paResult = taf_pa_radio_GetServiceDomain(instance, rat, &domain);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get service domain.");
        return result;
    }

    *domainPtr = Utility::Convert::ServiceDomain(domain);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the service domain preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServiceDomainPreferences
(
    taf_radio_ServiceDomainState_t* domainPtr, ///< [OUT] The service domain preferences.
    uint8_t phone                              ///< [IN] Phone.
)
{
    if (domainPtr == nullptr)
    {
        LE_ERROR("domainPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ServiceDomainBitMask_t bitmask = 0x0;
    pa_result_t paResult = taf_pa_radio_GetServiceDomainPreferences(instance, &bitmask);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get service domain preferences.");
        return result;
    }

    *domainPtr = Utility::Convert::ServiceDomain(bitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the service domain preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetServiceDomainPreferences
(
    taf_radio_ServiceDomainState_t domain, ///< [IN] The service domain preferences.
    uint8_t phone                          ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ServiceDomainBitMask_t bitmask = Utility::Convert::ServiceDomain(domain);
    pa_result_t result = taf_pa_radio_SetServiceDomainPreferences(instance, bitmask);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the signal strength level.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetSignalQual
(
    uint32_t* levelPtr, ///< [OUT] Signal strength level.
    uint8_t phone       ///< [IN] Phone.
)
{
    if (levelPtr == nullptr)
    {
        LE_ERROR("levelPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetServingRat(instance, &rat);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving RAT.");
        return result;
    }

    taf_pa_radio_SignalStrengthLevel_t level = TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_UNKNOWN;
    paResult = taf_pa_radio_GetSignalStrengthLevel(instance, rat, &level);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get signal strength level.");
        return result;
    }

    *levelPtr = Utility::Convert::SignalStrengthLevel(level);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the signal metrics.
 *
 * @return
 *  - taf_radio_MetricsRef_t reference for the signal metrics.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_MetricsRef_t taf_radio_MeasureSignalMetrics
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_SignalStrengthInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetSignalStrengthInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get signal strength information.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =
        (taf_pa_radio_SignalStrengthInfo_t*)le_mem_ForceAlloc(factory.pools.signalStrengthInfo);
    memcpy(infoPtr, &info, sizeof(taf_pa_radio_SignalStrengthInfo_t));
   
    return (taf_radio_MetricsRef_t)le_ref_CreateRef(factory.maps.signalStrengthInfo,
        (void*)infoPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the signal metrics.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef ///< [IN] The signal metrics reference.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(factory.maps.signalStrengthInfo, metricsRef);
    le_mem_Release(infoPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT bitmask of the signal metrics.
 *
 * @return
 *  - taf_radio_RatBitMask_t value is the RAT bitmask of the signal metrics.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RatBitMask_t taf_radio_GetRatOfSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef ///< [IN] The signal metrics reference.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return 0x0;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return 0x0;
    }

    return Utility::Convert::Rat(infoPtr->bitmask);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the GSM signal metrics.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetGsmSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] The signal metrics reference.
    int32_t* rssiPtr,                  ///< [OUT] Received Signal Strength Indicator in dBm.
    uint32_t* berPtr                   ///< [OUT] Bit Error Rate.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rssiPtr == nullptr)
    {
        LE_ERROR("rssiPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (berPtr == nullptr)
    {
        LE_ERROR("berPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->bitmask & TAF_PA_RADIO_BITMASK_RAT_GSM)
    {
        *rssiPtr = infoPtr->gsmInfo.rssi;
        *berPtr = infoPtr->gsmInfo.ber;
    }
    else
    {
        *rssiPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *berPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the UMTS signal metrics.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetUmtsSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] The signal metrics reference.
    int32_t* ssPtr,                    ///< [OUT] Signal Strength in dBm.
    uint32_t* berPtr,                  ///< [OUT] Bit Error Rate.
    int32_t* rscpPtr                   ///< [OUT] Received Signal Code Power in dBm.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (ssPtr == nullptr)
    {
        LE_ERROR("ssPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (berPtr == nullptr)
    {
        LE_ERROR("berPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rscpPtr == nullptr)
    {
        LE_ERROR("rscpPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->bitmask & TAF_PA_RADIO_BITMASK_RAT_UMTS)
    {
        *ssPtr = infoPtr->umtsInfo.ss;
        *berPtr = infoPtr->umtsInfo.ber;
        *rscpPtr = infoPtr->umtsInfo.rscp;
    }
    else
    {
        *ssPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *berPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *rscpPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the LTE signal metrics.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] The signal metrics reference.
    int32_t* rssiPtr,                  ///< [OUT] Received Signal Strength Indicator in dBm.
    int32_t* rsrqPtr,                  ///< [OUT] Reference Signal Received Quality in dB.
    int32_t* rsrpPtr,                  ///< [OUT] Reference Signal Received Power in dBm.
    int32_t* snrPtr                    ///< [OUT] Signal-to-Noise Ratio in units of 0.1 dB.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rssiPtr == nullptr)
    {
        LE_ERROR("rssiPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rsrqPtr == nullptr)
    {
        LE_ERROR("rsrqPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rsrpPtr == nullptr)
    {
        LE_ERROR("rsrpPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (snrPtr == nullptr)
    {
        LE_ERROR("snrPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->bitmask & TAF_PA_RADIO_BITMASK_RAT_LTE)
    {
        *rssiPtr = infoPtr->lteInfo.rssi;
        *rsrqPtr = infoPtr->lteInfo.rsrq;
        *rsrpPtr = infoPtr->lteInfo.rsrp;
        *snrPtr = infoPtr->lteInfo.snr;
    }
    else
    {
        *rssiPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *rsrqPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *rsrpPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *snrPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the CDMA signal metrics.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCdmaSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] The signal metrics reference.
    int32_t* ssPtr,                    ///< [OUT] Signal Strength in dBm.
    int32_t* ecioPtr,                  ///< [OUT] Ec/Io in units of -0.5 dB.
    int32_t* sinrPtr,                  ///< [OUT] Signal-to-Interference-plus-Noise Ratio level.
    int32_t* ioPtr                     ///< [OUT] Received IO in dBm.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (ssPtr == nullptr)
    {
        LE_ERROR("ssPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (ecioPtr == nullptr)
    {
        LE_ERROR("ecioPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (sinrPtr == nullptr)
    {
        LE_ERROR("sinrPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (ioPtr == nullptr)
    {
        LE_ERROR("ioPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->bitmask & TAF_PA_RADIO_BITMASK_RAT_CDMA)
    {
        *ssPtr = infoPtr->cdmaInfo.ss;
        *ecioPtr = infoPtr->cdmaInfo.ecio;
        *ioPtr = infoPtr->cdmaInfo.io;
        *sinrPtr = infoPtr->cdmaInfo.sinr;
    }
    else
    {
        *ssPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *ecioPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *ioPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *sinrPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the NR5G signal metrics
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNr5gSignalMetrics
(
    taf_radio_MetricsRef_t metricsRef, ///< [IN] The signal metrics reference.
    int32_t* rsrqPtr,                  ///< [OUT] Reference Signal Received Quality in dB.
    int32_t* rsrpPtr,                  ///< [OUT] Reference Signal Received Power in dBm.
    int32_t* snrPtr                    ///< [OUT] Signal-to-Noise Ratio in units of 0.1 dB.
)
{
    if (metricsRef == nullptr)
    {
        LE_ERROR("metricsRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rsrqPtr == nullptr)
    {
        LE_ERROR("rsrqPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rsrpPtr == nullptr)
    {
        LE_ERROR("rsrpPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (snrPtr == nullptr)
    {
        LE_ERROR("snrPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    taf_pa_radio_SignalStrengthInfo_t* infoPtr =(taf_pa_radio_SignalStrengthInfo_t*)le_ref_Lookup(
        factory.maps.signalStrengthInfo, metricsRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->bitmask & TAF_PA_RADIO_BITMASK_RAT_NR5G)
    {
        *rsrqPtr = infoPtr->nr5gInfo.rsrq;
        *rsrpPtr = infoPtr->nr5gInfo.rsrp;
        *snrPtr = infoPtr->nr5gInfo.snr;
    }
    else
    {
        *rsrqPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *rsrpPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        *snrPtr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for signal strength changes.
 *
 * @return
 *  - taf_radio_SignalStrengthChangeHandlerRef_t handler reference for signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_SignalStrengthChangeHandlerRef_t taf_radio_AddSignalStrengthChangeHandler
(
    taf_radio_Rat_t rat,                                        ///< [IN] Radio Access Technology.
    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                            ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = nullptr;
    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
            handlerRef = le_event_AddLayeredHandler("GsmSignalStrengthInfoChange",
                factory.events.gsmSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_RAT_CDMA:
            handlerRef = le_event_AddLayeredHandler("CdmaSignalStrengthInfoChange",
                factory.events.cdmaSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_RAT_UMTS:
            handlerRef = le_event_AddLayeredHandler("UmtsSignalStrengthInfoChange",
                factory.events.umtsSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            handlerRef = le_event_AddLayeredHandler("TdscdmaSignalStrengthInfoChange",
                factory.events.tdscdmaSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_RAT_LTE:
            handlerRef = le_event_AddLayeredHandler("LteSignalStrengthInfoChange",
                factory.events.lteSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        case TAF_RADIO_RAT_NR5G:
            handlerRef = le_event_AddLayeredHandler("Nr5gSignalStrengthInfoChange",
                factory.events.nr5gSignalStrengthInfoChange,
                Utility::LayeredFunction::SignalStrengthInfoChange, (void*)handlerFuncPtr);
            break;
        default:
            LE_ERROR("Unknown RAT %d.", rat);
            return nullptr;
    }

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_SignalStrengthChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for signal strength change.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveSignalStrengthChangeHandler
(
    taf_radio_SignalStrengthChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell identity. 
 *
 * @return
 *  - UINT32_MAX -- Failed.
 *  - Others -- The serving cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellId
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        switch (info.cellLocInfo[index].rat)
        {
            case TAF_PA_RADIO_RAT_GSM:
                return info.cellLocInfo[index].gsmInfo.cid;
            case TAF_PA_RADIO_RAT_UMTS:
                return info.cellLocInfo[index].umtsInfo.cid;
            case TAF_PA_RADIO_RAT_TDSCDMA:
                return info.cellLocInfo[index].tdscdmaInfo.cid;
            case TAF_PA_RADIO_RAT_LTE:
                return info.cellLocInfo[index].lteInfo.cid;
            case TAF_PA_RADIO_RAT_NR5G:
                LE_ERROR("Please use taf_radio_GetServingNrCellId instead to get 64-bit value.");
                break;
            default:
                LE_ERROR("Unsupported RAT %d.", info.cellLocInfo[index].rat);
        }
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell location area code. 
 *
 * @return
 *  - UINT32_MAX -- Failed.
 *  - Others -- The serving cell location area code. 
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellLocAreaCode
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        switch (info.cellLocInfo[index].rat)
        {
            case TAF_PA_RADIO_RAT_GSM:
                return info.cellLocInfo[index].gsmInfo.lac;
            case TAF_PA_RADIO_RAT_UMTS:
                return info.cellLocInfo[index].umtsInfo.lac;
            case TAF_PA_RADIO_RAT_TDSCDMA:
                return info.cellLocInfo[index].tdscdmaInfo.lac;
            default:
                LE_ERROR("Unsupported RAT %d.", info.cellLocInfo[index].rat);
        }
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving LTE cell tracking area code.
 *
 * @return
 *  - UINT16_MAX -- Failed.
 *  - Others -- The serving LTE cell tracking area code.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetServingCellLteTracAreaCode
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT16_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_LTE)
        return info.cellLocInfo[index].lteInfo.tac;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the LTE serving cell E-UTRA absolute radio frequen channel number.
 *
 * @return
 *  - UINT32_MAX -- Failed.
 *  - Others -- The LTE serving cell E-UTRA absolute radio frequen channel number.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellEarfcn
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_LTE)
        return info.cellLocInfo[index].lteInfo.earfcn;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell timing advance.
 *
 * @return
 *  - UINT32_MAX -- Failed.
 *  - Others -- The serving cell timing advance.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellTimingAdvance
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        switch (info.cellLocInfo[index].rat)
        {
            case TAF_PA_RADIO_RAT_GSM:
                return info.cellLocInfo[index].gsmInfo.ta;
            case TAF_PA_RADIO_RAT_LTE:
                return info.cellLocInfo[index].lteInfo.ta;
            default:
                LE_ERROR("Unsupported RAT %d.", info.cellLocInfo[index].rat);
        }
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving LTE cell physical cell identity.
 *
 * @return
 *  - UINT16_MAX -- Failed.
 *  - Others -- The serving LTE cell physical cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPhysicalServingLteCellId
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT16_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_LTE)
        return info.cellLocInfo[index].lteInfo.pcid;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving GSM cell base station identity code.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellGsmBsic
(
    uint8_t* bsicPtr, ///< [OUT] Base station identity code.
    uint8_t phone     ///< [IN] Phone.
)
{
    if (bsicPtr == nullptr)
    {
        LE_ERROR("bsicPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return result;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_GSM)
    {
        *bsicPtr = info.cellLocInfo[index].gsmInfo.bsic;
        return LE_OK;
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving UMTS cell primary scrambling code.
 *
 * @return
 *  - UINT16_MAX -- Failed.
 *  - Others -- The serving UMTS cell primary scrambling code.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetServingCellScramblingCode
(
    uint8_t phone
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT16_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_UMTS)
        return info.cellLocInfo[index].umtsInfo.psc;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the current network short name.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkName
(
    char* namePtr,      ///< [OUT] The short name.
    size_t namePtrSize, ///< [IN] The short name size.
    uint8_t phone       ///< [IN] Phone.
)
{
    if (namePtr == nullptr)
    {
        LE_ERROR("namePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CurrNetworkName_t name;
    name.shortNameValid = 1;
    name.shortNameSize = namePtrSize;
    name.shortNamePtr = namePtr;
    name.fullNameValid = 0;
    pa_result_t paResult = taf_pa_radio_GetCurrNetworkName(instance, &name);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get current network name.");
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the mobile country code and the mobile network code of the serving cell
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_UNSUPPORTED -- Unsupported.
 *  - LE_OUT_OF_RANGE -- Out of range.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkMccMnc
(
    char* mccPtr,      ///< [OUT] Mobile Country Code.
    size_t mccPtrSize, ///< [IN] Mobile Country Code length.
    char* mncPtr,      ///< [OUT] Mobile Network Code.
    size_t mncPtrSize, ///< [IN] Mobile Network Code length.
    uint8_t phone      ///< [IN] Phone.
)
{
    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtrSize < TAF_RADIO_MCC_BYTES)
    {
        LE_ERROR("Invalid mccPtrSize %" PRIuS " < %d.", mccPtrSize, TAF_RADIO_MCC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    if (mncPtrSize < TAF_RADIO_MNC_BYTES)
    {
        LE_ERROR("Invalid mncPtrSize %" PRIuS " < %d.", mncPtrSize, TAF_RADIO_MNC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return result;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        uint16_t mcc = 0, mnc = 0;
        switch (info.cellLocInfo[index].rat)
        {
            case TAF_PA_RADIO_RAT_GSM:
                if (info.cellLocInfo[index].gsmInfo.plmnIdValid)
                {
                    mcc = info.cellLocInfo[index].gsmInfo.plmnId.mcc;
                    mnc = info.cellLocInfo[index].gsmInfo.plmnId.mnc;
                }
                else
                    LE_ERROR("Invalid PLMN ID.");
                break;
            case TAF_PA_RADIO_RAT_UMTS:
                if (info.cellLocInfo[index].umtsInfo.plmnIdValid)
                {
                    mcc = info.cellLocInfo[index].umtsInfo.plmnId.mcc;
                    mnc = info.cellLocInfo[index].umtsInfo.plmnId.mnc;
                }
                else
                    LE_ERROR("Invalid PLMN ID.");
                break;
            case TAF_PA_RADIO_RAT_TDSCDMA:
                if (info.cellLocInfo[index].tdscdmaInfo.plmnIdValid)
                {
                    mcc = info.cellLocInfo[index].tdscdmaInfo.plmnId.mcc;
                    mnc = info.cellLocInfo[index].tdscdmaInfo.plmnId.mnc;
                }
                else
                    LE_ERROR("Invalid PLMN ID.");
                break;
            case TAF_PA_RADIO_RAT_LTE:
                if (info.cellLocInfo[index].lteInfo.plmnIdValid)
                {
                    mcc = info.cellLocInfo[index].lteInfo.plmnId.mcc;
                    mnc = info.cellLocInfo[index].lteInfo.plmnId.mnc;
                }
                else
                    LE_ERROR("Invalid PLMN ID.");
                break;
            case TAF_PA_RADIO_RAT_NR5G:
                if (info.cellLocInfo[index].nr5gInfo.plmnIdValid)
                {
                    mcc = info.cellLocInfo[index].nr5gInfo.plmnId.mcc;
                    mnc = info.cellLocInfo[index].nr5gInfo.plmnId.mnc;
                }
                else
                    LE_ERROR("Invalid PLMN ID.");
                break;
            default:
                LE_ERROR("Unsupported RAT %d.", info.cellLocInfo[index].rat);
                return LE_UNSUPPORTED;
        }
        result = Utility::Convert::U16ToString(mcc, mccPtr, TAF_RADIO_MCC_BYTES, false);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to convert MCC %d.", mcc);
            return result;
        }

        result = Utility::Convert::U16ToString(mnc, mncPtr, TAF_RADIO_MNC_BYTES, true);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to convert MNC %d.", mnc);
            return result;
        }

        return LE_OK;
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs a PLMN network scan.
 *
 * @return
 *  - taf_radio_ScanInformationListRef_t reference for the PLMN network scan list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationListRef_t taf_radio_PerformCellularNetworkScan
(
    uint8_t phone ///< [IN] Phone.
)
{
    return Utility::Common::PlmnNetworkScan(phone);
}

//--------------------------------------------------------------------------------------------------
/**
 * Peforms a network scan asynchronously.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_PerformCellularNetworkScanAsync
(
    taf_radio_CellularNetworkScanHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr,                                          ///< [IN] Handler context.
    uint8_t phone                                              ///< [IN] Phone.
)
{
    Request_t request;
    memset(&request, 0, sizeof(Request_t));

    request.command = COMMAND_PERFORM_PLMN_NETWORK_SCAN;
    request.handlerFuncPtr = (void*)handlerFuncPtr;
    request.contextPtr = contextPtr;
    request.phone = phone;

    le_event_Report(Factory::staticEvents.request, &request, sizeof(Request_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first scanned PLMN network.
 *
 * @return
 *  - taf_radio_ScanInformationRef_t reference for the first scanned PLMN network.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationRef_t taf_radio_GetFirstCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t listRef ///< [IN] The PLMN network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->commonList));
    if (linkPtr == nullptr)
    {
        LE_ERROR("linkPtr is nullptr.");
        return nullptr;
    }

    PlmnInfo_t* infoPtr = CONTAINER_OF(linkPtr, PlmnInfo_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)infoPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_ScanInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next scanned PLMN network.
 *
 * @return
 *  - taf_radio_ScanInformationRef_t reference for the next scanned PLMN network.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationRef_t taf_radio_GetNextCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t listRef ///< [IN] The PLMN network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->commonList), listPtr->currPtr);
    if (linkPtr == nullptr)
        return nullptr;

    PlmnInfo_t* netPtr = CONTAINER_OF(linkPtr, PlmnInfo_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)netPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_ScanInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the mobile country code and mobile network code of a scanned PLMN network
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_OUT_OF_RANGE -- Out of range.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCellularNetworkMccMnc
(
    taf_radio_ScanInformationRef_t infoRef, ///< [IN] The PLMN network reference.
    char* mccPtr,                           ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                      ///< [IN] Mobile Country Code length.
    char* mncPtr,                           ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                       ///< [IN] Mobile Network Code length.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtrSize < TAF_RADIO_MCC_BYTES)
    {
        LE_ERROR("Invalid mccPtrSize %" PRIuS " < %d.", mccPtrSize, TAF_RADIO_MCC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    if (mncPtrSize < TAF_RADIO_MNC_BYTES)
    {
        LE_ERROR("Invalid mncPtrSize %" PRIuS " < %d.", mncPtrSize, TAF_RADIO_MNC_BYTES);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (infoPtr->plmnInfo.plmnIdValid)
    {
        le_result_t result = Utility::Convert::U16ToString(infoPtr->plmnInfo.plmnId.mcc, mccPtr,
            TAF_RADIO_MCC_BYTES, false);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to convert MCC %d.", infoPtr->plmnInfo.plmnId.mcc);
            return result;
        }

        result = Utility::Convert::U16ToString(infoPtr->plmnInfo.plmnId.mnc, mncPtr,
            TAF_RADIO_MNC_BYTES, false);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to convert MNC %d.", infoPtr->plmnInfo.plmnId.mnc);
            return result;
        }
    }
    else
    {
        LE_ERROR("PLMN ID is invalid.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the the namework name of a scanned PLMN network
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (null pointer).
 *  - LE_NOT_FOUND -- Scan information reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCellularNetworkName
(
    taf_radio_ScanInformationRef_t infoRef, ///< [IN] The PLMN network reference.
    char* namePtr,                          ///< [OUT] The PLMN network name.
    size_t namePtrSize                      ///< [IN] The PLMN network name size.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (namePtr == nullptr)
    {
        LE_ERROR("namePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_utf8_Copy(namePtr, infoPtr->plmnInfo.description, namePtrSize, nullptr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the scanned PLMN network in use status
 *
 * @return
 *  - Ture -- The PLMN network is in use.
 *  - False -- The PLMN network is not in use.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkInUse
(
    taf_radio_ScanInformationRef_t infoRef ///< [IN] The PLMN network reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return false;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return false;
    }

    if (infoPtr->plmnInfo.inUseStatus == TAF_PA_RADIO_NETWORK_IN_USE_STATUS_CURRENT_SERVING)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the scanned PLMN network available status
 *
 * @return
 *  - Ture -- The PLMN network is available.
 *  - False -- The PLMN network is not available.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkAvailable
(
    taf_radio_ScanInformationRef_t infoRef ///< [IN] The PLMN network reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return false;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return false;
    }


    if (infoPtr->plmnInfo.inUseStatus == TAF_PA_RADIO_NETWORK_IN_USE_STATUS_AVAILABLE)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the scanned PLMN network home status
 *
 * @return
 *  - Ture -- The PLMN network is home status.
 *  - False -- The PLMN network is not home status.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkHome
(
    taf_radio_ScanInformationRef_t infoRef ///< [IN] The PLMN network reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return false;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return false;
    }


    if (infoPtr->plmnInfo.roamingStatus == TAF_PA_RADIO_NETWORK_ROAMING_STATUS_HOME)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the scanned PLMN network forbidden status
 *
 * @return
 *  - Ture -- The PLMN network is forbidden status.
 *  - False -- The PLMN network is not forbidden status.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkForbidden
(
    taf_radio_ScanInformationRef_t infoRef ///< [IN] The PLMN network reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return false;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* infoPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return false;
    }

    if (infoPtr->plmnInfo.forbiddenStatus == TAF_PA_RADIO_NETWORK_FORBIDDEN_STATUS_FORBIDDEN)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the scanned PLMN network list
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t listRef ///< [IN] The PLMN network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    PlmnInfo_t* infoPtr = nullptr;
    le_sls_Link_t* linkPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->commonList))) != nullptr)
    {
        infoPtr = CONTAINER_OF(linkPtr, PlmnInfo_t, link);
        le_mem_Release(infoPtr);
    }

    SafeRef_t* safeRefPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != nullptr)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, SafeRef_t, link);
        le_ref_DeleteRef(factory.maps.safeRef, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(factory.maps.commonList, listRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the signal strength indication thresholds.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndThresholds
(
    taf_radio_SigType_t metric,  ///< [IN] The signal metric.
    int32_t lowerRangeThreshold, ///< [IN] Lower range threshold in 0.1 dBm.
    int32_t upperRangeThreshold, ///< [IN] Upper range threshold in 0.1 dBm.
    uint8_t phone                ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_SignalStrengthIndConfig_t config;
    Utility::Convert::SignalStrengthIndConfig(instance, metric, &config);
    config.deltaValid = 0;
    config.thresholdValid = 1;
    config.thresholdCount = 2;
    config.thresholds[0] = lowerRangeThreshold;
    config.thresholds[1] = upperRangeThreshold;
    pa_result_t paResult = taf_pa_radio_SetSignalStrengthInd(instance, &config);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set signal strength indication configuration.");
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the signal strength indication delta.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndDelta
(
    taf_radio_SigType_t metric, ///< [IN] The signal metric.
    uint16_t delta,             ///< [IN] Delta in 0.1 dBm.
    uint8_t phone               ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_SignalStrengthIndConfig_t config;
    Utility::Convert::SignalStrengthIndConfig(instance, metric, &config);
    config.thresholdValid = 0;
    config.deltaValid = 1;
    config.delta = delta;
    pa_result_t paResult = taf_pa_radio_SetSignalStrengthInd(instance, &config);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set signal strength indication configuration.");
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving NR cell identity. 
 *
 * @return
 *  - UINT64_MAX -- Failed.
 *  - Others -- The serving NR cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetServingNrCellId
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT64_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_NR5G)
        return info.cellLocInfo[index].nr5gInfo.cid;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT64_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving NR cell tracking area code.
 *
 * @return
 *  - INT32_MAX -- Failed.
 *  - Others -- The serving NR cell tracking area code.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrTracAreaCode
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return INT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_NR5G)
        return info.cellLocInfo[index].nr5gInfo.tac;
    else
        LE_ERROR("Failed to find serving cell.");

    return INT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the NR serving cell absolute radio frequen channel number.
 *
 * @return
 *  - INT32_MAX -- Failed.
 *  - Others -- The NR serving cell absolute radio frequen channel number.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrArfcn
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return INT32_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_NR5G)
        return info.cellLocInfo[index].nr5gInfo.arfcn;
    else
        LE_ERROR("Failed to find serving cell.");

    return INT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving NR cell physical cell identity.
 *
 * @return
 *  - UINT32_MAX -- Failed.
 *  - Others -- The serving NR cell physical cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalServingNrCellId
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return UINT16_MAX;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_NR5G)
        return info.cellLocInfo[index].nr5gInfo.pcid;
    else
        LE_ERROR("Failed to find serving cell.");

    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor cells information.
 *
 * @return
 *  - taf_radio_NeighborCellsRef_t reference for neighbor cells information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NeighborCellsRef_t taf_radio_GetNeighborCellsInfo
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_mem_ForceAlloc(factory.pools.commonList);
    listPtr->commonList = LE_SLS_LIST_INIT;
    listPtr->safeRefList = LE_SLS_LIST_INIT;
    listPtr->currPtr = nullptr;

    NgbrCell_t* cellPtr = nullptr;
    for (uint32_t i = 0; i < info.cellLocInfoCount && i < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT;
        i++)
    {
        if (info.cellLocInfo[i].location == TAF_PA_RADIO_CELL_LOCATION_NEIGHBOR)
        {
            cellPtr = (NgbrCell_t*)le_mem_ForceAlloc(factory.pools.ngbrCell);
            cellPtr->ngbrCell = info.cellLocInfo[i];

            cellPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(listPtr->commonList), &(cellPtr->link));
        }
    }

    return (taf_radio_NeighborCellsRef_t)le_ref_CreateRef(factory.maps.commonList, (void*)listPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes neighbor cells information.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteNeighborCellsInfo
(
    taf_radio_NeighborCellsRef_t infoRef ///< [IN] The neighbor cells information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, infoRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    NgbrCell_t* cellPtr = nullptr;
    le_sls_Link_t* linkPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->commonList))) != nullptr)
    {
        cellPtr = CONTAINER_OF(linkPtr, NgbrCell_t, link);
        le_mem_Release(cellPtr);
    }

    SafeRef_t* safeRefPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != nullptr)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, SafeRef_t, link);
        le_ref_DeleteRef(factory.maps.safeRef, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(factory.maps.commonList, infoRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first neighbor cell.
 *
 * @return
 *  - taf_radio_CellInfoRef_t reference for the first neighbor cell.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetFirstNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t cellRef ///< [IN] The neighbor cell reference.
)
{
    if (cellRef == nullptr)
    {
        LE_ERROR("cellRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, cellRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->commonList));
    if (linkPtr == nullptr)
    {
        LE_ERROR("linkPtr is nullptr.");
        return nullptr;
    }

    NgbrCell_t* cellPtr = CONTAINER_OF(linkPtr, NgbrCell_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)cellPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_CellInfoRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next neighbor cell.
 *
 * @return
 *  - taf_radio_CellInfoRef_t reference for the next neighbor cell.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetNextNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t cellRef ///< [IN] The neighbor cell reference.
)
{
    if (cellRef == nullptr)
    {
        LE_ERROR("cellRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, cellRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->commonList), listPtr->currPtr);
    if (linkPtr == nullptr)
        return nullptr;

    NgbrCell_t* cellPtr = CONTAINER_OF(linkPtr, NgbrCell_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)cellPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_radio_CellInfoRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the neighbor cell identity.
 *
 * @return
 *  - UINT64_MAX -- Failed
 *  - Others -- The neighbor cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetNeighborCellId
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT64_MAX;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT64_MAX;
    }

    switch (cellPtr->ngbrCell.rat)
    {
        case TAF_PA_RADIO_RAT_GSM:
            return cellPtr->ngbrCell.gsmInfo.cid;
        case TAF_PA_RADIO_RAT_UMTS:
            return cellPtr->ngbrCell.umtsInfo.cid;
        case TAF_PA_RADIO_RAT_TDSCDMA:
            return cellPtr->ngbrCell.tdscdmaInfo.cid;
        case TAF_PA_RADIO_RAT_LTE:
            return cellPtr->ngbrCell.lteInfo.cid;
        case TAF_PA_RADIO_RAT_NR5G:
            return cellPtr->ngbrCell.nr5gInfo.cid;
        default:
            LE_ERROR("Unsupported RAT %d.", cellPtr->ngbrCell.rat);
    }

    return UINT64_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor cell location area code.
 *
 * @return
 *  - UINT32_MAX -- Failed
 *  - Others -- The neighbor cell location area code.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetNeighborCellLocAreaCode
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT32_MAX;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT32_MAX;
    }

    switch (cellPtr->ngbrCell.rat)
    {
        case TAF_PA_RADIO_RAT_GSM:
            return cellPtr->ngbrCell.gsmInfo.lac;
        case TAF_PA_RADIO_RAT_UMTS:
            return cellPtr->ngbrCell.umtsInfo.lac;
        case TAF_PA_RADIO_RAT_TDSCDMA:
            return cellPtr->ngbrCell.tdscdmaInfo.lac;
        default:
            LE_ERROR("Unsupported RAT %d.", cellPtr->ngbrCell.rat);
    }

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor cell signal strength.
 *
 * @return
 *  - UINT32_MAX -- Failed
 *  - Others -- The neighbor cell signal strength in dBm.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetNeighborCellRxLevel
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT32_MAX;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT32_MAX;
    }

    switch (cellPtr->ngbrCell.rat)
    {
        case TAF_PA_RADIO_RAT_GSM:
            return cellPtr->ngbrCell.gsmInfo.rssi;
        case TAF_PA_RADIO_RAT_CDMA:
            return cellPtr->ngbrCell.cdmaInfo.ss;
        case TAF_PA_RADIO_RAT_UMTS:
            return cellPtr->ngbrCell.umtsInfo.ss;
        case TAF_PA_RADIO_RAT_LTE:
            return cellPtr->ngbrCell.lteInfo.rssi;
        default:
            LE_ERROR("Unsupported RAT %d.", cellPtr->ngbrCell.rat);
    }

    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor cell RAT .
 *
 * @return
 *  - TAF_RADIO_RAT_UNKNOWN -- Failed.
 *  - Others -- The neighbor cell RAT.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetNeighborCellRat
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return TAF_RADIO_RAT_UNKNOWN;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return TAF_RADIO_RAT_UNKNOWN;
    }

    return Utility::Convert::Rat(cellPtr->ngbrCell.rat);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get neighbor GSM cell base station identity code.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNeighborCellGsmBsic
(
    taf_radio_CellInfoRef_t infoRef, ///< [IN] The neighbor cell information reference.
    uint8_t* bsicPtr                 ///< [OUT] Base station identity code.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bsicPtr == nullptr)
    {
        LE_ERROR("bsicPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (cellPtr->ngbrCell.rat != TAF_PA_RADIO_RAT_GSM)
    {
        LE_ERROR("Unable to obtain BSID from a non-GSM cell.");
        return LE_FAULT;
    }

    *bsicPtr = cellPtr->ngbrCell.gsmInfo.bsic;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor LTE cell physical cell identity.
 *
 * @return
 *  - UINT16_MAX -- Failed
 *  - Others -- The neighbor LTE cell physical cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPhysicalNeighborLteCellId
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT16_MAX;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT16_MAX;
    }

    if (cellPtr->ngbrCell.rat != TAF_PA_RADIO_RAT_LTE)
    {
        LE_ERROR("Unable to obtain PCID from a non-LTE cell.");
        return UINT16_MAX;
    }

    return cellPtr->ngbrCell.lteInfo.pcid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the neighbor NR5G cell physical cell identity.
 *
 * @return
 *  - UINT32_MAX -- Failed
 *  - Others -- The neighbor NR5G cell physical cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalNeighborNrCellId
(
    taf_radio_CellInfoRef_t infoRef ///< [IN] The neighbor cell information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT32_MAX;
    }

    auto& factory = Factory::GetInstance();
    NgbrCell_t* cellPtr = (NgbrCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT32_MAX;
    }

    if (cellPtr->ngbrCell.rat != TAF_PA_RADIO_RAT_NR5G)
    {
        LE_ERROR("Unable to obtain PCID from a non-NR5G cell.");
        return UINT32_MAX;
    }

    return cellPtr->ngbrCell.nr5gInfo.pcid;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the 2G/3G band capabilities.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandCapabilities
(
    taf_radio_BandBitMask_t* bitmaskPtr, ///< [OUT] The 2G/3G band capabilities.
    uint8_t phone                        ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_BandBitMask_t bitmask = 0x0;
    pa_result_t paResult = taf_pa_radio_GetBandCapabilities(instance, &bitmask);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get band capabilities.");
        return result;
    }

    *bitmaskPtr = Utility::Convert::ToBand(bitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the LTE band capabilities.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandCapabilities
(
    uint64_t* bitmaskPtr,   ///< [OUT] The LTE band capabilities.
    size_t* bitmaskPtrSize, ///< [OUT] The LTE band capabilities size.
    uint8_t phone           ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bitmaskPtrSize == nullptr)
    {
        LE_ERROR("bitmaskPtrSize is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_LteBand_t band;
    pa_result_t paResult = taf_pa_radio_GetLteBandCapabilities(instance, &band);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get band capabilities.");
        return result;
    }

    for (uint32_t i = 0; i < TAF_PA_RADIO_LTE_BAND_GROUP_COUNT; i++)
        bitmaskPtr[i] = band.bitmask[i];

    *bitmaskPtrSize = TAF_PA_RADIO_LTE_BAND_GROUP_COUNT;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the 2G/3G band preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetBandPreferences
(
    taf_radio_BandBitMask_t bitmask, ///< [IN] The 2G/3G band preferences.
    uint8_t phone                    ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_BandBitMask_t band = Utility::Convert::ToPaBand(bitmask);
    pa_result_t result = taf_pa_radio_SetBandPreferences(instance, band);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the 2G/3G band preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandPreferences
(
    taf_radio_BandBitMask_t* bitmaskPtr, ///< [OUT] The 2G/3G band preferences.
    uint8_t phone                        ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_BandBitMask_t bitmask = 0x0;
    pa_result_t paResult = taf_pa_radio_GetBandPreferences(instance, &bitmask);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get band preferences.");
        return result;
    }

    *bitmaskPtr = Utility::Convert::ToBand(bitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the LTE band preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetLteBandPreferences
(
    const uint64_t* bitmaskPtr, ///< [IN] The LTE band preferences.
    size_t bitmaskPtrSize,      ///< [IN] The size of LTE band preferences.
    uint8_t phone               ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bitmaskPtrSize < TAF_RADIO_LTE_BAND_GROUP_NUM)
    {
        LE_ERROR("Invalid bitmaskPtrSize %" PRIuS ".", bitmaskPtrSize);
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_LteBand_t band;
    for (uint32_t i = 0; i < TAF_RADIO_LTE_BAND_GROUP_NUM ; i++)
        band.bitmask[i] = bitmaskPtr[i];

    pa_result_t result = taf_pa_radio_SetLteBandPreferences(instance, &band);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the LTE band preferences.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandPreferences
(
    uint64_t* bitmaskPtr,   ///< [OUT] The LTE band preferences.
    size_t* bitmaskPtrSize, ///< [OUT] The LTE band preferences size.
    uint8_t phone           ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bitmaskPtrSize == nullptr)
    {
        LE_ERROR("bitmaskPtrSize is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_LteBand_t band;
    pa_result_t paResult = taf_pa_radio_GetLteBandPreferences(instance, &band);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get band capabilities.");
        return result;
    }

    for (uint32_t i = 0; i < TAF_PA_RADIO_LTE_BAND_GROUP_COUNT; i++)
        bitmaskPtr[i] = band.bitmask[i];

    *bitmaskPtrSize = TAF_PA_RADIO_LTE_BAND_GROUP_COUNT;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT of a scanned PLMN network
 *
 * @return
 *  - taf_radio_Rat_t RAT of a scanned PLMN network
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetCellularNetworkRat
(
    taf_radio_ScanInformationRef_t infoRef ///< [IN] The PLMN network reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return TAF_RADIO_RAT_UNKNOWN;
    }

    auto& factory = Factory::GetInstance();
    PlmnInfo_t* cellPtr = (PlmnInfo_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return TAF_RADIO_RAT_UNKNOWN;
    }

    return Utility::Convert::Rat(cellPtr->plmnInfo.rat);
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs a network scan with physical cell identity.
 *
 * @return
 *  - taf_radio_PciScanInformationListRef_t reference for the PCI network scan list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationListRef_t taf_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t bitmask, ///< [IN] RAT bitmask.
    uint8_t phone                   ///< [IN] Phone.
)
{
    return Utility::Common::PciNetworkScan(phone, bitmask);
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs a network scan with physical cell identity asynchronously.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_PerformPciNetworkScanAsync
(
    taf_radio_RatBitMask_t bitmask,                       ///< [IN] RAT bitmask.
    taf_radio_PciNetworkScanHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr,                                     ///< [IN] Handler context.
    uint8_t phone                                         ///< [IN] Phone.
)
{
    Request_t request;
    memset(&request, 0, sizeof(Request_t));

    request.command = COMMAND_PERFORM_PCI_NETWORK_SCAN;
    request.handlerFuncPtr = (void*)handlerFuncPtr;
    request.contextPtr = contextPtr;
    request.phone = phone;
    request.rat = bitmask;

    le_event_Report(Factory::staticEvents.request, &request, sizeof(Request_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information.
 *
 * @return
 *  - taf_radio_PciScanInformationRef_t reference for the first PCI network scan information
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t listRef ///< [IN] PCI network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->commonList));
    if (linkPtr == nullptr)
        return nullptr;

    PciCell_t* cellPtr = CONTAINER_OF(linkPtr, PciCell_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)cellPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PciScanInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information.
 *
 * @return
 *  - taf_radio_PciScanInformationRef_t reference for the next PCI network scan information
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t listRef ///< [IN] PCI network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->commonList), listPtr->currPtr);
    if (linkPtr == nullptr)
        return nullptr;

    PciCell_t* cellPtr = CONTAINER_OF(linkPtr, PciCell_t, link);
    listPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)cellPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PciScanInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN information.
 *
 * @return
 *  - taf_radio_PlmnInformationRef_t reference for the first PLMN information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t infoRef ///< [IN] PCI network scan information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    PciCell_t* cellPtr = (PciCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(cellPtr->plmnIdList));
    if (linkPtr == nullptr)
        return nullptr;

    PlmnId_t* idPtr = CONTAINER_OF(linkPtr, PlmnId_t, link);
    cellPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr =(SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)idPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(cellPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PlmnInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN information.
 *
 * @return
 *  - taf_radio_PlmnInformationRef_t reference for the next PLMN information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t infoRef ///< [IN] PCI network scan information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    PciCell_t* cellPtr = (PciCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return nullptr;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(cellPtr->plmnIdList), cellPtr->currPtr);
    if (linkPtr == nullptr)
        return nullptr;

    PlmnId_t* idPtr = CONTAINER_OF(linkPtr, PlmnId_t, link);
    cellPtr->currPtr = linkPtr;

    SafeRef_t* safeRefPtr =(SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.safeRef, (void*)idPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(cellPtr->safeRefList), &(safeRefPtr->link));

    return (taf_radio_PlmnInformationRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the PCI network scan cell identity.
 *
 * @return
 *  - UINT16_MAX -- Failed
 *  - Others -- The PCI network scan cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t infoRef ///< [IN] PCI network scan information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT16_MAX;
    }

    auto& factory = Factory::GetInstance();
    PciCell_t* cellPtr = (PciCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT16_MAX;
    }

    return cellPtr->cellId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the PCI network scan global cell identity.
 *
 * @return
 *  - UINT32_MAX -- Failed
 *  - Others -- The PCI network scan cell identity.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t infoRef ///< [IN] PCI network scan information reference.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return UINT32_MAX;
    }

    auto& factory = Factory::GetInstance();
    PciCell_t* cellPtr = (PciCell_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (cellPtr == nullptr)
    {
        LE_ERROR("cellPtr is nullptr.");
        return UINT32_MAX;
    }

    return cellPtr->globalCellId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the Mobile Country Code and Mobile Network Code of a PCI network scanned cell.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t infoRef, ///< [IN] PLMN information reference.
    char* mccPtr,                           ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                      ///< [IN] Mobile Country Code length.
    char* mncPtr,                           ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                       ///< [IN] Mobile Network Code length.
)
{
    if (infoRef == nullptr)
    {
        LE_ERROR("infoRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mccPtrSize < TAF_RADIO_MCC_BYTES)
    {
        LE_ERROR("Invalid mccPtrSize %" PRIuS ".", mccPtrSize);
        return LE_BAD_PARAMETER;
    }

    if (mncPtrSize < TAF_RADIO_MNC_BYTES)
    {
        LE_ERROR("Invalid mncPtrSize %" PRIuS ".", mncPtrSize);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    PlmnId_t* idPtr = (PlmnId_t*)le_ref_Lookup(factory.maps.safeRef, infoRef);
    if (idPtr == nullptr)
    {
        LE_ERROR("idPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    string mcc = to_string(idPtr->mcc);
    string mnc = to_string(idPtr->mnc);
    le_utf8_Copy(mccPtr, mcc.c_str(), TAF_RADIO_MCC_BYTES, nullptr);

    size_t offset = 0;
    // If MNC is a two-digit value and has PCS digit.
    if (idPtr->mncIncludesPcsDigit && idPtr->mnc < 100)
    {
        *mncPtr = '0';
        offset = 1;
    }
    le_utf8_Copy(mncPtr + offset, mnc.c_str(), TAF_RADIO_MNC_BYTES - offset, nullptr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the PCI network scan list.
 *
 * @return
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t listRef ///< [IN] PCI network scan list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_ref_Lookup(factory.maps.commonList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    PciCell_t* cellPtr = nullptr;
    le_sls_Link_t *linkPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->commonList))) != nullptr)
    {
        cellPtr = CONTAINER_OF(linkPtr, PciCell_t, link);

        PlmnId_t* idPtr = nullptr;
        le_sls_Link_t* idLinkPtr = nullptr;
        while ((idLinkPtr = le_sls_Pop(&(cellPtr->plmnIdList))) !=
            nullptr)
        {
            idPtr = CONTAINER_OF(idLinkPtr, PlmnId_t, link);
            le_mem_Release(idPtr);
        }

        SafeRef_t* idSafeRefPtr = nullptr;
        while ((idLinkPtr = le_sls_Pop(&(cellPtr->safeRefList)))
            != nullptr)
        {
            idSafeRefPtr = CONTAINER_OF(idLinkPtr, SafeRef_t, link);
            le_ref_DeleteRef(factory.maps.safeRef, idSafeRefPtr->safeRef);
            le_mem_Release(idSafeRefPtr);
        }

        le_mem_Release(cellPtr);
    }

    SafeRef_t* safeRefPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, SafeRef_t, link);
        le_ref_DeleteRef(factory.maps.safeRef, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(factory.maps.commonList, listRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS registration status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsRegStatus
(
    taf_radio_ImsRegStatus_t* statusPtr, ///< [OUT] The IMS registration status.
    uint8_t phone                        ///< [IN] Phone.
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ImsRegistrationStatus_t status = TAF_PA_RADIO_IMS_REGISTRATION_STATUS_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetImsRegistrationStatus(instance, &status);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get IMS registration status.");
        return result;
    }

    *statusPtr = Utility::Convert::ImsRegistrationStatus(status);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for IMS registration status.
 *
 * @return
 *  - taf_radio_ImsRegStatusChangeHandlerRef_t handler reference for IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsRegStatusChangeHandlerRef_t taf_radio_AddImsRegStatusChangeHandler
(
    taf_radio_ImsRegStatusChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                          ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ImsRegStatusChange",
        factory.events.imsRegStatusChange, Utility::LayeredFunction::ImsRegStatusChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_ImsRegStatusChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for IMS registration status.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveImsRegStatusChangeHandler
(
    taf_radio_ImsRegStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for operating mode changes.
 *
 * @return
 *  - taf_radio_OpModeChangeHandlerRef_t handler reference for operating mode changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_OpModeChangeHandlerRef_t taf_radio_AddOpModeChangeHandler
(
    taf_radio_OpModeChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                    ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("OperatingModeChange",
        factory.events.operatingModeChange, Utility::LayeredFunction::OperatingModeChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_OpModeChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for operating mode changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveOpModeChangeHandler
(
    taf_radio_OpModeChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the network status reference.
 *
 * @return
 *  - Non-null pointer -- The network status reference.
 *  - Null pointer -- Failed.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusRef_t taf_radio_GetNetStatus
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    return factory.cache.netStatusRefs[instance];
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the CS capabilitiy of LTE network.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteCsCap
(
    taf_radio_NetStatusRef_t reference,  ///< [IN] The network status reference.
    taf_radio_CsCap_t* capabilitiyPtr    ///< [OUT] The CS capabilitiy of LTE network.
)
{
    if (capabilitiyPtr == nullptr)
    {
        LE_ERROR("capabilitiyPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_LteCsCapability_t capability = TAF_PA_RADIO_LTE_CS_CAPABILITY_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetLteCsCapability(instance, &capability);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get LTE CS capability.");
        return result;
    }

    *capabilitiyPtr = Utility::Convert::LteCsCapability(capability);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the RAT service status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRatSvcStatus
(
    taf_radio_NetStatusRef_t reference, ///< [IN] The network status reference.
    taf_radio_RatSvcStatus_t* statusPtr ///< [OUT] The RAT service status.
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetServingRat(instance, &rat);
    if (paResult != 0)
    {
        LE_ERROR("Failed to get serving RAT.");
        return Utility::Convert::Result(paResult);
    }

    taf_pa_radio_RatServiceStatus_t status = TAF_PA_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
    paResult = taf_pa_radio_GetRatSvcStatus(instance, rat, &status);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get RAT service status.");
        return result;
    }

    *statusPtr = Utility::Convert::RatServiceStatus(status);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the network status changes.
 *
 * @return
 *  - taf_radio_NetStatusChangeHandlerRef_t handler reference for the network status changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusChangeHandlerRef_t taf_radio_AddNetStatusChangeHandler
(
    taf_radio_NetStatusHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                 ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NetStatusChange",
        factory.events.netStatusChange, Utility::LayeredFunction::NetStatusChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NetStatusChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for network status changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetStatusChangeHandler
(
    taf_radio_NetStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS reference.
 *
 * @return
 *  - Non-null pointer -- The IMS reference.
 *  - Null pointer -- Failed.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsRef_t taf_radio_GetIms
(
    uint8_t phone ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    return factory.cache.imsRefs[instance];
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS service status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_UNSUPPORTED -- Unsupported.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsSvcStatus
(
    taf_radio_ImsRef_t reference,       ///< [IN] The IMS reference.
    taf_radio_ImsSvcType_t sevice,      ///< [IN] The IMS service.
    taf_radio_ImsSvcStatus_t* statusPtr ///< [OUT] The IMS service status.
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_ImsService_t paService = TAF_PA_RADIO_IMS_SERVICE_UNKNOWN;
    result = Utility::Convert::ImsService(sevice, &paService);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to IMS service.");
        return result;
    }

    taf_pa_radio_ImsServiceStatus_t status = TAF_PA_RADIO_IMS_SERVICE_STATUS_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetImsServiceStatus(instance, paService, &status);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get IMS service status.");
        return result;
    }

    *statusPtr = Utility::Convert::ImsServiceStatus(status);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS PDP error.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsPdpError
(
    taf_radio_ImsRef_t reference,  ///< [IN] The IMS reference.
    taf_radio_PdpError_t* errorPtr ///< [OUT] The IMS PDP error.
)
{
    if (errorPtr == nullptr)
    {
        LE_ERROR("errorPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_ImsPdpFailureErrorCode_t code = TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetImsPdpFailureErrorCode(instance, &code);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get IMS PDP failure error code.");
        return result;
    }

    *errorPtr = Utility::Convert::PdpError(code);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Toggles the IMS service.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetImsSvcCfg
(
    taf_radio_ImsRef_t reference,   ///< [IN] The IMS reference.
    taf_radio_ImsSvcType_t service, ///< [IN] The IMS service.
    bool enable                     ///< [IN] Toggle.
)
{
    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_ImsServiceSettingBitMask_t bitmask = Utility::Convert::ImsService(service);
    pa_result_t paResult = taf_pa_radio_ToggleImsService(instance, bitmask, enable);
    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS service toggle status.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsSvcCfg
(
    taf_radio_ImsRef_t reference,   ///< [IN] The IMS reference.
    taf_radio_ImsSvcType_t service, ///< [IN] The IMS service.
    bool* enablePtr                 ///< [OUT] Toggle status.
)
{
    if (enablePtr == nullptr)
    {
        LE_ERROR("enablePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    taf_pa_radio_ImsServiceSettingBitMask_t bitmask = 0x0;
    pa_result_t paResult = taf_pa_radio_GetEnabledImsService(instance, &bitmask);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get enabled IMS service.");
        return result;
    }

    if (Utility::Convert::ImsService(service) & bitmask)
        *enablePtr = true;
    else
        *enablePtr = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the IMS user agent.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetImsUserAgent
(
    taf_radio_ImsRef_t reference, ///< [IN] The IMS reference.
    const char* namePtr           ///< [IN] The user agent to be sent with SIP message.
)
{
    if (namePtr == nullptr)
    {
        LE_ERROR("namePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    pa_result_t paResult = taf_pa_radio_SetImsUserAgent(instance, namePtr);
    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS user agent.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsUserAgent
(
    taf_radio_ImsRef_t reference, ///< [IN] The IMS reference.
    char* namePtr,                ///< [OUT] The user agent name.
    size_t namePtrSize            ///< [IN] The user agent name size.
)
{
    if (namePtr == nullptr)
    {
        LE_ERROR("namePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = INSTANCE_MAX_COUNT;
    le_result_t result = Utility::Convert::ReferenceToInstance(reference, &instance);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert reference.");
        return result;
    }

    pa_result_t paResult = taf_pa_radio_GetImsUserAgent(instance, namePtr, namePtrSize);
    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the IMS status changes.
 *
 * @return
 *  - taf_radio_ImsStatusChangeHandlerRef_t handler reference for the IMS status changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsStatusChangeHandlerRef_t taf_radio_AddImsStatusChangeHandler
(
    taf_radio_ImsStatusChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                       ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ImsStatusChange",
        factory.events.imsStatusChange, Utility::LayeredFunction::ImsStatusChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_ImsStatusChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for the IMS status changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveImsStatusChangeHandler
(
    taf_radio_ImsStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the ENDC availability and DCNR restriction.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNrDualConnectivityStatus
(
    taf_radio_NREndcAvailability_t* availabilityPtr, ///< [OUT] The ENDC availability.
    taf_radio_NRDcnrRestriction_t* restrictionPtr,   ///< [OUT] The DCNR restriction.
    uint8_t phone                                    ///< [IN] Phone.
)
{
    if (availabilityPtr == nullptr)
    {
        LE_ERROR("availabilityPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (restrictionPtr == nullptr)
    {
        LE_ERROR("restrictionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_EndcAvailability_t availability = TAF_PA_RADIO_ENDC_AVAILABILITY_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetEndcAvailability(instance, &availability);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get ENDC availability.");
        return result;
    }

    taf_pa_radio_DcnrRestriction_t restriction = TAF_PA_RADIO_DCNR_RESTRICTION_UNKNOWN;
    paResult = taf_pa_radio_GetDcnrRestriction(instance, &restriction);
    result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get DCNR restriction.");
        return result;
    }

    *availabilityPtr = Utility::Convert::EndcAvailability(availability);
    *restrictionPtr = Utility::Convert::DcnrRestriction(restriction);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the current network full name.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkLongName
(
    char* namePtr,      ///< [OUT] The full name.
    size_t namePtrSize, ///< [IN] The full name size.
    uint8_t phone       ///< [IN] Phone.
)
{
    if (namePtr == nullptr)
    {
        LE_ERROR("namePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CurrNetworkName_t name;
    name.fullNameValid = 1;
    name.fullNameSize = namePtrSize;
    name.fullNamePtr = namePtr;
    name.shortNameValid = 0;
    pa_result_t paResult = taf_pa_radio_GetCurrNetworkName(instance, &name);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get current network name.");
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the total SIM count and maximum active SIM count.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetHardwareSimConfig
(
    uint8_t* totalCountPtr,     ///< [OUT] The total SIM count.
    uint8_t* maxActiveCountPtr  ///< [OUT] The maximum active SIM count.
)
{
    if (totalCountPtr == nullptr)
    {
        LE_ERROR("totalCountPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (maxActiveCountPtr == nullptr)
    {
        LE_ERROR("maxActiveCountPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_SimCapabilityInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetSimCapacityInfo(&info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get SIM capabilitiy information.");
        return result;
    }

    *totalCountPtr = info.totalCount;
    *maxActiveCountPtr = info.maxActiveCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the device RAT capabilities and SIM card RAT capabilities.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetHardwareSimRatCapabilities
(
    taf_radio_RatBitMask_t* devBitmaskPtr, ///< [OUT] The device RAT capabilities.
    taf_radio_RatBitMask_t* simBitmaskPtr, ///< [OUT] The SIM card RAT capabilities.
    uint8_t phone                          ///< [IN] Phone.
)
{
    if (devBitmaskPtr == nullptr)
    {
        LE_ERROR("devBitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (simBitmaskPtr == nullptr)
    {
        LE_ERROR("simBitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_DeviceAndSimCardRatCapability_t capability;
    pa_result_t paResult = taf_pa_radio_GetDeviceAndSimCardRatCapability(instance, &capability);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get device and SIM card RAT capability.");
        return result;
    }

    *devBitmaskPtr = Utility::Convert::Rat(capability.devBitmask);
    *simBitmaskPtr = Utility::Convert::Rat(capability.simBitmask);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the cell information changes.
 *
 * @return
 *  - taf_radio_CellInfoChangeHandlerRef_t handler reference for the cell information changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoChangeHandlerRef_t taf_radio_AddCellInfoChangeHandler
(
    taf_radio_CellInfoChangeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                      ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("CellInfoChange",
        factory.events.cellInfoChange, Utility::LayeredFunction::CellInfoChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_CellInfoChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for the cell information changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveCellInfoChangeHandler
(
    taf_radio_CellInfoChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the hysteresis delta for signal strength criteria.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
  */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndHysteresis
(
    taf_radio_SigType_t metric, ///< The Signal metric.
    uint16_t delta,             ///< Hysteresis delta in units of 0.1 dBm.
    uint8_t phone               ///< Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    factory.cache.hysteresisConfig[instance].delta[metric] = delta;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the hysteresis time for signal strength criteria.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
  */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndHysteresisTimer
(
    uint16_t time, ///< [IN] Hysteresis time in milliseconds.
    uint8_t phone  ///< [IN] Phone.
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    factory.cache.hysteresisConfig[instance].time = time;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving GSM cell absolute radio frequency channel number.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellArfcn
(
    int32_t* arfcnPtr, ///< [OUT] The absolute radio frequency channel number.
    uint8_t phone      ///< [IN] Phone ID.
)
{
    if (arfcnPtr == nullptr)
    {
        LE_ERROR("arfcnPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return result;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT &&
        info.cellLocInfo[index].rat == TAF_PA_RADIO_RAT_GSM)
    {
        *arfcnPtr = info.cellLocInfo[index].gsmInfo.arfcn;
        return LE_OK;
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell UTRA absolute radio frequency channel number.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_UNSUPPORTED -- Unsupported.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellUarfcn
(
    int32_t* uarfcnPtr, ///< [OUT] The UTRA absolute radio frequency channel number.
    uint8_t phone       ///< [IN] Phone.
)
{
    if (uarfcnPtr == nullptr)
    {
        LE_ERROR("uarfcnPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_CellLocationListInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetCellLocationListInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get cell location list information.");
        return result;
    }

    uint32_t index = Utility::Common::FindServingCell(&info);
    if (index < info.cellLocInfoCount && index < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        switch (info.cellLocInfo[index].rat)
        {
            case TAF_PA_RADIO_RAT_UMTS:
                *uarfcnPtr = info.cellLocInfo[index].umtsInfo.uarfcn;
                return LE_OK;
            case TAF_PA_RADIO_RAT_TDSCDMA:
                *uarfcnPtr = info.cellLocInfo[index].tdscdmaInfo.uarfcn;
                return LE_OK;
            default:
                LE_ERROR("Unsupported RAT %d.", info.cellLocInfo[index].rat);
                return LE_UNSUPPORTED;
        }
        return LE_OK;
    }
    else
        LE_ERROR("Failed to find serving cell.");

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the operating mode.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetOperatingMode
(
    taf_radio_OpMode_t mode, ///< [IN] The operating mode.
    uint8_t phone            ///< [IN] Phone.
)
{
    taf_pa_radio_OperatingMode_t paMode = Utility::Convert::OperatingMode(mode);
    pa_result_t result = taf_pa_radio_SetOperatingMode(0, paMode);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the operating mode.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetOperatingMode
(
    taf_radio_OpMode_t* modePtr, ///< [OUT] The operating mode.
    uint8_t phone                ///< [IN] Phone.
)
{
    if (modePtr == nullptr)
    {
        LE_ERROR("modePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_OperatingMode_t mode = TAF_PA_RADIO_OPERATING_MODE_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetOperatingMode(0, &mode);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get operating mode.");
        return result;
    }

    result = Utility::Convert::OperatingMode(mode, modePtr);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert operating mode %d.", mode);
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets routing area code.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellRoutingAreaCode
(
    uint8_t* racPtr, ///< [OUT] The routing area code.
    uint8_t phone    ///< [IN] Phone.
)
{
    if (racPtr == nullptr)
    {
        LE_ERROR("racPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);

    taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
    pa_result_t result = taf_pa_radio_GetServingRat(instance, &rat);
    if (result != 0)
    {
        LE_ERROR("Failed to get serving RAT.");
        return LE_FAULT;
    }

    result = taf_pa_radio_GetServingCellRac(instance, rat, racPtr);

    return Utility::Convert::Result(result);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the 2G/3G serving cell band information.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellBandInfo
(
    taf_radio_BandBitMask_t* bitmaskPtr,   ///< [OUT] The 2G/3G active band.
    taf_radio_RFBandWidth_t* bandwidthPtr, ///< [OUT] The RF bandwidth.
    uint8_t phone                          ///< [IN] Phone.
)
{
    if (bitmaskPtr == nullptr)
    {
        LE_ERROR("bitmaskPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandwidthPtr == nullptr)
    {
        LE_ERROR("bandwidthPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ServingCellBandInfo_t info;
    info.bandInfoValid = 1;
    info.lteBandInfoValid = 0;
    info.nr5gBandInfoValid = 0;
    pa_result_t paResult = taf_pa_radio_GetServingCellBandInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving cell band information.");
        return result;
    }

    *bitmaskPtr = Utility::Convert::ToBand(info.activeBand);
    *bandwidthPtr = Utility::Convert::Bandwidth(info.bandwidth);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the LTE serving cell band information.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellLteBandInfo
(
    uint32_t* bandPtr,                     ///< [OUT] The LTE active band.
    taf_radio_RFBandWidth_t* bandwidthPtr, ///< [OUT] The RF bandwidth.
    uint8_t phone                          ///< [IN] Phone.
)
{
    if (bandPtr == nullptr)
    {
        LE_ERROR("bandPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandwidthPtr == nullptr)
    {
        LE_ERROR("bandwidthPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_Bandwidth_t bandwidth = TAF_PA_RADIO_BANDWIDTH_UNKNOWN;
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ServingCellBandInfo_t info;
    info.bandInfoValid = 0;
    info.lteBandInfoValid = 1;
    info.lteActiveBandPtr = bandPtr;
    info.lteBandwidthPtr = &bandwidth;
    info.nr5gBandInfoValid = 0;
    pa_result_t paResult = taf_pa_radio_GetServingCellBandInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving cell band information.");
        return result;
    }

    *bandwidthPtr = Utility::Convert::Bandwidth(bandwidth);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the NR5G serving cell band information.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellNrBandInfo
(
    uint32_t* bandPtr,                     ///< [OUT] The NR5G active band.
    taf_radio_RFBandWidth_t* bandwidthPtr, ///< [OUT] The RF bandwidth.
    uint8_t phone                          ///< [IN] Phone.
)
{
    if (bandPtr == nullptr)
    {
        LE_ERROR("bandPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandwidthPtr == nullptr)
    {
        LE_ERROR("bandwidthPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_Bandwidth_t bandwidth = TAF_PA_RADIO_BANDWIDTH_UNKNOWN;
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_ServingCellBandInfo_t info;
    info.bandInfoValid = 0;
    info.lteBandInfoValid = 0;
    info.nr5gBandInfoValid = 1;
    info.nr5gActiveBandPtr = bandPtr;
    info.nr5gBandwidthPtr = &bandwidth;
    pa_result_t paResult = taf_pa_radio_GetServingCellBandInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get serving cell band information.");
        return result;
    }

    *bandwidthPtr = Utility::Convert::Bandwidth(bandwidth);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the NR icon.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNrIconType
(
    taf_radio_NrIconType_t* iconPtr, ///< [OUT] The NR icon.
    uint8_t phone                    ///< [IN] Phone.
)
{
    if (iconPtr == nullptr)
    {
        LE_ERROR("iconPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_NrIcon_t icon = TAF_PA_RADIO_NR_ICON_UNKNOWN;
    pa_result_t paResult = taf_pa_radio_GetNrIcon(instance, &icon);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get NR icon.");
        return result;
    }

    *iconPtr = Utility::Convert::NrIcon(icon);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert NR icon %d.", icon);
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the NR icon type changes.
 *
 * @return
 *  - taf_radio_NrIconTypeHandlerRef_t handler reference for the NR icon type changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NrIconTypeHandlerRef_t taf_radio_AddNrIconTypeHandler
(
    taf_radio_NrIconTypeHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                  ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("NrIconChange",
        factory.events.nrIconChange, Utility::LayeredFunction::NrIconChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_NrIconTypeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for the NR icon type changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNrIconTypeHandler
(
    taf_radio_NrIconTypeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the CA information changes.
 *
 * @return
 *  - taf_radio_CAInfoHandlerRef_t handler reference for the CA information changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CAInfoHandlerRef_t taf_radio_AddCAInfoHandler
(
    taf_radio_Rat_t rat,                          ///< [IN] Radio Access Technology.
    taf_radio_CAInfoHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                              ///< [IN] Handler Context.
)
{
    if (rat != TAF_RADIO_RAT_LTE)
    {
        LE_ERROR("Invalid RAT %d.", rat);
        return nullptr;
    }

    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("CAInfoChange",
        factory.events.caInfoChange, Utility::LayeredFunction::CAInfoChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_CAInfoHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes handler for CA information changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveCAInfoHandler
(
    taf_radio_CAInfoHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the carrier aggregation information.
 *
 * @return
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 *  - LE_TIMEOUT -- Timeout.
 *  - LE_UNSUPPORTED -- Unsupported.
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCAInformation
(
    uint8_t phone,                      ///< [IN] Phone.
    taf_radio_Rat_t rat,                ///< [IN] Radio Access Technology.
    taf_radio_CAInfoRef_t* referencePtr ///< [OUT] The carrier aggregation information reference.
)
{
    if (referencePtr == nullptr)
    {
        LE_ERROR("referencePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (rat != TAF_RADIO_RAT_LTE)
    {
        LE_ERROR("Invalid RAT %d.", rat);
        return LE_UNSUPPORTED;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_LteCphyCaInfo_t info;
    pa_result_t paResult = taf_pa_radio_GetLteCphyCaInfo(instance, &info);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to LTE Control PHY Carrier Aggregation information.");
        return result;
    }

    auto& factory = Factory::GetInstance();
    CAInfo_t* infoPtr = (CAInfo_t*)le_mem_ForceAlloc(factory.pools.caInfo);
    Utility::Convert::LteCphyCaInfo(&info, infoPtr);
    *referencePtr = (taf_radio_CAInfoRef_t)le_ref_CreateRef(factory.maps.caInfo, (void*)infoPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the carrier aggregation information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteCAInformation
(
    taf_radio_CAInfoRef_t reference ///< [IN] The carrier aggregation information reference.
)
{
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CAInfo_t* infoPtr = (CAInfo_t*)le_ref_Lookup(factory.maps.caInfo, reference);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(factory.maps.caInfo, reference);

    le_mem_Release(infoPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the Carrier Aggregation (CA) status and the number of active Component Carriers (CC)
 * currently configured for LTE.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteCAStatus
(
    taf_radio_CAInfoRef_t reference, ///< [IN] The carrier aggregation information reference.
    taf_radio_CAStatus_t* statusPtr, ///< [OUT] The carrier aggregation status.
    uint32_t* numPtr                 ///< [OUT] The number of active component carriers
)
{
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (numPtr == nullptr)
    {
        LE_ERROR("numPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    CAInfo_t* infoPtr = (CAInfo_t*)le_ref_Lookup(factory.maps.caInfo, reference);
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    *statusPtr = infoPtr->status;
    *numPtr = infoPtr->cellCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds handler for the connection status changes.
 *
 * @return
 *  - taf_radio_ConnectionStatusHandlerRef_t handler reference for the connection status changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ConnectionStatusHandlerRef_t taf_radio_AddConnectionStatusHandler
(
    taf_radio_ConnectionStatusHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                                        ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ConnStatusChange",
        factory.events.connStatusChange, Utility::LayeredFunction::ConnStatusChange,
       (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_ConnectionStatusHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the handler for the connection status changes
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveConnectionStatusHandler
(
    taf_radio_ConnectionStatusHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of connection status.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (null pointer).
 *  - LE_FAULT -- Failed to get data available system status.
 *  - LE_NOT_IMPLEMENTED -- Not implemented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetConnStatus
(
    uint8_t phone,                          ///< [IN] Phone.
    taf_radio_ConnStatusRef_t* referencePtr ///< [OUT] The reference of connection status.
)
{
    if (referencePtr == nullptr)
    {
        LE_ERROR("referencePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);

    taf_pa_radio_DataAvailSysStatus_t status;
    pa_result_t paResult = taf_pa_radio_GetDataAvailSysStatus(instance, &status);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get data available system status.");
        return result;
    }

    auto& factory = Factory::GetInstance();

    taf_radio_NREndcAvailability_t* availabilityPtr =
        (taf_radio_NREndcAvailability_t*)le_mem_ForceAlloc(factory.pools.connStatus);
    *availabilityPtr = Utility::Convert::EndcStatus(&status);

    *referencePtr = (taf_radio_ConnStatusRef_t)le_ref_CreateRef(factory.maps.connStatus,
        (void*)availabilityPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the connection status.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (null reference or reference not found in map).
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteConnStatus
(
    taf_radio_ConnStatusRef_t reference ///< [IN] The reference of connection status.
)
{
    if (reference == nullptr)
    {
        LE_ERROR("reference is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();

    taf_radio_NREndcAvailability_t* availabilityPtr =
        (taf_radio_NREndcAvailability_t*)le_ref_Lookup(factory.maps.connStatus, reference);
    if (availabilityPtr == nullptr)
    {
        LE_ERROR("availabilityPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    le_ref_DeleteRef(factory.maps.connStatus, reference);

    le_mem_Release(availabilityPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 *  Gets the ENDC connection status.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters (reference not found in map or null statusPtr).
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetEndcConnectionStatus
(
    taf_radio_ConnStatusRef_t reference,            ///< [IN] The reference of connection status.
    taf_radio_NREndcAvailability_t* statusPtr ///< [OUT] The ENDC connection status.
)
{
    auto& factory = Factory::GetInstance();

    taf_radio_NREndcAvailability_t* availabilityPtr =
        (taf_radio_NREndcAvailability_t*)le_ref_Lookup(factory.maps.connStatus, reference);
    if (availabilityPtr == nullptr)
    {
        LE_ERROR("availabilityPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    *statusPtr = *availabilityPtr;

    return LE_OK;
}