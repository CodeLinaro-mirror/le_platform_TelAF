/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSnapshotEntity.hpp"

using namespace taf::dataAccess;

SnapshotEntity::SnapshotEntity
(
)
{
}

SnapshotEntity::SnapshotEntity
(
    int32_t id
)
{
    mId = id;
}

SnapshotEntity::SnapshotEntity
(
    int32_t id,
    int32_t dtc,
    int32_t did,
    uint8_t didVal[],
    int32_t didValLen,
    int32_t recNum,
    std::time_t createTime,
    std::time_t updateTime
)
{
    mId = id;
    mDtc = dtc;
    mDid = did;

    mDidValLen = (didValLen > DATA_ACCESS_DID_DATA_SIZE_MAX) ?
        DATA_ACCESS_DID_DATA_SIZE_MAX : didValLen;

    memcpy(mDidVal, didVal, mDidValLen);

    mRecNum = recNum;
    mCreateTime = createTime;
    mUpdateTime = updateTime;
}

int32_t SnapshotEntity::GetId
(
)
{
    return mId;
}

void SnapshotEntity::SetId
(
    int32_t id
)
{
    mId = id;
}

int32_t SnapshotEntity::GetDtc
(
)
{
    return mDtc;
}

void SnapshotEntity::SetDtc
(
    int32_t dtc
)
{
    mDtc = dtc;
}

int32_t SnapshotEntity::GetDid
(
)
{
    return mDid;
}

void SnapshotEntity::SetDid
(
    int32_t did
)
{
    mDid = did;
}

void SnapshotEntity::GetDidValue
(
    uint8_t *didVal,
    int32_t &didValLen
)
{
    if (didValLen < mDidValLen)
    {
        // The buffer is not enough.
        memcpy(didVal, mDidVal, didValLen);
        return;
    }

    memcpy(didVal, mDidVal, mDidValLen);
    didValLen = mDidValLen;
}

void SnapshotEntity::SetDidValue
(
    const uint8_t didVal[],
    int32_t didValLen
)
{
    if ((didVal == NULL) || (didValLen <= 0))
    {
        LE_ERROR("Unknow DID value");
        return;
    }

    mDidValLen = didValLen > DATA_ACCESS_DID_DATA_SIZE_MAX ?
        DATA_ACCESS_DID_DATA_SIZE_MAX : didValLen;

    memcpy(mDidVal, didVal, mDidValLen);
}

int32_t SnapshotEntity::GetRecordNum
(
)
{
    return mRecNum;
}

void SnapshotEntity::SetRecordNum
(
    int32_t recNum
)
{
    mRecNum = recNum;
}

std::time_t SnapshotEntity::GetCreateTime
(
)
{
    return mCreateTime;
}

void SnapshotEntity::SetCreateTime
(
    std::time_t time
)
{
    mCreateTime = time;
}

std::time_t SnapshotEntity::GetUpdateTime
(
)
{
    return mUpdateTime;
}

void SnapshotEntity::SetUpdateTime
(
    std::time_t time
)
{
    mUpdateTime = time;
}