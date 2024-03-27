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
#include "tafMngdConnSvcParser_ConfigurationParser.hpp"


using std::string;
using std::to_string;
using namespace telux::tafsvc;

namespace pt = boost::property_tree;
using telux::tafsvc::tafMngdConnSvc_ConfigurationParser;

/**
 * Name of the configuration should be a string
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Name(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Max string length should be TAF_MNGD_CONN_MAX_NAME_LEN
    if (Value.size() > TAF_MNGD_CONN_MAX_NAME_LEN)
    {
        LE_WARN("Configuration Name is too long");
        return false;
    }
    // Valid value. Update Configuration.
    le_utf8_Copy(Configuration.Name, Value.c_str(), TAF_MNGD_CONN_MAX_NAME_LEN,NULL);
    return true;
}

/**
 * SIM ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Sim_ID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Sim is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }
    // Update the Sim Count.
    // Index will be 0. So count will be Index + 1
    Configuration.SimCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Sim[Index].ID = std::stoi(Value);
    return true;
}

/**
 * Sim Name should be a string.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Sim_Name(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Sim is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Max Sim name should be TAF_MNGD_CONN_MAX_NAME_LEN
    if (Value.size() > TAF_MNGD_CONN_MAX_NAME_LEN)
    {
        LE_WARN("Sim Name is too long");
        return false;
    }

    //Check for Duplicates
    for (int i = 0; i <= Configuration.SimCount; i++) {
            if (Value == Configuration.Sim[i].Name) {
                LE_WARN("Duplicate Sim Name");
                return false;
            }
    }

    // Update the Sim Count.
    // Index will be 0. So count will be Index + 1
    Configuration.SimCount = Index + 1;

    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Sim[Index].Name, Value.c_str(), TAF_MNGD_CONN_MAX_NAME_LEN,NULL);

    return true;
}

/**
 * Sim slot number should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Sim_SlotNumber(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Sim is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Sim Count.
    // Index will be 0. So count will be Index + 1
    Configuration.SimCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Sim[Index].SlotNumber = std::stoi(Value);
    return true;
}

/**
 * Network ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Network_ID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Network is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Network Count.
    // Index will be 0. So count will be Index + 1
    Configuration.NetworkCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Network[Index].ID = std::stoi(Value);
    return true;
}

/**
 * Network Name should be a string.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Network_Name(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Network is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Max Network name should be TAF_MNGD_CONN_MAX_NAME_LEN
    if (Value.size() > TAF_MNGD_CONN_MAX_NAME_LEN)
    {
        LE_WARN("Network Name is too long");
        return false;
    }

    //Check for Duplicates
    for (int i = 0; i <= Configuration.NetworkCount; i++) {
            if (Value == Configuration.Network[i].Name) {
                LE_WARN("Duplicate Network Name");
                return false;
            }
    }

    // Update the Network Count.
    // Index will be 0. So count will be Index + 1
    Configuration.NetworkCount = Index + 1;

    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Network[Index].Name, Value.c_str(), TAF_MNGD_CONN_MAX_NAME_LEN,NULL);

    return true;
}

/**
 * Network Use_Sim_ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Network_Use_SIM_ID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Network is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Network Count.
    // Index will be 0. So count will be Index + 1
    Configuration.NetworkCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Network[Index].Use_Sim_ID = std::stoi(Value);
    return true;
}

/**
 *  Network Phone ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Network_PhoneID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Network is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    //If phoneID is less than zero or greater than two we return false
    if (std::stoi(Value) < 0 || std::stoi(Value) > 2)
    {
        LE_ERROR("Invalid PhoneID");
        return false;
    }

    // Update the Network Count.
    // Index will be 0. So count will be Index + 1
    Configuration.NetworkCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Network[Index].PhoneID = std::stoi(Value);
    return true;
}

/**
 * Network Registration should be Auto or Manual.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Network_Registration(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_NW_REGISTRATION != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Network is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Network Count.
    // Index will be 0. So count will be Index + 1
    Configuration.NetworkCount = Index + 1;

    // Valid value. Update Configuration.
    // Since we have already validated type above, we can ignore return value here
    Configuration.Network[Index].Registration = tafMngd_Convert_to_NW_Registration_Type_enum(Value);
    return true;
}

/**
 * Data ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_ID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Data[Index].ID = std::stoi(Value);

    return true;
}

/**
 * Data Use_Network_ID should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_Use_Network_ID(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Data[Index].Use_Network_ID= std::stoi(Value);
    return true;
}

/**
 * Data Profile Name should be a string.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_Name(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    //Check for Duplicates
    for (int i = 0; i <= Configuration.DataCount; i++) {
            if (Value == Configuration.Data[i].Profile.ProfileName) {
                LE_WARN("Duplicate Data Name");
                return false;
            }
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].Profile.ProfileName,
                            Value.c_str(),
                            TAF_MNGD_CONN_MAX_PROFILE_NAME_LEN,NULL);
    return true;
}

/**
 * Data Profile Number should be a number.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_Profile_Number(
                                            taf_mngd_Conn_Configuration_t &Configuration,
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Data[Index].Profile.ProfileNumber = std::stoi(Value);
    return true;
}

/**
 * Data Profile APN can be a string or NULL.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_Profile_APN(
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Configuration.Data[Index].Profile.APN, 0, TAF_MNGD_CONN_MAX_APN_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].Profile.APN, Value.c_str(),
                                            TAF_MNGD_CONN_MAX_APN_LEN,NULL);
    return true;
}

/**
 * Validate Data:AutoStart
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_AutoStart(
                                                    taf_mngd_Conn_Configuration_t &Configuration,
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
    // Valid value. Update Configuration.
    Configuration.Data[Index].AutoStart = tafMngd_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Data Connection Test URL can be a string or NULL.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_DataStartConnectionTest_URL(
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Configuration.Data[Index].DataStartConnectionTest.URL, 0,
            TAF_MNGD_CONN_MAX_CONNECTION_URL_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].DataStartConnectionTest.URL, Value.c_str(),
                                        TAF_MNGD_CONN_MAX_CONNECTION_URL_LEN,NULL);
    return true;
}

/**
 * Data Connection Test IPv4 can be a string or NULL.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_DataStartConnectionTest_IPv4(
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Configuration.Data[Index].DataStartConnectionTest.IPv4,0,TAF_MNGD_CONN_MAX_IPV4_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].DataStartConnectionTest.IPv4, Value.c_str(),
                                                TAF_MNGD_CONN_MAX_IPV4_LEN,NULL);

    return true;
}

/**
 * Data Connection Test IPv6 can be a string or NULL.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_DataStartConnectionTest_IPv6(
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
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Configuration.Data[Index].DataStartConnectionTest.IPv6,0,TAF_MNGD_CONN_MAX_IPV6_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].DataStartConnectionTest.IPv6, Value.c_str(),
                                                TAF_MNGD_CONN_MAX_IPV6_LEN,NULL);
    return true;
}

/**
 * Validate DataStartRetry:Enable
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_DSR_Enable(
                                                    taf_mngd_Conn_Configuration_t &Configuration,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_YES_NO != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Valid value. Update Configuration.
    Configuration.Data[Index].DataStartRetry.Enable = tafMngd_Convert_to_Yes_No_enum(Value);
    return true;
}

/**
 * Validate DataStartRetry:RetryCount
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_DSR_RetryCount(
                                                    taf_mngd_Conn_Configuration_t &Configuration,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());

    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Value should be valid RetryCount
    if (std::stoi(Value) < 0 || std::stoi(Value) > TAF_MNGD_CONN_MAX_RETRY_COUNT) {
        LE_WARN("Invalid RetryCount");
        return false;
    }
    // Valid value. Update Configuration.
    Configuration.Data[Index].DataStartRetry.RetryCount = std::stoi(Value);
    return true;
}

/**
 * PeriodicConnectivityCheck Interval should be a number
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_PeriodicConnectivityCheck_Interval(
                                                    taf_mngd_Conn_Configuration_t &Configuration,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    Configuration.Data[Index].PeriodicConnectivityCheck.Interval = std::stoi(Value);
    return true;
}

/**
 * Validate PeriodicConnectivityCheck:RetryCount
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_PeriodicConnectivityCheck_RetryCount(
                                                    taf_mngd_Conn_Configuration_t &Configuration,
                                                    std::string Value,
                                                    int Index)
{
    LE_DEBUG("%s", Value.c_str());

    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_NUMBER != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Value should be valid RetryCount
    if (std::stoi(Value) < 0 || std::stoi(Value) > TAF_MNGD_CONN_MAX_RETRY_COUNT) {
        LE_WARN("Invalid RetryCount");
        return false;
    }
    // Valid value. Update Configuration.
    Configuration.Data[Index].PeriodicConnectivityCheck.RetryCount = std::stoi(Value);
    return true;
}

/**
 * PeriodicConnectivityCheck URL can be a string or NULL.
 */
bool tafMngdConnSvc_ConfigurationParser::Validate_MCSC_Data_PeriodicConnectivityCheck_URL(
                                            taf_mngd_Conn_Configuration_t &Configuration,
                                            std::string Value,
                                            int Index)
{
    LE_DEBUG("%s", Value.c_str());
    taf_mngd_JSON_Data_Types_t DataType = tafMngd_GetDataType(Value);
    //Check the JSON version to be atleast 24.03.00
    if(Configuration.Version != TAF_MNGD_CONN_JSON_Version_24_03_00)
    {
        LE_WARN("Invalid JSON version");
        return false;
    }
    if (TAF_MNGD_JSON_DATA_TYPE_STRING != DataType && TAF_MNGD_JSON_DATA_TYPE_NULL != DataType)
    {
        LE_WARN("Incorrect data type");
        return false;
    }
    // Index should be valid as Data is an array
    if (Index < 0)
    {
        LE_WARN("Invalid Array Index");
        return false;
    }

    // Update the Data Count.
    // Index will be 0. So count will be Index + 1
    Configuration.DataCount = Index + 1;

    // Valid value. Update Configuration.
    // Set to NULL or string
    if (TAF_MNGD_JSON_DATA_TYPE_NULL == DataType)
    {
        memset(Configuration.Data[Index].PeriodicConnectivityCheck.URL,0,
               TAF_MNGD_CONN_MAX_CONNECTION_URL_LEN);
        return true;
    }
    // Valid String.
    // Since we have already validated string length above, we can ignore return value here
    le_utf8_Copy(Configuration.Data[Index].PeriodicConnectivityCheck.URL, Value.c_str(),
                                        TAF_MNGD_CONN_MAX_CONNECTION_URL_LEN,NULL);
    return true;
}

bool tafMngdConnSvc_ConfigurationParser::ValidateValue(
                                                 taf_mngd_Conn_Configuration_t &Configuration,
                                                 std::string property,
                                                 std::string Value,
                                                 int Index)
{
    LE_DEBUG("Property: %s, Value: %s", property.c_str(), Value.c_str());
    auto iterator = ConfigurationValidationFuncMap.find(property);
    if (iterator != ConfigurationValidationFuncMap.end())
    {
        return (*iterator->second)(Configuration, Value, Index);
    }

    // The property is not found, so it's unsupported. Return false.
    LE_WARN("%s is not supported", property.c_str());
    return false;
}

bool tafMngdConnSvc_ConfigurationParser::ParseAndUpdateConfigurationJSON(
                                                taf_mngd_Conn_Configuration_t &Configuration,
                                                std::string filename)
{
    LE_DEBUG ("Parse Configuration Function");
    // Try opening an input file stream
    std::ifstream jsonFile(filename);
    if (!jsonFile.is_open())
    {
        LE_WARN("Unable to open %s", filename.c_str());
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

    std::string log, JSON_Property, JSON_Value;
    for (auto & element: tree) {


        if ("ManagedConnectivityService" == element.first ) {
            log.clear();
            log = "Top Element: " + element.first;
            LE_DEBUG ("%s", log.c_str() );

            for (auto & property: element.second) {

                if ("Configuration" == property.first){
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
                            if (!ValidateValue(Configuration, JSON_Property, JSON_Value,
                                                                TAF_MNGD_CONN_INVALID_INDEX))
                            {
                                LE_WARN("Invalid JSON_Property Value");
                                LE_INFO("JSON_Property: %s, Value: %s", JSON_Property.c_str(),
                                                                                JSON_Value.c_str());
                                return false;
                            }
                        }

                        // Sim Object
                        // Array object with the following properties
                        // ID, Name, SlotNumber
                        if ( "Sim" == parent.first ) {
                            log.clear();
                            log = "Top Element: " + parent.first;
                            LE_DEBUG ("%s", log.c_str() );


                            int ElementCount = 0;
                            // Iterate through the Array elements
                            for (auto &array_element: parent.second)
                            {
                                log.clear();
                                log.append ( string("Sim[") + to_string (ElementCount) + "]" );
                                LE_DEBUG ("%s", log.c_str() );
                                // Iterate through elements in each array element
                                for (auto &iter: array_element.second)
                                {
                                    log.clear();
                                    log = "\tKey: " + iter.first + ", Value: " + iter.second.data();
                                    LE_DEBUG ("%s", log.c_str() );

                                    JSON_Property.clear();
                                    JSON_Property.append (parent.first + ":" + iter.first);
                                    JSON_Value.clear();
                                    JSON_Value = iter.second.data();
                                    // Validate the value and update Configuration structure.
                                    // Sim is an array, so pass element count.
                                    if (!ValidateValue(Configuration, JSON_Property, JSON_Value,
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

                        // Network object
                        if ( "Network" == parent.first ) {
                            log.clear();
                            log = "Top Element: " + parent.first;
                            LE_DEBUG ("%s", log.c_str() );

                            int ElementCount = 0;
                            // Iterate through the Array elements
                            for (auto &array_element: parent.second)
                            {
                                log.clear();
                                log.append ( string("Network[") + to_string (ElementCount) + "]" );
                                LE_DEBUG ("%s", log.c_str() );
                                // Iterate through elements in each array element
                                for (auto &iter: array_element.second)
                                {
                                    log.clear();
                                    log = "\tKey: " + iter.first + ", Value: " + iter.second.data();
                                    LE_DEBUG ("%s", log.c_str() );

                                    JSON_Property.clear();
                                    JSON_Property.append (parent.first + ":" + iter.first);
                                    JSON_Value.clear();
                                    JSON_Value = iter.second.data();
                                    // Validate the value and update Configuration structure.
                                    // Network is an array, so pass element count.
                                    if (!ValidateValue(Configuration, JSON_Property, JSON_Value,
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

                        // Data Object
                        if ( "Data" == parent.first ) {
                            log.clear();
                            log = "Top Element: " + parent.first;
                            LE_DEBUG ("%s", log.c_str() );

                            int ElementCount = 0;
                            // Iterate through the Array elements
                            for (auto &array_element: parent.second)
                            {
                                log.clear();
                                log.append ( string("\tData[") + to_string (ElementCount) + "]" );
                                LE_DEBUG ("%s", log.c_str() );
                                // Iterate through elements in each array element
                                for (auto &iter: array_element.second)
                                {
                                    if ( "ID" == iter.first ||
                                        "Name" == iter.first ||
                                        "Use_Network_ID" == iter.first ||
                                        "AutoStart" == iter.first )
                                    {
                                        log.clear();
                                        log = "\t\tKey: " + iter.first +
                                        ", Value: " + iter.second.data();
                                        LE_DEBUG("%s", log.c_str());

                                        JSON_Property.clear();
                                        JSON_Property.append (parent.first + ":" + iter.first);
                                        JSON_Value.clear();
                                        JSON_Value = iter.second.data();
                                        // Validate the value and update Configuration structure.
                                        // Data is an array, so pass element count.
                                        if (!ValidateValue(Configuration, JSON_Property, JSON_Value,
                                        ElementCount))
                                        {
                                            LE_WARN("Invalid JSON_Property Value");
                                            LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                            return false;
                                        }
                                    }
                                    // Iterate through Profile object
                                    else if ("Profile" == iter.first)
                                    {
                                        log.clear();
                                        log.append("\t\t").append("Profile Node");
                                        LE_DEBUG("%s", log.c_str());
                                        // Iterate through Profile object
                                        for (auto &iter2 : iter.second)
                                        {
                                            log.clear();
                                            log.append("\t\t\t").append("Key: " + iter2.first +
                                                                        ", Value: "
                                                                        + iter2.second.data());
                                            LE_DEBUG("%s", log.c_str());

                                            JSON_Property.clear();
                                            JSON_Property.append (parent.first + ":" + iter.first +
                                                                                ":" + iter2.first);
                                            JSON_Value.clear();
                                            JSON_Value = iter2.second.data();
                                        // Validate the value and update Configuration structure.
                                        // Data is an array, so pass element count.
                                            if (! ValidateValue(Configuration, JSON_Property,
                                                                JSON_Value, ElementCount))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                    }
                                    // Iterate through DataStartConnectionTest object
                                    else if ("DataStartConnectionTest" == iter.first)
                                    {
                                        log.clear();
                                        log.append("\t\t").append("DataStartConnectionTest Node");
                                        LE_DEBUG ("%s", log.c_str() );

                                        // Iterate through DataStartConnectionTest object
                                        for (auto &iter2: iter.second) {
                                            log.clear();
                                            log.append("\t\t\t").append("Key: " + iter2.first +
                                                                        ", Value: "
                                                                        + iter2.second.data());
                                            LE_DEBUG ("%s", log.c_str() );

                                            JSON_Property.clear();
                                            JSON_Property.append (parent.first + ":" + iter.first +
                                                                                ":" + iter2.first);
                                            JSON_Value.clear();
                                            JSON_Value = iter2.second.data();
                                        // Validate the value and update Configuration structure.
                                        // Data is an array, so pass element count.
                                            if (! ValidateValue(Configuration, JSON_Property,
                                                                JSON_Value, ElementCount))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                    }
                                    else if ("DataStartRetry" == iter.first)
                                    {
                                        log.clear();
                                        log.append("\t\t").append("DataStartRetry Node");
                                        LE_DEBUG ("%s", log.c_str() );

                                        // Iterate through DataStartRetry object
                                        for (auto &iter2: iter.second) {
                                            log.clear();
                                            log.append("\t\t\t").append("Key: " + iter2.first +
                                                                        ", Value: "
                                                                        + iter2.second.data());
                                            LE_DEBUG ("%s", log.c_str() );

                                            JSON_Property.clear();
                                            JSON_Property.append (parent.first + ":" + iter.first +
                                                                                ":" + iter2.first);
                                            JSON_Value.clear();
                                            JSON_Value = iter2.second.data();
                                        // Validate the value and update Configuration structure.
                                        // Data is an array, so pass element count.
                                            if (! ValidateValue(Configuration, JSON_Property,
                                                                JSON_Value, ElementCount))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                    }
                                    // Iterate through PeriodicConnectivityCheck object
                                    else if ("PeriodicConnectivityCheck" == iter.first)
                                    {
                                        log.clear();
                                        log.append("\t\t").append("PeriodicConnectivityCheck Node");
                                        LE_DEBUG ("%s", log.c_str() );

                                        // Iterate through PeriodicConnectivityCheck object
                                        for (auto &iter2: iter.second) {
                                            log.clear();
                                            log.append("\t\t\t").append("Key: " + iter2.first +
                                                                        ", Value: "
                                                                        + iter2.second.data());
                                            LE_DEBUG ("%s", log.c_str() );

                                            JSON_Property.clear();
                                            JSON_Property.append (parent.first + ":" + iter.first +
                                                                                ":" + iter2.first);
                                            JSON_Value.clear();
                                            JSON_Value = iter2.second.data();
                                        // Validate the value and update Configuration structure.
                                        // Data is an array, so pass element count.
                                            if (! ValidateValue(Configuration, JSON_Property,
                                                                JSON_Value, ElementCount))
                                            {
                                                LE_WARN("Invalid JSON_Property Value");
                                                LE_INFO("JSON_Property: %s, Value: %s",
                                                JSON_Property.c_str(),
                                                JSON_Value.c_str());
                                                return false;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        log.clear();
                                        log.append("*****").append("Unknown Object: " + iter.first);
                                        LE_WARN("%s", log.c_str());
                                    }
                                }
                                // Increment the element count
                                ElementCount++;
                            }
                        }
                    }
                }
            }
            LE_DEBUG ("%s", log.c_str() ); log.clear();
        }
    }
    return true;
}

/**
 * Match the JSON element with the validation function.
 */
void tafMngdConnSvc_ConfigurationParser::UpdateValidConfigurationFuncMap(void)
{
    ConfigurationValidationFuncMap["Configuration:Name"]
                                                            = &Validate_MCSC_Name;

    // Sim
    ConfigurationValidationFuncMap["Sim:ID"]         = &Validate_MCSC_Sim_ID;
    ConfigurationValidationFuncMap["Sim:Name"]       = &Validate_MCSC_Sim_Name;
    ConfigurationValidationFuncMap["Sim:SlotNumber"] = &Validate_MCSC_Sim_SlotNumber;

    // Network
    ConfigurationValidationFuncMap["Network:ID"]           = &Validate_MCSC_Network_ID;
    ConfigurationValidationFuncMap["Network:Name"]         = &Validate_MCSC_Network_Name;
    ConfigurationValidationFuncMap["Network:Use_Sim_ID"]   = &Validate_MCSC_Network_Use_SIM_ID;
    ConfigurationValidationFuncMap["Network:PhoneID"]      = &Validate_MCSC_Network_PhoneID;
    ConfigurationValidationFuncMap["Network:Registration"] = &Validate_MCSC_Network_Registration;

    // Data
    ConfigurationValidationFuncMap["Data:ID"]             = &Validate_MCSC_Data_ID;
    ConfigurationValidationFuncMap["Data:Use_Network_ID"] = &Validate_MCSC_Data_Use_Network_ID;
    ConfigurationValidationFuncMap["Data:Name"]           = &Validate_MCSC_Data_Name;

    // Data:Profile
    ConfigurationValidationFuncMap["Data:Profile:Number"] = &Validate_MCSC_Data_Profile_Number;
    ConfigurationValidationFuncMap["Data:Profile:APN"]    = &Validate_MCSC_Data_Profile_APN;
    // Data: AutoStart
    ConfigurationValidationFuncMap["Data:AutoStart"] = &Validate_MCSC_Data_AutoStart;
    // Data:DataStartConnectionTest
    ConfigurationValidationFuncMap["Data:DataStartConnectionTest:URL"]
                                = &Validate_MCSC_Data_DataStartConnectionTest_URL;
    ConfigurationValidationFuncMap["Data:DataStartConnectionTest:IPv4"]
                                = &Validate_MCSC_Data_DataStartConnectionTest_IPv4;
    ConfigurationValidationFuncMap["Data:DataStartConnectionTest:IPv6"]
                                = &Validate_MCSC_Data_DataStartConnectionTest_IPv6;
    // Data:DataStartRetry
    ConfigurationValidationFuncMap["Data:DataStartRetry:RetryCount"]
                                = &Validate_MCSC_Data_DSR_RetryCount;
    ConfigurationValidationFuncMap["Data:DataStartRetry:Enable"] = &Validate_MCSC_Data_DSR_Enable;
    // Data:PeriodicConnectivityCheck
    ConfigurationValidationFuncMap["Data:PeriodicConnectivityCheck:URL"]
                                = &Validate_MCSC_Data_PeriodicConnectivityCheck_URL;
    ConfigurationValidationFuncMap["Data:PeriodicConnectivityCheck:Interval"]
                                = &Validate_MCSC_Data_PeriodicConnectivityCheck_Interval;
    ConfigurationValidationFuncMap["Data:PeriodicConnectivityCheck:RetryCount"]
                                = &Validate_MCSC_Data_PeriodicConnectivityCheck_RetryCount;

}

void tafMngdConnSvc_ConfigurationParser::ResetConfigurationStructure (
                                            taf_mngd_Conn_Configuration_t &Configuration)
{
    Configuration.Name[0] = '\0';

    Configuration.SimCount = 0;
    for (unsigned int Index = 0; Index < TAF_MNGD_CONN_MAX_SIM_OBJECT_COUNT; Index++)
    {
        Configuration.Sim[Index].ID = 0;
        Configuration.Sim[Index].SlotNumber = 0;
        Configuration.Sim[Index].Name[0] = '\0';
    }

    Configuration.NetworkCount = 0;
    for (unsigned int Index = 0; Index < TAF_MNGD_CONN_MAX_NETWORK_OBJECT_COUNT; Index++)
    {
        Configuration.Network[Index].ID = 0;
        Configuration.Network[Index].Name[0] = '\0';
        Configuration.Network[Index].Use_Sim_ID = 0;
        Configuration.Network[Index].PhoneID = 0;
        Configuration.Network[Index].Registration = TAF_MNGD_CONN_NW_REGISTRATION_TYPE_AUTO;
    }

    Configuration.DataCount = 0;
    for (unsigned int Index = 0; Index < TAF_MNGD_CONN_MAX_DATA_OBJECT_COUNT; Index++)
    {
        Configuration.Data[Index].ID                     = 0;
        Configuration.Data[Index].Use_Network_ID         = 0;
        Configuration.Data[Index].Profile.ProfileNumber  = 0;
        Configuration.Data[Index].Profile.ProfileName[0] = '\0';
        Configuration.Data[Index].Profile.APN[0]         = '\0';
        Configuration.Data[Index].DataStartConnectionTest.URL[0]        = '\0';
        Configuration.Data[Index].DataStartConnectionTest.IPv4[0]       = '\0';
        Configuration.Data[Index].DataStartConnectionTest.IPv6[0]       = '\0';
        Configuration.Data[Index].PeriodicConnectivityCheck.Interval      = 0;
        Configuration.Data[Index].PeriodicConnectivityCheck.RetryCount    = 0;
        Configuration.Data[Index].PeriodicConnectivityCheck.URL[0]        = '\0';
        Configuration.Data[Index].DataStartRetry.RetryCount = 0;
        Configuration.Data[Index].DataStartRetry.Enable = tafMngd_Convert_to_Yes_No_enum("Yes");
    }
}

bool tafMngdConnSvc_ConfigurationParser::GetConfiguration(
                                                taf_mngd_Conn_Configuration_t &Configuration,
                                                std::string ConfigurationFileName)
{
    ResetConfigurationStructure(Configuration);
    if (!ParseAndUpdateConfigurationJSON (Configuration, ConfigurationFileName))
    {
        LE_WARN("Configuration JSON is not valid");
        // Reset the Configuration Structure
        ResetConfigurationStructure(Configuration);
        return false;
    }
    return true;
}

/**
 * Provide the single instance of the Configuration object
 */
tafMngdConnSvc_ConfigurationParser& tafMngdConnSvc_ConfigurationParser::getInstance()
{
    static tafMngdConnSvc_ConfigurationParser instance;

    // Update the properties and validation functions map
    instance.UpdateValidConfigurationFuncMap();
    return instance;
}