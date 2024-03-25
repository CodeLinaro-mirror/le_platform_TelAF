/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalECall.h"

//--------------------------------------------------------------------------------------------------
/**
 * eCall definition.
 */
//--------------------------------------------------------------------------------------------------

#define DEV_NAME "/dev/eCall0"

//--------------------------------------------------------------------------------------------------
/**
 * Implement functional interfaces.
 */
//--------------------------------------------------------------------------------------------------
static void taf_hal_PowerOn()
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    return 0;
}

static int taf_hal_SelfTest()
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    return 0;
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.eCallInf);
}

static le_result_t tal_hal_GetVehicleInfo(taf_hal_eCall_VehicleInfo* vehInfo)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    vehInfo->vehiType = ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1;
    snprintf(vehInfo->vin, sizeof("ECALLEXAMPLE02024"), "%s", "ECALLEXAMPLE02024");
    vehInfo->propulsionType = ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK;
    return LE_OK;
}

static le_result_t tal_hal_GetActivateType(taf_hal_eCall_ActivateType* actType)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    *actType = ECALL_HAL_ACTTYPE_AUTOMATIC;
    return LE_OK;
}

static le_result_t tal_hal_GetPassengerCount(uint8_t* passCount)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    *passCount = 5;
    return LE_OK;
}

static le_result_t tal_hal_GetIILocations(taf_hal_eCall_IILocations* iILocations)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    *iILocations = ECALL_HAL_LOI_FRONT;
    return LE_OK;
}

static le_result_t tal_hal_GetRolloverDetected(taf_hal_eCall_RolloverDetected* rollDetected)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    rollDetected->rolloverDetectedPresent = true;
    rollDetected->rolloverDetected = true;
    return LE_OK;
}

static le_result_t tal_hal_GetDeltaV(taf_hal_eCall_DeltaV* deltaV)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
    deltaV->rangeLimit = 125;
    deltaV->deltaVX = -45;
    deltaV->deltaVY = 10;
    return LE_OK;
}

static void Init(void)
{
    LE_INFO("eCallVHalDrv: %s", __FUNCTION__);
}

LE_SHARED eCall_InfoTab_t TAF_HAL_INFO_TAB = {
    // Management interface always comes first
    .mgrInf = {
        .name = TAF_ECALL_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_HAL,
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .eCallInf = {
        .InitHAL = Init,
        .getVehicleInfo = tal_hal_GetVehicleInfo,
        .getActivateType = tal_hal_GetActivateType,
        .getPassengerCount = tal_hal_GetPassengerCount,
        .getIILocations = tal_hal_GetIILocations,
        .getRolloverDetected = tal_hal_GetRolloverDetected,
        .getDeltaV = tal_hal_GetDeltaV
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    LE_INFO("eCall Drv is loading\n");
}
