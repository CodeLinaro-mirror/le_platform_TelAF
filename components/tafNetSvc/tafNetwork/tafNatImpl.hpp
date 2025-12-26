/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "tafSvcIF.hpp"

#include "taf_pa_nat.hpp"
/*
 * @brief The struct of static destination nat entry.
 */
typedef struct
{
    char addr[TAF_NET_IP_ADDR_MAX_LEN];
    uint16_t port;
    uint16_t globalPort;
    uint8_t proto;
} taf_NatConfig_t;

/*
 * @brief The struct of static destination nat entry list.
 */
typedef struct
{
    le_sls_List_t destNatEntryList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
    uint32_t profileId;
} taf_DestNatEntryList_t;

/*
 * @brief The struct of static destination nat entry with link.
 */
typedef struct
{
    taf_NatConfig_t info;
    le_sls_Link_t link;
} taf_DestNatEntry_t;

/*
 * @brief The struct of safe reference for destination nat entry.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_DestNatEntrySafeRef_t;

namespace tafsvc {

    /*
     * @brief taf_Nat class defined as a middleware between interfaces and implementation.
     */
    class taf_Nat :public ITafSvc
    {
        public:
            taf_Nat() {};
            ~taf_Nat() {};

            void Init(void);
            static taf_Nat &GetInstance();
            le_result_t AddDestNatEntry (uint32_t profileId, const char *priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto);
            le_result_t RemoveDestNatEntry (uint32_t profileId, const char *priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto);
            le_result_t GetDestNatEntryDetails(taf_net_DestNatEntryRef_t destNatEntryRef, char* privateIpAddrPtr, size_t privateIpAddrPtrSize, uint16_t* privatePort, uint16_t* globalPort, taf_net_IpProto_t* proto);
            taf_net_DestNatEntryRef_t GetNextDestNatEntry( taf_net_DestNatEntryListRef_t destNatEntryListRef);
            taf_net_DestNatEntryRef_t GetFirstDestNatEntry(taf_net_DestNatEntryListRef_t destNatEntryListRef);
            taf_net_DestNatEntryListRef_t GetDestNatEntryList(uint32_t profileId);
            le_result_t DeleteDestNatEntryList(taf_net_DestNatEntryListRef_t destNatEntryListRef);
            bool IsRmnetBringUp(uint32_t profileId);
            bool IsDestNatEntryPresent(uint32_t profileId, const char* priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto);
            taf_net_IpProto_t MapIPProtocol(uint8_t iptype);
            static void FirstLayerDestNatChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);

            le_mem_PoolRef_t destNatEntryListPool;
            le_mem_PoolRef_t destNatEntryPool;
            le_mem_PoolRef_t destNatEntrySafeRefPool;
            le_ref_MapRef_t destNatEntryListRefMap;
            le_ref_MapRef_t destNatEntrySafeRefMap;
            le_event_Id_t DestNatChangeEvId;
            le_mem_PoolRef_t DestNatChangePool;

        private:
            le_result_t CleanListRef(taf_net_DestNatEntryListRef_t destNatEntryListRef);

    };

}

