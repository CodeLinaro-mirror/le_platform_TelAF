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
#define NET_IP_PROTO_NUMBER_LEN      3

#define CHANGE_ROUTE_INTERFACE          "bridge0"
#define CHANGE_ROUTE_IP_V4_DEST_ADDR    "192.168.225.0"
#define CHANGE_ROUTE_IP_V4_PREFIX_LEN   "255.255.255.0"
#define CHANGE_ROUTE_IP_V6_DEST_ADDR    "fe80::1009:a5ff:fea7:99be"
#define CHANGE_ROUTE_IP_V6_PREFIX_LEN   "64"
#define METRIC                           99
#define CELLULAR_INTERFACE              "rmnet_data0"
#define NAT_ENTRY_PRIVATE_IP_ADDR       "111.111.111.11"
#define TEST_DESTINATION_NAT_ENTRY_NUM  3
#define TEST_VLAN_ENTRY_NUM  3

le_sem_Ref_t semaphore;

taf_net_RouteChangeHandlerRef_t routeChangeHandlerRef;
taf_net_GatewayChangeHandlerRef_t gatewayChangeHandlerRef;
taf_net_DNSChangeHandlerRef_t DNSChangeHandlerRef;
taf_net_DestNatChangeHandlerRef_t DestNatChangeHandlerRef;

static void NetRouteChangeHandlerFunc(const taf_net_RouteChangeInd_t* routeChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for route Change Indication (Begin)****");
    LE_INFO("----interface name: %s", routeChangeIndPtr->interfaceName);
    LE_INFO("----destination address: %s", routeChangeIndPtr->destAddr);
    LE_INFO("----subnetmask: %s", routeChangeIndPtr->prefixLength);
    LE_INFO("----metric: %d", routeChangeIndPtr->metric);
    LE_INFO("----action: %d", routeChangeIndPtr->action);

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
    LE_INFO("----interface name: %s", gatewayChangeIndPtr->interfaceName);
    LE_INFO("----destination address: %s", gatewayChangeIndPtr->gatewayAddr);
    LE_INFO("----ip type: %d", gatewayChangeIndPtr->ipType);

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
    LE_INFO("----ip addr1: %s", DNSChangeIndPtr->ipAddr1);
    LE_INFO("----ip addr2: %s", DNSChangeIndPtr->ipAddr2);
    LE_INFO("----ip type: %d", DNSChangeIndPtr->ipType);

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

static void DestNatChangeHandlerFunc(const taf_net_DestNatChangeInd_t* DestNatChangeIndPtr, void* contextPtr)
{
    LE_INFO("**** Handler for Destination Nat Change Indication (Begin)****");
    LE_INFO("----profileId: %d", DestNatChangeIndPtr->profileId);
    LE_INFO("----action: %d", DestNatChangeIndPtr->action);

    LE_INFO("**** Handler for Destination Nat Change Indication (End)****");
}

static void* DestNatThread(void* contextPtr)
{
    //  connect service in thread.
    taf_net_ConnectService();

    DestNatChangeHandlerRef = taf_net_AddDestNatChangeHandler(
        (taf_net_DestNatChangeHandlerFunc_t)DestNatChangeHandlerFunc, NULL);
    LE_ASSERT(DestNatChangeHandlerRef != NULL);

    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

static void NetworkGetInterfaceListTest()
{
    taf_net_InterfaceInfo_t intfInfoListPtr[50];
    size_t listSize ;

    LE_INFO("----get interface list test start");

    LE_ASSERT(taf_net_GetInterfaceList(NULL,&listSize) == LE_BAD_PARAMETER);

    LE_ASSERT(taf_net_GetInterfaceList(intfInfoListPtr,&listSize) == LE_OK);

    LE_INFO("---got interface list, num: %d", listSize);

    for(int i=0;i<listSize;i++)
    {
        LE_INFO("----interface name =%s, technology=%d,state=%d",intfInfoListPtr[i].interfaceName,
                intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
    }
}

static void NetworkChangeIpRouteTest()
{
    le_result_t result;

    LE_INFO("----add ip v4 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V4_DEST_ADDR,CHANGE_ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("----delete ip v4 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V4_DEST_ADDR,CHANGE_ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("----add ip v6 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V6_DEST_ADDR,CHANGE_ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);

    if (le_thread_Sleep(1))
    {
        LE_ERROR("Failed to sleep\n");
    }

    LE_INFO("----delete ip v6 route start");

    result=taf_net_ChangeRoute(CHANGE_ROUTE_INTERFACE,CHANGE_ROUTE_IP_V6_DEST_ADDR,CHANGE_ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);

}

static void NetworkGetInterfaceDefaultGatewayTest()
{
    char ipv4addr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6addr[NET_IPV6_ADDR_MAX_BYTES];

    LE_INFO("----get interface default gateway start");

    memset(ipv4addr, 0 , NET_IPV4_ADDR_MAX_BYTES);
    memset(ipv6addr, 0 , NET_IPV6_ADDR_MAX_BYTES);

    LE_ASSERT(taf_net_GetInterfaceGW(CELLULAR_INTERFACE,NULL , sizeof(ipv4addr), ipv6addr, sizeof(ipv6addr)) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceGW(CELLULAR_INTERFACE,ipv4addr , sizeof(ipv4addr), NULL, sizeof(ipv6addr)) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceGW(CELLULAR_INTERFACE,ipv4addr , sizeof(ipv4addr), ipv6addr, sizeof(ipv6addr)) == LE_OK);

    LE_INFO("----got default gateway address ipv4addr= %s,ipv6addr=%s",ipv4addr,ipv6addr);

    return ;
}

static void NetworkGetInterfaceDnsTest()
{
    taf_net_DnsServerAddresses_t dnsServerAddressesPtr;

    LE_INFO("----get interface DNS addresses start");

    LE_ASSERT(taf_net_GetInterfaceDNS(CELLULAR_INTERFACE,NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceDNS(CELLULAR_INTERFACE,&dnsServerAddressesPtr) == LE_OK);

    LE_INFO("----got ipv4 DNS1 is %s ,DNS2 is %s",dnsServerAddressesPtr.ipv4Addr1,dnsServerAddressesPtr.ipv4Addr2);
    LE_INFO("----got ipv6 DNS1 is %s ,DNS2 is %s",dnsServerAddressesPtr.ipv6Addr1,dnsServerAddressesPtr.ipv6Addr2);

    return ;
}

static void NetworkSetDnsTest()
{
    le_result_t result;

    LE_INFO("----Set dns test start");

    result=taf_net_SetDNS(CELLULAR_INTERFACE);
    LE_ASSERT(result == LE_OK);

    return;
}

static void NetworkBackupSetRestoreDefaultGatewayTest()
{
    le_result_t result;

    LE_INFO("----Backup default gateway test start");
    result=taf_net_BackupDefaultGW();
    //if no default gateway result is LE_NOT_FOUND
    if(result == LE_NOT_FOUND)
        LE_INFO("----Can't find default gateway,result =%d", result);
    else
    {
        LE_INFO("----Set default gateway test start");
        result=taf_net_SetDefaultGW(CELLULAR_INTERFACE);
        if(result == LE_NOT_FOUND)
            LE_INFO("----Can't find default gateway to be set from interface =%s,result =%d", CELLULAR_INTERFACE, result);
        else
        {
            LE_INFO("----Restore default gateway test start");
            result=taf_net_RestoreDefaultGW();
        }
    }
    return;
}

void VlanUnitTestFunc(void)
{
    LE_INFO("======== 4.1 Vlan Test ========");
    taf_net_VlanRef_t vlanRef[TEST_VLAN_ENTRY_NUM];
    int16_t retVlanId;
    int32_t retProfileId;
    bool isAccelerated=false;
    taf_net_VlanEntryRef_t vlanEntryRef;
    taf_net_VlanEntryListRef_t vlanEntryList;
    taf_net_VlanIfRef_t vlanIfRef;
    taf_net_VlanIfType_t vlanIfType;
    taf_net_VlanIfListRef_t vlanIfList[TEST_VLAN_ENTRY_NUM];
    const uint16_t vlanId[TEST_VLAN_ENTRY_NUM] = {103, 105, 107};
    const uint16_t profileId[TEST_VLAN_ENTRY_NUM] = {3, 5, 6};

    LE_ASSERT(taf_net_CreateVlan(5000, 0) == NULL);
    LE_ASSERT(taf_net_AddVlanInterface(NULL,TAF_NET_ETH) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_BindVlanWithProfile(NULL,5) == LE_BAD_PARAMETER);

    for (size_t i = 0; i < TEST_VLAN_ENTRY_NUM; i++)
    {
        //create vlan
        LE_ASSERT(taf_net_CreateVlan(vlanId[i], 0) != NULL);

        vlanRef[i]=taf_net_GetVlanById(vlanId[i]);
        LE_ASSERT(vlanRef[i] != NULL);

        //add vlan interface
        LE_ASSERT(taf_net_AddVlanInterface(vlanRef[i],TAF_NET_ETH) == LE_OK);

        LE_ASSERT(taf_net_AddVlanInterface(vlanRef[i],TAF_NET_ECM) == LE_OK);

        vlanIfList[i]=taf_net_GetVlanInterfaceList(vlanRef[i]);
        LE_ASSERT(vlanIfList[i] != NULL);

        vlanIfRef = taf_net_GetFirstVlanInterface(vlanIfList[i]);
        LE_ASSERT(vlanIfRef != NULL);

        vlanIfType=taf_net_GetVlanInterfaceType(vlanIfRef);
        LE_ASSERT(vlanIfType == TAF_NET_ETH);

        vlanIfRef = taf_net_GetNextVlanInterface(vlanIfList[i]);
        LE_ASSERT(vlanIfRef != NULL);

        vlanIfType=taf_net_GetVlanInterfaceType(vlanIfRef);
        LE_ASSERT(vlanIfType == TAF_NET_ECM);
        LE_ASSERT(taf_net_BindVlanWithProfile(vlanRef[i],profileId[i]) == LE_OK);
    }

    vlanEntryList=taf_net_GetVlanEntryList();
    LE_ASSERT(vlanEntryList != NULL);

    vlanEntryRef = taf_net_GetFirstVlanEntry(vlanEntryList);
    retVlanId = taf_net_GetVlanId(vlanEntryRef);
    LE_ASSERT(retVlanId == vlanId[0]);

    LE_ASSERT(taf_net_IsVlanAccelerated(vlanEntryRef,&isAccelerated) == LE_OK);

    retProfileId = taf_net_GetVlanBoundProfileId(vlanEntryRef);
    LE_ASSERT(retProfileId == profileId[0]);

    vlanEntryRef = taf_net_GetNextVlanEntry(vlanEntryList);
    retVlanId = taf_net_GetVlanId(vlanEntryRef);
    LE_ASSERT(retVlanId == vlanId[1]);

    LE_ASSERT(taf_net_IsVlanAccelerated(vlanEntryRef,&isAccelerated) == LE_OK);

    retProfileId = taf_net_GetVlanBoundProfileId(vlanEntryRef);
    LE_ASSERT(retProfileId == profileId[1]);

    vlanEntryRef = taf_net_GetNextVlanEntry(vlanEntryList);
    retVlanId = taf_net_GetVlanId(vlanEntryRef);
    LE_ASSERT(retVlanId == vlanId[2]);

    LE_ASSERT(taf_net_IsVlanAccelerated(vlanEntryRef,&isAccelerated) == LE_OK);

    retProfileId = taf_net_GetVlanBoundProfileId(vlanEntryRef);
    LE_ASSERT(retProfileId == profileId[2]);

    for (size_t i = 0; i < TEST_VLAN_ENTRY_NUM; i++)
    {
        LE_ASSERT(taf_net_UnbindVlanFromProfile(vlanRef[i]) == LE_OK);

        LE_ASSERT(taf_net_RemoveVlanInterface(vlanRef[i],TAF_NET_ECM) == LE_OK);

        LE_ASSERT(taf_net_RemoveVlanInterface(vlanRef[i],TAF_NET_ETH) == LE_OK);

        LE_ASSERT(taf_net_RemoveVlan(vlanRef[i]) == LE_OK);

        LE_ASSERT(taf_net_DeleteVlanInterfaceList(vlanIfList[i]) == LE_OK);
    }

    LE_ASSERT(taf_net_DeleteVlanEntryList(vlanEntryList) == LE_OK);

}

void NatDestNatUnitTestFunc(void)
{
    LE_INFO("======== 3.1 Destination NAT Entry Test ========");
    char ipaddr[NET_IPV6_ADDR_MAX_BYTES];
    uint16_t priPort;
    uint16_t glbPort;
    taf_net_IpProto_t proto;
    const uint16_t globalPort[TEST_DESTINATION_NAT_ENTRY_NUM] = {5000, 5001, 5002};
    const uint16_t privatePort[TEST_DESTINATION_NAT_ENTRY_NUM] = {6000, 6001, 6002};
    const uint16_t ipProtoNum[TEST_DESTINATION_NAT_ENTRY_NUM] = {6, 17, 6};//TCP,UDP,TCP

    LE_ASSERT(taf_net_AddDestNatEntryOnDefaultPdn("200.200.200", privatePort[0], globalPort[0], 6) == LE_BAD_PARAMETER);

    for (size_t i = 0; i < TEST_DESTINATION_NAT_ENTRY_NUM; i++)
    {
        LE_ASSERT(taf_net_AddDestNatEntryOnDefaultPdn(NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    }

    taf_net_DestNatEntryListRef_t listRef=taf_net_GetDestNatEntryListOnDefaultPdn();
    LE_ASSERT(listRef != NULL);
    taf_net_DestNatEntryRef_t entryRef = taf_net_GetFirstDestNatEntry(listRef);
    LE_ASSERT(entryRef != NULL);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(NULL, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, &proto) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(entryRef, NULL, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, &proto) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, NULL, &glbPort, &proto) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, NULL, &proto) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, &proto) == LE_OK);

    entryRef=taf_net_GetNextDestNatEntry(listRef);
    LE_ASSERT(entryRef != NULL);
    entryRef=taf_net_GetNextDestNatEntry(listRef);
    LE_ASSERT(entryRef != NULL);

    for (size_t i = 0; i < TEST_DESTINATION_NAT_ENTRY_NUM; i++)
    {
        LE_ASSERT(taf_net_RemoveDestNatEntryOnDefaultPdn(NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    }

    LE_ASSERT(taf_net_DeleteDestNatEntryList(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_DeleteDestNatEntryList(listRef) == LE_OK);
}

static void NetworkUnitTestFunc()
{
    LE_INFO("======== 2.1. Get interface list test ========");

    NetworkGetInterfaceListTest();

    LE_INFO("======== 2.2. Change route test ========");

    NetworkChangeIpRouteTest();

    LE_INFO("======== 2.3. Get interface default gateway test ========");

    NetworkGetInterfaceDefaultGatewayTest();

    LE_INFO("======== 2.4. Get interface DNS test ========");

    NetworkGetInterfaceDnsTest();

    LE_INFO("======== 2.5. Set DNS test ========");

    NetworkSetDnsTest();

    LE_INFO("======== 2.6. Backup,set and restore default gateway test ========");

    NetworkBackupSetRestoreDefaultGatewayTest();

    return ;
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

    LE_INFO("======== 1.4 DNS change Handler ========");
    threadRef = le_thread_Create("DestNatChangeThread", DestNatThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 2 Network unit test start========");
    NetworkUnitTestFunc();

    LE_INFO("======== 3 Destination NAT unit test start========");
    NatDestNatUnitTestFunc();

    LE_INFO("======== 4 Vlan unit test start========");
    VlanUnitTestFunc();
    LE_INFO("----all tests are passed");
    return NULL;
}

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     The initialization of Net Sevice Unit Test Component.

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("NetTestThread", UnitTestNetThread, NULL));
}