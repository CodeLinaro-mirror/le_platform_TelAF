/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <future>
#include <unistd.h>
#include "legato.h"
#include "interfaces.h"
#include "tafPM.hpp"

static taf_powerManager_t pm_recrd;
static bool isResumed = false;
LE_REF_DEFINE_STATIC_MAP(tafPMReferences, TAF_PM_REFERENCE_DEFAULT_POOL_SIZE);

/**
 * Type-cast from taf_pm_WakeupSourceRef_t passed from app to taf_ws_t
 */
taf_ws_t *taf_PM::ToTafWakeupSource(taf_pm_WakeupSourceRef_t w)
{
    taf_ws_t *ws = (taf_ws_t *)le_ref_Lookup(pm_recrd.refs, w);

    TAF_KILL_CLIENT_IF_RET_VAL(ws == NULL, NULL, "Invalid wakeup source provided");

    if (TAF_PM_WAKEUP_SOURCE_COOKIE != ws->cookie || ws->wsRef != w)
    {
        LE_FATAL("Error: Not valid wakeup source %p.", w);
    }

    return ws;
}

/**
 * Type-cast from void *(pm client table record pointer) to taf_Client_t
 */
taf_Client_t *taf_PM::to_taf_Client_t(void *c)
{
    taf_Client_t *cl = (taf_Client_t *)c;
    if (!cl || TAF_PM_CLIENT_COOKIE != cl->cookie)
    {
        LE_FATAL("Error: Not valid client %p.", c);
    }

    return cl;
}

/**
 * Class Handler Implementations
 */
taf_Handler::taf_Handler()
{
    return;
}

taf_Handler::~taf_Handler()
{
    return;
}

void taf_Handler::Init()
{
    return;
}

/**
 * Callback on client connection
 */
void taf_Handler::OnClientConnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    taf_Client_t *pClient;
    FILE *procFd;
    char procStr[PATH_MAX];
    size_t procLen;

    // update client record, exits on error
    pClient = (taf_Client_t*)le_mem_ForceAlloc(pm_recrd.clientpool);
    pClient->cookie = TAF_PM_CLIENT_COOKIE;
    pClient->sessionRef = sessionRef;
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pClient->procId))
    {
        LE_FATAL("Error, Failed to get pClient procId.");
    }

    // Opening the pClient process command line
    snprintf(procStr, sizeof(procStr), "/proc/%d/comm", pClient->procId);
    procFd = fopen(procStr, "r");
    if (NULL == procFd)
    {
        LE_FATAL("Failed to open process %d command line: %m", pClient->procId);
    }

    // Get the pClient process name
    if (NULL == fgets(procStr, sizeof(procStr), procFd))
    {
        LE_FATAL("Failed to scan process %d command line", pClient->procId);
    }
    fclose(procFd);
    procLen = strlen(procStr);
    procStr[procLen - 1] = '\0';
    memset(pClient->name, 0, sizeof(pClient->name));
    le_utf8_Copy(pClient->name, procStr, sizeof(pClient->name), NULL);

    // update client record in table
    if (le_hashmap_Put(pm_recrd.clients, sessionRef, pClient))
    {
        LE_FATAL("Failed to add client record for pid %d.", pClient->procId);
    }

    LE_INFO("Client %s/%d connected", pClient->name, pClient->procId);
}

/**
 * Callback on client disconnection
 */
void taf_Handler::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    taf_Client_t *pClient;
    taf_ws_t *ws;
    le_hashmap_It_Ref_t iter;

    // Find and remove powermanager client record from table
    pClient = taf_PM::to_taf_Client_t(le_hashmap_Remove(pm_recrd.clients, sessionRef));

    TAF_ERROR_IF_RET_NIL(pClient == NULL, "Failed to remove sessionRef %p from table.", sessionRef);

    LE_INFO("Client proccessId %d disconnected.", pClient->procId);

    // Find and remove all wakeup sources held for this client
    iter = le_hashmap_GetIterator(pm_recrd.locks);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        ws = (taf_ws_t*)le_hashmap_GetValue(iter);
        if (ws->clientPid != pClient->procId)
        {
            // skip if does not belong to this client
            continue;
        }

        // Release the aquired wakeup source
        if (ws->acquired)
        {
            LE_WARN("Releasing wakeup source '%s' on behalf of %s/%d.",
                    ws->name, pClient->name, ws->clientPid);
            ws->isRef = false;
            taf_pm_Relax((taf_pm_WakeupSourceRef_t)ws->wsRef);
        }

        // Delete wakeup source record from powermanager record, free memory
        LE_INFO("Deleting wakeup source '%s' on behalf of pid %d.", ws->name, ws->clientPid);
        le_hashmap_Remove(pm_recrd.locks, ws->name);
        le_ref_DeleteRef(pm_recrd.refs, ws->wsRef);
        le_mem_Release(ws);
    }
    le_mem_Release(pClient);
    return;
}

/**
 * Returns power instance
 */
taf_PM &taf_PM::GetInstance()
{
    static taf_PM instance;
    return instance;
}

void taf_PM::Init(void)
{
    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();
    // Get TCU-activity manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
#if defined(TARGET_SA515M)
    tcuActivityMgr = powerFactory.getTcuActivityManager(ClientType::MASTER, ProcType::LOCAL_PROC,
                        [&](telux::common::ServiceStatus status) {
                             prom.set_value(status);
                        });
#endif
#if defined(TARGET_SA525M)
    ClientInstanceConfig config;
    config.clientType = ClientType::MASTER;
    config.clientName = "tafPMSvc";
    config.machineName =  ALL_MACHINES;
    tcuActivityMgr = powerFactory.getTcuActivityManager(config,
                        [&](telux::common::ServiceStatus status) {
                             prom.set_value(status);
                        });
#endif
#ifdef TARGET_SA415M
    tcuActivityMgr = powerFactory.getTcuActivityManager(ClientType::MASTER);
#endif
    if(tcuActivityMgr == nullptr)
    {
        LE_INFO("tafPowerMgr is null Init...\n");
        return;
    }

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // wait unconditionally till the service is avilable
    bool isReady = (prom.get_future().get() == telux::common::ServiceStatus::SERVICE_AVAILABLE);
    if(isReady){
#endif
#ifdef TARGET_SA415M
    if(true){
#endif
        LE_INFO("TCU Activity manager is available");

        // Register for TCU service status change
        tcuServiceStatusListener = std::make_shared<tafTcuServiceStatusListener>();
        telux::common::Status status = tcuActivityMgr->registerServiceStateListener(tcuServiceStatusListener);
        if(status != telux::common::Status::SUCCESS) {
            LE_ERROR(" Failed to register for service state change ");
        }

        // Registering a listener for Local proc TCU-activity state updates
        tcuStateListener = std::make_shared<tafTcuStateListener>();
        StateChangeEvent = le_event_CreateId("tafStateChangeEvent",sizeof(stateEvent_t));
        AckEvent = le_event_CreateId("AckEvent",sizeof(TcuActivityState));
        le_event_AddHandler("AckEventId", AckEvent, sendAck);
        telux::common::Status registerStatus = tcuActivityMgr->registerListener(tcuStateListener);

        if(registerStatus != telux::common::Status::SUCCESS) {
            LE_INFO(" ERROR - Failed to register for TCU-activity state updates");
        } else {
            LE_INFO(" Registered Listener for TCU-activity state updates");
        }
    } else {
        LE_ERROR("ERROR Unable to intialize TCU activity service");
        return;
    }

    // Initialize powermanager record
    pm_recrd = {0, NULL, NULL, NULL, NULL, NULL};

    // Create table of safe references
    pm_recrd.refs = le_ref_InitStaticMap(tafPMReferences, TAF_PM_REFERENCE_DEFAULT_POOL_SIZE);
    if (NULL == pm_recrd.refs)
    {
        LE_FATAL("Failed to create safe reference table");
    }

    // Create memory pool for wakeup source records - exits on error
    pm_recrd.lockpool = le_mem_CreatePool("tafPMSource", sizeof(taf_ws_t));

    // Create table of wakeup sources
    pm_recrd.locks = le_hashmap_Create("tafPMWakeupSources", TAF_WAKEUP_SOURCE_DEFAULT_POOL_SIZE,
                                           le_hashmap_HashString, le_hashmap_EqualsString);
    if (NULL == pm_recrd.locks)
    {
        LE_FATAL("Failed to create wakeup source hashmap");
    }

    // exits on error in creating memory pool for client records - exits on error
    pm_recrd.clientpool = le_mem_CreatePool("tafPMClient", sizeof(taf_Client_t));

    // Create table of clients
    pm_recrd.clients = le_hashmap_Create("tafPMClient", TAF_PM_CLIENT_DEFAULT_HASH_SIZE,
                                             le_hashmap_HashVoidPointer, le_hashmap_EqualsVoidPointer);

    if (NULL == pm_recrd.clients)
    {
        LE_FATAL("Failed to create client hashmap");
    }

    // Register client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(taf_pm_GetServiceRef(), taf_Handler::OnClientConnection, NULL);
    le_msg_AddServiceCloseHandler(taf_pm_GetServiceRef(), taf_Handler::OnClientDisconnection, NULL);

#if defined(TARGET_SA525M)
    curTcuState = TAF_PM_STATE_RESUME;
#endif
    LE_INFO("tafPM service init done...\n");
}

/**
 * callback function to receive the error code of set TCU state
 */
void taf_Handler::commandCallback(ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO(" set TCU state command initiated successfully ");
    } else {
        LE_ERROR( " set TCU state command failed !!!");
    }
}

/**
 * Create a new wakeup source which can be used to acquire/release
 */
taf_pm_WakeupSourceRef_t taf_PM::NewWakeupSource( uint32_t options, const char *tag)
{
    taf_ws_t *pWakeSrc;
    taf_Client_t *pClient;
    char wsName[TAF_WS_NAME_LEN];

    TAF_KILL_CLIENT_IF_RET_VAL(('\0' == *tag) || (strlen(tag) > TAF_PM_TAG_LEN), NULL,
            "Error: wrong tag value.");

    // validate client record
    pClient = taf_PM::to_taf_Client_t(le_hashmap_Get(pm_recrd.clients,
                             taf_pm_GetClientSessionRef()));

    TAF_ERROR_IF_RET_VAL(pClient == NULL, NULL, "Client not found.");

    // Check if identical wakeup source already exists for this client
    snprintf(wsName, sizeof(wsName), TAF_WS_NAME_FORMAT, tag, pClient->name);
    pWakeSrc = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, wsName);
    TAF_KILL_CLIENT_IF_RET_VAL(pWakeSrc, NULL, "Error: Tag '%s' already exists.", tag);

    // Allocate and populate wakeup source record and exit on error
    pWakeSrc = (taf_ws_t*)le_mem_ForceAlloc(pm_recrd.lockpool);
    pWakeSrc->cookie = TAF_PM_WAKEUP_SOURCE_COOKIE;
    pWakeSrc->acquired = 0;
    pWakeSrc->clientPid = pClient->procId;
    le_utf8_Copy(pWakeSrc->name, wsName, sizeof(pWakeSrc->name), NULL);
    pWakeSrc->isRef = (options & TAF_PM_REF_COUNT ? true : false);
    pWakeSrc->wsRef = le_ref_CreateRef(pm_recrd.refs, pWakeSrc);

    // store in table of wakeup sources
    if (le_hashmap_Put(pm_recrd.locks, pWakeSrc->name, pWakeSrc))
    {
        LE_FATAL("Failed to add wakeup source '%s' to powermanager record.", pWakeSrc->name);
    }

    LE_INFO("New wakeup source '%s' for pid %d is created.", pWakeSrc->name, pWakeSrc->clientPid);
    return (taf_pm_WakeupSourceRef_t)pWakeSrc->wsRef;
}

/**
 * Acquire a wakeup source
 *
 */
le_result_t taf_PM::StayAwake(taf_pm_WakeupSourceRef_t wsRef)
{
    taf_ws_t *ws, *wsEntry;

    ws = taf_PM::ToTafWakeupSource(wsRef);

    if (NULL == ws)
    {
        return LE_OK;
    }

    wsEntry = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, ws->name);
    TAF_KILL_CLIENT_IF_RET_VAL(!wsEntry, LE_OK, "Wakeup source '%s' not created.\n", ws->name);

    if (wsEntry->acquired++)
    {
        if (!wsEntry->isRef)
        {
            LE_WARN("Wakeup source '%s' already acquired.", wsEntry->name);
        }
        if (0 == wsEntry->acquired)
        {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", wsEntry->name);
        }
        return LE_OK;
    }
    pm_recrd.wsAcquired++;

    // send resume state for local and remote proc if its not in resume state
    if( RemoteTcuActivityMgr != nullptr
            && RemoteTcuActivityMgr->getActivityState() != TcuActivityState::RESUME) {
        telux::common::Status RemoteStatus =
                RemoteTcuActivityMgr->setActivityState(TcuActivityState::RESUME,
                &taf_Handler::commandCallback);
        if( RemoteStatus == telux::common::Status::SUCCESS) {
            LE_INFO("cmd send successfully to remote process");
        } else {
            LE_ERROR("sending cmd to remote process failed");
        }
    }
    if(tcuActivityMgr->getActivityState() != TcuActivityState::RESUME) {
        telux::common::Status status = telux::common::Status::FAILED;
#if defined(TARGET_SA515M) || defined(TARGET_SA415M)
        status = tcuActivityMgr->setActivityState(
                TcuActivityState::RESUME, &taf_Handler::commandCallback);
#endif
#if defined(TARGET_SA525M)
         status = tcuActivityMgr->setActivityState(
                TcuActivityState::RESUME, ALL_MACHINES, &taf_Handler::commandCallback);
#endif
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("cmd send successfully");
            #if defined(TARGET_SA525M)
            curTcuState = TAF_PM_STATE_RESUME;
            #endif
        } else {
            LE_ERROR("sending cmd failed");
        }
    }
    return LE_OK;
}

/**
 * Releases a previously acquired wakeup source
 *
 */
le_result_t taf_PM::Relax( taf_pm_WakeupSourceRef_t wsRef)
{
    taf_ws_t *ws, *wsEntry;

    ws = taf_PM::ToTafWakeupSource(wsRef);
    if (NULL == ws)
    {
        return LE_OK;
    }

    wsEntry = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, ws->name);

    TAF_KILL_CLIENT_IF_RET_VAL(!wsEntry, LE_OK, "Wakeup source '%s' not created.\n", wsEntry->name);

    TAF_ERROR_IF_RET_VAL(!wsEntry->acquired, LE_OK, "Wakeup source '%s' already released",
            wsEntry->name);

    wsEntry->acquired--;
    if (wsEntry->isRef)
    {
        if (UINT_MAX == wsEntry->acquired)
        {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", wsEntry->name);
        }
        if (wsEntry->acquired > 0)
        {
           return LE_OK;
        }
    }
    else
    {
        wsEntry->acquired = 0;
    }

    pm_recrd.wsAcquired--;

    // if all the wake sources are in released state and set SUSPEND state
    if(pm_recrd.wsAcquired == 0) {
        if( RemoteTcuActivityMgr != nullptr)
        {
            telux::common::Status RemoteStatus = RemoteTcuActivityMgr->setActivityState(
                    TcuActivityState::SUSPEND, &taf_Handler::commandCallback);
            if( RemoteStatus == telux::common::Status::SUCCESS) {
                LE_INFO("cmd send successfully for remote proc");
            } else {
                LE_ERROR("sending cmd failed for remote proc");
            }
        }
#if defined(TARGET_SA515M) || defined(TARGET_SA415M)
        telux::common::Status status = tcuActivityMgr->setActivityState(
                TcuActivityState::SUSPEND, &taf_Handler::commandCallback);
#endif
#if defined(TARGET_SA525M)
        telux::common::Status status = tcuActivityMgr->setActivityState(
                TcuActivityState::SUSPEND, ALL_MACHINES, &taf_Handler::commandCallback);
#endif
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("cmd send successfully");
            #if defined(TARGET_SA525M)
            curTcuState = TAF_PM_STATE_SUSPEND;
            #endif
        } else {
            LE_ERROR("sending cmd failed");
        }
    }
    return LE_OK;
}

/**
 * Gives the current TCU state
 */
taf_pm_State_t taf_PM::GetPowerState()
{
    TAF_ERROR_IF_RET_VAL(tcuActivityMgr == NULL, TAF_PM_STATE_UNKNOWN, "tcuActivityMgr is null");
    telux::power::TcuActivityState state =  tcuActivityMgr->getActivityState();
    LE_INFO( "TCU state : %s\n ", tcuStateToString(state));
    return (tcuStateToTafPowerState(state));
}

/**
 * To convert taf_pm_State_t to TcuActivityState
 */
telux::power::TcuActivityState taf_PM::tafStateToTcuState(taf_pm_State_t tafState)
{
    TcuActivityState tcuState;
    switch(tafState) {
        case TAF_PM_STATE_RESUME:
            tcuState = TcuActivityState::RESUME;
            break;
        case TAF_PM_STATE_SUSPEND:
            tcuState = TcuActivityState::SUSPEND;
            break;
        case TAF_PM_STATE_SHUTDOWN:
            tcuState = TcuActivityState::SHUTDOWN;
            break;
        default:
            tcuState =  TcuActivityState::UNKNOWN;
            break;
    }
    return tcuState;
}

/**
 * To convert TcuActivityState to taf_pm_State_t
 */
taf_pm_State_t taf_PM::tcuStateToTafPowerState(TcuActivityState tcuState)
{
    taf_pm_State_t state;
    switch(tcuState) {
        case TcuActivityState::RESUME:
            state = TAF_PM_STATE_RESUME;
            break;
        case TcuActivityState::SUSPEND:
            state = TAF_PM_STATE_SUSPEND;
            break;
        case TcuActivityState::SHUTDOWN:
            state = TAF_PM_STATE_SHUTDOWN;
            break;
        default:
            state = TAF_PM_STATE_UNKNOWN;
            break;
    }
    return state;
}

/**
 * To convert TcuActivityState to string
 */
const char* taf_PM::tcuStateToString(TcuActivityState state)
{
    const char *tcuState;
    switch(state) {
        case TcuActivityState::RESUME:
            tcuState = "Resume";
            break;
        case TcuActivityState::SUSPEND:
            tcuState = "Suspend";
            break;
        case TcuActivityState::SHUTDOWN:
            tcuState = "Shutdown";
            break;
        default :
            tcuState = "Unknown";
            break;
    }
    return tcuState;
}

/**
 * To add handler for state change notification
 */
taf_pm_StateChangeHandlerRef_t taf_PM::AddStateChangeHandler(
taf_pm_StateChangeHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_INFO("AddStateChangeHandler");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "INVALID handler reference.");
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("tafStateChange EventId",
            StateChangeEvent, StateChanged, (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_pm_StateChangeHandlerRef_t)handlerRef;
}

void taf_PM::StateChanged(void* reportPtr, void* SecondLayeredHandlerFunc)
{
    stateEvent_t* stateEvent = (stateEvent_t*)reportPtr;
    taf_pm_StateChangeHandlerFunc_t clientHandlerFunc =
        (taf_pm_StateChangeHandlerFunc_t)SecondLayeredHandlerFunc;
    clientHandlerFunc(stateEvent->state, le_event_GetContextPtr());
}

/**
 * Removes state change handler
 */
void taf_PM::RemoveStateChangeHandler(taf_pm_StateChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    LE_INFO("Removed StateChangeHandler");
}

/**
 * callback function to receive TCU activity state update
 */
void tafTcuStateListener :: onTcuActivityStateUpdate(TcuActivityState state)
{
    taf_PM tafPwrMgr = taf_PM::GetInstance();
    LE_INFO("onTcuActivityStateUpdate state is %s\n", tafPwrMgr.tcuStateToString(state));

    stateEvent_t evt;
    if(state == TcuActivityState::SUSPEND) {
        evt.state = TAF_PM_STATE_SUSPEND;
    } else if(state == TcuActivityState::SHUTDOWN) {
        evt.state = TAF_PM_STATE_SHUTDOWN;
    } else if(state == TcuActivityState::RESUME) {
        evt.state = TAF_PM_STATE_RESUME;
    }

    // Send Resume request if WL is acquired, else send acknowledgement
    if(pm_recrd.wsAcquired > 0 && (state == TcuActivityState::SUSPEND
            || state == TcuActivityState::SHUTDOWN)) {
        telux::common::Status status = telux::common::Status::FAILED;
        status = tafPwrMgr.tcuActivityMgr->setActivityState(TcuActivityState::RESUME,
                &taf_Handler::commandCallback);
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("Resume cmd sent successfully");
            isResumed = true;
        } else {
            LE_ERROR("sending resume cmd failed");
        }
        le_event_Report(tafPwrMgr.AckEvent, &state, sizeof(state));
    } else {
        // send state change to all the handlers registered, except Resume triggered
        // from tafPMService if WL is acquired.
        if(!isResumed) {
            le_event_Report(tafPwrMgr.StateChangeEvent, &evt, sizeof(evt));
        } else {
            isResumed = false;
        }
        LE_INFO("sent report state %s\n",tafPwrMgr.tcuStateToString(state));
        if (state != TcuActivityState::RESUME)
            le_event_Report(tafPwrMgr.AckEvent, &state, sizeof(state));
    }
}

/**
 * callback received on slave applications sending ACK
 */
void tafTcuStateListener :: onSlaveAckStatusUpdate(Status status)
{
    LE_INFO("onSlaveAckStatusUpdate status %d\n", (int)status);
}

#if defined(TARGET_SA525M)
void tafTcuStateListener::onTcuActivityStateUpdate(TcuActivityState state, string machineName)
{
    taf_PM tafPwrMgr = taf_PM::GetInstance();
    LE_INFO("onTcuActivityStateUpdate machine : %s state : %s\n", machineName.c_str(),
            tafPwrMgr.tcuStateToString(state));
}

void tafTcuStateListener::onMachineUpdate(const string machineName, const MachineEvent machineEvt)
{
    LE_INFO("onMachineUpdate machineName : %s machineEvent : %s", machineName.c_str(),
            machineEvt == MachineEvent::AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");
}

void tafTcuStateListener::onSlaveAckStatusUpdate(const Status status,
                    const string machineName, const vector<ClientInfo> unresponsiveClients,
                    const vector<ClientInfo> nackResponseClients)
{
    LE_INFO("onSlaveAckStatusUpdate machineName : %s", machineName.c_str());
    taf_PM tafPwrMgr = taf_PM::GetInstance();
    if(status == telux::common::Status::SUCCESS) {
        LE_INFO("Slave applications successfully acknowledged the state transition");
    } else if(status == telux::common::Status::EXPIRED) {
        LE_INFO("Timeout occurred while waiting for acknowledgements from slave applications");
    } else {
        LE_ERROR("Failed to receive acknowledgements from slave applications");
    }
    if(unresponsiveClients.size() > 0) {
        LE_INFO("Number of unresponsive clients : %" PRIuS, unresponsiveClients.size());
        for (size_t i = 0; i < unresponsiveClients.size(); i++) {
            LE_INFO(" client name : %s machine name : %s", unresponsiveClients[i].first.c_str(),
                    unresponsiveClients[i].second.c_str());
        }
    }

    if(nackResponseClients.size() > 0) {
        LE_INFO("Number of clients responded with nack : %" PRIuS, nackResponseClients.size());
        for (size_t i = 0; i < nackResponseClients.size(); i++) {
            LE_INFO(" client name : %s, machine name : %s", nackResponseClients[i].first.c_str(),
                    nackResponseClients[i].second.c_str());
        }
    }
    stateEvent_t evt;
    evt.state = tafPwrMgr.curTcuState;
    le_event_Report(tafPwrMgr.StateChangeEvent, &evt, sizeof(evt));
}
#endif

/**
 * callback function to receive TCU activity state update from Remote Proc
 */
void tafRemoteTcuStateListener :: onTcuActivityStateUpdate(TcuActivityState state)
{
    taf_PM tafPwrMgr = taf_PM::GetInstance();
    LE_INFO("onTcuActivityStateUpdate form remote proc state is %s\n",
            tafPwrMgr.tcuStateToString(state));

    // Send Resume request if WL is acquired, else send acknowledgement
    if(pm_recrd.wsAcquired > 0 && (state == TcuActivityState::SUSPEND
            || state == TcuActivityState::SHUTDOWN)) {
        telux::common::Status status = tafPwrMgr.RemoteTcuActivityMgr->setActivityState(
                TcuActivityState::RESUME, &taf_Handler::commandCallback);
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("Resume cmd sent successfully to remote proc");
        } else {
            LE_ERROR("sending resume cmd failed to remote proc");
        }
    } else {
        Status ackStatus;
        if(state == TcuActivityState::SUSPEND) {
            ackStatus = tafPwrMgr.RemoteTcuActivityMgr->sendActivityStateAck(
                                TcuActivityStateAck::SUSPEND_ACK);
            if(ackStatus == Status::SUCCESS) {
                LE_INFO("Sent Suspend acknowledgement successfully to remote proc");
            } else {
                LE_INFO("Failed to send suspend acknowledgement to remote proc!");
            }
        } else if(state == TcuActivityState::SHUTDOWN) {
            ackStatus = tafPwrMgr.RemoteTcuActivityMgr->sendActivityStateAck(
                                TcuActivityStateAck::SHUTDOWN_ACK);
            if(ackStatus == Status::SUCCESS) {
                LE_INFO("Sent shutdown acknowledgement successfully to remote proc");
            } else {
                LE_INFO("Failed to send shutdown acknowledgement to remote proc!");
            }
        }
    }
}

/**
 * callback received from remote proc on slave applications sending ACK
 */
void tafRemoteTcuStateListener :: onSlaveAckStatusUpdate(Status status)
{
    LE_INFO("onSlaveAckStatusUpdate status for remote proc %d\n", (int)status);
}

/**
 * callback to receive TCU service status change
 */
void tafTcuServiceStatusListener::onServiceStatusChange(ServiceStatus status) {
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LE_INFO( " Service Status : UNAVAILABLE" );
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        LE_INFO( " Service Status : AVAILABLE" );
    }
}

void taf_PM::sendAck(void* reportPtr)
{
    Status ackStatus;
    TcuActivityState state = *(TcuActivityState*)reportPtr;
    taf_PM tafPwrMgr = GetInstance();
    if(state == TcuActivityState::SUSPEND) {
        ackStatus = tafPwrMgr.tcuActivityMgr->sendActivityStateAck(
                            TcuActivityStateAck::SUSPEND_ACK);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SUSPEND acknowledgement successfully");
        } else {
            LE_INFO("Failed to send SUSPEND acknowledgement !");
        }
    } else if(state == TcuActivityState::SHUTDOWN) {
        ackStatus = tafPwrMgr.tcuActivityMgr->sendActivityStateAck(
                            TcuActivityStateAck::SHUTDOWN_ACK);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SHUTDOWN acknowledgement successfully");
        } else {
            LE_INFO("Failed to send SHUTDOWN acknowledgement !");
        }
    }
}
