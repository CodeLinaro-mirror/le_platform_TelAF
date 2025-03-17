/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string>

namespace telux {
namespace tafsvc {

    const signed int TAF_MNGDPM_MAX_TRIGGER_REGISTERS = 20;

    const std::string TAF_MNGDPM_CONFIGURATION_PATH
            ("/data/ManagedServices/tafMngdPMSvc.json");

    const std::string TAF_MNGDPM_DEFAULT_CONF_PATH
            ("/legato/systems/current/appsWriteable/tafMngdPMSvc/tafMngdPMSvc.json");
}
}