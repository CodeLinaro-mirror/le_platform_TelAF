/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafDataIDSvr.hpp"
#include "tafSecuritySvr.hpp"

#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafRoutineCtrlSvr.hpp"
#include "tafResetSvr.hpp"
#include "tafUpdateSvr.hpp"
#include "tafEventSvr.hpp"
#include "configuration.hpp"
#include "tafDTCInf.hpp"
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

    LE_INFO("TelAF UDS DTC interface initialization start...");
    auto& dtcInf = taf_DTCInf::GetInstance();
    dtcInf.Init();
    LE_INFO("TelAF UDS DTC interface initialization end...");

    LE_INFO("TelAF Diag Backend initialization start...");
    auto& tafBackend = taf_DiagBackend::GetInstance();
    tafBackend.Init();
    LE_INFO("TelAF Diag Backend initialization end...");
#endif
}
