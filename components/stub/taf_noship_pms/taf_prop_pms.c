/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "taf_prop_pms.h"

le_result_t taf_prop_pms_Init
(
    taf_prop_pms_MpssRef_t * mpssRefPtr,
    taf_prop_pms_ErrCallback errCbFn,
    void * errCbCtx
)
{
    LE_UNUSED(mpssRefPtr);
    LE_UNUSED(errCbFn);
    LE_UNUSED(errCbCtx);
    return LE_NOT_IMPLEMENTED;
}

le_result_t taf_prop_pms_Deinit
(
    taf_prop_pms_MpssRef_t * mpssRefPtr
)
{
    LE_UNUSED(mpssRefPtr);
    return LE_NOT_IMPLEMENTED;
}

le_result_t taf_prop_pms_SetWsFilter
(
    taf_prop_pms_MpssRef_t mpssRef,
    taf_prop_pms_ModemWakeupSource_t bitset
)
{
    LE_UNUSED(mpssRef);
    LE_UNUSED(bitset);
    return LE_NOT_IMPLEMENTED;
}

le_result_t taf_prop_pms_GetWsFilter
(
    taf_prop_pms_MpssRef_t mpssRef,
    taf_prop_pms_ModemWakeupSource_t *bitset
)
{
    LE_UNUSED(mpssRef);
    LE_UNUSED(bitset);
    return LE_NOT_IMPLEMENTED;
}

le_result_t taf_prop_pms_EnableAllWs
(
    taf_prop_pms_MpssRef_t mpssRef
)
{
    LE_UNUSED(mpssRef);
    return LE_NOT_IMPLEMENTED;
}

COMPONENT_INIT
{
    LE_INFO("Stub for taf_prop_pms");
}
