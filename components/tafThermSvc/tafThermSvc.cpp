/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafTherm.hpp"
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <unistd.h>

using namespace telux::tafsvc;
using namespace telux::therm;

COMPONENT_INIT
{
    LE_INFO("tafTherm Service Init...\n");
    auto& tafTherm = taf_Therm::GetInstance();
    tafTherm.Init();
    LE_INFO("tafTherm Service Ready...\n");
}


/*======================================================================

 FUNCTION        taf_therm_GetThermalZonesList

 DESCRIPTION     Get list of all thermal zones.

 DEPENDENCIES    Initialization of thermal Service

 RETURN VALUE    taf_therm_ThermalZoneListRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_ThermalZoneListRef_t taf_therm_GetThermalZonesList()
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZonesList();
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZonesListSize

 DESCRIPTION     Gets thermal zone list size.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneListRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetThermalZonesListSize(
             taf_therm_ThermalZoneListRef_t listRef, uint32_t* listSize)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZonesListSize(listRef, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetFirstThermalZone

 DESCRIPTION     Get the reference of the first thermal zone from a list.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneListRef_t ThermalZoneListRef:The thermal list reference.

 RETURN VALUE    taf_therm_ThermalZoneRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_ThermalZoneRef_t taf_therm_GetFirstThermalZone(taf_therm_ThermalZoneListRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetFirstThermalZone(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetNextThermalZone

 DESCRIPTION     Get the reference of the next thermal zone from a list.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneListRef_t ThermalZoneListRef: The thermal list reference.

 RETURN VALUE    taf_therm_ThermalZoneRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_ThermalZoneRef_t taf_therm_GetNextThermalZone(taf_therm_ThermalZoneListRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetNextThermalZone(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetFirstTripPoint

 DESCRIPTION     Get the reference of the first trip point from thermal zone.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t ThermalZoneRef: The thermal zone reference.

 RETURN VALUE    taf_therm_TripPointRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_TripPointRef_t taf_therm_GetFirstTripPoint(taf_therm_ThermalZoneRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetFirstTripPoint(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetNextTripPoint

 DESCRIPTION     Get the reference of the next trip point from thermal zone.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t ThermalZoneRef: The thermal zone reference.

 RETURN VALUE    taf_therm_TripPointRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_TripPointRef_t taf_therm_GetNextTripPoint(taf_therm_ThermalZoneRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetNextTripPoint(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetFirstBoundCDev

 DESCRIPTION     Get the reference of the first bounded cooling device from thermal zone.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t ThermalZoneRef: The thermal zone reference.

 RETURN VALUE    taf_therm_BoundCoolingDeviceRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_BoundCoolingDeviceRef_t taf_therm_GetFirstBoundCDev(taf_therm_ThermalZoneRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetFirstBoundCDev(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetNextBoundCDev

 DESCRIPTION     Get the reference of the first bounded cooling device from thermal zone.

 DEPENDENCIES    Initialization of a thermal list

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t ThermalZoneRef: The thermal zone reference.

 RETURN VALUE    taf_therm_BoundCoolingDeviceRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_BoundCoolingDeviceRef_t taf_therm_GetNextBoundCDev(taf_therm_ThermalZoneRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetNextBoundCDev(listRef);
}


/*======================================================================

 FUNCTION        taf_therm_DeleteThermalZoneList

 DESCRIPTION     Delete a reference of thermal list.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneListRef_t ThermalZoneListRef:The thermal list reference.

 RETURN VALUE    le_result_t
                     LE_NOT_FOUND: Fail.
                     LE_OK:        Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_DeleteThermalZoneList(taf_therm_ThermalZoneListRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.DeleteThermalZoneList(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetCoolingDeviceList

 DESCRIPTION     Get reference to list of all cooling device.

 DEPENDENCIES    Initialization of thermal Service

 RETURN VALUE    taf_therm_CoolingDeviceListRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_CoolingDeviceListRef_t taf_therm_GetCoolingDeviceList()
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCoolingDeviceList();
}

/*======================================================================

 FUNCTION        taf_therm_GetCoolingDeviceListSize

 DESCRIPTION     Gets cooling device list size.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_CoolingDeviceListRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetCoolingDeviceListSize(
         taf_therm_CoolingDeviceListRef_t listRef, uint32_t* listSize)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCoolingDeviceListSize(listRef, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetFirstCoolingDevice

 DESCRIPTION     Get the reference of the first cooling device.

 DEPENDENCIES    Initialization of a cooling device list

 PARAMETERS      [IN] taf_therm_CoolingDeviceListRef_t CoolingDeviceListRef:
                     The cooling device list reference.

 RETURN VALUE    taf_therm_CoolingDeviceRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_CoolingDeviceRef_t taf_therm_GetFirstCoolingDevice(
         taf_therm_CoolingDeviceListRef_t listRef
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetFirstCoolingDevice(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetNextCoolingDevice

 DESCRIPTION     Get the reference of the next cooling device.

 DEPENDENCIES    Initialization of a cooling device list

 PARAMETERS      [IN] taf_therm_CoolingDeviceListRef_t CoolingDeviceListRef:
                 The cooling device list reference.

 RETURN VALUE    taf_therm_CoolingDeviceRef_t
                     nullptr:     Fail
                     non-nullptr: Success
 SIDE EFFECTS

======================================================================*/
taf_therm_CoolingDeviceRef_t taf_therm_GetNextCoolingDevice(
    taf_therm_CoolingDeviceListRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetNextCoolingDevice(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_DeleteCoolingDeviceList

 DESCRIPTION     Delete a reference of cooling device.

 DEPENDENCIES    Initialization of cooling device.

 PARAMETERS      [IN] taf_therm_CoolingDeviceListRef_t CoolingDeviceListRef:
                 The cooling device reference.

 RETURN VALUE    le_result_t
                     LE_NOT_FOUND: Fail.
                     LE_OK:        Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_DeleteCoolingDeviceList(taf_therm_CoolingDeviceListRef_t listRef)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.DeleteCoolingDeviceList(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZoneByName

 DESCRIPTION     Gets reference for a thermal zone.

 DEPENDENCIES    Initialization of thermal service.

 PARAMETERS      [IN] string thermalZone: The thermal zone name.

 RETURN VALUE    taf_therm_ThermalZoneRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_ThermalZoneRef_t taf_therm_GetThermalZoneByName(const char *thermalZone)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZoneByName(thermalZone);
}

/*======================================================================

 FUNCTION        taf_therm_GetCoolingDeviceByName

 DESCRIPTION     Gets reference for a cooling device.

 DEPENDENCIES    Initialization of thermal service.

 PARAMETERS      [IN] string thermalZone: The cooling device name.

 RETURN VALUE    taf_therm_CoolingDeviceRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_CoolingDeviceRef_t taf_therm_GetCoolingDeviceByName(const char* coolingDevice)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCoolingDeviceByName(coolingDevice);
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZoneID

 DESCRIPTION     Gets information for a thermal zone id.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetThermalZoneID(taf_therm_ThermalZoneRef_t listRef,uint32_t* thermalZoneID)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZoneID(listRef, thermalZoneID);
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZoneType

 DESCRIPTION     Gets rmation for a trip point type.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetThermalZoneType(
    taf_therm_ThermalZoneRef_t listRef, char* thermalZoneType, size_t listSize
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZoneType(listRef, thermalZoneType, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZoneCurrentTemp

 DESCRIPTION     Gets information for a thermal zone current temparature.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetThermalZoneCurrentTemp(
         taf_therm_ThermalZoneRef_t listRef, uint32_t* currTemp)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZoneCurrentTemp(listRef, currTemp);
}

/*======================================================================

 FUNCTION        taf_therm_GetThermalZonePassiveTemp

 DESCRIPTION     Gets information for a thermal zone passive temparature.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetThermalZonePassiveTemp(
         taf_therm_ThermalZoneRef_t listRef, uint32_t* passiveTemp)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetThermalZonePassiveTemp(listRef, passiveTemp);
}

/*======================================================================

 FUNCTION        taf_therm_GetTripPointListSize

 DESCRIPTION     Gets trip point list size.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointListSize(taf_therm_ThermalZoneRef_t listRef, uint32_t* listSize)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetTripPointListSize(listRef, listSize);
}


/*======================================================================

 FUNCTION        taf_therm_GetBoundCoolingDeviceListSize

 DESCRIPTION     Gets bound cooling device list size.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_ThermalZoneRef_t listRef: reference to thermal zone.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundCoolingDeviceListSize(
         taf_therm_ThermalZoneRef_t listRef, uint32_t* listSize)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundCoolingDeviceListSize(listRef, listSize);
}


/*======================================================================

 FUNCTION        taf_therm_GetTripPointType

 DESCRIPTION     Gets information for a trip point type.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointType(
    taf_therm_TripPointRef_t listRef,  char* tripType, size_t listSize
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetTripPointType(listRef, tripType, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetTripPointThreshold

 DESCRIPTION     Gets information for a trip point threshold.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointThreshold(taf_therm_TripPointRef_t listRef, uint32_t* threshold)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetTripPointThreshold(listRef, threshold);
}


/*======================================================================

 FUNCTION        taf_therm_GetTripPointHysterisis

 DESCRIPTION     Gets information for a trip point Hysterisis.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointHysterisis(taf_therm_TripPointRef_t listRef,uint32_t* hysterisis)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetTripPointHysterisis(listRef, hysterisis);
}

/*======================================================================

 FUNCTION        taf_therm_GetTripPointTripID

 DESCRIPTION     Gets information for a trip point trip ID.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointTripID(taf_therm_TripPointRef_t listRef, uint32_t* tripID)
{
    #if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
        auto& tafTherm = taf_Therm::GetInstance();
        return tafTherm.GetTripPointTripID(listRef, tripID);
    #endif
        return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_therm_GetTripPointThermalZoneID

 DESCRIPTION     Gets information of thermal zone ID the trip point is associated with.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetTripPointThermalZoneID(taf_therm_TripPointRef_t listRef,uint32_t* tZoneID)
{
    #if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
        auto& tafTherm = taf_Therm::GetInstance();
        return tafTherm.GetTripPointThermalZoneID(listRef, tZoneID);
    #endif
        return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_therm_GetCDevID

 DESCRIPTION     Gets information for cooling device's cooling id.

 DEPENDENCIES    Initialization of cooling device list.

 PARAMETERS      [IN] taf_therm_CoolingDeviceRef_t listRef: reference to cooling device.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetCDevID(taf_therm_CoolingDeviceRef_t listRef, uint32_t* cDevID)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCDevID(listRef, cDevID);
}

/*======================================================================

 FUNCTION        taf_therm_GetCDevMaxCoolingLevel

 DESCRIPTION     Gets information for a cooling device max cooling level.

 DEPENDENCIES    Initialization of cooling device list.

 PARAMETERS      [IN] taf_therm_CoolingDeviceRef_t listRef: reference to cooling device list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetCDevMaxCoolingLevel(
        taf_therm_CoolingDeviceRef_t listRef, uint32_t* maxCoolingLevel)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCDevMaxCoolingLevel(listRef, maxCoolingLevel);
}

/*======================================================================

 FUNCTION        taf_therm_GetCDevCurrentCoolingLevel

 DESCRIPTION     Gets information for a cooling device current cooling level.

 DEPENDENCIES    Initialization of cooling device list.

 PARAMETERS      [IN] taf_therm_CoolingDeviceRef_t listRef: reference to cooling device list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetCDevCurrentCoolingLevel(
        taf_therm_CoolingDeviceRef_t listRef, uint32_t* currentCoolingLevel)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCDevCurrentCoolingLevel(listRef, currentCoolingLevel);
}


/*======================================================================

 FUNCTION        taf_therm_GetCDevDescription

 DESCRIPTION     Gets information for a cooling device's description.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_CoolingDeviceRef_t listRef:
                 reference to bound cooling device list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetCDevDescription(
        taf_therm_CoolingDeviceRef_t listRef, char* description, size_t listSize
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetCDevDescription(listRef, description, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundCoolingId

 DESCRIPTION     Gets information for a bound cooling device's cooling id.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_BoundCoolingDeviceRef_t listRef:
                 reference to bound cooling device list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundCoolingId(
        taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* boundCoolingId)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundCoolingId(listRef, boundCoolingId);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointListSize

 DESCRIPTION     Gets information for a bound cooling device's trip point list size.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_BoundCoolingDeviceRef_t listRef:
                 reference to bound cooling device list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointListSize(
         taf_therm_BoundCoolingDeviceRef_t listRef, uint32_t* listSize)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointListSize(listRef, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetFirstBoundTripPoint

 DESCRIPTION     Gets reference for first bound cooling device's trip point.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_BoundCoolingDeviceRef_t listRef:
                 reference to bound cooling device list.

 RETURN VALUE    taf_therm_TripPointRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_TripPointRef_t taf_therm_GetFirstBoundTripPoint(
        taf_therm_BoundCoolingDeviceRef_t listRef
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetFirstBoundTripPoint(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetNextBoundTripPoint

 DESCRIPTION     Gets reference for next bound cooling device's trip point.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_BoundCoolingDeviceRef_t listRef:
                 reference to bound cooling device list.

 RETURN VALUE    taf_therm_TripPointRef_t
                     nullptr:     Fail
                     non-nullptr: Success

 SIDE EFFECTS

======================================================================*/
taf_therm_TripPointRef_t taf_therm_GetNextBoundTripPoint(
        taf_therm_BoundCoolingDeviceRef_t listRef
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetNextBoundTripPoint(listRef);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointType

 DESCRIPTION     Gets information for a trip point type.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointType(
        taf_therm_TripPointRef_t listRef, char* boundTripType, size_t listSize
)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointType(listRef, boundTripType, listSize);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointThreshold

 DESCRIPTION     Gets information for bound cooling device's trip point threshold.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointThreshold(
        taf_therm_TripPointRef_t listRef, uint32_t* boundThreshold)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointThreshold(listRef, boundThreshold);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointHysterisis

 DESCRIPTION     Gets information for bound cooling device's trip point hysterisis.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point list.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointHysterisis(
        taf_therm_TripPointRef_t listRef, uint32_t* boundHysterisis)
{
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointHysterisis(listRef, boundHysterisis);
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointTripID

 DESCRIPTION     Gets information for a trip point trip ID.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointTripID(
         taf_therm_TripPointRef_t listRef, uint32_t* boundTripID)
{
#if LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointTripID(listRef, boundTripID);
#endif
    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_therm_GetBoundTripPointThermalZoneID

 DESCRIPTION     Gets information of thermal zone ID the trip point is associated with.

 DEPENDENCIES    Initialization of thermal list.

 PARAMETERS      [IN] taf_therm_TripPointRef_t listRef: reference to trip point.

 RETURN VALUES   LE_OK on success, LE_FAULT on error

 SIDE EFFECTS

======================================================================*/
le_result_t taf_therm_GetBoundTripPointThermalZoneID(
        taf_therm_TripPointRef_t listRef, uint32_t* boundTZoneID)
{
#if LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.GetBoundTripPointThermalZoneID(listRef, boundTZoneID);
#endif
    return LE_FAULT;
}

/**
* FUNCTION     : taf_therm_AddTripEventHandler
* DESCRIPTION  : Sends trip point tripped notification
* DEPENDECY    :
* PARAMETERS   : Trip Event Handler Function
* RETURN VALUES: HandlerRef if registered successfully or else NULL
*/
taf_therm_TripEventHandlerRef_t taf_therm_AddTripEventHandler(
         taf_therm_TripEventHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_DEBUG("AddStateChangeHandler for trip type change");
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.AddTripEventHandler(handlerPtr,contextPtr);
}

/**
* FUNCTION     : taf_therm_RemoveTripEventHandler
* DESCRIPTION  : Remove trip event handler
* DEPENDECY    :
* PARAMETERS   : trip event handler reference
* RETURN VALUES:
*/
void taf_therm_RemoveTripEventHandler(taf_therm_TripEventHandlerRef_t handlerRef)
{
    LE_DEBUG("taf_therm_RemoveStateChangeHandler");
    auto& tafTherm = taf_Therm::GetInstance();
    tafTherm.RemoveTripEventHandler(handlerRef);
}

/**
* FUNCTION     : taf_therm_AddCoolingLevelChangeEventHandler
* DESCRIPTION  : Send cooling level change notification
* DEPENDECY    :
* PARAMETERS   : Cooling level change handler reference
* RETURN VALUES: HandlerRef if registered successfully or else NULL
*/
taf_therm_CoolingLevelChangeEventHandlerRef_t taf_therm_AddCoolingLevelChangeEventHandler
(
   taf_therm_CoolingLevelChangeEventHandlerFunc_t handlerPtr, void* contextPtr
)
{
    LE_DEBUG("AddStateChangeHandler for trip type change");
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.AddCoolingLevelChangeEventHandler(handlerPtr,contextPtr);
}

/**
* FUNCTION     : taf_therm_RemoveCoolingLevelChangeEventHandler
* DESCRIPTION  : Remove cooling level change handler
* DEPENDECY    :
* PARAMETERS   : Cooling level change handler reference
* RETURN VALUES:
*/

void taf_therm_RemoveCoolingLevelChangeEventHandler
(
    taf_therm_CoolingLevelChangeEventHandlerRef_t handlerRef
)
{
    LE_DEBUG("taf_therm_RemoveStateChangeHandler");
    auto& tafTherm = taf_Therm::GetInstance();
    tafTherm.RemoveCoolingLevelChangeEventHandler(handlerRef);
}

/**
* FUNCTION     : taf_therm_ReleaseTripEventRef
* DESCRIPTION  : Remove trip event change reference for the given trip point
* DEPENDECY    :
* PARAMETERS   : Trip Point reference
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid reference
*/

le_result_t taf_therm_ReleaseTripEventRef(taf_therm_TripPointRef_t tripEventRef) {
    LE_DEBUG("taf_therm_ReleaseTripEventRef");
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.ReleaseTripEventRef(tripEventRef);
}


/**
* FUNCTION     : taf_therm_ReleaseCoolingDeviceRef
* DESCRIPTION  : Remove cooling device reference for the given cooling device
* DEPENDECY    :
* PARAMETERS   : Cooling device reference
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid reference
*/

le_result_t taf_therm_ReleaseCoolingDeviceRef(taf_therm_CoolingDeviceRef_t cDevRef) {
    LE_DEBUG("taf_therm_ReleaseCoolingDeviceRef");
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.ReleaseCoolingDeviceRef(cDevRef);
}


/**
* FUNCTION     : taf_therm_ReleaseThermalZoneRef
* DESCRIPTION  : Remove thermal zone reference
* DEPENDECY    :
* PARAMETERS   : Thermal zone reference
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on invalid reference
*/

le_result_t taf_therm_ReleaseThermalZoneRef(taf_therm_ThermalZoneRef_t thermalZoneRef) {
    LE_DEBUG("taf_therm_ReleaseThermalZoneRef");
    auto& tafTherm = taf_Therm::GetInstance();
    return tafTherm.ReleaseThermalZoneRef(thermalZoneRef);
}