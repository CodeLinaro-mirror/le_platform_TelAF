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

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NatManager.hpp>
#include "tafSvcIF.hpp"

#define SESSION_TIMEOUT 60

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

namespace telux{
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
            bool IsSubSystemStatusUpdated=false;
            std::mutex mMutex;
            std::condition_variable conVar;
    };

}
}

