/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdConnECall.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace tafsvc;

void tafMngdConnECall::Init(void)
{
     LE_INFO("tafMngdConnECall: init");
}

tafMngdConnECall &tafMngdConnECall::GetInstance()
{
    static tafMngdConnECall instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if eCall is in progress
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnECall::IsECallInProgress()
{
    LE_DEBUG("tafMngdConnECall: IsECallInProgress");
    bool isInProgress = false;
#ifndef LE_CONFIG_TARGET_SIMULATION
    le_result_t result = LE_OK;
    taf_ecall_CallRef_t eCallRef = NULL;
    eCallRef = taf_ecall_Create();
    if (eCallRef) {
        result = taf_ecall_IsInProgress(eCallRef, &isInProgress);
        if ( result != LE_OK )
        {
            LE_ERROR ("Error in checking if ecall is in progress");
            return false;
        }
        taf_ecall_Delete(eCallRef);
    }
    else{
        LE_ERROR ("Error in creating the reference for ecall");
        return false;
    }
#endif
    return isInProgress;
}