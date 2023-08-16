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
 * @file       tafNetIntTest.cpp
 * @brief      This file includes integration test functions of the Networking Service.
 */

#include "legato.h"
#include "interfaces.h"

#define NET_IPV4_ADDR_MAX_BYTES      16
#define NET_IPV6_ADDR_MAX_BYTES      46
#define NET_IP_PROTO_NUMBER_LEN      4
#define NET_IP_PROTO_NUMBER_TCP      6
#define NET_IP_PROTO_NUMBER_UDP      17

le_sem_Ref_t semaphore;

taf_net_RouteChangeHandlerRef_t routeChangeHandlerRef;
taf_net_GatewayChangeHandlerRef_t gatewayChangeHandlerRef;
taf_net_DNSChangeHandlerRef_t DNSChangeHandlerRef;
taf_net_DestNatChangeHandlerRef_t DestNatChangeHandlerRef;

static void PrintUsage ()
{
    puts("\n"
            "app start tafNetIntTest\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getinterfacelist\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- changeiproute \
<interfacename> <destination> <subnetmask> <metric> <1/0> \n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getinterfacegw <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getinterfacedns <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- setdefaultgw <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- setdns <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- backupsetandrestoregw \
<interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- adddestnatondefaultpdn \
<privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- deldestnatondefaultpdn \
<privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getdestnatlistondefaultpdn\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- adddestnatondemandpdn \
<profileid> <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- deldestnatondemandpdn \
<profileid> <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getdestnatlistondemandpdn \
<profileid>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- createvlan <vlanId> <type> \
<isAccelerated> [optional priority]\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- removevlan <vlanId> <type>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getvlaninterfaceinfo <vlanid>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- bindwithprofile \
<vlanid> <profileid>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- bindwithprofileex \
<vlanid> <phoneid> <profileid>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- unbindwithprofile <vlanid>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getvlanentryinfo\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- enablel2tp <enablemss> <enablemtu> \
<mtusize>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- disablel2tp\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getl2tpinfo\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- addtunnel <locId> <peerId> \
<encaproto> [<localudpport> <peerudpport>] <peerIpAddrPtr> <ifNamePtr> \
<sessionNum> [<localsessionId> <peersessionId> <localsessionId> <peersessionId> <localsessionId> \
<peersessionId>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- removetunnel <locId>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- gettunnelinfo\n"
            "\n");
}


static void NetRouteChangeHandlerFunc
(
    const taf_net_RouteChangeInd_t* routeChangeIndPtr,
    void* contextPtr
)
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

static void NetGatewayChangeHandlerFunc
(
    const taf_net_GatewayChangeInd_t* gatewayChangeIndPtr,
    void* contextPtr
)
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

static void DestNatChangeHandlerFunc
(
    const taf_net_DestNatChangeInd_t* DestNatChangeIndPtr,
    void* contextPtr
)
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

static void* HandlerThread(void* contextPtr)
{
    LE_INFO("======== Test Thread of Net Service Start ========");

    //  connect service in thread.
    taf_net_ConnectService();

    semaphore = le_sem_Create("tafNetSem", 0);

    LE_INFO("======== 1. Network Add Handlers ========");

    LE_INFO("======== 1.1 Route change Handler ========");
    le_thread_Ref_t threadRef = le_thread_Create("NetRouteTh", NetRouteThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.2 Gateway change Handler ========");
    threadRef = le_thread_Create("NetGatewayTh", NetGatewayThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.3 DNS change Handler ========");
    threadRef = le_thread_Create("DNSChangeTh", NetDNSThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    LE_INFO("======== 1.4 Nat change Handler ========");
    threadRef = le_thread_Create("DestNatChgTh", DestNatThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    return NULL;
}

static int TafNetGetInterfaceList()
{
    le_result_t result;
    taf_net_InterfaceInfo_t intfInfoListPtr[50];
    size_t listSize = 0 ;

    LE_INFO("----Start getinterfacelist test ");
    if (le_arg_NumArgs() < 1)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    result = taf_net_GetInterfaceList(intfInfoListPtr,&listSize);
    LE_INFO("----interface number=%" PRIuS ",result=%d\n",listSize,result);
    for(int i=0;i<listSize;i++)
    {
        LE_INFO("----interface name =%s,technology =%d,state =%d",intfInfoListPtr[i].interfaceName,
                intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
    }

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetChangeIpRoute()
{
    LE_INFO("----Start changeiproute test");
    le_result_t result;

    if (le_arg_NumArgs() != 6)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* intfName = le_arg_GetArg(1);
    const char* destAddr = le_arg_GetArg(2);
    const char* prefixLength = le_arg_GetArg(3);
    const char* metricPtr = le_arg_GetArg(4);
    const char* isAddPtr = le_arg_GetArg(5);

    if(intfName == NULL || destAddr == NULL || prefixLength == NULL || metricPtr == NULL ||
       isAddPtr == NULL)
    {
        LE_ERROR("ifNamePtr, destAddr, prefixLength, metricPtr or isAddPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t metric = strtol(metricPtr, NULL, 0);
    uint8_t isAdd = strtol(isAddPtr, NULL, 0);

    result=taf_net_ChangeRoute(intfName,destAddr,prefixLength,metric,isAdd);
    LE_INFO("----result =%d" ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetGetInterfaceGw()
{
    LE_INFO("----Start getinterfacegateway test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* intfName = le_arg_GetArg(1);
    char ipv4addr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6addr[NET_IPV6_ADDR_MAX_BYTES];

    memset(ipv4addr, 0 , NET_IPV4_ADDR_MAX_BYTES);
    memset(ipv6addr, 0 , NET_IPV6_ADDR_MAX_BYTES);

    if(intfName == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }

    result=taf_net_GetInterfaceGW(intfName,ipv4addr , sizeof(ipv4addr), ipv6addr, sizeof(ipv6addr));

    LE_INFO("----result =%d" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    LE_INFO("----got gateway address ipv4 is %s,ipv6 is %s",ipv4addr,ipv6addr);

    return EXIT_SUCCESS;
}

static int TafNetGetInterfaceDns()
{
    LE_INFO("----Start getinterfacedns test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* intfName = le_arg_GetArg(1);

    taf_net_DnsServerAddresses_t dnsServerAddressesPtr;

    if(intfName == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }

    result=taf_net_GetInterfaceDNS(intfName,&dnsServerAddressesPtr);

    LE_INFO("----result =%d" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    LE_INFO("----got ipv4 DNS1 is %s,DNS2 is %s",dnsServerAddressesPtr.ipv4Addr1,
                                                 dnsServerAddressesPtr.ipv4Addr2);
    LE_INFO("----got ipv6 DNS1 is %s,DNS2 is %s",dnsServerAddressesPtr.ipv6Addr1,
                                                 dnsServerAddressesPtr.ipv6Addr2);

    return EXIT_SUCCESS;
}

static int TafNetSetDns()
{
    LE_INFO("----Start set dns test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* intfName = le_arg_GetArg(1);

    if(intfName == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }

    result=taf_net_SetDNS(intfName);

    LE_INFO("----result %d" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetSetDefaultGw()
{
    LE_INFO("----Start setdefaultgateway test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* intfName = le_arg_GetArg(1);

    if(intfName == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }

    result=taf_net_SetDefaultGW(intfName);

    LE_INFO("----result=%d " ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetBackupSetAndRestoregw()
{
    LE_INFO("----Start backupsetandrestoregw test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* intfName = le_arg_GetArg(1);

    if(intfName == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }

    taf_net_BackupDefaultGW();

    result=taf_net_SetDefaultGW(intfName);

    LE_INFO("----set result =%d ",result );

    if(result !=LE_OK)
        return EXIT_FAILURE;

    taf_net_RestoreDefaultGW();

    LE_INFO("----backupsetandrestoregw result =%d ",result );
    return EXIT_SUCCESS;
}

static int TafNatAddDestNatOnDefaultPdn()
{
    LE_INFO("----Start adddestnatondefaultpdn test " );
    le_result_t result;
    uint16_t protonum=NET_IP_PROTO_NUMBER_TCP;

    if (le_arg_NumArgs() !=5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* privateIpaddr = le_arg_GetArg(1);
    const char* priPortPtr = le_arg_GetArg(2);
    const char* gblPortPtr = le_arg_GetArg(3);
    const char* ipproto = le_arg_GetArg(4);

    if(privateIpaddr == NULL || priPortPtr == NULL || gblPortPtr == NULL || ipproto == NULL)
    {
        LE_ERROR("privateIpaddr, priPortPtr, gblPortPtr or ipproto is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t priPort = strtol(priPortPtr, NULL, 0);
    uint16_t gblPort = strtol(gblPortPtr, NULL, 0);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
       strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
            strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_UDP;
    else
    {
        LE_INFO("ERROR protocol");
        exit(EXIT_FAILURE);
    }

    result=taf_net_AddDestNatEntryOnDefaultPdn(privateIpaddr,priPort,gblPort,protonum);
    LE_INFO("----add dest nat result=%d",result);

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatOnDefaultPdn()
{
    LE_INFO("----Start deldestnatondefaultpdn test " );
    le_result_t result;
    uint16_t protonum=NET_IP_PROTO_NUMBER_TCP;

    if (le_arg_NumArgs() !=5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* privateIpaddr = le_arg_GetArg(1);
    const char* priPortPtr = le_arg_GetArg(2);
    const char* gblPortPtr = le_arg_GetArg(3);
    const char* ipproto = le_arg_GetArg(4);

    if(privateIpaddr == NULL || priPortPtr == NULL || gblPortPtr == NULL || ipproto == NULL)
    {
        LE_ERROR("privateIpaddr, priPortPtr, gblPortPtr or ipproto is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t priPort = strtol(priPortPtr, NULL, 0);
    uint16_t gblPort = strtol(gblPortPtr, NULL, 0);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
       strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
            strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_UDP;
    else
    {
        LE_INFO("ERROR protocol");
        exit(EXIT_FAILURE);
    }

    result=taf_net_RemoveDestNatEntryOnDefaultPdn(privateIpaddr,priPort,gblPort,protonum);
    LE_INFO("----delete dest nat result=%d",result);

    return EXIT_SUCCESS;
}

static int TafNatGetDestNatListOnDefaultPdn()
{
    le_result_t ret;
    char ipProtoStr[NET_IP_PROTO_NUMBER_LEN];
    taf_net_DestNatEntryListRef_t listRef=taf_net_GetDestNatEntryListOnDefaultPdn();

    if(listRef !=NULL)
    {
        taf_net_DestNatEntryRef_t entryRef = taf_net_GetFirstDestNatEntry(listRef);
        while(entryRef != NULL)
        {
            char ipaddr[NET_IPV6_ADDR_MAX_BYTES];
            uint16_t priPort;
            uint16_t glbPort;
            taf_net_IpProto_t proto;
            taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES,
                                           &priPort, &glbPort, &proto);
            if(proto == NET_IP_PROTO_NUMBER_TCP)
                    le_utf8_Copy(ipProtoStr, "TCP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else if(proto == NET_IP_PROTO_NUMBER_UDP)
                    le_utf8_Copy(ipProtoStr, "UDP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else
                LE_INFO("error ip proto number");

            LE_INFO("----ipaddr=%s private port=%d, global port=%d,proto=%s",ipaddr,
                                                        priPort,glbPort,ipProtoStr);

            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }

        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----OK");
        }
        else
        {
            LE_INFO("----delete dest Nat reference list ERROR");
        }
    }

    return EXIT_SUCCESS;
}

static int TafNatAddDestNatOnDemandPdn()
{
    LE_INFO("----Start adddestnatondemandpdn test " );
    le_result_t result;
    uint16_t protonum=NET_IP_PROTO_NUMBER_TCP;

    if (le_arg_NumArgs() !=6)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* profileIdPtr = le_arg_GetArg(1);
    const char* privateIpaddr = le_arg_GetArg(2);
    const char* priPortPtr = le_arg_GetArg(3);
    const char* gblPortPtr = le_arg_GetArg(4);
    const char* ipproto = le_arg_GetArg(5);

    if(profileIdPtr == NULL || privateIpaddr == NULL || priPortPtr == NULL || gblPortPtr == NULL ||
       ipproto == NULL)
    {
        LE_ERROR("profileIdPtr, privateIpaddr, priPortPtr, gblPortPtr or ipproto is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t profileId = strtol(profileIdPtr, NULL, 0);
    uint16_t priPort = strtol(priPortPtr, NULL, 0);
    uint16_t gblPort = strtol(gblPortPtr, NULL, 0);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
       strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
            strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_UDP;
    else
    {
        LE_INFO("ERROR protocol");
        exit(EXIT_FAILURE);
    }

    result=taf_net_AddDestNatEntryOnDemandPdn(profileId,privateIpaddr,priPort,gblPort,protonum);
    LE_INFO("----add dest nat result=%d",result);

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatOnDemandPdn()
{
    LE_INFO("----Start deldestnatondemandpdn test " );
    le_result_t result;
    uint16_t protonum=NET_IP_PROTO_NUMBER_TCP;

    if (le_arg_NumArgs() !=6)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* profileIdPtr = le_arg_GetArg(1);
    const char* privateIpaddr = le_arg_GetArg(2);
    const char* priPortPtr = le_arg_GetArg(3);
    const char* gblPortPtr = le_arg_GetArg(4);
    const char* ipproto = le_arg_GetArg(5);

    if(profileIdPtr == NULL || privateIpaddr == NULL || priPortPtr == NULL || gblPortPtr == NULL ||
       ipproto == NULL)
    {
        LE_ERROR("profileIdPtr, privateIpaddr, priPortPtr, gblPortPtr or ipproto is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t profileId = strtol(profileIdPtr, NULL, 0);
    uint16_t priPort = strtol(priPortPtr, NULL, 0);
    uint16_t gblPort = strtol(gblPortPtr, NULL, 0);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
       strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 ||
            strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_UDP;
    else
    {
        LE_INFO("ERROR protocol");
        exit(EXIT_FAILURE);
    }

    result=taf_net_RemoveDestNatEntryOnDemandPdn(profileId,privateIpaddr,priPort,gblPort,protonum);
    LE_INFO("----delete dest nat result=%d",result);

    return EXIT_SUCCESS;
}

static int TafNatGetDestNatListOnDemandPdn()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    char ipProtoStr[NET_IP_PROTO_NUMBER_LEN];

    const char* profileIdPtr = le_arg_GetArg(1);

    if(profileIdPtr == NULL)
    {
        LE_ERROR("profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t profileId = strtol(profileIdPtr, NULL, 0);
    taf_net_DestNatEntryListRef_t listRef=taf_net_GetDestNatEntryListOnDemandPdn(profileId);

    if(listRef !=NULL)
    {
        taf_net_DestNatEntryRef_t entryRef = taf_net_GetFirstDestNatEntry(listRef);
        while(entryRef != NULL)
        {
            char ipaddr[NET_IPV6_ADDR_MAX_BYTES];
            uint16_t priPort;
            uint16_t glbPort;
            taf_net_IpProto_t proto;
            taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES,
                                           &priPort, &glbPort, &proto);

            if(proto == NET_IP_PROTO_NUMBER_TCP)
                    le_utf8_Copy(ipProtoStr, "TCP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else if(proto == NET_IP_PROTO_NUMBER_UDP)
                    le_utf8_Copy(ipProtoStr, "UDP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else
            {
                LE_INFO("----error ip proto number");
                continue;
            }
            LE_INFO("ipaddr=%s private port=%d, global port=%d,proto=%s",ipaddr,priPort,
                                                                         glbPort,ipProtoStr);
            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }

        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----OK");
        }
        else
        {
            LE_INFO("----delete dest Nat reference list ERROR");
        }
    }

    return EXIT_SUCCESS;
}

static int TafVlanInterfaceInfo()
{
    le_result_t ret;
    int ifType=0;
    uint8_t priority=0;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);

    if(vlanIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t vlanId = strtol(vlanIdPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanId);

    taf_net_VlanIfListRef_t listRef=taf_net_GetVlanInterfaceList(vlanRef);

    if(listRef !=NULL)
    {
        taf_net_VlanIfRef_t entryRef = taf_net_GetFirstVlanInterface(listRef);
        while(entryRef != NULL)
        {

            ifType=taf_net_GetVlanInterfaceType(entryRef);

            LE_INFO("ifType=%d",ifType);

            ret = taf_net_GetVlanPriority(entryRef, &priority);
            if(ret == LE_OK)
                LE_INFO("priority=%d",priority);
            else
                LE_INFO("Getting priority error");

            entryRef=taf_net_GetNextVlanInterface(listRef);
        }

        ret = taf_net_DeleteVlanInterfaceList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----OK");
        }
        else
        {
            LE_INFO("----delete vlan interface reference list ERROR");
        }
    }

    return EXIT_SUCCESS;
}

//when client session closed, vlanRef is removed from vlanRefMap, call 2 APIs in this command
static int TafCreateVlan()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=4 && le_arg_NumArgs() !=5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* ifTypePtr = le_arg_GetArg(2);
    const char* isAcceleratedPtr = le_arg_GetArg(3);


    if(vlanIdPtr == NULL || ifTypePtr == NULL || isAcceleratedPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, ifTypePtr, isAcceleratedPtr or priorityPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t vlanId = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    bool isAccelerated = strtol(isAcceleratedPtr, NULL, 0);
    taf_net_VlanRef_t vlanRef=taf_net_CreateVlan(vlanId,isAccelerated);

    if(le_arg_NumArgs() ==5)
    {
        const char* priorityPtr = le_arg_GetArg(4);
        if(priorityPtr == NULL)
        {
            LE_ERROR("priorityPtr is NULL");
            exit(EXIT_FAILURE);
        }

        uint8_t priority = strtol(priorityPtr, NULL, 0);
        ret = taf_net_SetVlanPriority(vlanRef, priority);

        if(ret != LE_OK)
        {
            LE_INFO("---Setting VLAN priority error");
            return EXIT_FAILURE;
        }
    }

    if(vlanRef != NULL)
    {
        ret=taf_net_AddVlanInterface(vlanRef,ifType);
        if(ret == LE_OK)
            LE_INFO("---Creating VLAN OK");
        else
            LE_INFO("---Creating VLAN error");
    }
    else
        LE_INFO("---IsAccelerated conflict with the old vlue");

    return EXIT_SUCCESS;
}

static int TafRemoveVlan()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=3)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* ifTypePtr = le_arg_GetArg(2);

    if(vlanIdPtr == NULL || ifTypePtr == NULL)
    {
        LE_ERROR("vlanIdPtr or ifTypePtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint16_t vlanId = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanId);

    ret=taf_net_RemoveVlanInterface(vlanRef,ifType);
    if(ret == LE_OK)
        LE_INFO("---Removing VLAN OK---");
    else
        LE_INFO("---Removing VLAN error---");

    return EXIT_SUCCESS;
}

static int TafVlanInfo()
{
    int vlanId=0;
    uint8_t phoneId=0;
    int profileId=0;
    le_result_t ret;
    bool isAccelerated=false;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    taf_net_VlanEntryListRef_t listRef=taf_net_GetVlanEntryList();

    if(listRef !=NULL)
    {
        taf_net_VlanEntryRef_t entryRef = taf_net_GetFirstVlanEntry(listRef);
        while(entryRef != NULL)
        {

            vlanId=taf_net_GetVlanId(entryRef);

            LE_INFO("----vlanId=%d",vlanId);

            ret =taf_net_IsVlanAccelerated(entryRef,&isAccelerated);
            if(ret == LE_OK)
            {
                LE_INFO("----isAccelerated=%d",isAccelerated);
            }


            profileId=taf_net_GetVlanBoundProfileId(entryRef);

            if(profileId == -1)
                LE_INFO("----no binding----");
            else
            {
                ret=taf_net_GetVlanBoundPhoneId(entryRef, &phoneId);
                if(ret != LE_OK)
                {
                    LE_ERROR("error binding info");
                }
                else
                {
                    LE_INFO("----phone id=%d----", phoneId);
                    LE_INFO("----profile id=%d----",profileId);
                }
            }

            entryRef=taf_net_GetNextVlanEntry(listRef);
        }

        ret = taf_net_DeleteVlanEntryList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----OK");
        }
        else
        {
            LE_INFO("----delete vlan entry reference list ERROR");
        }
    }

    return EXIT_SUCCESS;
}

static int TafVlanBindWithProfile()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=3)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* profileidPtr = le_arg_GetArg(2);

    if(vlanIdPtr == NULL || profileidPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or profileidPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint32_t profileid = strtol(profileidPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    ret = taf_net_BindVlanWithProfile(vlanRef,profileid);
    if(ret == LE_OK)
    {
        LE_INFO("----bind with profile  ok");
    }
    else
    {
        LE_INFO("----bind with profile error");
    }

    return EXIT_SUCCESS;
}

static int TafVlanBindWithProfileEx()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* phoneIdPtr = le_arg_GetArg(2);
    const char* profileIdPtr = le_arg_GetArg(3);

    if(vlanIdPtr == NULL || phoneIdPtr == NULL || profileIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, phoneIdPtr or profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint8_t phoneid = strtol(phoneIdPtr, NULL, 0);
    uint32_t profileid = strtol(profileIdPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    ret = taf_net_BindVlanWithProfileEx(vlanRef, phoneid, profileid);
    if(ret == LE_OK)
    {
        LE_INFO("----bind with profile  ok");
    }
    else
    {
        LE_INFO("----bind with profile error");
    }

    return EXIT_SUCCESS;
}

static int TafVlanUnBindWithProfile()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);

    if(vlanIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    ret = taf_net_UnbindVlanFromProfile(vlanRef);
    if(ret == LE_OK)
    {
        LE_INFO("----unbind with profile  ok");
    }
    else
    {
        LE_INFO("----unbind with profile error");
    }

    return EXIT_SUCCESS;
}

static int TafEnableL2tp()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* enablemssPtr = le_arg_GetArg(1);
    const char* enablemtuPtr = le_arg_GetArg(2);
    const char* mtusizePtr = le_arg_GetArg(3);

    if(enablemssPtr == NULL || enablemtuPtr == NULL || mtusizePtr == NULL)
    {
        LE_ERROR("enablemssPtr, enablemssPtr or mtusizePtr is NULL");
        exit(EXIT_FAILURE);
    }

    bool enablemss = strtol(enablemssPtr, NULL, 0);
    bool enablemtu = strtol(enablemtuPtr, NULL, 0);
    uint32_t mtusize = strtol(mtusizePtr, NULL, 0);

    ret=taf_net_EnableL2tp(enablemss, enablemtu, mtusize);
    LE_INFO("enablemss =%d, enablemtu=%d, mtusize=%d",enablemss, enablemtu, mtusize);
    if(ret == LE_OK)
    {
        LE_INFO("----enable l2tp ok");
    }
    else
    {
        LE_INFO("----enable l2tp error");
    }

    return EXIT_SUCCESS;
}

static int TafDisableL2tp()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    ret=taf_net_DisableL2tp();
    if(ret == LE_OK)
    {
        LE_INFO("----disable l2tp ok");
    }
    else
    {
        LE_INFO("----disable l2tp failed");
    }

    return EXIT_SUCCESS;
}

static int TafGetL2tpInfo()
{
    bool status = false;

    status=taf_net_IsL2tpEnabled();
    if(status == true)
    {
        LE_INFO("L2TP is enabled");

        if(taf_net_IsL2tpMssEnabled() == true)
            LE_INFO("L2TP Mss is enabled");
        else
            LE_INFO("L2TP Mss is disabled");

        if(taf_net_IsL2tpMtuEnabled() == true)
        {
            LE_INFO("L2TP Mtu is enabled");
            LE_INFO("Mtu size = %d",taf_net_GetL2tpMtuSize());
        }
        else
            LE_INFO("L2TP Mss is disabled");
    }
    else
        LE_INFO("L2TP is disabled");

    return EXIT_SUCCESS;
}

/**
* taf_net_CreateTunnel API returns tunnelRef which will be used by taf_net_SetTunnelUdpPort,
* taf_net_AddSession and taf_net_StartTunnel APIs, but tafNetSvc will release tunnelRef when the
* client disconnected, so we use one command to call these APIs. The following is the command:
* app runProc tafNetIntTest --exe=tafNetIntTest -- createtunnel <locId> <peerId> \
* <encaproto> [<localudpport> <peerudpport>] <peerIpAddrPtr> <ifNamePtr> \
* <sessionNum> [<localsessionId> <peersessionId> <localsessionId> <peersessionId> <localsessionId>
* <peersessionId>
* if encaproto is udp ,and the next 2 parameters must be localudpport and peerudpport
* and the sessionNum specify the next local session id and peer session id number.
* e.g: app runProc tafNetIntTest --exe=tafNetIntTest -- createtunnel 1 1 1 fd53:7cb8:383:5::2
* eth0.5 2 3 3 4 4, this means encapsulation protocol is ip, session number is 2, and local session
* id and remote id are 3,3, and 4,4
*/
static int TafCreateL2tpTunnel()
{
    int paramIndex=1, i = 0;
    le_result_t ret = LE_OK;
    uint32_t localtunnelId = 0, peertunnelId=0, encaproto = 0, localudpport = 0, peerudpport = 0;
    const char* peerIpAddrPtr = NULL;
    const char* ifNamePtr = NULL;
    taf_net_TunnelRef_t tunnelRef = NULL;
    uint32_t localsessionId[3];
    uint32_t peersessionId[3];
    uint32_t sessionNum=0;

    if( (le_arg_NumArgs() < 9) || (le_arg_NumArgs() > 15) )
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    //Get local tunnel id
    const char* localtunnelIdPtr = le_arg_GetArg(paramIndex);

    if(localtunnelIdPtr == NULL)
    {
        LE_ERROR("localtunnelIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    localtunnelId = strtol(localtunnelIdPtr, NULL, 0);//paramIndex=1
    paramIndex++;
    //Get peer tunnel id
    const char* peertunnelIdPtr = le_arg_GetArg(paramIndex);

    if(peertunnelIdPtr == NULL)
    {
        LE_ERROR("peertunnelIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    peertunnelId = strtol(peertunnelIdPtr, NULL, 0);//paramIndex=2
    paramIndex++;
    //Get encapsulation protocol
    const char* encaprotoPtr = le_arg_GetArg(paramIndex);

    if(encaprotoPtr == NULL)
    {
        LE_ERROR("encaprotoPtr is NULL");
        exit(EXIT_FAILURE);
    }
    encaproto = strtol(encaprotoPtr, NULL, 0);//paramIndex=3
    paramIndex++;

    if(encaproto == (int)TAF_NET_L2TP_UDP)
    {
        //Get local udp port
        const char* localudpportPtr = le_arg_GetArg(paramIndex);

        if(localudpportPtr == NULL)
        {
            LE_ERROR("localudpportPtr is NULL");
            exit(EXIT_FAILURE);
        }

        localudpport = strtol(localudpportPtr, NULL, 0);//paramIndex=4
        paramIndex++;
        //Get peer udp port
        const char* peerudpportPtr = le_arg_GetArg(paramIndex);

        if(peerudpportPtr == NULL)
        {
            LE_ERROR("peerudpportPtr is NULL");
            exit(EXIT_FAILURE);
        }
        peerudpport = strtol(peerudpportPtr, NULL, 0);//paramIndex=5
        paramIndex++;

        //Check if udp port is valid
        if(localudpport == 0 || peerudpport == 0)
        {
            puts("----Udp port is error");
            exit(EXIT_FAILURE);
        }
        //Check param number
        if (le_arg_NumArgs() < 11)
        {
            puts("----Encapsulation protocol is udp, need set udp port param");
            exit(EXIT_FAILURE);
        }
    }

    //Get peer ip address
    peerIpAddrPtr = le_arg_GetArg(paramIndex);//paramIndex=4 or paramIndex=6
    if(peerIpAddrPtr == NULL)
    {
        LE_ERROR("peerIpAddrPtr is NULL");
        exit(EXIT_FAILURE);
    }
    paramIndex++;

    //Get interface name
    ifNamePtr = le_arg_GetArg(paramIndex);//paramIndex=5 or paramIndex=7
    if(ifNamePtr == NULL)
    {
        LE_ERROR("ifNamePtr is NULL");
        exit(EXIT_FAILURE);
    }
    paramIndex++;

    //Get session number
    const char* sessionNumPtr = le_arg_GetArg(paramIndex);//paramIndex=6 or paramIndex=8
    if(sessionNumPtr == NULL)
    {
        LE_ERROR("sessionNumPtr is NULL");
        exit(EXIT_FAILURE);
    }
    sessionNum = strtol(sessionNumPtr, NULL, 0);
    paramIndex++;

    //Check if the session number is valid
    if(sessionNum <= 0 || sessionNum > 3 )
    {
        puts("----SessionNum is invalid");
        exit(EXIT_FAILURE);
    }

    //Check if the param number is valid
    if(encaproto == (int)TAF_NET_L2TP_UDP && le_arg_NumArgs() != 9+ sessionNum*2 )
    {
        puts("----Session parameter is invalid");
        exit(EXIT_FAILURE);
    }
    else if(encaproto == (int)TAF_NET_L2TP_IP && le_arg_NumArgs() != 7+ sessionNum*2 )
    {
        puts("----Session parameter is invalid");
        exit(EXIT_FAILURE);
    }

    //Get local session ids and peer session ids
    for(i=0;i<sessionNum;i++)
    {
        localsessionId[i] = strtol(le_arg_GetArg(paramIndex), NULL, 0);
        paramIndex++;
        peersessionId[i] = strtol(le_arg_GetArg(paramIndex), NULL, 0);
        paramIndex++;
    }

    //Create tunnel
    tunnelRef=taf_net_CreateTunnel((taf_net_L2tpEncapProtocol_t) encaproto, localtunnelId,
                                   peertunnelId, peerIpAddrPtr, ifNamePtr);

    if(tunnelRef == NULL)
    {
        puts("----Failed to create tunnel reference ");
        exit(EXIT_FAILURE);
    }

    //Set udp port if needed
    if(encaproto == TAF_NET_L2TP_UDP)
        ret=taf_net_SetTunnelUdpPort(tunnelRef, localudpport, peerudpport);

    if(ret != LE_OK)
    {
        puts("----Failed to set tunnel udp port");
        taf_net_RemoveTunnel(tunnelRef);
        exit(EXIT_FAILURE);
    }

    //Add session into tunnel
    for(i=0;i<sessionNum;i++)
    {
        ret = taf_net_AddSession(tunnelRef, localsessionId[i], peersessionId[i]);
        if(ret != LE_OK)
            break;
    }

    if(ret != LE_OK)
    {
        puts("----Failed to add session");
        //Restore session number
        sessionNum = 0;
        taf_net_RemoveTunnel(tunnelRef);
        exit(EXIT_FAILURE);
    }

    //Start tunnel
    ret=taf_net_StartTunnel(tunnelRef);
    if(ret != LE_OK)
    {
        puts("----Failed to start tunnel");
        for(i=0;i<sessionNum;i++)
            taf_net_RemoveSession(tunnelRef, localsessionId[i], peersessionId[i]);

        //Restore session number
        sessionNum = 0;
        taf_net_RemoveTunnel(tunnelRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Add tunnel successfully--");
    return EXIT_SUCCESS;
}

//Just stop tunnel since session id is not known, and the reference will be released by closehandler
static int TafRemoveL2tpTunnel()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* localtunnelIdPtr = le_arg_GetArg(1);

    if(localtunnelIdPtr == NULL)
    {
        LE_ERROR("localtunnelIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t localtunnelId = strtol(localtunnelIdPtr, NULL, 0);

    taf_net_TunnelRef_t tunnelRef= taf_net_GetTunnelRefById(localtunnelId);

    if(tunnelRef == NULL)
    {
        puts("----Failed to get tunnel reference ");
        exit(EXIT_FAILURE);
    }

    ret=taf_net_StopTunnel(tunnelRef);
    if(ret != LE_OK)
    {
        puts("----Failed to stop l2tp tunnel ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Remove tunnel successfully--");
    return EXIT_SUCCESS;
}

static int TafGetTunnelInfo()
{
    le_result_t ret;
    taf_net_TunnelEntryListRef_t listRef=taf_net_GetTunnelEntryList();
    uint32_t localTunnelId=0;
    uint32_t peerTunnelId=0;

    uint32_t localUdpPort=0;
    uint32_t peerUdpPort=0;
    taf_net_L2tpEncapProtocol_t l2tpEncaproto;
    char ipv4addr[NET_IPV4_ADDR_MAX_BYTES];
    char ipv6addr[NET_IPV6_ADDR_MAX_BYTES];
    char interfacename[TAF_NET_INTERFACE_NAME_MAX_LEN];
    taf_net_IpFamilyType_t ipType;
    taf_net_L2tpSessionConfig_t sessionConfig[3];
    size_t sessionNum=0;

    if(listRef !=NULL)
    {
        taf_net_TunnelEntryRef_t entryRef = taf_net_GetFirstTunnelEntry(listRef);
        while(entryRef != NULL)
        {
            localTunnelId=taf_net_GetTunnelLocalId(entryRef);
            LE_INFO("----tunnel info start----");
            LE_INFO("----localTunnelId=%d",localTunnelId);

            peerTunnelId=taf_net_GetTunnelPeerId(entryRef);

            LE_INFO("----peerTunnelId=%d",peerTunnelId);

            l2tpEncaproto=taf_net_GetTunnelEncapProto(entryRef);
            if(l2tpEncaproto == TAF_NET_L2TP_UDP)
            {
                LE_INFO("----encapsulation protocol is UDP");
                localUdpPort=taf_net_GetTunnelLocalUdpPort(entryRef);
                LE_INFO("----local udp port is %d",localUdpPort);
                peerUdpPort=taf_net_GetTunnelPeerUdpPort(entryRef);
                LE_INFO("----peer udp port is %d",peerUdpPort);
            }
            else if(l2tpEncaproto == TAF_NET_L2TP_IP)
                LE_INFO("----encapsulation protocol is IP");

            ipType=taf_net_GetTunnelIpType(entryRef);
            if(ipType == TAF_NET_L2TP_IPV4)
            {
                LE_INFO("----IP version is IPV4");
                ret=taf_net_GetTunnelPeerIpv4Addr(entryRef, ipv4addr,
                                                           NET_IPV4_ADDR_MAX_BYTES);
                if(ret == LE_OK)
                    LE_INFO("----ipv4 address is %s",ipv4addr);
            }
            else if(ipType == TAF_NET_L2TP_IPV6)
            {
                LE_INFO("----IP version is IPV6");
                ret=taf_net_GetTunnelPeerIpv6Addr(entryRef, ipv6addr,
                                                           NET_IPV6_ADDR_MAX_BYTES);
                if(ret == LE_OK)
                    LE_INFO("----ipv6 address is %s",ipv6addr);
            }

            ret=taf_net_GetTunnelInterfaceName(entryRef, interfacename,
                                                       TAF_NET_INTERFACE_NAME_MAX_LEN);
            if(ret == LE_OK)
                LE_INFO("----interfacename is %s",interfacename);

            sessionNum = 3;
            ret=taf_net_GetSessionConfig(entryRef, sessionConfig, &sessionNum);
            if(ret == LE_OK)
            {

                for(int i=0;i<sessionNum;i++)
                {
                    LE_INFO("----session %d local id is %d", i, sessionConfig[i].locId);
                    LE_INFO("----session %d peer id is %d",i, sessionConfig[i].peerId);
                }
            }
            LE_INFO("----session num = %" PRIuS,sessionNum);
            entryRef=taf_net_GetNextTunnelEntry(listRef);
        }

        taf_net_DeleteTunnelEntryList(listRef);

    }

    return EXIT_SUCCESS;
}

#if 0
static int TafSetSocksAuthType()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    uint32_t authtype = strtol(le_arg_GetArg(1), NULL, 0);

    ret = taf_net_SetSocksAuthMethod((taf_net_AuthMethod_t) authtype);

    if(ret != LE_OK)
    {
        puts("----Failed to set socks auth type ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Set Socks auth type successfully--");

    return EXIT_SUCCESS;
}

static int TafGetSocksAuthType()
{
    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    taf_net_AuthMethod_t authtype = taf_net_GetSocksAuthMethod();

    LE_INFO("--Get Socks auth type successfully value=%d", authtype);

    return EXIT_SUCCESS;
}

static int TafSetSocksLanIfName()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* ifName = le_arg_GetArg(1);

    ret = taf_net_SetSocksLanInterface(ifName);

    if(ret != LE_OK)
    {
        puts("----Failed to set socks LAN interface ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Set Socks LAN interface successfully--");

    return EXIT_SUCCESS;
}

static int TafGetSocksLanIfName()
{
    le_result_t ret;
    char lanIfName[255];
    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }


    ret = taf_net_GetSocksLanInterface(lanIfName,255);

    if(ret != LE_OK)
    {
        puts("----Failed to Get socks LAN interface ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Get Socks LAN interface successfully--%s", lanIfName);

    return EXIT_SUCCESS;
}

static int TafAddSocksAssociation()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=3)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* userName = le_arg_GetArg(1);
    uint32_t profileId = strtol(le_arg_GetArg(2), NULL, 0);

    ret = taf_net_AddSocksAssociation(userName, profileId);

    if(ret != LE_OK)
    {
        puts("----Failed to add socks association ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Add socks association successfully--");

    return EXIT_SUCCESS;
}

static int TafDelSocksAssociation()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* userName = le_arg_GetArg(1);

    ret = taf_net_RemoveSocksAssociation(userName);

    if(ret != LE_OK)
    {
        puts("----Failed to delete socks association ");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Delete socks association successfully--");

    return EXIT_SUCCESS;
}

static int TafEnableSocks()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    ret = taf_net_EnableSocks();

    if(ret != LE_OK)
    {
        puts("----Failed to enable socks");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Enable socks successfully--");

    return EXIT_SUCCESS;
}

static int TafDisableSocks()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    ret = taf_net_DisableSocks();

    if(ret != LE_OK)
    {
        puts("----Failed to disable socks");
        exit(EXIT_FAILURE);
    }

    LE_INFO("--Disable socks successfully--");

    return EXIT_SUCCESS;
}

static int TafAddGsb()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    const char *ifName=le_arg_GetArg(1);
    taf_net_GsbIfType_t intfType = (taf_net_GsbIfType_t)strtol(le_arg_GetArg(2), NULL, 0);
    uint32_t bandwidth = strtol(le_arg_GetArg(3), NULL, 0);

    ret = taf_net_AddGsb(ifName, intfType, bandwidth);
    if(ret == LE_OK)
    {
        LE_INFO("----add gsb  ok");
    }
    else
    {
        LE_INFO("----add gsb error");
    }

    return EXIT_SUCCESS;
}

static int TafRemoveGsb()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    const char *ifName=le_arg_GetArg(1);

    ret = taf_net_RemoveGsb(ifName);
    if(ret == LE_OK)
    {
        LE_INFO("----remove gsb  ok");
    }
    else
    {
        LE_INFO("----remove gsb error");
    }

    return EXIT_SUCCESS;
}

static int TafEnableGsb()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    ret = taf_net_EnableGsb();
    if(ret == LE_OK)
    {
        LE_INFO("----enable gsb  ok");
    }
    else
    {
        LE_INFO("----enable gsb error");
    }

    return EXIT_SUCCESS;
}

static int TafDisableGsb()
{
    le_result_t ret;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    ret = taf_net_DisableGsb();
    if(ret == LE_OK)
    {
        LE_INFO("----disable gsb  ok");
    }
    else
    {
        LE_INFO("----disable gsb error");
    }

    return EXIT_SUCCESS;
}

static int TafGetGsbInfo()
{
    le_result_t ret;
    char intfName[TAF_NET_INTERFACE_NAME_MAX_LEN];
    taf_net_GsbIfType_t intfType;
    uint32_t bandwidth;

    if (le_arg_NumArgs() !=1)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    taf_net_GsbListRef_t listRef=taf_net_GetGsbList();

    if(listRef !=NULL)
    {
        taf_net_GsbRef_t gsbRef = taf_net_GetFirstGsb(listRef);
        while(gsbRef != NULL)
        {
            ret = taf_net_GetGsbInterfaceName(gsbRef,intfName,TAF_NET_INTERFACE_NAME_MAX_LEN);
            if(ret == LE_OK)
            {
                LE_INFO("----intfName=%s",intfName);
            }

            intfType = taf_net_GetGsbInterfaceType(gsbRef);

            LE_INFO("----intfType=%d", (int)intfType);

            bandwidth = taf_net_GetGsbBandWidth(gsbRef);

            LE_INFO("----bandwidth=%d(Mbps)", bandwidth);

            gsbRef=taf_net_GetNextGsb(listRef);
        }

        ret = taf_net_DeleteGsbList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----OK");
        }
        else
        {
            LE_INFO("----delete gsb reference list ERROR");
        }
    }

    return EXIT_SUCCESS;
}

#endif

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    const char* testType = "";
    if (le_arg_NumArgs() == 0 )
    {
        PrintUsage();
        le_thread_Sleep(2);
        LE_INFO("===register handler started===");
        le_thread_Start(le_thread_Create("NetTestThread", HandlerThread, NULL));
    }
    if (le_arg_NumArgs() >= 1)
    {
        testType = le_arg_GetArg(0);
        LE_INFO("arg0=%s ",testType);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }

        if(strcmp(testType, "getinterfacelist") ==0)
        {
            status=TafNetGetInterfaceList();
        }
        else if(strcmp(testType, "changeiproute") == 0)
        {
          status=TafNetChangeIpRoute();
        }
        else if(strcmp(testType, "getinterfacegw") == 0)
        {
          status=TafNetGetInterfaceGw();
        }
        else if(strcmp(testType, "getinterfacedns") == 0)
        {
          status=TafNetGetInterfaceDns();
        }
        else if(strcmp(testType, "setdns") == 0)
        {
          status=TafNetSetDns();
        }
        else if(strcmp(testType, "setdefaultgw") == 0)
        {
          status=TafNetSetDefaultGw();
        }
        else if(strcmp(testType, "backupsetandrestoregw") == 0)
        {
          status=TafNetBackupSetAndRestoregw();
        }
        else if(strcmp(testType, "adddestnatondefaultpdn") == 0)
        {
            status=TafNatAddDestNatOnDefaultPdn();
        }
        else if(strcmp(testType, "deldestnatondefaultpdn") == 0)
        {
            status=TafNatDelDestNatOnDefaultPdn();
        }
        else if(strcmp(testType, "getdestnatlistondefaultpdn") == 0)
        {
            status=TafNatGetDestNatListOnDefaultPdn();
        }
        else if(strcmp(testType, "adddestnatondemandpdn") == 0)
        {
            status=TafNatAddDestNatOnDemandPdn();
        }
        else if(strcmp(testType, "deldestnatondemandpdn") == 0)
        {
            status=TafNatDelDestNatOnDemandPdn();
        }
        else if(strcmp(testType, "getdestnatlistondemandpdn") == 0)
        {
            status=TafNatGetDestNatListOnDemandPdn();
        }
        else if(strcmp(testType, "createvlan") == 0)
        {
            status=TafCreateVlan();
        }
        else if(strcmp(testType, "removevlan") == 0)
        {
            status=TafRemoveVlan();
        }
        else if(strcmp(testType, "getvlanentryinfo") == 0)
        {
            status=TafVlanInfo();
        }
        else if(strcmp(testType, "getvlaninterfaceinfo") == 0)
        {
            status=TafVlanInterfaceInfo();
        }
        else if(strcmp(testType, "bindwithprofile") == 0)
        {
            status=TafVlanBindWithProfile();
        }
        else if(strcmp(testType, "bindwithprofileex") == 0)
        {
            status=TafVlanBindWithProfileEx();
        }
        else if(strcmp(testType, "unbindwithprofile") == 0)
        {
            status=TafVlanUnBindWithProfile();
        }
        else if(strcmp(testType, "enablel2tp") == 0)
        {
            status=TafEnableL2tp();
        }
        else if(strcmp(testType, "disablel2tp") == 0)
        {
            status=TafDisableL2tp();
        }
        else if(strcmp(testType, "getl2tpinfo") == 0)
        {
            status=TafGetL2tpInfo();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "addtunnel") == 0)
        {
            status=TafCreateL2tpTunnel();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "removetunnel") == 0)
        {
            status=TafRemoveL2tpTunnel();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "gettunnelinfo") == 0)
        {
            status=TafGetTunnelInfo();
            LE_INFO("status =%d",status);
        }
#if 0
        else if(strcmp(testType, "setsocksauthtype") == 0)
        {
            status=TafSetSocksAuthType();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "getsocksauthtype") == 0)
        {
            status=TafGetSocksAuthType();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "setsockslanifname") == 0)
        {
            status=TafSetSocksLanIfName();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "getsockslanifname") == 0)
        {
            status=TafGetSocksLanIfName();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "addsocksassociation") == 0)
        {
            status=TafAddSocksAssociation();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "deletesocksassociation") == 0)
        {
            status=TafDelSocksAssociation();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "enablesocks") == 0)
        {
            status=TafEnableSocks();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "disablesocks") == 0)
        {
            status=TafDisableSocks();
            LE_INFO("status =%d",status);
        }
        else if(strcmp(testType, "addgsb") == 0)
        {
            status=TafAddGsb();
        }
        else if(strcmp(testType, "removegsb") == 0)
        {
            status=TafRemoveGsb();
        }
        else if(strcmp(testType, "enablegsb") == 0)
        {
            status=TafEnableGsb(true);
        }
        else if(strcmp(testType, "disablegsb") == 0)
        {
            status=TafDisableGsb(false);
        }
        else if(strcmp(testType, "getgsbinfo") == 0)
        {
            status=TafGetGsbInfo();
        }
#endif
        exit(status);
    }
}
