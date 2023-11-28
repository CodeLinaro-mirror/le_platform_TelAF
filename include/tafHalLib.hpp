/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAFHALCOMP_H__
#define __TAFHALCOMP_H__

#ifdef __cplusplus
extern "C"
{
#endif

    // support load by name only or by name and version
    // return driver interface if successfully
    // a local object was created which include all the interface provide by so
    void* taf_devMgr_LoadDrv(const char* drvName, const char* drvVer);

    // free the interface
    bool taf_devMgr_UnloadDrv(void* handle);

#ifdef __cplusplus
}
#endif

#endif
