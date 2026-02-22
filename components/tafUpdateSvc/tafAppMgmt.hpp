/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

        /**
         * Returns the singleton instance of taf_AppMgmt.
         *
         * @return
         *  - Reference to the singleton taf_AppMgmt instance.
         */
        static taf_AppMgmt &GetInstance
        (
            void
        );

        /**
         * Initializes the app management service.
         *
         * @note This helper has no return value.
         */
        void Init
        (
            void
        );

        /**
         * Checks whether the given app is installed.
         *
         * @return
         *  - true  -- The app is installed.
         *  - false -- The app is not installed.
         */
        bool IsAppExist
        (
            const char* appName ///< [IN] App name to check.
        );

        /**
         * Checks whether the given app is configured for manual start.
         *
         * @return
         *  - true  -- The app is configured for manual start.
         *  - false -- The app is configured for automatic start.
         */
        bool IsStartManual
        (
            const char* appName ///< [IN] App name to check.
        );

        /**
         * Checks whether the given app is activated.
         *
         * @return
         *  - true  -- The app is activated.
         *  - false -- The app is not activated.
         */
        bool IsActivated
        (
            const char* appName ///< [IN] App name to check.
        );

        /**
         * Checks whether the given app is a system app.
         *
         * @return
         *  - true  -- The app is a system app.
         *  - false -- The app is not a system app.
         */
        bool IsSysApp
        (
            const char* appName ///< [IN] App name to check.
        );

        /**
         * Checks whether an app version string contains only digits and dot separators.
         *
         * @return
         *  - true  -- if app version is valid.
         *  - false -- if app version is invalid.
         */
        bool IsValidVersion
        (
            const char* appName ///< [IN] App name to check.
        );

        /**
         * Determines whether the given package can be installed over the current app version.
         *
         * @return
         *  - true  -- if the package is valid for installation.
         *  - false -- if the package is invalid or its version is not newer than the installed version.
         */
        bool IsValidToInstall
        (
            const char* appName, ///< [IN] App name to check.
            const char* appPath  ///< [IN] Path to the app package.
        );

        /**
         * Retrieves the stored version string for the given app.
         *
         * @return
         *  - LE_OK        -- The stored version string for the named app is retrieved successfully.
         *  - LE_NOT_FOUND -- No installed app entry matches appName.
         */
        le_result_t GetAppVersion
        (
            const char* appName,           ///< [IN]  App name to query.
            char*       versionPtr,        ///< [OUT] Buffer to receive the version string.
            size_t      versionNumElements ///< [IN]  Size of the version buffer.
        );

        /**
         * Updates the config tree node for the given app.
         *
         * @note This helper has no return value.
         */
        void UpdateAppNode
        (
            const char* appName,   ///< [IN] App name to update.
            bool        activated  ///< [IN] True if the app is activated.
        );

        /**
         * Handles JSON parse events for version parsing.
         *
         * @note This helper has no return value.
         */
        static void VersionEventHandler
        (
            le_json_Event_t event ///< [IN] JSON parse event.
        );

        /**
         * Handles JSON parse events for app info parsing.
         *
         * @note This helper has no return value.
         */
        static void JsonEventHandler
        (
            le_json_Event_t event ///< [IN] JSON parse event.
        );

        /**
         * Handles JSON parse errors.
         *
         * @note This helper has no return value.
         */
        static void JsonErrorHandler
        (
            le_json_Error_t error, ///< [IN] JSON parse error code.
            const char*     msg    ///< [IN] Error message string.
        );

        /**
         * Handles JSON parse completion events.
         *
         * @note This helper has no return value.
         */
        static void JsonParseHandler
        (
            void* contextPtr ///< [IN] Parse context pointer.
        );

        /**
         * JSON parse thread entry point.
         *
         * @note This thread has no return value.
         */
        static void* JsonParseThread
        (
            void* contextPtr ///< [IN] Thread context pointer.
        );

        /**
         * Executes a shell command and checks its exit status.
         *
         * @return
         *  - LE_OK    -- The command exited successfully.
         *  - LE_FAULT -- popen/pclose failed or the command exited with a non-zero status.
         */
        le_result_t SendPipeCmd
        (
            const char* cmd ///< [IN] Shell command to execute.
        );

        /**
         * Copies extended attributes from source to destination.
         *
         * @return
         *  - LE_OK    -- All extended attributes were copied successfully.
         *  - LE_FAULT -- The attribute list cannot be read, an attribute value cannot be retrieved,
         *                or an attribute cannot be written to the destination path.
         */
        le_result_t CopyAttr
        (
            const char* srcPath, ///< [IN] Source path.
            const char* dstPath  ///< [IN] Destination path.
        );

        /**
         * Copies a symlink target to the destination path.
         *
         * @return
         *  - LE_OK    -- The symlink target is resolved and copied successfully.
         *  - LE_FAULT -- The symlink cannot be read, its target metadata cannot be queried, the
         *                target type is unsupported, or the target copy operation fails.
         */
        le_result_t CopySymlink
        (
            const char* srcLink, ///< [IN] Source symlink path.
            const char* dstPath  ///< [IN] Destination path.
        );

        /**
         * Copies a regular file to the destination path.
         *
         * @return
         *  - LE_OK    -- The file content and metadata are copied successfully.
         *  - LE_FAULT -- Source status lookup fails, either file cannot be opened/created, permissions
         *                or ownership cannot be applied, extended attributes cannot be copied, or the
         *                content copy fails.
         */
        le_result_t CopyFile
        (
            const char* srcFile, ///< [IN] Source file path.
            const char* dstFile  ///< [IN] Destination file path.
        );

        /**
         * Copies a directory to the destination path.
         *
         * @return
         *  - LE_OK    -- The destination directory is created and its metadata is copied successfully.
         *  - LE_FAULT -- Source status lookup fails, destination creation fails, permissions/ownership
         *                cannot be applied, or extended attributes cannot be copied.
         */
        le_result_t CopyDir
        (
            const char* srcDir, ///< [IN] Source directory path.
            const char* dstDir  ///< [IN] Destination directory path.
        );

        /**
         * Recursively copies a directory tree.
         *
         * @return
         *  - LE_OK    -- The directory tree is copied successfully.
         *  - LE_FAULT -- The destination path cannot be built, the root or a child entry cannot be
         *                copied, or an unexpected filesystem entry type is encountered during traversal.
         */
        le_result_t RecursiveCopy
        (
            const char* srcDir, ///< [IN] Source directory path.
            const char* dstDir  ///< [IN] Destination directory path.
        );

        /**
         * Recursively removes a directory tree.
         *
         * @return
         *  - LE_OK    -- The directory tree was removed successfully.
         *  - LE_FAULT -- One or more entries could not be removed.
         */
        le_result_t RecursiveRemove
        (
            const char* srcDir ///< [IN] Directory path to remove recursively.
        );

        /**
         * Creates a backup bundle for an installed app.
         *
         * @return
         *  - LE_OK    -- The installed app content is copied, packed, annotated with metadata, and the
         *                temporary backup directory is removed successfully.
         *  - LE_FAULT -- The app tree cannot be copied, the bundle cannot be generated, the bundle
         *                file cannot be reopened, or the temporary backup directory cannot be cleaned up.
         */
        le_result_t BackupApp
        (
            const char* appName ///< [IN] App name to back up.
        );

        /**
         * Updates the app management progress.
         *
         * @note This helper has no return value.
         */
        void UpdateProgress
        (
            taf_update_State_t state,   ///< [IN] Current update state.
            uint32_t           percent, ///< [IN] Progress percentage.
            taf_update_Error_t error    ///< [IN] Error code.
        );

        /**
         * Handles probation timer expiry events.
         *
         * @note This helper has no return value.
         */
        static void ProbationTimerHandler
        (
            le_timer_Ref_t timerRef ///< [IN] Probation timer reference.
        );

        /**
         * Handles Legato app install state changes.
         *
         * @note This helper has no return value.
         */
        static void InstallHandler
        (
            le_update_State_t state,     ///< [IN] Legato update state.
            uint              percent,   ///< [IN] Progress percentage.
            void*             contextPtr ///< [IN] Context pointer.
        );

        /**
         * Handles app update requests in the update event loop.
         *
         * @note This helper has no return value.
         */
        static void AppUpdateHandler
        (
            void* reqPtr ///< [IN] Pointer to the update request.
        );

        /**
         * App update thread entry point.
         *
         * @note This thread has no return value.
         */
        static void* AppUpdateThread
        (
            void* contextPtr ///< [IN] Thread context pointer.
        );

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