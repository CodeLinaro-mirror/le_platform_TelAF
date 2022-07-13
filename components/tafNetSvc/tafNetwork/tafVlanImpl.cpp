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
#include "tafSvcIF.hpp"

#define OPERATION_TIMEOUT 10

using namespace telux::tafsvc;

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

std::list<std::pair<int, int>> tafVlanMappingCallback::vlanMappingInfo;
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

    vlanEntryListRefMap = le_ref_InitStaticMap(vlanEntryListRefMap, TAF_NET_MAX_VLAN_ENTRY_LIST);

    vlanEntrySafeRefMap = le_ref_InitStaticMap(vlanEntrySafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

    vlanIfListRefMap = le_ref_InitStaticMap(vlanIfListRefMap, TAF_NET_MAX_VLAN_ENTRY);

    vlanIfSafeRefMap = le_ref_InitStaticMap(vlanIfSafeRefMap, TAF_NET_MAX_VLAN_ENTRY);

    // 4. Get the DataFactory and static VlanManager instances
    if (vlanManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#ifdef TARGET_SA515M
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

#ifdef TARGET_SA515M
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

    vlanMappingInfo = mapping;

    le_sem_Post(semaphore);
}

#ifdef TARGET_SA515M
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
    isVlanPresentInDb = IsVlanPresentInDb(vlanId,&IsAcceleratedInDb);

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
        vlanPtr->sessionRef=sessionRef;
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

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "vlan is not present");

    //check if vlan interface is present in this vlan
    isVlanPresentInDb = IsVlanPresentInDb(vlanPtr->vlanId,&IsAcceleratedInDb);

    TAF_ERROR_IF_RET_VAL(isVlanPresentInDb == true, LE_FAULT, "interface is present in this vlan");

    le_ref_DeleteRef(vlanRefMap, vlanRef);

    le_mem_Release(vlanPtr);

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
    isVlanPresentInDb = IsVlanPresentInDb(vlanId,&IsAcceleratedInDb);
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
bool taf_Vlan::IsVlanPresentInDb(uint16_t vlanId, bool *isAccelerated)
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

        if(GetBindingInfo() != LE_OK)
        {
            LE_ERROR("Can't get vlan mapping info");
            return NULL;
        }

        for (auto info : tafVlanCallback::vlanEntryInfo)
        {
            //queue one item for same vlan id
            if(previous_vlanId != info.vlanId)
            {
                vlanEntryPtr = (taf_VlanEntry_t*)le_mem_ForceAlloc(vlanEntryPool);
                vlanEntryPtr->info.vlanId=info.vlanId;
                if(info.vlanId != 0)
                {
                    vlanEntryPtr->info.profileId = GetBoundProfileIdFromVlan(info.vlanId);
                }
                else
                    vlanEntryPtr->info.profileId = -1;

                vlanEntryPtr->info.isAccelerated=info.isAccelerated;
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
                     vlan id

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

 FUNCTION        taf_Vlan::GetVlanProfileId

 DESCRIPTION     Get profile Id binding with the VLAN.

 DEPENDENCIES    Initialization of a vlan entry list and get a safe reference of a vlan entry.

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef :
                          The vlan entry reference.

 RETURN VALUE    int16_t
                     vlan id

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

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(vlanIfListRefMap);

        while (!isAdded && (le_ref_NextNode(iterRef) == LE_OK))
        {
            existedVlanIfList = (taf_VlanIfList_t*) le_ref_GetValue(iterRef);

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
                 [IN] uint32_t profileId: The profile id

 RETURN VALUE    le_result_t
                     LE_TIMEOUT：       Time out
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_NOT_FOUND:     Vlan not found
                     LE_FAULT:         Failure.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Vlan::BindVlanWithProfile(taf_net_VlanRef_t vlanRef, uint32_t profileId)
{
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif

    le_result_t result;
    uint16_t vlanId=0;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

    if(GetBindingInfo() != LE_OK)
    {
        LE_ERROR("get binding info error");
        return LE_FAULT;
    }
   // fix telsdk bug:when the profile is already bound with VLAN,bindWithProfile api from telsdk
   // always return OK
    vlanId=GetBoundVlanIdFromProfile(profileId);
    if(vlanId !=0)
    {
        LE_ERROR("Profile is already bound with vlan");
        return LE_FAULT;
    }

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Vlan not found");
    vlanId = vlanPtr->vlanId;

    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                                   std::make_shared<tafVlanMappingCallback>();

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);
#ifdef TARGET_SA515M
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
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif

    le_result_t result;
    uint16_t vlanId=0;
    uint16_t profileId=0;
    std::chrono::seconds span(OPERATION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    VlanSyncPromise = std::promise<le_result_t>();

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    vlanId = vlanPtr->vlanId;

    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_FAULT, "Invalid vlan id");

    profileId=GetBoundProfileIdFromVlan(vlanId);

    TAF_ERROR_IF_RET_VAL(profileId < 0, LE_FAULT, "Invalid profile id");

    std::shared_ptr<tafVlanMappingCallback> bindVlanWithProfileCb =
                                                   std::make_shared<tafVlanMappingCallback>();

    auto  bindVlanWithProfileRespCb = std::bind(&tafVlanMappingCallback::onResponseCallback,
                                                bindVlanWithProfileCb, std::placeholders::_1);
#ifdef TARGET_SA515M
    Status status = vlanManager->unbindFromProfile(profileId, vlanId,
                                                   bindVlanWithProfileRespCb,slot);
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

 FUNCTION        taf_Vlan::GetBoundVlanIdFromProfile

 DESCRIPTION     Get the bound vlan id from profile.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint32_t profileId: The profile Id.

 RETURN VALUE    uint16_t
                      0:      no vlan binding with this profile.
                      others:  vlan id

 SIDE EFFECTS

======================================================================*/
uint16_t taf_Vlan::GetBoundVlanIdFromProfile(uint32_t profileId)
{

    if(tafVlanMappingCallback::vlanMappingInfo.size() == 0)
    {
        LE_DEBUG("no binding info for this profile");
        return 0;
    }

    for (auto info : tafVlanMappingCallback::vlanMappingInfo)
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

 FUNCTION        taf_Vlan::GetBoundProfileIdFromVlan

 DESCRIPTION     Get the bound profile if from vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint32_t profileId: The profile Id.

 RETURN VALUE    int32_t
                      -1:      no profile binding with this vlan.
                      others:  profile id

 SIDE EFFECTS

======================================================================*/
int32_t taf_Vlan::GetBoundProfileIdFromVlan(uint16_t vlanId)
{
    if(GetBindingInfo() != LE_OK)
    {
        LE_ERROR("get binding info error");
        return -1;
    }
    if(tafVlanMappingCallback::vlanMappingInfo.size() == 0)
    {
        LE_DEBUG("no binding info for this profile");
        return -1;
    }

    for (auto info : tafVlanMappingCallback::vlanMappingInfo)
    {
            if((uint16_t)info.second == vlanId)
            {
                LE_DEBUG("the vlan %d is bound with profile %d",vlanId,(int32_t)info.first);
                return (int32_t)info.first;
            }
    }

    return -1;
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
le_result_t taf_Vlan::GetBindingInfo()
{
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif
    TAF_ERROR_IF_RET_VAL(vlanManager == NULL, LE_FAULT, "vlanManager is null");
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
#ifdef TARGET_SA515M
    telux::common::Status status = vlanManager->queryVlanMappingList(
                                            tafVlanMappingCallback::onVlanMappingListResponse,slot);
#else
    telux::common::Status status = vlanManager->queryVlanMappingList(
                                            tafVlanMappingCallback::onVlanMappingListResponse);
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


