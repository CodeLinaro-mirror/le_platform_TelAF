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

/*
 * @file       tafUpdateIntTest.c
 * @brief      This file implements integration test for Update Service.
 */

#include "legato.h"
#include "interfaces.h"

le_sem_Ref_t semaphore;
taf_update_StateHandlerRef_t handlerRef;

/*======================================================================
 FUNCTION        PrintHelpMenu
 DESCRIPTION     Print help menue
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void PrintHelpMenu()
{
    LE_INFO("Please run \"app runProc tafUpdateIntTest tafUpdateIntTest -- [option]\"");
    LE_INFO("Description:");
    LE_INFO("help             : Print help menu.");
    LE_INFO("download         : Download OTA package.");
    LE_INFO("install firmware : Install firmware.");
    LE_INFO("install [app]    : Install application.");
    LE_INFO("version firmware : Show firmware version.");
    LE_INFO("version [app]    : Show app version.");
    LE_INFO("reboot           : Reboot to active slot.");
    LE_INFO("start [app]      : Start application.");
    LE_INFO("stop [app]       : Stop application.");
    LE_INFO("uninstall [app]  : Uninstall application.");
    LE_INFO("appState [app]   : Show app running state.");
    LE_INFO("appInfo          : Show app information.");
}

/*======================================================================
 FUNCTION        StateHandler
 DESCRIPTION     Handler function for update state
 PARAMETERS      [IN] indication : Indication for state
                 [IN] contextPtr: Context
 RETURN VALUE    void
======================================================================*/
void StateHandler(taf_update_StateInd_t* indication, void* contextPtr)
{
    switch (indication->state) {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            LE_TEST_OK(false, "taf_update_Download - Fail");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("Downloading %d%% .", indication->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            if (indication->ota == TAF_UPDATE_FOTA) {
                LE_INFO("Download firmware successfully.");
            } else {
                LE_INFO("Download app %s successfully.", indication->name);
            }
            LE_TEST_OK(true, "taf_update_Download - OK");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%% .", indication->percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_ERROR("Install %s fail.", indication->name);
            LE_TEST_OK(false, "taf_update_Install - Fail");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (indication->ota == TAF_UPDATE_FOTA) {
                LE_INFO("Install firmware successfully.");
            } else {
                LE_INFO("Install app %s successfully.", indication->name);
            }
            LE_TEST_OK(true, "taf_update_Install - OK");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_PROBATION:
            LE_INFO("Probation.");
            break;
        case TAF_UPDATE_IDLE:
            LE_INFO("Indle.");
            break;
        default:
            break;
    }
}

/*======================================================================
 FUNCTION        StateHandlerThread
 DESCRIPTION     Thread for adding state handler
 PARAMETERS      [IN] contextPtr: Context
 RETURN VALUE    void*
======================================================================*/
void* StateHandlerThread(void* contextPtr)
{
    taf_update_ConnectService();

    handlerRef = taf_update_AddStateHandler(
        (taf_update_StateHandlerFunc_t)StateHandler, NULL);

    LE_TEST_OK(handlerRef != NULL, "taf_update_AddStateHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

/*======================================================================
 FUNCTION        CreateHandlerThread
 DESCRIPTION     Create thread for adding handler.
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void CreateHandlerThread(void)
{
    le_thread_Ref_t stateThreadRef;

    // Add State Handler Test
    le_sem_Ref_t threadSem = le_sem_Create("threadSem", 0);
    stateThreadRef = le_thread_Create("StateHandlerThread", StateHandlerThread, (void*)threadSem);
    le_thread_Start(stateThreadRef);
    le_sem_Wait(threadSem);
    le_sem_Delete(threadSem);
}

/*======================================================================
 FUNCTION        GetAppInfo
 DESCRIPTION     Get app information
 PARAMETERS      sem : Semaphore
 RETURN VALUE    void
======================================================================*/
void GetAppInfo(le_sem_Ref_t sem)
{
    taf_appMgmt_AppListRef_t listRef = taf_appMgmt_CreateAppList();
    LE_ASSERT(listRef != NULL);
    taf_appMgmt_AppInfo_t info;
    taf_appMgmt_AppRef_t appRef = taf_appMgmt_GetFirstApp(listRef);
    while (appRef != NULL) {
        taf_appMgmt_GetAppDetails(appRef, &info);
        LE_INFO("-------------------------------------");
        LE_INFO("name:    %s", info.name);
        LE_INFO("version: %s", info.version);
        LE_INFO("hash:    %s", info.hash);
        if (info.isActivated) {
            LE_INFO("activated.");
        } else {
            LE_INFO("not activated.");
        }
        if (info.startMode == TAF_APPMGMT_START_AUTO) {
            LE_INFO("mode:    auto");
        } else {
            LE_INFO("mode:    manual");
        }
        if (info.isSandboxed) {
            LE_INFO("sandbox: yes");
        } else {
            LE_INFO("sandbox: no");
        }
        appRef = taf_appMgmt_GetNextApp(listRef);
    }
    taf_appMgmt_DeleteAppList(listRef);

    le_sem_Post(sem);
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    le_result_t result;
    const char* cmd = le_arg_GetArg(0);
    semaphore = le_sem_Create("semaphore", 0);

    if (strncmp(cmd, "download", strlen("download")) == 0) {
        LE_TEST_INFO("======== Download Test ========");
        CreateHandlerThread();
        taf_update_Download();
        le_sem_Wait(semaphore);
        taf_update_RemoveStateHandler(handlerRef);
        LE_TEST_OK(true, "taf_update_RemoveStateHandler - OK");
    } else if (strncmp(cmd, "install", strlen("install")) == 0) {
        LE_TEST_INFO("======== Install Test ========");
        CreateHandlerThread();
        const char* name = le_arg_GetArg(1);
        if (strncmp(name, "firmware", strlen("firmware")) == 0) {
            taf_update_Install(TAF_UPDATE_FOTA, name);
            le_sem_Wait(semaphore);
        } else if (name != NULL) {
            taf_update_Install(TAF_UPDATE_SOTA, name);
            le_sem_Wait(semaphore);
        }
        taf_update_RemoveStateHandler(handlerRef);
        LE_TEST_OK(true, "taf_update_RemoveStateHandler - OK");
    } else if (strncmp(cmd, "version", strlen("version")) == 0) {
        LE_TEST_INFO("======== Version Test ========");
        const char* name = le_arg_GetArg(1);
        if (strncmp(name, "firmware", strlen("firmware")) == 0) {
            char version[TAF_FWUPDATE_MAX_VERS_LEN];
            result = taf_fwupdate_GetFirmwareVersion(version, sizeof(version));
            LE_INFO("firmware version: %s", version);
            LE_TEST_OK(result == LE_OK, "taf_fwupdate_GetFirmwareVersion - OK");
        } else if (name != NULL) {
            char version[TAF_APPMGMT_APP_VERSION_BYTES];
            result = taf_appMgmt_GetVersion(name, version, sizeof(version));
            LE_INFO("app(%s) version:: %s", name, version);
            LE_TEST_OK(result == LE_OK, "taf_appMgmt_GetVersion - OK");
        }
    } else if (strncmp(cmd, "reboot", strlen("reboot")) == 0) {
        LE_TEST_INFO("======== Reboot Test ========");
        taf_fwupdate_RebootToActive();
    } else if (strncmp(cmd, "start", strlen("start")) == 0) {
        LE_TEST_INFO("======== App Start Test ========");
        const char* name = le_arg_GetArg(1);
        result = taf_appMgmt_Start(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Start - OK");
    } else if (strncmp(cmd, "stop", strlen("stop")) == 0) {
        LE_TEST_INFO("======== App Stop Test ========");
        const char* name = le_arg_GetArg(1);
        result = taf_appMgmt_Stop(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Stop - OK");
    } else if (strncmp(cmd, "uninstall", strlen("uninstall")) == 0) {
        LE_TEST_INFO("======== App Uninstall Test ========");
        const char* name = le_arg_GetArg(1);
        result = taf_appMgmt_Uninstall(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Uninstall - OK");
    } else if (strncmp(cmd, "appState", strlen("appState")) == 0) {
        LE_TEST_INFO("======== App State Test ========");
        const char* name = le_arg_GetArg(1);
        taf_appMgmt_AppState_t state = taf_appMgmt_GetState(name);
        if (state == TAF_APPMGMT_STATE_STOPPED) {
            LE_INFO("app %s is not running.", name);
        } else {
            LE_INFO("app %s is running.", name);
        }
        LE_TEST_OK(true, "taf_appMgmt_GetState - OK");
    } else if (strncmp(cmd, "appInfo", strlen("appInfo")) == 0) {
        LE_TEST_INFO("======== App Info Test ========");
        GetAppInfo(semaphore);
        le_sem_Wait(semaphore);
    } else {
        PrintHelpMenu();
    }

    le_sem_Delete(semaphore);

    LE_TEST_EXIT;
}