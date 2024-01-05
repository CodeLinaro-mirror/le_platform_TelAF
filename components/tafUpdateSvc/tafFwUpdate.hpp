/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAFFWUPDATE_HPP
#define TAFFWUPDATE_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"

#define TAF_FWUPDATE_INSTALL_CMD_LEN 256
// Data proccessing rate is about 3.84 MB/s.
#define TAF_FWUPDATE_PROC_DATA_RATE 4035394

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
// 33s for OTA start message and 17s for OTA end message.
#define TAF_FWUPDATE_PROC_MRC_TIME 50
// 98s for OTA sync message.
#define TAF_FWUPDATE_MRC_SYNC_TIME 98
#else
#define TAF_FWUPDATE_PROC_MRC_TIME 0
#define TAF_FWUPDATE_MRC_SYNC_TIME 0
#endif

#define TAF_FWUPDATE_RECOVERY_LOG_FILE "/tmp/recovery.log"
#define TAF_FWUPDATE_VERSION_FILE "/etc/version"

#define TAF_FWUPDATE_FOTA_STATE "/fotaState"

// Firmware update event
typedef enum {
    TAF_FWUPDATE_EV_INSTALL,
    TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE
} taf_FwUpdateEvent_t;

// Timer options
typedef enum {
    TAF_FWUPDATE_TIMER_OP_INST_START,
    TAF_FWUPDATE_TIMER_OP_INST_STOP,
    TAF_FWUPDATE_TIMER_OP_PRBT_START,
    TAF_FWUPDATE_TIMER_OP_PRBT_STOP
} taf_FwUpdateTimerOp_t;

// Firmware update request
typedef struct {
    taf_FwUpdateEvent_t event;
    char name[TAF_UPDATE_MAX_PKG_NAME_LEN];
} taf_FwUpdateReq_t;

namespace telux {
namespace tafsvc {
    class taf_FwUpdate : public ITafSvc {
    public:
        taf_FwUpdate() {};
        ~taf_FwUpdate() {};

        static taf_FwUpdate &GetInstance();

        le_result_t SendPipeCmd(const char* cmd, const char* mod);

        taf_update_State_t GetState();
        void SetState(taf_update_State_t state);

        void ReportStatus(taf_update_State_t state, uint32_t percent);
        void UpdateProgress(taf_update_State_t state);

        static void ProbationTimerHandler(le_timer_Ref_t timerRef);
        static void InstallTimerHandler(le_timer_Ref_t timerRef);

        void InstallFirmware(const char* filePath);
        void Init(void);

        static void FwUpdateHandler(void* reqPtr);
        static void* FwUpdateThread(void* contextPtr);

        static void TimerOpHandler(void* contextPtr);
        static void* TimerThread(void* contextPtr);

        static le_event_Id_t fwUpdateEvId;
        static le_event_Id_t fwTimerEvId;

        le_timer_Ref_t prbtTimerRef;
        le_timer_Ref_t instTimerRef;
        uint32_t prbtTime = 0;
        uint32_t percent = 0;
        uint32_t totalTime = 0;

        bool autoSync = true;
        char filePath[TAF_UPDATE_MAX_PKG_NAME_LEN];
    };
}
}

#endif
