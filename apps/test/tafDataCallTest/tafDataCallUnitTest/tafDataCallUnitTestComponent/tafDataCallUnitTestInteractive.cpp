/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   tafDataCallUnitTestInteractive.cpp
 * @brief  This file allows TelAF data call service APIs to be executed interactively and tested.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDcsHelper.hpp"
#include <string>
#include <future>
#include <iostream>
#include <map>

using namespace telux::tafsvc;

static taf_dcs_RoamingStatusHandlerRef_t                                g_roamingStatusHandlerRef;
static std::map<uint32_t, taf_dcs_SessionStateHandlerRef_t>  g_Profile_SessionStateHandlerRef_Map;

// Callback thread reference
le_thread_Ref_t callbackThreadRef = nullptr;

static void ShowMenu()
{
    std::cout << std::endl
              << "Select an option:" << std::endl
              << "0 -> Exit  " << std::endl
              << "1 -> Profile: Create Profile  " << std::endl
              << "2 -> Profile: Delete Profile  " << std::endl
              << "3 -> Session: Get data bearer technology" << std::endl
              << "4 -> Session: Get roaming status" << std::endl
              << "5 -> Session: Get max data bit rates" << std::endl
              << std::endl;
}

static taf_dcs_ProfileRef_t GetProfileRef()
{
    taf_dcs_ProfileRef_t ProfileRef;
    int profileId = 1;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin >> phoneID;

    std::cout << "Enter profile id:  ";
    std::cin >> profileId;

    LE_TEST_INFO("Phone ID: %d, Profile ID: %d", phoneID, profileId);

    ProfileRef= taf_dcs_GetProfileEx(static_cast<uint8_t>(phoneID), static_cast<uint32_t>(phoneID));
    LE_TEST_OK(nullptr != ProfileRef, "taf_dcs_GetProfileEx");

    return ProfileRef;
}

static le_result_t CreateProfile()
{
    LE_TEST_INFO("Create profile");
    return LE_OK;
}

static le_result_t DeleteProfile()
{
    LE_TEST_INFO("Delete profile");
    return LE_OK;
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
        LE_TEST_INFO("Failed to get data bearer technology: %d", result);
        std::cout << "Failed to get data bearer technology: " << result << std::endl;
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

static const char *RoamingTypeToString(taf_dcs_RoamingType_t roamingType)
{
    switch (roamingType)
    {
    case TAF_DCS_ROAMING_UNKNOWN:
        return "TAF_DCS_ROAMING_UNKNOWN";
    case TAF_DCS_ROAMING_DOMESTIC:
        return "TAF_DCS_ROAMING_DOMESTIC";
    case TAF_DCS_ROAMING_INTERNATIONAL:
        return "TAF_DCS_ROAMING_INTERNATIONAL";
    default:
        return "TAF_DCS_ROAMING_TYPE_UNKNOWN";
    }
}

static le_result_t GetRoamingStatus()
{
    LE_TEST_INFO("Get roaming status");
    le_result_t result = LE_OK;
    bool isRoaming;
    taf_dcs_RoamingType_t roamingType;
    int phoneID = 1;

    std::cout << "Enter phone id:  ";
    std::cin >> phoneID;

    result = taf_dcs_GetRoamingStatus(phoneID, &isRoaming, &roamingType);
    if (LE_OK!=result)
    {
        LE_ERROR("Failed to get roaming status");
        return result;
    }
    LE_TEST_INFO("Phone id: %d", phoneID);
    LE_TEST_INFO("Is roaming: %s", isRoaming ? "true" : "false");
    LE_TEST_INFO("Roaming type: %s", RoamingTypeToString(roamingType));
    std::cout << "Phone id: " << phoneID << std::endl;
    std::cout << "Is roaming: " << (isRoaming ? "true" : "false") << std::endl;
    std::cout << "Roaming type: " << RoamingTypeToString(roamingType) << std::endl;
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

static void *callback_thread_handler(void *ctxPtr)
{
    taf_dcs_ConnectService();
    le_sem_Post((le_sem_Ref_t)ctxPtr);
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    uint8_t phoneId = TAF_TYPES_PHONE_ID_1;
    le_result_t result;

    // Add roaming status handler
    g_roamingStatusHandlerRef = taf_dcs_AddRoamingStatusHandler(RoamingStatusHandlerFunc, NULL);

    // Add session state handler for all existing profiles for PHONE_ID_1
    result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
    LE_TEST_ASSERT(result == LE_OK, "taf_dcs_GetProfileListEx for phone id(%d): %d",
                phoneId, result);

    for (size_t i = 0; i < listSize; i++)
    {
        taf_dcs_SessionStateHandlerRef_t handlerRef = nullptr;
        taf_dcs_ProfileRef_t profileRef = nullptr;
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        profileRef = taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);
        LE_TEST_ASSERT(nullptr != profileRef, "taf_dcs_GetProfileEx: phone id(%d), \
                                                profile id: %d",
                       phoneId, profileInfoPtr->index);
        handlerRef = taf_dcs_AddSessionStateHandler(profileRef, SessionStateHandlerFunc, NULL);
        LE_TEST_ASSERT(nullptr != handlerRef, "taf_dcs_AddSessionStateHandler: phone id(%d), \
                                                profile id: %d", phoneId, profileInfoPtr->index);

        // Add the handler ref to the profile and handler map
        g_Profile_SessionStateHandlerRef_Map[profileInfoPtr->index] = handlerRef;
        // Set the references to nullptr
        profileRef = nullptr;
        handlerRef = nullptr;
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
    asyncCmdSemRef = le_sem_Create("asyncCmdSem", 0);

    asyncCmdThreadRef = le_thread_Create("async_cmd_thread", async_cmd_thread_handler,
                                         asyncCmdSemRef);
    le_thread_Start(asyncCmdThreadRef);
    le_sem_Wait(asyncCmdSemRef);
    le_sem_Delete(asyncCmdSemRef);

    LE_TEST_INIT;
    Register_Callbacks();
    while (bRun)
    {
        ShowMenu();
        std::cout << "Enter the option for the test" << std::endl;
        std::cin >> option;
        switch (option)
        {
            case 0:
            {
                // Stop the test
                bRun = false;
                break;
            }
            case 1 :
            {
                result = CreateProfile();
                LE_TEST_OK(LE_OK == result, "Create Profile: %d", result);
                break;
            }
            case 2 :
            {
                result = DeleteProfile();
                LE_TEST_OK(LE_OK == result, "Delete Profile: %d", result);
                break;
            }
            case 3:
            {
                result = GetDataBearerTechnology();
                LE_TEST_OK(LE_OK == result, "Get data bearer technology: %d", result);
                break;
            }
            case 4:
            {
                result = GetRoamingStatus();
                LE_TEST_OK(LE_OK == result, "Get roaming status: %d", result);
                break;
            }
            case 5:
            {
                result = GetMaxDataBitRates();
                LE_TEST_OK(LE_OK == result, "Get max data bit rates: %d", result);
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
    LE_TEST_EXIT;
}