/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafUpdateImpl.cpp
 * @brief      This file implements update component.
 */

#include <chrono>
#include <fstream>

#include "tafUpdate.hpp"
#include "tafAppMgmt.hpp"
#include "tafFwUpdate.hpp"

using namespace std;
using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Static map for update session reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(sessionMap, TAF_UPDATE_SESSION_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for update session reference.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(sessionPool, TAF_UPDATE_SESSION_NUM, sizeof(taf_UpdateSession_t));

/*======================================================================
 FUNCTION        taf_Update::GetInstance
 DESCRIPTION     Get the instance of taf_Update
 PARAMETERS      void
 RETURN VALUE    taf_Update: Instance reference
======================================================================*/
taf_Update &taf_Update::GetInstance()
{
    static taf_Update instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check QOTA header.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Update::CheckQotaHeader
(
    const char* file ///< [IN] File path for QOTA package.
)
{
    // 1. Get the file size.
    ifstream infile(file);
    infile.seekg (0, ios::end);
    long size = infile.tellg();
    infile.seekg (0);

    TAF_ERROR_IF_RET_VAL(size <= TAF_UPDATE_QOTA_HEADER_SIZE, LE_FAULT,
        "%s less than %d bytes.", file, TAF_UPDATE_QOTA_HEADER_SIZE);

    // 2. Check QOTA magic
    le_result_t result = LE_OK;
    const char magic[] = {0x00, 0x01, 0x02, 0x03};
    char* buffer = new char[sizeof(magic)];
    infile.read (buffer, sizeof(magic));
    for (size_t i = 0; i < sizeof(magic); i++)
    {
        if (buffer[i] != magic[i])
        {
            result =  LE_FAULT;
            break;
        }
    }

    delete[] buffer;
    infile.close();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove QOTA header.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Update::RemoveQotaHeader
(
    const char* file ///< [IN] File path for QOTA package.
)
{
    int fd = open(file, O_RDWR);
    TAF_ERROR_IF_RET_VAL(fd < 0, LE_FAULT, "Open %s failed.", file);

    long size = lseek(fd, 0, SEEK_END);

    char* buffer = new char[TAF_UPDATE_RW_BUFFER_SIZE];
    TAF_ERROR_IF_RET_VAL(buffer == nullptr, LE_FAULT, "Alloc buffer failed.");

    long pos = TAF_UPDATE_QOTA_HEADER_SIZE;
    long rdSize;

    le_result_t ret = LE_OK;
    while (pos < size)
    {
        lseek(fd, pos, SEEK_SET);
        rdSize = read(fd, buffer, TAF_UPDATE_RW_BUFFER_SIZE);
        if (rdSize <= 0)
        {
            LE_ERROR("Read buffer error, return size: %ld.", rdSize);
            ret = LE_FAULT;
            break;
        }
        lseek(fd, pos - TAF_UPDATE_QOTA_HEADER_SIZE, SEEK_SET);
        write(fd, buffer, rdSize);
        pos += rdSize;
    }

    if (ret == LE_OK)
    {
        ftruncate(fd, size - TAF_UPDATE_QOTA_HEADER_SIZE);
        LE_INFO("QOTA header removed.");
    }

    delete[] buffer;
    close(fd);

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered function for state handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::StateLayeredHandler
(
    void* reportPtr,       ///< Indictaion to be reproted.
    void* layerHandlerFunc ///< Layered handler function
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(layerHandlerFunc == nullptr, "Null ptr(layerHandlerFunc)");

    taf_update_StateHandlerFunc_t handlerFunc =
        (taf_update_StateHandlerFunc_t)layerHandlerFunc;

    handlerFunc((taf_update_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Report download status.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::ReportDownloadStatus
(
    taf_UpdateDownloadSession_t* sessPtr, ///< [IN] Download session.
    taf_update_State_t state              ///< [IN] Download state to be reported to client.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    if (state == TAF_UPDATE_DOWNLOAD_SUCCESS || state == TAF_UPDATE_DOWNLOAD_FAIL)
    {
        sessPtr->state = TAF_UPDATE_IDLE;
    }
    else
    {
        sessPtr->state = state;
    }

    taf_update_StateInd_t report;
    report.percent = sessPtr->percent;
    report.state = state;
    le_utf8_Copy(report.name, "download session", TAF_UPDATE_SESSION_NAME_LEN, NULL);
    le_event_Report(tafUpdate.stateEvId, &report, sizeof(taf_update_StateInd_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Download timer handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::DownloadTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Download timer reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateDownloadSession_t* sessPtr =
        (taf_UpdateDownloadSession_t*)le_timer_GetContextPtr(timerRef);

    taf_pi_da_Status_t status = TAF_PI_DA_STATUS_ERROR;
    int ret = (*(tafUpdate.daInfPtr->getProgress))(sessPtr->sessRef, &status, &sessPtr->percent,
        &sessPtr->error);
    if (ret)
    {
        LE_ERROR("DA plug-in get download progress failed, ret = %d.", ret);
        le_timer_Stop(timerRef);
        tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_FAIL);
    }
    else
	{
        switch (status)
        {
            case TAF_PI_DA_STATUS_INIT:
                LE_INFO("DA plug-in current status is download init.");
                break;
            case TAF_PI_DA_STATUS_DOWNLOADING:
                LE_INFO("DA plug-in current status is downloading, percent = %d.",
                    sessPtr->percent);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOADING);
                break;
            case TAF_PI_DA_STATUS_PAUSED:
                LE_INFO("DA plug-in current status is download paused, percent = %d.",
                    sessPtr->percent);
                le_timer_Stop(timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED);
                break;
            case TAF_PI_DA_STATUS_FINISH:
                LE_INFO("DA plug-in current status is download finish.");
                le_timer_Stop(timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_SUCCESS);
                break;
            case TAF_PI_DA_STATUS_ERROR:
                LE_WARN("DA plug-in current status is download error, error code = %d.",
                    sessPtr->error);
                le_timer_Stop(timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_FAIL);
                break;
            default:
                LE_ERROR("DA plug-in current status is unknown, status = %d.", status);
                le_timer_Stop(timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_FAIL);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Download handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::DownloadHandler
(
    void* reqPtr ///< [IN] Download request pointer.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateDlReq_t* dlReq = (taf_UpdateDlReq_t*)reqPtr;
    taf_UpdateDownloadSession_t* sessPtr = dlReq->sessPtr;

    switch (sessPtr->state)
    {
        case TAF_UPDATE_IDLE:
            if (dlReq->event == TAF_UPDATE_DL_START)
            {
                LE_INFO("DA plug-in start to download.");
                sessPtr->state = TAF_UPDATE_DOWNLOADING;
                sessPtr->percent = 0;
                int ret = (*(tafUpdate.daInfPtr->startDownload))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("DA plug-in download failed, ret = %d.", ret);
                    tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_FAIL);
                }
                else
                {
                    if (tafUpdate.daInfPtr->getProgress == NULL)
                    {
                        LE_WARN("DA plug-in get download progress is not supported.");
                        tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_SUCCESS);
                    }
                    else
                    {
                        LE_INFO("Start download timer.");
                        le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                        le_timer_Start(sessPtr->timerRef);
                    }
                }
            }
            else
            {
                LE_ERROR("DA plug-in is in idle state, invalid event(%d).", dlReq->event);
            }
            break;
        case TAF_UPDATE_DOWNLOADING:
            if (dlReq->event == TAF_UPDATE_DL_PAUSE)
            {
                LE_INFO("DA plug-in pause download.");
                int ret = (*(tafUpdate.daInfPtr->pauseDownload))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("DA plug-in pause download failed, ret = %d.", ret);
                }
                else
                {
                    LE_INFO("Stop download timer.");
                    le_timer_Stop(sessPtr->timerRef);

                    tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED);
                }
            }
            else if (dlReq->event == TAF_UPDATE_DL_CANCEL)
            {
                LE_INFO("DA plug-in cancel download.");
                int ret = (*(tafUpdate.daInfPtr->cancelDownload))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("DA plug-in cancel download failed, ret = %d.", ret);
                }
                else
                {
                    LE_INFO("Stop download timer.");
                    le_timer_Stop(sessPtr->timerRef);

                    tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_IDLE);
                }
            }
            else
            {
                LE_ERROR("DA plug-in is in downloading state, invalid event(%d).", dlReq->event);
            }
            break;
        case TAF_UPDATE_DOWNLOAD_PAUSED:
            if (dlReq->event == TAF_UPDATE_DL_RESUME)
            {
                LE_INFO("DA plug-in resume download.");
                int ret = (*(tafUpdate.daInfPtr->resumeDownload))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("DA plug-in resume download failed, ret = %d.", ret);
                }
                else
                {
                    LE_INFO("Restart download timer.");
                    le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                    le_timer_Start(sessPtr->timerRef);

                    tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED);
                }
            }
            else if (dlReq->event == TAF_UPDATE_DL_CANCEL)
            {
                LE_INFO("DA plug-in cancel download.");
                int ret = (*(tafUpdate.daInfPtr->cancelDownload))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("DA plug-in cancel download failed, ret = %d.", ret);
                }
                else
                {
                    tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_IDLE);
                }
            }
            else
            {
                LE_ERROR("DA plug-in is in download paused state, invalid event(%d).",
                    dlReq->event);
            }
            break;
        default:
            LE_ERROR("DA plug-in is in unknown state, state = %d.", sessPtr->state);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Report update status.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::ReportUpdateStatus
(
    taf_UpdatePlugInSession_t* sessPtr, ///< [IN] Update Plug-In session.
    taf_update_State_t state            ///< [IN] Update state to be reported to client.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    if (state == TAF_UPDATE_INSTALL_SUCCESS || state == TAF_UPDATE_INSTALL_FAIL)
    {
        sessPtr->state = TAF_UPDATE_IDLE;
    }
    else
    {
        sessPtr->state = state;
    }

    taf_update_StateInd_t report;
    report.percent = sessPtr->percent;
    report.state = state;
    le_utf8_Copy(report.name, "update plugin session", TAF_UPDATE_SESSION_NAME_LEN, NULL);
    le_event_Report(tafUpdate.stateEvId, &report, sizeof(taf_update_StateInd_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Update timer handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::UpdateTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Update timer reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdatePlugInSession_t* sessPtr =
        (taf_UpdatePlugInSession_t*)le_timer_GetContextPtr(timerRef);

    taf_pi_ua_Status_t status = TAF_PI_UA_STATUS_INIT;
    int ret = (*(tafUpdate.uaInfPtr->getProgress))(sessPtr->sessRef, &status, &sessPtr->percent,
        &sessPtr->error);
    if (ret)
    {
        LE_ERROR("UA plug-in get update progress failed, ret = %d.", ret);
        le_timer_Stop(timerRef);
        tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_FAIL);
    }
    else
    {
        switch (status)
        {
            case TAF_PI_UA_STATUS_INIT:
                LE_INFO("UA plug-in current status is update init.");
                break;
            case TAF_PI_UA_STATUS_UPDATING:
                LE_INFO("UA plug-in current status updating, percent = %d.",
                    sessPtr->percent);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALLING);
                break;
            case TAF_PI_UA_STATUS_PAUSED:
                LE_INFO("UA plug-in current status paused, percent = %d.",
                    sessPtr->percent);
                le_timer_Stop(timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_PAUSED);
                break;
            case TAF_PI_UA_STATUS_FINISH:
                LE_INFO("UA plug-in current status is update finish.");
                le_timer_Stop(timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_SUCCESS);
                break;
            case TAF_PI_UA_STATUS_ERROR:
                LE_INFO("UA plug-in current status is update error, error code = %d.",
                    sessPtr->error);
                le_timer_Stop(timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_FAIL);
                break;
            default:
                LE_ERROR("UA plug-in current status is unknown, status = %d.", status);
                le_timer_Stop(timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_FAIL);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::UpdateHandler
(
    void* reqPtr ///< [IN] Update request pointer.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateReq_t* updateReq = (taf_UpdateReq_t*)reqPtr;
    taf_UpdatePlugInSession_t* sessPtr = updateReq->sessPtr;

    switch (sessPtr->state)
    {
        case TAF_UPDATE_IDLE:
            if (updateReq->event == TAF_UPDATE_INST_START)
            {
                LE_INFO("UA plug-in start to install.");
                sessPtr->state = TAF_UPDATE_INSTALLING;
                sessPtr->percent = 0;
                int ret = (*(tafUpdate.uaInfPtr->startInstall))(sessPtr->sessRef,
                    updateReq->filePath);
                if (ret)
                {
                    LE_ERROR("UA plug-in install failed, ret = %d.", ret);
                    tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_FAIL);
                }
                else
                {
                    if (tafUpdate.uaInfPtr->getProgress == NULL)
                    {
                        LE_WARN("UA plug-in get update progress is not supported.");
                        tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_SUCCESS);
                    }
                    else
                    {
                        LE_INFO("Start update timer.");
                        le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                        le_timer_Start(sessPtr->timerRef);
                    }
                }
            }
            else
            {
                LE_ERROR("UA plug-in is in idle state, invalid event(%d).", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALLING:
            if (updateReq->event == TAF_UPDATE_INST_PAUSE)
            {
                LE_INFO("UA plug-in pause installation.");
                int ret = (*(tafUpdate.uaInfPtr->pauseInstall))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("UA plug-in pause installation failed, ret = %d.", ret);
                }
                else
                {
                    if (tafUpdate.uaInfPtr->getProgress == NULL)
                    {
                        LE_ERROR("UA plug-in get update progress is not supported.");
                    }
                }
            }
            else
            {
                LE_ERROR("UA plug-in is in installing state, invalid event(%d).", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALL_PAUSED:
            if (updateReq->event == TAF_UPDATE_INST_RESUME)
            {
                LE_INFO("UA plug-in resume installation.");
                int ret = (*(tafUpdate.uaInfPtr->resumeInstall))(sessPtr->sessRef);
                if (ret)
                {
                    LE_ERROR("UA plug-in resume installation failed, ret = %d.", ret);
                }
                else
                {
                    if (tafUpdate.uaInfPtr->getProgress == NULL)
                    {
                        LE_ERROR("UA plug-in get update progress is not supported.");
                    }
                    else
                    {
                        LE_INFO("Restart update timer.");
                        le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                        le_timer_Start(sessPtr->timerRef);
                    }
                }
            }
            else
            {
                LE_ERROR("UA plug-in is in installing state, invalid event(%d).", updateReq->event);
            }
            break;
        default:
            LE_ERROR("UA plug-in is in unknown state, state = %d.", sessPtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::Init
(
    void
)
{
    chrono::time_point<chrono::system_clock> startTime = chrono::system_clock::now();

    // 1. Create events.
    stateEvId = le_event_CreateId("stateEvId", sizeof(taf_update_StateInd_t));
    downloadEvId = le_event_CreateId("downloadEvId", sizeof(taf_UpdateDlReq_t));
    updatePiEvId = le_event_CreateId("updatePiEvId", sizeof(taf_UpdateReq_t));

    // 2. Create session reference map and pool.
    sessionMap = le_ref_InitStaticMap(sessionMap, TAF_UPDATE_SESSION_NUM);
    sessionPool = le_mem_InitStaticPool(sessionPool, TAF_UPDATE_SESSION_NUM,
        sizeof(taf_UpdateSession_t));

    taf_UpdateSession_t* sessPtr = NULL;
    // 3. Load Download agent plug-in module.
    daInfPtr = (da_Inf_t *)taf_devMgr_LoadDrv(TAF_DA_MODULE_NAME, NULL);
    if (daInfPtr == NULL)
    {
        LE_WARN("Please install DA module to support download.");
    }
    else
    {
        sessPtr = (taf_UpdateSession_t*)le_mem_ForceAlloc(sessionPool);
        sessPtr->sessType = TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD;

        // Create download timer.
        sessPtr->dlSess.timerRef = le_timer_Create("Download Timer");
        le_timer_SetMsInterval(sessPtr->dlSess.timerRef, 1000);
        le_timer_SetRepeat(sessPtr->dlSess.timerRef, 0);
        le_timer_SetHandler(sessPtr->dlSess.timerRef, DownloadTimerHandler);

        // Initiate download state.
        sessPtr->dlSess.state = TAF_UPDATE_IDLE;
        dlSessRef = (taf_update_SessionRef_t)le_ref_CreateRef(sessionMap, (void*)sessPtr);
        if (daInfPtr->init != NULL)
        {
            LE_INFO("DA plug-in init...");
            (*(daInfPtr->init))();
        }
    }

    // 4. Load Update agent plug-in module.
    uaInfPtr = (ua_Inf_t *)taf_devMgr_LoadDrv(TAF_UA_MODULE_NAME, NULL);
    if (uaInfPtr == NULL)
    {
        LE_WARN("UA module not installed.");
    }
    else
    {
        sessPtr = (taf_UpdateSession_t*)le_mem_ForceAlloc(sessionPool);
        sessPtr->sessType = TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE;

        // Create update timer.
        sessPtr->upiSess.timerRef = le_timer_Create("Update Timer");
        le_timer_SetMsInterval(sessPtr->upiSess.timerRef, TAF_UPDATE_TIMER_INTERVAL);
        le_timer_SetRepeat(sessPtr->upiSess.timerRef, 0);
        le_timer_SetHandler(sessPtr->upiSess.timerRef, UpdateTimerHandler);

        // Initiate update state.
        sessPtr->upiSess.state = TAF_UPDATE_IDLE;
        upiSessRef = (taf_update_SessionRef_t)le_ref_CreateRef(sessionMap, (void*)sessPtr);
        if (uaInfPtr->init != NULL)
        {
            LE_INFO("UA plug-in init...");
            (*(uaInfPtr->init))();
        }
    }

    // 4. Create reference and structure for installation sessions.
    sessPtr = (taf_UpdateSession_t*)le_mem_ForceAlloc(sessionPool);
    sessPtr->sessType = TAF_UPDATE_SESSION_TYPE_QOTA_PARSE;
    qotaSessRef = (taf_update_SessionRef_t)le_ref_CreateRef(sessionMap, (void*)sessPtr);

    sessPtr = (taf_UpdateSession_t*)le_mem_ForceAlloc(sessionPool);
    sessPtr->sessType = TAF_UPDATE_SESSION_TYPE_FW_UPDATE;
    fwSessRef = (taf_update_SessionRef_t)le_ref_CreateRef(sessionMap, (void*)sessPtr);

    sessPtr = (taf_UpdateSession_t*)le_mem_ForceAlloc(sessionPool);
    sessPtr->sessType = TAF_UPDATE_SESSION_TYPE_APP_UPDATE;
    appSessRef = (taf_update_SessionRef_t)le_ref_CreateRef(sessionMap, (void*)sessPtr);

    // 5. Register event handler.
    le_event_AddHandler("DownloadHandler", downloadEvId, DownloadHandler);
    le_event_AddHandler("UpdateHandler", updatePiEvId, UpdateHandler);

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafUpdate component: %lfs.", elapsedTime.count());
}
