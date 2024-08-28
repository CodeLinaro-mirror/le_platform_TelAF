/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define NET_IPV4_ADDR_MAX_BYTES      16
#define NET_IPV6_ADDR_MAX_BYTES      46
#define NET_IP_PROTO_NUMBER_LEN      4
#define NET_IP_PROTO_NUMBER_TCP      6
#define NET_IP_PROTO_NUMBER_UDP      17

#define TAF_CONFIG_SSIM_TEST
#define TAF_CONFIG_PHONE_ID_1_TEST
//#define TAF_CONFIG_PHONE_ID_2_TEST

//For SSIM
#ifdef TAF_CONFIG_SSIM_TEST
#define SSIM_TEST 1
#else
#define SSIM_TEST 0
#endif

//For DSSA and DSDA phone id 1
#ifdef TAF_CONFIG_PHONE_ID_1_TEST
#define PHONE_ID_1_TEST 1
#else
#define PHONE_ID_1_TEST 0
#endif

//For DSDA phone id 2
#ifdef TAF_CONFIG_PHONE_ID_2_TEST
#define PHONE_ID_2_TEST 1
#else
#define PHONE_ID_2_TEST 0
#endif

#define TEST_PROFILE         1 // profile id 1, used for phoneid 2
#define TEST_PROFILE_FIFTH   5 // profile id 5, used for phoneid 1

#define PHONE_ID_1     1
#define PHONE_ID_2     2

static bool isRpcNetConnected;
static bool isRpcDcsConnected;

static le_sem_Ref_t TestSemRef;
le_thread_Ref_t dataSessionThRef = NULL;
static taf_dcs_SessionStateHandlerRef_t TestSessionStateRef = NULL;
static uint32_t remoteDataProfileid;
char ApnStr_bak[TAF_DCS_APN_NAME_MAX_LEN];

char ifaddress[TAF_NET_IP_ADDR_MAX_LEN];
char gwaddress[TAF_NET_IP_ADDR_MAX_LEN];
char pDNSaddress[TAF_NET_IP_ADDR_MAX_LEN];
char sDNSaddress[TAF_NET_IP_ADDR_MAX_LEN];
uint32_t ifsubnetMask;


le_sem_Ref_t semaphore;

taf_net_RouteChangeHandlerRef_t routeChangeHandlerRef;
taf_net_GatewayChangeHandlerRef_t gatewayChangeHandlerRef;
taf_net_DNSChangeHandlerRef_t DNSChangeHandlerRef;
taf_net_DestNatChangeHandlerRef_t DestNatChangeHandlerRef;

static void PrintUsage ()
{
    puts("\n"
            "app start tafDualNadTest\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getinterfacelist\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- listen\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- changeiproute \
<interfacename> <destination> <subnetmask> <metric> <1/0> \n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getinterfacegw <interfacename>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getinterfacedns <interfacename>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setdefaultgw <interfacename>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setdns <interfacename>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- backupsetandrestoregw \
<interfacename>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- adddestnatondefaultpdn \
<privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- deldestnatondefaultpdn \
<privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getdestnatlistondefaultpdn\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- adddestnatondemandpdn \
<profileid> <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- deldestnatondemandpdn \
<profileid> <privateipaddr> <privateport> <globalport> <tcp/udp>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getdestnatlistondemandpdn \
<profileid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- createvlan <vlanId> <Interface type> \
<isAccelerated> <NetworkType> [optional priority]\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- removevlan <vlanId> <Interface type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpccreatevlan <Remote vlanId> <Remote Interface type> \
<Remote isAccelerated> <Remote NetworkType> [optional Remote priority]\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcremovevlan <Remote vlanId> <Remote Interface type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getvlaninterfaceinfo <vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- bindwithprofile \
<vlanid> <profileid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- bindwithprofileex \
<vlanid> <phoneid> <profileid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- unbindwithprofile <vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithprofile \
<Remote vlanid> <Remote profileid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithprofileex \
<Remote vlanid> <Remote phoneid> <Remote profileid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcunbindwithprofile <Remote vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- bindwithBackhaul \
<vlanid> <backhaulVlanId> <BackhaulType>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- bindwithBackhaulex \
<vlanid> <phoneid> <backhaulVlanId> <BackhaulType>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- unbindwithBackhaul <vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithBackhaul \
<Remote vlanid> <Remote profile Id> <Remote BackhaulType>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithBackhaulex \
<Remote vlanid> <Remote phoneid> <Remote profile id> <Remote BackhaulType>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcunbindwithBackhaul <Remote vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setIPPTOperation \
<vlanid> <operation type> <interface type> <mac address>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcsetIPPTOperation \
<Remote vlanid> <Remote operation type> <Remote interface type> <Remote mac address>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getIPPTConfig <vlan id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcgetIPPTConfig <Remote vlan id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setIPConfig <vlan id> <ifType> <ip type> <ip Opr> <assign type> <ifAddr> <gwAddr> <PriDnsAddr> <SecDnsAddr> <ifmask>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getIPConfig <vlan id> <ifType> <ip type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getvlanentryinfo\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcdatacall <Remote profile id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- datacall <profile id>\n"
            "\n");
}

void PrintCurrentTime() {
  char buffer[26];
  int millisec;
  struct tm* tm_info;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  millisec = lrint(tv.tv_usec/1000.0); // Round to the nearest millisec
  if (millisec>=1000) {
    millisec -=1000;
  }

  tm_info = localtime(&tv.tv_sec);

  strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
  printf("\n\033[1;35m%s.%03d\033[0m ", buffer, millisec);
}

static void NetRouteChangeHandlerFunc
(
    const taf_net_RouteChangeInd_t* routeChangeIndPtr,
    void* contextPtr
)
{
    printf("**** Handler for route Change Indication (Begin)****\n");
    printf("----interface name: %s\n", routeChangeIndPtr->interfaceName);
    printf("----destination address: %s\n", routeChangeIndPtr->destAddr);
    printf("----subnetmask: %s\n", routeChangeIndPtr->prefixLength);
    printf("----metric: %d\n", routeChangeIndPtr->metric);
    printf("----action: %d\n", routeChangeIndPtr->action);

    printf("**** Handler for route Change Indication (End)****\n");
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
    printf("**** Handler for gateway change Indication (Begin)****\n");
    printf("----interface name: %s\n", gatewayChangeIndPtr->interfaceName);
    printf("----destination address: %s\n", gatewayChangeIndPtr->gatewayAddr);
    printf("----ip type: %d\n", gatewayChangeIndPtr->ipType);

    printf("**** Handler for gateway change Indication (End)****\n");
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
    printf("**** Handler for DNS Change Indication (Begin)****\n");
    printf("----ip addr1: %s\n", DNSChangeIndPtr->ipAddr1);
    printf("----ip addr2: %s\n", DNSChangeIndPtr->ipAddr2);
    printf("----ip type: %d\n", DNSChangeIndPtr->ipType);

    printf("**** Handler for DNS Change Indication (End)****\n");
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
    printf("**** Handler for Destination Nat Change Indication (Begin)****\n");
    printf("----profileId: %d\n", DestNatChangeIndPtr->profileId);
    printf("----action: %d\n", DestNatChangeIndPtr->action);

    printf("**** Handler for Destination Nat Change Indication (End)****\n");
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
    printf("======== Test Thread of Net Service Start ========\n");

    //  connect service in thread.
    taf_net_ConnectService();

    semaphore = le_sem_Create("tafNetSem", 0);

    printf("======== 1. Network Add Handlers ========\n");

    printf("======== 1.1 Route change Handler ========\n");
    le_thread_Ref_t threadRef = le_thread_Create("NetRouteTh", NetRouteThread, NULL);
    le_thread_Start(threadRef);
    le_clk_Time_t timeToWait = {5, 0};
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    printf("======== 1.2 Gateway change Handler ========\n");
    threadRef = le_thread_Create("NetGatewayTh", NetGatewayThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    printf("======== 1.3 DNS change Handler ========\n");
    threadRef = le_thread_Create("DNSChangeTh", NetDNSThread, NULL);
    le_thread_Start(threadRef);
    LE_ASSERT(le_sem_WaitWithTimeOut(semaphore, timeToWait) == LE_OK);

    printf("======== 1.4 Nat change Handler ========\n");
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

    printf("----Start getinterfacelist test \n");
    if (le_arg_NumArgs() < 1)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    result = taf_net_GetInterfaceList(intfInfoListPtr,&listSize);
    printf("----interface number=%" PRIuS ",result=%d\n",listSize,result);
    for(int i=0;i<listSize;i++)
    {
        printf("----interface name =%s,technology =%d,state =%d\n",intfInfoListPtr[i].interfaceName,
                intfInfoListPtr[i].tech,intfInfoListPtr[i].state);
    }

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetChangeIpRoute()
{
    printf("----Start changeiproute test\n");
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
    printf("----result =%d\n" ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetGetInterfaceGw()
{
    printf("----Start getinterfacegateway test \n" );
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

    printf("----result =%d\n" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    printf("----got gateway address ipv4 is %s,ipv6 is %s\n",ipv4addr,ipv6addr);

    return EXIT_SUCCESS;
}

static int TafNetGetInterfaceDns()
{
    printf("----Start getinterfacedns test \n" );
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

    printf("----result =%d" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    printf("----got ipv4 DNS1 is %s,DNS2 is %s\n",dnsServerAddressesPtr.ipv4Addr1,
                                                 dnsServerAddressesPtr.ipv4Addr2);
    printf("----got ipv6 DNS1 is %s,DNS2 is %s\n",dnsServerAddressesPtr.ipv6Addr1,
                                                 dnsServerAddressesPtr.ipv6Addr2);

    return EXIT_SUCCESS;
}

static int TafNetSetDns()
{
    printf("----Start set dns test \n" );
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

    printf("----result %d\n" ,result);
    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetSetDefaultGw()
{
    printf("----Start setdefaultgateway test \n" );
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

    printf("----result=%d \n" ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int TafNetBackupSetAndRestoregw()
{
    printf("----Start backupsetandrestoregw test \n" );
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

    printf("----set result =%d \n",result );

    if(result !=LE_OK)
        return EXIT_FAILURE;

    taf_net_RestoreDefaultGW();

    printf("----backupsetandrestoregw result =%d \n",result );
    return EXIT_SUCCESS;
}

static int TafNatAddDestNatOnDefaultPdn()
{
    printf("----Start adddestnatondefaultpdn test \n" );
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
        printf("ERROR protocol\n");
        exit(EXIT_FAILURE);
    }

    result=taf_net_AddDestNatEntryOnDefaultPdn(privateIpaddr,priPort,gblPort,protonum);
    printf("----add dest nat result=%d\n",result);

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatOnDefaultPdn()
{
    printf("----Start deldestnatondefaultpdn test \n" );
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
        printf("ERROR protocol\n");
        exit(EXIT_FAILURE);
    }

    result=taf_net_RemoveDestNatEntryOnDefaultPdn(privateIpaddr,priPort,gblPort,protonum);
    printf("----delete dest nat result=%d\n",result);

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
                printf("error ip proto number\n");

            printf("----ipaddr=%s private port=%d, global port=%d,proto=%s\n",ipaddr,
                                                        priPort,glbPort,ipProtoStr);

            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }

        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            printf("----OK\n");
        }
        else
        {
            printf("----delete dest Nat reference list ERROR\n");
        }
    }

    return EXIT_SUCCESS;
}

static int TafNatAddDestNatOnDemandPdn()
{
    printf("----Start adddestnatondemandpdn test \n" );
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
        printf("ERROR protocol\n");
        exit(EXIT_FAILURE);
    }

    result=taf_net_AddDestNatEntryOnDemandPdn(profileId,privateIpaddr,priPort,gblPort,protonum);
    printf("----add dest nat result=%d\n",result);

    return EXIT_SUCCESS;
}

static int TafNatDelDestNatOnDemandPdn()
{
    printf("----Start deldestnatondemandpdn test \n" );
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
        printf("ERROR protocol\n");
        exit(EXIT_FAILURE);
    }

    result=taf_net_RemoveDestNatEntryOnDemandPdn(profileId,privateIpaddr,priPort,gblPort,protonum);
    printf("----delete dest nat result=%d\n",result);

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
                printf("----error ip proto number\n");
                continue;
            }
            printf("ipaddr=%s private port=%d, global port=%d,proto=%s\n",ipaddr,priPort,
                                                                         glbPort,ipProtoStr);
            entryRef=taf_net_GetNextDestNatEntry(listRef);
        }

        ret = taf_net_DeleteDestNatEntryList(listRef);
        if(ret == LE_OK)
        {
            printf("----OK\n");
        }
        else
        {
            printf("----delete dest Nat reference list ERROR\n");
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

            printf("ifType=%d\n",ifType);

            ret = taf_net_GetVlanPriority(entryRef, &priority);
            if(ret == LE_OK)
                printf("priority=%d",priority);
            else
                printf("Getting priority error\n");

            entryRef=taf_net_GetNextVlanInterface(listRef);
        }

        ret = taf_net_DeleteVlanInterfaceList(listRef);
        if(ret == LE_OK)
        {
            printf("----OK\n");
        }
        else
        {
            printf("----delete vlan interface reference list ERROR\n");
        }
    }

    return EXIT_SUCCESS;
}

//when client session closed, vlanRef is removed from vlanRefMap, call 2 APIs in this command
static int TafCreateVlan()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=5 && le_arg_NumArgs() !=6)
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

    const char* nwPtr = le_arg_GetArg(4);
    uint32_t networkType = strtol(nwPtr, NULL, 0);

    ret = taf_netIpPass_SetVlanNetworkType(vlanRef, (taf_netIpPass_NetworkType_t) networkType);

    if(le_arg_NumArgs() == 6)
    {
        const char* priorityPtr = le_arg_GetArg(5);
        if(priorityPtr == NULL)
        {
            LE_ERROR("priorityPtr is NULL");
            exit(EXIT_FAILURE);
        }

        uint8_t priority = strtol(priorityPtr, NULL, 0);
        ret = taf_net_SetVlanPriority(vlanRef, priority);

        if(ret != LE_OK)
        {
            printf("---Setting VLAN priority error\n");
            return EXIT_FAILURE;
        }
    }

    if(vlanRef != NULL)
    {
        ret=taf_net_AddVlanInterface(vlanRef,ifType);
        if(ret == LE_OK)
            printf("---Creating VLAN OK\n");
        else
            printf("---Creating VLAN error: %s\n", LE_RESULT_TXT(ret));
    }
    else
        printf("---IsAccelerated conflict with the old value\n");

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
        printf("---Removing VLAN OK---\n");
    else
        printf("---Removing VLAN error: %s\n", LE_RESULT_TXT(ret));

    return EXIT_SUCCESS;
}

static int RpcTafCreateVlan()
{
    le_result_t ret;
    if (le_arg_NumArgs() !=5 && le_arg_NumArgs() !=6)
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
    taf_net_VlanRef_t vlanRef=rpc_taf_net_CreateVlan(vlanId,isAccelerated);

    const char* nwPtr = le_arg_GetArg(4);
    uint32_t networkType = strtol(nwPtr, NULL, 0);

    ret = rpc_taf_netIpPass_SetVlanNetworkType(vlanRef, (taf_netIpPass_NetworkType_t) networkType);

    if(le_arg_NumArgs() == 6)
    {
        const char* priorityPtr = le_arg_GetArg(5);
        if(priorityPtr == NULL)
        {
            LE_ERROR("priorityPtr is NULL");
            exit(EXIT_FAILURE);
        }

        uint8_t priority = strtol(priorityPtr, NULL, 0);
        ret = rpc_taf_net_SetVlanPriority(vlanRef, priority);

        if(ret != LE_OK)
        {
            printf("---Setting Remote VLAN priority error\n");
            return EXIT_FAILURE;
        }
    }

    if(vlanRef != NULL)
    {
        ret=rpc_taf_net_AddVlanInterface(vlanRef,ifType);
        if(ret == LE_OK)
            printf("---Creating Remote VLAN OK\n");
        else
            printf("---Creating Remote VLAN error: %s\n", LE_RESULT_TXT(ret));
    }
    else
        printf("---Remote IsAccelerated conflict with the old value\n");

    return EXIT_SUCCESS;
}

static int RpcTafRemoveVlan()
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

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanId);

    ret=rpc_taf_net_RemoveVlanInterface(vlanRef,ifType);
    if(ret == LE_OK)
        printf("---Removing Remote VLAN OK---\n");
    else
        printf("---Removing Remote VLAN error---\n");

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

            printf("----vlanId=%d\n",vlanId);

            ret =taf_net_IsVlanAccelerated(entryRef,&isAccelerated);
            if(ret == LE_OK)
            {
                printf("----isAccelerated=%d\n",isAccelerated);
            }


            profileId=taf_net_GetVlanBoundProfileId(entryRef);

            if(profileId == -1)
                printf("----no binding----\n");
            else
            {
                ret=taf_net_GetVlanBoundPhoneId(entryRef, &phoneId);
                if(ret != LE_OK)
                {
                    LE_ERROR("error binding info");
                }
                else
                {
                    printf("----phone id=%d----\n", phoneId);
                    printf("----profile id=%d----\n",profileId);
                }
            }

            entryRef=taf_net_GetNextVlanEntry(listRef);
        }

        ret = taf_net_DeleteVlanEntryList(listRef);
        if(ret == LE_OK)
        {
            printf("----OK\n");
        }
        else
        {
            printf("----delete vlan entry reference list ERROR\n");
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
        printf("----bind with profile  ok\n");
    }
    else
    {
        printf("----bind with profile error: %s\n", LE_RESULT_TXT(ret));
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
        printf("----bind with profile  ok\n");
    }
    else
    {
        printf("----bind with profile error: %s\n", LE_RESULT_TXT(ret));
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
        printf("----unbind with profile  ok\n");
    }
    else
    {
        printf("----unbind with profile error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int TafVlanBindWithBackhaul()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* backhaulVlanIdPtr = le_arg_GetArg(2);
    const char* bhPtr = le_arg_GetArg(3);

    if(vlanIdPtr == NULL || backhaulVlanIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or profileidPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint32_t backhaulVlanId = strtol(backhaulVlanIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    ret = taf_netIpPass_SetVlanBackhaulVlanId(vlanRef, backhaulVlanId);

    ret = taf_netIpPass_SetVlanBackhaulType(vlanRef, (taf_netIpPass_BackhaulType_t) backHaulType);

    ret = taf_netIpPass_BindVlanWithBackhaul(vlanRef);

    if(ret == LE_OK)
    {
        printf("----bind vlan with backhaul OK\n");
    }
    else
    {
        printf("----bind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int TafVlanBindWithBackhaulEx()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* phoneIdPtr = le_arg_GetArg(2);
    const char* backhaulVlanIdPtr = le_arg_GetArg(3);
    const char* bhPtr = le_arg_GetArg(4);

    if(vlanIdPtr == NULL || phoneIdPtr == NULL || backhaulVlanIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, phoneIdPtr or profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint8_t phoneid = strtol(phoneIdPtr, NULL, 0);
    uint32_t backhaulVlanId = strtol(backhaulVlanIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    ret = taf_netIpPass_SetVlanBackhaulVlanId(vlanRef, backhaulVlanId);

    ret = taf_netIpPass_SetVlanBackhaulPhoneId(vlanRef, phoneid);
    ret = taf_netIpPass_SetVlanBackhaulType(vlanRef, (taf_netIpPass_BackhaulType_t) backHaulType);

    ret = taf_netIpPass_BindVlanWithBackhaul(vlanRef);
    if(ret == LE_OK)
    {
        printf("----bind vlan with backhaul OK\n");
    }
    else
    {
        printf("----bind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int TafVlanUnBindWithBackhaul()
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

    ret = taf_netIpPass_UnbindVlanFromBackhaul(vlanRef);
    if(ret == LE_OK)
    {
        printf("----unbind vlan with backhaul OK\n");
    }
    else
    {
        printf("----unbind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanBindWithBackhaul()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* backhaulProfileIdPtr = le_arg_GetArg(2);
    const char* bhPtr = le_arg_GetArg(3);

    if(vlanIdPtr == NULL || backhaulProfileIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or profileidPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint32_t backhaulProfileId = strtol(backhaulProfileIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_netIpPass_SetVlanBackhaulProfileId(vlanRef, backhaulProfileId);
    ret = rpc_taf_netIpPass_SetVlanBackhaulType(vlanRef, (taf_netIpPass_BackhaulType_t) backHaulType);

    ret = rpc_taf_netIpPass_BindVlanWithBackhaul(vlanRef);

    if(ret == LE_OK)
    {
        printf("----rpc bind vlan with backhaul OK\n");
    }
    else
    {
        printf("----rpc bind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanBindWithBackhaulEx()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* phoneIdPtr = le_arg_GetArg(2);
    const char* backhaulProfileIdPtr = le_arg_GetArg(3);
    const char* bhPtr = le_arg_GetArg(4);

    if(vlanIdPtr == NULL || phoneIdPtr == NULL || backhaulProfileIdPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, phoneIdPtr or profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint8_t phoneid = strtol(phoneIdPtr, NULL, 0);
    uint32_t backhaulProfileId = strtol(backhaulProfileIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_netIpPass_SetVlanBackhaulProfileId(vlanRef, backhaulProfileId);
    ret = rpc_taf_netIpPass_SetVlanBackhaulPhoneId(vlanRef, phoneid);
    ret = rpc_taf_netIpPass_SetVlanBackhaulType(vlanRef, (taf_netIpPass_BackhaulType_t) backHaulType);

    ret = rpc_taf_netIpPass_BindVlanWithBackhaul(vlanRef);
    if(ret == LE_OK)
    {
        printf("----rpc bind vlan with backhaul OK\n");
    }
    else
    {
        printf("----rpc bind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanUnBindWithBackhaul()
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

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_netIpPass_UnbindVlanFromBackhaul(vlanRef);
    if(ret == LE_OK)
    {
        printf("----rpc unbind vlan with backhaul OK\n");
    }
    else
    {
        printf("----rpc unbind vlan with backhaul error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanBindWithProfile()
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

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_net_BindVlanWithProfile(vlanRef,profileid);
    if(ret == LE_OK)
    {
        printf("----rpc bind with profile  ok\n");
    }
    else
    {
        printf("----rpc bind with profile error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanBindWithProfileEx()
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

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_net_BindVlanWithProfileEx(vlanRef, phoneid, profileid);
    if(ret == LE_OK)
    {
        printf("----rpc bind with profile  ok\n");
    }
    else
    {
        printf("----rpc bind with profile error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafVlanUnBindWithProfile()
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

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    ret = rpc_taf_net_UnbindVlanFromProfile(vlanRef);
    if(ret == LE_OK)
    {
        printf("----rpc unbind with profile  ok\n");
    }
    else
    {
        printf("----rpc unbind with profile error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafSetIPPTOperation()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* operationPtr = le_arg_GetArg(2);
    const char* ifTypePtr = le_arg_GetArg(3);
    const char* macAddrPtr = le_arg_GetArg(4);

    if(vlanIdPtr == NULL || operationPtr == NULL || ifTypePtr == NULL || macAddrPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or profileidPtr or ifTypePtr or macAddrPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    uint32_t operation = strtol(operationPtr, NULL, 0);

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = rpc_taf_netIpPass_GetInterface(ifType);

    ret = rpc_taf_netIpPass_SetIPPTOperation(ipptInterfaceRef, (taf_netIpPass_Operation_t) operation);
    ret = rpc_taf_netIpPass_SetIPPTDeviceMacAddress(ipptInterfaceRef, ifType, macAddrPtr);
    ret = rpc_taf_netIpPass_SetIPPassThroughConfig(ipptInterfaceRef, vlanid);

    if(ret == LE_OK)
    {
        printf("----rpc SetIPPTOperation ok\n");
    }
    else
    {
        printf("----rpc SetIPPTOperation error: %s\n", LE_RESULT_TXT(ret));
    }

    return EXIT_SUCCESS;
}

static int RpcTafGetIPPTConfig()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 2)
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

    taf_netIpPass_Operation_t operation = TAF_NETIPPASS_IPPT_UNKNOWN;
    taf_net_VlanIfType_t ifType = TAF_NET_IFACE_UNKNOWN;
    char macAddr[TAF_NET_MAC_ADDR_MAX_LEN] = "";

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = rpc_taf_netIpPass_GetIPPassThroughConfig(vlanid);

    ret = rpc_taf_netIpPass_GetIPPTOperation(ipptInterfaceRef, &operation);
    printf("----rpc GetIPPTOperation %s and Operation: %d\n", ret == LE_OK ? "success" : "failed", (int) operation);
    ret = rpc_taf_netIpPass_GetIPPTDeviceMacAddress(ipptInterfaceRef, &ifType, macAddr, TAF_NET_MAC_ADDR_MAX_LEN);
    printf("----rpc GetIPPTDeviceMacAddress %s, ifType: %d and macAddr: %s\n", ret == LE_OK ? "success" : "failed", (int) ifType, macAddr);

    return EXIT_SUCCESS;
}

static int TafSetIPPTOperation()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* operationPtr = le_arg_GetArg(2);
    const char* ifTypePtr = le_arg_GetArg(3);
    const char* macAddrPtr = le_arg_GetArg(4);

    if(vlanIdPtr == NULL || operationPtr == NULL || ifTypePtr == NULL || macAddrPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or profileidPtr or ifTypePtr or macAddrPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    uint32_t operation = strtol(operationPtr, NULL, 0);

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = taf_netIpPass_GetInterface(ifType);

    ret = taf_netIpPass_SetIPPTOperation(ipptInterfaceRef, (taf_netIpPass_Operation_t) operation);
    ret = taf_netIpPass_SetIPPTDeviceMacAddress(ipptInterfaceRef, ifType, macAddrPtr);
    ret = taf_netIpPass_SetIPPassThroughConfig(ipptInterfaceRef, vlanid);

    if(ret == LE_OK)
    {
        printf("--- SetIPPTOperation ok\n");
    }
    else
    {
        printf("---- SetIPPTOperation error: %s\n", LE_RESULT_TXT(ret));
    }
    taf_netIpPass_RemoveInterface(ipptInterfaceRef);
    return EXIT_SUCCESS;
}

static int TafGetIPPTConfig()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 2)
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

    taf_netIpPass_Operation_t operation = TAF_NETIPPASS_IPPT_UNKNOWN;
    taf_net_VlanIfType_t ifType = TAF_NET_IFACE_UNKNOWN;
    char macAddr[TAF_NET_MAC_ADDR_MAX_LEN] = "";

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = taf_netIpPass_GetIPPassThroughConfig(vlanid);

    ret = taf_netIpPass_GetIPPTOperation(ipptInterfaceRef, &operation);
    printf("---- GetIPPTOperation %s and Operation: %d\n", ret == LE_OK ? "success" : "failed", (int) operation);
    ret = taf_netIpPass_GetIPPTDeviceMacAddress(ipptInterfaceRef, &ifType, macAddr, TAF_NET_MAC_ADDR_MAX_LEN);
    printf("---- GetIPPTDeviceMacAddress %s, ifType: %d and macAddr: %s\n", ret == LE_OK ? "success" : "failed", (int) ifType, macAddr);

    taf_netIpPass_RemoveInterface(ipptInterfaceRef);
    return EXIT_SUCCESS;
}

static int TafSetIPConfig()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 11)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* ifTypePtr = le_arg_GetArg(2);
    const char* ipTypePtr = le_arg_GetArg(3);
    const char* ipOprPtr = le_arg_GetArg(4);
    const char* ipAssignTypePtr = le_arg_GetArg(5);
    const char* interfaceAddrPtr = le_arg_GetArg(6);
    const char* gwAddrPtr = le_arg_GetArg(7);
    const char* primaryDnsAddrPtr = le_arg_GetArg(8);
    const char* secondaryDnsAddrPtr = le_arg_GetArg(9);
    const char* ifMaskAddrPtr = le_arg_GetArg(10);

    if(vlanIdPtr == NULL || ipOprPtr == NULL || ipAssignTypePtr == NULL || interfaceAddrPtr == NULL)
    {
        LE_ERROR("vlanIdPtr or ipOprPtr or ipAssignTypePtr or interfaceAddrPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    taf_net_NetIpType_t  ipType = (taf_net_NetIpType_t)strtol(ipTypePtr, NULL, 0);
    taf_netIpPass_IpAssignOperation_t ipOpr = (taf_netIpPass_IpAssignOperation_t)strtol(ipOprPtr, NULL, 0);
    taf_netIpPass_IpAssignType_t ipAssignType = (taf_netIpPass_IpAssignType_t)strtol(ipAssignTypePtr, NULL, 0);

    taf_netIpPass_IpAddressInfo_t ipAddrInfo;
    le_utf8_Copy(ipAddrInfo.interfaceAddress, interfaceAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(ipAddrInfo.gwAddress, gwAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(ipAddrInfo.primaryDnsAddress, primaryDnsAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(ipAddrInfo.secondaryDnsAddress, secondaryDnsAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    ipAddrInfo.interfaceMask = (uint32_t)strtol(ifMaskAddrPtr, NULL, 0);

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = taf_netIpPass_GetInterface(ifType);

    ret = taf_netIpPass_SetIPConfigParams(ipptInterfaceRef, ipOpr, ipAssignType);
    if(ipAssignType == TAF_NETIPPASS_STATIC_IP)
    {
      ret = taf_netIpPass_SetIPConfigAddressParams(ipptInterfaceRef, &ipAddrInfo);
    }
    ret = taf_netIpPass_SetIPConfig(ipptInterfaceRef, ipType, ifType, vlanid);

    if(ret == LE_OK)
    {
        printf("---- SetIPConfig ok\n");
    }
    else
    {
        printf("---- SetIPConfig error: %s\n", LE_RESULT_TXT(ret));
    }
    taf_netIpPass_RemoveInterface(ipptInterfaceRef);
    return EXIT_SUCCESS;
}

static int TafGetIPConfig()
{
    le_result_t ret;

    if (le_arg_NumArgs() != 4)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* ifTypePtr = le_arg_GetArg(2);
    const char* ipTypePtr = le_arg_GetArg(3);

    if(vlanIdPtr == NULL || ipTypePtr == NULL || ifTypePtr == NULL)
    {
        LE_ERROR("vlanIdPtr or ipTypePtr or ifTypePtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    taf_net_NetIpType_t ipType = (taf_net_NetIpType_t)strtol(ipTypePtr, NULL, 0);

    taf_netIpPass_IpAssignOperation_t ipOpr = TAF_NETIPPASS_IP_UNKNOWN;
    taf_netIpPass_IpAssignType_t ipTypeOut = TAF_NETIPPASS_UNKNOWN_IP;

    taf_netIpPass_IpAddressInfo_t ipAddrInfo;

    taf_netIpPass_InterfaceRef_t ipptInterfaceRef = taf_netIpPass_GetIPConfig(ipType, ifType, vlanid);

    ret = taf_netIpPass_GetIPConfigParams(ipptInterfaceRef, &ipOpr, &ipTypeOut);
    printf("---- GetIPConfigParams %s, ipOpr: %d and ipType: %d\n", ret == LE_OK ? "success" : "failed", (int) ipOpr, (int) ipType);

    ret = taf_netIpPass_GetIPConfigAddressParams(ipptInterfaceRef, &ipAddrInfo);

    if(ret == LE_OK)
    {
        printf("---- TafGetIPConfig ok\n");
        printf("---- ipv4 ifaddr is %s,gw is %s\n",ipAddrInfo.interfaceAddress,
                                                 ipAddrInfo.gwAddress);
        printf("---- ipv4 DNS1 is %s,DNS2 is %s\n",ipAddrInfo.primaryDnsAddress,
                                                 ipAddrInfo.secondaryDnsAddress);
        printf("---- ipv4 Mask=%d\n",ipAddrInfo.interfaceMask);
    }
    else
    {
        printf("---- TafGetIPConfig error: %s\n", LE_RESULT_TXT(ret));
    }
    taf_netIpPass_RemoveInterface(ipptInterfaceRef);
    return EXIT_SUCCESS;
}

static char *callEventToString(taf_dcs_ConState_t callEvent)
{
    switch (callEvent)
    {
        case TAF_DCS_DISCONNECTED:
            return "disconnected";
        case TAF_DCS_CONNECTING:
            return "connecting";
        case TAF_DCS_CONNECTED:
            return "connected";
        case TAF_DCS_DISCONNECTING:
            return "disconnecting";
        default:
            LE_ERROR("unknown status: %d", callEvent);
            return "unknow status";
    }
    return "unknow status";
}

void rpc_data_event_handler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;
    uint32_t profileId=0;
    uint8_t phoneId;
    uint32_t subNetMask=0;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];
    le_result_t result = LE_OK;

    rpc_taf_dcs_ConnectService();

    PrintCurrentTime();

    printf("Remote data event: profile ref: %p, callEvent: %s, ipType: %d, expected ipType: %d\n",
             profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

    if (callEvent == TAF_DCS_CONNECTED)
    {
        rpc_taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);

        result = rpc_taf_dcs_GetProfileIdByInterfaceName(interfaceName, &profileId);
        LE_TEST_OK(result == LE_OK, "Connected: profile(%d), ifname(%s)", profileId, interfaceName);
        printf("Connected: profile(%d), ifname(%s)\n", profileId, interfaceName);

        result=rpc_taf_dcs_GetIPv4Address(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address - LE_OK");
        printf("IPv4 Addr: %s\n", ipAddr0);

        le_utf8_Copy(ifaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4GatewayAddress(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Gateway: %s\n", ipAddr0);

        le_utf8_Copy(gwaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4DNSAddresses(profileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
        printf("IPv4 Dns0: %s, Dns1: %s\n", ipAddr0, ipAddr1);

        le_utf8_Copy(pDNSaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(sDNSaddress,ipAddr1, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4SubnetMask(profileRef, &subNetMask);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Subnet Mask: %d\n", subNetMask);

        ifsubnetMask = subNetMask;

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = rpc_taf_dcs_GetPhoneIdByInterfaceName(interfaceName, &phoneId);
        LE_TEST_OK(result == LE_OK, "Connected: phoneId(%d), ifname(%s)", phoneId, interfaceName);
        printf("Connected: phoneId(%d), ifname(%s)\n", phoneId, interfaceName);

        LE_TEST_END_SKIP();
    }
    else if (callEvent == TAF_DCS_DISCONNECTED)
    {
        profileId = rpc_taf_dcs_GetProfileIndex(profileRef);
        LE_TEST_OK(profileId == TEST_PROFILE_FIFTH, "Disconnected: profile(%d)", profileId);

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = rpc_taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);
        printf("Disconnected: phoneId(%d)\n", phoneId);

        LE_TEST_END_SKIP();

        le_sem_Post(TestSemRef);
    }

}

void data_event_handler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;
    uint32_t profileId=0;
    uint8_t phoneId;
    uint32_t subNetMask=0;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];
    le_result_t result = LE_OK;

    taf_dcs_ConnectService();

    PrintCurrentTime();

    printf("Data event: profile ref: %p, callEvent: %s, ipType: %d, expected ipType: %d\n",
             profileRef, callEventToString(callEvent), infoPtr->ipType, expectIpType);

    if (callEvent == TAF_DCS_CONNECTED)
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);

        result = taf_dcs_GetProfileIdByInterfaceName(interfaceName, &profileId);
        LE_TEST_OK(result == LE_OK, "Connected: profile(%d), ifname(%s)", profileId, interfaceName);
        printf("Connected: profile(%d), ifname(%s)\n", profileId, interfaceName);

        result= taf_dcs_GetIPv4Address(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address - LE_OK");
        printf("IPv4 Addr: %s\n", ipAddr0);

        le_utf8_Copy(ifaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4GatewayAddress(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Gateway: %s\n", ipAddr0);

        le_utf8_Copy(gwaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4DNSAddresses(profileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
        printf("IPv4 Dns0: %s, Dns1: %s\n", ipAddr0, ipAddr1);

        le_utf8_Copy(pDNSaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(sDNSaddress,ipAddr1, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4SubnetMask(profileRef, &subNetMask);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Subnet Mask: %d\n", subNetMask);

        ifsubnetMask = subNetMask;

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = taf_dcs_GetPhoneIdByInterfaceName(interfaceName, &phoneId);
        LE_TEST_OK(result == LE_OK, "Connected: phoneId(%d), ifname(%s)", phoneId, interfaceName);
        printf("Connected: phoneId(%d), ifname(%s)\n", phoneId, interfaceName);

        LE_TEST_END_SKIP();
    }
    else if (callEvent == TAF_DCS_DISCONNECTED)
    {
        profileId = taf_dcs_GetProfileIndex(profileRef);
        LE_TEST_OK(profileId == TEST_PROFILE_FIFTH, "Disconnected: profile(%d)", profileId);

        LE_TEST_BEGIN_SKIP(!PHONE_ID_1_TEST, 1);

        result = taf_dcs_GetPhoneId(profileRef, &phoneId);
        LE_TEST_OK(result == LE_OK, "Disconnected: phoneId(%d)", phoneId);
        printf("Disconnected: phoneId(%d)\n", phoneId);

        LE_TEST_END_SKIP();

        le_sem_Post(TestSemRef);
    }

}


void rpc_start_session_sync_test()
{
    le_result_t result;
    taf_dcs_ConState_t state;
    taf_dcs_DataBearerTechnology_t upTech, downTech;

    taf_dcs_Pdp_t pdpGet;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = rpc_taf_dcs_SetPDP(rpc_taf_dcs_GetProfile(remoteDataProfileid), TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP UNKNOWN- OK");

    pdpGet = rpc_taf_dcs_GetPDP(rpc_taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_UNKNOWN, "taf_dcs_GetPDP UNKNOWN- OK");

    result = rpc_taf_dcs_SetPDP(rpc_taf_dcs_GetProfile(remoteDataProfileid), TAF_DCS_PDP_IPV4);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4- OK");

    pdpGet = rpc_taf_dcs_GetPDP(rpc_taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV4, "taf_dcs_GetPDP IPV4- OK");

    LE_TEST_END_SKIP();

    //////apn check
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    char *testApnStr = "";
    taf_dcs_ApnType_t apnType;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = rpc_taf_dcs_GetAPN(rpc_taf_dcs_GetProfile(remoteDataProfileid), ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");

    result = rpc_taf_dcs_SetAPN(rpc_taf_dcs_GetProfile(remoteDataProfileid), testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    result = rpc_taf_dcs_GetAPN(rpc_taf_dcs_GetProfile(remoteDataProfileid), apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");
    int cmpVal = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check apn value - OK");

    result = rpc_taf_dcs_GetApnTypes(rpc_taf_dcs_GetProfile(remoteDataProfileid), &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - OK");

    result = rpc_taf_dcs_SetAPN(rpc_taf_dcs_GetProfile(remoteDataProfileid), ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    LE_TEST_END_SKIP();

    /////// data session start

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = rpc_taf_dcs_GetSessionState(rpc_taf_dcs_GetProfile(remoteDataProfileid), &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_DISCONNECTED, "taf_dcs_GetSessionState - OK");

    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - session start");
    result = rpc_taf_dcs_StartSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession - OK");

    result = rpc_taf_dcs_StartSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - DUPLICATE");

    result = rpc_taf_dcs_GetSessionState(rpc_taf_dcs_GetProfile(remoteDataProfileid), &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_CONNECTED, "taf_dcs_GetSessionState - OK");

    result = rpc_taf_dcs_GetDataBearerTechnology(rpc_taf_dcs_GetProfile(remoteDataProfileid), &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology - OK");

    LE_TEST_END_SKIP();

    // data stop

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = rpc_taf_dcs_StopSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession - OK");

    LE_TEST_END_SKIP();
}

void start_session_sync_test()
{
    le_result_t result;
    taf_dcs_ConState_t state;
    taf_dcs_DataBearerTechnology_t upTech, downTech;

    taf_dcs_Pdp_t pdpGet;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_SetPDP(taf_dcs_GetProfile(remoteDataProfileid), TAF_DCS_PDP_UNKNOWN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP UNKNOWN- OK");

    pdpGet = taf_dcs_GetPDP(taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_UNKNOWN, "taf_dcs_GetPDP UNKNOWN- OK");

    result = taf_dcs_SetPDP(taf_dcs_GetProfile(remoteDataProfileid), TAF_DCS_PDP_IPV4);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetPDP IPV4- OK");

    pdpGet = taf_dcs_GetPDP(taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(pdpGet == TAF_DCS_PDP_IPV4, "taf_dcs_GetPDP IPV4- OK");

    LE_TEST_END_SKIP();

    //////apn check
    char apnStr[TAF_DCS_APN_NAME_MAX_LEN];
    char *testApnStr = "";
    taf_dcs_ApnType_t apnType;

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_GetAPN(taf_dcs_GetProfile(remoteDataProfileid), ApnStr_bak, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");

    result = taf_dcs_SetAPN(taf_dcs_GetProfile(remoteDataProfileid), testApnStr);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    result = taf_dcs_GetAPN(taf_dcs_GetProfile(remoteDataProfileid), apnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetAPN - OK");
    int cmpVal = strncmp(apnStr, testApnStr, TAF_DCS_APN_NAME_MAX_LEN);
    LE_TEST_OK(cmpVal == 0, "Check apn value - OK");

    result = taf_dcs_GetApnTypes(taf_dcs_GetProfile(remoteDataProfileid), &apnType);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetApnTypes - OK");

    result = taf_dcs_SetAPN(taf_dcs_GetProfile(remoteDataProfileid), ApnStr_bak);
    LE_TEST_OK(result == LE_OK, "taf_dcs_SetAPN - OK");

    LE_TEST_END_SKIP();

    /////// data session start

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_GetSessionState(taf_dcs_GetProfile(remoteDataProfileid), &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_DISCONNECTED, "taf_dcs_GetSessionState - OK");

    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - session start");
    result = taf_dcs_StartSession(taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_OK, "taf_dcs_StartSession - OK");

    result = taf_dcs_StartSession(taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_DUPLICATE, "taf_dcs_StartSession - DUPLICATE");

    result = taf_dcs_GetSessionState(taf_dcs_GetProfile(remoteDataProfileid), &state);
    LE_TEST_OK(result == LE_OK && state == TAF_DCS_CONNECTED, "taf_dcs_GetSessionState - OK");

    result = taf_dcs_GetDataBearerTechnology(taf_dcs_GetProfile(remoteDataProfileid), &downTech, &upTech);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetDataBearerTechnology - OK");

    LE_TEST_END_SKIP();

    // data stop

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);

    result = taf_dcs_StopSession(taf_dcs_GetProfile(remoteDataProfileid));
    LE_TEST_OK(result == LE_OK, "taf_dcs_StopSession - OK");

    LE_TEST_END_SKIP();
}


void rpc_ipv4_datacall_test()
{
    rpc_start_session_sync_test();
}

void ipv4_datacall_test()
{
    LE_INFO("Data ipv4_datacall_test start");
    start_session_sync_test();
    LE_INFO("Data ipv4_datacall_test end");
}

static void* rpc_taf_data_session_handler(void* ctxPtr)
{
    rpc_taf_dcs_ConnectService();

    LE_INFO("Test RPC start session handler added");
    TestSessionStateRef = rpc_taf_dcs_AddSessionStateHandler(rpc_taf_dcs_GetProfile(remoteDataProfileid), (taf_dcs_SessionStateHandlerFunc_t)rpc_data_event_handler, ctxPtr);

    LE_TEST_OK(TestSessionStateRef != NULL, "rpc_taf_data_session_handler - void");

    le_event_RunLoop();

    return NULL;
}

static void* taf_data_session_handler(void* ctxPtr)
{
    taf_dcs_ConnectService();

    LE_INFO("Test start session handler added");
    TestSessionStateRef = taf_dcs_AddSessionStateHandler(taf_dcs_GetProfile(remoteDataProfileid), (taf_dcs_SessionStateHandlerFunc_t)data_event_handler, ctxPtr);

    LE_TEST_OK(TestSessionStateRef != NULL, "taf_data_session_handler - void");

    le_event_RunLoop();

    return NULL;
}

static int RpcDataCallTest(void* contextPtr)
{
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4V6;

    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    TestSemRef = le_sem_Create("dcs_test_sem", 0);

    const char* profileIdPtr = le_arg_GetArg(1);
    remoteDataProfileid = strtol(profileIdPtr, NULL, 0);

    dataSessionThRef = le_thread_Create("dataSessionTh",
                                                         rpc_taf_data_session_handler, &ipType);

    le_thread_Start(dataSessionThRef);

    LE_INFO("Test RPC datacall start");

    rpc_ipv4_datacall_test();

    le_sem_Wait(TestSemRef);

    LE_INFO("Test RPC datacall end");

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);
    LE_INFO("---remove session %p",TestSessionStateRef);
    rpc_taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);
    LE_TEST_OK(le_thread_Cancel(dataSessionThRef) == LE_OK, "le_thread_Cancel session - OK");

    LE_TEST_END_SKIP();

    LE_INFO("---remove session for RPC %p",rpc_taf_dcs_GetProfile(remoteDataProfileid));
    rpc_taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);

    LE_INFO("====all tests are passed");

    le_sem_Delete(TestSemRef);

    return EXIT_SUCCESS;
}

static int DataCallTest(void* contextPtr)
{
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4;

    LE_INFO("Test Data start");

    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    TestSemRef = le_sem_Create("dcs_test_sem", 0);

    const char* profileIdPtr = le_arg_GetArg(1);
    remoteDataProfileid = strtol(profileIdPtr, NULL, 0);

    dataSessionThRef = le_thread_Create("dataSessionTh",
                                                         taf_data_session_handler, &ipType);

    le_thread_Start(dataSessionThRef);

    LE_INFO("Test data call start");

    ipv4_datacall_test();

    le_sem_Wait(TestSemRef);

    LE_INFO("Test data call end");

    LE_TEST_BEGIN_SKIP(!SSIM_TEST && !PHONE_ID_1_TEST, 1);
    LE_INFO("---remove session %p",TestSessionStateRef);
    taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);
    LE_TEST_OK(le_thread_Cancel(dataSessionThRef) == LE_OK, "le_thread_Cancel session - OK");

    LE_TEST_END_SKIP();

    LE_INFO("---remove session for RPC %p",taf_dcs_GetProfile(remoteDataProfileid));
    taf_dcs_RemoveSessionStateHandler(TestSessionStateRef);

    LE_INFO("====all tests are passed");

    le_sem_Delete(TestSemRef);

    return EXIT_SUCCESS;
}


COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    const char* testType = "";
    remoteDataProfileid = 1;

    le_result_t res = rpc_taf_net_TryConnectService();

    le_result_t result = rpc_taf_netIpPass_TryConnectService();

    isRpcNetConnected = (res == LE_OK && result == LE_OK);

    if(res == LE_OK) {
        LE_INFO("Client connected successfully to the remote taf_net service.");
    } else {
        LE_INFO("Client unable to connect the remote taf_net service!");
    }

    res = rpc_taf_dcs_TryConnectService();

    isRpcDcsConnected = res == LE_OK;

    if(res == LE_OK) {
        LE_INFO("Client connected successfully to the remote taf_dcs service.");
    } else {
        LE_INFO("Client unable to connect the remote taf_dcs service!");
    }

    if (le_arg_NumArgs() == 0)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
    if (le_arg_NumArgs() >= 1)
    {
        testType = le_arg_GetArg(0);
        printf("arg0=%s ",testType);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }

        if(!isRpcNetConnected && strncmp(testType, "rpc", 3) == 0) {
            if(strcmp(testType, "rpcdatacall") != 0) {
                printf("Remote NET RPC is not connecetd!\n");
                exit(EXIT_FAILURE);
            }
        }

        if(strcmp(testType, "getinterfacelist") ==0)
        {
            status=TafNetGetInterfaceList();
        }
        else if(strcmp(testType, "listen") == 0)
        {
          printf("===register handler started===\n");
          le_thread_Start(le_thread_Create("NetTestThread", HandlerThread, NULL));
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
        else if(strcmp(testType, "rpccreatevlan") == 0)
        {
            status=RpcTafCreateVlan();
        }
        else if(strcmp(testType, "rpcremovevlan") == 0)
        {
            status=RpcTafRemoveVlan();
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
        else if(strcmp(testType, "rpcbindwithprofile") == 0)
        {
            status=RpcTafVlanBindWithProfile();
        }
        else if(strcmp(testType, "rpcbindwithprofileex") == 0)
        {
            status=RpcTafVlanBindWithProfileEx();
        }
        else if(strcmp(testType, "rpcunbindwithprofile") == 0)
        {
            status=RpcTafVlanUnBindWithProfile();
        }
        else if(strcmp(testType, "bindwithBackhaul") == 0)
        {
            status=TafVlanBindWithBackhaul();
        }
        else if(strcmp(testType, "bindwithBackhaulex") == 0)
        {
            status=TafVlanBindWithBackhaulEx();
        }
        else if(strcmp(testType, "unbindwithBackhaul") == 0)
        {
            status=TafVlanUnBindWithBackhaul();
        }
        else if(strcmp(testType, "rpcbindwithBackhaul") == 0)
        {
            status=RpcTafVlanBindWithBackhaul();
        }
        else if(strcmp(testType, "rpcbindwithBackhaulex") == 0)
        {
            status=RpcTafVlanBindWithBackhaulEx();
        }
        else if(strcmp(testType, "rpcunbindwithBackhaul") == 0)
        {
            status=RpcTafVlanUnBindWithBackhaul();
        }
        else if(strcmp(testType, "rpcsetIPPTOperation") == 0)
        {
            status=RpcTafSetIPPTOperation();
        }
        else if(strcmp(testType, "rpcgetIPPTConfig") == 0)
        {
            status=RpcTafGetIPPTConfig();
        }
        else if(strcmp(testType, "setIPPTOperation") == 0)
        {
            status=TafSetIPPTOperation();
        }
        else if(strcmp(testType, "getIPPTConfig") == 0)
        {
            status=TafGetIPPTConfig();
        }
        else if(strcmp(testType, "setIPConfig") == 0)
        {
            status=TafSetIPConfig();
        }
        else if(strcmp(testType, "getIPConfig") == 0)
        {
            status=TafGetIPConfig();
        }
        else if(strcmp(testType, "rpcdatacall") == 0)
        {
            if (isRpcDcsConnected) {
                status = RpcDataCallTest(NULL);
            } else {
                printf("Remote DCS RPC is not connecetd!\n");
            }
        }
        else if(strcmp(testType, "datacall") == 0)
        {
            status = DataCallTest(NULL);
        }

        exit(status);
    }
}
