/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdConnECall.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace telux::tafsvc;


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