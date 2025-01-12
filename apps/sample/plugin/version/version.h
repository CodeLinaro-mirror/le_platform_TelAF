/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define MAX_LINE_LEN 1024

#define BOOT_VERSION_FILE "/proc/version"
#define BOOT_VERSION_PREFIX "Linux version "

#define FIRMWARE_VERSION_FILE "/firmware/image/Ver_Info.txt"
#define FIRMWARE_VERSION_PREFIX "MPSS.DE."
#define TZ_VERSION_PREFIX "TZ.XF."

#define ROOTFS_VERSION_FILE "/etc/version"
#define ROOTFS_VERSION_AU_PREFIX "TARGET_ALL."
#define ROOTFS_VERSION_LE_PREFIX "LE.UM."

#define TELAF_VERSION_FILE "/legato/systems/current/version"
#define TELAF_VERSION_PREFIX "telaf.lnx.1.1-"
#define TELAF_VERSION_MAJOR_LEN 2
#define TELAF_VERSION_MINOR_LEN 2
#define TELAF_VERSION_PATCH_LEN 2

#define LXC_VERSION_FILE "/lxcrootfs/etc/version"
#define LXC_VERSION_AU_PREFIX "TARGET_ALL."
#define LXC_VERSION_LE_PREFIX "LE.UM."

//--------------------------------------------------------------------------------------------------
/**
 * Version tier enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_VERSION_MAJOR,
    TAF_PI_VERSION_MINOR,
    TAF_PI_VERSION_PATCH
} taf_pi_version_Tier_t;