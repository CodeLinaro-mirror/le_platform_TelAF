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

#include <chrono>

#include "tafUpdate.hpp"
#include "tafAppMgmt.hpp"

using namespace telux::tafsvc;

le_event_Id_t taf_AppMgmt::appUpdateEvId = nullptr;

/* Pool and map for app list and app reference. */
LE_MEM_DEFINE_STATIC_POOL(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM, sizeof(taf_AppMgmtAppList_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_appMgmt_AppInfo_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_AppMgmtAppInfoSafeRef_t));
LE_REF_DEFINE_STATIC_MAP(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
LE_REF_DEFINE_STATIC_MAP(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);

/*======================================================================
 FUNCTION        taf_AppMgmt::GetInstance
 DESCRIPTION     Get a instance of taf_AppMgmt
 PARAMETERS      void
 RETURN VALUE    taf_AppMgmt: Instance reference
======================================================================*/
taf_AppMgmt &taf_AppMgmt::GetInstance()
{
    static taf_AppMgmt instance;
    return instance;
}

/*======================================================================
 FUNCTION        taf_AppMgmt::CreateAppNode
 DESCRIPTION     Create app node in config tree
 PARAMETERS      [IN] name: App name
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::CreateAppNode(const char* name)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_APPMGMT_SYSTEM_APPS);
    TAF_ERROR_IF_RET_NIL(le_cfg_GoToFirstChild(rdIter) == LE_NOT_FOUND,
        "%s not found.", name);

    do {
        // Get app name
        char appName[TAF_APPMGMT_APP_NAME_BYTES];
        le_cfg_GetNodeName(rdIter, "", appName, TAF_APPMGMT_APP_NAME_BYTES);

        if (strncmp(appName, name, strlen(name)) == 0) {
            LE_INFO("App %s found in current system.", appName);

            // Get app version
            char appVersion[TAF_APPMGMT_APP_VERSION_BYTES] = { 0 };
            le_cfg_GetString(rdIter, "version", appVersion, TAF_APPMGMT_APP_VERSION_BYTES, "");

            // Get app hash
            char appHash[TAF_APPMGMT_APP_HASH_BYTES] = { 0 };
            le_appInfo_GetHash(appName, appHash, TAF_APPMGMT_APP_HASH_BYTES);

            // Get app sandbox state
            bool appSandboxed = le_cfg_GetBool(rdIter, "sandboxed", true);

            // Get app start mode
            auto &tafAppMgmt = taf_AppMgmt::GetInstance();
            tafAppMgmt.isManualStart = le_cfg_GetBool(rdIter, "startManual", false);
            auto &tafUpdate = taf_Update::GetInstance();
            tafUpdate.WriteFs(TAF_APPMGMT_SOTA_APP_START_MODE, (uint8_t*)&tafAppMgmt.isManualStart, sizeof(bool));

            // Create app node
            char node[LE_CFG_STR_LEN_BYTES] = { 0 };
            snprintf(node, sizeof(node), TAF_APPMGMT_UPDATE_APP_NODE, appName);
            le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);

            le_cfg_SetString(wrIter, "version", appVersion);
            le_cfg_SetString(wrIter, "hash", appHash);
            le_cfg_SetBool(wrIter, "sandboxed", appSandboxed);
            le_cfg_SetBool(wrIter, "manual-start", tafAppMgmt.isManualStart);
            le_cfg_SetBool(wrIter, "activated", false);

            le_cfg_CommitTxn(wrIter);

            LE_INFO("App %s updated in config tree.", appName);

            if (tafAppMgmt.isManualStart) {
                LE_INFO("App %s should be started into probation or uninstalled manually.", appName);
            } else {
                LE_INFO("App %s started into probation automatically.", appName);
                taf_AppMgmtUpdateReq_t updateReq;
                updateReq.event = TAF_APPMGMT_EV_START_PROBATION;
                le_event_Report(taf_AppMgmt::appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            }
            break;
        }
    } while (le_cfg_GoToNextSibling(rdIter) == LE_OK);

    le_cfg_CancelTxn(rdIter);
}

/*======================================================================
 FUNCTION        taf_AppMgmt::DeleteAppNode
 DESCRIPTION     Remove app node from config tree
 PARAMETERS      [IN] name: App name
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::DeleteAppNode(const char* name)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_APPMGMT_UPDATE_APPS);
    le_cfg_DeleteNode(wrIter, name);
    le_cfg_CommitTxn(wrIter);
}

/*======================================================================
 FUNCTION        taf_AppMgmt::NotifyProgress
 DESCRIPTION     Notify SOTA progress to user
 PARAMETERS      [IN] state: SOTA state
                 [IN] percent: Percentage of progress
                 [IN] error: Error code of SOTA
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::NotifyProgress(taf_update_State_t state, uint32_t percent, taf_update_Error_t error)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;

    stateInd.ota = TAF_UPDATE_SOTA;
    stateInd.percent = percent;
    stateInd.error = error;
    stateInd.state = state;
    if (tafAppMgmt.sotaState != state) {
        tafAppMgmt.sotaState = state;
        tafUpdate.WriteFs(TAF_APPMGMT_SOTA_STATE, (uint8_t*)&state, sizeof(taf_update_State_t));
    }
    le_utf8_Copy(stateInd.name, tafAppMgmt.sotaApp, TAF_APPMGMT_APP_NAME_BYTES, NULL);
    le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
}

/*======================================================================
 FUNCTION        taf_AppMgmt::ReportState
 DESCRIPTION     Report SOTA result to server
 PARAMETERS      [IN] rState: Report state
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::ReportState(taf_update_ReportState_t rState)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();
    taf_AppMgmtUpdateReq_t updateReq;

    tafAppMgmt.sotaState = TAF_UPDATE_REPORTING;
    tafUpdate.WriteFs(TAF_APPMGMT_SOTA_STATE, (uint8_t*)&tafAppMgmt.sotaState, sizeof(taf_update_State_t));
    updateReq.event = TAF_APPMGMT_EV_START_REPORT;
    tafAppMgmt.rState = rState;
    tafUpdate.WriteFs(TAF_APPMGMT_SOTA_REPORT_STATE, (uint8_t*)&rState, sizeof(taf_update_ReportState_t));
    le_event_Report(taf_AppMgmt::appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
}

/*======================================================================
 FUNCTION        taf_AppMgmt::ProbationTimerHandler
 DESCRIPTION     Probation timer handler for app
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::ProbationTimerHandler(le_timer_Ref_t timerRef)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_update_State_t state = TAF_UPDATE_PROBATION_SUCCESS;
    taf_update_ReportState_t rState = TAF_UPDATE_REPORT_SUCCESS;
    taf_update_Error_t error = TAF_UPDATE_NONE;

    LE_INFO("Probation timer stopped.");

    if (le_appInfo_GetState(tafAppMgmt.sotaApp) == LE_APPINFO_STOPPED) {
        LE_ERROR("App %s is not running.", tafAppMgmt.sotaApp);

        state = TAF_UPDATE_PROBATION_FAIL;
        rState = TAF_UPDATE_REPORT_FAILURE;
        error = TAF_UPDATE_APP_NOT_RUNNING;

        LE_ERROR("Mark bad for current system.");
    } else {
        LE_INFO("App %s probation success.", tafAppMgmt.sotaApp);

        LE_INFO("Mark good for current system.");
        le_updateCtrl_MarkGood(true);

        char node[LE_CFG_STR_LEN_BYTES] = { 0 };
        snprintf(node, sizeof(node), TAF_APPMGMT_UPDATE_APP_NODE, tafAppMgmt.sotaApp);
        le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
        le_cfg_SetBool(wrIter, "activated", true);
        le_cfg_CommitTxn(wrIter);
    }

    tafAppMgmt.NotifyProgress(state, 0, error);
    tafAppMgmt.ReportState(rState);
}

/*======================================================================
 FUNCTION        taf_AppMgmt::ReportTimerHandler
 DESCRIPTION     Report timer handler for app
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::ReportTimerHandler(le_timer_Ref_t timerRef)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    uint32_t expiryCnt = le_timer_GetExpiryCount(timerRef);
    LE_INFO("Report times : %d.", expiryCnt);

    int ret = taf_pa_update_Report(tafUpdate.daSessionID, tafAppMgmt.rState);
    if (ret) {
        LE_ERROR("Download agent report failed, ret = %d.", ret);
    } else {
        LE_INFO("Download agent report successfully.");

        LE_INFO("Stop report timer.");
        le_timer_Stop(timerRef);
        tafAppMgmt.NotifyProgress(TAF_UPDATE_IDLE, 0, TAF_UPDATE_NONE);
    }
}

/*======================================================================
 FUNCTION        taf_AppMgmt::InstallHandler
 DESCRIPTION     App install handler
 PARAMETERS      [IN] state: State of installation
                 [IN] percent: Percent of the installation
                 [IN] contextPtr: Context of installation
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::InstallHandler(le_update_State_t state, uint percent, void* contextPtr)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_update_Error_t error = TAF_UPDATE_NONE;

    switch (state) {
        case LE_UPDATE_STATE_DOWNLOAD_SUCCESS:
            LE_INFO("Install init.");
            le_update_Install();
            break;
        case LE_UPDATE_STATE_UNPACKING:
            LE_INFO("Unpaking %d%%...", percent);
            tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALLING, percent, error);
            break;
        case LE_UPDATE_STATE_APPLYING:
            LE_INFO("Applying ...");
            break;
        case LE_UPDATE_STATE_SUCCESS:
            LE_INFO("Install success.");
            le_update_End();
            tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALL_SUCCESS, percent, error);
            tafAppMgmt.CreateAppNode(tafAppMgmt.sotaApp);
            break;
        case LE_UPDATE_STATE_FAILED:
        default:
            LE_ERROR("Install failed.");
            le_update_End();
            error = (taf_update_Error_t)le_update_GetErrorCode();
            tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent, error);
            tafAppMgmt.ReportState(TAF_UPDATE_REPORT_FAILURE);
            break;
    }
}

/*======================================================================
 FUNCTION        taf_AppMgmt::InstallHandler
 DESCRIPTION     Install handler
 PARAMETERS      [IN] reqPtr: App update request
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::AppUpdateHandler(void* reqPtr)
{
    taf_AppMgmtUpdateReq_t* updateReq = (taf_AppMgmtUpdateReq_t*)reqPtr;
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();

    switch (tafAppMgmt.sotaState) {
        case TAF_UPDATE_IDLE:
            if (updateReq->event == TAF_APPMGMT_EV_START_INSTALL) {
                LE_INFO("Start to install app %s.", updateReq->name);

                le_utf8_Copy(tafAppMgmt.sotaApp, updateReq->name, TAF_APPMGMT_APP_NAME_BYTES, NULL);
                tafUpdate.WriteFs(TAF_APPMGMT_SOTA_APP, (uint8_t*)&tafAppMgmt.sotaApp, TAF_APPMGMT_APP_NAME_BYTES);
                tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALLING, 0, TAF_UPDATE_NONE);

                char path[PATH_MAX] = {0};
                snprintf(path, sizeof(path), TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH, updateReq->name);
                if (access(path, 0)) {
                    LE_ERROR("App update bundle %s not found.", path);
                    tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, 0, TAF_UPDATE_PACKAGE_NOT_FOUND);
                    tafAppMgmt.ReportState(TAF_UPDATE_REPORT_FAILURE);
                } else {
                    int fd = open(path, O_RDONLY);
                    le_result_t result = le_update_Start(fd);
                    if (result != LE_OK) {
                        le_update_End();
                        tafAppMgmt.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, 0, TAF_UPDATE_BAD_PACKAGE);
                        tafAppMgmt.ReportState(TAF_UPDATE_REPORT_FAILURE);
                    }
                }
            } else {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (updateReq->event == TAF_APPMGMT_EV_START_PROBATION) {
                LE_INFO("Start probation for app.");
                tafAppMgmt.NotifyProgress(TAF_UPDATE_PROBATION, 0, TAF_UPDATE_NONE);
                le_timer_Start(tafAppMgmt.prbtTimerRef);
            } else if (updateReq->event == TAF_APPMGMT_EV_START_UNINSTALL) {
                LE_INFO("Start to uninstall app %s.", tafAppMgmt.sotaApp);

                taf_update_Error_t error = TAF_UPDATE_NONE;
                le_result_t result = le_appRemove_Remove(tafAppMgmt.sotaApp);
                if (result != LE_OK) {
                    LE_ERROR("Uninstall app %s failed.", tafAppMgmt.sotaApp);
                    error = TAF_UPDATE_INTERNAL_ERROR;
                } else {
                    tafAppMgmt.DeleteAppNode(tafAppMgmt.sotaApp);
                }

                tafAppMgmt.NotifyProgress(TAF_UPDATE_IDLE, 0, error);
            } else {
                LE_ERROR("Invalid operation (%d) for install success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            if (updateReq->event == TAF_APPMGMT_EV_REPORT_SUCCESS) {
                LE_INFO("Probation success.");
                tafAppMgmt.ReportState(TAF_UPDATE_REPORT_SUCCESS);
            } else {
                LE_ERROR("Invalid operation (%d) for probation success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION:
        case TAF_UPDATE_PROBATION_FAIL:
            if (updateReq->event == TAF_APPMGMT_EV_START_ROLLBACK) {
                LE_WARN("Rollback not supported yet, back to idle state.");
                tafAppMgmt.NotifyProgress(TAF_UPDATE_IDLE, 0, TAF_UPDATE_NONE);
            } else {
                LE_ERROR("Invalid operation (%d) for probation or probation fail state.", updateReq->event);
            }
        case TAF_UPDATE_REPORTING:
            if (updateReq->event == TAF_APPMGMT_EV_START_REPORT) {
                LE_INFO("Start reporting to server.");
                int ret = taf_pa_update_Report(tafUpdate.daSessionID, tafAppMgmt.rState);
                if (ret) {
                    LE_ERROR("Download agent report failed, ret = %d.", ret);

                    LE_INFO("Start timer to report status until network available.");
                    le_timer_Start(tafAppMgmt.rptTimerRef);
                } else {
                    LE_INFO("Report to server successfully.");
                    tafAppMgmt.NotifyProgress(TAF_UPDATE_IDLE, 0, TAF_UPDATE_NONE);
                }
            } else {
                LE_ERROR("Invalid operation (%d) for reporting state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALLING:
        case TAF_UPDATE_INSTALL_FAIL:
        case TAF_UPDATE_ROLLBACK:
        case TAF_UPDATE_ROLLBACK_FAIL:
        case TAF_UPDATE_ROLLBACK_SUCCESS:
            if (updateReq->event == TAF_APPMGMT_EV_REPORT_FAIL) {
                LE_INFO("App %s installation or probation with error.", tafAppMgmt.sotaApp);
                tafAppMgmt.ReportState(TAF_UPDATE_REPORT_FAILURE);
            } else {
                LE_ERROR("Invalid operation (%d) for current state (%d).", updateReq->event, tafAppMgmt.sotaState);
            }
            break;
        default:
            LE_ERROR("Invalid state (%d).", tafAppMgmt.sotaState);
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::AppUpdateThread
 DESCRIPTION     Thread for handling SOTA
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_AppMgmt::AppUpdateThread(void* contextPtr)
{
    // Read and write config tree.
    le_cfg_ConnectService();

    // App control and information.
    le_appInfo_ConnectService();
    le_appCtrl_ConnectService();
    le_appRemove_ConnectService();

    // Register update handler and mark system.
    le_update_ConnectService();
    le_updateCtrl_ConnectService();

    le_update_AddProgressHandler(InstallHandler, NULL);
    le_event_AddHandler("appUpdateHandler", appUpdateEvId, AppUpdateHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

/*======================================================================
 FUNCTION        taf_AppMgmt::Init
 DESCRIPTION     Initialization of app management component
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    // 1. Connect to supervisor services.
    le_cfg_ConnectService();
    le_appInfo_ConnectService();
    le_appCtrl_ConnectService();
    le_updateCtrl_ConnectService();

    // 2. Initiate the memory pool
    appListPool = le_mem_InitStaticPool(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM, sizeof(taf_AppMgmtAppList_t));
    appInfoPool = le_mem_InitStaticPool(appInfoPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_appMgmt_AppInfo_t));
    appInfoSafeRefPool = le_mem_InitStaticPool(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM,
        sizeof(taf_AppMgmtAppInfoSafeRef_t));

    // 3. Initiate the reference map.
    appListRefMap = le_ref_InitStaticMap(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
    appInfoSafeRefMap = le_ref_InitStaticMap(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);

    // 4. Create event for app update.
    appUpdateEvId = le_event_CreateId("appUpdateEvId", sizeof(taf_AppMgmtUpdateReq_t));

    // 5. Create thread for app update.
    le_sem_Ref_t semaphore = le_sem_Create("appUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("appUpdateThread", AppUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 6. Create probation timer.
    prbtTimerRef = le_timer_Create("App Probation Timer");
    le_timer_SetMsInterval(prbtTimerRef, TAF_APP_PROBATION_TIME_INTERVAL);
    le_timer_SetHandler(prbtTimerRef, ProbationTimerHandler);

    // 7. Create report timer.
    rptTimerRef = le_timer_Create("App Report Timer");
    le_timer_SetMsInterval(rptTimerRef, TAF_APP_REPORT_TIME_INTERVAL);
    le_timer_SetRepeat(rptTimerRef, 0);
    le_timer_SetHandler(rptTimerRef, ReportTimerHandler);

    // 8. Initiate state from file.
    auto &tafUpdate = taf_Update::GetInstance();
    if (le_fs_Exists(TAF_APPMGMT_SOTA_STATE)) {
        tafUpdate.ReadFs(TAF_APPMGMT_SOTA_STATE, (uint8_t*)&sotaState, sizeof(taf_update_State_t));
    }

    if (le_fs_Exists(TAF_APPMGMT_SOTA_REPORT_STATE)) {
        tafUpdate.ReadFs(TAF_APPMGMT_SOTA_REPORT_STATE, (uint8_t*)&rState, sizeof(taf_update_ReportState_t));
    }

    if (le_fs_Exists(TAF_APPMGMT_SOTA_APP_START_MODE)) {
        tafUpdate.ReadFs(TAF_APPMGMT_SOTA_APP_START_MODE, (uint8_t*)&isManualStart, sizeof(bool));
    }

    if (le_fs_Exists(TAF_APPMGMT_SOTA_APP)) {
        tafUpdate.ReadFs(TAF_APPMGMT_SOTA_APP, (uint8_t*)&sotaApp, sizeof(sotaApp));
    }

    // 9. Drive event from current state.
    taf_AppMgmtUpdateReq_t updateReq;
    switch (sotaState) {
        case TAF_UPDATE_IDLE:
            LE_INFO("SOTA idle state.");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("SOTA install success state.");
            if (isManualStart) {
                LE_INFO("App %s should be started into probation or uninstalled manually.", sotaApp);
            } else {
                LE_INFO("App %s started into probation automatically.", sotaApp);
                updateReq.event = TAF_APPMGMT_EV_START_PROBATION;
                le_event_Report(appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            }
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("SOTA probation success state.");
            updateReq.event = TAF_APPMGMT_EV_REPORT_SUCCESS;
            le_event_Report(appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            break;
        case TAF_UPDATE_REPORTING:
            LE_INFO("SOTA reporting state.");
            updateReq.event = TAF_APPMGMT_EV_START_REPORT;
            le_event_Report(appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            break;
        case TAF_UPDATE_PROBATION:
        case TAF_UPDATE_PROBATION_FAIL:
            LE_ERROR("SOTA app %s probation with state (%d).", sotaApp, sotaState);
            updateReq.event = TAF_APPMGMT_EV_START_ROLLBACK;
            le_event_Report(appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            break;
        case TAF_UPDATE_INSTALLING:
        case TAF_UPDATE_INSTALL_FAIL:
        case TAF_UPDATE_ROLLBACK:
        case TAF_UPDATE_ROLLBACK_FAIL:
        case TAF_UPDATE_ROLLBACK_SUCCESS:
        default:
            LE_ERROR("SOTA app %s install or rollback with state (%d).", sotaApp, sotaState);
            updateReq.event = TAF_APPMGMT_EV_REPORT_FAIL;
            le_event_Report(appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
    }

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafAppMgmt component: %lfs.", elapsedTime.count());
}