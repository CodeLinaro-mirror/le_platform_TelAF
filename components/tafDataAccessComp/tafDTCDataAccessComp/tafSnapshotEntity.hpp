/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_SNAPSHOT_ENTITY_HPP
#define TAF_SNAPSHOT_ENTITY_HPP
#include <ctime>

#include "legato.h"
#include "interfaces.h"
#include "tafDataAccessComp.h"

namespace taf{
namespace dataAccess{

    class SnapshotEntity {
        public:
            SnapshotEntity();
            SnapshotEntity(int32_t id);
            SnapshotEntity(int32_t id, int32_t dtc, int32_t did, uint8_t didVal[],
                    int32_t didValLen, int32_t recNum, std::time_t createTime,
                    std::time_t updateTime);
            ~SnapshotEntity() {};

            int32_t GetId();
            void SetId(int32_t id);

            int32_t GetDtc();
            void SetDtc(int32_t dtc);

            int32_t GetDid();
            void SetDid(int32_t did);

            void GetDidValue(uint8_t *didVal, int32_t &didValLen);
            void SetDidValue(const uint8_t didVal[], int32_t didValLen);

            int32_t GetRecordNum();
            void SetRecordNum(int32_t recNum);

            std::time_t GetCreateTime();
            void SetCreateTime(std::time_t time);

            std::time_t GetUpdateTime();
            void SetUpdateTime(std::time_t time);
        private:
            // Snapshot entity table content.
            int32_t mId = -1;  // Primary key.
            int32_t mDtc = -1;
            int32_t mDid = -1;
            uint8_t mDidVal[DATA_ACCESS_DID_DATA_SIZE_MAX];
            int32_t mDidValLen = 0;
            int32_t mRecNum = -1;
            std::time_t mCreateTime = 0;
            std::time_t mUpdateTime = 0;
    };
}
}
#endif