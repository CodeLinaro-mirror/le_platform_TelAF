/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * @file       tafUpdate.hpp
 * @brief      Internal interface for Update Service object. The functions
 *             in this file are impletmented internally.
 */

#ifndef TAFUPDATE_HPP
#define TAFUPDATE_HPP

#include <map>
#include <string>

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "taf_pa_update.hpp"

#define TAF_UPDATE_PAKCAGE_FILE_PATH "/data/images/TCU_target"
#define TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH "/data/images/firmware"
#define TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH "/data/images/app_%s"

#define TAF_UPDATE_THREAD_STACK_SIZE 0x20000

#define TAF_UPDATE_TIME_TO_ACCESS_FILE 20
#define TAF_UPDATE_DOWNLOAD_TIME_INTERVAL 200

#define TAF_UPDATE_QOTA_HEADER_SEG_NUM 13
#define TAF_UPDATE_QOTA_HEADER_SIZE 48

#define TAF_UPDATE_RW_BUFFER_SIZE 4096

// User request event
typedef enum {
    TAF_UPDATE_REQ_DOWNLOAD,
    TAF_UPDATE_REQ_INSTALL
} taf_UpdateReqEvent_t;

// Download event
typedef enum {
    TAF_UPDATE_DL_START,
    TAF_UPDATE_DL_PAUSED,
    TAF_UPDATE_DL_RESUME,
} taf_UpdateDlEvent_t;

// QOTA header segment
typedef struct {
    const char* name;
    size_t size;
} taf_UpdateQotaHeaderSeg_t;

// User Request
typedef struct {
    taf_UpdateReqEvent_t event;
    taf_update_OTA_t ota;
    char name[TAF_UPDATE_MAX_PKG_NAME_LEN];
} taf_UpdateUsrReq_t;

// Download request
typedef struct {
    taf_UpdateDlEvent_t event;
} taf_UpdateDlReq_t;

namespace telux {
namespace tafsvc {
    class taf_Update : public ITafSvc {
    public:
        taf_Update() {};
        ~taf_Update() {};

        static taf_Update &GetInstance();

        bool CheckHeader(const char* src, const char* dst, int n);
        le_result_t ParseHeader(const char* file, taf_update_OTA_t* ota);
        le_result_t RemoveHeader(const char* file);
        le_result_t ParsePackage(const char* file);
        static void NameEventHandler(le_json_Event_t event);
        static void JsonEventHandler(le_json_Event_t event);
        static void JsonErrorHandler(le_json_Error_t error, const char* msg);
        le_result_t ParseBundle(const char* file);

        void NotifyDownloadFail();
        static void DownloadTimerHandler(le_timer_Ref_t timerRef);

        static void StateLayeredHandler(void* reportPtr, void* layerHandlerFunc);

        static void DownloadHandler(void* reqPtr);
        static void RequestHandler(void* reqPtr);
        static void* RequestThread(void* contextPtr);

        void Init(void);

        le_event_Id_t stateEvId;
        static le_event_Id_t requestEvId;
        le_event_Id_t downloadEvId;

        le_timer_Ref_t dlTimerRef;
        taf_update_State_t downloadState = TAF_UPDATE_IDLE;
        taf_update_SessionRef_t daSessionID;
        std::map<std::string, char*> qotaHeader;
        int dlAppFd;
        char dlAppName[TAF_APPMGMT_APP_NAME_BYTES];
    };
}
}

#endif
