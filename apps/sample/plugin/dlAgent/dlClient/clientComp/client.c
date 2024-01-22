/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Default download configuration file path.
*/
//--------------------------------------------------------------------------------------------------
static const char defaultCfgPath[] = "/legato/systems/current/appsWriteable/dlClient/default.conf";

//--------------------------------------------------------------------------------------------------
/**
 * Download handler reference.
 */
//--------------------------------------------------------------------------------------------------
taf_update_StateHandlerRef_t handlerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Download session reference.
 */
//--------------------------------------------------------------------------------------------------
taf_update_SessionRef_t sessRef;

//--------------------------------------------------------------------------------------------------
/**
 * Download handler.
 */
//--------------------------------------------------------------------------------------------------
void DownloadHandler
(
    const taf_update_StateInd_t* indication, ///< [IN] Download indication.
    void* context                            ///< [IN] Handler context.
)
{
    switch (indication->state)
   {
        case TAF_UPDATE_DOWNLOAD_FAIL:
            LE_ERROR("Download failure");
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("Downloading %d%%", indication->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            LE_INFO("Download success");
            break;
        case TAF_UPDATE_DOWNLOAD_PAUSED:
            LE_INFO("Download paused");
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure download session.
 */
//--------------------------------------------------------------------------------------------------
void ctrlDl_ConfigSession
(
    const char* file ///< [IN] Download configuration file.
)
{
    le_result_t result = taf_update_GetDownloadSession(file, &sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to configure download session");
    }
    else
    {
        LE_INFO("Configured download session with %s", file);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start download.
 */
//--------------------------------------------------------------------------------------------------
void ctrlDl_StartDownload()
{
    le_result_t result = taf_update_StartDownload(sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to start download");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause download.
 */
//--------------------------------------------------------------------------------------------------
void ctrlDl_PauseDownload()
{
    le_result_t result = taf_update_PauseDownload(sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to pause download");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 */
//--------------------------------------------------------------------------------------------------
void ctrlDl_ResumeDownload()
{
    le_result_t result = taf_update_ResumeDownload(sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to resume download");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel download.
 */
//--------------------------------------------------------------------------------------------------
void ctrlDl_CancelDownload()
{
    le_result_t result = taf_update_CancelDownload(sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to cancel download");
    }
}

COMPONENT_INIT
{
    LE_INFO("Download Client started.");

    le_result_t result = taf_update_GetDownloadSession(defaultCfgPath, &sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to configure download session");
    }
    else
    {
        LE_INFO("Configured download session with %s", defaultCfgPath);
    }

    handlerRef = taf_update_AddStateHandler(DownloadHandler, NULL);
}