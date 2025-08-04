/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsUtils.hpp
 * @brief TelAF Data Call Service's utility functions.
 *
 */

#ifndef __TAF_DCS_SVC_UTILS_HPP__
#define __TAF_DCS_SVC_UTILS_HPP__

#include "legato.h"
#include "interfaces.h"
#include "taf_pa_dataTypes.hpp"

/**
 * Convert a ENUM to an integer primarily for printing with LE log APIs.
 *
 * Consider using the to_int template for more complex use cases.
 */
#define TO_INT(value) static_cast<int>(value)

/**
 * @brief Return true if the given "value" is within the specified range
 */
template <typename T>
bool isValueInRange(T value, T lowerBound, T upperBound)
{
    return value >= lowerBound && value <= upperBound;
}

namespace taf
{
namespace svc
{
namespace datacall
{
    class TafDcsUtils
    {
    public:
        static taf_dcs_ApnType_t               ConvertApnTypeMask(taf::pa::data::ApnTypeBitmask_e);
        static taf::pa::data::ApnTypeBitmask_e ConvertApnTypeMask(taf_dcs_ApnType_t);
        static taf_dcs_Auth_t                  ConvertAuthType(taf::pa::data::AuthType_e);
        static taf::pa::data::AuthType_e       ConvertAuthType(taf_dcs_Auth_t);
        static void                            ConvertCallEndReason
        (
            const taf::pa::data::DataCallEndReason_t&,
            taf_dcs_CallEndReasonType_t&,
             int32_t &
        );
        static taf_dcs_DataBearerTechnology_t  ConvertDataBearerTech
        (
            taf::pa::data::DataBearerTechnology_e
        );
        static taf::pa::data::DataCallStatus_e ConvertDataCallStatus(taf_dcs_ConState_t);
        static taf_dcs_ConState_t              ConvertDataCallStatus
        (
            taf::pa::data::DataCallStatus_e
        );

        static taf_dcs_Pdp_t                   ConvertPDP(taf::pa::data::IpType_e);
        static taf::pa::data::IpType_e         ConvertPDP(taf_dcs_Pdp_t);
        static taf_dcs_Tech_t                  ConvertTechPref(taf::pa::data::TechPref_e);
        static taf::pa::data::TechPref_e       ConvertTechPref(taf_dcs_Tech_t);
        static bool                            ConvertEmergencyCallSupport
        (
            taf::pa::data::EmergencyCapability_e
        );
        static taf::pa::data::EmergencyCapability_e ConvertEmergencyCallSupport(bool);
        static taf_dcs_RoamingType_t ConvertRoamingType(taf::pa::data::RoamingType_e);

        static taf_dcs_HwAccelerationState_t ConvertHwAccelerationState
        (
            taf::pa::data::HwAccelerationState_e
        );

        static taf_dcs_QosFlowState_t   ConvertQoSFlowState(taf::pa::data::QosFlowState_e);
        static taf_dcs_QosFlowBitMask_t ConvertQoSFlowBitMask(taf::pa::data::QosFlowMask_e);
    };

} // namespace datacall
} // namespace svc
} // namespace taf

#endif //__TAF_DCS_SVC_UTILS_HPP__