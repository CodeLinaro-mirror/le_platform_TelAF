/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafCM.cpp
 * @brief      TelAF Connection Manager source file
 */

#include <unistd.h>
#include "legato.h"
#include "interfaces.h"
#include <string>
#include <fstream>
#include <map>
#include <stdexcept>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/filesystem.hpp>

using std::ifstream;
using std::string;

// Boost property Tree need to parse JSON
namespace pt = boost::property_tree;

/**
 * Connectivity recovery events returned by Managed Connectivity Service
 **/
static void RecoveryEventHandler(taf_mngdConn_DataRef_t dataRef,
                                 taf_mngdConn_RecoveryEvent_t recoveryEvent,
                                 taf_mngdConn_RecoveryOperation_t operation,
                                 void *contextPtr)
{
    uint8_t dataId=0;
    le_result_t result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_ERROR ("Failed to get Data ID");
    }
    LE_INFO ("Event recevied for Data Id: %d", dataId);
    switch (recoveryEvent)
    {
    case TAF_MNGDCONN_RECOVERY_SCHEDULED:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_SCHEDULED");
    case TAF_MNGDCONN_RECOVERY_STARTED:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_STARTED");
    case TAF_MNGDCONN_RECOVERY_CANCELED:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_CANCELED");
    case TAF_MNGDCONN_RECOVERY_FAILED:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_FAILED");
    default:
        LE_INFO("unknown recovery event: %d", static_cast<int>(recoveryEvent));
    };
    switch (operation)
    {
    case TAF_MNGDCONN_RECOVERY_NONE:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_NONE");
    case TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON");
    case TAF_MNGDCONN_RECOVERY_SIM_OFF_ON:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_SIM_OFF_ON");
    case TAF_MNGDCONN_RECOVERY_FAILED:
        LE_INFO ("TAF_MNGDCONN_RECOVERY_NAD_REBOOT");
    default:
        LE_INFO ("unknown recovery event: %d", static_cast<int>(recoveryEvent));
    };

    LE_UNUSED(contextPtr);
    return;
}

/**
 * Return Connection information
*/
static int getConnectionInfo(taf_mngdConn_DataRef_t dataRef)
{
    le_result_t result;
    uint8_t profileID;
    uint8_t phoneID;
    taf_mngdConn_DataState_t state;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};

    result = taf_mngdConn_GetDataConnectionState(dataRef, &state);
    if (LE_OK != result){
        LE_WARN("DataGetConnectionState failed: %d", result);
        return result;
    }

    // Data is connected. Get IP addresses
    result = taf_mngdConn_GetDataConnectionIPAddresses(dataRef,
                                                       ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                       ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    if (LE_OK == result)
    {
        if (ipv4Addr[0] != '\0')
            LE_INFO("IPv4Addr = %s", ipv4Addr);
        if (ipv6Addr[0] != '\0')
            LE_INFO("IPv6Addr = %s", ipv6Addr);
    }
    else{
        LE_WARN("DataGetConnectionIPAddresses failed: %d", result);
    }

    // Get profile ID
    result = taf_mngdConn_GetProfileNumberByRef(dataRef, &profileID);
    if (LE_OK == result)
    {
        LE_INFO("Data profile = %d", profileID);
    }
    // Get profile ID
    result = taf_mngdConn_GetPhoneIdByRef(dataRef, &phoneID);
    if (LE_OK == result)
    {
        LE_INFO("Phone ID = %d", phoneID);
    }
    // Get profile reference
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(phoneID, profileID);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference for profile id: %d", profileID);
        return LE_FAULT;
    }

    size_t size = TAF_DCS_IPV4_ADDR_MAX_LEN;
    char IPv4[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    char IPv4_2[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};

    result = taf_dcs_GetIPv4Address(profileRef, IPv4, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv4 Address: %s", IPv4);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv4Address failed: %d", result);
    }
    memset(IPv4, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    result = taf_dcs_GetIPv4DNSAddresses(profileRef, IPv4, size, IPv4_2, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv4 DNS1 Address: %s", IPv4);
        LE_INFO("IPv4 DNS2 Address: %s", IPv4_2);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv4DNSAddresses failed: %d", result);
    }
    memset(IPv4, 0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    result = taf_dcs_GetIPv4GatewayAddress(profileRef, IPv4, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv4 Gateway Address: %s", IPv4);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv4GatewayAddress failed: %d", result);
    }

    taf_dcs_Pdp_t pdpType = taf_dcs_GetPDP(profileRef);
    if (TAF_DCS_PDP_IPV6 == pdpType || TAF_DCS_PDP_IPV4V6 == pdpType)
    {
        LE_INFO("Data profile supports IPv6");
    }
    else
    {
        LE_INFO("Data profile does not support IPv6");
        return LE_OK;
    }

    size = TAF_DCS_IPV6_ADDR_MAX_LEN;
    char IPv6[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    char IPv6_2[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};
    memset(IPv6, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    result = taf_dcs_GetIPv6Address(profileRef, IPv6, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv6 Address: %s", IPv6);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv6Address failed: %d", result);
    }

    memset(IPv6, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    result = taf_dcs_GetIPv6DNSAddresses(profileRef, IPv6, size, IPv6_2, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv6 DNS1 Address: %s", IPv6);
        LE_INFO("IPv6 DNS2 Address: %s", IPv6_2);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv6DNSAddresses failed: %d", result);
    }

    memset(IPv6, 0, TAF_DCS_IPV6_ADDR_MAX_LEN);
    result = taf_dcs_GetIPv6GatewayAddress(profileRef, IPv6, size);
    if (LE_OK == result)
    {
        LE_INFO("IPv6 Gateway Address: %s", IPv6);
    }
    else
    {
        LE_WARN("taf_dcs_GetIPv6GatewayAddress failed: %d", result);
    }

    /*
    le_result_t (taf_dcs_ProfileRef_t, char *, size_t)

le_result_t (taf_dcs_ProfileRef_t, char *, size_t, char *, size_t)

le_result_t (taf_dcs_ProfileRef_t, char *, size_t)
    */

    return result;
}

/**
 * Data state returned by Managed Connectivity Service
 **/
static void DataStateHandler(taf_mngdConn_DataRef_t dataRef,
                             taf_mngdConn_DataState_t dataState,
                             void *contextPtr)
{
    uint8_t dataId = 0;
    le_result_t result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to get Data ID");
    }
    LE_INFO("Event recevied for Data Id: %d", dataId);
    // Handle event
    if (TAF_MNGDCONN_DATA_CONNECTED == dataState)
    {
        LE_INFO("Data Connected");
        getConnectionInfo(dataRef);
    }
    else if (TAF_MNGDCONN_DATA_DISCONNECTED == dataState)
    {
        LE_INFO("Data Disconnected");
    }
    else if (TAF_MNGDCONN_DATA_CONNECTION_FAILED == dataState)
    {
        LE_INFO("Data Connection failed");
    }
    else if (TAF_MNGDCONN_DATA_CONNECTION_STALLED == dataState)
    {
        LE_INFO("Data Connection stalled");
    }
    else
    {
        LE_INFO("UNsupported data state: %d", dataState);
    }
}

/**
 * Map Data IDs to AutoStart values
*/
static bool Get_DataID_AutoStart_Mapping(const std::string &FileName,
                                         std::map<int, std::string> &DataID_AutoStart)
{
    std::ifstream jsonFile(FileName);
    if (!jsonFile.is_open())
    {
        LE_WARN("Unable to open %s", FileName.c_str());
        return false;
    }
    // Try parsing the JSON
    pt::ptree tree;
    try
    {
        read_json(jsonFile, tree);
    }
    catch (const std::exception &e)
    {
        LE_WARN("read_json exception: %s. Check validity of JSON.", e.what());
        return false;
    }

    for (auto &element : tree)
    {
        if ("ManagedConnectivityService" == element.first ) {

            for (auto & property: element.second) {

                if ("Configuration" == property.first){
                    for (auto & parent: property.second) {
                        if ("Data" == parent.first)
                        {
                            // Iterate through the Array elements
                            for (auto &array_element: parent.second)
                            {
                                int id = 0;
                                std::string autostart;
                                for (auto &iter : array_element.second)
                                {
                                    if ("ID" == iter.first)
                                    {
                                        id = std::stoi(iter.second.data());
                                    }
                                    if ("AutoStart" == iter.first)
                                    {
                                        autostart = iter.second.data();
                                    }
                                }
                                // Update the map
                                DataID_AutoStart.emplace(id, autostart);

                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

/**
 * Application init
*/
COMPONENT_INIT
{
    LE_INFO("TelAF-CM Init");
    // Policy and Configuration File Name
    const string configurationFileName =  "/data/ManagedServices/mngdConnectivity.json";

    // Parse Configuration JSON and get the Data IDs and matching AutoStart values
    std::map<int, std::string> dataID_AutoStart;
    if (!Get_DataID_AutoStart_Mapping(configurationFileName, dataID_AutoStart))
    {
        LE_WARN("Error in mapping Data ID and AutoStart");
        return;
    }

    // Register recovery state handler
    taf_mngdConn_AddRecoveryEventHandler(RecoveryEventHandler, NULL);

    // Create tafMngdConn Data references for the Data IDs.
    // Once the references are created, register for data session notifications.
    taf_mngdConn_DataRef_t tmpRef = NULL;
    std::map<int, taf_mngdConn_DataRef_t> dataID_dataRef;
    for (auto &id : dataID_AutoStart)
    {
        tmpRef = NULL;
        tmpRef = taf_mngdConn_GetData(id.first);
        if (NULL == tmpRef) {
            LE_WARN ("Error in getting data reference for Data ID: %d", id.first);
            return;
        }
        taf_mngdConn_AddDataStateHandler(tmpRef, DataStateHandler, NULL);
        dataID_dataRef.emplace(id.first, tmpRef);
    }

    // Parse through the map and start the Data IDs that are set to manual start
    le_result_t leResult = LE_OK;
    for (auto &id : dataID_AutoStart)
    {
        LE_DEBUG("Data ID: %d, AutoStart: %s", id.first, id.second.c_str());
        tmpRef = NULL;
        // Get the reference for the Data ID
        tmpRef = dataID_dataRef.at(id.first);
        if ("No" == id.second)
        {
            // AutoStart: No, start the data session
            LE_INFO("Data ID: %d, AutoStart: No", id.first);
            LE_INFO("Start Data for ID %d", id.first);
            leResult = taf_mngdConn_StartData(tmpRef);
            if ( LE_OK == leResult)
            {
                LE_INFO ("Data Session Started");
            }
            else if (LE_DUPLICATE == leResult)
            {
                LE_INFO("Data session already active for data ID: %d", id.first);
                getConnectionInfo(tmpRef);
            }
            else if (LE_IN_PROGRESS == leResult)
            {
                LE_INFO("Data session start is in progress for data ID: %d", id.first);
                getConnectionInfo(tmpRef);
            }
            else
            {
                LE_WARN ("Unable to start data for data ID %d, error: %d", id.first, leResult);
            }
        }
        else
        {
            // AutoStart: Yes
            // The service will manage the data session. Just get the connection information.
            LE_INFO("Data ID: %d, AutoStart: Yes", id.first);
            LE_INFO("Get Connection information for ID %d", id.first);
            getConnectionInfo(tmpRef);
        }
    }
}