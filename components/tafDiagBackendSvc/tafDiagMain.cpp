/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafDiagStackInf.hpp"
#include "tafDiagBackendSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("Initialization !");

    LE_DEBUG("Diag stack interface Initialization started!");
    auto &diagStack = taf_DiagStack::GetInstance();
    diagStack.Init();
    LE_DEBUG("Diag stack interface Initialization completed!");

    LE_DEBUG("Backend service Initialization started!");
    auto &diagBackend = taf_DiagBackend::GetInstance();
    diagBackend.Init();
    LE_DEBUG("Backend service Initialization completed!");

}