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

using namespace tafsvc;

static taf_dcs_RoamingStatusHandlerRef_t                                g_roamingStatusHandlerRef;
static std::map<uint32_t, taf_dcs_SessionStateHandlerRef_t>  g_Profile_SessionStateHandlerRef_Map;
static std::map<uint32_t, taf_dcs_QosStatusHandlerRef_t>    g_Profile_QosStatusHandlerRef_Map;
static std::map<uint32_t, taf_dcs_HwAccelerationStateHandlerRef_t> g_Profile_HwAccelHandlerRef_Map;

// Callback thread reference
le_thread_Ref_t callbackThreadRef = nullptr;

typedef enum
{
    PROFILE_GET_LIST = 1,
    PROFILE_CREATE,                 //
    PROFILE_DELETE,                 //
    PROFILE_SET_APN,                //
    PROFILE_SET_NAME,               //
    PROFILE_SET_TECH_PREF,          //
    PROFILE_SET_APN_TYPE_MASK,      //
    PROFILE_SET_PDP,                //
    PROFILE_SET_AUTHENTICATION,     //
    PROFILE_SET_DEFAULT,            //
    PROFILE_GET_ID,                 //
    PROFILE_GET_APN,                //
    PROFILE_GET_NAME,               //
    PROFILE_GET_TECH_PREF,          //
    PROFILE_GET_APN_TYPE_MASK,      //
    PROFILE_GET_PDP,                //
    PROFILE_GET_AUTHENTICATION,     //
    PROFILE_GET_DEFAULT,            //
    SESSION_GET_DATA_BEARER_TECH,   //
    SESSION_GET_ROAMING_STATUS,     //
    SESSION_GET_MAX_DATA_BIT_RATES, //
    SESSION_CALL_END_REASON,        //
    APN_GET_THROTTLE_INFO,          //
    PROFILE_GET_MTU
} dcsAPIs;

static void ShowMenu()
{
    std::cout << std::endl
              << "Select an option:" << std::endl
              << "0 -> Exit  " << std::endl
              << PROFILE_GET_LIST               << " -> Profile: Get list"
              << std::endl
              << PROFILE_CREATE                 << " -> Profile: Create Profile"
              << std::endl
              << PROFILE_DELETE                 << " -> Profile: Delete Profile"
              << std::endl
              << PROFILE_SET_APN                << " -> Profile: Set APN"
              << std::endl
              << PROFILE_SET_NAME               << " -> Profile: Set name"
              << std::endl
              << PROFILE_SET_TECH_PREF          << " -> Profile: Set technology preference"
              << std::endl
              << PROFILE_SET_APN_TYPE_MASK      << " -> Profile: Set APN type mask"
              << std::endl
              << PROFILE_SET_PDP                << " -> Profile: Set PDP(IP family type)"
              << std::endl
              << PROFILE_SET_AUTHENTICATION     << " -> Profile: Set authentication"
              << std::endl
              << PROFILE_SET_DEFAULT            << " -> Profile: Set default"
              << std::endl
              << PROFILE_GET_ID                 << " -> Profile: Get Id"
              << std::endl
              << PROFILE_GET_APN                << " -> Profile: Get APN"
              << std::endl
              << PROFILE_GET_NAME               << " -> Profile: Get name"
              << std::endl
              << PROFILE_GET_TECH_PREF          << " -> Profile: Get tech preference"
              << std::endl
              << PROFILE_GET_APN_TYPE_MASK      << " -> Profile: Get APN type mask"
              << std::endl
              << PROFILE_GET_PDP                << " -> Profile: Get PDP(IP family type)"
              << std::endl
              << PROFILE_GET_AUTHENTICATION     << " -> Profile: Get authentication"
              << std::endl
              << PROFILE_GET_DEFAULT            << " -> Profile: Get default"
              << std::endl
              << SESSION_GET_DATA_BEARER_TECH   << " -> Session: Get data bearer technology"
              << std::endl
              << SESSION_GET_ROAMING_STATUS     << " -> Session: Get roaming status"
              << std::endl
              << SESSION_GET_MAX_DATA_BIT_RATES << " -> Session: Get max data bit rates"
              << std::endl
              << SESSION_CALL_END_REASON        << " -> Session: Get call end reason"
              << std::endl
              << APN_GET_THROTTLE_INFO          << " -> Session: Get apn throttle status"
              << std::endl
              << PROFILE_GET_MTU                << " -> Profile: Get MTU"
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

    std::cout << "Enter profile id:  ";
    std::cin.clear();
    std::cin >> profileId;

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

    LE_TEST_INFO("Tech Pref: %d(%s)", techPref, taf_DCSHelper::TechPreferenceToString(techPref));
    std::cout << "Tech Pref: " << techPref << "("
                        << taf_DCSHelper::TechPreferenceToString(techPref) << ")" << std::endl;
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
                        taf_DCSHelper::ApnTypeMaskToString(apnTypeMask).c_str());
    std::cout << "APN types mask: " << apnTypeMask << "("
                     << taf_DCSHelper::ApnTypeMaskToString(apnTypeMask) << ")" << std::endl;
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
                 taf_DCSHelper::IpFamilyTypeToString(pdp));
    std::cout << "PDP(IP family): " << pdp << "("
              << taf_DCSHelper::IpFamilyTypeToString(pdp) << ")" << std::endl;
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

    LE_TEST_INFO("Auth Type : %s", taf_DCSHelper::AuthMaskToString(auth).c_str());
    std::cout << "Auth Type : " << taf_DCSHelper::AuthMaskToString(auth) << std::endl;
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

static le_result_t GetProfileListEx()
{
    LE_TEST_INFO("Get profile list");
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;
    int phoneID = 1;
    taf_dcs_ProfileRef_t profileRef = NULL;
    std::string logStr;
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    taf_dcs_Pdp_t pdp;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;

    result = taf_dcs_GetProfileListEx(static_cast<uint8_t>(phoneID), profilesInfoPtr, &listSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "taf_dcs_GetProfileListEx failed");

    logStr.clear();
    logStr = logStr + "Index \t APN \t\t PDP";
    LE_TEST_INFO ("%s", logStr.c_str());
    std::cout << logStr << std::endl;
    for (uint32_t i = 0; i < listSize; i++)
    {
        profileRef = NULL;
        logStr.clear();
        memset(apnStr, 0, TAF_DCS_APN_NAME_MAX_LEN);
        pdp = TAF_DCS_PDP_UNKNOWN;

        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        profileRef = taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID), profileInfoPtr->index);
        TAF_ERROR_IF_RET_VAL(NULL == profileRef, LE_FAULT, "taf_dcs_GetProfileEx failed");
        result = taf_dcs_GetAPN(profileRef, apnStr, TAF_DCS_APN_NAME_MAX_LEN);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "taf_dcs_GetAPN failed");
        pdp = taf_dcs_GetPDP(profileRef);

        logStr = logStr + std::to_string(profileInfoPtr->index) + "\t" + apnStr
                                        + "\t\t" + taf_DCSHelper::IpFamilyTypeToString(pdp);
        LE_TEST_INFO ("%s", logStr.c_str());
        std::cout << logStr << std::endl;
    }
    return LE_OK;
}

static le_result_t SetTechPreference(taf_dcs_ProfileRef_t ProfileRef)
{
    int intInput = 1;
    le_result_t result = LE_OK;
    std::cout << "Tech preference: " << std::endl;
    std::cout << 0 << "-" << "skip setting tech preference" << std::endl;
    std::cout << TAF_DCS_TECH_3GPP << "-"
                        << taf_DCSHelper::TechPreferenceToString(TAF_DCS_TECH_3GPP)  << std::endl;
    std::cout << TAF_DCS_TECH_3GPP2 << "-"
                        << taf_DCSHelper::TechPreferenceToString(TAF_DCS_TECH_3GPP2) << std::endl;
    std::cout << TAF_DCS_TECH_ANY << "-"
                        << taf_DCSHelper::TechPreferenceToString(TAF_DCS_TECH_ANY)   << std::endl;
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
                    taf_DCSHelper::TechPreferenceToString(static_cast<taf_dcs_Tech_t>(intInput)));
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
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_DEFAULT) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_IMS << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_IMS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_MMS << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_MMS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_DUN << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_DUN) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_SUPL << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_SUPL) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_HIPRI << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_HIPRI) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_FOTA << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_FOTA) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_CBS << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_CBS) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_IA << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_IA) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_EMERGENCY << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_EMERGENCY) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_UT << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_UT) << std::endl;
    std::cout << TAF_DCS_APN_TYPE_MCX << "-"
              << taf_DCSHelper::ApnTypeMaskToString(TAF_DCS_APN_TYPE_MCX) << std::endl;
    std::cout << "Enter APN type mask(OR the types needed. e.g DEFAULT|IMS=3): " << std::endl;
    std::cin.clear();
    std::cin >> intInput;

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
                         taf_DCSHelper::ApnTypeMaskToString(apnTypeMask).c_str());
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
              << taf_DCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV4) << std::endl;
    std::cout << TAF_DCS_PDP_IPV6 << "-"
              << taf_DCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV6) << std::endl;
    std::cout << TAF_DCS_PDP_IPV4V6 << "-"
              << taf_DCSHelper::IpFamilyTypeToString(TAF_DCS_PDP_IPV4V6) << std::endl;
    std::cout << "Enter PDP: " << std::endl;
    std::cin.clear();
    std::cin >> intInput;

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
                        taf_DCSHelper::IpFamilyTypeToString(static_cast<taf_dcs_Pdp_t>(intInput)));
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
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
              << taf_DCSHelper::AuthMaskToString(TAF_DCS_AUTH_NONE) << std::endl;
    std::cout << TAF_DCS_AUTH_PAP << "-"
              << taf_DCSHelper::AuthMaskToString(TAF_DCS_AUTH_PAP) << std::endl;
    std::cout << TAF_DCS_AUTH_CHAP << "-"
              << taf_DCSHelper::AuthMaskToString(TAF_DCS_AUTH_CHAP) << std::endl;

    std::cout << "Enter auth type mask(OR the types needed. e.g PAP|CHAP=6): " << std::endl;
    std::cin.clear();
    std::cin >> intInput;

    if (0 == intInput)
    {
        LE_TEST_INFO("Skip setting authenticaton");
        return LE_OK;
    }

    std::cout << "Enter username(max " << TAF_DCS_USER_NAME_MAX_LEN << " characters):  ";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.getline(unStr, (TAF_DCS_USER_NAME_MAX_LEN));
    std::cout << "Enter password(max " << TAF_DCS_PASSWORD_NAME_MAX_LEN << " characters):  ";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
                     taf_DCSHelper::AuthMaskToString(auth).c_str(), unStr, pwStr);
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
                                            taf_DCSHelper::DataBearerTechnologyToString(upTech));
    LE_TEST_INFO("Data bearer technology: downlink=%s",
                                            taf_DCSHelper::DataBearerTechnologyToString(downTech));
    std::cout << "Data bearer tech: uplink   = " <<
                                            taf_DCSHelper::DataBearerTechnologyToString(upTech) <<
                                            std::endl;
    std::cout << "Data bearer tech: downlink = " <<
                                            taf_DCSHelper::DataBearerTechnologyToString(downTech) <<
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

    result = taf_dcs_GetRoamingStatus(phoneID, &isRoaming, &roamingType);
    if (LE_OK!=result)
    {
        LE_TEST_INFO("Failed to get roaming status");
        return result;
    }
    LE_TEST_INFO("Phone id: %d", phoneID);
    LE_TEST_INFO("Is roaming: %s", isRoaming ? "true" : "false");
    LE_TEST_INFO("Roaming type: %s", taf_DCSHelper::RoamingTypeToString(roamingType));
    std::cout << "Phone id: " << phoneID << std::endl;
    std::cout << "Is roaming: " << (isRoaming ? "true" : "false") << std::endl;
    std::cout << "Roaming type: " << taf_DCSHelper::RoamingTypeToString(roamingType) << std::endl;
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
    const char *CallEndReasonTypeStr4 = taf_DCSHelper::CallEndReasonTypeToString(callEndReasonType);
    const char *CallEndReasonCodeStr4 = taf_DCSHelper::CallEndReasonCodeToString(
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
    const char *CallEndReasonTypeStr6 = taf_DCSHelper::CallEndReasonTypeToString(callEndReasonType);
    const char *CallEndReasonCodeStr6 = taf_DCSHelper::CallEndReasonCodeToString(
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
    LE_TEST_INFO("Get apn throttle information");
    le_result_t result = LE_OK;

    bool  areAllPLMNsThrottled;
    char mccStr[TAF_DCS_MCC_BYTES];
    char mncStr[TAF_DCS_MNC_BYTES];

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
    if(result != LE_OK)
    {
      LE_TEST_INFO("Failed to get apn throttle status: %d", result);
      std::cout << "Failed to get apn throttle status: " << result << std::endl;
      if(result == LE_UNAVAILABLE)
       LE_TEST_INFO("Data profile is not throttled");
      else if(result == LE_NOT_POSSIBLE)
       LE_TEST_INFO("Data profile is not created");
      else
       LE_TEST_INFO("Some other error");
      return result;
    }

    LE_TEST_INFO("----isThrottled : %d", (bool)isThrottled);
    LE_TEST_INFO("----ipv4 Time : %d", (int)ipv4RemainingTime);
    LE_TEST_INFO("----ipv6 Time : %d", (int)ipv6RemainingTime);

    std::cout << "apn throttle status: " << isThrottled << ",ipv4: " << ipv4RemainingTime
                                                        << ",ipv6: " << ipv6RemainingTime
                                                                     << std::endl;

    result = taf_dcs_GetAPNThrottledPLMN(ProfileRef, &areAllPLMNsThrottled,
                                                       mccStr,TAF_DCS_MCC_BYTES,
                                                       mncStr,TAF_DCS_MNC_BYTES);

    if(result != LE_OK)
    {
      LE_TEST_INFO("Failed to get apn throttle plmn: %d", result);
      std::cout << "Failed to get apn throttle plmn: " << result << std::endl;
      if(result == LE_UNAVAILABLE)
       LE_TEST_INFO("Data profile is not throttled");
      else if(result == LE_NOT_POSSIBLE)
       LE_TEST_INFO("Data profile is not created");
      else
       LE_TEST_INFO("Some other error");
      return result;
    }

    LE_TEST_INFO("----areAllPLMNsThrottled : %d", (bool)areAllPLMNsThrottled);
    LE_TEST_INFO("----MCC : %s", mccStr);
    LE_TEST_INFO("----MNC : %s", mncStr);

    std::cout << "areAllPLMNsThrottled: " << areAllPLMNsThrottled << ",MCC: " << mccStr
                                                        << ",MNC: " << mncStr
                                                                     << std::endl;

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

static void HwAccStateHandlerFunc(  taf_dcs_ProfileRef_t profileRef,
                                    taf_dcs_HwAccelerationState_t state,
                                    void *contextPtr)
{
    LE_UNUSED(contextPtr);

    uint32_t profileId = 0;
    le_result_t result = taf_dcs_GetProfileId(profileRef, &profileId);
    LE_TEST_INFO("taf_dcs_GetProfileId result: %d", result);

    LE_TEST_INFO("Profile %d Hw accel state : %d(%s)", profileId, state,
                                                            (state ? "ACTIVE" : "INACTIVE"));
    std::cout   << "Profile " << profileId
                << " Hw accel state    : " << state << "(" << (state ? "ACTIVE" : "INACTIVE") << ")"
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
                 taf_DCSHelper::RoamingTypeToString(roamingStatusIndPtr->type));
    std::cout << "\tRoaming status callback" << std::endl;
    std::cout << "\t\tPhone Id     : " << roamingStatusIndPtr->phoneId << std::endl;
    std::cout << "\t\tIs Roaming   : " << roamingStatusIndPtr->isRoaming << std::endl;
    std::cout << "\t\tRoaming type : " <<
                    taf_DCSHelper::RoamingTypeToString(roamingStatusIndPtr->type) << std::endl;
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
                 taf_DCSHelper::CallEventToString(callEvent),
                 taf_DCSHelper::IpFamilyTypeToString(infoPtr->ipType));
    std::cout << "\tSessionStateHandlerFunc" << std::endl;
    std::cout << "\t\tProfile id: " << profileId << std::endl;
    std::cout << "\t\tCall event: " << taf_DCSHelper::CallEventToString(callEvent) << std::endl;
    std::cout << "\t\tIP type   : " << taf_DCSHelper::IpFamilyTypeToString(infoPtr->ipType)
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
    std::cout << "**** Handler for qos status Indication (Begin)****" << std::endl;

    LE_TEST_INFO("----QOS State : %d", (int)qosState);

    std::cout << "\t\tQOS State id: " << qosState << std::endl;

    uint32_t qosFlowId = 0;
    le_result_t result = taf_dcs_GetQosId(qosFlowRef,&qosFlowId);
    if(result == LE_OK)
    {
      LE_TEST_INFO("----Qos ID : %d", (int)qosFlowId);
      std::cout << "\t\tQos ID id: " << qosFlowId << std::endl;
    }
    else
    {
      LE_TEST_INFO("----qos ID get error---");
      std::cout << "\t\t---qos ID get error---" << std::endl;
      return;
    }

    taf_dcs_QosFlowBitMask_t mask = 0;
    result = taf_dcs_GetQosParameterMask(qosFlowRef,&mask);
    if(result == LE_OK)
    {
      LE_TEST_INFO("----Qos Mask : %d", (int)mask);
      std::cout << "\t\tQos Mask id: " << mask << std::endl;
    }
    else
    {
      LE_TEST_INFO("----qos mask get error---");
      std::cout << "\t\t---Qos Mask get error---" << std::endl;
    }

   if(mask & TAF_DCS_QOS_BIT_MASK_FLOW_NONE)
   {
       LE_TEST_INFO("No QOS flow mask installed");
       std::cout << "\t\tNo QOS flow mask installed" << std::endl;

       return ;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_TX_GRANTED)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_TX_GRANTED");
       std::cout << "\t\tQOS Mask == MASK_FLOW_TX_GRANTED" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_RX_GRANTED)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_RX_GRANTED");
       std::cout << "\t\tQOS Mask == MASK_FLOW_RX_GRANTED" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_TX_FILTERS)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_TX_FILTERS");
       std::cout << "\t\tQOS Mask == MASK_FLOW_TX_FILTERS" << std::endl;
   }
   if (mask & TAF_DCS_QOS_BIT_MASK_FLOW_RX_FILTERS)
   {
       LE_TEST_INFO("QOS Mask == MASK_FLOW_RX_FILTERS");
       std::cout << "\t\tQOS Mask == MASK_FLOW_RX_FILTERS" << std::endl;
   }

    LE_TEST_INFO("**** Handler for qos status Indication (End)****");
    std::cout << "****Handler for qos status Indication (End)****" << std::endl;
}

static le_result_t GetDefaultProfileIndex()
{
    uint32_t profileId = 1;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin.clear();
    std::cin >> phoneID;

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

    std::cout << "Enter profile id:  ";
    std::cin.clear();
    std::cin >> profileId;

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

        // Add the handler ref to the profile and session handler map
        g_Profile_SessionStateHandlerRef_Map[profileInfoPtr->index] = handlerRef;

        // Add the handler ref to the profile and qos handler map
        g_Profile_QosStatusHandlerRef_Map[profileInfoPtr->index] = handlerQosRef;

        // Add the handler ref to the profile and HW acceleration handler map
        g_Profile_HwAccelHandlerRef_Map[profileInfoPtr->index] = handlerHwAccelRef;

        // Set the references to nullptr
        profileRef = nullptr;
        handlerRef = nullptr;
        handlerQosRef = nullptr;
        handlerHwAccelRef = nullptr;
    }

    // Start the event loop
    le_event_RunLoop();
    return NULL;
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
        LE_TEST_INFO("Removed session hander for profile ID: %d", profileId);
        taf_dcs_RemoveSessionStateHandler(handlerRef);
    }

    for (const auto &pair : g_Profile_QosStatusHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_QosStatusHandlerRef_t handlerRef = pair.second;

        // Remove the qos state handler
        LE_TEST_INFO("Removed qos state hander for profile ID: %d", profileId);
        taf_dcs_RemoveQosStatusHandler(handlerRef);
    }

    for (const auto &pair : g_Profile_HwAccelHandlerRef_Map)
    {
        uint32_t profileId = pair.first;
        taf_dcs_HwAccelerationStateHandlerRef_t handlerRef = pair.second;

        // Remove the qos state handler
        LE_TEST_INFO("Removed HW accel state hander for profile ID: %d", profileId);
        taf_dcs_RemoveHwAccelerationStateHandler(handlerRef);
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
    int option = 0;
    le_result_t result = LE_OK;
    le_thread_Ref_t asyncCmdThreadRef = nullptr;
    le_sem_Ref_t asyncCmdSemRef = nullptr;
    std::string logStr;
    asyncCmdSemRef = le_sem_Create("asyncCmdSem", 0);

    asyncCmdThreadRef = le_thread_Create("async_cmd_thread", async_cmd_thread_handler,
                                         asyncCmdSemRef);
    le_thread_Start(asyncCmdThreadRef);
    le_sem_Wait(asyncCmdSemRef);
    le_sem_Delete(asyncCmdSemRef);

    Register_Callbacks();
    while (bRun)
    {
        ShowMenu();
        std::cout << "Enter the option for the test" << std::endl;
        std::cin.clear();
        std::cin >> option;
        switch (option)
        {
            case 0:
            case 'q':
            {
                // Stop the test
                bRun = false;
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
            case SESSION_CALL_END_REASON:
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
            case APN_GET_THROTTLE_INFO:
            {
                result = GetAPNThrottleStatus();
                logStr.clear();
                logStr = logStr + "taf_dcs_GetAPNThrottleStatus: " +
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
            default:
            {
                std::cerr << "You entered an invalid option";
                LE_TEST_INFO("Invalid test command %d", option);
                break;
            }
        }
    }

    // Clean up and exit
    UnRegister_Callbacks();
}