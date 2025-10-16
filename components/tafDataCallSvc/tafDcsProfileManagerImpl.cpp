/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsProfileManagerImpl.cpp
 * @brief TelAF Data Call Service's profile manager class(TafDcsProfileManager) implementation.
 *
 */

#include "tafDcs.hpp"
#include "tafDcsProfile.hpp"
#include "tafDcsUtils.hpp"
#include "tafSvcIF.hpp"

// Headers for getting MTU
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <algorithm>
#include <chrono>

using namespace taf::svc::datacall;

/**
 * Get a reference to TafDcsProfile object that matches profileRef
 * The macro provides "profile" reference that can be used with subsequent TafDcsProfile APIs.
 * @return retVal passed by the caller
 */
#define GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, retVal)       \
    auto profileOptWrapper = getProfile(profileRef);               \
    TAF_ERROR_IF_RET_VAL((!profileOptWrapper.has_value()), retVal, \
                         "profileRef %p not found", profileRef);   \
    TafDcsProfile &profile = profileOptWrapper.value().get();

/**
 * Get a reference to TafDcsProfile object that matches profileRef
 * The macro provides "profile" reference that can be used with subsequent TafDcsProfile APIs.
 * @return None
 */
#define GET_DCS_PROFILE_FROM_REF_RET_NIL(profileRef, retVal)       \
    auto profileOptWrapper = getProfile(profileRef);               \
    TAF_ERROR_IF_RET_NIL((!profileOptWrapper.has_value()),         \
                         "profileRef %p not found", profileRef);   \
    TafDcsProfile &profile = profileOptWrapper.value().get();

/**
 * Get a reference to TafDcsProfile object that matches phone ID and profile ID
 * The macro provides "profile" reference that can be used with subsequent TafDcsProfile APIs.
 * @return val passed by the caller
 */
#define GET_DCS_PROFILE_FROM_ID_RET_VAL(phoneId, profileId, retVal)       \
    auto profileOptWrapper = getProfile(phoneId, profileId);              \
    TAF_ERROR_IF_RET_VAL((!profileOptWrapper.has_value()), retVal,        \
                         "profile[%d,%d] not found", phoneId, profileId); \
    TafDcsProfile &profile = profileOptWrapper.value().get();

/**
 * Get a reference to TafDcsProfile object that matches phone ID and profile ID
 * The macro provides "profile" reference that can be used with subsequent TafDcsProfile APIs.
 * @return None
 */
#define GET_DCS_PROFILE_FROM_ID_RET_NIL(phoneId, profileId)               \
    auto profileOptWrapper = getProfile(phoneId, profileId);              \
    TAF_ERROR_IF_RET_NIL((!profileOptWrapper.has_value()),                \
                         "profile[%d,%d] not found", phoneId, profileId); \
    TafDcsProfile &profile = profileOptWrapper.value().get();

/**
 * Check if a profile is created or not by checking the profile ID.
 */
#define TAF_CHECK_IF_PROFILE_IS_CREATED(profile)                                                 \
    do                                                                                           \
    {                                                                                            \
        uint32_t profileId;                                                                      \
        le_result_t result = profile.GetId(profileId);                                           \
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "Failed to get profile id");               \
        TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,         \
                                                                    "Profile not created yet."); \
    } while (0);

/**
 * Check if a profile is created or not by checking the profile ID and return false
 */
#define TAF_CHECK_IF_PROFILE_IS_CREATED_RET_FALSE(profile)                               \
    do                                                                                   \
    {                                                                                    \
        uint32_t profileId;                                                              \
        le_result_t result = profile.GetId(profileId);                                   \
        TAF_ERROR_IF_RET_VAL(LE_OK != result, false, "Failed to get profile id");        \
        TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, false,           \
                                                            "Profile not created yet."); \
    } while (0);

/**
 * Check if a profile is created or not by checking the profile ID.
 */
#define TAF_CHECK_IF_PROFILE_IS_CREATED_RET_VAL(profile, retVal)                                 \
    do                                                                                           \
    {                                                                                            \
        uint32_t profileId;                                                                      \
        le_result_t result = profile.GetId(profileId);                                           \
        TAF_ERROR_IF_RET_VAL(LE_OK != result, retVal, "Failed to get profile id");               \
        TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, retVal,                  \
                                                                    "Profile not created yet."); \
    } while (0);

/**************************************************************************************************/
// Public functions.
/**************************************************************************************************/
// Get the singleton instance
TafDcsProfileManager &TafDcsProfileManager::GetInstance()
{
    static TafDcsProfileManager instance;
    return instance;
}

// Initialize the profile manager by registering events and callbacks
void TafDcsProfileManager::Init()
{
    // Register callbacks for internal events. The callback registrations are done in the context of
    // the events thread, tafDcsEventsThreadRef_ (tafDcsEventsThread). So wait until the init
    // is completed.
    LE_INFO("Register internal event callbacks.");

    eventThreadInitPromise_ = std::promise<void>();
    std::future fut = eventThreadInitPromise_.get_future();

    registerInternalEventCallbacks();

    // Wait for the internal event callback registrations to complete for 5 seconds.
    if (fut.wait_for(std::chrono::seconds(5)) == std::future_status::ready)
    {
        LE_INFO("Registration of internal event callbacks done.");
    }
    else
    {
        // This should not happen.
        LE_FATAL("Registration of internal events timed out!");
    }

    // Register PA callbacks
    registerPACallbacks();

    LE_DEBUG("Profile manager init done.");
}

void TafDcsProfileManager::InitProfiles()
{
    // Get the phone IDs
    le_result_t result = taf::pa::data::GetPhoneIds(phoneIds_);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get PhoneIds. result: %d. Setting to 1.", result);
        // Set it to 1
        phoneIds_.push_back(taf::pa::data::PhoneId_e::PHONE_1);
    }
    LE_INFO("Number of phones: %zu", phoneIds_.size());

    // Initialize profiles by reading from the NAD
    initProfiles();

    // Update default profiles
    updateDefaultProfiles();
}

void TafDcsProfileManager::Deinit()
{
    deinitEventsAndMemory();
    deinitProfiles();
    deregisterPACallbacks();
}

// Check if the provided phone ID is valid based on current configuration.
bool TafDcsProfileManager::isPhoneIdValid(taf::pa::data::PhoneId_e phoneId) const
{
    // Check if the provided phone ID is present in the valid phone IDs list.
    return std::find(phoneIds_.begin(), phoneIds_.end(), phoneId) != phoneIds_.end();
}

// Initialize the profile manager
le_result_t TafDcsProfileManager::SvcGetProfilesList
(
    uint8_t phoneId,
    taf_dcs_ProfileInfo_t *profilesListPtr,
    size_t *profilesListSizePtr
)
{
    // Verify validity of provided phone ID.
    TAF_ERROR_IF_RET_VAL(TAF_TYPES_PHONE_ID_1 != phoneId && TAF_TYPES_PHONE_ID_2 != phoneId,
                         LE_BAD_PARAMETER, "Unsupported phoneId %d", phoneId);

    // Check if the provided phone ID is supported based on current configuration.
    if (!isPhoneIdValid(static_cast<taf::pa::data::PhoneId_e>(phoneId)))
    {
        LE_WARN ("Phone ID %d is not supported in the current configuration.", phoneId);
        return LE_FAULT;
    }

    // Get the list of profiles for the specified phone ID.
    std::vector<std::shared_ptr<TafDcsProfile>> profilePtrs = FindProfilesByPhoneId(phoneId);

    if (0 == profilePtrs.size())
    {
        LE_WARN("No profiles found for phone ID: %d", TO_INT(phoneId));
        // Update the size of the returned array to 0.
        *profilesListSizePtr = 0;
        return LE_OK;
    }

    // Update the results
    size_t iCount = 0;
    for (const auto &profilePtr : profilePtrs)
    {
        uint32_t id;
        taf_dcs_Tech_t tech;
        std::string name;
        profilePtr->GetName(name);
        profilePtr->GetId(id);
        profilePtr->GetTech(tech);

        profilesListPtr[iCount].index = id;
        profilesListPtr[iCount].tech = tech;
        le_utf8_Copy(profilesListPtr[iCount].name, name.c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
        LE_DEBUG("Id: %d, Tech: %d, Name: %s", profilesListPtr[iCount].index,
                                        profilesListPtr[iCount].tech, profilesListPtr[iCount].name);
        // Increment count
        iCount++;
    }

    *profilesListSizePtr = iCount;
    LE_DEBUG("Num profiles read for phone ID %d: %zu", phoneId, *profilesListSizePtr);
    return LE_OK;
}

// Get the profile reference
taf_dcs_ProfileRef_t TafDcsProfileManager::SvcGetProfileRef
(
    uint8_t phoneId,
    uint32_t profileId
)
{
    TAF_ERROR_IF_RET_VAL(TAF_TYPES_PHONE_ID_1 != phoneId && TAF_TYPES_PHONE_ID_2 != phoneId,
                                                     nullptr, "Unsupported phoneId %d", phoneId);

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    auto profileOptWrapper = getProfile(phoneId, profileId);
    if (profileOptWrapper.has_value())
    {
        TafDcsProfile &profile = profileOptWrapper.value().get();
        // Call the TafDcsProfile API with the object reference to get the profile's TAF reference
        return profile.GetReference();
    }

    // Profile object is not found. Check if client wants to create a profile. If not return error.
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID != profileId, nullptr,
                                        "Profile %d not found for phone ID %d", profileId, phoneId);

    // Create a new profile with TAF_DCS_UNDEFINED_PROFILE_ID
    TafDcsProfileInfo_t profileInfo =
        {
            TAF_DCS_UNDEFINED_PROFILE_ID,
            "", "", "", "",
            TAF_DCS_TECH_3GPP,
            TAF_DCS_AUTH_NONE,
            TAF_DCS_PDP_IPV4,
            TAF_DCS_APN_TYPE_DEFAULT,
            false};

    taf::pa::data::SlotId_e paSlotID;
    le_result_t result = taf::pa::data::GetSimSlotIdFromPhoneId(
                                    static_cast<taf::pa::data::PhoneId_e>(phoneId), paSlotID);
    if (LE_OK != result)
    {
        LE_WARN("PA GetSimSlotIdFromPhoneId failed: %d", TO_INT(result));
        // Use default slot ID 1
        paSlotID = taf::pa::data::SlotId_e::SLOT_1;
    }

    auto tafDcsProfile = std::make_shared<TafDcsProfile>(static_cast<uint8_t>(paSlotID),
                                                         phoneId,
                                                         profileInfo);
    LE_INFO("Created new profile.");

    // Add the newly created profile to maps.
    if (!addToProfilesMap(profileInfo.id, phoneId, tafDcsProfile))
    {
        LE_ERROR("Profiles map not updated");
        return nullptr;
    }
    if (!addToProfileRefsMap(tafDcsProfile->GetReference(), tafDcsProfile))
    {
        LE_ERROR("Profile refs map not updated");
        return nullptr;
    }

    LE_DEBUG("Profile and maps updated.");
    return tafDcsProfile->GetReference();
}

le_result_t TafDcsProfileManager::SvcGetProfileId
(
    taf_dcs_ProfileRef_t profileRef,
    uint32_t* profileIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileIdPtr, LE_BAD_PARAMETER, "profileIdPtr is NULL");

    uint32_t profileId;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Call the TafDcsProfile API with the object reference to get the profile Id
    le_result_t result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed: %d", TO_INT(result));

    *profileIdPtr = profileId;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetPhoneId
(
    taf_dcs_ProfileRef_t profileRef,
    uint8_t* phoneIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == phoneIdPtr, LE_BAD_PARAMETER, "phoneIdPtr is NULL");
    uint8_t phoneId;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Call the TafDcsProfile API with the object reference to get the phone Id
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed: %d", TO_INT(result));

    *phoneIdPtr = phoneId;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetPhoneIdByInterfaceName
(
    const char *LE_NONNULL ifNameStr,
    uint8_t *phoneIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == ifNameStr, LE_BAD_PARAMETER, "ifNameStr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == phoneIdPtr, LE_BAD_PARAMETER, "phoneIdPtr is NULL");

    std::string ifName(ifNameStr);
    le_result_t result = LE_OK;
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(profilesMapMutex_);
    // Iterate through the profiles map
    for (const auto &entry : profilesMap_)
    {
        std::string hostIf;
        const auto &profile = entry.second;
        result = profile->GetHostInterface(hostIf);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "GetHostInterface failed.");
        if (ifName == hostIf)
        {
            uint8_t phoneId = 0;
            taf_dcs_ConState_t state=TAF_DCS_DISCONNECTED, ipv4State, ipv6State;
            result = profile->GetSessionState(state, ipv4State, ipv6State);
            LE_WARN_IF(LE_OK != result, "GetSessionState error.");
            if (TAF_DCS_CONNECTED != state)
            {
                LE_DEBUG ("Profile is not conencted. Keep going.");
                continue;
            }
            result = profile->GetPhoneId(phoneId);
            TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "GetPhoneId failed.");
            *phoneIdPtr = phoneId;
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

le_result_t TafDcsProfileManager::SvcGetProfileIdByInterfaceName
(
    const char *LE_NONNULL ifNameStr,
    uint32_t *profileIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == ifNameStr, LE_BAD_PARAMETER, "ifNameStr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == profileIdPtr, LE_BAD_PARAMETER, "profileIdPtr is NULL");

    std::string ifName(ifNameStr);
    le_result_t result = LE_OK;
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(profilesMapMutex_);
    // Iterate through the profiles map
    for (const auto &entry : profilesMap_)
    {
        std::string hostIf;
        const auto &profile = entry.second;
        result = profile->GetHostInterface(hostIf);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "GetHostInterface failed.");
        if (ifName == hostIf)
        {
            uint32_t profileId = 0;
            taf_dcs_ConState_t state = TAF_DCS_DISCONNECTED, ipv4State, ipv6State;
            result = profile->GetSessionState(state, ipv4State, ipv6State);
            LE_WARN_IF(LE_OK != result, "GetSessionState error.");
            if (TAF_DCS_CONNECTED != state)
            {
                LE_DEBUG("Profile is not conencted. Keep going.");
                continue;
            }
            result = profile->GetId(profileId);
            TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "GetId failed.");
            *profileIdPtr = profileId;
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

le_result_t TafDcsProfileManager::SvcCreateProfile(taf_dcs_ProfileRef_t profileRef)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

        // Get the profile ID
    uint32_t profileId;
    le_result_t result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetID failed.");

    // Check if the profile has already been created or not.
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID != profileId, LE_DUPLICATE,
                                                        "Profile %d already exists.", profileId);

    // Get the phone ID.
    uint8_t phoneId = 0;
    taf::pa::data::ProfileId_e paProfileId;
    result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);
    // Create the profile
    result = taf::pa::data::CreateProfile(static_cast<taf::pa::data::PhoneId_e>(phoneId),
                                                                    profileInfo, paProfileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "CreateProfile failed.");

    LE_INFO("Created profile ID: %d", TO_INT(paProfileId));

    // Update the profile ID with the ID provided by the PA.
    result = profile.SetId(static_cast<uint32_t>(paProfileId));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetId failed.");

    // Remove TAF_DCS_UNDEFINED_PROFILE_ID key from profilesMap_
    if (!updateProfilesMapKey(TAF_DCS_UNDEFINED_PROFILE_ID, static_cast<uint32_t>(paProfileId),
                                                                                        phoneId))
    {
        LE_WARN("updateProfilesMapKey failed.");
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcDeleteProfile(taf_dcs_ProfileRef_t profileRef)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the profile ID
    uint32_t profileId;
    le_result_t result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetID failed.");

    // Check if the profile has been created or not.
    TAF_ERROR_IF_RET_VAL (TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile still to be created.");
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed.");

    //Get the phone ID.
    uint8_t phoneId = 0;
    result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d, Profile ID: %d", phoneId, profileId);

    // Ensure data is disconnected
    TAF_ERROR_IF_RET_VAL(TAF_DCS_DISCONNECTED != connState, LE_BUSY,
                                                "Profile is not in TAF_DCS_DISCONNECTED state.");

    taf::pa::data::ProfileInfo_t profileInfo;
    profileInfo.profileId = static_cast<taf::pa::data::ProfileId_e>(profileId);
    result = taf::pa::data::DeleteProfile(static_cast<taf::pa::data::PhoneId_e>(phoneId),
                                                                                    profileInfo);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "PA DeleteProfile failed.");

    // Remove the this profile object from the service map.
    result = deleteProfile(profileRef, phoneId, profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "deleteProfile failed.");

    return result;
}

le_result_t TafDcsProfileManager::SvcGetAPN
(
    taf_dcs_ProfileRef_t profileRef,
    char *apnStr,
    size_t apnNameSize)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == apnStr, LE_BAD_PARAMETER, "apnStr is NULL");

    // Use the lesser of the two sizes.
    size_t apnStrBytes =
            (apnNameSize < TAF_DCS_APN_NAME_MAX_BYTES) ? apnNameSize : TAF_DCS_APN_NAME_MAX_BYTES;
    if (0 == apnNameSize)
    {
        LE_WARN("apnNameSize is 0. Using TAF_DCS_APN_NAME_MAX_BYTES");
        apnStrBytes = TAF_DCS_APN_NAME_MAX_BYTES;
    }

    // Get the profile object
    std::string   apn;
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Call the TafDcsProfile API with the object reference to get the APN
    le_result_t result = profile.GetApn(apn);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetApn failed: %d", TO_INT(result));

    result = le_utf8_Copy(apnStr, apn.c_str(), apnStrBytes, NULL);
    LE_WARN_IF(LE_OK !=result, "le_utf8_Copy error: %d", TO_INT(result));
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
    char *nameStr,
    size_t nameSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == nameStr, LE_BAD_PARAMETER, "nameStr is NULL");
    TAF_ERROR_IF_RET_VAL(0 == nameSize, LE_BAD_PARAMETER, "nameSize is 0");

    // Get the profile object
    std::string profileName;
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Call the TafDcsProfile API with the object reference to get the APN
    le_result_t result = profile.GetName(profileName);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetName failed: %d", TO_INT(result));

    result = le_utf8_Copy(nameStr, profileName.c_str(), TAF_DCS_NAME_MAX_BYTES, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t *authPtr,
    char *userNameStr,
    size_t userNameSize,
    char *passwordStr,
    size_t pwSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == authPtr, LE_BAD_PARAMETER, "authPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == userNameStr, LE_BAD_PARAMETER, "userNameStr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == passwordStr, LE_BAD_PARAMETER, "passwordStr is NULL");

    // Use the lesser of the two sizes.
    size_t unStrBytes =
        (userNameSize < TAF_DCS_USER_NAME_MAX_BYTES) ? userNameSize : TAF_DCS_USER_NAME_MAX_BYTES;
    if (0 == unStrBytes)
    {
        LE_WARN("userNameSize is 0. Using TAF_DCS_USER_NAME_MAX_BYTES");
        unStrBytes = TAF_DCS_USER_NAME_MAX_BYTES;
    }
    size_t pwStrBytes =
            (pwSize < TAF_DCS_PASSWORD_NAME_MAX_BYTES) ? pwSize : TAF_DCS_PASSWORD_NAME_MAX_BYTES;
    if (0 == pwStrBytes)
    {
        LE_WARN("passwordSize is 0. Using TAF_DCS_PASSWORD_NAME_MAX_BYTES");
        pwStrBytes = TAF_DCS_USER_NAME_MAX_BYTES;
    }

    // Get the profile object
    std::string profileName;
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the auth type
    taf_dcs_Auth_t auth;
    le_result_t result = profile.GetAuthTypeBitmask(auth);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetAuthTypeBitmask failed: %d", TO_INT(result));
    *authPtr = auth;

    // Get the usename
    std::string userName;
    result = profile.GetUserName(userName);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetUserName failed: %d", TO_INT(result));
    result = le_utf8_Copy(userNameStr, userName.c_str(), unStrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    //Get the password
    std::string password;
    result = profile.GetPassword(password);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPassword failed: %d", TO_INT(result));
    result = le_utf8_Copy(passwordStr, password.c_str(), pwStrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Tech_t *techPrefPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == techPrefPtr, LE_BAD_PARAMETER, "techPrefPtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    taf_dcs_Tech_t tech;
    le_result_t result = profile.GetTech(tech);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetTech failed: %d", TO_INT(result));

    *techPrefPtr = tech;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ApnType_t *apnTypePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == apnTypePtr, LE_BAD_PARAMETER, "apnTypePtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    taf_dcs_ApnType_t apnTypes;
    le_result_t result = profile.GetApnTypeBitmask(apnTypes);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetApnTypeBitmask failed: %d", TO_INT(result));

    *apnTypePtr = apnTypes;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetPDP(taf_dcs_ProfileRef_t profileRef, taf_dcs_Pdp_t &pdp)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Call the TafDcsProfile API with the object reference to get the PDP (IP type)
    le_result_t result = profile.GetPdp(pdp);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPdp failed: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetAPN(taf_dcs_ProfileRef_t profileRef, const char *apnStr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == apnStr, LE_BAD_PARAMETER, "apnStr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t  phoneId   = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        // Update APN via PA.
        memset(profileInfo.apn, 0, taf::pa::data::MAX_APN_LEN);
        le_utf8_Copy(profileInfo.apn, apnStr, taf::pa::data::MAX_APN_LEN, nullptr);

        result = taf::pa::data::UpdateProfile(
                                    static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"PA UpdateProfile failed: %d", TO_INT(result));
    }

    // Update the details in the profile.
    result = profile.SetApn(std::string(apnStr));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetApn failed.");

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ApnType_t apnType
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        // Update APN type preference via PA
        profileInfo.apnTypeMask = TafDcsUtils::ConvertApnTypeMask(apnType);
        result = taf::pa::data::UpdateProfile(
                                    static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"PA UpdateProfile failed: %d", TO_INT(result));
    }

    // Update the details in the profile.
    result = profile.SetApnTypeBitmask(apnType);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetApn failed.");
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetPDP(taf_dcs_ProfileRef_t profileRef, taf_dcs_Pdp_t pdp)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        // Update PDP name via PA
        profileInfo.ipType = TafDcsUtils::ConvertPDP(pdp);

        result = taf::pa::data::UpdateProfile(
            static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"PA UpdateProfile failed: %d", TO_INT(result));
    }

    // Update the details in the profile.
    result = profile.SetPdp(pdp);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetName failed.");

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
    const char *nameStr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        // Update profile name via PA
        memset(profileInfo.name, 0, taf::pa::data::MAX_NAME_LEN);
        le_utf8_Copy(profileInfo.name, nameStr, taf::pa::data::MAX_NAME_LEN, nullptr);

        result = taf::pa::data::UpdateProfile(
            static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"PA UpdateProfile failed: %d", TO_INT(result));
    }

    // Update the details in the profile.
    result = profile.SetName(std::string(nameStr));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetName failed.");

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Tech_t techPreference
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        // Update tech preference via PA
        profileInfo.techPref = TafDcsUtils::ConvertTechPref(techPreference);

        result = taf::pa::data::UpdateProfile(
                            static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"PA UpdateProfile failed: %d", TO_INT(result));
    }
    result = profile.SetTech(techPreference);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetTechPreference failed.");

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t auth,
    const char *userName,
    const char *password
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    // Get the phone ID.
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    // Populate the profile details
    taf::pa::data::ProfileInfo_t profileInfo;
    profile.PopulateProfileInfoStruct(profileInfo);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not yet created.
        LE_INFO("Profile is yet to be created.");
    }
    else
    {
        if (TAF_DCS_AUTH_NONE == auth)
        {
            memset(profileInfo.userName, 0, taf::pa::data::MAX_USERNAME_LEN);
            memset(profileInfo.password, 0, taf::pa::data::MAX_PASSWORD_LEN);
        }
        else
        {
            // Update the auth type parameter
            profileInfo.authType = TafDcsUtils::ConvertAuthType(auth);
            // Update user name
            memset(profileInfo.userName, 0, taf::pa::data::MAX_USERNAME_LEN);
            le_utf8_Copy(profileInfo.userName, userName, taf::pa::data::MAX_USERNAME_LEN, nullptr);
            // Update password.
            memset(profileInfo.password, 0, taf::pa::data::MAX_PASSWORD_LEN);
            le_utf8_Copy(profileInfo.password, password, taf::pa::data::MAX_PASSWORD_LEN, nullptr);

            result = taf::pa::data::UpdateProfile(
                static_cast<taf::pa::data::PhoneId_e>(phoneId), profileInfo);
            TAF_ERROR_IF_RET_VAL(LE_OK != result,result, "PA UpdateProfile failed: %d",
                                                                                TO_INT(result));
        }
    }

    // Update the details in the profile.
    result = profile.SetAuthTypeBitmask(auth);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetAuthTypeBitmask failed.");
    result = profile.SetUserName(std::string(userName));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetUserName failed.");
    result = profile.SetPassword(std::string(password));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetPassword failed.");
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetSessionState
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t *connStatePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == connStatePtr, LE_BAD_PARAMETER, "connStatePtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Call the TafDcsProfile API with the object reference to get the session state
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    le_result_t result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));

    LE_DEBUG("State: %d", TO_INT(connState));

    *connStatePtr = connState;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv4Address
(
    taf_dcs_ProfileRef_t profileRef,
    char* ipAddr,
    size_t ipAddrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ipAddr, LE_BAD_PARAMETER, "ipAddr is NULL");

    // Use the lesser of the two sizes.
    size_t ipAddrBytes =
            (ipAddrSize < TAF_DCS_IPV4_ADDR_MAX_LEN) ? ipAddrSize : TAF_DCS_IPV4_ADDR_MAX_LEN;
    if (0 == ipAddrBytes)
    {
        LE_WARN("ipAddrSize is 0. Using TAF_DCS_IPV4_ADDR_MAX_LEN.");
        ipAddrBytes = TAF_DCS_IPV4_ADDR_MAX_LEN;
    }

    std::string ipv4, dummyStr;
    unsigned int dummyMask;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv4 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv4(profileRef), LE_UNAVAILABLE, "IPv4 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    le_result_t result = profile.GetIPv4Addresses(ipv4,
                                            dummyStr, dummyStr, dummyStr, dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv4Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(ipAddr, ipv4.c_str(), ipAddrSize, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetRoamingStatus
(
    uint8_t phoneId,
    bool *isRoamingPtr,
    taf_dcs_RoamingType_t *typePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == isRoamingPtr, LE_BAD_PARAMETER, "isRoamingPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == typePtr, LE_BAD_PARAMETER, "typePtr is NULL");

    LE_DEBUG ("Phone ID: %d", phoneId);

    taf::pa::data::RoamingStatus_t roamingStatus;
    le_result_t result = taf::pa::data::GetRoamingStatus(
                                    static_cast<taf::pa::data::PhoneId_e>(phoneId),roamingStatus);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "PA GetRoamingStatus failed.");

    *isRoamingPtr = roamingStatus.isRoaming;
    *typePtr = TafDcsUtils::ConvertRoamingType(roamingStatus.type);

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv4SubnetMask
(
    taf_dcs_ProfileRef_t profileRef,
    uint32_t *maskPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == maskPtr, LE_BAD_PARAMETER, "maskPtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv4 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv4(profileRef), LE_UNAVAILABLE, "IPv4 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    std::string dummyStr;
    unsigned int gwMask, dummyMask;

    le_result_t result = profile.GetIPv4Addresses(dummyStr, dummyStr, dummyStr, dummyStr,
                                                                            gwMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv4Address failed: %d", TO_INT(result));

    *maskPtr = gwMask;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv4GatewayAddress
(
    taf_dcs_ProfileRef_t profileRef,
    char *gatewayAddr,
    size_t gatewayAddrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == gatewayAddr, LE_BAD_PARAMETER, "gatewayAddr is NULL");


    // Use the lesser of the two sizes.
    size_t gwAddrBytes =
        (gatewayAddrSize < TAF_DCS_IPV4_ADDR_MAX_LEN) ? gatewayAddrSize : TAF_DCS_IPV4_ADDR_MAX_LEN;
    if (0 == gwAddrBytes)
    {
        LE_WARN("gatewayAddrSize is 0. Using TAF_DCS_IPV4_ADDR_MAX_LEN.");
        gwAddrBytes = TAF_DCS_IPV4_ADDR_MAX_LEN;
    }

    std::string gw, dummyStr;
    unsigned int dummyMask;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv4 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv4(profileRef), LE_UNAVAILABLE, "IPv4 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    le_result_t result = profile.GetIPv4Addresses(dummyStr, gw, dummyStr, dummyStr,
                                                                    dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv4Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(gatewayAddr, gw.c_str(), gwAddrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv4DNSAddresses
(
    taf_dcs_ProfileRef_t profileRef,
    char *dns1AddrStr,
    size_t dns1AddrStrSize,
    char *dns2AddrStr,
    size_t dns2AddrStrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == dns1AddrStr, LE_BAD_PARAMETER, "dns1AddrStr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == dns2AddrStr, LE_BAD_PARAMETER, "dns2AddrStr is NULL");

    // Use the lesser of the two sizes.
    size_t dns1Bytes =
        (dns1AddrStrSize < TAF_DCS_IPV4_ADDR_MAX_LEN) ? dns1AddrStrSize : TAF_DCS_IPV4_ADDR_MAX_LEN;
    if (0 == dns1Bytes)
    {
        LE_WARN("dns1AddrStrSize is 0. Using TAF_DCS_IPV4_ADDR_MAX_LEN.");
        dns1Bytes = TAF_DCS_IPV4_ADDR_MAX_LEN;
    }
    size_t dns2Bytes =
        (dns2AddrStrSize < TAF_DCS_IPV4_ADDR_MAX_LEN) ? dns2AddrStrSize : TAF_DCS_IPV4_ADDR_MAX_LEN;
    if (0 == dns2Bytes)
    {
        LE_WARN("dns2AddrStrSize is 0. Using TAF_DCS_IPV4_ADDR_MAX_LEN.");
        dns2Bytes = TAF_DCS_IPV4_ADDR_MAX_LEN;
    }

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv4 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv4(profileRef), LE_UNAVAILABLE, "IPv4 call is not active");

    std::string dns1, dns2, dummyStr;
    unsigned int dummyMask;

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    le_result_t result = profile.GetIPv4Addresses(dummyStr, dummyStr, dns1, dns2,
                                                                    dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv4Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(dns1AddrStr, dns1.c_str(), dns1Bytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    result = le_utf8_Copy(dns2AddrStr, dns2.c_str(), dns2Bytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv6Address
(
    taf_dcs_ProfileRef_t profileRef,
    char* ipAddr,
    size_t ipAddrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ipAddr, LE_BAD_PARAMETER, "ipAddr is NULL");

    // Use the lesser of the two sizes.
    size_t ipAddrBytes =
                (ipAddrSize < TAF_DCS_IPV6_ADDR_MAX_LEN) ? ipAddrSize : TAF_DCS_IPV6_ADDR_MAX_LEN;
    if (0 == ipAddrBytes)
    {
        LE_WARN("ipAddrSize is 0. Using TAF_DCS_IPV6_ADDR_MAX_LEN.");
        ipAddrBytes = TAF_DCS_IPV6_ADDR_MAX_LEN;
    }

    std::string ipv6, dummyStr;
    unsigned int dummyMask;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv6 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv6(profileRef), LE_UNAVAILABLE, "IPv6 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IP v6 address
    le_result_t result = profile.GetIPv6Addresses(ipv6,
                                            dummyStr, dummyStr, dummyStr, dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv6Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(ipAddr, ipv6.c_str(), ipAddrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv6SubnetMask
(
    taf_dcs_ProfileRef_t profileRef,
    uint32_t *maskPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == maskPtr, LE_BAD_PARAMETER, "maskPtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv6 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv6(profileRef), LE_UNAVAILABLE, "IPv6 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    std::string dummyStr;
    unsigned int gwMask, dummyMask;

    le_result_t result = profile.GetIPv4Addresses(dummyStr, dummyStr, dummyStr, dummyStr,
                                                                            gwMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv4Address failed: %d", TO_INT(result));

    *maskPtr = gwMask;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv6GatewayAddress
(
    taf_dcs_ProfileRef_t profileRef,
    char *gatewayAddr,
    size_t gatewayAddrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == gatewayAddr, LE_BAD_PARAMETER, "gatewayAddr is NULL");

    // Use the lesser of the two sizes.
    size_t gwAddrBytes =
        (gatewayAddrSize < TAF_DCS_IPV6_ADDR_MAX_LEN) ? gatewayAddrSize : TAF_DCS_IPV6_ADDR_MAX_LEN;
    if (0 == gwAddrBytes)
    {
        LE_WARN("gatewayAddrSize is 0. Using TAF_DCS_IPV6_ADDR_MAX_LEN.");
        gwAddrBytes = TAF_DCS_IPV6_ADDR_MAX_LEN;
    }

    std::string gw, dummyStr;
    unsigned int dummyMask;

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

        // Ensure IPv6 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv6(profileRef), LE_UNAVAILABLE, "IPv6 call is not active");

    // Call the TafDcsProfile API with the object reference to get the IP v6 address
    le_result_t result = profile.GetIPv6Addresses(dummyStr, gw, dummyStr, dummyStr,
                                                                        dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv6Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(gatewayAddr, gw.c_str(), gwAddrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetIPv6DNSAddresses
(
    taf_dcs_ProfileRef_t profileRef,
    char *dns1AddrStr,
    size_t dns1AddrStrSize,
    char *dns2AddrStr,
    size_t dns2AddrStrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == dns1AddrStr, LE_BAD_PARAMETER, "dns1AddrStr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == dns2AddrStr, LE_BAD_PARAMETER, "dns2AddrStr is NULL");

    // Use the lesser of the two sizes.
    size_t dns1Bytes =
        (dns1AddrStrSize < TAF_DCS_IPV6_ADDR_MAX_LEN) ? dns1AddrStrSize : TAF_DCS_IPV6_ADDR_MAX_LEN;
    if (0 == dns1Bytes)
    {
        LE_WARN("dns1AddrStrSize is 0. Using TAF_DCS_IPV6_ADDR_MAX_LEN.");
        dns1Bytes = TAF_DCS_IPV6_ADDR_MAX_LEN;
    }
    size_t dns2Bytes =
        (dns2AddrStrSize < TAF_DCS_IPV6_ADDR_MAX_LEN) ? dns2AddrStrSize : TAF_DCS_IPV6_ADDR_MAX_LEN;
    if (0 == dns2Bytes)
    {
        LE_WARN("dns2AddrStrSize is 0. Using TAF_DCS_IPV6_ADDR_MAX_LEN.");
        dns2Bytes = TAF_DCS_IPV6_ADDR_MAX_LEN;
    }

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure IPv6 data call is active.
    TAF_ERROR_IF_RET_VAL(!SvcIsIPv6(profileRef), LE_UNAVAILABLE, "IPv6 call is not active");

    std::string dns1, dns2, dummyStr;
    unsigned int dummyMask;

    // Call the TafDcsProfile API with the object reference to get the IPv4 address.
    le_result_t result = profile.GetIPv6Addresses(dummyStr, dummyStr, dns1, dns2,
                                                  dummyMask, dummyMask);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetIPv6Address failed: %d", TO_INT(result));

    result = le_utf8_Copy(dns1AddrStr, dns1.c_str(), dns1Bytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    result = le_utf8_Copy(dns2AddrStr, dns2.c_str(), dns2Bytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetInterfaceName
(
    taf_dcs_ProfileRef_t profileRef,
    char *ifNameStr,
    size_t ifNameStrSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ifNameStr, LE_BAD_PARAMETER, "ifNameStr is NULL");

    // Use the lesser of the two sizes.
    size_t ifStrBytes =
                (ifNameStrSize < TAF_DCS_NAME_MAX_BYTES) ? ifNameStrSize : TAF_DCS_NAME_MAX_BYTES;
    if (0 == ifStrBytes)
    {
        LE_WARN("ifNameStrSize is 0. Using TAF_DCS_NAME_MAX_BYTES.");
        ifStrBytes = TAF_DCS_NAME_MAX_BYTES;
    }

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure data call is active.
    // Call the TafDcsProfile API with the object reference to get the session state
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    std::string ifName;
    le_result_t result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));

    TAF_ERROR_IF_RET_VAL(TAF_DCS_CONNECTED != connState, LE_UNAVAILABLE, "Data call is not active");

    result = profile.GetHostInterface(ifName);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetHostInterface failed: %d", TO_INT(result));

    result = le_utf8_Copy(ifNameStr, ifName.c_str(), ifStrBytes, NULL);
    LE_WARN_IF(LE_OK != result, "le_utf8_Copy error: %d", TO_INT(result));

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetDataBearerTechnology
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_DataBearerTechnology_t *dlDataBearerTechPtrPtr,
    taf_dcs_DataBearerTechnology_t *ulDataBearerTechPtrPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == dlDataBearerTechPtrPtr, LE_BAD_PARAMETER,
                                                                "dlDataBearerTechPtrPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ulDataBearerTechPtrPtr, LE_BAD_PARAMETER,
                                                                "ulDataBearerTechPtrPtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Ensure data call is active.
    // Call the TafDcsProfile API with the object reference to get the session state
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    std::string ifName;
    le_result_t result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));

    TAF_ERROR_IF_RET_VAL(TAF_DCS_CONNECTED != connState, LE_UNAVAILABLE, "Data call is not active");

    taf_dcs_DataBearerTechnology_t tech;
    result = profile.GetDataBearerTech(tech);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetDataBearerTech failed: %d", TO_INT(result));

    *dlDataBearerTechPtrPtr = tech;
    *ulDataBearerTechPtrPtr = tech;

    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetCallEndReason
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Pdp_t pdpType,
    taf_dcs_CallEndReasonType_t *callEndReasonTypePtr,
    int32_t *callEndReasonCodePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == callEndReasonTypePtr, LE_BAD_PARAMETER,
                                                "callEndReasonTypePtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == callEndReasonCodePtr, LE_BAD_PARAMETER,
                                                "callEndReasonCodePtr is NULL");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_PDP_UNKNOWN == pdpType, LE_BAD_PARAMETER, "pdpType is invalid");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_PDP_IPV4V6 == pdpType, LE_BAD_PARAMETER, "Specify IPv4 or IPv6");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // A data call should have been setup
    TAF_ERROR_IF_RET_VAL(!profile.GetCallSetup(), LE_UNAVAILABLE,
                                                "Data calll has not been setup yet.");

    // Call the TafDcsProfile API with the object reference to get the session state
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    uint32_t profileId = 0;
    le_result_t result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));

    result = profile.GetId(profileId);
    LE_WARN_IF(LE_OK != result, "GetId failed: %d", TO_INT(result));

    if (TAF_DCS_PDP_IPV4 == pdpType)
    {
        // Check if a IPv4 data call is active
        if (ipv4state != TAF_DCS_DISCONNECTED)
        {
            // Call is connected. Return LE_UNAVAILABLE
            LE_WARN("IPv4 call is active for profile id %d", TO_INT(profileId));
            return LE_UNAVAILABLE;
        }
        taf_dcs_CallEndReasonType_t type, typeIPv4, typeIPv6;
        int32_t code;
        result = profile.GetCallEndReason(type, code, typeIPv4, typeIPv6);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"GetCallEndReason failed: %d", TO_INT(result));
        *callEndReasonTypePtr = typeIPv4;
        *callEndReasonCodePtr = code;
    }
    else if (TAF_DCS_PDP_IPV6 == pdpType)
    {
        // Check if a IPv6 data call is active
        if (ipv6state != TAF_DCS_DISCONNECTED)
        {
            // Call is connected. Return LE_UNAVAILABLE
            LE_WARN("IPv4 call is active for profile id %d", TO_INT(profileId));
            return LE_UNAVAILABLE;
        }
        taf_dcs_CallEndReasonType_t type, typeIPv4, typeIPv6;
        int32_t code;
        result = profile.GetCallEndReason(type, code, typeIPv4, typeIPv6);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetCallEndReason failed: %d", TO_INT(result));
        *callEndReasonTypePtr = typeIPv6;
        *callEndReasonCodePtr = code;
    }
    return LE_OK;
}

le_result_t TafDcsProfileManager::updateThrottledApnStatus
(
    const taf::pa::data::ThrottledApnEventInfo_t &throttledEvent
)
{
    // TODO
    LE_UNUSED(throttledEvent);
    return LE_UNSUPPORTED;
}

le_result_t TafDcsProfileManager::SvcGetAPNThrottledStatus
(
    taf_dcs_ProfileRef_t profileRef,
    bool *isThrottledPtr,
    uint32_t *ipv4RemainingTimePtr,
    uint32_t *ipv6RemainingTimePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == isThrottledPtr, LE_BAD_PARAMETER, "isThrottledPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ipv4RemainingTimePtr, LE_BAD_PARAMETER,
                                                                    "ipv4RemainingTimePtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == ipv6RemainingTimePtr, LE_BAD_PARAMETER,
                                                                    "ipv6RemainingTimePtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    std::vector<taf::pa::data::ThrottledApnEventInfo_t> throttledApnEventInfoList;
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    result = taf::pa::data::GetThrottledApnInfo(
        static_cast<taf::pa::data::PhoneId_e>(phoneId), throttledApnEventInfoList);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "PA GetThrottledApnInfo failed: %d", TO_INT(result));

    LE_INFO("Num throttled APNs: %zu", throttledApnEventInfoList.size());
    if (0 == throttledApnEventInfoList.size())
    {
        LE_INFO ("No APNs are throttled.");
        *isThrottledPtr = false;
        *ipv4RemainingTimePtr = 0;
        *ipv6RemainingTimePtr = 0;
        return LE_OK;
    }
    else
    {
        // Update the throttled status in the profiles.
        for (auto &throttledApnEventInfo : throttledApnEventInfoList)
        {
            for (taf::pa::data::ProfileId_e paProfileId : throttledApnEventInfo.profileIds)
            {
                if (profileId == static_cast<uint32_t>(paProfileId))
                {
                    *isThrottledPtr       = true;
                    *ipv4RemainingTimePtr = throttledApnEventInfo.ipv4Time;
                    *ipv6RemainingTimePtr = throttledApnEventInfo.ipv6Time;
                    LE_DEBUG ("Throttled APN      : %s", throttledApnEventInfo.apn.c_str());
                    LE_DEBUG ("IPv4 remaining time: %u", *ipv4RemainingTimePtr);
                    LE_DEBUG ("IPv6 remaining time: %u", *ipv6RemainingTimePtr);
                    return LE_OK;
                }
            }
        }
    }
    // No match to this profile found
    LE_INFO("No match found for this profile.");
    *isThrottledPtr = false;
    *ipv4RemainingTimePtr = 0;
    *ipv6RemainingTimePtr = 0;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetAPNThrottledPLMN
(
    taf_dcs_ProfileRef_t profileRef,
    bool *areAllPLMNsThrottledPtr,
    char *mcc,
    size_t mccSize,
    char *mnc,
    size_t mncSize
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == areAllPLMNsThrottledPtr, LE_BAD_PARAMETER,
                                                        "areAllPLMNsThrottledPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == mcc, LE_BAD_PARAMETER, "mcc is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == mnc, LE_BAD_PARAMETER, "mnc is NULL");

    // Use the lesser of the two sizes.
    size_t mccBytes = (mccSize < TAF_DCS_MCC_BYTES) ? mccSize : TAF_DCS_MCC_BYTES;
    if (0 == mccBytes)
    {
        LE_WARN("mccSize is 0. Using TAF_DCS_MCC_BYTES");
        mccBytes = TAF_DCS_MCC_BYTES;
    }
    size_t mncBytes = (mncSize < TAF_DCS_MNC_BYTES) ? mncSize : TAF_DCS_MNC_BYTES;
    if (0 == mncBytes)
    {
        LE_WARN("mncSize is 0. Using TAF_DCS_MNC_BYTES");
        mncBytes = TAF_DCS_MNC_BYTES;
    }

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    std::vector<taf::pa::data::ThrottledApnEventInfo_t> throttledApnEventInfoList;
    uint8_t phoneId = 0;
    le_result_t result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed.");
    LE_DEBUG("Phone ID: %d", phoneId);

    uint32_t profileId = 0;
    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed.");
    LE_DEBUG("Profile ID: %d", profileId);

    result = taf::pa::data::GetThrottledApnInfo(
        static_cast<taf::pa::data::PhoneId_e>(phoneId), throttledApnEventInfoList);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                        "PA GetThrottledApnInfo failed: %d", TO_INT(result));

    LE_INFO("Num throttled APNs: %zu", throttledApnEventInfoList.size());
    if (0 == throttledApnEventInfoList.size())
    {
        LE_INFO("No APNs are throttled.");
        *areAllPLMNsThrottledPtr = false;
        memset(mcc, 0, mccBytes);
        memset(mnc, 0, mncSize);
        return LE_OK;
    }
    else
    {
        // Update the throttled status in the profiles.
        for (auto &throttledApnEventInfo : throttledApnEventInfoList)
        {
            for (taf::pa::data::ProfileId_e paProfileId : throttledApnEventInfo.profileIds)
            {
                if (profileId == static_cast<uint32_t>(paProfileId))
                {
                    *areAllPLMNsThrottledPtr = throttledApnEventInfo.isBlockedOnAllPLMNs;
                    LE_DEBUG ("Blocked on all PLMNs: %s",*areAllPLMNsThrottledPtr ?"true" :"false");
                    if (throttledApnEventInfo.mcc.size())
                    {
                        le_utf8_Copy(mcc, throttledApnEventInfo.mcc.c_str(), mccBytes, NULL);
                        LE_DEBUG("MCC: %s", mcc);
                    }
                    if (throttledApnEventInfo.mnc.size())
                    {
                        le_utf8_Copy(mnc, throttledApnEventInfo.mnc.c_str(), mncSize, NULL);
                        LE_DEBUG("MNC: %s", mnc);
                    }

                    return LE_OK;
                }
            }
        }
    }
    // No match to this profile found
    LE_INFO("No match found for this profile.");
    *areAllPLMNsThrottledPtr = false;
    memset(mcc, 0, mccBytes);
    memset(mnc, 0, mncSize);
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetMaxDataBitRates
(
    taf_dcs_ProfileRef_t profileRef,
    uint64_t *maxRxBitRatePtr,
    uint64_t *maxTxBitRatePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == maxRxBitRatePtr, LE_BAD_PARAMETER,"maxRxBitRatePtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == maxTxBitRatePtr, LE_BAD_PARAMETER,"maxTxBitRatePtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    // Call the TafDcsProfile API with the object reference to get the session state
    taf_dcs_ConState_t connState, ipv4state, ipv6state;
    std::string ifName;
    le_result_t result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));
    if (
        TAF_DCS_CONNECTED == connState ||
        TAF_DCS_CONNECTED == ipv4state ||
        TAF_DCS_CONNECTED == ipv6state
       )
    {
        // Call is connected.
        uint64_t rxRate, txRate;
        result = profile.GetMaxDataBitRates(rxRate, txRate);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetMaxDataBitRates failed: %d",
                                                                            TO_INT(result));
        *maxRxBitRatePtr = rxRate;
        *maxTxBitRatePtr = txRate;
    }
    else
    {
        // Call is not connected. Return LE_UNAVAILABLE
        LE_WARN("Call is not connected");
        return LE_UNAVAILABLE;
    }
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcStartSessionSync
(
    taf_dcs_ProfileRef_t profileRef,
    le_msg_SessionRef_t clientRef
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    uint32_t profileId = 0;
    uint8_t  phoneId   = 0;
    taf_dcs_Pdp_t pdpIpType;
    taf_dcs_ConState_t connState;
    le_result_t result;

    result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed: %d", TO_INT(result));
    result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed: %d", TO_INT(result));
    result = profile.GetPdp(pdpIpType);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPdp failed: %d", TO_INT(result));
    taf_dcs_ConState_t ipv4state, ipv6state;
    result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));
    LE_DEBUG("Phone Id: %d, Profile Id: %d, PDP: %d", phoneId, profileId,TO_INT(pdpIpType));
    LE_DEBUG("State: %d", TO_INT(connState));

    if (TAF_DCS_CONNECTED == connState)
    {
        LE_INFO("Already connected.");
        return LE_DUPLICATE;
    }
    if (TAF_DCS_CONNECTING == connState)
    {
        LE_INFO("Connection in progress");
        return LE_IN_PROGRESS;
    }
    taf::pa::data::DataCallStartStopParams_t params =
    {
        static_cast<taf::pa::data::PhoneId_e>(phoneId),
        static_cast<taf::pa::data::ProfileId_e>(profileId),
        TafDcsUtils::ConvertPDP(pdpIpType),
        ""
    };
    result = taf::pa::data::StartDataSessionAsync(params);
    if (LE_OK != result)
    {
        LE_WARN("StartDataSession failed: %d", TO_INT(result));
        return result;
    }
    // Mark that a data call has been setup
    profile.SetCallSetup(true);

    // Wait for success to be declared via taf_pa_data_CallEventsCb callback

    // Init the promise
    syncCmdPromise_ = std::promise<le_result_t>();
    std::future<le_result_t> fut = syncCmdPromise_.get_future();
    isSyncCmdPromiseWaiting_.store(true);

    // Wait for the promise to be fulfilled

    if (fut.wait_for(std::chrono::seconds(syncSessionCmdTimeout_)) == std::future_status::ready)
    {
        result = fut.get();
    }
    else
    {
        // Timeout. Mark that future is no longer needed.
        LE_WARN("Timeout waiting for result.");
        isSyncCmdPromiseWaiting_.store(false);

        // Update the session state to DISCONNECTED
        profile.SetSessionState(TAF_DCS_DISCONNECTED, TAF_DCS_DISCONNECTED, TAF_DCS_DISCONNECTED);
        result = LE_TIMEOUT;
    }
    LE_DEBUG("Result: %d", TO_INT(result));
    if (LE_OK == result)
    {
        // Ensure data state is not disconnected.
        result = profile.GetSessionState(connState, ipv4state, ipv6state);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));
        LE_DEBUG("Phone Id: %d, Profile Id: %d, PDP: %d", phoneId, profileId,TO_INT(pdpIpType));
        LE_DEBUG("State: %d", TO_INT(connState));
        if (TAF_DCS_DISCONNECTED == connState)
        {
            LE_WARN ("StartDataSessionAsync did not succeed.");
            return LE_TERMINATED;
        }

        // Add the client to the list of clients that have requested data.
        size_t listSize = 0;
        profile.AddClient(clientRef, listSize);
        LE_DEBUG("Client %p added. Num clients: %zu", clientRef, listSize);
    }
    return result;
}

void TafDcsProfileManager::SvcStartSessionASync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void *contextPtr,
    le_msg_SessionRef_t clientRef
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerPtr, "handlerPtr is NULL");

    auto profileOptWrapper = getProfile(profileRef);
    if (!profileOptWrapper.has_value())
    {
        LE_ERROR("profileRef %p not found", profileRef);
        handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
        return;
    }
    TafDcsProfile &profile = profileOptWrapper.value().get();

    auto &tafDcsSvc = TafDcsSvc::GetInstance();

    if (!profile.AddStartSessionAsyncClient(clientRef, handlerPtr, contextPtr))
    {
        LE_WARN("Client already in start async list.");
    }

    TafDcsSendStartSessionAsyncRsp_t response;
    response.clientRef  = clientRef;
    response.profileRef = profileRef;

    uint32_t profileId = 0;
    uint8_t phoneId = 0;
    taf_dcs_Pdp_t pdpIpType;
    taf_dcs_ConState_t connState;
    le_result_t result;

    result = profile.GetId(profileId);
    if (LE_OK != result)
    {
        LE_WARN ("GetId failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not created yet
        LE_WARN("Profile is not created yet");
        response.result = LE_NOT_POSSIBLE;
        le_event_Report(
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t));
        return;
    }

    result = profile.GetPhoneId(phoneId);
    if (LE_OK != result)
    {
        LE_WARN("GetPhoneId failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    result = profile.GetPdp(pdpIpType);
    if (LE_OK != result)
    {
        LE_WARN("GetPdp failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    taf_dcs_ConState_t ipv4state, ipv6state;
    result = profile.GetSessionState(connState, ipv4state, ipv6state);
    if (LE_OK != result)
    {
        LE_WARN("GetSessionState failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    LE_DEBUG("Phone Id: %d, Profile Id: %d, PDP: %d", phoneId, profileId, TO_INT(pdpIpType));
    LE_DEBUG("State: %d", TO_INT(connState));

    if (TAF_DCS_CONNECTED == connState)
    {
        LE_INFO("Already connected.");
        response.result = LE_DUPLICATE;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        // Add this client to the list of clients that have requested data.
        size_t listSize;
        profile.AddClient(clientRef, listSize);
        LE_DEBUG("Client %p added. Num clients: %zu", clientRef, listSize);
        return;
    }
    if (TAF_DCS_CONNECTING == connState)
    {
        LE_INFO("Connection in progress");
        response.result = LE_IN_PROGRESS;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        // Add this client to the list of clients that have requested data.
        size_t listSize;
        profile.AddClient(clientRef, listSize);
        LE_DEBUG("Client %p added. Num clients: %zu", clientRef, listSize);
        return;
    }

    // Start the call
    taf::pa::data::DataCallStartStopParams_t params =
    {
        static_cast<taf::pa::data::PhoneId_e>(phoneId),
        static_cast<taf::pa::data::ProfileId_e>(profileId),
        TafDcsUtils::ConvertPDP(pdpIpType),
        ""
    };
    result = taf::pa::data::StartDataSessionAsync(params);
    if (LE_OK != result)
    {
        LE_WARN("StartDataSession failed: %d", TO_INT(result));
        response.result = result;
        le_event_Report
        (
            tafDcsSvc.GetStartSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    // Mark that a data call has been setup
    profile.SetCallSetup(true);
    LE_INFO("Session start in progress.");
    return;
}

le_result_t TafDcsProfileManager::SvcStopSessionSync
(
    taf_dcs_ProfileRef_t profileRef,
    le_msg_SessionRef_t clientRef
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);

    TAF_CHECK_IF_PROFILE_IS_CREATED(profile);

    uint32_t profileId = 0;
    uint8_t  phoneId   = 0;
    taf_dcs_Pdp_t pdpIpType;
    taf_dcs_ConState_t connState;
    size_t listSize = 0;

    le_result_t result = profile.GetId(profileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetId failed: %d", TO_INT(result));
    result = profile.GetPhoneId(phoneId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPhoneId failed: %d", TO_INT(result));
    result = profile.GetPdp(pdpIpType);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetPdp failed: %d", TO_INT(result));
    taf_dcs_ConState_t ipv4state, ipv6state;
    result = profile.GetSessionState(connState, ipv4state, ipv6state);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetSessionState failed: %d", TO_INT(result));
    LE_DEBUG("Phone Id: %d, Profile Id: %d, PDP: %d", phoneId, profileId,TO_INT(pdpIpType));
    LE_DEBUG("State: %d", TO_INT(connState));

    if (TAF_DCS_DISCONNECTED == connState)
    {
        LE_WARN("Already disconnected.  No active data call found.");
        // Remove the client from the list of clients that have requested data.
        // The return value does not matter in this scenario.
        profile.RemoveClient(clientRef, listSize);
        LE_DEBUG("Client %p removed. Num clients: %zu", clientRef, listSize);
        return LE_NOT_FOUND;
    }

    // Remove the client from the list of clients that had called StartSession before stopping data.
    listSize = 0;
    result = profile.RemoveClient(clientRef, listSize);
    if (LE_OK != result)
    {
        LE_WARN("Client %p has not requested data. Num clients: %zu", clientRef, listSize);
        return LE_NOT_FOUND;
    }
    LE_DEBUG("Client %p removed. Num clients: %zu", clientRef, listSize);

    if (TAF_DCS_DISCONNECTING == connState)
    {
        LE_INFO("Disconnection in progress");
        return LE_IN_PROGRESS;
    }
    taf::pa::data::DataCallStartStopParams_t params =
    {
        static_cast<taf::pa::data::PhoneId_e>(phoneId),
        static_cast<taf::pa::data::ProfileId_e>(profileId),
        TafDcsUtils::ConvertPDP(pdpIpType),
        ""
    };
    result = taf::pa::data::StopDataSessionAsync(params);

    if (LE_OK != result)
    {
        LE_WARN("StopDataSessionAsync failed: %d", TO_INT(result));
        return result;
    }

    // Wait for success to be declared via taf_pa_data_CallEventsCb callback
    // Init the promise
    syncCmdPromise_ = std::promise<le_result_t>();
    std::future<le_result_t> fut = syncCmdPromise_.get_future();
    isSyncCmdPromiseWaiting_.store(true);

    // Wait for the promise to be fulfilled.
    if (fut.wait_for(std::chrono::seconds(syncSessionCmdTimeout_)) == std::future_status::ready)
    {
        result = fut.get();
    }
    else
    {
        // Timeout. Mark that future is no longer needed.
        LE_WARN("Timeout waiting for result.");
        isSyncCmdPromiseWaiting_.store(false);
        result = LE_TIMEOUT;
    }
    LE_DEBUG("Result: %d", TO_INT(result));
    return result;
}

void TafDcsProfileManager::SvcStopSessionASync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void *contextPtr,
    le_msg_SessionRef_t clientRef
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerPtr, "handlerPtr is NULL");

    auto profileOptWrapper = getProfile(profileRef);
    if (!profileOptWrapper.has_value())
    {
        LE_ERROR("profileRef %p not found", profileRef);
        handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
        return;
    }
    TafDcsProfile &profile = profileOptWrapper.value().get();

    auto &tafDcsSvc = TafDcsSvc::GetInstance();

    if (!profile.AddStopSessionAsyncClient(clientRef, handlerPtr, contextPtr))
    {
        LE_WARN("Client already in stop async list.");
    }
    TafDcsSendStopSessionAsyncRsp_t response;
    response.clientRef  = clientRef;
    response.profileRef = profileRef;
    size_t listSize = 0;

    uint32_t profileId = 0;
    uint8_t phoneId = 0;
    taf_dcs_Pdp_t pdpIpType;
    taf_dcs_ConState_t connState;
    le_result_t result;
    result = profile.GetId(profileId);
    if (LE_OK != result)
    {
        LE_WARN ("GetId failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        // Profile is not created yet
        LE_WARN("Profile is not created yet");
        response.result = LE_NOT_POSSIBLE;
        le_event_Report(
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t));
        return;
    }

    result = profile.GetPhoneId(phoneId);
    if (LE_OK != result)
    {
        LE_WARN("GetPhoneId failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    result = profile.GetPdp(pdpIpType);
    if (LE_OK != result)
    {
        LE_WARN("GetPdp failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    taf_dcs_ConState_t ipv4state, ipv6state;
    result = profile.GetSessionState(connState, ipv4state, ipv6state);
    if (LE_OK != result)
    {
        LE_WARN("GetSessionState failed: %d", TO_INT(result));
        response.result = LE_FAULT;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    LE_DEBUG("Phone Id: %d, Profile Id: %d, PDP: %d", phoneId, profileId, TO_INT(pdpIpType));
    LE_DEBUG("State: %d", TO_INT(connState));

    if (TAF_DCS_DISCONNECTED == connState)
    {
        LE_WARN("Already disconnected. No active data call found.");
        response.result = LE_NOT_FOUND;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        // Remove this client from the list of clients that have requested data.
        // The return value does not matter in this scenario.
        profile.RemoveClient(clientRef, listSize);
        LE_DEBUG("Client %p removed. Num clients: %zu", clientRef, listSize);
        return;
    }
    if (TAF_DCS_DISCONNECTING == connState)
    {
        LE_INFO("Disconnection in progress");
        response.result = LE_IN_PROGRESS;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
    }

    // Remove the client from the list of clients that had called StartSession before stopping data.
    listSize = 0;
    result = profile.RemoveClient(clientRef, listSize);
    if (LE_OK != result)
    {
        LE_WARN("Client %p has not requested data. Num clients: %zu", clientRef, listSize);
        response.result = LE_NOT_FOUND;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    LE_DEBUG("Num clients: %zu", listSize);

    taf::pa::data::DataCallStartStopParams_t params =
    {
        static_cast<taf::pa::data::PhoneId_e>(phoneId),
        static_cast<taf::pa::data::ProfileId_e>(profileId),
        TafDcsUtils::ConvertPDP(pdpIpType),
        ""
    };
    result = taf::pa::data::StopDataSessionAsync(params);

    if (LE_OK != result)
    {
        LE_WARN("StopDataSessionAsync failed: %d", TO_INT(result));
        response.result = result;
        le_event_Report
        (
            tafDcsSvc.GetStopSessionAsyncRspEvtId(),
            &response,
            sizeof(TafDcsSendStartSessionAsyncRsp_t)
        );
        return;
    }
    LE_INFO("Session stop in progress.");
    return;
}

// Get the selected slot, the matching phone ID and then default profile for that phone
le_result_t TafDcsProfileManager::SvcGetDefaultPhoneIdAndProfileId
(
   uint8_t *phoneIdPtr,
   uint32_t *profileIdPtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == phoneIdPtr, LE_BAD_PARAMETER, "phoneIdPtr is null!");
    TAF_ERROR_IF_RET_VAL(nullptr == profileIdPtr, LE_BAD_PARAMETER, "profileIdPtr is null!");

    taf::pa::data::PhoneId_e phoneId = taf::pa::data::PhoneId_e::PHONE_1;
    LE_DEBUG("Phone ID: %d", TO_INT(phoneId));
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(defaultProfileIdMapMutex_);
    // Get the default profile id
    auto defaultProfileIdOpt = defaultProfileIdMap_.find(static_cast<uint8_t>(phoneId));
    if (defaultProfileIdOpt == defaultProfileIdMap_.end())
    {
        LE_ERROR("Default profile id not found for phone %d", static_cast<uint8_t>(phoneId));
        return LE_NOT_FOUND;
    }
    *phoneIdPtr = static_cast<uint8_t>(phoneId);
    *profileIdPtr = defaultProfileIdOpt->second;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetDefaultProfileIndexEx
(
    uint8_t phoneId,
    uint32_t *profileIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == profileIdPtr, LE_BAD_PARAMETER, "profileIdPtr is null!");
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(defaultProfileIdMapMutex_);
    // Get the default profile id
    auto defaultProfileIdOpt = defaultProfileIdMap_.find(phoneId);
    if (defaultProfileIdOpt == defaultProfileIdMap_.end())
    {
        LE_ERROR("Default profile id not found for phone %d", phoneId);
        return LE_NOT_FOUND;
    }
    *profileIdPtr = defaultProfileIdOpt->second;
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcSetDefaultProfileIndexEx(uint8_t phoneId, uint32_t profileId)
{
    TAF_ERROR_IF_RET_VAL(0 == phoneId, LE_BAD_PARAMETER, "phoneId is 0!");
    TAF_ERROR_IF_RET_VAL(0 == profileId, LE_BAD_PARAMETER, "profileId is 0!");

    {
        // Check if the phone ID is valid
        // Get a read lock
        std::shared_lock<std::shared_mutex> lock(defaultProfileIdMapMutex_);
        // Get the default profile id
        auto defaultProfileIdOpt = defaultProfileIdMap_.find(phoneId);
        if (defaultProfileIdOpt == defaultProfileIdMap_.end())
        {
            LE_ERROR("Default profile id not found for phone %d", phoneId);
            return LE_NOT_FOUND;
        }
    }
    taf::pa::data::PhoneId_e   paPhoneId   = static_cast<taf::pa::data::PhoneId_e>(phoneId);
    taf::pa::data::ProfileId_e paProfileId = static_cast<taf::pa::data::ProfileId_e>(profileId);
    le_result_t result = taf::pa::data::SetDefaultProfile(paPhoneId, paProfileId);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetDefaultProfile failed: %d", TO_INT(result));

    // Update local default profile ID map after getting a write lock.
    std::unique_lock<std::shared_mutex> lock(defaultProfileIdMapMutex_);
    defaultProfileIdMap_[phoneId] = profileId;

    return result;
}

le_result_t TafDcsProfileManager::getMtu(const std::string &ifNameStr, uint16_t &mtu)
{
    struct ifreq ifr;
    int8_t sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    TAF_ERROR_IF_RET_VAL(sock < 0, LE_FAULT, "socket error %d", sock);

    memset(&ifr, 0, sizeof(struct ifreq));
    le_result_t result = le_utf8_Copy(ifr.ifr_name, ifNameStr.c_str(), sizeof(ifr.ifr_name), NULL);
    TAF_ERROR_IF_RET_VAL(result == LE_OVERFLOW, LE_OVERFLOW,
                                                "IOCTL interface name length is smaller");
    if (ioctl(sock, SIOCGIFMTU, &ifr) < 0)
    {
        LE_ERROR("ioctl get error %d error:%s", errno, strerror(errno));
        close(sock);
        return LE_IO_ERROR;
    }

    mtu = static_cast<uint16_t>(ifr.ifr_mtu);
    LE_DEBUG("MTU for %s: %d", ifNameStr.c_str(), mtu);

    close(sock);
    return LE_OK;
}

le_result_t TafDcsProfileManager::SvcGetMtu(taf_dcs_ProfileRef_t profileRef, uint16_t *mtuPtr)
{
    le_result_t result;
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == mtuPtr, LE_BAD_PARAMETER, "mtuPtr is NULL");

    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, LE_NOT_FOUND);
    TAF_CHECK_IF_PROFILE_IS_CREATED (profile);

    // Check call status. Should be connected.
    taf_dcs_ConState_t callState, dummyState;
    result = profile.GetSessionState(callState, dummyState, dummyState);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "Failed to get session state");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_CONNECTED != callState, LE_NOT_POSSIBLE, "Not connected");

    // Get the interface name
    std::string ifName;
    result = profile.GetHostInterface(ifName);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetHostInterface failed");

    uint16_t mtu = 0;
    result = getMtu(ifName, mtu);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "getMtu failed");

    *mtuPtr = mtu;
    return LE_OK;
}

bool TafDcsProfileManager::SvcIsIPv4(taf_dcs_ProfileRef_t profileRef)
{
    le_result_t result;
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, false, "profileRef is NULL");
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, false);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_FALSE(profile);

    // Check if IPv4 session is connected.
    taf_dcs_ConState_t callState, ipv4State, dummyState;
    result = profile.GetSessionState(callState, ipv4State, dummyState);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, false, "Failed to get session state");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_CONNECTED != callState && TAF_DCS_CONNECTED != ipv4State,
                                                            false, "IPv4 call is not connected");
    if (TAF_DCS_CONNECTED == callState && TAF_DCS_CONNECTED == ipv4State)
    {
        LE_INFO ("IPv4 call is connected");
        return true;
    }
    LE_WARN("IPv4 call is not connected.");
    return false;
}

bool TafDcsProfileManager::SvcIsIPv6(taf_dcs_ProfileRef_t profileRef)
{
    le_result_t result;
    TAF_ERROR_IF_RET_VAL(nullptr == profileRef, false, "profileRef is NULL");
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, false);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_FALSE(profile);

    // Check if IPv6 session is connected.
    taf_dcs_ConState_t callState, ipv6State, dummyState;
    result = profile.GetSessionState(callState, dummyState, ipv6State);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, false, "Failed to get session state");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_CONNECTED != callState && TAF_DCS_CONNECTED != ipv6State,
                                                        false, "IPv6 call is not connected");
    if (TAF_DCS_CONNECTED == callState && TAF_DCS_CONNECTED == ipv6State)
    {
        LE_INFO("IPv6 call is connected");
        return true;
    }
    LE_WARN("IPv6 call is not connected.");
    return false;
}

/**
 * The handler from which RoamingStatus event will be sent to registered clients.
 */
void TafDcsProfileManager::firstRoamingStatusHandler(void *reportPtr, void *clientHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == nullptr, "clientHandlerFunc is NULL");
    taf_dcs_RoamingStatusInd_t *eventPtr = static_cast<taf_dcs_RoamingStatusInd_t *>(reportPtr);
    taf_dcs_RoamingStatusHandlerFunc_t handlerFunc =
                    reinterpret_cast<taf_dcs_RoamingStatusHandlerFunc_t>(clientHandlerFunc);
    // Call the client handler
    handlerFunc(eventPtr, le_event_GetContextPtr());
}

taf_dcs_RoamingStatusHandlerRef_t TafDcsProfileManager::SvcAddRoamingStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_RoamingStatusHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    // Call the TafDcsProfile API with the object reference to get the profile's TAF reference
    le_event_Id_t roamingEvent = TafDcsProfile::GetRoamingStateChangedEventId();

    // This API does not return on errors
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                                    "RoamingStatus",
                                                                    roamingEvent,
                                                                    firstRoamingStatusHandler,
                                                                    (void *)handlerPtr
                                                                );

    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_dcs_RoamingStatusHandlerRef_t)(handlerRef);
}

void TafDcsProfileManager::SvcRemoveRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}

/**
 * The handler from which data session state events will be sent to registered clients.
 * This is declared as static and has limited scope within this file.
 */
void TafDcsProfileManager::firstDataCallSessionStateHandler
(
    void *reportPtr,
    void *clientHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == nullptr, "clientHandlerFunc is NULL");
    TafDcsSessionStateChangedEvent_t *eventPtr =
                                static_cast<TafDcsSessionStateChangedEvent_t *>(reportPtr);
    taf_dcs_SessionStateHandlerFunc_t handlerFunc =
                            reinterpret_cast<taf_dcs_SessionStateHandlerFunc_t>(clientHandlerFunc);
    taf_dcs_StateInfo_t stateInfo;
    stateInfo.ipType = eventPtr->ipType;

    LE_DEBUG("Call SessionStateHandler client function: %p", handlerFunc);
    // Call the client callback
    handlerFunc
    (
        eventPtr->profileRef,
        eventPtr->connState,
        &stateInfo,
        le_event_GetContextPtr()
    );
}

taf_dcs_SessionStateHandlerRef_t TafDcsProfileManager::SvcAddSessionStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_SessionStateHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, nullptr);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_VAL(profile, nullptr);

    // Call the TafDcsProfile API with the object reference to get the profile's TAF reference
    le_event_Id_t sessionEvent = profile.GetSessionStateChangedEventId();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                                   "DataCallSessionState",
                                                                   sessionEvent,
                                                                   firstDataCallSessionStateHandler,
                                                                   (void *)handlerPtr
                                                                );

    LE_DEBUG("SessionStateHandler client func: %p", handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_SessionStateHandlerRef_t)(handlerRef);
}

void TafDcsProfileManager::SvcRemoveSessionStateHandler
(
    taf_dcs_SessionStateHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}

/**
 * The handler from which QoS status events will be sent to registered clients.
 * This is declared as static and has limited scope within this file.
 */
void TafDcsProfileManager::firstQosStatusHandler(void *reportPtr, void *clientHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == nullptr, "clientHandlerFunc is NULL");
    taf_dcs_QosTftEvent_t *eventPtr = static_cast<taf_dcs_QosTftEvent_t *>(reportPtr);
    taf_dcs_QosStatusHandlerFunc_t handlerFunc =
        reinterpret_cast<taf_dcs_QosStatusHandlerFunc_t>(clientHandlerFunc);
    // Call the client handler
    handlerFunc(eventPtr->qosFlowRef, eventPtr->qosState, le_event_GetContextPtr());
}

taf_dcs_QosStatusHandlerRef_t TafDcsProfileManager::SvcAddQosStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_QosStatusHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, nullptr);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_VAL(profile, nullptr);

    le_event_Id_t qosEvent = profile.GetQosStatusChangedEventId();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                                    "QosStatusHandler",
                                                                    qosEvent,
                                                                    firstQosStatusHandler,
                                                                    (void *)handlerPtr
                                                                );

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_QosStatusHandlerRef_t)(handlerRef);
}

void TafDcsProfileManager::SvcRemoveQosStatusHandler(taf_dcs_QosStatusHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}

/**
 * The handler from which HW acceleration state events will be sent to registered clients.
 * This is declared as static and has limited scope within this file.
 */
void TafDcsProfileManager::firstHwAccelerationStateHandler(void *reportPtr, void *clientHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == nullptr, "clientHandlerFunc is NULL");

    taf_dcs_HwAccelerationEvent_t *eventPtr = static_cast<taf_dcs_HwAccelerationEvent_t *>(
                                                                                        reportPtr);
    taf_dcs_HwAccelerationStateHandlerFunc_t handlerFunc =
                reinterpret_cast<taf_dcs_HwAccelerationStateHandlerFunc_t>(clientHandlerFunc);
    // Call the client handler
    LE_DEBUG ("Call client hw acceleration callback.");
    handlerFunc(eventPtr->profileRef, eventPtr->state, le_event_GetContextPtr());
}

taf_dcs_HwAccelerationStateHandlerRef_t TafDcsProfileManager::SvcAddHwAccelerationStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_HwAccelerationStateHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, nullptr);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_VAL(profile, nullptr);
    // Call the TafDcsProfile API with the object reference to get the profile's TAF reference
    le_event_Id_t hwAccelerationEvent = profile.GetHwAccelStateChangedEventId();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                                    "HwAccelerationStateHandler",
                                                                    hwAccelerationEvent,
                                                                    firstHwAccelerationStateHandler,
                                                                    (void *)handlerPtr
                                                                );

    le_event_SetContextPtr(handlerRef, contextPtr);

    // Add this handler to the handler profile ref map
    std::unique_lock<std::shared_mutex> lock(hwAccProfileRefMapMutex_);
    std::pair<le_event_HandlerRef_t, taf_dcs_ProfileRef_t> entry(handlerRef, profileRef);
    hwAccProfileRefMap_.insert(entry);

    return (taf_dcs_HwAccelerationStateHandlerRef_t)(handlerRef);
}

void TafDcsProfileManager::SvcRemoveHwAccelerationStateHandler
(
    taf_dcs_HwAccelerationStateHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    // Remove this handler from the handler profile ref map
    std::unique_lock<std::shared_mutex> lock(hwAccProfileRefMapMutex_);
    le_event_HandlerRef_t key = (le_event_HandlerRef_t)handlerRef;
    if (hwAccProfileRefMap_.find(key) != hwAccProfileRefMap_.end())
    {
        // Key exists, erase it
        hwAccProfileRefMap_.erase(key);
        LE_DEBUG("Entry erased successfully.");
    }
    return;
}

/**
 * The handler from which PDN throttled state events will be sent to registered clients.
 * This is declared as static and has limited scope within this file.
 */
void TafDcsProfileManager::firstThrottleStatusHandler(void *reportPtr, void *clientHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is NULL");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == nullptr, "clientHandlerFunc is NULL");
    taf_dcs_ThrottledApnEvent_t *eventPtr = static_cast<taf_dcs_ThrottledApnEvent_t *>(reportPtr);
    taf_dcs_ThrottledStatusHandlerFunc_t handlerFunc =
        reinterpret_cast<taf_dcs_ThrottledStatusHandlerFunc_t>(clientHandlerFunc);
    // Call the client handler
    handlerFunc
    (
        eventPtr->profileRef,
        eventPtr->isthrottled,
        eventPtr->ipv4Time,
        eventPtr->ipv6Time,
        le_event_GetContextPtr()
    );
}

taf_dcs_ThrottledStatusHandlerRef_t TafDcsProfileManager::SvcAddThrottledStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ThrottledStatusHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    // Get a reference (profile) to TafDcsProfile object object that matches profileRef
    GET_DCS_PROFILE_FROM_REF_RET_VAL(profileRef, nullptr);
    TAF_CHECK_IF_PROFILE_IS_CREATED_RET_VAL(profile, nullptr);

    // Call the TafDcsProfile API with the object reference to get the profile's TAF reference
    le_event_Id_t throttledStatusEvent = profile.GetThrottledStatusEventId();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                                    "ThrottledStatusHandler",
                                                                    throttledStatusEvent,
                                                                    firstThrottleStatusHandler,
                                                                    (void *)handlerPtr
                                                                );

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_ThrottledStatusHandlerRef_t)(handlerRef);
}

void TafDcsProfileManager::SvcRemoveThrottledStatusHandler
(
    taf_dcs_ThrottledStatusHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}

le_result_t TafDcsProfileManager::GetPhones(std::vector<taf::pa::data::PhoneId_e> &phones) const
{
    phones = phoneIds_;
    return LE_OK;
}

// Update profile in the service's maps
le_result_t TafDcsProfileManager::updateProfile(const TafDcsUpdateProfileEvent_t *eventPtr)
{
    le_result_t result;
    // Get the optional profile reference wrapper
    auto profileOptWrapper = getProfile(eventPtr->slotId, eventPtr->profileInfo.id);
    if (profileOptWrapper.has_value())
    {
        TafDcsProfile &profile = profileOptWrapper.value().get();
        // This is an existing profile. Update the details in the profile.
        LE_WARN("Profile [%d, %d]found. Updating existing profile.",
                                            eventPtr->slotId, eventPtr->profileInfo.id);

        result = profile.SetApn(eventPtr->profileInfo.apn);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetApn failed: %d", TO_INT(result));
        result = profile.SetName(eventPtr->profileInfo.name);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetName failed: %d", TO_INT(result));
        result = profile.SetUserName(eventPtr->profileInfo.userName);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetUserName failed: %d", TO_INT(result));
        result = profile.SetPassword(eventPtr->profileInfo.password);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetPassword failed: %d", TO_INT(result));
        result = profile.SetTech(eventPtr->profileInfo.techPref);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetTech failed: %d", TO_INT(result));
        result = profile.SetAuthTypeBitmask(eventPtr->profileInfo.authTypeBitmask);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetAuthTypeBitmask failed: %d", TO_INT(result));
        result = profile.SetPdp(eventPtr->profileInfo.ipType);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetPdp failed: %d", TO_INT(result));
        result = profile.SetApnTypeBitmask(eventPtr->profileInfo.apnTypeMask);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetApnTypeBitmask failed: %d", TO_INT(result));
        result = profile.SetEmergencyCallSupport(eventPtr->profileInfo.emergencyCallSupport);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,
                                            "SetEmergencyCallSupport failed: %d", TO_INT(result));

        return result;
    }

    // Create a new profile
    taf::pa::data::PhoneId_e phoneId = taf::pa::data::PhoneId_e::PHONE_1;
    taf::pa::data::SlotId_e  slotID = static_cast<taf::pa::data::SlotId_e>(eventPtr->slotId);
    result = taf::pa::data::GetPhoneIdFromSimSlotId(slotID, phoneId);
    if (LE_OK != result)
    {
        LE_WARN("PA GetPhoneIdFromSimSlotId failed: %d", TO_INT(result));
        // Use default phone ID 1
        phoneId = taf::pa::data::PhoneId_e::PHONE_1;
    }

    // Create a new profile
    auto tafDcsProfile = std::make_shared<TafDcsProfile> ( eventPtr->slotId,
                                                            static_cast<uint8_t>(phoneId),
                                                            eventPtr->profileInfo);
    LE_INFO("Profile [%d, %d] not found. Created new profile.",
                                                        eventPtr->slotId, eventPtr->profileInfo.id);

    // Add the newly created profile to maps.
    if (!addToProfilesMap(eventPtr->profileInfo.id, static_cast<uint8_t>(phoneId), tafDcsProfile))
    {
        LE_ERROR("Profiles map not updated");
        return LE_FAULT;
    }
    if (!addToProfileRefsMap(tafDcsProfile->GetReference(), tafDcsProfile))
    {
        LE_ERROR("Profile refs map not updated");
        return LE_FAULT;
    }

    LE_DEBUG("Profile and maps updated.");
    return LE_OK;
}

// Update the data connection state of a profile
le_result_t TafDcsProfileManager::updateSessionDetails(const TafDcsSessionChangeEvent_t *eventPtr)
{
    // Get the profile object based on phone ID and profile ID
    GET_DCS_PROFILE_FROM_ID_RET_VAL(
            eventPtr->profile.phoneId, eventPtr->profile.profileId, LE_NOT_FOUND);
    le_result_t result = profile.SetSessionState(
                            eventPtr->connState, eventPtr->ipv4ConnState, eventPtr->ipv6ConnState);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"SetSessionState failed: %d", TO_INT(result));
    if (TAF_DCS_CONNECTED == eventPtr->connState)
    {
        profile.ResetCallEndReason();
    }
    else
    {
        result = profile.SetCallEndReason(eventPtr->callEndReasonType,
                                          eventPtr->callEndReasonCode,
                                          eventPtr->ipv4CallEndReasonType,
                                          eventPtr->ipv6CallEndReasonType);
    }
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetCallEndReason failed: %d", TO_INT(result));

    if (TAF_DCS_CONNECTED == eventPtr->connState)
    {
        result = profile.SetDataBearerTech(eventPtr->bearerTech);
    }
    else
    {
        profile.ResetDataBearerTech();
    }
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetDataBearerTech failed: %d", TO_INT(result));

    if (TAF_DCS_CONNECTED == eventPtr->connState && TAF_DCS_CONNECTED == eventPtr->ipv4ConnState)
    {
        result = profile.SetIPv4Addresses(
                                            std::string(eventPtr->ipv4Addr),
                                            std::string(eventPtr->ipv4gwAddr),
                                            std::string(eventPtr->ipv4PrimaryDnsAddr),
                                            std::string(eventPtr->ipv4SecondaryDnsAddr),
                                            eventPtr->ipv4AddrMask,
                                            eventPtr->ipv4gwAddrMask);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result,"SetIPv4Addresses failed: %d", TO_INT(result));
    }
    else if (TAF_DCS_DISCONNECTED == eventPtr->ipv4ConnState)
    {
        profile.ResetIPv4Addresses();
    }

    if (TAF_DCS_CONNECTED == eventPtr->connState && TAF_DCS_CONNECTED == eventPtr->ipv6ConnState)
    {
        result = profile.SetIPv6Addresses(
            std::string(eventPtr->ipv6Addr),
            std::string(eventPtr->ipv6gwAddr),
            std::string(eventPtr->ipv6PrimaryDnsAddr),
            std::string(eventPtr->ipv6SecondaryDnsAddr),
            eventPtr->ipv6AddrMask,
            eventPtr->ipv6gwAddrMask);
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetIPv6Addresses failed: %d",TO_INT(result));
    }
    else if (TAF_DCS_DISCONNECTED == eventPtr->ipv6ConnState)
    {
        profile.ResetIPv6Addresses();
    }

    if (TAF_DCS_CONNECTED == eventPtr->connState)
    {
        result = profile.SetHostInterface(std::string(eventPtr->hostIfName));
        TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetHostInterface failed: %d",TO_INT(result));
    }
    else
    {
        profile.ResetHostInterface();
    }

    // Set the max bit rate if the data state is conencted or conenction and the values are not 0.
    if (TAF_DCS_CONNECTED == eventPtr->connState || TAF_DCS_CONNECTING == eventPtr->connState)
    {
        if ( 0 != eventPtr->maxRxBitRate || 0 != eventPtr->maxTxBitRate)
        {
            LE_DEBUG ("Updated data bit rates.");
            result = profile.SetMaxDataBitRates(eventPtr->maxRxBitRate, eventPtr->maxTxBitRate);
            TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "SetMaxDataBitRates failed: %d",
                                                                                    TO_INT(result));
        }
        else
        {
            LE_DEBUG ("Skip updating data bit rates.");
        }
    }

    return result;
}

/**************************************************************************************************/
// Functions to send events to registered clients
/**************************************************************************************************/

// Send Session state change events to clients
le_result_t TafDcsProfileManager::sendSessionSateEvent(const TafDcsSessionChangeEvent_t &eventPtr)
{
    // Get the profile object based on phone ID and profile ID
    GET_DCS_PROFILE_FROM_ID_RET_VAL(eventPtr.profile.phoneId, eventPtr.profile.profileId,
                                                                                    LE_NOT_FOUND);

    // Fill in the event to be sent
    TafDcsSessionStateChangedEvent_t event;
    event.profileRef = profile.GetReference();
    event.connState  = eventPtr.connState;
    event.ipType     = eventPtr.ipType_pdp;

    // Send the event
    le_event_Report
    (
        profile.GetSessionStateChangedEventId(),
        &event,
        sizeof(TafDcsSessionStateChangedEvent_t)
    );
    return LE_OK;
}

le_result_t TafDcsProfileManager::sendRoamingEvent(const TafDcsRoamingStatus_t *eventPtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == eventPtr, LE_BAD_PARAMETER, "eventPtr is NULL!");

    // Transform to external DCS struct
    taf_dcs_RoamingStatusInd_t event =
    {
        eventPtr->phoneId,
        eventPtr->isRoaming,
        eventPtr->type
    };

    // Send the external event
    le_event_Report(TafDcsProfile::GetRoamingStateChangedEventId(), &event,
                                                            sizeof(taf_dcs_RoamingStatusInd_t));
    return LE_OK;
}

le_result_t TafDcsProfileManager::sendThrottledApnEvent
(
    const TafDcsThrottledApnEventInfo_t *eventPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == eventPtr, LE_BAD_PARAMETER, "eventPtr is NULL!");

    GET_DCS_PROFILE_FROM_ID_RET_VAL(eventPtr->phoneId, eventPtr->profileId, LE_NOT_FOUND);

    // Transform to external DCS struct
    taf_dcs_ThrottledApnEvent_t event;
    event.profileRef  = profile.GetReference();
    event.ipv4Time    = eventPtr->ipv4Time;
    event.ipv6Time    = eventPtr->ipv6Time;
    event.isthrottled = eventPtr->isBlockedOnAllPLMNs;

    // Send the external event
    le_event_Report(profile.GetThrottledStatusEventId(), &event,
                                                            sizeof(taf_dcs_ThrottledApnEvent_t));
    return LE_OK;
}

le_result_t TafDcsProfileManager::sendHwAccelerationEvent
(
    const TafDcsHwAccelerationChangeEvent_t *eventPtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == eventPtr, LE_BAD_PARAMETER, "eventPtr is NULL!");

    // Transform to external DCS struct
    taf_dcs_HwAccelerationEvent_t event;
    event.state = eventPtr->state;
    LE_DEBUG("State: %d", TO_INT(eventPtr->state));

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    // Get a read lock to the maps of HW acceleration profile refs
    std::shared_lock<std::shared_mutex> lock(tafDcsProfileManager.hwAccProfileRefMapMutex_);
    for (auto &hwAccProfileRef : tafDcsProfileManager.hwAccProfileRefMap_)
    {
        event.profileRef = hwAccProfileRef.second;
        LE_DEBUG("Profile ref: %p", event.profileRef);

        // Get the profile object for the profileRef
        auto profileOptWrapper = tafDcsProfileManager.getProfile(event.profileRef);
        TAF_ERROR_IF_RET_VAL((!profileOptWrapper.has_value()), LE_NOT_FOUND,
                                                        "profileRef %p not found",event.profileRef);
        TafDcsProfile &profile = profileOptWrapper.value().get();
        // Send the external event
        le_event_Report(profile.GetHwAccelStateChangedEventId(),&event,
                                                            sizeof(taf_dcs_HwAccelerationEvent_t));
    }
    return LE_OK;
}

le_result_t TafDcsProfileManager::sendQosTftEvent(const TafDcsQosTftEventInfo_t *eventPtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == eventPtr, LE_BAD_PARAMETER, "eventPtr is NULL!");

    GET_DCS_PROFILE_FROM_ID_RET_VAL(eventPtr->phoneId, eventPtr->profileId, LE_NOT_FOUND);

    // Transform to external DCS struct
    taf_dcs_QosTftEvent_t event;

    LE_WARN ("QoS flow reference is NULL. TODO.");
    event.qosFlowRef = nullptr;
    event.qosState   = eventPtr->state;

    // Send the external event
    le_event_Report(profile.GetQosStatusChangedEventId(), &event,sizeof(taf_dcs_QosTftEvent_t));
    return LE_OK;
}

/**************************************************************************************************/
// Callback registration functions
/**************************************************************************************************/
void TafDcsProfileManager::registerInternalEventCallbacks()
{
    // Register the internal event handlers in the context of tafDcsEventsThreadRef_
    auto &tafDcsSvc = TafDcsSvc::GetInstance();

    // clientDisconnectedEvtId_
    LE_DEBUG("clientDisconnectedEvtId_");
    le_event_QueueFunctionToThread(
        tafDcsSvc.GetEventsThreadRef(),
        registerClientDisconnectedEvtHandler,
        NULL, NULL);

    // updateProfileEvtId_
    LE_DEBUG("updateProfileEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerUpdateProfileEvtHandler,
        NULL, NULL
    );

    // sessionStartEvtId_
    LE_DEBUG("sessionStartEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerSessionStartEvtHandler,
        NULL, NULL
    );

    // sessionStopEvtId_
    LE_DEBUG("sessionStopEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerSessionStopEvtHandler,
        NULL, NULL
    );

    // startSessionAsyncRspEvtId_
    LE_DEBUG("startSessionAsyncRspEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerStartSessionAsyncRspEventHandler,
        NULL, NULL
    );

    // stopSessionAsyncRspEvtId_
    LE_DEBUG("stopSessionAsyncRspEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerStopSessionAsyncRspEventHandler,
        NULL, NULL
    );

    // PA events
    // paSessionStateChangeEvtId_
    LE_DEBUG("paSessionStateChangeEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerPaSessionStateChangeEvtHandler,
        NULL, NULL
    );

    // paRoamingChangeEvtId_
    LE_DEBUG("paRoamingChangeEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerPaRoamingEvtHandler,
        NULL, NULL
    );

    // paThrottledAPNsEvtId_
    LE_DEBUG("paThrottledAPNsEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerPaThrottledAPNsEvtHandler,
        NULL, NULL
    );

    // paHwAccelerationChangeEvtId_
    LE_DEBUG("paHwAccelerationChangeEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerPaHwAccelerationEvtHandler,
        NULL, NULL
    );

    // paQosTftEvtId_
    LE_DEBUG("paQosTftEvtId_");
    le_event_QueueFunctionToThread
    (
        tafDcsSvc.GetEventsThreadRef(),
        registerPaQosTftEvtHandler,
        NULL, NULL
    );

    // Queue the function to signal the completion of all events last. This should be the LAST!
    LE_DEBUG("Queue signalEventThreadInitComplete");
    le_event_QueueFunctionToThread(
        tafDcsSvc.GetEventsThreadRef(),
        signalEventThreadInitComplete,
        NULL, NULL);
    LE_DEBUG("Completed.");
}

void TafDcsProfileManager::deinitEventsAndMemory()
{
    LE_WARN("TODO");
}

void TafDcsProfileManager::getProfilesAsyncCb
(
    taf::pa::data::PhoneId_e phoneId,                         ///< [IN] The phone id.
    le_result_t result,                                       ///< [IN] The result of the operation.
    const std::vector<taf::pa::data::ProfileInfo_t> &profiles,///< [IN] The profile list.
    void *contextPtr                                          ///< [IN] The context pointer.
)
{
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "Failed to get profiles for phone Id :%d. result: %d",
                                                                        TO_INT(phoneId),  result);

    LE_DEBUG("Received profiles for phone Id : %d", TO_INT(phoneId));
    LE_DEBUG("Number of profiles             : %zu", profiles.size());

    taf::pa::data::SlotId_e slotId;
    taf::pa::data::GetSimSlotIdFromPhoneId(phoneId, slotId);
    LE_DEBUG("Slot Id : %d", TO_INT(slotId));

    // Locking is not needed here as the PA holds a mutex
    for (const auto &profile : profiles)
    {
        // Process each profile
        TafDcsUpdateProfileEvent_t event;
        event.slotId = static_cast<uint8_t>(slotId);
        event.profileInfo.id = static_cast<uint32_t>(profile.profileId);
        event.profileInfo.techPref = TafDcsUtils::ConvertTechPref(profile.techPref);
        event.profileInfo.authTypeBitmask = TafDcsUtils::ConvertAuthType(profile.authType);
        event.profileInfo.ipType = TafDcsUtils::ConvertPDP(profile.ipType);
        event.profileInfo.apnTypeMask = TafDcsUtils::ConvertApnTypeMask(profile.apnTypeMask);
        event.profileInfo.emergencyCallSupport =
                            TafDcsUtils::ConvertEmergencyCallSupport(profile.emergencyCallSupport);

        memset(event.profileInfo.apn, 0, TAF_DCS_APN_NAME_MAX_BYTES);
        memset(event.profileInfo.name, 0, TAF_DCS_NAME_MAX_BYTES);
        memset(event.profileInfo.userName, 0, TAF_DCS_USER_NAME_MAX_BYTES);
        memset(event.profileInfo.password, 0, TAF_DCS_PASSWORD_NAME_MAX_BYTES);
        LE_WARN_IF(
            le_utf8_Copy(event.profileInfo.apn, profile.apn, TAF_DCS_APN_NAME_MAX_LEN, NULL) ==
                                                                                        LE_OVERFLOW,
            "APN '%s' has been truncated to '%s'.", profile.apn, event.profileInfo.apn);
        LE_WARN_IF(
            le_utf8_Copy(event.profileInfo.name, profile.name, TAF_DCS_NAME_MAX_LEN, NULL) ==
                                                                                        LE_OVERFLOW,
            "Profile name '%s' has been truncated to '%s'.", profile.name, event.profileInfo.name);
        LE_WARN_IF(
            le_utf8_Copy(
                event.profileInfo.userName, profile.userName, TAF_DCS_USER_NAME_MAX_LEN, NULL) ==
                                                                                        LE_OVERFLOW,
            "Username '%s' has been truncated to '%s'.", profile.userName,
                                                                        event.profileInfo.userName);
        LE_WARN_IF(
            le_utf8_Copy(
               event.profileInfo.password, profile.password, TAF_DCS_PASSWORD_NAME_MAX_LEN, NULL) ==
                                                                                        LE_OVERFLOW,
            "Username '%s' has been truncated to '%s'.", profile.password,
                                                                        event.profileInfo.password);

        // Send this event to the DCS internal event handler thread for processing.
        auto &tafDcsSvc = TafDcsSvc::GetInstance();
        le_event_Report(
            tafDcsSvc.GetUpdateProfileEvtId(), // updateProfileEvtId_
            static_cast<void *>(&event),
            sizeof(TafDcsUpdateProfileEvent_t)
        );
    }

    LE_UNUSED(contextPtr);
    return;
}

// Read profiles from the NAD and initialize the profile objects.
void TafDcsProfileManager::initProfiles()
{
    LE_DEBUG("Initializing profiles.");

    le_result_t result;
    for (taf::pa::data::PhoneId_e phoneId : phoneIds_)
    {
        LE_DEBUG("Get profiles for phone ID: %d", TO_INT(phoneId));
        result = taf::pa::data::GetProfilesAsync(phoneId, getProfilesAsyncCb, nullptr);
        if (LE_OK != result)
        {
            LE_WARN("GetProfilesAsync failed: %d", result);
        }
        else
        {
            LE_DEBUG("GetProfilesAsync in progress..");
            // A mutex is not needed here as the callback will come in the context of a new thread
            // and will be serialized via the DCS event handler thread(tafDcsEventsThreadRef_).
        }
    }
}

void TafDcsProfileManager::updateDefaultProfiles()
{
    // Get the default profile ID(s)
    for (taf::pa::data::PhoneId_e phoneId : phoneIds_)
    {
        taf::pa::data::ProfileId_e profileId;
        le_result_t result = taf::pa::data::GetDefaultProfile(phoneId, profileId);
        if (LE_OK == result)
        {
            LE_DEBUG("Def profile for phone ID: %d = %d", TO_INT(phoneId), TO_INT(profileId));
            // Store it in the default profile map after getting a write mutex.
            std::unique_lock<std::shared_mutex> lock(defaultProfileIdMapMutex_);
            defaultProfileIdMap_[static_cast<uint8_t>(phoneId)] = static_cast<uint32_t>(profileId);
        }
        else
        {
            LE_WARN("GetDefaultProfile failed for phone ID: %d: %d", TO_INT(phoneId), result);
        }
    }
}

void TafDcsProfileManager::deinitProfiles()
{
    LE_WARN("TODO");
}

void TafDcsProfileManager::registerPACallbacks()
{
    LE_DEBUG("Register PA callbacks");

    // Register the data events callback
    le_result_t result = taf::pa::data::AddDataCallEventsCallback (tafPaDataCallEventsCb, nullptr,
                                                                           dataEventsCallbackId_);
    LE_DEBUG("AddDataCallEventsCallback, ref: %d, Id: %d", TO_INT(result), dataEventsCallbackId_);

    // Register the roaming events callback
    result = taf::pa::data::AddRoamingEventsCallback (tafPaRoamingEventsCb, nullptr,
                                                                          roamingEventsCallbackId_);
    LE_DEBUG("AddRoamingEventsCallback, ref: %d, Id: %d", TO_INT(result), roamingEventsCallbackId_);

    // Register the throttled APN events callback
    result = taf::pa::data::AddThrottledApnEventsCallback ( tafPaThrottledApnEventsCb, nullptr,
                                                                    throttledApnEventsCallbackId_);
    LE_DEBUG("AddThrottledApnEventsCallback, ref: %d, Id: %d", TO_INT(result),
                                                                    throttledApnEventsCallbackId_);

    // Register the QoS TFT events callback
    result = taf::pa::data::AddQosTftEventsCallback (tafPaQosTftEventsCb, nullptr,
                                                                         qosTftEventsCallbackId_);
    LE_DEBUG("AddQosTftEventsCallback, ref: %d, Id: %d", TO_INT(result), qosTftEventsCallbackId_);

    // Register the HW acceleration events callback
    result = taf::pa::data::AddHwAccelerationChangeEventsCallback(tafPaHwAccelerationEventsCb,
                                                          nullptr, hwAccelerationEventsCallbackId_);
    LE_DEBUG("AddHwAccelerationChangeEventsCallback, ref: %d, Id: %d", TO_INT(result),
                                                                   hwAccelerationEventsCallbackId_);

    LE_DEBUG("PA callback registrations complete.");
}

void TafDcsProfileManager::deregisterPACallbacks()
{
    taf::pa::data::RemoveDataCallEventsCallback(dataEventsCallbackId_);
    dataEventsCallbackId_ = 0;
    taf::pa::data::RemoveRoamingEventsCallback(roamingEventsCallbackId_);
    roamingEventsCallbackId_ = 0;
    taf::pa::data::RemoveThrottledApnEventsCallback(throttledApnEventsCallbackId_);
    throttledApnEventsCallbackId_ = 0;
    taf::pa::data::RemoveQosTftEventsCallback(qosTftEventsCallbackId_);
    qosTftEventsCallbackId_ = 0;
    taf::pa::data::RemoveHwAccelerationChangeEventsCallback(hwAccelerationEventsCallbackId_);
    hwAccelerationEventsCallbackId_ = 0;
}

// Return pointer to matching phone id and profile id
std::optional<std::reference_wrapper<TafDcsProfile>> TafDcsProfileManager::getProfile
(
    uint8_t phoneId,
    uint32_t profileId
)
{
    std::pair<uint32_t, uint8_t> key(profileId, phoneId);

    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(profilesMapMutex_);
    auto iter = profilesMap_.find(key);
    if (iter != profilesMap_.end())
    {
        LE_DEBUG("Profile found. phoneId: %d, Id: %d", phoneId, profileId);
        if (iter->second)
        {
            // Dereference the shared_ptr and return the reference
            return *iter->second;
        }
        else
        {
            LE_ERROR("shared_ptr for phoneId: %d, Id: %d is NULL!", phoneId, profileId);
            return std::nullopt; // Return an empty optional if the shared_ptr is null
        }
    }
    LE_WARN("Profile not found. phoneId: %d, Id: %d", phoneId, profileId);
    return std::nullopt;
}

// Return pointer to matching profile reference
std::optional<std::reference_wrapper<TafDcsProfile>> TafDcsProfileManager::getProfile
(
    taf_dcs_ProfileRef_t profileRef
)
{
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(profilesRefMapMutex_);

    auto iter = profilesRefMap_.find(profileRef);
    if (iter != profilesRefMap_.end())
    {
        LE_DEBUG("Profile found for ref %p.", profileRef);
        if (iter->second)
        {
            // Dereference the shared_ptr and return the reference
            return *iter->second;
        }
        else
        {
            LE_ERROR("shared_ptr for ref %p is NULL!", profileRef);
            return std::nullopt; // Return an empty optional if the shared_ptr is null
        }
    }
    LE_WARN("Profile not found for ref %p.", profileRef);
    return std::nullopt;
}

le_result_t TafDcsProfileManager::deleteProfile
(
    taf_dcs_ProfileRef_t profileRef,
    uint8_t phoneId,
    uint32_t profileId
)
{
    LE_DEBUG("Phone ID: %d, Profile ID: %d", phoneId, profileId);

    // Remove from profilesMap_
    if (!removeFromProfilesMap(profileId, phoneId))
    {
        LE_WARN("removeFromProfilesMap failed");
        return LE_FAULT;
    }

    // Remove from profileRefsMap_
    if (!removeFromProfileRefsMap(profileRef))
    {
        LE_WARN("removeFromProfileRefsMap failed");
        return LE_FAULT;
    }
    return LE_OK;
}

// Function to find all TafDcsProfile entries for a specific Phone ID
std::vector<std::shared_ptr<TafDcsProfile>> TafDcsProfileManager::FindProfilesByPhoneId
(
    uint8_t phoneId
)
{
    std::vector<std::shared_ptr<TafDcsProfile>> matchingProfiles;
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(profilesMapMutex_);
    for (const auto &entry : profilesMap_)
    {
        if (entry.first.second == phoneId)
        {
            matchingProfiles.push_back(entry.second);
        }
    }
    LE_DEBUG("Num profiles found for phone ID %d: %zu", phoneId, matchingProfiles.size());
    return matchingProfiles;
}

// True on success, false on failure. This will update profilesMap_
bool TafDcsProfileManager::addToProfilesMap
(
    uint32_t profileId,
    uint8_t phoneId,
    std::shared_ptr<TafDcsProfile> profile)
{
    // Create a pair to use as the key
    std::pair<uint32_t, uint8_t> key = std::make_pair(profileId, phoneId);

    // Lock
    std::unique_lock<std::shared_mutex> lock(profilesMapMutex_);

    // Check if the key already exists in the map
    if (profilesMap_.find(key) != profilesMap_.end())
    {
        LE_ERROR("Key[%d, %d] already exists.", profileId, phoneId);
        return false; // Indicate that the duplicate was not added
    }
    // Insert the element into the map
    profilesMap_[key] = profile;
    LE_DEBUG("profilesMap_ updated with key[%d, %d]", profileId, phoneId);
    return true;
}

bool TafDcsProfileManager::removeFromProfilesMap(uint32_t profileId, uint8_t phoneId)
{

    // Create a pair to use as the key
    std::pair<uint32_t, uint8_t> key = std::make_pair(profileId, phoneId);

    // Lock
    std::unique_lock<std::shared_mutex> lock(profilesMapMutex_);

    // Find the element in the map
    auto it = profilesMap_.find(key);

    if (it != profilesMap_.end())
    {
        profilesMap_.erase(it);
        LE_INFO("Key[%d, %d] erased from profilesMap_.", profileId, phoneId);
        return true;
    }
    LE_WARN("Key[%d, %d] Not found in profilesMap_.", profileId, phoneId);
    return false;
}

bool TafDcsProfileManager::updateProfilesMapKey
(
    uint32_t oldProfileId,
    uint32_t newProfileId,
    uint8_t phoneId
)
{
    // Create a pair to use as the key
    std::pair<uint32_t, uint8_t> oldKey = std::make_pair(oldProfileId, phoneId);
    std::pair<uint32_t, uint8_t> newKey = std::make_pair(newProfileId, phoneId);

    // Lock
    std::unique_lock<std::shared_mutex> lock(profilesMapMutex_);
    // Find the old key-value pair
    auto it = profilesMap_.find(oldKey);
    if (it != profilesMap_.end())
    {
        // Key exists, create a new key-value pair with the updated key
        LE_INFO("Key[%d, %d] added to profilesMap_.", newProfileId, phoneId);
        profilesMap_.insert({newKey, it->second});

        // Erase the old key-value pair
        LE_INFO("Key[%d, %d] erased from profilesMap_.", oldProfileId, phoneId);
        profilesMap_.erase(it);
        return true;
    }
    LE_WARN("Key[%d, %d] Not found in profilesMap_.", oldProfileId, phoneId);
    return false;
}

// True on success, false on failure. This will update profilesRefMap_
bool TafDcsProfileManager::addToProfileRefsMap
(
    taf_dcs_ProfileRef_t profileRef,
    std::shared_ptr<TafDcsProfile> profile
)
{
    //Lock
    std::unique_lock<std::shared_mutex> lock(profilesRefMapMutex_);

    // Check if the key already exists in the map
    if (profilesRefMap_.find(profileRef) != profilesRefMap_.end())
    {
        LE_ERROR("Key[%p] already exists.", profileRef);
        return false; // Indicate that the duplicate was not added
    }
    // Insert the element into the map
    profilesRefMap_[profileRef] = profile;
    LE_DEBUG("profilesRefMap_ updated with key[%p]", profileRef);
    return true;
}

bool TafDcsProfileManager::removeFromProfileRefsMap(taf_dcs_ProfileRef_t profileRef)
{
    // Lock
    std::unique_lock<std::shared_mutex> lock(profilesRefMapMutex_);
    auto it = profilesRefMap_.find(profileRef);
    if (it != profilesRefMap_.end())
    {
        // Element found, erase it
        LE_INFO("Profile(%p) erased from profilesRefMap_.", profileRef);
        profilesRefMap_.erase(it);
            return true;
    }
    // Element not found
    LE_WARN("Profile(%p) not found in profilesRefMap_.", profileRef);
    return false;
}

/**************************************************************************************************/
// PA callbacks
/**************************************************************************************************/
void TafDcsProfileManager::tafPaDataCallEventsCb
(
    const taf::pa::data::DataCallEventInfo_t & dataCallEventInfo,
    std::shared_ptr<void> context
)
{
    LE_DEBUG("Phone   Id: %d", TO_INT(dataCallEventInfo.phoneId));
    LE_DEBUG("Profile Id: %d", TO_INT(dataCallEventInfo.profileId));
    LE_DEBUG("IP State  : %d", TO_INT(dataCallEventInfo.callStatus));
    LE_DEBUG("IPv4 State: %d", TO_INT(dataCallEventInfo.ipv4DataCallInfo.callStatus));
    LE_DEBUG("IPv6 State: %d", TO_INT(dataCallEventInfo.ipv6DataCallInfo.callStatus));
    LE_DEBUG("IP Type   : %d", TO_INT(dataCallEventInfo.ipType));
    LE_UNUSED(context);

    TafDcsSessionChangeEvent_t event;
    event.profile.phoneId   = TO_INT(dataCallEventInfo.phoneId);
    event.profile.profileId = TO_INT(dataCallEventInfo.profileId);
    event.connState         = TafDcsUtils::ConvertDataCallStatus(dataCallEventInfo.callStatus);
    event.ipType_pdp        = TafDcsUtils::ConvertPDP(dataCallEventInfo.ipType);
    event.techPreference    = TafDcsUtils::ConvertTechPref(dataCallEventInfo.techPref);
    event.bearerTech        = TafDcsUtils::ConvertDataBearerTech(dataCallEventInfo.bearerTech);
    event.maxRxBitRate      = dataCallEventInfo.maxRxBitRate;
    event.maxTxBitRate      = dataCallEventInfo.maxTxBitRate;
    TafDcsUtils::ConvertCallEndReason (
            dataCallEventInfo.callEndReason, event.callEndReasonType, event.callEndReasonCode);
    if (!dataCallEventInfo.hostIfName.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.hostIfName, dataCallEventInfo.hostIfName.c_str(),
                TAF_DCS_APN_NAME_MAX_BYTES, NULL
            ) == LE_OVERFLOW,
            "HostIfName '%s' has been truncated to '%s'.",
            dataCallEventInfo.hostIfName.c_str(), event.hostIfName
        );
    }
    else
    {
        memset(event.hostIfName, 0, TAF_DCS_APN_NAME_MAX_BYTES);
    }

    //IPv4 information
    int32_t dummy;
    event.ipv4ConnState = TafDcsUtils::ConvertDataCallStatus(
                                                    dataCallEventInfo.ipv4DataCallInfo.callStatus);
    TafDcsUtils::ConvertCallEndReason(
            dataCallEventInfo.ipv4DataCallInfo.callEndReason, event.ipv4CallEndReasonType, dummy);
    event.ipv4AddrMask = dataCallEventInfo.ipv4DataCallInfo.ipAddrMask;
    if (!dataCallEventInfo.ipv4DataCallInfo.ipAddr.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv4Addr, dataCallEventInfo.ipv4DataCallInfo.ipAddr.c_str(),
                TAF_DCS_IPV4_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv4Addr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv4DataCallInfo.ipAddr.c_str(), event.ipv4Addr
        );
    }
    else
    {
        memset(event.ipv4Addr, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    }
    event.ipv4gwAddrMask = dataCallEventInfo.ipv4DataCallInfo.gwAddrMask;
        if (!dataCallEventInfo.ipv4DataCallInfo.gwAddr.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv4gwAddr, dataCallEventInfo.ipv4DataCallInfo.gwAddr.c_str(),
                TAF_DCS_IPV4_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv4gwAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv4DataCallInfo.gwAddr.c_str(), event.ipv4gwAddr
        );
    }
    else
    {
        memset(event.ipv4gwAddr, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    }
    if (!dataCallEventInfo.ipv4DataCallInfo.dnsAddrPrimary.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv4PrimaryDnsAddr, dataCallEventInfo.ipv4DataCallInfo.dnsAddrPrimary.c_str(),
                TAF_DCS_IPV4_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv4PrimaryDnsAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv4DataCallInfo.dnsAddrPrimary.c_str(), event.ipv4PrimaryDnsAddr
        );
    }
    else
    {
        memset(event.ipv4PrimaryDnsAddr, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    }
        if (!dataCallEventInfo.ipv4DataCallInfo.dnsAddrSecondary.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv4SecondaryDnsAddr,
                                        dataCallEventInfo.ipv4DataCallInfo.dnsAddrSecondary.c_str(),
                TAF_DCS_IPV4_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv4SecondaryDnsAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv4DataCallInfo.dnsAddrSecondary.c_str(), event.ipv4SecondaryDnsAddr
        );
    }
    else
    {
        memset(event.ipv4SecondaryDnsAddr, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    }

    // IPv6 information
        event.ipv6ConnState = TafDcsUtils::ConvertDataCallStatus(
                                                    dataCallEventInfo.ipv6DataCallInfo.callStatus);
        TafDcsUtils::ConvertCallEndReason(
            dataCallEventInfo.ipv6DataCallInfo.callEndReason, event.ipv6CallEndReasonType, dummy);
        event.ipv6AddrMask = dataCallEventInfo.ipv6DataCallInfo.ipAddrMask;
        if (!dataCallEventInfo.ipv6DataCallInfo.ipAddr.empty())
        {
            LE_WARN_IF(
                le_utf8_Copy(
                    event.ipv6Addr, dataCallEventInfo.ipv6DataCallInfo.ipAddr.c_str(),
                    TAF_DCS_IPV6_ADDR_MAX_LEN, NULL) == LE_OVERFLOW,
                "ipv6Addr '%s' has been truncated to '%s'.",
                dataCallEventInfo.ipv6DataCallInfo.ipAddr.c_str(), event.ipv6Addr);
        }
    else
    {
        memset(event.ipv6Addr, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    }
    event.ipv6gwAddrMask = dataCallEventInfo.ipv6DataCallInfo.gwAddrMask;
        if (!dataCallEventInfo.ipv6DataCallInfo.gwAddr.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv6gwAddr, dataCallEventInfo.ipv6DataCallInfo.gwAddr.c_str(),
                TAF_DCS_IPV6_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv6gwAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv6DataCallInfo.gwAddr.c_str(), event.ipv6gwAddr
        );
    }
    else
    {
        memset(event.ipv6gwAddr, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    }
    if (!dataCallEventInfo.ipv6DataCallInfo.dnsAddrPrimary.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv6PrimaryDnsAddr, dataCallEventInfo.ipv6DataCallInfo.dnsAddrPrimary.c_str(),
                TAF_DCS_IPV6_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv6PrimaryDnsAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv6DataCallInfo.dnsAddrPrimary.c_str(), event.ipv6PrimaryDnsAddr
        );
    }
    else
    {
        memset(event.ipv6PrimaryDnsAddr, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    }
        if (!dataCallEventInfo.ipv6DataCallInfo.dnsAddrSecondary.empty())
    {
        LE_WARN_IF
        (
            le_utf8_Copy
            (
                event.ipv6SecondaryDnsAddr,
                                        dataCallEventInfo.ipv6DataCallInfo.dnsAddrSecondary.c_str(),
                TAF_DCS_IPV6_ADDR_MAX_LEN, NULL
            ) == LE_OVERFLOW,
            "ipv6SecondaryDnsAddr '%s' has been truncated to '%s'.",
            dataCallEventInfo.ipv6DataCallInfo.dnsAddrSecondary.c_str(), event.ipv6SecondaryDnsAddr
        );
    }
    else
    {
        memset(event.ipv6SecondaryDnsAddr, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    }

    // Send event to service and handle this state change
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_Report
    (
        tafDcsSvc.GetPaSessionStateChangeEvtId(), // paSessionStateChangeEvtId_
        static_cast<void *>(&event),
        sizeof(TafDcsSessionChangeEvent_t)
    );

    return;
}

void TafDcsProfileManager::tafPaRoamingEventsCb
(
    const taf::pa::data::RoamingStatus_t &roamingEventInfo,
    std::shared_ptr<void> context
)
{
    LE_UNUSED(context);
    LE_DEBUG("Phone ID: %d", TO_INT(roamingEventInfo.phoneId));
    LE_DEBUG("Roaming : %s", roamingEventInfo.isRoaming ? "true" : "false");
    LE_DEBUG("Type    : %d", TO_INT(roamingEventInfo.type));
    TafDcsRoamingStatus_t RoamingEvt;
    RoamingEvt.phoneId   = TO_INT(roamingEventInfo.phoneId);
    RoamingEvt.isRoaming = roamingEventInfo.isRoaming;
    RoamingEvt.type = TafDcsUtils::ConvertRoamingType(roamingEventInfo.type);

    // Send event to service and handle this roaming event
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_Report
    (
        tafDcsSvc.GetPaRoamingStatusChangeEvtId(), // paRoamingChangeEvtId_
        static_cast<void *>(&RoamingEvt),
        sizeof(TafDcsRoamingStatus_t)
    );
    return;
}

void TafDcsProfileManager::tafPaThrottledApnEventsCb
(
    const std::vector<taf::pa::data::ThrottledApnEventInfo_t> &throttledApnEventsList,
    std::shared_ptr<void> context
)
{
    LE_UNUSED(context);
    LE_DEBUG("Num throttled APNs: %zu", throttledApnEventsList.size());
    if (0 == throttledApnEventsList.size())
    {
        LE_INFO ("No throttled events.");
        return;
    }
    for (auto &throttledApnEvent : throttledApnEventsList)
    {
        LE_DEBUG("Num profile IDs: %zu", throttledApnEvent.profileIds.size());
        for (taf::pa::data::ProfileId_e profileId : throttledApnEvent.profileIds)
        {
            TafDcsThrottledApnEventInfo_t event;
            event.phoneId   = static_cast<uint8_t>(throttledApnEvent.phoneId);
            event.profileId = static_cast<uint32_t>(profileId);
            event.ipv4Time = throttledApnEvent.ipv4Time;
            event.ipv6Time = throttledApnEvent.ipv6Time;
            event.isBlockedOnAllPLMNs = throttledApnEvent.isBlockedOnAllPLMNs;
            memset(event.apn, 0,TAF_DCS_APN_NAME_MAX_BYTES);
            memset(event.mcc, 0, TAF_DCS_MCC_BYTES);
            memset(event.mnc, 0, TAF_DCS_MNC_BYTES);
            if (throttledApnEvent.apn.size())
            {
                le_utf8_Copy(event.apn, throttledApnEvent.apn.c_str(), TAF_DCS_APN_NAME_MAX_BYTES,
                                                                                           nullptr);
            }
            if (throttledApnEvent.mcc.size())
            {
                le_utf8_Copy(event.mcc, throttledApnEvent.mcc.c_str(), TAF_DCS_MCC_BYTES, nullptr);
            }
            if (throttledApnEvent.mnc.size())
            {
                le_utf8_Copy(event.mnc, throttledApnEvent.mnc.c_str(), TAF_DCS_MNC_BYTES, nullptr);
            }
            // Send the event to the DCS event thread.
            auto &tafDcsSvc = TafDcsSvc::GetInstance();
            le_event_Report
            (
                tafDcsSvc.GetPaThrottledAPNsEvtId(), // paThrottledAPNsEvtId_
                static_cast<void *>(&event),
                sizeof(TafDcsThrottledApnEventInfo_t)
            );
        }
    }
    return;
}

void TafDcsProfileManager::tafPaQosTftEventsCb
(
    const taf::pa::data::QosTftEventInfo_t &qosTftEventInfo,
    std::shared_ptr<void> context
)
{
    LE_UNUSED(context);
    LE_DEBUG("Phone ID  : %d",  TO_INT(qosTftEventInfo.phoneId));
    LE_DEBUG("Profile ID: %d",  TO_INT(qosTftEventInfo.profileId));
    LE_DEBUG("Num flows : %zu", qosTftEventInfo.qosFlows.size());

    TafDcsQosTftEventInfo_t qosEvent;
    qosEvent.phoneId = static_cast<uint8_t>(qosTftEventInfo.phoneId);
    qosEvent.profileId = static_cast<uint8_t>(qosTftEventInfo.profileId);
    for (auto qosFlow : qosTftEventInfo.qosFlows)
    {
        LE_DEBUG("Flow ID   : %d",  qosFlow.qosFlowId);
        LE_DEBUG("Flow state: %d",  TO_INT(qosFlow.state));
        LE_DEBUG("Flow mask : %lu", qosFlow.paramMask.to_ulong());
        qosEvent.qosFlowId = qosFlow.qosFlowId;
        qosEvent.state     = TafDcsUtils::ConvertQoSFlowState(qosFlow.state);
        qosEvent.paramMask = TafDcsUtils::ConvertQoSFlowBitMask(qosFlow.paramMask);
        // Send the event to the DCS event thread.
        auto &tafDcsSvc = TafDcsSvc::GetInstance();
        le_event_Report
        (
            tafDcsSvc.GetPaQosTftEvtId(), // paQosTftEvtId_
            static_cast<void *>(&qosEvent),
            sizeof(TafDcsQosTftEventInfo_t)
        );
    }
}

void TafDcsProfileManager::tafPaHwAccelerationEventsCb
(
    const taf::pa::data::HwAccelerationChangeEvent_t &hwAccelerationEventInfo,
    std::shared_ptr<void>              context
)
{
    LE_UNUSED(context);
    LE_DEBUG("Phone ID: %d", TO_INT(hwAccelerationEventInfo.phoneId));
    LE_DEBUG("State   : %d", TO_INT(hwAccelerationEventInfo.state));

    TafDcsHwAccelerationChangeEvent_t event;
    event.phoneId = static_cast<uint8_t>(hwAccelerationEventInfo.phoneId);
    event.state = TafDcsUtils::ConvertHwAccelerationState(hwAccelerationEventInfo.state);

    // Send the event to the DCS event thread.
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_Report
    (
        tafDcsSvc.GetPaHwAccelerationEvtId(), // paHwAccelerationChangeEvtId_
        static_cast<void *>(&event),
        sizeof(TafDcsHwAccelerationChangeEvent_t)
    );
    return;
}

/**************************************************************************************************/
// Internal Event handlers
/**************************************************************************************************/

// Register the event handler for updateProfileEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerUpdateProfileEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for updateProfileEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("updateProfileEvtId_ Handler", tafDcsSvc.GetUpdateProfileEvtId(),
                                                                        updateProfileEvtHandler);
}

// The event handler for updateProfileEvtId_.
// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::updateProfileEvtHandler(void *reqPtr)
{
    LE_DEBUG("The update profile event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL")
    TafDcsUpdateProfileEvent_t *eventPtr = static_cast<TafDcsUpdateProfileEvent_t *>(reqPtr);
    LE_DEBUG("Slot    Id: %d", eventPtr->slotId);
    LE_DEBUG("Profile Id: %d", eventPtr->profileInfo.id);
    LE_DEBUG("APN       : %s", eventPtr->profileInfo.apn);
    LE_DEBUG("Name      : %s", eventPtr->profileInfo.name);
    LE_DEBUG("Username  : %s", eventPtr->profileInfo.userName);
    LE_DEBUG("Password  : %s", eventPtr->profileInfo.password);
    LE_DEBUG("Tech pref : %d", eventPtr->profileInfo.techPref);
    LE_DEBUG("Auth type : %d", eventPtr->profileInfo.authTypeBitmask);
    LE_DEBUG("IP type   : %d", eventPtr->profileInfo.ipType);
    LE_DEBUG("APN Type  : %d", eventPtr->profileInfo.apnTypeMask);
    LE_DEBUG("Emergency call support  : %d", eventPtr->profileInfo.emergencyCallSupport);


    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();

    // Get a write lock
    std::unique_lock<std::shared_mutex> lock(tafDcsProfileManager.profileReadWriteMutex_);
    le_result_t result = tafDcsProfileManager.updateProfile(eventPtr);
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "updateProfile failed: %d", result);
}

// Register the event handler for clientDisconnectedEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerClientDisconnectedEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for clientDisconnectedEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("clientDisconnectedEvtId_ Hdlr", tafDcsSvc.GetClientsDisconnectedEvtId(),
                                                                    clientDisconnectedEvtHandler);
}

// The event handler for clientDisconnectedEvtId_.
// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::clientDisconnectedEvtHandler(void *reqPtr)
{
    LE_DEBUG("The client disconnected event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    // Get the client reference from the request pointer.
    le_msg_SessionRef_t clientRef=static_cast<TafDcsClientDisconnectedEvent_t *>(reqPtr)->clientRef;

    // Check all profiles for this client and remove from dataReqClients_ set, stop data (if needed)
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    std::vector<taf::pa::data::PhoneId_e> phoneIDs;
    tafDcsProfileManager.GetPhones(phoneIDs);
    LE_DEBUG("Phones count: %zu", phoneIDs.size());
    // Iterate thorough all profiles for each phone ID.
    for (taf::pa::data::PhoneId_e phoneId : phoneIDs)
    {
        uint8_t idx = static_cast<uint8_t>(phoneId);
        auto profiles = tafDcsProfileManager.FindProfilesByPhoneId(idx);
        for (auto &profile : profiles)
        {
            uint32_t profileId = 0;
            profile->GetId(profileId);
            LE_DEBUG("Phone Id: %d, Profile Id: %d", idx, profileId);
            // Remove this client from async response list so that it does not get called.
            profile->RemoveStartSessionAsyncClient(clientRef);
            profile->RemoveStopSessionAsyncClient(clientRef);

            // Stop any data called by this client.
            if (profile->HasClientCalledSessionStart(clientRef))
            {
                size_t listSize = 0;
                // The return value does not matter in this scenario.
                profile->RemoveClient(clientRef, listSize);
                if (listSize == 0)
                {
                    // Send event to stop data as there are no more clients for this profile.
                    auto &tafDcsSvc = TafDcsSvc::GetInstance();
                    TafDcsSessionStopEvent_t event;
                    event.profile.phoneId = idx;
                    event.profile.profileId = profileId;
                    profile->GetPdp(event.ipType_pdp);
                    le_event_Report(
                        tafDcsSvc.GetSessionStopEvtId(), // sessionStopEvtId_
                        static_cast<void *>(&event),
                        sizeof(TafDcsSessionStopEvent_t));
                }
            }
        }
    }
}

// The event handler for sessionStartEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerSessionStartEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for sessionStartEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("sessionStartEvtId_ Handler", tafDcsSvc.GetSessionStartEvtId(),
                                                                        sessionStartEvtHandler);
}

// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::sessionStartEvtHandler(void *reqPtr)
{
    LE_DEBUG("The session start event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");
    // TODO
}

// The event handler for sessionStopEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerSessionStopEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for sessionStopEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("sessionStopEvtId_ Handler", tafDcsSvc.GetSessionStopEvtId(),
                                                                        sessionStopEvtHandler);
}

// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::sessionStopEvtHandler(void *reqPtr)
{
    LE_DEBUG("The session stop event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");
    TafDcsSessionStopEvent_t *eventPtr = static_cast<TafDcsSessionStopEvent_t *>(reqPtr);

    taf::pa::data::DataCallStartStopParams_t params =
    {
        static_cast<taf::pa::data::PhoneId_e>(eventPtr->profile.phoneId),
        static_cast<taf::pa::data::ProfileId_e>(eventPtr->profile.profileId),
        TafDcsUtils::ConvertPDP(eventPtr->ipType_pdp),
        ""
    };
    le_result_t result = taf::pa::data::StopDataSessionAsync(params);
    LE_INFO ("StopDataSessionAsync result: %d", result);
}

// Register the event handler for startSessionAsyncRspEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerStartSessionAsyncRspEventHandler
(
    void *param1Ptr,
    void *param2Ptr
)
{
    LE_INFO("Register handler for startSessionAsyncRspEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paSessionStateChangeEvtId_ Hdlr", tafDcsSvc.GetStartSessionAsyncRspEvtId(),
                                                                startSessionAsyncRspEventHandler);
}

// The event handler for startSessionAsyncRspEvtId_.
// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::startSessionAsyncRspEventHandler(void *reqPtr)
{
    LE_DEBUG("The start session async command event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    TafDcsSendStartSessionAsyncRsp_t *eventPtr = static_cast<TafDcsSendStartSessionAsyncRsp_t *>
                                                                                        (reqPtr);
    LE_DEBUG("Client  ref: %p", eventPtr->clientRef);
    LE_DEBUG("Profile ref: %p", eventPtr->profileRef);
    LE_DEBUG("Result     : %d", TO_INT(eventPtr->result));

    // Get the profile.
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    auto profileOptWrapper = tafDcsProfileManager.getProfile(eventPtr->profileRef);
    if (!profileOptWrapper.has_value())
    {
        LE_ERROR("profileRef %p not found", eventPtr->profileRef);
        return;
    }
    TafDcsProfile &profile = profileOptWrapper.value().get();

    // Get the list of clients that have called taf_dcs_StartSessionAsync.
    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
        clientList = profile.GetStartSessionAsyncClients();

    // Send the event to all the clients.
    for (auto &client : clientList)
    {
        le_msg_SessionRef_t clientRef = std::get<0>(client);
        taf_dcs_AsyncSessionHandlerFunc_t handlerFunc = std::get<1>(client);
        void *context = std::get<2>(client);

        LE_DEBUG("Sending event to client %p", clientRef);
        handlerFunc(eventPtr->profileRef, eventPtr->result, context);

        // Remove the client from the list of clients that have requested async session start.
        profile.RemoveStartSessionAsyncClient(clientRef);

        if (LE_OK == eventPtr->result)
        {
            // Add this client to the list of clients that have requested data.
            size_t listSize = 0;
            profile.AddClient(clientRef, listSize);
            LE_DEBUG("Client %p added. Num clients: %zu", clientRef, listSize);
        }
    }
}

// Register the event handler for stopSessionAsyncRspEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerStopSessionAsyncRspEventHandler
(
    void *param1Ptr,
    void *param2Ptr
)
{
    LE_INFO("Register handler for stopSessionAsyncRspEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("stopSessionAsyncRspEvtId_ Hdlr", tafDcsSvc.GetStopSessionAsyncRspEvtId(),
                                                                stopSessionAsyncRspEventHandler);
}

// The event handler for stopSessionAsyncRspEvtId_.
// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::stopSessionAsyncRspEventHandler(void *reqPtr)
{
    LE_DEBUG("The stop session async command event handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    TafDcsSendStopSessionAsyncRsp_t *eventPtr = static_cast<TafDcsSendStopSessionAsyncRsp_t *>(
                                                                                            reqPtr);
    LE_DEBUG("Client  ref: %p", eventPtr->clientRef);
    LE_DEBUG("Profile ref: %p", eventPtr->profileRef);
    LE_DEBUG("Result     : %d", TO_INT(eventPtr->result));

    // Get the profile.
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    auto profileOptWrapper = tafDcsProfileManager.getProfile(eventPtr->profileRef);
    if (!profileOptWrapper.has_value())
    {
        LE_ERROR("profileRef %p not found", eventPtr->profileRef);
        return;
    }
    TafDcsProfile &profile = profileOptWrapper.value().get();

    // Get the list of clients that have called taf_dcs_StopSessionAsync.
    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
                                            clientList = profile.GetStopSessionAsyncClients();

        // Send the event to all the clients.
    for (auto &client : clientList)
    {
        le_msg_SessionRef_t clientRef = std::get<0>(client);
        taf_dcs_AsyncSessionHandlerFunc_t handlerFunc = std::get<1>(client);
        void *context = std::get<2>(client);

        LE_DEBUG("Sending event to client %p", clientRef);
        handlerFunc(eventPtr->profileRef, eventPtr->result, context);

        // Remove the client from the list of clients that have requested async session stop.
        profile.RemoveStopSessionAsyncClient(clientRef);

        // Remove this client from the list of clients that have requested data.
        size_t listSize = 0;
        // The return value does not matter in this scenario.
        profile.RemoveClient(clientRef, listSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * PA event handlers
 */
////////////////////////////////////////////////////////////////////////////////////////////////////

// Register the event handler for paSessionStateChangeEvtId_.
// This is done in the context of tafDcsEventsThreadRef_ (tafDcsEventsThread).
void TafDcsProfileManager::registerPaSessionStateChangeEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for paSessionStateChangeEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paSessionStateChangeEvtId_ Hdlr", tafDcsSvc.GetPaSessionStateChangeEvtId(),
                        paSessionStateChangeEvtHandler);
}

// The event handler for paSessionStateChangeEvtId_.
// This should be triggered in the context of tafDcsEventsThreadRef_(tafDcsEventsThread) and should
// not be called directly.
void TafDcsProfileManager::paSessionStateChangeEvtHandler(void *reqPtr)
{
    LE_DEBUG("The paSessionStateChangeEvtId_ handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    le_result_t result;
    TafDcsSessionChangeEvent_t *eventPtr = static_cast<TafDcsSessionChangeEvent_t *>(reqPtr);
    // The event to send to the client
    TafDcsSessionChangeEvent_t clientEvent;
    taf_dcs_ConState_t curState, curIpv4State, curIpv6State;
    taf_dcs_Pdp_t profileIpType;

    LE_DEBUG("Phone   Id: %d", TO_INT(eventPtr->profile.phoneId));
    LE_DEBUG("Profile Id: %d", TO_INT(eventPtr->profile.profileId));
    LE_DEBUG("IP State  : %d", TO_INT(eventPtr->connState));
    LE_DEBUG("IPv4 State: %d", TO_INT(eventPtr->ipv4ConnState));
    LE_DEBUG("IPv6 State: %d", TO_INT(eventPtr->ipv6ConnState));
    LE_DEBUG("IP Type   : %d", TO_INT(eventPtr->ipType_pdp));

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    // Get the profile object based on phone ID and profile ID
    auto profileOptWrapper = tafDcsProfileManager.getProfile(eventPtr->profile.phoneId,
                                                                    eventPtr->profile.profileId);
    if (!profileOptWrapper.has_value())
    {
        LE_WARN("profile[%d,%d] not found", eventPtr->profile.phoneId,
                eventPtr->profile.profileId);
        return;
    }
    TafDcsProfile &profile = profileOptWrapper.value().get();

    // Get the current call status from the profile object before it is updated.
    {
        // Get a read lock here as the profile is updated
        std::shared_lock<std::shared_mutex> lock(tafDcsProfileManager.profileReadWriteMutex_);

        clientEvent.profile.phoneId   = eventPtr->profile.phoneId;
        clientEvent.profile.profileId = eventPtr->profile.profileId;

        // Get the current call states
        result = profile.GetSessionState(curState, curIpv4State, curIpv6State);
        TAF_ERROR_IF_RET_NIL(LE_OK != result, "GetSessionState failed: %d", TO_INT(result));
        // Get the profile IP type
        result = profile.GetPdp(profileIpType);
        TAF_ERROR_IF_RET_NIL(LE_OK != result, "GetPdp failed: %d", TO_INT(result));
        LE_DEBUG("Current IP State  : %d", TO_INT(eventPtr->connState));
        LE_DEBUG("Current IPv4 State: %d", TO_INT(eventPtr->ipv4ConnState));
        LE_DEBUG("Current IPv6 State: %d", TO_INT(eventPtr->ipv6ConnState));
    }

    // Update internal session state
    {
        // Get a write lock here as the profile is updated
        std::unique_lock<std::shared_mutex> lock(tafDcsProfileManager.profileReadWriteMutex_);
        result = tafDcsProfileManager.updateSessionDetails(eventPtr);
        TAF_ERROR_IF_RET_NIL(LE_NOT_FOUND == result, "Profile not found. Create it first!");
        TAF_ERROR_IF_RET_NIL(LE_OK != result, "updateProfile failed: %d", result);
    }

    // If its a TAF_DCS_CONENCTED or TAF_DCS_DISCONNECTED event, complete the promise, if waiting.
    if (TAF_DCS_CONNECTED == eventPtr->connState || TAF_DCS_DISCONNECTED == eventPtr->connState)
    {
        if (tafDcsProfileManager.isSyncCmdPromiseWaiting_.load())
        {
            LE_INFO("syncCmdPromise_.set_value as its waiting.");
            tafDcsProfileManager.isSyncCmdPromiseWaiting_.store(false);
            tafDcsProfileManager.syncCmdPromise_.set_value(result);
        }
        else
        {
            LE_DEBUG("syncCmdPromise_ is not waiting.");

            // Get a read lock
            std::shared_lock<std::shared_mutex> lock(tafDcsProfileManager.profileReadWriteMutex_);

            // This could be an async command. Send message to notify clients.
            auto &tafDcsSvc = TafDcsSvc::GetInstance();
            TafDcsSendStartSessionAsyncRsp_t startAsyncRsp;
            startAsyncRsp.clientRef = nullptr;
            startAsyncRsp.profileRef = profile.GetReference();
            startAsyncRsp.result = LE_FAULT;
            TafDcsSendStopSessionAsyncRsp_t stopAsyncRsp;
            stopAsyncRsp.clientRef = nullptr;
            stopAsyncRsp.profileRef = profile.GetReference();
            stopAsyncRsp.result = LE_FAULT;
            if (TAF_DCS_CONNECTED == eventPtr->connState)
            {
                startAsyncRsp.result = LE_OK;
            }
            if (TAF_DCS_DISCONNECTED == eventPtr->connState)
            {
                stopAsyncRsp.result = LE_OK;
            }
            le_event_Report(
                tafDcsSvc.GetStartSessionAsyncRspEvtId(),
                &startAsyncRsp,
                sizeof(TafDcsSendStartSessionAsyncRsp_t));
            le_event_Report(
                tafDcsSvc.GetStopSessionAsyncRspEvtId(),
                &stopAsyncRsp,
                sizeof(TafDcsSendStopSessionAsyncRsp_t));
        }
    }

    // Fill and send the client event according to the IP type(PD) and state.
    if (TAF_DCS_PDP_IPV4V6 == profileIpType)
    {
        if (curIpv4State != eventPtr->ipv4ConnState)
        {
            clientEvent.connState  = eventPtr->ipv4ConnState;
            clientEvent.ipType_pdp = TAF_DCS_PDP_IPV4;
            // Send event to registered clients
            LE_DEBUG("Sending IPv4v6 IPv4 state(old): %d(%d)", TO_INT(clientEvent.connState),
                                                                            TO_INT(curIpv4State));
            // sendSessionSateEvent will only return LE_OK. So no need to check return.
            tafDcsProfileManager.sendSessionSateEvent(clientEvent);
        }
        if (curIpv6State != eventPtr->ipv6ConnState)
        {
            clientEvent.connState  = eventPtr->ipv6ConnState;
            clientEvent.ipType_pdp = TAF_DCS_PDP_IPV6;
            // Send event to registered clients
            LE_DEBUG("Sending IPv4v6 IPv6 state(old): %d(%d)", TO_INT(clientEvent.connState),
                                                                            TO_INT(curIpv6State));
            // sendSessionSateEvent will only return LE_OK. So no need to check return.
            tafDcsProfileManager.sendSessionSateEvent(clientEvent);
        }
    }
    else if (TAF_DCS_PDP_IPV4 == profileIpType)
    {
        if (curIpv4State != eventPtr->ipv4ConnState)
        {
            clientEvent.connState  = eventPtr->ipv4ConnState;
            clientEvent.ipType_pdp = TAF_DCS_PDP_IPV4;
            // Send event to registered clients
            LE_DEBUG("Sending IPv4 state(old): %d(%d)", TO_INT(clientEvent.connState),
                                                                            TO_INT(curIpv4State));
            // sendSessionSateEvent will only return LE_OK. So no need to check return.
            tafDcsProfileManager.sendSessionSateEvent(clientEvent);
        }
    }
    else if (TAF_DCS_PDP_IPV6 == profileIpType)
    {
        if (curIpv6State != eventPtr->ipv6ConnState)
        {
            clientEvent.connState  = eventPtr->ipv6ConnState;
            clientEvent.ipType_pdp = TAF_DCS_PDP_IPV6;
            // Send event to registered clients
            LE_DEBUG("Sending IPv4v6 IPv6 state(old): %d(%d)", TO_INT(clientEvent.connState),
                                                                            TO_INT(curIpv6State));
            // sendSessionSateEvent will only return LE_OK. So no need to check return.
            tafDcsProfileManager.sendSessionSateEvent(clientEvent);
        }
    }
    else
    {
        LE_WARN ("Unknown PDP: %d", TO_INT(profileIpType));
    }
}

void TafDcsProfileManager::registerPaRoamingEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for paRoamingChangeEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paRoamingChangeEvtId_ Hdlr", tafDcsSvc.GetPaRoamingStatusChangeEvtId(),
                                                                            paRoamingEvtHandler);
}

void TafDcsProfileManager::paRoamingEvtHandler(void *reqPtr)
{
    LE_DEBUG("The paRoamingChangeEvtId_ handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");
    TafDcsRoamingStatus_t *roamingEvt = static_cast<TafDcsRoamingStatus_t *> (reqPtr);
    LE_DEBUG ("Phone ID: %d", roamingEvt->phoneId);
    LE_DEBUG ("Roaming : %s", roamingEvt->isRoaming ? "true" : "false");
    LE_DEBUG ("Type    : %d", roamingEvt->type);

    // Nothing to do here

    // Send event to registered clients
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    le_result_t result = tafDcsProfileManager.sendRoamingEvent(roamingEvt);
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "sendRoamingEvent failed: %d", result);
}

void TafDcsProfileManager::registerPaThrottledAPNsEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for paThrottledAPNsEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paThrottledAPNsEvtId_ Hdlr", tafDcsSvc.GetPaThrottledAPNsEvtId(),
                                                                        paThrottledAPNsEvtHandler);
}

void TafDcsProfileManager::paThrottledAPNsEvtHandler(void *reqPtr)
{
    LE_DEBUG("The paThrottledAPNsEvtId_ handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    TafDcsThrottledApnEventInfo_t *throttledApnEvt =
                                            static_cast<TafDcsThrottledApnEventInfo_t *>(reqPtr);
    LE_DEBUG("Phone   Id : %d", throttledApnEvt->phoneId);
    LE_DEBUG("Profile Id : %d", throttledApnEvt->profileId);
    LE_DEBUG("ipv4Time   : %d", throttledApnEvt->ipv4Time);
    LE_DEBUG("ipv6Time   : %d", throttledApnEvt->ipv6Time);
    LE_DEBUG("Blocked on all PLMNs: %s", throttledApnEvt->isBlockedOnAllPLMNs ? "true" : "false");
    LE_DEBUG("apn        : %s", throttledApnEvt->apn);
    LE_DEBUG("mcc        : %s", throttledApnEvt->mcc);
    LE_DEBUG("mnc        : %s", throttledApnEvt->mnc);

    // TODO: Cache in the service under respective profile object for later retrieval.
    /**
     * From TelSDK:
     * This function is called when the throttled state changes, such as when a new APN is
     * throttled or an existing throttled APN is no longer throttled after the timeout.
     * APNs that are not throttled anymore will not appear in the list of throttled APNs.
     * Need to send APN unthrottled event appropriately.
    */

    // Send event to registered clients
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    le_result_t result = tafDcsProfileManager.sendThrottledApnEvent(throttledApnEvt);
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "sendThrottledApnEvent failed: %d", result);
}

void TafDcsProfileManager::registerPaHwAccelerationEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for paHwAccelerationChangeEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paHwAccelerationChangeEvtId_ Hdlr", tafDcsSvc.GetPaHwAccelerationEvtId(),
                                                                    paHwAccelerationEvtHandler);
}

void TafDcsProfileManager::paHwAccelerationEvtHandler(void *reqPtr)
{
    LE_DEBUG("The paHwAccelerationChangeEvtId_ handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    TafDcsHwAccelerationChangeEvent_t *hwAccEvt =
                                        static_cast<TafDcsHwAccelerationChangeEvent_t *>(reqPtr);
    // Nothing to do in the service.
    // Send event to registered clients
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    le_result_t result = tafDcsProfileManager.sendHwAccelerationEvent(hwAccEvt);
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "sendHwAccelerationEvent failed: %d", result);
}

void TafDcsProfileManager::registerPaQosTftEvtHandler(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Register handler for paQosTftEvtId_");
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_AddHandler("paQosTftEvtId_ Hdlr", tafDcsSvc.GetPaQosTftEvtId(), paQosTftEvtHandler);
}

void TafDcsProfileManager::paQosTftEvtHandler(void *reqPtr)
{
    LE_DEBUG("The paQosTftEvtId_ handler");
    TAF_ERROR_IF_RET_NIL(nullptr == reqPtr, "reqPtr is NULL");

    TafDcsQosTftEventInfo_t *qosTftEvt = static_cast<TafDcsQosTftEventInfo_t *>(reqPtr);

    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();

    // Get the profile object
    /*
    auto profileOptWrapper = tafDcsProfileManager.getProfile(qosTftEvt->phoneId,
                                                                        qosTftEvt->profileId);
    TAF_ERROR_IF_RET_NIL((!profileOptWrapper.has_value()),
                            "profile[%d,%d] not found", qosTftEvt->phoneId, qosTftEvt->profileId);
    TafDcsProfile &profile = profileOptWrapper.value().get();
    */
    // TODO: Update the profile with QoS flow details
    if (TAF_DCS_QOS_ACTIVATED == qosTftEvt->state)
    {
        // Create new QoS flow details
    }
    else if (TAF_DCS_QOS_MODIFIED== qosTftEvt->state)
    {
        // Modify an existing flow
    }
    else if (TAF_DCS_QOS_DELETED== qosTftEvt->state)
    {
        // Remove the flow
    }

    // Send event to clients
    le_result_t result = tafDcsProfileManager.sendQosTftEvent(qosTftEvt);
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "sendQosTftEvent failed: %d", result);
}

void TafDcsProfileManager::signalEventThreadInitComplete(void *param1Ptr, void *param2Ptr)
{
    LE_INFO("Signal the main thread to proceed.");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    tafDcsProfileManager.eventThreadInitPromise_.set_value();
}
