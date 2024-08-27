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
 * The audio_InfoTab_t TAF_HAL_INFO_TAB is the entry used by device manager and services.
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
        .CtlReportBubStatus = taf_hal_CtlReportBubStatus,
        .GetNodeType = taf_hal_GetNodeType,
        .SendNodeVendorConfig = taf_hal_SendNodeVendorConfig,
        .SetNodePowerState = taf_hal_SetNodePowerState,
        .GetNodePowerState = taf_hal_GetNodePowerState,
        .SetNodeMuteState = taf_hal_SetNodeMuteState,
        .GetNodeMuteState = taf_hal_GetNodeMuteState,
        .SetNodeGain = taf_hal_SetNodeGain,
        .GetNodeGain = taf_hal_GetNodeGain,
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
    HAL_AUDIO_MODE_VOICE_CALL, /**<Voice call */
    HAL_AUDIO_MODE_PLAYBACK,   /**<Playback */
    HAL_AUDIO_MODE_RECORDING,  /**<Recording */
    HAL_AUDIO_MODE_LOOPBACK    /**<Loopback */
} hal_audio_Mode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_AUDIO_NODE_TYPE_INVALID = -1, /**<Invalid */
    HAL_AUDIO_NODE_TYPE_CODEC,        /**<Codec */
    HAL_AUDIO_NODE_TYPE_PA,           /**<PA */
    HAL_AUDIO_NODE_TYPE_A2B,          /**<A2B */
    HAL_AUDIO_NODE_TYPE_MAX           /**<MAX */
} hal_audio_NodeType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device events
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_AUDIO_EVENT_MUTE,   /**<Mute */
    HAL_AUDIO_EVENT_UNMUTE  /**<Unmute */
} hal_audio_DevEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device power states
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_AUDIO_POWER_STATE_OFF,        /**<Power off */
    HAL_AUDIO_POWER_STATE_SUSPEND,    /**<Suspend */
    HAL_AUDIO_POWER_STATE_ACTIVE      /**<Active */
} hal_audio_PowerState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Backup battery (BuB) states
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_AUDIO_BUB_STATUS_NOT_IN_USE,    /**<BuB not in use */
    HAL_AUDIO_BUB_STATUS_IN_USE         /**<BuB in use */
} hal_audio_bubStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio device directions
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_AUDIO_DIRECTION_RX,    /**<Audio node direction RX */
    HAL_AUDIO_DIRECTION_TX     /**<Audio node direction TX */
} hal_audio_direction_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Audio VHAL driver.
 * @param void
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_audio_InitFunc_t)(void);

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
typedef le_result_t (*hal_audio_CtlSetAudioStatus_t)
(
    bool isActive,
    uint32_t route, hal_audio_Mode_t mode
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
typedef le_result_t (*hal_audio_SendVendorConfig_t)
(
    const char* config
);

//--------------------------------------------------------------------------------------------------
/** Gets audio device type of the requested audio node.
 * @param
 *      nodeId      - Audio device node ID
 *      nodeType    - Node type
 *
 * @return
 *      Type of audio device.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_audio_GetNodeType_t)
(
    uint8_t nodeId,
    hal_audio_NodeType_t *nodeType
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
typedef le_result_t (*hal_audio_SendNodeVendorConfig_t)
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
typedef le_result_t (*hal_audio_SetNodePowerState_t)
(
    uint8_t nodeId,
    hal_audio_PowerState_t state
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
typedef le_result_t (*hal_audio_GetNodePowerState_t)
(
    uint8_t nodeId,
    hal_audio_PowerState_t *state
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
typedef le_result_t (*hal_audio_SetNodeMuteState_t)
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
typedef le_result_t (*hal_audio_GetNodeMuteState_t)
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
typedef void (*hal_audio_DevStateChangeCallback_t)
(
    uint8_t nodeId,
    hal_audio_DevEvent_t audio_device_event
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
typedef le_result_t (*hal_audio_AddNodeStateChangeHandler_t)
(
    uint8_t nodeId,
    hal_audio_DevStateChangeCallback_t callback
);

//--------------------------------------------------------------------------------------------------
/** Reports BuB status when audio is active and on change in BuB status.
 * @param
 *      bubStatus      - BuB status
 *
 * @return
 *      result for reporing BuB status
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_audio_CtlReportBubStatus)
(
    hal_audio_bubStatus_t bubStatus
);

//--------------------------------------------------------------------------------------------------
/** Sets the gain to the audio device.
 * @param
 *      nodeId      - Audio device node ID
 *      direction   - Audio device direction to set gain
 *      gain        - Gain percentage ranged from 0 to 1
 *
 * @return
 *      Result of setting gain to the audio device.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_audio_SetNodeGain_t)
(
    uint8_t nodeId,
    hal_audio_direction_t direction,
    double gain
);

//--------------------------------------------------------------------------------------------------
/** Gets the gain of the audio device.
 * @param
 *      nodeId      - Audio device node ID
 *      direction   - Audio device direction to get gain
 *      gain        - Gain percentage ranged from 0 to 1
 *
 * @return
 *      Result of getting gain of the audio device.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_audio_GetNodeGain_t)
(
    uint8_t nodeId,
    hal_audio_direction_t direction,
    double *gain
);

typedef struct
{
    hal_audio_InitFunc_t InitHAL;
    hal_audio_CtlSetAudioStatus_t CtlSetAudioStatus;
    hal_audio_CtlReportBubStatus CtlReportBubStatus;
    hal_audio_SendVendorConfig_t SendVendorConfig;
    hal_audio_GetNodeType_t GetNodeType;
    hal_audio_SendNodeVendorConfig_t SendNodeVendorConfig;
    hal_audio_SetNodePowerState_t SetNodePowerState;
    hal_audio_GetNodePowerState_t GetNodePowerState;
    hal_audio_SetNodeMuteState_t SetNodeMuteState;
    hal_audio_GetNodeMuteState_t GetNodeMuteState;
    hal_audio_SetNodeGain_t SetNodeGain;
    hal_audio_GetNodeGain_t GetNodeGain;
    hal_audio_AddNodeStateChangeHandler_t AddNodeStateChangeHandler;
} hal_audio_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; // for device manager
    hal_audio_Inf_t audioInf; // for app/service

} hal_audio_InfoTab_t;

extern hal_audio_InfoTab_t TAF_HAL_INFO_TAB;

#endif
