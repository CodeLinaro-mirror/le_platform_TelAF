/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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
    taf_mngdPm_WakeupType_t wakeupType = TAF_MNGDPM_SMS;
    bool isWhiteListed = false;
    if(tafMngdPMSvc::wsWhiteList.size() > 0) {
        for(auto &client : tafMngdPMSvc::wsWhiteList)
        {
            if(client.wakeupType == wakeupType)
            {
                LE_INFO("wakeupType in wsWhiteList");
                isWhiteListed = true;
                break;
            }
            else
            {
                LE_INFO("wakeupType not in wsWhiteList");
            }
        }

    }
    if(isWhiteListed)
    {
        LE_INFO("Continue for state change request");
    }
    else
    {
        isWhiteListed = false;
        LE_INFO("Failed for state change request");
        return;
    }
    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(smsPMMap);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        taf_MngdPM_Sms_t* smsPtr = (taf_MngdPM_Sms_t*)le_hashmap_GetValue(iteratorRef);

        TAF_ERROR_IF_RET_NIL(smsPtr == nullptr, "Invalid hashmap reference");

        if(strncmp(text, smsPtr->text, sizeof(text)) == 0)
        {
            LE_DEBUG("SMS matched with SMS registetred for %s state",
                    tafMngdPMSvc::TafStateToString(smsPtr->state));

            taf_mngdPm_State_t requestedState = TAF_MNGDPM_STATE_UNKNOWN;
            switch((taf_pm_State_t)smsPtr->state)
            {
                case TAF_PM_STATE_SUSPEND:
                    requestedState = TAF_MNGDPM_STATE_SUSPENDING;
                    break;

                case TAF_PM_STATE_SHUTDOWN:
                    requestedState = TAF_MNGDPM_STATE_SHUTTING_DOWN;
                    break;

                case TAF_PM_STATE_RESUME:
                    requestedState = TAF_MNGDPM_STATE_WAKING_UP;
                    break;

                default:
                    break;
            }

            if(tafMngdPMSvc::RequestStateChange(requestedState) != LE_OK)
            {
                return;
            }

            taf_pm_SetAllVMPowerState((taf_pm_State_t)smsPtr->state);

            tafMngdPMSvc::ProcessStateChange(requestedState);
        }
    }
}

void tafMngdPMSms::RegisterSms(const char* text, taf_mngdPm_State_t state)
{
    if(smsMapPool == NULL) {
        smsMapPool = le_mem_CreatePool("tafMngdPMSmsMapPool", sizeof(taf_MngdPM_Sms_t));
    }
    taf_MngdPM_Sms_t* smsPMPtr = (taf_MngdPM_Sms_t*)le_mem_ForceAlloc(smsMapPool);
    memset(smsPMPtr, 0, sizeof(taf_MngdPM_Sms_t));
    le_utf8_Copy(smsPMPtr->text, text, TAF_SMS_TEXT_BYTES, NULL);
    smsPMPtr->state = state;

    if(smsPMMap == NULL) {
        smsPMMap =  le_hashmap_Create("tafMngdPMSmsMap", TAF_MNGDPM_MAX_TRIGGER_REGISTERS,
                le_hashmap_HashString, le_hashmap_EqualsString);
    }
    char stateName[32];
    le_utf8_Copy(stateName, tafMngdPMSvc::TafStateToString(state), 32, NULL);
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
        LE_DEBUG("Remove %s state from hashmap", tafMngdPMSvc::TafStateToString(smsPtr->state));
        le_hashmap_Remove(smsPMMap, smsPtr);
        le_mem_Release(smsPtr);
    }
}

tafMngdPMSms &tafMngdPMSms::GetInstance()
{
    static tafMngdPMSms instance;
    return instance;
}