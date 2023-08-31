/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * Return Connection information
*/
static int getConnectionInfo(taf_mngd_Conn_DataRef_t dataRef)
{
    le_result_t result;
    uint8_t dataID;
    taf_mngd_Conn_DataState_t state;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN] = {0};

    result = taf_mngd_Conn_DataGetConnectionState(dataRef, &dataID, &state);
    if (LE_OK != result){
        LE_WARN("DataGetConnectionState failed: %d", result);
        return result;
    }
    LE_INFO("Data ID  = %d ", dataID);
    if (TAF_MNGD_CONN_DATA_DISCONNECTED == state)
    {
        LE_INFO("Data Disconnected");
        return LE_OK;
    }
    else
    {
        LE_INFO("Data Connected");
    }

    // Data is connected. Get IP addresses
    result = taf_mngd_Conn_DataGetConnectionIPAddresses(dataRef,
                                                        ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                        ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    if (LE_OK == result)
    {
        LE_INFO("State    = %d ", state);
        if (ipv4Addr[0] != '\0')
            LE_INFO("IPv4Addr = %s", ipv4Addr);
        if (ipv6Addr[0] != '\0')
            LE_INFO("IPv6Addr = %s", ipv6Addr);
    }
    else{
        LE_WARN("DataGetConnectionIPAddresses failed: %d", result);
    }
    return result;
}

/**
 *  State returned by Managed Connectivity Service
 **/
static void ConnectionStateHandler(taf_mngd_Conn_DataRef_t dataRef,
                                   taf_mngd_Conn_DataState_t dataState,
                                   void *contextPtr)
{
    // Handle event
    getConnectionInfo(dataRef);
    return;
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

    // Create tafMngdConn Data references for the Data IDs.
    // Once the references are created, register for data session notifications.
    taf_mngd_Conn_DataRef_t tmpRef = NULL;
    std::map<int, taf_mngd_Conn_DataRef_t> dataID_dataRef;
    for (auto &id : dataID_AutoStart)
    {
        tmpRef = NULL;
        tmpRef = taf_mngd_Conn_GetData(id.first);
        if (NULL == tmpRef) {
            LE_WARN ("Error in getting data reference for Data ID: %d", id.first);
            return;
        }
        taf_mngd_Conn_AddDataStateHandler(tmpRef, ConnectionStateHandler, NULL);
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
            leResult = taf_mngd_Conn_DataStart(tmpRef);
            if ( LE_OK == leResult)
            {
                LE_INFO ("Data Session Started");
            }
            else if (LE_DUPLICATE == leResult)
            {
                LE_INFO("Data session already active for data ID: %d", id.first);
                getConnectionInfo(tmpRef);
            }
            else
            {
                LE_WARN ("Unable to start data for data ID %d, error: %d", id.first, leResult);
            }
        }
        else
        {
            // AutoStart: Yes, start the data session
            LE_INFO("Data ID: %d, AutoStart: Yes", id.first);
            LE_INFO("Get Connection information for ID %d", id.first);
            getConnectionInfo(tmpRef);
        }
    }
}