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

//-----------------------------------------------------------
#include <vector>
#include <boost/filesystem.hpp>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include "tafMngdConnSvcJSONParser.hpp"
#include "tafMngdConn_ConfigTreeHelper.hpp"

using namespace telux::tafsvc;
using std::string;
using std::to_string;

namespace pt = boost::property_tree;


// Map of properties and validation function pointers
std::map< std::string, ConnectivityValidationFunction_t >  ConnectivityValidationFuncMap;

/**
 * Validate ManagedConnectivityService:Version
 */
static bool Validate_MCS_Version(taf_mngd_Conn_Policy_t &Policy,
                                                taf_mngd_Conn_Configuration_t &Configuration,
                                                std::string Value,
                                                int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType && TAF_MNGD_JSON_DATA_TYPE_NULL != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy and Configuration.
    Policy.Version = std::stoi(Value);
    Configuration.Version = std::stoi(Value);
    return true;
}

static bool ValidateValue(taf_mngd_Conn_Policy_t& Policy,
                                taf_mngd_Conn_Configuration_t &Configuration,
                                std::string property,
                                std::string Value,
                                int Index)
{
    LE_DEBUG("Property: %s, Value: %s", property.c_str(), Value.c_str());
    auto iterator = ConnectivityValidationFuncMap.find(property);
    if (iterator != ConnectivityValidationFuncMap.end())
    {
        return (*iterator->second)(Policy, Configuration, Value, Index);
    }

    // The property is not found, so it's unsupported. Return false.
    LE_WARN("%s is not supported", property.c_str());
    return false;
}

/**
 * Retrun true if the filenames match
 * Return false if the filenames do not match
 */
static bool updateMangedConnectivityConfigTree(string ConfigurationFileName,
                                               bool   bDoConfigFileNamesMatch)
{
    le_result_t leRet = LE_OK;

    boost::filesystem::path fullFilePath(ConfigurationFileName);
    boost::filesystem::path dir = fullFilePath.parent_path();
    if (dir == TAF_MNGD_DefaultLocation_Configuration)
    {
        LE_INFO("Files are in default location. Write only the FileName");
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_ConfigurationFileName,
                                                            fullFilePath.filename().string());
    }
    else
    {
        // Write the full file path
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_ConfigurationFileName,
                                                                ConfigurationFileName);
    }

    if (bDoConfigFileNamesMatch)
    {
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_ConfigurationFileNameOverride, "Yes");
    }
    else
    {
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_ConfigurationFileNameOverride, "No");
    }

    if (LE_OK == leRet)
        return true;

    return false;
}



bool telux::tafsvc::tafMngdConnSvc_GetPolicyAndConfiguration(
    taf_mngd_Conn_Policy_t &PolicyStructRef,
    taf_mngd_Conn_Configuration_t &ConfigurationStructRef,
    std::string ConfigurationFileName)
{
    std::string newConfFileName;
    bool bDoConfigFileNamesMatch = true;
    tafMngdConnSvc_PolicyParser &PolicyParserRef = tafMngdConnSvc_PolicyParser::getInstance();
    tafMngdConnSvc_ConfigurationParser &ConfigurationParserRef =
                                                tafMngdConnSvc_ConfigurationParser::getInstance();

    newConfFileName = ConfigurationFileName;
    // Check if only Configuration file name is path or if path is also provided.
    // If path is not provided, add the default path
    if ('/' != newConfFileName[0])
    {
        newConfFileName.insert (0, (TAF_MNGD_DefaultLocation_Configuration + "/"));
    }
    // Update the properties and validation functions map
    UpdateValidConnectivityFuncMap();

    // Try opening an input file stream
    std::ifstream jsonFile(newConfFileName);
    if (!jsonFile.is_open()) {
        LE_WARN ("Unable to open %s", newConfFileName.c_str());
        return false;
    }

    // Try parsing the JSON
    pt::ptree tree;
    try {
        read_json(jsonFile, tree);
    }
    catch (const std::exception &e) {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return false;
    }

    std::string log, JSON_Property, JSON_Value;
    //Check for Product, Name, and Version before parsing
    for (auto & element: tree) {
        //Product
        if ("Product" == element.first ) {
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the elements within "Product"
            if (element.second.get_value < std::string > () != "TelAF")
            {
                LE_WARN("Invalid JSON_Property Value");
                return false;
            }
        }

          //Name
        if ("Name" == element.first ) {
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the elements within "Product"
            if (element.second.get_value < std::string > ()
             != "Managed Connectivity Configuration - V1")
            {
                LE_WARN("Invalid JSON_Property Value");
                return false;
            }
        }

        // ManagedConnectivityServicePolicy
        if ("ManagedConnectivityService" == element.first ) {
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the elements within "ManagedConnectivityServicePolicy"
            for (auto & property: element.second) {
                if ("Version" == property.first){
                    log.clear();
                    log.append ("Key: " + property.first + ", Value: " +
                                                    property.second.get_value < std::string > () );
                    LE_DEBUG("%s", log.c_str());
                    // Validate the read value
                    JSON_Property.clear();
                    JSON_Property.append(element.first + ":" + property.first);
                    JSON_Value.clear();
                    JSON_Value.append(property.second.get_value<std::string>());
                    // Validate values. Index is set to invald.
                    if (!ValidateValue(PolicyStructRef,
                                    ConfigurationStructRef,
                                    JSON_Property, JSON_Value,
                                    TAF_MNGD_CONN_INVALID_INDEX))
                    {
                        LE_WARN("Invalid JSON_Property Value");
                        LE_INFO("JSON_Property: %s, Value: %s", JSON_Property.c_str(),
                                                                            JSON_Value.c_str());
                        return false;
                    }
                }

                if("Policy" == property.first){
                    if ( PolicyParserRef.GetPolicy(PolicyStructRef, newConfFileName) )
                    {
                        LE_INFO("Policy Parsing Successful");
                    }
                    else
                    {
                        LE_ERROR("Policy Parsing Failed");
                        return false;
                    }
                }

                if("Configuration" == property.first){
                    if ( ConfigurationParserRef.GetConfiguration(ConfigurationStructRef,
                    newConfFileName) )
                    {
                        LE_INFO("Configuration Parsing Successful");
                    }
                    else
                    {
                        LE_ERROR("Configuration Parsing Failed");
                        return false;
                    }
                }
            }
        }
    }

    // Check if Policy DataSession->DataConnection->Use_Data_ID has a matching Data->ID in
    // Configuration
    std::vector<bool> bConfigDataIDFound(PolicyStructRef.DataSession.dataConnectionCount,false);

    // Iterate through all Policy Data Connection elements
    for ( uint8_t PolicyIdx=0;
          PolicyIdx < PolicyStructRef.DataSession.dataConnectionCount;
          PolicyIdx++)
    {
        // Iterate through all Configuration Data Connection elements
        for (uint8_t ConfigIdx = 0;ConfigIdx < ConfigurationStructRef.DataCount;ConfigIdx++)
        {
            // Match Policy Use_Data_ID with Configuration Data->ID
            if (ConfigurationStructRef.Data[ConfigIdx].ID ==
                    PolicyStructRef.DataSession.DataConnection[PolicyIdx].Use_Data_ID)
            {
                // Matching Configuration Data ID is found
                bConfigDataIDFound.at(PolicyIdx) = true;
            }
        }
    }
    // Ensure all Data IDs are found
    for (auto bValue : bConfigDataIDFound)
    {
        if (!bValue)
        {
            // Matching Data ID is not found
            LE_ERROR("Invalid Data ID referenced in Policy file.");
            LE_ERROR("Cross check Policy JSON's Use_Data_ID with Configruation JSON's Data->IDs");
            // Clean up and return false
            PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
            ConfigurationParserRef.ResetConfigurationStructure(ConfigurationStructRef);
            return false;
        }
    }

    // Check if Configuration Data objects refer valid Network objects
    std::vector<bool> bConfigNetworkIDFound(ConfigurationStructRef.DataCount, false);
    // Iterate through all Configuration Data elements
    for (uint8_t DataIdx = 0; DataIdx < ConfigurationStructRef.DataCount; DataIdx++)
    {
        // Iterate through all Configuration Network elements
        for (uint8_t NetworkIdx = 0; NetworkIdx < ConfigurationStructRef.NetworkCount; NetworkIdx++)
        {
            // Check if Data->Use_Network_ID has matching Nework->ID
            if (ConfigurationStructRef.Data[DataIdx].Use_Network_ID ==
                            ConfigurationStructRef.Network[NetworkIdx].ID)
            {
                // Matching Network ID found
                bConfigNetworkIDFound.at(DataIdx) = true;
            }
        }
    }
    // Ensure all Nework IDs are found
    for (auto bValue : bConfigNetworkIDFound)
    {
        if (!bValue)
        {
            // Matching Data ID is not found
            LE_ERROR("Invalid Network ID referenced in Configuration Data object.");
            LE_ERROR("Cross check Configuration JSON's Use_Network_ID with Network->IDs");
            // Clean up and return false
            PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
            ConfigurationParserRef.ResetConfigurationStructure(ConfigurationStructRef);
            return false;
        }
    }

    // Check if Configuration Network objects refer valid Sim objects
    std::vector<bool> bConfigSimIDFound(ConfigurationStructRef.NetworkCount, false);
    // Iterate through all Configuration Network elements
    for (uint8_t NetworkIdx = 0; NetworkIdx < ConfigurationStructRef.NetworkCount; NetworkIdx++)
    {
        // Iterate through all Configuration Network elements
        for (uint8_t SimIdx = 0; SimIdx < ConfigurationStructRef.SimCount; SimIdx++)
        {
            // Check if Data->Use_Network_ID has matching Nework->ID
            if (ConfigurationStructRef.Network[NetworkIdx].Use_Sim_ID ==
                                        ConfigurationStructRef.Sim[SimIdx].ID)
            {
                // Matching Network ID found
                bConfigSimIDFound.at(NetworkIdx) = true;
            }
        }
    }
    // Ensure all Sim IDs are found
    for (auto bValue : bConfigSimIDFound)
    {
        if (!bValue)
        {
            // Matching Data ID is not found
            LE_ERROR("Invalid Sim ID referenced in Configuration Network object.");
            LE_ERROR("Cross check Configuration JSON's Use_Sim_ID with Sim->IDs");
            // Clean up and return false
            PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
            ConfigurationParserRef.ResetConfigurationStructure(ConfigurationStructRef);
            return false;
        }
    }

    // Validations complete. Write to Config Tree
    if (!updateMangedConnectivityConfigTree(PolicyStructRef.ConfigurationFileName,
                                            bDoConfigFileNamesMatch))
    {
        LE_ERROR("Unable to update ConfigTree");
        // Clean up and return false
        PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
        ConfigurationParserRef.ResetConfigurationStructure(ConfigurationStructRef);
        return false;
    }

    LE_INFO("Parsing of JSON file and Updation of Policy and Configuration structures successful");
    return true;
}

/**
 * Match the JSON element with the validation function.
 */
void telux::tafsvc::UpdateValidConnectivityFuncMap(void)
{
    ConnectivityValidationFuncMap["ManagedConnectivityService:Version"] = &Validate_MCS_Version;
}