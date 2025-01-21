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
#include <algorithm>
#include "tafVlanImpl.hpp"
#include "tafNetworkImpl.hpp"
#include "tafSvcIF.hpp"
#include <arpa/inet.h>

#define OPERATION_TIMEOUT 30
#define OPERATION_DATA_SETTINGS 60

using namespace telux::tafsvc;

//Interface IP definition for LOCAL

LE_MEM_DEFINE_STATIC_POOL(interfacePool, TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_InterfaceConfig_t));

LE_REF_DEFINE_STATIC_MAP(interfaceRefMap, TAF_NET_MAX_VLAN_INTERFACE);

LE_MEM_DEFINE_STATIC_POOL(interfaceIPPool, TAF_NET_MAX_VLAN_INTERFACE,
                          sizeof(taf_InterfaceConfig_t));

LE_REF_DEFINE_STATIC_MAP(interfaceIPRefMap, TAF_NET_MAX_VLAN_INTERFACE);


//VLAN definition

LE_MEM_DEFINE_STATIC_POOL(vlanPool, TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_Vlan_t));

LE_REF_DEFINE_STATIC_MAP(vlanRefMap, TAF_NET_MAX_VLAN_ENTRY);

//VLAN entry list definition
LE_MEM_DEFINE_STATIC_POOL(vlanEntryListPool, TAF_NET_MAX_VLAN_ENTRY_LIST,
                          sizeof(taf_VlanEntryList_t));

LE_MEM_DEFINE_STATIC_POOL(vlanEntryPool, TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_VlanEntry_t));

LE_MEM_DEFINE_STATIC_POOL(vlanEntrySafeRefPool, TAF_NET_MAX_VLAN_ENTRY,
                          sizeof(taf_VlanEntrySafeRef_t));

LE_REF_DEFINE_STATIC_MAP(vlanEntryListRefMap, TAF_NET_MAX_VLAN_ENTRY_LIST);

LE_REF_DEFINE_STATIC_MAP(vlanEntrySafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

//VLAN interface list definition
LE_MEM_DEFINE_STATIC_POOL(vlanIfListPool, TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_VlanIfList_t));

LE_MEM_DEFINE_STATIC_POOL(vlanIfPool, TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_VlanIf_t));

LE_MEM_DEFINE_STATIC_POOL(vlanIfSafeRefPool, TAF_NET_MAX_VLAN_INTERFACE,
                          sizeof(taf_VlanIfSafeRef_t));

LE_REF_DEFINE_STATIC_MAP(vlanIfListRefMap, TAF_NET_MAX_VLAN_ENTRY);

LE_REF_DEFINE_STATIC_MAP(vlanIfSafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

std::map<SlotId, std::list<std::pair<int, int>>> tafVlanMappingCallback::slotVlanMappingInfo;
std::vector<telux::data::VlanConfig> tafVlanCallback::vlanEntryInfo;

le_sem_Ref_t tafVlanMappingCallback::semaphore = nullptr;
le_sem_Ref_t tafVlanCallback::semaphore = nullptr;


/*======================================================================

 FUNCTION        taf_Vlan::Init

 DESCRIPTION     Initialization of the taf Vlan component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Vlan::Init(void)
{

    bool isReady = false;

    // 1. Initiate the semaphore
    tafVlanCallback::semaphore = le_sem_Create("taf_VlanRespCbSem", 0);
    tafVlanMappingCallback::semaphore = le_sem_Create("taf_VlanMappingRespCbSem", 0);


    // 2. Initiate the memory pool

    vlanPool = le_mem_InitStaticPool(vlanPool,
                           TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_Vlan_t));

    interfacePool = le_mem_InitStaticPool(interfacePool,
                           TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_InterfaceConfig_t));

    interfaceIPPool = le_mem_InitStaticPool(interfaceIPPool,
                           TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_InterfaceConfig_t));

    vlanEntryListPool = le_mem_InitStaticPool(vlanEntryListPool,
                               TAF_NET_MAX_VLAN_ENTRY_LIST, sizeof(taf_VlanEntryList_t));

    vlanEntryPool = le_mem_InitStaticPool(vlanEntryPool,
                           TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_VlanEntry_t));

    vlanEntrySafeRefPool = le_mem_InitStaticPool(vlanEntrySafeRefPool,
                                  TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_VlanEntrySafeRef_t));

    vlanIfListPool = le_mem_InitStaticPool(vlanIfListPool,
                               TAF_NET_MAX_VLAN_ENTRY, sizeof(taf_VlanIfList_t));

    vlanIfPool = le_mem_InitStaticPool(vlanIfPool,
                           TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_VlanIf_t));

    vlanIfSafeRefPool = le_mem_InitStaticPool(vlanIfSafeRefPool,
                                  TAF_NET_MAX_VLAN_INTERFACE, sizeof(taf_VlanIfSafeRef_t));

    // 3. Initiate the reference map.

    vlanRefMap = le_ref_InitStaticMap(vlanRefMap, TAF_NET_MAX_VLAN_ENTRY);

    interfaceRefMap = le_ref_InitStaticMap(interfaceRefMap, TAF_NET_MAX_VLAN_INTERFACE);

    interfaceIPRefMap = le_ref_InitStaticMap(interfaceIPRefMap, TAF_NET_MAX_VLAN_INTERFACE);

    vlanEntryListRefMap = le_ref_InitStaticMap(vlanEntryListRefMap, TAF_NET_MAX_VLAN_ENTRY_LIST);

    vlanEntrySafeRefMap = le_ref_InitStaticMap(vlanEntrySafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

    vlanIfListRefMap = le_ref_InitStaticMap(vlanIfListRefMap, TAF_NET_MAX_VLAN_ENTRY);

    vlanIfSafeRefMap = le_ref_InitStaticMap(vlanIfSafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

    // 4. Get the DataFactory and static VlanManager instances
    if (vlanManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto initCb = std::bind(&taf_Vlan::onInitComplete, this, std::placeholders::_1);
        vlanManager = dataFactory.getVlanManager(telux::data::OperationType::DATA_LOCAL,
                            initCb);
#else
        vlanManager = dataFactory.getVlanManager(telux::data::OperationType::DATA_LOCAL);
#endif
    }

    if(vlanManager == nullptr )
    {
        LE_INFO("Vlan manager initialize error...");
        return ;
    }


#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // 6. Check if subsystem status
    std::unique_lock<std::mutex> lck(mMutex);

    telux::common::ServiceStatus subSystemStatus = vlanManager->getServiceStatus();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        LE_INFO("Vlan manager initialize...");
        conVar.wait(lck, [this]{return this->IsSubSystemStatusUpdated;});
        subSystemStatus = vlanManager->getServiceStatus();
    }

    //At this point, initialization should be either AVAILABLE or Failure
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("Vlan Manager initialization failed");
        vlanManager = nullptr;
        return ;
    }
#endif

    isReady = vlanManager->isSubsystemReady();

    if(isReady == false)
    {
        LE_INFO("Vlan component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = vlanManager->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("vlan component is ready...");
    }
    else
    {
        LE_CRIT("unable to init vlan component!");
    }

    // Get the iDataSettingsManager
    if(dataSettingsManager == nullptr)
    {
            auto &dataFactory = telux::data::DataFactory::getInstance();
            isReady = false;
            std::promise<telux::common::ServiceStatus> promSetting;
            dataSettingsManager = dataFactory.getDataSettingsManager(
                telux::data::OperationType::DATA_LOCAL,
                [&](telux::common::ServiceStatus svcStatus)
                {
                    if (svcStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                    {
                        promSetting.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                    }
                    else
                    {
                        promSetting.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                    }
                });

            if (!dataSettingsManager)
            {
                LE_ERROR("Failed to get Data Settings instance.");
            }
            else
            {
                telux::common::ServiceStatus dataSettingsManagerStatus = dataSettingsManager->getServiceStatus();
                if (dataSettingsManagerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_INFO("Data setting subsystem wait to be ready...");
                    std::future<telux::common::ServiceStatus> initFuture = promSetting.get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        OPERATION_DATA_SETTINGS));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL ("Timeout waiting for Data setting susbsytem");
                    }
                    else
                    {
                        dataSettingsManagerStatus = initFuture.get();
                    }
                }
                if (dataSettingsManagerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    LE_INFO("iDataSettingsManager is ready...");
                }
                else
                {
                    LE_FATAL("Fail to init Data Setting Manager subsystem");
                }
            }
        }

    if(isReady)
    {
        LE_INFO("data settings manager is ready...");
    }
    else
    {
        LE_CRIT("unable to init data settings manager!");
    }


    // Add a handler for client session close
    le_msg_AddServiceCloseHandler( taf_net_GetServiceRef(), ClientCloseSessionHandler, NULL );

    return;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetInstance

 DESCRIPTION     Get the instance of Vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      None

 RETURN VALUE    taf_Vlan &

 SIDE EFFECTS

======================================================================*/
taf_Vlan &taf_Vlan::GetInstance()
{
    static taf_Vlan instance;
    return instance;
}

/*======================================================================

 FUNCTION        tafVlanCallback::removeVlanResponse

 DESCRIPTION     Call back function for removing vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafVlanCallback::removeVlanResponse(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafVlan = taf_Vlan::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafVlan.VlanSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafVlanCallback::createVlanResponse

 DESCRIPTION     Call back function for create vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] isAccelerated: Is accelerated.
                 [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafVlanCallback::createVlanResponse(bool isAccelerated, telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafVlan = taf_Vlan::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafVlan.VlanSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafVlanCallback::onVlanListResponse

 DESCRIPTION     Call back function for request vlan entry list.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      [IN] const std::vector<telux::data::net::VlanConfig> &vlanConfigs:
                      The vlan entry list.
                 [IN] telux::common::ErrorCode error: error code.
 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafVlanCallback::onVlanListResponse(const std::vector<telux::data::VlanConfig> &vlanConfigs,
                                       telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> tafVlanCallback --> onVlanListResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
    }

    vlanEntryInfo.assign(vlanConfigs.begin(), vlanConfigs.end());

    le_sem_Post(semaphore);
}

tafVlanMappingCallback::tafVlanMappingCallback(SlotId slot) : slotId(slot) {}

/*======================================================================

 FUNCTION        tafVlanMappingCallback::onResponseCallback

 DESCRIPTION     Call back function for mapping profile.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafVlanMappingCallback::onResponseCallback(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafVlan = taf_Vlan::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafVlan.VlanSyncPromise.set_value(result);
}


/*======================================================================

 FUNCTION        tafVlanMappingCallback::onVlanMappingListResponse

 DESCRIPTION     Call back function for request vlan mapping list.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] const std::list<std::pair<int, int>> &mapping:
                      The vlan mapping list.
                 [IN] telux::common::ErrorCode error: error code.
 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafVlanMappingCallback::onVlanMappingListResponse
(
    const std::list<std::pair<int, int>> &mapping,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> tafVlanCallback --> onVlanMappingListResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
    }

    slotVlanMappingInfo[slotId]=mapping;

    le_sem_Post(semaphore);
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
/*======================================================================

 FUNCTION        taf_Vlan::onInitComplete

 DESCRIPTION     Call back function of vlanManager.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] telux::common::ServiceStatus status : Vlan manager service status.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Vlan::onInitComplete(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IsSubSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif

/*======================================================================

 FUNCTION        taf_Vlan::CreateVlan

 DESCRIPTION     Create a vlan, return a vlan reference.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      [IN] uint16_t vlanId : Vlan id.
                 [IN] bool isAccelerated : Is this vlan accelerated.
                 [IN] le_msg_SessionRef_t sessionRef : Client session reference.

 RETURN VALUE    taf_net_VlanRef_t
                     nullptr:     Failed to create vlan
                                  IsAccelerated conflicts with old value
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_net_VlanRef_t taf_Vlan::CreateVlan
(
    uint16_t vlanId,
    bool isAccelerated,
    le_msg_SessionRef_t sessionRef
)
{
    taf_Vlan_t *vlanPtr=NULL;
    bool isVlanPresentInDb=false;
    bool IsAcceleratedInDb=false;
    uint8_t priority=0;

    TAF_ERROR_IF_RET_VAL(vlanId < MIN_VLAN_ID || vlanId > MAX_VLAN_ID, NULL, "vlan id is invalid");
    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, NULL, "sessionRef is invalid");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(vlanRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        vlanPtr = (taf_Vlan_t*)le_ref_GetValue(iterRef);

        if (vlanPtr != NULL && vlanPtr->vlanId == vlanId)
        {
            if(vlanPtr->sessionRef != sessionRef || vlanPtr->isAccelerated != isAccelerated)
            {
                LE_ERROR("Session or isAccelerated mismatch for vlan id %d", vlanId);
                return NULL;
            }
            else
            {
                LE_DEBUG("found vlan for vlan id %d", vlanId);
                return (taf_net_VlanRef_t)le_ref_GetSafeRef(iterRef);
            }
        }
    }

    LE_DEBUG("not found in map");
    //Check if vlan is present in current db
    isVlanPresentInDb = IsVlanPresentInDb(vlanId,&IsAcceleratedInDb,&priority);

    //vlan is present in db and isAccelerated mismatch, return NULL
    if( isVlanPresentInDb && IsAcceleratedInDb != isAccelerated)
    {
        LE_ERROR("IsAccelerated mismatch %d", vlanId);
        return NULL;
    }
    else
    {
        //vlan is not present in db or vlan is present in db and isAccelerated match
        LE_DEBUG("found vlan in telsdk for vlan id %d", vlanId);
        vlanPtr = (taf_Vlan_t*)le_mem_ForceAlloc(vlanPool);
        vlanPtr->vlanId=vlanId;
        vlanPtr->isAccelerated=isAccelerated;
        vlanPtr->priority=priority;
        vlanPtr->sessionRef=sessionRef;
        //set vlan bind values to default values
        //because for backhaul type WWAN profile/slot are not needed
        vlanPtr->vlanBindConfig.profileId = -1;
        vlanPtr->vlanBindConfig.slotId = DEFAULT_SLOT_ID;
        vlanPtr->vlanBindConfig.vlanIdBackhaul = -1;
        return (taf_net_VlanRef_t)le_ref_CreateRef(vlanRefMap, (void*)vlanPtr);
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::RemoveVlan

 DESCRIPTION     Remove a vlan, return a vlan reference.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      [IN] taf_net_VlanRef_t vlanRef : The reference of VLAN

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to remove vlan
                     LE_NOT_FOUND:     Vlan is not found
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Interface is present in this vlan.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::RemoveVlan
(
    taf_net_VlanRef_t vlanRef
)
{
    taf_Vlan_t *vlanPtr = NULL;
    bool isVlanPresentInDb=false;
    bool IsAcceleratedInDb=false;
    uint8_t priority=0;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "vlan is not present");

    //check if vlan interface is present in this vlan
    isVlanPresentInDb = IsVlanPresentInDb(vlanPtr->vlanId,&IsAcceleratedInDb,&priority);

    TAF_ERROR_IF_RET_VAL(isVlanPresentInDb == true, LE_FAULT, "interface is present in this vlan");

    le_ref_DeleteRef(vlanRefMap, vlanRef);

    le_mem_Release(vlanPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::RemoveInterface

 DESCRIPTION     Remove a interface reference.

 DEPENDENCIES    The initialization of Interface.

 PARAMETERS      [IN] taf_net_InterfaceRef_t interfaceRef : The reference of Interface

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to remove vlan
                     LE_NOT_FOUND:     Interface is not found
                     LE_BAD_PARAMETER: Invalid parameter.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::RemoveInterface
(
    taf_netIpPass_InterfaceRef_t interfaceRef
)
{
    taf_Vlan_t *interfacePtr = NULL;

    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");
    interfacePtr = (taf_Vlan_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    if(interfacePtr != NULL)
    {
      le_ref_DeleteRef(interfaceRefMap, interfaceRef);
      le_mem_Release(interfacePtr);
      return LE_OK;
    }
    // else check in interfaceIPRefMap
    interfacePtr = (taf_Vlan_t*)le_ref_Lookup(interfaceIPRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "interface is not present");
    le_ref_DeleteRef(interfaceIPRefMap, interfaceRef);
    le_mem_Release(interfacePtr);

    return LE_OK;
}


/*======================================================================

 FUNCTION        taf_Vlan::GetVlanRefById

 DESCRIPTION     Search a created vlan.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      None.

 RETURN VALUE    taf_net_VlanRef_t
                     nullptr:     Not found
                     non-nullptr: The reference of a created vlan

 SIDE EFFECTS

======================================================================*/
taf_net_VlanRef_t taf_Vlan::GetVlanRefById
(
    uint16_t vlanId,
    le_msg_SessionRef_t sessionRef
)
{
    taf_Vlan_t* vlanPtr = NULL;
    bool isVlanPresentInDb=false;
    bool IsAcceleratedInDb=false;
    uint8_t priority=0;

    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, NULL, "sessionRef is invalid");
    le_ref_IterRef_t iterRef = le_ref_GetIterator(vlanRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        vlanPtr = (taf_Vlan_t*)le_ref_GetValue(iterRef);

        if (vlanPtr != NULL && vlanPtr->vlanId == vlanId)
        {
            if (vlanPtr->sessionRef == sessionRef)
            {
                LE_DEBUG("found vlan for vlan id %d", vlanId);
                return (taf_net_VlanRef_t)le_ref_GetSafeRef(iterRef);
            }
            else
            {
                LE_DEBUG("session mismatch for vlan id %d", vlanId);
                return NULL;
            }
        }
    }
    LE_DEBUG("not found in map");
    //Get vlan infor from telsdk
    isVlanPresentInDb = IsVlanPresentInDb(vlanId,&IsAcceleratedInDb,&priority);
    //Vlan is not present in Db
    if( !isVlanPresentInDb )
        return NULL;
    //Vlan is present in db,create a vlan in map
    else
    {
        LE_DEBUG("found vlan in telsdk for vlan id %d", vlanId);
        vlanPtr = (taf_Vlan_t*)le_mem_ForceAlloc(vlanPool);
        vlanPtr->vlanId=vlanId;
        vlanPtr->isAccelerated=IsAcceleratedInDb;
        vlanPtr->priority=priority;
        vlanPtr->sessionRef=sessionRef;
        return (taf_net_VlanRef_t)le_ref_CreateRef(vlanRefMap, (void*)vlanPtr);
    }
}

/*======================================================================

 FUNCTION        taf_Vlan::AddVlanInterface

 DESCRIPTION     Add a vlan interface.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Failure.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::AddVlanInterface
(
    taf_net_VlanRef_t vlanRef,
    taf_net_VlanIfType_t ifType
)
{
    le_result_t result;
    taf_Vlan_t *vlanPtr=NULL;
    telux::data::VlanConfig vconfig;
    bool interfacePresent = false;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, LE_FAULT, "vlanManager is null");
    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vconfig.iface = telux::data::InterfaceType(ifType);
    vconfig.vlanId = vlanPtr->vlanId;
    vconfig.isAccelerated = vlanPtr->isAccelerated;
    vconfig.priority = vlanPtr->priority;
    vconfig.nwType = (telux::data::NetworkType)vlanPtr->nwType;

    if (vconfig.nwType != NetworkType::LAN) {
        vconfig.createBridge = false;
    }

    interfacePresent=IsVlanInterfacePresentInDb(vconfig.vlanId, ifType);
    if(interfacePresent)
        return LE_DUPLICATE;

    VlanSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafVlanCallback> addVlanIfCb = std::make_shared<tafVlanCallback>();

    auto  addVlanIfRespCb = std::bind(&tafVlanCallback::createVlanResponse, addVlanIfCb,
                                          std::placeholders::_1, std::placeholders::_2);

    Status status = vlanManager->createVlan(vconfig, addVlanIfRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Add vlan interface timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to add vlan interface, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::SetVlanNetworkType

 DESCRIPTION     Set VLAN network type.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanNetworkType
(
    taf_net_VlanRef_t vlanRef,
    taf_netIpPass_NetworkType_t nwType
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->nwType= nwType;

    return LE_OK;
}


/*======================================================================

 FUNCTION        taf_Vlan::SetVlanPriority

 DESCRIPTION     Set VLAN priority.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanPriority
(
    taf_net_VlanRef_t vlanRef,
    uint8_t priority
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(priority > MAX_VLAN_PRIORITY, LE_OUT_OF_RANGE, "priority is out of range");

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->priority= priority;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::SetVlanBackhaulType

 DESCRIPTION     Set VLAN backhaulType for vlan binding.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanBackhaulType
(
    taf_net_VlanRef_t vlanRef,
    taf_netIpPass_BackhaulType_t bhType
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->vlanBindConfig.backhaulType= bhType;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::SetVlanBackhaulVlanId

 DESCRIPTION     Set VLAN backhaulType vlan id for vlan binding.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanBackhaulVlanId
(
    taf_net_VlanRef_t vlanRef,
    uint16_t vlanId
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->vlanBindConfig.vlanIdBackhaul= vlanId;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::SetVlanBackhaulProfileId

 DESCRIPTION     Set VLAN Profile id for vlan binding.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanBackhaulProfileId
(
    taf_net_VlanRef_t vlanRef,
    uint32_t profileId
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->vlanBindConfig.profileId= profileId;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::SetVlanBackhaulSlotId

 DESCRIPTION     Set VLAN Profile id for vlan binding.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::SetVlanBackhaulSlotId
(
    taf_net_VlanRef_t vlanRef,
    uint8_t slotId
)
{
    taf_Vlan_t *vlanPtr=NULL;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    vlanPtr->vlanBindConfig.slotId= slotId;

    return LE_OK;
}



/*======================================================================

 FUNCTION        taf_Vlan::RemoveVlanInterface

 DESCRIPTION     Remove a vlan interface.

 DEPENDENCIES    The creation of vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_FAULT:         Failure.
                     LE_NOT_FOUND:     Vlan is not present.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::RemoveVlanInterface
(
    taf_net_VlanRef_t vlanRef,
    taf_net_VlanIfType_t ifType
)
{
    le_result_t result;
    taf_Vlan_t *vlanPtr=NULL;
    telux::data::InterfaceType interfaceType;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, LE_FAULT, "vlanManager is null");
    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    interfaceType=(telux::data::InterfaceType) ifType;

    VlanSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafVlanCallback> removeVlanIfCb = std::make_shared<tafVlanCallback>();

    auto  removeVlanIfRespCb = std::bind(&tafVlanCallback::removeVlanResponse,
                                             removeVlanIfCb, std::placeholders::_1);

    Status status = vlanManager->removeVlan(vlanPtr->vlanId, interfaceType,
                                                  removeVlanIfRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Remove vlan interface timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to remove vlan interface, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::IsVlanPresentInDb

 DESCRIPTION     Is vlan present in telsdk.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      None.

 RETURN VALUE    bool
                     nullptr:     Failure
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
bool taf_Vlan::IsVlanPresentInDb(uint16_t vlanId, bool *isAccelerated, uint8_t *priority)
{
    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, false, "vlanManager is null");
    TAF_ERROR_IF_RET_VAL(isAccelerated == NULL, false, "isAccelerated is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    telux::common::Status status = vlanManager->queryVlanInfo(tafVlanCallback::onVlanListResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafVlanCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                   std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafVlanCallback::vlanEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan");
            return false;
        }

        for (auto info : tafVlanCallback::vlanEntryInfo)
        {
            //found vlan info
            if(vlanId == info.vlanId)
            {
                *isAccelerated=info.isAccelerated;
                *priority=info.priority;
                return true;
            }
        }
    }
    else
    {
        LE_ERROR("Request vlan info failed, status: %d",int(status));
        return false;
    }
    return false;
}

/*======================================================================

 FUNCTION        taf_Vlan::IsVlanInterfacePresentInDb

 DESCRIPTION     Is vlan interface already added.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      None.

 RETURN VALUE    bool
                     nullptr:     Failure
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
bool taf_Vlan::IsVlanInterfacePresentInDb(uint16_t vlanId, taf_net_VlanIfType_t ifType)
{
    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, false, "vlanManager is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    telux::common::Status status = vlanManager->queryVlanInfo(tafVlanCallback::onVlanListResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafVlanCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                   std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafVlanCallback::vlanEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan");
            return false;
        }

        for (auto info : tafVlanCallback::vlanEntryInfo)
        {
            //found vlan info
            if(vlanId == info.vlanId && ifType == (taf_net_VlanIfType_t)info.iface)
            {
                return true;
            }
        }
    }
    else
    {
        LE_ERROR("Request vlan info failed, status: %d",int(status));
        return false;
    }
    return false;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanEntryList

 DESCRIPTION     Get a reference of an vlan entry list.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      None.

 RETURN VALUE    taf_net_VlanEntryListRef_t
                     nullptr:     Failure
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_net_VlanEntryListRef_t taf_Vlan::GetVlanEntryList()
{
    le_ref_IterRef_t iterRef;
    uint16_t previous_vlanId=0;
    uint8_t slotId;
    uint32_t profileId;

    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, NULL, "vlanManager is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    telux::common::Status status = vlanManager->queryVlanInfo(tafVlanCallback::onVlanListResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafVlanCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                   std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafVlanCallback::vlanEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan entry");
            return NULL;
        }

        TAF_ERROR_IF_RET_VAL(vlanEntryListRefMap == NULL, NULL, "vlanEntryListRefMap is null");

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(vlanEntryListRefMap);

        if(iterRef != NULL && le_ref_GetValue(iterRef) != NULL
                           && le_ref_GetSafeRef(iterRef) != NULL)
            CleanListRef((taf_net_VlanEntryListRef_t)le_ref_GetSafeRef(iterRef));

        taf_VlanEntryList_t* vlanEntriesList =
                                       (taf_VlanEntryList_t*)le_mem_ForceAlloc(vlanEntryListPool);
        vlanEntriesList->vlanEntryList = LE_SLS_LIST_INIT;
        vlanEntriesList->safeRefList = LE_SLS_LIST_INIT;
        vlanEntriesList->currPtr = NULL;

        taf_VlanEntry_t* vlanEntryPtr;
        //sort vlan entry by vlan id
        std::sort(tafVlanCallback::vlanEntryInfo.begin(),tafVlanCallback::vlanEntryInfo.end(),
                 sort_vlanId);

        for (auto info : tafVlanCallback::vlanEntryInfo)
        {
            //queue one item for same vlan id
            if(previous_vlanId != info.vlanId)
            {
                vlanEntryPtr = (taf_VlanEntry_t*)le_mem_ForceAlloc(vlanEntryPool);
                vlanEntryPtr->info.vlanId=info.vlanId;
                if(info.vlanId != 0)
                {
                    if(GetBoundSlotIdProfileIdFromVlan(info.vlanId, &slotId, &profileId) == LE_OK)
                    {
                        vlanEntryPtr->info.slotId = slotId;
                        vlanEntryPtr->info.profileId = profileId;
                    }
                    else
                    {
                        vlanEntryPtr->info.slotId = 0;
                        vlanEntryPtr->info.profileId = -1;
                    }
                }
                else
                {
                    vlanEntryPtr->info.slotId = 0;
                    vlanEntryPtr->info.profileId = -1;
                }

                vlanEntryPtr->info.isAccelerated=info.isAccelerated;
                vlanEntryPtr->info.nwType= (taf_netIpPass_NetworkType_t)info.nwType;
                vlanEntryPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(vlanEntriesList->vlanEntryList), &(vlanEntryPtr->link));
                previous_vlanId = info.vlanId;
            }
        }

        return (taf_net_VlanEntryListRef_t)le_ref_CreateRef(vlanEntryListRefMap,
                                                            (void*)vlanEntriesList);

    }
    else
    {
        LE_ERROR("Request vlan entry list failed, status: %d",int(status));
        return NULL;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::GetFirstVlanEntry

 DESCRIPTION     Get the reference of the first vlan entry from a list.

 DEPENDENCIES    Initialization of a vlan entry list

 PARAMETERS      [IN] taf_net_VlanEntryListRef_t vlanEntryListRef: The vlan entry list reference.

 RETURN VALUE    taf_net_VlanEntryRef_t
                     nullptr:     Failure
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_VlanEntryRef_t taf_Vlan::GetFirstVlanEntry
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{
    taf_VlanEntryList_t* listPtr = (taf_VlanEntryList_t*)le_ref_Lookup(vlanEntryListRefMap,
        vlanEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", vlanEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->vlanEntryList));
    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_VlanEntry_t* vlanEntryPtr = CONTAINER_OF(linkPtr, taf_VlanEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_VlanEntrySafeRef_t* safeRefPtr =
                           (taf_VlanEntrySafeRef_t*)le_mem_ForceAlloc(vlanEntrySafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(vlanEntrySafeRefMap, (void*)vlanEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_VlanEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetNextVlanEntry

 DESCRIPTION     Get the reference of the next vlan entry from a list.

 DEPENDENCIES    Initialization of an vlan entry list

 PARAMETERS      [IN] taf_net_VlanEntryListRef_t vlanEntryListRef: The vlan entry list reference.

 RETURN VALUE    taf_net_VlanEntryRef_t
                     nullptr:     Failure
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_VlanEntryRef_t taf_Vlan::GetNextVlanEntry
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{
    taf_VlanEntryList_t* listPtr = (taf_VlanEntryList_t*)le_ref_Lookup(vlanEntryListRefMap,
        vlanEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", vlanEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->vlanEntryList), listPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_VlanEntry_t* vlanEntryPtr = CONTAINER_OF(linkPtr, taf_VlanEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_VlanEntrySafeRef_t* safeRefPtr =
                           (taf_VlanEntrySafeRef_t*)le_mem_ForceAlloc(vlanEntrySafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(vlanEntrySafeRefMap, (void*)vlanEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_VlanEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Vlan::DeleteVlanEntryList

 DESCRIPTION     Delete a reference of a vlan entry list.

 DEPENDENCIES    Initialization of a vlan entry list

 PARAMETERS      [IN] taf_net_VlanEntryListRef_t vlanEntryListRef: The vlan entry list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Failure.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::DeleteVlanEntryList
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{

    TAF_ERROR_IF_RET_VAL(vlanEntryListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(vlanEntryListRef)");

    return CleanListRef(vlanEntryListRef);
}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanId

 DESCRIPTION     Get the vlan id from a reference.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    int16_t
                     vlan id

 SIDE EFFECTS

======================================================================*/
int16_t taf_Vlan::GetVlanId
(
    taf_net_VlanEntryRef_t vlanEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(vlanEntryRef == NULL, -1,
        "Null reference(vlanEntryRef)");

    taf_VlanEntry_t* vlanEntryPtr = (taf_VlanEntry_t*)le_ref_Lookup(vlanEntrySafeRefMap,
                                                                    vlanEntryRef);
    TAF_ERROR_IF_RET_VAL(vlanEntryPtr == NULL, -1, "Invalid para(null reference ptr)");

    return vlanEntryPtr->info.vlanId;

}

/*======================================================================

 FUNCTION        taf_Vlan::IsVlanAccelerated

 DESCRIPTION     Get the vlan id from a reference.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded.
                     LE_NOT_FOUND:     Vlan is not found
                     LE_BAD_PARAMETER: Invalid parameter.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::IsVlanAccelerated
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    bool* isAcceleratedPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanEntryRef == NULL, LE_BAD_PARAMETER,
        "Null reference(vlanEntryRef)");

    TAF_ERROR_IF_RET_VAL(isAcceleratedPtr == NULL, LE_BAD_PARAMETER,
        "Null Ptr(isAcceleratedPtr)");

    taf_VlanEntry_t* vlanEntryPtr = (taf_VlanEntry_t*)le_ref_Lookup(vlanEntrySafeRefMap,
                                                                    vlanEntryRef);
    TAF_ERROR_IF_RET_VAL(vlanEntryPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *isAcceleratedPtr=vlanEntryPtr->info.isAccelerated;

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanNetworkType

 DESCRIPTION     Get the vlan network type from a reference.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded.
                     LE_NOT_FOUND:     Vlan is not found
                     LE_BAD_PARAMETER: Invalid parameter.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::GetVlanNetworkType
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    taf_netIpPass_NetworkType_t  *networkType
)
{
    TAF_ERROR_IF_RET_VAL(vlanEntryRef == NULL, LE_BAD_PARAMETER,
        "Null reference(vlanEntryRef)");

    TAF_ERROR_IF_RET_VAL(networkType == NULL, LE_BAD_PARAMETER,
        "Null Ptr(GetVlanNetworkType)");

    taf_VlanEntry_t* vlanEntryPtr = (taf_VlanEntry_t*)le_ref_Lookup(vlanEntrySafeRefMap,
                                                                    vlanEntryRef);
    TAF_ERROR_IF_RET_VAL(vlanEntryPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *networkType=vlanEntryPtr->info.nwType;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanProfileId

 DESCRIPTION     Get profile Id binding with the VLAN.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    int16_t
                     profile id

 SIDE EFFECTS

======================================================================*/
int32_t taf_Vlan::GetVlanProfileId
(
    taf_net_VlanEntryRef_t vlanEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(vlanEntryRef == NULL, -1,
        "Null reference(vlanEntryRef)");

    taf_VlanEntry_t* vlanEntryPtr = (taf_VlanEntry_t*)le_ref_Lookup(vlanEntrySafeRefMap,
                                                                    vlanEntryRef);
    TAF_ERROR_IF_RET_VAL(vlanEntryPtr == NULL, -1, "Invalid para(null reference ptr)");

    return vlanEntryPtr->info.profileId;

}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanPhoneId

 DESCRIPTION     Get phone Id binding with the VLAN.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to get phone Id.
                     LE_NOT_FOUND:     Vlan is not found
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Failed to get phone Id.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::GetVlanPhoneId
(
    taf_net_VlanEntryRef_t vlanEntryRef,
    uint8_t* phoneIdPtr
)
{
    le_result_t result;
    auto &network = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_VAL(vlanEntryRef == NULL, LE_BAD_PARAMETER, "Null reference(vlanEntryRef)");
    TAF_ERROR_IF_RET_VAL(phoneIdPtr == NULL, LE_BAD_PARAMETER, "Null reference(phoneIdPtr)");

    taf_VlanEntry_t* vlanEntryPtr = (taf_VlanEntry_t*)le_ref_Lookup(vlanEntrySafeRefMap,
                                                                    vlanEntryRef);
    TAF_ERROR_IF_RET_VAL(vlanEntryPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    result = network.getPhoneIdFromSlotId(vlanEntryPtr->info.slotId, phoneIdPtr);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "failed to get phone id from slot id");

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_Vlan::CleanListRef

 DESCRIPTION     Clean the vlan entry list stored in the map.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanEntryListRef_t vlanEntryListRef: The vlan entry list reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::CleanListRef
(
    taf_net_VlanEntryListRef_t vlanEntryListRef
)
{
    taf_VlanEntry_t* vlanEntryPtr;
    taf_VlanEntrySafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(vlanEntryListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(vlanEntryListRef)");

    taf_VlanEntryList_t* listPtr = (taf_VlanEntryList_t*)le_ref_Lookup(vlanEntryListRefMap,
        vlanEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(listPtr->vlanEntryList))) != NULL)
    {
        vlanEntryPtr = CONTAINER_OF(linkPtr, taf_VlanEntry_t, link);
        le_mem_Release(vlanEntryPtr);
    }

    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_VlanEntrySafeRef_t, link);
        le_ref_DeleteRef(vlanEntrySafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(vlanEntryListRefMap, vlanEntryListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}
/*======================================================================

 FUNCTION        taf_Vlan::GetVlanInterfaceList

 DESCRIPTION     Get a reference of a vlan interface list.

 DEPENDENCIES    The initialization of vlan.

 PARAMETERS      vlanRef.

 RETURN VALUE    taf_net_VlanIfListRef_t
                     nullptr:     Failure
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_net_VlanIfListRef_t taf_Vlan::GetVlanInterfaceList
(
    taf_net_VlanRef_t vlanRef
)
{
    le_ref_IterRef_t iterRef;
    bool isAdded = false;
    uint16_t vlanId;
    taf_VlanIfList_t* existedVlanIfList;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , NULL, "vlanRef is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    telux::common::Status status = vlanManager->queryVlanInfo(tafVlanCallback::onVlanListResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafVlanCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                   std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafVlanCallback::vlanEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan entry");
            return NULL;
        }

        taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
        TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, NULL, "Invalid para(null reference ptr)");
        vlanId = vlanPtr->vlanId;

        TAF_ERROR_IF_RET_VAL(vlanIfListRefMap == NULL , NULL, "vlanIfListRefMap is null");

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(vlanIfListRefMap);

        TAF_ERROR_IF_RET_VAL(iterRef == NULL , NULL, "iterRef is null");

        while (!isAdded && (iterRef != NULL) && (le_ref_NextNode(iterRef) == LE_OK))
        {
            existedVlanIfList = (taf_VlanIfList_t*) le_ref_GetValue(iterRef);
            TAF_ERROR_IF_RET_VAL(existedVlanIfList == NULL, NULL, "Vlan if list is NULL)");

            if (existedVlanIfList->vlanId == vlanId)
            {
                isAdded = true;
                break;
            }
        }

        // if the vlan interface list already exists in map,update it
        if(isAdded && iterRef != NULL && le_ref_GetSafeRef(iterRef) != NULL)
        {
            CleanVlanInterfaceListRef((taf_net_VlanIfListRef_t)le_ref_GetSafeRef(iterRef));
        }

        taf_VlanIfList_t* vlanIfsList =
                                        (taf_VlanIfList_t*)le_mem_ForceAlloc(vlanIfListPool);
        vlanIfsList->vlanIfList = LE_SLS_LIST_INIT;
        vlanIfsList->safeRefList = LE_SLS_LIST_INIT;
        vlanIfsList->vlanId = vlanId;
        vlanIfsList->currPtr = NULL;

        taf_VlanIf_t* vlanIfPtr;

        for (auto info : tafVlanCallback::vlanEntryInfo)
        {
            //queue one item for same vlan id
            if(vlanId == info.vlanId)
            {
                vlanIfPtr = (taf_VlanIf_t*)le_mem_ForceAlloc(vlanIfPool);
                vlanIfPtr->interface=(taf_net_VlanIfType_t)info.iface;
                vlanIfPtr->priority=info.priority;
                vlanIfPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(vlanIfsList->vlanIfList), &(vlanIfPtr->link));
            }
        }

        return (taf_net_VlanIfListRef_t)le_ref_CreateRef(vlanIfListRefMap,
                                                           (void*)vlanIfsList);

    }
    else
    {
        LE_ERROR("Request vlan interface list failed, status: %d",int(status));
        return NULL;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::GetFirstVlanInterface

 DESCRIPTION     Get the reference of the first vlan interface from a list.

 DEPENDENCIES    Initialization of a vlan interface list

 PARAMETERS      [IN] taf_net_VlanIfListRef_t vlanIfListRef:
                                                      The vlan interface list reference.

 RETURN VALUE    taf_net_VlanIfRef_t
                     nullptr:     Failure
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_VlanIfRef_t taf_Vlan::GetFirstVlanInterface
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    taf_VlanIfList_t* listPtr = (taf_VlanIfList_t*)le_ref_Lookup(vlanIfListRefMap,
        vlanIfListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", vlanIfListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->vlanIfList));
    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_VlanIf_t* vlanIfPtr = CONTAINER_OF(linkPtr, taf_VlanIf_t , link);
    listPtr->currPtr = linkPtr;

    taf_VlanIfSafeRef_t* safeRefPtr =
                                  (taf_VlanIfSafeRef_t*)le_mem_ForceAlloc(vlanIfSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(vlanIfSafeRefMap, (void*)vlanIfPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_VlanIfRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetNextVlanInterface

 DESCRIPTION     Get the reference of the next vlan interface from a list.

 DEPENDENCIES    Initialization of a vlan interface list

 PARAMETERS      [IN] taf_net_VlanIfListRef_t vlanIfListRef:
                                                           The vlan interface list reference.

 RETURN VALUE    taf_net_VlanIfRef_t
                     nullptr:     Failure
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_VlanIfRef_t  taf_Vlan::GetNextVlanInterface
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    taf_VlanIfList_t* listPtr = (taf_VlanIfList_t*)le_ref_Lookup(vlanIfListRefMap,
        vlanIfListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", vlanIfListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->vlanIfList), listPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_VlanIf_t* vlanIfPtr = CONTAINER_OF(linkPtr, taf_VlanIf_t , link);
    listPtr->currPtr = linkPtr;

    taf_VlanIfSafeRef_t* safeRefPtr =
                                  (taf_VlanIfSafeRef_t*)le_mem_ForceAlloc(vlanIfSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(vlanIfSafeRefMap, (void*)vlanIfPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_VlanIfRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Vlan::DeleteVlanInterfaceList

 DESCRIPTION     Delete a reference of a vlan interface list.

 DEPENDENCIES    Initialization of a vlan interface list

 PARAMETERS      [IN] taf_net_VlanIfListRef_t vlanIfListRef:
                                                                The vlan interface list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Failure.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::DeleteVlanInterfaceList
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    TAF_ERROR_IF_RET_VAL(vlanIfListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(vlanIfListRef)");

    return CleanVlanInterfaceListRef(vlanIfListRef);
}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanInterfaceType

 DESCRIPTION     Get the vlan interface type from a reference.

 DEPENDENCIES    Initialization of a vlan interface list and get a safe reference of a vlan
                 interface.

 PARAMETERS      [IN] taf_net_VlanIfRef_t vlanIfRef : The vlan interface reference.

 RETURN VALUE    taf_net_VlanIfType_t
                     interface type

 SIDE EFFECTS

======================================================================*/
taf_net_VlanIfType_t taf_Vlan::GetVlanInterfaceType
(
    taf_net_VlanIfRef_t vlanIfRef
)
{
    TAF_ERROR_IF_RET_VAL(vlanIfRef == NULL, TAF_NET_IFACE_UNKNOWN,
        "Null reference(vlanIfRef)");

    taf_VlanIf_t* vlanIfPtr = (taf_VlanIf_t*)le_ref_Lookup(vlanIfSafeRefMap,
                                                                    vlanIfRef);
    TAF_ERROR_IF_RET_VAL(vlanIfPtr == NULL, TAF_NET_IFACE_UNKNOWN,
                         "Invalid para(null reference ptr)");

    return vlanIfPtr->interface;

}

/*======================================================================

 FUNCTION        taf_Vlan::GetVlanPriority

 DESCRIPTION     Get the vlan priority from a reference.

 DEPENDENCIES    Initialization of a vlan interface list and get a safe reference of a vlan
                 interface.

 PARAMETERS      [IN] taf_net_VlanIfRef_t vlanIfRef : The vlan interface reference.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded.
                     LE_NOT_FOUND:     Vlan is not found
                     LE_BAD_PARAMETER: Invalid parameter.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::GetVlanPriority(taf_net_VlanIfRef_t vlanIfRef, uint8_t* priority)
{
    TAF_ERROR_IF_RET_VAL(vlanIfRef == NULL, LE_BAD_PARAMETER, "Null reference(vlanIfRef)");
    TAF_ERROR_IF_RET_VAL(priority == NULL, LE_BAD_PARAMETER, "Null reference(priority)");

    taf_VlanIf_t* vlanIfPtr = (taf_VlanIf_t*)le_ref_Lookup(vlanIfSafeRefMap, vlanIfRef);

    TAF_ERROR_IF_RET_VAL(vlanIfPtr == NULL, LE_NOT_FOUND, "Invalid para(null vlanIfPtr)");

    *priority = vlanIfPtr->priority;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Vlan::CleanVlanInterfaceListRef

 DESCRIPTION     Clean the vlan interface list stored in the map.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanIfListRef_t vlanIfListRef:
                                                              The vlan interface list reference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::CleanVlanInterfaceListRef
(
    taf_net_VlanIfListRef_t vlanIfListRef
)
{
    taf_VlanIf_t* vlanIfPtr;
    taf_VlanIfSafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(vlanIfListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(vlanIfListRef)");

    taf_VlanIfList_t* listPtr = (taf_VlanIfList_t*)le_ref_Lookup(vlanIfListRefMap,
        vlanIfListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(listPtr->vlanIfList))) != NULL)
    {
        vlanIfPtr = CONTAINER_OF(linkPtr, taf_VlanIf_t, link);
        le_mem_Release(vlanIfPtr);
    }

    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_VlanIfSafeRef_t, link);
        le_ref_DeleteRef(vlanIfSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(vlanIfListRefMap, vlanIfListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}
/*======================================================================

 FUNCTION        taf_Vlan::BindVlanWithProfile

 DESCRIPTION     Bind a VLAN with a particular profile ID by invoking telsdk API.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanRef_t vlanRef: The reference of vlan.
                 [IN] uint8_t slotid: The slot id
                 [IN] uint32_t profileId: The profile id

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_NOT_FOUND:     Vlan not found
                     LE_FAULT:         Failure.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::BindVlanWithProfile(taf_net_VlanRef_t vlanRef, uint8_t slotId, uint32_t profileId)
{
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    SlotId slot = (SlotId)slotId;
#endif

    le_result_t result;
    uint16_t vlanId=0;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

   // fix telsdk bug:when the profile is already bound with VLAN,bindWithProfile api from telsdk
   // always return OK
    vlanId=GetBoundVlanIdFromSlotAndProfile(slotId, profileId);
    if(vlanId !=0)
    {
        LE_ERROR("Profile is already bound with vlan");
        return LE_FAULT;
    }

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Vlan not found");
    vlanId = vlanPtr->vlanId;

    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                                   std::make_shared<tafVlanMappingCallback>(slot);

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = vlanManager->bindWithProfile(profileId, vlanId, bindVlanWithProfileRespCb,slot);
#else
    Status status = vlanManager->bindWithProfile(profileId, vlanId, bindVlanWithProfileRespCb);
#endif
    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Bind vlan with profile timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to bind vlan with profile, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::BindVlanWithBackhaul

 DESCRIPTION     Bind a VLAN with a particular profile ID by invoking telsdk API.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanRef_t vlanRef: The reference of vlan.
                 [IN] uint8_t slotid: The slot id
                 [IN] uint32_t profileId: The profile id

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_NOT_FOUND:     Vlan not found
                     LE_FAULT:         Failure.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::BindVlanWithBackhaul(taf_net_VlanRef_t vlanRef)
{

    le_result_t result;
    uint16_t vlanId=0;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Vlan not found");

    SlotId slot = (SlotId)vlanPtr->vlanBindConfig.slotId;
    telux::data::net::VlanBindConfig vlanBind = {};

    if(vlanPtr->vlanBindConfig.backhaulType == TAF_NETIPPASS_BH_WWAN)
    {
        vlanId=GetBoundVlanIdFromSlotAndProfile(vlanPtr->vlanBindConfig.slotId,
                                                vlanPtr->vlanBindConfig.profileId);
        if(vlanId !=0)
        {
           LE_ERROR("Profile is already bound with vlan");
           return LE_FAULT;
        }
        vlanBind.bhInfo.slotId = (SlotId)vlanPtr->vlanBindConfig.slotId;
        vlanBind.bhInfo.profileId = vlanPtr->vlanBindConfig.profileId;
    }
    else // for ETH and rest where SIM does not exist.
    {
        uint16_t bhvlanid = GetBackhaulVlanIdBoundWithVlan(
                                              vlanId, vlanPtr->vlanBindConfig.backhaulType, slot);

        LE_INFO("bindVlanFromBackhaul: bhvlanid %d", bhvlanid);
        TAF_ERROR_IF_RET_VAL(bhvlanid != 0, LE_FAULT, "vlan id is already bound to the backhaul");
        vlanBind.bhInfo.vlanId = vlanPtr->vlanBindConfig.vlanIdBackhaul;
    }

    vlanId = vlanPtr->vlanId;
    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                                   std::make_shared<tafVlanMappingCallback>(slot);

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);

    vlanBind.vlanId = vlanId;

    switch (vlanPtr->vlanBindConfig.backhaulType)
    {
        case TAF_NETIPPASS_BH_ETH:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::ETH;
            break;
        case TAF_NETIPPASS_BH_USB:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::USB;
            break;
        case TAF_NETIPPASS_BH_WLAN:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::WLAN;
            break;
        case TAF_NETIPPASS_BH_WWAN:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
            break;
        case TAF_NETIPPASS_BH_BLE:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::BLE;
            break;
        default:
            LE_ERROR("Invalid backhaul type (%d).", vlanPtr->vlanBindConfig.backhaulType);
            return LE_BAD_PARAMETER;
    }

    Status status = vlanManager->bindToBackhaul(vlanBind, bindVlanWithProfileRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Bind vlan with backhaul timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to bind vlan with backhaul, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::UnbindVlanFromProfile

 DESCRIPTION     Unbind a VLAN from a particular profile ID by invoking telsdk API.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanRef_t vlanRef: The reference of vlan.

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_NOT_FOUND:     Vlan not found
                     LE_FAULT:         Failuree.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::UnbindVlanFromProfile(taf_net_VlanRef_t vlanRef)
{
    le_result_t result;
    uint16_t vlanId=0;
    uint32_t profileId=0;
    uint8_t slotId=0;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    vlanId = vlanPtr->vlanId;

    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_FAULT, "Invalid vlan id");

    result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting slotId and profileId failed");

    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                          std::make_shared<tafVlanMappingCallback>((SlotId)slotId);

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = vlanManager->unbindFromProfile(profileId, vlanId,
                                                   bindVlanWithProfileRespCb, (SlotId)slotId);
#else
    Status status = vlanManager->unbindFromProfile(profileId, vlanId, bindVlanWithProfileRespCb);
#endif
    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Unbind vlan from profile timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR("ERROR - Failed to unbind vlan from profile, Status:%d ",
                  static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::UnbindVlanFromBackhaul

 DESCRIPTION     Unbind a VLAN from a particular profile ID by invoking telsdk API.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] taf_net_VlanRef_t vlanRef: The reference of vlan.

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_NOT_FOUND:     Vlan not found
                     LE_FAULT:         Failuree.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::UnbindVlanFromBackhaul(taf_net_VlanRef_t vlanRef)
{
    le_result_t result;
    uint16_t vlanId=0;
    uint32_t profileId=0;
    uint8_t slotId=0;
    taf_netIpPass_BackhaulType_t backhaulType;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    vlanId = vlanPtr->vlanId;
    backhaulType = vlanPtr->vlanBindConfig.backhaulType;
    SlotId slot = (SlotId)vlanPtr->vlanBindConfig.slotId;
    telux::data::net::VlanBindConfig vlanBind = {};

    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_FAULT, "Invalid vlan id");

    if(backhaulType == TAF_NETIPPASS_BH_WWAN)
    {
        result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting slotId and profileId failed");
        vlanBind.bhInfo.slotId = (SlotId)slotId;
        vlanBind.bhInfo.profileId = profileId;
        slot = (SlotId)slotId;
    }
    else // for ETH and rest where SIM does not exist.
    {
        uint16_t bhvlanid = GetBackhaulVlanIdBoundWithVlan(vlanId,backhaulType, slot);
        LE_INFO("UnbindVlanFromBackhaul: bhvlanid %d", bhvlanid);
        TAF_ERROR_IF_RET_VAL(bhvlanid == 0, LE_FAULT, "vlan id is not bound to the backhaul");
        vlanBind.bhInfo.vlanId = bhvlanid;
    }

    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                 std::make_shared<tafVlanMappingCallback>(slot);

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);
    vlanBind.vlanId = vlanId;

    switch (backhaulType)
    {
        case TAF_NETIPPASS_BH_ETH:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::ETH;
            break;
        case TAF_NETIPPASS_BH_USB:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::USB;
            break;
        case TAF_NETIPPASS_BH_WLAN:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::WLAN;
            break;
        case TAF_NETIPPASS_BH_WWAN:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
            break;
        case TAF_NETIPPASS_BH_BLE:
            vlanBind.bhInfo.backhaul = telux::data::BackhaulType::BLE;
            break;
        default:
            LE_ERROR("Invalid backhaul type (%d).", backhaulType);
            return LE_BAD_PARAMETER;
    }

    Status status = vlanManager->unbindFromBackhaul(vlanBind,bindVlanWithProfileRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = VlanSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Unbind vlan from profile timeout for %d seconds", OPERATION_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR("ERROR - Failed to unbind vlan from profile, Status:%d ",
                  static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_Vlan::GetBackhaulVlanIdBoundWithVlan

 DESCRIPTION     Get the bound backhaul id with vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint16_t vlanId: The vlan Id.
 PARAMETERS      [IN] taf_netIpPass_BackhaulType_t backhaulType: The backhaul type.
 PARAMETERS      [IN] SlotId slot: The slot Id.

 RETURN VALUE    uint16_t
                    0:       no backhaul binding with this vlan.
                    others:  backhaul id.

 SIDE EFFECTS

======================================================================*/
uint16_t taf_Vlan::GetBackhaulVlanIdBoundWithVlan(uint16_t vlanId,
                                      taf_netIpPass_BackhaulType_t backhaulType,
                                      SlotId slot)
{
    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, 0, "vlanManager is null");

    std::promise<telux::common::ErrorCode> p;
    std::promise<const std::vector<telux::data::net::VlanBindConfig>> q;

    telux::data::net::VlanBindingsResponseCb cb =
            [&p, &q](const std::vector<telux::data::net::VlanBindConfig> bindings,
                     telux::common::ErrorCode error) {
                p.set_value(error);
                q.set_value(bindings);
            };

    telux::common::Status status = vlanManager->queryVlanToBackhaulBindings(
                                               (telux::data::BackhaulType) backhaulType, cb, slot);

    if (status == Status::SUCCESS)
    {
        LE_INFO("GetBackhaulVlanIdBoundWithVlan request status: %d", (int) status);
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("GetBackhaulVlanIdBoundWithVlan response error code: %d", (int) error);
        if (error == ErrorCode::SUCCESS)
        {
            const std::vector<telux::data::net::VlanBindConfig> vlanbindings = q.get_future().get();
            LE_INFO("Size of vector VlanBindConfig: %d", (int) vlanbindings.size());
            for (auto binding:vlanbindings)
            {
                LE_INFO("binding.vlanId: %d, binding.bhInfo.vlanId: %d",
                                                          binding.vlanId,
                                                          binding.bhInfo.vlanId);
                if (binding.vlanId == vlanId) {
                    if (binding.bhInfo.vlanId > 0) {
                        return binding.bhInfo.vlanId;
                    }
                }
            }
        }
    }

    return 0;
}


/*======================================================================

 FUNCTION        taf_Vlan::GetBoundVlanIdFromSlotAndProfile

 DESCRIPTION     Get the bound vlan id from profile.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint32_t profileId: The profile Id.

 RETURN VALUE    uint16_t
                      0:      no vlan binding with this profile.
                      others:  vlan id

 SIDE EFFECTS

======================================================================*/
uint16_t taf_Vlan::GetBoundVlanIdFromSlotAndProfile(uint8_t slotId, uint32_t profileId)
{

    if(GetBindingInfo(slotId) != LE_OK)
        return 0;

    if (tafVlanMappingCallback::slotVlanMappingInfo.find((SlotId)slotId) ==
        tafVlanMappingCallback::slotVlanMappingInfo.end() ||
        tafVlanMappingCallback::slotVlanMappingInfo[(SlotId)slotId].size() == 0)
    {
        LE_DEBUG("no binding info for this profile");
        return 0;
    }

    for (auto info : tafVlanMappingCallback::slotVlanMappingInfo[(SlotId)slotId])
    {
            if((uint32_t)info.first == profileId)
            {
                LE_DEBUG("the profile %d is bound with vlan %d",profileId,(uint16_t)info.second);
                return (uint16_t)info.second;
            }
    }

    return 0;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetBoundSlotIdProfileIdFromVlan

 DESCRIPTION     Get the bound profile if from vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint32_t profileId: The profile Id.

 RETURN VALUE    int32_t
                      -1:      no profile binding with this vlan.
                      others:  profile id

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::GetBoundSlotIdProfileIdFromVlan(uint16_t vlanId, uint8_t* slotId, uint32_t* profileId)
{

    TAF_ERROR_IF_RET_VAL(slotId == NULL , LE_BAD_PARAMETER, "slotId ptr is null");
    TAF_ERROR_IF_RET_VAL(profileId == NULL , LE_BAD_PARAMETER, "profileId ptr is null");

    for(int slotIdIdx =1; slotIdIdx <= 2; slotIdIdx++)
    {

        if(GetBindingInfo(slotIdIdx) != LE_OK)
            continue;

        if (tafVlanMappingCallback::slotVlanMappingInfo.find((SlotId)slotIdIdx) ==
            tafVlanMappingCallback::slotVlanMappingInfo.end() ||
            tafVlanMappingCallback::slotVlanMappingInfo[(SlotId)slotIdIdx].size() == 0)
            continue;

        for (auto info : tafVlanMappingCallback::slotVlanMappingInfo[(SlotId)slotIdIdx])
        {
            if((uint32_t)info.second == vlanId)
            {
                LE_DEBUG("the vlan %d is bound with profile %d",vlanId,(int32_t)info.first);
                *slotId = slotIdIdx;
                *profileId = info.first;
                return LE_OK;
            }
        }
    }

    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetBindingInfo

 DESCRIPTION     Get the binding info between vlan id and profile id.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                      LE_FAULT     Failure.
                      LE_OK        Success

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::GetBindingInfo(uint8_t slotId)
{
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    SlotId slot = SlotId(slotId);
#endif
    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, LE_FAULT, "vlanManager is null");
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)


    std::shared_ptr<tafVlanMappingCallback> vlanMappingCb =
                                                     std::make_shared<tafVlanMappingCallback>(slot);

    auto  vlanMappingRespCb = std::bind(&tafVlanMappingCallback::onVlanMappingListResponse,
                                       vlanMappingCb, std::placeholders::_1, std::placeholders::_2);

    telux::common::Status status = vlanManager->queryVlanMappingList(vlanMappingRespCb, slot);
#else

    std::shared_ptr<tafVlanMappingCallback> vlanMappingCb =
                                                     std::make_shared<tafVlanMappingCallback>(1);

    auto  vlanMappingRespCb = std::bind(&tafVlanMappingCallback::onVlanMappingListResponse,
                                       vlanMappingCb, std::placeholders::_1, std::placeholders::_2);
    telux::common::Status status = vlanManager->queryVlanMappingList(vlanMappingRespCb);
#endif
    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafVlanMappingCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                  std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

        return LE_OK;
    }
    else
    {
        LE_ERROR("Request vlan mapping list failed, status: %d",int(status));
        return LE_FAULT;
    }
    return LE_OK;
}

bool taf_Vlan::sort_vlanId(const telux::data::VlanConfig& s1, const telux::data::VlanConfig& s2)
{
    if(s1.vlanId < s2.vlanId)
        return true;

    return false;
}

void taf_Vlan::ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void  *contextPtr)
{
    void* vlanRef;

    taf_Vlan_t* vlanPtr = NULL;
    auto &tafVlan = taf_Vlan::GetInstance();

    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is invalid");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafVlan.vlanRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        vlanPtr = (taf_Vlan_t*)le_ref_GetValue(iterRef);

        if (vlanPtr != NULL && vlanPtr->sessionRef == sessionRef)
        {
            // Remove the vlan reference.
            vlanRef = (void*)le_ref_GetSafeRef(iterRef);
            LE_ASSERT(vlanRef != NULL);
            le_ref_DeleteRef(tafVlan.vlanRefMap, vlanRef);

            le_mem_Release(vlanPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get IP Pass through and IP config reference.
 *
 * @return
 *  - NULL   Invalid parameters or internal errors.
 *  - Others IMS reference.
 */
//--------------------------------------------------------------------------------------------------
taf_netIpPass_InterfaceRef_t taf_Vlan::GetInterface
(
     taf_net_VlanIfType_t ifType
)
{
     taf_InterfaceConfig_t* interfacePtr = NULL;
     interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfacePool);
     interfacePtr->ifType=ifType;

     return (taf_netIpPass_InterfaceRef_t)le_ref_CreateRef(interfaceRefMap, (void*)interfacePtr);
}

le_result_t taf_Vlan::SetIPPTOperation
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    taf_netIpPass_Operation_t  operation
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    interfacePtr->passThroughConfig.operation = operation;

    LE_DEBUG("SetIPPTOperation(%d)", (int)operation);

    return LE_OK;

}


le_result_t taf_Vlan::SetIPPTDeviceMacAddress
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    taf_net_VlanIfType_t ifType,
    const char *macAddr
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    interfacePtr->passThroughConfig.ifType = ifType;
    le_utf8_Copy(interfacePtr->passThroughConfig.macAddr,macAddr, TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);

    LE_DEBUG("SetIPPTDeviceMacAddress(%d)", (int)ifType);
    LE_DEBUG("Mac address %s", macAddr);

    return LE_OK;

}


le_result_t taf_Vlan::SetIPPassThroughConfig
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    uint32_t profileId=0;
    uint8_t slotId=0;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL , LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting slotId and profileId failed");

    telux::data::IpptParams ipptParams;
    telux::data::IpptConfig ipptConfig;
    telux::data::IpptDeviceConfig ipptDeviceConfig;

    ipptParams.profileId = profileId;
    ipptParams.slotId  = (SlotId)slotId;
    ipptParams.vlanId  = vlanId;
    LE_DEBUG("SetIPPassThroughConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("SetIPPassThroughConfig slotId(%d)", (int)slotId);
    LE_DEBUG("SetIPPassThroughConfig profileId(%d)", (int)profileId);

    ipptConfig.ipptOpr = (telux::data::Operation)interfacePtr->passThroughConfig.operation;
    LE_DEBUG("SetIPPassThroughConfig operation(%d)", (int)interfacePtr->passThroughConfig.operation);
    if( interfacePtr->passThroughConfig.operation == TAF_NETIPPASS_IPPT_ENABLE )
    {
      ipptConfig.devConfig.nwInterface =
        (telux::data::InterfaceType)interfacePtr->passThroughConfig.ifType;

      //le_utf8_Copy((char*)ipptConfig.devConfig.macAddr.data(),
           //interfacePtr->passThroughConfig.macAddr, TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);
      std::string mac(interfacePtr->passThroughConfig.macAddr);
      ipptConfig.devConfig.macAddr = mac;

      LE_DEBUG("SetIPPassThrough iftype(%d)", (int)interfacePtr->passThroughConfig.ifType);
      LE_DEBUG("SetIPPassThrough Mac in %s", interfacePtr->passThroughConfig.macAddr);
      LE_DEBUG("SetIPPassThrough Mac out %s", ipptConfig.devConfig.macAddr.data());
    }
    else
    {
        LE_ERROR("IP Pass through disabled or unknown; not filling device config");
    }

    telux::common::ErrorCode error = dataSettingsManager->setIpPassThroughConfig(ipptParams, 
                                                                                 ipptConfig);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to set ip pass through, error:%d ",static_cast<int>(error));
        return LE_FAULT;
    }
    else
    {
        LE_INFO("set ip pass through is success...");
    }
    return LE_OK;
}

taf_netIpPass_InterfaceRef_t taf_Vlan::GetIPPassThroughConfig
(
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    bool result = false;
    uint32_t profileId=0;
    uint8_t slotId=0;

    result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "Getting slotId and profileId failed");

    telux::data::IpptConfig ipptConfig;
    telux::data::IpptParams ipptParams;

    ipptParams.profileId = profileId;
    ipptParams.vlanId = vlanId;
    ipptParams.slotId = static_cast<SlotId>(slotId);
    LE_DEBUG("GetIPPassThroughConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("GetIPPassThroughConfig slotId(%d)", (int)slotId);
    LE_DEBUG("GetIPPassThroughConfig profileId(%d)", (int)profileId);

    telux::common::ErrorCode error = dataSettingsManager->getIpPassThroughConfig(ipptParams,
                                                                                 ipptConfig);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to get ip pass through, error:%d ",static_cast<int>(error));
        return NULL;
    }
    else
    {
        LE_INFO("set ip pass through GET is success...");
    }

    // fill values in the REFERENCE to be passed to the client
    interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfaceIPPool);

    interfacePtr->passThroughConfig.operation = (taf_netIpPass_Operation_t)ipptConfig.ipptOpr;
    LE_DEBUG("GetIPPassThroughConfig operation(%d)", (int)interfacePtr->passThroughConfig.operation);
    interfacePtr->passThroughConfig.ifType =
                 (taf_net_VlanIfType_t)ipptConfig.devConfig.nwInterface;
    le_utf8_Copy(interfacePtr->passThroughConfig.macAddr,ipptConfig.devConfig.macAddr.c_str(),
                 TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);
    LE_DEBUG("GetIPPassThroughConfig iftype(%d)", (int)interfacePtr->passThroughConfig.ifType);
    LE_DEBUG("GetIPPassThroughConfig Mac %s", interfacePtr->passThroughConfig.macAddr);

    return (taf_netIpPass_InterfaceRef_t)le_ref_CreateRef(interfaceIPRefMap, (void*)interfacePtr);
}


le_result_t taf_Vlan::GetIPPTOperation
(
    taf_netIpPass_InterfaceRef_t ifIPRef,
    taf_netIpPass_Operation_t*  operation
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap,ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    *operation = interfacePtr->passThroughConfig.operation;

    return LE_OK;

}

le_result_t taf_Vlan::GetIPPTDeviceMacAddress
(
    taf_netIpPass_InterfaceRef_t ifIPRef,
    taf_net_VlanIfType_t *ifType,
    char *macAddr, size_t macAddrSize
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap,ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    //if( interfacePtr->passThroughConfig.operation == TAF_NETIPPASS_IPPT_ENABLE )
    if(interfacePtr->passThroughConfig.macAddr != NULL)
    {
      *ifType = interfacePtr->passThroughConfig.ifType;
       le_utf8_Copy(macAddr,interfacePtr->passThroughConfig.macAddr,TAF_NET_MAC_ADDR_MAX_LEN + 1,
                  NULL);
       LE_DEBUG("GetIPPTDeviceMacAddress iftype(%d)", (int)interfacePtr->passThroughConfig.ifType);
       LE_DEBUG("GetIPPTDeviceMacAddress Mac %s", interfacePtr->passThroughConfig.macAddr);
    }
    else
    {
        LE_ERROR("IP Pass through disabled or unknown; not filling device config");
    }

    return LE_OK;
}
// IP config
le_result_t taf_Vlan::SetIPConfig
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t   ifType,
    uint16_t vlanId
)
{

    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    telux::data::IpConfig ipConfig;
    telux::data::IpConfigParams ipConfigParams;

    //Set ipConfig
    ipConfig.ipType  = (telux::data::IpAssignType)interfacePtr->ipConfig.ipAssignType;
    ipConfig.ipOpr  = (telux::data::IpAssignOperation)interfacePtr->ipConfig.ipOpr;

    LE_DEBUG("SetIPConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("SetIPConfig ipType(%d)", (int)ipType);

    if(interfacePtr->ipConfig.ipAssignType == TAF_NETIPPASS_STATIC_IP)
    {
    TAF_ERROR_IF_RET_VAL( (interfacePtr->ipConfig.ipAddrInfo.interfaceAddress == NULL) ||
                          (interfacePtr->ipConfig.ipAddrInfo.gwAddress == NULL) ||
                          (interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress == NULL) ||
                          (interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress == NULL),
                          LE_BAD_PARAMETER, "invalid ip address");

    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;

    if(ipType == TAF_NET_IPV4)
    {
       if(
       (inet_pton(AF_INET,
        interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,&(addr.sin_addr)) != 1) ||
       (inet_pton(AF_INET,
        interfacePtr->ipConfig.ipAddrInfo.gwAddress,&(addr.sin_addr)) != 1) ||
       (inet_pton(AF_INET,
        interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,&(addr.sin_addr)) != 1) ||
       (inet_pton(AF_INET,
        interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,&(addr.sin_addr)) != 1)
          )
         {
           return LE_BAD_PARAMETER;
         }
    }
    else if(ipType == TAF_NET_IPV6)
    {
         if ((inet_pton(AF_INET6,
              interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,&(addr6.sin6_addr)) != 1)
         || (inet_pton(AF_INET6,
             interfacePtr->ipConfig.ipAddrInfo.gwAddress,&(addr6.sin6_addr)) != 1)
        || (inet_pton(AF_INET6,
            interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,&(addr6.sin6_addr)) != 1)
       || (inet_pton(AF_INET6,
           interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,&(addr6.sin6_addr)) != 1)
          )
         {
           return LE_BAD_PARAMETER;
         }
    }

    std::string interfaceAddress(interfacePtr->ipConfig.ipAddrInfo.interfaceAddress);
    ipConfig.ipAddr.ifAddress = interfaceAddress;
    ipConfig.ipAddr.ifMask = interfacePtr->ipConfig.ipAddrInfo.interfaceMask;

    std::string gwAddress(interfacePtr->ipConfig.ipAddrInfo.gwAddress);
    ipConfig.ipAddr.gwAddress = gwAddress;

    std::string primaryDnsAddress(interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress);
    ipConfig.ipAddr.primaryDnsAddress = primaryDnsAddress;

    std::string secondaryDnsAddress(interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress);
    ipConfig.ipAddr.secondaryDnsAddress = secondaryDnsAddress;

    LE_DEBUG("SetIPConfig if %s", ipConfig.ipAddr.ifAddress.data());
    LE_DEBUG("SetIPConfig gw %s", ipConfig.ipAddr.gwAddress.data());
    LE_DEBUG("SetIPConfig pDNS %s", ipConfig.ipAddr.primaryDnsAddress.data());
    LE_DEBUG("SetIPConfig sDNS %s", ipConfig.ipAddr.secondaryDnsAddress.data());
    }

    //Set ipConfigParams
    ipConfigParams.ifType = (telux::data::InterfaceType)ifType;
    ipConfigParams.vlanId = vlanId;

    if(ipType == TAF_NET_IPV4) 
       { ipConfigParams.ipFamilyType = telux::data::IpFamilyType::IPV4; }
    else if (ipType == TAF_NET_IPV6) 
        { ipConfigParams.ipFamilyType = telux::data::IpFamilyType::IPV6; }
    else
        return LE_BAD_PARAMETER;

    telux::common::ErrorCode error = dataSettingsManager->setIpConfig(ipConfigParams, 
                                                                      ipConfig);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to set ip config , error:%d ",static_cast<int>(error));
        return LE_FAULT;
    }
    else
    {
        LE_INFO("set ip config is success...");
    }

    return LE_OK;

}

le_result_t taf_Vlan::SetIPConfigParams
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    taf_netIpPass_IpAssignOperation_t ipOpr,
    taf_netIpPass_IpAssignType_t ipAssignType
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    interfacePtr->ipConfig.ipOpr = ipOpr;
    interfacePtr->ipConfig.ipAssignType = ipAssignType;

    LE_DEBUG("SetIPConfigParams ipOpr(%d)", (int)interfacePtr->ipConfig.ipOpr);
    LE_DEBUG("SetIPConfigParams ipAssignType(%d)", (int)interfacePtr->ipConfig.ipAssignType);

    return LE_OK;
}

le_result_t taf_Vlan::SetIPConfigAddressParams
(
    taf_netIpPass_InterfaceRef_t  interfaceRef,
    const taf_netIpPass_IpAddressInfo_t*  ipAddrInfo
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    TAF_ERROR_IF_RET_VAL( (ipAddrInfo->interfaceAddress == NULL) ||
                          (ipAddrInfo->gwAddress == NULL) ||
                          (ipAddrInfo->primaryDnsAddress == NULL) ||
                          (ipAddrInfo->secondaryDnsAddress == NULL),
                          LE_BAD_PARAMETER, "ip address NULL");

    le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,ipAddrInfo->interfaceAddress, 
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    interfacePtr->ipConfig.ipAddrInfo.interfaceMask = ipAddrInfo->interfaceMask;

    le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.gwAddress,ipAddrInfo->gwAddress, 
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,ipAddrInfo->primaryDnsAddress, 
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,ipAddrInfo->secondaryDnsAddress, 
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);

    LE_DEBUG("SetIPConfigAddressParams if %s", interfacePtr->ipConfig.ipAddrInfo.interfaceAddress);
    LE_DEBUG("SetIPConfigAddressParams gw %s", interfacePtr->ipConfig.ipAddrInfo.gwAddress);
    LE_DEBUG("SetIPConfigAddressParams pDNS %s", interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress);
    LE_DEBUG("SetIPConfigAddressParams sDNS %s", interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress);

    return LE_OK;
}

le_result_t taf_Vlan::GetIPConfigParams
(
    taf_netIpPass_InterfaceRef_t ifIPRef,
    taf_netIpPass_IpAssignOperation_t* ipOpr,
    taf_netIpPass_IpAssignType_t* ipAssignType
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap, ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    *ipOpr = interfacePtr->ipConfig.ipOpr;
    *ipAssignType = interfacePtr->ipConfig.ipAssignType;

    LE_DEBUG("GetIPConfigParams ipOpr(%d)", (int)interfacePtr->ipConfig.ipOpr);
    LE_DEBUG("GetIPConfigParams ipAssignType(%d)", (int)interfacePtr->ipConfig.ipAssignType);

    return LE_OK;
}

le_result_t taf_Vlan::GetIPConfigAddressParams
(
    taf_netIpPass_InterfaceRef_t ifIPRef,
    taf_netIpPass_IpAddressInfo_t*  ipAddrInfo
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap, ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    if(interfacePtr->ipConfig.ipAssignType == TAF_NETIPPASS_STATIC_IP)
    {    
    TAF_ERROR_IF_RET_VAL( (ipAddrInfo == NULL) || (ipAddrInfo->interfaceAddress == NULL) ||
                          (ipAddrInfo->gwAddress == NULL) ||
                          (ipAddrInfo->primaryDnsAddress == NULL) ||
                          (ipAddrInfo->secondaryDnsAddress == NULL),
                          LE_BAD_PARAMETER, "ip address NULL");

    le_utf8_Copy(ipAddrInfo->interfaceAddress,interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    ipAddrInfo->interfaceMask = interfacePtr->ipConfig.ipAddrInfo.interfaceMask;

    le_utf8_Copy(ipAddrInfo->gwAddress,interfacePtr->ipConfig.ipAddrInfo.gwAddress,
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(ipAddrInfo->primaryDnsAddress,
                 interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(ipAddrInfo->secondaryDnsAddress,
                 interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,
                 TAF_NET_IP_ADDR_MAX_LEN, NULL);

    LE_DEBUG("GetIPConfigAddressParams if %s", interfacePtr->ipConfig.ipAddrInfo.interfaceAddress);
    LE_DEBUG("GetIPConfigAddressParams gw %s", interfacePtr->ipConfig.ipAddrInfo.gwAddress);
    LE_DEBUG("GetIPConfigAddressParams pDNS %s", interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress);
    LE_DEBUG("GetIPConfigAddressParams sDNS %s", interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress);
    }
    else
    {
        LE_ERROR("IP config is DYNAMIC or UNKNOWN; dont copy address information");
    }

    return LE_OK;
}

taf_netIpPass_InterfaceRef_t taf_Vlan::GetIPConfig
(
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t ifType,
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;

    telux::data::IpConfig ipConfig;
    telux::data::IpConfigParams ipConfigParams;

    //Set ipConfigParams
    ipConfigParams.ifType = (telux::data::InterfaceType)ifType;
    ipConfigParams.vlanId = vlanId;
    LE_DEBUG("GetIPConfig vlanid(%d)", (int)vlanId);

    if(ipType == TAF_NET_IPV4) 
       { ipConfigParams.ipFamilyType = telux::data::IpFamilyType::IPV4; }
    else if (ipType == TAF_NET_IPV6) 
        { ipConfigParams.ipFamilyType = telux::data::IpFamilyType::IPV6; }
    else
    {
        LE_ERROR("ERROR - invalid ip type , error:%d ",static_cast<int>(ipType));
        return NULL;
    }

    telux::common::ErrorCode error = dataSettingsManager->getIpConfig(ipConfigParams, 
                                                                      ipConfig);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to get ip config , error:%d ",static_cast<int>(error));
        return NULL;
    }
    else
    {
        LE_INFO("get ip config is success...");
    }

    // fill values in the REFERENCE to be passed to the client
    interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfaceIPPool);

    interfacePtr->ipConfig.ipOpr = (taf_netIpPass_IpAssignOperation_t)ipConfig.ipOpr;
    interfacePtr->ipConfig.ipAssignType = (taf_netIpPass_IpAssignType_t)ipConfig.ipType;

    LE_DEBUG("GetIPConfig ipOpr(%d)", (int)interfacePtr->ipConfig.ipOpr);
    LE_DEBUG("GetIPConfig ipAssignType(%d)", (int)interfacePtr->ipConfig.ipAssignType);

    if(ipConfig.ipType == telux::data::IpAssignType::STATIC_IP)
    {
       //copy IP address info
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,
                     ipConfig.ipAddr.ifAddress.c_str(),TAF_NET_IP_ADDR_MAX_LEN, NULL);
        interfacePtr->ipConfig.ipAddrInfo.interfaceMask = ipConfig.ipAddr.ifMask;

        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.gwAddress,
                     ipConfig.ipAddr.gwAddress.c_str(),
                     TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,
                     ipConfig.ipAddr.primaryDnsAddress.c_str(),TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,
                     ipConfig.ipAddr.secondaryDnsAddress.c_str(),TAF_NET_IP_ADDR_MAX_LEN, NULL);

        LE_DEBUG("GetIPConfig if %s", interfacePtr->ipConfig.ipAddrInfo.interfaceAddress);
        LE_DEBUG("GetIPConfig gw %s", interfacePtr->ipConfig.ipAddrInfo.gwAddress);
        LE_DEBUG("GetIPConfig pDNS %s", interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress);
        LE_DEBUG("GetIPConfig sDNS %s", interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress);

    }
    else
    {
        LE_ERROR("IP config is DYNAMIC or UNKNOWN; dont copy address information");
    }

    return (taf_netIpPass_InterfaceRef_t)le_ref_CreateRef(interfaceIPRefMap, (void*)interfacePtr);
}

le_result_t taf_Vlan::SetIPPassThroughNatConfig(bool isEnabled)
{
    telux::common::ErrorCode error = dataSettingsManager->setIpPassThroughNatConfig(isEnabled);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to get get ippt NAT config , error:%d ",static_cast<int>(error));
        return LE_FAULT;
    }
    else
    {
        LE_INFO("set ippt NAT config is success...");
    }
    LE_DEBUG("SetIPPassThroughNatConfig %d", static_cast<int>(isEnabled));
    return LE_OK;
}

le_result_t taf_Vlan::GetIPPassThroughNatConfig(bool *isEnabledPtr)
{
    TAF_ERROR_IF_RET_VAL(isEnabledPtr == NULL, LE_BAD_PARAMETER, "isEnabledPtr is null");

    telux::common::ErrorCode error = dataSettingsManager->getIpPassThroughNatConfig(*isEnabledPtr);
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("ERROR - Failed to get ippt NAT config , error:%d ",static_cast<int>(error));
        return LE_FAULT;
    }
    else
    {
        LE_INFO("get ippt NAT config is success...");
    }
    LE_DEBUG("GetIPPassThroughNatConfig %d", static_cast<int>(*isEnabledPtr));
    return LE_OK;
}





