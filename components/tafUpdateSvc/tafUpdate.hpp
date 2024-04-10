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

#include <string>

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafPiDA.h"
#include "tafPiUA.h"

#define TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH "/data/images/app_%s"

#define TAF_UPDATE_THREAD_STACK_SIZE 0x20000

#define TAF_UPDATE_SESSION_NUM 5

#define TAF_UPDATE_QOTA_HEADER_SIZE 48
#define TAF_UPDATE_QOTA_MAGIC_SIZE 4

#define TAF_UPDATE_RW_BUFFER_SIZE 4096

#define TAF_UPDATE_TIMER_INTERVAL 1000

//--------------------------------------------------------------------------------------------------
/**
 * Download event enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    TAF_UPDATE_DL_START,
    TAF_UPDATE_DL_PAUSE,
    TAF_UPDATE_DL_RESUME,
    TAF_UPDATE_DL_CANCEL
} taf_UpdateDlEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update event enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    TAF_UPDATE_INST_START,
    TAF_UPDATE_INST_CANCEL
} taf_UpdateEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update session type enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD,
    TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE,
    TAF_UPDATE_SESSION_TYPE_QOTA_PARSE,
    TAF_UPDATE_SESSION_TYPE_FW_UPDATE,
    TAF_UPDATE_SESSION_TYPE_APP_UPDATE
} taf_UpdateSessionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_timer_Ref_t timerRef;
    taf_pi_da_SessionRef_t sessRef;
    taf_update_State_t state;
    int percent;
    int error;
} taf_UpdateDownloadSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update Plug-In session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_timer_Ref_t timerRef;
    taf_pi_ua_SessionRef_t sessRef;
    taf_update_State_t state;
    int percent;
    int error;
} taf_UpdatePlugInSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Firmware install session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateFwInstallSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download request structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateDlEvent_t event;
    taf_UpdateDownloadSession_t* sessPtr;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateDlReq_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update request structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateEvent_t event;
    taf_UpdatePlugInSession_t* sessPtr;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateReq_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateSessionType_t sessType;
    union
    {
        taf_UpdateDownloadSession_t dlSess;
        taf_UpdatePlugInSession_t upiSess;
        taf_UpdateFwInstallSession_t fwSess;
    };
} taf_UpdateSession_t;

namespace telux {
namespace tafsvc {
    class taf_Update : public ITafSvc {
    public:
        taf_Update() {};
        ~taf_Update() {};

        static taf_Update &GetInstance();

        le_result_t CheckQotaHeader(const char* file);
        le_result_t RemoveQotaHeader(const char* file);

        void ReportDownloadStatus(taf_UpdateDownloadSession_t* sessPtr, taf_update_State_t state);
        void ReportUpdateStatus(taf_UpdatePlugInSession_t* sessPtr, taf_update_State_t state);
        static void DownloadTimerHandler(le_timer_Ref_t timerRef);
        static void UpdateTimerHandler(le_timer_Ref_t timerRef);

        static void StateLayeredHandler(void* reportPtr, void* layerHandlerFunc);

        static void DownloadHandler(void* reqPtr);
        static void UpdateHandler(void* reqPtr);

        void Init(void);

        da_Inf_t* daInfPtr;
        ua_Inf_t* uaInfPtr;

        le_mem_PoolRef_t sessionPool;
        le_ref_MapRef_t sessionMap;

        taf_update_SessionRef_t dlSessRef;
        taf_update_SessionRef_t upiSessRef;
        taf_update_SessionRef_t qotaSessRef;
        taf_update_SessionRef_t fwSessRef;
        taf_update_SessionRef_t appSessRef;

        le_event_Id_t stateEvId;
        le_event_Id_t downloadEvId;
        le_event_Id_t updatePiEvId;
    };
}
}

#endif
