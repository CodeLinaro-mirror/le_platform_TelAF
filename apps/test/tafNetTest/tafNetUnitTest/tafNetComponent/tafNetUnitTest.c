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
 * @file       tafDataUnitTest.cpp
 * @brief      This file includes unit test functions of the Data Service.
 */

#include "legato.h"
#include "interfaces.h"

#define NET_IPV4_ADDR_MAX_BYTES      16
#define NET_IPV6_ADDR_MAX_BYTES      46

#define CHANGE_ROUTE_INTERFACE          "bridge0"
#define CHANGE_ROUTE_IP_V4_DEST_ADDR    "192.168.225.0"
#define CHANGE_ROUTE_IP_V4_PREFIX_LEN   "255.255.255.0"
#define CHANGE_ROUTE_IP_V6_DEST_ADDR    "fe80::1009:a5ff:fea7:99be"
#define CHANGE_ROUTE_IP_V6_PREFIX_LEN   "64"
#define METRIC                           99
#define CELLULAR_INTERFACE              "rmnet_data0"

le_sem_Ref_t semaphore;

taf_net_RouteChangeHandlerRef_t routeChangeHandlerRef;
taf_net_GatewayChangeHandlerRef_t gatewayChangeHandlerRef;
taf_net_DNSChangeHandlerRef_t DNSChangeHandlerRef;

static void ut_get_interface_list_test()
{
    taf_net_InterfaceInfo_t intfInfoListPtr[50];
    size_t listSize ;
    le_result_t result;

    LE_INFO("get interface list test start");
    result = taf_net_GetInterfaceList(intfInfoListPtr,&listSize);
    LE_ASSERT(result == LE_OK);
    LE_INFO("got interface list, num: %d, result: %d", listSize, result);

    for(int i=0;i<listSize;i++)
    {
        LE_INFO("interface name =%s, technology=%d,state=%d",intfInfoListPtr[i].interfaceName,intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
    }
}

static void ut_change_ip_route_test()
{
    le_result_t result;


    LE_INFO("add ip v4 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V4_DEST_ADDR,CHANGE_ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("delete ip v4 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V4_DEST_ADDR,CHANGE_ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("add ip v6 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V6_DEST_ADDR,CHANGE_ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("delete ip v6 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V6_DEST_ADDR,CHANGE_ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }
}

static void ut_get_interface_default_gateway()
{
    le_result_t result;
    char ipv4addr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6addr[NET_IPV6_ADDR_MAX_BYTES];
    LE_INFO("get interface default gateway start");
    memset(ipv4addr, 0 , NET_IPV4_ADDR_MAX_BYTES);
    memset(ipv6addr, 0 , NET_IPV6_ADDR_MAX_BYTES);

    result=taf_net_GetInterfaceGW(CELLULAR_INTERFACE,ipv4addr , sizeof(ipv4addr), ipv6addr, sizeof(ipv6addr));
    LE_ASSERT(result == LE_OK);

    LE_INFO("got ipv4 default gateway address is %s",ipv4addr);
    LE_INFO("got ipv6 default gateway address is %s",ipv6addr);

    return ;

}

static void ut_get_interface_dns()
{
    le_result_t result;
    taf_net_DnsServerAddresses_t dnsServerAddressesPtr;

    LE_INFO("get interface DNS addresses start");

    result=taf_net_GetInterfaceDNS(CELLULAR_INTERFACE,&dnsServerAddressesPtr);
    LE_ASSERT(result == LE_OK);

    LE_INFO("got ipv4 DNS1 is %s",dnsServerAddressesPtr.ipv4Addr1);
    LE_INFO("got ipv4 DNS2 is %s",dnsServerAddressesPtr.ipv4Addr2);
    LE_INFO("got ipv6 DNS1 is %s",dnsServerAddressesPtr.ipv6Addr1);
    LE_INFO("got ipv6 DNS2 is %s",dnsServerAddressesPtr.ipv6Addr2);

    return ;

}

static void ut_set_dns_test()
{
    le_result_t result;

    LE_INFO("Set dns test start");

    result=taf_net_SetDNS(CELLULAR_INTERFACE);
    LE_ASSERT(result == LE_OK);

}

static void ut_backup_set_restore_default_gateway_test()
{
    le_result_t result;

    LE_INFO("Backup default gateway test start");
    result=taf_net_BackupDefaultGW();
    LE_ASSERT(result == LE_OK);

    LE_INFO("Set default gateway test start");
    result=taf_net_SetDefaultGW(CELLULAR_INTERFACE);

    LE_ASSERT(result == LE_FAULT || result == LE_OK);

    LE_INFO("Restore default gateway test start");
    result=taf_net_RestoreDefaultGW();
    LE_ASSERT(result == LE_FAULT);

}

static void NetRouteChangeHandlerFunc(const taf_net_RouteChangeInd_t* routeChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for route Change Indication (Begin)****");
    LE_INFO("interface name: %s", routeChangeIndPtr->interfaceName);
    LE_INFO("destination address: %s", routeChangeIndPtr->destAddr);
    LE_INFO("subnetmask: %s", routeChangeIndPtr->prefixLength);
    LE_INFO("metric: %d", routeChangeIndPtr->metric);
    LE_INFO("action: %d", routeChangeIndPtr->action);

    LE_INFO("**** Handler for route Change Indication (End)****");
}
static void* NetRouteThread(void* contextPtr)
{
    //  connect service in thread.
    taf_net_ConnectService();

    routeChangeHandlerRef = taf_net_AddRouteChangeHandler(
        (taf_net_RouteChangeHandlerFunc_t)NetRouteChangeHandlerFunc, NULL);
    LE_ASSERT(routeChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

static void NetGatewayChangeHandlerFunc(const taf_net_GatewayChangeInd_t* gatewayChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for gateway change Indication (Begin)****");
    LE_INFO("interface name: %s", gatewayChangeIndPtr->interfaceName);
    LE_INFO("destination address: %s", gatewayChangeIndPtr->gatewayAddr);
    LE_INFO("ip type: %d", gatewayChangeIndPtr->ipType);

    LE_INFO("**** Handler for gateway change Indication (End)****");
}

static void* NetGatewayThread(void* contextPtr)
{
    //  connect service in thread.
    taf_net_ConnectService();

    gatewayChangeHandlerRef = taf_net_AddGatewayChangeHandler(
        (taf_net_GatewayChangeHandlerFunc_t)NetGatewayChangeHandlerFunc, NULL);
    LE_ASSERT(gatewayChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

static void NetDNSChangeHandlerFunc(const taf_net_DNSChangeInd_t* DNSChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for DNS Change Indication (Begin)****");
    LE_INFO("ip addr1: %s", DNSChangeIndPtr->ipAddr1);
    LE_INFO("ip addr2: %s", DNSChangeIndPtr->ipAddr2);
    LE_INFO("ip type: %d", DNSChangeIndPtr->ipType);

    LE_INFO("**** Handler for DNS Change Indication (End)****");
}

static void* NetDNSThread(void* contextPtr)
{
    //  connect service in thread.
    taf_net_ConnectService();

    DNSChangeHandlerRef = taf_net_AddDNSChangeHandler(
        (taf_net_DNSChangeHandlerFunc_t)NetDNSChangeHandlerFunc, NULL);
    LE_ASSERT(DNSChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

static void* UnitTestNetThread(void* contextPtr)
{
    LE_INFO("======== Test Thread of Net Service Start ========");

    //  connect service in thread.
    taf_net_ConnectService();

    semaphore = le_sem_Create("tafNetSem", 0);

    LE_INFO("======== 1. Network Add Handlers ========");

    LE_INFO("======== 1.1 Route change Handler ========");
    le_thread_Ref_t threadRef = le_thread_Create("NetRouteThread", NetRouteThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.2 Gateway change Handler ========");
    threadRef = le_thread_Create("NetGatewayThread", NetGatewayThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.3 DNS change Handler ========");
    threadRef = le_thread_Create("DNSChangeThread", NetDNSThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 2. Get interface list test ========");

    ut_get_interface_list_test();

    LE_INFO("======== 3. Change route test ========");

    ut_change_ip_route_test();

    LE_INFO("======== 4. Get interface default gateway test ========");

    ut_get_interface_default_gateway();

    LE_INFO("======== 5. Get interface DNS test ========");

    ut_get_interface_dns();

    LE_INFO("======== 6. Set DNS test ========");

    ut_set_dns_test();

    LE_INFO("======== 7. Set backup,set and restore default gateway test ========");

    ut_backup_set_restore_default_gateway_test();

    LE_INFO("all tests are passed");

    return NULL;
}

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     The initialization of Net Sevice Test Component.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("NetTestThread", UnitTestNetThread, NULL));
}