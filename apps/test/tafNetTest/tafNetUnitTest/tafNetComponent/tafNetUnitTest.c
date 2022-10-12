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

#define ROUTE_INTERFACE          "bridge0"
#define ROUTE_IP_V4_DEST_ADDR    "192.168.225.0"
#define ROUTE_IP_V4_PREFIX_LEN   "255.255.255.0"
#define ROUTE_IP_V6_DEST_ADDR    "fe80::1009:a5ff:fea7:99be"
#define ROUTE_IP_V6_PREFIX_LEN   "64"
#define METRIC                           99
#define RMNET0                           "rmnet_data0"
#define RMNET1                           "rmnet_data1"
#define NAT_ENTRY_PRIVATE_IP_ADDR       "111.111.111.11"
#define TEST_DESTINATION_NAT_ENTRY_NUM  3
#define TEST_VLAN_ENTRY_NUM  3
#define DEFAULT_MTU_SIZE 1422
#define ON_DEMAND_PDN_PROFILE_ID        2

le_sem_Ref_t semaphore;
int stopping_num = 0;

taf_net_RouteChangeHandlerRef_t routeChangeHandlerRef;
taf_net_GatewayChangeHandlerRef_t gatewayChangeHandlerRef;
taf_net_DNSChangeHandlerRef_t DNSChangeHandlerRef;
taf_net_DestNatChangeHandlerRef_t DestNatChangeHandlerRef;

static void NetRouteChangeHandlerFunc
(
    const taf_net_RouteChangeInd_t* routeChangeIndPtr,
    void* contextPtr
){
    LE_INFO("**** Handler for route Change Indication (Begin)****");
    LE_INFO("----interface name: %s", routeChangeIndPtr->interfaceName);
    LE_INFO("----destination address: %s", routeChangeIndPtr->destAddr);
    LE_INFO("----subnetmask: %s", routeChangeIndPtr->prefixLength);
    LE_INFO("----metric: %d", routeChangeIndPtr->metric);
    LE_INFO("----action: %d", routeChangeIndPtr->action);

    LE_INFO("**** Handler for route Change Indication (End)****");
}
static void* NetRouteThread(void* contextPtr){
    //  connect service in thread.
    taf_net_ConnectService();
    routeChangeHandlerRef = taf_net_AddRouteChangeHandler((taf_net_RouteChangeHandlerFunc_t)NetRouteChangeHandlerFunc, NULL);
    LE_ASSERT(routeChangeHandlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();
    return NULL;
}
static void NetGatewayChangeHandlerFunc(const taf_net_GatewayChangeInd_t* gatewayChangeIndPtr,void* contextPtr){
    LE_INFO("**** Handler for gateway change Indication (Begin)****");
    LE_INFO("----interface name: %s", gatewayChangeIndPtr->interfaceName);
    LE_INFO("----destination address: %s", gatewayChangeIndPtr->gatewayAddr);
    LE_INFO("----ip type: %d", gatewayChangeIndPtr->ipType);
    LE_INFO("**** Handler for gateway change Indication (End)****");
}
static void* NetGatewayThread(void* contextPtr){
    taf_net_ConnectService();
    gatewayChangeHandlerRef = taf_net_AddGatewayChangeHandler((taf_net_GatewayChangeHandlerFunc_t)NetGatewayChangeHandlerFunc, NULL);
    LE_ASSERT(gatewayChangeHandlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();
    return NULL;
}
static void NetDNSChangeHandlerFunc(const taf_net_DNSChangeInd_t* DNSChangeIndPtr,void* contextPtr){
    LE_INFO("**** Handler for DNS Change Indication (Begin)****");
    LE_INFO("----ip addr1: %s", DNSChangeIndPtr->ipAddr1);
    LE_INFO("----ip addr2: %s", DNSChangeIndPtr->ipAddr2);
    LE_INFO("----ip type: %d", DNSChangeIndPtr->ipType);
    LE_INFO("**** Handler for DNS Change Indication (End)****");
}
static void* NetDNSThread(void* contextPtr){
    taf_net_ConnectService();
    DNSChangeHandlerRef = taf_net_AddDNSChangeHandler((taf_net_DNSChangeHandlerFunc_t)NetDNSChangeHandlerFunc, NULL);
    LE_ASSERT(DNSChangeHandlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();
    return NULL;
}
static void DestNatChangeHandlerFunc(const taf_net_DestNatChangeInd_t* DestNatChangeIndPtr,void* contextPtr){
    LE_INFO("**** Handler for Destination Nat Change Indication (Begin)****");
    LE_INFO("----profileId: %d", DestNatChangeIndPtr->profileId);
    LE_INFO("----action: %d", DestNatChangeIndPtr->action);
    LE_INFO("**** Handler for Destination Nat Change Indication (End)****");
}
static void* DestNatThread(void* contextPtr){
    taf_net_ConnectService();
    DestNatChangeHandlerRef = taf_net_AddDestNatChangeHandler((taf_net_DestNatChangeHandlerFunc_t)DestNatChangeHandlerFunc, NULL);
    LE_ASSERT(DestNatChangeHandlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();
    return NULL;
}
static void NetworkGetInterfaceListTest(){
    taf_net_InterfaceInfo_t intfInfoListPtr[50];
    size_t listSize ;
    LE_INFO("----get interface list test start");
    LE_ASSERT(taf_net_GetInterfaceList(NULL,&listSize) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceList(intfInfoListPtr,&listSize) == LE_OK);
    LE_INFO("---got interface list, num: %d", listSize);
    for(int i=0;i<listSize;i++)
        LE_INFO("ifName =%s, tech=%d,state=%d",intfInfoListPtr[i].interfaceName,intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
}

static void NetworkChangeIpRouteTest(){
    le_result_t result=taf_net_ChangeRoute(ROUTE_INTERFACE,ROUTE_IP_V4_DEST_ADDR, ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);
    le_thread_Sleep(1);
    result=taf_net_ChangeRoute(ROUTE_INTERFACE,ROUTE_IP_V4_DEST_ADDR, ROUTE_IP_V4_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);
    le_thread_Sleep(1);
    result=taf_net_ChangeRoute(ROUTE_INTERFACE,ROUTE_IP_V6_DEST_ADDR, ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_ADD);
    LE_ASSERT(result == LE_OK);
    le_thread_Sleep(1);
    result=taf_net_ChangeRoute(ROUTE_INTERFACE,ROUTE_IP_V6_DEST_ADDR, ROUTE_IP_V6_PREFIX_LEN,METRIC,TAF_NET_DELETE);
    LE_ASSERT(result == LE_OK);
}

static void NetworkGetInterfaceDefaultGatewayTest(){
    char ipv4addr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6addr[NET_IPV6_ADDR_MAX_BYTES];
    LE_INFO("----get interface default gateway start");
    memset(ipv4addr, 0 , NET_IPV4_ADDR_MAX_BYTES);
    memset(ipv6addr, 0 , NET_IPV6_ADDR_MAX_BYTES);
    LE_ASSERT(taf_net_GetInterfaceGW(RMNET0,NULL , sizeof(ipv4addr),ipv6addr, sizeof(ipv6addr)) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceGW(RMNET0,ipv4addr , sizeof(ipv4addr),NULL, sizeof(ipv6addr)) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceGW(RMNET0,ipv4addr , sizeof(ipv4addr),ipv6addr, sizeof(ipv6addr)) == LE_OK);
    LE_INFO("got default gw addresses ipv4: %s, ipv6: %s from %s", ipv4addr, ipv6addr, RMNET0);
    LE_ASSERT(taf_net_GetInterfaceGW(RMNET1,ipv4addr , sizeof(ipv4addr),ipv6addr, sizeof(ipv6addr)) == LE_OK);
    LE_INFO("got default gw addresses ipv4: %s, ipv6: %s from %s", ipv4addr, ipv6addr, RMNET1);
    return ;
}
static void NetworkGetInterfaceDnsTest(){
    taf_net_DnsServerAddresses_t addPtr;
    LE_INFO("----get interface DNS addresses start");
    LE_ASSERT(taf_net_GetInterfaceDNS(RMNET0,NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_GetInterfaceDNS(RMNET0,&addPtr) == LE_OK);
    LE_INFO("----got ipv4 DNS1 is %s ,DNS2 is %s",addPtr.ipv4Addr1, addPtr.ipv4Addr2);
    LE_INFO("----got ipv6 DNS1 is %s ,DNS2 is %s",addPtr.ipv6Addr1, addPtr.ipv6Addr2);
    return ;
}
static void NetworkSetDnsTest(){
    le_result_t result;
    LE_INFO("----Set dns test start");
    result=taf_net_SetDNS(RMNET0);
    LE_ASSERT(result == LE_OK);
    return;
}
static void NetworkBackupSetRestoreDefaultGatewayTest(){
    le_result_t result;
    LE_INFO("----Backup default gateway test start");
    result=taf_net_BackupDefaultGW();
    LE_ASSERT(result == LE_OK);
    result=taf_net_SetDefaultGW(RMNET1);
    LE_ASSERT(result == LE_OK);
    result=taf_net_RestoreDefaultGW();
    LE_ASSERT(result == LE_OK);
    return;
}
static void VlanUnitTestFunc(void){
    taf_net_VlanRef_t vlanRef[TEST_VLAN_ENTRY_NUM];
    int16_t retVlanId = 0;
    int32_t retProfileId = 0;
    bool isAccelerated=false;
    taf_net_VlanEntryRef_t vlanEntryRef;
    taf_net_VlanIfRef_t vlanIfRef;
    taf_net_VlanIfType_t vlanIfType;
    taf_net_VlanIfListRef_t vlanIfList[TEST_VLAN_ENTRY_NUM];
    const uint16_t vlanId[TEST_VLAN_ENTRY_NUM] = {103, 105, 107};
    const uint16_t profileId[TEST_VLAN_ENTRY_NUM] = {3, 5, 6};
    LE_ASSERT(taf_net_CreateVlan(5000, 0) == NULL);
    LE_ASSERT(taf_net_AddVlanInterface(NULL,TAF_NET_ETH) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_BindVlanWithProfile(NULL,5) == LE_BAD_PARAMETER);
    for (size_t i = 0; i < TEST_VLAN_ENTRY_NUM; i++){
        LE_ASSERT(taf_net_CreateVlan(vlanId[i], 0) != NULL);
        vlanRef[i]=taf_net_GetVlanById(vlanId[i]);
        LE_ASSERT(vlanRef[i] != NULL);
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
    taf_net_VlanEntryListRef_t vlanEntryList=taf_net_GetVlanEntryList();
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
    for (size_t i = 0; i < TEST_VLAN_ENTRY_NUM; i++){
        LE_ASSERT(taf_net_UnbindVlanFromProfile(vlanRef[i]) == LE_OK);
        LE_ASSERT(taf_net_RemoveVlanInterface(vlanRef[i],TAF_NET_ECM) == LE_OK);
        LE_ASSERT(taf_net_RemoveVlanInterface(vlanRef[i],TAF_NET_ETH) == LE_OK);
        LE_ASSERT(taf_net_RemoveVlan(vlanRef[i]) == LE_OK);
        LE_ASSERT(taf_net_DeleteVlanInterfaceList(vlanIfList[i]) == LE_OK);
    }
    LE_ASSERT(taf_net_DeleteVlanEntryList(vlanEntryList) == LE_OK);
}
static void DisableL2tpAsyncHandlerFunc( le_result_t result, void* contextPtr){
    taf_net_VlanRef_t vlanRef = NULL;
    const uint16_t vlanId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {5, 6};
    LE_INFO("**** Handler for disable L2tp Asynchronously (Begin)****");
    if(result == LE_OK){
        for(int i = 0; i < TAF_NET_L2TP_MAX_TUNNEL_NUMBER; i++){
            vlanRef=taf_net_GetVlanById(vlanId[i]);
            LE_ASSERT(vlanRef != NULL);
            if(taf_net_RemoveVlanInterface(vlanRef, TAF_NET_ETH) != LE_OK){
                le_thread_Sleep(15);
                taf_net_RemoveVlanInterface(vlanRef, TAF_NET_ETH);
            }
            taf_net_RemoveVlan(vlanRef);
        }
        LE_INFO("======== L2TP test successfully ========");
        exit(EXIT_SUCCESS);
    }
    else
        exit(EXIT_FAILURE);
}
static void StopTunnelAsyncHandlerFunc(taf_net_TunnelRef_t tunnelRef,le_result_t result, void* contextPtr){
    LE_INFO("**** Handler for stop Tunnel Asynchronously (Begin)****");
    LE_INFO("tunnelRef= %p, result: %d", tunnelRef, result);
    if(result == LE_OK){
        LE_ASSERT(taf_net_RemoveSession(tunnelRef, 9, 9) == LE_OK);
        LE_ASSERT(taf_net_RemoveTunnel(tunnelRef) == LE_OK);
        taf_net_DisableL2tpAsync(DisableL2tpAsyncHandlerFunc, NULL);
    }
    LE_INFO("**** Handler for stop Tunnel Asynchronously (End)****");
}
static void StartTunnelAsyncHandlerFunc(taf_net_TunnelRef_t tunnelRef,le_result_t result, void* contextPtr){
    LE_INFO("**** Handler for Start Tunnel Asynchronously (Begin)****");
    LE_INFO("tunnelRef= %p, result: %d", tunnelRef, result);
    taf_net_StopTunnelAsync(tunnelRef,StopTunnelAsyncHandlerFunc,NULL);
    LE_INFO("**** Handler for Start Tunnel Asynchronously (End)****");
}
static void EnableL2tpAsyncHandlerFunc(le_result_t result, void* contextPtr){
    LE_INFO("**** Handler for enable L2tp Asynchronously (Begin)****");
    LE_INFO("result: %d", result);//tmp problem
    if(result == LE_OK){
        taf_net_TunnelRef_t tunnelRef=taf_net_CreateTunnel(TAF_NET_L2TP_IP, 3, 3, "fd53:7cb8:383:5::2", "eth0.5");
        LE_ASSERT(tunnelRef != NULL);
        LE_ASSERT(taf_net_AddSession(tunnelRef, 9, 9) == LE_OK);
        taf_net_StartTunnelAsync(tunnelRef,StartTunnelAsyncHandlerFunc,NULL);
    }
    LE_INFO("**** Handler for enable L2tp Asynchronously (End)****");
}
static void L2tpUnitTestFunc(void){
    int i = 0;
    le_result_t ret = LE_OK;
    size_t sessionNum=3;
    taf_net_IpFamilyType_t ipType;
    const uint32_t localUdpPort = 5555, peerUdpPort = 6666;
    taf_net_TunnelEntryListRef_t listRef= NULL;
    char ipv6addr[TAF_NET_IPV6_ADDR_MAX_LEN];
    char retInterface[TAF_NET_INTERFACE_NAME_MAX_LEN];
    taf_net_L2tpSessionConfig_t sessionConfig[2];
    taf_net_VlanRef_t vlanRef[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {NULL,NULL};
    const uint16_t vlanId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {5, 6};
    taf_net_TunnelRef_t tunnelRef[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {NULL,NULL};
    const uint32_t localTunnelId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {1, 2};
    const uint32_t peerTunnelId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER] = {1, 2};
    taf_net_L2tpEncapProtocol_t encaproto[TAF_NET_L2TP_MAX_TUNNEL_NUMBER]={TAF_NET_L2TP_IP, TAF_NET_L2TP_UDP};
    const uint32_t localSessionId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER][TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL] = {{1, 2, 3}, {4, 5, 6}};
    const uint32_t peerSessionId[TAF_NET_L2TP_MAX_TUNNEL_NUMBER][TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL] = {{11, 22, 33}, {44, 55, 66}};
    const char ipaddr[TAF_NET_L2TP_MAX_TUNNEL_NUMBER][TAF_NET_IPV6_ADDR_MAX_LEN] = {"fd53:7cb8:383:5::2", "fd53:7cb8:383:6::2"};
    const char interface[TAF_NET_L2TP_MAX_TUNNEL_NUMBER][TAF_NET_INTERFACE_NAME_MAX_LEN] ={"eth0.5","eth0.6"};

    LE_INFO("======== Create vlan ========");
    for(i = 0; i < TAF_NET_L2TP_MAX_TUNNEL_NUMBER; i++){
        vlanRef[i] = taf_net_CreateVlan(vlanId[i], 0);
        LE_ASSERT(vlanRef[i] != NULL);
        LE_ASSERT(taf_net_AddVlanInterface(vlanRef[i],TAF_NET_ETH) == LE_OK);
    }
    LE_INFO("======== Enable/disable l2tp ========");
    //telsdk has issues, need to set 0 0 0 ,then enable with other parameters
    LE_ASSERT(taf_net_EnableL2tp(0,0,0) == LE_OK);
    LE_ASSERT(taf_net_EnableL2tp(1,1,0) == LE_OK);
    LE_ASSERT(taf_net_IsL2tpEnabled() == true);
    LE_ASSERT(taf_net_IsL2tpMssEnabled() == true);
    LE_ASSERT(taf_net_IsL2tpMtuEnabled() == true);
    LE_ASSERT(taf_net_GetL2tpMtuSize() == DEFAULT_MTU_SIZE);
    LE_ASSERT(taf_net_DisableL2tp() == LE_OK);
    LE_ASSERT(taf_net_EnableL2tp(0,0,0) == LE_OK);
    LE_ASSERT(taf_net_EnableL2tp(1,1,1300) == LE_OK);
    LE_ASSERT(taf_net_IsL2tpEnabled() == true);
    LE_ASSERT(taf_net_IsL2tpMssEnabled() == true);
    LE_ASSERT(taf_net_IsL2tpMtuEnabled() == true);
    LE_ASSERT(taf_net_GetL2tpMtuSize() == 1300);
    LE_INFO("======== Create tunnel========");
    for(i = 0; i < TAF_NET_L2TP_MAX_TUNNEL_NUMBER; i++){
        tunnelRef[i]=taf_net_CreateTunnel(encaproto[i], localTunnelId[i], peerTunnelId[i], "8.8.8", interface[i]);
        LE_ASSERT(tunnelRef[i] == NULL);
        tunnelRef[i]=taf_net_CreateTunnel(encaproto[i], localTunnelId[i], peerTunnelId[i], ipaddr[i], interface[i]);
        LE_ASSERT(tunnelRef[i] != NULL);
        LE_ASSERT(taf_net_RemoveTunnel(NULL) == LE_BAD_PARAMETER);
        LE_ASSERT(taf_net_RemoveTunnel(tunnelRef[i]) == LE_OK);
        tunnelRef[i]=taf_net_CreateTunnel(encaproto[i], localTunnelId[i], peerTunnelId[i], ipaddr[i], interface[i]);
        LE_ASSERT(tunnelRef[i] != NULL);
        LE_ASSERT(taf_net_SetTunnelUdpPort(NULL, localUdpPort, peerUdpPort) == LE_BAD_PARAMETER);
        if(encaproto[i] == TAF_NET_L2TP_IP)
            LE_ASSERT(taf_net_SetTunnelUdpPort(tunnelRef[i], localUdpPort, peerUdpPort) == LE_FAULT);
        LE_ASSERT(taf_net_AddSession(NULL, localSessionId[i][0], peerSessionId[i][0]) == LE_BAD_PARAMETER);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], localSessionId[i][0], peerSessionId[i][0]) == LE_OK);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], localSessionId[i][0], 0) == LE_FAULT);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], 0, peerSessionId[i][0]) == LE_FAULT);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], localSessionId[i][1], peerSessionId[i][1]) == LE_OK);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], localSessionId[i][2], peerSessionId[i][2]) == LE_OK);
        LE_ASSERT(taf_net_AddSession(tunnelRef[i], 44, 44) == LE_FAULT);
        LE_ASSERT(taf_net_RemoveSession(tunnelRef[i], localSessionId[i][2], 0) == LE_FAULT);
        LE_ASSERT(taf_net_RemoveSession(tunnelRef[i], localSessionId[i][2], peerSessionId[i][2]) == LE_OK);
        LE_ASSERT(taf_net_RemoveSession(tunnelRef[i], localSessionId[i][1], peerSessionId[i][1]) == LE_OK);
        LE_ASSERT(taf_net_StartTunnel(NULL) == LE_BAD_PARAMETER);
        if(encaproto[i] == TAF_NET_L2TP_UDP)
            LE_ASSERT(taf_net_SetTunnelUdpPort(tunnelRef[i], localUdpPort, peerUdpPort) == LE_OK);
        LE_ASSERT(taf_net_GetTunnelRefById(localTunnelId[i]) == tunnelRef[i]);
        LE_ASSERT(taf_net_StartTunnel(tunnelRef[i]) == LE_OK);
        le_thread_Sleep(3);
    }
    listRef = taf_net_GetTunnelEntryList();
    if(listRef !=NULL){
        taf_net_TunnelEntryRef_t entryRef = taf_net_GetFirstTunnelEntry(listRef);
        i=0;
        while(entryRef != NULL){
            LE_ASSERT(taf_net_GetTunnelLocalId(entryRef) == localTunnelId[i]);
            LE_ASSERT(taf_net_GetTunnelPeerId(entryRef) == peerTunnelId[i]);
            LE_ASSERT(taf_net_GetTunnelEncapProto(entryRef) == encaproto[i]);
            if(encaproto[i] == TAF_NET_L2TP_UDP){
                LE_ASSERT(taf_net_GetTunnelLocalUdpPort(entryRef) == localUdpPort);
                LE_ASSERT(taf_net_GetTunnelPeerUdpPort(entryRef) == peerUdpPort);
            }
            ipType=taf_net_GetTunnelIpType(entryRef);
            if(ipType == TAF_NET_L2TP_IPV6){
                ret=taf_net_GetTunnelPeerIpv6Addr(entryRef, ipv6addr, TAF_NET_IPV6_ADDR_MAX_LEN);
                LE_ASSERT(ret == LE_OK);
                LE_ASSERT(strncmp(ipv6addr, ipaddr[i], TAF_NET_IPV6_ADDR_MAX_LEN) == 0);
            }
            ret=taf_net_GetTunnelInterfaceName(entryRef, retInterface, TAF_NET_INTERFACE_NAME_MAX_LEN);
            LE_ASSERT(ret == LE_OK);
            LE_ASSERT(strncmp(retInterface, interface[i], TAF_NET_INTERFACE_NAME_MAX_LEN) == 0);
            ret=taf_net_GetSessionConfig(entryRef, sessionConfig, &sessionNum);
            LE_ASSERT(ret == LE_OK);
            for(int j =0; j< sessionNum;j++){
                LE_ASSERT(sessionConfig[j].locId == localSessionId[i][j]);
                LE_ASSERT(sessionConfig[j].peerId == peerSessionId[i][j]);
            }
            entryRef=taf_net_GetNextTunnelEntry(listRef);
            i++;
        }
        taf_net_DeleteTunnelEntryList(listRef);
    }
    LE_INFO("======== stop tunnel ========");
    for(i = 0; i < TAF_NET_L2TP_MAX_TUNNEL_NUMBER; i++){
        LE_ASSERT(taf_net_StopTunnel(tunnelRef[i]) == LE_OK);
        for(int j = 0; j < 1;j ++)
            LE_ASSERT(taf_net_RemoveSession(tunnelRef[i], localSessionId[i][j], peerSessionId[i][j]) == LE_OK);
        LE_ASSERT(taf_net_RemoveTunnel(tunnelRef[i]) == LE_OK);
    }
    LE_INFO("======== Disable L2TP ========");
    LE_ASSERT(taf_net_DisableL2tp() == LE_OK);
    le_thread_Sleep(10);
    LE_INFO("======== Async L2TP test ========");
    taf_net_EnableL2tpAsync(0,0,0,EnableL2tpAsyncHandlerFunc, NULL);
    le_event_RunLoop();
}
static void StopSocksAsyncHandlerFunc( le_result_t result, void* contextPtr){
    LE_INFO("**** Handler for stop socks Asynchronously (Begin)****");
    LE_INFO("result: %d", result);
    if(result == LE_OK){
        LE_INFO("======== SOCKS test successfully ========");
        exit(EXIT_SUCCESS);
    }else
        exit(EXIT_FAILURE);
}
static void StartSocksAsyncHandlerFunc(le_result_t result, void* contextPtr){
    LE_INFO("**** Handler for start socks Asynchronously (Begin)****");
    LE_INFO("result: %d", result);
    le_thread_Sleep(1);
    if(result == LE_OK)
        taf_net_DisableSocksAsync(StopSocksAsyncHandlerFunc,NULL);
    LE_INFO("**** Handler for start socks Asynchronously (End)****");
}
static void SocksUnitTestFunc(void){
    char ifName[50];
    taf_net_AuthMethod_t authType;
    LE_ASSERT(taf_net_SetSocksAuthMethod(TAF_NET_SOCKS_NONE) == LE_OK);
    authType = taf_net_GetSocksAuthMethod();
    LE_ASSERT(authType == TAF_NET_SOCKS_NONE);
    LE_ASSERT(taf_net_SetSocksAuthMethod(TAF_NET_SOCKS_USER_PASSWD) == LE_OK);
    authType = taf_net_GetSocksAuthMethod();
    LE_ASSERT(authType == TAF_NET_SOCKS_USER_PASSWD);
    LE_ASSERT(taf_net_SetSocksLanInterface("eth0.5") == LE_OK);
    LE_ASSERT(taf_net_GetSocksLanInterface(ifName,50) == LE_OK);
    LE_ASSERT(strncmp(ifName,"eth0.5", 50) == 0);
    LE_ASSERT(taf_net_SetSocksLanInterface("bridge0") == LE_OK);
    LE_ASSERT(taf_net_AddSocksAssociation("test",4) == LE_OK);
    LE_ASSERT(taf_net_RemoveSocksAssociation("test") == LE_OK);
    LE_ASSERT(taf_net_AddSocksAssociation("testnew",5) == LE_OK);
    LE_ASSERT(taf_net_RemoveSocksAssociation("testnew") == LE_OK);
    LE_INFO("Enabling SOCKS...");
    LE_ASSERT(taf_net_EnableSocks()== LE_OK);
    le_thread_Sleep(2);
    taf_net_DisableSocks();
    LE_INFO("======== Async socks Test ========");
    le_thread_Sleep(2);
    taf_net_EnableSocksAsync(StartSocksAsyncHandlerFunc,NULL);
    le_event_RunLoop();
}
static void DestNatUnitTestFunc(void){
    char ipaddr[NET_IPV6_ADDR_MAX_BYTES];
    uint16_t priPort,glbPort;
    taf_net_IpProto_t proto;
    taf_net_DestNatEntryListRef_t listRef = NULL;
    taf_net_DestNatEntryRef_t entryRef = NULL;
    const uint16_t globalPort[TEST_DESTINATION_NAT_ENTRY_NUM] = {5000, 5001, 5002};
    const uint16_t privatePort[TEST_DESTINATION_NAT_ENTRY_NUM] = {6000, 6001, 6002};
    const uint16_t ipProtoNum[TEST_DESTINATION_NAT_ENTRY_NUM] = {6, 17, 6};//TCP,UDP,TCP

    LE_ASSERT(taf_net_AddDestNatEntryOnDefaultPdn("200.200.200", privatePort[0], globalPort[0], 6) == LE_BAD_PARAMETER);
    for (size_t i = 0; i < TEST_DESTINATION_NAT_ENTRY_NUM; i++)
        LE_ASSERT(taf_net_AddDestNatEntryOnDefaultPdn(NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    listRef=taf_net_GetDestNatEntryListOnDefaultPdn();
    LE_ASSERT(listRef != NULL);
    entryRef = taf_net_GetFirstDestNatEntry(listRef);
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
        LE_ASSERT(taf_net_RemoveDestNatEntryOnDefaultPdn(NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    LE_ASSERT(taf_net_DeleteDestNatEntryList(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_DeleteDestNatEntryList(listRef) == LE_OK);
    LE_ASSERT(taf_net_AddDestNatEntryOnDemandPdn(ON_DEMAND_PDN_PROFILE_ID, "200.200.200", privatePort[0], globalPort[0], 6) == LE_BAD_PARAMETER);
    for (size_t i = 0; i < TEST_DESTINATION_NAT_ENTRY_NUM; i++)
        LE_ASSERT(taf_net_AddDestNatEntryOnDemandPdn(ON_DEMAND_PDN_PROFILE_ID, NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    listRef=taf_net_GetDestNatEntryListOnDemandPdn(ON_DEMAND_PDN_PROFILE_ID);
    LE_ASSERT(listRef != NULL);
    entryRef = taf_net_GetFirstDestNatEntry(listRef);
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
        LE_ASSERT(taf_net_RemoveDestNatEntryOnDemandPdn(ON_DEMAND_PDN_PROFILE_ID, NAT_ENTRY_PRIVATE_IP_ADDR, privatePort[i], globalPort[i], ipProtoNum[i]) == LE_OK);
    LE_ASSERT(taf_net_DeleteDestNatEntryList(NULL) == LE_BAD_PARAMETER);
    LE_ASSERT(taf_net_DeleteDestNatEntryList(listRef) == LE_OK);
}
static void NetworkUnitTestFunc(){
    LE_INFO("======== 2.1. Getting interface list test ========");
    NetworkGetInterfaceListTest();
    LE_INFO("======== 2.2. Changing route test ========");
    NetworkChangeIpRouteTest();
    LE_INFO("======== 2.3. Get default gateway test ========");
    NetworkGetInterfaceDefaultGatewayTest();
    LE_INFO("======== 2.4. Get interface DNS test ========");
    NetworkGetInterfaceDnsTest();
    LE_INFO("======== 2.5. Set DNS test ========");
    NetworkSetDnsTest();
    LE_INFO("======== 2.6. Backup,set and restore default gateway test ========");
    NetworkBackupSetRestoreDefaultGatewayTest();
    return ;
}
static void* UnitTestNetThread(void* contextPtr){
    char* testType = (char *) contextPtr;
    LE_INFO("======== Test Thread of Net Service Start deviceMode=%s", testType);
    taf_net_ConnectService();
    semaphore = le_sem_Create("tafNetSem", 0);
    if(strcmp(testType, "default") == 0){
        LE_INFO("======== 1. Add Network Handlers ========");
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
        LE_INFO("======== 1.4 Destination NAT change Handler ========");
        threadRef = le_thread_Create("DestNatChangeThread", DestNatThread, NULL);
        le_thread_Start(threadRef);
        LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);
        LE_INFO("======== 2 Network unit test start========");
        NetworkUnitTestFunc();
        le_thread_Sleep(1);
        LE_INFO("======== 3 Destination NAT unit test start========");
        DestNatUnitTestFunc();
        le_thread_Sleep(1);
        LE_INFO("======== 4 Vlan unit test start========");
        VlanUnitTestFunc();
        LE_INFO("======== 5 Remove handlers========");
        le_thread_Sleep(3);
        taf_net_RemoveRouteChangeHandler(routeChangeHandlerRef);
        taf_net_RemoveGatewayChangeHandler(gatewayChangeHandlerRef);
        taf_net_RemoveDNSChangeHandler(DNSChangeHandlerRef);
        taf_net_RemoveDestNatChangeHandler(DestNatChangeHandlerRef);
    }else if(strcmp(testType, "l2tp") == 0){
        LE_INFO("======== L2tp unit test start========");
        L2tpUnitTestFunc();
    }else if(strcmp(testType, "socks") == 0){
        LE_INFO("======== Socks unit test start========");
        SocksUnitTestFunc();
    }
    taf_net_DisconnectService();
    LE_INFO("========all tests are passed========");
    exit(EXIT_SUCCESS);
}
COMPONENT_INIT
{
    const char* testType = "";
    LE_INFO("number = %d",le_arg_NumArgs());
    if(le_arg_NumArgs() != 1){
        puts("usage:app runProc tafNetUnitTest --exe=tafNetUnitTest -- default/l2tp/socks");
        return;
    }
    testType = le_arg_GetArg(0);
    LE_INFO("test type=%s ",testType);
    le_thread_Start(le_thread_Create("NetTestThread", UnitTestNetThread, (void *)testType));
}
