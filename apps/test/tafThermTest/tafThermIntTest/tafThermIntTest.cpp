/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <iostream>
#include <vector>

#define TYPE_SIZE 32
taf_therm_TripEventHandlerRef_t  tripEventHandlerRef;
taf_therm_CoolingLevelChangeEventHandlerRef_t cooolingLevelChangeHandlerRef;


void ThermalPrintHelpMenu()
{
    puts(
        "NAME:\n"
        "app runProc tafThermIntTest tafThermIntTest - Thermal Service Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafThermIntTest tafThermIntTest -- help\n"
        "    app runProc tafThermIntTest tafThermIntTest -- GetZoneAndCoolingDevInfo\n"
        "    app runProc tafThermIntTest tafThermIntTest -- AllThermalZones\n"
        "    app runProc tafThermIntTest tafThermIntTest -- AllCoolingDevices\n"
        "    app runProc tafThermIntTest tafThermIntTest -- ThermalZoneInfo thermalZoneName\n"
        "    app runProc tafThermIntTest tafThermIntTest -- CDevInfo cDevName\n"
        "    app runProc tafThermIntTest tafThermIntTest -- TripEventHandler 500\n"
        "    app runProc tafThermIntTest tafThermIntTest -- CoolingLevelChangeEventHandler 500\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafThermIntTest tafThermIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- AllThermalZones\n"
        "       Gets information about all thermal zones available on the device\n"
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- AllCoolingDevices\n"
        "       Gets information about all cooling devices available on the device\n"
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- GetZoneAndCoolingDevInfo\n"
        "       Print all thermal zone type and its binding cooling device."
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- ThermalZoneInfo thermalZoneName\n"
        "       Gets information about thermal zone 'thermalZoneName'\n"
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- CDevInfo cDevName\n"
        "       Gets information about cooling device 'cDevName'"
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- TripEventHandler 'WaitSecs'\n"
        "       Monitor the trip event for 'WaitSecs', will receive a notification "
        "       when the temparture of a thermal zone crosses the threshold and trip point trips."
        "\n"
        "    app runProc tafThermIntTest tafThermIntTest -- CoolingLevelChangeEventHandler WaitSecs\n"
        "       Monitor the cooling level change event for 'WaitSecs', will receive a notification"
        "       when the cooling level of a cooling device changes."
        "\n"
    );

    exit(EXIT_SUCCESS);
}

void ThermalCheckArgs(uint8_t argNum)
{
    if (le_arg_NumArgs() < argNum)
    {
        ThermalPrintHelpMenu();
    }
}

void TestTripPointInformation(taf_therm_ThermalZoneRef_t tZone, uint32_t listSize)
{
    le_result_t result;
    LE_TEST_INFO("Retrieving first trip point");
    taf_therm_TripPointRef_t tripPoint = taf_therm_GetFirstTripPoint(tZone);
    LE_TEST_OK((tripPoint != NULL), "taf_therm_GetFirstTripPoint - LE_OK");

    if (listSize <= 0)
    {
        LE_ERROR("No trip points are associated with the thermal zone");
        return;
    }
    while (tripPoint != NULL and listSize--)
    {
        uint32_t tripID;
        result = taf_therm_GetTripPointTripID(tripPoint, &tripID);
        LE_TEST_OK(result == LE_OK, "Trip Point ID: %d", tripID);

        uint32_t tZoneID;
        result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
        LE_TEST_OK(result == LE_OK, "Trip Point Thermal Zone ID: %d", tZoneID);

        char tripType[TYPE_SIZE];
        memset(tripType, 0, TYPE_SIZE);
        result = taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
        LE_TEST_OK(result == LE_OK, "Trip Point Type %s",tripType);

        int32_t threshold;
        result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
        LE_TEST_OK(result == LE_OK, "Trip Point threshold %d", threshold);

        int32_t hysterisis;
        result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
        LE_TEST_OK(result == LE_OK, "Trip Point hysterisis %d", hysterisis);
        if (listSize > 0)
        {
            LE_TEST_INFO("Retrieving next trip point");
            tripPoint = taf_therm_GetNextTripPoint(tZone);
            LE_TEST_OK((tripPoint != NULL), "taf_therm_GetNextTripPoint - LE_OK");
        }
    }
}

void TestBoundCoolingDevicesInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    LE_TEST_INFO("Retrieving first Bounded Cooling device");
    taf_therm_BoundCoolingDeviceRef_t boundCDev = taf_therm_GetFirstBoundCDev(tZone);
    LE_TEST_OK((boundCDev != NULL), "taf_therm_GetFirstBoundCDev - LE_OK");
    uint32_t listSize;
    result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &listSize);
    LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
        "taf_therm_GetBoundCoolingDeviceListSize - LE_OK, ListSize: %d", listSize);
    if(result != LE_OK || listSize <= 0)
    {
        LE_ERROR("No Bound CoolingDevices are associated with the thermal zone");
        return;
    }

    while (boundCDev != NULL and listSize--)
    {
        uint32_t coolingID;
        result = taf_therm_GetBoundCoolingId(boundCDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "Bound cooling ID: %d",coolingID);

        uint32_t tripPointListSize;
        result = taf_therm_GetBoundTripPointListSize(boundCDev, &tripPointListSize);
        LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
            "taf_therm_GetBoundTripPointListSize - LE_OK, ListSize: %d", tripPointListSize);
        if(result != LE_OK || tripPointListSize <= 0)
        {
            //Do not return if no trip point is bounded to Cdev here
            // as there are other bounded cooling devices to go through
            LE_ERROR("No Bound TripPoints are associated with the thermal zone");
        }

        LE_TEST_INFO("Retrieving First Bound Cooling Device's Trip Point Reference");
        taf_therm_TripPointRef_t boundTripPoint = taf_therm_GetFirstBoundTripPoint(boundCDev);
        LE_TEST_OK((boundTripPoint != NULL), "taf_therm_GetFirstBoundTripPoint - LE_OK");

        while (boundTripPoint != NULL and tripPointListSize--)
        {
            uint32_t boundTripID;
            result = taf_therm_GetBoundTripPointTripID(boundTripPoint, &boundTripID);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint ID: %d", boundTripID);

            uint32_t boundTZoneID;
            result = taf_therm_GetBoundTripPointThermalZoneID(boundTripPoint, &boundTZoneID);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint Thermal Zone ID: %d", boundTZoneID);

            char boundTripType[TYPE_SIZE];
            memset(boundTripType, 0, TYPE_SIZE);
            result = taf_therm_GetBoundTripPointType(boundTripPoint, boundTripType,
                sizeof(boundTripType));
            LE_TEST_OK(result == LE_OK, "BoundTripPoint Type: %s", boundTripType);

            int32_t boundThreshold;
            result = taf_therm_GetBoundTripPointThreshold(boundTripPoint, &boundThreshold);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint threshold: %d", boundThreshold);

            int32_t boundHysterisis;
            result = taf_therm_GetBoundTripPointHysterisis(boundTripPoint, &boundHysterisis);
            LE_TEST_OK(result == LE_OK, "BoundTripPoint hysterisis: %d", boundHysterisis);

            if (tripPointListSize > 0)
            {
                LE_TEST_INFO("Retrieving Next Bound Cooling Device's Trip Point Reference");
                boundTripPoint = taf_therm_GetNextBoundTripPoint(boundCDev);
                LE_TEST_OK(boundTripPoint != NULL, "taf_therm_GetNextBoundTripPoint - LE_OK");
            }
        }
        if (listSize > 0)
        {
            LE_TEST_INFO("Retrieval Next Bound Cooling Device Reference Retrieval with-");
            boundCDev = taf_therm_GetNextBoundCDev(tZone);
            LE_TEST_OK((boundCDev != NULL), "taf_therm_GetNextBoundCDev - LE_OK");
        }
    }
}

void TestZoneInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    uint32_t tZoneid;
    result = taf_therm_GetThermalZoneID(tZone, &tZoneid);
    LE_TEST_OK(result == LE_OK, "Thermal Zone ID: %d", tZoneid);

    char thermalZoneType[TYPE_SIZE];
    memset(thermalZoneType, 0, TYPE_SIZE);
    result = taf_therm_GetThermalZoneType(tZone, thermalZoneType, sizeof(thermalZoneType));
    LE_TEST_OK(result == LE_OK, "Thermal Zone Type: %s", thermalZoneType);

    int32_t currTemp;
    result = taf_therm_GetThermalZoneCurrentTemp(tZone, &currTemp);
    LE_TEST_OK((result == LE_OK),"Thermal Zone Current Temp: %d", currTemp);

    LE_TEST_INFO("Test Thermal Zone Passive Temp Retrieval");
    int32_t passiveTemp;
    result = taf_therm_GetThermalZonePassiveTemp(tZone, &passiveTemp);
    LE_TEST_OK((result == LE_OK),"Thermal Zone Passive Temp: %d", passiveTemp);
}


void ThermalZoneInfoTest(void)
{

    ThermalCheckArgs(2);
    const char* thermalZoneName = le_arg_GetArg(1);
    if (thermalZoneName == NULL)
    {
        LE_ERROR("thermalZoneName is NULL.");
        return;
    }

    LE_TEST_INFO("===== Get Thermal Zone By Name =====");
    le_result_t result;
    taf_therm_ThermalZoneRef_t thermalZone;

    thermalZone =taf_therm_GetThermalZoneByName(thermalZoneName);
    LE_TEST_OK((thermalZone != NULL), "taf_therm_GetThermalZoneByName - LE_OK");

    if (thermalZone != NULL)
    {
        TestZoneInformation(thermalZone);
        uint32_t tripPointListSize;
        result = taf_therm_GetTripPointListSize(thermalZone, &tripPointListSize);
        LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
            "taf_therm_GetTripPointListSize - LE_OK");
        if(result != LE_OK || tripPointListSize <= 0)
        {
            LE_WARN("No trip points are associated with the thermal zone");
        }
        else if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(thermalZone, tripPointListSize);
        }
        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(thermalZone, &boundCDevListSize);
        LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
            "taf_therm_GetBoundCoolingDeviceListSize - LE_OK");
        if(result != LE_OK || boundCDevListSize <= 0)
        {
            LE_WARN("No cooling devices bounded with thermal zone");
        }
        else if (boundCDevListSize > 0)
        {
            TestBoundCoolingDevicesInformation(thermalZone);
        }
    }
    taf_therm_ReleaseThermalZoneRef(thermalZone);
}

void CDevInfoTest()
{
    LE_TEST_INFO("===== Get Cooling Device By Name =====");
    le_result_t result;

    ThermalCheckArgs(2);
    const char* coolingDeviceName = le_arg_GetArg(1);
    if (coolingDeviceName == NULL)
    {
        LE_ERROR("coolingDeviceName is NULL.");
        return;
    }

    taf_therm_CoolingDeviceRef_t cDev;

    cDev = taf_therm_GetCoolingDeviceByName(coolingDeviceName);
    LE_TEST_OK((cDev != NULL), "taf_therm_GetCoolingDeviceByName - LE_OK");

    if (cDev != NULL)
    {
        char description[TYPE_SIZE];
        memset(description, 0, TYPE_SIZE);
        result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
        LE_TEST_OK(result == LE_OK, "Cooling Device description: %s", description);

        uint32_t coolingID;
        result = taf_therm_GetCDevID(cDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "Cooling device ID : %d", coolingID);

        uint32_t maxCooling;
        result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
        LE_TEST_OK(result == LE_OK, "Cooling Device max cooling %d",
                maxCooling);

        LE_TEST_INFO("Test cooling device current cooling Retrieval");
        uint32_t currCooling;
        result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
        LE_TEST_OK(result == LE_OK, "Cooling Device current cooling: %d", currCooling);

        result = taf_therm_ReleaseCoolingDeviceRef(cDev);
        LE_TEST_OK(result == LE_OK, "taf_therm_ReleaseCoolingDeviceRef - LE_OK");
    }
}

void AllThermalZonesTest(void)
{
    le_result_t result;
    LE_TEST_INFO("===== Get All Thermal Zones =====");
    LE_TEST_INFO("Retrieving thermal zone list with taf_therm_GetThermalZonesList");
    taf_therm_ThermalZoneListRef_t tZoneListRef = taf_therm_GetThermalZonesList();
    LE_TEST_OK((tZoneListRef != NULL), "taf_therm_GetThermalZonesList - LE_OK");

    if (tZoneListRef == NULL)
    {
        LE_ERROR("Not able to get reference to thermal zone list");
        return;
    }

    taf_therm_ThermalZoneListRef_t headTZoneListRef = tZoneListRef;

    LE_TEST_INFO("Retrieving first thermal zone with taf_therm_GetFirstThermalZone");
    taf_therm_ThermalZoneRef_t tZone = taf_therm_GetFirstThermalZone(tZoneListRef);
    LE_TEST_OK((tZone != NULL), "taf_therm_GetFirstThermalZone - LE_OK");

    uint32_t thermalZoneListSize;
    result = taf_therm_GetThermalZonesListSize(tZoneListRef, &thermalZoneListSize);
    LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
        "taf_therm_GetThermalZonesListSize - LE_OK");
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
        LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
            "taf_therm_GetTripPointListSize - LE_OK");
        if(result != LE_OK || tripPointListSize <= 0)
        {
            LE_WARN("No trip points are associated with the thermal zone");
        }
        else if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(tZone, tripPointListSize);
        }

        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &boundCDevListSize);
        LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
            "taf_therm_GetBoundCoolingDeviceListSize - LE_OK");
        if(result != LE_OK || boundCDevListSize <= 0)
        {
            LE_WARN("No cooling devices bounded with thermal zone");
        }
        else if (boundCDevListSize > 0)
        {
            TestBoundCoolingDevicesInformation(tZone);
        }

        if (thermalZoneListSize > 0)
        {
            LE_TEST_INFO("Retrieving next thermal zone with taf_therm_GetNextThermalZone");
            tZone = taf_therm_GetNextThermalZone(tZoneListRef);
            LE_TEST_OK((tZone != NULL), "taf_therm_GetNextThermalZone - LE_OK");
        }
    }

    LE_TEST_INFO("Deleting thermal zone list with taf_therm_DeleteThermalZoneList");
    result = taf_therm_DeleteThermalZoneList(headTZoneListRef);
    LE_TEST_OK(result == LE_OK, "taf_therm_DeleteThermalZoneList - LE_OK");
    LE_INFO("===== Completed retrieving information about all thermal zones =====");
}

void AllCoolingDevicesTest(void)
{
    le_result_t result;
    LE_TEST_INFO("===== Get All Cooling Devices =====");
    LE_TEST_INFO("Retrieving cooling device list with taf_therm_GetCoolingDeviceList");
    taf_therm_CoolingDeviceListRef_t cDevListRef = taf_therm_GetCoolingDeviceList();
    LE_TEST_OK((cDevListRef != NULL), "taf_therm_GetCoolingDeviceList - LE_OK");

    if (cDevListRef == NULL)
    {
        LE_ERROR("Not able to get reference to cooling device list");
        return;
    }

    taf_therm_CoolingDeviceListRef_t headCDevListRef = cDevListRef;

    LE_TEST_INFO("Retrieving first cooling device with taf_therm_GetFirstCoolingDevice");
    taf_therm_CoolingDeviceRef_t cDev = taf_therm_GetFirstCoolingDevice(cDevListRef);
    LE_TEST_OK((cDev != NULL), "taf_therm_GetFirstCoolingDevice - LE_OK");

    uint32_t coolingDeviceListSize;
    result = taf_therm_GetCoolingDeviceListSize(cDevListRef, &coolingDeviceListSize);
    LE_TEST_OK(result == LE_OK || result == LE_NOT_FOUND,
        "taf_therm_GetCoolingDeviceListSize - LE_OK");
    LE_INFO("Cooling device list size: %d", coolingDeviceListSize);

    if(result != LE_OK || coolingDeviceListSize <= 0)
    {
        LE_ERROR("No cooling device present");
        return;
    }

    while (cDev != NULL and coolingDeviceListSize--)
    {
        char description[TYPE_SIZE];
        memset(description, 0, TYPE_SIZE);
        result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
        LE_TEST_OK(result == LE_OK, "CDev Description: %s", description);

        uint32_t coolingID;
        result = taf_therm_GetCDevID(cDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "CDev ID: %d", coolingID);

        uint32_t maxCooling;
        result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
        LE_TEST_OK(result == LE_OK, "CDev Max CoolingLevel: %d", maxCooling);

        uint32_t currCooling;
        result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
        LE_TEST_OK(result == LE_OK, "CDev Current CoolingLevel: %d", currCooling);

        if (coolingDeviceListSize > 0)
        {
            LE_TEST_INFO("Retrieving next cooling device with taf_therm_GetNextCoolingDevice");
            cDev = taf_therm_GetNextCoolingDevice(cDevListRef);
            LE_TEST_OK((cDev != NULL), "taf_therm_GetNextCoolingDevice - LE_OK");
        }
    }

    LE_TEST_INFO("Deleting cooling device list with taf_therm_DeleteCoolingDeviceList");
    result = taf_therm_DeleteCoolingDeviceList(headCDevListRef);
    LE_TEST_OK((result == LE_OK), "taf_therm_DeleteCoolingDeviceList - LE_OK");
    LE_INFO("===== Completed retrieving information about all cooling devices =====");
}

void GetZoneAndCoolingDevInfoTest()
{
    le_result_t result;
    uint32_t thermalZoneListSize = 0;

    LE_TEST_INFO("===== Print All Thermal Sensor Types =====");

    taf_therm_ThermalZoneListRef_t tZoneListRef = taf_therm_GetThermalZonesList();
    if (tZoneListRef == NULL)
    {
        LE_ERROR("tZoneListRef is NULL");
        return;
    }
    result = taf_therm_GetThermalZonesListSize(tZoneListRef, &thermalZoneListSize);
    if (result != LE_OK || thermalZoneListSize == 0)
    {
        LE_WARN("No thermal zones present");
        taf_therm_DeleteThermalZoneList(tZoneListRef);
        return;
    }

    taf_therm_ThermalZoneRef_t tZone = taf_therm_GetFirstThermalZone(tZoneListRef);
    if (tZone == NULL)
    {
        LE_ERROR("taf_therm_GetFirstThermalZone failed");
        taf_therm_DeleteThermalZoneList(tZoneListRef);
        return;
    }

    while ((tZone != NULL) && thermalZoneListSize--)
    {
        char thermalZoneType[TYPE_SIZE] = {0};

        result = taf_therm_GetThermalZoneType(tZone, thermalZoneType, sizeof(thermalZoneType));
        if (result != LE_OK)
        {
            LE_ERROR("taf_therm_GetThermalZoneType failed");
            if (thermalZoneListSize > 0)
            {
                tZone = taf_therm_GetNextThermalZone(tZoneListRef);
            }
            continue;
        }

        LE_INFO("Therm Zone Type: '%s'", thermalZoneType);

        // Get trip point for thermal zone if any
        uint32_t tripPointListSize = 0;
        result = taf_therm_GetTripPointListSize(tZone, &tripPointListSize);
        if (result == LE_OK && tripPointListSize > 0)
        {
            taf_therm_TripPointRef_t tripPoint = taf_therm_GetFirstTripPoint(tZone);
            if (tripPoint != NULL)
            {
                uint32_t tripPointTZoneID = 0;
                result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tripPointTZoneID);
                if (result == LE_OK)
                {
                    LE_INFO("Trip Point ID: %d", tripPointTZoneID);
                }
            }
        }

        // Get bound cooling device for thermal zone if any
        uint32_t boundCDevListSize = 0;
        result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &boundCDevListSize);
        if (result == LE_OK && boundCDevListSize > 0)
        {
            LE_INFO("boundCDevListSize: %d", boundCDevListSize);

            taf_therm_BoundCoolingDeviceRef_t boundCDev = taf_therm_GetFirstBoundCDev(tZone);
            while ((boundCDev != NULL) && boundCDevListSize--)
            {
                uint32_t coolingID = 0;
                result = taf_therm_GetBoundCoolingId(boundCDev, &coolingID);
                if (result != LE_OK){
                    LE_ERROR("taf_therm_GetBoundCoolingId failed");
                    break;
                }

                LE_INFO("Therm Zone Type: '%s', Bound Cooling ID: %d", thermalZoneType, coolingID);

                if (boundCDevListSize > 0)
                {
                    boundCDev = taf_therm_GetNextBoundCDev(tZone);
                    if (boundCDev == NULL)
                    {
                        LE_ERROR("taf_therm_GetNextBoundCDev failed");
                        break;
                    }
                }
            }
        }

        if (thermalZoneListSize > 0)
        {
            tZone = taf_therm_GetNextThermalZone(tZoneListRef);

        }
    }

    result = taf_therm_DeleteThermalZoneList(tZoneListRef);
    if (result != LE_OK)
    {
        LE_ERROR("taf_therm_DeleteThermalZoneList failed");
    }
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
    LE_TEST_OK(result == LE_OK, "Tip Point id: %d", tripID);

    uint32_t tZoneID;
    result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
    LE_TEST_OK(result == LE_OK, "Trip Point Thermal Zone ID: %d",
            tZoneID);

    char tripType[TYPE_SIZE];
    memset(tripType, 0, TYPE_SIZE);
    result = taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
    LE_TEST_OK(result == LE_OK, "Trip Point Type: %s", tripType);

    int32_t threshold;
    result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
    LE_TEST_OK(result == LE_OK, "Trip Point Threshold: %d",
           threshold);

    int32_t hysterisis;
    result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
    LE_TEST_OK(result == LE_OK, "Trip Point Hysterisis: %d",
           hysterisis);

    LE_INFO("Trip Type is %s\n\n", TripEventToString(type));
    result = taf_therm_ReleaseTripEventRef(tripPoint);
    LE_TEST_OK(result == LE_OK, "taf_therm_ReleaseTripEventRef - LE_OK");
}

static void* AddTripEventHandler(void* contextPtr)
{
    taf_therm_ConnectService();

    tripEventHandlerRef =
        taf_therm_AddTripEventHandler(TestTripEventHandler, NULL);
    LE_TEST_OK(tripEventHandlerRef != NULL,
        "Register AddTripEventHandler change handler is successfull");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();
}

void RemoveTripEventRefHandler() {
    taf_therm_RemoveTripEventHandler(tripEventHandlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
 //------------------------------------------------------------------------------------------------

void TripEventHandlerTest(void)
{
    ThermalCheckArgs(2);
    LE_TEST_INFO("======== Thermal Trip Event Handler Test ========\n");

    const char* arg3 = le_arg_GetArg(1);
    if (arg3 != NULL)
    {
        long time = strtol(arg3, NULL, 10);
        le_sem_Ref_t semaphore = le_sem_Create("TripEventSemaphore", 0);
        le_thread_Ref_t threadRef = le_thread_Create("TripEventThread",
            AddTripEventHandler, (void*)semaphore);
        le_thread_Start(threadRef);

        le_thread_Sleep(time);
        le_sem_Wait(semaphore);
        le_sem_Delete(semaphore);

        RemoveTripEventRefHandler();
    }
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
    LE_TEST_OK(result == LE_OK, "CDev Max CoolingLevel : %d", maxCooling);

    uint32_t currCooling;
    result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
    LE_TEST_OK(result == LE_OK, "CDev Current CoolingLevel : %d\n\n",
            currCooling);

    result = taf_therm_ReleaseCoolingDeviceRef(cDev);
    LE_TEST_OK(result == LE_OK, "taf_therm_ReleaseCoolingDeviceRef - LE_OK");
}


static void* AddCoolingLevelChangeEventHandler(void* contextPtr)
{
    taf_therm_ConnectService();

    cooolingLevelChangeHandlerRef =
            taf_therm_AddCoolingLevelChangeEventHandler(TestCoolingLevelChangeHandler, NULL);
    LE_TEST_OK(cooolingLevelChangeHandlerRef != NULL,
            "Register AddCoolingLevelChangeEventHandler change handler is successfull");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();
}

void RemoveCoolingLevelChangeEventRefHandler() {
    taf_therm_RemoveCoolingLevelChangeEventHandler(cooolingLevelChangeHandlerRef);
}

void CoolingLevelChangeEventHandlerrTest(void)
{
    ThermalCheckArgs(2);
    LE_TEST_INFO("======== Thermal Cooling Level Change Event Handler Test ========\n");

    const char* arg2 = le_arg_GetArg(1);
    if (arg2 != NULL)
    {
        long time = strtol(arg2, NULL, 10);
        le_sem_Ref_t semaphore = le_sem_Create("CoolingLevelChangeSemaphore", 0);
        le_thread_Ref_t threadRef = le_thread_Create("CoolingLevelChangeThread",
            AddCoolingLevelChangeEventHandler, (void*)semaphore);
        le_thread_Start(threadRef);

        le_thread_Sleep(time);
        le_sem_Wait(semaphore);
        le_sem_Delete(semaphore);

        RemoveCoolingLevelChangeEventRefHandler();
    }
}

COMPONENT_INIT
{
    LE_INFO("*** Checking for Console args ***");
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    ThermalCheckArgs(1);
    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        LE_ERROR("cmd is NULL");
        LE_TEST_EXIT;
    }

    LE_TEST_INFO("======== TelAF Thermal Service Integration Test %s ========", cmd);

    if (strncmp(cmd, "AllThermalZones", strlen(cmd)) == 0)
    {
        AllThermalZonesTest();
    }
    else if (strncmp(cmd, "AllCoolingDevices", strlen(cmd)) == 0)
    {
        AllCoolingDevicesTest();
    }
    else if (strncmp(cmd, "ThermalZoneInfo", strlen(cmd)) == 0)
    {
        ThermalZoneInfoTest();
    }
    else if (strncmp(cmd, "GetZoneAndCoolingDevInfo", strlen(cmd)) == 0)
    {
        GetZoneAndCoolingDevInfoTest();
    }
    else if (strncmp(cmd, "CDevInfo", strlen(cmd)) == 0)
    {
        CDevInfoTest();
    }
    else if (strncmp(cmd, "TripEventHandler", strlen(cmd)) == 0)
    {
        TripEventHandlerTest();
    }
    else if (strncmp(cmd, "CoolingLevelChangeEventHandler", strlen(cmd)) == 0)
    {
        CoolingLevelChangeEventHandlerrTest();
    }
    else
    {
        ThermalPrintHelpMenu();
    }
    LE_TEST_EXIT;
}
