/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDIDEntity.hpp"

using namespace taf::dataAccess;

DIDEntity::DIDEntity
(
    int32_t did
)
{
    mDid = did;
}

DIDEntity::DIDEntity
(
    int32_t did,
    const uint8_t *value,
    int32_t valueLen,
    std::time_t createTime,
    std::time_t updateTime
)
{
    mDid = did;
    if (value != nullptr && valueLen > 0)
    {
        mDIDValue.assign(value, value + valueLen);
    }
    mCreateTime = createTime;
    mUpdateTime = updateTime;
}

int32_t DIDEntity::GetDid
(
) const
{
    return mDid;
}

void DIDEntity::SetDid
(
    int32_t did
)
{
    mDid = did;
}

const std::vector<uint8_t>& DIDEntity::GetDIDValue
(
) const
{
    return mDIDValue;
}

void DIDEntity::SetDIDValue
(
    const uint8_t *value,
    int32_t len
)
{
    if (value != nullptr && len > 0)
    {
        mDIDValue.assign(value, value + len);
    }
    else
    {
        mDIDValue.clear();
    }
}

int32_t DIDEntity::GetDIDValueLen
(
) const
{
    return static_cast<int32_t>(mDIDValue.size());
}

std::time_t DIDEntity::GetCreateTime
(
) const
{
    return mCreateTime;
}

void DIDEntity::SetCreateTime
(
    std::time_t time
)
{
    mCreateTime = time;
}

std::time_t DIDEntity::GetUpdateTime
(
) const
{
    return mUpdateTime;
}

void DIDEntity::SetUpdateTime
(
    std::time_t time
)
{
    mUpdateTime = time;
}
