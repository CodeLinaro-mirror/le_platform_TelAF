/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   dcsAdaptor.cpp
 * @brief  Source file for TelAF Data Call Service adpator.
 */

#include "dcsAdaptor.hpp"
#include "iostream"

DcsAdaptor::DcsAdaptor()
{
    LE_INFO("DcsAdaptor constructor called");
}

DcsAdaptor::~DcsAdaptor()
{
    LE_INFO("DcsAdaptor destructor called");

    // Clean up any resources here, such as:
    if (SessionStateChangeCbkCtxPtr)
        delete SessionStateChangeCbkCtxPtr;
}

/**
 * The thread context in which  DCS service events will be received.
 */
void * DcsAdaptor::TelafDcsTask(void *arg)
 {
    LE_INFO("TelafDcsTask start");

    // Check if we have the DCS adaptor object pointer
    DcsAdaptor* objPtr = (DcsAdaptor*)(arg);
    if (NULL == objPtr)
    {
        LE_ERROR("objPtr is NULL");
        // Set the promise to LE_FAULT
        objPtr->DcsThreadPromise.set_value(LE_FAULT);
        return NULL;
    }

    // Initialize Legato thread data
    le_thread_InitLegatoThreadData("TelafDcsTask-thread");

    // Connect to DCS service.
    taf_dcs_ConnectService();

    // Create command events and register their handlers in this thread's context
    objPtr->AddCmdEventsAndHandler();

    // Set the promise to LE_OK to indicate successful connect.
    objPtr->DcsThreadPromise.set_value(LE_OK);

    // Start the event loop
    LE_INFO("TelafDcsTask fwk run event loop");
    le_event_RunLoop();

    return NULL;
}

/**
 * Connect to the TelAF Data Call Service
 */
le_result_t DcsAdaptor::Connect()
{
    int ret;
    le_result_t result = LE_OK;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    LE_INFO("libdcs Connect");

    DcsThreadPromise = std::promise<le_result_t>();

    // Run TelafTask in a separate thread.
    ret = pthread_create(&DcsTId, NULL, &DcsAdaptor::TelafDcsTask, this);
    if (ret < 0)
    {
        std::cout << "pthread_create failed" << ret << std::endl;
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = DcsThreadPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("libdcs Connect waiting promise timeout");
        return LE_TIMEOUT;
    }
    result = futResult.get();

    return result;
}

//Create the supported command events and register their handlers
void DcsAdaptor::AddCmdEventsAndHandler()
{
    // GetDefaultProfileId API
    GetDefaultProfileIdEventId = le_event_CreateId("GetDefaultPhoneIdProfileIdEvent",
                                                   sizeof(GetDefaultProfileIdCmdEventParm_t));
    le_event_AddHandler("GetDefaultPhoneIdProfileIdEventHandler",
                            GetDefaultProfileIdEventId,
                            DcsAdaptor::OnGetDefaultProfileIdEventHandler);

    // StartSession API
    StartSessionEventId = le_event_CreateId("StartSessionEvent",
                                                sizeof(StartSessionCmdEventParm_t));
    le_event_AddHandler("StartSessionEventHandler",
                            StartSessionEventId,
                            DcsAdaptor::OnStartSessionEventIdEventHandler);

    // StopSession API
    StopSessionEventId = le_event_CreateId("StopSessionEvent",
                                                sizeof(StopSessionCmdEventParm_t));
    le_event_AddHandler("StopSessionEventHandler",
                            StopSessionEventId,
                            DcsAdaptor::OnStopSessionEventHandler);

    // GetInterfaceName API
    GetInterfaceNameEventId = le_event_CreateId("GetInterfaceNameEvent",
                                                sizeof(GetInterfaceNameCmdEventParm_t));
    le_event_AddHandler("GetInterfaceNameEventHandler",
                        GetInterfaceNameEventId,
                        DcsAdaptor::OnGetInterfaceNameEventHandler);

    // GetIPv4Addr API
    GetIPv4AddressEventId = le_event_CreateId("GetIPv4AddrEvent",
                                           sizeof(GetIPv4AddressCmdEventParm_t));
    le_event_AddHandler("GetIPv4AddrEventHandler",
                        GetIPv4AddressEventId,
                        DcsAdaptor::OnGetIPv4AddressEventHandler);

    // GetCallEndReason API
    GetCallEndReasonEventId = le_event_CreateId("GetCallEndReasonEvent",
                                                sizeof(GetCallEndReasonCmdEventParm_t));
    le_event_AddHandler("GetCallEndReasonEventHandler",
                        GetCallEndReasonEventId,
                        DcsAdaptor::OnGetCallEndReasonEventHandler);

    // AddSessionStateChangeCbk API
    AddSessionStateHandlerEventId = le_event_CreateId("AddSessionStateHandlerEvent",
                                                sizeof(AddSessionStateChangeCbkCmdEventParm_t));
    le_event_AddHandler("AddSessionStateChangeCbkEventHandler",
                        AddSessionStateHandlerEventId,
                        DcsAdaptor::OnAddSessionStateChangeCbkEventHandler);
    // Allocate memory for the session state context
    SessionStateChangeCbkCtxPtr = new (std::nothrow) AddSessionStateChangeCbkCtx_t;
    if (nullptr == SessionStateChangeCbkCtxPtr)
    {
        // The AddSessionStateChangeCbk function cannot be supported
        LE_ERROR("SessionStateChangeCbkCtxPtr is NULL. AddSessionStateChangeCbk cannot work.");
    }
}

// Handler for GetDefaultPhoneIdProfileIdEventId
void DcsAdaptor::OnGetDefaultProfileIdEventHandler(void *reportPtr)
{
    LE_INFO("%s", __func__);
    GetDefaultProfileIdCmdEventParm_t *evtPtr =
                                static_cast<GetDefaultProfileIdCmdEventParm_t *>(reportPtr);
    GetDefaultProfileIdCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr== NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Get the default profile ID and phone id
    le_result_t result;
    uint8_t defaultPhoneId = 0;
    uint32_t defaultProfileId = 0;
    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defaultPhoneId, &defaultProfileId);
    LE_INFO("taf_dcs_GetDefaultPhoneIdAndProfileId result: %d(%s)", result, LE_RESULT_TXT(result));

    // Set the result and profile ID and set the promise value.
    cmdResult.result = result;
    cmdResult.profileId = defaultProfileId;
    evtPtr->objPtr->GetDefaultProfileIdPromise.set_value(cmdResult);
}

// The GetDefaultProfileId API
le_result_t DcsAdaptor::GetDefaultProfileId(uint32_t &profileIdRef)
{
    LE_INFO("%s", __func__);
    GetDefaultProfileIdCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    GetDefaultProfileIdCmdResult_t cmdResult;

    LE_INFO("libdcs GetDefaultProfileId called\n");

    evt.objPtr = this;

    GetDefaultProfileIdPromise = std::promise<GetDefaultProfileIdCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(GetDefaultProfileIdEventId, &evt, sizeof(GetDefaultProfileIdCmdEventParm_t));

    // blocking here to get response
    std::future<GetDefaultProfileIdCmdResult_t> futResult = GetDefaultProfileIdPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetDefaultProfileId waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();
    profileIdRef = cmdResult.profileId;

    return cmdResult.result;
}

// Handler for StartSessionEventId
void DcsAdaptor::OnStartSessionEventIdEventHandler (void *reportPtr)
{
    LE_INFO("%s", __func__);
    StartSessionCmdEventParm_t *evtPtr = static_cast<StartSessionCmdEventParm_t *>(reportPtr);
    StartSessionCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Start session with provided profile ID
    le_result_t result;
    uint32_t profileId = evtPtr->profileId;
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
    }
    else
    {
        result = taf_dcs_StartSession(profileRef);
        LE_INFO("taf_dcs_StartSession result: %d(%s)", result, LE_RESULT_TXT(result));
        cmdResult.result = result;
    }
    evtPtr->objPtr->StartSessionPromise.set_value(cmdResult);
}

// The StartSession API
le_result_t DcsAdaptor::StartSession (uint32_t profileId)
{
    LE_INFO("%s", __func__);
    StartSessionCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    StartSessionCmdResult_t cmdResult;

    LE_INFO("libdcs StartSession called\n");

    evt.objPtr = this;
    evt.profileId = profileId;

    StartSessionPromise = std::promise<StartSessionCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(StartSessionEventId, &evt, sizeof(StartSessionCmdEventParm_t));

    // blocking here to get response
    std::future<StartSessionCmdResult_t> futResult = StartSessionPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("StartSession waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();

    return cmdResult.result;
}

// Handler for StopSessionEventId
void DcsAdaptor::OnStopSessionEventHandler (void *reportPtr)
{
    LE_INFO("%s", __func__);
    StopSessionCmdEventParm_t *evtPtr = static_cast<StopSessionCmdEventParm_t *>(reportPtr);
    StopSessionCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Stop session with provided profile ID
    le_result_t result;
    uint32_t profileId = evtPtr->profileId;
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
    }
    else
    {
        result = taf_dcs_StopSession(profileRef);
        LE_INFO("taf_dcs_StopSession result: %d(%s)", result, LE_RESULT_TXT(result));
        cmdResult.result = result;
    }
    evtPtr->objPtr->StopSessionPromise.set_value(cmdResult);
}

// The StopSession API
le_result_t DcsAdaptor::StopSession (uint32_t profileId)
{
    LE_INFO("%s", __func__);
    StopSessionCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    StopSessionCmdResult_t cmdResult;

    LE_INFO("libdcs StopSession called\n");

    evt.objPtr = this;
    evt.profileId = profileId;

    StopSessionPromise = std::promise<StopSessionCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(StopSessionEventId, &evt, sizeof(StopSessionCmdEventParm_t));

    // blocking here to get response
    std::future<StopSessionCmdResult_t> futResult = StopSessionPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("StopSession waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();

    return cmdResult.result;
}

// Handler for GetIntfNameEventId
void DcsAdaptor::OnGetInterfaceNameEventHandler (void *reportPtr)
{
    LE_INFO("%s", __func__);
    GetInterfaceNameCmdEventParm_t *evtPtr = static_cast<GetInterfaceNameCmdEventParm_t *>(reportPtr);
    GetInterfaceNameCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Get interface name with provided profile ID
    le_result_t result;
    uint32_t profileId = evtPtr->profileId;
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
    }
    else
    {
        char intfNameStr[TAF_DCS_NAME_MAX_LEN]={0};

        result = taf_dcs_GetInterfaceName(profileRef, intfNameStr, TAF_DCS_NAME_MAX_LEN);
        LE_INFO("taf_dcs_GetInterfaceName result: %d(%s)", result, LE_RESULT_TXT(result));
        cmdResult.result = result;
        if (LE_OK == result)
        {
            cmdResult.IntfName.clear();
            cmdResult.IntfName = intfNameStr;
        }
    }
    evtPtr->objPtr->GetInterfaceNamePromise.set_value(cmdResult);
}

// The GetInterfaceName API
le_result_t DcsAdaptor::GetInterfaceName(uint32_t profileId, std::string &IntfName)
{
    LE_INFO("%s", __func__);
    GetInterfaceNameCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    GetInterfaceNameCmdResult_t cmdResult;

    LE_INFO("libdcs GetInterfaceName called\n");

    evt.objPtr = this;
    evt.profileId = profileId;

    GetInterfaceNamePromise = std::promise<GetInterfaceNameCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(GetInterfaceNameEventId, &evt, sizeof(GetInterfaceNameCmdEventParm_t));

    // blocking here to get response
    std::future<GetInterfaceNameCmdResult_t> futResult = GetInterfaceNamePromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetInterfaceName waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();

    // Copy the interface name to the provided buffer
    if (LE_OK==cmdResult.result)
    {
        IntfName = cmdResult.IntfName;
    }

    return cmdResult.result;
}

// Handler for GetIPv4AddrEventId
void DcsAdaptor::OnGetIPv4AddressEventHandler (void *reportPtr)
{
    LE_INFO("%s", __func__);
    GetIPv4AddressCmdEventParm_t *evtPtr = static_cast<GetIPv4AddressCmdEventParm_t *>(reportPtr);
    GetIPv4AddressCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Get IPv4 address with provided profile ID
    le_result_t result;
    uint32_t profileId = evtPtr->profileId;
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
    }
    else
    {
        char ipv4AddrStr[TAF_DCS_IPV4_ADDR_MAX_LEN] = {0};
        result = taf_dcs_GetIPv4Address(profileRef, ipv4AddrStr, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_INFO("taf_dcs_GetIPv4Address result: %d(%s)", result, LE_RESULT_TXT(result));
        cmdResult.result = result;
        if (LE_OK == result)
        {
            cmdResult.IPv4Address.clear();
            cmdResult.IPv4Address = ipv4AddrStr;
        }
    }

    evtPtr->objPtr->GetIPv4AddressPromise.set_value(cmdResult);
}

// The GetIPv4Address API
le_result_t DcsAdaptor::GetIPv4Address(uint32_t profileId, std::string &IPv4Addr)
{
    LE_INFO("%s", __func__);
    GetIPv4AddressCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    GetIPv4AddressCmdResult_t cmdResult;

    LE_INFO("libdcs GetIPv4Address called\n");

    evt.objPtr = this;
    evt.profileId = profileId;

    GetIPv4AddressPromise = std::promise<GetIPv4AddressCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(GetIPv4AddressEventId, &evt, sizeof(GetIPv4AddressCmdEventParm_t));

    // blocking here to get response
    std::future<GetIPv4AddressCmdResult_t> futResult = GetIPv4AddressPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetIPv4Address waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();

    // Copy the IPv4 address to the provided buffer
    if (LE_OK == cmdResult.result)
    {
        IPv4Addr = cmdResult.IPv4Address;
    }

    return cmdResult.result;
}

// Handler for GetCallEndReasonEventId
void DcsAdaptor::OnGetCallEndReasonEventHandler (void *reportPtr)
{
    LE_INFO("%s", __func__);
    GetCallEndReasonCmdEventParm_t *evtPtr = static_cast<GetCallEndReasonCmdEventParm_t *>(reportPtr);
    GetCallEndReasonCmdResult_t cmdResult;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Get call end reason with provided profile ID and pdp
    le_result_t result;
    uint32_t profileId = evtPtr->profileId;
    taf_dcs_Pdp_t pdp = evtPtr->pdp;
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
    }
    else
    {
        taf_dcs_CallEndReasonType_t callEndReasonType;
        int32_t callEndReason;
        result = taf_dcs_GetCallEndReason(profileRef, pdp, &callEndReasonType, &callEndReason);
        LE_INFO("taf_dcs_GetCallEndReason result: %d(%s)", result, LE_RESULT_TXT(result));
        cmdResult.result = result;
        if (LE_OK == result)
        {
            cmdResult.CallEndReasonType = callEndReasonType;
            cmdResult.CallEndReason = callEndReason;
        }
    }
    evtPtr->objPtr->GetCallEndReasonPromise.set_value(cmdResult);
}

// The GetCallEndReason API
le_result_t DcsAdaptor::GetCallEndReason(uint32_t profileId, taf_dcs_Pdp_t pdp,
                                         taf_dcs_CallEndReasonType_t &typeRef, int32_t &reasonRef)
{
    LE_INFO("%s", __func__);
    GetCallEndReasonCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    GetCallEndReasonCmdResult_t cmdResult;

    LE_INFO("libdcs GetCallEndReason called\n");

    evt.objPtr = this;
    evt.profileId = profileId;
    evt.pdp = pdp;

    GetCallEndReasonPromise = std::promise<GetCallEndReasonCmdResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(GetCallEndReasonEventId, &evt, sizeof(GetCallEndReasonCmdEventParm_t));

    // blocking here to get response
    std::future<GetCallEndReasonCmdResult_t> futResult = GetCallEndReasonPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetCallEndReason waiting promise timeout");
        return LE_TIMEOUT;
    }

    cmdResult = futResult.get();

    // Copy the call end reason to the provided buffers
    if (LE_OK == cmdResult.result)
    {
        typeRef   = cmdResult.CallEndReasonType;
        reasonRef = cmdResult.CallEndReason;
    }

    return cmdResult.result;
}

// The taf_dcs_AddSessionStateHandler callback handler
void DcsAdaptor::SessionStateChangeHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t state,
    const taf_dcs_StateInfo_t *stateInfoPtr,
    void *contextPtr
)
{
    LE_INFO("%s", __func__);
    AddSessionStateChangeCbkCtx_t *eventCtx = NULL;
    uint32_t profileId = 0;
    le_result_t result;

    if  (NULL == contextPtr)
    {
        LE_WARN ("contextPtr is NULL");
        return;
    }
    eventCtx = static_cast<AddSessionStateChangeCbkCtx_t *>(contextPtr);
    if (NULL == eventCtx->callback)
    {
        LE_WARN ("eventCtx->parm.callback is NULL");
        return;
    }

    // Get the associated profile Id
    result = taf_dcs_GetProfileId(profileRef, &profileId);
    if (LE_OK != result)
    {
        LE_WARN("taf_dcs_GetProfileId failed: %d(%s)", result, LE_RESULT_TXT(result));
    }

    // Call the callback along with the app provided context
    eventCtx->callback(profileId, state, stateInfoPtr->ipType, eventCtx->ctx);
}

// Handler for AddSessionStateHandlerEventId
void DcsAdaptor::OnAddSessionStateChangeCbkEventHandler(void *reportPtr)
{
    LE_INFO("%s", __func__);
    AddSessionStateChangeCbkCmdEventParm_t *evtPtr =
                                static_cast<AddSessionStateChangeCbkCmdEventParm_t *>(reportPtr);
    AddSessionStateChangeCbkResult_t cmdResult;
    taf_dcs_SessionStateHandlerRef_t ref    = NULL;
    AddSessionStateChangeCbkCtx_t *eventCtx = NULL;
    if (evtPtr == NULL)
    {
        LE_ERROR("evtPtr is NULL");
        return;
    }
    if (evtPtr->objPtr == NULL)
    {
        LE_ERROR("evtPtr->objPtr is NULL");
        return;
    }

    // Check if memory for context has been allocated
    LE_INFO("SessionStateChangeCbkCtxPtr: %p", evtPtr->objPtr->SessionStateChangeCbkCtxPtr);
    if (nullptr == evtPtr->objPtr->SessionStateChangeCbkCtxPtr)
    {
        LE_ERROR("OnAddRegStateChangeNotifyEventHandler SessionStateChangeCbkCtxPtr == NULL");
        cmdResult.result = LE_FAULT;
        evtPtr->objPtr->AddSessionStateChangeCbkPromise.set_value(cmdResult);
        return;
    }

    // Pass app callback and app context to taf_dcs_AddSessionStateHandler
    eventCtx            = evtPtr->objPtr->SessionStateChangeCbkCtxPtr;
    eventCtx->callback  = evtPtr->callback;
    eventCtx->ctx       = evtPtr->ctx;

    // Get the profile reference
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfileEx(DCS_DEFAULT_PHONE_ID, evtPtr->profileId);
    if (NULL == profileRef)
    {
        LE_WARN("Unable to get profile reference");
        cmdResult.result = LE_FAULT;
        evtPtr->objPtr->AddSessionStateChangeCbkPromise.set_value(cmdResult);
        return;
    }
    else
    {
        // Register for state change events
        ref = taf_dcs_AddSessionStateHandler(profileRef, &DcsAdaptor::SessionStateChangeHandler,
                                                                                (void *)eventCtx);
        if (NULL == ref)
        {
            LE_WARN("Unable to register taf_dcs_AddSessionStateHandler");
            cmdResult.result = LE_FAULT;
            evtPtr->objPtr->AddSessionStateChangeCbkPromise.set_value(cmdResult);
            return;
        }
    }

    // Update the result
    cmdResult.result = LE_OK;
    evtPtr->objPtr->AddSessionStateChangeCbkPromise.set_value(cmdResult);
}

// The AddSessionStateChangeCbk API
le_result_t DcsAdaptor::AddSessionStateChangeCbk
(
    uint32_t profileId,
    SessionStateChangeCb callback,
    void *ctx
)
{
    LE_INFO("%s", __func__);
    AddSessionStateChangeCbkCmdEventParm_t evt;
    std::chrono::seconds span(DCS_API_TIMEOUT);

    AddSessionStateChangeCbkResult_t sResult;

    LE_INFO("libdcs AddSessionStateChangeCbk called");

    evt.objPtr    = this;
    evt.profileId = profileId;
    evt.callback  = callback;
    evt.ctx       = ctx;

    AddSessionStateChangeCbkPromise = std::promise<AddSessionStateChangeCbkResult_t>();

    // Send to TelAF DCS thread handler
    le_event_Report(AddSessionStateHandlerEventId, &evt,
                                            sizeof(AddSessionStateChangeCbkCmdEventParm_t));

    // blocking here to get response
    std::future<AddSessionStateChangeCbkResult_t> futResult =
                                                AddSessionStateChangeCbkPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("AddSessionStateChangeCbk waiting promise timeout");
        return LE_TIMEOUT;
    }

    sResult = futResult.get();
    sResult.result;

    return sResult.result;
}