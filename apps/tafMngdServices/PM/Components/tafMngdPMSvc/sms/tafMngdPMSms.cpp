/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdPMSms.hpp"
#include "tafMngdPMCommon.hpp"
#include "tafMngdPMSvc.hpp"

static le_hashmap_Ref_t smsPMMap;

using namespace telux::tafsvc;

void tafMngdPMSms::SmsRxHandler(taf_sms_MsgRef_t msgRef, void* context){
    char text[TAF_SMS_TEXT_BYTES];
    taf_sms_GetText(msgRef, text, sizeof(text));
    LE_INFO("Received SMS %s", text);

    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(smsPMMap);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        taf_MngdPM_Sms_t* smsPtr = (taf_MngdPM_Sms_t*)le_hashmap_GetValue(iteratorRef);

        TAF_ERROR_IF_RET_NIL(smsPtr == nullptr, "Invalid hashmap reference");

        if(strncmp(text, smsPtr->text, sizeof(text)) == 0)
        {
            LE_DEBUG("SMS matched with SMS registetred for %s state",
                    tafMngdPMSvc::tafStateToString(smsPtr->state));
            taf_pm_SetAllVMPowerState((taf_pm_State_t)smsPtr->state);
        }
    }
}

void tafMngdPMSms::RegisterSms(const char* text, taf_mngd_pm_State_t state)
{
    if(smsMapPool == NULL) {
        smsMapPool = le_mem_CreatePool("tafMngdPMSmsMapPool", sizeof(taf_MngdPM_Sms_t));
    }
    taf_MngdPM_Sms_t* smsPMPtr = (taf_MngdPM_Sms_t*)le_mem_ForceAlloc(smsMapPool);
    memset(smsPMPtr, 0, sizeof(taf_MngdPM_Sms_t));
    le_utf8_Copy(smsPMPtr->text, text, TAF_SMS_TEXT_BYTES, NULL);
    smsPMPtr->state = state;

    if(smsPMMap == NULL) {
        smsPMMap =  le_hashmap_Create("tafMngdPMSmsMap", TAF_MNGD_PM_MAX_TRIGGER_REGISTERS,
                le_hashmap_HashString, le_hashmap_EqualsString);
    }
    char stateName[32];
    le_utf8_Copy(stateName, tafMngdPMSvc::tafStateToString(state), 32, NULL);
    LE_INFO("Register SMS for %s state", stateName);
    le_hashmap_Put(smsPMMap, stateName, smsPMPtr);

    if(smsRxHandlerRef == NULL)
    {
        smsRxHandlerRef = taf_sms_AddRxMsgHandler(tafMngdPMSms::SmsRxHandler, NULL);
    }
}

void tafMngdPMSms::DeregisterSms()
{
    LE_DEBUG("DeregisterCanEvents");

    if(smsRxHandlerRef != NULL)
    {
        taf_sms_RemoveRxMsgHandler(smsRxHandlerRef);
    }

    if(!smsPMMap)
        return;

    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(smsPMMap);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        taf_MngdPM_Sms_t *smsPtr = (taf_MngdPM_Sms_t*)le_hashmap_GetValue(iter);
        TAF_ERROR_IF_RET_NIL(smsPtr == nullptr, "Invalid hashmap reference");
        LE_DEBUG("Remove %s state from hashmap", tafMngdPMSvc::tafStateToString(smsPtr->state));
        le_hashmap_Remove(smsPMMap, smsPtr);
        le_mem_Release(smsPtr);
    }
}

tafMngdPMSms &tafMngdPMSms::GetInstance()
{
    static tafMngdPMSms instance;
    return instance;
}