/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <ctime>
#include <vector>
#include <cstdint>

#include "legato.h"
#include "interfaces.h"

namespace taf{
namespace dataAccess {

class DIDEntity {
public:
    DIDEntity() = default;
    DIDEntity(int32_t did);
    DIDEntity(int32_t did, const uint8_t *value, int32_t valueLen,
              std::time_t createTime, std::time_t updateTime);
    ~DIDEntity() = default;

    int32_t GetDid() const;
    void SetDid(int32_t did);

    const std::vector<uint8_t>& GetDIDValue() const;
    void SetDIDValue(const uint8_t *value, int32_t len);
    int32_t GetDIDValueLen() const;

    std::time_t GetCreateTime() const;
    void SetCreateTime(std::time_t time);

    std::time_t GetUpdateTime() const;
    void SetUpdateTime(std::time_t time);

private:
    int32_t mDid{-1};
    std::vector<uint8_t> mDIDValue{};
    std::time_t mCreateTime{0};
    std::time_t mUpdateTime{0};
};

} // namespace dataAccess
}
