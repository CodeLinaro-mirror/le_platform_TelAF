/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_SECURITY_ACCESS_HPP
#define TAF_SECURITY_ACCESS_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafUDSCommunicationMgr.hpp"

namespace taf{
namespace uds{

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SecAccSignal_s {

    /* Dtool -> Stack */
    REQUEST_SEED_SIG = 3, /* == M_USER_SIG */
    SEND_KEY_SIG,

    /* Apps -> Stack */
    REQUEST_SEED_RESPONSE_SIG,
    SEND_KEY_RESPONSE_SIG,

    /* Delay_Timer timerout */
    DELAY_TIMER_EXPIRED_SIG,

    /* Session switch */
    SESSION_CONTROL_SIG,

    /* Session timeout */
    SESSION_TIMEOUT_SIG,

    /* Reserve one invalid signal for internal-checking */
    INVALID_SIG = 0xFF,
} SecAccSignal_t;

typedef struct {
    SecAccSignal_t type;
    le_sem_Ref_t sem;
    UdsCommunicationMgr * mgr;
    char semName[UDS_SEC_SEM_NAME_MAX_LEN];
    bool * is_internal;
} SecAccReport_t;

struct AO_SecurityAccess_s;

extern le_event_Id_t SecAccEventIdRef;

void SecurityAccess_Init(void * u, void * p);
void SecurityAccess_CreateActiveObject(void * mgr, void * ifname);
void SecurityAccess_StartWorker(void * u, void *p);

bool SecurityAccess_IsUnlocked(UdsCommunicationMgr * mgr);
bool SecurityAccess_IsLevelUnlocked
(
    UdsCommunicationMgr * mgr,
    uint8_t level
);

#ifdef __cplusplus
}
#endif

}
}


#endif /* TAF_SECURITY_ACCESS_HPP */
