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

#ifndef TEST_IDS_H_INCLUDE_GUARD
#define TEST_IDS_H_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

#define RT0_DEV_NAME "bridge0"        // Device name of default routing manager.
#define RT1_DEV_NAME "eth0.1"         // Device name of routing manager 1.
#define RT2_DEV_NAME "eth0.2"         // Device name of routing manager 2.

#define RT0_SID_A 0x000A       // Service/Inst/method ID offered by default routMgr of appA
#define RT0_SID_B 0x000B       // Service/Inst/method ID offered by default routMgr of appB

#define RT1_SID_A 0x111A       // Service/Inst/method ID offered by routMgr 1 of appA
#define RT1_SID_B 0x111B       // Service/Inst/method ID offered by routMgr 1 of appB

#define RT2_SID_A 0x222A       // Service/inst/method ID offered by routMgr 2 of appA
#define RT2_SID_B 0x222B       // Service/Inst/method ID offered by routMgr 2 of appB

#define RT0_EID_A 0xE00A       // Event/Group ID offered by default routMgr of appA
#define RT0_EID_B 0xE00B       // Event/Group ID offered by default routMgr of appB

#define RT1_EID_A 0xE11A       // Event/Group ID offered by routMgr 1 of appA
#define RT1_EID_B 0xE11B       // Event/Group ID offered by routMgr 1 of appB

#define RT2_EID_A 0xE22A       // Event/Group ID offered by routMgr 2 of appA
#define RT2_EID_B 0xE22B       // Event/Group ID offered by routMgr 2 of appB

#define RT0_PORT_A 52221       // Service port for default routMgr of appA
#define RT0_PORT_B 52222       // Service port for default routMgr of appB

#define RT1_PORT_A 52223       // Serivce port for routMgr 1 of appA
#define RT1_PORT_B 52224       // Serivce port for routMgr 1 of appB

#define RT2_PORT_A 52225       // Service port for routMgr 2 of appA
#define RT2_PORT_B 52226       // Service port for routMgr 2 of appB

#define TEST_MAJ_VERSION 0x00
#define TEST_MIN_VERSION 0x00000000

#define TEST_MAX_MSG_SIZE 256
#define TEST_MAX_APP_ID 6

typedef enum
{
    RT0_APP_A = 0,
    RT0_APP_B,
    RT1_APP_A,
    RT1_APP_B,
    RT2_APP_A,
    RT2_APP_B
}
AppTestId_t;

typedef struct
{
    uint16_t sid;
    uint16_t eid;
    uint16_t port;
    char dev[TAF_SOMEIPDEF_MAX_IFNAME_LENGTH];
    taf_someipSvr_ServiceRef_t svrRef;
    taf_someipSvr_RxMsgHandlerRef_t rxRef;
    taf_someipSvr_SubscriptionHandlerRef_t subsRef;
    le_timer_Ref_t ntimerRef;
}
ServerAppInfo_t;

typedef struct
{
    uint16_t sid;
    uint16_t eid;
    char dev[TAF_SOMEIPDEF_MAX_IFNAME_LENGTH];
    taf_someipClnt_ServiceRef_t cliRef;
    taf_someipClnt_StateChangeHandlerRef_t stateRef;
    taf_someipClnt_EventMsgHandlerRef_t evtRef;
    le_timer_Ref_t stimerRef;
}
ClientAppInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct initialization.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_DataInit
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Offer a service with a given test app ID.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_OfferService
(
    AppTestId_t appId, ///< [IN]
    ServerAppInfo_t* serverInfoPtr   ///< [OUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Request service with a given test app ID.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_RequestService
(
    AppTestId_t appId, ///< [IN]
    ClientAppInfo_t* ClientInfoPtr   ///< [OUT]
);

#endif /* TEST_IDS_H_INCLUDE_GUARD */
