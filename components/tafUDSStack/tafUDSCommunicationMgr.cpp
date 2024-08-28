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

#include <string>
#include "tafUDSCommunicationMgr.hpp"
#include "legato.h"
#include "interfaces.h"
#include "configuration.hpp"

using namespace telux::tafsvc;
using namespace std;
using namespace taf::uds;

bool UdsCommunicationMgr::isResetInProgress = false;
/*=================================================================================================
 FUNCTION        UdsCommunicationMgr::GetInstance
 DESCRIPTION     Get an instance of UdsCommunicationMgr
 PARAMETERS      void
 RETURN VALUE    UdsCommunicationMgr: Instance reference
=================================================================================================*/
UdsCommunicationMgr &UdsCommunicationMgr::GetInstance()
{
    static UdsCommunicationMgr instance;

    return instance;
}

UdsCommunicationMgr::UdsCommunicationMgr()
{}

UdsCommunicationMgr::~UdsCommunicationMgr()
{}

/**
 * UDS stack Communication Manager Init.
 */
void UdsCommunicationMgr::Init
(
)
{
    LE_INFO("UDS communication manager init.");

    semRef = le_sem_Create("SemRef", 0);

    udsHandlerRefMap = le_ref_CreateMap("udsHandlerRefMap", TAF_UDS_HANDLER_REF_CNT);

    udsTimerEventId = le_event_CreateId("Uds Timer Event", sizeof(udsTimerEvent_t));

    // Create timer thread.
    le_thread_Ref_t udsTimerThreadRef = le_thread_Create("udsTimerTh", UdsTimerThread, NULL);

    le_thread_Start(udsTimerThreadRef);
    le_sem_Wait(semRef);

    LE_INFO("UDS communication manager ok.");
    return;
}

void UdsCommunicationMgr::UdsTimerHandler(void* reqPtr)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    udsTimerEvent_t* eventReq = (udsTimerEvent_t*)reqPtr;

    if(eventReq == NULL)
    {
        LE_ERROR ("Invalid Parameters passed");
        return;
    }

    LE_DEBUG("Timer event:%d", (int)(eventReq->event));

    switch (eventReq->event) {
        case TAF_UDS_S3_TIMER_STOP:
            if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
            {
                LE_DEBUG("Stop S3 timer");
                le_timer_Stop(udsCmMgr.s3TimerRef);
            }
            break;

        case TAF_UDS_S3_TIMER_START:
            le_timer_SetMsInterval(udsCmMgr.s3TimerRef, eventReq->interval);
            if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
            {
                LE_DEBUG("Restart S3 timer");
                le_timer_Restart(udsCmMgr.s3TimerRef);
            }
            else
            {
                LE_DEBUG("Start S3 timer");
                le_timer_Start(udsCmMgr.s3TimerRef);
            }
            break;

        case TAF_UDS_S3_TIMER_RESTART:
            if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
            {
                LE_DEBUG("Restart S3 timer");
                le_timer_SetMsInterval(udsCmMgr.s3TimerRef, eventReq->interval);
                le_timer_Restart(udsCmMgr.s3TimerRef);
            }
            break;

        case TAF_UDS_P2STAR_TIMER_STOP:
            if(le_timer_IsRunning(udsCmMgr.p2StarTimerRef))
            {
                LE_DEBUG("Stop P2* timer");
                le_timer_Stop(udsCmMgr.p2StarTimerRef);
            }
            break;
        case TAF_UDS_P2STAR_TIMER_START:
            LE_DEBUG("Start P2* timer");
            {
                uint32_t p2StarServerInterval, maxNumberOfRcrrp;
                //Get P2* server interval;
                try
                {
                    cfg::Node & node = cfg::top_diagnostic_session<int>("id",
                            (int)udsCmMgr.SessionType);
                    p2StarServerInterval = node.get<uint32_t>("p2_star_server_max") * 1000;//To msec
                    LE_DEBUG("p2StarServerInterval : %d", p2StarServerInterval);
                    if(p2StarServerInterval > UDS_P2_STAR_SERVER_MAX ||
                            p2StarServerInterval < UDS_P2_STAR_SERVER_MIN)
                    {
                        p2StarServerInterval = UDS_P2_STAR_SERVER;
                        LE_ERROR("p2_star_server_max is incorrect. Use default value:%dms",
                                p2StarServerInterval);
                    }
                }
                catch (const std::exception& e)
                {
                    p2StarServerInterval = UDS_P2_STAR_SERVER;
                    LE_ERROR("Exception: %s. Use default value: %dms", e.what() ,
                            p2StarServerInterval);
                }
                //Get P2* server count
                try
                {
                    cfg::Node & root = cfg::get_root_node();
                    cfg::Node & common = root.get_child("common_props");
                    maxNumberOfRcrrp = common.get<uint32_t>(
                            "max_number_of_request_correctly_received_response_pending");
                    LE_DEBUG("maxNumberOfRcrrp = %d", maxNumberOfRcrrp);
                }
                catch (const std::exception& e)
                {
                    maxNumberOfRcrrp = UDS_P2_STAR_SERVER_CNT;
                    LE_ERROR("Exception: %s. Use default value:%d", e.what() , maxNumberOfRcrrp);
                }

                le_timer_SetMsInterval(udsCmMgr.p2StarTimerRef,
                        p2StarServerInterval - DELTA_UDS_P2_RESP);
                le_timer_SetRepeat(udsCmMgr.p2StarTimerRef, maxNumberOfRcrrp);
                le_timer_Start(udsCmMgr.p2StarTimerRef);
            }
            break;
        case TAF_UDS_P2STAR_TIMER_RESTART:
            break;
        default:
            LE_ERROR("Undefined event received.");
            break;
    }
}

void UdsCommunicationMgr::UdsTimerEventReport
(
    taf_UDSTimer_EventType_t timerEvent,
    uint32_t interval
)
{
    udsTimerEvent_t udsTimerEvt;
    udsTimerEvt.event = timerEvent;
    udsTimerEvt.interval = interval;

    le_event_Report(udsTimerEventId, &udsTimerEvt, sizeof(udsTimerEvent_t));
}

//Timer can only be started/stopped in the same thread. Create a thread to handle timer.
void* UdsCommunicationMgr::UdsTimerThread(void* ctxPtr)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    //P2 star timer
    udsCmMgr.p2StarTimerRef = le_timer_Create("UDSP2StarTimer");
    le_timer_SetHandler(udsCmMgr.p2StarTimerRef, P2StarTimeoutHandler);

    //S3 timer
    udsCmMgr.s3TimerRef = le_timer_Create("UDSS3Timer");

    le_timer_SetRepeat(udsCmMgr.s3TimerRef, 1);
    le_timer_SetHandler(udsCmMgr.s3TimerRef, S3TimeoutHandler);

    le_event_AddHandler("UDS Timer Event Handler", udsCmMgr.udsTimerEventId, UdsTimerHandler);

    le_sem_Post(udsCmMgr.semRef);

    LE_INFO("Create event loop for timer event");

    le_event_RunLoop();

    return NULL;
}

void UdsCommunicationMgr::P2StarTimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    uint32_t maxNumberOfRcrrp;

    LE_DEBUG("P2StarTimeoutHandler count = %d",le_timer_GetExpiryCount(timerRef));
    //Get P2* server count
    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        maxNumberOfRcrrp = common.get<uint32_t>(
                "max_number_of_request_correctly_received_response_pending");
        LE_DEBUG("maxNumberOfRcrrp = %d", maxNumberOfRcrrp);
    }
    catch (const std::exception& e)
    {
        maxNumberOfRcrrp = UDS_P2_STAR_SERVER_CNT;
        LE_ERROR("Exception: %s. Use default value:%d", e.what() , maxNumberOfRcrrp);
    }

    if(le_timer_GetExpiryCount(timerRef) < maxNumberOfRcrrp)
    {
        uint32_t sid = udsCmMgr.recvBuf[0];
        udsCmMgr.SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, &udsCmMgr.addrInfo);
        return;
    }

    LE_INFO("P2* timeout");
    udsCmMgr.readyToRecvData = true;
    memset(udsCmMgr.recvBuf, 0, UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = 0;
    udsCmMgr.sendDataLen = 0;
}

/*
 * When 'timeout' or 'disconnection' happen, change to DEFAULT session
 * and raise a indication to applications.
*/
void UdsCommunicationMgr::IndicateWhenChangingToDefault()
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    if (DEFAULT_SESSION == udsCmMgr.SessionType)
    {
        // No session change, just ignore
        return;
    }

    taf_SessionType_t oldSessionType = udsCmMgr.SessionType;
    udsCmMgr.SessionType = DEFAULT_SESSION;
    udsCmMgr.reqSeedLevel = 0;
    udsCmMgr.securityLevel = 0;

    taf_doip_DiagMsg_t sesChangeMsg;
    if(udsCmMgr.udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not find handler to notify session change");
        return;
    }

    taf_UDSIndicationHandler_t* udsHandler =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsCmMgr.udsHandlerRefMap,
                    udsCmMgr.udsIndicationHandler.safeRef);

    if(udsHandler == NULL || udsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not find handler to notify session change");
        return;
    }

    udsCmMgr.sesChangeBuf[0] = udsCmMgr.sesChangeId;
    udsCmMgr.sesChangeBuf[1] = oldSessionType;
    udsCmMgr.sesChangeBuf[2] = udsCmMgr.SessionType;
    sesChangeMsg.dataPtr = udsCmMgr.sesChangeBuf;
    sesChangeMsg.dataLen = UDS_SESSION_CHANGE_DATA_SIZE;

    // Indicate session change to the application.
    udsHandler->funcPtr(&(udsCmMgr.addrInfo), &sesChangeMsg,
                        TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
}

void UdsCommunicationMgr::S3TimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Session time out");
    IndicateWhenChangingToDefault();
}

void UdsCommunicationMgr::CheckAndRestartS3Timer
(
    uint8_t serviceId
)
{
    uint32_t p2StarServerInterval, maxNumberOfRcrrp, s3ServerInterval;

    UdsTimerEventReport(TAF_UDS_P2STAR_TIMER_STOP, 0);
    readyToRecvData = true;

    //Get P2* server interval;
    try
    {
        cfg::Node & node = cfg::top_diagnostic_session<int>("id", (int)SessionType);
        p2StarServerInterval = node.get<uint32_t>("p2_star_server_max") * 1000;//Sec to msec.
        LE_DEBUG("p2StarServerInterval : %d", p2StarServerInterval);
        if(p2StarServerInterval > UDS_P2_STAR_SERVER_MAX)
        {
            p2StarServerInterval = UDS_P2_STAR_SERVER;
            LE_ERROR("p2_star_server_max > maxmimal value. Use default value:%dms",
                    p2StarServerInterval);
        }
    }
    catch (const std::exception& e)
    {
        p2StarServerInterval = UDS_P2_STAR_SERVER;
        LE_ERROR("Exception: %s. Use default value:%dms", e.what(), p2StarServerInterval);
    }
    //Get P2* server count
    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        maxNumberOfRcrrp = common.get<uint32_t>(
                "max_number_of_request_correctly_received_response_pending");
        LE_DEBUG("maxNumberOfRcrrp = %d", maxNumberOfRcrrp);
    }
    catch (const std::exception& e)
    {
        maxNumberOfRcrrp = UDS_P2_STAR_SERVER_CNT;
        LE_ERROR("Exception: %s. Use default value:%d", e.what(), maxNumberOfRcrrp);
    }
    //Get S3* server interval
    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        s3ServerInterval = ((uint32_t)common.get<float>("s3_server_max")) * 1000; //Sec to msec.
        LE_DEBUG("s3ServerInterval = %d", s3ServerInterval);
    }
    catch (const std::exception& e)
    {
        s3ServerInterval = UDS_S3_SERVER;
        LE_ERROR("Exception: %s. Use default value:%dms", e.what(), s3ServerInterval);
    }

    //If S3 timer is running, restart it with smaller interval.
    if((serviceId != SESSION_CONTROL_REQUEST_ID) && (s3ServerInterval <
            p2StarServerInterval*maxNumberOfRcrrp))
    {
        UdsTimerEventReport(TAF_UDS_S3_TIMER_RESTART, s3ServerInterval);
    }
}

static taf_doip_PowerMode_t PowerModeQueryHandler
(
    void* userPtr
)
{
    LE_DEBUG("PowerModeQueryHandler");

    return TAF_DOIP_POWER_MODE_NOT_SUPPORTED;
}

/**
 * Start UDS stack.
 */
le_result_t UdsCommunicationMgr::UdsStart
(
    const char* configPathPtr
)
{
    LE_INFO("UDS stack start.");

    le_result_t ret;

    if(configPathPtr == NULL)
    {
        LE_FATAL("configPathPtr is Null");
        return LE_FAULT;
    }

    LE_INFO("Start UDS server with config file %s", configPathPtr);

    //Init and start DoIP to enable the ability of sending/receiving data packets
    if (DoipEntityRef == NULL)
    {
        DoipEntityRef = taf_doip_Create(configPathPtr);
        if(DoipEntityRef == NULL)
        {
            LE_FATAL("Failed to create DoIP Entity");
            return LE_FAULT;
        }
    }

    ret = taf_doip_Start(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_FATAL("Failed to start DoIP Entity(%d)", ret);
        return LE_FAULT;
    }

    PmQueryRef = taf_doip_AddPowerModeQueryHandler(DoipEntityRef,
        PowerModeQueryHandler, NULL);
    if (PmQueryRef == NULL)
    {
        LE_FATAL("Failed to register power mode query handler");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Pack NRC.
 */
le_result_t UdsCommunicationMgr::SetNRC
(
    uint8_t sid,
    uint8_t errorCode
)
{
    LE_DEBUG("SetNRC, sid= 0x%x, error code=0x%x",sid, errorCode);

    // pack the NRC data
    sendBuf[0] = UDS_NEGATIVE_RESP_SID;
    sendBuf[1] = sid;
    sendBuf[2] = errorCode;
    sendDataLen = UDS_NEG_RESP_LEN;

    return LE_OK;
}

/**
 * Send NRC.
 */
le_result_t UdsCommunicationMgr::SendNRC
(
    uint8_t sid,
    uint8_t errorCode,
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("SendNRC, sid= 0x%x, error code=0x%x",sid, errorCode);

    // pack the NRC data
    sendBuf[0] = UDS_NEGATIVE_RESP_SID;
    sendBuf[1] = sid;
    sendBuf[2] = errorCode;
    sendDataLen = UDS_NEG_RESP_LEN;

    SendData(addrInfoPtr);

    if(errorCode == REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING)
    {
        LE_DEBUG("RCRRP is sent");
    }

    return LE_OK;
}

void UdsCommunicationMgr::SendData
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    le_result_t ret;
    taf_doip_AddrInfo_t respAddrInfo;
    taf_doip_DiagMsg_t respDiagMsg;

    respAddrInfo.sa = addrInfoPtr->ta;
    respAddrInfo.ta = addrInfoPtr->sa;
    respAddrInfo.taType = addrInfoPtr->taType;
    respDiagMsg.dataPtr = sendBuf;
    respDiagMsg.dataLen = sendDataLen;

    ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
    if(ret == LE_OK)
    {
        LE_DEBUG("Send Diagnostic response successfully");
    }
    else
    {
        LE_ERROR("Failed to send Diagnostic response");
    }
}

/**
 * Indicate received ReadDataByIdentifier message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateReadDIDReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateReadDIDReq");

    uint16_t didNum = 0;
    uint16_t dataId = 0;
    uint8_t sid = recvBuf[0];
    uint8_t updatedRecvBuf[UDS_DATA_SIZE];
    uint16_t updatedRecvDataLen = 0;

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    *isInternalHandle = true;

    // Step 1: Minimum length check. UDS_0x22_NRC_13
    if(recvDataLen < UDS_READ_DID_REQ_MIN_LEN)
    {
        LE_WARN("recvDataLen is less than the ReadDID request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr); //NRC 0x13
    }

    // Modulo 2 division check. UDS_0x22_NRC_13
    if((recvDataLen -1) % 2 !=0)
    {
        LE_WARN("Modulo 2 division check error.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr); //NRC 0x13
    }

    // Step 2: Maximum length check. UDS_0x22_NRC_13
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_WARN("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr); //NRC 0x13
    }

    // Get the DID list in active session
    didNum = (recvDataLen -1)/2;
    updatedRecvBuf[0] = sid;
    updatedRecvDataLen = sizeof(sid);
    for(uint16_t i = 0; i < didNum; i++)
    {
        dataId = ((recvBuf[i*UDS_DID_LEN + 1]) << 8) + recvBuf[i*UDS_DID_LEN + 2];

        cfg::Node node;
        try
        {
            // Get DID node.
            node = cfg::top_did_all<uint16_t>("identification.code", dataId);
        }
        catch (const std::exception& e)
        {
            LE_WARN("Exception: %s. dataId:0x%x is not configured", e.what(), dataId);
            //DID is not configured, don't response it.
            continue;
        }

        try
        {
            // Get read_did attribute.
            bool idDidReadable = node.get_child("supported_functions").get<bool>("read_did");
            if(!idDidReadable)
            {
                LE_DEBUG("dataId:0x%x is not readable",  dataId);
                continue;
            }
        }
        catch (const std::exception& e)
        {
            LE_WARN("Exception: %s. read_did is not configured for dataId:0x%x", e.what(), dataId);
            //DID is not configured, don't response it.
            continue;
        }

        try
        {
            // Check active session type for data ID.
            cfg::Node & sesType = node.get_child("access.session");
            bool isSessionMatched = false;
            string type;
            for (const auto & session: sesType)
            {
                type = session.second.get_value<string>("");

                cfg::Node & sesNode = cfg::top_diagnostic_session<string>("short_name", type);
                int confSessionId = sesNode.get<int>("id");
                if ((uint8_t)confSessionId == (uint8_t)SessionType)
                {
                    LE_DEBUG("current session type is supported for this data ID");
                    isSessionMatched = true;
                    break;
                }
            }

            if(!isSessionMatched)
            {
                LE_DEBUG("current session type is not supported for dataId:0x%x.", dataId);
                //Current session is not configured, don't response it.
                continue;
            }

            string accessibilitySession = "did_accessibility.diagnostic_session." + type;
            cfg::Node & rwAttributes = node.get_child(accessibilitySession);

            // Check active session type for data ID.
            bool isRWTypeMatched = false;
            for (const auto & rw: rwAttributes)
            {
                string attr = rw.second.get_value<string>("");
                if(attr == "R")
                {
                    LE_DEBUG("diagnostic_session is R");
                    isRWTypeMatched = true;
                    break;
                }
            }

            if(!isRWTypeMatched)
            {
                LE_DEBUG("current session attribute is not read for dataId:0x%x.", dataId);
                //Current session is not read for this DID, don't response it.
                continue;
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception: %s. session/diagnostic_session is not configured for dataId:0x%x.",
                    e.what(), dataId);
            //session or is diagnostic_session not configured for this DID, don't response it.
            continue;
        }

        //Step 3: Authentication check and Security access check
        try
        {
            int security_type = node.get_child("access").get<int>("security_level");
            //Authentication check after authentication service is supported, send NRC 0x34
            //Security access check. UDS_0x22_NRC_33
            if(security_type == SECURITY_ACCESS_REQUEST_ID && securityLevel == 0)
            {
                LE_WARN("Did is secured and the server is not unlocked.");
                return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
            }
        }
        catch (const std::exception& e)
        {
            //security_level is not configured. Don't check it.
            LE_WARN("Exception: %s. security_level is not configured for dataId 0x%x", e.what(),
                    dataId);
        }

        //Store DID in active session.
        updatedRecvBuf[updatedRecvDataLen] = recvBuf[i*UDS_DID_LEN + 1];
        updatedRecvBuf[updatedRecvDataLen+1] = recvBuf[i*UDS_DID_LEN + 2];
        updatedRecvDataLen = updatedRecvDataLen + 2;
    }

    //Step 4: Check if at least one DID is supported in the active session. UDS_0x22_NRC_31
    if(updatedRecvDataLen == sizeof(sid))
    {
        LE_WARN("None of DIDs is supported in the active session.");
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    //Only response valid DIDs
    memcpy(recvBuf, updatedRecvBuf, updatedRecvDataLen);
    recvDataLen = updatedRecvDataLen;

    //Will send indication to the diag service
    *isInternalHandle = false;

    return LE_OK;
}

/**
 * Indicate received WriteDataByIdentifier message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateWriteDIDReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateWriteDIDReq");
    uint16_t dataId = 0;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    *isInternalHandle = true;

    // Step 1: Minimum length check. UDS_0x2E_NRC_13
    if(recvDataLen < UDS_WRITE_DID_REQ_MIN_LEN)
    {
        LE_WARN("recvDataLen is less than the WriteDID request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Step 2: Active session check. UDS_0x2E_NRC_31
    dataId = ((recvBuf[1]) << 8) + recvBuf[2];
    cfg::Node node;

    try
    {
        // Get DID node.
        node = cfg::top_did_all<uint16_t>("identification.code", dataId);
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. dataId:0x%x is not configured", e.what(), dataId);
        //DID is not configured.
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    try
    {
        // Get read_did attribute.
        bool idDidWriteable = node.get_child("supported_functions").get<bool>("write_did");
        if(!idDidWriteable)
        {
            LE_WARN("dataId:0x%x is not writable",  dataId);
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. write_did is not configured for dataId:0x%x", e.what(), dataId);
    }

    try
    {
        // Check active session type for data ID.
        cfg::Node & sesType = node.get_child("access.session");
        bool isSessionMatched = false;
        string type;
        for (const auto & session: sesType)
        {
            type = session.second.get_value<string>("");

            cfg::Node & sesNode = cfg::top_diagnostic_session<string>("short_name", type);
            int confSessionId = sesNode.get<int>("id");
            if ((uint8_t)confSessionId == (uint8_t)SessionType)
            {
                LE_DEBUG("current session type is supported for this data ID");
                isSessionMatched = true;
                break;
            }
        }

        if(!isSessionMatched)
        {
            LE_WARN("current session type is not supported for dataId:0x%x.", dataId);
            //Current session is not configured, don't response it.
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
        }

        string accessibilitySession = "did_accessibility.diagnostic_session." + type;
        LE_INFO("accessibilitySession =%s", accessibilitySession.c_str());
        cfg::Node & rwAttributes = node.get_child(accessibilitySession);

        // Check active session type for data ID.
        bool isRWTypeMatched = false;
        for (const auto & rw: rwAttributes)
        {
            string attr = rw.second.get_value<string>("");
            if(attr == "W")
            {
                LE_DEBUG("Support w");
                isRWTypeMatched = true;
                break;
            }
        }

        if(!isRWTypeMatched)
        {
            LE_WARN("current session attribute is not written for dataId:0x%x.", dataId);
            //Current session is not written for this DID, don't response it.
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. session/diagnostic_session is not configured for dataId:0x%x.",
                e.what(), dataId);
    }

    // Step 3: Maximum length check. UDS_0x2E_NRC_13
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_WARN("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Step 4: Authentication check and Security access check
    try
    {
        int security_type = node.get_child("access").get<int>("security_level");
        //Authentication check after authentication service is supported, send NRC 0x34
        //Security access check. UDS_0x2E_NRC_33
        if(security_type == SECURITY_ACCESS_REQUEST_ID && securityLevel == 0)
        {
            LE_WARN("Did is secured and the server is not unlocked.");
            return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
        }
    }
    catch (const std::exception& e)
    {
        //security_level is not configured. Don't check it.
        LE_WARN("Exception: %s. security_level is not configured for dataId 0x%x", e.what(),
                dataId);
    }

    //Step 5: Data record check. UDS_0x2E_NRC_31
    try
    {
        int dataRecordSize = node.get_child("implementation").get<int>("did_size");
        //Only check size here, will check data later.
        if(dataRecordSize != (recvDataLen - UDS_WRITE_DID_REQ_BASE_LEN))
        {
            LE_WARN("Data record size is invalid");
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
        }
    }
    catch (const std::exception& e)
    {
        //security_level is not configured. Don't check it.
        LE_WARN("Exception: %s. did_size is not configured for dataId 0x%x", e.what(), dataId);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;

    return LE_OK;
}

/**
 * Indicate received SessionCtrl message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateSessionCtrlReq
(
    taf_doip_AddrInfo_t* addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateSessionCtrlReq");

    // Received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall be 2
    // Step 2: Minimum length check. UDS_0x10_NRC_13
    if(recvDataLen != UDS_SESSION_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the SessionCtrl request msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Received sesssion type.
    uint8_t sessionId = recvBuf[1] & 0x7F;

    // Step 3: Session type check. Exception if can't get node. UDS_0x10_NRC_12
    try
    {
        // Check if session type configured.
        cfg::Node sesNode = cfg::top_diagnostic_session<int>("id", sessionId);
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s", e.what());
        LE_DEBUG("Requested session type is not configured: 0x%x", sessionId);
        *isInternalHandle = true;
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr); // NRC 0x12
    }

    // copy addressInfo localy to use while sending session change indication.
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = addrInfoPtr->taType;

    //Will send indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Indicate received ECUReset message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateECUResetReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateECUResetReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check negative err code for request msg length
    if(recvDataLen != UDS_ECU_RESET_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ECUReset request msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    cfg::Node node;
    uint8_t subFunc = recvBuf[1] & 0x7f;
    try
    {
        node = cfg::top_reset_all<int>("sub_function_identifier", subFunc);
        LE_INFO("Reset type 0x%x is supported", subFunc);
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s", e.what());
        LE_DEBUG("ECUReset SubFunction0x%x is not supported in the server.", subFunc);
        *isInternalHandle = true;
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // Check security access for ECUReset.
    if (!IsSecurityAccessMatched(node))
    {
        LE_DEBUG("Security access is not matched for ECUReset.");
        *isInternalHandle = true;
        return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
    }

    // Check active session type for ECUReset.
    if (!IsSessTypeMatched(node))
    {
        LE_DEBUG("Session type is not matched for ECUReset.");
        *isInternalHandle = true;
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION, addrInfoPtr);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Indicate received Security access message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateSecAccessReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateSecAccessReq");

    // received service ID
    uint8_t sid = recvBuf[0];
    uint8_t subFunc;

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_SECURITY_ACCESS_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the SecurityAccess msg minimum length.");
        *isInternalHandle = true;
        // UDS_0x27_NRC_13: Less than minimum length
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        // UDS_0x27_NRC_13: More than UDS_DATA_SIZE
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    subFunc = recvBuf[1] & 0x7f;

    if(subFunc == 0x00 || (0x43 <= subFunc && subFunc >= 0x5E) || subFunc == 0x7f)
    {
        LE_DEBUG("subFunc is reserved.");
        *isInternalHandle = true;
        // UDS_0x27_NRC_12: The range is not supported
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // Check active session type for SecurityAccess.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for IndicateSecAccessReq.");
        *isInternalHandle = true;
        // UDS_0x27_NRC_22: Bad session you are in
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // requestSeed
    if (subFunc % 2 !=0)
    {
        reqSeedLevel = subFunc;
        //If already unlock with the same security level, send 0 as the seed.
        if(subFunc == securityLevel)
        {
            //send data 00
            // Fill the response data
            sendBuf[0] = SECURITY_ACCESS_RESPONSE_ID;
            sendBuf[1] = subFunc;

            try
            {
                int seedBitSize = cfg::get_root_node().get<int>("security_level.seed_size");
                int seedByteSize = (seedBitSize % 8) ? (seedBitSize / 8) + 1 : (seedBitSize / 8);
                LE_INFO("Seed bit_size: %d, byte_size:%d", seedBitSize, seedByteSize);

                sendDataLen = 2 + seedByteSize; // 2 = (SID + SUBFUNC)

                if (sendDataLen > UDS_DATA_SIZE)
                {
                    LE_ERROR("Response-Seed size if overflow (> %u)", UDS_DATA_SIZE);
                    *isInternalHandle = true;
                    // UDS_0x27_NRC_22: Response-Seed size if overflow
                    return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
                }

                memset(sendBuf + 2, 0x00, seedByteSize);
            }
            catch (const std::exception& e)
            {
                LE_ERROR("Failed to get the seed size from YAML configuration: %s", e.what());
                *isInternalHandle = true;
                // UDS_0x27_NRC_22: Failed to get the seed size from YAML configuration
                return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
            }

            sendDataLen = UDS_SECURITY_ACCESS_RESP_SEED_ZERO_LEN;

            //Send positive response
            SendData(addrInfoPtr);
            *isInternalHandle = true;
            return LE_OK;
        }
        else
        {
            //Will send the indication to the diag service
            *isInternalHandle = false;
            return LE_OK;
        }

    }
    // sendKey
    else
    {
        // Check negative err code for minimum sendkey request msg length
        if(recvDataLen < UDS_SECURITY_ACCESS_SEND_KEY_REQ_MIN_LEN)
        {
            LE_DEBUG("recvDataLen is less than the SecurityAccess sendkey msg minimum length.");
            *isInternalHandle = true;
            // UDS_0x27_NRC_13: Bad send key length
            return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
        }

        //Without first receiving a 'requestSeed' request message.
        if(reqSeedLevel == 0)
        {
            LE_DEBUG("Without first receiving a 'requestSeed' request message.");
            *isInternalHandle = true;
            // UDS_0x27_NRC_24: Received send-key before request-seed
            return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
        }

        //'SendKey' level shall equal the 'requestSeed' SubFunction parameter value plus one.
        if(subFunc != reqSeedLevel + 1)
        {
            LE_DEBUG("SendKey level shall equal the 'requestSeed' SubFunc param value plus one.");
            *isInternalHandle = true;
            // UDS_0x27_NRC_24: Request-seed + 1 != send-key level
            return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
        }

        try
        {
            int keyBitSize = cfg::get_root_node().get<int>("security_level.key_size");
            int keyByteSize = (keyBitSize % 8) ? (keyBitSize / 8) + 1 : (keyBitSize / 8);
            LE_INFO("Key bit_size: %d, byte_size:%d", keyBitSize, keyByteSize);

            if (recvDataLen - 2 != keyByteSize) // 2 = sizeof( SID + SUBFUNCTION )
            {
                LE_ERROR("Key size is not match YAML configuration");
                *isInternalHandle = true;
                // UDS_0x27_NRC_13: Key size is not match to YAML configuration
                return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Failed to get the key size from YAML configuration: %s", e.what());
            *isInternalHandle = true;
            // UDS_0x27_NRC_22: Failed to get the key size from YAML configuration
            return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
        }

        reqSeedLevel = 0;

        //Will send the indication to the diag service
        *isInternalHandle = false;
        return LE_OK;
    }

    return LE_OK;
}

/**
 * Indicate received InputOutputControlByIdentifier message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateIOCBIDReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{

    // received service ID
    uint8_t sid = recvBuf[0];
    uint16_t dataId = 0;
    uint8_t ioCtrlParam;

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    *isInternalHandle = true;
    // NRC checking
    // Step 1: Minimum length check. UDS_0x2F_NRC_13
    if(recvDataLen < UDS_IOCBID_REQ_MIN_LEN)
    {
        LE_WARN("recvDataLen is less than the IOCBID msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);//NRC 0x13
    }

    dataId = ((recvBuf[1]) << 8) + recvBuf[2];

    cfg::Node node;
    // Step 2: Data ID check. Exception if can't get node. UDS_0x2F_NRC_31
    try
    {
        node = cfg::top_IO_all<int>("identifier", dataId);
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. dataId 0x%x is not configured", e.what(), dataId);
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
    }

    // Step 3: Session check. UDS_0x2F_NRC_31
    try
    {
        cfg::Node & sesType = node.get_child("access.session");
        bool isSessionMatched = false;
        for (const auto & session: sesType)
        {
            string type = session.second.get_value<string>("");

            cfg::Node & sesNode = cfg::top_diagnostic_session<string>("short_name", type);
            int confSessionId = sesNode.get<int>("id");
            if ((uint8_t)confSessionId == (uint8_t)SessionType)
            {
                LE_DEBUG("current session type is supported for this data ID");
                isSessionMatched = true;
                break;
            }
        }

        if(!isSessionMatched)
        {
            LE_WARN("current session type is not supported for this data ID.");
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. access.session is not configured for dataId 0x%x", e.what(),
                dataId);
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
    }

    // Step 4.1: InputOutputControl parameter check. UDS_0x2F_NRC_31
    ioCtrlParam = recvBuf[3];
    try
    {
        cfg::Node & ioCtrlValues =
                node.get_child("request.control_option_record.io_control_parameter");
        bool isControlParamMatched = false;
        for (const auto & controlValue: ioCtrlValues)
        {
            int confIoCtrlValue = controlValue.second.get_value<int>();
            if(confIoCtrlValue == ioCtrlParam)
            {
                isControlParamMatched = true;
                break;
            }
        }

        if(!isControlParamMatched)
        {
            LE_WARN("InputOutputControl parameter(%d) is not supported.", ioCtrlParam);
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s. io_control_parameter in request is not configured for dataId 0x%x",
                e.what(), dataId);
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
    }

    // Step 4.2: ControlState parameter check for short term adjustment. UDS_0x2F_NRC_31
    if(ioCtrlParam == SHORT_TERM_ADJUSTMENT)
    {
        int bitSize = 0;
        uint16_t controlStateSize = 0;
        // Get the controlState size from config module

        cfg::Node dataNode;
        try
        {
            std::string ctrlState =
                    node.get<std::string>("request.control_option_record.control_state");

            dataNode = cfg::top_datas<std::string>("mnemonic", ctrlState);
            bitSize = dataNode.get<int>("functional_definition.bit_size");
            LE_DEBUG("Configured bitSize : %d", bitSize);

            if(bitSize % 8 == 0)
                controlStateSize = bitSize/8;
            else
                controlStateSize = (bitSize/8) + 1;

            //Check control state size.
            if(recvDataLen < controlStateSize + UDS_IOCBID_REQ_MIN_LEN)
            {
                LE_WARN("The received length is less than required.");
                return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);//UDS_0x2F_NRC_13
            }
        }
        catch (const std::exception& e)
        {
            LE_WARN("Exception: %s", e.what());
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
        }

        try
        {
            std::string dataType= dataNode.get<string>("functional_definition.data_type");
            if(dataType == "Numeric")
            {
                LE_INFO("dataType : Numeric");
                int min = dataNode.get<int>("functional_definition.min");
                int max = dataNode.get<int>("functional_definition.max");
                std::string valueType= dataNode.get<string>("functional_definition.value_type");
                if(valueType == "uint8" && controlStateSize == 1)
                {
                    uint8_t controlState = recvBuf[4];
                    uint8_t newMin = static_cast<uint8_t>(min);
                    uint8_t newMax = static_cast<uint8_t>(max);
                    if(controlState < newMin || controlState > newMax)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                newMin, newMax);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
                else if(valueType == "sint8" && controlStateSize == 1)
                {
                    int8_t controlState = recvBuf[4];

                    if(controlState < min || controlState > max)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                min, max);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
                else if(valueType == "uint16" && controlStateSize == 2)
                {
                    uint16_t controlState = ((recvBuf[4]) << 8) + recvBuf[5];

                    uint16_t newMin = static_cast<uint16_t>(min);
                    uint16_t newMax = static_cast<uint16_t>(max);
                    if(controlState < newMin || controlState > newMax)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                newMin, newMax);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
                else if(valueType == "sint16" && controlStateSize == 2)
                {
                    int16_t controlState = ((recvBuf[4]) << 8) + recvBuf[5];

                    if(controlState < min || controlState > max)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                min, max);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
                else if(valueType == "uint32" && controlStateSize == 4)
                {
                    uint32_t controlState = ((recvBuf[4]) << 24) + ((recvBuf[5]) << 16) +
                            ((recvBuf[6]) << 8) + recvBuf[7];

                    uint32_t newMin = static_cast<uint32_t>(min);
                    uint32_t newMax = static_cast<uint32_t>(max);
                    if(controlState < newMin || controlState > newMax)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                newMin, newMax);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
                else if(valueType == "sint32" && controlStateSize == 2)
                {
                    int32_t controlState = ((recvBuf[4]) << 24) + ((recvBuf[5]) << 16) +
                            ((recvBuf[6]) << 8) + recvBuf[7];

                    if(controlState < min || controlState > max)
                    {
                        LE_WARN("controlState(0x%x) is not in range(0x%x:0x%x)", controlState,
                                min, max);
                        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);//NRC 0x31
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception: %s", e.what());
        }
    }

    //Step 5: Total length check. UDS_0x2F_NRC_13
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_WARN("recvDataLen is more than UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr); // NRC 0x13
    }

    //Step 6: Authentication check and Security access check
    try
    {
        int security_type = node.get_child("access").get<int>("security_level");
        LE_INFO("security type=0x%x", security_type);
        //Authentication check after authentication service is supported, send NRC 0x34
        //Security access check. UDS_0x2F_NRC_33
        if(security_type == SECURITY_ACCESS_REQUEST_ID && securityLevel == 0)
        {
            LE_WARN("Did is secured and the server is not unlocked.");
            return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
        }
    }
    catch (const std::exception& e)
    {
        //security_level is not configured. Don't check it.
        LE_WARN("Exception: %s. security_level is not configured for dataId 0x%x", e.what(),
                dataId);
        *isInternalHandle = false;
        return LE_OK;
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Indicate received Routine control message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRoutinrCtrlReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRoutinrCtrlReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_ROUTINE_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RoutinrCtrlReq msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    uint16_t rid = ((recvBuf[2]) << 8) + recvBuf[3];
    cfg::Node node;
    try
    {
        // Check if RID supported in active session.
        node = cfg::top_routines_all<uint16_t>("identifier", rid);
    }
    catch (const std::exception& e)
    {
        LE_WARN("Exception: %s", e.what());
        LE_DEBUG("RID0x%x is not supported in the server.", rid);
        *isInternalHandle = true;
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    // Check active session type for RID.
    if (!IsSessTypeMatched(node))
    {
        LE_DEBUG("Session type is not matched for routine control.");
        *isInternalHandle = true;
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    if (!IsSecurityAccessMatched(node))
    {
        LE_DEBUG("RID0x%x is secured and the server is not unlocked.", rid);
        *isInternalHandle = true;
        return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
    }

    // Check if subfunction is supported for the RID.
    uint8_t subFunc = recvBuf[1] & 0x7F;
    if (!IsRequestSubFuncSupported(node, subFunc))
    {
        LE_DEBUG("SubFunction0x%x for RID0x%x is not supported in the server.",
            subFunc, rid);
        *isInternalHandle = true;
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

std::map<std::string, uint8_t>& UdsCommunicationMgr::GetSessionMap(void)
{
    static std::map<std::string, uint8_t> sessionMap = cfg::get_diagnostic_session_map();
    return sessionMap;
}

// FIXME: get the reference for the access speed

bool UdsCommunicationMgr::IsValidSvcActiveSession(uint8_t sid, taf_SessionType_t currentSession)
{
    try{

        cfg::Node & svcAllNode = cfg::get_root_node().get_child("services_all");
        cfg::Node & sessList = svcAllNode.get_child(std::to_string(sid) + ".access.session");

        std::map<std::string, uint8_t>& sessMap = GetSessionMap();

        std::vector<uint8_t> activeSessionList;

        for (auto &sess: sessList)
        {
            uint8_t sessId = sessMap[sess.second.get_value<std::string>()];
            activeSessionList.push_back(sessId);
        }

        if (std::find(activeSessionList.begin(),
                      activeSessionList.end(),
                      (uint8_t)currentSession) != activeSessionList.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Failed to get the 0x%02X's session from YAML configuration: %s", sid, e.what());
        return false; // Mark the exception as 'false'
    }
}


/**
 * Check NRC and Indicate received TransferData (0x36) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferDataReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRxXferDataReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_DATA_BASE_LEN)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is less than the RxXferDataReq msg minimum length.");
        *isInternalHandle = true;
        // UDS_0x36_NRC_13: Less than minimum length
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        // UDS_0x36_NRC_13: overflow UDS_DATA_SIZE
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check active session type for TransferData.
    if (! IsValidSvcActiveSession(sid, SessionType))
    {
        LE_DEBUG("Programming session type is not active for TransferData.");
        *isInternalHandle = true;
        // UDS_0x36_NRC_22: Not in programming session
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    if (!isXferActive)
    {
        *isInternalHandle = true;
        // UDS_0x36_NRC_24: Transfer is NOT in progress
        return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received RequestTransferExit (0x37) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferExitReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRxXferExitReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_EXIT_BASE_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RxXferExitReq msg minimum length.");
        *isInternalHandle = true;
        // UDS_0x37_NRC_13: Data less than minimum request msg len
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        // UDS_0x37_NRC_13: Data more than UDS_DATA_SIZE
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check active session type for RequestTransferExit.
    if (! IsValidSvcActiveSession(sid, SessionType))
    {
        LE_DEBUG("Programming session type is not active for RequestTransferExit.");
        *isInternalHandle = true;
        // UDS_0x37_NRC_22: Transfer exit is not in programming session
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    if (!isXferActive)
    {
        *isInternalHandle = true;
        // UDS_0x37_NRC_24: Transfer is not active
        return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check whether the file name is ASCII format.
 */
static bool isAcsiiFormat(const char * arr, size_t arrLen)
{
    for (size_t i = 0; i <= arrLen; i++)
    {
        if (arr[i] < 0 || arr[i] > 127)
        {
            return false;
        }
    }
    return true;
}

/**
 * Check NRC and Indicate received RequestFileTransfer (0x38) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxFileXferReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("In %s", __FUNCTION__);

    #define RTF_SID recvBuf[0]
    #define RFT_MOOP recvBuf[1]
    #define LENGTH_OF_FILE_NAME (((recvBuf[2]) << 8 ) | (recvBuf[3]))
    #define LENGTH_OF_FILE_SIZE(buffer, loc) (buffer[loc])
    #define isTransferInProgress() isXferActive

    LE_INFO("[RFT] Request for moop:[0x%02X]", RFT_MOOP);

    // Minimum length checking, the filePathAndNameLength >= 1
    if (recvDataLen < RFT_MIN_LEN || recvDataLen > UDS_DATA_SIZE)
    {
        LE_ERROR("Bad data size for 0x38");
        // UDS_0x38_NRC_13: Invalid msg length
        return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // The validity check of the message parameters depends on the modeOfOperation parameter
    if (RFT_MOOP < MOOP_ADD_FILE || RFT_MOOP > MOOP_RESUME_FILE)
    {
        LE_ERROR("Bad moop for 0x38");
        // UDS_0x38_NRC_31: Invalid moop
        return SendNRC(RTF_SID, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    // Check active session type for RequestFileTransfer.
    if (! IsValidSvcActiveSession(RTF_SID, SessionType))
    {
        LE_ERROR("Programming session type is not active for RequestFileTransfer.");
        // UDS_0x38_NRC_22: Not in programming session
        return SendNRC(RTF_SID, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION, addrInfoPtr);
    }
    else
    {
        uint16_t filePathAndNameLength = LENGTH_OF_FILE_NAME;

        // Maximum length can be computed using fileSizeParamterLength and filePathAndNameLength
        switch(RFT_MOOP)
        {
            case MOOP_READ_DIR:
            case MOOP_DELETE_FILE:
            {
                if (recvDataLen != (RFT_BASE_LEN + filePathAndNameLength))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    // UDS_0x38_NRC_13: Mismatch msg size (moop: 02/05)
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;

            case MOOP_ADD_FILE:
            case MOOP_RESUME_FILE:
            case MOOP_REPLACE_FILE:
            {
                uint8_t fileSizeParameterLength =
                    LENGTH_OF_FILE_SIZE(recvBuf, RFT_BASE_LEN + filePathAndNameLength +
                            SIZE_OF_DFI_);

                if (fileSizeParameterLength > 4 /* 4 byptes == 32 bits --> 4GB */
                 || recvDataLen != (RFT_BASE_LEN + filePathAndNameLength
                                    + SIZE_OF_DFI_
                                    + SIZE_OF_FSL + (fileSizeParameterLength * 2)))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    // UDS_0x38_NRC_13: Mismatch msg size (moop: 01/03/06)
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;

            case MOOP_READ_FILE:
            {
                if (recvDataLen != (RFT_BASE_LEN + filePathAndNameLength + SIZE_OF_DFI_))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    // UDS_0x38_NRC_13: Mismatch msg size (moop: 04)
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;
        }

        if ( RFT_MOOP == MOOP_ADD_FILE
          || RFT_MOOP == MOOP_RESUME_FILE
          || RFT_MOOP == MOOP_REPLACE_FILE
          || RFT_MOOP == MOOP_READ_FILE )
        {
            uint8_t dataFormatIdentifier = recvBuf[RFT_BASE_LEN + filePathAndNameLength];

#ifdef LE_CONFIG_DIAG_FEATURE_A
            if (dataFormatIdentifier != 0x00 && dataFormatIdentifier != 0x01)
            // FIXME: Only support 0x00 first
#else
            if (dataFormatIdentifier != 0x00)
#endif
            {
                LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                // UDS_0x38_NRC_31: Invalid dataFormatIdentifier
                return SendNRC(RTF_SID, REQ_OUT_OF_RANGE, addrInfoPtr);
            }
        }

        // Check the specified filePathAndName is valid.
        if (!isAcsiiFormat((char *)(recvBuf + INDEX_FP_B1), filePathAndNameLength))
        {
            LE_ERROR("Data is not ASCII format");
            // UDS_0x38_NRC_31: File name not ASCII format
            return SendNRC(RTF_SID, REQ_OUT_OF_RANGE, addrInfoPtr);
        }

        // Check if in the process of downloading or uploading data
        if (isTransferInProgress())
        {
            LE_ERROR("Bad order, transfer is in progress...");
            // UDS_0x38_NRC_22: Transfer is in progress
            return SendNRC(RTF_SID, CONDITIONS_NOT_CORRECT, addrInfoPtr);
        }
    }


    try
    {
        cfg::Node & node = cfg::get_root_node().get_child(std::string("services_all.")
                                                        + std::to_string(RTF_SID)
                                                        + ".access.security_level");
        bool lockSupported = false;

        for (auto &secLevel : node)
        {
            if ( 0x27 == secLevel.second.get_value<uint8_t>() )
            {
                lockSupported = true;
                break;
            }
        }

        if (lockSupported == true)
        {
            if (securityLevel == 0)
            {
                LE_ERROR("Security access denied");
                // UDS_0x38_NRC_33: Access denied
                return SendNRC(RTF_SID, SECURITY_ACCESS_DENY, addrInfoPtr);
            }
            else
            {
                LE_DEBUG("unlock: OK");
            }
        }
        else
        {
            LE_WARN("Can't invalid value for 0x%02X.access.security_level", RTF_SID);
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Can't get 0x%02X.access.security_level info: %s", RTF_SID, e.what());
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Send Tester present response message.
 */
le_result_t UdsCommunicationMgr::TesterPresentResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("TesterPresentResp");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for request msg length
    if(recvDataLen != UDS_TESTER_PRESENT_REQ_LEN)
    {
        LE_DEBUG("recvDataLen is incorrect.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    if( (recvBuf[1] & 0x7F) != 0)
    {
        LE_DEBUG("sub function is incorrect.");
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        LE_DEBUG("Don't send response");
        return LE_OK;
    }

    // Fill the response data
    sendBuf[0] = TESTER_PRESENT_RESPONSE_ID;
    sendBuf[1] = 0;
    sendDataLen = UDS_TESTER_PRESENT_RESP_LEN;

    //Send positive response
    SendData(addrInfoPtr);

    return LE_OK;
}

/**
 * Check NRC and Indicate received ClearDiagnosticInformation (0x14) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateClearDiagInfoReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateClearDiagInfoReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_CLEAR_DIAG_INFO_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ClearDiagnosticInformation msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received ControlDTCSetting (0x85) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateCtrlDTCSettingReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateClearDiagInfoReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for ControlDTCSetting.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for ControlDTCSetting.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_CTRL_DTC_SETTING_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ControlDTCSetting msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received ReadDTCInformation (0x19) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateReadDTCInfoReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateReadDTCInfoReq");

    // received service ID and sub function.
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_READ_DTC_INFO_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ReadDTC request msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Send indication message to Diag service.
 */
le_result_t UdsCommunicationMgr::CheckAndSendInd
(
    uint8_t sid,
    taf_doip_AddrInfo_t* addrInfoPtr,
    taf_doip_DiagMsg_t* diagMsgPtr
)
{
    taf_doip_AddrInfo_t indAddrInfo;
    taf_doip_DiagMsg_t indDiagMsg;

    if(addrInfoPtr == NULL || diagMsgPtr == NULL)
    {
        LE_ERROR("Bad parameters for address & msg handler");
        return LE_FAULT;
    }

    if(udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not found any valid reference.");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }

    taf_UDSIndicationHandler_t* udsHandler =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsHandlerRefMap,
                    udsIndicationHandler.safeRef);

    if(udsHandler == NULL)
    {
        LE_ERROR("Not find handler");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }
    if(udsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not found handler callback.");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }

    //Send RCRRP, since the application might spend much time to handle the request.

    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    LE_DEBUG("------P2* timer start -------");
    readyToRecvData = false;

    UdsTimerEventReport(TAF_UDS_P2STAR_TIMER_START, 0);
    indAddrInfo.sa = addrInfoPtr->sa;
    indAddrInfo.ta = addrInfoPtr->ta;
    indAddrInfo.taType = addrInfoPtr->taType;
    indDiagMsg.dataPtr = recvBuf;
    indDiagMsg.dataLen = recvDataLen;
    udsHandler->funcPtr(&indAddrInfo, &indDiagMsg, TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
    sendDataLen = 0;

    return LE_OK;
}

/**
 * Check NRC and Indicate to Diag service on receiving request message from DoIP.
 */
void UdsCommunicationMgr::DiagIndicationHandler
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    taf_doip_DiagMsg_t*   diagMsgPtr,
    taf_doip_Result_t result,
    void* userPtr
)
{
    LE_DEBUG("DiagIndicationHandler");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    bool isInternalHandle = true;
    le_result_t ret = LE_OK;

    if(addrInfoPtr == NULL)
    {
        LE_ERROR("addrInfoPtr invalid.");
        return;
    }

    if (result == TAF_DOIP_RESULT_SA_REGISTERED)
    {
        LE_DEBUG("result =%d",result);
        return;
    }

    if (result == TAF_DOIP_RESULT_SA_DEREGISTERED)
    {
        LE_INFO("Disconnected, stopped the running timer");

        udsCmMgr.UdsTimerEventReport(TAF_UDS_S3_TIMER_STOP, 0);
        udsCmMgr.UdsTimerEventReport(TAF_UDS_P2STAR_TIMER_STOP, 0);

        udsCmMgr.readyToRecvData = true;
        udsCmMgr.isXferActive = false;
        memset(udsCmMgr.recvBuf, 0, UDS_DATA_SIZE);
        udsCmMgr.recvDataLen = 0;
        udsCmMgr.sendDataLen = 0;

        IndicateWhenChangingToDefault();

        return;
    }

    if(diagMsgPtr == NULL)
    {
        LE_ERROR("diagMsgPtr invalid.");
        return;
    }

    LE_DEBUG("UDS requst is from 0x%x to 0x%x", addrInfoPtr->sa, addrInfoPtr->ta);
    LE_DEBUG("Data pack length: %" PRIuS, diagMsgPtr->dataLen);

    if (result != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Diag message invalid.");
        return;
    }

    //Ignore other requests if hardware reset is in progress until system is restarted
    if(udsCmMgr.isResetInProgress)
    {
        LE_ERROR("Hardware reset is in progress, ignore other requests");
        return;
    }

    if(!udsCmMgr.readyToRecvData)
    {
        LE_ERROR("Handle in progress, can't receive another request");
        udsCmMgr.SendNRC(diagMsgPtr->dataPtr[0], BUSY_REPEAT_REQ, addrInfoPtr);
        return;
    }

    memcpy((char*)(udsCmMgr.recvBuf), (char*)(diagMsgPtr->dataPtr), UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = diagMsgPtr->dataLen;
    udsCmMgr.sendDataLen = 0;

    // Check negative err code for minimum request msg length
    if(udsCmMgr.recvDataLen < UDS_REQ_MIN_LEN)
    {
        LE_ERROR("recvDataLen is less than the UDS msg minimum length.");
        return;
    }

    // received service ID
    uint8_t sid = udsCmMgr.recvBuf[0];

    LE_DEBUG("-------Request service id = 0x%x",sid);
    //Handle the request message according to the service id
    switch (sid)
    {
        case SESSION_CONTROL_REQUEST_ID:  // 0x10
        {
            // Check NRC and then send indication to TelAf diag service if necessary for Session
            // control request msg.
            ret = udsCmMgr.IndicateSessionCtrlReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case ECU_RESET_REQUEST_ID:  // 0x11
        {
            // Check NRC and then send indication to TelAf diag service if necessary for ECUReset
            // request msg.
            ret = udsCmMgr.IndicateECUResetReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case READ_DTC_INFO_REQUEST_ID:  // 0x19
        {
            // Check NRC and then send indication to TelAf diag service if necessary for ReadDTCInfo
            // request msg.
            ret = udsCmMgr.IndicateReadDTCInfoReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case READ_DID_REQUEST_ID:  // 0x22
        {
            // Check NRC and then send indication to TelAf diag service if necessary for readDid.
            ret = udsCmMgr.IndicateReadDIDReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case SECURITY_ACCESS_REQUEST_ID:  // 0x27
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // Security access request msg.
            ret = udsCmMgr.IndicateSecAccessReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case WRITE_DID_REQUEST_ID:  // 0x2E
        {
            // Check NRC and then send indication to TelAf diag service if necessary for writeDid.
            ret = udsCmMgr.IndicateWriteDIDReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case INPUT_OUTPUT_CONTROL_REQUEST_ID:  // 0x2F
        {
            // Check NRC and then send indication to TelAf diag service if necessary for IOCBID.
            ret = udsCmMgr.IndicateIOCBIDReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case ROUTINE_CONTROL_REQUEST_ID: // 0x31
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // RoutineControl request msg.
            ret = udsCmMgr.IndicateRoutinrCtrlReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case TRANSFER_DATA_REQUEST_ID:  // 0x36
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // TransferData request msg.
            ret = udsCmMgr.IndicateRxXferDataReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case REQUEST_TRANSFER_EXIT_REQUEST_ID:  // 0x37
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // RequestTransferExit request msg.
            ret = udsCmMgr.IndicateRxXferExitReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case REQUEST_FILE_TRANSFER_REQUEST_ID:  // 0x38
        {
            // Check NRC and then send indication to TelAf diag service if necessary
            // for RequestFileTransfer request msg.
            ret = udsCmMgr.IndicateRxFileXferReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case TESTER_PRESENT_REQUEST_ID:  // 0x3E
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.TesterPresentResp(addrInfoPtr);
            isInternalHandle = true;
        }
        break;
        case CLEAR_DIAG_INFO_REQUEST_ID:  // 0x14
        {
            // Check NRC and then send indication to TelAf diag service if necessary.
            ret = udsCmMgr.IndicateClearDiagInfoReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        default:
        {
            LE_DEBUG("Service type is not supported");
            ret = udsCmMgr.SendNRC(sid, SERVICE_NOT_SUPPORTED, addrInfoPtr);
            isInternalHandle = true;
        }
        break;
    }

    if(ret != LE_OK)
    {
        LE_ERROR("Failed to handle request");
        return;
    }

    // Keep a diagnostic session other than the defaultSession active while not receiving any
    // diagnostic request message
    if((sid != SESSION_CONTROL_REQUEST_ID) && (udsCmMgr.SessionType != DEFAULT_SESSION))
    {
        uint32_t p2StarServerInterval, maxNumberOfRcrrp, s3ServerInterval;

        LE_DEBUG("In non-default session, received the request, then restart S3 timer");
        //Get P2* server interval;
        try
        {
            cfg::Node & node = cfg::top_diagnostic_session<int>("id", (int)udsCmMgr.SessionType);
            p2StarServerInterval = node.get<uint32_t>("p2_star_server_max") * 1000;//Sec to msec.
            LE_DEBUG("p2StarServerInterval : %d", p2StarServerInterval);
            if(p2StarServerInterval > UDS_P2_STAR_SERVER_MAX)
            {
                p2StarServerInterval = UDS_P2_STAR_SERVER;
                LE_ERROR("p2_star_server_max > maxmimal value. Use default value:%dms",
                        p2StarServerInterval);
            }
        }
        catch (const std::exception& e)
        {
            p2StarServerInterval = UDS_P2_STAR_SERVER;
            LE_ERROR("Exception: %s. Use default value:%dms", e.what() , p2StarServerInterval);
        }
        //Get P2* server count
        try
        {
            cfg::Node & root = cfg::get_root_node();
            cfg::Node & common = root.get_child("common_props");
            maxNumberOfRcrrp = common.get<uint32_t>(
                    "max_number_of_request_correctly_received_response_pending");
            LE_DEBUG("maxNumberOfRcrrp = %d", maxNumberOfRcrrp);
        }
        catch (const std::exception& e)
        {
            maxNumberOfRcrrp = UDS_P2_STAR_SERVER_CNT;
            LE_ERROR("Exception: %s. Use default value:%d", e.what() , maxNumberOfRcrrp);
        }
        //Get S3* server interval
        try
        {
            cfg::Node & root = cfg::get_root_node();
            cfg::Node & common = root.get_child("common_props");
            s3ServerInterval = ((uint32_t)common.get<float>("s3_server_max")) * 1000; //Sec to msec.
            LE_DEBUG("s3ServerInterval = %d", s3ServerInterval);
        }
        catch (const std::exception& e)
        {
            s3ServerInterval = UDS_S3_SERVER;
            LE_ERROR("Exception: %s. Use default value:%dms", e.what(), s3ServerInterval);
        }


        if(!isInternalHandle && (s3ServerInterval < p2StarServerInterval*maxNumberOfRcrrp))
            s3ServerInterval = p2StarServerInterval*maxNumberOfRcrrp;

        LE_DEBUG("restart s3 with %d mili seconds", s3ServerInterval);
        udsCmMgr.UdsTimerEventReport(TAF_UDS_S3_TIMER_START, s3ServerInterval);
    }

    if(!isInternalHandle)
    {
        udsCmMgr.CheckAndSendInd(sid, addrInfoPtr, diagMsgPtr);
    }

    return;
}

/**
 * Receive confirmation message from DoIP.
 */
void UdsCommunicationMgr::DiagConfirmHandler
(
    const taf_doip_AddrInfo_t*  addrInfoPtr, ///< [IN] Logical address information pointer.
    taf_doip_Result_t           result,      ///< [IN] Result of the confirm execution
    void*                       userPtr      ///< [IN] User-defined pointer
)
{
    if (result != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to send the uds response.%d", result);
    }
}

/**
 * Add handler function to indicate when message recived from DoIP stack.
 */
le_result_t UdsCommunicationMgr::UdsAddDiagIndicationHandler
(
)
{
    LE_INFO("UdsAddDiagIndicationHandler");

    if (DoipEntityRef == NULL)
    {
        LE_ERROR("DoIP stack is not initialized");
        return LE_FAULT;
    }

    IndicationRef = taf_doip_AddDiagIndicationHandler(DoipEntityRef,
            (taf_doip_DiagIndicationHandlerFunc_t)DiagIndicationHandler, NULL);

    if (IndicationRef == NULL)
    {
        LE_FATAL("Failed to register diag indication handler");
        return LE_FAULT;
    }

    ConfirmRef = taf_doip_AddDiagConfirmHandler(DoipEntityRef,
        (taf_doip_DiagConfirmHandlerFunc_t)DiagConfirmHandler, NULL);
    if (ConfirmRef == NULL)
    {
        LE_FATAL("Failed to register diag confirmation handler");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Send Diagnostic response message to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SendUDSResp
(
    uint16_t sa,
    uint16_t ta,
    uint8_t addrType,
    uint8_t serviceId,
    uint8_t err,
    const uint8_t* dataPtr,
    uint16_t dataSize
)
{
    LE_DEBUG("SendUDSResp");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    le_result_t ret;
    taf_doip_DiagMsg_t respDiagMsg;
    taf_doip_AddrInfo_t respAddrInfo;

    if (DoipEntityRef == NULL)
    {
        LE_ERROR("DoIP stack is not initialized");
        return LE_FAULT;
    }

    //If already disconnected, can't send the response.
    if(udsCmMgr.recvDataLen == 0)
    {
        LE_ERROR("Already disconnected, can not send response");
        return LE_FAULT;
    }

    if (serviceId != recvBuf[0])
    {
        LE_ERROR("Request and Response service id mismatch, RespSid=0x%x, RecvSid=0x%x",
                serviceId, recvBuf[0]);
        return LE_BAD_PARAMETER;
    }

    // Check the send dataLength shall not be more than UDS_DATA_SIZE.
    if (dataSize > UDS_DATA_SIZE)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    LE_DEBUG("SERVICEID = 0x%x", serviceId);
    //Send UDS response according to the service id.
    switch(serviceId)
    {
        case SESSION_CONTROL_REQUEST_ID:
            ret = SessionCtrlResp(serviceId, err);
        break;
        case ECU_RESET_REQUEST_ID:
            ret = ECUResetResp(serviceId, err);
        break;
        case READ_DID_REQUEST_ID:
            ret = ReadDIDResp(serviceId, dataPtr, dataSize, err);
        break;
        case WRITE_DID_REQUEST_ID:
            ret = WriteDIDResp(serviceId, err);
        break;
        case SECURITY_ACCESS_REQUEST_ID:
            ret = SecurityAccessResp(serviceId, dataPtr, dataSize, err);
        break;
        case INPUT_OUTPUT_CONTROL_REQUEST_ID:
            ret = IOCBIDResp(serviceId, dataPtr, dataSize, err);
        break;
        case ROUTINE_CONTROL_REQUEST_ID:
            ret = RoutineCtrlResp(serviceId, dataPtr, dataSize, err);
        break;
        case TRANSFER_DATA_REQUEST_ID:
            ret = XferDataResp(serviceId, dataPtr, dataSize, err);
        break;
        case REQUEST_TRANSFER_EXIT_REQUEST_ID:
            ret = ReqXferExitResp(serviceId, err);
        break;
        case REQUEST_FILE_TRANSFER_REQUEST_ID:
            ret = ReqFileXferResp(serviceId, dataPtr, dataSize, err);
        break;
        case CLEAR_DIAG_INFO_REQUEST_ID:
            ret = ClearDiagInfoResp(serviceId, err);
        break;
        case READ_DTC_INFO_REQUEST_ID:
            ret = ReadDTCInfoResp(serviceId, dataPtr, dataSize, err);
        break;
        case CONTROL_DTC_SETTING_REQUEST_ID:
            ret = CtrlDTCSettingResp(serviceId, err);
        break;
        default:
            SetNRC(serviceId, SERVICE_NOT_SUPPORTED);
            LE_ERROR("Service type is not supported");
            ret = LE_OK;
        break;
    }

    if (ret == LE_UNSUPPORTED)
    {
        // Suppress positive response.
        udsCmMgr.CheckAndRestartS3Timer(serviceId);
        return LE_OK;
    }
    else if (ret != LE_OK)
    {
        LE_ERROR("Send error");
        return ret;
    }

    respAddrInfo.sa = sa;
    respAddrInfo.ta = ta;
    respAddrInfo.taType = (taf_doip_TaType_t)addrType;
    respDiagMsg.dataPtr = udsCmMgr.sendBuf;
    respDiagMsg.dataLen = udsCmMgr.sendDataLen;

    ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
    if (ret == LE_OK)
    {
        LE_DEBUG("Requested Diagnostic message response sent.");
        udsCmMgr.CheckAndRestartS3Timer(serviceId);
    }

    return LE_OK;
}

/**
 * Check error code and Pack SessionCtrlResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SessionCtrlResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    uint32_t p2StarServerInterval, p2ServerInterval, maxNumberOfRcrrp, s3ServerInterval;

    LE_INFO("SessionCtrlResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    taf_SessionType_t oldSessionType;
    taf_SessionType_t newSessionType;
    uint8_t suppressPosRspFlag;

    newSessionType = (taf_SessionType_t)(recvBuf[1] & 0x7F);
    suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;

    // copy the previous session type as old session locally.
    oldSessionType = SessionType;

    // change the session type as requested and maintain it in stack
    SessionType = newSessionType;

    //Get P2* server interval;
    try
    {
        cfg::Node & node = cfg::top_diagnostic_session<int>("id", (int)SessionType);
        p2StarServerInterval = node.get<uint32_t>("p2_star_server_max") * 1000;//Sec to msec.
        LE_DEBUG("p2StarServerInterval : %d", p2StarServerInterval);
        if(p2StarServerInterval > UDS_P2_STAR_SERVER_MAX)
        {
            p2StarServerInterval = UDS_P2_STAR_SERVER;
            LE_ERROR("p2_star_server_max > maxmimal value. Use default value:%dms",
                    p2StarServerInterval);
        }
    }
    catch (const std::exception& e)
    {
        p2StarServerInterval = UDS_P2_STAR_SERVER;
        LE_ERROR("Exception: %s. Use default value:%dms", e.what(), p2StarServerInterval);
    }
    //Get P2* server count
    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        maxNumberOfRcrrp = common.get<uint32_t>(
                "max_number_of_request_correctly_received_response_pending");
        LE_DEBUG("maxNumberOfRcrrp = %d", maxNumberOfRcrrp);
    }
    catch (const std::exception& e)
    {
        maxNumberOfRcrrp = UDS_P2_STAR_SERVER_CNT;
        LE_ERROR("Exception: %s. Use default value:%d", e.what(), maxNumberOfRcrrp);
    }
    //Get S3* server interval
    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        s3ServerInterval = ((uint32_t)common.get<float>("s3_server_max")) * 1000; //Sec to msec.
        LE_DEBUG("s3ServerInterval = %d", s3ServerInterval);
    }
    catch (const std::exception& e)
    {
        s3ServerInterval = UDS_S3_SERVER;
        LE_ERROR("Exception: %s. Use default value:%dms", e.what(), s3ServerInterval);
    }
    //Get P2 server interval;
    try
    {
        cfg::Node & node = cfg::top_diagnostic_session<int>("id", (int)SessionType);
        p2ServerInterval = node.get<float>("p2_server_max") * 1000;//Sec to msec.
        LE_DEBUG("p2ServerInterval : %d", p2ServerInterval);
        if(p2ServerInterval > UDS_P2_SERVER_MAX)
        {
            p2ServerInterval = UDS_P2_SERVER;
            LE_ERROR("p2_server_max > maxmimal value. Use default value:%dms", p2ServerInterval);
        }
    }
    catch (const std::exception& e)
    {
        p2ServerInterval = UDS_P2_SERVER;
        LE_ERROR("Exception: %s. Use default value:%dms", e.what(), p2ServerInterval);
    }

    // Notify session change to the application.
    if (oldSessionType != newSessionType)
    {
        // Session switched to default session.
        if(newSessionType == DEFAULT_SESSION)
        {
            LE_INFO("default session:need to stop s3 timer");
            UdsTimerEventReport(TAF_UDS_S3_TIMER_STOP, 0);
        }
        //Session switched to non-default session.
        else
        {
            LE_INFO("other session:need to start s3 timer");

            UdsTimerEventReport(TAF_UDS_S3_TIMER_START, s3ServerInterval);
        }

        //Non default session to other session.
        if(oldSessionType != DEFAULT_SESSION)
        {
            reqSeedLevel = 0;
            securityLevel = 0;
            LE_DEBUG("Session switched, reset the security level");
        }

        taf_doip_DiagMsg_t sesChangeMsg;
        if(udsIndicationHandler.safeRef == NULL)
        {
            LE_ERROR("Not find handler to notify session change");
            goto out;
        }

        taf_UDSIndicationHandler_t* udsHandler =
                (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsHandlerRefMap,
                        udsIndicationHandler.safeRef);

        if(udsHandler == NULL || udsHandler->funcPtr == NULL)
        {
            LE_ERROR("Not find handler to notify session change!");
            goto out;
        }

        LE_DEBUG("Notify session change to application!");
        sesChangeBuf[0] = sesChangeId;
        sesChangeBuf[1] = oldSessionType;
        sesChangeBuf[2] = SessionType;
        sesChangeMsg.dataPtr = sesChangeBuf;
        sesChangeMsg.dataLen = UDS_SESSION_CHANGE_DATA_SIZE;
        udsHandler->funcPtr(&addrInfo, &sesChangeMsg, TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
    }

out:
    if (suppressPosRspFlag == 0)
    {
        // Fill the response data to send the session response msg to DTool
        sendBuf[0] = SESSION_CONTROL_RESPONSE_ID;
        sendBuf[1] = recvBuf[1] & 0x7F;
        sendBuf[2] = (p2ServerInterval & 0xff00) >> 8;
        sendBuf[3] = p2ServerInterval & 0xff;
        sendBuf[4] = (((p2StarServerInterval)/10) & 0xff00) >> 8; // The resolution for P2* is 10ms
        sendBuf[5] = ((p2StarServerInterval)/10) & 0xff; // The resolution for P2* is 10ms
        sendDataLen = UDS_SESSION_CTRL_RESP_LEN;
        return LE_OK;
    }
    else
    {
        return LE_UNSUPPORTED;
    }
}

/**
 * Check error code and Pack ECUResetResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ECUResetResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ECUResetResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    //Get ignore_request_for_hardreset from YAML
    bool ignoreReqForHardReset = false;

    try
    {
        cfg::Node & root = cfg::get_root_node();
        cfg::Node & common = root.get_child("common_props");
        ignoreReqForHardReset = common.get<bool>("ignore_request_for_hardreset");
        LE_INFO("ignoreReqForHardReset = %d", ignoreReqForHardReset);
    }
    catch (const std::exception& e)
    {
        LE_DEBUG("Exception: %s. ignore_request_for_hardreset not configured", e.what());
    }

    uint8_t resetType = recvBuf[1] & 0x7F;
    // Currently it's positive response for hardreset and let's get the attribute, tmp
    if(ignoreReqForHardReset && (resetType == HARD_RESET))
        isResetInProgress = true;

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        return LE_UNSUPPORTED;
    }

    sendBuf[0] = ECU_RESET_RESPONSE_ID;
    sendBuf[1] = resetType;
    sendDataLen = UDS_ECU_RESET_RESP_BASE_LEN;

    return LE_OK;
}

/**
 * Check error code and Pack ReadDID message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReadDIDResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("ReadDIDResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_READ_DID_RESP_BASE_LEN ||
        dataSize < UDS_READ_DID_RESP_MIN_LEN)
    {
        LE_ERROR("dataLength is not correct.");
        return LE_FAULT;
    }

    sendBuf[0] = READ_DID_RESPONSE_ID;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_READ_DID_RESP_BASE_LEN, dataPtr, dataSize);
        sendDataLen = UDS_READ_DID_RESP_BASE_LEN + dataSize;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Check error code and Pack WriteDID message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::WriteDIDResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("WriteDIDResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = WRITE_DID_RESPONSE_ID;
    sendBuf[1] = recvBuf[1];
    sendBuf[2] = recvBuf[2];
    sendDataLen = UDS_WRITE_DID_RESP_LEN;

    return LE_OK;
}

/**
 * Check error code and Pack SecurityAccess message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SecurityAccessResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("SecurityAccessResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_SECURITY_ACCESS_RESP_MIN_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        return LE_UNSUPPORTED;
    }

    uint8_t securityAccessType = recvBuf[1] & 0x7F;

    sendBuf[0] = SECURITY_ACCESS_RESPONSE_ID;
    sendBuf[1] = securityAccessType;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_SECURITY_ACCESS_RESP_MIN_LEN, dataPtr, dataSize);
        sendDataLen = UDS_SECURITY_ACCESS_RESP_MIN_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_SECURITY_ACCESS_RESP_MIN_LEN;
    }

    if (securityAccessType % 2 ==0)
        securityLevel = securityAccessType - 1;

    LE_DEBUG("securityLevel = %d", securityLevel);
    return LE_OK;
}

/**
 * Check error code and Pack RoutineCtrlResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::RoutineCtrlResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("RoutineCtrlResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_ROUTINE_CTRL_RESP_MIN_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        return LE_UNSUPPORTED;
    }

    uint8_t routineControlType = recvBuf[1] & 0x7F;
    uint16_t routineIdentifier = (recvBuf[2] << 8) + recvBuf[3];

    sendBuf[0] = ROUTINE_CONTROL_RESPONSE_ID;
    sendBuf[1] = routineControlType;
    sendBuf[2] = routineIdentifier >> 8;
    sendBuf[3] = routineIdentifier;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_ROUTINE_CTRL_RESP_MIN_LEN, dataPtr, dataSize);
        sendDataLen = UDS_ROUTINE_CTRL_RESP_MIN_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_ROUTINE_CTRL_RESP_MIN_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack InputOutputControlByIdentifier message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::IOCBIDResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("IOCBIDResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_IOCBID_RESP_MIN_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    //InputOutputControlParameter is not included in the data record.
    sendBuf[0] = IOCBID_RESPONSE_ID;
    sendBuf[1] = recvBuf[1]; //DataId high byte
    sendBuf[2] = recvBuf[2]; //DataId low byte
    sendBuf[3] = recvBuf[3]; //InputOutputControlParameter

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_IOCBID_RESP_MIN_LEN, dataPtr, dataSize);
        sendDataLen = UDS_IOCBID_RESP_MIN_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_IOCBID_RESP_MIN_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack XferDataResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::XferDataResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("XferDataResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_RESP_XFER_DATA_BASE_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        isXferActive = false;
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t blockSequenceCounter = recvBuf[1];

    sendBuf[0] = TRANSFER_DATA_RESPONSE_ID;
    sendBuf[1] = blockSequenceCounter;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_RESP_XFER_DATA_BASE_LEN, dataPtr, dataSize);
        sendDataLen = UDS_RESP_XFER_DATA_BASE_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_RESP_XFER_DATA_BASE_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack ReqXferExitResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReqXferExitResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ReqXferExitResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = REQUEST_TRANSFER_EXIT_RESPONSE_ID;
    sendDataLen = UDS_RESP_XFER_EXIT_BASE_LEN;

    isXferActive = false;

    return LE_OK;
}

// Given a number, determine how many bytes it takes
static uint8_t HowManyChars(uint16_t maxNumberOfBlock)
{
    uint8_t nbytes = 0;
    while (maxNumberOfBlock) {
        maxNumberOfBlock >>= 1;
        nbytes++;
    }

    nbytes = (nbytes + 7) / 8;
    return nbytes;
}

/**
 * Check error code and Pack ReqFileXferResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReqFileXferResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("In %s", __FUNCTION__);

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint16_t filePathAndNameLength = LENGTH_OF_FILE_NAME;
    uint16_t maxNumberOfBlockLen = UDS_DATA_SIZE - 4; // Reduce the source & target addresses.
    uint8_t lengthFormatIdentifier = HowManyChars(maxNumberOfBlockLen);

    // Echo DFI_ in response
    #define RFT_DFI_ (recvBuf[RFT_BASE_LEN + filePathAndNameLength])

    switch (RFT_MOOP)
    {
        case MOOP_DELETE_FILE:
        {
            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendDataLen = (SIZE_OF_SID + SIZE_OF_MOOP);
        }
        break;

        case MOOP_ADD_FILE:
        case MOOP_REPLACE_FILE:
        case MOOP_RESUME_FILE:
        {
            isXferActive = true;

            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendBuf[2] = lengthFormatIdentifier;

            // Convert maxNumberOfBlockLen to big-endian storage.
            for (uint8_t index = 0; index < sizeof(maxNumberOfBlockLen); index++)
            {
                uint8_t shiftBytes = sizeof(maxNumberOfBlockLen) - 1 - index;
                uint8_t indexValue = (maxNumberOfBlockLen >> (shiftBytes * 8)) & 0xFF;
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + index] = indexValue;
            }

            sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = RFT_DFI_;

            sendDataLen = RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen) + SIZE_OF_DFI_;

            // For MOOP_RESUME_FILE, filePosition is required
            if (dataPtr)
            {
                memcpy(sendBuf + sendDataLen, dataPtr, dataSize);
                sendDataLen += dataSize;
            }
            else
            {
                LE_FATAL_IF(RFT_MOOP == MOOP_RESUME_FILE, "No file position parameter");
            }
        }
        break;

        case MOOP_READ_FILE:
        case MOOP_READ_DIR:
        {
            isXferActive = true;

            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendBuf[2] = lengthFormatIdentifier;

            // Convert maxNumberOfBlockLen to big-endian storage.
            for (uint8_t index = 0; index < sizeof(maxNumberOfBlockLen); index++)
            {
                uint8_t shiftBytes = sizeof(maxNumberOfBlockLen) - 1 - index;
                uint8_t indexValue = (maxNumberOfBlockLen >> (shiftBytes * 8)) & 0xFF;
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + index] = indexValue;
            }

            if (RFT_MOOP == MOOP_READ_DIR)
            {
                // For MOOP_READ_DIR, the fixed 0x00 is reponsed
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = 0x00;
            }
            else
            {
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = RFT_DFI_;
            }

            sendDataLen = RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen) + SIZE_OF_DFI_;

            LE_FATAL_IF(dataPtr == NULL, "No file size or dir info length arguments");

            memcpy(sendBuf + sendDataLen, dataPtr, dataSize);
            sendDataLen += dataSize;
        }
        break;

        default:
            LE_ERROR("Response for requested mode of operation is not supported");
            break;
    }

    return LE_OK;
}

/**
 * Check error code and Pack ReadDTCInfoResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReadDTCInfoResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("ReadDTCInfoResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_READ_DTC_INFO_RESP_BASE_LEN)
    {
        LE_ERROR("dataLength is not correct.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        return LE_UNSUPPORTED;
    }

    uint8_t reportType = recvBuf[1] & 0x7F;  // Equal to subfunction 0~6bit

    sendBuf[0] = READ_DTC_INFO_RESPONSE_ID;
    sendBuf[1] = reportType;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_READ_DTC_INFO_RESP_BASE_LEN, dataPtr, dataSize);
        sendDataLen = UDS_READ_DTC_INFO_RESP_BASE_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_READ_DTC_INFO_RESP_BASE_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack ClearDiagInfo message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ClearDiagInfoResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ClearDiagInfoResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = CLEAR_DIAG_INFO_RESPONSE_ID;
    sendDataLen = UDS_CLEAR_DIAG_INFO_RESP_LEN;

    return LE_OK;
}

/**
 * Check error code and Pack CtrlDTCSetting message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::CtrlDTCSettingResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ClearDiagInfoResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t suppressPosRspFlag = (recvBuf[1] >> 7) & 0x1;
    if (suppressPosRspFlag == 1)
    {
        return LE_UNSUPPORTED;
    }

    uint8_t settingType = recvBuf[1] & 0x7F;  // Equal to subfunction 0~6bit

    sendBuf[0] = CONTROL_DTC_SETTING_RESPONSE_ID;
    sendBuf[1] = settingType;
    sendDataLen = UDS_CTRL_DTC_SETTING_RESP_LEN;

    return LE_OK;
}

bool UdsCommunicationMgr::IsSessTypeMatched
(
    cfg::Node& node
)
{
    try
    {
        cfg::Node & sesType = node.get_child("access.session");

        for (const auto & session: sesType)
        {
            string type = session.second.get_value<string>("");

            cfg::Node & sesNode = cfg::top_diagnostic_session<string>("short_name", type);
            int session_id = sesNode.get<int>("id");

            if (session_id == SessionType)
            {
                LE_DEBUG("Current session type is supported for node");
                return true;
            }
        }
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Exception: %s", e.what());
        return true;  // If not found in configuration, return true.
    }

    LE_DEBUG("Current session type(%d) is unsupported for node", SessionType);

    return false;
}

bool UdsCommunicationMgr::IsSecurityAccessMatched
(
    cfg::Node& node
)
{
    if (SessionType == DEFAULT_SESSION)
    {
        return true;  // In default session, no need to check security level.
    }

    try
    {
        int secType = node.get_child("access").get<int>("security_level");
        LE_DEBUG("Security type = 0x%x", secType);

        // Security access check
        if (secType == SECURITY_ACCESS_REQUEST_ID && securityLevel == 0)
        {
            LE_WARN("Node is secured and the server is not unlocked.");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Exception: %s", e.what());
    }

    return true;
}

bool UdsCommunicationMgr::IsRequestSubFuncSupported
(
    cfg::Node& node,
    uint8_t subFunc
)
{
    try
    {
        cfg::Node & subFuncList = node.get_child("request.sub_function");

        for (const auto & subFunNode : subFuncList)
        {
            int id = subFunNode.second.get_value<int>();
            if ((uint8_t)id == subFunc)
            {
                LE_DEBUG("subFunction(0x%x) is supported for node", subFunc);
                return true;
            }
        }
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Exception: %s", e.what());
        return true;  // If not found in configuration, return true.
    }

    LE_DEBUG("subFunction(0x%x) is unsupported for node", subFunc);

    return false;
}
