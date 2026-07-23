/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAF_NET_SVC_UTILS_HPP__
#define __TAF_NET_SVC_UTILS_HPP__

#include "legato.h"
#include "interfaces.h"
#include "tafCommonPa.h"


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

#endif //__TAF_NET_SVC_UTILS_HPP__