/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "legato.h"
#include "interfaces.h"

#include "tafPiDA.h"

#include "dlEmu.h"

extern da_InfoTab_t TAF_HAL_INFO_TAB;

#define DA_SESSION_NUM 1

//--------------------------------------------------------------------------------------------------
/**
 * Download session.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_da_Session session;

//--------------------------------------------------------------------------------------------------
/**
 * Download session reference.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_da_SessionRef_t sessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Download session reference map.
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t sessionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Download command event.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t downloadCmdEv;

//--------------------------------------------------------------------------------------------------
/**
 * Download timer reference.
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t downoadTimerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Download status.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_da_Status_t downloadStatus = TAF_PI_DA_STATUS_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Static map for update session reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(sessionRefMap, DA_SESSION_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Power on download agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_hal_PowerOn()
{
    LE_INFO("Power on DA.");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Power off download agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_hal_PowerOff()
{
    LE_INFO("Power off DA.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize download agent hardware.
 */
//--------------------------------------------------------------------------------------------------
int taf_hal_HwInit()
{
    LE_INFO("DA hardware initializing.");

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set download agent to sleep.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_hal_Sleep()
{
    LE_INFO("Set DA module to sleep.");

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Wake up download agent.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_hal_Wakeup()
{
    LE_INFO("Wakeup DA module.");

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Selftest on download agent module.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_hal_SelfTest()
{
    LE_INFO("DA module selftest.");

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get download agent information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- DA module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf()
{
    LE_INFO("Get DA module information table.");

    return &(TAF_HAL_INFO_TAB.daInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get file size.
 */
//--------------------------------------------------------------------------------------------------
void GetFileSize
(
    const char* file, ///< [IN] File path.
    size_t* fileSize  ///< [OUT] File size.
)
{
    struct stat fileStat;

    if (stat(file, &fileStat) != -1)
    {
        *fileSize = fileStat.st_size;
    }
    else
    {
        *fileSize = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Download timer handler.
 */
//--------------------------------------------------------------------------------------------------
void DownloadTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Download timer reference.
)
{
    size_t fileSize = 0;

    GetFileSize(session.target, &fileSize);

    if (fileSize < 100)
    {
        FILE* file;
        file = fopen(session.target, "a");
        if (file == NULL)
        {
            LE_ERROR("Fail to open %s", session.target);
            le_timer_Stop(downoadTimerRef);
            downloadStatus = TAF_PI_DA_STATUS_ERROR;
        }
        else
        {
            fputc((char)fileSize, file);
            fclose(file);
        }
    }
    else
    {
        LE_INFO("Download is completed.");
        le_timer_Stop(downoadTimerRef);
        downloadStatus = TAF_PI_DA_STATUS_FINISH;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Download command handler.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void DownloadCmdHandler
(
    void* cmdPtr ///< [IN] Download command.
)
{
    da_Command_t* daCmdPtr = (da_Command_t*)cmdPtr;

    switch (daCmdPtr->action)
    {
        case DA_ACTION_START:
            if (le_timer_IsRunning(downoadTimerRef))
            {
                LE_ERROR("Download is in progress.");
            }
            else
            {
                if (access(session.target, 0) == 0)
                {
                    LE_WARN("Existing download target %s removed.", session.target);
                    remove(session.target);
                }

                downloadStatus = TAF_PI_DA_STATUS_DOWNLOADING;
                le_timer_Start(downoadTimerRef);
            }
            break;
        case DA_ACTION_PAUSE:
            if (le_timer_IsRunning(downoadTimerRef))
            {
                le_timer_Stop(downoadTimerRef);
                downloadStatus = TAF_PI_DA_STATUS_PAUSED;
            }
            else
            {
                LE_ERROR("Download is not in progress.");
            }
            break;
        case DA_ACTION_RESUME:
            if (le_timer_IsRunning(downoadTimerRef))
            {
                LE_ERROR("Download is in progress.");
            }
            else
            {
                downloadStatus = TAF_PI_DA_STATUS_DOWNLOADING;
                le_timer_Start(downoadTimerRef);
            }
            break;
        case DA_ACTION_CANCEL:
            if (le_timer_IsRunning(downoadTimerRef))
            {
                le_timer_Stop(downoadTimerRef);
                if (access(session.target, 0) == 0)
                {
                    remove(session.target);
                }
                downloadStatus = TAF_PI_DA_STATUS_INIT;
            }
            else
            {
                LE_ERROR("Download is not in progress.");
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Download thread.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void* DownloadThread
(
    void* contextPtr ///< [IN] Context.
)
{
    le_event_AddHandler("DownloadCmdHandler", downloadCmdEv, DownloadCmdHandler);

    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize download agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_pi_da_Init()
{
    LE_INFO("DA Plug-In Init...");

    sessionRefMap = le_ref_InitStaticMap(sessionRefMap, DA_SESSION_NUM);

    le_sem_Ref_t semaphore = le_sem_Create("downloadSem", 0);
    downloadCmdEv = le_event_CreateId("downloadCmdEv", sizeof(da_Command_t));
    le_thread_Ref_t threadRef = le_thread_Create("downloadThread", DownloadThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, 0x20000);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);

    downoadTimerRef = le_timer_Create("downloadTimer");
    le_timer_SetMsInterval(downoadTimerRef, 1000);
    le_timer_SetRepeat(downoadTimerRef, 0);
    le_timer_SetHandler(downoadTimerRef, DownloadTimerHandler);

    LE_INFO("DA Plug-In Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Create download agent session.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_GetSession
(
    const char* cfgFile,            ///< [IN]  Download session configuration file.
    taf_pi_da_SessionRef_t* sessRef ///< [OUT] Download session reference.
)
{
    LE_INFO("DA Plug-In get download session.");

    FILE* file = fopen(cfgFile, "r");
    if (file == NULL)
    {
        LE_ERROR("Fail to open file %s", cfgFile);
        return -1;
    }

    if (fgets(session.target, sizeof(session.target), file) == NULL)
    {
        LE_ERROR("Fail to get download target.");
        return -1;
	}

    if (sessionRef == NULL)
    {
        sessionRef = (taf_pi_da_SessionRef_t)le_ref_CreateRef(sessionRefMap, (void*)&session);
    }

    *sessRef = sessionRef;

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start download.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_StartDownload
(
    taf_pi_da_SessionRef_t sessRef ///< [IN] Download session reference.
)
{
    LE_INFO("DA Plug-In start download...");

    taf_pi_da_Session* sessPtr = (taf_pi_da_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find download session.");
        return -1;
    }

    da_Command_t cmd;
    cmd.action = DA_ACTION_START;
    le_event_Report(downloadCmdEv, &cmd, sizeof(da_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause download.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_PauseDownload
(
    taf_pi_da_SessionRef_t sessRef ///< [IN] Download session reference.
)
{
    LE_INFO("DA Plug-In pause download...");

    taf_pi_da_Session* sessPtr = (taf_pi_da_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find download session.");
        return -1;
    }

    da_Command_t cmd;
    cmd.action = DA_ACTION_PAUSE;
    le_event_Report(downloadCmdEv, &cmd, sizeof(da_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_ResumeDownload
(
    taf_pi_da_SessionRef_t sessRef ///< [IN] Download session reference.
)
{
    LE_INFO("DA Plug-In resume download...");

    taf_pi_da_Session* sessPtr = (taf_pi_da_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find download session.");
        return -1;
    }

    da_Command_t cmd;
    cmd.action = DA_ACTION_RESUME;
    le_event_Report(downloadCmdEv, &cmd, sizeof(da_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_CancelDownload
(
    taf_pi_da_SessionRef_t sessRef ///< [IN] Download session reference.
)
{
    LE_INFO("DA Plug-In cancel download...");

    taf_pi_da_Session* sessPtr = (taf_pi_da_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find download session.");
        return -1;
    }

    da_Command_t cmd;
    cmd.action = DA_ACTION_CANCEL;
    le_event_Report(downloadCmdEv, &cmd, sizeof(da_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get download progress.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_da_GetProgress
(
    taf_pi_da_SessionRef_t sessRef, ///< [IN]  Download session reference.
    taf_pi_da_Status_t* status,     ///< [OUT] Download status.
    int* percent,                   ///< [OUT] Download percentage.
    int* error                      ///< [OUT] Error code in download process.
)
{
    taf_pi_da_Session* sessPtr = (taf_pi_da_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find download session.");
        return -1;
    }

    *status = downloadStatus;
    size_t fileSize = 0;
    GetFileSize(sessPtr->target, &fileSize);
    *percent = (int)fileSize;

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Download agent information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED da_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_DA_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .daInf =
    {
        .init = taf_pi_da_Init,
        .getSess = taf_pi_da_GetSession,
        .startDownload = taf_pi_da_StartDownload,
        .pauseDownload = taf_pi_da_PauseDownload,
        .resumeDownload = taf_pi_da_ResumeDownload,
        .cancelDownload = taf_pi_da_CancelDownload,
        .getProgress = taf_pi_da_GetProgress,
    },
};

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
