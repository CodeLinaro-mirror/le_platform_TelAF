/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFHALIF_HPP
#define TAFHALIF_HPP

#include <stdint.h>
#include <unistd.h>

#define TAF_HAL_NAME_MAX_LEN   50  // 50 bytes including null terminated
#define TAF_HAL_VERSION_MAX_LEN   10  // Format [xx].[yy]
#define TAF_HAL_VENDOR_NAME_MAX_LEN 50 // 50 bytes including null terminated_

#ifdef __cplusplus
extern "C" {
#endif

// API defintions
typedef int (*TAF_HAL_HWINIT)(void);
typedef void (*TAF_HAL_POWERON)(void);
typedef void (*TAF_HAL_POWEROFF)(void);

typedef void (*TAF_HAL_SLEEP)(void);
typedef void (*TAF_HAL_WAKEUP)(void);
typedef int32_t (*TAF_HAL_SELFTEST)(void);

typedef void* (*TAF_HAL_GETMODINF)(void);


#define TAF_HAL_INFO_TAB   tafHalInfoTab
#define TAF_HAL_INFO_TAB_STR   "tafHalInfoTab"


// !!! The module handle will be closed by device manager
// Make sure after return from those API, it is clean.
// No some memory objects left in the system
typedef struct {

    // module info
    const char* name;
    uint16_t    majorVer;
    uint16_t    minorVer;
    uint8_t     drvType;
    const char* vendor;

    uint8_t     serviceMax;

    // generic hal module interface
    TAF_HAL_HWINIT   hwInitInf;
    TAF_HAL_POWEROFF powerOffInf;
    TAF_HAL_POWERON  powerOnInf;

    // Self Test
    TAF_HAL_SELFTEST selfTest;

    //Get the specific interface
    TAF_HAL_GETMODINF getModInf;

    // Reserve
    uint8_t res[32];
}TAF_HAL_MGR_INF_t;


#ifdef __cplusplus
}
#endif

#endif
