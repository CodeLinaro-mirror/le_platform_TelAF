/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Default unpdate configuration file path.
*/
//--------------------------------------------------------------------------------------------------
static const char defaultCfgPath[] =
    "/legato/systems/current/appsWriteable/updateClient/default.conf";

//--------------------------------------------------------------------------------------------------
/**
 * Update handler reference.
 */
//--------------------------------------------------------------------------------------------------
taf_update_StateHandlerRef_t handlerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Update session reference.
 */
//--------------------------------------------------------------------------------------------------
taf_update_SessionRef_t sessRef;

//--------------------------------------------------------------------------------------------------
/**
 * Update handler.
 */
//--------------------------------------------------------------------------------------------------
void UpdateHandler
(
    const taf_update_StateInd_t* indication, ///< [IN] Update indication.
    void* context                            ///< [IN] Handler context.
)
{
    switch (indication->state)
   {
        case TAF_UPDATE_INSTALL_FAIL:
            LE_ERROR("Install failure");
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("Installing %d%%", indication->percent);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("Install success");
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start install.
 */
//--------------------------------------------------------------------------------------------------
void ctrlUpdate_StartInstall
(
    const char* package ///< Update package.
)
{
    le_result_t result = taf_update_StartInstall(sessRef, package);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to start install");
    }
}

COMPONENT_INIT
{
    LE_INFO("Update Client started.");

    le_result_t result = taf_update_GetInstallationSession(TAF_UPDATE_PACKAGE_TYPE_UAPI_PACKAGE,
        defaultCfgPath, &sessRef);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to configure update session");
    }
    else
    {
        LE_INFO("Configured update session with %s", defaultCfgPath);
    }

    handlerRef = taf_update_AddStateHandler(UpdateHandler, NULL);
}