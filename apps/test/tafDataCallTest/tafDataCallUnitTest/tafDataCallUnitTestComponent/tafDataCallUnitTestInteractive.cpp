/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   tafDataCallUnitTestInteractive.cpp
 * @brief  This file allows TelAF data call service APIs to be executed interactively and tested.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDcsHelper.hpp"
#include <string>
#include <future>
#include <iostream>
#include <map>
#include <chrono>
#include <iomanip> // for std::fixed and std::setprecision

using namespace taf::svc::datacall;

static bool bPrintNotifLogsOnConsole = true;

static taf_dcs_RoamingStatusHandlerRef_t                                g_roamingStatusHandlerRef;
static std::map<uint32_t, taf_dcs_SessionStateHandlerRef_t>  g_Profile_SessionStateHandlerRef_Map;
static std::map<uint32_t, taf_dcs_QosStatusHandlerRef_t>    g_Profile_QosStatusHandlerRef_Map;
static std::map<uint32_t, taf_dcs_HwAccelerationStateHandlerRef_t> g_Profile_HwAccelHandlerRef_Map;
static std::map<uint32_t, taf_dcs_ThrottledStatusHandlerRef_t>
                                                            g_Profile_ThrottledStatusHandlerRef_Map;

// Callback thread reference
le_thread_Ref_t callbackThreadRef = nullptr;

// Async commands thread reference
le_thread_Ref_t asyncCmdThreadRef = nullptr;

/**
 * Profile management and related API are from 1 to 49.
 * Session management and related API start from 50, with Start session being the first API.
 * Add new APIs after this so that the order is not changed to avoid breaking automation scripts.
 * */
typedef enum
{
    PROFILE_GET_LIST = 1,            // 1
    PROFILE_CREATE,                  // 2
    PROFILE_DELETE,                  // 3
    PROFILE_SET_APN,                 // 4
    PROFILE_SET_NAME,                // 5
    PROFILE_SET_TECH_PREF,           // 6
    PROFILE_SET_APN_TYPE_MASK,       // 7
    PROFILE_SET_PDP,                 // 8
    PROFILE_SET_AUTHENTICATION,      // 9
    PROFILE_SET_DEFAULT,             // 10
    PROFILE_GET_ID,                  // 11
    PROFILE_GET_APN,                 // 12
    PROFILE_GET_NAME,                // 13
    PROFILE_GET_TECH_PREF,           // 14
    PROFILE_GET_APN_TYPE_MASK,       // 15
    PROFILE_GET_PDP,                 // 16
    PROFILE_GET_AUTHENTICATION,      // 17
    PROFILE_GET_DEFAULT,             // 18
    PROFILE_GET_MTU,                 // 19
    PROFILE_IS_IPV4,                 // 20
    PROFILE_IS_IPV6,                 // 21
    PROFILE_GET_PHONE_ID,            // 22
    GET_DEFAULT_PHONE_AND_PROFILE,   // 23
    SESSION_START = 50,              // 50
    SESSION_START_ASYNC,             // 51
    SESSION_STOP,                    // 52
    SESSION_STOP_ASYNC,              // 53
    SESSION_GET_STATE,               // 54
    SESSION_GET_DATA_BEARER_TECH,    // 55
    SESSION_GET_ROAMING_STATUS,      // 56
    SESSION_GET_MAX_DATA_BIT_RATES,  // 57
    SESSION_GET_CALL_END_REASON,     // 58
    SESSION_GET_APN_THROTTLE_STATUS, // 59
    SESSION_GET_APN_THROTTLE_PLMN,   // 60
    SESSION_GET_IPV4_ADDRESS,        // 61
    SESSION_GET_IPV6_ADDRESS,        // 62
    SESSION_GET_IPV4_DNS,            // 63
    SESSION_GET_IPV6_DNS,            // 64
    SESSION_GET_IPV4_GATEWAY,        // 65
    SESSION_GET_IPV6_GATEWAY,        // 66
    SESSION_GET_IPV4_SUBNET_MASK,    // 67
    SESSION_GET_IPV6_SUBNET_MASK,    // 68
    SESSION_GET_INTERFACE_NAME,      // 69
    SESSION_GET_PH_ID_BY_INTF_NAME,  // 70
    SESSION_GET_PROF_ID_BY_INTF_NAME // 71
} dcsAPIs;

static void ShowMenu()
{
    std::cout << std::endl
              << "Select an option:"                           << std::endl
              << "0  -> Show menu  "                           << std::endl
              << "98 -> Toggle notification output in console" << std::endl
              << "99 -> Exit  "                   << std::endl
              << PROFILE_GET_LIST                 << "  -> Profile: Get list"
              << std::endl
              << PROFILE_CREATE                   << "  -> Profile: Create Profile"
              << std::endl
              << PROFILE_DELETE                   << "  -> Profile: Delete Profile"
              << std::endl
              << PROFILE_SET_APN                  << "  -> Profile: Set APN"
              << std::endl
              << PROFILE_SET_NAME                 << "  -> Profile: Set name"
              << std::endl
              << PROFILE_SET_TECH_PREF            << "  -> Profile: Set tech preference"
              << std::endl
              << PROFILE_SET_APN_TYPE_MASK        << "  -> Profile: Set APN type mask"
              << std::endl
              << PROFILE_SET_PDP                  << "  -> Profile: Set PDP(IP family type)"
              << std::endl
              << PROFILE_SET_AUTHENTICATION       << "  -> Profile: Set authentication"
              << std::endl
              << PROFILE_SET_DEFAULT              << " -> Profile: Set default"
              << std::endl
              << PROFILE_GET_ID                   << " -> Profile: Get Id"
              << std::endl
              << PROFILE_GET_APN                  << " -> Profile: Get APN"
              << std::endl
              << PROFILE_GET_NAME                 << " -> Profile: Get name"
              << std::endl
              << PROFILE_GET_TECH_PREF            << " -> Profile: Get tech preference"
              << std::endl
              << PROFILE_GET_APN_TYPE_MASK        << " -> Profile: Get APN type mask"
              << std::endl
              << PROFILE_GET_PDP                  << " -> Profile: Get PDP(IP family type)"
              << std::endl
              << PROFILE_GET_AUTHENTICATION       << " -> Profile: Get authentication"
              << std::endl
              << PROFILE_GET_DEFAULT              << " -> Profile: Get default"
              << std::endl
              << PROFILE_GET_MTU                  << " -> Profile: Get MTU"
              << std::endl
              << PROFILE_IS_IPV4                  << " -> Profile: Is IPv4?"
              << std::endl
              << PROFILE_IS_IPV6                  << " -> Profile: Is IPv6?"
              << std::endl
              << PROFILE_GET_PHONE_ID             << " -> Profile: Get phone ID"
              << std::endl
              << GET_DEFAULT_PHONE_AND_PROFILE    << " -> Get default phone ID and profile ID"
              << std::endl
              << SESSION_START                    << " -> Session: Start"
              << std::endl
              << SESSION_START_ASYNC              << " -> Session: Start asynchronously"
              << std::endl
              << SESSION_STOP                     << " -> Session: Stop"
              << std::endl
              << SESSION_STOP_ASYNC               << " -> Session: Stop asynchronously"
              << std::endl
              << SESSION_GET_STATE                << " -> Session: Get state"
              << std::endl
              << SESSION_GET_DATA_BEARER_TECH     << " -> Session: Get data bearer technology"
              << std::endl
              << SESSION_GET_ROAMING_STATUS       << " -> Session: Get roaming status"
              << std::endl
              << SESSION_GET_MAX_DATA_BIT_RATES   << " -> Session: Get max data bit rates"
              << std::endl
              << SESSION_GET_CALL_END_REASON      << " -> Session: Get call end reason"
              << std::endl
              << SESSION_GET_APN_THROTTLE_STATUS  << " -> Session: Get apn throttled status"
              << std::endl
              << SESSION_GET_APN_THROTTLE_PLMN    << " -> Session: Get apn throttled PLMN"
              << std::endl
              << SESSION_GET_IPV4_ADDRESS         << " -> Session: Get IPv4 addresses"
              << std::endl
              << SESSION_GET_IPV6_ADDRESS         << " -> Session: Get IPv6 addresses"
              << std::endl
              << SESSION_GET_IPV4_DNS             << " -> Session: Get IPv4 DNS addresses"
              << std::endl
              << SESSION_GET_IPV6_DNS             << " -> Session: Get IPv6 DNS addresses"
              << std::endl
              << SESSION_GET_IPV4_GATEWAY         << " -> Session: Get IPv4 gateway address"
              << std::endl
              << SESSION_GET_IPV6_GATEWAY         << " -> Session: Get IPv6 gateway address"
              << std::endl
              << SESSION_GET_IPV4_SUBNET_MASK     << " -> Session: Get IPv4 subnet mask"
              << std::endl
              << SESSION_GET_IPV6_SUBNET_MASK     << " -> Session: Get IPv6 subnet mask"
              << std::endl
              << SESSION_GET_INTERFACE_NAME       << " -> Session: Get interface name"
              << std::endl
              << SESSION_GET_PH_ID_BY_INTF_NAME   << " -> Session: Get phone Id by interface name"
              << std::endl
              << SESSION_GET_PROF_ID_BY_INTF_NAME << " -> Session: Get profile Id by interface name"
              << std::endl
              << std::endl;
}

static taf_dcs_ProfileRef_t GetProfileRef()
{
    taf_dcs_ProfileRef_t ProfileRef;
    int profileId = 1;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile id:  ";
    std::cin.clear();
    std::cin >> profileId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    LE_TEST_INFO("Phone ID: %d, Profile ID: %d", phoneID, profileId);

    ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                        static_cast<uint32_t>(profileId));
    LE_TEST_OK(nullptr != ProfileRef, "taf_dcs_GetProfileEx");

    return ProfileRef;
}

// Function to get profile ref with undefined profile ID to create a profile
static taf_dcs_ProfileRef_t GetProfileRef(uint8_t phoneID)
{
    taf_dcs_ProfileRef_t ProfileRef;
    ProfileRef = taf_dcs_GetProfileEx(phoneID, TAF_DCS_UNDEFINED_PROFILE_ID);
    LE_TEST_OK(nullptr != ProfileRef, "taf_dcs_GetProfileEx");
    return ProfileRef;
}

static le_result_t GetProfileId()
{
    le_result_t result;
    uint32_t profileId;
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetProfileId(ProfileRef, &profileId);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get profile id failed");

    LE_TEST_INFO("Profile Id: %d", profileId);
    std::cout << "Profile Id: " << profileId << std::endl;
    return result;
}

static le_result_t GetAPN()
{
    le_result_t result;
    char apnName[TAF_DCS_APN_NAME_MAX_LEN] = {0};
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetAPN(ProfileRef, apnName, TAF_DCS_APN_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get APN failed");

    LE_TEST_INFO("APN: %s", apnName);
    std::cout << "APN: " << apnName << std::endl;
    return result;
}

static le_result_t GetProfileName()
{
    le_result_t result;
    char profileName[TAF_DCS_NAME_MAX_LEN] = {0};
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetProfileName(ProfileRef, profileName, TAF_DCS_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get profile name failed");

    LE_TEST_INFO("Profile name: %s", profileName);
    std::cout << "Profile name: " << profileName << std::endl;
    return result;
}

static le_result_t GetTechPref()
{
    le_result_t result;
    taf_dcs_Tech_t techPref;
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetTechPreference(ProfileRef, &techPref);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get tech preference failed");

    LE_TEST_INFO("Tech Pref: %d(%s)", techPref, tafDCSHelper::TechPreferenceToString(techPref));
    std::cout << "Tech Pref: " << techPref << "("
                        << tafDCSHelper::TechPreferenceToString(techPref) << ")" << std::endl;
    return result;
}

static le_result_t GetApnTypeMask()
{
    le_result_t result;
    taf_dcs_ApnType_t apnTypeMask;
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetApnTypes(ProfileRef, &apnTypeMask);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get APN types failed");

    LE_TEST_INFO("APN types mask: %d(%s)", apnTypeMask,
                        tafDCSHelper::ApnTypeMaskToString(apnTypeMask).c_str());
    std::cout << "APN types mask: " << apnTypeMask << "("
                     << tafDCSHelper::ApnTypeMaskToString(apnTypeMask) << ")" << std::endl;
    return result;
}

static le_result_t GetPDP()
{
    taf_dcs_Pdp_t pdp;
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    pdp = taf_dcs_GetPDP(ProfileRef);
    LE_TEST_INFO("PDP(IP family): %d(%s)", pdp,
                 tafDCSHelper::IpFamilyTypeToString(pdp));
    std::cout << "PDP(IP family): " << pdp << "("
              << tafDCSHelper::IpFamilyTypeToString(pdp) << ")" << std::endl;
    return LE_OK;
}

static le_result_t GetAuthentication()
{
    le_result_t result;
    taf_dcs_Auth_t auth;
    char unStr[TAF_DCS_USER_NAME_MAX_LEN] = {0};     // 1 for trailing null
    char pwStr[TAF_DCS_PASSWORD_NAME_MAX_LEN] = {0}; // 1 for trailing null
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetAuthentication(ProfileRef, &auth,
                                       unStr, TAF_DCS_USER_NAME_MAX_LEN,
                                       pwStr, TAF_DCS_PASSWORD_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get authentication failed");

    LE_TEST_INFO("Auth Type : %s", tafDCSHelper::AuthMaskToString(auth).c_str());
    std::cout << "Auth Type : " << tafDCSHelper::AuthMaskToString(auth) << std::endl;
    if (strlen(unStr)>0)
    {
        LE_TEST_INFO("Username  : %s", unStr);
        std::cout << "Username  : " << unStr << std::endl;
    }
    if (strlen(pwStr) > 0)
    {
        LE_TEST_INFO("Password  : %s", pwStr);
        std::cout << "Password  : " << pwStr << std::endl;
    }

    return result;
}

static le_result_t GetProfileListEx(uint8_t phoneId)
{
    size_t listSize = 0;
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    taf_dcs_ProfileRef_t profileRef = NULL;
    std::string logStr;
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    taf_dcs_Pdp_t pdp;
    le_result_t result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "taf_dcs_GetProfileListEx failed");

    logStr.clear();
    logStr = logStr + "Index \t APN \t\t PDP";
    LE_TEST_INFO("%s", logStr.c_str());
    std::cout << logStr << std::endl;
    for (uint32_t i = 0; i < listSize; i++)
    {
        profileRef = NULL;
        logStr.clear();
        memset(apnStr, 0, TAF_DCS_APN_NAME_MAX_LEN);
        pdp = TAF_DCS_PDP_UNKNOWN;

        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        profileRef = taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);
        TAF_ERROR_IF_RET_VAL(NULL == profileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");
        result = taf_dcs_GetAPN(profileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "taf_dcs_GetAPN failed");
        pdp = taf_dcs_GetPDP(profileRef);

        logStr = logStr + std::to_string(profileInfoPtr->index) + "\t" + apnStr + "\t\t" +
                 tafDCSHelper::IpFamilyTypeToString(pdp);
        LE_TEST_INFO("%s", logStr.c_str());
        std::cout << logStr << std::endl;
    }
    return result;
}

static le_result_t GetProfileListEx()
{
    LE_TEST_INFO("Get profile list");
    int phoneID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return GetProfileListEx(static_cast<uint8_t>(phoneID));
}

static le_result_t SetTechPreference(taf_dcs_ProfileRef_t ProfileRef)
{
    int intInput = 1;
    le_result_t result = LE_OK;
    std::cout << "Tech preference: " << std::endl;
    std::cout << 0 << "-" << "skip setting tech preference" << std::endl;
    std::cout << TAF_DCS_TECH_3GPP << "-"
                        << tafDCSHelper::TechPreferenceToString(TAF_DCS_TECH_3GPP)  << std::endl;
    std::cout << TAF_DCS_TECH_3GPP2 << "-"
                        << tafDCSHelper::TechPreferenceToString(TAF_DCS_TECH_3GPP2) << std::endl;
    std::cout << TAF_DCS_TECH_ANY << "-"
                        << tafDCSHelper::TechPreferenceToString(TAF_DCS_TECH_ANY)   << std::endl;
    std::cout << "Enter tech preference: " << std::endl;
    std::cin  >> intInput;

    if (0 != intInput)
    {
        result = taf_dcs_SetTechPreference(ProfileRef, static_cast<taf_dcs_Tech_t>(intInput));
        if (LE_OK != result)
        {
            LE_TEST_INFO("Failed to set tech pref: %d", result);
        }
        else
        {
            LE_TEST_INFO("Tech pref set: %d(%s)", intInput,
                    tafDCSHelper::TechPreferenceToString(static_cast<taf_dcs_Tech_t>(intInput)));
        }
    }
    {
        LE_TEST_INFO("Skipped setting tech preference.");
    }
    return result;
}

static le_result_t SetTechPreference()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetTechPreference(ProfileRef);
}

static le_result_t SetApnTypeMask(taf_dcs_ProfileRef_t ProfileRef)
{
    int intInput = 0;
    le_result_t result = LE_OK;

    std::cout << "APN type mask: " << std::endl;
    std::cout << 0 << "-" << "skip setting APN type" << std::endl;
    std::cout << TAF_DCS_APN_TYPE_DEFAULT << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_DEFAULT) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_IMS << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_IMS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_MMS << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_MMS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_DUN << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_DUN) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_SUPL << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_SUPL) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_HIPRI << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_HIPRI) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_FOTA << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_FOTA) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_CBS << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_CBS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_IA << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_IA) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_EMERGENCY << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_EMERGENCY) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_UT << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_UT) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_MCX << "-"
              << tafDCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_MCX) << std::endl;
    std::cout << "Enter APN type mask(OR the types needed. e.g DEFAULT|IMS=3): " << std::endl;
    std::cin.clear();
    std::cin >> intInput;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ApnType_t apnTypeMask = static_cast<taf_dcs_ApnType_t>(intInput);
    if (0 != intInput)
    {
        result = taf_dcs_SetApnTypes(ProfileRef, apnTypeMask);
        if (LE_OK != result)
        {
            LE_TEST_INFO("Failed to APN type mask: %d", result);
        }
        else
        {
            LE_TEST_INFO("APN type mask set: %d(%s)", intInput,
                         tafDCSHelper::ApnTypeMaskToString(apnTypeMask).c_str());
        }
    }
    {
        LE_TEST_INFO("Skipped setting APN type mask.");
    }
    return result;
}

static le_result_t SetApnTypeMask()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetApnTypeMask(ProfileRef);
}

static le_result_t SetPDP(taf_dcs_ProfileRef_t ProfileRef)
{
    le_result_t result = LE_OK;
    int intInput = 0;
    std::cout << "Packet Data Protocol(PDP) type: " << std::endl;
    std::cout << 0 << "-" << "skip setting PDP" << std::endl;
    std::cout << TAF_DCS_PDP_IPV4 << "-"
              << tafDCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV4) << std::endl;
    std::cout << TAF_DCS_PDP_IPV6 << "-"
              << tafDCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV6) << std::endl;
    std::cout << TAF_DCS_PDP_IPV4V6 << "-"
              << tafDCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV4V6) << std::endl;
    std::cout << "Enter PDP: " << std::endl;
    std::cin.clear();
    std::cin >> intInput;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (0 != intInput)
    {
        result = taf_dcs_SetPDP(ProfileRef, static_cast<taf_dcs_Pdp_t>(intInput));
        if (LE_OK != result)
        {
            LE_TEST_INFO("Failed to set PDP: %d", result);
        }
        else
        {
            LE_TEST_INFO("PDP set: %d(%s)", intInput,
                        tafDCSHelper::IpFamilyTypeToString(static_cast<taf_dcs_Pdp_t>(intInput)));
        }
    }
    {
        LE_TEST_INFO("Skipped setting PDP");
    }
    return result;
}

static le_result_t SetPDP()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetPDP(ProfileRef);
}

static le_result_t SetProfileName(taf_dcs_ProfileRef_t ProfileRef)
{
    le_result_t result;
    char profileName[TAF_DCS_NAME_MAX_LEN + 1] = {0}; // 1 for trailing null
    std::cout << "Enter profile name(max " << TAF_DCS_NAME_MAX_LEN << " characters):  ";
    std::cin.getline(profileName, (TAF_DCS_NAME_MAX_LEN));
    result = taf_dcs_SetProfileName(ProfileRef, profileName);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to set profile name: %d", result);
    }
    else
    {
        LE_TEST_INFO("Profile name set: %s", profileName);
    }
    return result;
}

static le_result_t SetProfileName()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetProfileName(ProfileRef);
}

static le_result_t SetAPN(taf_dcs_ProfileRef_t ProfileRef)
{
    le_result_t result;
    char apnName[TAF_DCS_APN_NAME_MAX_LEN + 1] = {0}; // 1 for trailing null

    std::cout << "Enter APN(max " << TAF_DCS_APN_NAME_MAX_LEN << " characters):  ";
    std::cin.getline(apnName, (TAF_DCS_APN_NAME_MAX_LEN));
    result = taf_dcs_SetAPN(ProfileRef, apnName);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to set APN: %d", result);
    }
    else
    {
        LE_TEST_INFO("APN set: %s", apnName);
    }
    return result;
    }

static le_result_t SetAPN()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetAPN(ProfileRef);
}

static le_result_t SetAuthentication(taf_dcs_ProfileRef_t ProfileRef)
{
    le_result_t result = LE_OK;
    int intInput = 0;
    taf_dcs_Auth_t auth;
    char unStr[TAF_DCS_USER_NAME_MAX_LEN + 1] = {0};     // 1 for trailing null
    char pwStr[TAF_DCS_PASSWORD_NAME_MAX_LEN + 1] = {0}; // 1 for trailing null

    //Auth mask
    std::cout << "Authentication mask: " << std::endl;
    std::cout << 0 << "-" << "skip setting authentication" << std::endl;
    std::cout << TAF_DCS_AUTH_NONE << "-"
              << tafDCSHelper::AuthMaskToString(TAF_DCS_AUTH_NONE) << std::endl;
    std::cout << TAF_DCS_AUTH_PAP << "-"
              << tafDCSHelper::AuthMaskToString(TAF_DCS_AUTH_PAP) << std::endl;
    std::cout << TAF_DCS_AUTH_CHAP << "-"
              << tafDCSHelper::AuthMaskToString(TAF_DCS_AUTH_CHAP) << std::endl;

    std::cout << "Enter auth type mask(OR the types needed. e.g PAP|CHAP=6): " << std::endl;
    std::cin.clear();
    std::cin >> intInput;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (0 == intInput)
    {
        LE_TEST_INFO("Skip setting authentication");
        return LE_OK;
    }

    // Get username
    std::cout << "Enter username(max " << TAF_DCS_USER_NAME_MAX_LEN << " characters):  ";
    std::cin.getline(unStr, (TAF_DCS_USER_NAME_MAX_LEN));

    // Get password
    std::cout << "Enter password(max " << TAF_DCS_PASSWORD_NAME_MAX_LEN << " characters):  ";
    std::cin.getline(pwStr, (TAF_DCS_PASSWORD_NAME_MAX_LEN));

    auth = static_cast<taf_dcs_Auth_t>(intInput);

    result = taf_dcs_SetAuthentication(ProfileRef, auth,unStr, pwStr);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to set authentication: %d", result);
    }
    else
    {
        LE_TEST_INFO("Authentication set: %s, %s, %s",
                     tafDCSHelper::AuthMaskToString(auth).c_str(), unStr, pwStr);
    }
    return result;
    }

static le_result_t SetAuthentication()
{
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return SetAuthentication(ProfileRef);
}

static le_result_t CreateProfile()
{
    LE_TEST_INFO("Create profile");
    taf_dcs_ProfileRef_t ProfileRef;
    le_result_t result;
    int intInput = 1;
    uint32_t profileId;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> intInput;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    ProfileRef = GetProfileRef(static_cast<uint8_t>(intInput));
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref for phone id: %d", intInput);
        return LE_FAULT;
    }

    // APN
    result = SetAPN(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set APN failed");

    // Profile name
    result = SetProfileName(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set profile name failed");

    // Technology preference
    result = SetTechPreference(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set tech preference failed");

    // APN types
    result = SetApnTypeMask(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set APN type mask failed");

    // PDP
    result = SetPDP(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set PDP(ip family type) failed");

    // Authentication
    result = SetAuthentication(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Set authentication failed");

    // Create profile
    result = taf_dcs_CreateProfile(ProfileRef);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Create profile failed");

    // Get the created profile id
    result = taf_dcs_GetProfileId(ProfileRef, &profileId);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get profile id failed");

    LE_TEST_INFO("Created Profile Id: %d", profileId);
    std::cout << "Created Profile Id: " << profileId << std::endl;

    return result;
}

static le_result_t DeleteProfile()
{
    LE_TEST_INFO("Delete profile");
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    return taf_dcs_DeleteProfile(ProfileRef);
}

static le_result_t GetDefaultPhoneIdAndProfileId()
{
    le_result_t result;
    uint8_t defaultPhoneId = 0;
    uint32_t defaultProfileId = 0;
    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defaultPhoneId, &defaultProfileId);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get default phone id and profile id failed");
    LE_TEST_INFO("Default phone id: %d, Default profile id: %d", defaultPhoneId, defaultProfileId);
    std::cout << "Default phone id: " << static_cast<int>(defaultPhoneId)
              << ", Default profile id: " << defaultProfileId << std::endl;
    return result;
}

static le_result_t GetDataBearerTechnology()
{
    LE_TEST_INFO("Get data bearer technology");
    le_result_t result = LE_OK;

    taf_dcs_DataBearerTechnology_t upTech, downTech;

    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetDataBearerTechnology(ProfileRef, &upTech, &downTech);
    if (LE_OK!=result)
    {
        if (LE_UNAVAILABLE == result)
        {
            LE_TEST_INFO("LE_UNAVAILABLE: Data call not active");
            std::cout << "LE_UNAVAILABLE: Data call not active." << std::endl;
        }
        return result;
    }
    LE_TEST_INFO("Data bearer technology: uplink=%s",
                                            tafDCSHelper::DataBearerTechnologyToString(upTech));
    LE_TEST_INFO("Data bearer technology: downlink=%s",
                                            tafDCSHelper::DataBearerTechnologyToString(downTech));
    std::cout << "Data bearer tech: uplink   = " <<
                                            tafDCSHelper::DataBearerTechnologyToString(upTech) <<
                                            std::endl;
    std::cout << "Data bearer tech: downlink = " <<
                                            tafDCSHelper::DataBearerTechnologyToString(downTech) <<
                                            std::endl;
    return result;
}

static le_result_t GetRoamingStatus()
{
    LE_TEST_INFO("Get roaming status");
    le_result_t result = LE_OK;
    bool isRoaming;
    taf_dcs_RoamingType_t roamingType;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    result = taf_dcs_GetRoamingStatus(phoneID, &isRoaming, &roamingType);
    if (LE_OK!=result)
    {
        LE_TEST_INFO("Failed to get roaming status");
        return result;
    }
    LE_TEST_INFO("Phone id: %d", phoneID);
    LE_TEST_INFO("Is roaming: %s", isRoaming ? "true" : "false");
    LE_TEST_INFO("Roaming type: %s", tafDCSHelper::RoamingTypeToString(roamingType));
    std::cout << "Phone id: " << static_cast<int>(phoneID) << std::endl;
    std::cout << "Is roaming: " << (isRoaming ? "true" : "false") << std::endl;
    std::cout << "Roaming type: " << tafDCSHelper::RoamingTypeToString(roamingType) << std::endl;
    return result;
}

static le_result_t GetMaxDataBitRates()
{
    LE_TEST_INFO("Get max data bit rates");
    le_result_t result = LE_OK;

    uint64_t RxBitRate, TxBitRate;

    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetMaxDataBitRates(ProfileRef, &RxBitRate, &TxBitRate);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get max bit rates: %d", result);
        std::cout << "Failed to get max bit rates: " << result << std::endl;
        if (LE_UNAVAILABLE == result)
        {
            LE_TEST_INFO("LE_UNAVAILABLE: Data call not active");
            std::cout << "LE_UNAVAILABLE: Data call not active." << std::endl;
        }
        return result;
    }
    LE_TEST_INFO("Max bit rates in bits/sec. Rx: %" PRIu64 ",Tx: %" PRIu64 " ",
                                                                            RxBitRate, TxBitRate);
    std::cout << "Max bit rates in bits/sec. Rx: " << RxBitRate << ",Tx: " << TxBitRate
                                                                            << std::endl;
    return result;
}

static le_result_t GetCallEndReason()
{
    LE_TEST_INFO("Get call end reason");
    le_result_t result = LE_OK;
    std::string logStr;

    taf_dcs_CallEndReasonType_t callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    int32_t callEndReasonCode = -1;

    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }

    // IPv4
    result = taf_dcs_GetCallEndReason(ProfileRef, TAF_DCS_PDP_IPV4,
                                                        &callEndReasonType, &callEndReasonCode);
    if (LE_OK != result)
    {
        logStr.clear();
        logStr = logStr + "IPv4 taf_dcs_GetCallEndReason: " +
                 std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
        LE_TEST_OK(LE_OK == result, "%s", logStr.c_str());
        std::cout << logStr << std::endl;
    }
    const char *CallEndReasonTypeStr4 = tafDCSHelper::CallEndReasonTypeToString(callEndReasonType);
    const char *CallEndReasonCodeStr4 = tafDCSHelper::CallEndReasonCodeToString(
                                                             callEndReasonType, callEndReasonCode);

    LE_TEST_INFO("IPv4 Call end reason type: %d(%s)", callEndReasonType, CallEndReasonTypeStr4);
    std::cout << "IPv4 Call end reason type: " << callEndReasonType
                                               << "(" << CallEndReasonTypeStr4 << ")" << std::endl;

    LE_TEST_INFO("IPv4 Call end reason code: %d(%s)", callEndReasonCode, CallEndReasonCodeStr4);
    std::cout << "IPv4 Call end reason code: " << callEndReasonCode;
    if (CallEndReasonCodeStr4)
    {
        std::cout << "(" << CallEndReasonCodeStr4 << ")";
    }
    std::cout << std::endl;

    // IPv6
    result = taf_dcs_GetCallEndReason(ProfileRef, TAF_DCS_PDP_IPV6,
                                      &callEndReasonType, &callEndReasonCode);
    if (LE_OK != result)
    {
        logStr.clear();
        logStr = logStr + "IPv6 taf_dcs_GetCallEndReason: " +
                 std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
        LE_TEST_OK(LE_OK == result, "%s", logStr.c_str());
        std::cout << logStr << std::endl;
        return result;
    }
    const char *CallEndReasonTypeStr6 = tafDCSHelper::CallEndReasonTypeToString(callEndReasonType);
    const char *CallEndReasonCodeStr6 = tafDCSHelper::CallEndReasonCodeToString(
        callEndReasonType, callEndReasonCode);

    LE_TEST_INFO("IPv6 Call end reason type: %d(%s)", callEndReasonType, CallEndReasonTypeStr6);
    std::cout << "IPv6 Call end reason type: " << callEndReasonType
                                               << "(" << CallEndReasonTypeStr6 << ")" << std::endl;

    LE_TEST_INFO("IPv6 Call end reason code: %d(%s)", callEndReasonCode, CallEndReasonCodeStr6);
    std::cout << "IPv6 Call end reason code: " << callEndReasonCode;
    if (CallEndReasonCodeStr6)
    {
        std::cout << "(" << CallEndReasonCodeStr6 << ")";
    }
    std::cout << std::endl;
    return result;
}

static le_result_t GetAPNThrottleStatus()
{
    le_result_t result = LE_OK;
    bool       isThrottled;
    uint32_t     ipv4RemainingTime;
    uint32_t     ipv6RemainingTime;

    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }

    result = taf_dcs_GetAPNThrottledStatus(ProfileRef,&isThrottled,&ipv4RemainingTime,
                                                                       &ipv6RemainingTime);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get throttled status failed");

    LE_TEST_INFO("----isThrottled : %d", (bool)isThrottled);
    LE_TEST_INFO("----ipv4 Time : %d", (int)ipv4RemainingTime);
    LE_TEST_INFO("----ipv6 Time : %d", (int)ipv6RemainingTime);

    std::cout << "apn throttle status: " << isThrottled << ",ipv4: " << ipv4RemainingTime
                                                        << ",ipv6: " << ipv6RemainingTime
                                                                     << std::endl;
    return result;
}

static le_result_t GetAPNThrottledPLMN()
{
    le_result_t result = LE_OK;
    bool  areAllPLMNsThrottled;
    char mccStr[TAF_DCS_MCC_BYTES];
    char mncStr[TAF_DCS_MNC_BYTES];

    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }

    result = taf_dcs_GetAPNThrottledPLMN(ProfileRef, &areAllPLMNsThrottled,
                                                       mccStr,TAF_DCS_MCC_BYTES,
                                                       mncStr,TAF_DCS_MNC_BYTES);

    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get throttled PLMN failed");

    LE_TEST_INFO("----areAllPLMNsThrottled : %d", (bool)areAllPLMNsThrottled);
    LE_TEST_INFO("----MCC : %s", mccStr);
    LE_TEST_INFO("----MNC : %s", mncStr);

    std::cout << "areAllPLMNsThrottled: " << areAllPLMNsThrottled
              << ", MCC: " << mccStr << ", MNC: " << mncStr << std::endl;

    return result;
}

static le_result_t GetMtu()
{
    le_result_t result;
    uint16_t  mtu;
    taf_dcs_ProfileRef_t ProfileRef = GetProfileRef();
    if (nullptr == ProfileRef)
    {
        LE_TEST_INFO("Failed to get profile ref");
        return LE_FAULT;
    }
    result = taf_dcs_GetMtu(ProfileRef, &mtu);
    TAF_ERROR_IF_RET_VAL((LE_OK != result), result, "Get MTU failed");

    LE_TEST_INFO("MTU value: %d", mtu);
    std::cout << "MTU value:   " << mtu << std::endl;
    return result;
}

static std::promise<le_result_t> StartSessionDeferredPromise;
static void StartSessionDeferred(void *valuePtr)
{
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)valuePtr;
    le_result_t result = taf_dcs_StartSession(ProfileRef);
    LE_TEST_INFO("taf_dcs_StartSession result: %d", result);
    StartSessionDeferredPromise.set_value(result);
}

static le_result_t StartSession()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = GetProfileListEx(static_cast<uint8_t>(phoneID));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetProfileListEx failed");

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    StartSessionDeferredPromise = std::promise<le_result_t>();
    std::future<le_result_t> fut = StartSessionDeferredPromise.get_future();
    // Send the command from another thread and wait for the result. If this is not done, the sync
    // and async data will be started from different contexts and DCS won't be able to find it
    // properly.
    le_event_QueueFunctionToThread(asyncCmdThreadRef,
                                   (le_event_DeferredFunc_t)StartSessionDeferred,
                                   ProfileRef, NULL);
    result = fut.get();
    return result;
}

static std::promise<le_result_t> StartSessionAsyncPromise;
static void asyncStartHandler(taf_dcs_ProfileRef_t profileRef, le_result_t result, void *contextPtr)
{
    uint32_t profileId = 0;
    LE_TEST_INFO("Async Handler Result: %d", result);
    le_result_t getIdResult = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK == getIdResult, "taf_dcs_GetProfileId: %d", result);
    LE_TEST_INFO("Profile ID: %d", profileId);
    StartSessionAsyncPromise.set_value(result);
}

// Async commands need to be run from a separate Legato thread (not from the main thread)
static void StartSessionAsyncDeferred(void *valuePtr)
{
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)valuePtr;
    taf_dcs_StartSessionAsync(ProfileRef, asyncStartHandler, nullptr);
    LE_TEST_INFO("Sent taf_dcs_StartSessionAsync");
}

static le_result_t StartSessionAsync()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = GetProfileListEx(static_cast<uint8_t>(phoneID));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetProfileListEx failed");

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    StartSessionAsyncPromise = std::promise<le_result_t>();
    std::future<le_result_t> fut = StartSessionAsyncPromise.get_future();
    auto start = std::chrono::high_resolution_clock::now();

    // Send the command from another thread and wait for the result.
    le_event_QueueFunctionToThread( asyncCmdThreadRef,
                                    (le_event_DeferredFunc_t)StartSessionAsyncDeferred,
                                    ProfileRef, NULL);
    LE_TEST_INFO("Waiting on async response...");
    std::cout << "Waiting on async response..." << std::endl;
    result = fut.get();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    LE_TEST_INFO("taf_dcs_StartSessionAsync result: %d", result);
    LE_TEST_INFO("taf_dcs_StartSessionAsync time elapsed: %.2f s", elapsed.count());
    std::cout << "Time elapsed: " << std::fixed << std::setprecision(2)
                                  << elapsed.count() << " seconds" << std::endl;

    return result;
}

static std::promise<le_result_t> StopSessionDeferredPromise;
static void StopSessionDeferred(void *valuePtr)
{
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)valuePtr;
    le_result_t result = taf_dcs_StopSession(ProfileRef);
    LE_TEST_INFO("taf_dcs_StopSession result: %d", result);
    StopSessionDeferredPromise.set_value(result);
}

static le_result_t StopSession()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = GetProfileListEx(static_cast<uint8_t>(phoneID));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetProfileListEx failed");

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    StopSessionDeferredPromise = std::promise<le_result_t>();
    std::future<le_result_t> fut = StopSessionDeferredPromise.get_future();
    // Send the command from another thread and wait for the result. If this is not done, the sync
    // and async data will be started from different contexts and DCS won't be able to find it
    // properly.
    le_event_QueueFunctionToThread(asyncCmdThreadRef,
                                   (le_event_DeferredFunc_t)StopSessionDeferred,
                                   ProfileRef, NULL);
    result = fut.get();
    return result;
}

static std::promise<le_result_t> StopSessionAsyncPromise;
static void asyncStopHandler(taf_dcs_ProfileRef_t profileRef, le_result_t result, void *contextPtr)
{
    uint32_t profileId = 0;
    LE_TEST_INFO("Async Handler Result: %d", result);
    le_result_t getIdResult = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK == getIdResult, "taf_dcs_GetProfileId: %d", result);
    LE_TEST_INFO("Profile ID: %d", profileId);
    StopSessionAsyncPromise.set_value(result);
}

// Async commands need to be run from a separate Legato thread (not from the main thread)
static void StopSessionAsyncDeferred(void *valuePtr)
{
    taf_dcs_ProfileRef_t ProfileRef = (taf_dcs_ProfileRef_t)valuePtr;
    taf_dcs_StopSessionAsync(ProfileRef, asyncStopHandler, nullptr);
    LE_TEST_INFO("Sent StopSessionAsyncDeferred");
}

static le_result_t StopSessionAsync()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = GetProfileListEx(static_cast<uint8_t>(phoneID));
    TAF_ERROR_IF_RET_VAL(LE_OK != result, result, "GetProfileListEx failed");

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    StopSessionAsyncPromise = std::promise<le_result_t>();
    std::future<le_result_t> fut = StopSessionAsyncPromise.get_future();
    auto start = std::chrono::high_resolution_clock::now();
    // Send the command from another thread and wait for response
    le_event_QueueFunctionToThread( asyncCmdThreadRef,
                                    (le_event_DeferredFunc_t)StopSessionAsyncDeferred,
                                    ProfileRef, NULL);

    LE_TEST_INFO("Waiting on async response...");
    std::cout << "Waiting on async response..." << std ::endl;
    result = fut.get();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    LE_TEST_INFO("taf_dcs_StopSessionAsync result: %d", result);
    LE_TEST_INFO("taf_dcs_StopSessionAsync time elapsed: %.2f s", elapsed.count());
    std::cout << "Time elapsed: " << std::fixed << std::setprecision(2)
                                  << elapsed.count() << " seconds" << std::endl;

    return result;
}

static std::string GetStateString(taf_dcs_ConState_t state)
{
    switch (state)
    {
        case TAF_DCS_CONNECTING:
            return "TAF_DCS_CONNECTING";
        case TAF_DCS_CONNECTED:
            return "TAF_DCS_CONNECTED";
        case TAF_DCS_DISCONNECTING:
            return "TAF_DCS_DISCONNECTING";
        case TAF_DCS_DISCONNECTED:
        default:
            return "TAF_DCS_DISCONNECTED";
    };
}

static le_result_t GetSessionState()
{
    int phoneID = 1;
    int profileID = 1;
    taf_dcs_ConState_t state;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetSessionState(ProfileRef, &state);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetSessionState");
    LE_TEST_INFO("State: %d(%s)", state, GetStateString(state).c_str());
    std::cout << "State: " << state << "(" << GetStateString(state) << ")" << std::endl;

    return result;
}

static le_result_t GetIPv4Address()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv4Address(ProfileRef, ipAddr, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address");
    LE_TEST_INFO("IPv4 Address: %s", ipAddr);
    std::cout << "IPv4 Address: " << ipAddr << std::endl;
    return result;
}

static le_result_t GetIPv6Address()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv6Address(ProfileRef, ipAddr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6Address");
    LE_TEST_INFO("IPv6 Address: %s", ipAddr);
    std::cout << "IPv6 Address: " << ipAddr << std::endl;
    return result;
}

static le_result_t GetIPv4DNSAddresses()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    char ipAddr2[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv4DNSAddresses(ProfileRef, ipAddr1, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                     ipAddr2, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses");
    LE_TEST_INFO("IPv4 Dns1: %s, Dns2: %s", ipAddr1, ipAddr2);
    std::cout << "IPv4 Dns1: " << ipAddr1 << ", Dns2: " << ipAddr2 << std::endl;
    return result;
}

static le_result_t GetIPv6DNSAddresses()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr1[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    char ipAddr2[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv6DNSAddresses(ProfileRef, ipAddr1, TAF_DCS_IPV6_ADDR_MAX_LEN,
                                                     ipAddr2, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6DNSAddresses");
    LE_TEST_INFO("IPv6 Dns1: %s, Dns2: %s", ipAddr1, ipAddr2);
    std::cout << "IPv6 Dns1: " << ipAddr1 << ", Dns2: " << ipAddr2 << std::endl;
    return result;
}

static le_result_t GetIPv4GatewayAddress()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv4GatewayAddress(ProfileRef, ipAddr, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress");
    LE_TEST_INFO("IPv4 gateway address: %s", ipAddr);
    std::cout << "IPv4 gateway address: " << ipAddr << std::endl;
    return result;
}

static le_result_t GetIPv6GatewayAddress()
{
    int phoneID = 1;
    int profileID = 1;
    char ipAddr[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv6GatewayAddress(ProfileRef, ipAddr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6GatewayAddress");
    LE_TEST_INFO("IPv6 gateway address: %s", ipAddr);
    std::cout << "IPv6 gateway address: " << ipAddr << std::endl;
    return result;
}

static le_result_t GetIPv4SubnetMask()
{
    int phoneID = 1;
    int profileID = 1;
    uint32_t mask = 0;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv4SubnetMask(ProfileRef, &mask);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4SubnetMask");
    LE_TEST_INFO("IPv4 subnet mask: %d", mask);
    std::cout << "IPv4 subnet mask: " << mask << std::endl;
    return result;
}

static le_result_t GetIPv6SubnetMask()
{
    int phoneID = 1;
    int profileID = 1;
    uint32_t mask = 0;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetIPv6SubnetMask(ProfileRef, &mask);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv6SubnetMask");
    LE_TEST_INFO("IPv6 subnet mask: %d", mask);
    std::cout << "IPv6 subnet mask: " << mask << std::endl;
    return result;
}

static le_result_t IsIPv4()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    bool bIsIPv4 = taf_dcs_IsIPv4(ProfileRef);
    LE_TEST_INFO("Is IPv4: %d", bIsIPv4);
    std::cout << "Is IPv4: " << bIsIPv4 << std::endl;
    return LE_OK;
}

static le_result_t IsIPv6()
{
    int phoneID = 1;
    int profileID = 1;
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    bool bIsIPv6 = taf_dcs_IsIPv6(ProfileRef);
    LE_TEST_INFO("Is IPv6: %d", bIsIPv6);
    std::cout << "Is IPv6: " << bIsIPv6 << std::endl;
    return LE_OK;
}

static le_result_t GetPhoneId()
{
    int phoneID = 1;
    int profileID = 1;
    uint8_t phoneIDOut = 0;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetPhoneId(ProfileRef, &phoneIDOut);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "taf_dcs_GetPhoneId failed");

    LE_TEST_INFO("Phone ID: %d", phoneIDOut);
    std::cout << "Phone ID: " << static_cast<int>(phoneIDOut) << std::endl;
    return result;
}

static le_result_t GetInterfaceName()
{
    int phoneID = 1;
    int profileID = 1;
    char ifNameStr[TAF_DCS_NAME_MAX_LEN] = {0};
    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile index:  ";
    std::cin.clear();
    std::cin >> profileID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_dcs_ProfileRef_t ProfileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID),
                                                           static_cast<uint32_t>(profileID));
    TAF_ERROR_IF_RET_VAL(nullptr == ProfileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");

    le_result_t result = taf_dcs_GetInterfaceName(ProfileRef, ifNameStr, TAF_DCS_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "taf_dcs_GetInterfaceName failed");

    LE_TEST_INFO ("Interface name: %s", ifNameStr);
    std::cout << "Interface name: " << ifNameStr << std::endl;
    return result;
}

static le_result_t GetPhoneIdByInterfaceName()
{
    uint8_t phoneID = 0;
    char ifNameStr[TAF_DCS_NAME_MAX_LEN] = {0};
    std::cout << "Enter interface name:  ";
    std::cin.getline(ifNameStr, TAF_DCS_NAME_MAX_LEN);

    le_result_t result = taf_dcs_GetPhoneIdByInterfaceName(ifNameStr, &phoneID);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "taf_dcs_GetPhoneIdByInterfaceName failed");

    LE_TEST_INFO("Phone Id: %d", phoneID);
    std::cout << "Phone Id: " << static_cast<int>(phoneID) << std::endl;
    return result;
}

static le_result_t GetProfileIdByInterfaceName()
{
    uint32_t profileID = 0;
    char ifNameStr[TAF_DCS_NAME_MAX_LEN] = {0};
    std::cout << "Enter interface name:  ";
    std::cin.getline(ifNameStr, TAF_DCS_NAME_MAX_LEN);

    le_result_t result = taf_dcs_GetProfileIdByInterfaceName(ifNameStr, &profileID);
    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "taf_dcs_GetProfileIdByInterfaceName failed");

    LE_TEST_INFO("Profile Id: %d", profileID);
    std::cout << "Profile Id: " << static_cast<int>(profileID) << std::endl;
    return result;
}

static void HwAccStateHandlerFunc(  taf_dcs_ProfileRef_t profileRef,
                                    taf_dcs_HwAccelerationState_t state,
                                    void *contextPtr)
{
    LE_UNUSED(contextPtr);

    uint32_t profileId = 0;
    uint8_t phoneId = 0;
    le_result_t result = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK==result, "taf_dcs_GetProfileId result: %d", result);
    result = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetPhoneId result: %d", result);

    LE_TEST_INFO("Profile %d Hw accel state : %d(%s)", profileId, state,
                                                            (state ? "ACTIVE" : "INACTIVE"));

    // Print logs on console only if enabled
    if (!bPrintNotifLogsOnConsole)
        return;

    std::cout << "Phone ID: " << static_cast<int>(phoneId)
              << ", Profile ID: " << profileId
              << ", Hw accel state: " << state << "(" << (state ? "ACTIVE" : "INACTIVE") << ")"
              << std::endl;
}

static void ThrottledStatusHandlerFunc
(
    taf_dcs_ProfileRef_t    profileRef,        ///< The profile reference.
    bool       isThrottled,       ///< True when APN is throttled. False when APN is unthrottled.
    uint32_t     ipv4RemainingTime, ///< The remaining IPv4 throttled time in milliseconds.
    uint32_t     ipv6RemainingTime, ///< The remaining IPv6 throttled time in milliseconds.
    void* contextPtr
)
{
    uint32_t profileId = 0;
    uint8_t phoneId = 0;

    le_result_t result = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileId result: %d", result);
    result = taf_dcs_GetPhoneId(profileRef, &phoneId);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetPhoneId result: %d", result);


    LE_TEST_INFO("----Phone ID    : %d", (int)phoneId);
    LE_TEST_INFO("----Profile ID  : %d", (int)profileId);
    LE_TEST_INFO("----isThrottled : %d", (bool)isThrottled);
    LE_TEST_INFO("----IPv4 Time   : %d", (int)ipv4RemainingTime);
    LE_TEST_INFO("----IPv6 Time   : %d", (int)ipv6RemainingTime);

    // Print logs on console only if enabled
    if (!bPrintNotifLogsOnConsole)
        return;

    std::cout << "Phone ID: " << static_cast<int>(phoneId)
              << ", Profile ID: " << profileId
              << ", isThrottled: " << isThrottled
              << ", IPv4 time: " << ipv4RemainingTime
              << ", IPv6 time: " << ipv6RemainingTime
              << std::endl;
}

void RoamingStatusHandlerFunc(
    const taf_dcs_RoamingStatusInd_t *LE_NONNULL roamingStatusIndPtr,
    ///< Roaming status indication.
    void *contextPtr
    ///<
)
{
    LE_TEST_INFO("Phone Id     : %d", roamingStatusIndPtr->phoneId);
    LE_TEST_INFO("Is Roaming   : %d", roamingStatusIndPtr->isRoaming);
    LE_TEST_INFO("Roaming type : %s",
                 tafDCSHelper::RoamingTypeToString(roamingStatusIndPtr->type));

    // Print logs on console only if enabled
    if (!bPrintNotifLogsOnConsole)
        return;

    std::cout << "\tRoaming status callback" << std::endl;
    std::cout << "\t\tPhone Id     : " << roamingStatusIndPtr->phoneId << std::endl;
    std::cout << "\t\tIs Roaming   : " << roamingStatusIndPtr->isRoaming << std::endl;
    std::cout << "\t\tRoaming type : " <<
                    tafDCSHelper::RoamingTypeToString(roamingStatusIndPtr->type) << std::endl;
}

void SessionStateHandlerFunc
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    uint32_t profileId;
    le_result_t result;

    result = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_OK(LE_OK == result, "taf_dcs_GetProfileId: %d", result);
    LE_TEST_INFO("SessionStateHandlerFunc. Profile id: %d, callEvent: %s, ip type: %s",
                 profileId,
                 tafDCSHelper::CallEventToString(callEvent),
                 tafDCSHelper::IpFamilyTypeToString(infoPtr->ipType));

    // Print logs on console only if enabled
    if (!bPrintNotifLogsOnConsole)
        return;

    std::cout << "\tSessionStateHandlerFunc" << std::endl;
    std::cout << "\t\tProfile id: " << profileId << std::endl;
    std::cout << "\t\tCall event: " << tafDCSHelper::CallEventToString(callEvent) << std::endl;
    std::cout << "\t\tIP type   : " << tafDCSHelper::IpFamilyTypeToString(infoPtr->ipType)
                                    << std::endl;
}

void QosStatusHandlerFunc
(
    taf_dcs_QosFlowRef_t         qosFlowRef,
    taf_dcs_QosFlowState_t       qosState,
    void* contextPtr
)
{
    LE_TEST_INFO("**** Handler for qos status Indication (Begin)****");

    LE_TEST_INFO("----QOS State : %d", (int)qosState);
    if (bPrintNotifLogsOnConsole)
        std::cout << "\t\tQOS State: " << qosState << std::endl;

    uint32_t qosFlowId = 0;
    le_result_t result = taf_dcs_GetQosId(qosFlowRef,&qosFlowId);
    if(result == LE_OK)
    {
      LE_TEST_INFO("----Qos ID : %d", (int)qosFlowId);
      if (bPrintNotifLogsOnConsole)
        std::cout << "\t\tQos ID: " << qosFlowId << std::endl;
    }
    else
    {
      LE_TEST_INFO("----qos ID get error---");
      if (bPrintNotifLogsOnConsole)
          std::cout << "\t\t---qos ID get error---" << std::endl;
      return;
    }

    taf_dcs_QosFlowBitMask_t mask = 0;
    result = taf_dcs_GetQosParameterMask(qosFlowRef,&mask);
    if(result == LE_OK)
    {
      LE_TEST_INFO("----Qos Mask : %d", (int)mask);
      if (bPrintNotifLogsOnConsole)
          std::cout << "\t\tQos Mask: " << mask << std::endl;
    }
    else
    {
      LE_TEST_INFO("----qos mask get error---");
      if (bPrintNotifLogsOnConsole)
          std::cout << "\t\t---Qos Mask get error---" << std::endl;
    }

   if(mask & TAF_DCS_QOS_BIT_MASK_FLOW_NONE)
   {
       LE_TEST_INFO("No QOS flow mask installed");
       if (bPrintNotifLogsOnConsole)
           std::cout << "\t\tNo QOS flow mask installed" << std::endl;

       return ;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_TX_GRANTED)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_TX_GRANTED");
       if (bPrintNotifLogsOnConsole)
           std::cout << "\t\tQOS Mask == MASK_FLOW_TX_GRANTED" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_RX_GRANTED)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_RX_GRANTED");
       if (bPrintNotifLogsOnConsole)
           std::cout << "\t\tQOS Mask == MASK_FLOW_RX_GRANTED" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_TX_FILTERS)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_TX_FILTERS");
       if (bPrintNotifLogsOnConsole)
           std::cout << "\t\tQOS Mask == MASK_FLOW_TX_FILTERS" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_RX_FILTERS)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_RX_FILTERS");
       if (bPrintNotifLogsOnConsole)
           std::cout << "\t\tQOS Mask == MASK_FLOW_RX_FILTERS" << std::endl;
   }

    LE_TEST_INFO("**** Handler for qos status Indication (End)****");
}

static le_result_t GetDefaultProfileIndex()
{
    uint32_t profileId = 1;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = taf_dcs_GetDefaultProfileIndexEx(static_cast<uint8_t>(phoneID),
                                                        &profileId);
    LE_TEST_INFO("taf_dcs_SetDefaultProfileIndex result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("Default profile: %d", profileId);
        std::cout << "Default profile : " << profileId << std::endl;
    }
    return result;
}

static le_result_t SetDefaultProfileIndex()
{
    int profileId = 1;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter profile id:  ";
    std::cin.clear();
    std::cin >> profileId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    le_result_t result = taf_dcs_SetDefaultProfileIndexEx(static_cast<uint8_t>(phoneID),
                                                        static_cast<uint32_t>(profileId));
    LE_TEST_INFO("taf_dcs_SetDefaultProfileIndex result: %d", result);
    return result;
}

static void *callback_thread_handler(void *ctxPtr)
{
    taf_dcs_ConnectService();
    le_sem_Post((le_sem_Ref_t)ctxPtr);
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    uint8_t phoneId = TAF_TYPES_PHONE_ID_1;
    le_result_t result;

    // Add roaming status handler
    g_roamingStatusHandlerRef  = taf_dcs_AddRoamingStatusHandler(RoamingStatusHandlerFunc, NULL);

    // Add session state handler for all existing profiles for PHONE_ID_1
    result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
    LE_TEST_ASSERT(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d): %d",
                phoneId, result);

    for (size_t i = 0; i < listSize; i++)
    {
        taf_dcs_SessionStateHandlerRef_t handlerRef = nullptr;
        taf_dcs_QosStatusHandlerRef_t handlerQosRef = nullptr;
        taf_dcs_HwAccelerationStateHandlerRef_t handlerHwAccelRef = nullptr;
        taf_dcs_ThrottledStatusHandlerRef_t handlerThrottledRef = nullptr;

        taf_dcs_ProfileRef_t profileRef = nullptr;
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        profileRef = taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);
        LE_TEST_ASSERT(nullptr != profileRef, "taf_dcs_GetProfileEx: phone id(%d), \
                                                profile id: %d",
                       phoneId, profileInfoPtr->index);

        // add session state handler
        handlerRef = taf_dcs_AddSessionStateHandler(profileRef, SessionStateHandlerFunc, NULL);
        LE_TEST_ASSERT(nullptr != handlerRef, "taf_dcs_AddSessionStateHandler: phone id(%d), \
                                                profile id: %d", phoneId, profileInfoPtr->index);

        // add qos state handler
        handlerQosRef = taf_dcs_AddQosStatusHandler(profileRef, QosStatusHandlerFunc, NULL);
        LE_TEST_ASSERT(nullptr != handlerQosRef, "taf_dcs_AddQosStatusHandler: phone id(%d), \
                                                profile id: %d", phoneId, profileInfoPtr->index);

        // Add HW acceleration state handler
        handlerHwAccelRef = taf_dcs_AddHwAccelerationStateHandler(profileRef, HwAccStateHandlerFunc,
                                                                                              NULL);
        LE_TEST_ASSERT(nullptr != handlerHwAccelRef, "taf_dcs_AddHwAccelerationStateHandler: \
                                                                    phone id(%d),profile id: %d",
                                                                    phoneId, profileInfoPtr->index);

        // Throttled status handler
        handlerThrottledRef = taf_dcs_AddThrottledStatusHandler(profileRef,
                                                                ThrottledStatusHandlerFunc, NULL);
        LE_TEST_ASSERT(nullptr != handlerThrottledRef, "taf_dcs_AddThrottledStatusHandler: \
                                                                    phone id(%d),profile id: %d",
                       phoneId, profileInfoPtr->index);

        // Add the handler ref to the profile and session handler map
        g_Profile_SessionStateHandlerRef_Map[profileInfoPtr->index] = handlerRef;

        // Add the handler ref to the profile and qos handler map
        g_Profile_QosStatusHandlerRef_Map[profileInfoPtr->index] = handlerQosRef;

        // Add the handler ref to the profile and HW acceleration handler map
        g_Profile_HwAccelHandlerRef_Map[profileInfoPtr->index] = handlerHwAccelRef;

        // Add the handler ref to the profile and throttled state handler map
        g_Profile_ThrottledStatusHandlerRef_Map[profileInfoPtr->index] = handlerThrottledRef;

        // Set the references to nullptr
        profileRef = nullptr;
        handlerRef = nullptr;
        handlerQosRef = nullptr;
        handlerHwAccelRef = nullptr;
        handlerThrottledRef = nullptr;
    }

    // Start the event loop
    le_event_RunLoop();
    return NULL;
}

// Notification outputs are typically verbose and flood the console. This allows user to turn off
// console notification logs. "logread" output will not be affected
static void ToggleNotificationsOutputsOnConsole()
{
    char input;
    std::cout << "Turn off notification logs on console? (y/n): ";
    std::cin >> input;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if ('y' == input)
    {
        bPrintNotifLogsOnConsole = false;
        std::cout << "Notification logs will not be printed on console." << std::endl;
    }
    else
    {
        bPrintNotifLogsOnConsole = true;
        std::cout << "Notification logs will be printed on console." << std::endl;
    }
}

static void Register_Callbacks()
{
    LE_TEST_INFO("Starting callback thread");
    le_result_t result;

    // Get the number of slots
    int32_t simSlotCount = 0;
    result = taf_sim_GetSlotCount(&simSlotCount);
    LE_TEST_ASSERT(LE_OK == result, "taf_sim_GetSlotCount: %d", result);
    LE_TEST_ASSERT(simSlotCount >= 1, "Slot count: %d", simSlotCount);

    // There is at least 1 SIM. Let's use phone ID 1.
    le_sem_Ref_t callbackSemRef = nullptr;
    callbackSemRef = le_sem_Create("callbackSem", 0);

    callbackThreadRef = le_thread_Create("callback_thread", callback_thread_handler,
                                         callbackSemRef);
    le_thread_Start(callbackThreadRef);
    le_sem_Wait(callbackSemRef);
    le_sem_Delete(callbackSemRef);

    return;
}

static void UnRegister_Callbacks()
{
    LE_TEST_INFO("Stopping callback thread");

    // Remove handlers
    taf_dcs_RemoveRoamingStatusHandler(g_roamingStatusHandlerRef);

    for (const auto &pair : g_Profile_SessionStateHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_SessionStateHandlerRef_t handlerRef = pair.second;

        // Remove the session state handler
        LE_TEST_INFO("Removed session handler for profile ID: %d", profileId);
        taf_dcs_RemoveSessionStateHandler(handlerRef);
    }

    for (const auto &pair : g_Profile_QosStatusHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_QosStatusHandlerRef_t handlerRef = pair.second;

        // Remove the qos state handler
        LE_TEST_INFO("Removed qos state handler for profile ID: %d", profileId);
        taf_dcs_RemoveQosStatusHandler(handlerRef);
    }

    for (const auto &pair : g_Profile_HwAccelHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_HwAccelerationStateHandlerRef_t handlerRef = pair.second;

        // Remove the qos state handler
        LE_TEST_INFO("Removed HW accel state handler for profile ID: %d", profileId);
        taf_dcs_RemoveHwAccelerationStateHandler(handlerRef);
    }

    for (const auto &pair : g_Profile_ThrottledStatusHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_ThrottledStatusHandlerRef_t handlerRef = pair.second;

        // Remove the qos state handler
        LE_TEST_INFO("Removed throttled state handler for profile ID: %d", profileId);
        taf_dcs_RemoveThrottledStatusHandler (handlerRef);
    }

    // Stop the callback thread
    le_thread_Cancel(callbackThreadRef);
    le_thread_Join(callbackThreadRef, NULL);
    return;
}

static void *async_cmd_thread_handler(void *ctxPtr)
{
    taf_dcs_ConnectService();
    le_sem_Post((le_sem_Ref_t)ctxPtr);
    le_event_RunLoop();
    return NULL;
}

void tafDCSUnitTest_RunInteractiveTests()
{
    bool bRun = true;
    int option = -1;
    le_result_t result = LE_OK;
    le_sem_Ref_t asyncCmdSemRef = nullptr;
    std::string logStr;
    asyncCmdSemRef = le_sem_Create("asyncCmdSem", 0);

    asyncCmdThreadRef = le_thread_Create("async_cmd_thread", async_cmd_thread_handler,
                                         asyncCmdSemRef);
    le_thread_Start(asyncCmdThreadRef);
    le_sem_Wait(asyncCmdSemRef);
    le_sem_Delete(asyncCmdSemRef);

    Register_Callbacks();
    ShowMenu();
    while (bRun)
    {
        std::cout << "Enter option to test: ";
        if (!(std::cin >> option))
        {
            std::cout << "Not a number" << std::endl;
            std::cin.clear(); // Clear the error flag
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (option)
        {
            case 0:
            {
                ShowMenu();
                option = -1;
                break;
            }
            case 98:
            {
                ToggleNotificationsOutputsOnConsole();
                break;
            }
            case 99:
            {
                // Stop the test
                bRun = false;
                break;
            }
            case GET_DEFAULT_PHONE_AND_PROFILE:
            {
                result = GetDefaultPhoneIdAndProfileId();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetDefaultPhoneIdAndProfileId: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_DATA_BEARER_TECH:
            {
                result = GetDataBearerTechnology();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetDataBearerTechnology: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_ROAMING_STATUS:
            {
                result = GetRoamingStatus();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetRoamingStatus: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_MAX_DATA_BIT_RATES:
            {
                result = GetMaxDataBitRates();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetMaxDataBitRates: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_CALL_END_REASON:
            {
                result = GetCallEndReason();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetCallEndReason: " +
                                    std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO ("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_LIST:
            {
                result = GetProfileListEx();
                logStr.clear();
                logStr = logStr + "GetProfileListEx: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_CREATE:
            {
                result = CreateProfile();
                logStr.clear();
                logStr = logStr + "taf_dcs_CreateProfile: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_DELETE:
            {
                result = DeleteProfile();
                logStr.clear();
                logStr = logStr + "taf_dcs_DeleteProfile: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_APN:
            {
                result = SetAPN();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetAPN: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_NAME:
            {
                result = SetProfileName();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetProfileName: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_TECH_PREF:
            {
                result = SetTechPreference();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetTechPreference: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_APN_TYPE_MASK:
            {
                result = SetApnTypeMask();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetApnTypes: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_PDP:
            {
                result = SetPDP();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetPDP: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_AUTHENTICATION:
            {
                result = SetAuthentication();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetAuthentication: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_ID:
            {
                result = GetProfileId();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetProfileId: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_APN:
            {
                result = GetAPN();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetAPN: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_NAME:
            {
                result = GetProfileName();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetProfileName: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_TECH_PREF:
            {
                result = GetTechPref();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetTechPreference: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_APN_TYPE_MASK:
            {
                result = GetApnTypeMask();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetApnTypes: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_PDP:
            {
                result = GetPDP();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetPDP: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_AUTHENTICATION:
            {
                result = GetAuthentication();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetAuthentication: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_APN_THROTTLE_STATUS:
            {
                result = GetAPNThrottleStatus();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetAPNThrottleStatus: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_APN_THROTTLE_PLMN:
            {
                result = GetAPNThrottledPLMN();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetAPNThrottledPLMN: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_MTU:
            {
                result = GetMtu();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetMtu: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_SET_DEFAULT:
            {
                result = SetDefaultProfileIndex();
                logStr.clear();
                logStr = logStr + "taf_dcs_SetDefaultProfileIndexEx: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_DEFAULT:
            {
                result = GetDefaultProfileIndex();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetDefaultProfileIndexEx: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_START:
            {
                result = StartSession();
                logStr.clear();
                logStr = logStr + "taf_dcs_StartSession: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_START_ASYNC:
            {
                result = StartSessionAsync();
                logStr.clear();
                logStr = logStr + "taf_dcs_StartSessionAsync: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_STOP:
            {
                result = StopSession();
                logStr.clear();
                logStr = logStr + "taf_dcs_StopSession: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_STOP_ASYNC:
            {
                result = StopSessionAsync();
                logStr.clear();
                logStr = logStr + "taf_dcs_StopSessionAsync: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_STATE:
            {
                result = GetSessionState();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetSessionState: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV4_ADDRESS:
            {
                result = GetIPv4Address();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv4Address: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV6_ADDRESS:
            {
                result = GetIPv6Address();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv6Address: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV4_DNS:
            {
                result = GetIPv4DNSAddresses();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv4DNSAddresses: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV6_DNS:
            {
                result = GetIPv6DNSAddresses();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv6DNSAddresses: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV4_GATEWAY:
            {
                result = GetIPv4GatewayAddress();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv4GatewayAddress: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV6_GATEWAY:
            {
                result = GetIPv6GatewayAddress();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv6GatewayAddress: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV4_SUBNET_MASK:
            {
                result = GetIPv4SubnetMask();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv4SubnetMask: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_IPV6_SUBNET_MASK:
            {
                result = GetIPv6SubnetMask();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetIPv6SubnetMask: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_IS_IPV4:
            {
                result = IsIPv4();
                logStr.clear();
                logStr = logStr + "taf_dcs_IsIPv4: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_IS_IPV6:
            {
                result = IsIPv6();
                logStr.clear();
                logStr = logStr + "taf_dcs_IsIPv6: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case PROFILE_GET_PHONE_ID:
            {
                result = GetPhoneId();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetPhoneId: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_INTERFACE_NAME:
            {
                result = GetInterfaceName();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetInterfaceName: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_PH_ID_BY_INTF_NAME:
            {
                result = GetPhoneIdByInterfaceName();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetPhoneIdByInterfaceName: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            case SESSION_GET_PROF_ID_BY_INTF_NAME:
            {
                result = GetProfileIdByInterfaceName();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetProfileIdByInterfaceName: " +
                         std::to_string(result) + "(" + LE_RESULT_TXT(result) + ")";
                LE_TEST_INFO("%s", logStr.c_str());
                std::cout << logStr << std::endl;
                break;
            }
            default:
            {
                std::cerr << "You entered an invalid option." << std::endl;
                LE_TEST_INFO("Invalid test command %d", option);
                break;
            }
        }
    }

    // Clean up and exit
    UnRegister_Callbacks();
}
