/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafUpdate.hpp"
#include "tafFlash.hpp"
#include "tafFwUpdate.hpp"
#include "tafAppMgmt.hpp"

using namespace tafsvc;

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Update service component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_INFO("tafUpdate Service Init...\n");
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.Init();
    LE_INFO("tafUpdate Component Ready...\n");
    auto &tafFlashAccess = taf_FlashAccess::GetInstance();
    tafFlashAccess.Init();
    LE_INFO("tafFlashAccess Component Ready...\n");
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    tafAppMgmt.Init();
    LE_INFO("tafAppMgmt Component Ready...\n");
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.Init();
    LE_INFO("tafFwUpdate Component Ready...\n");
    LE_INFO("tafUpdate Service Ready...\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Post a request to firmware update thread.
 */
//--------------------------------------------------------------------------------------------------
static void PostFwRequest
(
    taf_FwUpdateEvent_t event, ///< [IN] Firmware update event.
    const char* filePath       ///< [IN] File path, can be NULL.
)
{
    taf_FwUpdateReq_t fwReq;
    memset(&fwReq, 0, sizeof(fwReq));
    fwReq.event = event;
    if (filePath != NULL)
    {
        le_utf8_Copy(fwReq.filePath, filePath, TAF_UPDATE_FILE_PATH_LEN, NULL);
    }
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Post a request to download plug-in handler.
 */
//--------------------------------------------------------------------------------------------------
static void PostDownloadRequest
(
    taf_UpdateDlEvent_t event,           ///< [IN] Download event.
    taf_UpdateDownloadSession_t* sessPtr ///< [IN] Download session.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateDlReq_t dlReq;
    memset(&dlReq, 0, sizeof(dlReq));
    dlReq.event = event;
    dlReq.sessPtr = sessPtr;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Post a request to update plug-in handler.
 */
//--------------------------------------------------------------------------------------------------
static void PostUpdateRequest
(
    taf_UpdateEvent_t event,            ///< [IN] Update event.
    taf_UpdatePlugInSession_t* sessPtr, ///< [IN] Update plug-in session.
    const char* filePath                ///< [IN] File path, can be NULL.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateReq_t updateReq;
    memset(&updateReq, 0, sizeof(updateReq));
    updateReq.event = event;
    updateReq.sessPtr = sessPtr;
    if (filePath != NULL)
    {
        le_utf8_Copy(updateReq.filePath, filePath, TAF_UPDATE_FILE_PATH_LEN, NULL);
    }
    le_event_Report(tafUpdate.updatePiEvId, &updateReq, sizeof(taf_UpdateReq_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Look up a download session.
 *
 * @return
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid session reference or not a download session.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LookupDownloadSession
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Download session reference.
    taf_UpdateSession_t** sessPtrPtr    ///< [OUT] Download session.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up download session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD,
        LE_BAD_PARAMETER, "Current session is not download session.");

    *sessPtrPtr = sessPtr;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Downloads an OTA package from a cloud server.
 *
 * @note Update service will parse the download package and remove the QOTA header once the download
         is complete.
 */
//--------------------------------------------------------------------------------------------------
void taf_update_Download()
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_NIL(tafUpdate.daInfPtr == NULL,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_NIL(tafUpdate.daInfPtr->startDownload == NULL,
        "DA plug-in start download function is not supported.");

    taf_UpdateSession_t* sessPtr = NULL;
    TAF_ERROR_IF_RET_NIL(LookupDownloadSession(tafUpdate.dlSessRef, &sessPtr) != LE_OK,
        "Fail to look up download session.");

    TAF_ERROR_IF_RET_NIL(tafUpdate.dlCtrl.RequestStart(taf_Update::DownloadPhase) != LE_OK,
        "Download is ongoing.");

    PostDownloadRequest(TAF_UPDATE_DL_START, &sessPtr->dlSess);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get download session reference.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetDownloadSession
(
    const char* cfgFile,                ///< [IN] Download configuration file.
    taf_update_SessionRef_t* sessionRef ///< [OUT] Download session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, LE_BAD_PARAMETER, "Session reference is NULL.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr == NULL, LE_UNSUPPORTED,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->getSess == NULL, LE_UNSUPPORTED,
        "Get DA plug-in download session is not supported.");

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        tafUpdate.dlSessRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up download session.");

    int ret = (*(tafUpdate.daInfPtr->getSess))(cfgFile, &sessPtr->dlSess.sessRef);
    TAF_ERROR_IF_RET_VAL(ret != 0, LE_FAULT, "Fail to get DA plug-in download session.");

    *sessionRef = tafUpdate.dlSessRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start download.
 *
 * The result is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Download is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartDownload
(
    taf_update_SessionRef_t sessionRef ///< [IN] Download session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr == NULL, LE_UNSUPPORTED,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->startDownload == NULL, LE_UNSUPPORTED,
        "DA plug-in start download function is not supported.");

    taf_UpdateSession_t* sessPtr = NULL;
    le_result_t result = LookupDownloadSession(sessionRef, &sessPtr);
    if (result != LE_OK)
    {
        return result;
    }

    result = tafUpdate.dlCtrl.RequestStart(taf_Update::DownloadPhase);
    if (result != LE_OK)
    {
        return result;
    }

    PostDownloadRequest(TAF_UPDATE_DL_START, &sessPtr->dlSess);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause download.
 *
 * The result is reported by state handler, a failure of the plug-in is reported with current state
 * and TAF_UPDATE_INVALID_OPERATION.
 *
 * @return
 *  - LE_OK            Request is accepted, or download is already paused.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Download is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseDownload
(
    taf_update_SessionRef_t sessionRef ///< [IN] Download session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr == NULL, LE_UNSUPPORTED,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->pauseDownload == NULL, LE_UNSUPPORTED,
        "DA plug-in pause download function is not supported.");

    taf_UpdateSession_t* sessPtr = NULL;
    le_result_t result = LookupDownloadSession(sessionRef, &sessPtr);
    if (result != LE_OK)
    {
        return result;
    }

    bool post = false;
    result = tafUpdate.dlCtrl.RequestPause(TAF_UPDATE_DOWNLOADING, &post);
    if (result == LE_OK && post)
    {
        PostDownloadRequest(TAF_UPDATE_DL_PAUSE, &sessPtr->dlSess);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 *
 * The result is reported by state handler, a failure of the plug-in is reported with
 * TAF_UPDATE_DOWNLOAD_PAUSED and TAF_UPDATE_INVALID_OPERATION.
 *
 * @return
 *  - LE_OK            Request is accepted, or download is already running.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Download is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeDownload
(
    taf_update_SessionRef_t sessionRef ///< [IN] Download session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr == NULL, LE_UNSUPPORTED,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->resumeDownload == NULL, LE_UNSUPPORTED,
        "DA plug-in resume download function is not supported.");

    taf_UpdateSession_t* sessPtr = NULL;
    le_result_t result = LookupDownloadSession(sessionRef, &sessPtr);
    if (result != LE_OK)
    {
        return result;
    }

    bool post = false;
    result = tafUpdate.dlCtrl.RequestResume(TAF_UPDATE_DOWNLOADING, &post);
    if (result == LE_OK && post)
    {
        PostDownloadRequest(TAF_UPDATE_DL_RESUME, &sessPtr->dlSess);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel download.
 *
 * The result is reported by state handler, a failure of the plug-in is reported with current state
 * and TAF_UPDATE_INVALID_OPERATION.
 *
 * @return
 *  - LE_OK            Request is accepted, or cancel is already pending.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Download is not ongoing.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelDownload
(
    taf_update_SessionRef_t sessionRef ///< [IN] Download session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr == NULL, LE_UNSUPPORTED,
        "Please install DA module to support download.");

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->cancelDownload == NULL, LE_UNSUPPORTED,
        "DA plug-in cancel download function is not supported.");

    taf_UpdateSession_t* sessPtr = NULL;
    le_result_t result = LookupDownloadSession(sessionRef, &sessPtr);
    if (result != LE_OK)
    {
        return result;
    }

    bool post = false;
    result = tafUpdate.dlCtrl.RequestCancel(TAF_UPDATE_DOWNLOADING, &post);
    if (result == LE_OK && post)
    {
        PostDownloadRequest(TAF_UPDATE_DL_CANCEL, &sessPtr->dlSess);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AB Sync.
 *
 * The result is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Another firmware operation is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartSync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Sync session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER,
        "Fail to look up synchronization session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
        {
            le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::SyncPhase);
            if (result != LE_OK)
            {
                return result;
            }

            PostFwRequest(TAF_FWUPDATE_EV_START_SYNC, NULL);
            break;
        }
        default:
            LE_ERROR("Unsupported session type (%d) for synchronization.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pauses AB sync.
 *
 * The sync is paused at the next partition, TAF_UPDATE_SYNC_PAUSED is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted, or sync is already paused.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Sync is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseSync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Sync session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER,
        "Fail to look up synchronization session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
        {
            // Firmware update thread polls the request, nothing to post.
            bool post = false;
            LE_INFO("Pause NAD sync.");
            return tafFwUpdate.ctrl.RequestPause(TAF_UPDATE_SYNCHRONIZING, &post);
        }
        default:
            LE_ERROR("Unsupported session type (%d) for synchronization.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resumes AB Sync.
 *
 * @return
 *  - LE_OK            Request is accepted, or sync is already running.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Sync is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeSync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Sync session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER,
        "Fail to look up synchronization session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
        {
            bool post = false;
            le_result_t result = tafFwUpdate.ctrl.RequestResume(TAF_UPDATE_SYNCHRONIZING, &post);
            if (result == LE_OK && post)
            {
                PostFwRequest(TAF_FWUPDATE_EV_RESUME_SYNC, NULL);
            }
            return result;
        }
        default:
            LE_ERROR("Unsupported session type (%d) for synchronization.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels AB Sync.
 *
 * TAF_UPDATE_CANCELLED is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted, or cancel is already pending.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Sync is not ongoing.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelSync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Sync session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER,
        "Fail to look up synchronization session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
        {
            bool post = false;
            LE_INFO("Cancel NAD sync.");
            le_result_t result = tafFwUpdate.ctrl.RequestCancel(TAF_UPDATE_SYNCHRONIZING, &post);
            if (result == LE_OK && post)
            {
                PostFwRequest(TAF_FWUPDATE_EV_CANCEL, NULL);
            }
            return result;
        }
        default:
            LE_ERROR("Unsupported session type (%d) for cancelling synchronization.",
                sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get installation session reference.
 *
 * @return
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetInstallationSession
(
    taf_update_PackageType_t pkgType,   ///< [IN] Package type for installation.
    const char* cfgFile,                ///< [IN] Configuration file for installation.
    taf_update_SessionRef_t* sessionRef ///< [OUT] Installation session reference.
)
{
    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, LE_BAD_PARAMETER, "Session reference is NULL.");

    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateSession_t* sessPtr = NULL;
    int ret = 0;

    switch (pkgType)
    {
        case TAF_UPDATE_PACKAGE_TYPE_QOTA_PACKAGE:
            *sessionRef = tafUpdate.qotaSessRef;
            break;
        case TAF_UPDATE_PACKAGE_TYPE_NAD_ZIP:
            *sessionRef = tafUpdate.fwSessRef;
            break;
        case TAF_UPDATE_PACKAGE_TYPE_TELAF_APP:
            *sessionRef = tafUpdate.appSessRef;
            break;
        case TAF_UPDATE_PACKAGE_TYPE_UAPI_PACKAGE:
            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr == NULL, LE_UNSUPPORTED,
                "Please install UA plug-in module.");

            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr->getSess == NULL, LE_UNSUPPORTED,
                "Get UA plug-in update session is not supported.");

            sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap, tafUpdate.upiSessRef);
            TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up update session.");

            ret = (*(tafUpdate.uaInfPtr->getSess))(cfgFile, &sessPtr->upiSess.sessRef);
            TAF_ERROR_IF_RET_VAL(ret != 0, LE_FAULT, "Fail to get UA plug-in update session.");

            *sessionRef = tafUpdate.upiSessRef;
            break;
        default:
            LE_ERROR("Unsupported package type (%d) for installation.", pkgType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks prerequisites for installation.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_InstallPreCheck
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    const char* manifest                ///< [IN] Manifest for pre-check.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    le_result_t result = LE_OK;

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    if (access(manifest, 0))
    {
        LE_INFO("%s not exists, bypass pre-check.", manifest);
        return LE_OK;
    }

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_QOTA_PARSE:
            result = tafUpdate.CheckQotaHeader(manifest);
            TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Invalid QOTA package.");
            result = tafUpdate.RemoveQotaHeader(manifest);
            TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to remove QOTA header.");
            break;
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            result = tafFwUpdate.InstallPreCheck(manifest);
            TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Install pre-check failed.");
            break;
        default:
            LE_ERROR("Unsupported session type (%d) for pre-check.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start firmware installation in firmware update thread.
 *
 * @return
 *  - LE_OK   Request is accepted.
 *  - LE_BUSY Another firmware operation is ongoing or paused.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartFwInstall
(
    taf_UpdateSession_t* sessPtr, ///< [IN] Firmware update session.
    const char* pkgPath           ///< [IN] Package path.
)
{
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::InstallPhase);
    if (result != LE_OK)
    {
        return result;
    }

    le_utf8_Copy(sessPtr->fwSess.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
    PostFwRequest(TAF_FWUPDATE_EV_START_INSTALL, pkgPath);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Installs an OTA package on the target device.
 *
 * @note QOTA header should be removed before calling this API.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 * - LE_BAD_PARAMETER -- Invalid OTA workflow or package path.
 * - LE_BUSY -- Another firmware operation is ongoing or paused.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Install
(
    taf_update_OTA_t ota, ///< [IN] OTA workflow
    const char* pkgPath   ///< [IN] Package path.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_AppMgmtUpdateReq_t appReq;

    taf_update_SessionRef_t sessionRef = NULL;
    if (ota == TAF_UPDATE_FOTA)
    {
        sessionRef = tafUpdate.fwSessRef;
    }
    else if (ota == TAF_UPDATE_SOTA)
    {
        sessionRef = tafUpdate.appSessRef;
    }
    else
    {
        LE_ERROR("Invalid OTA workflow %d.", ota);
        return LE_BAD_PARAMETER;
    }

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            return StartFwInstall(sessPtr, pkgPath);
        case TAF_UPDATE_SESSION_TYPE_APP_UPDATE:
            if (strncmp(pkgPath, TAF_APPMGMT_APP_INSTALL_PATH_PREFIX,
                strlen(TAF_APPMGMT_APP_INSTALL_PATH_PREFIX)) != 0)
            {
                LE_ERROR("Invalid path %s for app installation.", pkgPath);
                return LE_BAD_PARAMETER;
            }
            appReq.event = TAF_APPMGMT_EV_INSTALL;
            le_utf8_Copy(appReq.appName, pkgPath + strlen(TAF_APPMGMT_APP_INSTALL_PATH_PREFIX),
                TAF_UPDATE_FILE_PATH_LEN, NULL);
            le_event_Report(taf_AppMgmt::appUpdateEvId, &appReq, sizeof(taf_AppMgmtUpdateReq_t));
            break;
        default:
            LE_ERROR("Unsupported session type (%d) for installation.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Installs update package.
 *
 * The result is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session or parameters.
 *  - LE_BUSY          Another operation of the session is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartInstall
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    const char* pkgPath                 ///< [IN] Package path.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_AppMgmtUpdateReq_t appReq;
    le_result_t result = LE_OK;

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            return StartFwInstall(sessPtr, pkgPath);
        case TAF_UPDATE_SESSION_TYPE_APP_UPDATE:
            if (strncmp(pkgPath, TAF_APPMGMT_APP_INSTALL_PATH_PREFIX,
                strlen(TAF_APPMGMT_APP_INSTALL_PATH_PREFIX)) != 0)
            {
                LE_ERROR("Invalid path %s for app installation.", pkgPath);
                return LE_BAD_PARAMETER;
            }
            appReq.event = TAF_APPMGMT_EV_INSTALL;
            le_utf8_Copy(appReq.appName, pkgPath + strlen(TAF_APPMGMT_APP_INSTALL_PATH_PREFIX),
                TAF_UPDATE_FILE_PATH_LEN, NULL);
            le_event_Report(taf_AppMgmt::appUpdateEvId, &appReq, sizeof(taf_AppMgmtUpdateReq_t));
            break;
        case TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE:
            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr == NULL, LE_UNSUPPORTED,
                "Please install UA module.");

            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr->startInstall == NULL, LE_UNSUPPORTED,
                "UA plug-in start install function is not supported.");

            result = tafUpdate.upiCtrl.RequestStart(taf_Update::PlugInInstallPhase);
            if (result != LE_OK)
            {
                return result;
            }

            PostUpdateRequest(TAF_UPDATE_INST_START, &sessPtr->upiSess, pkgPath);
            break;
        default:
            LE_ERROR("Unsupported session type (%d) for installation.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pauses installation.
 *
 * Firmware installation is paused at the next checkpoint, TAF_UPDATE_INSTALL_PAUSED is reported by
 * state handler. A pause which can not be performed is reported with current state and
 * TAF_UPDATE_INVALID_OPERATION.
 *
 * @return
 *  - LE_OK            Request is accepted, or installation is already paused.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Installation is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseInstall
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    bool post = false;
    le_result_t result = LE_OK;

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            // Firmware update thread polls the request, nothing to post.
            LE_INFO("Pause NAD update.");
            return tafFwUpdate.ctrl.RequestPause(TAF_UPDATE_INSTALLING, &post);
        case TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE:
            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr == NULL, LE_UNSUPPORTED,
                "Please install UA module.");

            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr->pauseInstall == NULL, LE_UNSUPPORTED,
                "UA plug-in pause install function is not supported.");

            result = tafUpdate.upiCtrl.RequestPause(TAF_UPDATE_INSTALLING, &post);
            if (result == LE_OK && post)
            {
                PostUpdateRequest(TAF_UPDATE_INST_PAUSE, &sessPtr->upiSess, NULL);
            }
            return result;
        default:
            LE_ERROR("Unsupported session type (%d) for installation.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resumes installation.
 *
 * @return
 *  - LE_OK            Request is accepted, or installation is already running.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Installation is not ongoing or cancel is pending.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeInstall
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    bool post = false;
    le_result_t result = LE_OK;

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            result = tafFwUpdate.ctrl.RequestResume(TAF_UPDATE_INSTALLING, &post);
            if (result == LE_OK && post)
            {
                PostFwRequest(TAF_FWUPDATE_EV_RESUME_INSTALL, NULL);
            }
            return result;
        case TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE:
            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr == NULL, LE_UNSUPPORTED,
                "Please install UA module.");

            TAF_ERROR_IF_RET_VAL(tafUpdate.uaInfPtr->resumeInstall == NULL, LE_UNSUPPORTED,
                "UA plug-in resume install function is not supported.");

            result = tafUpdate.upiCtrl.RequestResume(TAF_UPDATE_INSTALLING, &post);
            if (result == LE_OK && post)
            {
                PostUpdateRequest(TAF_UPDATE_INST_RESUME, &sessPtr->upiSess, NULL);
            }
            return result;
        default:
            LE_ERROR("Unsupported session type (%d) for installation.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels installation.
 *
 * An ongoing or paused firmware installation is cancelled at the next checkpoint. If no operation
 * is ongoing, the update which is installed but not activated is cancelled synchronously and the
 * next boot bank is switched back.
 *
 * @return
 *  - LE_OK            Request is accepted, or cancel is already pending.
 *  - LE_FAULT         Fail to cancel the installed update.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Cancel is not permitted in current state, e.g. the device has booted into
 *                     the new bank, rollback must be used instead.
 *  - LE_BUSY          Another firmware operation is ongoing.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelInstall
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
        {
            le_result_t result = LE_OK;
            if (tafFwUpdate.ctrl.IsIdle())
            {
                LE_INFO("Cancel post installation while in the idle state.");

                // Hold the controller, so no operation can be started while cancelling.
                result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::PostInstallCancelPhase);
                if (result != LE_OK)
                {
                    return result;
                }

                result = tafFwUpdate.CancelInstalledUpdate();
                if (result != LE_OK)
                {
                    tafFwUpdate.ctrl.OnFinished();
                }
                return result;
            }

            bool post = false;
            LE_INFO("Cancel NAD update.");
            result = tafFwUpdate.ctrl.RequestCancel(TAF_UPDATE_INSTALLING, &post);
            if (result == LE_OK && post)
            {
                PostFwRequest(TAF_FWUPDATE_EV_CANCEL, NULL);
            }
            return result;
        }
        default:
            LE_ERROR("Unsupported session type (%d) for cancelling installation.",
                sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Installation post check.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Another firmware operation is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_InstallPostCheck
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for post-check.", sessPtr->sessType);

    le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::PostCheckPhase);
    if (result != LE_OK)
    {
        return result;
    }

    PostFwRequest(TAF_FWUPDATE_EV_INSTALL_POST_CHECK, sessPtr->fwSess.filePath);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get avtive bank.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid session or parameters.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetActiveBank
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    taf_update_Bank_t* bankPtr          ///< [OUT] The active bank.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(bankPtr == NULL, LE_BAD_PARAMETER, "bankPtr is NULL.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for getting active bank.", sessPtr->sessType);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (tafFwUpdate.GetActiveBank(bankPtr) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify activation.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Another firmware operation is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_VerifyActivation
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    const char* manifest                ///< [IN] Manifest for activation.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for activation verification.", sessPtr->sessType);

    le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::ActivationPhase);
    if (result != LE_OK)
    {
        return result;
    }

    PostFwRequest(TAF_FWUPDATE_EV_START_ACTIVATION, manifest);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause activation.
 *
 * Activation is paused at the next item, TAF_UPDATE_PROBATION_PAUSED is reported by state handler.
 *
 * @return
 *  - LE_OK            Request is accepted, or activation is already paused.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Activation is not ongoing.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseActivation
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for activation verification.", sessPtr->sessType);

    // Firmware update thread polls the request, nothing to post.
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    bool post = false;
    return tafFwUpdate.ctrl.RequestPause(TAF_UPDATE_PROBATION, &post);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume activation.
 *
 * @return
 *  - LE_OK            Request is accepted, or activation is already running.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_NOT_PERMITTED Activation is not ongoing.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeActivation
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for activation verification.", sessPtr->sessType);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    bool post = false;
    le_result_t result = tafFwUpdate.ctrl.RequestResume(TAF_UPDATE_PROBATION, &post);
    if (result == LE_OK && post)
    {
        PostFwRequest(TAF_FWUPDATE_EV_RESUME_ACTIVATION, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rollback to previous configurations to keep pesistency.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Another firmware operation is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Rollback
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for rollback.", sessPtr->sessType);

    le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::RollbackPhase);
    if (result != LE_OK)
    {
        return result;
    }

    PostFwRequest(TAF_FWUPDATE_EV_ROLLBACK, sessPtr->fwSess.filePath);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Bank synchronization.
 *
 * @return
 *  - LE_OK            Request is accepted.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_BUSY          Another firmware operation is ongoing or paused.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Sync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for bank synchronization.", sessPtr->sessType);

    le_result_t result = tafFwUpdate.ctrl.RequestStart(taf_FwUpdate::BankSyncPhase);
    if (result != LE_OK)
    {
        return result;
    }

    PostFwRequest(TAF_FWUPDATE_EV_SYNC, NULL);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erases a bank.
 *
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid session.
 *  - LE_UNSUPPORTED   Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_EraseBank
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    taf_update_Bank_t bank              ///< [IN] The bank to be erased.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_BAD_PARAMETER, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for getting active bank.", sessPtr->sessType);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    taf_update_Bank_t activeBank = TAF_UPDATE_BANK_UNKNOWN;
    if (tafFwUpdate.GetActiveBank(&activeBank) != LE_OK)
    {
        LE_ERROR("Fail to get active bank.");
        return LE_FAULT;
    }
    TAF_ERROR_IF_RET_VAL(activeBank == bank, LE_UNSUPPORTED, "Can not erase the active bank.");

    if (tafFwUpdate.EraseBank(bank) != LE_OK)
    {
        LE_ERROR("Fail to erase inactive bank.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for update state.
 *
 * @return 
 *  - taf_update_StateHandlerRef_t State handler reference.
 */
//--------------------------------------------------------------------------------------------------
taf_update_StateHandlerRef_t taf_update_AddStateHandler
(
    taf_update_StateHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function.
    void* contextPtr                              ///< [IN] Context.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UpdateStateHandler",
        tafUpdate.stateEvId, taf_Update::StateLayeredHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_update_StateHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for update state.
 */
//--------------------------------------------------------------------------------------------------
void taf_update_RemoveStateHandler
(
    taf_update_StateHandlerRef_t handlerRef ///< [IN] State handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
