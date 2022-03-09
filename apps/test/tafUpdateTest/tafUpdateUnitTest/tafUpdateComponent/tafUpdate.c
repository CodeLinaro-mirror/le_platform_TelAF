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
 * @file       tafUpdate.cpp
 * @brief      This file includes test functions of the Update Service.
 */

#include "legato.h"
#include "interfaces.h"

#define TAF_UPDATE_TIME_FOR_APP_CONTROL 3
#define TAF_UPDATE_TIME_FOR_REBOOT 10
#define TAF_UPDATE_TIME_FOR_EXIT 600

#define TAF_UPDATE_INSTALL_PACKAGE "/data/images/TCU_target"
#define TAF_UPDATE_TEST_APP_NAME "helloWorld"

le_sem_Ref_t semaphore;

taf_update_StateHandlerRef_t handlerRef;

void PrintHelp()
{
    LE_INFO("Please run \"app runProc tafUpdateTestApp tafUpdate -- number\"");
    LE_INFO("number:");
    LE_INFO("1: Download package.");
    LE_INFO("2: Install FOTA package.");
    LE_INFO("3: Install SOTA package.");
    LE_INFO("4: Reboot to active slot.(FOTA only)");
    LE_INFO("5: Start helloWorld app.");
    LE_INFO("6: Stop helloWorld app.");
    LE_INFO("7: Uninstall helloWorld app.");
    LE_INFO("8: State handler for OTA.");
    LE_INFO("9: App information.");
}

void UpdateStateHandlerFunc(taf_update_StateInd_t* stateInd, void* contextPtr)
{
    LE_INFO("**** Update Handler (Begin)****");

    switch (stateInd->state) {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            LE_ERROR("Download fail.");
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("Downloading %d%% .", stateInd->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            LE_INFO("Download success.");
            if (stateInd->pkgType == TAF_UPDATE_PACKAGE_FOTA) {
                LE_INFO("Install firmware.");
                LE_ASSERT(taf_update_Install(TAF_UPDATE_PACKAGE_FOTA, TAF_UPDATE_INSTALL_PACKAGE) == LE_OK);
            } else {
                LE_INFO("Install application.");
                if (stateInd->pkgName != NULL) {
                    LE_INFO("App name is %s", stateInd->pkgName);
                }
                LE_ASSERT(taf_update_Install(TAF_UPDATE_PACKAGE_SOTA, TAF_UPDATE_INSTALL_PACKAGE) == LE_OK);
            }
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%% .", stateInd->percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_ERROR("Install fail.");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (stateInd->pkgType == TAF_UPDATE_PACKAGE_FOTA) {
                LE_INFO("Firmware install success");
                LE_INFO("Reboot to active after 10s.");
                le_thread_Sleep(TAF_UPDATE_TIME_FOR_REBOOT);
                taf_fwupdate_RebootToActive();
            } else {
                LE_INFO("Application install success");

                if ((taf_appMgmt_GetState(stateInd->pkgName) == TAF_APPMGMT_STATE_STARTED)) {
                    LE_INFO("Application is running.");
                } else {
                    LE_ASSERT(taf_appMgmt_Start(stateInd->pkgName) == LE_OK);
                }

                le_thread_Sleep(TAF_UPDATE_TIME_FOR_APP_CONTROL);
                LE_ASSERT(taf_appMgmt_GetState(stateInd->pkgName) == TAF_APPMGMT_STATE_STARTED);
            }
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

    LE_INFO("**** Update Handler (End)****");
}

void* UpdateStateThread(void* contextPtr)
{
    //  connect service in thread.
    taf_update_ConnectService();
    taf_fwupdate_ConnectService();
    taf_appMgmt_ConnectService();

    handlerRef = taf_update_AddStateHandler(
        (taf_update_StateHandlerFunc_t)UpdateStateHandlerFunc, NULL);
    LE_ASSERT(handlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

void* AppInfoThread(void* contextPtr)
{
    //  connect service in thread.
    taf_appMgmt_ConnectService();

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
        if (info.state == TAF_APPMGMT_STATE_STARTED) {
            LE_INFO("state:   running");
        } else {
            LE_INFO("state:   stopped");
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

    exit(EXIT_SUCCESS);

    return NULL;
}

COMPONENT_INIT
{
    long number = strtol(le_arg_GetArg(0), NULL, 10);
    switch (number) {
        case 1:
            taf_update_Download();
            break;
        case 2:
            taf_update_Install(TAF_UPDATE_PACKAGE_FOTA, TAF_UPDATE_INSTALL_PACKAGE);
            le_thread_Sleep(TAF_UPDATE_TIME_FOR_EXIT);
            break;
        case 3:
            taf_update_Install(TAF_UPDATE_PACKAGE_SOTA, TAF_UPDATE_INSTALL_PACKAGE);
            break;
        case 4:
            taf_fwupdate_RebootToActive();
            break;
        case 5:
            taf_appMgmt_Start(TAF_UPDATE_TEST_APP_NAME);
            le_thread_Sleep(10);
            break;
        case 6:
            taf_appMgmt_Stop(TAF_UPDATE_TEST_APP_NAME);
            le_thread_Sleep(10);
            break;
        case 7:
            taf_appMgmt_Uninstall(TAF_UPDATE_TEST_APP_NAME);
            le_thread_Sleep(10);
            break;
        case 8:
            semaphore = le_sem_Create("tafUpdateSem", 0);
            le_thread_Ref_t threadRef = le_thread_Create("UpdateStateThread",
                UpdateStateThread, NULL);
            le_thread_Start(threadRef);
            le_sem_Wait(semaphore);

            LE_INFO("Start download.");
            taf_update_Download();

            le_thread_Sleep(TAF_UPDATE_TIME_FOR_EXIT);
            break;
        case 9:
            le_thread_Start(le_thread_Create("AppInfoThread", AppInfoThread, NULL));
            le_thread_Sleep(10);
            break;
        default:
            PrintHelp();
            break;
    }

    exit(EXIT_SUCCESS);
}
