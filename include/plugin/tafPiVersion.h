/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAF_PI_VERSION_H__
#define __TAF_PI_VERSION_H__

#include "tafHalIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Version plugin module name.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_VERSION_MODULE_NAME "TafPiVersion"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes in version.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_PI_VERSION_MAJOR_MAX_BYTES 8
#define TAF_PI_VERSION_MINOR_MAX_BYTES 8
#define TAF_PI_VERSION_PATCH_MAX_BYTES 8

//--------------------------------------------------------------------------------------------------
/**
 * Version format bitmask.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_PI_VERSION_FORMAT_BASE 0x1
#define TAF_PI_VERSION_FORMAT_MAJOR_MINOR_PATCH 0x2
#define TAF_PI_VERSION_FORMAT_HASH 0x4

//--------------------------------------------------------------------------------------------------
/**
 * Component enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_VERSION_COMP_BOOT,
    TAF_PI_VERSION_COMP_ROOTFS,
    TAF_PI_VERSION_COMP_FIRMWARE,
    TAF_PI_VERSION_COMP_TZ,
    TAF_PI_VERSION_COMP_TELAF,
    TAF_PI_VERSION_COMP_LXC
} taf_pi_version_Comp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize version plugin module.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_VERSION_INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Get version format.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
typedef uint32_t (*TAF_PI_VERSION_GET_FORMAT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Get revision.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_VERSION_GET_REVISION)(taf_pi_version_Comp_t component, char* versionPtr,
    size_t versionSize);

//--------------------------------------------------------------------------------------------------
/**
 * Version plugin information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Initialization.
    TAF_PI_VERSION_INIT init;

    // Get version format.
    TAF_PI_VERSION_GET_FORMAT getFormat;

    // Get major, minor, patch version.
    TAF_PI_VERSION_GET_REVISION getMajor;
    TAF_PI_VERSION_GET_REVISION getMinor;
    TAF_PI_VERSION_GET_REVISION getPatch;

} version_Inf_t;

//--------------------------------------------------------------------------------------------------
/**
 * Version plugin information table structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Device manager information.
    version_Inf_t versionInf; ///< Version plugin information.
} version_InfoTab_t;

#endif