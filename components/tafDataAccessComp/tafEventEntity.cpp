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
#include "tafEventEntity.hpp"

using namespace taf::dataAccess;

EventEntity::EventEntity
(
)
{
}

EventEntity::EventEntity
(
    int32_t eventId
)
{
    mEventId = eventId;
}

EventEntity::EventEntity
(
    int32_t eventId,
    int32_t dtc,
    int32_t status,
    int32_t testFailedCounter,
    std::time_t createTime,
    std::time_t updateTime
)
{
    mEventId = eventId;
    mDtc = dtc;
    mStatus = status;
    mTestFailedCounter = testFailedCounter;
    mCreateTime = createTime;
    mUpdateTime = updateTime;
}

int32_t EventEntity::GetEventId
(
)
{
    return mEventId;
}

void EventEntity::SetEventId
(
    int32_t eventId
)
{
    mEventId = eventId;
}

int32_t EventEntity::GetEventDtc
(
)
{
    return mDtc;
}

void EventEntity::SetEventDtc
(
    int32_t dtc
)
{
    mDtc = dtc;
}

int32_t EventEntity::GetEventStatus
(
)
{
    return mStatus;
}

void EventEntity::SetEventStatus
(
    int32_t status
)
{
    mStatus = status;
}

int32_t EventEntity::GetTestFailedCounter
(
)
{
    return mTestFailedCounter;
}

void EventEntity::SetTestFailedCounter
(
    int32_t counter
)
{
    mTestFailedCounter = counter;
}

std::time_t EventEntity::GetCreateTime
(
)
{
    return mCreateTime;
}

void EventEntity::SetCreateTime
(
    std::time_t time
)
{
    mCreateTime = time;
}

std::time_t EventEntity::GetUpdateTime
(
)
{
    return mUpdateTime;
}

void EventEntity::SetUpdateTime
(
    std::time_t time
)
{
    mUpdateTime = time;
}