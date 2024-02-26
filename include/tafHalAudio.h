/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @page c_tafAudioVhal Audio VHAL
 *
 * @ref tafHalAudio.h "API Reference"
 *
 * <HR>
 *
 * To build a driver using Audio VHAL, the developer needs to include this VHAL
 * module header file with the driver component.
 *
 * @section audioVhal_impl Implement VHAL in compliance with VHAL APIs
 *
 * To implement a VHAL for a Audio hardware module, the developer needs to get this file. The header
 * tafHalAudio.h provides all driver management interfaces, which requires the driver information that
 * will be checked by the device manager during driver installation. The driver information includes
 * the driver’s name, version type, and initialization functions for the device.
 *
 * The pm_InfoTab_t TAF_HAL_INFO_TAB is the entry used by device manager and services.
 *
 * @section audioVhal_def Define VHAL management interfaces
 *
 * In the context of Audio VHAL, the Audio driver needs to include tafHalAudio.h and implement the
 * functions defined in the header file. The Audio VHAL component defines the audio_InfoTab_t with the
 * actual implementation, including the driver management information and functional interfaces.
 * The following is an example of the definition of management interfaces.
 *
 * @code

        .mgrInf = {
        .name = TAF_AUDIO_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

 * @endcode
 *
 * - name -- Use the corresponding module name macro define in tafHalAudio.h.
 * - majorVer -- Major version of the driver.
 * - minorVer -- Minor version of the driver.
 * - vendor -- Vendor name of the driver.
 * - hwInitInf -- Hardware initialization function after installation.
 * - powerOffInf -- Power off function called by the device manager during uninstallation.
 * - powerOnInf -- Power on function called by the device manager during installation.
 * - selfTest -- Self-test function to check if the driver functions work normally (reserved for future).
 * - getModInf -- Function to get the address of the functional interface.
 *
 * The following is an example of the definition of Audio functional interfaces.
 *
 * @code

    .audioInf = {
        .InitHAL = Init,
        .CtlSetAudioStatus = taf_hal_CtlSetAudioStatus,
    }

 * @endcode
 *
 * <HR>
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef __AUDIO_H__

#define __AUDIO_H__

#include "tafHalIF.hpp"

// Define the name or ID for HAL module
#define TAF_AUDIO_MODULE_NAME "TafHalAudio"

//--------------------------------------------------------------------------------------------------
/**
 * Audio mode type
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AUDIO_HAL_MODE_VOICE_CALL,
    AUDIO_HAL_MODE_RESERVED,
    AUDIO_HAL_MODE_PLAYBACK,
    AUDIO_HAL_MODE_RECORDING,
    AUDIO_HAL_MODE_LOOPBACK
} taf_hal_audio_Mode;

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Audio VHAL driver.
 * @param void
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*INIT)(void);

//--------------------------------------------------------------------------------------------------
/** set audio status for route and mode
 * @param
 *      status      - 0 - inactive, 1 - active
 *      route       - RouteId
 *      mode        - mode of stream
 *
 * @return
 *      result for setting audio status
 */
//--------------------------------------------------------------------------------------------------

typedef le_result_t (*TAF_HAL_CTLSETAUDIOSTATUS)(bool status,
    uint32_t route, taf_hal_audio_Mode mode);

typedef struct
{
    INIT InitHAL;
    TAF_HAL_CTLSETAUDIOSTATUS CtlSetAudioStatus;

} audio_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; // for device manager
    audio_Inf_t audioInf; // for app/service

} audio_InfoTab_t;

extern audio_InfoTab_t TAF_HAL_INFO_TAB;

#endif