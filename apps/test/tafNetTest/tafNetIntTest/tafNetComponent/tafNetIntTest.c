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
            "app runProc tafNetIntTest --exe=tafNetIntTest -- changeiproute <interfacename> <destination> <subnetmask> <metric> <1/0> \n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getinterfacegw <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getinterfacedns <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- setdefaultgw <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- setdns <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- backupsetandrestoregw <interfacename>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- adddestnatondefaultpdn <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- deldestnatondefaultpdn <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- getdestnatlistondefaultpdn\n"
            "app runProc tafNetIntTest --exe=tafNetIntTest -- deldestnatlistrefondefaultpdn\n"
            "\n");
}


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
static void* HandlerThread(void* contextPtr)
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
    LE_INFO("----interface number=%d,result=%d\n",listSize,result);
    for(int i=0;i<listSize;i++)
    {
        LE_INFO("----interface name =%s,technology =%d,state =%d",intfInfoListPtr[i].interfaceName,intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
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
    uint16_t metric = strtol(le_arg_GetArg(4), NULL, 0);
    uint8_t isAdd = strtol(le_arg_GetArg(5), NULL, 0);

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

    result=taf_net_GetInterfaceDNS(intfName,&dnsServerAddressesPtr);

    LE_INFO("----result =%d" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    LE_INFO("----got ipv4 DNS1 is %s,DNS2 is %s",dnsServerAddressesPtr.ipv4Addr1,dnsServerAddressesPtr.ipv4Addr2);
    LE_INFO("----got ipv6 DNS1 is %s,DNS2 is %s",dnsServerAddressesPtr.ipv6Addr1,dnsServerAddressesPtr.ipv6Addr2);

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
    uint16_t priPort = strtol(le_arg_GetArg(2), NULL, 0);
    uint16_t gblPort = strtol(le_arg_GetArg(3), NULL, 0);

    const char* ipproto = le_arg_GetArg(4);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 || strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
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
    uint16_t priPort = strtol(le_arg_GetArg(2), NULL, 0);
    uint16_t gblPort = strtol(le_arg_GetArg(3), NULL, 0);

    const char* ipproto = le_arg_GetArg(4);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 || strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
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
            taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, &proto);
            if(proto == NET_IP_PROTO_NUMBER_TCP)
                    le_utf8_Copy(ipProtoStr, "TCP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else if(proto == NET_IP_PROTO_NUMBER_UDP)
                    le_utf8_Copy(ipProtoStr, "UDP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else
                LE_INFO("error ip proto number");

            LE_INFO("----ipaddr=%s private port=%d, global port=%d,proto=%s",ipaddr,priPort,glbPort,ipProtoStr);

            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }
    }

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatListRefOnDefaultPdn()
{
    le_result_t ret;
    taf_net_DestNatEntryListRef_t listRef=taf_net_GetDestNatEntryListOnDefaultPdn();
    if(listRef !=NULL)
    {
        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----delete ok");
        }
        else
        {
            LE_INFO("----delete error");
        }
    }
    else
    {
        LE_INFO("----No list exist");
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

    uint32_t profileId = strtol(le_arg_GetArg(1), NULL, 0);
    const char* privateIpaddr = le_arg_GetArg(2);
    uint16_t priPort = strtol(le_arg_GetArg(3), NULL, 0);
    uint16_t gblPort = strtol(le_arg_GetArg(4), NULL, 0);

    const char* ipproto = le_arg_GetArg(5);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 || strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
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

    uint32_t profileId = strtol(le_arg_GetArg(1), NULL, 0);
    const char* privateIpaddr = le_arg_GetArg(2);
    uint16_t priPort = strtol(le_arg_GetArg(3), NULL, 0);
    uint16_t gblPort = strtol(le_arg_GetArg(4), NULL, 0);
    const char* ipproto = le_arg_GetArg(5);

    if(strncmp(ipproto,"tcp",NET_IP_PROTO_NUMBER_LEN) ==0 ||strncmp(ipproto,"TCP",NET_IP_PROTO_NUMBER_LEN) ==0)
        protonum = NET_IP_PROTO_NUMBER_TCP;
    else if(strncmp(ipproto,"udp",NET_IP_PROTO_NUMBER_LEN) ==0 || strncmp(ipproto,"UDP",NET_IP_PROTO_NUMBER_LEN) ==0)
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
    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    char ipProtoStr[NET_IP_PROTO_NUMBER_LEN];
    uint32_t profileId = strtol(le_arg_GetArg(1), NULL, 0);
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
            taf_net_GetDestNatEntryDetails(entryRef, ipaddr, NET_IPV6_ADDR_MAX_BYTES, &priPort, &glbPort, &proto);

            if(proto == NET_IP_PROTO_NUMBER_TCP)
                    le_utf8_Copy(ipProtoStr, "TCP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else if(proto == NET_IP_PROTO_NUMBER_UDP)
                    le_utf8_Copy(ipProtoStr, "UDP", NET_IP_PROTO_NUMBER_LEN, NULL);
            else
            {
                LE_INFO("----error ip proto number");
                continue;
            }
            LE_INFO("ipaddr=%s private port=%d, global port=%d,proto=%s",ipaddr,priPort,glbPort,ipProtoStr);
            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }

    }

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatListRefOnDemandPdn()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    uint32_t profileId = strtol(le_arg_GetArg(1), NULL, 0);
    taf_net_DestNatEntryListRef_t listRef=taf_net_GetDestNatEntryListOnDemandPdn(profileId);
    if(listRef !=NULL)
    {
        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            LE_INFO("----delete ok");
        }
        else
        {
            LE_INFO("----delete error");
        }
    }

    return EXIT_SUCCESS;
}

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
        else if(strcmp(testType, "deldestnatlistrefondefaultpdn") == 0)
        {
            status=TafNatDelDestNatListRefOnDefaultPdn();
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
        else if(strcmp(testType, "deldestnatlistrefondemandpdn") == 0)
        {
            status=TafNatDelDestNatListRefOnDemandPdn();
        }
        exit(status);
    }
}




