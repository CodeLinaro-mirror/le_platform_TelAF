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

    FILE* fp = fopen(TAF_FWUPDATE_FOTA_STATE, "w");
    fwrite(&state, sizeof(taf_update_State_t), 1, fp);
    fflush(fp);
    fclose(fp);
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
    taf_update_State_t state = TAF_UPDATE_IDLE;
    if (access(TAF_FWUPDATE_FOTA_STATE, F_OK) == 0)
    {
        FILE* fp = fopen(TAF_FWUPDATE_FOTA_STATE, "r");
        fread(&state, sizeof(taf_update_State_t), 1, fp);
        fclose(fp);
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
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t report;
    report.percent = percent;
    report.state = state;
    le_utf8_Copy(report.name, "firmware update session", TAF_UPDATE_SESSION_NAME_LEN, NULL);
    le_event_Report(tafUpdate.stateEvId, &report, sizeof(taf_update_StateInd_t));
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
            if (taf_mrc_SendOtaAbsyncMsg() != LE_OK)
            {
                LE_ERROR("Fail to send OTA sync message to MRC daemon.");
            }
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            tafFwUpdate.SetState(TAF_UPDATE_INSTALL_SUCCESS);
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Probation success.");
            tafFwUpdate.SetState(TAF_UPDATE_PROBATION_SUCCESS);
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_INFO("Probation success.");
            tafFwUpdate.SetState(TAF_UPDATE_PROBATION_FAIL);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_INFO("Synchronizing %d%%...", tafFwUpdate.percent);
            break;
        case TAF_UPDATE_SYNC_SUCCESS:
            LE_INFO("Sync success.");
            timerOp = TAF_FWUPDATE_TIMER_OP_SYNC_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_SYNC_FAIL:
            LE_INFO("Sync failed.");
            timerOp = TAF_FWUPDATE_TIMER_OP_SYNC_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            tafFwUpdate.SetState(TAF_UPDATE_SYNC_FAIL);
            break;
        case TAF_UPDATE_ROLLBACK_SUCCESS:
            LE_INFO("Rollback success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_ROLLBACK_FAIL:
            LE_INFO("Rollback failed.");
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

//--------------------------------------------------------------------------------------------------
/**
 * Sync timer handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SyncTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Timer reference.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    uint32_t time = le_timer_GetExpiryCount(timerRef);

    tafFwUpdate.percent = time * 100 / TAF_FWUPDATE_MRC_SYNC_TIME;

    if (tafFwUpdate.percent > 100)
    {
        tafFwUpdate.percent = 100;
        LE_INFO("Waiting to complete the synchronization...");
    }

    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);
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
 * Get rootfs version.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetRootfsVersion
(
    char* version ///< [OUT] Current rootfs version.
)
{
    std::ifstream rootfsFin(TAF_ROOTFS_VERSION_FILE);
    std::string rootfsVer;

    getline(rootfsFin, rootfsVer);

    le_utf8_Copy(version, rootfsVer.c_str(), TAF_FWUPDATE_MAX_VERS_LEN, NULL);

    rootfsFin.close();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get telaf version.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetTelafVersion
(
    char* version ///< [OUT] Current telaf version.
)
{
    std::ifstream telafFin(TAF_TELAF_VERSION_FILE);
    std::string telafVer;

    getline(telafFin, telafVer);

    le_utf8_Copy(version, telafVer.c_str(), TAF_TELAF_VERSION_LEN, NULL);

    telafFin.close();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get firmware version.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::GetFirmwareVersion
(
    char* version ///< [OUT] Current firmware version.
)
{
    std::ifstream firmwareFin(TAF_FIRMWARE_VERSION_FILE);
    std::string firmwareVer;

    size_t start = string::npos;
    while (getline(firmwareFin, firmwareVer))
    {
        start = firmwareVer.find("MPSS");
        if (start != string::npos)
            break;
    }
   
    size_t end = firmwareVer.find(",");
    if (start != string::npos && end != string::npos)
    {
        firmwareVer = firmwareVer.substr(start, end - start - 1);
    }
    else
    {
        LE_ERROR("Invalid character in current firmware version.");
        return LE_FAULT;
    }

    le_utf8_Copy(version, firmwareVer.c_str(), TAF_FWUPDATE_MAX_VERS_LEN, NULL);

    firmwareFin.close();

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Firmware installation pre-check.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::InstallPreCheck
(
    const char* manifest ///< [IN] File path for manifest.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (strncmp(manifest, TAF_FWUPDATE_BYPASS_CHECK_TAG,
        strlen(TAF_FWUPDATE_BYPASS_CHECK_TAG)) == 0)
    {
        LE_INFO("Bypass activation verification.");
        return LE_OK;
    }

    std::ifstream manifestFin(manifest);
    std::string manifestVer;

    char rootfsVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
    char telafVer[TAF_TELAF_VERSION_LEN] = {0};
    char firmwareVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};

    tafFwUpdate.GetRootfsVersion(rootfsVer);
    tafFwUpdate.GetTelafVersion(telafVer);
    le_result_t result = tafFwUpdate.GetFirmwareVersion(firmwareVer);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get current firmware version.");
        return LE_FAULT;
    }

    LE_INFO("Current rootfs version : %s", rootfsVer);
    LE_INFO("Current telaf version : %s", telafVer);
    LE_INFO("Current firmware version : %s", firmwareVer);

    // Check if rootfs version is downgraded.
    getline(manifestFin, manifestVer);
    size_t pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in rootfs version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), rootfsVer, strlen(rootfsVer)) < 0)
    {
        LE_ERROR("Detect rootfs version %s is downgraded.", manifestVer.c_str());
        return LE_FAULT;
    }

    // Check if firmware version is downgraded.
    getline(manifestFin, manifestVer);
    pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in firmware version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), firmwareVer, strlen(firmwareVer)) < 0)
    {
        LE_ERROR("Detect firmware version %s is downgraded.", manifestVer.c_str());
        return LE_FAULT;
    }

    // Check if telaf version is downgraded.
    getline(manifestFin, manifestVer);
    pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in telaf version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), telafVer, strlen(telafVer)) < 0)
    {
        LE_ERROR("Detect telaf version %s is downgraded.", manifestVer.c_str());
        return LE_FAULT;
    }

    // Close file stream.
    manifestFin.close();

    return LE_OK;
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

    // 9. Install successfully.
    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);

    // 10. Synchronize state to storage.
    if (tafFwUpdate.SendPipeCmd("sync", "w") != LE_OK)
    {
        LE_ERROR("Fail to send sync cmd.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Firmware installation post-check.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::InstallPostCheck
(
    const char* filePath ///< [IN] File path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    LE_INFO("recovery client start post-check.");
    char instCmd[TAF_FWUPDATE_INSTALL_CMD_LEN];
    snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s:--post_verify", filePath);
    if (tafFwUpdate.SendPipeCmd(instCmd, "w") != LE_OK)
    {
        LE_ERROR("Fail to send pipe cmd.");
        return LE_FAULT;
    }

    LE_INFO("Checking post-check log.");
    ifstream fin(TAF_FWUPDATE_RECOVERY_LOG_FILE);
    string strline;
    int line = 0;
    le_result_t ret = LE_OK;
    while (getline(fin, strline))
    {
        line++;
        if (strline.find("--post_verify") != string::npos)
        {
            LE_DEBUG("Found --post_verify in line %d", line);
            ret = LE_OK;
        }

        if (strline.find("partition has unexpected contents after OTA update") != string::npos)
        {
            LE_DEBUG("Found verification failure in line %d", line);
            ret = LE_FAULT;
        }
    }
    fin.close();

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get active bank.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::GetActiveBank
(
    taf_update_Bank_t* bankPtr ///< [OUT] The active bank.
)
{
    le_result_t result = LE_OK;
    char cmdRes[TAF_FWUPDATE_CMD_RESULT_LEN];

    FILE* fp = popen("/usr/bin/nad-abctl --boot_slot", "r");
    TAF_ERROR_IF_RET_VAL(fp == NULL, LE_FAULT, "popen failed.");

    fgets(cmdRes, sizeof(cmdRes), fp);
    string resStr(cmdRes);
    if (resStr.find("a") != string::npos)
    {
        *bankPtr = TAF_UPDATE_BANK_A;
    }
    else if (resStr.find("b") != string::npos)
    {
        *bankPtr = TAF_UPDATE_BANK_B;
    }
    else
    {
        LE_ERROR("Invalid result for getting active bank.");
        result = LE_FAULT;
    }

    pclose(fp);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify activation.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::VerifyActivation
(
    const char* manifest ///< [IN] File path for manifest.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (strncmp(manifest, TAF_FWUPDATE_BYPASS_CHECK_TAG,
        strlen(TAF_FWUPDATE_BYPASS_CHECK_TAG)) == 0)
    {
        LE_INFO("Bypass activation verification.");
        return LE_OK;
    }

    std::ifstream manifestFin(manifest);
    std::string manifestVer;

    char rootfsVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
    char telafVer[TAF_TELAF_VERSION_LEN] = {0};
    char firmwareVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};

    tafFwUpdate.GetRootfsVersion(rootfsVer);
    tafFwUpdate.GetTelafVersion(telafVer);
    le_result_t result = tafFwUpdate.GetFirmwareVersion(firmwareVer);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get current firmware version.");
        return LE_FAULT;
    }

    LE_INFO("Current rootfs version : %s", rootfsVer);
    LE_INFO("Current telaf version : %s", telafVer);
    LE_INFO("Current firmware version : %s", firmwareVer);

    // Check if rootfs version is an updated version.
    getline(manifestFin, manifestVer);
    size_t pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in rootfs version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), rootfsVer, strlen(rootfsVer)) != 0)
    {
        LE_ERROR("Detect rootfs version %s is not updated.", manifestVer.c_str());
        return LE_FAULT;
    }
    // Check if firmware version is an updated version.
    getline(manifestFin, manifestVer);
    pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in firmware version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), firmwareVer, strlen(firmwareVer)) != 0)
    {
        LE_ERROR("Detect firmware version %s is not updated.", manifestVer.c_str());
        return LE_FAULT;
    }

    // Check if telaf version is an updated version.
    getline(manifestFin, manifestVer);
    pos = manifestVer.find(":");
    if (pos == string::npos)
    {
        LE_ERROR("Invalid character in telaf version from manifest.");
        return LE_FAULT;
    }
    manifestVer = manifestVer.substr(pos + 1);
    if (strncmp(manifestVer.c_str(), telafVer, strlen(telafVer)) != 0)
    {
        LE_ERROR("Detect telaf version %s is not updated.", manifestVer.c_str());
        return LE_FAULT;
    }

    // Close file stream.
    manifestFin.close();

    return LE_OK;
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
        case TAF_FWUPDATE_TIMER_OP_SYNC_START:
            le_timer_Start(tafFwUpdate.syncTimerRef);
            break;
        case TAF_FWUPDATE_TIMER_OP_SYNC_STOP:
            le_timer_Stop(tafFwUpdate.syncTimerRef);
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
    tafFwUpdate.syncTimerRef = le_timer_Create("Firmware Sync Timer");
    le_timer_SetMsInterval(tafFwUpdate.syncTimerRef, 1000);
    le_timer_SetRepeat(tafFwUpdate.syncTimerRef, 0);
    le_timer_SetHandler(tafFwUpdate.syncTimerRef, SyncTimerHandler);

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
                tafFwUpdate.InstallFirmware(updateReq->filePath);
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
            else if (updateReq->event == TAF_FWUPDATE_EV_VERIFY_ACTIVATION)
            {
                LE_INFO("Activation verification.");

                if (tafFwUpdate.VerifyActivation(updateReq->filePath) != LE_OK)
                {
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                }
                else
                {
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for install success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            if (updateReq->event == TAF_FWUPDATE_EV_SYNC)
            {
                LE_INFO("Start bank synchronization.");
                tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);

                taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_SYNC_START;
                le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));

                if (taf_mrc_SendOtaAbsyncMsg() != LE_OK)
                {
                    LE_ERROR("Fail to send OTA AB Sync message to MRC daemon.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
                }
                else
                {
                    LE_INFO("Send OTA AB Sync message to MRC daemon successfully.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for probation success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            if (updateReq->event == TAF_FWUPDATE_EV_ROLLBACK)
            {
                LE_INFO("Start rollback.");

                if (access(updateReq->filePath, 0))
                {
                    LE_ERROR("%s not exists.", updateReq->filePath);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL);
                }
                else
                {
                    remove(updateReq->filePath);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for probation success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_ERROR("Invalid operation for synchronizing state.");
            break;
        case TAF_UPDATE_SYNC_FAIL:
            if (updateReq->event == TAF_FWUPDATE_EV_SYNC)
            {
                LE_INFO("Retry bank synchronization.");

                // Start timer to report progress.
                taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_SYNC_START;
                le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));

                if (taf_mrc_SendOtaAbsyncMsg() != LE_OK)
                {
                    LE_ERROR("Fail to send OTA AB Sync message to MRC daemon.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
                }
                else
                {
                    LE_ERROR("Send OTA AB Sync message to MRC daemon successfully.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for sync fail state.", updateReq->event);
            }
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

    // 1. Create event for firmware update.
    fwUpdateEvId = le_event_CreateId("fwUpdateEvId", sizeof(taf_FwUpdateReq_t));
    fwTimerEvId = le_event_CreateId("fwTimerEvId", sizeof(taf_FwUpdateTimerOp_t));

    // 2. Create thread for firmware update.
    le_sem_Ref_t semaphore = le_sem_Create("fwUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("fwUpdateThread", FwUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 3. Create thread for timer.
    semaphore = le_sem_Create("fwTimerThreadSem", 0);
    threadRef = le_thread_Create("fwTimerThread", TimerThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 4. Initiate from current state.
    taf_update_State_t state = GetState();
    switch (state)
    {
        case TAF_UPDATE_IDLE:
            break;
        case TAF_UPDATE_INSTALLING:
            UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            UpdateProgress(TAF_UPDATE_SYNC_FAIL);
        default:
            LE_ERROR("FOTA invalid state(%d), reset to idle.", state);
            SetState(TAF_UPDATE_IDLE);
    }

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
