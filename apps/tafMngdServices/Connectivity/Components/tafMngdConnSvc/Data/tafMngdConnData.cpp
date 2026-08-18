/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdConnData.hpp"
#include "tafMngdConnAdmin.hpp"
#include "tafDcsHelper.hpp"

#define MIN_PHONE_ID 1
#define MAX_PHONE_ID 2

using namespace tafsvc;

// Boolean variable to indicate if we are waiting for a promise to be fulfilled
// This tracks if async data start/stop times out and the promise is not needed anymore.
static std::atomic<bool>
    bWaitingForPromise = {false};

//Initialize static variables
std::promise<le_result_t> tafMngdConnData::AsyncAPIPromise;
le_thread_Ref_t tafMngdConnData::dataThreadRef = NULL;
le_sem_Ref_t tafMngdConnData::semRef = NULL;

void tafMngdConnData::Init(void)
{
    semRef = le_sem_Create("SmThreadSem", 0);
    dataThreadRef = le_thread_Create("DataSessionThread",
                                               DataThreadHandler,  (void*)semRef);
    le_thread_SetJoinable(dataThreadRef);
    le_thread_Start (dataThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);
    semRef = NULL;
    LE_DEBUG("tafMngdConnData::Init Done");
}

void tafMngdConnData::Deinit(void)
{
    if (dataThreadRef)
    {
        LE_DEBUG("Stopping dataThreadRef");
        le_thread_Cancel(dataThreadRef);
        le_thread_Join(dataThreadRef, NULL);
        dataThreadRef = NULL;
    }
    LE_DEBUG("tafMngdConnData::Deinit Done");
}

tafMngdConnData &tafMngdConnData::GetInstance()
{
    static tafMngdConnData instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for data session state change.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::SessionStateChangeHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t state,
    const taf_dcs_StateInfo_t *stateInfoPtr,
    void *contextPtr
)
{
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    uint32_t profileId;
    uint8_t phoneId;
    stateMachineEvent_t stateMachineEvt = {MCS_EVT_INIT,0};
    mcs_DataCtx_t* dataCtxPtr = NULL;
    le_result_t result;
    taf_dcs_CallEndReasonType_t callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    int32_t callEndReasonCode = -1;

    profileId = taf_dcs_GetProfileIndex(profileRef);
    result = taf_dcs_GetPhoneId(profileRef, &phoneId);

    LE_DEBUG("Data Connection State: %d, phoneid: %d, profileId: %d, PDP: %d",
             state, phoneId, profileId, stateInfoPtr->ipType);
    if(state == TAF_DCS_CONNECTED && stateInfoPtr->ipType!=TAF_DCS_PDP_IPV4)
    {
        //Only IPv4 is supported
        //TODO: Add support for IPv6
        LE_DEBUG("Returning from here as only IPv4 is supported");
        return;
    }

    if(result != LE_OK)
    {
        LE_ERROR("Can't find the phoneId for profileRef(%p)", profileRef);
        return;
    }

    LE_DEBUG ("Data Connection State: %d, phoneid: %d, profileId: %d", state, phoneId, profileId);
    dataCtxPtr = mngdConnAdmin.GetDataCtx(phoneId, profileId);
    //The connection is not created by tafMngdConnSvc, don't report the event.
    if(dataCtxPtr == NULL)
    {
        LE_DEBUG("Can't find the context for phoneId(%d), profileId(%d)", phoneId, profileId);
        return;
    }
    switch (state)
    {
        case TAF_DCS_DISCONNECTED:
            LE_DEBUG ("Data Disconnected Event called for dataID  %d", dataCtxPtr->dataId);
            stateMachineEvt.event = MCS_EVT_DATA_CONNECTION_DISCONNECTED;
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            // Get call end reason for IPv4
            result = taf_dcs_GetCallEndReason(profileRef, TAF_DCS_PDP_IPV4,
                                              &callEndReasonType, &callEndReasonCode);
            if (LE_OK != result)
            {
                LE_ERROR("Can't get the CallEndReason for profileRef(%p)", profileRef);
            }
            else
            {
                const char *CallEndReasonTypeStr4 =
                    taf::svc::datacall::tafDCSHelper::CallEndReasonTypeToString(callEndReasonType);
                const char *CallEndReasonCodeStr4 =
                                taf::svc::datacall::tafDCSHelper::CallEndReasonCodeToString(
                                                            callEndReasonType, callEndReasonCode);

                LE_INFO("IPv4 Call end reason type: %d(%s)",
                        callEndReasonType, CallEndReasonTypeStr4);
                LE_INFO("IPv4 Call end reason code: %d(%s)",
                        callEndReasonCode, CallEndReasonCodeStr4);
            }
            break;
        case TAF_DCS_CONNECTED:
            LE_DEBUG ("Data connected Event called for dataID  %d", dataCtxPtr->dataId);
            stateMachineEvt.event=MCS_EVT_DATA_CONNECTION_CONNECTED;
            stateMachineEvt.dataId = dataCtxPtr->dataId;
            break;
        default:
            return;
    }

    auto &mcsAdmin = tafMngdConnAdmin::GetInstance();
    le_event_Report(mcsAdmin.GetStateMachineEventId(), &stateMachineEvt,
                    sizeof(stateMachineEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Unregister data connection event handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::UnregisterEvents()
{
    // Iterate through the map and remove all session state handlers
    for (auto& entry : sessionStateHandlerMap_)
    {
        auto& key = entry.first;
        auto& handlerRef = entry.second;

        if (handlerRef != NULL)
        {
            taf_dcs_RemoveSessionStateHandler(handlerRef);
            LE_DEBUG("Unregistered session state handler for phoneId %d, profileId %d",
                    key.first, key.second);
        }
    }

    // Clear the map
    sessionStateHandlerMap_.clear();
    LE_DEBUG("All data event callbacks have been unregistered");
}

//--------------------------------------------------------------------------------------------------
/**
 * Register data connection event handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::RegisterEvents()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;

    taf_dcs_ConnectService();

    for(int phoneId = MIN_PHONE_ID; phoneId <= MAX_PHONE_ID; phoneId++)
    {
        result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
        if(result != LE_OK)
        {
            LE_DEBUG("Getting profile list for phone id %d failed", phoneId);
            continue;
        }

        for (size_t i = 0; i < listSize; i++)
        {
            const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
            profileRef = taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);

            if (profileRef == NULL)
            {
                LE_ERROR("Invalid profile %p\n", profileRef);
                continue;
            }

            taf_dcs_SessionStateHandlerRef_t handlerRef =
                        taf_dcs_AddSessionStateHandler(profileRef, SessionStateChangeHandler, NULL);

            if (handlerRef == NULL)
            {
                LE_ERROR("Adding session state handler failed for phoneId %d, profileId %d",
                         phoneId, profileInfoPtr->index);
                continue;
            }

            // Store the handler in the map with (phoneId, profileId) as key
            auto key = std::make_pair(phoneId, profileInfoPtr->index);
            sessionStateHandlerMap_[key] = handlerRef;
            LE_INFO("Registered session state handler for phoneId %d, profileId %d",
                    phoneId, profileInfoPtr->index);
        }
    }

    LE_INFO ("Data Event callback is set");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Startdata(uint8_t phoneId, uint32_t profileId)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    uint8_t defaultPhoneId = 0;
    uint32_t defaultProfileId = 0;

    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defaultPhoneId, &defaultProfileId);

    if(result == LE_OK)
    {
        if(defaultProfileId != profileId)
        {
            LE_INFO("Profile %d is not a default profile", profileId);
        }
        if (defaultPhoneId != phoneId)
        {
            LE_WARN("Phone ID %d is not default phone ID", phoneId);
            return LE_UNSUPPORTED;
        }
    }
    else
    {
        LE_ERROR("Getting default profile failed");
        return LE_FAULT;
    }

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);
    result = taf_dcs_StartSession(profileRef);

    LE_INFO("Startdata: result =%d " ,result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with timeout.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Startdata(uint8_t phoneId, uint32_t profileId, uint8_t timeout)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    uint8_t defaultPhoneId = 0;
    uint32_t defaultProfileId = 0;

    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defaultPhoneId, &defaultProfileId);

    if(result == LE_OK)
    {
        if(defaultProfileId != profileId)
        {
            LE_INFO("Profile %d is not a default profile", profileId);
        }
        if (defaultPhoneId != phoneId)
        {
            LE_WARN("Phone ID %d is not default phone ID", phoneId);
            return LE_UNSUPPORTED;
        }
    }
    else
    {
        LE_ERROR("Getting default profile failed");
        return LE_FAULT;
    }

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);

    if(profileRef == NULL)
    {
        LE_ERROR("profileRef Not found");
        return LE_FAULT;
    }

    AsyncAPIPromise = std::promise<le_result_t>();
    le_event_QueueFunctionToThread(dataThreadRef,(le_event_DeferredFunc_t)StartDataAsync,
                                   profileRef, NULL);

     // blocking here to get response
    std::chrono::system_clock::time_point timeoutsec
        = std::chrono::system_clock::now() + std::chrono::seconds(timeout);
    std::future<le_result_t> futResult = AsyncAPIPromise.get_future();
    // Set waiting for promise
    bWaitingForPromise.store(true);
    std::future_status status = futResult.wait_until(timeoutsec);
    if (status == std::future_status::ready) {
        // Result is available
        // getting and printing the result
        if (futResult.valid()) {
            LE_DEBUG("futResult.get_future");
            result = futResult.get();
        }
        else {
            LE_ERROR("Invalid state %d", result);
            result = LE_FAULT;
        }
    } else if (status == std::future_status::timeout) {
        // Timeout occurred
        LE_ERROR("Timeout occurred while starting data Result: %d", result);
        result = LE_TIMEOUT;
    }
    // Reset waiting for promise
    bWaitingForPromise.store(false);
    LE_INFO("Startdata: result =%d " ,result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Datasession thread destructor.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::DataThreadDestructor(void *contextPtr)
{
    LE_DEBUG("Disconnect from DCS");
    taf_dcs_DisconnectService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Datasession thread handler.
 */
//--------------------------------------------------------------------------------------------------
void *tafMngdConnData::DataThreadHandler(void *contextPtr)
{
    LE_DEBUG("DataThreadHandler Entry");

    le_thread_AddDestructor(DataThreadDestructor, NULL);

    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    taf_dcs_ConnectService();

    le_sem_Post(semRef);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session asynchronously.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::StartDataAsync(void *contextPtr)
{
    LE_DEBUG("StartDataAsync session");
    taf_dcs_ProfileRef_t profileRef = (taf_dcs_ProfileRef_t)contextPtr;
    //Call async start session API in DataSvc
    taf_dcs_StartSessionAsync(profileRef, StartSessionAsyncHandlerFunc, NULL);
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session asynchronously.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::StopDataAsync(void *contextPtr)
{
    LE_DEBUG("StopDataAsync session");
    taf_dcs_ProfileRef_t profileRef = (taf_dcs_ProfileRef_t)contextPtr;
    //Call async start session API in DataSvc
    taf_dcs_StopSessionAsync(profileRef, StopSessionAsyncHandlerFunc, NULL);
    return;
}

void tafMngdConnData::StartSessionAsyncHandlerFunc(taf_dcs_ProfileRef_t profileRef,
                                            le_result_t result,
                                            void* contextPtr)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_DEBUG("Handler for Asynchornous session -- Begin");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_DEBUG("Handler for Asynchornous session -- End");
    if (bWaitingForPromise.exchange(false)) {
        LE_DEBUG("AsyncAPIPromise.set_value");
        AsyncAPIPromise.set_value(result);
    }
    else {
        LE_DEBUG("Promise already satisfied");
    }
}

void tafMngdConnData::StopSessionAsyncHandlerFunc(taf_dcs_ProfileRef_t profileRef,
                                            le_result_t result,
                                            void* contextPtr)
{
    int32_t profileId = taf_dcs_GetProfileIndex(profileRef);
    LE_DEBUG("Handler for Asynchornous session -- Begin");
    LE_INFO("profileId= %d, result: %d", profileId, result);
    LE_DEBUG("Handler for Asynchornous session -- End");
    if (bWaitingForPromise.exchange(false))
    {
        LE_DEBUG("AsyncAPIPromise.set_value");
        AsyncAPIPromise.set_value(result);
    }
    else
    {
        LE_DEBUG("Promise already satisfied");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Stopdata(uint8_t phoneId, uint32_t profileId)
{
    taf_dcs_ProfileRef_t profileRef = NULL;

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);

    return taf_dcs_StopSession(profileRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session with timeout
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Stopdata(uint8_t phoneId, uint32_t profileId, uint8_t timeout)
{
    le_result_t result = LE_OK;
    taf_dcs_ProfileRef_t profileRef = NULL;

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);

    AsyncAPIPromise = std::promise<le_result_t>();
    le_event_QueueFunctionToThread(dataThreadRef,(le_event_DeferredFunc_t)StopDataAsync,
                                   profileRef, NULL);

     // blocking here to get response
    std::chrono::system_clock::time_point timeoutsec
        = std::chrono::system_clock::now() + std::chrono::seconds(timeout);
    std::future<le_result_t> futResult = AsyncAPIPromise.get_future();
    // Set waiting for promise
    bWaitingForPromise.store(true);
    std::future_status status = futResult.wait_until(timeoutsec);
    if (status == std::future_status::ready) {
        // Result is available
        // getting and printing the result
        if (futResult.valid()) {
            LE_DEBUG("futResult.get_future");
            result = futResult.get();
        }
        else {
            LE_ERROR("Invalid state %d", result);
            result = LE_FAULT;
        }
    } else if (status == std::future_status::timeout) {
        // Timeout occurred
        LE_ERROR("Timeout occurred while stoping data Result: %d", result);
        result = LE_TIMEOUT;
    }
    // Reset waiting for promise
    bWaitingForPromise.store(false);
    LE_INFO("StopData: result =%d " ,result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connection information for specified connection context.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::GetConnectionInfo(mcs_DataCtx_t* dataCtxPtr)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    bool isIpv4 = false, isIpv6 = false;

    TAF_ERROR_IF_RET_VAL(dataCtxPtr == NULL, LE_BAD_PARAMETER, "Null ptr(dataCtxPtr)");
    profileRef = taf_dcs_GetProfileEx (dataCtxPtr->phoneId, dataCtxPtr->profileNumber);

    result = taf_dcs_GetInterfaceName(profileRef, dataCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Getting interface name failed for dataID %d",dataCtxPtr->dataId);
        return result;
    }

    isIpv4 = taf_dcs_IsIPv4(profileRef);
    isIpv6 = taf_dcs_IsIPv6(profileRef);

    if(isIpv4 && !isIpv6)         //Ipv4 only
    {
        dataCtxPtr->ipType = TAF_DCS_PDP_IPV4;
        result=taf_dcs_GetIPv4Address(profileRef, dataCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV4 address failed for dataID %d ",dataCtxPtr->dataId);
            return result;
        }
    }
    else if(!isIpv4 && isIpv6)    //Ipv6 only
    {
        dataCtxPtr->ipType = TAF_DCS_PDP_IPV6;
        result=taf_dcs_GetIPv6Address(profileRef, dataCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV6 address failed for dataID %d ",dataCtxPtr->dataId);
            return result;
        }
    }
    else if(isIpv4 && isIpv6)     //Ipv4v6
    {
        dataCtxPtr->ipType = TAF_DCS_PDP_IPV4V6;
        result=taf_dcs_GetIPv4Address(profileRef, dataCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV4 address failed for dataID %d ",dataCtxPtr->dataId);
            return result;
        }

        result=taf_dcs_GetIPv6Address(profileRef, dataCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV6 address failed for dataID %d ",dataCtxPtr->dataId);
            return result;
        }
    }
    else
    {
        dataCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
        result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection information for the specified list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::GetAllConnectionInfo(profileInfo_t *profileNumberList, int listSize)
{
    bool isIpv4 = false, isIpv6 = false;
    taf_dcs_ProfileRef_t profileRef = NULL;
    mcs_DataCtx_t* dataCtxPtr = NULL;
    le_result_t result;

    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();

    TAF_ERROR_IF_RET_VAL(profileNumberList == NULL, LE_BAD_PARAMETER,
                        "Null ptr(profileNumberList)");

    for(int i = 0; i < listSize; i++)
    {
        dataCtxPtr = mngdConnAdmin.GetDataCtx(profileNumberList[i].phoneId,
                                              profileNumberList[i].profileNumber);

        if(dataCtxPtr == NULL)
            continue;

        if(dataCtxPtr->dataState != TAF_MNGDCONN_DATA_CONNECTED)
            continue;

        profileRef = taf_dcs_GetProfileEx (dataCtxPtr->phoneId, dataCtxPtr->profileNumber);

        result = taf_dcs_GetInterfaceName(profileRef, dataCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
        if(result != LE_OK)
        {
            LE_ERROR("Getting interface name failed");
            continue;
        }

        isIpv4 = taf_dcs_IsIPv4(profileRef);
        isIpv6 = taf_dcs_IsIPv6(profileRef);

        if(isIpv4 && !isIpv6)
        {
            dataCtxPtr->ipType = TAF_DCS_PDP_IPV4;
            result=taf_dcs_GetIPv4Address(profileRef, dataCtxPtr->ipv4Addr,
                                          TAF_DCS_IPV4_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV4 address failed");
                continue;
            }
        }
        else if(!isIpv4 && isIpv6)
        {
            dataCtxPtr->ipType = TAF_DCS_PDP_IPV6;
            result=taf_dcs_GetIPv6Address(profileRef, dataCtxPtr->ipv6Addr,
                                          TAF_DCS_IPV6_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV6 address failed");
                continue;
            }
        }
        else if(isIpv4 && isIpv6)
        {
            dataCtxPtr->ipType = TAF_DCS_PDP_IPV4V6;
            result=taf_dcs_GetIPv4Address(profileRef, dataCtxPtr->ipv4Addr,
                                          TAF_DCS_IPV4_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV4 address failed");
                continue;
            }

            result=taf_dcs_GetIPv6Address(profileRef, dataCtxPtr->ipv6Addr,
                                          TAF_DCS_IPV6_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV6 address failed");
                continue;
            }
        }
        else
        {
            dataCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
        }
    }

    if(listSize <= 0)
    {
        LE_ERROR("No context");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets interface.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::SetInterface
(
    uint8_t phoneId,
    uint32_t profileId,
    const char* namePtr
)
{
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(phoneId, profileId);
    return taf_dcs_SetInterface(profileRef, namePtr);
}