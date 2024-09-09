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