/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSSVCCOMMON_HPP_
#define TAFIVSSSVCCOMMON_HPP_

#include "legato.h"
#include "interfaces.h"
#include <CommonAPI/CommonAPI.hpp>

using namespace v0::com::qualcomm::qti::modem;

inline CommonTypes::OnOffType OnoffLeToIvss(le_onoff_t onoff)
{
    CommonTypes::OnOffType ret = CommonTypes::OnOffType::OFF;
    switch (onoff)
    {
        case LE_OFF:
            ret = CommonTypes::OnOffType::OFF;
            break;
        case LE_ON:
            ret = CommonTypes::OnOffType::ON;
            break;
        default:
            LE_ERROR("OnoffLeToIvss : Unsupported input (%d)", static_cast<int>(onoff));
            break;
    }
    return ret;
}

inline le_onoff_t OnoffIvssToLe(CommonTypes::OnOffType onoff)
{
    le_onoff_t ret = LE_OFF;
    switch (onoff)
    {
        case CommonTypes::OnOffType::OFF:
            ret = LE_OFF;
            break;
        case CommonTypes::OnOffType::ON:
            ret = LE_ON;
            break;
        default:
            LE_ERROR("OnoffIvssToLe : Unsupported input (%d)", static_cast<int>(onoff));
            break;
    }
    return ret;
}

inline CommonTypes::Result ResultLeToIvss(le_result_t result)
{
    CommonTypes::Result ret = CommonTypes::Result::FAULT;
    switch (result)
    {
        case LE_OK:
            ret = CommonTypes::Result::OK;
            break;
        case LE_NOT_FOUND:
            ret = CommonTypes::Result::NOT_FOUND;
            break;
        case LE_OUT_OF_RANGE:
            ret = CommonTypes::Result::OUT_OF_RANGE;
            break;
        case LE_NO_MEMORY:
            ret = CommonTypes::Result::NO_MEMORY;
            break;
        case LE_NOT_PERMITTED:
            ret = CommonTypes::Result::NOT_PERMITTED;
            break;
        case LE_FAULT:
            ret = CommonTypes::Result::FAULT;
            break;
        case LE_COMM_ERROR:
            ret = CommonTypes::Result::COMM_ERROR;
            break;
        case LE_TIMEOUT:
            ret = CommonTypes::Result::TIMEOUT;
            break;
        case LE_OVERFLOW:
            ret = CommonTypes::Result::OVERFLOW;
            break;
        case LE_UNDERFLOW:
            ret = CommonTypes::Result::UNDERFLOW;
            break;
        case LE_WOULD_BLOCK:
            ret = CommonTypes::Result::WOULD_BLOCK;
            break;
        case LE_DEADLOCK:
            ret = CommonTypes::Result::DEADLOCK;
            break;
        case LE_FORMAT_ERROR:
            ret = CommonTypes::Result::FORMAT_ERROR;
            break;
        case LE_DUPLICATE:
            ret = CommonTypes::Result::DUPLICATE;
            break;
        case LE_BAD_PARAMETER:
            ret = CommonTypes::Result::BAD_PARAMETER;
            break;
        case LE_CLOSED:
            ret = CommonTypes::Result::CLOSED;
            break;
        case LE_BUSY:
            ret = CommonTypes::Result::BUSY;
            break;
        case LE_UNSUPPORTED:
            ret = CommonTypes::Result::UNSUPPORTED;
            break;
        case LE_IO_ERROR:
            ret = CommonTypes::Result::IO_ERROR;
            break;
        case LE_NOT_IMPLEMENTED:
            ret = CommonTypes::Result::NOT_IMPLEMENTED;
            break;
        case LE_UNAVAILABLE:
            ret = CommonTypes::Result::UNAVAILABLE;
            break;
        case LE_TERMINATED:
            ret = CommonTypes::Result::TERMINATED;
            break;
        case LE_IN_PROGRESS:
            ret = CommonTypes::Result::IN_PROGRESS;
            break;
        case LE_SUSPENDED:
            ret = CommonTypes::Result::SUSPENDED;
            break;
        default:
            LE_ERROR("ResultLeToIvss : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

inline le_result_t ResultIvssToLe(CommonTypes::Result result)
{
    le_result_t ret = LE_FAULT;
    switch (result)
    {
        case CommonTypes::Result::OK:
            ret = LE_OK;
            break;
        case CommonTypes::Result::NOT_FOUND:
            ret = LE_NOT_FOUND;
            break;
        case CommonTypes::Result::OUT_OF_RANGE:
            ret = LE_OUT_OF_RANGE;
            break;
        case CommonTypes::Result::NO_MEMORY:
            ret = LE_NO_MEMORY;
            break;
        case CommonTypes::Result::NOT_PERMITTED:
            ret = LE_NOT_PERMITTED;
            break;
        case CommonTypes::Result::FAULT:
            ret = LE_FAULT;
            break;
        case CommonTypes::Result::COMM_ERROR:
            ret = LE_COMM_ERROR;
            break;
        case CommonTypes::Result::TIMEOUT:
            ret = LE_TIMEOUT;
            break;
        case CommonTypes::Result::OVERFLOW:
            ret = LE_OVERFLOW;
            break;
        case CommonTypes::Result::UNDERFLOW:
            ret = LE_UNDERFLOW;
            break;
        case CommonTypes::Result::WOULD_BLOCK:
            ret = LE_WOULD_BLOCK;
            break;
        case CommonTypes::Result::DEADLOCK:
            ret = LE_DEADLOCK;
            break;
        case CommonTypes::Result::FORMAT_ERROR:
            ret = LE_FORMAT_ERROR;
            break;
        case CommonTypes::Result::DUPLICATE:
            ret = LE_DUPLICATE;
            break;
        case CommonTypes::Result::BAD_PARAMETER:
            ret = LE_BAD_PARAMETER;
            break;
        case CommonTypes::Result::CLOSED:
            ret = LE_CLOSED;
            break;
        case CommonTypes::Result::BUSY:
            ret = LE_BUSY;
            break;
        case CommonTypes::Result::UNSUPPORTED:
            ret = LE_UNSUPPORTED;
            break;
        case CommonTypes::Result::IO_ERROR:
            ret = LE_IO_ERROR;
            break;
        case CommonTypes::Result::NOT_IMPLEMENTED:
            ret = LE_NOT_IMPLEMENTED;
            break;
        case CommonTypes::Result::UNAVAILABLE:
            ret = LE_UNAVAILABLE;
            break;
        case CommonTypes::Result::TERMINATED:
            ret = LE_TERMINATED;
            break;
        case CommonTypes::Result::IN_PROGRESS:
            ret = LE_IN_PROGRESS;
            break;
        case CommonTypes::Result::SUSPENDED:
            ret = LE_SUSPENDED;
            break;
        default:
            LE_ERROR("ResultIvssToLe : Unsupported input (%d)", static_cast<int>(result));
            break;
    }
    return ret;
}

inline CommonTypes::PhoneId PhoneIdUint8ToIvss(uint8_t phoneId)
{
    CommonTypes::PhoneId ret = CommonTypes::PhoneId::PHONE_ID_1;
    switch (phoneId)
    {
        case 1:
            ret = CommonTypes::PhoneId::PHONE_ID_1;
            break;
        case 2:
            ret = CommonTypes::PhoneId::PHONE_ID_2;
            break;
        default:
            LE_ERROR("PhoneIdUint8ToIvss : Unsupported input (%d)", static_cast<int>(phoneId));
            break;
    }
    return ret;
}

inline uint8_t PhoneIdIvssToUint8(CommonTypes::PhoneId phoneId)
{
    uint8_t ret = 1;
    switch (phoneId)
    {
        case CommonTypes::PhoneId::PHONE_ID_1:
            ret = 1;
            break;
        case CommonTypes::PhoneId::PHONE_ID_2:
            ret = 2;
            break;
        default:
            LE_ERROR("PhoneIdIvssToUint8 : Unsupported input (%d)", static_cast<int>(phoneId));
            break;
    }
    return ret;
}

#endif // TAFIVSSSVCCOMMON_HPP_
