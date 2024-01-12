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

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalPM.h"
#include "tafHalLib.hpp"

#define TAF_MNGD_PM_VM_HASH_SIZE 10
#define NODE_PRIMARY_NAD 0
#define VHAL_ACK_TIMEOUT 10000
#define WAKELOCK_WITHOUT_REF 0
#define MAX_SESSION 1

namespace telux {
namespace tafsvc {

typedef struct
{
    taf_mngd_pm_Nad_t nad;
    char vmName[TAF_MNGD_PM_MACHINE_NAME_LEN];
    taf_mngd_pm_State_t state;
}taf_mngdPm_State_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* shutdownCBCtxPtr;
    taf_mngd_pm_AsyncShutdownReqHandlerFunc_t shutdownCallbackFunc;
}taf_mngdPm_ShutdownCb_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* restartCBCtxPtr;
    taf_mngd_pm_AsyncRestartReqHandlerFunc_t restartCallbackFunc;
}taf_mngdPm_RestartCb_t;

typedef enum
{
    SYSTEM_FORCEFUL_SHUTDOWN,
    RESTART_WITH_NAD_POWER_OFF_ON
}taf_mngdPm_RequestedState_t;

typedef struct
{
   le_msg_SessionRef_t sessionRef;
   pid_t               pid;
}
taf_mngdPm_SessionNode_t;

typedef struct
{
    le_hashmap_Ref_t    clients;
    le_mem_PoolRef_t    SessionNodePool = NULL;
}
taf_mngdPm_Client_t;

class tafMngdPMSvc: public ITafSvc
{
    public:
        tafMngdPMSvc() {};
        ~tafMngdPMSvc() {};

        void Init(void);
        static tafMngdPMSvc &GetInstance();
        static le_result_t ParseJsonConfig(std::string configPath);
        static const char* tafStateToString(taf_mngd_pm_State_t tafState);
        static void OnClientConnection(le_msg_SessionRef_t sessionRef, void *ctxPtr);
        static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr);
        static le_result_t IsClientValid();
        static void StateChangeHandler(taf_pm_State_t state, void* contextPtr);
        static le_result_t InitVHalModule();
        static void StateChangeExHandler(taf_pm_PowerStateRef_t powerStateRef,
                taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr);
        static void VhalAckTimerHandler(le_timer_Ref_t timerRef);

};
}
}
