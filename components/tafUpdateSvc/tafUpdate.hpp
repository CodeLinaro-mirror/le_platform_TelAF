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

#include "legato.h"
#include "interfaces.h"

#include <future>
#include <memory>
#include <map>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/tel/ServingSystemManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#include "tafSvcIF.hpp"
#include "tafUpdatePa.hpp"
#include "tafAppMgmt.hpp"
#include "tafFwUpdate.hpp"

#define TAF_UPDATE_PAKCAGE_FILE_PATH "/data/images/TCU_target"
#define TAF_UPDATE_STATE_FILE "/taf_update_state"

#define TAF_UPDATE_TIME_INTERVAL 1000
#define TAF_UPDATE_THREAD_STACK_SIZE 0x20000
#define TAF_UPDATE_INSTALL_CMD_LEN 50

#define TAF_UPDATE_QOTA_HEADER_SEG_NUM 13
#define TAF_UPDATE_QOTA_HEADER_SIZE 48

#define TAF_UPDATE_DATA_SERVICE_TIME_OUT 20
#define TAF_UPDATE_PROBATION_TIME 60

#define TAF_UPDATE_RW_BUFFER_SIZE 4096

typedef enum
{
    TAF_UPDATE_CMD_TYPE_DOWNLOAD,
    TAF_UPDATE_CMD_TYPE_INSTALL
} taf_UpdateCmdType_t;

typedef struct
{
    const char* name;
    size_t size;
} taf_UpdateQotaHeaderSeg_t;

typedef struct
{
    taf_UpdateCmdType_t cmdType;
    taf_update_Package_t pkgType;
    const char* pkgName;
} taf_UpdateCmdReq_t;

typedef struct
{
    taf_update_State_t state;
    taf_update_Package_t pkgType;
    uint32_t tick;
} taf_UpdateTimerContext;

namespace telux {
namespace tafsvc {

    class taf_UpdateServingSystemListener : public telux::data::IServingSystemListener {
    public:
        std::mutex cv_mutex;
        std::condition_variable conVar;
        telux::data::DataServiceState dsStatus;

        taf_UpdateServingSystemListener(SlotId slot);
        void onServiceStateChanged(telux::data::ServiceStatus status) override;

    private:
        SlotId slotId;
    };

    class taf_UpdateRequestServiceStatusCallback {
    public:
        le_sem_Ref_t semaphore;
        telux::data::ServiceStatus status;
        void requestServiceStatus(telux::data::ServiceStatus serviceStatus, telux::common::ErrorCode error);
    };

    class taf_Update : public ITafSvc {
    public:
        taf_Update() {};
        ~taf_Update() {};

        static taf_Update &GetInstance();

        bool CheckHeader(const char* src, const char* dst, int n);
        le_result_t ParsePackage(const char* file);
        le_result_t RemoveHeader(const char* file);
        static void NameEventHandler(le_json_Event_t event);
        static void JsonEventHandler(le_json_Event_t event);
        static void JsonErrorHandler(le_json_Error_t error, const char* msg);
        le_result_t ParseBundle(const char* file);

        void UpdateSetState(taf_update_State_t state);
        void UpdateGetState(taf_update_State_t* state);

        static void DownloadTimerTick(le_timer_Ref_t timerRef);
        static void ProbationTimerTick(le_timer_Ref_t timerRef);

        static void AppInstallHandler(le_update_State_t updateState, uint percentDone,void* contextPtr);
        static void UpdateStateLayeredHandler(void* reportPtr, void* secondLayerHandlerFunc);
        static void* UpdateCmdThread(void* contextPtr);

        static void UpdateProcCmdHandler(void* cmdReqPtr);

        void onInitCompleted(telux::common::ServiceStatus status);

        void Init(void);

        static le_event_Id_t updateCmdEvId;
        le_event_Id_t updateStateEvId;
        le_timer_Ref_t dlTimerRef;
        le_timer_Ref_t prbtTimerRef;
        taf_UpdateTimerContext dlTimerContext;
        taf_UpdateTimerContext prbtTimerContext;
        taf_update_pa_SessionRef_t daSessionID;
        std::map<std::string, char*> qotaHeader;
        taf_update_Package_t pkgType;
        char sotaAppName[TAF_APPMGMT_APP_NAME_BYTES];
        int sotaFd;

    private:
        bool subSystemStatusUpdated;
        std::mutex mtx;
        std::condition_variable conVar;
        std::map<SlotId, std::shared_ptr<telux::data::IServingSystemManager>> dataServingSystemManagers;
        std::map<SlotId, std::shared_ptr<telux::data::IServingSystemListener>> dataServingSystemListeners;
        std::map<SlotId, std::shared_ptr<taf_UpdateServingSystemListener>> updateServingSystemlisteners;
        std::shared_ptr<taf_UpdateRequestServiceStatusCallback> reqSvcStateCb;
    };
}
}

#endif
