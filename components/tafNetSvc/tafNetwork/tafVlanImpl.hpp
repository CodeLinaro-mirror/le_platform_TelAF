/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <iostream>
#include "tafSvcIF.hpp"
#include "taf_pa_vlan.hpp"

#define MIN_VLAN_ID                         1  /*vlan 0 is reserved as per RFC*/
#define MAX_VLAN_ID                         4094/*vlan 4095 is max and it is reserved*/
#define MAX_VLAN_PRIORITY                   7  /*The maxium value of vlan priority*/


/*
 * @brief The struct of IP PASS through operation config to REMOTE with WWAN support.
 */
typedef struct
{
    taf_net_VlanIfType_t ifType;
    char macAddr[TAF_NET_MAC_ADDR_MAX_LEN + 1];
    taf_net_Operation_t operation;
} taf_InterfaceIPPTConfig_t;

/*
 * @brief The struct of IP config to LOCAL without WWAN support.
 */
typedef struct
{
    taf_net_IpAssignOperation_t ipOpr;    // disable /enable /reconfigure
    taf_net_IpAssignType_t ipAssignType;  // static / dynamic
    taf_net_NetIpType_t  ipType;          // ipv4/ipv6/ipv4v6
    taf_net_IpAddressInfo_t  ipAddrInfo;
} taf_InterfaceIPConfig_t;

typedef struct
{
    int16_t vlanId;
    taf_net_VlanIfType_t ifType;
    taf_InterfaceIPPTConfig_t passThroughConfig;  // PASS through config for REMOTE ( NAD2 )
    taf_InterfaceIPConfig_t   ipConfig;           // IPConfig for LOCAL ( NAD1 )
} taf_InterfaceConfig_t;

/*
 * @brief The struct of vlan bind config.
 */
typedef struct
{
    uint8_t slotId;                  //optional slotId , otherwise default slotId
    int32_t profileId;               //mandatory if backhaul type is WWAN.
    taf_net_BackhaulType_t backhaulType; //VLAN bind config item for backhaul type.
    int16_t vlanIdBackhaul;          //optional VLAN bind config item if Backhaul is not WWAN.
} taf_VlanBHBindConfig_t;

/*
 * @brief The struct of vlan.
 */
typedef struct
{
    int16_t vlanId;
    bool isAccelerated;
    uint8_t priority;
    taf_net_NetworkType_t nwType;
    taf_net_VlanRef_t vlanRef;
    le_msg_SessionRef_t sessionRef;
    taf_VlanBHBindConfig_t vlanBindConfig;
    taf_InterfaceIPPTConfig_t passThroughConfig;
    taf_InterfaceIPConfig_t   ipConfig;
} taf_Vlan_t;


/*
 * @brief The struct of vlan info.
 */
typedef struct
{
    uint8_t slotId;
    int32_t profileId;
    int16_t vlanId;
    bool isAccelerated;
    taf_net_NetworkType_t nwType;
} taf_VlanInfo_t;

/*
 * @brief The struct of vlan entry list.
 */
typedef struct
{
    le_sls_List_t vlanEntryList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_VlanEntryList_t;

/*
 * @brief The struct of vlan entry with link.
 */
typedef struct
{
    taf_VlanInfo_t info;
    le_sls_Link_t link;
} taf_VlanEntry_t;

/*
 * @brief The struct of safe reference for vlan entry.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_VlanEntrySafeRef_t;

/*
 * @brief The struct of vlan interface list.
 */
typedef struct
{
    le_sls_List_t vlanIfList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
    uint16_t vlanId;
} taf_VlanIfList_t;

/*
 * @brief The struct of vlan interface with link.
 */
typedef struct
{
    taf_net_VlanIfType_t interface;
    uint8_t priority;
    le_sls_Link_t link;
} taf_VlanIf_t;

/*
 * @brief The struct of safe reference for vlan interface.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_VlanIfSafeRef_t;

// Internal message structure for onHwAccelerationChanged events
typedef struct
{
    taf_net_VlanHwAccelerationState_t state;
} VlanHwAccelerationState_t;

namespace tafsvc {

    /*
     * @brief taf_Vlan class defined as a middleware between interfaces and implementation.
     */
    class taf_Vlan :public ITafSvc
    {
        public:
            taf_Vlan() {};
            ~taf_Vlan() {};

            void Init(void);
            static taf_Vlan &GetInstance();

            static void ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void *contextPtr);
            le_result_t BindVlanWithProfile(taf_net_VlanRef_t vlanRef, uint8_t slotId, uint32_t profileId);
            le_result_t UnbindVlanFromProfile(taf_net_VlanRef_t vlanRef);
            uint16_t GetBackhaulVlanIdBoundWithVlan(uint16_t vlanId,
                                         taf_net_BackhaulType_t backhaulType, uint8_t slot);
            uint16_t GetBoundVlanIdFromSlotAndProfile(uint8_t slotId, uint32_t profileId);
            le_result_t GetBoundSlotIdProfileIdFromVlan(uint16_t vlanId, uint8_t* slotId,
                                                        uint32_t* profileId);
            le_result_t GetBindingInfo(uint8_t slotId);

            taf_net_VlanRef_t CreateVlan(uint16_t vlanId, bool isAccelerated,
                                         le_msg_SessionRef_t sessionRef);
            le_result_t RemoveVlan(taf_net_VlanRef_t vlanRef);
            le_result_t UnbindVlanFromBackhaul(taf_net_VlanRef_t vlanRef);
            le_result_t BindVlanWithBackhaul(taf_net_VlanRef_t vlanRef);
            //Create Parameters in REF
            le_result_t AddVlanInterface(taf_net_VlanRef_t vlanRef, taf_net_VlanIfType_t ifType);
            le_result_t SetVlanPriority(taf_net_VlanRef_t vlanRef, uint8_t priority);
            le_result_t SetVlanNetworkType(taf_net_VlanRef_t vlanRef,
                                                              taf_net_NetworkType_t nwType);
            //Bind Parameters in REF
            le_result_t SetVlanBackhaulType(taf_net_VlanRef_t vlanRef,
                                            taf_net_BackhaulType_t bhType);
            le_result_t SetVlanBackhaulVlanId(taf_net_VlanRef_t vlanRef,uint16_t vlanId);
            le_result_t SetVlanBackhaulProfileId(taf_net_VlanRef_t vlanRef,uint32_t profileId);
            le_result_t SetVlanBackhaulSlotId(taf_net_VlanRef_t vlanRef,uint8_t slotId);

            le_result_t RemoveVlanInterface(taf_net_VlanRef_t vlanRef,
                                        taf_net_VlanIfType_t ifType);
            taf_net_VlanRef_t GetVlanRefById(uint16_t vlanId, le_msg_SessionRef_t sessionRef);
            bool IsVlanPresentInDb(uint16_t vlanId, bool *isAccelerated, uint8_t *priority);
            bool IsVlanInterfacePresentInDb(uint16_t vlanId, taf_net_VlanIfType_t ifType);
            //Entry REF APIs which gives create and bind info.
            taf_net_VlanEntryListRef_t GetVlanEntryList();
            taf_net_VlanEntryRef_t GetFirstVlanEntry(taf_net_VlanEntryListRef_t vlanEntryListRef);
            taf_net_VlanEntryRef_t GetNextVlanEntry(taf_net_VlanEntryListRef_t vlanEntryListRef);
            le_result_t DeleteVlanEntryList(taf_net_VlanEntryListRef_t vlanEntryListRef);
            int16_t GetVlanId(taf_net_VlanEntryRef_t vlanEntryRef);
            le_result_t IsVlanAccelerated(taf_net_VlanEntryRef_t vlanEntryRef,
                                          bool* isAcceleratedPtr);
            int32_t GetVlanProfileId(taf_net_VlanEntryRef_t vlanEntryRef);
            le_result_t GetVlanPhoneId(taf_net_VlanEntryRef_t vlanEntryRef, uint8_t* phoneIdPtr);
            le_result_t GetVlanNetworkType(taf_net_VlanEntryRef_t vlanEntryRef,
                                                     taf_net_NetworkType_t  *networkType);
            //Interface REF APIs which give only create info.
            taf_net_VlanIfListRef_t GetVlanInterfaceList(taf_net_VlanRef_t vlanRef);
            taf_net_VlanIfRef_t GetFirstVlanInterface(taf_net_VlanIfListRef_t vlanIfListRef);
            taf_net_VlanIfRef_t GetNextVlanInterface(taf_net_VlanIfListRef_t vlanIfListRef);
            le_result_t DeleteVlanInterfaceList(taf_net_VlanIfListRef_t vlanIfListRef);
            taf_net_VlanIfType_t GetVlanInterfaceType(taf_net_VlanIfRef_t vlanIfRef);
            le_result_t GetVlanPriority(taf_net_VlanIfRef_t vlanIfRef, uint8_t* priority);
            le_result_t CleanListRef(taf_net_VlanEntryListRef_t vlanEntryListRef);
            le_result_t CleanVlanInterfaceListRef(taf_net_VlanIfListRef_t vlanIfListRef);
            // IP Pass through
            taf_net_InterfaceRef_t GetInterface(taf_net_VlanIfType_t ifType);
            le_result_t RemoveInterface(taf_net_InterfaceRef_t interfaceRef);
            le_result_t SetIPPTOperation(taf_net_InterfaceRef_t  interfaceRef,
                                                 taf_net_Operation_t  operation);
            le_result_t SetIPPTDeviceMacAddress(taf_net_InterfaceRef_t  interfaceRef,
                                                taf_net_VlanIfType_t ifType,
                                                const char *macAddr);
            le_result_t SetIPPassThroughConfig(taf_net_InterfaceRef_t  interfaceRef,
                                               uint16_t vlanid);

            taf_net_InterfaceRef_t GetIPPassThroughConfig(uint16_t vlanId);
            le_result_t GetIPPTOperation(taf_net_InterfaceRef_t vlanIPRef,
                                                 taf_net_Operation_t*  operation);
            le_result_t GetIPPTDeviceMacAddress(taf_net_InterfaceRef_t vlanIPRef,
                                                taf_net_VlanIfType_t *ifType,
                                                char *macAddr, size_t macAddrSize);
            // IP config
            le_result_t SetIPConfig(taf_net_InterfaceRef_t  interfaceRef,
                                    taf_net_NetIpType_t  ipType,
                                    taf_net_VlanIfType_t ifType,
                                    uint16_t vlanid);
            le_result_t SetIPConfigParams(taf_net_InterfaceRef_t  interfaceRef,
                                          taf_net_IpAssignOperation_t ipOpr,
                                          taf_net_IpAssignType_t ipType);
            le_result_t SetIPConfigAddressParams(taf_net_InterfaceRef_t  interfaceRef,
                                          const taf_net_IpAddressInfo_t*  ipAddrInfo);
            le_result_t GetIPConfigParams(taf_net_InterfaceRef_t vlanRef,
                                          taf_net_IpAssignOperation_t* ipOpr,
                                          taf_net_IpAssignType_t* ipType);
            le_result_t GetIPConfigAddressParams(taf_net_InterfaceRef_t vlanRef,
                                                 taf_net_IpAddressInfo_t*  ipAddrInfo);
            taf_net_InterfaceRef_t GetIPConfig(taf_net_NetIpType_t  ipType,
                                                 taf_net_VlanIfType_t ifType,
                                                 uint16_t vlanId);
            le_result_t SetIPPassThroughNatConfig(bool isEnabled);
            le_result_t GetIPPassThroughNatConfig(bool *isEnabledPtr);

            le_result_t GetBackhaulPreference(taf_net_BackhaulType_t* backhaulPrefListPtr,
                                              size_t* backhaulPrefListSizePtr);
            le_result_t SetBackhaulPreference(const taf_net_BackhaulType_t* backhaulPrefListPtr,
                                              size_t backhaulPrefListSize);

            static bool sort_vlanId(const taf_pa_Vlan_t& s1,
                                    const taf_pa_Vlan_t& s2);

            le_mem_PoolRef_t vlanPool;
            le_ref_MapRef_t vlanRefMap;

            le_mem_PoolRef_t interfacePool;
            le_ref_MapRef_t  interfaceRefMap;

            le_mem_PoolRef_t interfaceIPPool;
            le_ref_MapRef_t  interfaceIPRefMap;

            le_mem_PoolRef_t vlanEntryListPool;
            le_mem_PoolRef_t vlanEntryPool;
            le_mem_PoolRef_t vlanEntrySafeRefPool;
            le_ref_MapRef_t vlanEntryListRefMap;
            le_ref_MapRef_t vlanEntrySafeRefMap;

            le_mem_PoolRef_t vlanIfListPool;
            le_mem_PoolRef_t vlanIfPool;
            le_mem_PoolRef_t vlanIfSafeRefPool;
            le_ref_MapRef_t vlanIfListRefMap;
            le_ref_MapRef_t vlanIfSafeRefMap;

            // Hardware acceleration state event related declarations and functions
            le_event_Id_t vlanHwAccelerationStateEvtId;
            le_mem_PoolRef_t vlanHwAccelerationStateEvtPool;

            static std::map<uint8_t, std::list<std::pair<int, int>>> slotVlanMappingInfo;
            static std::vector<taf_pa_Vlan_t> vlanPAEntryInfo;

    };

}

