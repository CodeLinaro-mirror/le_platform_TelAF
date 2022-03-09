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

#ifndef TAFAPPMGMT_HPP
#define TAFAPPMGMT_HPP

#include "legato.h"
#include "interfaces.h"

#include <vector>

#include "tafSvcIF.hpp"

#define TAF_APPMGMT_APP_LISTS_MAX_NUM 1
#define TAF_APPMGMT_APP_MAX_NUM 128

typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_AppMgmtAppInfoSafeRef_t;

typedef struct
{
    char name[TAF_APPMGMT_APP_NAME_BYTES];
    char version[TAF_APPMGMT_APP_VERSION_BYTES];
    char hash[TAF_APPMGMT_APP_HASH_BYTES];
    taf_appMgmt_AppState_t state;
    bool isStartManual;
    bool isSandboxed;
    le_sls_Link_t link;
} taf_AppMgmtAppInfo_t;

typedef struct
{
    le_sls_List_t appList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_AppMgmtAppList_t;

namespace telux {
namespace tafsvc {
    class taf_AppMgmt : public ITafSvc {
    public:
        taf_AppMgmt() {};
        ~taf_AppMgmt() {};

        static taf_AppMgmt &GetInstance();
        void Init(void);

        le_mem_PoolRef_t appListPool;
        le_mem_PoolRef_t appInfoPool;
        le_mem_PoolRef_t appInfoSafeRefPool;

        le_ref_MapRef_t appListRefMap;
        le_ref_MapRef_t appInfoSafeRefMap;
    };
}
}

#endif
