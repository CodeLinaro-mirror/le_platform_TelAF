/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDataIDSvr.hpp"
#include "tafSecuritySvr.hpp"
#include "tafUpdateSvr.hpp"
#include "tafRoutineCtrlSvr.hpp"
#include "tafResetSvr.hpp"
#include "tafIOCtrlSvr.hpp"
#include "configuration.hpp"
#include "tafDiagBackend.hpp"
#include "tafDiagSvr.hpp"
#include "tafIDPSSvr.hpp"

#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafEventSvr.hpp"
#include "tafSnapshotSvc.hpp"
#include "tafDTCInf.hpp"
#include "tafDTCSvr.hpp"
#include "tafDiagDoIPSvr.hpp"
#include "tafAuthSvr.hpp"
#include "tafROESvr.hpp"
#endif

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF diag service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("TelAF Diag service initialization start!");

    auto& diag = taf_DiagSvr::GetInstance();
    diag.Init();

    auto& did = taf_DataIDSvr::GetInstance();
    did.Init();

    auto& tafSecurity = taf_SecuritySvr::GetInstance();
    tafSecurity.Init();

    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();
    tafUpdateSvr.Init();

    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();
    tafRCS.Init();

    auto &reset = taf_ResetSvr::GetInstance();
    reset.Init();

    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    ioCtrl.Init();

#ifndef LE_CONFIG_DIAG_VSTACK
    auto& event = taf_EventSvr::GetInstance();
    event.Init();

    auto& dtcSvc = taf_DTCSvr::GetInstance();
    dtcSvc.Init();

    taf_DataAccess_Init();

    auto& dtcInf = taf_DTCInf::GetInstance();
    dtcInf.Init();

    auto& snapshot = taf_SnapshotSvr::GetInstance();
    snapshot.Init();

    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();
    doipSvc.Init();

    auto& authSvc = taf_AuthSvr::GetInstance();
    authSvc.Init();

    auto& ROESvc = taf_ROESvr::GetInstance();
    ROESvc.Init();

    auto& tafBackend = taf_DiagBackend::GetInstance();
    tafBackend.Init();

#endif

    if(cfg::IsIDPSAvailable())
    {
        LE_INFO("init IDPS");
        auto& idpsSvc = taf_IdpsSvr::GetInstance();
        idpsSvc.Init();
    }
    LE_INFO("TelAF Diag service initialization completed!");

    // Add boot KPI marker
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    const char *kpi_marker = "L - TelAF diagnostic service is ready";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_ERROR("%s does not exist", kpi_file);
        return;
    }
    if (fwrite(kpi_marker, sizeof(char), strlen(kpi_marker), file) != strlen(kpi_marker))
    {
        LE_ERROR("failed to write %s to %s", kpi_marker, kpi_file);
    }
    fclose(file);
}
