/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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