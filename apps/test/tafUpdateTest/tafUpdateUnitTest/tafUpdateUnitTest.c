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
 * @file       tafUpdateUnitTest.c
 * @brief      This file implements unit test for Update Service.
 */

#include "legato.h"
#include "interfaces.h"

#ifdef TAF_CONFIG_DOWNLOAD_TEST
#define DOWNLOAD_TEST 1
#else
#define DOWNLOAD_TEST 0
#endif

#ifdef TAF_CONFIG_FOTA_TEST
#define FOTA_TEST 1
#else
#define FOTA_TEST 0
#endif

#ifdef TAF_CONFIG_SOTA_TEST
#define SOTA_TEST 1
#else
#define SOTA_TEST 0
#endif

#define DOWNLOAD_TIME 600

#define SESSION_CONF_FILE "/data/session.conf"

#define FOTA_INSTALL_TIME 600

#define SOTA_APP_NAME "/data/images/app_helloWorld"
#define SOTA_INSTALL_TIME 10
#define SOTA_PROBATION_TIME 15

taf_update_StateHandlerRef_t StateHandlerRef = NULL;

/*======================================================================
 FUNCTION        StateHandlerFunction
 DESCRIPTION     Handler function for update state
 PARAMETERS      [IN] indication : Indication for state
                 [IN] contextPtr: Context
 RETURN VALUE    void
======================================================================*/
void StateHandlerFunction(taf_update_StateInd_t* indication, void* contextPtr)
{
    switch (indication->state) {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            LE_INFO("Download fail.");
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("Downloading %d%% .", indication->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            LE_INFO("Download is successful.");
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%% .", indication->percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_INFO("Instalation is failed.");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Installation is successful.");
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
static void* StateHandlerThread(void* contextPtr)
{
    taf_update_ConnectService();

    LE_INFO("======== State Handler Thread  ========");
    StateHandlerRef = taf_update_AddStateHandler((taf_update_StateHandlerFunc_t)StateHandlerFunction, NULL);
    LE_TEST_OK((StateHandlerRef != NULL), "taf_update_AddStateHandler - !NULL");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();
    return NULL;
}

/*======================================================================
 FUNCTION        TestTafUpdateHandler
 DESCRIPTION     Update handler API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafUpdateHandler(void)
{
    le_thread_Ref_t stateThreadRef;

    LE_TEST_INFO("Start taf_update_StateHandler Test");

    // Add State Handler Test
    le_sem_Ref_t threadSem = le_sem_Create("threadSem", 0);
    stateThreadRef = le_thread_Create("StateHandlerThread", StateHandlerThread, (void*)threadSem);
    le_thread_Start(stateThreadRef);
    le_sem_Wait(threadSem);
    le_sem_Delete(threadSem);

    // Remove State Handler Test
    taf_update_RemoveStateHandler(StateHandlerRef);
    LE_TEST_OK(true, "taf_update_RemoveStateHandler - void");
}

/*======================================================================
 FUNCTION        TestTafUpdateDownload
 DESCRIPTION     Download API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafUpdateDownload(void)
{
    LE_TEST_INFO("Start taf_update_Download Test");

    LE_TEST_BEGIN_SKIP(!DOWNLOAD_TEST, 2);
    // Download Test
    taf_update_SessionRef_t sessRef = NULL;
    le_result_t result = taf_update_GetDownloadSession(SESSION_CONF_FILE, &sessRef);
    LE_TEST_OK(result == LE_OK, "taf_update_GetDownloadSession - OK");

    result = taf_update_StartDownload(sessRef);
    LE_TEST_OK(result == LE_OK, "taf_update_Download - OK");

    le_thread_Sleep(DOWNLOAD_TIME);
    LE_TEST_END_SKIP();
}

/*======================================================================
 FUNCTION        TestTafUpdateInstall
 DESCRIPTION     Install API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafUpdateInstall(void)
{
    le_result_t result;
    taf_update_SessionRef_t sessRef = NULL;

    LE_TEST_INFO("Start taf_update_Install Test");

    // Install Test
    LE_TEST_BEGIN_SKIP(!SOTA_TEST, 1);
    result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_TELAF_APP,
        SESSION_CONF_FILE, &sessRef);
    LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
    result = taf_update_StartInstall(sessRef, SOTA_APP_NAME);
    LE_TEST_OK(result == LE_OK, "taf_update_Install - OK");

    le_thread_Sleep(SOTA_INSTALL_TIME);

    LE_TEST_END_SKIP();
}

/*======================================================================
 FUNCTION        TestTafAppInformation
 DESCRIPTION     App info API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafAppInformation(void)
{
    taf_appMgmt_AppInfo_t info;
    taf_appMgmt_AppRef_t appRef;
    le_result_t result;
    char version[TAF_APPMGMT_APP_VERSION_BYTES];

    LE_TEST_INFO("Start taf_appMgmt_Info Test");

    // App Information Create List Test
    taf_appMgmt_AppListRef_t listRef = taf_appMgmt_CreateAppList();
    LE_TEST_OK((listRef != NULL), "taf_appMgmt_CreateAppList - !NULL");

    // App Information Boundary Test
    appRef = taf_appMgmt_GetFirstApp(NULL);
    LE_TEST_OK((appRef == NULL), "taf_appMgmt_GetFirstApp - NULL");
    appRef = taf_appMgmt_GetNextApp(NULL);
    LE_TEST_OK((appRef == NULL), "taf_appMgmt_GetNextApp - NULL");
    result = taf_appMgmt_GetAppDetails(NULL, &info);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_appMgmt_GetAppDetails - LE_BAD_PARAMETER");
    result = taf_appMgmt_DeleteAppList(NULL);
    LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_appMgmt_DeleteAppList - LE_BAD_PARAMETER");

    // App Information List Test
    LE_TEST_BEGIN_SKIP(!SOTA_TEST, 7);

    bool firstRound = true;
    appRef = taf_appMgmt_GetFirstApp(listRef);
    LE_TEST_OK((appRef != NULL), "taf_appMgmt_GetFirstApp - !NULL");

    while (appRef != NULL) {
        result = taf_appMgmt_GetAppDetails(appRef, NULL);
        if (firstRound) {
            LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_appMgmt_GetAppDetails - LE_BAD_PARAMETER");
        }

        result = taf_appMgmt_GetAppDetails(appRef, &info);
        if (firstRound) {
            LE_TEST_OK((result == LE_OK), "taf_appMgmt_GetAppDetails - LE_OK");
        }

        LE_INFO("-------------------------------------");
        LE_INFO("name:    %s", info.name);

        result = taf_appMgmt_GetVersion(info.name, NULL, 0);
        if (firstRound) {
            LE_TEST_OK((result == LE_BAD_PARAMETER), "taf_appMgmt_GetVersion - LE_BAD_PARAMETER");
        }

        result = taf_appMgmt_GetVersion(info.name, version, sizeof(version));
        if (firstRound) {
            LE_TEST_OK((result == LE_OK), "taf_appMgmt_GetVersion - LE_OK");
        }
        LE_INFO("version: %s", version);

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
        if (firstRound) {
            LE_TEST_OK(true, "taf_appMgmt_GetNextApp - !NULL");
        }
    }

    // App Information Delete List Test
    result = taf_appMgmt_DeleteAppList(listRef);
    LE_TEST_OK((result == LE_OK), "taf_appMgmt_DeleteAppList - LE_OK");

    LE_TEST_END_SKIP();
}

/*======================================================================
 FUNCTION        TestTafAppControl
 DESCRIPTION     App control API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafAppControl(void)
{
    le_result_t result;

    LE_TEST_INFO("Start taf_appMgmt_Control Test");

    LE_TEST_BEGIN_SKIP(!SOTA_TEST, 4);

    // App State Test
    taf_appMgmt_AppState_t state = taf_appMgmt_GetState(SOTA_APP_NAME);
    LE_TEST_OK((state == TAF_APPMGMT_STATE_STOPPED) || (state == TAF_APPMGMT_STATE_STARTED), "taf_appMgmt_GetState - LE_OK");

    // App Start and Stop Test
    if (state == TAF_APPMGMT_STATE_STOPPED) {
        result = taf_appMgmt_Start(SOTA_APP_NAME);
        LE_TEST_OK((result == LE_OK), "taf_appMgmt_Start - LE_OK");
        le_thread_Sleep(SOTA_PROBATION_TIME);
        result = taf_appMgmt_Stop(SOTA_APP_NAME);
        LE_TEST_OK((result == LE_OK), "taf_appMgmt_Stop - LE_OK");
    } else {
        le_thread_Sleep(SOTA_PROBATION_TIME);
        result = taf_appMgmt_Stop(SOTA_APP_NAME);
        LE_TEST_OK((result == LE_OK), "taf_appMgmt_Stop - LE_OK");
        result = taf_appMgmt_Start(SOTA_APP_NAME);
        LE_TEST_OK((result == LE_OK), "taf_appMgmt_Start - LE_OK");
    }

    // App Uninstall Test
    result = taf_appMgmt_Uninstall(SOTA_APP_NAME);
    LE_TEST_OK((result == LE_OK), "taf_appMgmt_Uninstall - LE_OK");

    LE_TEST_END_SKIP();
}

/*======================================================================
 FUNCTION        TestTafFirmwareUpdate
 DESCRIPTION     Firmware update API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestTafFirmwareUpdate(void)
{
    char version[TAF_FWUPDATE_MAX_VERS_LEN];
    le_result_t result;

    LE_TEST_INFO("Start taf_fwupdate Test");

    // Firmware Version Test
    result = taf_fwupdate_GetFirmwareVersion(version, sizeof(version));
    LE_TEST_OK((result == LE_OK), "taf_fwupdate_GetFirmwareVersion - LE_OK");
    LE_INFO("firmware version:    %s", version);

    // Firmware Install Test
    LE_TEST_BEGIN_SKIP(!FOTA_TEST, 2);

    result = taf_fwupdate_Install();
    LE_TEST_OK((result == LE_OK), "taf_fwupdate_Install - LE_OK");

    le_thread_Sleep(FOTA_INSTALL_TIME);

    taf_fwupdate_RebootToActive();
    LE_TEST_OK(true, "taf_fwupdate_RebootToActive - void");

    LE_TEST_END_SKIP();
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_TEST_PLAN(23);

    LE_TEST_INFO("======== Update State Handler Test ========");
    TestTafUpdateHandler();
    LE_TEST_INFO("======== Update Download Test ========");
    TestTafUpdateDownload();
    LE_TEST_INFO("======== Update Install Test ========");
    TestTafUpdateInstall();
    LE_TEST_INFO("======== App Information Test ========");
    TestTafAppInformation();
    LE_TEST_INFO("======== App Control Test ========");
    TestTafAppControl();
    LE_TEST_INFO("======== Firmware Update Test ========");
    TestTafFirmwareUpdate();

    LE_TEST_EXIT;
}