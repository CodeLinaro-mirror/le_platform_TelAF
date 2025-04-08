/*
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

#endif
