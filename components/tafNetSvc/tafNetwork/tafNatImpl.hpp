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
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NatManager.hpp>
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

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
     * @brief A callback class must be provided when invoke teladk API.
     */
    class tafNatCallback
    {
        public:
            static void onNatListResponse(
                      const std::vector<telux::data::net::NatConfig> &snatEntries, telux::common::ErrorCode error);
            void onResponseCallback(telux::common::ErrorCode error);
            tafNatCallback(){};
            ~tafNatCallback(){};
            static std::vector<telux::data::net::NatConfig> destNatEntryInfo;
            static le_sem_Ref_t semaphore;
    };

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
            taf_net_IpProto_t MapIPProtocol(telux::data::IpProtocol iptype);
            static void FirstLayerDestNatChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
            void onInitComplete(telux::common::ServiceStatus status);

            le_mem_PoolRef_t destNatEntryListPool;
            le_mem_PoolRef_t destNatEntryPool;
            le_mem_PoolRef_t destNatEntrySafeRefPool;
            le_ref_MapRef_t destNatEntryListRefMap;
            le_ref_MapRef_t destNatEntrySafeRefMap;
            le_event_Id_t DestNatChangeEvId;
            le_mem_PoolRef_t DestNatChangePool;
            std::promise<le_result_t> NatSyncPromise;

        private:
            le_result_t CleanListRef(taf_net_DestNatEntryListRef_t destNatEntryListRef);

            std::shared_ptr<telux::data::net::INatManager> staticNatManager = nullptr;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            bool IsSubSystemStatusUpdated=false;
            std::mutex mMutex;
            std::condition_variable conVar;
#endif
    };

}

