/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdConnData.hpp"
#include "tafMngdConnRadio.hpp"
#include "tafMngdConnSim.hpp"
#include "tafMngdConnECall.hpp"
#include "tafMngdConnAdmin.hpp"
#include "limit.h"


using namespace tafsvc;

#define CONFIG_FILE_NAME "mngdConnectivity.json"

/**
 * Initialize static variables
 */
mcs_Clients_t tafMngdConnAdmin::ConnectedClients = {NULL, NULL};

mcs_RetryClients_t tafMngdConnAdmin::RetryClients = {NULL};

/**
 * Initialize static memory pools
 */
LE_MEM_DEFINE_STATIC_POOL(tafMngdConnMemPool, MCS_MAX_DATA_OBJ,
                                            sizeof(mcs_DataCtx_t));
LE_MEM_DEFINE_STATIC_POOL(ConnectedClientsCtxMemPool, MCS_MAX_SESSIONS,
                                            sizeof(mcs_ClientNode_t));
LE_MEM_DEFINE_STATIC_POOL(RetryClientsCtxMemPool, MCS_MAX_SESSIONS,
                                            sizeof(mcs_ClientNode_t));

tafMngdConnAdmin &tafMngdConnAdmin::GetInstance()
{
    static tafMngdConnAdmin instance;
    return instance;
}

void Data_init()
{
    auto &data = tafMngdConnData::GetInstance();
    data.Init();
}

void Radio_init()
{
    auto &radio = tafMngdConnRadio::GetInstance();
    radio.Init();
}

void Sim_init()
{
    auto &sim = tafMngdConnSim::GetInstance();
    sim.Init();
}

void ECall_init()
{
#ifndef LE_CONFIG_TARGET_SIMULATION
    auto &ecall = tafMngdConnECall::GetInstance();
    ecall.Init();
#endif
}

void tafMngdConnAdmin::Init(void)
{
    //Initiate data module.
    Data_init();
    //Initiate radio module.
    Radio_init();
    //Initiate sim module.
    Sim_init();
    //Initiate ECall module.
    ECall_init();

    //Initiate the memory pool.
    DataCtxPool = le_mem_InitStaticPool(tafMngdConnMemPool, MCS_MAX_DATA_OBJ,
                                        sizeof(mcs_DataCtx_t));

    dataStatePool = le_mem_CreatePool("dataStatePool", sizeof(DataState_t));


    recoveryEventPool = le_mem_CreatePool("recoveryEventPool", sizeof(RecoveryEvent_t));
    // Create recovery state event
    recoveryEvent = le_event_CreateIdWithRefCounting("recoveryEvent");

    //Create reference map for data context
    DataRefMap = le_ref_CreateMap("DataRefMap", MCS_MAX_DATA_OBJ);

    //Create the mutex.
    DataCtxMutex = le_mutex_CreateNonRecursive("DataCtxMutex");

    //Create the semaphore
    le_sem_Ref_t semRef = le_sem_Create("SmThreadSem", 0);

    //Create the event handle thread
    StateMachineEventThreadRef = le_thread_Create("MngdEvtThread", StateMachineEventThreadFunc,
                                                  (void *)semRef);
    le_thread_SetJoinable(StateMachineEventThreadRef);
    le_thread_Start(StateMachineEventThreadRef);
    le_sem_Wait(semRef);

    //Create the callback handle thread
    tafMngd_event_thread = le_thread_Create("MngdCbThread",  callback_thread_func,  (void*)semRef);
    le_thread_SetJoinable(tafMngd_event_thread);
    le_thread_Start (tafMngd_event_thread);
    le_sem_Wait(semRef);

    le_sem_Delete(semRef);

    // Check if ConfigTree has the Policy and Configuration File names
    if (tafMngdConnSvc_GetPolicyAndConfiguration( Policy, Configuration, CONFIG_FILE_NAME))
    {
        // Policy and Configuration parsed and validated. Move to "Data-Not_Connected" state
        LE_DEBUG("JSONs parsed. Initialization Complete and set IsJsonValid with true");
        IsJsonValid = true;
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        // Report the event to the state machine
        stateMachineEvt.event=MCS_EVT_INIT;
        le_event_Report(StateMachineEventId, &stateMachineEvt,
                                                        sizeof(stateMachineEvent_t));
    }
    else
    {
        LE_FATAL("Stopping service. Valid configuration JSON file is needed.");
    }

    // Create client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(taf_mngdConn_GetServiceRef(),
                                            tafMngdConnAdmin::OnClientConnect,NULL);

    le_msg_AddServiceCloseHandler(taf_mngdConn_GetServiceRef(),
                                            tafMngdConnAdmin::OnClientDisconnect,NULL);

    // Create memory pool for connected client nodes.
    ConnectedClients.memPool = le_mem_InitStaticPool(ConnectedClientsCtxMemPool,
                                                        MCS_MAX_SESSIONS,
                                                        sizeof(mcs_ClientNode_t));
    // Create hash map of connected clients
    ConnectedClients.hashMap = le_hashmap_Create("ClientsHashMap", MCS_MAX_SESSIONS,
                                                 le_hashmap_HashVoidPointer,
                                                 le_hashmap_EqualsVoidPointer);

    if (NULL == ConnectedClients.hashMap)
    {
        LE_FATAL("Failed to create connected client hashmap");
    }

     // Create memory pool for retry client nodes.
    RetryClients.memPool = le_mem_InitStaticPool(RetryClientsCtxMemPool,
                                                        MCS_MAX_SESSIONS,
                                                        sizeof(mcs_RetryClientNode_t));
    // Create hash map of retry clients
    RetryClients.hashMap = le_hashmap_Create("RetryClientsHashMap", MCS_MAX_SESSIONS,
                                                 le_hashmap_HashVoidPointer,
                                                 le_hashmap_EqualsVoidPointer);

    if (NULL == RetryClients.hashMap)
    {
        LE_FATAL("Failed to create connected client hashmap");
    }
}

void tafMngdConnAdmin::Deinit(void)
{
    auto &data = tafMngdConnData::GetInstance();
    data.Deinit();

    if (StateMachineEventThreadRef)
    {
        LE_DEBUG("Stopping StateMachineEventThreadRef");
        le_thread_Cancel(StateMachineEventThreadRef);
        le_thread_Join(StateMachineEventThreadRef, NULL);
        StateMachineEventThreadRef = NULL;
    }
    if (tafMngd_event_thread)
    {
        LE_DEBUG("Stopping tafMngd_event_thread");
        le_thread_Cancel(tafMngd_event_thread);
        le_thread_Join(tafMngd_event_thread, NULL);
        tafMngd_event_thread = NULL;
    }

    LE_DEBUG("Delete DataCtxMutex");
    le_mutex_Delete(DataCtxMutex);
    DataCtxMutex = NULL;

    LE_DEBUG("Release objects from DataCtxList");
    le_dls_Link_t *linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        void *dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        le_mem_Release(dataCtxPtr);
    }
    LE_INFO("tafMngdConnAdmin::Deinit done");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::PowerStateChangeHandler
(
    taf_pm_State_t state,
    void* contextPtr
)
{
    if (state != TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to SUSPEND");
        return;
    }

    LE_INFO("Power state change to RESUME");

    auto& mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    le_event_Id_t eventId = mngdConnAdmin.GetStateMachineEventId();

    std::vector<stateMachineEvent_t> eventsToReport;

    le_mutex_Lock(mngdConnAdmin.DataCtxMutex);
    le_dls_Link_t* linkPtr = le_dls_Peek(&mngdConnAdmin.DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&mngdConnAdmin.DataCtxList, linkPtr);

        taf_radio_NetRegState_t psState = TAF_RADIO_NET_REG_STATE_UNKNOWN;
        le_result_t result = taf_radio_GetPacketSwitchedState(&psState, dataCtxPtr->phoneId);
        if (result != LE_OK)
        {
            LE_WARN("Failed to get packet switched state for phoneId %d after resume: %d",
                dataCtxPtr->phoneId, result);
            continue;
        }

        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};

        if ((psState == TAF_RADIO_NET_REG_STATE_HOME ||
            psState == TAF_RADIO_NET_REG_STATE_ROAMING) &&
            dataCtxPtr->dataState == TAF_MNGDCONN_DATA_DISCONNECTED)
        {
            stateMachineEvt.event = MCS_EVT_NETWORK_REG_STATE;
            stateMachineEvt.phoneId = dataCtxPtr->phoneId;
            eventsToReport.push_back(stateMachineEvt);
        }
        else if (psState == TAF_RADIO_NET_REG_STATE_NONE &&
            dataCtxPtr->dataState != TAF_MNGDCONN_DATA_DISCONNECTED)
        {
            stateMachineEvt.event = MCS_EVT_NETWORK_UNREG_STATE;
            stateMachineEvt.phoneId = dataCtxPtr->phoneId;
            eventsToReport.push_back(stateMachineEvt);
        }
    }
    le_mutex_Unlock(mngdConnAdmin.DataCtxMutex);

    for (auto& evt : eventsToReport)
    {
        if (eventId != nullptr)
        {
            le_event_Report(eventId, &evt, sizeof(stateMachineEvent_t));
            LE_INFO("Reported event %d for phoneId %d after resume", evt.event, evt.phoneId);
        }
        else
            LE_ERROR("Cannot report event - eventId is null");
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Client connect function.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::OnClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);
    pid_t pid;
    char appName[LIMIT_MAX_PATH_BYTES] = {0};

    LE_INFO("Client %p Connected.", sessionRef);
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pid))
    {
        LE_WARN("Error, Failed to get client pid.");
    }

    if (le_appInfo_GetName(pid, appName, sizeof(appName)) == LE_OK)
    {
        LE_DEBUG ("Client Details: pid: %ld, name: %s", static_cast<long int>(pid), appName);
    }
    else
    {
        LE_WARN("Error, Failed to get client app name");
    }

    // Add this client to ConnectedClients hash map
    mcs_ClientNode_t *clientNodePtr =
        (mcs_ClientNode_t *)le_mem_TryAlloc(ConnectedClients.memPool);
    TAF_ERROR_IF_RET_NIL(clientNodePtr == nullptr, "Cannot allocate connected clientNodePtr");

    clientNodePtr->sessionRef = sessionRef;
    clientNodePtr->pid        = pid;

    if (le_hashmap_Put(ConnectedClients.hashMap, sessionRef, clientNodePtr))
    {
        LE_ERROR("Failed to add connected client record for session %p.", sessionRef);
    }
    LE_INFO("Number of clients: %" PRIuS, le_hashmap_Size(ConnectedClients.hashMap));
}

//--------------------------------------------------------------------------------------------------
/**
 * Client disconnect function.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::OnClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);

    LE_INFO("Client %p Disconnected.", sessionRef);

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};

    // Remove this client from ConnectedClients hash map
    mcs_ClientNode_t *clientNodePtr =
        (mcs_ClientNode_t *)le_hashmap_Remove(ConnectedClients.hashMap, sessionRef);
    TAF_ERROR_IF_RET_NIL(clientNodePtr == nullptr,
                         "Failed to get sessionRef %p from ConnectedClients hashmap", sessionRef);
    le_mem_Release(clientNodePtr);
    LE_DEBUG("Number of clients: %" PRIuS,le_hashmap_Size(ConnectedClients.hashMap));

    // Check if this client had requested a data session
    auto &admin = tafMngdConnAdmin::GetInstance();
    le_mutex_Lock(admin.DataCtxMutex);
    le_dls_Link_t *linkPtr = le_dls_Peek(&admin.DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t *dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);

        // Check if this data id auto started or not
        if (false == dataCtxPtr->autoStart)
        {
            // Remove this client from the list of clients that have requested data
            dataCtxPtr->clients.erase(sessionRef);

            LE_INFO("Data ID %d manually started", dataCtxPtr->dataId);
            // Check if this client was the last client that requested data
            if (dataCtxPtr->clients.empty())
            {
                // If data is connected or in retrying, send request to stop data
                if (MCS_DATA_CONNECTED_ACTIVE == dataCtxPtr->adminState ||
                    MCS_DATA_CONNECTED_INACTIVE == dataCtxPtr->adminState ||
                    MCS_DATA_CONNECTED_INACTIVE_RETRYING == dataCtxPtr->adminState ||
                    MCS_DATA_CONNECTED_IDLE == dataCtxPtr->adminState ||
                    MCS_DATA_NOT_CONNECTED_RETRYING == dataCtxPtr->adminState)
                {
                    LE_INFO("Client %p is the last client that requested data", sessionRef);
                    LE_INFO("Requesting data stop for Data ID %d", dataCtxPtr->dataId);
                    // Send a data stop request
                    stateMachineEvt = {MCS_EVT_INIT, 0};
                    stateMachineEvt.event = MCS_EVT_DATA_STOP;
                    stateMachineEvt.dataId = dataCtxPtr->dataId;
                    le_event_Report(admin.StateMachineEventId, &stateMachineEvt,
                                    sizeof(stateMachineEvent_t));
                }
                // Check if connectivity recovery is scheduled and stop them
                if (dataCtxPtr->isConnectivityRecoveryScheduled)
                {
                    // Send an event to check and cancel recovery, if scheduled
                    stateMachineEvt = {MCS_EVT_INIT, 0};

                    if (MCS_RECOVERY_SCHEDULED_L1 == dataCtxPtr->adminState ||
                        MCS_RECOVERY_STARTED_L1 == dataCtxPtr->adminState)
                    {
                        LE_WARN("Cancel Radio Off/On Recovery");
                        dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON;
                    }
                    if (MCS_RECOVERY_SCHEDULED_L2 == dataCtxPtr->adminState ||
                        MCS_RECOVERY_STARTED_L2 == dataCtxPtr->adminState)
                    {
                        LE_WARN("Cancel Sim Off/On Recovery");
                        dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_SIM_OFF_ON;
                    }
                    if (MCS_RECOVERY_SCHEDULED_L3 == dataCtxPtr->adminState ||
                        MCS_RECOVERY_STARTED_L3 == dataCtxPtr->adminState)
                    {
                        LE_WARN("Cancel NAD Reboot Recovery");
                        dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_NAD_REBOOT;
                    }
                    stateMachineEvt = {MCS_EVT_INIT, 0};
                    stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_CANCEL;
                    stateMachineEvt.dataId = dataCtxPtr->dataId;
                    le_event_Report(admin.StateMachineEventId, &stateMachineEvt,
                                    sizeof(stateMachineEvent_t));
                    dataCtxPtr->isConnectivityRecoveryScheduled = false;
                }
                // Reconnection is not needed as this is the last client that requested data.
                dataCtxPtr->needReConn = false;
            }
        }
        else
        {
            LE_INFO("Number of clients still using data(id: %d) %" PRIuS,
                                                            dataCtxPtr->dataId,
                                                            dataCtxPtr->clients.size());
        }

        // Move to the next item in the list
        linkPtr = le_dls_PeekNext(&admin.DataCtxList, linkPtr);
    }
    le_mutex_Unlock(admin.DataCtxMutex);
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback thread destructor.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::callback_thread_destructor(void *contextPtr)
{
    auto &data = tafMngdConnData::GetInstance();
    data.UnregisterEvents();
    taf_dcs_DisconnectService();

    auto &radio = tafMngdConnRadio::GetInstance();
    radio.UnregisterEvents ();
    taf_radio_DisconnectService();

    auto &sim = tafMngdConnSim::GetInstance();
    sim.UnregisterEvents ();
    taf_sim_DisconnectService();
    LE_DEBUG("Disconnect done");
}
//--------------------------------------------------------------------------------------------------
/**
 * Callback thread function.
 */
//--------------------------------------------------------------------------------------------------
void *tafMngdConnAdmin::callback_thread_func(void *contextPtr)
{
    LE_DEBUG("MngdCbThread Entry");

    // Add a destructor
    le_thread_AddDestructor(callback_thread_destructor, NULL);

    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    auto &data = tafMngdConnData::GetInstance();
    data.RegisterEvents ();

    auto &radio = tafMngdConnRadio::GetInstance();
    radio.RegisterEvents ();

    auto &sim = tafMngdConnSim::GetInstance();
    sim.RegisterEvents ();

    le_sem_Post(semRef);

    le_event_RunLoop();
}

/*========================== API functions. Called by tafMngdSvc.cpp==============================*/

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data reference for the given Data ID.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataRef_t tafMngdConnAdmin::GetRefByDataId(uint8_t dataId)
{

    mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataId);

    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return NULL;
    }

    return dataCtxPtr->dataRef;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data reference for the given Data Name.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataRef_t tafMngdConnAdmin::GetRefByName(const char *dataName)
{

    mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataName);

    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return NULL;
    }

    return dataCtxPtr->dataRef;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data id for the given data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetDataIdByRef (taf_mngdConn_DataRef_t dataRef, uint8_t *dataIdPtr)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(dataIdPtr == NULL, LE_BAD_PARAMETER, "Null ptr(dataIdPtr)");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_ref_Lookup(DataRefMap, (void *)dataRef);
    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Data reference not found");
        *dataIdPtr = 0;
        return LE_NOT_FOUND;
    }
    *dataIdPtr = dataCtxPtr->dataId;
    LE_INFO("Data Id: %d", *dataIdPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data name for the given data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetDataNameByRef(taf_mngdConn_DataRef_t dataRef,
                                           char* dataName, size_t dataNameSize)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(dataName == NULL, LE_BAD_PARAMETER, "Null ptr(dataName)");
    TAF_ERROR_IF_RET_VAL(dataNameSize <= 0, LE_BAD_PARAMETER, "dataNameSize invalid value");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_ref_Lookup(DataRefMap, (void *)dataRef);
    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Data reference not found");
        *dataName = 0;
        return LE_NOT_FOUND;
    }

    le_utf8_Copy(dataName, dataCtxPtr->dataName,
                 // Use dataNameSize if it's smaller than MCS_MAX_NAME_LEN, else MCS_MAX_NAME_LEN
                 dataNameSize < MCS_MAX_NAME_LEN ? dataNameSize : MCS_MAX_NAME_LEN,
                 NULL);
    LE_INFO("Data Name: %s", dataName);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data profile number for the given data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetProfileNumberByRef(taf_mngdConn_DataRef_t dataRef,
                                                    uint8_t *dataProfileNumberPtr)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(dataProfileNumberPtr == NULL, LE_BAD_PARAMETER,
                                                            "Null ptr(dataProfileNumberPtr)");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_ref_Lookup(DataRefMap, (void *)dataRef);
    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Data reference not found");
        *dataProfileNumberPtr = 0;
        return LE_NOT_FOUND;
    }
    *dataProfileNumberPtr = static_cast<uint8_t>(dataCtxPtr->profileNumber);
    LE_INFO("Profile number: %d", *dataProfileNumberPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the phone id for the given data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetPhoneIdByRef(taf_mngdConn_DataRef_t dataRef,
                                              uint8_t *phoneIdPtr)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(phoneIdPtr == NULL, LE_BAD_PARAMETER,
                         "Null ptr(phoneIdPtr)");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_ref_Lookup(DataRefMap, (void *)dataRef);
    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Data reference not found");
        *phoneIdPtr = 0;
        return LE_NOT_FOUND;
    }
    *phoneIdPtr = static_cast<uint8_t>(dataCtxPtr->phoneId);
    LE_INFO("Phone ID: %d", *phoneIdPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add data state handler.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataStateHandlerRef_t tafMngdConnAdmin::AddDataStateHandler(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataStateHandlerFunc_t handlerPtr,
    void *contextPtr)
{
    le_event_Id_t dataStateEvent = GetDataStateEvent(dataRef);
    if (dataStateEvent == NULL)
    {
        LE_ERROR("Data event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DataStateHandler",
                                                                  dataStateEvent,
                                                                  FirstLayerDataStateHandler,
                                                                  (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_mngdConn_DataStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add recovery state handler.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_RecoveryEventHandlerRef_t tafMngdConnAdmin::AddRecoveryEventHandler(
    taf_mngdConn_RecoveryEventHandlerFunc_t handlerPtr,
    void *contextPtr)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("RecoveryEventHandler",
                                                                  recoveryEvent,
                                                                  FirstLayerRecoveryEventHandler,
                                                                  (void *)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_mngdConn_RecoveryEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data connection with the specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::Startdata(taf_mngdConn_DataRef_t dataRef)
{
    le_result_t result = LE_OK;
    mcs_DataCtx_t* dataCtxPtr = NULL;
    CmdSynchronousPromise = std::promise<le_result_t>();
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");

    dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);
    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }
    std::string state = StateToString(dataCtxPtr->adminState);
    LE_DEBUG("State is %s", state.c_str());
    //Do action according to the current state.
    if( MCS_DATA_CONNECTED_INACTIVE_RETRYING == dataCtxPtr->adminState ||
        MCS_DATA_NOT_CONNECTED_RETRYING == dataCtxPtr->adminState )
    {
        LE_INFO("Retry is in progress");
        return LE_IN_PROGRESS;
    }

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT,0};
    stateMachineEvt.event = MCS_EVT_DATA_START_SYNC;
    stateMachineEvt.dataId=dataCtxPtr->dataId;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // blocking here to get response
    result = futResult.get();

    // If data start is success and the data id is not auto started, then add this client to the
    // list of clients that have requested data start with this data id.
    le_msg_SessionRef_t sessionRef = taf_mngdConn_GetClientSessionRef();
    if ((LE_OK == result || LE_DUPLICATE == result || LE_TIMEOUT == result )
                                                        && (!dataCtxPtr->autoStart))
    {
        LE_INFO("Client %p started data for Data ID: %d", sessionRef, dataCtxPtr->dataId);
        dataCtxPtr->clients.insert(sessionRef);
    }

    // If start data failed, check if data start retry is enabled with count greated than 0.
    // If yes, add this client and also mark for reconnection
    if (LE_OK != result && dataCtxPtr->dataRetry && dataCtxPtr->dataStartRetryCount > 0)
    {
        LE_INFO("Client %p requested data for Data ID: %d", sessionRef, dataCtxPtr->dataId);
        dataCtxPtr->clients.insert(sessionRef);
        dataCtxPtr->needReConn = true;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data connection with the specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::Stopdata(taf_mngdConn_DataRef_t dataRef)
{
    le_result_t result = LE_OK;
    mcs_DataCtx_t* dataCtxPtr = NULL;
    CmdSynchronousPromise = std::promise<le_result_t>();
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");

    dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);
    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }
    // Check if a data start connection test is in progress and if it return LE_NOT_POSSIBLE. The
    // application can try again after a delay, typicaly 5s.
    if (dataCtxPtr->isDStartConnTestInProgress)
    {
        LE_WARN("Data start connection test is in progress.");
        return LE_NOT_POSSIBLE;
    }

    // Remove this client from the list of clients that have requested data start.
    le_msg_SessionRef_t sessionRef = taf_mngdConn_GetClientSessionRef();
    LE_INFO("Client %p stopped data for Data ID: %d", sessionRef, dataCtxPtr->dataId);
    dataCtxPtr->clients.erase(sessionRef);

    // If the clients list is not empty, simply return OK.
    if (!dataCtxPtr->clients.empty())
    {
        LE_INFO("Number of clients still using data(id: %d) %" PRIuS, dataCtxPtr->dataId,
                                                            dataCtxPtr->clients.size());
        LE_INFO("Return LE_OK without stopping data");
        return LE_OK;
    }

    // Last client called data stop. Stop data connection.
    LE_INFO("No more clients using data(id: %d)", dataCtxPtr->dataId);
    LE_INFO("Stopping data for Data ID %d", dataCtxPtr->dataId);
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.event=MCS_EVT_DATA_STOP_SYNC;
    stateMachineEvt.dataId=dataCtxPtr->dataId;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    //wait until return
    result = futResult.get();

    // Regardless of data stop result, check if connectivity recovery is scheduled and send
    // request to stop L1, L2 and L3 recovery. These will be handled asynchronously
    if (dataCtxPtr->isConnectivityRecoveryScheduled)
    {
        auto &admin = tafMngdConnAdmin::GetInstance();
        // Send an event to check and cancel recovery, if scheduled
        stateMachineEvt = {MCS_EVT_INIT, 0};

        if (MCS_RECOVERY_SCHEDULED_L1 == dataCtxPtr->adminState ||
            MCS_RECOVERY_STARTED_L1 == dataCtxPtr->adminState)
        {
            LE_WARN("Cancel Radio Off/On Recovery");
            dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON;
        }
        if (MCS_RECOVERY_SCHEDULED_L2 == dataCtxPtr->adminState ||
            MCS_RECOVERY_STARTED_L2 == dataCtxPtr->adminState)
        {
            LE_WARN("Cancel Sim Off/On Recovery");
            dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_SIM_OFF_ON;
        }
        if (MCS_RECOVERY_SCHEDULED_L3 == dataCtxPtr->adminState ||
            MCS_RECOVERY_STARTED_L3 == dataCtxPtr->adminState)
        {
            LE_WARN("Cancel NAD Reboot Recovery");
            dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_NAD_REBOOT;
        }
        stateMachineEvt = {MCS_EVT_INIT, 0};
        LE_INFO("Stop Data: Cancel recovery for level %d",dataCtxPtr->recoveryOperation);
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_CANCEL;
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        le_event_Report(admin.StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        dataCtxPtr->isConnectivityRecoveryScheduled = false;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection state with specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetConnectionState
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t *statePtr
)
{
    mcs_DataCtx_t* dataCtxPtr = NULL;

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(statePtr == NULL, LE_BAD_PARAMETER, "Null ptr(statePtr)");

    dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        *statePtr = TAF_MNGDCONN_DATA_DISCONNECTED;
        return LE_FAULT;
    }

    LE_INFO("MCS  State: %s", StateToString(dataCtxPtr->adminState));
    LE_INFO("Data State: %s", DataStateToString(dataCtxPtr->dataState));

    *statePtr = dataCtxPtr->dataState;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection IP addresses with specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetConnectionIPAddresses
(
    taf_mngdConn_DataRef_t dataRef,
    char *ipv4AddrPtr,
    size_t ipv4AddrSize,
    char *ipv6AddrPtr,
    size_t ipv6AddrSize
)
{
    le_result_t result = LE_OK;
    mcs_DataCtx_t* dataCtxPtr = NULL;
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    CmdSynchronousPromise = std::promise<le_result_t>();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(ipv4AddrPtr == NULL, LE_BAD_PARAMETER, "Null ptr(ipv4AddrPtr)");
    TAF_ERROR_IF_RET_VAL(ipv6AddrPtr == NULL, LE_BAD_PARAMETER, "Null ptr(ipv6AddrPtr)");

    memset(ipv4AddrPtr, 0, ipv4AddrSize);
    memset(ipv6AddrPtr, 0, ipv6AddrSize);

    dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }

    if(dataCtxPtr->dataState == TAF_MNGDCONN_DATA_DISCONNECTED)
    {
        LE_INFO("State is not active");
        return LE_OK;
    }

    stateMachineEvt.event=MCS_EVT_GET_CONNECTION_INFO_SYNC;
    stateMachineEvt.dataId=dataCtxPtr->dataId;
    //wait until return

    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    if(result == LE_OK)
    {
        switch(dataCtxPtr->ipType)
        {
            case TAF_DCS_PDP_IPV4:
                le_utf8_Copy(ipv4AddrPtr, dataCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
            break;
            case TAF_DCS_PDP_IPV6:
                le_utf8_Copy(ipv6AddrPtr, dataCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
            break;
            case TAF_DCS_PDP_IPV4V6:
                le_utf8_Copy(ipv4AddrPtr, dataCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(ipv6AddrPtr, dataCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
            break;
            default:
            break;
        }
        return LE_OK;
    }
    else
    {
        LE_ERROR("Getting IP addresses error");
        memset(ipv4AddrPtr, 0, ipv4AddrSize);
        memset(ipv6AddrPtr, 0, ipv6AddrSize);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts the data retry mechanism. If data session is connected, the data session will be
 * disconnected before data retry mechanism is started. This functions is asynchronous and
 * applications should monitor data events via DataState handler.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::StartDataRetry(taf_mngdConn_DataRef_t dataRef)
{
    TAF_ERROR_IF_RET_VAL(nullptr == dataRef, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_ref_Lookup(DataRefMap, (void *)dataRef);
    TAF_ERROR_IF_RET_VAL(nullptr == dataCtxPtr, LE_NOT_FOUND, "Data reference not found");

    CmdSynchronousPromise = std::promise<le_result_t>();
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();

    // Data start should be called first when AutoStart: No
    if (TAF_MNGDCONN_DATA_DISCONNECTED == dataCtxPtr->dataState && !dataCtxPtr->autoStart)
    {
        LE_WARN("Data start has not been called yet.");
        return LE_NOT_PERMITTED;
    }

    le_msg_SessionRef_t sessionRef = taf_mngdConn_GetClientSessionRef();
    LE_INFO("StartDataRetry for Client %p", sessionRef);

    // Send request to admin to handle this request
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.event = MCS_EVT_DATA_START_RETRY_APP_REQ;
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    stateMachineEvt.sessionRef = sessionRef;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
    // Blocking here to get response
    le_result_t result = futResult.get();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels a scheduled recovery process. This API should be called for all data references that
 * scheduled a recovery.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_POSSIBLE -- A recovery process has not been scheduled.
 *   - LE_NOT_PERMITTED -- A recovery process has already started.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::CancelRecovery(taf_mngdConn_DataRef_t dataRef)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");

    mcs_DataCtx_t* dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);
    TAF_ERROR_IF_RET_VAL(dataCtxPtr == NULL, LE_NOT_FOUND, "Data reference not found");

    if (MCS_CONNECTIONRECOVERY_LEVEL_NONE == Policy.DataSession.ConnectivityRecovery.Level)
    {
        LE_WARN("Recovery is not enabled in JSON");
        return LE_NOT_POSSIBLE;
    }

    if(!Policy.DataSession.ConnectivityRecovery.AllowCancel)
    {
        LE_WARN("CancelRecovery is not enabled in JSON");
        return LE_NOT_PERMITTED;
    }

    if(Policy.DataSession.ConnectivityRecovery.VerifyCancelingApp)
    {
        le_msg_SessionRef_t sessionRef = taf_mngdConn_GetClientSessionRef();
        // Check if an client exists in the list
        if (dataCtxPtr->clients.find(sessionRef) == dataCtxPtr->clients.end())
        {
            LE_INFO("Client doesnot exists in the list.");
            return LE_NOT_PERMITTED;
        }
    }

    LE_INFO("Cancel recovery for operation Level %d",dataCtxPtr->recoveryOperation);

    if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON)
    {
        if (MCS_RECOVERY_SCHEDULED_L1 != dataCtxPtr->adminState)
        {
            LE_WARN("Radio Off/On Recovery is not scheduled");
            return LE_NOT_POSSIBLE;
        }
        if (MCS_RECOVERY_STARTED_L1 == dataCtxPtr->adminState)
        {
            LE_WARN("Radio Off/On Recovery is already started");
            return LE_NOT_PERMITTED;
        }
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_SIM_OFF_ON)
    {
        if (MCS_RECOVERY_SCHEDULED_L2 != dataCtxPtr->adminState)
        {
            LE_WARN("Sim Off/On Recovery is not scheduled");
            return LE_NOT_POSSIBLE;
        }
        if (MCS_RECOVERY_STARTED_L2 == dataCtxPtr->adminState)
        {
            LE_WARN("Sim Off/On Recovery is already started");
            return LE_NOT_PERMITTED;
        }
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_NAD_REBOOT)
    {
        if (MCS_RECOVERY_SCHEDULED_L3 != dataCtxPtr->adminState)
        {
            LE_WARN("NAD Reboot Recovery is not scheduled");
            return LE_NOT_POSSIBLE;
        }
        if (MCS_RECOVERY_STARTED_L3 == dataCtxPtr->adminState)
        {
            LE_WARN("NAD Reboot Recovery is already started");
            return LE_NOT_PERMITTED;
        }
    }

    stateMachineEvent_t stateMachineEvt;
    // Send event to admin to cancel recovery
    stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_CANCEL_SYNC;
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();
    // Send request to admin
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // wait for result from admin
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    le_result_t result = futResult.get();
    if (LE_OK != result)
    {
        LE_WARN("Recovery Cancel for Data Id(%d) failed: %d", dataCtxPtr->dataId, result);
    }

    // Inform the admin that recovery is interrupted.
    // This shoud be sent from the main thread so that apps will recieve result first followed by
    // RecoveryEventHandler.
    stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // Return result to the application
    return result;
}


/*=====================================Event handle functions.===================================*/
//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_INIT which is sent when system startup.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventInit()
{
    // Initialize states for each data object
    le_result_t result = InitializeStates();
    if (LE_OK != result)
    {
        // Initialization did not complete. Wait for SIM/Radio events and act on them
        LE_INFO("Initialization not complete. Wait for further events");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_SET_POLICY_CONF which is sent by calling the API.
 * This is not used
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventSetPolicyConfigJSONs
(
    const char* ConfigFileNamePtr
)
{
    if ( NULL == ConfigFileNamePtr)
    {
        LE_ERROR ("Invalid Parameters passed");
        return LE_FAULT;
    }

    if((strlen(ConfigFileNamePtr) >= MCS_MAX_FILE_PATH_LEN))
    {
        LE_ERROR ("Invalid file name");
        return LE_FAULT;
    }

    if (tafMngdConnSvc_GetPolicyAndConfiguration(Policy, Configuration, ConfigFileNamePtr))
    {
        LE_DEBUG("JSONs parsed and validated");
        LE_DEBUG("Version          : %d", Policy.Version);
        LE_DEBUG("Name             : %s", Policy.Name);
        LE_DEBUG("\t\tData Connection Count : %d", Policy.DataSession.dataConnectionCount);

        for (int Index = 0; Index < Policy.DataSession.dataConnectionCount; Index++)
        {
            LE_DEBUG("\t\tData Connection[%d].Priority    : %d", Index,
                    Policy.DataSession.DataConnection[Index].Priority);
            LE_DEBUG("\t\tData Connection[%d].Use_Data_ID : %d", Index,
                    Policy.DataSession.DataConnection[Index].Use_Data_ID);
        }

        LE_DEBUG("Version          : %d", Configuration.Version);
        LE_DEBUG("Name             : %s", Configuration.Name);

        // Sim
        for (int Index = 0; Index < Configuration.SimCount; Index++)
        {
            LE_DEBUG("\tSim[%d].ID         : %d", Index,
                    Configuration.Sim[Index].ID);
            LE_DEBUG("\tSim[%d].Name       : %s", Index,
                    Configuration.Sim[Index].Name);
            LE_DEBUG("\tSim[%d].SlotNumber : %d", Index,
                    Configuration.Sim[Index].SlotNumber);
        }

        // Network
        for (int Index = 0; Index < Configuration.NetworkCount; Index++)
        {
            LE_DEBUG("\tNetwork[%d].ID           : %d", Index,
                    Configuration.Network[Index].ID);
            LE_DEBUG("\tNetwork[%d].Use_Sim_ID   : %d", Index,
                    Configuration.Network[Index].Use_Sim_ID);
            LE_DEBUG("\tNetwork[%d].PhoneID      : %d", Index,
                    Configuration.Network[Index].PhoneID);
            LE_DEBUG("\tNetwork[%d].Registration : %d", Index,
                    Configuration.Network[Index].Registration);
        }

        IsJsonValid = true;
        return InitializeStates();
    }
    else
    {
        LE_ERROR("JSON Parsing Failed");
        IsJsonValid = false;
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_RADIO_POWER_ON which is sent when system startup.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventSetRadioPowerOn()
{
    le_result_t result = LE_OK;
    auto &radio = tafMngdConnRadio::GetInstance();
    le_mutex_Lock(DataCtxMutex);
    le_dls_Link_t *linkPtr = le_dls_Peek(&DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t *dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        result = radio.StartUp(dataCtxPtr->phoneId);
        if (result != LE_OK)
        {
            LE_ERROR("Radio startup failed");
            continue;
        }
        ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
    }
    le_mutex_Unlock(DataCtxMutex);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_START and MCS_EVT_DATA_START_SYNC.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStartData(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    le_result_t result;
    mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataId);

    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }
    std::string state = StateToString(dataCtxPtr->adminState);
    LE_DEBUG("State is %s", state.c_str());
    //Do action according to the current state.
    switch(dataCtxPtr->adminState)
    {
        case MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
                // Network is not yet registed. Wait for registered event and data state will
                // happen from there.
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                LE_INFO("Registration is in progress");
                return LE_IN_PROGRESS;
            break;

        case MCS_DATA_NOT_CONNECTED_SIM_NOT_READY:
            LE_ERROR("Sim is not ready or network is not registered");
            return LE_FAULT;
            break;

        case MCS_DATA_NOT_CONNECTED_RETRYING:
        case MCS_DATA_NOT_CONNECTED_NW_REGISTERED:
        case MCS_DATA_NOT_CONNECTED:
        case MCS_DATA_NOT_CONNECTED_FAILED:
        case MCS_RECOVERY_CANCELED_L1:
        case MCS_RECOVERY_CANCELED_L2:
        case MCS_RECOVERY_CANCELED_L3:
        case MCS_RECOVERY_FAILED_L1:
        case MCS_RECOVERY_FAILED_L2:
        case MCS_RECOVERY_FAILED_L3:
            for (uint32_t dataIdx = 0; dataIdx < Configuration.DataCount; dataIdx++)
            {
                if(Configuration.Data[dataIdx].ID == dataId &&
                    Configuration.Data[dataIdx].Interface[0] != '\0')
                {
                    result = data.SetInterface(dataCtxPtr->phoneId, dataCtxPtr->profileNumber,
                        Configuration.Data[dataIdx].Interface);
                    if (result != LE_OK)
                        LE_ERROR("Failed to set interface %s for profile %d.",
                            Configuration.Data[dataIdx].Interface, dataCtxPtr->profileNumber);
                }
            }

            result = data.Startdata(dataCtxPtr->phoneId, dataCtxPtr->profileNumber,
                                    dataCtxPtr->startDataTimeout);
            if (result == LE_OK) {
                LE_INFO("StartData returned LE_OK");
                // connection is created. Wait for DCS events to proceed.
                dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE;
                return result;
            }
            else if (result == LE_DUPLICATE)
            {
                LE_INFO("StartData returned LE_DUPLICATE");
                dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE;
                /**
                 * Connection is created. However, DCS will not send events. So send
                 * MCS_EVT_DATA_CONNECTION_CONNECTED(which is sent on TAF_DCS_CONNECTED)
                 * event to the state machine.
                 */
                stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT,0};
                stateMachineEvt.event=MCS_EVT_DATA_CONNECTION_CONNECTED;
                stateMachineEvt.dataId = dataCtxPtr->dataId;
                le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
                return result;
            }
            else if(result == LE_TIMEOUT)
            {
                LE_INFO("StartData timedout. Monitor DataState Events");
                return result;
            }
            else {
                LE_ERROR("Starting a data call failed. Retrying ...");
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_RETRYING;
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                // Send MCS_EVT_DATA_START_RETRY event to admin to handle accordingly
                stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
                stateMachineEvt.event = MCS_EVT_DATA_START_RETRY;
                stateMachineEvt.dataId = dataCtxPtr->dataId;
                le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
                return LE_IN_PROGRESS;
            } break;

            case MCS_DATA_CONNECTED_ACTIVE:
        case MCS_DATA_CONNECTED_INACTIVE:
            LE_INFO("Already connected");
            return LE_DUPLICATE;
            break;

        default:
            LE_INFO("Default case. Returning LE_FAULT from here. State is %s", state.c_str());
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the back off time interval
 */
//--------------------------------------------------------------------------------------------------
uint32_t tafMngdConnAdmin::CalculateBackOffInterval(uint16_t IntervalInSec,
                                                    uint8_t step,
                                                    uint8_t retryCount)
{
    LE_INFO("Interval    : %d sec", IntervalInSec);
    LE_INFO("Step        : %d", step);
    LE_INFO("Retry Count : %d", retryCount);
    uint32_t intervalInMilliSec = 0;
    if (1 == step || 1 == retryCount)
    {
        // Interval is always the same
        intervalInMilliSec = IntervalInSec * 1000;
    }
    else if (2 ==step && retryCount > 1)
    {
        // Cap retryCount to prevent overflow
        uint8_t safeRetryCount = (retryCount > 5) ? 5 : retryCount;
        uint32_t multiplier = 1U << (safeRetryCount - 1U);
        intervalInMilliSec = multiplier * IntervalInSec * 1000;
    }

    LE_INFO("Back off interval = %d ms", intervalInMilliSec);
    return intervalInMilliSec;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_START_RETRY
 * When this event is received, it means that data start failed or data disconnected after it was
 * started. From here, a retry timer will be started, with appropriate back-off.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStartDataRetry(uint8_t dataId)
{

    // Get the context for the data ID
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);

    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Unable to find context for data ID: %d", dataId);
        return LE_FAULT;
    }

    if(!dataCtxPtr->dataRetry)
    {
        LE_ERROR("Data retry disabled.");
        dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_FAILED;
        //Since the data connection has failed remove all the clients
        if(!dataCtxPtr->clients.empty() &&
            MCS_CONNECTIONRECOVERY_LEVEL_NONE == Policy.DataSession.ConnectivityRecovery.Level)
        {
            LE_INFO("Connection Failed. Clear all clients for Data ID: %d", dataCtxPtr->dataId);
            dataCtxPtr->clients.clear();
        }
        ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTION_FAILED);
        return LE_NOT_POSSIBLE;
    }

    // Check if timer is running, ideally it should not be running.
    if (le_timer_IsRunning(dataCtxPtr->dataStartRetryTimerRef))
    {
        LE_INFO("Timer Running");
        // Stop the timer
        le_timer_Stop(dataCtxPtr->dataStartRetryTimerRef);
    }

    LE_INFO("Data Id: %d, Retries: %d", dataCtxPtr->dataId, dataCtxPtr->dataStartRetryCount);
    if (dataCtxPtr->dataStartRetryCount >= dataCtxPtr->maxdataRetryCount)
    {
        // It is not possible to proceed with the retry mechanism
        LE_ERROR("Data retry count exceeded. Data connection FAILED.");
        dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_FAILED;
        //Since the data connection has failed remove all the clients
        if(!dataCtxPtr->clients.empty() &&
            MCS_CONNECTIONRECOVERY_LEVEL_NONE == Policy.DataSession.ConnectivityRecovery.Level)
        {
            LE_INFO("Connection Failed. Clear all clients for Data ID: %d", dataCtxPtr->dataId);
            dataCtxPtr->clients.clear();
        }
        ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTION_FAILED);
        return LE_NOT_POSSIBLE;
    }

    le_timer_SetHandler(dataCtxPtr->dataStartRetryTimerRef, DataRetryTimerHandler);
    le_timer_SetContextPtr(dataCtxPtr->dataStartRetryTimerRef, (void *)dataCtxPtr);

    // Increment the retry count number
    dataCtxPtr->dataStartRetryCount = (dataCtxPtr->dataStartRetryCount) + 1;

    // Set the retry count back off period
    le_timer_SetMsInterval(dataCtxPtr->dataStartRetryTimerRef,
                           CalculateBackOffInterval(dataCtxPtr->dataRetryBackoffIntervalInSec,
                                                    dataCtxPtr->dataRetryBackoffIntervalStep,
                                                    dataCtxPtr->dataStartRetryCount));

    // Start the data start retry timer
    le_timer_Start(dataCtxPtr->dataStartRetryTimerRef);

    // Return IN PROGRESS signalling retry timer is running.
    return LE_IN_PROGRESS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_START_RETRY_APP_REQ
 * When this event is received, it means app has requested the service to perform data start retry.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStartDataRetryAppReq(uint8_t dataId,
                                                        le_msg_SessionRef_t ref)
{
    // Get the context for the data ID
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    //session reference of the client
    le_msg_SessionRef_t sessionRef = ref;
    LE_INFO("EventStartDataRetryAppReq for Client %p", sessionRef);

    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Unable to find context for data ID: %d", dataId);
        return LE_FAULT;
    }

    if(!Policy.DataSession.AppMngdConnectivityRecovery.Enable)
    {
        LE_WARN("App triggered StartDataRetry is not enabled in JSON");
        return LE_NOT_PERMITTED;
    }

    if(Policy.DataSession.AppMngdConnectivityRecovery.VerifyCallingApp)
    {
        // Check if a client exists in the list
        if (dataCtxPtr->clients.find(sessionRef) == dataCtxPtr->clients.end())
        {
            LE_INFO("Client %p doesnot exists in the list.", sessionRef);
            return LE_NOT_PERMITTED;
        }
    }

    // Check if a minTimeBetweenTriggers timer is active
    // Subsequent calls within this time will return LE_NOT_POSSIBLE.
    if (le_timer_IsRunning(dataCtxPtr->minTimeBetweenTriggersRef))
    {
        LE_WARN("minTimeBetweenTriggers is still running");
        return LE_NOT_POSSIBLE;
    }

    // Check if a periodic connectivity check timer is active and stop it.
    // This might be the case for data that is auto started.
    if (le_timer_IsRunning(dataCtxPtr->periodicConnectivityTestTimerRef))
    {
        le_timer_Stop(dataCtxPtr->periodicConnectivityTestTimerRef);
    }

    // Check if a maxTimeBetweenTriggers timer is active
    if (le_timer_IsRunning(dataCtxPtr->maxTimeBetweenTriggersRef))
    {
        //Check if the same client has called the api
        if (le_hashmap_ContainsKey(RetryClients.hashMap, sessionRef))
        {
            mcs_RetryClientNode_t* retryClient =
                        (mcs_RetryClientNode_t*)le_hashmap_Get(RetryClients.hashMap, sessionRef);
            if(retryClient == NULL)
            {
                LE_ERROR("retryClient hashmap is null");
                return LE_FAULT;
            }
            //If so then check if StartDataRetry is done and go to recovery
            if(retryClient->flag)
            {
                LE_INFO("Data (%d) not connected.", dataCtxPtr->dataId);
                LE_INFO("Scheduling connectivity recovery from app requested DataStartRetry.");
                stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
                stateMachineEvt.dataId = dataCtxPtr->dataId;
                stateMachineEvt.event  = MCS_EVT_CONN_RECOVERY_SCHEDULE_L1;
                le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
                return LE_OK;
            }
        }
        //Else return LE_BUSY
        {
            LE_WARN("maxTimeBetweenTriggers is still running for other client");
            return LE_BUSY;
        }

    }

    // Start the retry procedure only in the active connected state.
    if (MCS_DATA_CONNECTED_ACTIVE == dataCtxPtr->adminState)
    {
        //Set the context with session ref to be passed for MaxTimeBetweenTriggersHandler
        dataCtxPtr->sessionRef = sessionRef;
        le_timer_SetContextPtr(dataCtxPtr->maxTimeBetweenTriggersRef, dataCtxPtr);
        // Start the maxTimeBetweenTriggers and minTimeBetweenTriggers timer
        le_timer_Start(dataCtxPtr->minTimeBetweenTriggersRef);
        le_timer_Start(dataCtxPtr->maxTimeBetweenTriggersRef);

        //Add the new client in the retryClient map and change the flag of DataStartRetried as true
        LE_INFO("Client %p started data retry for Data ID: %d", sessionRef, dataCtxPtr->dataId);
        mcs_RetryClientNode_t* retryClientNodePtr =
            (mcs_RetryClientNode_t *)le_mem_TryAlloc(RetryClients.memPool);
        if(retryClientNodePtr == NULL)
        {
            LE_ERROR("retryClientNodePtr is null");
            return LE_FAULT;
        }
        retryClientNodePtr->sessionRef = sessionRef;
        retryClientNodePtr->flag       = true;
        if (le_hashmap_Put(RetryClients.hashMap, sessionRef, retryClientNodePtr))
        {
            LE_ERROR("Failed to add retried client record for session %p.", sessionRef);
        }

        LE_INFO("StartDataRetry triggered for Data (%d)", dataCtxPtr->dataId);
        dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE_RETRYING;
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.event = MCS_EVT_DATA_STOP;
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_STOP_SYNC and MCS_EVT_DATA_STOP
 * which is sent by calling API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStopData(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    le_result_t result;

    mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataId);
    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context");
        return LE_FAULT;
    }

    // Check if the data is started automatically by the service.
    // If true, return LE_NOT_PERMITTED
    // If false, allow DataStop to proceed.
    if (true == dataCtxPtr->autoStart &&
        dataCtxPtr->adminState != MCS_DATA_CONNECTED_INACTIVE_RETRYING)
    {
        LE_INFO("%s",StateToString(dataCtxPtr->adminState));
        LE_WARN("Stopping auto started(Autostart: Yes) data session is not allowed");
        return LE_NOT_PERMITTED;
    }

    //Do action according to the current state.
    switch(dataCtxPtr->adminState)
    {
        case MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
            LE_ERROR("Network not registered. Data is not started");
            return LE_FAULT;
        case MCS_DATA_NOT_CONNECTED_SIM_NOT_READY:
            LE_ERROR("SIM not ready. Data is not started");
            return LE_FAULT;
        case MCS_DATA_NOT_CONNECTED_NW_REGISTERED:
        case MCS_DATA_NOT_CONNECTED:
            // DataStart has not been called yet.
            LE_ERROR("Data not started. DataStart should be called first.");
            return LE_FAULT;
        case MCS_DATA_CONNECTED_INACTIVE_RETRYING:
            result = data.Stopdata(dataCtxPtr->phoneId, dataCtxPtr->profileNumber,
                                    dataCtxPtr->stopDataTimeout);
            if(result == LE_OK)
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_INACTIVE_RETRYING;
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                if (dataCtxPtr->dataConnTestFailedRetryCount >=
                   dataCtxPtr->maxdataRetryCount)
                {
                    // It is not possible to proceed with the retry mechanism
                    dataCtxPtr->dataConnTestFailedRetryCount = 0;
                    LE_ERROR("Data retry count exceeded. Data connection FAILED.");
                    dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_FAILED;
                    //Since the data connection has failed remove all the clients
                    if(!dataCtxPtr->clients.empty() &&
                        MCS_CONNECTIONRECOVERY_LEVEL_NONE ==
                        Policy.DataSession.ConnectivityRecovery.Level)
                    {
                        LE_INFO("Connection Failed. Clear all clients for Data ID: %d",
                                dataCtxPtr->dataId);
                        dataCtxPtr->clients.clear();
                    }
                    ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTION_FAILED);
                    return LE_NOT_POSSIBLE;
                }
                dataCtxPtr->dataStartRetryCount = dataCtxPtr->dataConnTestFailedRetryCount;
                //Set the retry count to 1
                dataCtxPtr->dataConnTestFailedRetryCount += 1;
                //If manually stopped the data successfully. Set reconnection flag to false.
                dataCtxPtr->needReConn = false;
                return LE_OK;
            }
            else if(result == LE_TIMEOUT)
            {
                LE_INFO("StopData timedout. Monitor DataState Events");
            }
            else if (result == LE_NOT_FOUND)
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                LE_INFO("Data session has been removed internally.");
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                return LE_OK;
            }
            else
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                LE_ERROR("Stopping data failed");
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                return LE_FAULT;
            }
            break;
        case MCS_DATA_CONNECTED_ACTIVE:
            result = data.Stopdata(dataCtxPtr->phoneId, dataCtxPtr->profileNumber,
                                    dataCtxPtr->stopDataTimeout);
            if(result == LE_OK)
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                //If manually stopped the data successfully. Set reconnection flag to false.
                dataCtxPtr->needReConn = false;
                return LE_OK;
            }
            else if(result == LE_TIMEOUT)
            {
                LE_INFO("StopData timedout. Monitor DataState Events");
            }
            else
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                LE_ERROR("Stopping data failed");
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                return LE_FAULT;
            }
            break;
        case MCS_DATA_NOT_CONNECTED_RETRYING:
            dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
            //If manually stopped the data successfully. Set reconnection flag to false.
            dataCtxPtr->needReConn = false;
            LE_INFO("Cancel retrying...");
            le_timer_Stop(dataCtxPtr->dataStartRetryTimerRef);
            break;
        case MCS_RECOVERY_SCHEDULED_L1:
        case MCS_RECOVERY_SCHEDULED_L2:
        case MCS_RECOVERY_SCHEDULED_L3:
            LE_INFO("%s",StateToString(dataCtxPtr->adminState));
            break;
        default:
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_GET_CONNECTION_INFO_SYNC which is sent by calling API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventGetConnectionInfo(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    le_result_t result;

    if(dataId != 0xff)
    {
        mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataId);
        if(dataCtxPtr == NULL)
        {
            LE_ERROR("Can't find the context");
            return LE_FAULT;
        }
        result = data.GetConnectionInfo(dataCtxPtr);
    }
    else
    {
        profileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
        int listSize;
        if(getProfileList(profilesInfoPtr, &listSize) == LE_OK && listSize > 0)
        {
            result = data.GetAllConnectionInfo(profilesInfoPtr, listSize);
        }
        else
            result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_SIM_READY which is sent by SIM module when SIM status is
 * ready.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventSimReadyState(uint8_t slotId)
{
    le_dls_Link_t* linkPtr = NULL;
    auto &radio = tafMngdConnRadio::GetInstance();
    le_result_t result;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if(slotId == dataCtxPtr->slotId)
        {
            if(dataCtxPtr->adminState == MCS_DATA_NOT_CONNECTED_SIM_NOT_READY ||
               dataCtxPtr->adminState == MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED)
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_SIM_READY;
                result = radio.StartUp(dataCtxPtr->phoneId);
                if(result != LE_OK)
                {
                    LE_ERROR("Radio startup failed");
                    continue;
                }
            }

            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
        }

    }

    le_mutex_Unlock(DataCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_SIM_NOT_READY which is sent by SIM module when SIM status is
 * not ready.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventSimNotReadyState(uint8_t slotId)
{

    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if(slotId == dataCtxPtr->slotId)
        {
            if(dataCtxPtr->adminState == MCS_DATA_NOT_CONNECTED_SIM_READY)
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_SIM_NOT_READY;

            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
        }
    }

    le_mutex_Unlock(DataCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_NETWORK_REG_STATE which is sent by radio module when network
 * status is registered.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventNetworkRegState(uint8_t phoneId)
{

    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if(phoneId == dataCtxPtr->phoneId)
        {
            LE_INFO("current admin state= %d, autoStart = %d, needReConn=%d",
                     dataCtxPtr->adminState, dataCtxPtr->autoStart, dataCtxPtr->needReConn);
            //Do action according to the current state.
            switch(dataCtxPtr->adminState)
            {
                case MCS_DATA_NOT_CONNECTED_SIM_NOT_READY:
                case MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
                case MCS_DATA_NOT_CONNECTED_SIM_READY:
                case MCS_DATA_NOT_CONNECTED_RETRYING:
                    dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_NW_REGISTERED;
                    //Start a data call if autoStart, or reconnection flag is true
                    if(dataCtxPtr->autoStart || dataCtxPtr->needReConn)
                    {
                        // Send MCS_EVT_DATA_START event to admin
                        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
                        stateMachineEvt.event = MCS_EVT_DATA_START;
                        stateMachineEvt.dataId = dataCtxPtr->dataId;
                        le_event_Report(StateMachineEventId, &stateMachineEvt,
                                                            sizeof(stateMachineEvent_t));
                    }
                    else
                    {
                        // Wait for user to call DataStart()
                        dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    le_mutex_Unlock(DataCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_NETWORK_UNREG_STATE which is sent by radio module when
 * network status is unregistered.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventNetworkUnregState(uint8_t phoneId)
{

    LE_INFO("EventNetworkUnregState. Phone ID: %d", phoneId);
    le_dls_Link_t *linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if(phoneId == dataCtxPtr->phoneId)
        {
            //Do action according to the current state.
            switch(dataCtxPtr->adminState)
            {
                case MCS_DATA_NOT_CONNECTED_RETRYING:
                    // Stop data start retry timer if it's running
                    if (le_timer_IsRunning(dataCtxPtr->dataStartRetryTimerRef))
                    {
                        le_timer_Stop(dataCtxPtr->dataStartRetryTimerRef);
                    }
                    break;
                default:
                    break;
            }
            // Update service state and report DISCONNECTED event
            LE_INFO("Report data disconnected event");
            dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
        }
    }
    le_mutex_Unlock(DataCtxMutex);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_CONNECTION_CONNECTED which is sent by data module when
 * the data connection is created.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataConnected(uint8_t dataId)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    mcs_DataCtx_t* dataCtxPtr = GetDataCtx(dataId);
    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context for dataId(%d)", dataId);
        return;
    }

    //Check for interface
    profileRef = taf_dcs_GetProfileEx (dataCtxPtr->phoneId, dataCtxPtr->profileNumber);

    result = taf_dcs_GetInterfaceName(profileRef, dataCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Getting interface name failed for dataID %d",dataCtxPtr->dataId);
        return;
    }
    LE_INFO("Interface Name for DataID: %d is %s", dataId, dataCtxPtr->intfName);

    //Check for corresponding DNS
    result = taf_dcs_GetIPv4DNSAddresses(profileRef, dataCtxPtr->dns1Addr,
            MCS_MAX_IPV4_LEN,dataCtxPtr->dns2Addr,MCS_MAX_IPV4_LEN);

    if(result != LE_OK)
    {
        LE_ERROR("Getting dns address failed for dataID %d",dataCtxPtr->dataId);
        return;
    }
    LE_INFO("DNS addresses for DataID: %d are %s and %s", dataId, dataCtxPtr->dns1Addr,
            dataCtxPtr->dns2Addr);
#ifndef LE_CONFIG_TARGET_SIMULATION
    // Set the DNS for the interface. The Net service will do the heavy lifting.
    result = taf_net_SetDNS (dataCtxPtr->intfName);
    if(result != LE_OK)
    {
        LE_ERROR("Set DNS fialed for %d",dataCtxPtr->dataId);
        return;
    }
    LE_INFO ("DNS set for dataID %d",dataCtxPtr->dataId);
#else
    LE_ERROR("DNS not set for Simulation target");
#endif

    // Set the correct data context state
    dataCtxPtr->adminState = MCS_DATA_START_CONNECTIONTEST_START;

    // Send an event to start DataStartConnectionTest
    LE_INFO("Sending event to start DataStartConnectionTest for ID: %d", dataId);
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.event = MCS_EVT_DATA_START_CONNECTIONTEST;
    stateMachineEvt.dataId = dataCtxPtr->dataId;

    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE which is sent by data module
 * when the data connection is started and data start connection tests succeed.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataConnectedActive(uint8_t dataId)
{
    LE_INFO ("Data(%d) is CONNECTED ACTIVE", dataId);

    // Reset the recovery performed flag
    mcs_DataCtx_t *dataCtxPtr = NULL;
    dataCtxPtr = GetDataCtx(dataId);
    if (dataCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context for dataId(%d)", dataId);
        return;
    }
    dataCtxPtr->wasL1ConnectivityRecoveryDone = false;
    // Start the Periodic Connection Test timer if URL is not null
    if(dataCtxPtr->conn_periodic_test_url[0] != '\0')
    {
        LE_INFO("Periodic Connection Test Interval: %d ms",
                    dataCtxPtr->conn_periodic_test_interval*1000);
        le_timer_SetMsInterval(dataCtxPtr->periodicConnectivityTestTimerRef,
                    dataCtxPtr->conn_periodic_test_interval*1000);
        le_timer_Start(dataCtxPtr->periodicConnectivityTestTimerRef);
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_CONNECTION_DISCONNECTED which is sent by data module
 * when the data connection is destroyed.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataDisconnected(uint8_t dataId)
{
    LE_DEBUG("EventDataDisconnected-Start");
    mcs_DataCtx_t* dataCtxPtr = NULL;
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    dataCtxPtr = GetDataCtx(dataId);
    if(dataCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context for dataId(%d)", dataId);
        return;
    }


    LE_DEBUG("AdminState : %d(%s)", dataCtxPtr->adminState,
                                    mngdConnAdmin.StateToString(dataCtxPtr->adminState));

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT,0};
    //Do action according to the current state.
    switch(dataCtxPtr->adminState)
    {
        // Data disconnected from ACTIVE state
        case MCS_DATA_NOT_CONNECTED_INACTIVE_RETRYING:

            //No need to check for autoStart as this will come in case of StartDataRetryAppReq
            //So we directly go for retrying state
            dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_RETRYING;
            LE_INFO("Data call disconnected, retrying");
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
            // Send MCS_EVT_DATA_START_RETRY event to the admin to handle accordingly

            stateMachineEvt.event = MCS_EVT_DATA_START_RETRY;
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
            break;

        case MCS_DATA_CONNECTED_ACTIVE:
        {
            // Check if autoStart or needReConn are true before starting the retry mechanism
            if (dataCtxPtr->autoStart || dataCtxPtr->needReConn)
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_RETRYING;
                LE_INFO("Data call disconnected, retrying");
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                // Send MCS_EVT_DATA_START_RETRY event to the admin to handle accordingly
                stateMachineEvt.event = MCS_EVT_DATA_START_RETRY;
                stateMachineEvt.dataId = dataCtxPtr->dataId;
                le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
            }
            else
            {
                // If autoStart is false, do not start the retry mechanism
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
            }
            break;
        }
    // If DataStartConnectionTest fails, we stop the data and then let the state handler to retry

        default:
            break;
    }

}

/*===================================End Event process functions.=================================*/
//--------------------------------------------------------------------------------------------------
/**
 * StateMachineEvtThread Destructor
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::StateMachineEvtThreadDestructorFunc(void *contextPtr)
{
    LE_INFO("Disconnect from services");
    taf_radio_DisconnectService();
    taf_dcs_DisconnectService();
    taf_sim_DisconnectService();
    taf_pm_DisconnectService();
#ifndef LE_CONFIG_TARGET_SIMULATION
    taf_net_DisconnectService();
    taf_mngdPm_DisconnectService();
    taf_ecall_DisconnectService();
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * StateMachineEventThreadFunc.
 */
//--------------------------------------------------------------------------------------------------
void *tafMngdConnAdmin::StateMachineEventThreadFunc(void *contextPtr)
{
    // Add a destructor
    le_thread_AddDestructor(StateMachineEvtThreadDestructorFunc, NULL);

    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();

    taf_pm_ConnectService();
    taf_radio_ConnectService();
    taf_dcs_ConnectService();
    taf_sim_ConnectService();
#ifndef LE_CONFIG_TARGET_SIMULATION
    taf_net_ConnectService();
    taf_mngdPm_ConnectService();
    taf_ecall_ConnectService();
#endif

    // internal event handler
    mngdConnAdmin.StateMachineEventId = le_event_CreateId("Sm Event", sizeof(stateMachineEvent_t));
    le_event_AddHandler("StateMachine Event Handler", mngdConnAdmin.StateMachineEventId,
                        StateMachineEvtHandlerFunc);

    taf_pm_AddStateChangeHandler(tafMngdConnAdmin::PowerStateChangeHandler, nullptr);

    le_sem_Post(semRef);

    LE_INFO("Create event loop for state machine event");

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset Data Retry and periodic connectivity check values for all Data Id contexts
 * Stop timer if it's running.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ResetDataRetryPeriodicConnCheckValues()
{
    LE_INFO("Reset Data Retry Values for all Data Ids");

    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        LE_INFO("Reset Data Retry Values for Data Id: %d", dataCtxPtr->dataId);
        // Stop timer if it is running
        if (le_timer_IsRunning(dataCtxPtr->dataStartRetryTimerRef))
        {
            le_timer_Stop(dataCtxPtr->dataStartRetryTimerRef);
        }
        dataCtxPtr->dataStartRetryCount = 0;
        // Stop periodic connectivity check timer if it's running
        if (le_timer_IsRunning(dataCtxPtr->periodicConnectivityTestTimerRef))
        {
            le_timer_Stop(dataCtxPtr->periodicConnectivityTestTimerRef);
        }
        // Reset the retryCount
        dataCtxPtr->conn_periodic_test_retryCount = 1;
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset Data Retry and periodic connectivity check values for specific Data Id contexts.
 * Stop timer if it's running.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ResetDataRetryPeriodicConnCheckValues(uint8_t dataId)
{
    LE_INFO("Reset Data Retry Values for Data Id: %d", dataId);
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (NULL == dataCtxPtr)
    {
        return;
    }
    // Stop timer if it is running
    if (le_timer_IsRunning(dataCtxPtr->dataStartRetryTimerRef))
    {
        le_timer_Stop(dataCtxPtr->dataStartRetryTimerRef);
    }
    dataCtxPtr->dataStartRetryCount = 0;
    // Stop periodic connectivity check timer if it's running
    if (le_timer_IsRunning(dataCtxPtr->periodicConnectivityTestTimerRef))
    {
        le_timer_Stop(dataCtxPtr->periodicConnectivityTestTimerRef);
    }
    // Reset the retryCount
    dataCtxPtr->conn_periodic_test_retryCount = 1;
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * StateMachineEvtHandlerFunc.
 * SYNC commands are API calls from applications. Tthe result will be passed back to the calling
 * application.
 * Non SYNC commands will send relevant events to be handled appropriately.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::StateMachineEvtHandlerFunc(void *reqPtr)
{
    le_result_t result = LE_OK;
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    stateMachineEvent_t* eventReq = (stateMachineEvent_t*)reqPtr;

    if(eventReq == NULL)
    {
        LE_ERROR ("Invalid Parameters passed");
        return;
    }

    LE_INFO("STATE MACHINE --received event=%s for data ID: %d",
                                            mngdConnAdmin.EventToString(eventReq->event),
                                            eventReq->dataId);

    switch (eventReq->event) {
        case MCS_EVT_INIT:
            mngdConnAdmin.EventInit();
            break;

        case MCS_EVT_SET_POLICY_CONF_SYNC:
            result = mngdConnAdmin.EventSetPolicyConfigJSONs(mngdConnAdmin.ConfigFileName);
            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;

        case MCS_RADIO_POWER_ON:
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues();
            mngdConnAdmin.EventSetRadioPowerOn();
            break;

        case MCS_EVT_DATA_START_SYNC:
            result = mngdConnAdmin.EventStartData(eventReq->dataId);
            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;

        case MCS_EVT_DATA_START:
            mngdConnAdmin.EventStartData(eventReq->dataId);
            break;

        case MCS_EVT_DATA_START_RETRY:
            mngdConnAdmin.EventStartDataRetry(eventReq->dataId);
            break;

        case MCS_EVT_DATA_START_RETRY_APP_REQ:
            result = mngdConnAdmin.EventStartDataRetryAppReq(eventReq->dataId, eventReq->sessionRef);
            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;

        case MCS_EVT_DATA_STOP_SYNC:
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues(eventReq->dataId);
            result = mngdConnAdmin.EventStopData(eventReq->dataId);
            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;

        case MCS_EVT_DATA_STOP:
        {
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues(eventReq->dataId);
            mngdConnAdmin.EventStopData(eventReq->dataId);
            break;
        }

        case MCS_EVT_DATA_PERIODIC_CONNECTIONTEST:
        {
            LE_DEBUG("Starting Periodic Connection test");
            mngdConnAdmin.EventDataPeriodicConnectivityTest(eventReq->dataId);
            break;
        }

        case MCS_EVT_GET_CONNECTION_INFO_SYNC:
            result = mngdConnAdmin.EventGetConnectionInfo(eventReq->dataId);
            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;

        case MCS_EVT_SIM_READY:
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues();
            mngdConnAdmin.EventSimReadyState(eventReq->slotId);
            break;

        case MCS_EVT_SIM_NOT_READY:
             mngdConnAdmin.EventSimNotReadyState(eventReq->slotId);
            break;

        case MCS_EVT_NETWORK_REG_STATE:
            LE_DEBUG("phoneid=%d", eventReq->phoneId);
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues();
            mngdConnAdmin.EventNetworkRegState(eventReq->phoneId);
            break;

        case MCS_EVT_NETWORK_UNREG_STATE:
            LE_DEBUG("phoneid=%d", eventReq->phoneId);
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues();
            mngdConnAdmin.EventNetworkUnregState(eventReq->phoneId);
            break;

        case MCS_EVT_DATA_CONNECTION_CONNECTED:
            mngdConnAdmin.ResetDataRetryPeriodicConnCheckValues(eventReq->dataId);
            mngdConnAdmin.EventDataConnected(eventReq->dataId);
            break;

        case MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE:
            mngdConnAdmin.EventDataConnectedActive(eventReq->dataId);
            break;

        case MCS_EVT_DATA_CONNECTION_DISCONNECTED:
            mngdConnAdmin.EventDataDisconnected(eventReq->dataId);
            break;

        case MCS_EVT_DATA_START_CONNECTIONTEST:
            LE_DEBUG("Starting Connection test");
            mngdConnAdmin.EventDataStartConnectionTest(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L1:
            LE_DEBUG("Radio Off/On Connectivity Recovery Schedule event");
            mngdConnAdmin.EventL1ConnRecoverySchedule(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_CANCEL:
            LE_DEBUG("Connectivity Recovery Cancel event");
            mngdConnAdmin.EventConnRecoveryCancel(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_CANCEL_SYNC:
            LE_DEBUG("Connectivity Recovery Cancel SYNC event");
            mngdConnAdmin.EventConnRecoveryCancelSync(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_CANCEL_SEND_IND:
            LE_DEBUG("Connectivity Recovery send indication");
            mngdConnAdmin.EventConnRecoveryCancelSendInd(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_INTERRUPTED:
            LE_DEBUG("Connectivity Recovery interrupted");
            mngdConnAdmin.EventConnRecoveryInterrupted(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_START_L1:
            LE_DEBUG("Radio Off/On Connectivity Recovery Start event");
            mngdConnAdmin.EventL1ConnRecoveryStart(eventReq->dataId);
        break;

        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L2:
            LE_DEBUG("Sim Off/On Connectivity Recovery Schedule event");
            mngdConnAdmin.EventL2ConnRecoverySchedule(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_START_L2:
            LE_DEBUG("Sim Off/On Connectivity Recovery Start event");
            mngdConnAdmin.EventL2ConnRecoveryStart(eventReq->dataId);
        break;

        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L3:
            LE_DEBUG("NAD Reboot Connectivity Recovery Schedule event");
            mngdConnAdmin.EventL3ConnRecoverySchedule(eventReq->dataId);
            break;

        case MCS_EVT_CONN_RECOVERY_START_L3:
            LE_DEBUG("NAD Reboot Connectivity Recovery Start event");
            mngdConnAdmin.EventL3ConnRecoveryStart(eventReq->dataId);
            break;

        default:
            LE_ERROR("Undefined request received.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connectivity context by data id.
 */
//--------------------------------------------------------------------------------------------------
mcs_DataCtx_t* tafMngdConnAdmin::GetDataCtx(uint8_t dataId)
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if (dataCtxPtr->dataId == dataId)
        {
            le_mutex_Unlock(DataCtxMutex);
            return dataCtxPtr;
        }
    }

    le_mutex_Unlock(DataCtxMutex);
    LE_WARN ("Data context not found for id: %d", dataId);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connectivity context by data name.
 */
//--------------------------------------------------------------------------------------------------
mcs_DataCtx_t* tafMngdConnAdmin::GetDataCtx(const char *dataName)
{
    LE_DEBUG("Enter GetDataCtx: DataName %s ", dataName);
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if (strcmp(dataCtxPtr->dataName,dataName) == 0)
        {
            le_mutex_Unlock(DataCtxMutex);
            return dataCtxPtr;
        }
    }

    le_mutex_Unlock(DataCtxMutex);
    LE_WARN ("Data context not found for name: %s", dataName);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connectivity context by phone id and profileNumber.
 */
//--------------------------------------------------------------------------------------------------
mcs_DataCtx_t* tafMngdConnAdmin::GetDataCtx(uint8_t phoneId, uint32_t profileNumber)
{
    le_dls_Link_t* linkPtr = NULL;
    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if (dataCtxPtr->phoneId == phoneId && dataCtxPtr->profileNumber == profileNumber)
        {
            le_mutex_Unlock(DataCtxMutex);
            return dataCtxPtr;
        }
    }

    le_mutex_Unlock(DataCtxMutex);
    LE_WARN ("Data context not found for phoneId: %d and profileNumber %d", phoneId, profileNumber);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MCS state machine event ID for use by other objects.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t tafMngdConnAdmin::GetStateMachineEventId()
{
    // return the state machine event id
    return StateMachineEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create connectivity context.
 */
//--------------------------------------------------------------------------------------------------
mcs_DataCtx_t *
tafMngdConnAdmin::CreateDataCtx(
    uint8_t dataId,
    uint8_t slotId,
    uint8_t phoneId,
    uint8_t profileNumber,
    char dataName[MCS_MAX_NAME_LEN],
    bool autoStart,
    char *conn_test_url,
    char *conn_test_ipv4Addr)
{
    char timerName[32] = {0};
    char eventName[32] = {0};
    mcs_DataCtx_t* dataCtxPtr = NULL;
    dataCtxPtr = (mcs_DataCtx_t*)le_mem_ForceAlloc(DataCtxPool);
    TAF_ERROR_IF_RET_VAL(dataCtxPtr == NULL, NULL, "cannot alloc dataCtxPtr");

    dataCtxPtr->dataId = dataId;
    dataCtxPtr->slotId = slotId;
    dataCtxPtr->phoneId = phoneId;
    dataCtxPtr->dataStartRetryCount = 0;
    dataCtxPtr->conn_periodic_test_retryCount = 1;
    dataCtxPtr->profileNumber = profileNumber;
    le_utf8_Copy(dataCtxPtr->dataName,dataName,
                             MCS_MAX_NAME_LEN,NULL);
    dataCtxPtr->autoStart = autoStart;
    dataCtxPtr->needReConn = false;
    dataCtxPtr->adminState = MCS_ADMIN_INIT;
    dataCtxPtr->dataState = TAF_MNGDCONN_DATA_DISCONNECTED;
    dataCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
    dataCtxPtr->dataConnTestFailedRetryCount = 0;
    dataCtxPtr->isConnectivityRecoveryScheduled = false;
    dataCtxPtr->wasL1ConnectivityRecoveryDone = false;
    dataCtxPtr->isDStartConnTestInProgress = false;
    dataCtxPtr->link = LE_DLS_LINK_INIT;

    if(conn_test_url != NULL)
    {
        le_utf8_Copy(dataCtxPtr->conn_test_url, conn_test_url,
                    MCS_MAX_CONNECTION_URL_LEN,NULL);
    }
    if(conn_test_ipv4Addr != NULL)
    {
        le_utf8_Copy(dataCtxPtr->conn_test_ipv4Addr, conn_test_ipv4Addr,
                    MCS_MAX_IPV4_LEN,NULL);
    }

    memset(dataCtxPtr->intfName, 0, sizeof(dataCtxPtr->intfName));
    memset(dataCtxPtr->dns1Addr, 0, sizeof(dataCtxPtr->dns1Addr));
    memset(dataCtxPtr->dns2Addr, 0, sizeof(dataCtxPtr->dns2Addr));

    //Create data start retry timer
    snprintf(timerName, sizeof(timerName)-1, "dataId-%d Start Retry Timer", dataId);
    dataCtxPtr->dataStartRetryTimerRef = le_timer_Create(timerName);
    le_timer_SetWakeup(dataCtxPtr->dataStartRetryTimerRef, false);

    //Create radio off/on interval timer
    memset(timerName, 0, sizeof(timerName));
    snprintf(timerName, sizeof(timerName)-1, "dataId-%d radio off/on timer", dataId);
    dataCtxPtr->radioOffOnIntervalTimerRef = le_timer_Create(timerName);
    le_timer_SetContextPtr(dataCtxPtr->radioOffOnIntervalTimerRef, dataCtxPtr);
    le_timer_SetWakeup(dataCtxPtr->radioOffOnIntervalTimerRef, false);
    le_timer_SetHandler(dataCtxPtr->radioOffOnIntervalTimerRef,
                        RadioOffOnIntervalTimerHandler);
    uint32_t radioOffOnInterval =
                            Policy.DataSession.ConnectivityRecovery.RadioOffOnInterval * 1000; //ms
    LE_INFO("Radio Off/On Interval: %d ms", radioOffOnInterval);
    le_timer_SetMsInterval(dataCtxPtr->radioOffOnIntervalTimerRef, radioOffOnInterval);

    //Create sim off/on interval timer
    memset(timerName, 0, sizeof(timerName));
    snprintf(timerName, sizeof(timerName)-1, "dataId-%d sim off/on timer", dataId);
    dataCtxPtr->simOffOnIntervalTimerRef = le_timer_Create(timerName);
    le_timer_SetContextPtr(dataCtxPtr->simOffOnIntervalTimerRef, dataCtxPtr);
    le_timer_SetWakeup(dataCtxPtr->simOffOnIntervalTimerRef, false);
    le_timer_SetHandler(dataCtxPtr->simOffOnIntervalTimerRef,
                        SimOffOnIntervalTimerHandler);
    uint32_t simOffOninterval =
                            Policy.DataSession.ConnectivityRecovery.SimOffOnInterval * 1000; //ms
    LE_INFO("Sim Off/On Interval: %d ms", simOffOninterval);
    le_timer_SetMsInterval(dataCtxPtr->simOffOnIntervalTimerRef, simOffOninterval);

    // Create recovery schedule timer
    memset(timerName, 0, sizeof(timerName));
    snprintf(timerName, sizeof(timerName) - 1, "dataId-%d RecoverySchdTimer", dataId);
    dataCtxPtr->recoveryScheduleTimerRef = le_timer_Create(timerName);
    le_timer_SetHandler(dataCtxPtr->recoveryScheduleTimerRef, RecoveryScheduleTimerHandler);
    le_timer_SetWakeup(dataCtxPtr->recoveryScheduleTimerRef, false);
    le_timer_SetContextPtr(dataCtxPtr->recoveryScheduleTimerRef, dataCtxPtr);
    uint32_t recoveryDelay = Policy.DataSession.ConnectivityRecovery.StartWaitTime * 1000; //ms
    LE_INFO("Connectivity Recovery Scheduled Delay: %d ms", recoveryDelay);
    le_timer_SetMsInterval(dataCtxPtr->recoveryScheduleTimerRef, recoveryDelay);

    // Create recovery retry timer
    memset(timerName, 0, sizeof(timerName));
    snprintf(timerName, sizeof(timerName) - 1, "dataId-%d RecoveryRetryTimer", dataId);
    dataCtxPtr->recoveryRetryTimerRef = le_timer_Create(timerName);
    le_timer_SetHandler(dataCtxPtr->recoveryRetryTimerRef, RecoveryRetryTimerHandler);
    le_timer_SetWakeup(dataCtxPtr->recoveryRetryTimerRef, false);
    le_timer_SetContextPtr(dataCtxPtr->recoveryRetryTimerRef, dataCtxPtr);
    uint32_t recoveryRetryDelay = Policy.DataSession.ConnectivityRecovery.RetryWaitTime * 1000; //ms
    LE_INFO("Connectivity Recovery Retry Delay: %d ms", recoveryRetryDelay);
    le_timer_SetMsInterval(dataCtxPtr->recoveryRetryTimerRef, recoveryRetryDelay);

    // Create MinTimeBetweenTriggers timer for DataStartRetry
    memset(timerName, 0, sizeof(timerName));
    dataCtxPtr->minTimeBetweenTriggersRef = le_timer_Create(timerName);
    le_timer_SetHandler(dataCtxPtr->minTimeBetweenTriggersRef, MinTimeBetweenTriggersHandler);
    le_timer_SetWakeup(dataCtxPtr->minTimeBetweenTriggersRef, false);
    le_timer_SetContextPtr(dataCtxPtr->minTimeBetweenTriggersRef, dataCtxPtr);
    uint32_t minTimeBetweenTriggers =
                Policy.DataSession.AppMngdConnectivityRecovery.MinTimeBetweenTriggers * 1000; //ms
    LE_INFO("minTimeBetweenTriggers time: %d ms", minTimeBetweenTriggers);
    le_timer_SetMsInterval(dataCtxPtr->minTimeBetweenTriggersRef, minTimeBetweenTriggers);

    // Create MaxTimeBetweenTriggers timer for DataStartRetry
    memset(timerName, 0, sizeof(timerName));
    dataCtxPtr->maxTimeBetweenTriggersRef = le_timer_Create(timerName);
    le_timer_SetHandler(dataCtxPtr->maxTimeBetweenTriggersRef, MaxTimeBetweenTriggersHandler);
    le_timer_SetWakeup(dataCtxPtr->maxTimeBetweenTriggersRef, false);
    uint32_t maxTimeBetweenTriggers =
                Policy.DataSession.AppMngdConnectivityRecovery.MaxTimeBetweenTriggers * 1000; //ms
    LE_INFO("maxTimeBetweenTriggers time: %d ms", maxTimeBetweenTriggers);
    le_timer_SetMsInterval(dataCtxPtr->maxTimeBetweenTriggersRef, maxTimeBetweenTriggers);

    //Create PeriodicConnectivityTest timer
    snprintf(timerName, sizeof(timerName)-1, "dataId-%d PeriodicTest Timer", dataId);
    dataCtxPtr->periodicConnectivityTestTimerRef = le_timer_Create(timerName);
    le_timer_SetContextPtr(dataCtxPtr->periodicConnectivityTestTimerRef, dataCtxPtr);
    le_timer_SetWakeup(dataCtxPtr->periodicConnectivityTestTimerRef, false);
    le_timer_SetHandler(dataCtxPtr->periodicConnectivityTestTimerRef,
                        PeriodicConnectivityTestTimerHandler);

    //Create API Management timeouts
    dataCtxPtr->startDataTimeout = Policy.DataSession.APIManagement.StartDataTimeout;
    dataCtxPtr->stopDataTimeout = Policy.DataSession.APIManagement.StopDataTimeout;

    //Create event id
    snprintf(eventName, sizeof(eventName)-1, "connCtx-%d", dataId);
    dataCtxPtr->dataStateEvent = le_event_CreateIdWithRefCounting(eventName);

    // create reference for this connectivity context
    taf_mngdConn_DataRef_t dataRef = (taf_mngdConn_DataRef_t)le_ref_CreateRef(DataRefMap,
                                                                            (void *)dataCtxPtr);
    TAF_ERROR_IF_RET_VAL(dataCtxPtr == NULL, NULL, "Cannot alloc dataRef");

    dataCtxPtr->dataRef = dataRef;

    le_mutex_Lock(DataCtxMutex);
    le_dls_Queue(&DataCtxList, &dataCtxPtr->link);
    le_mutex_Unlock(DataCtxMutex);
    return dataCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if at least one connection is created.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnAdmin::IsStateConnected()
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        if (dataCtxPtr->adminState == MCS_DATA_CONNECTED_ACTIVE)
        {
            le_mutex_Unlock(DataCtxMutex);
            return true;
        }
    }

    le_mutex_Unlock(DataCtxMutex);
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get data state event ID.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t tafMngdConnAdmin::GetDataStateEvent(taf_mngdConn_DataRef_t dataRef)
{
    mcs_DataCtx_t* dataCtxPtr = NULL;

    dataCtxPtr = (mcs_DataCtx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    TAF_ERROR_IF_RET_VAL(dataCtxPtr == NULL, NULL,
                         "Cannot find connectivity context for dataRef(%p)",
                         dataRef);

    return dataCtxPtr->dataStateEvent;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get exited profile list info.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::getProfileList
(
    profileInfo_t *profileNumberList,
    int *listSize
)
{
    le_dls_Link_t* linkPtr = NULL;
    int profileCnt = 0;

    if(profileNumberList == NULL || listSize == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    le_mutex_Lock(DataCtxMutex);
    linkPtr = le_dls_Peek(&DataCtxList);
    while (linkPtr)
    {
        mcs_DataCtx_t* dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
        profileNumberList[profileCnt].phoneId = dataCtxPtr->phoneId;
        profileNumberList[profileCnt].profileNumber = dataCtxPtr->profileNumber;
        profileNumberList[profileCnt].dataState = dataCtxPtr->dataState;
        profileCnt++;
    }

    le_mutex_Unlock(DataCtxMutex);
    *listSize = profileCnt;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerDataStateHandler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::FirstLayerDataStateHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    DataState_t* dataStateEvent = (DataState_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_mngdConn_DataStateHandlerFunc_t handlerFunc =
                                    (taf_mngdConn_DataStateHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(dataStateEvent->dataRef, dataStateEvent->dataState, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report connectivity state and update internal state
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ReportAndUpdateDataState
(
    mcs_DataCtx_t* dataCtxPtr,
    taf_mngdConn_DataState_t newstate
)
{
    TAF_ERROR_IF_RET_NIL(dataCtxPtr == NULL, "Null pointer");
    auto &admin = tafMngdConnAdmin::GetInstance();

    if(dataCtxPtr->dataState == newstate)
        return;

    DataState_t *dataStateIndPtr =
        (DataState_t *)le_mem_ForceAlloc(admin.dataStatePool);

    dataStateIndPtr->dataRef = dataCtxPtr->dataRef;
    dataStateIndPtr->dataState = newstate;
    LE_INFO("#########send state dataRef=%p, state = %d  ",
            dataStateIndPtr->dataRef, newstate);

    le_event_ReportWithRefCounting(dataCtxPtr->dataStateEvent, (void *)dataStateIndPtr);
    //Update data state
    dataCtxPtr->dataState = newstate;

    /**
     * Send an event to the state machine to handle event. This checking is done here as the state
     * is updated here and application events are sent.
    */

    /**
     * If current data state is MCS_DATA_NOT_CONNECTED_FAILED && apps are notified with
     * LE_MNGD_CONN_DATA_CONNECTION_FAILED event, then connectivity recovery should be triggered.
     */
    if (MCS_DATA_NOT_CONNECTED_FAILED == dataCtxPtr->adminState &&
        TAF_MNGDCONN_DATA_CONNECTION_FAILED == dataCtxPtr->dataState)
    {
        LE_INFO("Data (%d) not connected failed. Scheduling connectivity recovery.",
                                                                        dataCtxPtr->dataId);
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event  = MCS_EVT_CONN_RECOVERY_SCHEDULE_L1;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }
    /**
     * If current data state is MCS_DATA_CONNECTED_ACTIVE && apps are notified with
     * TAF_MNGDCONN_DATA_CONNECTED event, then send admin event
     * MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE.
     */
    if (MCS_DATA_CONNECTED_ACTIVE == dataCtxPtr->adminState &&
        TAF_MNGDCONN_DATA_CONNECTED == dataCtxPtr->dataState)
    {
        LE_INFO("Data (%d) connected active.", dataCtxPtr->dataId);
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerRecoveryEventHandler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::FirstLayerRecoveryEventHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    RecoveryEvent_t *recoveryEventPtr = (RecoveryEvent_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_mngdConn_RecoveryEventHandlerFunc_t handlerFunc =
        (taf_mngdConn_RecoveryEventHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(recoveryEventPtr->dataRef,
                recoveryEventPtr->recoveryEvent,
                recoveryEventPtr->operation,
                le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send recovery state event to all clients.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ReportRecoveryEvent(
    taf_mngdConn_RecoveryEvent_t recoveryEvent,
    mcs_DataCtx_t *dataCtxPtr,
    taf_mngdConn_RecoveryOperation_t operation
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();

    RecoveryEvent_t *recoveryEvenetIndPtr =
                                    (RecoveryEvent_t *)le_mem_ForceAlloc(admin.recoveryEventPool);
    TAF_ERROR_IF_RET_NIL(recoveryEvenetIndPtr == NULL, "Unable to alloc recoveryEvenetIndPtr");
    recoveryEvenetIndPtr->recoveryEvent = recoveryEvent;
    recoveryEvenetIndPtr->dataRef = dataCtxPtr->dataRef;
    recoveryEvenetIndPtr->operation = operation;
    le_event_ReportWithRefCounting(admin.recoveryEvent, (void *)recoveryEvenetIndPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the configuration in JSON and set correct states for Network and SIM.
 * If data is set to Autostart send MCS_EVT_DATA_START to the admin state machine.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::InitializeStates()
{
    uint8_t sessionIdx = 0, dataIdx = 0, networkIdx = 0, simIdx = 0;
    uint8_t dataId = 0, phoneId = 0, slotNumber = 0;
    uint32_t profileNumber = 0;
    bool autoStart = false;
    char conn_test_url[MCS_MAX_CONNECTION_URL_LEN] = {'\0'};
    char conn_test_ipv4Addr[MCS_MAX_IPV4_LEN] = {'\0'};
    char dataName[MCS_MAX_NAME_LEN] = {'\0'};
    le_result_t result = LE_OK;
    mcs_DataCtx_t* dataCtxPtr = NULL;
    auto &radio = tafMngdConnRadio::GetInstance();
    auto &sim = tafMngdConnSim::GetInstance();
    taf_dcs_ProfileRef_t profileRef = NULL;

    //Iterate through Policy DataSession elements to create the data sessions
    for (sessionIdx = 0; sessionIdx < Policy.DataSession.dataConnectionCount; sessionIdx++)
    {
        LE_INFO("Data Connection[%d].Use_Data_ID : %d", sessionIdx,
                Policy.DataSession.DataConnection[sessionIdx].Use_Data_ID);

        // Iterate through Config Data elements to create the data sessions when AutoStart is "Yes"
        for (dataIdx = 0; dataIdx < Configuration.DataCount; dataIdx++)
        {
            if(Configuration.Data[dataIdx].ID !=
                                 Policy.DataSession.DataConnection[sessionIdx].Use_Data_ID)
            continue;

            // Reset connection test fields before processing each data entry so that null JSON
            // values do not inherit stale values from a previous iteration.
            conn_test_url[0] = '\0';
            conn_test_ipv4Addr[0] = '\0';

            //Update the dataConnectionCount
            if(Configuration.Data[dataIdx].DataStartRetry.Enable)
            {
                Policy.DataSession.dataConnectionCount =
                       Policy.DataSession.MultiDataSession.NumConnections;
            }
            dataId = Configuration.Data[dataIdx].ID;
            autoStart = Configuration.Data[dataIdx].AutoStart;
            profileNumber = Configuration.Data[dataIdx].Profile.ProfileNumber;
            LE_DEBUG("The data name is %s", Configuration.Data[dataIdx].DataName);
            le_utf8_Copy(dataName,Configuration.Data[dataIdx].DataName,
                         MCS_MAX_NAME_LEN,NULL);

            if(Configuration.Data[dataIdx].DataStartConnectionTest.URL[0] != '\0')
            {
                LE_DEBUG("Setting the url for testing");
                le_utf8_Copy(conn_test_url,Configuration.Data[dataIdx].DataStartConnectionTest.URL,
                    MCS_MAX_CONNECTION_URL_LEN,NULL);
            }

            if(Configuration.Data[dataIdx].DataStartConnectionTest.IPv4[0] != '\0')
            {
                LE_DEBUG("Setting the ipv4 for testing");
                le_utf8_Copy(conn_test_ipv4Addr,
                            Configuration.Data[dataIdx].DataStartConnectionTest.IPv4,
                            MCS_MAX_IPV4_LEN,NULL);
            }


            // Iterate through all Configuration Network elements to find the phoneId
            for(networkIdx = 0; networkIdx < Configuration.NetworkCount; networkIdx++)
            {
                if(Configuration.Network[networkIdx].ID ==
                                                        Configuration.Data[dataIdx].Use_Network_ID)
                {
                    phoneId = Configuration.Network[networkIdx].PhoneID;
                    // Iterate through all Configuration Sim elements to find the slot number
                    for(simIdx=0;simIdx<Configuration.SimCount;simIdx++)
                    {
                        if(Configuration.Sim[simIdx].ID ==
                                                      Configuration.Network[networkIdx].Use_Sim_ID)
                        {
                            slotNumber = Configuration.Sim[simIdx].SlotNumber;
                            break;
                        }
                    }
                    //Can't find the slot number.
                    if(simIdx == Configuration.SimCount)
                    {
                        LE_ERROR("Can't find the slot number");
                        continue;
                    }
                    break;
                }
            }
            //Can't find the phone id.
            if(networkIdx == Configuration.NetworkCount)
            {
                LE_ERROR("Can't find the nework related info for dataId(%d)", dataId);
                continue;
            }
            profileRef = taf_dcs_GetProfileEx (phoneId, profileNumber);

            const char *setapnPtr = Configuration.Data[dataIdx].Profile.APN;
            //Check if APN in JSON is null
            if(setapnPtr[0]=='\0')
            {
                LE_INFO("JSON apn is null");
            }
            else
            {
                LE_INFO("Set apn=%s",Configuration.Data[dataIdx].Profile.APN);
                char getapnPtr[MCS_MAX_APN_LEN];
                //Get the NAD APN
                result = taf_dcs_GetAPN(profileRef, getapnPtr,MCS_MAX_APN_LEN);
                if(result != LE_OK)
                {
                    LE_ERROR("APN get failed for profile %d ", profileNumber);
                    return LE_FAULT;
                }
                //Compare both the APNs and Set if the criteria is met
                if(CompareAPN(getapnPtr, setapnPtr))
                {
                    //Check if its just a space and set it as an empty string
                    if(strlen(setapnPtr) == 1 && setapnPtr[0] == ' ')
                    {
                        //Set APN as an empty string
                        LE_INFO("Set apn as an empty string");
                        result = taf_dcs_SetAPN(profileRef, "");
                    }
                    else
                    {
                        //Set APN
                        result = taf_dcs_SetAPN(profileRef, setapnPtr);
                    }
                    if(result == LE_OK)
                    {
                        LE_INFO("APN : %s set for %d profile", setapnPtr, profileNumber);
                    }
                    else
                    {
                        LE_ERROR("APN : %s  set failed for profile %d ",
                                setapnPtr, profileNumber);
                        return LE_FAULT;
                    }
                }
            }

            LE_INFO("dataId = %d, phoneId = %d, profileNumber=%d, autostart=%d",
                     dataId, phoneId, profileNumber, autoStart);
            dataCtxPtr = GetDataCtx(dataId);
            if(dataCtxPtr == NULL)
            {
                dataCtxPtr = CreateDataCtx(dataId, slotNumber, phoneId, profileNumber, dataName,
                                    autoStart, conn_test_url, conn_test_ipv4Addr);
                if(dataCtxPtr == NULL)
                {
                    LE_ERROR("Creating connection context failed");
                    continue;
                }

                ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
            }
            // Clear the list of clients
            dataCtxPtr->clients.clear();

            dataCtxPtr->maxdataRetryCount = Configuration.Data[dataIdx].DataStartRetry.RetryCount;
            dataCtxPtr->dataRetry = Configuration.Data[dataIdx].DataStartRetry.Enable;
            // JSON has backoff interval in seconds. Convert it to milliseconds
            dataCtxPtr->dataRetryBackoffIntervalInSec =
                                        Configuration.Data[dataIdx].DataStartRetry.BackoffInterval;
            dataCtxPtr->dataRetryBackoffIntervalStep =
                                    Configuration.Data[dataIdx].DataStartRetry.BackoffIntervalStep;

            if(Configuration.Data[dataIdx].PeriodicConnectivityCheck.URL[0] != '\0')
            {
                le_utf8_Copy(dataCtxPtr->conn_periodic_test_url,
                Configuration.Data[dataIdx].PeriodicConnectivityCheck.URL,
                MCS_MAX_CONNECTION_URL_LEN,NULL);
            }

            dataCtxPtr->conn_periodic_test_interval =
                                Configuration.Data[dataIdx].PeriodicConnectivityCheck.Interval;

            dataCtxPtr->conn_periodic_test_maxRetryCount =
                                Configuration.Data[dataIdx].PeriodicConnectivityCheck.RetryCount;

            if(sim.IsSimReady(slotNumber))
            {
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_SIM_READY;
            }
            else
            {
                LE_ERROR("SIM NOT READY");
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_SIM_NOT_READY;
                continue;
            }

            // Send an event to start radio
            LE_INFO("Sending event to start radio for phoneID: %d", phoneId);
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.event = MCS_RADIO_POWER_ON;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

            //Update dataCtxPtr->adminState according to the network register state.
            if(radio.IsNetworkRegistered(dataCtxPtr->phoneId))
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_NW_REGISTERED;
            else
                dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;

            if ( Configuration.Data[dataIdx].AutoStart)
            {
                // Send an event to start data
                LE_INFO("Sending event to start data for ID: %d", Configuration.Data[dataIdx].ID);
                stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
                stateMachineEvt.event = MCS_EVT_DATA_START;
                stateMachineEvt.dataId = Configuration.Data[dataIdx].ID;
                le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
            }
            else
            {
                // Set state expecting application to start data
                if (dataCtxPtr->adminState == MCS_DATA_NOT_CONNECTED_NW_REGISTERED)
                {
                    dataCtxPtr->adminState = MCS_DATA_NOT_CONNECTED;
                    ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_DISCONNECTED);
                }
            }
        }
    }

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Connectivity recovery schedule timer handler
 * Timer Reference: recoveryScheduleTimerRef
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::RecoveryScheduleTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("Connectivity Recovery scheduled timer handler");
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if (NULL == dataCtxPtr)
    {
        LE_WARN("dataCtxPtr is null");
        return;
    }

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    // Check if the data context is still in the recovery scheduled state
    if (MCS_RECOVERY_SCHEDULED_L1 == dataCtxPtr->adminState)
    {
        // Set state to L1 recovery started
        dataCtxPtr->adminState = MCS_RECOVERY_STARTED_L1;

        // Send L1 recovery start event to the admin
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_START_L1;
        le_event_Report(mngdConnAdmin.StateMachineEventId,
                        &stateMachineEvt, sizeof(stateMachineEvent_t));
        LE_INFO("Sent %s to admin",
                        mngdConnAdmin.EventToString(MCS_EVT_CONN_RECOVERY_START_L1));
        return;
    }
    else if (MCS_RECOVERY_SCHEDULED_L2 == dataCtxPtr->adminState)
    {
        // Set state to L2 recovery started
        dataCtxPtr->adminState = MCS_RECOVERY_STARTED_L2;
        // Send L2 recovery start event to the admin
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_START_L2;
        le_event_Report(mngdConnAdmin.StateMachineEventId,
                        &stateMachineEvt, sizeof(stateMachineEvent_t));
        LE_INFO("Sent %s to admin",
                mngdConnAdmin.EventToString(MCS_EVT_CONN_RECOVERY_START_L2));
        return;
    }
    else if (MCS_RECOVERY_SCHEDULED_L3 == dataCtxPtr->adminState)
    {
        // Set state to L3 recovery started
        dataCtxPtr->adminState = MCS_RECOVERY_STARTED_L3;
        // Send L3 recovery start event to the admin
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_START_L3;
        le_event_Report(mngdConnAdmin.StateMachineEventId,
                        &stateMachineEvt, sizeof(stateMachineEvent_t));
        LE_INFO("Sent %s to admin",
                mngdConnAdmin.EventToString(MCS_EVT_CONN_RECOVERY_START_L3));
        return;
    }
    LE_INFO("Unhandled state: %d(%s)", dataCtxPtr->adminState,
                                       mngdConnAdmin.StateToString(dataCtxPtr->adminState));
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connectivity recovery retry timer handler
 * Timer Reference: recoveryRetryTimerRef
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::RecoveryRetryTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("Connectivity Recovery retry timer handler");
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if (NULL == dataCtxPtr)
    {
        LE_WARN("dataCtxPtr is null");
        return;
    }
    // Check the state and schedule the appropriate recovery
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    if ( MCS_RECOVERY_CANCELED_L1 == dataCtxPtr->adminState  ||
         MCS_RECOVERY_FAILED_L1 == dataCtxPtr->adminState)
    {
        LE_INFO("Scheduling Radio Off/On recovery");
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_SCHEDULE_L1;
        le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                                                           sizeof(stateMachineEvent_t));
    }
    else if ( MCS_RECOVERY_CANCELED_L2 == dataCtxPtr->adminState  ||
         MCS_RECOVERY_FAILED_L2 == dataCtxPtr->adminState)
    {
        LE_INFO("Scheduling Sim Off/On recovery");
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_SCHEDULE_L2;
        le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                                                           sizeof(stateMachineEvent_t));
    }
    else if ( MCS_RECOVERY_CANCELED_L3 == dataCtxPtr->adminState ||
              MCS_RECOVERY_FAILED_L3 == dataCtxPtr->adminState)
    {
        // Schedule a L3 recovery
        LE_INFO("Scheduling NAD Reboot recovery");
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_SCHEDULE_L3;
        le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                                                           sizeof(stateMachineEvent_t));
    }
    else
    {
        LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                                    mngdConnAdmin.StateToString(dataCtxPtr->adminState));
        return;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * RadioOffOnInterval timer handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::RadioOffOnIntervalTimerHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("RadioOffOnInterval handler");
    auto &radio = tafMngdConnRadio::GetInstance();
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
            le_timer_Stop(timerRef);
        return;
    }
    le_result_t result = radio.PowerOn(dataCtxPtr->phoneId);
    if (LE_OK != result)
    {
        LE_WARN ("Radio power on failed: %d", result);
    }
    LE_INFO("Radio turned on");
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * SimOffOnInterval timer handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::SimOffOnIntervalTimerHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("SimOffOnInterval handler");
    auto &sim = tafMngdConnSim::GetInstance();
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
            le_timer_Stop(timerRef);
        return;
    }
    le_result_t result = sim.PowerOn(dataCtxPtr->slotId);
    if (LE_OK != result)
    {
        LE_WARN ("SIM power on failed: %d", result);
    }
    LE_INFO("SIM turned on");
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Data start retry timer handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::DataRetryTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("Data Retry timer handler");
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    mcs_DataCtx_t* dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
            le_timer_Stop(timerRef);
        return;
    }

    //If need to reconnect, start a data call
    if(dataCtxPtr->adminState == MCS_DATA_NOT_CONNECTED_RETRYING)
    {
        // Send MCS_EVT_DATA_START event to admin
        LE_INFO("Send MCS_EVT_DATA_START event");
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.event = MCS_EVT_DATA_START;
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        le_event_Report(mngdConnAdmin.StateMachineEventId,
                                            &stateMachineEvt, sizeof(stateMachineEvent_t));
    }
    else if(dataCtxPtr->adminState == MCS_DATA_CONNECTED_ACTIVE)
    {
        LE_INFO("Connected, stop the timer.");
        if (le_timer_IsRunning(timerRef))
            le_timer_Stop(timerRef);
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * PeriodicConnectivityTest timer handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::PeriodicConnectivityTestTimerHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("PeriodicConnectivityTest handler");
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
            le_timer_Stop(timerRef);
        return;
    }

    LE_DEBUG ("Periodic Connectivity test retry count is %d",
            dataCtxPtr->conn_periodic_test_retryCount );

    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    // Send PeriodicConnectivityTestStart event to the admin
    stateMachineEvt.event = MCS_EVT_DATA_PERIODIC_CONNECTIONTEST;
    le_event_Report(mngdConnAdmin.StateMachineEventId,
                    &stateMachineEvt, sizeof(stateMachineEvent_t));
    LE_INFO("Sent %s to admin",
        mngdConnAdmin.EventToString(MCS_EVT_DATA_PERIODIC_CONNECTIONTEST));
}

//--------------------------------------------------------------------------------------------------
/**
 * MinTimeBetweenTriggers timer handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::MinTimeBetweenTriggersHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("MinTimeBetweenTriggers handler");
    le_result_t result;
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
        {
            result = le_timer_Stop(timerRef);
            if (LE_OK != result)
            {
                LE_DEBUG("Stopping timer failed: %d", result);
            }
        }
        return;
    }
    LE_INFO("MinTimeBetweenTriggers timer expired.");

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * MaxTimeBetweenTriggers timer handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::MaxTimeBetweenTriggersHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("MaxTimeBetweenTriggers handler");
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)le_timer_GetContextPtr(timerRef);
    le_result_t result;
    if(dataCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        if (le_timer_IsRunning(timerRef))
        {
            result = le_timer_Stop(timerRef);
            if (LE_OK != result)
            {
                LE_DEBUG("Stopping timer failed: %d", result);
            }
        }
        return;
    }
    LE_INFO("MaxTimeBetweenTriggers timer expired.");
    //Remove the client from the retry clients list
    mcs_ClientNode_t *clientNodePtr =
        (mcs_ClientNode_t *)le_hashmap_Remove(RetryClients.hashMap, dataCtxPtr->sessionRef);
    TAF_ERROR_IF_RET_NIL(clientNodePtr == nullptr,
                "Failed to get sessionRef %p from RetryClients hashmap", dataCtxPtr->sessionRef);
    LE_DEBUG("Client %p removed from RetriedClient List for Data ID: %d",
              dataCtxPtr->sessionRef, dataCtxPtr->dataId);
    le_mem_Release(clientNodePtr);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_START_CONNECTIONTEST which is sent when data is started.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataStartConnectionTest(uint8_t dataId)
{
    LE_INFO("DataStartConnectionTest entered");
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }
    std::string url = dataCtxPtr->conn_test_url;
    std::string ipv4add = dataCtxPtr->conn_test_ipv4Addr;
    std::string interfaceName = dataCtxPtr->intfName;

    //cURL will be tried first, and if it fails Ping will be used.
    //If Ping also fails, data will be treated as not connected.

    if(!url.empty())
    {
        // Set data start connection test in progress to true
        dataCtxPtr->isDStartConnTestInProgress = true;
        if(DataConnectivityTest_URL(url , interfaceName))
        {
            //connection is created.
            dataCtxPtr->adminState = MCS_DATA_CONNECTED_ACTIVE;
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTED);
            //If manually started the data successfully. Set reconnection flag to true.
            dataCtxPtr->needReConn = true;
        }
        else if(!ipv4add.empty() && DataConnectivityTest_IPv4(ipv4add) &&
            DataConnectivityTest_Ping(ipv4add , interfaceName))
        {
            //connection is created.
            dataCtxPtr->adminState = MCS_DATA_CONNECTED_ACTIVE;
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTED);
            //If manually started the data successfully. Set reconnection flag to true.
            dataCtxPtr->needReConn = true;
        }
        else
        {
            // Connectiontest failed.
            LE_INFO("DataStartConnectionTest failed for dataID: %d", dataId);
            LE_INFO("Stopping the data and retrying.");
            dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE_RETRYING;
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.event=MCS_EVT_DATA_STOP;
            stateMachineEvt.dataId=dataCtxPtr->dataId;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        }
        // Set data start connection test in progress to false
        dataCtxPtr->isDStartConnTestInProgress = false;
    }
    else if(!ipv4add.empty())
    {
        // Set data start connection test in progress to true
        dataCtxPtr->isDStartConnTestInProgress = true;
        if(DataConnectivityTest_IPv4(ipv4add) && DataConnectivityTest_Ping(ipv4add , interfaceName))
        {
            //connection is created.
            dataCtxPtr->adminState = MCS_DATA_CONNECTED_ACTIVE;
            ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTED);
            //If manually started the data successfully. Set reconnection flag to true.
            dataCtxPtr->needReConn = true;
        }
        else
        {
            // Connectiontest failed.
            LE_INFO("DataStartConnectionTest failed. Stopping the data and retrying.");
            dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE_RETRYING;
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.event=MCS_EVT_DATA_STOP;
            stateMachineEvt.dataId=dataCtxPtr->dataId;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        }
        // Set data start connection test in progress to false
        dataCtxPtr->isDStartConnTestInProgress = false;
    }
    //If both url and ipaddr is null
    else
    {
        LE_INFO("DataStartConnectionTest passed because both url and ipv4 are null for dataID: %d",
                                                                                            dataId);
        //connection is created.
        dataCtxPtr->adminState = MCS_DATA_CONNECTED_ACTIVE;
        ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTED);
        //If manually started the data successfully. Set reconnection flag to true.
        dataCtxPtr->needReConn = true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event MCS_EVT_DATA_PERIODIC_CONNECTIONTEST which is sent when data is
 * started.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataPeriodicConnectivityTest(uint8_t dataId)
{
    LE_DEBUG("PeriodicConnectivityTest entered");
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }
    std::string url = dataCtxPtr->conn_periodic_test_url;
    std::string interfaceName = dataCtxPtr->intfName;

    if(!url.empty())
    {

            if(!DataConnectivityTest_URL(url, interfaceName))
            {
                // PeriodicConnectivitytest failed for this iteration.
                LE_INFO("PeriodicConnectivitytest failed for dataID: %d", dataId);
                //Report and update the state (only once)
                if(dataCtxPtr->dataState != TAF_MNGDCONN_DATA_CONNECTION_STALLED)
                {
                    ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTION_STALLED);
                }
                //Increase the RetryCount in case of failure
                dataCtxPtr->conn_periodic_test_retryCount =
                                            dataCtxPtr->conn_periodic_test_retryCount + 1;

                if(dataCtxPtr->conn_periodic_test_retryCount <=
                                                dataCtxPtr->conn_periodic_test_maxRetryCount)
                {
                    // Start the Periodic Connection Test timer
                    le_timer_Start(dataCtxPtr->periodicConnectivityTestTimerRef);
                }
                else
                {
                    // It is not possible to proceed with the PeriodicConnectivityTest retries
                    LE_ERROR("PeriodicConnectivityTest retry count exceeded. Start DataRetry");
                    //Start the retry procedure
                    dataCtxPtr->adminState = MCS_DATA_CONNECTED_INACTIVE_RETRYING;
                    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
                    stateMachineEvt.event=MCS_EVT_DATA_STOP;
                    stateMachineEvt.dataId=dataCtxPtr->dataId;
                    le_event_Report(StateMachineEventId, &stateMachineEvt,
                                    sizeof(stateMachineEvent_t));
                }
            }
            else
            {
                //Reset the retryCount
                dataCtxPtr->conn_periodic_test_retryCount = 1;

                // Start the Periodic Connection Test timer
                le_timer_Start(dataCtxPtr->periodicConnectivityTestTimerRef);

                //Change the State back to connected
                if(dataCtxPtr->dataState == TAF_MNGDCONN_DATA_CONNECTION_STALLED)
                {
                    dataCtxPtr->adminState = MCS_DATA_CONNECTED_ACTIVE;
                    ReportAndUpdateDataState(dataCtxPtr, TAF_MNGDCONN_DATA_CONNECTED);
                }
            }
    }
    else
    {
        //In case of null url
        le_timer_Stop(dataCtxPtr->periodicConnectivityTestTimerRef);
    }
}

bool tafMngdConnAdmin::DataConnectivityTest_URL(std::string url, std::string interfaceName)
{
    std::string URL = RemoveProtocol(url);
#ifdef LE_CONFIG_TAFMNGDCONNSVC_USE_CURL
    if (PerformCurl(URL.c_str(), interfaceName.c_str()))
    {
        LE_INFO("DataConnectivityTest_URL cURL passed ");
        return true;
    }
#else
    if (DataConnectivityTest_Ping(URL, interfaceName))
    {
        LE_INFO("DataConnectivityTest_URL URL passed ");
        return true;
    }
#endif
    else
    {
        LE_INFO ("DataConnectivityTest_URL failed ");
    }
    return false;
}

bool tafMngdConnAdmin::DataConnectivityTest_IPv4(std::string ipv4)
{
    // Validate IPv4 format before attempting ping
    std::regex ipv4Pattern(
        "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
        "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
    );

    if (!std::regex_match(ipv4, ipv4Pattern))
    {
        LE_ERROR("Invalid IPv4 address format: %s", ipv4.c_str());
        return false;
    }

    return true;
}

bool tafMngdConnAdmin::DataConnectivityTest_Ping(std::string addr, std::string interfaceName)
{
    //Enable LE_CONFIG_DEBUG to get the output of ping in logs
    LE_INFO("DataConnectivityTest_Ping entered for interface %s",interfaceName.c_str());
#if LE_CONFIG_DEBUG
        std::string pingCommand = "ping -c 5 -I "+ interfaceName +" "+ addr;
        //5 is the number of ping pockets
#else
        std::string pingCommand = "ping -c 5 -I "+ interfaceName +" "+  addr
                                  + " 1> /dev/null 2> /dev/null";
#endif
    LE_DEBUG("%s", pingCommand.c_str());
    int result = system(pingCommand.c_str());

    if (result == 0)
    {
        // connection is created.
        LE_INFO("DataConnectivityTest_Ping passed for interface %s", interfaceName.c_str());
        return true;
    }
    else
    {
        LE_INFO("DataConnectivityTest_Ping failed for interface %s",interfaceName.c_str());
        return false;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_SCHEDULE_L1 event handler.
 * - Send appropriate recovery schduled notification to applications
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL1ConnRecoverySchedule (uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

    // Check if connectivity recovery is enabled in policy.
    if (MCS_CONNECTIONRECOVERY_LEVEL_NONE == Policy.DataSession.ConnectivityRecovery.Level)
    {
        LE_WARN ("Connectivity Recovery is not enabled. Unable to recover data");
        return;
    }

    if (MCS_DATA_NOT_CONNECTED_FAILED == dataCtxPtr->adminState ||
        MCS_RECOVERY_CANCELED_L1 == dataCtxPtr->adminState )
    {
        // Check if L1 recovery has been tried already or not
        if (!dataCtxPtr->wasL1ConnectivityRecoveryDone)
        {
            // MCS_CONNECTIONRECOVERY_LEVEL_L1
            // Update the admin state to L1 recovery scheduled
            dataCtxPtr->adminState = MCS_RECOVERY_SCHEDULED_L1;
            LE_INFO("Radio Off/On recovery scheduled for Data id: %d", dataId);
            //Set the recovery operation
            dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON;
            // Report recovery state to all clients
            ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_SCHEDULED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON);
            // Start the recovery schedule timer
            if (le_timer_IsRunning(dataCtxPtr->recoveryScheduleTimerRef))
            {
                // Recovery schedule timer should not be running
                LE_WARN("Recovery schedule timer already running");
                // Stop the timer
                le_timer_Stop(dataCtxPtr->recoveryScheduleTimerRef);
            }
            le_timer_Start(dataCtxPtr->recoveryScheduleTimerRef);
            // Track that a recovery is scheduled. This is needed to send canceled
            // notification (if needed).
            dataCtxPtr->isConnectivityRecoveryScheduled = true;
        }
        else
        {
            // L1 recovery already tried. Send event to admin to schedule L2 recovery
            // Update the admin state to L1 recovery failed
            LE_INFO("Radio Off/On recovery already tried.");
            LE_INFO("Mark Radio Off/On as failed and schedule Sim Off/On recovery");
            dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L1;
            ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_FAILED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON);
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_SCHEDULE_L2;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        }
    }
    else
    {
        LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                                         StateToString(dataCtxPtr->adminState));
        LE_INFO("Marking Radio Off/On recovery as interrupted and informing admin");
        // Set the internal state to data recovery canceled
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L1;
        // Set the reconnected needed flag to TRUE
        dataCtxPtr->needReConn = true;
        // Inform admin that Radio Off/On recovery is interrupted and to inform clients
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_CANCEL event handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventConnRecoveryCancel(uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }
    LE_INFO("Cancel a scheduled recovery for level %d",dataCtxPtr->recoveryOperation);
    // Stop recovery timer if it is running
    if (le_timer_IsRunning(dataCtxPtr->recoveryScheduleTimerRef))
    {
        le_timer_Stop(dataCtxPtr->recoveryScheduleTimerRef);
    }
    dataCtxPtr->isConnectivityRecoveryScheduled = false;

    // Set the internal state to data recovery failed
    if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON)
    {
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L1;
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_SIM_OFF_ON)
    {
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L2;
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_NAD_REBOOT)
    {
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L3;
    }


    // Set the reconnected needed flag to TRUE
    dataCtxPtr->needReConn = true;

    // Inform admin that recovery is interrupted and to inform clients
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_CANCEL_SYNC event handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventConnRecoveryCancelSync(uint8_t dataId)
{
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    le_result_t result = LE_NOT_FOUND;

    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to find reference for data id: %d", dataId);
        mngdConnAdmin.CmdSynchronousPromise.set_value(result);
        return;
    }
    LE_INFO("Cancel a scheduled recovery synchronously for level %d",dataCtxPtr->recoveryOperation);
    // Stop recovery timer if it is running
    if (le_timer_IsRunning(dataCtxPtr->recoveryScheduleTimerRef))
    {
        result = le_timer_Stop(dataCtxPtr->recoveryScheduleTimerRef);
        if (LE_OK != result)
        {
            LE_DEBUG("Stopping timer failed: %d", result);
        }
    }
    // No recovery is scheduled
    dataCtxPtr->isConnectivityRecoveryScheduled = false;

    // Set the internal state to data recovery failed
    if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON)
    {
        LE_INFO("Radio Off/On recovery canceled");
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L1;
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_SIM_OFF_ON)
    {
        LE_INFO("Sim Off/On recovery canceled");
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L2;
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_NAD_REBOOT)
    {
        LE_INFO("NAD Reboot recovery canceled");
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L3;
    }

    // Set the reconnected needed flag to TRUE
    dataCtxPtr->needReConn = true;

    // Unblock the waiting API
    mngdConnAdmin.CmdSynchronousPromise.set_value(result);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_CANCEL_SEND_IND event handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventConnRecoveryCancelSendInd(uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to find reference for data id: %d", dataId);
        return;
    }
    LE_INFO("Send cancel event for level %d",dataCtxPtr->recoveryOperation);
    // send canceled event to clients
    if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON)
    {
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_CANCELED, dataCtxPtr ,
                        TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON);
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_SIM_OFF_ON)
    {
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_CANCELED, dataCtxPtr ,
                        TAF_MNGDCONN_RECOVERY_SIM_OFF_ON);
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_NAD_REBOOT)
    {
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_CANCELED, dataCtxPtr ,
                        TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_INTERRUPTED event handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventConnRecoveryInterrupted(uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }
    LE_INFO("Interrupt recovery for level %d",dataCtxPtr->recoveryOperation);
    // Ensure state is recovery failed
    if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON)
    {
        if (MCS_RECOVERY_FAILED_L1 != dataCtxPtr->adminState &&
            MCS_RECOVERY_CANCELED_L1 != dataCtxPtr->adminState)
        {
            LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                    StateToString(dataCtxPtr->adminState));
            return;
        }
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_SIM_OFF_ON)
    {
        if (MCS_RECOVERY_FAILED_L2 != dataCtxPtr->adminState &&
            MCS_RECOVERY_CANCELED_L2 != dataCtxPtr->adminState)
        {
            LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                    StateToString(dataCtxPtr->adminState));
            return;
        }
    }
    else if(dataCtxPtr->recoveryOperation == TAF_MNGDCONN_RECOVERY_NAD_REBOOT)
    {
        if (MCS_RECOVERY_FAILED_L3 != dataCtxPtr->adminState &&
            MCS_RECOVERY_CANCELED_L3 != dataCtxPtr->adminState)
        {
            LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                    StateToString(dataCtxPtr->adminState));
            return;
        }
    }


    // Check if recovery retry is enabled in JSON (RetryWaitTime>0)
    if (Policy.DataSession.ConnectivityRecovery.RetryWaitTime > 0)
    {
        if (dataCtxPtr->needReConn)
        {
            // Start the timer for scheduling a L2 recovery retry after configured delay
            if (le_timer_IsRunning(dataCtxPtr->recoveryRetryTimerRef))
            {
                // Recovery schedule timer should not be running
                LE_WARN("Recovery schedule timer already running");
                // Stop the timer
                le_timer_Stop(dataCtxPtr->recoveryRetryTimerRef);
            }
            le_timer_Start(dataCtxPtr->recoveryRetryTimerRef);
        }
        else
        {
            LE_INFO("Recovery retry not scheduled.  Data id(%d) does not need reconnection",
                                                                                        dataId);
        }
    }
    else
    {
        LE_WARN("Recovery retry is not enabled in JSON. Recovery cannot be rescheduled");
    }

    // Send recovery canceled event to applications
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
    stateMachineEvt.dataId = dataCtxPtr->dataId;
    stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_CANCEL_SEND_IND;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_START_L1 event handler.
 * - Send CONN_RECOVERY_L1_STARTED notification to applications
 * - Stop all active data sessions
 * - Mark sessions for restarting as needed
 * - Turn off radio, wait 5s and turn on radio
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL1ConnRecoveryStart(uint8_t dataId)
{
    // Send an event to all applications that L1 recovery has started.
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

    // Reset connectivity recovery scheduled flag
    dataCtxPtr->isConnectivityRecoveryScheduled = false;

    // Set the reconnected needed flag to TRUE
    dataCtxPtr->needReConn = true;

#ifndef LE_CONFIG_TARGET_SIMULATION
    auto &ecall = tafMngdConnECall::GetInstance();
    if (ecall.IsECallInProgress())
    {
        LE_WARN("eCall is in progress. Cannot proceed with L1 recovery");
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L1;
        // Inform admin that L1 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }
#endif
    // Mark that connectivity recovery was tried
    dataCtxPtr->wasL1ConnectivityRecoveryDone = true;

    ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_STARTED, dataCtxPtr,
                        TAF_MNGDCONN_RECOVERY_RADIO_OFF_ON);

    // Stop all active data sessions
    // Hold the mutex until data is stopped and radio is turned off and on.
    le_mutex_Lock(DataCtxMutex);
    le_dls_Link_t *linkPtr = le_dls_Peek(&DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t *dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        if (MCS_DATA_CONNECTED_ACTIVE == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_INACTIVE == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_INACTIVE_RETRYING == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_IDLE == dataCtxPtr->adminState)
        {
                // Set the reconnected needed flag to TRUE
                dataCtxPtr->needReConn = true;
        }
        // Move to the next item in the list
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
    }

    // All data sessions have been stopped. Toggle the radio link that is linked to this data ID.
    auto &radio = tafMngdConnRadio::GetInstance();
    le_result_t result = radio.PowerOff(dataCtxPtr->phoneId);
    if (LE_OK != result)
    {
        LE_WARN ("Radio power off failed: %d", result);
    }
    LE_INFO("Radio turned off");
    //Start the timer for the interval
    le_timer_Start(dataCtxPtr->radioOffOnIntervalTimerRef);
    le_mutex_Unlock(DataCtxMutex);

    //NAD should register again and data will be managed based on received events
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_SCHEDULE_L2 event handler.
 * - Send appropriate recovery schduled notification to applications
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL2ConnRecoverySchedule (uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

    // Check if connectivity recovery is enabled in policy.
    if (MCS_CONNECTIONRECOVERY_LEVEL_L2 != Policy.DataSession.ConnectivityRecovery.Level &&
        MCS_CONNECTIONRECOVERY_LEVEL_L3 != Policy.DataSession.ConnectivityRecovery.Level)
    {
        LE_WARN("Sim Off/On Connectivity Recovery is not enabled. Unable to recover data");
        return;
    }

    if (MCS_RECOVERY_FAILED_L1 == dataCtxPtr->adminState ||
        MCS_RECOVERY_FAILED_L2 == dataCtxPtr->adminState ||
        MCS_RECOVERY_CANCELED_L2 == dataCtxPtr->adminState )
    {
        // Check if L2 recovery has been tried already or not
        if (!dataCtxPtr->wasL2ConnectivityRecoveryDone)
        {
            // MCS_CONNECTIONRECOVERY_LEVEL_L2
            // Update the admin state to L2 recovery scheduled
            dataCtxPtr->adminState = MCS_RECOVERY_SCHEDULED_L2;
            LE_INFO("Sim Off/On recovery scheduled for Data id: %d", dataId);
            //Set the recovery operation
            dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_SIM_OFF_ON;
            // Report recovery state to all clients
            ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_SCHEDULED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_SIM_OFF_ON);
            // Start the recovery schedule timer
            if (le_timer_IsRunning(dataCtxPtr->recoveryScheduleTimerRef))
            {
                // Recovery schedule timer should not be running
                LE_WARN("Recovery schedule timer already running");
                // Stop the timer
                le_timer_Stop(dataCtxPtr->recoveryScheduleTimerRef);
            }
            le_timer_Start(dataCtxPtr->recoveryScheduleTimerRef);
            // Track that a recovery is scheduled. This is needed to send canceled
            // notification (if needed).
            dataCtxPtr->isConnectivityRecoveryScheduled = true;
        }
        else
        {
            // L2 recovery already tried. Send event to admin to schedule L2 recovery
            // Update the admin state to L2 recovery failed
            LE_INFO("Sim Off/On recovery already tried.");
            LE_INFO("Mark Sim Off/On as failed and schedule NAD Reboot recovery");
            dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L2;
            ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_FAILED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_SIM_OFF_ON);
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_SCHEDULE_L3;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        }
        le_timer_Start(dataCtxPtr->recoveryScheduleTimerRef);
        dataCtxPtr->isConnectivityRecoveryScheduled = true;
        // Report recovery state to all clients
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_SCHEDULED, dataCtxPtr,
                            TAF_MNGDCONN_RECOVERY_SIM_OFF_ON);
    }
    else
    {
        LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                                         StateToString(dataCtxPtr->adminState));
        LE_INFO("Marking Sim Off/On recovery as interrupted and informing admin");
        // Set the internal state to data recovery canceled
        dataCtxPtr->adminState = MCS_RECOVERY_CANCELED_L2;
        // Set the reconnected needed flag to TRUE
        dataCtxPtr->needReConn = true;
        // Inform admin that L2 recovery is interrupted and to inform clients
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_START_L2 event handler.
 * - Send CONN_RECOVERY_L2_STARTED notification to applications
 * - Stop all active data sessions
 * - Mark sessions for restarting as needed
 * - Turn off radio, wait 5s and turn on radio
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL2ConnRecoveryStart(uint8_t dataId)
{
    // Send an event to all applications that L2 recovery has started.
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

    // Reset connectivity recovery scheduled flag
    dataCtxPtr->isConnectivityRecoveryScheduled = false;

    // Set the reconnected needed flag to TRUE
    dataCtxPtr->needReConn = true;

#ifndef LE_CONFIG_TARGET_SIMULATION
    auto &ecall = tafMngdConnECall::GetInstance();
    if (ecall.IsECallInProgress())
    {
        LE_WARN("eCall is in progress. Cannot proceed with L2 recovery");
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L2;
        // Inform admin that L2 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }
#endif
    // Mark that connectivity recovery was tried
    dataCtxPtr->wasL2ConnectivityRecoveryDone = true;

    ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_STARTED, dataCtxPtr,
                        TAF_MNGDCONN_RECOVERY_SIM_OFF_ON);

    // Stop all active data sessions
    // Hold the mutex until data is stopped and radio is turned off and on.
    le_mutex_Lock(DataCtxMutex);
    le_dls_Link_t *linkPtr = le_dls_Peek(&DataCtxList);

    while (linkPtr)
    {
        mcs_DataCtx_t *dataCtxPtr = CONTAINER_OF(linkPtr, mcs_DataCtx_t, link);
        if (MCS_DATA_CONNECTED_ACTIVE == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_INACTIVE == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_INACTIVE_RETRYING == dataCtxPtr->adminState ||
            MCS_DATA_CONNECTED_IDLE == dataCtxPtr->adminState)
        {
                // Set the reconnected needed flag to TRUE
                dataCtxPtr->needReConn = true;
        }
        // Move to the next item in the list
        linkPtr = le_dls_PeekNext(&DataCtxList, linkPtr);
    }

    // All data sessions have been stopped. Toggle the SIM that is linked to this data ID.
    auto &sim = tafMngdConnSim::GetInstance();
    le_result_t result = sim.PowerOff(dataCtxPtr->slotId);
    if (LE_OK != result)
    {
        LE_WARN ("SIM power off failed: %d", result);
    }
    LE_INFO("SIM turned off");
    le_timer_Start(dataCtxPtr->simOffOnIntervalTimerRef);
    le_mutex_Unlock(DataCtxMutex);
}

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_SCHEDULE_L3 event handler.
 * - Send appropriate recovery schduled notification to applications
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL3ConnRecoverySchedule(uint8_t dataId)
{
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

    // Check if connectivity recovery is enabled in policy.
    if (MCS_CONNECTIONRECOVERY_LEVEL_L3 != Policy.DataSession.ConnectivityRecovery.Level)
    {
        LE_WARN("NAD Reboot Connectivity Recovery is not enabled. Unable to recover data");
        return;
    }

    if (MCS_RECOVERY_FAILED_L2 == dataCtxPtr->adminState ||
        MCS_RECOVERY_FAILED_L3 == dataCtxPtr->adminState ||
        MCS_RECOVERY_CANCELED_L3 == dataCtxPtr->adminState)
    {
        // Set the internal state to L3 data recovery scheduled
        dataCtxPtr->adminState = MCS_RECOVERY_SCHEDULED_L3;
        LE_INFO("NAD Reboot recovery scheduled for Data id: %d", dataId);
        //Set the recovery operation
        dataCtxPtr->recoveryOperation = TAF_MNGDCONN_RECOVERY_NAD_REBOOT;
        // Start the recovery schedule timer
        if (le_timer_IsRunning(dataCtxPtr->recoveryScheduleTimerRef))
        {
            // Recovery schedule timer should not be running
            LE_WARN("Recovery schedule timer already running");
            // Stop the timer
            le_timer_Stop(dataCtxPtr->recoveryScheduleTimerRef);
        }
        le_timer_Start(dataCtxPtr->recoveryScheduleTimerRef);
        dataCtxPtr->isConnectivityRecoveryScheduled = true;
        // Report recovery state to all clients
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_SCHEDULED, dataCtxPtr,
                            TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
    }
    else
    {
        LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                                         StateToString(dataCtxPtr->adminState));
        return;
    }
}


#ifndef LE_CONFIG_TARGET_SIMULATION
//--------------------------------------------------------------------------------------------------
/**
 * taf_mngdPm_ShutdownReqAsync response handler
 */
//-------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::RestartReqAsyncCallBack(taf_mngdPm_RestartMode_t RestartMode,
                                               taf_mngdPm_ResponseMode_t ResponseMode,
                                               le_result_t result,
                                               void *contextPtr)
{
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    LE_DEBUG("Restart  mode : %d", RestartMode);
    LE_DEBUG("Response mode : %d", ResponseMode);
    LE_DEBUG("result is : %d", result);
    mcs_DataCtx_t *dataCtxPtr = (mcs_DataCtx_t *)contextPtr;
    if (TAF_MNGDPM_READY == ResponseMode)
    {
        LE_INFO("Restart in progress");
        dataCtxPtr->adminState = MCS_RECOVERY_STARTED_L3;
    }
    else
    {
        LE_INFO("Restart request failed %d", static_cast<int>(ResponseMode));
        // Update admin state that L3 recovery has failed
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L3;
        // Inform admin that L3 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                                                           sizeof(stateMachineEvent_t));
    }
}
#endif // LE_CONFIG_TARGET_SIMULATION

//--------------------------------------------------------------------------------------------------
/**
 * MCS_EVT_CONN_RECOVERY_START_L3 event handler
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventL3ConnRecoveryStart(uint8_t dataId)
{
    // Send an event to all applications that L1 recovery has started.
    mcs_DataCtx_t *dataCtxPtr = GetDataCtx(dataId);
    if (nullptr == dataCtxPtr)
    {
        LE_ERROR("Unable to get reference for data id: %d", dataId);
        return;
    }

#ifndef LE_CONFIG_TARGET_SIMULATION
    auto &ecall = tafMngdConnECall::GetInstance();
    if (ecall.IsECallInProgress())
    {
        LE_WARN("eCall is in progress. Cannot proceed with L3 recovery");
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L3;
        // Inform admin that L3 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        return;
    }
#endif

    // Ensure service is in the correct state.
    if (MCS_RECOVERY_STARTED_L3 == dataCtxPtr->adminState)
    {
#ifndef LE_CONFIG_TARGET_SIMULATION
        le_result_t result = LE_OK;
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_STARTED, dataCtxPtr,
                            TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
        // Call API to start NAD reboot
        result = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
                         RestartReqAsyncCallBack, (void *)dataCtxPtr, TAF_MNGDPM_RESTART_REASON_NORMAL);
        if (LE_OK == result)
        {
            LE_INFO("Restart NAD request sent");
            dataCtxPtr->adminState = MCS_RECOVERY_STARTED_L3;
            dataCtxPtr->isConnectivityRecoveryScheduled = false;
        }
        else
        {
            LE_WARN("Restart NAD request failed: %d", result);
            // Update admin state that L3 recovery has failed
            dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L3;
            ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_FAILED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
            // Set the reconnected needed flag to TRUE
            dataCtxPtr->needReConn = true;
            // Inform admin that L3 recovery is interrupted
            stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
            le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
        }
#else
        LE_ERROR ("NAD reboot not started for simulation target");
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L3;
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_FAILED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
        // Inform admin that L3 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));
#endif // LE_CONFIG_TARGET_SIMULATION
    }
    else
    {
        LE_WARN("Invalid state: %d(%s)", dataCtxPtr->adminState,
                StateToString(dataCtxPtr->adminState));
        LE_INFO("Mark NAD Reboot recovery as interrupted and inform admin");
        // Update admin state that L3 recovery has failed
        dataCtxPtr->adminState = MCS_RECOVERY_FAILED_L3;
        ReportRecoveryEvent(TAF_MNGDCONN_RECOVERY_FAILED, dataCtxPtr,
                                TAF_MNGDCONN_RECOVERY_NAD_REBOOT);
        // Set the reconnected needed flag to TRUE
        dataCtxPtr->needReConn = true;
        // Inform admin that L3 recovery is interrupted
        stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT, 0};
        stateMachineEvt.dataId = dataCtxPtr->dataId;
        stateMachineEvt.event = MCS_EVT_CONN_RECOVERY_INTERRUPTED;
        le_event_Report(StateMachineEventId, &stateMachineEvt,
                        sizeof(stateMachineEvent_t));
        return;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * CURL helper method to perform the Curl operation
 */
//--------------------------------------------------------------------------------------------------
#ifdef LE_CONFIG_TAFMNGDCONNSVC_USE_CURL
bool tafMngdConnAdmin::PerformCurl(const char* URLStr, const char* interfacePtr)
{
    CURL *curl;
    CURLcode res;
    bool result;

    LE_INFO("curl URL: %s", URLStr);
    LE_INFO("curl interface: %s", interfacePtr);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, URLStr);
        curl_easy_setopt(curl, CURLOPT_INTERFACE, interfacePtr);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        // Complete within 2s
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        // Just check the connection.
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);
        // Check for errors
        if (res != CURLE_OK)
        {
            LE_WARN("cURL to %s error: %s", URLStr, curl_easy_strerror(res));
            result = false;
        }
        else
        {
            LE_INFO("cURL to %s succeeded.", URLStr);
            result = true;
        }

        // always cleanup
        curl_easy_cleanup(curl);
    }
    else
    {
        result = false;
        LE_WARN("Unable to initialize cURL");
    }

    curl_global_cleanup();

    return result;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * CURL helper method to remove the protocol from the URL
 */
//--------------------------------------------------------------------------------------------------
std::string tafMngdConnAdmin::RemoveProtocol(const std::string &url)
{
    LE_INFO("URL: %s", url.c_str());
    std::regex pattern("^https?://");
    std::string new_url = std::regex_replace(url, pattern, "");
    LE_INFO("New URL: %s", new_url.c_str());
    return new_url;
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper method to compare the Set and Get APNs
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnAdmin::CompareAPN(const char *getapnPtr, const char *setapnPtr)
{
    size_t getapnPtrLen = strlen(getapnPtr);
    size_t setapnPtrLen = strlen(setapnPtr);
    LE_INFO("getapnPtrLen length %d and setapnPtrLen length %d",
            (int)getapnPtrLen, (int)setapnPtrLen);

    if(getapnPtrLen == 0 && setapnPtrLen == 0)
    {
        LE_INFO("APN null for both json and NAD, no need to do anything");
        return false;
    }
    else if((getapnPtrLen > 0 && setapnPtrLen == 0) ||
            (setapnPtrLen > 0 && getapnPtrLen == 0) )
    {
        return true;
    }
    else if(getapnPtrLen > 0 && setapnPtrLen > 0)
    {
        //check if the APNs are same
        int compareLen = std::min(std::max(setapnPtrLen, getapnPtrLen),
                                            (size_t)MCS_MAX_APN_LEN);
        if (strncmp(setapnPtr, getapnPtr, compareLen) == 0)
        {
            LE_INFO("APN : %s already present", setapnPtr);
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

const char * tafMngdConnAdmin::EventToString(mcs_EventType_t event)
{
    switch (event)
    {
        case MCS_EVT_INIT:
            return "MCS_EVT_INIT";
        case MCS_EVT_SET_POLICY_CONF_SYNC:
            return "MCS_EVT_SET_POLICY_CONF_SYNC";
        case MCS_EVT_SIM_READY:
            return "MCS_EVT_SIM_READY";
        case MCS_EVT_SIM_NOT_READY:
            return "MCS_EVT_SIM_NOT_READY";
        case MCS_RADIO_POWER_ON:
            return "MCS_RADIO_POWER_ON";
        case MCS_EVT_NETWORK_REG_STATE:
            return "MCS_EVT_NETWORK_REG_STATE";
        case MCS_EVT_NETWORK_UNREG_STATE:
            return "MCS_EVT_NETWORK_UNREG_STATE";
        case MCS_EVT_DATA_START_SYNC:
            return "MCS_EVT_DATA_START_SYNC";
        case MCS_EVT_DATA_START:
            return "MCS_EVT_DATA_START";
        case MCS_EVT_DATA_START_RETRY:
            return "MCS_EVT_DATA_START_RETRY";
        case MCS_EVT_DATA_START_RETRY_APP_REQ:
            return "MCS_EVT_DATA_START_RETRY_APP_REQ";
        case MCS_EVT_DATA_STOP_SYNC:
            return "MCS_EVT_DATA_STOP_SYNC";
        case MCS_EVT_DATA_STOP:
            return "MCS_EVT_DATA_STOP";
        case MCS_EVT_DATA_CONNECTION_CONNECTED:
            return "MCS_EVT_DATA_CONNECTION_CONNECTED";
        case MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE:
            return "MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE";
        case MCS_EVT_DATA_CONNECTION_DISCONNECTED:
            return "MCS_EVT_DATA_CONNECTION_DISCONNECTED";
        case MCS_EVT_GET_CONNECTION_INFO_SYNC:
            return "MCS_EVT_GET_CONNECTION_INFO_SYNC";
        case MCS_EVT_DATA_START_CONNECTIONTEST:
            return "MCS_EVT_DATA_START_CONNECTIONTEST";
        case MCS_EVT_DATA_PERIODIC_CONNECTIONTEST:
            return "MCS_EVT_DATA_PERIODIC_CONNECTIONTEST";
        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L1:
            return "MCS_EVT_CONN_RECOVERY_SCHEDULE_L1";
        case MCS_EVT_CONN_RECOVERY_CANCEL:
            return "MCS_EVT_CONN_RECOVERY_CANCEL";
        case MCS_EVT_CONN_RECOVERY_CANCEL_SYNC:
            return "MCS_EVT_CONN_RECOVERY_CANCEL_SYNC";
        case MCS_EVT_CONN_RECOVERY_CANCEL_SEND_IND:
            return "MCS_EVT_CONN_RECOVERY_CANCEL_SEND_IND";
        case MCS_EVT_CONN_RECOVERY_INTERRUPTED:
            return "MCS_EVT_CONN_RECOVERY_INTERRUPTED";
        case MCS_EVT_CONN_RECOVERY_START_L1:
            return "MCS_EVT_CONN_RECOVERY_START_L1";
        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L2:
            return "MCS_EVT_CONN_RECOVERY_SCHEDULE_L2";
        case MCS_EVT_CONN_RECOVERY_START_L2:
            return "MCS_EVT_CONN_RECOVERY_START_L2";
        case MCS_EVT_CONN_RECOVERY_SCHEDULE_L3:
            return "MCS_EVT_CONN_RECOVERY_SCHEDULE_L3";
        case MCS_EVT_CONN_RECOVERY_START_L3:
            return "MCS_EVT_CONN_RECOVERY_START_L3";
        default:
            LE_ERROR("unknown status: %d", event);
            return "unknow status";
    }

    return "unknow status";
}

const char * tafMngdConnAdmin::StateToString(mcs_Admin_State_t state)
{
    switch (state)
    {
        case MCS_ADMIN_INIT:
            return "MCS_ADMIN_INIT";
        case MCS_ADMIN_ERROR:
            return "MCS_ADMIN_ERROR";
        case MCS_DATA_NOT_CONNECTED_SIM_READY:
            return "MCS_DATA_NOT_CONNECTED_SIM_READY";
        case MCS_DATA_NOT_CONNECTED_SIM_NOT_READY:
            return "MCS_DATA_NOT_CONNECTED_SIM_NOT_READY";
        case MCS_DATA_NOT_CONNECTED_NW_REGISTERED:
            return "MCS_DATA_NOT_CONNECTED_NW_REGISTERED";
        case MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
            return "MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED";
        case MCS_DATA_NOT_CONNECTED:
            return "MCS_DATA_NOT_CONNECTED";
        case MCS_DATA_NOT_CONNECTED_RETRYING:
            return "MCS_DATA_NOT_CONNECTED_RETRYING";
        case MCS_DATA_NOT_CONNECTED_FAILED:
            return "MCS_DATA_NOT_CONNECTED_FAILED";
        case MCS_DATA_CONNECTED_ACTIVE:
            return "MCS_DATA_CONNECTED_ACTIVE";
        case MCS_DATA_CONNECTED_INACTIVE:
            return "MCS_DATA_CONNECTED_INACTIVE";
        case MCS_DATA_CONNECTED_IDLE:
            return "MCS_DATA_CONNECTED_IDLE";
        case MCS_DATA_START_CONNECTIONTEST_START:
            return "MCS_DATA_START_CONNECTIONTEST_START";
        case MCS_DATA_START_CONNECTIONTEST_FAILED:
            return "MCS_DATA_START_CONNECTIONTEST_FAILED";
        case MCS_DATA_CONNECTED_INACTIVE_RETRYING:
            return "MCS_DATA_CONNECTED_INACTIVE_RETRYING";
        case MCS_DATA_NOT_CONNECTED_INACTIVE_RETRYING:
            return "MCS_DATA_NOT_CONNECTED_INACTIVE_RETRYING";
        case MCS_RECOVERY_SCHEDULED_L1:
            return "MCS_RECOVERY_SCHEDULED_L1";
        case MCS_RECOVERY_STARTED_L1:
            return "MCS_RECOVERY_STARTED_L1";
        case MCS_RECOVERY_CANCELED_L1:
            return "MCS_RECOVERY_CANCELED_L1";
        case MCS_RECOVERY_FAILED_L1:
            return "MCS_RECOVERY_FAILED_L1";
        case MCS_RECOVERY_SCHEDULED_L2:
            return "MCS_RECOVERY_SCHEDULED_L2";
        case MCS_RECOVERY_STARTED_L2:
            return "MCS_RECOVERY_STARTED_L2";
        case MCS_RECOVERY_CANCELED_L2:
            return "MCS_RECOVERY_CANCELED_L2";
        case MCS_RECOVERY_FAILED_L2:
            return "MCS_RECOVERY_FAILED_L2";
        case MCS_RECOVERY_SCHEDULED_L3:
            return "MCS_RECOVERY_SCHEDULED_L3";
        case MCS_RECOVERY_STARTED_L3:
            return "MCS_RECOVERY_STARTED_L3";
        case MCS_RECOVERY_CANCELED_L3:
            return "MCS_RECOVERY_CANCELED_L3";
        case MCS_RECOVERY_FAILED_L3:
            return "MCS_RECOVERY_FAILED_L3";
        default:
            LE_ERROR("unknown status: %d", state);
            return "unknown status";
    }

    return "unknow status";
}

const char *tafMngdConnAdmin::DataStateToString(taf_mngdConn_DataState_t state)
{
    switch (state)
    {
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            return "TAF_MNGDCONN_DATA_DISCONNECTED";
        case TAF_MNGDCONN_DATA_CONNECTED:
            return "TAF_MNGDCONN_DATA_CONNECTED";
        case TAF_MNGDCONN_DATA_CONNECTION_STALLED:
            return "TAF_MNGDCONN_DATA_CONNECTION_STALLED";
        case TAF_MNGDCONN_DATA_CONNECTION_FAILED:
            return "TAF_MNGDCONN_DATA_CONNECTION_FAILED";
        default:
            LE_ERROR("unknown status: %d", state);
            break;
        }
    return "unknow status";
}
