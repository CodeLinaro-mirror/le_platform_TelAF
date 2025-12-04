/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include "tafVlanImpl.hpp"
#include "tafNetworkImpl.hpp"
#include "tafNetUtility.hpp"
#include "tafSvcIF.hpp"
#include <arpa/inet.h>

#define OPERATION_TIMEOUT 30

using namespace tafsvc;

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

LE_MEM_DEFINE_STATIC_POOL(vlanHwAccelerationStateEvtPool, TAF_NET_MAX_VLAN_INTERFACE,
                                                                sizeof(VlanHwAccelerationState_t));

#define MAX_SLOT_NUM   2

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes.
 */
//--------------------------------------------------------------------------------------------------
void PowerStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        taf_pa_net_RegVlanListener();
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        taf_pa_net_DeregVlanListener();
    }

}



std::map<uint8_t, std::list<std::pair<int, int>>> taf_Vlan::slotVlanMappingInfo;
std::vector<taf_pa_Vlan_t> taf_Vlan::vlanPAEntryInfo;

//The VLAN hardware acceleration state.
void HwAccelarationUpdatePAHandler(taf_pa_vlan_hwacc_state_t state)
{
    VlanHwAccelerationState_t *reportPtr = NULL;
    auto &tafVlan = taf_Vlan::GetInstance();
    reportPtr = static_cast<VlanHwAccelerationState_t *>(
        le_mem_ForceAlloc(tafVlan.vlanHwAccelerationStateEvtPool));
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    if(state == TAF_PA_VLAN_HW_ACC_STATE_ACTIVE )
    {    reportPtr->state =  TAF_NET_VLAN_HW_ACC_STATE_ACTIVE;   }
    else
    {    reportPtr->state =  TAF_NET_VLAN_HW_ACC_STATE_INACTIVE;   };

    LE_DEBUG("VlanHWAccelerationState: %d", reportPtr->state);
    le_event_ReportWithRefCounting(tafVlan.vlanHwAccelerationStateEvtId, (void *)reportPtr);
}


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

    le_result_t isReady = LE_OK;
    
    isReady =  PA_TO_LE_RESULT(taf_pa_vlan_Init());

    if(isReady == LE_OK)
    {
        LE_INFO("vlanManager component is ready...");
    }
    else
    {
        LE_CRIT("unable to init vlanManager component!");
    }

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

    // Event and data pool for HW accleration related events
    vlanHwAccelerationStateEvtId   = le_event_CreateIdWithRefCounting("VlanHwAccelerationStateEvt");
    vlanHwAccelerationStateEvtPool = le_mem_InitStaticPool(vlanHwAccelerationStateEvtPool,
                                                       TAF_NET_MAX_VLAN_INTERFACE,
                                                       sizeof(VlanHwAccelerationState_t));

    // Add a handler for client session close
    le_msg_AddServiceCloseHandler( taf_net_GetServiceRef(), ClientCloseSessionHandler, NULL );

    // Add power state change handle.
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        taf_pa_net_RegVlanListener();
    }

    taf_pa_vlan_RegHwAccelarationUpdateHandler(HwAccelarationUpdatePAHandler);

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
        //To support backward compatibility for network type
        vlanPtr->nwType=TAF_NET_NETWORK_UNKNOWN;
        //set vlan bind values to default values
        //because for backhaul type WWAN profile/slot are not needed
        vlanPtr->vlanBindConfig.profileId = -1;
        vlanPtr->vlanBindConfig.slotId = DEFAULT_SLOT_ID_1;
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
    taf_net_InterfaceRef_t interfaceRef
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
        //To support backward compatibility for network type
        vlanPtr->nwType=TAF_NET_NETWORK_UNKNOWN;
        //set vlan bind values to default values
        //because for backhaul type WWAN profile/slot are not needed
        vlanPtr->vlanBindConfig.profileId = -1;
        vlanPtr->vlanBindConfig.slotId = DEFAULT_SLOT_ID_1;
        vlanPtr->vlanBindConfig.vlanIdBackhaul = -1;
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
    taf_pa_Vlan_t config;
    bool interfacePresent = false;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    config.vlanId = vlanPtr->vlanId;
    config.isAccelerated = vlanPtr->isAccelerated;
    config.priority = vlanPtr->priority;
    config.nwType = static_cast<taf_pa_vlan_network_type_t>(vlanPtr->nwType);

    //else pick default value sdk value which are nwType=LAN and createBridge=true
    LE_DEBUG("NetworkType %d ", static_cast<int>(config.nwType));

    interfacePresent=IsVlanInterfacePresentInDb(config.vlanId, ifType);
    if(interfacePresent)
        return LE_DUPLICATE;

    taf_pa_vlan_iface_type_t ifPAType = static_cast<taf_pa_vlan_iface_type_t>(ifType);
    result = PA_TO_LE_RESULT(taf_pa_net_AddVlanInterface(config,ifPAType));
    return result;

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
    taf_net_NetworkType_t nwType
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
    taf_net_BackhaulType_t bhType
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

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL, LE_BAD_PARAMETER, "vlanRef is null");

    vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);

    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "can't get vlan info");

    taf_pa_Vlan_t config;
    config.vlanId = vlanPtr->vlanId;


    taf_pa_vlan_iface_type_t ifPAType = static_cast<taf_pa_vlan_iface_type_t>(ifType);
    result = PA_TO_LE_RESULT(taf_pa_net_RemoveVlanInterface(config,ifPAType));
    return result;

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
    TAF_ERROR_IF_RET_VAL(isAccelerated == NULL, false, "isAccelerated is null");

    le_result_t  result;

    std::vector<taf_pa_Vlan_t> vlanEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanInfo(vlanEntryInfo));
    vlanPAEntryInfo.clear();

    if (result == LE_OK)
    {
        vlanPAEntryInfo.assign(vlanEntryInfo.begin(), vlanEntryInfo.end());

        if(vlanPAEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan");
            return false;
        }

        for (auto info : vlanPAEntryInfo)
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
        LE_ERROR("Request vlan info failed, result: %d",int(result));
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

    le_result_t  result;

    std::vector<taf_pa_Vlan_t> vlanEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanInfo(vlanEntryInfo));
    vlanPAEntryInfo.clear();

    if (result == LE_OK)
    {
        vlanPAEntryInfo.assign(vlanEntryInfo.begin(), vlanEntryInfo.end());

        if(vlanPAEntryInfo.size() == 0)
        {
            LE_DEBUG("No vlan");
            return false;
        }

        for (auto info : vlanPAEntryInfo)
        {
            //found vlan info
            taf_net_VlanIfType_t ifPAType = static_cast<taf_net_VlanIfType_t>(info.iface);
            if(vlanId == info.vlanId && ifType == ifPAType)
            {
                return true;
            }
        }
    }
    else
    {
        LE_ERROR("Request vlan info failed, result: %d",int(result));
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
    le_result_t  result;

    std::vector<taf_pa_Vlan_t> vlanEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanInfo(vlanEntryInfo));
    vlanPAEntryInfo.clear();

    if (result == LE_OK)
    {
        vlanPAEntryInfo.assign(vlanEntryInfo.begin(), vlanEntryInfo.end());

        if(vlanPAEntryInfo.size() == 0)
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
        std::sort(vlanPAEntryInfo.begin(),vlanPAEntryInfo.end(),
                 sort_vlanId);

        for (auto info : vlanPAEntryInfo)
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
                vlanEntryPtr->info.nwType= (taf_net_NetworkType_t)info.nwType;
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
        LE_ERROR("Request vlan entry list failed, result: %d",int(result));
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

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef:
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
    taf_net_NetworkType_t  *networkType
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

 PARAMETERS      [IN] taf_net_VlanEntryRef_t vlanEntryRef:
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
    le_result_t  result;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , NULL, "vlanRef is null");

    std::vector<taf_pa_Vlan_t> vlanEntryInfo;

    result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanInfo(vlanEntryInfo));
    vlanPAEntryInfo.clear();

    if (result == LE_OK)
    {
        vlanPAEntryInfo.assign(vlanEntryInfo.begin(), vlanEntryInfo.end());

        if(vlanPAEntryInfo.size() == 0)
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

        for (auto info : vlanPAEntryInfo)
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
        LE_ERROR("Request vlan interface list failed, result: %d",int(result));
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

    le_result_t result;
    uint16_t vlanId=0;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    taf_pa_VlanBindConfig_t vlanBindConfig = {};
    taf_pa_Vlan_t vlanConfig = {};
   // fix telsdk bug:when the profile is already bound with VLAN,bindWithProfile api from telsdk
   // always return OK
    vlanId=GetBoundVlanIdFromSlotAndProfile(slotId, profileId);
    if(vlanId !=0)
    {
        LE_ERROR("Profile is already bound with vlan");
        return LE_FAULT;
    }
    LE_INFO("BindVlanWithProfile: no vlanId mapped hence proceed to bind");

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Vlan not found");
    vlanId = vlanPtr->vlanId;

    vlanConfig.vlanId = vlanId;
    vlanBindConfig.slotId = slotId;
    vlanBindConfig.profileId = profileId;

    LE_INFO("bindwithProfile: vlanid %d slotId %d profileId %d", vlanId,slotId,profileId);

    result = PA_TO_LE_RESULT(taf_pa_net_BindWithProfile(vlanConfig,vlanBindConfig));
    return result;

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

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Vlan not found");

    uint8_t slot = vlanPtr->vlanBindConfig.slotId;
    taf_pa_VlanBindConfig_t vlanBindConfig;
    taf_pa_Vlan_t vlanConfig;

    if(vlanPtr->vlanBindConfig.backhaulType == TAF_NET_BH_WWAN)
    {
        vlanId=GetBoundVlanIdFromSlotAndProfile(vlanPtr->vlanBindConfig.slotId,
                                                vlanPtr->vlanBindConfig.profileId);
        if(vlanId !=0)
        {
           LE_ERROR("Profile is already bound with vlan");
           return LE_FAULT;
        }
        vlanBindConfig.slotId = vlanPtr->vlanBindConfig.slotId;
        vlanBindConfig.profileId = vlanPtr->vlanBindConfig.profileId;
    }
    else // for ETH and rest where SIM does not exist.
    {
        uint16_t bhvlanid = GetBackhaulVlanIdBoundWithVlan(
                                              vlanId, vlanPtr->vlanBindConfig.backhaulType, slot);

        LE_INFO("bindVlanFromBackhaul: bhvlanid %d", bhvlanid);
        TAF_ERROR_IF_RET_VAL(bhvlanid != 0, LE_FAULT, "vlan id is already bound to the backhaul");
        vlanBindConfig.vlanIdBackhaul = vlanPtr->vlanBindConfig.vlanIdBackhaul;
    }

    vlanId = vlanPtr->vlanId;

    vlanConfig.vlanId = vlanId;

    switch (vlanPtr->vlanBindConfig.backhaulType)
    {
        case TAF_NET_BH_ETH:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_ETH;
            break;
        case TAF_NET_BH_USB:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_USB;
            break;
        case TAF_NET_BH_WLAN:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_WLAN;
            break;
        case TAF_NET_BH_WWAN:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_WWAN;
            break;
        case TAF_NET_BH_BLE:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_BLE;
            break;
        default:
            LE_ERROR("Invalid backhaul type (%d).", vlanPtr->vlanBindConfig.backhaulType);
            return LE_BAD_PARAMETER;
    }

    result = PA_TO_LE_RESULT(taf_pa_net_BindWithBackhaul(vlanConfig,vlanBindConfig));
    return result;
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
    taf_pa_VlanBindConfig_t vlanBindConfig;
    taf_pa_Vlan_t vlanConfig;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    vlanId = vlanPtr->vlanId;

    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_FAULT, "Invalid vlan id");

    result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting slotId and profileId failed");

    vlanConfig.vlanId = vlanId;
    vlanBindConfig.slotId = slotId;
    vlanBindConfig.profileId = profileId;

    result = PA_TO_LE_RESULT(taf_pa_net_UnbindWithProfile(vlanConfig,vlanBindConfig));
    return result;
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
    taf_net_BackhaulType_t backhaulType;

    taf_pa_VlanBindConfig_t vlanBindConfig;
    taf_pa_Vlan_t vlanConfig;

    TAF_ERROR_IF_RET_VAL(vlanRef == NULL , LE_BAD_PARAMETER, "vlanRef is null");

    taf_Vlan_t* vlanPtr = (taf_Vlan_t*)le_ref_Lookup(vlanRefMap, vlanRef);
    TAF_ERROR_IF_RET_VAL(vlanPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    vlanId = vlanPtr->vlanId;
    backhaulType = vlanPtr->vlanBindConfig.backhaulType;
    uint8_t slot = vlanPtr->vlanBindConfig.slotId;

    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_FAULT, "Invalid vlan id");

    if(backhaulType == TAF_NET_BH_WWAN)
    {
        result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting slotId and profileId failed");
        vlanBindConfig.slotId = slotId;
        vlanBindConfig.profileId = profileId;
        slot = slotId;
    }
    else // for ETH and rest where SIM does not exist.
    {
        uint16_t bhvlanid = GetBackhaulVlanIdBoundWithVlan(vlanId,backhaulType, slot);
        LE_INFO("UnbindVlanFromBackhaul: bhvlanid %d", bhvlanid);
        TAF_ERROR_IF_RET_VAL(bhvlanid == 0, LE_FAULT, "vlan id is not bound to the backhaul");
        vlanBindConfig.vlanIdBackhaul = bhvlanid;
    }

    vlanConfig.vlanId = vlanId;

     switch (vlanPtr->vlanBindConfig.backhaulType)
    {
        case TAF_NET_BH_ETH:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_ETH;
            break;
        case TAF_NET_BH_USB:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_USB;
            break;
        case TAF_NET_BH_WLAN:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_WLAN;
            break;
        case TAF_NET_BH_WWAN:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_WWAN;
            break;
        case TAF_NET_BH_BLE:
            vlanBindConfig.backhaulType = TAF_PA_VLAN_BH_BLE;
            break;
        default:
            LE_ERROR("Invalid backhaul type (%d).", vlanPtr->vlanBindConfig.backhaulType);
            return LE_BAD_PARAMETER;
    }

    result = PA_TO_LE_RESULT(taf_pa_net_UnbindWithBackhaul(vlanConfig,vlanBindConfig));
    return result;
}

/*======================================================================

 FUNCTION        taf_Vlan::GetBackhaulVlanIdBoundWithVlan

 DESCRIPTION     Get the bound backhaul id with vlan.

 DEPENDENCIES    The initialization of Vlan.

 PARAMETERS      [IN] uint16_t vlanId: The vlan Id.
 PARAMETERS      [IN] taf_net_BackhaulType_t backhaulType: The backhaul type.
 PARAMETERS      [IN] slot: The slot Id.

 RETURN VALUE    uint16_t
                    0:       no backhaul binding with this vlan.
                    others:  backhaul id.

 SIDE EFFECTS

======================================================================*/
uint16_t taf_Vlan::GetBackhaulVlanIdBoundWithVlan(uint16_t vlanId,
                                      taf_net_BackhaulType_t backhaulType,
                                      uint8_t slot)
{
    std::vector<taf_pa_VlanBindConfig_t> vlanBindInfo;
    taf_pa_vlan_backhaul_type_t pabackhaulType;
    switch (backhaulType)
    {
        case TAF_NET_BH_ETH:
            pabackhaulType = TAF_PA_VLAN_BH_ETH;
            break;
        case TAF_NET_BH_USB:
            pabackhaulType = TAF_PA_VLAN_BH_USB;
            break;
        case TAF_NET_BH_WLAN:
            pabackhaulType = TAF_PA_VLAN_BH_WLAN;
            break;
        case TAF_NET_BH_WWAN:
            pabackhaulType = TAF_PA_VLAN_BH_WWAN;
            break;
        case TAF_NET_BH_BLE:
            pabackhaulType = TAF_PA_VLAN_BH_BLE;
            break;
        default:
            LE_ERROR("Invalid backhaul type (%d).", backhaulType);
            return LE_BAD_PARAMETER;
    }

    le_result_t result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanToBackhaulMappingList((uint8_t)slot,pabackhaulType,vlanBindInfo));

    if(result == LE_OK)
    {
         LE_INFO("Size of vector VlanBindConfig: %d", (int) vlanBindInfo.size());
         for (auto binding:vlanBindInfo)
         {
             LE_INFO("binding.vlanId: %d, binding.bhInfo.vlanId: %d",
                                                          binding.vlanId,
                                                          binding.vlanIdBackhaul);
             if (binding.vlanId == vlanId) {
                    if (binding.vlanIdBackhaul > 0) {
                        return binding.vlanIdBackhaul;
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

    if(slotVlanMappingInfo[slotId].size() == 0)
    {
        LE_DEBUG("no binding info for this profile");
        return 0;
    }

    if (slotVlanMappingInfo.find(slotId) ==
        slotVlanMappingInfo.end() ||
        slotVlanMappingInfo[slotId].size() == 0)
    {
        LE_DEBUG("no binding info for this profile");
        return 0;
    }

    for (auto info : slotVlanMappingInfo[slotId])
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
        
        if(slotVlanMappingInfo[slotIdIdx].size() == 0)
        {
          LE_DEBUG("no binding info");
          continue;
        }

        if (slotVlanMappingInfo.find(slotIdIdx) ==
            slotVlanMappingInfo.end() ||
            slotVlanMappingInfo[slotIdIdx].size() == 0)
            continue;

        for (auto info : slotVlanMappingInfo[slotIdIdx])
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

    le_result_t result = LE_OK;

    std::list<std::pair<int, int>> vlanMapping;

    result = PA_TO_LE_RESULT(taf_pa_net_QueryVlanMappingList(slotId,vlanMapping));

    if (result != LE_OK)
    {
        LE_ERROR("Error(%d)", (int)result);
        return LE_FAULT;
    }
    slotVlanMappingInfo[slotId].clear();
    if(vlanMapping.size() != 0)
       slotVlanMappingInfo[slotId]=vlanMapping;

    return LE_OK;
}

bool taf_Vlan::sort_vlanId(const taf_pa_Vlan_t& s1, const taf_pa_Vlan_t& s2)
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
taf_net_InterfaceRef_t taf_Vlan::GetInterface
(
     taf_net_VlanIfType_t ifType
)
{
     taf_InterfaceConfig_t* interfacePtr = NULL;
     interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfacePool);
     interfacePtr->ifType=ifType;

     return (taf_net_InterfaceRef_t)le_ref_CreateRef(interfaceRefMap, (void*)interfacePtr);
}

le_result_t taf_Vlan::SetIPPTOperation
(
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_Operation_t  operation
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
    taf_net_InterfaceRef_t  interfaceRef,
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
    taf_net_InterfaceRef_t  interfaceRef,
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

    taf_pa_IpptConfigIn_t ipptConfigIn;
    taf_pa_IpptConfigOut_t ipptConfigOut;

    ipptConfigIn.profileId = profileId;
    ipptConfigIn.slotId = slotId;
    ipptConfigIn.vlanId = vlanId;

    LE_DEBUG("SetIPPassThroughConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("SetIPPassThroughConfig slotId(%d)", (int)slotId);
    LE_DEBUG("SetIPPassThroughConfig profileId(%d)", (int)profileId);

    // Convert operation type
    switch (interfacePtr->passThroughConfig.operation)
    {
        case TAF_NET_IPPT_DISABLE:
            ipptConfigOut.operation = TAF_PA_VLAN_IPPT_DISABLE;
            break;
        case TAF_NET_IPPT_ENABLE:
            ipptConfigOut.operation = TAF_PA_VLAN_IPPT_ENABLE;
            break;
        default:
            ipptConfigOut.operation = TAF_PA_VLAN_IPPT_UNKNOWN;
            break;
    }

    LE_DEBUG("SetIPPassThroughConfig operation(%d)", (int)interfacePtr->passThroughConfig.operation);

    if (interfacePtr->passThroughConfig.operation == TAF_NET_IPPT_ENABLE)
    {
        ipptConfigOut.ifType = static_cast<taf_pa_vlan_iface_type_t>(interfacePtr->passThroughConfig.ifType);
        le_utf8_Copy(ipptConfigOut.macAddr, interfacePtr->passThroughConfig.macAddr, 18, NULL);

        LE_DEBUG("SetIPPassThrough iftype(%d)", (int)interfacePtr->passThroughConfig.ifType);
        LE_DEBUG("SetIPPassThrough Mac %s", interfacePtr->passThroughConfig.macAddr);
    }
    else
    {
        LE_ERROR("IP Pass through disabled or unknown; not filling device config");
    }

    result = PA_TO_LE_RESULT(taf_pa_net_SetIPPassThroughConfig(&ipptConfigIn, &ipptConfigOut));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to set ip pass through, result:%d ", static_cast<int>(result));
        return LE_FAULT;
    }
    else
    {
        LE_INFO("set ip pass through is success...");
    }
    return LE_OK;
}

taf_net_InterfaceRef_t taf_Vlan::GetIPPassThroughConfig
(
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    le_result_t result = LE_OK;
    uint32_t profileId=0;
    uint8_t slotId=0;

    result=GetBoundSlotIdProfileIdFromVlan(vlanId, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "Getting slotId and profileId failed");

    taf_pa_IpptConfigIn_t ipptConfigIn;
    taf_pa_IpptConfigOut_t ipptConfigOut;

    ipptConfigIn.profileId = profileId;
    ipptConfigIn.vlanId = vlanId;
    ipptConfigIn.slotId = slotId;

    LE_DEBUG("GetIPPassThroughConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("GetIPPassThroughConfig slotId(%d)", (int)slotId);
    LE_DEBUG("GetIPPassThroughConfig profileId(%d)", (int)profileId);

    result = PA_TO_LE_RESULT(taf_pa_net_GetIPPassThroughConfig(&ipptConfigIn, &ipptConfigOut));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to get ip pass through, result:%d ", static_cast<int>(result));
        return NULL;
    }
    else
    {
        LE_INFO("get ip pass through is success...");
    }

    // fill values in the REFERENCE to be passed to the client
    interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfaceIPPool);

    // Convert operation type back
    switch (ipptConfigOut.operation)
    {
        case TAF_PA_VLAN_IPPT_DISABLE:
            interfacePtr->passThroughConfig.operation = TAF_NET_IPPT_DISABLE;
            break;
        case TAF_PA_VLAN_IPPT_ENABLE:
            interfacePtr->passThroughConfig.operation = TAF_NET_IPPT_ENABLE;
            break;
        default:
            interfacePtr->passThroughConfig.operation = TAF_NET_IPPT_UNKNOWN;
            break;
    }

    LE_DEBUG("GetIPPassThroughConfig operation(%d)", (int)interfacePtr->passThroughConfig.operation);

    interfacePtr->passThroughConfig.ifType = static_cast<taf_net_VlanIfType_t>(ipptConfigOut.ifType);
    le_utf8_Copy(interfacePtr->passThroughConfig.macAddr, ipptConfigOut.macAddr,
                 TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);

    LE_DEBUG("GetIPPassThroughConfig iftype(%d)", (int)interfacePtr->passThroughConfig.ifType);
    LE_DEBUG("GetIPPassThroughConfig Mac %s", interfacePtr->passThroughConfig.macAddr);

    return (taf_net_InterfaceRef_t)le_ref_CreateRef(interfaceIPRefMap, (void*)interfacePtr);
}


le_result_t taf_Vlan::GetIPPTOperation
(
    taf_net_InterfaceRef_t ifIPRef,
    taf_net_Operation_t*  operation
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
    taf_net_InterfaceRef_t ifIPRef,
    taf_net_VlanIfType_t *ifType,
    char *macAddr, size_t macAddrSize
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap,ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    //if( interfacePtr->passThroughConfig.operation == TAF_NET_IPPT_ENABLE )
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
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t   ifType,
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(interfaceRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceRefMap, interfaceRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    taf_pa_IpConfig_t ipConfig;
    taf_pa_IpConfigParams_t ipConfigParams;

    // Set ipConfig
    ipConfig.ipAssignType = (taf_pa_vlan_ip_assign_type_t)interfacePtr->ipConfig.ipAssignType;
    ipConfig.ipOpr = (taf_pa_vlan_ip_assign_operation_t)interfacePtr->ipConfig.ipOpr;

    LE_DEBUG("SetIPConfig vlanid(%d)", (int)vlanId);
    LE_DEBUG("SetIPConfig ipType(%d)", (int)ipType);

    if(interfacePtr->ipConfig.ipAssignType == TAF_NET_STATIC_IP)
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

        le_utf8_Copy(ipConfig.ipAddrInfo.interfaceAddress,
                     interfacePtr->ipConfig.ipAddrInfo.interfaceAddress, 46, NULL);
        ipConfig.ipAddrInfo.interfaceMask = interfacePtr->ipConfig.ipAddrInfo.interfaceMask;

        le_utf8_Copy(ipConfig.ipAddrInfo.gwAddress,
                     interfacePtr->ipConfig.ipAddrInfo.gwAddress, 46, NULL);
        le_utf8_Copy(ipConfig.ipAddrInfo.primaryDnsAddress,
                     interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress, 46, NULL);
        le_utf8_Copy(ipConfig.ipAddrInfo.secondaryDnsAddress,
                     interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress, 46, NULL);

        LE_DEBUG("SetIPConfig if %s", ipConfig.ipAddrInfo.interfaceAddress);
        LE_DEBUG("SetIPConfig gw %s", ipConfig.ipAddrInfo.gwAddress);
        LE_DEBUG("SetIPConfig pDNS %s", ipConfig.ipAddrInfo.primaryDnsAddress);
        LE_DEBUG("SetIPConfig sDNS %s", ipConfig.ipAddrInfo.secondaryDnsAddress);
    }

    // Set ipConfigParams
    ipConfigParams.ifType = (taf_pa_vlan_iface_type_t)ifType;
    ipConfigParams.vlanId = vlanId;

    if(ipType == TAF_NET_IPV4)
    {
        ipConfigParams.ipFamilyType = TAF_PA_VLAN_IPV4;
    }
    else if (ipType == TAF_NET_IPV6)
    {
        ipConfigParams.ipFamilyType = TAF_PA_VLAN_IPV6;
    }
    else
    {
        return LE_BAD_PARAMETER;
    }

    le_result_t result = PA_TO_LE_RESULT(taf_pa_net_SetIPConfig(&ipConfigParams, &ipConfig));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to set ip config, result:%d ", static_cast<int>(result));
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
    taf_net_InterfaceRef_t  interfaceRef,
    taf_net_IpAssignOperation_t ipOpr,
    taf_net_IpAssignType_t ipAssignType
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
    taf_net_InterfaceRef_t  interfaceRef,
    const taf_net_IpAddressInfo_t*  ipAddrInfo
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
    taf_net_InterfaceRef_t ifIPRef,
    taf_net_IpAssignOperation_t* ipOpr,
    taf_net_IpAssignType_t* ipAssignType
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
    taf_net_InterfaceRef_t ifIPRef,
    taf_net_IpAddressInfo_t*  ipAddrInfo
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;
    TAF_ERROR_IF_RET_VAL(ifIPRef == NULL, LE_BAD_PARAMETER, "interfaceRef is null");

    interfacePtr = (taf_InterfaceConfig_t*)le_ref_Lookup(interfaceIPRefMap, ifIPRef);
    TAF_ERROR_IF_RET_VAL(interfacePtr == NULL, LE_NOT_FOUND, "can't get interface info");

    if(interfacePtr->ipConfig.ipAssignType == TAF_NET_STATIC_IP)
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

taf_net_InterfaceRef_t taf_Vlan::GetIPConfig
(
    taf_net_NetIpType_t  ipType,
    taf_net_VlanIfType_t ifType,
    uint16_t vlanId
)
{
    taf_InterfaceConfig_t *interfacePtr=NULL;

    taf_pa_IpConfig_t ipConfig;
    taf_pa_IpConfigParams_t ipConfigParams;

    // Set ipConfigParams
    ipConfigParams.ifType = (taf_pa_vlan_iface_type_t)ifType;
    ipConfigParams.vlanId = vlanId;
    LE_DEBUG("GetIPConfig vlanid(%d)", (int)vlanId);

    if(ipType == TAF_NET_IPV4)
    {
        ipConfigParams.ipFamilyType = TAF_PA_VLAN_IPV4;
    }
    else if (ipType == TAF_NET_IPV6)
    {
        ipConfigParams.ipFamilyType = TAF_PA_VLAN_IPV6;
    }
    else
    {
        LE_ERROR("ERROR - invalid ip type, error:%d ", static_cast<int>(ipType));
        return NULL;
    }

    le_result_t result = PA_TO_LE_RESULT(taf_pa_net_GetIPConfig(&ipConfigParams, &ipConfig));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to get ip config, result:%d ", static_cast<int>(result));
        return NULL;
    }
    else
    {
        LE_INFO("get ip config is success...");
    }

    // fill values in the REFERENCE to be passed to the client
    interfacePtr = (taf_InterfaceConfig_t*)le_mem_ForceAlloc(interfaceIPPool);

    interfacePtr->ipConfig.ipOpr = (taf_net_IpAssignOperation_t)ipConfig.ipOpr;
    interfacePtr->ipConfig.ipAssignType = (taf_net_IpAssignType_t)ipConfig.ipAssignType;

    LE_DEBUG("GetIPConfig ipOpr(%d)", (int)interfacePtr->ipConfig.ipOpr);
    LE_DEBUG("GetIPConfig ipAssignType(%d)", (int)interfacePtr->ipConfig.ipAssignType);

    if(ipConfig.ipAssignType == TAF_PA_VLAN_STATIC_IP)
    {
        // copy IP address info
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.interfaceAddress,
                     ipConfig.ipAddrInfo.interfaceAddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        interfacePtr->ipConfig.ipAddrInfo.interfaceMask = ipConfig.ipAddrInfo.interfaceMask;

        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.gwAddress,
                     ipConfig.ipAddrInfo.gwAddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress,
                     ipConfig.ipAddrInfo.primaryDnsAddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress,
                     ipConfig.ipAddrInfo.secondaryDnsAddress, TAF_NET_IP_ADDR_MAX_LEN, NULL);

        LE_DEBUG("GetIPConfig if %s", interfacePtr->ipConfig.ipAddrInfo.interfaceAddress);
        LE_DEBUG("GetIPConfig gw %s", interfacePtr->ipConfig.ipAddrInfo.gwAddress);
        LE_DEBUG("GetIPConfig pDNS %s", interfacePtr->ipConfig.ipAddrInfo.primaryDnsAddress);
        LE_DEBUG("GetIPConfig sDNS %s", interfacePtr->ipConfig.ipAddrInfo.secondaryDnsAddress);
    }
    else
    {
        LE_ERROR("IP config is DYNAMIC or UNKNOWN; dont copy address information");
    }

    return (taf_net_InterfaceRef_t)le_ref_CreateRef(interfaceIPRefMap, (void*)interfacePtr);
}

le_result_t taf_Vlan::SetIPPassThroughNatConfig(bool isEnabled)
{

    le_result_t result;

    result = PA_TO_LE_RESULT(taf_pa_net_SetIPPassThroughNatConfig(isEnabled));

    LE_DEBUG("SetIPPassThroughNatConfig %d", static_cast<int>(isEnabled));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to get get ippt NAT config , error:%d ",static_cast<int>(result));
    }
    else
    {
        LE_INFO("set ippt NAT config is success...");
    }
    return result;
}

le_result_t taf_Vlan::GetIPPassThroughNatConfig(bool *isEnabledPtr)
{
    TAF_ERROR_IF_RET_VAL(isEnabledPtr == NULL, LE_BAD_PARAMETER, "isEnabledPtr is null");

    le_result_t result;

    result = PA_TO_LE_RESULT(taf_pa_net_GetIPPassThroughNatConfig(*isEnabledPtr));

    if (result != LE_OK)
    {
        LE_ERROR("ERROR - Failed to get ippt NAT config , error:%d ",static_cast<int>(result));
    }
    else
    {
        LE_INFO("get ippt NAT config is success...");
    }
    LE_DEBUG("GetIPPassThroughNatConfig %d", static_cast<int>(*isEnabledPtr));
    return result;
}

le_result_t taf_Vlan::GetBackhaulPreference(taf_net_BackhaulType_t* bhPrefListPtr,
                                              size_t* bhPrefListSizePtr)
{
    TAF_ERROR_IF_RET_VAL(bhPrefListPtr == NULL, LE_BAD_PARAMETER, "GetBackhaulPreference is null");

    std::vector<taf_pa_vlan_backhaul_type_t> backhaulPref;
    le_result_t result = PA_TO_LE_RESULT(taf_pa_net_GetBackhaulPreference(backhaulPref));

    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
        "Failed to get backhaul pref %d",static_cast<int>(result));

    uint8_t index = 0;
    for (auto pref : backhaulPref)
    {
        switch (pref)
        {
           case TAF_PA_VLAN_BH_ETH:
                   bhPrefListPtr[index] = TAF_NET_BH_ETH;
                   break;
           case TAF_PA_VLAN_BH_USB:
                   bhPrefListPtr[index] =  TAF_NET_BH_USB;
                   break;
           case TAF_PA_VLAN_BH_WLAN:
                   bhPrefListPtr[index] = TAF_NET_BH_WLAN;
                   break;
           case TAF_PA_VLAN_BH_WWAN:
                   bhPrefListPtr[index] = TAF_NET_BH_WWAN;
                   break;
            case TAF_PA_VLAN_BH_BLE:
                   bhPrefListPtr[index] = TAF_NET_BH_BLE;
                   break;
            default:
               LE_DEBUG("Invalid backhaul preference.");
            }
            index++;
        }

    *bhPrefListSizePtr = index;

    return LE_OK;
}

le_result_t taf_Vlan::SetBackhaulPreference(const taf_net_BackhaulType_t* bhPrefListPtr,
                                              size_t bhPrefListSize)
{

    std::vector<taf_pa_vlan_backhaul_type_t> backhaulPref;
    le_result_t result;

    //LE_DEBUG("SetBackhaulPreference %d", static_cast<int>(bhTypeMask));
    for(uint8_t index = 0;index < bhPrefListSize;index++)
        {
            switch (bhPrefListPtr[index])
            {
               case TAF_NET_BH_ETH:
                   backhaulPref.emplace_back(TAF_PA_VLAN_BH_ETH);
                   break;
               case TAF_NET_BH_USB:
                   backhaulPref.emplace_back(TAF_PA_VLAN_BH_USB);
                   break;
               case TAF_NET_BH_WLAN:
                   backhaulPref.emplace_back(TAF_PA_VLAN_BH_WLAN);
                   break;
               case TAF_NET_BH_WWAN:
                   backhaulPref.emplace_back(TAF_PA_VLAN_BH_WWAN);
                   break;
               case TAF_NET_BH_BLE:
                   backhaulPref.emplace_back(TAF_PA_VLAN_BH_BLE);
                   break;
               default:
               LE_DEBUG("Invalid backhaul preference.");
            }
        }

    result = PA_TO_LE_RESULT(taf_pa_net_SetBackhaulPreference(backhaulPref));

    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT,
        "Failed to set backhaul pref %d",static_cast<int>(result));

    return LE_OK;
}
