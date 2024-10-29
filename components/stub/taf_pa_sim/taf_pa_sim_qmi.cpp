/*=============================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/

#include "legato.h"
#include "interfaces.h"
#include "taf_pa_sim.hpp"

//--------------------------------------------------------------------------------------------------
/**
 *  UIM refresh register.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_sim_RefreshRegister
(
    taf_pa_sim_SessionType_t sessionType,
    uint32_t filesLen,
    taf_pa_sim_RefreshFile_t* files
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  UIM refresh ok.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_sim_RefreshOk
(
    taf_pa_sim_SessionType_t sessionType,
    bool refreshAllow
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  UIM refresh complete.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_sim_RefreshComplete
(
    taf_pa_sim_SessionType_t sessionType
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for SIM refresh
 */
//--------------------------------------------------------------------------------------------------
taf_pa_sim_RefreshChangeHandlerRef_t taf_pa_sim_AddRefreshChangeHandler
(
    taf_pa_sim_RefreshChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for SIM refresh
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_sim_RemoveRefreshChangeHandler
(
    taf_pa_sim_RefreshChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_pa_radio stub");
}
