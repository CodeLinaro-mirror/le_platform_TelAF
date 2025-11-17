/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <cstring>
#include <chrono>
#include <fstream>

#include "jansson.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"
#include "tafUpdateConfigTreeHelper.hpp"

using namespace std;
using namespace tafsvc;

le_event_Id_t taf_FwUpdate::fwUpdateEvId = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * These tables define the partitons that can be included for update.
 */
//--------------------------------------------------------------------------------------------------
static taf_FwUpdateParition_t partitonTableInfo[] =
{
    {"abl", false, "firmware-update/abl_userdebug.elf", NULL},
    {"aop", false, "firmware-update/aop.mbn", NULL},
    {"aop_devcfg", false, "firmware-update/aop_devcfg.mbn", NULL},
    {"apdp", false, "firmware-update/apdp.mbn", NULL},
    {"cmnlib64", false, "firmware-update/cmnlib64.mbn", NULL},
    {"cpucpfw", false, "firmware-update/cpucp.elf", NULL},
    {"tz_devcfg", false, "firmware-update/devcfg_auto.mbn", NULL},
    {"dtbo", false, "firmware-update/dtbo.img", NULL},
    {"qhee", false, "firmware-update/hypvmperformance.mbn", NULL},
    {"keymaster", false, "firmware-update/km5virt.mbn", NULL},
    {"multi_oem", false, "firmware-update/multi_image.mbn", NULL},
    {"multi_qti", false, "firmware-update/multi_image_qti.mbn", NULL},
    {"qupfw", false, "firmware-update/qupv3fw.elf", NULL},
    {"shrm", false, "firmware-update/shrm.elf", NULL},
    {"tz", false, "firmware-update/tz.mbn", NULL},
    {"uefi", false, "firmware-update/uefi.elf", NULL},
    {"xbl_config", false, "firmware-update/xbl_config.elf", NULL},
    {"xbl_ramdump", false, "firmware-update/xbl_ramdump.elf", NULL},
    {"sbl", false, "firmware-update/xbl_s_nand.melf", NULL},
    {"boot", false, "boot.img", "patch/boot.img.p"},
    {"telaf", true, "telaf.new.dat", "telaf.patch.dat"},
    {"rootfs", true, "system.new.dat", "system.patch.dat"},
    {"firmware", true, "modem.new.dat", "modem.patch.dat"},
    {"lxcrootfs", true, "lxcrootfs.new.dat", "lxcrootfs.patch.dat"}
};

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
 * Get unpack directory.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetUnpackDir
(
    char* unpackDir, ///< [OUT] Unpack directory.
    size_t dirLen    ///< [IN] Directory length.
)
{
    json_t *root;
    json_error_t error;

    // Load entire JSON file.
    root = json_load_file(TAF_FWUPDATE_CFG_FILE, 0, &error);
    if (root == NULL)
    {
        LE_ERROR("JSON file error: line: %d, column: %d, position: %d, source: '%s', error: %s",
            error.line, error.column, error.position, error.source, error.text);
        return false;
    }

    // Check if "root" is an object.
    if (!json_is_object(root))
    {
        LE_ERROR("root is not an object.");
        json_decref(root);
        return false;
    }

    // Load the 'firmware' object.
    json_t* js_firmware = json_object_get(root, "firmware");
    if (!json_is_object(js_firmware))
    {
        LE_ERROR("firmware object is not set in JSON file %s.", TAF_FWUPDATE_CFG_FILE);
        return false;
    }

    // Load the 'unpack' object.
    json_t* js_unpack = json_object_get(js_firmware, "unpack");
    if (!json_is_object(js_unpack))
    {
        LE_ERROR("unpack object is not set in JSON file %s.", TAF_FWUPDATE_CFG_FILE);
        return false;
    }

    // Load the 'directory' object.
    json_t* js_directory = json_object_get(js_unpack, "directory");
    if (!json_is_string(js_directory))
    {
        LE_ERROR("directory string is not set in JSON file %s.", TAF_FWUPDATE_CFG_FILE);
        return false;
    }

    le_utf8_Copy(unpackDir, json_string_value(js_directory), dirLen, NULL);
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get post script.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::GetPostScript
(
    taf_update_State_t state, ///< [IN] Update state.
    char* scriptPath,         ///< [OUT] Script path.
    size_t pathLen            ///< [IN] Script path length.
)
{
    json_t *root;
    json_error_t error;

    // Load entire JSON file.
    root = json_load_file(TAF_FWUPDATE_CFG_FILE, 0, &error);
    if (root == NULL)
    {
        LE_ERROR("JSON file error: line: %d, column: %d, position: %d, source: '%s', error: %s",
            error.line, error.column, error.position, error.source, error.text);
        return LE_FAULT;
    }

    // Check if "root" is an object.
    if (!json_is_object(root))
    {
        LE_ERROR("root is not an object.");
        json_decref(root);
        return LE_FAULT;
    }

    // Load the 'firmware' object.
    json_t* js_firmware = json_object_get(root, "firmware");
    if (!json_is_object(js_firmware))
    {
        LE_ERROR("firmware object is not set in JSON file %s.", TAF_FWUPDATE_CFG_FILE);
        return LE_FAULT;
    }

    json_t* js_post;
    if (state == TAF_UPDATE_INSTALL_SUCCESS)
    {
        // Load the 'post-install' object.
        js_post = json_object_get(js_firmware, "post-install");
    }
    else if (state == TAF_UPDATE_ROLLBACK_SUCCESS)
    {
        // Load the 'post-rollback' object.
        js_post = json_object_get(js_firmware, "post-rollback");
    }
    else
    {
        LE_ERROR("Invalid state for hook function.");
        return LE_BAD_PARAMETER;
    }

    if (!json_is_object(js_post))
    {
        LE_ERROR("post-install or post-rollback object is not set in JSON file %s.",
            TAF_FWUPDATE_CFG_FILE);
        return LE_FAULT;
    }

    // Load the 'user-script' object.
    json_t* js_user_script = json_object_get(js_post, "user-script");
    if (!json_is_string(js_user_script))
    {
        LE_ERROR("user-script string is not set in JSON file %s.", TAF_FWUPDATE_CFG_FILE);
        return LE_FAULT;
    }

    le_utf8_Copy(scriptPath, json_string_value(js_user_script), pathLen, NULL);

    return LE_OK;
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
    if (fp == NULL)
    {
        LE_ERROR("Can not open state file %s.", TAF_FWUPDATE_FOTA_STATE);
    }
    else
    {
        fwrite(&state, sizeof(taf_update_State_t), 1, fp);
        fflush(fp);
        fclose(fp);
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
    taf_update_State_t state = TAF_UPDATE_IDLE;
    if (access(TAF_FWUPDATE_FOTA_STATE, F_OK) == 0)
    {
        FILE* fp = fopen(TAF_FWUPDATE_FOTA_STATE, "r");
        if (fp == NULL)
        {
            LE_ERROR("Can not open state file %s.", TAF_FWUPDATE_FOTA_STATE);
        }
        else
        {
            if (fread(&state, sizeof(taf_update_State_t), 1, fp) <= 0)
            {
                LE_WARN("Read %s failed", TAF_FWUPDATE_FOTA_STATE);
            }

            fclose(fp);
        }
    }
    return state;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clean up config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::CleanupContext
(
    taf_update_State_t state ///< [IN] Update state.
)
{
    le_cfg_IteratorRef_t wrIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not clean up context.");
            return;
    }

    le_cfg_DeleteNode(wrIter, "");
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set pause action in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetPauseAction
(
    taf_update_State_t state, ///< [IN] Update state.
    bool paused               ///< [IN] Pause action.
)
{
    le_cfg_IteratorRef_t wrIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not set pause action.");
            return;
    }

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
    taf_update_State_t state ///< [IN] Update state.
)
{
    le_cfg_IteratorRef_t rdIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not get pause action.");
            return false;
    }

    bool paused = le_cfg_GetBool(rdIter, "paused", false);
    le_cfg_CancelTxn(rdIter);
    return paused;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set pause action in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetCancelAction
(
    taf_update_State_t state, ///< [IN] Update state.
    bool cancel               ///< [IN] Cancel action.
)
{
    le_cfg_IteratorRef_t wrIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not set pause action.");
            return;
    }

    le_cfg_SetBool(wrIter, "cancel", cancel);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get cancel action from config tree.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::GetCancelAction
(
    taf_update_State_t state ///< [IN] Update state.
)
{
    le_cfg_IteratorRef_t rdIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not get cancel action.");
            return false;
    }

    bool cancel = le_cfg_GetBool(rdIter, "cancel", false);
    le_cfg_CancelTxn(rdIter);
    return cancel;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set page number in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetPageNumber
(
    taf_update_State_t state, ///< [IN] Update state.
    bool isTotal,             ///< [IN] True if it is total page number.
    uint32_t number           ///< [IN] Page number.
)
{
    le_cfg_IteratorRef_t wrIter;

    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not set page number.");
            return;
    }

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
    taf_update_State_t state, ///< [IN] Update state.
    bool isTotal              ///< [IN] True if it is total page number.
)
{
    le_cfg_IteratorRef_t rdIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not get page number.");
            return 0;
    }

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
 * Set package data path in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetPackageDataPath
(
    const char* dataPath ///< [IN] Package data path.
)
{
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    le_cfg_SetString(wrIter, "packagePath", dataPath);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get package data path from config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::GetPackageDataPath
(
    char* dataPath,    ///< [OUT] Package data path.
    size_t pathLen     ///< [IN] Path length.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
    le_cfg_GetString(rdIter, "packagePath", dataPath, pathLen, "");
    le_cfg_CancelTxn(rdIter);
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
 * Set image status in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetImageStatus
(
    taf_update_State_t state, ///< [IN] Update state.
    const char* image,        ///< [IN] Image.
    bool updated              ///< [IN] True if the image is updated.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    le_cfg_IteratorRef_t wrIter;

    // 2. Set image state.
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_SYNC_IMGAE_NODE, image);
            break;
        default:
            LE_ERROR("Can not get image status.");
            return;
    }

    wrIter = le_cfg_CreateWriteTxn(node);
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
    taf_update_State_t state, ///< [IN] Update state.
    const char* image         ///< [IN] Image.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_SYNC_IMGAE_NODE, image);
            break;
        default:
            LE_ERROR("Can not get page number.");
            return 0;
    }

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
    taf_update_State_t state, ///< [IN] Update state.
    const char* image,        ///< [IN] Image.
    uint32_t pageNum          ///< [IN] Image page number.
)
{
    // 1. Find image node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_INSTALL_IMGAE_NODE, image);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            snprintf(node, sizeof(node), TAF_FWUPDATE_SYNC_IMGAE_NODE, image);
            break;
        default:
            LE_ERROR("Can not set page number.");
            return;
    }

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
    taf_update_State_t state, ///< [IN] Update state.
    char* name,               ///< [OUT] Image name.
    size_t nameLen            ///< [IN] Image name length.
)
{
    le_cfg_IteratorRef_t rdIter;
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_INSTALL_CONTEXT);
            break;
        case TAF_UPDATE_SYNCHRONIZING:
            rdIter = le_cfg_CreateReadTxn(TAF_FWUPDATE_SYNC_CONTEXT);
            break;
        default:
            LE_ERROR("Can not get image for update.");
            return false;
    }

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
 * Intialize partition list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::InitPartitionList
(
    void
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    static bool init = false;

    if (!init)
    {
        taf_pa_flash_Init();
        FILE *fp = fopen("/proc/mtd", "r");
        if (fp == NULL)
        {
            LE_ERROR("Can not open /proc/mtd.");
            return LE_FAULT;
        }

        // 1. Find MTD partitions.
        char line[TAF_FWUPDATE_CMD_LEN] = "";
        while (fgets(line, sizeof(line), fp) != NULL)
        {
            if (strstr(line, "mtd") == NULL)
                continue;

            char *saveptr;
            char *token = strtok_r(line, " ", &saveptr);
            token = strtok_r(NULL, " ", &saveptr);
            token = strtok_r(NULL, " ", &saveptr);
            if (token != NULL)
            {
                uint32_t eraseSize = strtol(token, NULL, 16);
                token = strtok_r(NULL, " ", &saveptr);
                if (token != NULL && eraseSize == TAF_FWUPDATE_FLASH_MTD_EB_SIZE)
                {
                    taf_FlashPartition_t partition;
                    partition.isUbi = false;
                    partition.bank = TAF_UPDATE_BANK_UNKNOWN;
                    token[strlen(token) - 2] = '\0';
                    le_utf8_Copy(partition.name, token + 1, TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);

                    tafFwUpdate.partitions.emplace_back(partition);
                }
            }
        }
        fclose(fp);

        // 2. Find UBI volumes.
        char* pathArrayPtr[] = {(char*)"/sys/devices/virtual/ubi", NULL};
        FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);
        FTSENT* entPtr;
        char* baseName;
        while ((entPtr = fts_read(ftsPtr)) != NULL)
        {
            switch (entPtr->fts_info)
            {
                case FTS_D:
                case FTS_DP:
                case FTS_DEFAULT:
                    break;
                case FTS_F:
                    baseName = le_path_GetBasenamePtr(entPtr->fts_path, "/");
                    if (strncmp(baseName, "name", strlen("name")) == 0)
                    {
                        fp = fopen(entPtr->fts_path, "r");
                        if (fp != NULL && fgets(line, sizeof(line), fp) != NULL)
                        {
                            taf_FlashPartition_t partition;
                            partition.isUbi = true;
                            partition.bank = TAF_UPDATE_BANK_UNKNOWN;
                            le_utf8_Copy(partition.name, line, strlen(line), NULL);

                            tafFwUpdate.partitions.emplace_back(partition);
                        }

                        fclose(fp);
                    }
                    break;
                case FTS_SL:
                case FTS_SLNONE:
                    break;
                case FTS_DC:
                case FTS_DNR:
                case FTS_NS:
                case FTS_ERR:
                default:
                    LE_ERROR("Unexpected file type, %d, on file %s.", entPtr->fts_info,
                        entPtr->fts_path);
                    break;
            }
        }

        fts_close(ftsPtr);

        // 3. Initialize bank for partitions.
        for (uint32_t i = 0; i < tafFwUpdate.partitions.size(); i++)
        {
            if (strlen(tafFwUpdate.partitions[i].name) > TAF_FWUPDATE_PARTITION_SUFFIX_LEN)
            {
                if (strncmp(tafFwUpdate.partitions[i].name +
                    strlen(tafFwUpdate.partitions[i].name) - 2, "_a", 2) == 0)
                {
                    tafFwUpdate.partitions[i].bank = TAF_UPDATE_BANK_A;
                    LE_DEBUG("%s is Bank A.", tafFwUpdate.partitions[i].name);
                }

                if (strncmp(tafFwUpdate.partitions[i].name +
                    strlen(tafFwUpdate.partitions[i].name) - 2, "_b", 2) == 0)
                {
                    tafFwUpdate.partitions[i].bank = TAF_UPDATE_BANK_B;
                    LE_DEBUG("%s is Bank B.", tafFwUpdate.partitions[i].name);

                    if (!tafFwUpdate.partitions[i].isUbi)
                    {
                        for (uint32_t j = 0; j < tafFwUpdate.partitions.size(); j++)
                        {
                            if ((strlen(tafFwUpdate.partitions[i].name) ==
                                strlen(tafFwUpdate.partitions[j].name) + 2) &&
                                (strncmp(tafFwUpdate.partitions[j].name,
                                tafFwUpdate.partitions[i].name,
                                strlen(tafFwUpdate.partitions[j].name)) == 0))
                            {
                                tafFwUpdate.partitions[j].bank = TAF_UPDATE_BANK_A;
                                LE_DEBUG("%s is Bank A.", tafFwUpdate.partitions[i].name);
                            }
                        }
                    }
                }
            }
        }

        init = true;
    }

    return LE_OK;
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
 * Set error code
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SetErrorCode
(
    int errCode ///< [IN] errno
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    if(errCode == 0)
    {
        tafFwUpdate.error = TAF_UPDATE_INTERNAL_ERROR;
    }
    else if(errCode == EIO)
    {
        tafFwUpdate.error = TAF_UPDATE_IO_ERROR;
    }
    else if(errCode == ENOMEM)
    {
        tafFwUpdate.error = TAF_UPDATE_NO_MEM_ERROR;
    }
    else if(errCode == EINVAL)
    {
        tafFwUpdate.error = TAF_UPDATE_INVAL_ARG_ERROR;
    }
    else if(errCode == EBUSY)
    {
        tafFwUpdate.error = TAF_UPDATE_BUSY_ERROR;
    }
    else if(errCode == ENODEV)
    {
        tafFwUpdate.error = TAF_UPDATE_NODEV_ERROR;
    }
    else
    {
        tafFwUpdate.error = TAF_UPDATE_NONE;
    }
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
        case TAF_UPDATE_IDLE:
            tafFwUpdate.SetState(TAF_UPDATE_IDLE);
            break;
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
    if (patchPath == NULL)
    {
        return false;
    }

    char tmp[TAF_FWUPDATE_CMD_LEN];
    char dir[TAF_FWUPDATE_DIRNAME_LEN];

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    if (!tafFwUpdate.GetUnpackDir(dir, TAF_FWUPDATE_DIRNAME_LEN))
    {
        LE_ERROR("Fail to get unpack directory.");
        return false;
    }

    snprintf(tmp, sizeof(tmp), "unzip -o %s %s -d %s", filePath, patchPath, dir);

    tafFwUpdate.SendPipeCmd(tmp, "w");
    tafFwUpdate.SendPipeCmd("sync", "w");

    snprintf(tmp, sizeof(tmp), "%s/%s", dir, patchPath);

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

    size_t i = 0;
    while (i < NUM_ARRAY_MEMBERS(partitonTableInfo))
    {
        if (tafFwUpdate.IsPatchExist(filePath, partitonTableInfo[i].patchPath))
        {
            LE_INFO("Delta update with %s.", partitonTableInfo[i].partition);
            return true;
        }

        i++;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if string has "_a" or "_b" suffix.
 */
//--------------------------------------------------------------------------------------------------
bool taf_FwUpdate::HasSuffix
(
    const char *str ///< [IN] Input string.
)
{
    size_t len = strlen(str);

    if (len > TAF_FWUPDATE_PARTITION_SUFFIX_LEN)
    {
        if (strncmp(str + len - TAF_FWUPDATE_PARTITION_SUFFIX_LEN, "_a",
            TAF_FWUPDATE_PARTITION_SUFFIX_LEN) == 0)
        {
            return true;
        }

        if (strncmp(str + len - TAF_FWUPDATE_PARTITION_SUFFIX_LEN, "_b",
            TAF_FWUPDATE_PARTITION_SUFFIX_LEN) == 0)
        {
            return true;
        }
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
    char dir[TAF_FWUPDATE_DIRNAME_LEN];

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    if (!tafFwUpdate.GetUnpackDir(dir, TAF_FWUPDATE_DIRNAME_LEN))
    {
        LE_ERROR("Fail to get unpack directory.");
        return false;
    }

    struct stat st;
    if (stat(filePath, &st) == -1)
    {
        LE_WARN("%s not exists.", filePath);
        *pageNum = 0;
        return false;
    }

    snprintf(tmp, sizeof(tmp), "unzip -o %s %s -d %s", filePath, imagePath, dir);

    tafFwUpdate.SendPipeCmd(tmp, "w");
    tafFwUpdate.SendPipeCmd("sync", "w");

    snprintf(tmp, sizeof(tmp), "%s/%s", dir, imagePath);

    if (stat(tmp, &st) == -1)
    {
        LE_WARN("%s not exists.", tmp);
        *pageNum = 0;
        return false;
    }
    else if (st.st_size == 0)
    {
        LE_INFO("%s is empty.", tmp);
        *pageNum = 0;
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
    char image[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    char dataPath[TAF_UPDATE_FILE_PATH_LEN];
    uint8_t buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Get partition layout.
    le_result_t result = tafFwUpdate.InitPartitionList();
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get partition list");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    // 2. Get the next image for update.
    bool hasImageToUpdate = !tafFwUpdate.GetImageForUpdate(TAF_UPDATE_INSTALLING, image,
        sizeof(image));
    while (hasImageToUpdate)
    {
        if (!tafFwUpdate.GetPauseAction(TAF_UPDATE_INSTALLING) &&
            !tafFwUpdate.GetCancelAction(TAF_UPDATE_INSTALLING))
        {
            // 3. Find the partition for flash access.
            uint32_t i = 0;
            for (i = 0; i < tafFwUpdate.partitions.size(); i++)
            {
                if ((strlen(image) == strlen(tafFwUpdate.partitions[i].name)) &&
                    (strncmp(image, tafFwUpdate.partitions[i].name, strlen(image)) == 0))
                {
                    break;
                }
            }

            if (i == tafFwUpdate.partitions.size())
            {
                LE_ERROR("Fail to find partition %s from layout.", image);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                return;
            }

            // 4. Get image information from config tree.
            tafFwUpdate.GetImageDataPath(image, dataPath, sizeof(dataPath));
            if (access(dataPath, 0) != 0)
            {
                // 5. Unpack if the temporary file not exists.
                char filePath[TAF_UPDATE_FILE_PATH_LEN];
                tafFwUpdate.GetPackageDataPath(filePath, sizeof(filePath));
                LE_INFO("%s not exits, trying to unpack from %s.", dataPath, filePath);

                char partition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
                le_utf8_Copy(partition, image, TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
                if (tafFwUpdate.HasSuffix(image))
                {
                    partition[strlen(image) - TAF_FWUPDATE_PARTITION_SUFFIX_LEN] = '\0';
                }

                size_t j = 0;
                while (j < NUM_ARRAY_MEMBERS(partitonTableInfo))
                {
                    if (strcmp(partition, partitonTableInfo[j].partition) == 0)
                        break;

                    j++;
                }

                if (j >= NUM_ARRAY_MEMBERS(partitonTableInfo))
                {
                    LE_ERROR("Fail to find %s from partition layout.", partition);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    return;
                }

                char dir[TAF_FWUPDATE_DIRNAME_LEN];
                if (!tafFwUpdate.GetUnpackDir(dir, TAF_FWUPDATE_DIRNAME_LEN))
                {
                    LE_ERROR("Fail to get unpack directory.");
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    return;
                }

                if (!tafFwUpdate.UnpackImage(filePath, partitonTableInfo[j].dataPath, &pages))
                {
                    LE_ERROR("Fail to unnpack %s from %s.",
                        filePath, partitonTableInfo[j].dataPath);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    return;
                }

                snprintf(dataPath, sizeof(dataPath), "%s/%s", dir, partitonTableInfo[j].dataPath);
                tafFwUpdate.SetImagePageNumber(TAF_UPDATE_INSTALLING, image, pages);
                tafFwUpdate.SetImageDataPath(image, dataPath);
                LE_INFO("%s is unpacked from %s.", dataPath, filePath);
            }
            else
            {
                pages = tafFwUpdate.GetImagePageNumber(TAF_UPDATE_INSTALLING, image);
            }

            FILE *fp = fopen(dataPath, "r");
            if (fp == NULL)
            {
                LE_ERROR("File %s not exists.", dataPath);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                return;
            }

            // 6. Update MTD partitions.
            uint32_t pageUpdated = 0;
            if (!tafFwUpdate.partitions[i].isUbi)
            {
                taf_pa_flash_MtdRef_t mtdRef = nullptr;
                int ret = taf_pa_flash_OpenMtd(tafFwUpdate.partitions[i].name, &mtdRef);
                if (ret)
                {
                    LE_ERROR("Fail to open partition %s, ret = %d, error: %s",
                        tafFwUpdate.partitions[i].name, ret, strerror(errno));
                    tafFwUpdate.SetErrorCode(errno);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }

                mtd_info_t info;
                ret = taf_pa_flash_GetMtdInfo(mtdRef, &info);
                if (ret || info.erasesize == 0)
                {
                    LE_ERROR("Fail to get partition %s information, ret = %d, error: %s",
                        tafFwUpdate.partitions[i].name, ret, strerror(errno));
                    tafFwUpdate.SetErrorCode(errno);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    taf_pa_flash_CloseMtd(mtdRef);
                    fclose(fp);
                    return;
                }

                uint32_t totalBlocks = info.size / info.erasesize;
                LE_INFO("Erasing %d blocks in MTD partition %s.", totalBlocks,
                    tafFwUpdate.partitions[i].name);
                for (uint32_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx)
                {
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, blockIdx);
                    if (ret == 0)
                    {
                        LE_WARN("Skip erasing %d bad block", blockIdx);
                    }
                    else if (ret == 1)
                    {
                        if (taf_pa_flash_EraseBlock(mtdRef, blockIdx))
                        {
                            LE_WARN("Fail to erase block %d.", blockIdx);
                            if (taf_pa_flash_MarkBadBlock(mtdRef, blockIdx))
                            {
                                LE_ERROR("Fail to mark %d block as bad.", blockIdx);
                            }
                        }
                    }
                }
                LE_INFO("MTD partition %s is erased.", tafFwUpdate.partitions[i].name);

                pageUpdated = tafFwUpdate.GetPageNumber(TAF_UPDATE_INSTALLING, false);
                uint32_t totalPages = tafFwUpdate.GetPageNumber(TAF_UPDATE_INSTALLING, true);
                uint32_t percent = tafFwUpdate.percent;
                uint32_t pagesPerBlock =
                    TAF_FWUPDATE_FLASH_MTD_EB_SIZE / TAF_FWUPDATE_FLASH_PAGE_SIZE;
                uint32_t remainPages = pages;
                for (uint32_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx)
                {
                    // Skip the bad blocks.
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, blockIdx);
                    if (ret != 1)
                    {
                        LE_WARN("Skip bad blcok at %d.", blockIdx);
                        continue;
                    }

                    for (uint32_t pageIdx = 0; pageIdx < pagesPerBlock; ++pageIdx)
                    {
                        ret = fread(buffer, 1, TAF_FWUPDATE_FLASH_PAGE_SIZE, fp);
                        if (ret < TAF_FWUPDATE_FLASH_PAGE_SIZE)
                        {
                            memset(buffer + ret, 0xFF, TAF_FWUPDATE_FLASH_PAGE_SIZE - ret);
                            LE_INFO("Padding 0xFF in %s at page %d, start at %d.\n",
                                tafFwUpdate.partitions[i].name,
                                pageIdx + blockIdx * pagesPerBlock, ret);
                        }

                        ret = taf_pa_flash_WriteMtdPage(
                                mtdRef,
                                buffer,
                                pageIdx + blockIdx * pagesPerBlock,
                                TAF_FWUPDATE_FLASH_PAGE_SIZE);
                        if (ret)
                        {
                            LE_ERROR("Can not to write %s at page %d, error: %s",
                                tafFwUpdate.partitions[i].name,
                                pageIdx + blockIdx * pagesPerBlock, strerror(errno));
                            tafFwUpdate.SetErrorCode(errno);
                            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                            taf_pa_flash_CloseMtd(mtdRef);
                            fclose(fp);
                            return;
                        }

                        remainPages--;
                        percent = (pageUpdated + pages - remainPages) * 100 / totalPages;
                        if (percent != tafFwUpdate.percent)
                        {
                            tafFwUpdate.percent = percent;
                            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALLING);
                        }

                        if (remainPages == 0)
                        {
                            break;
                        }
                    }

                    if (remainPages == 0)
                    {
                        break;
                    }
                }

                taf_pa_flash_CloseMtd(mtdRef);
            }
            else
            {
                taf_pa_flash_UbiRef_t ubiRef = nullptr;
                int ret = taf_pa_flash_OpenUbi(tafFwUpdate.partitions[i].name, false, &ubiRef);
                if (ret)
                {
                    LE_ERROR("Fail to open partition %s, ret = %d, error: %s",
                        tafFwUpdate.partitions[i].name, ret, strerror(errno));
                    tafFwUpdate.SetErrorCode(errno);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }

                ret = taf_pa_flash_SetUbiVolUpSize(ubiRef, pages * TAF_FWUPDATE_FLASH_PAGE_SIZE);
                if (ret)
                {
                    LE_ERROR("Fail to set upgrade size for ubi %s, ret = %d, error: %s",
                        tafFwUpdate.partitions[i].name, ret, strerror(errno));
                    tafFwUpdate.SetErrorCode(errno);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    ret = taf_pa_flash_CloseUbi(ubiRef);
                    if (ret)
                        LE_ERROR("Fail to close ubi %s, ret = %d, error: %s",
                            tafFwUpdate.partitions[i].name, ret, strerror(errno));
                    fclose(fp);
                    return;
                }

                pageUpdated = tafFwUpdate.GetPageNumber(TAF_UPDATE_INSTALLING, false);
                uint32_t totalPages = tafFwUpdate.GetPageNumber(TAF_UPDATE_INSTALLING, true);
                uint32_t percent = tafFwUpdate.percent;
                int error = 0;
                for (uint32_t j = 0; j < pages; j++)
                {
                    ret = fread(buffer, 1, TAF_FWUPDATE_FLASH_PAGE_SIZE, fp);
                    if (ret < TAF_FWUPDATE_FLASH_PAGE_SIZE)
                    {
                        memset(buffer + ret, 0xFF, TAF_FWUPDATE_FLASH_PAGE_SIZE - ret);
                        LE_INFO("Padding 0xFF in %s at page %d, start at %d.\n",
                            tafFwUpdate.partitions[i].name, j, ret);
                    }

                    ret = taf_pa_flash_WriteUbi(ubiRef, buffer, TAF_FWUPDATE_FLASH_PAGE_SIZE);
                    if (ret)
                    {
                        error = errno;
                        LE_ERROR("Can not to write %s at page %d, ret = %d, error: %s",
                            tafFwUpdate.partitions[i].name, j, ret, strerror(errno));
                    }

                    percent = (pageUpdated + j) * 100 / totalPages;
                    if (percent != tafFwUpdate.percent)
                    {
                        tafFwUpdate.percent = percent;
                        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALLING);
                    }
                }

                ret = taf_pa_flash_CloseUbi(ubiRef);
                if (error)
                {
                    LE_ERROR("There was write errors on ubi %s.", tafFwUpdate.partitions[i].name);
                    tafFwUpdate.SetErrorCode(error);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }
                if (ret)
                {
                    LE_ERROR("Fail to close partition %s.", tafFwUpdate.partitions[i].name);
                    tafFwUpdate.SetErrorCode(errno);
                    tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                    fclose(fp);
                    return;
                }
            }

            fclose(fp);

            LE_INFO("%s is updated.", tafFwUpdate.partitions[i].name);

            unlink(dataPath);
            tafFwUpdate.SetImageStatus(TAF_UPDATE_INSTALLING, image, true);
            tafFwUpdate.SetPageNumber(TAF_UPDATE_INSTALLING, false, pageUpdated + pages);

            hasImageToUpdate = !tafFwUpdate.GetImageForUpdate(TAF_UPDATE_INSTALLING, image,
                sizeof(image));
        }
        else
        {
            if (tafFwUpdate.GetCancelAction(TAF_UPDATE_INSTALLING))
            {
                LE_INFO("Cancelled during update.");
                tafFwUpdate.UpdateProgress(TAF_UPDATE_IDLE);
            }
            else
            {
                LE_INFO("Paused during update.");
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_PAUSED);
            }

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
        // Post process for install success.
        result = tafFwUpdate.PostProcess(TAF_UPDATE_INSTALL_SUCCESS);
        if (result != LE_OK)
        {
            tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
            return;
        }
        tafFwUpdate.SetActivationContext(TAF_UPDATE_INSTALL_SUCCESS, bank);

        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize partitions with sync context in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::SyncPartition
(
    void
)
{
    uint32_t pages = 0;
    int ret = 0;
    char srcPartition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    char partition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    // 1. Get the next partition for sync.
    le_result_t result = tafFwUpdate.InitPartitionList();
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get partition list");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
        return;
    }

    bool hasPartitionToSync = !tafFwUpdate.GetImageForUpdate(TAF_UPDATE_SYNCHRONIZING, partition,
        sizeof(partition));
    while (hasPartitionToSync)
    {
        if (!tafFwUpdate.GetPauseAction(TAF_UPDATE_SYNCHRONIZING) &&
            !tafFwUpdate.GetCancelAction(TAF_UPDATE_SYNCHRONIZING))
        {
            // 2. Find the partition for sync.
            uint32_t i =0;
            for (i = 0; i < tafFwUpdate.partitions.size(); i++)
            {
                if ((strlen(partition) == strlen(tafFwUpdate.partitions[i].name)) &&
                    (strncmp(partition, tafFwUpdate.partitions[i].name,
                    strlen(partition)) == 0))
                    break;
            }

            if (i >= tafFwUpdate.partitions.size())
            {
                LE_ERROR("Fail to find the partition %s for sync.",partition);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
                return;
            }

            pages = tafFwUpdate.GetImagePageNumber(TAF_UPDATE_SYNCHRONIZING, partition);
            le_utf8_Copy(srcPartition, partition, TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
            if (tafFwUpdate.partitions[i].isUbi)
            {
                if (tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A)
                    srcPartition[strlen(srcPartition) - 1] = 'b';
                else
                    srcPartition[strlen(srcPartition) - 1] = 'a';

                unsigned char buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
                ret = taf_pa_flash_CopyUbi(srcPartition, partition, buffer,
                    TAF_FWUPDATE_FLASH_PAGE_SIZE, pages * TAF_FWUPDATE_FLASH_PAGE_SIZE);
            }
            else
            {
                if (tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A)
                    le_utf8_Append(srcPartition, "_b", TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
                else
                    srcPartition[strlen(srcPartition) - 2] = '\0';

                LE_INFO("Sync MTD from %s to %s with %d bytes.", srcPartition, partition, pages * TAF_FWUPDATE_FLASH_PAGE_SIZE);
                ret = taf_pa_flash_CopyMtd(srcPartition, partition,
                    pages * TAF_FWUPDATE_FLASH_PAGE_SIZE);
            }
            if (ret && ret != TAF_FWUPDATE_FLASH_PAGE_ERASED)
            {
                LE_ERROR("Fail to copy from %s to %s.", srcPartition, partition);
                tafFwUpdate.SetErrorCode(errno);
                tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
                return;
            }

            uint32_t pageUpdated = tafFwUpdate.GetPageNumber(TAF_UPDATE_SYNCHRONIZING, false);
            uint32_t totalPages = tafFwUpdate.GetPageNumber(TAF_UPDATE_SYNCHRONIZING, true);

            tafFwUpdate.percent = (pageUpdated + pages) * 100 / totalPages;
            tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNCHRONIZING);

            LE_INFO("%s is synced.", partition);

            tafFwUpdate.SetImageStatus(TAF_UPDATE_SYNCHRONIZING, partition, true);
            tafFwUpdate.SetPageNumber(TAF_UPDATE_SYNCHRONIZING, false, pageUpdated + pages);

            hasPartitionToSync = !tafFwUpdate.GetImageForUpdate(TAF_UPDATE_SYNCHRONIZING,
                partition, sizeof(partition));
        }
        else
        {
            if (tafFwUpdate.GetCancelAction(TAF_UPDATE_SYNCHRONIZING))
            {
                LE_INFO("Cancelled during sync.");
                tafFwUpdate.UpdateProgress(TAF_UPDATE_IDLE);
            }
            else
            {
                LE_INFO("Paused during sync.");
                tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_PAUSED);
            }

            return;
        }
    }

    tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_SUCCESS);
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
    tafFwUpdate.CleanupContext(TAF_UPDATE_INSTALLING);

    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    char dir[TAF_FWUPDATE_DIRNAME_LEN];
    if (!tafFwUpdate.GetUnpackDir(dir, TAF_FWUPDATE_DIRNAME_LEN))
    {
        LE_ERROR("Fail to get unpack directory.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    tafFwUpdate.SetPackageDataPath(filePath);

    uint32_t totalPage = 0;
    uint32_t imagePage = 0;
    char image[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    char dataPath[TAF_UPDATE_FILE_PATH_LEN];
    size_t i = 0;
    while (i < NUM_ARRAY_MEMBERS(partitonTableInfo))
    {
        if (tafFwUpdate.UnpackImage(filePath, partitonTableInfo[i].dataPath, &imagePage))
        {
            LE_INFO("Detect %s with %d pages to be updated.", partitonTableInfo[i].partition, imagePage);
            snprintf(dataPath, sizeof(dataPath), "%s/%s", dir, partitonTableInfo[i].dataPath);

            if (bank == TAF_UPDATE_BANK_A)
            {
                snprintf(image, sizeof(image), "%s_b", partitonTableInfo[i].partition);

                tafFwUpdate.SetImageStatus(TAF_UPDATE_INSTALLING, image, false);
                tafFwUpdate.SetImagePageNumber(TAF_UPDATE_INSTALLING, image, imagePage);
                tafFwUpdate.SetImageDataPath(image, dataPath);
            }
            else if (bank == TAF_UPDATE_BANK_B)
            {
                if (partitonTableInfo[i].hasSuffix)
                {
                    snprintf(image, sizeof(image), "%s_a", partitonTableInfo[i].partition);
                    tafFwUpdate.SetImageStatus(TAF_UPDATE_INSTALLING, image, false);
                    tafFwUpdate.SetImagePageNumber(TAF_UPDATE_INSTALLING, image, imagePage);
                    tafFwUpdate.SetImageDataPath(image, dataPath);
                }
                else
                {
                    tafFwUpdate.SetImageStatus(TAF_UPDATE_INSTALLING,
                        partitonTableInfo[i].partition, false);
                    tafFwUpdate.SetImagePageNumber(TAF_UPDATE_INSTALLING,
                        partitonTableInfo[i].partition, imagePage);
                    tafFwUpdate.SetImageDataPath(partitonTableInfo[i].partition, dataPath);
                }
            }

            totalPage += imagePage;
        }

        i++;
    }

    tafFwUpdate.SetPageNumber(TAF_UPDATE_INSTALLING, true, totalPage);
    tafFwUpdate.SetPageNumber(TAF_UPDATE_INSTALLING, false, 0);

    tafFwUpdate.UpdateImage();
}

//--------------------------------------------------------------------------------------------------
/**
 * Start synchronization.
 */
//--------------------------------------------------------------------------------------------------
void taf_FwUpdate::StartSync
(
    void
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.CleanupContext(TAF_UPDATE_SYNCHRONIZING);

    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
        return;
    }

    // 1. Get partition layout.
    le_result_t result = tafFwUpdate.InitPartitionList();
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get partition list");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_SYNC_FAIL);
        return;
    }

    // 2. Initialize the partition for synchronization.
    uint32_t totalPage = 0;
    uint32_t partitionPage = 0;
    for (size_t i = 0; i < tafFwUpdate.partitions.size(); i++)
    {
        if ((bank == TAF_UPDATE_BANK_A && tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_B) ||
            (bank == TAF_UPDATE_BANK_B && tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A))
        {
            LE_INFO("Detect %s to be synced.", tafFwUpdate.partitions[i].name);

            uint32_t partitionSize = 0;
            int ret = 0;
            if (tafFwUpdate.partitions[i].isUbi)
            {
                taf_pa_flash_UbiRef_t ubiRef = nullptr;
                ret = taf_pa_flash_OpenUbi(tafFwUpdate.partitions[i].name, true, &ubiRef);
                if (ret)
                {
                    LE_ERROR("Fail to open ubi %s.", tafFwUpdate.partitions[i].name);
                    continue;
                }

                ret = taf_pa_flash_GetUbiVolSize(ubiRef, &partitionSize);
                if (ret)
                {
                    LE_ERROR("Fail to get size of ubi %s.", tafFwUpdate.partitions[i].name);
                    ret = taf_pa_flash_CloseUbi(ubiRef);
                    if (ret)
                    {
                        LE_ERROR("Fail to close ubi %s.", tafFwUpdate.partitions[i].name);
                    }
                    continue;
                }

                ret = taf_pa_flash_CloseUbi(ubiRef);
                if (ret)
                {
                    LE_ERROR("Fail to close ubi %s.", tafFwUpdate.partitions[i].name);
                    continue;
                }
            }
            else
            {
                char partition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
                le_utf8_Copy(partition, tafFwUpdate.partitions[i].name,
                    TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
                if (tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A)
                {
                    le_utf8_Append(partition, "_b", TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
                }
                else
                    partition[strlen(tafFwUpdate.partitions[i].name) - 2] = '\0';

                taf_pa_flash_MtdRef_t mtdRef = nullptr;
                ret = taf_pa_flash_OpenMtd(partition, &mtdRef);
                if (ret)
                {
                    LE_ERROR("Fail to open mtd %s.", partition);
                    continue;
                }

                mtd_info_t info;
                ret = taf_pa_flash_GetMtdInfo(mtdRef, &info);
                if (ret)
                {
                    LE_ERROR("Fail to get info of mtd %s.", partition);
                    taf_pa_flash_CloseMtd(mtdRef);
                    continue;
                }

                uint8_t buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
                uint32_t blocks = info.size / info.erasesize;
                uint32_t pagesPerBlock = info.erasesize / info.writesize;
                for (uint32_t i = 0; i < blocks; i++)
                {
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, i);
                    if (ret == 0)
                        continue;
                    else
                    {
                        ret = taf_pa_flash_ReadMtdPage(mtdRef, buffer, i * pagesPerBlock,
                            TAF_FWUPDATE_FLASH_PAGE_SIZE);
                        if (ret == TAF_FWUPDATE_FLASH_PAGE_ERASED)
                            break;

                        partitionSize += info.erasesize;
                    }
                }

                taf_pa_flash_CloseMtd(mtdRef);
            }

            partitionPage =  partitionSize / TAF_FWUPDATE_FLASH_PAGE_SIZE;
            tafFwUpdate.SetImageStatus(TAF_UPDATE_SYNCHRONIZING,
                tafFwUpdate.partitions[i].name, false);
            tafFwUpdate.SetImagePageNumber(TAF_UPDATE_SYNCHRONIZING,
                tafFwUpdate.partitions[i].name, partitionPage);

            totalPage += partitionPage;
        }
    }

    tafFwUpdate.SetPageNumber(TAF_UPDATE_SYNCHRONIZING, true, totalPage);
    tafFwUpdate.SetPageNumber(TAF_UPDATE_SYNCHRONIZING, false, 0);

    tafFwUpdate.SyncPartition();
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

    // 7. Post process for install success.
    ret = tafFwUpdate.PostProcess(TAF_UPDATE_INSTALL_SUCCESS);
    if (ret != LE_OK)
    {
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }
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
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL)
    {
        LE_ERROR("md_ctx is NULL.");
        return LE_FAULT;
    }

    const EVP_MD *md = EVP_sha1();
    if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
    {
        LE_ERROR("Fail to initiate sha1 context.");
        EVP_MD_CTX_free(md_ctx);
        return LE_FAULT;
    }

    uint8_t content[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    int bytes = 0;
    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        LE_ERROR("Fail to open %s.", filePath);
        return LE_FAULT;
    }
    while ((bytes = fread(content, 1, TAF_FWUPDATE_FLASH_PAGE_SIZE, file)) != 0)
    {
        EVP_DigestUpdate(md_ctx, content, bytes);
        *calSize += bytes;
    }
    fclose(file);

    EVP_DigestFinal_ex(md_ctx, hash, hashLen);
    EVP_MD_CTX_free(md_ctx);

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
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    le_result_t result = tafFwUpdate.InitPartitionList();
    if (result != LE_OK)
    {
        LE_ERROR("Get partition list failed.");
        return LE_FAULT;
    }

    uint32_t i = 0;
    int ret = 0;
    for (i = 0; i < tafFwUpdate.partitions.size(); i++)
    {
        if (strncmp(partition, tafFwUpdate.partitions[i].name, strlen(partition)) == 0 &&
            strlen(partition) == strlen(tafFwUpdate.partitions[i].name))
            break;
    }

    if (i == tafFwUpdate.partitions.size())
    {
        LE_ERROR("Partition %s not found.", partition);
        return LE_FAULT;
    }

    taf_pa_flash_MtdRef_t mtdRef = nullptr;
    taf_pa_flash_UbiRef_t ubiRef = nullptr;
    if (tafFwUpdate.partitions[i].isUbi)
        ret = taf_pa_flash_OpenUbi(partition, true, &ubiRef);
    else
        ret = taf_pa_flash_OpenMtd(partition, &mtdRef);
    if (ret)
    {
        LE_ERROR("Fail to open partition %s, ret: %d", partition, ret);
        return LE_FAULT;
    }

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL)
    {
        LE_ERROR("md_ctx is NULL.");
        return LE_FAULT;
    }

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
    uint32_t pagesPerBlock = TAF_FWUPDATE_FLASH_MTD_EB_SIZE / TAF_FWUPDATE_FLASH_PAGE_SIZE;
    uint32_t block = 0;
    uint32_t totalBlock = 0;
    mtd_info_t info;

    if (!tafFwUpdate.partitions[i].isUbi)
    {
        ret = taf_pa_flash_GetMtdInfo(mtdRef, &info);
        if (ret)
        {
            LE_ERROR("Fail to get info of mtd %s, ret: %d", partition, ret);
            taf_pa_flash_CloseMtd(mtdRef);
            EVP_MD_CTX_free(md_ctx);
            return LE_FAULT;
        }
        totalBlock = info.size / info.erasesize;
    }

    for (uint32_t j = 0; j < iteration; j++)
    {
        bytes = TAF_FWUPDATE_FLASH_PAGE_SIZE;
        if (!tafFwUpdate.partitions[i].isUbi)
        {
            // If it is the 1st page of the block, check the block status.
            if (j % pagesPerBlock == 0)
            {
                // Get the next good block.
                ret = taf_pa_flash_IsGoodBlock(mtdRef, block);
                while (ret == 0)
                {
                    LE_WARN("Skip reading bad block at %d", block);
                    block++;

                    if (block >= totalBlock)
                    {
                        LE_ERROR("Invalid block index %d.", block);
                        EVP_MD_CTX_free(md_ctx);
                        taf_pa_flash_CloseMtd(mtdRef);
                        break;
                    }
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, block);
                }
            }

            ret = taf_pa_flash_ReadMtdPage(mtdRef, content,
                j % pagesPerBlock + block * pagesPerBlock, bytes);

            // If it is the last page of the block, move to the next block.
            if (j % pagesPerBlock == pagesPerBlock - 1)
                block++;
        }
        else
            ret = taf_pa_flash_ReadUbi(ubiRef, content, j * TAF_FWUPDATE_FLASH_PAGE_SIZE, bytes);

        if (ret < 0 && ret != TAF_FWUPDATE_FLASH_PAGE_ERASED)
        {
            LE_ERROR("Fail to read partition %s at iteration %d, ret: %d",
                partition, j, ret);
            EVP_MD_CTX_free(md_ctx);
            if (!tafFwUpdate.partitions[i].isUbi)
                taf_pa_flash_CloseMtd(mtdRef);
            else
            {
                ret = taf_pa_flash_CloseUbi(ubiRef);
                if (ret)
                    LE_ERROR("Fail to close ubi %s, ret = %d.", partition, ret);
            }
            return LE_FAULT;
        }

        EVP_DigestUpdate(md_ctx, content, bytes);
    }

    bytes = calSize % TAF_FWUPDATE_FLASH_PAGE_SIZE;
    if (bytes != 0)
    {
        if (!tafFwUpdate.partitions[i].isUbi)
        {
            // If it is the 1st page of the block, check the block status.
            if (iteration % pagesPerBlock == 0)
            {
                // Get the next good block.
                ret = taf_pa_flash_IsGoodBlock(mtdRef, block);
                while (ret == 0)
                {
                    LE_WARN("Skip reading bad block at %d", block);
                    block++;

                    if (block >= totalBlock)
                    {
                        LE_ERROR("Invalid block index %d.", block);
                        EVP_MD_CTX_free(md_ctx);
                        taf_pa_flash_CloseMtd(mtdRef);
                        break;
                    }
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, block);
                }
            }

            ret = taf_pa_flash_ReadMtdPage(mtdRef, content,
                iteration % pagesPerBlock + block * pagesPerBlock, bytes);
        }
        else
            ret = taf_pa_flash_ReadUbi(ubiRef, content,
                iteration * TAF_FWUPDATE_FLASH_PAGE_SIZE, bytes);

        if (ret < 0 && ret != TAF_FWUPDATE_FLASH_PAGE_ERASED)
        {
            LE_ERROR("Fail to read partition %s at iteration %d, ret: %d",
                partition, iteration, ret);
            EVP_MD_CTX_free(md_ctx);
            if (!tafFwUpdate.partitions[i].isUbi)
                taf_pa_flash_CloseMtd(mtdRef);
            else
            {
                ret = taf_pa_flash_CloseUbi(ubiRef);
                if (ret)
                    LE_ERROR("Fail to close ubi %s, ret = %d.", partition, ret);
            }
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
    if (tafFwUpdate.partitions[i].isUbi)
    {
        ret = taf_pa_flash_CloseUbi(ubiRef);
        if (ret)
        {
            LE_ERROR("Fail to close ubi %s, ret = %d.", partition, ret);
            return LE_FAULT;
        }
    }
    else
        taf_pa_flash_CloseMtd(mtdRef);

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
    unlink(filePath);
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

    char dir[TAF_FWUPDATE_DIRNAME_LEN];
    if (!tafFwUpdate.GetUnpackDir(dir, TAF_FWUPDATE_DIRNAME_LEN))
    {
        LE_ERROR("Fail to get unpack directory.");
        tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
        return;
    }

    uint32_t imagePage = 0;
    bool verified = false;
    char partition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    char dataPath[TAF_UPDATE_FILE_PATH_LEN];

    size_t i = 0;
    while (i < NUM_ARRAY_MEMBERS(partitonTableInfo))
    {
        if (tafFwUpdate.UnpackImage(filePath, partitonTableInfo[i].dataPath, &imagePage))
        {
            LE_INFO("Install post-check on %s.", partitonTableInfo[i].partition);
            snprintf(dataPath, sizeof(dataPath), "%s/%s", dir, partitonTableInfo[i].dataPath);

            if (bank == TAF_UPDATE_BANK_A)
            {
                snprintf(partition, sizeof(partition), "%s_b", partitonTableInfo[i].partition);
                verified = tafFwUpdate.VerifyHash(partition, dataPath);
            }
            else if (bank == TAF_UPDATE_BANK_B)
            {
                if (partitonTableInfo[i].hasSuffix)
                {
                    snprintf(partition, sizeof(partition), "%s_a", partitonTableInfo[i].partition);
                    verified = tafFwUpdate.VerifyHash(partition, dataPath);
                }
                else
                {
                    verified = tafFwUpdate.VerifyHash(partitonTableInfo[i].partition, dataPath);
                }
            }

            if (!verified || bank == TAF_UPDATE_BANK_UNKNOWN)
            {
                LE_ERROR("Install post-check failure on %s.", partitonTableInfo[i].partition);
                tafFwUpdate.error = TAF_UPDATE_SECURITY_FAILURE;
                tafFwUpdate.UpdateProgress(TAF_UPDATE_INSTALL_FAIL);
                return;
            }

            LE_INFO("Install post-check on %s success.", partitonTableInfo[i].partition);
        }

        i++;
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

    if (fgets(cmdRes, sizeof(cmdRes), fp) == NULL)
    {
        LE_ERROR("Failed to read fp.");
        pclose(fp);
        return LE_FAULT;
    }

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
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    int ret = 0;
    le_result_t result = InitPartitionList();
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can not get partition list.");

    for (uint32_t i = 0; i < tafFwUpdate.partitions.size(); i++)
    {
        if (tafFwUpdate.partitions[i].bank == bank)
        {
            if (!tafFwUpdate.partitions[i].isUbi)
            {
                if (strncmp(tafFwUpdate.partitions[i].name, "sbl", strlen("sbl")) == 0)
                {
                    LE_INFO("Skip erasing MTD %s.", tafFwUpdate.partitions[i].name);
                    continue;
                }
                LE_INFO("Erasing MTD %s...", tafFwUpdate.partitions[i].name);

                taf_pa_flash_MtdRef_t mtdRef = nullptr;
                ret = taf_pa_flash_OpenMtd(tafFwUpdate.partitions[i].name, &mtdRef);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT,
                    "Fail to open MTD partition, ret: %d", ret);

                mtd_info_t info;
                ret = taf_pa_flash_GetMtdInfo(mtdRef, &info);
                TAF_ERROR_IF_RET_VAL(ret || info.erasesize == 0, LE_FAULT,
                    "Fail to get MTD information, ret: %d", ret);

                uint32_t blockNum = info.size / info.erasesize;
                for (uint32_t j = 0; j < blockNum; j++)
                {
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, j);
                    if (ret == 0)
                    {
                        LE_DEBUG("Detect bad block at %d.", j);
                    }
                    else if (ret == 1)
                    {
                        ret = taf_pa_flash_EraseBlock(mtdRef, j);
                        if (ret)
                        {
                            LE_ERROR("Fail to erase block %d.", j);
                            ret = taf_pa_flash_MarkBadBlock(mtdRef, j);
                            if (ret)
                                LE_ERROR("Fail to mark bad block %d.", j);
                        }
                    }
                }

                taf_pa_flash_CloseMtd(mtdRef);
            }
            else
            {
                LE_INFO("Erasing UBI %s...", tafFwUpdate.partitions[i].name);
                int ret = taf_pa_flash_EraseUbi(tafFwUpdate.partitions[i].name);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Fail to erase UBI, ret: %d", ret);
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

    int ret = 0;
    uint32_t imageSize = 0;
    taf_update_Bank_t bootBank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&bootBank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }

    le_result_t result = tafFwUpdate.InitPartitionList();
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can not get partition list.");

    char partition[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    unsigned char buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
    for (uint32_t i = 0; i < tafFwUpdate.partitions.size(); i++)
    {
        if (tafFwUpdate.partitions[i].bank == bootBank)
        {
            le_utf8_Copy(partition, tafFwUpdate.partitions[i].name,
                TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);

            if (tafFwUpdate.partitions[i].isUbi)
            {
                taf_pa_flash_UbiRef_t ubiRef = nullptr;
                ret = taf_pa_flash_OpenUbi(tafFwUpdate.partitions[i].name, true, &ubiRef);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Can not open UBI %s, ret = %d.",
                    tafFwUpdate.partitions[i].name, ret);

                ret = taf_pa_flash_GetUbiVolSize(ubiRef, &imageSize);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Can not get UBI %s volume size, ret = %d.",
                    tafFwUpdate.partitions[i].name, ret);

                ret = taf_pa_flash_CloseUbi(ubiRef);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Can not close UBI %s, ret = %d.",
                    tafFwUpdate.partitions[i].name, ret);

                if (tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A)
                {
                    partition[strlen(partition) - 1] = 'b';
                }
                else
                {
                    partition[strlen(partition) - 1] = 'a';
                }

                LE_INFO("Sync UBI from %s to %s.", tafFwUpdate.partitions[i].name, partition);
                ret = taf_pa_flash_CopyUbi(tafFwUpdate.partitions[i].name, partition, buffer,
                    TAF_FWUPDATE_FLASH_PAGE_SIZE, imageSize);
            }
            else
            {
                taf_pa_flash_MtdRef_t mtdRef = nullptr;
                ret = taf_pa_flash_OpenMtd(tafFwUpdate.partitions[i].name, &mtdRef);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Can not open MTD %s, ret = %d.", tafFwUpdate.partitions[i].name, ret);

                mtd_info_t info;
                uint8_t buffer[TAF_FWUPDATE_FLASH_PAGE_SIZE];
                ret = taf_pa_flash_GetMtdInfo(mtdRef, &info);
                TAF_ERROR_IF_RET_VAL(ret, LE_FAULT, "Can not get MTD %s info, ret = %d.", tafFwUpdate.partitions[i].name, ret);
                imageSize = 0;
                uint32_t blocks = info.size / info.erasesize;
                uint32_t pagesPerBlock = info.erasesize / info.writesize;
                for (uint32_t i = 0; i < blocks; i++)
                {
                    ret = taf_pa_flash_IsGoodBlock(mtdRef, i);
                    if (ret == 0)
                        continue;
                    else
                    {
                        ret = taf_pa_flash_ReadMtdPage(mtdRef, buffer, i * pagesPerBlock,
                            TAF_FWUPDATE_FLASH_PAGE_SIZE);
                        if (ret == TAF_FWUPDATE_FLASH_PAGE_ERASED)
                            break;

                        imageSize += info.erasesize;
                    }
                }

                taf_pa_flash_CloseMtd(mtdRef);

                if (tafFwUpdate.partitions[i].bank == TAF_UPDATE_BANK_A)
                {
                    le_utf8_Append(partition, "_b", TAF_FLASH_PARTITION_NAME_MAX_BYTES, NULL);
                }
                else
                {
                    partition[strlen(partition) - 2] = '\0';
                }

                LE_INFO("Sync MTD from %s to %s with %d bytes.", tafFwUpdate.partitions[i].name, partition, imageSize);
                ret = taf_pa_flash_CopyMtd(tafFwUpdate.partitions[i].name, partition, imageSize);
            }
            TAF_ERROR_IF_RET_VAL(ret && ret != TAF_FWUPDATE_FLASH_PAGE_ERASED, LE_FAULT,
                "Fail to copy from %s to %s, ret = %d.", tafFwUpdate.partitions[i].name, partition, ret);
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
            else if (updateReq->event == TAF_FWUPDATE_EV_START_SYNC)
            {
                LE_INFO("NAD sync start.");
                tafFwUpdate.error = TAF_UPDATE_NONE;
                tafFwUpdate.StartSync();
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_INSTALL_POST_CHECK)
            {
                LE_INFO("Installation post check.");
                tafFwUpdate.GetPackageDataPath(updateReq->filePath, TAF_UPDATE_FILE_PATH_LEN);
                tafFwUpdate.InstallPostCheck(updateReq->filePath);
            }
            else if (updateReq->event == TAF_FWUPDATE_EV_ROLLBACK)
            {
                LE_INFO("Start to rollback.");
                tafFwUpdate.SetState(TAF_UPDATE_ROLLBACK);
                le_result_t result = tafFwUpdate.Rollback();
                if (result == LE_OK)
                    result = tafFwUpdate.PostProcess(TAF_UPDATE_ROLLBACK_SUCCESS);

                if (result != LE_OK)
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
                tafFwUpdate.SetPauseAction(TAF_UPDATE_INSTALLING, false);
                tafFwUpdate.SetState(TAF_UPDATE_INSTALLING);
                tafFwUpdate.UpdateImage();
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for installing state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_SYNC_PAUSED:
            if (updateReq->event == TAF_FWUPDATE_EV_RESUME_SYNC)
            {
                LE_INFO("Resume NAD sync.");
                tafFwUpdate.SetPauseAction(TAF_UPDATE_SYNCHRONIZING, false);
                tafFwUpdate.SetState(TAF_UPDATE_SYNCHRONIZING);
                tafFwUpdate.SyncPartition();
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
 * Post process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_FwUpdate::PostProcess
(
    taf_update_State_t state ///< [IN] Update state.
)
{
    char script[TAF_FWUPDATE_POST_SCRIPT_PATH_LEN];
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    le_result_t result = tafFwUpdate.GetPostScript(state, script, TAF_FWUPDATE_POST_SCRIPT_PATH_LEN);
    if (result != LE_OK)
    {
        LE_ERROR("Can not get user script.");
        return LE_FAULT;
    }

    if (access(TAF_FWUPDATE_POST_HOOK, 0) == 0 && access(script, 0) == 0)
    {
        LE_INFO("Post processing...");

        char cmd[TAF_FWUPDATE_CMD_LEN];
        snprintf(cmd, sizeof(cmd), "%s %s", TAF_FWUPDATE_POST_HOOK, script);

        result = tafFwUpdate.SendPipeCmd(cmd, "w");
        if (result != LE_OK)
        {
            LE_ERROR("Post processing failed.");
            return LE_FAULT;
        }

        LE_INFO("Post processing successfully.");
        return LE_OK;
    }

    LE_INFO("Skip post processing.");
    return LE_OK;
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

    // 2. Create thread for firmware update.
    le_sem_Ref_t semaphore = le_sem_Create("fwUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("fwUpdateThread", FwUpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 3. Initiate from current state.
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
        default:
            LE_ERROR("FOTA invalid state(%d), reset to idle.", state);
            SetState(TAF_UPDATE_IDLE);
    }

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafFwUpdate component: %lfs.", elapsedTime.count());
}
