/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

using namespace telux::tafsvc;

void taf_net_init()
{
    LE_INFO("taf net component init start...\n");
    auto &network = taf_Net::GetInstance();
    network.Init();
    LE_INFO("taf net component init done...\n");

    return;
}
/**
 * Get network interface List
 *
 * @param [out] intfInfoList               The network interface information list
 * @param [out] listSize                   The size of the network interface list
 *
 * @returns LE_OK                          Success
 *          OTHER                          Failed to get the network interface information list.
 */
le_result_t taf_net_GetInterfaceList(taf_net_InterfaceInfo_t *intfInfoList, size_t *listSize)
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t profListSize = 0, intfIdx= 0;
    le_result_t result;
    char interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN];
    taf_dcs_ProfileRef_t profileRef = NULL;

    TAF_ERROR_IF_RET_VAL(intfInfoList == NULL, LE_FAULT, "intfInfoList is NULL!");
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT, "listSize is NULL!");

    result = taf_dcs_GetProfileList(profilesInfoPtr, &profListSize);

    if(result != LE_OK)
    {
        //To get wifi interface name information will be supported
        return LE_FAULT;
    }

    for (size_t i = 0; i < profListSize; i++)
    {

        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        profileRef=taf_dcs_GetProfile(profileInfoPtr->index);

        if(profileRef == NULL)
            continue;

        result=taf_dcs_GetInterfaceName(profileRef, interfaceName, TAF_NET_INTERFACE_NAME_MAX_LEN);

        if(result != LE_OK)
            continue;

        le_utf8_Copy(intfInfoList[intfIdx].interfaceName, interfaceName, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
        intfInfoList[intfIdx].tech = TAF_NET_TECH_CELLULAR;
        intfInfoList[intfIdx].state = TAF_NET_STATE_UP;
        intfIdx++;
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
 * @param [in] handlerRef               The state handler reference returned by taf_net_AddRouteChangeHandler().
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
 * @param [in] handlerRef               The state handler reference returned by taf_net_AddGatewayChangeHandler().
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
 * @param [in] handlerRef               The state handler reference returned by taf_net_AddDNSChangeHandler().
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
 * @param [in] preLenPtr                   The destination subnet mask or length to be used to change route
 * @param [in] metric                      The metric to be used to change route
 * @param [in] isAdd                       Add or delete a route
 *
 * @returns LE_OK                          Success
 *          OTHER                          Failed to add or remove a route.
 */

le_result_t taf_net_ChangeRoute(const char *namePtr, const char *destAddrPtr, const char *preLenPtr, uint16_t metric, taf_net_NetAction_t isAdd)
{

    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_FAULT, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_FAULT, "destination address is NULL!");
    TAF_ERROR_IF_RET_VAL(preLenPtr == NULL, LE_FAULT, "prefixLength is NULL!");

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
 *          OTHER                       Failed to Restore default gateway.
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
 *          OTHER                       Failed to set default gateway.
 */
le_result_t taf_net_SetDefaultGW(const char *namePtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    char ipv4GwAddr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6GwAddr[NET_IPV6_ADDR_MAX_BYTES];
    uint32_t profileId=0;
    le_result_t ipv4Ret, ipv6Ret, result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");

    auto &network = taf_Net::GetInstance();

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);

    if(result != LE_OK)
    { // to be supported: get wifi gateway address or ethernet gateway address
        LE_ERROR("interface : %s is invalid to get default gateway address", namePtr);
        return LE_FAULT;
    }

    //get cellular default gateway address
    profileRef= taf_dcs_GetProfile( profileId );

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
 * @param [out] ipv4AddrSize            The size of ip v4 gateway address
 * @param [out] ipv6AddrPtr             The ipv6 gateway address
 * @param [out] ipv6AddrSize            The size of ip v6 gateway address
 *
 * @returns LE_OK                       Get IPV4 and/or IPV6 gateway addresses from the interface successfully.
 *          OTHER                       Failed to get gateway addresses from the interface.
 */
le_result_t taf_net_GetInterfaceGW(const char *namePtr, char *ipv4AddrPtr, size_t ipv4AddrSize, char *ipv6AddrPtr, size_t ipv6AddrSize)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(ipv4AddrPtr == NULL, LE_BAD_PARAMETER, "ipv4AddrPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(ipv6AddrPtr == NULL, LE_BAD_PARAMETER, "ipv6AddrPtr is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);

    if(result != LE_OK)
    { // to be supported: get wifi gateway address or ethernet gateway address
        LE_ERROR("interface : %s is invalid to get default gateway address", namePtr);
        return LE_FAULT;
    }

    //get cellular gateway address
    profileRef= taf_dcs_GetProfile( profileId );

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
 *          LE_DUPLICATE              IPV4 exists in the system and can't get ipv6 dns address to be set from interface .
 *          LE_DUPLICATE              IPV6 exists in the system and can't get ipv4 dns address to be set from interface.
 *          LE_DUPLICATE              IPV6 and IPV4 exist in the system.
 *          OTHER                     Failed to set DNS addresses.
 */
le_result_t taf_net_SetDNS(const char *namePtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    char ipv4DnsAddrs[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV4_ADDR_MAX_BYTES] = {{0}, {0}};
    char ipv6DnsAddrs[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV6_ADDR_MAX_BYTES] = {{0}, {0}};
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr, &profileId);

    if(result != LE_OK)
    { // to be supported: get wifi DNS address or ethernet DNS address
        LE_ERROR("interface : %s is invalid to get default DNS address", namePtr);
        return LE_FAULT;
    }

    profileRef= taf_dcs_GetProfile( profileId );

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
 * @returns LE_OK                       Get IPV4 and/or IPV6 DNS addresses successfully from the interfaces.
 *          OTHER                       Failed to get DNS addresses.
 */
le_result_t taf_net_GetInterfaceDNS(const char *namePtr,taf_net_DnsServerAddresses_t *dnsSvrAddrsPtr)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    uint32_t profileId=0;
    le_result_t ipv4Ret,ipv6Ret,result=LE_NOT_FOUND;

    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(dnsSvrAddrsPtr == NULL, LE_BAD_PARAMETER, "dnsSvrAddrsPtr is NULL!");

    result = taf_dcs_GetProfileIdByInterfaceName(namePtr,&profileId);

    if(result != LE_OK)
    { // to be supported: get wifi DNS address or ethernet DNS address
        LE_ERROR("interface : %s is invalid to get default DNS address", namePtr);
        return LE_FAULT;
    }

    //get cellular DNS address
    profileRef= taf_dcs_GetProfile( profileId);

    if(profileRef == NULL)
    {
        LE_ERROR("interface : %s , can't get profile reference from cellular", namePtr);
        return LE_NOT_FOUND;
    }

    ipv6Ret = taf_dcs_GetIPv6DNSAddresses(profileRef, dnsSvrAddrsPtr->ipv6Addr1, sizeof(dnsSvrAddrsPtr->ipv6Addr1),
                                                      dnsSvrAddrsPtr->ipv6Addr2, sizeof(dnsSvrAddrsPtr->ipv6Addr2));

    ipv4Ret = taf_dcs_GetIPv4DNSAddresses(profileRef, dnsSvrAddrsPtr->ipv4Addr1, sizeof(dnsSvrAddrsPtr->ipv4Addr1),
                                                      dnsSvrAddrsPtr->ipv4Addr2, sizeof(dnsSvrAddrsPtr->ipv4Addr2));

    if(ipv6Ret != LE_OK && ipv4Ret != LE_OK)
    {
        LE_ERROR("interface : %s , can't get ipv4 or ipv6 DNS address", namePtr);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

COMPONENT_INIT
{

    taf_net_init();

}

