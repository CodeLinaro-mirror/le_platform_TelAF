/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
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

/*
 * @file       tafNet.cpp
 * @brief      This file provides the telematics application framework network component APIs.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafNetworkImpl.hpp"
#include "tafNatImpl.hpp"
#include "tafVlanImpl.hpp"
#include "tafL2tpImpl.hpp"
#include "tafSocksImpl.hpp"
#include "tafGsbImpl.hpp"
#include "taf_pa_net.hpp"

using namespace telux::tafsvc;

void taf_net_init()
{
    LE_INFO("taf net component init start...\n");
    auto &network = taf_Net::GetInstance();
    network.Init();
    LE_INFO("taf net component init done...\n");

    return;
}

void taf_nat_init()
{
    LE_INFO("taf nat component init start...\n");
    auto &nat = taf_Nat::GetInstance();
    nat.Init();
    LE_INFO("taf nat component init done...\n");

    return;
}

void taf_vlan_init()
{
    LE_INFO("taf vlan component init start...\n");
    auto &vlan = taf_Vlan::GetInstance();
    vlan.Init();
    LE_INFO("taf vlan component init done...\n");

    return;
}

void taf_l2tp_init()
{
    LE_INFO("taf l2tp component init start...\n");
    auto &l2tp = taf_L2tp::GetInstance();
    l2tp.Init();
    LE_INFO("taf l2tp component init done...\n");

    return;
}

void taf_socks_init()
{
    LE_INFO("taf socks component init start...\n");
    auto &socks = taf_Socks::GetInstance();
    socks.Init();
    LE_INFO("taf socks component init done...\n");

    return;
}

void taf_gsb_init()
{
    LE_INFO("taf gsb component init start...\n");
    auto &gsb = taf_Gsb::GetInstance();
    gsb.Init();
    LE_INFO("taf gsb component init done...\n");

    return;
}

/**
 * Get network interface List
 *
 * @param [out] intfInfoList               The network interface information list
 * @param [out] listSize                   The size of the network interface list
 *
 * @returns LE_OK                          Success
 *          LE_FAULT                       Failed to get the network interface information list.
 */
le_result_t taf_net_GetInterfaceList(taf_net_InterfaceInfo_t *intfInfoList, size_t *listSize)
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t profListSize = 0, intfIdx= 0;
    le_result_t result;
    char interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN];
    taf_dcs_ProfileRef_t profileRef = NULL;

    TAF_ERROR_IF_RET_VAL(intfInfoList == NULL, LE_BAD_PARAMETER, "intfInfoList is NULL!");
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_BAD_PARAMETER, "listSize is NULL!");

    for(int phoneId = 1; phoneId <= MAX_SLOT_COUNT; phoneId++)
    {
        result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &profListSize);
        if(result != LE_OK)
        {
            LE_DEBUG("Getting profile list for phone id %d failed", phoneId);
            continue;
        }

        for (size_t i = 0; i < profListSize; i++)
        {
            const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
            profileRef=taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);

            if(profileRef == NULL)
                continue;

            result=taf_dcs_GetInterfaceName(profileRef,
                                            interfaceName,
                                            TAF_NET_INTERFACE_NAME_MAX_LEN);

            if(result != LE_OK)
                continue;

            if(intfIdx >= TAF_NET_INTERFACE_NAME_MAX_NUM)
            {
                LE_ERROR("The interface number exceeds the max number");
                break;
            }

            le_utf8_Copy(intfInfoList[intfIdx].interfaceName, interfaceName,
                        TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
            intfInfoList[intfIdx].tech = TAF_NET_TECH_CELLULAR;
            intfInfoList[intfIdx].state = TAF_NET_STATE_UP;
            intfIdx++;
        }
    }

    *listSize=intfIdx;

    return LE_OK ;
}

/**
 * Add a handler function for route change.
 *
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add route change handler.
 *          NULL                        Failed to add route change handler.
 */

taf_net_RouteChangeHandlerRef_t taf_net_AddRouteChangeHandler
(
    taf_net_RouteChangeHandlerFunc_t handlerFuncPtr,
    void *contextPtr
)
{
    auto &network = taf_Net::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("RouteChangeHandler",
        network.RouteChangeEvId, taf_Net::FirstLayerRouteChangeHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_net_RouteChangeHandlerRef_t)(handlerRef);
}

/**
 * Remove route change handler.
 *
 * @param [in] handlerRef               The state handler reference returned by
 *                                      taf_net_AddRouteChangeHandler().
 *
 * @returns NA
 *
 */
void taf_net_RemoveRouteChangeHandler(taf_net_RouteChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * Add a handler function for gateway change.
 *
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add gateway change handler.
 *          NULL                        Failed to add gateway change handler.
 */

taf_net_GatewayChangeHandlerRef_t taf_net_AddGatewayChangeHandler
(
    taf_net_GatewayChangeHandlerFunc_t handlerFuncPtr,
    void *contextPtr
)
{
    auto &network = taf_Net::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("GatewayChangeHandler",
        network.GatewayChangeEvId, taf_Net::FirstLayerGatewayChangeHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_net_GatewayChangeHandlerRef_t)(handlerRef);
}

/**
 * Remove gateway change handler.
 *
 * @param [in] handlerRef               The state handler reference returned by
 *                                      taf_net_AddGatewayChangeHandler().
 *
 * @returns NA
 *
 */
void taf_net_RemoveGatewayChangeHandler(taf_net_GatewayChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * Add a handler function for DNS change.
 *
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add DNS change handler.
 *          NULL                        Failed to add DNS change handler.
 */

taf_net_DNSChangeHandlerRef_t taf_net_AddDNSChangeHandler
(
    taf_net_DNSChangeHandlerFunc_t handlerFuncPtr,
    void *contextPtr
)
{
    auto &network = taf_Net::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DNSChangeHandler",
        network.DNSChangeEvId, taf_Net::FirstLayerDNSChangeHandler,
        (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_net_DNSChangeHandlerRef_t)(handlerRef);
}

/**
 * Remove DNS change handler.
 *
 * @param [in] handlerRef               The state handler reference returned by
 *                                      taf_net_AddDNSChangeHandler().
 *
 * @returns NA
 *
 */
void taf_net_RemoveDNSChangeHandler(taf_net_DNSChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * add or delete a route
 *
 * @param [in] namePtr                     The network interface to be used to change route
 * @param [in] destAddrPtr                 The destination address to be used to change route
 * @param [in] preLenPtr                   The destination prefix length to be used to change route
 * @param [in] metric                      The metric to be used to change route
 * @param [in] isAdd                       Add or delete a route
 *
 * @returns LE_OK                          Success
 *          LE_BAD_PARAMETER               Invalid parameter.
 *          LE_FAULT                       Failed to add or remove a route.
 */

le_result_t taf_net_ChangeRoute(const char *namePtr, const char *destAddrPtr, const char *preLenPtr,
                                uint16_t metric, taf_net_NetAction_t isAdd)
{

    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_BAD_PARAMETER, "destination address is NULL!");
    TAF_ERROR_IF_RET_VAL(preLenPtr == NULL, LE_BAD_PARAMETER, "prefixLength is NULL!");

    return network.ChangeRoute(namePtr, destAddrPtr, preLenPtr, metric, isAdd);
}

/**
 * Backup the current default gateway of the system into db, including both IPv4 and IPv6
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                No default gateway found to be backed up.
 */
le_result_t taf_net_BackupDefaultGW()
{

    auto &network = taf_Net::GetInstance();

    return network.BackupDefaultGW(taf_net_GetClientSessionRef());
}

/**
 * Restore the default gateway of the system from backup db, including IPv4 and/or IPv6
 *
 * Only the gateway was backed up and set by the same session,it can be restored.
 *
 * @returns LE_OK                       Restore IPV4 and/or IPV6 gateway successfully.
 *          LE_NOT_FOUND                No backup.
 *          LE_FAULT                    Failed to Restore default gateway.
 */
le_result_t taf_net_RestoreDefaultGW()
{

    auto &network = taf_Net::GetInstance();

    return network.RestoreDefaultGW(taf_net_GetClientSessionRef());
}

/**
 * Set a system's default gateway addresses to those assigned for a given interface.
 *
 * @param [in] namePtr                  The interface name onto which to get the gateway addresses
 *
 * @returns LE_OK                       Set IPV4 and/or IPV6 gateway successfully.
 *          LE_NOT_FOUND                No gateway address from the interface.
 *          LE_BAD_PARAMETER            Invalid interface name.
 *          LE_FAULT                    Failed to set default gateway.
 */
le_result_t taf_net_SetDefaultGW(const char *namePtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    char ipv4GwAddr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6GwAddr[NET_IPV6_ADDR_MAX_BYTES];
    uint32_t profileId=0;
    uint8_t phoneId=0;
    le_result_t ipv4Ret, ipv6Ret, result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");

    auto &network = taf_Net::GetInstance();

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find profile id by name!");

    result = taf_dcs_GetPhoneIdByInterfaceName(namePtr, &phoneId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find phone id by name!");

    //get cellular default gateway address
    profileRef= taf_dcs_GetProfileEx(phoneId, profileId );

    if(profileRef == NULL)
    {
        LE_ERROR("interface : %s , can't get profile reference from cellular", namePtr);
        return LE_NOT_FOUND;
    }

    ipv6Ret = taf_dcs_GetIPv6GatewayAddress(profileRef, ipv6GwAddr, NET_IPV6_ADDR_MAX_BYTES);

    ipv4Ret = taf_dcs_GetIPv4GatewayAddress(profileRef, ipv4GwAddr, NET_IPV4_ADDR_MAX_BYTES);

    if( ipv4Ret != LE_OK && ipv6Ret != LE_OK)
    {
        LE_ERROR("interface : %s , can't get ipv4 or ipv6 gateway address", namePtr);
        return LE_NOT_FOUND;
    }

    return network.SetDefaultGW(taf_net_GetClientSessionRef(), namePtr, ipv4GwAddr, ipv6GwAddr);
}

/**
 * Get the assigned default gateway addresses from the specified interface
 *
 * @param [in]  namePtr                 The interface name onto which to get the gateway addresses
 * @param [out] ipv4AddrPtr             The ipv4 gateway address
 * @param [in] ipv4AddrSize             The size of ip v4 gateway address
 * @param [out] ipv6AddrPtr             The ipv6 gateway address
 * @param [in] ipv6AddrSize             The size of ip v6 gateway address
 *
 * @returns LE_OK           Get IPV4 and/or IPV6 gateway addresses from the interface successfully.
 *          LE_NOT_FOUND                No gateway address from the interface.
 *          LE_BAD_PARAMETER            Invalid interface name.
 *          LE_FAULT                    Failed to get gateway addresses from the interface.
 */
le_result_t taf_net_GetInterfaceGW(const char *namePtr, char *ipv4AddrPtr, size_t ipv4AddrSize,
                                   char *ipv6AddrPtr, size_t ipv6AddrSize)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    uint8_t phoneId=0;
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(ipv4AddrPtr == NULL, LE_BAD_PARAMETER, "ipv4AddrPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(ipv6AddrPtr == NULL, LE_BAD_PARAMETER, "ipv6AddrPtr is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find profile id by name!");

    result = taf_dcs_GetPhoneIdByInterfaceName(namePtr, &phoneId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find phone id by name!");

    //get cellular gateway address
    profileRef= taf_dcs_GetProfileEx(phoneId, profileId );

    if(profileRef == NULL)
    {
        LE_ERROR("interface : %s , can't get profile reference from cellular", namePtr);
        return LE_NOT_FOUND;
    }

    ipv6Ret = taf_dcs_GetIPv6GatewayAddress(profileRef, ipv6AddrPtr, NET_IPV6_ADDR_MAX_BYTES);

    ipv4Ret = taf_dcs_GetIPv4GatewayAddress(profileRef, ipv4AddrPtr, NET_IPV4_ADDR_MAX_BYTES);

    if( ipv4Ret != LE_OK && ipv6Ret != LE_OK)
    {
        LE_ERROR("interface : %s , can't get ipv4 or ipv6 gateway address", namePtr);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

/**
 * Set a system's DNS addresses to those assigned for a given interface,and back up the addresses.
 *
 * @param [in] namePtr                The interface Name onto which to get the DNS addresses
 *
 * @returns LE_OK                     Set IPV4 and/or IPV6 DNS into system successfully.
 *          LE_NOT_FOUND              No DNS addresses from the interface.
 *          LE_BAD_PARAMETER          Invalid interface name.
 *          LE_FAULT                  Failed to set DNS addresses.
 */
le_result_t taf_net_SetDNS(const char *namePtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    uint8_t phoneId=0;
    char ipv4DnsAddrs[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV4_ADDR_MAX_BYTES] = {{0}, {0}};
    char ipv6DnsAddrs[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV6_ADDR_MAX_BYTES] = {{0}, {0}};
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find profile id by name!");

    result = taf_dcs_GetPhoneIdByInterfaceName(namePtr, &phoneId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find phone id by name!");

    profileRef= taf_dcs_GetProfileEx(phoneId, profileId );

    if(profileRef == NULL)
    {
        LE_ERROR("interface : %s , can't get profile reference from cellular", namePtr);
        return LE_NOT_FOUND;
    }
    //get cellular dns address
    ipv6Ret = taf_dcs_GetIPv6DNSAddresses(profileRef, ipv6DnsAddrs[0], NET_IPV6_ADDR_MAX_BYTES,
                                                      ipv6DnsAddrs[1], NET_IPV6_ADDR_MAX_BYTES);

    ipv4Ret = taf_dcs_GetIPv4DNSAddresses(profileRef, ipv4DnsAddrs[0], NET_IPV4_ADDR_MAX_BYTES,
                                                      ipv4DnsAddrs[1], NET_IPV4_ADDR_MAX_BYTES);

    if(ipv6Ret != LE_OK && ipv4Ret != LE_OK)
    {
        LE_ERROR("interface : %s , can't get ipv4 or ipv6 DNS address", namePtr);
        return LE_NOT_FOUND;
    }

    return network.SetDNS(taf_net_GetClientSessionRef(), ipv4DnsAddrs[0], ipv4DnsAddrs[1],
                                                         ipv6DnsAddrs[0], ipv6DnsAddrs[1]);
}

/**
 * Get the assigned DNS addresses from the interface
 *
 * @param [in]  namePtr                 The interface name onto which to get the DNS addresses
 * @param [out] dnsSvrAddrsPtr          DNS addresses structure
 *
 * @returns LE_OK                       Get IPV4 and/or IPV6 DNS addresses successfully
 *                                      from the interfaces.
 *          LE_NOT_FOUND                No DNS addresses from the interface.
 *          LE_BAD_PARAMETER            Invalid interface name.
 *          LE_FAULT                    Failed to get DNS addresses.
 */
le_result_t taf_net_GetInterfaceDNS(const char *namePtr,
                                    taf_net_DnsServerAddresses_t *dnsSvrAddrsPtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    uint8_t phoneId=0;
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(dnsSvrAddrsPtr == NULL, LE_BAD_PARAMETER, "dnsSvrAddrsPtr is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find profile id by name!");

    result = taf_dcs_GetPhoneIdByInterfaceName(namePtr, &phoneId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Can't find phone id by name!");

    //get cellular DNS address
    profileRef= taf_dcs_GetProfileEx(phoneId, profileId);

    if(profileRef == NULL)
    {
        LE_ERROR("interface : %s , can't get profile reference from cellular", namePtr);
        return LE_NOT_FOUND;
    }

    ipv6Ret = taf_dcs_GetIPv6DNSAddresses(profileRef, dnsSvrAddrsPtr->ipv6Addr1,
                                          sizeof(dnsSvrAddrsPtr->ipv6Addr1),
                                          dnsSvrAddrsPtr->ipv6Addr2,
                                          sizeof(dnsSvrAddrsPtr->ipv6Addr2));

    ipv4Ret = taf_dcs_GetIPv4DNSAddresses(profileRef, dnsSvrAddrsPtr->ipv4Addr1,
                                          sizeof(dnsSvrAddrsPtr->ipv4Addr1),
                                          dnsSvrAddrsPtr->ipv4Addr2,
                                          sizeof(dnsSvrAddrsPtr->ipv4Addr2));

    if(ipv6Ret != LE_OK && ipv4Ret != LE_OK)
    {
        LE_ERROR("interface : %s , can't get ipv4 or ipv6 DNS address", namePtr);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

/*=============================Destination NAT=========================================*/
/**
 * Add a handler function for destination NAT change.
 *
 * @param [in] handlerPtr             The handler function.
 * @param [in] contextPtr             The handler context.
 *
 * @returns reference                 Success to add destination NAT change handler.
 *          NULL                      Failed to add destination NAT change handler.
 */
taf_net_DestNatChangeHandlerRef_t taf_net_AddDestNatChangeHandler
(
    taf_net_DestNatChangeHandlerFunc_t handlerFuncPtr,
    void *contextPtr
)
{
    auto &tafNat = taf_Nat::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DestNatChangeHandler",
        tafNat.DestNatChangeEvId, taf_Nat::FirstLayerDestNatChangeHandler, (void*)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_net_DestNatChangeHandlerRef_t)(handlerRef);
}

/**
 * Remove destination NAT change handler.
 *
 * @param [in] handlerRef         The state handler reference returned by
 *                                taf_net_AddDestNatChangeHandler().
 *
 * @returns NA
 *
 */
void taf_net_RemoveDestNatChangeHandler
(
    taf_net_DestNatChangeHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * Add a destination NAT entry on default PDN.
 *
 * @param [in] privateIpAddr          The private IP address.
 * @param [in] privatePort            The private port.
 * @param [in] globalPort             The global port.
 * @param [in] ipProto                The ip protocol number.
 *
 * @returns LE_OK                     Success.
 *          LE_BAD_PARAMETER          Invalid private ip address.
 *          LE_FAULT                  Failed to add a destination NAT entry.
 */
le_result_t taf_net_AddDestNatEntryOnDefaultPdn
(
    const char* privateIpAddr,
    uint16_t privatePort,
    uint16_t globalPort,
    taf_net_IpProto_t ipProto
)
{
    le_result_t result;
    uint32_t profileId;
    auto &tafNat = taf_Nat::GetInstance();

    TAF_ERROR_IF_RET_VAL(privateIpAddr == NULL, LE_BAD_PARAMETER, "private ip address is NULL!");

    profileId = taf_dcs_GetDefaultProfileIndex();

    if(!tafNat.IsRmnetBringUp(profileId))
    {
        LE_ERROR("Rmnet interface is not brought up");
        return LE_FAULT;
    }

    result=tafNat.AddDestNatEntry(profileId, privateIpAddr, privatePort, globalPort, ipProto);
    return result;
}

/**
 * Remove a destination NAT entry on default PDN.
 *
 * @param [in] privateIpAddr          The private IP address.
 * @param [in] privatePort            The private port.
 * @param [in] globalPort             The global port.
 * @param [in] ipProto                The ip protocol number.
 *
 * @returns LE_OK                     Success.
 *          LE_BAD_PARAMETER          Invalid private ip address.
 *          LE_FAULT                  Failed to remove a destination NAT entry.
 */
le_result_t taf_net_RemoveDestNatEntryOnDefaultPdn
(
    const char* privateIpAddr,
    uint16_t privatePort,
    uint16_t globalPort,
    taf_net_IpProto_t ipProto
)
{
    le_result_t result;
    uint32_t profileId;
    auto &tafNat = taf_Nat::GetInstance();

    TAF_ERROR_IF_RET_VAL(privateIpAddr == NULL, LE_BAD_PARAMETER, "private ip address is NULL!");

    profileId = taf_dcs_GetDefaultProfileIndex();

    if(!tafNat.IsRmnetBringUp(profileId))
    {
        LE_ERROR("Rmnet interface is not brought up");
        return LE_FAULT;
    }

    result=tafNat.RemoveDestNatEntry( profileId, privateIpAddr, privatePort, globalPort, ipProto);
    return result;
}

/**
 * Add a destination NAT entry on demand PDN.
 *
 * @param [in] profileId                The profile id.
 * @param [in] privateIpAddr            The private IP address.
 * @param [in] privatePort              The private port.
 * @param [in] globalPort               The global port.
 * @param [in] ipProto                  The ip protocol number.
 *
 * @returns LE_OK                       Success.
 *          LE_BAD_PARAMETER            Invalid private ip address.
 *          LE_FAULT                    Failed to add a destination NAT entry.
 */
le_result_t taf_net_AddDestNatEntryOnDemandPdn
(
    uint32_t profileId,
    const char* privateIpAddr,
    uint16_t privatePort,
    uint16_t globalPort,
    taf_net_IpProto_t ipProto
)
{
    le_result_t result;
    auto &tafNat = taf_Nat::GetInstance();

    TAF_ERROR_IF_RET_VAL(privateIpAddr == NULL, LE_BAD_PARAMETER, "private ip address is NULL!");

    if(!tafNat.IsRmnetBringUp(profileId))
    {
        LE_ERROR("Rmnet interface is not brought up");
        return LE_FAULT;
    }

    result=tafNat.AddDestNatEntry( profileId, privateIpAddr, privatePort, globalPort, ipProto);
    return result;
}

/**
 * Remove a destination NAT entry on demand PDN.
 *
 * @param [in] profileId              The profile id.
 * @param [in] privateIpAddr          The private IP address.
 * @param [in] privatePort            The private port.
 * @param [in] globalPort             The global port.
 * @param [in] ipProto                The ip protocol number.
 *
 * @returns LE_OK                     Success.
 *          LE_BAD_PARAMETER          Invalid private ip address.
 *          LE_FAULT                  Failed to remove a destination NAT entry.
 */
le_result_t taf_net_RemoveDestNatEntryOnDemandPdn
(
    uint32_t profileId,
    const char* privateIpAddr,
    uint16_t privatePort,
    uint16_t globalPort,
    taf_net_IpProto_t ipProto
)
{
    le_result_t result;
    auto &tafNat = taf_Nat::GetInstance();

    TAF_ERROR_IF_RET_VAL(privateIpAddr == NULL, LE_BAD_PARAMETER, "private ip address is NULL!");

    if(!tafNat.IsRmnetBringUp(profileId))
    {
        LE_ERROR("Rmnet interface is not brought up");
        return LE_FAULT;
    }

    result=tafNat.RemoveDestNatEntry( profileId, privateIpAddr, privatePort, globalPort, ipProto);
    return result;
}

/**
 * Get a reference of an destination NAT entry list on default PDN.
 *
 * @returns NULL                     If there was some other error
 *          Others                   The reference of destination NAT entry list.
 */
taf_net_DestNatEntryListRef_t taf_net_GetDestNatEntryListOnDefaultPdn()
{
    uint32_t profileId;
    auto &tafNat = taf_Nat::GetInstance();

    profileId = taf_dcs_GetDefaultProfileIndex();

    if(!tafNat.IsRmnetBringUp(profileId))
        return NULL;

    return tafNat.GetDestNatEntryList(profileId);
}

/**
 * Get a reference of an destination NAT entry list on demand PDN.
 *
 * @param [in] profileId                The profile id.
 *
 * @returns NULL                     If there was some other error
 *          Others                   The reference of destination NAT entry list.
 */
taf_net_DestNatEntryListRef_t taf_net_GetDestNatEntryListOnDemandPdn
(
    uint32_t profileId
)
{
    auto &tafNat = taf_Nat::GetInstance();

    if(!tafNat.IsRmnetBringUp(profileId))
    {
        LE_ERROR("Rmnet interface is not brought up");
        return NULL;
    }

    return tafNat.GetDestNatEntryList(profileId);
}

/**
 * Get the reference of the first destination NAT entry from a list.
 *
 * @param [in] destNatEntryListRef      The destination nat entry list reference.
 *
 * @returns NULL                     If there was some other error
 *          Others                   The reference of the first destination NAT entry.
 */
taf_net_DestNatEntryRef_t taf_net_GetFirstDestNatEntry
(
    taf_net_DestNatEntryListRef_t destNatEntryListRef
)
{
    auto &tafNat = taf_Nat::GetInstance();

    return tafNat.GetFirstDestNatEntry(destNatEntryListRef);
}

/**
 * Get the reference of the next destination NAT entry from a list.
 *
 * @param [in] destNatEntryListRef      The destination nat entry list reference.
 *
 * @returns NULL                        If there was some other error
 *          Others                      The reference of the next destination NAT entry.
 */
taf_net_DestNatEntryRef_t taf_net_GetNextDestNatEntry
(
    taf_net_DestNatEntryListRef_t  destNatEntryListRef
)
{
    auto &tafNat = taf_Nat::GetInstance();

    return tafNat.GetNextDestNatEntry(destNatEntryListRef);
}

/**
 * Get infomation of an destination NAT entry from a list.
 *
 * @param [in] destNatEntryRef          The entry reference.
 * @param [out] privateIpAddrPtr        The private ip address.
 * @param [in] privateIpAddrPtrSize     The private ip address length.
 * @param [out] privatePortPtr          The private port.
 * @param [out] globalPortPtr           The global port.
 * @param [out] protoPtr                The ip protocol number.
 *
 * @returns LE_OK                       Success.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to get the destination NAT entry details.
 */
le_result_t taf_net_GetDestNatEntryDetails
(
    taf_net_DestNatEntryRef_t  destNatEntryRef,
    char* privateIpAddrPtr,
    size_t privateIpAddrPtrSize,
    uint16_t* privatePortPtr,
    uint16_t* globalPortPtr,
    taf_net_IpProto_t* protoPtr
)
{
    auto &tafNat = taf_Nat::GetInstance();

    return tafNat.GetDestNatEntryDetails(destNatEntryRef, privateIpAddrPtr, privateIpAddrPtrSize,
                                         privatePortPtr, globalPortPtr, protoPtr);
}

/**
 * Delete a reference of an destination nat entry list.
 *
 * @param [in] destNatEntryListRef      The destination nat entry list reference.
 *
 * @returns LE_OK                       Success.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to delete the reference.
 */
le_result_t taf_net_DeleteDestNatEntryList
(
    taf_net_DestNatEntryListRef_t  destNatEntryListRef
)
{
    auto &tafNat = taf_Nat::GetInstance();

    return tafNat.DeleteDestNatEntryList(destNatEntryListRef);
}

/*=========================================VLAN=========================================*/
/**
 * Create a VLAN.
 *
 * @param [in] vlanId                   The VLAN id(i.e 1-4094).
 * @param [in] isAccelerated            Is acceleration allowed.
 *
 * @returns NULL                        If isAccelerated conflicts with the old vlue
 *          Others                      The reference of VLAN.
 */
taf_net_VlanRef_t taf_net_CreateVlan
(
    uint16_t vlanId,
    bool isAccelerated
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.CreateVlan(vlanId, isAccelerated, taf_net_GetClientSessionRef());
}

/**
 * Sets the network type to a VLAN. By default Network type is LAN.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanNetworkType
(
    taf_net_VlanRef_t vlanRef,
    taf_net_NetworkType_t nwType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    return tafVlan.SetVlanNetworkType(vlanRef, nwType);
}

/**
 * Sets the backhaul type to a VLAN binding.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanBackhaulType
(
    taf_net_VlanRef_t vlanRef,
    taf_net_BackhaulType_t bhType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    return tafVlan.SetVlanBackhaulType(vlanRef, bhType);
}

/**
 * Sets the backhaul vlan ID to a VLAN binding.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanBackhaulVlanId
(
    taf_net_VlanRef_t vlanRef,
    uint16_t vlanId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    return tafVlan.SetVlanBackhaulVlanId(vlanRef, vlanId);
}

/**
 * Sets the phoneId of backhaul to a VLAN binding if backhaul type is WWAN.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanBackhaulPhoneId
(
    taf_net_VlanRef_t vlanRef,
    uint8_t phoneId
)
{
    le_result_t result;
    uint8_t slotId;
    auto &tafVlan = taf_Vlan::GetInstance();
    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result = network.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "failed to get slot id from phone id");

    return tafVlan.SetVlanBackhaulSlotId(vlanRef, slotId);
}

/**
 * Sets the profileID of backhaul to a VLAN binding if backhaul type is WWAN.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanBackhaulProfileId
(
    taf_net_VlanRef_t vlanRef,
    uint32_t profileId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    return tafVlan.SetVlanBackhaulProfileId(vlanRef, profileId);
}



/**
 * Set a VLAN priority.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] priority                 The priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to set priority to a VLAN.
 *
 */
le_result_t taf_net_SetVlanPriority
(
    taf_net_VlanRef_t vlanRef,
    uint8_t priority
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.SetVlanPriority(vlanRef, priority);
}


/**
 * Remove a VLAN. If the VLAN interface is present in this VLAN, it can't be removed.
 *
 * @param [in] vlanRef                  The VLAN reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Interface is present in this VLAN.
 */
le_result_t taf_net_RemoveVlan
(
    taf_net_VlanRef_t vlanRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.RemoveVlan(vlanRef);
}

/**
 * Get the VLAN reference by VLAN id.
 *
 * @param [in] vlanId                   VLAN id.
 *
 * @returns NULL                        Not found.
 *          Others                      The VLAN reference.
 */
taf_net_VlanRef_t taf_net_GetVlanById
(
    uint16_t vlanId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanRefById(vlanId, taf_net_GetClientSessionRef());
}

/**
 * Add a VLAN interface.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] ifType                   The interface type.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to add a VLAN interface.
 *          LE_TIMEOUT                  Time out.
 *
 * @note  Note:If the parameter isAccelerated is true when create VLAN, and then add the VLAN
 *             interface at the first time, the system will auto reboot after 5 seconds.
 */
le_result_t taf_net_AddVlanInterface
(
    taf_net_VlanRef_t vlanRef,
    taf_net_VlanIfType_t ifType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.AddVlanInterface(vlanRef, ifType);
}

/**
 * Remove a VLAN interface.
 *
 * @param [in] vlanRef                  The VLAN reference.
 * @param [in] ifType                   The interface type.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to remove a VLAN interface.
 *          LE_TIMEOUT                  Time out.
 *
 * @note  Note:If the parameter isAccelerated is true when create VLAN, and then remove the VLAN
 *             interface at the last time, the system will auto reboot after 5 seconds.
 */
le_result_t taf_net_RemoveVlanInterface
(
    taf_net_VlanRef_t vlanRef,
    taf_net_VlanIfType_t ifType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.RemoveVlanInterface(vlanRef, ifType);
}

/**
 * Get the reference of the VLAN interface list.
 *
 * @param [in] vlanRef                  The VLAN reference.
 *
 * @returns NULL                        Failure
 *          Others                      The reference of the VLAN interface list.
 */
taf_net_VlanIfListRef_t taf_net_GetVlanInterfaceList
(
    taf_net_VlanRef_t vlanRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanInterfaceList(vlanRef);
}

/**
 * Get the reference of the first VLAN interface with a list reference.
 *
 * @param [in] vlanIfListRef            The VLAN interface list reference.
 *
 * @returns NULL                        Failure
 *          Others                      The reference of the first VLAN interface.
 */
taf_net_VlanIfRef_t taf_net_GetFirstVlanInterface
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetFirstVlanInterface(vlanIfListRef);
}

/**
 * Get the reference of the next VLAN interface with a list reference.
 *
 * @param [in] vlanIfListRef            The reference of a VLAN interface list.
 *
 * @returns NULL                        Failure
 *          Others                      The reference of the next VLAN interface.
 */
taf_net_VlanIfRef_t taf_net_GetNextVlanInterface
(
    taf_net_VlanIfListRef_t  vlanIfListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetNextVlanInterface(vlanIfListRef);
}

/**
 * Delete the VLAN interface list.
 *
 * @param [in] vlanIfListRef            The reference of a VLAN interface list.
 *
 * @returns LE_OK                       Success.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to delete the reference.
 */
le_result_t taf_net_DeleteVlanInterfaceList
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.DeleteVlanInterfaceList(vlanIfListRef);
}

/**
 * Get the VLAN interface type with a VLAN interface reference.
 *
 * @param [in] vlanIfRef                The interface reference.
 *
 * @returns taf_net_VlanIfType_t        The interface type.
 */
taf_net_VlanIfType_t taf_net_GetVlanInterfaceType
(
    taf_net_VlanIfRef_t  vlanIfRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanInterfaceType(vlanIfRef);
}

/**
 * Get the VLAN priority with a VLAN interface reference.
 *
 * @param [in] vlanIfRef                The interface reference.
 * @param [out] priority                The VLAN priority.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 */
le_result_t taf_net_GetVlanPriority
(
    taf_net_VlanIfRef_t vlanIfRef,
    uint8_t* priority
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanPriority(vlanIfRef, priority);
}

/**
 * Get the reference of the VLAN entry list.
 *
 * @returns NULL                        Failure.
 *          Others                      The reference of the VLAN entry list.
 */
taf_net_VlanEntryListRef_t taf_net_GetVlanEntryList
(
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanEntryList();
}

/**
 * Get the reference of the first VLAN entry with a list reference.
 *
 * @param [in] vlanEntryListRef         The VLAN entry list reference.
 *
 * @returns NULL                        Failure.
 *          Others                      The reference of the first VLAN entry.
 */
taf_net_VlanEntryRef_t taf_net_GetFirstVlanEntry
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetFirstVlanEntry(vlanEntryListRef);
}

/**
 * Get the reference of the next VLAN entry with a list reference.
 *
 * @param [in] vlanEntryListRef         The VLAN entry list reference.
 *
 * @returns NULL                        Failure.
 *          Others                      The reference of the next VLAN entry.
 */
taf_net_VlanEntryRef_t taf_net_GetNextVlanEntry
(
    taf_net_VlanEntryListRef_t  vlanEntryListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetNextVlanEntry(vlanEntryListRef);
}

/**
 * Delete the VLAN entry reference.
 *
 * @param [in] vlanEntryListRef         The VLAN entry list reference.
 *
 * @returns LE_OK                       Success.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to delete the VLAN entry list.
 */
le_result_t taf_net_DeleteVlanEntryList
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.DeleteVlanEntryList(vlanEntryListRef);
}

/**
 * Get VLAN ID.
 *
 * @param [in] vlanEntryRef             The VLAN entry reference.
 *
 * @returns Others                      Reference is null or can't find VLAN id.
 *          1-4094                      Success
 */
int16_t taf_net_GetVlanId
(
    taf_net_VlanEntryRef_t  vlanEntryRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanId(vlanEntryRef);
}

/**
 * Is VLAN accelerated or not.
 *
 * @param [in] vlanEntryRef             The VLAN entry reference.
 * @param [out] isAcceleratedPtr        Is VLAN accelerated or not.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 */
le_result_t taf_net_IsVlanAccelerated
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    bool* isAcceleratedPtr
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.IsVlanAccelerated(vlanEntryRef, isAcceleratedPtr);
}

/**
 * Gets the NetworkType of VLAN by the VLAN entry reference..
 *
 * @param [in] vlanEntryRef             The VLAN entry reference.
 * @param [out] isAcceleratedPtr        Is VLAN accelerated or not.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                VLAN is not present.
 *          LE_BAD_PARAMETER            Invalid parameter.
 */
le_result_t taf_net_GetVlanNetworkType
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    taf_net_NetworkType_t* nwType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanNetworkType(vlanEntryRef, nwType);
}

/**
 * Get the bound profile ID.
 *
 * @param [in] vlanEntryRef             The VLAN entry reference.
 *
 * @returns less than 0                 Reference is null or can't find profile id.
 *          Others                      Success
 */
int32_t taf_net_GetVlanBoundProfileId
(
    taf_net_VlanEntryRef_t vlanEntryRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanProfileId(vlanEntryRef);
}

/**
 * Get the bound phone id and profile id.
 *
 * @param [in] vlanEntryRef             The VLAN entry reference.
 * @param [in] phoneId                  The phone id for VLAN association.
 *
 * @returns less than 0                 Reference is null or can't find profile id.
 *          Others                      Success
 */
le_result_t taf_net_GetVlanBoundPhoneId
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    uint8_t* phoneId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();

    return tafVlan.GetVlanPhoneId(vlanEntryRef, phoneId);
}

/**
 * Bind a VLAN with a particular profile ID. This API can only be supported on device mode 0
 *
 * @param [in] vlanRef                  The VLAN Reference.
 * @param [in] profileId                The profile id for VLAN association.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND:               VLAN not found
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to bind VLAN with profile.
 *          LE_TIMEOUT                  Time out.
 *
 * @note  If bind VLAN with default profile id, the system will auto reboot after 5 seconds
 */
le_result_t taf_net_BindVlanWithProfile
(
    taf_net_VlanRef_t vlanRef,
    uint32_t profileId
)
{
    le_result_t result;
    uint8_t slotId;
    auto &tafVlan = taf_Vlan::GetInstance();
    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result = network.getSlotIdFromPhoneId(DEFAULT_PHONE_ID_1, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "failed to get slot id from phone id");

    result=tafVlan.BindVlanWithProfile(vlanRef, slotId, profileId);
    return result;
}

/**
 * Bind a VLAN with a particular profile ID. This API can only be supported on device mode 0
 *
 * @param [in] vlanRef                  The VLAN Reference.
 * @param [in] phoneId                  The phone id for VLAN association.
 * @param [in] profileId                The profile id for VLAN association.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND:               VLAN not found
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to bind VLAN with profile.
 *          LE_TIMEOUT                  Time out.
 *
 * @note  If bind VLAN with default profile id and phone id, the system will auto reboot after 5
 *        seconds
 */
le_result_t taf_net_BindVlanWithProfileEx
(
    taf_net_VlanRef_t vlanRef,
    uint8_t phoneId,
    uint32_t profileId
)
{
    le_result_t result;
    uint8_t slotId;
    auto &tafVlan = taf_Vlan::GetInstance();
    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result = network.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "failed to get slot id from phone id");

    result=tafVlan.BindVlanWithProfile(vlanRef, slotId, profileId);
    return result;
}



/**
 * Unbind a VLAN From a particular profile ID.
 *
 * @param [in] vlanRef                  The VLAN Reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND:               VLAN not found
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to unbind VLAN from profile.
 *          LE_TIMEOUT                  Time out.
 *
 * @note if unbind VLAN from default profile id, the system will auto reboot after 5 seconds
 */
le_result_t taf_net_UnbindVlanFromProfile
(
    taf_net_VlanRef_t vlanRef
)
{
    le_result_t result;
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result=tafVlan.UnbindVlanFromProfile(vlanRef);
    return result;
}

/**
 * First event handler used by taf_net_AddHwAccelerationStateHandler().
 *
 * @param [in] reportPtr          event pointer.
 * @param [in] subHandlerFunc     Callback function from taf_net_AddHwAccelerationStateHandler().
 */
static void FirstHwAccelerationStateHandler(void *reportPtr, void *subHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(subHandlerFunc == nullptr, "Null ptr(subHandlerFunc)");

    taf_net_VlanHwAccelerationStateHandlerFunc_t handlerFunc =
                                (taf_net_VlanHwAccelerationStateHandlerFunc_t)subHandlerFunc;
    VlanHwAccelerationState_t *statePtr = static_cast<VlanHwAccelerationState_t *>(reportPtr);
    LE_DEBUG("VLANHWAccelerationState: %d", statePtr->state);
    handlerFunc(NULL, statePtr->state, le_event_GetContextPtr());

    // Release memory back to the hw acceleration event pool
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_net_VlanHwAccelerationState'
 *
 * Event to report when a change occurs in hardware acceleration state.<br>
 * If reported state is TAF_NET_VLAN_HW_ACC_INACTIVE: All existing data calls will take software
 * acceleration path.<br>
 * If reported state is TAF_NET_VLAN_HW_ACC_ACTIVE: All new data calls that are started after
 * this event invocation will be hardware accelerated. Data calls that are already started will
 * continue without hardware acceleration. Clients could stop and re-start active data calls in
 * order to use hardware acceleration.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_net_VlanHwAccelerationStateHandlerRef_t taf_net_AddVlanHwAccelerationStateHandler
(
    taf_net_VlanRef_t vlanRef,
    taf_net_VlanHwAccelerationStateHandlerFunc_t handlerPtr,
        ///< [IN] Handler for hardware acceleration state.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_net_AddVlanHwAccelerationStateHandler");

    LE_UNUSED(vlanRef);   // as of now acceleration is system level only hence vlanRef is not needed
                          // TBD will be implemented in later releases to notify per vlanRef

    TAF_ERROR_IF_RET_VAL((handlerPtr == NULL), nullptr, "Null ptr(handlerPtr)");
    auto &tafVlan = taf_Vlan::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                            "VlanHwAccelerationStateEvent",
                                            tafVlan.vlanHwAccelerationStateEvtId,
                                            FirstHwAccelerationStateHandler,
                                            (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_net_VlanHwAccelerationStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_dcs_HwAccelerationState'
 */
//--------------------------------------------------------------------------------------------------
void taf_net_RemoveVlanHwAccelerationStateHandler
(
    taf_net_VlanHwAccelerationStateHandlerRef_t handlerRef
)
{
    TAF_ERROR_IF_RET_NIL((handlerRef == NULL), "Null ptr(handlerRef)");
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    return;
}


/**
 * Binds a VLAN with a specified backhaul config.
 *
 * @param [in] vlanRef                  The VLAN Reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND:               VLAN not found
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to bind VLAN with profile.
 *          LE_TIMEOUT                  Time out.
 *
 * @note  If bind VLAN with default profile id and phone id, the system will auto reboot after 5
 *        seconds
 */
 le_result_t taf_net_BindVlanWithBackhaul
(
    taf_net_VlanRef_t vlanRef
)
{
    le_result_t result;
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result=tafVlan.BindVlanWithBackhaul(vlanRef);
    return result;
}

/**
 * Unbinds a VLAN from previous profile ID.
 *
 * @param [in] vlanRef                  The VLAN Reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND:               VLAN not found
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to unbind VLAN from profile.
 *          LE_TIMEOUT                  Time out.
 *
 * @note if unbind VLAN from default profile id, the system will auto reboot after 5 seconds
 */
le_result_t taf_net_UnbindVlanFromBackhaul
(
    taf_net_VlanRef_t vlanRef
)
{
    le_result_t result;
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");

    result=tafVlan.UnbindVlanFromBackhaul(vlanRef);
    return result;
}

taf_net_InterfaceRef_t taf_net_GetInterface
(
     taf_net_VlanIfType_t ifType
)
{
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.GetInterface(ifType);
}

le_result_t taf_net_RemoveInterface
(
    taf_net_InterfaceRef_t interfaceRef
)
{
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.RemoveInterface(interfaceRef);
}

le_result_t taf_net_SetIPPTOperation
(
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_Operation_t  operation
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");
    result=tafVlan.SetIPPTOperation(interfaceRef,operation);

    return result;

}

le_result_t taf_net_SetIPPTDeviceMacAddress
(
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_VlanIfType_t ifType,
    const char *macAddr
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "interfaceRef is null");
    result=tafVlan.SetIPPTDeviceMacAddress(interfaceRef,ifType,macAddr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Allows the client to set the IP passthrough configuration for a specific profile and vlan ID.
 * When operation is set to ENABLE, the client can add a new ipptConfig or modify an existing
 * configuration.
 *
 * If ipptConfig is not provided, the system will perform an IP passthrough operation on the
 * existing configuration.
 *
 * Configuration changes will be persistent across reboots.
 * @param [in] interfaceRef                  The VLAN Reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_FAULT -- Failed.
  *  - LE_BAD_PARAMETER -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_SetIPPassThroughConfig
(
    taf_net_InterfaceRef_t  interfaceRef,
    uint16_t vlanid
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "interfaceRef is null");
    result=tafVlan.SetIPPassThroughConfig(interfaceRef,vlanid);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current IP passthrough configuration for a specific profile ID and vlan ID.
 *
 * @return
 *   - NON-NULL -- Succeeded.
 *   - NULL -- Failed.
 */
//--------------------------------------------------------------------------------------------------
taf_net_InterfaceRef_t taf_net_GetIPPassThroughConfig
(
    uint16_t vlanId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.GetIPPassThroughConfig(vlanId);
}

le_result_t taf_net_GetIPPTOperation
(
    taf_net_InterfaceRef_t interfaceRef,
    taf_net_Operation_t*  operation
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");
    TAF_ERROR_IF_RET_VAL(operation == nullptr , LE_BAD_PARAMETER, "operation is null");
    result=tafVlan.GetIPPTOperation(interfaceRef,operation);

    return result;
}

le_result_t taf_net_GetIPPTDeviceMacAddress
(
    taf_net_InterfaceRef_t interfaceRef,
    taf_net_VlanIfType_t *ifType,
    char *macAddr,
    size_t macAddrSize
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");
    TAF_ERROR_IF_RET_VAL(ifType == nullptr , LE_BAD_PARAMETER, "ifType is null");
    result=tafVlan.GetIPPTDeviceMacAddress(interfaceRef,ifType,macAddr,macAddrSize);

    return result;

}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the IP configuration for an interface.
 * Prior to invoking this API, the data call should be up and running.
 *
 * Provides the ability to configure STATIC_IP or DYNAMIC_IP to a specified InterfaceType.
 *
 * When DataCallStatus, whose IP address is being passed through to this NAD, changes to NET_NO_NET,
 * this API must be invoked again with operation to DISABLE.
 * When DataCallStatus, whose IP address is being passed through to this NAD, changes to
 * NET_CONNECTED, this API must be invoked again with operation to ENABLE.
 * When DataCallStatus, whose IP address is being passed through to this NAD, changes to
 * NET_RECONFIGURED, this API must be invoked again with operation to RECONFIG.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_SetIPConfig
(
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t ifType,
    uint16_t vlanId
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "interfaceRef is null");
    result=tafVlan.SetIPConfig(interfaceRef,ipType,ifType,vlanId);

    return result;
}

le_result_t taf_net_SetIPConfigParams
(
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_IpAssignOperation_t ipOpr,
    taf_net_IpAssignType_t ipType
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "interfaceRef is null");
    result=tafVlan.SetIPConfigParams(interfaceRef,ipOpr,ipType);

    return result;
}

le_result_t taf_net_SetIPConfigAddressParams
(
    taf_net_InterfaceRef_t  interfaceRef,
    const taf_net_IpAddressInfo_t*  ipAddrInfo
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(interfaceRef == nullptr , LE_BAD_PARAMETER, "interfaceRef is null");
    result=tafVlan.SetIPConfigAddressParams(interfaceRef,ipAddrInfo);

    return result;

}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IP configuration for an interface.
 * Provides the ability to get the configuration for STATIC_IP or DYNAMIC_IP for a specific
 * InterfaceType and IpFamilyType.
 *
 * This API does not support IpFamilyType::IPV4V6. The client must invoke this
 * API multiple times to get IP configuration for IpFamilyType::IPV4 and IpFamilyType::IPV6.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_GetIPConfigParams
(
    taf_net_InterfaceRef_t vlanIPRef,
    taf_net_IpAssignOperation_t* ipOpr,
    taf_net_IpAssignType_t* ipType
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(vlanIPRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");
    TAF_ERROR_IF_RET_VAL(ipOpr == nullptr , LE_BAD_PARAMETER, "ipOpr is null");
    TAF_ERROR_IF_RET_VAL(ipType == nullptr , LE_BAD_PARAMETER, "ipType is null");
    result=tafVlan.GetIPConfigParams(vlanIPRef,ipOpr,ipType);
    return result;
}

le_result_t taf_net_GetIPConfigAddressParams
(
    taf_net_InterfaceRef_t vlanIPRef,
    taf_net_IpAddressInfo_t*  ipAddrInfo
)
{
    le_result_t result;

    auto &tafVlan = taf_Vlan::GetInstance();
    TAF_ERROR_IF_RET_VAL(vlanIPRef == nullptr , LE_BAD_PARAMETER, "vlanRef is null");
    result=tafVlan.GetIPConfigAddressParams(vlanIPRef,ipAddrInfo);
    return result;
}

taf_net_InterfaceRef_t taf_net_GetIPConfig
(
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t ifType,
    uint16_t vlanId
)
{
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.GetIPConfig(ipType,ifType,vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IP Passthrough feature configuration Network Address Translation (NAT) is enabled or not.
 * IP Passthrough with NAT or without NAT is a device level configuration.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_GetIPPTNatConfig
(
    taf_net_VlanRef_t vlanRef,
    bool       *isNatEnabled       ///< True when NAT enabled.
)
{
    LE_UNUSED(vlanRef);   // as of now IPPT NAT config is device level hence vlanRef is not needed.
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.GetIPPassThroughNatConfig(isNatEnabled);
}

//--------------------------------------------------------------------------------------------------
/**
 * Allows the client to configure the Network Address Translation (NAT) for IP passthrough
 * feature. Network Address Translation (NAT) is enabled or not.
 * IP Passthrough with NAT or without NAT is a device level configuration.
 * Configuration changes will be persistent across reboots.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_SetIPPTNatConfig
(
    taf_net_VlanRef_t vlanRef,
    bool       isNatEnabled
)
{
    LE_UNUSED(vlanRef);   // as of now IPPT NAT config is device level hence vlanRef is not needed.
    auto &tafVlan = taf_Vlan::GetInstance();
    return tafVlan.SetIPPassThroughNatConfig(isNatEnabled);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the backhaul preference for default bridge.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_GetBackhaulPreference
(
    taf_net_VlanRef_t       vlanRef,
    taf_net_BackhaulType_t* backhaulPrefListPtr,
    size_t* backhaulPrefListSizePtr
)
{
    LE_UNUSED(vlanRef);  //as of now only default bridge is allowed hence vlanRef is not needed.
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(backhaulPrefListPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(profileList)");
    TAF_ERROR_IF_RET_VAL(backhaulPrefListSizePtr == nullptr, LE_BAD_PARAMETER, "Null ptr(listSize)");

    return tafVlan.GetBackhaulPreference(backhaulPrefListPtr,backhaulPrefListSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the backhaul preference for default bridge.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_net_SetBackhaulPreference
(
    taf_net_VlanRef_t       vlanRef,
    const taf_net_BackhaulType_t* backhaulPrefListPtr,
    size_t backhaulPrefListSize
)
{
    LE_UNUSED(vlanRef);  //as of now only default bridge is allowed hence vlanRef is not needed.
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_VAL(backhaulPrefListPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(profileList)");

    return tafVlan.SetBackhaulPreference(backhaulPrefListPtr,backhaulPrefListSize);
}


/*=========================================L2TP=========================================*/

/**
 * Enable L2TP.
 *
 * @param [in] enableMss                Enable or disable MSS.
 * @param [in] enableMtu                Enable or disable MTU.
 * @param [in] mtuSize                  MTU size.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                L2TP is not found.
 *          LE_DUPLICATE                L2TP is already enabled.
 *          LE_FAULT                    Failed to enable L2TP.
 */
le_result_t taf_net_EnableL2tp
(
    bool enableMss,
    bool enableMtu,
    uint32_t mtuSize
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.EnableL2tpCmdSync(enableMss, enableMtu, mtuSize);
}

/**
 *  Asynchronously enables L2TP.
 *
 * @param [in] enableMss                Enable or disable MSS.
 * @param [in] enableMtu                Enable or disable MTU.
 * @param [in] mtuSize                  MTU size.
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_EnableL2tpAsync
(
    bool enableMss,
    bool enableMtu,
    uint32_t mtuSize,
    taf_net_AsyncL2tpHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
     auto &tafL2tp = taf_L2tp::GetInstance();

     return tafL2tp.EnableL2tpCmdAsync(enableMss, enableMtu, mtuSize, handlerPtr,
                                    contextPtr, taf_net_GetClientSessionRef());
}

/**
 * Disable L2TP.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                L2TP is not found.
 *          LE_BAD_PARAMETER            Bad parameter.
 *          LE_DUPLICATE                L2TP is already disabled.
 *          LE_FAULT                    Failed to disable L2TP.
 */
le_result_t taf_net_DisableL2tp
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.DisableL2tpCmdSync(taf_net_GetClientSessionRef());
}

/**
 *  Asynchronously disables L2TP.
 *
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_DisableL2tpAsync
(
    taf_net_AsyncL2tpHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
     auto &tafL2tp = taf_L2tp::GetInstance();
     
     return tafL2tp.DisableL2tpCmdAsync(handlerPtr, contextPtr, taf_net_GetClientSessionRef());
}

/**
 * Is L2TP enabled or not.
 *
 * @returns true                        L2TP is enabled.
 *          false                       L2TP is disabled.
 */
bool taf_net_IsL2tpEnabled
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.IsL2tpEnabled();
}

/**
 * Is L2TP MSS enabled or not.
 *
 * @returns true                        L2TP MSS is enabled.
 *          false                       L2TP MSS is disabled.
 */
bool taf_net_IsL2tpMssEnabled
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.IsL2tpMssEnabled();
}

/**
 * Is L2TP MTU enabled or not.
 *
 * @returns true                        L2TP MTU is enabled.
 *          false                       L2TP MTU is disabled.
 */
bool taf_net_IsL2tpMtuEnabled
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.IsL2tpMtuEnabled();
}

/**
 * Is L2TP MTU enabled or not.
 *
 * @returns uint32_t                    L2TP MTU size.
 */
uint32_t  taf_net_GetL2tpMtuSize
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetL2tpMtuSize();
}

/**
 * Create L2TP tunnel.
 *
 * @param [in] encaProto                Encapsulation protocol.
 * @param [in] locId                    Local tunnel id.
 * @param [in] peerId                   Peer tunnel id.
 * @param [in] peerIpAddr               Peer ip address.
 * @param [in] ifName                   Interface name.
 *
 * @returns nullptr                     Failed to create tunnel.
 *          others                      The reference of tunnel.
 */
taf_net_TunnelRef_t taf_net_CreateTunnel
(
    taf_net_L2tpEncapProtocol_t encaProto,
    uint32_t locId,
    uint32_t peerId,
    const char* peerIpAddr,
    const char* ifName
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.CreateTunnel(encaProto, locId, peerId, peerIpAddr,
                                ifName, taf_net_GetClientSessionRef());
}

/**
 * Remove L2TP tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to remove tunnel.
 */
le_result_t taf_net_RemoveTunnel
(
    taf_net_TunnelRef_t tunnelRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.RemoveTunnel(tunnelRef);
}

/**
 * Set udp port if the encapsulation protocol is UDP.
 *
 * @param [in] tunnelRef                Tunnel reference.
 * @param [in] localUdpPort             Local udp port.
 * @param [in] peerUdpPort              Peer udp port.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Encapsulation protol is not UDP.
 */
le_result_t taf_net_SetTunnelUdpPort
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t localUdpPort,
    uint32_t peerUdpPort
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.SetTunnelUdpPort(tunnelRef, localUdpPort, peerUdpPort);
}

/**
 *  Add a session into the tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 * @param [in] locId                    Local session id.
 * @param [in] peerId                   Peer session id.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to add session.
 */

le_result_t taf_net_AddSession
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t locId,
    uint32_t peerId
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.AddSession(tunnelRef, locId, peerId);
}

/**
 *  Remove a session from the tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 * @param [in] locId                    Local session id.
 * @param [in] peerId                   Peer session id.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to remove session.
 */
le_result_t taf_net_RemoveSession
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t locId,
    uint32_t peerId
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.RemoveSession(tunnelRef, locId, peerId);
}

/**
 *  Synchronously starts a tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to start tunnel.
 */

le_result_t taf_net_StartTunnel
(
    taf_net_TunnelRef_t tunnelRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.StartTunnelCmdSync(tunnelRef);
}

/**
 *  Asynchronously starts a tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_StartTunnelAsync
(
    taf_net_TunnelRef_t tunnelRef,
    taf_net_AsyncTunnelHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
     auto &tafL2tp = taf_L2tp::GetInstance();
     
     return tafL2tp.StartTunnelCmdAsync(tunnelRef, handlerPtr, contextPtr,
                                     taf_net_GetClientSessionRef());

}

/**
 *  Synchronously stops a tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Tunnel is not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to stop tunnel.
 */
le_result_t taf_net_StopTunnel
(
    taf_net_TunnelRef_t tunnelRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.StopTunnelCmdSync(tunnelRef);
}

/**
 *  Asynchronously stops a tunnel.
 *
 * @param [in] tunnelRef                Tunnel reference.
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_StopTunnelAsync
(
    taf_net_TunnelRef_t tunnelRef,
    taf_net_AsyncTunnelHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
     auto &tafL2tp = taf_L2tp::GetInstance();
     
     return tafL2tp.StopTunnelCmdAsync(tunnelRef, handlerPtr, contextPtr,
                                       taf_net_GetClientSessionRef());
}

/**
 * Get tunnel reference by local tunnel id.
 *
 * @param [in] locId                    Local tunnel id.
 *
 * @returns NULL                        Not found.
 *          others                      Reference of the tunnel.
 */
taf_net_TunnelRef_t taf_net_GetTunnelRefById
(
    uint32_t locId
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelRefById(locId, taf_net_GetClientSessionRef());
}

/**
 * Get the reference of L2TP tunnel entry list.
 *
 * @param [in]                          None.
 *
 * @returns NULL                        Failure
 *          Others                      The reference of the L2TP tunnel entry list.
 */
taf_net_TunnelEntryListRef_t taf_net_GetTunnelEntryList
(
    void
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelEntryList();
}

/**
 * Get the reference of the first L2TP tunnel with a list reference.
 *
 * @param [in] tunnelEntryListRef       Tunnel entry list reference.
 *
 * @returns NULL                        Failure
 *          Others                      Reference of the first L2TP tunnel entry.
 */
taf_net_TunnelEntryRef_t taf_net_GetFirstTunnelEntry
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetFirstTunnelEntry(tunnelEntryListRef);
}

/**
 * Get the reference of the next L2TP tunnel with a list reference.
 *
 * @param [in] tunnelEntryListRef       Tunnel entry list reference.
 *
 * @returns NULL                        Failure
 *          Others                      Reference of the next L2TP tunnel entry.
 */
taf_net_TunnelEntryRef_t taf_net_GetNextTunnelEntry
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetNextTunnelEntry(tunnelEntryListRef);
}

/**
 * Delete the L2TP tunnel entry list.
 *
 * @param [in] tunnelEntryListRef       Tunnel entry list reference.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to delete the L2TP tunnel entry list.
 */
le_result_t taf_net_DeleteTunnelEntryList
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.DeleteTunnelEntryList(tunnelEntryListRef);
}

/**
 * Get encapsulation protocol of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns taf_net_L2tpEncapProtocol_t
 */
taf_net_L2tpEncapProtocol_t taf_net_GetTunnelEncapProto
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelEncapProto(tunnelEntryRef);
}

/**
 * Get local id of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns 0                           Failed to get the local id.
 *          Others                      The local id of the tunnel.
 */
uint32_t taf_net_GetTunnelLocalId
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelLocalId(tunnelEntryRef);
}

/**
 * Get peer id of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns 0                           Failed to get peer id.
 *          Others                      Success.
 */
uint32_t taf_net_GetTunnelPeerId
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelPeerId(tunnelEntryRef);
}

/**
 * Get local udp port of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns 0                           Failed to get local udp port.
 *          Others                      Success.
 */
uint32_t taf_net_GetTunnelLocalUdpPort
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelLocalUdpPort(tunnelEntryRef);
}

/**
 * Get peer udp port of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns 0                           Failed to get peer udp port.
 *          Others                      Success.
 */
uint32_t taf_net_GetTunnelPeerUdpPort
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelPeerUdpPort(tunnelEntryRef);
}

/**
 * Get peer IP v6 address of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to get peer ipv6 address.
 */
le_result_t taf_net_GetTunnelPeerIpv6Addr
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* peerIpv6Addr,
    size_t peerIpv6AddrSize
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelPeerIpv6Addr(tunnelEntryRef, peerIpv6Addr, peerIpv6AddrSize);
}

/**
 * Get peer IP v4 address of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to get peer ipv4 address.
 */
le_result_t taf_net_GetTunnelPeerIpv4Addr
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* peerIpv4Addr,
    size_t peerIpv4AddrSize
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelPeerIpv4Addr(tunnelEntryRef, peerIpv4Addr, peerIpv4AddrSize);
}

/**
 * Get interface name of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 */
le_result_t taf_net_GetTunnelInterfaceName
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* ifName,
    size_t ifNameSize
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelInterfaceName(tunnelEntryRef, ifName, ifNameSize);
}

/**
 * Get ip type of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns taf_net_IpFamilyType_t
 */
taf_net_IpFamilyType_t taf_net_GetTunnelIpType
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetTunnelIpType(tunnelEntryRef);
}

/**
 * Get session config of a tunnel.
 *
 * @param [in] tunnelEntryRef           Reference of a L2TP tunnel entry.
 *
 * @returns LE_OK                       Success.
 *          LE_NOT_FOUND                Not found.
 *          LE_BAD_PARAMETER            Invalid parameter.
 *          LE_FAULT                    Failed to get session config.
 */
le_result_t taf_net_GetSessionConfig
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    taf_net_L2tpSessionConfig_t* sessionConfigPtr,
    size_t* sessionConfigSizePtr
)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    return tafL2tp.GetSessionConfig(tunnelEntryRef, sessionConfigPtr, sessionConfigSizePtr);
}

/*=========================================SOCKS=========================================*/

/**
 * Synchronously enables the Socks proxy service.
 *
 * @param None.
 *
 * @returns LE_OK                    Success.
 *          LE_FAULT                 Failed to enable socks
 *
 */
le_result_t taf_net_EnableSocks
(
)
{
    auto &tafSocks = taf_Socks::GetInstance();

    return tafSocks.EnableSocksCmdSync();
}

/**
 *  Asynchronously enables the Socks proxy service.
 *
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_EnableSocksAsync
(
    taf_net_AsyncSocksHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &tafSocks = taf_Socks::GetInstance();

    return tafSocks.EnableSocksCmdAsync(handlerPtr,contextPtr,taf_net_GetClientSessionRef());
}

/**
 * Synchronously disables the Socks proxy service.
 *
 * @param None.
 *
 * @returns LE_OK                    Success.
 *          LE_FAULT                 Failed to disable socks
 *
 */
le_result_t taf_net_DisableSocks
(
)
{
    auto &tafSocks = taf_Socks::GetInstance();

    return tafSocks.DisableSocksCmdSync();
}

/**
 *  Asynchronously disables the Socks proxy service.
 *
 * @param [in] handlerPtr               Asynchronous handler function.
 * @param [in] contextPtr               Context pointer.
 *
 * @returns None
 */
void taf_net_DisableSocksAsync
(
    taf_net_AsyncSocksHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
     auto &tafSocks = taf_Socks::GetInstance();

     return tafSocks.DisableSocksCmdAsync(handlerPtr,contextPtr,taf_net_GetClientSessionRef());
}

/**
 * Sets SOCKS authentication method.
 *
 * @param [in] authMethod               Authentication method.
 *
 * @returns LE_OK                       Success.
 *          LE_FAULT                    Failed to set socks authentication method
 *
 */
le_result_t taf_net_SetSocksAuthMethod
(
    taf_net_AuthMethod_t authMethod
)
{
    if(authMethod != TAF_NET_SOCKS_NONE && authMethod != TAF_NET_SOCKS_USER_PASSWD)
        return LE_FAULT;

    return taf_pa_net_SetSocksAuthMethod(authMethod);
}

/**
 * Gets SOCKS authentication method.
 *
 * @param None
 *
 * @returns SOCKS_UNKNOWN               Error.
 *          SOCKS_NONE                  No authentication.
 *          SOCKS_USER_PASSWD           User and password.
 *
 */
taf_net_AuthMethod_t taf_net_GetSocksAuthMethod
(
)
{
    return taf_pa_net_GetSocksAuthMethod();
}

/**
 * Sets SOCKS LAN interface.
 *
 * @param [in] ifName                   Interface name.
 *
 * @returns LE_OK                       Success.
 *          LE_FAULT                    Failed to set socks lan interface.
 *
 */
le_result_t taf_net_SetSocksLanInterface
(
    const char* ifName
)
{
    return taf_pa_net_SetSocksLanInterface(ifName);
}

/**
 * Gets SOCKS LAN interface.
 *
 * @param [out] ifName                  Interface name.
 *        [in] ifNameSize               Interface name size.
 *
 * @returns LE_OK                       Success.
 *          LE_FAULT                    Failed to get socks lan interface.
 *
 */
le_result_t taf_net_GetSocksLanInterface
(
    char* ifName,
    size_t ifNameSize
)
{
    return taf_pa_net_GetSocksLanInterface(ifName, ifNameSize);
}

/**
 * Adds SOCKS association between user name and profile id.
 *
 * @param [in] userName                 User name.
 *        [in] profileId                Profile id.
 *
 * @returns LE_OK                       Success.
 *          LE_FAULT                    Failed to add association between user and profile.
 *
 */
le_result_t taf_net_AddSocksAssociation
(
    const char* userName,
    uint32_t profileId
)
{
    return taf_pa_net_AddSocksAssociation(userName, profileId);
}

/**
 * Deletes SOCKS association between user name and profile id.
 *
 * @param [in] userName                 User name.
 *
 * @returns LE_OK                       Success.
 *          LE_FAULT                    Failed to delete association between user and profile.
 *
 */
le_result_t taf_net_RemoveSocksAssociation
(
    const char* userName
)
{
    return taf_pa_net_RemoveSocksAssociation(userName);
}

/*=========================================GSB=========================================*/

/**
 * Add generic software bridge configuration for an interface.
 *
 * @param [in] ifName                The interface name .
 *        [in] ifType                The interface type.
 *        [in] bandwidth             The band width.
 *
 * @returns LE_OK                    Succeeded to add a gsb configuration for an interface.
 *          LE_BAD_PARAMETER         Invalid parameter.
 *          LE_FAULT                 Failed to add a gsb configuration for an interface.
 *
 */
le_result_t taf_net_AddGsb
(
    const char* ifName,
    taf_net_GsbIfType_t ifType,
    uint32_t bandwidth
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.AddGsb(ifName, ifType, bandwidth);
}

/**
 * Remove generic software bridge configuration for an interface.
 *
 * @param [in] ifName                The interface name .
 *
 * @returns LE_OK                    Succeeded to remove a gsb configuration for an interface.
 *          LE_BAD_PARAMETER         Invalid parameter.
 *          LE_FAULT                 Failed to remove a gsb configuration for an interface.
 *
 */
le_result_t taf_net_RemoveGsb
(
    const char* ifName
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.RemoveGsb(ifName);
}

/**
 * Enable the generic software bridge in the system.
 *
 * @param   None.
 *
 * @returns LE_OK                    Success.
 *          LE_FAULT                 Failed to enable gsb.
 *
 */
le_result_t taf_net_EnableGsb()
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.EnableGsb(true);
}

/**
 * Disable the generic software bridge in the system.
 *
 * @param   None.
 *
 * @returns LE_OK                    Success.
 *          LE_FAULT                 Failed to disable gsb.
 *
 */
le_result_t taf_net_DisableGsb()
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.EnableGsb(false);
}

/**
 * Get the reference of the generic software bridge list.
 *
 * @returns NULL                     Failure.
 *          Others                   The reference of the generic software bridge list.
 */
taf_net_GsbListRef_t taf_net_GetGsbList()
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetGsbList();
}

/**
 * Get the reference of the first generic software bridge with a list reference.
 *
 * @param [in] gsbListRef      The generic software bridge list reference.
 *
 * @returns NULL                     Failure.
 *          Others                   The reference of the first generic software bridge.
 */
taf_net_GsbRef_t taf_net_GetFirstGsb
(
    taf_net_GsbListRef_t gsbListRef
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetFirstGsb(gsbListRef);
}

/**
 * Get the reference of the next generic software bridge with a list reference.
 *
 * @param [in] gsbListRef      The generic software bridge list reference.
 *
 * @returns NULL                     Failure.
 *          Others                   The reference of the next generic software bridge.
 */
taf_net_GsbRef_t taf_net_GetNextGsb
(
    taf_net_GsbListRef_t gsbListRef
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetNextGsb(gsbListRef);
}

/**
 * Delete the generic software bridge list reference.
 *
 * @param [in] gsbListRef      The generic software bridge list reference.
 *
 * @returns LE_OK                    Success.
 *          LE_BAD_PARAMETER         Invalid parameter.
 *          LE_FAULT                 Failed to delete the reference.
 */
le_result_t taf_net_DeleteGsbList
(
    taf_net_GsbListRef_t gsbListRef
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.DeleteGsbList(gsbListRef);
}

/**
 * Get the interface name of a generic software bridge with a bridge reference.
 *
 * @param [in]    gsbRef       The generic software bridge reference.
 *        [out]   ifNamePtr    The interface name.
 *        [out]    ifNamePtrSize     The interface name length.
 *
 * @returns LE_OK                    Success.
 *          LE_BAD_PARAMETER         Invalid parameter.
 *          LE_FAULT                 Failed to delete the reference.
 */
le_result_t taf_net_GetGsbInterfaceName
(
    taf_net_GsbRef_t gsbRef,
    char* ifNamePtr,
    size_t ifNamePtrSize
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetGsbInterfaceName(gsbRef, ifNamePtr, ifNamePtrSize);
}

/**
 * Get the interface type of a generic software bridge with a bridge reference.
 *
 * @param [in]    gsbRef       The generic software bridge reference.
 *
 * @returns taf_net_GsbIfType_t      The interface type.
 */
taf_net_GsbIfType_t taf_net_GetGsbInterfaceType
(
    taf_net_GsbRef_t gsbRef
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetGsbInterfaceType(gsbRef);
}

/**
 * Get the band width of a generic software bridge with a gsb reference.
 *
 * @param [in]    gsbRef       The generic software bridge reference.
 *
 * @returns int32_t      The bandwidth(in Mbps).
 */
int32_t taf_net_GetGsbBandWidth
(
    taf_net_GsbRef_t gsbRef
)
{
    auto &tafGsb = taf_Gsb::GetInstance();

    return tafGsb.GetGsbBandWidth(gsbRef);
}


COMPONENT_INIT
{

    taf_net_init();

    taf_nat_init();

    taf_vlan_init();

    taf_l2tp_init();

    taf_socks_init();

    taf_gsb_init();

}

