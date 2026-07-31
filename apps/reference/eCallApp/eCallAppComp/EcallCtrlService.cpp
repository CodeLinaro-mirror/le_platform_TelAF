/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// EcallCtrlService.cpp
//
// Implements the ctrlEcall Legato IPC service handlers.
// Legato auto-generates the ctrlEcall_* function signatures from ctrlEcall.api.
// This file must be compiled as a plain C++ translation unit with no
// namespace ecall includes, to avoid conflicts with the generated interfaces.h.

extern "C"
{
#include "legato.h"
#include "interfaces.h"
}

#include "EcallController.hpp"

le_result_t ctrlEcall_StartAutomatic(void)
{
    return ecall::EcallController::GetInstance().StartAutomatic();
}

le_result_t ctrlEcall_StartManual(void)
{
    return ecall::EcallController::GetInstance().StartManual();
}

le_result_t ctrlEcall_StartTest(void)
{
    return ecall::EcallController::GetInstance().StartTest();
}

le_result_t ctrlEcall_End(void)
{
    return ecall::EcallController::GetInstance().End();
}

le_result_t ctrlEcall_CpuStart(void)
{
    return ecall::EcallController::GetInstance().CpuStart();
}

le_result_t ctrlEcall_CpuStop(void)
{
    return ecall::EcallController::GetInstance().CpuStop();
}
