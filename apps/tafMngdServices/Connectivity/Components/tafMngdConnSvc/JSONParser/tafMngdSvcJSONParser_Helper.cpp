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

#include <string>
#include <regex>
#include <boost/algorithm/string.hpp>
#include "tafMngdSvcJSONParser_Helper.hpp"

using namespace telux::tafsvc;

/**
 *
 * All values returned by Boost JSON parser via Propery Tree are strings. Type information is lost.
 * So we compare strings to get the specific data types. By default the function will return string.
 *
*/
mcs_JSON_Data_Types_t telux::tafsvc::mcs_GetDataType(std::string Value)
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

mcs_Yes_No_t telux::tafsvc::mcs_Convert_to_Yes_No_enum(std::string Value)
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
            telux::tafsvc::mcs_Convert_to_NW_Registration_Type_enum(std::string Value)
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

mcs_Policy_ConnRecoveryLevel_t telux::tafsvc::
                                                mcs_Convert_to_ConnRecovery_Level_Type_enum
                                                (std::string Value)
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