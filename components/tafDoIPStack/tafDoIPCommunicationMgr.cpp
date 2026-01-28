/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <errno.h>
#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
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

    for (int i = 0; i < MAX_INF_NUM; ++i)
    {
        tcpDataSockRef[i] = NULL;
        udpEquipSockRef[i] = NULL;
        memset(&localIpInfo[i], 0, sizeof(taf_doip_IPInfo_t));
    }
    udpDiscoverSockRef = NULL;
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
    int                     af,
    std::string&            ifname,
    taf_doip_IPInfo_t*      ipInfo,
    socklen_t               size
)
{
    struct ifaddrs  *ifaPtr, *tmpPtr;

    LE_DEBUG("Enter GetLocalIPAddr, family address is %d, iface is %s",
        af, ifname.c_str());

    if (ipInfo == NULL || (af != AF_INET && af != AF_INET6))
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
            LE_DEBUG("Address family is unmatch, skip.\n");
            continue;
        }
        LE_DEBUG("Start to get IP address, address family is %d.\n", af);

        if (af == AF_INET6)
        {
            struct in6_addr addr6;
            addr6 = (((struct sockaddr_in6 *)(tmpPtr->ifa_addr))->sin6_addr);

            if (IN6_IS_ADDR_LINKLOCAL(&addr6))
            {
                if (inet_ntop(af, &addr6, ipInfo->localIp, size) == NULL)
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
            struct in_addr netmaskAddr;
            addr = (((struct sockaddr_in *)(tmpPtr->ifa_addr))->sin_addr);
            netmaskAddr = (((struct sockaddr_in *)(tmpPtr->ifa_netmask))->sin_addr);

            if (inet_ntop(af, &addr, ipInfo->localIp, size) == NULL)
            {
                LE_ERROR("Can not get IPv4 address for %s.", ifname.c_str());
                freeifaddrs(ifaPtr);
                return TAF_DOIP_RESULT_ERROR;
            }
            ipInfo->ipv4_s_addr = addr.s_addr;
            ipInfo->ipv4_mask_s_addr = netmaskAddr.s_addr;
            break;
        }
    }

    LE_DEBUG("IP is %s\n", ipInfo->localIp);

    freeifaddrs(ifaPtr);

    if (tmpPtr == NULL)
    {
        LE_DEBUG("Can't find interface %s\n", ifname.c_str());
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
    std::string&             ifname,
    taf_doip_IPInfo_t*       ipInfo,
    socklen_t                size
)
{
    auto&   vehicleMgr = VehicleManager::GetInstance();

    //If announceWait is configured, run announcement mechanisam to get IP address.
    if ( vehicleMgr.GetAnnounceWait() )
    {
        LE_DEBUG("Run Announcement wait mechanisam to get IP address");
        return GetIpAddrWithAnnounceWaitMech(AF_INET6, ifname, ipInfo, size);
    }

    return GetLocalIPAddr(AF_INET6, ifname, ipInfo, size);
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
    std::string&            ifname,
    taf_doip_IPInfo_t*      ipInfo,
    socklen_t               size
)
{
    auto&   vehicleMgr = VehicleManager::GetInstance();

    //If announceWait is configured, run announcement mechanisam to get IP address.
    if ( vehicleMgr.GetAnnounceWait() )
    {
        LE_DEBUG("Run Announcement wait mechanisam to get IP address");
        return GetIpAddrWithAnnounceWaitMech(AF_INET, ifname, ipInfo, size);
    }

    return GetLocalIPAddr(AF_INET, ifname, ipInfo, size);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetIpAddrWithAnnounceWaitMech
 DESCRIPTION     Run announcement mechanisam to get IP address.
 PARAMETERS      [IN] af: IP address family
                 [IN] ifname: interface name reference
                 [OUT] ip: IPv4 address buffer.
                 [IN] size: IP address buffer length
 RETURN VALUE    taf_doip_Result_t: Result of geting local IP address
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::GetIpAddrWithAnnounceWaitMech
(
    int                     af,
    std::string&            ifname,
    taf_doip_IPInfo_t*      ipInfo,
    socklen_t               size
)
{
    if (ipInfo == NULL || (af != AF_INET && af != AF_INET6))
    {
        LE_ERROR("Parameter error!\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    uint16_t getIpWaitTime = 0;
    while(getIpWaitTime <= TAF_DOIP_MAX_GET_IP_WAIT_TIME)
    {
        if(GetLocalIPAddr(af, ifname, ipInfo, size) == TAF_DOIP_RESULT_OK)
        {
            return TAF_DOIP_RESULT_OK;
        }

        // Get a random time less than 10 miliseconds.
        uint8_t waitTime = le_rand_GetNumBetween(0, MAX_CUSTOMIZED_ANNOUNCE_WAIT_TIME);
        usleep(waitTime * 1000);

        // Increase getIpWaitTime with sleep time and the time consumed by code execution
        getIpWaitTime = getIpWaitTime + waitTime + TIME_CONSUMED_BY_CODE_EXECUTION;
    }

    LE_FATAL("Could not get ip address for interface %s", ifname.c_str());

    return TAF_DOIP_RESULT_ERROR;

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

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(cmMgr.doipSessionRefMap);

    LE_DEBUG("FindDoipSession, iter ref is %p.", iterRef);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        doipSessionPtr = (taf_doipSession_t *)le_ref_GetValue(iterRef);
        if (doipSessionPtr != NULL
            && sa == doipSessionPtr->logicalSrcAddr)
        {
            LE_DEBUG("found a doip session%p, SA is '%d'", doipSessionPtr, sa);
            isFound = true;
            break;
        }
        else
        {
            // Trace info
            if (doipSessionPtr != NULL)
            {
                LE_DEBUG("Get a doip session%p with SA'%d', but we found the '%d'",
                    doipSessionPtr, doipSessionPtr->logicalSrcAddr, sa);
            }
            else
            {
                LE_DEBUG("DoIP Session pointer is NULL");
            }
        }
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

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

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
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
                le_mutex_Unlock(cmMgr.doipSessionRefMutex);
                return handler->funcPtr(handler->ctxPtr);
            }
        }
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

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
    LE_DEBUG("Enter CommunicationMgr::SessionStart-state%d.\n", state);

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

    if (udpDiscoverSockRef != NULL)
    {
        if (LE_OK != le_socket_SetMonitoring(udpDiscoverSockRef, true))
        {
           LE_ERROR("Failed to enable udp discovery socket monitor.");
           return TAF_DOIP_RESULT_ERROR;
        }
    }

    for (uint32_t i = 0; i < MAX_INF_NUM; i++)
    {
        if (tcpDataSockRef[i] != NULL)
        {
            // Enable async mode and start fd monitor.
            if (LE_OK != le_socket_SetMonitoring(tcpDataSockRef[i], true))
            {
               LE_ERROR("Failed to enable tcp data socket monitor.");
               return TAF_DOIP_RESULT_ERROR;
            }
        }

        if (udpEquipSockRef[i] != NULL)
        {
            if (LE_OK != le_socket_SetMonitoring(udpEquipSockRef[i], true))
            {
               LE_ERROR("Failed to enable udp discovery socket monitor.");
               return TAF_DOIP_RESULT_ERROR;
            }
        }
    }

    state = TAF_DOIP_STATE_RUNNING;
    LE_DEBUG("DoIP session start success.!");

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
    LE_DEBUG("Enter CommunicationMgr::SessionStop-state%d.\n", state);

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

    for (uint32_t i = 0; i < MAX_INF_NUM; ++i)
    {
        if (tcpDataSockRef[i] != NULL)
        {
            le_socket_SetMonitoring(tcpDataSockRef[i], false);
            LE_DEBUG("tcpDataSockRef=%p\n", tcpDataSockRef[i]);
        }

        if (udpEquipSockRef[i] != NULL)
        {
            le_socket_SetMonitoring(udpEquipSockRef[i], false);
            LE_DEBUG("udpEquipSockRef=%p\n", udpEquipSockRef[i]);
        }
    }

    if (udpDiscoverSockRef != NULL)
    {
        le_socket_SetMonitoring(udpDiscoverSockRef, false);
        LE_DEBUG("udpDiscoverSockRef=%p\n", udpDiscoverSockRef);
    }

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
    uint32_t    length,
    const char* ifacePtr
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
    le_utf8_Copy(diagInfoPtr->ifName, ifacePtr, TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
    diagInfoPtr->vlanId = GetVlanId(ifacePtr);
    LE_DEBUG("UDS message is coming from %s(with vlan id %d)", ifacePtr, diagInfoPtr->vlanId);

    auto &vehicleMgr = VehicleManager::GetInstance();
    uint16_t sourceAddr = 0;
    vehicleMgr.GetEntityLogicalAddr(&sourceAddr);

    if (vehicleMgr.IsFunctionalAddress(ta))
    {
        diagInfoPtr->ta = sourceAddr;  // Use our logical address.
        diagInfoPtr->taType = TAF_DOIP_TA_TYPE_FUNCTIONAL;
    }
    else
    {
        if (ta != sourceAddr)
        {
            LE_ERROR("Invalid TA(0x%x). Expect 0x%x\n", ta, sourceAddr);
            return TAF_DOIP_RESULT_UNKNOWN_TA;
        }

        diagInfoPtr->ta = ta;
        diagInfoPtr->taType = TAF_DOIP_TA_TYPE_PHYSICAL;
    }

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
    taf_doipDiagDataInfo_t* diagAddrInfoPtr;
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
    diagAddrInfoPtr = (taf_doipDiagDataInfo_t*)le_mem_ForceAlloc(udsInfoPool);

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

    diagAddrInfoPtr->sa = sa;
    diagAddrInfoPtr->ta = ta;
    diagAddrInfoPtr->taType = taType;
    le_event_QueueFunctionToThread(udsThrRef, ConfirmUserMessage, diagAddrInfoPtr, NULL);
    LE_DEBUG("TransUdsMessage done.");

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

    struct sockaddr_storage socketAddrInfo;
    char local[TAF_DOIP_IP_ADDR_MAX_LEN];
    if (inet_pton(AF_INET, desIpPtr, &(((struct sockaddr_in*)&socketAddrInfo)->sin_addr)) == 1)
    {
        if (GetLocalIPv4FromSource((struct sockaddr_in*)&socketAddrInfo, local)
            != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to get local ip from source ip.\n");
            return TAF_DOIP_RESULT_ERROR;
        }
    }
    else if (inet_pton(AF_INET6, desIpPtr, &(((struct sockaddr_in6*)&socketAddrInfo)->sin6_addr)) == 1)
    {
        if (GetLocalIPv6FromSource((struct sockaddr_in6*)&socketAddrInfo, local)
            != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to get local ip from source ip.\n");
            return TAF_DOIP_RESULT_ERROR;
        }
    }
    else
    {
        LE_ERROR("inet_pton error.");
        return TAF_DOIP_RESULT_ERROR;
    }

    uint16_t localPort;
    auto& vehicleMgr = VehicleManager::GetInstance();
    taf_doip_Result_t ret = vehicleMgr.GetUdpSrcPort(&localPort);
    if ((ret == TAF_DOIP_RESULT_OK) && (localPort != 0))
    {
        sndSockRef = le_socket_Create(NULL, localPort, local, UDP_TYPE);
        if (sndSockRef == NULL)
        {
            LE_ERROR("Failed to create udp socket reference.\n");
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }

        le_socket_Bind(sndSockRef);
    }
    else
    {
        sndSockRef = le_socket_Create(NULL, 0, local, UDP_TYPE);
        if (sndSockRef == NULL)
        {
            LE_ERROR("Failed to create udp socket reference.\n");
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }
    }

    le_socket_SetTimeout(sndSockRef, SOCKET_TIMEOUT_MS);

    if (le_socket_SendTo(sndSockRef, messagePtr, len, desIpPtr, desPort) != LE_OK)
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
        for (uint32_t i = 0; i < MAX_INF_NUM; ++i)
        {
            if (udpEquipSockRef[i] != NULL)
            {
                // [DoIP-125]: Set target IPv4 address to limited broadcast address.
                // The udpDiscoverSockRef bind INADDR_ANY to receive client's limited broadcast.
                // If here use it to send the limited broadcast and the system didn't set default
                // gateway or route, it will return network unreachable.
                if (le_socket_SendTo(udpEquipSockRef[i], messagePtr, len,
                    (char*)TAF_DOIP_UDP_BROADCAST_IP, udpDiscoveryPort) != LE_OK)
                {
                    LE_ERROR("Unable to transmit multicast packet.");
                    return TAF_DOIP_RESULT_NETWORK_ERROR;
                }
            }
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

    // Filter local annoucement message.
    for (int i = 0; i < MAX_INF_NUM; ++i)
    {
        if (strcmp(remoteIp, localIpInfo[i].localIp) == 0)
        {
            return TAF_DOIP_RESULT_OK;
        }
    }

    if (strcmp(remoteIp, "0.0.0.0") == 0)
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
        LE_WARN("DoIP header error!");
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
#if TAF_DOIP_VIN_REQ_WITH_EID_OPTION
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID
#endif
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN
#if TAF_DOIP_ENTITY_STATUS_REQ_OPTION
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST
#endif
        && header.payloadType != TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST)
    {
        // [DoIP-042] NACK code set to 0x01 if the payload type is not supported.
        LE_WARN("Unknow payload type!payload type is [0x%x]\n", header.payloadType);
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

    if (header.payloadLen > (udpDataLen - TAF_DOIP_HEADER_GENERIC_LENGTH))
    {
        // [DoIP-044] NACK code set to 0x03 if the payload length exceeds
        // the currently availbale DoIP protocol handler memory.
        LE_ERROR("DoIP message is out of memory! payload len is %d, available memory is %d\n",
                header.payloadLen, udpDataLen - TAF_DOIP_HEADER_GENERIC_LENGTH);
        nackCode = TAF_DOIP_HEADER_NACK_OUT_OF_MEMORY;
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
    RespondHeaderNegativeACK(ipPtr, port, nackCode);

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

    if (payloadLen != TAF_DOIP_EID_SIZE)
    {
        LE_ERROR("EID is not enough in vehicle identification request.\n");
        // [DoIP-45]
        RespondHeaderNegativeACK(ipPtr, port, TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
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

    if (payloadLen != TAF_DOIP_VIN_SIZE)
    {
        LE_ERROR("VIN is not enough in vehicle identification request.\n");
        // [DoIP-45]
        RespondHeaderNegativeACK(ipPtr, port, TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
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
 FUNCTION        CommunicationMgr::UdpSocketEventCallback
 DESCRIPTION     process the async events on UDP sockets.
 PARAMETERS      [IN] sockRef: Socket context reference.
                 [IN] events: Bitmap of events that occurred.
                 [IN] userPtr: User data pointer
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::UdpSocketEventCallback
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
    const char* ifacePtr = (const char*)userPtr;
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
    LE_DEBUG("Accept a connection(%s:%d) from %s", ip, port, ifacePtr);

    commMgr.connectionMgrPtr->FindOrCreateConnection(childSockRef, ip, port, ifacePtr);

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
            addrInfo.vlanId = dataInfoPtr->vlanId;
            le_utf8_Copy(addrInfo.ifName, dataInfoPtr->ifName,
                TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
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
 FUNCTION        CommunicationMgr::ConfirmUserMessage
 DESCRIPTION     Confirm the user response message in the Uds handle thread.
 PARAMETERS      [IN] dataPtr: uds data
                 [IN] length: uds data length
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::ConfirmUserMessage
(
    void* param1Ptr,
    void* param2Ptr
)
{
    taf_doipSession_t*      sessionPtr;
    taf_doip_AddrInfo_t     addrInfo;
    taf_doipDiagDataInfo_t* dataInfoPtr = (taf_doipDiagDataInfo_t*)param1Ptr;

    auto&   cmMgr = CommunicationMgr::GetInstance();
    sessionPtr = cmMgr.FindDoipSession(dataInfoPtr->sa);
    if (sessionPtr != NULL)
    {
        taf_doipDiagConfirmHandler_t*    handler;
        handler = &sessionPtr->diagConfirmHandler;

        le_mutex_Lock(handler->mutexRef);
        if (handler->funcPtr != NULL)
        {
            addrInfo.sa = dataInfoPtr->sa;
            addrInfo.ta = dataInfoPtr->ta;
            addrInfo.taType = (taf_doip_TaType_t)dataInfoPtr->taType;

            // Confirm the request result.
            handler->funcPtr(&addrInfo, TAF_DOIP_RESULT_OK, handler->ctxPtr);
        }
        le_mutex_Unlock(handler->mutexRef);
    }
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
    taf_doip_Result_t rgistResult,
    const char* ifacePtr
)
{
    taf_doipDiagDataInfo_t* diagInfoPtr;
    uint16_t vid = GetVlanId(ifacePtr);

    diagInfoPtr = (taf_doipDiagDataInfo_t*)le_mem_ForceAlloc(udsInfoPool);

    diagInfoPtr->sa     = sa;
    diagInfoPtr->ta     = ta;
    le_utf8_Copy(diagInfoPtr->ifName, ifacePtr, TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
    diagInfoPtr->vlanId = vid;
    LE_DEBUG("Connection event is coming from %s(with vlan id %d)", ifacePtr, diagInfoPtr->vlanId);

    // Currently, we only support physical addressing;
    diagInfoPtr->taType = TAF_DOIP_TA_TYPE_PHYSICAL;

    le_event_QueueFunctionToThread(udsThrRef,
                                   CommunicationMgr::IndicateConnectionEvent,
                                   diagInfoPtr,
                                   (void*)rgistResult);

    // Report the connection event to upper layer.
    // If upper layer registered event handler, it will receive the event.
    taf_doip_Status_t status;
    taf_doipSession_t*  sessPtr = FindDoipSession(ta);
    if (sessPtr == NULL)
    {
        return;
    }

    status.clientAddr   = sa;
    status.entityAddr   = ta;
    status.eventStatus  = rgistResult;
    le_utf8_Copy(status.ifName, ifacePtr, TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
    status.vlanId = vid;
    le_event_Report(sessPtr->statusEvtId, (void*)&status, sizeof(status));

    return;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::IndicateConnectionEvent
 DESCRIPTION     Indicate the connection event to the Uds handle thread.
 PARAMETERS      [IN] param1Ptr: Address information.
                 [IN] param2Ptr: Connection registered state.
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::IndicateConnectionEvent
(
    void* param1Ptr,
    void* param2Ptr
)
{
    taf_doipSession_t*      sessionPtr;
    taf_doip_AddrInfo_t     addrInfo;
    taf_doip_DiagIndicationHandlerFunc_t handlerFunc;
    taf_doipDiagDataInfo_t* dataInfoPtr = (taf_doipDiagDataInfo_t*)param1Ptr;
    size_t rgistResult = (size_t)param2Ptr;

    auto&   cmMgr = CommunicationMgr::GetInstance();
    sessionPtr = cmMgr.FindDoipSession(dataInfoPtr->ta);
    if (sessionPtr != NULL)
    {
        taf_doipIndicationHandler_t*    handler;
        handler = &sessionPtr->diagIndicationHandler;

        le_mutex_Lock(handler->mutexRef);
        handlerFunc = handler->funcPtr;
        le_mutex_Unlock(handler->mutexRef);

        if (handlerFunc != NULL)
        {
            addrInfo.sa = dataInfoPtr->sa;
            addrInfo.ta = dataInfoPtr->ta;
            addrInfo.taType = (taf_doip_TaType_t)dataInfoPtr->taType;
            addrInfo.vlanId = dataInfoPtr->vlanId;
            le_utf8_Copy(addrInfo.ifName, dataInfoPtr->ifName,
                TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
            // Indicate reception message to upper layer.
            handlerFunc(&addrInfo, NULL, (taf_doip_Result_t)rgistResult, handler->ctxPtr);
        }
    }
    le_mem_Release(dataInfoPtr);

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
    if (connection == nullptr)
    {
        le_mem_Release(dataInfoPtr->data);
        le_mem_Release(dataInfoPtr);

        return;
    }

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
 FUNCTION        CommunicationMgr::RespondHeaderNegativeACK
 DESCRIPTION     DoIP header negative ack response
 PARAMETERS      [IN] ipPtr: Destination IP Pointer
                 [IN] port: Destination port
                 [IN] nackCode: negative ack code
 RETURN VALUE    void
=================================================================================================*/
void CommunicationMgr::RespondHeaderNegativeACK
(
    const char* ipPtr,
    uint16_t    port,
    taf_doipHeaderNACKCode_t nackCode
)
{
    auto& parser = ProtocolParser::GetInstance();
    taf_doipLink_t link;

    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_UNI;
    link.sockRef = udpDiscoverSockRef;
    le_utf8_Copy(link.ip, ipPtr, strlen(ipPtr) + 1, NULL);
    link.port = port;
    parser.HeaderNegativeACK(&link, nackCode);
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::GetVlanId
 DESCRIPTION     Get VLAN ID from interface
 PARAMETERS      [IN] iface: Destination IP Pointer
                 [IN] port: Destination port
                 [IN] nackCode: negative ack code
 RETURN VALUE    void
=================================================================================================*/
uint16_t CommunicationMgr::GetVlanId(const char *ifacePtr)
{
    FILE *fp;
    char buf[256];
    char *nextPtr = NULL;
    int32_t vlanId = 0;

    // open /proc/net/vlan/config to get vlan information
    fp = fopen(VLAN_PROC_PATH, "r");
    if (fp == NULL)
    {
        LE_ERROR("Failed to open %s", VLAN_PROC_PATH);
        return 0;
    }

    // Find the vlan id with the interface name.
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        char *tmpPtr = strtok_r(buf, " ", &nextPtr);
        if ((tmpPtr != NULL) && (strcmp(ifacePtr, tmpPtr) == 0))
        {
            sscanf(nextPtr, "%*s %d", &vlanId);
            break;
        }
    }

    fclose(fp);
    LE_DEBUG("Get vlan id(0x%x) for %s", vlanId, ifacePtr);

    return (uint16_t)vlanId;
}

taf_doip_Result_t CommunicationMgr::GetLocalIPv4FromSource
(
    struct sockaddr_in *srcAddrPtr,
    char *local
)
{
    if (srcAddrPtr == NULL || local == NULL)
    {
        LE_ERROR("Invalid parameters: srcAddrPtr or local is NULL");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    for (int i = 0; i < MAX_INF_NUM; ++i)
    {
        if ((srcAddrPtr->sin_addr.s_addr & localIpInfo[i].ipv4_mask_s_addr) ==
            (localIpInfo[i].ipv4_s_addr & localIpInfo[i].ipv4_mask_s_addr))
        {
            le_utf8_Copy(local, localIpInfo[i].localIp, TAF_DOIP_IP_ADDR_MAX_LEN, NULL);
            LE_DEBUG("local ip=%s", local);
            return TAF_DOIP_RESULT_OK;
        }
    }

    return TAF_DOIP_RESULT_ERROR;
}

taf_doip_Result_t CommunicationMgr::GetLocalIPv6FromSource
(
    struct sockaddr_in6 *srcAddrPtr,
    char *local
)
{
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *addr = NULL;
    taf_doip_Result_t result = TAF_DOIP_RESULT_ERROR;

    int ret = getifaddrs(&interfaces);
    if (ret != 0)
    {
        LE_ERROR("Failed to get interface addresses");
        return TAF_DOIP_RESULT_ERROR;
    }

    for (addr = interfaces; addr != NULL; addr = addr->ifa_next)
    {
        if (addr->ifa_addr == NULL || addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        struct sockaddr_in6 *ipAddr = (struct sockaddr_in6 *)addr->ifa_addr;
        struct sockaddr_in6 *netMask = (struct sockaddr_in6 *)addr->ifa_netmask;

        if ((addr->ifa_flags & IFF_UP) && (addr->ifa_flags & IFF_RUNNING) && netMask != NULL)
        {
            // Check the address.
            if ((srcAddrPtr->sin6_addr.s6_addr[0] & netMask->sin6_addr.s6_addr[0]) ==
                (ipAddr->sin6_addr.s6_addr[0] & netMask->sin6_addr.s6_addr[0])
                && (srcAddrPtr->sin6_addr.s6_addr[1] & netMask->sin6_addr.s6_addr[1]) ==
                (ipAddr->sin6_addr.s6_addr[1] & netMask->sin6_addr.s6_addr[1])
                && (srcAddrPtr->sin6_addr.s6_addr[2] & netMask->sin6_addr.s6_addr[2]) ==
                (ipAddr->sin6_addr.s6_addr[2] & netMask->sin6_addr.s6_addr[2])
                && (srcAddrPtr->sin6_addr.s6_addr[3] & netMask->sin6_addr.s6_addr[3]) ==
                (ipAddr->sin6_addr.s6_addr[3] & netMask->sin6_addr.s6_addr[3])
                && (srcAddrPtr->sin6_addr.s6_addr[4] & netMask->sin6_addr.s6_addr[4]) ==
                (ipAddr->sin6_addr.s6_addr[4] & netMask->sin6_addr.s6_addr[4])
                && (srcAddrPtr->sin6_addr.s6_addr[5] & netMask->sin6_addr.s6_addr[5]) ==
                (ipAddr->sin6_addr.s6_addr[5] & netMask->sin6_addr.s6_addr[5])
                && (srcAddrPtr->sin6_addr.s6_addr[6] & netMask->sin6_addr.s6_addr[6]) ==
                (ipAddr->sin6_addr.s6_addr[6] & netMask->sin6_addr.s6_addr[6])
                && (srcAddrPtr->sin6_addr.s6_addr[7] & netMask->sin6_addr.s6_addr[7]) ==
                (ipAddr->sin6_addr.s6_addr[7] & netMask->sin6_addr.s6_addr[7]))
            {
                LE_DEBUG("Interface : %s\n", addr->ifa_name);
                inet_ntop(AF_INET6, &(ipAddr->sin6_addr), local, TAF_DOIP_IPV6_ADDR_MAX_LEN);
                result = TAF_DOIP_RESULT_OK;
                goto out;
            }
        }
    }

out:
    freeifaddrs(interfaces);
    return result;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CreateSpecIPv4Socket
 DESCRIPTION     Create IPv4 sockets for DoIP communication.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of specific IPv4 socket creation.
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::CreateSpecIPv4Socket
(
    uint32_t index,
    const char* ifNamePtr,
    uint16_t udpDiscoveryPort,
    uint16_t tcpDataPort
)
{
    taf_doip_Result_t result = TAF_DOIP_RESULT_OK;

    std::string ifNameStr;
    ifNameStr.assign(ifNamePtr);
    if (GetLocalIPv4Addr(ifNameStr, &localIpInfo[index], TAF_DOIP_IPV4_ADDR_MAX_LEN)
        != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Can not get local IPv4 address.\n");
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    tcpDataSockRef[index] = le_socket_Create(NULL, tcpDataPort, localIpInfo[index].localIp,
            TCP_TYPE);
    if (tcpDataSockRef[index] == NULL)
    {
        LE_ERROR("Failed to create tcp socket reference.\n");
        result = TAF_DOIP_RESULT_NO_SOCKET;
        goto errOut;
    }

    udpEquipSockRef[index] = le_socket_Create(NULL, udpDiscoveryPort, localIpInfo[index].localIp,
            UDP_TYPE);
    if (udpEquipSockRef[index] == NULL)
    {
        LE_ERROR("Failed to create udp socket reference.\n");
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    if (le_socket_Bind(udpEquipSockRef[index]) != LE_OK)
    {
        LE_ERROR("Failed to bind with port%d", udpDiscoveryPort);
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    le_socket_SetTimeout(udpEquipSockRef[index], SOCKET_TIMEOUT_MS);
    le_socket_AddEventHandler(udpEquipSockRef[index], UdpSocketEventCallback, NULL);

    LE_DEBUG("tcp socket ref is %p", tcpDataSockRef[index]);
    le_socket_SetTimeout(tcpDataSockRef[index], SOCKET_TIMEOUT_MS);

    // Set the socket event callback for tcp listener fd monitor.
    le_socket_AddEventHandler(tcpDataSockRef[index], TcpDataSocketEventCallback, (void*)ifNamePtr);
    if (le_socket_Bind(tcpDataSockRef[index]) != LE_OK)
    {
        LE_ERROR("le_socket_Bind error!\n");
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    return result;

errOut:
    if (tcpDataSockRef[index] != NULL)
    {
        le_socket_Delete(tcpDataSockRef[index]);
        tcpDataSockRef[index] = NULL;
    }

    if (udpEquipSockRef[index] != NULL)
    {
        le_socket_Delete(udpEquipSockRef[index]);
        udpEquipSockRef[index] = NULL;
    }

    return result;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CreateSpecIPv6Socket
 DESCRIPTION     Create IPv6 sockets for DoIP communication.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of specific IPv6 socket creation.
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::CreateSpecIPv6Socket
(
    uint32_t index,
    const char* ifNamePtr,
    uint16_t udpDiscoveryPort,
    uint16_t tcpDataPort
)
{
    taf_doip_Result_t result = TAF_DOIP_RESULT_OK;

    std::string ifNameStr;
    ifNameStr.assign(ifNamePtr);
    if (GetLocalIPv6Addr(ifNameStr, &localIpInfo[index], TAF_DOIP_IPV6_ADDR_MAX_LEN)
        != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Can not get local IPv6 address.\n");
        goto errOut;
    }

    LE_DEBUG("Get IPv6-%s\n", localIpInfo[index].localIp);

    tcpDataSockRef[index] = le_socket_Create(NULL, tcpDataPort, localIpInfo[index].localIp,
            TCP_TYPE);
    if (tcpDataSockRef[index] == NULL)
    {
        LE_ERROR("Failed to create tcp socket reference.\n");
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    // Set the socket event callback for tcp listener fd monitor.
    le_socket_AddEventHandler(tcpDataSockRef[index], TcpDataSocketEventCallback, (void*)ifNamePtr);
    if (le_socket_Bind(tcpDataSockRef[index]) != LE_OK)
    {
        LE_ERROR("le_socket_Bind error!\n");
        result = TAF_DOIP_RESULT_NETWORK_ERROR;
        goto errOut;
    }

    return result;
errOut:
    if (tcpDataSockRef[index] != NULL)
    {
        le_socket_Delete(tcpDataSockRef[index]);
        tcpDataSockRef[index] = NULL;
    }

    return result;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CreateIPv4Socket
 DESCRIPTION     Create IPv4 socket resource for DoIP communication.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of IPv4 socket resource creation.
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::CreateIPv4SocketRes
(
)
{
    uint16_t    udpDiscoveryPort;
    uint16_t    tcpDataPort;
    taf_doip_Result_t result;

    auto& vehicleMgr = VehicleManager::GetInstance();

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetTcpPort(&tcpDataPort))
    {
        tcpDataPort = TAF_DOIP_TCP_DATA_DEFAULT;
    }

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetUdpPort(&udpDiscoveryPort))
    {
        udpDiscoveryPort = TAF_DOIP_UDP_DISCOVERY_DEFAULT;
    }

    LE_DEBUG("TCP_DATA is %d, UDP_DISCOVEERY is %d",
            tcpDataPort, udpDiscoveryPort);

    le_dls_List_t *ifaceListPtr = vehicleMgr.GetIfaceList();
    if (ifaceListPtr == NULL)
    {
        LE_ERROR("Interface is empty.");
        return TAF_DOIP_RESULT_NO_LINK;
    }

    if (le_dls_IsEmpty(ifaceListPtr))
    {
        result = CreateSpecIPv4Socket(0, TAF_DOIP_INTERFACE_DEFAULT,
                udpDiscoveryPort, tcpDataPort);
        if (result != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to create specific socket reference.");
            return result;
        }
    }
    else
    {
        uint32_t cnt = 0;
        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Peek(ifaceListPtr);
        while (linkPtr)
        {
            taf_doip_Iface_t *ifacePtr = CONTAINER_OF(linkPtr, taf_doip_Iface_t, link);
            result = CreateSpecIPv4Socket(cnt, ifacePtr->ifName, udpDiscoveryPort, tcpDataPort);
            if (result != TAF_DOIP_RESULT_OK)
            {
                LE_ERROR("Failed to create specific socket reference.");
                return result;
            }

            linkPtr = le_dls_PeekNext(ifaceListPtr, linkPtr);
            cnt++;
        }
    }

    udpDiscoverSockRef = le_socket_Create(NULL, udpDiscoveryPort,
        (char*)TAF_DOIP_IPv4_ADDRESS_ANY, UDP_TYPE);
    if (udpDiscoverSockRef == NULL)
    {
        LE_ERROR("Failed to create udp socket reference.\n");
        return TAF_DOIP_RESULT_NO_SOCKET;
    }

    if (le_socket_Bind(udpDiscoverSockRef) != LE_OK)
    {
        LE_ERROR("Failed to bind with port%d", udpDiscoveryPort);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    le_socket_SetTimeout(udpDiscoverSockRef, SOCKET_TIMEOUT_MS);

    // Set the socket event callback for discovery fd monitor.
    le_socket_AddEventHandler(udpDiscoverSockRef, UdpSocketEventCallback, NULL);
    LE_DEBUG("Discovery socket ref is %p", udpDiscoverSockRef);

    return TAF_DOIP_RESULT_OK;
}

/*=================================================================================================
 FUNCTION        CommunicationMgr::CreateIPv6SocketRes
 DESCRIPTION     Create IPv4 socket resource for DoIP communication.
 PARAMETERS      void
 RETURN VALUE    taf_doip_Result_t: Result of IPv6 socket resource creation.
=================================================================================================*/
taf_doip_Result_t CommunicationMgr::CreateIPv6SocketRes
(
)
{
    uint16_t    udpDiscoveryPort;
    uint16_t    tcpDataPort;
    taf_doip_Result_t result;

    auto& vehicleMgr = VehicleManager::GetInstance();

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetTcpPort(&tcpDataPort))
    {
        tcpDataPort = TAF_DOIP_TCP_DATA_DEFAULT;
    }

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetUdpPort(&udpDiscoveryPort))
    {
        udpDiscoveryPort = TAF_DOIP_UDP_DISCOVERY_DEFAULT;
    }

    LE_DEBUG("TCP_DATA is %d, UDP_DISCOVEERY is %d",
            tcpDataPort, udpDiscoveryPort);

    le_dls_List_t *ifaceListPtr = vehicleMgr.GetIfaceList();
    if (ifaceListPtr == NULL)
    {
        LE_ERROR("Interface is empty.");
        return TAF_DOIP_RESULT_NO_LINK;
    }

    if (le_dls_IsEmpty(ifaceListPtr))
    {
        result = CreateSpecIPv6Socket(0, TAF_DOIP_INTERFACE_DEFAULT,
            udpDiscoveryPort, tcpDataPort);
        if (result != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to create specific socket reference.");
            return result;
        }
    }
    else
    {
        uint32_t cnt = 0;
        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Peek(ifaceListPtr);
        while (linkPtr)
        {
            taf_doip_Iface_t *ifacePtr = CONTAINER_OF(linkPtr, taf_doip_Iface_t, link);
            result = CreateSpecIPv6Socket(cnt, ifacePtr->ifName, udpDiscoveryPort, tcpDataPort);
            if (result != TAF_DOIP_RESULT_OK)
            {
                LE_ERROR("Failed to create specific socket reference.");
                return result;
            }

            linkPtr = le_dls_PeekNext(ifaceListPtr, linkPtr);
            cnt++;
        }
    }

    udpDiscoverSockRef = le_socket_Create(NULL, udpDiscoveryPort,
        (char*)TAF_DOIP_UDP6_BROADCAST_IP, UDP_TYPE);
    if (udpDiscoverSockRef == NULL)
    {
        LE_ERROR("Failed to create udp socket reference.\n");
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    if (le_socket_Bind(udpDiscoverSockRef) != LE_OK)
    {
        LE_ERROR("Failed to bind with port%d", udpDiscoveryPort);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    if (le_socket_JoinMulticastGroup(udpDiscoverSockRef,
        (const char*)TAF_DOIP_UDP6_BROADCAST_IP) != LE_OK)
    {
        LE_ERROR("Failed to join multicast group for IPv6.");
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    // Set the socket event callback for discovery fd monitor.
    le_socket_AddEventHandler(udpDiscoverSockRef, UdpSocketEventCallback, NULL);
    LE_DEBUG("Discovery socket ref is %p", udpDiscoverSockRef);

    return TAF_DOIP_RESULT_OK;
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
    taf_doip_Result_t result = TAF_DOIP_RESULT_ERROR;

    if (TAF_DOIP_RESULT_OK != vehicleMgr.GetNetType(netType))
    {
        le_utf8_Copy(netType, TAF_DOIP_IPTYPE_DEFAULT,
                     TAF_DOIP_IPTYPE_MAX_LEN, NULL);
    }

    LE_DEBUG("net type is %s\n", netType);

    if (!strncasecmp(netType, "IPv4", 4))
    {
        result = CreateIPv4SocketRes();
        if (result != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to create IPv4 socket resource.");
            return result;
        }
    }
    else
    {
        result = CreateIPv6SocketRes();
        if (result != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to create IPv4 socket resource.");
            return result;
        }
    }

    // Initialize TCP connection manager.
    connectionMgrPtr->Init();

    // Initialzie the reception and sending thread.
    mainThrRef = le_thread_GetCurrent();

    state = TAF_DOIP_STATE_INIT;

    return TAF_DOIP_RESULT_OK;
}

void CommunicationMgr::shutdownTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("shutdownTimerHandler");
    auto&   cmMgr = CommunicationMgr::GetInstance();
    cmMgr.connectionMgrPtr->DeleteAllConnection();
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

    auto&   cmMgr = CommunicationMgr::GetInstance();

    LE_INFO("ShutdownAllConnection");
    cmMgr.connectionMgrPtr->ShutdownAllConnection();

    le_timer_SetHandler(shutdownTimerRef, shutdownTimerHandler);
    le_timer_SetWakeup(shutdownTimerRef, false);
    le_timer_SetMsInterval(shutdownTimerRef, TAF_DOIP_CLOSE_SOCKET_INTERVAL);
    le_timer_Start(shutdownTimerRef);

    le_socket_Delete(udpDiscoverSockRef);
    udpDiscoverSockRef = NULL;

    for (int i = 0; i < MAX_INF_NUM; ++i)
    {
        le_socket_Delete(tcpDataSockRef[i]);
        tcpDataSockRef[i] = NULL;

        if (udpEquipSockRef[i] != NULL)
        {
            le_socket_Delete(udpEquipSockRef[i]);
            udpEquipSockRef[i] = NULL;
        }
    }

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
    LE_DEBUG("DoIP communication manager init...\n");

    udsThrRef = le_thread_Create("UdsThread", UdsHandleThread, NULL);
    le_thread_SetStackSize(udsThrRef, TAF_DOIP_THREAD_STACK_SIZE);
    le_thread_Start(udsThrRef);

    udpMsgPool = le_mem_CreatePool("DoipUdpMsgPool", TAF_DOIP_MAX_BUFFER_SIZE);
    udsInfoPool = le_mem_CreatePool("DoipUdsInfoPool", sizeof(taf_doipDiagDataInfo_t));

    doipSessionPool = le_mem_CreatePool("DoipSessionPool", sizeof(taf_doipSession_t));
    doipSessionRefMap = le_ref_CreateMap("DoipSessionRefMap", TAF_DOIP_MAX_ENTITY_NUM);
    doipHandlerRefMap = le_ref_CreateMap("DoipHandlerRefMap", TAF_DOIP_MAX_USER_HANDLER_NUM);
    doipSessionRefMutex = le_mutex_CreateRecursive("DoipSessionRefMutex");

    shutdownTimerRef = le_timer_Create("shutdownTimer");

    // Initialized vehicle deiscovery.
    auto& vehicleDisovery = VehicleDiscovery::GetInstance();
    vehicleDisovery.Init();

    LE_DEBUG("DoIP communication manager ok...\n");

    return;
}