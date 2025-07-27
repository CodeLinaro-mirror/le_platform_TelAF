/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsSvc.cpp
 * @brief TelAF Data Call Service external APIs implementation
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDcs.hpp"
#include "tafDcsProfile.hpp"
#include "tafDcsUtils.hpp"
#include "tafSvcIF.hpp"

using namespace taf::svc::datacall;

//--------------------------------------------------------------------------------------------------
/**
 * Get list of profiles for the specified slot ID.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t  taf_dcs_GetProfilesList
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    taf_dcs_ProfileInfo_t* profilesListPtr,
        ///< [OUT] The details of profiles.
    size_t* profilesListSizePtr
        ///< [INOUT]
)
{

    TAF_ERROR_IF_RET_VAL(nullptr == profilesListPtr    , LE_BAD_PARAMETER,
                                                                "profilesListPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profilesListSizePtr, LE_BAD_PARAMETER,
                                                                "profilesListSizePtr is NULL");
    TAF_ERROR_IF_RET_VAL(0 == *profilesListSizePtr     , LE_BAD_PARAMETER,
                                                                "profilesListSizePtr value is 0");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfilesList(phoneId, profilesListPtr, profilesListSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the profile reference for the given phone ID and profile ID.
 *
 * @return
 *  - NULL -- Error.
 *  - Others -- The profile reference.
 */
//--------------------------------------------------------------------------------------------------
taf_dcs_ProfileRef_t taf_dcs_GetProfileRef
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    uint32_t profileId
        ///< [IN] The profile ID. <br>
        ///< Pass #TAF_DCS_UNDEFINED_PROFILE_ID to indicate the profile will be
        ///< created by calling taf_dcs_CreateProfile().
        ///< The API sets #TAF_DCS_TECH_3GPP and #TAF_DCS_PDP_IPV4 for technology
        ///< and PDP respectively.<br>
        ///< After creating the profile, clients can call taf_dcs_GetProfileId()
        ///< to get the created profile's ID.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfileRef(phoneId, profileId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the profile ID for the given profile reference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetProfileId
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint32_t* profileIdPtr
        ///< [OUT] The profile ID. <br>
        ///< #TAF_DCS_UNDEFINED_PROFILE_ID indicates the profile is not yet
        ///< created.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfileId(profileRef, profileIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the phone ID for the given profile reference.
 *
 * @return
 *  - LE_OK            -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND     -- Profile reference was not found.
 *  - LE_FAULT         -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetPhoneId
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint8_t* phoneIdPtr
        ///< [OUT] The phone ID.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetPhoneId(profileRef, phoneIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile PDP type.
 *
 * @return
 *  - TAF_DCS_PDP_UNKNOWN -- Unknown.
 *  - TAF_DCS_PDP_IPV4 -- IPV4.
 *  - TAF_DCS_PDP_IPV6 -- IPV6.
 *  - TAF_DCS_PDP_IPV4V6 -- IPV4 and IPV6.
 */
//--------------------------------------------------------------------------------------------------
taf_dcs_Pdp_t taf_dcs_GetPDP
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                TAF_DCS_PDP_UNKNOWN, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    taf_dcs_Pdp_t pdp;
    le_result_t result = tafDcsProfileManager.SvcGetPDP(profileRef, pdp);
    if (LE_OK != result)
    {
        LE_WARN("Failed to get PDP type for profile %d", result);
        return TAF_DCS_PDP_UNKNOWN;
    }
    return pdp;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile APN name.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_OVERFLOW -- Apn size is smaller than APN_NAME_MAX_BYTES.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetAPN
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* apnName,
        ///< [OUT] The APN name.
    size_t apnNameSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetAPN(profileRef, apnName, apnNameSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts a synchronous data cellular session for the given profile reference.
 *
 * @return
 *  - LE_OK           -- Succeeded.
 *  - LE_NOT_FOUND    -- Parameter is invalid or couldn't create the data call.
 *  - LE_OUT_OF_RANGE -- PDP type is unknown.
 *  - LE_DUPLICATE    -- If the data session is already connected for the given profile.
 *  - LE_IN_PROGRESS  -- The profile is in use and a data call is in progress.
 *  - LE_TIMEOUT      -- Timed out when attempting to make the data call.
 *  - LE_NOT_POSSIBLE -- Data profile is not yet created.
 *  - LE_FAULT        -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_StartSession
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcStartSessionSync(profileRef, taf_dcs_GetClientSessionRef());
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts an asynchronous data cellular session for the given profile reference.
 *
 **/
//--------------------------------------------------------------------------------------------------
void taf_dcs_StartSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
        ///< [IN] The handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    if (taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState())
    {
        LE_ERROR("Service not initialized.");
    }
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    tafDcsProfileManager.SvcStartSessionASync(profileRef, handlerPtr, contextPtr,
                                                                    taf_dcs_GetClientSessionRef());
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops (synchronously) a cellular data session for the given profile reference.
 *
 * @return
 *  - LE_OK            -- Succeeded.
 *  - LE_NOT_FOUND     -- Can't find a data call.
 *  - LE_OUT_OF_RANGE  -- PdpType type is unknown.
 *  - LE_BAD_PARAMETER -- Call context is NULL.
 *  - LE_TIMEOUT       -- Timeout to stop a data call.
 *  - LE_IN_PROGRESS   -- The profile is in use and a data call is in progress.
 *  - LE_NOT_POSSIBLE  -- Data profile is not yet created.
 *  - LE_FAULT         -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_StopSession
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcStopSessionSync(profileRef, taf_dcs_GetClientSessionRef());
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops an asynchronous data cellular session for the given profile reference.
 *
 **/
//--------------------------------------------------------------------------------------------------

void taf_dcs_StopSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
        ///< [IN] The handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    if (taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState())
    {
        LE_ERROR("Service not initialized.");
    }
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    tafDcsProfileManager.SvcStopSessionASync(profileRef, handlerPtr, contextPtr,
                                                                    taf_dcs_GetClientSessionRef());
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the data session's current state.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_NOT_POSSIBLE -- Data profile is not yet created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetSessionState
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
        taf_dcs_ConState_t* connectionStatePtr
        ///< [OUT] The connection state.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetSessionState(profileRef, connectionStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the profile reference for the given phone ID and profile ID.
 *
 * @return
 *  - NULL -- Error.
 *  - Others -- The profile reference.
 */
//--------------------------------------------------------------------------------------------------
taf_dcs_ProfileRef_t taf_dcs_GetProfileEx
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    uint32_t profileId
        ///< [IN] The profile ID. <br>
        ///< Pass #TAF_DCS_UNDEFINED_PROFILE_ID to indicate the profile will be
        ///< created by calling taf_dcs_CreateProfile().
        ///< The API sets #TAF_DCS_TECH_3GPP and #TAF_DCS_PDP_IPV4 for technology
        ///< and PDP respectively.<br>
        ///< After creating the profile, clients can call taf_dcs_GetProfileId()
        ///< to get the created profile's ID.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfileRef(phoneId, profileId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the profile reference for the given profile index.
 *
 * @b NOTE: For Dual Sim-Single Active (DSSA) mode and Dual Sim-Dual Active (DSDA) mode, this API
 * gets the profile reference with phone ID 1.
 *
 * @b NOTE: For creating profiles, use taf_dcs_GetProfileEx() followed by taf_dcs_CreateProfile().
 *
 * @return
 *  - NULL -- Error.
 *  - Others -- The profile reference.
 */
//--------------------------------------------------------------------------------------------------
taf_dcs_ProfileRef_t taf_dcs_GetProfile
(
    uint32_t profileId
        ///< [IN] The profile index.
)
{
    LE_WARN("This API is deprecated. Use taf_dcs_GetProfileEx().");
    LE_WARN("Get reference with phone ID 1.");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    // Get the default phone ID
    uint8_t localDefPhoneId = 0;
    uint32_t localDefProfileId = 0;
    le_result_t result = taf_dcs_GetDefaultPhoneIdAndProfileId(&localDefPhoneId,&localDefProfileId);
    // Check return value
    TAF_ERROR_IF_RET_VAL(LE_OK != result, nullptr,
                                        "taf_dcs_GetDefaultPhoneIdAndProfileId failed: %d", result);
    LE_INFO("Calling taf_dcs_GetProfileEx with phone ID: %d", localDefPhoneId);

    return taf_dcs_GetProfileEx(localDefPhoneId, profileId);
}

le_result_t taf_dcs_GetRoamingStatus
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    bool* isRoamingPtr,
        ///< [OUT] True means that roaming is on; False means that
    taf_dcs_RoamingType_t* typePtr
        ///< [OUT] The roaming type. Valid only if roaming is on.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetRoamingStatus(phoneId, isRoamingPtr, typePtr);
}

le_result_t taf_dcs_GetDefaultPhoneIdAndProfileId
(
    uint8_t* phoneIdPtr,
        ///< [OUT] The phone ID.
    uint32_t* profileIdPtr
        ///< [OUT] The profile index.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetDefaultPhoneIdAndProfileId(phoneIdPtr, profileIdPtr);
}

le_result_t taf_dcs_CreateProfile
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcCreateProfile(profileRef);
}

le_result_t taf_dcs_DeleteProfile
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcDeleteProfile(profileRef);
}

le_result_t taf_dcs_SetAPN
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    const char* LE_NONNULL apnStr
        ///< [IN] The APN name.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetAPN(profileRef, apnStr);
}

le_result_t taf_dcs_SetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_ApnType_t apnType
        ///< [IN] The APN type.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetApnTypes(profileRef, apnType);
}

le_result_t taf_dcs_SetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    const char* LE_NONNULL nameStr
        ///< [IN] The profile name.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetProfileName(profileRef, nameStr);
}

uint32_t taf_dcs_GetDefaultProfileIndex
(
    void
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            0, "Service not initialized.");

    uint8_t phoneId = 0;
    uint32_t profileId = 0;
    // Get the default phone id and profile id.
    le_result_t result = taf_dcs_GetDefaultPhoneIdAndProfileId(&phoneId, &profileId);
    // Return 0 in case of error
    TAF_ERROR_IF_RET_VAL(LE_OK != result, 0, "taf_dcs_GetDefaultPhoneIdAndProfileId failed: %d",
                                                                                            result);
    LE_DEBUG("Phone: %d, Profile: %d", phoneId, profileId);
    return profileId;
}

le_result_t taf_dcs_GetDefaultProfileIndexEx
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    uint32_t* profileIdPtr
        ///< [OUT] The profile index.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetDefaultProfileIndexEx(phoneId, profileIdPtr);
}

le_result_t taf_dcs_GetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_ApnType_t* apnTypePtr
        ///< [OUT] The APN type.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetApnTypes(profileRef, apnTypePtr);
}

le_result_t taf_dcs_SetPDP
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Pdp_t pdp
        ///< [IN] The PDP type.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetPDP(profileRef, pdp);
}

uint32_t taf_dcs_GetProfileIndex
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    uint32_t profileId = 0;
    le_result_t result = taf_dcs_GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, 0, "taf_dcs_GetProfileId failed: %d", result);

    return profileId;
}

le_result_t taf_dcs_GetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* name,
        ///< [OUT] The profile name.
    size_t nameSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfileName(profileRef, name, nameSize);
}

le_result_t taf_dcs_SetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t techPref
        ///< [IN] The technology preference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetTechPreference(profileRef, techPref);
}

le_result_t taf_dcs_GetPhoneIdByInterfaceName
(
    const char* LE_NONNULL ifName,
        ///< [IN] The interface name.
    uint8_t* phoneIdPtr
        ///< [OUT] The phone ID.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetPhoneIdByInterfaceName(ifName, phoneIdPtr);
}

le_result_t taf_dcs_GetProfileIdByInterfaceName
(
    const char* LE_NONNULL ifName,
        ///< [IN] The interface name.
    uint32_t* profileIdPtr
        ///< [OUT] The profile index.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfileIdByInterfaceName(ifName, profileIdPtr);
}

le_result_t taf_dcs_GetInterfaceName
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* ifName,
        ///< [OUT] The interface name.
    size_t ifNameSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetInterfaceName(profileRef, ifName, ifNameSize);
}

le_result_t taf_dcs_GetMtu
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint16_t* mtuPtr
        ///< [OUT] The MTU value.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetMtu(profileRef, mtuPtr);
}

le_result_t taf_dcs_GetIPv4GatewayAddress
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* gatewayAddr,
        ///< [OUT] The IPv4 gateway address.
    size_t gatewayAddrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv4GatewayAddress(profileRef, gatewayAddr,gatewayAddrSize);
}

le_result_t taf_dcs_GetIPv6GatewayAddress
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* gatewayAddr,
        ///< [OUT] The IPv6 gateway address.
    size_t gatewayAddrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv6GatewayAddress(profileRef, gatewayAddr, gatewayAddrSize);
}

le_result_t taf_dcs_GetIPv4SubnetMask
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint32_t* maskPtr
        ///< [OUT] The IPv4 mask.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv4SubnetMask(profileRef, maskPtr);
}

le_result_t taf_dcs_GetIPv6SubnetMask
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint32_t* maskPtr
        ///< [OUT] The IPv6 mask.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv6SubnetMask(profileRef, maskPtr);
}

le_result_t taf_dcs_GetProfileListEx
(
    uint8_t phoneId,
        ///< [IN] The phone ID.
    taf_dcs_ProfileInfo_t* profileListPtr,
        ///< [OUT] The profile list information.
    size_t* profileListSizePtr
        ///< [INOUT]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileListPtr, LE_BAD_PARAMETER, "profileListPtr is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetProfilesList(phoneId, profileListPtr, profileListSizePtr);
}

le_result_t taf_dcs_GetProfileList
(
    taf_dcs_ProfileInfo_t* profileListPtr,
        ///< [OUT] The profile list information.
    size_t* profileListSizePtr
        ///< [INOUT]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    // Call taf_dcs_GetProfileListEx with phone ID 1.
    return taf_dcs_GetProfileListEx(TAF_DCS_DEFAULT_PHONE_ID, profileListPtr, profileListSizePtr);
}


le_result_t taf_dcs_GetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t* techPrefPtr
        ///< [OUT] The technology preference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetTechPreference(profileRef, techPrefPtr);
}

le_result_t taf_dcs_GetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Auth_t* authPtr,
        ///< [OUT] The authentication type.
    char* userName,
        ///< [OUT] The username.
    size_t userNameSize,
        ///< [IN]
    char* password,
        ///< [OUT] The password.
    size_t passwordSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetAuthentication(profileRef, authPtr, userName, userNameSize,
                                                                            password, passwordSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the connected data session IPv4 address.
 *
 * @return
 *  - LE_OK           -- Succeeded.
 *  - LE_NOT_FOUND    -- Failed.
 *  - LE_NOT_POSSIBLE -- Data profile is not yet created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetIPv4Address
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* ipAddr,
        ///< [OUT] The IPv4 address.
    size_t ipAddrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");


    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv4Address(profileRef, ipAddr, ipAddrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the connected data session IPv6 address.
 *
 * @return
 *  - LE_OK           -- Succeeded.
 *  - LE_NOT_FOUND    -- Failed.
 *  - LE_NOT_POSSIBLE -- Data profile is not yet created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetIPv6Address
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* ipAddr,
        ///< [OUT] The IPv6 address.
    size_t ipAddrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv6Address(profileRef, ipAddr, ipAddrSize);
}

le_result_t taf_dcs_GetIPv4DNSAddresses
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* dns1AddrStr,
        ///< [OUT] The IPv4 primary DNS address.
    size_t dns1AddrStrSize,
        ///< [IN]
    char* dns2AddrStr,
        ///< [OUT] The IPv4 secondary DNS address.
    size_t dns2AddrStrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv4DNSAddresses(profileRef, dns1AddrStr, dns1AddrStrSize,
                                                                    dns2AddrStr, dns2AddrStrSize);
}

le_result_t taf_dcs_GetIPv6DNSAddresses
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* dns1AddrStr,
        ///< [OUT] The IPv6 primary DNS address.
    size_t dns1AddrStrSize,
        ///< [IN]
    char* dns2AddrStr,
        ///< [OUT] The IPv6 secondary DNS address.
    size_t dns2AddrStrSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetIPv6DNSAddresses(profileRef, dns1AddrStr, dns1AddrStrSize,
                                                       dns2AddrStr, dns2AddrStrSize);
}

le_result_t taf_dcs_SetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Auth_t auth,
        ///< [IN] The authentication type.
    const char* LE_NONNULL userName,
        ///< [IN] The username.
    const char* LE_NONNULL password
        ///< [IN] The password.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetAuthentication(profileRef, auth, userName, password);
}

le_result_t taf_dcs_GetAPNThrottledPLMN
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    bool* areAllPLMNsThrottledPtr,
        ///< [OUT] True if APN is throttled on all PLMNs.
    char* mcc,
        ///< [OUT] MCC of the PLMN on which the APN is throttled.
    size_t mccSize,
        ///< [IN]
    char* mnc,
        ///< [OUT] MNC of the PLMN on which the APN is throttled.
    size_t mncSize
        ///< [IN]
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetAPNThrottledPLMN(profileRef, areAllPLMNsThrottledPtr, mcc,
                                                       mccSize, mnc, mncSize);
}


le_result_t taf_dcs_SetDefaultProfileIndexEx
(
    uint8_t phoneId,
    ///< [IN] The phone ID.
    uint32_t profileId
    ///< [IN] The profile index.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    // Check what the current default phone ID and profile ID are
    uint8_t  localDefPhoneId   = 0;
    uint32_t localDefProfileId = 0;
    le_result_t result = taf_dcs_GetDefaultPhoneIdAndProfileId(&localDefPhoneId,&localDefProfileId);
    // Check return value
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"taf_dcs_GetDefaultPhoneIdAndProfileId failed: %d",
                                                                                        result);
    if (localDefPhoneId == phoneId && localDefProfileId == profileId)
    {
        LE_DEBUG("No changes needed.");
        return LE_OK;
    }
    // Set the new default phone ID and profile ID
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcSetDefaultProfileIndexEx(phoneId, profileId);
}

le_result_t taf_dcs_SetDefaultProfileIndex
(
    uint32_t profileId
        ///< [IN] The profile index.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");

    uint8_t defPhoneId = 0;
    uint32_t localProfileId = 0;
    // Get the default phone id and profile id.
    le_result_t result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defPhoneId, &localProfileId);
    // Check return value
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"taf_dcs_GetDefaultPhoneIdAndProfileId failed: %d",
                                                                                        result);
    LE_DEBUG("Using default Phone ID: %d", defPhoneId);

    // Call taf_dcs_SetDefaultProfileIndexEx with the default phone id and user provided profile id.
    return taf_dcs_SetDefaultProfileIndexEx(defPhoneId, profileId);
}

bool taf_dcs_IsIPv4
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                                false, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcIsIPv4(profileRef);
}

bool taf_dcs_IsIPv6
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         false, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcIsIPv6(profileRef);
}

le_result_t taf_dcs_GetMaxDataBitRates
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint64_t* maxRxBitRatePtr,
        ///< [OUT] The maximum receive data rate in bits/second.
    uint64_t* maxTxBitRatePtr
        ///< [OUT] The maximum transmit data rate in bits/second.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetMaxDataBitRates(profileRef, maxRxBitRatePtr, maxTxBitRatePtr);
}

le_result_t taf_dcs_GetAPNThrottledStatus
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    bool* isThrottledPtr,
        ///< [OUT] True when APN is throttled. False when APN is unthrottled.
    uint32_t* ipv4RemainingTimePtr,
        ///< [OUT] The remaining IPv4 throttled time in milliseconds.
    uint32_t* ipv6RemainingTimePtr
        ///< [OUT] The remaining IPv6 throttled time in milliseconds.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetAPNThrottledStatus(profileRef, isThrottledPtr,
                                                        ipv4RemainingTimePtr, ipv6RemainingTimePtr);
}

le_result_t taf_dcs_GetDataBearerTechnology
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_DataBearerTechnology_t* dlDataBearerTechPtrPtr,
        ///< [OUT] The downlink data bearer technology.
    taf_dcs_DataBearerTechnology_t* ulDataBearerTechPtrPtr
        ///< [OUT] The uplink data bearer technology.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetDataBearerTechnology(profileRef, dlDataBearerTechPtrPtr,
                                                                            ulDataBearerTechPtrPtr);
}

le_result_t taf_dcs_GetCallEndReason
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Pdp_t pdpType,
        ///< [IN] The packet data protocol type.
    taf_dcs_CallEndReasonType_t* callEndReasonTypePtr,
        ///< [OUT] The call end reason type.
    int32_t* callEndReasonCodePtr
        ///< [OUT] The call end reason.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcGetCallEndReason(profileRef, pdpType,
                                                    callEndReasonTypePtr, callEndReasonCodePtr);
}

le_result_t taf_mdc_StartSession
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The taf_dcs profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    // Start session with 0 as session reference so that the service will maintain the connection
    // even if the calling client disconnects.
    return tafDcsProfileManager.SvcStartSessionSync(profileRef, 0);
}

le_result_t taf_mdc_StartSessionAsync
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The taf_dcs profile reference.
)
{
    return taf_mdc_StartSession(profileRef);
}

le_result_t taf_mdc_StopSession
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The taf_dcs profile reference.
)
{
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    // Stop session with 0 as session reference which was used when starting the session.
    return tafDcsProfileManager.SvcStopSessionSync(profileRef, 0);
}

le_result_t taf_mdc_StopSessionAsync
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The taf_dcs profile reference.
)
{
    return taf_mdc_StopSession(profileRef);
}


le_result_t taf_dcs_GetQosProfile
(
    taf_dcs_QosFlowRef_t qosFlowRef,
        ///< [IN] The QOS flow reference.
    taf_dcs_ProfileRef_t* profileRefPtr
        ///< [OUT]
)
{
    LE_UNUSED(qosFlowRef);
    LE_UNUSED(profileRefPtr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    return LE_UNSUPPORTED;
}

le_result_t taf_dcs_GetQosId
(
    taf_dcs_QosFlowRef_t qosFlowRef,
        ///< [IN] The QOS flow reference.
    uint32_t* qosFlowIdPtr
        ///< [OUT] QOS ID.
)
{
    LE_UNUSED(qosFlowRef);
    LE_UNUSED(qosFlowIdPtr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                                                            LE_FAULT, "Service not initialized.");
    return LE_UNSUPPORTED;
}

le_result_t taf_dcs_GetQosParameterMask
(
    taf_dcs_QosFlowRef_t qosFlowRef,
        ///< [IN] The QOS flow reference.
    taf_dcs_QosFlowBitMask_t* qosFlowMaskPtr
        ///< [OUT] QOS flow bitmask.
)
{
    LE_UNUSED(qosFlowRef);
    LE_UNUSED(qosFlowMaskPtr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE != tafDcsSvc.GetInitState(),
                         LE_FAULT, "Service not initialized.");
    return LE_UNSUPPORTED;
}


/**
 * Add a roaming state handler to monitor the roaming status.
 *
 */
taf_dcs_RoamingStatusHandlerRef_t taf_dcs_AddRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == handlerPtr, nullptr, "handlerPtr is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcAddRoamingStatusHandler(nullptr, handlerPtr, contextPtr);
}

/**
 * Remove roaming state handler.
 *
 */
void taf_dcs_RemoveRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerRef_t handlerRef
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcRemoveRoamingStatusHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_dcs_SessionState'
 *
 * The data call session event.
 */
//--------------------------------------------------------------------------------------------------
taf_dcs_SessionStateHandlerRef_t taf_dcs_AddSessionStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_SessionStateHandlerFunc_t handlerPtr,
        ///< [IN] The event handler reference.
    void* contextPtr
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == handlerPtr, nullptr, "handlerPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, nullptr, "profileRef is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcAddSessionStateHandler(profileRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_dcs_SessionState'
 */
//--------------------------------------------------------------------------------------------------
void taf_dcs_RemoveSessionStateHandler
(
    taf_dcs_SessionStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcRemoveSessionStateHandler(handlerRef);
}

taf_dcs_QosStatusHandlerRef_t taf_dcs_AddQosStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_QosStatusHandlerFunc_t handlerPtr,
        ///< [IN] Handler for QOS flow status.
    void* contextPtr
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == handlerPtr, nullptr, "handlerPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, nullptr, "profileRef is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcAddQosStatusHandler(profileRef, handlerPtr, contextPtr);
}

void taf_dcs_RemoveQosStatusHandler
(
    taf_dcs_QosStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcRemoveQosStatusHandler(handlerRef);
}

taf_dcs_HwAccelerationStateHandlerRef_t taf_dcs_AddHwAccelerationStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_HwAccelerationStateHandlerFunc_t handlerPtr,
        ///< [IN] Handler for hardware acceleration state.
    void* contextPtr
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == handlerPtr, nullptr, "handlerPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, nullptr, "profileRef is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcAddHwAccelerationStateHandler(profileRef, handlerPtr, contextPtr);
}

void taf_dcs_RemoveHwAccelerationStateHandler
(
    taf_dcs_HwAccelerationStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcRemoveHwAccelerationStateHandler(handlerRef);
}

taf_dcs_ThrottledStatusHandlerRef_t taf_dcs_AddThrottledStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_ThrottledStatusHandlerFunc_t handlerPtr,
        ///< [IN] Handler for throttled status.
    void* contextPtr
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == handlerPtr, nullptr, "handlerPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, nullptr, "profileRef is NULL");

    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(taf::pa::data::InitState_e::INIT_DONE!= tafDcsSvc.GetInitState(),
                                                            nullptr, "Service not initialized.");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcAddThrottledStatusHandler(profileRef, handlerPtr, contextPtr);
}

void taf_dcs_RemoveThrottledStatusHandler
(
    taf_dcs_ThrottledStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL");

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    return tafDcsProfileManager.SvcRemoveThrottledStatusHandler(handlerRef);
}