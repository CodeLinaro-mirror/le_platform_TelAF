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
 * Phase of download plug-in session. Every request is posted to the main thread.
 */
//--------------------------------------------------------------------------------------------------
const taf_UpdatePhase_t taf_Update::DownloadPhase =
    { TAF_UPDATE_DOWNLOADING, TAF_UPDATE_DOWNLOAD_PAUSED, true, true, false };

//--------------------------------------------------------------------------------------------------
/**
 * Phase of update plug-in session. UA plug-in does not support cancel.
 */
//--------------------------------------------------------------------------------------------------
const taf_UpdatePhase_t taf_Update::PlugInInstallPhase =
    { TAF_UPDATE_INSTALLING, TAF_UPDATE_INSTALL_PAUSED, true, false, false };

//--------------------------------------------------------------------------------------------------
/**
 * Constructor of operation controller.
 */
//--------------------------------------------------------------------------------------------------
taf_UpdateOpCtrl::taf_UpdateOpCtrl
(
)
    : run(RUN_IDLE), phase(), pauseReq(false), cancelReq(false), resumeQueued(false)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to start an operation (IPC side).
 *
 * @return
 *  - LE_OK   The operation is accepted and the controller enters running state.
 *  - LE_BUSY Another operation is running or paused.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_UpdateOpCtrl::RequestStart
(
    const taf_UpdatePhase_t& newPhase ///< [IN] Phase of the operation.
)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (run != RUN_IDLE)
    {
        LE_ERROR("Busy, current state %d.", (run == RUN_PAUSED) ? phase.paused : phase.running);
        return LE_BUSY;
    }

    phase = newPhase;
    run = RUN_RUNNING;
    pauseReq = false;
    cancelReq = false;
    resumeQueued = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to pause the running operation (IPC side).
 *
 * @return
 *  - LE_OK            Accepted, or already paused/pause pending.
 *  - LE_UNSUPPORTED   Pause is not supported in current phase.
 *  - LE_NOT_PERMITTED No such operation, or a cancel is pending.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_UpdateOpCtrl::RequestPause
(
    taf_update_State_t running, ///< [IN] Running state of the phase to be paused.
    bool* postPtr               ///< [OUT] True if caller must post the request to the worker.
)
{
    std::lock_guard<std::mutex> lock(mtx);
    *postPtr = false;

    if (run == RUN_IDLE || phase.running != running || cancelReq)
    {
        LE_ERROR("Pause is not permitted, run = %d, phase = %d, cancel = %d.",
            run, phase.running, cancelReq);
        return LE_NOT_PERMITTED;
    }

    TAF_ERROR_IF_RET_VAL(!phase.canPause, LE_UNSUPPORTED,
        "Pause is not supported in phase %d.", phase.running);

    if ((run == RUN_PAUSED && !resumeQueued) || pauseReq)
    {
        LE_INFO("Already paused or pause is pending.");
        return LE_OK;
    }

    pauseReq = true;
    *postPtr = !phase.workerPolls;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to resume the paused operation (IPC side).
 *
 * A resume while the pause is still pending withdraws the pause.
 *
 * @return
 *  - LE_OK            Accepted, or already running/resume pending.
 *  - LE_NOT_PERMITTED No such operation, or a cancel is pending.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_UpdateOpCtrl::RequestResume
(
    taf_update_State_t running, ///< [IN] Running state of the phase to be resumed.
    bool* postPtr               ///< [OUT] True if caller must post the request to the worker.
)
{
    std::lock_guard<std::mutex> lock(mtx);
    *postPtr = false;

    if (run == RUN_IDLE || phase.running != running || cancelReq)
    {
        LE_ERROR("Resume is not permitted, run = %d, phase = %d, cancel = %d.",
            run, phase.running, cancelReq);
        return LE_NOT_PERMITTED;
    }

    if (run == RUN_RUNNING || resumeQueued)
    {
        if (pauseReq)
        {
            LE_INFO("Withdraw pending pause request.");
            pauseReq = false;
        }
        return LE_OK;
    }

    resumeQueued = true;
    *postPtr = true;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to cancel the running or paused operation (IPC side).
 *
 * @return
 *  - LE_OK            Accepted, or cancel is already pending.
 *  - LE_UNSUPPORTED   Cancel is not supported in current phase.
 *  - LE_NOT_PERMITTED No such operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_UpdateOpCtrl::RequestCancel
(
    taf_update_State_t running, ///< [IN] Running state of the phase to be cancelled.
    bool* postPtr               ///< [OUT] True if caller must post the request to the worker.
)
{
    std::lock_guard<std::mutex> lock(mtx);
    *postPtr = false;

    if (run == RUN_IDLE || phase.running != running)
    {
        LE_ERROR("Cancel is not permitted, run = %d, phase = %d.", run, phase.running);
        return LE_NOT_PERMITTED;
    }

    if (cancelReq)
    {
        LE_INFO("Cancel is pending.");
        return LE_OK;
    }

    TAF_ERROR_IF_RET_VAL(!phase.canCancel, LE_UNSUPPORTED,
        "Cancel is not supported in phase %d.", phase.running);

    cancelReq = true;
    pauseReq = false;

    // A polling worker only needs an event when it is parked in paused state without a queued
    // resume, otherwise the running worker or the queued resume will pick up the cancel.
    *postPtr = !phase.workerPolls || (run == RUN_PAUSED && !resumeQueued);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checkpoint of a running worker (worker side).
 *
 * If a pause is pending the controller enters paused state atomically, so a resume issued after
 * this point is always posted to the worker.
 *
 * @return
 *  - TAF_UPDATE_CP_CANCEL   Worker must stop and finish as cancelled.
 *  - TAF_UPDATE_CP_PAUSE    Worker must stop and report paused state.
 *  - TAF_UPDATE_CP_CONTINUE Worker keeps running.
 */
//--------------------------------------------------------------------------------------------------
taf_UpdateCheckpoint_t taf_UpdateOpCtrl::Checkpoint
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (run != RUN_RUNNING)
        return TAF_UPDATE_CP_CONTINUE;

    if (cancelReq)
        return TAF_UPDATE_CP_CANCEL;

    if (pauseReq)
    {
        pauseReq = false;
        run = RUN_PAUSED;
        return TAF_UPDATE_CP_PAUSE;
    }

    return TAF_UPDATE_CP_CONTINUE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable pause and cancel for the remaining part of current operation (worker side).
 *
 * @return The request pending before the interrupt is disabled. A pending pause is dropped.
 */
//--------------------------------------------------------------------------------------------------
taf_UpdateCheckpoint_t taf_UpdateOpCtrl::DisableInterrupt
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);

    taf_UpdateCheckpoint_t cp = TAF_UPDATE_CP_CONTINUE;
    if (cancelReq)
        cp = TAF_UPDATE_CP_CANCEL;
    else if (pauseReq)
        cp = TAF_UPDATE_CP_PAUSE;

    pauseReq = false;
    phase.canPause = false;
    phase.canCancel = false;
    return cp;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a pause is pending.
 */
//--------------------------------------------------------------------------------------------------
bool taf_UpdateOpCtrl::IsPauseRequested
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    return (run == RUN_RUNNING) && pauseReq && !cancelReq;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a cancel is pending.
 */
//--------------------------------------------------------------------------------------------------
bool taf_UpdateOpCtrl::IsCancelRequested
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    return (run != RUN_IDLE) && cancelReq;
}

//--------------------------------------------------------------------------------------------------
/**
 * Drop the pending pause, used when the pause can not be performed.
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateOpCtrl::ClearPause
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    pauseReq = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Drop the pending cancel, used when the cancel can not be performed.
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateOpCtrl::ClearCancel
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    cancelReq = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * The operation is paused (worker side).
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateOpCtrl::OnPaused
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (run != RUN_IDLE)
    {
        run = RUN_PAUSED;
        pauseReq = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The queued resume is picked up by the worker (worker side).
 *
 * @return
 *  - true  The controller enters running state, worker must resume the operation.
 *  - false The operation is not paused or a cancel is pending, worker must not resume.
 */
//--------------------------------------------------------------------------------------------------
bool taf_UpdateOpCtrl::OnResume
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    resumeQueued = false;

    if (run != RUN_PAUSED || cancelReq)
        return false;

    run = RUN_RUNNING;
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * The operation is finished with success, failure or cancellation (worker side).
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateOpCtrl::OnFinished
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    run = RUN_IDLE;
    pauseReq = false;
    cancelReq = false;
    resumeQueued = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Restore the controller from persisted state at start-up.
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateOpCtrl::Restore
(
    const taf_UpdatePhase_t& newPhase, ///< [IN] Phase of the operation.
    bool paused                        ///< [IN] True if the operation is paused.
)
{
    std::lock_guard<std::mutex> lock(mtx);
    phase = newPhase;
    run = paused ? RUN_PAUSED : RUN_RUNNING;
    pauseReq = false;
    cancelReq = false;
    resumeQueued = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if there is no ongoing operation.
 */
//--------------------------------------------------------------------------------------------------
bool taf_UpdateOpCtrl::IsIdle
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    return run == RUN_IDLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the operation is paused.
 */
//--------------------------------------------------------------------------------------------------
bool taf_UpdateOpCtrl::IsPaused
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    return run == RUN_PAUSED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get running state of current phase, only valid if the controller is not idle.
 */
//--------------------------------------------------------------------------------------------------
taf_update_State_t taf_UpdateOpCtrl::GetRunningState
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    return phase.running;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get current state from the controller.
 */
//--------------------------------------------------------------------------------------------------
taf_update_State_t taf_UpdateOpCtrl::GetState
(
    void
)
{
    std::lock_guard<std::mutex> lock(mtx);
    switch (run)
    {
        case RUN_RUNNING:
            return phase.running;
        case RUN_PAUSED:
            return phase.paused;
        default:
            return TAF_UPDATE_IDLE;
    }
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
        if (write(fd, buffer, rdSize) == -1)
        {
            LE_WARN("write buffer error, for size: %ld.", rdSize);
        }

        pos += rdSize;
    }

    if (ret == LE_OK)
    {
        if (ftruncate(fd, size - TAF_UPDATE_QOTA_HEADER_SIZE) == 0)
        {
            LE_INFO("QOTA header removed.");
        }
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
 *
 * The operation controller is updated before the report is sent, so a client which receives the
 * report can issue the next request immediately.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::ReportDownloadStatus
(
    taf_UpdateDownloadSession_t* sessPtr, ///< [IN] Download session.
    taf_update_State_t state,             ///< [IN] Download state to be reported to client.
    taf_update_Error_t error              ///< [IN] Error to be reported to client.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    switch (state)
    {
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
        case TAF_UPDATE_DOWNLOAD_FAIL:
        case TAF_UPDATE_CANCELLED:
            sessPtr->state = TAF_UPDATE_IDLE;
            tafUpdate.dlCtrl.OnFinished();
            break;
        case TAF_UPDATE_DOWNLOAD_PAUSED:
            sessPtr->state = state;
            tafUpdate.dlCtrl.OnPaused();
            break;
        default:
            sessPtr->state = state;
            break;
    }

    taf_update_StateInd_t report;
    memset(&report, 0, sizeof(report));
    report.state = state;
    report.error = error;
    report.percent = sessPtr->percent;
    report.ota = TAF_UPDATE_NON_QOTA;
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
                LE_DEBUG("DA plug-in current status is download init.");
                break;
            case TAF_PI_DA_STATUS_DOWNLOADING:
                LE_DEBUG("DA plug-in current status is downloading, percent = %d.",
                    sessPtr->percent);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOADING);
                break;
            case TAF_PI_DA_STATUS_PAUSED:
                LE_DEBUG("DA plug-in current status is download paused, percent = %d.",
                    sessPtr->percent);
                le_timer_Stop(timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED);
                break;
            case TAF_PI_DA_STATUS_FINISH:
                LE_DEBUG("DA plug-in current status is download finish.");
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
 *
 * Requests are validated by the operation controller before posted. The handler re-checks the
 * controller because a pending request may be withdrawn or superseded before it is handled.
 * A failure of the plug-in is reported with current state and TAF_UPDATE_INVALID_OPERATION.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::DownloadHandler
(
    void* reqPtr ///< [IN] Download request pointer.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateOpCtrl &ctrl = tafUpdate.dlCtrl;

    taf_UpdateDlReq_t* dlReq = (taf_UpdateDlReq_t*)reqPtr;
    taf_UpdateDownloadSession_t* sessPtr = dlReq->sessPtr;
    int ret = 0;

    switch (dlReq->event)
    {
        case TAF_UPDATE_DL_START:
            LE_DEBUG("DA plug-in start to download.");
            sessPtr->state = TAF_UPDATE_DOWNLOADING;
            sessPtr->percent = 0;
            ret = (*(tafUpdate.daInfPtr->startDownload))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("DA plug-in download failed, ret = %d.", ret);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_FAIL);
            }
            else if (tafUpdate.daInfPtr->getProgress == NULL)
            {
                LE_WARN("DA plug-in get download progress is not supported.");
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_SUCCESS);
            }
            else
            {
                LE_DEBUG("Start download timer.");
                le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                le_timer_Start(sessPtr->timerRef);
            }
            break;
        case TAF_UPDATE_DL_PAUSE:
            if (!ctrl.IsPauseRequested())
            {
                LE_INFO("Pause download is withdrawn or superseded.");
                break;
            }

            LE_DEBUG("DA plug-in pause download.");
            ret = (*(tafUpdate.daInfPtr->pauseDownload))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("DA plug-in pause download failed, ret = %d.", ret);
                ctrl.ClearPause();
                tafUpdate.ReportDownloadStatus(sessPtr, ctrl.GetState(),
                    TAF_UPDATE_INVALID_OPERATION);
            }
            else
            {
                LE_DEBUG("Stop download timer.");
                le_timer_Stop(sessPtr->timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED);
            }
            break;
        case TAF_UPDATE_DL_RESUME:
            if (!ctrl.OnResume())
            {
                LE_INFO("Resume download is superseded.");
                break;
            }

            LE_DEBUG("DA plug-in resume download.");
            ret = (*(tafUpdate.daInfPtr->resumeDownload))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("DA plug-in resume download failed, ret = %d.", ret);
                ctrl.OnPaused();
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOAD_PAUSED,
                    TAF_UPDATE_INVALID_OPERATION);
            }
            else
            {
                LE_DEBUG("Restart download timer.");
                le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                le_timer_Start(sessPtr->timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_DOWNLOADING);
            }
            break;
        case TAF_UPDATE_DL_CANCEL:
            if (!ctrl.IsCancelRequested())
            {
                LE_INFO("Cancel download is superseded.");
                break;
            }

            LE_DEBUG("DA plug-in cancel download.");
            ret = (*(tafUpdate.daInfPtr->cancelDownload))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("DA plug-in cancel download failed, ret = %d.", ret);
                ctrl.ClearCancel();
                tafUpdate.ReportDownloadStatus(sessPtr, ctrl.GetState(),
                    TAF_UPDATE_INVALID_OPERATION);
            }
            else
            {
                LE_DEBUG("Stop download timer.");
                le_timer_Stop(sessPtr->timerRef);
                tafUpdate.ReportDownloadStatus(sessPtr, TAF_UPDATE_CANCELLED);
            }
            break;
        default:
            LE_ERROR("DA plug-in invalid event(%d).", dlReq->event);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Report update status.
 *
 * The operation controller is updated before the report is sent, so a client which receives the
 * report can issue the next request immediately.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::ReportUpdateStatus
(
    taf_UpdatePlugInSession_t* sessPtr, ///< [IN] Update Plug-In session.
    taf_update_State_t state,           ///< [IN] Update state to be reported to client.
    taf_update_Error_t error            ///< [IN] Error to be reported to client.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    switch (state)
    {
        case TAF_UPDATE_INSTALL_SUCCESS:
        case TAF_UPDATE_INSTALL_FAIL:
        case TAF_UPDATE_CANCELLED:
            sessPtr->state = TAF_UPDATE_IDLE;
            tafUpdate.upiCtrl.OnFinished();
            break;
        case TAF_UPDATE_INSTALL_PAUSED:
            sessPtr->state = state;
            tafUpdate.upiCtrl.OnPaused();
            break;
        default:
            sessPtr->state = state;
            break;
    }

    taf_update_StateInd_t report;
    memset(&report, 0, sizeof(report));
    report.state = state;
    report.error = error;
    report.percent = sessPtr->percent;
    report.ota = TAF_UPDATE_NON_QOTA;
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
 *
 * Requests are validated by the operation controller before posted. The handler re-checks the
 * controller because a pending request may be withdrawn or superseded before it is handled.
 * A failure of the plug-in is reported with current state and TAF_UPDATE_INVALID_OPERATION.
 */
//--------------------------------------------------------------------------------------------------
void taf_Update::UpdateHandler
(
    void* reqPtr ///< [IN] Update request pointer.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateOpCtrl &ctrl = tafUpdate.upiCtrl;

    taf_UpdateReq_t* updateReq = (taf_UpdateReq_t*)reqPtr;
    taf_UpdatePlugInSession_t* sessPtr = updateReq->sessPtr;
    int ret = 0;

    switch (updateReq->event)
    {
        case TAF_UPDATE_INST_START:
            LE_INFO("UA plug-in start to install.");
            sessPtr->state = TAF_UPDATE_INSTALLING;
            sessPtr->percent = 0;
            ret = (*(tafUpdate.uaInfPtr->startInstall))(sessPtr->sessRef, updateReq->filePath);
            if (ret)
            {
                LE_ERROR("UA plug-in install failed, ret = %d.", ret);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_FAIL);
            }
            else if (tafUpdate.uaInfPtr->getProgress == NULL)
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
            break;
        case TAF_UPDATE_INST_PAUSE:
            if (!ctrl.IsPauseRequested())
            {
                LE_INFO("Pause installation is withdrawn or superseded.");
                break;
            }

            LE_INFO("UA plug-in pause installation.");
            ret = (*(tafUpdate.uaInfPtr->pauseInstall))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("UA plug-in pause installation failed, ret = %d.", ret);
                ctrl.ClearPause();
                tafUpdate.ReportUpdateStatus(sessPtr, ctrl.GetState(),
                    TAF_UPDATE_INVALID_OPERATION);
            }
            else
            {
                LE_INFO("Stop update timer.");
                le_timer_Stop(sessPtr->timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_PAUSED);
            }
            break;
        case TAF_UPDATE_INST_RESUME:
            if (!ctrl.OnResume())
            {
                LE_INFO("Resume installation is superseded.");
                break;
            }

            LE_INFO("UA plug-in resume installation.");
            ret = (*(tafUpdate.uaInfPtr->resumeInstall))(sessPtr->sessRef);
            if (ret)
            {
                LE_ERROR("UA plug-in resume installation failed, ret = %d.", ret);
                ctrl.OnPaused();
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALL_PAUSED,
                    TAF_UPDATE_INVALID_OPERATION);
            }
            else
            {
                LE_INFO("Restart update timer.");
                le_timer_SetContextPtr(sessPtr->timerRef, (void*)sessPtr);
                le_timer_Start(sessPtr->timerRef);
                tafUpdate.ReportUpdateStatus(sessPtr, TAF_UPDATE_INSTALLING);
            }
            break;
        default:
            LE_ERROR("UA plug-in invalid event(%d).", updateReq->event);
            break;
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
        le_timer_SetWakeup(sessPtr->dlSess.timerRef, false);    // Disable wakeup
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
        le_timer_SetWakeup(sessPtr->upiSess.timerRef, false);    // Disable wakeup
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
