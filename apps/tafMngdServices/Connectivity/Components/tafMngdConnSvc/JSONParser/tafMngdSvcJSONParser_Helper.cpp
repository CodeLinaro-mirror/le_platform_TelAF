/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <string>
#include <regex>
#include <boost/algorithm/string.hpp>
#include "tafMngdSvcJSONParser_Helper.hpp"

using namespace tafsvc;

/**
 *
 * All values returned by Boost JSON parser via Propery Tree are strings. Type information is lost.
 * So we compare strings to get the specific data types. By default the function will return string.
 *
*/
mcs_JSON_Data_Types_t tafsvc::mcs_GetDataType(std::string Value)
{
    if (boost::iequals(Value, "null")) {
        return MCS_JSON_DATA_TYPE_NULL;
    }

    if (boost::iequals(Value, "Yes")) {
        return MCS_JSON_DATA_TYPE_YES_NO;
    }

    if (boost::iequals(Value, "No")) {
        return MCS_JSON_DATA_TYPE_YES_NO;
    }

    if (boost::iequals(Value, "Auto")) {
        return MCS_JSON_DATA_TYPE_NW_REGISTRATION;
    }

    if (boost::iequals(Value, "Manual"))
    {
        return MCS_JSON_DATA_TYPE_NW_REGISTRATION;
    }

    if (boost::iequals(Value, "None")) {
        return MCS_JSON_DATA_TYPE_NONE;
    }

    if (boost::iequals(Value, "L1") || boost::iequals(Value, "L2") || boost::iequals(Value, "L3"))
    {
        return MCS_JSON_DATA_TYPE_CONNRECOVERY_LEVEL;
    }

    // Only positive integers
    if (std::regex_match(Value, std::regex("[0-9]+"))){
        return MCS_JSON_DATA_TYPE_NUMBER;
    }

    return MCS_JSON_DATA_TYPE_STRING;
}

mcs_Yes_No_t tafsvc::mcs_Convert_to_Yes_No_enum(std::string Value)
{
    if (boost::iequals(Value, "yes"))
    {
        return MCS_YES;
    }

    if (boost::iequals(Value, "no"))
    {
        return MCS_NO;
    }
    return (mcs_Yes_No_t)MCS_JSON_DATA_TYPE_UNKNOWN;
}

mcs_NW_Registration_Type_t
            tafsvc::mcs_Convert_to_NW_Registration_Type_enum(std::string Value)
{
    if (boost::iequals(Value, "Auto"))
    {
        return MCS_NW_REGISTRATION_TYPE_AUTO;
    }

    if (boost::iequals(Value, "Manual"))
    {
        return MCS_NW_REGISTRATION_TYPE_MANUAL;
    }
    return (mcs_NW_Registration_Type_t)MCS_JSON_DATA_TYPE_UNKNOWN;
}

mcs_Policy_ConnRecoveryLevel_t
            tafsvc::mcs_Convert_to_ConnRecovery_Level_Type_enum(std::string Value)
{
    if (boost::iequals(Value, "None"))
    {
        return MCS_CONNECTIONRECOVERY_LEVEL_NONE;
    }

    if (boost::iequals(Value, "L1"))
    {
        return MCS_CONNECTIONRECOVERY_LEVEL_L1;
    }
    if (boost::iequals(Value, "L2"))
    {
        return MCS_CONNECTIONRECOVERY_LEVEL_L2;
    }
    if (boost::iequals(Value, "L3"))
    {
        return MCS_CONNECTIONRECOVERY_LEVEL_L3;
    }
    return (mcs_Policy_ConnRecoveryLevel_t)MCS_JSON_DATA_TYPE_UNKNOWN;
}