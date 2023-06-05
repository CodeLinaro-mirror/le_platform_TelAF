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
#include "tafMngdConnSvcJSONParser.hpp"
#include "tafMngdConn_ConfigTreeHelper.hpp"

using namespace telux::tafsvc;
using std::string;

/**
 * Retrun true if the filenames match
 * Return false if the filenames do not match
 */
static bool updateMangedConnectivityConfigTree(string PolicyFileName,
                                               string ConfigurationFileName,
                                               bool   bDoConfigFileNamesMatch)
{
    le_result_t leRet = LE_OK;

    boost::filesystem::path fullFilePath(PolicyFileName);
    boost::filesystem::path dir = fullFilePath.parent_path();
    if (dir == TAF_MNGD_DefaultLocation_Policy)
    {
        LE_INFO("Files are in default location. Write only the FileName");
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_PolicyFileName,
                                                        fullFilePath.filename().string());
    }
    else
    {
        // Write the full file path
        leRet = tafMngd_ConfigTree_Update(TAF_MNGD_ct_node_PolicyFileName, PolicyFileName);
    }

    fullFilePath.clear();
    dir.clear();

    fullFilePath = ConfigurationFileName;
    dir = fullFilePath.parent_path();
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

/**
 * Retrun true if the filenames match
 * Return false if the filenames do not match
 */
static bool DoConfigurationFileNamesMatch(string API_ConfigurationFileName,
                                            string Policy_ConfigurationFileName)
{
    if (API_ConfigurationFileName == Policy_ConfigurationFileName)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool telux::tafsvc::tafMngdConnSvc_GetPolicyAndConfiguration(
    taf_mngd_Conn_Policy_t &PolicyStructRef,
    std::string PolicyFileName,
    taf_mngd_Conn_Configuration_t &ConfigurationStructRef,
    std::string ConfigurationFileName)
{
    std::string newConfFileName;
    bool bDoConfigFileNamesMatch = true;
    tafMngdConnSvc_PolicyParser &PolicyParserRef = tafMngdConnSvc_PolicyParser::getInstance();

    // Check if only Policy file name is path or if path is also provided.If path is not provided,
    // add the default path
    if ('/' != PolicyFileName[0])
    {
        LE_INFO("Path is not included in Policy File Name");
        PolicyFileName.insert (0, (TAF_MNGD_DefaultLocation_Policy + "/"));
    }

    if ( PolicyParserRef.GetPolicy(PolicyStructRef, PolicyFileName) )
    {
        LE_INFO("Policy Parsing Successful");
    }
    else
    {
        LE_ERROR("Policy Parsing Failed");
        return false;
    }

    if( ConfigurationFileName.empty() )
    {
        LE_INFO(" ConfigurationFileName empty");
        if(strlen(PolicyStructRef.ConfigurationFileName) == 0)
        {
            LE_ERROR("Configuration File name is not specified");
            return false;
        }
        else
        {
            newConfFileName.clear();
            newConfFileName = newConfFileName.append(PolicyStructRef.ConfigurationFileName);
        }
    }
    else
    {
        //Check if the configuration name matchs
        if (DoConfigurationFileNamesMatch(ConfigurationFileName,
                                            PolicyStructRef.ConfigurationFileName))
        {
            LE_INFO("Configuration File Names match");
        }
        else
        {
            LE_INFO("Configuration File Names do not match");
            bDoConfigFileNamesMatch = false;
            // Update the Policy Structure with the Configuration FileName from the API
            memset(PolicyStructRef.ConfigurationFileName, 0, TAF_MNGD_CONN_MAX_FILE_NAME_LEN);
            size_t copied_len = 0;
            le_utf8_Copy (PolicyStructRef.ConfigurationFileName,
                                                        ConfigurationFileName.c_str(),
                                                        TAF_MNGD_CONN_MAX_FILE_NAME_LEN,
                                                        &copied_len);
            if (copied_len > TAF_MNGD_CONN_MAX_FILE_NAME_LEN)
            {
                // Buffer overrun
                LE_ERROR("PolicyStructRef conf name buf overrun. Copied: %d. Buf Size: %d",
                            (int)copied_len, TAF_MNGD_CONN_MAX_FILE_NAME_LEN);
                // Clean up and return false
                PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
                return false;
            }
        }

        newConfFileName = ConfigurationFileName;
    }

    // Check if only Configuration file name is path or if path is also provided.If path is not
    // provided, add the default path
    if ('/' != newConfFileName[0])
    {
        LE_INFO("Path is not included in Configuration File Name");
        newConfFileName.insert (0, (TAF_MNGD_DefaultLocation_Configuration + "/"));
    }

    tafMngdConnSvc_ConfigurationParser &ConfigurationParserRef =
                                        tafMngdConnSvc_ConfigurationParser::getInstance();
    if ( ConfigurationParserRef.GetConfiguration(ConfigurationStructRef, newConfFileName) )
    {
        LE_INFO("Configuration Parsing Successful");
    }
    else
    {
        LE_ERROR("Configuration Parsing Failed");
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

    // Validations complete. Write to Config Tree
    if (!updateMangedConnectivityConfigTree(PolicyFileName,
                                            PolicyStructRef.ConfigurationFileName,
                                            bDoConfigFileNamesMatch))
    {
        LE_ERROR("Unable to update ConfigTree");
        // Clean up and return false
        PolicyParserRef.ResetPolicyStructure(PolicyStructRef);
        ConfigurationParserRef.ResetConfigurationStructure(ConfigurationStructRef);
        return false;
    }

    LE_INFO("Parsing of JSON files and Updation of Policy and Configuration structures successful");
    return true;
}