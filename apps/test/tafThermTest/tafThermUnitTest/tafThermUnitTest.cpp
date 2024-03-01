/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <iostream>
#include <vector>

#define TYPE_SIZE 32
static le_sem_Ref_t semRef1;
taf_therm_TripEventHandlerRef_t handlerRef;
taf_therm_CoolingLevelChangeEventHandlerRef_t cooolingLevelChangeHandlerRef;

void TestZoneInformation(taf_therm_ThermalZoneRef_t tZone)
{
    le_result_t result;
    LE_TEST_INFO("Test Thermal Zone ID Retrieval");
    uint32_t tZoneid;
    result = taf_therm_GetThermalZoneID(tZone, &tZoneid);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetThermalZoneID - LE_OK. Returned %d", tZoneid);

    char thermalZoneType[TYPE_SIZE];
    memset(thermalZoneType, 0, TYPE_SIZE);
    LE_TEST_INFO("Test Thermal Zone Name Retrieval");
    result = taf_therm_GetThermalZoneType(tZone, thermalZoneType, sizeof(thermalZoneType));
    LE_TEST_OK(result == LE_OK,
            "taf_therm_GetThermalZoneType - LE_OK. Returned %s", thermalZoneType);

    LE_TEST_INFO("Test Thermal Zone Current Temp Retrieval");
    uint32_t currTemp;
    result = taf_therm_GetThermalZoneCurrentTemp(tZone, &currTemp);
    LE_TEST_OK((result == LE_OK),"taf_therm_GetThermalZoneCurrentTemp - LE_OK. Returned %d",
            currTemp);

    LE_TEST_INFO("Test Thermal Zone Passive Temp Retrieval");
    uint32_t passiveTemp;
    result = taf_therm_GetThermalZonePassiveTemp(tZone, &passiveTemp);
    LE_TEST_OK((result == LE_OK),"taf_therm_GetThermalZonePassiveTemp - LE_OK. Returned %d",
            passiveTemp);
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
    while (tripPoint != NULL and listSize--)
    {
        LE_TEST_INFO("Test Trip Point ID Retrieval");
        uint32_t tripID;
        result = taf_therm_GetTripPointTripID(tripPoint, &tripID);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointTripID - LE_OK. Returned %d", tripID);

        LE_TEST_INFO("Test Trip Point Thermal Zone ID Retrieval");
        uint32_t tZoneID;
        result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointThermalZoneID - LE_OK. Returned %d",
                tZoneID);

        char tripType[TYPE_SIZE];
        memset(tripType, 0, TYPE_SIZE);
        LE_TEST_INFO("Test Trip Point Type Retrieval");
        taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointType - LE_OK. Returned %s",tripType);

        LE_TEST_INFO("Test Trip Point threshold Retrieval");
        uint32_t threshold;
        result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointThreshold - LE_OK. Returned %d",
                threshold);

        LE_TEST_INFO("Test Trip Point hysterisis Retrieval");
        uint32_t hysterisis;
        result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointHysterisis - LE_OK. Returned %d",
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

    while (boundCDev != NULL and listSize--)
    {
        LE_TEST_INFO("Test Bound cooling ID Retrieval");
        uint32_t coolingID;
        result = taf_therm_GetBoundCoolingId(boundCDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundCoolingId - LE_OK. Returned %d",coolingID);
        uint32_t tripPointListSize;
        if (taf_therm_GetBoundTripPointListSize(boundCDev, &tripPointListSize) <= 0)
            return;
        LE_TEST_INFO("Testing TelAF First Bound Cooling Device's Trip Point Reference"
                "Retrieval with - taf_therm_GetFirstBoundTripPoint");
        taf_therm_TripPointRef_t boundTripPoint = taf_therm_GetFirstBoundTripPoint(boundCDev);
        LE_TEST_OK((boundTripPoint != NULL), "taf_therm_GetFirstBoundTripPoint - LE_OK");
        result = taf_therm_GetBoundTripPointListSize(boundCDev, &tripPointListSize);
        while (boundTripPoint != NULL and tripPointListSize--)
        {
            LE_TEST_INFO("Test Trip Point ID Retrieval");
            uint32_t boundTripID;
            result = taf_therm_GetBoundTripPointTripID(boundTripPoint, &boundTripID);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundTripPointTripID -LE_OK. Returned %d",
                    boundTripID);

            LE_TEST_INFO("Test Trip Point Thermal Zone ID Retrieval");
            uint32_t boundTZoneID;
            result = taf_therm_GetBoundTripPointThermalZoneID(boundTripPoint, &boundTZoneID);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundTripPointThermalZoneID -"
                    "LE_OK. Returned %d", boundTZoneID);

            char boundTripType[TYPE_SIZE];
            memset(boundTripType, 0, TYPE_SIZE);
            LE_TEST_INFO("Test Trip Point Type Retrieval");
            taf_therm_GetBoundTripPointType(boundTripPoint, boundTripType, sizeof(boundTripType));
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundTripPointType - LE_OK."
                    "Returned %s", boundTripType);

            LE_TEST_INFO("Test Trip Point threshold Retrieval");
            uint32_t boundThreshold;
            result = taf_therm_GetBoundTripPointThreshold(boundTripPoint, &boundThreshold);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundTripPointThreshold -"
                    "LE_OK. Returned %d", boundThreshold);

            LE_TEST_INFO("Test Trip Point hysterisis Retrieval");
            uint32_t boundHysterisis;
            result = taf_therm_GetBoundTripPointHysterisis(boundTripPoint, &boundHysterisis);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetBoundTripPointHysterisis -"
                    "LE_OK. Returned %d", boundHysterisis);

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
        return;

    taf_therm_ThermalZoneListRef_t headTZoneListRef = tZoneListRef;

    LE_TEST_INFO("Testing TelAF First Thermal Zone Reference Retrieval with -"
            "taf_therm_GetFirstThermalZone");
    taf_therm_ThermalZoneRef_t tZone = taf_therm_GetFirstThermalZone(tZoneListRef);
    LE_TEST_OK((tZone != NULL), "taf_therm_GetFirstThermalZone - LE_OK");
    uint32_t thermalZoneListSize;
    result = taf_therm_GetThermalZonesListSize(tZoneListRef, &thermalZoneListSize);

    while (tZone != NULL and thermalZoneListSize--)
    {
        uint32_t tripPointListSize;
        result = taf_therm_GetTripPointListSize(tZone, &tripPointListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(tZone, &boundCDevListSize);
        TestZoneInformation(tZone);
        if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(tZone);
        }
        if (boundCDevListSize > 0)
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
    while (cDev != NULL and coolingDeviceListSize--)
    {
        char description[TYPE_SIZE];
        memset(description, 0, TYPE_SIZE);
        LE_TEST_INFO("Test cooling device description Retrieval");
        result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
        LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevDescription - LE_OK. Returned %s",
                description);

        LE_TEST_INFO("Test cooling device ID Retrieval");
        uint32_t coolingID;
        result = taf_therm_GetCDevID(cDev, &coolingID);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevID - LE_OK. Returned %d", coolingID);

        LE_TEST_INFO("Test cooling device max cooling Retrieval");
        uint32_t maxCooling;
        result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevMaxCoolingLevel - LE_OK. Returned %d",
                maxCooling);

        LE_TEST_INFO("Test cooling device current cooling Retrieval");
        uint32_t currCooling;
        result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevCurrentCoolingLevel - LE_OK.Returned %d",
                currCooling);

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

    const char* thermalZoneName[36] = { "sdr0_pa","sdr0","mmw0","mmw1","mmw2","mmw3","mmw_ific0",
        "epm0","epm1","epm2","epm3","epm4","epm5","epm6","epm7","aoss-0","cpuss-0",
        "cpuss-1","cpuss-2","cpuss-3","ethphy-0","mvmss-0","mdmq6-0","ctile",
        "mdmss-0","mdmss-1","mdmss-2","zeroc","pmx75_tz","sys-therm-1","sys-therm-2",
        "sys-therm-3","sys-therm-4","xo-therm","sdr1_pa","sdr1" };

    for (uint i = 0; i < 36; i++)
    {
        LE_TEST_INFO("Testing TelAF getting thermal zone by name with -"
                "taf_therm_GetThermalZoneByName");
        taf_therm_ThermalZoneRef_t thermalZone =taf_therm_GetThermalZoneByName(thermalZoneName[i]);
        LE_TEST_OK((thermalZone != NULL), "taf_therm_GetThermalZoneByName - LE_OK");

        if (thermalZone != NULL)
        {
            uint32_t tripPointListSize;
            result = taf_therm_GetTripPointListSize(thermalZone, &tripPointListSize);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
            uint32_t boundCDevListSize;
            result = taf_therm_GetBoundCoolingDeviceListSize(thermalZone, &boundCDevListSize);
            TestZoneInformation(thermalZone);
            if (tripPointListSize > 0)
            {
                LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
                TestTripPointInformation(thermalZone);
            }
            if (boundCDevListSize > 0)
            {
                TestBoundCoolingDevicesInformation(thermalZone);
            }
        }
    }
    LE_TEST_INFO("Testing for unavailable thermal zone");
    taf_therm_ThermalZoneRef_t thermalZone =
        taf_therm_GetThermalZoneByName("UnavailableThermalZone");
    LE_TEST_OK((thermalZone == NULL), "Negative assertion taf_therm_GetThermalZoneByName - LE_OK");

    if (thermalZone != NULL)
    {
        uint32_t tripPointListSize;
        result = taf_therm_GetTripPointListSize(thermalZone, &tripPointListSize);
        LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointListSize - LE_OK");
        uint32_t boundCDevListSize;
        result = taf_therm_GetBoundCoolingDeviceListSize(thermalZone, &boundCDevListSize);
        TestZoneInformation(thermalZone);
        if (tripPointListSize > 0)
        {
            LE_INFO("TRIP POINT LIST SIZE %d", tripPointListSize);
            TestTripPointInformation(thermalZone);
        }
        if (boundCDevListSize > 0)
        {
            TestBoundCoolingDevicesInformation(thermalZone);
        }
    }
    LE_INFO("===== UnitTest Completed for getting thermal zone by name =====");
}

void TestCoolingDeviceByName()
{
    LE_TEST_INFO("===== Get Cooling Device By Name =====");
    le_result_t result;
    const char* coolingDevice[28] = { "cpu-hotplug1","cpu-hotplug2","cpu-hotplug3",
        "cpufreq-cpu0","modem_vdd","modem_lte_dsc","modem_lte_sub1_dsc",
        "modem_nr_dsc","modem_nr_sub1_dsc","modem_nr_scg_dsc","modem_nr_scg_sub1_dsc",
        "sdr0_lte_dsc","sdr0_nr_dsc","pa_lte_sdr0_dsc","pa_lte_sdr0_sub1_dsc",
        "pa_nr_sdr0_dsc","pa_nr_sdr0_sub1_dsc","pa_nr_sdr0_scg_dsc",
        "pa_nr_sdr0_scg_sub1_dsc","mmw0_dsc","mmw0_sub1_dsc","mmw1_dsc",
        "mmw1_sub1_dsc","mmw2_dsc","mmw2_sub1_dsc","mmw3_dsc","mmw3_sub1_dsc","modem_v2x", };

    for (uint i = 0; i < 28; i++)
    {
        LE_TEST_INFO("Testing TelAF getting cooling device by name with -"
                "taf_therm_GetCoolingDeviceByName");
        taf_therm_CoolingDeviceRef_t cDev = taf_therm_GetCoolingDeviceByName(coolingDevice[i]);
        LE_TEST_OK((cDev != NULL), "taf_therm_GetCoolingDeviceByName - LE_OK");

        if (cDev != NULL)
        {
            char description[TYPE_SIZE];
            memset(description, 0, TYPE_SIZE);
            LE_TEST_INFO("Test cooling device description Retrieval");
            result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
            LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevDescription - LE_OK. Returned %s",
                    description);

            LE_TEST_INFO("Test cooling device ID Retrieval");
            uint32_t coolingID;
            result = taf_therm_GetCDevID(cDev, &coolingID);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevID - LE_OK. Returned %d", coolingID);

            LE_TEST_INFO("Test cooling device max cooling Retrieval");
            uint32_t maxCooling;
            result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevMaxCoolingLevel - LE_OK. Returned %d",
                    maxCooling);

            LE_TEST_INFO("Test cooling device current cooling Retrieval");
            uint32_t currCooling;
            result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
            LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevCurrentCoolingLevel - LE_OK.Returned %d",
                    currCooling);
        }
    }
    LE_TEST_INFO("Testing for unavailable cooling device");
    taf_therm_CoolingDeviceRef_t cDev = taf_therm_GetCoolingDeviceByName("UnavailableCoolingDev");
    LE_TEST_OK((cDev == NULL), "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
    LE_TEST_OK((cDev == nullptr), "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
    LE_TEST_OK(!cDev, "Negative assertion taf_therm_GetCoolingDeviceByName - LE_OK");
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
    LE_TEST_INFO("Test Trip Point ID Retrieval");
    uint32_t tripID;
    result = taf_therm_GetTripPointTripID(tripPoint, &tripID);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointTripID - LE_OK. Returned %d", tripID);
    LE_TEST_INFO("Test Trip Point Thermal Zone ID Retrieval");
    uint32_t tZoneID;
    result = taf_therm_GetTripPointThermalZoneID(tripPoint, &tZoneID);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointThermalZoneID - LE_OK. Returned %d",
            tZoneID);
    char tripType[TYPE_SIZE];
    memset(tripType, 0, TYPE_SIZE);
    LE_TEST_INFO("Test Trip Point Type Retrieval");
    taf_therm_GetTripPointType(tripPoint, tripType, sizeof(tripType));
    LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointType - LE_OK. Returned %s", tripType);

    LE_TEST_INFO("Test Trip Point threshold Retrieval");
    uint32_t threshold;
    result = taf_therm_GetTripPointThreshold(tripPoint, &threshold);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointThreshold - LE_OK. Returned %d",
           threshold);

    LE_TEST_INFO("Test Trip Point hysterisis Retrieval");
    uint32_t hysterisis;
    result = taf_therm_GetTripPointHysterisis(tripPoint, &hysterisis);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetTripPointHysterisis - LE_OK. Returned %d",
           hysterisis);
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
    LE_TEST_INFO("Test cooling device description Retrieval");
    result = taf_therm_GetCDevDescription(cDev, description, sizeof(description));
    LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevDescription - LE_OK. Returned %s",
            description);
    LE_TEST_INFO("Test cooling device ID Retrieval");
    uint32_t coolingID;
    result = taf_therm_GetCDevID(cDev, &coolingID);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevID - LE_OK. Returned %d", coolingID);
    LE_TEST_INFO("Test cooling device max cooling Retrieval");
    uint32_t maxCooling;
    result = taf_therm_GetCDevMaxCoolingLevel(cDev, &maxCooling);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevMaxCoolingLevel - LE_OK. Returned %d",
            maxCooling);

    LE_TEST_INFO("Test cooling device current cooling Retrieval");
    uint32_t currCooling;
    result = taf_therm_GetCDevCurrentCoolingLevel(cDev, &currCooling);
    LE_TEST_OK(result == LE_OK, "taf_therm_GetCDevCurrentCoolingLevel - LE_OK.Returned %d\n\n",
            currCooling);

    taf_therm_ReleaseCoolingDeviceRef(cDev);
}



static void* Test_taf_therm_AddTripEventHandler(void* ctxPtr)
{
    taf_therm_ConnectService();

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

    le_sem_Post(semRef1);
    le_event_RunLoop();
}

le_result_t WaitForSem_Timeout
(
      le_sem_Ref_t semRef,
      uint32_t seconds
)
{
    le_clk_Time_t timeToWait = { static_cast<int>(seconds), 0 };
    return le_sem_WaitWithTimeOut(semRef, timeToWait);
}

void CreateHandlerTestThread() {
    semRef1 = le_sem_Create("SemRef1", 0);
    le_thread_Ref_t threadRef1 =
            le_thread_Create("Thread1", Test_taf_therm_AddTripEventHandler, NULL);
    le_thread_Start(threadRef1);
    WaitForSem_Timeout(semRef1, 5);
}

void RemoveTestHandler() {

    LE_TEST_INFO("Testing taf_therm_RemoveTripEventHandler handler reference");
    taf_therm_RemoveTripEventHandler(handlerRef);
    LE_TEST_OK(true, "taf_therm_RemoveTripEventHandler successfull");

    LE_TEST_INFO("Testing RemoveCoolingLevelChangeEventHandler handler reference");
    taf_therm_RemoveCoolingLevelChangeEventHandler(cooolingLevelChangeHandlerRef);
    LE_TEST_OK(true, "RemoveCoolingLevelChangeEventHandler successfull");
}

COMPONENT_INIT
{
    //long time = 20;
    int args = le_arg_NumArgs();
    int time;
    const char* arg1;
    if (args == 1)
    {
        arg1 = le_arg_GetArg(0);
    }
    else
    {
        arg1 = "20";
    }
    time = atoi(arg1);
    CreateHandlerTestThread();

    le_thread_Sleep(time);

    RemoveTestHandler();

    TestThermalZoneInformation();
    TestCoolingDeviceInformation();
    TestThermalZoneByName();
    TestCoolingDeviceByName();  

    exit(EXIT_SUCCESS);
}