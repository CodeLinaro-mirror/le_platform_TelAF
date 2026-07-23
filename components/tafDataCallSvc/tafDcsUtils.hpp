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
#include "tafCommonPa.h"
#include "tafDataPa.hpp"

/**
 * Convert a ENUM to an integer primarily for printing with LE log APIs.
 *
 * Consider using the to_int template for more complex use cases.
 */
#define TO_INT(value) static_cast<int>(value)

/**
 * Template tp convert taf_pa_result_t to le_result_t
 *
 */
template <typename T>
le_result_t PA_TO_LE_RESULT(T paResult)
{
    switch (static_cast<int32_t>(paResult))
    {
        case 0: return LE_OK;                  // TAF_PA_OK
        case -1: return LE_NOT_FOUND;          // TAF_PA_NOT_FOUND
        case -2: return LE_NOT_POSSIBLE;       // TAF_PA_NOT_POSSIBLE
        case -3: return LE_OUT_OF_RANGE;       // TAF_PA_OUT_OF_RANGE
        case -4: return LE_NO_MEMORY;          // TAF_PA_NO_MEMORY
        case -5: return LE_NOT_PERMITTED;      // TAF_PA_NOT_PERMITTED
        case -6: return LE_FAULT;              // TAF_PA_FAULT
        case -7: return LE_COMM_ERROR;         // TAF_PA_COMM_ERROR
        case -8: return LE_TIMEOUT;            // TAF_PA_TIMEOUT
        case -9: return LE_OVERFLOW;           // TAF_PA_OVERFLOW
        case -10: return LE_UNDERFLOW;         // TAF_PA_UNDERFLOW
        case -11: return LE_WOULD_BLOCK;       // TAF_PA_WOULD_BLOCK
        case -12: return LE_DEADLOCK;          // TAF_PA_DEADLOCK
        case -13: return LE_FORMAT_ERROR;      // TAF_PA_FORMAT_ERROR
        case -14: return LE_DUPLICATE;         // TAF_PA_DUPLICATE
        case -15: return LE_BAD_PARAMETER;     // TAF_PA_BAD_PARAMETER
        case -16: return LE_CLOSED;            // TAF_PA_CLOSED
        case -17: return LE_BUSY;              // TAF_PA_BUSY
        case -18: return LE_UNSUPPORTED;       // TAF_PA_UNSUPPORTED
        case -19: return LE_IO_ERROR;          // TAF_PA_IO_ERROR
        case -20: return LE_NOT_IMPLEMENTED;   // TAF_PA_NOT_IMPLEMENTED
        case -21: return LE_UNAVAILABLE;       // TAF_PA_UNAVAILABLE
        case -22: return LE_TERMINATED;        // TAF_PA_TERMINATED
        case -23: return LE_IN_PROGRESS;       // TAF_PA_IN_PROGRESS
        case -24: return LE_SUSPENDED;         // TAF_PA_SUSPENDED
        default:                               // Unknown PA result
        {
            LE_WARN("Unknown PA result value: %d", TO_INT(paResult));
            return LE_FAULT;
        }
    }
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

        // To string functions
        static const char *ToString(taf::pa::data::TechPref_e);
        static const char *ToString(taf::pa::data::Subsystem_e);
        static const char *ToString(taf::pa::data::SubsystemState_e);
    };

} // namespace datacall
} // namespace svc
} // namespace taf

#endif //__TAF_DCS_SVC_UTILS_HPP__