/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

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


static void PrintUsage ()
{
    puts("\n"
            "app start tafDualNadTest\n"
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
<vlanid> <profileid> <BackhaulType> [Optional: phoneid]\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- unbindwithBackhaul <vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithBackhaul \
<Remote vlanid> <Remote profile Id> <Remote BackhaulType>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcbindwithBackhaulex \
<Remote vlanid> <Remote profile Id> <Remote BackhaulType> [Optional: Remote phoneid]\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcunbindwithBackhaul <Remote vlanid>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setIPPTOperation \
<vlanid> <operation type> <interface type> <mac address>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcsetIPPTOperation \
<Remote vlanid> <Remote operation type> <Remote interface type> <Remote mac address>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getIPPTConfig <vlan id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcgetIPPTConfig <Remote vlan id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- setIPConfig <vlan id> <ifType> <ip type> <ip Opr> <assign type> [Optional if IpAssignType is DYNAMIC_IP: ifAddr> [Optional if IpAssignType is DYNAMIC_IP: gwAddr] [Optional if IpAssignType is DYNAMIC_IP: PriDnsAddr] [Optional if IpAssignType is DYNAMIC_IP: SecDnsAddr] [Optional if IpAssignType is DYNAMIC_IP: ifmask]\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getIPConfig <vlan id> <ifType> <ip type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getvlanentryinfo\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcdatacall <Remote profile id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- datacall <profile id>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcstartdatangetipconfig <Remote profile id> <vlan id> <ifType> <ip type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcstartdatansetipconfig <Remote profile id> <vlan id> <ifType> <ip type> <ip Opr> <assign type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- getIPConfig <vlan id> <ifType> <ip type>\n"
            "app runProc tafDualNadTest --exe=tafDualNadTest -- rpcstopdatacall <Remote profile id>\n"
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
    int numArgs = le_arg_NumArgs();

    if (numArgs != 4 && numArgs != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* profileIdPtr = le_arg_GetArg(2);
    const char* bhPtr = le_arg_GetArg(3);

    const char* phoneIdPtr = "1";

    if (numArgs == 5) {
        phoneIdPtr = le_arg_GetArg(4);
    }

    if(vlanIdPtr == NULL || profileIdPtr == NULL || bhPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, backHaulType or profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint32_t profileid = strtol(profileIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    uint8_t phoneid = strtol(phoneIdPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=taf_net_GetVlanById(vlanid);

    if (numArgs == 5) {
        //User input phoneid (i.e. slot id)
        ret = taf_netIpPass_SetVlanBackhaulPhoneId(vlanRef, phoneid);
    }

    ret = taf_netIpPass_SetVlanBackhaulProfileId(vlanRef, profileid);

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

    int numArgs = le_arg_NumArgs();

    if (numArgs != 4 && numArgs != 5)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1);
    const char* profileIdPtr = le_arg_GetArg(2);
    const char* bhPtr = le_arg_GetArg(3);

    const char* phoneIdPtr = "1";

    if (numArgs == 5) {
        phoneIdPtr = le_arg_GetArg(4);
    }

    if(vlanIdPtr == NULL || profileIdPtr == NULL || bhPtr == NULL)
    {
        LE_ERROR("vlanIdPtr, backHaulType or profileIdPtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    uint32_t profileid = strtol(profileIdPtr, NULL, 0);
    uint32_t backHaulType = strtol(bhPtr, NULL, 0);

    uint8_t phoneid = strtol(phoneIdPtr, NULL, 0);

    taf_net_VlanRef_t vlanRef=rpc_taf_net_GetVlanById(vlanid);

    if (numArgs == 5) {
        //User input phoneid (i.e. slot id)
        ret = rpc_taf_netIpPass_SetVlanBackhaulPhoneId(vlanRef, phoneid);
    }

    ret = rpc_taf_netIpPass_SetVlanBackhaulProfileId(vlanRef, profileid);

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

static int TafSetIPConfig(int cmdOffset, bool takeIpInfoFromDataCall)
{
    le_result_t ret;

    int numArgs = le_arg_NumArgs();

    if (numArgs != 11+cmdOffset)
    {
        if ((numArgs < 6+cmdOffset) || ((numArgs == 6+cmdOffset) && atoi(le_arg_GetArg(5+cmdOffset)) == TAF_NETIPPASS_STATIC_IP && !takeIpInfoFromDataCall)) {
            //IpAssignType is TAF_NETIPPASS_STATIC_IP but neither input IP, GW, DNS0, DNS1 nor started data call and ask to take these from data call.
            LE_INFO("TafSetIPConfig numArgs: %d and IpAssignType: %d", numArgs, (numArgs < 6+cmdOffset) ? -1 : atoi(le_arg_GetArg(5+cmdOffset)));
            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }

    const char* vlanIdPtr = le_arg_GetArg(1+cmdOffset);
    const char* ifTypePtr = le_arg_GetArg(2+cmdOffset);
    const char* ipTypePtr = le_arg_GetArg(3+cmdOffset);
    const char* ipOprPtr = le_arg_GetArg(4+cmdOffset);
    const char* ipAssignTypePtr = le_arg_GetArg(5+cmdOffset);

    const char* interfaceAddrPtr = "";
    const char* gwAddrPtr = "";
    const char* primaryDnsAddrPtr = "";
    const char* secondaryDnsAddrPtr = "";
    const char* ifMaskAddrPtr = "";

    if (numArgs == 11+cmdOffset) {
        //IpAssignType is TAF_NETIPPASS_STATIC_IP and user input the IP Addr Details
        interfaceAddrPtr = le_arg_GetArg(6+cmdOffset);
        gwAddrPtr = le_arg_GetArg(7+cmdOffset);
        primaryDnsAddrPtr = le_arg_GetArg(8+cmdOffset);
        secondaryDnsAddrPtr = le_arg_GetArg(9+cmdOffset);
        ifMaskAddrPtr = le_arg_GetArg(10+cmdOffset);
    }

    if(vlanIdPtr == NULL || ipOprPtr == NULL || ipAssignTypePtr == NULL)
    {
        LE_ERROR("vlanIdPtr or ipOprPtr or ipAssignTypePtr is NULL");
        exit(EXIT_FAILURE);
    }

    uint32_t vlanid = strtol(vlanIdPtr, NULL, 0);
    taf_net_VlanIfType_t ifType = (taf_net_VlanIfType_t)strtol(ifTypePtr, NULL, 0);
    taf_net_NetIpType_t  ipType = (taf_net_NetIpType_t)strtol(ipTypePtr, NULL, 0);
    taf_netIpPass_IpAssignOperation_t ipOpr = (taf_netIpPass_IpAssignOperation_t)strtol(ipOprPtr, NULL, 0);
    taf_netIpPass_IpAssignType_t ipAssignType = (taf_netIpPass_IpAssignType_t)strtol(ipAssignTypePtr, NULL, 0);

    taf_netIpPass_IpAddressInfo_t ipAddrInfo;

    if (ipAssignType == TAF_NETIPPASS_STATIC_IP && numArgs == 11+cmdOffset) {
        //Take IP details from user input if IpAssignType is TAF_NETIPPASS_STATIC_IP
        le_utf8_Copy(ipAddrInfo.interfaceAddress, interfaceAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.gwAddress, gwAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.primaryDnsAddress, primaryDnsAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.secondaryDnsAddress, secondaryDnsAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        ipAddrInfo.interfaceMask = (uint32_t)strtol(ifMaskAddrPtr, NULL, 0);
    } else if (ipAssignType == TAF_NETIPPASS_STATIC_IP && takeIpInfoFromDataCall) {
        //Take IP details from data call in IpAssignType as TAF_NETIPPASS_STATIC_IP
        le_utf8_Copy(ipAddrInfo.interfaceAddress, ifaddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.gwAddress, gwaddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.primaryDnsAddress, pDNSaddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(ipAddrInfo.secondaryDnsAddress, sDNSaddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        ipAddrInfo.interfaceMask = ifsubnetMask;
    }

    if (ipAssignType == TAF_NETIPPASS_STATIC_IP) {
        LE_INFO("IPv4 info Addr: %s, GW: %s, DNS0: %s, DNS1: %s and Subnet Mask: %u\n",
                ipAddrInfo.interfaceAddress, ipAddrInfo.gwAddress, ipAddrInfo.primaryDnsAddress,
                ipAddrInfo.secondaryDnsAddress,
                takeIpInfoFromDataCall ? ifsubnetMask : (uint32_t) strtol(ifMaskAddrPtr, NULL, 0));
    }

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

static int TafGetIPConfig(int cmdOffset)
{
    le_result_t ret;

    if (le_arg_NumArgs() != 4+cmdOffset)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* vlanIdPtr = le_arg_GetArg(1+cmdOffset);
    const char* ifTypePtr = le_arg_GetArg(2+cmdOffset);
    const char* ipTypePtr = le_arg_GetArg(3+cmdOffset);

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
        printf("IPv4 Addr:        %s\n", ipAddr0);

        le_utf8_Copy(ifaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4GatewayAddress(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Gateway:     %s\n", ipAddr0);

        le_utf8_Copy(gwaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4DNSAddresses(profileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
        printf("IPv4 Dns0:        %s \nIPv4 Dns1:        %s\n", ipAddr0, ipAddr1);

        le_utf8_Copy(pDNSaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(sDNSaddress,ipAddr1, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=rpc_taf_dcs_GetIPv4SubnetMask(profileRef, &subNetMask);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Subnet Mask: %u\n", subNetMask);

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
        printf("IPv4 Addr:        %s\n", ipAddr0);

        le_utf8_Copy(ifaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4GatewayAddress(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Gateway:     %s\n", ipAddr0);

        le_utf8_Copy(gwaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4DNSAddresses(profileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
        printf("IPv4 Dns0:        %s \nIPv4 Dns1:        %s\n", ipAddr0, ipAddr1);

        le_utf8_Copy(pDNSaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(sDNSaddress,ipAddr1, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        result=taf_dcs_GetIPv4SubnetMask(profileRef, &subNetMask);
        LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
        printf("IPv4 Subnet Mask: %u\n", subNetMask);

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

void PrintRpcDataInfo() {

    char interfaceName[64];
    uint32_t profileId=0;
    uint8_t phoneId;
    uint32_t subNetMask=0;
    char ipAddr0[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipAddr1[TAF_DCS_IPV4_ADDR_MAX_LEN];
    le_result_t result = LE_OK;

    taf_dcs_ProfileRef_t profileRef = rpc_taf_dcs_GetProfile(remoteDataProfileid);

    rpc_taf_dcs_GetInterfaceName(profileRef, interfaceName, 64);

    result = rpc_taf_dcs_GetProfileIdByInterfaceName(interfaceName, &profileId);
    LE_TEST_OK(result == LE_OK, "Connected: profile(%d), ifname(%s)", profileId, interfaceName);
    printf("Connected: profile(%d), ifname(%s)\n", profileId, interfaceName);

    result= rpc_taf_dcs_GetIPv4Address(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4Address - LE_OK");
    printf("IPv4 Addr:        %s\n", ipAddr0);

    le_utf8_Copy(ifaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

    result=rpc_taf_dcs_GetIPv4GatewayAddress(profileRef, ipAddr0, TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
    printf("IPv4 Gateway:     %s\n", ipAddr0);

    le_utf8_Copy(gwaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);

    result=rpc_taf_dcs_GetIPv4DNSAddresses(profileRef, ipAddr0,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN, ipAddr1,
                                       TAF_DCS_IPV4_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4DNSAddresses - LE_OK");
    printf("IPv4 Dns0:        %s \nIPv4 Dns1:        %s\n", ipAddr0, ipAddr1);

    le_utf8_Copy(pDNSaddress,ipAddr0, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(sDNSaddress,ipAddr1, TAF_NET_IP_ADDR_MAX_LEN, NULL);

    result=rpc_taf_dcs_GetIPv4SubnetMask(profileRef, &subNetMask);
    LE_TEST_OK(result == LE_OK, "taf_dcs_GetIPv4GatewayAddress - LE_OK");
    printf("IPv4 Subnet Mask: %u\n", subNetMask);

    ifsubnetMask = subNetMask;

    result = rpc_taf_dcs_GetPhoneIdByInterfaceName(interfaceName, &phoneId);
    LE_TEST_OK(result == LE_OK, "Connected: phoneId(%d), ifname(%s)", phoneId, interfaceName);
    printf("Connected: phoneId(%d), ifname(%s)\n", phoneId, interfaceName);
}

static int RpcStartDataCallAndGetIpConfig()
{
    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* profileIdPtr = le_arg_GetArg(1);
    remoteDataProfileid = strtol(profileIdPtr, NULL, 0);

    le_result_t result = rpc_taf_mdc_StartSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));

    LE_TEST_OK(result == LE_OK, "rpc_taf_mdc_StartSession - OK");

    printf("Start remote data call. Result: %s\n", LE_RESULT_TXT(result));

    PrintRpcDataInfo();

    printf("Getting local IP config, wait ...\n");

    sleep(4);

    TafGetIPConfig(1);

    return EXIT_SUCCESS;
}

static int RpcStartDataCallAndSetIpConfig()
{
    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* profileIdPtr = le_arg_GetArg(1);
    remoteDataProfileid = strtol(profileIdPtr, NULL, 0);

    le_result_t result = rpc_taf_mdc_StartSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));

    LE_TEST_OK(result == LE_OK, "rpc_taf_mdc_StartSession - OK");

    printf("Start remote data call. Result: %s\n", LE_RESULT_TXT(result));

    PrintRpcDataInfo();

    printf("Setting local IP config, wait ...\n");

    sleep(4);

    //Need to start data call and cache ip info before give takeIpInfoFromDataCall as true.
    TafSetIPConfig(1, true);

    return EXIT_SUCCESS;
}

static int RpcStopDataCall()
{
    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    const char* profileIdPtr = le_arg_GetArg(1);
    remoteDataProfileid = strtol(profileIdPtr, NULL, 0);

    le_result_t result = rpc_taf_mdc_StopSession(rpc_taf_dcs_GetProfile(remoteDataProfileid));

    LE_TEST_OK(result == LE_OK, "rpc_taf_mdc_StopSession - OK");

    printf("Stop remote data call. Result: %s\n", LE_RESULT_TXT(result));

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

    if(isRpcNetConnected) {
        LE_INFO("Client connected successfully to the remote taf_net and taf_netIpPass services.");
    } else {
        LE_INFO("Client unable to connect the remote taf_net and/or taf_netIpPass service!");
    }

    res = rpc_taf_dcs_TryConnectService();
    result = rpc_taf_mdc_TryConnectService();

    isRpcDcsConnected = (res == LE_OK && result == LE_OK);

    if(isRpcDcsConnected) {
        LE_INFO("Client connected successfully to the remote taf_dcs and taf_mdc services.");
    } else {
        LE_INFO("Client unable to connect the remote taf_dcs and/or taf_mdc service!");
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
            if(strcmp(testType, "rpcdatacall") != 0 && strcmp(testType, "rpcstopdatacall") != 0) {
                printf("Remote NET RPC is not connecetd!\n");
                exit(EXIT_FAILURE);
            }
        }

        if(strcmp(testType, "createvlan") == 0)
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
            status=TafSetIPConfig(0, false);
        }
        else if(strcmp(testType, "getIPConfig") == 0)
        {
            status=TafGetIPConfig(0);
        }
        else if(strcmp(testType, "rpcdatacall") == 0)
        {
            if (isRpcDcsConnected) {
                status = RpcDataCallTest(NULL);
            } else {
                printf("Remote DCS and/or MDC RPC are not connecetd!\n");
            }
        }
        else if(strcmp(testType, "rpcstartdatangetipconfig") == 0)
        {
            if (isRpcDcsConnected) {
                status = RpcStartDataCallAndGetIpConfig();
            } else {
                printf("Remote DCS and/or MDC RPC are not connecetd!\n");
            }
        }
        else if(strcmp(testType, "rpcstartdatansetipconfig") == 0)
        {
            if (isRpcDcsConnected) {
                status = RpcStartDataCallAndSetIpConfig();
            } else {
                printf("Remote DCS and/or MDC RPC are not connecetd!\n");
            }
        }
        else if(strcmp(testType, "rpcstopdatacall") == 0)
        {
            if (isRpcDcsConnected) {
                status = RpcStopDataCall();
            } else {
                printf("Remote DCS and/or MDC RPC is not connecetd!\n");
            }
        }
        else if(strcmp(testType, "datacall") == 0)
        {
            status = DataCallTest(NULL);
        }

        exit(status);
    }
}
