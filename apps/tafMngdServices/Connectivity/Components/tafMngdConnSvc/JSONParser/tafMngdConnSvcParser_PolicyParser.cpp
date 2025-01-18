/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
using telux::tafsvc::mcs_PolicyParser;

/**
 * Validate DataSession:DataConnection:Use_Data_ID
 */
bool mcs_PolicyParser::Validate_DS_DC_Use_Data_ID(mcs_Policy_t &Policy,
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
    if (MCS_JSON_DATA_TYPE_NUMBER != DataType)
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

    //Check for Duplicates
    for (int i = 0; i <= Policy.DataSession.dataConnectionCount; i++) {
            if (std::stoi(Value) == Policy.DataSession.DataConnection[i].Use_Data_ID) {
                LE_WARN("Duplicate Data Id");
                return false;
            }
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
bool mcs_PolicyParser::Validate_DS_DC_Priority(mcs_Policy_t &Policy,
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
    if (MCS_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as DataConnection is an array
    if (Index<0) {
        LE_WARN("Invalid Array Index");
        return false;
    }

    //Check for Duplicates
    for (int i = 0; i <= Policy.DataSession.dataConnectionCount; i++) {
            if (std::stoi(Value) == Policy.DataSession.DataConnection[i].Priority) {
                LE_WARN("Duplicate Priority");
                return false;
            }
    }

    // Valid value. Update Policy.
    Policy.DataSession.DataConnection[Index].Priority = std::stoi(Value);

    // Update the Data Connection Count.
    // Index will be 0. So count will be Index + 1
    Policy.DataSession.dataConnectionCount = Index + 1;
    return true;
}

/**
 * Validate ManagedConnectivityServicePolicy:Name
 */
bool mcs_PolicyParser::Validate_MCSP_Name(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_STRING!= DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Max string length should be MCS_MAX_NAME_LEN
    if (Value.size() > MCS_MAX_NAME_LEN)
    {
        LE_WARN("Name is too long");
        return false;
    }
    // Valid value. Update Policy.
    le_utf8_Copy(Policy.Name, Value.c_str(), MCS_MAX_NAME_LEN,NULL);
    return true;
}



/**
 * Validate DataSession:MultiDataSession:Enable
 */
bool mcs_PolicyParser::Validate_DS_MDS_Enable(mcs_Policy_t &Policy,
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
    if (MCS_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.MultiDataSession.Enable = mcs_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataSession:MultiDataSession:NumConnections
 */
bool mcs_PolicyParser::Validate_DS_MDS_NumConnections(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    int localInt = 0;
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }
    if (MCS_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Ensure the value is within the range [0, UINT8_MAX]
    localInt = std::stoi(Value);
    if (localInt < 0 || localInt > UINT8_MAX)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.MultiDataSession.NumConnections = static_cast<uint8_t>(localInt);
    return true;
}

/**
 * Validate ConnectivityRecovery Level
 */
bool mcs_PolicyParser::Validate_DS_CR_Level(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    //Check the JSON version to be atleast 24.03.00
    if(Policy.Version < MCS_JSON_VERSION_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }

    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_CONNRECOVERY_LEVEL != DataType &&
        MCS_JSON_DATA_TYPE_NONE != DataType &&
        MCS_JSON_DATA_TYPE_NULL != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }

    if (MCS_JSON_DATA_TYPE_UNKNOWN == DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }

    // Valid value. Update Policy.
    // Since we have already validated type above, we can ignore return value here
    Policy.DataSession.ConnectivityRecovery.Level =
                                        mcs_Convert_to_ConnRecovery_Level_Type_enum(Value);

    return true;
}

/**
 * Validate ConnectivityRecovery StartWaitTime
 */
bool mcs_PolicyParser::Validate_DS_CR_StartWaitTime(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    int localInt = 0;
    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Check the JSON version to be atleast 24.07.00
    if (Policy.Version < MCS_JSON_VERSION_24_07_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    // Ensure the value is within the range [0, TAF_MNGDCONN_MAX_CONN_RECOVERY_START_WAIT_TIME]
    localInt = std::stoi(Value);
    if (localInt < 0 || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_START_WAIT_TIME)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.StartWaitTime = static_cast<uint8_t>(localInt);
    return true;
}

/**
 * Validate ConnectivityRecovery RetryWaitTime
 */
bool mcs_PolicyParser::Validate_DS_CR_RetryWaitTime(mcs_Policy_t &Policy,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());
    int localInt = 0;
    // Check the JSON version to be atleast 24.07.00
    if (Policy.Version < MCS_JSON_VERSION_24_07_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }

    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Ensure the value is within the range [0, TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME]
    localInt = std::stoi(Value);
    if (localInt < 0 || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.RetryWaitTime = static_cast<uint16_t>(localInt);
    return true;
}

/**
 * Validate ConnectivityRecovery:RadioOffOnInterval
 */
bool mcs_PolicyParser::Validate_DS_CR_RadioOffOnInterval(mcs_Policy_t &Policy,
                                                               std::string Value,
                                                               int Index)
{
    LE_DEBUG("%s", Value.c_str());
    int localInt = 0;

    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }

    // Check the JSON version to be atleast 24.07.00
    if (Policy.Version < MCS_JSON_VERSION_24_07_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }


    // Ensure the value is within the range [TAF_MNGDCONN_MIN_CONN_RECOVERY_L1_OFF_ON_INTERVAL,
    // TAF_MNGDCONN_MAX_CONN_RECOVERY_L1_OFF_ON_INTERVAL]
    localInt = std::stoi(Value);
    if (localInt < TAF_MNGDCONN_MIN_CONN_RECOVERY_RADIO_OFF_ON_INTERVAL
        || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_RADIO_OFF_ON_INTERVAL)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.RadioOffOnInterval = static_cast<uint8_t>(localInt);
    return true;
}

/**
 * Validate ConnectivityRecovery:SimOffOnInterval
 */
bool mcs_PolicyParser::Validate_DS_CR_SimOffOnInterval(mcs_Policy_t &Policy,
                                                               std::string Value,
                                                               int Index)
{
    LE_DEBUG("%s", Value.c_str());
    int localInt = 0;

    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }

    // Check the JSON version to be atleast 24.09.00
    if (Policy.Version < MCS_JSON_VERSION_24_09_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }


    // Ensure the value is within the range [TAF_MNGDCONN_MIN_CONN_RECOVERY_L1_OFF_ON_INTERVAL,
    // TAF_MNGDCONN_MAX_CONN_RECOVERY_L1_OFF_ON_INTERVAL]
    localInt = std::stoi(Value);
    if (localInt < TAF_MNGDCONN_MIN_CONN_RECOVERY_SIM_OFF_ON_INTERVAL
        || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_SIM_OFF_ON_INTERVAL)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.SimOffOnInterval = static_cast<uint8_t>(localInt);
    return true;
}

/**
 * Validate DataSession:ConnectivityRecovery:AllowCancel
 */
bool mcs_PolicyParser::Validate_DS_CR_AllowCancel(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }
    if (MCS_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.AllowCancel = mcs_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataSession:ConnectivityRecovery:VerifyCancelingApp
 */
bool mcs_PolicyParser::Validate_DS_CR_VerifyCancelingApp(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }
    if (MCS_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.ConnectivityRecovery.VerifyCancelingApp = mcs_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataSession:AppMngdConnectivityRecovery:Enable
 */
bool mcs_PolicyParser::Validate_DS_AMCR_Enable(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }
    if (MCS_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.AppMngdConnectivityRecovery.Enable = mcs_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataSession:AppMngdConnectivityRecovery:VerifyCallingApp
 */
bool mcs_PolicyParser::Validate_DS_AMCR_VerifyCallingApp(mcs_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    mcs_JSON_Data_Types_t DataType = mcs_GetDataType(Value);
    if (MCS_JSON_DATA_TYPE_NULL == DataType)
    {
        LE_WARN("Null value");
        return false;
    }
    if (MCS_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.AppMngdConnectivityRecovery.VerifyCallingApp =
                                                                 mcs_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataSession:AppMngdConnectivityRecovery:MinTimeBetweenTriggers
 */
bool mcs_PolicyParser::Validate_DS_AMCR_MinTimeBetweenTriggers(mcs_Policy_t &Policy,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    int localInt = 0;

    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Ensure the value is within the range [0, TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME]
    localInt = std::stoi(Value);
    if (localInt < 0 || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.AppMngdConnectivityRecovery.MinTimeBetweenTriggers =
                                                                    static_cast<uint8_t>(localInt);
    return true;
}

/**
 * Validate DataSession:AppMngdConnectivityRecovery:MaxTimeBetweenTriggers
 */
bool mcs_PolicyParser::Validate_DS_AMCR_MaxTimeBetweenTriggers(mcs_Policy_t &Policy,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());
    // Check the JSON version to be atleast 24.12.00
    if (Policy.Version < MCS_JSON_VERSION_24_12_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    int localInt = 0;

    if (MCS_JSON_DATA_TYPE_NUMBER != mcs_GetDataType(Value))
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Ensure the value is within the range [0, TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME]
    localInt = std::stoi(Value);
    if (localInt < 0 || localInt > TAF_MNGDCONN_MAX_CONN_RECOVERY_RETRY_WAIT_TIME)
    {
        LE_WARN("Value out of range: %d", localInt);
        return false;
    }
    // Valid value. Update Policy.
    Policy.DataSession.AppMngdConnectivityRecovery.MaxTimeBetweenTriggers =
                                                                    static_cast<uint16_t>(localInt);
    return true;
}

/**
 * Check if the value for the property is of the correct type and also contains valid value.
 * The function to validate each value will be called. The respective function will update the
 * Policy structure if the value is valid.
 */
bool mcs_PolicyParser::ValidateValue(mcs_Policy_t &Policy,
                                          std::string property,
                                          std::string Value,
                                          int Index)
{
    LE_DEBUG("JSON_Property: %s, Value: %s", property.c_str(), Value.c_str());
    if(Policy.DataSession.dataConnectionCount > MCS_MAX_DATA_OBJECT_COUNT)
    {
        LE_ERROR("DataConnection Count exceeded");
        return false;
    }
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
 * will set the index to -1 (MCS_INVALID_INDEX)
 *
 */
bool mcs_PolicyParser::ParseAndUpdatePolicyJSON(mcs_Policy_t &Policy,
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

    //  Keep track of mandatory objets. If they are absent return an error.
    bool bDataConnectionAvailable            = false;
    bool bMultiDataSessionAvailable          = false;
    bool bConnectivityRecoveryAvailable      = false;
    bool bAppManagedConnectivityRecovery     = false;
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
                            // Validate the value and update Policy structure.
                            // Pass an invalid index as these are not arrays
                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                                                MCS_INVALID_INDEX))
                            {
                                LE_WARN("Invalid JSON_Property Value");
                                LE_INFO("JSON_Property: %s, Value: %s", JSON_Property.c_str(),
                                                                        JSON_Value.c_str());
                                return false;
                            }
                        }

                        // Get the elements within "DataSession"
                        if ( "DataSession" == parent.first ) {
                            bDataConnectionAvailable = true;
                            log.clear();
                            log.append("Section: " + parent.first);
                            LE_DEBUG ("%s", log.c_str() );
                            for (auto & child: parent.second) {
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
                                if ("MultiDataSession" == child.first) {
                                    bMultiDataSessionAvailable = true;
                                    // Use an iterator to go through  the MultiDataSession elements
                                    for (auto &it: child.second) {
                                            log.clear();
                                            log.append ( std::string ("\t") + "Key: "
                                                        + it.first +
                                                        ", Value: " + it.second.data() );
                                            LE_DEBUG ("%s", log.c_str() );
                                            JSON_Property.clear();
                                            JSON_Property.append(parent.first + ":"
                                            + child.first + ":" +
                                            it.first);
                                            JSON_Value.clear();
                                            JSON_Value.append(it.second.data());
                                            // Validate values. Index is set to correct value
                                            // as this is an array.
                                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                            MCS_INVALID_INDEX))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                }
                                if ("ConnectivityRecovery" == child.first) {
                                    bConnectivityRecoveryAvailable = true;
                                 // Use an iterator to go through  the ConnectivityRecovery elements
                                    for (auto &it: child.second) {
                                            log.clear();
                                            log.append ( std::string ("\t") + "Key: "
                                                        + it.first +
                                                        ", Value: " + it.second.data() );
                                            LE_DEBUG ("%s", log.c_str() );
                                            JSON_Property.clear();
                                            JSON_Property.append(parent.first + ":"
                                            + child.first + ":" +
                                            it.first);
                                            JSON_Value.clear();
                                            JSON_Value.append(it.second.data());
                                            // Validate values. Index is set to correct value
                                            // as this is an array.
                                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                            MCS_INVALID_INDEX))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                }
                                if ("AppManagedConnectivityRecovery" == child.first) {
                                    bAppManagedConnectivityRecovery = true;
                                 // Use an iterator to go through  the
                                 // AppManagedConnectivityRecovery elements
                                    for (auto &it: child.second) {
                                            log.clear();
                                            log.append ( std::string ("\t") + "Key: "
                                                        + it.first +
                                                        ", Value: " + it.second.data() );
                                            LE_DEBUG ("%s", log.c_str() );
                                            JSON_Property.clear();
                                            JSON_Property.append(parent.first + ":"
                                            + child.first + ":" +
                                            it.first);
                                            JSON_Value.clear();
                                            JSON_Value.append(it.second.data());
                                            // Validate values. Index is set to correct value
                                            // as this is an array.
                                            if (!ValidateValue(Policy, JSON_Property, JSON_Value,
                                            MCS_INVALID_INDEX))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Validate presence of mandatory objects
    // The checking is done separately to return specific error logs.
    if (!bDataConnectionAvailable)
    {
        LE_ERROR("DataConnection object is missing");
        return false;
    }
    if (!bMultiDataSessionAvailable)
    {
        LE_ERROR("MultiDataSession object is missing");
        return false;
    }
    if (!bConnectivityRecoveryAvailable)
    {
        LE_ERROR("ConnectivityRecovery object is missing");
        return false;
    }
    if (!bAppManagedConnectivityRecovery)
    {
        LE_ERROR("AppManagedConnectivityRecovery object is missing");
        return false;
    }
    return true;
}

void mcs_PolicyParser::ResetPolicyStructure(mcs_Policy_t &Policy)
{
    Policy.Name[0] = '\0';
    Policy.DataSession.dataConnectionCount = 0;
}

/**
 * Comparator function
 */
static bool compareDataConn ( mcs_Policy_DataConnection_t DataConn_a,
                              mcs_Policy_DataConnection_t DataConn_b)
{
    return (DataConn_a.Priority < DataConn_b.Priority);
}

bool mcs_PolicyParser::GetPolicy ( mcs_Policy_t &Policy,
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
    // Sort DataConnection object based on priority
    const uint8_t count = Policy.DataSession.dataConnectionCount;
    uint8_t index;
    for (index = 0; index < count; index++)
    {
        LE_DEBUG("Priority: %d, Use_Data_ID: %d",
                Policy.DataSession.DataConnection[index].Priority,
                Policy.DataSession.DataConnection[index].Use_Data_ID);
    }
    std::sort(Policy.DataSession.DataConnection, Policy.DataSession.DataConnection + count,
                            compareDataConn);
    LE_DEBUG("Sorted");
    for (index = 0; index < count; index++)
    {
        LE_DEBUG("Priority: %d, Use_Data_ID: %d",
                Policy.DataSession.DataConnection[index].Priority,
                Policy.DataSession.DataConnection[index].Use_Data_ID);
    }
    //Check for Duplicates
    for(index = 0; index < count; index++)
    {
        // comparing adjacent elements of array
        if(Policy.DataSession.DataConnection[index].Priority==
           Policy.DataSession.DataConnection[index+1].Priority)
        {
            return false;
        }
    }
    return true;
}

/**
 * Match the JSON element with the validation function.
 */
void mcs_PolicyParser::UpdateValidPolicyFuncMap(void)
{
    PolicyValidationFuncMap["Policy:Name"] = &Validate_MCSP_Name;
    PolicyValidationFuncMap["DataSession:DataConnection:Priority"] = &Validate_DS_DC_Priority;
    PolicyValidationFuncMap["DataSession:DataConnection:Use_Data_ID"] = &Validate_DS_DC_Use_Data_ID;
    PolicyValidationFuncMap["DataSession:MultiDataSession:Enable"] = &Validate_DS_MDS_Enable;
    PolicyValidationFuncMap["DataSession:MultiDataSession:NumConnections"] =
                                                            &Validate_DS_MDS_NumConnections;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:Level"] = &Validate_DS_CR_Level;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:StartWaitTime"] =
                                                            &Validate_DS_CR_StartWaitTime;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:RetryWaitTime"] =
                                                        &Validate_DS_CR_RetryWaitTime;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:RadioOffOnInterval"] =
                                                        &Validate_DS_CR_RadioOffOnInterval;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:SimOffOnInterval"] =
                                                        &Validate_DS_CR_SimOffOnInterval;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:AllowCancel"] =
                                                        &Validate_DS_CR_AllowCancel;
    PolicyValidationFuncMap["DataSession:ConnectivityRecovery:VerifyCancelingApp"] =
                                                        &Validate_DS_CR_VerifyCancelingApp;
    PolicyValidationFuncMap["DataSession:AppManagedConnectivityRecovery:Enable"] =
                                                        &Validate_DS_AMCR_Enable;
    PolicyValidationFuncMap["DataSession:AppManagedConnectivityRecovery:VerifyCallingApp"] =
                                                        &Validate_DS_AMCR_VerifyCallingApp;
    PolicyValidationFuncMap["DataSession:AppManagedConnectivityRecovery:MinTimeBetweenTriggers"] =
                                                        &Validate_DS_AMCR_MinTimeBetweenTriggers;
    PolicyValidationFuncMap["DataSession:AppManagedConnectivityRecovery:MaxTimeBetweenTriggers"] =
                                                        &Validate_DS_AMCR_MaxTimeBetweenTriggers;
}

/**
 * Provide the single instance of the Policy object
 */
mcs_PolicyParser& mcs_PolicyParser::getInstance()
{
    static mcs_PolicyParser instance;

    // Update the properties and validation functions map
    instance.UpdateValidPolicyFuncMap();
    return instance;
}