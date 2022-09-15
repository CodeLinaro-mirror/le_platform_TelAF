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
#include <telux/data/net/VlanManager.hpp>
#include "tafSvcIF.hpp"

#define MIN_VLAN_ID                         1  /*vlan 0 is reserved as per RFC*/
#define MAX_VLAN_ID                         4094/*vlan 4095 is max and it is reserved*/

using namespace telux::data;
using namespace telux::common;

/*
 * @brief The struct of vlan.
 */
typedef struct
{
    int16_t vlanId;
    bool isAccelerated;
    taf_net_VlanRef_t vlanRef;
    le_msg_SessionRef_t sessionRef;
} taf_Vlan_t;


/*
 * @brief The struct of vlan entry.
 */
typedef struct
{
    taf_net_VlanIfType_t interface;
    int16_t vlanId;
    bool isAccelerated;
} taf_VlanConfig_t;

/*
 * @brief The struct of vlan info.
 */
typedef struct
{
    int32_t profileId;
    int16_t vlanId;
    bool isAccelerated;
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

namespace telux{
namespace tafsvc {
    /*
     * @brief A callback class must be provided when invoke teladk API.
     */
    class tafVlanCallback
    {
        public:
            static void onVlanListResponse(
                      const std::vector<telux::data::VlanConfig> &vlanConfigs,
                      telux::common::ErrorCode error);

            void removeVlanResponse(telux::common::ErrorCode error);
            void createVlanResponse(bool isAccelerated, telux::common::ErrorCode error);

            tafVlanCallback(){};
            ~tafVlanCallback(){};
            static std::vector<telux::data::VlanConfig> vlanEntryInfo;
            static le_sem_Ref_t semaphore;
    };

    /*
     * @brief A callback class must be provided when invoke teladk API.
     */
    class tafVlanMappingCallback
    {
        public:
            static void onVlanMappingListResponse(
                      const std::list<std::pair<int, int>> &mapping,
                      telux::common::ErrorCode error);
            void onResponseCallback(telux::common::ErrorCode error);

            tafVlanMappingCallback(){};
            ~tafVlanMappingCallback(){};
            static std::list<std::pair<int, int>> vlanMappingInfo;
            static le_sem_Ref_t semaphore;
    };

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
            le_result_t BindVlanWithProfile(taf_net_VlanRef_t vlanRef, uint32_t profileId);
            le_result_t UnbindVlanFromProfile(taf_net_VlanRef_t vlanRef);
            void onInitComplete(telux::common::ServiceStatus status);
            uint16_t GetBoundVlanIdFromProfile(uint32_t profileId);
            int32_t GetBoundProfileIdFromVlan(uint16_t vlanId);
            le_result_t GetBindingInfo();

            taf_net_VlanRef_t CreateVlan(uint16_t vlanId, bool isAccelerated, le_msg_SessionRef_t sessionRef);
            le_result_t RemoveVlan(taf_net_VlanRef_t vlanRef);
            le_result_t AddVlanInterface(taf_net_VlanRef_t vlanRef, taf_net_VlanIfType_t ifType);
            le_result_t RemoveVlanInterface(taf_net_VlanRef_t vlanRef,
                                        taf_net_VlanIfType_t ifType);
            taf_net_VlanRef_t GetVlanRefById(uint16_t vlanId, le_msg_SessionRef_t sessionRef);
            bool IsVlanPresentInDb(uint16_t vlanId, bool *isAccelerated);
            bool IsVlanInterfacePresentInDb(uint16_t vlanId, taf_net_VlanIfType_t ifType);
            taf_net_VlanEntryListRef_t GetVlanEntryList();
            taf_net_VlanEntryRef_t GetFirstVlanEntry(taf_net_VlanEntryListRef_t vlanEntryListRef);
            taf_net_VlanEntryRef_t GetNextVlanEntry(taf_net_VlanEntryListRef_t vlanEntryListRef);
            le_result_t DeleteVlanEntryList(taf_net_VlanEntryListRef_t vlanEntryListRef);
            int16_t GetVlanId(taf_net_VlanEntryRef_t vlanEntryRef);
            le_result_t IsVlanAccelerated(taf_net_VlanEntryRef_t vlanEntryRef,
                                          bool* isAcceleratedPtr);
            int32_t GetVlanProfileId(taf_net_VlanEntryRef_t vlanEntryRef);
            taf_net_VlanIfListRef_t GetVlanInterfaceList(taf_net_VlanRef_t vlanRef);
            taf_net_VlanIfRef_t GetFirstVlanInterface(taf_net_VlanIfListRef_t vlanIfListRef);
            taf_net_VlanIfRef_t GetNextVlanInterface(taf_net_VlanIfListRef_t vlanIfListRef);
            le_result_t DeleteVlanInterfaceList(taf_net_VlanIfListRef_t vlanIfListRef);
            taf_net_VlanIfType_t GetVlanInterfaceType(taf_net_VlanIfRef_t vlanIfRef);

            le_result_t CleanListRef(taf_net_VlanEntryListRef_t vlanEntryListRef);
            le_result_t CleanVlanInterfaceListRef(taf_net_VlanIfListRef_t vlanIfListRef);

            static bool sort_vlanId(const telux::data::VlanConfig& s1,
                                    const telux::data::VlanConfig& s2);
            std::promise<le_result_t> VlanSyncPromise;

            le_mem_PoolRef_t vlanPool;
            le_ref_MapRef_t vlanRefMap;

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
        private:
            std::shared_ptr<telux::data::net::IVlanManager> vlanManager = nullptr;
#ifdef TARGET_SA515M //only sa515 using
            bool IsSubSystemStatusUpdated=false;
            std::mutex mMutex;
            std::condition_variable conVar;
#endif
    };

}
}

