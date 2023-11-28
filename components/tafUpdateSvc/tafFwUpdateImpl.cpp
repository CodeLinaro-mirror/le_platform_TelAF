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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"

using namespace std;
using namespace telux::tafsvc;
using namespace boost::property_tree;

le_event_Id_t taf_FwUpdate::fwUpdateEvId = nullptr;
le_event_Id_t taf_FwUpdate::fwTimerEvId = nullptr;

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

//--------------------------------------------------------------------------------------------------
/**
 * Set update state.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetState
(
    taf_update_State_t state ///< [IN] Update state.
)
{
    le_fs_FileRef_t fileRef;
    if (le_fs_Open(TAF_FWUPDATE_FOTA_STATE, LE_FS_CREAT | LE_FS_WRONLY, &fileRef) == LE_OK)
    {
        le_fs_Write(fileRef, (uint8_t*)&state, sizeof(taf_update_State_t));
        le_fs_Close(fileRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get update state.
 */
//--------------------------------------------------------------------------------------------------
taf_update_State_t taf_FwUpdate::GetState
(
    void
)
{
    le_fs_FileRef_t fileRef;
    taf_update_State_t state = TAF_UPDATE_IDLE;
    if (le_fs_Open(TAF_FWUPDATE_FOTA_STATE, LE_FS_RDONLY, &fileRef) == LE_OK)
    {
        size_t size = sizeof(taf_update_State_t);
        le_fs_Read(fileRef, (uint8_t*)&state, &size);
        le_fs_Close(fileRef);
    }
    return state;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report Status.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::ReportStatus
(
    taf_update_State_t state, ///< [IN] Update state.
    uint32_t percent          ///< [IN] Update percent.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;
    stateInd.ota = TAF_UPDATE_FOTA;
    stateInd.percent = percent;
    stateInd.state = state;
    le_utf8_Copy(stateInd.name, tafFwUpdate.filePath, TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
    le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Update progress.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::UpdateProgress
(
    taf_update_State_t state ///< [IN] Update state.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    taf_FwUpdateTimerOp_t timerOp;

    // 1. Update state.
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%%...", tafFwUpdate.percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_INFO("Install failed.");
            timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            if (taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE) != LE_OK)
            {
                LE_ERROR("Fail to send OTA end message to MRC daemon.");
            }
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            tafFwUpdate.SetState(TAF_UPDATE_INSTALL_SUCCESS);
            break;
        case TAF_UPDATE_PROBATION:
            LE_INFO("Probation %d%%...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_PROBATION);
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Probation success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_INFO("Probation failed.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        default:
            break;
    }

    // 2. Report current status to user.
    tafFwUpdate.ReportStatus(state, tafFwUpdate.percent);
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

    uint32_t time = le_timer_GetExpiryCount(timerRef);

    if (time >= tafFwUpdate.prbtTime)
    {
        LE_INFO("Probation timer stopped.");

        if (tafFwUpdate.autoSync)
        {
            LE_INFO("Sending OTA sync message to MRC darmon.");
            if (taf_mrc_SendOtaAbsyncMsg() != LE_OK)
            {
                LE_ERROR("Fail to send OTA AB Sync message to MRC daemon.");
                tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                return;
            }
        }
        tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);
    }
    else
    {
        if (tafFwUpdate.prbtTime)
        {
            if (tafFwUpdate.autoSync)
            {
                tafFwUpdate.percent = time * 100 / (tafFwUpdate.prbtTime +
                    TAF_FWUPDATE_MRC_SYNC_TIME);
            }
            else
            {
                tafFwUpdate.percent = time * 100 / tafFwUpdate.prbtTime;
            }

            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION);
        }
        else
        {
            LE_ERROR("Invalid probation time.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Install timer handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::InstallTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Timer reference.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    uint32_t time = le_timer_GetExpiryCount(timerRef);

    if (tafFwUpdate.totalTime)
    {
        tafFwUpdate.percent = time * 100 / tafFwUpdate.totalTime;
        if (tafFwUpdate.percent > 100)
        {
            tafFwUpdate.percent = 100;
            LE_INFO("Waiting to complete the installation...");
            taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
        }
    }
    else
    {
        LE_WARN("Can not estimate time for installation.");
        taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
        le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
    }

    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALLING);
}

//--------------------------------------------------------------------------------------------------
/**
 * Install firmware.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::InstallFirmware
(
    const char* filePath ///< [IN] File path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Initiate status.
    tafFwUpdate.percent = 0;
    tafFwUpdate.SetState(TAF_UPDATE_INSTALLING);

    // 2. Check if package exists
    LE_INFO("Checking FOTA package.");
    if (access(filePath, 0))
    {
        LE_ERROR("FOTA package not found.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 3. Evaluate time for installation.
    ifstream infile(filePath);
    infile.seekg (0, ios::end);
    tafFwUpdate.totalTime = (uint32_t)(infile.tellg() / TAF_FWUPDATE_PROC_DATA_RATE)
        + TAF_FWUPDATE_PROC_MRC_TIME;
    LE_INFO("Estimate to complete installation in %d s.", tafFwUpdate.totalTime);
    infile.close();
    le_utf8_Copy(tafFwUpdate.filePath, filePath, TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);

    // 4. Start timer to report progress.
    taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_INST_START;
    le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));

    // 5. Send OTA start message.
    LE_INFO("Sending OTA start message to MRC darmon.");
    if (taf_mrc_SendOtaStartMsg() != LE_OK)
   {
        LE_ERROR("Fail to send OTA start message to MRC daemon.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 6. Install pacackeg with recovery client.
    LE_INFO("recovery client installing.");
    char instCmd[TAF_FWUPDATE_INSTALL_CMD_LEN];
    snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s", filePath);
    if (tafFwUpdate.SendPipeCmd(instCmd, "w") != LE_OK)
    {
        LE_ERROR("Fail to send pipe cmd.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 7. Check log after installation.
    LE_INFO("Checking recovery log.");
    ifstream fin(TAF_FWUPDATE_RECOVERY_LOG_FILE);
    string strline;
    int line = 0;
    le_result_t ret = LE_FAULT;
    while (getline(fin, strline))
    {
        line++;
        if (!(strline.find("Starting recovery") == string::npos))
        {
            LE_DEBUG("Found Starting recovery in line %d", line);
            ret = LE_FAULT;
        }

        if (!(strline.find("upgrade success") == string::npos))
        {
            LE_DEBUG("Found upgrade success in line %d", line);
            ret = LE_OK;
        }
    }
    fin.close();
    if (ret != LE_OK)
    {
        LE_ERROR("Error found in recovery log.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 8. Send OTA end message.
    LE_INFO("Sending OTA end message to MRC darmon.");
    if (taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS) != LE_OK)
    {
        LE_ERROR("Fail to send OTA end message to MRC daemon.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 9. Mark install success with file.
    LE_INFO("Marking FOTA success.");
    le_fs_FileRef_t fileRef;
    ret = le_fs_Open("/INSTALL_SUCCESS", LE_FS_CREAT, &fileRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Fail to mark install success.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 10. Install successfully.
    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer option handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::TimerOpHandler
(
    void* contextPtr ///< [IN] Context.
)
{
    taf_FwUpdateTimerOp_t* op = (taf_FwUpdateTimerOp_t*)contextPtr;
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    switch (*op)
    {
        case TAF_FWUPDATE_TIMER_OP_INST_START:
            le_timer_Start(tafFwUpdate.instTimerRef);
            break;
        case TAF_FWUPDATE_TIMER_OP_INST_STOP:
            le_timer_Stop(tafFwUpdate.instTimerRef);
            break;
        case TAF_FWUPDATE_TIMER_OP_PRBT_START:
            le_timer_Start(tafFwUpdate.prbtTimerRef);
            break;
        case TAF_FWUPDATE_TIMER_OP_PRBT_STOP:
            le_timer_Stop(tafFwUpdate.prbtTimerRef);
            break;
        default:
            LE_ERROR("Invalid timer option (%d).", *op);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer thread.
 */
//--------------------------------------------------------------------------------------------------
void* taf_FwUpdate::TimerThread
(
    void* contextPtr ///< [IN] Context.
)
{
    taf_mrc_ConnectService();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Create probation timer.
    tafFwUpdate.prbtTimerRef = le_timer_Create("Firmware Probation Timer");
    le_timer_SetMsInterval(tafFwUpdate.prbtTimerRef, 1000);
    le_timer_SetHandler(tafFwUpdate.prbtTimerRef, ProbationTimerHandler);

    // 2. Create install timer.
    tafFwUpdate.instTimerRef = le_timer_Create("Firmware Install Timer");
    le_timer_SetMsInterval(tafFwUpdate.instTimerRef, 1000);
    le_timer_SetRepeat(tafFwUpdate.instTimerRef, 0);
    le_timer_SetHandler(tafFwUpdate.instTimerRef, InstallTimerHandler);

    // 3. Add handler for timer options.
    le_event_AddHandler("timerOpHandler", fwTimerEvId, TimerOpHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
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
    taf_update_State_t state = tafFwUpdate.GetState();

    switch (state)
    {
        case TAF_UPDATE_IDLE:
            if (updateReq->event == TAF_FWUPDATE_EV_INSTALL)
            {
                LE_INFO("FOTA start.");
                tafFwUpdate.InstallFirmware(updateReq->name);
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (updateReq->event == TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE)
            {
                LE_INFO("Reboot to active slot.");
                if (reboot(RB_AUTOBOOT) == -1)
                {
                    LE_FATAL("Fail to reboot. Errno = %s.", LE_ERRNO_TXT(errno));
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for install success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION:
            LE_ERROR("Invalid operation for probation state.");
            break;
        default:
            LE_ERROR("Invalid state (%d).", state);
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
    taf_mrc_ConnectService();

    le_event_AddHandler("fwUpdateHandler", fwUpdateEvId, FwUpdateHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::Init
(
    void
)
{
    chrono::time_point<chrono::system_clock> startTime = chrono::system_clock::now();

    // 1. Get configurations from json file.
    ptree root;
    read_json("tafUpdate.json", root);
    prbtTime = root.get<uint32_t>("firmware.probation.time");
    autoSync = root.get<bool>("firmware.probation.auto_sync");
    taf_update_State_t state = GetState();

    // 2. Create event for firmware update.
    fwUpdateEvId = le_event_CreateId("fwUpdateEvId", sizeof(taf_FwUpdateReq_t));
    fwTimerEvId = le_event_CreateId("fwTimerEvId", sizeof(taf_FwUpdateTimerOp_t));

    // 3. Create thread for firmware update.
    le_sem_Ref_t semaphore = le_sem_Create("fwUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("fwUpdateThread", FwUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 4. Create thread for timer.
    semaphore = le_sem_Create("fwTimerThreadSem", 0);
    threadRef = le_thread_Create("fwTimerThread", TimerThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 5. Initiate from current state.
    if (le_fs_Exists("/INSTALL_SUCCESS"))
    {
        LE_INFO("Removing file tag...");
        le_fs_Delete("/INSTALL_SUCCESS");
        if (prbtTime)
        {
            SetState(TAF_UPDATE_PROBATION);
            le_timer_SetRepeat(prbtTimerRef, prbtTime);
            taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_PRBT_START;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
        }
        else
        {
            LE_INFO("FOTA probation disabled.");
            SetState(TAF_UPDATE_IDLE);
        }
    }

    switch (state)
    {
        case TAF_UPDATE_IDLE:
            break;
        case TAF_UPDATE_INSTALLING:
            UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (prbtTime)
            {
                SetState(TAF_UPDATE_PROBATION);
                le_timer_SetRepeat(prbtTimerRef, prbtTime);
                taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_PRBT_START;
                le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            }
            else
            {
                LE_INFO("FOTA probation disabled.");
                if (autoSync)
                {
                    LE_INFO("Sending OTA sync message to MRC darmon.");
                    if (taf_mrc_SendOtaAbsyncMsg() != LE_OK)
                    {
                        LE_ERROR("Fail to send OTA AB Sync message to MRC daemon.");
                    }
                }
                SetState(TAF_UPDATE_IDLE);
            }
        case TAF_UPDATE_PROBATION:
            UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            break;
        default:
            LE_ERROR("FOTA invalid state(%d), reset to idle.", state);
            SetState(TAF_UPDATE_IDLE);
    }

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
