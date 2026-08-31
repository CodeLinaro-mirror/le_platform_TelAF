/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DTC_ENTITY_HPP
#define TAF_DTC_ENTITY_HPP
#include <ctime>

#include "legato.h"
#include "interfaces.h"

namespace taf{
namespace dataAccess{

    class DtcEntity {
        public:
            DtcEntity();
            DtcEntity(int32_t dtc);
            DtcEntity(int32_t dtc, int32_t status, int32_t faultOccurCounter,
                    int32_t agingCounter, int32_t agedCounter, std::time_t createTime,
                    std::time_t updateTime, std::time_t testFailedTime, std::time_t confirmedTime);
            ~DtcEntity() {};

            int32_t GetDtc();
            void SetDtc(int32_t dtc);

            int32_t GetStatus();
            void SetStatus(int32_t status);

            int32_t GetFaultOccurenceCounter();
            void SetFaultOccurenceCounter(int32_t counter);

            int32_t GetAgingCounter();
            void SetAgingCounter(int32_t counter);

            int32_t GetAgedCounter();
            void SetAgedCounter(int32_t counter);

            int32_t GetActivation();
            void SetActivation(int32_t activation);

            int32_t GetSuppression();
            void SetSuppression(int32_t suppression);

            std::time_t GetCreateTime();
            void SetCreateTime(std::time_t time);

            std::time_t GetUpdateTime();
            void SetUpdateTime(std::time_t time);

            std::time_t GetTestFailedTime();
            void SetTestFailedTime(std::time_t time);

            std::time_t GetConfirmedTime();
            void SetConfirmedTime(std::time_t time);
        private:
            // DTC entity table content.
            int32_t mDtc = -1;  // Primary key.
            int32_t mStatus = -1;
            int32_t mFaultOccurCounter = -1;
            int32_t mAgingCounter = -1;
            int32_t mAgedCounter = -1;
            int32_t mActivation = -1;
            int32_t mSuppression = -1;
            std::time_t mCreateTime = 0;
            std::time_t mUpdateTime = 0;
            std::time_t mTestFailedTime = 0;
            std::time_t mConfirmedTime = 0;
    };
}
}
#endif