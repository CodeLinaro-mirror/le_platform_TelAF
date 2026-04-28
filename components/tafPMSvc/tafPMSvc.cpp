/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <iostream>
#include <string>
#include <memory>
#include <future>
#include <unistd.h>
#include <vector>

using namespace std;

// provided: Actual PA layer APIs
#include "tafPmsPa.hpp"

#define TAF_TAG_PREFIX   "taf"
#define TAF_WS_NAME_FORMAT TAF_TAG_PREFIX"_%s_%p"
#define TAF_WS_PROCNAME_LEN 30
#define TAF_WS_NAME_LEN (sizeof(TAF_TAG_PREFIX) + TAF_PM_TAG_LEN + TAF_WS_PROCNAME_LEN + 3)

#define TAF_MNGD_PM_SVC       "tafMngdPMSvc"
#define TAF_RPC_PROXY         "tafRpcProxy"
#define TAF_PM_UNIT_TEST_APP  "tafPMUnitTest"
#define TAF_PM_INTG_TEST_APP  "tafPMIntgTest"

#define TAF_PM_CLIENT_DEFAULT_POOL_SIZE      8
#define TAF_PM_CLIENT_DEFAULT_HASH_SIZE      31
#define TAF_WAKEUP_SOURCE_DEFAULT_POOL_SIZE  64
#define TAF_PM_REFERENCE_DEFAULT_POOL_SIZE   31
#define TAF_POWER_SOURCE_DEFAULT_POOL_SIZE   64
#define PMS_CLNTS_ACK_TIMEOUT                350
#define DEFAULT_SUPPORTED_REG_HANDLER_NUM    5
#define DEFAULT_VM_LIST_SIZE                 3
#define PA_LAYER_TIMEOUT_MAX_MS              1000

#define API(x)  taf_pm_ ## x

typedef struct
{
    char          name[TAF_WS_NAME_LEN];
    pid_t         pid;
    uint32_t      acquired;
    bool          isRef;
    void          *wsRef;
}
WakeSrc_t;

typedef struct
{
    pid_t               pid;
    le_msg_SessionRef_t sessionRef;
    char                name[TAF_WS_PROCNAME_LEN + 1];
}
Client_t;

typedef struct
{
    void                 *safeRef;
    void                 *handler;
    void                 *context;
    le_msg_SessionRef_t   session;
    bool                  ismpm;
}
ApiHandler_t;

typedef struct
{
    char          name[TAF_PM_MACHINE_NAME_LEN];
    le_dls_Link_t link;
}
VmInfo_t;

typedef struct
{
    le_dls_List_t       list;
    le_dls_Link_t*      current;
    taf_pm_VMListRef_t  safeRef;
    le_msg_SessionRef_t session;
}
VmList_t;

typedef struct {
    taf_pm_State_t  state;
    bool            hasNack;
    uint32_t        nAlreadySent;
    uint32_t        nAlreadyAcked;
    void*           safeRef;
} AckCollection_t;

static AckCollection_t AckCollection;

typedef struct {
    taf_pa_pms_ConsolidatedInfo_t info;
    bool isAllAcked;
    taf_pm_State_t state;
} FinalConsolidatedInfo_t;

static FinalConsolidatedInfo_t FinalConsolidatedInfo;

static class taf_PM
{
public:

    le_ref_MapRef_t  refmap_WakeSrc;  // [safeRef: wakeSrcItem]

    le_mem_PoolRef_t pool_WakeSrc;
    le_hashmap_Ref_t hmap_WakeSrc;    // [taf_<tag>_<sess-ptr>: wakeSrcItem]

    le_mem_PoolRef_t pool_Client;
    le_hashmap_Ref_t hmap_Client;     // [client-session-ref: clientItem]

    uint32_t         totalWakeupSrc;

    le_event_Id_t    evt_StateChangedAckNeeded;

    le_timer_Ref_t   ref_RestartTimer;

    le_mem_PoolRef_t pool_ApiHandler;
    le_mem_PoolRef_t pool_VmInfo;
    le_mem_PoolRef_t pool_VmList;

    le_ref_MapRef_t  refmap_ConsolidatedAckInfo;
    le_ref_MapRef_t  refmap_StateChanged;
    le_ref_MapRef_t  refmap_StateChangedAckNeeded;
    le_ref_MapRef_t  refmap_WakeupInfo;
    le_ref_MapRef_t  refmap_AckNeededClients;
    le_ref_MapRef_t  refmap_VmList;

    // For 'powerMode'
    void SetPowerMode(taf_pm_PowerMode_t m);
    taf_pm_PowerMode_t GetPowerMode(void);

    le_result_t SetPowerState
    (
        taf_pm_State_t state,
        const char* machineName
    );

    void SetCurrentState
    (
        taf_pm_State_t state
    );

    taf_pm_State_t GetCurrentState(void);

    taf_pm_NodeModemWsBitMask_t GetLastModemWsReason();

    static void SendAckToPaLayer
    (
        taf_pm_State_t state,
        taf_pa_pms_Ack_t ack
    );

    static void ShowCurrentClientSessionInfo
    (
        const char * fname,
        const char * action
    );

    static void ShowWakeSourceInfo
    (
        const char * from
    );

    taf_pa_pms_Reference_t pa;

    void AllocateResource(void);

    void TryToInitPaLayer(void);

private:
    // IPC client connection is opened
    static void OnClientConnected
    (
        le_msg_SessionRef_t sessionRef,
        void *ctxPtr
    );

    // IPC client connection is closed
    static void OnClientDisconnected
    (
        le_msg_SessionRef_t sessionRef,
        void *ctxPtr
    );

    static void Handle_evt_RestartTimerExpired
    (
        le_timer_Ref_t timerRef
    );

    static void Handle_evt_ConsolidatedAckInfo
    (
        void * reportPtr
    );

    static void Handle_evt_StateChanged
    (
        void * reportPtr
    );

    static void Handle_evt_StateChangedAckNeeded
    (
        void * reportPtr
    );

    static void PaHandler_evt_WakeupInfo
    (
        void * payload
    );

    static void PaHandler_evt_ServiceAvailable
    (
        void * payload
    );

    static void PaHandler_evt_PowerStateUpdate
    (
        void * payload
    );

    static void PaHandler_evt_MachineUpdate
    (
        void * payload
    );

    static void PaHandler_evt_ConsolidatedInfo
    (
        void * payload
    );

    static void Handle_sig_SIGTERM
    (
        int sig
    );

    static void PaEventReportCallback
    (
        taf_pa_pms_Event_t * ev
    );

    static void PaIndication_Handler
    (
        void * reportPtr
    );

    static void * AllocPaEvtPayload
    (
        taf_pa_pms_Event_t * ev
    );

    static void FreePaEvtPayload
    (
        taf_pa_pms_Event_t * ev
    );

    le_event_Id_t evt_ConsolidatedAckInfo;
    le_event_Id_t evt_StateChanged;
    le_event_Id_t evt_PaInd;
    le_event_HandlerRef_t ref_PaEventHandler;

    bool                   isFull;
    taf_pm_PowerMode_t     powerMode;
    taf_pm_State_t         currentState;

    static taf_pm_NodeModemWsBitMask_t lastModemWsReason;

} pm; /* Singleton */

/* static members declared */
taf_pm_NodeModemWsBitMask_t taf_PM::lastModemWsReason;

static inline WakeSrc_t * to_WakeSrc_t
(
    taf_pm_WakeupSourceRef_t wsRef
)
{
    WakeSrc_t *ws =
        (WakeSrc_t *)
            le_ref_Lookup(pm.refmap_WakeSrc, wsRef);

    if (nullptr == ws)
    {
        return nullptr;
    }

    return ws;
}

void taf_PM::SetPowerMode
(
    taf_pm_PowerMode_t mode
)
{
    pm.powerMode = mode;
}

taf_pm_NodeModemWsBitMask_t taf_PM::GetLastModemWsReason
(
    void
)
{
    return pm.lastModemWsReason;
}

taf_pm_PowerMode_t taf_PM::GetPowerMode(void)
{
    return pm.powerMode;
}

static bool IsClientMPMS
(
    le_msg_SessionRef_t currentSession
)
{
    Client_t *pClient =
        (Client_t *) le_hashmap_Get(pm.hmap_Client, currentSession);

    if (pClient == nullptr)
    {
        return false;
    }

    LE_INFO("Client is %s", pClient->name);

    if(strncmp(pClient->name, TAF_MNGD_PM_SVC, sizeof(TAF_MNGD_PM_SVC)) == 0)
    {
        return true;
    }

    return false;
}

static inline const char *to_StateText
(
    taf_pm_State_t state
)
{
    switch (state)
    {
        case TAF_PM_STATE_RESTART: return "st(RESTART)";
        case TAF_PM_STATE_RESUME: return "st(RESUME)";
        case TAF_PM_STATE_SUSPEND: return "st(SUSPEND)";
        case TAF_PM_STATE_SHUTDOWN: return "st(SHUTDOWN)";

        case TAF_PM_STATE_ALL_ACKED:
            return "st(ALL_ACKED/internal)";
        case TAF_PM_STATE_ALL_WAKELOCKS_RELEASED:
            return "st(ALL_WAKELOCKS_RELEASED/internal)";

        case TAF_PM_STATE_UNKNOWN:
        default:
            return "st(UNKNOWN)";
    }
}

static taf_pa_pms_PowerState_t to_PaPowerState
(
    taf_pm_State_t state
)
{
    switch(state)
    {
        case TAF_PM_STATE_RESUME: return taf_pa_pms_PwrSts_RESUME;
        case TAF_PM_STATE_SUSPEND: return taf_pa_pms_PwrSts_SUSPEND;
        case TAF_PM_STATE_SHUTDOWN: return taf_pa_pms_PwrSts_SHUTDOWN;

        /* Internal state */
        case TAF_PM_STATE_RESTART:
        case TAF_PM_STATE_ALL_ACKED:
        case TAF_PM_STATE_ALL_WAKELOCKS_RELEASED:
        case TAF_PM_STATE_UNKNOWN:
        default:
            return taf_pa_pms_PwrSts_UNKNOWN;
    }
}

static taf_pm_State_t from_PaPowerState
(
    taf_pa_pms_PowerState_t paState
)
{
    switch (paState)
    {
        case taf_pa_pms_PwrSts_SUSPEND:  return TAF_PM_STATE_SUSPEND;
        case taf_pa_pms_PwrSts_RESUME:   return TAF_PM_STATE_RESUME;
        case taf_pa_pms_PwrSts_SHUTDOWN: return TAF_PM_STATE_SHUTDOWN;
        case taf_pa_pms_PwrSts_UNKNOWN:
        default:
            return TAF_PM_STATE_UNKNOWN;
    }
}

void taf_PM::SetCurrentState
(
    taf_pm_State_t state
)
{
    currentState = state;
}

taf_pm_State_t taf_PM::GetCurrentState
(
    void
)
{
    return currentState;
}

le_result_t taf_PM::SetPowerState
(
    taf_pm_State_t state,
    const char* machineName
)
{
    le_result_t rst = LE_OK;
    taf_pm_State_t currentState = pm.GetCurrentState();

    // TODO:
    // Duplicated request for same 'state' from the client should be handled properly.

    LE_INFO("Wants to change from [%s] to [%s]",
            to_StateText(currentState),
            to_StateText(state));

    switch(state)
    {
        /* Handle SUSPEND state first */
        case TAF_PM_STATE_SUSPEND:
        {
            if (pm.GetPowerMode() == TAF_PM_POWER_MODE_LOW_POWER)
            {
                LE_INFO("Current is in [low-power] mode, force SUSPEND");

                // Don't change any wake lock!

                // go to 'TAF_PM_STATE_SHUTDOWN' block to continue..
            }
            else // == TAF_PM_POWER_MODE_NORMAL
            {
                LE_INFO("Current is in [normal-power] mode");

                if (pm.totalWakeupSrc > 0)
                {
                    LE_INFO("Total wake lock is [%d],"
                            " but wants SUSPEND, reject", pm.totalWakeupSrc);

                    break; // nothing to do in this case
                }
                else // == 0
                {
                    LE_INFO("Total wake lock is [0], to SUSPEND");

                    // continue following..
                }
            }
        }
        /* No 'break', go through --> */
        case TAF_PM_STATE_RESUME:
        case TAF_PM_STATE_SHUTDOWN:
        {
            pa_result_t paRst =
                taf_pa_pms_SetPowerStateAsMaster(
                    pm.pa,
                    to_PaPowerState(state),
                    machineName);

            if (PA_OK != paRst)
            {
                LE_ERROR("Failed to invoke PA:SetPowerStateAsMaster: %d", paRst);
                rst = LE_FAULT;
            }
            else
            {
                LE_INFO("PA:SetPowerStateAsMaster done");
                pm.SetCurrentState(state);
            }
        }
        break;

        /* Consumed by PMS itself */
        case TAF_PM_STATE_RESTART:
        {
            pm.SetCurrentState(state);
            LE_INFO("Set 'RESTART' state done");

            le_event_Report(pm.evt_StateChanged, &state, sizeof(state));
            le_event_Report(pm.evt_StateChangedAckNeeded, &state, sizeof(state));

            if (le_timer_IsRunning(ref_RestartTimer))
            {
                le_timer_Stop(ref_RestartTimer);
            }

            le_timer_Start(ref_RestartTimer);

            LE_INFO("Start the timer for NeededAckClient");
        }
        break;

        /* UNKNOWN
         * STATE_ALL_ACKED
         * STATE_ALL_WAKELOCKS_RELEASED */
        default:
        {
            LE_ERROR("API Unsupported state: [%d]", state);
            rst = LE_BAD_PARAMETER;
        }
    }

    return rst;
}

void taf_PM::OnClientConnected
(
    le_msg_SessionRef_t sessionRef,
    void *ctxPtr
)
{
    Client_t *pClient;
    FILE *procFd;
    char procStr[PATH_MAX];
    size_t procLen;
    pid_t clientPid;

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &clientPid))
    {
        LE_ERROR("Session was closed by client ? Ignore");
        return;
    }

    if (le_hashmap_ContainsKey(pm.hmap_Client, sessionRef))
    {
        LE_KILL_CLIENT("Client session has stored, reject duplicated connection!");
        return;
    }

    // update client record, exits on error
    pClient = (Client_t*)le_mem_ForceAlloc(pm.pool_Client);
    memset(pClient, 0x00, sizeof(Client_t));
    pClient->sessionRef = sessionRef;
    pClient->pid = clientPid;

    // Opening the pClient process command line
    snprintf(procStr, sizeof(procStr), "/proc/%d/comm", pClient->pid);
    procFd = fopen(procStr, "r");
    if (nullptr == procFd)
    {
        LE_FATAL("Failed to open %s: %m", procStr);
    }

    // Get the pClient process name
    if (nullptr == fgets(procStr, sizeof(procStr), procFd))
    {
        LE_FATAL("Failed to scan %s: %m", procStr);
    }
    fclose(procFd);
    procLen = strlen(procStr);
    procStr[procLen - 1] = '\0';
    le_utf8_Copy(pClient->name, procStr, sizeof(pClient->name), nullptr);

    le_hashmap_Put(pm.hmap_Client, sessionRef, pClient);

    LE_INFO("Client (%s/%d) connected", pClient->name, pClient->pid);
}

// NOTE: Invoke this function in 'OnClientDisconnected' only
static void RemoveClientRegisteredHandler
(
    le_ref_MapRef_t mapRef
)
{
    vector<const void *> recordedHandlers;

    le_ref_IterRef_t it = le_ref_GetIterator(mapRef);

    le_result_t rst = le_ref_NextNode(it);

    while (rst == LE_OK)
    {
        ApiHandler_t * hdlr =
            (ApiHandler_t *)
                le_ref_GetValue(it);

        // Remove 'current-session' that client was closed already.
        if (hdlr->session == taf_pm_GetClientSessionRef())
        {
            const void * item = le_ref_GetSafeRef(it);
            recordedHandlers.push_back(item);
        }

        rst = le_ref_NextNode(it);
    }

    for (auto iDeleted: recordedHandlers)
    {
        ApiHandler_t * hdlr =
            (ApiHandler_t *)
                le_ref_Lookup(mapRef, (void *)iDeleted);

        // Release the memory allocated
        le_mem_Release(hdlr);

        // Remove the safe reference from map
        le_ref_DeleteRef(mapRef, (void *)iDeleted);
    }
}

void taf_PM::OnClientDisconnected
(
    le_msg_SessionRef_t sessionRef,
    void *ctxPtr
)
{
    Client_t *pClient;
    WakeSrc_t *ws;
    le_hashmap_It_Ref_t iter;

    // Try to remove all registered IPC-Handlers
    RemoveClientRegisteredHandler(pm.refmap_StateChanged);
    RemoveClientRegisteredHandler(pm.refmap_ConsolidatedAckInfo);
    RemoveClientRegisteredHandler(pm.refmap_StateChangedAckNeeded);
    RemoveClientRegisteredHandler(pm.refmap_WakeupInfo);

    // Find and remove powermanager client record from table
    pClient = (Client_t *)le_hashmap_Remove(pm.hmap_Client, sessionRef);

    if (pClient == nullptr)
    {
        LE_WARN("Session was not registered: %p, remove failed", sessionRef);
        return;
    }

    LE_INFO("Client proccessId (%s/%d) disconnected.",
            pClient->name,
            pClient->pid);

    // Find and remove all wakeup sources held for this client
    iter = le_hashmap_GetIterator(pm.hmap_WakeSrc);

    while (LE_OK == le_hashmap_NextNode(iter))
    {
        ws = (WakeSrc_t*)le_hashmap_GetValue(iter);

        if (ws->pid != pClient->pid)
        {
            // skip if does not belong to this client
            continue;
        }

        // Release the aquired wakeup source
        if (ws->acquired)
        {
            LE_WARN("Releasing wakeup source '%s' on behalf of %s/%d.",
                    ws->name, pClient->name, pClient->pid);

            ws->isRef = false; // Must to set for relax
            API(Relax)((taf_pm_WakeupSourceRef_t)ws->wsRef);
        }

        // Delete wakeup source record from powermanager record, free memory
        LE_INFO("Deleting wakeup source '%s' on behalf of pid %d.", ws->name, ws->pid);
        le_hashmap_Remove(pm.hmap_WakeSrc, ws->name);
        le_ref_DeleteRef(pm.refmap_WakeSrc, ws->wsRef);
        le_mem_Release(ws);
    }

    le_mem_Release(pClient);

    // Clean all VmList_t references not-released about the losing session
    le_ref_IterRef_t iterRef = le_ref_GetIterator(pm.refmap_VmList);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        VmList_t * vmList =
            (VmList_t *)
                le_ref_GetValue(iterRef);

        if (vmList != nullptr && vmList->session == sessionRef)
        {
            API(DeleteMachineList)(vmList->safeRef);
        }
    }
}

void taf_PM::SendAckToPaLayer
(
    taf_pm_State_t state,
    taf_pa_pms_Ack_t ack
)
{
    if (state != TAF_PM_STATE_SUSPEND
    &&  state != TAF_PM_STATE_SHUTDOWN)
    {
        LE_INFO("Pa layer only supports [SUSPEND] & [SHUTDOWN], but %s",
                to_StateText(state));
        return;
    }

    LE_INFO("Sending [%s] to PA Layer with [%s]",
            ack == taf_pa_pms_ACK ? "ACK" : "NACK",
            to_StateText(state));

    pa_result_t rst =
        taf_pa_pms_SendAckForStateUpdate(
            pm.pa, to_PaPowerState(state), ack);

    if (PA_OK != rst)
    {
        LE_ERROR("Failed to invoke PA:SendAckForStateUpdate: %d", rst);
    }
}

void taf_PM::ShowCurrentClientSessionInfo
(
    const char * fname,
    const char * action
)
{
    Client_t *pClient =
        (Client_t *)
            le_hashmap_Get(pm.hmap_Client, taf_pm_GetClientSessionRef());

    if (pClient != nullptr)
    {
        LE_INFO("%s, action:[%s] from <- (%s/%d)",
                fname,
                action,
                pClient->name,
                pClient->pid);
    }
}

void taf_PM::ShowWakeSourceInfo
(
    const char * from
)
{
    WakeSrc_t * ws;

    LE_INFO("Total [wakeup-source]: %d from [%s]", pm.totalWakeupSrc, from);

    le_ref_IterRef_t it = le_ref_GetIterator(pm.refmap_WakeSrc);
    while (LE_OK == le_ref_NextNode(it))
    {
        ws = (WakeSrc_t *) le_ref_GetValue(it);
        LE_INFO(" -- ws: [%s](%d), acquired: [%d], ref: [%d]",
                ws->name,
                ws->pid,
                ws->acquired,
                ws->isRef);
    }
}

void taf_PM::PaHandler_evt_ConsolidatedInfo
(
    void * payload
)
{
    taf_pa_pms_ConsolidatedInfo_t * info = (taf_pa_pms_ConsolidatedInfo_t *) payload;

    LE_DEBUG("--> Consolidated info indication is coming");

    uint32_t validSize = 0;

    if (info->nackResponseClntSize > TAF_CONSOLIDATED_CLNT_SIZE)
    {
        // Need to highlight the 'overflowed'
        LE_ERROR("N-ACK client size is overflowed");
        validSize = TAF_CONSOLIDATED_CLNT_SIZE;
    }
    else
    {
        validSize = info->nackResponseClntSize;
    }

    for(uint32_t i = 0; i < validSize; i++)
    {
        le_utf8_Copy(
            FinalConsolidatedInfo.info.nackResponseClntData[i].clientName,
            info->nackResponseClntData[i].clientName,
            sizeof(FinalConsolidatedInfo.info.nackResponseClntData[i].clientName),
            NULL);
        le_utf8_Copy(
            FinalConsolidatedInfo.info.nackResponseClntData[i].machineName,
            info->nackResponseClntData[i].machineName,
            sizeof(FinalConsolidatedInfo.info.nackResponseClntData[i].machineName),
            NULL);
    }

    FinalConsolidatedInfo.info.nackResponseClntSize = validSize;

    if (info->unresponsiveClntSize > TAF_CONSOLIDATED_CLNT_SIZE)
    {
        LE_ERROR("N-RESP client size is overflowed");
        validSize = TAF_CONSOLIDATED_CLNT_SIZE;
    }
    else
    {
        validSize = info->unresponsiveClntSize;
    }

    for(uint32_t i = 0; i < validSize; i++)
    {
        le_utf8_Copy(
            FinalConsolidatedInfo.info.unresponsiveClntData[i].clientName,
            info->unresponsiveClntData[i].clientName,
            sizeof(FinalConsolidatedInfo.info.unresponsiveClntData[i].clientName),
            NULL);
        le_utf8_Copy(
            FinalConsolidatedInfo.info.unresponsiveClntData[i].machineName,
            info->unresponsiveClntData[i].machineName,
            sizeof(FinalConsolidatedInfo.info.unresponsiveClntData[i].machineName),
            NULL);
    }

    FinalConsolidatedInfo.info.unresponsiveClntSize = validSize;

    LE_INFO("is-all-acked? - [nack](%d) + [unresp](%d)",
            FinalConsolidatedInfo.info.nackResponseClntSize,
            FinalConsolidatedInfo.info.unresponsiveClntSize);

    FinalConsolidatedInfo.isAllAcked = (
        FinalConsolidatedInfo.info.nackResponseClntSize == 0
        &&
        FinalConsolidatedInfo.info.unresponsiveClntSize == 0
    );

    FinalConsolidatedInfo.state = pm.GetCurrentState();

    LE_INFO("Consolidated info->state(current): %s(%d) at %p",
            to_StateText(FinalConsolidatedInfo.state),
            FinalConsolidatedInfo.state,
            &FinalConsolidatedInfo);

    struct {
        void * pFinalConsolidatedInfo;
    } pp = { &FinalConsolidatedInfo };

    // Just transfer 'pointer' to handle function
    le_event_Report(pm.evt_ConsolidatedAckInfo,
                    &pp,
                    sizeof(&FinalConsolidatedInfo));
}

void taf_PM::Handle_evt_RestartTimerExpired
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("RESTART timer timeout, raises the ALL_ACKED evt");

    taf_pm_State_t internalState = TAF_PM_STATE_ALL_ACKED;
    le_event_Report(pm.evt_StateChangedAckNeeded,
                    &internalState,
                    sizeof(internalState));
}

void taf_PM::Handle_evt_ConsolidatedAckInfo
(
    void * reportPtr
)
{
    FinalConsolidatedInfo_t * info = *( FinalConsolidatedInfo_t **) reportPtr;

    le_ref_IterRef_t it =
        le_ref_GetIterator(pm.refmap_ConsolidatedAckInfo);

    le_result_t rst = le_ref_NextNode(it);
    while (rst == LE_OK)
    {
        ApiHandler_t* hdlr = (ApiHandler_t*) le_ref_GetValue(it);
        LE_INFO("Check: from %p to %p", &FinalConsolidatedInfo, info);
        if (hdlr->handler)
        {
            ((taf_pm_ConsolidatedAckInfoHandlerFunc_t)
                hdlr->handler)(
                    nullptr,
                    info->isAllAcked,
                    info->state,
                    hdlr->context);
        }

        rst = le_ref_NextNode(it);
    }
}

void taf_PM::Handle_evt_StateChanged
(
    void * reportPtr
)
{
    taf_pm_State_t state = * (taf_pm_State_t *) reportPtr;

    LE_INFO("ev(StateChanged): %s is coming", to_StateText(state));

    le_ref_IterRef_t it =
        le_ref_GetIterator(pm.refmap_StateChanged);

    le_result_t rst = le_ref_NextNode(it);

    while (rst == LE_OK)
    {
        ApiHandler_t* hdlr = (ApiHandler_t*) le_ref_GetValue(it);

        if (hdlr->handler)
        {
            ((taf_pm_StateChangeHandlerFunc_t) hdlr->handler)(
                state,
                hdlr->context);
        }

        rst = le_ref_NextNode(it);
    }
}

/**
 * Note: Triggered by SDK indication and Internal Report-action
 */
void taf_PM::Handle_evt_StateChangedAckNeeded
(
    void * reportPtr
)
{
    taf_pm_State_t reportState = *(taf_pm_State_t*)reportPtr;

    LE_INFO("ev(StateChanged): %s is coming", to_StateText(reportState));

    le_ref_IterRef_t it = le_ref_GetIterator(pm.refmap_StateChangedAckNeeded);

    le_result_t rst = le_ref_NextNode(it);

    if (rst != LE_OK)
    {
        LE_INFO("No client registered for /StateChangeEx evt handler");
        SendAckToPaLayer(reportState, taf_pa_pms_ACK);

        return;
    }

    // Checking if the waiting feedback is going on
    if (AckCollection.safeRef != nullptr)
    {
        // Delete the stored ref in the map
        le_ref_DeleteRef(
            pm.refmap_AckNeededClients,
            AckCollection.safeRef);
    }

    // Reset the AckCollection for new indication
    AckCollection.safeRef = nullptr;
    AckCollection.hasNack = false;
    AckCollection.state = reportState;
    AckCollection.nAlreadySent = 0;
    AckCollection.nAlreadyAcked = 0;

    // Notify all registered handler, then check in 'SendStateChangeAck'
    do {

        ApiHandler_t * apiHandler = (ApiHandler_t *) le_ref_GetValue(it);

        if (apiHandler->handler != nullptr)
        {
            if (reportState == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED
            ||  reportState == TAF_PM_STATE_ALL_ACKED)
            {
                if (apiHandler->ismpm != true)
                {
                    LE_WARN("Recorded API handle is [NOT] MPMS, skip");

                    rst = le_ref_NextNode(it);

                    continue;
                }
                else
                {
                    LE_INFO("Recorded API handler from [MPMS]");
                }
            }

            LE_INFO("Transfer ev(StateChangeEx) to client");

            // Fill the valid safe reference
            AckCollection.safeRef =
                le_ref_CreateRef(
                    pm.refmap_AckNeededClients,
                    &AckCollection);

            // Increase the counter for SENT
            ++ AckCollection.nAlreadySent;

            // Invoke the registered handler
            ((taf_pm_StateChangeExHandlerFunc_t)
                apiHandler->handler)(
                    (taf_pm_PowerStateRef_t) AckCollection.safeRef,
                    TAF_PM_PVM,
                    reportState,
                    apiHandler->context);
        }

        rst = le_ref_NextNode(it);

    } while (rst == LE_OK);
}

void taf_PM::PaHandler_evt_WakeupInfo
(
    void * payload
)
{
    taf_pm_NodeModemWsBitMask_t bitset =
        * (taf_pm_NodeModemWsBitMask_t *) payload;

    le_ref_IterRef_t it =
        le_ref_GetIterator(pm.refmap_WakeupInfo);

    le_result_t rst = le_ref_NextNode(it);

    pm.lastModemWsReason = bitset;

    while (rst == LE_OK)
    {
        ApiHandler_t* hdlr = (ApiHandler_t*) le_ref_GetValue(it);

        if (hdlr->handler)
        {
            ((taf_pm_ModemAwakeHandlerFunc_t)
                hdlr->handler)
                    (nullptr, bitset, hdlr->context);
        }

        rst = le_ref_NextNode(it);
    }
}

void taf_PM::AllocateResource()
{
    // Create table of safe references
    refmap_WakeSrc = le_ref_CreateMap("ws-refs", TAF_PM_REFERENCE_DEFAULT_POOL_SIZE);
    LE_ASSERT(refmap_WakeSrc != nullptr);

    // Create memory pool for wakeup source records - exits on error
    pool_WakeSrc = le_mem_CreatePool("ws-pool", sizeof(WakeSrc_t));

    // Create table of wakeup sources
    hmap_WakeSrc = le_hashmap_Create("ws-map",
                                    TAF_WAKEUP_SOURCE_DEFAULT_POOL_SIZE,
                                    le_hashmap_HashString,
                                    le_hashmap_EqualsString);
    LE_ASSERT(hmap_WakeSrc != nullptr);

    // Create memory pool for client records - exits on error
    pool_Client = le_mem_CreatePool("cl-pool", sizeof(Client_t));

    // Create table of clients
    hmap_Client = le_hashmap_Create("cl-map",
                                   TAF_PM_CLIENT_DEFAULT_HASH_SIZE,
                                   le_hashmap_HashVoidPointer,
                                   le_hashmap_EqualsVoidPointer);
    LE_ASSERT(hmap_Client != nullptr);

    pool_ApiHandler = le_mem_CreatePool("api-handler", sizeof(ApiHandler_t));
    pool_VmInfo = le_mem_CreatePool("vm-info", sizeof(VmInfo_t));
    pool_VmList = le_mem_CreatePool("vm-list", sizeof(VmList_t));

    refmap_AckNeededClients = le_ref_CreateMap("ack-needed-clients", 1);

    refmap_ConsolidatedAckInfo =
        le_ref_CreateMap(
            "consolidated-ack",
            DEFAULT_SUPPORTED_REG_HANDLER_NUM);
    evt_ConsolidatedAckInfo =
        le_event_CreateId(
            "consolidated-ack",
            sizeof(FinalConsolidatedInfo_t *));
    le_event_AddHandler(
        "consolidated-ack",
        evt_ConsolidatedAckInfo,
        Handle_evt_ConsolidatedAckInfo);

    refmap_StateChanged =
        le_ref_CreateMap(
            "state-changed",
            DEFAULT_SUPPORTED_REG_HANDLER_NUM);
    evt_StateChanged =
        le_event_CreateId(
            "state-changed",
            sizeof(taf_pm_State_t));
    le_event_AddHandler(
        "state-changed",
        evt_StateChanged,
        Handle_evt_StateChanged);

    refmap_StateChangedAckNeeded =
        le_ref_CreateMap(
            "state-changed-ack",
            DEFAULT_SUPPORTED_REG_HANDLER_NUM);
    evt_StateChangedAckNeeded =
        le_event_CreateId(
            "state-changed-ack",
            sizeof(taf_pm_State_t));
    le_event_AddHandler(
        "state-changed-ack",
        evt_StateChangedAckNeeded,
        Handle_evt_StateChangedAckNeeded);

    refmap_WakeupInfo =
        le_ref_CreateMap(
            "wakeup-info-m",
            DEFAULT_SUPPORTED_REG_HANDLER_NUM);

    refmap_VmList = le_ref_CreateMap("vm-list", DEFAULT_VM_LIST_SIZE);

    // By default, this timer is once
    ref_RestartTimer = le_timer_Create("restart-timer");
    le_timer_SetWakeup(ref_RestartTimer, false);
    le_timer_SetMsInterval(ref_RestartTimer, PMS_CLNTS_ACK_TIMEOUT);
    le_timer_SetHandler(ref_RestartTimer, Handle_evt_RestartTimerExpired);

    // Register client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(
        taf_pm_GetServiceRef(),
        taf_PM::OnClientConnected,
        nullptr);
    le_msg_AddServiceCloseHandler(
        taf_pm_GetServiceRef(),
        taf_PM::OnClientDisconnected,
        nullptr);

    // Re-register the 'sig-terminate(15)' to do some cleanup
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, taf_PM::Handle_sig_SIGTERM);

    // Set the initial state to 'RESUME'
    pm.SetCurrentState(TAF_PM_STATE_RESUME);
}

void taf_PM::Handle_sig_SIGTERM
(
    int sig
)
{
    LE_INFO("Captured sig(SIGTERM) <--");

    // Resume in SA525M before service termination as master app is terminating
    if (pm.GetCurrentState() != TAF_PM_STATE_RESUME)
    {
        pa_result_t rst =
            taf_pa_pms_SetPowerStateAsMaster(
                pm.pa,
                to_PaPowerState(TAF_PM_STATE_RESUME),
                "ALL_MACHINES");

        if (PA_OK != rst)
        {
            LE_ERROR("Failed to invoke PA:SetPowerStateAsMaster error: %d", rst);
        }
        else
        {
            LE_INFO("Leaving ... set PA to RESUME once");
            pm.SetCurrentState(TAF_PM_STATE_RESUME);
        }
    }

    le_event_RemoveHandler(pm.ref_PaEventHandler);

    taf_pa_pms_Deinit(&pm.pa);

    exit(EXIT_SUCCESS);
}

void taf_PM::PaHandler_evt_ServiceAvailable
(
    void * payload
)
{
    taf_pa_pms_ServiceStatus_t status = *(taf_pa_pms_ServiceStatus_t *) payload;

    if (status == SVC_AVAILABLE)
    {
        LE_INFO("PA svc is available");
    }
    else
    {
        LE_WARN("PA svc is unavailable");
    }
}

void taf_PM::PaHandler_evt_PowerStateUpdate
(
    void * payload
)
{
    taf_pa_pms_PowerUpdateEvent_t * evp = (taf_pa_pms_PowerUpdateEvent_t *)payload;

    taf_pm_State_t state = from_PaPowerState(evp->state);

    LE_INFO("Machine [%s] state update: %s",
            evp->machineName,
            to_StateText(state));

    if (state == TAF_PM_STATE_UNKNOWN)
    {
        LE_ERROR("Captured [unknown] state, do NOT raise event");
        return;
    }

    le_event_Report(pm.evt_StateChanged, &state, sizeof(state));
    le_event_Report(pm.evt_StateChangedAckNeeded, &state, sizeof(state));
}

void taf_PM::PaHandler_evt_MachineUpdate
(
    void * payload
)
{
    taf_pa_pms_MachineUpdateEvent_t * evp = (taf_pa_pms_MachineUpdateEvent_t *) payload;

    if (evp->machineEvent == MACHINE_AVAILABLE)
    {
        LE_INFO("Machine [%s] is available", evp->machineName);
    }
    else // MACHINE_UNAVAILABLE
    {
        LE_WARN("Machine [%s] is unavailable", evp->machineName);
    }
}

void * taf_PM::AllocPaEvtPayload
(
    taf_pa_pms_Event_t * ev
)
{
    void * newPayload = malloc(ev->evPsize);
    LE_ASSERT(newPayload != NULL);
    memcpy(newPayload, ev->evPayload, ev->evPsize);
    return newPayload;
}

void taf_PM::FreePaEvtPayload
(
    taf_pa_pms_Event_t * ev
)
{
    if (ev)
    {
        if (ev->evPayload)
        {
            free(ev->evPayload);
            ev->evPayload = NULL;
        }
    }
}

void taf_PM::PaIndication_Handler
(
    void * reportPtr
)
{
    taf_pa_pms_Event_t * evp = (taf_pa_pms_Event_t *) reportPtr;

    LE_INFO("PA Event: [%d] captured", evp->evType);

    switch(evp->evType)
    {
        case EV_WAKEUP_INFO:
        {
            PaHandler_evt_WakeupInfo(evp->evPayload);
        }
        break;

        case EV_SVC_STATUS:
        {
            PaHandler_evt_ServiceAvailable(evp->evPayload);
        }
        break;

        case EV_POWER_STATE_UPDATE:
        {
            PaHandler_evt_PowerStateUpdate(evp->evPayload);
        }
        break;

        case EV_MACHINE_UPDATE:
        {
            PaHandler_evt_MachineUpdate(evp->evPayload);
        }
        break;

        case EV_CONSOLIDATED_INFO:
        {
            PaHandler_evt_ConsolidatedInfo(evp->evPayload);
        }
        break;

        default:
        {
            LE_ERROR("Unknown event type was captured: %d", evp->evType);
        }
    }

    LE_INFO("Pa Event Handled: [%d]", evp->evType);

    FreePaEvtPayload(evp);
}

// Function-Call by the PA layer
void taf_PM::PaEventReportCallback
(
    taf_pa_pms_Event_t * ev
)
{
    taf_pa_pms_Event_t newEv;
    newEv.evType = ev->evType;
    newEv.evPayload = AllocPaEvtPayload(ev);
    newEv.evPsize = ev->evPsize;

    le_event_Report(pm.evt_PaInd, &newEv, sizeof(newEv));
}

void taf_PM::TryToInitPaLayer()
{
    // Create event-id to handle PA indications
    pm.evt_PaInd = le_event_CreateId("pms-pa-evt", sizeof(taf_pa_pms_Event_t));
    pm.ref_PaEventHandler =
        le_event_AddHandler(
            "pms-pa-evt-hdlr",
            pm.evt_PaInd,
            PaIndication_Handler);

    // Also register the logger and event-reporter
    pa_result_t rst =
        taf_pa_pms_Init(
            &pm.pa,
            PaEventReportCallback,
            PA_LAYER_TIMEOUT_MAX_MS);

    if (PA_OK == rst)
    {
        LE_INFO("Pa Init Done");
    }
    else if (PA_TIMEOUT == rst)
    {
        // Retry ? No, we don't know where is stoped, so..
        LE_FATAL("Pa Init Timeout");
    }
    else // Others: e.g. PaResult(FAULT)
    {
        LE_FATAL("Pa Init Failed");
    }
}

COMPONENT_INIT
{
    /* First, Assign the basic resource for current service */
    pm.AllocateResource();

    /* Try to initialize the Pa layer */
    pm.TryToInitPaLayer();
}

/* --------------------------------------------------------------------------------------*/
/* -------------------------------------- API (BEGIN) -----------------------------------*/
/* --------------------------------------------------------------------------------------*/

/**
 * FUNCTION: taf_pm_NewWakeupSource
 */
taf_pm_WakeupSourceRef_t API(NewWakeupSource)
(
    uint32_t options,
    const char *tag
)
{
    WakeSrc_t *pWakeSrc;
    Client_t  *pClient;
    char wsName[TAF_WS_NAME_LEN];

    if (('\0' == *tag) || (strlen(tag) > TAF_PM_TAG_LEN))
    {
        LE_ERROR("Err: invalid tag value");
        return nullptr;
    }

    // validate client record
    pClient = (Client_t *) le_hashmap_Get(pm.hmap_Client,
                                       taf_pm_GetClientSessionRef());

    if (pClient == nullptr)
    {
        LE_ERROR("Err: client not found");
        return nullptr;
    }

    // Check if identical wakeup source already exists for this client
    snprintf(wsName, sizeof(wsName), TAF_WS_NAME_FORMAT, tag, pClient->sessionRef);

    if (le_hashmap_ContainsKey(pm.hmap_WakeSrc, wsName))
    {
        LE_ERROR("Err: tag '%s' already exists", tag);
        return nullptr;
    }

    // Allocate and populate wakeup source record and exit on error
    pWakeSrc = (WakeSrc_t*)le_mem_ForceAlloc(pm.pool_WakeSrc);
    memset(pWakeSrc, 0, sizeof(WakeSrc_t));
    pWakeSrc->acquired = 0;
    pWakeSrc->pid = pClient->pid;
    le_utf8_Copy(pWakeSrc->name, wsName, sizeof(pWakeSrc->name), nullptr);
    pWakeSrc->isRef = (options & TAF_PM_REF_COUNT ? true : false);
    pWakeSrc->wsRef = le_ref_CreateRef(pm.refmap_WakeSrc, pWakeSrc);

    // store in table of wakeup sources
    if (le_hashmap_Put(pm.hmap_WakeSrc, pWakeSrc->name, pWakeSrc))
    {
        LE_FATAL("Fatal: failed to add wakeup source '%s'",
                 pWakeSrc->name);
    }

    LE_INFO("Created new wakeup source '%s' for pid (%d)",
            pWakeSrc->name, pWakeSrc->pid);

    LE_INFO("Audit all wakeup source info:");
    le_ref_IterRef_t it = le_ref_GetIterator(pm.refmap_WakeSrc);
    while (LE_OK == le_ref_NextNode(it))
    {
        WakeSrc_t * ws = (WakeSrc_t *) le_ref_GetValue(it);
        LE_INFO("> ws: [%s](%d), acquired: [%d], ref: [%d]",
                ws->name,
                ws->pid,
                ws->acquired,
                ws->isRef);
    }

    return (taf_pm_WakeupSourceRef_t) pWakeSrc->wsRef;
}

/**
 * FUNCTION: taf_pm_StayAwake
 */
le_result_t API(StayAwake)
(
    taf_pm_WakeupSourceRef_t wsRef
)
{
    WakeSrc_t *ws;

    ws = to_WakeSrc_t(wsRef);
    if (ws == nullptr)
    {
        LE_ERROR("Invalid Wakeup source reference");
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Operating [ws]: %s", ws->name);

    // For low power mode, only MPMS is permitted
    if(pm.GetPowerMode() == TAF_PM_POWER_MODE_LOW_POWER)
    {
        LE_INFO("Only 'MPMS' is allowed for state changing");

        if(!IsClientMPMS( taf_pm_GetClientSessionRef() ))
        {
            LE_INFO("Client is not 'MPMS'");
            return LE_NOT_PERMITTED;
        }
    }

    if (ws->isRef == false)
    {
        if (ws->acquired != 0)
        {
            LE_WARN("Wakeup source '%s' already acquired.", ws->name);
            pm.ShowWakeSourceInfo(__FUNCTION__);
            return LE_OK;
        }
        else // ws->acquired == 0
        {
            ++ ws->acquired;
        }
    }
    else // ws->isRef == true
    {
        ++ ws->acquired;

        if (ws->acquired == 0)
        {
            -- ws->acquired;
            // It almost never happens—except during tests.
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overflowed", ws->name);
            return LE_FAULT;
        }
    }

    // Increase the counter for total wakeup source
    ++ pm.totalWakeupSrc;
    if (pm.totalWakeupSrc == 0)
    {
        -- pm.totalWakeupSrc;
        LE_KILL_CLIENT("Total recorded wakeup source overflowed");
        return LE_FAULT;
    }

    pm.ShowWakeSourceInfo(__FUNCTION__);

    if (pm.GetCurrentState() != TAF_PM_STATE_RESUME)
    {
        le_result_t rst = pm.SetPowerState(TAF_PM_STATE_RESUME, "ALL_MACHINES");
        if (rst != LE_OK)
        {
            LE_ERROR("Failed to SetPowerState from non-RESUME to RESUME");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_Relax
 */
le_result_t API(Relax)
(
    taf_pm_WakeupSourceRef_t wsRef
)
{
    WakeSrc_t * ws = to_WakeSrc_t(wsRef);

    if (ws == nullptr)
    {
        LE_ERROR("Invalid wsRef");
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Operating [ws]: %s", ws->name);

    if (ws->acquired == 0)
    {
        LE_INFO("Wakeup source '%s' already released", ws->name);
        pm.ShowWakeSourceInfo(__FUNCTION__);
        return LE_OK;
    }

    ws->acquired --;
    pm.totalWakeupSrc --;

    if (ws->isRef == true)
    {
        if (ws->acquired > 0)
        {
            pm.ShowWakeSourceInfo(__FUNCTION__);
            return LE_OK;
        }
    }
    else // ws->isRef == false
    {
        // NOTE: (see OnClientDisconnected)
        // When the client disconnects, all wakesources are set to non-referenced,
        // then the counter for the refenced wake sources should be decreased from total.
        if (ws->acquired > 0)
        {
            pm.totalWakeupSrc -= ws->acquired;
        }

        ws->acquired = 0;
    }

    pm.ShowWakeSourceInfo(__FUNCTION__);

    // If all wakeup locks were released, switch to SUSPEND
    if (pm.totalWakeupSrc == 0)
    {
        LE_INFO("Trigger [%s] when [total] wakeup source is 0",
                to_StateText(TAF_PM_STATE_ALL_WAKELOCKS_RELEASED));

        taf_pm_State_t internalState = TAF_PM_STATE_ALL_WAKELOCKS_RELEASED;
        le_event_Report(
                pm.evt_StateChangedAckNeeded,
                &internalState,
                sizeof(internalState));
    }

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_SetAllVMPowerState
 */
le_result_t API(SetAllVMPowerState)
(
    taf_pm_State_t state
)
{
    return pm.SetPowerState(state, "ALL_MACHINES");
}

/**
 * FUNCTION: taf_pm_SetVMPowerState
 */
le_result_t API(SetVMPowerState)
(
    taf_pm_State_t state,
    const char *machineName
)
{
    return pm.SetPowerState(state, machineName);
}

/**
 * FUNCTION: taf_pm_GetPowerState
 */
taf_pm_State_t API(GetPowerState)
(
    void
)
{
    return pm.GetCurrentState();
}

/**
 * FUNCTION: taf_pm_AddConsolidatedAckInfoHandler
 */
taf_pm_ConsolidatedAckInfoHandlerRef_t API(AddConsolidatedAckInfoHandler)
(
    taf_pm_ConsolidatedAckInfoHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t *)
            le_mem_ForceAlloc(pm.pool_ApiHandler);

    memset(hdlr, 0x00, sizeof(*hdlr));

    hdlr->handler = (void *)handlerPtr;
    hdlr->context = contextPtr;
    hdlr->session = taf_pm_GetClientSessionRef();
    hdlr->safeRef = le_ref_CreateRef(pm.refmap_ConsolidatedAckInfo, hdlr);
    hdlr->ismpm = false; // Don't care

    pm.ShowCurrentClientSessionInfo(__FUNCTION__, "register");

    return (taf_pm_ConsolidatedAckInfoHandlerRef_t) hdlr->safeRef;
}

/**
 * FUNCTION: taf_pm_RemoveConsolidatedAckInfoHandler
 */
void API(RemoveConsolidatedAckInfoHandler)
(
    taf_pm_ConsolidatedAckInfoHandlerRef_t handlerRef
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t * )
            le_ref_Lookup(pm.refmap_ConsolidatedAckInfo, handlerRef);

    if (hdlr != nullptr)
    {
        le_ref_DeleteRef(pm.refmap_ConsolidatedAckInfo, handlerRef);
        le_mem_Release(hdlr);

        pm.ShowCurrentClientSessionInfo(__FUNCTION__, "deregister");
    }
}

/**
 * FUNCTION: taf_pm_GetNackClientInfo
 */
le_result_t API(GetNackClientInfo)
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
    taf_pm_ClientInfo_t* nackClientsPtr,
    size_t* nackClientsSizePtr
)
{
    LE_UNUSED(consolidatedAckInfoRef);

    if (nullptr == nackClientsPtr)
    {
        LE_ERROR("Bad nackClientsPtr");
        return LE_BAD_PARAMETER;
    }

    if (nullptr == nackClientsSizePtr
    ||  *nackClientsSizePtr > TAF_PM_MAX_CLIENT_NUMBER
    ||  *nackClientsSizePtr == 0)
    {
        LE_ERROR("Bad nackClientsSizePtr");
        return LE_BAD_PARAMETER;
    }

    uint32_t trimedSize = 0;

    LE_INFO("Total record [nack] client is: %d",
            FinalConsolidatedInfo.info.nackResponseClntSize);

    LE_INFO("API buffer size: %ld", *nackClientsSizePtr);

    if (FinalConsolidatedInfo.info.nackResponseClntSize
    >   *nackClientsSizePtr)
    {
        LE_WARN("API client buffer is NOT enough, (%d > %ld)",
                FinalConsolidatedInfo.info.nackResponseClntSize,
                *nackClientsSizePtr);

        trimedSize = *nackClientsSizePtr;
    }
    else
    {
        trimedSize = FinalConsolidatedInfo.info.nackResponseClntSize;
    }

    for (uint32_t i = 0; i < trimedSize; i++)
    {
        LE_INFO("info[nack] -> client: %s, machine: %s",
                FinalConsolidatedInfo.info.nackResponseClntData[i].clientName,
                FinalConsolidatedInfo.info.nackResponseClntData[i].machineName);
    }

    for(uint32_t i = 0; i < trimedSize; i++)
    {
        le_utf8_Copy(
            nackClientsPtr[i].clientName,
            FinalConsolidatedInfo.info.nackResponseClntData[i].clientName,
            sizeof(FinalConsolidatedInfo.info.nackResponseClntData[i].clientName),
            NULL);
        le_utf8_Copy(
            nackClientsPtr[i].machineName,
            FinalConsolidatedInfo.info.nackResponseClntData[i].machineName,
            sizeof(FinalConsolidatedInfo.info.nackResponseClntData[i].machineName),
            NULL);
    }

    *nackClientsSizePtr = trimedSize;

    LE_INFO("Total (report to client) [nack] is %d", trimedSize);

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_GetUnrespClientInfo
 */
le_result_t API(GetUnrespClientInfo)
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
    taf_pm_ClientInfo_t* unrespClientsPtr,
    size_t* unrespClientsSizePtr
)
{
    LE_UNUSED(consolidatedAckInfoRef);

    if (nullptr == unrespClientsPtr)
    {
        LE_ERROR("Bad unrespClientsPtr");
        return LE_BAD_PARAMETER;
    }

    if (nullptr == unrespClientsSizePtr
    || *unrespClientsSizePtr > TAF_PM_MAX_CLIENT_NUMBER
    || *unrespClientsSizePtr == 0)
    {
        LE_ERROR("Bad unrespClientsSizePtr");
        return LE_BAD_PARAMETER;
    }

    uint32_t trimedSize = 0;

    LE_INFO("API buffer size: %ld", *unrespClientsSizePtr);
    LE_INFO("Stored info size: %d", FinalConsolidatedInfo.info.unresponsiveClntSize);

    if (FinalConsolidatedInfo.info.unresponsiveClntSize
    >   *unrespClientsSizePtr)
    {
        LE_WARN("API client buffer is NOT enough, (%d > %ld)",
                FinalConsolidatedInfo.info.unresponsiveClntSize,
                *unrespClientsSizePtr);

        trimedSize = *unrespClientsSizePtr;
    }
    else
    {
        trimedSize = FinalConsolidatedInfo.info.unresponsiveClntSize;
    }


    for (uint32_t i = 0; i < trimedSize; i++)
    {
        LE_INFO("[unresp] info -> client: %s, machine: %s",
                FinalConsolidatedInfo.info.unresponsiveClntData[i].clientName,
                FinalConsolidatedInfo.info.unresponsiveClntData[i].machineName);
    }

    for(uint32_t i = 0; i < trimedSize; i++)
    {
        le_utf8_Copy(
            unrespClientsPtr[i].clientName,
            FinalConsolidatedInfo.info.unresponsiveClntData[i].clientName,
            sizeof(FinalConsolidatedInfo.info.unresponsiveClntData[i].clientName),
            NULL);
        le_utf8_Copy(
            unrespClientsPtr[i].machineName,
            FinalConsolidatedInfo.info.unresponsiveClntData[i].machineName,
            sizeof(FinalConsolidatedInfo.info.unresponsiveClntData[i].machineName),
            NULL);
    }

    *unrespClientsSizePtr = trimedSize;

    LE_INFO("Total (report to client) [unresp] is %d", trimedSize);

    return LE_OK;
}


/**
 * FUNCTION: taf_pm_SetPowerMode
 */
le_result_t API(SetPowerMode)
(
    taf_pm_PowerMode_t powerMode
)
{
    if (powerMode != TAF_PM_POWER_MODE_LOW_POWER
    &&  powerMode != TAF_PM_POWER_MODE_NORMAL)
    {
        LE_ERROR("Bad powerMode");
        return LE_BAD_PARAMETER;
    }

    pm.SetPowerMode(powerMode);

    return LE_OK;
}

/**
* FUNCTION: taf_pm_AddStateChangeHandler
*/
taf_pm_StateChangeHandlerRef_t API(AddStateChangeHandler)
(
    taf_pm_StateChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t *)
            le_mem_ForceAlloc(pm.pool_ApiHandler);

    memset(hdlr, 0x00, sizeof(*hdlr));

    hdlr->handler = (void *)handlerPtr;
    hdlr->context = contextPtr;
    hdlr->session = taf_pm_GetClientSessionRef();
    hdlr->safeRef = le_ref_CreateRef(pm.refmap_StateChanged, hdlr);
    hdlr->ismpm = false; // Don't care

    pm.ShowCurrentClientSessionInfo(__FUNCTION__, "register");

    return (taf_pm_StateChangeHandlerRef_t) hdlr->safeRef;
}

/**
* FUNCTION: taf_pm_RemoveStateChangeHandler
*/
void API(RemoveStateChangeHandler)
(
    taf_pm_StateChangeHandlerRef_t handlerRef
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t * )
            le_ref_Lookup(pm.refmap_StateChanged, handlerRef);

    if (hdlr != nullptr)
    {
        le_ref_DeleteRef(pm.refmap_StateChanged, handlerRef);
        le_mem_Release(hdlr);

        pm.ShowCurrentClientSessionInfo(__FUNCTION__, "deregister");
    }
}

/**
 * FUNCTION:  taf_pm_AddStateChangeExHandler
 */
taf_pm_StateChangeExHandlerRef_t API(AddStateChangeExHandler)
(
    taf_pm_StateChangeExHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    Client_t * pClient =
        (Client_t *) le_hashmap_Get(pm.hmap_Client,
                                    taf_pm_GetClientSessionRef());

    if (pClient == nullptr)
    {
        LE_ERROR("Why connected client is not registered in map?");
        return nullptr;
    }

    ApiHandler_t * hdlr =
        (ApiHandler_t *)
            le_mem_ForceAlloc(pm.pool_ApiHandler);

    memset(hdlr, 0x00, sizeof(*hdlr));

    if (strncmp(pClient->name, TAF_MNGD_PM_SVC, sizeof(TAF_MNGD_PM_SVC)) == 0
    ||  strncmp(pClient->name, TAF_RPC_PROXY, sizeof(TAF_RPC_PROXY)) == 0
    ||  strncmp(pClient->name, TAF_PM_UNIT_TEST_APP, sizeof(TAF_PM_UNIT_TEST_APP)) == 0
    ||  strncmp(pClient->name, TAF_PM_INTG_TEST_APP, sizeof(TAF_PM_INTG_TEST_APP)) == 0)
    {
        LE_INFO("mpm client is registered: %s", pClient->name);
        hdlr->ismpm = true;
    }
    else
    {
        hdlr->ismpm = false;
    }

    hdlr->handler = (void *) handlerPtr;
    hdlr->context = contextPtr;
    hdlr->session = taf_pm_GetClientSessionRef();
    hdlr->safeRef =
        (taf_pm_StateChangeExHandlerRef_t)
            le_ref_CreateRef(pm.refmap_StateChangedAckNeeded, hdlr);

    pm.ShowCurrentClientSessionInfo(__FUNCTION__, "register");

    return (taf_pm_StateChangeExHandlerRef_t) hdlr->safeRef;
}

/**
 * FUNCTION: taf_pm_RemoveStateChangeExHandler
 */
void API(RemoveStateChangeExHandler)
(
    taf_pm_StateChangeExHandlerRef_t handlerRef
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t * )
            le_ref_Lookup(pm.refmap_StateChangedAckNeeded, handlerRef);

    if (hdlr != nullptr)
    {
        le_ref_DeleteRef(pm.refmap_StateChangedAckNeeded, handlerRef);
        le_mem_Release(hdlr);

        pm.ShowCurrentClientSessionInfo(__FUNCTION__, "deregister");
    }
}

/**
 * FUNCTION: taf_pm_SendStateChangeAck
 */
void API(SendStateChangeAck)
(
    taf_pm_PowerStateRef_t powerStateRef,
    taf_pm_State_t state,
    taf_pm_NadVm_t vmId,
    taf_pm_ClientAck_t ackType
)
{
    Client_t * pClient =
        (Client_t *) le_hashmap_Get(pm.hmap_Client,
                                    taf_pm_GetClientSessionRef());
    if (pClient)
    {
        LE_INFO("ACK-ing from client: %s", pClient->name);
    }
    else
    {
        LE_ERROR("Never be here, why?");
    }

    AckCollection_t * collection =
        (AckCollection_t *)
            le_ref_Lookup(pm.refmap_AckNeededClients, powerStateRef);

    if (collection == nullptr)
    {
        LE_ERROR("Invalid powerStateRef");
        return;
    }

    if (state != collection->state)
    {
        LE_ERROR("Invalid state, mismatched");
        return;
    }

    // For now, only support PVM node
    if (vmId != TAF_PM_PVM)
    {
        LE_ERROR("Invalid vmId");
        return;
    }

    if (ackType != TAF_PM_READY
    &&  ackType != TAF_PM_NOT_READY)
    {
        LE_ERROR("Invalid ackType");
        return;
    }

    if (collection->hasNack)
    {
        LE_INFO("N-ACK exists, drop current ack");
        return;
    }

    // First, capture the internal state, from [mpms]
    // Note: TAF_PM_STATE_ALL_WAKELOCKS_RELEASED can't ack from [mpms]
    if (state == TAF_PM_STATE_ALL_ACKED)
    {
        if (pm.GetCurrentState() == TAF_PM_STATE_RESTART)
        {
            LE_INFO("Try to reboot system by supervisor");
            le_result_t rst = le_framework_Reboot();
            if (rst != LE_OK)
            {
                LE_ERROR("Failed to le_framework_Reboot: %s", LE_RESULT_TXT(rst));
            }
            else
            {
                LE_INFO("Successfully triggered, waiting system reboot ...");
            }
        }
        else
        {
            pm.SendAckToPaLayer(pm.GetCurrentState(), taf_pa_pms_ACK);
        }

        return;
    }
    // Check with the current state
    else if (pm.GetCurrentState() == state)
    {
        LE_INFO("Current state is: %s, during send-ack process",
                to_StateText(state));

        if (ackType == TAF_PM_NOT_READY)
        {
            LE_INFO("Got N-ACK");

            if (TAF_PM_STATE_RESTART == state)
            {
                LE_INFO("Expects the pmClientsAckTimer timeout");
            }
            else
            {
                collection->hasNack = true;

                pm.SendAckToPaLayer(pm.GetCurrentState(), taf_pa_pms_NACK);
            }

            return;
        }
        else // Got ACK, counter it
        {
            LE_INFO("Got ACK, ++ counter, checking total..");

            ++ collection->nAlreadyAcked;

            LE_INFO("Audit [ACKed] info, sent: %d, acked: %d",
                    collection->nAlreadySent,
                    collection->nAlreadyAcked);

            if (collection->nAlreadyAcked == collection->nAlreadySent)
            {
                LE_INFO("Good, all clients ACKed");

                // For 'RESTART' state, cancel the timer
                if(le_timer_IsRunning(pm.ref_RestartTimer))
                {
                    LE_DEBUG("Stop the pmClientsAckTimer");
                    le_timer_Stop(pm.ref_RestartTimer);
                }

                taf_pm_State_t internalState = TAF_PM_STATE_ALL_ACKED;
                le_event_Report(pm.evt_StateChangedAckNeeded,
                                &internalState,
                                sizeof(internalState));
            }
            else
            {
                LE_INFO("Continue waiting for remaining clients: %d ACK",
                        (collection->nAlreadySent - collection->nAlreadyAcked) );
            }

            return;
        }
    }
}

/**
 * FUNCTION: taf_pm_GetMachineList
 */
taf_pm_VMListRef_t API(GetMachineList)
(
    void
)
{
    std::vector<std::string> machineNames;

    pa_result_t rst = taf_pa_pms_GetAllMachineNames(pm.pa, machineNames);
    if (PA_OK != rst)
    {
        LE_ERROR("Failed to GetAllMachineNames from PA Layer");
        return nullptr;
    }

    VmList_t *vmList = (VmList_t *) le_mem_ForceAlloc(pm.pool_VmList);
    memset(vmList, 0x00, sizeof(*vmList));

    vmList->list = LE_DLS_LIST_INIT;
    vmList->current = nullptr;
    vmList->session = taf_pm_GetClientSessionRef();

    for (size_t i = 0; i < machineNames.size(); i++)
    {
        VmInfo_t * vmInfo = (VmInfo_t *) le_mem_ForceAlloc(pm.pool_VmInfo);
        memset(vmInfo, 0x00, sizeof(*vmInfo));

        le_utf8_Copy(vmInfo->name, machineNames[i].c_str(),
                     TAF_PM_MACHINE_NAME_LEN, nullptr);

        vmInfo->link = LE_DLS_LINK_INIT;

        le_dls_Queue(&vmList->list, &vmInfo->link);
    }

    vmList->safeRef =
        (taf_pm_VMListRef_t)
            le_ref_CreateRef(pm.refmap_VmList, vmList);

    return vmList->safeRef;
}

/**
 * FUNCTION: taf_pm_DeleteMachineList
 */
le_result_t API(DeleteMachineList)
(
    taf_pm_VMListRef_t vmListRef
)
{
    if (vmListRef == nullptr)
    {
        LE_ERROR("Invalid vmListRef");
        return LE_BAD_PARAMETER;
    }

    VmList_t * vmList = (VmList_t *) le_ref_Lookup(pm.refmap_VmList, vmListRef);

    if (vmList == nullptr)
    {
        LE_ERROR("Not found valid reference");
        return LE_NOT_FOUND;
    }

    le_dls_Link_t * linkPtr = le_dls_Peek(&vmList->list);
    while (linkPtr != nullptr)
    {
        VmInfo_t * vmInfo = CONTAINER_OF(linkPtr, VmInfo_t, link);

        linkPtr = le_dls_PeekNext(&vmList->list, linkPtr);

        // After PeekNext, remove the link from Structure
        le_dls_Remove(&vmList->list, &vmInfo->link);

        le_mem_Release(vmInfo);
    }

    le_ref_DeleteRef(pm.refmap_VmList, vmListRef);

    le_mem_Release(vmList);

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_GetFirstMachineName
 */
le_result_t API(GetFirstMachineName)
(
    taf_pm_VMListRef_t vmListRef,
    char* vmNamePtr,
    size_t vmNamePtrSize
)
{
    VmList_t * vmList = (VmList_t *) le_ref_Lookup(pm.refmap_VmList, vmListRef);
    if (vmList == nullptr)
    {
        LE_ERROR("Invalid vmListRef");
        return LE_BAD_PARAMETER;
    }

    if (vmNamePtr == nullptr)
    {
        LE_ERROR("Invalid vmNamePtr");
        return LE_BAD_PARAMETER;
    }

    if (vmNamePtrSize < TAF_PM_MACHINE_NAME_LEN)
    {
        LE_ERROR("Invalid vmNamePtrSize, expected: >= %d",
                 TAF_PM_MACHINE_NAME_LEN);
        return LE_BAD_PARAMETER;
    }

    // Every time, just get the new iterator
    // it's different from le_dls_PeekNext() in API(GetNextMachineName)
    le_dls_Link_t* linkPtr = le_dls_Peek(&vmList->list);
    if (linkPtr == nullptr)
    {
        LE_INFO("Vm List is empty");
        return LE_NOT_FOUND;
    }

    VmInfo_t * vmInfo = CONTAINER_OF(linkPtr, VmInfo_t, link);
    le_utf8_Copy(vmNamePtr, vmInfo->name, TAF_PM_MACHINE_NAME_LEN, nullptr);

    // Update the cursor to next one
    vmList->current = linkPtr;

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_GetNextMachineName
 */
le_result_t API(GetNextMachineName)
(
    taf_pm_VMListRef_t vmListRef,
    char* vmNamePtr,
    size_t vmNamePtrSize
)
{
    VmList_t * vmList = (VmList_t *) le_ref_Lookup(pm.refmap_VmList, vmListRef);
    if (vmList == nullptr)
    {
        LE_ERROR("Invalid vmListRef");
        return LE_BAD_PARAMETER;
    }

    if (vmNamePtr == nullptr)
    {
        LE_ERROR("Invalid vmNamePtr");
        return LE_BAD_PARAMETER;
    }

    if (vmNamePtrSize < TAF_PM_MACHINE_NAME_LEN)
    {
        LE_ERROR("Invalid vmNamePtrSize, expected: >= %d",
                 TAF_PM_MACHINE_NAME_LEN);
        return LE_BAD_PARAMETER;
    }

    // Try to get next one by '->current' cursor
    le_dls_Link_t* linkPtr = le_dls_PeekNext(&vmList->list, vmList->current);
    if (linkPtr == nullptr)
    {
        LE_INFO("Reached to the end of the list");
        return LE_NOT_FOUND;
    }

    VmInfo_t * vmInfo = CONTAINER_OF(linkPtr, VmInfo_t, link);
    le_utf8_Copy(vmNamePtr, vmInfo->name, TAF_PM_MACHINE_NAME_LEN, nullptr);

    // Must update the cursor to next one
    vmList->current = linkPtr;

    return LE_OK;
}


/**
 * FUNCTION: taf_pm_SetModemWakeupSel
 */
le_result_t API(SetModemWakeupSel)
(
    taf_pm_NodeModemWsBitMask_t wsBitmask
)
{
    pa_result_t rst = taf_pa_pms_SetModemWakeupFilter(pm.pa, wsBitmask);
    if (PA_OK != rst)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_GetModemWakeupSel
 */
le_result_t API(GetModemWakeupSel)
(
    taf_pm_NodeModemWsBitMask_t* wsBitmaskPtr
)
{
    if (wsBitmaskPtr == nullptr)
    {
        LE_ERROR("Invalid wsBitmaskPtr");
        return LE_BAD_PARAMETER;
    }

    pa_result_t rst = taf_pa_pms_GetModemWakeupFilter(pm.pa, wsBitmaskPtr);
    if (PA_OK != rst)
    {
        *wsBitmaskPtr = 0;
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_GetModemAwakeReason
 */
le_result_t API(GetModemAwakeReason)
(
    taf_pm_NodeModemWsBitMask_t* wsBitmaskPtr
)
{
    if(wsBitmaskPtr == nullptr)
    {
        LE_ERROR("Bad wsBitmaskPtr: nullptr");
        return LE_BAD_PARAMETER;
    }

    uint32_t mask = 0;

    mask = (TAF_PM_NODE_MODEM_WS_BIT_MASK_SMS |
            TAF_PM_NODE_MODEM_WS_BIT_MASK_VOICE_CALL |
            TAF_PM_NODE_MODEM_WS_BIT_MASK_REMOTE_SIM_PROFILE_SWAP);

    LE_INFO("mask: 0x%08x, value: 0x%08x [get]",
            mask,
            pm.GetLastModemWsReason() & mask);

    *wsBitmaskPtr =
        (taf_pm_NodeModemWsBitMask_t)
            pm.GetLastModemWsReason() & mask;

    return LE_OK;
}

/**
 * FUNCTION: taf_pm_AddModemAwakeHandler
 */
taf_pm_ModemAwakeHandlerRef_t API(AddModemAwakeHandler)
(
    taf_pm_ModemAwakeHandlerFunc_t handlerPtr,
    void                          *contextPtr,
    taf_pm_NodeModemWsBitMask_t    wsBitmask
)
{
    LE_UNUSED(wsBitmask);

    ApiHandler_t * hdlr =
        (ApiHandler_t *)
            le_mem_ForceAlloc(pm.pool_ApiHandler);

    memset(hdlr, 0x00, sizeof(*hdlr));

    hdlr->handler = (void *)handlerPtr;
    hdlr->context = contextPtr;
    hdlr->session = taf_pm_GetClientSessionRef();
    hdlr->safeRef = le_ref_CreateRef(pm.refmap_WakeupInfo, hdlr);
    hdlr->ismpm = false; // Don't care

    pm.ShowCurrentClientSessionInfo(__FUNCTION__, "register");

    return (taf_pm_ModemAwakeHandlerRef_t) hdlr->safeRef;
}

/**
 * FUNCTION: taf_pm_RemoveModemAwakeHandler
 */
void API(RemoveModemAwakeHandler)
(
    taf_pm_ModemAwakeHandlerRef_t handlerRef
)
{
    ApiHandler_t * hdlr =
        (ApiHandler_t * )
            le_ref_Lookup(pm.refmap_WakeupInfo, handlerRef);

    if (hdlr != nullptr)
    {
        le_ref_DeleteRef(pm.refmap_WakeupInfo, handlerRef);
        le_mem_Release(hdlr);

        pm.ShowCurrentClientSessionInfo(__FUNCTION__, "deregister");
    }
}


/* --------------------------------------------------------------------------------------*/
/* -------------------------------------- API (END) -------------------------------------*/
/* --------------------------------------------------------------------------------------*/
