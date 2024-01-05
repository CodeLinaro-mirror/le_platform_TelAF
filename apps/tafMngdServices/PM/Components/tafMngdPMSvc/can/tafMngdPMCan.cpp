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

#include "tafMngdPMCan.hpp"
#include "tafMngdPMCommon.hpp"
#include "tafMngdPMSvc.hpp"

static le_hashmap_Ref_t canPMMap;

using namespace telux::tafsvc;

// Callback function for registered can frameId event
void tafMngdPMCan::CanEventCallback(taf_can_CanInterfaceRef_t canInfRef, bool isCanFdFrame,
                uint32_t frameId, const uint8_t* dataPtr, size_t size, void* contextPtr)
{
    taf_pm_ConnectService();
    LE_DEBUG("CanEventCallback frameid is 0x%X", frameId);

    TAF_ERROR_IF_RET_NIL(size > TAF_CAN_DATA_MAX_LENGTH, "Received size exceeds the max limit");

    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(canPMMap);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        taf_MngdPM_CanFrame_t* canFramePtr =
                (taf_MngdPM_CanFrame_t*)le_hashmap_GetValue(iteratorRef);
        TAF_ERROR_IF_RET_NIL(canFramePtr == nullptr, "Invalid hashmap reference");
        char rcvData[(2*size) + 1];
        le_hex_BinaryToString(dataPtr, size, rcvData, (2*size) + 1);
        if((canFramePtr->canFrameId << 1 == frameId << 1) &&
                (strncmp(rcvData, canFramePtr->msg, ((2*size) + 1)) == 0))
        {
            LE_INFO("canFrame 0x%X with msg %s initiate %s state", canFramePtr->canFrameId,
                    canFramePtr->msg, tafMngdPMSvc::tafStateToString(canFramePtr->state));
            taf_pm_SetAllVMPowerState((taf_pm_State_t)canFramePtr->state);
        }
    }
}

void tafMngdPMCan::RegisterCanEvents(uint32_t canFrameId, taf_mngd_pm_State_t state, const char* msg)
{
    LE_DEBUG("RegisterCanEvents canFrameId 0x%X", canFrameId);
    if(canMapPool == nullptr) {
        canMapPool = le_mem_CreatePool("tafMngdPMCanMapPool", sizeof(taf_MngdPM_CanFrame_t));
    }

    if(canPMMap == nullptr) {
        canPMMap =  le_hashmap_Create("tafMngdPMCanMap", TAF_MNGD_PM_MAX_TRIGGER_REGISTERS,
                le_hashmap_HashString, le_hashmap_EqualsString);
    }

    taf_MngdPM_CanFrame_t* canPMFramePtr = (taf_MngdPM_CanFrame_t*)le_mem_ForceAlloc(canMapPool);
    memset(canPMFramePtr, 0, sizeof(taf_MngdPM_CanFrame_t));
    canPMFramePtr->canFrameId = canFrameId;
    le_utf8_Copy(canPMFramePtr->msg, msg, 32, NULL);
    canPMFramePtr->state = state;

    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(canPMMap);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        taf_MngdPM_CanFrame_t* canFramePtr =
                (taf_MngdPM_CanFrame_t*)le_hashmap_GetValue(iteratorRef);
        TAF_ERROR_IF_RET_NIL(canFramePtr == nullptr, "Invalid hashmap reference");
        if(canFramePtr->canFrameId == canPMFramePtr->canFrameId)
        {
            LE_INFO("Already registered for canFrameId");
            canPMFramePtr->handlerRef = canFramePtr->handlerRef;
            char stateName[32];
            le_utf8_Copy(stateName, tafMngdPMSvc::tafStateToString(canPMFramePtr->state), 32, NULL);
            le_hashmap_Put(canPMMap, stateName, canPMFramePtr);
            return;
        }
    }

    // Create separate thread for registering for each frameid
    canPMFramePtr->threadRef = le_thread_Create("register_can_handler_thread",
                                    RegisterCanHandler, (void*)canPMFramePtr);
    le_thread_Start(canPMFramePtr->threadRef);
    return;
}

void* tafMngdPMCan::RegisterCanHandler(void* canFramePtr)
{
    taf_can_ConnectService();
    taf_MngdPM_CanFrame_t* canPMFramePtr = (taf_MngdPM_CanFrame_t*)canFramePtr;
    LE_INFO("RegisterCanHandler canFrameId 0x%X", canPMFramePtr->canFrameId);
    le_result_t res;
    auto &mngdPMCan = tafMngdPMCan::GetInstance();

    const char* infNamePtr = "can0";
    taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
    taf_can_CanInterfaceRef_t canInfRef = taf_can_CreateCanInf(infNamePtr, canInfType);
    res = taf_can_SetFilter(canInfRef, canPMFramePtr->canFrameId);
    if(res != LE_OK) {
        LE_ERROR("Failed to setFilter");
    } else {
        LE_INFO("setFilter successful");
    }
    canPMFramePtr->handlerRef = taf_can_AddCanEventHandler(canInfRef, canPMFramePtr->canFrameId,
            mngdPMCan.frIdMask, tafMngdPMCan::CanEventCallback, NULL);

    if(canPMFramePtr->handlerRef != NULL){
        LE_INFO("Susseccfully registered for CAN handler");
    }else{
        LE_ERROR("Failed to register for CAN event handler");
        return NULL;
    }
    char stateName[32];
    le_utf8_Copy(stateName, tafMngdPMSvc::tafStateToString(canPMFramePtr->state), 32, NULL);
    le_hashmap_Put(canPMMap, stateName, canPMFramePtr);
    le_event_RunLoop();
}

void tafMngdPMCan::DeregisterCanEvents()
{
    LE_DEBUG("DeregisterCanEvents");
    if(!canPMMap)
        return;

    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(canPMMap);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        taf_MngdPM_CanFrame_t *canFramePtr = (taf_MngdPM_CanFrame_t*)le_hashmap_GetValue(iter);
        TAF_ERROR_IF_RET_NIL(canFramePtr == nullptr, "Invalid hashmap reference");
        LE_DEBUG("Remove %p from hashmap", canFramePtr);
        taf_can_RemoveCanEventHandler(canFramePtr->handlerRef);
        le_hashmap_Remove(canPMMap, canFramePtr);
        le_mem_Release(canFramePtr);
    }
}

tafMngdPMCan &tafMngdPMCan::GetInstance()
{
    static tafMngdPMCan instance;
    return instance;
}

