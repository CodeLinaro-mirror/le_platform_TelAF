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

#include <exception>
#include <stdexcept>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <cstring>
#include "tafMngdConnSvcParser_PolicyParser.hpp"


using std::to_string;

namespace pt = boost::property_tree;
using namespace telux::tafsvc;
using telux::tafsvc::tafMngdConnSvc_PolicyParser;

/**
 * Validate DataSession:DataConnection:Use_Data_ID
 */
bool tafMngdConnSvc_PolicyParser::Validate_DS_DC_Use_Data_ID(taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());

    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as DataConnection is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.DataConnection[Index].Use_Data_ID = std::stoi(Value);

    // Update the Data Connection Count.
    //Index will be 0. So count will be Index + 1
    Policy.DataSession.dataConnectionCount = Index + 1;
    return true;
}

/**
 * Validate DataSession:DataConnection:Priority
 */
bool tafMngdConnSvc_PolicyParser::Validate_DS_DC_Priority(taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as DataConnection is an array
    if (Index<0) {
        LE_WARN("Invalid Array Index");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.DataConnection[Index].Priority = std::stoi(Value);

    // Update the Data Connection Count.
    // Index will be 0. So count will be Index + 1
    Policy.DataSession.dataConnectionCount = Index + 1;
    return true;
}

/**
 * Validate DataSession:Fallback
 */
bool tafMngdConnSvc_PolicyParser::Validate_DS_Fallback(taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.Fallback = tafMngd_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate ManagedConnectivityServicePolicy:ConfigName
 */
bool tafMngdConnSvc_PolicyParser::Validate_MCSP_ConfigFileName(taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if ( TAF_MNGD_JSON_DATA_TYPE_STRING != DataType && TAF_MNGD_JSON_DATA_TYPE_NULL != DataType )
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Max string length should be TAF_MNGD_CONN_MAX_NAME_LEN
    if (Value.size() > TAF_MNGD_CONN_MAX_FILE_NAME_LEN)
    {
        LE_WARN("Configuration File Name is too long");
        return false;
    }
    // Valid value. Update Policy.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Policy.ConfigurationFileName, 0, TAF_MNGD_CONN_MAX_FILE_NAME_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Policy.ConfigurationFileName, Value.c_str(), TAF_MNGD_CONN_MAX_FILE_NAME_LEN,NULL);
    return true;
}

/**
 * Validate ManagedConnectivityServicePolicy:Name
 */
bool tafMngdConnSvc_PolicyParser::Validate_MCSP_Name(taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_STRING!= DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Max string length should be TAF_MNGD_CONN_MAX_NAME_LEN
    if (Value.size() > TAF_MNGD_CONN_MAX_NAME_LEN)
    {
        LE_WARN("Name is too long");
        return false;
    }
    // Valid value. Update Policy.
    le_utf8_Copy(Policy.Name, Value.c_str(), TAF_MNGD_CONN_MAX_NAME_LEN,NULL);
    return true;
}

/**
 * Validate ManagedConnectivityServicePolicy:Version
 */
bool tafMngdConnSvc_PolicyParser::Validate_MCSP_Version(taf_mngd_Conn_Policy_t &Policy,
                                                  std::string Value,
                                                  int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.Version = std::stoi(Value);
    return true;
}

/**
 * Check if the value for the property is of the correct type and also contains valid value.
 * The function to validate each value will be called. The respective function will update the
 * Policy structure if the value is valid.
 */
bool tafMngdConnSvc_PolicyParser::ValidateValue(taf_mngd_Conn_Policy_t &Policy,
                                          std::string property,
                                          std::string Value,
                                          int Index)
{
    LE_DEBUG("JSON_Property: %s, Value: %s", property.c_str(), Value.c_str());
    auto iterator = PolicyValidationFuncMap.find(property);
    if (iterator != PolicyValidationFuncMap.end() )
    {
        return (*iterator->second)(Policy,Value,Index);
    }

    // The property is not found, so it's unsupported. Return false.
    LE_WARN("%s is not supported", property.c_str());
    return false;
}

/**
 *
 * Function to parse JOSN. This function is aware of the JSON layout and will get each value
 * validated. This function will set the correct index for arrays. For non-array values, it
 * will set the index to -1 (TAF_MNGD_CONN_INVALID_INDEX)
 *
 */
bool tafMngdConnSvc_PolicyParser::ParseAndUpdatePolicyJSON(taf_mngd_Conn_Policy_t &Policy,
                                                                    std::string filename)
{
    // Try opening an input file stream
    std::ifstream jsonFile(filename);
    if (!jsonFile.is_open()) {
        LE_WARN ("Unable to open %s", filename.c_str());
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
    for (auto & element: tree) {

       if ("ManagedConnectivityService" == element.first ) {
            log.clear();
            log = "Top Element: " + element.first;
            LE_DEBUG ("%s", log.c_str() );

            for (auto & property: element.second) {

                if ("Policy" == property.first){
                    for (auto & parent: property.second) {
                        if ("Name" == parent.first){
                            log.clear();
                            log = "Key: " + parent.first + ", Value: " +
                                                    parent.second.get_value < std::string > ();
                            LE_DEBUG ("%s", log.c_str() );
                            JSON_Property.clear();
                            JSON_Property.append (property.first + ":" + parent.first);
                            JSON_Value.clear();
                            JSON_Value = parent.second.get_value<std::string>();
                            // Validate the value and update Configuration structure.
                            // Pass an invalid index as these are not arrays
                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                                                TAF_MNGD_CONN_INVALID_INDEX))
                            {
                                LE_WARN("Invalid JSON_Property Value");
                                LE_INFO("JSON_Property: %s, Value: %s", JSON_Property.c_str(),
                                                                                JSON_Value.c_str());
                                return false;
                            }
                        }

                        // Get the elements within "DataSession"
                        if ( "DataSession" == parent.first ) {
                            log.clear();
                            log.append("Section: " + parent.first);
                            LE_DEBUG ("%s", log.c_str() );
                            for (auto & child: parent.second) {
                                if (child.first != "" && "DataConnection" != child.first ){
                                    log.clear();
                                    log.append ("Key: " + child.first + ", Value: " +
                                                        child.second.get_value < std::string > () );
                                    LE_DEBUG ("%s", log.c_str() );
                                    JSON_Property.clear();
                                    JSON_Property.append(parent.first + ":" + child.first);
                                    JSON_Value.clear();
                                    JSON_Value.append(child.second.get_value<std::string>());
                                    // Validate values. Index is set to invald.
                                    if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                                                        TAF_MNGD_CONN_INVALID_INDEX))
                                    {
                                        LE_WARN("Invalid JSON_Property Value");
                                        LE_INFO("JSON_Property: %s, Value: %s",
                                        JSON_Property.c_str(),
                                        JSON_Value.c_str());
                                        return false;
                                    }
                                }

                                if ("DataConnection" == child.first) {
                                    int ElementCount = 0;
                                    // Use an iterator to get into the DataConnection array
                                    for (auto &it: child.second) {
                                        log.clear();
                                        log.append ("DataConnection["
                                        + to_string (ElementCount) + "]");
                                        LE_DEBUG ("%s", log.c_str() );
                                        // Use an iterator to go through  the DataConnection array
                                        for (auto &it2: it.second) {
                                            log.clear();
                                            log.append ( std::string ("\t") + "Key: "
                                                        + it2.first +
                                                        ", Value: " + it2.second.data() );
                                            LE_DEBUG ("%s", log.c_str() );
                                            JSON_Property.clear();
                                            JSON_Property.append(parent.first + ":"
                                            + child.first + ":" +
                                            it2.first);
                                            JSON_Value.clear();
                                            JSON_Value.append(it2.second.data());
                                            // Validate values. Index is set to correct value
                                            // as this is an array.
                                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                            ElementCount))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                        // Increment the element count
                                        ElementCount++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

void tafMngdConnSvc_PolicyParser::ResetPolicyStructure(taf_mngd_Conn_Policy_t &Policy)
{
    Policy.Name[0] = '\0';
    memset(Policy.ConfigurationFileName, 0, TAF_MNGD_CONN_MAX_FILE_NAME_LEN);
    Policy.DataSession.dataConnectionCount = 0;
}

bool tafMngdConnSvc_PolicyParser::GetPolicy ( taf_mngd_Conn_Policy_t &Policy,
                                                      std::string ConfigurationFileName )
{
    ResetPolicyStructure(Policy);
    if (!ParseAndUpdatePolicyJSON(Policy, ConfigurationFileName))
    {
        LE_WARN("Policy JSON is not valid");
        // Reset the Policy Structure
        ResetPolicyStructure(Policy);
        return false;
    }
    return true;
}

/**
 * Match the JSON element with the validation function.
 */
void tafMngdConnSvc_PolicyParser::UpdateValidPolicyFuncMap(void)
{
    PolicyValidationFuncMap["Policy:Name"] = &Validate_MCSP_Name;
    PolicyValidationFuncMap["DataSession:Fallback"] = &Validate_DS_Fallback;
    PolicyValidationFuncMap["DataSession:DataConnection:Priority"] = &Validate_DS_DC_Priority;
    PolicyValidationFuncMap["DataSession:DataConnection:Use_Data_ID"] = &Validate_DS_DC_Use_Data_ID;
}

/**
 * Provide the single instance of the Policy object
 */
tafMngdConnSvc_PolicyParser& tafMngdConnSvc_PolicyParser::getInstance()
{
    static tafMngdConnSvc_PolicyParser instance;

    // Update the properties and validation functions map
    instance.UpdateValidPolicyFuncMap();
    return instance;
}