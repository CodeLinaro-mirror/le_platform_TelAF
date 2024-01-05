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
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sys/sendfile.h>

#include "tafUpdate.hpp"
#include "tafAppMgmt.hpp"

using namespace telux::tafsvc;
using namespace std;
using namespace boost::property_tree;

le_event_Id_t taf_AppMgmt::appUpdateEvId = nullptr;
le_event_Id_t taf_AppMgmt::jsonParseEvId = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for app list and app info.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM, sizeof(taf_AppMgmtAppList_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_appMgmt_AppInfo_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM,
    sizeof(taf_AppMgmtAppInfoSafeRef_t));

//--------------------------------------------------------------------------------------------------
/**
 * Map for app list and app info.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
LE_REF_DEFINE_STATIC_MAP(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Get instance of app management.
 */
//--------------------------------------------------------------------------------------------------
taf_AppMgmt &taf_AppMgmt::GetInstance
(
    void
)
{
    static taf_AppMgmt instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app exists in config tree.
 *
 * @return
 * - true if app exists.
 * - false if app not exists.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsAppExist
(
    const char* appName ///< [IN] App name.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_APPMGMT_SYSTEM_APPS);
    bool exist = le_cfg_NodeExists(rdIter, appName);
    le_cfg_CancelTxn(rdIter);
    return exist;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app is in manual start mode.
 *
 * @return
 * - true if app is in manual start mode.
 * - false if app is not in manual start mode.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsStartManual
(
    const char* appName ///< [IN] App name.
)
{
    // 1. Find app node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_APPMGMT_SYSTEM_APPS_NODE, appName);

    // 2. Get app start mode.
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
    bool startManual = le_cfg_GetBool(rdIter, "startManual", false);
    le_cfg_CancelTxn(rdIter);
    return startManual;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app is activated.
 *
 * @return
 * - true if app is activated.
 * - false if app is inactivated.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsActivated
(
    const char* appName ///< [IN] App name.
)
{
    if (appName == NULL)
        return false;

    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_APPMGMT_SYSTEM_APPS);
    bool activated = false;

    if (le_cfg_NodeExists(rdIter, appName))
    {
        // 1. Find app node in config tree.
        char node[LE_CFG_STR_LEN_BYTES] = { 0 };
        snprintf(node, sizeof(node), TAF_APPMGMT_SYSTEM_APPS_NODE, appName);

        // 2. Get app activate state.
        le_cfg_IteratorRef_t appIter = le_cfg_CreateReadTxn(node);
        activated = le_cfg_GetBool(appIter, "activated", true);
        le_cfg_CancelTxn(appIter);
    }

    le_cfg_CancelTxn(rdIter);
    return activated;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app is system integrated.
 *
 * @return
 * - true if app is system integrated.
 * - false if app is not system integrated.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsSysApp
(
    const char* appName ///< [IN] App name.
)
{
    bool isSysApp = false;
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    if (tafAppMgmt.IsAppExist(appName))
    {
        char node[LE_CFG_STR_LEN_BYTES] = { 0 };
        snprintf(node, sizeof(node), TAF_APPMGMT_SYSTEM_APPS_NODE, appName);

        le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(node);
        if (!le_cfg_NodeExists(rdIter, "activated"))
        {
            isSysApp = true;
        }
        le_cfg_CancelTxn(rdIter);
    }

    return isSysApp;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for json event.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::VersionEventHandler
(
    le_json_Event_t event ///< [IN] Json event.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    le_utf8_Copy(tafAppMgmt.appVersion, le_json_GetString(), TAF_APPMGMT_APP_VERSION_BYTES, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for json event.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::JsonEventHandler
(
    le_json_Event_t event ///< [IN] Json event.
)
{
    const char* verStr;
    le_json_ParsingSessionRef_t sessRef;
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    switch (event) {
        case LE_JSON_OBJECT_START:
            LE_INFO("Start json object parsing.");
            break;
        case LE_JSON_OBJECT_END:
            LE_INFO("Complete json object parsing.");
            break;
        case LE_JSON_DOC_END:
            LE_INFO("Complete json doc parsing.");

            sessRef = le_json_GetSession();
            if (sessRef != NULL)
                le_json_Cleanup(sessRef);
            if (tafAppMgmt.jsonFd > 0)
                close(tafAppMgmt.jsonFd);
            le_sem_Post(tafAppMgmt.jsonSem);
            break;
        case LE_JSON_OBJECT_MEMBER:
            verStr = le_json_GetString();
            if (strcmp(verStr, "version") == 0)
                le_json_SetEventHandler(VersionEventHandler);
            break;
        case LE_JSON_NUMBER:
        case LE_JSON_STRING:
        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        default:
            LE_WARN("Warning event : %s.", le_json_GetEventName(event));
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for json error.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::JsonErrorHandler
(
    le_json_Error_t error, ///< [IN] Json parsing error.
    const char* msg        ///< [IN] Message of error.
)
{
    LE_ERROR("Json parisng error.");

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    le_json_ParsingSessionRef_t sessRef = le_json_GetSession();
    if (sessRef != NULL)
        le_json_Cleanup(sessRef);
    if (tafAppMgmt.jsonFd != -1)
        close(tafAppMgmt.jsonFd);

    le_sem_Post(tafAppMgmt.jsonSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Json parse handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::JsonParseHandler
(
    void* contextPtr ///< [IN] Context.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    (void)le_json_Parse(tafAppMgmt.jsonFd, JsonEventHandler, JsonErrorHandler, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for parsing json file.
 */
//--------------------------------------------------------------------------------------------------
void* taf_AppMgmt::JsonParseThread
(
    void* contextPtr ///< [IN] Context.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    tafAppMgmt.jsonSem = le_sem_Create("jsonSem", 0);
	le_event_AddHandler("jsonParseHandler", jsonParseEvId, JsonParseHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get app version.
 *
 * @return
 * - LE_OK if app version is successfully retrieved.
 * - LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::GetAppVersion
(
    const char* appName,      ///< [IN] App name.
    char* versionPtr,         ///< [OUT] App version.
    size_t versionNumElements ///< [IN] App version size.
)
{
    le_cfg_IteratorRef_t rdIter = le_cfg_CreateReadTxn(TAF_APPMGMT_SYSTEM_APPS);
    if (le_cfg_GoToFirstChild(rdIter) == LE_NOT_FOUND) {
        LE_ERROR("No apps installed.");
        le_cfg_CancelTxn(rdIter);
        return LE_NOT_FOUND;
    }

    le_result_t result = LE_NOT_FOUND;
    do {
        // Get app name
        char name[TAF_APPMGMT_APP_NAME_BYTES] = { 0 };
        le_cfg_GetNodeName(rdIter, "", name, TAF_APPMGMT_APP_NAME_BYTES);

        if (strcmp(appName, name) == 0) {
            // Get app version
            le_cfg_GetString(rdIter, "version", versionPtr, versionNumElements, "");
            result = LE_OK;
            break;
        }
    } while (le_cfg_GoToNextSibling(rdIter) == LE_OK);

    le_cfg_CancelTxn(rdIter);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app version is valid. A version number consists of one or more revision numbers, each
 * revision number is connected by a '.'. Each revision number consists of multiple digits and may
 * contain leading zeros.
 *
 * @return
 * - true if app version is valid.
 * - false if app version is invalid.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsValidVersion
(
    const char* version ///< [IN] App version.
)
{
    if (version == NULL)
        return true;

    for (size_t i = 0; i < strlen(version); i++)
    {
        if (version[i] == '.')
            continue;

        if (version[i] >= '0' && version[i] <= '9')
            continue;

        LE_ERROR("%s has illegal character %c.", version, version[i]);
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if app version is valid for installation.
 *
 * @return
 * - true if app version is valid for installation.
 * - false if app version is invalid.
 */
//--------------------------------------------------------------------------------------------------
bool taf_AppMgmt::IsValidToInstall
(
    const char* appName, ///< [IN] App name.
    const char* appPath  ///< [IN] File path of app.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    if (tafAppMgmt.IsAppExist(appName))
    {
        char version[TAF_APPMGMT_APP_VERSION_BYTES] = {0};
        if (tafAppMgmt.GetAppVersion(appName, version, TAF_APPMGMT_APP_VERSION_BYTES) != LE_OK)
        {
            LE_ERROR("Fail to get %s version.", appName);
            return false;
        }

        tafAppMgmt.jsonFd = open(appPath, O_RDONLY);
        if(tafAppMgmt.jsonFd < 0)
        {
            LE_ERROR("Fail to parse version in %s.", appPath);
            return false;
        }

        le_event_Report(taf_AppMgmt::jsonParseEvId, &tafAppMgmt.jsonFd, sizeof(int));

        le_clk_Time_t time = {TAF_APPMGMT_JSON_PARSE_TIMEOUT, 0};
        le_result_t result  = le_sem_WaitWithTimeOut(tafAppMgmt.jsonSem, time);
        if (result != LE_OK)
        {
            LE_ERROR("Json parisng timeout.");
            return false;
        }

        if (!tafAppMgmt.IsValidVersion(version) || !tafAppMgmt.IsValidVersion(version))
        {
            LE_ERROR("App vesion is invalid.");
            return false;
        }

        if (strcmp(version, "") == 0)
        {
            LE_INFO("%s has no version.", appName);
            return true;
        } else if (strcmp(tafAppMgmt.appVersion, "") == 0)
        {
            LE_ERROR("%s bundle has no version.", appName);
            return false;
        }

        LE_INFO("%s update from %s to %s.", appName, version, tafAppMgmt.appVersion);

        string v1(version), v2(tafAppMgmt.appVersion);
        istringstream issv1(v1 + '.'), issv2(v2 + '.');
        int d1 = 0, d2 = 0;
        char dot = '.';
        while (issv1.good() || issv2.good())
        {
            if (issv1.good())
                issv1 >> d1 >> dot;
            if (issv2.good())
                issv2 >> d2 >> dot;

            if (d1 < d2)
                return true;
            else if (d1 > d2)
                return false;

            d1 = d2 = 0;
        }
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update app activate state in config tree.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::UpdateAppNode
(
    const char* appName, ///< [IN] App name.
    bool activated       ///< [IN] Activate state.
)
{
    // 1. Find app node in config tree.
    char node[LE_CFG_STR_LEN_BYTES] = { 0 };
    snprintf(node, sizeof(node), TAF_APPMGMT_SYSTEM_APPS_NODE, appName);

    // 2. Set activate state.
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);
    le_cfg_SetBool(wrIter, "activated", activated);
    le_cfg_CommitTxn(wrIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Update progress.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::UpdateProgress
(
    taf_update_State_t state, ///< [IN] Update state.
    uint32_t percent,         ///< [IN] Update percent.
    taf_update_Error_t error  ///< [IN] Update error.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtUpdateReq_t updateReq;

    // 1. Update state.
    switch (state)
    {
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing app %s %d%%.", tafAppMgmt.appName, percent);
            break;
        case TAF_UPDATE_INSTALL_FAIL:
            LE_ERROR("Install app %s failed.", tafAppMgmt.appName);
            tafAppMgmt.state = TAF_UPDATE_IDLE;
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            tafAppMgmt.UpdateAppNode(tafAppMgmt.appName, false);
            if (tafAppMgmt.IsStartManual(tafAppMgmt.appName))
            {
                LE_INFO("Install app %s (manual-start) successfully.", tafAppMgmt.appName);
                tafAppMgmt.state = TAF_UPDATE_IDLE;
            }
            else
            {
                LE_INFO("Install app %s (auto-start) successfully.", tafAppMgmt.appName);
                if (tafAppMgmt.prbtTime)
                {
                    tafAppMgmt.state = TAF_UPDATE_PROBATION;
                    LE_INFO("Starting probation timer...");
                    le_timer_SetRepeat(tafAppMgmt.prbtTimerRef, tafAppMgmt.prbtTime);
                    le_timer_Start(tafAppMgmt.prbtTimerRef);
                }
                else
                {
                    tafAppMgmt.state = TAF_UPDATE_IDLE;
                }
            }
            break;
        case TAF_UPDATE_PROBATION:
            LE_INFO("App %s is in probation %d%%.", tafAppMgmt.appName, percent);
            break;
        case TAF_UPDATE_PROBATION_SUCCESS:
            LE_INFO("App %s probation succeeded.", tafAppMgmt.appName);
            tafAppMgmt.state = TAF_UPDATE_IDLE;
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            LE_ERROR("App %s probation failed.", tafAppMgmt.appName);
            tafAppMgmt.state = TAF_UPDATE_PROBATION_FAIL;
            updateReq.event = TAF_APPMGMT_EV_ROLLBACK;
            le_event_Report(taf_AppMgmt::appUpdateEvId, &updateReq,
                sizeof(taf_AppMgmtUpdateReq_t));
            break;
        case TAF_UPDATE_ROLLBACK:
            LE_INFO("App %s is in rollback %d%%.", tafAppMgmt.appName, percent);
            break;
        case TAF_UPDATE_ROLLBACK_SUCCESS:
            LE_INFO("App %s rollback succeeded.", tafAppMgmt.appName);
            tafAppMgmt.UpdateAppNode(tafAppMgmt.appName, true);
            tafAppMgmt.state = TAF_UPDATE_IDLE;
            break;
        case TAF_UPDATE_ROLLBACK_FAIL:
            LE_ERROR("App %s rollback failed.", tafAppMgmt.appName);
            tafAppMgmt.state = TAF_UPDATE_IDLE;
            break;
        default:
            break;
    }

    // 2. Report current status to user.
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;
    stateInd.ota = TAF_UPDATE_SOTA;
    stateInd.percent = percent;
    stateInd.error = error;
    stateInd.state = state;
    le_utf8_Copy(stateInd.name, tafAppMgmt.appName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
    le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Probation timer handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::ProbationTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Probation timer reference.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    uint32_t time = le_timer_GetExpiryCount(timerRef);
    uint32_t percent = time * 100 / tafAppMgmt.prbtTime;

    if (time >= tafAppMgmt.prbtTime)
    {
        LE_INFO("Stopping probation timer...");
        le_timer_Stop(tafAppMgmt.prbtTimerRef);

        tafAppMgmt.UpdateProgress(TAF_UPDATE_PROBATION_SUCCESS, 100, TAF_UPDATE_NONE);

        LE_INFO("App %s is activated.", tafAppMgmt.appName);
        tafAppMgmt.UpdateAppNode(tafAppMgmt.appName, true);
    }
    else
    {
        if (le_appInfo_GetState(tafAppMgmt.appName) == LE_APPINFO_STOPPED)
        {
            LE_ERROR("Detect app %s is not running.", tafAppMgmt.appName);
            LE_INFO("Stopping probation timer...");
            le_timer_Stop(tafAppMgmt.prbtTimerRef);
            tafAppMgmt.UpdateProgress(TAF_UPDATE_PROBATION_FAIL, percent,
                TAF_UPDATE_APP_NOT_RUNNING);
        }
        else
        {
            tafAppMgmt.UpdateProgress(TAF_UPDATE_PROBATION, percent, TAF_UPDATE_NONE);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Install handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::InstallHandler
(
    le_update_State_t state, ///< [IN] Update state.
    uint percent,            ///< [IN] Update percent.
    void* contextPtr         ///< [IN] Context.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_update_Error_t error = TAF_UPDATE_NONE;

    switch (state)
    {
        case LE_UPDATE_STATE_DOWNLOAD_SUCCESS:
            LE_DEBUG("Install init.");
            le_update_Install();
            break;
        case LE_UPDATE_STATE_UNPACKING:
            LE_DEBUG("Unpaking %d%%...", percent);
            tafAppMgmt.UpdateProgress(tafAppMgmt.state, percent, error);
            break;
        case LE_UPDATE_STATE_APPLYING:
            LE_DEBUG("Applying ...");
            break;
        case LE_UPDATE_STATE_SUCCESS:
            LE_DEBUG("Install success.");
            le_update_End();
            if (tafAppMgmt.state == TAF_UPDATE_INSTALLING)
                tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_SUCCESS, percent, error);
            else if (tafAppMgmt.state == TAF_UPDATE_ROLLBACK)
                tafAppMgmt.UpdateProgress(TAF_UPDATE_ROLLBACK_SUCCESS, percent, error);
            break;
        case LE_UPDATE_STATE_FAILED:
        default:
            LE_DEBUG("Install failed.");
            error = (taf_update_Error_t)le_update_GetErrorCode();
            le_update_End();
            if (tafAppMgmt.state == TAF_UPDATE_INSTALLING)
                tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, percent, error);
            else if (tafAppMgmt.state == TAF_UPDATE_ROLLBACK)
                tafAppMgmt.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL, percent, error);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send pipe command.
 *
 * @return
 * - LE_OK if the command is executed correctly.
 * - LE_FAULT if an error occurred while executing the command.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::SendPipeCmd
(
    const char* cmd ///< [IN] Command.
)
{
    FILE* fp = popen(cmd, "w");
    TAF_ERROR_IF_RET_VAL(fp == NULL, LE_FAULT, "popen failed.");

    int res = pclose(fp);
    if (WIFEXITED(res))
    {
        res = WEXITSTATUS(res);
    }

    TAF_ERROR_IF_RET_VAL(res != 0, LE_FAULT, "pclose errno(%d), result(%d).", errno, res);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy arritribute from source file or directory to destination.
 *
 * @return
 * - LE_OK if file arritributes are copied successfully.
 * - LE_FAULT if an error occurred while copying file arritributes.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::CopyAttr
(
    const char* srcPath, ///< [IN] Source path.
    const char* dstPath  ///< [IN] Destination path.
)
{
    char attrList[TAF_APPMGMT_MAX_XATTR_LIST_SIZE] = "";
    ssize_t listSize = listxattr(srcPath, attrList, sizeof(attrList));
    if (listSize < 0)
    {
        LE_ERROR("Fail to get list of attributes for %s.  %m.", srcPath);
        return LE_FAULT;
    }

    char* attrPtr = attrList;
    while (listSize > 0)
    {
        char value[TAF_APPMGMT_MAX_XATTR_VALUE_SIZE] = "";
        ssize_t valueSize = getxattr(srcPath, attrPtr, value, sizeof(value));
        if (valueSize == -1)
        {
            LE_ERROR("Could not get value for attribute %s for %s.", attrPtr, srcPath);
            return LE_FAULT;
        }

        if (setxattr(dstPath, attrPtr, value, valueSize, 0) == -1)
        {
            LE_ERROR("Could not set attribute %s for %s.", attrPtr, dstPath);
            return LE_FAULT;
        }

        ssize_t attrLen = strlen(attrPtr) + 1;
        listSize -= attrLen;
        attrPtr += attrLen;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy directory from source path to destination path.
 *
 * @return
 * - LE_OK if directory is copied successfully.
 * - LE_FAULT if an error occurred while copying directory.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::CopyDir
(
    const char* srcDir, ///< [IN] Source directory path.
    const char* dstDir  ///< [IN] Destination directory path.
)
{
    struct stat status;
    if (stat(srcDir, &status) != 0)
    {
        LE_ERROR("Fail to get status of directory %s.", srcDir);
        return LE_FAULT;
    }

    if (le_dir_MakePath(dstDir, status.st_mode) != LE_OK)
    {
        LE_ERROR("Fail to create directory %s.", dstDir);
        return LE_FAULT;
    }

    if (chmod(dstDir, status.st_mode) != 0)
    {
        LE_ERROR("Fail to change directory permission %s.", dstDir);
        return LE_FAULT;
    }

    if (chown(dstDir, 0, 0) < 0)
    {
        LE_ERROR("Fail to set owner and group of %s.", dstDir);
        return LE_FAULT;
    }

    if (stat(dstDir, &status) != 0)
    {
        LE_ERROR("Fail to get status of directory %s.", dstDir);
        return LE_FAULT;
    }

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    if (tafAppMgmt.CopyAttr(srcDir, dstDir) != LE_OK)
    {
        LE_ERROR("Fail to copy arritribute from %s to %s.", srcDir, dstDir);
        return LE_FAULT;
    }

    if (stat(dstDir, &status) != 0)
    {
        LE_ERROR("Fail to get status of directory %s.", dstDir);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy file from source path to destination path.
 *
 * @return
 * - LE_OK if file is copied successfully.
 * - LE_FAULT if an error occurred while copying file.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::CopyFile
(
    const char* srcFile, ///< [IN] Source file.
    const char* dstFile  ///< [IN] Destination file.
)
{
    struct stat status;
    le_result_t result = LE_OK;
    ssize_t wrSize = 0;
    off_t offset = 0;
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    if (stat(srcFile, &status) != 0)
    {
        LE_ERROR("Fail to get status of file %s.", srcFile);
        return LE_FAULT;
    }

    int rdFd = open(srcFile, O_RDONLY);
    if (rdFd < 0)
    {
        LE_ERROR("Fail to open file %s for read.", srcFile);
        return LE_FAULT;
    }

    int wrFd = creat(dstFile, status.st_mode);
    if (wrFd < 0)
    {
        LE_ERROR("Fail to open file %s for write.", dstFile);
        close(rdFd);
        return LE_FAULT;
    }

    if (fchmod(wrFd, status.st_mode) != 0)
    {
        LE_ERROR("Fail to change file permission %s.", dstFile);
        result = LE_FAULT;
        goto cleanup;
    }

    if (chown(dstFile, 0, 0) < 0)
    {
        LE_ERROR("Fail to set owner and group of %s.", dstFile);
        result = LE_FAULT;
        goto cleanup;
    }

    if (tafAppMgmt.CopyAttr(srcFile, dstFile) != LE_OK)
    {
        LE_ERROR("Fail to copy arritribute from %s to %s.", srcFile, dstFile);
        result = LE_FAULT;
        goto cleanup;
    }

    while (wrSize < status.st_size)
    {
        ssize_t size = sendfile(wrFd, rdFd, &offset, status.st_size - wrSize);

        if (size == -1)
        {
            LE_ERROR("Error when copying file %s to %s.", srcFile, dstFile);
            result = LE_FAULT;
            goto cleanup;
        }

        wrSize += size;
    }

    if (stat(dstFile, &status) != 0)
    {
        LE_ERROR("Fail to get status of file %s.", dstFile);
        return LE_FAULT;
    }

cleanup:
    close(rdFd);
    close(wrFd);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy realpath of symlink to destination path..
 *
 * @return
 * - LE_OK if realpath of symlink is copied successfully.
 * - LE_FAULT if an error occurred while copying realpath of symlink.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::CopySymlink
(
    const char* srcLink, ///< [IN] Source symlink.
    const char* dstPath  ///< [IN] Destination path.
)
{
    char link[PATH_MAX] = "";
    ssize_t bytes = readlink(srcLink, link, sizeof(link));
    if ((bytes < 0) || ((size_t)bytes >= sizeof(link)))
    {
        LE_ERROR("Failed to read symlink %s: bytes %." PRIdS, srcLink, bytes);
        return LE_FAULT;
    }

    link[bytes] = '\0';

    char linkPath[PATH_MAX] = "";
    if (realpath(link, linkPath) == NULL)
    {
        LE_ERROR("No real path for symlink: %s.", link);
    }

    if (strlen(link) <= 0)
    {
        LE_ERROR("Invalid link path.");
        return LE_FAULT;
    }
    else
    {
        auto &tafAppMgmt = taf_AppMgmt::GetInstance();
        struct stat status;
        if (stat(link, &status) != 0)
        {
            LE_ERROR("Fail to get status of file %s.", link);
            return LE_FAULT;
        }

        if (S_ISREG(status.st_mode))
        {
            if (tafAppMgmt.CopyFile(link, dstPath) != LE_OK)
            {
                LE_ERROR("Failed to copy symlink %s to %s.", link, dstPath);
                return LE_FAULT;
            }
        }
        else if (S_ISDIR(status.st_mode))
        {
            if (tafAppMgmt.CopyDir(link, dstPath) != LE_OK)
            {
                LE_ERROR("Failed to copy symlink %s to %s.", link, dstPath);
                return LE_FAULT;
            }
        }
        else
        {
            LE_ERROR("Symlink %s is not a file or directory.", link);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy files, directories and symlink recursively.
 *
 * @return
 * - LE_OK if files, directories and symlink are copied successfully
 * - LE_FAULT if an error occurred while copying files, directories or symlink.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::RecursiveCopy
(
    const char* srcDir, ///< [IN] Source directory path.
    const char* dstDir  ///< [IN] Destination directory path.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    if (tafAppMgmt.CopyDir(srcDir, dstDir) != LE_OK)
    {
        LE_ERROR("Fail to create parent path %s.", dstDir);
        return LE_FAULT;
    }

    size_t srcLen = strlen(srcDir);
    char* pathArrayPtr[] = {(char*)srcDir, NULL};
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        char path[PATH_MAX] = "";

        if (le_path_Concat("/", path, sizeof(path), dstDir, entPtr->fts_path + srcLen, NULL) !=
            LE_OK)
        {
            LE_ERROR("Fail to create path %s.", dstDir);
            fts_close(ftsPtr);
            return LE_FAULT;
        }

        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (entPtr->fts_level > 0)
                {
                    if (tafAppMgmt.CopyDir(entPtr->fts_path, path) != LE_OK)
                    {
                        LE_ERROR("Fail to copy directory from %s to %s.", entPtr->fts_path, path);
                        fts_close(ftsPtr);
                        return LE_FAULT;
                    }
                }
                break;
            case FTS_DP:
            case FTS_DEFAULT:
                break;
            case FTS_F:
                if (tafAppMgmt.CopyFile(entPtr->fts_path, path) != LE_OK)
                {
                    LE_ERROR("Fail to copy file from %s to %s.", entPtr->fts_path, path);
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
                break;
            case FTS_SL:
            case FTS_SLNONE:
                if (tafAppMgmt.CopySymlink(entPtr->fts_path, path) != LE_OK)
                {
                    LE_ERROR("Fail to copy symlink from %s to file %s.", entPtr->fts_path, path);
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
                break;
            case FTS_DC:
            case FTS_DNR:
            case FTS_NS:
            case FTS_ERR:
            default:
                LE_ERROR("Unexpected file type, %d, on file %s.", entPtr->fts_info,
                    entPtr->fts_path);
                fts_close(ftsPtr);
                return LE_FAULT;
        }
    }

    fts_close(ftsPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Backup app.
 *
 * @return
 * - LE_OK if successful backup for app.
 * - LE_FAULT if an error occurred while backing up an app.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_AppMgmt::BackupApp
(
    const char* appName ///< [IN] App name.
)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    char hash[TAF_APPMGMT_APP_HASH_BYTES] = {0};
    le_appInfo_GetHash(appName, hash, TAF_APPMGMT_APP_HASH_BYTES);

    char appDir[TAF_APPMGMT_TELAF_BACKUP_PATH_MAX] = {0};
    char backupDir[TAF_APPMGMT_TELAF_BACKUP_PATH_MAX] = {0};
    snprintf(appDir, sizeof(appDir), TAF_APPMGMT_TELAF_APPS_NODE, hash);
    snprintf(backupDir, sizeof(backupDir), TAF_APPMGMT_APP_BACKUP_NODE, hash);

    LE_INFO("Backup app directory %s.", appDir);

    if (tafAppMgmt.RecursiveCopy(appDir, backupDir) != LE_OK)
    {
        LE_ERROR("Fail to backup app directory %s.", appDir);
        return LE_FAULT;
    }

    LE_INFO("Generating %s app bundles...", appName);

    char backupBundle[TAF_APPMGMT_TELAF_BACKUP_PATH_MAX] = {0};
    snprintf(backupBundle, sizeof(backupBundle), TAF_APPMGMT_APP_BACKUP_PATH, appName);

    // 1. Create tarball.
    char cmd[PATH_MAX] = "";
    snprintf(cmd, sizeof(cmd), "tar -cjf %s -C %s .", backupBundle, backupDir);
    if (tafAppMgmt.SendPipeCmd(cmd) != LE_OK)
    {
        LE_ERROR("Fail to generate app bundle %s.", backupBundle);
        return LE_FAULT;
    }

    // 2. Get tarball size.
    ifstream infile(backupBundle);
    infile.seekg(0, ios::end);
    long size = infile.tellg();
    infile.seekg(0);

    // 3. Insert header.
    stringstream sstream;
    sstream << "{\n";
    sstream << "\"command\":\"updateApp\",\n";
    string appStr(appName);
    sstream << "\"name\":\"" << appStr << "\",\n";
    char version[TAF_APPMGMT_APP_VERSION_BYTES] = {0};
    if (tafAppMgmt.GetAppVersion(appName, version, TAF_APPMGMT_APP_VERSION_BYTES) == LE_OK)
    {
        string verStr(version);
        sstream << "\"version\":\"" << verStr << "\",\n";
    }
    else
    {
        sstream << "\"version\":\"\",\n";
    }
    string md5Str(hash);
    sstream << "\"md5\":\"" << md5Str << "\",\n";
    sstream << "\"size\":" << size << "\n";
    sstream << "}";
    sstream << infile.rdbuf();

    fstream outfile(backupBundle, ios::out);
    outfile << sstream.rdbuf();

    // 4. Close file streams.
    infile.close();
    outfile.close();

    LE_INFO("Cleaning up temparory directory %s.", backupDir);
    if (le_dir_RemoveRecursive(backupDir) != LE_OK)
    {
        LE_ERROR("Fail to remove backup directory : %s.", backupDir);
        return LE_FAULT;
    }

    LE_INFO("%s generated.", backupBundle);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * App update handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::AppUpdateHandler
(
    void* reqPtr ///< [IN] Update request.
)
{
    taf_AppMgmtUpdateReq_t* updateReq = (taf_AppMgmtUpdateReq_t*)reqPtr;
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    switch (tafAppMgmt.state)
    {
        case TAF_UPDATE_IDLE:
            le_utf8_Copy(tafAppMgmt.appName, updateReq->name, TAF_APPMGMT_APP_NAME_BYTES, NULL);
            if (updateReq->event == TAF_APPMGMT_EV_INSTALL)
            {
                tafAppMgmt.state = TAF_UPDATE_INSTALLING;
                LE_INFO("Installing app %s.", updateReq->name);

                if (tafAppMgmt.IsSysApp(tafAppMgmt.appName))
                {
                    LE_ERROR("%s can not be updated.", tafAppMgmt.appName);
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, 0,
                        TAF_UPDATE_BAD_PACKAGE);
                    return;
                }

                char path[PATH_MAX] = {0};
                snprintf(path, sizeof(path), TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH, updateReq->name);

                if (!tafAppMgmt.IsValidToInstall(tafAppMgmt.appName, path))
                {
                    LE_ERROR("%s is invalid for installation.", tafAppMgmt.appName);
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, 0,
                        TAF_UPDATE_BAD_PACKAGE);
                    return;
                }

                if (tafAppMgmt.IsActivated(tafAppMgmt.appName) &&
                    tafAppMgmt.BackupApp(tafAppMgmt.appName) != LE_OK)
                {
                    LE_ERROR("Fail to backup %s before installation.", tafAppMgmt.appName);
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, 0,
                        TAF_UPDATE_INTERNAL_ERROR);
                    return;
                }

                int fd = open(path, O_RDONLY);
                if (fd < 0)
                {
                    LE_ERROR("Invalid file %s.", path);
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, 0,
                        TAF_UPDATE_PACKAGE_NOT_FOUND);
                    return;
                }

                le_result_t result = le_update_Start(fd);
                if (result != LE_OK) 
                {
                    LE_ERROR("Fail to install app %s.", updateReq->name);
                    le_update_End();
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_INSTALL_FAIL, 0, TAF_UPDATE_BAD_PACKAGE);
                }

                if (fd > 0)
                    close(fd);
            }
            else if (updateReq->event == TAF_APPMGMT_EV_PROBATION)
            {
                tafAppMgmt.state = TAF_UPDATE_PROBATION;
                LE_INFO("Starting app %s probation.", updateReq->name);

                le_appCtrl_Start(updateReq->name);
                LE_INFO("Starting probation timer...");
                le_timer_SetRepeat(tafAppMgmt.prbtTimerRef, tafAppMgmt.prbtTime);
                le_timer_Start(tafAppMgmt.prbtTimerRef);
            }
            else
            {
                LE_ERROR("Invalid operation (%d) for idle state.", updateReq->event);
            }
            break;
        case TAF_UPDATE_INSTALLING:
            LE_ERROR("Invalid operation for installing state.");
            break;
        case TAF_UPDATE_PROBATION:
            LE_ERROR("Invalid operation for probation state.");
            break;
        case TAF_UPDATE_PROBATION_FAIL:
            if (updateReq->event == TAF_APPMGMT_EV_ROLLBACK)
            {
                tafAppMgmt.state = TAF_UPDATE_ROLLBACK;
                LE_INFO("App %s in rollback.", tafAppMgmt.appName);

                char path[PATH_MAX] = {0};
                snprintf(path, sizeof(path), TAF_APPMGMT_APP_BACKUP_PATH, tafAppMgmt.appName);

                int fd = open(path, O_RDONLY);
                if (fd < 0)
                {
                    LE_ERROR("App backup bundle invalid %s.", path);
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL, 0,
                        TAF_UPDATE_PACKAGE_NOT_FOUND);
                    return;
                }

                le_result_t result = le_update_Start(fd);
                if (result != LE_OK) 
                {
                    LE_ERROR("Fail to rollback app %s.", updateReq->name);
                    le_update_End();
                    tafAppMgmt.UpdateProgress(TAF_UPDATE_ROLLBACK_FAIL, 0, TAF_UPDATE_BAD_PACKAGE);
                }

                if (fd > 0)
                    close(fd);
            }
            break;
        case TAF_UPDATE_ROLLBACK:
            LE_ERROR("Invalid operation for rollback state.");
            break;
        default:
            LE_ERROR("Invalid state (%d).", tafAppMgmt.state);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for app update.
 */
//--------------------------------------------------------------------------------------------------
void* taf_AppMgmt::AppUpdateThread
(
    void* contextPtr ///< [IN] Context.
)
{
    // Read and write config tree.
    le_cfg_ConnectService();

    // App control and information.
    le_appInfo_ConnectService();
    le_appCtrl_ConnectService();
    le_appRemove_ConnectService();

    // Register update handler.
    le_update_ConnectService();

    le_update_AddProgressHandler(InstallHandler, NULL);
    le_event_AddHandler("appUpdateHandler", appUpdateEvId, AppUpdateHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_AppMgmt::Init
(
    void
)
{
    chrono::time_point<chrono::system_clock> startTime = chrono::system_clock::now();

    // 1. Get configurations from json file.
    ptree root;
    read_json("tafUpdate.json", root);
    prbtTime = root.get<uint32_t>("app.probation.time");

    // 2. Connect to core services.
    le_cfg_ConnectService();
    le_appInfo_ConnectService();
    le_appCtrl_ConnectService();
    le_appRemove_ConnectService();

    // 3. Initiate the memory pool
    appListPool = le_mem_InitStaticPool(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM,
        sizeof(taf_AppMgmtAppList_t));
    appInfoPool = le_mem_InitStaticPool(appInfoPool, TAF_APPMGMT_APP_MAX_NUM,
        sizeof(taf_appMgmt_AppInfo_t));
    appInfoSafeRefPool = le_mem_InitStaticPool(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM,
        sizeof(taf_AppMgmtAppInfoSafeRef_t));

    // 4. Initiate the reference map.
    appListRefMap = le_ref_InitStaticMap(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
    appInfoSafeRefMap = le_ref_InitStaticMap(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);

    // 5. Create events.
    appUpdateEvId = le_event_CreateId("appUpdateEvId", sizeof(taf_AppMgmtUpdateReq_t));
    jsonParseEvId = le_event_CreateId("jsonParseEvId", sizeof(int));

    // 6. Create thread for app update.
    le_sem_Ref_t semaphore = le_sem_Create("appUpdateThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("appUpdateThread", AppUpdateThread,
        (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 7. Create thread for json parser.
    semaphore = le_sem_Create("jsonParseThreadSem", 0);
    threadRef = le_thread_Create("jsonParseThread", JsonParseThread, (void*)semaphore);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 8. Create probation timer.
    prbtTimerRef = le_timer_Create("App Probation Timer");
    le_timer_SetMsInterval(prbtTimerRef, 1000);
    le_timer_SetHandler(prbtTimerRef, ProbationTimerHandler);

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafAppMgmt component: %lfs.", elapsedTime.count());
}
