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
#include <fstream>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"

using namespace std;
using namespace telux::tafsvc;

le_event_Id_t taf_FwUpdate::fwUpdateEvId = nullptr;

/*======================================================================
 FUNCTION        taf_FwUpdate::GetInstance
 DESCRIPTION     Get a instance of taf_FwUpdate
 PARAMETERS      void
 RETURN VALUE    taf_FwUpdate: Instance reference
======================================================================*/
taf_FwUpdate &taf_FwUpdate::GetInstance()
{
    static taf_FwUpdate instance;
    return instance;
}

/*======================================================================
 FUNCTION        taf_FwUpdate::NotifyProgress
 DESCRIPTION     Get a instance of taf_FwUpdate
 PARAMETERS      [IN] state: FOTA state
                 [IN] percent: Percentage of progress
 RETURN VALUE    taf_FwUpdate: Instance reference
======================================================================*/
void taf_FwUpdate::NotifyProgress(taf_update_State_t state, uint32_t percent)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;

    stateInd.ota = TAF_UPDATE_FOTA;
    stateInd.percent = percent;
    stateInd.state = state;
    if (tafFwUpdate.fotaState != state) {
        tafFwUpdate.fotaState = state;
        tafUpdate.WriteFs(TAF_FWUPDATE_FOTA_STATE, (uint8_t*)&state, sizeof(taf_update_State_t));
    }
    le_utf8_Copy(stateInd.name, "firmware", TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
    le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
}

/*======================================================================
 FUNCTION        taf_FwUpdate::ReportState
 DESCRIPTION     Report FOTA result to server
 PARAMETERS      [IN] rState: Report state
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::ReportState(taf_update_ReportState_t rState)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();
    taf_FwUpdateReq_t updateReq;

    tafFwUpdate.fotaState = TAF_UPDATE_REPORTING;
    tafUpdate.WriteFs(TAF_FWUPDATE_FOTA_STATE, (uint8_t*)&tafFwUpdate.fotaState, sizeof(taf_update_State_t));
    updateReq.event = TAF_FWUPDATE_EV_START_REPORT;
    tafFwUpdate.rState = rState;
    tafUpdate.WriteFs(TAF_FWUPDATE_FOTA_REPORT_STATE, (uint8_t*)&rState, sizeof(taf_update_ReportState_t));
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
}

/*======================================================================
 FUNCTION        taf_FwUpdate::SendPipeCmd
 DESCRIPTION     Report FOTA result to server
 PARAMETERS      [IN] cmd: Pipe command
                 [IN] mode: Pipe open mode.
 RETURN VALUE    le_result_t: Result of sending pipe command
======================================================================*/
le_result_t taf_FwUpdate::SendPipeCmd(const char* cmd, const char* mod)
{
    FILE* fp = popen(cmd, mod);
    TAF_ERROR_IF_RET_VAL(fp == NULL, LE_FAULT, "popen failed.");

    int res = pclose(fp);
    if (WIFEXITED(res)) {
        res = WEXITSTATUS(res);
    }

    TAF_ERROR_IF_RET_VAL(res != 0, LE_FAULT, "pclose errno(%d), result(%d).", errno, res);

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_FwUpdate::ProbationTimerHandler
 DESCRIPTION     Probation timer handler for firmware
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::ProbationTimerHandler(le_timer_Ref_t timerRef)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    LE_INFO("Probation timer stopped.");

#ifdef TARGET_SA515M
    LE_INFO("Sending OTA sync message to MRC darmon.");

    if (taf_mrc_SendOtaAbsyncMsg() != LE_OK) {
        LE_ERROR("Fail to send OTA AB Sync message to MRC daemon.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_PROBATION_FAIL, 0);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }

    LE_INFO("Sync active slot to inactive slot successfully.");
#endif

    tafFwUpdate.NotifyProgress(TAF_UPDATE_PROBATION_SUCCESS, 0);
    tafFwUpdate.ReportState(TAF_UPDATE_REPORT_SUCCESS);
}

/*======================================================================
 FUNCTION        taf_FwUpdate::ReportTimerHandler
 DESCRIPTION     Report timer handler for firmware
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::ReportTimerHandler(le_timer_Ref_t timerRef)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    uint32_t expiryCnt = le_timer_GetExpiryCount(timerRef);
    LE_INFO("Report times : %d.", expiryCnt);

    int ret = taf_pa_update_Report(tafUpdate.daSessionID, tafFwUpdate.rState);
    if (ret) {
        LE_ERROR("Download agent report failed, ret = %d.", ret);
    } else {
        LE_INFO("Download agent report successfully.");

        LE_INFO("Stop report timer.");
        le_timer_Stop(timerRef);
        tafFwUpdate.NotifyProgress(TAF_UPDATE_IDLE, 0);
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::InstallFirmware
 DESCRIPTION     Install firmware
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::InstallFirmware()
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    uint32_t percent = 0;

    LE_INFO("Checking firmware update package.");

    if (access(TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH, 0)) {
        LE_ERROR("Firmware update package not found.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }

    percent = TAF_FWUPDATE_PROG_CHECK_FILE;
    tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALLING, percent);

#ifdef TARGET_SA515M
    LE_INFO("Sending OTA start message to MRC darmon.");
    if (taf_mrc_SendOtaStartMsg() != LE_OK) {
        LE_ERROR("Fail to send OTA start message to MRC daemon.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }

    percent = TAF_FWUPDATE_PROG_MRC_START;
    tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALLING, percent);
#endif

    LE_INFO("Recovery client installing.");

    char instCmd[TAF_FWUPDATE_INSTALL_CMD_LEN];
    snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s", TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH);

    if (tafFwUpdate.SendPipeCmd(instCmd, "w") != LE_OK) {
        LE_ERROR("Fail to send pipe cmd.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }

    percent = TAF_FWUPDATE_PROG_RECOVERY_INSTALL;
    tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALLING, percent);

    LE_INFO("Checking recovery log.");

    ifstream fin(TAF_FWUPDATE_RECOVERY_LOG_FILE);
    string strline;
    int line = 0;
    le_result_t ret = LE_FAULT;
    while (getline(fin, strline)) {
        line++;
        if (!(strline.find("Starting recovery") == string::npos)) {
            LE_DEBUG("Found Starting recovery in line %d", line);
            ret = LE_FAULT;
        }

        if (!(strline.find("upgrade success") == string::npos)) {
            LE_DEBUG("Found upgrade success in line %d", line);
            ret = LE_OK;
        }
    }
    fin.close();
    if (ret != LE_OK) {
        LE_ERROR("Error found in recovery log.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }

    percent = TAF_FWUPDATE_PROG_CHECK_LOG;
    tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALLING, percent);

#ifdef TARGET_SA515M
    LE_INFO("Sending OTA end message to MRC darmon.");

    if (taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS) != LE_OK) {
        LE_ERROR("Fail to send OTA end message to MRC daemon.");
        tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_FAIL, percent);
        tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
        return;
    }
#endif

    percent = TAF_FWUPDATE_PROG_SUCCESS;
    tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALL_SUCCESS, percent);

    LE_INFO("Install firmware successfully.");
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwUpdateHandler
 DESCRIPTION     Firmware update handler
 PARAMETERS      [IN] reqPtr: firmware update request
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::FwUpdateHandler(void* reqPtr)
{
    taf_FwUpdateReq_t* updateReq = (taf_FwUpdateReq_t*)reqPtr;
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    switch (tafFwUpdate.fotaState) {
        case TAF_UPDATE_IDLE:
            if (updateReq->event == TAF_FWUPDATE_EV_START_INSTALL) {
                LE_INFO("Start to install firmware.");
                tafFwUpdate.NotifyProgress(TAF_UPDATE_INSTALLING, TAF_FWUPDATE_PROG_INIT);
                tafFwUpdate.InstallFirmware();
            } else {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (updateReq->event == TAF_FWUPDATE_EV_START_PROBATION) {
                LE_INFO("Start probation for firmware.");
                tafFwUpdate.NotifyProgress(TAF_UPDATE_PROBATION, 0);
                le_timer_Start(tafFwUpdate.prbtTimerRef);
            } else if (updateReq->event == TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE) {
                LE_INFO("Install success, rebooting to active slot.");
                if (reboot(RB_AUTOBOOT) == -1) {
                    LE_FATAL("Fail to reboot. Errno = %s.", LE_ERRNO_TXT(errno));
                }
            } else {
                LE_ERROR("Invalid operation (%d) for install success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            if (updateReq->event == TAF_FWUPDATE_EV_REPORT_SUCCESS) {
                LE_INFO("Probation success.");
                tafFwUpdate.ReportState(TAF_UPDATE_REPORT_SUCCESS);
            } else {
                LE_ERROR("Invalid operation (%d) for probation success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_REPORTING:
            if (updateReq->event == TAF_FWUPDATE_EV_START_REPORT) {
                if (tafFwUpdate.isLocalUpgrade) {
                    LE_INFO("Local firmware upgrade, skip reporting to server.");
                    tafFwUpdate.NotifyProgress(TAF_UPDATE_IDLE, 0);
                } else {
                    LE_INFO("Start reporting to server.");
                    auto &tafUpdate = taf_Update::GetInstance();
                    int ret = taf_pa_update_Report(tafUpdate.daSessionID, tafFwUpdate.rState);
                    if (ret) {
                        LE_ERROR("Download agent report failed, ret = %d.", ret);

                        LE_INFO("Start timer to report status until network available.");
                        le_timer_Start(tafFwUpdate.rptTimerRef);
                    } else {
                        LE_INFO("Report to server successfully.");
                        tafFwUpdate.NotifyProgress(TAF_UPDATE_IDLE, 0);
                    }
                }
            } else {
                LE_ERROR("Invalid operation (%d) for reporting state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALLING:
        case TAF_UPDATE_INSTALL_FAIL:
        case TAF_UPDATE_PROBATION:
        case TAF_UPDATE_PROBATION_FAIL:
            if (updateReq->event == TAF_FWUPDATE_EV_REPORT_FAIL) {
                LE_INFO("Install firmware or probation with error.");
                tafFwUpdate.ReportState(TAF_UPDATE_REPORT_FAILURE);
            } else {
                LE_ERROR("Invalid operation (%d) for current state (%d).", updateReq->event, tafFwUpdate.fotaState);
            }
            break;
        default:
            LE_ERROR("Invalid state (%d).", tafFwUpdate.fotaState);
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwUpdateThread
 DESCRIPTION     Thread for handling FOTA
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_FwUpdate::FwUpdateThread(void* contextPtr)
{
#ifdef TARGET_SA515M
    taf_mrc_ConnectService();
#endif

    le_event_AddHandler("fwUpdateHandler", fwUpdateEvId, FwUpdateHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

/*======================================================================
 FUNCTION        taf_FwUpdate::Init
 DESCRIPTION     Initialization of fwupdate component
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    // 1. Create event for firmware update.
    fwUpdateEvId = le_event_CreateId("fwUpdateEvId", sizeof(taf_FwUpdateReq_t));

    // 2. Create thread for firmware update.
    le_sem_Ref_t semaphore = le_sem_Create("fwUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("fwUpdateThread", FwUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 3. Create probation timer.
    prbtTimerRef = le_timer_Create("Firmware Probation Timer");
    le_timer_SetMsInterval(prbtTimerRef, TAF_FWUPDATE_PROBATION_TIME_INTERVAL);
    le_timer_SetHandler(prbtTimerRef, ProbationTimerHandler);

    // 4. Create report timer.
    rptTimerRef = le_timer_Create("Firmware Report Timer");
    le_timer_SetMsInterval(rptTimerRef, TAF_FWUPDATE_REPORT_TIME_INTERVAL);
    le_timer_SetRepeat(rptTimerRef, 0);
    le_timer_SetHandler(rptTimerRef, ReportTimerHandler);

    // 5. Initiate state from file.
    auto &tafUpdate = taf_Update::GetInstance();
    if (le_fs_Exists(TAF_FWUPDATE_FOTA_STATE)) {
        tafUpdate.ReadFs(TAF_FWUPDATE_FOTA_STATE, (uint8_t*)&fotaState, sizeof(taf_update_State_t));
    }

    if (le_fs_Exists(TAF_FWUPDATE_FOTA_REPORT_STATE)) {
        tafUpdate.ReadFs(TAF_FWUPDATE_FOTA_REPORT_STATE, (uint8_t*)&rState, sizeof(taf_update_ReportState_t));
    }

    if (le_fs_Exists(TAF_FWUPDATE_FOTA_IS_LOCAL)) {
        tafUpdate.ReadFs(TAF_FWUPDATE_FOTA_IS_LOCAL, (uint8_t*)&isLocalUpgrade, sizeof(bool));
    }

    // 6. Initiate event for current state.
    taf_FwUpdateReq_t updateReq;
    switch (fotaState) {
        case TAF_UPDATE_IDLE:
            LE_INFO("FOTA idle state.");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("FOTA install success state.");
            updateReq.event = TAF_FWUPDATE_EV_START_PROBATION;
            le_event_Report(fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("FOTA probation success state.");
            updateReq.event = TAF_FWUPDATE_EV_REPORT_SUCCESS;
            le_event_Report(fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
            break;
        case TAF_UPDATE_REPORTING:
            LE_INFO("FOTA reporting state.");
            updateReq.event = TAF_FWUPDATE_EV_START_REPORT;
            le_event_Report(fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
            break;
        case TAF_UPDATE_INSTALLING:
        case TAF_UPDATE_INSTALL_FAIL:
        case TAF_UPDATE_PROBATION:
        case TAF_UPDATE_PROBATION_FAIL:
        default:
            LE_ERROR("FOTA install or probation with state (%d).", fotaState);
            updateReq.event = TAF_FWUPDATE_EV_REPORT_FAIL;
            le_event_Report(fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
    }

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
