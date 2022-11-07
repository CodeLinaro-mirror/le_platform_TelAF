/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSomeipSvr.hpp"
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
            app->register_state_handler(std::bind(&taf_vsomeipApp::on_state,
                                        this, std::placeholders::_1));

            app->register_message_handler(vsomeip::ANY_SERVICE,
                                          vsomeip::ANY_INSTANCE,
                                          vsomeip::ANY_METHOD,
                                          std::bind(&taf_vsomeipApp::on_message,
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
        void on_state(vsomeip::state_type_e state)
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
        void on_message(const std::shared_ptr<vsomeip::message> &msg)
        {
            vsomeip::message_type_e msgType = msg->get_message_type();
            vsomeip::length_t msgLen = msg->get_payload()->get_length();
            vsomeip::method_t methodId = msg->get_method();
            LE_DEBUG("Receive a message(len=%" PRIu32 ")with Client/Session/Type [0x%x/0x%x/0x%x].",
                     msgLen, msg->get_client(), msg->get_session(), (uint32_t)msgType);

            // Sanity check for request message for server instance.
            if (((vsomeip::message_type_e::MT_REQUEST == msgType) ||
                (vsomeip::message_type_e::MT_REQUEST_NO_RETURN == msgType)) &&
                (msgLen <= TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE) &&
                !(methodId & 0x8000))

            {
                taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
                mySomeipSvr.VSOMEIPHandler(msg);
                return;
            }
        }

        private:
            std::shared_ptr<vsomeip::application> app;
};

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
        mySomeipSvr.VSOMEIPInit(vsomeip.getApp());

        // Notifies the main thread that the VSOMEIP stack is ready.
        le_sem_Post(mySomeipSvr.InitSem);
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
    mySomeipSvr.Init();

    le_thread_Ref_t vsomeipThreadRef = le_thread_Create("vsomeip", VSOMEIPThread, NULL);
    le_thread_Start(vsomeipThreadRef);
    le_clk_Time_t timeToWait = {10, 0};
    if (LE_OK != le_sem_WaitWithTimeOut(mySomeipSvr.InitSem, timeToWait))
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
