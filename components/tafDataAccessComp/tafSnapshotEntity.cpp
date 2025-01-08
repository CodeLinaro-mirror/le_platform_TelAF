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