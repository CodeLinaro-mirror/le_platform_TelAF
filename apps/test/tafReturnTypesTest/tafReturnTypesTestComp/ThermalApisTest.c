/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Thermal Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void thermRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("thermRetTest_RunApis");

    //1.taf_therm_GetThermalZonesListSize LE_BAD_PARAMETER scenario
    res = taf_therm_GetThermalZonesListSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetThermalZonesListSize-LE_NOT_PERMITTED");

    //2.taf_therm_GetFirstThermalZone NULL scenario
    taf_therm_ThermalZoneRef_t therm;
    therm = taf_therm_GetFirstThermalZone(NULL);
    LE_TEST_OK(therm == NULL,"taf_therm_GetFirstThermalZone-NULL");

    //3.taf_therm_GetNextThermalZone NULL scenario
    therm = taf_therm_GetNextThermalZone(NULL);
    LE_TEST_OK(therm == NULL,"taf_therm_GetNextThermalZone-NULL");

    //4.taf_therm_GetFirstTripPoint NULL scenario
    taf_therm_TripPointRef_t trip;
    trip = taf_therm_GetFirstTripPoint(NULL);
    LE_TEST_OK(trip == NULL,"taf_therm_GetFirstTripPoint-NULL");

    //5.taf_therm_GetNextTripPoint NULL scenario
    trip = taf_therm_GetNextTripPoint(NULL);
    LE_TEST_OK(trip == NULL,"taf_therm_GetNextTripPoint-NULL");

    //6.taf_therm_GetFirstBoundCDev NULL scenario
    taf_therm_BoundCoolingDeviceRef_t bound;
    bound = taf_therm_GetFirstBoundCDev(NULL);
    LE_TEST_OK(bound == NULL,"taf_therm_GetFirstBoundCDev-NULL");

    //7.taf_therm_GetNextBoundCDev NULL scenario
    bound = taf_therm_GetNextBoundCDev(NULL);
    LE_TEST_OK(bound == NULL,"taf_therm_GetNextBoundCDev-NULL");

    //8.taf_therm_GetFirstBoundTripPoint NULL scenario
    trip = taf_therm_GetFirstBoundTripPoint(NULL);
    LE_TEST_OK(trip == NULL,"taf_therm_GetFirstBoundTripPoint-NULL");

    //9.taf_therm_GetNextBoundTripPoint NULL scenario
    trip = taf_therm_GetNextBoundTripPoint(NULL);
    LE_TEST_OK(trip == NULL,"taf_therm_GetNextBoundTripPoint-NULL");

    //10.taf_therm_GetThermalZoneID LE_BAD_PARAMETER scenario
    res = taf_therm_GetThermalZoneID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetThermalZoneID-LE_BAD_PARAMETER");

    //11.taf_therm_GetThermalZoneType LE_BAD_PARAMETER scenario
    res = taf_therm_GetThermalZoneType(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetThermalZoneType-LE_BAD_PARAMETER");

    //12.taf_therm_GetThermalZoneCurrentTemp LE_BAD_PARAMETER scenario
    res = taf_therm_GetThermalZoneCurrentTemp(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetThermalZoneCurrentTemp-LE_BAD_PARAMETER");

    //13.taf_therm_GetThermalZonePassiveTemp LE_BAD_PARAMETER scenario
    res = taf_therm_GetThermalZonePassiveTemp(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetThermalZonePassiveTemp-LE_BAD_PARAMETER");

    //14.taf_therm_GetTripPointListSize LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointListSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointListSize-LE_BAD_PARAMETER");

    //15.taf_therm_GetBoundCoolingDeviceListSize LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundCoolingDeviceListSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundCoolingDeviceListSize-LE_BAD_PARAMETER");

    //16.taf_therm_GetTripPointType LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointType(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointType-LE_BAD_PARAMETER");

    //17.taf_therm_GetTripPointThreshold LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointThreshold(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointThreshold-LE_BAD_PARAMETER");

    //18.taf_therm_GetTripPointHysterisis LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointHysterisis(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointHysterisis-LE_BAD_PARAMETER");

    //19.taf_therm_GetTripPointTripID LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointTripID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointTripID-LE_BAD_PARAMETER");

    //20.taf_therm_GetTripPointThermalZoneID LE_BAD_PARAMETER scenario
    res = taf_therm_GetTripPointThermalZoneID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetTripPointThermalZoneID-LE_BAD_PARAMETER");

    //21.taf_therm_GetBoundCoolingId LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundCoolingId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundCoolingId-LE_BAD_PARAMETER");

    //22.taf_therm_GetBoundTripPointListSize LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointListSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointListSize-LE_BAD_PARAMETER");

    //23.taf_therm_GetBoundTripPointType LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointType(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointType-LE_BAD_PARAMETER");

    //24.taf_therm_GetBoundTripPointThreshold LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointThreshold(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointThreshold-LE_BAD_PARAMETER");

    //25.taf_therm_GetBoundTripPointHysterisis LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointHysterisis(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointHysterisis-LE_BAD_PARAMETER");

    //26.taf_therm_GetBoundTripPointTripID LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointTripID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointTripID-LE_BAD_PARAMETER");

    //26.taf_therm_GetBoundTripPointThermalZoneID LE_BAD_PARAMETER scenario
    res = taf_therm_GetBoundTripPointThermalZoneID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetBoundTripPointThermalZoneID-LE_BAD_PARAMETER");

    //27.taf_therm_DeleteThermalZoneList LE_BAD_PARAMETER scenario
    res = taf_therm_DeleteThermalZoneList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_DeleteThermalZoneList-LE_BAD_PARAMETER");

    //28.taf_therm_GetCoolingDeviceListSize LE_BAD_PARAMETER scenario
    res = taf_therm_GetCoolingDeviceListSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetCoolingDeviceListSize-LE_BAD_PARAMETER");

    //29.taf_therm_GetFirstCoolingDevice NULL scenario
    taf_therm_CoolingDeviceRef_t cool;
    cool = taf_therm_GetFirstCoolingDevice(NULL);
    LE_TEST_OK(cool == NULL,"taf_therm_GetFirstCoolingDevice-NULL");

    //30.taf_therm_GetNextCoolingDevice NULL scenario
    cool = taf_therm_GetNextCoolingDevice(NULL);
    LE_TEST_OK(cool == NULL,"taf_therm_GetNextCoolingDevice-NULL");

    //31.taf_therm_GetCDevID LE_BAD_PARAMETER scenario
    res = taf_therm_GetCDevID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetCDevID-LE_BAD_PARAMETER");

    //32.taf_therm_GetCDevDescription LE_BAD_PARAMETER scenario
    res = taf_therm_GetCDevDescription(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetCDevDescription-LE_BAD_PARAMETER");

    //33.taf_therm_GetCDevMaxCoolingLevel LE_BAD_PARAMETER scenario
    res = taf_therm_GetCDevMaxCoolingLevel(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetCDevMaxCoolingLevel-LE_BAD_PARAMETER");

    //34.taf_therm_GetCDevCurrentCoolingLevel LE_BAD_PARAMETER scenario
    res = taf_therm_GetCDevCurrentCoolingLevel(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_GetCDevCurrentCoolingLevel-LE_BAD_PARAMETER");

    //35.taf_therm_ReleaseTripEventRef LE_BAD_PARAMETER scenario
    res = taf_therm_ReleaseTripEventRef(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_ReleaseTripEventRef-LE_BAD_PARAMETER");

    //36.taf_therm_ReleaseCoolingDeviceRef LE_BAD_PARAMETER scenario
    res = taf_therm_ReleaseCoolingDeviceRef(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_ReleaseCoolingDeviceRef-LE_BAD_PARAMETER");

    //37.taf_therm_ReleaseThermalZoneRef LE_BAD_PARAMETER scenario
    res = taf_therm_ReleaseThermalZoneRef(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_therm_ReleaseThermalZoneRef-LE_BAD_PARAMETER");

}
