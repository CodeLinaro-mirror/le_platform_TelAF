/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tafAppMgmt.hpp"

using namespace telux::tafsvc;

/*======================================================================
 FUNCTION        taf_AppMgmt::GetInstance
 DESCRIPTION     Get a instance of taf_AppMgmt
 PARAMETERS      void
 RETURN VALUE    taf_AppMgmt: Instance reference
======================================================================*/
taf_AppMgmt &taf_AppMgmt::GetInstance()
{
    static taf_AppMgmt instance;
    return instance;
}

/* Pool and map for app list and app reference. */
LE_MEM_DEFINE_STATIC_POOL(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM, sizeof(taf_AppMgmtAppList_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_appMgmt_AppInfo_t));
LE_MEM_DEFINE_STATIC_POOL(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_AppMgmtAppInfoSafeRef_t));
LE_REF_DEFINE_STATIC_MAP(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
LE_REF_DEFINE_STATIC_MAP(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);

/*======================================================================
 FUNCTION        taf_AppMgmt::Init
 DESCRIPTION     Initialization of the app management component
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_AppMgmt::Init(void)
{
    // 1. Connect to supervisor services.
    le_cfg_ConnectService();

    le_appInfo_ConnectService();
    le_appCtrl_ConnectService();
    le_appRemove_ConnectService();

    le_updateCtrl_ConnectService();

    // 2. Initiate the memory pool
    appListPool = le_mem_InitStaticPool(appListPool, TAF_APPMGMT_APP_LISTS_MAX_NUM, sizeof(taf_AppMgmtAppList_t));
    appInfoPool = le_mem_InitStaticPool(appInfoPool, TAF_APPMGMT_APP_MAX_NUM, sizeof(taf_appMgmt_AppInfo_t));
    appInfoSafeRefPool = le_mem_InitStaticPool(appInfoSafeRefPool, TAF_APPMGMT_APP_MAX_NUM,
        sizeof(taf_AppMgmtAppInfoSafeRef_t));

    // 3. Initiate the reference map.
    appListRefMap = le_ref_InitStaticMap(appListRefMap, TAF_APPMGMT_APP_LISTS_MAX_NUM);
    appInfoSafeRefMap = le_ref_InitStaticMap(appInfoSafeRefMap, TAF_APPMGMT_APP_MAX_NUM);
}
