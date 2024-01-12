/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
/**
 * State Change registered Clients record
 */
std::vector<taf_pm_PowerStateRef_t>regClientrecrd;

/**
 * State Change Acknowledged Clients record
 */
std::vector<taf_pm_PowerStateRef_t>ackClientrecrd;

#if LE_CONFIG_TARGET_SA515M
static bool isResumed = false;
#endif
LE_REF_DEFINE_STATIC_MAP(tafPMReferences, TAF_PM_REFERENCE_DEFAULT_POOL_SIZE);

#if LE_CONFIG_TARGET_SA525M
LE_REF_DEFINE_STATIC_MAP(tafPMVmListRef, TAF_PM_VM_LIST_POOL_SIZE);
std::promise<le_result_t> stateChangePromise;
static bool isNack = false;

#endif

/**
 * Type-cast from taf_pm_WakeupSourceRef_t passed from app to taf_ws_t
 */
taf_ws_t *taf_PM::ToTafWakeupSource(taf_pm_WakeupSourceRef_t w)
{
    taf_ws_t *ws = (taf_ws_t *)le_ref_Lookup(pm_recrd.refs, w);

    TAF_ERROR_IF_RET_VAL(ws == NULL, NULL, "Invalid wakeup source provided");

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
    memset(pClient, 0, sizeof(taf_Client_t));
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

        if (!ws || ws->sessionRef != sessionRef)
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
#if LE_CONFIG_TARGET_SA525M
    auto &tafPowerMgr = taf_PM::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafPowerMgr.vmListRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_PMVmList_t* vmListPtr = (taf_PMVmList_t*)le_ref_GetValue(iterRef);
        if(vmListPtr && vmListPtr->sessionRef == sessionRef)
        {
            taf_pm_DeleteMachineList(vmListPtr->ref);
        }
    }
#endif
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
    std::promise<telux::common::ServiceStatus> slaveProm
                        = std::promise<telux::common::ServiceStatus>();
#if defined(TARGET_SA515M)
    tcuActivityMgr = powerFactory.getTcuActivityManager(ClientType::MASTER, ProcType::LOCAL_PROC,
                        [&](telux::common::ServiceStatus status) {
                             prom.set_value(status);
                        });
#endif
#if LE_CONFIG_TARGET_SA525M
    ClientInstanceConfig config;
    config.clientType = ClientType::MASTER;
    config.clientName = "tafPMSvc";
    config.machineName =  ALL_MACHINES;
    tcuActivityMgr = powerFactory.getTcuActivityManager(config,
                        [&](telux::common::ServiceStatus status) {
                             prom.set_value(status);
                        });
    ClientInstanceConfig slaveconfig;
    slaveconfig.clientType = ClientType::SLAVE;
    slaveconfig.clientName = "tafPMSvc";
    slaveconfig.machineName =  ALL_MACHINES;
    tcuSlaveActivityMgr = powerFactory.getTcuActivityManager(slaveconfig,
                        [&](telux::common::ServiceStatus status) {
                             slaveProm.set_value(status);
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
    else if(tcuSlaveActivityMgr == nullptr)
    {
        LE_ERROR("tafPowerMgr is null Init for slave...\n");
    }

#if defined(TARGET_SA515M) || LE_CONFIG_TARGET_SA525M
    // wait unconditionally till the service is avilable
    bool isReady = (prom.get_future().get() == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            && (slaveProm.get_future().get() == telux::common::ServiceStatus::SERVICE_AVAILABLE);
    if(isReady){
#endif
#ifdef TARGET_SA415M
    if(true){
#endif
        LE_INFO("TCU Activity manager is available");

        // Register for TCU service status change
        tcuServiceStatusListener = std::make_shared<tafTcuServiceStatusListener>();
        telux::common::Status status =
                tcuActivityMgr->registerServiceStateListener(tcuServiceStatusListener);
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
        if(tcuSlaveActivityMgr) {
            //Register for state change listener to notify the state changes to clients
            tcuSlaveStateListener = std::make_shared<tafTcuStateListener>();
            telux::common::Status registerStatuSlave =
                    tcuSlaveActivityMgr->registerListener(tcuSlaveStateListener);

            if(registerStatuSlave != telux::common::Status::SUCCESS) {
                LE_INFO(" ERROR - Failed to register for TCU-activity state updates");
            } else {
                LE_INFO(" Registered Listener for TCU-activity state updates");
            }
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

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, taf_PM::TafSigTermEventHandler);

#if LE_CONFIG_TARGET_SA525M
    curTcuState = TAF_PM_STATE_RESUME;
    vmListPool = le_mem_CreatePool("tafPMVirtualMachineList", sizeof(taf_PMVmList_t));
    vmInfoPool = le_mem_CreatePool("tafPMVirtualMachineInfo", sizeof(taf_PMVmInfo_t));
    vmListRefMap = le_ref_InitStaticMap(tafPMVmListRef, TAF_PM_VM_LIST_POOL_SIZE);

    powerStateRefPool = le_mem_CreatePool("tafPMPowerStateRefList", sizeof(taf_PmPowerStateRef_t));
    powerStateRefMap = le_ref_CreateMap("tafPMpowrStateRefMap", TAF_POWER_SOURCE_DEFAULT_POOL_SIZE);

    stateChangeExEvent = le_event_CreateId("tafstateChangeExEvent", sizeof(stateEvent_t));
    le_event_AddHandler("tafPMPowerStateChange event", stateChangeExEvent, PowerStateChanged);

    powerStateHandlerPool = le_mem_CreatePool("tafPMPStateHandlerList",
        sizeof(taf_PStateHandlerCtx_t));
    powerStateHandlerList = LE_DLS_LIST_INIT;
    powerStateHandlerRefMap = le_ref_CreateMap("tafPStateHandler",
        TAF_POWER_SOURCE_DEFAULT_POOL_SIZE);

#endif
    LE_INFO("tafPM service init done...\n");
}

/**
 * callback function to receive the error code of set TCU state
 */
void taf_Handler::commandCallback(ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO(" set TCU state command initiated successfully ");
#if LE_CONFIG_TARGET_SA525M
        stateChangePromise.set_value(LE_OK);
#endif
    } else {
        LE_ERROR( " set TCU state command failed !!!");
#if LE_CONFIG_TARGET_SA525M
        stateChangePromise.set_value(LE_FAULT);
#endif
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

    TAF_ERROR_IF_RET_VAL(('\0' == *tag) || (strlen(tag) > TAF_PM_TAG_LEN), NULL,
            "Error: wrong tag value.");

    // validate client record
    pClient = taf_PM::to_taf_Client_t(le_hashmap_Get(pm_recrd.clients,
                             taf_pm_GetClientSessionRef()));

    TAF_ERROR_IF_RET_VAL(pClient == NULL, NULL, "Client not found.");

    // Check if identical wakeup source already exists for this client
    snprintf(wsName, sizeof(wsName), TAF_WS_NAME_FORMAT, tag, pClient->sessionRef);
    pWakeSrc = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, wsName);
    TAF_ERROR_IF_RET_VAL(pWakeSrc, NULL, "Error: Tag '%s' already exists.", tag);

    // Allocate and populate wakeup source record and exit on error
    pWakeSrc = (taf_ws_t*)le_mem_ForceAlloc(pm_recrd.lockpool);
    memset(pWakeSrc, 0, sizeof(taf_ws_t));
    pWakeSrc->cookie = TAF_PM_WAKEUP_SOURCE_COOKIE;
    pWakeSrc->acquired = 0;
    pWakeSrc->clientPid = pClient->procId;
    pWakeSrc->sessionRef = taf_pm_GetClientSessionRef();
    le_utf8_Copy(pWakeSrc->name, wsName, sizeof(pWakeSrc->name), NULL);
    pWakeSrc->isRef = (options & TAF_PM_REF_COUNT ? true : false);
    pWakeSrc->wsRef = le_ref_CreateRef(pm_recrd.refs, pWakeSrc);

    // store in table of wakeup sources
    if (le_hashmap_Put(pm_recrd.locks, pWakeSrc->name, pWakeSrc))
    {
        LE_FATAL("Failed to add wakeup source '%s' to powermanager record.", pWakeSrc->name);
    }

    LE_INFO("New wakeup source '%s' for pid %d and sessionRef %p is created.", pWakeSrc->name,
            pWakeSrc->clientPid, taf_pm_GetClientSessionRef());
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

    TAF_ERROR_IF_RET_VAL(!ws, LE_BAD_PARAMETER, "Invalid Wakeup source reference.\n");

    wsEntry = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, ws->name);
    TAF_ERROR_IF_RET_VAL(!wsEntry, LE_BAD_PARAMETER, "Wakeup source '%s' not created.\n", ws->name);

    uint32_t prevCount = wsEntry->acquired;
    if (wsEntry->acquired++)
    {
        if (!wsEntry->isRef)
        {
            LE_WARN("Wakeup source '%s' already acquired.", wsEntry->name);
        }
        if (0 == wsEntry->acquired)
        {
            LE_ERROR("Wakeup source '%s' reference counter overlaps.", wsEntry->name);
            wsEntry->acquired = prevCount;
            return LE_FAULT;
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
#if defined(TARGET_SA515M) || defined(TARGET_SA415M)
        telux::common::Status status = tcuActivityMgr->setActivityState(
                TcuActivityState::RESUME, &taf_Handler::commandCallback);
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("cmd send successfully");
        } else {
            LE_ERROR("sending cmd failed");
        }
#endif
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
    TAF_ERROR_IF_RET_VAL(!ws, LE_BAD_PARAMETER, "Invalid Wakeup source reference.\n");


    wsEntry = (taf_ws_t*)le_hashmap_Get(pm_recrd.locks, ws->name);

    TAF_KILL_CLIENT_IF_RET_VAL(!wsEntry, LE_OK, "Wakeup source '%s' not created.\n", wsEntry->name);

    TAF_ERROR_IF_RET_VAL(!wsEntry->acquired, LE_OK, "Wakeup source '%s' already released",
            wsEntry->name);

    if (wsEntry->isRef)
    {
        TAF_ERROR_IF_RET_VAL(UINT_MAX == (wsEntry->acquired - 1), LE_FAULT,
                "Wakeup source '%s' reference counter overlaps", wsEntry->name);

        wsEntry->acquired--;
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
#if LE_CONFIG_TARGET_SA525M
        auto &tafPwrMgr = taf_PM::GetInstance();
        stateEvent_t evt;
        evt.state = TAF_PM_STATE_ALL_WAKELOCKS_RELEASED;
        le_event_Report(tafPwrMgr.stateChangeExEvent, &evt, sizeof(evt));
#endif
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
        if( status == telux::common::Status::SUCCESS) {
            LE_INFO("cmd send successfully");
        } else {
            LE_ERROR("sending cmd failed");
        }
#endif
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

#if LE_CONFIG_TARGET_SA525M
/**
 * To add handler for Extend power state change notification
 */
taf_pm_StateChangeExHandlerRef_t taf_PM::AddStateChangeExHandler
(taf_pm_StateChangeExHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_INFO("AddStateChangeExHandler");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "INVALID handler reference.");
    taf_PStateHandlerCtx_t * handlerCtxPtr =
            (taf_PStateHandlerCtx_t *)le_mem_ForceAlloc(powerStateHandlerPool);
    taf_Client_t *pClient;
    pClient = taf_PM::to_taf_Client_t(le_hashmap_Get(pm_recrd.clients,
            taf_pm_GetClientSessionRef()));
    LE_INFO("Client is %s", pClient->name);
    if(strncmp(pClient->name, "tafMngdPMSvc", 12) == 0)
    {
        LE_INFO("Client is MPM");
        handlerCtxPtr->ismpm = true;
    }
    else
    {
        LE_INFO("Client is %s", pClient->name);
        handlerCtxPtr->ismpm = false;
    }
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->contextPtr = contextPtr;
    handlerCtxPtr->handlerRef = (taf_pm_StateChangeExHandlerRef_t)le_ref_CreateRef(
            powerStateHandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&powerStateHandlerList, &handlerCtxPtr->link);

    return handlerCtxPtr->handlerRef;
}

void taf_PM::PowerStateChanged(void* reportPtr)
{
    stateEvent_t* stateEvent = (stateEvent_t*)reportPtr;
    taf_PM tafPwrMgr = taf_PM::GetInstance();
    tafPwrMgr.CallClientHandlerFunc(stateEvent->state);
}

void taf_PM::DeletePowerStateRefs()
{
    LE_INFO("DeletePowerStateRefs");
    for (auto it = regClientrecrd.begin(); it != regClientrecrd.end(); ++it) {
        if (*it != NULL)
        {
            le_ref_DeleteRef(powerStateRefMap, *it);
        }
    }
}

/**
 * To Call Clients for Extend power state change notification
 */
void taf_PM::CallClientHandlerFunc(taf_pm_State_t state)
{
    LE_INFO("CallClientHandlerFunc");
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&powerStateHandlerList);
    //clearing the previous references for new state notification
    DeletePowerStateRefs();
    regClientrecrd.clear();
    ackClientrecrd.clear();
    isNack = false;
    while (linkHandlerPtr)
    {
        taf_PStateHandlerCtx_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_PStateHandlerCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&powerStateHandlerList, linkHandlerPtr);
        if (handlerCtxPtr->handlerPtr)
        {
            LE_INFO("Client found");
            /////Notifying only to MPMS for graceful
            if((state == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED || state == TAF_PM_STATE_ALL_ACKED))
            {
                if(handlerCtxPtr->ismpm == true)
                {
                    LE_INFO("Notified to MPM");
                    taf_PmPowerStateRef_t* pmPStateListPtr =
                            (taf_PmPowerStateRef_t*)le_mem_ForceAlloc(powerStateRefPool);

                    pmPStateListPtr->pStateRef =
                            (taf_pm_PowerStateRef_t)le_ref_CreateRef(powerStateRefMap, pmPStateListPtr);

                    regClientrecrd.push_back(pmPStateListPtr->pStateRef);
                    handlerCtxPtr->handlerPtr(pmPStateListPtr->pStateRef, TAF_PM_PVM, state,
                            handlerCtxPtr->contextPtr);
                    return;
                }
                else {
                    continue;
                }
            }
            else {
                LE_INFO("Notifying to PM clients");
                /// //creating a power state ref for each client
                taf_PmPowerStateRef_t* pmPStateListPtr =
                        (taf_PmPowerStateRef_t*)le_mem_ForceAlloc(powerStateRefPool);
                pmPStateListPtr->pStateRef =
                        (taf_pm_PowerStateRef_t)le_ref_CreateRef(powerStateRefMap, pmPStateListPtr);
                regClientrecrd.push_back(pmPStateListPtr->pStateRef);
                handlerCtxPtr->handlerPtr(pmPStateListPtr->pStateRef, TAF_PM_PVM, state,
                        handlerCtxPtr->contextPtr);
            }
        }
    }
}
/**
 * Removes Extend power state change handler
 */
void taf_PM::RemoveStateChangeExHandler(taf_pm_StateChangeExHandlerRef_t handlerRef)
{
    LE_INFO("RemoveStateChangeExHandler");
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&powerStateHandlerList);
    while (linkHandlerPtr)
    {
        taf_PStateHandlerCtx_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_PStateHandlerCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&powerStateHandlerList, linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
            le_ref_DeleteRef(powerStateHandlerRefMap, handlerRef);
            le_dls_Remove(&powerStateHandlerList, &(handlerCtxPtr->link));
            le_mem_Release((void*)handlerCtxPtr);
        }
    }
}

/**
 * Sets the power state to VM
 */
le_result_t taf_PM::SetPowerState(taf_pm_State_t state, const char* machineName)
{
    TcuActivityState tcuState = tafStateToTcuState(state);
    LE_INFO( "SetPowerState state : %s to machine : %s", tcuStateToString(tcuState), machineName);

    if(state == TAF_PM_STATE_SUSPEND && pm_recrd.wsAcquired > 0)
    {
        LE_ERROR("Trying to set suspend state when wake source is acquired");
        return LE_FAULT;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    stateChangePromise = std::promise<le_result_t>();
    status = tcuActivityMgr->setActivityState(
            tcuState, machineName, &taf_Handler::commandCallback);
    if( status == telux::common::Status::SUCCESS) {
        LE_INFO("cmd send successfully");
        curTcuState = state;
        // blocking here to get result from callback
        std::future<le_result_t> futResult = stateChangePromise.get_future();
        std::future_status waitStatus = futResult.wait_for(std::chrono::seconds(SET_STATE_TIMEOUT));
        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("waiting promise timeout for %d seconds", SET_STATE_TIMEOUT);
            return LE_FAULT;
        }
        else
        {
            le_result_t res = futResult.get();
            LE_INFO("result is %s", res== LE_FAULT ? "FAULT" : "OK");
            return res;
        }
    } else {
        LE_ERROR("sending cmd failed");
    }

    return LE_FAULT;
}

taf_pm_VMListRef_t taf_PM::GetMachineList()
{
    taf_PMVmList_t* vmListPtr = (taf_PMVmList_t*)le_mem_ForceAlloc(vmListPool);
    memset(vmListPtr, 0, sizeof(taf_PMVmList_t));
    vmListPtr->VmInfoList = LE_SLS_LIST_INIT;
    vmListPtr->sessionRef = taf_pm_GetClientSessionRef();
    vmListPtr->currPtr = NULL;

    std::vector<std::string> machineNames;
    telux::common::Status status = tcuActivityMgr->getAllMachineNames(machineNames);
    if(status == telux::common::Status::SUCCESS) {
        taf_PMVmInfo_t* vmInfoPtr;
        for (size_t i = 0; i < machineNames.size(); i++){
            vmInfoPtr = (taf_PMVmInfo_t*)le_mem_ForceAlloc(vmInfoPool);
            memset(vmInfoPtr, 0, sizeof(taf_PMVmInfo_t));
            le_utf8_Copy(vmInfoPtr->name, machineNames[i].c_str(), TAF_PM_MACHINE_NAME_LEN, NULL);
            LE_DEBUG("Virtual Machine name is %s", vmInfoPtr->name);
            vmInfoPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(vmListPtr->VmInfoList), &(vmInfoPtr->link));
        }
        vmListPtr->ref = (taf_pm_VMListRef_t)le_ref_CreateRef(vmListRefMap, (void*)vmListPtr);
        return vmListPtr->ref;
    }
    return NULL;
}

le_result_t taf_PM::DeleteMachineList(taf_pm_VMListRef_t vmListRef)
{
    TAF_ERROR_IF_RET_VAL(vmListRef == nullptr, LE_BAD_PARAMETER, "Null reference(vmListRef)");

    taf_PMVmList_t* listPtr = (taf_PMVmList_t*)le_ref_Lookup(vmListRefMap, vmListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    LE_DEBUG("DeleteMachineList : %p", vmListRef);
    taf_PMVmInfo_t* vmInfoPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->VmInfoList))) != NULL) {
        vmInfoPtr = CONTAINER_OF(linkPtr, taf_PMVmInfo_t, link);
        le_mem_Release(vmInfoPtr);
    }

    le_ref_DeleteRef(vmListRefMap, vmListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

le_result_t taf_PM::GetFirstMachineName( taf_pm_VMListRef_t vmListRef,
        char* vmNamePtr, size_t vmNamePtrSize )
{
    taf_PMVmList_t* listPtr = (taf_PMVmList_t*)le_ref_Lookup(vmListRefMap,
        vmListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_BAD_PARAMETER,
        "Failed to look up the reference:%p", vmListRef);

    TAF_ERROR_IF_RET_VAL(vmNamePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(vmNamePtr)");

    TAF_ERROR_IF_RET_VAL(vmNamePtrSize < TAF_PM_MACHINE_NAME_LEN, LE_BAD_PARAMETER,
        "Invalid para(vmNamePtrSize: %" PRIuS " < %d)", vmNamePtrSize, TAF_PM_MACHINE_NAME_LEN);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->VmInfoList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, LE_NOT_FOUND, "Empty list");

    taf_PMVmInfo_t* vmInfoPtr = CONTAINER_OF(linkPtr, taf_PMVmInfo_t , link);
    listPtr->currPtr = linkPtr;

    le_utf8_Copy(vmNamePtr, vmInfoPtr->name, TAF_PM_MACHINE_NAME_LEN, NULL);

    return LE_OK;
}

le_result_t taf_PM::GetNextMachineName( taf_pm_VMListRef_t vmListRef,
        char* vmNamePtr, size_t vmNamePtrSize )
{
    taf_PMVmList_t* listPtr = (taf_PMVmList_t*)le_ref_Lookup(vmListRefMap,
        vmListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_BAD_PARAMETER,
        "Failed to look up the reference:%p", vmListRef);

    TAF_ERROR_IF_RET_VAL(vmNamePtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(vmNamePtr)");

    TAF_ERROR_IF_RET_VAL(vmNamePtrSize < TAF_PM_MACHINE_NAME_LEN, LE_BAD_PARAMETER,
        "Invalid para(vmNamePtrSize: %" PRIuS " < %d)", vmNamePtrSize, TAF_PM_MACHINE_NAME_LEN);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->VmInfoList),  listPtr->currPtr);
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, LE_NOT_FOUND, "Reached to the end of list");

    taf_PMVmInfo_t* vmInfoPtr = CONTAINER_OF(linkPtr, taf_PMVmInfo_t , link);
    listPtr->currPtr = linkPtr;

    le_utf8_Copy(vmNamePtr, vmInfoPtr->name, TAF_PM_MACHINE_NAME_LEN, NULL);

    return LE_OK;
}
#endif

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

#if LE_CONFIG_TARGET_SA515M
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
#endif

void taf_PM::TafSigTermEventHandler(int tafSigNum)
{
    LE_DEBUG("TafSigTermEventHandler :%d", tafSigNum);
    auto &tafPwrMgr = taf_PM::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;
#if LE_CONFIG_TARGET_SA525M
    // Resume in SA525M before service termination as master app is terminating
    if(tafPwrMgr.tcuActivityMgr->getActivityState() != TcuActivityState::RESUME) {
        status = tafPwrMgr.tcuActivityMgr->setActivityState(
                TcuActivityState::RESUME, ALL_MACHINES, &taf_Handler::commandCallback);
        if( status == telux::common::Status::SUCCESS) {
            LE_DEBUG("Resume cmd sent successfully on service termination");
            tafPwrMgr.curTcuState = TAF_PM_STATE_RESUME;
        } else {
            LE_ERROR("sending cmd failed");
        }
    }
#endif
    status = tafPwrMgr.tcuActivityMgr->deregisterListener(tafPwrMgr.tcuStateListener);
    if(status != telux::common::Status::SUCCESS) {
        LE_ERROR("Failed to deregister for TCU-activity state updates on service termination");
    } else {
        LE_DEBUG("Deregistered listener for TCU-activity state updates on service termination");
    }

    status = tafPwrMgr.tcuSlaveActivityMgr->deregisterListener(tafPwrMgr.tcuSlaveStateListener);
    if(status != telux::common::Status::SUCCESS) {
        LE_ERROR("Failed to deregister slave for TCU-activity state update on service termination");
    } else {
        LE_DEBUG("Deregistered slave listener for TCU-activity state updates on service termination");
    }

    status = tafPwrMgr.tcuActivityMgr->deregisterServiceStateListener(tafPwrMgr
            .tcuServiceStatusListener);
    if(status != telux::common::Status::SUCCESS) {
        LE_ERROR("Failed to deregister for service state change on service termination");
    } else {
        LE_DEBUG("Deregistered Listener for service state change on service termination");
    }
}

#if LE_CONFIG_TARGET_SA525M
void tafTcuStateListener::onTcuActivityStateUpdate(TcuActivityState state, string machineName)
{
    auto &tafPwrMgr = taf_PM::GetInstance();
    LE_INFO("onTcuActivityStateUpdate machine : %s state : %s\n", machineName.c_str(),
            tafPwrMgr.tcuStateToString(state));
    ackClientrecrd.clear();
    LE_INFO("ackClientrecrd size is:%ld",ackClientrecrd.size());
    stateEvent_t evt;
    if(state == TcuActivityState::SUSPEND) {
        evt.state = TAF_PM_STATE_SUSPEND;
    } else if(state == TcuActivityState::SHUTDOWN) {
        evt.state = TAF_PM_STATE_SHUTDOWN;
    } else if(state == TcuActivityState::RESUME) {
        evt.state = TAF_PM_STATE_RESUME;
    }

    // send state change notification to all the handlers registered
    LE_INFO("sent report state %s\n",tafPwrMgr.tcuStateToString(state));
    le_event_Report(tafPwrMgr.StateChangeEvent, &evt, sizeof(evt));
    LE_INFO("sent Extend report state %s\n",tafPwrMgr.tcuStateToString(state));
    le_event_Report(tafPwrMgr.stateChangeExEvent, &evt, sizeof(evt));
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
    auto &tafPwrMgr = GetInstance();
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
#if defined(TARGET_SA525M)
void taf_PM:: SendAckToPmd(taf_pm_State_t state)
{
    LE_INFO("SendAckToPmd");
    auto &tafPwrMgr = GetInstance();
    Status ackStatus;
    if(state == TAF_PM_STATE_SUSPEND) {
        ackStatus = tafPwrMgr.tcuSlaveActivityMgr->sendActivityStateAck(
                            StateChangeResponse::ACK, TcuActivityState::SUSPEND);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SUSPEND acknowledgement to TCU successfully");
        } else {
            LE_INFO("Failed to send SUSPEND acknowledgement to TCU!");
        }
    } else if(state == TAF_PM_STATE_SHUTDOWN) {
        ackStatus = tafPwrMgr.tcuSlaveActivityMgr->sendActivityStateAck(
                            StateChangeResponse::ACK, TcuActivityState::SHUTDOWN);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SHUTDOWN acknowledgement to TCU successfully");
        } else {
            LE_INFO("Failed to send SHUTDOWN acknowledgement to TCU !");
        }
    }
}

void taf_PM:: SendNackToPmd(taf_pm_State_t state)
{
    LE_INFO("SendNackToPmd");
    auto &tafPwrMgr = GetInstance();
    Status ackStatus;
    if(state == TAF_PM_STATE_SUSPEND) {
        ackStatus = tafPwrMgr.tcuSlaveActivityMgr->sendActivityStateAck(
                            StateChangeResponse::NACK, TcuActivityState::SUSPEND);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SUSPEND NACK successfully");
        } else {
            LE_INFO("Failed to send SUSPEND NACK !");
        }
    } else if(state == TAF_PM_STATE_SHUTDOWN) {
        ackStatus = tafPwrMgr.tcuSlaveActivityMgr->sendActivityStateAck(
                            StateChangeResponse::NACK, TcuActivityState::SHUTDOWN);
        if(ackStatus == Status::SUCCESS) {
            LE_INFO("Sent SHUTDOWN NACK successfully");
        } else {
            LE_INFO("Failed to send SHUTDOWN NACK !");
        }
    }
}

/**
 * Calls from clients to acknowledge power state change transition.
 */
void taf_PM::SendStateChangeAck(taf_pm_PowerStateRef_t powerStateRef,
taf_pm_State_t state, taf_pm_NadVm_t vm_id, taf_pm_ClientAck_t ackType )
{
    LE_INFO("sendStateChangeAck");
    //Getting the current client data from pm_recrd
    taf_Client_t *pClient;
    pClient = taf_PM::to_taf_Client_t(le_hashmap_Get(pm_recrd.clients,
            taf_pm_GetClientSessionRef()));
    TcuActivityState tcuState = tafStateToTcuState(state);
    // validate client record existed in state change registered clients
    for (auto it = regClientrecrd.begin(); it != regClientrecrd.end(); ++it ) {
        if (*it == (taf_pm_PowerStateRef_t)powerStateRef) {
            LE_INFO("Client found in record");
            if(isNack) {
                LE_INFO("Nack received from previous client");
                return ;
            }
            break;
        }
        if (it == regClientrecrd.end()) {
            LE_INFO("Client not found in the regClientrecrd");
            return;
        }
    }
    if(state == TAF_PM_STATE_ALL_ACKED && ackType == TAF_PM_READY)
    {
         LE_INFO("Received ACK from client %s",pClient->name);
         SendAckToPmd(curTcuState);
         return;
    }
    else if(state == TAF_PM_STATE_ALL_ACKED && ackType == TAF_PM_NOT_READY)
    {
        LE_INFO("Received NACK from client %s", pClient->name);
        isNack = true;
        SendNackToPmd(curTcuState);
        return;
    }
    else if(curTcuState == state)
    {
        if(ackType == TAF_PM_NOT_READY)
        {
            LE_INFO("Received NACK from client %s for state %s", pClient->name,
                    tcuStateToString(tcuState));
            isNack = true;
            SendNackToPmd(state);
        }
        else
        {
            LE_INFO("Received ACK from client %s for state %s",pClient->name,
                    tcuStateToString(tcuState));

            ackClientrecrd.push_back((taf_pm_PowerStateRef_t)powerStateRef);
            LE_INFO("regClientrecrd size is %ld ,ackClientrecrd size is:%ld",regClientrecrd.size(),
                    ackClientrecrd.size());
            //If Last acknowledged client , proceed for ack state change
            if(regClientrecrd.size() == ackClientrecrd.size())
            {
                auto &tafPwrMgr = taf_PM::GetInstance();
                stateEvent_t evt;
                evt.state = TAF_PM_STATE_ALL_ACKED;
                le_event_Report(tafPwrMgr.stateChangeExEvent, &evt, sizeof(evt));
            }
            else
            {
                LE_INFO("All clients not acknowledged for state change yet");
                return;
            }
        }
    }
    else
    {
        LE_INFO("Client %s Ack response not sent for current transition %s", pClient->name,
                tcuStateToString(tcuState));
    }
}
#endif