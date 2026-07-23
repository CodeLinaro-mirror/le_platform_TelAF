/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafThermImpl.cpp
 * @brief      This file describes the implementation method that thermal
 *             service is in use with PA-OSS layer.
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
LE_MEM_DEFINE_STATIC_POOL(OnTripPointEventPool, TAF_THERM_EVENT_POOL_SIZE,
        sizeof(taf_TripEventInfo_t));
LE_MEM_DEFINE_STATIC_POOL(coolingLevelChangeEventPool, TAF_THERM_EVENT_POOL_SIZE,
        sizeof(taf_coolingLevelChangeInfo_t));

LE_REF_DEFINE_STATIC_MAP(tZoneListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tZoneRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(tripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(boundCDRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(boundTripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(cDevListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(cDevRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(onTripPointEventRefMap, TAF_THERM_EVENT_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(coolingLevelChangeEventRefMap, TAF_THERM_EVENT_POOL_SIZE);


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

/*======================================================================

 FUNCTION        ConvertTripTypeFromPA

 DESCRIPTION     Convert PA trip type to service trip type

 PARAMETERS      taf_pa_therm_TripType - PA trip type

 RETURN VALUE    taf_therm_TripType_t

======================================================================*/
static taf_therm_TripType_t ConvertTripTypeFromPA(taf_pa_therm_TripType tripType)
{
    switch (tripType)
    {
        case taf_pa_therm_TripType::CRITICAL:
            return TAF_THERM_CRITICAL;
        case taf_pa_therm_TripType::HOT:
            return TAF_THERM_HOT;
        case taf_pa_therm_TripType::PASSIVE:
            return TAF_THERM_PASSIVE;
        case taf_pa_therm_TripType::ACTIVE:
            return TAF_THERM_ACTIVE;
        case taf_pa_therm_TripType::CONFIGURABLE_HIGH:
            return TAF_THERM_CONFIGURABLE_HIGH;
        case taf_pa_therm_TripType::CONFIGURABLE_LOW:
            return TAF_THERM_CONFIGURABLE_LOW;
        default:
            return TAF_THERM_UNKNOWN;
    }
}

/*======================================================================

 FUNCTION        ProcessTripEvent

 DESCRIPTION     Process trip event in Legato thread context

 PARAMETERS      reportPtr - Event report from event queue

 RETURN VALUE    None

======================================================================*/
static void ProcessTripEvent(void* reportPtr)
{
    taf_pa_therm_TripEventInfo* eventInfo = (taf_pa_therm_TripEventInfo*)reportPtr;
    if (!eventInfo)
    {
      LE_ERROR("Reported event doesn't exist");
      return;
    }

    auto& tafTherm = taf_Therm::GetInstance();

    if (!tafTherm.HasTripEventHandlers())
    {
        LE_DEBUG("No trip event handler registered, skip the event");
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafTherm.onTripPointEventRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        // Get the handler registration info
        taf_TripEventInfo_t* registrationPtr = (taf_TripEventInfo_t*)le_ref_GetValue(iterRef);

        if (!registrationPtr || !registrationPtr->handlerFuncPtr)
        {
            LE_WARN("Found invalid handler registration, skipping.");
            continue;
        }

        taf_TripPoint_t* tripPointDataPtr =
            (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);

        memset(tripPointDataPtr, 0, sizeof(taf_TripPoint_t));

        tripPointDataPtr->tripType   = ConvertTripTypeFromPA(eventInfo->tripPoint.tripType);
        tripPointDataPtr->threshold  = eventInfo->tripPoint.threshold;
        tripPointDataPtr->hysteresis = eventInfo->tripPoint.hysteresis;
        tripPointDataPtr->tripId     = eventInfo->tripPoint.tripId;
        tripPointDataPtr->tZoneId    = eventInfo->tripPoint.thermalZoneId;

        tripPointDataPtr->ref = (taf_therm_TripPointRef_t)le_ref_CreateRef(
            tafTherm.tripPointRefMap, tripPointDataPtr);

        // Convert the event type enum
        taf_therm_TripEventType_t tripEventType;
        switch (eventInfo->tripEvent)
        {
            case taf_pa_therm_TripEvent::CROSSED_OVER:
                tripEventType = TAF_THERM_CROSSED_OVER;
                break;
            case taf_pa_therm_TripEvent::CROSSED_UNDER:
                tripEventType = TAF_THERM_CROSSED_UNDER;
                break;
            default:
                tripEventType = TAF_THERM_NONE;
                break;
        }

        LE_INFO("Calling trip event handler. Ref: %p", tripPointDataPtr->ref);

        registrationPtr->handlerFuncPtr(tripPointDataPtr->ref, tripEventType,
            registrationPtr->contextPtr);
    }
}

/*======================================================================

 FUNCTION        TripEventPAHandler

 DESCRIPTION     PA layer callback for trip events (called from non-Legato thread)

 PARAMETERS      taf_pa_therm_TripEventInfo - Trip event information from PA

 RETURN VALUE    None

======================================================================*/
static void TripEventPAHandler(taf_pa_therm_TripEventInfo eventInfo)
{
    LE_DEBUG("Trip event received from PA layer");

    // Queue the event to be processed in Legato thread context
    auto& tafTherm = taf_Therm::GetInstance();
    le_event_Report(tafTherm.tripEventId, &eventInfo, sizeof(eventInfo));
}

/*======================================================================

 FUNCTION        ProcessCoolingLevelChangeEvent

 DESCRIPTION     Process cooling level change in Legato thread context

 PARAMETERS      reportPtr - Event report from event queue

 RETURN VALUE    None

======================================================================*/
static void ProcessCoolingLevelChangeEvent(void* reportPtr)
{
    taf_pa_therm_CoolingLevelChangeInfo* changeInfo =
        (taf_pa_therm_CoolingLevelChangeInfo*)reportPtr;

    if (!changeInfo)
    {
      LE_ERROR("Reported event doesn't exist");
      return;
    }

    auto& tafTherm = taf_Therm::GetInstance();

    if (!tafTherm.HasCoolingLevelHandlers())
    {
        LE_DEBUG("No cooling level change handler registered, skip the event");
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafTherm.coolingLevelChangeEventRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        // Get the handler registration info
        taf_coolingLevelChangeInfo_t* registrationPtr =
            (taf_coolingLevelChangeInfo_t*)le_ref_GetValue(iterRef);

        if (!registrationPtr || !registrationPtr->handlerFuncPtr)
        {
            LE_WARN("No registered client for on cooling level change!");
            continue;
        }

        taf_CoolingDevice_t* cDeviceDataPtr =
            (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);

            memset(cDeviceDataPtr, 0, sizeof(taf_CoolingDevice_t));

        cDeviceDataPtr->cDevId = changeInfo->coolingDevice.deviceId;
        cDeviceDataPtr->maxCoolingLevel = changeInfo->coolingDevice.maxCoolingLevel;
        cDeviceDataPtr->currentCoolingLevel = changeInfo->coolingDevice.currentCoolingLevel;

        // Copy string safely into the new object
        le_utf8_Copy(cDeviceDataPtr->description,
                changeInfo->coolingDevice.description.c_str(),
                sizeof(cDeviceDataPtr->description), NULL);

        cDeviceDataPtr->ref =
            (taf_therm_CoolingDeviceRef_t)le_ref_CreateRef(
                tafTherm.cDevRefMap, cDeviceDataPtr);

        LE_INFO("Calling cooling level change handler. Ref: %p", cDeviceDataPtr->ref);

        registrationPtr->handlerFuncPtr(cDeviceDataPtr->ref, registrationPtr->contextPtr);
    }
}

/*======================================================================

 FUNCTION        CoolingLevelChangePAHandler

 DESCRIPTION     PA layer callback for cooling level changes (called from non-Legato thread)

 PARAMETERS      taf_pa_therm_CoolingLevelChangeInfo - Cooling level change info from PA

 RETURN VALUE    None

======================================================================*/
static void CoolingLevelChangePAHandler(taf_pa_therm_CoolingLevelChangeInfo changeInfo)
{
    // Queue the event to be processed in Legato thread context
    auto& tafTherm = taf_Therm::GetInstance();
    le_event_Report(tafTherm.coolingLevelChangeEventId, &changeInfo,
        sizeof(changeInfo));
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
    LE_INFO("Initializing Thermal Service with PA layer");

    // Initialize PA layer
    taf_pa_result_t result = taf_pa_therm_Init();
    if (result != TAF_PA_OK)
    {
        LE_FATAL("ERROR - Failed to initialize Thermal PA layer.");
    }
    LE_INFO("Thermal PA layer initialized successfully");

    // Create event IDs for queuing events from non-Legato threads
    tripEventId = le_event_CreateId("TripEvent", sizeof(taf_pa_therm_TripEventInfo));
    coolingLevelChangeEventId =
        le_event_CreateId("CoolingLevelChange", sizeof(taf_pa_therm_CoolingLevelChangeInfo));

    // Add handlers for the events
    le_event_AddHandler("TripEventHandler", tripEventId, ProcessTripEvent);

    le_event_AddHandler("CoolingLevelChangeHandler",
        coolingLevelChangeEventId, ProcessCoolingLevelChangeEvent);

    // Initialize memory pools
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
    onTripPointEventPool = le_mem_InitStaticPool(OnTripPointEventPool,TAF_THERM_EVENT_POOL_SIZE,
        sizeof(taf_TripEventInfo_t));
    coolingLevelChangeEventPool = le_mem_InitStaticPool(coolingLevelChangeEventPool,
        TAF_THERM_EVENT_POOL_SIZE, sizeof(taf_coolingLevelChangeInfo_t));

    tZoneListRefMap = le_ref_InitStaticMap(tZoneListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    tZoneRefMap = le_ref_InitStaticMap(tZoneRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    tripPointRefMap = le_ref_InitStaticMap(tripPointRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    boundCDRefMap = le_ref_InitStaticMap(boundCDRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    boundTripPointRefMap = le_ref_InitStaticMap(boundTripPointRefMap,TAF_THERM_MAX_LIST_POOL_SIZE);
    cDevListRefMap = le_ref_InitStaticMap(cDevListRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    cDevRefMap = le_ref_InitStaticMap(cDevRefMap, TAF_THERM_MAX_LIST_POOL_SIZE);
    onTripPointEventRefMap =le_ref_InitStaticMap(onTripPointEventRefMap,TAF_THERM_EVENT_POOL_SIZE);
    coolingLevelChangeEventRefMap =
        le_ref_InitStaticMap(coolingLevelChangeEventRefMap,TAF_THERM_EVENT_POOL_SIZE);


    le_msg_AddServiceCloseHandler(taf_therm_GetServiceRef(),
        taf_Handler::OnClientDisconnection, NULL);

    // Register PA layer callbacks
    result = taf_pa_therm_RegisterTripEventHandler(TripEventPAHandler, NULL);

    if (result != TAF_PA_OK)
    {
        LE_ERROR("Failed to register trip event handler");
    }

    result = taf_pa_therm_RegisterCoolingLevelChangeHandler(CoolingLevelChangePAHandler, NULL);
    if (result != TAF_PA_OK)
    {
        LE_ERROR("Failed to register cooling level change handler");
    }

    LE_INFO("Thermal service initialization complete");
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

    // Clean up trip event handlers for this session
    le_ref_IterRef_t tripEvtIterRef = le_ref_GetIterator(tafThermalMgr.onTripPointEventRefMap);
    std::vector<taf_therm_TripEventHandlerRef_t> tripHandlersToDelete;
    while (le_ref_NextNode(tripEvtIterRef) == LE_OK)
    {
        taf_TripEventInfo_t* evtPtr =
            (taf_TripEventInfo_t*)le_ref_GetValue(tripEvtIterRef);
        if (evtPtr && evtPtr->sessionRef == sessionRef)
        {
            tripHandlersToDelete.push_back(evtPtr->handlerRef);
        }
    }
    for (auto ref : tripHandlersToDelete)
    {
        tafThermalMgr.RemoveTripEventHandler(ref);
    }

    // Clean up cooling level change handlers for this session
    le_ref_IterRef_t coolingEvtIterRef =
        le_ref_GetIterator(tafThermalMgr.coolingLevelChangeEventRefMap);
    std::vector<taf_therm_CoolingLevelChangeEventHandlerRef_t> coolingHandlersToDelete;
    while (le_ref_NextNode(coolingEvtIterRef) == LE_OK)
    {
        taf_coolingLevelChangeInfo_t* evtPtr =
            (taf_coolingLevelChangeInfo_t*)le_ref_GetValue(coolingEvtIterRef);
        if (evtPtr && evtPtr->sessionRef == sessionRef)
        {
            coolingHandlersToDelete.push_back(evtPtr->handlerRef);
        }
    }
    for (auto ref : coolingHandlersToDelete)
    {
        tafThermalMgr.RemoveCoolingLevelChangeEventHandler(ref);
    }
}

taf_therm_ThermalZoneListRef_t taf_Therm::GetThermalZonesList()
{
    auto& tafTherm = taf_Therm::GetInstance();

    taf_ThermalZoneList_t* tZoneListPtr =
            (taf_ThermalZoneList_t*)le_mem_ForceAlloc(tafTherm.tZoneListPool);
    memset(tZoneListPtr, 0, sizeof(taf_ThermalZoneList_t));
    tZoneListPtr->ThermalZoneList = LE_SLS_LIST_INIT;
    tZoneListPtr->sessionRef = taf_therm_GetClientSessionRef();
    tZoneListPtr->currPtr = NULL;

    // Get thermal zones from PA layer
    std::vector<taf_pa_therm_ThermalZoneInfo> zonesInfo;
    taf_pa_result_t result = taf_pa_therm_GetThermalZones(zonesInfo);

    if (result != TAF_PA_OK)
    {
        LE_ERROR("Failed to get thermal zones from PA layer");
        le_mem_Release(tZoneListPtr);
        return NULL;
    }

    tZoneListPtr->thermalZoneListSize = zonesInfo.size();

    if (zonesInfo.size() > 0)
    {
        for (size_t i = 0; i < zonesInfo.size(); i++)
        {
            taf_ThermalZone_t* tZonePtr =(taf_ThermalZone_t*)le_mem_ForceAlloc(tafTherm.tZonePool);
            memset(tZonePtr, 0, sizeof(taf_ThermalZone_t));
            tZonePtr->TripPointList = LE_SLS_LIST_INIT;
            tZonePtr->BoundCoolingDeviceList = LE_SLS_LIST_INIT;
            tZonePtr->currPtr = NULL;

            le_utf8_Copy(tZonePtr->Type, zonesInfo[i].description.c_str(),
                    TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
            tZonePtr->tZoneId = zonesInfo[i].zoneId;
            tZonePtr->currTemp = zonesInfo[i].currentTemp;
            tZonePtr->passiveTemp = zonesInfo[i].passiveTemp;

            // Get trip points for this zone
            std::vector<taf_pa_therm_TripPointInfo> tripPoints;
            result = taf_pa_therm_GetTripPoints(zonesInfo[i].zoneId, tripPoints);

            if (result == TAF_PA_OK)
            {
                tZonePtr->tripPointListSize = tripPoints.size();
                for (size_t t = 0; t < tripPoints.size(); t++)
                {
                    taf_TripPoint_t* tripPointPtr =
                        (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);
                    memset(tripPointPtr, 0, sizeof(taf_TripPoint_t));

                    tripPointPtr->tripType = ConvertTripTypeFromPA(tripPoints[t].tripType);
                    tripPointPtr->threshold = tripPoints[t].threshold;
                    tripPointPtr->hysteresis = tripPoints[t].hysteresis;
                    tripPointPtr->tripId = tripPoints[t].tripId;
                    tripPointPtr->tZoneId = tripPoints[t].thermalZoneId;

                    tripPointPtr->link = LE_SLS_LINK_INIT;
                    le_sls_Queue(&(tZonePtr->TripPointList), &(tripPointPtr->link));
                    tripPointPtr->ref =
                        (taf_therm_TripPointRef_t)le_ref_CreateRef(tripPointRefMap, tripPointPtr);
                }
            }

            // Get bound cooling devices for this zone
            std::vector<taf_pa_therm_BoundCoolingDevice> boundDevices;
            result = taf_pa_therm_GetBoundCoolingDevices(zonesInfo[i].zoneId, boundDevices);

            if (result == TAF_PA_OK)
            {
                tZonePtr->boundCoolingDeviceListSize = boundDevices.size();
                for (size_t t = 0; t < boundDevices.size(); t++)
                {
                    taf_BoundCoolingDevice_t* boundCoolingDevicePtr =
                        (taf_BoundCoolingDevice_t*)le_mem_ForceAlloc(tafTherm.boundCDPool);
                    memset(boundCoolingDevicePtr, 0, sizeof(taf_BoundCoolingDevice_t));
                    boundCoolingDevicePtr->TripPointList = LE_SLS_LIST_INIT;
                    boundCoolingDevicePtr->currPtr = NULL;
                    boundCoolingDevicePtr->coolingDeviceId = boundDevices[t].coolingDeviceId;
                    boundCoolingDevicePtr->tripPointListSize = boundDevices[t].bindingInfo.size();

                    for (size_t idx = 0; idx < boundDevices[t].bindingInfo.size(); idx++)
                    {
                        taf_TripPoint_t* bindingPtr =
                            (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.boundTripPointCDPool);
                        memset(bindingPtr, 0, sizeof(taf_TripPoint_t));

                        bindingPtr->tripType =
                            ConvertTripTypeFromPA(boundDevices[t].bindingInfo[idx].tripType);
                        bindingPtr->threshold = boundDevices[t].bindingInfo[idx].threshold;
                        bindingPtr->hysteresis = boundDevices[t].bindingInfo[idx].hysteresis;
                        bindingPtr->tripId = boundDevices[t].bindingInfo[idx].tripId;
                        bindingPtr->tZoneId = boundDevices[t].bindingInfo[idx].thermalZoneId;

                        bindingPtr->link = LE_SLS_LINK_INIT;
                        le_sls_Queue(&(boundCoolingDevicePtr->TripPointList), &(bindingPtr->link));
                        bindingPtr->ref =
                            (taf_therm_TripPointRef_t)le_ref_CreateRef(
                                boundTripPointRefMap, bindingPtr);
                    }

                    boundCoolingDevicePtr->link = LE_SLS_LINK_INIT;
                    le_sls_Queue(&(tZonePtr->BoundCoolingDeviceList),
                        &(boundCoolingDevicePtr->link));

                    boundCoolingDevicePtr->ref =
                         (taf_therm_BoundCoolingDeviceRef_t)
                         le_ref_CreateRef(boundCDRefMap, boundCoolingDevicePtr);
                }
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

    TAF_ERROR_IF_RET_VAL(boundTripPointPtr==NULL,NULL,"Failed to retrieve next bound trip point");

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

    taf_CoolingDeviceList_t* cDevListPtr =
            (taf_CoolingDeviceList_t*)le_mem_ForceAlloc(tafTherm.cDevListPool);
    memset(cDevListPtr, 0, sizeof(taf_CoolingDeviceList_t));
    cDevListPtr->CoolingDeviceList = LE_SLS_LIST_INIT;
    cDevListPtr->sessionRef = taf_therm_GetClientSessionRef();
    cDevListPtr->currPtr = NULL;

    // Get cooling devices from PA layer
    std::vector<taf_pa_therm_CoolingDeviceInfo> coolingDevices;
    taf_pa_result_t result = taf_pa_therm_GetCoolingDevices(coolingDevices);

    if (result != TAF_PA_OK)
    {
        LE_ERROR("Failed to get cooling devices from PA layer");
        le_mem_Release(cDevListPtr);
        return NULL;
    }

    cDevListPtr->coolingDeviceListSize = coolingDevices.size();

    if (coolingDevices.size() > 0)
    {
        for (size_t i = 0; i < coolingDevices.size(); i++)
        {
            taf_CoolingDevice_t* cDevPtr =
                (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);
            memset(cDevPtr, 0, sizeof(taf_CoolingDevice_t));

            le_utf8_Copy(cDevPtr->description, coolingDevices[i].description.c_str(),
                sizeof(cDevPtr->description), NULL);
            cDevPtr->cDevId = coolingDevices[i].deviceId;
            cDevPtr->maxCoolingLevel = coolingDevices[i].maxCoolingLevel;
            cDevPtr->currentCoolingLevel = coolingDevices[i].currentCoolingLevel;

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

    taf_pa_therm_ThermalZoneInfo zoneInfo;
    taf_pa_result_t result = taf_pa_therm_GetThermalZoneByName(std::string(thermalZoneName), zoneInfo);

    if (result != TAF_PA_OK)
    {
        LE_ERROR("NO THERMAL ZONE ASSOCIATED WITH %s", thermalZoneName);
        return NULL;
    }

    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_mem_ForceAlloc(tafTherm.tZonePool);
    memset(tZonePtr, 0, sizeof(taf_ThermalZone_t));
    tZonePtr->TripPointList = LE_SLS_LIST_INIT;
    tZonePtr->BoundCoolingDeviceList = LE_SLS_LIST_INIT;
    tZonePtr->currPtr = NULL;

    le_utf8_Copy(tZonePtr->Type, zoneInfo.description.c_str(),
        TAF_THERM_ZONE_TYPE_MAX_SIZE, NULL);
    tZonePtr->tZoneId = zoneInfo.zoneId;
    tZonePtr->currTemp = zoneInfo.currentTemp;
    tZonePtr->passiveTemp = zoneInfo.passiveTemp;

    // Get trip points
    std::vector<taf_pa_therm_TripPointInfo> tripPoints;
    result = taf_pa_therm_GetTripPoints(zoneInfo.zoneId, tripPoints);

    if (result == TAF_PA_OK)
    {
        tZonePtr->tripPointListSize = tripPoints.size();
        for (size_t t = 0; t < tripPoints.size(); t++)
        {
            taf_TripPoint_t* tripPointPtr =
                (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.tripPointPool);
            memset(tripPointPtr, 0, sizeof(taf_TripPoint_t));

            tripPointPtr->tripType = ConvertTripTypeFromPA(tripPoints[t].tripType);
            tripPointPtr->threshold = tripPoints[t].threshold;
            tripPointPtr->hysteresis = tripPoints[t].hysteresis;
            tripPointPtr->tripId = tripPoints[t].tripId;
            tripPointPtr->tZoneId = tripPoints[t].thermalZoneId;

            tripPointPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(tZonePtr->TripPointList), &(tripPointPtr->link));
            tripPointPtr->ref =
                    (taf_therm_TripPointRef_t)le_ref_CreateRef(tripPointRefMap, tripPointPtr);
        }
    }

    // Get bound cooling devices
    std::vector<taf_pa_therm_BoundCoolingDevice> boundDevices;
    result = taf_pa_therm_GetBoundCoolingDevices(zoneInfo.zoneId, boundDevices);

    if (result == TAF_PA_OK)
    {
        tZonePtr->boundCoolingDeviceListSize = boundDevices.size();
        for (size_t t = 0; t < boundDevices.size(); t++)
        {
            taf_BoundCoolingDevice_t* boundCoolingDevicePtr =
                (taf_BoundCoolingDevice_t*)le_mem_ForceAlloc(tafTherm.boundCDPool);
            memset(boundCoolingDevicePtr, 0, sizeof(taf_BoundCoolingDevice_t));
            boundCoolingDevicePtr->TripPointList = LE_SLS_LIST_INIT;
            boundCoolingDevicePtr->currPtr = NULL;
            boundCoolingDevicePtr->coolingDeviceId = boundDevices[t].coolingDeviceId;
            boundCoolingDevicePtr->tripPointListSize = boundDevices[t].bindingInfo.size();

            for (size_t idx = 0; idx < boundDevices[t].bindingInfo.size(); idx++)
            {
                taf_TripPoint_t* bindingPtr =
                    (taf_TripPoint_t*)le_mem_ForceAlloc(tafTherm.boundTripPointCDPool);
                memset(bindingPtr, 0, sizeof(taf_TripPoint_t));

                bindingPtr->tripType =
                    ConvertTripTypeFromPA(boundDevices[t].bindingInfo[idx].tripType);
                bindingPtr->threshold = boundDevices[t].bindingInfo[idx].threshold;
                bindingPtr->hysteresis = boundDevices[t].bindingInfo[idx].hysteresis;
                bindingPtr->tripId = boundDevices[t].bindingInfo[idx].tripId;
                bindingPtr->tZoneId = boundDevices[t].bindingInfo[idx].thermalZoneId;

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
    }
    tZonePtr->ref = (taf_therm_ThermalZoneRef_t)le_ref_CreateRef(tZoneRefMap, tZonePtr);
    return tZonePtr->ref;
}

taf_therm_CoolingDeviceRef_t taf_Therm::GetCoolingDeviceByName(const char* cDevName)
{
    auto& tafTherm = taf_Therm::GetInstance();

    taf_pa_therm_CoolingDeviceInfo deviceInfo;
    taf_pa_result_t result = taf_pa_therm_GetCoolingDeviceByName(std::string(cDevName), deviceInfo);

    if (result != TAF_PA_OK)
    {
        LE_ERROR("NO COOLING DEVICE ASSOCIATED WITH %s", cDevName);
        return NULL;
    }

    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_mem_ForceAlloc(tafTherm.cDevPool);
    memset(cDevPtr, 0, sizeof(taf_CoolingDevice_t));

    le_utf8_Copy(cDevPtr->description, deviceInfo.description.c_str(),
            sizeof(cDevPtr->description), NULL);
    cDevPtr->cDevId = deviceInfo.deviceId;
    cDevPtr->maxCoolingLevel = deviceInfo.maxCoolingLevel;
    cDevPtr->currentCoolingLevel = deviceInfo.currentCoolingLevel;
    cDevPtr->ref = (taf_therm_CoolingDeviceRef_t)le_ref_CreateRef(cDevRefMap, cDevPtr);

    return cDevPtr->ref;
}

le_result_t taf_Therm::GetThermalZoneID(taf_therm_ThermalZoneRef_t listRef,uint32_t* thermalZoneID)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT, "Failed to retrieve thermal zone.");

    *thermalZoneID = tZonePtr->tZoneId;
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZoneCurrentTemp
(
    taf_therm_ThermalZoneRef_t listRef, int32_t* currTemp
)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);

    *currTemp = tZonePtr->currTemp;

    TAF_ERROR_IF_RET_VAL(*currTemp == (int32_t) -274000, LE_FAULT,
            "Failed to return current temp.");
    return LE_OK;
}

le_result_t taf_Therm::GetThermalZonePassiveTemp
(
    taf_therm_ThermalZoneRef_t listRef, int32_t* passiveTemp
)
{
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tZonePtr == NULL, LE_FAULT,
            "Invalid reference (%p) provided!", tZonePtr);

    *passiveTemp = tZonePtr->passiveTemp;

    TAF_ERROR_IF_RET_VAL(*passiveTemp != (int32_t) -274000, LE_FAULT,
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

le_result_t taf_Therm::GetTripPointThreshold(taf_therm_TripPointRef_t listRef, int32_t* threshold)
{
    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *threshold = tripPointPtr->threshold;

    TAF_ERROR_IF_RET_VAL(*threshold == (int32_t) -274000, LE_FAULT,"Failed to return threshold.");
    return LE_OK;
}

le_result_t taf_Therm::GetTripPointHysterisis
(
    taf_therm_TripPointRef_t listRef, int32_t* hysterisis
)
{
    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *hysterisis = tripPointPtr->hysteresis;

    TAF_ERROR_IF_RET_VAL(*hysterisis == (int32_t) -274000,
             LE_FAULT, "Failed to return hysterisis.");
    return LE_OK;
}

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
le_result_t taf_Therm::GetTripPointTripID(taf_therm_TripPointRef_t listRef, uint32_t* tripID)
{
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
             tripPointPtr);

    *tripID = tripPointPtr->tripId;

    TAF_ERROR_IF_RET_VAL(*tripID < 0, LE_FAULT, "Failed to return tripID.");
    return LE_OK;
}
#endif

#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
le_result_t taf_Therm::GetTripPointThermalZoneID
(
    taf_therm_TripPointRef_t listRef, uint32_t* tZoneID
)
{
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
            tripPointPtr);

    *tZoneID = tripPointPtr->tZoneId;

    TAF_ERROR_IF_RET_VAL(*tZoneID < 0, LE_FAULT, "Failed to return thermal zone ID.");
    return LE_OK;
}
#endif

le_result_t taf_Therm::GetCDevID(taf_therm_CoolingDeviceRef_t listRef, uint32_t* cDevID)
{
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
    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!", cDevPtr);

    *maxCoolingLevel = cDevPtr->maxCoolingLevel;
    return LE_OK;
}

le_result_t taf_Therm::GetCDevCurrentCoolingLevel
(
    taf_therm_CoolingDeviceRef_t listRef, uint32_t* currentCoolingLevel
)
{
    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(cDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!", cDevPtr);

    *currentCoolingLevel = cDevPtr->currentCoolingLevel;
    return LE_OK;
}

le_result_t taf_Therm::GetBoundCoolingId
(
    taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* boundCoolingId
)
{
    taf_BoundCoolingDevice_t* boundCDPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundCDPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            boundCDPtr);

    *boundCoolingId = boundCDPtr->coolingDeviceId;
    TAF_ERROR_IF_RET_VAL(*boundCoolingId < 0, LE_FAULT, "Failed to return bound cooling ID.");
    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointListSize
(
    taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* listSize
)
{
    taf_BoundCoolingDevice_t* boundCDPtr =
            (taf_BoundCoolingDevice_t*)le_ref_Lookup(boundCDRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(boundCDPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            boundCDPtr);
    TAF_ERROR_IF_RET_VAL(listSize == NULL, LE_FAULT,
        "Invalid address for listsize (%p) provided!", listSize);

    *listSize = boundCDPtr->tripPointListSize;

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

    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Failed to retrieve bound trip point.");

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
    taf_therm_TripPointRef_t listRef, int32_t* boundThreshold
)
{
    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundThreshold = tripPointPtr->threshold;

    TAF_ERROR_IF_RET_VAL(*boundThreshold == (int32_t) -274000, LE_FAULT,
            "Failed to return bound threshold.");
    return LE_OK;
}

le_result_t taf_Therm::GetBoundTripPointHysterisis
(
    taf_therm_TripPointRef_t listRef, int32_t* boundHysterisis
)
{
    taf_TripPoint_t* tripPointPtr = (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            tripPointPtr);

    *boundHysterisis = tripPointPtr->hysteresis;

    TAF_ERROR_IF_RET_VAL(*boundHysterisis == (int32_t) -274000,
             LE_FAULT, "Failed to return bound hysterisis.");
    return LE_OK;
}

#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
le_result_t taf_Therm::GetBoundTripPointTripID
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundTripID
)
{
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
             tripPointPtr);

    *boundTripID = tripPointPtr->tripId;

    TAF_ERROR_IF_RET_VAL(*boundTripID < 0, LE_FAULT, "Failed to return bound tripID.");
    return LE_OK;
}
#endif

#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
le_result_t taf_Therm::GetBoundTripPointThermalZoneID
(
    taf_therm_TripPointRef_t listRef, uint32_t* boundTZoneID
)
{
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(boundTripPointRefMap, listRef);

    TAF_ERROR_IF_RET_VAL(tripPointPtr == NULL, LE_FAULT,"Invalid reference (%p) provided!",
            tripPointPtr);

    *boundTZoneID = tripPointPtr->tZoneId;

    TAF_ERROR_IF_RET_VAL(*boundTZoneID < 0, LE_FAULT, "Failed to return bound thermal zone ID.");
    return LE_OK;
}
#endif



taf_therm_TripEventHandlerRef_t taf_Therm::AddTripEventHandler
(
    taf_therm_TripEventHandlerFunc_t handlerPtr, void* contextPtr
)
{
    taf_TripEventInfo_t* evtInfoPtr =
        (taf_TripEventInfo_t*)le_mem_ForceAlloc(onTripPointEventPool);
    TAF_ERROR_IF_RET_VAL(evtInfoPtr == NULL, NULL, "evtInfoPtr is NULL !");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");

    evtInfoPtr->handlerRef =
                (taf_therm_TripEventHandlerRef_t)le_ref_CreateRef(onTripPointEventRefMap, evtInfoPtr);
    evtInfoPtr->handlerFuncPtr = handlerPtr;
    evtInfoPtr->contextPtr = contextPtr;
    evtInfoPtr->sessionRef = taf_therm_GetClientSessionRef();
    return (taf_therm_TripEventHandlerRef_t) evtInfoPtr->handlerRef;
}

void taf_Therm::RemoveTripEventHandler(taf_therm_TripEventHandlerRef_t eventRef)
{
    TAF_ERROR_IF_RET_NIL(eventRef == nullptr, "Invalid para(null reference)");
    taf_TripEventInfo_t* evtPtr =
        (taf_TripEventInfo_t*)le_ref_Lookup(onTripPointEventRefMap, eventRef);
    TAF_ERROR_IF_RET_NIL(evtPtr == nullptr, "Invalid para(null reference ptr)");
    LE_DEBUG("Release TripEventChangeHandlerRef : %p", eventRef);
    le_ref_DeleteRef(onTripPointEventRefMap, eventRef);
    le_mem_Release(evtPtr);
    LE_INFO("Removed TripEventChangeHandler");
}

bool taf_Therm::HasTripEventHandlers(void)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(onTripPointEventRefMap);
    return (le_ref_NextNode(iterRef) == LE_OK);
}

bool taf_Therm::HasCoolingLevelHandlers(void)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(coolingLevelChangeEventRefMap);
    return (le_ref_NextNode(iterRef) == LE_OK);
}


taf_therm_CoolingLevelChangeEventHandlerRef_t taf_Therm::AddCoolingLevelChangeEventHandler
(
    taf_therm_CoolingLevelChangeEventHandlerFunc_t handlerPtr, void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "handlerPtr is NULL !");
    taf_coolingLevelChangeInfo_t* evtInfoPtr =
        (taf_coolingLevelChangeInfo_t*)le_mem_ForceAlloc(coolingLevelChangeEventPool);
    TAF_ERROR_IF_RET_VAL(evtInfoPtr == NULL, NULL, "evtInfoPtr is NULL !");

    evtInfoPtr->handlerRef =
    (taf_therm_CoolingLevelChangeEventHandlerRef_t)le_ref_CreateRef(
        coolingLevelChangeEventRefMap, evtInfoPtr);
    evtInfoPtr->handlerFuncPtr = handlerPtr;
    evtInfoPtr->contextPtr = contextPtr;
    evtInfoPtr->sessionRef = taf_therm_GetClientSessionRef();
    return (taf_therm_CoolingLevelChangeEventHandlerRef_t) evtInfoPtr->handlerRef;
}

void taf_Therm::RemoveCoolingLevelChangeEventHandler(
    taf_therm_CoolingLevelChangeEventHandlerRef_t eventRef
)
{
     TAF_ERROR_IF_RET_NIL(eventRef == nullptr, "Invalid para(null reference)");
    taf_coolingLevelChangeInfo_t* evtPtr =
        (taf_coolingLevelChangeInfo_t*)le_ref_Lookup(coolingLevelChangeEventRefMap, eventRef);
    TAF_ERROR_IF_RET_NIL(evtPtr == nullptr, "Invalid para(null reference ptr)");
    LE_DEBUG("ReleaseModemEvt : %p", eventRef);
    le_ref_DeleteRef(coolingLevelChangeEventRefMap, eventRef);
    le_mem_Release(evtPtr);
}

le_result_t taf_Therm::ReleaseThermalZoneRef(taf_therm_ThermalZoneRef_t tZoneRef)
{
    TAF_ERROR_IF_RET_VAL(tZoneRef == nullptr, LE_NOT_FOUND, "Invalid para(null reference)");
    taf_ThermalZone_t* tZonePtr = (taf_ThermalZone_t*)le_ref_Lookup(tZoneRefMap, tZoneRef);
    TAF_ERROR_IF_RET_VAL(tZonePtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("ReleaseThermalZoneRef : %p", tZoneRef);
    le_ref_DeleteRef(tZoneRefMap, tZoneRef);
    le_mem_Release(tZonePtr);
    return LE_OK;
}

le_result_t taf_Therm::ReleaseTripEventRef(taf_therm_TripPointRef_t tripEventRef)
{
    TAF_ERROR_IF_RET_VAL(tripEventRef == nullptr, LE_NOT_FOUND, "Invalid para(null reference)");
    taf_TripPoint_t* tripPointPtr =
            (taf_TripPoint_t*)le_ref_Lookup(tripPointRefMap, tripEventRef);
    TAF_ERROR_IF_RET_VAL(tripPointPtr == nullptr, LE_NOT_FOUND,"Invalid para(null reference ptr)");
    LE_DEBUG("ReleaseTripEventRef : %p", tripEventRef);
    le_ref_DeleteRef(tripPointRefMap, tripEventRef);
    le_mem_Release(tripPointPtr);
    return LE_OK;
}

le_result_t taf_Therm::ReleaseCoolingDeviceRef(taf_therm_CoolingDeviceRef_t cDevRef)
{
    TAF_ERROR_IF_RET_VAL(cDevRef == nullptr, LE_NOT_FOUND, "Invalid para(null reference)");
    taf_CoolingDevice_t* cDevPtr = (taf_CoolingDevice_t*)le_ref_Lookup(cDevRefMap, cDevRef);
    TAF_ERROR_IF_RET_VAL(cDevPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("ReleaseCoolingDeviceRef : %p", cDevRef);
    le_ref_DeleteRef(cDevRefMap, cDevRef);
    le_mem_Release(cDevPtr);
    return LE_OK;
}
