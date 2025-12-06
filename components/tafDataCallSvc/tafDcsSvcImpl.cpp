/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsImpl.cpp
 * @brief TelAF Data Call Service implementation
 *
 */

#include "tafDcs.hpp"
#include "tafDcsProfile.hpp"
#include "tafDcsUtils.hpp"
#include "tafSvcIF.hpp"

using namespace taf::svc::datacall;

TafDcsSvc &TafDcsSvc::GetInstance()
{
    static TafDcsSvc instance;
    return instance;
}

void TafDcsSvc::Init()
{
    // Initialize internal threads and events.
    startThreads();

    // Create internal events
    createDCSEvents();

    // Register clients connect/disconnect handlers
    registerClientsConnectDisconnectHandlers();

    // Initialize the PA
    le_result_t result;
    taf::pa::data::InitState_e paInitState;


    result = taf::pa::data::Init(paInitState);
    if (LE_OK == result)
    {
        // All good
        LE_INFO("PA is initialized.");
        paInitState_ = paInitState;
    }
    else if (LE_NOT_IMPLEMENTED == result)
    {
        paInitState_ = taf::pa::data::InitState_e::INIT_FAILED;
        LE_ERROR("PA is not implemented.");
    }
    else if (LE_FAULT == result)
    {
        paInitState_ = taf::pa::data::InitState_e::INIT_FAILED;
        LE_ERROR("PA initialization failed.");
    }
    else if (LE_UNAVAILABLE == result)
    {
        LE_WARN("PA is partially initialized.");
        LE_INFO("PA Initialization state: %d", static_cast<int>(paInitState));
        paInitState_ = paInitState;
    }

    // TODO: If partially initialized, use taf::pa::data::GetInitState() to check if the PA
    // is available.

    // Add power state change handler
    powerStateChangeHandlerRef_= taf_pm_AddStateChangeHandler(powerStateChangeHandler, NULL);
    if (nullptr == powerStateChangeHandlerRef_)
    {
        LE_ERROR("Unable to register power state change handler.");
    }
    else
    {
        LE_INFO("Power state change handler registered.");
    }
}

void TafDcsSvc::powerStateChangeHandler(taf_pm_State_t state, void *contextPtr)
{
    LE_UNUSED(contextPtr);
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        taf::pa::data::RegisterSDKCallbacks();
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        taf::pa::data::DeregisterSDKCallbacks();
    }
    else
    {
        LE_WARN("Unknown power state: %d", TO_INT(state));
    }
}

void TafDcsSvc::Deinit()
{
    stopThreads();
    unregisterClientsConnectDisconnectHandlers();
}

taf::pa::data::InitState_e TafDcsSvc::GetInitState() const
{
    return paInitState_;
}

// Return the updateProfileEvtId_
le_event_Id_t TafDcsSvc::GetUpdateProfileEvtId() const
{
    return updateProfileEvtId_;
}

// Return the clientConnectedEvtId_
le_event_Id_t TafDcsSvc::GetClientsDisconnectedEvtId() const
{
    return clientDisconnectedEvtId_;
}

// Return the sessionStartEvtId_
le_event_Id_t TafDcsSvc::GetSessionStartEvtId() const
{
    return sessionStartEvtId_;
}

// Return the sessionStopEvtId_
le_event_Id_t TafDcsSvc::GetSessionStopEvtId() const
{
    return sessionStopEvtId_;
}

// Return the paSessionStateChangeEvtId_
le_event_Id_t TafDcsSvc::GetPaSessionStateChangeEvtId() const
{
    return paSessionStateChangeEvtId_;
}

// Return paRoamingChangeEvtId_
le_event_Id_t TafDcsSvc::GetPaRoamingStatusChangeEvtId() const
{
    return paRoamingChangeEvtId_;
}

// Return paThrottledAPNsEvtId_
le_event_Id_t TafDcsSvc::GetPaThrottledAPNsEvtId() const
{
    return paThrottledAPNsEvtId_;
}

// Return paQosTftEvtId_
le_event_Id_t TafDcsSvc::GetPaQosTftEvtId() const
{
    return paQosTftEvtId_;
}

// Return paHwAccelerationChangeEvtId_
le_event_Id_t TafDcsSvc::GetPaHwAccelerationEvtId() const
{
    return paHwAccelerationChangeEvtId_;
}

// Return the startSessionAsyncRspEvtId_
le_event_Id_t TafDcsSvc::GetStartSessionAsyncRspEvtId() const
{
    return startSessionAsyncRspEvtId_;
}

// Return stopSessionAsyncRspEvtId_
le_event_Id_t TafDcsSvc::GetStopSessionAsyncRspEvtId() const
{
    return stopSessionAsyncRspEvtId_;
}

le_thread_Ref_t TafDcsSvc::GetEventsThreadRef() const
{
    return tafDcsEventsThreadRef_;
}

// The event handler for the internal events thread.
static void *tafDcsEventsThreadHandler
(
    void *context
)
{
    LE_INFO("tafDcsEventsThread_ started. Run event loop.");
    LE_UNUSED(context);
    // Run the event loop
    le_event_RunLoop();
    return NULL;
}

// These are internal DCS and PA events
void TafDcsSvc::createDCSEvents()
{
    // Event to manage profile updates
    updateProfileEvtId_      = le_event_CreateId("updateProfileEvtId_",
                                                            sizeof(TafDcsUpdateProfileEvent_t));
    // Event to handle client disconnects
    clientDisconnectedEvtId_ = le_event_CreateId("clientDisconnectedEvtId_",
                                                        sizeof(TafDcsClientDisconnectedEvent_t));

    // Event to start data session
    sessionStartEvtId_ = le_event_CreateId("sessionStartEvtId_", sizeof(TafDcsSessionStartEvent_t));

    // Event to stop data session
    sessionStopEvtId_ = le_event_CreateId("sessionStopEvtId_", sizeof(TafDcsSessionStopEvent_t));

    // Event to send start session async events
    startSessionAsyncRspEvtId_ = le_event_CreateId("startSessionAsyncRspEvtId_",
                                                        sizeof(TafDcsSendStartSessionAsyncRsp_t));

    // Event to send stop session async events
    stopSessionAsyncRspEvtId_ = le_event_CreateId("stopSessionAsyncRspEvtId_",
                                                        sizeof(TafDcsSendStopSessionAsyncRsp_t));

    LE_INFO("Internal DCS events created.");

    // Event for session state changes from PA
    paSessionStateChangeEvtId_ = le_event_CreateId("paSessionStateChangeEvtId_",
                                                            sizeof(TafDcsSessionChangeEvent_t));

    // Event for throttled APN changes from PA
    paRoamingChangeEvtId_ = le_event_CreateId("paRoamingChangeEvtId_",
                                                            sizeof(TafDcsRoamingStatus_t));

    // Event for roaming state changes from PA
    paThrottledAPNsEvtId_ = le_event_CreateId("paThrottledAPNsEvtId_",
                                                            sizeof(TafDcsThrottledApnEventInfo_t));

    // Event for QoS TFT state changes from PA
    paQosTftEvtId_ = le_event_CreateId("paQosTftEvtId_", sizeof(TafDcsQosTftEventInfo_t));

    // Event for HW acceleration state changes from PA
    paHwAccelerationChangeEvtId_ = le_event_CreateId("paHwAccelerationChangeEvtId_",
                                                        sizeof(TafDcsHwAccelerationChangeEvent_t));

    LE_INFO("Internal PA events created.");
}

void TafDcsSvc::startThreads()
{
    LE_INFO("Start tafDcsEventsThread_");
    // Start the internal events thread
    tafDcsEventsThreadRef_ = le_thread_Create("tafDcsEventsThread", tafDcsEventsThreadHandler,
                                                                                            NULL);
    le_thread_SetJoinable(tafDcsEventsThreadRef_);
    le_thread_Start(tafDcsEventsThreadRef_);
}

void TafDcsSvc::stopThreads()
{
    // Stop the internal events thread
    le_thread_Cancel(tafDcsEventsThreadRef_);
    le_thread_Join(tafDcsEventsThreadRef_, NULL);
}

void TafDcsSvc::registerClientsConnectDisconnectHandlers()
{
    // Create client connect/disconnect handlers
    dcsClientConnectHandlerRef = le_msg_AddServiceOpenHandler
    (
        taf_dcs_GetServiceRef(),
        TafDcsSvc::onDCSClientConnect,
        NULL
    );

    dcsClientDisconnectHandlerRef = le_msg_AddServiceCloseHandler
    (
        taf_dcs_GetServiceRef(),
        TafDcsSvc::onDCSClientDisconnect,
        NULL
    );
}

void TafDcsSvc::unregisterClientsConnectDisconnectHandlers()
{
    le_msg_RemoveServiceHandler (dcsClientConnectHandlerRef);
    le_msg_RemoveServiceHandler(dcsClientDisconnectHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Client connect function. Nothing is done.
 */
//--------------------------------------------------------------------------------------------------
void TafDcsSvc::onDCSClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);
    LE_DEBUG ("Client connected: %p", sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Client disconnect function. Nothing is done.
 */
//--------------------------------------------------------------------------------------------------
void TafDcsSvc::onDCSClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);
    TAF_ERROR_IF_RET_NIL (nullptr == sessionRef, "sessionRef is NULL!");
    LE_DEBUG ("Client disconnected: %p", sessionRef);
    TafDcsClientDisconnectedEvent_t event = {sessionRef};
    // Send this event to the DCS internal event handler thread for processing.
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    le_event_Report(
        tafDcsSvc.GetClientsDisconnectedEvtId(), // clientDisconnectedEvtId_
        static_cast<void *>(&event),
        sizeof(TafDcsClientDisconnectedEvent_t)
    );
}
