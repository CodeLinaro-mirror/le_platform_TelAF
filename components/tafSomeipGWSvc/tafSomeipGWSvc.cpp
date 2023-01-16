/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSomeipSvr.hpp"
#include "tafSomeipClnt.hpp"
#include "tafSvcIF.hpp"

#define VSOMEIP_APP_NAME "tafSomeipGWSvc"
using namespace telux::tafsvc;

class taf_vsomeipApp
{
    public:
        taf_vsomeipApp() {};
        ~taf_vsomeipApp() {};
        bool init()
        {
            app = vsomeip::runtime::get()->create_application(VSOMEIP_APP_NAME);
            if (!app->init())
            {
                LE_ERROR("Couldn't initialize VSOMEIP application '%s'.", VSOMEIP_APP_NAME);
                return false;
            }
            app->register_state_handler(std::bind(&taf_vsomeipApp::onState,
                                        this, std::placeholders::_1));
            app->register_message_handler(vsomeip::ANY_SERVICE,
                                          vsomeip::ANY_INSTANCE,
                                          vsomeip::ANY_METHOD,
                                          std::bind(&taf_vsomeipApp::onMessage,
                                          this, std::placeholders::_1));

            LE_INFO("VSOMEIP application '%s' is initialized.", VSOMEIP_APP_NAME);
            return true;
        }
        void start()
        {
            app->start();
        }
        void stop()
        {
            app->clear_all_handler();
            app->stop();
        }
        std::shared_ptr<vsomeip::application>& getApp()
        {
            return app;
        }
        void onState(vsomeip::state_type_e state)
        {
            if (state == vsomeip::state_type_e::ST_REGISTERED)
            {
                LE_INFO("VSOMEIP application '%s' is registered.", VSOMEIP_APP_NAME);
            }
            else if (state == vsomeip::state_type_e::ST_DEREGISTERED)
            {
                LE_INFO("VSOMEIP application '%s' is de-registered.", VSOMEIP_APP_NAME);
            }
        }
        void onMessage(const std::shared_ptr<vsomeip::message> &msg)
        {
            vsomeip::message_type_e msgType = msg->get_message_type();
            vsomeip::length_t msgLen = msg->get_payload()->get_length();
            vsomeip::method_t methodId = msg->get_method();

            LE_DEBUG("VSOMEIP message(len=%" PRIu32 ")with Client/Session/Type[0x%x/0x%x/0x%x].",
                     msgLen, msg->get_client(), msg->get_session(), (uint32_t)msgType);

            // Sanity check for the payload size.
            if (msgLen > TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE)
            {
                LE_WARN("Payload size overflows, dropped it.");
                return;
            }
            // Sanity check for request message for server instance.
            if (((vsomeip::message_type_e::MT_REQUEST == msgType) ||
                (vsomeip::message_type_e::MT_REQUEST_NO_RETURN == msgType)) &&
                !(methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
                mySomeipSvr.VSOMEIPHandler(msg);
                return;
            }
            // Sanity check for a response for client instance.
            else if (((vsomeip::message_type_e::MT_RESPONSE == msgType) ||
                (vsomeip::message_type_e::MT_ERROR == msgType)) &&
                !(methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
                mySomeipClient.VSOMEIPRespHandler(msg);
                return;
            }
            // Sanity check for an event for client instance.
            else if ((vsomeip::message_type_e::MT_NOTIFICATION == msgType) &&
                     (methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
                mySomeipClient.VSOMEIPEventHandler(msg);
                return;
            }
        }

        private:
            std::shared_ptr<vsomeip::application> app;
};

//--------------------------------------------------------------------------------------------------
/**
 * Add a routing entry for multicast address.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void AddRoutingForMulticast
(
    const char* multicastAddr,      /// [IN] multicast address (eg. "224.0.0.1")
    const char* ifName              /// [IN] interface name (eg. "eth0")
)
{
    LE_ASSERT(multicastAddr != NULL);
    LE_ASSERT(ifName != NULL);

    char* argumentsPtr[6];
    char addr[64] = {0};
    char intf[64] = {0};
    snprintf(addr, sizeof(addr), "%s", multicastAddr);
    snprintf(intf, sizeof(intf), "%s", ifName);

    argumentsPtr[0] = (char*)"/sbin/route";
    argumentsPtr[1] = (char*)"add";
    argumentsPtr[2] = addr;
    argumentsPtr[3] = (char*)"dev";
    argumentsPtr[4] = intf;
    argumentsPtr[5] = NULL;

    le_proc_Parameters_t proc =
    {
        .executableStr   = "/sbin/route",
        .argumentsPtr    = argumentsPtr,
        .environmentPtr  = NULL,
        .detach          = false,
        .closeFds        = LE_PROC_NO_FDS,
        .init            = NULL,
        .userPtr         = NULL
    };

    pid_t pid = le_proc_Execute(&proc);
    if (pid < 0)
    {
        LE_FATAL("Failed to set routing(error %d).", errno);
    }

    int status;
    if (waitpid(pid, &status, 0) > 0)
    {
        LE_INFO("%s[%d] returned %d", proc.executableStr, (int) pid, status);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * VSOMEIP dedicated entry thread.
 */
//--------------------------------------------------------------------------------------------------
static void* VSOMEIPThread
(
    void* contextPtr
)
{
    taf_vsomeipApp vsomeip;
    if (vsomeip.init())
    {
        LE_INFO("VSOMEIP thread started.");
        taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
        taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
        mySomeipSvr.VSOMEIPInit(vsomeip.getApp());
        mySomeipClient.VSOMEIPInit(vsomeip.getApp());
        // Notifies the main thread that the VSOMEIP stack is ready.
        le_sem_Post(mySomeipSvr.InitSem);
        le_sem_Post(mySomeipClient.InitSem);
        vsomeip.start();
    }

    LE_INFO("VSOMEIP thread exited.");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF SOME/IP GW service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    mySomeipSvr.Init();
    mySomeipClient.Init();

    le_thread_Ref_t vsomeipThreadRef = le_thread_Create("vsomeip", VSOMEIPThread, NULL);
    le_thread_Start(vsomeipThreadRef);
    le_clk_Time_t timeToWait = {10, 0};
    if ((LE_OK != le_sem_WaitWithTimeOut(mySomeipSvr.InitSem, timeToWait)) ||
        (LE_OK != le_sem_WaitWithTimeOut(mySomeipClient.InitSem, timeToWait)))
    {
        LE_FATAL("Failed to initialize TelAF SOME/IP Gateway Service.");
    }

    LE_INFO("TelAF SOME/IP GateWay Service initialized.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the reference to a server service instance.
 *
 * @return
 *     - Reference to the serivce instance.
 *     - NULL if not allowed to represent the service.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_ServiceRef_t taf_someipSvr_GetService
(
    uint16_t serviceId,
        ///< [IN] Service ID
    uint16_t instanceId
        ///< [IN] Instance ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetServiceRef(serviceId, instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the major and minor version for the server service instance. The default major version = 0x00
 * minor version = 0x00000000.
 *
 * NOTE: This API must be called before OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetServiceVersion
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint8_t majVer,
        ///< [IN] Major Version
    uint32_t minVer
        ///< [IN] Minor Version
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetServiceVersion(serviceRef, majVer, minVer);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set up an UDP server and/or a TCP server with given port(s) for the server service instance.
 *
 * NOTE: This API must be called before OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetServicePort
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t udpPort,
        ///< [IN] Set up an UDP server with the given UDP port for the
    uint16_t tcpPort,
        ///< [IN] Set up an TCP server with the given TCP port for the
        ///< service. 0 means no TCP server is set up.
    bool enableMagicCookies
        ///< [IN] If TCP magic cookie is enabled when TCP server is set
        ///< up.
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetServicePort(serviceRef, udpPort, tcpPort, enableMagicCookies);
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer a server service instance. The TCP port or/and UDP port must be set if the service need to
 * be offered remotely, otherwise it's offered locally.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_OfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
        ///< [IN] Service Reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.OfferService(serviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop offering a server service instance. It also stops offering all events of the service.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_StopOfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
        ///< [IN] Service Reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.StopOfferService(serviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the event type for an event of a specified service.
 *
 * NOTE: The default type is ET_EVENT and the API must be called before OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetEventType
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    taf_someipDef_EventType_t eventType
        ///< [IN] Event Type
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetEventType(serviceRef, eventId, eventType);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the cycle time for an periodic event notification(ET_EVENT) of a specified service. By
 * default the cycle time is 0 which means periodic event notification is disabled.
 *
 * NOTE: This API has no effect for field notification(ET_FIELD) and must be called before
 * OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetEventCycleTime
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    uint32_t cycleTime
        ///< [IN] Cycle Time in milliseconds
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetEventCycleTime(serviceRef, eventId, cycleTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable an event and add the event to an event group.
 *
 * NOTE: This API must be called before offerEvent() and can be called for multiple times if the
 * event need to be added into more than one event group.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_EnableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    uint16_t eventgroupId
        ///< [IN] Event Group ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.EnableEvent(serviceRef, eventId, eventgroupId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable an event and remove the event from all event groups.
 *
 * NOTE: This API must be called before OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_DisableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.DisableEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer an event which is already enabled for a server service instance.
 *
 * NOTE: This API must be called after OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_OfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.OfferEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop offering an event for a server service instance.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_StopOfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.StopOfferEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Fire an event or field notification. The specified event is updated with the specified payload
 * Data. Dependent on the type of the event, the payload is distributed to all notified clients
 * (always for events, only if the payload has changed for fields).
 *
 * NOTE: This API is only available after offerService() and OfferEvent().
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_Notify
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    const uint8_t* dataPtr,
        ///< [IN] Payload Data
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.Notify(serviceRef, eventId, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipSvr_RxMsg'
 *
 * This event provides information on Rx message.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_RxMsgHandlerRef_t taf_someipSvr_AddRxMsgHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    taf_someipSvr_RxMsgHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.AddRxMsgHandler(serviceRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipSvr_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipSvr_RemoveRxMsgHandler
(
    taf_someipSvr_RxMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.RemoveRxMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipSvr_Subscription'
 *
 * This event provides information on event group subscription.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_SubscriptionHandlerRef_t taf_someipSvr_AddSubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event Group ID.
    taf_someipSvr_SubscriptionHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.AddSubscriptionHandler(serviceRef, eventGroupId, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipSvr_Subscription'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipSvr_RemoveSubscriptionHandler
(
    taf_someipSvr_SubscriptionHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.RemoveSubscriptionHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the service ID and Instance ID of the Rx Message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetSerivceId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message Reference
    uint16_t* serviceIdPtr,
        ///< [OUT] Service ID
    uint16_t* instanceIdPtr
        ///< [OUT] Instance ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetSerivceId(msgRef, serviceIdPtr, instanceIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the method ID of the Rx message.
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetMethodId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint16_t* methodIdPtr
        ///< [OUT] Method ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetMethodId(msgRef, methodIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client ID of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetClientId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint16_t* clientIdPtr
        ///< [OUT] Client ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetClientId(msgRef, clientIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message type of the Rx message. The possible message type can be MT_REQUEST(0x00) or
 * MT_REQUEST_NO_RETURN(0x01).
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetMsgType
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint8_t* msgTypePtr
        ///< [OUT] Message Type
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetMsgType(msgRef, msgTypePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload size of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetPayloadSize
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint32_t* payloadSizePtr
        ///< [OUT] Payload Size in bytes
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetPayloadSize(msgRef, payloadSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload data of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetPayloadData
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint8_t* dataPtr,
        ///< [OUT] Payload Data
    size_t* dataSizePtr
        ///< [INOUT]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetPayloadData(msgRef, dataPtr, dataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a response message with message type MT_RESPONSE(0x80) for a Rx message.
 *
 * NOTE: Only the Rx message with message type MT_REQUEST(0x00) is allowed to send a response.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SendResponse
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    bool isErrRsp,
        ///< [IN] If true set message type to MT_ERROR(0x81)
    uint8_t returnCode,
        ///< [IN] Return Code
    const uint8_t* dataPtr,
        ///< [IN] Payload Data
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SendResponse(msgRef, isErrRsp, returnCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_ReleaseRxMsg
(
    taf_someipSvr_RxMsgRef_t msgRef
        ///< [IN] Rx Message reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.ReleaseRxMsg(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the SOME/IP client ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_someipClnt_GetClientId
(
    void
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.GetClientId();
}
//--------------------------------------------------------------------------------------------------
/**
 * Requests a client-service-instance to connect the service, and return the reference to the client
 * -service-instance.
 *
 * @return
 *     - Reference to the client-service-instance.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_ServiceRef_t taf_someipClnt_RequestService
(
    uint16_t serviceId,
        ///< [IN] Service ID.
    uint16_t instanceId
        ///< [IN] Instance ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestService(serviceId, instanceId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Releases a client-service-instance to disconnect the service. This also clears all pending
 * messages, unsubscribe event groups and remove all registered handlers for the client application.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_ReleaseService
(
    taf_someipClnt_ServiceRef_t serviceRef
        ///< [IN] Service Reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.ReleaseService(serviceRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the service state.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_GetState
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    taf_someipClnt_State_t* statePtr
        ///< [OUT] Service State if return LE_OK.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.GetState(serviceRef, statePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the service version.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_UNAVAILABLE -- Service is unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_GetVersion
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint8_t* majVerPtr,
        ///< [OUT] Major Version of the service if return LE_OK.
    uint32_t* minVerPtr
        ///< [OUT] Minor Version of the service if return LE_OK.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.GetVersion(serviceRef, majVerPtr, minVerPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipClnt_StateChange'
 *
 * This event provides information on service state change.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_StateChangeHandlerRef_t taf_someipClnt_AddStateChangeHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    taf_someipClnt_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.AddStateChangeHandler(serviceRef, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipClnt_StateChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RemoveStateChangeHandler
(
    taf_someipClnt_StateChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RemoveStateChangeHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates a request message and set the destination.
 *
 * @return
 *     - Reference to the request message.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_TxMsgRef_t taf_someipClnt_CreateMsg
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t methodId
        ///< [IN] Method ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.CreateMsg(serviceRef, methodId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the request to a non-return-request(MT_REQUEST_NO_RETURN). By default it's MT_REQUEST.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetNonRet
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetNonRet(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Uses TCP to send the request. By default is using UDP.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetReliable
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetReliable(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets timeout milliseconds waiting for the response. By default the timeout is 0(forever).
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetTimeout
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    uint32_t timeOut
        ///< [IN] Timeout in milliseconds.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetTimeout(msgRef, timeOut);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets payload data of the request. By default the payload is empty.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetPayload
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    const uint8_t* dataPtr,
        ///< [IN] Payload Data.
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetPayload(msgRef, dataPtr, dataSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Delete a request message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_DeleteMsg
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.DeleteMsg(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Send an asynchronous request message. The response handler will be called once the response is
 * received or any errors occur.
 *
 * NOTE: The request message will be automatically deleted after calling this API.
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RequestResponse
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    taf_someipClnt_RespMsgHandlerFunc_t handlerPtr,
        ///< [IN] Response message handler.
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestResponse(msgRef, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Enable an event group by adding an event into the group.
 *
 * NOTE: This API can be called for multiple times if there are more than one events adding into the
 * group. Currently one event can be only added into one group, and one event group can be enabled
 * only by one client.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is subscribed or already enabled by another client,
 *       or the event is already added into another group.
 *     - LE_DUPLICATE -- The event is already added into this group.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_EnableEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event Group ID.
    uint16_t eventId,
        ///< [IN] Event ID.
    taf_someipDef_EventType_t eventType
        ///< [IN] Event Type.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.EnableEventGroup(serviceRef, eventGroupId, eventId, eventType);
}
//--------------------------------------------------------------------------------------------------
/**
 * Disable an event group by removing all of the events from the group.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is subscribed or already enabled by another client.
 *     - LE_DUPLICATE -- The event group is not enabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_DisableEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.DisableEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Subscribe an event group of a service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is not enabled or already enabled by another client.
 *     - LE_DUPLICATE -- The event group is already subscribed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SubscribeEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SubscribeEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Unsubscribe an event group of a service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is not enabled or already enabled by another client.
 *     - LE_DUPLICATE -- the event group is not subscribed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_UnsubscribeEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.UnsubscribeEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipClnt_EventMsg'
 *
 * This event provides information on event message.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_EventMsgHandlerRef_t taf_someipClnt_AddEventMsgHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event group ID.
    taf_someipClnt_EventMsgHandlerFunc_t handlerPtr,
        ///< [IN] Event Handler.
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.AddEventMsgHandler(serviceRef, eventGroupId, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipClnt_EventMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RemoveEventMsgHandler
(
    taf_someipClnt_EventMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RemoveEventMsgHandler(handlerRef);
}

