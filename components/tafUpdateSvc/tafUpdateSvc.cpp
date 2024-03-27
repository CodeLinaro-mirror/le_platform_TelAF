/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"
#include "tafAppMgmt.hpp"

using namespace telux::tafsvc;

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Update service component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_INFO("tafUpdate Service Init...\n");
    LE_INFO("tafUpdate Component Init...\n");
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.Init();
    LE_INFO("tafUpdate Component Ready...\n");
    LE_INFO("tafAppMgmt Component Init...\n");
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    tafAppMgmt.Init();
    LE_INFO("tafAppMgmt Component Ready...\n");
    LE_INFO("tafFwUpdate Component Init...\n");
    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
    tafFwUpdate.Init();
    LE_INFO("tafFwUpdate Component Ready...\n");
    LE_INFO("tafUpdate Service Ready...\n");
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
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        tafUpdate.dlSessRef);
    TAF_ERROR_IF_RET_NIL(sessPtr == NULL, "Fail to look up download session.");

    TAF_ERROR_IF_RET_NIL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD,
        "Current session is not download session.");

    taf_UpdateDlReq_t dlReq;
    dlReq.event = TAF_UPDATE_DL_START;
    dlReq.sessPtr = &sessPtr->dlSess;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));
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
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up download session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD, LE_FAULT,
        "Current session is not download session.");

    taf_UpdateDlReq_t dlReq;
    dlReq.event = TAF_UPDATE_DL_START;
    dlReq.sessPtr = &sessPtr->dlSess;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause download.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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

    TAF_ERROR_IF_RET_VAL(tafUpdate.daInfPtr->startDownload == NULL, LE_UNSUPPORTED,
        "DA plug-in pause download function is not supported.");
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up download session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD, LE_FAULT,
        "Current session is not download session.");

    taf_UpdateDlReq_t dlReq;
    dlReq.event = TAF_UPDATE_DL_PAUSE;
    dlReq.sessPtr = &sessPtr->dlSess;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up download session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD, LE_FAULT,
        "Current session is not download session.");

    taf_UpdateDlReq_t dlReq;
    dlReq.event = TAF_UPDATE_DL_RESUME;
    dlReq.sessPtr = &sessPtr->dlSess;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel download.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up download session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD, LE_FAULT,
        "Current session is not download session.");

    taf_UpdateDlReq_t dlReq;
    dlReq.event = TAF_UPDATE_DL_CANCEL;
    dlReq.sessPtr = &sessPtr->dlSess;
    le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));

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
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

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
 * Installs an OTA package on the target device.
 *
 * @note QOTA header should be removed before calling this API.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Install
(
    taf_update_OTA_t ota, ///< [IN] OTA workflow
    const char* pkgPath   ///< [IN] Package path.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_FwUpdateReq_t fwReq;
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
    }

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            le_utf8_Copy(sessPtr->fwSess.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
            fwReq.event = TAF_FWUPDATE_EV_INSTALL;
            le_utf8_Copy(fwReq.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
            le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));
            break;
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
 * @return
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
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

    taf_FwUpdateReq_t fwReq;
    taf_AppMgmtUpdateReq_t appReq;
    taf_UpdateReq_t updateReq;

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    switch (sessPtr->sessType)
    {
        case TAF_UPDATE_SESSION_TYPE_FW_UPDATE:
            le_utf8_Copy(sessPtr->fwSess.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
            fwReq.event = TAF_FWUPDATE_EV_INSTALL;
            le_utf8_Copy(fwReq.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
            le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));
            break;
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

            updateReq.event = TAF_UPDATE_INST_START;
            updateReq.sessPtr = &sessPtr->upiSess;
            le_utf8_Copy(updateReq.filePath, pkgPath, TAF_UPDATE_FILE_PATH_LEN, NULL);
            le_event_Report(tafUpdate.updatePiEvId, &updateReq, sizeof(taf_UpdateReq_t));
            break;
        default:
            LE_ERROR("Unsupported session type (%d) for installation.", sessPtr->sessType);
            return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Installation post check.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_InstallPostCheck
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for post-check.", sessPtr->sessType);

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (tafFwUpdate.InstallPostCheck(sessPtr->fwSess.filePath) != LE_OK)
    {
        LE_ERROR("Post-check failure on fimware installation.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get avtive bank.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
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
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

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
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_VerifyActivation
(
    taf_update_SessionRef_t sessionRef, ///< [IN] Installation session reference.
    const char* manifest                ///< [IN] Manifest for activation.
)
{
    auto &tafUpdate = taf_Update::GetInstance();
            
    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for activation verification.", sessPtr->sessType);

    taf_FwUpdateReq_t fwReq;
    fwReq.event = TAF_FWUPDATE_EV_VERIFY_ACTIVATION;
    le_utf8_Copy(fwReq.filePath, manifest, TAF_UPDATE_FILE_PATH_LEN, NULL);
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rollback to previous configurations to keep pesistency.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Rollback
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for rollback.", sessPtr->sessType);

    taf_FwUpdateReq_t fwReq;
    fwReq.event = TAF_FWUPDATE_EV_ROLLBACK;
    le_utf8_Copy(fwReq.filePath, sessPtr->fwSess.filePath, TAF_UPDATE_FILE_PATH_LEN, NULL);
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Bank synchronization.
 *
 * @return
 *  - LE_FAULT       On failure.
 *  - LE_OK          On success.
 *  - LE_UNSUPPORTED Unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Sync
(
    taf_update_SessionRef_t sessionRef ///< [IN] Installation session reference.
)
{
    auto &tafUpdate = taf_Update::GetInstance();

    taf_UpdateSession_t* sessPtr = (taf_UpdateSession_t*)le_ref_Lookup(tafUpdate.sessionMap,
        sessionRef);
    TAF_ERROR_IF_RET_VAL(sessPtr == NULL, LE_FAULT, "Fail to look up installtion session.");

    TAF_ERROR_IF_RET_VAL(sessPtr->sessType != TAF_UPDATE_SESSION_TYPE_FW_UPDATE, LE_UNSUPPORTED,
        "Unsupported session type (%d) for bank synchronization.", sessPtr->sessType);

    taf_FwUpdateReq_t fwReq;
    fwReq.event = TAF_FWUPDATE_EV_SYNC;
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwReq, sizeof(taf_FwUpdateReq_t));

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
