/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSomeipSvr.hpp"
#include "tafSomeipGWSvc.hpp"

using namespace tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF SOME/IP server.
 */
//--------------------------------------------------------------------------------------------------
taf_SomeipSvr& taf_SomeipSvr::GetInstance()
{
    static taf_SomeipSvr instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Generic VSOME/IP message handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPHandler
(
    uint8_t routingId,
    const std::shared_ptr<vsomeip::message>& msg
)
{
    // Create the incoming Rx message from vsomeip stack.
    SomeipSvr_RxMsg_t* rxMsgPtr = (SomeipSvr_RxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
    memset(rxMsgPtr, 0, sizeof(SomeipSvr_RxMsg_t));

    // Fill message fields.
    rxMsgPtr->routingId = routingId;
    rxMsgPtr->serviceId = msg->get_service();
    rxMsgPtr->instanceId = msg->get_instance();
    rxMsgPtr->clientId = msg->get_client();
    rxMsgPtr->methodId = msg->get_method();
    rxMsgPtr->sessionId = msg->get_session();
    rxMsgPtr->interfaceVer = msg->get_interface_version();
    rxMsgPtr->protocolVer = msg->get_protocol_version();
    rxMsgPtr->isReliable = msg->is_reliable();
    rxMsgPtr->msgType = (uint8_t)msg->get_message_type();
    rxMsgPtr->payloadSize = msg->get_payload()->get_length();
    rxMsgPtr->link = LE_DLS_LINK_INIT;

    // Check the payload size.
    if(rxMsgPtr->payloadSize > sizeof(rxMsgPtr->payloadData))
    {
        LE_ERROR("VSOMEIP Rx message size is too long , dropped it.");
        le_mem_Release(rxMsgPtr);
        return;
    }

    // Copy the payload data to our own payload buffer.
    uint8_t* destPtr = &rxMsgPtr->payloadData[0];
    uint8_t* srcPtr = msg->get_payload()->get_data();
    memcpy(destPtr, srcPtr, rxMsgPtr->payloadSize);

    // Create a generic response message object.
    VsMsg_t vsMsg;

    vsMsg.type = VS_RX_MSG_PTR;
    vsMsg.ptr = (void*)rxMsgPtr;

    // Report to the common VSOMEIP msg handler in service layer.
    le_event_Report(VsomeipEvent, &vsMsg, sizeof(VsMsg_t));

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Wrapper interface to VSOME/IP register subscription handler.
 */
//--------------------------------------------------------------------------------------------------
bool taf_SomeipSvr::VSOMEIPSubsHandler
(
    taf_someipSvr_SubscriptionHandlerRef_t subsHandlerRef,
    vsomeip::client_t clientId,
    uid_t uId,
    gid_t gId,
    bool isSubscribed
)
{
    if (subsHandlerRef != NULL)
    {
        // Create a generic response message object.
        VsMsg_t vsMsg;

        vsMsg.type = VS_SUBS_HANDLE;
        vsMsg.handle.ref = (void*)subsHandlerRef;
        vsMsg.handle.clientId = clientId;
        vsMsg.handle.uId = uId;
        vsMsg.handle.gId = gId;
        vsMsg.handle.isSubscribed = isSubscribed;

        // Report to the common VSOMEIP msg handler in service layer.
        le_event_Report(VsomeipEvent, &vsMsg, sizeof(VsMsg_t));

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Wrapper interface to VSOME/IP offer service.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPOfferService
(
    SomeipSvr_Service_t* servicePtr
)
{
    // Sanity check for the service.
    LE_ASSERT((servicePtr != NULL) && (!servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Offer the service locally first.
    routingApp->offer_service(servicePtr->serviceId, servicePtr->instanceId,
                              servicePtr->majorVersion, servicePtr->minorVersion);

    // Update service configuration so that the local service can be offered on the network
    // later as well.
    if (servicePtr->udpPort != 0)
    {
        routingApp->update_service_configuration(servicePtr->serviceId,
                                                 servicePtr->instanceId,
                                                 servicePtr->udpPort,
                                                 false, false, true);
    }
    if (servicePtr->tcpPort != 0)
    {
        routingApp->update_service_configuration(servicePtr->serviceId,
                                                 servicePtr->instanceId,
                                                 servicePtr->tcpPort, true,
                                                 servicePtr->isMagicCookieEnabled,
                                                 true);
    }

    // Since the update_service_configuration() can be only called after the service is offered,
    // here we need restart the service to make the new configuration take effect immediately.
    routingApp->stop_offer_service(servicePtr->serviceId, servicePtr->instanceId,
                                   servicePtr->majorVersion, servicePtr->minorVersion);
    routingApp->offer_service(servicePtr->serviceId, servicePtr->instanceId,
                              servicePtr->majorVersion, servicePtr->minorVersion);

    // Service is marked as offered.
    servicePtr->isOffered = true;
    LE_INFO("VSOMEIP offered Service(%u:0x%x/0x%x) of version(%u.%u).",
            servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId,
            servicePtr->majorVersion, servicePtr->minorVersion);
}

//--------------------------------------------------------------------------------------------------
/**
 * Wrapper interface to VSOME/IP stop service.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPStopOfferService
(
    SomeipSvr_Service_t* servicePtr
)
{
    // Sanity check for the service.
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Stop service.
    routingApp->stop_offer_service(servicePtr->serviceId, servicePtr->instanceId,
                                   servicePtr->majorVersion, servicePtr->minorVersion);

    // Remove UPD port from the configuration.
    if (servicePtr->udpPort != 0)
    {
        routingApp->update_service_configuration(servicePtr->serviceId,
                                                 servicePtr->instanceId,
                                                 servicePtr->udpPort,
                                                 false, false, false);
    }
    // Remove TCP port from the configuration.
    if (servicePtr->tcpPort != 0)
    {
        routingApp->update_service_configuration(servicePtr->serviceId,
                                                 servicePtr->instanceId,
                                                 servicePtr->tcpPort, true,
                                                 servicePtr->isMagicCookieEnabled,
                                                 false);
    }

    // Service is marked as stopped.
    servicePtr->isOffered = false;
    LE_INFO("VSOMEIP stopped Service(%u:0x%x/0x%x) of version(%u.%u).",
            servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId,
            servicePtr->majorVersion, servicePtr->minorVersion);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Wrapper interface to VSOME/IP offer event.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPOfferEvent
(
    SomeipSvr_Event_t* eventPtr
)
{
    // Sanity check for event.
    LE_ASSERT((eventPtr != NULL) &&
              (!eventPtr->isOffered) &&
              (eventPtr->isEnabled) &&
              (!le_dls_IsEmpty(&eventPtr->eventGroupList)));

    // Sanity check for service.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, eventPtr->serviceRef);
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Populate the eventgroups that contain the event.
    std::set<vsomeip::eventgroup_t> groups;
    le_dls_Link_t* linkPtr = le_dls_Peek(&eventPtr->eventGroupList);
    while (linkPtr != NULL)
    {
        SomeipSvr_EventGroup_t* egroupPtr = CONTAINER_OF(linkPtr, SomeipSvr_EventGroup_t, link);
        if (egroupPtr != NULL)
        {
            groups.insert(egroupPtr->groupId);
        }

        linkPtr = le_dls_PeekNext(&eventPtr->eventGroupList, linkPtr);
    }

    // Offer the event.
    if (eventPtr->eventType == TAF_SOMEIPDEF_ET_FIELD)
    {
        // Offer a field event.
        routingApp->offer_event(servicePtr->serviceId, servicePtr->instanceId,
                                eventPtr->eventId, groups, vsomeip::event_type_e::ET_FIELD);
    }
    else
    {
        // Offer a pure event with cycle time.
        routingApp->offer_event(servicePtr->serviceId, servicePtr->instanceId,
                                eventPtr->eventId, groups, vsomeip::event_type_e::ET_EVENT,
                                std::chrono::milliseconds(eventPtr->cycleTime));
    }

    // Mark the event is offered.
    eventPtr->isOffered = true;
    LE_INFO("VSOMEIP offered Event(id=0x%x) of Service(%u:0x%x/0x%x).",
             eventPtr->eventId, servicePtr->routingId,
             servicePtr->serviceId, servicePtr->instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Wrapper interface to VSOME/IP stop event.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPStopOfferEvent
(
    SomeipSvr_Event_t* eventPtr
)
{
    // Sanity check for event.
    LE_ASSERT((eventPtr != NULL) &&
              (eventPtr->isOffered) &&
              (eventPtr->isEnabled) &&
              (!le_dls_IsEmpty(&eventPtr->eventGroupList)));

    // Sanity check for service.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, eventPtr->serviceRef);
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Stop offering event.
    routingApp->stop_offer_event(servicePtr->serviceId, servicePtr->instanceId,
                                 eventPtr->eventId);
    eventPtr->isOffered = false;
    LE_INFO("VSOMEIP stopped Event(id=0x%x) of Service(%u:0x%x/0x%x).",
             eventPtr->eventId, servicePtr->routingId,
             servicePtr->serviceId, servicePtr->instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * VSOMIE/IP interface to stop all events of a service.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPStopOfferAllEvents
(
    SomeipSvr_Service_t* servicePtr
)
{
    // Sanity check.
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Find the event in the event list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&servicePtr->eventList);
    while (linkPtr != NULL)
    {
        SomeipSvr_Event_t* eventPtr = CONTAINER_OF(linkPtr, SomeipSvr_Event_t, link);

        // Stop the event if it's offered.
        if ((eventPtr != NULL) && (eventPtr->isOffered))
        {
            VSOMEIPStopOfferEvent(eventPtr);
        }

        linkPtr = le_dls_PeekNext(&servicePtr->eventList, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * VSOMIE/IP interface to send a response.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPSendResponse
(
    SomeipSvr_RxMsg_t* reqPtr,
    bool isErrRsp,
    uint8_t returnCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    // Sanity check.
    LE_ASSERT((reqPtr != NULL) && (reqPtr->msgType == TAF_SOMEIPDEF_MT_REQUEST))

    SomeipSvr_Service_t* servicePtr = SearchServiceInList(reqPtr->routingId, reqPtr->serviceId,
                                                          reqPtr->instanceId);
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Create the response.
    std::shared_ptr<vsomeip::message> response =
        vsomeip::runtime::get()->create_message(reqPtr->isReliable);

    // Set the response from the request message.
    response->set_service(reqPtr->serviceId);
    response->set_instance(reqPtr->instanceId);
    response->set_method(reqPtr->methodId);
    response->set_client(reqPtr->clientId);
    response->set_session(reqPtr->sessionId);
    response->set_interface_version(reqPtr->interfaceVer);
    response->set_reliable(reqPtr->isReliable);

    // Set the response from the input parameters.
    response->set_message_type(vsomeip::message_type_e::MT_RESPONSE);
    if (isErrRsp)
    {
        response->set_message_type(vsomeip::message_type_e::MT_ERROR);
    }
    response->set_return_code((vsomeip::return_code_e)returnCode);

    // Payload data can be empty, so let's check and set payload if needed.
    if ((dataPtr != NULL) && (dataSize != 0))
    {
        // Create the payload of the response.
        std::shared_ptr<vsomeip::payload> payload =
            vsomeip::runtime::get()->create_payload();

        // Set the payload.
        payload->set_data(dataPtr, dataSize);
        response->set_payload(payload);
    }

    // Send the response.
    routingApp->send(response);

    LE_DEBUG("VSOMEIP sent Response(id=0x%x) of Service(%u:0x%x/0x%x).",
             reqPtr->methodId, servicePtr->routingId,
             servicePtr->serviceId, servicePtr->instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * VSOMIE/IP interface to send a event.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::VSOMEIPNotify
(
    SomeipSvr_Event_t* eventPtr,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    // Sanity check for event.
    LE_ASSERT((eventPtr != NULL) &&
              (eventPtr->isEnabled) &&
              (eventPtr->isOffered) &&
              (!le_dls_IsEmpty(&eventPtr->eventGroupList)))

    // Sanity check for service.
    SomeipSvr_Service_t* servicePtr =
    (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, eventPtr->serviceRef);
    LE_ASSERT((servicePtr != NULL) && (servicePtr->isOffered));

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Send the notification.
    auto payload = vsomeip::runtime::get()->create_payload();
    if ((dataPtr != NULL) && (dataSize != 0))
    {
        payload->set_data(dataPtr, dataSize);
    }
    routingApp->notify(servicePtr->serviceId, servicePtr->instanceId,
                       eventPtr->eventId, payload);

    LE_DEBUG("VSOMEIP notified event(id=0x%x) of Service(%u:0x%x/0x%x).",
             eventPtr->eventId, servicePtr->routingId,
             servicePtr->serviceId, servicePtr->instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Process the Rx message.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ProcessRxMsgRef
(
    void* msgRef
)
{
    // Get the message object.
    taf_someipSvr_RxMsgRef_t rxMsgRef = (taf_someipSvr_RxMsgRef_t)msgRef;
    SomeipSvr_RxMsg_t* rxMsgPtr =
        (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);

    if(rxMsgPtr != NULL)
    {
        // Get the service object.
        SomeipSvr_Service_t* servicePtr =
            SearchServiceInList(rxMsgPtr->routingId, rxMsgPtr->serviceId, rxMsgPtr->instanceId);

        // Simply free the message if no service and/or handler is found to handle it.
        if ((servicePtr == NULL) || (servicePtr->handlerRef == NULL))
        {
            LE_WARN("No service/handler found for rxMsgRef(%p), free it.", rxMsgRef);
            le_ref_DeleteRef(RxMsgRefMap, rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        // Get the service handler and do sanity check.
        SomeipSvr_Handler_t* handlerPtr =
            (SomeipSvr_Handler_t*)le_ref_Lookup(RxHandlerRefMap, servicePtr->handlerRef);
        LE_ASSERT(handlerPtr != NULL);
        LE_ASSERT(handlerPtr->func != NULL);

        // Add the message to service handler's message list.
        rxMsgPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&servicePtr->rxMsgList, &rxMsgPtr->link);

        // Call the service handler.
        handlerPtr->func(rxMsgRef, handlerPtr->context);
    }
    else
    {
        LE_WARN("No rxMsgPtr found for rxMsgRef(%p), dropped it.", rxMsgRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process the Rx message.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ProcessRxMsgPtr
(
    void* msgPtr
)
{
    if(msgPtr != NULL)
    {
        SomeipSvr_RxMsg_t* rxMsgPtr = (SomeipSvr_RxMsg_t*)msgPtr;

        // Get the service object.
        SomeipSvr_Service_t* servicePtr =
            SearchServiceInList(rxMsgPtr->routingId, rxMsgPtr->serviceId, rxMsgPtr->instanceId);

        // Simply free the message if no service and/or handler is found to handle it.
        if ((servicePtr == NULL) || (servicePtr->handlerRef == NULL))
        {
            LE_WARN("No service/handler found for rxMsgPtr(%p), free it.", rxMsgPtr);
            le_mem_Release(rxMsgPtr);
            return;
        }

        // Create a reference to this message.
        rxMsgPtr->ref = (taf_someipSvr_RxMsgRef_t)le_ref_CreateRef(RxMsgRefMap, rxMsgPtr);

        LE_DEBUG("VSOMEIP rxMsgRef(%p) of service(%u:0x%x/0x%x) is created.", rxMsgPtr->ref,
             rxMsgPtr->routingId, rxMsgPtr->serviceId, rxMsgPtr->instanceId);

        // Get the service handler and do sanity check.
        SomeipSvr_Handler_t* handlerPtr =
            (SomeipSvr_Handler_t*)le_ref_Lookup(RxHandlerRefMap, servicePtr->handlerRef);
        LE_ASSERT(handlerPtr != NULL);
        LE_ASSERT(handlerPtr->func != NULL);

        // Add the message to service handler's message list.
        rxMsgPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&servicePtr->rxMsgList, &rxMsgPtr->link);

        // Call the service handler.
        handlerPtr->func(rxMsgPtr->ref, handlerPtr->context);

        // Note: The rxMsgPtr is now owned by the service's message list and will be freed
        // when the message is processed via ReleaseRxMsg or SendResponse
    }
    else
    {
        LE_WARN("msgPtr is NULL.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process the subscription handler message.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ProcessSubsHandle
(
    VsSubsHandle_t handle
)
{
    taf_someipSvr_SubscriptionHandlerRef_t handlerRef =
        (taf_someipSvr_SubscriptionHandlerRef_t)handle.ref;

    SomeipSvr_SubscriptionHandler_t* handlerPtr =
        (SomeipSvr_SubscriptionHandler_t*)le_ref_Lookup(SubsHandlerRefMap, handlerRef);

    if (handlerPtr != NULL)
    {
        SomeipSvr_Service_t* servicePtr =
            (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, handlerPtr->serviceRef);

        if (servicePtr == NULL)
        {
            LE_FATAL("ServiceRef(%p) is not found.", handlerPtr->serviceRef);
            return;
        }

        LE_DEBUG("GroupId(0x%x) of service(0x%x/0x%x) is '%s' from client(0x%x).",
                handlerPtr->groupId, handlerPtr->serviceId, handlerPtr->instanceId,
                handle.isSubscribed ? "Subscribed" : "Unsubscribed", handle.clientId);

        if (handlerPtr->func != NULL)
        {
            handlerPtr->func(handlerPtr->serviceRef, handlerPtr->groupId,
                             handle.isSubscribed, handlerPtr->context);
        }
    }
    else
    {
        LE_WARN("No subsHandlerPtr found for handlerRef(%p), dropped it.", handlerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Generic VSOMEIP event handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::RxEventHandler
(
    void* reportPtr
)
{
    if (reportPtr != NULL)
    {
        // Get someip server instance.
        taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();

        // Get the msg type.
        VsMsg_t* vsMsgPtr = (VsMsg_t*)reportPtr;
        VsMsgType_t msgType = vsMsgPtr->type;

        switch(msgType)
        {
            case VS_RX_MSG_PTR:
                mySomeipSvr.ProcessRxMsgPtr(vsMsgPtr->ptr);
                break;

            case VS_RX_MSG_REF:
                mySomeipSvr.ProcessRxMsgRef(vsMsgPtr->ref);
                break;

            case VS_SUBS_HANDLE:
                mySomeipSvr.ProcessSubsHandle(vsMsgPtr->handle);
                break;

            default:
                LE_FATAL("Unknown VsMsg(type=%d).", msgType);
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Event object destructor.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::EventObjDestructor
(
    void* objPtr
)
{
    if (objPtr != NULL)
    {
        // Get someip server instance.
        taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();

        // Clear the eventgroup list.
        SomeipSvr_Event_t* eventPtr = (SomeipSvr_Event_t*)objPtr;
        mySomeipSvr.ClearEventGroupList(&eventPtr->eventGroupList);

        LE_DEBUG("Destructor event(id=0x%x).", eventPtr->eventId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Service object destructor.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ServiceObjDestructor
(
    void* objPtr
)
{
    if (objPtr != NULL)
    {
        SomeipSvr_Service_t* servicePtr = (SomeipSvr_Service_t*)objPtr;

        // Get someip server instance.
        taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();

        // Clear the message list.
        mySomeipSvr.ClearRxMessageList(&servicePtr->rxMsgList);

        // Clear the event list.
        mySomeipSvr.ClearEventList(&servicePtr->eventList);

        LE_DEBUG("Destructor service(%u:0x%x/0x%x).", servicePtr->routingId,
                servicePtr->serviceId, servicePtr->instanceId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    // Get someip server instance.
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();

    // Go through the Service list and find out the service that is represented by the client.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(mySomeipSvr.ServiceRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        SomeipSvr_Service_t* servicePtr = (SomeipSvr_Service_t*)le_ref_GetValue(iterRef);

        // Do sanity check here. The service handler shall be already freed in IPC stub code.
        // Here we just free the service object. The event list and message list are freed
        // in service destructor.
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->ref == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            // Stop the service and all its events if the service is offered.
            if (servicePtr->isOffered)
            {
                // Stop all the events of the service.
                mySomeipSvr.VSOMEIPStopOfferAllEvents(servicePtr);

                // Stop the service.
                mySomeipSvr.VSOMEIPStopOfferService(servicePtr);
            }

            // Free the service.
            LE_DEBUG("Freed Service(%u:0x%x/0x%x).", servicePtr->routingId,
                    servicePtr->serviceId, servicePtr->instanceId);
            le_ref_DeleteRef(mySomeipSvr.ServiceRefMap, servicePtr->ref);
            le_mem_Release(servicePtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a server service instance object by serviceId and instanceId, return the object pointer.
 */
//--------------------------------------------------------------------------------------------------
SomeipSvr_Service_t* taf_SomeipSvr::SearchServiceInList
(
    uint8_t routingId,
    uint16_t serviceId,
    uint16_t instanceId
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        SomeipSvr_Service_t* servicePtr = (SomeipSvr_Service_t*)le_ref_GetValue(iterRef);

        if ((servicePtr != NULL) &&
            (servicePtr->routingId == routingId) &&
            (servicePtr->serviceId == serviceId) &&
            (servicePtr->instanceId == instanceId))
        {
            return servicePtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search an event in an event list and return the event object pointer.
 */
//--------------------------------------------------------------------------------------------------
SomeipSvr_Event_t* taf_SomeipSvr::SearchEventInList
(
    le_dls_List_t* eventListPtr,
    uint16_t eventId
)
{
    // Find the event in the event list.
    le_dls_Link_t* linkPtr = le_dls_Peek(eventListPtr);
    while (linkPtr != NULL)
    {
        SomeipSvr_Event_t* eventPtr = CONTAINER_OF(linkPtr, SomeipSvr_Event_t, link);

        if ((eventPtr != NULL) && (eventId == eventPtr->eventId))
        {
            return eventPtr;
        }

        linkPtr = le_dls_PeekNext(eventListPtr, linkPtr);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search an eventGroup in an eventGroup list and return the eventGroup pointer.
 */
//--------------------------------------------------------------------------------------------------
SomeipSvr_EventGroup_t* taf_SomeipSvr::SearchEventGroupInList
(
    le_dls_List_t* eventGroupListPtr,
    uint16_t eventGroupId
)
{
    // Find the group in the group list.
    le_dls_Link_t* linkPtr = le_dls_Peek(eventGroupListPtr);
    while (linkPtr != NULL)
    {
        SomeipSvr_EventGroup_t* egroupPtr = CONTAINER_OF(linkPtr, SomeipSvr_EventGroup_t, link);

        if ((egroupPtr != NULL) && (eventGroupId == egroupPtr->groupId))
        {
            return egroupPtr;
        }

        linkPtr = le_dls_PeekNext(eventGroupListPtr, linkPtr);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a subscription handler for a given eventGroupId of the service.
 */
//--------------------------------------------------------------------------------------------------
SomeipSvr_SubscriptionHandler_t* taf_SomeipSvr::SearchSubscriptionHandlerInList
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t groupId
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SubsHandlerRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        SomeipSvr_SubscriptionHandler_t* subsHandlerPtr =
            (SomeipSvr_SubscriptionHandler_t*)le_ref_GetValue(iterRef);

        if ((subsHandlerPtr != NULL) &&
            (subsHandlerPtr->serviceRef == serviceRef) &&
            (subsHandlerPtr->groupId == groupId))
        {
            return subsHandlerPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear an event list
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ClearEventList
(
    le_dls_List_t* eventListPtr
)
{
    le_dls_Link_t* linkPtr = le_dls_Pop(eventListPtr);

    while (linkPtr != NULL)
    {
        SomeipSvr_Event_t* eventPtr = CONTAINER_OF(linkPtr, SomeipSvr_Event_t, link);

        if (eventPtr != NULL)
        {
            LE_DEBUG("Freed event(id=0x%x).", eventPtr->eventId);
            le_mem_Release(eventPtr);
        }

        linkPtr = le_dls_Pop(eventListPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear an event group list
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ClearEventGroupList
(
    le_dls_List_t* eventGroupListPtr
)
{
    le_dls_Link_t* linkPtr = le_dls_Pop(eventGroupListPtr);

    while (linkPtr != NULL)
    {
        SomeipSvr_EventGroup_t* egroupPtr = CONTAINER_OF(linkPtr, SomeipSvr_EventGroup_t, link);

        if (egroupPtr != NULL)
        {
            LE_DEBUG("Freed eventGroup(id=0x%x).", egroupPtr->groupId);
            le_mem_Release(egroupPtr);
        }

        linkPtr = le_dls_Pop(eventGroupListPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear a Rx message list.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::ClearRxMessageList
(
    le_dls_List_t* msgListPtr
)
{
    le_dls_Link_t* linkPtr = le_dls_Pop(msgListPtr);

    while (linkPtr != NULL)
    {
        SomeipSvr_RxMsg_t* msgPtr = CONTAINER_OF(linkPtr, SomeipSvr_RxMsg_t, link);

        if (msgPtr != NULL)
        {
            LE_DEBUG("Freed rxMsg(ref=%p).", msgPtr->ref);
            le_ref_DeleteRef(RxMsgRefMap, msgPtr->ref);
            le_mem_Release(msgPtr);
        }

        linkPtr = le_dls_Pop(msgListPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the reference of a service, also create a reference for the service if the reference doesn't
 * exist.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_ServiceRef_t taf_SomeipSvr::GetServiceRef
(
    uint8_t routingId,
    uint16_t serviceId,
    uint16_t instanceId
)
{
    // Check the valid serviceId and instanceId according to [PRS_SOMEIPSD_00515] and
    // [PRS_SOMEIPSD_00516] of <SOME/IP Service Discovery Protocol Specification AUTOSAR FO R21-11>.
    if (((serviceId == 0xFFFF) || (serviceId == 0xFFFE) || (serviceId == 0x0000)) ||
        ((instanceId == 0xFFFF) || (instanceId == 0x0000)))
    {
        LE_ERROR("Invalid serviceId or instarnceId.");
        return NULL;
    }

    // Search the service.
    SomeipSvr_Service_t* servicePtr = SearchServiceInList(routingId, serviceId, instanceId);

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (SomeipSvr_Service_t*)le_mem_ForceAlloc(ServicePool);
        memset(servicePtr, 0, sizeof(SomeipSvr_Service_t));

        // Init the serviceId and instanceId and versions.
        servicePtr->routingId = routingId;
        servicePtr->serviceId = serviceId;
        servicePtr->instanceId = instanceId;
        servicePtr->majorVersion = TAF_SOMEIPDEF_DEFAULT_MAJOR;
        servicePtr->minorVersion = TAF_SOMEIPDEF_DEFAULT_MINOR;

        // Init the lists.
        servicePtr->eventList = LE_DLS_LIST_INIT;
        servicePtr->rxMsgList = LE_DLS_LIST_INIT;

        // Init the IP ports. Declare a local service by default by seting both
        // TCP port and UDP port to 0.
        servicePtr->udpPort = 0;
        servicePtr->tcpPort = 0;
        servicePtr->isMagicCookieEnabled = false;
        servicePtr->isOffered = false;

        // Init the service Rx Handler.
        servicePtr->handlerRef = NULL;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_someipSvr_GetClientSessionRef();

        // Create a Safe Reference for this service object.
        servicePtr->ref = (taf_someipSvr_ServiceRef_t)le_ref_CreateRef(ServiceRefMap, servicePtr);

        LE_DEBUG("serviceRef %p of client %p is created for Service(%u:0x%x/0x%x).",
                servicePtr->ref, servicePtr->sessionRef, routingId, serviceId, instanceId);
    }
    else
    {
        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
        {
            LE_ERROR("The Service(%u:0x%x/0x%x) is not created by the client.",
                     servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
            return NULL;
        }
    }

    LE_DEBUG("Get serviceRef %p for Service(%u:0x%x/0x%x).", servicePtr->ref,
            routingId, serviceId, instanceId);

    return servicePtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service version.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::SetServiceVersion
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint8_t majVer,
    uint32_t minVer
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is already offered.
    if (servicePtr->isOffered)
    {
        LE_ERROR("Service(%u:0x%x/0x%x) is already offered.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Update the service version.
    servicePtr->majorVersion = majVer;
    servicePtr->minorVersion = minVer;

    LE_DEBUG("Set majVer=%u minVer=%u for serviceRef %p.", majVer, minVer, serviceRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service version.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::SetServicePort
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t udpPort,
    uint16_t tcpPort,
    bool enableMagicCookies
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is already offered.
    if (servicePtr->isOffered)
    {
        LE_ERROR("Service(%u:0x%x/0x%x) is already offered.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Update the IP ports.
    servicePtr->udpPort = udpPort;
    servicePtr->tcpPort = tcpPort;
    servicePtr->isMagicCookieEnabled = enableMagicCookies;

    LE_DEBUG("Set udpPort=0x%x tcpPort=0x%x MagicCookie= 0x%x for serviceRef %p.",
            udpPort, tcpPort, enableMagicCookies, serviceRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer a service.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::OfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) found!", serviceRef);
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is already offered.
    if (servicePtr->isOffered)
    {
        LE_WARN("Service(%u:0x%x/0x%x) is already offered, skipped OfferService().",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_DUPLICATE;
    }

    // Call VSOMEIP functions.
    VSOMEIPOfferService(servicePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a service.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::StopOfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is already stopped.
    if (!servicePtr->isOffered)
    {
        LE_WARN("Service(%u:0x%x/0x%x) is already stopped, skipped StopOfferService().",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_DUPLICATE;
    }

    // Call VSOMEIP functions.

    // Stop all events of the service.
    VSOMEIPStopOfferAllEvents(servicePtr);

    // Next stop the service.
    VSOMEIPStopOfferService(servicePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the event by adding it into a given event group.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::EnableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId,
    uint16_t eventgroupId
)
{
    // Check the vaild eventId and eventGroupId according to [PRS_SOMEIPSD_00517] and
    // [PRS_SOMEIPSD_00531] of <SOME/IP Service Discovery Protocol Specification AUTOSAR FO R21-11>.
    if (((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF)) || ((eventgroupId == 0x0000) || (eventgroupId == 0xFFFF)))
    {
        LE_ERROR("Invalid eventId or eventgroupId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Create an event object if it doesn't exist in the list.
    if (eventPtr == NULL)
    {
        eventPtr = (SomeipSvr_Event_t*)le_mem_ForceAlloc(EventPool);
        memset(eventPtr, 0, sizeof(SomeipSvr_Event_t));

        // Init the event object.
        eventPtr->eventId = eventId;
        eventPtr->eventType = TAF_SOMEIPDEF_ET_EVENT;
        eventPtr->cycleTime = 0;
        eventPtr->link = LE_DLS_LINK_INIT;
        eventPtr->eventGroupList = LE_DLS_LIST_INIT;
        eventPtr->isEnabled = false;
        eventPtr->isOffered = false;
        eventPtr->serviceRef = serviceRef;

        // Add the event into service event list.
        le_dls_Queue(&servicePtr->eventList, &eventPtr->link);

        LE_DEBUG("Created Event(id=0x%x) for Service(%u:0x%x/0x%x)",
                eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
    }

    // Check if the event is already offered.
    if (eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is already offered.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the event is already added into the eventGroup.
    SomeipSvr_EventGroup_t* egroupPtr = SearchEventGroupInList(&eventPtr->eventGroupList,
                                                                   eventgroupId);
    if (egroupPtr == NULL)
    {
        egroupPtr = (SomeipSvr_EventGroup_t*)le_mem_ForceAlloc(EventGroupPool);
        memset(egroupPtr, 0, sizeof(SomeipSvr_EventGroup_t));

        // Init the eventGroup object.
        egroupPtr->groupId = eventgroupId;
        egroupPtr->link = LE_DLS_LINK_INIT;

        // Add the eventGroup into the event group list.
        le_dls_Queue(&eventPtr->eventGroupList, &egroupPtr->link);

        LE_DEBUG("Created EventGroup(id=0x%x) for event(id=0x%x) of Service(%u:0x%x/0x%x)",
                eventgroupId, eventId, servicePtr->routingId,
                servicePtr->serviceId, servicePtr->instanceId);
    }

    // Enable the event after eventGroup is set.
    eventPtr->isEnabled = true;
    LE_DEBUG("Enabled eventGroup(id=0x%x) for event(id=0x%x) of service(%u:0x%x/0x%x)",
            eventgroupId, eventId, servicePtr->routingId,
            servicePtr->serviceId, servicePtr->instanceId);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set event type.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::SetEventType
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId,
    taf_someipDef_EventType_t eventType
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Create an event object if it doesn't exist in the list.
    if (eventPtr == NULL)
    {
        eventPtr = (SomeipSvr_Event_t*)le_mem_ForceAlloc(EventPool);
        memset(eventPtr, 0, sizeof(SomeipSvr_Event_t));

        // Init the event object.
        eventPtr->eventId = eventId;
        eventPtr->eventType = TAF_SOMEIPDEF_ET_EVENT;
        eventPtr->cycleTime = 0;
        eventPtr->link = LE_DLS_LINK_INIT;
        eventPtr->eventGroupList = LE_DLS_LIST_INIT;
        eventPtr->isEnabled = false;
        eventPtr->isOffered = false;
        eventPtr->serviceRef = serviceRef;

        // Add the event into service event list.
        le_dls_Queue(&servicePtr->eventList, &eventPtr->link);

        LE_DEBUG("Created Event(id=0x%x) for Service(%u:0x%x/0x%x)",
                eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
    }

    // Check if the event is already offered.
    if (eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) for Service(%u:0x%x/0x%x) is already offered.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Set event type.
    eventPtr->eventType = eventType;

    LE_DEBUG("Set eventType(0x%x) for Event(id=0x%x) for serviceRef %p.",
             eventType, eventId, serviceRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set event cycle time for pure event.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::SetEventCycleTime
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId,
    uint32_t cycleTime
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) found!", serviceRef);
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Create an event object if it doesn't exist in the list.
    if (eventPtr == NULL)
    {
        eventPtr = (SomeipSvr_Event_t*)le_mem_ForceAlloc(EventPool);
        memset(eventPtr, 0, sizeof(SomeipSvr_Event_t));

        // Init the event object.
        eventPtr->eventId = eventId;
        eventPtr->eventType = TAF_SOMEIPDEF_ET_EVENT;
        eventPtr->cycleTime = 0;
        eventPtr->link = LE_DLS_LINK_INIT;
        eventPtr->eventGroupList = LE_DLS_LIST_INIT;
        eventPtr->isEnabled = false;
        eventPtr->isOffered = false;
        eventPtr->serviceRef = serviceRef;

        // Add the event into service event list.
        le_dls_Queue(&servicePtr->eventList, &eventPtr->link);

        LE_DEBUG("Create Event(id=0x%x) for Service(%u:0x%x/0x%x)",
                eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
    }

    // Check if the event is already offered.
    if (eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) for Service(%u:0x%x/0x%x) is already offered.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check the event type and only ET_EVENT is allowed to set the cycle time.
    if (eventPtr->eventType != TAF_SOMEIPDEF_ET_EVENT)
    {
        LE_ERROR("Only ET_EVENT is allowed to set the cycle time.");
        return LE_NOT_PERMITTED;
    }

    // Set event clcle time
    eventPtr->cycleTime = cycleTime;

    LE_DEBUG("Set cycleTime(0x%x) for Event(id=0x%x) for serviceRef %p.",
            cycleTime, eventId, serviceRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the event and remove the event from the event group list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::DisableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) found!", serviceRef);
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    if (eventPtr != NULL)
    {
        // Check if the event is already offered.
        if (eventPtr->isOffered)
        {
            LE_WARN("Event(id=0x%x) of Service(%u:0x%x/0x%x) is already offered.",
                    eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
            return LE_NOT_PERMITTED;
        }

        if (eventPtr->isEnabled)
        {
            // Remove the event from eventGroup list.
            ClearEventGroupList(&eventPtr->eventGroupList);

            // Disable the event after eventGroup list is cleared.
            eventPtr->isEnabled = false;
            LE_DEBUG("Disabled event(id=0x%x) of service(%u:0x%x/0x%x)",
                    eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
            return LE_OK;
        }
    }

    // If event object is not found then it's disabled and not offered by default,
    // return LE_OK is fine.
    LE_WARN("Event(id=0x%x) of Service(%u:0x%x/0x%x) is already disabled.",
            eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer the event
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::OfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is offered. The event is allowed to be offered
    // after its service being offered.
    if (!servicePtr->isOffered)
    {
        LE_ERROR("Service(%u:0x%x/0x%x) is not offered yet.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Check if the event is enabled. The event is allowed to be offered
    // after its being enabled.
    if ((eventPtr == NULL) || (!eventPtr->isEnabled) ||
        (le_dls_IsEmpty(&eventPtr->eventGroupList)))
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is not enabled yet.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the event is offered.
    if (eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is already offered.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_DUPLICATE;
    }

    // Call VSOMEIP offer event function.
    VSOMEIPOfferEvent(eventPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop offering the event
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::StopOfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is offered.
    if (!servicePtr->isOffered)
    {
        LE_WARN("Service(%u:0x%x/0x%x) is not offered yet.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Check if the event is enabled.
    if ((eventPtr == NULL) || (!eventPtr->isEnabled) ||
        (le_dls_IsEmpty(&eventPtr->eventGroupList)))
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is not enabled yet.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the event is stopped.
    if (!eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is already stopped.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_DUPLICATE;
    }

    // Call VSOMEIP functions.
    VSOMEIPStopOfferEvent(eventPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send an event/field notification
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::Notify
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventId,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    if ((!(eventId & TAF_SOMEIPDEF_EVENT_MASK)) || (eventId == TAF_SOMEIPDEF_EVENT_MASK) ||
        (eventId == 0xFFFF))
    {
        LE_ERROR("Invalid eventId.");
        return LE_BAD_PARAMETER;
    }

    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is offered.
    if (!servicePtr->isOffered)
    {
        LE_WARN("Service(%u:0x%x/0x%x) is not offered yet.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Search the event in service's event list.
    SomeipSvr_Event_t* eventPtr = SearchEventInList(&servicePtr->eventList, eventId);

    // Check if the event is enabled.
    if ((eventPtr == NULL) || (!eventPtr->isEnabled) ||
        (le_dls_IsEmpty(&eventPtr->eventGroupList)))
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is not enabled yet.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the event is stopped.
    if (!eventPtr->isOffered)
    {
        LE_ERROR("Event(id=0x%x) of Service(%u:0x%x/0x%x) is not offered.",
                 eventId, servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Call VSOMEIP functions.
    VSOMEIPNotify(eventPtr, dataPtr, dataSize);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service. Note only one Rx Handler can be registere per service.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_RxMsgHandlerRef_t taf_SomeipSvr::AddRxMsgHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
    taf_someipSvr_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if ((servicePtr == NULL) || (handlerPtr == NULL))
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return NULL;
    }

    // Check if a Rx Handler for the service is already registered.
    if (servicePtr->handlerRef != NULL)
    {
        LE_ERROR("Rx Handler for Service(%u:0x%x/0x%x) is already registered.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return NULL;
    }

    // Create and set the Rx Handler.
    SomeipSvr_Handler_t* rxHandlerPtr =
        (SomeipSvr_Handler_t*)le_mem_ForceAlloc(RxHandlerPool);
    memset(rxHandlerPtr, 0, sizeof(SomeipSvr_Handler_t));

    // Init the fields.
    rxHandlerPtr->serviceRef = serviceRef;
    rxHandlerPtr->func = handlerPtr;
    rxHandlerPtr->context = contextPtr;
    rxHandlerPtr->ref =
        (taf_someipSvr_RxMsgHandlerRef_t)le_ref_CreateRef(RxHandlerRefMap, rxHandlerPtr);

    // Attach the Rx Handler to the service.
    servicePtr->handlerRef = rxHandlerPtr->ref;

    LE_DEBUG("Created handlerRef(%p) for Service(%u:0x%x/0x%x).",
            rxHandlerPtr->ref, servicePtr->routingId,
            servicePtr->serviceId, servicePtr->instanceId);

    return rxHandlerPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::RemoveRxMsgHandler
(
    taf_someipSvr_RxMsgHandlerRef_t handlerRef
)
{
    SomeipSvr_Handler_t* rxHandlerPtr =
        (SomeipSvr_Handler_t*)le_ref_Lookup(RxHandlerRefMap, handlerRef);

    if (rxHandlerPtr != NULL)
    {
        // Do sanity check.
        LE_ASSERT(rxHandlerPtr->ref == handlerRef);

        SomeipSvr_Service_t* servicePtr =
            (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, rxHandlerPtr->serviceRef);

        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr != NULL)
        {
            if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
            {
                LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                         servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
                return;
            }

            // Detach the handler to the service.
            servicePtr->handlerRef = NULL;
        }

        LE_DEBUG("Removed rxHandlerRef(%p).", handlerRef);

        // Free the handler.
        le_ref_DeleteRef(RxHandlerRefMap, handlerRef);
        le_mem_Release(rxHandlerPtr);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a Subscription handler for a given eventGroup of service.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_SubscriptionHandlerRef_t taf_SomeipSvr::AddSubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventGroupId,
    taf_someipSvr_SubscriptionHandlerFunc_t handlerPtr,
    void * contextPtr
)
{
    // Find the service in the list.
    SomeipSvr_Service_t* servicePtr =
        (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, serviceRef);
    if ((servicePtr == NULL) || (handlerPtr == NULL) ||
        (eventGroupId == 0x0000) || (eventGroupId == 0xFFFF))
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    // Do sanity check.
    LE_ASSERT(servicePtr->ref == serviceRef);

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return NULL;
    }

    // Check if a subscription handler for the given eventGroupId of the service
    // is already registered.
    SomeipSvr_SubscriptionHandler_t* subsHandlerPtr =
        SearchSubscriptionHandlerInList(serviceRef, eventGroupId);
    if (subsHandlerPtr != NULL)
    {
        LE_ERROR("SubsHandler for groupId(0x%x) of Service(%u:0x%x/0x%x) is already registered.",
                 eventGroupId, servicePtr->routingId,
                 servicePtr->serviceId, servicePtr->instanceId);
        return NULL;
    }

    // Create and set the Subscription Handler.
    subsHandlerPtr = (SomeipSvr_SubscriptionHandler_t*)le_mem_ForceAlloc(SubsHandlerPool);
    memset(subsHandlerPtr, 0, sizeof(SomeipSvr_SubscriptionHandler_t));

    // Init the fields.
    subsHandlerPtr->serviceRef = serviceRef;
    subsHandlerPtr->routingId = servicePtr->routingId;
    subsHandlerPtr->serviceId = servicePtr->serviceId;
    subsHandlerPtr->instanceId = servicePtr->instanceId;
    subsHandlerPtr->groupId = eventGroupId;
    subsHandlerPtr->func = handlerPtr;
    subsHandlerPtr->context = contextPtr;
    subsHandlerPtr->ref =
        (taf_someipSvr_SubscriptionHandlerRef_t)le_ref_CreateRef(SubsHandlerRefMap,
                                                                 subsHandlerPtr);

    // Get corresponding vsomeip routing manager instance.
    std::shared_ptr<vsomeip::application> routingApp =
        someip_GetRoutingManager(servicePtr->routingId);

    // Register to VSOMEIP subs handler.
    routingApp->register_subscription_handler(servicePtr->serviceId,
                                              servicePtr->instanceId,
                                              eventGroupId,
                                              std::bind(&taf_SomeipSvr::VSOMEIPSubsHandler,
                                              this, subsHandlerPtr->ref, std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4));

    LE_DEBUG("Created SubsHandlerRef(%p) for groupId(0x%x) of Service(%u:0x%x/0x%x).",
            subsHandlerPtr->ref, eventGroupId,
            servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);

    return subsHandlerPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the Subscription handler for a given eventGroup of service.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::RemoveSubscriptionHandler
(
    taf_someipSvr_SubscriptionHandlerRef_t handlerRef
)
{
    SomeipSvr_SubscriptionHandler_t* subsHandlerPtr =
        (SomeipSvr_SubscriptionHandler_t*)le_ref_Lookup(SubsHandlerRefMap, handlerRef);

    if (subsHandlerPtr != NULL)
    {
        // Do sanity check.
        LE_ASSERT(subsHandlerPtr->ref == handlerRef);

        SomeipSvr_Service_t* servicePtr =
            (SomeipSvr_Service_t*)le_ref_Lookup(ServiceRefMap, subsHandlerPtr->serviceRef);

        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr != NULL)
        {
            if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
            {
                LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                         servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
                return;
            }
        }

        LE_DEBUG("Removed subsHandlerRef(%p).", handlerRef);

        // Get corresponding vsomeip routing manager instance.
        std::shared_ptr<vsomeip::application> routingApp =
            someip_GetRoutingManager(subsHandlerPtr->routingId);

        // Deregister VSOMEIP subs handler.
        routingApp->unregister_subscription_handler(subsHandlerPtr->serviceId,
                                                    subsHandlerPtr->instanceId,
                                                    subsHandlerPtr->groupId);

        // Free the handler.
        le_ref_DeleteRef(SubsHandlerRefMap, handlerRef);
        le_mem_Release(subsHandlerPtr);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the serviceId and instanceId from a Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetServiceId
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint16_t* serviceIdPtr,
    uint16_t* instanceIdPtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (serviceIdPtr == NULL) || (instanceIdPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    *serviceIdPtr = msgPtr->serviceId;
    *instanceIdPtr = msgPtr->instanceId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the methodId from a Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetMethodId
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint16_t* methodIdPtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (methodIdPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    *methodIdPtr = msgPtr->methodId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the clientId from a Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetClientId
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint16_t* clientIdPtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (clientIdPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    *clientIdPtr = msgPtr->clientId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message type from a Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetMsgType
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint8_t* msgTypePtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (msgTypePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    *msgTypePtr = msgPtr->msgType;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload size of the Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetPayloadSize
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint32_t* payloadSizePtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (payloadSizePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    *payloadSizePtr = msgPtr->payloadSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload data of the Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::GetPayloadData
(
    taf_someipSvr_RxMsgRef_t msgRef,
    uint8_t* dataPtr,
    size_t* dataSizePtr
)
{
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if ((msgPtr == NULL) || (dataPtr == NULL) || (dataSizePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    size_t desSize = *dataSizePtr;
    size_t srcSize = msgPtr->payloadSize;
    size_t copySize = srcSize > desSize ? desSize : srcSize;

    memcpy(dataPtr, &msgPtr->payloadData[0], copySize);
    *dataSizePtr = copySize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send Response.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::SendResponse
(
    taf_someipSvr_RxMsgRef_t msgRef,
    bool isErrRsp,
    uint8_t returnCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    // Find the Rx message object.
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("msgPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Check if the Rx message is a REQUEST message, only REQUEST is allowed
    // to send a response.
    if (msgPtr->msgType != TAF_SOMEIPDEF_MT_REQUEST)
    {
        LE_ERROR("Invalid msgType, Only REQUEST message is allowed to send response.");
        return LE_NOT_PERMITTED;
    }

    // Check the return code for ERROR type message. An ERROR message is not allowed to set
    // return code to E_OK.
    if (isErrRsp && (returnCode == TAF_SOMEIPDEF_E_OK))
    {
        LE_ERROR("ERROR message is not allowed to set returnCode = E_OK.");
        return LE_NOT_PERMITTED;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    // Get the service.
    SomeipSvr_Service_t* servicePtr = SearchServiceInList(msgPtr->routingId, msgPtr->serviceId,
                                                          msgPtr->instanceId);

    // Check if the service is offered.
    if ((servicePtr == NULL) || (!servicePtr->isOffered))
    {
        LE_ERROR("Service(%u:0x%x/0x%x) is not offered.", msgPtr->routingId,
                 msgPtr->serviceId, msgPtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Call VSOMEIP send response function.
    VSOMEIPSendResponse(msgPtr, isErrRsp, returnCode, dataPtr, dataSize);

    // Remove the message from message list.
    le_dls_Remove(&servicePtr->rxMsgList, &msgPtr->link);

    // Free the message.
    le_ref_DeleteRef(RxMsgRefMap, msgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Freed msgRef(%p).", msgRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a Rx message.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SomeipSvr::ReleaseRxMsg
(
    taf_someipSvr_RxMsgRef_t msgRef
)
{
    // Find the Rx message object.
    SomeipSvr_RxMsg_t* msgPtr = (SomeipSvr_RxMsg_t*)le_ref_Lookup(RxMsgRefMap, msgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("msgPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Do sanity check.
    LE_ASSERT(msgPtr->ref == msgRef);

    // Get the service.
    SomeipSvr_Service_t* servicePtr = SearchServiceInList(msgPtr->routingId, msgPtr->serviceId,
                                                          msgPtr->instanceId);

    if (servicePtr == NULL)
    {
        LE_ERROR("servicePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Only the service owner app can get the service reference for subsequent operations.
    if (servicePtr->sessionRef != taf_someipSvr_GetClientSessionRef())
    {
        LE_ERROR("The Service(%u:0x%x/0x%x) is not created by this client.",
                 servicePtr->routingId, servicePtr->serviceId, servicePtr->instanceId);
        return LE_NOT_PERMITTED;
    }

    // Remove the message from message list.
    le_dls_Remove(&servicePtr->rxMsgList, &msgPtr->link);

    // Free the message.
    le_ref_DeleteRef(RxMsgRefMap, msgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Freed msgRef(%p).", msgRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_SomeipSvr::Init
(
    void
)
{
    // Create reference maps.
    ServiceRefMap = le_ref_CreateMap("Server ServiceRefMap", DEFAULT_SERVICE_REF_CNT);
    RxMsgRefMap = le_ref_CreateMap("Server RxMsgRefMap", DEFAULT_RXMSG_REF_CNT);
    RxHandlerRefMap = le_ref_CreateMap("Server RxHandlerRefMap", DEFAULT_RXHANDLER_REF_CNT);
    SubsHandlerRefMap = le_ref_CreateMap("Server SubsHandlerRefMap", DEFAULT_SUBSHANDLER_REF_CNT);

    // Create memory pools.
    ServicePool = le_mem_CreatePool("Server ServicePool", sizeof(SomeipSvr_Service_t));
    EventPool = le_mem_CreatePool("Server EventPool", sizeof(SomeipSvr_Event_t));
    EventGroupPool = le_mem_CreatePool("Server EventGroupPool", sizeof(SomeipSvr_EventGroup_t));
    RxMsgPool = le_mem_CreatePool("Server RxMsgPool", sizeof(SomeipSvr_RxMsg_t));
    RxHandlerPool = le_mem_CreatePool("Server HandlerPool", sizeof(SomeipSvr_Handler_t));
    SubsHandlerPool = le_mem_CreatePool("Server SubsHandlerPool",
                                        sizeof(SomeipSvr_SubscriptionHandler_t));

    // Set memory pool destructors.
    le_mem_SetDestructor(EventPool, taf_SomeipSvr::EventObjDestructor);
    le_mem_SetDestructor(ServicePool, taf_SomeipSvr::ServiceObjDestructor);

    // Create the event and add event handler.
    VsomeipEvent = le_event_CreateId("Server Vsomip Event", sizeof(VsMsg_t));
    VsomeipEventHandlerRef = le_event_AddHandler("Server Vsomeip Event Handler",
                                                 VsomeipEvent, taf_SomeipSvr::RxEventHandler);
    // Create client session close hander.
    le_msg_AddServiceCloseHandler(taf_someipSvr_GetServiceRef(),
                                  taf_SomeipSvr::OnClientDisconnection, NULL);

    LE_INFO("taf_SomeipSvr Service started");
}
