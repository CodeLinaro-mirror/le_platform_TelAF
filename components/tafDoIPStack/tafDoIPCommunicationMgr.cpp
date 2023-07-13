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
#include <errno.h>
#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdint.h>
#include <memory>

#include "legato.h"
#include "interfaces.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "le_socketLib.h"
#ifdef __cplusplus
}
#endif

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPCommunicationMgr.hpp"
#include "tafDoIPConnectionMgr.hpp"
#include "tafDoIPVehicleMgr.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

using namespace taf::doip;

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetInstance
 DESCRIPTION     Get an instance of CommunicationMgr
 PARAMETERS      void
 RETURN VALUE    CommunicationMgr: Instance reference
=================================================================================================*/
CommunicationMgr &CommunicationMgr::GetInstance()
{
    static CommunicationMgr instance;

    return instance;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CommunicationMgr
 DESCRIPTION     the constructor of CommunicationMgr
 PARAMETERS      void
 RETURN VALUE    void
=================================================================================================*/
CommunicationMgr::CommunicationMgr()
{
    connectionMgrPtr = std::make_shared<ConnectionManager>();
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CommunicationMgr
 DESCRIPTION     the destructor of CommunicationMgr
 PARAMETERS      void
 RETURN VALUE    void
=================================================================================================*/
CommunicationMgr::~CommunicationMgr()
{
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CommunicationMgr
 DESCRIPTION     Get connection manager from communication manager
 PARAMETERS      void
 RETURN VALUE    ConnectionManager: connection manager pointer
=================================================================================================*/
std::shared_ptr<ConnectionManager> CommunicationMgr::GetConnectionMgr
(
)
{
    return connectionMgrPtr;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetLocalIPAddr
 DESCRIPTION     Get IP address according to interface name
 PARAMETERS      [IN] af: IP address family
                 [IN] ifname: interface name reference
                 [OUT] ip: IPv4 address buffer.
                 [IN] size: IP address buffer length
 RETURN VALUE    taf_doip_Result_t: Result of geting local IP address
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::GetLocalIPAddr
(
    int             af,
    std::string&    ifname,
    char            *ip,
    socklen_t       size
)
{
    struct ifaddrs  *ifaPtr, *tmpPtr;

    LE_DEBUG("Enter GetLocalIPAddr, family address is %d\n", af);

    if (ip == NULL || (af != AF_INET && af != AF_INET6))
    {
        LE_ERROR("Parameter error!\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    if (getifaddrs(&ifaPtr) != 0)
    {
        LE_ERROR("getifaddrs error!\n");
        return TAF_DOIP_RESULT_ERROR;
    }

    for (tmpPtr = ifaPtr; tmpPtr != NULL; tmpPtr = tmpPtr->ifa_next)
    {
        if (ifname != tmpPtr->ifa_name)
        {
            continue;
        }

        if (tmpPtr->ifa_addr == NULL)
        {
            LE_DEBUG("Unset address.\n");
            freeifaddrs(ifaPtr);
            return TAF_DOIP_RESULT_UNSET;
        }

        if (tmpPtr->ifa_addr->sa_family != af)
        {
            LE_INFO("Address family is unmatch, skip.\n");
            continue;
        }
        LE_INFO("Start to get IP address, address family is %d.\n", af);

        if (af == AF_INET6)
        {
            struct in6_addr addr6;
            addr6 = (((struct sockaddr_in6 *)(tmpPtr->ifa_addr))->sin6_addr);

            if (IN6_IS_ADDR_LINKLOCAL(&addr6))
            {
                if (inet_ntop(af, &addr6, ip, size) == NULL)
                {
                    LE_ERROR("Can not get IPv6 address.\n");
                    freeifaddrs(ifaPtr);
                    return TAF_DOIP_RESULT_ERROR;
                }
                break;
            }
        }
        else
        {
            struct in_addr addr;
            addr = (((struct sockaddr_in *)(tmpPtr->ifa_addr))->sin_addr);

            if (inet_ntop(af, &addr, ip, size) == NULL)
            {
                LE_ERROR("Can not get IPv4 address.\n");
                freeifaddrs(ifaPtr);
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
    }

    LE_INFO("IP is %s\n", ip);

    freeifaddrs(ifaPtr);

    if (tmpPtr == NULL)
    {
        LE_ERROR("Can't find interface %s\n", ifname.c_str());
        return TAF_DOIP_RESULT_ERROR;
    }

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetLocalIPv6Addr
 DESCRIPTION     Get IPv6 address according to interface name
 PARAMETERS      [IN] ifname: interface name reference
                 [OUT] ip: IPv6 address buffer.
                 [IN] size: IP address buffer length
 RETURN VALUE    taf_doip_Result_t: Result of geting local IPv6 address
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::GetLocalIPv6Addr
(
    std::string&    ifname,
    char            *ip,
    socklen_t       size
)
{
    return GetLocalIPAddr(AF_INET6, ifname, ip, size);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetLocalIPv4Addr
 DESCRIPTION     Get IPv4 address according to interface name
 PARAMETERS      [IN] ifname: interface name reference
                 [OUT] ip: IPv6 address buffer.
                 [IN] size: IP address buffer length
 RETURN VALUE    taf_doip_Result_t: Result of geting local IPv4 address
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::GetLocalIPv4Addr
(
    std::string&    ifname,
    char*           ip,
    socklen_t       size
)
{
    return GetLocalIPAddr(AF_INET, ifname, ip, size);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::FindDoipSession
 DESCRIPTION     Find a doip session from source address
 PARAMETERS      [IN] sa: logical source address
 RETURN VALUE    taf_doipSession_t: doip session pointer
=================================================================================================*/
taf_doipSession_t* CommunicationMgr::FindDoipSession
(
    uint16_t    sa
)
{
    bool                isFound = false;
    taf_doipSession_t*  doipSessionPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(cmMgr.doipSessionRefMap);

    LE_DEBUG("FindDoipSession, iter ref is %p.", iterRef);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        doipSessionPtr = (taf_doipSession_t *)le_ref_GetValue(iterRef);
        if (doipSessionPtr != NULL
            && sa == doipSessionPtr->logicalSrcAddr)
        {
            LE_INFO("found a doip session, SA is '%d'\n", sa);
            isFound = true;
            break;
        }
    }

    if (isFound)
    {
        return doipSessionPtr;
    }

    return NULL;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::QueryPowerMode
 DESCRIPTION     Query power mode
 PARAMETERS      void
 RETURN VALUE    taf_doip_PowerMode_t: power mode
=================================================================================================*/
taf_doip_PowerMode_t CommunicationMgr::QueryPowerMode
(
)
{
    taf_doipSession_t   *doipSessionPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(cmMgr.doipSessionRefMap);

    LE_DEBUG("FindDoipSession, iter ref is %p.", iterRef);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        doipSessionPtr = (taf_doipSession_t *)le_ref_GetValue(iterRef);
        if (doipSessionPtr != NULL)
        {
            taf_doipPmQueryHandler_t*   handler;
            handler = &doipSessionPtr->pmQueryhandler;
            if (handler->funcPtr)
            {
                return handler->funcPtr(handler->ctxPtr);
            }
        }
    }

    return TAF_DOIP_POWER_MODE_NOT_SUPPORTED;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::ConfirmRoutingActivation
 DESCRIPTION     Confirm routing activation
 PARAMETERS      [IN]sa: Tester logical source address
                 [IN]ta: logical target address
 RETURN VALUE    taf_doip_PowerMode_t: power mode
=================================================================================================*/
taf_doip_UserConfirmResult_t CommunicationMgr::ConfirmRoutingActivation
(
    uint16_t sa,
    uint16_t ta
)
{
    taf_doipSession_t*  sessionPtr;
    taf_doip_UserConfirmResult_t result = TAF_DOIP_USER_CONFIRM_RESULT_NO_NEED;

    sessionPtr = FindDoipSession(ta);
    if (sessionPtr == NULL)
    {
        LE_ERROR("DoIP session was not created with SA-0x%x.\n", sa);
        return TAF_DOIP_USER_CONFIRM_RESULT_REJECT;
    }

    taf_doipUserConfirmHandler_t*   handler;
    handler = &sessionPtr->userConfirmHandler;

    if (handler->funcPtr)
    {
        handler->funcPtr(sa, ta, &result, handler->ctxPtr);
    }

    return result;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetEntitySourceAddress
 DESCRIPTION     Get logical source address
 PARAMETERS      void
 RETURN VALUE    logical source address
=================================================================================================*/
uint16_t CommunicationMgr::GetEntitySourceAddress
(
)
{
    uint16_t    sa;
    auto&   vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.GetEntityLogicalAddr(&sa))
    {
        sa = TAF_DOIP_SA_DEFAULT;
    }

    return sa;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SessionStart
 DESCRIPTION     Start doip session in communication manager.
                 So we can communicate with outside.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of start
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SessionStart
(
)
{
    LE_INFO("Enter CommunicationMgr::SessionStart-state%d.\n", state);

    if (state == TAF_DOIP_STATE_START
        || state == TAF_DOIP_STATE_RUNNING)
    {
        return TAF_DOIP_RESULT_OK;
    }

    if (state != TAF_DOIP_STATE_INIT
        && state != TAF_DOIP_STATE_STOP)
    {
        LE_ERROR("Can not start communication, please initialize first(state%d)!\n", state);
        return TAF_DOIP_RESULT_ERROR;
    }

    state = TAF_DOIP_STATE_START;

    if (udpDataBuffer == NULL)
    {
        udpDataBuffer = (char*)le_mem_ForceAlloc(udpMsgPool);
        memset(udpDataBuffer, 0, TAF_DOIP_MAX_BUFFER_SIZE);
    }
    udpDataLen = 0;

    //auto& vehicleMgr = VehicleManager::GetInstance();
    //sa = vehicleMgr.GetEntityLogicalAddr();

    // Enable async mode and start fd monitor.
    if (LE_OK != le_socket_SetMonitoring(tcpDataSockRef, true))
    {
       LE_ERROR("Failed to enable tcp data socket monitor.");
       return TAF_DOIP_RESULT_ERROR;
    }

    if (LE_OK != le_socket_SetMonitoring(udpDiscoverSockRef, true))
    {
       LE_ERROR("Failed to enable udp discovery socket monitor.");
       return TAF_DOIP_RESULT_ERROR;
    }

    state = TAF_DOIP_STATE_RUNNING;
    LE_INFO("DoIP session start success.!");

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SessionStop
 DESCRIPTION     Stop doip session in communication manager.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of start
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SessionStop
(
)
{
    LE_INFO("Enter CommunicationMgr::SessionStop-state%d.\n", state);

    if (state != TAF_DOIP_STATE_START
        && state != TAF_DOIP_STATE_RUNNING)
    {
        LE_DEBUG("Not need to stop-(%d)!!\n", state);
        return TAF_DOIP_RESULT_OK;
    }

    if (udpDataBuffer != NULL)
    {
        le_mem_Release((void*)udpDataBuffer);
        udpDataBuffer = NULL;
    }
    udpDataLen = 0;

    le_socket_SetMonitoring(tcpDataSockRef, false);
    LE_DEBUG("tcpDataSockRef=%p\n", tcpDataSockRef);
    le_socket_SetMonitoring(udpDiscoverSockRef, false);
    LE_DEBUG("udpDiscoverSockRef=%p\n", udpDiscoverSockRef);

    state = TAF_DOIP_STATE_STOP;
    LE_DEBUG("DoIP session stop success.!");

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::InformUdsMessage
 DESCRIPTION     Inform uds and send to upper layer.
 PARAMETERS      [IN]sa: logical source address
                 [IN]ta: logical target address
                 [IN]dataPtr: uds data pointer
                 [IN]length: the length of uds data
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::InformUdsMessage
(
    uint16_t    sa,
    uint16_t    ta,
    char*       dataPtr,
    uint32_t    length
)
{
    taf_doipDiagDataInfo_t* diagInfoPtr;

    if (dataPtr == NULL || length == 0)
    {
        LE_ERROR("Parameter error.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    diagInfoPtr = (taf_doipDiagDataInfo_t*)le_mem_ForceAlloc(udsInfoPool);

    diagInfoPtr->sa     = sa;
    diagInfoPtr->ta     = ta;

    // Currently, we only support physical addressing;
    diagInfoPtr->taType = TAF_DOIP_TA_TYPE_PHYSICAL;
    diagInfoPtr->data   = dataPtr;
    diagInfoPtr->len    = length;

    le_event_QueueFunctionToThread(udsThrRef,
                                   IndicateUdsMessage,
                                   diagInfoPtr,
                                   NULL);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::TransUdsMessage
 DESCRIPTION     Called by upper layer to transmit uds.
 PARAMETERS      [IN]sa: logical source address
                 [IN]ta: logical target address
                 [IN]taType: DoIP logical target address type
                 [IN]dataPtr: uds data pointer
                 [IN]length: the length of uds data
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::TransUdsMessage
(
    uint16_t            sa,
    uint16_t            ta,
    taf_doip_TaType_t   taType,
    char*               dataPtr,
    uint32_t            length
)
{
    taf_doipSession_t*      sessionPtr;
    taf_doipDiagDataInfo_t* diagInfoPtr;
    taf_doip_AddrInfo_t     addrInfo;
    char*                   diagDataPtr;
    uint32_t                pos = 0;

    if (dataPtr == NULL || length == 0)
    {
        LE_ERROR("Parameter error.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    sessionPtr = FindDoipSession(sa);
    if (sessionPtr == NULL)
    {
        LE_ERROR("DoIP session was not created with SA-0x%x.\n", sa);
        return TAF_DOIP_RESULT_UNKNOWN_SA;
    }

    diagInfoPtr = (taf_doipDiagDataInfo_t*)le_mem_ForceAlloc(udsInfoPool);
    diagDataPtr = (char*)le_mem_ForceAlloc(connectionMgrPtr->udsMsgPool);

    // Pack DoIP header.
    auto& parser = ProtocolParser::GetInstance();
    uint32_t payloadLen = length + 2 * TAF_DOIP_LOGICAL_ADDRESS_LENGTH;
    parser.PackGenericHeader(diagDataPtr, TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE, payloadLen);
    pos += TAF_DOIP_HEADER_GENERIC_LENGTH;

    // Fill SA and TA in the uds message.
    uint16_t sourceAddress = htons(sa);
    memcpy(diagDataPtr + pos, &sourceAddress, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    uint16_t targetAddress = htons(ta);
    memcpy(diagDataPtr + pos, &targetAddress, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    // Copy data into shared memory and send to main thread.
    memcpy(diagDataPtr + pos, dataPtr, length);
    pos += length;

    diagInfoPtr->sa     = sa;
    diagInfoPtr->ta     = ta;
    diagInfoPtr->taType = taType;
    diagInfoPtr->data   = diagDataPtr;
    diagInfoPtr->len    = pos;

    le_event_QueueFunctionToThread(mainThrRef, RequestUdsMessage, diagInfoPtr, NULL);

    taf_doipDiagConfirmHandler_t*    handler;
    handler = &sessionPtr->diagConfirmHandler;

    le_mutex_Lock(handler->mutexRef);

    if (handler->funcPtr != NULL)
    {
        addrInfo.sa = sa;
        addrInfo.ta = ta;
        addrInfo.taType = taType;

        // Confirm the request result.
        handler->funcPtr(&addrInfo, TAF_DOIP_RESULT_OK, handler->ctxPtr);
    }
    le_mutex_Unlock(handler->mutexRef);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SendUDPData
 DESCRIPTION     Send udp unicast packet.
 PARAMETERS      [IN]sockRef: udp socket reference
                 [IN]desIpPtr: destination ip address pointer
                 [IN]desPort: destination port
                 [IN]messagePtr: message pointer
                 [IN]len: message length
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SendUDPData
(
    le_socket_Ref_t sockRef,
    char*           desIpPtr,
    uint16_t        desPort,
    char*           messagePtr,
    uint32_t        len
)
{
    le_socket_Ref_t sndSockRef;

    if (desIpPtr == NULL || messagePtr == NULL || len == 0)
    {
        LE_ERROR("Parameter error.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    sndSockRef = le_socket_Create(desIpPtr, desPort, localIp, UDP_TYPE);
    if (sndSockRef == NULL)
    {
        LE_ERROR("Failed to create udp socket reference.\n");
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    if (le_socket_Connect(sndSockRef) != LE_OK)
    {
        LE_ERROR("Failed to connect %s:%d.", desIpPtr, desPort);
        le_socket_Delete(sndSockRef);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    le_socket_SetTimeout(sndSockRef, SOCKET_TIMEOUT_MS);

    if (le_socket_Send(sndSockRef, messagePtr, len) != LE_OK)
    {
        LE_ERROR("Unable to transmit multicast packet.");
        le_socket_Delete(sndSockRef);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    le_socket_Delete(sndSockRef);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SendUDPData
 DESCRIPTION     Send udp multicast packet.
 PARAMETERS      [IN]sockRef: udp socket reference
                 [IN]messagePtr: message pointer
                 [IN]len: message length
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SendUDPData
(
    le_socket_Ref_t sockRef,
    char*           messagePtr,
    uint32_t        len
)
{
    char netType[TAF_DOIP_IPTYPE_MAX_LEN];
    uint16_t udpDiscoveryPort;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (messagePtr == NULL || len == 0)
    {
        LE_ERROR("Parameter error.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    vehicleMgr.GetNetType(netType);
    vehicleMgr.GetUdpPort(&udpDiscoveryPort);

    if (!strncasecmp(netType, "IPv4", 4))
    {
        // [DoIP-125]: Set target IPv4 address to limited broadcast address.
        if (le_socket_SendTo(udpDiscoverSockRef, messagePtr, len,
            (char*)TAF_DOIP_UDP_BROADCAST_IP, udpDiscoveryPort) != LE_OK)
        {
            LE_ERROR("Unable to transmit multicast packet.");
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }
    }
    else
    {
        // [DoIP-155]: Set target IPv6 address to link-local scope multicast address.
        if (le_socket_SendTo(udpDiscoverSockRef, messagePtr, len,
            (char*)TAF_DOIP_UDP6_BROADCAST_IP, udpDiscoveryPort) != LE_OK)
        {
            LE_ERROR("Unable to transmit multicast packet.");
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }
    }

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::RecvUdpData
 DESCRIPTION     Receive udp data from UDP_DISCOVERY.
 PARAMETERS      [IN]sockRef: Receive data from socket reference
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::RecvUdpData
(
    le_socket_Ref_t sockRef
)
{
    le_result_t ret;
    size_t      received = TAF_DOIP_MAX_BUFFER_SIZE;
    char        remoteIp[TAF_DOIP_IP_ADDR_MAX_LEN];
    uint16_t    remotePort;

    // Receive udp packet and get the remote address.
    ret = le_socket_RecvFrom(sockRef, udpDataBuffer, &received,
        remoteIp, TAF_DOIP_IP_ADDR_MAX_LEN, &remotePort);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to receive data from socket. Error code is %d\n", ret);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    // Debug
    LE_DEBUG("UDP data info from socket%p(%s:%d):", sockRef, remoteIp, remotePort);
    for (size_t i = 0; i < received; i++)
    {
        LE_DEBUG("0x%x", udpDataBuffer[i]);
    }

    // Filter local annoucement message.
    if (strcmp(remoteIp, localIp) == 0 || strcmp(remoteIp, "0.0.0.0") == 0)
    {
        return TAF_DOIP_RESULT_OK;
    }

    udpDataLen = received;

    if (udpDataLen < TAF_DOIP_HEADER_GENERIC_LENGTH) {
        // The message is too short, discard it.
        LE_ERROR("DoIP message is invalid! [Data size is less than 8 bytes].");
        return TAF_DOIP_RESULT_MESSAGE_TOO_SHORT;
    }

    auto& parser = ProtocolParser::GetInstance();
    taf_doipHeader_t hdr;

    parser.UnpackHeaderStruct(udpDataBuffer, udpDataLen, &hdr);

    if (CheckDoipHeaderOverUdp(hdr, remoteIp, remotePort) != 0)
    {
        LE_ERROR("DoIP header error!, ret=0x%x\n", ret);
        return TAF_DOIP_RESULT_HDR_ERROR;
    }

    EntityProcessDoipMessageOverUdp(hdr.payloadType, udpDataBuffer + TAF_DOIP_HEADER_GENERIC_LENGTH,
        hdr.payloadLen, remoteIp, remotePort);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SendTCPData
 DESCRIPTION     Send tcp packet.
 PARAMETERS      [IN]dataPtr: uds data pointer
                 [IN]length: the length of uds data
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SendTCPData
(
    le_socket_Ref_t sockRef,
    char*           messagePtr,
    uint32_t        len
)
{
    std::shared_ptr<Connection> connection;
    taf_doip_Buffer_t*          doipBuf;

    if (sockRef == NULL || messagePtr == NULL || len == 0 || len >= TAF_DOIP_MAX_BUFFER_SIZE)
    {
        LE_ERROR("Parameter error.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto&   cmMgr = CommunicationMgr::GetInstance();
    doipBuf = (taf_doip_Buffer_t*)le_mem_ForceAlloc(cmMgr.connectionMgrPtr->outMsgPool);

    connection = cmMgr.connectionMgrPtr->FindConnectionBySocket(sockRef);
    if (connection == nullptr)
    {
        LE_ERROR("Failed to find connection.\n");
        le_mem_Release(doipBuf);
        return TAF_DOIP_RESULT_ERROR;
    }

    // Fill DoIP buffer data.
    memcpy(doipBuf->data, messagePtr, len);
    doipBuf->dataSize   = len;
    doipBuf->dataPos    = 0;

    connection->SendDoipMessage(doipBuf);

    le_mem_Release(doipBuf);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CheckDoipHeaderOverUdp
 DESCRIPTION     Check the DoIP message which is over udp.
 PARAMETERS      [IN]dataPtr: uds data pointer
                 [IN]length: the length of uds data
 RETURN VALUE    taf_doip_Result_t: Result of inform uds message
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::CheckDoipHeaderOverUdp
(
    taf_doipHeader_t&   header,
    char*               ipPtr,
    uint16_t            port
)
{
    uint32_t    mds;
    taf_doipHeaderNACKCode_t    nackCode;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2010 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2012 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2019 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_DEFAULT)
    {
        // [DoIP-041] NACK code set to 0x00 when protocol version does not match.
        LE_ERROR("Protocol version[0x%02x] doesn't support!\n", header.protocolVer);
        nackCode = TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT;
        goto errOut;
    }

    if (header.protocolVer != (uint8_t)(~header.invProtocolVer))
    {
        // [DoIP-041] NACK code set to 0x00 when inverse protocol version does not match.
        LE_ERROR("Incorrect pattern format!version is [0x%02x], inverse version is [0x%02x]\n",
            header.protocolVer, header.invProtocolVer);
        nackCode = TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT;
        goto errOut;
    }

    // We are DoIP entity.
    if (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST)
    {
        // [DoIP-042] NACK code set to 0x01 if the payload type is not supported.
        LE_ERROR("Unknow payload type!payload type is [0x%x]\n", header.payloadType);
        nackCode = TAF_DOIP_HEADER_NACK_UNKNOWN_PAYLOAD_TYPE;
        goto errOut;
    }

    if (vehicleMgr.GetMaxDataSize(&mds) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max data size, using default!\n");
        mds = TAF_DOIP_MDS_DEFAULT;
    }

    if (header.payloadLen > mds)
    {
        // [DoIP-043] NACK code set to 0x02 if the payload length exceeds
        // the maximum DoIP message size.
        LE_ERROR("DoIP message is too large! payload len is %d, mds is %d\n",
                header.payloadLen, mds);
        nackCode = TAF_DOIP_HEADER_NACK_MESSAGE_TOO_LARGE;
        goto errOut;
    }

    return TAF_DOIP_RESULT_OK;
errOut:
    if (instanceType == TAF_DOIP_INSTANCE_TYPE_TESTER)
    {
        // [DoIP-040] Tester shall not send DoIP header negative ack.
        return TAF_DOIP_RESULT_HDR_ERROR;
    }

    // [DoIP-038],[DoIP-087]
    auto& parser = ProtocolParser::GetInstance();
    taf_doipLink_t link;

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;
    parser.HeaderNegativeACK(&link, nackCode);

    return TAF_DOIP_RESULT_HDR_ERROR;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::EntityProcessDoipMessageOverUdp
 DESCRIPTION     Process DoIP message which is over udp.
 PARAMETERS      [IN]payloadType: DoIP message type
                 [IN]payload: DoIP message payload
                 [IN]payloadLen: DoIP message payload length
                 [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::EntityProcessDoipMessageOverUdp
(
    uint16_t    payloadType,
    char*       payload,
    uint32_t    payloadLen,
    const char* ipPtr,
    uint16_t    port
)
{
    switch (payloadType)
    {
    case TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST:
    {
        LE_DEBUG("Vehicle identify request is receiving.\n");
        VehicleIdentifyReqHandler(ipPtr, port);
        break;
    }
    case TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID:
    {
        LE_DEBUG("Vehicle identify request with EID is receiving.\n");
        VehicleIdentifyReqWithEidHandler(payload, payloadLen, ipPtr, port);
        break;
    }
    case TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN:
    {
        LE_DEBUG("Vehicle identify request with VIN is receiving.\n");
        VehicleIdentifyReqWithVinHandler(payload, payloadLen, ipPtr, port);
        break;
    }
    case TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST:
    {
        LE_DEBUG("Entity status request is receiving.\n");
        EntityStatusReqHandler(ipPtr, port);
        break;
    }
    case TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST:
    {
        LE_DEBUG("Power mode information request is receiving.\n");
        PowerModeReqHandler(ipPtr, port);
        break;
    }
    default:
        LE_DEBUG("Unsupport payload type-0x%x.\n", payloadType);
        break;
    }

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::VehicleIdentifyReqHandler
 DESCRIPTION     vehicle identify request handler.
 PARAMETERS      [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::VehicleIdentifyReqHandler
(
    const char* ipPtr,
    uint16_t    port
)
{
    taf_doipLink_t link;
    auto& vehicleMgr = VehicleManager::GetInstance();
    auto& parser = ProtocolParser::GetInstance();

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;

    uint16_t    sourceAddr;
    if (vehicleMgr.GetEntityLogicalAddr(&sourceAddr) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get entity source address.\n");
        return;
    }

    if (vehicleMgr.GetAuthEnableStatus())
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::VehicleIdentifyReqWithEidHandler
 DESCRIPTION     vehicle identify request with EID handler.
 PARAMETERS      [IN]payload: DoIP message payload
                 [IN]payloadLen: DoIP message payload length
                 [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::VehicleIdentifyReqWithEidHandler
(
    char*       payload,
    uint32_t    payloadLen,
    const char* ipPtr,
    uint16_t    port
)
{
    char ourEid[TAF_DOIP_EID_SIZE + 1];
    taf_doipLink_t link;
    auto& vehicleMgr = VehicleManager::GetInstance();
    auto& parser = ProtocolParser::GetInstance();

    if (payloadLen < TAF_DOIP_EID_SIZE)
    {
        LE_ERROR("EID is not enough in vehicle identification request.\n");
        return;
    }

    if (vehicleMgr.GetEid(ourEid) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get our EID.\n");
        return;
    }

    if (memcmp(ourEid, payload, TAF_DOIP_EID_SIZE) != 0)
    {
        LE_ERROR("EID unmatch.\n");
        return;
    }

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;

    uint16_t    sourceAddr;
    if (vehicleMgr.GetEntityLogicalAddr(&sourceAddr) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get entity source address.\n");
        return;
    }

    if (vehicleMgr.GetAuthEnableStatus())
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::VehicleIdentifyReqWithVinHandler
 DESCRIPTION     vehicle identify request with VIN handler.
 PARAMETERS      [IN]payload: DoIP message payload
                 [IN]payloadLen: DoIP message payload length
                 [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::VehicleIdentifyReqWithVinHandler
(
    char*       payload,
    uint32_t    payloadLen,
    const char* ipPtr,
    uint16_t    port
)
{
    char ourVin[TAF_DOIP_VIN_SIZE + 1];
    taf_doipLink_t link;
    auto& vehicleMgr = VehicleManager::GetInstance();
    auto& parser = ProtocolParser::GetInstance();

    if (payloadLen < TAF_DOIP_VIN_SIZE)
    {
        LE_ERROR("VIN is not enough in vehicle identification request.\n");
        return;
    }

    if (vehicleMgr.GetVin(ourVin) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get our VIN.\n");
        return;
    }

    if (memcmp(ourVin, payload, TAF_DOIP_VIN_SIZE) != 0)
    {
        LE_ERROR("VIN unmatch.\n");
        return;
    }

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;

    uint16_t    sourceAddr;
    if (vehicleMgr.GetEntityLogicalAddr(&sourceAddr) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get entity source address.\n");
        return;
    }

    if (vehicleMgr.GetAuthEnableStatus())
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else
    {
        parser.VehicleAnnounceOrIdRes(&link, sourceAddr, TAF_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::EntityStatusReqHandler
 DESCRIPTION     Entity status request handler.
 PARAMETERS      [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::EntityStatusReqHandler
(
    const char* ipPtr,
    uint16_t    port
)
{
    uint32_t    mcts;
    uint32_t    mds;
    uint32_t    ncts;
    taf_doipLink_t link;
    auto& vehicleMgr = VehicleManager::GetInstance();
    auto& parser = ProtocolParser::GetInstance();

    if (vehicleMgr.GetMaxConcurrentSockNum(&mcts))
    {
        mcts = TAF_DOIP_MCTS_DEFAULT;
    }

    if (vehicleMgr.GetMaxDataSize(&mds))
    {
        mds = TAF_DOIP_MDS_DEFAULT;
    }

    ncts = connectionMgrPtr->GetConnectionNum();

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;
    parser.DoIPEntityStatusRes(&link, TAF_DOIP_NT_NODE, mcts, ncts, mds);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::PowerModeReqHandler
 DESCRIPTION     Power mode request handler.
 PARAMETERS      [IN]ipPtr: The sender's IP address
                 [IN]port: The sender's port
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::PowerModeReqHandler
(
    const char* ipPtr,
    uint16_t    port
)
{
    taf_doipLink_t link;
    auto& parser = ProtocolParser::GetInstance();

    taf_doip_PowerMode_t mode;
    mode = QueryPowerMode();

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;
    parser.DiagPowerModeRes(&link, (uint8_t)mode);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::UdpDiscoverSocketEventCallback
 DESCRIPTION     process the async events on TCP_DATA socket.
 PARAMETERS      [IN] sockRef: Socket context reference.
                 [IN] events: Bitmap of events that occurred.
                 [IN] userPtr: User data pointer
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::UdpDiscoverSocketEventCallback
(
    le_socket_Ref_t sockRef,
    short           events,
    void*           userPtr
)
{
    auto&   cmMgr = CommunicationMgr::GetInstance();

    LE_DEBUG("Socket%p event0x%x is coming...\n", sockRef, events);

    if (events & POLLIN)
    {
        // Receive message.
        cmMgr.RecvUdpData(sockRef);
    }
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::TcpDataSocketEventCallback
 DESCRIPTION     process the async events on TCP_DATA socket.
 PARAMETERS      [IN] sockRef: Socket context reference.
                 [IN] events: Bitmap of events that occurred.
                 [IN] userPtr: User data pointer
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::TcpDataSocketEventCallback
(
    le_socket_Ref_t sockRef,
    short           events,
    void*           userPtr
)
{
    le_socket_Ref_t childSockRef;
    char    ip[TAF_DOIP_IP_ADDR_MAX_LEN];
    int     port;
    auto&   commMgr = CommunicationMgr::GetInstance();

    if ((events & POLLIN) != POLLIN)
    {
        LE_DEBUG("Events bitmap is 0x%x", events);
        return;
    }

    childSockRef = le_socket_Accept(sockRef, ip, TAF_DOIP_IP_ADDR_MAX_LEN, &port);
    if (childSockRef == NULL)
    {
        LE_ERROR("Failed to accept client connection.");
        return;
    }
    LE_INFO("Accept a connection(%s:%d)", ip, port);

    commMgr.connectionMgrPtr->FindOrCreateConnection(childSockRef, ip, port);

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::IndicateUdsMessage
 DESCRIPTION     Indicate the received uds to the Uds handle thread.
 PARAMETERS      [IN] dataPtr: uds data
                 [IN] length: uds data length
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::IndicateUdsMessage
(
    void* param1Ptr,
    void* param2Ptr
)
{
    taf_doipSession_t*      sessionPtr;
    taf_doip_AddrInfo_t     addrInfo;
    taf_doip_DiagMsg_t      diagMsg;
    taf_doipDiagDataInfo_t* dataInfoPtr = (taf_doipDiagDataInfo_t*)param1Ptr;

    auto&   cmMgr = CommunicationMgr::GetInstance();
    sessionPtr = cmMgr.FindDoipSession(dataInfoPtr->ta);
    if (sessionPtr != NULL)
    {
        taf_doipIndicationHandler_t*    handler;
        handler = &sessionPtr->diagIndicationHandler;

        le_mutex_Lock(handler->mutexRef);

        if (handler->funcPtr != NULL)
        {
            addrInfo.sa = dataInfoPtr->sa;
            addrInfo.ta = dataInfoPtr->ta;
            addrInfo.taType = (taf_doip_TaType_t)dataInfoPtr->taType;
            diagMsg.dataPtr = (uint8_t*)dataInfoPtr->data + 2 * TAF_DOIP_LOGICAL_ADDRESS_LENGTH;
            diagMsg.dataLen = dataInfoPtr->len - 2 * TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

            // Indicate reception message to upper layer.
            handler->funcPtr(&addrInfo, &diagMsg, TAF_DOIP_RESULT_OK, handler->ctxPtr);
        }
        le_mutex_Unlock(handler->mutexRef);
    }

    le_mem_Release(dataInfoPtr->data);
    le_mem_Release(dataInfoPtr);

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::ReportConnectionEvent
 DESCRIPTION     Indicate connection registered event.
 PARAMETERS      [IN] sa: Source address.
                 [IN] ta: Target address.
                 [IN] rgistResult: Connection registered state.
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::ReportConnectionEvent
(
    uint16_t sa,
    uint16_t ta,
    taf_doip_Result_t rgistResult
)
{
    taf_doipSession_t*      sessionPtr;
    taf_doip_AddrInfo_t     addrInfo;
    taf_doip_DiagIndicationHandlerFunc_t handlerFunc;

    auto&   cmMgr = CommunicationMgr::GetInstance();
    sessionPtr = cmMgr.FindDoipSession(ta);
    if (sessionPtr != NULL)
    {
        taf_doipIndicationHandler_t*    handler;
        handler = &sessionPtr->diagIndicationHandler;

        le_mutex_Lock(handler->mutexRef);
        handlerFunc = handler->funcPtr;
        le_mutex_Unlock(handler->mutexRef);

        if (handlerFunc != NULL)
        {
            addrInfo.sa = sa;
            addrInfo.ta = ta;
            addrInfo.taType = TAF_DOIP_TA_TYPE_PHYSICAL;

            // Indicate reception message to upper layer.
            handlerFunc(&addrInfo, NULL, rgistResult, handler->ctxPtr);
        }
    }

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::RequestUdsMessage
 DESCRIPTION     Indicate the received uds to the Uds handle thread.
 PARAMETERS      [IN] dataPtr: uds data
                 [IN] length: uds data length
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::RequestUdsMessage
(
    void* param1Ptr,
    void* param2Ptr
)
{
    std::shared_ptr<Connection> connection;
    taf_doipDiagDataInfo_t *dataInfoPtr = (taf_doipDiagDataInfo_t*)param1Ptr;

    auto&   cmMgr = CommunicationMgr::GetInstance();

    connection = cmMgr.connectionMgrPtr->FindConnectionByLogicalAddr(dataInfoPtr->ta);
    connection->SendDiagMessage(dataInfoPtr->data, dataInfoPtr->len);

    le_mem_Release(dataInfoPtr->data);
    le_mem_Release(dataInfoPtr);

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::UdsHandleThread
 DESCRIPTION     Initialization of DoIP communication manager
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
=================================================================================================*/
void* CommunicationMgr::UdsHandleThread
(
    void* contextPtr
)
{
    le_event_RunLoop();

    return NULL;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SessionInit
 DESCRIPTION     Initialization of DoIP session resource
                 in communication manager
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of initialization
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SessionInit
(
)
{
    auto& vehicleMgr = VehicleManager::GetInstance();
    char netType[TAF_DOIP_IPTYPE_MAX_LEN];
    char ifname[TAF_DOIP_INTERFACE_NAME_MAX_LEN];
    uint16_t    udpDiscoveryPort;
    uint16_t    tcpDataPort;
    taf_doip_Result_t result = TAF_DOIP_RESULT_ERROR;

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetNetType(netType))
    {
        le_utf8_Copy(netType, TAF_DOIP_IPTYPE_DEFAULT,
                     TAF_DOIP_IPTYPE_MAX_LEN, NULL);
    }

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetIfName(ifname))
    {
        le_utf8_Copy(ifname, TAF_DOIP_INTERFACE_DEFAULT,
                TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
    }

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetTcpPort(&tcpDataPort))
    {
        tcpDataPort = TAF_DOIP_TCP_DATA_DEFAULT;
    }

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetUdpPort(&udpDiscoveryPort))
    {
        udpDiscoveryPort = TAF_DOIP_UDP_DISCOVERY_DEFAULT;
    }

    LE_DEBUG("TCP_DATA is %d, UDP_DISCOVEERY is %d, type is %s\n",
            tcpDataPort, udpDiscoveryPort, netType);

    if (!strncasecmp(netType, "IPv4", 4))
    {
        std::string ifNameStr = ifname;
        if (GetLocalIPv4Addr(ifNameStr, localIp, sizeof(localIp)) != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Can not get local IPv4 address.\n");
            goto errOut;
        }

        LE_INFO("Get IPv4-%s\n", localIp);

        tcpDataSockRef = le_socket_Create(NULL, tcpDataPort, localIp, TCP_TYPE);
        if (tcpDataSockRef == NULL)
        {
            LE_ERROR("Failed to create tcp socket reference.\n");
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }

        udpDiscoverSockRef = le_socket_Create(NULL, udpDiscoveryPort, localIp, UDP_TYPE);
        if (udpDiscoverSockRef == NULL)
        {
            LE_ERROR("Failed to create udp socket reference.\n");
            result = TAF_DOIP_RESULT_NETWORK_ERROR;
            goto errOut;
        }

        if (le_socket_Bind(udpDiscoverSockRef) != LE_OK)
        {
            LE_ERROR("Failed to bind with port%d", udpDiscoveryPort);
            result = TAF_DOIP_RESULT_NETWORK_ERROR;
            goto errOut;
        }
    }
    else
    {
        std::string ifNameStr = ifname;
        if (GetLocalIPv6Addr(ifNameStr, localIp, sizeof(localIp)) != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Can not get local IPv6 address.\n");
            goto errOut;
        }

        LE_INFO("Get IPv6-%s\n", localIp);

        tcpDataSockRef = le_socket_Create(NULL, tcpDataPort, localIp, TCP_TYPE);
        if (tcpDataSockRef == NULL)
        {
            LE_ERROR("Failed to create tcp socket reference.\n");
            result = TAF_DOIP_RESULT_NETWORK_ERROR;
            goto errOut;
        }

        udpDiscoverSockRef = le_socket_Create(NULL, udpDiscoveryPort, localIp, UDP_TYPE);
        if (udpDiscoverSockRef == NULL)
        {
            LE_ERROR("Failed to create udp socket reference.\n");
            result = TAF_DOIP_RESULT_NETWORK_ERROR;
            goto errOut;
        }

        if (le_socket_Bind(udpDiscoverSockRef) != LE_OK)
        {
            LE_ERROR("Failed to bind with port%d", udpDiscoveryPort);
            result = TAF_DOIP_RESULT_NETWORK_ERROR;
            goto errOut;
        }

        if (le_socket_JoinMulticastGroup(udpDiscoverSockRef,
            (const char*)TAF_DOIP_UDP6_BROADCAST_IP) != LE_OK)
        {
            LE_ERROR("Failed to join multicast group for IPv6.");
            goto errOut;
        }
    }

    LE_DEBUG("tcp socket ref is %p, discovery socket ref is %p",
        tcpDataSockRef, udpDiscoverSockRef);
    le_socket_SetTimeout(tcpDataSockRef, SOCKET_TIMEOUT_MS);

    // Set the socket event callback for tcp listener fd monitor.
    le_socket_AddEventHandler(tcpDataSockRef, TcpDataSocketEventCallback, NULL);
    if (le_socket_Bind(tcpDataSockRef) != LE_OK)
    {
        LE_ERROR("le_socket_Bind error!\n");
        goto errOut;
    }

    le_socket_SetTimeout(udpDiscoverSockRef, SOCKET_TIMEOUT_MS);

    // Set the socket event callback for discovery fd monitor.
    le_socket_AddEventHandler(udpDiscoverSockRef, UdpDiscoverSocketEventCallback, NULL);

    // Initialize TCP connection manager.
    connectionMgrPtr->Init();

    // Initialzie the reception and sending thread.
    mainThrRef = le_thread_GetCurrent();

    state = TAF_DOIP_STATE_INIT;

    return TAF_DOIP_RESULT_OK;

errOut:
    if (tcpDataSockRef != NULL)
    {
        le_socket_Delete(tcpDataSockRef);
        tcpDataSockRef = NULL;
    }

    if (udpDiscoverSockRef != NULL)
    {
        le_socket_Delete(udpDiscoverSockRef);
        udpDiscoverSockRef = NULL;
    }

    return result;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::SessionDeinit
 DESCRIPTION     Deinitialization of DoIP session resource
                 in communication manager
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of initialization
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::SessionDeInit()
{
    if (state == TAF_DOIP_STATE_RUNNING || state == TAF_DOIP_STATE_START)
    {
        SessionStop();
    }

    le_socket_Delete(tcpDataSockRef);
    tcpDataSockRef = NULL;
    le_socket_Delete(udpDiscoverSockRef);
    udpDiscoverSockRef = NULL;

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::Init
 DESCRIPTION     Initialization of DoIP communication manager
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of initialization
=================================================================================================*/
void CommunicationMgr::Init
(
)
{
    LE_INFO("DoIP communication manager init...\n");

    udsThrRef = le_thread_Create("UdsThread", UdsHandleThread, NULL);
    le_thread_SetStackSize(udsThrRef, TAF_DOIP_THREAD_STACK_SIZE);
    le_thread_Start(udsThrRef);

    udpMsgPool = le_mem_CreatePool("DoipUdpMsgPool", TAF_DOIP_MAX_BUFFER_SIZE);
    udsInfoPool = le_mem_CreatePool("DoipUdsInfoPool", sizeof(taf_doipDiagDataInfo_t));

    doipSessionPool = le_mem_CreatePool("DoipSessionPool", sizeof(taf_doipSession_t));
    doipSessionRefMap = le_ref_CreateMap("DoipSessionRefMap", TAF_DOIP_MAX_ENTITY_NUM);
    doipHandlerRefMap = le_ref_CreateMap("DoipHandlerRefMap", TAF_DOIP_MAX_USER_HANDLER_NUM);

    // Initialized vehicle deiscovery.
    auto& vehicleDisovery = VehicleDiscovery::GetInstance();
    vehicleDisovery.Init();

    LE_INFO("DoIP communication manager ok...\n");

    return;
}