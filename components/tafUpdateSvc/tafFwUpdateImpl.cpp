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
#include "tafUpdateConfigTreeHelper.hpp"

using namespace std;
using namespace telux::tafsvc;

le_event_Id_t taf_FwUpdate::fwUpdateEvId = nullptr;
le_event_Id_t taf_FwUpdate::fwTimerEvId = nullptr;
le_event_Id_t taf_FwUpdate::fwStartSyncEvId = nullptr;
le_event_Id_t taf_FwUpdate::fwSyncHandlerEvId = nullptr;

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
 * Check if bank is swicthed.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::IsBankSwitched
(
    void
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
    tafFwUpdate.GetActiveBank(&bootBank);

    if (access(TAF_FWUPDATE_PREVIOUS_BANK, F_OK) == 0)
    {
        taf_update_Bank_t previousBank = TAF_UPDATE_BANK_UNKNOWN;
        FILE* fp = fopen(TAF_FWUPDATE_PREVIOUS_BANK, "r");
        fread(&previousBank, sizeof(taf_update_Bank_t), 1, fp);
        fclose(fp);

        if (previousBank != bootBank)
            return true;
    }
    else
    {
        FILE* fp = fopen(TAF_FWUPDATE_PREVIOUS_BANK, "w");
        fwrite(&bootBank, sizeof(taf_update_Bank_t), 1, fp);
        fflush(fp);
        fclose(fp);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report Status.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::ReportStatus
(
    taf_update_State_t state, ///< [IN] Update state.
    uint32_t percent,         ///< [IN] Update percent.
    taf_update_Error_t error  ///< [IN] Update error.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t report;
    report.percent = percent;
    report.state = state;
    report.error = error;
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
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            timerOp = TAF_FWUPDATE_TIMER_OP_INST_STOP;
            le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));
            tafFwUpdate.SetState(TAF_UPDATE_INSTALL_SUCCESS);
            tafFwUpdate.error = TAF_UPDATE_IMAGE_NOT_VERFIED;
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Probation success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_INFO("Probation failed.");
            tafFwUpdate.SetState(TAF_UPDATE_PROBATION_FAIL);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_INFO("Synchronizing %d%%...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);
            break;
        case TAF_UPDATE_SYNC_SUCCESS:
            LE_INFO("Sync success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_SYNC_FAIL:
            LE_INFO("Sync failed.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
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
    tafFwUpdate.ReportStatus(state, tafFwUpdate.percent, tafFwUpdate.error);
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
    tafFwUpdate.totalTime = (uint32_t)(infile.tellg() / TAF_FWUPDATE_PROC_DATA_RATE);
    LE_INFO("Estimate to complete installation in %d s.", tafFwUpdate.totalTime);
    infile.close();

    // 4. Start timer to report progress.
    taf_FwUpdateTimerOp_t timerOp = TAF_FWUPDATE_TIMER_OP_INST_START;
    le_event_Report(taf_FwUpdate::fwTimerEvId, &timerOp, sizeof(taf_FwUpdateTimerOp_t));

    // 5. Install pacackeg with recovery client.
    LE_INFO("recovery client installing.");
    char instCmd[TAF_FWUPDATE_INSTALL_CMD_LEN];
    snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s", filePath);
    if (tafFwUpdate.SendPipeCmd(instCmd, "w") != LE_OK)
    {
        LE_ERROR("Fail to send pipe cmd.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 6. Check log after installation.
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

    // 7. Install successfully.
    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);

    // 8. Synchronize state to storage.
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
 * Set active bank.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::SetActiveBank
(
    taf_update_Bank_t bank ///< [IN] The bank to be activated.
)
{
    FILE* fp = NULL;
    if (bank == TAF_UPDATE_BANK_A)
    {
        fp = popen("/usr/bin/nad-abctl --set_acvtive 0", "r");
    }
    else if (bank == TAF_UPDATE_BANK_B)
    {
        fp = popen("/usr/bin/nad-abctl --set_acvtive 1", "r");
    }
    else
    {
        LE_ERROR("Invalid bank to set acvtive.");
        return LE_FAULT;
    }
    TAF_ERROR_IF_RET_VAL(fp == NULL, LE_FAULT, "popen failed.");

    pclose(fp);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase bank.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::EraseBank
(
    taf_update_Bank_t bank ///< [IN] The bank to be erased.
)
{

    taf_lib_flash_PartitionList_t partitionList;
    taf_lib_flash_Bank_t eraseBank = NOT_DUAL_BANK;

    switch (bank)
    {
        case TAF_UPDATE_BANK_A:
            eraseBank = DUAL_BANK_A;
            break;
        case TAF_UPDATE_BANK_B:
            eraseBank = DUAL_BANK_B;
            break;
        default:
            LE_ERROR("Invalid bank.");
            return LE_BAD_PARAMETER;
    }

    le_result_t result = taf_lib_flash_GetPartitionList(&partitionList);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can not get partition list.");

    for (uint32_t i = 0; i < partitionList.number; i++)
    {
        if (partitionList.partition[i].bank == eraseBank)
        {
            if (partitionList.partition[i].eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
            {
                LE_INFO("Erasing MTD %s...", partitionList.partition[i].name);

                result = taf_lib_flash_OpenPartition(&partitionList.partition[i], O_RDWR);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open MTD partition.");

                uint32_t blockSize = 0;
                result = taf_lib_flash_GetMtdEraseSize(&partitionList.partition[i], &blockSize);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD erase size.");
                TAF_ERROR_IF_RET_VAL(blockSize == 0, LE_FAULT, "Invalid para(block size is 0)");

                uint32_t size = 0;
                result = taf_lib_flash_GetMtdSize(&partitionList.partition[i], &size);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD partition size.");

                uint32_t blockNum = size / blockSize;
                bool isBad = false;
                for (uint32_t j = 0; j < blockNum; j++)
                {
                    result = taf_lib_flash_IsMtdBadBlock(&partitionList.partition[i], j, &isBad);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
                        "Fail to get block %d status.", j);

                    if (isBad)
                    {
                        LE_DEBUG("Detect bad block at %d.", j);
                    }
                    else
                    {
                        result = taf_lib_flash_EraseMtdBlock(&partitionList.partition[i], j);
                        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
                            "Fail to erase block %d.", j);
                    }
                }

                result = taf_lib_flash_ClosePartition(&partitionList.partition[i]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close MTD partition.");
            }
            else
            {
                LE_INFO("Erasing UBI %s...", partitionList.partition[i].name);
                result = taf_lib_flash_EraseUbiVol(&partitionList.partition[i]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to erase UBI volume.");
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * PerformBankSync.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::PerformBankSync
(
    void
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
    taf_lib_flash_PartitionList_t partitionList;
    taf_lib_flash_Bank_t srcBank = NOT_DUAL_BANK;

    if (tafFwUpdate.GetActiveBank(&bootBank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }

    switch (bootBank)
    {
        case TAF_UPDATE_BANK_A:
            srcBank = DUAL_BANK_A;
            break;
        case TAF_UPDATE_BANK_B:
            srcBank = DUAL_BANK_B;
            break;
        default:
            LE_ERROR("Invalid bank.");
            return LE_BAD_PARAMETER;
    }

    le_result_t result = taf_lib_flash_GetPartitionList(&partitionList);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can not get partition list.");

    for (uint32_t i = 0; i < partitionList.number; i++)
    {
        if (partitionList.partition[i].bank == srcBank)
        {
            uint32_t j = partitionList.partition[i].mirrorIndex;
            if (partitionList.partition[i].eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
            {
                LE_INFO("Perform sync from MTD %s to %s...",
                    partitionList.partition[i].name, partitionList.partition[j].name);

                result = taf_lib_flash_OpenPartition(&partitionList.partition[i], O_RDWR);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open MTD partition.");

                result = taf_lib_flash_OpenPartition(&partitionList.partition[j], O_RDWR);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open MTD partition.");

                uint32_t blockNum = partitionList.partition[j].size / TAF_LIB_FLASH_MTD_BLOCK_SIZE;
                uint32_t pageNum = partitionList.partition[j].size / TAF_FWUPDATE_FLASH_PAGE_SIZE;
                for (uint32_t k = 0; k < blockNum; k++)
                {
                    result = taf_lib_flash_EraseMtdBlock(&partitionList.partition[j], k);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
                        "Fail to erase block %d.", k);
                }
                for (uint32_t k = 0; k < pageNum; k++)
                {
                    uint8_t data[TAF_FWUPDATE_FLASH_PAGE_SIZE];
                    size_t rdSize = TAF_FWUPDATE_FLASH_PAGE_SIZE;
                    result = taf_lib_flash_ReadPartition(&partitionList.partition[i],
                        k * TAF_FWUPDATE_FLASH_PAGE_SIZE, data, &rdSize);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
                        "Fail to read page %d.", k);

                    result = taf_lib_flash_WritePartition(&partitionList.partition[j],
                        k * TAF_FWUPDATE_FLASH_PAGE_SIZE, data, rdSize);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
                        "Fail to write page %d.", k);
                }

                result = taf_lib_flash_ClosePartition(&partitionList.partition[i]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close MTD partition.");

                result = taf_lib_flash_ClosePartition(&partitionList.partition[j]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close MTD partition.");
            }
            else
            {
                LE_INFO("Perform sync from UBI %s to %s...",
                    partitionList.partition[i].name, partitionList.partition[j].name);

                result = taf_lib_flash_OpenPartition(&partitionList.partition[i], O_RDWR);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open UBI volume.");

                result = taf_lib_flash_OpenPartition(&partitionList.partition[j], O_RDWR);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to open UBI volume.");

                uint32_t pageNum = partitionList.partition[j].size / TAF_FWUPDATE_FLASH_PAGE_SIZE;
                result = taf_lib_flash_SetUbiVolUpSize(&partitionList.partition[j],
                    partitionList.partition[j].size);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to set UBI upgrade size.");

                for (uint32_t k = 0; k < pageNum; k++)
                {
                    uint8_t data[TAF_FWUPDATE_FLASH_PAGE_SIZE];
                    size_t rdSize = TAF_FWUPDATE_FLASH_PAGE_SIZE;
                    result = taf_lib_flash_ReadPartition(&partitionList.partition[i],
                        k * TAF_FWUPDATE_FLASH_PAGE_SIZE, data, &rdSize);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to read page %d.", k);

                    result = taf_lib_flash_WritePartition(&partitionList.partition[j],
                        k * TAF_FWUPDATE_FLASH_PAGE_SIZE, data, rdSize);
                    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to write page %d.", k);
                }

                result = taf_lib_flash_ClosePartition(&partitionList.partition[i]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close MTD partition.");

                result = taf_lib_flash_ClosePartition(&partitionList.partition[j]);
                TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to close MTD partition.");
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rollback.
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::Rollback
(
    void
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bootBank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }

    if (access(TAF_FWUPDATE_PREVIOUS_BANK, F_OK) == 0)
    {
        FILE* fp = fopen(TAF_FWUPDATE_PREVIOUS_BANK, "w");
        fwrite(&bootBank, sizeof(taf_update_Bank_t), 1, fp);
        fflush(fp);
        fclose(fp);
    }
    else
    {
        LE_ERROR("Fail to get the previous bank.");
        return LE_FAULT;
    }

    if (bootBank == TAF_UPDATE_BANK_A)
        bootBank = TAF_UPDATE_BANK_B;
    else if (bootBank == TAF_UPDATE_BANK_B)
        bootBank = TAF_UPDATE_BANK_A;

    if (tafFwUpdate.SetActiveBank(bootBank) != LE_OK)
    {
        LE_ERROR("Fail to set active bank.");
        return LE_FAULT;
    }

    return LE_OK;
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
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Create install timer.
    tafFwUpdate.instTimerRef = le_timer_Create("Firmware Install Timer");
    le_timer_SetMsInterval(tafFwUpdate.instTimerRef, 1000);
    le_timer_SetRepeat(tafFwUpdate.instTimerRef, 0);
    le_timer_SetHandler(tafFwUpdate.instTimerRef, InstallTimerHandler);

    // 2. Add handler for timer options.
    le_event_AddHandler("timerOpHandler", fwTimerEvId, TimerOpHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

le_result_t taf_FwUpdate::InitPartitionList()
{
    le_result_t result = taf_lib_flash_GetPartitionList(&partitionList);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get partition list");
        return result;
    }

    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_lib_flash_Partition_t partition = partitionList.partition[i];
        partitionMap[partition.name] = i;
    }

    isPartitionListInit = true;
    return LE_OK;
}

le_result_t taf_FwUpdate::EraseAllMTDBlocks(taf_update_Bank_t activeBank)
{
    if(!isPartitionListInit)
    {
        LE_ERROR("Partition list is not initialised");
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_lib_flash_Partition_t partition = partitionList.partition[i];

        // check if it is a MTD partition
        if(partition.eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
        {
            // if the active bank is 'A' erase dual banked volumes from 'B'
            // OR if the active bank is 'B' erase dual banked volumes from 'A'
            if(((activeBank == TAF_UPDATE_BANK_A) && (partition.bank == DUAL_BANK_B))
                || ((activeBank == TAF_UPDATE_BANK_B) && (partition.bank == DUAL_BANK_A)))
            {
                // Open Partition
                result = taf_lib_flash_OpenPartition(&partition, O_RDWR);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_OpenPartition for %s failed", partition.name);
                    return result;
                }

                // Get MTD information
                uint32_t totalBlocks = 0, badBlocksNumber = 0,
                    blockSize = 0, pageSize = 0;
                result = GetMtdInformation(&partition,
                    &totalBlocks, &badBlocksNumber, &blockSize, &pageSize);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_flash_MtdInformation for %s failed", partition.name);
                    return result;
                }

                // Erase all blocks
                for (uint32_t block = 0; block < totalBlocks; ++block)
                {
                    result = taf_lib_flash_EraseMtdBlock(&partition, block);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_EraseMtdBlock at blocksNumber %u failed", block);
                        return result;
                    }
                }

                // close partition
                result = taf_lib_flash_ClosePartition(&partition);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_ClosePartition for %s failed", partition.name);
                    return result;
                }
            }
        }
    }

    tafUpdate_ConfigTree_SetBool(kAreBlocksErased, true);
    return LE_OK;
}

le_result_t taf_FwUpdate::GetMtdInformation
(
    taf_lib_flash_Partition_t* partition,
    uint32_t* blocksNumber,
    uint32_t* badBlocksNumber,
    uint32_t* blockSize,
    uint32_t* pageSize
)
{
    TAF_ERROR_IF_RET_VAL(partition == nullptr, LE_BAD_PARAMETER, "Null ptr(partition)");
    TAF_ERROR_IF_RET_VAL(blocksNumber == nullptr, LE_BAD_PARAMETER, "Null ptr(blocksNumber)");
    TAF_ERROR_IF_RET_VAL(badBlocksNumber == nullptr, LE_BAD_PARAMETER,
        "Null ptr(badBlocksNumber)");
    TAF_ERROR_IF_RET_VAL(blockSize == nullptr, LE_BAD_PARAMETER, "Null ptr(blockSize)");
    TAF_ERROR_IF_RET_VAL(pageSize == nullptr, LE_BAD_PARAMETER, "Null ptr(pageSize)");

    /* Get mtd information */
    le_result_t result = taf_lib_flash_GetMtdWriteSize(partition, pageSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Failed to get MTD write size.");

    result = taf_lib_flash_GetMtdEraseSize(partition, blockSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Failed to get MTD erase size.");
    TAF_ERROR_IF_RET_VAL(*blockSize == 0, LE_FAULT, "Invalid block size (0)");

    uint32_t mtdSize;
    result = taf_lib_flash_GetMtdSize(partition, &mtdSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get MTD partition size.");

    *blocksNumber = mtdSize / *blockSize;

    *badBlocksNumber = 0;
    bool isBadBlock = false;
    for (uint32_t i = 0; i < *blocksNumber; ++i)
    {
        result = taf_lib_flash_IsMtdBadBlock(partition, i, &isBadBlock);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to check MTD block at %d.", i);

        if (isBadBlock)
        {
            (*badBlocksNumber)++;
            LE_WARN("Bad block at %d detected.", i);
        }
    }
    return LE_OK;
}

le_result_t taf_FwUpdate::GetUbiInformation
(
    taf_lib_flash_Partition_t* partition,
    uint32_t* lebNumber,
    uint32_t* freeLebNumber,
    uint32_t* volumeSize
)
{
    TAF_ERROR_IF_RET_VAL(partition == nullptr, LE_BAD_PARAMETER, "Null ptr(partition)");
    TAF_ERROR_IF_RET_VAL(lebNumber == nullptr, LE_BAD_PARAMETER, "Null ptr(lebNumber)");
    TAF_ERROR_IF_RET_VAL(freeLebNumber == nullptr, LE_BAD_PARAMETER, "Null ptr(freeLebNumber)");
    TAF_ERROR_IF_RET_VAL(volumeSize == nullptr, LE_BAD_PARAMETER, "Null ptr(volumeSize)");

    /* Get ubi information */
    le_result_t result = taf_lib_flash_GetUbiVolResvLebNum(partition, lebNumber);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI reserved LEB number.");

    result = taf_lib_flash_GetUbiAvailLebNum(partition, freeLebNumber);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI available LEB number.");

    result = taf_lib_flash_GetUbiVolSize(partition, volumeSize);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get UBI volume size.");

    return LE_OK;
}

le_result_t taf_FwUpdate::CalculateTotalPages()
{
    if (!isPartitionListInit)
    {
        LE_WARN("Partition list is not initialised");
        InitPartitionList();
    }

    le_result_t result = LE_OK;
    uint32_t totalPagesForSync = 0;
    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_lib_flash_Partition_t *partition = &(partitionList.partition[i]);

        if(partition->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
        {
            std::string mtdName = std::string(partition->name);
            if(partition->bank == DUAL_BANK_A)
            {
                if(partitionMap.find(mtdName + std::string("_b")) != partitionMap.end())
                {
                    taf_lib_flash_Partition_t *partition_a = partition;

                    // Open Partition 'A'
                    result = taf_lib_flash_OpenPartition(partition_a, O_RDWR);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_OpenPartition for Bank A failed");
                        return LE_FAULT;
                    }

                    // Get MTD information for Partition 'A'
                    uint32_t totalBlocks_a = 0, badBlocksNumber_a = 0,
                        blockSize_a = 0, pageSize_a = 0;
                    result = GetMtdInformation(partition_a, &totalBlocks_a, &badBlocksNumber_a,
                        &blockSize_a, &pageSize_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("GetMtdInformation for partition A failed");
                        return LE_FAULT;
                    }

                    size_t totalPartitionSize = totalBlocks_a * blockSize_a;
                    uint32_t totalPages = totalPartitionSize / pageSize_a;

                    totalPagesForSync += totalPages;

                    // close partition 'A'
                    result = taf_lib_flash_ClosePartition(partition_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ClosePartition for partition A failed");
                        return LE_FAULT;
                    }
                }
            }
        }

        else if(partition->eraseSize == TAF_LIB_FLASH_UBI_BLOCK_SIZE)
        {
            std::string ubiName = std::string(partition->name);
            // check if ubiName ends with "_b"
            if(partition->bank == DUAL_BANK_B)
            {
                std::string ubiName_b(partition->name);

                // generate ubiName ending with '_a'
                std::string suffix("_b");
                std::string::size_type i = ubiName.find(suffix);
                if (ubiName.find(suffix) != std::string::npos)
                {
                    ubiName.erase(i, suffix.length());
                }
                std::string ubiName_a = ubiName + std::string("_a");

                // check if a UBI name with suffix "_a" exists in the map
                if(partitionMap.find(ubiName_a) != partitionMap.end())
                {
                    taf_lib_flash_Partition_t *partition_a =
                        &(partitionList.partition[partitionMap[ubiName_a]]);

                    // Open Partition 'A'
                    result = taf_lib_flash_OpenPartition(partition_a, O_RDWR);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_OpenPartition for Bank A failed");
                        return LE_FAULT;
                    }

                    // UBI Information for volume 'A'
                    uint32_t lebNumber_a = 0, freeLebNumber_a = 0, volumeSize_a = 0;
                    result = GetUbiInformation(partition_a, &lebNumber_a,
                                &freeLebNumber_a, &volumeSize_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("GetUbiInformation for volume A failed");
                        return LE_FAULT;
                    }

                    uint32_t totalPages = volumeSize_a / kPageSize;

                    totalPagesForSync += totalPages;

                    // close volume 'A'
                    result = taf_lib_flash_ClosePartition(partition_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ClosePartition failed");
                        return LE_FAULT;
                    }
                }
            }
        }
    }
    tafUpdate_ConfigTree_SetInt(kTotalPages, totalPagesForSync);
    LE_DEBUG("totalPagesForSync = %u", totalPagesForSync);
    return LE_OK;
}

le_result_t taf_FwUpdate::SyncMTD(taf_update_Bank_t activeBank)
{
    if (!isPartitionListInit)
    {
        LE_ERROR("Partition list is not initialised");
        return LE_FAULT;
    }

    le_result_t result = LE_OK;

    bool areBlocksErased = tafUpdate_ConfigTree_GetBool(kAreBlocksErased);
    if(!areBlocksErased)
    {
        result = EraseAllMTDBlocks(activeBank);
        if(result != LE_OK)
        {
            LE_ERROR("EraseAllMTDBlocks failed");
            return result;
        }
    }

    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_update_State_t state =
            (taf_update_State_t)tafUpdate_ConfigTree_GetInt(kState);
        if(state == TAF_UPDATE_SYNC_PAUSED)
        {
            LE_INFO("Received pause signal in MTD sync, stopping...");
            return LE_OK;
        }

        taf_lib_flash_Partition_t *partition = &(partitionList.partition[i]);

        // Not a MTD partition, skip
        if(partition->eraseSize != TAF_LIB_FLASH_MTD_BLOCK_SIZE)
        {
            continue;
        }

        if(partition->bank == DUAL_BANK_A)
        {
            std::string mtdName = std::string(partition->name);
            if(partitionMap.find(mtdName + std::string("_b")) != partitionMap.end())
            {
                bool isCopied = tafUpdate_ConfigTree_GetBool(mtdName);
                if(isCopied)
                {
                    LE_INFO("MTD %s is already synced, skipping it", mtdName.c_str());
                    continue;
                }
                else
                {
                    LE_INFO("Syncing MTD %s", mtdName.c_str());
                }

                taf_lib_flash_Partition_t *partition_a = partition;
                taf_lib_flash_Partition_t *partition_b =
                    &(partitionList.partition[partitionMap[std::string(partition->name) + "_b"]]);

                // Open Partition 'A'
                result = taf_lib_flash_OpenPartition(partition_a, O_RDWR);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_OpenPartition for partition A failed");
                    return LE_FAULT;
                }

                // Open Partition 'B'
                result = taf_lib_flash_OpenPartition(partition_b, O_RDWR);
                if (result != LE_OK)
                {
                    // close partition 'A'
                    result = taf_lib_flash_ClosePartition(partition_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ClosePartition for partition A failed");
                        return LE_FAULT;
                    }

                    LE_ERROR("taf_lib_flash_OpenPartition for partition B failed");
                    return LE_FAULT;
                }

                // Get MTD information for Partition 'A'
                uint32_t totalBlocks_a = 0, badBlocksNumber_a = 0,
                    blockSize_a = 0, pageSize_a = 0;
                result = GetMtdInformation(partition_a, &totalBlocks_a, &badBlocksNumber_a,
                    &blockSize_a, &pageSize_a);
                if (result != LE_OK)
                {
                    LE_ERROR("GetMtdInformation for partition A failed");
                    return LE_FAULT;
                }
                if(badBlocksNumber_a > 0)
                {
                    LE_ERROR("bad blocks(%u) detected in partition A, stopping sync..",
                        badBlocksNumber_a);
                    return LE_FAULT;
                }

                // Get MTD information for Partition 'B'
                uint32_t totalBlocks_b = 0, badBlocksNumber_b = 0,
                    blockSize_b = 0, pageSize_b = 0;
                result = GetMtdInformation(partition_b, &totalBlocks_b, &badBlocksNumber_b,
                    &blockSize_b, &pageSize_b);
                if (result != LE_OK)
                {
                    LE_ERROR("GetMtdInformation for partition B failed");
                    return LE_FAULT;
                }
                if(badBlocksNumber_b > 0)
                {
                    LE_ERROR("bad blocks(%u) detected in partition B, stopping sync..",
                        badBlocksNumber_b);
                    return LE_FAULT;
                }

                size_t totalPartitionSize = 0;
                uint32_t totalPages = 0;
                taf_lib_flash_Partition_t *src = nullptr, *dest = nullptr;
                if(activeBank == TAF_UPDATE_BANK_A)
                {
                    totalPartitionSize = totalBlocks_a * blockSize_a;
                    totalPages = totalPartitionSize / pageSize_a;
                    src = partition_a;
                    dest = partition_b;
                }
                else if(activeBank == TAF_UPDATE_BANK_B)
                {
                    totalPartitionSize = totalBlocks_b * blockSize_b;
                    totalPages = totalPartitionSize / pageSize_b;
                    src = partition_b;
                    dest = partition_a;
                }

                size_t pSize = kPageSize;
                uint8_t page[kPageSize] = { 0 };

                uint32_t totalPagesForSync = tafUpdate_ConfigTree_GetInt(kTotalPages);

                // report sync progress to the user
                uint32_t pagesSynced = tafUpdate_ConfigTree_GetInt(kPagesSynced);
                uint32_t percentage = (pagesSynced * 100) / totalPagesForSync;
                ReportStatus(TAF_UPDATE_SYNCHRONIZING, percentage, TAF_UPDATE_NONE);

                for(uint32_t p = 0; p < totalPages; ++p)
                {
                    result = taf_lib_flash_ReadPartition(src,
                                p * TAF_FLASH_MTD_PAGE_MAX_READ_SIZE, page, &pSize);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ReadPartition at page index %u failed", p);
                        return LE_FAULT;
                    }

                    result = taf_lib_flash_WritePartition(dest,
                                p * TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE, page, pSize);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_WritePartition at page index %u failed", p);
                        return LE_FAULT;
                    }
                }
                tafUpdate_ConfigTree_SetInt(kPagesSynced, pagesSynced + totalPages);

                // close partition 'A' and 'B'
                result = taf_lib_flash_ClosePartition(partition_a);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_ClosePartition for partition A failed");
                    return LE_FAULT;
                }
                result = taf_lib_flash_ClosePartition(partition_b);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_ClosePartition for partition B failed");
                    return LE_FAULT;
                }

                // mark sync done for mtdName
                tafUpdate_ConfigTree_SetBool(mtdName, true);

                LE_INFO("Successfully synced MTD %s", mtdName.c_str());
            }
            else
            {
                LE_DEBUG("Skipping sync for %s, not dual banked", mtdName.c_str());
            }
        }
    }

    tafUpdate_ConfigTree_SetBool(kIsMTDSynced, true);
    return LE_OK;
}

le_result_t taf_FwUpdate::SyncUBI(taf_update_Bank_t activeBank)
{
    if (!isPartitionListInit)
    {
        LE_ERROR("Partition list is not initialised");
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_update_State_t state =
            (taf_update_State_t)tafUpdate_ConfigTree_GetInt(kState);
        if(state == TAF_UPDATE_SYNC_PAUSED)
        {
            LE_INFO("Received pause signal in UBI sync, stopping...");
            return LE_OK;
        }

        taf_lib_flash_Partition_t *partition = &(partitionList.partition[i]);

        // Not a UBI partition, skip
        if(partition->eraseSize != TAF_LIB_FLASH_UBI_BLOCK_SIZE)
        {
            continue;
        }

        std::string ubiName = std::string(partition->name);

        // check if ubiName ends with "_b"
        if(partition->bank == DUAL_BANK_B)
        {
            std::string suffix("_b");
            std::string::size_type i = ubiName.find(suffix);
            if (ubiName.find(suffix) != std::string::npos)
            {
                ubiName.erase(i, suffix.length());
            }

            bool isCopied = tafUpdate_ConfigTree_GetBool(partition->name);
            if(isCopied)
            {
                LE_INFO("UBI %s is already synced, skipping it", ubiName.c_str());
                continue;
            }

            std::string ubiName_a = ubiName + std::string("_a");
            std::string ubiName_b(partition->name);

            // check if a UBI name with suffix "_a" exists in the map
            if(partitionMap.find(ubiName_a) != partitionMap.end())
            {
                taf_lib_flash_Partition_t *partition_a =
                    &(partitionList.partition[partitionMap[ubiName_a]]);
                taf_lib_flash_Partition_t *partition_b = partition;

                LE_INFO("Syncing UBI %s", ubiName.c_str());

                // Open Partition 'A'
                result = taf_lib_flash_OpenPartition(partition_a, O_RDWR);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_OpenPartition for Bank A failed");
                    return LE_FAULT;
                }

                // Open Partition 'B'
                result = taf_lib_flash_OpenPartition(partition_b, O_RDWR);
                if (result != LE_OK)
                {
                    // close volume 'A'
                    result = taf_lib_flash_ClosePartition(partition_a);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ClosePartition failed");
                        return LE_FAULT;
                    }
                    LE_ERROR("taf_lib_flash_OpenPartition for Bank B failed");
                    return LE_FAULT;
                }

                // UBI Information for volume 'A'
                uint32_t lebNumber_a = 0, freeLebNumber_a = 0, volumeSize_a = 0;
                result = GetUbiInformation(partition_a, &lebNumber_a,
                            &freeLebNumber_a, &volumeSize_a);
                if (result != LE_OK)
                {
                    LE_ERROR("GetUbiInformation for volume A failed");
                    return LE_FAULT;
                }

                // UBI Information for volume 'B'
                uint32_t lebNumber_b = 0, freeLebNumber_b = 0, volumeSize_b = 0;
                result = GetUbiInformation(partition_b, &lebNumber_b,
                            &freeLebNumber_b, &volumeSize_b);
                if (result != LE_OK)
                {
                    LE_ERROR("GetUbiInformation for volume B failed");
                    return LE_FAULT;
                }

                size_t pSize = kPageSize;
                uint8_t page[kPageSize] = { 0 };
                uint32_t volumeSize = 0;
                taf_lib_flash_Partition_t *src = nullptr, *dest = nullptr;
                if(activeBank == TAF_UPDATE_BANK_A)
                {
                    src = partition_a;
                    dest = partition_b;
                    volumeSize = volumeSize_b;
                }
                else if(activeBank == TAF_UPDATE_BANK_B)
                {
                    src = partition_b;
                    dest = partition_a;
                    volumeSize = volumeSize_a;
                }

                result = taf_lib_flash_SetUbiVolUpSize(dest, volumeSize);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_SetUbiVolUpSize failed");
                    return LE_FAULT;
                }

                uint32_t totalPages = volumeSize / pSize;
                LE_DEBUG("volumeSize = %u", volumeSize);
                LE_DEBUG("totalPages = %u", totalPages);

                uint32_t totalPagesForSync = tafUpdate_ConfigTree_GetInt(kTotalPages);

                // report sync progress to the user
                uint32_t pagesSynced = tafUpdate_ConfigTree_GetInt(kPagesSynced);
                uint32_t percentage = (pagesSynced * 100) / totalPagesForSync;
                ReportStatus(TAF_UPDATE_SYNCHRONIZING, percentage, TAF_UPDATE_NONE);

                // Read pages from source and write to destination
                for (uint32_t p = 0; p < totalPages; ++p)
                {
                    result = taf_lib_flash_ReadPartition(src, p * pSize, page, &pSize);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_ReadPartition failed at index %u", p);
                        return LE_FAULT;
                    }

                    result = taf_lib_flash_WritePartition(dest, 0, page, pSize);
                    if (result != LE_OK)
                    {
                        LE_ERROR("taf_lib_flash_WritePartition failed at index %u", p);
                        return LE_FAULT;
                    }
                }
                tafUpdate_ConfigTree_SetInt(kPagesSynced, pagesSynced + totalPages);

                // close volume src and dest
                result = taf_lib_flash_ClosePartition(src);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_ClosePartition failed");
                    return LE_FAULT;
                }

                result = taf_lib_flash_ClosePartition(dest);
                if (result != LE_OK)
                {
                    LE_ERROR("taf_lib_flash_ClosePartition failed");
                    return LE_FAULT;
                }

                // mark sync done for current UBI
                tafUpdate_ConfigTree_SetBool(partition->name, true);

                //tafUpdate_ConfigTree_SetBool(kIsUBIVolUpSizeSet, false);
                LE_INFO("Successfully synced UBI %s", partition->name);
            }
        }
    }
    tafUpdate_ConfigTree_SetBool(kIsUBISynced, true);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform A-B bank sync
 *
 * @return
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::PerformABSync()
{
    le_result_t result = LE_OK;

    /* get active slot */
    taf_update_Bank_t activeBank = TAF_UPDATE_BANK_UNKNOWN;
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    result = tafFwUpdate.GetActiveBank(&activeBank);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
        tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNC_FAIL);
        return;
    }
    switch (activeBank)
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

    if(!isPartitionListInit)
    {
        InitPartitionList();
    }

    uint32_t totalPagesForSync = tafUpdate_ConfigTree_GetInt(kTotalPages);
    if(totalPagesForSync == 0)
    {
        CalculateTotalPages();
    }

    bool isMTDSynced = tafUpdate_ConfigTree_GetBool(kIsMTDSynced);
    taf_update_State_t state =
        (taf_update_State_t)tafUpdate_ConfigTree_GetInt(kState);
    if((!isMTDSynced) && (state != TAF_UPDATE_SYNC_PAUSED))
    {
        result = SyncMTD(activeBank);
        if(result != LE_OK)
        {
            LE_ERROR("SyncMTD failed");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
            tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNC_FAIL);
            return;
        }
    }

    bool isUBISynced = tafUpdate_ConfigTree_GetBool(kIsUBISynced);
    state = (taf_update_State_t)tafUpdate_ConfigTree_GetInt(kState);
    if((!isUBISynced) && (state != TAF_UPDATE_SYNC_PAUSED))
    {
        result = SyncUBI(activeBank);
        if(result != LE_OK)
        {
            LE_ERROR("SyncUBI failed");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
            tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNC_FAIL);
            return;
        }
    }

    isMTDSynced = tafUpdate_ConfigTree_GetBool(kIsMTDSynced);
    isUBISynced = tafUpdate_ConfigTree_GetBool(kIsUBISynced);
    if(isMTDSynced && isUBISynced)
    {
        LE_INFO("Sync completed successfully");
        tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNC_SUCCESS);
        ReportStatus(TAF_UPDATE_SYNCHRONIZING, 100, TAF_UPDATE_NONE);
        ReportStatus(TAF_UPDATE_SYNC_SUCCESS, 0, TAF_UPDATE_NONE);
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwStartSync
 DESCRIPTION     AB start sync handler
 PARAMETERS      [IN] reqPtr: firmware update request
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::FwStartSync(void* reqPtr)
{
    LE_DEBUG("In taf_FwUpdate::FwStartSync");
    taf_FwUpdateEvent_t evt = *((taf_FwUpdateEvent_t*)reqPtr);
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if((evt == TAF_FWUPDATE_EV_START_SYNC) || (evt == TAF_FWUPDATE_EV_RESUME_SYNC))
    {
        tafFwUpdate.PerformABSync();
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwSyncHandler
 DESCRIPTION     AB sync handler
 PARAMETERS      [IN] reqPtr: firmware update request
 RETURN VALUE    void
======================================================================*/
void taf_FwUpdate::FwSyncHandler(void* reqPtr)
{
    LE_DEBUG("In taf_FwUpdate::FwSyncHandler");
    taf_FwUpdateReq_t* updateReq = (taf_FwUpdateReq_t*)reqPtr;
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    taf_update_State_t state =
        (taf_update_State_t)tafUpdate_ConfigTree_GetInt(kState);

    if (updateReq->event == TAF_FWUPDATE_EV_START_SYNC)
    {
        if((state == TAF_UPDATE_SYNCHRONIZING) || (state == TAF_UPDATE_SYNC_PAUSED))
        {
            tafFwUpdate.ReportStatus(state, 0, TAF_UPDATE_INVALID_OPERATION);
            std::string currState = tafFwUpdate.FwUpdateStateToString(state);
            LE_WARN("Invalid state to start AB sync, current state is %s", currState.c_str());
            return;
        }

        // clear the config tree
        tafUpdate_ConfigTree_ClearTree();

        tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_IDLE);
        tafUpdate_ConfigTree_SetInt(kPagesSynced, 0);
        tafUpdate_ConfigTree_SetInt(kTotalPages, 0);
        tafUpdate_ConfigTree_SetBool(kAreBlocksErased, false);
        tafUpdate_ConfigTree_SetBool(kAreBlocksErased, false);
        tafUpdate_ConfigTree_SetBool(kIsMTDSynced, false);
        tafUpdate_ConfigTree_SetBool(kIsUBISynced, false);

        LE_INFO("Starting A-B bank synchronization");
        tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);

        tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNCHRONIZING);

        // report current state to the user
        tafFwUpdate.ReportStatus(TAF_UPDATE_SYNCHRONIZING, 0, TAF_UPDATE_NONE);

        taf_FwUpdateEvent_t req = TAF_FWUPDATE_EV_START_SYNC;
        le_event_Report(taf_FwUpdate::fwStartSyncEvId, &req, sizeof(taf_FwUpdateEvent_t));
    }
    else if (updateReq->event == TAF_FWUPDATE_EV_PAUSE_SYNC)
    {
        if(state == TAF_UPDATE_SYNCHRONIZING)
        {
            LE_INFO("Pausing A-B bank synchronization");
            tafFwUpdate.SetState(TAF_UPDATE_SYNC_PAUSED);
            tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNC_PAUSED);

            // report current state to the user
            tafFwUpdate.ReportStatus(TAF_UPDATE_SYNC_PAUSED, 0, TAF_UPDATE_NONE);
        }
        else
        {
            tafFwUpdate.ReportStatus(state, 0, TAF_UPDATE_INVALID_OPERATION);
            std::string currState = tafFwUpdate.FwUpdateStateToString(state);
            LE_WARN("Invalid state for pause, current state is %s", currState.c_str());
        }
    }
    else if (updateReq->event == TAF_FWUPDATE_EV_RESUME_SYNC)
    {
        if(state == TAF_UPDATE_SYNC_PAUSED)
        {
            LE_INFO("Resuming A-B bank synchronization");
            tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);
            tafUpdate_ConfigTree_SetInt(kState, TAF_UPDATE_SYNCHRONIZING);
            taf_FwUpdateEvent_t req = TAF_FWUPDATE_EV_START_SYNC;
            le_event_Report(taf_FwUpdate::fwStartSyncEvId, &req, sizeof(taf_FwUpdateEvent_t));
        }
        else
        {
            tafFwUpdate.ReportStatus(state, 0, TAF_UPDATE_INVALID_OPERATION);
            std::string currState = tafFwUpdate.FwUpdateStateToString(state);
            LE_WARN("Invalid state for resume, current state is %s", currState.c_str());
        }
    }
    else
    {
        LE_WARN("Unknown event %d", (int)updateReq->event);
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwUpdateStateToString
 DESCRIPTION     Returns FW update state in string format
 PARAMETERS      [IN] state: FW update state
 RETURN VALUE    std::string
======================================================================*/
std::string taf_FwUpdate::FwUpdateStateToString(taf_update_State_t state)
{
    switch (state)
    {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            return "TAF_UPDATE_DOWNLOAD_FAIL";
        case TAF_UPDATE_DOWNLOADING:
            return "TAF_UPDATE_DOWNLOADING";
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            return "TAF_UPDATE_DOWNLOAD_SUCCESS";
        case TAF_UPDATE_INSTALLING:
            return "TAF_UPDATE_INSTALLING";
        case TAF_UPDATE_INSTALL_FAIL:
            return "TAF_UPDATE_INSTALL_FAIL";
        case TAF_UPDATE_INSTALL_SUCCESS:
            return "TAF_UPDATE_INSTALL_SUCCESS";
        case TAF_UPDATE_PROBATION:
            return "TAF_UPDATE_PROBATION";
        case TAF_UPDATE_IDLE:
            return "TAF_UPDATE_IDLE";
        case TAF_UPDATE_SYNCHRONIZING:
            return "TAF_UPDATE_SYNCHRONIZING";
        case TAF_UPDATE_SYNC_SUCCESS:
            return "TAF_UPDATE_SYNC_SUCCESS";
        case TAF_UPDATE_SYNC_PAUSED:
            return "TAF_UPDATE_SYNC_PAUSED";
        case TAF_UPDATE_SYNC_FAIL:
            return "TAF_UPDATE_SYNC_FAIL";
        default:
            return "";
    }
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwSyncHandlerThread
 DESCRIPTION     Thread for handling AB Sync operation
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_FwUpdate::FwSyncHandlerThread(void* contextPtr)
{
    le_cfg_ConnectService();

    le_event_AddHandler("fwSyncHandler", fwSyncHandlerEvId, FwSyncHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

/*======================================================================
 FUNCTION        taf_FwUpdate::FwSyncHandlerThread
 DESCRIPTION     Thread for handling AB Sync operation
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_FwUpdate::FwStartSyncThread(void* contextPtr)
{
    le_cfg_ConnectService();

    le_event_AddHandler("fwStartSync", fwStartSyncEvId, FwStartSync);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Firmware unpdate handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::FwUpdateHandler
(
    void* reqPtr
)
{
    taf_FwUpdateReq_t* updateReq = (taf_FwUpdateReq_t*)reqPtr;
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    taf_update_State_t state = tafFwUpdate.GetState();

    switch (state)
    {
        case TAF_UPDATE_IDLE:
            if (updateReq->event == TAF_FWUPDATE_EV_INSTALL)
            {
                LE_INFO("NAD update start.");
                tafFwUpdate.error = TAF_UPDATE_NONE;
                tafFwUpdate.InstallFirmware(updateReq->filePath);
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_SYNC)
            {
                LE_INFO("NAD bank sync start.");
                tafFwUpdate.error = TAF_UPDATE_NONE;
                if (tafFwUpdate.PerformBankSync() != LE_OK)
                {
                    LE_ERROR("Fail to perform bank sync.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
                }
                else
                {
                    LE_INFO("Perform bank sync successfully.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            if (updateReq->event == TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE)
            {
                if (tafFwUpdate.IsBankSwitched())
                {
                    LE_WARN("Bank is swicthed.");
                }
                else
                {
                    LE_INFO("Reboot to active bank.");

                    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
                    tafFwUpdate.GetActiveBank(&bank);
                    if (bank == TAF_UPDATE_BANK_A && !tafFwUpdate.IsBankSwitched())
                        tafFwUpdate.SetActiveBank(TAF_UPDATE_BANK_B);
                    else if (bank == TAF_UPDATE_BANK_B)
                        tafFwUpdate.SetActiveBank(TAF_UPDATE_BANK_A);
                }

                if (reboot(RB_AUTOBOOT) == -1)
                {
                    LE_FATAL("Fail to reboot. Errno = %s.", LE_ERRNO_TXT(errno));
                }
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_VERIFY_ACTIVATION)
            {
                LE_INFO("Activation verification.");
                if (tafFwUpdate.VerifyActivation(updateReq->filePath) != LE_OK)
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                else
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);

                taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
                tafFwUpdate.GetActiveBank(&bootBank);

                if (access(TAF_FWUPDATE_PREVIOUS_BANK, F_OK) == 0)
                {
                    FILE* fp = fopen(TAF_FWUPDATE_PREVIOUS_BANK, "w");
                    fwrite(&bootBank, sizeof(taf_update_Bank_t), 1, fp);
                    fflush(fp);
                    fclose(fp);
                }
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_INSTALL_POST_CHECK)
            {
                LE_INFO("Installation post check.");
                if (tafFwUpdate.InstallPostCheck(updateReq->filePath) != LE_OK)
                {
                    tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                }
                else
                {
                    tafFwUpdate.error = TAF_UPDATE_NONE;
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);
                }
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_ROLLBACK)
            {
                if (!tafFwUpdate.IsBankSwitched())
                {
                    LE_ERROR("Bank is not swicthed.");
                }
                else
                {
                    LE_INFO("Start rollback before activation.");
                    if (tafFwUpdate.Rollback() != LE_OK)
                        tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL);
                    else
                        tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_SUCCESS);
                }
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for install success state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            if (updateReq->event == TAF_FWUPDATE_EV_ROLLBACK)
            {
                LE_INFO("Start rollback after activation.");
                if (tafFwUpdate.Rollback() != LE_OK)
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL);
                else
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_SUCCESS);
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for probation fail state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_ERROR("Invalid operation for synchronizing state.");
            break;
        case TAF_UPDATE_INSTALLING:
            LE_ERROR("Invalid operation for installing state.");
            break;
        default:
            LE_ERROR("Invalid state (%d).", state);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Firmware unpdate thread.
 */
//--------------------------------------------------------------------------------------------------
void* taf_FwUpdate::FwUpdateThread
(
    void* contextPtr ///< [IN] Context
)
{
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
    fwSyncHandlerEvId = le_event_CreateId("fwSyncHandlerEvId", sizeof(taf_FwUpdateEvent_t));
    fwStartSyncEvId = le_event_CreateId("fwStartSyncEvId", sizeof(taf_FwUpdateEvent_t));

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

    // 4. Create thread for sync handler.
    tafUpdate_ConfigTree_ClearTree();
    semaphore = le_sem_Create("FwSyncHandlerThreadSem", 0);
    threadRef = le_thread_Create("FwSyncHandlerThread", FwSyncHandlerThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 5. Create thread for start sync event.
    semaphore = le_sem_Create("FwStartSyncThreadSem", 0);
    threadRef = le_thread_Create("FwStartSyncThread", FwStartSyncThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 6. Initiate from current state.
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
        default:
            LE_ERROR("FOTA invalid state(%d), reset to idle.", state);
            SetState(TAF_UPDATE_IDLE);
    }

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
