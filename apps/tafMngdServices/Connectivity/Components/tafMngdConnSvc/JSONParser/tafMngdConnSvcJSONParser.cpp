/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

using namespace tafsvc;
using std::string;
using std::to_string;

namespace pt = boost::property_tree;

// Map of properties and validation function pointers
static std::map<std::string, ConnectivityValidationFunction_t> ConnectivityValidationFuncMap;

/**
 * List of supported JSON versions. Whenever there is an update to the JSON, ensure the version
 * is updated and checked.
 *
 *
 **/
static const char *JSON_Version_23_07_00 = "23.07.00";
static const char *JSON_Version_23_11_00 = "23.11.00";
static const char *JSON_Version_24_03_00 = "24.03.00";
static const char *JSON_Version_24_06_00 = "24.06.00";
static const char *JSON_Version_24_07_00 = "24.07.00";
static const char *JSON_Version_24_09_00 = "24.09.00";
static const char *JSON_Version_24_12_00 = "24.12.00";
static const char *JSON_Version_25_02_00 = "25.02.00";
static const char *JSON_Version_25_03_00 = "25.03.00";

/**
 * Validate ManagedConnectivityService:Version
 * Check for supported versions and set the Version to correct mcs_JSON_Version_t value.
 */
static bool Validate_MCS_Version(mcs_Policy_t &Policy,
                                                mcs_Configuration_t &Configuration,
                                                std::string Value,
                                                int Index)
{
    LE_DEBUG("%s", Value.c_str());
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);

    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }

    // Value should be a string
    if (MCS_JSON_DATA_TYPE_STRING != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }

    // Ensure JSON version is an approved verion
    // Set the Policy and Configuration Version accordingly
    if (Value == JSON_Version_23_07_00)
    {
        Policy.Version        = MCS_JSON_VERSION_23_07_00;
        Configuration.Version = MCS_JSON_VERSION_23_07_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_23_11_00)
    {
        Policy.Version        = MCS_JSON_VERSION_23_11_00;
        Configuration.Version = MCS_JSON_VERSION_23_11_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_24_03_00)
    {
        Policy.Version        = MCS_JSON_VERSION_24_03_00;
        Configuration.Version = MCS_JSON_VERSION_24_03_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_24_06_00)
    {
        Policy.Version        = MCS_JSON_VERSION_24_06_00;
        Configuration.Version = MCS_JSON_VERSION_24_06_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_24_07_00)
    {
        Policy.Version        = MCS_JSON_VERSION_24_07_00;
        Configuration.Version = MCS_JSON_VERSION_24_07_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_24_09_00)
    {
        Policy.Version        = MCS_JSON_VERSION_24_09_00;
        Configuration.Version = MCS_JSON_VERSION_24_09_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_24_12_00)
    {
        Policy.Version        = MCS_JSON_VERSION_24_12_00;
        Configuration.Version = MCS_JSON_VERSION_24_12_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_25_02_00)
    {
        Policy.Version        = MCS_JSON_VERSION_25_02_00;
        Configuration.Version = MCS_JSON_VERSION_25_02_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }
    else if (Value == JSON_Version_25_03_00)
    {
        Policy.Version        = MCS_JSON_VERSION_25_03_00;
        Configuration.Version = MCS_JSON_VERSION_25_03_00;
        LE_INFO("Valid JSON Version: %s", Value.c_str());
        return true;
    }

    LE_WARN("Invalid JSON Version: %s", Value.c_str());
    return false;
}

static bool ValidateValue(mcs_Policy_t& Policy,
                                mcs_Configuration_t &Configuration,
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

bool tafsvc::tafMngdConnSvc_GetPolicyAndConfiguration(
    mcs_Policy_t &PolicyStructRef,
    mcs_Configuration_t &ConfigurationStructRef,
    std::string ConfigurationFileName)
{
    le_result_t result;
    result = PreCheckExtensionJson(ConfigurationFileName, PolicyStructRef, ConfigurationStructRef);
    if(result == LE_OK)
    {
        return true;
    }
    return false;
}

le_result_t tafsvc::PreCheckExtensionJson(std::string ConfigurationFileName,
                            mcs_Policy_t &PolicyStructRef,
                            mcs_Configuration_t &ConfigurationStructRef)
{
    // Create a root
    pt::ptree root;
    std::string version = "";
    // Load the json file in this ptree
    try
    {
        pt::read_json(ConfigurationFileName, root);
        std::string extension = root.get<std::string>("Extension");
        if (extension != ""){
            char extensionPath[LE_LIMIT_MAX_PATH_LEN];
            snprintf(extensionPath,LE_LIMIT_MAX_PATH_LEN,"%s%s",extension.c_str(),
                ConfigurationFileName.c_str());
            if(DoesFileExist(extensionPath)){
                LE_INFO("Intializing with extension json");
                //Parse the JSON file, if fails initialize with default JSON file
                if(ParseJSON(extensionPath, PolicyStructRef, ConfigurationStructRef)){
                    LE_INFO("Service Initialize with extension json %s",extensionPath);
                    return LE_OK;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        LE_ERROR("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    LE_INFO("Unable to intialize with extension json ,Intializing with default json");
    //Initializing with default json
    bool res = ParseJSON(ConfigurationFileName, PolicyStructRef, ConfigurationStructRef);
    if(res){
        return LE_OK;
    }
    return LE_FAULT;
}

bool tafsvc::ParseJSON(std::string ConfigurationFileName,
                            mcs_Policy_t &PolicyStructRef,
                            mcs_Configuration_t &ConfigurationStructRef)
{
    mcs_PolicyParser &PolicyParserRef = mcs_PolicyParser::getInstance();
    mcs_ConfigurationParser &ConfigurationParserRef =
                                                mcs_ConfigurationParser::getInstance();
    // Update the properties and validation functions map
    UpdateValidConnectivityFuncMap();

    // Try opening an input file stream
    std::ifstream jsonFile(ConfigurationFileName);
    if (!jsonFile.is_open()) {
        LE_WARN ("Unable to open %s", ConfigurationFileName.c_str());
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
    // Check for Product, Name, and Version before parsing

    //  Keep track of mandatory objets. If they are absent return an error.
    bool bProductAvailable       = false;
    bool bNameAvailable          = false;
    bool bMngdConnSvcAvailable   = false;
    bool bVersionAvailable       = false;
    bool bPolicyAvailable        = false;
    bool bConfigurationAvailable = false;

    for (auto & element: tree) {
        //Product
        if ("Product" == element.first ) {
            // Mark presence of Product
            bProductAvailable = true;
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the value of "Product". This should be "TelAF"
            if (element.second.get_value<std::string>() != MCS_Default_Product_Value)
            {
                LE_WARN("Invalid JSON Product Value.");
                return false;
            }
        }

        //Name
        if ("Name" == element.first ) {
            // Mark presence of Name
            bNameAvailable = true;
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the value "Name" and print it for information.
            // Name key is not used by the service
            LE_INFO("JSON Name Value: %s", element.second.get_value<std::string>().c_str());
        }

        // ManagedConnectivityServicePolicy
        if ("ManagedConnectivityService" == element.first ) {
            // Mark presence of ManagedConnectivityService
            bMngdConnSvcAvailable = true;
            log.clear();
            log.append ( "Section: " + element.first );
            LE_DEBUG ("%s", log.c_str() );

            // Get the elements within "ManagedConnectivityServicePolicy"
            for (auto & property: element.second) {
                if ("Version" == property.first){
                    // Mark Version is present
                    bVersionAvailable = true;
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
                                    MCS_INVALID_INDEX))
                    {
                        LE_WARN("Invalid JSON_Property Value");
                        LE_INFO("JSON_Property: %s, Value: %s", JSON_Property.c_str(),
                                                                            JSON_Value.c_str());
                        return false;
                    }
                }

                if("Policy" == property.first){
                    // Mark Policy is present
                    bPolicyAvailable = true;
                    if (PolicyParserRef.GetPolicy(PolicyStructRef, ConfigurationFileName))
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
                    // Mark Configuration is present
                    bConfigurationAvailable = true;
                    if ( ConfigurationParserRef.GetConfiguration(ConfigurationStructRef,
                                                                        ConfigurationFileName) )
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

    // Validate presence of mandatory objects
    // The checking is done separately to return specific error logs.
    if (!bProductAvailable)
    {
        LE_ERROR("Product object is missing");
        return false;
    }
    if (!bNameAvailable)
    {
        LE_ERROR("Name object is missing");
        return false;
    }
    if (!bMngdConnSvcAvailable)
    {
        LE_ERROR("ManagedConnectivityService object is missing");
        return false;
    }
    if (!bVersionAvailable)
    {
        LE_ERROR("Version object is missing");
        return false;
    }
    if (!bPolicyAvailable)
    {
        LE_ERROR("Policy object is missing");
        return false;
    }
    if (!bConfigurationAvailable)
    {
        LE_ERROR("Configuration object is missing");
        return false;
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

    LE_INFO("Parsing of JSON file and Updation of Policy and Configuration structures successful");
    return true;
}

bool tafsvc::DoesFileExist(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

/**
 * Match the JSON element with the validation function.
 */
void tafsvc::UpdateValidConnectivityFuncMap(void)
{
    ConnectivityValidationFuncMap["ManagedConnectivityService:Version"] = &Validate_MCS_Version;
}
