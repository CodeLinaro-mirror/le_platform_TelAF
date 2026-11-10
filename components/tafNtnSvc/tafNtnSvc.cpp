/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafNtn.hpp"

#include <vector>

using namespace std;

//--------------------------------------------------------------------------------------------------
// Support, mode, state and data APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether NTN is supported on this device.
 *
 * This is a device-level API that checks overall NTN support capability without requiring
 * a phone ID or session reference. Phone-specific support is determined through session
 * creation (taf_ntn_CreateSession).
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_IsSupported
(
    bool* supportedPtr ///< [OUT] True if NTN is supported.
)
{
    if (supportedPtr == nullptr)
    {
        LE_ERROR("supportedPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_IsNtnSupported(0, supportedPtr);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_IsNtnSupported reason=%d", paResult);
    }
    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates an NTN session.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_UNSUPPORTED -- Session creation for the requested context is not supported.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_CreateSession
(
    uint8_t phoneId,                    ///< [IN] Phone ID.
    taf_ntn_SessionRef_t* sessionRefPtr ///< [OUT] NTN session reference.
)
{
    if (sessionRefPtr == nullptr)
    {
        LE_ERROR("sessionRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phoneId);
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid phoneId %d.", phoneId);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_mem_ForceAlloc(factory.pools.session);
    sessionPtr->instance = instance;
    sessionPtr->phoneId = phoneId;

    taf_ntn_SessionRef_t sessionRef =
        (taf_ntn_SessionRef_t)le_ref_CreateRef(factory.maps.session, (void*)sessionPtr);

    // Track the most recently created session as the active context used to tag PA indications.
    factory.cache.activeSessionRef[sessionPtr->instance] = sessionRef;
    factory.cache.activeInstance = sessionPtr->instance;

    *sessionRefPtr = sessionRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes an NTN session.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Session reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_DeleteSession
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (factory.cache.activeSessionRef[sessionPtr->instance] == sessionRef)
        factory.cache.activeSessionRef[sessionPtr->instance] = nullptr;

    le_ref_DeleteRef(factory.maps.session, sessionRef);
    le_mem_Release(sessionPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets NTN mode enablement.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNSUPPORTED -- NTN is not supported.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetNtnMode
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    bool enable,                     ///< [IN] True to enable NTN mode, false to disable.
    bool isEmergency                 ///< [IN] True if NTN may be used for emergency purpose.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_EnableNtn(sessionPtr->instance, enable, isEmergency);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_EnableNtn reason=%d", paResult);
    }
    else if (enable)
    {
        factory.cache.activeInstance = sessionPtr->instance;
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets current NTN state.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetState
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    taf_ntn_State_t* statePtr        ///< [OUT] Current NTN state.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (statePtr == nullptr)
    {
        LE_ERROR("statePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_ntn_State_t paState = TAF_PA_NTN_STATE_UNKNOWN;
    taf_pa_result_t paResult = taf_pa_ntn_GetNtnState(sessionPtr->instance, &paState);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_GetNtnState reason=%d", paResult);
        return result;
    }

    *statePtr = Utility::Convert::State(paState);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the maximum data size allowed by the NTN network for the given session.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Session reference not found.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetCapabilityMaxDataSize
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    uint64_t* maxDataSizePtr         ///< [OUT] Maximum data size in bytes.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (maxDataSizePtr == nullptr)
    {
        LE_ERROR("maxDataSizePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    taf_pa_ntn_NtnCapabilities_t capabilities = {};
    taf_pa_result_t paResult = taf_pa_ntn_GetNtnCapabilities(sessionPtr->instance, &capabilities);
    le_result_t result = Utility::Convert::Result(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_GetNtnCapabilities reason=%d", paResult);
        return result;
    }

    if (capabilities.maxDataSize < 0)
    {
        return LE_UNAVAILABLE;
    }

    *maxDataSizePtr = (uint64_t)capabilities.maxDataSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends non-IP data over NTN.
 *
 * @return
 *  - LE_OK -- Data accepted by modem.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_OUT_OF_RANGE -- Data size exceeds supported size.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNAVAILABLE -- NTN service is unavailable or NTN is not in service.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SendData
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    const uint8_t* data,             ///< [IN] Data to be sent.
    size_t dataSize,                 ///< [IN] Data size.
    bool isEmergency,                ///< [IN] True if this is emergency data.
    uint64_t* transactionIdPtr       ///< [OUT] Transaction ID for acknowledgement correlation.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (data == nullptr)
    {
        LE_ERROR("data is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (transactionIdPtr == nullptr)
    {
        LE_ERROR("transactionIdPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (dataSize == 0 || dataSize > TAF_NTN_MAX_DATA_BYTES)
    {
        LE_ERROR("Invalid dataSize %" PRIuS ".", dataSize);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_SendData(sessionPtr->instance, (uint8_t*)data,
        (uint32_t)dataSize, isEmergency, transactionIdPtr);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_SendData reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Aborts all queued NTN data packets waiting for transmission.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_AbortData
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_AbortData(sessionPtr->instance);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_AbortData reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads incoming NTN data packet.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Packet reference not found.
 *  - LE_OVERFLOW -- Output buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_ReadDataPacket
(
    taf_ntn_DataPacketRef_t packetRef, ///< [IN] Data packet reference.
    uint8_t* data,                     ///< [OUT] Output data buffer.
    size_t* dataSizePtr                ///< [INOUT] Output data buffer size.
)
{
    if (packetRef == nullptr)
    {
        LE_ERROR("packetRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (data == nullptr || dataSizePtr == nullptr)
    {
        LE_ERROR("data or dataSizePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    DataPacket_t* packetPtr = (DataPacket_t*)le_ref_Lookup(factory.maps.dataPacket, packetRef);
    if (packetPtr == nullptr)
    {
        LE_ERROR("packetPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (*dataSizePtr < packetPtr->size)
    {
        LE_ERROR("Output buffer too small: %" PRIuS " < %" PRIu32 ".", *dataSizePtr,
            packetPtr->size);
        return LE_OVERFLOW;
    }

    memcpy(data, packetPtr->data, packetPtr->size);
    *dataSizePtr = packetPtr->size;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes incoming NTN data packet reference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Packet reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_DeleteDataPacket
(
    taf_ntn_DataPacketRef_t packetRef ///< [IN] Data packet reference.
)
{
    if (packetRef == nullptr)
    {
        LE_ERROR("packetRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    DataPacket_t* packetPtr = (DataPacket_t*)le_ref_Lookup(factory.maps.dataPacket, packetRef);
    if (packetPtr == nullptr)
    {
        LE_ERROR("packetPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(factory.maps.dataPacket, packetRef);
    le_mem_Release(packetPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets terrestrial cellular scan enablement while NTN mode is active.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetCellularScan
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    bool enable                      ///< [IN] True to enable terrestrial cellular scan.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_EnableCellularScan(sessionPtr->instance, enable);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_EnableCellularScan reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
// System selection specifier list builder APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Creates an empty system selection specifier list.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_CreateSystemSelectionList
(
    taf_ntn_SessionRef_t sessionRef,            ///< [IN] NTN session reference.
    taf_ntn_SystemSelectionListRef_t* listRefPtr ///< [OUT] System selection list reference.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (listRefPtr == nullptr)
    {
        LE_ERROR("listRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_mem_ForceAlloc(factory.pools.systemSelectionList);
    listPtr->instance = sessionPtr->instance;
    listPtr->specifierList = LE_SLS_LIST_INIT;
    listPtr->safeRefList = LE_SLS_LIST_INIT;
    listPtr->currPtr = nullptr;

    *listRefPtr = (taf_ntn_SystemSelectionListRef_t)le_ref_CreateRef(
        factory.maps.systemSelectionList, (void*)listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the system selection specifier list.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- List reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_DeleteSystemSelectionList
(
    taf_ntn_SystemSelectionListRef_t listRef ///< [IN] System selection list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_ref_Lookup(factory.maps.systemSelectionList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_sls_Link_t* linkPtr = nullptr;
    while ((linkPtr = le_sls_Pop(&(listPtr->specifierList))) != nullptr)
    {
        SystemSelectionSpecifier_t* specPtr =
            CONTAINER_OF(linkPtr, SystemSelectionSpecifier_t, link);
        le_ref_DeleteRef(factory.maps.specifier, specPtr->ref);
        le_mem_Release(specPtr);
    }

    le_ref_DeleteRef(factory.maps.systemSelectionList, listRef);
    le_mem_Release(listPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds an empty system selection specifier into the system selection list.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- List reference not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_AddSystemSelectionSpecifier
(
    taf_ntn_SystemSelectionListRef_t listRef,          ///< [IN] System selection list reference.
    taf_ntn_SystemSelectionSpecifierRef_t* specRefPtr  ///< [OUT] Created specifier reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (specRefPtr == nullptr)
    {
        LE_ERROR("specRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_ref_Lookup(factory.maps.systemSelectionList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_mem_ForceAlloc(factory.pools.specifier);
    memset(specPtr, 0, sizeof(SystemSelectionSpecifier_t));
    specPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->specifierList), &(specPtr->link));

    specPtr->ref = (taf_ntn_SystemSelectionSpecifierRef_t)le_ref_CreateRef(factory.maps.specifier,
        (void*)specPtr);
    *specRefPtr = specPtr->ref;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds PLMN information to a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found.
 *  - LE_DUPLICATE -- PLMN information has already been added to this specifier.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_AddSystemSelectionPlmn
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    const char* mcc,                               ///< [IN] Mobile Country Code.
    const char* mnc                                ///< [IN] Mobile Network Code.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mcc == nullptr || mnc == nullptr)
    {
        LE_ERROR("mcc or mnc is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (strlen(mcc) > TAF_NTN_MCC_MAX_LEN || strlen(mnc) > TAF_NTN_MNC_MAX_LEN)
    {
        LE_ERROR("mcc or mnc too long.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (specPtr->plmnValid)
    {
        LE_ERROR("PLMN already added to this specifier.");
        return LE_DUPLICATE;
    }

    le_utf8_Copy(specPtr->mcc, mcc, sizeof(specPtr->mcc), nullptr);
    le_utf8_Copy(specPtr->mnc, mnc, sizeof(specPtr->mnc), nullptr);
    specPtr->plmnValid = true;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds NTN bands to a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_AddSystemSelectionBands
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    const uint64_t* bandMask,                      ///< [IN] NTN band bitmask.
    size_t bandMaskSize                            ///< [IN] NTN band bitmask size.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandMask == nullptr)
    {
        LE_ERROR("bandMask is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandMaskSize < TAF_NTN_NTN_BAND_GROUP_NUM)
    {
        LE_ERROR("Invalid bandMaskSize %" PRIuS ".", bandMaskSize);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    for (uint32_t i = 0; i < TAF_NTN_NTN_BAND_GROUP_NUM; i++)
        specPtr->bandMask[i] = bandMask[i];

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds EARFCNs to a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_AddSystemSelectionEarfcns
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    const uint64_t* earfcns,                       ///< [IN] EARFCN list.
    size_t earfcnsSize                             ///< [IN] EARFCN list size.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (earfcns == nullptr)
    {
        LE_ERROR("earfcns is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (earfcnsSize > TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS)
    {
        LE_ERROR("Invalid earfcnsSize %" PRIuS ".", earfcnsSize);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    for (uint32_t i = 0; i < earfcnsSize; i++)
        specPtr->earfcns[i] = earfcns[i];
    specPtr->earfcnCount = (uint32_t)earfcnsSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first system selection specifier from the system selection list.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- List reference not found or no specifier exists.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetFirstSystemSelectionSpecifier
(
    taf_ntn_SystemSelectionListRef_t listRef,          ///< [IN] System selection list reference.
    taf_ntn_SystemSelectionSpecifierRef_t* specRefPtr  ///< [OUT] First specifier reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (specRefPtr == nullptr)
    {
        LE_ERROR("specRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_ref_Lookup(factory.maps.systemSelectionList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->specifierList));
    if (linkPtr == nullptr)
    {
        LE_ERROR("No specifier in list.");
        return LE_NOT_FOUND;
    }

    listPtr->currPtr = linkPtr;
    SystemSelectionSpecifier_t* specPtr = CONTAINER_OF(linkPtr, SystemSelectionSpecifier_t, link);

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.specifier, (void*)specPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    *specRefPtr = (taf_ntn_SystemSelectionSpecifierRef_t)safeRefPtr->safeRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next system selection specifier from the system selection list.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- List reference not found or no more specifier exists.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetNextSystemSelectionSpecifier
(
    taf_ntn_SystemSelectionListRef_t listRef,          ///< [IN] System selection list reference.
    taf_ntn_SystemSelectionSpecifierRef_t* specRefPtr  ///< [OUT] Next specifier reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (specRefPtr == nullptr)
    {
        LE_ERROR("specRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_ref_Lookup(factory.maps.systemSelectionList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->specifierList), listPtr->currPtr);
    if (linkPtr == nullptr)
        return LE_NOT_FOUND;

    listPtr->currPtr = linkPtr;
    SystemSelectionSpecifier_t* specPtr = CONTAINER_OF(linkPtr, SystemSelectionSpecifier_t, link);

    SafeRef_t* safeRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
    safeRefPtr->safeRef = le_ref_CreateRef(factory.maps.specifier, (void*)specPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    *specRefPtr = (taf_ntn_SystemSelectionSpecifierRef_t)safeRefPtr->safeRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets PLMN information from a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found or PLMN information was not added.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetSystemSelectionPlmn
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    char* mcc,                                     ///< [OUT] Mobile Country Code.
    size_t mccSize,                                ///< [IN] Mobile Country Code size.
    char* mnc,                                     ///< [OUT] Mobile Network Code.
    size_t mncSize                                 ///< [IN] Mobile Network Code size.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mcc == nullptr || mnc == nullptr)
    {
        LE_ERROR("mcc or mnc is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (!specPtr->plmnValid)
    {
        LE_ERROR("PLMN not added to this specifier.");
        return LE_NOT_FOUND;
    }

    le_utf8_Copy(mcc, specPtr->mcc, mccSize, nullptr);
    le_utf8_Copy(mnc, specPtr->mnc, mncSize, nullptr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the NTN bands from a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetSystemSelectionBands
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    uint64_t* bandMask,                            ///< [OUT] NTN band bitmask.
    size_t* bandMaskSizePtr                        ///< [INOUT] NTN band bitmask size.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (bandMask == nullptr || bandMaskSizePtr == nullptr)
    {
        LE_ERROR("bandMask or bandMaskSizePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (*bandMaskSizePtr < TAF_NTN_NTN_BAND_GROUP_NUM)
    {
        LE_ERROR("Invalid bandMaskSize %" PRIuS ".", *bandMaskSizePtr);
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    for (uint32_t i = 0; i < TAF_NTN_NTN_BAND_GROUP_NUM; i++)
        bandMask[i] = specPtr->bandMask[i];
    *bandMaskSizePtr = TAF_NTN_NTN_BAND_GROUP_NUM;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the EARFCNs from a system selection specifier.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Specifier reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_GetSystemSelectionEarfcns
(
    taf_ntn_SystemSelectionSpecifierRef_t specRef, ///< [IN] System selection specifier reference.
    uint64_t* earfcns,                             ///< [OUT] EARFCN list.
    size_t* earfcnsSizePtr                         ///< [INOUT] EARFCN list size.
)
{
    if (specRef == nullptr)
    {
        LE_ERROR("specRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (earfcns == nullptr || earfcnsSizePtr == nullptr)
    {
        LE_ERROR("earfcns or earfcnsSizePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionSpecifier_t* specPtr =
        (SystemSelectionSpecifier_t*)le_ref_Lookup(factory.maps.specifier, specRef);
    if (specPtr == nullptr)
    {
        LE_ERROR("specPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    if (*earfcnsSizePtr < specPtr->earfcnCount)
    {
        LE_ERROR("Output buffer too small: %" PRIuS " < %" PRIu32 ".", *earfcnsSizePtr,
            specPtr->earfcnCount);
        return LE_BAD_PARAMETER;
    }

    for (uint32_t i = 0; i < specPtr->earfcnCount; i++)
        earfcns[i] = specPtr->earfcns[i];
    *earfcnsSizePtr = specPtr->earfcnCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Writes the system selection specifier list to the modem for NTN service acquisition.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_WriteSystemSelectionList
(
    taf_ntn_SystemSelectionListRef_t listRef ///< [IN] System selection list reference.
)
{
    if (listRef == nullptr)
    {
        LE_ERROR("listRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    SystemSelectionList_t* listPtr =
        (SystemSelectionList_t*)le_ref_Lookup(factory.maps.systemSelectionList, listRef);
    if (listPtr == nullptr)
    {
        LE_ERROR("listPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    // Count specifiers to size the PA arrays.
    uint32_t count = 0;
    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->specifierList));
    while (linkPtr != nullptr)
    {
        count++;
        linkPtr = le_sls_PeekNext(&(listPtr->specifierList), linkPtr);
    }

    if (count == 0)
    {
        LE_ERROR("Empty system selection list.");
        return LE_BAD_PARAMETER;
    }

    std::vector<taf_pa_ntn_SystemSelectionSpecifier_t> paSpecs(count);
    // Backing storage for the band lists referenced by each PA specifier; kept alive until the
    // PA call returns.
    std::vector<std::vector<uint64_t>> bandStorage(count);

    uint32_t index = 0;
    linkPtr = le_sls_Peek(&(listPtr->specifierList));
    while (linkPtr != nullptr && index < count)
    {
        SystemSelectionSpecifier_t* specPtr =
            CONTAINER_OF(linkPtr, SystemSelectionSpecifier_t, link);
        taf_pa_ntn_SystemSelectionSpecifier_t& paSpec = paSpecs[index];
        memset(&paSpec, 0, sizeof(paSpec));

        if (specPtr->plmnValid)
        {
            le_utf8_Copy(paSpec.mcc, specPtr->mcc, sizeof(paSpec.mcc), nullptr);
            le_utf8_Copy(paSpec.mnc, specPtr->mnc, sizeof(paSpec.mnc), nullptr);
        }

        // Expand the API band bitmask into the PA list of band numbers. Bit (N-1) of word
        // floor((N-1)/64) set means band N is included.
        for (uint32_t word = 0; word < TAF_NTN_NTN_BAND_GROUP_NUM; word++)
        {
            uint64_t mask = specPtr->bandMask[word];
            for (uint32_t bit = 0; bit < 64; bit++)
            {
                if (mask & (((uint64_t)1) << bit))
                    bandStorage[index].push_back((uint64_t)(word * 64 + bit + 1));
            }
        }

        if (!bandStorage[index].empty())
        {
            paSpec.ntnBands = bandStorage[index].data();
            paSpec.ntnBandsLen = (uint32_t)bandStorage[index].size();
        }

        if (specPtr->earfcnCount > 0)
        {
            paSpec.ntnEarfcns = specPtr->earfcns;
            paSpec.ntnEarfcnsLen = specPtr->earfcnCount;
        }

        index++;
        linkPtr = le_sls_PeekNext(&(listPtr->specifierList), linkPtr);
    }

    taf_pa_result_t paResult = taf_pa_ntn_UpdateSystemSelectionSpecifiers(listPtr->instance,
        paSpecs.data(), count);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_UpdateSystemSelectionSpecifiers reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
// Location fix builder APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Creates a location fix object.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_CreateLocationFix
(
    taf_ntn_SessionRef_t sessionRef,     ///< [IN] NTN session reference.
    taf_ntn_LocationFixRef_t* locationRefPtr ///< [OUT] Location fix reference.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (locationRefPtr == nullptr)
    {
        LE_ERROR("locationRefPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    LocationFix_t* locationPtr = (LocationFix_t*)le_mem_ForceAlloc(factory.pools.locationFix);
    memset(locationPtr, 0, sizeof(LocationFix_t));
    locationPtr->instance = sessionPtr->instance;

    *locationRefPtr = (taf_ntn_LocationFixRef_t)le_ref_CreateRef(factory.maps.locationFix,
        (void*)locationPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes a location fix object.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_DeleteLocationFix
(
    taf_ntn_LocationFixRef_t locationRef ///< [IN] Location fix reference.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(factory.maps.locationFix, locationRef);
    le_mem_Release(locationPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets location coordinates.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OUT_OF_RANGE -- Coordinate is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetLocationCoordinates
(
    taf_ntn_LocationFixRef_t locationRef, ///< [IN] Location fix reference.
    double latitude,                      ///< [IN] Latitude in degrees.
    double longitude,                     ///< [IN] Longitude in degrees.
    double altitude,                      ///< [IN] Altitude above sea level.
    uint32_t horizontalUncertainty        ///< [IN] Horizontal uncertainty in meters.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
    {
        LE_ERROR("Coordinate out of range: lat=%f lon=%f.", latitude, longitude);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    locationPtr->fix.lat = (float)latitude;
    locationPtr->fix.lon = (float)longitude;
    locationPtr->fix.alt = (float)altitude;
    locationPtr->fix.uncerCircular = horizontalUncertainty;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets ENU velocity information.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Reference not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetLocationVelocityEnu
(
    taf_ntn_LocationFixRef_t locationRef, ///< [IN] Location fix reference.
    bool velocityValid,                   ///< [IN] True if ENU velocity is valid.
    double eastVelocity,                  ///< [IN] East velocity in meters per second.
    double northVelocity,                 ///< [IN] North velocity in meters per second.
    double upVelocity,                    ///< [IN] Up velocity in meters per second.
    bool uncertaintyValid,                ///< [IN] True if ENU velocity uncertainty is valid.
    double eastUncertainty,               ///< [IN] East velocity uncertainty in m/s.
    double northUncertainty,              ///< [IN] North velocity uncertainty in m/s.
    double upUncertainty                  ///< [IN] Up velocity uncertainty in m/s.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    taf_pa_ntn_VelocityInfo_t* velInfo = &locationPtr->fix.velInfo;
    velInfo->isEnuValueValid = velocityValid;
    velInfo->enuVel[0] = (float)eastVelocity;
    velInfo->enuVel[1] = (float)northVelocity;
    velInfo->enuVel[2] = (float)upVelocity;
    velInfo->isEnuUncerValid = uncertaintyValid;
    velInfo->enuUncer[0] = (float)eastUncertainty;
    velInfo->enuUncer[1] = (float)northUncertainty;
    velInfo->enuUncer[2] = (float)upUncertainty;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets heading information.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OUT_OF_RANGE -- Heading is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetLocationHeading
(
    taf_ntn_LocationFixRef_t locationRef, ///< [IN] Location fix reference.
    bool headingValid,                    ///< [IN] True if heading is valid.
    uint32_t headingDeg,                  ///< [IN] Heading in degrees.
    bool uncertaintyValid,                ///< [IN] True if heading uncertainty is valid.
    uint32_t headingUncertaintyDeg        ///< [IN] Heading uncertainty in degrees.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (headingValid && headingDeg > 360)
    {
        LE_ERROR("Heading out of range: %" PRIu32 ".", headingDeg);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    locationPtr->fix.isHeadingValid = headingValid;
    locationPtr->fix.heading = headingDeg;
    locationPtr->fix.isHeadingUncerValid = uncertaintyValid;
    locationPtr->fix.headingUncer = headingUncertaintyDeg;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets horizontal confidence information.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OUT_OF_RANGE -- Confidence is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SetLocationConfidence
(
    taf_ntn_LocationFixRef_t locationRef, ///< [IN] Location fix reference.
    bool confidenceValid,                 ///< [IN] True if confidence is valid.
    uint32_t confidence                   ///< [IN] Horizontal uncertainty confidence percentage.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (confidenceValid && confidence > 100)
    {
        LE_ERROR("Confidence out of range: %" PRIu32 ".", confidence);
        return LE_OUT_OF_RANGE;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_NOT_FOUND;
    }

    locationPtr->fix.isConfidenceValid = confidenceValid;
    locationPtr->fix.confidence = confidence;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Updates modem with location fix information.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNSUPPORTED -- Location assistance is not supported.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_UpdateLocationFix
(
    taf_ntn_LocationFixRef_t locationRef ///< [IN] Location fix reference.
)
{
    if (locationRef == nullptr)
    {
        LE_ERROR("locationRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    LocationFix_t* locationPtr = (LocationFix_t*)le_ref_Lookup(factory.maps.locationFix,
        locationRef);
    if (locationPtr == nullptr)
    {
        LE_ERROR("locationPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_SetLocationFix(locationPtr->instance, &locationPtr->fix);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_SetLocationFix reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides response to modem location fix request.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_PERMITTED -- Permission denied.
 *  - LE_UNSUPPORTED -- Location assistance is not supported.
 *  - LE_UNAVAILABLE -- NTN service is unavailable.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ntn_SendLocationFixResponse
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    taf_ntn_LocationStatus_t status, ///< [IN] Location fix request response status.
    uint64_t waitTimeMs              ///< [IN] Retry wait time in milliseconds.
)
{
    if (sessionRef == nullptr)
    {
        LE_ERROR("sessionRef is nullptr.");
        return LE_BAD_PARAMETER;
    }

    auto& factory = Factory::GetInstance();
    Session_t* sessionPtr = (Session_t*)le_ref_Lookup(factory.maps.session, sessionRef);
    if (sessionPtr == nullptr)
    {
        LE_ERROR("sessionPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_result_t paResult = taf_pa_ntn_LocationFixResponse(sessionPtr->instance,
        Utility::Convert::LocationStatus(status), waitTimeMs);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_LocationFixResponse reason=%d", paResult);
    }

    return Utility::Convert::Result(paResult);
}

//--------------------------------------------------------------------------------------------------
// Event handler add/remove APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for NTN state change.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_StateChangeHandlerRef_t taf_ntn_AddStateChangeHandler
(
    taf_ntn_StateChangeHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                             ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("StateChange",
        factory.events.stateChange, Utility::LayeredFunction::StateChange, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_StateChangeHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for NTN state change.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveStateChangeHandler
(
    taf_ntn_StateChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for NTN capability change.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_CapabilitiesChangeHandlerRef_t taf_ntn_AddCapabilitiesChangeHandler
(
    taf_ntn_CapabilitiesChangeHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                                    ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("CapabilitiesChange",
        factory.events.capabilitiesChange, Utility::LayeredFunction::CapabilitiesChange,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_CapabilitiesChangeHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for NTN capability change.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveCapabilitiesChangeHandler
(
    taf_ntn_CapabilitiesChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for NTN data acknowledgement.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_DataAckHandlerRef_t taf_ntn_AddDataAckHandler
(
    taf_ntn_DataAckHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                         ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DataAck",
        factory.events.dataAck, Utility::LayeredFunction::DataAck, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_DataAckHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for NTN data acknowledgement.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveDataAckHandler
(
    taf_ntn_DataAckHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for incoming NTN data.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_IncomingDataHandlerRef_t taf_ntn_AddIncomingDataHandler
(
    taf_ntn_IncomingDataHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                              ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("IncomingData",
        factory.events.incomingData, Utility::LayeredFunction::IncomingData, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_IncomingDataHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for incoming NTN data.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveIncomingDataHandler
(
    taf_ntn_IncomingDataHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for terrestrial cellular coverage.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_CellularCoverageHandlerRef_t taf_ntn_AddCellularCoverageHandler
(
    taf_ntn_CellularCoverageHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                                  ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("CellularCoverage",
        factory.events.cellularCoverage, Utility::LayeredFunction::CellularCoverage,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_CellularCoverageHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for terrestrial cellular coverage.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveCellularCoverageHandler
(
    taf_ntn_CellularCoverageHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for location fix request.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_LocationFixRequestHandlerRef_t taf_ntn_AddLocationFixRequestHandler
(
    taf_ntn_LocationFixRequestHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                                    ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("LocationFixRequest",
        factory.events.locationFixRequest, Utility::LayeredFunction::LocationFixRequest,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_LocationFixRequestHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for location fix request.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveLocationFixRequestHandler
(
    taf_ntn_LocationFixRequestHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler for NTN band update.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_BandUpdateHandlerRef_t taf_ntn_AddBandUpdateHandler
(
    taf_ntn_BandUpdateHandlerFunc_t handlerPtr, ///< [IN] Handler function.
    void* contextPtr                            ///< [IN] Handler context.
)
{
    auto& factory = Factory::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("BandUpdate",
        factory.events.bandUpdate, Utility::LayeredFunction::BandUpdate, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ntn_BandUpdateHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a handler for NTN band update.
 */
//--------------------------------------------------------------------------------------------------
void taf_ntn_RemoveBandUpdateHandler
(
    taf_ntn_BandUpdateHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
