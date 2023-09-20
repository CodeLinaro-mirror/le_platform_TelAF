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

#include "tafMngdConn_ConfigTreeHelper.hpp"
#include "tafMngdConnData.hpp"
#include "tafMngdConnRadio.hpp"
#include "tafMngdConnSim.hpp"
#include "tafMngdConnAdmin.hpp"

using namespace telux::tafsvc;

#define CONFIG_FILE_NAME "mngdConnectivity.json"

LE_MEM_DEFINE_STATIC_POOL(tafMngdConn, TAF_DCS_MAX_CALL_OBJ, sizeof(taf_mngd_Conn_Ctx_t));

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

//--------------------------------------------------------------------------------------------------
/**
 * Read JSON file names from Config Tree nodes
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnAdmin::ReadJSONFileNamesFromConfigTree
(
    char *ConfigurationFileNamePtr
)
{
    le_result_t result = LE_OK;
    char ConfigFileNameUpdate[TAF_MNGD_CONN_MAX_YES_NO_LEN] = {0};

    if (ConfigurationFileNamePtr == NULL )
    {
        LE_WARN ("Invalid Parameters passed");
        return false;
    }

    // The Config Tree Node names are defined in tafMngdConn_ConfigTreeHelper.hpp

    // Read Configuration File Name
    result = tafMngd_ConfigTree_Read(TAF_MNGD_ct_node_ConfigurationFileName,
                                    ConfigurationFileNamePtr,
                                    TAF_MNGD_CONN_MAX_FILE_NAME_LEN);
    if ( result != LE_OK )
    {
        LE_WARN ("Error in reading Configuration File Name");
        return false;
    }
    else if ( strlen(ConfigurationFileNamePtr) == 0 )
    {
        LE_WARN("Configuration Filename not present in Config Tree");
        return false;
    }
    LE_INFO("Configuration File Name: %s", ConfigurationFileNamePtr);

    // Read if the Configuration File Name was overridden
    result = tafMngd_ConfigTree_Read(TAF_MNGD_ct_node_ConfigurationFileNameOverride,
                                                                    ConfigFileNameUpdate,
                                                                    TAF_MNGD_CONN_MAX_YES_NO_LEN);
    if ( result != LE_OK )
    {
        LE_INFO ("Configuration File Name overridden by API provided value");
        return false;
    }
    return true;
}

void tafMngdConnAdmin::Init(void)
{
    //Initiate data module.
    Data_init();
    //Initiate radio module.
    Radio_init();
    //Initiate sim module.
    Sim_init();

    //Initiate the memory pool.
    ConnCtxPool = le_mem_InitStaticPool(tafMngdConn, TAF_DCS_MAX_CALL_OBJ,
                                            sizeof(taf_mngd_Conn_Ctx_t));

    connStatePool = le_mem_CreatePool("connStatePool", sizeof(DataState_t));

    //Create reference map for data context
    DataRefMap = le_ref_CreateMap("DataRefMap", TAF_DCS_MAX_CALL_OBJ);

    //Create the mutex.
    connCtxMutex = le_mutex_CreateNonRecursive("connCtxMutex");

    //Create the semaphore
    le_sem_Ref_t semRef = le_sem_Create("SmThreadSem", 0);

    //Create the event handle thread
    StateMachineEventThreadRef = le_thread_Create("MngdEvtThread", StateMachineEventThread,
                                                (void*)semRef);
    le_thread_Start(StateMachineEventThreadRef);
    le_sem_Wait(semRef);

    //Create the callback handle thread
    tafMngd_event_thread = le_thread_Create("MngdCbThread",  callback_thread,  (void*)semRef);
    le_thread_Start (tafMngd_event_thread);
    le_sem_Wait(semRef);

    le_sem_Delete(semRef);

    // Check if ConfigTree has the Policy and Configuration File names
    if (tafMngdConnSvc_GetPolicyAndConfiguration( Policy, Configuration, CONFIG_FILE_NAME))
    {
        // Policy and Configuration parsed and validated. Move to "Data-Not_Connected" state
        LE_DEBUG("JSONs parsed. Initialization Complete and set IsJsonValid with true");
        IsJsonValid = true;
        stateMachineEvent_t stateMachineEvt;
        // Report the event to the state machine
        stateMachineEvt.event=EVT_INIT;
        le_event_Report(StateMachineEventId, &stateMachineEvt,
                                                        sizeof(stateMachineEvent_t));
    }
    else
    {
        LE_FATAL("Correct JSON files are needed.");
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Callback thread function.
 */
//--------------------------------------------------------------------------------------------------
void* tafMngdConnAdmin::callback_thread(void* contextPtr)
{
    LE_INFO ("Admin callback_thread Entry");
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
 * Set policy and configuration JSONs to config tree, and start the data connections if needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::SetPolicyConfigurationJSONs
(
    const char* ConfigFileNamePtr
)
{
    le_result_t result = LE_OK;
    CmdSynchronousPromise = std::promise<le_result_t>();

    if(ConfigFileNamePtr == NULL)
    {
        LE_ERROR ("Invalid Parameters passed");
        return LE_FAULT;
    }

    //Check if at least one connection is created.
    if(IsStateConnected())
    {
        LE_ERROR("Some connections are already active");
        return LE_FAULT;
    }

    // Check if ConfigTree has the Configuration File name
    char PresentConfFileName[TAF_MNGD_CONN_MAX_FILE_NAME_LEN]={0};
    if (IsJsonValid && ReadJSONFileNamesFromConfigTree(PresentConfFileName))
    {
        if ((strncmp(PresentConfFileName, ConfigFileNamePtr, sizeof(PresentConfFileName)) == 0))
        {
            LE_ERROR ("Policy and Configuration file names are same");
            return LE_DUPLICATE;
        }
        else
        {
            LE_ERROR ("Policy and Configuration file names are already configured");
            return LE_FAULT;
        }
    }

    le_utf8_Copy(ConfigFileName, ConfigFileNamePtr, MAX_MNGD_CONN_FILE_PATH_LEN,NULL);

    stateMachineEvent_t stateMachineEvt;
    stateMachineEvt.event=EVT_SET_POLICY_CONF;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the data reference for the given Data ID.
 */
//--------------------------------------------------------------------------------------------------
taf_mngd_Conn_DataRef_t tafMngdConnAdmin::GetRefByDataId(uint8_t dataId)
{

    taf_mngd_Conn_Ctx_t* connCtxPtr = GetConnCtx(dataId);

    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return NULL;
    }

    return connCtxPtr->dataRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data connection with the specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::Startdata(taf_mngd_Conn_DataRef_t dataRef)
{
    le_result_t result = LE_OK;
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    CmdSynchronousPromise = std::promise<le_result_t>();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");

    connCtxPtr = (taf_mngd_Conn_Ctx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);
    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }

    stateMachineEvent_t stateMachineEvt;
    stateMachineEvt.event=EVT_DATA_START;
    stateMachineEvt.dataId=connCtxPtr->dataId;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data connection with the specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::Stopdata(taf_mngd_Conn_DataRef_t dataRef)
{
    le_result_t result = LE_OK;
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    CmdSynchronousPromise = std::promise<le_result_t>();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");

    connCtxPtr = (taf_mngd_Conn_Ctx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);
    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }

    stateMachineEvent_t stateMachineEvt;
    stateMachineEvt.event=EVT_DATA_STOP;
    stateMachineEvt.dataId=connCtxPtr->dataId;
    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    //wait until return
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection state with specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetConnectionState
(
    taf_mngd_Conn_DataRef_t dataRef,
    uint8_t* dataIdPtr,
    taf_mngd_Conn_DataState_t *statePtr
)
{
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(dataIdPtr == NULL, LE_BAD_PARAMETER, "Null ptr(dataIdPtr)");
    TAF_ERROR_IF_RET_VAL(statePtr == NULL, LE_BAD_PARAMETER, "Null ptr(statePtr)");

    connCtxPtr = (taf_mngd_Conn_Ctx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        *statePtr = TAF_MNGD_CONN_DATA_DISCONNECTED;
        return LE_FAULT;
    }

    *dataIdPtr = connCtxPtr->dataId;

    if(connCtxPtr->dataState == TAF_MNGD_CONN_DATA_DISCONNECTED)
        *statePtr = TAF_MNGD_CONN_DATA_DISCONNECTED;
    else
        *statePtr = TAF_MNGD_CONN_DATA_CONNECTED;

    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection IP addresses with specified data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::GetConnectionIPAddresses
(
    taf_mngd_Conn_DataRef_t dataRef,
    char *ipv4AddrPtr,
    size_t ipv4AddrSize,
    char *ipv6AddrPtr,
    size_t ipv6AddrSize
)
{
    le_result_t result = LE_OK;
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    stateMachineEvent_t stateMachineEvt;
    CmdSynchronousPromise = std::promise<le_result_t>();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_BAD_PARAMETER, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(ipv4AddrPtr == NULL, LE_BAD_PARAMETER, "Null ptr(ipv4AddrPtr)");
    TAF_ERROR_IF_RET_VAL(ipv6AddrPtr == NULL, LE_BAD_PARAMETER, "Null ptr(ipv6AddrPtr)");

    memset(ipv4AddrPtr, 0, ipv4AddrSize);
    memset(ipv6AddrPtr, 0, ipv6AddrSize);

    connCtxPtr = (taf_mngd_Conn_Ctx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }

    if(connCtxPtr->dataState == TAF_MNGD_CONN_DATA_DISCONNECTED)
    {
        LE_INFO("State is not active");
        return LE_OK;
    }

    stateMachineEvt.event=EVT_GET_CONNECTION_INFO;
    stateMachineEvt.dataId=connCtxPtr->dataId;
    //wait until return

    le_event_Report(StateMachineEventId, &stateMachineEvt, sizeof(stateMachineEvent_t));

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    if(result == LE_OK)
    {
        switch(connCtxPtr->ipType)
        {
            case TAF_DCS_PDP_IPV4:
                le_utf8_Copy(ipv4AddrPtr, connCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
            break;
            case TAF_DCS_PDP_IPV6:
                le_utf8_Copy(ipv6AddrPtr, connCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
            break;
            case TAF_DCS_PDP_IPV4V6:
                le_utf8_Copy(ipv4AddrPtr, connCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(ipv6AddrPtr, connCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
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

/*=====================================Event handle functions.===================================*/
//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_INIT which is sent when system startup.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventInit()
{

    CreateConnectionsBasedPolicy();

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_SET_POLICY_CONF which is sent by calling the API.
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

    if((strlen(ConfigFileNamePtr) >= MAX_MNGD_CONN_FILE_PATH_LEN))
    {
        LE_ERROR ("Invalid file name");
        return LE_FAULT;
    }

    if (tafMngdConnSvc_GetPolicyAndConfiguration(Policy, Configuration, ConfigFileNamePtr))
    {
        LE_DEBUG("JSONs parsed and validated");
        LE_DEBUG("Version          : %d", Policy.Version);
        LE_DEBUG("Name             : %s", Policy.Name);
        LE_DEBUG("Config File Name : %s", Policy.ConfigurationFileName);
        LE_DEBUG("\tFallback  : %d", Policy.DataSession.Fallback);
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
        return CreateConnectionsBasedPolicy();
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
 * Handle the event EVT_DATA_START which is sent by calling API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStartData(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    auto &radio = tafMngdConnRadio::GetInstance();
    le_result_t result;

    taf_mngd_Conn_Ctx_t* connCtxPtr = GetConnCtx(dataId);

    if(connCtxPtr == NULL)
    {
        LE_ERROR("Json is needed");
        return LE_FAULT;
    }

    //Do action according to the current state.
    switch(connCtxPtr->state)
    {
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
            result = radio.StartUp(connCtxPtr->phoneId);
            if(result != LE_OK)
            {
                LE_ERROR("Radio startup failed");
                return LE_FAULT;
            }

            //Update connCtxPtr->state according to the network register state.
            if(radio.IsNetworkRegistered(connCtxPtr->phoneId))
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED;
            else
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;

            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);

            if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED)
            {
                result = data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);
                if(result == LE_OK)
                {
                    //connection is created.
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);
                    //If manually started the data successfully. Set reconnection flag to true.
                    connCtxPtr->needReConn = true;
                    return LE_OK;
                }
                else
                {
                    LE_ERROR("Starting a data call failed");
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    LE_INFO("Registration is in progress");
                    le_timer_SetMsInterval(connCtxPtr->reconnTimerRef, RECONNECT_TIME_INTERVAL);
                    le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
                    le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
                    le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
                    le_timer_Start(connCtxPtr->reconnTimerRef);
                    return LE_IN_PROGRESS;
                }
            }
            else
            {
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
                ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                LE_INFO("Registration is in progress");
                le_timer_SetMsInterval(connCtxPtr->reconnTimerRef, RECONNECT_TIME_INTERVAL);
                le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
                le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
                le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
                le_timer_Start(connCtxPtr->reconnTimerRef);
                return LE_IN_PROGRESS;
            }
            break;
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY:
            LE_ERROR("Sim is not ready or network is not registered");
            return LE_FAULT;
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED:
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND:

            result = data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);
            if(result == LE_OK)
            {
                //connection is created.
                connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
                ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);
                //If manually started the data successfully. Set reconnection flag to true.
                connCtxPtr->needReConn = true;
                return LE_OK;
            }
            else
            {
                LE_ERROR("Starting a data call failed");
                return result;
            }

            break;
        case TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE:

            LE_INFO("Already active");
            return LE_DUPLICATE;
            break;
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING:
            LE_INFO("RETRYING......");
            return LE_IN_PROGRESS;
        default:
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_DATA_STOP which is sent by calling API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventStopData(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    le_result_t result;

    taf_mngd_Conn_Ctx_t* connCtxPtr = GetConnCtx(dataId);
    if(connCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context");
        return LE_FAULT;
    }

    // Check if the data is started automatically by the service.
    // If true, return LE_NOT_PERMITTED
    // If false, allow DataStop to proceed.
    if (true == connCtxPtr->autoStart)
    {
        LE_WARN("Stopping auto started(Autostart: Yes) data session is not allowed");
        return LE_NOT_PERMITTED;
    }

    //Do action according to the current state.
    switch(connCtxPtr->state)
    {
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY:
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED:
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND:
            LE_ERROR("Not started");
            return LE_FAULT;
        case TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE:
            result = data.Stopdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);
            if(result == LE_OK)
            {
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
                ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                //If manually stopped the data successfully. Set reconnection flag to false.
                connCtxPtr->needReConn = false;
                return LE_OK;
            }
            else
            {
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
                LE_ERROR("Stopping data failed");
                ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                return LE_FAULT;
            }
            break;
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING:
            connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
            //If manually stopped the data successfully. Set reconnection flag to false.
            connCtxPtr->needReConn = false;
            LE_INFO("Cancel retrying...");
            le_timer_Stop(connCtxPtr->reconnTimerRef);
            break;
        default:
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_GET_CONNECTION_INFO which is sent by calling API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventGetConnectionInfo(uint8_t dataId)
{
    auto &data = tafMngdConnData::GetInstance();
    le_result_t result;

    if(dataId != 0xff)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = GetConnCtx(dataId);
        if(connCtxPtr == NULL)
        {
            LE_ERROR("Can't find the context");
            return LE_FAULT;
        }
        result = data.GetConnectionInfo(connCtxPtr);
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
 * Handle the event EVT_SIM_READY which is sent by SIM module when SIM status is ready.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventSimReadyState(uint8_t slotId)
{
    le_dls_Link_t* linkPtr = NULL;
    auto &radio = tafMngdConnRadio::GetInstance();
    le_result_t result;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);

    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if(slotId == connCtxPtr->slotId)
        {
            if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY ||
               connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED)
            {
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY;
                result = radio.StartUp(connCtxPtr->phoneId);
                if(result != LE_OK)
                {
                    LE_ERROR("Radio startup failed");
                    continue;
                }
            }

            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
        }

    }

    le_mutex_Unlock(connCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_SIM_NOT_READY which is sent by SIM module when SIM status is not ready.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventSimNotReadyState(uint8_t slotId)
{

    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if(slotId == connCtxPtr->slotId)
        {
            if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY)
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY;

            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
        }
    }

    le_mutex_Unlock(connCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_NETWORK_REG_STATE which is sent by radio module when network status is
 * registered.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventNetworkRegState(uint8_t phoneId)
{

    auto &data = tafMngdConnData::GetInstance();
    le_dls_Link_t* linkPtr = NULL;
    le_result_t result;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);

    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if(phoneId == connCtxPtr->phoneId)
        {
            LE_INFO("current state= %d, autoStart = %d, needReconn=%d",
                     connCtxPtr->state, connCtxPtr->autoStart, connCtxPtr->needReConn);
            //Do action according to the current state.
            switch(connCtxPtr->state)
            {
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY:
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY:

                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    //Start a data call if autoStart, or reconnection flag is true
                    if(connCtxPtr->autoStart || connCtxPtr->needReConn)
                    {
                        result = data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);
                        if( result == LE_OK)
                        {
                            //connection is created.
                            connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
                            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);
                            continue;
                        }
                        else if( result == LE_UNSUPPORTED)
                        {
                            //Not default phone ID.
                            continue;
                        }
                        else
                        {
                            connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
                            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                            LE_INFO("starting data call failed, retry");
                            le_timer_SetMsInterval(connCtxPtr->reconnTimerRef,
                                                   RECONNECT_TIME_INTERVAL);
                            le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
                            le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
                            le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
                            le_timer_Start(connCtxPtr->reconnTimerRef);
                            continue;
                        }
                    }
                    else
                    {
                        connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
                        ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    }
                    break;
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING:
                    //Currently the retry timer is 3s and one time shot. If network registered after
                    //retrying, need to start it again.
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    LE_INFO("Cancel retry and start again");
                    le_timer_Stop(connCtxPtr->reconnTimerRef);

                    result = data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);
                    if( result == LE_OK)
                    {
                        //connection is created.
                        connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
                        ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);
                        continue;
                    }
                    else
                    {
                        //Not successful.
                        continue;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    le_mutex_Unlock(connCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_NETWORK_UNREG_STATE which is sent by radio module when network status is
 * unregistered.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::EventNetworkUnregState(uint8_t phoneId)
{

    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if(phoneId == connCtxPtr->phoneId)
        {
            //Do action according to the current state.
            switch(connCtxPtr->state)
            {
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY:
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY:
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED:
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND:
                case TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE:
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    break;
                case TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING:
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    LE_INFO("Cancel retry...");
                    le_timer_Stop(connCtxPtr->reconnTimerRef);
                    break;
                default:
                    break;
            }

        }

    }

    le_mutex_Unlock(connCtxMutex);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_DATA_CONNECTION_CONNECTED which is sent by data module when the data
 * connection is created.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataConnected(uint8_t dataId)
{

    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;

    connCtxPtr = GetConnCtx(dataId);

    if(connCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context for dataId(%d)", dataId);
        return;
    }

    //Status is set with value TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE after calling the SYNC API
    //successfully. Don't need to do anything here. Otherwise tafMngdConnSvc status will be
    // impacted by Xtra-daemon which starts/stops data session sometimes.

}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event EVT_DATA_CONNECTION_DISCONNECTED which is sent by data module when the data
 * connection is destroyed.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::EventDataDisconnected(uint8_t dataId)
{
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;

    connCtxPtr = GetConnCtx(dataId);
    if(connCtxPtr == NULL)
    {
        LE_ERROR("Can't find the context for dataId(%d)", dataId);
        return;
    }

    //Do action according to the current state.
    switch(connCtxPtr->state)
    {
        case TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE:
            connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
            ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
            LE_INFO("Data call disconnected, retrying");
            le_timer_SetMsInterval(connCtxPtr->reconnTimerRef, RECONNECT_TIME_INTERVAL);
            le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
            le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
            le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
            le_timer_Start(connCtxPtr->reconnTimerRef);
            break;
        default:
            break;
    }

}

/*===================================End Event process functions.=================================*/

//--------------------------------------------------------------------------------------------------
/**
 * StateMachineEventThread.
 */
//--------------------------------------------------------------------------------------------------
void* tafMngdConnAdmin::StateMachineEventThread(void* contextPtr)
{
    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();

    le_cfg_ConnectService();
    taf_radio_ConnectService();
    taf_dcs_ConnectService();
    taf_sim_ConnectService();

    // internal event handler
    mngdConnAdmin.StateMachineEventId = le_event_CreateId("Sm Event", sizeof(stateMachineEvent_t));
    le_event_AddHandler("StateMachine Event Handler", mngdConnAdmin.StateMachineEventId,
                         StateMachineHandler);

    le_sem_Post(semRef);

    LE_INFO("Create event loop for state machine event");

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * StateMachineHandler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::StateMachineHandler(void* reqPtr)
{
    le_result_t result = LE_OK;
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    stateMachineEvent_t* eventReq = (stateMachineEvent_t*)reqPtr;

    if(eventReq == NULL)
    {
        LE_ERROR ("Invalid Parameters passed");
        return;
    }

    LE_DEBUG("STATE MACHINE --received event=%s", mngdConnAdmin.EventToString(eventReq->event));

    switch (eventReq->event) {
        case EVT_INIT:
            mngdConnAdmin.EventInit();
            break;
        case EVT_SET_POLICY_CONF:

            result = mngdConnAdmin.EventSetPolicyConfigJSONs(mngdConnAdmin.ConfigFileName);

            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;
        case EVT_DATA_START:

            result = mngdConnAdmin.EventStartData(eventReq->dataId);

            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;
        case EVT_DATA_STOP:

            result = mngdConnAdmin.EventStopData(eventReq->dataId);

            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;
        case EVT_GET_CONNECTION_INFO:

            result = mngdConnAdmin.EventGetConnectionInfo(eventReq->dataId);

            mngdConnAdmin.CmdSynchronousPromise.set_value(result);
            break;
        case EVT_SIM_READY:

             LE_DEBUG("slotId=%d", eventReq->slotId);
             mngdConnAdmin.EventSimReadyState(eventReq->slotId);
            break;
        case EVT_SIM_NOT_READY:

             LE_DEBUG("slotId=%d", eventReq->slotId);
             mngdConnAdmin.EventSimNotReadyState(eventReq->slotId);
            break;
        case EVT_NETWORK_REG_STATE:

             LE_DEBUG("phoneid=%d", eventReq->phoneId);
             mngdConnAdmin.EventNetworkRegState(eventReq->phoneId);
            break;
        case EVT_NETWORK_UNREG_STATE:

             LE_DEBUG("phoneid=%d", eventReq->phoneId);
             mngdConnAdmin.EventNetworkUnregState(eventReq->phoneId);
            break;
        case EVT_DATA_CONNECTION_CONNECTED:

            LE_DEBUG("dataId = %d", eventReq->dataId);
            mngdConnAdmin.EventDataConnected(eventReq->dataId);
            break;
        case EVT_DATA_CONNECTION_DISCONNECTED:

            LE_DEBUG("dataId = %d", eventReq->dataId);
            mngdConnAdmin.EventDataDisconnected(eventReq->dataId);
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
taf_mngd_Conn_Ctx_t* tafMngdConnAdmin::GetConnCtx(uint8_t dataId)
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if (connCtxPtr->dataId == dataId)
        {
            le_mutex_Unlock(connCtxMutex);
            return connCtxPtr;
        }
    }

    le_mutex_Unlock(connCtxMutex);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connectivity context by phone id and profileNumber.
 */
//--------------------------------------------------------------------------------------------------
taf_mngd_Conn_Ctx_t* tafMngdConnAdmin::GetConnCtx(uint8_t phoneId, uint32_t profileNumber)
{
    le_dls_Link_t* linkPtr = NULL;
    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if (connCtxPtr->phoneId == phoneId && connCtxPtr->profileNumber == profileNumber)
        {
            le_mutex_Unlock(connCtxMutex);
            return connCtxPtr;
        }
    }

    le_mutex_Unlock(connCtxMutex);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create connectivity context.
 */
//--------------------------------------------------------------------------------------------------
taf_mngd_Conn_Ctx_t* tafMngdConnAdmin::CreateConnCtx
(
    uint8_t dataId,
    uint8_t slotId,
    uint8_t phoneId,
    uint32_t profileNumber,
    bool autoStart
)
{
    char timerName[32] = {0};
    char eventName[32] = {0};
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    connCtxPtr = (taf_mngd_Conn_Ctx_t*)le_mem_ForceAlloc(ConnCtxPool);
    TAF_ERROR_IF_RET_VAL(connCtxPtr == NULL, NULL, "cannot alloc connCtxPtr");

    connCtxPtr->dataId = dataId;
    connCtxPtr->slotId = slotId;
    connCtxPtr->phoneId = phoneId;
    connCtxPtr->profileNumber = profileNumber;
    connCtxPtr->autoStart = autoStart;
    connCtxPtr->needReConn = false;
    connCtxPtr->state = TAF_MNGD_CONN_ADMIN_INIT;
    connCtxPtr->dataState = TAF_MNGD_CONN_DATA_DISCONNECTED;
    connCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
    memset(connCtxPtr->intfName, 0, sizeof(connCtxPtr->intfName));

    //Create timer
    snprintf(timerName, sizeof(timerName)-1, "dataId-%d Timer", dataId);
    connCtxPtr->reconnTimerRef = le_timer_Create(timerName);

    //Create event id
    snprintf(eventName, sizeof(eventName)-1, "connCtx-%d", dataId);
    connCtxPtr->dataStateEvent = le_event_CreateIdWithRefCounting(eventName);

    // create reference for this connectivity context
    taf_mngd_Conn_DataRef_t dataRef = (taf_mngd_Conn_DataRef_t)le_ref_CreateRef(DataRefMap,
                                                                                (void *)connCtxPtr);
    TAF_ERROR_IF_RET_VAL(connCtxPtr == NULL, NULL, "Cannot alloc dataRef");

    connCtxPtr->dataRef = dataRef;

    le_mutex_Lock(connCtxMutex);
    le_dls_Queue(&ConnectionCtxList, &connCtxPtr->link);
    le_mutex_Unlock(connCtxMutex);

    return connCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if at least one connection is created.
 */
//--------------------------------------------------------------------------------------------------
bool tafMngdConnAdmin::IsStateConnected()
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        if (connCtxPtr->state == TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE)
        {
            le_mutex_Unlock(connCtxMutex);
            return true;
        }
    }

    le_mutex_Unlock(connCtxMutex);
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get data state event ID.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t tafMngdConnAdmin::GetDataStateEvent(taf_mngd_Conn_DataRef_t dataRef)
{
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;

    connCtxPtr = (taf_mngd_Conn_Ctx_t* )le_ref_Lookup(DataRefMap, (void*)dataRef);

    TAF_ERROR_IF_RET_VAL(connCtxPtr == NULL, NULL,
                         "Cannot find connectivity context for dataRef(%p)",
                         dataRef);

    return connCtxPtr->dataStateEvent;
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

    le_mutex_Lock(connCtxMutex);
    linkPtr = le_dls_Peek(&ConnectionCtxList);
    while (linkPtr)
    {
        taf_mngd_Conn_Ctx_t* connCtxPtr = CONTAINER_OF(linkPtr, taf_mngd_Conn_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&ConnectionCtxList, linkPtr);
        profileNumberList[profileCnt].phoneId = connCtxPtr->phoneId;
        profileNumberList[profileCnt].profileNumber = connCtxPtr->profileNumber;
        profileNumberList[profileCnt].dataState = connCtxPtr->dataState;
        profileCnt++;
    }

    le_mutex_Unlock(connCtxMutex);
    *listSize = profileCnt;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerConnStateHandler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::FirstLayerConnStateHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    DataState_t* dataStateEvent = (DataState_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_mngd_Conn_DataStateHandlerFunc_t handlerFunc =
                                    (taf_mngd_Conn_DataStateHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(dataStateEvent->dataRef, dataStateEvent->dataState, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report connectivity state.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ReportAndUpdateDataState
(
    taf_mngd_Conn_Ctx_t* connCtxPtr,
    taf_mngd_Conn_DataState_t newstate
)
{
    TAF_ERROR_IF_RET_NIL(connCtxPtr == NULL, "Null pointer");
    auto &admin = tafMngdConnAdmin::GetInstance();

    if(connCtxPtr->dataState == newstate)
        return;

    DataState_t* connStateIndPtr =
                (DataState_t*)le_mem_ForceAlloc(admin.connStatePool);

    connStateIndPtr->dataRef = connCtxPtr->dataRef;
    connStateIndPtr->dataState = newstate;
    LE_INFO("#########send state dataRef=%p, state = %d  ",
            connStateIndPtr->dataRef, newstate);

    le_event_ReportWithRefCounting(connCtxPtr->dataStateEvent, (void*)connStateIndPtr);
    //Update data state
    connCtxPtr->dataState = newstate;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create connections according to the configuration in JSON.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnAdmin::CreateConnectionsBasedPolicy()
{
    uint8_t sessionIdx, dataIdx, networkIdx, simIdx;
    uint8_t dataId = 0, phoneId = 0, slotNumber = 0;
    uint32_t profileNumber = 0;
    bool autoStart = false;
    le_result_t result;
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    auto &data = tafMngdConnData::GetInstance();
    auto &radio = tafMngdConnRadio::GetInstance();
    auto &sim = tafMngdConnSim::GetInstance();

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

            dataId = Configuration.Data[dataIdx].ID;
            autoStart = Configuration.Data[dataIdx].AutoStart;
            profileNumber = Configuration.Data[dataIdx].Profile.ProfileNumber;

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

            LE_INFO("dataId = %d, phoneId = %d, profileNumber=%d, autostart=%d",
                     dataId, phoneId, profileNumber, autoStart);
            connCtxPtr = GetConnCtx(dataId);
            if(connCtxPtr == NULL)
            {
                connCtxPtr = CreateConnCtx(dataId, slotNumber, phoneId, profileNumber, autoStart);
                if(connCtxPtr == NULL)
                {
                    LE_ERROR("Creating connection context failed");
                    continue;
                }

                ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
            }

            if(sim.IsSimReady(slotNumber))
            {
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY;
            }
            else
            {
                LE_ERROR("SIM NOT READY");
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY;
                continue;
            }

            result = radio.StartUp(connCtxPtr->phoneId);
            if(result != LE_OK)
            {
                LE_ERROR("Radio startup failed");
                continue;
            }

            //Update connCtxPtr->state according to the network register state.
            if(radio.IsNetworkRegistered(connCtxPtr->phoneId))
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED;
            else
                connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED;

            if(Configuration.Data[dataIdx].AutoStart)
            {
                if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED)
                {
                    result = data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber);

                    if(result == LE_OK)
                    {
                        //connection is created.
                        connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
                        ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);
                        continue;
                    }
                    else if(result == LE_UNSUPPORTED)
                    {
                        //Not default phone ID.
                        connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
                        ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                        LE_ERROR("Not default phone ID");
                        continue;
                    }
                    else
                    {
                        connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
                        ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                        LE_INFO("Starting data call failed, retry");
                        le_timer_SetMsInterval(connCtxPtr->reconnTimerRef, RECONNECT_TIME_INTERVAL);
                        le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
                        le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
                        le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
                        le_timer_Start(connCtxPtr->reconnTimerRef);
                        continue;
                    }
                }
                else
                {
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                    LE_INFO("Starting data call failed, retry");
                    le_timer_SetMsInterval(connCtxPtr->reconnTimerRef, RECONNECT_TIME_INTERVAL);
                    le_timer_SetRepeat(connCtxPtr->reconnTimerRef, RECONNECT_RETRY_COUNT);
                    le_timer_SetContextPtr(connCtxPtr->reconnTimerRef, (void*)connCtxPtr);
                    le_timer_SetHandler(connCtxPtr->reconnTimerRef, ReconnectTimerHandler);
                    le_timer_Start(connCtxPtr->reconnTimerRef);
                    continue;
                }
            }
            //Not auto start
            else
            {
                if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED)
                {
                    connCtxPtr->state = TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND;
                    ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_DISCONNECTED);
                }
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnAdmin::ReconnectTimerHandler(le_timer_Ref_t timerRef)
{
    auto &data = tafMngdConnData::GetInstance();
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    taf_mngd_Conn_Ctx_t* connCtxPtr = (taf_mngd_Conn_Ctx_t *)le_timer_GetContextPtr(timerRef);
    if(connCtxPtr == NULL)
    {
        LE_INFO("Stop the timer.");
        le_timer_Stop(timerRef);
        return;
    }
    LE_INFO("Timer handler");
    //If need to reconnect, start a data call
    if(connCtxPtr->state == TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING)
    {
        if(data.Startdata(connCtxPtr->phoneId, connCtxPtr->profileNumber) == LE_OK)
        {
            //connection is created.
            connCtxPtr->state = TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE;
            mngdConnAdmin.ReportAndUpdateDataState(connCtxPtr, TAF_MNGD_CONN_DATA_CONNECTED);

            //If manually started the data successfully. Set reconnection flag to true.
            if(!connCtxPtr->autoStart)
                connCtxPtr->needReConn = true;
        }
    }

    if(connCtxPtr->state == TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE)
    {
        LE_INFO("Connected, stop the timer.");
        le_timer_Stop(timerRef);
    }

}

const char * tafMngdConnAdmin::EventToString(EventType_t event)
{
    switch (event)
    {
        case EVT_INIT:
            return "EVT_INIT";
        case EVT_SET_POLICY_CONF:
            return "EVT_SET_POLICY_CONF";
        case EVT_SIM_READY:
            return "EVT_SIM_READY";
        case EVT_SIM_NOT_READY:
            return "EVT_SIM_NOT_READY";
        case EVT_NETWORK_REG_STATE:
            return "EVT_NETWORK_REG_STATE";
        case EVT_NETWORK_UNREG_STATE:
            return "EVT_NETWORK_UNREG_STATE";
        case EVT_DATA_START:
            return "EVT_DATA_START";
        case EVT_DATA_STOP:
            return "EVT_DATA_STOP";
        case EVT_DATA_CONNECTION_CONNECTED:
            return "EVT_DATA_CONNECTION_CONNECTED";
        case EVT_DATA_CONNECTION_DISCONNECTED:
            return "EVT_DATA_CONNECTION_DISCONNECTED";
        case EVT_GET_CONNECTION_INFO:
            return "EVT_GET_CONNECTION_INFO";
        default:
            LE_ERROR("unknown status: %d", event);
            return "unknow status";
    }

    return "unknow status";
}

const char * tafMngdConnAdmin::StateToString(taf_mngd_Conn_Admin_State_t state)
{
    switch (state)
    {
        case TAF_MNGD_CONN_ADMIN_INIT:
            return "TAF_MNGD_CONN_ADMIN_INIT";
        case TAF_MNGD_CONN_ADMIN_ERROR:
            return "TAF_MNGD_CONN_ADMIN_ERROR";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND";
        case TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING:
            return "TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING";
        case TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE:
            return "TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE";
        case TAF_MNGD_CONN_DATA_CONNECTED_IDLE:
            return "TAF_MNGD_CONN_DATA_CONNECTED_IDLE";
        default:
            LE_ERROR("unknown status: %d", state);
            return "unknow status";
    }

    return "unknow status";
}
