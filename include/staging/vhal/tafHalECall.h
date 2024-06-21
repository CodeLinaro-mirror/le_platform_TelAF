/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @page c_tafECallVhal eCall VHAL
 *
 * @ref tafHalECall.h "API Reference"
 *
 * <HR>
 *
 * To build a driver using eCall VHAL, the developer needs to include this VHAL
 * module header file with the driver component.
 *
 * @section eCallVhal_impl Implement VHAL in compliance with VHAL APIs
 *
 * To implement a VHAL for a eCall hardware module, the developer needs to get this file. The header
 * tafHalECall.h provides all driver management interfaces, which requires the driver information that
 * will be checked by the device manager during driver installation. The driver information includes
 * the driver’s name, version type, and initialization functions for the device.
 *
 * The eCall_InfoTab_t TAF_HAL_INFO_TAB is the entry used by device manager and services.
 *
 * @section eCallVhal_def Define VHAL management interfaces
 *
 * In the context of eCall VHAL, the eCall driver needs to include tafHalECall.h and implement the
 * functions defined in the header file. The eCall VHAL component defines the eCall_InfoTab_t with the
 * actual implementation, including the driver management information and functional interfaces.
 * The following is an example of the definition of management interfaces.
 *
 * @code

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

 * @endcode
 *
 * - name -- Use the corresponding module name macro define in tafHalECall.h.
 * - majorVer -- Major version of the driver.
 * - minorVer -- Minor version of the driver.
 * - vendor -- Vendor name of the driver.
 * - hwInitInf -- Hardware initialization function after installation.
 * - powerOffInf -- Power off function called by the device manager during uninstallation.
 * - powerOnInf -- Power on function called by the device manager during installation.
 * - selfTest -- Self-test function to check if the driver functions work normally (reserved for future).
 * - getModInf -- Function to get the address of the functional interface.
 *
 * The following is an example of the definition of eCall functional interfaces.
 *
 * @code

    .eCallInf = {
        .InitHAL = Init,
        .getVehicleInfo = taf_hal_GetVehicleInfo,
        .getActivateType = taf_hal_GetActivateType,
        .getPassengerCount = taf_hal_GetPassengerCount,
        .getIILocations = taf_hal_GetIILocations,
        .getRolloverDetected = taf_hal_GetRolloverDetected,
        .getDeltaV = taf_hal_GetDeltaV;
    }

 * @endcode
 *
 * <HR>
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef __ECALL_H__

#define __ECALL_H__

#include "tafHalIF.hpp"

// Define the name or ID for HAL module
#define TAF_ECALL_MODULE_NAME "TafHalECall"

#define MAX_VIN_BYTES 17

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle propulsion storage type.
 */
//--------------------------------------------------------------------------------------------------
typedef uint16_t taf_hal_eCall_PropulsionType;
typedef enum
{
    ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK = 1 << 0,         ///< Indicates the presence of a gasoline tank in the vehicle.
    ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK = 1 << 1,           ///< Indicates the presence of diesel tank in the vehicle
    ECALL_HAL_BITMASK_PROP_TYPE_COMPRESSED_NATURALGAS = 1 << 2, ///< Indicates the presence of CNG in the vehicle
    ECALL_HAL_BITMASK_PROP_TYPE_PROPANE_GAS = 1 << 3,           ///< Indicates the presence of liquid propane in the vehicle
    ECALL_HAL_BITMASK_PROP_TYPE_ELECTRIC = 1 << 4,              ///< Indicates the presence of electronic storage in the vehicle
    ECALL_HAL_BITMASK_PROP_TYPE_HYDROGEN = 1 << 5,              ///< Indicates the presence of hydrogen Storage in the vehicle
    ECALL_HAL_BITMASK_PROP_TYPE_OTHER = 1 << 6                  ///< Indicates the presence of other types of storage in the vehicle
} taf_hal_eCall_PropType;

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle classes per the European eCall MSD standard.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1,              ///< Passenger vehicle (Class M1)
    ECALL_HAL_VEHITYPE_BUSES_AND_COACHES_CLASS_M2,              ///< Buses and coaches (Class M2)
    ECALL_HAL_VEHITYPE_BUSES_AND_COACHES_CLASS_M3,              ///< Buses and coaches (Class M3)
    ECALL_HAL_VEHITYPE_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1,      ///< Light commercial vehicles (Class N1)
    ECALL_HAL_VEHITYPE_HEAVY_DUTY_VEHICLES_CLASS_N2,            ///< Heavy duty vehicles (Class N2)
    ECALL_HAL_VEHITYPE_HEAVY_DUTY_VEHICLES_CLASS_N3,            ///< Heavy duty vehicles (Class N3)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L1E,                  ///< Motorcycles (Class L1e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L2E,                  ///< Motorcycles (Class L2e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L3E,                  ///< Motorcycles (Class L3e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L4E,                  ///< Motorcycles (Class L4e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L5E,                  ///< Motorcycles (Class L5e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L6E,                  ///< Motorcycles (Class L6e)
    ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L7E,                  ///< Motorcycles (Class L7e)
    ECALL_HAL_VEHITYPE_TRAILERS_CLASS_O,                        ///< Trailers (Class O)
    ECALL_HAL_VEHITYPE_AGRI_VEHICLES_CLASS_R,                   ///< Agricultural vehicles (Class R)
    ECALL_HAL_VEHITYPE_AGRI_VEHICLES_CLASS_S,                   ///< Agricultural vehicles (Class S)
    ECALL_HAL_VEHITYPE_AGRI_VEHICLES_CLASS_T,                   ///< Agricultural vehicles (Class T)
    ECALL_HAL_VEHITYPE_OFF_ROAD_VEHICLES_G,                     ///< Off-road vehicles (Class G)
    ECALL_HAL_VEHITYPE_SPECIAL_PURPOSE_MOTOR_CARAVAN_CLASS_SA,  ///< Special purpose motor caravan (Class SA)
    ECALL_HAL_VEHITYPE_SPECIAL_PURPOSE_ARMOURED_VEHICLE_CLASS_SB,///< Special purpose armoured vehicle (Class SB)
    ECALL_HAL_VEHITYPE_SPECIAL_PURPOSE_AMBULANCE_CLASS_SC,      ///< Special purpose ambulance (Class SC)
    ECALL_HAL_VEHITYPE_SPECIAL_PURPOSE_HEARSE_CLASS_SD,         ///< Special purpose hearse (Class SD)
    ECALL_HAL_VEHITYPE_OTHER_VEHICLE_CLASS                      ///< Other vehicle class
} taf_hal_eCall_VehicleType;

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_hal_eCall_VehicleType vehiType;
    char vin[MAX_VIN_BYTES+1];
    taf_hal_eCall_PropulsionType propulsionType;
} taf_hal_eCall_VehicleInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Activate type of eCall.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_HAL_ACTTYPE_UNKNOWN,    ///< Unknown
    ECALL_HAL_ACTTYPE_AUTOMATIC,  ///< Automatic
    ECALL_HAL_ACTTYPE_MANUAL      ///< Manual
} taf_hal_eCall_ActivateType;

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle location of impact per Euro NCAP.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_HAL_LOI_UNKNOWN,        ///< Unknown
    ECALL_HAL_LOI_NONE,           ///< None
    ECALL_HAL_LOI_FRONT,          ///< Front
    ECALL_HAL_LOI_REAR,           ///< Rear
    ECALL_HAL_LOI_DRIVERSIDE,     ///< Drive side
    ECALL_HAL_LOI_NONDRIVERSIDE,  ///< Non drive side
    ECALL_HAL_LOI_OTHER           ///< Other
} taf_hal_eCall_IILocations;

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle rollover detected information per Euro NCAP.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool rolloverDetectedPresent;
    bool rolloverDetected;
} taf_hal_eCall_RolloverDetected;

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle deltaV per Euro NCAP.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t rangeLimit;
    int16_t deltaVX;
    int16_t deltaVY;
} taf_hal_eCall_DeltaV;

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the eCall VHAL driver.
 * @param void
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle information of eCall MSD from the vehicle.
 * @param
 *      vehInfo        - Vehicle information
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETVEHICLEINFO)
(
    taf_hal_eCall_VehicleInfo* vehInfo
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle information of eCall MSD from the vehicle.
 * @param
 *      actType        - Activate type
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETACTIVATETYPE)
(
    taf_hal_eCall_ActivateType* actType
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the passenger count of eCall MSD from the vehicle.
 * @param
 *      passCount        - Passenger count
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETPASSENGERCOUNT)
(
    uint8_t* passCount
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the impact of location of eCall MSD from the vehicle.
 * @param
 *      iILocations        - Location of impact
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETIILOCATIONS)
(
    taf_hal_eCall_IILocations* iILocations
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the rollover detected of eCall MSD from the vehicle.
 * @param
 *      rollDetected        - Rollover detected information
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETROLLOVERDETECTED)
(
    taf_hal_eCall_RolloverDetected* rollDetected
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the deltaV of eCall MSD from the vehicle.
 * @param
 *      deltaV         - DeltaV information
 *
 * @return
 *      Result for getting the information
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_ECALL_GETDELTAV)
(
    taf_hal_eCall_DeltaV* deltaV
);

typedef struct
{
    INIT InitHAL;
    TAF_HAL_ECALL_GETVEHICLEINFO getVehicleInfo;
    TAF_HAL_ECALL_GETACTIVATETYPE getActivateType;
    TAF_HAL_ECALL_GETPASSENGERCOUNT getPassengerCount;
    TAF_HAL_ECALL_GETIILOCATIONS getIILocations;
    TAF_HAL_ECALL_GETROLLOVERDETECTED getRolloverDetected;
    TAF_HAL_ECALL_GETDELTAV getDeltaV;
} eCall_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Management interface for device manager
    eCall_Inf_t eCallInf;     ///< Module interface for application/service
} eCall_InfoTab_t;

extern eCall_InfoTab_t TAF_HAL_INFO_TAB;

#endif
