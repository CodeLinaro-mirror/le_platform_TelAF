/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafThermImpl.cpp
 * @brief      This file describes the implementation method that thermal
 *             service is in use.
 */

#include "tafTherm.hpp"
#include <memory>
#include <vector>
#include <string>
using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tZoneListPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_ThermalZoneList_t));
LE_MEM_DEFINE_STATIC_POOL(tZonePool, TAF_THERM_MAX_ZONE_POOL_SIZE, sizeof(taf_ThermalZone_t));
LE_MEM_DEFINE_STATIC_POOL(tripPointPool, TAF_THERM_MAX_TRIP_POINT_POOL_SIZE,
        sizeof(taf_TripPoint_t));
LE_MEM_DEFINE_STATIC_POOL(boundCDPool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_BoundCoolingDevice_t));
LE_MEM_DEFINE_STATIC_POOL(boundTripPointCDPool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_TripPoint_t));
LE_MEM_DEFINE_STATIC_POOL(cDevListPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_CoolingDeviceList_t));
LE_MEM_DEFINE_STATIC_POOL(cDevPool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_CoolingDevice_t));
LE_MEM_DEFINE_STATIC_POOL(tripEventHandlerPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_EventHandlerInfo_t));
LE_MEM_DEFINE_STATIC_POOL(coolingLevelHandlerPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_EventHandlerInfo_t));

LE_REF_DEFINE_STATIC_MAP(tZoneListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tZoneRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(boundCDRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(boundTripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(cDevListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(cDevRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tripEventHandlerRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(coolingLevelHandlerRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);


/*======================================================================

 FUNCTION        taf_Therm::GetInstance

 DESCRIPTION     Get the instance of therm.

 DEPENDENCIES    The initialization of therm.

 PARAMETERS      None

 RETURN VALUE    taf_Therm&

 SIDE EFFECTS

======================================================================*/
taf_Therm& taf_Therm::GetInstance()
{
    static taf_Therm instance;
    return instance;
}

telux::common::Status mapZoneNametoId()
{
    auto& tafTherm = taf_Therm::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;

    TAF_ERROR_IF_RET_VAL(tafTherm.thermalManager == NULL, status, "thermalManager is NULL");

    tafTherm.zoneNameToIdMap.clear();
    std::vector<std::shared_ptr<telux::therm::IThermalZone>> zonesInfo
            = tafTherm.thermalManager->getThermalZones();
    if (zonesInfo.size() > 0)
    {
        status = telux::common::Status::SUCCESS;
        for (size_t i = 0; i < zonesInfo.size(); i++)
        {
            std::string zone = zonesInfo[i]->getDescription();
            int id = zonesInfo[i]->getId();
            tafTherm.zoneNameToIdMap[zone] = id;
        }
    }
    else
    {
        return status;
    }
    return status;
}

telux::common::Status mapCDeviceNametoId()
{
    auto& tafTherm = taf_Therm::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;

    TAF_ERROR_IF_RET_VAL(tafTherm.thermalManager == NULL, status, "thermalManager is NULL");
    tafTherm.CDevNameToIdMap.clear();
    std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> cDevInfo
            = tafTherm.thermalManager->getCoolingDevices();
    if (cDevInfo.size() > 0)
    {
        status = telux::common::Status::SUCCESS;
        for (size_t i = 0; i < cDevInfo.size(); i++)
        {
            std::string cDev = cDevInfo[i]->getDescription();
            int id = cDevInfo[i]->getId();
            tafTherm.CDevNameToIdMap[cDev] = id;
        }
    }
    else
    {
        return status;
    }
    return status;
}

telux::common::Status manageIndication(bool registerInd)
{
    auto& tafTherm = taf_Therm::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;

    TAF_ERROR_IF_RET_VAL(tafTherm.thermalManager == NULL, status, "thermalManager is NULL");
    if (registerInd)
    {
        if (tafTherm.thermalListener == NULL)
        {
            tafTherm.thermalListener = std::make_shared<taf_ThermServiceListener>();
        }
        status = tafTherm.thermalManager->registerListener(tafTherm.thermalListener);
        TAF_ERROR_IF_RET_VAL(status == telux::common::Status::FAILED,
                status,"Listener registered - Failed");
    }
    else
    {
        TAF_ERROR_IF_RET_VAL(tafTherm.thermalListener == NULL, status,
                "thermalListener is NULL");
        status = tafTherm.thermalManager->deregisterListener(tafTherm.thermalListener);
        TAF_ERROR_IF_RET_VAL(status == telux::common::Status::FAILED,
                status, "Listener deregistered - Failed");
        tafTherm.thermalListener.reset();
    }
    return status;
}

void RemoveEventHandlersBySession
(
    le_msg_SessionRef_t sessionRef,
    le_ref_MapRef_t handlerRefMap,
    le_dls_List_t* handlerListPtr
)
{
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");
    TAF_ERROR_IF_RET_NIL(handlerRefMap == NULL, "handlerRefMap is NULL");
    TAF_ERROR_IF_RET_NIL(handlerListPtr == NULL, "handlerListPtr is NULL");

    le_dls_Link_t* linkPtr = le_dls_Peek(handlerListPtr);
    while (linkPtr != NULL)
    {
        taf_EventHandlerInfo_t* handlerInfo =
            CONTAINER_OF(linkPtr, taf_EventHandlerInfo_t, link);
        linkPtr = le_dls_PeekNext(handlerListPtr, linkPtr);
        if (handlerInfo->sessionRef == sessionRef)
        {
            le_event_RemoveHandler(handlerInfo->handlerRef);
            le_dls_Remove(handlerListPtr, &handlerInfo->link);
            le_ref_DeleteRef(handlerRefMap, handlerInfo->safeRef);
            le_mem_Release(handlerInfo);
        }
    }
}

void RemoveEventHandlerByRef
(
    taf_EventHandlerInfo_t* handlerInfo,
    le_ref_MapRef_t handlerRefMap,
    le_dls_List_t* handlerListPtr
)
{
    TAF_ERROR_IF_RET_NIL(handlerInfo == NULL, "handlerInfo is NULL");
    TAF_ERROR_IF_RET_NIL(handlerRefMap == NULL, "handlerRefMap is NULL");
    TAF_ERROR_IF_RET_NIL(handlerListPtr == NULL, "handlerListPtr is NULL");

    le_event_RemoveHandler(handlerInfo->handlerRef);
    le_dls_Remove(handlerListPtr, &handlerInfo->link);
    le_ref_DeleteRef(handlerRefMap, handlerInfo->safeRef);
    le_mem_Release(handlerInfo);
}

/*======================================================================

 FUNCTION        taf_Therm::Init

 DESCRIPTION     Initialization of the thermal Service and registering listeners

 DEPENDENCIES    The initialization of telaf

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Therm::Init(void)
{
    telux::common::ProcType procType = telux::common::ProcType::LOCAL_PROC;
    auto& thermalFactory = telux::therm::ThermalFactory::getInstance();

    auto promisePtr = std::make_shared<std::promise<telux::common::ServiceStatus>>();

    thermalManager = thermalFactory.getThermalManager(
        [promisePtr](telux::common::ServiceStatus status) {
            try {
                promisePtr->set_value(status);
            } catch (const std::future_error &e) {
                LE_FATAL("Promise already satisfied: %s", e.what());
            }
        },
        procType
    );

    if (thermalManager == nullptr)
    {
        LE_FATAL("ERROR - Failed to get thermal manager instance.");
    }

    auto future = promisePtr->get_future();

    if (future.wait_for(std::chrono::seconds(TAF_THERM_MANAGER_TIMEOUT)) == std::future_status::ready)
    {
        telux::common::ServiceStatus mgrStatus = future.get();
        if (mgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Thermal Subsystem is ready.");
        }
        else
        {
            LE_FATAL("ERROR - Unable to initialize Thermal subsystem.");
        }
    }
    else
    {
        LE_FATAL("Timeout waiting for serviceStatus callback");
    }

    LE_INFO("Thermal manager instance returned for proc type: %d", static_cast<int>(procType));

    tZoneListPool = le_mem_InitStaticPool(tZoneListPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_ThermalZoneList_t));
    tZonePool = le_mem_InitStaticPool(tZonePool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_ThermalZone_t));
    tripPointPool = le_mem_InitStaticPool(tripPointPool, TAF_THERM_MAX_TRIP_POINT_POOL_SIZE,
        sizeof(taf_TripPoint_t));
    boundCDPool = le_mem_InitStaticPool(boundCDPool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_BoundCoolingDevice_t));
    boundTripPointCDPool = le_mem_InitStaticPool(boundTripPointCDPool,TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_TripPoint_t));
    cDevListPool = le_mem_InitStaticPool(cDevListPool, TAF_THERM_MAX_LIST_POOL_SIZE,
        sizeof(taf_CoolingDeviceList_t));
    cDevPool = le_mem_InitStaticPool(cDevPool, TAF_THERM_MAX_ZONE_POOL_SIZE,
        sizeof(taf_CoolingDevice_t));
    tripEventHandlerPool = le_mem_InitStaticPool(tripEventHandlerPool,
        TAF_THERM_MAX_LIST_POOL_SIZE, sizeof(taf_EventHandlerInfo_t));
    coolingLevelHandlerPool = le_mem_InitStaticPool(coolingLevelHandlerPool,
        TAF_THERM_MAX_LIST_POOL_SIZE, sizeof(taf_EventHandlerInfo_t));

    tZoneListRefMap = le_ref_InitStaticMap(tZoneListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    tZoneRefMap = le_ref_InitStaticMap(tZoneRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    tripPointRefMap = le_ref_InitStaticMap(tripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    boundCDRefMap = le_ref_InitStaticMap(boundCDRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    boundTripPointRefMap = le_ref_InitStaticMap(boundTripPointRefMap,TAF_THERM_MAX_LIST_POOL_SIZE);
    cDevListRefMap = le_ref_InitStaticMap(cDevListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    cDevRefMap = le_ref_InitStaticMap(cDevRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    tripEventHandlerRefMap = le_ref_InitStaticMap(tripEventHandlerRefMap,
        TAF_THERM_MAX_LIST_POOL_SIZE);
    coolingLevelHandlerRefMap = le_ref_InitStaticMap(coolingLevelHandlerRefMap,
        TAF_THERM_MAX_LIST_POOL_SIZE);

    le_msg_AddServiceCloseHandler(taf_therm_GetServiceRef(),
        taf_Handler::OnClientDisconnection, NULL);

    stateChangeEvent = le_event_CreateId("stateChangeEvent", sizeof(taf_TripEventInfo_t));
    onCoolingLevelChangeEvent =
            le_event_CreateId("onCoolingLevelChangeEvent", sizeof(coolingLevelChangeInfo_t));

    if (manageIndication(true) != telux::common::Status::SUCCESS)
    {
        LE_INFO("Failed to register listner");
    }

    if (mapZoneNametoId() != telux::common::Status::SUCCESS)
    {
        LE_INFO("Failed to map thermal zone names to thermal zone ID ");
    }

    if (mapCDeviceNametoId() != telux::common::Status::SUCCESS)
    {
        LE_INFO("Failed to map cooling device names to cooling device ID");
    }
    LE_INFO("Returning from thermal init()");
}

void taf_Handler::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void* ctxPtr)
{
     auto& tafThermalMgr = taf_Therm::GetInstance();
     std::vector<taf_therm_ThermalZoneListRef_t> tZoneRefsToDelete;
     std::vector<taf_therm_CoolingDeviceListRef_t> cDevRefsToDelete;

     le_ref_IterRef_t tZoneIterRef = le_ref_GetIterator(tafThermalMgr.tZoneListRefMap);
     le_ref_IterRef_t cDevIterRef = le_ref_GetIterator(tafThermalMgr.cDevListRefMap);
     while (le_ref_NextNode(tZoneIterRef) == LE_OK)
     {
         taf_ThermalZoneList_t* thermalListPtr =
                 (taf_ThermalZoneList_t*)le_ref_GetValue(tZoneIterRef);
         if (thermalListPtr && thermalListPtr->sessionRef == sessionRef)
         {
             tZoneRefsToDelete.push_back(thermalListPtr->ref);
         }
     }
     while (le_ref_NextNode(cDevIterRef) == LE_OK)
     {
         taf_CoolingDeviceList_t* cDevListPtr =
             (taf_CoolingDeviceList_t*)le_ref_GetValue(cDevIterRef);
         if (cDevListPtr && cDevListPtr->sessionRef == sessionRef)
         {
             cDevRefsToDelete.push_back(cDevListPtr->ref);
         }
     }

     for (auto ref : tZoneRefsToDelete)
     {
         taf_therm_DeleteThermalZoneList(ref);
     }

     for (auto ref : cDevRefsToDelete)
     {
         taf_therm_DeleteCoolingDeviceList(ref);
     }

     RemoveEventHandlersBySession(sessionRef, tafThermalMgr.tripEventHandlerRefMap,
         &tafThermalMgr.tripEventHandlerList);
     RemoveEventHandlersBySession(sessionRef, tafThermalMgr.coolingLevelHandlerRefMap,
         &tafThermalMgr.coolingLevelHandlerList);
}

void taf_ThermServiceListener::onServiceStatusChange(telux::common::ServiceStatus serviceStatus)
{
    switch (serviceStatus)
    {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            LE_INFO("Thermal service status: Available.");
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            LE_WARN("Thermal service status: Unavailable.");
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            LE_WARN("Thermal service status: Failed.");
            break;
        default:
            LE_WARN("Thermal service status: Unknown.");
            break;
    }
}


void GetTripType(taf_TripPoint_t* tripPointPtr,
    std::shared_ptr <telux::therm::ITripPoint> tripPoint)
{
    TAF_ERROR_IF_RET_NIL(tripPointPtr == NULL, "tripPointPtr is NULL");
    TAF_ERROR_IF_RET_NIL(tripPoint    == NULL, "tripPoint is NULL");

    telux::therm::TripType tripType = tripPoint->getType();
    switch (tripType)
    {
        case telux::therm::TripType::UNKNOWN:
            tripPointPtr->tripType = TAF_THERM_UNKNOWN;
        break;

        case telux::therm::TripType::CRITICAL:
            tripPointPtr->tripType = TAF_THERM_CRITICAL;
            break;

        case telux::therm::TripType::HOT:
            tripPointPtr->tripType = TAF_THERM_HOT;
            break;

        case telux::therm::TripType::PASSIVE:
            tripPointPtr->tripType = TAF_THERM_PASSIVE;
            break;

        case telux::therm::TripType::ACTIVE:
            tripPointPtr->tripType = TAF_THERM_ACTIVE;
            break;

        case telux::therm::TripType::CONFIGURABLE_HIGH:
            tripPointPtr->tripType = TAF_THERM_CONFIGURABLE_HIGH;
            break;

        case telux::therm::TripType::CONFIGURABLE_LOW:
        tripPointPtr->tripType = TAF_THERM_CONFIGURABLE_LOW;
        break;
    }
}

taf_therm_ThermalZoneListRef_t taf_Therm::GetThermalZonesList()
{
    auto& tafTherm = taf_Therm::GetInstance();

    TAF_ERROR_IF_RET_VAL(thermalManager == NULL, NULL, "thermalManager is NULL");

    taf_ThermalZoneList_t* tZoneListPtr =
            (taf_ThermalZoneList_t*)le_mem_ForceAlloc(tafTherm.tZoneListPool);
    memset(tZoneListPtr, 0, sizeof(taf_ThermalZoneList_t));
    tZoneListPtr->ThermalZoneList = LE_SLS_LIST_INIT;
    tZoneListPtr->sessionRef = taf_therm_GetClientSessionRef();
    tZoneListPtr->currPtr = NULL;
    std::vector<std::shared_ptr<telux::therm::IThermalZone>> zonesInfo =
            thermalManager->getThermalZones();
    tZoneListPtr->thermalZoneListSize = zonesInfo.size();
    if (zonesInfo.size() > 0)
    {
        taf_ThermalZone_t* tZonePtr;
        taf_TripPoint_t* tripPointPtr;
        taf_BoundCoolingDevice_t* boundCoolingDevicePtr;
        taf_TripPoint_t* bindingPtr;
        for (size_t i = 0; i < zonesInfo.size(); i++)
        {
            tZonePtr = (taf_ThermalZone_t*)le_mem_ForceAlloc(tafTherm.tZonePool);
            memset(tZonePtr, 0, sizeof(taf_ThermalZone_t));
            tZonePtr->TripPointList = LE_SLS_LIST_INIT;
            tZonePtr->BoundCoolingDeviceList = LE_SLS_LIST_INIT;
            tZonePtr->currPtr = NULL;
            le_utf8_Copy(tZonePtr->Type, zonesInfo[i]->getDescription().c_str(),
                    TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
            tZonePtr->tZoneId = zonesInfo[i]->getId();
            tZonePtr->currTemp = zonesInfo[i]->getCurrentTemp();
            tZonePtr->passiveTemp = zonesInfo[i]->getPassiveTemp();
            //Trip Points Information
            std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripPoint =
                zonesInfo[i]->getTripPoints();
            tZonePtr->tripPointListSize = tripPoint.size();
            for (size_t t = 0; t < tripPoint.size(); t++)
            {
                tripPointPtr = (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);
                memset(tripPointPtr, 0, sizeof(taf_TripPoint_t));
                GetTripType(tripPointPtr, tripPoint[t]);
                tripPointPtr->threshold = tripPoint[t]->getThresholdTemp();
                tripPointPtr->hysteresis = tripPoint[t]->getHysteresis();

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
                tripPointPtr->tripId = tripPoint[t]->getTripId();
#endif
#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
                tripPointPtr->tZoneId = tripPoint[t]->getTZoneId();
#endif

                tripPointPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(tZonePtr->TripPointList), &(tripPointPtr->link));
                tripPointPtr->ref =
                        (taf_therm_TripPointRef_t)le_ref_CreateRef(tripPointRefMap, tripPointPtr);
            }
            //Bound Cooling Device Information
            std::vector<telux::therm::BoundCoolingDevice> boundCoolingDevice =
                    zonesInfo[i]->getBoundCoolingDevices();
            tZonePtr->boundCoolingDeviceListSize = boundCoolingDevice.size();
            for (size_t t = 0; t < boundCoolingDevice.size(); t++)
            {
                boundCoolingDevicePtr =
                    (taf_BoundCoolingDevice_t*)le_mem_ForceAlloc(tafTherm.boundCDPool);
                memset(boundCoolingDevicePtr, 0, sizeof(taf_BoundCoolingDevice_t));
                boundCoolingDevicePtr->TripPointList = LE_SLS_LIST_INIT;
                boundCoolingDevicePtr->currPtr = NULL;
                boundCoolingDevicePtr->coolingDeviceId = boundCoolingDevice[t].coolingDeviceId;
                std::vector<std::shared_ptr<telux::therm::ITripPoint>> cDevBinding =
                        boundCoolingDevice[t].bindingInfo;
                boundCoolingDevicePtr->tripPointListSize = cDevBinding.size();
                for (size_t idx = 0; idx < cDevBinding.size(); idx++)
                {
                    bindingPtr =(taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.boundTripPointCDPool);
                    memset(bindingPtr, 0, sizeof(taf_TripPoint_t));
                    GetTripType(bindingPtr, cDevBinding[idx]);
                    bindingPtr->threshold = cDevBinding[idx]->getThresholdTemp();
                    bindingPtr->hysteresis = cDevBinding[idx]->getHysteresis();

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
                    bindingPtr->tripId = cDevBinding[idx]->getTripId();
#endif
#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
                    bindingPtr->tZoneId = cDevBinding[idx]->getTZoneId();
#endif
                    bindingPtr->link = LE_SLS_LINK_INIT;
                    le_sls_Queue(&(boundCoolingDevicePtr->TripPointList), &(bindingPtr->link));
                    bindingPtr->ref =
                         (taf_therm_TripPointRef_t)le_ref_CreateRef(boundTripPointRefMap, bindingPtr);
                }
                boundCoolingDevicePtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(tZonePtr->BoundCoolingDeviceList), &(boundCoolingDevicePtr->link));
                boundCoolingDevicePtr->ref =
                     (taf_therm_BoundCoolingDeviceRef_t)
                     le_ref_CreateRef(boundCDRefMap, boundCoolingDevicePtr);
            }
            tZonePtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(tZoneListPtr->ThermalZoneList), &(tZonePtr->link));
            tZonePtr->ref = (taf_therm_ThermalZoneRef_t)le_ref_CreateRef(tZoneRefMap, tZonePtr);
        }
        tZoneListPtr->ref =
                (taf_therm_ThermalZoneListRef_t)le_ref_CreateRef(tZoneListRefMap, tZoneListPtr);
        return tZoneListPtr->ref;
    }

    le_mem_Release(tZoneListPtr);
    return NULL;
}


taf_therm_ThermalZoneRef_t taf_Therm::GetFirstThermalZone(
    taf_therm_ThermalZoneListRef_t tZoneListRef)
{
    taf_ThermalZoneList_t* tZoneListPtr =
            (taf_ThermalZoneList_t*)le_ref_Lookup(tZoneListRefMap, tZoneListRef);

    TAF_ERROR_IF_RET_VAL(tZoneListPtr == NULL, NULL, "Failed to retrieve thermal zone.");

    le_sls_Link_t* tZonelinkPtr = le_sls_Peek(&(tZoneListPtr->ThermalZoneList));
    if (tZonelinkPtr != NULL)
    {
        taf_ThermalZone_t* tZonePtr = CONTAINER_OF(tZonelinkPtr, taf_ThermalZone_t, link);
        tZoneListPtr->currPtr = tZonelinkPtr;
        return tZonePtr->ref;
    }
    return NULL;
}

taf_therm_ThermalZoneRef_t taf_Therm::GetNextThermalZone(
    taf_therm_ThermalZoneListRef_t tZoneListRef
)
{
    taf_ThermalZoneList_t* tZoneListPtr =
            (taf_ThermalZoneList_t*)le_ref_Lookup(tZoneListRefMap, tZoneListRef);

    TAF_ERROR_IF_RET_VAL(tZoneListPtr == NULL, NULL, "Failed to retrieve next thermal zone.");

    le_sls_Link_t* tZonelinkPtr =
        le_sls_PeekNext(&(tZoneListPtr->ThermalZoneList), tZoneListPtr->currPtr);
    if (tZonelinkPtr != NULL)
    {
        taf_ThermalZone_t* tZonePtr = CONTAINER_OF(tZonelinkPtr, taf_ThermalZone_t, link);
        tZoneListPtr->currPtr = tZonelinkPtr;
        return tZonePtr->ref;
    }
    return NULL;
}

taf_therm_TripPointRef_t taf_Therm::GetFirstTripPoint(taf_therm_ThermalZoneRef_t listRef)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, NULL,"Failed to retrieve thermal zone.");

    le_sls_Link_t* tripPointPtr = le_sls_Peek(&(tZonePtr->TripPointList));
    if (tripPointPtr != NULL)
    {
        taf_TripPoint_t* tripPoint = CONTAINER_OF(tripPointPtr, taf_TripPoint_t, link);
        tZonePtr->currPtr = tripPointPtr;
        return tripPoint->ref;
    }
    return NULL;
}

taf_therm_TripPointRef_t taf_Therm::GetNextTripPoint(taf_therm_ThermalZoneRef_t listRef)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, NULL,"Failed to retrieve next thermal zone.");

    le_sls_Link_t* tripPointPtr = le_sls_PeekNext(&(tZonePtr->TripPointList), tZonePtr->currPtr);
    if (tripPointPtr != NULL)
    {
        taf_TripPoint_t* tripPoint = CONTAINER_OF(tripPointPtr, taf_TripPoint_t, link);
        tZonePtr->currPtr = tripPointPtr;
        return tripPoint->ref;
    }
    return NULL;
}

taf_therm_BoundCoolingDeviceRef_t taf_Therm::GetFirstBoundCDev(taf_therm_ThermalZoneRef_t listRef)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, NULL,"Failed to retrieve next thermal zone.");

    le_sls_Link_t* cDevPtr = le_sls_Peek(&(tZonePtr->BoundCoolingDeviceList));
    if (cDevPtr != NULL)
    {
        taf_BoundCoolingDevice_t* cDev = CONTAINER_OF(cDevPtr, taf_BoundCoolingDevice_t, link);
        tZonePtr->currPtr = cDevPtr;
        return cDev->ref;
    }
    return NULL;
}

taf_therm_BoundCoolingDeviceRef_t taf_Therm::GetNextBoundCDev(taf_therm_ThermalZoneRef_t listRef)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, NULL,"Failed to retrieve next thermal zone.");

    le_sls_Link_t* cDevPtr
        = le_sls_PeekNext(&(tZonePtr->BoundCoolingDeviceList), tZonePtr->currPtr);
    if (cDevPtr != NULL)
    {
        taf_BoundCoolingDevice_t* cDev = CONTAINER_OF(cDevPtr, taf_BoundCoolingDevice_t, link);
        tZonePtr->currPtr = cDevPtr;
        return cDev->ref;
    }
    return NULL;
}

taf_therm_TripPointRef_t taf_Therm::GetFirstBoundTripPoint(
    taf_therm_BoundCoolingDeviceRef_t listRef
)
{
    taf_BoundCoolingDevice_t* boundTripPointPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundTripPointPtr == NULL, NULL,
            "Failed to retrieve first bound trip point.");

    le_sls_Link_t* tripPointPtr = le_sls_Peek(&(boundTripPointPtr->TripPointList));
    if (tripPointPtr != NULL)
    {
        taf_TripPoint_t* tripPoint = CONTAINER_OF(tripPointPtr, taf_TripPoint_t, link);
        boundTripPointPtr->currPtr = tripPointPtr;
        return tripPoint->ref;
    }
    return NULL;
}
taf_therm_TripPointRef_t taf_Therm::GetNextBoundTripPoint(
    taf_therm_BoundCoolingDeviceRef_t listRef
)
{
    taf_BoundCoolingDevice_t* boundTripPointPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundTripPointPtr ==NULL,NULL,"Failed to retrieve next bound trip point");

    le_sls_Link_t* tripPointPtr =
            le_sls_PeekNext(&(boundTripPointPtr->TripPointList), boundTripPointPtr->currPtr);

    if (tripPointPtr != NULL)
    {
        taf_TripPoint_t* tripPoint = CONTAINER_OF(tripPointPtr, taf_TripPoint_t, link);
        boundTripPointPtr->currPtr = tripPointPtr;
        return tripPoint->ref;
    }
    return NULL;
}

le_result_t taf_Therm::DeleteThermalZoneList(taf_therm_ThermalZoneListRef_t ListRef)
{
    TAF_ERROR_IF_RET_VAL(ListRef == nullptr, LE_BAD_PARAMETER, "Null reference(ListRef)");
    taf_ThermalZoneList_t* listPtr =
        (taf_ThermalZoneList_t*)le_ref_Lookup(tZoneListRefMap, ListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteMachineList : %p", ListRef);
    taf_ThermalZone_t* tZonePtr;
    taf_TripPoint_t* tripPointPtr;
    taf_BoundCoolingDevice_t* boundCDPtr;
    taf_TripPoint_t* boundTripPointPtr;
    le_sls_Link_t* tZoneLinkPtr;
    le_sls_Link_t* tripPointLinkPtr;
    le_sls_Link_t* boundCDLinkPtr;
    le_sls_Link_t* boundTripPointLinkPtr;
    while ((tZoneLinkPtr = le_sls_Pop(&(listPtr->ThermalZoneList))) != NULL)
    {
        tZonePtr = CONTAINER_OF(tZoneLinkPtr, taf_ThermalZone_t, link);
        while ((tripPointLinkPtr = le_sls_Pop(&(tZonePtr->TripPointList))) != NULL)
        {
            tripPointPtr = CONTAINER_OF(tripPointLinkPtr, taf_TripPoint_t, link);
            le_ref_DeleteRef(tripPointRefMap, tripPointPtr->ref);
            le_mem_Release(tripPointPtr);
        }
        while ((boundCDLinkPtr = le_sls_Pop(&(tZonePtr->BoundCoolingDeviceList))) != NULL)
        {
            boundCDPtr = CONTAINER_OF(boundCDLinkPtr, taf_BoundCoolingDevice_t, link);
            while ((boundTripPointLinkPtr = le_sls_Pop(&(boundCDPtr->TripPointList))) != NULL)
            {
                boundTripPointPtr = CONTAINER_OF(boundTripPointLinkPtr, taf_TripPoint_t, link);
                le_ref_DeleteRef(boundTripPointRefMap, boundTripPointPtr->ref);
                le_mem_Release(boundTripPointPtr);
            }
            le_ref_DeleteRef(boundCDRefMap, boundCDPtr->ref);
            le_mem_Release(boundCDPtr);
        }
        le_ref_DeleteRef(tZoneRefMap, tZonePtr->ref);
        le_mem_Release(tZonePtr);
    }
    le_ref_DeleteRef(tZoneListRefMap, ListRef);
    le_mem_Release(listPtr);
    return LE_OK;
}

taf_therm_CoolingDeviceListRef_t taf_Therm::GetCoolingDeviceList()
{
    auto& tafTherm = taf_Therm::GetInstance();
    TAF_ERROR_IF_RET_VAL(thermalManager == NULL, NULL, "thermalManager is NULL");

    taf_CoolingDeviceList_t* cDevListPtr =
            (taf_CoolingDeviceList_t*)le_mem_ForceAlloc(tafTherm.cDevListPool);
    memset(cDevListPtr, 0, sizeof(taf_CoolingDeviceList_t));
    cDevListPtr->CoolingDeviceList = LE_SLS_LIST_INIT;
    cDevListPtr->sessionRef = taf_therm_GetClientSessionRef();
    cDevListPtr->currPtr = NULL;

    std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevices =
            thermalManager->getCoolingDevices();
    cDevListPtr->coolingDeviceListSize = coolingDevices.size();
    if (coolingDevices.size() > 0)
    {
        taf_CoolingDevice_t* cDevPtr;
        for (size_t i = 0; i < coolingDevices.size(); i++)
        {
            cDevPtr = (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);
            memset(cDevPtr, 0, sizeof(taf_CoolingDevice_t));
            le_utf8_Copy(cDevPtr->description, coolingDevices[i]->getDescription().c_str(),
                TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
            cDevPtr->cDevId = coolingDevices[i]->getId();
            cDevPtr->maxCoolingLevel = coolingDevices[i]->getMaxCoolingLevel();
            cDevPtr->currentCoolingLevel = coolingDevices[i]->getCurrentCoolingLevel();
            cDevPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(cDevListPtr->CoolingDeviceList), &(cDevPtr->link));
            cDevPtr->ref =
                (taf_therm_CoolingDeviceRef_t)le_ref_CreateRef(cDevRefMap, (void*)cDevPtr);
        }
        cDevListPtr->ref =
                (taf_therm_CoolingDeviceListRef_t)le_ref_CreateRef(cDevListRefMap, cDevListPtr);
        return cDevListPtr->ref;
    }

    le_mem_Release(cDevListPtr);
    return NULL;
}

taf_therm_CoolingDeviceRef_t taf_Therm::GetFirstCoolingDevice(
    taf_therm_CoolingDeviceListRef_t cDevListRef)
{
    taf_CoolingDeviceList_t* cDevListPtr =
            (taf_CoolingDeviceList_t*)le_ref_Lookup(cDevListRefMap, cDevListRef);

    TAF_ERROR_IF_RET_VAL(cDevListPtr == NULL, NULL, "Failed to retrieve first cooling device.");

    le_sls_Link_t* cDevlinkPtr = le_sls_Peek(&(cDevListPtr->CoolingDeviceList));
    if (cDevlinkPtr != NULL)
    {
        taf_CoolingDevice_t* cDevPtr = CONTAINER_OF(cDevlinkPtr, taf_CoolingDevice_t, link);
        cDevListPtr->currPtr = cDevlinkPtr;
        return cDevPtr->ref;
    }
    return NULL;
}

taf_therm_CoolingDeviceRef_t taf_Therm::GetNextCoolingDevice(
    taf_therm_CoolingDeviceListRef_t cDevListRef
)
{
    taf_CoolingDeviceList_t* cDevListPtr =
        (taf_CoolingDeviceList_t*)le_ref_Lookup(cDevListRefMap, cDevListRef);

    TAF_ERROR_IF_RET_VAL(cDevListPtr == NULL, NULL, "Failed to retrieve next cooling device.");

    le_sls_Link_t* cDevlinkPtr =
        le_sls_PeekNext(&(cDevListPtr->CoolingDeviceList), cDevListPtr->currPtr);
    if (cDevlinkPtr != NULL)
    {
        taf_CoolingDevice_t* cDevPtr = CONTAINER_OF(cDevlinkPtr, taf_CoolingDevice_t, link);
        cDevListPtr->currPtr = cDevlinkPtr;
        return cDevPtr->ref;
    }
    return NULL;
}

le_result_t taf_Therm::DeleteCoolingDeviceList(taf_therm_CoolingDeviceListRef_t ListRef)
{
    TAF_ERROR_IF_RET_VAL(ListRef == nullptr, LE_BAD_PARAMETER, "Null reference(ListRef)");
    taf_CoolingDeviceList_t* listPtr =
        (taf_CoolingDeviceList_t*)le_ref_Lookup(cDevListRefMap, ListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteMachineList : %p", ListRef);
    taf_CoolingDevice_t* cDevPtr;
    le_sls_Link_t* linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->CoolingDeviceList))) != NULL)
    {
        cDevPtr = CONTAINER_OF(linkPtr, taf_CoolingDevice_t, link);
        le_ref_DeleteRef(cDevRefMap, cDevPtr->ref);
        le_mem_Release(cDevPtr);
    }
    le_ref_DeleteRef(cDevListRefMap, ListRef);
    le_mem_Release(listPtr);
    return LE_OK;
}

taf_therm_ThermalZoneRef_t taf_Therm::GetThermalZoneByName(const char *thermalZoneName)
{
    auto& tafTherm = taf_Therm::GetInstance();

    TAF_ERROR_IF_RET_VAL(thermalManager  == NULL, NULL, "thermalManager is NULL");
    TAF_ERROR_IF_RET_VAL(thermalZoneName == NULL, NULL, "thermalZoneName is NULL");

    int thermalZoneId = MapThermalZonetoId(thermalZoneName);

    TAF_ERROR_IF_RET_VAL(thermalZoneId == -1, NULL, "NO THERMAL ZONE ASSOCIATED WITH %s",
            thermalZoneName);

    std::shared_ptr<telux::therm::IThermalZone> thermalZone =
            thermalManager->getThermalZone(thermalZoneId);
    TAF_ERROR_IF_RET_VAL(thermalZone == NULL, NULL, "Failed to get thermal zone");

    taf_TripPoint_t* tripPointPtr;
    taf_BoundCoolingDevice_t* boundCoolingDevicePtr;
    taf_TripPoint_t* bindingPtr;
    taf_ThermalZone_t*  tZonePtr = (taf_ThermalZone_t*)le_mem_ForceAlloc(tafTherm.tZonePool);
    memset(tZonePtr, 0, sizeof(taf_ThermalZone_t));
    tZonePtr->TripPointList = LE_SLS_LIST_INIT;
    tZonePtr->BoundCoolingDeviceList = LE_SLS_LIST_INIT;
    tZonePtr->currPtr = NULL;
    le_utf8_Copy(tZonePtr->Type, thermalZone->getDescription().c_str(),
        TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
    tZonePtr->tZoneId = thermalZone->getId();
    tZonePtr->currTemp = thermalZone->getCurrentTemp();
    tZonePtr->passiveTemp = thermalZone->getPassiveTemp();

    //Trip Points Information
    std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripPoint =
        thermalZone->getTripPoints();

    tZonePtr->tripPointListSize = tripPoint.size();
    for (size_t t = 0; t < tripPoint.size(); t++)
    {
        tripPointPtr = (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);
        memset(tripPointPtr, 0, sizeof(taf_TripPoint_t));
        GetTripType(tripPointPtr, tripPoint[t]);
        tripPointPtr->threshold = tripPoint[t]->getThresholdTemp();
        tripPointPtr->hysteresis = tripPoint[t]->getHysteresis();

        #if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
            tripPointPtr->tripId = tripPoint[t]->getTripId();
        #endif
        #if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
            tripPointPtr->tZoneId = tripPoint[t]->getTZoneId();
        #endif

        tripPointPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(tZonePtr->TripPointList), &(tripPointPtr->link));
        tripPointPtr->ref =
                (taf_therm_TripPointRef_t)le_ref_CreateRef(tripPointRefMap, tripPointPtr);
    }

    //Bound Cooling Device Information
    std::vector<telux::therm::BoundCoolingDevice> boundCoolingDevice =
        thermalZone->getBoundCoolingDevices();

    tZonePtr->boundCoolingDeviceListSize = boundCoolingDevice.size();
    for (size_t t = 0; t < boundCoolingDevice.size(); t++)
    {
        boundCoolingDevicePtr = (taf_BoundCoolingDevice_t*)le_mem_ForceAlloc(tafTherm.boundCDPool);
        memset(boundCoolingDevicePtr, 0, sizeof(taf_BoundCoolingDevice_t));
        boundCoolingDevicePtr->TripPointList = LE_SLS_LIST_INIT;
        boundCoolingDevicePtr->currPtr = NULL;
        boundCoolingDevicePtr->coolingDeviceId = boundCoolingDevice[t].coolingDeviceId;
        std::vector<std::shared_ptr<telux::therm::ITripPoint>> cDevBinding =
            boundCoolingDevice[t].bindingInfo;
        boundCoolingDevicePtr->tripPointListSize = cDevBinding.size();
        for (size_t idx = 0; idx < cDevBinding.size(); idx++)
        {
            bindingPtr = (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.boundTripPointCDPool);
            memset(bindingPtr, 0, sizeof(taf_TripPoint_t));
            GetTripType(bindingPtr, cDevBinding[idx]);
            bindingPtr->threshold = cDevBinding[idx]->getThresholdTemp();
            bindingPtr->hysteresis = cDevBinding[idx]->getHysteresis();

            #if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
                bindingPtr->tripId = cDevBinding[idx]->getTripId();
            #endif
            #if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
                bindingPtr->tZoneId = cDevBinding[idx]->getTZoneId();
            #endif

            bindingPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(boundCoolingDevicePtr->TripPointList), &(bindingPtr->link));
            bindingPtr->ref =
                (taf_therm_TripPointRef_t)le_ref_CreateRef(boundTripPointRefMap, bindingPtr);
        }

        boundCoolingDevicePtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(tZonePtr->BoundCoolingDeviceList), &(boundCoolingDevicePtr->link));
        boundCoolingDevicePtr->ref =
                 (taf_therm_BoundCoolingDeviceRef_t)
                 le_ref_CreateRef(boundCDRefMap, boundCoolingDevicePtr);
    }
    tZonePtr->ref = (taf_therm_ThermalZoneRef_t)le_ref_CreateRef(tZoneRefMap, tZonePtr);
    return tZonePtr->ref;
}

int taf_Therm::MapThermalZonetoId(const char *thermalZone)
{
    auto& tafTherm = taf_Therm::GetInstance();

    TAF_ERROR_IF_RET_VAL(thermalZone == NULL, -1, "thermalZone is NULL");
    TAF_ERROR_IF_RET_VAL(tafTherm.zoneNameToIdMap.find(thermalZone) == tafTherm.zoneNameToIdMap.end(),
            -1, "THERMAL ZONE NOT FOUND");

    return tafTherm.zoneNameToIdMap[thermalZone];
}


le_result_t taf_Therm::GetThermalZoneID(taf_therm_ThermalZoneRef_t listRef,uint32_t* thermalZoneID)
{
    TAF_ERROR_IF_RET_VAL(thermalZoneID == NULL, LE_BAD_PARAMETER, "thermalZoneID is NULL");

    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT, "Failed to retrieve thermal zone.");

    *thermalZoneID = tZonePtr->tZoneId;
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZoneCurrentTemp
(
    taf_therm_ThermalZoneRef_t listRef, uint32_t* currTemp
)
{
    TAF_ERROR_IF_RET_VAL(currTemp == NULL, LE_BAD_PARAMETER, "currTemp is NULL");

    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);

    *currTemp = tZonePtr->currTemp;

    TAF_ERROR_IF_RET_VAL(*currTemp == (uint32_t) -274000, LE_FAULT,
            "Failed to return current temp.");
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZonePassiveTemp
(
    taf_therm_ThermalZoneRef_t listRef, uint32_t* passiveTemp
)
{
    TAF_ERROR_IF_RET_VAL(passiveTemp == NULL, LE_BAD_PARAMETER, "passiveTemp is NULL");

    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);

    *passiveTemp = tZonePtr->passiveTemp;

    TAF_ERROR_IF_RET_VAL(*passiveTemp != (uint32_t) -274000, LE_FAULT,
            "Failed to return passive temp.");
    return LE_OK;
}

le_result_t taf_Therm::GetTripPointListSize(taf_therm_ThermalZoneRef_t listRef, uint32_t* listSize)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT, "Invalid reference (%p) provided!", tZonePtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = tZonePtr->tripPointListSize;
    if(*listSize == 0)
    {
        LE_WARN("List does not exist.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZonesListSize
(
    taf_therm_ThermalZoneListRef_t listRef, uint32_t* listSize
)
{
    taf_ThermalZoneList_t* tZoneListPtr =
            (taf_ThermalZoneList_t*)le_ref_Lookup(tZoneListRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZoneListPtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!",tZoneListPtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = tZoneListPtr->thermalZoneListSize;
    if(*listSize == 0)
    {
        LE_WARN("List does not exist.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t taf_Therm::GetBoundCoolingDeviceListSize
(
    taf_therm_ThermalZoneRef_t listRef, uint32_t* listSize
)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = tZonePtr->boundCoolingDeviceListSize;

    if(*listSize == 0)
    {
        LE_WARN("List does not exist.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZoneType(
    taf_therm_ThermalZoneRef_t listRef, char* thermalZoneType, size_t listSize)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);

    TAF_ERROR_IF_RET_VAL(thermalZoneType == NULL, LE_FAULT, "Thermal zone type is NULL.");

    TAF_ERROR_IF_RET_VAL(listSize == 0, LE_BAD_PARAMETER, "listSize is zero");
    le_utf8_Copy(thermalZoneType, tZonePtr->Type, listSize, NULL);

    return LE_OK;
}


le_result_t taf_Therm::GetTripPointType(
    taf_therm_TripPointRef_t listRef, char* tripType, size_t listSize
)
{
    TAF_ERROR_IF_RET_VAL(tripType == NULL, LE_FAULT, "Failed to return trip type.");
    TAF_ERROR_IF_RET_VAL(listSize == 0, LE_BAD_PARAMETER, "listSize is zero");

    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Failed to retrieve trip point.");

    taf_therm_TripType_t tripTypeInfo = tripPointPtr->tripType;
    switch (tripTypeInfo)
    {
        case TAF_THERM_UNKNOWN:
            le_utf8_Copy(tripType, "UNKNOWN", listSize, NULL);
            break;

        case TAF_THERM_CRITICAL:
            le_utf8_Copy(tripType, "CRITICAL", listSize, NULL);
            break;

        case TAF_THERM_HOT:
            le_utf8_Copy(tripType, "HOT", listSize, NULL);
            break;

        case TAF_THERM_PASSIVE:
            le_utf8_Copy(tripType, "PASSIVE", listSize, NULL);
            break;

        case TAF_THERM_ACTIVE:
            le_utf8_Copy(tripType, "ACTIVE", listSize, NULL);
            break;

        case TAF_THERM_CONFIGURABLE_HIGH:
            le_utf8_Copy(tripType, "CONFIGURABLE_HIGH", listSize, NULL);
            break;

        case TAF_THERM_CONFIGURABLE_LOW:
            le_utf8_Copy(tripType, "CONFIGURABLE_LOW", listSize, NULL);
            break;
    }

    return LE_OK;
}

le_result_t taf_Therm::GetTripPointThreshold(taf_therm_TripPointRef_t listRef, uint32_t* threshold)
{
    TAF_ERROR_IF_RET_VAL(threshold == NULL, LE_BAD_PARAMETER, "threshold is NULL");

    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *threshold = tripPointPtr->threshold;

    TAF_ERROR_IF_RET_VAL(*threshold == (uint32_t) -274000, LE_FAULT, "Failed to return threshold.");
    return LE_OK;
}

le_result_t taf_Therm::GetTripPointHysterisis
(
    taf_therm_TripPointRef_t listRef, uint32_t* hysterisis
)
{
    TAF_ERROR_IF_RET_VAL(hysterisis == NULL, LE_BAD_PARAMETER, "hysterisis is NULL");

    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *hysterisis = tripPointPtr->hysteresis;

    TAF_ERROR_IF_RET_VAL(*hysterisis == (uint32_t) -274000,
             LE_FAULT, "Failed to return hysterisis.");
    return LE_OK;
}

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
le_result_t taf_Therm::GetTripPointTripID(taf_therm_TripPointRef_t listRef, uint32_t* tripID)
        {
            TAF_ERROR_IF_RET_VAL(tripID == NULL, LE_BAD_PARAMETER, "tripID is NULL");

            taf_TripPoint_t* tripPointPtr =
                    (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

            TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
                     tripPointPtr);

            *tripID = tripPointPtr->tripId;
            return LE_OK;
        }
#endif

#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
le_result_t taf_Therm::GetTripPointThermalZoneID
(
    taf_therm_TripPointRef_t listRef, uint32_t* tZoneID
)
        {
            TAF_ERROR_IF_RET_VAL(tZoneID == NULL, LE_BAD_PARAMETER, "tZoneID is NULL");

            taf_TripPoint_t* tripPointPtr =
                    (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

            TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
                    tripPointPtr);

            *tZoneID = tripPointPtr->tZoneId;
            return LE_OK;
        }
#endif

le_result_t taf_Therm::GetCDevID(taf_therm_CoolingDeviceRef_t listRef, uint32_t* cDevID)
{
    TAF_ERROR_IF_RET_VAL(cDevID == NULL, LE_BAD_PARAMETER, "cDevID is NULL");

    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            cDevPtr);

    *cDevID = cDevPtr->cDevId;
    return LE_OK;
}


le_result_t taf_Therm::GetCoolingDeviceListSize
(
    taf_therm_CoolingDeviceListRef_t listRef, uint32_t* listSize
)
{
    taf_CoolingDeviceList_t* cDevPtr =
            (taf_CoolingDeviceList_t*)le_ref_Lookup(cDevListRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            cDevPtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = cDevPtr->coolingDeviceListSize;

    if(*listSize == 0)
    {
        LE_WARN("List does not exist.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t taf_Therm::GetCDevDescription(
    taf_therm_CoolingDeviceRef_t listRef, char* description, size_t listSize)
{
    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",cDevPtr);

    TAF_ERROR_IF_RET_VAL(description == NULL, LE_FAULT,
            "Failed to return cooling device description.");

    TAF_ERROR_IF_RET_VAL(listSize == 0, LE_BAD_PARAMETER, "listSize is zero");
    le_utf8_Copy(description, cDevPtr->description, listSize, NULL);

    return LE_OK;
}

le_result_t taf_Therm::GetCDevMaxCoolingLevel
(
    taf_therm_CoolingDeviceRef_t listRef, uint32_t* maxCoolingLevel
)
{
    TAF_ERROR_IF_RET_VAL(maxCoolingLevel == NULL, LE_BAD_PARAMETER, "maxCoolingLevel is NULL");

    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            cDevPtr);

    *maxCoolingLevel = cDevPtr->maxCoolingLevel;
    return LE_OK;
}

le_result_t taf_Therm::GetCDevCurrentCoolingLevel
(
    taf_therm_CoolingDeviceRef_t listRef, uint32_t* currentCoolingLevel
)
{
    TAF_ERROR_IF_RET_VAL(currentCoolingLevel == NULL, LE_BAD_PARAMETER,
            "currentCoolingLevel is NULL");

    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            cDevPtr);

    *currentCoolingLevel = cDevPtr->currentCoolingLevel;
    return LE_OK;
}

le_result_t taf_Therm::GetBoundCoolingId
(
    taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* boundCoolingId
)
{
    TAF_ERROR_IF_RET_VAL(boundCoolingId == NULL, LE_BAD_PARAMETER, "boundCoolingId is NULL");

    taf_BoundCoolingDevice_t* boundCevPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundCevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            boundCevPtr);

    *boundCoolingId = boundCevPtr->coolingDeviceId;
    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointListSize
(
    taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* listSize
)
{
    taf_BoundCoolingDevice_t* boundCevPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundCevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            boundCevPtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = boundCevPtr->tripPointListSize;

    if(*listSize == 0)
    {
        LE_WARN("List does not exist.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointType(
    taf_therm_TripPointRef_t listRef, char* tripType, size_t listSize
)
{
    TAF_ERROR_IF_RET_VAL(tripType == NULL, LE_FAULT, "Failed to return bound trip point type.");
    TAF_ERROR_IF_RET_VAL(listSize == 0, LE_BAD_PARAMETER, "listSize is zero");

    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr ==NULL,LE_FAULT,"Failed to retrieve trip point.");

    taf_therm_TripType_t tripTypeInfo = tripPointPtr->tripType;
    switch (tripTypeInfo)
    {
        case TAF_THERM_UNKNOWN:
            le_utf8_Copy(tripType, "UNKNOWN", listSize, NULL);
            break;

        case TAF_THERM_CRITICAL:
            le_utf8_Copy(tripType, "CRITICAL", listSize, NULL);
            break;

        case TAF_THERM_HOT:
            le_utf8_Copy(tripType, "HOT", listSize, NULL);
            break;

        case TAF_THERM_PASSIVE:
            le_utf8_Copy(tripType, "PASSIVE", listSize, NULL);
            break;

        case TAF_THERM_ACTIVE:
            le_utf8_Copy(tripType, "ACTIVE", listSize, NULL);
            break;

        case TAF_THERM_CONFIGURABLE_HIGH:
            le_utf8_Copy(tripType, "CONFIGURABLE_HIGH", listSize, NULL);
            break;

        case TAF_THERM_CONFIGURABLE_LOW:
            le_utf8_Copy(tripType, "CONFIGURABLE_LOW", listSize, NULL);
            break;
    }

    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointThreshold
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundThreshold
)
{
    TAF_ERROR_IF_RET_VAL(boundThreshold == NULL, LE_BAD_PARAMETER, "boundThreshold is NULL");

    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundThreshold = tripPointPtr->threshold;
    TAF_ERROR_IF_RET_VAL(*boundThreshold == (uint32_t) -274000, LE_FAULT,
            "Failed to return bound threshold.");
    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointHysterisis
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundHysterisis
)
{
    TAF_ERROR_IF_RET_VAL(boundHysterisis == NULL, LE_BAD_PARAMETER,
            "boundHysterisis is NULL");

    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundHysterisis = tripPointPtr->hysteresis;

    TAF_ERROR_IF_RET_VAL(*boundHysterisis == (uint32_t) -274000, LE_FAULT,
            "Failed to return bound hysterisis.");
    return LE_OK;
}

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
le_result_t taf_Therm::GetBoundTripPointTripID
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundTripID
)
{
    TAF_ERROR_IF_RET_VAL(boundTripID == NULL, LE_BAD_PARAMETER, "boundTripID is NULL");

    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundTripID = tripPointPtr->tripId;
    return LE_OK;
}
#endif
#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
le_result_t taf_Therm::GetBoundTripPointThermalZoneID
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundTZoneID
)
{
    TAF_ERROR_IF_RET_VAL(boundTZoneID == NULL, LE_BAD_PARAMETER, "boundTZoneID is NULL");

    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundTZoneID = tripPointPtr->tZoneId;
    return LE_OK;
}
#endif

int taf_Therm::MapCDevtoId(const char* cDev)
{
    auto& tafTherm = taf_Therm::GetInstance();

    TAF_ERROR_IF_RET_VAL(cDev == NULL, -1, "cDev is NULL");
    TAF_ERROR_IF_RET_VAL(tafTherm.CDevNameToIdMap.find(cDev) == tafTherm.CDevNameToIdMap.end(),
        -1, "CDEV NOT FOUND");

    return tafTherm.CDevNameToIdMap[cDev];
}

taf_therm_CoolingDeviceRef_t taf_Therm::GetCoolingDeviceByName(const char* cDevName)
{
    auto& tafTherm = taf_Therm::GetInstance();

    TAF_ERROR_IF_RET_VAL(thermalManager == nullptr, NULL, "thermalManager is NULL");
    TAF_ERROR_IF_RET_VAL(cDevName == NULL, NULL, "cDevName is NULL");

    int cDevID = MapCDevtoId(cDevName);
    TAF_ERROR_IF_RET_VAL(cDevID == -1, NULL, "NO COOLING DEVICE ASSOICIATED WITH %s",
            cDevName);
    std::shared_ptr<telux::therm::ICoolingDevice> coolingDevices =
            thermalManager->getCoolingDevice(cDevID);
    TAF_ERROR_IF_RET_VAL(coolingDevices == nullptr, NULL, "Failed to get cooling device");
    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);
    memset(cDevPtr, 0, sizeof(taf_CoolingDevice_t));
    le_utf8_Copy(cDevPtr->description, coolingDevices->getDescription().c_str(),
            TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
    cDevPtr->cDevId = coolingDevices->getId();
    cDevPtr->maxCoolingLevel = coolingDevices->getMaxCoolingLevel();
    cDevPtr->currentCoolingLevel = coolingDevices->getCurrentCoolingLevel();
     cDevPtr->ref =
             (taf_therm_CoolingDeviceRef_t)le_ref_CreateRef(cDevRefMap, cDevPtr);
     return cDevPtr->ref;
}

void taf_Therm::EventChanged(void* reportPtr,void* secondLayerHandlerFunc)
{
    taf_TripEventInfo_t* eventType = (taf_TripEventInfo_t*)reportPtr;
    taf_therm_TripEventHandlerFunc_t clientHandlerFunc =
            (taf_therm_TripEventHandlerFunc_t)secondLayerHandlerFunc;

    TAF_ERROR_IF_RET_NIL(eventType == NULL, "eventType is NULL !");
    TAF_ERROR_IF_RET_NIL(eventType->tripPoint == NULL, "tripPoint is NULL !");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == NULL, "clientHandlerFunc is NULL !");

    clientHandlerFunc(eventType->tripPoint->ref,
            eventType->tripEvent,le_event_GetContextPtr());
}

taf_therm_TripEventHandlerRef_t taf_Therm::AddTripEventHandler
(
    taf_therm_TripEventHandlerFunc_t handlerPtr,void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("Trip Type Change",
            stateChangeEvent, EventChanged, (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    taf_EventHandlerInfo_t* handlerInfo =
        (taf_EventHandlerInfo_t*)le_mem_ForceAlloc(tripEventHandlerPool);
    memset(handlerInfo, 0, sizeof(taf_EventHandlerInfo_t));
    handlerInfo->sessionRef = taf_therm_GetClientSessionRef();
    handlerInfo->handlerRef = handlerRef;
    handlerInfo->link = LE_DLS_LINK_INIT;
    handlerInfo->safeRef = le_ref_CreateRef(tripEventHandlerRefMap, handlerInfo);
    le_dls_Queue(&tripEventHandlerList, &handlerInfo->link);

    return (taf_therm_TripEventHandlerRef_t)handlerInfo->safeRef;
}

void taf_Therm::RemoveTripEventHandler(taf_therm_TripEventHandlerRef_t handlerRef)
{
    taf_EventHandlerInfo_t* handlerInfo =
        (taf_EventHandlerInfo_t*)le_ref_Lookup(tripEventHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerInfo == NULL, "Invalid trip event handler reference");

    RemoveEventHandlerByRef(handlerInfo, tripEventHandlerRefMap, &tripEventHandlerList);
    LE_INFO("Removed TripEventChangeHandler");
}

bool taf_Therm::HasTripEventHandlers(void)
{
    return (le_dls_NumLinks(&tripEventHandlerList) > 0);
}

const char* taf_Therm::TripEventToString(telux::therm::TripEvent state)
{
    const char* tripEvent;

    switch (state)
    {
        case telux::therm::TripEvent::CROSSED_UNDER:
            tripEvent = "CROSSED_UNDER";
            break;
        case telux::therm::TripEvent::CROSSED_OVER:
            tripEvent = "CROSSED_OVER";
            break;
        default:
            tripEvent = "NONE";
            break;
    }
    return tripEvent;
}

void taf_ThermServiceListener::onTripEvent
(std::shared_ptr<telux::therm::ITripPoint> tripPoint, telux::therm::TripEvent tripEvent
)
{
    TAF_ERROR_IF_RET_NIL(tripPoint == nullptr, "tripPoint is NULL");

    auto& tafTherm = taf_Therm::GetInstance();
    if (!tafTherm.HasTripEventHandlers())
    {
        LE_DEBUG("No trip event handler registered, skip the event");
        return;
    }
    taf_TripPoint_t* triggeredTripPointPtr =
            (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);
    memset(triggeredTripPointPtr, 0, sizeof(taf_TripPoint_t));
    triggeredTripPointPtr->ref =
          (taf_therm_TripPointRef_t)
          le_ref_CreateRef(tafTherm.tripPointRefMap,triggeredTripPointPtr);
    taf_TripEventInfo_t evt;

    if (tripEvent == telux::therm::TripEvent::NONE) {
        evt.tripEvent = TAF_THERM_NONE;
    }
    else if (tripEvent == telux::therm::TripEvent::CROSSED_UNDER) {
        evt.tripEvent = TAF_THERM_CROSSED_UNDER;

    }
    else if (tripEvent == telux::therm::TripEvent::CROSSED_OVER) {
        evt.tripEvent = TAF_THERM_CROSSED_OVER;
    }
    GetTripType(triggeredTripPointPtr, tripPoint);
    triggeredTripPointPtr->threshold = tripPoint->getThresholdTemp();
    triggeredTripPointPtr->hysteresis = tripPoint->getHysteresis();
    triggeredTripPointPtr->tripId = tripPoint->getTripId();
    triggeredTripPointPtr->tZoneId = tripPoint->getTZoneId();
    evt.tripPoint = triggeredTripPointPtr;

    le_event_Report(tafTherm.stateChangeEvent, &evt, sizeof(evt));
}

void taf_Therm::CoolingLevelChanged(void* reportPtr, void* secondLayerHandlerFunc)
{
    coolingLevelChangeInfo_t* coolingDevice = (coolingLevelChangeInfo_t*)reportPtr;
    taf_therm_CoolingLevelChangeEventHandlerFunc_t clientHandlerFunc =
            (taf_therm_CoolingLevelChangeEventHandlerFunc_t)secondLayerHandlerFunc;
    TAF_ERROR_IF_RET_NIL(coolingDevice == NULL, "coolingDevice is NULL !");
    TAF_ERROR_IF_RET_NIL(coolingDevice->cDevice == NULL, "cDevice is NULL !");
    TAF_ERROR_IF_RET_NIL(clientHandlerFunc == NULL, "clientHandlerFunc is NULL !");

    clientHandlerFunc(coolingDevice->cDevice->ref, le_event_GetContextPtr());
}

bool taf_Therm::HasCoolingLevelHandlers(void)
{
    return (le_dls_NumLinks(&coolingLevelHandlerList) > 0);
}

taf_therm_CoolingLevelChangeEventHandlerRef_t taf_Therm::AddCoolingLevelChangeEventHandler
(
    taf_therm_CoolingLevelChangeEventHandlerFunc_t handlerPtr, void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("Cooling Level Change",
             onCoolingLevelChangeEvent, CoolingLevelChanged, (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    taf_EventHandlerInfo_t* handlerInfo =
        (taf_EventHandlerInfo_t*)le_mem_ForceAlloc(coolingLevelHandlerPool);
    memset(handlerInfo, 0, sizeof(taf_EventHandlerInfo_t));
    handlerInfo->sessionRef = taf_therm_GetClientSessionRef();
    handlerInfo->handlerRef = handlerRef;
    handlerInfo->link = LE_DLS_LINK_INIT;
    handlerInfo->safeRef = le_ref_CreateRef(coolingLevelHandlerRefMap, handlerInfo);
    le_dls_Queue(&coolingLevelHandlerList, &handlerInfo->link);

    return (taf_therm_CoolingLevelChangeEventHandlerRef_t)handlerInfo->safeRef;
}

void taf_ThermServiceListener::onCoolingDeviceLevelChange
(
    std::shared_ptr<telux::therm::ICoolingDevice> coolingDevice
)
{
    TAF_ERROR_IF_RET_NIL(coolingDevice == nullptr, "coolingDevice is NULL");

    auto& tafTherm = taf_Therm::GetInstance();
    if (!tafTherm.HasCoolingLevelHandlers())
    {
        LE_DEBUG("No cooling level change handler registered, skip the event");
        return;
    }
    coolingLevelChangeInfo_t evt;
    taf_CoolingDevice_t* triggeredCDevPtr =
            (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);
    memset(triggeredCDevPtr, 0, sizeof(taf_CoolingDevice_t));
    triggeredCDevPtr->ref =
            (taf_therm_CoolingDeviceRef_t)le_ref_CreateRef(tafTherm.cDevRefMap, triggeredCDevPtr);
    le_utf8_Copy(triggeredCDevPtr->description, coolingDevice->getDescription().c_str(),
            TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
    triggeredCDevPtr->cDevId = coolingDevice->getId();
    triggeredCDevPtr->maxCoolingLevel = coolingDevice->getMaxCoolingLevel();
    triggeredCDevPtr->currentCoolingLevel = coolingDevice->getCurrentCoolingLevel();
    evt.cDevice = triggeredCDevPtr;
    le_event_Report(tafTherm.onCoolingLevelChangeEvent, &evt, sizeof(evt));
}

void taf_Therm::RemoveCoolingLevelChangeEventHandler
(
    taf_therm_CoolingLevelChangeEventHandlerRef_t handlerRef
)
{
    taf_EventHandlerInfo_t* handlerInfo =
        (taf_EventHandlerInfo_t*)le_ref_Lookup(coolingLevelHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerInfo == NULL, "Invalid cooling level handler reference");

    RemoveEventHandlerByRef(handlerInfo, coolingLevelHandlerRefMap, &coolingLevelHandlerList);
    LE_INFO("Removed CoolingLevelChangeEventHandler");
}

le_result_t taf_Therm::ReleaseTripEventRef(taf_therm_TripPointRef_t tripEventRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(tafTherm.tripPointRefMap, tripEventRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_BAD_PARAMETER, "Reference is NULL");

    le_ref_DeleteRef(tafTherm.tripPointRefMap, tripEventRef);
    le_mem_Release(tripPointPtr);
    return LE_OK;
}

le_result_t taf_Therm::ReleaseCoolingDeviceRef(taf_therm_CoolingDeviceRef_t cDevRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    taf_CoolingDevice_t* cDevPtr =
        (taf_CoolingDevice_t*)le_ref_Lookup(tafTherm.cDevRefMap, cDevRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_BAD_PARAMETER, "Reference is NULL");

    le_ref_DeleteRef(tafTherm.cDevRefMap, cDevRef);
    le_mem_Release(cDevPtr);
    return LE_OK;
}

le_result_t taf_Therm::ReleaseThermalZoneRef(taf_therm_ThermalZoneRef_t tZoneRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    taf_ThermalZone_t* tZonePtr =
        (taf_ThermalZone_t*)le_ref_Lookup(tafTherm.tZoneRefMap, tZoneRef);
    TAF_ERROR_IF_RET_VAL(tZonePtr == nullptr, LE_BAD_PARAMETER, "Null reference(tZonePtr)");
    taf_TripPoint_t* tripPointPtr;
    taf_BoundCoolingDevice_t* boundCDPtr;
    taf_TripPoint_t* boundTripPointPtr;
    le_sls_Link_t* tripPointLinkPtr;
    le_sls_Link_t* boundCDLinkPtr;
    le_sls_Link_t* boundTripPointLinkPtr;

    while ((tripPointLinkPtr = le_sls_Pop(&(tZonePtr->TripPointList))) != NULL)
    {
        tripPointPtr = CONTAINER_OF(tripPointLinkPtr, taf_TripPoint_t, link);
        le_ref_DeleteRef(tripPointRefMap, tripPointPtr->ref);
        le_mem_Release(tripPointPtr);
    }
    while ((boundCDLinkPtr = le_sls_Pop(&(tZonePtr->BoundCoolingDeviceList))) != NULL)
    {
        boundCDPtr = CONTAINER_OF(boundCDLinkPtr, taf_BoundCoolingDevice_t, link);
        while ((boundTripPointLinkPtr = le_sls_Pop(&(boundCDPtr->TripPointList))) != NULL)
        {
            boundTripPointPtr = CONTAINER_OF(boundTripPointLinkPtr, taf_TripPoint_t, link);
            le_ref_DeleteRef(boundTripPointRefMap, boundTripPointPtr->ref);
            le_mem_Release(boundTripPointPtr);
        }
        le_ref_DeleteRef(boundCDRefMap, boundCDPtr->ref);
        le_mem_Release(boundCDPtr);
    }
    le_ref_DeleteRef(tZoneRefMap, tZonePtr->ref);
    le_mem_Release(tZonePtr);
    return LE_OK;
}