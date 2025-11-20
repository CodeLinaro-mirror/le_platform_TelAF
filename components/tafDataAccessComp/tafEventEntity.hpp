/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_EVENT_ENTITY_HPP
#define TAF_EVENT_ENTITY_HPP
#include <ctime>
#include <string>

#include "legato.h"
#include "interfaces.h"

namespace taf{
namespace dataAccess{

    class EventEntity {
        public:
            EventEntity();
            EventEntity(int32_t eventId);
            EventEntity(int32_t eventId, const char *eventName, int32_t dtc, int32_t status,
                    int32_t testFailedCounter, std::time_t createTime, std::time_t updateTime);
            ~EventEntity() {};

            int32_t GetEventId();
            void SetEventId(int32_t eventId);

            const char *GetEventName();
            void SetEventName(const char *eventName);

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
            std::string mEventName;
            int32_t mDtc = -1;
            int32_t mStatus = -1;
            int32_t mTestFailedCounter = -1;
            std::time_t mCreateTime = 0;
            std::time_t mUpdateTime = 0;
    };
}
}
#endif