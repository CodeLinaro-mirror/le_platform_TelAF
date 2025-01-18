/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @page c_tafPMVhal Power Management VHAL
 *
 * @ref tafHalPM.h "API Reference"
 *
 * <HR>
 *
 * To build a driver using Power Management (PM) VHAL, the developer needs to include this VHAL
 * module header file with the driver component.
 *
 * @section pmVhal_impl Implement VHAL in compliance with VHAL APIs
 *
 * To implement a VHAL for a PM hardware module, the developer needs to get this file. The header
 * tafHalPM.h provides all driver management interfaces, which requires the driver information that
 * will be checked by the device manager during driver installation. The driver information includes
 * the driver’s name, version type, and initialization functions for the device.
 *
 * The pm_InfoTab_t TAF_HAL_INFO_TAB is the entry used by device manager and services.
 *
 * @section pmVhal_def Define VHAL management interfaces
 *
 * In the context of PM VHAL, the PM driver needs to include tafHalPM.h and implement the functions
 * defined in the header file. The PM VHAL component defines the pm_InfoTab_t with the actual
 * implementation, including the driver management information and functional interfaces.
 * The following is an example of the definition of management interfaces.
 *
 * @code

    .mgrInf = {
        .name = TAF_PM_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    }

 * @endcode
 *
 * - name -- Use the corresponding module name macro define in tafHalPM.h.
 * - majorVer -- Major version of the driver.
 * - minorVer -- Minor version of the driver.
 * - vendor -- Vendor name of the driver.
 * - hwInitInf -- Hardware initialization function after installation.
 * - powerOffInf -- Power off function called by the device manager during uninstallation.
 * - powerOnInf -- Power on function called by the device manager during installation.
 * - selfTest -- Self-test function to check if the driver functions work normally (reserved for future).
 * - getModInf -- Function to get the address of the functional interface.
 *
 * The following is an example of the definition of PM functional interfaces.
 *
 * @code

    .pmInf = {
        .InitHAL = Init,
        .shutdownReqAsync = taf_hal_ShutDownReqAsync,
        .restartReqAsync = taf_hal_RestartReqAsync,
        .nodeStateChangeNotification = taf_hal_NodeStateChangeNotification,
    }

 * @endcode
 *
 * <HR>
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef __PM_H__

#define __PM_H__

#include "tafHalIF.hpp"

// Define the name or ID for HAL module
#define TAF_PM_MODULE_NAME "TafHalPowerManagement"

//Define the vehichle wakeup reason
#define HAL_PM_VEHICHLE_WAKEUP_REASON_DEFAULT 0

//Define the vehichle wakeup status
#define HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE 0
#define HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ 1
#define HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN 2

//Define the Bub status
#define HAL_PM_BUB_STATUS_UNKNOWN -1
#define HAL_PM_BUB_STATUS_NOT_IN_USE 0
#define HAL_PM_BUB_STATUS_IN_USE 1

//--------------------------------------------------------------------------------------------------
/**
 * Power Mode request from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_PM_SHUTDOWN_MODE_NORMAL,               /**<Normal shutdown */
    HAL_PM_SHUTDOWN_MODE_GRACEFUL,             /**<Graceful shutdown */
    HAL_PM_RESTART_MODE_NAD_REBOOT,            /**<NAD reboot for system restart */
    HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, /**<NAD shutdown for system restart */
    HAL_PM_SUSPEND_MODE_FULL                   /**<Full suspend */
} hal_pm_PowerMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * PM reason for response to service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_PM_RSP_READY,             /**<Ready */
    HAL_PM_RSP_NOT_READY,         /**<Not ready */
    HAL_PM_RSP_TIMEOUT,           /**Timeout */
    HAL_PM_RSP_INVALID_REQUEST    /** Invalid Request */
} hal_pm_RspReason_t;

//--------------------------------------------------------------------------------------------------
/**
 * PM node state for notification from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_PM_NODE_STATE_UNKNOWN, /**<Unknown state */
    HAL_PM_NODE_STATE_RESUME,  /**<Resume */
    HAL_PM_NODE_STATE_SUSPEND, /**<Suspend */
    HAL_PM_NODE_STATE_SHUTDOWN,/**<Shutdown */
    HAL_PM_NODE_STATE_RESTART  /**<Restart */
} hal_pm_NodeState_t;

//--------------------------------------------------------------------------------------------------
/**
 * PM node info for notification from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_PM_NODE_INFO_LOCK_ACQUIRED, /**<Wake source acquired */
    HAL_PM_NODE_INFO_LOCK_RELEASED  /**<Wake source released */
} hal_pm_NodeInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * PM confirmation status for callback to service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HAL_PM_NODE_STATUS_READY,    /**<Ready */
    HAL_PM_NODE_STATUS_NOT_READY /**<Not ready */
} hal_pm_ConfirmStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the PM VHAL driver.
 * @param void
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Callback for response of NodeStateChangePrepare request.
 *
 *  @param    pmNodeId    nodeid for a given node
 *  @param    state       State of the given node
 *  @param    mode        Corresonding shutdown mode to respond
 *  @param    StateChangeReason     State Change request Reason
 *  @param    reason      Response to the request
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeStateChangePrepareCallbackFunc_t)
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    const uint8_t StateChangeReason,
    hal_pm_RspReason_t reason
);

//--------------------------------------------------------------------------------------------------
/**
 * NodeStateChangeShutdownPrepareAsync request to the VHAL hardware component.
 *
 *  @param    pmNodeId    nodeid for a given node
 *  @param    state       State of the given node
 *  @param    mode        Shutdown mode request to the VHAL compoment
 *  @param    reason      shutdown/restart reason.
 *  @param    callback    Callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_NodeStateChangePrepareAsync)(
   uint8_t pmNodeId,
   hal_pm_NodeState_t state,
   hal_pm_PowerMode_t mode,
   const uint8_t reason,
   hal_pm_NodeStateChangePrepareCallbackFunc_t callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback for response of NodeStateChange request.
 *
 *  @param    pmNodeId    nodeid for a given node
 *  @param    state       State of the given node
 *  @param    mode        Corresonding shutdown mode to respond
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeStateChangeReqCallbackFunc_t)
(
uint8_t pmNodeId,
hal_pm_NodeState_t state,
hal_pm_PowerMode_t mode
);
//--------------------------------------------------------------------------------------------------
/**
 * NodeStateChange request to the VHAL hardware component.
 *
 *  @param    pmNodeId    nodeid for a given node
 *  @param    state       State of the given node
 *  @param    mode        Shutdown mode request to the VHAL compoment
 *  @param    callback    Callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_NodeStateChangeReqAsync)(

   uint8_t pmNodeId,

   hal_pm_NodeState_t state,

   hal_pm_PowerMode_t mode,

   hal_pm_NodeStateChangeReqCallbackFunc_t callback

);

//--------------------------------------------------------------------------------------------------
/**
 * Suspend response callback function.
 *
 *  @param    mode    Corresonding suspend mode to respond
 *  @param    reason  Response to the request
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_WakeupVehicleRspCallbackFunc_t)
(
    int32_t reason,
    int32_t response
);

//--------------------------------------------------------------------------------------------------
/**
 * Suspend request to the VHAL hardware component asynchronously.
 *
 *  @param    mode        Suspend mode request to the VHAL compoment
 *  @param    callback    Callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_WakeupVehicleReqAsyncFunc_t)
(
    int32_t reason,
    hal_pm_WakeupVehicleRspCallbackFunc_t  callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Node state change notification callback function.
 *
 *  @param    pm_node_id    Corresonding node to respond
 *  @param    state         Corresonding node state to respond
 *  @param    status        Confirm status for the notification
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeStateChangeNotificationConfirmCallbackFunc_t)
(
    uint8_t pm_node_id,
    hal_pm_NodeState_t state,
    hal_pm_ConfirmStatus_t status
);

//--------------------------------------------------------------------------------------------------
/**
 * Node state change notification to the VHAL hardware component.
 *
 *  @param    pm_node_id    Notification is from which node
 *  @param    state         State that the node is going to be
 *  @param    callback      Callback function to respond the request
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeStateChangeNotificationFunc_t)
(
    uint8_t pm_node_id,
    hal_pm_NodeState_t state,
    hal_pm_NodeStateChangeNotificationConfirmCallbackFunc_t callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Node information notification to the VHAL hardware component.
 *
 *  @param    pm_node_id    Notification is from which node
 *  @param    info          Information that the node is sending to the VHAL compoment
 *  @param    callback      Callback function to respond the request
 *
 * @return
 *      result for sending the notification
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeInfoNotificationFunc_t)
(
    uint8_t pm_node_id,
    hal_pm_NodeInfo_t info,
    const uint8_t reason
);

//--------------------------------------------------------------------------------------------------
/**
 * Node event callback function.
 *
 *  @param    pm_node_id          Corresonding node to respond
 *  @param    pm_node_event_info  Corresonding node info to respond
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_NodeEventCallbackFunc_t)
(
    uint8_t pm_node_id,
    const char* pm_node_event_info
);

//--------------------------------------------------------------------------------------------------
/**
 * Add node event handler to VHAL component.
 *
 *  @param    pm_node_id    Corresonding node to respond
 *  @param    state         Corresonding node state to respond
 *  @param    status        Confirm status for the notification
 * @return
 *      result for adding the handler
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_AddNodeEventHandlerFunc_t)
(
    uint8_t pm_node_id,
    hal_pm_NodeEventCallbackFunc_t callback
);
//--------------------------------------------------------------------------------------------------
/**
 * Bub status callback function.
 * @param
 *      status    Bub status pointer
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*hal_pm_AddBubStatusCallbackFunc_t)
(
   int32_t *status
);
//--------------------------------------------------------------------------------------------------
/**
 * Add Bub status handler to VHAL component.
 * @param
 *      handlerRef      Bub status handler callback function
 * @return
 *      result for adding the handler
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_AddBubStatusHandlerFunc_t)
(
    hal_pm_AddBubStatusCallbackFunc_t handler
);
//--------------------------------------------------------------------------------------------------
/**
 * BuB status query to the VHAL hardware component.
 * @param
 *      status    Bub status pointer
 *
 * @return
 *      result for sending the notification
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*hal_pm_GetBubStatusFunc_t)
(
    int32_t *status
);

typedef struct
{
    INIT InitHAL;

    hal_pm_NodeStateChangeNotificationFunc_t nodeStateChangeNotification;

    hal_pm_NodeInfoNotificationFunc_t nodeInfoNotification;

    hal_pm_AddNodeEventHandlerFunc_t addNodeEventHandler;

    hal_pm_WakeupVehicleReqAsyncFunc_t wakeupVehicleReqAsync;

    hal_pm_NodeStateChangePrepareAsync nodeStateChangePrepareAsync;

    hal_pm_NodeStateChangeReqAsync nodeStateChangeReqAsync;

    hal_pm_AddBubStatusHandlerFunc_t addBubStatusHandler;

    hal_pm_GetBubStatusFunc_t getBubStatus;

} hal_pm_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf;   // management interface for device manager
    hal_pm_Inf_t pmInf;             // module interface for application/service

} hal_pm_InfoTab_t;

extern hal_pm_InfoTab_t TAF_HAL_INFO_TAB;

#endif
