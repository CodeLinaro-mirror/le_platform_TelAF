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
        .shutdownReqAsync = taf_hal_ShutdownReqAsync,
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

//--------------------------------------------------------------------------------------------------
/**
 * PM shutdown mode for request from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_SHUTDOWN_MODE_FORCEFUL, /**<Forceful shutdown */
    PM_HAL_SHUTDOWN_MODE_GRACEFUL  /**<Graceful shutdown */
} taf_hal_pm_ShutdownMode;

//--------------------------------------------------------------------------------------------------
/**
 * PM restart mode for request from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF /**<NAD shutdown for system restart */
} taf_hal_pm_RestartMode;

//--------------------------------------------------------------------------------------------------
/**
 * PM suspend mode for request from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_SUSPEND_MODE_FULL /**<Full suspend */
} taf_hal_pm_SuspendMode;

//--------------------------------------------------------------------------------------------------
/**
 * PM reason for response to service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_RSP_READY,    /**<Ready */
    PM_HAL_RSP_NOT_READY /**<Not ready */
} taf_hal_pm_RspReason;

//--------------------------------------------------------------------------------------------------
/**
 * PM node state for notification from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_NODE_STATE_UNKNOWN, /**<Unknown state */
    PM_HAL_NODE_STATE_RESUME,  /**<Resume */
    PM_HAL_NODE_STATE_SUSPEND, /**<Suspend */
    PM_HAL_NODE_STATE_SHUTDOWN,/**<Shutdown */
    PM_HAL_NODE_STATE_RESTART  /**<Restart */
} taf_hal_pm_NodeState;

//--------------------------------------------------------------------------------------------------
/**
 * PM node info for notification from service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_NODE_INFO_LOCK_ACQUIRED, /**<Wake source acquired */
    PM_HAL_NODE_INFO_LOCK_RELEASED  /**<Wake source released */
} taf_hal_pm_NodeInfo;

//--------------------------------------------------------------------------------------------------
/**
 * PM confirmation status for callback to service
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PM_HAL_NODE_STATUS_READY,    /**<Ready */
    PM_HAL_NODE_STATUS_NOT_READY /**<Not ready */
} taf_hal_pm_ConfirmStatus;

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
 * Callback for response of shutdown request.
 * @param
 *      mode    - corresonding shutdown mode to respond
 *      reason  - response to the request
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_SHUTDOWNRSPCALLBACK)
(
    taf_hal_pm_ShutdownMode mode,
    taf_hal_pm_RspReason reason
);

//--------------------------------------------------------------------------------------------------
/**
 * Shutdown request to the VHAL hardware component.
 * @param
 *      mode        - shutdown mode request to the VHAL compoment
 *      callback    - the callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_PM_SHUTDOWNREQASYNC)
(
    taf_hal_pm_ShutdownMode mode,
    TAF_HAL_PM_SHUTDOWNRSPCALLBACK callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Restart response callback function.
 * @param
 *      mode    - corresonding restart mode to respond
 *      reason  - response to the request
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_RESTARTRSPCALLBACK)
(
    taf_hal_pm_RestartMode mode,
    taf_hal_pm_RspReason reason
);

//--------------------------------------------------------------------------------------------------
/**
 * Restart request to the VHAL hardware component asynchronously.
 * @param
 *      mode        - restart mode request to the VHAL compoment
 *      callback    - the callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_PM_RESTARTREQASYNC)
(
    taf_hal_pm_RestartMode mode,
    TAF_HAL_PM_RESTARTRSPCALLBACK callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Suspend response callback function.
 * @param
 *      mode    - corresonding suspend mode to respond
 *      reason  - response to the request
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_SUSPENDRSPCALLBACK)
(
    taf_hal_pm_SuspendMode mode,
    taf_hal_pm_RspReason reason
);

//--------------------------------------------------------------------------------------------------
/**
 * Suspend request to the VHAL hardware component asynchronously.
 * @param
 *      mode        - suspend mode request to the VHAL compoment
 *      callback    - the callback function to response the request
 *
 * @return
 *      result for sending the request
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_PM_SUSPENDREQASYNC)
(
    taf_hal_pm_SuspendMode mode,
    TAF_HAL_PM_SUSPENDRSPCALLBACK  callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Node state change notification callback function.
 * @param
 *      pm_node_id    - corresonding node to respond
 *      state         - corresonding node state to respond
 *      status        - confirm status for the notification
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_NODESTATECHANGENOTIFICATIONCONFIRMCALLBACK)
(
    uint8_t pm_node_id,
    taf_hal_pm_NodeState state,
    taf_hal_pm_ConfirmStatus status
);

//--------------------------------------------------------------------------------------------------
/**
 * Node state change notification to the VHAL hardware component.
 * @param
 *      pm_node_id    - the notification is from which node
 *      state         - the state that the node is going to be
 *      callback      - the callback function to respond the request
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_NODE_STATECHANGE_NOTIFICATION)
(
    uint8_t pm_node_id,
    taf_hal_pm_NodeState state,
    TAF_HAL_PM_NODESTATECHANGENOTIFICATIONCONFIRMCALLBACK callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Node information notification to the VHAL hardware component.
 * @param
 *      pm_node_id    - the notification is from which node
 *      info          - the information that the node is sending to the VHAL compoment
 *      callback      - the callback function to respond the request
 *
 * @return
 *      result for sending the notification
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_NODEINFO_NOTIFICATION)
(
    uint8_t pm_node_id,
    taf_hal_pm_NodeInfo info,
    const char* vhalTag
);

//--------------------------------------------------------------------------------------------------
/**
 * Node event callback function.
 * @param
 *      pm_node_id          - corresonding node to respond
 *      pm_node_event_info  - corresonding node info to respond
 * @return void
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_PM_NODEEVENTCALLBACK)
(
    uint8_t pm_node_id,
    const char* pm_node_event_info
);

//--------------------------------------------------------------------------------------------------
/**
 * Add node event handler to VHAL component.
 * @param
 *      pm_node_id    - corresonding node to respond
 *      state         - corresonding node state to respond
 *      status        - confirm status for the notification
 * @return
 *      result for adding the handler
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_PM_ADDNODEEVENTHANDLER)
(
    uint8_t pm_node_id,
    TAF_HAL_PM_NODEEVENTCALLBACK callback
);

typedef struct
{
    INIT InitHAL;

    TAF_HAL_PM_SHUTDOWNREQASYNC shutdownReqAsync;

    TAF_HAL_PM_RESTARTREQASYNC restartReqAsync;

    TAF_HAL_PM_SUSPENDREQASYNC suspendReqAsync;

    TAF_HAL_PM_NODE_STATECHANGE_NOTIFICATION nodeStateChangeNotification;

    TAF_HAL_PM_NODEINFO_NOTIFICATION nodeInfoNotification;

    TAF_HAL_PM_ADDNODEEVENTHANDLER addNodeEventHandler;

} pm_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf;   // management interface for device manager
    pm_Inf_t pmInf;             // module interface for application/service

} pm_InfoTab_t;

extern pm_InfoTab_t TAF_HAL_INFO_TAB;

#endif
