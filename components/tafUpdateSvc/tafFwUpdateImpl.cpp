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

#include <cstring>
#include <chrono>
#include <fstream>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"
#include "tafUpdateConfigTreeHelper.hpp"

using namespace std;
using namespace telux::tafsvc;

le_event_Id_t taf_FwUpdate::fwUpdateEvId = nullptr;
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
 * Set pause action in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetPauseAction
(
    bool paused ///< [IN] Pause state.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    le_cfg_SetBool(wrIter, "paused", paused);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get pause action from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetPauseAction
(
    void
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    bool paused = le_cfg_GetBool(rdIter, "paused", false);
    le_cfg_CancelTxn(rdIter);
    return paused;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set page number in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetPageNumber
(
    bool isTotal,   ///< [IN] True if it is total page number.
    uint32_t number ///< [IN] Page number.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    if (isTotal)
    {
        le_cfg_SetInt(wrIter, "total_page", (int)number);
    }
    else
    {
        le_cfg_SetInt(wrIter, "updated_page", (int)number);
    }
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get page number from config tree.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_FwUpdate::GetPageNumber
(
    bool isTotal ///< [IN] True if it is total page number.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    uint32_t pageNum = 0;
    if (isTotal)
    {
        pageNum = (uint32_t)le_cfg_GetInt(rdIter, "total_page", 0);
    }
    else
    {
        pageNum = (uint32_t)le_cfg_GetInt(rdIter, "updated_page", 0);
    }
    le_cfg_CancelTxn(rdIter);
    return pageNum;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set image data path in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetImageDataPath
(
    const char* image,   ///< [IN] Image.
    const char* dataPath ///< [IN] Image data path.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Set image data path.
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
    le_cfg_SetString(wrIter, "dataPath", dataPath);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get image data path from config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetImageDataPath
(
    const char* image, ///< [IN] Image.
    char* dataPath,    ///< [OUT] Image data path.
    size_t pathLen     ///< [IN] Path length.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Get image data path.
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
    le_cfg_GetString(rdIter, "dataPath", dataPath, pathLen, "");
    le_cfg_CancelTxn(rdIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get image data path from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetImageStatus
(
    const char* image ///< [IN] Image.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Get image state.
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
    bool updated = le_cfg_GetBool(rdIter, "updated", false);
    le_cfg_CancelTxn(rdIter);
    return updated;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set image status in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetImageStatus
(
    const char* image, ///< [IN] Image.
    bool updated       ///< [IN] True if the image is updated.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Set image state.
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
    le_cfg_SetBool(wrIter, "updated", updated);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get image page number from config tree.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_FwUpdate::GetImagePageNumber
(
    const char* image ///< [IN] Image.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Get image page number.
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
    uint32_t pageNum = (uint32_t)le_cfg_GetInt(rdIter, "page_num", 0);
    le_cfg_CancelTxn(rdIter);
    return pageNum;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set image page number in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetImagePageNumber
(
    const char* image, ///< [IN] Image.
    uint32_t pageNum   ///< [IN] Image page number.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);

    // 2. Set image page number.
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
    le_cfg_SetInt(wrIter, "page_num", (int)pageNum);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get image to be updated from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetImageForUpdate
(
    char* name,    ///< [OUT] Image name.
    size_t nameLen ///< [IN] Image name length.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    le_cfg_GoToNode(rdIter, "image");

    if (le_cfg_GoToFirstChild(rdIter) == LE_NOT_FOUND)
    {
        LE_WARN("No image to be updated.");
        le_cfg_CancelTxn(rdIter);
        return false;
    }

    bool updated = false;
    do
    {
        le_cfg_GetNodeName(rdIter, "", name, nameLen);

        updated = le_cfg_GetBool(rdIter, "updated", false);
        if (!updated)
            break;
    }
    while (le_cfg_GoToNextSibling(rdIter) == LE_OK);

    le_cfg_CancelTxn(rdIter);

    return updated;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set activation paused in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetActivationPaused
(
    bool paused ///< [IN] Pause state.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_SetBool(wrIter, "paused", paused);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get activation pause status from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetActivationPaused
(
    void
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    bool paused = le_cfg_GetBool(rdIter, "paused", false);
    le_cfg_CancelTxn(rdIter);
    return paused;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get activation status from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetActivateItemStatus
(
    const char* item ///< [IN] Activation item.
)
{
    // 1. Find node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_ACTIVATE_ITEM_NODE, item);

    // 2. Get item status.
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
    bool activated = le_cfg_GetBool(rdIter, "activated", false);
    le_cfg_CancelTxn(rdIter);
    return activated;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set activation status in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetActivateItemStatus
(
    const char* item, ///< [IN] Activation item.
    bool activated    ///< [IN] True if item is activated.
)
{
    // 1. Find node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_FWUPDATE_ACTIVATE_ITEM_NODE, item);

    // 2. Set item status.
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
    le_cfg_SetBool(wrIter, "activated", activated);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set manifest in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetManifest
(
    const char* manifest ///< [IN] Path for manifest.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_SetString(wrIter, "manifest", manifest);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get manifest from config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetManifest
(
    char* manifest, ///< [OUT] Manifest path.
    size_t pathLen  ///< [IN] Path length.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_GetString(rdIter, "manifest", manifest, pathLen, "");
    le_cfg_CancelTxn(rdIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set activation context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetActivationContext
(
    taf_update_State_t state, ///< [IN] State.
    taf_update_Bank_t bank   ///< [IN] Bank.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_SetInt(wrIter, "state", (int)state);
    if (state == TAF_UPDATE_INSTALL_SUCCESS || state == TAF_UPDATE_ROLLBACK_SUCCESS)
    {
        le_cfg_SetInt(wrIter, "previous_bank", (int)bank);
    }

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    if (state == TAF_UPDATE_INSTALL_SUCCESS)
    {
        char rootfsVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
        char telafVer[TAF_TELAF_VERSION_LEN] = {0};
        char firmwareVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};

        tafFwUpdate.GetTelafVersion(telafVer);
        le_cfg_SetString(wrIter, "telaf_version", telafVer);
        tafFwUpdate.GetRootfsVersion(rootfsVer);
        le_cfg_SetString(wrIter, "rootfs_version", rootfsVer);
        tafFwUpdate.GetFirmwareVersion(firmwareVer);
        le_cfg_SetString(wrIter, "firmware_version", firmwareVer);
    }
    le_cfg_CommitTxn(wrIter);

    if (state == TAF_UPDATE_INSTALL_SUCCESS || state == TAF_UPDATE_ROLLBACK_SUCCESS)
    {
        tafFwUpdate.SetActivateItemStatus("bank_switch", false);
        tafFwUpdate.SetActivateItemStatus("telaf", false);
        tafFwUpdate.SetActivateItemStatus("rootfs", false);
        tafFwUpdate.SetActivateItemStatus("firmware", false);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get previous version.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetPreviousVersion
(
    const char* item, ///< [OUT] Activation item.
    char* version,    ///< [OUT] Previous version.
    size_t verSize    ///< [IN] Version size;
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    if (strncmp(item, "telaf", strlen(item)) == 0)
    {
        le_cfg_GetString(rdIter, "telaf_version", version, verSize, "");
    }
    else if (strncmp(item, "rootfs", strlen(item)) == 0)
    {
        le_cfg_GetString(rdIter, "rootfs_version", version, verSize, "");
    }
    else if (strncmp(item, "firmware", strlen(item)) == 0)
    {
        le_cfg_GetString(rdIter, "firmware_version", version, verSize, "");
    }

    le_cfg_CancelTxn(rdIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get previous bank from activation context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetPreviousBank
(
    taf_update_Bank_t* bank ///< [OUT] Bank.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    *bank = (taf_update_Bank_t)le_cfg_GetInt(rdIter, "previous_bank", 0);
    le_cfg_CancelTxn(rdIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get activation state from activation context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetActivationState
(
    taf_update_State_t* state ///< [OUT] Activation state.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    *state = (taf_update_State_t)le_cfg_GetInt(rdIter, "state", 0);
    le_cfg_CancelTxn(rdIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get item to be activated from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetItemForActivation
(
    char* item,     ///< [OUT] Activation item.
    size_t itemLen, ///< [IN] Activation item name length.
    uint32_t* index ///< [IN] Activation item node index.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_GoToNode(rdIter, "item");

    if (le_cfg_GoToFirstChild(rdIter) == LE_NOT_FOUND)
    {
        LE_WARN("No item to be activated.");
        le_cfg_CancelTxn(rdIter);
        return false;
    }

    bool activated = false;
    uint32_t i = 0;
    do
    {
        i++;
        le_cfg_GetNodeName(rdIter, "", item, itemLen);

        activated = le_cfg_GetBool(rdIter, "activated", false);
        if (!activated)
            break;
    }
    while (le_cfg_GoToNextSibling(rdIter) == LE_OK);

    le_cfg_CancelTxn(rdIter);
    *index = i;

    return activated;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get activation item count.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_FwUpdate::GetActivationItemCount
(
    void
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_ACTIVATE_CONTEXT);
    le_cfg_GoToNode(rdIter, "item");

    if (le_cfg_GoToFirstChild(rdIter) == LE_NOT_FOUND)
    {
        LE_WARN("No item to be activated.");
        le_cfg_CancelTxn(rdIter);
        return 0;
    }

    uint32_t i = 0;
    do
    {
        i++;
    }
    while (le_cfg_GoToNextSibling(rdIter) == LE_OK);

    le_cfg_CancelTxn(rdIter);

    return i;
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

    taf_update_Bank_t currBank = TAF_UPDATE_BANK_UNKNOWN;
    taf_update_Bank_t prevBank = TAF_UPDATE_BANK_UNKNOWN;

    tafFwUpdate.GetActiveBank(&currBank);
    tafFwUpdate.GetPreviousBank(&prevBank);

    if (currBank != prevBank)
        return true;

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

    // 1. Update state.
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%%...", tafFwUpdate.percent);
            break;
        case TAF_UPDATE_INSTALL_PAUSED:
            LE_INFO("Installing %d%% paused...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_INSTALL_PAUSED);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_INFO("Install failed.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            tafFwUpdate.error = TAF_UPDATE_IMAGE_NOT_VERFIED;
            break;
        case TAF_UPDATE_PROBATION_PAUSED:
            LE_INFO("Probation %d%% paused...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_PROBATION_PAUSED);
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Probation success.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_INFO("Probation failed.");
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_INFO("A-B bank sync is in progress, completed %d%%...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);
            break;
        case TAF_UPDATE_SYNC_SUCCESS:
            LE_INFO("A-B bank sync success.");
            tafFwUpdate.percent = 0;
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
        case TAF_UPDATE_SYNC_PAUSED:
            LE_INFO("A-B bank sync paused at %d%%...", tafFwUpdate.percent);
            tafFwUpdate.SetState(TAF_UPDATE_SYNC_PAUSED);
            break;
        case TAF_UPDATE_SYNC_FAIL:
            LE_INFO("A-B bank sync failed.");
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
 * Check if the patch file exist.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::IsPatchExist
(
    const char* filePath, ///< [IN] File path.
    const char* patchPath ///< [IN] Patch path.
)
{
    char tmp[TAF_FWUPDATE_CMD_LEN];
    snprintf(tmp, sizeof(tmp), "unzip -jo %s %s -d /data/", filePath, patchPath);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.SendPipeCmd(tmp, "w");
    tafFwUpdate.SendPipeCmd("sync", "w");

    snprintf(tmp, sizeof(tmp), "/data/%s", patchPath);

    struct stat st;
    if (stat(tmp, &st) == -1)
    {
        LE_WARN("%s not exists.", tmp);
        return false;
    }

    unlink(tmp);

    if (st.st_size == 0)
    {
        LE_INFO("%s is empty.", tmp);
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if it is delta update.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::IsDeltaUpdate
(
    const char* filePath ///< [IN] File path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (tafFwUpdate.IsPatchExist(filePath, "telaf.patch.dat"))
    {
        LE_INFO("Delta update with telaf.");
        return true;
    }

    if (tafFwUpdate.IsPatchExist(filePath, "system.patch.dat"))
    {
        LE_INFO("Delta update with rootfs.");
        return true;
    }

    if (tafFwUpdate.IsPatchExist(filePath, "modem.patch.dat"))
    {
        LE_INFO("Delta update with firmware.");
        return true;
    }

    if (tafFwUpdate.IsPatchExist(filePath, "lxcrootfs.patch.dat"))
    {
        LE_INFO("Delta update with lxc.");
        return true;
    }

    if (tafFwUpdate.IsPatchExist(filePath, "patch/boot.img.p"))
    {
        LE_INFO("Delta update with boot.");
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unpack image.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::UnpackImage
(
    const char* filePath,  ///< [IN] File path.
    const char* imagePath, ///< [IN] Image path.
    uint32_t* pageNum      ///< [OUT] Page number.
)
{
    char tmp[TAF_FWUPDATE_CMD_LEN];
    snprintf(tmp, sizeof(tmp), "unzip -jo %s %s -d /data/", filePath, imagePath);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.SendPipeCmd(tmp, "w");
    tafFwUpdate.SendPipeCmd("sync", "w");

    snprintf(tmp, sizeof(tmp), "/data/%s", imagePath);

    struct stat st;
    if (stat(tmp, &st) == -1)
    {
        LE_WARN("%s not exists.", tmp);
        return false;
    }
    else if (st.st_size == 0)
    {
        LE_INFO("%s is empty.", tmp);
        return false;
    }

    if (st.st_size % TAF_FWUPDATE_FLASH_PAGE_SIZE)
    {
        *pageNum = (uint32_t)st.st_size / TAF_FWUPDATE_FLASH_PAGE_SIZE + 1;
    }
    else
    {
        *pageNum = (uint32_t)st.st_size / TAF_FWUPDATE_FLASH_PAGE_SIZE;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update image with install context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::UpdateImage
(
    void
)
{
    uint32_t pages = 0;
    char image[TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN];
    char dataPath[TAF_UPDATE_FILE_PATH_LEN];
    uint8_t buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Get partition layout.
    taf_lib_flash_PartitionList_t list;
    le_result_t result = taf_lib_flash_GetPartitionList(&list);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get partition list");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 2. Get the next image for update.
    bool hasImageToUpdate = !tafFwUpdate.GetImageForUpdate(image, sizeof(image));
    while (hasImageToUpdate)
    {
        if (!tafFwUpdate.GetPauseAction())
        {
            // 3. Find the partition for flash access.
            uint32_t i =0;
            for (i = 0; i < list.number; i++)
            {
                if ((strlen(image) == strlen(list.partition[i].name)) &&
                    (strncmp(image, list.partition[i].name, strlen(image)) == 0))
                    break;
            }

            if (i == list.number)
            {
                LE_ERROR("Fail to find partition %s from layout.", image);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                return;
            }

            // 4. Open image data file for read.
            pages = tafFwUpdate.GetImagePageNumber(image);

            // 5. Get image information from config tree.
            GetImageDataPath(image, dataPath, sizeof(dataPath));
            FILE *fp = fopen(dataPath, "r");
            if (fp == NULL)
            {
                printf("File %s not exists.", dataPath);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                return;
            }

            // 6. Open partition for read and write.
            result = taf_lib_flash_OpenPartition(&list.partition[i], O_RDWR);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to open partition %s.", list.partition[i].name);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                fclose(fp);
                return;
            }

            // 7. Erase MTD partition or UBI volume.
            if (list.partition[i].eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
            {
                uint32_t blockNum = list.partition[i].size / list.partition[i].eraseSize;
                LE_INFO("Erasing %d blocks in MTD partition %s.", blockNum,
                    list.partition[i].name);
                for (uint32_t k = 0; k < blockNum; k++)
                {
                    bool isBad = false;
                    result = taf_lib_flash_IsMtdBadBlock(&list.partition[i], k, &isBad);
                    if (result != LE_OK)
                    {
                        LE_ERROR("Fail to get block %d status.", k);
                        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                        fclose(fp);
                        return;
                    }

                    if (isBad)
                    {
                        LE_WARN("Bad block detected at %d", k);
                    }
                    else
                    {
                        result = taf_lib_flash_EraseMtdBlock(&list.partition[i], k);
                        if (result != LE_OK)
                        {
                            LE_ERROR("Fail to erase block %d.", i);
                            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                            fclose(fp);
                            return;
                        }
                    }
                }
                LE_INFO("MTD partition %s is erased.", list.partition[i].name);
            }
            else
            {
                LE_INFO("Erasing UBI volume %s.", list.partition[i].name);
                result = taf_lib_flash_EraseUbiVol(&list.partition[i]);
                if (result != LE_OK)
                {
                    LE_ERROR("Fail to erase volume %s.", list.partition[i].name);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }

                LE_INFO("UBI volume %s is erased.", list.partition[i].name);

                result = taf_lib_flash_SetUbiVolUpSize(&list.partition[i],
                    pages * TAF_FWUPDATE_FLASH_PAGE_SIZE);
                if (result != LE_OK)
                {
                    LE_ERROR("Fail to set volume %s upgrade size.", list.partition[i].name);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }
            }

            // 8. Perform flash write.
            uint32_t pageUpdated = tafFwUpdate.GetPageNumber(false);
            uint32_t totalPages = tafFwUpdate.GetPageNumber(true);
            uint32_t percent = tafFwUpdate.percent;
            for (uint32_t j = 0; j < pages; j++)
            {
                int ret = fread(buffer, 1, TAF_FWUPDATE_FLASH_PAGE_SIZE, fp);
                if (ret < TAF_FWUPDATE_FLASH_PAGE_SIZE)
                {
                    memset(buffer + ret, 0xFF, TAF_FWUPDATE_FLASH_PAGE_SIZE - ret);
                    LE_INFO("Padding 0xFF in %s at page %d, start at %d.\n",
                        list.partition[i].name, j, ret);
                }

                result = taf_lib_flash_WritePartition(&list.partition[i],
                    j * TAF_FWUPDATE_FLASH_PAGE_SIZE, buffer, TAF_FWUPDATE_FLASH_PAGE_SIZE);
                if (result != LE_OK)
                {
                    LE_ERROR("Can not to write %s at page %d.", list.partition[i].name, j);
                }

                percent = (pageUpdated + j) * 100 / totalPages;
                if (percent != tafFwUpdate.percent)
                {
                    tafFwUpdate.percent = percent;
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALLING);
                }
            }

            // 9. Close partition for read and write.
            result = taf_lib_flash_ClosePartition(&list.partition[i]);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to close partition %s.", list.partition[i].name);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                fclose(fp);
                return;
            }

            fclose(fp);

            LE_INFO("%s is updated.", list.partition[i].name);

            unlink(dataPath);
            tafFwUpdate.SetImageStatus(image, true);
            tafFwUpdate.SetPageNumber(false, pageUpdated + pages);

            hasImageToUpdate = !tafFwUpdate.GetImageForUpdate(image, sizeof(image));
        }
        else
        {
            LE_INFO("Paused during update.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_PAUSED);
            return;
        }
    }

    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    result = tafFwUpdate.GetActiveBank(&bank);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }
    else
    {
        if (bank == TAF_UPDATE_BANK_A)
        {
            result = tafFwUpdate.SetActiveBank(TAF_UPDATE_BANK_B);
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            result = tafFwUpdate.SetActiveBank(TAF_UPDATE_BANK_A);
        }

        if (bank == TAF_UPDATE_BANK_UNKNOWN || result != LE_OK)
        {
            LE_ERROR("Fail to set active bank.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        tafFwUpdate.SetActivationContext(TAF_UPDATE_INSTALL_SUCCESS, bank);
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start installation.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::StartInstall
(
    const char* filePath ///< [IN] File path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    tafFwUpdate.SetPauseAction(false);
    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
    }

    uint32_t totalPage = 0;
    uint32_t imagePage = 0;
    if (tafFwUpdate.UnpackImage(filePath, "telaf.new.dat", &imagePage))
    {
        LE_INFO("Detect telaf to be updated.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            tafFwUpdate.SetImageStatus("telaf_b", false);
            tafFwUpdate.SetImagePageNumber("telaf_b", imagePage);
            tafFwUpdate.SetImageDataPath("telaf_b", "/data/telaf.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            tafFwUpdate.SetImageStatus("telaf_a", false);
            tafFwUpdate.SetImagePageNumber("telaf_a", imagePage);
            tafFwUpdate.SetImageDataPath("telaf_a", "/data/telaf.new.dat");
        }

        totalPage += imagePage;
    }

    if (tafFwUpdate.UnpackImage(filePath, "system.new.dat", &imagePage))
    {
        LE_INFO("Detect rootfs to be updated.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            tafFwUpdate.SetImageStatus("rootfs_b", false);
            tafFwUpdate.SetImagePageNumber("rootfs_b", imagePage);
            tafFwUpdate.SetImageDataPath("rootfs_b", "/data/system.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            tafFwUpdate.SetImageStatus("rootfs_a", false);
            tafFwUpdate.SetImagePageNumber("rootfs_a", imagePage);
            tafFwUpdate.SetImageDataPath("rootfs_a", "/data/system.new.dat");
        }

        totalPage += imagePage;
    }

    if (tafFwUpdate.UnpackImage(filePath, "modem.new.dat", &imagePage))
    {
        LE_INFO("Detect firmware to be updated.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            tafFwUpdate.SetImageStatus("firmware_b", false);
            tafFwUpdate.SetImagePageNumber("firmware_b", imagePage);
            tafFwUpdate.SetImageDataPath("firmware_b", "/data/modem.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            tafFwUpdate.SetImageStatus("firmware_a", false);
            tafFwUpdate.SetImagePageNumber("firmware_a", imagePage);
            tafFwUpdate.SetImageDataPath("firmware_a", "/data/modem.new.dat");
        }

        totalPage += imagePage;
    }

    if (tafFwUpdate.UnpackImage(filePath, "lxcrootfs.new.dat", &imagePage))
    {
        LE_INFO("Detect lxcrootfs to be updated.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            tafFwUpdate.SetImageStatus("lxcrootfs_b", false);
            tafFwUpdate.SetImagePageNumber("lxcrootfs_b", imagePage);
            tafFwUpdate.SetImageDataPath("lxcrootfs_b", "/data/lxcrootfs.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            tafFwUpdate.SetImageStatus("lxcrootfs_a", false);
            tafFwUpdate.SetImagePageNumber("lxcrootfs_a", imagePage);
            tafFwUpdate.SetImageDataPath("lxcrootfs_a", "/data/lxcrootfs.new.dat");
        }

        totalPage += imagePage;
    }

    if (tafFwUpdate.UnpackImage(filePath, "boot.img", &imagePage))
    {
        LE_INFO("Detect boot to be updated.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            tafFwUpdate.SetImageStatus("boot_b", false);
            tafFwUpdate.SetImagePageNumber("boot_b", imagePage);
            tafFwUpdate.SetImageDataPath("boot_b", "/data/boot.img");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            tafFwUpdate.SetImageStatus("boot", false);
            tafFwUpdate.SetImagePageNumber("boot", imagePage);
            tafFwUpdate.SetImageDataPath("boot", "/data/boot.img");
        }

        totalPage += imagePage;
    }

    tafFwUpdate.SetPageNumber(true, totalPage);
    tafFwUpdate.SetPageNumber(false, 0);

    tafFwUpdate.UpdateImage();
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

    // 3. Check if it is delta update.
    if (!tafFwUpdate.IsDeltaUpdate(filePath))
    {
        tafFwUpdate.StartInstall(filePath);
        return;
    }

    // 5. Install pacackeg with recovery client.
    LE_INFO("recovery client installing.");
    char instCmd[TAF_FWUPDATE_CMD_LEN];
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
 * Calculate Hash of a file
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::CalFileHash
(
    const char* filePath, ///< [IN] File path of the image data.
    uint32_t* calSize,    ///< [OUT] Size of file for calculation.
    uint8_t* hash,        ///< [OUT] Hash of a file.
    unsigned int* hashLen ///< [OUT] Hash length.
)
{
    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        LE_ERROR("Fail to open %s.", filePath);
        return LE_FAULT;
    }

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1();
    if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
    {
        LE_ERROR("Fail to initiate sha1 context.");
        EVP_MD_CTX_free(md_ctx);
        return LE_FAULT;
    }

    uint8_t content[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    int bytes = 0;
    while ((bytes = fread(content, 1, TAF_FWUPDATE_FLASH_PAGE_SIZE, file)) != 0)
    {
        EVP_DigestUpdate(md_ctx, content, bytes);
        *calSize += bytes;
    }

    EVP_DigestFinal_ex(md_ctx, hash, hashLen);
    EVP_MD_CTX_free(md_ctx);

    fclose(file);
    LE_INFO("%s sha1 hash calculated.", filePath);
    for (unsigned int i = 0; i < *hashLen; ++i)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculate Hash of a partition
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::CalPartitionHash
(
    const char* partition, ///< [IN] Partition name.
    uint32_t calSize,      ///< [IN] Partition size for calculation.
    uint8_t* hash,         ///< [OUT] Hash of a partition.
    unsigned int* hashLen  ///< [OUT] Hash length.
)
{
    taf_lib_flash_PartitionList_t list;
    le_result_t result = taf_lib_flash_GetPartitionList(&list);
    if (result != LE_OK)
    {
        LE_ERROR("Get partition list failed.");
        return LE_FAULT;
    }

    uint32_t i = 0;
    for (i = 0; i < list.number; i++)
    {
        if (strncmp(partition, list.partition[i].name, strlen(partition)) == 0 &&
            strlen(partition) == strlen(list.partition[i].name))
            break;
    }

    if (i == list.number)
    {
        LE_ERROR("Partition %s not found.", partition);
        return LE_FAULT;
    }

    result = taf_lib_flash_OpenPartition(&list.partition[i], O_RDONLY);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open partition %s.", partition);
        return LE_FAULT;
    }

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1();
    if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
    {
        LE_ERROR("Fail to initiate sha1 context.");
        EVP_MD_CTX_free(md_ctx);
        return LE_FAULT;
    }

    uint8_t content[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    size_t bytes = 0;
    uint32_t iteration = calSize / TAF_FWUPDATE_FLASH_PAGE_SIZE;
    for (uint32_t j = 0; j < iteration; j++)
    {
        bytes = TAF_FWUPDATE_FLASH_PAGE_SIZE;
        result = taf_lib_flash_ReadPartition(&list.partition[i], j * TAF_FWUPDATE_FLASH_PAGE_SIZE,
            content, &bytes);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read partition %s at iteration %d.", partition, j);
            EVP_MD_CTX_free(md_ctx);
            return LE_FAULT;
        }

        EVP_DigestUpdate(md_ctx, content, bytes);
    }

    bytes = calSize % TAF_FWUPDATE_FLASH_PAGE_SIZE;
    if (bytes != 0)
    {
        result = taf_lib_flash_ReadPartition(&list.partition[i],
            iteration * TAF_FWUPDATE_FLASH_PAGE_SIZE, content, &bytes);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read the reset of partition %s.", partition);
            EVP_MD_CTX_free(md_ctx);
            return LE_FAULT;
        }

        EVP_DigestUpdate(md_ctx, content, bytes);
    }

    EVP_DigestFinal_ex(md_ctx, hash, hashLen);
    EVP_MD_CTX_free(md_ctx);

    LE_INFO("%s sha1 hash calculated.", partition);
    for (unsigned int i = 0; i < *hashLen; ++i)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");

     // 7. Close partition.
    result = taf_lib_flash_ClosePartition(&list.partition[i]);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close partition.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify hash.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::VerifyHash
(
    const char* partition, ///< [IN] Partition name.
    const char* filePath   ///< [IN] File path of the image data.
)
{
    uint8_t fileHash[SHA_DIGEST_LENGTH];
    uint8_t partHash[SHA_DIGEST_LENGTH];
    unsigned int fileHashLen = sizeof(fileHash);
    unsigned int partHashLen = sizeof(partHash);
    uint32_t calSize = 0;

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    le_result_t result = tafFwUpdate.CalFileHash(filePath, &calSize, fileHash, &fileHashLen);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to calculate %s hash.", filePath);
        return false;
    }

    result = tafFwUpdate.CalPartitionHash(partition, calSize, partHash, &partHashLen);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to calculate %s hash.", partition);
        return false;
    }

    if (partHashLen != fileHashLen)
    {
        LE_ERROR("Hash output length not equal.");
        return false;
    }

    for (unsigned int i = 0; i < partHashLen; i++)
    {
        if (fileHash[i] != partHash[i])
        {
            LE_ERROR("Hash verified with failure at %d.", i);
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Firmware installation post-check.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::InstallPostCheck
(
    const char* filePath ///< [IN] File path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    uint32_t imagePage = 0;
    bool verified = false;
    if (tafFwUpdate.UnpackImage(filePath, "telaf.new.dat", &imagePage))
    {
        LE_INFO("Install post-check on telaf.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            verified = tafFwUpdate.VerifyHash("telaf_b", "/data/telaf.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            verified = tafFwUpdate.VerifyHash("telaf_a", "/data/telaf.new.dat");
        }

        if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
        {
            LE_ERROR("Post-check failure on telaf.");
            tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        LE_INFO("Install post-check on telaf success.");
    }

    if (tafFwUpdate.UnpackImage(filePath, "system.new.dat", &imagePage))
    {
        LE_INFO("Install post-check on rootfs.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            verified = tafFwUpdate.VerifyHash("rootfs_b", "/data/system.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            verified = tafFwUpdate.VerifyHash("rootfs_a", "/data/system.new.dat");
        }

        if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
        {
            LE_ERROR("Post-check failure on rootfs.");
            tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        LE_INFO("Install post-check on rootfs success.");
    }

    if (tafFwUpdate.UnpackImage(filePath, "modem.new.dat", &imagePage))
    {
        LE_INFO("Install post-check on firmware.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            verified = tafFwUpdate.VerifyHash("firmware_b", "/data/modem.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            verified = tafFwUpdate.VerifyHash("firmware_a", "/data/modem.new.dat");
        }

        if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
        {
            LE_ERROR("Post-check failure on firmware.");
            tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        LE_INFO("Install post-check on firmware success.");
    }

    if (tafFwUpdate.UnpackImage(filePath, "lxcrootfs.new.dat", &imagePage))
    {
        LE_INFO("Install post-check on lxcrootfs.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            verified = tafFwUpdate.VerifyHash("lxcrootfs_b", "/data/lxcrootfs.new.dat");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            verified = tafFwUpdate.VerifyHash("lxcrootfs_a", "/data/lxcrootfs.new.dat");
        }

        if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
        {
            LE_ERROR("Post-check failure on lxcrootfs.");
            tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        LE_INFO("Install post-check on lxcrootfs success.");
    }

    if (tafFwUpdate.UnpackImage(filePath, "boot.img", &imagePage))
    {
        LE_INFO("Install post-check on boot.");

        if (bank == TAF_UPDATE_BANK_A)
        {
            verified = tafFwUpdate.VerifyHash("boot_b", "/data/boot.img");
        }
        else if (bank == TAF_UPDATE_BANK_B)
        {
            verified = tafFwUpdate.VerifyHash("boot", "/data/boot.img");
        }

        if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
        {
            LE_ERROR("Post-check failure on boot.");
            tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }

        LE_INFO("Install post-check on boot success.");
    }

    tafFwUpdate.error = TAF_UPDATE_NONE;
    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);
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
        fp = popen("/usr/bin/nad-abctl --set_active 0", "r");
    }
    else if (bank == TAF_UPDATE_BANK_B)
    {
        fp = popen("/usr/bin/nad-abctl --set_active 1", "r");
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
    if (!tafFwUpdate.IsBankSwitched())
    {
        LE_ERROR("Bank is not swicthed.");
        return LE_FAULT;
    }

    taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bootBank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }

    taf_update_Bank_t activeBank = TAF_UPDATE_BANK_UNKNOWN;
    if (bootBank == TAF_UPDATE_BANK_A)
        activeBank = TAF_UPDATE_BANK_B;
    else if (bootBank == TAF_UPDATE_BANK_B)
        activeBank = TAF_UPDATE_BANK_A;

    if (tafFwUpdate.SetActiveBank(activeBank) != LE_OK)
    {
        LE_ERROR("Fail to set active bank.");
        return LE_FAULT;
    }

    tafFwUpdate.SetActivationContext(TAF_UPDATE_ROLLBACK_SUCCESS, bootBank);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate with context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::ActivateComponent
(
    void
)
{
    char item[LE_CFG_STR_LEN_BYTES] = { 0 };
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    uint32_t index = 0;
    char manifest[TAF_UPDATE_FILE_PATH_LEN] = { 0 };
    bool withMainifest = false;
    char rootfsCmpVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
    char telafCmpVer[TAF_TELAF_VERSION_LEN] = {0};
    char firmwareCmpVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};

    // 1. Check if activation has been performed.
    taf_update_State_t state = TAF_UPDATE_PROBATION;
    tafFwUpdate.GetActivationState(&state);
    switch (state)
    {
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("Activation items have been verified with success.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);
            return;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_ERROR("Activation items have been verified with failure.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            return;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Activation after installion.");
            withMainifest = true;
            break;
        case TAF_UPDATE_ROLLBACK_SUCCESS:
            LE_INFO("Activation after rollback.");
            break;
        default:
            LE_ERROR("Invalid activation operation.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            return;
    }

    // 2. Parse manifest file to get versions.
    if (withMainifest)
    {
        tafFwUpdate.GetManifest(manifest, sizeof(manifest));
        // Bypass activation.
        if (strncmp(manifest, TAF_FWUPDATE_BYPASS_CHECK_TAG,
            strlen(TAF_FWUPDATE_BYPASS_CHECK_TAG)) == 0)
        {
            LE_INFO("Bypass activation verification.");
            tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_SUCCESS,
                TAF_UPDATE_BANK_UNKNOWN);
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);
            return;
        }

        std::ifstream manifestFin(manifest);
        std::string manifestVer;

        // Get rootfs version from manifest file.
        getline(manifestFin, manifestVer);
        size_t pos = manifestVer.find(":");
        if (pos == string::npos)
        {
            LE_ERROR("Invalid character for rootfs version in manifest.");
            tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                TAF_UPDATE_BANK_UNKNOWN);
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            return;
        }
        manifestVer = manifestVer.substr(pos + 1);
        le_utf8_Copy(rootfsCmpVer, manifestVer.c_str(), TAF_FWUPDATE_MAX_VERS_LEN, NULL);

        // Get firmware version from manifest file.
        getline(manifestFin, manifestVer);
        pos = manifestVer.find(":");
        if (pos == string::npos)
        {
            LE_ERROR("Invalid character for firmware version in manifest.");
            tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                TAF_UPDATE_BANK_UNKNOWN);
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            return;
        }
        manifestVer = manifestVer.substr(pos + 1);
        le_utf8_Copy(firmwareCmpVer, manifestVer.c_str(), TAF_FWUPDATE_MAX_VERS_LEN, NULL);

        // Get telaf version from manifest file.
        getline(manifestFin, manifestVer);
        pos = manifestVer.find(":");
        if (pos == string::npos)
        {
            LE_ERROR("Invalid character for telaf version in manifest.");
            tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                TAF_UPDATE_BANK_UNKNOWN);
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
            return;
        }
        manifestVer = manifestVer.substr(pos + 1);
        le_utf8_Copy(telafCmpVer, manifestVer.c_str(), TAF_TELAF_VERSION_LEN, NULL);

        // Close file stream.
        manifestFin.close();
    }
    else
    {
        // Get previous version from config tree.
        GetPreviousVersion("telaf", telafCmpVer, sizeof(telafCmpVer));
        GetPreviousVersion("rootfs", rootfsCmpVer, sizeof(rootfsCmpVer));
        GetPreviousVersion("firmware", firmwareCmpVer, sizeof(firmwareCmpVer));
    }

    // 3. Activate remain items.
    bool hasItemToActivate = !tafFwUpdate.GetItemForActivation(item, sizeof(item), &index);
    uint32_t total = tafFwUpdate.GetActivationItemCount();
    while (hasItemToActivate)
    {
        if (!tafFwUpdate.GetActivationPaused())
        {
            tafFwUpdate.percent = index * 100 / total;
            if (strncmp(item, "bank_switch", strlen(item)) == 0)
            {
                LE_INFO("Checking if bank is switched...");
                if (tafFwUpdate.IsBankSwitched())
                {
                    LE_INFO("Check bank activation -- PASS.");
                    tafFwUpdate.SetActivateItemStatus("bank_switch", true);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION);
                }
                else
                {
                    LE_ERROR("Check bank activation -- FAIL.");
                    tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                        TAF_UPDATE_BANK_UNKNOWN);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                    return;
                }
            }

            if (strncmp(item, "telaf", strlen(item)) == 0)
            {
                LE_INFO("Checking telaf version...");
                char telafVer[TAF_TELAF_VERSION_LEN] = {0};
                tafFwUpdate.GetTelafVersion(telafVer);
                LE_INFO("Current telaf version : %s", telafVer);
                if (strncmp(telafCmpVer, telafVer, strlen(telafVer)) == 0)
                {
                    LE_INFO("Check telaf activation -- PASS.");
                    tafFwUpdate.SetActivateItemStatus("telaf", true);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION);
                }
                else
                {
                    LE_ERROR("Check telaf activation -- FAIL.");
                    LE_ERROR("The expected telaf version : %s", telafCmpVer);
                    tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                        TAF_UPDATE_BANK_UNKNOWN);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                    return;
                }
            }

            if (strncmp(item, "rootfs", strlen(item)) == 0)
            {
                LE_INFO("Checking rootfs version...");
                char rootfsVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
                tafFwUpdate.GetRootfsVersion(rootfsVer);
                LE_INFO("Current rootfs version : %s", rootfsVer);
                if (strncmp(rootfsCmpVer, rootfsVer, strlen(rootfsVer)) == 0)
                {
                    LE_INFO("Check rootfs activation -- PASS.");
                    tafFwUpdate.SetActivateItemStatus("rootfs", true);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION);
                }
                else
                {
                    LE_ERROR("Check rootfs activation -- FAIL.");
                    LE_ERROR("The expected rootfs version : %s", rootfsCmpVer);
                    tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                        TAF_UPDATE_BANK_UNKNOWN);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                    return;
                }
            }

            if (strncmp(item, "firmware", strlen(item)) == 0)
            {
                LE_INFO("Checking firmware version...");
                char firmwareVer[TAF_FWUPDATE_MAX_VERS_LEN] = {0};
                le_result_t result = tafFwUpdate.GetFirmwareVersion(firmwareVer);
                LE_INFO("Current firmware version : %s", firmwareVer);
                if (result == LE_OK &&
                    strncmp(firmwareCmpVer, firmwareVer, strlen(firmwareVer)) == 0)
                {
                    LE_INFO("Check firmware activation -- PASS.");
                    tafFwUpdate.SetActivateItemStatus("firmware", true);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION);
                }
                else
                {
                    LE_ERROR("Check firmware activation -- FAIL.");
                    LE_ERROR("The expected firmware version : %s", firmwareCmpVer);
                    tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_FAIL,
                        TAF_UPDATE_BANK_UNKNOWN);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_FAIL);
                    return;
                }
            }

            hasItemToActivate = !tafFwUpdate.GetItemForActivation(item, sizeof(item), &index);
        }
        else
        {
            LE_INFO("Paused during activation.");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_PAUSED);
            return;
        }
    }

    LE_INFO("Activation success.");
    tafFwUpdate.SetActivationContext(TAF_UPDATE_PROBATION_SUCCESS, TAF_UPDATE_BANK_UNKNOWN);
    tafFwUpdate.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS);
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

bool taf_FwUpdate::CompareBinaryFiles(const std::string& filename1, const std::string& filename2)
{
    // Open the files for binary read
    std::ifstream file1(filename1, std::ios::binary);
    std::ifstream file2(filename2, std::ios::binary);

    // Check if both files were successfully opened
    if (!file1.is_open() || !file2.is_open())
    {
        LE_ERROR("Error opening files!");
        return false;
    }

    // Compare file sizes
    file1.seekg(0, std::ios::end);
    file2.seekg(0, std::ios::end);
    if (file1.tellg() != file2.tellg())
    {
        return false; // Files are of different sizes
    }
    file1.seekg(0, std::ios::beg);
    file2.seekg(0, std::ios::beg);

    // Read and compare the files at page level
    char buffer1[kPageSize] = {0};
    char buffer2[kPageSize] = {0};

    while (!file1.eof() && !file2.eof())
    {
        file1.read(buffer1, kPageSize);
        file2.read(buffer2, kPageSize);
        if (std::memcmp(buffer1, buffer2, kPageSize) != 0)
        {
            return false; // Files differ
        }
    }

    return true; // Files are identical
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

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_update_State_t state = tafFwUpdate.GetState();
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
                taf_lib_flash_Partition_t *partition_a = partition;
                taf_lib_flash_Partition_t *partition_b =
                    &(partitionList.partition[partitionMap[std::string(partition->name) + "_b"]]);
                bool isEqual = CompareBinaryFiles(partition_a->mtdDevPath, partition_b->mtdDevPath);
                if(isEqual)
                {
                    LE_INFO("MTD %s is already synced, skipping it", mtdName.c_str());
                    continue;
                }
                else
                {
                    LE_INFO("Syncing MTD %s", mtdName.c_str());
                }

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
                tafFwUpdate.percent = (pagesSynced * 100) / totalPagesForSync;
                tafFwUpdate.error = TAF_UPDATE_NONE;
                tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);

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
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    for (uint32_t i = 0; i < partitionList.number; ++i)
    {
        taf_update_State_t state = tafFwUpdate.GetState();
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

            std::string ubiName_a = ubiName + std::string("_a");
            std::string ubiName_b(partition->name);

            // check if a UBI name with suffix "_a" exists in the map
            if(partitionMap.find(ubiName_a) != partitionMap.end())
            {
                taf_lib_flash_Partition_t *partition_a =
                    &(partitionList.partition[partitionMap[ubiName_a]]);
                taf_lib_flash_Partition_t *partition_b = partition;

                bool isEqual = CompareBinaryFiles(partition_a->ubiDevPath, partition_b->ubiDevPath);
                if(isEqual)
                {
                    LE_INFO("UBI %s is already synced, skipping it", ubiName.c_str());
                    continue;
                }

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
                tafFwUpdate.percent = (pagesSynced * 100) / totalPagesForSync;
                tafFwUpdate.error = TAF_UPDATE_NONE;
                tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);

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
    taf_update_State_t state = tafFwUpdate.GetState();
    if((!isMTDSynced) && (state != TAF_UPDATE_SYNC_PAUSED))
    {
        result = SyncMTD(activeBank);
        if(result != LE_OK)
        {
            LE_ERROR("SyncMTD failed");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
            return;
        }
    }

    bool isUBISynced = tafUpdate_ConfigTree_GetBool(kIsUBISynced);
    state = tafFwUpdate.GetState();
    if((!isUBISynced) && (state != TAF_UPDATE_SYNC_PAUSED))
    {
        result = SyncUBI(activeBank);
        if(result != LE_OK)
        {
            LE_ERROR("SyncUBI failed");
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
            return;
        }
    }

    isMTDSynced = tafUpdate_ConfigTree_GetBool(kIsMTDSynced);
    isUBISynced = tafUpdate_ConfigTree_GetBool(kIsUBISynced);
    if(isMTDSynced && isUBISynced)
    {
        LE_INFO("Sync completed successfully");
        tafFwUpdate.percent = 100;
        tafFwUpdate.error = TAF_UPDATE_NONE;
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_SUCCESS);
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
    taf_update_State_t state = tafFwUpdate.GetState();

    if (updateReq->event == TAF_FWUPDATE_EV_START_SYNC)
    {
        if((state == TAF_UPDATE_SYNCHRONIZING) || (state == TAF_UPDATE_SYNC_PAUSED))
        {
            tafFwUpdate.error = TAF_UPDATE_INVALID_OPERATION;
            tafFwUpdate.UpdateProgress(state);
            std::string currState = tafFwUpdate.FwUpdateStateToString(state);
            LE_WARN("Invalid state to start AB sync, current state is %s", currState.c_str());
            return;
        }

        // clear the config tree
        tafUpdate_ConfigTree_ClearTree();

        tafUpdate_ConfigTree_SetInt(kPagesSynced, 0);
        tafUpdate_ConfigTree_SetInt(kTotalPages, 0);
        tafUpdate_ConfigTree_SetBool(kAreBlocksErased, false);
        tafUpdate_ConfigTree_SetBool(kAreBlocksErased, false);
        tafUpdate_ConfigTree_SetBool(kIsMTDSynced, false);
        tafUpdate_ConfigTree_SetBool(kIsUBISynced, false);

        LE_INFO("Starting A-B bank synchronization");

        // report current state to the user
        tafFwUpdate.percent = 0;
        tafFwUpdate.error = TAF_UPDATE_NONE;
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);

        taf_FwUpdateEvent_t req = TAF_FWUPDATE_EV_START_SYNC;
        le_event_Report(taf_FwUpdate::fwStartSyncEvId, &req, sizeof(taf_FwUpdateEvent_t));
    }
    else if (updateReq->event == TAF_FWUPDATE_EV_PAUSE_SYNC)
    {
        if(state == TAF_UPDATE_SYNCHRONIZING)
        {
            LE_INFO("Pausing A-B bank synchronization");
            uint32_t totalPagesForSync = tafUpdate_ConfigTree_GetInt(kTotalPages);
            uint32_t pagesSynced = tafUpdate_ConfigTree_GetInt(kPagesSynced);
            tafFwUpdate.percent = (pagesSynced * 100) / totalPagesForSync;
            tafFwUpdate.error = TAF_UPDATE_NONE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_PAUSED);
        }
        else
        {
            tafFwUpdate.error = TAF_UPDATE_INVALID_OPERATION;
            tafFwUpdate.UpdateProgress(state);
            std::string currState = tafFwUpdate.FwUpdateStateToString(state);
            LE_WARN("Invalid state for pause, current state is %s", currState.c_str());
        }
    }
    else if (updateReq->event == TAF_FWUPDATE_EV_RESUME_SYNC)
    {
        if(state == TAF_UPDATE_SYNC_PAUSED)
        {
            LE_INFO("Resuming A-B bank synchronization");
            uint32_t totalPagesForSync = tafUpdate_ConfigTree_GetInt(kTotalPages);
            uint32_t pagesSynced = tafUpdate_ConfigTree_GetInt(kPagesSynced);
            tafFwUpdate.percent = (pagesSynced * 100) / totalPagesForSync;
            tafFwUpdate.error = TAF_UPDATE_NONE;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);

            taf_FwUpdateEvent_t req = TAF_FWUPDATE_EV_START_SYNC;
            le_event_Report(taf_FwUpdate::fwStartSyncEvId, &req, sizeof(taf_FwUpdateEvent_t));
        }
        else
        {
            tafFwUpdate.error = TAF_UPDATE_INVALID_OPERATION;
            tafFwUpdate.UpdateProgress(state);
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
            if (updateReq->event == TAF_FWUPDATE_EV_START_INSTALL)
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
            else if (updateReq->event == TAF_FWUPDATE_EV_START_ACTIVATION)
            {
                LE_INFO("NAD activation start.");
                tafFwUpdate.error = TAF_UPDATE_NONE;
                tafFwUpdate.SetActivationPaused(false);
                tafFwUpdate.SetState(TAF_UPDATE_PROBATION);
                tafFwUpdate.SetManifest(updateReq->filePath);
                tafFwUpdate.ActivateComponent();
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_INSTALL_POST_CHECK)
            {
                LE_INFO("Installation post check.");
                tafFwUpdate.InstallPostCheck(updateReq->filePath);
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_ROLLBACK)
            {
                LE_INFO("Start to rollback.");
                tafFwUpdate.SetState(TAF_UPDATE_ROLLBACK);
                if (tafFwUpdate.Rollback() != LE_OK)
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL);
                else
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_ROLLBACK_SUCCESS);
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_ROLLBACK:
            LE_ERROR("Invalid operation for rollback state.");
            break;
        case TAF_UPDATE_PROBATION:
            LE_ERROR("Invalid operation for probation state.");
            break;
        case TAF_UPDATE_PROBATION_PAUSED:
            if (updateReq->event == TAF_FWUPDATE_EV_RESUME_ACTIVATION)
            {
                LE_INFO("Resume NAD activation.");
                tafFwUpdate.SetActivationPaused(false);
                tafFwUpdate.SetState(TAF_UPDATE_PROBATION);
                tafFwUpdate.ActivateComponent();
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for probation state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            LE_ERROR("Invalid operation for synchronizing state.");
            break;
        case TAF_UPDATE_INSTALLING:
            LE_ERROR("Invalid operation (%d) for installing state.", updateReq->event);
            break;
        case TAF_UPDATE_INSTALL_PAUSED:
            if (updateReq->event == TAF_FWUPDATE_EV_RESUME_INSTALL)
            {
                LE_INFO("Resume NAD update.");
                tafFwUpdate.SetPauseAction(false);
                tafFwUpdate.SetState(TAF_UPDATE_INSTALLING);
                tafFwUpdate.UpdateImage();
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for installing state.", updateReq->event);
            }
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
    le_cfg_ConnectService();

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
    fwSyncHandlerEvId = le_event_CreateId("fwSyncHandlerEvId", sizeof(taf_FwUpdateEvent_t));
    fwStartSyncEvId = le_event_CreateId("fwStartSyncEvId", sizeof(taf_FwUpdateEvent_t));

    // 2. Create thread for firmware update.
    le_sem_Ref_t semaphore = le_sem_Create("fwUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("fwUpdateThread", FwUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 4. Create thread for sync handler.
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
        case TAF_UPDATE_INSTALL_PAUSED:
            LE_INFO("Install paused.");
            break;
        case TAF_UPDATE_PROBATION_PAUSED:
            LE_INFO("Activation paused.");
            break;
        case TAF_UPDATE_SYNC_PAUSED:
            LE_INFO("Sync paused.");
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            {
                // check if a previous sync was still in progress then complete it first
                LE_INFO("A-B bank synchronization was stopped aburptly, resuming it..");
                taf_FwUpdateEvent_t req = TAF_FWUPDATE_EV_START_SYNC;
                le_event_Report(taf_FwUpdate::fwStartSyncEvId, &req, sizeof(taf_FwUpdateEvent_t));
            }
            break;
        default:
            LE_ERROR("FOTA invalid state(%d), reset to idle.", state);
            SetState(TAF_UPDATE_IDLE);
    }

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
