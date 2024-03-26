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
#ifndef TAF_EVENT_ENTITY_HPP
#define TAF_EVENT_ENTITY_HPP
#include <ctime>

#include "legato.h"
#include "interfaces.h"

namespace taf{
namespace dataAccess{

    class EventEntity {
        public:
            EventEntity();
            EventEntity(int32_t eventId);
            EventEntity(int32_t eventId, int32_t dtc, int32_t status, int32_t testFailedCounter,
                    std::time_t createTime, std::time_t updateTime);
            ~EventEntity() {};

            int32_t GetEventId();
            void SetEventId(int32_t eventId);

            int32_t GetEventDtc();
            void SetEventDtc(int32_t dtc);

            int32_t GetEventStatus();
            void SetEventStatus(int32_t status);

            int32_t GetTestFailedCounter();
            void SetTestFailedCounter(int32_t counter);

            std::time_t GetCreateTime();
            void SetCreateTime(std::time_t time);

            std::time_t GetUpdateTime();
            void SetUpdateTime(std::time_t time);
        private:
            // Event entity table content.
            int32_t mEventId = -1;  // Primary key.
            int32_t mDtc = -1;
            int32_t mStatus = -1;
            int32_t mTestFailedCounter = -1;
            std::time_t mCreateTime = 0;
            std::time_t mUpdateTime = 0;
    };
}
}
#endif