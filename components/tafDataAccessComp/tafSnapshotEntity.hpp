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