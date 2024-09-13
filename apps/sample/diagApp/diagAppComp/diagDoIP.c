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

#include "diagPrivate.h"

// Diag DoIP service reference
static taf_diagDoIP_ServiceRef_t diagDoIPSvcRef = NULL;
static taf_diagDoIP_EventHandlerRef_t diagDoIPEventRef = NULL;
static le_sem_Ref_t semRef;

static const char defVIN[] = "LE40BBEB4NL921057";
static const char defEID[] = "3A5B10228D03";
static const char defGID[] = "EA09D9D8B87A";

// Callback function for DoIP event
static void DoIPEventHandler
(
    taf_diagDoIP_ServiceRef_t svrRef,
    taf_diagDoIP_EventType_t eventType,
    uint16_t remoteLogicAddr,
    uint16_t vlanId,
    void* contextPtr
)
{
    LE_TEST_INFO("Received a DoIP event notification");
    LE_TEST_INFO("Event type is 0x%x, client logical address is 0x%x, vlan0x%x",
        (uint32_t)eventType, remoteLogicAddr, vlanId);
}

void SetAndGetVIN
(
    void
)
{
    le_result_t ret;
    char vin[TAF_DIAGDOIP_VIN_SIZE+1];
    
    ret = taf_diagDoIP_SetVIN(defVIN);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_SetVIN");
    
    ret = taf_diagDoIP_GetVIN(vin, TAF_DIAGDOIP_VIN_SIZE+1);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_GetVIN");
    
    LE_TEST_ASSERT(strcmp(defVIN, vin) == 0, "SetAndGetVIN");
}

void SetAndGetEID
(
    void
)
{
    le_result_t ret;
    char eid[TAF_DIAGDOIP_EID_SIZE+1];
    
    ret = taf_diagDoIP_SetEID(defEID);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_SetEID");
    
    ret = taf_diagDoIP_GetEID(eid, TAF_DIAGDOIP_EID_SIZE+1);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_GetEID");
    
    LE_TEST_INFO("EID: %s", eid);
    LE_TEST_ASSERT(strcasecmp(defEID, eid) == 0, "SetAndGetEID");
}

void SetAndGetGID
(
    void
)
{
    le_result_t ret;
    char gid[TAF_DIAGDOIP_GID_SIZE+1];
    
    ret = taf_diagDoIP_SetGID(defGID);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_SetGID");
    
    ret = taf_diagDoIP_GetGID(gid, TAF_DIAGDOIP_GID_SIZE+1);
    LE_TEST_ASSERT(ret == LE_OK, "taf_diagDoIP_GetGID");
    
    LE_TEST_ASSERT(strcasecmp(defGID, gid) == 0, "SetAndGetGID");
}

static void* diagDoIPEventThread
(
    void* ctxPtr
)
{
    taf_diagDoIP_ConnectService();

    diagDoIPEventRef = taf_diagDoIP_AddEventHandler(diagDoIPSvcRef, 0, DoIPEventHandler, NULL);
    LE_TEST_ASSERT(diagDoIPEventRef != NULL, "Registered DoIPEventHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagDoIP_Init
(
    void
)
{
    semRef = le_sem_Create("SemRef", 0);
    
    diagDoIPSvcRef = taf_diagDoIP_GetService(0x0201);
    LE_TEST_ASSERT(diagDoIPSvcRef != NULL, "taf_diagDoIP_GetService");
    
    SetAndGetVIN();
    SetAndGetEID();
    SetAndGetGID();
    
    // Create diag DoIP event handle thread to handle related events
    le_thread_Ref_t diagDoIPThrRef = le_thread_Create("diagDoIPThr",
            diagDoIPEventThread, NULL);

    le_thread_Start(diagDoIPThrRef);
    le_sem_Wait(semRef);

    return LE_OK;    
}