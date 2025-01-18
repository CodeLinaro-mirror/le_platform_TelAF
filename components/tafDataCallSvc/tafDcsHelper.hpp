/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
            static const char *CallEndReasonTypeToString(taf_dcs_CallEndReasonType_t endReasonType);
            static const char *CallEndMobileIpReasonCodeToString(
                                                       taf_dcs_CallEndMobileIpReasonCode_t endCode);
            static const char *CallEndInternalReasonCodeToString(
                                                       taf_dcs_CallEndInternalReasonCode_t endCode);
            static const char *CallEndCallManagerReasonCodeToString(
                                                    taf_dcs_CallEndCallManagerReasonCode_t endCode);
            static const char *CallEnd3GPPSpecReasonCodeToString(
                                                       taf_dcs_CallEnd3GPPSpecReasonCode_t endCode);
            static const char *CallEndPPPReasonCodeToString(
                                                            taf_dcs_CallEndPPPReasonCode_t endCode);
            static const char *CallEndEHRPDReasonCodeToString(
                                                          taf_dcs_CallEndEHRPDReasonCode_t endCode);
            static const char *CallEndIPv6ReasonCodeToString(
                                                           taf_dcs_CallEndIPv6ReasonCode_t endCode);
            static const char *CallEndHandoffReasonCodeToString(
                                                        taf_dcs_CallEndHandoffReasonCode_t endCode);
            static const char *CallEndReasonCodeToString
                                       (taf_dcs_CallEndReasonType_t endReasonType, int32_t endCode);
            static const char *TechPreferenceToString(taf_dcs_Tech_t techPref);
            static std::string ApnTypeMaskToString(taf_dcs_ApnType_t apnTypeMask);
            static std::string AuthMaskToString(taf_dcs_Auth_t authMask);

            /* TelSDK conversions */
            static const char *CallStatusToString(telux::data::DataCallStatus status);
            static const char *IpFamilyTypeToString(telux::data::IpFamilyType ipType);
            static const char *TechPreferenceToString(telux::data::TechPreference techPref);
            static const char *DataBearerToString(telux::data::DataBearerTechnology techPref);
            static const char *CallEndReasonTypeToString(telux::common::EndReasonType endType);

            // Conversions from TelSDK types to TelAF types and vice-versa
            static taf_dcs_CallEndReasonType_t ConvertCallEndReasonType
                                            (telux::common::EndReasonType endReasonType);
            static taf_dcs_CallEndMobileIpReasonCode_t ConvertCallEndMobileIpReasonCode
                                            (telux::common::MobileIpReasonCode endReasonCode);
            static taf_dcs_CallEndInternalReasonCode_t ConvertCallEndInternalReasonCode
                                            (telux::common::InternalReasonCode endReasonCode);
            static taf_dcs_CallEndCallManagerReasonCode_t ConvertCallEndCallManagerReasonCode
                                            (telux::common::CallManagerReasonCode endReasonCode);
            static taf_dcs_CallEnd3GPPSpecReasonCode_t ConvertCallEnd3GPPSpecReasonCode
                                            (telux::common::SpecReasonCode endReasonCode);
            static taf_dcs_CallEndPPPReasonCode_t ConvertCallEndPPPReasonCode
                                            (telux::common::PPPReasonCode endReasonCode);
            static taf_dcs_CallEndEHRPDReasonCode_t ConvertCallEndEHRPDReasonCode
                                            (telux::common::EHRPDReasonCode endReasonCode);
            static taf_dcs_CallEndIPv6ReasonCode_t ConvertCallEndIPv6ReasonCode
                                            (telux::common::Ipv6ReasonCode endReasonCode);
            static taf_dcs_CallEndHandoffReasonCode_t ConvertCallEndHandoffReasonCode
                                            (telux::common::HandoffReasonCode endReasonCode);
        };
    } // namespace tafsvc
} // namespace telux

