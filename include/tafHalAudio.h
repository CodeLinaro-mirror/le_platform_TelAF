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
        .SendVendorConfig = taf_hal_SendVendorConfig,
        .GetNodeType = taf_hal_GetNodeType,
        .SendNodeVendorConfig = taf_hal_SendNodeVendorConfig,
        .SetNodePowerState = taf_hal_SetNodePowerState,
        .GetNodePowerState = taf_hal_GetNodePowerState,
        .SetNodeMuteState = taf_hal_SetNodeMuteState,
        .GetNodeMuteState = taf_hal_GetNodeMuteState,
        .AddNodeStateChangeHandler = taf_hal_AddNodeStateChangeHandler,
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
 * Audio device types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AUDIO_HAL_NODE_INVALID = -1,
    AUDIO_HAL_NODE_CODEC,
    AUDIO_HAL_NODE_PA,
    AUDIO_HAL_NODE_A2B,
    AUDIO_HAL_NODE_MAX
} taf_hal_audio_NodeType;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device events
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AUDIO_HAL_MUTE,
    AUDIO_HAL_UNMUTE
} taf_hal_audio_DevEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device power states
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    POWER_OFF,
    SUSPEND,
    ACTIVE
} taf_hal_audio_Powerstate;

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
typedef le_result_t (*TAF_HAL_CTLSETAUDIOSTATUS)
(
    bool isActive,
    uint32_t route, taf_hal_audio_Mode mode
);

//--------------------------------------------------------------------------------------------------
/** Vendor configuration from app to vendor to perform customized operations.
 * @param
 *      config      - Vendor configuration
 *
 * @return
 *      Result for sending vendor configuration.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_SENDVENDORCONFIG)
(
    const char* config
);

//--------------------------------------------------------------------------------------------------
/** Gets audio device type of the requested audio node.
 * @param
 *      nodeId      - Audio device node ID
 *
 * @return
 *      Type of audio device.
 */
//--------------------------------------------------------------------------------------------------
typedef taf_hal_audio_NodeType (*TAF_HAL_AUDIO_GETNODETYPE)
(
    uint8_t nodeId
);

//--------------------------------------------------------------------------------------------------
/** Sends the audio device configuration to vendor for customized operations.
 * @param
 *      nodeId      - Audio device node ID
 *      config      - Audio device configuration
 *
 * @return
 *      Result of sending audio device configuration to vendor.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_SENDNODEVENDORCONFIG)
(
    uint8_t nodeId,
    const char* config
);

//--------------------------------------------------------------------------------------------------
/** Sets the audio device power state.
 * @param
 *      nodeId      - Audio device node ID
 *      state       - Power state to be set
 *
 * @return
 *      Result of setting audio device power state.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_SETNODEPOWERSTATE)
(
    uint8_t nodeId,
    taf_hal_audio_Powerstate state
);

//--------------------------------------------------------------------------------------------------
/** Gets the audio device power state.
 * @param
 *      nodeId      - Audio device node ID
 *      state       - Power state
 *
 * @return
 *      Result of getting audio device power state.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_GETNODEPOWERSTATE)
(
    uint8_t nodeId,
    taf_hal_audio_Powerstate *state
);

//--------------------------------------------------------------------------------------------------
/** Sets the audio device mute status.
 * @param
 *      nodeId      - Audio device node ID
 *      mute        - True to mute, false to unmute
 *
 * @return
 *      Result of setting audio device mute status.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_SETNODEMUTESTATE)
(
    uint8_t nodeId,
    bool mute
);

//--------------------------------------------------------------------------------------------------
/** Gets the audio device mute status.
 * @param
 *      nodeId      - Audio device node ID
 *      isMuted     - Mute status
 *
 * @return
 *      Result of getting mute status of the audio device.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_GETNODEMUTESTATE)
(
    uint8_t nodeId,
    bool *isMuted
);

//--------------------------------------------------------------------------------------------------
/**
 * Device state changed event callback function.
 * @param
 *      nodeId              - Corresponding node to respond
 *      audio_device_event  - Corresponding node information to respond
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_AUDIO_DEVSTATECHANGECALLBACK)
(
    uint8_t nodeId,
    taf_hal_audio_DevEvent audio_device_event
);

//--------------------------------------------------------------------------------------------------
/**
 * Adds node state change event handler.
 * @param
 *      nodeId   - Corresponding node to respond
 *      callback - Node state change handler callback
 * @return
 *      Result for adding the handler
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_AUDIO_ADDNODESTATECHANGEHANDLER)
(
    uint8_t nodeId,
    TAF_HAL_AUDIO_DEVSTATECHANGECALLBACK callback
);

typedef struct
{
    INIT InitHAL;
    TAF_HAL_CTLSETAUDIOSTATUS CtlSetAudioStatus;
    TAF_HAL_AUDIO_SENDVENDORCONFIG SendVendorConfig;
    TAF_HAL_AUDIO_GETNODETYPE GetNodeType;
    TAF_HAL_AUDIO_SENDNODEVENDORCONFIG SendNodeVendorConfig;
    TAF_HAL_AUDIO_SETNODEPOWERSTATE SetNodePowerState;
    TAF_HAL_AUDIO_GETNODEPOWERSTATE GetNodePowerState;
    TAF_HAL_AUDIO_SETNODEMUTESTATE SetNodeMuteState;
    TAF_HAL_AUDIO_GETNODEMUTESTATE GetNodeMuteState;
    TAF_HAL_AUDIO_ADDNODESTATECHANGEHANDLER AddNodeStateChangeHandler;
} audio_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; // for device manager
    audio_Inf_t audioInf; // for app/service

} audio_InfoTab_t;

extern audio_InfoTab_t TAF_HAL_INFO_TAB;

#endif