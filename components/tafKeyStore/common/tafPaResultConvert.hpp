/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFPARESULTCONVERT_HPP
#define TAFPARESULTCONVERT_HPP

#include "legato.h"
#include "tafCommonPa.h"

//--------------------------------------------------------------------------------------------------
/**
 * Mapping taf_pa_result_t to le_result_t. Shared by the KeyStore/FS-Crypt service classes.
 */
//--------------------------------------------------------------------------------------------------
inline le_result_t ConvertPaToLeRet(taf_pa_result_t paResult)
{
    switch(paResult)
    {
        case TAF_PA_OK:
            return LE_OK;
            break;
        case TAF_PA_NOT_FOUND:
            return LE_NOT_FOUND;
            break;
        case TAF_PA_NOT_POSSIBLE:
            return LE_NOT_POSSIBLE;
            break;
        case TAF_PA_OUT_OF_RANGE:
            return LE_OUT_OF_RANGE;
            break;
        case TAF_PA_NO_MEMORY:
            return LE_NO_MEMORY;
            break;
        case TAF_PA_NOT_PERMITTED:
            return LE_NOT_PERMITTED;
            break;
        case TAF_PA_FAULT:
            return LE_FAULT;
            break;
        case TAF_PA_COMM_ERROR:
            return LE_COMM_ERROR;
            break;
        case TAF_PA_TIMEOUT:
            return LE_TIMEOUT;
            break;
        case TAF_PA_OVERFLOW:
            return LE_OVERFLOW;
            break;
        case TAF_PA_UNDERFLOW:
            return LE_UNDERFLOW;
            break;
        case TAF_PA_WOULD_BLOCK:
            return LE_WOULD_BLOCK;
            break;
        case TAF_PA_DEADLOCK:
            return LE_DEADLOCK;
            break;
        case TAF_PA_FORMAT_ERROR:
            return LE_FORMAT_ERROR;
            break;
        case TAF_PA_DUPLICATE:
            return LE_DUPLICATE;
            break;
        case TAF_PA_BAD_PARAMETER:
            return LE_BAD_PARAMETER;
            break;
        case TAF_PA_CLOSED:
            return LE_CLOSED;
            break;
        case TAF_PA_BUSY:
            return LE_BUSY;
            break;
        case TAF_PA_UNSUPPORTED:
            return LE_CAP_UNSUPPORTED;
            break;
        case TAF_PA_IO_ERROR:
            return LE_IO_ERROR;
            break;
        case TAF_PA_NOT_IMPLEMENTED:
            return LE_CAP_NOT_IMPLEMENTED;
            break;
        case TAF_PA_UNAVAILABLE:
            return LE_UNAVAILABLE;
            break;
        case TAF_PA_TERMINATED:
            return LE_TERMINATED;
            break;
        case TAF_PA_IN_PROGRESS:
            return LE_IN_PROGRESS;
            break;
        case TAF_PA_SUSPENDED:
            return LE_SUSPENDED;
            break;
        default:
            LE_FATAL("Unknown PA return code: %d.", paResult);
            break;
    }
}

#endif /* #ifndef TAFPARESULTCONVERT_HPP */
