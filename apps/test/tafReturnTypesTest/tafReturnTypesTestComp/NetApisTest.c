/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Net Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void netRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("netRetTest_RunApis");

    //1.taf_net_GetFirstDestNatEntry NULL scenario
    taf_net_DestNatEntryRef_t destNat;
    destNat = taf_net_GetFirstDestNatEntry(NULL);
    LE_TEST_OK(destNat == NULL,"taf_net_GetFirstDestNatEntry-NULL");

    //2.taf_net_GetNextDestNatEntry NULL scenario
    destNat = taf_net_GetNextDestNatEntry(NULL);
    LE_TEST_OK(destNat == NULL,"taf_net_GetNextDestNatEntry-NULL");

    //3.taf_net_GetDestNatEntryDetails LE_BAD_PARAMETER scenario
    res = taf_net_GetDestNatEntryDetails(NULL,NULL,0,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetDestNatEntryDetails-LE_BAD_PARAMETER");

    //4.taf_net_DeleteDestNatEntryList LE_BAD_PARAMETER scenario
    res = taf_net_DeleteDestNatEntryList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_DeleteDestNatEntryList-LE_BAD_PARAMETER");

    //5.taf_net_SetVlanPriority LE_OUT_OF_RANGE scenario
    res = taf_net_SetVlanPriority(NULL,8);
    LE_TEST_OK(res == LE_OUT_OF_RANGE,"taf_net_SetVlanPriority-LE_OUT_OF_RANGE");

    //6.taf_net_SetVlanPriority LE_BAD_PARAMETER scenario
    res = taf_net_SetVlanPriority(NULL,7);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_SetVlanPriority-LE_BAD_PARAMETER");

    //7.taf_net_RemoveVlan LE_BAD_PARAMETER scenario
    res = taf_net_RemoveVlan(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_RemoveVlan-LE_BAD_PARAMETER");

    //8.taf_net_RemoveVlanInterface LE_BAD_PARAMETER scenario
    res = taf_net_RemoveVlanInterface(NULL,TAF_NET_ECM);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_RemoveVlanInterface-LE_BAD_PARAMETER");

    //9.taf_net_GetVlanInterfaceList NULL scenario
    taf_net_VlanIfListRef_t netRef;
    netRef = taf_net_GetVlanInterfaceList(NULL);
    LE_TEST_OK(netRef == NULL,"taf_net_GetVlanInterfaceList-NULL");

    //10.taf_net_GetFirstVlanInterface NULL scenario
    taf_net_VlanIfRef_t vlanref;
    vlanref = taf_net_GetFirstVlanInterface(NULL);
    LE_TEST_OK(vlanref == NULL,"taf_net_GetFirstVlanInterface-NULL");

    //11.taf_net_GetNextVlanInterface NULL scenario
    vlanref = taf_net_GetNextVlanInterface(NULL);
    LE_TEST_OK(vlanref == NULL,"taf_net_GetNextVlanInterface-NULL");

    //12.taf_net_DeleteVlanInterfaceList LE_BAD_PARAMETER scenario
    res = taf_net_DeleteVlanInterfaceList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_DeleteVlanInterfaceList-LE_BAD_PARAMETER");

    //13.taf_net_GetVlanInterfaceType TAF_NET_IFACE_UNKNOWN scenario
    taf_net_VlanIfType_t interface;
    interface = taf_net_GetVlanInterfaceType(NULL);
    LE_TEST_OK(interface == TAF_NET_IFACE_UNKNOWN,"taf_net_GetVlanInterfaceType-TAF_NET_IFACE_UNKNOWN");

    //14.taf_net_GetVlanPriority LE_BAD_PARAMETER scenario
    res = taf_net_GetVlanPriority(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetVlanPriority-LE_BAD_PARAMETER");

    //15.taf_net_GetFirstVlanEntry NULL scenario
    taf_net_VlanEntryRef_t vlanEntry;
    vlanEntry = taf_net_GetFirstVlanEntry(NULL);
    LE_TEST_OK(vlanEntry == NULL,"taf_net_GetFirstVlanEntry-NULL");

    //16.taf_net_GetNextVlanEntry NULL scenario
    vlanEntry = taf_net_GetNextVlanEntry(NULL);
    LE_TEST_OK(vlanEntry == NULL,"taf_net_GetNextVlanEntry-NULL");

    //17.taf_net_DeleteVlanEntryList LE_BAD_PARAMETER scenario
    res = taf_net_DeleteVlanEntryList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_DeleteVlanEntryList-LE_BAD_PARAMETER");

    //18.taf_net_GetVlanId -1 scenario
    int16_t vlanID;
    vlanID = taf_net_GetVlanId(NULL);
    LE_TEST_OK(vlanID == -1,"taf_net_GetVlanId--1");

    //19.taf_net_IsVlanAccelerated LE_BAD_PARAMETER scenario
    res = taf_net_IsVlanAccelerated(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_IsVlanAccelerated-LE_BAD_PARAMETER");

    //20.taf_net_GetVlanBoundProfileId -1 scenario
    int32_t profID;
    profID = taf_net_GetVlanBoundProfileId(NULL);
    LE_TEST_OK(profID == -1,"taf_net_GetVlanBoundProfileId--1");

    //21.taf_net_GetVlanBoundPhoneId LE_BAD_PARAMETER scenario
    res = taf_net_GetVlanBoundPhoneId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetVlanBoundPhoneId-LE_BAD_PARAMETER");

    //22.taf_net_BindVlanWithProfileEx LE_BAD_PARAMETER scenario
    res = taf_net_BindVlanWithProfileEx(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_BindVlanWithProfileEx-LE_BAD_PARAMETER");

    //23.taf_net_UnbindVlanFromProfile LE_BAD_PARAMETER scenario
    res = taf_net_UnbindVlanFromProfile(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_UnbindVlanFromProfile-LE_BAD_PARAMETER");

    //24.taf_net_RemoveSession LE_BAD_PARAMETER scenario
    res = taf_net_RemoveSession(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_RemoveSession-LE_BAD_PARAMETER");

    //25.taf_net_StopTunnel LE_BAD_PARAMETER scenario
    res = taf_net_StopTunnel(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_StopTunnel-LE_BAD_PARAMETER");

    //26.taf_net_GetFirstTunnelEntry NULL scenario
    taf_net_TunnelEntryRef_t tunEntry;
    tunEntry = taf_net_GetFirstTunnelEntry(NULL);
    LE_TEST_OK(tunEntry == NULL,"taf_net_GetFirstTunnelEntry-NULL");

    //27.taf_net_GetNextTunnelEntry NULL scenario
    tunEntry = taf_net_GetNextTunnelEntry(NULL);
    LE_TEST_OK(tunEntry == NULL,"taf_net_GetNextTunnelEntry-NULL");

    //28.taf_net_DeleteTunnelEntryList LE_BAD_PARAMETER scenario
    res = taf_net_DeleteTunnelEntryList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_DeleteTunnelEntryList-LE_BAD_PARAMETER");

    //29.taf_net_GetTunnelEncapProto TAF_NET_L2TP_NONE scenario
    taf_net_L2tpEncapProtocol_t L2tpEncap;
    L2tpEncap = taf_net_GetTunnelEncapProto(NULL);
    LE_TEST_OK(L2tpEncap == TAF_NET_L2TP_NONE,"taf_net_GetTunnelEncapProto-TAF_NET_L2TP_NONE");

    //30.taf_net_GetTunnelLocalId 0 scenario
    uint32_t LocalID;
    LocalID = taf_net_GetTunnelLocalId(NULL);
    LE_TEST_OK(LocalID == 0,"taf_net_GetTunnelLocalId-0");

    //31.taf_net_GetTunnelPeerId 0 scenario
    LocalID = taf_net_GetTunnelPeerId(NULL);
    LE_TEST_OK(LocalID == 0,"taf_net_GetTunnelPeerId-0");

    //32.taf_net_GetTunnelLocalUdpPort 0 scenario
    LocalID = taf_net_GetTunnelLocalUdpPort(NULL);
    LE_TEST_OK(LocalID == 0,"taf_net_GetTunnelLocalUdpPort-0");

    //33.taf_net_GetTunnelPeerUdpPort 0 scenario
    LocalID = taf_net_GetTunnelPeerUdpPort(NULL);
    LE_TEST_OK(LocalID == 0,"taf_net_GetTunnelPeerUdpPort-0");

    //34.taf_net_GetTunnelPeerIpv6Addr LE_BAD_PARAMETER scenario
    res = taf_net_GetTunnelPeerIpv6Addr(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetTunnelPeerIpv6Addr-LE_BAD_PARAMETER");

    //35.taf_net_GetTunnelPeerIpv4Addr LE_BAD_PARAMETER scenario
    res = taf_net_GetTunnelPeerIpv4Addr(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetTunnelPeerIpv4Addr-LE_BAD_PARAMETER");

    //36.taf_net_GetTunnelInterfaceName LE_BAD_PARAMETER scenario
    res = taf_net_GetTunnelInterfaceName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetTunnelInterfaceName-LE_BAD_PARAMETER");

    //37.taf_net_GetTunnelIpType TAF_NET_L2TP_UNKNOWN scenario
    taf_net_IpFamilyType_t IpFamily;
    IpFamily = taf_net_GetTunnelIpType(NULL);
    LE_TEST_OK(IpFamily == TAF_NET_L2TP_UNKNOWN,"taf_net_GetTunnelIpType-TAF_NET_L2TP_UNKNOWN");

    //38.taf_net_GetSessionConfig LE_BAD_PARAMETER scenario
    res = taf_net_GetSessionConfig(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_net_GetSessionConfig-LE_BAD_PARAMETER");

    //39.taf_net_SetSocksAuthMethod LE_FAULT scenario
    res = taf_net_SetSocksAuthMethod(2);
    LE_TEST_OK(res == LE_FAULT,"taf_net_SetSocksAuthMethod-LE_FAULT");

    //40.taf_net_GetSocksAuthMethod TAF_NET_SOCKS_NONE scenario
    taf_net_AuthMethod_t Auth;
    taf_net_SetSocksAuthMethod(TAF_NET_SOCKS_NONE);
    Auth = taf_net_GetSocksAuthMethod();
    LE_TEST_OK(Auth == TAF_NET_SOCKS_NONE,"taf_net_GetSocksAuthMethod-TAF_NET_SOCKS_NONE");

    //41.taf_net_GetSocksAuthMethod TAF_NET_SOCKS_USER_PASSWD scenario
    taf_net_SetSocksAuthMethod(TAF_NET_SOCKS_USER_PASSWD);
    Auth = taf_net_GetSocksAuthMethod();
    LE_TEST_OK(Auth == TAF_NET_SOCKS_USER_PASSWD,"taf_net_GetSocksAuthMethod-TAF_NET_SOCKS_USER_PASSWD");

    //42.taf_net_GetFirstGsb NULL scenario
    taf_net_GsbRef_t Gsb;
    Gsb = taf_net_GetFirstGsb(NULL);
    LE_TEST_OK(Gsb == NULL,"taf_net_GetFirstGsb-NULL");

    //43.taf_net_GetNextGsb NULL scenario
    Gsb = taf_net_GetNextGsb(NULL);
    LE_TEST_OK(Gsb == NULL,"taf_net_GetNextGsb-NULL");

    //44.taf_net_GetBackhaulPreference LE_BAD_PARAMETER  scenario
    res = taf_net_GetBackhaulPreference(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetBackhaulPreference-LE_BAD_PARAMETER ");

    //45.taf_net_SetVlanNetworkType LE_BAD_PARAMETER  scenario
    res = taf_net_SetVlanNetworkType(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetVlanNetworkType-LE_BAD_PARAMETER ");

    //46.taf_net_GetVlanNetworkType LE_BAD_PARAMETER  scenario
    res = taf_net_GetVlanNetworkType(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetVlanNetworkType-LE_BAD_PARAMETER ");

    //47.taf_net_SetVlanBackhaulType LE_BAD_PARAMETER  scenario
    res = taf_net_SetVlanBackhaulType(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetVlanBackhaulType-LE_BAD_PARAMETER ");

    //48.taf_net_SetVlanBackhaulVlanId LE_BAD_PARAMETER  scenario
    res = taf_net_SetVlanBackhaulVlanId(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetVlanBackhaulVlanId-LE_BAD_PARAMETER ");

    //49.taf_net_SetVlanBackhaulPhoneId LE_BAD_PARAMETER  scenario
    res = taf_net_SetVlanBackhaulPhoneId(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetVlanBackhaulPhoneId-LE_BAD_PARAMETER ");

    //50.taf_net_SetVlanBackhaulProfileId LE_BAD_PARAMETER  scenario
    res = taf_net_SetVlanBackhaulProfileId(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetVlanBackhaulProfileId-LE_BAD_PARAMETER ");

    //51.taf_net_BindVlanWithBackhaul LE_BAD_PARAMETER  scenario
    res = taf_net_BindVlanWithBackhaul(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_BindVlanWithBackhaul-LE_BAD_PARAMETER ");

    //52.taf_net_UnbindVlanFromBackhaul LE_BAD_PARAMETER  scenario
    res = taf_net_UnbindVlanFromBackhaul(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_UnbindVlanFromBackhaul-LE_BAD_PARAMETER ");

    //53.taf_net_RemoveInterface LE_BAD_PARAMETER  scenario
    res = taf_net_RemoveInterface(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_RemoveInterface-LE_BAD_PARAMETER ");

    //54.taf_net_SetIPPTOperation LE_BAD_PARAMETER  scenario
    res = taf_net_SetIPPTOperation(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPPTOperation-LE_BAD_PARAMETER ");

    //55.taf_net_SetIPPTDeviceMacAddress LE_BAD_PARAMETER  scenario
    res = taf_net_SetIPPTDeviceMacAddress(NULL,TAF_NET_IFACE_UNKNOWN,"IPP");
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPPTDeviceMacAddress-LE_BAD_PARAMETER ");

    //56.taf_net_SetIPPassThroughConfig LE_BAD_PARAMETER  scenario
    res = taf_net_SetIPPassThroughConfig(NULL,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPPassThroughConfig-LE_BAD_PARAMETER ");

    //57.taf_net_GetIPPTOperation LE_BAD_PARAMETER  scenario
    res = taf_net_GetIPPTOperation(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetIPPTOperation-LE_BAD_PARAMETER ");

    //58.taf_net_GetIPPTDeviceMacAddress LE_BAD_PARAMETER  scenario
    res = taf_net_GetIPPTDeviceMacAddress(NULL,NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetIPPTDeviceMacAddress-LE_BAD_PARAMETER ");

    //59.taf_net_SetIPConfig LE_BAD_PARAMETER  scenario
    res = taf_net_SetIPConfig(NULL,0,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPConfig-LE_BAD_PARAMETER ");

    //60.taf_net_SetIPConfigParams LE_BAD_PARAMETER  scenario
    res = taf_net_SetIPConfigParams(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPConfigParams-LE_BAD_PARAMETER ");

    //61.taf_net_SetIPConfigAddressParams LE_BAD_PARAMETER  scenario
    taf_net_IpAddressInfo_t ipAddrInfo;
    res = taf_net_SetIPConfigAddressParams(NULL,&ipAddrInfo);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_SetIPConfigAddressParams-LE_BAD_PARAMETER ");

    //62.taf_net_GetIPConfigParams LE_BAD_PARAMETER  scenario
    res = taf_net_GetIPConfigParams(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetIPConfigParams-LE_BAD_PARAMETER ");

    //63.taf_net_GetIPConfigAddressParams LE_BAD_PARAMETER  scenario
    res = taf_net_GetIPConfigAddressParams(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetIPConfigAddressParams-LE_BAD_PARAMETER ");

    //64.taf_net_GetIPPTNatConfig LE_BAD_PARAMETER  scenario
    res = taf_net_GetIPPTNatConfig(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_net_GetIPPTNatConfig-LE_BAD_PARAMETER ");

}
