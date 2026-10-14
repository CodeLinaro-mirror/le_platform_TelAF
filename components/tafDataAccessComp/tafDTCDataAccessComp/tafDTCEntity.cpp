/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDTCEntity.hpp"

using namespace taf::dataAccess;

DtcEntity::DtcEntity
(
)
{
}

DtcEntity::DtcEntity
(
    int32_t dtc
)
{
    mDtc = dtc;
}

DtcEntity::DtcEntity
(
    int32_t dtc,
    int32_t status,
    int32_t faultOccurCounter,
    int32_t agingCounter,
    int32_t agedCounter,
    std::time_t createTime,
    std::time_t updateTime,
    std::time_t testFailedTime,
    std::time_t confirmedTime
)
{
    mDtc = dtc;
    mStatus = status;
    mFaultOccurCounter = faultOccurCounter;
    mAgingCounter = agingCounter;
    mAgedCounter = agedCounter;
    mCreateTime = createTime;
    mUpdateTime = updateTime;
    mTestFailedTime = testFailedTime;
    mConfirmedTime = confirmedTime;
}

int32_t DtcEntity::GetDtc
(
)
{
    return mDtc;
}

void DtcEntity::SetDtc
(
    int32_t dtc
)
{
    mDtc = dtc;
}

int32_t DtcEntity::GetStatus
(
)
{
    return mStatus;
}

void DtcEntity::SetStatus
(
    int32_t status
)
{
    mStatus = status;
}

int32_t DtcEntity::GetFaultOccurenceCounter
(
)
{
    return mFaultOccurCounter;
}

void DtcEntity::SetFaultOccurenceCounter
(
    int32_t counter
)
{
    mFaultOccurCounter = counter;
}

int32_t DtcEntity::GetAgingCounter
(
)
{
    return mAgingCounter;
}

void DtcEntity::SetAgingCounter
(
    int32_t counter
)
{
    mAgingCounter = counter;
}

int32_t DtcEntity::GetAgedCounter
(
)
{
    return mAgedCounter;
}

void DtcEntity::SetAgedCounter
(
    int32_t counter
)
{
    mAgedCounter = counter;
}

int32_t DtcEntity::GetActivation
(
)
{
    return mActivation;
}

void DtcEntity::SetActivation
(
    int32_t activation
)
{
    mActivation = activation;
}

int32_t DtcEntity::GetSuppression
(
)
{
    return mSuppression;
}

void DtcEntity::SetSuppression
(
    int32_t suppression
)
{
    mSuppression = suppression;
}

std::time_t DtcEntity::GetCreateTime
(
)
{
    return mCreateTime;
}

void DtcEntity::SetCreateTime
(
    std::time_t time
)
{
    mCreateTime = time;
}

std::time_t DtcEntity::GetUpdateTime
(
)
{
    return mUpdateTime;
}

void DtcEntity::SetUpdateTime
(
    std::time_t time
)
{
    mUpdateTime = time;
}

std::time_t DtcEntity::GetTestFailedTime
(
)
{
    return mTestFailedTime;
}

void DtcEntity::SetTestFailedTime
(
    std::time_t time
)
{
    mTestFailedTime = time;
}

std::time_t DtcEntity::GetConfirmedTime
(
)
{
    return mConfirmedTime;
}

void DtcEntity::SetConfirmedTime
(
    std::time_t time
)
{
    mConfirmedTime = time;
}