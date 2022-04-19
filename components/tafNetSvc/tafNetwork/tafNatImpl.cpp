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
#include "tafNatImpl.hpp"
#include <arpa/inet.h>
#include "tafSvcIF.hpp"

using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(destNatEntryListPool, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_DestNatEntryList_t));

LE_MEM_DEFINE_STATIC_POOL(destNatEntryPool, TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntry_t));

LE_MEM_DEFINE_STATIC_POOL(destNatEntrySafeRefPool, TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntrySafeRef_t));

LE_REF_DEFINE_STATIC_MAP(destNatEntryListRefMap, TAF_DCS_PROFILE_LIST_MAX_ENTRY);

LE_REF_DEFINE_STATIC_MAP(destNatEntrySafeRefMap, TAF_NET_MAX_NAT_ENTRY);


std::vector<telux::data::net::NatConfig> tafNatCallback::destNatEntryInfo;

le_sem_Ref_t tafNatCallback::semaphore = nullptr;

/*======================================================================

 FUNCTION        taf_Nat::Init

 DESCRIPTION     Initialization of the taf Nat component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Nat::Init(void)
{
    bool isReady = false;

    // 1. Initiate the semaphore
    tafNatCallback::semaphore = le_sem_Create("taf_NatDestNatEntryRespCbSem", 0);

    // 2. create the event id
    DestNatChangeEvId = le_event_CreateIdWithRefCounting("DestNatRouteChange");

    // 3. Initiate the memory pool
    destNatEntryListPool = le_mem_InitStaticPool(destNatEntryListPool,
                               TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_DestNatEntryList_t));

    destNatEntryPool = le_mem_InitStaticPool(destNatEntryPool,
                           TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntry_t));

    destNatEntrySafeRefPool = le_mem_InitStaticPool(destNatEntrySafeRefPool,
                                  TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntrySafeRef_t));

    DestNatChangePool = le_mem_CreatePool("DestNatChangePool", sizeof(taf_net_DestNatChangeInd_t));

    // 4. Initiate the reference map.
    destNatEntryListRefMap = le_ref_InitStaticMap(destNatEntryListRefMap, TAF_DCS_PROFILE_LIST_MAX_ENTRY);

    destNatEntrySafeRefMap = le_ref_InitStaticMap(destNatEntrySafeRefMap, TAF_NET_MAX_NAT_ENTRY);

    // 5. Get the DataFactory and staticNatManager instances
    if (staticNatManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#ifdef TARGET_SA515M
        auto initCb = std::bind(&taf_Nat::onInitComplete, this, std::placeholders::_1);
        staticNatManager = dataFactory.getNatManager(telux::data::OperationType::DATA_LOCAL, initCb);
#else
        staticNatManager = dataFactory.getNatManager(telux::data::OperationType::DATA_LOCAL);
#endif
    }

    if(staticNatManager == nullptr )
    {
        LE_INFO("Nat manager initialize error...");
        return ;
    }

#ifdef TARGET_SA515M
    // 6. Check if subsystem status
    std::unique_lock<std::mutex> lck(mMutex);

    telux::common::ServiceStatus subSystemStatus = staticNatManager->getServiceStatus();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        LE_INFO("Nat manager initialize...");
        conVar.wait(lck, [this]{return this->IsSubSystemStatusUpdated;});
        subSystemStatus = staticNatManager->getServiceStatus();
    }

    //At this point, initialization should be either AVAILABLE or FAIL
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("SNAT Manager initialization failed");
        staticNatManager = nullptr;
        return ;
    }
#endif
    isReady = staticNatManager->isSubsystemReady();

    if(isReady == false)
    {
        LE_INFO("Nat component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = staticNatManager->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("nat component is ready...");
    }
    else
    {
        LE_CRIT("unable to init nat component!");
    }

    return;
}

/*======================================================================

 FUNCTION        taf_Nat::GetInstance

 DESCRIPTION     Get the instance of Nat.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      None

 RETURN VALUE    taf_Nat &

 SIDE EFFECTS

======================================================================*/
taf_Nat &taf_Nat::GetInstance()
{
    static taf_Nat instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_Nat::FirstLayerDestNatChangeHandler

 DESCRIPTION     The first layer handler function for destination nat change.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for destination nat change.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Nat::FirstLayerDestNatChangeHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_net_DestNatChangeHandlerFunc_t handlerFunc = (taf_net_DestNatChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_net_DestNatChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}
#ifdef TARGET_SA515M
/*======================================================================

 FUNCTION        taf_Nat::onInitComplete

 DESCRIPTION     Call back function of getNatManager.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] telux::common::ServiceStatus status : Nat manager service status.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Nat::onInitComplete(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IsSubSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif
/*======================================================================

 FUNCTION        taf_Nat::AddDestNatEntry

 DESCRIPTION     Add destination NAT entry by invoking telsdk API.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile Id which bring up the rmnet interface.
                 [IN] const char *priIpAddrPtr: The private IP address.
                 [IN] uint16_t priPort: The private port.
                 [IN] uint16_t globalPort: The global port.
                 [IN] taf_net_IpProto_t ipProto: The ip protocol number.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid private ip address.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Nat::AddDestNatEntry(uint32_t profileId, const char *priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto)
{
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif
    struct telux::data::net::NatConfig snatConfig;
    taf_net_DestNatChangeInd_t *reportPtr = NULL;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(priIpAddrPtr == NULL, LE_BAD_PARAMETER, "priIpAddrPtr is NULL");

    if( inet_pton(AF_INET, priIpAddrPtr, &(addr.sin_addr)) != 1 &&
        inet_pton(AF_INET6, priIpAddrPtr, &(addr6.sin6_addr)) != 1)
    {
        LE_ERROR("invalid ip address");
        return LE_BAD_PARAMETER;
    }

    snatConfig.addr= priIpAddrPtr;
    snatConfig.port = priPort;
    snatConfig.globalPort = globalPort;
    snatConfig.proto = (uint8_t)ipProto;

    NatSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafNatCallback> addStaticNatEntryCb = std::make_shared<tafNatCallback>();

    auto  addStaticEntryRespCb = std::bind(&tafNatCallback::onResponseCallback, addStaticNatEntryCb, std::placeholders::_1);
#ifdef TARGET_SA515M
    Status status = staticNatManager->addStaticNatEntry(profileId, snatConfig, addStaticEntryRespCb,slot);
#else
    Status status = staticNatManager->addStaticNatEntry(profileId, snatConfig, addStaticEntryRespCb);
#endif
    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = NatSyncPromise.get_future();
        result = futureResult.get();
        if(result == LE_OK)
        {
            reportPtr = (taf_net_DestNatChangeInd_t*)le_mem_ForceAlloc(DestNatChangePool);
            reportPtr->profileId = profileId;
            reportPtr->action = TAF_NET_ADD;
            le_event_ReportWithRefCounting(DestNatChangeEvId, (void*)reportPtr);
            return LE_OK;
        }
    }
    else
    {
        LE_ERROR( "ERROR - Failed to send add static entry, Status:%d ", static_cast<int>(status));
    }

    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_Nat::RemoveDestNatEntry

 DESCRIPTION     Remove destination NAT entry by invoking telsdk API.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile Id which bring up the rmnet interface.
                 [IN] const char *priIpAddrPtr: The private IP address.
                 [IN] uint16_t priPort: The private port.
                 [IN] uint16_t globalPort: The global port.
                 [IN] taf_net_IpProto_t ipProto: The ip protocol number.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid private ip address.
                     LE_FAULT:         Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Nat::RemoveDestNatEntry(uint32_t profileId, const char *priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto)
{
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif
    struct telux::data::net::NatConfig snatConfig;
    taf_net_DestNatChangeInd_t *reportPtr = NULL;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(priIpAddrPtr == NULL, LE_BAD_PARAMETER, "priIpAddrPtr is NULL");

    if( inet_pton(AF_INET, priIpAddrPtr, &(addr.sin_addr)) != 1 &&
        inet_pton(AF_INET6, priIpAddrPtr, &(addr6.sin6_addr)) != 1)
    {
        LE_ERROR("invalid ip address");
        return LE_BAD_PARAMETER;
    }

   // fix telsdk bug:when the dest nat entry exists,then call removeStaticNatEntry,telsdk always return OK
    if(!IsDestNatEntryPresent(profileId, priIpAddrPtr, priPort, globalPort, ipProto))
    {
        LE_ERROR("Can't find the destination NAT entry");
        return LE_FAULT;
    }

    //if destination NAT entry exists,remove it
    snatConfig.addr= priIpAddrPtr;
    snatConfig.port = priPort;
    snatConfig.globalPort = globalPort;
    snatConfig.proto = (uint8_t)ipProto;

    NatSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafNatCallback> addStaticNatEntryCb = std::make_shared<tafNatCallback>();

    auto  addStaticEntryRespCb = std::bind(&tafNatCallback::onResponseCallback, addStaticNatEntryCb, std::placeholders::_1);
#ifdef TARGET_SA515M
    Status status = staticNatManager->removeStaticNatEntry(profileId, snatConfig, addStaticEntryRespCb,slot);
#else
    Status status = staticNatManager->removeStaticNatEntry(profileId, snatConfig, addStaticEntryRespCb);
#endif
    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = NatSyncPromise.get_future();
        result = futureResult.get();
        if(result == LE_OK)
        {
            reportPtr = (taf_net_DestNatChangeInd_t*)le_mem_ForceAlloc(DestNatChangePool);
            reportPtr->profileId = profileId;
            reportPtr->action = TAF_NET_DELETE;
            le_event_ReportWithRefCounting(DestNatChangeEvId, (void*)reportPtr);
            return LE_OK;
        }
    }
    else
    {
        LE_ERROR( "ERROR - Failed to send remove static entry, Status:%d ", static_cast<int>(status));
    }

    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_Nat::IsDestNatEntryPresent

 DESCRIPTION     Check whether the destination NAT entry is present or not.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile Id which bring up the rmnet interface.
                 [IN] const char *priIpAddrPtr: The private IP address.
                 [IN] uint16_t priPort: The private port.
                 [IN] uint16_t globalPort: The global port.
                 [IN] taf_net_IpProto_t ipProto: The ip protocol number.

 RETURN VALUE    bool
                      true: destination nat entry is present.
                      false:destination nat entry is not present.

 SIDE EFFECTS

======================================================================*/
bool taf_Nat::IsDestNatEntryPresent(uint32_t profileId, const char* priIpAddrPtr, uint16_t priPort, uint16_t globalPort, taf_net_IpProto_t ipProto)
{
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif
    TAF_ERROR_IF_RET_VAL(staticNatManager == NULL, false, "staticNatManager is null");

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
#ifdef TARGET_SA515M
    telux::common::Status status = staticNatManager->requestStaticNatEntries( profileId, tafNatCallback::onNatListResponse,slot);
#else
    telux::common::Status status = staticNatManager->requestStaticNatEntries( profileId, tafNatCallback::onNatListResponse);
#endif
    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafNatCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

        if(tafNatCallback::destNatEntryInfo.size() == 0)
        {
            LE_DEBUG("profileId %d has no destination NAT entry", profileId);
            return false;
        }

        for (auto info : tafNatCallback::destNatEntryInfo)
        {
            if(info.port == priPort && info.globalPort == globalPort && info.proto == ipProto &&
               strncmp(info.addr.c_str(),priIpAddrPtr,TAF_DCS_USER_NAME_MAX_LEN) == 0)
            {
                LE_DEBUG("the dest nat entry exists in the system");
                return true;
            }
        }
    }
    else
    {
        LE_ERROR("Request static nat entry list failed, status: %d",int(status));
        return false;
    }
    return false;
}

/*======================================================================

 FUNCTION        taf_Nat::GetDestNatEntryList

 DESCRIPTION     Get a reference of an destination NAT entry list.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile Id.

 RETURN VALUE    taf_net_DestNatEntryListRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_net_DestNatEntryListRef_t taf_Nat::GetDestNatEntryList(uint32_t profileId)
{
#ifdef TARGET_SA515M
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
#endif
    le_ref_IterRef_t iterRef;
    bool isAdded = false;
    taf_DestNatEntryList_t* existedDestNatEntryList;

    TAF_ERROR_IF_RET_VAL(staticNatManager == NULL, nullptr, "staticNatManager is null");

    std::shared_ptr<tafNatCallback> staticNatEntryListCb = std::make_shared<tafNatCallback>();

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
#ifdef TARGET_SA515M
    telux::common::Status status = staticNatManager->requestStaticNatEntries( profileId, tafNatCallback::onNatListResponse,slot);
#else
    telux::common::Status status = staticNatManager->requestStaticNatEntries( profileId, tafNatCallback::onNatListResponse);
#endif
    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {1, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafNatCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, nullptr, "Wait semaphore timeout\n");
        std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());
        if(tafNatCallback::destNatEntryInfo.size() == 0)
        {
            LE_DEBUG("profileId %d has no destination NAT entry", profileId);
            return NULL;
        }

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(destNatEntryListRefMap);

        while (!isAdded && (le_ref_NextNode(iterRef) == LE_OK))
        {
            existedDestNatEntryList = (taf_DestNatEntryList_t*) le_ref_GetValue(iterRef);

            if (existedDestNatEntryList->profileId == profileId)
            {
                isAdded = true;
                break;
            }
        }

        // if the NAT entry list already exists in map ,only update the data,else add it
        if(isAdded)
        {
             CleanListRef((taf_net_DestNatEntryListRef_t)le_ref_GetSafeRef(iterRef));
        }

        taf_DestNatEntryList_t* destNatEntriesList = (taf_DestNatEntryList_t*)le_mem_ForceAlloc(destNatEntryListPool);
        destNatEntriesList->destNatEntryList = LE_SLS_LIST_INIT;
        destNatEntriesList->safeRefList = LE_SLS_LIST_INIT;
        destNatEntriesList->profileId = profileId;
        destNatEntriesList->currPtr = NULL;

        taf_DestNatEntry_t* destNatEntryPtr;

        for (auto info : tafNatCallback::destNatEntryInfo)
        {
            destNatEntryPtr = (taf_DestNatEntry_t*)le_mem_ForceAlloc(destNatEntryPool);
            destNatEntryPtr->info.port=info.port;
            destNatEntryPtr->info.globalPort=info.globalPort;
            destNatEntryPtr->info.proto=info.proto;
            le_utf8_Copy(destNatEntryPtr->info.addr, info.addr.c_str(), TAF_NET_IP_ADDR_MAX_LEN, NULL);
            destNatEntryPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(destNatEntriesList->destNatEntryList), &(destNatEntryPtr->link));
        }

        return (taf_net_DestNatEntryListRef_t)le_ref_CreateRef(destNatEntryListRefMap, (void*)destNatEntriesList);

    }
    else
    {
        LE_ERROR("Request static nat entry list failed, status: %d",int(status));
        return NULL;
    }

}

/*======================================================================

 FUNCTION        taf_Nat::GetFirstDestNatEntry

 DESCRIPTION     Get the reference of the first destination nat entry from a list.

 DEPENDENCIES    Initialization of an destination nat entry list

 PARAMETERS      [IN] taf_net_DestNatEntryListRef_t destNatEntryListRef: The destination nat entry list reference.

 RETURN VALUE    taf_net_DestNatEntryRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_DestNatEntryRef_t taf_Nat::GetFirstDestNatEntry
(
    taf_net_DestNatEntryListRef_t destNatEntryListRef
)
{
    taf_DestNatEntryList_t* listPtr = (taf_DestNatEntryList_t*)le_ref_Lookup(destNatEntryListRefMap,
        destNatEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", destNatEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->destNatEntryList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Empty list");

    taf_DestNatEntry_t* destNatEntryPtr = CONTAINER_OF(linkPtr, taf_DestNatEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_DestNatEntrySafeRef_t* safeRefPtr = (taf_DestNatEntrySafeRef_t*)le_mem_ForceAlloc(destNatEntrySafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(destNatEntrySafeRefMap, (void*)destNatEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_DestNatEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Nat::GetNextDestNatEntry

 DESCRIPTION     Get the reference of the next destination nat entry from a list.

 DEPENDENCIES    Initialization of an destination nat entry list

 PARAMETERS      [IN] taf_net_DestNatEntryListRef_t destNatEntryListRef: The destination nat entry list reference.

 RETURN VALUE    taf_net_DestNatEntryRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_net_DestNatEntryRef_t taf_Nat::GetNextDestNatEntry
(
    taf_net_DestNatEntryListRef_t destNatEntryListRef
)
{
    taf_DestNatEntryList_t* listPtr = (taf_DestNatEntryList_t*)le_ref_Lookup(destNatEntryListRefMap,
        destNatEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", destNatEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->destNatEntryList), listPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_DestNatEntry_t* destNatEntryPtr = CONTAINER_OF(linkPtr, taf_DestNatEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_DestNatEntrySafeRef_t* safeRefPtr = (taf_DestNatEntrySafeRef_t*)le_mem_ForceAlloc(destNatEntrySafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(destNatEntrySafeRefMap, (void*)destNatEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_DestNatEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_Nat::GetDestNatEntryDetails

 DESCRIPTION     Get the infomation of an destination nat entry from a reference.

 DEPENDENCIES    Initialization of an destination nat entry list and get a safe reference of an destination nat entry.

 PARAMETERS      [IN] taf_net_DestNatEntryRef_t destNatEntryRef:
                          The destination nat entry reference.
                 [OUT] char* privateIpAddrPtr:                    The private ip address.
                 [IN] size_t privateIpAddrPtrSize:                The private ip address length.
                 [OUT] uint16_t* privatePort:                     The private port.
                 [OUT] uint16_t* globalPort:                      The global port.
                 [OUT] taf_net_IpProto_t* proto:                  The IP protocol number.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Nat::GetDestNatEntryDetails
(
    taf_net_DestNatEntryRef_t  destNatEntryRef,
    char* privateIpAddrPtr,
    size_t privateIpAddrPtrSize,
    uint16_t* privatePort,
    uint16_t* globalPort,
    taf_net_IpProto_t* proto
)
{
    //uint8_t protocol;
    TAF_ERROR_IF_RET_VAL(destNatEntryRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(destNatEntryRef)");

    TAF_ERROR_IF_RET_VAL(privateIpAddrPtr == nullptr, LE_BAD_PARAMETER,
        "Null ptr(privateIpAddrPtr)");

    TAF_ERROR_IF_RET_VAL(privateIpAddrPtrSize < TAF_NET_IP_ADDR_MAX_LEN, LE_BAD_PARAMETER,
        "Invalid para(privateIpAddrPtrSize: %d < %d)", privateIpAddrPtrSize, TAF_NET_IP_ADDR_MAX_LEN);

    TAF_ERROR_IF_RET_VAL(privatePort == nullptr, LE_BAD_PARAMETER,
        "Null ptr(privatePort)");

    TAF_ERROR_IF_RET_VAL(globalPort == nullptr, LE_BAD_PARAMETER,
        "Null ptr(globalPort)");

    TAF_ERROR_IF_RET_VAL(proto == nullptr, LE_BAD_PARAMETER,
        "Null ptr(proto)");

    taf_DestNatEntry_t* destNatEntryPtr = (taf_DestNatEntry_t*)le_ref_Lookup(destNatEntrySafeRefMap, destNatEntryRef);
    TAF_ERROR_IF_RET_VAL(destNatEntryPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    *privatePort = destNatEntryPtr->info.port;
    *globalPort = destNatEntryPtr->info.globalPort;
    *proto = MapIPProtocol(destNatEntryPtr->info.proto);

    le_utf8_Copy(privateIpAddrPtr, destNatEntryPtr->info.addr, TAF_NET_IP_ADDR_MAX_LEN, NULL);

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_Nat::DeleteDestNatEntryList

 DESCRIPTION     Delete a reference of an destination nat entry list.

 DEPENDENCIES    Initialization of an destination nat entry list

 PARAMETERS      [IN] taf_net_DestNatEntryListRef_t destNatEntryListRef: The destination nat entry list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Fail.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Nat::DeleteDestNatEntryList
(
    taf_net_DestNatEntryListRef_t destNatEntryListRef
)
{

    TAF_ERROR_IF_RET_VAL(destNatEntryListRef == nullptr, LE_BAD_PARAMETER, "Null reference(destNatEntryListRef)");

    return CleanListRef(destNatEntryListRef);
}

/*======================================================================

 FUNCTION        taf_Nat::CleanListRef

 DESCRIPTION     Clean the destination nat entry list stored in the map.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile Id which bring up the rmnet interface.
                 [IN] const char *priIpAddrPtr: The private IP address.
                 [IN] uint16_t priPort: The private port.
                 [IN] uint16_t globalPort: The global port.
                 [IN] taf_net_IpProto_t ipProto: The ip protocol number.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
le_result_t taf_Nat::CleanListRef
(
    taf_net_DestNatEntryListRef_t destNatEntryListRef
)
{
    taf_DestNatEntry_t* destNatEntryPtr;
    taf_DestNatEntrySafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(destNatEntryListRef == nullptr, LE_BAD_PARAMETER, "Null reference(destNatEntryListRef)");

    taf_DestNatEntryList_t* listPtr = (taf_DestNatEntryList_t*)le_ref_Lookup(destNatEntryListRefMap,
        destNatEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(listPtr->destNatEntryList))) != NULL)
    {
        destNatEntryPtr = CONTAINER_OF(linkPtr, taf_DestNatEntry_t, link);
        le_mem_Release(destNatEntryPtr);
    }

    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_DestNatEntrySafeRef_t, link);
        le_ref_DeleteRef(destNatEntrySafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(destNatEntryListRefMap, destNatEntryListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_Nat::IsRmnetBringUp

 DESCRIPTION     Check whether the rmnet interface is bringed up or not corresponding this profile id.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint32_t profileId: The profile id.

 RETURN VALUE    bool
                     true:  the rmnet interface is bringed up.
                     false: the rmnet interface is not bringed up.

 SIDE EFFECTS

======================================================================*/
bool taf_Nat::IsRmnetBringUp(uint32_t profileId)
{
    taf_dcs_ProfileRef_t profileRef=NULL;
    le_result_t result;
    char interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN];

    profileRef=taf_dcs_GetProfile(profileId);

    if(profileRef == NULL)
        return false;

    result=taf_dcs_GetInterfaceName(profileRef, interfaceName, TAF_NET_INTERFACE_NAME_MAX_LEN);

    if(result != LE_OK)
        return false;

    return true;
}

/*======================================================================

 FUNCTION        taf_Nat::MapIPProtocol

 DESCRIPTION     Map the enum from telux::data::IpProtocol to taf_net_IpProto_t.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] telux::data::IpProtocol iptype: The IP protocol number.
 RETURN VALUE    taf_net_IpProto_t.

 SIDE EFFECTS

======================================================================*/
taf_net_IpProto_t taf_Nat::MapIPProtocol(telux::data::IpProtocol iptype)
{
    return taf_net_IpProto_t(iptype);
}

/*======================================================================

 FUNCTION        tafNatCallback::onResponseCallback

 DESCRIPTION     Call back function for adding or removing a static nat entry.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafNatCallback::onResponseCallback(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafNat = taf_Nat::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafNat.NatSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafNatCallback::onNatListResponse

 DESCRIPTION     Call back function for request static nat entry list.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] const std::vector<telux::data::net::NatConfig> &snatEntries:
                      The destination nat entry list.
                 [IN] telux::common::ErrorCode error: error code.
 RETURN VALUE    None.

 SIDE EFFECTS

======================================================================*/
void tafNatCallback::onNatListResponse(const std::vector<telux::data::net::NatConfig> &snatEntries,
                                       telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> tafNatCallback --> onNatListResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
    }

    destNatEntryInfo.assign(snatEntries.begin(), snatEntries.end());

    le_sem_Post(semaphore);
}