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
    uint32_t curr_session_id;
    uint32_t prev_session_id;
    le_sem_Ref_t sem;
    UdsCommunicationMgr * mgr;
    bool * is_internal;
} SecAccReport_t;

extern le_event_Id_t SecAccEventIdRef;

/* All initialization operations and create a new thread for the State Machine */
void SecurityAccess_Init(void *, void *);

bool SecurityAccess_IsUnlocked(void);

#ifdef __cplusplus
}
#endif

}
}


#endif /* TAF_SECURITY_ACCESS_HPP */
