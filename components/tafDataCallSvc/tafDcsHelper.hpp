/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   tafDcsHelper.hpp
 * @brief  Helper functions for the data call service.
 */

#pragma once

#include <string>
#include "interfaces.h"

namespace taf{
namespace svc{
namespace datacall{
class tafDCSHelper
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

};
} // namespace datacall
} // namespace svc
} // namespace taf
