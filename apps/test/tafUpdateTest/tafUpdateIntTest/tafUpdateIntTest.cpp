/*
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafUpdateIntTest.cpp
 * @brief      This file implements integration test for Update Service.
 */

#include <iostream>
#include "legato.h"
#include "interfaces.h"

le_sem_Ref_t semaphore;
taf_update_StateHandlerRef_t handlerRef;

#define SESSION_CONF_FILE "/data/session.conf"
#define IMAGE_VERSION_FILE "/data/image_version.txt"

/*======================================================================
 FUNCTION        PrintHelpMenu
 DESCRIPTION     Print help menue
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void PrintHelpMenu()
{
    printf("Please run \"app runProc tafUpdateIntTest tafUpdateIntTest -- [option]\"\n");
    printf("Description:\n");
    printf("help                    : Print help menu.\n");
    printf("download                : Download OTA package.");
    printf("install-precheck        : Install precheck with image versions.\n");
    printf("install firmware [path] : Install firmware.\n");
    printf("install-postcheck       : Install postcheck on partition md5.\n");
    printf("install [appBundlePath] : Install application.\n");
    printf("get-active-bank         : Get active bank.\n");
    printf("activation              : Activation verification on image versions after bank switch.\n");
    printf("rollback                : Perform rollback.\n");
    printf("bank-sync               : Bank synchronization.\n");
    printf("bank-erase [bank]       : Erase bank.\n");
    printf("start-bank-sync         : Start Bank synchronization.\n");
    printf("pause-bank-sync         : Pause Bank synchronization.\n");
    printf("resume-bank-sync        : Resume Bank synchronization.\n");
    printf("sync-console            : Perform sync operations with notifications.\n");
    printf("version firmware        : Show firmware version.\n");
    printf("version [app]           : Show app version.\n");
    printf("reboot                  : Reboot to active slot.\n");
    printf("start [app]             : Start application.\n");
    printf("stop [app]              : Stop application.\n");
    printf("uninstall [app]         : Uninstall application.\n");
    printf("appState [app]          : Show app running state.\n");
    printf("appInfo                 : Show app information.\n");
}

std::string GetErrorCodeString(taf_update_Error_t errCode)
{
    std::string ret;
    if(errCode == TAF_UPDATE_BAD_PACKAGE)
    {
        ret = "TAF_UPDATE_BAD_PACKAGE";
    }
    else if(errCode == TAF_UPDATE_INTERNAL_ERROR)
    {
        ret = "TAF_UPDATE_INTERNAL_ERROR";
    }
    else if(errCode == TAF_UPDATE_SECURITY_FAILURE)
    {
        ret = "TAF_UPDATE_SECURITY_FAILURE";
    }
    else if(errCode == TAF_UPDATE_PACKAGE_NOT_FOUND)
    {
        ret = "TAF_UPDATE_PACKAGE_NOT_FOUND";
    }
    else if(errCode == TAF_UPDATE_APP_NOT_RUNNING)
    {
        ret = "TAF_UPDATE_APP_NOT_RUNNING";
    }
    else if(errCode == TAF_UPDATE_INVALID_OPERATION)
    {
        ret = "TAF_UPDATE_INVALID_OPERATION";
    }
    else if(errCode == TAF_UPDATE_IMAGE_NOT_VERFIED)
    {
        ret = "TAF_UPDATE_IMAGE_NOT_VERFIED";
    }
    else if(errCode == TAF_UPDATE_IO_ERROR)
    {
        ret = "TAF_UPDATE_IO_ERROR";
    }
    else if(errCode == TAF_UPDATE_NO_MEM_ERROR)
    {
        ret = "TAF_UPDATE_NO_MEM_ERROR";
    }
    else if(errCode == TAF_UPDATE_INVAL_ARG_ERROR)
    {
        ret = "TAF_UPDATE_INVAL_ARG_ERROR";
    }
    else if(errCode == TAF_UPDATE_BUSY_ERROR)
    {
        ret = "TAF_UPDATE_BUSY_ERROR";
    }
    else if(errCode == TAF_UPDATE_NODEV_ERROR)
    {
        ret = "TAF_UPDATE_NODEV_ERROR";
    }
    else
    {
        ret = "TAF_UPDATE_NONE";
    }
    return ret;
}

/*======================================================================
 FUNCTION        StateHandler
 DESCRIPTION     Handler function for update state
 PARAMETERS      [IN] indication : Indication for state
                 [IN] contextPtr: Context
 RETURN VALUE    void
======================================================================*/
void StateHandler(taf_update_StateInd_t* indication, taf_update_SessionRef_t sessRef, void* contextPtr)
{
    if(indication->error != TAF_UPDATE_NONE)
    {
        std::string errStr = GetErrorCodeString(indication->error);
        LE_ERROR("Error code received: %s", errStr.c_str());
        if(indication->error == TAF_UPDATE_INVALID_OPERATION)
        {
            printf("Invalid operation requested\n");
            le_sem_Post(semaphore);
            return;
        }
    }

    switch (indication->state)
    {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            LE_TEST_OK(false, "taf_update_Download - Fail");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("Downloading %d%% .", indication->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            LE_TEST_OK(true, "taf_update_Download - OK");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%% .", indication->percent);
            break;
        case TAF_UPDATE_INSTALL_PAUSED:
            LE_INFO("Install paused %d%% .", indication->percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_INFO("Install failed.");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            break;
        case TAF_UPDATE_PROBATION:
            LE_INFO("Probation %d%% .", indication->percent);
            break;
        case TAF_UPDATE_PROBATION_PAUSED:
            LE_INFO("Probation paused %d%% .", indication->percent);
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_INFO("Probation failed.");
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Probation success.");
            break;
        case TAF_UPDATE_IDLE:
            LE_INFO("Idle.");
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            printf("AB Sync in progress, %u%% completed\n", indication->percent);
            break;
        case TAF_UPDATE_SYNC_SUCCESS:
            printf("\n\nAB Sync was successful\n\n");
            le_sem_Post(semaphore);
            break;
        case TAF_UPDATE_SYNC_PAUSED:
            printf("\n\nAB Sync was paused\n");
            break;
        case TAF_UPDATE_SYNC_FAIL:
            printf("\n\nAB Sync failed\n");
            break;
        case TAF_UPDATE_CANCELLED:
            printf("\n\nCancelled.\n");
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
        (taf_update_StateHandlerFunc_t)StateHandler, nullptr);

    LE_TEST_OK(handlerRef != nullptr, "taf_update_AddStateHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return nullptr;
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
    LE_ASSERT(listRef != nullptr);
    taf_appMgmt_AppInfo_t info;
    taf_appMgmt_AppRef_t appRef = taf_appMgmt_GetFirstApp(listRef);
    while (appRef != nullptr) {
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

    if(le_arg_NumArgs() == 0)
    {
        printf("No arguments provided\n");
        PrintHelpMenu();
        LE_TEST_EXIT;
    }

    const char* cmd = le_arg_GetArg(0);
    if(cmd == nullptr)
    {
        PrintHelpMenu();
        LE_TEST_EXIT;
    }

    semaphore = le_sem_Create("semaphore", 0);
    le_result_t result;
    taf_update_SessionRef_t sessRef = nullptr;
    if (strncmp(cmd, "download", strlen("download")) == 0)
    {
        LE_TEST_INFO("======== Download Test ========");
        CreateHandlerThread();
        result = taf_update_GetDownloadSession(SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetDownloadSession - OK");
        result = taf_update_StartDownload(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_Download - OK");
        le_sem_Wait(semaphore);
        taf_update_RemoveStateHandler(handlerRef);
        LE_TEST_OK(true, "taf_update_RemoveStateHandler - OK");
    }
    else if (strncmp(cmd, "start-bank-sync", strlen("start-bank-sync")) == 0)
    {
        LE_TEST_INFO("======== Start Bank Sync Test ========");
        result = taf_update_GetInstallationSession(
            TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP, SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
        result = taf_update_StartSync(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_StartSync - OK");
    }
    else if (strncmp(cmd, "pause-bank-sync", strlen("pause-bank-sync")) == 0)
    {
        LE_TEST_INFO("======== Pause Bank Sync Test ========");
        result = taf_update_GetInstallationSession(
            TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP, SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
        result = taf_update_PauseSync(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_PauseSync - OK");
    }
    else if (strncmp(cmd, "resume-bank-sync", strlen("resume-bank-sync")) == 0)
    {
        LE_TEST_INFO("======== Resume Bank Sync Test ========");
        result = taf_update_GetInstallationSession(
            TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP, SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
        result = taf_update_ResumeSync(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_ResumeSync - OK");
    }
    else if (strncmp(cmd, "sync-console", strlen("sync-console")) == 0)
    {
        printf("A-B bank sync menu:\n");
        printf("Enter s to start A-B sync operation\n");
        printf("Enter p to pause A-B sync operation\n");
        printf("Enter r to resume A-B sync operation\n");
        printf("Enter c to cancel A-B sync operation\n");
        printf("Enter e to exit\n");

        CreateHandlerThread();

        le_result_t result;
        taf_update_SessionRef_t sessRef = nullptr;
        result = taf_update_GetInstallationSession(
            TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP, SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
        while(true)
        {
            char input;
            std::cin >> input;
            if(input == 's')
            {
                printf("\nStarting AB Sync..\n");
                result = taf_update_StartSync(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_StartSync - OK");
            }
            else if(input == 'p')
            {
                printf("\nPausing AB Sync..\n");
                result = taf_update_PauseSync(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_PauseSync - OK");
            }
            else if(input == 'r')
            {
                printf("\nResuming AB Sync..\n");
                result = taf_update_ResumeSync(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_ResumeSync - OK");
            }
            else if(input == 'c')
            {
                printf("\nCanceling AB Sync..\n");
                result = taf_update_CancelSync(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_CancelSync - OK");
            }
            else if(input == 'e')
            {
                printf("Exiting..\n");
                break;
            }
            else
            {
                printf("Wrong input, try again\n");
            }
        }
        taf_update_RemoveStateHandler(handlerRef);
    }
    else if (strncmp(cmd, "install-precheck", strlen("install-precheck")) == 0)
    {
        LE_TEST_INFO("======== Install Pre-Check Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        result = taf_update_InstallPreCheck(sessRef, IMAGE_VERSION_FILE);
        LE_TEST_OK(result == LE_OK, "taf_update_InstallPreCheck - OK");
    }
    else if (strncmp(cmd, "install-postcheck", strlen("install-postcheck")) == 0)
    {
        LE_TEST_INFO("======== Install Post-Check Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        result = taf_update_InstallPostCheck(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_InstallPostCheck - OK");
    }
    else if (strncmp(cmd, "install", strlen("install")) == 0) {
        LE_TEST_INFO("======== Install Test ========");
        CreateHandlerThread();
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        if (strncmp(name, "firmware", strlen("firmware")) == 0) {
            result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
                SESSION_CONF_FILE, &sessRef);
            LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
            const char* path = le_arg_GetArg(2);
            if (path != nullptr)
            {
                LE_INFO("Enter s to start installation.");
                LE_INFO("Enter p to pause installation.");
                LE_INFO("Enter r to resume installation.");
                LE_INFO("Enter c to cancel installation.");
                LE_INFO("Enter e to exit installation.");
                while (true)
                {
                    char input;
                    std::cin >> input;
                    if (input == 's')
                    {
                        LE_INFO("Start install.");
                        result = taf_update_StartInstall(sessRef, path);
                        LE_TEST_OK(result == LE_OK, "taf_update_StartInstall - OK");
                    }
                    else if (input == 'p')
                    {
                        LE_INFO("Pause install.");
                        result = taf_update_PauseInstall(sessRef);
                        LE_TEST_OK(result == LE_OK, "taf_update_PauseInstall - OK");
                    }
                    else if (input == 'r')
                    {
                        LE_INFO("Resume install.");
                        result = taf_update_ResumeInstall(sessRef);
                        LE_TEST_OK(result == LE_OK, "taf_update_ResumeInstall - OK");
                    }
                    else if (input == 'c')
                    {
                        LE_INFO("Cancel install.");
                        result = taf_update_CancelInstall(sessRef);
                        LE_TEST_OK(result == LE_OK, "taf_update_CancelInstall - OK");
                    }
                    else if (input == 'e')
                    {
                        LE_INFO("Exit install.");
                        break;
                    }
                }
            }
        }
        else if (name != nullptr)
        {
            result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_TELAF_APP,
                SESSION_CONF_FILE, &sessRef);
            LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");
            result = taf_update_StartInstall(sessRef, name);
            LE_TEST_OK(result == LE_OK, "taf_update_StartInstall - OK");
        }
        taf_update_RemoveStateHandler(handlerRef);
        LE_TEST_OK(true, "taf_update_RemoveStateHandler - OK");
    }
    else if (strncmp(cmd, "get-active-bank", strlen("get-active-bank")) == 0)
    {
        LE_TEST_INFO("======== Active Bank Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
        result = taf_update_GetActiveBank(sessRef, &bank);
        LE_TEST_OK(result == LE_OK, "taf_update_GetActiveBank - OK");
        switch (bank)
        {
            case TAF_UPDATE_BANK_A:
                LE_INFO("Active bank is A.");
                break;
            case TAF_UPDATE_BANK_B:
                LE_INFO("Active bank is B.");
                break;
            default:
                LE_INFO("Active bank is Unknown.");
                break;
        }
    }
    else if (strncmp(cmd, "activation", strlen("activation")) == 0)
    {
        LE_TEST_INFO("======== Activation Test ========");
        CreateHandlerThread();
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        LE_INFO("Enter s to start activation.");
        LE_INFO("Enter p to pause activation.");
        LE_INFO("Enter r to resume activation.");
        LE_INFO("Enter e to exit activation.");
        while (true)
        {
            char input;
            std::cin >> input;
            if (input == 's')
            {
                LE_INFO("Start activation.");
                result = taf_update_VerifyActivation(sessRef, IMAGE_VERSION_FILE);
                LE_TEST_OK(result == LE_OK, "taf_update_VerifyActivation - OK");
            }
            else if (input == 'p')
            {
                LE_INFO("Pause activation.");
                result = taf_update_PauseActivation(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_PauseActivation - OK");
            }
            else if (input == 'r')
            {
                LE_INFO("Resume activation.");
                result = taf_update_ResumeActivation(sessRef);
                LE_TEST_OK(result == LE_OK, "taf_update_ResumeActivation - OK");
            }
            else if (input == 'e')
            {
                LE_INFO("Exit activation.");
                break;
            }
        }
        taf_update_RemoveStateHandler(handlerRef);
        LE_TEST_OK(true, "taf_update_RemoveStateHandler - OK");
    }
    else if (strncmp(cmd, "bank-sync", strlen("bank-sync")) == 0)
    {
        LE_TEST_INFO("======== Bank Sync Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        result = taf_update_Sync(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_Sync - OK");
    }
    else if (strncmp(cmd, "bank-erase", strlen("bank-erase")) == 0)
    {
        LE_TEST_INFO("======== Bank Erase Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        const char* bank = le_arg_GetArg(1);
        if(bank == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }

        if (strncmp(bank, "A", strlen("A")) == 0)
        {
            result = taf_update_EraseBank(sessRef, TAF_UPDATE_BANK_A);
            LE_TEST_OK(result == LE_OK, "taf_update_EraseBank - OK");
        }
        else if (strncmp(bank, "B", strlen("B")) == 0)
        {
            result = taf_update_EraseBank(sessRef, TAF_UPDATE_BANK_B);
            LE_TEST_OK(result == LE_OK, "taf_update_EraseBank - OK");
        }
    }
    else if (strncmp(cmd, "rollback", strlen("rollback")) == 0)
    {
        LE_TEST_INFO("======== Rollback Test ========");
        result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP,
            SESSION_CONF_FILE, &sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_GetInstallationSession - OK");

        result = taf_update_Rollback(sessRef);
        LE_TEST_OK(result == LE_OK, "taf_update_Rollback - OK");
    }
    else if (strncmp(cmd, "version", strlen("version")) == 0)
    {
        LE_TEST_INFO("======== Version Test ========");
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        if (strncmp(name, "firmware", strlen("firmware")) == 0)
        {
            char version[TAF_FWUPDATE_MAX_VERS_LEN];
            result = taf_fwupdate_GetFirmwareVersion(version, sizeof(version));
            LE_INFO("firmware version: %s", version);
            LE_TEST_OK(result == LE_OK, "taf_fwupdate_GetFirmwareVersion - OK");
        }
        else if (name != nullptr)
        {
            char version[TAF_APPMGMT_APP_VERSION_BYTES];
            result = taf_appMgmt_GetVersion(name, version, sizeof(version));
            LE_INFO("app(%s) version:: %s", name, version);
            LE_TEST_OK(result == LE_OK, "taf_appMgmt_GetVersion - OK");
        }
    }
    else if (strncmp(cmd, "reboot", strlen("reboot")) == 0)
    {
        LE_TEST_INFO("======== Reboot Test ========");
        taf_fwupdate_RebootToActive();
    }
    else if (strncmp(cmd, "start", strlen("start")) == 0)
    {
        LE_TEST_INFO("======== App Start Test ========");
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        result = taf_appMgmt_Start(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Start - OK");
    }
    else if (strncmp(cmd, "stop", strlen("stop")) == 0)
    {
        LE_TEST_INFO("======== App Stop Test ========");
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        result = taf_appMgmt_Stop(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Stop - OK");
    }
    else if (strncmp(cmd, "uninstall", strlen("uninstall")) == 0)
    {
        LE_TEST_INFO("======== App Uninstall Test ========");
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        result = taf_appMgmt_Uninstall(name);
        LE_TEST_OK(result == LE_OK, "taf_appMgmt_Uninstall - OK");
    }
    else if (strncmp(cmd, "appState", strlen("appState")) == 0)
    {
        LE_TEST_INFO("======== App State Test ========");
        const char* name = le_arg_GetArg(1);
        if(name == nullptr)
        {
            PrintHelpMenu();
            LE_TEST_EXIT;
        }
        taf_appMgmt_AppState_t state = taf_appMgmt_GetState(name);
        if (state == TAF_APPMGMT_STATE_STOPPED)
        {
            LE_INFO("app %s is not running.", name);
        }
        else
        {
            LE_INFO("app %s is running.", name);
        }
        LE_TEST_OK(true, "taf_appMgmt_GetState - OK");
    }
    else if (strncmp(cmd, "appInfo", strlen("appInfo")) == 0)
    {
        LE_TEST_INFO("======== App Info Test ========");
        GetAppInfo(semaphore);
        le_sem_Wait(semaphore);
    }
    else
    {
        PrintHelpMenu();
    }

    le_sem_Delete(semaphore);

    LE_TEST_EXIT;
}
