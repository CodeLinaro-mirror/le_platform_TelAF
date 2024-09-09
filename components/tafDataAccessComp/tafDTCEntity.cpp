/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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