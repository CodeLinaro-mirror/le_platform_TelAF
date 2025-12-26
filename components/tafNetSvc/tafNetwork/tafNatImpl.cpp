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
#include "tafNatImpl.hpp"
#include "tafNetworkImpl.hpp"
#include "tafNetUtility.hpp"
#include <arpa/inet.h>
#include "tafSvcIF.hpp"
#include "taf_pa_nat.hpp"

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(destNatEntryListPool, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_DestNatEntryList_t));

LE_MEM_DEFINE_STATIC_POOL(destNatEntryPool, TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntry_t));

LE_MEM_DEFINE_STATIC_POOL(destNatEntrySafeRefPool, TAF_NET_MAX_NAT_ENTRY, sizeof(taf_DestNatEntrySafeRef_t));

LE_REF_DEFINE_STATIC_MAP(destNatEntryListRefMap, TAF_DCS_PROFILE_LIST_MAX_ENTRY);

LE_REF_DEFINE_STATIC_MAP(destNatEntrySafeRefMap, TAF_NET_MAX_NAT_ENTRY);


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
    le_result_t isReady = LE_OK;

    isReady =  PA_TO_LE_RESULT(taf_pa_nat_Init());

    if(isReady == LE_OK)
    {
        LE_INFO("natManager component is ready...");
    }
    else
    {
        LE_CRIT("unable to init natManager component!");
    }

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
    uint8_t slotId = DEFAULT_SLOT_ID_1;

    taf_net_DestNatChangeInd_t *reportPtr = NULL;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(priIpAddrPtr == NULL, LE_BAD_PARAMETER, "priIpAddrPtr is NULL");

    if( inet_pton(AF_INET, priIpAddrPtr, &(addr.sin_addr)) != 1 &&
        inet_pton(AF_INET6, priIpAddrPtr, &(addr6.sin6_addr)) != 1)
    {
        LE_ERROR("invalid ip address");
        return LE_BAD_PARAMETER;
    }

    // Prepare PA NAT config
    taf_pa_net_NatConfig_t natConfig;
    le_utf8_Copy(natConfig.addr, priIpAddrPtr, sizeof(natConfig.addr), NULL);
    natConfig.port = priPort;
    natConfig.globalPort = globalPort;
    natConfig.proto = (uint8_t)ipProto;

    // Call PA layer function
    result = PA_TO_LE_RESULT(taf_pa_nat_AddDestNatEntry(profileId, slotId, &natConfig));

    if (result == LE_OK)
    {
        reportPtr = (taf_net_DestNatChangeInd_t*)le_mem_ForceAlloc(DestNatChangePool);
        reportPtr->profileId = profileId;
        reportPtr->action = TAF_NET_ADD;
        le_event_ReportWithRefCounting(DestNatChangeEvId, (void*)reportPtr);
        return LE_OK;
    }
    else
    {
        LE_ERROR("ERROR - Failed to add destination NAT entry, result: %d", static_cast<int>(result));
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
    uint8_t slotId = DEFAULT_SLOT_ID_1;

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
    // Prepare PA NAT config
    taf_pa_net_NatConfig_t natConfig;
    le_utf8_Copy(natConfig.addr, priIpAddrPtr, sizeof(natConfig.addr), NULL);
    natConfig.port = priPort;
    natConfig.globalPort = globalPort;
    natConfig.proto = (uint8_t)ipProto;

    // Call PA layer function
    result = PA_TO_LE_RESULT(taf_pa_nat_RemoveDestNatEntry(profileId, slotId, &natConfig));

    if (result == LE_OK)
    {
        reportPtr = (taf_net_DestNatChangeInd_t*)le_mem_ForceAlloc(DestNatChangePool);
        reportPtr->profileId = profileId;
        reportPtr->action = TAF_NET_DELETE;
        le_event_ReportWithRefCounting(DestNatChangeEvId, (void*)reportPtr);
        return LE_OK;
    }
    else
    {
        LE_ERROR("ERROR - Failed to remove destination NAT entry, result: %d", static_cast<int>(result));
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
    uint8_t slotId = DEFAULT_SLOT_ID_1;
    le_result_t result;

    std::vector<taf_pa_net_NatConfig_t> natEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_nat_QueryDestNatEntryList(profileId, slotId, natEntryInfo));

    if (result == LE_OK)
    {
        if(natEntryInfo.size() == 0)
        {
            LE_DEBUG("profileId %d has no destination NAT entry", profileId);
            return false;
        }

        for (auto info : natEntryInfo)
        {
            if(info.port == priPort && info.globalPort == globalPort && info.proto == (uint8_t)ipProto &&
               strncmp(info.addr, priIpAddrPtr, TAF_DCS_USER_NAME_MAX_LEN) == 0)
            {
                LE_DEBUG("the dest nat entry exists in the system");
                return true;
            }
        }
    }
    else
    {
        LE_ERROR("Request static nat entry list failed, result: %d", int(result));
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
    uint8_t slotId = DEFAULT_SLOT_ID_1;
    le_ref_IterRef_t iterRef;
    bool isAdded = false;
    taf_DestNatEntryList_t* existedDestNatEntryList;
    le_result_t result;

    std::vector<taf_pa_net_NatConfig_t> natEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_nat_QueryDestNatEntryList(profileId, slotId, natEntryInfo));

    if (result == LE_OK)
    {
        if(natEntryInfo.size() == 0)
        {
            LE_DEBUG("profileId %d has no destination NAT entry", profileId);
            return NULL;
        }

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(destNatEntryListRefMap);

        while (!isAdded && (le_ref_NextNode(iterRef) == LE_OK))
        {
            existedDestNatEntryList = (taf_DestNatEntryList_t*) le_ref_GetValue(iterRef);
            TAF_ERROR_IF_RET_VAL(existedDestNatEntryList == NULL, nullptr, "entry list is null");

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

        for (auto info : natEntryInfo)
        {
            destNatEntryPtr = (taf_DestNatEntry_t*)le_mem_ForceAlloc(destNatEntryPool);
            destNatEntryPtr->info.port=info.port;
            destNatEntryPtr->info.globalPort=info.globalPort;
            destNatEntryPtr->info.proto=info.proto;
            le_utf8_Copy(destNatEntryPtr->info.addr, info.addr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
            destNatEntryPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(destNatEntriesList->destNatEntryList), &(destNatEntryPtr->link));
        }

        return (taf_net_DestNatEntryListRef_t)le_ref_CreateRef(destNatEntryListRefMap, (void*)destNatEntriesList);

    }
    else
    {
        LE_ERROR("Request static nat entry list failed, result: %d",int(result));
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
        "Invalid para(privateIpAddrPtrSize: %" PRIuS " < %d)", privateIpAddrPtrSize, TAF_NET_IP_ADDR_MAX_LEN);

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

 PARAMETERS      [IN] taf_net_DestNatEntryListRef_t destNatEntryListRef: The destination nat entry list reference

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

 DESCRIPTION     Map the enum from uint8_t to taf_net_IpProto_t.

 DEPENDENCIES    The initialization of Nat.

 PARAMETERS      [IN] uint8_t iptype: The IP protocol number.
 RETURN VALUE    taf_net_IpProto_t.

 SIDE EFFECTS

======================================================================*/
taf_net_IpProto_t taf_Nat::MapIPProtocol(uint8_t iptype)
{
    if(iptype == 6)
    {
        return TAF_NET_TCP;
    }
    else
    {
        return TAF_NET_UDP;
    }
}

