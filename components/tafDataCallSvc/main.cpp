/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file main.cpp
 * @brief TelAF Data Call Service main entry point
 *
 */
#include "legato.h"
#include "tafDcs.hpp"
#include "tafDcsProfile.hpp"

using namespace taf::svc::datacall;

static void DeferredDCSInitFunc(void *param1Ptr, void *param2Ptr)
{
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    // Initialize the profile manager.
    LE_DEBUG("Initializing profile manager...");
    auto &tafDcsProfileManager = TafDcsProfileManager::GetInstance();
    // TODO: Add return values and check init status
    tafDcsProfileManager.Init();

    // Read and update the data profiles. TODO: Add return values and check init state
    tafDcsProfileManager.InitProfiles();
    return;

}

// The data call service's signal handler
static void DcsSigTermEventHandler
(
    int sigNum
)
{
    LE_INFO("Signal :%d", sigNum);

    // Call deinit function to cleanup
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    tafDcsSvc.Deinit();
    exit(EXIT_SUCCESS);
}


COMPONENT_INIT
{
    LE_INFO("Data Call Service Component Init");

    // Setup signal event handler.
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, DcsSigTermEventHandler);

    // Check if the PA service is initialized.
    auto &tafDcsSvc = TafDcsSvc::GetInstance();
    tafDcsSvc.Init();
    if (taf::pa::data::SubsystemState_e::AVAILABLE != tafDcsSvc.GetInitState())
    {
        // Kill the service as the PA is not ready
        LE_FATAL("PA service is not initialized");
    }

    // Add boot KPI marker
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    const char *kpi_marker = "L - TelAF data call service is ready";
    FILE *file = fopen(kpi_file, "w");
    if (file != NULL)
    {
        if (fwrite(kpi_marker, sizeof(char), strlen(kpi_marker), file) != strlen(kpi_marker))
            LE_ERROR("failed to write %s to %s", kpi_marker, kpi_file);

        fclose(file);
    }
    else
        LE_ERROR("%s does not exist", kpi_file);

    // Complete the initialization of DCS profile manager in the background and return from here.
    le_event_QueueFunction(DeferredDCSInitFunc, nullptr, nullptr);
}
