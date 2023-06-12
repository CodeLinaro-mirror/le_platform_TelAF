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
#include "tafGsbImpl.hpp"
#include "tafSvcIF.hpp"

#define CONFIG_GSB_TIMEOUT 10

using namespace telux::tafsvc;

//gsb list definition
LE_MEM_DEFINE_STATIC_POOL(gsbListPool, TAF_NET_MAX_GSB_LIST_NUM, sizeof(taf_GsbList_t));

LE_MEM_DEFINE_STATIC_POOL(gsbPool, TAF_NET_MAX_GSB_NUM, sizeof(taf_Gsb_t));

LE_MEM_DEFINE_STATIC_POOL(gsbSafeRefPool, TAF_NET_MAX_GSB_NUM, sizeof(taf_GsbSafeRef_t));

LE_REF_DEFINE_STATIC_MAP(gsbListRefMap, TAF_NET_MAX_GSB_LIST_NUM);

LE_REF_DEFINE_STATIC_MAP(gsbSafeRefMap, TAF_NET_MAX_GSB_NUM);

std::vector<telux::data::net::BridgeInfo> tafGsbCallback::gsbInfo;

le_sem_Ref_t tafGsbCallback::semaphore = nullptr;


/*======================================================================

 FUNCTION        taf_Gsb::Init

 DESCRIPTION     Initialization of the taf Gsb component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

======================================================================*/
void taf_Gsb::Init(void)
{

    bool isReady = false;

    // 1. Initiate the semaphore
    tafGsbCallback::semaphore = le_sem_Create("taf_GsbRespCbSem", 0);

    gsbListPool = le_mem_InitStaticPool(gsbListPool,
                               TAF_NET_MAX_GSB_LIST_NUM, sizeof(taf_GsbList_t));

    gsbPool = le_mem_InitStaticPool(gsbPool,
                           TAF_NET_MAX_GSB_NUM, sizeof(taf_Gsb_t));

    // 2. Initiate the memory pool
    gsbListPool = le_mem_InitStaticPool(gsbListPool,
                                        TAF_NET_MAX_GSB_LIST_NUM, sizeof(taf_GsbList_t));

    gsbPool = le_mem_InitStaticPool(gsbPool, TAF_NET_MAX_GSB_NUM, sizeof(taf_Gsb_t));

    gsbSafeRefPool = le_mem_InitStaticPool(gsbSafeRefPool,
                                        TAF_NET_MAX_GSB_NUM, sizeof(taf_GsbSafeRef_t));

    // 3. Initiate the reference map.
    gsbListRefMap = le_ref_InitStaticMap(gsbListRefMap, TAF_NET_MAX_GSB_LIST_NUM);
    gsbSafeRefMap = le_ref_InitStaticMap(gsbSafeRefMap, TAF_NET_MAX_GSB_NUM);

    // 4. Get the DataFactory and static BridgeManager instances
    if (gsbManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto initCb = std::bind(&taf_Gsb::onInitComplete, this, std::placeholders::_1);
        gsbManager = dataFactory.getBridgeManager( initCb );
#else
        gsbManager = dataFactory.getBridgeManager();
#endif
    }

    if(gsbManager == nullptr )
    {
        LE_INFO("Gsb manager initialize error...");
        return ;
    }

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // 6. Check subsystem status
    std::unique_lock<std::mutex> lck(mMutex);

    telux::common::ServiceStatus subSystemStatus = gsbManager->getServiceStatus();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        LE_INFO("Gsb manager initialize...");
        conVar.wait(lck, [this]{return this->IsSubSystemStatusUpdated;});
        subSystemStatus = gsbManager->getServiceStatus();
    }

    //At this point, initialization should be either AVAILABLE or Failure
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("Gsb Manager initialization failed");
        gsbManager = nullptr;
        return ;
    }
#endif

    isReady = gsbManager->isSubsystemReady();

    if(isReady == false)
    {
        LE_INFO("Gsb component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = gsbManager->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("gsb component is ready...");
    }
    else
    {
        LE_CRIT("unable to init gsb component!");
    }

    return;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetInstance

 DESCRIPTION     Get the instance of gsb.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      None

 RETURN VALUE    taf_Gsb &

======================================================================*/
taf_Gsb &taf_Gsb::GetInstance()
{
    static taf_Gsb instance;
    return instance;
}

/*======================================================================

 FUNCTION        tafGsbCallback::onResponseCallback

 DESCRIPTION     Call back function for enable, add or remove gsb.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafGsbCallback::onResponseCallback(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafGsb = taf_Gsb::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafGsb.GsbSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafGsbCallback::onBridgeListResponse

 DESCRIPTION     Call back function for request bridge info list.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] std::vector<telux::data::net::BridgeInfo> &infos:
                                                                The gsb list.
                 [IN] telux::common::ErrorCode error: error code.
 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafGsbCallback::onBridgeListResponse
(
    const std::vector<telux::data::net::BridgeInfo> &infos,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> tafGsbCallback --> onBridgeListResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
    }

    gsbInfo.assign(infos.begin(), infos.end());

    le_sem_Post(semaphore);
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
/*======================================================================

 FUNCTION        taf_Gsb::onInitComplete

 DESCRIPTION     Call back function of gsbManager.

 DEPENDENCIES    The initialization of Gsb.

 PARAMETERS      [IN] telux::common::ServiceStatus status : Gsb manager service status.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Gsb::onInitComplete(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IsSubSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif

/*======================================================================

 FUNCTION        taf_Gsb::AddGsb

 DESCRIPTION     Add generic software bridge configuration for an interface.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] char* ifName : The interface name.
                 [IN] taf_net_GsbIfType_t ifType : The interface type.
                 [IN] uint32_t bandwidth : The bandwidth(in Mbps).

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to add gsb
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Failed to add gsb.

======================================================================*/
le_result_t taf_Gsb::AddGsb(const char* ifName, taf_net_GsbIfType_t ifType, uint32_t bandwidth)
{
    #if 0
    le_result_t result;
    telux::data::net::BridgeInfo bridgeConfig;
    std::chrono::seconds span(CONFIG_GSB_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(gsbManager == NULL, LE_FAULT, "gsbManager is null");
    TAF_ERROR_IF_RET_VAL(ifName == NULL, LE_BAD_PARAMETER, "ifName is null");
    TAF_ERROR_IF_RET_VAL(bandwidth > 900, LE_BAD_PARAMETER, "bandwidth is error");

    bridgeConfig.ifaceName = ifName;
    bridgeConfig.ifaceType = telux::data::net::BridgeIFaceType(ifType);
    bridgeConfig.bandwidth = bandwidth;

    GsbSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafGsbCallback> addGsbCb = std::make_shared<tafGsbCallback>();

    auto  addGsbRespCb = std::bind(&tafGsbCallback::onResponseCallback, addGsbCb,
                                          std::placeholders::_1);

    Status status = gsbManager->addBridge(bridgeConfig, addGsbRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = GsbSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Add GSB timeout for %d seconds", CONFIG_GSB_TIMEOUT);
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
        LE_ERROR( "ERROR - Failed to add GSB, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }
    #endif
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::RemoveGsb

 DESCRIPTION     Remove generic software bridge configuration for an interface.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] char* ifName : The interface name.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to add gsb
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Failed to add gsb.

======================================================================*/
le_result_t taf_Gsb::RemoveGsb(const char* ifName)
{
    #if 0
    le_result_t result;
    std::chrono::seconds span(CONFIG_GSB_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(gsbManager == NULL, LE_FAULT, "gsbManager is null");
    TAF_ERROR_IF_RET_VAL(ifName == NULL, LE_BAD_PARAMETER, "ifName is null");

    GsbSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafGsbCallback> removeGsbCb = std::make_shared<tafGsbCallback>();

    auto  removeGsbRespCb = std::bind(&tafGsbCallback::onResponseCallback,
                                             removeGsbCb, std::placeholders::_1);

    Status status = gsbManager->removeBridge(ifName, removeGsbRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = GsbSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Remove GSB timeout for %d seconds", CONFIG_GSB_TIMEOUT);
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
        LE_ERROR( "ERROR - Failed to remove GSB, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }
    #endif
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::EnableGsb

 DESCRIPTION     Enable the generic software bridge.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] bool enable : True for enable or false for disable gsb.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to enable gsb
                     LE_FAULT:         Failed to enable gsb.

======================================================================*/
le_result_t taf_Gsb::EnableGsb(bool enable)
{
    #if 0
    le_result_t result;
    std::chrono::seconds span(CONFIG_GSB_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(gsbManager == NULL, LE_FAULT, "gsbManager is null");

    GsbSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafGsbCallback> enableGsbCb = std::make_shared<tafGsbCallback>();

    auto  enableGsbRespCb = std::bind(&tafGsbCallback::onResponseCallback, enableGsbCb,
                                          std::placeholders::_1);

    Status status = gsbManager->enableBridge(enable, enableGsbRespCb);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = GsbSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Enable/Disable GSB timeout for %d seconds", CONFIG_GSB_TIMEOUT);
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
        LE_ERROR( "ERROR - Failed to enable(%d) GSB, Status:%d ", enable, static_cast<int>(status));
        return LE_FAULT;
    }
    #endif
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbList

 DESCRIPTION     Get the reference of the generic software bridge list.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      None.

 RETURN VALUE    taf_net_GsbListRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbListRef_t taf_Gsb::GetGsbList()
{
    #if 0
    TAF_ERROR_IF_RET_VAL(gsbManager == NULL, NULL, "gsbManager is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

    telux::common::Status status = gsbManager->requestBridgeInfo(tafGsbCallback::onBridgeListResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {CONFIG_GSB_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafGsbCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime =
                                                                   std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafGsbCallback::gsbInfo.size() == 0)
        {
            LE_DEBUG("No gsb");
            return NULL;
        }

        taf_GsbList_t* gsbsList =
                                       (taf_GsbList_t*)le_mem_ForceAlloc(gsbListPool);
        gsbsList->gsbList = LE_SLS_LIST_INIT;
        gsbsList->safeRefList = LE_SLS_LIST_INIT;
        gsbsList->currPtr = NULL;

        taf_Gsb_t* gsbPtr;
        //queue gsb information
        for (auto info : tafGsbCallback::gsbInfo)
        {
            gsbPtr = (taf_Gsb_t*)le_mem_ForceAlloc(gsbPool);
            le_utf8_Copy(gsbPtr->info.ifName, info.ifaceName.c_str(), TAF_NET_INTERFACE_NAME_MAX_NUM, NULL);
            gsbPtr->info.ifType=(taf_net_GsbIfType_t)info.ifaceType;
            gsbPtr->info.bandwidth=info.bandwidth;

            gsbPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(gsbsList->gsbList), &(gsbPtr->link));
        }

        return (taf_net_GsbListRef_t)le_ref_CreateRef(gsbListRefMap,
                                                            (void*)gsbsList);

    }
    else
    {
        LE_ERROR("Request gsb list failed, status: %d",int(status));
        return NULL;
    }
    #endif
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetFirstGsb

 DESCRIPTION     Get the reference of the first generic software bridge with a list reference.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    taf_net_GsbRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbRef_t taf_Gsb::GetFirstGsb( taf_net_GsbListRef_t gsbListRef )
{
    #if 0
    taf_GsbList_t* listPtr = (taf_GsbList_t*)le_ref_Lookup(gsbListRefMap, gsbListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", gsbListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->gsbList));
    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_Gsb_t* gsbPtr = CONTAINER_OF(linkPtr, taf_Gsb_t , link);
    listPtr->currPtr = linkPtr;

    taf_GsbSafeRef_t* safeRefPtr =
                           (taf_GsbSafeRef_t*)le_mem_ForceAlloc(gsbSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(gsbSafeRefMap, (void*)gsbPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_GsbRef_t)safeRefPtr->safeRef;
    #endif
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetNextGsb

 DESCRIPTION     Get the reference of the next gsb from a list.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    taf_net_GsbRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbRef_t taf_Gsb::GetNextGsb( taf_net_GsbListRef_t gsbListRef )
{
    #if 0
    taf_GsbList_t* listPtr = (taf_GsbList_t*)le_ref_Lookup(gsbListRefMap, gsbListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", gsbListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->gsbList), listPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_Gsb_t* gsbPtr = CONTAINER_OF(linkPtr, taf_Gsb_t , link);
    listPtr->currPtr = linkPtr;

    taf_GsbSafeRef_t* safeRefPtr =
                           (taf_GsbSafeRef_t*)le_mem_ForceAlloc(gsbSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(gsbSafeRefMap, (void*)gsbPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_GsbRef_t)safeRefPtr->safeRef;
    #endif
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::DeleteGsbList

 DESCRIPTION     Delete a reference of a gsb list.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Failure.
                     LE_OK:            Success.

======================================================================*/
le_result_t taf_Gsb::DeleteGsbList( taf_net_GsbListRef_t gsbListRef )
{
    #if 0
    taf_Gsb_t* gsbPtr;
    taf_GsbSafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(gsbListRef == NULL, LE_BAD_PARAMETER, "Null reference(gsbListRef)");

    taf_GsbList_t* listPtr = (taf_GsbList_t*)le_ref_Lookup(gsbListRefMap, gsbListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(listPtr->gsbList))) != NULL)
    {
        gsbPtr = CONTAINER_OF(linkPtr, taf_Gsb_t, link);
        le_mem_Release(gsbPtr);
    }

    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_GsbSafeRef_t, link);
        le_ref_DeleteRef(gsbSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(gsbListRefMap, gsbListRef);

    le_mem_Release(listPtr);

    return LE_OK;
    #endif
    return LE_UNSUPPORTED;
}


/*======================================================================

 FUNCTION        taf_Gsb::GetGsbInterfaceName

 DESCRIPTION     Get the interface name of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.
                 [OUT] char* ifNamePtr: The interface name.
                 [IN] size_t ifNamePtrSize: The interface name size.

 RETURN VALUE    le_result_t
                     LE_OK  Successful
                     LE_NOT_FOUND  Not found the interface name
                     LE_BAD_PARAMETER  Invalid parameter
                     LE_FAULT  Failed

======================================================================*/
le_result_t taf_Gsb::GetGsbInterfaceName
(
    taf_net_GsbRef_t gsbRef,
    char* ifNamePtr,
    size_t ifNamePtrSize
)
{
    #if 0
    TAF_ERROR_IF_RET_VAL(gsbRef == NULL, LE_NOT_FOUND, "gsbRef is null");
    TAF_ERROR_IF_RET_VAL(ifNamePtr == NULL, LE_BAD_PARAMETER, "ifNamePtr is null");

    taf_Gsb_t* gsbPtr = (taf_Gsb_t*)le_ref_Lookup(gsbSafeRefMap, gsbRef);
    TAF_ERROR_IF_RET_VAL(gsbPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    le_utf8_Copy(ifNamePtr, gsbPtr->info.ifName, ifNamePtrSize, NULL);

    return LE_OK;
    #endif
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbInterfaceType

 DESCRIPTION     Get the interface type of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.

 RETURN VALUE    taf_net_GsbIfType_t
                               The interface type

======================================================================*/
taf_net_GsbIfType_t taf_Gsb::GetGsbInterfaceType
(
    taf_net_GsbRef_t gsbRef
)
{
    #if 0
    TAF_ERROR_IF_RET_VAL(gsbRef == NULL, TAF_NET_GSB_UNKNOWN, "Null reference(gsbRef)");

    taf_Gsb_t* gsbPtr = (taf_Gsb_t*)le_ref_Lookup(gsbSafeRefMap,
                                                                    gsbRef);
    TAF_ERROR_IF_RET_VAL(gsbPtr  == NULL, TAF_NET_GSB_UNKNOWN, "Invalid para(null reference ptr)");

    return gsbPtr ->info.ifType;
    #endif
    return TAF_NET_GSB_UNKNOWN;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbBandWidth

 DESCRIPTION     Get the band width of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.

 RETURN VALUE    int32_t
                     -1    ERROR
                     others bandWitdh

======================================================================*/
int32_t taf_Gsb::GetGsbBandWidth
(
    taf_net_GsbRef_t gsbRef
)
{
    #if 0
    TAF_ERROR_IF_RET_VAL(gsbRef == NULL, -1, "Null reference(gsbRef)");

    taf_Gsb_t* gsbPtr = (taf_Gsb_t*)le_ref_Lookup(gsbSafeRefMap,
                                                                    gsbRef);
    TAF_ERROR_IF_RET_VAL(gsbPtr == NULL, -1, "Invalid para(null reference ptr)");

    return gsbPtr->info.bandwidth;
    #endif
    return -1;
}

