/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** @file devManager.hpp
 *
 * This file contains device manager control interface specfication.
 *
 *
 */

#ifndef DEV_MANAGER_INCLUDE_GUARD
#define DEV_MANAGER_INCLUDE_GUARD

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
/**
 * The maximum length for driver information
 */
//--------------------------------------------------------------------------------------------------
#define DEV_MANAGER_DRIVER_NAME_MAX_LEN         128
#define DEV_MANAGER_DRIVER_VERSION_MAX_LEN      10
#define DEV_MANAGER_DRIVER_VENDOR_MAX_LEN       20
#define DEV_MANAGER_DRIVER_LOCATION_MAX_LEN     256

//--------------------------------------------------------------------------------------------------
/**
 * The maximum packet size in bytes of device manager commands.
 */
//--------------------------------------------------------------------------------------------------
#define DEV_MANAGER_MAX_CMD_PACKET_BYTES        500

//--------------------------------------------------------------------------------------------------
/**
 * The maximum message size in bytes of device manager responses to commands.
 */
//--------------------------------------------------------------------------------------------------
#define DEV_MANAGER_MAX_RESP_MSG_BYTES          256

//--------------------------------------------------------------------------------------------------
/**
 * The device manager protocol string.
 */
//--------------------------------------------------------------------------------------------------
#define DEV_MANAGER_PROTOCOL_ID     "DeviceManagerProtocol"

//--------------------------------------------------------------------------------------------------
/**
 * The device manager service instance name.
 */
//--------------------------------------------------------------------------------------------------

#define DEV_MANAGER_SERVICE_NAME    "DeviceManager"     // interface to services
#define DEV_MANAGER_TOOL_NAME       "DeviceManagerTool" // interface to tools


// =====================================
//  COMMANDS
// =====================================

//--------------------------------------------------------------------------------------------------
/**
 * messages or commands that can be sent from the client tool to the device manager daemon
 */
//--------------------------------------------------------------------------------------------------

#define DEV_MANAGER_TOOL_INSTALL_CMD           'i' // install a driver
#define DEV_MANAGER_TOOL_REMOVE_CMD            'r' // remove a driver

// For the services who will use the driver
#define DEV_MANAGER_SERVICE_OPEN_CMD           'o' // open the driver
#define DEV_MANAGER_SERVICE_CLOSE_CMD          'c' // close the device driver

//--------------------------------------------------------------------------------------------------
/**
 * device manager commands that can be sent from the log tool to the log daemon only.
 */
//--------------------------------------------------------------------------------------------------
#define DEV_MANAGER_TOOL_LIST_DRIVERS          'l' // list the installed drivers
#define DEV_MANAGER_TOOL_QUERY_DRIVERS         'q' // query the status of the driver
#define DEV_MANAGER_TOOL_RUN_SELFTEST          't' // query the status of the driver


#define DEV_MANAGER_DRV_HW_ERROR    -1
#define DEV_MANAGER_DRV_IDLE        0 // just installed
#define DEV_MANAGER_DRV_BUSY        1 // service open it
#define DEV_MANAGER_DRV_STANDBY     2 // After test ok, ready to server

#ifdef __cplusplus
}
#endif

#endif // LOG_DAEMON_INCLUDE_GUARD
