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

#ifndef TAFAPPMGMT_HPP
#define TAFAPPMGMT_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"

#define TAF_APPMGMT_SYSTEM_APPS "system:/apps"
#define TAF_APPMGMT_SYSTEM_APPS_NODE "system:/apps/%s"
#define TAF_APPMGMT_TELAF_APPS "/legato/apps"
#define TAF_APPMGMT_TELAF_APPS_NODE "/legato/apps/%s"
#define TAF_APPMGMT_APP_INSTALL_PATH_PREFIX "/data/images/app_"
#define TAF_APPMGMT_APP_BACKUP_DIR "/data/images"
#define TAF_APPMGMT_APP_BACKUP_NODE "/data/images/%s"
#define TAF_APPMGMT_APP_BACKUP_PATH "/data/images/app_%s.backup"

#define TAF_APPMGMT_APP_LISTS_MAX_NUM 1
#define TAF_APPMGMT_APP_MAX_NUM 128

#define TAF_APPMGMT_TELAF_BACKUP_PATH_MAX 256
#define TAF_APPMGMT_JSON_PARSE_TIMEOUT 10

#define TAF_APPMGMT_MAX_XATTR_LIST_SIZE 4096
#define TAF_APPMGMT_MAX_XATTR_VALUE_SIZE 4096

typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_AppMgmtAppInfoSafeRef_t;

typedef struct
{
    char name[TAF_APPMGMT_APP_NAME_BYTES];
    char version[TAF_APPMGMT_APP_VERSION_BYTES];
    char hash[TAF_APPMGMT_APP_HASH_BYTES];
    bool isStartManual;
    bool isSandboxed;
    bool isActivated;
    le_sls_Link_t link;
} taf_AppMgmtAppInfo_t;

typedef struct
{
    le_sls_List_t appList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_AppMgmtAppList_t;

// App update event
typedef enum
{
    TAF_APPMGMT_EV_INSTALL,
    TAF_APPMGMT_EV_PROBATION,
    TAF_APPMGMT_EV_ROLLBACK
} taf_AppMgmtUpdateEvent_t;

// App update request
typedef struct
{
    taf_AppMgmtUpdateEvent_t event;
    char appName[TAF_APPMGMT_APP_NAME_BYTES];
} taf_AppMgmtUpdateReq_t;

namespace telux {
namespace tafsvc {
    class taf_AppMgmt : public ITafSvc {
    public:
        taf_AppMgmt() {};
        ~taf_AppMgmt() {};

        static taf_AppMgmt &GetInstance();
        void Init(void);

        bool IsAppExist(const char* appName);
        bool IsStartManual(const char* appName);
        bool IsActivated(const char* appName);
        bool IsSysApp(const char* appName);
        bool IsValidVersion(const char* appName);
        bool IsValidToInstall(const char* appName, const char* appPath);
        le_result_t GetAppVersion(const char* appName, char* versionPtr,
            size_t versionNumElements);
        void UpdateAppNode(const char* appName, bool activated);

        static void VersionEventHandler(le_json_Event_t event);
        static void JsonEventHandler(le_json_Event_t event);
        static void JsonErrorHandler(le_json_Error_t error, const char* msg);
        static void JsonParseHandler(void* contextPtr);
        static void* JsonParseThread(void* contextPtr);

        le_result_t SendPipeCmd(const char* cmd);
        le_result_t CopyAttr(const char* srcPath, const char* dstPath);
        le_result_t CopySymlink(const char* srcLink, const char* dstPath);
        le_result_t CopyFile(const char* srcFile, const char* dstFile);
        le_result_t CopyDir(const char* srcDir, const char* dstDir);
        le_result_t RecursiveCopy(const char* srcDir, const char* dstDir);
        le_result_t RecursiveRemove(const char* srcDir);
        le_result_t BackupApp(const char* appName);

        void UpdateProgress(taf_update_State_t state, uint32_t percent, taf_update_Error_t error);
        static void ProbationTimerHandler(le_timer_Ref_t timerRef);
        static void InstallHandler(le_update_State_t state, uint percent,void* contextPtr);

        static void AppUpdateHandler(void* reqPtr);
        static void* AppUpdateThread(void* contextPtr);
        static le_event_Id_t appUpdateEvId;

        le_mem_PoolRef_t appListPool;
        le_mem_PoolRef_t appInfoPool;
        le_mem_PoolRef_t appInfoSafeRefPool;

        le_ref_MapRef_t appListRefMap;
        le_ref_MapRef_t appInfoSafeRefMap;

        le_timer_Ref_t prbtTimerRef;
        uint32_t prbtTime = 0;

        taf_update_State_t state = TAF_UPDATE_IDLE;
        int jsonFd = -1;
        le_sem_Ref_t jsonSem;
        static le_event_Id_t jsonParseEvId;
        char appName[TAF_APPMGMT_APP_NAME_BYTES] = {0};
        char appVersion[TAF_APPMGMT_APP_VERSION_BYTES] = {0};
    };
}
}

#endif
