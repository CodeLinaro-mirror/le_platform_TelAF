/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDataIDSvr.hpp"
#include "tafSecuritySvr.hpp"
#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafDiagBackend.hpp"
#include "tafDataIDSvr.hpp"
#include "tafSecuritySvr.hpp"

#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafRoutineCtrlSvr.hpp"
#include "tafResetSvr.hpp"
#include "tafUpdateSvr.hpp"
#include "tafIOCtrlSvr.hpp"
#include "tafEventSvr.hpp"
#include "tafSnapshotSvc.hpp"
#include "configuration.hpp"
#include "tafDTCInf.hpp"
#include "tafDTCSvr.hpp"
#include "tafDiagDoIPSvr.hpp"
#endif

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF diag service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    LE_INFO("TelAF UDS DataID service initialization start...");
    auto& did = taf_DataIDSvr::GetInstance();
    did.Init();
    LE_INFO("TelAF UDS DataID service initialization end...");

    LE_INFO("TelAF UDS Security service initialization start...");
    auto& tafSecurity = taf_SecuritySvr::GetInstance();
    tafSecurity.Init();
    LE_INFO("TelAF UDS Security service initialization end...");

#ifndef LE_CONFIG_DIAG_VSTACK
    try
    {
        cfg::diag_config_init("./diag_template.yaml.json");
    }
    catch (const std::exception& e)
    {
        LE_FATAL("json file is not present");
    }

    LE_INFO("TelAF UDS routine conctrol service initialization start...");
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();
    tafRCS.Init();
    LE_INFO("TelAF UDS routine conctrol service initialization end...");

    LE_INFO("TelAF UDS update service initialization start...");
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();
    tafUpdateSvr.Init();
    LE_INFO("TelAF UDS update service initialization end...");

    auto &reset = taf_ResetSvr::GetInstance();
    reset.Init();
    LE_INFO("TelAF UDS update service initialization end...");

    LE_INFO("TelAF Event Management service initialization start...");
    auto& event = taf_EventSvr::GetInstance();
    event.Init();
    LE_INFO("TelAF Event Management service initialization end...");

    LE_INFO("TelAF DTC service initialization start...");
    auto& dtcSvc = taf_DTCSvr::GetInstance();
    dtcSvc.Init();
    LE_INFO("TelAF DTC service initialization end...");

    LE_INFO("TelAF UDS DTC interface initialization start...");
    auto& dtcInf = taf_DTCInf::GetInstance();
    dtcInf.Init();
    LE_INFO("TelAF UDS DTC interface initialization end...");

    LE_INFO("TelAF Snapshot service initialization start...");
    auto& snapshot = taf_SnapshotSvr::GetInstance();
    snapshot.Init();
    LE_INFO("TelAF Snapshot service initialization end...");

    LE_INFO("TelAF DoIP service initialization start...");
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();
    doipSvc.Init();
    LE_INFO("TelAF DoIP service initialization end...");

    LE_INFO("TelAF IOCtrl service initialization start...");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    ioCtrl.Init();
    LE_INFO("TelAF IOCtrl service initialization end...");

    LE_INFO("TelAF Diag Backend initialization start...");
    auto& tafBackend = taf_DiagBackend::GetInstance();
    tafBackend.Init();
    LE_INFO("TelAF Diag Backend initialization end...");

#endif
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
