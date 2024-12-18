/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   tafDcsHelper.hpp
 * @brief  Helper functions for the data call service.
 */

#pragma once

#include "interfaces.h"
#include "telux/data/DataDefines.hpp"
#include "telux/common/ConnectivityDefines.hpp"

namespace telux
{
    namespace tafsvc
    {
        class taf_DCSHelper
        {
            public:

            /* TelAF conversions */
            static const char *DataBearerTechnologyToString(taf_dcs_DataBearerTechnology_t tech);
            static const char *RoamingTypeToString(taf_dcs_RoamingType_t roamingType);
            static const char *CallEventToString(taf_dcs_ConState_t callEvent);
            static const char *IpFamilyTypeToString(taf_dcs_Pdp_t ipType);

            /* TelSDK conversions */
            static const char *CallStatusToString(telux::data::DataCallStatus status);
            static const char *IpFamilyTypeToString(telux::data::IpFamilyType ipType);
            static const char *TechPreferenceToString(telux::data::TechPreference techPref);
            static const char *DataBearerToString(telux::data::DataBearerTechnology techPref);
            static const char *CallEndReasonTypeToString(telux::common::EndReasonType endType);
        };
    } // namespace tafsvc
} // namespace telux