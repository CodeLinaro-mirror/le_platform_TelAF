/*
* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <iostream>
#include <vector>
#include <set>

#define TYPE_SIZE 32
std::set<std::string> CDevNameList;
std::set<std::string> ThermalZoneNameList;

void TestZoneInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    uint32_t tZoneid;
    result = taf_therm_GetThermalZoneID(tZone, &tZoneid);
    LE_TEST_OK(result == LE_OK, "Thermal ZoneID : %d", tZoneid);

    char thermalZoneType[TYPE_SIZE];
    memset(thermalZoneType, 0, TYPE_SIZE);
    result = taf_therm_GetThermalZoneType(tZone, thermalZoneType, sizeof(thermalZoneType));
    LE_TEST_OK(result == LE_OK, "Thermal ZoneType : %s", thermalZoneType);

    ThermalZoneNameList.insert(std::string(thermalZoneType));

    uint32_t currTemp;
    result = taf_therm_GetThermalZoneCurrentTemp(tZone, &currTemp);
    LE_TEST_OK((result == LE_OK),"Thermal Zone CurrentTemp : %d", currTemp);

    uint32_t passiveTemp;
    result = taf_therm_GetThermalZonePassiveTemp(tZone, &passiveTemp);
    LE_TEST_OK((result == LE_OK),"Thermal Zone PassiveTemp : %d", passiveTemp);
}

void TestTripPointInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF First Trip Point Reference Retrieval with -"
            "taf_therm_GetFirstTripPoint");
    taf_therm_TripPointRef_t tripPoint = taf_therm_GetFirstTripPoint(tZone);
    LE_TEST_OK((tripPoint != NULL), "taf_therm_GetFirstTripPoint - LE_OK");

    uint32_t listSize;
    result = taf_therm_GetTripPointListSize(tZone, &listSize);
    LE_TEST_OK(result == LE_OK, "TripPoint ListSize : %d", listSize);

    if(result != LE_OK || listSize <= 0)
    {
        LE_ERROR("No trip points are associated with the thermal zone");
        return;
    }

    while (tripPoint != NULL and listSize--)
    {
        uint32_t tripID;
        result = taf_therm_GetTripPointTripID(tripPoint, &tripID);
        LE_TEST_OK(result == LE_OK, "TripPoint TripID : %d", tripID);

        uint32_t tZoneID;
        result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
        LE_TEST_OK(result == LE_OK, "TripPoint ThermalZone ID : %d", tZoneID);

        char tripType[TYPE_SIZE];
        memset(tripType, 0, TYPE_SIZE);
        result = taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
        LE_TEST_OK(result == LE_OK, "TripPoint Type : %s",tripType);

        uint32_t threshold;
        result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
        LE_TEST_OK(result == LE_OK, "TripPoint Threshold : %d",
                threshold);

        uint32_t hysterisis;
        result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
        LE_TEST_OK(result == LE_OK, "TripPoint Hysterisis : %d",
                hysterisis);
        if (listSize > 0)
        {
            LE_TEST_INFO("Testing TelAF Next Trip Point Reference Retrieval with -"
                    "taf_therm_GetNextTripPoint");
            tripPoint = taf_therm_GetNextTripPoint(tZone);
            LE_TEST_OK((tripPoint != NULL), "taf_therm_GetNextTripPoint - LE_OK");
        }
    }
}

void TestBoundCoolingDevicesInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF First Bound Cooling Device Reference Retrieval with -"
            "taf_therm_GetFirstBoundCDev");
    taf_therm_BoundCoolingDeviceRef_t boundCDev = taf_therm_GetFirstBoundCDev(tZone);
    LE_TEST_OK((boundCDev != NULL), "taf_therm_GetFirstBoundCDev - LE_OK");
    uint32_t listSize;
    result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &listSize);
    LE_TEST_OK(result == LE_OK, "BoundCoolingDevice ListSize : %d",listSize);

    if(result != LE_OK || listSize <= 0)
    {
        LE_ERROR("No bound cooling devices available");
        return;
    }

    while (boundCDev != NULL and listSize--)
    {
        uint32_t coolingID;
        result = taf_therm_GetBoundCoolingId(boundCDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "BoundCooling Id : %d",coolingID);
        uint32_t tripPointListSize;
        result = taf_therm_GetBoundTripPointListSize(boundCDev, &tripPointListSize);
        LE_TEST_OK(result == LE_OK, "BoundTripPoint ListSize : %d", tripPointListSize);
        if(result != LE_OK || tripPointListSize <= 0)
        {
            //Do not return if no trip point is bounded to Cdev here
            // as there are other bounded cooling devices to go through
            LE_ERROR("No trip points are associated with the bounded cooling devices available");
        }
        LE_TEST_INFO("Testing TelAF First Bound Cooling Device's Trip Point Reference"
                "Retrieval with - taf_therm_GetFirstBoundTripPoint");
        taf_therm_TripPointRef_t boundTripPoint = taf_therm_GetFirstBoundTripPoint(boundCDev);
        LE_TEST_OK((boundTripPoint != NULL), "taf_therm_GetFirstBoundTripPoint - LE_OK");

        while (boundTripPoint != NULL and tripPointListSize--)
        {
            uint32_t boundTripID;
            result = taf_therm_GetBoundTripPointTripID(boundTripPoint, &boundTripID);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint TripID : %d", boundTripID);

            uint32_t boundTZoneID;
            result = taf_therm_GetBoundTripPointThermalZoneID(boundTripPoint, &boundTZoneID);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint ThermalZoneID : %d", boundTZoneID);

            char boundTripType[TYPE_SIZE];
            memset(boundTripType, 0, TYPE_SIZE);
            result = taf_therm_GetBoundTripPointType(boundTripPoint, boundTripType, sizeof(boundTripType));
            LE_TEST_OK(result == LE_OK, "BoundTripPoint Type : %s", boundTripType);

            uint32_t boundThreshold;
            result = taf_therm_GetBoundTripPointThreshold(boundTripPoint, &boundThreshold);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint Threshold : %d", boundThreshold);

            uint32_t boundHysterisis;
            result = taf_therm_GetBoundTripPointHysterisis(boundTripPoint, &boundHysterisis);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint Hysterisis : %d", boundHysterisis);

            if (tripPointListSize > 0)
            {
                LE_TEST_INFO("Testing TelAF Next Bound Cooling Device's Trip Point Reference"
                        "Retrieval with - taf_therm_GetNextBoundTripPoint");
                boundTripPoint = taf_therm_GetNextBoundTripPoint(boundCDev);
                LE_TEST_OK(boundTripPoint != NULL, "taf_therm_GetNextBoundTripPoint - LE_OK");
            }
        }
        if (listSize > 0)
        {
            LE_TEST_INFO("Testing TelAF Next Bound Cooling Device Reference Retrieval with-"
                    "taf_therm_GetNextBoundCDev");
            boundCDev = taf_therm_GetNextBoundCDev(tZone);
            LE_TEST_OK((boundCDev != NULL), "taf_therm_GetNextBoundCDev - LE_OK");
        }
    }
}

void TestThermalZoneInformation(void)
{
    le_result_t result;
    LE_TEST_INFO("Testing TelAF Thermal Zone List Retrieval with -taf_therm_GetThermalZonesList");
    taf_therm_ThermalZoneListRef_t tZoneListRef = taf_therm_GetThermalZonesList();
    LE_TEST_OK((tZoneListRef != NULL), "taf_therm_GetThermalZonesList - LE_OK");

    if (tZoneListRef == NULL)
    {
        LE_ERROR("Not able to get reference to thermal zone list");
        return;
    }

    taf_therm_ThermalZoneListRef_t headTZoneListRef = tZoneListRef;

    LE_TEST_INFO("Testing TelAF First Thermal Zone Reference Retrieval with -"
            "taf_therm_GetFirstThermalZone");
    taf_therm_ThermalZoneRef_t tZone = taf_therm_GetFirstThermalZone(tZoneListRef);
    LE_TEST_OK((tZone != NULL), "taf_therm_GetFirstThermalZone - LE_OK");
    uint32_t thermalZoneListSize;
    result = taf_therm_GetThermalZonesListSize(tZoneListRef, &thermalZoneListSize);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetThermalZonesListSize - LE_OK");
    LE_INFO("Thermal zone list size: %d", thermalZoneListSize);
    if(result != LE_OK || thermalZoneListSize <= 0)
    {
        LE_ERROR("No thermal zones present");
        return;
    }

    while (tZone != NULL and thermalZoneListSize--)
    {
        TestZoneInformation(tZone);
        uint32_t tripPointListSize;
        result = taf_therm_GetTripPointListSize(tZone, &tripPointListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
        if(result != LE_OK || tripPointListSize <= 0)
        {
            LE_ERROR("No trip points are associated with the thermal zone");
        }
        else if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(tZone);
        }
        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &boundCDevListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundCoolingDeviceListSize - LE_OK");
        if(result != LE_OK || boundCDevListSize <= 0)
        {
            LE_ERROR("No cooling devices bounded with thermal zone");
        }
        else if (boundCDevListSize > 0)
        {
            TestBoundCoolingDevicesInformation(tZone);
        }

        if (thermalZoneListSize > 0)
        {
            LE_TEST_INFO("Testing TelAF Next Thermal Zone Reference Retrieval with -"
                    "taf_therm_GetNextThermalZone");
            tZone = taf_therm_GetNextThermalZone(tZoneListRef);
            LE_TEST_OK((tZone != NULL), "taf_therm_GetNextThermalZone - LE_OK");
        }
    }
    LE_TEST_INFO("Testing TelAF deleting thermal zone list with -taf_therm_DeleteThermalZoneList");
    result = taf_therm_DeleteThermalZoneList(headTZoneListRef);
    LE_TEST_OK(result == LE_OK, "taf_therm_DeleteThermalZoneList - LE_OK");
    LE_INFO("===== UnitTest Completed for retrieving information about thermal zones =====");
}

void TestCoolingDeviceInformation(void)
{
    le_result_t result;
    LE_TEST_INFO("===== All Cooling Device Information =====");
    LE_TEST_INFO("Testing TelAF Cooling Device List Retrieval with-"
            "taf_therm_GetCoolingDeviceList");
    taf_therm_CoolingDeviceListRef_t cDevListRef = taf_therm_GetCoolingDeviceList();
    //Always will have cooling devices
    LE_TEST_OK((cDevListRef != NULL), "taf_therm_GetCoolingDeviceList - LE_OK");

    taf_therm_CoolingDeviceListRef_t headCDevListRef = cDevListRef;

    LE_TEST_INFO("Testing TelAF First Cooling Device Reference Retrieval with -"
            "taf_therm_GetFirstCoolingDevice");
    taf_therm_CoolingDeviceRef_t cDev = taf_therm_GetFirstCoolingDevice(cDevListRef);
    LE_TEST_OK((cDev != NULL), "taf_therm_GetFirstCoolingDevice - LE_OK");
    uint32_t coolingDeviceListSize;
    result = taf_therm_GetCoolingDeviceListSize(cDevListRef, &coolingDeviceListSize);
    LE_TEST_OK((result == LE_OK), "taf_therm_GetCoolingDeviceListSize - LE_OK");
    LE_INFO("Cooling device list size: %d", coolingDeviceListSize);
    if(result != LE_OK || coolingDeviceListSize <= 0)
    {
        LE_ERROR("No cooling device present");
        return;
    }
    while (cDev != NULL and coolingDeviceListSize--)
    {
        char description[TYPE_SIZE] = {0};
        memset(description, 0, TYPE_SIZE);
        result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
        LE_TEST_OK(result == LE_OK, "CDev Description : %s", description);

        CDevNameList.insert(std::string(description));

        uint32_t coolingID;
        result = taf_therm_GetCDevID(cDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "CDev ID : %d", coolingID);

        uint32_t maxCooling;
        result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
        LE_TEST_OK(result == LE_OK, "CDevMax CoolingLevel : %d",maxCooling);

        uint32_t currCooling;
        result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
        LE_TEST_OK(result == LE_OK, "CDev CurrentCoolingLevel : %d", currCooling);

        if (coolingDeviceListSize > 0) {
            LE_TEST_INFO("Testing TelAF Next cooling device Reference Retrieval with -"
                    "taf_therm_GetNextCoolingDevice");
            cDev = taf_therm_GetNextCoolingDevice(cDevListRef);
            LE_TEST_OK((cDev != NULL), "taf_therm_GetNextCoolingDevice - LE_OK");
        }
    }

    LE_TEST_INFO("Testing TelAF deleting cooling device list with -"
            "taf_therm_DeleteCoolingDeviceList");
    result = taf_therm_DeleteCoolingDeviceList(headCDevListRef);
    LE_TEST_OK((result == LE_OK), "taf_therm_DeleteCoolingDeviceList - LE_OK");

    LE_INFO("===== UnitTest Completed for retrieving cooling device information =====");
}

void TestThermalZoneByName(void)
{
    LE_TEST_INFO("===== Get Thermal Zone By Name =====");
    le_result_t result;
    taf_therm_ThermalZoneRef_t thermalZone;

    for (auto thermalZoneName: ThermalZoneNameList)
    {
        LE_TEST_INFO("Testing TelAF getting thermal zone by name with -"
                "taf_therm_GetThermalZoneByName");
        LE_INFO("Testing thermal zone by name : %s", thermalZoneName.c_str());
        thermalZone =taf_therm_GetThermalZoneByName(thermalZoneName.c_str());
        LE_TEST_OK((thermalZone != NULL), "taf_therm_GetThermalZoneByName - LE_OK");

        if (thermalZone != NULL)
        {
            TestZoneInformation(thermalZone);
            uint32_t tripPointListSize;
            result = taf_therm_GetTripPointListSize(thermalZone, &tripPointListSize);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
            if(result != LE_OK || tripPointListSize <= 0)
            {
                LE_ERROR("No trip points are associated with the thermal zone");
            }
            else if (tripPointListSize > 0)
            {
                LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
                TestTripPointInformation(thermalZone);
            }
            uint32_t boundCDevListSize;
            result = taf_therm_GetBoundCoolingDeviceListSize(thermalZone, &boundCDevListSize);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundCoolingDeviceListSize - LE_OK.");
            if(result != LE_OK || boundCDevListSize <= 0)
            {
                LE_ERROR("No cooling devices bounded with thermal zone");
            }
            else if (boundCDevListSize > 0)
            {
                TestBoundCoolingDevicesInformation(thermalZone);
            }
        }
        taf_therm_ReleaseThermalZoneRef(thermalZone);
    }
    LE_TEST_INFO("Testing for unavailable thermal zone");
    thermalZone =
        taf_therm_GetThermalZoneByName("UnavailableThermalZone");
    LE_TEST_OK((thermalZone == NULL), "Negative assertion taf_therm_GetThermalZoneByName - LE_OK");

    if (thermalZone != NULL)
    {
        TestZoneInformation(thermalZone);
        uint32_t tripPointListSize;
        result = taf_therm_GetTripPointListSize(thermalZone, &tripPointListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
        if(result != LE_OK || tripPointListSize <= 0)
        {
            LE_ERROR("No trip points are associated with the thermal zone");
        }
        else if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(thermalZone);
        }
        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(thermalZone, &boundCDevListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundCoolingDeviceListSize - LE_OK.");
        if(result != LE_OK || boundCDevListSize <= 0)
        {
            LE_ERROR("No cooling devices bounded with thermal zone");
        }
        else if (boundCDevListSize > 0)
        {
            TestBoundCoolingDevicesInformation(thermalZone);
        }
    }
    taf_therm_ReleaseThermalZoneRef(thermalZone);
    LE_INFO("===== UnitTest Completed for getting thermal zone by name =====");
}

void TestCoolingDeviceByName()
{
    LE_TEST_INFO("===== Get Cooling Device By Name =====");
    le_result_t result;
    taf_therm_CoolingDeviceRef_t cDev;

    for (auto cDevName: CDevNameList)
    {
        LE_TEST_INFO("Testing TelAF getting cooling device by name with -"
                "taf_therm_GetCoolingDeviceByName");
        LE_INFO("Testing Cooling Device by name for : %s", cDevName.c_str());
        cDev = taf_therm_GetCoolingDeviceByName(cDevName.c_str());
        LE_TEST_OK((cDev != NULL), "taf_therm_GetCoolingDeviceByName - LE_OK");

        if (cDev != NULL)
        {
            char description[TYPE_SIZE];
            memset(description, 0, TYPE_SIZE);
            result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
            LE_TEST_OK(result == LE_OK, "CDev Description : %s", description);

            uint32_t coolingID;
            result = taf_therm_GetCDevID(cDev, &coolingID);
            LE_TEST_OK(result == LE_OK, "CDev ID : %d", coolingID);

            uint32_t maxCooling;
            result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
            LE_TEST_OK(result == LE_OK, "CDev MaxCoolingLevel : %d", maxCooling);

            uint32_t currCooling;
            result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
            LE_TEST_OK(result == LE_OK, "CDev CurrentCoolingLevel : %d", currCooling);
        }
        taf_therm_ReleaseCoolingDeviceRef(cDev);
    }
    LE_TEST_INFO("Testing for unavailable cooling device");
    cDev = taf_therm_GetCoolingDeviceByName("UnavailableCoolingDev");
    LE_TEST_OK((cDev == NULL), "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
    LE_TEST_OK((cDev == nullptr), "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
    LE_TEST_OK(!cDev, "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
    taf_therm_ReleaseCoolingDeviceRef(cDev);
    LE_INFO("===== UnitTest Completed for getting cooling device by name =====");
}

const char* TripEventToString(taf_therm_TripEventType_t state)
{
    const char* tripEvent;

    switch (state)
    {
    case TAF_THERM_CROSSED_UNDER:
        tripEvent = "CROSSED_UNDER";
        break;
    case TAF_THERM_CROSSED_OVER:
        tripEvent = "CROSSED_OVER";
        break;
    default:
        tripEvent = "NONE";
        break;
    }
    return tripEvent;
}

static void TestTripEventHandler
(
    taf_therm_TripPointRef_t tripPoint,
    taf_therm_TripEventType_t type,
    void* contextPtr
)
{
    le_result_t result;
    uint32_t tripID;
    result = taf_therm_GetTripPointTripID(tripPoint, &tripID);
    LE_TEST_OK(result == LE_OK, "TripPoint TripID : %d", tripID);
    uint32_t tZoneID;
    result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
    LE_TEST_OK(result == LE_OK, "TripPoint ThermalZoneID : %d", tZoneID);
    char tripType[TYPE_SIZE];
    memset(tripType, 0, TYPE_SIZE);
    result = taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
    LE_TEST_OK(result == LE_OK, "TripPoint Type : %s", tripType);

    uint32_t threshold;
    result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
    LE_TEST_OK(result == LE_OK, "TripPoint Threshold : %d", threshold);

    uint32_t hysterisis;
    result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
    LE_TEST_OK(result == LE_OK, "TripPoint Hysterisis : %d", hysterisis);
    LE_INFO("TestTripEventHandler -- Trip type is %s\n\n", TripEventToString(type));
    taf_therm_ReleaseTripEventRef(tripPoint);
}

void TestCoolingLevelChangeHandler
(
    taf_therm_CoolingDeviceRef_t cDev,
    void* contextPtr
)
{
    le_result_t result;
    char description[TYPE_SIZE];
    memset(description, 0, TYPE_SIZE);
    result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
    LE_TEST_OK(result == LE_OK, "CDev Description : %s", description);
    uint32_t coolingID;
    result = taf_therm_GetCDevID(cDev, &coolingID);
    LE_TEST_OK(result == LE_OK, "CDev ID : %d", coolingID);
    uint32_t maxCooling;
    result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
    LE_TEST_OK(result == LE_OK, "CDev MaxCoolingLevel : %d", maxCooling);

    uint32_t currCooling;
    result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
    LE_TEST_OK(result == LE_OK, "CDev CurrentCoolingLevel : %d", currCooling);

    taf_therm_ReleaseCoolingDeviceRef(cDev);
}



void TestAddEventHandler()
{
    taf_therm_TripEventHandlerRef_t handlerRef;
    taf_therm_CoolingLevelChangeEventHandlerRef_t cooolingLevelChangeHandlerRef;

    LE_TEST_INFO("Testing Test_taf_therm_AddTripEventHandler on invalid handler reference");
    handlerRef = taf_therm_AddTripEventHandler(NULL,NULL);
    LE_TEST_OK(handlerRef != NULL, "Register AddTripEventHandler handler with INVALID reference");
    LE_TEST_INFO("Testing taf_therm_RemoveTripEventHandler handler reference");

    taf_therm_RemoveTripEventHandler(handlerRef);
    LE_TEST_OK(true, "taf_therm_RemoveTripEventHandler successfull");
    LE_TEST_INFO("Testing Test_taf_therm_AddTripEventHandler on valid handler reference");

    handlerRef = taf_therm_AddTripEventHandler(TestTripEventHandler, NULL);
    LE_TEST_OK(handlerRef != NULL, "Register AddTripEventHandler change handler is successfull");
    LE_TEST_INFO("Testing taf_therm_AddCoolingLevelChangeEventHandler on invalid handler ref");

    cooolingLevelChangeHandlerRef = taf_therm_AddCoolingLevelChangeEventHandler(NULL, NULL);
    LE_TEST_OK(cooolingLevelChangeHandlerRef != NULL,
            "Register cooolingLevelChange handler with INVALID reference");
    LE_TEST_INFO("Testing RemoveCoolingLevelChangeEventHandler handler reference");

    taf_therm_RemoveCoolingLevelChangeEventHandler(cooolingLevelChangeHandlerRef);
    LE_TEST_OK(true, "RemoveCoolingLevelChangeEventHandler successfull");
    LE_TEST_INFO("Testing taf_therm_AddCoolingLevelChangeEventHandler on valid handler reference");

    cooolingLevelChangeHandlerRef =
            taf_therm_AddCoolingLevelChangeEventHandler(TestCoolingLevelChangeHandler, NULL);
    LE_TEST_OK(cooolingLevelChangeHandlerRef != NULL,
            "Register cooolingLevelChange handler is successfull");
}

COMPONENT_INIT
{
    CDevNameList.clear();
    ThermalZoneNameList.clear();

    TestThermalZoneInformation();
    TestCoolingDeviceInformation();
    TestThermalZoneByName();
    TestCoolingDeviceByName();
    TestAddEventHandler();

    LE_INFO("TelAF Thermal Unit Test App Completed");

    exit(EXIT_SUCCESS);
}
